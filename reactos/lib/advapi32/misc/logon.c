/* $Id: logon.c,v 1.1 2004/01/20 01:40:18 ekohl Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/advapi32/misc/logon.c
 * PURPOSE:     Logon functions
 * PROGRAMMER:  Eric Kohl
 */

#include <windows.h>

#define NTOS_MODE_USER
#include <ntos.h>

/* FUNCTIONS ***************************************************************/

/*
 * @unimplemented
 */
BOOL STDCALL
CreateProcessAsUserA (HANDLE hToken,
		      LPCSTR lpApplicationName,
		      LPSTR lpCommandLine,
		      LPSECURITY_ATTRIBUTES lpProcessAttributes,
		      LPSECURITY_ATTRIBUTES lpThreadAttributes,
		      BOOL bInheritHandles,
		      DWORD dwCreationFlags,
		      LPVOID lpEnvironment,
		      LPCSTR lpCurrentDirectory,
		      LPSTARTUPINFOA lpStartupInfo,
		      LPPROCESS_INFORMATION lpProcessInformation)
{
  /* redirect call to CreateProcess() as long as we don't support user logins */
  return CreateProcessA (lpApplicationName,
			 lpCommandLine,
			 lpProcessAttributes,
			 lpThreadAttributes,
			 bInheritHandles,
			 dwCreationFlags,
			 lpEnvironment,
			 lpCurrentDirectory,
			 lpStartupInfo,
			 lpProcessInformation);
}


/*
 * @unimplemented
 */
BOOL STDCALL
CreateProcessAsUserW (HANDLE hToken,
		      LPCWSTR lpApplicationName,
		      LPWSTR lpCommandLine,
		      LPSECURITY_ATTRIBUTES lpProcessAttributes,
		      LPSECURITY_ATTRIBUTES lpThreadAttributes,
		      BOOL bInheritHandles,
		      DWORD dwCreationFlags,
		      LPVOID lpEnvironment,
		      LPCWSTR lpCurrentDirectory,
		      LPSTARTUPINFOW lpStartupInfo,
		      LPPROCESS_INFORMATION lpProcessInformation)
{
  /* redirect call to CreateProcess() as long as we don't support user logins */
  return CreateProcessW (lpApplicationName,
			 lpCommandLine,
			 lpProcessAttributes,
			 lpThreadAttributes,
			 bInheritHandles,
			 dwCreationFlags,
			 lpEnvironment,
			 lpCurrentDirectory,
			 lpStartupInfo,
			 lpProcessInformation);
}


/*
 * @unimplemented
 */
BOOL STDCALL
CreateProcessWithLogonW (LPCWSTR lpUsername,
			 LPCWSTR lpDomain,
			 LPCWSTR lpPassword,
			 DWORD dwLogonFlags,
			 LPCWSTR lpApplicationName,
			 LPWSTR lpCommandLine,
			 DWORD dwCreationFlags,
			 LPVOID lpEnvironment,
			 LPCWSTR lpCurrentDirectory,
			 LPSTARTUPINFOW lpStartupInfo,
			 LPPROCESS_INFORMATION lpProcessInformation)
{
  return(FALSE);
}


/*
 * @unimplemented
 */
BOOL STDCALL
LogonUserA (LPCSTR lpszUsername,
	    LPCSTR lpszDomain,
	    LPCSTR lpszPassword,
	    DWORD dwLogonType,
	    DWORD dwLogonProvider,
	    PHANDLE phToken)
{
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
LogonUserW (LPCWSTR lpszUsername,
	    LPCWSTR lpszDomain,
	    LPCWSTR lpszPassword,
	    DWORD dwLogonType,
	    DWORD dwLogonProvider,
	    PHANDLE phToken)
{
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
LogonUserExA (LPCSTR lpszUsername,
	      LPCSTR lpszDomain,
	      LPCSTR lpszPassword,
	      DWORD dwLogonType,
	      DWORD dwLogonProvider,
	      PHANDLE phToken,
	      PSID *ppLogonSid,
	      PVOID *ppProfileBuffer,
	      LPDWORD pdwProfileLength,
	      PQUOTA_LIMITS pQuotaLimits)
{
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
LogonUserExW (LPCWSTR lpszUsername,
	      LPCWSTR lpszDomain,
	      LPCWSTR lpszPassword,
	      DWORD dwLogonType,
	      DWORD dwLogonProvider,
	      PHANDLE phToken,
	      PSID *ppLogonSid,
	      PVOID *ppProfileBuffer,
	      LPDWORD pdwProfileLength,
	      PQUOTA_LIMITS pQuotaLimits)
{
  return FALSE;
}

/* EOF */
