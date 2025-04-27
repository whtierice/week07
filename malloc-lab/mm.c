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
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// 매크로 기본 설정
#define WSIZE 4 // 워드, 헤더와 푸터 사이즈
#define BSIZE 8 // 더블 워드 사이즈.
#define CHUNKSIZE (1<<12) // 한번에 힙을 확장할 때 크기, 1을 12비트만큼 왼쪽으로 밀어라 -> 2^12 = 4KB

#define MAX(x,y) ((x) > (y)? (x) :(y))

#define PACK(size, alloc) ((size) | (alloc)) // size와 할당여부를 패킹하여라.

#define GET(p)      (*(unsigned int *)(p))
#define PUT(p,val)  (*(unsigned int *)(p) = (val))

// 힙 블록의 헤더/풋터에 “크기+할당 플래그” 같은 메타데이터를 저장·조회하기 위해
// 메모리의 임의 주소에서 4바이트(워드) 단위로 읽기·쓰기
// 워드 크기(WSIZE)를 4바이트로 정하고, 블록 헤더·풋터도 한 워드씩 할당되어있으므로 
// 4바이트씩 읽을 수 있는 unsinged int형 포인터를 이용하는 것.

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
// 헤더를 가리키는 포인터 p에서, 할당여부를 제외한 값을 읽어오기
// 헤더를 가리키는 포인터 p에서, 할당여부만 읽어오기

#define HDRP(bp)        ((char*)(bp) - WSZIE)
#define FTRP(bp)        ((char*)(bp) + GET_SZIE(HDRP(bp)) - DSIZE)
// bp는 payload 시작주소를 가리킨다, 이를 통해, 워드 사이즈(헤더 사이즈)만큼 뒤로 이동해서 헤더를 가리키는 포인터로 설정해라
// bp는 payload 시작주소를 가리킨다, 이를 통해, 헤더에 접근해서 블락 전체 사이즈를 알아낸 다음 그 사이즈 만큼 앞으로 가기, 그러면 다음 블럭의 payload부분을 가리키고 있을테니, 헤더 사이즈, 푸터 사이즈(워드 2개) 만큼 뒤로 오면 푸터의 시작주소이다.

#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
// payload 시작주소를 가리키는 bp에 헤더의 시작주소로 가서 블락의 사이즈를 알아낸 다음 그만큼 더해서 다음 블럭의 payload를 가리켜라.
// payload 시작주소를 가리키는 bp에 이전 블럭의 푸터로 가서 블락의 사이즈를 알아낸다음 그만큼 빼서 이전블럭의 payload를 가리켜라

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{

    // 가장 단순한 할당기
    // 블록 재사용.검색과정이 없음
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {   
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
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
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}