/* $Id: semgr.c,v 1.51 2004/11/21 18:35:05 gdalsnes Exp $
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

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#define TAG_SXPT   TAG('S', 'X', 'P', 'T')


/* GLOBALS ******************************************************************/

PSE_EXPORTS EXPORTED SeExports = NULL;

static ERESOURCE SepSubjectContextLock;


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

  /* Initialize the subject context lock */
  ExInitializeResource(&SepSubjectContextLock);

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
			 SynchronizationEvent,
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
 * @implemented
 */
VOID STDCALL
SeCaptureSubjectContext(OUT PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
  PETHREAD Thread;
  BOOLEAN CopyOnOpen;
  BOOLEAN EffectiveOnly;

  Thread = PsGetCurrentThread();
  if (Thread == NULL)
    {
      SubjectContext->ProcessAuditId = 0;
      SubjectContext->PrimaryToken = NULL;
      SubjectContext->ClientToken = NULL;
      SubjectContext->ImpersonationLevel = 0;
    }
  else
    {
      SubjectContext->ProcessAuditId = Thread->ThreadsProcess;
      SubjectContext->ClientToken = 
	PsReferenceImpersonationToken(Thread,
				      &CopyOnOpen,
				      &EffectiveOnly,
				      &SubjectContext->ImpersonationLevel);
      SubjectContext->PrimaryToken = PsReferencePrimaryToken(Thread->ThreadsProcess);
    }
}


/*
 * @implemented
 */
VOID STDCALL
SeLockSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&SepSubjectContextLock, TRUE);
}


/*
 * @implemented
 */
VOID STDCALL
SeUnlockSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
  ExReleaseResourceLite(&SepSubjectContextLock);
  KeLeaveCriticalRegion();
}


/*
 * @implemented
 */
VOID STDCALL
SeReleaseSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
  if (SubjectContext->PrimaryToken != NULL)
    {
      ObDereferenceObject(SubjectContext->PrimaryToken);
    }

  if (SubjectContext->ClientToken != NULL)
    {
      ObDereferenceObject(SubjectContext->ClientToken);
    }
}


/*
 * @implemented
 */
