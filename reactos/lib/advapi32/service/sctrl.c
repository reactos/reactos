/* $Id$
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

#include "advapi32.h"
#define NDEBUG
#include <debug.h>


/* TYPES *********************************************************************/

typedef struct _ACTIVE_SERVICE
{
  DWORD ThreadId;
  UNICODE_STRING ServiceName;
  LPSERVICE_MAIN_FUNCTIONW MainFunction;
  LPHANDLER_FUNCTION HandlerFunction;
  SERVICE_STATUS ServiceStatus;
} ACTIVE_SERVICE, *PACTIVE_SERVICE;


/* GLOBALS *******************************************************************/

static DWORD dwActiveServiceCount = 0;
static PACTIVE_SERVICE lpActiveServices = NULL;
/* static PHANDLE ActiveServicesThreadHandles; */ /* uncomment when in use */


/* FUNCTIONS *****************************************************************/

static PACTIVE_SERVICE
ScLookupServiceByServiceName(LPWSTR lpServiceName)
{
  DWORD i;

  for (i = 0; i < dwActiveServiceCount; i++)
    {
      if (_wcsicmp(lpActiveServices[i].ServiceName.Buffer, lpServiceName) == 0)
	{
	  return &lpActiveServices[i];
	}
    }

  SetLastError(ERROR_SERVICE_DOES_NOT_EXIST);

  return NULL;
}


static PACTIVE_SERVICE
ScLookupServiceByThreadId(DWORD ThreadId)
{
  DWORD i;

  for (i = 0; i < dwActiveServiceCount; i++)
    {
      if (lpActiveServices[i].ThreadId == ThreadId)
	{
	  return &lpActiveServices[i];
	}
    }

  SetLastError(ERROR_SERVICE_DOES_NOT_EXIST);

  return NULL;
}


static DWORD WINAPI
ScServiceMainStub(LPVOID Context)
{
  PACTIVE_SERVICE lpService;

  lpService = (PACTIVE_SERVICE)Context;

  DPRINT("ScServiceMainStub() called\n");

  /* FIXME: Send argc and argv (from command line) as arguments */


  (lpService->MainFunction)(0, NULL);

  return ERROR_SUCCESS;
}


static DWORD
ScConnectControlPipe(HANDLE *hPipe)
{
  DWORD dwBytesWritten;
  DWORD dwProcessId;
  DWORD dwState;

  if (!WaitNamedPipeW(L"\\\\.\\pipe\\net\\NtControlPipe", 15000))
    {
      DPRINT1("WaitNamedPipe() failed (Error %lu)\n", GetLastError());
      return ERROR_FAILED_SERVICE_CONTROLLER_CONNECT;
    }

  *hPipe = CreateFileW(L"\\\\.\\pipe\\net\\NtControlPipe",
		       GENERIC_READ | GENERIC_WRITE,
		       0,
		       NULL,
		       OPEN_EXISTING,
		       FILE_ATTRIBUTE_NORMAL,
		       NULL);
  if (*hPipe == INVALID_HANDLE_VALUE)
    {
      DPRINT1("CreateFileW() failed (Error %lu)\n", GetLastError());
      return ERROR_FAILED_SERVICE_CONTROLLER_CONNECT;
    }

  dwState = PIPE_READMODE_MESSAGE;
  if (!SetNamedPipeHandleState(*hPipe, &dwState, NULL, NULL))
    {
      CloseHandle(hPipe);
      *hPipe = INVALID_HANDLE_VALUE;
      return ERROR_FAILED_SERVICE_CONTROLLER_CONNECT;
    }

  dwProcessId = GetCurrentProcessId();
  WriteFile(*hPipe,
	    &dwProcessId,
	    sizeof(DWORD),
	    &dwBytesWritten,
	    NULL);

  DPRINT("Sent process id %lu\n", dwProcessId);

  return ERROR_SUCCESS;
}


static BOOL
ScServiceDispatcher(HANDLE hPipe,
		    PUCHAR lpBuffer,
		    DWORD dwBufferSize)
{
  PACTIVE_SERVICE lpService;
  HANDLE ThreadHandle;

  DPRINT("ScDispatcherLoop() called\n");

  lpService = &lpActiveServices[0];

  ThreadHandle = CreateThread(NULL,
			      0,
			      ScServiceMainStub,
			      lpService,
			      CREATE_SUSPENDED,
			      &lpService->ThreadId);
  if (ThreadHandle == NULL)
    return FALSE;

  ResumeThread(ThreadHandle);

  CloseHandle(ThreadHandle);

#if 0
  while (TRUE)
    {
      /* Read command from the control pipe */

      /* Execute command */

    }
#endif

  return TRUE;
}


/**********************************************************************
 *	RegisterServiceCtrlHandlerA
 *
 * @implemented
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
      return (SERVICE_STATUS_HANDLE)0;
    }

  SHandle = RegisterServiceCtrlHandlerW(ServiceNameU.Buffer,
					lpHandlerProc);

  RtlFreeUnicodeString(&ServiceNameU);

  return SHandle;
}


/**********************************************************************
 *	RegisterServiceCtrlHandlerW
 *
 * @implemented
 */
SERVICE_STATUS_HANDLE STDCALL
RegisterServiceCtrlHandlerW(LPCWSTR lpServiceName,
			    LPHANDLER_FUNCTION lpHandlerProc)
{
  PACTIVE_SERVICE Service;

  Service = ScLookupServiceByServiceName((LPWSTR)lpServiceName);
  if (Service == NULL)
    {
      return (SERVICE_STATUS_HANDLE)NULL;
    }

  Service->HandlerFunction = lpHandlerProc;

  return (SERVICE_STATUS_HANDLE)Service->ThreadId;
}


/**********************************************************************
 *	SetServiceBits
 *
 * @unimplemented
 */
