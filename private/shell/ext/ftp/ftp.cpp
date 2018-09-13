/*****************************************************************************
 *
 *        ftp.cpp - FTP folder bookkeeping
 *
 *****************************************************************************/

#include "priv.h"
#include "ftpinet.h"
#include "ftpsite.h"
#include "ftplist.h"
#include "msieftp.h"
#include "cookie.h"

extern CFtpList * g_FtpSiteCache;
extern DWORD g_dwOpenConnections;

/*****************************************************************************
 *
 *    Dynamic Globals.  There should be as few of these as possible.
 *
 *    All access to dynamic globals must be thread-safe.
 *
 *****************************************************************************/

ULONG g_cRef = 0;            /* Global reference count */
CRITICAL_SECTION g_csDll;    /* The shared critical section */


extern HANDLE g_hthWorker;             // Background worker thread

#ifdef DEBUG
DWORD g_TlsMem = 0xffffffff;
extern DWORD g_TLSliStopWatchStartHi;
extern DWORD g_TLSliStopWatchStartLo;

LEAKSTRUCT g_LeakList[] =
{
    {0, "CFtpFolder"},
    {0, "CFtpDir"},
    {0, "CFtpSite"},
    {0, "CFtpObj"},
    {0, "CFtpEidl"},
    {0, "CFtpDrop"},
    {0, "CFtpList"},
    {0, "CFtpStm"},
    {0, "CAccount"},
    {0, "CFtpFactory"},
    {0, "CFtpContextMenu"},
    {0, "CFtpEfe"},
    {0, "CFtpGlob"},
    {0, "CFtpIcon"},
    {0, "CMallocItem"},
    {0, "CFtpPidlList"},
    {0, "CFtpProp"},
    {0, "CStatusBar"},
    {0, "CFtpView"},
    {0, "CFtpWebView"},
    {0, "CCookieList"},
    {0, "CDropOperation"}
};
#endif // DEBUG

ULONG g_cRef_CFtpView = 0;  // Needed to determine when to purge cache.

/*****************************************************************************
 *
 *    DllAddRef / DllRelease
 *
 *    Maintain the DLL reference count.
 *
 *****************************************************************************/

void DllAddRef(void)
{
    CREATE_CALLERS_ADDRESS;         // For debug spew.

    InterlockedIncrement((LPLONG)&g_cRef);
    TraceMsg(TF_FTPREF, "DllAddRef() g_cRef=%d, called from=%#08lx.", g_cRef, GET_CALLERS_ADDRESS);
}

void DllRelease(void)
{
    CREATE_CALLERS_ADDRESS;         // For debug spew.

    TraceMsg(TF_FTPREF, "DllRelease() g_cRef=%d, called from=%#08lx.", g_cRef-1, GET_CALLERS_ADDRESS);
    InterlockedDecrement((LPLONG)&g_cRef);
}

/*****************************************************************************
 *
 *    DllGetClassObject
 *
 *    OLE entry point.  Produces an IClassFactory for the indicated GUID.
 *
 *    The artificial refcount inside DllGetClassObject helps to
 *    avoid the race condition described in DllCanUnloadNow.  It's
 *    not perfect, but it makes the race window much smaller.
 *
 *****************************************************************************/

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres;

    DllAddRef();
    if (IsEqualIID(rclsid, CLSID_FtpFolder) ||
        IsEqualIID(rclsid, CLSID_FtpWebView) ||
        IsEqualIID(rclsid, CLSID_FtpDataObject) ||
        IsEqualIID(rclsid, CLSID_FtpInstaller))
    {
        hres = CFtpFactory_Create(rclsid, riid, ppvObj);
    }
    else
    {
        *ppvObj = NULL;
        hres = CLASS_E_CLASSNOTAVAILABLE;
    }

    DllRelease();
    return hres;
}

