/* $Id: semgr.c,v 1.34 2004/07/18 13:02:28 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              kernel/se/semgr.c
 * PROGRAMER:         ?
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>
#include <internal/se.h>

#define NDEBUG
#include <internal/debug.h>

#define TAG_SXPT   TAG('S', 'X', 'P', 'T')


/* GLOBALS ******************************************************************/

PSE_EXPORTS EXPORTED SeExports = NULL;


/* PROTOTYPES ***************************************************************/

static BOOLEAN SepInitExports(VOID);

/* FUNCTIONS ****************************************************************/


BOOLEAN INIT_FUNCTION
SeInit1(VOID)
{
  SepInitLuid();

  if (!SepInitSecurityIDs())
    return FALSE;

  if (!SepInitDACLs())
    return FALSE;

  if (!SepInitSDs())
    return FALSE;

  SepInitPrivileges();

  if (!SepInitExports())
    return FALSE;

  return TRUE;
}


BOOLEAN INIT_FUNCTION
SeInit2(VOID)
{
  SepInitializeTokenImplementation();

  return TRUE;
}


BOOLEAN
SeInitSRM(VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING Name;
  HANDLE DirectoryHandle;
  HANDLE EventHandle;
  NTSTATUS Status;

  /* Create '\Security' directory */
  RtlInitUnicodeString(&Name,
		       L"\\Security");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_PERMANENT,
			     0,
			     NULL);
  Status = NtCreateDirectoryObject(&DirectoryHandle,
				   DIRECTORY_ALL_ACCESS,
				   &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to create 'Security' directory!\n");
      return FALSE;
    }

  /* Create 'LSA_AUTHENTICATION_INITALIZED' event */
  RtlInitUnicodeString(&Name,
		       L"\\LSA_AUTHENTICATION_INITALIZED");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_PERMANENT,
			     DirectoryHandle,
			     SePublicDefaultSd);
  Status = NtCreateEvent(&EventHandle,
			 EVENT_ALL_ACCESS,
			 &ObjectAttributes,
			 FALSE,
			 FALSE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to create 'LSA_AUTHENTICATION_INITALIZED' event!\n");
      NtClose(DirectoryHandle);
      return FALSE;
    }

  NtClose(EventHandle);
  NtClose(DirectoryHandle);

  /* FIXME: Create SRM port and listener thread */

  return TRUE;
}


