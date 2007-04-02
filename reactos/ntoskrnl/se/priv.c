/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/priv.c
 * PURPOSE:         Security manager
 *
 * PROGRAMMERS:     No programmer listed.
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, SepInitPrivileges)
#endif


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
LUID SeUndockPrivilege;
LUID SeSyncAgentPrivilege;
LUID SeEnableDelegationPrivilege;


/* FUNCTIONS ***************************************************************/

VOID
INIT_FUNCTION
NTAPI
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
  SeUndockPrivilege.LowPart = SE_UNDOCK_PRIVILEGE;
  SeUndockPrivilege.HighPart = 0;
  SeSyncAgentPrivilege.LowPart = SE_SYNC_AGENT_PRIVILEGE;
  SeSyncAgentPrivilege.HighPart = 0;
  SeEnableDelegationPrivilege.LowPart = SE_ENABLE_DELEGATION_PRIVILEGE;
  SeEnableDelegationPrivilege.HighPart = 0;
}


BOOLEAN
NTAPI
SepPrivilegeCheck (PTOKEN Token,
		   PLUID_AND_ATTRIBUTES Privileges,
		   ULONG PrivilegeCount,
		   ULONG PrivilegeControl,
		   KPROCESSOR_MODE PreviousMode)
{
  ULONG i;
  ULONG j;
  ULONG k;

  DPRINT ("SepPrivilegeCheck() called\n");

  PAGED_CODE();

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
NTAPI
SeCaptureLuidAndAttributesArray (PLUID_AND_ATTRIBUTES Src,
				 ULONG PrivilegeCount,
				 KPROCESSOR_MODE PreviousMode,
				 PLUID_AND_ATTRIBUTES AllocatedMem,
				 ULONG AllocatedLength,
				 POOL_TYPE PoolType,
				 BOOLEAN CaptureIfKernel,
				 PLUID_AND_ATTRIBUTES* Dest,
				 PULONG Length)
{
    ULONG BufferSize;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if (PrivilegeCount == 0)
    {
        *Dest = 0;
        *Length = 0;
        return STATUS_SUCCESS;
    }

    if (PreviousMode == KernelMode && !CaptureIfKernel)
    {
        *Dest = Src;
        return STATUS_SUCCESS;
    }

    /* FIXME - check PrivilegeCount for a valid number so we don't
               cause an integer overflow or exhaust system resources! */

    BufferSize = PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);
    *Length = ROUND_UP(BufferSize, 4); /* round up to a 4 byte alignment */

    /* probe the buffer */
    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForRead(Src,
                         BufferSize,
                         sizeof(ULONG));
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    /* allocate enough memory or check if the provided buffer is
       large enough to hold the array */
    if (AllocatedMem != NULL)
    {
        if (AllocatedLength < BufferSize)
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        *Dest = AllocatedMem;
    }
    else
    {
        *Dest = ExAllocatePool(PoolType,
                               BufferSize);

        if (&Dest == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    /* copy the array to the buffer */
    _SEH_TRY
    {
        RtlCopyMemory(*Dest,
                      Src,
                      BufferSize);
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if (!NT_SUCCESS(Status) && AllocatedMem == NULL)
    {
        ExFreePool(*Dest);
    }

    return Status;
}


VOID
NTAPI
SeReleaseLuidAndAttributesArray (PLUID_AND_ATTRIBUTES Privilege,
				 KPROCESSOR_MODE PreviousMode,
				 BOOLEAN CaptureIfKernel)
{
    PAGED_CODE();

    if (Privilege != NULL &&
        (PreviousMode != KernelMode || CaptureIfKernel))
    {
        ExFreePool(Privilege);
    }
}


NTSTATUS STDCALL
NtPrivilegeCheck (IN HANDLE ClientToken,
		  IN PPRIVILEGE_SET RequiredPrivileges,
		  OUT PBOOLEAN Result)
{
  PLUID_AND_ATTRIBUTES Privileges;
  PTOKEN Token;
  ULONG PrivilegeCount = 0;
  ULONG PrivilegeControl = 0;
  ULONG Length;
  BOOLEAN CheckResult;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status = STATUS_SUCCESS;

  PAGED_CODE();

  PreviousMode = KeGetPreviousMode();

  /* probe the buffers */
  if (PreviousMode != KernelMode)
  {
    _SEH_TRY
    {
      ProbeForWrite(RequiredPrivileges,
                    FIELD_OFFSET(PRIVILEGE_SET,
                                 Privilege),
                    sizeof(ULONG));

      PrivilegeCount = RequiredPrivileges->PrivilegeCount;
      PrivilegeControl = RequiredPrivileges->Control;

      /* Check PrivilegeCount to avoid an integer overflow! */
      if (FIELD_OFFSET(PRIVILEGE_SET,
                       Privilege[PrivilegeCount]) /
          sizeof(RequiredPrivileges->Privilege[0]) != PrivilegeCount)
      {
          Status = STATUS_INVALID_PARAMETER;
          _SEH_LEAVE;
      }

      /* probe all of the array */
      ProbeForWrite(RequiredPrivileges,
                    FIELD_OFFSET(PRIVILEGE_SET,
                                 Privilege[PrivilegeCount]),
                    sizeof(ULONG));

      ProbeForWriteBoolean(Result);
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if (!NT_SUCCESS(Status))
    {
      return Status;
    }
  }
  else
  {
    PrivilegeCount = RequiredPrivileges->PrivilegeCount;
    PrivilegeControl = RequiredPrivileges->Control;
  }

  /* reference the token and make sure we're
     not doing an anonymous impersonation */
  Status = ObReferenceObjectByHandle (ClientToken,
				      TOKEN_QUERY,
				      SepTokenObjectType,
				      PreviousMode,
				      (PVOID*)&Token,
				      NULL);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  if (Token->TokenType == TokenImpersonation &&
      Token->ImpersonationLevel < SecurityIdentification)
    {
      ObDereferenceObject (Token);
      return STATUS_BAD_IMPERSONATION_LEVEL;
    }

  /* capture the privileges */
  Status = SeCaptureLuidAndAttributesArray (RequiredPrivileges->Privilege,
					    PrivilegeCount,
					    PreviousMode,
					    NULL,
					    0,
					    PagedPool,
					    TRUE,
					    &Privileges,
					    &Length);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject (Token);
      return Status;
    }

  CheckResult = SepPrivilegeCheck (Token,
			           Privileges,
			           PrivilegeCount,
			           PrivilegeControl,
			           PreviousMode);

  ObDereferenceObject (Token);

  /* return the array */
  _SEH_TRY
  {
      RtlCopyMemory(RequiredPrivileges->Privilege,
                    Privileges,
                    PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));
      *Result = CheckResult;
      Status = STATUS_SUCCESS;
  }
  _SEH_HANDLE
  {
      Status = _SEH_GetExceptionCode();
  }
  _SEH_END;

  SeReleaseLuidAndAttributesArray (Privileges,
				   PreviousMode,
				   TRUE);

  return Status;
}


/*
 * @implemented
 */
BOOLEAN STDCALL
SePrivilegeCheck (PPRIVILEGE_SET Privileges,
		  PSECURITY_SUBJECT_CONTEXT SubjectContext,
		  KPROCESSOR_MODE PreviousMode)
{
  PACCESS_TOKEN Token = NULL;

  PAGED_CODE();

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


/*
 * @implemented
 */
BOOLEAN STDCALL
SeSinglePrivilegeCheck (IN LUID PrivilegeValue,
			IN KPROCESSOR_MODE PreviousMode)
{
  SECURITY_SUBJECT_CONTEXT SubjectContext;
  PRIVILEGE_SET Priv;
  BOOLEAN Result;

  PAGED_CODE();

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
