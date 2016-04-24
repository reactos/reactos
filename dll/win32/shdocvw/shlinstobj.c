/* 
 * Shell Instance Objects - Add hot water and stir until dissolved.
 *
 * Copyright 2005 Michael Jung
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

/* 'Shell Instance Objects' allow you to add a node to the shell namespace
 * (typically a shortcut to some location in the filesystem), just by setting
 * some registry entries. This feature was introduced with win2k. Please
 * search for 'Shell Instance Objects' on MSDN to get more information. */

#include "shdocvw.h"

#define CHARS_IN_GUID 39

/******************************************************************************
 * RegistryPropertyBag 
 *
 * Gives access to a registry key's values via the IPropertyBag interface.
 */
typedef struct _RegistryPropertyBag {
    IPropertyBag           IPropertyBag_iface;
    LONG                   m_cRef;
    HKEY                   m_hInitPropertyBagKey;
} RegistryPropertyBag;

static inline RegistryPropertyBag *impl_from_IPropertyBag(IPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, RegistryPropertyBag, IPropertyBag_iface);
}

static HRESULT WINAPI RegistryPropertyBag_IPropertyBag_QueryInterface(IPropertyBag *iface,
    REFIID riid, void **ppv)
{
    RegistryPropertyBag *This = impl_from_IPropertyBag(iface);

    TRACE("(iface=%p, riid=%s, ppv=%p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IPropertyBag, riid)) {
        *ppv = &This->IPropertyBag_iface;
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI RegistryPropertyBag_IPropertyBag_AddRef(IPropertyBag *iface)
{
    RegistryPropertyBag *This = impl_from_IPropertyBag(iface);
    ULONG cRef;

    TRACE("(iface=%p)\n", iface);

    cRef = InterlockedIncrement(&This->m_cRef);

    if (cRef == 1)
        SHDOCVW_LockModule();

    return cRef;
}

static ULONG WINAPI RegistryPropertyBag_IPropertyBag_Release(IPropertyBag *iface)
{
    RegistryPropertyBag *This = impl_from_IPropertyBag(iface);
    ULONG cRef;

    TRACE("(iface=%p)\n", iface);

    cRef = InterlockedDecrement(&This->m_cRef);

    if (cRef == 0) {
        TRACE("Destroying This=%p)\n", This);
        RegCloseKey(This->m_hInitPropertyBagKey);
        heap_free(This);
        SHDOCVW_UnlockModule();
    }

    return cRef;
}

static HRESULT WINAPI RegistryPropertyBag_IPropertyBag_Read(IPropertyBag *iface,
    LPCOLESTR pwszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
    RegistryPropertyBag *This = impl_from_IPropertyBag(iface);
    WCHAR *pwszValue;
    DWORD dwType, cbData;
    LONG res;
    VARTYPE vtDst = V_VT(pVar);
    HRESULT hr = S_OK;

    TRACE("(iface=%p, pwszPropName=%s, pVar=%p, pErrorLog=%p)\n", iface, debugstr_w(pwszPropName), 
          pVar, pErrorLog);

    res = RegQueryValueExW(This->m_hInitPropertyBagKey, pwszPropName, NULL, &dwType, NULL, &cbData);
    if (res != ERROR_SUCCESS) 
        return E_INVALIDARG;

    pwszValue = heap_alloc(cbData);
    if (!pwszValue)
        return E_OUTOFMEMORY;
 
    res = RegQueryValueExW(This->m_hInitPropertyBagKey, pwszPropName, NULL, &dwType, 
                           (LPBYTE)pwszValue, &cbData);
    if (res != ERROR_SUCCESS) {
        heap_free(pwszValue);
        return E_INVALIDARG;
    }

    V_VT(pVar) = VT_BSTR;
    V_BSTR(pVar) = SysAllocString(pwszValue);
    heap_free(pwszValue);

    if (vtDst != VT_BSTR) {
        hr = VariantChangeTypeEx(pVar, pVar, LOCALE_SYSTEM_DEFAULT, 0, vtDst);
        if (FAILED(hr))
            SysFreeString(V_BSTR(pVar));
    }

    return hr;    
}

static HRESULT WINAPI RegistryPropertyBag_IPropertyBag_Write(IPropertyBag *iface, 
    LPCOLESTR pwszPropName, VARIANT *pVar)
{
    FIXME("(iface=%p, pwszPropName=%s, pVar=%p) stub\n", iface, debugstr_w(pwszPropName), pVar);
    return E_NOTIMPL;
}

static const IPropertyBagVtbl RegistryPropertyBag_IPropertyBagVtbl = {
    RegistryPropertyBag_IPropertyBag_QueryInterface,
    RegistryPropertyBag_IPropertyBag_AddRef,
    RegistryPropertyBag_IPropertyBag_Release,
    RegistryPropertyBag_IPropertyBag_Read,
    RegistryPropertyBag_IPropertyBag_Write
};

static HRESULT RegistryPropertyBag_Constructor(HKEY hInitPropertyBagKey, REFIID riid, LPVOID *ppvObject) {
    HRESULT hr = E_FAIL;
    RegistryPropertyBag *pRegistryPropertyBag;

    TRACE("(hInitPropertyBagKey=%p, riid=%s, ppvObject=%p)\n", hInitPropertyBagKey, 
        debugstr_guid(riid), ppvObject);
    
    pRegistryPropertyBag = heap_alloc(sizeof(RegistryPropertyBag));
    if (pRegistryPropertyBag) {
        pRegistryPropertyBag->IPropertyBag_iface.lpVtbl = &RegistryPropertyBag_IPropertyBagVtbl;
        pRegistryPropertyBag->m_cRef = 0;
        pRegistryPropertyBag->m_hInitPropertyBagKey = hInitPropertyBagKey;

        /* The clasping AddRef/Release is for the case that QueryInterface fails, which will result
         * in a reference count of 0 in the Release call, which will result in object destruction.*/
        IPropertyBag_AddRef(&pRegistryPropertyBag->IPropertyBag_iface);
        hr = IPropertyBag_QueryInterface(&pRegistryPropertyBag->IPropertyBag_iface, riid, ppvObject);
        IPropertyBag_Release(&pRegistryPropertyBag->IPropertyBag_iface);
    }

    return hr;
}

/******************************************************************************
 * InstanceObjectFactory
 * Builds Instance Objects and asks them to initialize themselves based on the
 * values of a PropertyBag.
 */
typedef struct _InstanceObjectFactory {
    IClassFactory           IClassFactory_iface;
    LONG                    m_cRef;
    CLSID                   m_clsidInstance; /* CLSID of the objects to create. */
    IPropertyBag            *m_pPropertyBag; /* PropertyBag to initialize those objects. */
} InstanceObjectFactory;

static inline InstanceObjectFactory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, InstanceObjectFactory, IClassFactory_iface);
}

