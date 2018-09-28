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
