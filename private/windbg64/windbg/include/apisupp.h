/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Apisupp.h

Abstract:

Author:

    David J. Gilman (davegi) 04-May-1992

Environment:

    Win32, User Mode

--*/

#if ! defined( _APISUPP_ )
#define _APISUPP_


#define MHOmfLock(a) (a)
#define MHOmfUnLock(a)

extern LPTD LptdFuncEval;


typedef struct _SYM_FILE_LOAD_ERR {
    _SYM_FILE_LOAD_ERR * Flink;
    _SYM_FILE_LOAD_ERR * Blink;
    
    // Handle to symbol file
    HANDLE  hSymFile;

    // Load error
    SHE     she;

    // Sym handler errors
    PTSTR   pszSHE;
    PTSTR   pszExtendedSHE;
    
    // Path to symbol file
    TCHAR   szSymFilePath[_MAX_PATH];
} SYM_FILE_LOAD_ERR, * PSYM_FILE_LOAD_ERR;

typedef struct {
    BOOL                bFound;
    PFNVALIDATEEXE      pfnValidateExe;
    PVLDCHK             pVldChk;
    TCHAR               szImageFilePath[_MAX_PATH];
    // Head of the list. The head of the
    // list may contain information.
    SYM_FILE_LOAD_ERR   LoadErr;
} FIND_SYM_FILE, * PFIND_SYM_FILE;


void
CopyShToEe(
    void
    );

void
SYSetFrame(
    HFRAME
    hFrame
    );

XOSD
SYFixupAddr(
    PADDR paddr
    );

XOSD
SYUnFixupAddr(
    PADDR addr
    );

XOSD
SYSanitizeAddr(
    PADDR paddr
    );

XOSD
SYIsStackSetup(
    HPID hpidCurr,
    HTID htidCurr,
    PADDR paddr
    );

VOID
SetFindExeBaseName(
    LPSTR lpName
    );

// kcarlos
// BUGBUG
// dead code
/*
VOID
SplitPath(
    LSZ  lsz1,
    LSZ  lsz2,
    LSZ  lsz3,
    LSZ  lsz4,
    LSZ  lsz5
    );
*/


PVOID
MHAlloc(
    size_t cb
    );

PVOID
MHRealloc(
    PVOID lpv,
    size_t cb
    );

void
MHFree(
    PVOID lpv
    );

LPTSTR
MHStrdup(
    LPCTSTR str
    );


HDEP
MMAllocHmem(
    size_t cb
    );

HDEP
MMReallocHmem(
    HDEP hmem,
    size_t cb
    );

void
MMFreeHmem(
    HDEP hmem
    );

BOOL
MMFIsLocked(
    HDEP hMem
    );

void*
MMLpvLockMb(
    HDEP hmem
    );

void
MMbUnlockMb(
    HDEP hmem
    );

DWORD
OSDAPI
DHGetNumber(
    LPSTR String,
    LPLONG Result
    );


typedef struct _NEARESTSYM {
    CXT     cxt;
    ADDR    addrP;
    HSYM    hsymP;
    ADDR    addrN;
    HSYM    hsymN;
} NEARESTSYM, *LPNEARESTSYM;

LPSTR GetNearestSymbolFromAddr(LPADDR lpaddr, LPADDR lpAddrRet);
BOOL  GetNearestSymbolInfo(LPADDR lpaddr, LPNEARESTSYM lpnsym);
LPSTR FormatSymbol(HSYM hsym, PCXT lpcxt);

#endif // _APISUPP_
