//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       libmain.cxx
//
//  Contents:   DllMain for Forms3
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_URLMON_H_
#define X_URLMON_H_
#include "urlmon.h"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

#if DBG == 1
// used for assert to fool the compiler
DWORD g_dwFALSE = 0;
#endif

//  WARNING -- Do not put any global objects with constructors or destructors
//  in this file due to the presence of the init_seg directive below.
//

//+------------------------------------------------------------------------
//
//  External intialization functions
//
//-------------------------------------------------------------------------

// Process attach/detach routines
#ifdef WIN16
// InitUnicodeWrappers initializes this, so when that
// gets fixed get rid of this line.
DWORD g_dwPlatformID = VER_PLATFORM_WIN16;
void InitHeap();
void DeinitHeap();
#else
extern void     InitUnicodeWrappers();
#endif // !WIN16

#ifdef PRODUCT_96
extern HRESULT  InitDynamicVtable();
extern void     DeinitDynamicVtable();
#endif
extern void     DeinitTearOffCache();
extern void     DeinitWindowClasses();
extern void     InitFormClipFormats();
extern void     DeinitForm();
extern void     InitColorTranslation();
extern void     ClearFaceCache();
extern void     InitDefaultControlSizes();
extern HRESULT  InitPalette();
extern BOOL     InitImageUtil();
extern HRESULT  InitICM();
extern HRESULT  InitUrlCompatTable();
extern void     DeinitPalette();
extern void     DeinitSurface();

#ifdef _MAC
extern void     InitMacDrawing();
extern void     DeinitMacDrawing();
extern "C" pascal void TermRoutine(void);
#endif

extern void     InitFontCache();
extern void     DeinitTextSubSystem();
extern void     DeinitDownload();
extern void     KillDwnTaskExec();
extern void     KillImgTaskExec();
extern void     DeinitDynamicLibraries();
extern void     DeinitImageSizeCache();

// Thread attach/passivate/detach routines
typedef HRESULT (*PFN_HRESULT_INIT)(THREADSTATE * pts);
typedef void    (*PFN_VOID_DEINIT)(THREADSTATE * pts);

extern HRESULT InitScrollbar(THREADSTATE * pts);
extern void    DeinitScrollbar(THREADSTATE * pts);
extern void    LSDeinitUnderlinePens(THREADSTATE * pts);
extern HRESULT InitBrushCache(THREADSTATE * pts);
extern void    DeinitBrushCache(THREADSTATE * pts);
extern HRESULT InitBmpBrushCache(THREADSTATE * pts);
extern void    DeinitBmpBrushCache(THREADSTATE * pts);
extern HRESULT InitGlobalWindow(THREADSTATE * pts);
extern void    DeinitGlobalWindow(THREADSTATE * pts);
extern HRESULT InitSystemMetricValues(THREADSTATE * pts);
extern void    DeinitSystemMetricValues(THREADSTATE * pts);
extern void    DeinitTimerCtx(THREADSTATE * pts);
extern void    DeinitTooltip(THREADSTATE * pts);
extern void    DeinitTypeLibCache(THREADSTATE * pts);
extern void    DeinitCommitHolder(THREADSTATE *pts);
extern HRESULT InitTaskManager(THREADSTATE * pts);
extern void    DeinitTaskManager(THREADSTATE * pts);
extern void    DeinitFormatCache(THREADSTATE * pts);
extern void    DeinitSpooler(THREADSTATE * pts);
extern void    DeinitPostMan(THREADSTATE * pts);

#ifndef NO_IME
extern void    DeinitMultiLanguage();
extern void    DeinitUniscribe();
#endif // NO_IME

#ifndef WIN16
extern void    DeinitScriptDebugging();
#endif //!WIN16
extern void    DeinitOptionSettings(THREADSTATE *pts);
extern void    DeinitUserAgentString(THREADSTATE *pts);
extern void    OnSettingsChangeAllDocs(BOOL fNeedLayout);
extern void    DeinitWindowInfo(THREADSTATE *pts);
#ifndef NO_MENU
extern void    DeinitContextMenus(THREADSTATE *pts);
#endif // NO_MENU
extern void    DeinitUserStyleSheets(THREADSTATE *pts);
extern void    DeinitImgAnim(THREADSTATE *pts);
extern void    DeinitCategoryInfo(THREADSTATE *pts);
#ifndef NO_IME
extern void    DeinitDIMM();
#endif // ndef NO_IME
#ifdef QUILL
extern void    DeinitQuillLayout();
#endif

