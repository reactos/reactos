/* $Id: privilege.c,v 1.7 2004/03/25 11:30:07 ekohl Exp $ 
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
