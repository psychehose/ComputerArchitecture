#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for getopt()

#define BYTES_PER_WORD 4
// #define DEBUG

/*
 * Cache structures
 */
int time = 0;
int number = 0; // For Debug

typedef struct
{
    int age; //LRU를 구현
    int valid;
    int modified; //dirty bit?
    uint32_t tag; //cash line의 태그 정보
} cline;

typedef struct
{
    cline *lines;
} cset; //이러한 라인들이 모여서 set이 되는 거고,

typedef struct
{
    int s; // index bits
    int E; // way
    int b; // block offset bits
    cset *sets;
} cache; //set들이 모여서, cache가 되는 거고.

int index_bit(int n){
    int cnt = 0;

    while(n) {
        cnt++;
        n = n >> 1;
    }

    return cnt-1; //2^n에서,  n을 리턴함
}


char * slice(char *string, int start, int end) {

    int stringSize = end - start + 1;
    char *target = malloc(sizeof(char) * stringSize) ;
    strncpy(target,string + start, stringSize);

    return  target;
}

char* num_to_bits(unsigned int num, int len)
{
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
    bits[len] = '\0';
    return bits;
}

int fromBinary(char *s)
{
    return (int) strtol(s, NULL, 2);
}

/***************************************************************/
/*                                                             */
/* Procedure : build_cache                                     */
/*                                                             */
/* Purpose   : Initialize cache structure                      */
/*                                                             */
/* Parameters:                                                 */
/*     int S: The set of cache                                 */
/*     int E: The associativity way of cache                   */
/*     int b: The blocksize of cache                           */
/*                                                             */
/***************************************************************/
cache build_cache(int S, int E, int b)
{
	/* Implement this function */
    cache result_cache;
    result_cache.s = index_bit(S);    /*     int S: The set of cache                                 */
    result_cache.E = E;               /*     int E: The associativity way of cache                   */
    result_cache.b = index_bit(b);    /*     int b: The blocksize of cache                           */


//    printf("block offset: %d \t, set of cache: %d \n",result_cache.b,result_cache.s);


    cset* result_set;
    result_set = (cset*)malloc(sizeof(cset) * S );


    for (int i = 0; i < S; i++) {
        cline* result_cline;
        result_cline = (cline*)malloc(sizeof(cline) * E);
        result_set[i].lines = result_cline;
    }

    result_cache.sets = result_set;

    for(int i = 0; i < S; i++) {
        for(int j =0; j < E; j ++) {
            result_cache.sets[i].lines[j].valid =0;
            result_cache.sets[i].lines[j].modified = 0;
            result_cache.sets[i].lines[j].age = 0;
            result_cache.sets[i].lines[j].tag = 0;
        }
    }

	 return result_cache;
}

