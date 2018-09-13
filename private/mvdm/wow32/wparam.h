/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WPARAM.H
 *  WOW32 16-bit handle alias support
 *
 *  History:
--*/

typedef enum ParamMode {
  PARAM_NONE,
  PARAM_16,
  PARAM_32
} ParamMode;

/*
 *   FindParamMap
 *
 *      lpFindParam is for wparam.c use (set to NULL)
 *
 *      lParam is either 16-bit or 32-bit memory pointer
 *
 *      fMode is either param_16 or param_32
 *
 */


DWORD FindParamMap(VOID* lpFindParam, DWORD lParam, UINT fMode);

PVOID AddParamMap(DWORD dwPtr32, DWORD dwPtr16);

BOOL DeleteParamMap(DWORD lParam, UINT fMode, BOOL* pfFreePtr);

DWORD GetParam16(DWORD dwParam32);

BOOL W32CheckThunkParamFlag(void);

VOID FreeParamMap(HAND16 htask16);

BOOL SetParamRefCount(DWORD dwParam, UINT fMode, DWORD dwRefCount);


// Add dwPtr16 mapping for a parameter, which is allocated
// size of the buffer is cbExtra and 32-bit pointer to the buffer
// is returned

PVOID AddParamMapEx(DWORD dwPtr16, DWORD cbExtra);

// Update node when pointer is suspect to have been moved
// returns updated pointer (32-bit)
PVOID ParamMapUpdateNode(DWORD dwPtr, UINT fMode, VOID* lpn);

VOID InitParamMap(VOID);

///////////////////////////////////////////////////////////////
//
// This is rather fast and dirty heap allocator
//

typedef struct tagBlkHeader *PBLKHEADER;

typedef struct tagBlkHeader {
    PBLKHEADER pNext;
    DWORD dwSize; // block size
} BLKHEADER, *PBLKHEADER;

typedef struct tagBlkCache {
	LPBYTE pCache;
#ifdef DEBUG
	PBLKHEADER pCacheHead;
#endif
	PBLKHEADER pCacheFree;
	DWORD dwCacheSize;
	DWORD dwFlags;

} BLKCACHE, *PBLKCACHE;

LPVOID	CacheBlockAllocate	(PBLKCACHE pc, DWORD dwSize);
VOID 	CacheBlockFree		(PBLKCACHE pc, LPVOID lpv);
VOID     CacheBlockInit      (PBLKCACHE pc, DWORD dwCacheSize);


/////////////////////////////////////////////////////////////////


