#include "sendmail.h"       // pch file
#include "comdll.h"

UINT g_cRefDll = 0;     // reference count for this DLL
HANDLE g_hinst = NULL;  // HMODULE for this DLL

typedef struct {
    const IClassFactoryVtbl *cf;
    const CLSID *pclsid;
    HRESULT (STDMETHODCALLTYPE *pfnCreate)(IUnknown *, REFIID, void **);
    HRESULT (STDMETHODCALLTYPE *pfnRegUnReg)(BOOL bReg, HKEY hkCLSID, LPCTSTR pszCLSID, LPCTSTR pszModule);
} OBJ_ENTRY;

extern const IClassFactoryVtbl c_CFVtbl;        // forward

STDAPI DesktopShortcut_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);

//
// we always do a linear search here so put your most often used things first
//
const OBJ_ENTRY c_clsmap[] = {
    { &c_CFVtbl, &CLSID_MailRecipient,   MailRecipient_CreateInstance, MailRecipient_RegUnReg },
    { &c_CFVtbl, &CLSID_DesktopShortcut,   DesktopShortcut_CreateInstance, DesktopShortcut_RegUnReg },
    // add more entries here
    { NULL, NULL, NULL, NULL }
};

// static class factory (no allocs!)

STDMETHODIMP CClassFactory_QueryInterface(IClassFactory *pcf, REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = (void *)pcf;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    DllAddRef();
    return NOERROR;
}

STDMETHODIMP_(ULONG) CClassFactory_AddRef(IClassFactory *pcf)
{
    DllAddRef();
    return 2;
}

STDMETHODIMP_(ULONG) CClassFactory_Release(IClassFactory *pcf)
{
    DllRelease();
    return 1;
}

STDMETHODIMP CClassFactory_CreateInstance(IClassFactory *pcf, IUnknown *punkOuter, REFIID riid, void **ppvObject)
{
    OBJ_ENTRY *this = IToClass(OBJ_ENTRY, cf, pcf);
    return this->pfnCreate(punkOuter, riid, ppvObject);
}

STDMETHODIMP CClassFactory_LockServer(IClassFactory *pcf, BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();
    return S_OK;
}

const IClassFactoryVtbl c_CFVtbl = {
    CClassFactory_QueryInterface, CClassFactory_AddRef, CClassFactory_Release,
    CClassFactory_CreateInstance,
    CClassFactory_LockServer
};

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
    {
        const OBJ_ENTRY *pcls;
        for (pcls = c_clsmap; pcls->pclsid; pcls++)
        {
            if (IsEqualIID(rclsid, pcls->pclsid))
            {
                *ppv = (void *)&(pcls->cf);
                DllAddRef();    // Class Factory keeps dll in memory
                return NOERROR;
            }
        }
    }
    // failure
    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;;
}

STDAPI_(void) DllAddRef()
{
    InterlockedIncrement(&g_cRefDll);
}

STDAPI_(void) DllRelease()
{
    InterlockedDecrement(&g_cRefDll);
}

STDAPI DllCanUnloadNow(void)
{
    return g_cRefDll == 0 ? S_OK : S_FALSE;
}

STDAPI_(BOOL) DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hinst = hDll;
        DisableThreadLibraryCalls(hDll);

#ifdef DEBUG
        CcshellGetDebugFlags();
        if (IsFlagSet(g_dwBreakFlags, BF_ONLOADED))
        {
            TraceMsg(TF_ALWAYS, "SENDMAIL.DLL has just loaded");
            DEBUG_BREAK;
        }
#endif
    }

    return TRUE;
}

void StringFromGUID(const CLSID* piid, LPTSTR pszBuf)
{
    wnsprintf(pszBuf, MAX_PATH, TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"), piid->Data1,
            piid->Data2, piid->Data3, piid->Data4[0], piid->Data4[1], piid->Data4[2],
            piid->Data4[3], piid->Data4[4], piid->Data4[5], piid->Data4[6], piid->Data4[7]);
}

