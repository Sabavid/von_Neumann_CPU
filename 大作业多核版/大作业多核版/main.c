//
//  main.c
//  大作业多核版
//
//  Created by Ice Bear on 2020/6/13.
//  Copyright © 2020 Mushroom. All rights reserved.
//

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h>

#define MAX_MEM 16384
#define MEM_SIZE 32768     //主存总字节数
#define CMD_LEN 32         //指令字符长度
#define REG_LEN 2          //寄存器字节长度
#define CODE_GROUP 4       //代码输出的字节组长度
#define DATA_GROUP 2       //数据输出的字节组长度
#define OUT_LINES 16       //输出行数
#define CODES_LINE 8       //代码每行输出个数
#define DATAS_LINE 16      //数据每行输出个数
#define TICKETS_n 100      //初始票数
#define THREAD_n 2         //线程数
#define THREAD_start1 0    //线程1代码段开始
#define THREAD_start2 256  //线程2代码段开始

HANDLE hMutex[MAX_MEM/2];
HANDLE hMut;

struct MEM_thread{    //线程函数的参数
    short *MEM;
    short Th_ID;
};

typedef struct MEM_thread Threads;
typedef struct MEM_thread *Thread;

typedef struct registers {
    short dataReg[4];   //数据寄存器
    short addrReg[4];   //地址寄存器
    short ir[2];        //指令寄存器
    short flag;         //标志寄存器
    short ip;           //程序计数器
    short numReg;       //立即数寄存器
} REGS;

unsigned __stdcall Th_Function(void* pArguments);        //线程函数
void readCmdFile(FILE *fp, short memory[], short id);                        //将指令文件写入主存
void getCmd(short memory[], REGS *REGPtr);                         //从主存中取指令，并存入各寄存器
void operate(REGS *REGPtr, short memory[], int*stop, short id);              //执行指令
void ipNew(REGS*REGPtr);                                           //进行完一轮操作后程序计数器指向下一个指令
void init(REGS *REGPtr, short id);                           //初始化主存全为0
void end(short memory[]);                                          //结束程序时输出内存状态
void outRegState(REGS *REGPtr, short id);                                    //输出各寄存器状态
void shorttoshort2(short s, short buffer[]);                       //short拆成2个字节存到短整数里
short char8toshort(char buffer[]);                                 //8位二进制字符串转为十进制short
short short2toshort(short buffer[]);                               //双short对合并为short
int short4toint(short buffer[]);                                   //4个short合并为1个int

int main()
{
    int i;
    short memory[MEM_SIZE] = {0};
    memory[MAX_MEM+1] = TICKETS_n;
    HANDLE hThread[THREAD_n];
    Threads threads[THREAD_n];
    for(i = 0; i<MAX_MEM/2; i++){
        hMutex[i] = CreateMutex(NULL,FALSE,NULL);
    }
    hMut = CreateMutex(NULL,FALSE,NULL);
    threads[0].MEM = memory;
    threads[1].MEM = memory;
    threads[0].Th_ID=1;
    threads[1].Th_ID=2;
    for(i = 0; i<THREAD_n; i++){
        hThread[i] = (HANDLE)_beginthreadex(NULL,0,Th_Function,&threads[i],0,NULL);  //创建线程
    }
    WaitForMultipleObjects(THREAD_n,hThread,1,INFINITE);      //等待线程结束
    for(i = 0; i < THREAD_n; i++){
        CloseHandle(hThread[i]);      //关闭句柄
    }
    printf("\n");
    end(memory);  //输出主存
    return 0;
}

unsigned __stdcall Th_Function(void *pArguments){
    int stop = 0;
    short *memory;
    short id;
    REGS reg;
    REGS *REGPtr = &reg;
    Thread THREADPtr;
    THREADPtr = (Thread)pArguments;
    FILE *fp;
    memory = THREADPtr->MEM;
    id = THREADPtr->Th_ID;
    if((fp = fopen("dict.dic","r"))==NULL){
        printf("error...\n");
        exit(0);
    }
    else
    {
        init(REGPtr,id);//初始化寄存器
        readCmdFile(fp,memory,id);
    }
    while(!stop)
    {
        getCmd(memory, REGPtr);
        operate(REGPtr, memory, &stop, id);
        ipNew(REGPtr);
        outRegState(REGPtr, id);
    }
    printf("\n");
    _endthreadex(0);
    return 0;
}

void readCmdFile(FILE *fp, short memory[], short id)
{
    char buffer[CMD_LEN+3];
    int i,j = 0;
    if(id==1){
        j = THREAD_start1;
    }
    if(id==2){
        j = THREAD_start2;
    }
    while (fgets(buffer, CMD_LEN+2, fp)!=NULL) {
        for(i = j; i<j+4; i++){
            memory[i] = char8toshort(&buffer[8*(i-j)]);
        }
        j = i;
    }
}

