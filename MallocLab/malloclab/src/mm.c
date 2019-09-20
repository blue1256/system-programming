/*
 * mm.c - Seglist allocator.
 * 
 * In this assignment, seglist allocator was used.
 * There are 20 free lists sized by power of two, and each free block is linked to
 * appropriate free list for its size. Every block has its own header and footer,
 * and every free block has SUCC(successor) and PRED(predecessor) pointer used for the free list. 
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

/* pointer for heap list */
static char* heap_listp;

/* pointer for free lists(in two-power size) */
static char* free_list0=0;
static char* free_list1=0;
static char* free_list2=0;
static char* free_list3=0;
static char* free_list4=0;
static char* free_list5=0;
static char* free_list6=0;
static char* free_list7=0;
static char* free_list8=0;
static char* free_list9=0;
static char* free_list10=0;
static char* free_list11=0;
static char* free_list12=0;
static char* free_list13=0;
static char* free_list14=0;
static char* free_list15=0;
static char* free_list16=0;
static char* free_list17=0;
static char* free_list18=0;
static char* free_list19=0;

/*************
 * Subroutines
 ************/

/* get_index - get index of free list for given size
 *
 * Calculate and return appropriate index.
 *
 */
static int get_index(size_t size){
    int index = 18;
    int csize = (1<<18);
    while(index>=0){
        if(size>csize)
            break;
        index--;
        csize = (1<<index);
    }
    return index+1;
}

/* get_list - get corresponding free list for given index 
 *
 * Return approrpriate free_list.
 *
 */
static char** get_list(int index){
    switch(index){
        case 0: return &free_list0;
        case 1: return &free_list1;
        case 2: return &free_list2;
        case 3: return &free_list3;
        case 4: return &free_list4;
        case 5: return &free_list5;
        case 6: return &free_list6;
        case 7: return &free_list7;
        case 8: return &free_list8;
        case 9: return &free_list9;
        case 10: return &free_list10;
        case 11: return &free_list11;
        case 12: return &free_list12;
        case 13: return &free_list13;
        case 14: return &free_list14;
        case 15: return &free_list15;
        case 16: return &free_list16;
        case 17: return &free_list17;
        case 18: return &free_list18;
        default: return &free_list19;
    }
}

/*
 * find_free - find free block in the free lists.
 *
 * Check if bp is in the free lists.
 * Return 1 if found, else return 0.
 *
 */
static int find_free(void* bp){
    int index = get_index(GET_SIZE(HDRP(bp)));
    char* tar = *get_list(index);
    while(tar!=0){
        if(tar==bp){
            return 1;
        }
        tar = SUCC(tar);
    }
    return 0;
}

/*
 * put_free - put free block to the free list.
 *
 * Put new free block to 
 * doubly linked list regarding the size.
 *
 */
static void put_free(void* bp){
    int index = get_index(GET_SIZE(HDRP(bp)));
    char** free_listp = get_list(index);
    if(*free_listp==0){
        PUT_PTR(PREDP(bp), 0);
        PUT_PTR(SUCCP(bp), 0);
        *free_listp = bp;
        return;
    }
    void* tar = *free_listp;
    while(SUCC(tar)!=0){
        if(tar>bp)
            break;
        tar = SUCC(tar);
    }
    if(tar==(*free_listp)){
        PUT_PTR(PREDP(*free_listp), bp);
        PUT_PTR(PREDP(bp), 0);
        PUT_PTR(SUCCP(bp), *free_listp);
        *free_listp = bp;
    } else if(SUCC(tar)!=0) {
        PUT_PTR(PREDP(bp), tar);
        PUT_PTR(SUCCP(bp), SUCC(tar));
        PUT_PTR(PREDP(SUCC(tar)), bp);
        PUT_PTR(SUCCP(tar), bp);
    } else {
        PUT_PTR(PREDP(bp), tar);
        PUT_PTR(SUCCP(bp), 0);
        PUT_PTR(SUCCP(tar), bp);
    }
}