static BOOLEAN INIT_FUNCTION
SepInitExports(VOID)
{
  SeExports = ExAllocatePoolWithTag(NonPagedPool,
				    sizeof(SE_EXPORTS),
				    TAG_SXPT);
  if (SeExports == NULL)
    return FALSE;

  SeExports->SeCreateTokenPrivilege = SeCreateTokenPrivilege;
  SeExports->SeAssignPrimaryTokenPrivilege = SeAssignPrimaryTokenPrivilege;
  SeExports->SeLockMemoryPrivilege = SeLockMemoryPrivilege;
  SeExports->SeIncreaseQuotaPrivilege = SeIncreaseQuotaPrivilege;
  SeExports->SeUnsolicitedInputPrivilege = SeUnsolicitedInputPrivilege;
  SeExports->SeTcbPrivilege = SeTcbPrivilege;
  SeExports->SeSecurityPrivilege = SeSecurityPrivilege;
  SeExports->SeTakeOwnershipPrivilege = SeTakeOwnershipPrivilege;
  SeExports->SeLoadDriverPrivilege = SeLoadDriverPrivilege;
  SeExports->SeCreatePagefilePrivilege = SeCreatePagefilePrivilege;
  SeExports->SeIncreaseBasePriorityPrivilege = SeIncreaseBasePriorityPrivilege;
  SeExports->SeSystemProfilePrivilege = SeSystemProfilePrivilege;
  SeExports->SeSystemtimePrivilege = SeSystemtimePrivilege;
  SeExports->SeProfileSingleProcessPrivilege = SeProfileSingleProcessPrivilege;
  SeExports->SeCreatePermanentPrivilege = SeCreatePermanentPrivilege;
  SeExports->SeBackupPrivilege = SeBackupPrivilege;
  SeExports->SeRestorePrivilege = SeRestorePrivilege;
  SeExports->SeShutdownPrivilege = SeShutdownPrivilege;
  SeExports->SeDebugPrivilege = SeDebugPrivilege;
  SeExports->SeAuditPrivilege = SeAuditPrivilege;
  SeExports->SeSystemEnvironmentPrivilege = SeSystemEnvironmentPrivilege;
  SeExports->SeChangeNotifyPrivilege = SeChangeNotifyPrivilege;
  SeExports->SeRemoteShutdownPrivilege = SeRemoteShutdownPrivilege;

  SeExports->SeNullSid = SeNullSid;
  SeExports->SeWorldSid = SeWorldSid;
  SeExports->SeLocalSid = SeLocalSid;
  SeExports->SeCreatorOwnerSid = SeCreatorOwnerSid;
  SeExports->SeCreatorGroupSid = SeCreatorGroupSid;
  SeExports->SeNtAuthoritySid = SeNtAuthoritySid;
  SeExports->SeDialupSid = SeDialupSid;
  SeExports->SeNetworkSid = SeNetworkSid;
  SeExports->SeBatchSid = SeBatchSid;
  SeExports->SeInteractiveSid = SeInteractiveSid;
  SeExports->SeLocalSystemSid = SeLocalSystemSid;
  SeExports->SeAliasAdminsSid = SeAliasAdminsSid;
  SeExports->SeAliasUsersSid = SeAliasUsersSid;
  SeExports->SeAliasGuestsSid = SeAliasGuestsSid;
  SeExports->SeAliasPowerUsersSid = SeAliasPowerUsersSid;
  SeExports->SeAliasAccountOpsSid = SeAliasAccountOpsSid;
  SeExports->SeAliasSystemOpsSid = SeAliasSystemOpsSid;
  SeExports->SeAliasPrintOpsSid = SeAliasPrintOpsSid;
  SeExports->SeAliasBackupOpsSid = SeAliasBackupOpsSid;

  return TRUE;
}


VOID SepReferenceLogonSession(PLUID AuthenticationId)
{
   UNIMPLEMENTED;
}

VOID SepDeReferenceLogonSession(PLUID AuthenticationId)
{
   UNIMPLEMENTED;
}



/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtAllocateUuids(PULARGE_INTEGER Time,
		PULONG Range,
		PULONG Sequence)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


/*
 * @implemented
 */
VOID STDCALL
SeCaptureSubjectContext(OUT PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
  PEPROCESS Process;
  BOOLEAN CopyOnOpen;
  BOOLEAN EffectiveOnly;

  Process = PsGetCurrentThread ()->ThreadsProcess;

  SubjectContext->ProcessAuditId = Process;
  SubjectContext->ClientToken = 
    PsReferenceImpersonationToken (PsGetCurrentThread(),
				   &CopyOnOpen,
				   &EffectiveOnly,
				   &SubjectContext->ImpersonationLevel);
  SubjectContext->PrimaryToken = PsReferencePrimaryToken (Process);
}


/*
 * @unimplemented
 */
VOID STDCALL
SeLockSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
  UNIMPLEMENTED;
}


/*
 * @implemented
 */
VOID STDCALL
SeReleaseSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
  ObDereferenceObject (SubjectContext->PrimaryToken);
  if (SubjectContext->ClientToken != NULL)
    {
      ObDereferenceObject (SubjectContext->ClientToken);
    }
}


/*
 * @unimplemented
 */
VOID STDCALL
SeUnlockSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
  UNIMPLEMENTED;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
SeDeassignSecurity(PSECURITY_DESCRIPTOR* SecurityDescriptor)
{
  if ((*SecurityDescriptor) != NULL)
    {
      ExFreePool(*SecurityDescriptor);
      (*SecurityDescriptor) = NULL;
    }
  return(STATUS_SUCCESS);
}


