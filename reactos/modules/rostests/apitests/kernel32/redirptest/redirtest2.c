
#include <windef.h>
#include <winbase.h>

DWORD WINAPI GetVersion()
{
    return 2;
}

BOOL
WINAPI
DllMain(HINSTANCE hinstDll,
        DWORD dwReason,
        LPVOID reserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDll);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

   return TRUE;
}