static HRESULT WINAPI InstanceObjectFactory_IClassFactory_QueryInterface(IClassFactory *iface,
    REFIID riid, void **ppv)
{
    InstanceObjectFactory *This = impl_from_IClassFactory(iface);

    TRACE("iface=%p, riid=%s, ppv=%p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IClassFactory, riid)) {
        *ppv = &This->IClassFactory_iface;
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}
    
static ULONG WINAPI InstanceObjectFactory_IClassFactory_AddRef(IClassFactory *iface)
{
    InstanceObjectFactory *This = impl_from_IClassFactory(iface);
    ULONG cRef;

    TRACE("(iface=%p)\n", iface);

    cRef = InterlockedIncrement(&This->m_cRef);

    if (cRef == 1)
        IClassFactory_LockServer(iface, TRUE);

    return cRef;
}

static ULONG WINAPI InstanceObjectFactory_IClassFactory_Release(IClassFactory *iface)
{
    InstanceObjectFactory *This = impl_from_IClassFactory(iface);
    ULONG cRef;

    TRACE("(iface=%p)\n", iface);

    cRef = InterlockedDecrement(&This->m_cRef);

    if (cRef == 0) {
        IClassFactory_LockServer(iface, FALSE);
        IPropertyBag_Release(This->m_pPropertyBag);
        heap_free(This);
    }

    return cRef;
}

static HRESULT WINAPI InstanceObjectFactory_IClassFactory_CreateInstance(IClassFactory *iface,
    IUnknown *pUnkOuter, REFIID riid, LPVOID *ppvObj)
{
    InstanceObjectFactory *This = impl_from_IClassFactory(iface);
    IPersistPropertyBag *pPersistPropertyBag;
    HRESULT hr;

    TRACE("(pUnkOuter=%p, riid=%s, ppvObj=%p)\n", pUnkOuter, debugstr_guid(riid), ppvObj);

    hr = CoCreateInstance(&This->m_clsidInstance, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IPersistPropertyBag, (LPVOID*)&pPersistPropertyBag);
    if (FAILED(hr)) {
        TRACE("Failed to create instance of %s. hr = %08x\n",
              debugstr_guid(&This->m_clsidInstance), hr);
        return hr;
    }

    hr = IPersistPropertyBag_Load(pPersistPropertyBag, This->m_pPropertyBag, NULL);
    if (FAILED(hr)) {
        TRACE("Failed to initialize object from PropertyBag: hr = %08x\n", hr);
        IPersistPropertyBag_Release(pPersistPropertyBag);
        return hr;
    }

    hr = IPersistPropertyBag_QueryInterface(pPersistPropertyBag, riid, ppvObj);
    IPersistPropertyBag_Release(pPersistPropertyBag);

    return hr;
}

static HRESULT WINAPI InstanceObjectFactory_IClassFactory_LockServer(IClassFactory *iface, 
    BOOL fLock)
{
    TRACE("(iface=%p, fLock=%d) stub\n", iface, fLock);

    if (fLock)
        SHDOCVW_LockModule();
    else
        SHDOCVW_UnlockModule();

    return S_OK;        
}

static const IClassFactoryVtbl InstanceObjectFactory_IClassFactoryVtbl = {
    InstanceObjectFactory_IClassFactory_QueryInterface,
    InstanceObjectFactory_IClassFactory_AddRef,
    InstanceObjectFactory_IClassFactory_Release,
    InstanceObjectFactory_IClassFactory_CreateInstance,
    InstanceObjectFactory_IClassFactory_LockServer
};

static HRESULT InstanceObjectFactory_Constructor(REFCLSID rclsid, IPropertyBag *pPropertyBag,
                                                 REFIID riid, LPVOID *ppvObject)
{
    InstanceObjectFactory *pInstanceObjectFactory;
    HRESULT hr = E_FAIL;

    TRACE("(RegistryPropertyBag=%p, riid=%s, ppvObject=%p)\n", pPropertyBag,
        debugstr_guid(riid), ppvObject);

    pInstanceObjectFactory = heap_alloc(sizeof(InstanceObjectFactory));
    if (pInstanceObjectFactory) {
        pInstanceObjectFactory->IClassFactory_iface.lpVtbl = &InstanceObjectFactory_IClassFactoryVtbl;
        pInstanceObjectFactory->m_cRef = 0;
        pInstanceObjectFactory->m_clsidInstance = *rclsid;
        pInstanceObjectFactory->m_pPropertyBag = pPropertyBag;
        IPropertyBag_AddRef(pPropertyBag);

        IClassFactory_AddRef(&pInstanceObjectFactory->IClassFactory_iface);
        hr = IClassFactory_QueryInterface(&pInstanceObjectFactory->IClassFactory_iface,
                                          riid, ppvObject);
        IClassFactory_Release(&pInstanceObjectFactory->IClassFactory_iface);
    }

    return hr;
}

/******************************************************************************
 * SHDOCVW_GetShellInstanceObjectClassObject [Internal]
 *
 *  Figure if there is a 'Shell Instance Object' conformant registry entry for
 *  the given CLSID and if so create and return a corresponding ClassObject.
 *
 * PARAMS
 *  rclsid      [I] CLSID of the 'Shell Instance Object'.
 *  riid        [I] Desired interface. Only IClassFactory supported.
 *  ppvClassObj [O] The corresponding ClassObject.
 *
 * RETURNS
 *  Success: S_OK,
 *  Failure: CLASS_E_CLASSNOTAVAILABLE
 */
HRESULT SHDOCVW_GetShellInstanceObjectClassObject(REFCLSID rclsid, REFIID riid, 
    LPVOID *ppvClassObj)
{
    WCHAR wszInstanceKey[] = { 'C','L','S','I','D','\\','{','0','0','0','0','0','0','0','0','-',
        '0','0','0','0','-','0','0','0','0','-','0','0','0','0','-','0','0','0','0','0','0','0','0',
        '0','0','0','0','}','\\','I','n','s','t','a','n','c','e', 0 };
    const WCHAR wszCLSID[] = { 'C','L','S','I','D',0 };
    const WCHAR wszInitPropertyBag[] = 
        { 'I','n','i','t','P','r','o','p','e','r','t','y','B','a','g',0 };
    WCHAR wszCLSIDInstance[CHARS_IN_GUID];
    CLSID clsidInstance;
    HKEY hInstanceKey, hInitPropertyBagKey;
    DWORD dwType, cbBytes = sizeof(wszCLSIDInstance);
    IPropertyBag *pInitPropertyBag;
    HRESULT hr;
    LONG res;
        
    TRACE("(rclsid=%s, riid=%s, ppvClassObject=%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), 
          ppvClassObj);

    /* Figure if there is an 'Instance' subkey for the given CLSID and acquire a handle. */
    if (!StringFromGUID2(rclsid, wszInstanceKey + 6, CHARS_IN_GUID))
        return CLASS_E_CLASSNOTAVAILABLE;
    wszInstanceKey[5+CHARS_IN_GUID] = '\\'; /* Repair the null-termination. */
    if (ERROR_SUCCESS != RegOpenKeyExW(HKEY_CLASSES_ROOT, wszInstanceKey, 0, KEY_READ, &hInstanceKey))
        /* If there is no 'Instance' subkey, then it's not a Shell Instance Object. */
        return CLASS_E_CLASSNOTAVAILABLE;

    if (ERROR_SUCCESS != RegQueryValueExW(hInstanceKey, wszCLSID, NULL, &dwType, (LPBYTE)wszCLSIDInstance, &cbBytes) ||
        FAILED(CLSIDFromString(wszCLSIDInstance, &clsidInstance)))
    {
        /* 'Instance' should have a 'CLSID' value with a well-formed clsid-string. */
        FIXME("Failed to infer instance CLSID! %s\n", debugstr_w(wszCLSIDInstance));
        RegCloseKey(hInstanceKey);
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    /* Try to open the 'InitPropertyBag' subkey. */
    res = RegOpenKeyExW(hInstanceKey, wszInitPropertyBag, 0, KEY_READ, &hInitPropertyBagKey);
    RegCloseKey(hInstanceKey);
    if (res != ERROR_SUCCESS) {
        /* Besides 'InitPropertyBag's, shell instance objects might be initialized by streams.
         * So this case might not be an error. */
        TRACE("No InitPropertyBag key found!\n");
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    /* If the construction succeeds, the new RegistryPropertyBag is responsible for closing
     * hInitPropertyBagKey. */
    hr = RegistryPropertyBag_Constructor(hInitPropertyBagKey, &IID_IPropertyBag,
                                         (LPVOID*)&pInitPropertyBag);
    if (FAILED(hr)) {
        RegCloseKey(hInitPropertyBagKey);
        return hr;
    }

    /* Construct an Instance Object Factory, which creates objects of class 'clsidInstance'
     * and asks them to initialize themselves with the help of the 'pInitiPropertyBag' */
    hr = InstanceObjectFactory_Constructor(&clsidInstance, pInitPropertyBag, riid, ppvClassObj);
    IPropertyBag_Release(pInitPropertyBag); /* The factory will hold a reference the bag. */
        
    return hr;
}
