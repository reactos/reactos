
#include <windef.h>
#include <winbase.h>

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}
