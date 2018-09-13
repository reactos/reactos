//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       debug.cxx
//
//  Contents:   Shell debugging functionality
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include <olectl.h>
#endif

#ifndef X_MAPICODE_H_
#define X_MAPICODE_H_
#include <mapicode.h>
#endif

#ifndef X_STDIO_H_
#define X_STDIO_H_
#include <stdio.h>
#endif

#ifndef X_DOCOBJ_H_
#define X_DOCOBJ_H_
#include <docobj.h>
#endif

#ifdef UNIX // IEUNIX: it should be in objbase.h. Here to prevent build break.
#ifndef X_URLMON_H_
#define X_URLMON_H_
#include <urlmon.h>
#endif
#endif // UNIX

HRESULT UnLoadPSAPI();

#ifdef WIN16
#define GetStackBacktrace(a, b, c, d)   0
#define GetStringFromSymbolInfo(dwAddr, psi, pszString) wsprintfA(pszString, "  %08x <symbols not available>", dwAddr)

#endif

//  Globals

#ifdef _MAC
BOOL    fEnableMacCheckHeap = TRUE;
#endif

typedef BOOL (WINAPI * PFNDLLMAIN)(HANDLE, DWORD, LPVOID);

HMODULE                 g_hModule          = NULL;
HINSTANCE               g_hinstMain        = NULL;
HANDLE                  g_hProcess         = NULL;
HANDLE                  g_rgDllHandle[8];
PFNDLLMAIN              g_rgDllMain[8];

BOOL                    g_fAbnormalProcessTermination = FALSE;
BOOL                    g_fOutputToConsole = FALSE;
BOOL                    g_fDetached        = FALSE;

CRITICAL_SECTION        g_csTrace;
CRITICAL_SECTION        g_csResDlg;
CRITICAL_SECTION        g_csDebug;
CRITICAL_SECTION        g_csHeapHack;
CRITICAL_SECTION        g_csSpy;

DWORD                   g_dwTls = (DWORD) -1;

static DBGTHREADSTATE *    s_pts;

static HANDLE           s_hFileLog         = INVALID_HANDLE_VALUE;

//  TAGS and stuff

/*
 *  Number of TAG's registered so far.
 *
 */
TRACETAG tagMac;


/*
 *  Mapping from TAG's to information about them.  Entries
 *  0...tagMac-1 are valid.
 */
TGRC    mptagtgrc[tagMax];


TRACETAG     tagDefault                  = tagNull;
TRACETAG     tagError                    = tagNull;
TRACETAG     tagWarn                     = tagNull;
TRACETAG     tagAssertPop                = tagNull;
TRACETAG     tagAssertExit               = tagNull;
TRACETAG     tagAssertStacks             = tagNull;
TRACETAG     tagTestFailures             = tagNull;
TRACETAG     tagTestFailuresIgnore       = tagNull;
TRACETAG     tagRRETURN                  = tagNull;
TRACETAG     tagValidate                 = tagNull;
TRACETAG     tagLeaks                    = tagNull;
TRACETAG     tagLeaksExpected            = tagNull;
TRACETAG     tagSymbols                  = tagNull;
TRACETAG     tagSpySymbols               = tagNull;
TRACETAG     tagTrackItf                 = tagNull;
TRACETAG     tagTrackItfVerbose          = tagNull;
TRACETAG     tagThrd                     = tagNull;
TRACETAG     tagMemoryStrict_            = tagNull;
TRACETAG     tagCoMemoryStrict_          = tagNull;
TRACETAG     tagMemoryStrictTail_        = tagNull;
TRACETAG     tagMemoryStrictAlign_       = tagNull;
TRACETAG     tagOLEWatchvar              = tagNull;
TRACETAG     tagMemTrace                 = tagNull;
TRACETAG     tagPerf_                    = tagNull;
TRACETAG     tagTraceCalls               = tagNull;
TRACETAG     tagHexDumpLeaks             = tagNull;
TRACETAG     tagNoLeakAssert             = tagNull;
TRACETAG     tagMagic                    = tagNull;
TRACETAG     tagStackSpew                = tagNull;

#ifndef _MAC
#define SZ_NEWLINE "\r\n"
#else
#define SZ_NEWLINE "\n"
#endif

static CHAR szStateFileExt[]    = ".tag";
static CHAR szDbgOutFileExt[]   = ".log";
static CHAR szStateFileName[]   = "HTMLPad.dbg";
static CHAR szDbgOutFileName[]  = "HTMLPad.log";


CHAR    rgchTraceTagBuffer[1024] = { 0 };

void    DllProcessAttach(HINSTANCE hinstance);
void    DllProcessDetach();
const   LPTSTR GetHResultName(HRESULT r);
TRACETAG TagRegisterSomething(
        TGTY tgty, CHAR * szOwner, CHAR * szDescrip, BOOL fEnabled = FALSE);
BOOL    SendTagToDisk(TRACETAG tag, BOOL fSendToDisk);
VOID    SpitSzToDisk(CHAR * sz);
void    SetSortFirstFlag (TRACETAG tag);


#ifdef WIN16

BOOL fW16ThreadInited = FALSE;

BOOL FAR PASCAL __loadds
LibMain(
        HANDLE hInst,
    WORD wDataSeg,
        WORD cbHeapSize,
    LPSTR lpszCmdLine )
{
    DllProcessAttach((HINSTANCE) hInst);

    return TRUE;
}

int CALLBACK _WEP(int nExitType)
{
    DllProcessDetach();
    return 1;
}

#else

extern void PerfProcessAttach();
extern void PerfProcessDetach();
extern void MeterProcessAttach();
extern void MeterProcessDetach();

#ifdef UNIX
extern "C"
#endif
BOOL
DllMain(HANDLE hinst, DWORD dwReason, LPVOID lpReason)
{

    DbgExAssertThreadDisable(TRUE);

#if defined(UNIX)
    // IEUNIX Take out use of threads for the assert dialog
    DbgExAssertThreadDisable(TRUE);
#endif

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DllProcessAttach((HINSTANCE)hinst);
        PerfProcessAttach();
        MeterProcessAttach();
        break;

    case DLL_THREAD_DETACH:
        DllThreadDetach(DbgGetThreadState());
        break;

    case DLL_PROCESS_DETACH:
        g_fDetached = TRUE;
        DllProcessDetach();
        MeterProcessDetach();
        PerfProcessDetach();
        break;
    }

    DbgExAssertThreadDisable(FALSE);

    return TRUE;
}
#endif

HRESULT
DllThreadAttach()
{
    DBGTHREADSTATE *   pts;

#ifdef WIN16
    if (!fW16ThreadInited)
    {
        DWORD refcount = w16InitializeThreading();
        if (0 == refcount)
            return -1; // BUGBUG: Wrong error.
        fW16ThreadInited = TRUE;
    }
#endif

    // Allocate directly from the heap, rather than through new, since new
    // requires that the DBGTHREADSTATE is established
#ifndef WIN16
    pts = (DBGTHREADSTATE *)LocalAlloc(LMEM_FIXED, sizeof(DBGTHREADSTATE));
#else
    pts = (DBGTHREADSTATE *)GlobalAllocPtr(GMEM_MOVEABLE, sizeof(DBGTHREADSTATE));
#endif
    if (!pts)
    {
        Assert("Debug Thread initialization failed");
        return E_OUTOFMEMORY;
    }

    memset(pts, 0, sizeof(DBGTHREADSTATE));

    {
        LOCK_GLOBALS;

        pts->ptsNext = s_pts;
        if (s_pts)
            s_pts->ptsPrev = pts;
        s_pts = pts;
    }

    TlsSetValue(g_dwTls, pts);

    return S_OK;
}

void
DllThreadDetach(DBGTHREADSTATE *pts)
{
    DBGTHREADSTATE **  ppts;

    if (!pts)
        return;

    LOCK_GLOBALS;

    for (ppts = &s_pts; *ppts && *ppts != pts; ppts = &((*ppts)->ptsNext));
    if (*ppts)
    {
        *ppts = pts->ptsNext;
        if (pts->ptsNext)
        {
            pts->ptsNext->ptsPrev = pts->ptsPrev;
        }
    }

#ifndef WIN16
   LocalFree(pts);
#else
   GlobalFreePtr(pts);
#endif
}

