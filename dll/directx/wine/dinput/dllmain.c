#include <windef.h>

/***********************************************************************
 *      DllRegisterServer (DINPUT.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return 0;
}

/***********************************************************************
 *      DllUnregisterServer (DINPUT.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return 0;
}

/***********************************************************************
 *      DllCanUnloadNow (DINPUT.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}
