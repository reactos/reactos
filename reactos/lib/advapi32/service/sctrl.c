/* $Id: sctrl.c,v 1.9 2003/02/02 19:27:17 hyperion Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/service/sctrl.c
 * PURPOSE:         Service control manager functions
 * PROGRAMMER:      Emanuele Aliberti
 * UPDATE HISTORY:
 *	19990413 EA	created
 *	19990515 EA
 */

/* INCLUDES ******************************************************************/

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>
#include <string.h>
#include <wchar.h>

#define NDEBUG
#include <debug.h>


/* TYPES *********************************************************************/

typedef struct
{
  DWORD ThreadId;
  UNICODE_STRING ServiceName;
  LPSERVICE_MAIN_FUNCTION MainFunction;
  LPHANDLER_FUNCTION HandlerFunction;
  SERVICE_STATUS ServiceStatus;
} ACTIVE_SERVICE, *PACTIVE_SERVICE;

/* GLOBALS *******************************************************************/

static ULONG ActiveServiceCount;
static PACTIVE_SERVICE ActiveServices;
/* static PHANDLE ActiveServicesThreadHandles; */ /* uncomment when in use */

/* FUNCTIONS *****************************************************************/


static PACTIVE_SERVICE
ScLookupServiceByServiceName(LPWSTR lpServiceName)
{
  DWORD i;

  for (i = 0; i < ActiveServiceCount; i++)
    {
      if (_wcsicmp(ActiveServices[i].ServiceName.Buffer, lpServiceName) == 0)
	{
	  return(&ActiveServices[i]);
	}
    }

  SetLastError(ERROR_SERVICE_DOES_NOT_EXIST);

  return(NULL);
}


static PACTIVE_SERVICE
ScLookupServiceByThreadId(DWORD ThreadId)
{
  DWORD i;

  for (i = 0; i < ActiveServiceCount; i++)
    {
      if (ActiveServices[i].ThreadId == ThreadId)
	{
	  return(&ActiveServices[i]);
	}
    }

  SetLastError(ERROR_SERVICE_DOES_NOT_EXIST);

  return(NULL);
}


static DWORD
ScConnectControlPipe(HANDLE *hPipe)
{
  DWORD dwBytesWritten;
  DWORD dwProcessId;
  DWORD dwState;

  WaitNamedPipeW(L"\\\\.\\pipe\\net\\NtControlPipe",
		 15000);

  *hPipe = CreateFileW(L"\\\\.\\pipe\\net\\NtControlPipe",
		       GENERIC_READ | GENERIC_WRITE,
		       FILE_SHARE_READ | FILE_SHARE_WRITE,
		       NULL,
		       OPEN_EXISTING,
		       FILE_ATTRIBUTE_NORMAL,
		       NULL);
  if (*hPipe == INVALID_HANDLE_VALUE)
    return(ERROR_FAILED_SERVICE_CONTROLLER_CONNECT);

  dwState = PIPE_READMODE_MESSAGE;
  if (!SetNamedPipeHandleState(*hPipe, &dwState, NULL, NULL))
    {
      CloseHandle(hPipe);
      *hPipe = INVALID_HANDLE_VALUE;
      return(ERROR_FAILED_SERVICE_CONTROLLER_CONNECT);
    }

  dwProcessId = GetCurrentProcessId();
  WriteFile(*hPipe,
	    &dwProcessId,
	    sizeof(DWORD),
	    &dwBytesWritten,
	    NULL);

  return(ERROR_SUCCESS);
}


static VOID
ScServiceDispatcher(HANDLE hPipe, PVOID p1, PVOID p2)
{
  DPRINT1("ScDispatcherLoop() called\n");

#if 0
  while (TRUE)
    {
      /* Read command from the control pipe */

      /* Execute command */

    }
#endif
}


DWORD WINAPI
ScServiceMainStub(LPVOID Context)
{
  LPSERVICE_MAIN_FUNCTION lpServiceProc = (LPSERVICE_MAIN_FUNCTION)Context;

  /* FIXME: Send argc and argv (from command line) as arguments */

  (lpServiceProc)(0, NULL);

  return ERROR_SUCCESS;
}


/**********************************************************************
 *	RegisterServiceCtrlHandlerA
 */
