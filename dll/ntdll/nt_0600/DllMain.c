#include "ntdll_vista.h"

VOID
NTAPI
LdrpInitializeDllDirectorySection(VOID);

BOOL
WINAPI
DllMain(HANDLE hDll,
        DWORD dwReason,
        LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        LdrDisableThreadCalloutsForDll(hDll);
        RtlpInitializeKeyedEvent();
        LdrpInitializeDllDirectorySection();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        RtlpCloseKeyedEvent();
    }
    return TRUE;
}
