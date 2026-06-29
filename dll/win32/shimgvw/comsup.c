#define INITGUID

#include "shimgvw.h"
#include "comsup.h"
#include <rpcproxy.h>

LONG LockCount;
LONG ObjectCount;


VOID
DllInitServer(VOID)
{
    ObjectCount = 0;
    LockCount = 0;
}


STDAPI
DllRegisterServer(VOID)
{
    return __wine_register_resources(g_hInstance);
}


STDAPI
DllUnregisterServer(VOID)
{
    return __wine_unregister_resources(g_hInstance);
}


STDAPI
DllCanUnloadNow(VOID)
{
    if ((ObjectCount != 0) || (LockCount != 0))
    {
        return S_FALSE;
    }
    else
    {
        return S_OK;
    }
}


STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    HRESULT hr;

    /* There are no classes to export, so always return CLASS_E_CLASSNOTAVAILABLE*/
    *ppv = NULL;
    hr = CLASS_E_CLASSNOTAVAILABLE;

    return hr;
}