BOOL DeleteKeyAndSubKeys(HKEY hkIn, LPCTSTR pszSubKey)
{
    HKEY  hk;
    TCHAR szTmp[MAX_PATH];
    DWORD dwTmpSize;
    long  l;
    BOOL  f;
    int   x;

    l = RegOpenKeyEx(hkIn, pszSubKey, 0, KEY_ALL_ACCESS, &hk);
    if (l != ERROR_SUCCESS) 
        return FALSE;

    // loop through all subkeys, blowing them away.
    //
    f = TRUE;
    x = 0;
    while (f) {
        dwTmpSize = MAX_PATH;
        l = RegEnumKeyEx(hk, x, szTmp, &dwTmpSize, 0, NULL, NULL, NULL);
        if (l != ERROR_SUCCESS) 
            break;
        f = DeleteKeyAndSubKeys(hk, szTmp);
        x++;
    }

    // there are no subkeys left, [or we'll just generate an error and return FALSE].
    // let's go blow this dude away.
    //
    RegCloseKey(hk);
    l = RegDeleteKey(hkIn, pszSubKey);

    return (l == ERROR_SUCCESS) ? TRUE : FALSE;
}


#define INPROCSERVER32  TEXT("InProcServer32")
#define CLSID           TEXT("CLSID")
#define THREADINGMODEL  TEXT("ThreadingModel")
#define APARTMENT       TEXT("Apartment")
#define APPROVED        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved")
#define DESC            TEXT("Sendmail service")

STDAPI DllRegisterServer(void)
{
    const OBJ_ENTRY *pcls;
    TCHAR szPath[MAX_PATH];

    GetModuleFileName(g_hinst, szPath, ARRAYSIZE(szPath));  // get path to this DLL

    for (pcls = c_clsmap; pcls->pclsid; pcls++)
    {
        HKEY hkCLSID;
        if (RegOpenKey(HKEY_CLASSES_ROOT, CLSID, &hkCLSID) == ERROR_SUCCESS) 
        {
            HKEY hkOurs;
            LONG err;
            TCHAR szGUID[80];

            StringFromGUID(pcls->pclsid, szGUID);

            err = RegCreateKey(hkCLSID, szGUID, &hkOurs);
            if (err == ERROR_SUCCESS) 
            {
                HKEY hkInproc;
                err = RegCreateKey(hkOurs, INPROCSERVER32, &hkInproc);
                if (err == ERROR_SUCCESS) 
                {
                    err = RegSetValueEx(hkInproc, NULL, 0, REG_SZ, (LPBYTE)szPath, (lstrlen(szPath) + 1) * sizeof(TCHAR));
                    if (err == ERROR_SUCCESS) 
                    {
                        err = RegSetValueEx(hkInproc, THREADINGMODEL, 0, REG_SZ, (LPBYTE)APARTMENT, sizeof(APARTMENT));
                    }
                    RegCloseKey(hkInproc);
                }

                if (pcls->pfnRegUnReg)
                    pcls->pfnRegUnReg(TRUE, hkOurs, szGUID, szPath);

                if (err == ERROR_SUCCESS)
                {   
                    HKEY hkApproved;
                    
                    err = RegOpenKey(HKEY_LOCAL_MACHINE, APPROVED, &hkApproved);
                    if (err == ERROR_SUCCESS)
                    {
                        err = RegSetValueEx(hkApproved, szGUID, 0, REG_SZ, (LPBYTE)DESC, sizeof(DESC));
                        RegCloseKey(hkApproved);
                    }
                }
                RegCloseKey(hkOurs);
            }
            RegCloseKey(hkCLSID);

            if (err != ERROR_SUCCESS)
                return HRESULT_FROM_WIN32(err);
        }
    }
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    const OBJ_ENTRY *pcls;
    for (pcls = c_clsmap; pcls->pclsid; pcls++)
    {
        HKEY hkCLSID;
        if (RegOpenKey(HKEY_CLASSES_ROOT, CLSID, &hkCLSID) == ERROR_SUCCESS) 
        {
            TCHAR szGUID[80];
            HKEY  hkApproved;

            StringFromGUID(pcls->pclsid, szGUID);

            DeleteKeyAndSubKeys(hkCLSID, szGUID);
            if (RegOpenKey(HKEY_LOCAL_MACHINE, APPROVED, &hkApproved) == ERROR_SUCCESS)
            {
                RegDeleteValue(hkApproved, szGUID);
                RegCloseKey(hkApproved);
            }

            RegCloseKey(hkCLSID);

            if (pcls->pfnRegUnReg)
                pcls->pfnRegUnReg(FALSE, NULL, szGUID, NULL);

        }
    }
    return S_OK;
}
