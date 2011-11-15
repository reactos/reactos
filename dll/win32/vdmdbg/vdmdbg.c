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

DWORD WINAPI VDMGetDbgFlags( HANDLE hProcess )
{
  UNIMPLEMENTED;
  return ERROR_CALL_NOT_IMPLEMENTED;
}

BOOL WINAPI VDMIsModuleLoaded( LPSTR szPath )
{
  UNIMPLEMENTED;
  return FALSE;
}

ULONG WINAPI VDMGetPointer( HANDLE handle,
                            HANDLE handle2,
                            WORD   wSelector,
                            DWORD  dwOffset,
                            BOOL   fProtMode )
{
  UNIMPLEMENTED;
  return ERROR_CALL_NOT_IMPLEMENTED;
}

BOOL WINAPI VDMProcessException( LPDEBUG_EVENT   lpDebugEvent )
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL WINAPI VDMGetSegmentInfo( WORD        word,
							   ULONG       ulong,
                               BOOL        boolVal,
                               VDM_SEGINFO *pVDMSegInfo )
{
  UNIMPLEMENTED;
  return FALSE;
}


BOOL WINAPI VDMSetDbgFlags( HANDLE hProcess,
                DWORD  dwFlags )
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL WINAPI VDMDetectWOW( void )
{
  UNIMPLEMENTED;
  return FALSE;
}

INT WINAPI VDMEnumTaskWOW( DWORD dword,
                           TASKENUMPROC TaskEnumProc,
                           LPARAM       lParam )
{
  UNIMPLEMENTED;
  return ERROR_CALL_NOT_IMPLEMENTED;
}

BOOL WINAPI VDMStartTaskInWOW( DWORD dwProcessId,
                        LPSTR lpCommandLine,
                        WORD  wShow )
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL WINAPI VDMKillWOW( VOID )
{
  UNIMPLEMENTED;
  return FALSE;
}

INT WINAPI VDMEnumProcessWOW( PROCESSENUMPROC ProcessEnumProc,
                              LPARAM          lParam )
{
  UNIMPLEMENTED;
  return ERROR_CALL_NOT_IMPLEMENTED;
}


INT WINAPI VDMEnumTaskWOWEx( DWORD dwProcessId,
                             TASKENUMPROCEX  TaskEnumProcEx,
                             LPARAM          lParam )
{
  UNIMPLEMENTED;
  return ERROR_CALL_NOT_IMPLEMENTED;
}

BOOL WINAPI VDMTerminateTaskWOW( DWORD dwProcessId,
                                 WORD  hTask )
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL WINAPI VDMGetContext( HANDLE       handle,
                           HANDLE       handle2,
                           LPVDMCONTEXT lpVDMContext )
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL WINAPI VDMSetContext( HANDLE       handle,
                           HANDLE       handle2,
                           LPVDMCONTEXT lpVDMContext)
{
  UNIMPLEMENTED;
  return FALSE;
}


BOOL WINAPI VDMGetSelectorModule( HANDLE handle,
                                  HANDLE handle2,
                                  WORD   word,
                                  PUINT  punit,
                                  LPSTR  lpModuleName,
                                  UINT   uInt,
                                  LPSTR  lpstr,
                                  UINT   uInt2 )
{
  UNIMPLEMENTED;
  return FALSE;
}


BOOL WINAPI VDMGetModuleSelector( HANDLE handle,
                                  HANDLE handle2,
                                  UINT   uInt,
                                  LPSTR  lpModuleName,
                                  LPWORD lpword )
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL WINAPI VDMModuleFirst( HANDLE         handle,
						    HANDLE         handle2,
                            LPMODULEENTRY  lpModuleEntry,
                            DEBUGEVENTPROC lpDebugEventProc,
                            LPVOID         lpvoid )
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL WINAPI VDMGlobalFirst( HANDLE         handle,
                            HANDLE         handle2,
                            LPGLOBALENTRY  lpGlobalEntry,
                            WORD           word,
                            DEBUGEVENTPROC lpDebugEventProc,
                            LPVOID         lpvoid )
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL WINAPI VDMGlobalNext( HANDLE         handle,
                           HANDLE         handle2,
                           LPGLOBALENTRY  lpGlobalEntry,
                           WORD           word,
                           DEBUGEVENTPROC lpDebugEventProc,
                           LPVOID         lpvoid )
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL WINAPI VDMModuleNext( HANDLE         handle,
                           HANDLE         handle2,
                           LPMODULEENTRY  lpModuleEntry,
                           DEBUGEVENTPROC lpDebugEventProc,
                           LPVOID         lpvoid )
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL WINAPI
DllMain( IN HINSTANCE hinstDLL,
         IN DWORD     dwReason,
         IN LPVOID    lpvReserved )
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

