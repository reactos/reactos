#include "private.h"
#include "multiusr.h"

#define INITGUID
#include <initguid.h>
#undef __msident_h__
#include "msident.h"
#include "shlwapi.h"

extern "C" int _fltused = 0;    // define this so that floats and doubles don't bring in the CRT

// Count number of objects and number of locks.
ULONG       g_cObj=0;
ULONG       g_cLock=0;

// DLL Instance handle
HINSTANCE   g_hInst=0;

// mutex for preventing logon re-entrancy
HANDLE      g_hMutex = NULL;

#define IDENTITY_LOGIN_VALUE    0x00098053
#define DEFINE_STRING_CONSTANTS
#include "StrConst.h"

#define MLUI_SUPPORT
#define MLUI_INIT
#include "mluisup.h"

BOOL    g_fNotifyComplete = TRUE;
GUID    g_uidOldUserId = {0x0};
GUID    g_uidNewUserId = {0x0};

void FixMissingIdentityNames();
void UnloadPStore();

// This is needed so we can link to libcmt.dll, because floating-point
// initialization code is required.
void __cdecl main()
{
} 

//////////////////////////////////////////////////////////////////////////
//
// DLL entry point
//
//////////////////////////////////////////////////////////////////////////
EXTERN_C BOOL WINAPI LibMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pvReserved)
{

    switch (ulReason)
    {
        case DLL_PROCESS_ATTACH:
            MemInit();
            MLLoadResources(hInstance, TEXT("msidntld.dll"));
            if (MLGetHinst() == NULL)
                return FALSE;

            if (g_hMutex == NULL)
            {
                g_hMutex = CreateMutex(NULL, FALSE, "MSIdent Logon");

                if (g_hMutex == NULL)
                {
                    
                }

                if (GetLastError() != ERROR_ALREADY_EXISTS)
                {
                    GUID        uidStart;
                    USERINFO    uiLogin;
					
                    FixMissingIdentityNames();
                    // we are the first instance to come up.
                    // may need to reset the last user.....
                    if (GetProp(GetDesktopWindow(),"IDENTITY_LOGIN") != (HANDLE)IDENTITY_LOGIN_VALUE)
                    {
                        _MigratePasswords();
                        MU_GetLoginOption(&uidStart);

                        // if there is a password on this identity, we can't auto start as them
                        if (uidStart != GUID_NULL && MU_GetUserInfo(&uidStart, &uiLogin) && (uiLogin.fUsePassword || !uiLogin.fPasswordValid))
                        {
                            uidStart = GUID_NULL;
                        }

                        if (uidStart == GUID_NULL)
                        {
                            MU_SwitchToUser("");
                            SetProp(GetDesktopWindow(),"IDENTITY_LOGIN", (HANDLE)IDENTITY_LOGIN_VALUE);
                        }
                        else
                        {
                            if(MU_GetUserInfo(&uidStart, &uiLogin))
                                MU_SwitchToUser(uiLogin.szUsername);
                            else
                                MU_SwitchToUser("");
                        }
                        SetProp(GetDesktopWindow(),"IDENTITY_LOGIN", (HANDLE)IDENTITY_LOGIN_VALUE);
                    }
                }
            }
            DisableThreadLibraryCalls(hInstance);
            g_hInst = hInstance;

            break;

        case DLL_PROCESS_DETACH:
            MLFreeResources(hInstance);
            UnloadPStore();
            CloseHandle(g_hMutex);
            g_hMutex = NULL;
            MemUnInit();
            break;
    }

    return TRUE;
} 

//////////////////////////////////////////////////////////////////////////
//
// Standard OLE entry points
//
//////////////////////////////////////////////////////////////////////////

//  Class factory -
//  For classes with no special needs these macros should take care of it.
//  If your class needs some special stuff just to get the ball rolling,
//  implement your own CreateInstance method.  (ala, CConnectionAgent)

#define DEFINE_CREATEINSTANCE(cls, iface) \
HRESULT cls##_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk) \
{ \
    *ppunk = (iface *)new cls; \
    return (NULL != *ppunk) ? S_OK : E_OUTOFMEMORY; \
}

#define DEFINE_AGGREGATED_CREATEINSTANCE(cls, iface) \
HRESULT cls##_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk) \
{ \
    *ppunk = (iface *)new cls(punkOuter); \
    return (NULL != *ppunk) ? S_OK : E_OUTOFMEMORY; \
}

DEFINE_CREATEINSTANCE(CUserIdentityManager, IUserIdentityManager)

const CFactoryData g_FactoryData[] = 
{
 {   &CLSID_UserIdentityManager,        CUserIdentityManager_CreateInstance,    0 }
};

HRESULT APIENTRY DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    HRESULT hr = S_OK;
    IUnknown *punk = NULL;

    *ppv = NULL;

    MU_Init();

    // Validate request
    for (int i = 0; i < ARRAYSIZE(g_FactoryData); i++)
    {
        if (rclsid == *g_FactoryData[i].m_pClsid)
        {
            punk = new CClassFactory(&g_FactoryData[i]);
            break;
        }
    }

    if (ARRAYSIZE(g_FactoryData) <= i)
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }
    else if (NULL == punk)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = punk->QueryInterface(riid, ppv);
        punk->Release();
    } 


    return hr;
}

STDAPI DllCanUnloadNow(void)
{
    // check objects and locks
    return (0L == DllGetRef() && 0L == DllGetLock()) ? S_OK : S_FALSE;
}

//////////////////////////////////////////////////////////////////////////
//
// Autoregistration entry points
//
//////////////////////////////////////////////////////////////////////////

HRESULT CallRegInstall(LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    char        szDll[MAX_PATH];
    int         cch;
    STRENTRY    seReg[2];
    STRTABLE    stReg;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, achREGINSTALL);

        if (pfnri)
        {
            // Get our location
            GetModuleFileName(g_hInst, szDll, sizeof(szDll));

            // Setup special registration stuff
            // Do this instead of relying on _SYS_MOD_PATH which loses spaces under '95
            stReg.cEntries = 0;
            seReg[stReg.cEntries].pszName = "SYS_MOD_PATH";
            seReg[stReg.cEntries].pszValue = szDll;
            stReg.cEntries++;    
            stReg.pse = seReg;

            hr = pfnri(g_hInst, szSection, &stReg);
        }

        FreeLibrary(hinstAdvPack);
    }

    return hr;
}


STDAPI DllRegisterServer(void)
{
    // Delete any old registration entries, then add the new ones.
    // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    CallRegInstall("Reg");
    if (hinstAdvPack)
    {
        FreeLibrary(hinstAdvPack);
    }

    return NOERROR;
}

STDAPI
DllUnregisterServer(void)
{
    return NOERROR;
}
