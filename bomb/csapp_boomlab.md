# BoomLab
这个实验非常有意思，需要利用csapp里面第三章学到的x86-64汇编知识以及一些逆向的技巧，来破解程序的的意图，体会“拆弹”的快感，成为代码世界的“拆弹专家”！。
## 实验目标
对于给出的一个二进制文件，寻找正确的输入，当输入错误的时候，会提示“Boom!”意味着拆弹失败，二进制炸弹爆炸。实验总共有6个普通关卡和一个附加关卡<del>MIT也开始搞DLC</del>,也就是说，总共需要破解7个输入。
## 前置内容
### 材料准备
下载并阅读<a href="http://csapp.cs.cmu.edu/3e/bomblab.pdf">实验指导书</a>
下载并解压缩<a href="http://csapp.cs.cmu.edu/3e/bomb.tar">实验材料</a>
部署gcc、objdump、gdb环境。
<del>肥宅快乐水</del>

### x86_64 汇编的参数传递
编译器会按照函数参数从左到右的顺序，依次分配以下寄存器。

| 操作数大小 | 1    | 2    | 3    | 4    | 5    | 6    |
| ---------- | ---- | ---- | ---- | ---- | ---- | ---- |
| 64         | %rdi | %rsi | %rdx | %rcx | %r8  | %r9  |
| 32         | %edi | %esi | %edx | %ecx | %r8d | %r9d |
| 16         | %di  | %si  | %dx  | %cx  | %r8w | %r9w |
| 8          | %dil | %sil | %dl  | %cl  | %r8d | %r9b |

而对于一个函数的返回值，如果是非浮点数，一般是存储在%rax或者相关寄存器。
### 大小端
x86-64处理器采用小端法存储数据，也就是数据的高位字节存储在低地址处。
### gdb调试工具
对于gdb调试，我列出几条这个实验用的最多的命令。
>break function 在function函数的入口处打一个断点
break *address 在地址address处打一个断点

example:
>break main 在main函数入口处打一个断点
break *0x4c00 在地址0x4c00处打一个断点

>run            开始运行
stepi           单步执行
step n          单步执行n步
continue        执行到下一个断点
info register   查看寄存器的值

特别的，对于打印值，给出网上的资料
>x/<n/f/u> <addr>
n、f、u是可选的参数，<addr>表示一个内存地址
1） n 是一个正整数，表示显示内存的长度，也就是说从当前地址向后显示几个地址的内容
2） f 表示显示的格式
3） u 表示将多少个字节作为一个值取出来，如果不指定的话，GDB默认是4个bytes，如果不指定的话，默认是4个bytes。当我们指定了字节长度后，GDB会从指内存定的内存地址开始，读写指定字节，并把其当作一个值取出来。
参数 f 的可选值：
x 按十六进制格式显示变量。
d 按十进制格式显示变量。
u 按十六进制格式显示无符号整型。
o 按八进制格式显示变量。
t 按二进制格式显示变量。
a 按十六进制格式显示变量。
c 按字符格式显示变量。
f 按浮点数格式显示变量。
s 按字符串格式显示变量
参数 u 可以用下面的字符来代替：
b 表示单字节
h 表示双字节
w 表示四字节
g 表示八字节

对于此次试验，用的比较多的例子是:
>x/s 0x4ab    打印地址0x4ab处的字符串


### dumpobj工具
objdump -t    加载二进制文件的符号表
objdump -d    加载二进制文件的汇编代码
### python
因为objdump之后的汇编代码里面有大量的十六进制运算，故我采用python作为一个简单的计算器，在python中可以这样进行计算。
```python
a=0xa
b=0xb
c=a+b
print("%#x",%c)
```
上面这段代码表示将0xa和0xb加起来并且以十六进制的格式打印

