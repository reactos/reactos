#include <vdmdbg.h>

#define NDEBUG
#include <debug.h>

HINSTANCE hDllInstance;

BOOL WINAPI VDMBreakThread( HANDLE hProcess,
                            HANDLE hThread )
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL WINAPI VDMDetectWOW( void )
{
  UNIMPLEMENTED;
  return FALSE;
}

INT WINAPI VDMEnumProcessWOW( PROCESSENUMPROC fp,
                              LPARAM lparam )
{
  UNIMPLEMENTED;
  return FALSE;
}


INT WINAPI VDMEnumTaskWOWEx( DWORD dwProcessId,
                             TASKENUMPROCEX  fp,
                             LPARAM          lparam )
{
  UNIMPLEMENTED;
  return ERROR_CALL_NOT_IMPLEMENTED;
}

BOOL WINAPI VDMTerminateTaskWOW( DWORD dwProcessId,
                                 WORD htask )
{
  UNIMPLEMENTED;
  return FALSE; 
}

BOOL STDCALL
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hDllInstance = hinstDLL;
            DisableThreadLibraryCalls(hDllInstance);
            /* Don't break, initialize first thread */
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