void outRegState(REGS *REGPtr, short id)
{
    WaitForSingleObject(hMut,INFINITE);
    printf("\nid = %d\n",id);
    printf("ip = %hd\n",REGPtr->ip);
    printf("flag = %hd\n",REGPtr->flag);
    printf("ir = %hd\n",short2toshort(REGPtr->ir));
    printf("ax1 = %hd ax2 = %hd ax3 = %hd ax4 = %hd\n",
           REGPtr->dataReg[0],REGPtr->dataReg[1],REGPtr->dataReg[2],REGPtr->dataReg[3]);
    printf("ax5 = %hd ax6 = %hd ax7 = %hd ax8 = %hd\n",
           REGPtr->addrReg[0],REGPtr->addrReg[1],REGPtr->addrReg[2],REGPtr->addrReg[3]);
    ReleaseMutex(HMUT);
}

int short4toint(short buffer[])
{
    int n = 0;
    int i;
    for (i = 0; i < 4; i++) {
        n<<=8;
        n |= (buffer[i] & 0xff);
    }
    return n;
}

short char8toshort(char buffer[])
{
    short n = 0;
    int i = 0;
    for(i = 0; i < 8; i++){
        n = 2*(n+(*buffer)-'0');
        buffer++;
    }
    return n/2;
}

short short2toshort(short buffer[])
{
    short n = 0;
    int i = 0;
    for (i = 0; i < 2; i++)
    {
        n <<= 8;
        n |= (buffer[i] & 0xff);
    }
    return n;
}

void shorttoshort2(short s, short buffer[])
{
    int i,offset;
    for(i = 0; i < 2; i++){
        offset = 16-(i+1)*8;
        buffer[i]=(s>>offset)&0xff;
    }
}

void init(REGS *REGPtr, short id)
{
    int i = 0;
    if(id==1){
        REGPtr->ip = THREAD_start1;
    }
    if(id==2){
        REGPtr->ip = THREAD_start2;
    }
    REGPtr->flag = 0;
    REGPtr->numReg = 0;
    for (i = 0; i<4; i++) {
        REGPtr->dataReg[i] = 0;
        REGPtr->addrReg[i] = 0;
    }
    for (i = 0; i<2; i++) {
        REGPtr->ir[i] = 0;
    }
}

void getCmd(short memory[MEM_SIZE], REGS *REGPtr)
{
    REGPtr->ir[0] = memory[0+REGPtr->ip];                      //指令类型编号
    REGPtr->ir[1] = memory[1+REGPtr->ip];                      //目标寄存器号 源寄存器号
    REGPtr->numReg = short2toshort(memory+2+REGPtr->ip);       //立即数-合并了两个字节
}

void ipNew(REGS *REGPtr)
{
    REGPtr->ip += 4;
}

void end(short memory[])
{
    int i = 0,j = 0;
    printf("codeSegment :\n");
    for (i = 0; i<OUT_LINES; i++) {                 //代码段Code
        for (j = 0; j<CODES_LINE; j++) {
            if(j==CODES_LINE-1)
                printf("%d",short4toint(memory+j*CODE_GROUP+i*CODE_GROUP*CODES_LINE));
            else
                printf("%d ",short4toint(memory+j*CODE_GROUP+i*CODE_GROUP*CODES_LINE));
        }
        printf("\n");
    }
    printf("\n");
    printf("dataSegment :\n");
    for (i = 0; i<OUT_LINES; i++) {                 //数据段
        for (j = 0; j<DATAS_LINE; j++) {
            if(j==DATAS_LINE-1)
                printf("%hd",short2toshort(memory+j*DATA_GROUP+i*DATAS_LINE*DATA_GROUP+MEM_SIZE/2));
            else
                printf("%hd ",short2toshort(memory+j*DATA_GROUP+i*DATAS_LINE*DATA_GROUP+MEM_SIZE/2));
        }
        printf("\n");
    }
}