extern void    DeinitFontLinking(THREADSTATE * pts);
extern class   CResProtocolCF g_cfResProtocol;
extern class   CAboutProtocolCF g_cfAboutProtocol;
extern class   CViewSourceProtocolCF g_cfViewSourceProtocol;
extern void    DeinitHTMLDialogs();
extern void    InitClassTable();

extern void    DeinitGlobalVersions();

extern HRESULT InitLSCache(THREADSTATE * pts);
extern void    DeinitLSCache(THREADSTATE * pts);

#if DBG == 1
HINSTANCE g_hinstF3Debug = NULL;
#endif

#if defined(DEBUG_TEAROFFS) && !defined(WIN16)
void DebugCheckAllTearoffTables();
#endif

BOOL  SetDLLName(  void  );


//+---------------------------------------------------------------------------
//
//  Class:      CUnloadLibraries
//
//  Purpose:    Special class with a destructor that will be called after
//              all other static destructors are called. This ensures that
//              we don't free any of the libraries we loaded until we're
//              completely cleaned up. Win95 apparently likes to clean up
//              DLLs a little too aggressively when you call FreeLibrary
//              inside DLL_PROCESS_DETACH handlers.
//
//----------------------------------------------------------------------------
#pragma warning(disable:4073) // warning about the following init_seg statement
#pragma init_seg(lib)
class CUnloadLibraries
{
public:
    ~CUnloadLibraries();
};

#ifndef WIN16
// put into WEP for win16 (it doesn't get called otherwise for some reason.)
CUnloadLibraries g_CUnloadLibs;
#endif // ndef WIN16

//+---------------------------------------------------------------------------
//
//  Member:     CUnloadLibraries::~CUnloadLibraries, public
//
//  Synopsis:   class dtor
//
//  Notes:      The init_seg pragma ensures this dtor is called after all
//              others.
//
//----------------------------------------------------------------------------
CUnloadLibraries::~CUnloadLibraries()
{
    DeinitDynamicLibraries();

#if DBG == 1
    if (g_hinstF3Debug)
        FreeLibrary(g_hinstF3Debug);
#endif

}
#pragma warning(default:4073)


//+------------------------------------------------------------------------
//
//  Misc globals
//
//  WARNING -- Do not put any global objects with constructors or destructors
//  in this file due to the presence of the init_seg directive above.
//
//-------------------------------------------------------------------------

static const DWORD  TLS_NULL = ((DWORD)-1);                 // NULL TLS value (defined as 0xFFFFFFFF)

LONG g_lSecondaryObjCount = 0;

#if DBG==1
int g_lSecondaryObjCountCallers[15] = { 0 };
#endif

EXTERN_C HANDLE     g_hProcessHeap = NULL;
HINSTANCE           g_hInstCore = NULL;
HINSTANCE           g_hInstResource = NULL;
TCHAR               g_achDLLCore[MAX_PATH];                 //  Review: Could this be unified
#ifndef WIN16
TCHAR               g_achDLLResource[MAX_PATH];             //          with g_szCodeFragName of the Mac build?
#endif
TCHAR               g_achHelpFileName[MAX_PATH];            // Help file name
DWORD               g_dwTls = TLS_NULL;                     // TLS index associated with THREADSTATEs
THREADSTATE *       g_pts = NULL;                           // Head of THREADSTATE chain

ULONG               g_ulLcl;

#if !defined(WIN16)
CRITICAL_SECTION    CGlobalLock::s_cs;                      // Critical section to protect globals
#if DBG==1
DWORD               CGlobalLock::s_dwThreadID = 0;          // Thread ID which owns the critical section
LONG                CGlobalLock::s_cNesting = 0;            // Enter/LeaveCriticalSection nesting level (DEBUG only)
#endif
#endif

HANDLE              g_hMapHtmPerfCtl = NULL;                // Perf control block memory map handle
HTMPERFCTL *        g_pHtmPerfCtl = NULL;                   // Perf control block (typically NULL)

HRESULT DllThreadAttach();
void    DllThreadDetach(THREADSTATE * pts);
void    DllThreadPassivate(THREADSTATE * pts);
BOOL    DllProcessAttach();
void    DllProcessDetach();


#ifdef WIN16
//+---------------------------------------------------------------------------
//
//  Function:   (De)InitWin16TaskGlobals
//
//  Synopsis:   (De)Inits the pTaskGlobals member of the thread state.
//              Called at threadattach time.
//              Some variables are stored for each HTASK; we should
//              create separate threads for each HTASK; we'll store the
//              HTASK information in the threadstate.
//
//              These functions could be moved to another file.
//
//----------------------------------------------------------------------------

