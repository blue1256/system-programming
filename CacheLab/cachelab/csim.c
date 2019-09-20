#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <string.h>

char* help = 
"Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n\
Options:\n\
  -h         Print this help message.\n\
  -v         Optional verbose flag.\n\
  -s <num>   Number of set index bits.\n\
  -E <num>   Number of lines per set.\n\
  -b <num>   Number of block offset bits.\n\
  -t <file>  Trace file.\n\
\n\
Examples:\n\
  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n\
  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace";

char* missArg = "Missing required command line argument";

char* noFile = "No such file or directory";

typedef struct block {
    long long tag;
	int time;
    char v;
} block;

typedef struct set {
    block* blocks;
} set;

int main(int argc, char** argv)
{
	int hit_count=0, miss_count=0, eviction_count=0;
    int hflag=0, vflag=0;
    int opt=0;
	int s=0,E=0,b=0;
	char* t=NULL;
    while((opt=getopt(argc,argv,"hvs:E:b:t:"))!=-1){
        switch(opt){
            case 'h':
                hflag = 1;
                break;
            case 'v':
                vflag = 1;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
				t = (char*) malloc(sizeof(char)*(strlen(optarg)+1));
                strcpy(t,optarg);
                break;
            case '?':
                hflag = 1;
                break;
        }
    }
	if((s==0||E==0||b==0||t==NULL)&&!hflag){
		hflag = 1;
		printf("%s: %s\n",argv[0],missArg);
	}
	if(hflag){
		printf("%s\n",help);
		return 0;
	}
	int cachesize= (int)(pow(2,s));
    set cache[cachesize];
    for(int i=0;i<pow(2,s);i++){
        cache[i].blocks = malloc(sizeof(block)*E);
        for(int j=0;j<E;j++){
            cache[i].blocks[j].v = 0;
        }
    }
    FILE* fp = fopen(t,"r");
	char op;
    char dummy;
    long long addr;
    int size;
    if(fp==NULL){
        printf("%s: %s\n",t,noFile);
        return 0;
    }
    int time = 0;
    while(1){
        int hit=0,miss=0,eviction=0;
		fscanf(fp,"%c",&dummy);
		fscanf(fp,"%c",&op);
        fscanf(fp,"%llx",&addr);
        fscanf(fp,"%c",&dummy);
        fscanf(fp,"%d",&size);
		fscanf(fp,"%c",&dummy);
		while(dummy!='\n'){
			fscanf(fp,"%c",&dummy);
		}
		if(feof(fp)){
			break;
		}
		if(op==' '){
            continue;
        }
        int bOffset = 0;
        int setInd = 0;
        long long tag = 0;
        int bkInd = -1;
        int invalInd = -1;
        int leastInd = 0;
        int mask = 1;
        for(int i=0;i<b;i++){
            bOffset += mask&addr;
            mask = mask<<1;
        }
        for(int i=0;i<s;i++){
            setInd += mask&addr;
            mask = mask<<1;
        }
        setInd = setInd>>b;
        tag = addr>>(b+s);
        for(int i=0;i<E;i++){
            if(cache[setInd].blocks[i].tag==tag&&cache[setInd].blocks[i].v==1){
                bkInd = i;
                break;
            }
        }
        if(bkInd>=0){
			cache[setInd].blocks[bkInd].time = time;
            hit_count++;
            hit = 1;
        }else{
            miss_count++;
            miss = 1;
            for(int i=0;i<E;i++){
                if(cache[setInd].blocks[i].v==0){
					invalInd = i;
					break;
                }
            }
            if(invalInd>=0){
                cache[setInd].blocks[invalInd].v=1;
                cache[setInd].blocks[invalInd].time = time;
                cache[setInd].blocks[invalInd].tag = tag;
            }else{
                eviction_count++;
                eviction = 1;
                int minTime = cache[setInd].blocks[0].time;
                for(int i=0;i<E;i++){
                    if(cache[setInd].blocks[i].time<minTime){
                        leastInd = i;
                        minTime = cache[setInd].blocks[i].time;
                    }
                }
                cache[setInd].blocks[leastInd].v=1;
                cache[setInd].blocks[leastInd].time = time;
                cache[setInd].blocks[leastInd].tag = tag;
            }
        }
        if(op=='M'){
            hit_count++;
            hit++;
        }
        if(vflag){
            printf("%c %llx,%d ",op,addr,size);
            if(miss){
                printf("miss ");
            }
            if(eviction){
                printf("eviction ");
            }
			if(hit){
				printf("hit ");
			}
            printf("\n");
        }
		time++;
    }
    printSummary(hit_count,miss_count,eviction_count);
    return 0;
}