#if 0
VOID
SepGetDefaultsSubjectContext(PSECURITY_SUBJECT_CONTEXT SubjectContext,
			     PSID* Owner,
			     PSID* PrimaryGroup,
			     PSID* ProcessOwner,
			     PSID* ProcessPrimaryGroup,
			     PACL* DefaultDacl)
{
  PACCESS_TOKEN Token;

  if (SubjectContext->ClientToken != NULL)
    {
	Token = SubjectContext->ClientToken;
    }
  else
    {
	Token = SubjectContext->PrimaryToken;
    }
  *Owner = Token->UserAndGroups[Token->DefaultOwnerIndex].Sid;
  *PrimaryGroup = Token->PrimaryGroup;
  *DefaultDacl = Token->DefaultDacl;
  *ProcessOwner = SubjectContext->PrimaryToken->
    UserAndGroups[Token->DefaultOwnerIndex].Sid;
  *ProcessPrimaryGroup = SubjectContext->PrimaryToken->PrimaryGroup;
}


NTSTATUS
SepInheritAcl(PACL Acl,
	      BOOLEAN IsDirectoryObject,
	      PSID Owner,
	      PSID PrimaryGroup,
	      PACL DefaultAcl,
	      PSID ProcessOwner,
	      PSID ProcessGroup,
	      PGENERIC_MAPPING GenericMapping)
{
  if (Acl == NULL)
    {
	return(STATUS_UNSUCCESSFUL);
    }

  if (Acl->AclRevision != 2 &&
      Acl->AclRevision != 3 )
    {
	return(STATUS_UNSUCCESSFUL);
    }

}
#endif


/*
 * @unimplemented
 */
NTSTATUS STDCALL
SeAssignSecurity(PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
		 PSECURITY_DESCRIPTOR ExplicitDescriptor OPTIONAL,
		 PSECURITY_DESCRIPTOR *NewDescriptor,
		 BOOLEAN IsDirectoryObject,
		 PSECURITY_SUBJECT_CONTEXT SubjectContext,
		 PGENERIC_MAPPING GenericMapping,
		 POOL_TYPE PoolType)
{
  PSECURITY_DESCRIPTOR Descriptor;
  ULONG Length;
  NTSTATUS Status;

  if (ExplicitDescriptor != NULL)
    {
      Length = RtlLengthSecurityDescriptor(ExplicitDescriptor);
    }
  else
    {
      DPRINT("No explicit security descriptor\n");
      return STATUS_UNSUCCESSFUL;
    }

  Descriptor = ExAllocatePool(NonPagedPool,
			      Length);
  if (Descriptor == NULL)
    {
      DPRINT1("ExAlloctePool() failed\n");
      return STATUS_UNSUCCESSFUL;
    }

  Status = RtlMakeSelfRelativeSD(ExplicitDescriptor,
				 Descriptor,
				 &Length);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("RtlMakeSelfRelativeSD() failed (Status %lx)\n", Status);
      return Status;
    }

  *NewDescriptor = Descriptor;

  return STATUS_SUCCESS;

#if 0
   PSID Owner;
   PSID PrimaryGroup;
   PACL DefaultDacl;
   PSID ProcessOwner;
   PSID ProcessPrimaryGroup;
   PACL Sacl;

   if (ExplicitDescriptor == NULL)
     {
	RtlCreateSecurityDescriptor(&Descriptor, 1);
     }
   else
     {
	Descriptor = ExplicitDescriptor;
     }

   SeLockSubjectContext(SubjectContext);

   SepGetDefaultsSubjectContext(SubjectContext,
				&Owner,
				&PrimaryGroup,
				&DefaultDacl,
				&ProcessOwner,
				&ProcessPrimaryGroup);

   if (Descriptor->Control & SE_SACL_PRESENT ||
       Descriptor->Control & SE_SACL_DEFAULTED)
     {
	if (ParentDescriptor == NULL)
	  {
	  }

	if (Descriptor->Control & SE_SACL_PRESENT ||
	    Descriptor->Sacl == NULL ||)
	  {
	     Sacl = NULL;
	  }
	else
	  {
	     Sacl = Descriptor->Sacl;
	     if (Descriptor->Control & SE_SELF_RELATIVE)
	       {
		  Sacl = (PACL)(((ULONG_PTR)Sacl) + (ULONG_PTR)Descriptor);
	       }
	  }

	SepInheritAcl(Sacl,
		      IsDirectoryObject,
		      Owner,
		      PrimaryGroup,
		      DefaultDacl,
		      ProcessOwner,
		      GenericMapping);
     }
