// struct block: a cache block simulation
// function readargv: read the arguments passed by the command line
// function cache_initialize: malloc and initialize the cache
// function cache_free: free the cache
// function getset: get the set of the cache block to be placed
// function gettag: get the tag of the cache block
// function solve: update the cache


#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

int s=0,E=0,b=0;
int hit=0,miss=0,eviction=0;
char filepath[200]={};
int printmessage=0;// print detailed hits, miss and evictions

char readargverror[]=
    "An error occured when analysising arguments. Program terminated.";
char invalidargv[]=
    "Invalid arguments input. Program terminated.";
char mallocerror[]=
    "An error occured when mallocing cache memory. Program terminated.";
char openfileerror[]=
    "Can not find the trace file. Program terminated.";
char* help_info[11]=
    {
        "Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>",
        "Options:",
        "-h: Optional help flag that prints usage info",
        "-v: Optional verbose flag that displays trace info",
        "-s <s>: Number of set index bits (S = 2^s is the number of sets)",
        "-E <E>: Associativity (number of lines per set)",
        "-b <b>: Number of block bits (B = 2^b is the block size)",
        "-t <tracefile>: Name of the valgrind trace to replay",
        "Examples:",
        "linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace",
        "linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace"
    };

// struct block
// NOTE: In the array, the index indicates the recently used order.
// When evict, the block with index 0 should be evicted.
typedef struct
{
    int valid;
    unsigned long tag;
}block;
block** block_ptr=NULL;
int* block_used=NULL;// total num of the line in one set used


// read the arguments passed by the command line
// input: argc, argv[]
// return value: 0-OK, 1-command error, 2-invalid cache information
int readargv(int argc,char* argv[])
{
    int tmp;
    while ((tmp=getopt(argc,argv,"s:E:b:t:"))!=-1)
    {
        switch (tmp)
        {
            case 's':
                s=atoi(optarg);
                if (s<0)
                    return 2;
                break;
            case 'E':
                E=atoi(optarg);
                if (E<=0)
                    return 2;
                break;
            case 'b':
                b=atoi(optarg);
                if (b<0)
                    return 2;
                break;
            case 't':
                strcpy(filepath,optarg);
                break;
            default:
                // printf("error opterr: %d\n", opterr);
                return 1;// error
        }
    }
    return 0;
}

// malloc and initialize the cache
// input: none
// return value: 0 if succeed, 1 if fail
int cache_initialize()
{
    block_used=(int*)malloc(sizeof(int)*(1<<s));
    if (block_used==NULL)
        return 1;
    
    block_ptr=(block**)malloc(sizeof(block*)*(1<<s));
    if (block_ptr==NULL)
        return 1;
    for (int i=0;i<(1<<s);i++)
    {
        block_ptr[i]=(block*)malloc(sizeof(block)*E);
        if (block_ptr[i]==NULL)
            return 1;
    }
    
    //initialize
    for (int i=0;i<(1<<s);i++)
    {
        for (int j=0;j<E;j++)
        {
            block_ptr[i][j].valid=0;
            block_ptr[i][j].tag=0;
        }
    }
    return 0;
}

// free the cache
// input: none
// return value: none
void cache_free()
{
    for (int i=0;i<(1<<s);i++)
        free(block_ptr[i]);
    free(block_ptr);
    free(block_used);
}


// get the set of the cache block to be placed
// input: unsigned long, as an address
// return value: the set number
unsigned long getset(unsigned long addr)
{
    return (addr>>b)&((1<<s)-1);
}

// get the tag of the cache block
// input: unsigned long, as an address
// return value: the tag number
unsigned long gettag(unsigned long addr)
{
    return (addr>>(b+s));
}

// update the cache
// input: char, indicates cache assessment type ;unsigned long, as an address
// return value: none
void solve(char type,unsigned long addr)
{
    unsigned setnum=getset(addr);
    unsigned nowtag=gettag(addr);
    
    //search
    for (int i=0;i<block_used[setnum];i++)
    {
        if (block_ptr[setnum][i].valid==1&&block_ptr[setnum][i].tag==nowtag)
        {
            hit++;
            if (printmessage==1)
                printf("%s","hit");

            // move the block to the last
            for (int j=i+1;j<block_used[setnum];j++)
            {
                block_ptr[setnum][j-1].valid=block_ptr[setnum][j].valid;
                block_ptr[setnum][j-1].tag=block_ptr[setnum][j].tag;
            }
            block_ptr[setnum][block_used[setnum]-1].valid=1;
            block_ptr[setnum][block_used[setnum]-1].tag=nowtag;

            if (type=='M')
            {
                hit++;
                if (printmessage==1)
                    printf("%s"," hit\n");
            }
            else if (printmessage==1)
                printf("%s","\n");
            return;
        }
    }
    
    //miss
    miss++;
    if (printmessage==1)
        printf("%s","miss");
    if (block_used[setnum]<E)// does not need to evict
    {
        block_ptr[setnum][block_used[setnum]].valid=1;
        block_ptr[setnum][block_used[setnum]].tag=nowtag;
        block_used[setnum]++;
    }
    else// evict the first block
    {
        eviction++;
        if (printmessage==1)
            printf("%s"," eviction");
        for (int i=1;i<E;i++)
        {
            block_ptr[setnum][i-1].valid=block_ptr[setnum][i].valid;
            block_ptr[setnum][i-1].tag=block_ptr[setnum][i].tag;
        }
        block_ptr[setnum][E-1].valid=1;
        block_ptr[setnum][E-1].tag=nowtag;
    }
    if (type=='M')
    {
        hit++;
        if (printmessage==1)
            printf("%s"," hit\n");
    }
    else if (printmessage==1)
        printf("%s","\n");
    return;
}


// main function
int main(int argc,char* argv[])
{
    // does argv[1]=='-h'?
    if (strcmp(argv[1],"-h")==0)
    {
        for (int i=0;i<11;i++)
            puts(help_info[i]);
        return 0;
    }

    // does argv[1]=='-v'?
    if (strcmp(argv[1],"-v")==0)
        printmessage=1;

    int analyargv=0;
    if (printmessage==1)
    {
        for (int i=1;i<argc;i++)
            argv[i]=argv[i+1];
        analyargv=readargv(argc-1,argv);
    }
    else
        analyargv=readargv(argc,argv);
    
    if (analyargv==1)// analysising argument failed
    {
        puts(readargverror);
        return 1;
    }
    else if (analyargv==2)
    {
        puts(invalidargv);
        return 2;
    }
    
    if (cache_initialize()==1)// malloc failed
    {
        puts(mallocerror);
        return 3;
    }
    
    FILE *fp=fopen(filepath,"r");// open the file, read-only
    if (fp==NULL)// open failed
    {
        puts(openfileerror);
        return 4;
    }
    
    char line[100]={};// read one line
    char type;// read type
    unsigned long addr=0;// 64-bit address
    unsigned long len=0;// the length of bytes to be get
    
    while (fgets(line,100,fp)!=NULL)
    {
        if (line[0]=='I')// instruction cache access
            continue;
        sscanf(line," %c%lx,%lx",&type,&addr,&len);// get the address
        if (printmessage==1)
            printf("%c %lx,%lx ",type,addr,len);
        solve(type,addr);
        //printf("%c %lx\n",type,addr);
    }
    printSummary(hit,miss,eviction);
    
    fclose(fp);
    cache_free();
    //printf("%d %d %d %s\n",s,E,b,filepath);
    return 0;
}