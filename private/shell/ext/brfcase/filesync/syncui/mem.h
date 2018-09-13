#ifndef _INC_MEM
#define _INC_MEM

#ifdef WIN32
//
// These macros are used in our controls, that in 32 bits we simply call
// LocalAlloc as to have the memory associated with the process that created
// it and as such will be cleaned up if the process goes away.
//
#ifdef DEBUG

LPVOID  PUBLIC MemAlloc(HANDLE hheap, DWORD cb);
LPVOID  PUBLIC MemReAlloc(HANDLE hheap, LPVOID pb, DWORD cb);
BOOL    PUBLIC MemFree(HANDLE hheap, LPVOID pb);
DWORD   PUBLIC MemSize(HANDLE hheap, LPVOID pb);

#else // DEBUG

#define MemAlloc(hheap, cb)       HeapAlloc((hheap), HEAP_ZERO_MEMORY, (cb))
#define MemReAlloc(hheap, pb, cb) HeapReAlloc((hheap), HEAP_ZERO_MEMORY, (pb),(cb))
#define MemFree(hheap, pb)        HeapFree((hheap), 0, (pb))
#define MemSize(hheap, pb)        HeapSize((hheap), 0, (LPCVOID)(pb))

#endif // DEBUG

#else // WIN32

// In 16 bit code we need the Allocs to go from our heap code as we do not
// want to limit them to 64K of data.  If we have some type of notification of
// 16 bit application termination, We may want to see if we can
// dedicate different heaps for different processes to cleanup...

#define MemAlloc(hheap, cb)       Alloc(cb)  /* calls to verify heap exists */
#define MemReAlloc(hheap, pb, cb) ReAlloc(pb, cb)
#define MemFree(hheap, pb)        Free(pb)
#define MemSize(hheap, pb)        GetSize((LPCVOID)pb)

#endif // WIN32


void PUBLIC Mem_Terminate();

extern HANDLE g_hSharedHeap;

// Shared memory allocation functions.
//
//      void _huge* SharedAlloc(long cb);
//          Alloc a chunk of memory, quickly, with no 64k limit on size of
//          individual objects or total object size.  Initialize to zero.
//
void _huge* PUBLIC SharedAlloc(long cb);                              

//      void _huge* SharedReAlloc(void _huge* pb, long cb);
//          Realloc one of above.  If pb is NULL, then this function will do
//          an alloc for you.  Initializes new portion to zero.
//
void _huge* PUBLIC SharedReAlloc(void _huge* pb, long cb);             

//      BOOL SharedFree(void _huge* FAR * ppb);
//          Free a chunk of memory alloced or realloced with above routines.
//          Sets *ppb to zero.
//
BOOL    PUBLIC SharedFree(void _huge*  * ppb);

//      DWORD SharedGetSize(void _huge* pb);
//          Get the size of a block allocated by Alloc()
//      
DWORD   PUBLIC SharedGetSize(void _huge* pb);                      


//      type _huge * SharedAllocType(type);                    (macro)
//          Alloc some memory the size of <type> and return pointer to <type>.
//
#define SharedAllocType(type)           (type _huge *)SharedAlloc(sizeof(type))

//      type _huge * SharedAllocArray(type, int cNum);         (macro)
//          Alloc an array of data the size of <type>.
//
#define SharedAllocArray(type, cNum)    (type _huge *)SharedAlloc(sizeof(type) * (cNum))

//      type _huge * SharedReAllocArray(type, void _huge * pb, int cNum);
//
#define SharedReAllocArray(type, pb, cNum) (type _huge *)SharedReAlloc(pb, sizeof(type) * (cNum))

#endif  // !_INC_MEM

