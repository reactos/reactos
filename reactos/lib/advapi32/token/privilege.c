/* $Id: privilege.c,v 1.6 2004/02/25 14:25:11 ekohl Exp $ 
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/token/privilege.c
 * PURPOSE:         advapi32.dll token's privilege handling
 * PROGRAMMER:      E.Aliberti
 * UPDATE HISTORY:
 *	20010317 ea	stubs
 */
#include <windows.h>
#include <ddk/ntddk.h>


/**********************************************************************
 *	LookupPrivilegeValueA				EXPORTED
 *
 * @implemented
 */
BOOL STDCALL
LookupPrivilegeValueA (LPCSTR lpSystemName,
		       LPCSTR lpName,
		       PLUID lpLuid)
{
  UNICODE_STRING SystemName;
  UNICODE_STRING Name;
  BOOL Result;

  /* Remote system? */
  if (lpSystemName != NULL)
    {
      RtlCreateUnicodeStringFromAsciiz (&SystemName,
					(LPSTR)lpSystemName);
    }

  /* Check the privilege name is not NULL */
  if (lpName == NULL)
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  RtlCreateUnicodeStringFromAsciiz (&Name,
				    (LPSTR)lpName);

  Result = LookupPrivilegeValueW ((lpSystemName != NULL) ? SystemName.Buffer : NULL,
				  Name.Buffer,
				  lpLuid);

  RtlFreeUnicodeString (&Name);

  /* Remote system? */
  if (lpSystemName != NULL)
    {
      RtlFreeUnicodeString (&SystemName);
    }

  return Result;
}


/**********************************************************************
 *	LookupPrivilegeValueW				EXPORTED
 *
 * @unimplemented
 */
BOOL STDCALL
LookupPrivilegeValueW (LPCWSTR lpSystemName,
		       LPCWSTR lpName,
		       PLUID lpLuid)
{
  SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/**********************************************************************
 *	LookupPrivilegeDisplayNameA			EXPORTED
 *
 * @unimplemented
 */
BOOL STDCALL
LookupPrivilegeDisplayNameA (LPCSTR lpSystemName,
			     LPCSTR lpName,
			     LPSTR lpDisplayName,
			     LPDWORD cbDisplayName,
			     LPDWORD lpLanguageId)
{
  SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/**********************************************************************
 *	LookupPrivilegeDisplayNameW			EXPORTED
 *
 * @unimplemented
 */
BOOL STDCALL
LookupPrivilegeDisplayNameW (LPCWSTR lpSystemName,
			     LPCWSTR lpName,
			     LPWSTR lpDisplayName,
			     LPDWORD cbDisplayName,
			     LPDWORD lpLanguageId)
{
  SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/**********************************************************************
 *	LookupPrivilegeNameA				EXPORTED
 *
 * @unimplemented
 */
BOOL STDCALL
LookupPrivilegeNameA (LPCSTR lpSystemName,
		      PLUID lpLuid,
		      LPSTR lpName,
		      LPDWORD cbName)
{
  SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/**********************************************************************
 *	LookupPrivilegeNameW				EXPORTED
 *
 * @unimplemented
 */
BOOL STDCALL
LookupPrivilegeNameW (LPCWSTR lpSystemName,
		      PLUID lpLuid,
		      LPWSTR lpName,
		      LPDWORD cbName)
{
  SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/**********************************************************************
 *	PrivilegeCheck					EXPORTED
 *
 * @implemented
 */
BOOL STDCALL
PrivilegeCheck (HANDLE ClientToken,
		PPRIVILEGE_SET RequiredPrivileges,
		LPBOOL pfResult)
{
  BOOLEAN Result;
  NTSTATUS Status;

  Status = NtPrivilegeCheck (ClientToken,
			     RequiredPrivileges,
			     &Result);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  *pfResult = (BOOL) Result;

  return TRUE;
}

/* EOF */
