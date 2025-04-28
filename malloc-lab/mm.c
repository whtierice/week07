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
#define DSIZE (2*WSIZE) // 더블 워드 사이즈.
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

#define HDRP(bp)        ((char*)(bp) - WSIZE)
#define FTRP(bp)        ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
// bp는 payload 시작주소를 가리킨다, 이를 통해, 워드 사이즈(헤더 사이즈)만큼 뒤로 이동해서 헤더를 가리키는 포인터로 설정해라
// bp는 payload 시작주소를 가리킨다, 이를 통해, 헤더에 접근해서 블락 전체 사이즈를 알아낸 다음 그 사이즈 만큼 앞으로 가기, 그러면 다음 블럭의 payload부분을 가리키고 있을테니, 헤더 사이즈, 푸터 사이즈(워드 2개) 만큼 뒤로 오면 푸터의 시작주소이다.

#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
// payload 시작주소를 가리키는 bp에 헤더의 시작주소로 가서 블락의 사이즈를 알아낸 다음 그만큼 더해서 다음 블럭의 payload를 가리켜라.
// payload 시작주소를 가리키는 bp에 이전 블럭의 푸터로 가서 블락의 사이즈를 알아낸다음 그만큼 빼서 이전블럭의 payload를 가리켜라

static char *heap_listp = 0; // 프로로그 블록의 페이로드 시작을 가리키기 위한 포인터

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // 16바이트로 시작하는 이유, 4바이트 패딩, 프로로그 블록의 헤더, 푸터, 에필로그 블록의 헤더
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *) -1)
        return -1;

    PUT(heap_listp, 0);
    // 0–3: 정렬 패딩(padding)  
    // mem_sbrk가 반환한 주소가 8바이트 경계가 아닐 경우를 대비해 빈 공간(0)으로 채워,
    // 이후 프로로그 헤더가 정확히 8바이트 배수 지점에서 시작하도록 함
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
    // 4–7: 프로로그 헤더(Prologue Header)  
    // 크기 DSIZE(8바이트), 할당 플래그=1로 패킹하여 “항상 할당된” 가짜 블록 헤더로 설정
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
    // 8–11: 프로로그 풋터(Prologue Footer)  
    // 헤더와 동일한 값으로 프로로그 블록 끝을 표시, 병합 로직에서 이전 블록 존재를 보장
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));
    // 12–15: 에필로그 헤더(Epilogue Header)  
    // 크기=0, 할당=1로 설정하여 힙의 끝을 나타내는 가짜 블록 헤더로 사용
    heap_listp += (2*WSIZE);
    // heap_listp를 프로로그 블록의 페이로드 시작 위치(8바이트 이후)로 이동  
    // 이후부터 실제 첫 번째 할당 블록이 이 위치를 기준으로 시작됨

    // 포인터는 항상 페이로드 시작주소를 가리킴

    return 0;
}




// 힙을 지정한 크기만큼 늘리고, 새 영역을 free블록으로 초기화하여 반환해줌.
void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    // 확장요청이 홀수이면 짝수로 만든 뒤 WSIZE를 곱하기, 이러면 항상 size는 2워드 배수가 됨.
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;

    // 힙 확장 함수를 호출하여 힙을 늘리자, memsbrk의 반환형이 void 포인터이므로 이를 long으로 캐스팅하여 -1과 비교하자.
    // long으로 캐스팅 하는 이유는 int(보통 32비트) 은 포인터보다 작을 수 있어서, 64비트 시스템에서 포인터 값을 잘못 잘라버릴 위험이 있지만
    // 반면 long 은 32비트 시스템에서는 32비트, 64비트 시스템에서는 64비트로 포인터와 같은 크기를 가지므로, (long)bp == -1 비교 시 포인터 값이 손상되지 않는다!
    bp = mem_sbrk(size);

    if ((long)bp == -1){
        return NULL;
    }


    // 위에서 bp에 늘어난 힙의 시작주소를 반환받았다. 헤더로 가서 사이즈와 할당여부를 기록하자.
    // free블록이므로 footer에도 기록하자.
    // 원래 에필로그 헤더는 새로 생성된 블락에 덮어씌워진다.
    // NEXT_BLKP(bp) 로 확장된 블록 바로 다음(새 에필로그)의 페이로드 시작을 구하고,
    // 그 위치의 헤더를 “크기=0, alloc=1” 로 설정해 힙 끝을 표시하자.

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    
    // 방금 만든 “free 블록” bp와 인접한 블록이 모두 free 상태이면 합치고,
    // 최종 병합된 블록의 페이로드 주소를 반환합니다.
    return coalesce(bp);
  
}


/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{

    size_t asize; // 바이트 -> 워드 환산용 변수
    size_t extendsize;
    char *bp;

    // 올바르지 않은 요청 거절
    if (size == 0){
        return NULL;
    }

    if (size <= DSIZE){
        asize = 2*DSIZE;
    }
    else{
        asize = DSIZE * ((size + (DSIZE) + (DSIZE -1)) / DSIZE);
        // 8
    }

    // 빈 리스트 찾고, 배치하기
    bp = find_fit(asize);
    
    if (bp != NULL){
        place(bp, asize);
        return bp;
    }

    extendsize = (MAX(asize, CHUNKSIZE)); // 추가 요청 사이즈와 최소 확장사이즈 비교
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL){
        return NULL;
    }
    place(bp, asize);
    return bp;


}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));


    // 헤더와 푸터로 가서 블락의 사이즈만큼 할당여부를 0으로 바꾸고
    // 인접한 free 블락이 있으면 병합.
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
    
    // 병합후의 블록 위치를 활용할 일이 있다면(명시적 리스트), 반환값을 받아두는 편이 좋다.
}

void *coalesce(void *bp)
{   
    // 병합을 위해선 이전블락의 할당여부와, 다음블락의 할당여부를 알아야하므로 헤더와 footer를 이용해 구하자.
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    

    // 둘다 할당돼있으면 병합할 수 없으므로 그냥 return
    if (prev_alloc && next_alloc){
        return bp;
    }

    // 뒤에게 할당 안되어있으면, 다음 블록의 헤더로 가서 사이즈를 읽은다음, 지금 블락의 사이즈와 더하자.
    // 더해진 사이즈와 할당여부를 헤더에 덮어 쓰고,
    // footer에도 마찬가지로 덮어씌우기
    else if(prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

    }

    // 이전 블럭이 할당안되어있다면, 이전 블럭의 헤더로 가서 사이즈를 읽은 다음 지금 블락의 사이즈와 더하자.
    // footer의 위치는 그대로이므로 바로 가서 덮어씌우기
    // 합쳐질 블락의 헤더로 가서 사이즈 덮어씌우기
    // 포인터 위치 이전 블럭의 payload 시작위치로 바꾸기
    else if (!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    // 둘다 안되어있으면 이전블록의 footer, 다음블록의 header에 접근하여 사이즈를 구한 뒤,
    // 이전 블록의 헤더에 값 덮어 씌우기,
    // 다음 블록의 footer에 값 덮어 씌우고
    // 이전 블록의 payload 시작위치로 포인터를 변경,
    else{
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + 
        GET_SIZE(FTRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
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