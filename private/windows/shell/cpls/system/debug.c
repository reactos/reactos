//*************************************************************
//  File name:    DEBUG.C
//
//  Description:  Debug helper code for System control panel
//                applet
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1992-1996
//  All rights reserved
//
//*************************************************************
#include "sysdm.h"

///////////////////////////////////////////////////////////////
//      Constants
///////////////////////////////////////////////////////////////

#ifdef DBG_CODE

#define CCH_LABEL (sizeof(DWORD) * 2)   // 64 BITS == 8 ANSI chars

#define CB_TAG     sizeof(DWORD)
#define DW_TAG      ((DWORD)(0x44535953))   // 'SYSD'

#define DW_TAG2     ((DWORD)(0x444F4F47))   // 'GOOD'

#define CH_FILL     '*'

///////////////////////////////////////////////////////////////
//      Structures and Types
///////////////////////////////////////////////////////////////

/*
 * NOTE!!!!
 *
 * The HOBJHDR structure MUST be a multiple of 8 bytes (64bits) in len!
 * otherwise this code will *FAULT* on ALPHA machines!
 *
 */
typedef struct HHO *PHHO;

struct HHO {
    PHHO    phhoNext;
    PHHO    phhoPrev;
    CHAR    szFile[CCH_LABEL];
    DWORD   iLine;
    DWORD   cBytesData;
    DWORD   dwTmp;
    DWORD   dwTag2;
};

typedef struct HHO HOBJHDR;

typedef struct {
    LPVOID  pvPtr;
    CHAR    szFile[CCH_LABEL];
    DWORD   iLine;
    CHAR    szFreedBy[CCH_LABEL];
    DWORD   iLineFreed;
} FREELOGREC, *PFREELOGREC;

///////////////////////////////////////////////////////////////
//      Global variables
///////////////////////////////////////////////////////////////

/*
 * Root of memory chain
 */
HOBJHDR hhoRoot = { &hhoRoot, &hhoRoot, { 'R', 'O', 'O', 'T' }, 0, sizeof(hhoRoot) };


/*
 * Buffer used for OutputDebugString formatting (See DbgPrintf and DbgStopX)
 */
TCHAR szDbgOutBuffer[1024];

/*
 * Buffer used for logging
 */
#define CFLR_MAX    1024
FREELOGREC aflrFreeLog[CFLR_MAX];
PFREELOGREC g_pflrUnused = NULL;

#define NextFreeLogRec( pflr )    ((pflr >= &aflrFreeLog[CFLR_MAX-1]) ? aflrFreeLog : pflr+1)
#define PrevFreeLogRec( pflr )    ((pflr <= aflrFreeLog) ? &aflrFreeLog[CFLR_MAX-1] : pflr-1)

//***************************************************************
//
// void DbgPrintf( LPTSTR szFmt, ... )
//
//  Formatted version of OutputDebugString
//
//  Parameters: Same as printf()
//
//  History:
//      18-Jan-1996 JonPa       Wrote it
//***************************************************************
void DbgPrintf( LPTSTR szFmt, ... ) {
    va_list marker;

    va_start( marker, szFmt );

    wvsprintf( szDbgOutBuffer, szFmt, marker );
    OutputDebugString( szDbgOutBuffer );

    va_end( marker );
}


//***************************************************************
//
// void DbgStopX(LPSTR mszFile, int iLine, LPTSTR szText )
//
//  Print a string (with location id) and then break
//
//  Parameters:
//      mszFile     ANSI filename (__FILE__)
//      iLine       line number   (__LINE__)
//      szText      Text string to send to debug port
//
//  History:
//      18-Jan-1996 JonPa       Wrote it
//***************************************************************
void DbgStopX(LPSTR mszFile, int iLine, LPTSTR szText ) {
    int cch;

    wsprintf( szDbgOutBuffer, TEXT("SYSDM.CPL (%hs %d) : %s\n"), mszFile, iLine, szText );

    OutputDebugString(szDbgOutBuffer);

    DebugBreak();
}

