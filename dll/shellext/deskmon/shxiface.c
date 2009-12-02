#include "precomp.h"

#define NDEBUG
#include <debug.h>

LONG dll_refs = 0;

static HRESULT STDMETHODCALLTYPE
IDeskMonitor_IShellPropSheetExt_QueryInterface(IShellPropSheetExt *iface,
                                               REFIID iid,
                                               PVOID *pvObject)
{
    PDESKMONITOR This = interface_to_impl(iface, IShellPropSheetExt);
    return IDeskMonitor_QueryInterface(This,
                                       iid,
                                       pvObject);
}

static ULONG STDMETHODCALLTYPE
IDeskMonitor_IShellPropSheetExt_AddRef(IShellPropSheetExt* iface)
{
    PDESKMONITOR This = interface_to_impl(iface, IShellPropSheetExt);
    return IDeskMonitor_AddRef(This);
}

static ULONG STDMETHODCALLTYPE
IDeskMonitor_IShellPropSheetExt_Release(IShellPropSheetExt* iface)
{
    PDESKMONITOR This = interface_to_impl(iface, IShellPropSheetExt);
    return IDeskMonitor_Release(This);
}

static HRESULT STDMETHODCALLTYPE
IDeskMonitor_IShellPropSheetExt_AddPages(IShellPropSheetExt* iface,
                                         LPFNADDPROPSHEETPAGE pfnAddPage,
                                         LPARAM lParam)
{
    PDESKMONITOR This = interface_to_impl(iface, IShellPropSheetExt);
    return IDeskMonitor_AddPages(This,
                                 pfnAddPage,
                                 lParam);
}

static HRESULT STDMETHODCALLTYPE
IDeskMonitor_IShellPropSheetExt_ReplacePage(IShellPropSheetExt* iface,
                                            EXPPS uPageID,
                                            LPFNADDPROPSHEETPAGE pfnReplacePage,
                                            LPARAM lParam)
{
    PDESKMONITOR This = interface_to_impl(iface, IShellPropSheetExt);
    return IDeskMonitor_ReplacePage(This,
                                    uPageID,
                                    pfnReplacePage,
                                    lParam);
}

static IShellPropSheetExtVtbl efvtIShellPropSheetExt =
{
    IDeskMonitor_IShellPropSheetExt_QueryInterface,
    IDeskMonitor_IShellPropSheetExt_AddRef,
    IDeskMonitor_IShellPropSheetExt_Release,
    IDeskMonitor_IShellPropSheetExt_AddPages,
    IDeskMonitor_IShellPropSheetExt_ReplacePage
};

static HRESULT STDMETHODCALLTYPE
IDeskMonitor_IShellExtInit_QueryInterface(IShellExtInit *iface,
                                          REFIID iid,
                                          PVOID *pvObject)
{
    PDESKMONITOR This = interface_to_impl(iface, IShellExtInit);
    return IDeskMonitor_QueryInterface(This,
                                       iid,
                                       pvObject);
}

static ULONG STDMETHODCALLTYPE
IDeskMonitor_IShellExtInit_AddRef(IShellExtInit* iface)
{
    PDESKMONITOR This = interface_to_impl(iface, IShellExtInit);
    return IDeskMonitor_AddRef(This);
}

static ULONG STDMETHODCALLTYPE
IDeskMonitor_IShellExtInit_Release(IShellExtInit* iface)
{
    PDESKMONITOR This = interface_to_impl(iface, IShellExtInit);
    return IDeskMonitor_Release(This);
}

static HRESULT STDMETHODCALLTYPE
IDeskMonitor_IShellExtInit_Initialize(IShellExtInit* iface,
                                      LPCITEMIDLIST pidlFolder,
                                      IDataObject *pdtobj,
                                      HKEY hkeyProgID)
{
    PDESKMONITOR This = interface_to_impl(iface, IShellExtInit);
    return IDeskMonitor_Initialize(This,
                                   pidlFolder,
                                   pdtobj,
                                   hkeyProgID);
}

static IShellExtInitVtbl efvtIShellExtInit =
{
    IDeskMonitor_IShellExtInit_QueryInterface,
    IDeskMonitor_IShellExtInit_AddRef,
    IDeskMonitor_IShellExtInit_Release,
    IDeskMonitor_IShellExtInit_Initialize
};

static HRESULT STDMETHODCALLTYPE
IDeskMonitor_IClassFactory_QueryInterface(IClassFactory *iface,
                                          REFIID iid,
                                          PVOID *pvObject)
{
    PDESKMONITOR This = interface_to_impl(iface, IClassFactory);
    return IDeskMonitor_QueryInterface(This,
                                       iid,
                                       pvObject);
}

static ULONG STDMETHODCALLTYPE
IDeskMonitor_IClassFactory_AddRef(IClassFactory* iface)
{
    PDESKMONITOR This = interface_to_impl(iface, IClassFactory);
    return IDeskMonitor_AddRef(This);
}

static ULONG STDMETHODCALLTYPE
IDeskMonitor_IClassFactory_Release(IClassFactory* iface)
{
    PDESKMONITOR This = interface_to_impl(iface, IClassFactory);
    return IDeskMonitor_Release(This);
}

static HRESULT STDMETHODCALLTYPE
IDeskMonitor_IClassFactory_CreateInstance(IClassFactory *iface,
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

    return IDeskMonitor_Constructor(riid,
                                    ppvObject);
}

static HRESULT STDMETHODCALLTYPE
IDeskMonitor_IClassFactory_LockServer(IClassFactory *iface,
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
    IDeskMonitor_IClassFactory_QueryInterface,
    IDeskMonitor_IClassFactory_AddRef,
    IDeskMonitor_IClassFactory_Release,
    IDeskMonitor_IClassFactory_CreateInstance,
    IDeskMonitor_IClassFactory_LockServer,
};

VOID
IDeskMonitor_InitIface(PDESKMONITOR This)
{
    This->lpIShellPropSheetExtVtbl = &efvtIShellPropSheetExt;
    This->lpIShellExtInitVtbl = &efvtIShellExtInit;
    This->lpIClassFactoryVtbl = &efvtIClassFactory;

    IDeskMonitor_AddRef(This);
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
                     &CLSID_IDeskMonitor))
    {
        return IDeskMonitor_Constructor(riid,
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
