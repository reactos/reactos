/* $Id: priv.c,v 1.7 2003/06/17 10:42:37 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              kernel/se/priv.c
 * PROGRAMER:         ?
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/se.h>

#include <internal/debug.h>


/* GLOBALS *******************************************************************/

LUID SeCreateTokenPrivilege;
LUID SeAssignPrimaryTokenPrivilege;
LUID SeLockMemoryPrivilege;
LUID SeIncreaseQuotaPrivilege;
LUID SeUnsolicitedInputPrivilege;
LUID SeTcbPrivilege;
LUID SeSecurityPrivilege;
LUID SeTakeOwnershipPrivilege;
LUID SeLoadDriverPrivilege;
LUID SeCreatePagefilePrivilege;
LUID SeIncreaseBasePriorityPrivilege;
LUID SeSystemProfilePrivilege;
LUID SeSystemtimePrivilege;
LUID SeProfileSingleProcessPrivilege;
LUID SeCreatePermanentPrivilege;
LUID SeBackupPrivilege;
LUID SeRestorePrivilege;
LUID SeShutdownPrivilege;
LUID SeDebugPrivilege;
LUID SeAuditPrivilege;
LUID SeSystemEnvironmentPrivilege;
LUID SeChangeNotifyPrivilege;
LUID SeRemoteShutdownPrivilege;


/* FUNCTIONS ***************************************************************/

VOID
SepInitPrivileges (VOID)
{
  SeCreateTokenPrivilege.LowPart = SE_CREATE_TOKEN_PRIVILEGE;
  SeCreateTokenPrivilege.HighPart = 0;
  SeAssignPrimaryTokenPrivilege.LowPart = SE_ASSIGNPRIMARYTOKEN_PRIVILEGE;
  SeAssignPrimaryTokenPrivilege.HighPart = 0;
  SeLockMemoryPrivilege.LowPart = SE_LOCK_MEMORY_PRIVILEGE;
  SeLockMemoryPrivilege.HighPart = 0;
  SeIncreaseQuotaPrivilege.LowPart = SE_INCREASE_QUOTA_PRIVILEGE;
  SeIncreaseQuotaPrivilege.HighPart = 0;
  SeUnsolicitedInputPrivilege.LowPart = SE_UNSOLICITED_INPUT_PRIVILEGE;
  SeUnsolicitedInputPrivilege.HighPart = 0;
  SeTcbPrivilege.LowPart = SE_TCB_PRIVILEGE;
  SeTcbPrivilege.HighPart = 0;
  SeSecurityPrivilege.LowPart = SE_SECURITY_PRIVILEGE;
  SeSecurityPrivilege.HighPart = 0;
  SeTakeOwnershipPrivilege.LowPart = SE_TAKE_OWNERSHIP_PRIVILEGE;
  SeTakeOwnershipPrivilege.HighPart = 0;
  SeLoadDriverPrivilege.LowPart = SE_LOAD_DRIVER_PRIVILEGE;
  SeLoadDriverPrivilege.HighPart = 0;
  SeSystemProfilePrivilege.LowPart = SE_SYSTEM_PROFILE_PRIVILEGE;
  SeSystemProfilePrivilege.HighPart = 0;
  SeSystemtimePrivilege.LowPart = SE_SYSTEMTIME_PRIVILEGE;
  SeSystemtimePrivilege.HighPart = 0;
  SeProfileSingleProcessPrivilege.LowPart = SE_PROF_SINGLE_PROCESS_PRIVILEGE;
  SeProfileSingleProcessPrivilege.HighPart = 0;
  SeIncreaseBasePriorityPrivilege.LowPart = SE_INC_BASE_PRIORITY_PRIVILEGE;
  SeIncreaseBasePriorityPrivilege.HighPart = 0;
  SeCreatePagefilePrivilege.LowPart = SE_CREATE_PAGEFILE_PRIVILEGE;
  SeCreatePagefilePrivilege.HighPart = 0;
  SeCreatePermanentPrivilege.LowPart = SE_CREATE_PERMANENT_PRIVILEGE;
  SeCreatePermanentPrivilege.HighPart = 0;
  SeBackupPrivilege.LowPart = SE_BACKUP_PRIVILEGE;
  SeBackupPrivilege.HighPart = 0;
  SeRestorePrivilege.LowPart = SE_RESTORE_PRIVILEGE;
  SeRestorePrivilege.HighPart = 0;
  SeShutdownPrivilege.LowPart = SE_SHUTDOWN_PRIVILEGE;
  SeShutdownPrivilege.HighPart = 0;
  SeDebugPrivilege.LowPart = SE_DEBUG_PRIVILEGE;
  SeDebugPrivilege.HighPart = 0;
  SeAuditPrivilege.LowPart = SE_AUDIT_PRIVILEGE;
  SeAuditPrivilege.HighPart = 0;
  SeSystemEnvironmentPrivilege.LowPart = SE_SYSTEM_ENVIRONMENT_PRIVILEGE;
  SeSystemEnvironmentPrivilege.HighPart = 0;
  SeChangeNotifyPrivilege.LowPart = SE_CHANGE_NOTIFY_PRIVILEGE;
  SeChangeNotifyPrivilege.HighPart = 0;
  SeRemoteShutdownPrivilege.LowPart = SE_REMOTE_SHUTDOWN_PRIVILEGE;
  SeRemoteShutdownPrivilege.HighPart = 0;
}