//***************************************************************
//
// void MemAllocWorker(LPSTR szFile, int iLine, UINT uFlags, UINT cBytes)
//
//  Debug replacement for LocalAlloc
//
//  Parameters:
//      mszFile     ANSI filename (__FILE__)
//      iLine       line number   (__LINE__)
//      uFlags      same as LocalAlloc
//      cBytes      same as LocalAlloc
//
//  History:
//      18-Jan-1996 JonPa       Wrote it
//***************************************************************
HLOCAL MemAllocWorker(LPSTR szFile, int iLine, UINT uFlags, UINT cBytes) {
    PHHO phhoNew;
    HLOCAL hMem;
    LPSTR psz;
    UINT i, cBytesAlloc;

    cBytesAlloc = cBytes;

    //
    // If fixed alloc...
    //
    if ((uFlags & (LMEM_MOVEABLE | LMEM_DISCARDABLE)) != 0) {
        DBGSTOPX( szFile, iLine, "Attempting to allocate movable memory... Returning NULL");
        return NULL;
    }

    cBytesAlloc = cBytes + sizeof(HOBJHDR);

    // DWORD align Tag
    cBytesAlloc = ((cBytesAlloc + 3) & ~3);
    cBytesAlloc += CB_TAG;


    hMem = LocalAlloc( uFlags, cBytesAlloc );

    //
    // If a valid pointer, and it is a fixed pointer...
    //
    phhoNew = (PHHO)hMem;

    if (hMem != NULL) {


        phhoNew->phhoNext = hhoRoot.phhoNext;
        hhoRoot.phhoNext = phhoNew;
        phhoNew->phhoNext->phhoPrev = phhoNew;
        phhoNew->phhoPrev = &hhoRoot;

        phhoNew->dwTag2 = DW_TAG2;

        for( psz = szFile; *psz != '\0'; psz++ );

        for( ; psz != szFile && *psz != ':' && *psz != '/' && *psz != '\\'; psz--);
        if (*psz == ':' || *psz == '/' || *psz == '\\')
            psz++;

        for( i = 0; i < CCH_LABEL; i++ ) {
            phhoNew->szFile[i] = *psz;
            if (*psz) {
                psz++;
            }
        }

        phhoNew->iLine = iLine;

        phhoNew->cBytesData = cBytes;

        phhoNew += 1;   // point phhoNew to 1st byte after structure

        // round up to nearest DWORD
        { LPBYTE pb = (LPBYTE)phhoNew + cBytes;

            cBytesAlloc -= CB_TAG;
            cBytes += sizeof(HOBJHDR);

            while( cBytes < cBytesAlloc ) {
                *pb++ = CH_FILL;
                cBytes++;
            }

            *((LPDWORD)pb) = DW_TAG;
        }
    }

    return (HLOCAL)phhoNew;
}

