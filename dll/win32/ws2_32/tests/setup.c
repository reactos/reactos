#include <windows.h>
#include "regtests.h"

extern BOOL
WINAPI
DllMain(HANDLE hInstDll,
        ULONG dwReason,
        LPVOID lpReserved);

_SetupOnce()
{
  DllMain(NULL, DLL_PROCESS_ATTACH, NULL);
}