BOOLEAN
SepPrivilegeCheck (PACCESS_TOKEN Token,
		   PLUID_AND_ATTRIBUTES Privileges,
		   ULONG PrivilegeCount,
		   ULONG PrivilegeControl,
		   KPROCESSOR_MODE PreviousMode)
{
  ULONG i;
  ULONG j;
  ULONG k;

  DPRINT ("SepPrivilegeCheck() called\n");

  if (PreviousMode == KernelMode)
    {
      return TRUE;
    }

  k = 0;
  if (PrivilegeCount > 0)
    {
      for (i = 0; i < Token->PrivilegeCount; i++)
	{
	  for (j = 0; j < PrivilegeCount; j++)
	    {
	      if (Token->Privileges[i].Luid.LowPart == Privileges[j].Luid.LowPart &&
		  Token->Privileges[i].Luid.HighPart == Privileges[j].Luid.HighPart)
		{
		  DPRINT ("Found privilege\n");
		  DPRINT ("Privilege attributes %lx\n",
			  Token->Privileges[i].Attributes);

		  if (Token->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED)
		    {
		      Privileges[j].Attributes |= SE_PRIVILEGE_USED_FOR_ACCESS;
		      k++;
		    }
		}
	    }
	}
    }

  if ((PrivilegeControl & PRIVILEGE_SET_ALL_NECESSARY) &&
      PrivilegeCount == k)
    {
      return TRUE;
    }

  if (k > 0 &&
      !(PrivilegeControl & PRIVILEGE_SET_ALL_NECESSARY))
    {
      return TRUE;
    }

  return FALSE;
}


NTSTATUS
SeCaptureLuidAndAttributesArray (PLUID_AND_ATTRIBUTES Src,
				 ULONG PrivilegeCount,
				 KPROCESSOR_MODE PreviousMode,
				 PLUID_AND_ATTRIBUTES AllocatedMem,
				 ULONG AllocatedLength,
				 POOL_TYPE PoolType,
				 ULONG d,
				 PLUID_AND_ATTRIBUTES* Dest,
				 PULONG Length)
{
  PLUID_AND_ATTRIBUTES* NewMem;
  ULONG SrcLength;

  if (PrivilegeCount == 0)
    {
      *Dest = 0;
      *Length = 0;
      return STATUS_SUCCESS;
    }

  if (PreviousMode == KernelMode && d == 0)
    {
      *Dest = Src;
      return STATUS_SUCCESS;
    }

  SrcLength = ((PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES)) + 3) & 0xfc;
  *Length = SrcLength;
  if (AllocatedMem == NULL)
    {
      NewMem = ExAllocatePool (PoolType,
			       SrcLength);
      *Dest = (PLUID_AND_ATTRIBUTES)NewMem;
      if (NewMem == NULL)
	{
	  return STATUS_UNSUCCESSFUL;
	}
    }
  else
    {
      if (SrcLength > AllocatedLength)
	{
	  return STATUS_UNSUCCESSFUL;
	}
      *Dest = AllocatedMem;
    }
  memmove (*Dest, Src, SrcLength);

  return STATUS_SUCCESS;
}