/*****************************************************************************
 *
 *    DllCanUnloadNow
 *
 *    OLE entry point.  Fail iff there are outstanding refs.
 *
 *    There is an unavoidable race condition between DllCanUnloadNow
 *    and the creation of a new IClassFactory:  Between the time we
 *    return from DllCanUnloadNow() and the caller inspects the value,
 *    another thread in the same process may decide to call
 *    DllGetClassObject, thus suddenly creating an object in this DLL
 *    when there previously was none.
 *
 *    It is the caller's responsibility to prepare for this possibility;
 *    there is nothing we can do about it.
 *
 *****************************************************************************/

STDMETHODIMP DllCanUnloadNow(void)
{
    HRESULT hres;

    ENTERCRITICALNOASSERT;

    // Purge Cache if there aren't any FtpViews open.
    if ((0 == g_cRef_CFtpView))
    {
        // Since no views are open, we want to try to purge
        // the Delayed Actions so we can closed down the background
        // thread.  Is it running?
        if (AreOutstandingDelayedActions())
        {
            LEAVECRITICALNOASSERT;
            PurgeDelayedActions();  // Try to close it down.
            ENTERCRITICALNOASSERT;
        }

        if (!AreOutstandingDelayedActions())    // Did it close down?
        {
            // We need to purge the session key because we lost the password
            // redirects in the CFtpSites.  So we would login but later fail
            // when we try to fish out the password when falling back to
            // URLMON/shdocfl for file downloads. (NT #362108)
            PurgeSessionKey();
            CFtpPunkList_Purge(&g_FtpSiteCache);    // Yes so purge the cache...
        }
    }

    hres = g_cRef ? S_FALSE : S_OK;
    TraceMsg(TF_FTP_DLLLOADING, "DllCanUnloadNow() DllRefs=%d, returning hres=%#08lx. (S_OK means yes)", g_cRef, hres);

    LEAVECRITICALNOASSERT;

    return hres;
}


void CheckForLeaks(BOOL fForce)
{
#ifdef DEBUG
    DWORD dwLeakCount = 0;

    if (fForce)
    {
        // Let's free our stuff so we can make sure not to leak it.
        // This is done more to force our selves to be w/o leaks
        // than anything else.
        DllCanUnloadNow();
    }

    for (int nIndex = 0; nIndex < ARRAYSIZE(g_LeakList); nIndex++)
        dwLeakCount += g_LeakList[nIndex].dwRef;

    if ((!g_FtpSiteCache || fForce) && (dwLeakCount || g_dwOpenConnections || g_cRef))
    {
        TraceMsg(TF_ALWAYS, "***********************************************");
        TraceMsg(TF_ALWAYS, "* LEAK  -  LEAK  -  LEAK  -  LEAK  -  LEAK    *");
        TraceMsg(TF_ALWAYS, "*                                             *");
        TraceMsg(TF_ALWAYS, "* WARNING: The FTP Shell Extension Leaked     *");
        TraceMsg(TF_ALWAYS, "*          one or more objects                *");
        TraceMsg(TF_ALWAYS, "***********************************************");
        TraceMsg(TF_ALWAYS, "*                                             *");
        for (int nIndex = 0; nIndex < ARRAYSIZE(g_LeakList); nIndex++)
        {
            if (g_LeakList[nIndex].dwRef)
                TraceMsg(TF_ALWAYS, "* %hs, Leaked=%d                          *", g_LeakList[nIndex].szObject, g_LeakList[nIndex].dwRef);
        }
        TraceMsg(TF_ALWAYS, "*                                             *");
        TraceMsg(TF_ALWAYS, "* Open Wininet Connections=%d                  *", g_dwOpenConnections);
        TraceMsg(TF_ALWAYS, "* DLL Refs=%d                                  *", g_cRef);
        TraceMsg(TF_ALWAYS, "*                                             *");
        TraceMsg(TF_ALWAYS, "***********************************************");
        ASSERT(0);
    }

#endif // DEBUG
}


