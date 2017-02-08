#include "precomp.h"

#include <olectl.h>

HINSTANCE netshell_hInstance;

extern "C"
{

/* FIXME: rpcproxy.h */
HRESULT __wine_register_resources(HMODULE module);
HRESULT __wine_unregister_resources(HMODULE module);

BOOL
WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            netshell_hInstance = hinstDLL;
            DisableThreadLibraryCalls(netshell_hInstance);
            break;
    default:
        break;
    }

    return TRUE;
}

HRESULT
WINAPI
DllCanUnloadNow(void)
{
    return S_FALSE;
}

STDAPI
DllRegisterServer(void)
{
    return __wine_register_resources(netshell_hInstance);
}

STDAPI
DllUnregisterServer(void)
{
    return __wine_unregister_resources(netshell_hInstance);
}

STDAPI
DllGetClassObject(
  REFCLSID rclsid,
  REFIID riid,
  LPVOID *ppv)
{
    if (!ppv)
        return E_INVALIDARG;

    *ppv = NULL;

    return IClassFactory_fnConstructor(rclsid, riid, ppv);
}

VOID
WINAPI
NcFreeNetconProperties(NETCON_PROPERTIES *pProps)
{
    CoTaskMemFree(pProps->pszwName);
    CoTaskMemFree(pProps->pszwDeviceName);
    CoTaskMemFree(pProps);
}

} // extern "C"
