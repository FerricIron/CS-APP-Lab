# CacheLab
这一章我们实战操作，编写一个模拟程序模拟存储器的缓存机制!
## 实验目标
通过存储器模拟实验和矩阵优化实验，实践cs:app第六章的内容，体会存储器体系的原理及学习如何利用局部性去优化程序性能。
## 前置内容
## 材料准备
<a href="http://csapp.cs.cmu.edu/3e/cachelab.pdf">实验指导书</a>
<a href="http://csapp.cs.cmu.edu/3e/cachelab-handout.tar">实验材料</a>
## 实验过程
### Reference Trace Files
#### 实验内容
这一部分的实验中，需要补全给出的csim.c文件，实现一个与给出的样例程序csim-ref反馈一致的缓存模拟器。
在这一部分的实验中需要接触到valgrind输出的文件,大概格式为
```common
I 0400d7d4,8
  M 0421c7f0,4
  L 04f6b868,8
  S 7ff0005c8,8
```
其格式为
```common
[space]operation address,size
```
operation部分如果为"I"表示为指令读入，这是不影响缓存系统的。"M"表示修改,"L"表示读取,"S"表示写入。显然"M"可以看做"LS"的组合。address是一个**64位**的地址，size表示读入的地址块大小(实际上本实验中并未用到)。
现在需要模拟一个缓存系统，大致可以参考书p426的图6-25，统计命中，未命中，"驱逐(淘汰)"的次数，并与标准样例进行比较，得到分数(满分27)。
#### 注意细节
* **本次实验的地址为64位地址，以16进制给出**
* I是指令读，不算入内存操作
* M一定会导致一次hit
* 利用scanf类似的函数读入给出的文件的时候"%s,%c"这样的format是无效的，后面的%c实际上是读不到的，需要自己写read函数
* 命令行解析很毒瘤，请使用getop
* 实验使用的make规则把所有warning看做error，所以该加括号的地方请加上括号.