HRESULT InitWin16TaskGlobals(THREADSTATE * pts)
{
    THREADSTATE * pts2;
    HTASK hTask = GetCurrentTask();
    LOCK_GLOBALS;

    // search every pts for an appropriate task global.
    // Expensive but happens infrequently.
    for (pts2 = g_pts;
         pts2 && (!pts2->pTaskGlobals || pts2->pTaskGlobals->hTask != hTask);
         pts2 = pts2->ptsNext);
    if (pts2)
    {
        pts->pTaskGlobals = pts2->pTaskGlobals;
    }
    else
    {
        // create a new set of task globals.
        pts->pTaskGlobals = (WIN16_TASK_GLOBALS *) MemAllocClear(sizeof(WIN16_TASK_GLOBALS));
        if (!pts->pTaskGlobals)
        {
            return E_OUTOFMEMORY;
        }
        pts->pTaskGlobals->hTask = hTask;
    }
    pts->pTaskGlobals->cAttachedThreads++;

    return S_OK;
}

void    DeinitWin16TaskGlobals(THREADSTATE * pts)
{
    if (pts->pTaskGlobals)
    {
        pts->pTaskGlobals->cAttachedThreads--;
        if (!pts->pTaskGlobals->cAttachedThreads)
        {
            MemFree(pts->pTaskGlobals);
        }
    }
}

#endif




//+---------------------------------------------------------------------------
//
//  Function:   DllUpdateSettings
//
//  Synopsis:   Updated cached system settings.  Called when the DLL is
//              attached and in response to WM_WININICHANGE, WM_DEVMODECHANGE,
//              and so on.
//
//----------------------------------------------------------------------------
#ifdef _MAC
    //BUGBUG: ReinitStdTypes should be called regardless of fulldebug or not.
    //        The ReinitStdTypes needs to be in oa and always called.
#  ifdef _MAC_FULLDEBUG
void ReinitStdTypes(void);
#  endif
#endif

extern BOOL g_fSystemFontsNeedRefreshing;
extern BOOL g_fNoDisplayChange;

void
DllUpdateSettings(UINT msg)
{
    g_fSystemFontsNeedRefreshing = TRUE;

    if (msg == WM_SYSCOLORCHANGE || msg == WM_DISPLAYCHANGE)
    {
        g_fNoDisplayChange = (g_fNoDisplayChange && (msg != WM_DISPLAYCHANGE));

        // On syscolor change, we need to update color table
        InitColorTranslation();
        InitPalette();
        InitImageUtil();
    }

    // When the fonts available have changed, we need to
    // recheck the system for font faces.
    else if (msg == WM_FONTCHANGE)
    {
        ClearFaceCache();
    }

#ifdef _MAC
    //BUGBUG: ReinitStdTypes should be called regardless of fulldebug or not.
    //        The ReinitStdTypes needs to be in oa and always called.

    //      We need to recreate the hdcDesktop on a WM_SYSCOLORCHANGE
    //      Should this call be in direct response to that message or a general
    //      response to any display setting change?
#  ifdef _MAC_FULLDEBUG
    ReinitStdTypes();
#  endif
    DeinitSystemMetricValues(GetThreadState());
#endif
    InitSystemMetricValues(GetThreadState());

    OnSettingsChangeAllDocs(WM_SYSCOLORCHANGE == msg ||
                            WM_FONTCHANGE == msg ||
                            (WM_USER + 338) == msg);

}