void
DllProcessAttach(HINSTANCE hinst)
{
    static struct
    {
        TRACETAG *  ptag;
        TGTY        tgty;
        LPSTR       pszClass;
        LPSTR       pszDescr;
        BOOL        fEnabled;
    }
    g_ataginfo[] =
    {
        &tagDefault,                tgtyTrace,  "Debug",
            "General debug output",                             TRUE,
        &tagAssertPop,              tgtyOther,  "Assert",
            "Popups on asserts",                                TRUE,
        &tagAssertExit,             tgtyOther,  "Assert",
            "Exit on asserts",                                  FALSE,
        &tagAssertStacks,           tgtyOther,  "Assert",
            "Stacktraces on asserts",                           TRUE,
        &tagValidate,               tgtyOther,  "Memory",
            "Aggressive memory validation",                     FALSE,
        &tagLeaks,                  tgtyTrace,  "Memory",
            "Memory Leaks",                                     TRUE,
        &tagLeaksExpected,          tgtyTrace,  "Memory",
            "Memory Leaks (Expected)",                          FALSE,
        &tagMemoryStrict_,          tgtyOther,  "Memory",
            "Use VMem for MemAlloc",                            FALSE,
        &tagCoMemoryStrict_,        tgtyOther,  "Memory",
            "Use VMem for CoTaskMemAlloc",                      FALSE,
        &tagMemoryStrictTail_,      tgtyOther,  "Memory",
            "VMem strict at end (vs beginning)",                TRUE,
        &tagMemoryStrictAlign_,     tgtyOther,  "Memory",
            "VMem pad to quadword at end",                      FALSE,
        &tagMemTrace,               tgtyOther,  "Memory",
            "Trace Memory Allocations",                         FALSE,
        &tagSymbols,                tgtyTrace,  "Memory",
            "Leaks: Stacktraces & symbols",                     FALSE,
        &tagSpySymbols,             tgtyTrace,  "Memory",
            "Leaks: Stacktraces & symbols for CoTaskMemAlloc",  FALSE,
        &tagHexDumpLeaks,           tgtyTrace,  "Memory",
            "Leaks: Hexdump contents of blocks",                FALSE,
        &tagNoLeakAssert,           tgtyTrace,  "Memory",
            "Leaks: Don't assert on leaks",                     FALSE,
        &tagStackSpew,              tgtyOther,  "Memory",
            "Stack Spew: Fill stack with known value",          TRUE,
        &tagError,                  tgtyTrace,  "Trace",
            "Errors",                                           TRUE,
        &tagWarn,                   tgtyTrace,  "Trace",
            "Warnings",                                         FALSE,
        &tagTestFailures,           tgtyTrace,  "Trace",
            "THR",                                              FALSE,
        &tagTestFailuresIgnore,     tgtyTrace,  "Trace",
            "IGNORE_HR",                                        FALSE,
        &tagRRETURN,                tgtyTrace,  "Trace",
            "RRETURN",                                          FALSE,
        &tagTraceCalls,             tgtyTrace,  "Trace",
            "Trace all THR calls",                              FALSE,
        &tagTrackItf,               tgtyTrace,  "Track",
            "Interface watch",                                  FALSE,
        &tagTrackItfVerbose,        tgtyOther,  "Track",
            "Verbose trace on interface watch",                 FALSE,
        &tagThrd,                   tgtyTrace,  "Thread",
            "Thread related tracing",                           FALSE,
        &tagOLEWatchvar,            tgtyTrace,  "Trace",
            "All calls to OCX interfaces",                      FALSE,
        &tagPerf_,                  tgtyTrace,  "Perf",
            "Perf and size killers",                            FALSE,
        &tagMagic,                  tgtyTrace,  "Magic",
            "Trace imagehlp.dll failures",                      FALSE,
};


    TGRC *  ptgrc;
    char    szLogPath[MAX_PATH];
    int     i;

#ifdef WIN16
    if (!fW16ThreadInited)
    {
        w16InitializeThreading(); // will init if need be.
        fW16ThreadInited = TRUE;
    }
#endif

#ifndef  _MAC
    InitializeCriticalSection(&g_csTrace);
    InitializeCriticalSection(&g_csResDlg);
    InitializeCriticalSection(&g_csDebug);
    InitializeCriticalSection(&g_csHeapHack);
    InitializeCriticalSection(&g_csSpy);
#endif

    g_dwTls = TlsAlloc();
    if (g_dwTls == (DWORD)(-1))
    {
        DbgExAssertThreadDisable(FALSE);
        return;
    }

    EnsureThreadState();

    g_hinstMain = hinst;
#ifdef WIN16
    g_hModule = hinst;
#endif

    // don't want windows to put up message box on INT 24H errors.
    SetErrorMode(SEM_FAILCRITICALERRORS);

    // Initialize simulated failures
    DbgExSetSimFailCounts(0, 1);

    // Initialize TAG array

    tagMac = tagMin;

    // enable tagNull at end of DbgExRestoreDefaultDebugState
    ptgrc = mptagtgrc + tagNull;
    ptgrc->tgty = tgtyNull;
    ptgrc->fEnabled = FALSE;
    ptgrc->ulBitFlags = TGRC_DEFAULT_FLAGS;
    ptgrc->szOwner = "dgreene";
    ptgrc->szDescrip = "NULL";

    for (i = 0; i < ARRAY_SIZE(g_ataginfo); i++)
    {
        *g_ataginfo[i].ptag = TagRegisterSomething(
                g_ataginfo[i].tgty,
                g_ataginfo[i].pszClass,
                g_ataginfo[i].pszDescr,
                g_ataginfo[i].fEnabled);

        SetSortFirstFlag(*g_ataginfo[i].ptag);
    }

    g_hProcess = GetCurrentProcess();

    if (GetEnvironmentVariableA("TRIDENT_LOGPATH", szLogPath, MAX_PATH))
    {
        DbgExOpenLogFile(szLogPath);
    }

    IF_NOT_WIN16(MagicInit());
    DbgExRestoreDefaultDebugState();
}

void
DllProcessDetach(void)
{
    TRACETAG    tag;
    TGRC *      ptgrc;
    int         i;

    EnsureThreadState();

    for (i = 0; i < ARRAY_SIZE(g_rgDllHandle); ++i)
    {
        if (g_rgDllHandle[i] != NULL && g_rgDllMain[i] != NULL)
        {
            g_rgDllMain[i](g_rgDllHandle[i], DLL_PROCESS_DETACH, NULL);
        }
    }

    if (!g_fAbnormalProcessTermination)
    {
        DbgExTraceMemoryLeaks();
    }

    // Close the debug output file
    if (s_hFileLog != INVALID_HANDLE_VALUE)
    {
        char achBuf[128];

        wsprintfA(achBuf, "<<<<< End logging" SZ_NEWLINE);
        SpitSzToDisk(achBuf);
        CloseHandle(s_hFileLog);
        s_hFileLog = INVALID_HANDLE_VALUE;
    }

    // Free the tag strings if not already done
    for (tag = tagMin, ptgrc = mptagtgrc + tag;
         tag < tagMac; tag++, ptgrc++)
    {
        if (ptgrc->TestFlag(TGRC_FLAG_VALID))
        {
            GlobalFreePtr(ptgrc->szOwner);
            ptgrc->szOwner = NULL;
            GlobalFreePtr(ptgrc->szDescrip);
            ptgrc->szDescrip = NULL;
        }
    }

    //    Set flags to FALSE.  Need to separate from loop above so that
    //    final memory leak trace tag can work.

    for (tag=tagMin, ptgrc = mptagtgrc + tag;
         tag < tagMac; tag++, ptgrc++)
    {
        if (ptgrc->TestFlag(TGRC_FLAG_VALID))
        {
            ptgrc->fEnabled = FALSE;
            ptgrc->ClearFlag(TGRC_FLAG_VALID);
        }
    }

    IF_NOT_WIN16(MagicDeinit());

    if (g_dwTls != (DWORD)(-1))
    {
        while (s_pts)
            DllThreadDetach(s_pts);
        TlsFree(g_dwTls);
    }

#ifndef WIN16
    // Unload PSAPI.DLL
    UnLoadPSAPI();
#endif

#ifndef _MAC
    DeleteCriticalSection(&g_csTrace);
    DeleteCriticalSection(&g_csResDlg);
    DeleteCriticalSection(&g_csDebug);
    DeleteCriticalSection(&g_csHeapHack);
    DeleteCriticalSection(&g_csSpy);
#endif

#ifdef WIN16
    if (fW16ThreadInited)
    {
        w16UninitializeThreading();
        //no need for this: fW16ThreadInited = FALSE;
    }
#endif
}


/*
 *  FReadDebugState
 *
 *  Purpose:
 *      Read the debug state information file whose name is given by the
 *      string szDebugFile.  Set up the tag records accordingly.
 *
 *  Parameters:
 *      szDebugFile     Name of debug file to read
 *
 *  Returns:
 *      TRUE if file was successfully read; FALSE otherwise.
 *
 */

BOOL
FReadDebugState( CHAR * szDebugFile )

{
    HANDLE      hfile = NULL;
    TGRC        tgrc;
    TGRC *      ptgrc;
    TRACETAG    tag;
    INT         cchOwner;
    CHAR        rgchOwner[MAX_PATH];
    INT         cchDescrip;
    CHAR        rgchDescrip[MAX_PATH];
    BOOL        fReturn = FALSE;
    DWORD       cRead;

#ifdef UNIX
    CHAR *pszSlash = strrchr(szDebugFile, '/');
    hfile = INVALID_HANDLE_VALUE;

    if ( pszSlash )
    {
        strcpy( szDebugFile, pszSlash+1 );
        hfile = CreateFileA(szDebugFile,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            (HANDLE) NULL);
    }
#else
    hfile = CreateFileA(szDebugFile,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        (HANDLE) NULL);
#endif

    if (hfile != INVALID_HANDLE_VALUE)
    {
        for (;;)
        {
            if (!ReadFile(hfile, &tgrc, sizeof(TGRC), &cRead, NULL))
                break;

            if (cRead == 0)
                break;

            if (!ReadFile(hfile, &cchOwner, sizeof(UINT), &cRead, NULL))
                goto ErrorReturn;
            Assert(cchOwner <= sizeof(rgchOwner));
            if (!ReadFile(hfile, rgchOwner, cchOwner, &cRead, NULL))
                goto ErrorReturn;

            if (!ReadFile(hfile, &cchDescrip, sizeof(UINT), &cRead, NULL))
                goto ErrorReturn;
            Assert(cchDescrip <= sizeof(rgchDescrip));
            if (!ReadFile(hfile, rgchDescrip, cchDescrip, &cRead, NULL))
                goto ErrorReturn;

            ptgrc = mptagtgrc + tagMin;
            for (tag = tagMin; tag < tagMac; tag++)
            {
                if (ptgrc->TestFlag(TGRC_FLAG_VALID) &&
                    !strcmp(rgchOwner, ptgrc->szOwner) &&
                    !strcmp(rgchDescrip, ptgrc->szDescrip))
                {
                    if (!ptgrc->TestFlag(TGRC_FLAG_INITED))
                    {
                        Assert(tgrc.TestFlag(TGRC_FLAG_VALID));

                        ptgrc->fEnabled = tgrc.fEnabled;
                        ptgrc->ulBitFlags = tgrc.ulBitFlags;

                        // Only read each tag once
                        ptgrc->SetFlag(TGRC_FLAG_INITED);
                    }
                    break;
                }

                ptgrc++;
            }
        }

        CloseHandle(hfile);
        fReturn = TRUE;
    }

    goto Exit;

ErrorReturn:
    if (hfile)
        CloseHandle(hfile);

Exit:
    return fReturn;
}

/*
 *  FWriteDebugState
 *
 *  Purpose:
 *      Writes the current state of the Debug Module to the file
 *      name given.  The saved state can be restored later by calling
 *      FReadDebugState.
 *
 *  Parameters:
 *      szDebugFile     Name of the file to create and write the debug
 *                      state to.
 *
 *  Returns:
 *      TRUE if file was successfully written; FALSE otherwise.
 */
