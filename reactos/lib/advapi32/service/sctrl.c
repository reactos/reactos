/* $Id: sctrl.c,v 1.3 2001/10/21 19:06:42 chorns Exp $
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

#include <windows.h>
#include <ddk/ntddk.h>

/* TYPES *********************************************************************/

typedef struct
{
  DWORD ThreadId;
  UNICODE_STRING ServiceName;
  SERVICE_STATUS ServiceStatus;
  LPHANDLER_FUNCTION	Handler;
} ACTIVE_SERVICE, *PACTIVE_SERVICE;

/* GLOBALS *******************************************************************/

static ULONG ActiveServiceCount;
static PACTIVE_SERVICE ActiveServices;
static PHANDLE ActiveServicesThreadHandles;

/* FUNCTIONS *****************************************************************/

PACTIVE_SERVICE
ScLookupServiceByThreadId(
  DWORD ThreadId)
{
  DWORD i;

  for (i = 0; i < ActiveServiceCount; i++)
  {
    if (ActiveServices[i].ThreadId == ThreadId)
    {
      return &ActiveServices[i];
    }
  }

  return NULL;
}


DWORD
WINAPI
ScServiceMainStub(
  LPVOID Context)
{
  LPSERVICE_MAIN_FUNCTION lpServiceProc = (LPSERVICE_MAIN_FUNCTION)Context;

  /* FIXME: Send argc and argv (from command line) as arguments */

  (lpServiceProc)(0, NULL);

  return ERROR_SUCCESS;
}


/**********************************************************************
 *	RegisterServiceCtrlHandlerA
 */
SERVICE_STATUS_HANDLE STDCALL RegisterServiceCtrlHandlerA(
	LPCSTR			lpServiceName,
	LPHANDLER_FUNCTION	lpHandlerProc)
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
   
   return(SHandle);
}


/**********************************************************************
 *	RegisterServiceCtrlHandlerW
 */
SERVICE_STATUS_HANDLE STDCALL RegisterServiceCtrlHandlerW(
	LPCWSTR			lpServiceName,
	LPHANDLER_FUNCTION	lpHandlerProc)
{
  PACTIVE_SERVICE Service;

  /* FIXME: Locate active service entry from name */

  Service = &ActiveServices[0];

  Service->Handler = lpHandlerProc;

	return (SERVICE_STATUS_HANDLE)Service->ThreadId;
}


/**********************************************************************
 *	SetServiceBits
 */
BOOL STDCALL SetServiceBits(SERVICE_STATUS_HANDLE hServiceStatus,
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
WINBOOL
STDCALL
SetServiceObjectSecurity(
	SC_HANDLE		hService,
	SECURITY_INFORMATION	dwSecurityInformation,
	PSECURITY_DESCRIPTOR	lpSecurityDescriptor
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	SetServiceStatus
 */
BOOL
STDCALL
SetServiceStatus(
	SERVICE_STATUS_HANDLE	hServiceStatus,
	LPSERVICE_STATUS	lpServiceStatus)
{
  PACTIVE_SERVICE Service;

  Service = ScLookupServiceByThreadId((DWORD)hServiceStatus);
  if (!Service)
  {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  RtlCopyMemory(
    &Service->ServiceStatus,
    lpServiceStatus, 
    sizeof(SERVICE_STATUS));

	return TRUE;
}

/**********************************************************************
 *	StartServiceCtrlDispatcherA
 */
BOOL STDCALL StartServiceCtrlDispatcherA(
  LPSERVICE_TABLE_ENTRYA	lpServiceStartTable)
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

  ServiceStartTableW = RtlAllocateHeap(
    RtlGetProcessHeap(),
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
BOOL STDCALL StartServiceCtrlDispatcherW(
	LPSERVICE_TABLE_ENTRYW	lpServiceStartTable)
{
  ULONG i;
  HANDLE h;
  DWORD Tid;
  DWORD r;

  i = 0;
  while (lpServiceStartTable[i].lpServiceProc != NULL)
  {
    i++;
  }

  ActiveServiceCount = i;
  ActiveServices = RtlAllocateHeap(
    RtlGetProcessHeap(),
   	HEAP_ZERO_MEMORY,
   	ActiveServiceCount * 
   	sizeof(ACTIVE_SERVICE));
  if (!ActiveServices)
  {
    return FALSE;
  }

  ActiveServicesThreadHandles = RtlAllocateHeap(
    RtlGetProcessHeap(),
   	HEAP_ZERO_MEMORY,
   	(ActiveServiceCount + 1) *
   	sizeof(HANDLE));
  if (!ActiveServicesThreadHandles)
  {
    RtlFreeHeap(RtlGetProcessHeap(), 0, ActiveServices);
    ActiveServices = NULL;
    return FALSE;
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
}