## 准备工作
首先采用下面两条命令保存符号表和汇编代码，方便查看。
```common
#objdump -t bomb &>tag
#objdump -d bomb &>asm
```
接着可以打开符号表，这里面存储了该二进制文件的符号信息。我们只关心第三列，如果第三列为F，表示为function，是一个函数名。
## 拆弹开始!
凭借经验，我们猜测这个东西，是以main作为入口的，<del>因为main是c语言的入口，而csapp里面都是c语言233</del>，去tag里面搜索main，果然有！那么转到asm看汇编代码，全局搜索main，共有8次记录，很轻易的我们就找到了main函数对应部分的汇编代码。是以这样的形式开头的，这也是汇编函数开头的形式:
```asm
0000000000400da0 <main>:
  400da0:	53                   	push   %rbx
  400da1:	83 ff 01             	cmp    $0x1,%edi
  400da4:	75 10                	jne    400db6 <main+0x16>
```
表示main函数地址为0x0000000000400da0，第一行代码是push %rbx，对应的机器码是53，该行代码地址为400da0。通看这一段代码，可以发现一大堆没啥用的部分，对于拆弹没有关系，比如:
```asm
  400dc7:	e8 44 fe ff ff       	callq  400c10 <fopen@plt>
```
表明使用了fopen函数，这个可能是用来支持直接从文件中读入而不用每次都手动输入的这个特性。往后我们看到了有
```asm
400e19:	e8 84 05 00 00       	callq  4013a2 <initialize_bomb>
400e1e:	bf 38 23 40 00       	mov    $0x402338,%edi
400e23:	e8 e8 fc ff ff       	callq  400b10 <puts@plt>
400e28:	bf 78 23 40 00       	mov    $0x402378,%edi
400e2d:	e8 de fc ff ff       	callq  400b10 <puts@plt>
400e32:	e8 67 06 00 00       	callq  40149e <read_line>
400e37:	48 89 c7             	mov    %rax,%rdi
400e3a:	e8 a1 00 00 00       	callq  400ee0 <phase_1>
400e3f:	e8 80 07 00 00       	callq  4015c4 <phase_defused>
400e44:	bf a8 23 40 00       	mov    $0x4023a8,%edi
400e49:	e8 c2 fc ff ff       	callq  400b10 <puts@plt>
400e4e:	e8 4b 06 00 00       	callq  40149e <read_line>
400e53:	48 89 c7             	mov    %rax,%rdi
400e56:	e8 a1 00 00 00       	callq  400efc <phase_2>
400e5b:	e8 64 07 00 00       	callq  4015c4 <phase_defused>
400e60:	bf ed 22 40 00       	mov    $0x4022ed,%edi
400e65:	e8 a6 fc ff ff       	callq  400b10 <puts@plt>
400e6a:	e8 2f 06 00 00       	callq  40149e <read_line>
400e6f:	48 89 c7             	mov    %rax,%rdi
400e72:	e8 cc 00 00 00       	callq  400f43 <phase_3>
400e77:	e8 48 07 00 00       	callq  4015c4 <phase_defused>
```
先是init_bomb，感觉上可能密码藏在这里面，然后搜索跳转过去发现啥也没有<del>显然不会这么简单啊！</del>然后是两个put一个read_line接一个phase_i和phase_defuse。而如果运行bomb，会发现他的交互模式是先打印几句话，然后让你输入一句话，然后判断是否正确。于是猜测puts是没啥卵用的<del>很显然嘛，怎么可能有用</del>,read_line是用来读入的，可能在读入里面进行了判断，然后跳转过去，发现啥也没有<del>傻啊你，怎么可能会在readline里面判断</del>，于是代码核心部分应该在phase_i和phase_defused，而我们发现phase_defused是六次相同的函数，直接翻过去看这个函数，发现里面应该没有利用static或者全局变量判断第几次进入的方法，大概率密码不会藏在里面，于是乎，结果就只能在phase_i里面了！
### phase_1
第一个关卡，正式开始我们的拆弹之旅！搜索phase_1，找到了如下代码。
```asm
0000000000400ee0 <phase_1>:
  400ee0:	48 83 ec 08          	sub    $0x8,%rsp
  400ee4:	be 00 24 40 00       	mov    $0x402400,%esi
  400ee9:	e8 4a 04 00 00       	callq  401338 <strings_not_equal>
  400eee:	85 c0                	test   %eax,%eax
  400ef0:	74 05                	je     400ef7 <phase_1+0x17>
  400ef2:	e8 43 05 00 00       	callq  40143a <explode_bomb>
  400ef7:	48 83 c4 08          	add    $0x8,%rsp
  400efb:	c3                   	retq
```
非常的短，第一关显然也不会那么难。读一下结构，先分配了8大小的栈空间，然后把一个值丢到了%esi里面，然后调用了strings_not_equal，判断返回值是否为0，如果为0就跳转到$phase_1+0x17$的位置，否则就调用explode_bomb，凭借多年的代码下毒经验，我猜测strings_not_equal和explode_bomb的函数功能不会完全与名字一致<del>自己下毒也别怀疑别人下毒啊喂</del>于是乎花费了大量时间进去找找找，结果发现自己是个sb。
排除了这种可能，我们重新观察这段代码，strings_not_equal显然是要比较两个字符串是否相等，这两个字符串如何传入呢？我们发现这样一段写死的代码
```asm
  400ee4:	be 00 24 40 00       	mov    $0x402400,%esi
```
把$0x402400的值送到%esi里面，紧接着就调用了string_not_equal!esi是函数的第二个参数，而$0x402400是一个立即数，与我们的输入无关，密码应该与0x402400有关！然后尝试去tag和asm里面全局搜索这个值，一无所获。<del>傻了吧</del>，于是乎尝试用gdb运行bomb，在phase_1打上断点，用字符串形式打印这个地址的值，有了!
```common
Border relations with Canada have never been better.  
```
这显然是一串有意义的字符串而非乱码，那就是他了！输入这串字符串，顺利过关。
### phase_2
有了前一关的经验，直接跳到phase_2的汇编代码，这就有那么一点长了。
```asm
0000000000400efc <phase_2>:
  400efc:	55                   	push   %rbp
  400efd:	53                   	push   %rbx
  400efe:	48 83 ec 28          	sub    $0x28,%rsp
  400f02:	48 89 e6             	mov    %rsp,%rsi
  400f05:	e8 52 05 00 00       	callq  40145c <read_six_numbers>
  400f0a:	83 3c 24 01          	cmpl   $0x1,(%rsp)
  400f0e:	74 20                	je     400f30 <phase_2+0x34>
  400f10:	e8 25 05 00 00       	callq  40143a <explode_bomb>
  400f15:	eb 19                	jmp    400f30 <phase_2+0x34>
  400f17:	8b 43 fc             	mov    -0x4(%rbx),%eax
  400f1a:	01 c0                	add    %eax,%eax
  400f1c:	39 03                	cmp    %eax,(%rbx)
  400f1e:	74 05                	je     400f25 <phase_2+0x29>
  400f20:	e8 15 05 00 00       	callq  40143a <explode_bomb>
  400f25:	48 83 c3 04          	add    $0x4,%rbx
  400f29:	48 39 eb             	cmp    %rbp,%rbx
  400f2c:	75 e9                	jne    400f17 <phase_2+0x1b>
  400f2e:	eb 0c                	jmp    400f3c <phase_2+0x40>
  400f30:	48 8d 5c 24 04       	lea    0x4(%rsp),%rbx
  400f35:	48 8d 6c 24 18       	lea    0x18(%rsp),%rbp
  400f3a:	eb db                	jmp    400f17 <phase_2+0x1b>
  400f3c:	48 83 c4 28          	add    $0x28,%rsp
  400f40:	5b                   	pop    %rbx
  400f41:	5d                   	pop    %rbp
  400f42:	c3                   	retq
```
看了又看看了又看，我觉得玄机应该在read_six_numbers里面！
```asm
000000000040145c <read_six_numbers>:
  40145c:	48 83 ec 18          	sub    $0x18,%rsp
  401460:	48 89 f2             	mov    %rsi,%rdx
  401463:	48 8d 4e 04          	lea    0x4(%rsi),%rcx
  401467:	48 8d 46 14          	lea    0x14(%rsi),%rax
  40146b:	48 89 44 24 08       	mov    %rax,0x8(%rsp)
  401470:	48 8d 46 10          	lea    0x10(%rsi),%rax
  401474:	48 89 04 24          	mov    %rax,(%rsp)
  401478:	4c 8d 4e 0c          	lea    0xc(%rsi),%r9
  40147c:	4c 8d 46 08          	lea    0x8(%rsi),%r8
  401480:	be c3 25 40 00       	mov    $0x4025c3,%esi
  401485:	b8 00 00 00 00       	mov    $0x0,%eax
  40148a:	e8 61 f7 ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
  40148f:	83 f8 05             	cmp    $0x5,%eax
  401492:	7f 05                	jg     401499 <read_six_numbers+0x3d>
  401494:	e8 a1 ff ff ff       	callq  40143a <explode_bomb>
  401499:	48 83 c4 18          	add    $0x18,%rsp
  40149d:	c3                   	retq
```
根据上面的经验，我们发现了这样一串代码
```asm
  401480:	be c3 25 40 00       	mov    $0x4025c3,%esi
```
难道说.....胜利就在眼前？
结果打印出来是...
```common
%d %d %d %d %d %d
```
啊血崩，被毒瘤了。不过这也给了我们一个启示，这第二个实验要求输入六个数字，为了考察这六个数字存储的位置，尝试在phase_2的0x400f0a处打上断点，运行程序。先输入第一关的密码，然后随便输入六个数字，观察phase_2的代码，我觉得读入的结果应该放在了$rsp处，因为他通过%rsi传入了read_six_numbers函数，于是打印从$rsp开始的一段空间的值，发现六个数字是顺序存放的，也就是说read_six_numbers函数并没有下毒，比如交换位置以增大破解难度什么的<del>这证明不是所有人都像我这么无聊</del>。现在要开始正式的破解了！
假设我的输入是
```common
  a b c d e f
```
这六个数字，那么$\%rsp+(i-1)\*4$是输入的第i个数字的地址。观察phase_2的代码，显示调用了读入，然后判断(%rsp)是否为1，如果为1跳转到$phase_2+0x34$，否则就调用explode_bomb，那么很明显，a必须是1。找到$phase_2+0x34$处的代码，把b的地址赋给$rbx,然后把$rsp+0x18$这个值给了%rbp，因为$4*6=24=0x18$所以这个地址是比f还要大4。然后跳转到了$phase_2+0x40$处，把%rbx-4这个地址处的值赋予eax，然后eax乘2，判断是否等于%rbx地址处的值，不等就bomb....脑内过一遍代码之后，发现了这个逻辑，也就是说a必须是1，然后b、c、d、e、f必须是前一个数字的两倍，所以显然的答案就是
```common
1 2 4 8 16 32
```
过关!
### phase_3
这一关开始我有了经验<del>并没有人在代码里面下毒</del>，找到phase_3的代码，通览一遍。
```asm
0000000000400f43 <phase_3>:
  400f43:	48 83 ec 18          	sub    $0x18,%rsp
  400f47:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx
  400f4c:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx
  400f51:	be cf 25 40 00       	mov    $0x4025cf,%esi
  400f56:	b8 00 00 00 00       	mov    $0x0,%eax
  400f5b:	e8 90 fc ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
  400f60:	83 f8 01             	cmp    $0x1,%eax
  400f63:	7f 05                	jg     400f6a <phase_3+0x27>
  400f65:	e8 d0 04 00 00       	callq  40143a <explode_bomb>
  400f6a:	83 7c 24 08 07       	cmpl   $0x7,0x8(%rsp)
  400f6f:	77 3c                	ja     400fad <phase_3+0x6a>
  400f71:	8b 44 24 08          	mov    0x8(%rsp),%eax
  400f75:	ff 24 c5 70 24 40 00 	jmpq   *0x402470(,%rax,8)
  400f7c:	b8 cf 00 00 00       	mov    $0xcf,%eax
  400f81:	eb 3b                	jmp    400fbe <phase_3+0x7b>
  400f83:	b8 c3 02 00 00       	mov    $0x2c3,%eax
  400f88:	eb 34                	jmp    400fbe <phase_3+0x7b>
  400f8a:	b8 00 01 00 00       	mov    $0x100,%eax
  400f8f:	eb 2d                	jmp    400fbe <phase_3+0x7b>
  400f91:	b8 85 01 00 00       	mov    $0x185,%eax
  400f96:	eb 26                	jmp    400fbe <phase_3+0x7b>
  400f98:	b8 ce 00 00 00       	mov    $0xce,%eax
  400f9d:	eb 1f                	jmp    400fbe <phase_3+0x7b>
  400f9f:	b8 aa 02 00 00       	mov    $0x2aa,%eax
  400fa4:	eb 18                	jmp    400fbe <phase_3+0x7b>
  400fa6:	b8 47 01 00 00       	mov    $0x147,%eax
  400fab:	eb 11                	jmp    400fbe <phase_3+0x7b>
  400fad:	e8 88 04 00 00       	callq  40143a <explode_bomb>
  400fb2:	b8 00 00 00 00       	mov    $0x0,%eax
  400fb7:	eb 05                	jmp    400fbe <phase_3+0x7b>
  400fb9:	b8 37 01 00 00       	mov    $0x137,%eax
  400fbe:	3b 44 24 0c          	cmp    0xc(%rsp),%eax
  400fc2:	74 05                	je     400fc9 <phase_3+0x86>
  400fc4:	e8 71 04 00 00       	callq  40143a <explode_bomb>
  400fc9:	48 83 c4 18          	add    $0x18,%rsp
  400fcd:	c3                   	retq
```
发现有一个sscanf和$0x4025cf这个魔法数被存到了esi，也就是sscanf的第二个参数，而我们知道sscanf第二个参数是format<del>怎么现在才想起来</del>。打印一下，果不其然
```common
%d %d
```
也就是说要输入两个数，往后查找第三第四个参数%dx和%cx，发现这两个值是栈上分别偏移0x8和0xc的地址值。往后看，杜如以后先判断是否等于一，如果是就bomb，不然就跳转到0x400f6a,跳转之后比较一下第一个读入的值和7的关系，如果大于7直接Bomb，不然就继续，然后后面是一长串代码。注意这一串代码
```common
400f75:	ff 24 c5 70 24 40 00 	jmpq   *0x402470(,%rax,8)
```
虽然不知道0x402470从哪来的，但是这串代码实现了一个很简单的功能，根据%rax的值去跳转到对应的位置，类似于switch case的功能，这是很显然的，往后看正好有7个mov jmp 的组合，猜测是从0-7对应下面的每一组mov jmp组合，这个组合实现了这样一个功能，这样这么长一段代码实现了这样一个功能，也就是根据a的值来比较b的值，也就是a、b有以下的对应关系