SERVICE_STATUS_HANDLE STDCALL
RegisterServiceCtrlHandlerA(LPCSTR lpServiceName,
			    LPHANDLER_FUNCTION lpHandlerProc)
{
  ANSI_STRING ServiceNameA;
  UNICODE_STRING ServiceNameU;
  SERVICE_STATUS_HANDLE SHandle;

  RtlInitAnsiString(&ServiceNameA, (LPSTR)lpServiceName);
  if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&ServiceNameU, &ServiceNameA, TRUE)))
    {
      SetLastError(ERROR_OUTOFMEMORY);
      return((SERVICE_STATUS_HANDLE)0);
    }

  SHandle = RegisterServiceCtrlHandlerW(ServiceNameU.Buffer,
					lpHandlerProc);

  RtlFreeUnicodeString(&ServiceNameU);

  return(SHandle);
}


/**********************************************************************
 *	RegisterServiceCtrlHandlerW
 */
SERVICE_STATUS_HANDLE STDCALL
RegisterServiceCtrlHandlerW(LPCWSTR lpServiceName,
			    LPHANDLER_FUNCTION lpHandlerProc)
{
  PACTIVE_SERVICE Service;

  Service = ScLookupServiceByServiceName((LPWSTR)lpServiceName);
  if (Service == NULL)
    {
      return((SERVICE_STATUS_HANDLE)NULL);
    }

  Service->HandlerFunction = lpHandlerProc;

  return((SERVICE_STATUS_HANDLE)Service->ThreadId);
}


/**********************************************************************
 *	SetServiceBits
 */
BOOL STDCALL
SetServiceBits(SERVICE_STATUS_HANDLE hServiceStatus,
	       DWORD dwServiceBits,
	       BOOL bSetBitsOn,
	       BOOL bUpdateImmediately)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return(FALSE);
}


/**********************************************************************
 *	SetServiceObjectSecurity
 */
WINBOOL STDCALL
SetServiceObjectSecurity(SC_HANDLE hService,
			 SECURITY_INFORMATION dwSecurityInformation,
			 PSECURITY_DESCRIPTOR lpSecurityDescriptor)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/**********************************************************************
 *	SetServiceStatus
 */
BOOL STDCALL
SetServiceStatus(SERVICE_STATUS_HANDLE hServiceStatus,
		 LPSERVICE_STATUS lpServiceStatus)
{
  PACTIVE_SERVICE Service;

  Service = ScLookupServiceByThreadId((DWORD)hServiceStatus);
  if (!Service)
    {
      SetLastError(ERROR_INVALID_HANDLE);
      return(FALSE);
    }

  RtlCopyMemory(&Service->ServiceStatus,
		lpServiceStatus,
		sizeof(SERVICE_STATUS));

  return(TRUE);
}


/**********************************************************************
 *	StartServiceCtrlDispatcherA
 */
BOOL STDCALL
StartServiceCtrlDispatcherA(LPSERVICE_TABLE_ENTRYA lpServiceStartTable)
{
  LPSERVICE_TABLE_ENTRYW ServiceStartTableW;
  ANSI_STRING ServiceNameA;
  UNICODE_STRING ServiceNameW;
  ULONG i, j;
  ULONG Count;
  BOOL b;

  i = 0;
  while (lpServiceStartTable[i].lpServiceProc != NULL)
    {
      i++;
    }
  Count = i;

  ServiceStartTableW = RtlAllocateHeap(RtlGetProcessHeap(),
				       HEAP_ZERO_MEMORY,
				       sizeof(SERVICE_TABLE_ENTRYW) * Count);
  for (i = 0; i < Count; i++)
  {
    RtlInitAnsiString(
      &ServiceNameA,
   	  lpServiceStartTable[i].lpServiceName);
    if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(
      &ServiceNameW,
   	  &ServiceNameA,
   		TRUE)))
    {
      for (j = 0; j < i; j++)
      {
        RtlInitUnicodeString(
          &ServiceNameW,
     	    ServiceStartTableW[j].lpServiceName);
        RtlFreeUnicodeString(&ServiceNameW);
      }
      RtlFreeHeap(RtlGetProcessHeap(), 0, ServiceStartTableW);
      SetLastError(ERROR_OUTOFMEMORY);
  	  return FALSE;
    }
    ServiceStartTableW[i].lpServiceName = ServiceNameW.Buffer;
    ServiceStartTableW[i].lpServiceProc = 
      lpServiceStartTable[i].lpServiceProc;
  }

  b = StartServiceCtrlDispatcherW(ServiceStartTableW);

  for (i = 0; i < Count; i++)
    {
      RtlFreeHeap(RtlGetProcessHeap(), 0, ServiceStartTableW[i].lpServiceName);
    }

  RtlFreeHeap(RtlGetProcessHeap(), 0, ServiceStartTableW);

  return b;
}


