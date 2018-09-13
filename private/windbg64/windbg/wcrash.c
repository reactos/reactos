/*++


Copyright (c) 1998  Microsoft Corporation

Module Name:

    crash.c

Abstract:

    This file contains the code necessary to generate a
    a user mode dump of the current app. The code will essentially
    take a non-fatal snapshot and allow the debugging session
    to continue.


Author:

    Carlos Klapp (Sept. 24, 98)

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop

#include <crash.h>
#include "dbugexcp.h"


/************************** Data declaration    *************************/
#define MEM_SIZE (64*1024)

/****** Publics ********/

extern LPPD     LppdCommand;
extern LPTD     LptdCommand;
extern LPSHF    Lpshf;   // vector table for symbol handler



/****** Externs from ??? *******/

//
// these are here only so that we can link
// with crashlib.  they are only referenced
// when reading a kernel mode crash dump
//
extern "C" {
ULONG64 KiProcessors[1];
ULONG64 KiPcrBaseAddress[1];
};


/**************************       Private Structures          *************************/

typedef struct _tagMODULELIST {
    LPMODULE_LIST   pModList;
    LPMODULE_ENTRY  pModEntry;
    DWORD           dwModIdx;
    BOOL            bFinished;
} MODULELIST_WCRASH, * PMODULELIST_WCRASH;

//
// private data structure use for communcating
// crash dump data to the callback function
//
typedef struct _tagDEBUGPACKET {
    //HWND                    hwnd;
    //HANDLE                  hEvent;
    //OPTIONS                 options;
    //DWORD                   dwPidToDebug;
    //HANDLE                  hEventToSignal;
    //DWORD                   dwProcessId;
    HPID                    hpid;
    HTID                    htid;
    TCHAR                   szProcessName[_MAX_PATH];
    DWORD                   dwBaseOfImage;
    DWORD                   dwImageSize;
    LPTD                    lptdThreadListHead;
    //LIST_ENTRY              ThreadList;


    //PTHREADCONTEXT          tctx;
    //DWORD                   stackBase;
    //DWORD                   stackRA;
    DEBUG_EVENT             DebugEvent;
    //DWORD                   ExitStatus;
} DEBUGPACKET, *PDEBUGPACKET;

typedef struct _CRASH_DUMP_INFO {
    // TRUE - stop all processing
    // FALSE - continue processing
    BOOL                        bFatalError;

    // Module specific data
    MODULELIST_WCRASH           modlst;

    DEBUGPACKET                 dp;
    EXCEPTION_DEBUG_INFO       *pExceptionInfo;
    DWORD                       MemoryCount;
    DWORD_PTR                   Address;
    PUCHAR                      pMemoryData;
    MEMORY_BASIC_INFORMATION    mbi;
    SIZE_T                      MbiOffset;
    SIZE_T                      MbiRemaining;
    LPTD                        lptdCurThread;      // LIST_ENTRY              ThreadList;
    PCRASH_MODULE               CrashModule;
} CRASH_DUMP_INFO, *PCRASH_DUMP_INFO;


//
// Used by the sort algorithm to make sure that the
// main module is the first in the list
//
static TCHAR szMainModule[MAX_PATH];


/*******************       Func Declarations       *************************/

BOOL GetThreadContext_CrashDump(LPTD, LPCONTEXT, UINT);

void ConvertMemInfoFromOSDFmtToSysFmt(MEMORY_BASIC_INFORMATION *, MEMINFO *);
int __cdecl SkewedSortModEntryByAddress(const void *, const void *);

UINT WINAPI DHGetDebuggeeBytes(ADDR addr, UINT cb, void * lpb);


/**************************       Code          *************************/

BOOL
WindbgCrashDumpCallback(
    DWORD               dwDataType,
    PVOID               *ppvDumpData,
    LPDWORD             pdwDumpDataLength,
    PVOID               pvData
    )
