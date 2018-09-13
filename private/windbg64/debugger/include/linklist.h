#ifndef _LINKLIST_
#define _LINKLIST_

#ifdef  __cplusplus
extern "C" {
#endif

//
//
// -- Here's the APIs
//
extern  HLLI    PASCAL  LLHlliInit( DWORD, LLF, LPFNKILLNODE, LPFNFCMPNODE );
extern  HLLE    PASCAL  LLHlleCreate( HLLI );
extern  void    PASCAL  LLAddHlleToLl( HLLI, HLLE );
extern  void    PASCAL  LLInsertHlleInLl( HLLI, HLLE, DWORD );
extern  BOOL    PASCAL  LLFDeleteHlleIndexed( HLLI, DWORD );
extern  BOOL    PASCAL  LLFDeleteLpvFromLl( HLLI, HLLE, LPV, DWORD );
extern  BOOL    PASCAL  LLFDeleteHlleFromLl( HLLI, HLLE );
extern  HLLE    PASCAL  LLHlleFindNext( HLLI, HLLE );
#ifdef DBLLINK
extern  HLLE    PASCAL  LLHlleFindPrev( HLLI, HLLE );
#endif // DBLLINK
extern  DWORD   PASCAL  LLChlleDestroyLl( HLLI );
extern  HLLE    PASCAL  LLHlleFindLpv( HLLI, HLLE, LPV, DWORD );
extern  DWORD   PASCAL  LLChlleInLl( HLLI );
extern  LPV     PASCAL  LLLpvFromHlle( HLLE );
extern  HLLE    PASCAL  LLHlleGetLast( HLLI );
extern  void    PASCAL  LLHlleAddToHeadOfLI( HLLI, HLLE );
extern  BOOL    PASCAL  LLFRemoveHlleFromLl( HLLI, HLLE );

//
// FCheckHlli is for debug versions ONLY as an integrity check
//
#ifdef DEBUGVER
extern  BOOL  PASCAL  LLFCheckHlli( HLLI );
#else // DEBUGVER
#define LLFCheckHlli(hlli)  1
#endif // DEBUGVER
//
// Map memory manager to our source versions
//
#ifdef NT_BUILD
#define AllocHmem(cb) malloc(cb)
#define FreeHmem(h) free(h)
#define LockHmem(h) (h)
#define UnlockHmem(p) (0)
#else
    #define AllocHmem(cb)   BMAlloc(cb) // _fmalloc(cb)
    #define FreeHmem(h)     BMFree(h)          // _ffree(h)
    #define LockHmem(h)     BMLock(h)          // (h)
    #define UnlockHmem(h)   BMUnlock(h)          //
#endif

//
//  This helps the codes appearance!
//
#define UnlockHlle(hlle)    UnlockHmem(hlle)
#define UnlockHlli(hlli)    UnlockHmem(hlli)

#ifndef hlleNull
#define hlleNull    (HLLE)NULL
#endif // !hlleNull

#ifdef  __cplusplus
}
#endif

#endif // _LINKLIST_