/**********************************************************************
 *	StartServiceCtrlDispatcherW
 */
BOOL STDCALL
StartServiceCtrlDispatcherW(LPSERVICE_TABLE_ENTRYW lpServiceStartTable)
{
  ULONG i;
  HANDLE hPipe;
  DWORD dwError;

  DPRINT1("StartServiceCtrlDispatcherW() called\n");

  i = 0;
  while (lpServiceStartTable[i].lpServiceProc != NULL)
    {
      i++;
    }

  ActiveServiceCount = i;
  ActiveServices = RtlAllocateHeap(RtlGetProcessHeap(),
				   HEAP_ZERO_MEMORY,
				   ActiveServiceCount * sizeof(ACTIVE_SERVICE));
  if (ActiveServices == NULL)
    {
      return(FALSE);
    }

  /* Copy service names and start procedure */
  for (i = 0; i < ActiveServiceCount; i++)
    {
      RtlCreateUnicodeString(&ActiveServices[i].ServiceName,
			     lpServiceStartTable[i].lpServiceName);
      ActiveServices[i].MainFunction = lpServiceStartTable[i].lpServiceProc;
    }

  dwError = ScConnectControlPipe(&hPipe);
  if (dwError == ERROR_SUCCESS)
    {
      /* FIXME: free the service table */
      return(FALSE);
    }

  ScServiceDispatcher(hPipe, NULL, NULL);
  CloseHandle(hPipe);

  /* FIXME: free the service table */

  return(TRUE);

#if 0
  ActiveServicesThreadHandles = RtlAllocateHeap(RtlGetProcessHeap(),
						HEAP_ZERO_MEMORY,
						(ActiveServiceCount + 1) * sizeof(HANDLE));
  if (!ActiveServicesThreadHandles)
    {
      RtlFreeHeap(RtlGetProcessHeap(), 0, ActiveServices);
      ActiveServices = NULL;
      return(FALSE);
    }

  for (i = 0; i<ActiveServiceCount; i++)
  {
    h = CreateThread(
      NULL,
   	  0,
   	  ScServiceMainStub,
   	  lpServiceStartTable[i].lpServiceProc,
   	  0,
   	  &Tid);
    if (h == INVALID_HANDLE_VALUE)
    {
      RtlFreeHeap(RtlGetProcessHeap(), 0, ActiveServicesThreadHandles);
      ActiveServicesThreadHandles = NULL;
      RtlFreeHeap(RtlGetProcessHeap(), 0, ActiveServices);
      ActiveServices = NULL;
      return(FALSE);
    }
    ActiveServicesThreadHandles[i + 1] = h;
    ActiveServices[i].ThreadId = Tid;
  }

  while (ActiveServiceCount > 0)
  {
    r = WaitForMultipleObjects(
      ActiveServiceCount + 1,
   	  ActiveServicesThreadHandles,
   		FALSE,
   		INFINITE);
    if (r == WAIT_OBJECT_0)
    {
      /* Received message from the scm */
    }
    else if (r > WAIT_OBJECT_0 && r < (WAIT_OBJECT_0 + ActiveServiceCount))
    {
      /* A service died */
        
      ActiveServiceCount--;
      ActiveServicesThreadHandles[r - WAIT_OBJECT_0 - 1] =
        ActiveServicesThreadHandles[ActiveServiceCount + 1];
      RtlCopyMemory(
        &ActiveServices[r - WAIT_OBJECT_0 - 2],
        &ActiveServices[ActiveServiceCount],
        sizeof(ACTIVE_SERVICE));
    }
    else
    {
      /* Bail */
    }
  }
  return TRUE;
#endif
}

/* EOF */