| a   | b     |
| --- | ----- |
| 0   | 0xcf  |
| 1   | 0x2c3 |
| 2   | 0x100 |
| 3   | 0x185 |
| 4   | 0xce  |
| 5   | 0x2aa |
| 6   | 0x147 |
当一切遵从直觉之后，拆弹变得这么简单!<del>离死不远了</del>，然后测试，0正确，1...1bomb!为啥bomb了。。显然直觉出现了问题。反过头去看汇编代码，数了一下mov和jmp。发现mov + jmp是7个字节而非8个字节！这就很头疼了，需要去找一下0x402470是什么玩意儿，然后才能计算跳转的位置。找啊找找啊找并不能从代码中找到这个东西，于是尝试一下减少一下匹配，一位一位的添加，结果发现一个事实，前面的各种字符串常量，比如"%d %d"等都是用$0x4024**$指出的。。于是我们猜测这个值存放在内存中，gdb打印内存，果然
```common
(gdb) x/x 0x402470
0x402470:	0x00400f7c
```
<del>于是我们找到了起始地址为0x400f7c，所以跳转的地址应该是$0x400f7c+8*a$</deL>谁告诉你的啊喂！作者居然在代码里下毒了！跳转地址并非顺序存放的！使用下面这条命令后，一切都揭晓了！
```common
x/16x 0x402470
```
结果为
```common
(gdb) x/16x 0x402470
0x402470:	0x00400f7c	0x00000000	0x00400fb9	0x00000000
0x402480:	0x00400f83	0x00000000	0x00400f8a	0x00000000
0x402490:	0x00400f91	0x00000000	0x00400f98	0x00000000
0x4024a0:	0x00400f9f	0x00000000	0x00400fa6	0x00000000
```
于是我们就可以找到正确的对应关系，这次没错了！

| a   | b     |
| --- | ----- |
| 0   | 0xcf  |
| 1   | 0x137 |
| 2   | 0x2c3 |
| 3   | 0x100 |
| 4   | 0x185 |
| 5   | 0xce  |
| 6   | 0x2aa |
| 7   | 0x147 |

### phase_4
进入了紧张的中场阶段！<del>我好像掌握了作者的思路!</del>
```asm

000000000040100c <phase_4>:
  40100c:	48 83 ec 18          	sub    $0x18,%rsp
  401010:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx
  401015:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx
  40101a:	be cf 25 40 00       	mov    $0x4025cf,%esi
  40101f:	b8 00 00 00 00       	mov    $0x0,%eax
  401024:	e8 c7 fb ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
  401029:	83 f8 02             	cmp    $0x2,%eax
  40102c:	75 07                	jne    401035 <phase_4+0x29>
  40102e:	83 7c 24 08 0e       	cmpl   $0xe,0x8(%rsp)
  401033:	76 05                	jbe    40103a <phase_4+0x2e>
  401035:	e8 00 04 00 00       	callq  40143a <explode_bomb>
  40103a:	ba 0e 00 00 00       	mov    $0xe,%edx
  40103f:	be 00 00 00 00       	mov    $0x0,%esi
  401044:	8b 7c 24 08          	mov    0x8(%rsp),%edi
  401048:	e8 81 ff ff ff       	callq  400fce <func4>
  40104d:	85 c0                	test   %eax,%eax
  40104f:	75 07                	jne    401058 <phase_4+0x4c>
  401051:	83 7c 24 0c 00       	cmpl   $0x0,0xc(%rsp)
  401056:	74 05                	je     40105d <phase_4+0x51>
  401058:	e8 dd 03 00 00       	callq  40143a <explode_bomb>
  40105d:	48 83 c4 18          	add    $0x18,%rsp
  401061:	c3                   	retq
```
还是熟悉的配方!还是熟悉的参数!不过这里多了个func4,仔细观察func4的代码
```asm
0000000000400fce <func4>:
  400fce:	48 83 ec 08          	sub    $0x8,%rsp
  400fd2:	89 d0                	mov    %edx,%eax
  400fd4:	29 f0                	sub    %esi,%eax
  400fd6:	89 c1                	mov    %eax,%ecx
  400fd8:	c1 e9 1f             	shr    $0x1f,%ecx
  400fdb:	01 c8                	add    %ecx,%eax
  400fdd:	d1 f8                	sar    %eax
  400fdf:	8d 0c 30             	lea    (%rax,%rsi,1),%ecx
  400fe2:	39 f9                	cmp    %edi,%ecx
  400fe4:	7e 0c                	jle    400ff2 <func4+0x24>
  400fe6:	8d 51 ff             	lea    -0x1(%rcx),%edx
  400fe9:	e8 e0 ff ff ff       	callq  400fce <func4>
  400fee:	01 c0                	add    %eax,%eax
  400ff0:	eb 15                	jmp    401007 <func4+0x39>
  400ff2:	b8 00 00 00 00       	mov    $0x0,%eax
  400ff7:	39 f9                	cmp    %edi,%ecx
  400ff9:	7d 0c                	jge    401007 <func4+0x39>
  400ffb:	8d 71 01             	lea    0x1(%rcx),%esi
  400ffe:	e8 cb ff ff ff       	callq  400fce <func4>
  401003:	8d 44 00 01          	lea    0x1(%rax,%rax,1),%eax
  401007:	48 83 c4 08          	add    $0x8,%rsp
  40100b:	c3                   	retq
```
emmm有一点麻烦了，这好像还是递归的。因为已经知道这玩意儿有三个参数，所以尝试将他翻译成c语言代码，忽略前面调用部分，只考虑func4函数，有三个参数，分别在%rdi,%rsi,%rdx。然后分析这段函数，发现并没有保存和弹出，猜测应该是引用传参，于是写出c代码(注意sar %eax这个指令，猜测是右移一位省略了$1这个参数)
```cpp
int func4(int &a,int &b,int &c)
{
  int x=c-b;
  if(x<0)
  {
    x=(x+1);
  }
  x=x/2;
  int y=x+b;
  if(y<=a)
  {
    x=0;
    if(y>=a)
    {
      return x;
    }else
    {
      b=y+1;
      func4(a,b,c);
      x=2*x+1;
      return x;
    }
  }else
  {
    c=y-1;
    func4(a,b,c);
    x=2*x;
    return x;
  }
}

```
直接翻译过来很难理解，做一点简单的变动，让他变得更好理解一些，事实上上面的代码等价于下面的代码
```cpp
int func4(int &a,int &b,int &c)
{
  int x;
  if(c-b<0)
  {
    x=(c-b+1)/2;
  }else
  {
    x=(c-b)/2;
  }
  int y=x+b;
  if(y==a)
  {
    return 0;
  }else if(y<a)
  {
    b=y+1;
    func4(a,b,c);
    x=2*x+1;
    return x;
  }else
  {
    c=y-1;
    func4(a,b,c);
    x=2*x;
    return x;
  }
}
```
仔细分析上面代码的这个片段
```cpp
{
  int x;
  if(c-b<0)
  {
    x=(c-b+1)/2;
  }else
  {
    x=(c-b)/2;
  }
  int y=x+b;
}
```