//+---------------------------------------------------------------------------
//
//  Function:   DllThreadPassivate
//
//  Synopsis:   This function is called when the per-thread object count,
//              dll.lObjCount, transitions to zero.  Deinit things here that
//              cannot be handled at process detach time.
//
//              This function can be called zero, one, or more times during
//              the time the DLL is loaded. Every function called from here
//              here should be prepared to handle this.
//
//----------------------------------------------------------------------------
void
DllThreadPassivate(
    THREADSTATE *   pts)
{
    // Passivate per-thread objects
    // These include:
    //  a) Per-thread OLE error object (held by OLE automation)
    //  b) OLE clipboard object (only one thread should have anything on the clipboard)
    //  c) Per-thread ITypeInfo caches
    //  d) Per-thread picture helper
    //  e) Per-thread default IFont objects
    //
    // NOTE: This code contains one possible race condition: It is possible for
    //       the contents of the clipboard to be changed between the call to
    //       OleIsCurrentClipboard and OleFlushClipboard. If that occurs, this
    //       will flush the wrong object.
    //       GaryBu and I (BrendanD) discussed this case and felt it was not
    //       sufficiently important to warrent implementing a more complete
    //       (and costly) solution.
    Assert(pts);
    TraceTag((tagThread, "DllThreadPassivate called - TID 0x%08x", GetCurrentThreadId()));

    //
    // Tell ole to clear its error info object if one exists.  If we
    // did not load OLEAUT32 then one could not exist.
    //

#ifndef WIN16
    extern DYNLIB g_dynlibOLEAUT32;

    if (g_dynlibOLEAUT32.hinst)
    {
        Verify(OK(SetErrorInfo(NULL, NULL)));
    }
#endif // !WIN16

    if (pts->pDataClip && !OleIsCurrentClipboard(pts->pDataClip))
    {
        WHEN_DBG(HRESULT hr =) THR(OleFlushClipboard());
        #if DBG==1
        if (hr)
        {
             Assert(!pts->pDataClip && "Clipboard data should be flushed now");
        }
        #endif
    }

    DeinitTypeLibCache(pts);
    DeinitSpooler(pts);
    if (pts->pInetSess)
    {
        pts->pInetSess->UnregisterNameSpace((IClassFactory *) &g_cfViewSourceProtocol, _T("view-source"));
        pts->pInetSess->UnregisterNameSpace((IClassFactory *) &g_cfResProtocol, _T("res"));
        pts->pInetSess->UnregisterNameSpace((IClassFactory *) &g_cfAboutProtocol, _T("about"));

    }
    ClearInterface(&pts->pInetSess);
}

#ifndef WIN16
//+----------------------------------------------------------------------------
//
//  Function:   DllAllThreadsDetach
//
//  Synopsis:   Cleanup when all Trident threads go away.
//
//-----------------------------------------------------------------------------

void
DllAllThreadsDetach()
{
#ifndef NO_SCRIPT_DEBUGGER  //IEUNIX: UNIX macro.
    IF_NOT_WIN16(DeinitScriptDebugging());
#endif
    DeinitHTMLDialogs();
    DeinitSurface();
    KillDwnTaskExec();
    KillImgTaskExec();
#ifndef NO_IME
    DeinitMultiLanguage();
    DeinitDIMM();
    DeinitUniscribe();
#endif
    DeinitGlobalVersions();

    if (g_hInstResource)
    {
        MLFreeLibrary(g_hInstResource);
        g_hInstResource = NULL;
    }
}

#endif // ndef WIN16

//+----------------------------------------------------------------------------
//
//  Function:   DllThreadDetach
//
//  Synopsis:   Release/clean-up per-thread variables
//
//  NOTE:   Since DllThreadDetach is called when DllThreadAttach fails
//          all Deinitxxxx routines must be robust. They must work correctly
//          even if the corresponding Initxxxx routine was not first called.
//
//-----------------------------------------------------------------------------
void
DllThreadDetach(
    THREADSTATE *   pts)
{
    //  Deinitialization routines (called in order)
    static const PFN_VOID_DEINIT s_apfnDeinit[] =
                    {
                    DeinitOptionSettings,
                    DeinitUserAgentString,
                    DeinitPostMan,
                    DeinitTaskManager,
                    DeinitCommitHolder,
                    DeinitWindowInfo,
                    DeinitCategoryInfo,
                    DeinitTooltip,
                    LSDeinitUnderlinePens,
                    DeinitBrushCache,
                    DeinitBmpBrushCache,
                    DeinitScrollbar,
                    DeinitTimerCtx,
                    DeinitSystemMetricValues,
                    DeinitFormatCache,
#ifndef NO_MENU
                    DeinitContextMenus,
#endif // NO_MENU
                    DeinitUserStyleSheets,
                    DeinitImgAnim,
                    DeinitFontLinking,
#ifdef WIN16
                    DeinitWin16TaskGlobals,
#endif
                    DeinitLSCache,
                    DeinitGlobalWindow  // Must occur last
                    };
    THREADSTATE **  ppts;
    int             cDeinit;

    if (!pts)
        return;

    Assert(pts->dll.idThread == GetCurrentThreadId());

#ifdef OBJCNTCHK
#if DBG==1
    AssertSz(pts->dll.cLockServer == 0, "DllThreadDetach called with cLockServer count non-zero");
#else
    if (pts->dll.cLockServer != 0)
        F3DebugBreak();
#endif
#endif

#if DBG==1
    TraceTag((tagThread, "DllThreadDetach called - TID 0x%08x", GetCurrentThreadId()));
    if (pts->dll.lObjCount)
    {
        TraceTag((tagError,
                "Thread (TID=0x%08x) terminated with primary object count=%d",
                GetCurrentThreadId(), pts->dll.lObjCount));
    }
#endif

#ifdef _MAC
    // The MAC has only one thread, so no THREADSTATE is explicitly passed
    DeinitMacDrawing();
#endif

    // Deinitialize the per-thread variables
    for (cDeinit = 0; cDeinit < ARRAY_SIZE(s_apfnDeinit); cDeinit++)
    {
        (*s_apfnDeinit[cDeinit])(pts);
    }

    ClearErrorInfo(pts);

    // Remove the per-thread structure from the global list
    LOCK_GLOBALS;
    for (ppts = &g_pts; *ppts && *ppts != pts; ppts = &((*ppts)->ptsNext));
    if (*ppts)
    {
        *ppts = pts->ptsNext;
    }
#if DBG==1
    else
    {
        TraceTag((tagThread,
                "THREADSTATE not on global chain - TID=0x%08x",
                GetCurrentThreadId()));
    }
#endif

#ifdef QUILL
	DeinitQuillLayout();
#endif

    // Disconnect the memory from the thread and delete it
    TlsSetValue(g_dwTls, NULL);
    delete pts;

    return;
}