BOOL
FWriteDebugState( CHAR * szDebugFile )
{
    HANDLE      hfile = NULL;
    TRACETAG    tag;
    UINT        cch;
    TGRC *      ptgrc;
    BOOL        fReturn = FALSE;
    DWORD       cWrite;

#ifdef UNIX
    CHAR *pszSlash = strrchr(szDebugFile, '/');
    hfile = INVALID_HANDLE_VALUE;

    if ( pszSlash )
    {
        strcpy( szDebugFile, pszSlash+1 );
        hfile = CreateFileA(szDebugFile,
                            GENERIC_WRITE,
                            FILE_SHARE_WRITE,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            (HANDLE) NULL);
    }
#else
    hfile = CreateFileA(szDebugFile,
                        GENERIC_WRITE,
                        FILE_SHARE_WRITE,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        (HANDLE) NULL);
#endif

    if (hfile != INVALID_HANDLE_VALUE)
    {
        for (tag = tagMin; tag < tagMac; tag++)
        {
            ptgrc = mptagtgrc + tag;

            if (!ptgrc->TestFlag(TGRC_FLAG_VALID))
                continue;

            Assert(ptgrc->szOwner);
            Assert(ptgrc->szDescrip);

            if (!WriteFile(hfile, ptgrc, sizeof(TGRC), &cWrite, NULL))
                goto ErrorReturn;

            // SZ fields will be overwritten when read back

            cch = strlen(ptgrc->szOwner) + 1;
            if (!WriteFile(hfile, &cch, sizeof(UINT), &cWrite, NULL))
                goto ErrorReturn;
            if (!WriteFile(hfile, ptgrc->szOwner, cch, &cWrite, NULL))
                goto ErrorReturn;

            cch = strlen(ptgrc->szDescrip) + 1;
            if (!WriteFile(hfile, &cch, sizeof(UINT), &cWrite, NULL))
                goto ErrorReturn;
            if (!WriteFile(hfile, ptgrc->szDescrip, cch, &cWrite, NULL))
                goto ErrorReturn;
        }

        CloseHandle(hfile);
        fReturn = TRUE;
    }

    goto Exit;

ErrorReturn:
    if (hfile)
        CloseHandle(hfile);
#ifndef WIN16
    DeleteFileA(szDebugFile);
#endif

Exit:
    return fReturn;
}


//+------------------------------------------------------------------------
//
//  Function:   SaveDefaultDebugState
//
//  Synopsis:   Saves the debug state of the executing program to a file
//              of the same name, substituting the ".tag" suffix.
//
//  Arguments:  [void]
//
//-------------------------------------------------------------------------

void
SaveDefaultDebugState( void )
{
    CHAR    rgch[MAX_PATH] = "";

    if (g_hinstMain)
    {
#ifndef _MAC
        UINT cch = (UINT) GetModuleFileNameA(g_hModule, rgch, sizeof(rgch));
#ifdef UNIX
        strcat(rgch, szStateFileExt);
#else
        Assert(rgch[cch - 4] == '.');
        strcpy(&rgch[cch - 4], szStateFileExt);
#endif
#else
        CHAR   achAppLoc[MAX_PATH];
        DWORD   dwRet;
        short   iRet;

        dwRet = GetModuleFileNameA(NULL, achAppLoc, ARRAY_SIZE(achAppLoc));
        Assert (dwRet != 0);

        iRet = GetFileTitleA(achAppLoc,rgch,sizeof(rgch));
        Assert(iRet == 0);

        strcat (rgch, szStateFileExt);
#endif
    }
    else
    {
        strcat(rgch, szStateFileName);
    }
    FWriteDebugState(rgch);
}


//+------------------------------------------------------------------------
//
//  Function:   DbgExRestoreDefaultDebugState
//
//  Synopsis:   Restores the debug state for the executing program from
//              the state file of the same name, substituting the ".tag"
//              suffix.
//
//  Arguments:  [void]
//
//-------------------------------------------------------------------------

void WINAPI
DbgExRestoreDefaultDebugState()
{
    CHAR    rgch[MAX_PATH] = "";

    EnsureThreadState();

    if (g_hinstMain)
    {
#if defined(_MAC)
        CHAR   achAppLoc[MAX_PATH];
        DWORD   dwRet;
        short   iRet;

        dwRet = GetModuleFileNameA(NULL, achAppLoc, ARRAY_SIZE(achAppLoc));
        Assert (dwRet != 0);

        iRet = GetFileTitleA(achAppLoc,rgch,sizeof(rgch));
        Assert(iRet == 0);

        strcat (rgch, szStateFileExt);

#elif defined(UNIX)
        UINT cch = (UINT) GetModuleFileNameA(NULL, rgch, sizeof(rgch));
        strcat(rgch, szStateFileExt);

#else
        UINT cch = (UINT) GetModuleFileNameA(NULL, rgch, sizeof(rgch));
        Assert(rgch[cch - 4] == '.');
        strcpy(&rgch[cch - 4], szStateFileExt);
#endif
    }
    else
    {
        strcat(rgch, szStateFileName);
    }
    FReadDebugState(rgch);

    mptagtgrc[tagNull].fEnabled = TRUE;
}

TRACETAG WINAPI
DbgExFindTag(char * szTagDesc)
{
    Assert(szTagDesc);
    for (TRACETAG i = tagMin; i < tagMac; i++)
    {
        if (mptagtgrc[i].szDescrip && strlen(mptagtgrc[i].szDescrip))
        {
            if (!_stricmp(mptagtgrc[i].szDescrip, szTagDesc))
            {
                return i;
            }
        }
    }

    return 0;
}

/*
 *  DbgExIsTagEnabled
 *
 *  Purpose:
 *      Returns a boolean value indicating whether the given TAG
 *      has been enabled or disabled by the user.
 *
 *  Parameters:
 *      tag     The TAG to check
 *
 *  Returns:
 *      TRUE    if the TAG has been enabled.
 *      FALSE   if the TAG has been disabled.
 */

BOOL WINAPI
DbgExIsTagEnabled(TRACETAG tag)
{
    return  mptagtgrc[tag].TestFlag(TGRC_FLAG_VALID) &&
            mptagtgrc[tag].fEnabled;
}


/*
 *  DbgExEnableTag
 *
 *  Purpose:
 *      Sets or resets the TAG value given.  Allows code to enable or
 *      disable TAG'd assertions and trace switches.
 *
 *  Parameters:
 *      tag         The TAG to enable or disable
 *      fEnable     TRUE if TAG should be enabled, FALSE if it should
 *                  be disabled.
 *  Returns:
 *      old state of tag (TRUE if tag was enabled, otherwise FALSE)
 *
 */

BOOL WINAPI
DbgExEnableTag(TRACETAG tag, BOOL fEnable)
{
    BOOL    fOld;

    EnsureThreadState();
    Assert(mptagtgrc[tag].TestFlag(TGRC_FLAG_VALID));
    fOld = mptagtgrc[tag].fEnabled;
    mptagtgrc[tag].fEnabled = fEnable;
    mptagtgrc[tag].ulBitFlags |= TGRC_FLAG_INITED;
    return fOld;
}

void SetSortFirstFlag (TRACETAG tag)
{
    EnsureThreadState();

    Assert(mptagtgrc[tag].TestFlag(TGRC_FLAG_VALID));

    mptagtgrc[tag].ulBitFlags |= TGRC_FLAG_SORTFIRST;
}

BOOL WINAPI
DbgExSetDiskFlag(TRACETAG tag, BOOL fSendToDisk)
{
    BOOL fOld;

    EnsureThreadState();
    Assert(mptagtgrc[tag].TestFlag(TGRC_FLAG_VALID));
    fOld = mptagtgrc[tag].ulBitFlags;
    if (fSendToDisk) mptagtgrc[tag].ulBitFlags |= TGRC_FLAG_DISK;
    else mptagtgrc[tag].ulBitFlags &= ~TGRC_FLAG_DISK;
    return fOld;
}

BOOL WINAPI
DbgExSetBreakFlag(TRACETAG tag, BOOL fBreak)
{
    BOOL fOld;

    EnsureThreadState();
    Assert(mptagtgrc[tag].TestFlag(TGRC_FLAG_VALID));
    fOld = mptagtgrc[tag].ulBitFlags;
    if (fBreak) mptagtgrc[tag].ulBitFlags |= TGRC_FLAG_BREAK;
    else mptagtgrc[tag].ulBitFlags &= ~TGRC_FLAG_BREAK;
    return fOld;
}

void
SpitSzToDisk(char *pch)
{
    DWORD cWrite;

    if (s_hFileLog != INVALID_HANDLE_VALUE && pch && *pch)
    {
        WriteFile(s_hFileLog, pch, strlen(pch), &cWrite, NULL);
        FlushFileBuffers(s_hFileLog);
    }
}

/*
 *  TagRegisterSomething
 *
 *  Purpose:
 *      Does actual work of allocating TAG, and initializing TGRC.
 *      The owner and description strings are duplicated from the
 *      arguments passed in.
 *
 *  Parameters:
 *      tgty        Tag type to register.
 *      szOwner     Owner.
 *      szDescrip   Description.
 *
 *  Returns:
 *      New TAG, or tagNull if none is available.
 */