#### 参考代码
```cpp
#include "cachelab.h"
#include <unistd.h>
#include<stdio.h>
#include<math.h>
#include<stdlib.h>
#include<string.h>
#include <getopt.h>
#define true 1
#define false 0
#define bool int
#define maxFileSize 300
#define maxAddrSize 30+5
#define maxBufSize 300
#define Debug 0
char readBuf[maxBufSize];
int args_s,args_E,args_b;
bool flag_h,flag_v;
int args_t;
char tracefile[maxFileSize];
int parseConsole(int argc,char *args[])
{
    char ch;
    while((ch=getopt(argc,args,"hvs:E:b:t:"))!=-1)
    {
        switch (ch)
        {
            case 'h':flag_h=true;break;
            case 'v':flag_v=true;break;
            case 's':sscanf(optarg,"%d",&args_s);
                break;
            case 'E':sscanf(optarg,"%d",&args_E);break;
            case 'b':sscanf(optarg,"%d",&args_b);
                break;
            case 't':sscanf(optarg,"%s",tracefile);break;
            default:
                return -1;
        }
    }
    return 0;
}
typedef struct CacheLine{
    bool isAlive;
    int flag;
    int acceseTime;
}Line;
typedef struct Cache{
    Line *line;
}Cache;

void setCache(Cache *cache, int cacheLineNumber) {
    cache->line = (Line *) malloc(sizeof(Line) * cacheLineNumber);
    for (int i = 0; i < cacheLineNumber; ++i) {
        cache->line[i].isAlive = false;
        cache->line[i].flag = 0;
        cache->line[i].acceseTime = 0;
    }
}
void parseFlag(char* addr, int* flag, int* cacheSetFlag)
{
    unsigned int tempAddr;
    sscanf(addr,"%x",&tempAddr);
    tempAddr=tempAddr>>args_b;
    if(Debug)
    {
        printf("%x\n",tempAddr);
    }
    int retFlag,retCacheSetFlag;
    retFlag=retCacheSetFlag=0;
    for(int i=args_s-1;i>=0;--i)
    {
        retCacheSetFlag=retCacheSetFlag<<1;
        if(((tempAddr>>i)&1)==1)
        {
            retCacheSetFlag+=1;
        }
    }
    tempAddr=tempAddr>>args_s;
    for(int i=args_t-1;i>=0;--i)
    {
        retFlag=retFlag<<1;
        if(((tempAddr>>i)&1)==1)
        {
            retFlag+=1;
        }
    }
    if(Debug)
    {
        printf("Flag: %d\n",retFlag);
    }
    *flag=retFlag;
    *cacheSetFlag=retCacheSetFlag;
    return ;
}
bool readData(char *op,char *addr,int *size,FILE* file)
{
    //char ch;
    //while((ch==getchar()))
    char *flag=NULL;
    flag=fgets(readBuf,60,file);
    *op='#';
    if(flag==NULL||feof(file))
    {
        return 0;
    }
    int len=strlen(readBuf);
    int i;
    for(i=0;i<len;++i)
    {
        if(readBuf[i]=='\r'||readBuf[i]=='\n')
        {
            continue;
        }
        else if(readBuf[i]==' '&&*op!='#')
        {
            i++;
            break;
        }else if(readBuf[i]==' ')
        {
            continue;
        }else
        {
            *op=readBuf[i];
        }
    }
    int k=0;
    for(;i<len;++i)
    {
        if(readBuf[i]=='\r'||readBuf[i]=='\n')
        {
            continue;
        }else if(readBuf[i]==',')
        {
            i++;
            break;
        }else
        {
            addr[k++]=readBuf[i];
        }
    }
    addr[k]='\0';
    sscanf(readBuf+i,"%d",size);
    return 1;
}
int main(int argc, char *args[]) {
    flag_h = flag_v = false;
    int acParse = parseConsole(argc, args);
    if (acParse != 0) {
        printf("Parse args error!\n");
        return 0;
    }
    args_t=64-args_b-args_s;
    int cacheLineNumber = args_E;
    int cacheSetNumber = pow(2, args_s);
    Cache *cache = (Cache *) malloc(sizeof(Cache) * cacheSetNumber);
    for (int i = 0; i < cacheSetNumber; ++i) {
        //cache[i].init(cacheLineNumber);
        setCache(&cache[i], cacheLineNumber);
    }

    FILE *stream;
    stream = fopen(tracefile, "r");
    if (stream == NULL) {
        printf("NO such file!\n");
    }
    //lseek(stream,0,SEEK_SET);
    char op;
    char addr[maxAddrSize];
    int size;
    unsigned int miss_count = 0;
    unsigned int hit_count = 0;
    unsigned int eviction_count = 0;
    int time_count = 0;
    while (true) {
        if(readData(&op,addr,&size,stream)==false)
        {
            break;
        }
        time_count++;
        if (op == 'I') {
            continue;
        }
        int flag, cacheSetFlag;
        parseFlag(addr, &flag, &cacheSetFlag);
        bool ac = false;
        bool evictionNum = -1;
        bool haveZero = false;
        for (int i = 0; i < cacheLineNumber; ++i) {
            if (cache[cacheSetFlag].line[i].isAlive == true && cache[cacheSetFlag].line[i].flag == flag) {
                ac = true;
                hit_count++;
                cache[cacheSetFlag].line[i].acceseTime = time_count;
                break;
            } else if (haveZero == false) {
                if (cache[cacheSetFlag].line[i].isAlive == false) {
                    haveZero = true;
                    evictionNum = i;
                } else {
                    if (evictionNum == -1) {
                        evictionNum = i;
                    } else if (cache[cacheSetFlag].line[i].acceseTime <
                               cache[cacheSetFlag].line[evictionNum].acceseTime) {
                        evictionNum = i;
                    }
                }
            }
        }
        //evicition
        if (ac == false) {
            miss_count++;
            if (haveZero == false) {
                eviction_count++;
            }
            cache[cacheSetFlag].line[evictionNum].isAlive = true;
            cache[cacheSetFlag].line[evictionNum].flag = flag;
            cache[cacheSetFlag].line[evictionNum].acceseTime = time_count;
        }
        if (op == 'M') {
            hit_count++;
        }

        if (flag_v == true) {
            printf("%c %s", op, addr);
            if (ac) {
                printf(" hit");
            } else {
                printf(" miss");
            }
            if ((!haveZero&&!ac) == true) {
                printf(" eviction");
            }
            if (op == 'M') {
                printf(" hit");
            }
            printf("\n");
        }
    }
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
```
### Optimizing Matrix Transpose
#### 实验内容
这一部分要利用所学的缓存部分的知识，然后采用各种蛇皮操作来降低矩阵转置过程中的缓存不命中的数量，主要是分块.
#### 注意细节
* 这个实验非常毒瘤，请参考<a href="https://wdxtub.com/2016/04/16/thick-csapp-lab-4/">不周山之CacheLab的讲解,虽然这个讲解也很毒瘤</a>
* 不止在读取矩阵的时候需要操作缓存，写入到新矩阵的时候也需要操作缓存
* **注意"抖动"**带来的多余miss
* 注意对角线带来的重复请求同一个组导致"驱逐(淘汰)"
* 三种情况应该分开处理，如果谋求一个完美的没有做分开处理的程序，几乎一定拿不到满分
* 即使你做了以上这些，你还是拿不到满分($64\times 64$非常毒瘤)