/***************************************************************/
/*                                                             */
/* Procedure : access_cache                                    */
/*                                                             */
/* Purpose   : Update cache stat and content                   */
/*                                                             */
/* Parameters:                                                 */
/*     cache *L: An accessed cache                             */
/*     int op: Read/Write operation                            */
/*     uint32_t addr: The address of memory access             */
/*     int *hit: The number of cache hit                       */
/*     int *miss: The number of cache miss                     */
/*     int *wb: The number of write-back                       */
/*                                                             */
/***************************************************************/
void access_cache(cache *L, char *op, uint32_t addr, int *hit, int *miss, int *wb)
{
    /*************************************************************/
    /* Implement this function                                   */
    /* cache가 초기화된 상태임.                                       */
    /* 32bit 중에서                                                */
    /* set의 개수는 2의 N 제곱으로 표현 되고 N만큼 비트를 차지하고             */
    /* block은 2의 N 제곱으로 표현 되고 N만큼 비트를 차지하고                */
    /* 나머지는 tag가 비트를 차지 한다.                                  */
    /*************************************************************/

#ifdef DEBUG
    number ++; //debug
#endif

    int anchor_block = L->b;
    int anchor_set = L->s;
    int anchor_way = L->E;
    int anchor_tag = 32 - anchor_set - anchor_block;

    int tag_bit_addr;
    int index_bit_addr;
    int offset_bit_addr;


    char * string_binary;
    string_binary = num_to_bits(addr,32);


    tag_bit_addr = (uint32_t)fromBinary(slice(string_binary,0,anchor_tag-1));
    index_bit_addr = (uint32_t)fromBinary(slice(string_binary,anchor_tag,anchor_tag+anchor_set-1));
    offset_bit_addr = (uint32_t)fromBinary(slice(string_binary,anchor_tag + anchor_set,31));



#ifdef DEBUG

    printf("%d번째 라인\n",number);
    printf("stringBinary: %s\n",string_binary);
    printf("slice Tag: %s\n", slice(string_binary,0,anchor_tag-1));
    printf("slice Offset: %s\n", slice(string_binary,anchor_tag + anchor_set, 31));
    printf("slice Index: %s\n", slice(string_binary,anchor_tag,anchor_tag + anchor_set-1));
    printf("offset: %d, index: %d, tag: %d, op: %c\n", offset_bit_addr,index_bit_addr,tag_bit_addr,op[0]);
    printf("addr: %#x\n",addr);


    printf("\n");

#endif

    /*valid가 0인 경우 miss 1인 경우 hit */


    int is_not_coincide = 0; //빈 공간
    int is_LRU = 0; // 0일 때 LRU 1이면 LRU아님
    int target = 0;
    uint32_t old_age = UINT32_MAX;


    switch (*op) {
        case 'R':
            for(int i = 0 ; i< anchor_way; i++) {

                if(L->sets[index_bit_addr].lines[i].valid == 1){

                    if (L->sets[index_bit_addr].lines[i].tag == tag_bit_addr) { // tag가 일치하는 경우.
                        (*hit)++;

                        L->sets[index_bit_addr].lines[i].age = time;
                        is_LRU = 1;
                        is_not_coincide = 1;
                        break;
                    }
                }
            }

            if(is_not_coincide ==0 ){
                for(int i =0 ; i < anchor_way; i++) {
                    if(L->sets[index_bit_addr].lines[i].valid == 0) {
                        L->sets[index_bit_addr].lines[i].valid = 1;
                        L->sets[index_bit_addr].lines[i].tag = tag_bit_addr;
                        L->sets[index_bit_addr].lines[i].age = time;

                        (*miss)++;
                        is_LRU = 1;
                        break;
                    }
                }
            }

            /** 태그가 일치하는 것이 없고, 캐쉬도 꽉 차 있는 경우 **/
            /* LRU를 한다. 그러기 위해서는 가장 오래된 것을 찾아야 한다.*/

            if(is_LRU == 0 ) {

                for(int i =0 ; i < anchor_way ; i++) {
                    if (old_age > L->sets[index_bit_addr].lines[i].age) {
                        old_age = L->sets[index_bit_addr].lines[i].age;
                        target = i; //target is oldest
                    }
                }

                L->sets[index_bit_addr].lines[target].tag = tag_bit_addr;
                L->sets[index_bit_addr].lines[target].age = time;
                L->sets[index_bit_addr].lines[target].valid = 1;
                (*miss)++;

                if(L->sets[index_bit_addr].lines[target].modified == 1) {
                    L->sets[index_bit_addr].lines[target].modified = 0;
                    (*wb)++;
                }
            }
            break;

        case 'W':

            for(int i = 0 ; i< anchor_way; i++) {
                if (L->sets[index_bit_addr].lines[i].valid == 1){

                    if (L->sets[index_bit_addr].lines[i].tag == tag_bit_addr) { // tag가 일치하는 경우.
                        L->sets[index_bit_addr].lines[i].modified = 1;
                        L->sets[index_bit_addr].lines[i].age = time;

                        (*hit)++;
                        is_LRU = 1;
                        is_not_coincide = 1;
                        break;
                    }
                }
            }
            if(is_not_coincide ==0 ){
                for(int i =0 ; i < anchor_way; i++) {
                    if(L->sets[index_bit_addr].lines[i].valid == 0) {

                        L->sets[index_bit_addr].lines[i].modified = 1;
                        L->sets[index_bit_addr].lines[i].valid = 1;
                        L->sets[index_bit_addr].lines[i].tag = tag_bit_addr;
                        L->sets[index_bit_addr].lines[i].age = time;
                        (*miss)++;
                        is_LRU = 1;
                        break;
                    }
                }
            }

            if(is_LRU == 0 ) {
                for(int i =0 ; i < anchor_way ;i++) {
                    if (old_age > L->sets[index_bit_addr].lines[i].age) {
                        old_age = L->sets[index_bit_addr].lines[i].age;
                        target = i; //target is oldest
                    }
                }

                L->sets[index_bit_addr].lines[target].valid = 1;
                L->sets[index_bit_addr].lines[target].tag = tag_bit_addr;
                L->sets[index_bit_addr].lines[target].age = time;
                (*miss)++;

                if(L->sets[index_bit_addr].lines[target].modified == 1) {
                    (*wb)++;
                }
                L->sets[index_bit_addr].lines[target].modified = 1;
            }
            break;
    }

}