//***************************************************************
//
// void MemFreeWorker( LPSTR szFile, int iLine, HLOCAL hMem )
//
//  Debug replacement for LocalFree
//
//  Parameters:
//      mszFile     ANSI filename (__FILE__)
//      iLine       line number   (__LINE__)
//      hMem        same as LocalAlloc
//
//  History:
//      18-Jan-1996 JonPa       Wrote it
//***************************************************************
HLOCAL MemFreeWorker( LPSTR szFile, int iLine, HLOCAL hMem ) {
    PHHO phhoMem;
    UINT uFlags;
    UINT cBytes, cBytesAlloc;
    LPSTR psz;
    INT  i;


    if (g_pflrUnused == NULL) {
        ZeroMemory( aflrFreeLog, sizeof(aflrFreeLog) );
        g_pflrUnused = aflrFreeLog;
    }

    if (hMem == NULL) {
        DBGSTOPX( szFile, iLine, "Freeing NULL handle!");
        return LocalFree(hMem);
    }

    phhoMem = (PHHO)hMem - 1;

    if (phhoMem->dwTag2 != DW_TAG2) {
        PFREELOGREC pflr;
        //
        // Our tag has been stompped on, see if we have already freed this object
        //
        for( pflr = PrevFreeLogRec(g_pflrUnused); pflr != g_pflrUnused; pflr = PrevFreeLogRec(pflr) ) {
            if (pflr->pvPtr == phhoMem) {
                DBGPRINTF((TEXT("SYSDM.CPL: Object may have already been freed by %.8hs line %d\n(that obj was allocated by %.8hs line %d)\n"),
                    pflr->szFreedBy, pflr->iLineFreed, pflr->szFile, pflr->iLine));
                break;
            }
        }

        DBGPRINTF((TEXT("SYSDM.CPL: Trashed memory object was allocated in %.8hs line %d (%d bytes)\n"), phhoMem->szFile, phhoMem->iLine, phhoMem->cBytesData));
        DBGSTOPX( szFile, iLine, "Either heap object trashed or not allocated object");
    }

    cBytes = phhoMem->cBytesData;

#if 0
    if (cBytes < 0) {
        // Not our object?
        DBGSTOPX( szFile, iLine, "Either heap object trashed or not allocated object");
        return LocalFree(hMem);
    }
#endif

    cBytes += sizeof(HOBJHDR);

    // DWORD align
    cBytesAlloc = (cBytes + 3) & ~3;

    { LPBYTE pb = (LPBYTE)(phhoMem);
        pb += cBytes;
        while( cBytes < cBytesAlloc ) {
            if (*pb++ != CH_FILL) {
                DBGPRINTF((TEXT("SYSDM.CPL: Trashed memory object was allocated in %.8hs line %d (%d bytes)\n"),
                        phhoMem->szFile, phhoMem->iLine, phhoMem->cBytesData));
                DBGSTOPX( szFile, iLine, "End of structure overwritten");
            }
            cBytes++;
        }

        if (*((LPDWORD)pb) != DW_TAG) {
            DBGSTOPX( szFile, iLine, "Freeing structure that was not allocated!");

            // Not our structure
            return LocalFree(hMem);
        }
    }

    // Our structure, check header
    if (phhoMem->phhoNext->phhoPrev != phhoMem || phhoMem->phhoPrev->phhoNext != phhoMem ) {
        DBGPRINTF((TEXT("SYSDM.CPL: Orphaned memory object was allocated in %.8hs line %d (%d bytes)\n"),
                phhoMem->szFile, phhoMem->iLine, phhoMem->cBytesData));
        DBGSTOPX( szFile, iLine, "Attempting to free orphaned memory object");
    }

    phhoMem->phhoPrev->phhoNext = phhoMem->phhoNext;
    phhoMem->phhoNext->phhoPrev = phhoMem->phhoPrev;

    //
    // Log this free, incase we try and free it twice
    //

    // Mark as freed
    phhoMem->dwTag2 = 0;

    // Remember who alloc'ed obj
    g_pflrUnused->pvPtr = phhoMem;
    CopyMemory( g_pflrUnused->szFile, phhoMem->szFile, sizeof(g_pflrUnused->szFile) );
    g_pflrUnused->iLine = phhoMem->iLine;

    // Remember who freed the obj
    for( psz = szFile; *psz != '\0'; psz++ );

    for( ; psz != szFile && *psz != ':' && *psz != '/' && *psz != '\\'; psz--);
    if (*psz == ':' || *psz == '/' || *psz == '\\')
        psz++;

    for( i = 0; i < CCH_LABEL; i++ ) {
        g_pflrUnused->szFreedBy[i] = *psz;
        if (*psz) {
            psz++;
        }
    }
    g_pflrUnused->iLineFreed = iLine;

    // Point roaming ptr to next record and mark as unused
    g_pflrUnused = NextFreeLogRec(g_pflrUnused);
    ZeroMemory( g_pflrUnused, sizeof(*g_pflrUnused) );

    return LocalFree(phhoMem);
}

//***************************************************************
//
//  void MemExitCheckWorker() {
//
//  Debug replacement for LocalFree
//
//  Parameters:
//      mszFile     ANSI filename (__FILE__)
//      iLine       line number   (__LINE__)
//      hMem        same as LocalAlloc
//
//  History:
//      18-Jan-1996 JonPa       Wrote it
//***************************************************************
void MemExitCheckWorker( void ) {
    PHHO phho;

    for( phho = hhoRoot.phhoNext; phho != &hhoRoot; phho = phho->phhoNext ) {
        DBGPRINTF((TEXT("SYSDM.CPL: Exiting with out freeing object allocated in %.8hs line %d (%d bytes)\n"),
                phho->szFile, phho->iLine, phho->cBytesData));
    }
}

#endif // DBG_CODE
