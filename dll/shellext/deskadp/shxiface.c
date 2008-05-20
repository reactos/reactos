#include "precomp.h"

#define NDEBUG
#include <debug.h>

LONG dll_refs = 0;

static HRESULT STDMETHODCALLTYPE
IDeskDisplayAdapter_IShellPropSheetExt_QueryInterface(IShellPropSheetExt *iface,
                                                      REFIID iid,
                                                      PVOID *pvObject)
{
    PDESKDISPLAYADAPTER This = interface_to_impl(iface, IShellPropSheetExt);
    return IDeskDisplayAdapter_QueryInterface(This,
                                              iid,
                                              pvObject);
}

static ULONG STDMETHODCALLTYPE
IDeskDisplayAdapter_IShellPropSheetExt_AddRef(IShellPropSheetExt* iface)
{
    PDESKDISPLAYADAPTER This = interface_to_impl(iface, IShellPropSheetExt);
    return IDeskDisplayAdapter_AddRef(This);
}

static ULONG STDMETHODCALLTYPE
IDeskDisplayAdapter_IShellPropSheetExt_Release(IShellPropSheetExt* iface)
{
    PDESKDISPLAYADAPTER This = interface_to_impl(iface, IShellPropSheetExt);
    return IDeskDisplayAdapter_Release(This);
}

static HRESULT STDMETHODCALLTYPE
IDeskDisplayAdapter_IShellPropSheetExt_AddPages(IShellPropSheetExt* iface,
                                                LPFNADDPROPSHEETPAGE pfnAddPage,
                                                LPARAM lParam)
{
    PDESKDISPLAYADAPTER This = interface_to_impl(iface, IShellPropSheetExt);
    return IDeskDisplayAdapter_AddPages(This,
                                        pfnAddPage,
                                        lParam);
}

static HRESULT STDMETHODCALLTYPE
IDeskDisplayAdapter_IShellPropSheetExt_ReplacePage(IShellPropSheetExt* iface,
                                                   EXPPS uPageID,
                                                   LPFNADDPROPSHEETPAGE pfnReplacePage,
                                                   LPARAM lParam)
{
    PDESKDISPLAYADAPTER This = interface_to_impl(iface, IShellPropSheetExt);
    return IDeskDisplayAdapter_ReplacePage(This,
                                           uPageID,
                                           pfnReplacePage,
                                           lParam);
}

static IShellPropSheetExtVtbl efvtIShellPropSheetExt =
{
    IDeskDisplayAdapter_IShellPropSheetExt_QueryInterface,
    IDeskDisplayAdapter_IShellPropSheetExt_AddRef,
    IDeskDisplayAdapter_IShellPropSheetExt_Release,
    IDeskDisplayAdapter_IShellPropSheetExt_AddPages,
    IDeskDisplayAdapter_IShellPropSheetExt_ReplacePage
};

static HRESULT STDMETHODCALLTYPE
IDeskDisplayAdapter_IShellExtInit_QueryInterface(IShellExtInit *iface,
                                                 REFIID iid,
                                                 PVOID *pvObject)
{
    PDESKDISPLAYADAPTER This = interface_to_impl(iface, IShellExtInit);
    return IDeskDisplayAdapter_QueryInterface(This,
                                              iid,
                                              pvObject);
}

static ULONG STDMETHODCALLTYPE
IDeskDisplayAdapter_IShellExtInit_AddRef(IShellExtInit* iface)
{
    PDESKDISPLAYADAPTER This = interface_to_impl(iface, IShellExtInit);
    return IDeskDisplayAdapter_AddRef(This);
}

static ULONG STDMETHODCALLTYPE
IDeskDisplayAdapter_IShellExtInit_Release(IShellExtInit* iface)
{
    PDESKDISPLAYADAPTER This = interface_to_impl(iface, IShellExtInit);
    return IDeskDisplayAdapter_Release(This);
}

