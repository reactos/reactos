/* $Id: scm.c,v 1.20 2004/04/12 17:14:54 navaraf Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/service/scm.c
 * PURPOSE:         Service control manager functions
 * PROGRAMMER:      Emanuele Aliberti
 * UPDATE HISTORY:
 *  19990413 EA created
 *  19990515 EA
 */

/* INCLUDES ******************************************************************/

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>
#include <wchar.h>
#include <tchar.h>
#include <services/scmprot.h>

//#define DBG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/**********************************************************************
 *  ChangeServiceConfigA
 *
 * @unimplemented
 */
BOOL
STDCALL
ChangeServiceConfigA(
    SC_HANDLE   hService,
    DWORD       dwServiceType,
    DWORD       dwStartType,
    DWORD       dwErrorControl,
    LPCSTR      lpBinaryPathName,
    LPCSTR      lpLoadOrderGroup,
    LPDWORD     lpdwTagId,
    LPCSTR      lpDependencies,
    LPCSTR      lpServiceStartName,
    LPCSTR      lpPassword,
    LPCSTR      lpDisplayName)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  ChangeServiceConfigW
 *
 * @unimplemented
 */
BOOL
STDCALL
ChangeServiceConfigW(
    SC_HANDLE   hService,
    DWORD       dwServiceType,
    DWORD       dwStartType,
    DWORD       dwErrorControl,
    LPCWSTR     lpBinaryPathName,
    LPCWSTR     lpLoadOrderGroup,
    LPDWORD     lpdwTagId,
    LPCWSTR     lpDependencies,
    LPCWSTR     lpServiceStartName,
    LPCWSTR     lpPassword,
    LPCWSTR     lpDisplayName)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  CloseServiceHandle
 *
 * @implemented
 */
