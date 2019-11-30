#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for getopt()
#include <math.h>

#define BYTES_PER_WORD 4
// #define DEBUG

/*
 * Cache structures
 */
int time = 0;

typedef struct {
	int age;
	int valid;
	int modified;
	uint32_t tag;
} cline;

typedef struct {
	cline *lines;
} cset;

typedef struct {
	int s;
	int E;
	int b;
	cset *sets;
} cache;

static int index_bit(int n) {
	int cnt = 0;
	while(n){
		cnt++;
		n = n >> 1;
	}
	return cnt-1;
}

char* NumToBits (unsigned int num, int len) {
    char* bits = (char *) malloc(len+1);
    int idx = len-1, i;

    while (num > 0 && idx >= 0) {
	if (num % 2 == 1) {
	    bits[idx--] = '1';
	} else {
	    bits[idx--] = '0';
	}
	num /= 2;
    }

    for (i = idx; i >= 0; i--){
	bits[i] = '0';
    }
    
    return bits;    
}

void bintonum (char* ptr, char plag[])
{
    int decimal = 0, position = 0;
    int len = strlen(ptr);
    for (int i = len - 1; i >= 0; i--)
    {
	char ch = ptr[i];
	decimal += (ch - '0') * pow(2, position);
	position++;
    }
    sprintf(plag, "%d", decimal);
}

void build_cache(cache* cash) {
    int set = 1 << cash->s;
    int way = cash->E;
    cset* son = (cset*)malloc(sizeof(cset) * set);               
    cash->sets = son;
    for (int i = 0; i < set; i++)
    {
	cline* kane = (cline*)malloc(sizeof(cline) * way);
	for (int j = 0; j < way; j++)
	{
	    kane[j].age = 0; kane[j].valid = 0; kane[j].modified = 0; kane[j].tag = 0; 
	}
	cash->sets[i].lines = kane;
    }
}
// read-hits = 0, write-hits 1, read-misses = 2, write-misses = 3
int access_cache(char* op, char* tag_r, char* index_r, cache* cash, int* writeback) {    
    int tag_s = atoi(tag_r), index_s = atoi(index_r);
    int way = cash->E, pass, cnt = 0;
    for (int i = 0; i < way; i++)
    {
	if (cash->sets[index_s].lines[i].valid == 1 &&
	    tag_s == cash->sets[index_s].lines[i].tag)
	{
	    if (*op == 'R')
		pass = 0;
	    else if (*op == 'W')
	    {
		cash->sets[index_s].lines[i].modified = 1;
		pass = 1;
	    }
	}
	else
	{
	    cnt++;
	}
    }
    if (cnt == way)
    {
	int max = -1, oldest;		
	for (int i = 0; i < way; i++)
	{
	    if (cash->sets[index_s].lines[i].age > max)
	    {
		max = cash->sets[index_s].lines[i].age;
		oldest = i;
	    }	
	}
	for (int i = 0; i < way; i++)
	{
	    cash->sets[index_s].lines[i].age++;
	}
	cash->sets[index_s].lines[oldest].age = 0; cash->sets[index_s].lines[oldest].valid = 1;
	cash->sets[index_s].lines[oldest].modified = 0; cash->sets[index_s].lines[oldest].tag = tag_s;
	if (*op == 'R')
	    pass = 2;
	else if (*op == 'W')
	    pass = 3;
    }
    return pass;    
}

