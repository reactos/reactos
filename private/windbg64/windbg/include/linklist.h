#ifndef _LINKLIST_
#define _LINKLIST_


/*
**  Prototypes for the list manager system
*/
typedef struct _LLI *PLLI;
typedef struct _LLE *PLLE;

PLLI    WINAPI  LLPlliInit( DWORD, LLF, LPFNKILLNODE, LPFNFCMPNODE );
PLLE    WINAPI  LLPlleCreate( PLLI );
VOID    WINAPI  LLAddPlleToLl( PLLI, PLLE );
VOID    WINAPI  LLInsertPlleInLl( PLLI, PLLE, DWORD );
BOOL    WINAPI  LLFDeletePlleIndexed( PLLI, DWORD );
BOOL    WINAPI  LLFDeleteLpvFromLl( PLLI, PLLE, LPV, DWORD );
BOOL    WINAPI  LLFDeletePlleFromLl( PLLI, PLLE );
PLLE    WINAPI  LLPlleFindNext( PLLI, PLLE );
DWORD   WINAPI  LLChlleDestroyLl( PLLI );
PLLE    WINAPI  LLPlleFindLpv( PLLI, PLLE, LPV, DWORD );
DWORD   WINAPI  LLChlleInLl( PLLI );
LPV     WINAPI  LLLpvFromPlle( PLLE );
VOID    WINAPI  LLUnlockPlle( PLLE );
PLLE    WINAPI  LLPlleGetLast( PLLI );
VOID    WINAPI  LLPlleAddToHeadOfLI( PLLI, PLLE );
BOOL    WINAPI  LLFRemovePlleFromLl( PLLI, PLLE );


#ifdef DBLLINK
extern  PLLE    WINAPI  LLPlleFindPrev( PLLI, PLLE );
#endif // DBLLINK

//
// FCheckPlli is for debug versions ONLY as an integrety check
//

#ifdef DEBUGVER
extern  BOOL    PASCAL          LLFCheckPlli( PLLI );
#else // DEBUGVER
#define                         LLFCheckPlli(plli)  TRUE
#endif // DEBUGVER

#endif // _LINKLIST_
