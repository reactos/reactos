/* $Id: sctrl.c,v 1.2 2001/06/25 14:19:56 ekohl Exp $
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
} ACTIVE_SERVICE, *PACTIVE_SERVICE;

/* GLOBALS *******************************************************************/

static ULONG ActiveServiceCount;
static PACTIVE_SERVICE ActiveServices;
static PHANDLE ActiveServicesThreadHandles;

/* FUNCTIONS *****************************************************************/

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
   RtlAnsiStringToUnicodeString(&ServiceNameU, &ServiceNameA, TRUE);
   
   SHandle = RegisterServiceCtrlHandlerW(ServiceNameU.Buffer,
					 lpHandlerProc);
   
   RtlFreeHeap(RtlGetProcessHeap(), 0, ServiceNameU.Buffer);
   
   return(SHandle);
}


/**********************************************************************
 *	RegisterServiceCtrlHandlerW
 */
SERVICE_STATUS_HANDLE STDCALL RegisterServiceCtrlHandlerW(
	LPCWSTR			lpServiceName,
	LPHANDLER_FUNCTION	lpHandlerProc)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
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
	LPSERVICE_STATUS	lpServiceStatus
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
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
   ULONG i;
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
	RtlInitAnsiString(&ServiceNameA,
			  lpServiceStartTable[i].lpServiceName);
	RtlAnsiStringToUnicodeString(&ServiceNameW,
				     &ServiceNameA,
				     TRUE);
	ServiceStartTableW[i].lpServiceName = ServiceNameW.Buffer;
	ServiceStartTableW[i].lpServiceProc = 
	  lpServiceStartTable[i].lpServiceProc;
     }
   
   b = StartServiceCtrlDispatcherW(ServiceStartTableW);
   
   for (i = 0; i < Count; i++)
     {
	RtlFreeHeap(RtlGetProcessHeap(),
		    0,
		    ServiceStartTableW[i].lpServiceName);
     }
   RtlFreeHeap(RtlGetProcessHeap(),
	       0,
	       ServiceStartTableW);
   
   return(b);
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
   ActiveServices = RtlAllocateHeap(GetProcessHeap(),
				    HEAP_ZERO_MEMORY,
				    ActiveServiceCount * 
				    sizeof(ACTIVE_SERVICE));
   ActiveServicesThreadHandles = RtlAllocateHeap(GetProcessHeap(),
						 HEAP_ZERO_MEMORY,
						 (ActiveServiceCount + 1) *
						 sizeof(HANDLE));
   
   for (i = 0; i<ActiveServiceCount; i++)
     {
	h = CreateThread(NULL,
			 0,
			 (LPTHREAD_START_ROUTINE)lpServiceStartTable[i].lpServiceProc,
			 NULL,
			 0,
			 &Tid);
	if (h == INVALID_HANDLE_VALUE)
	  {
	     return(FALSE);
	  }
	ActiveServicesThreadHandles[i + 1] = h;
	ActiveServices[i].ThreadId = Tid;
     }
   
   while (ActiveServiceCount > 0)
     {
	r = WaitForMultipleObjects(ActiveServiceCount + 1,
				   ActiveServicesThreadHandles,
				   FALSE,
				   INFINITE);
	if (r == WAIT_OBJECT_0)
	  {
	     /* Received message from the scm */
	  }
	else if (r > WAIT_OBJECT_0 &&
		 r < (WAIT_OBJECT_0 + ActiveServiceCount))
	  {
	     /* A service died */
	     
	     ActiveServiceCount--;
	     ActiveServicesThreadHandles[r - WAIT_OBJECT_0 - 1] =
	       ActiveServicesThreadHandles[ActiveServiceCount + 1];
	     memcpy(&ActiveServices[r - WAIT_OBJECT_0 - 2],
		    &ActiveServices[ActiveServiceCount],
		    sizeof(ACTIVE_SERVICE));
	  }
	else
	  {
	     /* Bail */
	  }
     }
   return(TRUE);
}


