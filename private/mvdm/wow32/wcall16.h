/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WCALL16.H
 *  WOW32 16-bit message/callback support
 *
 *  History:
 *  Created 11-Mar-1991 by Jeff Parsons (jeffpar)
 *  Changed 18-Aug-1992 by Mike Tricker (MikeTri) Added DOS PDB and SFT prototypes
--*/


/* Function prototypes
 */
HANDLE  LocalAlloc16(WORD wFlags, INT cb, HANDLE hInstance);
HANDLE  LocalReAlloc16(HANDLE hMem, INT cb, WORD wFlags);
VPVOID  LocalLock16(HANDLE hMem);
BOOL    LocalUnlock16(HANDLE hMem);
WORD    LocalSize16(HANDLE hMem);
HANDLE  LocalFree16(HANDLE hMem);
BOOL    LockSegment16(WORD wSeg);
BOOL    UnlockSegment16(WORD wSeg);
HAND16  GetExePtr16( HAND16 hInstance );
WORD    ChangeSelector16( WORD wSeg );
VPVOID  RealLockResource16( HMEM16 hMem, PINT pcb );
WORD    GetModuleFileName16( HAND16 hInst, VPVOID lpszModuleName, WORD cchModuleName );

BOOL CallBack16(INT iRetID, PPARM16 pParms, VPPROC vpfnProc, PVPVOID pvpReturn);


VPVOID FASTCALL malloc16(UINT cb);
BOOL   FASTCALL free16(VPVOID vp);
VPVOID FASTCALL stackalloc16(UINT cb);

#ifdef DEBUG
VOID   FASTCALL StackFree16(VPVOID vp, UINT cb);
#define stackfree16(vp,cb) StackFree16(vp,cb)
#else
VOID   FASTCALL StackFree16(UINT cb);
#define stackfree16(vp,cb) StackFree16(cb)
#endif

ULONG  GetDosPDB16(VOID);
ULONG  GetDosSFT16(VOID);
int WINAPI WOWlstrcmp16(LPCWSTR lpString1, LPCWSTR lpString2);

/* Function prototypes for 16-bit Global memory functions are now in
 * \nt\public\sdk\inc\winntwow.h with slightly different names.  The
 * old names are supported by the following defines:
 */

#define GlobalAllocLock16  WOWGlobalAllocLock16
#define GlobalLock16       WOWGlobalLockSize16
#define GlobalUnlock16     WOWGlobalUnlock16
#define GlobalUnlockFree16 WOWGlobalUnlockFree16