#endif
}


static BOOLEAN
SepSidInToken(PACCESS_TOKEN Token,
	      PSID Sid)
{
  ULONG i;

  if (Token->UserAndGroupCount == 0)
  {
    return FALSE;
  }

  for (i=0; i<Token->UserAndGroupCount; i++)
  {
    if (RtlEqualSid(Sid, Token->UserAndGroups[i].Sid))
    {
      if (Token->UserAndGroups[i].Attributes & SE_GROUP_ENABLED)
      {
        return TRUE;
      }

      return FALSE;
    }
  }

  return FALSE;
}


/*
 * FUNCTION: Determines whether the requested access rights can be granted
 * to an object protected by a security descriptor and an object owner
 * ARGUMENTS:
 *      SecurityDescriptor = Security descriptor protecting the object
 *      SubjectSecurityContext = Subject's captured security context
 *      SubjectContextLocked = Indicates the user's subject context is locked
 *      DesiredAccess = Access rights the caller is trying to acquire
 *      PreviouslyGrantedAccess = Specified the access rights already granted
 *      Privileges = ?
 *      GenericMapping = Generic mapping associated with the object
 *      AccessMode = Access mode used for the check
 *      GrantedAccess (OUT) = On return specifies the access granted
 *      AccessStatus (OUT) = Status indicating why access was denied
 * RETURNS: If access was granted, returns TRUE
 *
 * @implemented
 */
BOOLEAN STDCALL
SeAccessCheck(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	      IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
	      IN BOOLEAN SubjectContextLocked,
	      IN ACCESS_MASK DesiredAccess,
	      IN ACCESS_MASK PreviouslyGrantedAccess,
	      OUT PPRIVILEGE_SET* Privileges,
	      IN PGENERIC_MAPPING GenericMapping,
	      IN KPROCESSOR_MODE AccessMode,
	      OUT PACCESS_MASK GrantedAccess,
	      OUT PNTSTATUS AccessStatus)
{
  LUID_AND_ATTRIBUTES Privilege;
  ACCESS_MASK CurrentAccess;
  PACCESS_TOKEN Token;
  ULONG i;
  PACL Dacl;
  BOOLEAN Present;
  BOOLEAN Defaulted;
  PACE CurrentAce;
  PSID Sid;
  NTSTATUS Status;

  CurrentAccess = PreviouslyGrantedAccess;

  Token = SubjectSecurityContext->ClientToken ?
	    SubjectSecurityContext->ClientToken : SubjectSecurityContext->PrimaryToken;

  /* Get the DACL */
  Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
					&Present,
					&Dacl,
					&Defaulted);
  if (!NT_SUCCESS(Status))
    {
      *AccessStatus = Status;
      return FALSE;
    }

  /* RULE 1: Grant desired access if the object is unprotected */
  if (Dacl == NULL)
    {
      *GrantedAccess = DesiredAccess;
      *AccessStatus = STATUS_SUCCESS;
      return TRUE;
    }

  CurrentAccess = PreviouslyGrantedAccess;

  /* RULE 2: Check token for 'take ownership' privilege */
  Privilege.Luid = SeTakeOwnershipPrivilege;
  Privilege.Attributes = SE_PRIVILEGE_ENABLED;

  if (SepPrivilegeCheck(Token,
			&Privilege,
			1,
			PRIVILEGE_SET_ALL_NECESSARY,
			AccessMode))
    {
      CurrentAccess |= WRITE_OWNER;
      if (DesiredAccess == CurrentAccess)
	{
	  *GrantedAccess = CurrentAccess;
	  *AccessStatus = STATUS_SUCCESS;
	  return TRUE;
	}
    }

  /* RULE 3: Check whether the token is the owner */
  Status = RtlGetOwnerSecurityDescriptor(SecurityDescriptor,
					 &Sid,
					 &Defaulted);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("RtlGetOwnerSecurityDescriptor() failed (Status %lx)\n", Status);
      *AccessStatus = Status;
      return FALSE;
   }

  if (SepSidInToken(Token, Sid))
    {
      CurrentAccess |= (READ_CONTROL | WRITE_DAC);
      if (DesiredAccess == CurrentAccess)
	{
	  *GrantedAccess = CurrentAccess;
	  *AccessStatus = STATUS_SUCCESS;
	  return TRUE;
	}
    }

  /* RULE 4: Grant rights according to the DACL */
  CurrentAce = (PACE)(Dacl + 1);
  for (i = 0; i < Dacl->AceCount; i++)
    {
      Sid = (PSID)(CurrentAce + 1);
      if (CurrentAce->Header.AceType == ACCESS_DENIED_ACE_TYPE)
	{
	  if (SepSidInToken(Token, Sid))
	    {
	      *GrantedAccess = 0;
	      *AccessStatus = STATUS_ACCESS_DENIED;
	      return TRUE;
	    }
	}

      if (CurrentAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
	{
	  if (SepSidInToken(Token, Sid))
	    {
	      CurrentAccess |= CurrentAce->AccessMask;
	    }
	}
    }

  DPRINT("CurrentAccess %08lx\n DesiredAccess %08lx\n",
         CurrentAccess, DesiredAccess);

  *GrantedAccess = CurrentAccess & DesiredAccess;

  *AccessStatus =
    (*GrantedAccess == DesiredAccess) ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;

  return TRUE;
}