TRACETAG
TagRegisterSomething(
        TGTY    tgty,
        CHAR *  szOwner,
        CHAR *  szDescrip,
        BOOL    fEnabled)
{
    TRACETAG tag;
    TRACETAG tagNew         = tagNull;
    TGRC *  ptgrc;
    CHAR *  szOwnerDup      = NULL;
    CHAR *  szDescripDup    = NULL;
    UINT    cb;
    BOOL    fSortFirst = FALSE;

    if (szOwner[0] == '!')
    {
        fSortFirst = TRUE;
        szOwner++;
    }

    for (tag = tagMin, ptgrc = mptagtgrc + tag; tag < tagMac;
            tag++, ptgrc++)
    {
        if (ptgrc->TestFlag(TGRC_FLAG_VALID))
        {
            if(!(strcmp(szOwner, ptgrc->szOwner) ||
                strcmp(szDescrip, ptgrc->szDescrip)))
            {
                return tag;
            }
        }
        else if (tagNew == tagNull)
            tagNew= tag;
    }

    // Make duplicate copies.

    Assert(szOwner);
    Assert(szDescrip);
    cb = strlen(szOwner) + 1;

    // we use LocalAlloc here instead of new so
    // we don't interfere with leak reporting because of the
    // dependency between the debug library and the
    // leak reporting code (i.e., don't touch this --Erik)

    szOwnerDup = (LPSTR) GlobalAllocPtr(GMEM_MOVEABLE, cb);

    if (szOwnerDup == NULL)
    {
        goto Error;
    }

    strcpy(szOwnerDup, szOwner);

    cb = strlen(szDescrip) + 1;
    szDescripDup = (LPSTR) GlobalAllocPtr(GMEM_MOVEABLE, cb);

    if (szDescripDup == NULL)
    {
        goto Error;
    }

    strcpy(szDescripDup, szDescrip);

    if (tagNew == tagNull)
    {
        if (tagMac >= tagMax)
        {
#ifdef  NEVER
            AssertSz(FALSE, "Too many tags registered already!");
#endif
            Assert(FALSE);
            return tagNull;
        }

        tag = tagMac++;
    }
    else
        tag = tagNew;

    ptgrc = mptagtgrc + tag;

    ptgrc->fEnabled = fEnabled;
    ptgrc->ulBitFlags = TGRC_DEFAULT_FLAGS;
    ptgrc->tgty = tgty;
    ptgrc->szOwner = szOwnerDup;
    ptgrc->szDescrip = szDescripDup;

    if (fSortFirst)
    {
        ptgrc->ulBitFlags |= TGRC_FLAG_SORTFIRST;
    }

    return tag;

Error:
    GlobalFree((HGLOBAL)szOwnerDup);
    GlobalFree((HGLOBAL)szDescripDup);
    return tagNull;
}


/*
 *  DeregisterTag
 *
 *  Purpose:
 *      Deregisters tag, removing it from tag table.
 *
 *  Parameters:
 *      tag     Tag to deregister.
 */

void
DeregisterTag(TRACETAG tag)
{
    //  don't allow deregistering the tagNull entry
    //  but exit gracefully
    if (!tag)
        return;

    Assert(tag < tagMac);
    Assert(mptagtgrc[tag].TestFlag(TGRC_FLAG_VALID));

    mptagtgrc[tag].fEnabled = FALSE;
    mptagtgrc[tag].ClearFlag(TGRC_FLAG_VALID);
    GlobalFree((HGLOBAL)mptagtgrc[tag].szOwner);
    mptagtgrc[tag].szOwner = NULL;
    GlobalFree((HGLOBAL)mptagtgrc[tag].szDescrip);
    mptagtgrc[tag].szDescrip = NULL;
}


/*
 *  DbgExTagRegisterTrace
 *
 *  Purpose:
 *      Registers a class of trace points, and returns an identifying
 *      TAG for that class.
 *
 *  Parameters:
 *      szOwner     The email name of the developer writing the code
 *                  that registers the class.
 *      szDescrip   A short description of the class of trace points.
 *                  For instance: "All calls to PvAlloc() and HvFree()"
 *
 *  Returns:
 *      TAG identifying class of trace points, to be used in calls to
 *      the trace routines.
 */

TRACETAG WINAPI
DbgExTagRegisterTrace( CHAR * szOwner, CHAR * szDescrip, BOOL fEnabled )
{
    EnsureThreadState();
    return TagRegisterSomething(tgtyTrace, szOwner, szDescrip, fEnabled);
}



TRACETAG
DbgExTagRegisterOther( CHAR * szOwner, CHAR * szDescrip, BOOL fEnabled )
{
    EnsureThreadState();
    return TagRegisterSomething(tgtyOther, szOwner, szDescrip, fEnabled);
}



TRACETAG WINAPI
DbgExTagError()
{
    EnsureThreadState();
    return tagError;
}

TRACETAG WINAPI
DbgExTagAssertExit()
{
    EnsureThreadState();
    return tagAssertExit;
}

TRACETAG WINAPI
DbgExTagAssertStacks()
{
    EnsureThreadState();
    return tagAssertStacks;
}

TRACETAG WINAPI
DbgExTagWarning()
{
    EnsureThreadState();
    return tagWarn;
}

TRACETAG WINAPI
DbgExTagThread()
{
    EnsureThreadState();
    return tagThrd;
}

TRACETAG WINAPI
DbgExTagMemoryStrict()
{
    EnsureThreadState();
    return tagMemoryStrict_;
}

TRACETAG WINAPI
DbgExTagCoMemoryStrict()
{
    EnsureThreadState();
    return tagCoMemoryStrict_;
}

TRACETAG WINAPI
DbgExTagMemoryStrictTail()
{
    EnsureThreadState();
    return tagMemoryStrictTail_;
}

TRACETAG WINAPI
DbgExTagMemoryStrictAlign()
{
    EnsureThreadState();
    return tagMemoryStrictAlign_;
}

TRACETAG WINAPI
DbgExTagOLEWatch()
{
    EnsureThreadState();
    return tagOLEWatchvar;
}

TRACETAG WINAPI
DbgExTagPerf()
{
    EnsureThreadState();
    return tagPerf_;
}

/*
 *  DbgExTaggedTrace
 *
 *  Purpose:
 *      Uses the given format string and parameters to render a
 *      string into a buffer.  The rendered string is sent to the
 *      destination indicated by the given tag, or sent to the bit
 *      bucket if the tag is disabled.
 *
 *  Arguments:
 *      tag     Identifies the tag group
 *      szFmt   Format string for _snprintf (qqv)
 */

BOOL __cdecl
DbgExTaggedTrace(TRACETAG tag, CHAR * szFmt, ...)
{
    BOOL    f;

    EnsureThreadState();

    va_list valMarker;

    va_start(valMarker, szFmt);
    f = DbgExTaggedTraceListEx(tag, 0, szFmt, valMarker);
    va_end(valMarker);

    return f;
}

BOOL __cdecl
DbgExTaggedTraceEx(TRACETAG tag, USHORT usFlags, CHAR * szFmt, ...)
{
    BOOL    f;

    EnsureThreadState();

    va_list valMarker;

    va_start(valMarker, szFmt);
    f = DbgExTaggedTraceListEx(tag, usFlags, szFmt, valMarker);
    va_end(valMarker);

    return f;
}

BOOL WINAPI
DbgExTaggedTraceListEx(TRACETAG tag, USHORT usFlags, CHAR * szFmt, va_list valMarker)
{
    static CHAR szFmtOwner[] = "%s %s: ";
    TGRC *      ptgrc;
    int         cch;
    int         cchCopied;
    int         iIndent;
    DBGTHREADSTATE *pts;
    CHAR        achBuf[4096];

    EnsureThreadState();

    pts = DbgGetThreadState();

    if (tag == tagNull)
        ptgrc = mptagtgrc + tagDefault;
    else
        ptgrc = mptagtgrc + tag;

    if (!ptgrc->fEnabled)
        return FALSE;

    LOCK_TRACE;

    Assert(ptgrc->TestFlag(TGRC_FLAG_VALID));

    // indenting

    cch = 0;

    if (usFlags & TAG_INDENT)
        pts->iIndent++;

    iIndent = min(pts->iIndent, 64);

    if (usFlags & TAG_OUTDENT)
        pts->iIndent--;

    if (iIndent > 0)
    {
        memset(achBuf, ' ', iIndent);
        achBuf[iIndent-1] = (usFlags & TAG_INDENT ? '+' : usFlags & TAG_OUTDENT ? '-' : ' ');
        cch += iIndent;
    }

    // tag name

    if (!(usFlags & TAG_NONAME))
    {
        cch += _snprintf(
                achBuf + cch,
                ARRAY_SIZE(achBuf) - cch,
                szFmtOwner,
                "TRI",
                ptgrc->szOwner);
    }

    cchCopied = hrvsnprintf(
                       achBuf + cch,
                       ARRAY_SIZE(achBuf) - cch,
                       szFmt,
                       valMarker);

    if ( cchCopied == -1 ) {
        cch = ARRAY_SIZE(achBuf) - ARRAY_SIZE(SZ_NEWLINE);
    } else {
        cch += cchCopied;
    }

    if (!(usFlags & TAG_NONEWLINE))
        strcpy(achBuf+cch, SZ_NEWLINE);

    if (ptgrc->TestFlag(TGRC_FLAG_DISK))
    {
        SpitSzToDisk(achBuf);
    }

#ifndef WIN16
    if ((usFlags & TAG_USECONSOLE) || g_fOutputToConsole)
    {
       printf(achBuf);
    }
#endif

    if (!(usFlags & TAG_USECONSOLE))
    {
        OutputDebugStringA(achBuf);
    }

    if (ptgrc->TestFlag(TGRC_FLAG_BREAK))
    {
        AssertSz(FALSE, ptgrc->szDescrip);
    }

    return FALSE;
}

void WINAPI
DbgExTaggedTraceCallers(TRACETAG tag, int iStart, int cTotal)
{
    SYMBOL_INFO      asiSym[32];
    DWORD            dwEip[32];
    int              i;
    int              c;
    CHAR             achSymbol[256];

    EnsureThreadState();

    if (!DbgExIsTagEnabled(tag))
        return;

    if (cTotal > ARRAY_SIZE(dwEip))
        cTotal = ARRAY_SIZE(dwEip);

    c = GetStackBacktrace(iStart + 3, cTotal, dwEip, asiSym);
    for (i = 0; i < c; i++)
    {
        GetStringFromSymbolInfo(dwEip[i], &asiSym[i], achSymbol);
        DbgExTaggedTraceEx(tag, TAG_NONAME, "%s", achSymbol);
    }
}


//+---------------------------------------------------------------
//
//  Function:   GetHResultName
//
//  Synopsis:   Returns a printable string for the given hresult
//
//  Arguments:  [scode] -- The status code to report.
//
//  Notes:      This function disappears in retail builds.
//
//----------------------------------------------------------------

