/* $Id: scm.c,v 1.6 2001/06/17 20:36:35 ea Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/service/scm.c
 * PURPOSE:         Service control manager functions
 * PROGRAMMER:      Emanuele Aliberti
 * UPDATE HISTORY:
 *	19990413 EA	created
 *	19990515 EA
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>

/* FUNCTIONS *****************************************************************/

/**********************************************************************
 *	ChangeServiceConfigA
 */
BOOL
STDCALL
ChangeServiceConfigA(
	SC_HANDLE	hService,
	DWORD		dwServiceType,
	DWORD		dwStartType,
	DWORD		dwErrorControl,
	LPCSTR		lpBinaryPathName,
	LPCSTR		lpLoadOrderGroup,
	LPDWORD		lpdwTagId,
	LPCSTR		lpDependencies,
	LPCSTR		lpServiceStartName,
	LPCSTR		lpPassword,
	LPCSTR		lpDisplayName 
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	ChangeServiceConfigW
 */
BOOL
STDCALL
ChangeServiceConfigW(
	SC_HANDLE	hService,
	DWORD		dwServiceType,
	DWORD		dwStartType,
	DWORD		dwErrorControl,
	LPCWSTR		lpBinaryPathName,
	LPCWSTR		lpLoadOrderGroup,
	LPDWORD		lpdwTagId,
	LPCWSTR		lpDependencies,
	LPCWSTR		lpServiceStartName,
	LPCWSTR		lpPassword,
	LPCWSTR		lpDisplayName 
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	CloseServiceHandle
 */
BOOL 
STDCALL
CloseServiceHandle( SC_HANDLE hSCObject )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	ControlService
 */
BOOL
STDCALL
ControlService(
	SC_HANDLE		hService,
	DWORD			dwControl,
	LPSERVICE_STATUS	lpServiceStatus
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	CreateServiceA
 */
SC_HANDLE
STDCALL
CreateServiceA(
	SC_HANDLE	hSCManager,
	LPCTSTR		lpServiceName,
	LPCTSTR		lpDisplayName,
	DWORD		dwDesiredAccess,
	DWORD		dwServiceType,
	DWORD		dwStartType,
	DWORD		dwErrorControl,
	LPCTSTR		lpBinaryPathName,
	LPCTSTR		lpLoadOrderGroup,
	LPDWORD		lpdwTagId,
	LPCTSTR		lpDependencies,
	LPCTSTR		lpServiceStartName,
	LPCTSTR		lpPassword
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return NULL;
}


/**********************************************************************
 *	CreateServiceW
 */
SC_HANDLE
STDCALL
CreateServiceW(
	SC_HANDLE	hSCManager,
	LPCWSTR		lpServiceName,
	LPCWSTR		lpDisplayName,
	DWORD		dwDesiredAccess,
	DWORD		dwServiceType,
	DWORD		dwStartType,
	DWORD		dwErrorControl,
	LPCWSTR		lpBinaryPathName,
	LPCWSTR		lpLoadOrderGroup,
	LPDWORD		lpdwTagId,
	LPCWSTR		lpDependencies,
	LPCWSTR		lpServiceStartName,
	LPCWSTR		lpPassword
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return NULL;
}


/**********************************************************************
 *	DeleteService
 */
BOOL
STDCALL
DeleteService( SC_HANDLE hService )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	EnumDependentServicesA
 */
BOOL
STDCALL
EnumDependentServicesA(
	SC_HANDLE		hService,
	DWORD			dwServiceState,
	LPENUM_SERVICE_STATUSA	lpServices,
	DWORD			cbBufSize,
	LPDWORD			pcbBytesNeeded,
	LPDWORD			lpServicesReturned
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	EnumDependentServicesW
 */
BOOL
STDCALL
EnumDependentServicesW(
	SC_HANDLE		hService,
	DWORD			dwServiceState,
	LPENUM_SERVICE_STATUSW	lpServices,
	DWORD			cbBufSize,
	LPDWORD			pcbBytesNeeded,
	LPDWORD			lpServicesReturned
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	EnumServiceGroupW
 *
 * (unknown)
 */
BOOL
STDCALL
EnumServiceGroupW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6,
	DWORD	Unknown7,
	DWORD	Unknown8
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	EnumServicesStatusA
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
	LPDWORD                 lpResumeHandle
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	EnumServicesStatusExA
 */
BOOL
STDCALL
EnumServicesStatusExA(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	EnumServicesStatusExW
 */
BOOL
STDCALL
EnumServicesStatusExW(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	EnumServicesStatusW
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
	LPDWORD                 lpResumeHandle
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	GetServiceDisplayNameA
 */
BOOL
STDCALL
GetServiceDisplayNameA(
	SC_HANDLE	hSCManager,
	LPCSTR		lpServiceName,
	LPSTR		lpDisplayName,
	LPDWORD		lpcchBuffer
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	GetServiceDisplayNameW
 */
BOOL
STDCALL
GetServiceDisplayNameW(
	SC_HANDLE	hSCManager,
	LPCWSTR		lpServiceName,
	LPWSTR		lpDisplayName,
	LPDWORD		lpcchBuffer
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	GetServiceKeyNameA
 */
BOOL
STDCALL
GetServiceKeyNameA(
	SC_HANDLE	hSCManager,
	LPCSTR		lpDisplayName,
	LPSTR		lpServiceName,
	LPDWORD		lpcchBuffer
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	GetServiceKeyNameW
 */
BOOL
STDCALL
GetServiceKeyNameW(
	SC_HANDLE	hSCManager,
	LPCWSTR		lpDisplayName,
	LPWSTR		lpServiceName,
	LPDWORD		lpcchBuffer
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/**********************************************************************
 *	LockServiceDatabase
 */
SC_LOCK
STDCALL
LockServiceDatabase(
	SC_HANDLE	hSCManager
	)
{
	SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
	return NULL;
}


/**********************************************************************
 *	OpenSCManagerA
 */
SC_HANDLE STDCALL OpenSCManagerA(LPCSTR	lpMachineName,
				 LPCSTR	lpDatabaseName,
				 DWORD	dwDesiredAccess)
{
   SC_HANDLE h;
   UNICODE_STRING MachineNameW;
   UNICODE_STRING DatabaseNameW;
   ANSI_STRING MachineNameA;
   ANSI_STRING DatabaseNameA;
   
   RtlInitAnsiString(&MachineNameA, lpMachineName);
   RtlAnsiStringToUnicodeString(&MachineNameW,
				&MachineNameA,
				TRUE);
   RtlInitAnsiString(&DatabaseNameA, lpDatabaseName);
   RtlAnsiStringToUnicodeString(&DatabaseNameW,
				&DatabaseNameA,
				TRUE);
   
   h = OpenSCManagerW(MachineNameW.Buffer,
		      DatabaseNameW.Buffer,
		      dwDesiredAccess);
   
   RtlFreeHeap(GetProcessHeap(),
	       0,
	       MachineNameW.Buffer);
   RtlFreeHeap(GetProcessHeap(),
	       0,
	       DatabaseNameW.Buffer);
   
   return(h);
}


/**********************************************************************
 *	OpenSCManagerW
 */
SC_HANDLE STDCALL OpenSCManagerW(LPCWSTR	lpMachineName,
				 LPCWSTR	lpDatabaseName,			
				 DWORD	dwDesiredAccess)
{
   HANDLE h;   
   
   if (lpMachineName == NULL ||
       wcslen(lpMachineName) == 0)
     {
	if (lpDatabaseName != NULL &&
	    wcscmp(lpDatabaseName, SERVICES_ACTIVE_DATABASEW) != 0)
	  {
	     return(NULL);
	  }
	
	h = CreateFile(L"\\\\.\\pipe\\ntsrvctrl",
		       dwDesiredAccess,
		       0,
		       NULL,
		       OPEN_EXISTING,
		       0,
		       NULL);
	if (h == INVALID_HANDLE_VALUE)
	  {
	     return(NULL);
	  }
	
	return(h);
     }
   else
     {
	return(NULL);
     }
}


/**********************************************************************
 *	OpenServiceA
 */
SC_HANDLE
STDCALL
OpenServiceA(
	SC_HANDLE	hSCManager,
	LPCSTR		lpServiceName,
	DWORD		dwDesiredAccess
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return NULL;
}


/**********************************************************************
 *	OpenServiceW
 */
SC_HANDLE
STDCALL
OpenServiceW(
	SC_HANDLE	hSCManager,
	LPCWSTR		lpServiceName,
	DWORD		dwDesiredAccess
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return NULL;
}


/**********************************************************************
 *	PrivilegedServiceAuditAlarmA
 */
BOOL
STDCALL
PrivilegedServiceAuditAlarmA(
	LPCSTR		SubsystemName,
	LPCSTR		ServiceName,
	HANDLE		ClientToken,
	PPRIVILEGE_SET	Privileges,
	BOOL		AccessGranted 
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	PrivilegedServiceAuditAlarmW
 */
BOOL
STDCALL
PrivilegedServiceAuditAlarmW(
	LPCWSTR		SubsystemName,
	LPCWSTR		ServiceName,
	HANDLE		ClientToken,
	PPRIVILEGE_SET	Privileges,
	BOOL		AccessGranted 
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 1;
}


/**********************************************************************
 *	QueryServiceConfigA
 */
BOOL
STDCALL
QueryServiceConfigA(
	SC_HANDLE		hService,
	LPQUERY_SERVICE_CONFIGA	lpServiceConfig,
	DWORD			cbBufSize,
	LPDWORD			pcbBytesNeeded
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	QueryServiceConfigW
 */
BOOL
STDCALL
QueryServiceConfigW(
	SC_HANDLE		hService,
	LPQUERY_SERVICE_CONFIGW lpServiceConfig,
	DWORD                   cbBufSize,
	LPDWORD                 pcbBytesNeeded
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	QueryServiceLockStatusA
 */
BOOL
STDCALL
QueryServiceLockStatusA(
	SC_HANDLE			hSCManager,
	LPQUERY_SERVICE_LOCK_STATUSA	lpLockStatus,
	DWORD				cbBufSize,
	LPDWORD				pcbBytesNeeded
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	QueryServiceLockStatusW
 */
BOOL
STDCALL
QueryServiceLockStatusW(
	SC_HANDLE			hSCManager,
	LPQUERY_SERVICE_LOCK_STATUSW	lpLockStatus,
	DWORD				cbBufSize,
	LPDWORD				pcbBytesNeeded
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	QueryServiceObjectSecurity
 */
BOOL
STDCALL
QueryServiceObjectSecurity(
	SC_HANDLE		hService,
	SECURITY_INFORMATION	dwSecurityInformation,
	PSECURITY_DESCRIPTOR	lpSecurityDescriptor,
	DWORD			cbBufSize,
	LPDWORD			pcbBytesNeeded
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	QueryServiceStatus
 */
BOOL
STDCALL
QueryServiceStatus(
	SC_HANDLE		hService,
	LPSERVICE_STATUS	lpServiceStatus
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	QueryServiceStatusEx
 */
BOOL
STDCALL
QueryServiceStatusEx(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	StartServiceA
 */
BOOL
STDCALL
StartServiceA(
	SC_HANDLE	hService,
	DWORD		dwNumServiceArgs,
	LPCSTR		*lpServiceArgVectors
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}




/**********************************************************************
 *	StartServiceW
 */
BOOL
STDCALL
StartServiceW(
	SC_HANDLE	hService,
	DWORD		dwNumServiceArgs,
	LPCWSTR		*lpServiceArgVectors
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 *	UnlockServiceDatabase
 */
BOOL
STDCALL
UnlockServiceDatabase(
	SC_LOCK	ScLock
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/* EOF */
