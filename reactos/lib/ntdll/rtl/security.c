/* $Id: security.c,v 1.10 2003/07/11 13:50:23 royce Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/rtl/security.c
 * PURPOSE:         Miscellaneous securitiy related functions
 * PROGRAMMER:      Eric Kohl
 * UPDATE HISTORY:
 *                  21/11/2001 Created
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

#define NDEBUG
#include <ntdll/ntdll.h>


/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
NTSTATUS STDCALL
RtlImpersonateSelf(IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  SECURITY_QUALITY_OF_SERVICE SecQos;
  HANDLE ProcessToken;
  HANDLE ImpersonationToken;
  NTSTATUS Status;

  Status = NtOpenProcessToken(NtCurrentProcess(),
			      TOKEN_DUPLICATE,
			      &ProcessToken);
  if (!NT_SUCCESS(Status))
    return(Status);

  SecQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
  SecQos.ImpersonationLevel = ImpersonationLevel;
  SecQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
  SecQos.EffectiveOnly = FALSE;

  ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
  ObjectAttributes.RootDirectory = 0;
  ObjectAttributes.ObjectName = NULL;
  ObjectAttributes.Attributes = 0;
  ObjectAttributes.SecurityDescriptor = NULL;
  ObjectAttributes.SecurityQualityOfService = &SecQos;

  Status = NtDuplicateToken(ProcessToken,
			    TOKEN_IMPERSONATE,
			    &ObjectAttributes,
			    0,
			    TokenImpersonation,
			    &ImpersonationToken);
  if (!NT_SUCCESS(Status))
    {
      NtClose(ProcessToken);
      return(Status);
    }

  Status = NtSetInformationThread(NtCurrentThread(),
				  ThreadImpersonationToken,
				  &ImpersonationToken,
				  sizeof(HANDLE));
  NtClose(ImpersonationToken);
  NtClose(ProcessToken);

  return(Status);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlAdjustPrivilege(IN ULONG Privilege,
		   IN BOOLEAN Enable,
		   IN BOOLEAN CurrentThread,
		   OUT PBOOLEAN Enabled)
{
  TOKEN_PRIVILEGES NewState;
  TOKEN_PRIVILEGES OldState;
  ULONG ReturnLength;
  HANDLE TokenHandle;
  NTSTATUS Status;

  DPRINT ("RtlAdjustPrivilege() called\n");

  if (CurrentThread)
    {
      Status = NtOpenThreadToken (NtCurrentThread (),
				  TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
				  FALSE,
				  &TokenHandle);
    }
  else
    {
      Status = NtOpenProcessToken (NtCurrentProcess (),
				   TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
				   &TokenHandle);
    }

  if (!NT_SUCCESS (Status))
    {
      DPRINT1 ("Retrieving token handle failed (Status %lx)\n", Status);
      return Status;
    }

  OldState.PrivilegeCount = 1;

  NewState.PrivilegeCount = 1;
  NewState.Privileges[0].Luid.LowPart = Privilege;
  NewState.Privileges[0].Luid.HighPart = 0;
  NewState.Privileges[0].Attributes = (Enable) ? SE_PRIVILEGE_ENABLED : 0;

  Status = NtAdjustPrivilegesToken (TokenHandle,
				    FALSE,
				    &NewState,
				    sizeof(TOKEN_PRIVILEGES),
				    &OldState,
				    &ReturnLength);
  NtClose (TokenHandle);
  if (Status == STATUS_NOT_ALL_ASSIGNED)
    {
      DPRINT1 ("Failed to assign all privileges\n");
      return STATUS_PRIVILEGE_NOT_HELD;
    }
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("NtAdjustPrivilegesToken() failed (Status %lx)\n", Status);
      return Status;
    }

  if (OldState.PrivilegeCount == 0)
    {
      *Enabled = Enable;
    }
  else
    {
      *Enabled = (OldState.Privileges[0].Attributes & SE_PRIVILEGE_ENABLED);
    }

  DPRINT ("RtlAdjustPrivilege() done\n");

  return STATUS_SUCCESS;
}

/* EOF */