#### 参考代码
```cpp
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int t1, t2, t3, t4;
    for (int i = 0; i < N / 8; ++i) {
        for (int j = 0; j < M / 8; ++j) {
            if (M == 32) {
                for (int r = 0; r < 8; ++r) {
                    for (int s = 0; s < 8; ++s) {
                        if (i == j && r == s) {
                            t1 = r;
                            t2 = A[i * 8 + r][j * 8 + s];
                        } else {
                            t3 = A[i * 8 + r][j * 8 + s];
                            B[j * 8 + s][i * 8 + r] = t3;
                        }
                    }
                    if (i == j) {
                        B[j * 8 + t1][i * 8 + t1] = t2;
                    }
                }
            } else {
                //#1
                for (int r = 0; r < 4; ++r) {
                    t1 = A[i * 8 + r][j * 8 + 0];
                    t2 = A[i * 8 + r][j * 8 + 1];
                    t3 = A[i * 8 + r][j * 8 + 2];
                    t4 = A[i * 8 + r][j * 8 + 3];

                    B[j * 8 + 0][i * 8 + r] = t1;
                    B[j * 8 + 1][i * 8 + r] = t2;
                    B[j * 8 + 2][i * 8 + r] = t3;
                    B[j * 8 + 3][i * 8 + r] = t4;

                }
                //#2
                for (int r = 0; r < 4; ++r) {
                    t1 = A[i * 8 + r][j * 8 + 4];
                    t2 = A[i * 8 + r][j * 8 + 5];
                    t3 = A[i * 8 + r][j * 8 + 6];
                    t4 = A[i * 8 + r][j * 8 + 7];


                    B[j * 8 + 4][i * 8 + r] = t1;
                    B[j * 8 + 5][i * 8 + r] = t2;
                    B[j * 8 + 6][i * 8 + r] = t3;
                    B[j * 8 + 7][i * 8 + r] = t4;
                }

                //#3
                for (int r = 4; r < 8; ++r) {
                    t1 = A[i * 8 + r][j * 8 + 0];
                    t2 = A[i * 8 + r][j * 8 + 1];
                    t3 = A[i * 8 + r][j * 8 + 2];
                    t4 = A[i * 8 + r][j * 8 + 3];


                    B[j * 8 + 0][i * 8 + r] = t1;
                    B[j * 8 + 1][i * 8 + r] = t2;
                    B[j * 8 + 2][i * 8 + r] = t3;
                    B[j * 8 + 3][i * 8 + r] = t4;

                }
                //#4
                for (int r = 4; r < 8; ++r) {
                    t1 = A[i * 8 + r][j * 8 + 4];
                    t2 = A[i * 8 + r][j * 8 + 5];
                    t3 = A[i * 8 + r][j * 8 + 6];
                    t4 = A[i * 8 + r][j * 8 + 7];

                    B[j * 8 + 4][i * 8 + r] = t1;
                    B[j * 8 + 5][i * 8 + r] = t2;
                    B[j * 8 + 6][i * 8 + r] = t3;
                    B[j * 8 + 7][i * 8 + r] = t4;
                }
            }
        }
    }
    for (int i = 0; i < N / 8 * 8; ++i) {
        for (int j = 0; j < M % 8; ++j) {
            B[M / 8 * 8 + j][i] = A[i][M / 8 * 8 + j];
        }
    }
    for (int j = 0; j < M; ++j) {
        for (int i = N / 8 * 8; i < N; ++i) {
            B[j][i] = A[i][j];
        }
    }
}
```
注意，这份代码得分为
```common
Trans perf 32x32           8.0         8         287
Trans perf 64x64           3.4         8        1699
Trans perf 61x67           9.3        10        2067
```
如果把分块从$8\times 8$在$61\times 67$的时候改为$16 \times 16$,那么$61\times 67$也可以得到满分，但是$64 \times 64$我还没发现拿满分的办法。
