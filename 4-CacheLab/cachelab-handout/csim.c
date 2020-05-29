/*
 * A cache simulator in csim.c that takes a 
 * valgrind memory trace as input, simulates the hit/miss behavior of 
 * a cache memory on this trace, and outputs the total number of hits, 
 * misses, and evictions.
 * Author: Fan Zou
 */

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>   /* for getopt */
#include <stdlib.h>  /* for atoi */
#include "cachelab.h"

#define ADDR_SIZE 64

/* Global variables, read from command line */
int verbose_flag;
int set_num;
int associativity;
int block_num;
char* filename;
const char* opt_args = "hvs:E:b:t:";

int hit_count = 0;
int miss_count = 0;
int eviction_count = 0;

typedef struct {
    unsigned long tag;
    unsigned long count;  /* invalid when count == 0 */
} cache_line;



/* Read options from command line, use getopt function */
void set_arg(int argc, char** argv) {
    int opt;
    while ((opt = getopt(argc, argv, opt_args)) != -1) {
        switch (opt)
        {
        case 'h':
            /* code */
            printf("help\n");
            exit(0);
            break;
        case 'v':
            verbose_flag = 1;
            printf("v = %d\n", verbose_flag);
            break; 
        case 's':
            set_num = atoi(optarg);
            printf("s = %d\n", set_num);
            break;
        case 'E':
            associativity = atoi(optarg);
            printf("E = %d\n", associativity);
            break;
        case 'b':
            block_num = atoi(optarg);
            printf("b = %d\n", block_num);
            break;
        case 't':
            filename = optarg;
            printf("filename = %s\n", optarg);
            break;
        default:
            printf("Wrong argument!\n");
            break;
        }
    }
}


/* Input address, set_num, associativity */
void try_hit(unsigned long addr, cache_line* cache) {
    static int timestamp = 0;
    int tag_len = ADDR_SIZE - set_num - block_num;
    unsigned long tag = addr >> (set_num + block_num);
    unsigned long set = (addr << tag_len) >> (tag_len + block_num);
    int t = set * associativity;
    int min = 0;

    /* determine whether it's cache hit */
    for (int i = 0; i < associativity; ++i) {
        int ctmp = cache[t+i].count;
        int ttmp = cache[t+i].tag;

        /* cache hit */
        if (ctmp != 0 && ttmp == tag) {
            cache[t+i].count = ++timestamp;
            ++hit_count;
            if (verbose_flag) printf(" hit\n");
            return;
        }

        /* set is not full */
        else if (ctmp == 0) {
            cache[t+i].count = ++timestamp;
            cache[t+i].tag = tag;
            ++miss_count;
            if (verbose_flag) printf(" miss\n");
            return;
        }

        /* maintain the minimum timestamp */
        if (cache[t+i].count < cache[t+min].count) {
            min = i;
        }
    }

    /* set is full, need to replace victim cache_line, choose the smallest timestamp */
    cache[t+min].count = ++timestamp;
    cache[t+min].tag = tag;
    ++miss_count;
    ++eviction_count;
    if (verbose_flag) printf(" miss eviction"); 
    return;
}



int main(int argc, char** argv)
{
    set_arg(argc, argv);
    if (filename == NULL) {
        printf("Input file invalid\n");
        exit(1);
    }

    int S = 1 << set_num;
    int E = 1 << associativity;
    cache_line *cache = (cache_line *) calloc(S*E, sizeof(cache_line)); // calloc will do initialization

    char op;
    unsigned long addr;
    short sz;
    FILE* fp =  fopen(filename, "r");
    while (fscanf(fp, " %c %lx,%hd", &op, &addr, &sz) > 0) {
        if (op == 'I') continue;
        if (verbose_flag) printf("operation %c, address: 0x%lx", op, addr);
        try_hit(addr, cache);
        if (op == 'M') try_hit(addr, cache);
    }
    fclose(fp);
    free(cache);
    
    printSummary(hit_count, miss_count, eviction_count);

    return 0;
}