const LPTSTR
GetHResultName(HRESULT r)
{
    LPTSTR lpstr;

#define CASE_SCODE(sc)  \
        case sc: lpstr = _T(#sc); break;

#define CASE_SCODE2(sc,sc2)  \
        case sc: lpstr = _T(#sc) _T("/") _T(#sc2); break;

    switch (r) {
        /* Generic SCODEs */
        CASE_SCODE(S_OK)
        CASE_SCODE(S_FALSE)

        CASE_SCODE(E_ABORT)
        CASE_SCODE(E_ACCESSDENIED)
        CASE_SCODE(E_FAIL)
        CASE_SCODE(E_HANDLE)
        CASE_SCODE(E_INVALIDARG)
        CASE_SCODE(E_NOINTERFACE)
        CASE_SCODE(E_NOTIMPL)
        CASE_SCODE(E_OUTOFMEMORY)
        CASE_SCODE(E_PENDING)
        CASE_SCODE(E_POINTER)
        CASE_SCODE(E_UNEXPECTED)

        /* SCODEs from all files in alphabetical order */
        CASE_SCODE(CACHE_E_NOCACHE_UPDATED)
        CASE_SCODE(CACHE_S_FORMATETC_NOTSUPPORTED)
        CASE_SCODE(CACHE_S_SAMECACHE)
        CASE_SCODE(CACHE_S_SOMECACHES_NOTUPDATED)
        CASE_SCODE(CLASS_E_CLASSNOTAVAILABLE)
        CASE_SCODE(CLASS_E_NOAGGREGATION)
        CASE_SCODE(CLASS_E_NOTLICENSED)
        CASE_SCODE(CLIPBRD_E_BAD_DATA)
        CASE_SCODE(CLIPBRD_E_CANT_CLOSE)
        CASE_SCODE(CLIPBRD_E_CANT_EMPTY)
        CASE_SCODE(CLIPBRD_E_CANT_OPEN)
        CASE_SCODE(CLIPBRD_E_CANT_SET)
        CASE_SCODE2(CONNECT_E_ADVISELIMIT,SELFREG_E_CLASS)
        CASE_SCODE(CONNECT_E_CANNOTCONNECT)
        CASE_SCODE2(CONNECT_E_NOCONNECTION,PERPROP_E_NOPAGEAVAILABLE/SELFREG_E_TYPELIB)
        CASE_SCODE(CONNECT_E_OVERRIDDEN)
        CASE_SCODE(CONVERT10_E_OLESTREAM_BITMAP_TO_DIB)
        CASE_SCODE(CONVERT10_E_OLESTREAM_FMT)
        CASE_SCODE(CONVERT10_E_OLESTREAM_GET)
        CASE_SCODE(CONVERT10_E_OLESTREAM_PUT)
        CASE_SCODE(CONVERT10_E_STG_DIB_TO_BITMAP)
        CASE_SCODE(CONVERT10_E_STG_FMT)
        CASE_SCODE(CONVERT10_E_STG_NO_STD_STREAM)
        CASE_SCODE(CONVERT10_S_NO_PRESENTATION)
        CASE_SCODE(CO_E_ALREADYINITIALIZED)
        CASE_SCODE(CO_E_APPDIDNTREG)
        CASE_SCODE(CO_E_APPNOTFOUND)
        CASE_SCODE(CO_E_APPSINGLEUSE)
        CASE_SCODE(CO_E_CANTDETERMINECLASS)
        CASE_SCODE(CO_E_CLASSSTRING)
        CASE_SCODE(CO_E_DLLNOTFOUND)
        CASE_SCODE(CO_E_ERRORINAPP)
        CASE_SCODE(CO_E_ERRORINDLL)
        CASE_SCODE(CO_E_IIDSTRING)
        CASE_SCODE(CO_E_NOTINITIALIZED)
        CASE_SCODE(CO_E_OBJISREG)
        CASE_SCODE(CO_E_OBJNOTCONNECTED)
        CASE_SCODE(CO_E_OBJNOTREG)
#ifndef WIN16
        CASE_SCODE(CO_E_BAD_PATH)
        CASE_SCODE(CO_E_BAD_SERVER_NAME)
        CASE_SCODE(CO_E_CANT_REMOTE)
        CASE_SCODE(CO_E_CLASS_CREATE_FAILED)
        CASE_SCODE(CO_E_CLSREG_INCONSISTENT)
        CASE_SCODE(CO_E_CREATEPROCESS_FAILURE)
        CASE_SCODE(CO_E_IIDREG_INCONSISTENT)
        CASE_SCODE(CO_E_INIT_CLASS_CACHE)
        CASE_SCODE(CO_E_INIT_MEMORY_ALLOCATOR)
        CASE_SCODE(CO_E_INIT_ONLY_SINGLE_THREADED)
        CASE_SCODE(CO_E_INIT_RPC_CHANNEL)
        CASE_SCODE(CO_E_INIT_SCM_EXEC_FAILURE)
        CASE_SCODE(CO_E_INIT_SCM_FILE_MAPPING_EXISTS)
        CASE_SCODE(CO_E_INIT_SCM_MAP_VIEW_OF_FILE)
        CASE_SCODE(CO_E_INIT_SCM_MUTEX_EXISTS)
        CASE_SCODE(CO_E_INIT_SHARED_ALLOCATOR)
        CASE_SCODE(CO_E_INIT_TLS)
        CASE_SCODE(CO_E_INIT_TLS_CHANNEL_CONTROL)
        CASE_SCODE(CO_E_INIT_TLS_SET_CHANNEL_CONTROL)
        CASE_SCODE(CO_E_INIT_UNACCEPTED_USER_ALLOCATOR)
        CASE_SCODE(CO_E_LAUNCH_PERMSSION_DENIED)
        CASE_SCODE(CO_E_OBJSRV_RPC_FAILURE)
        CASE_SCODE(CO_E_OLE1DDE_DISABLED)
        CASE_SCODE(CO_E_RELEASED)
        CASE_SCODE(CO_E_REMOTE_COMMUNICATION_FAILURE)
        CASE_SCODE(CO_E_RUNAS_CREATEPROCESS_FAILURE)
//        CASE_SCODE(CO_E_RUNAS_INCOMPATIBLE)
        CASE_SCODE(CO_E_RUNAS_LOGON_FAILURE)
        CASE_SCODE(CO_E_RUNAS_SYNTAX)
        CASE_SCODE(CO_E_SCM_ERROR)
        CASE_SCODE(CO_E_SCM_RPC_FAILURE)
        CASE_SCODE(CO_E_SERVER_EXEC_FAILURE)
        CASE_SCODE(CO_E_SERVER_START_TIMEOUT)
        CASE_SCODE(CO_E_SERVER_STOPPING)
        CASE_SCODE(CO_E_START_SERVICE_FAILURE)
        CASE_SCODE(CO_E_WRONGOSFORAPP)
        CASE_SCODE(CO_S_NOTALLINTERFACES)
#endif //!WIN16
        CASE_SCODE(CTL_E_BADFILEMODE)
        CASE_SCODE(CTL_E_BADFILENAME)
        CASE_SCODE(CTL_E_BADFILENAMEORNUMBER)
        CASE_SCODE(CTL_E_BADRECORDLENGTH)
        CASE_SCODE(CTL_E_BADRECORDNUMBER)
        CASE_SCODE(CTL_E_CANTSAVEFILETOTEMP)
        CASE_SCODE(CTL_E_DEVICEIOERROR)
        CASE_SCODE(CTL_E_DEVICEUNAVAILABLE)
        CASE_SCODE(CTL_E_DISKFULL)
        CASE_SCODE(CTL_E_DISKNOTREADY)
        CASE_SCODE(CTL_E_DIVISIONBYZERO)
        CASE_SCODE(CTL_E_FILEALREADYEXISTS)
        CASE_SCODE(CTL_E_FILEALREADYOPEN)
        CASE_SCODE(CTL_E_FILENOTFOUND)
        CASE_SCODE(CTL_E_GETNOTSUPPORTED)
        CASE_SCODE(CTL_E_GETNOTSUPPORTEDATRUNTIME)
        CASE_SCODE(CTL_E_ILLEGALFUNCTIONCALL)
        CASE_SCODE(CTL_E_INVALIDCLIPBOARDFORMAT)
        CASE_SCODE(CTL_E_INVALIDFILEFORMAT)
        CASE_SCODE(CTL_E_INVALIDPATTERNSTRING)
        CASE_SCODE(CTL_E_INVALIDPICTURE)
        CASE_SCODE(CTL_E_INVALIDPROPERTYARRAYINDEX)
        CASE_SCODE(CTL_E_INVALIDPROPERTYVALUE)
        CASE_SCODE(CTL_E_INVALIDUSEOFNULL)
        CASE_SCODE(CTL_E_NEEDPROPERTYARRAYINDEX)
        CASE_SCODE(CTL_E_OUTOFMEMORY)
        CASE_SCODE(CTL_E_OUTOFSTACKSPACE)
        CASE_SCODE(CTL_E_OUTOFSTRINGSPACE)
        CASE_SCODE(CTL_E_OVERFLOW)
        CASE_SCODE(CTL_E_PATHFILEACCESSERROR)
        CASE_SCODE(CTL_E_PATHNOTFOUND)
        CASE_SCODE(CTL_E_PERMISSIONDENIED)
        CASE_SCODE(CTL_E_PRINTERERROR)
        CASE_SCODE(CTL_E_PROPERTYNOTFOUND)
        CASE_SCODE(CTL_E_REPLACEMENTSTOOLONG)
        CASE_SCODE(CTL_E_SEARCHTEXTNOTFOUND)
        CASE_SCODE(CTL_E_SETNOTPERMITTED)
        CASE_SCODE(CTL_E_SETNOTSUPPORTED)
        CASE_SCODE(CTL_E_SETNOTSUPPORTEDATRUNTIME)
        CASE_SCODE(CTL_E_TOOMANYFILES)
        CASE_SCODE(DATA_S_SAMEFORMATETC)
        CASE_SCODE(DISP_E_ARRAYISLOCKED)
        CASE_SCODE(DISP_E_BADCALLEE)
        CASE_SCODE(DISP_E_BADINDEX)
        CASE_SCODE(DISP_E_BADPARAMCOUNT)
        CASE_SCODE(DISP_E_BADVARTYPE)
        CASE_SCODE(DISP_E_EXCEPTION)
        CASE_SCODE(DISP_E_MEMBERNOTFOUND)
        CASE_SCODE(DISP_E_NONAMEDARGS)
        CASE_SCODE(DISP_E_NOTACOLLECTION)
        CASE_SCODE(DISP_E_OVERFLOW)
        CASE_SCODE(DISP_E_PARAMNOTFOUND)
        CASE_SCODE(DISP_E_PARAMNOTOPTIONAL)
        CASE_SCODE(DISP_E_TYPEMISMATCH)
        CASE_SCODE(DISP_E_UNKNOWNINTERFACE)
        CASE_SCODE(DISP_E_UNKNOWNLCID)
        CASE_SCODE(DISP_E_UNKNOWNNAME)
//        CASE_SCODE(DRAGDROP_E_ALREADYREGISTERED)      // same as OLECMDERR_E_DISABLED
//        CASE_SCODE(DRAGDROP_E_INVALIDHWND)            // same as OLECMDERR_E_NOHELP
//        CASE_SCODE(DRAGDROP_E_NOTREGISTERED)          // same as OLECMDERR_E_NOTSUPPORTED
        CASE_SCODE(DRAGDROP_S_CANCEL)
        CASE_SCODE2(DRAGDROP_S_DROP,HLINK_S_DONTHIDE)
        CASE_SCODE(DRAGDROP_S_USEDEFAULTCURSORS)
        CASE_SCODE(DV_E_CLIPFORMAT)
        CASE_SCODE(DV_E_DVASPECT)
        CASE_SCODE(DV_E_DVTARGETDEVICE)
        CASE_SCODE(DV_E_DVTARGETDEVICE_SIZE)
        CASE_SCODE(DV_E_FORMATETC)
        CASE_SCODE(DV_E_LINDEX)
        CASE_SCODE(DV_E_NOIVIEWOBJECT)
        CASE_SCODE(DV_E_STATDATA)
        CASE_SCODE(DV_E_STGMEDIUM)
        CASE_SCODE(DV_E_TYMED)
//        CASE_SCODE(HLINK_S_DONTHIDE)                  // same as DRAGDROP_S_DROP
        CASE_SCODE(INET_E_AUTHENTICATION_REQUIRED)
        CASE_SCODE(INET_E_CANNOT_CONNECT)
        CASE_SCODE(INET_E_CANNOT_INSTANTIATE_OBJECT)
        CASE_SCODE(INET_E_CANNOT_LOAD_DATA)
        CASE_SCODE(INET_E_CONNECTION_TIMEOUT)
        CASE_SCODE(INET_E_DATA_NOT_AVAILABLE)
        CASE_SCODE(INET_E_DOWNLOAD_FAILURE)
        CASE_SCODE(INET_E_INVALID_REQUEST)
        CASE_SCODE(INET_E_INVALID_URL)
        CASE_SCODE(INET_E_NO_SESSION)
        CASE_SCODE(INET_E_NO_VALID_MEDIA)
        CASE_SCODE(INET_E_OBJECT_NOT_FOUND)
        CASE_SCODE(INET_E_RESOURCE_NOT_FOUND)
        CASE_SCODE(INET_E_SECURITY_PROBLEM)
        CASE_SCODE(INET_E_UNKNOWN_PROTOCOL)
        CASE_SCODE(INPLACE_E_NOTOOLSPACE)
        CASE_SCODE(INPLACE_E_NOTUNDOABLE)
        CASE_SCODE(INPLACE_S_TRUNCATED)
#ifndef WIN16
        CASE_SCODE(MAPI_E_AMBIGUOUS_RECIP)
//        CASE_SCODE(MAPI_E_BAD_CHARWIDTH)
        CASE_SCODE(MAPI_E_BAD_COLUMN)
        CASE_SCODE(MAPI_E_BAD_VALUE)
        CASE_SCODE(MAPI_E_BUSY)
//        CASE_SCODE(MAPI_E_CALL_FAILED)
        CASE_SCODE(MAPI_E_CANCEL)
        CASE_SCODE(MAPI_E_COLLISION)
        CASE_SCODE(MAPI_E_COMPUTED)
        CASE_SCODE(MAPI_E_CORRUPT_DATA)
        CASE_SCODE(MAPI_E_CORRUPT_STORE)
        CASE_SCODE(MAPI_E_DECLINE_COPY)
        CASE_SCODE(MAPI_E_DISK_ERROR)
//        CASE_SCODE(MAPI_E_END_OF_SESSION)
        CASE_SCODE(MAPI_E_EXTENDED_ERROR)
        CASE_SCODE(MAPI_E_FAILONEPROVIDER)
        CASE_SCODE(MAPI_E_FOLDER_CYCLE)
        CASE_SCODE(MAPI_E_HAS_FOLDERS)
        CASE_SCODE(MAPI_E_HAS_MESSAGES)
//        CASE_SCODE(MAPI_E_INTERFACE_NOT_SUPPORTED)
        CASE_SCODE(MAPI_E_INVALID_BOOKMARK)
        CASE_SCODE(MAPI_E_INVALID_ENTRYID)
        CASE_SCODE(MAPI_E_INVALID_OBJECT)
//        CASE_SCODE(MAPI_E_INVALID_PARAMETER)
        CASE_SCODE(MAPI_E_INVALID_TYPE)
//        CASE_SCODE(MAPI_E_LOGON_FAILED)
//        CASE_SCODE(MAPI_E_MISSING_REQUIRED_COLUMN)
        CASE_SCODE(MAPI_E_NETWORK_ERROR)
        CASE_SCODE(MAPI_E_NON_STANDARD)
        CASE_SCODE(MAPI_E_NOT_ENOUGH_DISK)
//        CASE_SCODE(MAPI_E_NOT_ENOUGH_MEMORY)
        CASE_SCODE(MAPI_E_NOT_ENOUGH_RESOURCES)
        CASE_SCODE(MAPI_E_NOT_FOUND)
        CASE_SCODE(MAPI_E_NOT_INITIALIZED)
        CASE_SCODE(MAPI_E_NOT_IN_QUEUE)
        CASE_SCODE(MAPI_E_NOT_ME)
//        CASE_SCODE(MAPI_E_NO_ACCESS)
        CASE_SCODE(MAPI_E_NO_RECIPIENTS)
//        CASE_SCODE(MAPI_E_NO_SUPPORT)
        CASE_SCODE(MAPI_E_NO_SUPPRESS)
        CASE_SCODE(MAPI_E_OBJECT_CHANGED)
        CASE_SCODE(MAPI_E_OBJECT_DELETED)
//        CASE_SCODE(MAPI_E_SESSION_LIMIT)
        CASE_SCODE(MAPI_E_STRING_TOO_LONG)
        CASE_SCODE(MAPI_E_SUBMITTED)
        CASE_SCODE(MAPI_E_TABLE_EMPTY)
        CASE_SCODE(MAPI_E_TABLE_TOO_BIG)
        CASE_SCODE(MAPI_E_TIMEOUT)
        CASE_SCODE(MAPI_E_TOO_BIG)
        CASE_SCODE(MAPI_E_TOO_COMPLEX)
        CASE_SCODE(MAPI_E_TYPE_NO_SUPPORT)
        CASE_SCODE(MAPI_E_UNABLE_TO_ABORT)
        CASE_SCODE(MAPI_E_UNABLE_TO_COMPLETE)
        CASE_SCODE(MAPI_E_UNCONFIGURED)
        CASE_SCODE(MAPI_E_UNEXPECTED_ID)
        CASE_SCODE(MAPI_E_UNEXPECTED_TYPE)
//        CASE_SCODE(MAPI_E_UNKNOWN_ENTRYID)
        CASE_SCODE(MAPI_E_UNKNOWN_FLAGS)
        CASE_SCODE(MAPI_E_USER_CANCEL)
//        CASE_SCODE(MAPI_E_VERSION)
        CASE_SCODE(MAPI_E_WAIT)
        CASE_SCODE(MAPI_W_APPROX_COUNT)
        CASE_SCODE(MAPI_W_CANCEL_MESSAGE)
        CASE_SCODE(MAPI_W_ERRORS_RETURNED)
        CASE_SCODE(MAPI_W_NO_SERVICE)
        CASE_SCODE(MAPI_W_PARTIAL_COMPLETION)
        CASE_SCODE(MAPI_W_POSITION_CHANGED)
        CASE_SCODE(MEM_E_INVALID_LINK)
        CASE_SCODE(MEM_E_INVALID_ROOT)
        CASE_SCODE(MEM_E_INVALID_SIZE)
        CASE_SCODE(MK_E_ENUMERATION_FAILED)
#endif //!WIN16
        CASE_SCODE(MK_E_CANTOPENFILE)
        CASE_SCODE(MK_E_CONNECTMANUALLY)
        CASE_SCODE(MK_E_EXCEEDEDDEADLINE)
        CASE_SCODE(MK_E_INTERMEDIATEINTERFACENOTSUPPORTED)
        CASE_SCODE(MK_E_INVALIDEXTENSION)
        CASE_SCODE(MK_E_MUSTBOTHERUSER)
        CASE_SCODE(MK_E_NEEDGENERIC)
        CASE_SCODE(MK_E_NOINVERSE)
        CASE_SCODE(MK_E_NOOBJECT)
        CASE_SCODE(MK_E_NOPREFIX)
        CASE_SCODE(MK_E_NOSTORAGE)
        CASE_SCODE(MK_E_NOTBINDABLE)
        CASE_SCODE(MK_E_NOTBOUND)
        CASE_SCODE(MK_E_SYNTAX)
        CASE_SCODE(MK_E_UNAVAILABLE)
        CASE_SCODE(MK_S_HIM)
        CASE_SCODE(MK_S_ME)
        CASE_SCODE(MK_S_MONIKERALREADYREGISTERED)
        CASE_SCODE(MK_S_REDUCED_TO_SELF)
        CASE_SCODE(MK_S_US)
#ifndef WIN16
        CASE_SCODE(NTE_BAD_ALGID)
        CASE_SCODE(NTE_BAD_DATA)
        CASE_SCODE(NTE_BAD_FLAGS)
        CASE_SCODE(NTE_BAD_HASH)
        CASE_SCODE(NTE_BAD_HASH_STATE)
        CASE_SCODE(NTE_BAD_KEY)
        CASE_SCODE(NTE_BAD_KEYSET)
        CASE_SCODE(NTE_BAD_KEYSET_PARAM)
        CASE_SCODE(NTE_BAD_KEY_STATE)
        CASE_SCODE(NTE_BAD_LEN)
        CASE_SCODE(NTE_BAD_PROVIDER)
        CASE_SCODE(NTE_BAD_PROV_TYPE)
        CASE_SCODE(NTE_BAD_PUBLIC_KEY)
        CASE_SCODE(NTE_BAD_SIGNATURE)
        CASE_SCODE(NTE_BAD_TYPE)
        CASE_SCODE(NTE_BAD_UID)
        CASE_SCODE(NTE_BAD_VER)
        CASE_SCODE(NTE_DOUBLE_ENCRYPT)
        CASE_SCODE(NTE_EXISTS)
        CASE_SCODE(NTE_FAIL)
        CASE_SCODE(NTE_KEYSET_ENTRY_BAD)
        CASE_SCODE(NTE_KEYSET_NOT_DEF)
        CASE_SCODE(NTE_NOT_FOUND)
        CASE_SCODE(NTE_NO_KEY)
        CASE_SCODE(NTE_NO_MEMORY)
//        CASE_SCODE(NTE_OP_OK)                         // same as S_OK
        CASE_SCODE(NTE_PERM)
        CASE_SCODE(NTE_PROVIDER_DLL_FAIL)
        CASE_SCODE(NTE_PROV_DLL_NOT_FOUND)
        CASE_SCODE(NTE_PROV_TYPE_ENTRY_BAD)
        CASE_SCODE(NTE_PROV_TYPE_NOT_DEF)
        CASE_SCODE(NTE_PROV_TYPE_NO_MATCH)
        CASE_SCODE(NTE_SIGNATURE_FILE_BAD)
        CASE_SCODE(NTE_SYS_ERR)
#endif //!WIN16
        CASE_SCODE(OLECMDERR_E_CANCELED)
        CASE_SCODE2(OLECMDERR_E_DISABLED,DRAGDROP_E_ALREADYREGISTERED)
        CASE_SCODE2(OLECMDERR_E_NOHELP,DRAGDROP_E_INVALIDHWND)
        CASE_SCODE2(OLECMDERR_E_NOTSUPPORTED,DRAGDROP_E_NOTREGISTERED)
        CASE_SCODE(OLECMDERR_E_UNKNOWNGROUP)
        CASE_SCODE(OLEOBJ_E_INVALIDVERB)
        CASE_SCODE(OLEOBJ_E_NOVERBS)
        CASE_SCODE(OLEOBJ_S_CANNOT_DOVERB_NOW)
        CASE_SCODE(OLEOBJ_S_INVALIDHWND)
        CASE_SCODE(OLEOBJ_S_INVALIDVERB)
        CASE_SCODE(OLE_E_ADVF)
        CASE_SCODE(OLE_E_ADVISENOTSUPPORTED)
        CASE_SCODE(OLE_E_BLANK)
        CASE_SCODE(OLE_E_CANTCONVERT)
        CASE_SCODE(OLE_E_CANT_BINDTOSOURCE)
        CASE_SCODE(OLE_E_CANT_GETMONIKER)
        CASE_SCODE(OLE_E_CLASSDIFF)
        CASE_SCODE(OLE_E_ENUM_NOMORE)
        CASE_SCODE(OLE_E_INVALIDHWND)
        CASE_SCODE(OLE_E_INVALIDRECT)
        CASE_SCODE(OLE_E_NOCACHE)
        CASE_SCODE(OLE_E_NOCONNECTION)
        CASE_SCODE(OLE_E_NOSTORAGE)
        CASE_SCODE(OLE_E_NOTRUNNING)
        CASE_SCODE(OLE_E_NOT_INPLACEACTIVE)
        CASE_SCODE(OLE_E_OLEVERB)
        CASE_SCODE(OLE_E_PROMPTSAVECANCELLED)
        CASE_SCODE(OLE_E_STATIC)
        CASE_SCODE(OLE_E_WRONGCOMPOBJ)
        CASE_SCODE(OLE_S_MAC_CLIPFORMAT)
        CASE_SCODE(OLE_S_STATIC)
        CASE_SCODE(OLE_S_USEREG)
//        CASE_SCODE(PERPROP_E_NOPAGEAVAILABLE)         // same as CONNECT_E_NOCONNECTION
        CASE_SCODE(REGDB_E_CLASSNOTREG)
        CASE_SCODE(REGDB_E_IIDNOTREG)
        CASE_SCODE(REGDB_E_INVALIDVALUE)
        CASE_SCODE(REGDB_E_KEYMISSING)
        CASE_SCODE(REGDB_E_READREGDB)
        CASE_SCODE(REGDB_E_WRITEREGDB)
#ifndef WIN16
        CASE_SCODE(RPC_E_ACCESS_DENIED)
        CASE_SCODE(RPC_E_ATTEMPTED_MULTITHREAD)
        CASE_SCODE(RPC_E_CALL_CANCELED)
        CASE_SCODE(RPC_E_CALL_COMPLETE)
        CASE_SCODE(RPC_E_CALL_REJECTED)
        CASE_SCODE(RPC_E_CANTCALLOUT_AGAIN)
        CASE_SCODE(RPC_E_CANTCALLOUT_INASYNCCALL)
        CASE_SCODE(RPC_E_CANTCALLOUT_INEXTERNALCALL)
        CASE_SCODE(RPC_E_CANTCALLOUT_ININPUTSYNCCALL)
        CASE_SCODE(RPC_E_CANTPOST_INSENDCALL)
        CASE_SCODE(RPC_E_CANTTRANSMIT_CALL)
        CASE_SCODE(RPC_E_CHANGED_MODE)
        CASE_SCODE(RPC_E_CLIENT_CANTMARSHAL_DATA)
        CASE_SCODE(RPC_E_CLIENT_CANTUNMARSHAL_DATA)
        CASE_SCODE(RPC_E_CLIENT_DIED)
        CASE_SCODE(RPC_E_CONNECTION_TERMINATED)
        CASE_SCODE(RPC_E_DISCONNECTED)
        CASE_SCODE(RPC_E_FAULT)
        CASE_SCODE(RPC_E_INVALIDMETHOD)
        CASE_SCODE(RPC_E_INVALID_CALLDATA)
        CASE_SCODE(RPC_E_INVALID_DATA)
        CASE_SCODE(RPC_E_INVALID_DATAPACKET)
        CASE_SCODE(RPC_E_INVALID_EXTENSION)
        CASE_SCODE(RPC_E_INVALID_HEADER)
        CASE_SCODE(RPC_E_INVALID_IPID)
        CASE_SCODE(RPC_E_INVALID_OBJECT)
        CASE_SCODE(RPC_E_INVALID_PARAMETER)
        CASE_SCODE(RPC_E_NOT_REGISTERED)
        CASE_SCODE(RPC_E_NO_GOOD_SECURITY_PACKAGES)
        CASE_SCODE(RPC_E_OUT_OF_RESOURCES)
        CASE_SCODE(RPC_E_REMOTE_DISABLED)
        CASE_SCODE(RPC_E_RETRY)
        CASE_SCODE(RPC_E_SERVERCALL_REJECTED)
        CASE_SCODE(RPC_E_SERVERCALL_RETRYLATER)
        CASE_SCODE(RPC_E_SERVERFAULT)
        CASE_SCODE(RPC_E_SERVER_CANTMARSHAL_DATA)
        CASE_SCODE(RPC_E_SERVER_CANTUNMARSHAL_DATA)
        CASE_SCODE(RPC_E_SERVER_DIED)
        CASE_SCODE(RPC_E_SERVER_DIED_DNE)
        CASE_SCODE(RPC_E_SYS_CALL_FAILED)
        CASE_SCODE(RPC_E_THREAD_NOT_INIT)
        CASE_SCODE(RPC_E_TOO_LATE)
        CASE_SCODE(RPC_E_UNEXPECTED)
        CASE_SCODE(RPC_E_UNSECURE_CALL)
        CASE_SCODE(RPC_E_VERSION_MISMATCH)
        CASE_SCODE(RPC_E_WRONG_THREAD)
        CASE_SCODE(RPC_S_CALLPENDING)
        CASE_SCODE(RPC_S_WAITONTIMER)
//        CASE_SCODE(SELFREG_E_CLASS)                   // same as CONNECT_E_ADVISELIMIT
//        CASE_SCODE(SELFREG_E_TYPELIB)                 // same as CONNECT_E_NOCONNECTION
        CASE_SCODE(STG_E_ABNORMALAPIEXIT)
        CASE_SCODE(STG_E_ACCESSDENIED)
        CASE_SCODE(STG_E_BADBASEADDRESS)
        CASE_SCODE(STG_E_CANTSAVE)
        CASE_SCODE(STG_E_DISKISWRITEPROTECTED)
        CASE_SCODE(STG_E_DOCFILECORRUPT)
        CASE_SCODE(STG_E_EXTANTMARSHALLINGS)
        CASE_SCODE(STG_E_FILEALREADYEXISTS)
        CASE_SCODE(STG_E_FILENOTFOUND)
        CASE_SCODE(STG_E_INCOMPLETE)
        CASE_SCODE(STG_E_INSUFFICIENTMEMORY)
        CASE_SCODE(STG_E_INUSE)
        CASE_SCODE(STG_E_INVALIDFLAG)
        CASE_SCODE(STG_E_INVALIDFUNCTION)
        CASE_SCODE(STG_E_INVALIDHANDLE)
        CASE_SCODE(STG_E_INVALIDHEADER)
        CASE_SCODE(STG_E_INVALIDNAME)
        CASE_SCODE(STG_E_INVALIDPARAMETER)
        CASE_SCODE(STG_E_INVALIDPOINTER)
        CASE_SCODE(STG_E_LOCKVIOLATION)
        CASE_SCODE(STG_E_MEDIUMFULL)
        CASE_SCODE(STG_E_NOMOREFILES)
        CASE_SCODE(STG_E_NOTCURRENT)
        CASE_SCODE(STG_E_NOTFILEBASEDSTORAGE)
        CASE_SCODE(STG_E_OLDDLL)
        CASE_SCODE(STG_E_OLDFORMAT)
        CASE_SCODE(STG_E_PATHNOTFOUND)
        CASE_SCODE(STG_E_PROPSETMISMATCHED)
        CASE_SCODE(STG_E_READFAULT)
        CASE_SCODE(STG_E_REVERTED)
        CASE_SCODE(STG_E_SEEKERROR)
        CASE_SCODE(STG_E_SHAREREQUIRED)
        CASE_SCODE(STG_E_SHAREVIOLATION)
        CASE_SCODE(STG_E_TERMINATED)
        CASE_SCODE(STG_E_TOOMANYOPENFILES)
        CASE_SCODE(STG_E_UNIMPLEMENTEDFUNCTION)
        CASE_SCODE(STG_E_UNKNOWN)
        CASE_SCODE(STG_E_WRITEFAULT)
        CASE_SCODE(STG_S_BLOCK)
        CASE_SCODE(STG_S_CONVERTED)
        CASE_SCODE(STG_S_MONITORING)
        CASE_SCODE(STG_S_RETRYNOW)
        CASE_SCODE(TRUST_E_ACTION_UNKNOWN)
        CASE_SCODE(TRUST_E_PROVIDER_UNKNOWN)
        CASE_SCODE(TRUST_E_SUBJECT_FORM_UNKNOWN)
        CASE_SCODE(TRUST_E_SUBJECT_NOT_TRUSTED)
#endif //!WIN16
        CASE_SCODE(TYPE_E_AMBIGUOUSNAME)
        CASE_SCODE(TYPE_E_BADMODULEKIND)
        CASE_SCODE(TYPE_E_BUFFERTOOSMALL)
        CASE_SCODE(TYPE_E_CANTCREATETMPFILE)
        CASE_SCODE(TYPE_E_CANTLOADLIBRARY)
        CASE_SCODE(TYPE_E_CIRCULARTYPE)
        CASE_SCODE(TYPE_E_DLLFUNCTIONNOTFOUND)
        CASE_SCODE(TYPE_E_DUPLICATEID)
        CASE_SCODE(TYPE_E_ELEMENTNOTFOUND)
        CASE_SCODE(TYPE_E_INCONSISTENTPROPFUNCS)
        CASE_SCODE(TYPE_E_INVALIDID)
        CASE_SCODE(TYPE_E_INVALIDSTATE)
        CASE_SCODE(TYPE_E_INVDATAREAD)
        CASE_SCODE(TYPE_E_IOERROR)
        CASE_SCODE(TYPE_E_LIBNOTREGISTERED)
        CASE_SCODE(TYPE_E_NAMECONFLICT)
        CASE_SCODE(TYPE_E_OUTOFBOUNDS)
        CASE_SCODE(TYPE_E_QUALIFIEDNAMEDISALLOWED)
        CASE_SCODE(TYPE_E_REGISTRYACCESS)
        CASE_SCODE(TYPE_E_SIZETOOBIG)
        CASE_SCODE(TYPE_E_TYPEMISMATCH)
        CASE_SCODE(TYPE_E_UNDEFINEDTYPE)
        CASE_SCODE(TYPE_E_UNKNOWNLCID)
        CASE_SCODE(TYPE_E_UNSUPFORMAT)
        CASE_SCODE(TYPE_E_WRONGTYPEKIND)
        CASE_SCODE(VIEW_E_DRAW)
        CASE_SCODE(VIEW_S_ALREADY_FROZEN)

        default:
            lpstr = _T("UNKNOWN SCODE");
    }

#undef CASE_SCODE
#undef CASE_SCODE2

    return lpstr;
}



//+---------------------------------------------------------------------------
//
//  Function:   hrvsnprintf
//
//  Synopsis:   Prints a string to a buffer, interpreting %hr as a
//              format string for an HRESULT.
//
//  Arguments:  [achBuf]    -- The buffer to print into.
//              [cchBuf]    -- The size of the buffer.
//              [pstrFmt]   -- The format string.
//              [valMarker] -- List of arguments to format string.
//
//  Returns:    Number of characters printed to the buffer not including
//              the terminating NULL.  In case of buffer overflow, returns
//              -1.
//
//  Modifies:   [achBuf]
//
//----------------------------------------------------------------------------

int
hrvsnprintf(char * achBuf, int cchBuf, const char * pstrFmt, va_list valMarker)
{
    static char achFmtHR[] = "<%ls (0x%lx)>";
    static char achHRID[] = "%hr";

    char            achFmt[1024];
    int             cch;
    int             cchTotal;
    char *          lpstr;
    char *          lpstrLast;
    int             cFormat;
    HRESULT         hrVA;

    // We stomp on the string, so copy to writeable memory.

    strcpy(achFmt, pstrFmt);

    //
    // Scan for %hr tokens.  If found, print the corresponding
    // hresult into the buffer.
    //

    cch = 0;
    cchTotal = 0;
    cFormat = 0;
    lpstrLast = achFmt;
    lpstr = achFmt;
    while (*lpstr)
    {
        if (*lpstr != '%')
        {
            lpstr++;
        }
        else if (lpstr[1] == '%')
        {
            lpstr += 2;
        }
        else if (strncmp(lpstr, achHRID, ARRAY_SIZE(achHRID) - 1))
        {
            cFormat++;
            lpstr++;
        }
        else
        {
            //
            // Print format string up to the hresult.
            //

            * lpstr = 0;
            cch = _vsnprintf(
                    achBuf + cchTotal,
                    cchBuf - cchTotal,
                    lpstrLast,
                    valMarker);
            * lpstr = '%';
            if (cch == -1)
                break;

            cchTotal += cch;

            //
            // Advance valMarker for each printed format.
            //

            while (cFormat-- > 0)
            {
                //
                // BUGBUG (adams): Won't work for floats, as their stack size
                // is not four bytes.
                //

                va_arg(valMarker, void *);
            }

            //
            // Print hresult into buffer.
            //

            hrVA = va_arg(valMarker, HRESULT);
            cch = _snprintf(
                    achBuf + cchTotal,
                    cchBuf - cchTotal,
                    achFmtHR,
                    GetHResultName(hrVA),
                    hrVA);
            if (cch == -1)
                break;

            cchTotal += cch;
            lpstr += ARRAY_SIZE(achHRID) - 1;
            lpstrLast = lpstr;
        }
    }

    if (cch != -1)
    {
        cch = _vsnprintf(
                achBuf + cchTotal,
                cchBuf - cchTotal,
                lpstrLast,
                valMarker);
    }

    return (cch == -1) ? -1 : cchTotal + cch;
}


// This function uses ANSII strings so I use C RTL functions for string manipulation
// instead of the _t... fucntions
void WINAPI
DbgExOpenLogFile(LPCSTR szFName)
{
    char    rgch[MAX_PATH];

#ifdef _MAC
    DWORD dwCt = 0x74747874;  // 'ttxt'
    DWORD dwFtype = 0x54455854; // 'TEXT'
    DWORD dwOldct, dwOldftype;

    dwOldct = SetDefaultCreatorType(dwCt);
    dwOldftype = SetDefaultFileType(dwFtype);
#endif

    if(szFName == NULL || strlen(szFName) == 0)
    {
        // log file name or path is not specified, we must use the default ones
        if (g_hinstMain)
        {
            // Get program path and file name and replace the extension with the
            // log file extension
#ifndef _MAC
            UINT    cch = (UINT) GetModuleFileNameA(g_hModule, rgch, sizeof(rgch));
#ifdef UNIX
            strcat(rgch, szDbgOutFileExt);
#else
            Assert(rgch[cch - 4] == '.');
            strcpy(&rgch[cch - 4], szDbgOutFileExt);
#endif
#else
            CHAR   achAppLoc[MAX_PATH];
            DWORD   dwRet;
            short   iRet;

            dwRet = GetModuleFileNameA(NULL, achAppLoc, ARRAY_SIZE(achAppLoc));
            Assert (dwRet != 0);

            iRet = GetFileTitleA(achAppLoc,rgch,sizeof(rgch));
            Assert(iRet == 0);

            strcat (rgch, szDbgOutFileExt);
#endif
        }
        else
        {
            // Use the dewfault file name in default directory
            strcpy(rgch, szDbgOutFileName);
        }

    }
    else
    {

        // check, and if file name is not specified in the command line append the
        // default log file name to the specified path
        int nLen = strlen(szFName);

        strncpy(rgch, szFName, sizeof(rgch) / sizeof(rgch[0]) - 1);
        rgch[sizeof(rgch) / sizeof(rgch[0]) - 1] = 0;

        if(szFName[nLen - 1] == '\\')
        {
            strcat(rgch, szDbgOutFileName);
        }

    }

    s_hFileLog = CreateFileA(rgch, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
        0, (HANDLE) NULL);

    if (s_hFileLog != INVALID_HANDLE_VALUE)
    {
        wsprintfA(rgch, ">>>>> Start logging" SZ_NEWLINE);
        SetFilePointer(s_hFileLog, 0, NULL, FILE_END);
        SpitSzToDisk(rgch);
    }
#ifndef WIN16
    else
    {
        char msg[200];
        _snprintf(msg, sizeof(msg) / sizeof(msg[0]), "Cannot create the log file \"%s\"" SZ_NEWLINE, rgch);
        MessageBoxA(NULL, msg, "Error", MB_OK);
    }
#endif

#ifdef _MAC
        SetDefaultCreatorType(dwOldct);
        SetDefaultFileType(dwOldftype);
#endif
}

DWORD WINAPI
DbgExGetVersion()
{
    return(g_fDetached ? 0 : MSHTMDBG_API_VERSION);
}

BOOL WINAPI
DbgExIsFullDebug()
{
    return(TRUE);
}

BOOL WINAPI
DbgExGetChkStkFill(DWORD * pdwFill)
{
    *pdwFill = GetPrivateProfileIntA("chkstk", "fill", 0xCCCCCCCC, "mshtmdbg.ini");
    return(DbgExIsTagEnabled(tagStackSpew));
}

void WINAPI
DbgExSetDllMain(HANDLE hDllHandle, BOOL (WINAPI * pfnDllMain)(HANDLE, DWORD, LPVOID))
{
    int i;

    for (i = 0; i < ARRAY_SIZE(g_rgDllHandle); ++i)
    {
        if (pfnDllMain == NULL)
        {
            if (g_rgDllHandle[i] == hDllHandle)
            {
                g_rgDllHandle[i] = NULL;
                g_rgDllMain[i] = NULL;
                break;
            }
        }
        else
        {
            if (g_rgDllHandle[i] == NULL)
            {
                g_rgDllHandle[i] = hDllHandle;
                g_rgDllMain[i] = pfnDllMain;
                break;
            }
        }
    }
}