/*++

Routine Description:

    This function is the callback used by crashlib.
    Its purpose is to provide data to DmpCreateUserDump()
    for writting to the crashdump file.

Arguments:

    dwDataType        - requested data type

    ppvDumpData        - pointer to a pointer to the data

    pdwDumpDataLength  - pointer to the data length

    CrashdumpInfo   - DrWatson private data

Return Value:

    TRUE    - continue calling back for the requested data type
    FALSE   - stop calling back and go on to the next data type

--*/
{
    PCRASH_DUMP_INFO pCrashdumpInfo = (PCRASH_DUMP_INFO) pvData;
    DWORD   cb;

    if (pCrashdumpInfo->bFatalError) {
        *pdwDumpDataLength = 0;
        return FALSE;
    }

    switch( dwDataType ) {
        case DMP_DEBUG_EVENT:
            Assert(sizeof(pCrashdumpInfo->dp.DebugEvent) == sizeof(DEBUG_EVENT));

            *ppvDumpData = &pCrashdumpInfo->dp.DebugEvent;
            *pdwDumpDataLength = sizeof(DEBUG_EVENT);
            break;

        case DMP_THREAD_STATE:
            {
                static CRASH_THREAD     CrashThread;
                TST                     tst = {0};

//
// BUGBUG no 64 bit support
//
                *ppvDumpData = &CrashThread;

                if (pCrashdumpInfo->lptdCurThread == NULL) {
                    pCrashdumpInfo->lptdCurThread = pCrashdumpInfo->dp.lptdThreadListHead;
                } else {
                    pCrashdumpInfo->lptdCurThread = pCrashdumpInfo->lptdCurThread->lptdNext;
                }

                if (NULL == pCrashdumpInfo->lptdCurThread) {
                    pCrashdumpInfo->lptdCurThread = NULL;
                    return FALSE;
                }

                ZeroMemory(&CrashThread, sizeof(CrashThread));
                CrashThread.ThreadId = HandleToUlong(pCrashdumpInfo->lptdCurThread->htid);

                if (xosdNone != OSDGetThreadStatus(
                    pCrashdumpInfo->lptdCurThread->lppd->hpid,
                    pCrashdumpInfo->lptdCurThread->htid, &tst)) {

                    CmdLogFmt("Crash warning: No status for thread %d\r\n",
                        pCrashdumpInfo->lptdCurThread->itid);
                    CrashThread.SuspendCount = (DWORD) -1;
                } else {
                    CrashThread.SuspendCount = tst.dwSuspendCount;
                    /*if (CrashThread.SuspendCount != (DWORD)-1) {
                        ResumeThread(ptctx->hThread);
                    }*/
// BUGBUG 64 bit
                    CrashThread.Teb = (DWORD)tst.dwTeb;

                    Assert(tst.dwPriorityMax <= 31);
                    if (tst.dwPriorityMax < 16) {

                        // NOT realtime
                        if (tst.dwPriority <= 6) {
                            CrashThread.PriorityClass = IDLE_PRIORITY_CLASS;
                            CrashThread.Priority = tst.dwPriority - 4;
                        } else if (tst.dwPriority <= 11) {
                            CrashThread.PriorityClass = NORMAL_PRIORITY_CLASS;
                            CrashThread.Priority = tst.dwPriority - 9;
                        } else if (tst.dwPriority <= 15) {
                            CrashThread.PriorityClass = HIGH_PRIORITY_CLASS;
                            if (15 == tst.dwPriority) {
                                CrashThread.Priority = THREAD_PRIORITY_TIME_CRITICAL;
                            } else {
                                CrashThread.Priority = tst.dwPriority - 13;
                            }
                        }

                    } else {
                        CrashThread.PriorityClass = REALTIME_PRIORITY_CLASS;

                        if (15 == tst.dwPriority) {
                            CrashThread.Priority = THREAD_PRIORITY_TIME_CRITICAL;
                        } else {
                            CrashThread.Priority = tst.dwPriority - 4;
                        }
                    }
                }
                *pdwDumpDataLength = sizeof(CRASH_THREAD);
            }
            break;

        case DMP_MEMORY_BASIC_INFORMATION:
            while( TRUE ) {
                MEMINFO meminfo = {0};

                pCrashdumpInfo->Address += pCrashdumpInfo->mbi.RegionSize;

                meminfo.addr.addr.off = pCrashdumpInfo->Address;

                if (xosdNone != OSDGetMemoryInformation(pCrashdumpInfo->dp.hpid,
                    NULL, &meminfo)) {
                    return FALSE;
                }

                ConvertMemInfoFromOSDFmtToSysFmt(&pCrashdumpInfo->mbi, &meminfo);

                if ((pCrashdumpInfo->mbi.AllocationProtect & PAGE_GUARD) ||
                    (pCrashdumpInfo->mbi.AllocationProtect & PAGE_NOACCESS)) {
                    continue;
                }
                if ((pCrashdumpInfo->mbi.State & MEM_FREE) ||
                    (pCrashdumpInfo->mbi.State & MEM_RESERVE)) {
                    continue;
                }
                break;
            }
            *ppvDumpData = &pCrashdumpInfo->mbi;
            *pdwDumpDataLength = sizeof(MEMORY_BASIC_INFORMATION);
            break;

        case DMP_THREAD_CONTEXT:
            {
                static CONTEXT          context;

                if (pCrashdumpInfo->lptdCurThread == NULL) {
                    pCrashdumpInfo->lptdCurThread = pCrashdumpInfo->dp.lptdThreadListHead;
                } else {
                    pCrashdumpInfo->lptdCurThread = pCrashdumpInfo->lptdCurThread->lptdNext;
                }

                if (NULL == pCrashdumpInfo->lptdCurThread) {
                    pCrashdumpInfo->lptdCurThread = NULL;
                    return FALSE;
                }

                ZeroMemory(&context, sizeof(context));
                if (!GetThreadContext_CrashDump(pCrashdumpInfo->lptdCurThread, &context,
                    sizeof(context))) {
                    CmdLogFmt("!!!Crash error: No context for thread %d\r\n",
                        pCrashdumpInfo->lptdCurThread->itid);
                    pCrashdumpInfo->bFatalError = TRUE;
                }

                *ppvDumpData = &context;
                *pdwDumpDataLength = sizeof(CONTEXT);
            }
            break;

        case DMP_MODULE:
            //
            // Initialize module list
            //
            if (!pCrashdumpInfo->modlst.pModList) {
                if (xosdNone != OSDGetModuleList( LppdCur->hpid,
                    LptdCur->htid, NULL, &pCrashdumpInfo->modlst.pModList)) {

                    //
                    // Error obtaining mod list
                    //
                    CmdLogFmt("!!!Crash error: Unable to obtain module list\r\n");
                    pCrashdumpInfo->bFatalError = TRUE;
                    // Fall thru to cleanup
                    pCrashdumpInfo->modlst.bFinished = TRUE;
                } else {

                    //
                    // Obtained mod list
                    //
                    if (ModuleListCount(pCrashdumpInfo->modlst.pModList) > 0) {
                        // Necessary for everything to function. This sort will
                        // always place the process before the modules. It then
                        // sorts the modules by mem address.
                        qsort(FirstModuleEntry(pCrashdumpInfo->modlst.pModList),
                            ModuleListCount(pCrashdumpInfo->modlst.pModList),
                            sizeof(MODULE_ENTRY), SkewedSortModEntryByAddress);

                        // Get the first module entry
                        pCrashdumpInfo->modlst.pModEntry =
                            FirstModuleEntry(pCrashdumpInfo->modlst.pModList);
                        pCrashdumpInfo->modlst.dwModIdx = 0;
                    } else {
                        CmdLogFmt("!!!Crash error: Module list is empty\r\n");
                        pCrashdumpInfo->bFatalError = TRUE;
                        // Fall thru to cleanup
                        pCrashdumpInfo->modlst.bFinished = TRUE;
                    }
                }
            }

            //
            // Cleanup ??
            //
            if (pCrashdumpInfo->modlst.bFinished) {
                // Cleanup
                if (pCrashdumpInfo->modlst.pModList) {
                    MHFree(pCrashdumpInfo->modlst.pModList);
                }
                ZeroMemory(&pCrashdumpInfo->modlst, sizeof(pCrashdumpInfo->modlst));
                return FALSE;
            }

            //
            // Process the current module
            //
// BUGBUG 64 bit
            pCrashdumpInfo->CrashModule->BaseOfImage = (DWORD)ModuleEntryBase(pCrashdumpInfo->modlst.pModEntry);
            pCrashdumpInfo->CrashModule->SizeOfImage = (DWORD)ModuleEntryLimit(pCrashdumpInfo->modlst.pModEntry);

            // Get module name
            {
                PSTR pszImgName = SHGetExeName( (HEXE)ModuleEntryEmi(pCrashdumpInfo->modlst.pModEntry) );
                Assert(pszImgName);

                pCrashdumpInfo->CrashModule->ImageNameLength = strlen(pszImgName) + 1;
                strcpy(pCrashdumpInfo->CrashModule->ImageName, pszImgName);

            }
            *pdwDumpDataLength = sizeof(CRASH_MODULE) + pCrashdumpInfo->CrashModule->ImageNameLength;
            *ppvDumpData = pCrashdumpInfo->CrashModule;

            //
            // Get the next module
            //
            pCrashdumpInfo->modlst.dwModIdx++;
            if (pCrashdumpInfo->modlst.dwModIdx < ModuleListCount(pCrashdumpInfo->modlst.pModList)) {
                pCrashdumpInfo->modlst.pModEntry = NextModuleEntry(pCrashdumpInfo->modlst.pModEntry);
            } else {
                pCrashdumpInfo->modlst.bFinished = TRUE;
            }
            break;

        case DMP_MEMORY_DATA:
            {
                MEMINFO meminfo = {0};

                if (!pCrashdumpInfo->MemoryCount) {
                    pCrashdumpInfo->Address = 0;
                    pCrashdumpInfo->MbiOffset = 0;
                    pCrashdumpInfo->MbiRemaining = 0;
                    ZeroMemory( &pCrashdumpInfo->mbi, sizeof(MEMORY_BASIC_INFORMATION) );
                    pCrashdumpInfo->pMemoryData = (PUCHAR) VirtualAlloc(
                        NULL,
                        MEM_SIZE,
                        MEM_COMMIT,
                        PAGE_READWRITE
                        );
                }
                if (!pCrashdumpInfo->MbiRemaining) {
                    while( TRUE ) {

                        pCrashdumpInfo->Address += pCrashdumpInfo->mbi.RegionSize;

                        meminfo.addr.addr.off = pCrashdumpInfo->Address;

                        if (xosdNone != OSDGetMemoryInformation(pCrashdumpInfo->dp.hpid,
                            NULL, &meminfo)) {

                            if (pCrashdumpInfo->pMemoryData) {
                                VirtualFree( pCrashdumpInfo->pMemoryData, MEM_SIZE, MEM_RELEASE );
                            }
                            return FALSE;
                        }

                        ConvertMemInfoFromOSDFmtToSysFmt(&pCrashdumpInfo->mbi, &meminfo);

                        if ((pCrashdumpInfo->mbi.Protect & PAGE_GUARD) ||
                            (pCrashdumpInfo->mbi.Protect & PAGE_NOACCESS)) {
                            continue;
                        }
                        if ((pCrashdumpInfo->mbi.State & MEM_FREE) ||
                            (pCrashdumpInfo->mbi.State & MEM_RESERVE)) {
                            continue;
                        }
                        pCrashdumpInfo->MbiOffset = 0;
                        pCrashdumpInfo->MbiRemaining = pCrashdumpInfo->mbi.RegionSize;
                        pCrashdumpInfo->MemoryCount += 1;
                        break;
                    }
                }
                *pdwDumpDataLength = (DWORD)__min( pCrashdumpInfo->MbiRemaining, MEM_SIZE );
                pCrashdumpInfo->MbiRemaining -= *pdwDumpDataLength;

                meminfo.addr.addr.off = ((UOFFSET)pCrashdumpInfo->mbi.BaseAddress + pCrashdumpInfo->MbiOffset);
                if (xosdNone != OSDReadMemory(pCrashdumpInfo->dp.hpid,
                    pCrashdumpInfo->dp.htid, &meminfo.addr,
                    pCrashdumpInfo->pMemoryData, *pdwDumpDataLength, &cb)) {

                    CmdLogFmt( "Crash warning: could not read memory at 0x%I64X\n", meminfo.addr.addr.off);
                }

                *ppvDumpData = pCrashdumpInfo->pMemoryData;
                pCrashdumpInfo->MbiOffset += *pdwDumpDataLength;
            }
            break;
    }

    return TRUE;
}