NTSTATUS STDCALL
NtAccessCheck(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	      IN HANDLE TokenHandle,
	      IN ACCESS_MASK DesiredAccess,
	      IN PGENERIC_MAPPING GenericMapping,
	      OUT PPRIVILEGE_SET PrivilegeSet,
	      OUT PULONG ReturnLength,
	      OUT PACCESS_MASK GrantedAccess,
	      OUT PNTSTATUS AccessStatus)
{
  SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
  KPROCESSOR_MODE PreviousMode;
  PACCESS_TOKEN Token;
  NTSTATUS Status;

  DPRINT("NtAccessCheck() called\n");

  PreviousMode = KeGetPreviousMode();
  if (PreviousMode == KernelMode)
    {
      *GrantedAccess = DesiredAccess;
      *AccessStatus = STATUS_SUCCESS;
      return STATUS_SUCCESS;
    }

  Status = ObReferenceObjectByHandle(TokenHandle,
				     TOKEN_QUERY,
				     SepTokenObjectType,
				     PreviousMode,
				     (PVOID*)&Token,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to reference token (Status %lx)\n", Status);
      return Status;
    }

  /* Check token type */
  if (Token->TokenType != TokenImpersonation)
    {
      DPRINT1("No impersonation token\n");
      ObDereferenceObject(Token);
      return STATUS_ACCESS_VIOLATION;
    }

  /* Check impersonation level */
  if (Token->ImpersonationLevel < SecurityAnonymous)
    {
      DPRINT1("Invalid impersonation level\n");
      ObDereferenceObject(Token);
      return STATUS_ACCESS_VIOLATION;
    }

  RtlZeroMemory(&SubjectSecurityContext,
		sizeof(SECURITY_SUBJECT_CONTEXT));
  SubjectSecurityContext.ClientToken = Token;
  SubjectSecurityContext.ImpersonationLevel = Token->ImpersonationLevel;

  /* FIXME: Lock subject context */

  if (!SeAccessCheck(SecurityDescriptor,
		     &SubjectSecurityContext,
		     TRUE,
		     DesiredAccess,
		     0,
		     &PrivilegeSet,
		     GenericMapping,
		     PreviousMode,
		     GrantedAccess,
		     AccessStatus))
    {
      Status = *AccessStatus;
    }
  else
    {
      Status = STATUS_SUCCESS;
    }

  /* FIXME: Unlock subject context */

  ObDereferenceObject(Token);

  DPRINT("NtAccessCheck() done\n");

  return Status;
}

/* EOF */
