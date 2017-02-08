#include <oscalls.h>
#define _DECL_DLLMAIN
#include <process.h>

WINBOOL WINAPI DllMain (HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved)
{
    /* If the DLL provides no DllMain, then chances are that it doesn't bother with thread initialization */
    if(dwReason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(hDllHandle);
    return TRUE;
}
