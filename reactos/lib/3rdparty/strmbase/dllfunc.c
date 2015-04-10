/*
 * Strmbase DLL functions
 *
 * Copyright (C) 2005 Rolf Kalbermatter
 * Copyright (C) 2010 Aric Stewart, CodeWeavers
*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "strmbase_private.h"

#define NO_SHLWAPI_PATH
#define NO_SHLWAPI_STRFCNS
#define NO_SHLWAPI_GDI
#define NO_SHLWAPI_STREAM
#include <shlwapi.h>

extern const int g_cTemplates;
extern const FactoryTemplate g_Templates[];

static HINSTANCE g_hInst = NULL;
static LONG server_locks = 0;

/*
 * defines and constants
 */
#define MAX_KEY_LEN  260

static const WCHAR clsid_keyname[] = {'C','L','S','I','D',0 };
static const WCHAR ips32_keyname[] = {'I','n','P','r','o','c','S','e','r','v','e','r','3','2',0};
static const WCHAR tmodel_keyname[] = {'T','h','r','e','a','d','i','n','g','M','o','d','e','l',0};
static const WCHAR tmodel_both[] = {'B','o','t','h',0};

/*
 * SetupRegisterClass()
 */
static HRESULT SetupRegisterClass(HKEY clsid, LPCWSTR szCLSID,
                                  LPCWSTR szDescription,
                                  LPCWSTR szFileName,
                                  LPCWSTR szServerType,
                                  LPCWSTR szThreadingModel)
{
    HKEY hkey, hsubkey = NULL;
    LONG ret = RegCreateKeyW(clsid, szCLSID, &hkey);
    if (ERROR_SUCCESS != ret)
        return HRESULT_FROM_WIN32(ret);

    /* set description string */
    ret = RegSetValueW(hkey, NULL, REG_SZ, szDescription,
                       sizeof(WCHAR) * (lstrlenW(szDescription) + 1));
    if (ERROR_SUCCESS != ret)
        goto err_out;

    /* create CLSID\\{"CLSID"}\\"ServerType" key, using key to CLSID\\{"CLSID"}
       passed back by last call to RegCreateKeyW(). */
    ret = RegCreateKeyW(hkey,  szServerType, &hsubkey);
    if (ERROR_SUCCESS != ret)
        goto err_out;

    /* set server path */
    ret = RegSetValueW(hsubkey, NULL, REG_SZ, szFileName,
                       sizeof(WCHAR) * (lstrlenW(szFileName) + 1));
    if (ERROR_SUCCESS != ret)
        goto err_out;

    /* set threading model */
    ret = RegSetValueExW(hsubkey, tmodel_keyname, 0L, REG_SZ,
                         (const BYTE*)szThreadingModel,
                         sizeof(WCHAR) * (lstrlenW(szThreadingModel) + 1));
err_out:
    if (hsubkey)
        RegCloseKey(hsubkey);
    RegCloseKey(hkey);
    return HRESULT_FROM_WIN32(ret);
}

/*
 * RegisterAllClasses()
 */
static HRESULT SetupRegisterAllClasses(const FactoryTemplate * pList, int num,
                                       LPCWSTR szFileName, BOOL bRegister)
{
    HRESULT hr = NOERROR;
    HKEY hkey;
    OLECHAR szCLSID[CHARS_IN_GUID];
    LONG i, ret = RegCreateKeyW(HKEY_CLASSES_ROOT, clsid_keyname, &hkey);
    if (ERROR_SUCCESS != ret)
        return HRESULT_FROM_WIN32(ret);

    for (i = 0; i < num; i++, pList++)
    {
        /* (un)register CLSID and InprocServer32 */
        hr = StringFromGUID2(pList->m_ClsID, szCLSID, CHARS_IN_GUID);
        if (SUCCEEDED(hr))
        {
            if (bRegister )
                hr = SetupRegisterClass(hkey, szCLSID,
                                        pList->m_Name, szFileName,
                                        ips32_keyname, tmodel_both);
            else
                hr = SHDeleteKeyW(hkey, szCLSID);
        }
    }
    RegCloseKey(hkey);
    return hr;
}

HRESULT WINAPI AMovieSetupRegisterFilter2(const AMOVIESETUP_FILTER *pFilter, IFilterMapper2  *pIFM2, BOOL  bRegister)
{
    if (!pFilter)
        return S_OK;

    if (bRegister)
    {
        {
            REGFILTER2 rf2;
            rf2.dwVersion = 1;
            rf2.dwMerit = pFilter->merit;
            rf2.u.s1.cPins = pFilter->pins;
            rf2.u.s1.rgPins = pFilter->pPin;

            return IFilterMapper2_RegisterFilter(pIFM2, pFilter->clsid, pFilter->name, NULL, &CLSID_LegacyAmFilterCategory, NULL, &rf2);
        }
    }
    else
       return IFilterMapper2_UnregisterFilter(pIFM2, &CLSID_LegacyAmFilterCategory, NULL, pFilter->clsid);
}