猜测y的值应该是 $(b+c)/2$，那么上面这段代码可以这样理解，如果 $\frac{b+c}{2}=a$ 返回值为0，否则当 $\frac{b+c}{2}<a$的时候让$b$加上1然后递归，返回值乘2倍加上1，否则就$b$减少1，返回值乘以2倍。
奇了怪了，这段代码到底在算些什么呢?但事实上我们不需要关心这段代码到底tmd在算啥，我们直接hack掉，找一组最直接的样例构造我们想要的结果即可！我们往回去看phase_4的代码，读入两个数ab，判断第一个数小于等于0xe，否则就爆掉，小于等于的话调用func4(0xe,0,a),然后判断返回值如果不为0，爆掉，否则看第二个数是否等于0，等于0顺利跳出。那么我们可以这样构造，先让$b=0$，再去考虑怎么让func4返回值为0，直接考虑$a=0xe/2$即可!测试一下
```common
Welcome to my fiendish little bomb. You have 6 phases with
which to blow yourself up. Have a nice day!
Border relations with Canada have never been better.
Phase 1 defused. How about the next one?
1 2 4 8 16 32
That's number 2.  Keep going!
0 207
Halfway there!
7 0
So you got that one.  Try this one.
```
通过!这个故事告诉我们，看不看得懂不重要，能构造出数据就行(逃
### phase_5
熟练地找到对应的汇编代码，然后认真<del>xjr</del>分析一波
```asm
0000000000401062 <phase_5>:
  401062:	53                   	push   %rbx
  401063:	48 83 ec 20          	sub    $0x20,%rsp
  401067:	48 89 fb             	mov    %rdi,%rbx
  40106a:	64 48 8b 04 25 28 00 	mov    %fs:0x28,%rax
  401071:	00 00
  401073:	48 89 44 24 18       	mov    %rax,0x18(%rsp)
  401078:	31 c0                	xor    %eax,%eax
  40107a:	e8 9c 02 00 00       	callq  40131b <string_length>
  40107f:	83 f8 06             	cmp    $0x6,%eax
  401082:	74 4e                	je     4010d2 <phase_5+0x70>
  401084:	e8 b1 03 00 00       	callq  40143a <explode_bomb>
  401089:	eb 47                	jmp    4010d2 <phase_5+0x70>
  40108b:	0f b6 0c 03          	movzbl (%rbx,%rax,1),%ecx
  40108f:	88 0c 24             	mov    %cl,(%rsp)
  401092:	48 8b 14 24          	mov    (%rsp),%rdx
  401096:	83 e2 0f             	and    $0xf,%edx
  401099:	0f b6 92 b0 24 40 00 	movzbl 0x4024b0(%rdx),%edx
  4010a0:	88 54 04 10          	mov    %dl,0x10(%rsp,%rax,1)
  4010a4:	48 83 c0 01          	add    $0x1,%rax
  4010a8:	48 83 f8 06          	cmp    $0x6,%rax
  4010ac:	75 dd                	jne    40108b <phase_5+0x29>
  4010ae:	c6 44 24 16 00       	movb   $0x0,0x16(%rsp)
  4010b3:	be 5e 24 40 00       	mov    $0x40245e,%esi
  4010b8:	48 8d 7c 24 10       	lea    0x10(%rsp),%rdi
  4010bd:	e8 76 02 00 00       	callq  401338 <strings_not_equal>
  4010c2:	85 c0                	test   %eax,%eax
  4010c4:	74 13                	je     4010d9 <phase_5+0x77>
  4010c6:	e8 6f 03 00 00       	callq  40143a <explode_bomb>
  4010cb:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  4010d0:	eb 07                	jmp    4010d9 <phase_5+0x77>
  4010d2:	b8 00 00 00 00       	mov    $0x0,%eax
  4010d7:	eb b2                	jmp    40108b <phase_5+0x29>
  4010d9:	48 8b 44 24 18       	mov    0x18(%rsp),%rax
  4010de:	64 48 33 04 25 28 00 	xor    %fs:0x28,%rax
  4010e5:	00 00
  4010e7:	74 05                	je     4010ee <phase_5+0x8c>
  4010e9:	e8 42 fa ff ff       	callq  400b30 <__stack_chk_fail@plt>
  4010ee:	48 83 c4 20          	add    $0x20,%rsp
  4010f2:	5b                   	pop    %rbx
  4010f3:	c3                   	retq
```
看一下结构，读一个串，判断长度是不是6，然后做一堆操作，然后movzbl $0x4024b0(%rdx)，看这个地址貌似是常量区，然后又做一堆操作，判断字符串是否相等。咦，那么猜测字符串应该在$0x4024b0附近，先gdb看一下，看到这样的结果,咦，怎么结果不对呢？那么让我们跳到strings_not_equal附近，找一下比较的是哪两个字符串。第一个参数是$0x10(%rsp)，第二个参数是$0x40245e,有了!
```common
(gdb) x/s 0x40245e
0x40245e:	"flyers"
```
刚好长度为6！，难道说....
```common
Welcome to my fiendish little bomb. You have 6 phases with
which to blow yourself up. Have a nice day!
Border relations with Canada have never been better.
Phase 1 defused. How about the next one?
1 2 4 8 16 32
That's number 2.  Keep going!
0 207
Halfway there!
7 0
So you got that one.  Try this one.
flyers

BOOM!!!
The bomb has blown up.
```
果不其然<del>怎么可能这么简单啊喂</del>，但是参数应该是没有错的，那么就是在读入的串上做了点手脚！我们现在去寻找读入的字符串和$0x10(%rsp)之间的联系!输入串abcdef进去看看断点打到0x4010b8(比较字符串之前)，看看这串内存到底tmd是啥东西,结果发现目标位置变成了
```common
aduier
```
至此我们大致明白了这一关的流程，读一个串，对串做改造，看看是否等于目标串，也就是说如果改造过程为$f(x)$，那么就是要求$f(x)="flyers"$的$x$的值，也就是 $x=f^{-1}("flyers")$。那么就需要知道 $f^{-1}(x)$ 的具体情况！
仔细考察这个过程，先读入一个串，地址为%rdi,然后一波神奇的操作之后地址处第一个字符给了%ecx,然后我们注意到了这个过程
```asm
401099:	0f b6 92 b0 24 40 00 	movzbl 0x4024b0(%rdx),%edx
4010a0:	88 54 04 10          	mov    %dl,0x10(%rsp,%rax,1)
4010a4:	48 83 c0 01          	add    $0x1,%rax
4010a8:	48 83 f8 06          	cmp    $0x6,%rax
4010ac:	75 dd                	jne    40108b <phase_5+0x29>
```
0x6!，做六次循环，处理过程应该就是在这里没错了!把0x4024b0(%rdx)间接寻址后的值给了相应的字符串位，%rdx又是什么呢？往上找找，发现了
```asm
40108f:	88 0c 24             	mov    %cl,(%rsp)
401092:	48 8b 14 24          	mov    (%rsp),%rdx
401096:	83 e2 0f             	and    $0xf,%edx
```
咦，%cx是读入的第一个字符，把第一个字符的低4位给了%rdx，高位全0，然后就到了最上面那一串寻址，这不就是switch case嘛，把整个循环部分贴出来更利于寻找答案.
```asm
40108b:	0f b6 0c 03          	movzbl (%rbx,%rax,1),%ecx
40108f:	88 0c 24             	mov    %cl,(%rsp)
401092:	48 8b 14 24          	mov    (%rsp),%rdx
401096:	83 e2 0f             	and    $0xf,%edx
401099:	0f b6 92 b0 24 40 00 	movzbl 0x4024b0(%rdx),%edx
4010a0:	88 54 04 10          	mov    %dl,0x10(%rsp,%rax,1)
4010a4:	48 83 c0 01          	add    $0x1,%rax
4010a8:	48 83 f8 06          	cmp    $0x6,%rax
4010ac:	75 dd                	jne    40108b <phase_5+0x29>
```
上面这段代码循环6次，也就是说把6个字符的每个字符的ascii码低4位取出来，去做一次间接寻址，送到目的地。
那么我们去找找0x4024b0处到底都是些什么东西!
```common
(gdb) x/s 0x4024b0
0x4024b0 <array.3449>:	"maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?"
```
找到了!
```common
maduiersnfotvbyl
```
刚好16个字符,后面恰好是一段话！也就是说这16个字符就是对应的映射表！
| 序号 | 字符 |
| ---- | ---- |
| 0    | m    |
| 1    | a    |
| 2    | d    |
| 3    | u    |
| 4    | i    |
| 5    | e    |
| 6    | r    |
| 7    | s    |
| 8    | n    |
| 9    | f    |
| 10   | o    |
| 11   | t    |
| 12   | v    |
| 13   | b    |
| 14   | y    |
| 15   | l    |

为了拼凑出flyers,我们分别需要输入的字符串分别为序号9、15、14、5、 6、 7。显然结果不唯一1!那么我们就构造一组简单的结果，想到字符a的ascii码为97,而 $97\&0x1=1$，也就是说a的序号为1，然后在模16的情况下依次往后增加！显然，我们可以利用字符串"ionefg"来hack掉这个炸弹!
```common
Good work!  On to the next...
```
顺利拆弹!
### phase_6
来到了除dlc以外的最后一关，这一关应该会有点难度。
```asm
00000000004010f4 <phase_6>:
  4010f4:	41 56                	push   %r14
  4010f6:	41 55                	push   %r13
  4010f8:	41 54                	push   %r12
  4010fa:	55                   	push   %rbp
  4010fb:	53                   	push   %rbx
  4010fc:	48 83 ec 50          	sub    $0x50,%rsp
  401100:	49 89 e5             	mov    %rsp,%r13
  401103:	48 89 e6             	mov    %rsp,%rsi
  401106:	e8 51 03 00 00       	callq  40145c <read_six_numbers>
  40110b:	49 89 e6             	mov    %rsp,%r14
  40110e:	41 bc 00 00 00 00    	mov    $0x0,%r12d
  401114:	4c 89 ed             	mov    %r13,%rbp
  401117:	41 8b 45 00          	mov    0x0(%r13),%eax
  40111b:	83 e8 01             	sub    $0x1,%eax
  40111e:	83 f8 05             	cmp    $0x5,%eax
  401121:	76 05                	jbe    401128 <phase_6+0x34>
  401123:	e8 12 03 00 00       	callq  40143a <explode_bomb>
  401128:	41 83 c4 01          	add    $0x1,%r12d
  40112c:	41 83 fc 06          	cmp    $0x6,%r12d
  401130:	74 21                	je     401153 <phase_6+0x5f>
  401132:	44 89 e3             	mov    %r12d,%ebx
  401135:	48 63 c3             	movslq %ebx,%rax
  401138:	8b 04 84             	mov    (%rsp,%rax,4),%eax
  40113b:	39 45 00             	cmp    %eax,0x0(%rbp)
  40113e:	75 05                	jne    401145 <phase_6+0x51>
  401140:	e8 f5 02 00 00       	callq  40143a <explode_bomb>
  401145:	83 c3 01             	add    $0x1,%ebx
  401148:	83 fb 05             	cmp    $0x5,%ebx
  40114b:	7e e8                	jle    401135 <phase_6+0x41>
  40114d:	49 83 c5 04          	add    $0x4,%r13
  401151:	eb c1                	jmp    401114 <phase_6+0x20>
  401153:	48 8d 74 24 18       	lea    0x18(%rsp),%rsi
  401158:	4c 89 f0             	mov    %r14,%rax
  40115b:	b9 07 00 00 00       	mov    $0x7,%ecx
  401160:	89 ca                	mov    %ecx,%edx
  401162:	2b 10                	sub    (%rax),%edx
  401164:	89 10                	mov    %edx,(%rax)
  401166:	48 83 c0 04          	add    $0x4,%rax
  40116a:	48 39 f0             	cmp    %rsi,%rax
  40116d:	75 f1                	jne    401160 <phase_6+0x6c>
  40116f:	be 00 00 00 00       	mov    $0x0,%esi
  401174:	eb 21                	jmp    401197 <phase_6+0xa3>
  401176:	48 8b 52 08          	mov    0x8(%rdx),%rdx
  40117a:	83 c0 01             	add    $0x1,%eax
  40117d:	39 c8                	cmp    %ecx,%eax
  40117f:	75 f5                	jne    401176 <phase_6+0x82>
  401181:	eb 05                	jmp    401188 <phase_6+0x94>
  401183:	ba d0 32 60 00       	mov    $0x6032d0,%edx
  401188:	48 89 54 74 20       	mov    %rdx,0x20(%rsp,%rsi,2)
  40118d:	48 83 c6 04          	add    $0x4,%rsi
  401191:	48 83 fe 18          	cmp    $0x18,%rsi
  401195:	74 14                	je     4011ab <phase_6+0xb7>
  401197:	8b 0c 34             	mov    (%rsp,%rsi,1),%ecx
  40119a:	83 f9 01             	cmp    $0x1,%ecx
  40119d:	7e e4                	jle    401183 <phase_6+0x8f>
  40119f:	b8 01 00 00 00       	mov    $0x1,%eax
  4011a4:	ba d0 32 60 00       	mov    $0x6032d0,%edx
  4011a9:	eb cb                	jmp    401176 <phase_6+0x82>
  4011ab:	48 8b 5c 24 20       	mov    0x20(%rsp),%rbx
  4011b0:	48 8d 44 24 28       	lea    0x28(%rsp),%rax
  4011b5:	48 8d 74 24 50       	lea    0x50(%rsp),%rsi
  4011ba:	48 89 d9             	mov    %rbx,%rcx
  4011bd:	48 8b 10             	mov    (%rax),%rdx
  4011c0:	48 89 51 08          	mov    %rdx,0x8(%rcx)
  4011c4:	48 83 c0 08          	add    $0x8,%rax
  4011c8:	48 39 f0             	cmp    %rsi,%rax
  4011cb:	74 05                	je     4011d2 <phase_6+0xde>
  4011cd:	48 89 d1             	mov    %rdx,%rcx
  4011d0:	eb eb                	jmp    4011bd <phase_6+0xc9>
  4011d2:	48 c7 42 08 00 00 00 	movq   $0x0,0x8(%rdx)
  4011d9:	00
  4011da:	bd 05 00 00 00       	mov    $0x5,%ebp
  4011df:	48 8b 43 08          	mov    0x8(%rbx),%rax
  4011e3:	8b 00                	mov    (%rax),%eax
  4011e5:	39 03                	cmp    %eax,(%rbx)
  4011e7:	7d 05                	jge    4011ee <phase_6+0xfa>
  4011e9:	e8 4c 02 00 00       	callq  40143a <explode_bomb>
  4011ee:	48 8b 5b 08          	mov    0x8(%rbx),%rbx
  4011f2:	83 ed 01             	sub    $0x1,%ebp
  4011f5:	75 e8                	jne    4011df <phase_6+0xeb>
  4011f7:	48 83 c4 50          	add    $0x50,%rsp
  4011fb:	5b                   	pop    %rbx
  4011fc:	5d                   	pop    %rbp
  4011fd:	41 5c                	pop    %r12
  4011ff:	41 5d                	pop    %r13
  401201:	41 5e                	pop    %r14
  401203:	c3                   	retq
```
这一关我们看到了熟悉的read_six_number!因为前面已经探讨过了这个函数的功能<del>没有人在这段代码下毒</del>，于是我们就不再进去仔细看这段代码的逻辑。这次我们速战速决，不去分析read之前的代码了，直接上断点到read之后，然后看一下寄存器，找到对应的值存在哪，输入1 2 3 4 5 6，然后
```common
(gdb) x/20d $rsp
0x7fffffffdc70:	1	2	3	4
0x7fffffffdc80:	5	6	0	0
0x7fffffffdc90:	-8752	32767	0	0
0x7fffffffdca0:	0	0	4199372	0
0x7fffffffdcb0:	6306064	0	6306064	0
```
显然，六个数字完整的摆在了栈顶。假设分别为abcdef,接着把第一个值给了r14,r12,rbp,eax，然后判断eax-1和5的大小，比他大就爆掉，所以 $a<=5$,然后判%r12和6的关系，如果等于6跳转，否则不跳转，这里显然 $a=5$ 是一个极其特殊的情况，为了hack掉这个炸弹，我们忽略细节往他想要的答案处构造，直接让$a=5$，跟着跳转过去。把7减去 $a$,也就是 $\%edx=7-5=2$,又把%edx送给了a,一连串操作之后a变成了2。然后的然后发现了重点！
```asm
401160:	89 ca                	mov    %ecx,%edx
401162:	2b 10                	sub    (%rax),%edx
401164:	89 10                	mov    %edx,(%rax)
401166:	48 83 c0 04          	add    $0x4,%rax
40116a:	48 39 f0             	cmp    %rsi,%rax
40116d:	75 f1                	jne    401160 <phase_6+0x6c>
```
之前的操作中让rsi变成了偏移0x18的位置，也就是第六个数字之后的位置，那么这一段代码的意图就很显然了，把六个数字都做一遍处理，用7减去然后再放回去。
但这都不是重点，我们往后看，%r12d为1往下执行，到了后面用了一行间接寻址
```asm
  401138:	8b 04 84             	mov    (%rsp,%rax,4),%eax
```
把第二个数取了出来，发现和%rbp做了比较，%rbp往前找发现值为栈顶也就是间接寻址到第一个数字,必须不相等，否则爆掉！然后执行了上面我们所说那段循环。并且我们在此处找到了一串循环
```asm
401114:	4c 89 ed             	mov    %r13,%rbp
401117:	41 8b 45 00          	mov    0x0(%r13),%eax
40111b:	83 e8 01             	sub    $0x1,%eax
40111e:	83 f8 05             	cmp    $0x5,%eax
401121:	76 05                	jbe    401128 <phase_6+0x34>
401123:	e8 12 03 00 00       	callq  40143a <explode_bomb>
401128:	41 83 c4 01          	add    $0x1,%r12d
40112c:	41 83 fc 06          	cmp    $0x6,%r12d
401130:	74 21                	je     401153 <phase_6+0x5f>
401132:	44 89 e3             	mov    %r12d,%ebx
401135:	48 63 c3             	movslq %ebx,%rax
401138:	8b 04 84             	mov    (%rsp,%rax,4),%eax
40113b:	39 45 00             	cmp    %eax,0x0(%rbp)
40113e:	75 05                	jne    401145 <phase_6+0x51>
401140:	e8 f5 02 00 00       	callq  40143a <explode_bomb>
401145:	83 c3 01             	add    $0x1,%ebx
401148:	83 fb 05             	cmp    $0x5,%ebx
40114b:	7e e8                	jle    401135 <phase_6+0x41>
40114d:	49 83 c5 04          	add    $0x4,%r13
401151:	eb c1                	jmp    401114 <phase_6+0x20>
```
这段代码检查所有值是否有相等且是否有大于6的，如果有就爆掉。如果没有爆掉就执行上面讨论的那段循环，进行值的变化。
然后紧接着又来了一大段的循环<del>内心崩溃，就不能简单点吗？</del>
```asm
401176:	48 8b 52 08          	mov    0x8(%rdx),%rdx
40117a:	83 c0 01             	add    $0x1,%eax
40117d:	39 c8                	cmp    %ecx,%eax
40117f:	75 f5                	jne    401176 <phase_6+0x82>
401181:	eb 05                	jmp    401188 <phase_6+0x94>
401183:	ba d0 32 60 00       	mov    $0x6032d0,%edx
401188:	48 89 54 74 20       	mov    %rdx,0x20(%rsp,%rsi,2)
40118d:	48 83 c6 04          	add    $0x4,%rsi
401191:	48 83 fe 18          	cmp    $0x18,%rsi
401195:	74 14                	je     4011ab <phase_6+0xb7>
401197:	8b 0c 34             	mov    (%rsp,%rsi,1),%ecx
40119a:	83 f9 01             	cmp    $0x1,%ecx
40119d:	7e e4                	jle    401183 <phase_6+0x8f>
40119f:	b8 01 00 00 00       	mov    $0x1,%eax
4011a4:	ba d0 32 60 00       	mov    $0x6032d0,%edx
4011a9:	eb cb                	jmp    401176 <phase_6+0x82>
```
猜测这应该是一段while循环，跳出条件可能是处理完了所有的数字跳到0x4011ab(很像jump to middle的翻译方式)。然后我们看到rdx和一串地址0x6032d0有关，打印一下。
```common
(gdb) x/40d 0x6032d0
0x6032d0 <node1>:	76	1	0	0	1	0	0	0
0x6032d8 <node1+8>:	-32	50	96	0	0	0	0	0
0x6032e0 <node2>:	-88	0	0	0	2	0	0	0
0x6032e8 <node2+8>:	-16	50	96	0	0	0	0	0
0x6032f0 <node3>:	-100	3	0	0	3	0	0	0
```
node，这难道是一个结构体？多打印一点出来试试
```asm
(gdb) x/100 0x6032d0
0x6032d0 <node1>:	76	1	0	0	1	0	0	0
0x6032d8 <node1+8>:	-32	50	96	0	0	0	0	0
0x6032e0 <node2>:	-88	0	0	0	2	0	0	0
0x6032e8 <node2+8>:	-16	50	96	0	0	0	0	0
0x6032f0 <node3>:	-100	3	0	0	3	0	0	0
0x6032f8 <node3+8>:	0	51	96	0	0	0	0	0
0x603300 <node4>:	-77	2	0	0	4	0	0	0
0x603308 <node4+8>:	16	51	96	0	0	0	0	0
0x603310 <node5>:	-35	1	0	0	5	0	0	0
0x603318 <node5+8>:	32	51	96	0	0	0	0	0
0x603320 <node6>:	-69	1	0	0	6	0	0	0
0x603328 <node6+8>:	0	0	0	0	0	0	0
0x603330:	0	0	0	0
```
咦，我们读入六个数字，这里也只标记到6，会不会有什么阴谋呢？反正最终一定和这串地址有关，那我们仔细观察一下。感觉不太明显，换16进制试试
```common
0x6032d0 <node1>:	0x4c	0x01	0x00	0x00	0x01	0x00	0x00	0x00
0x6032d8 <node1+8>:	0xe0	0x32	0x60	0x00	0x00	0x00	0x00	0x00
0x6032e0 <node2>:	0xa8	0x00	0x00	0x00	0x02	0x00	0x00	0x00
0x6032e8 <node2+8>:	0xf0	0x32	0x60	0x00	0x00	0x00	0x00	0x00
0x6032f0 <node3>:	0x9c	0x03	0x00	0x00	0x03	0x00	0x00	0x00
0x6032f8 <node3+8>:	0x00	0x33	0x60	0x00	0x00	0x00	0x00	0x00
0x603300 <node4>:	0xb3	0x02	0x00	0x00	0x04	0x00	0x00	0x00
0x603308 <node4+8>:	0x10	0x33	0x60	0x00	0x00	0x00	0x00	0x00
0x603310 <node5>:	0xdd	0x01	0x00	0x00	0x05	0x00	0x00	0x00
0x603318 <node5+8>:	0x20	0x33	0x60	0x00	0x00	0x00	0x00	0x00
0x603320 <node6>:	0xbb	0x01	0x00	0x00	0x06	0x00	0x00	0x00
0x603328 <node6+8>:	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
```
还是没什么思路，去看看代码，发现有这么一串
```asm
401176:	48 8b 52 08          	mov    0x8(%rdx),%rdx
```
好像和偏移地址0x8有关，仔细看一下所有的node+8的位置，考虑到x86采用小端法，那么就要倒过来才能看明白。观察第一个，倒过来看值是
```asm
0x00000000006032e0
```
这个地址不就正好在这一段里面吗！这是个指针，恰好也是8字节！而第六个指向了0x0，在c语言里面代表NULL，难道说这是个链表？而我们知道read_six_num读入六个int型数据，应该是四字节，也就是说最后对比大概率也是四字节，再加上之前我们分析出读入的数据应该在1-6之间，观察一下 我们猜测node+4的位置存放了我们的数据。我们从NULL入手倒着去找他们之间的连接关系，发现答案可能是:$6<-5<-4<-3<-2<-1$的倒叙。但总感觉哪里不太对劲。这个顺序太特殊了，试一下，果然爆掉了。现在还剩前面四个字节没有使用，苦思冥想，还是想不出来。看了下别人的解法<del>实在是不想读那么长的汇编</del>，发现这是一个结构体，类似
```cpp
struct node{
  int value;
  int i;
  node *next;
}node;
```
前四个字节应该是对应的值，后四个字节是对应的序号，对这六个节点进行排序之后，按输出序号(注意小端法的特点)。
<del>于是结果为:3 4 5 6 1 2</del>
boom!傻了吧！网传教程早已经过时了，自力更生吧!
于是我们想到，之前一长串代码使得所有的值都变成了$7-x$，那么会不会结果在这上面改造呢？尝试性输入4 3 2 1 6 5
```common
Good work!  On to the next...
4 3 2 1 6 5
Congratulations! You've defused the bomb!
```
果然！顺利结束,差点以为要去看那一长串汇编(逃。

### DLC
#### 被"欺骗"的单身狗
8月23日，凌晨五点，天好热，电费好贵(电费*3)，作为一个上进的单身狗，我沉浸在代码的世界里，<del>顺利</del>解决掉了六个问题，程序对我表示了祝贺。
>Congratulations! You've defused the bomb!

突然，我发现我被"歧视"了，或许因为我不是“付费”玩家的缘故，我没有找到实验指导书上告诉我的一个隐藏关卡，程序就直接结束了...为了探寻“充值入口”，我打开了符号表，搜索phase，找到了这个函数secret_phase。可是入口在哪呢？直接全局搜索，发现他出现在了phase_defused里面。
```asm
00000000004015c4 <phase_defused>:
  4015c4:	48 83 ec 78          	sub    $0x78,%rsp
  4015c8:	64 48 8b 04 25 28 00 	mov    %fs:0x28,%rax
  4015cf:	00 00
  4015d1:	48 89 44 24 68       	mov    %rax,0x68(%rsp)
  4015d6:	31 c0                	xor    %eax,%eax
  4015d8:	83 3d 81 21 20 00 06 	cmpl   $0x6,0x202181(%rip)     # 603760 <num_input_strings>
  4015df:	75 5e                	jne    40163f <phase_defused+0x7b>
  4015e1:	4c 8d 44 24 10       	lea    0x10(%rsp),%r8
  4015e6:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx
  4015eb:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx
  4015f0:	be 19 26 40 00       	mov    $0x402619,%esi
  4015f5:	bf 70 38 60 00       	mov    $0x603870,%edi
  4015fa:	e8 f1 f5 ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
  4015ff:	83 f8 03             	cmp    $0x3,%eax
  401602:	75 31                	jne    401635 <phase_defused+0x71>
  401604:	be 22 26 40 00       	mov    $0x402622,%esi
  401609:	48 8d 7c 24 10       	lea    0x10(%rsp),%rdi
  40160e:	e8 25 fd ff ff       	callq  401338 <strings_not_equal>
  401613:	85 c0                	test   %eax,%eax
  401615:	75 1e                	jne    401635 <phase_defused+0x71>
  401617:	bf f8 24 40 00       	mov    $0x4024f8,%edi
  40161c:	e8 ef f4 ff ff       	callq  400b10 <puts@plt>
  401621:	bf 20 25 40 00       	mov    $0x402520,%edi
  401626:	e8 e5 f4 ff ff       	callq  400b10 <puts@plt>
  40162b:	b8 00 00 00 00       	mov    $0x0,%eax
  401630:	e8 0d fc ff ff       	callq  401242 <secret_phase>
  401635:	bf 58 25 40 00       	mov    $0x402558,%edi
  40163a:	e8 d1 f4 ff ff       	callq  400b10 <puts@plt>
  40163f:	48 8b 44 24 68       	mov    0x68(%rsp),%rax
  401644:	64 48 33 04 25 28 00 	xor    %fs:0x28,%rax
  40164b:	00 00
  40164d:	74 05                	je     401654 <phase_defused+0x90>
  40164f:	e8 dc f4 ff ff       	callq  400b30 <__stack_chk_fail@plt>
  401654:	48 83 c4 78          	add    $0x78,%rsp
```
名称叫拆弹，里面确大有玄机。而这个函数出现在了每一个关卡的后面，说明隐藏关一定有一个特殊的唤醒机制。而这个函数里面居然出现了sscanf和strings_not_equal，不符合正常操作的逻辑，也就是说，这里面对一个我们读入的字符串进行了判断。按照惯例，我们去找比较的两个字符串，找到了这两行
```asm
401604:	be 22 26 40 00       	mov    $0x402622,%esi
401609:	48 8d 7c 24 10       	lea    0x10(%rsp),%rdi
```
也就是说在比较一个栈上的字符串和一个常量区的字符串，打印0x402622，看看是什么东西.
```common
(gdb) x/s 0x402622
0x402622:	"DrEvil"
```
果然，是一串有意义的字符串。可是，要怎么触发这个机制呢？经过测试，发现只有当到达最后一关，也就是第六关的时候，会走到一个奇怪的位置
```asm
4015fa:	e8 f1 f5 ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
4015ff:	83 f8 03             	cmp    $0x3,%eax
401602:	75 31                	jne    401635 <phase_defused+0x71>
```
而跳转过去的位置恰好是秘密关卡的后面，所以如果要进入秘密关卡，就不能跳转！看到判断sscanf的返回值是否等于3，心里已经有了点数。再往后，看到了一个字符串比较，于是打上断点，看看都发生了什么
```common
(gdb) x/s 0x402619
0x402619:	"%d %d %s"
(gdb) x/s 0x603870
0x603870 <input_strings+240>:	"7 0"
```
打印前面的常量，发现了一个奇怪的串和一个奇怪的位置。%d%d%s应该是读入两个数字和一个字符串，从0x603870处读入，而0x603870是"7 0"，是我们在第二关输入的答案！尝试在第二关后面加一点东西hack一下，果然这样使得sscanf成功读入了三个数据。加什么字符串呢？往后看看到了字符串比较，那就是他了！
```common
Welcome to my fiendish little bomb. You have 6 phases with
which to blow yourself up. Have a nice day!
Border relations with Canada have never been better.
Phase 1 defused. How about the next one?
1 2 4 8 16 32
That's number 2.  Keep going!
0 207
Halfway there!
7 0 DrEvil
So you got that one.  Try this one.
ionefg
Good work!  On to the next...
4 3 2 1 6 5
Curses, you've found the secret phase!
But finding it and solving it are quite different...
```
果然触发了DLC剧情!
#### 进入正题

```asm
0000000000401242 <secret_phase>:
  401242:	53                   	push   %rbx
  401243:	e8 56 02 00 00       	callq  40149e <read_line>
  401248:	ba 0a 00 00 00       	mov    $0xa,%edx
  40124d:	be 00 00 00 00       	mov    $0x0,%esi
  401252:	48 89 c7             	mov    %rax,%rdi
  401255:	e8 76 f9 ff ff       	callq  400bd0 <strtol@plt>
  40125a:	48 89 c3             	mov    %rax,%rbx
  40125d:	8d 40 ff             	lea    -0x1(%rax),%eax
  401260:	3d e8 03 00 00       	cmp    $0x3e8,%eax
  401265:	76 05                	jbe    40126c <secret_phase+0x2a>
  401267:	e8 ce 01 00 00       	callq  40143a <explode_bomb>
  40126c:	89 de                	mov    %ebx,%esi
  40126e:	bf f0 30 60 00       	mov    $0x6030f0,%edi
  401273:	e8 8c ff ff ff       	callq  401204 <fun7>
  401278:	83 f8 02             	cmp    $0x2,%eax
  40127b:	74 05                	je     401282 <secret_phase+0x40>
  40127d:	e8 b8 01 00 00       	callq  40143a <explode_bomb>
  401282:	bf 38 24 40 00       	mov    $0x402438,%edi
  401287:	e8 84 f8 ff ff       	callq  400b10 <puts@plt>
  40128c:	e8 33 03 00 00       	callq  4015c4 <phase_defused>
  401291:	5b                   	pop    %rbx
```
看到几串非常奇怪的代码,先进行简单的分析.
```asm
401248:	ba 0a 00 00 00       	mov    $0xa,%edx
40124d:	be 00 00 00 00       	mov    $0x0,%esi
401252:	48 89 c7             	mov    %rax,%rdi
401255:	e8 76 f9 ff ff       	callq  400bd0 <strtol@plt>
40125a:	48 89 c3             	mov    %rax,%rbx
40125d:	8d 40 ff             	lea    -0x1(%rax),%eax
401260:	3d e8 03 00 00       	cmp    $0x3e8,%eax
401265:	76 05                	jbe    40126c <secret_phase+0x2a>
```
调用了strtol，这个函数需要三个参数，第一个为字符串，第二个为endptr,第三个为int型，表示base，然后把字符串转化为长整形数。也就是说这个地方把一串数字字符串转化成了一个整数然后与0x3e8进行了比较。而$0x3e8=1000$。再仔细看传入的参数，第三个表示base的参数为$0xa=10$,也就是10进制。
非常短的代码，根据前面的分析，读一行之后进行进制转换，然后判断减１是否小于等于0x3e8,如果是，就把这个数和一串地址传入fun7，然后判断返回值是否等于２．
```asm
0000000000401204 <fun7>:
  401204:	48 83 ec 08          	sub    $0x8,%rsp
  401208:	48 85 ff             	test   %rdi,%rdi
  40120b:	74 2b                	je     401238 <fun7+0x34>
  40120d:	8b 17                	mov    (%rdi),%edx
  40120f:	39 f2                	cmp    %esi,%edx
  401211:	7e 0d                	jle    401220 <fun7+0x1c>
  401213:	48 8b 7f 08          	mov    0x8(%rdi),%rdi
  401217:	e8 e8 ff ff ff       	callq  401204 <fun7>
  40121c:	01 c0                	add    %eax,%eax
  40121e:	eb 1d                	jmp    40123d <fun7+0x39>
  401220:	b8 00 00 00 00       	mov    $0x0,%eax
  401225:	39 f2                	cmp    %esi,%edx
  401227:	74 14                	je     40123d <fun7+0x39>
  401229:	48 8b 7f 10          	mov    0x10(%rdi),%rdi
  40122d:	e8 d2 ff ff ff       	callq  401204 <fun7>
  401232:	8d 44 00 01          	lea    0x1(%rax,%rax,1),%eax
  401236:	eb 05                	jmp    40123d <fun7+0x39>
  401238:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
  40123d:	48 83 c4 08          	add    $0x8,%rsp
  401241:	c3                   	retq
```
fun7中我们可以发现，这玩意儿tmd还是一个递归的东西。于是我们尝试将他翻译成c语言
```cpp
int fun7(int* a,int b)
{
  if(a==NULL)
  {
    return -1;
  }
  int y=*a;
  if(y==b)
  {
    return 0;
  }else if(y<b)
  {
    a=*(a+0x10);
    int q=fun7(a,b);
    return 2*q+1;
  }else
  {
    a=*(a+0x8);
    int q=fun7(a,b);
    return 2*q;
  }
}
```
仔细理一下思路，如果a是空指针返回－１，否则比较a处的值和b的关系，相等返回0，其他情况在往后走8字节或者16字节然后**取出当前的值作为地址**。而我们要的结果是２，只需要构造一个返回值为２的情况就行了!打印a初始值表示的地址附近的值。
```common
(gdb) x/128d 0x6030f0
0x6030f0 <n1>:	36	0	6304016	0
0x603100 <n1+16>:	6304048	0	0	0
0x603110 <n21>:	8	0	6304144	0
0x603120 <n21+16>:	6304080	0	0	0
0x603130 <n22>:	50	0	6304112	0
0x603140 <n22+16>:	6304176	0	0	0
0x603150 <n32>:	22	0	6304368	0
0x603160 <n32+16>:	6304304	0	0	0
0x603170 <n33>:	45	0	6304208	0
0x603180 <n33+16>:	6304400	0	0	0
0x603190 <n31>:	6	0	6304240	0
0x6031a0 <n31+16>:	6304336	0	0	0
0x6031b0 <n34>:	107	0	6304272	0
0x6031c0 <n34+16>:	6304432	0	0	0
0x6031d0 <n45>:	40	0	0	0
0x6031e0 <n45+16>:	0	0	0	0
0x6031f0 <n41>:	1	0	0	0
0x603200 <n41+16>:	0	0	0	0
0x603210 <n47>:	99	0	0	0
0x603220 <n47+16>:	0	0	0	0
0x603230 <n44>:	35	0	0	0
0x603240 <n44+16>:	0	0	0	0
0x603250 <n42>:	7	0	0	0
0x603260 <n42+16>:	0	0	0	0
0x603270 <n43>:	20	0	0	0
0x603280 <n43+16>:	0	0	0	0
0x603290 <n46>:	47	0	0	0
0x6032a0 <n46+16>:	0	0	0	0
0x6032b0 <n48>:	1001	0	0	0
0x6032c0 <n48+16>:	0	0	0	0

```
首先断定一定是$y=b$才能跳出递归，其次$b\leq 1001$总是成立,而且因为每一次递归结果都会至少翻一倍，所以猜测结果一定在不远处。那么我们只考虑8的倍数的地址，一个一个看过去看到22的时候发现，22的时候返回值刚好是2!
```common
Welcome to my fiendish little bomb. You have 6 phases with
which to blow yourself up. Have a nice day!
Border relations with Canada have never been better.
Phase 1 defused. How about the next one?
1 2 4 8 16 32
That's number 2.  Keep going!
0 207
Halfway there!
7 0 DrEvil
So you got that one.  Try this one.
ionefg
Good work!  On to the next...
4 3 2 1 6 5
Curses, you've found the secret phase!
But finding it and solving it are quite different...
22
Wow! You've defused the secret stage!
Congratulations! You've defused the bomb!
```
## 总结
这个实验非常有意思，花了我两天时间来完成，一开始耐性还不错，直接读汇编去找答案，但是到第六关真的崩溃了，这么长一段汇编，读了快一个小时，分析出输入的数字必须是1-6，然后找到了一个莫名奇妙的地址之后实在是心态爆炸，于是去参考了<a href="https://wdxtub.com/2016/04/16/thick-csapp-lab-2/">不周山Lab解析</a>，这也是本次实验最大的遗憾之处<del>最大的黑点</del>，没有完全独立自主的完成。假期也快要结束，csapp之旅却还刚刚开始!
