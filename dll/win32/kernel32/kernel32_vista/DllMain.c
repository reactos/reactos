
#include "k32_vista.h"

/* Necessary for locale.c NT6+ build */
UNICODE_STRING BaseWindowsSystemDirectory;

/* From wine locale.c */
extern void init_locale(HMODULE module);

BOOL
WINAPI
DllMain(HANDLE hDll,
        DWORD dwReason,
        LPVOID lpReserved)
{
    /* For now, there isn't much to do */
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hDll);
        init_locale(hDll);
    }
    return TRUE;
}