LOGERR
LogCrash(
    LPSTR   pszFileNameArg,
    DWORD   /*dwUnused*/
    )
{
    LOGERR logerr = LOGERROR_NOERROR;
    PTSTR pszDumpName = NULL;

    //
    // This should have been handled by the "LogDotCommand" function
    //
    Assert(!g_contWorkspace_WkSp.m_bKernelDebugger);

    //
    // Error checking
    //
    CmdInsertInit();

    // don't generate a dump of a dump
    if (g_contWorkspace_WkSp.m_bUserCrashDump ||
        (g_contWorkspace_WkSp.m_bKernelDebugger && g_contKernelDbgPreferences_WkSp.m_bUseCrashDump)) {
        CmdLogFmt( "Crash: <.crash> is not allowed for crash dumps\n" );
        return LOGERROR_QUIET;
    }

    // Process must be stopped
    if (IsProcRunning( LppdCur )) {
        CmdLogFmt( "Crash: process must first be stopped\n" );
        return LOGERROR_QUIET;
    }

    PreRunInvalid();

    PDWildInvalid();
    TDWildInvalid();

    // debuggee has to be active
    if (!DebuggeeActive()) {
        CmdLogFmt("Crash: There is not process to dump\r\n");
        return LOGERROR_QUIET;
    }

    //  Skip over any leading blanks
    pszDumpName = CPSkipWhitespace(pszFileNameArg);

    //  Check for no argument
    if (!*pszDumpName) {
        CmdLogFmt("Crash: Must specify a file name\r\n");
        return LOGERROR_QUIET;
    }


    //
    // We finally get to do something
    //

    //
    // Initialize structures
    //
    CRASH_DUMP_INFO     CrashdumpInfo = {0};

    //
    // Init CrashdumpInfo
    *szMainModule = NULL;

    // Get the stopped thread
    {
        TCHAR szExt[_MAX_EXT] = {0};
        LPTD lptd = LppdCur->lptdList;

        for (; lptd; lptd = lptd->lptdNext) {
            if (tsStopped == lptd->tstate || tsException2 == lptd->tstate) {
                CrashdumpInfo.dp.hpid = lptd->lppd->hpid;;
                CrashdumpInfo.dp.htid = lptd->htid;
                CrashdumpInfo.dp.lptdThreadListHead = lptd->lppd->lptdList;

                _tsplitpath(lptd->lppd->lpBaseExeName, NULL, NULL, szMainModule, szExt);
                _tcscat(szMainModule, szExt);
                break;
            }
        }

        if (NULL == CrashdumpInfo.dp.hpid) {
            CmdLogFmt("Crash error: There is no stopped thread on which to base the dump\r\n");
            goto CLEANUP;
        }
    }

    // Get the last debug event for the stopped thread
    {
        PIOCTLGENERIC   pig;
        DWORD           dw;

        pig = (PIOCTLGENERIC)calloc(sizeof(IOCTLGENERIC) + sizeof(DEBUG_EVENT), 1);
        if (!pig) {
            CmdLogFmt("!!!!!Crash error: Unable to allocate memory\r\n");
            logerr = LOGERROR_QUIET;
            goto CLEANUP;
        }

        pig->ioctlSubType = IG_DEBUG_EVENT;
        pig->length = sizeof(DEBUG_EVENT);
        if (xosdNone != OSDSystemService( CrashdumpInfo.dp.hpid,
                          CrashdumpInfo.dp.htid,
                          (SSVC) ssvcGeneric,
                          (LPV)pig,
                          sizeof(IOCTLGENERIC) + sizeof(DEBUG_EVENT),
                          &dw
                          )) {
            CmdLogFmt("!!!!!Crash error: Unable to obtain debug event\r\n");
            logerr = LOGERROR_QUIET;
            goto CLEANUP;
        }

        Assert(sizeof(DEBUG_EVENT) == pig->length);
        memcpy(&CrashdumpInfo.dp.DebugEvent, pig->data, sizeof(DEBUG_EVENT));
        free( pig );
    }

    CrashdumpInfo.pExceptionInfo = &CrashdumpInfo.dp.DebugEvent.u.Exception;
    CrashdumpInfo.CrashModule = (PCRASH_MODULE) calloc( 4096,  1);


    //
    // Write the dump file out
    //
    CmdLogFmt("Crash: generating dump file. This could take a while.\r\n");
    if (DmpCreateUserDump(pszDumpName, WindbgCrashDumpCallback, &CrashdumpInfo)) {

        CmdLogFmt("Crash: success\r\n");

    } else {

        CmdLogFmt("Crash: error generating dump file\r\n");
        logerr = LOGERROR_QUIET;

    }

CLEANUP:
    if (CrashdumpInfo.CrashModule) {
        free(CrashdumpInfo.CrashModule);
    }

    *szMainModule = NULL;
    return logerr;
}