BOOL 
STDCALL
CloseServiceHandle(SC_HANDLE hSCObject)
{
    DPRINT("CloseServiceHandle\n");

    if (!CloseHandle(hSCObject))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  ControlService
 *
 * @unimplemented
 */
BOOL
STDCALL
ControlService(SC_HANDLE        hService,
               DWORD            dwControl,
               LPSERVICE_STATUS lpServiceStatus)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  CreateServiceA
 *
 * @unimplemented
 */
SC_HANDLE
STDCALL
CreateServiceA(
    SC_HANDLE   hSCManager,
    LPCSTR      lpServiceName,
    LPCSTR      lpDisplayName,
    DWORD       dwDesiredAccess,
    DWORD       dwServiceType,
    DWORD       dwStartType,
    DWORD       dwErrorControl,
    LPCSTR      lpBinaryPathName,
    LPCSTR      lpLoadOrderGroup,
    LPDWORD     lpdwTagId,
    LPCSTR      lpDependencies,
    LPCSTR      lpServiceStartName,
    LPCSTR      lpPassword)
{
    UNICODE_STRING lpServiceNameW;
    UNICODE_STRING lpDisplayNameW;
    UNICODE_STRING lpBinaryPathNameW;
    UNICODE_STRING lpLoadOrderGroupW;
    UNICODE_STRING lpServiceStartNameW;
    UNICODE_STRING lpPasswordW;
    SC_HANDLE hService;

    RtlCreateUnicodeStringFromAsciiz(&lpServiceNameW, (LPSTR)lpServiceName);
    RtlCreateUnicodeStringFromAsciiz(&lpDisplayNameW, (LPSTR)lpDisplayName);
    RtlCreateUnicodeStringFromAsciiz(&lpBinaryPathNameW, (LPSTR)lpBinaryPathName);
    RtlCreateUnicodeStringFromAsciiz(&lpLoadOrderGroupW, (LPSTR)lpLoadOrderGroup);
    RtlCreateUnicodeStringFromAsciiz(&lpServiceStartNameW, (LPSTR)lpServiceStartName);
    RtlCreateUnicodeStringFromAsciiz(&lpPasswordW, (LPSTR)lpPassword);
    if (lpDependencies != NULL)
    {
       DPRINT1("Unimplemented case\n");
    }

    hService = CreateServiceW(
        hSCManager,
        lpServiceNameW.Buffer,
        lpDisplayNameW.Buffer,
        dwDesiredAccess,
        dwServiceType,
        dwStartType,
        dwErrorControl,
        lpBinaryPathNameW.Buffer,
        lpLoadOrderGroupW.Buffer,
        lpdwTagId,
        NULL,
        lpServiceStartNameW.Buffer,
        lpPasswordW.Buffer);

    RtlFreeUnicodeString(&lpServiceNameW);
    RtlFreeUnicodeString(&lpDisplayNameW);
    RtlFreeUnicodeString(&lpBinaryPathNameW);
    RtlFreeUnicodeString(&lpLoadOrderGroupW);
    RtlFreeUnicodeString(&lpServiceStartNameW);
    RtlFreeUnicodeString(&lpPasswordW);

    return hService;
}


/**********************************************************************
 *  CreateServiceW
 *
 * @unimplemented
 */
SC_HANDLE
STDCALL
CreateServiceW(
    SC_HANDLE   hSCManager,
    LPCWSTR     lpServiceName,
    LPCWSTR     lpDisplayName,
    DWORD       dwDesiredAccess,
    DWORD       dwServiceType,
    DWORD       dwStartType,
    DWORD       dwErrorControl,
    LPCWSTR     lpBinaryPathName,
    LPCWSTR     lpLoadOrderGroup,
    LPDWORD     lpdwTagId,
    LPCWSTR     lpDependencies,
    LPCWSTR     lpServiceStartName,
    LPCWSTR     lpPassword)
{
    SCM_CREATESERVICE_REQUEST Request;
    SCM_CREATESERVICE_REPLY Reply;
    BOOL fSuccess;
    DWORD cbWritten, cbRead;
    SC_HANDLE hService;

    DPRINT("CreateServiceW\n");

    if (lpServiceName == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    Request.RequestCode = SCM_CREATESERVICE;
    INIT_SCM_STRING(Request.ServiceName, lpServiceName);
    INIT_SCM_STRING(Request.DisplayName, lpDisplayName);
    Request.dwDesiredAccess = dwDesiredAccess;
    Request.dwServiceType = dwServiceType;
    Request.dwStartType = dwStartType;
    Request.dwErrorControl = dwErrorControl;
#if 0
    INIT_SCM_STRING(Request.BinaryPathName, lpBinaryPathName);
#else
    Request.BinaryPathName.Length = (wcslen(lpBinaryPathName) + 4) * sizeof(WCHAR);
    swprintf(Request.BinaryPathName.Buffer, L"\\??\\%s", lpBinaryPathName);
#endif
    INIT_SCM_STRING(Request.LoadOrderGroup, lpLoadOrderGroup);
    INIT_SCM_STRING(Request.ServiceStartName, lpServiceStartName);
    INIT_SCM_STRING(Request.Password, lpPassword);
    if (lpDependencies != NULL)
    {
        DWORD Length;
        for (Length = 0;
             lpDependencies[Length++];
             Length += lstrlenW(&lpDependencies[Length]) + 1)
            ;
        Request.Dependencies.Length = Length;
    }
    else
    {
        Request.Dependencies.Length = 0;
    }
    RtlCopyMemory(
        Request.Dependencies.Buffer,
        lpDependencies,
        Request.Dependencies.Length);

    fSuccess = WriteFile(
        hSCManager,       // pipe handle
        &Request,         // message
        sizeof(Request),  // message length
        &cbWritten,       // bytes written
        NULL);            // not overlapped

    if (!fSuccess || cbWritten != sizeof(Request))
    {
        DPRINT("CreateServiceW - Failed to write to pipe.\n");
        return NULL;
    }
      
    fSuccess = ReadFile(
        hSCManager,       // pipe handle
        &Reply,           // buffer to receive reply
        sizeof(Reply),    // size of buffer
        &cbRead,          // number of bytes read
        NULL);            // not overlapped
	  
    if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
    {
        DPRINT("CreateServiceW - Error\n");
        return NULL;
    }
	  
    if (Reply.ReplyStatus != NO_ERROR)
    {
        DPRINT("CreateServiceW - Error (%x)\n", Reply.ReplyStatus);
        SetLastError(Reply.ReplyStatus);
        return NULL;
    }

    hService = CreateFileW(
        Reply.PipeName,   // pipe name
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        0,                // no sharing
        NULL,             // no security attributes
        OPEN_EXISTING,    // opens existing pipe
        0,                // default attributes
        NULL);            // no template file

    if (hService == INVALID_HANDLE_VALUE)
    {
        /* FIXME: Set last error! */
        return NULL;
    }

    DPRINT("CreateServiceW - Success - %x\n", hService);
    return hService;
}


/**********************************************************************
 *  DeleteService
 *
 * @unimplemented
 */
BOOL
STDCALL
DeleteService(SC_HANDLE hService)
{
    SCM_SERVICE_REQUEST Request;
    SCM_SERVICE_REPLY Reply;
    BOOL fSuccess;
    DWORD cbWritten, cbRead;

    DPRINT("DeleteService\n");

    Request.RequestCode = SCM_DELETESERVICE;

    fSuccess = WriteFile(
        hService,         // pipe handle
        &Request,         // message
        sizeof(DWORD),    // message length
        &cbWritten,       // bytes written
        NULL);            // not overlapped

    if (!fSuccess || cbWritten != sizeof(DWORD))
    {
        DPRINT("Error: %x . %x\n", GetLastError(), hService);
        /* FIXME: Set last error */
        return FALSE;
    }
      
    fSuccess = ReadFile(
        hService,         // pipe handle
        &Reply,           // buffer to receive reply
        sizeof(DWORD),    // size of buffer
        &cbRead,          // number of bytes read
        NULL);            // not overlapped
	  
    if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
    {
        CHECKPOINT;
        /* FIXME: Set last error */
        return FALSE;
    }
	  
    if (Reply.ReplyStatus != NO_ERROR)
    {
        CHECKPOINT;
        SetLastError(Reply.ReplyStatus);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  EnumDependentServicesA
 *
 * @unimplemented
 */
BOOL
STDCALL
EnumDependentServicesA(
    SC_HANDLE       hService,
    DWORD           dwServiceState,
    LPENUM_SERVICE_STATUSA  lpServices,
    DWORD           cbBufSize,
    LPDWORD         pcbBytesNeeded,
    LPDWORD         lpServicesReturned)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  EnumDependentServicesW
 *
 * @unimplemented
 */
BOOL
STDCALL
EnumDependentServicesW(
    SC_HANDLE       hService,
    DWORD           dwServiceState,
    LPENUM_SERVICE_STATUSW  lpServices,
    DWORD           cbBufSize,
    LPDWORD         pcbBytesNeeded,
    LPDWORD         lpServicesReturned)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  EnumServiceGroupW
 *
 * @unimplemented
 */
BOOL
STDCALL
EnumServiceGroupW (
    DWORD   Unknown0,
    DWORD   Unknown1,
    DWORD   Unknown2,
    DWORD   Unknown3,
    DWORD   Unknown4,
    DWORD   Unknown5,
    DWORD   Unknown6,
    DWORD   Unknown7,
    DWORD   Unknown8)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  EnumServicesStatusA
 *
 * @unimplemented
 */
BOOL
STDCALL
EnumServicesStatusA (
    SC_HANDLE               hSCManager,
    DWORD                   dwServiceType,
    DWORD                   dwServiceState,
    LPENUM_SERVICE_STATUSA  lpServices,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded,
    LPDWORD                 lpServicesReturned,
    LPDWORD                 lpResumeHandle)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  EnumServicesStatusExA
 *
 * @unimplemented
 */
BOOL
STDCALL
EnumServicesStatusExA(SC_HANDLE  hSCManager,
  SC_ENUM_TYPE  InfoLevel,
  DWORD  dwServiceType,
  DWORD  dwServiceState,
  LPBYTE  lpServices,
  DWORD  cbBufSize,
  LPDWORD  pcbBytesNeeded,
  LPDWORD  lpServicesReturned,
  LPDWORD  lpResumeHandle,
  LPCSTR  pszGroupName)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  EnumServicesStatusExW
 *
 * @unimplemented
 */
BOOL
STDCALL
EnumServicesStatusExW(SC_HANDLE  hSCManager,
  SC_ENUM_TYPE  InfoLevel,
  DWORD  dwServiceType,
  DWORD  dwServiceState,
  LPBYTE  lpServices,
  DWORD  cbBufSize,
  LPDWORD  pcbBytesNeeded,
  LPDWORD  lpServicesReturned,
  LPDWORD  lpResumeHandle,
  LPCWSTR  pszGroupName)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  EnumServicesStatusW
 *
 * @unimplemented
 */
BOOL
STDCALL
EnumServicesStatusW(
    SC_HANDLE               hSCManager,
    DWORD                   dwServiceType,
    DWORD                   dwServiceState,
    LPENUM_SERVICE_STATUSW  lpServices,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded,
    LPDWORD                 lpServicesReturned,
    LPDWORD                 lpResumeHandle)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  GetServiceDisplayNameA
 *
 * @unimplemented
 */
BOOL
STDCALL
GetServiceDisplayNameA(
    SC_HANDLE   hSCManager,
    LPCSTR      lpServiceName,
    LPSTR       lpDisplayName,
    LPDWORD     lpcchBuffer)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  GetServiceDisplayNameW
 *
 * @unimplemented
 */
BOOL
STDCALL
GetServiceDisplayNameW(
    SC_HANDLE   hSCManager,
    LPCWSTR     lpServiceName,
    LPWSTR      lpDisplayName,
    LPDWORD     lpcchBuffer)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  GetServiceKeyNameA
 *
 * @unimplemented
 */
BOOL
STDCALL
GetServiceKeyNameA(
    SC_HANDLE   hSCManager,
    LPCSTR      lpDisplayName,
    LPSTR       lpServiceName,
    LPDWORD     lpcchBuffer)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  GetServiceKeyNameW
 *
 * @unimplemented
 */
BOOL
STDCALL
GetServiceKeyNameW(
    SC_HANDLE   hSCManager,
    LPCWSTR     lpDisplayName,
    LPWSTR      lpServiceName,
    LPDWORD     lpcchBuffer)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 *  LockServiceDatabase
 *
 * @unimplemented
 */
SC_LOCK
STDCALL
LockServiceDatabase(SC_HANDLE   hSCManager)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}


/**********************************************************************
 *  OpenSCManagerA
 *
 * @unplemented
 */
SC_HANDLE STDCALL
OpenSCManagerA(LPCSTR lpMachineName,
           LPCSTR lpDatabaseName,
           DWORD dwDesiredAccess)
{
  SC_HANDLE Handle;
  UNICODE_STRING MachineNameW;
  UNICODE_STRING DatabaseNameW;
  ANSI_STRING MachineNameA;
  ANSI_STRING DatabaseNameA;

  DPRINT("OpenSCManagerA(%x, %x, %d)\n", lpMachineName, lpDatabaseName, dwDesiredAccess);

  RtlInitAnsiString(&MachineNameA, (LPSTR)lpMachineName);
  RtlAnsiStringToUnicodeString(&MachineNameW, &MachineNameA, TRUE);
  RtlInitAnsiString(&DatabaseNameA, (LPSTR)lpDatabaseName);
  RtlAnsiStringToUnicodeString(&DatabaseNameW, &DatabaseNameA, TRUE);

  Handle = OpenSCManagerW(lpMachineName ? MachineNameW.Buffer : NULL,
              lpDatabaseName ? DatabaseNameW.Buffer : NULL,
              dwDesiredAccess);

  RtlFreeHeap(GetProcessHeap(), 0, MachineNameW.Buffer);
  RtlFreeHeap(GetProcessHeap(), 0, DatabaseNameW.Buffer);
  return Handle;
}


/**********************************************************************
 *  OpenSCManagerW
 *
 * @unimplemented
 */
SC_HANDLE STDCALL OpenSCManagerW(LPCWSTR lpMachineName,
                                 LPCWSTR lpDatabaseName,
                                 DWORD dwDesiredAccess)
{
  HANDLE hPipe;
  DWORD dwMode;
  DWORD dwWait;
  BOOL fSuccess;
  HANDLE hStartEvent;
  LPWSTR lpszPipeName = L"\\\\.\\pipe\\Ntsvcs";

  DPRINT("OpenSCManagerW(%x, %x, %d)\n", lpMachineName, lpDatabaseName, dwDesiredAccess);

  if (lpMachineName == NULL || wcslen(lpMachineName) == 0)
    {
      if (lpDatabaseName != NULL && wcscmp(lpDatabaseName, SERVICES_ACTIVE_DATABASEW) != 0)
	{
	  DPRINT("OpenSCManagerW() - Invalid parameters.\n");
	  return NULL; 
	}

      DPRINT("OpenSCManagerW() - OpenEvent(\"SvcctrlStartEvent_A3725DX\")\n");

      // Only connect to scm when event "SvcctrlStartEvent_A3725DX" is signaled
      hStartEvent = OpenEventW(SYNCHRONIZE, FALSE, L"SvcctrlStartEvent_A3725DX");
      if (hStartEvent == NULL)
	{
	  SetLastError(ERROR_DATABASE_DOES_NOT_EXIST);
	  DPRINT("OpenSCManagerW() - Failed to Open Event \"SvcctrlStartEvent_A3725DX\".\n");
	  return NULL;
	}

      DPRINT("OpenSCManagerW() - Waiting forever on event handle: %x\n", hStartEvent);

#if 1
      dwWait = WaitForSingleObject(hStartEvent, INFINITE);
      if (dwWait == WAIT_FAILED)
	{
	  DPRINT("OpenSCManagerW() - Wait For Start Event failed.\n");
	  SetLastError(ERROR_ACCESS_DENIED);
	  return NULL;
	}
#else
	{
          DWORD Count;

	  /* wait for event creation (by SCM) for max. 20 seconds */
	  for (Count = 0; Count < 20; Count++)
	    {
	      dwWait = WaitForSingleObject(hStartEvent, 1000);
	      if (dwWait == WAIT_FAILED)
		{
		  DPRINT("OpenSCManagerW() - Wait For Start Event failed.\n");
		  Sleep(1000);
		}
	      else
		{
		  break;
		}
	    }

	  if (dwWait == WAIT_FAILED)
	    {
	      DbgPrint("WL: Failed to wait on event \"SvcctrlStartEvent_A3725DX\"\n");
	    }

	}
#endif

      DPRINT("OpenSCManagerW() - Closing handle to event...\n");
      
      CloseHandle(hStartEvent);
      
      // Try to open a named pipe; wait for it, if necessary
      while (1)
      {
	DWORD dwLastError;
	DPRINT("OpenSCManagerW() - attempting to open named pipe to SCM.\n");
	hPipe = CreateFileW(lpszPipeName,     // pipe name
	    dwDesiredAccess,
	    0,                // no sharing
	    NULL,             // no security attributes
	    OPEN_EXISTING,    // opens existing pipe
	    0,                // default attributes
	    NULL);            // no template file
	
	DPRINT("OpenSCManagerW() - handle to named pipe: %x\n", hPipe);
	// Break if the pipe handle is valid
	if (hPipe != INVALID_HANDLE_VALUE)
	  {
	    break;
	  }
	
	// Exit if an error other than ERROR_PIPE_BUSY occurs
	dwLastError = GetLastError();
	if (dwLastError != ERROR_PIPE_BUSY)
	  {
	    DPRINT("OpenSCManagerW() - returning at 4, dwLastError %d\n", dwLastError);
	    return NULL;
	  }
	
	// All pipe instances are busy, so wait for 20 seconds
	if (!WaitNamedPipeW(lpszPipeName, 20000))
	  {
	    DPRINT("OpenSCManagerW() - Failed on WaitNamedPipeW(...).\n");
	    return NULL;
	  }
      }
    
      // The pipe connected; change to message-read mode
      dwMode = PIPE_READMODE_MESSAGE;
      fSuccess = SetNamedPipeHandleState(
	  hPipe,    // pipe handle
	  &dwMode,  // new pipe mode
	  NULL,     // don't set maximum bytes
	  NULL);    // don't set maximum time
      if (!fSuccess)
	{
	  CloseHandle(hPipe);
	  DPRINT("OpenSCManagerW() - Failed on SetNamedPipeHandleState(...).\n");
	  return NULL;
	}
#if 0
      // Send a message to the pipe server
      lpvMessage = (argc > 1) ? argv[1] : "default message";
      
      fSuccess = WriteFile(
	  hPipe,                  // pipe handle
	  lpvMessage,             // message
	  strlen(lpvMessage) + 1, // message length
	  &cbWritten,             // bytes written
	  NULL);                  // not overlapped
      if (!fSuccess)
	{
	  CloseHandle(hPipe);
	  DPRINT("OpenSCManagerW() - Failed to write to pipe.\n");
	  return NULL;
	}
      
      do
	{
	  DPRINT("OpenSCManagerW() - in I/O loop to SCM...\n");
	  // Read from the pipe
	  fSuccess = ReadFile(
	      hPipe,    // pipe handle
	      chBuf,    // buffer to receive reply
	      512,      // size of buffer
	      &cbRead,  // number of bytes read
	      NULL);    // not overlapped
	  
	  if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
	    {
	      break;
	    }
	  
	  // Reply from the pipe is written to STDOUT.
	  if (!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), chBuf, cbRead, &cbWritten, NULL))
	    {
	      break;
	    }
	} while(!fSuccess);  // repeat loop if ERROR_MORE_DATA
      
      DPRINT("OpenSCManagerW() - I/O loop completed.\n");
      //CloseHandle(hPipe);
#endif
      DPRINT("OpenSCManagerW() - success, returning handle to pipe %x\n", hPipe);
      return hPipe;
    }
  else
    {
      /* FIXME: Connect to remote SCM */
      DPRINT("OpenSCManagerW() - FIXME: Connect to remote SCM not implemented.\n");
      return NULL;
    }
}


/**********************************************************************
 *  OpenServiceA
 *
 * @implemented
 */
SC_HANDLE
STDCALL
OpenServiceA(
    SC_HANDLE hSCManager,
    LPCSTR lpServiceName,
    DWORD dwDesiredAccess
    )
{
    UNICODE_STRING lpServiceNameW;
    SC_HANDLE hService;

    RtlCreateUnicodeStringFromAsciiz(&lpServiceNameW, (LPSTR)lpServiceName);
    hService = OpenServiceW(
        hSCManager,
        lpServiceNameW.Buffer,
        dwDesiredAccess);
    RtlFreeUnicodeString(&lpServiceNameW);

    return hService;
}


/**********************************************************************
 *  OpenServiceW
 *
 * @implemented
 */
SC_HANDLE
STDCALL
OpenServiceW(
    SC_HANDLE   hSCManager,
    LPCWSTR     lpServiceName,
    DWORD       dwDesiredAccess
    )
{
    SCM_OPENSERVICE_REQUEST Request;
    SCM_OPENSERVICE_REPLY Reply;
    BOOL fSuccess;
    DWORD cbWritten, cbRead;
    SC_HANDLE hService;

    DPRINT("OpenServiceW\n");

    if (lpServiceName == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    Request.RequestCode = SCM_OPENSERVICE;
    INIT_SCM_STRING(Request.ServiceName, lpServiceName);
    Request.dwDesiredAccess = dwDesiredAccess;

    fSuccess = WriteFile(
        hSCManager,       // pipe handle
        &Request,         // message
        sizeof(Request),  // message length
        &cbWritten,       // bytes written
        NULL);            // not overlapped

    if (!fSuccess || cbWritten != sizeof(Request))
    {
        DPRINT("OpenServiceW - Failed to write to pipe.\n");
        return NULL;
    }
      
    fSuccess = ReadFile(
        hSCManager,       // pipe handle
        &Reply,           // buffer to receive reply
        sizeof(Reply),    // size of buffer
        &cbRead,          // number of bytes read
        NULL);            // not overlapped
	  
    if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
    {
        DPRINT("OpenServiceW - Failed to read from pipe\n");
        return NULL;
    }
	  
    if (Reply.ReplyStatus != NO_ERROR)
    {
        DPRINT("OpenServiceW - Error (%x)\n", Reply.ReplyStatus);
        SetLastError(Reply.ReplyStatus);
        return NULL;
    }

    hService = CreateFileW(
        Reply.PipeName,   // pipe name
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        0,                // no sharing
        NULL,             // no security attributes
        OPEN_EXISTING,    // opens existing pipe
        0,                // default attributes
        NULL);            // no template file

    if (hService == INVALID_HANDLE_VALUE)
    {
        DPRINT("OpenServiceW - Failed to connect to pipe\n");
        return NULL;
    }

    DPRINT("OpenServiceW - Success - %x\n", hService);
    return hService;
}


/**********************************************************************
 *  QueryServiceConfigA
 *
 * @unimplemented
 */
BOOL
STDCALL
QueryServiceConfigA(
    SC_HANDLE       hService,
    LPQUERY_SERVICE_CONFIGA lpServiceConfig,
    DWORD           cbBufSize,
    LPDWORD         pcbBytesNeeded)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  QueryServiceConfigW
 *
 * @unimplemented
 */
BOOL
STDCALL
QueryServiceConfigW(
    SC_HANDLE       hService,
    LPQUERY_SERVICE_CONFIGW lpServiceConfig,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  QueryServiceLockStatusA
 *
 * @unimplemented
 */
BOOL
STDCALL
QueryServiceLockStatusA(
    SC_HANDLE           hSCManager,
    LPQUERY_SERVICE_LOCK_STATUSA    lpLockStatus,
    DWORD               cbBufSize,
    LPDWORD             pcbBytesNeeded)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  QueryServiceLockStatusW
 *
 * @unimplemented
 */
BOOL
STDCALL
QueryServiceLockStatusW(
    SC_HANDLE           hSCManager,
    LPQUERY_SERVICE_LOCK_STATUSW    lpLockStatus,
    DWORD               cbBufSize,
    LPDWORD             pcbBytesNeeded)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  QueryServiceObjectSecurity
 *
 * @unimplemented
 */
BOOL
STDCALL
QueryServiceObjectSecurity(
    SC_HANDLE       hService,
    SECURITY_INFORMATION    dwSecurityInformation,
    PSECURITY_DESCRIPTOR    lpSecurityDescriptor,
    DWORD           cbBufSize,
    LPDWORD         pcbBytesNeeded)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  QueryServiceStatus
 *
 * @unimplemented
 */
BOOL
STDCALL
QueryServiceStatus(
    SC_HANDLE       hService,
    LPSERVICE_STATUS    lpServiceStatus)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  QueryServiceStatusEx
 *
 * @unimplemented
 */
BOOL
STDCALL
QueryServiceStatusEx(SC_HANDLE  hService,
  SC_STATUS_TYPE  InfoLevel,
  LPBYTE  lpBuffer,
  DWORD  cbBufSize,
  LPDWORD  pcbBytesNeeded)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  StartServiceA
 *
 * @unimplemented
 */
BOOL
STDCALL
StartServiceA(
    SC_HANDLE   hService,
    DWORD       dwNumServiceArgs,
    LPCSTR      *lpServiceArgVectors)
{
#if 0
    DPRINT("StartServiceA\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
#else
    SCM_SERVICE_REQUEST Request;
    SCM_SERVICE_REPLY Reply;
    BOOL fSuccess;
    DWORD cbWritten, cbRead;

    DPRINT("StartServiceA\n");

    if (dwNumServiceArgs != 0)
    {
       UNIMPLEMENTED;
    }

    Request.RequestCode = SCM_STARTSERVICE;

    fSuccess = WriteFile(
        hService,         // pipe handle
        &Request,         // message
        sizeof(DWORD),    // message length
        &cbWritten,       // bytes written
        NULL);            // not overlapped

    if (!fSuccess || cbWritten != sizeof(DWORD))
    {
        DPRINT("Error: %x . %x\n", GetLastError(), hService);
        /* FIXME: Set last error */
        return FALSE;
    }
      
    fSuccess = ReadFile(
        hService,         // pipe handle
        &Reply,           // buffer to receive reply
        sizeof(DWORD),    // size of buffer
        &cbRead,          // number of bytes read
        NULL);            // not overlapped
	  
    if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
    {
        CHECKPOINT;
        /* FIXME: Set last error */
        return FALSE;
    }
	  
    if (Reply.ReplyStatus != NO_ERROR)
    {
        CHECKPOINT;
        SetLastError(Reply.ReplyStatus);
        return FALSE;
    }

    CHECKPOINT;
    return TRUE;
#endif
}




/**********************************************************************
 *  StartServiceW
 *
 * @unimplemented
 */
BOOL
STDCALL
StartServiceW(
    SC_HANDLE   hService,
    DWORD       dwNumServiceArgs,
    LPCWSTR     *lpServiceArgVectors)
{
    DPRINT("StartServiceW\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  UnlockServiceDatabase
 *
 * @unimplemented
 */
BOOL
STDCALL
UnlockServiceDatabase(SC_LOCK   ScLock)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/* EOF */