HRESULT WINAPI AMovieDllRegisterServer2(BOOL bRegister)
{
    HRESULT hr;
    int i;
    IFilterMapper2 *pIFM2 = NULL;
    WCHAR szFileName[MAX_PATH];

    if (!GetModuleFileNameW(g_hInst, szFileName, MAX_PATH))
    {
        ERR("Failed to get module file name for registration\n");
        return E_FAIL;
    }

    if (bRegister)
        hr = SetupRegisterAllClasses(g_Templates, g_cTemplates, szFileName, TRUE );

    CoInitialize(NULL);

    TRACE("Getting IFilterMapper2\r\n");
    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFilterMapper2, (void **)&pIFM2);

    for (i = 0; SUCCEEDED(hr) && i < g_cTemplates; i++)
        hr = AMovieSetupRegisterFilter2(g_Templates[i].m_pAMovieSetup_Filter, pIFM2, bRegister);

    /* release interface */
    if (pIFM2)
        IFilterMapper2_Release(pIFM2);

    /* and clear up */
    CoFreeUnusedLibraries();
    CoUninitialize();

    /* if unregistering, unregister all OLE servers */
    if (SUCCEEDED(hr) && !bRegister)
        hr = SetupRegisterAllClasses(g_Templates, g_cTemplates, szFileName, FALSE);

    return hr;
}

/****************************************************************************
 * SetupInitializeServers
 *
 * This function is table driven using the static members of the
 * CFactoryTemplate class defined in the Dll.
 *
 * It calls the initialize function for any class in CFactoryTemplate with
 * one defined.
 *
 ****************************************************************************/
static void SetupInitializeServers(const FactoryTemplate * pList, int num,
                            BOOL bLoading)
{
    int i;

    for (i = 0; i < num; i++, pList++)
    {
        if (pList->m_lpfnInit)
            pList->m_lpfnInit(bLoading, pList->m_ClsID);
    }
}

BOOL WINAPI STRMBASE_DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hInst = hInstDLL;
            DisableThreadLibraryCalls(hInstDLL);
            SetupInitializeServers(g_Templates, g_cTemplates, TRUE);
            break;
        case DLL_PROCESS_DETACH:
            SetupInitializeServers(g_Templates, g_cTemplates, FALSE);
            break;
    }
    return TRUE;
}

/******************************************************************************
 * DLL ClassFactory
 */
typedef struct {
    IClassFactory IClassFactory_iface;
    LONG ref;
    LPFNNewCOMObject pfnCreateInstance;
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI DSCF_QueryInterface(IClassFactory *iface, REFIID riid, void **ppobj)
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *ppobj = iface;
        return S_OK;
    }

    *ppobj = NULL;
    WARN("(%p)->(%s,%p), not found\n", iface, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI DSCF_AddRef(IClassFactory *iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI DSCF_Release(IClassFactory *iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI DSCF_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
                                          REFIID riid, void **ppobj)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    HRESULT hres = ERROR_SUCCESS;
    LPUNKNOWN punk;

    TRACE("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);

    if (!ppobj)
        return E_POINTER;

    /* Enforce the normal OLE rules regarding interfaces and delegation */
    if (pOuter && !IsEqualGUID(riid, &IID_IUnknown))
        return E_NOINTERFACE;

    *ppobj = NULL;
    punk = This->pfnCreateInstance(pOuter, &hres);
    if (!punk)
    {
        /* No object created, update error if it isn't done already and return */
        if (SUCCEEDED(hres))
            hres = E_OUTOFMEMORY;
    return hres;
    }

    if (SUCCEEDED(hres))
    {
        hres = IUnknown_QueryInterface(punk, riid, ppobj);
    }
    /* Releasing the object. If everything was successful, QueryInterface
       should have incremented the refcount once more, otherwise this will
       purge the object. */
    IUnknown_Release(punk);
    return hres;
}

static HRESULT WINAPI DSCF_LockServer(IClassFactory *iface, BOOL dolock)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    TRACE("(%p)->(%d)\n",This, dolock);

    if (dolock)
        InterlockedIncrement(&server_locks);
    else
        InterlockedDecrement(&server_locks);
    return S_OK;
}

static const IClassFactoryVtbl DSCF_Vtbl =
{
    DSCF_QueryInterface,
    DSCF_AddRef,
    DSCF_Release,
    DSCF_CreateInstance,
    DSCF_LockServer
};

/***********************************************************************
 *    DllGetClassObject
 */
HRESULT WINAPI STRMBASE_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    const FactoryTemplate *pList = g_Templates;
    IClassFactoryImpl *factory;
    int i;

    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    *ppv = NULL;

    if (!IsEqualGUID(&IID_IClassFactory, riid) &&
        !IsEqualGUID(&IID_IUnknown, riid))
        return E_NOINTERFACE;

    for (i = 0; i < g_cTemplates; i++, pList++)
    {
        if (IsEqualGUID(pList->m_ClsID, rclsid))
            break;
    }

    if (i == g_cTemplates)
    {
        char dllname[MAX_PATH];
        if (!GetModuleFileNameA(g_hInst, dllname, sizeof(dllname)))
            strcpy(dllname, "???");
        ERR("%s: no class found in %s.\n", debugstr_guid(rclsid), dllname);
        return CLASS_E_CLASSNOTAVAILABLE;
    }
    else if (!pList->m_lpfnNew)
    {
        FIXME("%s: class not implemented yet.\n", debugstr_guid(rclsid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    factory = HeapAlloc(GetProcessHeap(), 0, sizeof(IClassFactoryImpl));
    if (!factory)
        return E_OUTOFMEMORY;

    factory->IClassFactory_iface.lpVtbl = &DSCF_Vtbl;
    factory->ref = 1;

    factory->pfnCreateInstance = pList->m_lpfnNew;

    *ppv = &factory->IClassFactory_iface;
    return S_OK;
}

/***********************************************************************
 *    DllCanUnloadNow
 */
HRESULT WINAPI STRMBASE_DllCanUnloadNow(void)
{
    TRACE("\n");

    if (server_locks == 0)
        return S_OK;
    return S_FALSE;
}