BOOL
GetThreadContext_CrashDump(
    LPTD        lptd,
    LPCONTEXT   lpContext,
    UINT        uSize
    )

/*++

Routine Description:

    This function gets thread context.

Arguments:

    lpContext - Supplies pointer to CONTEXT strructure.

    cbSizeOfContext - Supplies the size of CONTEXT structure.

Return Value:

    TRUE for success; FALSE for failure.

--*/

{
    DWORD dw = 0;

    Assert(uSize < ULONG_MAX);

    return OSDSystemService( lptd->lppd->hpid,
                             lptd->htid,
                             (SSVC) ssvcGetThreadContext,
                             lpContext,
                             (DWORD) uSize,
                             &dw
                           ) == xosdNone;

    Assert(dw == uSize);
}


void
ConvertMemInfoFromOSDFmtToSysFmt(
    MEMORY_BASIC_INFORMATION    * pmiSys,
    MEMINFO                     * pmiOSD
    )
{
    Assert(pmiOSD);
    Assert(pmiSys);

    ZeroMemory(pmiSys, sizeof(*pmiSys));

// BUGBUG 64 bit
    pmiSys->BaseAddress = (PVOID) pmiOSD->addr.addr.off;
    pmiSys->AllocationBase = (PVOID) pmiOSD->addrAllocBase.addr.off;
    pmiSys->RegionSize = (DWORD)pmiOSD->uRegionSize;
    pmiSys->Protect = pmiOSD->dwProtect;
    pmiSys->State = pmiOSD->dwState;
    pmiSys->Type = pmiOSD->dwType;
    pmiSys->AllocationProtect = pmiOSD->dwAllocationProtect;
}

int __cdecl
SkewedSortModEntryByAddress(
    const void *pMod1,
    const void *pMod2
    )
/*++
    The main module will always be first
--*/
{
    TCHAR szFullName[_MAX_PATH] = {0};
    TCHAR szExt[_MAX_EXT] = {0};

    // Build the full name of the modules
    _tsplitpath(SHGetExeName( (HEXE)ModuleEntryEmi((LPMODULE_ENTRY)pMod1) ),
        NULL, NULL, szFullName, szExt);
    _tcscat(szFullName, szExt);

    // Found the main module?
    if (!_tcsicmp(szMainModule, szFullName)) {
        return -1;
    }

    _tsplitpath(SHGetExeName( (HEXE)ModuleEntryEmi((LPMODULE_ENTRY)pMod2) ),
        NULL, NULL, szFullName, szExt);
    _tcscat(szFullName, szExt);

    // Found the main module?
    if (!_tcsicmp(szMainModule, szFullName)) {
        return 1;
    }

    return ModuleEntryBase((LPMODULE_ENTRY)pMod1) < ModuleEntryBase((LPMODULE_ENTRY)pMod2) ? -1 : 1;
}
