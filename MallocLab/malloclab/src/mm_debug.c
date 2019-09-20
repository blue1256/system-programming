/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*********************
 * From csapp textbook 
 ********************/

/* Basic constants and macros */
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

#define MAX(x,y) ((x)>(y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int*)(p))
#define PUT(p, val) (*(unsigned int*)(p) = (val)) 

/* Write ptr at address p */
#define PUT_PTR(p, ptr) (*(unsigned int*)(p) = (unsigned int)(ptr))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & (~0x7))
#define GET_ALLOC(p) (GET(p) & (0x1))

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char*)(bp)+GET_SIZE((char*)(bp)-WSIZE)) 
#define PREV_BLKP(bp) ((char*)(bp)-GET_SIZE((char*)(bp)-DSIZE))

/* Given block ptr bp, get ptr of its pred and succ on payload */
#define SUCCP(bp) ((char*)(bp))
#define PREDP(bp) ((char*)(bp+WSIZE))

/* Given block ptr bp, compute address of predecessor and successor of the free list */
#define SUCC(bp) (*(char**)(bp))
#define PRED(bp) (*(char**)(bp+WSIZE))

/********************
 * End csapp textbook
 *******************/

/* pointer for a heap list */
static char* heap_listp;
static char* free_listp=0;

/*************
 * Subroutines
 ************/

/*
 * put_free - put free block to the free list.
 *
 * Put new free block to the head of the doubly linked list
 *
 */
static void put_free(void* bp){
    PUT_PTR(PREDP(bp), 0);
    if(free_listp!=0)
        PUT_PTR(PREDP(free_listp), bp);
    PUT_PTR(SUCCP(bp), free_listp);
    free_listp = bp;
}

/*
 * del_free - put free block to the free list.
 *
 * Delete free block from the doubly linked list.
 *
 */
static void del_free(void* bp){
    if(PRED(bp)!=0 && SUCC(bp)!=0){
        PUT_PTR(SUCCP(PRED(bp)), SUCC(bp));
        PUT_PTR(PREDP(SUCC(bp)), PRED(bp));
    } else if(PRED(bp)==0 && SUCC(bp)!=0){
        PUT_PTR(PREDP(SUCC(bp)), 0);
        free_listp = SUCC(bp);
    } else if(PRED(bp)!=0 && SUCC(bp)==0){
        PUT_PTR(SUCCP(PRED(bp)), 0);
    } else {
        free_listp = 0;
    }
}

/*
 * coalesce - coalesce with free neighbor blocks.
 *
 * For 4 different cases,
 * modify block header/footer with correct information,
 * and coalesce with neighbor if needed,
 * and insert to the free list,
 * and change the location which bp points to.
 *
 */
static void* coalesce(void* bp){
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc){
        put_free(bp);
        return bp;
    } else if(prev_alloc && !next_alloc){
        del_free(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if(!prev_alloc && next_alloc){
        del_free(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {
        del_free(PREV_BLKP(bp));
        del_free(NEXT_BLKP(bp));
        size += (GET_SIZE(HDRP(PREV_BLKP(bp)))+GET_SIZE(HDRP(NEXT_BLKP(bp))));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    put_free(bp);
    return bp;
}

/*
 * extend_heap - extend the heap by (size_t)words.
 *
 * Recalculate size of words in bytes so that extension maintains alignment,
 * and initialize block header/footer and new epilogue header,
 * then coalesce with previous block if it is free.
 *
 */
static void* extend_heap(size_t words){
    char* bp;
    size_t size;

    size = (words%2) ? (words+1)*WSIZE : words*WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

/*
 * find_fit - find fit block from the heap list.
 *
 * Until bp meets epilogue header, traverse the heap list.
 * If block's size is enough and it is not allocated, return its pointer.
 * If there is no fitting block, return NULL.
 *
 */
static void* find_fit(size_t asize){
    char* bp = free_listp;
    
    while(bp!=0){
        if(asize<=GET_SIZE(HDRP(bp)) && !GET_ALLOC(HDRP(bp))){
            return bp;
        }
        bp = SUCC(bp);
    }
    return NULL;
}

/*
 * place - place block in the heap list.
 *
 * Modify block header/footer,
 * and modify next block header/footer,
 * then insert it to free list.
 */
static void place(void* bp, size_t asize){
    size_t size = GET_SIZE(HDRP(bp));

    if((size-asize)>=2*DSIZE){
        del_free(bp);
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(size-asize, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size-asize, 0));
        put_free(NEXT_BLKP(bp));
    } else {
        del_free(bp);
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
    }
}

void print_heap(){
    char* bp = heap_listp;
    printf("\nheap_listp: %p\n", bp);
    int bn=1;

    printf("*** HEAP INFO ***\n");
    printf("-------------------\n");
    while(GET_SIZE(HDRP(bp))>0){
        printf("block number: %d\n", bn);
        printf("block address: %p\n", bp);
        printf("block size: %u\n", GET_SIZE(HDRP(bp)));
        printf("allocated: %u\n", GET_ALLOC(HDRP(bp)));
        printf("-------------------\n");
        bp = NEXT_BLKP(bp);
        bn++;
    }
    printf("*** END INFO ***\n");
}

void print_free(){
    char* bp = free_listp;
    printf("\nfree_listp: %p\n", free_listp);
    int fn=1;
    
    printf("*** FREE INFO ***\n");
    printf("------------------------\n");
    while(bp!=0){
        printf("free block number: %d\n", fn);
        printf("block address: %p\n", bp);
        printf("successor: %p\n", SUCC(bp));
        printf("predecessor: %p\n", PRED(bp));
        printf("------------------------\n");
        bp = SUCC(bp);
        fn++;
    }
    printf("*** END FREE INFO ***\n");
}

/*****************
 * End subroutines
 ****************/

/* 
 * mm_init - initialize the malloc package.
 * 
 * Initialize the heap at heap_listp with prologue block and epilogue block,
 * and set free_listp to point same block as heap_listp,
 * then extend it with a regular block size of CHUNKSIZE.
 *
 */
int mm_init(void)
{
    printf("initializing...\n");
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp+WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp+(2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp+(3*WSIZE), PACK(DSIZE, 1));
    heap_listp += (2*WSIZE);
    free_listp = 0;

    if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    print_heap();
    print_free();
    printf("allocating %lubytes...\n",size);
    size_t asize;
    size_t extendsize;
    char* bp;

    if(size==0){
        return NULL;
    }

    if(size<=DSIZE){
        asize = 2*DSIZE;
    } else {
        asize = DSIZE*((size+(DSIZE)+(DSIZE-1))/DSIZE);
    }

    if((bp=find_fit(asize))!=NULL){
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE))==NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 *
 * Modify information of block header/footer,
 * then coalesce.
 *
 */
void mm_free(void *bp)
{
    print_heap();
    print_free();
    printf("freeing %p...\n", bp);
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = GET_SIZE(HDRP(oldptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