/***************************************************************/
/*                                                             */
/* Procedure : cdump                                           */
/*                                                             */
/* Purpose   : Dump cache configuration                        */
/*                                                             */
/***************************************************************/
void cdump(int capacity, int assoc, int blocksize)
{

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
/* Purpose   : Dump cache stat                                 */
/*                                                             */
/***************************************************************/
void sdump(int total_reads, int total_writes, int write_backs,
           int reads_hits, int write_hits, int reads_misses, int write_misses)
{
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
/*                                                             */
/* Cache Design                                                */
/*                                                             */
/*      cache[set][assoc][word per block]                      */
/*                                                             */
/*                                                             */
/*       ----------------------------------------              */
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
/*                                                             */
/*                                                             */
/***************************************************************/
void xdump(cache* L)
{
    int i, j, k = 0;
    int b = L->b, s = L->s;
    int way = L->E, set = 1 << s;

    uint32_t line;


    printf("Cache Content:\n");
    printf("-------------------------------------\n");

    for(i = 0; i < way; i++) {
        if(i == 0) {
            printf("    ");
        }
        printf("      WAY[%d]", i);
    }
    printf("\n");

    for(i = 0; i < set; i++) {
        printf("SET[%d]:   ", i);

        for(j = 0; j < way; j++) {
            if(k != 0 && j == 0) {
                printf("          ");
            }
            if(L->sets[i].lines[j].valid) {
                line = L->sets[i].lines[j].tag << (s + b);
                line = line | (i << b);
            }
            else {
                line = 0;
            }
            printf("0x%08x  ", line);
        }
        printf("\n");
    }
    printf("\n");
}


int main(int argc, char *argv[])
{
    int capacity=1024;
    int way=8;
    int blocksize=8;
    int set;

    // Cache
    cache simCache;


    // Counts
    int read=0, write=0, writeback=0;
    int readhit=0, writehit=0;
    int readmiss=0, writemiss = 0;

    // Input option
    int opt = 0;
    char* token;
    int xflag = 0;

    // Parse file
    char *trace_name = (char*)malloc(32);
    FILE *fp;
    char line[16];
    char *op;
    uint32_t addr;

    /* You can define any variables that you want */

    trace_name = argv[argc-1];
    if (argc < 3) {
        printf("Usage: %s -c cap:assoc:block_size [-x] input_trace \n", argv[0]);
        exit(1);
    }

    while((opt = getopt(argc, argv, "c:x")) != -1) {
        switch(opt) {
            case 'c':
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
                printf("Usage: %s -c cap:assoc:block_size [-x] input_trace \n", argv[0]);
                exit(1);

        }
    }

    // Allocate
    set = capacity / way / blocksize;

    /* TODO: Define a cache based on the struct declaration */
    
     simCache = build_cache(set,way,blocksize);


    // Simulate
    fp = fopen(trace_name, "r"); // read trace file
    if(fp == NULL) {
        printf("\nInvalid trace file: %s\n", trace_name);
        return 1;
    }

    cdump(capacity, way, blocksize);


    /* TODO: Build an access function to load and store data from the file */
    while (fgets(line, sizeof(line), fp) != NULL) {
        op = strtok(line, " ");
        addr = strtoull(strtok(NULL, ","), NULL, 16);
        time++;
#ifdef DEBUG
        // You can use #define DEBUG above for seeing traces of the file.
//        fprintf(stderr, "op: %s\n", op);    //  op와 w인지 r인지
//        fprintf(stderr, "addr: %x\n", addr);
        //addr 을 access_cache (cache에 넣어주면 된다)
#endif

        if(*op == 'R') {
            read++;
            access_cache(&simCache,op,addr,&readhit,&readmiss,&writeback);

        }else if(*op == 'W') {
            write++;
            access_cache(&simCache,op,addr,&writehit,&writemiss,&writeback);
        }

    }

    // test example
    sdump(read, write, writeback, readhit, writehit, readmiss, writemiss);
    if (xflag) {
        xdump(&simCache);
    }

    return 0;
}
