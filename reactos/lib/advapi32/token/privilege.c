/* $Id: privilege.c,v 1.8 2004/06/17 09:07:12 ekohl Exp $ 
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/token/privilege.c
 * PURPOSE:         advapi32.dll token's privilege handling
 * PROGRAMMER:      E.Aliberti
 * UPDATE HISTORY:
 *	20010317 ea	stubs
 */

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>


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