void operate(REGS *REGPtr, short memory[], int*stop, short id)
{
    int lock_n,sleepTime;
    short n = REGPtr->numReg;
    short target,source;            //目标寄存器和源寄存器编号
    target = REGPtr->ir[1]/16;      //目标寄存器编号取前4位
    source = REGPtr->ir[1]%16;      //源寄存器编号取后4位
    switch (REGPtr->ir[0]) {
/*停机*/case 0:
            *stop = 1;
            return;    //直接跳出函数
/*数据传送*/case 1:
            if(!source) //源为0
            {
                if(target>=5)
                    REGPtr->addrReg[target-5] = n;
                else
                    REGPtr->dataReg[target-1] = n;
            }
            else
            {
                if(source>4) //源为地址
                    REGPtr->dataReg[target-1] = short2toshort(&memory[REGPtr->addrReg[source-5]]);
                else         //源为数据
                {
                    shorttoshort2(REGPtr->dataReg[source-1], memory+REGPtr->addrReg[target-5]);
                }
            }
            break;
/*加法操作*/case 2:
            if(!source) //源为0
                REGPtr->dataReg[target-1] += n;
            else
                REGPtr->dataReg[target-1] += short2toshort(&(memory[REGPtr->addrReg[source-5]]));
            break;
/*减法操作*/case 3:
            if(!source) //源为0
                REGPtr->dataReg[target-1] -= n;
            else
                REGPtr->dataReg[target-1] -= short2toshort(&(memory[REGPtr->addrReg[source-5]]));
            break;
/*乘法操作*/case 4:
            if(!source) //源为0
                REGPtr->dataReg[target-1] *= n;
            else
                REGPtr->dataReg[target-1] *= short2toshort(&(memory[REGPtr->addrReg[source-5]]));
            break;
/*除法操作*/case 5:
            if(!source) //源为0
            {
                REGPtr->dataReg[target-1] /= n;
            }
            else
                REGPtr->dataReg[target-1] /= short2toshort(&(memory[REGPtr->addrReg[source-5]]));
            break;
/*与操作*/case 6:
            if(!source)
                REGPtr->dataReg[target-1] = REGPtr->dataReg[target-1]&&n;
            else
                REGPtr->dataReg[target-1] = REGPtr->dataReg[target-1]&&short2toshort(&(memory[REGPtr->addrReg[source-5]]));
            break;
/*或操作*/case 7:
            if(!source)
                REGPtr->dataReg[target-1] = REGPtr->dataReg[target-1]||n;
            else
                REGPtr->dataReg[target-1] = REGPtr->dataReg[target-1]||short2toshort(&(memory[REGPtr->addrReg[source-5]]));
            break;
/*非操作*/case 8:
            if(!source) //源为0，目标非0-->目标存目标
                REGPtr->dataReg[target-1] = !REGPtr->dataReg[target-1];
            else        //目标0，源非0-->源存源
            {
                memory[REGPtr->addrReg[source-5]] = 0;
                if((!short2toshort(&memory[REGPtr->addrReg[source-5]]))==0)
                    memory[REGPtr->addrReg[source-5]+1] = 0;
                else
                    memory[REGPtr->addrReg[source-5]+1] = 1;
            }
            break;
/*比较操作*/case 9:
            if(!source) //源为0
            {
                if(REGPtr->dataReg[target-1]-n==0)
                    REGPtr->flag = 0;
                else if(REGPtr->dataReg[target-1]-n>0)
                    REGPtr->flag = 1;
                else
                    REGPtr->flag = -1;
            }
            else
            {
                if(REGPtr->dataReg[target-1]-short2toshort(&(memory[REGPtr->addrReg[source-5]]))==0)
                    REGPtr->flag = 0;
                else if(REGPtr->dataReg[target-1]-short2toshort(&(memory[REGPtr->addrReg[source-5]]))>0)
                    REGPtr->flag = 1;
                else
                    REGPtr->flag = -1;
            }
            break;
/*跳转操作*/case 10:
            switch (source) {
                case 0:
                    REGPtr->ip += n-4;
                    break;
                case 1:
                    if(REGPtr->flag==0)
                        REGPtr->ip += n-4;
                    break;
                case 2:
                    if(REGPtr->flag==1)
                        REGPtr->ip += n-4;
                    break;
                case 3:
                    if(REGPtr->flag==-1)
                        REGPtr->ip += n-4;
                    break;
                default:
                    break;
            }
            break;
/*输入*/case 11:
            printf("in:\n");
            scanf("%hd",&(REGPtr->dataReg[target-1]));
            break;
/*输出*/case 12:
            printf("id = %d ",id);
            printf("out: %hd\n",REGPtr->dataReg[target-1]);
            break;
/*锁住*/case 13:
            lock_n = (int)REGPtr->numReg;
            WaitForSingleObject(hMutex[(lock_n-MAX_MEM)/2],INFINITE);
            break;
/*释放*/case 14:
            lock_n = (int)REGPtr->numReg;
            ReleaseMutex(hMutex[(lock_n-MAX_MEM)/2]);
            break;
/*休眠*/case 15:
            sleepTime = (int)REGPtr->numReg;
            Sleep(sleepTime);
            break;
        default:
            break;
    }
}

