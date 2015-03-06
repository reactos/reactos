#define INITGUID

#include <windef.h>
#include <comsup.h>

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
    /* Always return S_OK, since there is currently nothing that can go wrong */
    return S_OK;
}


STDAPI
DllUnregisterServer(VOID)
{
    /* Always return S_OK, since there is currently nothing that can go wrong */
    return S_OK;
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
