#include <stdarg.h>

#define WIN32_NO_STATUS

#include <windef.h>

BOOL
WINAPI
DllMain(
    HANDLE hDll,
    DWORD dwReason,
    LPVOID lpReserved)
{
    return TRUE;
}