// Globals to free. 
extern CCookieList * g_pCookieList;

/*****************************************************************************
 *
 *    Entry32
 *
 *    DLL entry point.
 *
 *    BUGBUG -- On a thread detach, must check if the thread owns any
 *    global timeouts.  If so, we must transfer the timeout to another
 *    thread or something.
 *
 *****************************************************************************/
STDAPI_(BOOL) DllEntry(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    // This is called in two situations, FreeLibrary() is called and lpReserved is
    // NULL, or the process is shutting down and lpReserved is not NULL.
    BOOL fIsProcessShuttingDown = (lpReserved ? TRUE : FALSE);

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        InitializeCriticalSection(&g_csDll);
#ifdef DEBUG
        g_TlsMem = TlsAlloc();
        g_TLSliStopWatchStartHi = TlsAlloc();
        g_TLSliStopWatchStartLo = TlsAlloc();
#endif

        // Don't put it under #ifdef DEBUG
        CcshellGetDebugFlags();
        DisableThreadLibraryCalls(hinst);

        g_hthWorker = NULL;

        g_hinst = hinst;
        g_formatEtcOffsets.cfFormat         = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLISTOFFSET);
        g_formatPasteSucceeded.cfFormat     = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PASTESUCCEEDED);
        g_cfTargetCLSID                     = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_TARGETCLSID);

        g_dropTypes[DROP_FCont].cfFormat    = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILECONTENTS);
        g_dropTypes[DROP_FGDW].cfFormat     = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);
        g_dropTypes[DROP_FGDA].cfFormat     = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA);
        g_dropTypes[DROP_IDList].cfFormat   = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLIST);
        g_dropTypes[DROP_FNMA].cfFormat     = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILENAMEMAPA);
        g_dropTypes[DROP_FNMW].cfFormat     = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILENAMEMAPW);
        g_dropTypes[DROP_PrefDe].cfFormat   = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
        g_dropTypes[DROP_PerfDe].cfFormat   = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PERFORMEDDROPEFFECT);
        g_dropTypes[DROP_FTP_PRIVATE].cfFormat = (CLIPFORMAT)RegisterClipboardFormat(TEXT("FtpPrivateData"));
        g_dropTypes[DROP_URL].cfFormat      = (CLIPFORMAT)RegisterClipboardFormat(TEXT("UniformResourceLocator"));
        g_dropTypes[DROP_OLEPERSIST].cfFormat       = (CLIPFORMAT)RegisterClipboardFormat(TEXT("OleClipboardPersistOnFlush"));

        GetModuleFileNameA(GetModuleHandle(TEXT("SHELL32")), g_szShell32, ARRAYSIZE(g_szShell32));

        if (FAILED(CFtpSite_Init()))
            return 0;

        break;

    case DLL_PROCESS_DETACH:
    {
        CCookieList * pCookieList = (CCookieList *) InterlockedExchangePointer((void **) &g_pCookieList, NULL);
        if (pCookieList)
            delete pCookieList;

        // Yes, so we need to make sure all of the CFtpView's have closed down
        // or it's really bad to purge the FTP cache of FTP Servers (CFtpSite) and
        // their directories (CFtpDir).
        ASSERT(0 == g_cRef_CFtpView);

        // Now force the Delayed Actions to happen now instead of waiting.
        PurgeDelayedActions();

        CheckForLeaks(fIsProcessShuttingDown);

        UnloadWininet();
        DeleteCriticalSection(&g_csDll);
#ifdef DEBUG
        if (g_TLSliStopWatchStartHi)
        {
            TlsFree(g_TLSliStopWatchStartHi);
            g_TLSliStopWatchStartHi = NULL;
        }
        if (g_TLSliStopWatchStartLo)
        {
            TlsFree(g_TLSliStopWatchStartLo);
            g_TLSliStopWatchStartLo = NULL;
        }
#endif
    }
    break;
    }
    return 1;
}