VOID
SeReleaseLuidAndAttributesArray (PLUID_AND_ATTRIBUTES Privilege,
				 KPROCESSOR_MODE PreviousMode,
				 ULONG a)
{
  ExFreePool (Privilege);
}


NTSTATUS STDCALL
NtPrivilegeCheck (IN HANDLE ClientToken,
		  IN PPRIVILEGE_SET RequiredPrivileges,
		  IN PBOOLEAN Result)
{
  PLUID_AND_ATTRIBUTES Privilege;
  PACCESS_TOKEN Token;
  ULONG PrivilegeCount;
  ULONG PrivilegeControl;
  ULONG Length;
  NTSTATUS Status;

  Status = ObReferenceObjectByHandle (ClientToken,
				      0,
				      SepTokenObjectType,
				      UserMode,
				      (PVOID*)&Token,
				      NULL);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  if (Token->TokenType == TokenImpersonation &&
      Token->ImpersonationLevel < SecurityAnonymous)
    {
      ObDereferenceObject (Token);
      return STATUS_UNSUCCESSFUL;
    }

  PrivilegeCount = RequiredPrivileges->PrivilegeCount;
  PrivilegeControl = RequiredPrivileges->Control;
  Privilege = 0;
  Status = SeCaptureLuidAndAttributesArray (RequiredPrivileges->Privilege,
					    PrivilegeCount,
					    1,
					    0,
					    0,
					    1,
					    1,
					    &Privilege,
					    &Length);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject (Token);
      return STATUS_UNSUCCESSFUL;
    }

  *Result = SepPrivilegeCheck (Token,
			       Privilege,
			       PrivilegeCount,
			       PrivilegeControl,
			       UserMode);

  memmove (RequiredPrivileges->Privilege,
	   Privilege,
	   Length);

  SeReleaseLuidAndAttributesArray (Privilege,
				   UserMode,
				   1);

  return STATUS_SUCCESS;
}


BOOLEAN STDCALL
SePrivilegeCheck (PPRIVILEGE_SET Privileges,
		  PSECURITY_SUBJECT_CONTEXT SubjectContext,
		  KPROCESSOR_MODE PreviousMode)
{
  PACCESS_TOKEN Token = NULL;

  if (SubjectContext->ClientToken == NULL)
    {
      Token = SubjectContext->PrimaryToken;
    }
  else
    {
      Token = SubjectContext->ClientToken;
      if (SubjectContext->ImpersonationLevel < 2)
	{
	  return FALSE;
	}
    }

  return SepPrivilegeCheck (Token,
			    Privileges->Privilege,
			    Privileges->PrivilegeCount,
			    Privileges->Control,
			    PreviousMode);
}


BOOLEAN STDCALL
SeSinglePrivilegeCheck (IN LUID PrivilegeValue,
			IN KPROCESSOR_MODE PreviousMode)
{
  SECURITY_SUBJECT_CONTEXT SubjectContext;
  PRIVILEGE_SET Priv;
  BOOLEAN Result;

  SeCaptureSubjectContext (&SubjectContext);

  Priv.PrivilegeCount = 1;
  Priv.Control = PRIVILEGE_SET_ALL_NECESSARY;
  Priv.Privilege[0].Luid = PrivilegeValue;
  Priv.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

  Result = SePrivilegeCheck (&Priv,
			     &SubjectContext,
			     PreviousMode);

  if (PreviousMode != KernelMode)
    {
#if 0
      SePrivilegedServiceAuditAlarm (0,
				     &SubjectContext,
				     &PrivilegeValue);
#endif
    }

  SeReleaseSubjectContext (&SubjectContext);

  return Result;
}

/* EOF */