static HRESULT STDMETHODCALLTYPE
IDeskDisplayAdapter_IShellExtInit_Initialize(IShellExtInit* iface,
                                             LPCITEMIDLIST pidlFolder,
                                             IDataObject *pdtobj,
                                             HKEY hkeyProgID)
{
    PDESKDISPLAYADAPTER This = interface_to_impl(iface, IShellExtInit);
    return IDeskDisplayAdapter_Initialize(This,
                                          pidlFolder,
                                          pdtobj,
                                          hkeyProgID);
}

static IShellExtInitVtbl efvtIShellExtInit =
{
    IDeskDisplayAdapter_IShellExtInit_QueryInterface,
    IDeskDisplayAdapter_IShellExtInit_AddRef,
    IDeskDisplayAdapter_IShellExtInit_Release,
    IDeskDisplayAdapter_IShellExtInit_Initialize
};

static HRESULT STDMETHODCALLTYPE
IDeskDisplayAdapter_IClassFactory_QueryInterface(IClassFactory *iface,
                                                 REFIID iid,
                                                 PVOID *pvObject)
{
    PDESKDISPLAYADAPTER This = interface_to_impl(iface, IClassFactory);
    return IDeskDisplayAdapter_QueryInterface(This,
                                              iid,
                                              pvObject);
}

static ULONG STDMETHODCALLTYPE
IDeskDisplayAdapter_IClassFactory_AddRef(IClassFactory* iface)
{
    PDESKDISPLAYADAPTER This = interface_to_impl(iface, IClassFactory);
    return IDeskDisplayAdapter_AddRef(This);
}

static ULONG STDMETHODCALLTYPE
IDeskDisplayAdapter_IClassFactory_Release(IClassFactory* iface)
{
    PDESKDISPLAYADAPTER This = interface_to_impl(iface, IClassFactory);
    return IDeskDisplayAdapter_Release(This);
}

static HRESULT STDMETHODCALLTYPE
IDeskDisplayAdapter_IClassFactory_CreateInstance(IClassFactory *iface,
                                                 IUnknown * pUnkOuter,
                                                 REFIID riid,
                                                 PVOID *ppvObject)
{
    if (pUnkOuter != NULL &&
        !IsEqualIID(riid,
                    &IID_IUnknown))
    {
        return CLASS_E_NOAGGREGATION;
    }

    return IDeskDisplayAdapter_Constructor(riid,
                                           ppvObject);
}

static HRESULT STDMETHODCALLTYPE
IDeskDisplayAdapter_IClassFactory_LockServer(IClassFactory *iface,
                                             BOOL fLock)
{
    if (fLock)
        InterlockedIncrement(&dll_refs);
    else
        InterlockedDecrement(&dll_refs);

    return S_OK;
}

static IClassFactoryVtbl efvtIClassFactory =
{
    IDeskDisplayAdapter_IClassFactory_QueryInterface,
    IDeskDisplayAdapter_IClassFactory_AddRef,
    IDeskDisplayAdapter_IClassFactory_Release,
    IDeskDisplayAdapter_IClassFactory_CreateInstance,
    IDeskDisplayAdapter_IClassFactory_LockServer,
};

VOID
IDeskDisplayAdapter_InitIface(PDESKDISPLAYADAPTER This)
{
    This->lpIShellPropSheetExtVtbl = &efvtIShellPropSheetExt;
    This->lpIShellExtInitVtbl = &efvtIShellExtInit;
    This->lpIClassFactoryVtbl = &efvtIClassFactory;

    IDeskDisplayAdapter_AddRef(This);
}

HRESULT WINAPI
DllGetClassObject(REFCLSID rclsid,
                  REFIID riid,
                  LPVOID *ppv)
{
    if (ppv == NULL)
        return E_INVALIDARG;

    *ppv = NULL;
    if (IsEqualCLSID(rclsid,
                     &CLSID_IDeskDisplayAdapter))
    {
        return IDeskDisplayAdapter_Constructor(riid,
                                               ppv);
    }

    DPRINT1("DllGetClassObject: CLASS_E_CLASSNOTAVAILABLE\n");
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI
DllCanUnloadNow(VOID)
{
    return dll_refs == 0 ? S_OK : S_FALSE;
}