BOOL STDCALL
SetServiceBits(SERVICE_STATUS_HANDLE hServiceStatus,
	       DWORD dwServiceBits,
	       BOOL bSetBitsOn,
	       BOOL bUpdateImmediately)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/**********************************************************************
 *	SetServiceObjectSecurity
 *
 * @unimplemented
 */
BOOL STDCALL
SetServiceObjectSecurity(SC_HANDLE hService,
			 SECURITY_INFORMATION dwSecurityInformation,
			 PSECURITY_DESCRIPTOR lpSecurityDescriptor)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/**********************************************************************
 *	SetServiceStatus
 *
 * @implemented
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
      return FALSE;
    }

  RtlCopyMemory(&Service->ServiceStatus,
		lpServiceStatus,
		sizeof(SERVICE_STATUS));

  return TRUE;
}


/**********************************************************************
 *	StartServiceCtrlDispatcherA
 *
 * @unimplemented
 */
BOOL STDCALL
StartServiceCtrlDispatcherA(LPSERVICE_TABLE_ENTRYA lpServiceStartTable)
{
  ULONG i;
  HANDLE hPipe;
  DWORD dwError;
  PUCHAR lpMessageBuffer;

  DPRINT("StartServiceCtrlDispatcherA() called\n");

  i = 0;
  while (lpServiceStartTable[i].lpServiceProc != NULL)
    {
      i++;
    }

  dwActiveServiceCount = i;
  lpActiveServices = RtlAllocateHeap(RtlGetProcessHeap(),
				     HEAP_ZERO_MEMORY,
				     dwActiveServiceCount * sizeof(ACTIVE_SERVICE));
  if (lpActiveServices == NULL)
    {
      return FALSE;
    }

  /* Copy service names and start procedure */
  for (i = 0; i < dwActiveServiceCount; i++)
    {
      RtlCreateUnicodeStringFromAsciiz(&lpActiveServices[i].ServiceName,
				       lpServiceStartTable[i].lpServiceName);
      lpActiveServices[i].MainFunction = (LPSERVICE_MAIN_FUNCTIONW)lpServiceStartTable[i].lpServiceProc;
    }

  dwError = ScConnectControlPipe(&hPipe);
  if (dwError != ERROR_SUCCESS)
    {
      /* Free the service table */
      RtlFreeHeap(RtlGetProcessHeap(), 0, lpActiveServices);
      lpActiveServices = NULL;
      dwActiveServiceCount = 0;
      return FALSE;
    }

  lpMessageBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
				    HEAP_ZERO_MEMORY,
				    256);
  if (lpMessageBuffer == NULL)
    {
      /* Free the service table */
      RtlFreeHeap(RtlGetProcessHeap(), 0, lpActiveServices);
      lpActiveServices = NULL;
      dwActiveServiceCount = 0;
      CloseHandle(hPipe);
      return FALSE;
    }

  ScServiceDispatcher(hPipe, lpMessageBuffer, 256);
  CloseHandle(hPipe);

  /* Free the message buffer */
  RtlFreeHeap(RtlGetProcessHeap(), 0, lpMessageBuffer);

  /* Free the service table */
  RtlFreeHeap(RtlGetProcessHeap(), 0, lpActiveServices);
  lpActiveServices = NULL;
  dwActiveServiceCount = 0;

  return TRUE;
}


/**********************************************************************
 *	StartServiceCtrlDispatcherW
 *
 * @unimplemented
 */
BOOL STDCALL
StartServiceCtrlDispatcherW(LPSERVICE_TABLE_ENTRYW lpServiceStartTable)
{
  ULONG i;
  HANDLE hPipe;
  DWORD dwError;
  PUCHAR lpMessageBuffer;

  DPRINT("StartServiceCtrlDispatcherW() called\n");

  i = 0;
  while (lpServiceStartTable[i].lpServiceProc != NULL)
    {
      i++;
    }

  dwActiveServiceCount = i;
  lpActiveServices = RtlAllocateHeap(RtlGetProcessHeap(),
				     HEAP_ZERO_MEMORY,
				     dwActiveServiceCount * sizeof(ACTIVE_SERVICE));
  if (lpActiveServices == NULL)
    {
      return FALSE;
    }

  /* Copy service names and start procedure */
  for (i = 0; i < dwActiveServiceCount; i++)
    {
      RtlCreateUnicodeString(&lpActiveServices[i].ServiceName,
			     lpServiceStartTable[i].lpServiceName);
      lpActiveServices[i].MainFunction = lpServiceStartTable[i].lpServiceProc;
    }

  dwError = ScConnectControlPipe(&hPipe);
  if (dwError != ERROR_SUCCESS)
    {
      /* Free the service table */
      RtlFreeHeap(RtlGetProcessHeap(), 0, lpActiveServices);
      lpActiveServices = NULL;
      dwActiveServiceCount = 0;
      return FALSE;
    }

  lpMessageBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
				    HEAP_ZERO_MEMORY,
				    256);
  if (lpMessageBuffer == NULL)
    {
      /* Free the service table */
      RtlFreeHeap(RtlGetProcessHeap(), 0, lpActiveServices);
      lpActiveServices = NULL;
      dwActiveServiceCount = 0;
      CloseHandle(hPipe);
      return FALSE;
    }

  ScServiceDispatcher(hPipe, lpMessageBuffer, 256);
  CloseHandle(hPipe);

  /* Free the message buffer */
  RtlFreeHeap(RtlGetProcessHeap(), 0, lpMessageBuffer);

  /* Free the service table */
  RtlFreeHeap(RtlGetProcessHeap(), 0, lpActiveServices);
  lpActiveServices = NULL;
  dwActiveServiceCount = 0;

  return TRUE;
}

/* EOF */