void
DllThreadEmergencyDetach()
{
    // This gets called by DLL_THREAD_DETACH.  We need to be careful to only do the
    // absolute minimum here to make sure we don't crash in the future.

    THREADSTATE * pts = (THREADSTATE *)TlsGetValue(g_dwTls);

    if (    pts
        &&  pts->dll.idThread == GetCurrentThreadId()
        &&  pts->gwnd.hwndGlobalWindow)
    {
        EnterCriticalSection(&pts->gwnd.cs);
        DestroyWindow(pts->gwnd.hwndGlobalWindow);
        pts->gwnd.hwndGlobalWindow = NULL;
        LeaveCriticalSection(&pts->gwnd.cs);
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   _AddRefThreadState
//
//  Synopsis:   Prepare per-thread variables
//
//  Returns:    S_OK, E_OUTOFMEMORY
//
//-----------------------------------------------------------------------------
HRESULT
#ifdef OBJCNTCHK
_AddRefThreadState(DWORD * pdwObjCnt)
#else
_AddRefThreadState()
#endif
{
#ifdef OBJCNTCHK
    if (pdwObjCnt)
    {
        *pdwObjCnt = 0;
    }
#endif

    extern HRESULT DllThreadAttach();
    extern DWORD g_dwTls;
    HRESULT hr;
    if (TlsGetValue(g_dwTls) == 0)
    {
        hr = DllThreadAttach();
        if (hr)
            RRETURN(hr);
    }

    IncrementObjectCount(pdwObjCnt);

    return(S_OK);
}

//+----------------------------------------------------------------------------
//
//  Function:   DllThreadAttach
//
//  Synopsis:   Prepare per-thread variables
//
//  Returns:    S_OK, E_OUTOFMEMORY
//
//-----------------------------------------------------------------------------
HRESULT
DllThreadAttach()
{
    //  Initialization routines (called in order)
    static const PFN_HRESULT_INIT s_apfnInit[] =
                    {
                    InitGlobalWindow,       // Must occur first
                    InitSystemMetricValues,
                    InitBrushCache,
                    InitBmpBrushCache,
                    InitScrollbar,
                    InitTaskManager,
#ifdef WIN16
                    InitWin16TaskGlobals,
#endif
                    InitLSCache,
                    };
    THREADSTATE *   pts;
    int             cInit;
    HRESULT         hr = S_OK;

    TraceTag((tagThread, "DllThreadAttach called - TID 0x%08x", GetCurrentThreadId()));

    // Allocate per-thread variables
    Assert(!TlsGetValue(g_dwTls));
    pts = new THREADSTATE;
    if (!pts || !TlsSetValue(g_dwTls, pts))
    {
        if (pts)
        {
            delete pts;
            pts = NULL;
        }
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    MemSetName((pts, "THREADSTATE %08x", GetCurrentThreadId()));

    // Initialize the structure
    pts->dll.idThread = GetCurrentThreadId();

    // Run thread initialization routines
    for (cInit = 0; cInit < ARRAY_SIZE(s_apfnInit); cInit++)
    {
        hr = THR((*s_apfnInit[cInit])(pts));
        if (FAILED(hr))
            goto Error;
    }
#ifdef QUILL
	pts->_pQLM = NULL;
	pts->fQuillInitFailed = FALSE;
#endif

#ifdef _MAC
    // The MAC has only one thread, so no THREADSTATE is explicitly passed
    InitMacDrawing();
#endif

Cleanup:
    // If successful, insert the per-thread state structure into the global chain
    if (!hr)
    {
        LOCK_GLOBALS;
        pts->ptsNext = g_pts;
        g_pts = pts;
    }
    Assert(SUCCEEDED(hr) && "Thread initialization failed");
    return hr;

Error:
    DllThreadDetach(pts);
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllProcessDetach
//
//  Synopsis:   Deinitialize the DLL.
//
//              This function can be called on partial initialization
//              of the DLL.  Every function called from here must be
//              capable being called without a corresponding call to
//              an initialization function.
//
//----------------------------------------------------------------------------
void
DllProcessDetach()
{
#if DBG==1
    char szModule[MAX_PATH];
    GetModuleFileNameA(NULL, szModule, MAX_PATH);
    TraceTag((0, "DllProcessDetach %s", szModule));
#endif

#if DBG==1
    for (int i = 0; i < sizeof(g_Zero); i++)
    {
        if (((BYTE *)&g_Zero)[i] != 0)
        {
            TraceTag((tagError, "g_Zero is non-zero at offset %d", i));
            Assert(0 && "g_Zero is not zero!");
            break;
        }
    }
#endif

	if (g_pts)
	{	
		// Our last, best hope for avoiding a crash it to try to take down the global
		// window on this thread and pray it was the last one.

		DllThreadEmergencyDetach();
	}

    DeinitWindowClasses();
    DeinitTextSubSystem();
    DeinitDownload();
    DeinitPalette();
#ifndef WIN16
    DeinitImageSizeCache();
#endif // ndef WIN16

#if defined(PRODUCT_PROF) && defined(_MAC)
    _FDisableMeas();
//    _UnHookGlue();
#endif // define(PRODUCT_PROF) && !defined(_MAC)

    DeinitTearOffCache();

#if DBG==1
    if (g_lSecondaryObjCount)
    {
        TraceTag((tagError, "Secondary object count=%d on DLL unload.", g_lSecondaryObjCount));
    }

    if (g_pts != NULL)
    {
        TraceTag((tagError, "One or more THREADSTATE's exist on DLL unload."));
    }
#endif

#ifndef WIN16
    // Delete global variable's critical section
    CGlobalLock::Deinit();
#endif // !WIN16

    // Free per-thread storage index
    if (g_dwTls != TLS_NULL)
    {
        TlsFree(g_dwTls);
    }
#ifdef WIN16
#if DBG==1
    if (g_hinstF3Debug > HINSTANCE_ERROR)
    {
        FreeLibrary(g_hinstF3Debug);
        g_hinstF3Debug = NULL;
    }
#endif
    DeinitHeap();
#endif

    if (g_pHtmPerfCtl)
        Verify(UnmapViewOfFile(g_pHtmPerfCtl));
    if (g_hMapHtmPerfCtl)
        Verify(CloseHandle(g_hMapHtmPerfCtl));
}


//+---------------------------------------------------------------------------
//
//  Function:   DllProcessAttach
//
//  Synopsis:   Initialize the DLL.
//
//  NOTE:       Even though DllMain is *not* called with DLL_THREAD_ATTACH
//              for the primary thread (since it is assumed during
//              DLL_PROCESS_ATTACH processing), this routines does not call
//              DllThreadAttach. Instead, all entry points are protected by a
//              call to EnsureThreadState which will, if necessary, call
//              DllThreadAttach. To call DllThreadAttach from here, might create
//              an unnecessary instance of the per-thread state structure.
//
//----------------------------------------------------------------------------

BOOL
DllProcessAttach()
{
    TCHAR *     pch = NULL;

#if DBG==1
    char szModule[MAX_PATH];
    GetModuleFileNameA(NULL, szModule, MAX_PATH);
    TraceTag((0, "DllProcessAttach %s", szModule));
#endif

#ifdef WIN16
    InitHeap();
#endif

    // Allocate per-thread storage index
    g_dwTls = TlsAlloc();
    if (g_dwTls == TLS_NULL)
        goto Error;

#ifndef WIN16
    // Prepare global variable's critical section
    CGlobalLock::Init();
#endif // !WIN16

    memset(&g_Zero, 0, sizeof(g_Zero));

    // BUGBUG: Move initialization to C-runtime dll0crt.c
    //g_hProcessHeap = GetProcessHeap();

#if DBG==1

    //  Tags for the .dll should be registered before
    //  calling DbgExRestoreDefaultDebugState().  Do this by
    //  declaring each global tag object or by explicitly calling
    //  DbgExTagRegisterTrace.

    DbgExRestoreDefaultDebugState();

#endif // DBG==1

#if defined(DEBUG_TEAROFFS) && !defined(WIN16)
    DebugCheckAllTearoffTables();
#endif

    InitUnicodeWrappers();

    if (!SetDLLName())
    {
        TraceTag((tagError, "Could not SetDLLName! (%d)", GetLastError()));
        goto Error;
    }

    // Register common clipboard formats used in Forms^3.  These are
    // available in the g_acfCommon array.

    RegisterClipFormats();

    // Now that the g_acfCommon are registered, we can initialize
    // the clip format array for the form and all the controls.

#ifndef WIN16
    InitFormClipFormats();
#endif // ndef WIN16

    // Initialize the global halftone palette
    if (FAILED(InitPalette()))
        goto Error;


    // Build help file name

    _tcscpy(g_achHelpFileName, g_achDLLCore);

#ifndef _MAC
    pch = _tcsrchr(g_achHelpFileName, _T('.'));

    Assert(pch);
    _tcscpy(pch, TEXT(".hlp"));
#endif

    IGNORE_HR(InitUrlCompatTable());

    InitClassTable();

#ifdef SWITCHES_ENABLED
    InitRuntimeSwitches();
#endif

    // Initialize the font cache. We do this here instead
    // of in the InitTextSubSystem() function because that
    // is very slow and we don't want to do it until
    // we know we're loading text. We need to do this here
    // because the registry loading code will initialize
    // the font face atom table.
    InitFontCache();

    // Hook into the htmperfctl block created by our host, if any

    char achName[sizeof(HTMPERFCTL_NAME) + 8 + 1];
    wsprintfA(achName, "%s%08lX", HTMPERFCTL_NAME, GetCurrentProcessId());

#ifndef UNIX // A hack to prevent Unix printing crash.
    g_hMapHtmPerfCtl = OpenFileMappingA(FILE_MAP_WRITE, FALSE, achName);
#else
    g_hMapHtmPerfCtl = NULL;
#endif

    if (g_hMapHtmPerfCtl)
    {
        // If MapViewOfFile fails then g_pHtmPerfCtl will be NULL, which is fine.  We just won't
        // have control of perf for this process.

        g_pHtmPerfCtl = (HTMPERFCTL *)MapViewOfFile(g_hMapHtmPerfCtl, FILE_MAP_WRITE, 0, 0, 0);

        Assert(g_pHtmPerfCtl);
    }

    return TRUE;

Error:
    DllProcessDetach();
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Standard DLL initialization entrypoint
//
//----------------------------------------------------------------------------
#ifndef WIN16

#if defined(UNIX) || defined(_MAC)
extern "C"
#endif
BOOL
WINAPI
DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    BOOL fOk = TRUE;

    AssertThreadDisable(TRUE);

    g_hInstCore = (HINSTANCE)hDll;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        fOk = DllProcessAttach();
        break;
    case DLL_PROCESS_DETACH:
        DllProcessDetach();
        break;
    case DLL_THREAD_DETACH:
        DllThreadEmergencyDetach();
        break;
    }

    AssertThreadDisable(FALSE);

    return fOk;
}


#else // win16
BOOL FAR PASCAL __loadds
LibMain(
    HINSTANCE hDll,
    WORD wDataSeg,
    WORD cbHeapSize,
    LPSTR lpszCmdLine )
{
    BOOL fOk = TRUE;

    AssertThreadDisable(TRUE);

    g_hInstCore = hDll;

    fOk = DllProcessAttach();

    AssertThreadDisable(FALSE);

    return fOk;
}

extern "C" int CALLBACK _WEP(int nExitType)
//int __export WINAPI WEP(int nExitType)
{
    DllProcessDetach();

    // this isn't being called properly, so do it here.
    CUnloadLibraries CUnloadLibs;

    return 1;
}
#endif // !win16 else win16

#ifndef WIN16
//+---------------------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Standard DLL entrypoint to determine if DLL can be unloaded
//              (Only the secondary object count need be checked since it
//               contains the total secondary object count and the sum of
//               primary object counts maintained by each thread.)
//
//---------------------------------------------------------------------------
STDAPI
DllCanUnloadNow()
{
    if (g_lSecondaryObjCount == 0 && g_pts == NULL)
        return(S_OK);

    return(S_FALSE);
}
#endif // ndef WIN16

//+---------------------------------------------------------------------------
//
//  Function:   _IncrementObjectCount
//
//  Synopsis:   Increment the per-thread object count
//
//----------------------------------------------------------------------------

void
_IncrementObjectCount()
{
    TLS(dll.lObjCount)++;
    IncrementSecondaryObjectCount( 9 );
}

//+---------------------------------------------------------------------------
//
//  Function:   IncrementObjectCountChk
//
//  Synopsis:   Increment the per-thread object count, and remember the thread
//              id of the thread which did it.
//
//----------------------------------------------------------------------------

#ifdef OBJCNTCHK

void
IncrementObjectCountChk(DWORD * pdwObjCnt)
{
    _IncrementObjectCount();

    if (pdwObjCnt)
    {
        *pdwObjCnt = GetCurrentThreadId();
    }
}

#endif

//+---------------------------------------------------------------------------
//
//  Function:   _DecrementObjectCount
//
//  Synopsis:   Decrement the per-thread object count and passivate the
//              thread when it transitions to zero.
//
//----------------------------------------------------------------------------
void
_DecrementObjectCount()
{
    THREADSTATE * pts = GetThreadState();

    Assert(pts->dll.lObjCount > 0);

    if (--pts->dll.lObjCount == 0)
    {
        pts->dll.lObjCount += ULREF_IN_DESTRUCTOR;
        DllThreadPassivate(pts);
        pts->dll.lObjCount -= ULREF_IN_DESTRUCTOR;
        Assert(pts->dll.lObjCount == 0);
        DllThreadDetach(pts);
        LOCK_GLOBALS;
#ifndef WIN16
        if (g_pts == NULL)
        {
            DllAllThreadsDetach();
        }
#endif // ndef WIN16
    }
    DecrementSecondaryObjectCount( 9 );
}

//+---------------------------------------------------------------------------
//
//  Function:   DecrementObjectCountChk
//
//  Synopsis:   Verify that the thread which incremented the object count is
//              the same as the current thread, then decrement the object count.
//
//----------------------------------------------------------------------------

#ifdef OBJCNTCHK

void
DecrementObjectCountChk(DWORD * pdwObjCnt)
{
    if (pdwObjCnt && *pdwObjCnt != GetCurrentThreadId())
    {
        char ach[512];
        wsprintfA(ach, "Attempt to DecrementObjectCount on the wrong thread.  The matching "
                 "IncrementObjectCount call happened on thread 0x%08lX; attemping release "
                 "on thread 0x%08lX.", *pdwObjCnt, GetCurrentThreadId());
#if DBG==1
        AssertSz(0, ach);
#else
        F3DebugBreak();
#endif
    }
    else
    {
        _DecrementObjectCount();
    }

    if (pdwObjCnt)
    {
        *pdwObjCnt = 0;
    }
}

#endif

//+---------------------------------------------------------------------------
//
//  Function:   SetDLLName ()
//
//  Synopsis:   Get the fm20.dll
//
//----------------------------------------------------------------------------

#ifdef _MAC
TCHAR g_szMacBase[32];
#endif

BOOL  SetDLLName(  void )
{



    //  BUGBUG: Win95 has a known bug: If you specify the
    //          LOAD_LIBRARY_AS_DATAFILE flag, all subsequent Load<Resource>
    //          calls could fail. That's why we use a straight LoadLibrary to
    //          load the resource DLL. This is due to the differences between
    //          instance handles and module handles in Windows.
    //  Fix:    When a fix for Win95 is issued be sure to insert the flag here.
    //          When the change happens, the following Windows calls need to
    //          be reviewed:
    //          CreateDialog, CreateDialogParam, DialogBox, DialogBoxParam,
    //          MessageBox, MessageBoxIndirect.
    //  Note:   DONT_RESOLVE_DLL_REFERENCES is specified as "NT only"
#ifndef _MAC
    if ((GetModuleFileName(g_hInstCore,
                           g_achDLLCore,
                           ARRAY_SIZE(g_achDLLCore)) == 0)
                           || (_tcslen(g_achDLLCore) == 0))
        return( FALSE );
#else
    TCHAR    achAppLoc[MAX_PATH];

    if (GetModuleFileName(g_hInstCore, achAppLoc, MAX_PATH) == 0)

    if (GetFileTitle(achAppLoc,g_szMacBase,MAX_PATH))
            return( FALSE );

    if (_tcslen(g_szMacBase) == 0)
            return( FALSE );

    _tcscpy(g_achDLLCore,_T("MSHTM"));

#endif

    return (TRUE);


}