/***************************************************************/
/*                                                             */
/* Procedure : cdump                                           */
/*                                                             */
/* Purpose   : Dump cache configuration                        */
/*                                                             */
/***************************************************************/
void cdump(int capacity, int assoc, int blocksize){

	printf("Cache Configuration:\n");
    	printf("-------------------------------------\n");
	printf("Capacity: %dB\n", capacity);
	printf("Associativity: %dway\n", assoc);
	printf("Block Size: %dB\n", blocksize);
	printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : sdump                                           */
/*                                                             */
/* Purpose   : Dump cache stat		                           */
/*                                                             */
/***************************************************************/
void sdump(int total_reads, int total_writes, int write_backs,
	int reads_hits, int write_hits, int reads_misses, int write_misses) {
	printf("Cache Stat:\n");
    	printf("-------------------------------------\n");
	printf("Total reads: %d\n", total_reads);
	printf("Total writes: %d\n", total_writes);
	printf("Write-backs: %d\n", write_backs);
	printf("Read hits: %d\n", reads_hits);
	printf("Write hits: %d\n", write_hits);
	printf("Read misses: %d\n", reads_misses);
	printf("Write misses: %d\n", write_misses);
	printf("\n");
}


/***************************************************************/
/*                                                             */
/* Procedure : xdump                                           */
/*                                                             */
/* Purpose   : Dump current cache state                        */
/* 					                       */
/* Cache Design						       */
/*  							       */
/* 	    cache[set][assoc][word per block]		       */
/*                                			       */
/*      				                       */
/*       ----------------------------------------	       */
/*       I        I  way0  I  way1  I  way2  I                 */
/*       ----------------------------------------              */
/*       I        I  word0 I  word0 I  word0 I                 */
/*       I  set0  I  word1 I  word1 I  work1 I                 */
/*       I        I  word2 I  word2 I  word2 I                 */
/*       I        I  word3 I  word3 I  word3 I                 */
/*       ----------------------------------------              */
/*       I        I  word0 I  word0 I  word0 I                 */
/*       I  set1  I  word1 I  word1 I  work1 I                 */
/*       I        I  word2 I  word2 I  word2 I                 */
/*       I        I  word3 I  word3 I  word3 I                 */
/*       ----------------------------------------              */
/*                              			       */
/*                                                             */
/***************************************************************/
void xdump(cache* L)
{
	int i,j,k = 0;
	int b = L->b, s = L->s;
	int way = L->E, set = 1 << s;
	int E = index_bit(way);

	uint32_t line;

	printf("Cache Content:\n");
    	printf("-------------------------------------\n");
	for(i = 0; i < way;i++)
	{
		if(i == 0)
		{
			printf("    ");
		}
		printf("      WAY[%d]",i);
	}
	printf("\n");

	for(i = 0 ; i < set;i++)
	{
		printf("SET[%d]:   ",i);
		for(j = 0; j < way;j++)
		{
			if(k != 0 && j == 0)
			{
				printf("          ");
			}
			if(L->sets[i].lines[j].valid){
				line = L->sets[i].lines[j].tag << (s+b);
				line = line|(i << b);
			}
			else{
				line = 0;
			}
			printf("0x%08x  ", line);
		}
		printf("\n");
	}
	printf("\n");
}


int main(int argc, char *argv[]) {
	int i, j, k;
	int capacity=1024;
	int way=8;
	int blocksize=8;
	int set;

	//cache
	cache simCache;

	// counts
	int read=0, write=0, writeback=0;
	int readhit=0, writehit=0;
	int readmiss=0, writemiss = 0;

	// Input option
	int opt = 0;
	char* token;
	int xflag = 0;

	// parse file
	char *trace_name = (char*)malloc(32);
	FILE *fp;
        char line[16];
        char *op;
        uint32_t addr;

        /* You can define any variables that you want */
	trace_name = argv[argc-1];
	if (argc < 3) {
		printf("Usage: %s -c cap:assoc:block_size [-x] input_trace \n",argv[0]);
		exit(1);
	}
	while((opt = getopt(argc, argv, "c:x")) != -1){
		switch(opt){
			case 'c':
                // extern char *optarg;
				token = strtok(optarg, ":");
				capacity = atoi(token);
				token = strtok(NULL, ":");
				way = atoi(token);
				token = strtok(NULL, ":");
				blocksize  = atoi(token);
				break;
			case 'x':
				xflag = 1;
				break;
			default:
			printf("Usage: %s -c cap:assoc:block_size [-x] input_trace \n",argv[0]);
			exit(1);
		}
	}

	// allocate
	set = capacity/way/blocksize;

    /* TODO: Define a cache based on the struct declaration */
    simCache.E = way;
    simCache.s = index_bit(set);
    simCache.b = index_bit(blocksize);
    cache* targetcache = &simCache;
    build_cache(targetcache);

	// simulate
	fp = fopen(trace_name, "r"); // read trace file
	if(fp == NULL){
		printf("\nInvalid trace file: %s\n", trace_name);
		return 1;
	}
	cdump(capacity, way, blocksize);

    /* TODO: Build an access function to load and store data from the file */
    while (fgets(line, sizeof(line), fp) != NULL) {
        op = strtok(line, " ");
        addr = strtoull(strtok(NULL, ","), NULL, 16);	
    
#ifdef DEBUG
        // You can use #define DEBUG above for seeing traces of the file.
        fprintf(stderr, "op: %s\n", op);
        fprintf(stderr, "addr: %x\n", addr);
#endif
        // ...
        // access_cache()
        // ...
	if (*op == 'R')
	{
	    read++;
	}
	else
	{
	    write++;
	}
	char* ptr = NumToBits(addr, 32);
	char index[32], tag[32], index_r[32], tag_r[32];
	int k = 0, u = 0, flag;
	int* writelater = &writeback;
	for(int i = 32 - (targetcache->b + targetcache->s); i < 32 - targetcache->b; i++, k++)
	{
	    index[k] = ptr[i];
	}
	for (int i = 0; i < 32 - (targetcache->b + targetcache->s); i++, u++)
	{
	    tag[u] = ptr[i];
	}
	index[k] = '\0'; tag[u] = '\0';
	bintonum(tag, tag_r); bintonum(index, index_r);
	flag = access_cache(op, tag_r, index_r, targetcache, writelater);
	if (flag == 0)
	    readhit++;
	else if (flag == 1)
	    writehit++;
	else if (flag == 2)
	    readmiss++;
	else if (flag == 3)
	    writemiss++;	    
    }
    
    // test example
	sdump(read, write, writeback, readhit, writehit, readmiss, writemiss);
	if (xflag){
	    	xdump(&simCache);
	}

    	return 0;
}
