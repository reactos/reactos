#include <windows.h>

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}
