#include "priv.h"
#include "unicpp\clsobj.h"
#include "clsobj.h"

#define MLUI_INIT
#include <mluisupp.h>

LONG                g_cRefThisDll = 0;      // per-instance
HINSTANCE           g_hinst;

CRITICAL_SECTION g_csDll;

BOOL g_fRunningOnNT;
BOOL g_bRunOnNT5;
BOOL g_bRunOnMemphis;
BOOL g_bMirroredOS = FALSE;
DWORD g_dwShell32;
HKEY g_hkcuExplorer;
HKEY g_hklmExplorer;
UINT g_msgMSWheel;
t_AllowSetForegroundWindow g_pfnAllowSetForegroundWindow;

// static class factory (no allocs!)

typedef struct {
    const IClassFactoryVtbl *cf;
    REFCLSID rclsid;
    HRESULT (*pfnCreate)(IUnknown *, REFIID, void **);
    ULONG flags;
} OBJ_ENTRY;

#define OBJ_AGGREGATABLE 1

extern const IClassFactoryVtbl c_CFVtbl;        // forward


//
// we always do a linear search here so put your most often used things first
//
// TODO: save code by checking aggregation on our side (have a bit in this const table)
// instead of having each CreateInstance function do it (since most are not aggregatable)
//
const OBJ_ENTRY c_clsmap[] = {
    #include "unicpp\clsobj.tbl"
    #include "clsobj.tbl"
    { NULL, NULL, NULL }
};

STDMETHODIMP CCF_QueryInterface(IClassFactory *pcf, REFIID riid, void **ppvObj)
{
    // OBJ_ENTRY *this = IToClass(OBJ_ENTRY, cf, pcf);
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = (void *)pcf;
        DllAddRef();
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCF_AddRef(IClassFactory *pcf)
{
    DllAddRef();
    return 2;
}

STDMETHODIMP_(ULONG) CCF_Release(IClassFactory *pcf)
{
    DllRelease();
    return 1;
}

STDMETHODIMP CCF_CreateInstance(IClassFactory *pcf, IUnknown *punkOuter, REFIID riid, void **ppvObject)
{
    OBJ_ENTRY *this = IToClass(OBJ_ENTRY, cf, pcf);
    *ppvObject = NULL; // to avoid nulling it out in every create function...

    if (punkOuter && !(this->flags & OBJ_AGGREGATABLE))
        return CLASS_E_NOAGGREGATION;

    return this->pfnCreate(punkOuter, riid, ppvObject);
}

STDMETHODIMP CCF_LockServer(IClassFactory *pcf, BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();
    return S_OK;
}

const IClassFactoryVtbl c_CFVtbl = {
    CCF_QueryInterface, CCF_AddRef, CCF_Release,
    CCF_CreateInstance,
    CCF_LockServer
};

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
    {
        const OBJ_ENTRY *pcls;
        for (pcls = c_clsmap; pcls->rclsid; pcls++)
        {
            if (IsEqualIID(rclsid, pcls->rclsid))
            {
                *ppv = (void *)&(pcls->cf);
                DllAddRef();
                return NOERROR;
            }
        }
#ifdef DEBUG
        {
            TCHAR szClass[GUIDSTR_MAX];
            SHStringFromGUID(rclsid, szClass, ARRAYSIZE(szClass));
            AssertMsg(TF_ERROR,
                      TEXT("DllGetClassObject !!! %s not in shell32; ")
                      TEXT("corrupted registry?"), szClass);
        }
#endif

    }

    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;;
}


STDAPI_(void) DllAddRef(void)
{
    InterlockedIncrement(&g_cRefThisDll);
    ASSERT(g_cRefThisDll < 1000);   // reasonable upper limit
}

STDAPI_(void) DllRelease(void)
{
    InterlockedDecrement(&g_cRefThisDll);
    ASSERT(g_cRefThisDll >= 0);      // don't underflow

}

STDAPI DllCanUnloadNow(void)
{
    if (g_cRefThisDll)
        return S_FALSE;
    return S_OK;
}

// Stub function in case we're running on something that doesn't support
// AllowSetForegroundWindow.
BOOL Dummy_AllowSetForegroundWindow(DWORD dwProcessID)
{
    return FALSE;
}

void _ProcessAttach(HINSTANCE hDll)
{
    DEBUG_CODE( g_bInDllEntry = TRUE );

    g_hinst = hDll;
    DisableThreadLibraryCalls(hDll);    // perf

    MLLoadResources(g_hinst, TEXT("shd401lc.dll"));

    // override the ML_CROSSCODEPAGE flag in the g_mluiInfo from
    // mluisupp.h so that shdoc401 will plug to the system codepage
    // but not to anything else
    g_mluiInfo.dwCrossCodePage = ML_SHELL_LANGUAGE;

    InitializeCriticalSection(&g_csDll);

    // Don't put it under #ifdef DEBUG
    CcshellGetDebugFlags();

    // AllowSetForegroundWindow
    g_pfnAllowSetForegroundWindow = (t_AllowSetForegroundWindow)
            GetProcAddress(GetModuleHandle(TEXT("USER32.DLL")),
                           "AllowSetForegroundWindow");
    if (!g_pfnAllowSetForegroundWindow) {
        g_pfnAllowSetForegroundWindow = Dummy_AllowSetForegroundWindow;
    }

    g_msgMSWheel = RegisterWindowMessage(TEXT("MSWHEEL_ROLLMSG"));

#if 0
#ifdef DEBUG
    g_TlsMem = TlsAlloc();
    if (IsFlagSet(g_dwBreakFlags, BF_ONLOADED))
    {
        TraceMsg(TF_ALWAYS, "SHDOC401.DLL has just loaded");
        DEBUG_BREAK;
    }
#endif
#endif

    g_fRunningOnNT = IsOS(OS_NT);
    if (g_fRunningOnNT)
        g_bRunOnNT5 = IsOS(OS_NT5);
    else
        g_bRunOnMemphis = IsOS(OS_MEMPHIS);

        g_bMirroredOS = IS_MIRRORING_ENABLED();
#if 0
    g_fRunOnFE = GetSystemMetrics(SM_DBCSENABLED);
    g_uiACP = GetACP();
#endif

    {
        HINSTANCE hinst = GetModuleHandle(TEXT("SHELL32.DLL"));
        if (hinst)
        {

            // NOTE: GetProcAddress always takes ANSI strings!
            DLLGETVERSIONPROC pfnGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinst, "DllGetVersion");
            DLLVERSIONINFO dllinfo;

            dllinfo.cbSize = sizeof(DLLVERSIONINFO);
            if (pfnGetVersion && pfnGetVersion(&dllinfo) == NOERROR)
                g_dwShell32 = MAKELONG(dllinfo.dwMinorVersion,
                                       dllinfo.dwMajorVersion);
        }
    }

    // globally useful registry keys
    RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, &g_hkcuExplorer);
    RegCreateKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_EXPLORER, &g_hklmExplorer);

    DEBUG_CODE( g_bInDllEntry = FALSE );

}

void _ProcessDetach(void)
{
    MLFreeResources(g_hinst);

    if (g_hkcuExplorer)
        RegCloseKey(g_hkcuExplorer);
    if (g_hklmExplorer)
        RegCloseKey(g_hklmExplorer);
    DeleteCriticalSection(&g_csDll);
}

STDAPI_(BOOL) DllMain(HINSTANCE hDll, DWORD dwReason, void *lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _ProcessAttach(hDll);
    }

    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _ProcessDetach();
    }

    DeskMovr_DllMain( hDll, dwReason, lpReserved );

    return TRUE;
}