/*
 * del_free - delete free block to the free list.
 *
 * Delete free block from the doubly linked list.
 *
 */
static void del_free(void* bp){
    int index = get_index(GET_SIZE(HDRP(bp)));
    char** free_listp = get_list(index);
    if(PRED(bp)!=0 && SUCC(bp)!=0){
        PUT_PTR(SUCCP(PRED(bp)), SUCC(bp));
        PUT_PTR(PREDP(SUCC(bp)), PRED(bp));
    } else if(PRED(bp)==0 && SUCC(bp)!=0){
        PUT_PTR(PREDP(SUCC(bp)), 0);
        *free_listp = SUCC(bp);
    } else if(PRED(bp)!=0 && SUCC(bp)==0){
        PUT_PTR(SUCCP(PRED(bp)), 0);
    } else {
        *free_listp = 0;
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
        char* nbp = NEXT_BLKP(bp);
        del_free(nbp);
        size += GET_SIZE(HDRP(nbp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if(!prev_alloc && next_alloc){
        char* pbp = PREV_BLKP(bp);
        del_free(pbp);
        size += GET_SIZE(HDRP(pbp));
        PUT(HDRP(pbp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = pbp;
    } else {
        char* pbp = PREV_BLKP(bp);
        char* nbp = NEXT_BLKP(bp);
        del_free(pbp);
        del_free(nbp);
        size += (GET_SIZE(HDRP(pbp))+GET_SIZE(HDRP(nbp)));
        PUT(HDRP(pbp), PACK(size, 0));
        PUT(FTRP(nbp), PACK(size, 0));
        bp = pbp;
    }
    put_free(bp);
    return bp;
}

/*
 * extend_heap - extend the heap by (size_t)words.
 *
 * Recalculate size of words in bytes so that extension maintains alignment,
 * and initialize block header/footer and new epilogue header,
 * then coalesce with neighbors block if they are free.
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
 * Until bp meets epilogue header, traverse appropriate free list.
 * If block's size is enough and it is not allocated, return its pointer.
 * If there is no fitting block in the free list, traverse larger classes.
 * If there is no fitting block, return NULL.
 *
 */
static void* find_fit(size_t asize){
    int index = get_index(asize);
    char** free_listp;
    char* bp;
    
    while(index<20){
        free_listp = get_list(index);
        if(free_listp!=0 || index==19){
            bp = *free_listp;
            while(bp!=0){
                if(asize<=GET_SIZE(HDRP(bp)) && !GET_ALLOC(HDRP(bp))){
                    return bp;
                }
                bp = SUCC(bp);
            }
        }
        index++;
    }
    return NULL;
}

/*
 * place - place block in the heap list.
 *
 * Modify block header/footer,
 * and modify next block header/footer,
 * (segregating single block)
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

/*****************
 * End subroutines
 ****************/

/*
 * mm_check - Heap consistency checker.`
 * 
 * I: Checking heap list
 *
 *  Starts from the heap_listp, check every block for its consistency.
 *  mm_check function checks all of the followings for each block:
 *      1. Are there any contiguous free blocks in the heap?
 *      2. If the block is free, is it in the free list?
 *      3. Does the pointer point to valid heap address?
 *      4. Are there any overlapped blocks?
 *
 * II: Checking free lists
 *
 *  Starts from every free_list, check every free block for its consistency.
 *  mm_check function chekcs all of the follwings for each free block:
 *      1. Is every block marked as free?
 *      2. Does the pointer point tot valid heap address?
 *
 * If every condition mentioned above is satisfied, mm_check returns 0.
 * If not, print corressponding error messages, then mm_check returns -1.
 *
 */
int mm_check(){
    char* bp = heap_listp;
    char* lo = mem_heap_lo();
    char* hi = mem_heap_hi();

    int cont = 0;
    size_t alloc;
    size_t size;
    size_t nsize;
    while(GET_SIZE(HDRP(bp))!=0 || GET_SIZE(HDRP(NEXT_BLKP(bp)))!=0){
        if(bp<lo || bp>hi){
            printf("There is invalid address in the heap list.\n");
            return -1;
        }

        alloc = GET_ALLOC(HDRP(bp));
        size = GET_SIZE(HDRP(bp));
        nsize = GET_SIZE(HDRP(NEXT_BLKP(bp)));

        if(!alloc){
            if(cont){
                printf("There are contiguous free blocks in the heap.\n");
                return -1;
            }
            if(!find_free(bp)){
                printf("There is a free block not in the free list.\n");
                return -1;
            }
            cont = 1;
        } else {
            char* nftr = FTRP(NEXT_BLKP(bp));
            if(nftr!=(HDRP(bp)+size+nsize-WSIZE)){
                printf("There are overlapped blocks in the heap.\n");
                return -1;
            }
            cont = 0;
        }
        bp = NEXT_BLKP(bp);
    }

    for(int i=0;i<20;i++){
        char* free_listp = *get_list(i);
        bp = free_listp;
        while(bp!=0){
            if(bp<lo || bp>hi){
                printf("There is invalid address in the free list.\n");
                return -1;
            }
            alloc = GET_ALLOC(HDRP(bp));
            if(alloc){
                printf("There is allocated block in the free list.\n");
                return -1;
            }
            bp = SUCC(bp);
        }
    }
    return 0;
}

/* 
 * mm_init - initialize the malloc package.
 * 
 * Initialize the heap at heap_listp with prologue block and epilogue block,
 * and set all the free_lists to 0,
 * then extend it with a regular block size of CHUNKSIZE.
 *
 */
int mm_init(void)
{
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp+WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp+(2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp+(3*WSIZE), PACK(DSIZE, 1));
    heap_listp += 2*WSIZE;
    free_list0=0;
    free_list1=0;
    free_list2=0;
    free_list3=0;
    free_list4=0;
    free_list5=0;
    free_list6=0;
    free_list7=0;
    free_list8=0;
    free_list9=0;
    free_list10=0;
    free_list11=0;
    free_list12=0;
    free_list13=0;
    free_list14=0;
    free_list15=0;
    free_list16=0;
    free_list17=0;
    free_list18=0;
    free_list19=0;

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
 * mm_free - Modify header and footer then coalesce.
 *
 * Modify information of block header/footer,
 * then coalesce.
 *
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Reallocate only when it is necessary.
 *
 * If there is more free block or epilogue header after ptr,
 * extend the block.
 * If not, just allocate new block for reallocation.
 *
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    size_t asize;
    int padding;
    int remain;

    if(size<=DSIZE){
        asize = 2*DSIZE;
    } else {
        asize = DSIZE*((size+(DSIZE)+(DSIZE-1))/DSIZE);
    }

    padding = GET_SIZE(HDRP(ptr)) - asize;
    if(padding<0){
        if(GET_ALLOC(HDRP(NEXT_BLKP(ptr)))==0 || GET_SIZE(HDRP(NEXT_BLKP(ptr)))==0){
            remain = GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(NEXT_BLKP(ptr))) - asize;

            if(remain<0){
                size_t extendsize = MAX(-remain, CHUNKSIZE);
                if(extend_heap(extendsize/WSIZE)==NULL)
                    return NULL;
                remain = remain+extendsize;
            }
            del_free(NEXT_BLKP(ptr));

            PUT(HDRP(ptr), PACK(asize+remain,1));
            PUT(FTRP(ptr), PACK(asize+remain,1));
        } else {
            copySize = GET_SIZE(HDRP(oldptr));
            if (size < copySize)
                copySize = size;
            newptr = mm_malloc(size);
            memcpy(newptr, oldptr, copySize);
            mm_free(oldptr);
            oldptr=newptr;
        }
    }
    return oldptr;
}

