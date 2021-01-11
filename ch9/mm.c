#include "csapp.h"
#include "memlib.h"
#include "mm.h"

// Basic constants and macros
#define WSIZE   4    // Word and head/footer size
#define DSIZE   8    // Double word size
#define CHUNKSIZE (1<<12)  // Extend heap by this amount (bytes), which is 4KB

#define MAX(x, y)  ((x) > (y) ? (x) : (y))

// Pack a size and allocated bit into a word, 将大小和已分配bit结合起来并返回一个值
#define PACK(size, alloc)  ((size) | (alloc))

// Read and write a word at address p
#define GET(p)      (*(unsigned int *)(p))  // 强制类型转换是至关重要的
#define PUT(p, val) (*(unsigned int *)(p) = (val)) // 把 val 存放在 p 指向的 word 中

// Read the size and allocated fields from address p
#define GET_SIZE(p)  (GET(p) & ~0x7)  // 最后 3 bit 值为 0，其余 bit 全为 1.
#define GET_ALLOC(p) (GET(p) & 0x1)   // 最后 1 bit 值为 0，其余全为 1.

// Given block ptr bp, compute address of its header and footer
#define HDRP(bp)   ((char *)(bp) - WSIZE)  // 指向 block 的头部，pointer向前移动 4 bytes
#define FTRP(bp)   ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)  // 指向 block 的 footer

// Given block ptr bp, compute address of next and previous blocks, 
// 根据 block size 来计算后面和前面的 block
// 通过当前block的header记录的size计算下一个block的address
#define NEXT_BLKP(bp) ((char *)(bp) + GETSIZE( ((char *)(bp) - WSIZE) ))
// 通过前一个block的footer记录的size计算上一个block的address
#define PREV_BLKP(bp) ((char *)(bp) - GETSIZE( ((char *)(bp) - DSIZE) ))  


// declare static functions
static void *extend_heap(size_t word);
static void *coalesce(void *bp);
static char *find_fit(size_t size);


// heap list pointer
void * heap_listp;

/*
 * 从内存系统得到 4 个 words，并将它们初始化，创建一个空的空闲链表。
 */
int mm_init(void) {
    // Create the initial empty heap
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) {
        return -1;
    }
    PUT(heap_listp, 0);                             // Alignment padding
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));    // Prologue header
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));    // Prologue footer
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));        // Epilogue header 结尾块头部
    heap_listp += (2*WSIZE);  

    // Extend the empty heap with a free block of CHUNKSIZE bytes
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return -1;
    }
    return 0;
}

/*
 * 将heap拓展 words 
 */
static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    // Allocate an even number of words to maintain alignment
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) {
        return NULL;
    }

    // Initialize free block header/footer and the epilogue header
    PUT(HDRP(bp), PACK(size, 0));   // 原来的结尾块变成新block的header
    PUT(FTRP(bp), PACK(size, 0));   // 先空闲块的footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));  // 新的结尾块header

    // Coalesce if the previous block was free
    return coalesce(bp);
}

/*
 * 释放空闲块
 */
void mm_free(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * 合并 bp block，分四种情况。
 */
static void *coalesce(void *bp) {
    // 首先获取其previous block 和 next block 是否已经被占用
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // Case 1
    if (prev_alloc && next_alloc) {
        return bp;
    }

    // next block is empty
    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    // 前后都为empty
    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    return bp;
}

/*
 * 分配内存空间
 */
void *mm_malloc(size_t size) {
    size_t asize;       // Adjusted block size
    size_t extendsize;  // Amount to extedn heap if no fit
    char *bp;

    // Ignore spurious requests
    if (size == 0) return NULL;

    // Adjust block size to include overhead and alignment reqs.
    if (size <= DSIZE) 
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    
    // Search the free list for a fit
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    // No fit found. Get more meory and place the block
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) 
        return NULL;
    place(bp, asize);
    return bp;
}

static char *find_fit(size_t size) {
    char *bp = heap_listp + 2*WSIZE;
    if (GET_SIZE(HDRP(bp)) >= size && GET_ALLOC(HDRP(bp))) {
        return bp;
    } else {
        bp = NEXT_BLKP(bp);
    }
    return NULL;
}







