#include "private.h"
#include "mlmain.h"
#include "mlstr.h"
#include "convobj.h"
#include "cpdetect.h"
#ifdef NEWMLSTR
#include "attrstrw.h"
#include "attrstra.h"
#include "attrloc.h"
#include "util.h"
#endif

#define _ATL_MIN_CRT
#define _WINDLL
#include <atlimpl.cpp>

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CMultiLanguage, CMultiLanguage)
    OBJECT_ENTRY(CLSID_CMLangString, CMLStr)
    OBJECT_ENTRY(CLSID_CMLangConvertCharset, CMLangConvertCharset)
#ifdef NEWMLSTR
    OBJECT_ENTRY(CLSID_CMLStrAttrWStr, CMLStrAttrWStr)
    OBJECT_ENTRY(CLSID_CMLStrAttrAStr, CMLStrAttrAStr)
    OBJECT_ENTRY(CLSID_CMLStrAttrLocale, CMLStrAttrLocale)
#endif
END_OBJECT_MAP()

//
//  Globals
//
HINSTANCE   g_hInst = NULL;
HINSTANCE   g_hUrlMon = NULL;
CRITICAL_SECTION g_cs;
CComModule _Module;
#ifdef NEWMLSTR
CMLAlloc* g_pMalloc;
#endif
BOOL g_bIsNT5;
BOOL g_bIsNT;
BOOL g_bIsWin98;
//
//  Build Global Objects
//
void BuildGlobalObjects(void)
{
    DebugMsg(DM_TRACE, TEXT("BuildGlobalObjects called."));
    EnterCriticalSection(&g_cs);
    // Build CMimeDatabase Object
    if (NULL == g_pMimeDatabase)
        g_pMimeDatabase = new CMimeDatabase;
#ifdef NEWMLSTR
    if (NULL == g_pMalloc)
        g_pMalloc = new CMLAlloc;
#endif
    LeaveCriticalSection(&g_cs);
}

void FreeGlobalObjects(void)
{
    DebugMsg(DM_TRACE, TEXT("FreeGlobalObjects called."));
    // Free CMimeDatabase Object
    if (NULL != g_pMimeDatabase)
    {
        delete g_pMimeDatabase;
        g_pMimeDatabase = NULL;
    }
#ifdef NEWMLSTR
    if (NULL != g_pMalloc)
    {
        delete g_pMalloc;
        g_pMalloc = NULL;
    }
#endif

    // LCDETECT
    if ( NULL != g_pLCDetect )
    {
        delete (LCDetect *)g_pLCDetect;
        g_pLCDetect = NULL;
    }

    if (NULL != g_pCpMRU)
    {
        delete g_pCpMRU;
        g_pCpMRU = NULL;
    }

    if (g_pMimeDatabaseReg)
    {
        delete g_pMimeDatabaseReg;
        g_pMimeDatabaseReg = NULL;
    }

    CMLangFontLink_FreeGlobalObjects();
}

//
//  DLL part of the Object
//
extern "C" BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwReason, LPVOID)
{
    BOOL fRet = TRUE;

    DebugMsg(DM_TRACE, TEXT("DllMain called. dwReason=0x%08x"), dwReason);
    switch (dwReason)
    {
        LPVOID lpv;

        case DLL_PROCESS_ATTACH:
            InitializeCriticalSection(&g_cs);
            g_hInst = (HINSTANCE)hInstance;
            DisableThreadLibraryCalls(g_hInst);
            
            _Module.Init(ObjectMap, g_hInst);
            // HACKHACK (reinerf) - because ATL2.1 bites the big one, we have to malloc some memory
            // here so that it will cause _Module.m_hHeap to be initialized. They do not init this
            // member variable in a thread safe manner, so we will alloc and free a small chunk of
            // memory right now to ensure that the heap is created only once.
            lpv = malloc(2 * sizeof(CHAR));
            if (lpv)
            {
                free(lpv);
            }

            g_bIsNT5 = staticIsOS(OS_NT5);
            g_bIsNT = staticIsOS(OS_NT);
            g_bIsWin98 = staticIsOS(OS_MEMPHIS);
            break;

        case DLL_PROCESS_DETACH:
            FreeGlobalObjects();
            _Module.Term();
            DeleteCriticalSection(&g_cs);
            if (g_hUrlMon)
            {
               FreeLibrary(g_hUrlMon);
            }

            break;
    }
    return TRUE;
}

void DllAddRef(void)
{
    _Module.Lock();
}

void DllRelease(void)
{
    _Module.Unlock();
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppvObj)
{
    DebugMsg(DM_TRACE, TEXT("DllGetClassObject called."));
    if (NULL == g_pMimeDatabase)
        BuildGlobalObjects();

    return _Module.GetClassObject(rclsid, riid, ppvObj);
}

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount() == 0) ? S_OK : S_FALSE;
}

//
//  Self Registration part
//
#if 0
HRESULT CallRegInstall(LPCSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    DebugMsg(DM_TRACE, TEXT("CallRegInstall called for %s."), szSection);
    if (NULL != hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, achREGINSTALL);

        if (NULL != pfnri)
            hr = pfnri(g_hInst, szSection, NULL);
        FreeLibrary(hinstAdvPack);
    }
    return hr;
}
#endif

STDAPI DllRegisterServer(void)
{
    HRESULT hr;

    DebugMsg(DM_TRACE, TEXT("DllRegisterServer called."));

#if 0
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    OSVERSIONINFO osvi;
    BOOL fRunningOnNT;


    // Determine which version of NT or Windows we're running on
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);
    fRunningOnNT = (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId);
    
    // Delete any old registration entries, then add the new ones.
    // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.
    CallRegInstall("UnReg");
    hr = CallRegInstall(fRunningOnNT? "Reg.NT": "Reg");
    if (NULL != hinstAdvPack)
        FreeLibrary(hinstAdvPack);

    // BUGBUG: Need to register TypeLib here ...
    // Get the full path of this module
    GetModuleFileName(g_hInst, szModule, ARRAYSIZE(szModule));

    // Register our TypeLib
    MultiByteToWideChar(CP_ACP, 0, szModule, -1, wszTemp, ARRAYSIZE(wszTemp));
    hr = LoadTypeLib(wszTemp, &pTypeLib);
    if (SUCCEEDED(hr))
    {
        hr = RegisterTypeLib(pTypeLib, wszTemp, NULL);
        pTypeLib->Release();
    }
#else
    hr = RegisterServerInfo();
// Legacy registry MIME DB code, keep it for backward compatiblility
    MimeDatabaseInfo();
#endif
    return hr;
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;

    DebugMsg(DM_TRACE, TEXT("DllUnregisterServer called."));
#if 0
    hr = CallRegInstall("UnReg");
#else
    hr = UnregisterServerInfo();
#endif
    return hr;
}