NTSTATUS STDCALL
SeDeassignSecurity(PSECURITY_DESCRIPTOR *SecurityDescriptor)
{
  if (*SecurityDescriptor != NULL)
    {
      ExFreePool(*SecurityDescriptor);
      *SecurityDescriptor = NULL;
    }

  return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
SeAssignSecurityEx(IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
		   IN PSECURITY_DESCRIPTOR ExplicitDescriptor OPTIONAL,
		   OUT PSECURITY_DESCRIPTOR *NewDescriptor,
		   IN GUID *ObjectType OPTIONAL,
		   IN BOOLEAN IsDirectoryObject,
		   IN ULONG AutoInheritFlags,
		   IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
		   IN PGENERIC_MAPPING GenericMapping,
		   IN POOL_TYPE PoolType)
{
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * FUNCTION: Creates a security descriptor for a new object.
 * ARGUMENTS:
 *         ParentDescriptor = 
 *         ExplicitDescriptor = 
 *         NewDescriptor = 
 *         IsDirectoryObject = 
 *         SubjectContext = 
 *         GeneralMapping = 
 *         PoolType = 
 * RETURNS: Status
 *
 * @implemented
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
  PACCESS_TOKEN Token;
  ULONG OwnerLength = 0;
  ULONG GroupLength = 0;
  ULONG DaclLength = 0;
  ULONG SaclLength = 0;
  ULONG Length = 0;
  ULONG Control = 0;
  ULONG_PTR Current;
  PSID Owner = NULL;
  PSID Group = NULL;
  PACL Dacl = NULL;
  PACL Sacl = NULL;

  /* Lock subject context */
  SeLockSubjectContext(SubjectContext);

  if (SubjectContext->ClientToken != NULL)
    {
      Token = SubjectContext->ClientToken;
    }
  else
    {
      Token = SubjectContext->PrimaryToken;
    }


  /* Inherit the Owner SID */
  if (ExplicitDescriptor != NULL && ExplicitDescriptor->Owner != NULL)
    {
      DPRINT("Use explicit owner sid!\n");
      Owner = ExplicitDescriptor->Owner;
      if (ExplicitDescriptor->Control & SE_SELF_RELATIVE)
	{
	  Owner = (PSID)(((ULONG_PTR)Owner) + (ULONG_PTR)ExplicitDescriptor);
	}
    }
  else
    {
      if (Token != NULL)
	{
	  DPRINT("Use token owner sid!\n");
	  Owner = Token->UserAndGroups[Token->DefaultOwnerIndex].Sid;
	}
      else
	{
	  DPRINT("Use default owner sid!\n");
	  Owner = SeLocalSystemSid;
	}

      Control |= SE_OWNER_DEFAULTED;
    }

  OwnerLength = ROUND_UP(RtlLengthSid(Owner), 4);


  /* Inherit the Group SID */
  if (ExplicitDescriptor != NULL && ExplicitDescriptor->Group != NULL)
    {
      DPRINT("Use explicit group sid!\n");
      Group = ExplicitDescriptor->Group;
      if (ExplicitDescriptor->Control & SE_SELF_RELATIVE)
	{
	  Group = (PSID)(((ULONG_PTR)Group) + (ULONG_PTR)ExplicitDescriptor);
	}
    }
  else
    {
      if (Token != NULL)
	{
	  DPRINT("Use token group sid!\n");
	  Group = Token->PrimaryGroup;
	}
      else
	{
	  DPRINT("Use default group sid!\n");
	  Group = SeLocalSystemSid;
	}

      Control |= SE_OWNER_DEFAULTED;
    }

  GroupLength = ROUND_UP(RtlLengthSid(Group), 4);


  /* Inherit the DACL */
  if (ExplicitDescriptor != NULL &&
      (ExplicitDescriptor->Control & SE_DACL_PRESENT) &&
      !(ExplicitDescriptor->Control & SE_DACL_DEFAULTED))
    {
      DPRINT("Use explicit DACL!\n");
      Dacl = ExplicitDescriptor->Dacl;
      if (Dacl != NULL && (ExplicitDescriptor->Control & SE_SELF_RELATIVE))
	{
	  Dacl = (PACL)(((ULONG_PTR)Dacl) + (ULONG_PTR)ExplicitDescriptor);
	}

      Control |= SE_DACL_PRESENT;
    }
  else if (ParentDescriptor != NULL &&
	   (ParentDescriptor->Control & SE_DACL_PRESENT))
    {
      DPRINT("Use parent DACL!\n");
      /* FIXME: Inherit */
      Dacl = ParentDescriptor->Dacl;
      if (Dacl != NULL && (ParentDescriptor->Control & SE_SELF_RELATIVE))
	{
	  Dacl = (PACL)(((ULONG_PTR)Dacl) + (ULONG_PTR)ParentDescriptor);
	}
      Control |= (SE_DACL_PRESENT | SE_DACL_DEFAULTED);
    }
  else if (Token != NULL && Token->DefaultDacl != NULL)
    {
      DPRINT("Use token default DACL!\n");
      /* FIXME: Inherit */
      Dacl = Token->DefaultDacl;
      Control |= (SE_DACL_PRESENT | SE_DACL_DEFAULTED);
    }
  else
    {
      DPRINT("Use NULL DACL!\n");
      Dacl = NULL;
      Control |= (SE_DACL_PRESENT | SE_DACL_DEFAULTED);
    }

  DaclLength = (Dacl != NULL) ? ROUND_UP(Dacl->AclSize, 4) : 0;


  /* Inherit the SACL */
  if (ExplicitDescriptor != NULL &&
      (ExplicitDescriptor->Control & SE_SACL_PRESENT) &&
      !(ExplicitDescriptor->Control & SE_SACL_DEFAULTED))
    {
      DPRINT("Use explicit SACL!\n");
      Sacl = ExplicitDescriptor->Sacl;
      if (Sacl != NULL && (ExplicitDescriptor->Control & SE_SELF_RELATIVE))
	{
	  Sacl = (PACL)(((ULONG_PTR)Sacl) + (ULONG_PTR)ExplicitDescriptor);
	}

      Control |= SE_SACL_PRESENT;
    }
  else if (ParentDescriptor != NULL &&
	   (ParentDescriptor->Control & SE_SACL_PRESENT))
    {
      DPRINT("Use parent SACL!\n");
      /* FIXME: Inherit */
      Sacl = ParentDescriptor->Sacl;
      if (Sacl != NULL && (ParentDescriptor->Control & SE_SELF_RELATIVE))
	{
	  Sacl = (PACL)(((ULONG_PTR)Sacl) + (ULONG_PTR)ParentDescriptor);
	}
      Control |= (SE_SACL_PRESENT | SE_SACL_DEFAULTED);
    }

  SaclLength = (Sacl != NULL) ? ROUND_UP(Sacl->AclSize, 4) : 0;


  /* Allocate and initialize the new security descriptor */
  Length = sizeof(SECURITY_DESCRIPTOR) + 
      OwnerLength + GroupLength + DaclLength + SaclLength;

  DPRINT("L: sizeof(SECURITY_DESCRIPTOR) %d OwnerLength %d GroupLength %d DaclLength %d SaclLength %d\n", 
	 sizeof(SECURITY_DESCRIPTOR),
	 OwnerLength,
	 GroupLength,
	 DaclLength,
	 SaclLength);

  Descriptor = ExAllocatePool(NonPagedPool,
			      Length);
  RtlZeroMemory( Descriptor, Length );

  if (Descriptor == NULL)
    {
      DPRINT1("ExAlloctePool() failed\n");
      /* FIXME: Unlock subject context */
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  RtlCreateSecurityDescriptor(Descriptor,
			      SECURITY_DESCRIPTOR_REVISION);

  Descriptor->Control = Control | SE_SELF_RELATIVE;

  Current = (ULONG_PTR)Descriptor + sizeof(SECURITY_DESCRIPTOR);

  if (SaclLength != 0)
    {
      RtlCopyMemory((PVOID)Current,
		    Sacl,
		    SaclLength);
      Descriptor->Sacl = (PACL)((ULONG_PTR)Current - (ULONG_PTR)Descriptor);
      Current += SaclLength;
    }

  if (DaclLength != 0)
    {
      RtlCopyMemory((PVOID)Current,
		    Dacl,
		    DaclLength);
      Descriptor->Dacl = (PACL)((ULONG_PTR)Current - (ULONG_PTR)Descriptor);
      Current += DaclLength;
    }

  if (OwnerLength != 0)
    {
      RtlCopyMemory((PVOID)Current,
		    Owner,
		    OwnerLength);
      Descriptor->Owner = (PSID)((ULONG_PTR)Current - (ULONG_PTR)Descriptor);
      Current += OwnerLength;
      DPRINT("Owner of %x at %x\n", Descriptor, Descriptor->Owner);
    }
  else
      DPRINT("Owner of %x is zero length\n", Descriptor);

  if (GroupLength != 0)
    {
      memmove((PVOID)Current,
              Group,
              GroupLength);
      Descriptor->Group = (PSID)((ULONG_PTR)Current - (ULONG_PTR)Descriptor);
    }

  /* Unlock subject context */
  SeUnlockSubjectContext(SubjectContext);

  *NewDescriptor = Descriptor;

  DPRINT("Descrptor %x\n", Descriptor);
  ASSERT(RtlLengthSecurityDescriptor(Descriptor));

  return STATUS_SUCCESS;
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

  if (SubjectContextLocked == FALSE)
    {
      SeLockSubjectContext(SubjectSecurityContext);
    }

  Token = SubjectSecurityContext->ClientToken ?
	    SubjectSecurityContext->ClientToken : SubjectSecurityContext->PrimaryToken;

  /* Get the DACL */
  Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
					&Present,
					&Dacl,
					&Defaulted);
  if (!NT_SUCCESS(Status))
    {
      if (SubjectContextLocked == FALSE)
	{
	  SeUnlockSubjectContext(SubjectSecurityContext);
	}

      *AccessStatus = Status;
      return FALSE;
    }

  /* RULE 1: Grant desired access if the object is unprotected */
  if (Present == TRUE && Dacl == NULL)
    {
      if (SubjectContextLocked == FALSE)
	{
	  SeUnlockSubjectContext(SubjectSecurityContext);
	}

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
	  if (SubjectContextLocked == FALSE)
	    {
	      SeUnlockSubjectContext(SubjectSecurityContext);
	    }

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
      if (SubjectContextLocked == FALSE)
	{
	  SeUnlockSubjectContext(SubjectSecurityContext);
	}

      *AccessStatus = Status;
      return FALSE;
   }

  if (SepSidInToken(Token, Sid))
    {
      CurrentAccess |= (READ_CONTROL | WRITE_DAC);
      if (DesiredAccess == CurrentAccess)
	{
	  if (SubjectContextLocked == FALSE)
	    {
	      SeUnlockSubjectContext(SubjectSecurityContext);
	    }

	  *GrantedAccess = CurrentAccess;
	  *AccessStatus = STATUS_SUCCESS;
	  return TRUE;
	}
    }

  /* Fail if DACL is absent */
  if (Present == FALSE)
    {
      if (SubjectContextLocked == FALSE)
	{
	  SeUnlockSubjectContext(SubjectSecurityContext);
	}

      *GrantedAccess = 0;
      *AccessStatus = STATUS_ACCESS_DENIED;
      return TRUE;
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
	      if (SubjectContextLocked == FALSE)
		{
		  SeUnlockSubjectContext(SubjectSecurityContext);
		}

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

  if (SubjectContextLocked == FALSE)
    {
      SeUnlockSubjectContext(SubjectSecurityContext);
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

  /* Lock subject context */
  SeLockSubjectContext(&SubjectSecurityContext);

  if (SeAccessCheck(SecurityDescriptor,
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
      Status = STATUS_ACCESS_DENIED;
    }

  /* Unlock subject context */
  SeUnlockSubjectContext(&SubjectSecurityContext);

  ObDereferenceObject(Token);

  DPRINT("NtAccessCheck() done\n");

  return Status;
}

/* EOF */
