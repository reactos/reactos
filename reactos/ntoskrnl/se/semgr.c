/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/semgr.c
 * PURPOSE:         Security manager
 *
 * PROGRAMMERS:     No programmer listed.
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

PSE_EXPORTS SeExports = NULL;
SE_EXPORTS SepExports;

static ERESOURCE SepSubjectContextLock;
extern ULONG ExpInitializationPhase;


/* PROTOTYPES ***************************************************************/

static BOOLEAN SepInitExports(VOID);

/* FUNCTIONS ****************************************************************/

BOOLEAN
NTAPI
SepInitializationPhase0(VOID)
{
    SepInitLuid();
    if (!SepInitSecurityIDs()) return FALSE;
    if (!SepInitDACLs()) return FALSE;
    if (!SepInitSDs()) return FALSE;
    SepInitPrivileges();
    if (!SepInitExports()) return FALSE;

    /* Initialize the subject context lock */
    ExInitializeResource(&SepSubjectContextLock);

    /* Initialize token objects */
    SepInitializeTokenImplementation();

    /* Clear impersonation info for the idle thread */
    PsGetCurrentThread()->ImpersonationInfo = NULL;
    PspClearCrossThreadFlag(PsGetCurrentThread(),
                            CT_ACTIVE_IMPERSONATION_INFO_BIT);

    /* Initialize the boot token */
    ObInitializeFastReference(&PsGetCurrentProcess()->Token, NULL);
    ObInitializeFastReference(&PsGetCurrentProcess()->Token,
                              SepCreateSystemProcessToken());
    return TRUE;
}

BOOLEAN
NTAPI
SepInitializationPhase1(VOID)
{
    NTSTATUS Status;
    PAGED_CODE();

    /* Insert the system token into the tree */
    Status = ObInsertObject((PVOID)(PsGetCurrentProcess()->Token.Value &
                                    ~MAX_FAST_REFS),
                            NULL,
                            0,
                            0,
                            NULL,
                            NULL);
    ASSERT(NT_SUCCESS(Status));

    /* FIXME: TODO \\ Security directory */
    return TRUE;
}

BOOLEAN
NTAPI
SeInit(VOID)
{
    /* Check the initialization phase */
    switch (ExpInitializationPhase)
    {
    case 0:

        /* Do Phase 0 */
        return SepInitializationPhase0();

    case 1:

        /* Do Phase 1 */
        return SepInitializationPhase1();

    default:

        /* Don't know any other phase! Bugcheck! */
        KeBugCheckEx(UNEXPECTED_INITIALIZATION_CALL,
                     0,
                     ExpInitializationPhase,
                     0,
                     0);
        return FALSE;
    }
}

BOOLEAN
NTAPI
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
  Status = ZwCreateDirectoryObject(&DirectoryHandle,
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
  Status = ZwCreateEvent(&EventHandle,
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

  ZwClose(EventHandle);
  ZwClose(DirectoryHandle);

  /* FIXME: Create SRM port and listener thread */

  return TRUE;
}


static BOOLEAN INIT_FUNCTION
SepInitExports(VOID)
{
  SepExports.SeCreateTokenPrivilege = SeCreateTokenPrivilege;
  SepExports.SeAssignPrimaryTokenPrivilege = SeAssignPrimaryTokenPrivilege;
  SepExports.SeLockMemoryPrivilege = SeLockMemoryPrivilege;
  SepExports.SeIncreaseQuotaPrivilege = SeIncreaseQuotaPrivilege;
  SepExports.SeUnsolicitedInputPrivilege = SeUnsolicitedInputPrivilege;
  SepExports.SeTcbPrivilege = SeTcbPrivilege;
  SepExports.SeSecurityPrivilege = SeSecurityPrivilege;
  SepExports.SeTakeOwnershipPrivilege = SeTakeOwnershipPrivilege;
  SepExports.SeLoadDriverPrivilege = SeLoadDriverPrivilege;
  SepExports.SeCreatePagefilePrivilege = SeCreatePagefilePrivilege;
  SepExports.SeIncreaseBasePriorityPrivilege = SeIncreaseBasePriorityPrivilege;
  SepExports.SeSystemProfilePrivilege = SeSystemProfilePrivilege;
  SepExports.SeSystemtimePrivilege = SeSystemtimePrivilege;
  SepExports.SeProfileSingleProcessPrivilege = SeProfileSingleProcessPrivilege;
  SepExports.SeCreatePermanentPrivilege = SeCreatePermanentPrivilege;
  SepExports.SeBackupPrivilege = SeBackupPrivilege;
  SepExports.SeRestorePrivilege = SeRestorePrivilege;
  SepExports.SeShutdownPrivilege = SeShutdownPrivilege;
  SepExports.SeDebugPrivilege = SeDebugPrivilege;
  SepExports.SeAuditPrivilege = SeAuditPrivilege;
  SepExports.SeSystemEnvironmentPrivilege = SeSystemEnvironmentPrivilege;
  SepExports.SeChangeNotifyPrivilege = SeChangeNotifyPrivilege;
  SepExports.SeRemoteShutdownPrivilege = SeRemoteShutdownPrivilege;

  SepExports.SeNullSid = SeNullSid;
  SepExports.SeWorldSid = SeWorldSid;
  SepExports.SeLocalSid = SeLocalSid;
  SepExports.SeCreatorOwnerSid = SeCreatorOwnerSid;
  SepExports.SeCreatorGroupSid = SeCreatorGroupSid;
  SepExports.SeNtAuthoritySid = SeNtAuthoritySid;
  SepExports.SeDialupSid = SeDialupSid;
  SepExports.SeNetworkSid = SeNetworkSid;
  SepExports.SeBatchSid = SeBatchSid;
  SepExports.SeInteractiveSid = SeInteractiveSid;
  SepExports.SeLocalSystemSid = SeLocalSystemSid;
  SepExports.SeAliasAdminsSid = SeAliasAdminsSid;
  SepExports.SeAliasUsersSid = SeAliasUsersSid;
  SepExports.SeAliasGuestsSid = SeAliasGuestsSid;
  SepExports.SeAliasPowerUsersSid = SeAliasPowerUsersSid;
  SepExports.SeAliasAccountOpsSid = SeAliasAccountOpsSid;
  SepExports.SeAliasSystemOpsSid = SeAliasSystemOpsSid;
  SepExports.SeAliasPrintOpsSid = SeAliasPrintOpsSid;
  SepExports.SeAliasBackupOpsSid = SeAliasBackupOpsSid;
  SepExports.SeAuthenticatedUsersSid = SeAuthenticatedUsersSid;
  SepExports.SeRestrictedSid = SeRestrictedSid;
  SepExports.SeAnonymousLogonSid = SeAnonymousLogonSid;

  SepExports.SeUndockPrivilege = SeUndockPrivilege;
  SepExports.SeSyncAgentPrivilege = SeSyncAgentPrivilege;
  SepExports.SeEnableDelegationPrivilege = SeEnableDelegationPrivilege;
  
  SeExports = &SepExports;
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

NTSTATUS
STDCALL
SeDefaultObjectMethod(PVOID Object,
                      SECURITY_OPERATION_CODE OperationType,
                      PSECURITY_INFORMATION _SecurityInformation,
                      PSECURITY_DESCRIPTOR _SecurityDescriptor,
                      PULONG ReturnLength,
                      PSECURITY_DESCRIPTOR *OldSecurityDescriptor,
                      POOL_TYPE PoolType,
                      PGENERIC_MAPPING GenericMapping)
{
  PISECURITY_DESCRIPTOR ObjectSd;
  PISECURITY_DESCRIPTOR NewSd;
  PISECURITY_DESCRIPTOR SecurityDescriptor = _SecurityDescriptor;
  POBJECT_HEADER Header = OBJECT_TO_OBJECT_HEADER(Object);
  PSID Owner = 0;
  PSID Group = 0;
  PACL Dacl = 0;
  PACL Sacl = 0;
  ULONG OwnerLength = 0;
  ULONG GroupLength = 0;
  ULONG DaclLength = 0;
  ULONG SaclLength = 0;
  ULONG Control = 0;
  ULONG_PTR Current;
  NTSTATUS Status;
  SECURITY_INFORMATION SecurityInformation;

    if (OperationType == SetSecurityDescriptor)
    {
        ObjectSd = Header->SecurityDescriptor;
        SecurityInformation = *_SecurityInformation;

      /* Get owner and owner size */
      if (SecurityInformation & OWNER_SECURITY_INFORMATION)
        {
          if (SecurityDescriptor->Owner != NULL)
            {
                if( SecurityDescriptor->Control & SE_SELF_RELATIVE )
                    Owner = (PSID)((ULONG_PTR)SecurityDescriptor->Owner +
                                   (ULONG_PTR)SecurityDescriptor);
                else
                    Owner = (PSID)SecurityDescriptor->Owner;
                OwnerLength = ROUND_UP(RtlLengthSid(Owner), 4);
            }
          Control |= (SecurityDescriptor->Control & SE_OWNER_DEFAULTED);
        }
      else
        {
          if (ObjectSd->Owner != NULL)
          {
              Owner = (PSID)((ULONG_PTR)ObjectSd->Owner + (ULONG_PTR)ObjectSd);
              OwnerLength = ROUND_UP(RtlLengthSid(Owner), 4);
          }
          Control |= (ObjectSd->Control & SE_OWNER_DEFAULTED);
        }

      /* Get group and group size */
      if (SecurityInformation & GROUP_SECURITY_INFORMATION)
        {
          if (SecurityDescriptor->Group != NULL)
            {
                if( SecurityDescriptor->Control & SE_SELF_RELATIVE )
                    Group = (PSID)((ULONG_PTR)SecurityDescriptor->Group +
                                   (ULONG_PTR)SecurityDescriptor);
                else
                    Group = (PSID)SecurityDescriptor->Group;
                GroupLength = ROUND_UP(RtlLengthSid(Group), 4);
            }
          Control |= (SecurityDescriptor->Control & SE_GROUP_DEFAULTED);
        }
      else
        {
          if (ObjectSd->Group != NULL)
            {
              Group = (PSID)((ULONG_PTR)ObjectSd->Group + (ULONG_PTR)ObjectSd);
              GroupLength = ROUND_UP(RtlLengthSid(Group), 4);
            }
          Control |= (ObjectSd->Control & SE_GROUP_DEFAULTED);
        }

      /* Get DACL and DACL size */
      if (SecurityInformation & DACL_SECURITY_INFORMATION)
        {
          if ((SecurityDescriptor->Control & SE_DACL_PRESENT) &&
              (SecurityDescriptor->Dacl != NULL))
            {
                if( SecurityDescriptor->Control & SE_SELF_RELATIVE )
                    Dacl = (PACL)((ULONG_PTR)SecurityDescriptor->Dacl +
                                  (ULONG_PTR)SecurityDescriptor);
                else
                    Dacl = (PACL)SecurityDescriptor->Dacl;

              DaclLength = ROUND_UP((ULONG)Dacl->AclSize, 4);
            }
          Control |= (SecurityDescriptor->Control & (SE_DACL_DEFAULTED | SE_DACL_PRESENT));
        }
      else
        {
          if ((ObjectSd->Control & SE_DACL_PRESENT) &&
              (ObjectSd->Dacl != NULL))
            {
              Dacl = (PACL)((ULONG_PTR)ObjectSd->Dacl + (ULONG_PTR)ObjectSd);
              DaclLength = ROUND_UP((ULONG)Dacl->AclSize, 4);
            }
          Control |= (ObjectSd->Control & (SE_DACL_DEFAULTED | SE_DACL_PRESENT));
        }

      /* Get SACL and SACL size */
      if (SecurityInformation & SACL_SECURITY_INFORMATION)
        {
          if ((SecurityDescriptor->Control & SE_SACL_PRESENT) &&
              (SecurityDescriptor->Sacl != NULL))
            {
                if( SecurityDescriptor->Control & SE_SELF_RELATIVE )
                    Sacl = (PACL)((ULONG_PTR)SecurityDescriptor->Sacl +
                                  (ULONG_PTR)SecurityDescriptor);
                else
                    Sacl = (PACL)SecurityDescriptor->Sacl;
                SaclLength = ROUND_UP((ULONG)Sacl->AclSize, 4);
            }
          Control |= (SecurityDescriptor->Control & (SE_SACL_DEFAULTED | SE_SACL_PRESENT));
        }
      else
        {
          if ((ObjectSd->Control & SE_SACL_PRESENT) &&
              (ObjectSd->Sacl != NULL))
            {
              Sacl = (PACL)((ULONG_PTR)ObjectSd->Sacl + (ULONG_PTR)ObjectSd);
              SaclLength = ROUND_UP((ULONG)Sacl->AclSize, 4);
            }
          Control |= (ObjectSd->Control & (SE_SACL_DEFAULTED | SE_SACL_PRESENT));
        }

      NewSd = ExAllocatePool(NonPagedPool,
                             sizeof(SECURITY_DESCRIPTOR) + OwnerLength + GroupLength +
                             DaclLength + SaclLength);
      if (NewSd == NULL)
        {
          ObDereferenceObject(Object);
          return STATUS_INSUFFICIENT_RESOURCES;
        }

      RtlCreateSecurityDescriptor(NewSd,
                                  SECURITY_DESCRIPTOR_REVISION1);
      /* We always build a self-relative descriptor */
      NewSd->Control = (USHORT)Control | SE_SELF_RELATIVE;

      Current = (ULONG_PTR)NewSd + sizeof(SECURITY_DESCRIPTOR);

      if (OwnerLength != 0)
        {
          RtlCopyMemory((PVOID)Current,
                        Owner,
                        OwnerLength);
          NewSd->Owner = (PSID)(Current - (ULONG_PTR)NewSd);
          Current += OwnerLength;
        }

      if (GroupLength != 0)
        {
          RtlCopyMemory((PVOID)Current,
                        Group,
                        GroupLength);
          NewSd->Group = (PSID)(Current - (ULONG_PTR)NewSd);
          Current += GroupLength;
        }

      if (DaclLength != 0)
        {
          RtlCopyMemory((PVOID)Current,
                        Dacl,
                        DaclLength);
          NewSd->Dacl = (PACL)(Current - (ULONG_PTR)NewSd);
          Current += DaclLength;
        }

      if (SaclLength != 0)
        {
          RtlCopyMemory((PVOID)Current,
                        Sacl,
                        SaclLength);
          NewSd->Sacl = (PACL)(Current - (ULONG_PTR)NewSd);
          Current += SaclLength;
        }

      /* Add the new SD */
      Status = ObpAddSecurityDescriptor(NewSd,
                                        &Header->SecurityDescriptor);
      if (NT_SUCCESS(Status))
        {
          /* Remove the old security descriptor */
          ObpRemoveSecurityDescriptor(ObjectSd);
        }
      else
        {
          /* Restore the old security descriptor */
          Header->SecurityDescriptor = ObjectSd;
        }

      ExFreePool(NewSd);
    }
    else if (OperationType == QuerySecurityDescriptor)
    {
        Status = SeQuerySecurityDescriptorInfo(_SecurityInformation,
                                               SecurityDescriptor,
                                               ReturnLength,
                                               &Header->SecurityDescriptor);
    }
    else if (OperationType == AssignSecurityDescriptor)
    {
      /* Assign the security descriptor to the object header */
      Status = ObpAddSecurityDescriptor(SecurityDescriptor,
                                        &Header->SecurityDescriptor);
    }


    return STATUS_SUCCESS;
}

VOID
NTAPI
SeCaptureSubjectContextEx(IN PETHREAD Thread,
                          IN PEPROCESS Process,
                          OUT PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
    BOOLEAN CopyOnOpen, EffectiveOnly;
    PAGED_CODE();

    /* Save the unique ID */
    SubjectContext->ProcessAuditId = Process->UniqueProcessId;

    /* Check if we have a thread */
    if (!Thread)
    {
        /* We don't, so no token */
        SubjectContext->ClientToken = NULL;
    }
    else
    {
        /* Get the impersonation token */
        SubjectContext->ClientToken =
            PsReferenceImpersonationToken(Thread,
                                          &CopyOnOpen,
                                          &EffectiveOnly,
                                          &SubjectContext->ImpersonationLevel);
    }

    /* Get the primary token */
    SubjectContext->PrimaryToken = PsReferencePrimaryToken(Process);
}

/*
 * @implemented
 */
VOID
NTAPI
SeCaptureSubjectContext(OUT PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
    /* Call the internal API */
    SeCaptureSubjectContextEx(PsGetCurrentThread(),
                              PsGetCurrentProcess(),
                              SubjectContext);
}


/*
 * @implemented
 */
VOID STDCALL
SeLockSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
  PAGED_CODE();

  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&SepSubjectContextLock, TRUE);
}


/*
 * @implemented
 */
VOID STDCALL
SeUnlockSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
  PAGED_CODE();

  ExReleaseResourceLite(&SepSubjectContextLock);
  KeLeaveCriticalRegion();
}


/*
 * @implemented
 */
VOID STDCALL
SeReleaseSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
  PAGED_CODE();

  if (SubjectContext->PrimaryToken != NULL)
    {
      ObFastDereferenceObject(&PsGetCurrentProcess()->Token, SubjectContext->PrimaryToken);
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
  PAGED_CODE();

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
SeAssignSecurity(PSECURITY_DESCRIPTOR _ParentDescriptor OPTIONAL,
                 PSECURITY_DESCRIPTOR _ExplicitDescriptor OPTIONAL,
                 PSECURITY_DESCRIPTOR *NewDescriptor,
                 BOOLEAN IsDirectoryObject,
                 PSECURITY_SUBJECT_CONTEXT SubjectContext,
                 PGENERIC_MAPPING GenericMapping,
                 POOL_TYPE PoolType)
{
  PISECURITY_DESCRIPTOR ParentDescriptor = _ParentDescriptor;
  PISECURITY_DESCRIPTOR ExplicitDescriptor = _ExplicitDescriptor;
  PISECURITY_DESCRIPTOR Descriptor;
  PTOKEN Token;
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

  PAGED_CODE();

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

  Descriptor = ExAllocatePool(PagedPool,
                              Length);
  if (Descriptor == NULL)
    {
      DPRINT1("ExAlloctePool() failed\n");
      /* FIXME: Unlock subject context */
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  RtlZeroMemory( Descriptor, Length );
  RtlCreateSecurityDescriptor(Descriptor,
                              SECURITY_DESCRIPTOR_REVISION);

  Descriptor->Control = (USHORT)Control | SE_SELF_RELATIVE;

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
SepSidInToken(PACCESS_TOKEN _Token,
              PSID Sid)
{
  ULONG i;
  PTOKEN Token = (PTOKEN)_Token;

  PAGED_CODE();

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
    ACCESS_MASK CurrentAccess, AccessMask;
    PACCESS_TOKEN Token;
    ULONG i;
    PACL Dacl;
    BOOLEAN Present;
    BOOLEAN Defaulted;
    PACE CurrentAce;
    PSID Sid;
    NTSTATUS Status;
    PAGED_CODE();

    /* Check if this is kernel mode */
    if (AccessMode == KernelMode)
    {
        /* Check if kernel wants everything */
        if (DesiredAccess & MAXIMUM_ALLOWED)
        {
            /* Give it */
            *GrantedAccess = GenericMapping->GenericAll;
            *GrantedAccess |= (DesiredAccess &~ MAXIMUM_ALLOWED);
            *GrantedAccess |= PreviouslyGrantedAccess;
        }
        else
        {
            /* Give the desired and previous access */
            *GrantedAccess = DesiredAccess | PreviouslyGrantedAccess;
        }

        /* Success */
        *AccessStatus = STATUS_SUCCESS;
        return TRUE;
    }

    /* Check if we didn't get an SD */
    if (!SecurityDescriptor)
    {
        /* Automatic failure */
        *AccessStatus = STATUS_ACCESS_DENIED;
        return FALSE;
    }

    /* Check for invalid impersonation */
    if ((SubjectSecurityContext->ClientToken) &&
        (SubjectSecurityContext->ImpersonationLevel < SecurityImpersonation))
    {
        *AccessStatus = STATUS_BAD_IMPERSONATION_LEVEL;
        return FALSE;
    }

    /* Check for no access desired */
    if (!DesiredAccess)
    {
        /* Check if we had no previous access */
        if (!PreviouslyGrantedAccess)
        {
            /* Then there's nothing to give */
            *AccessStatus = STATUS_ACCESS_DENIED;
            return FALSE;
        }

        /* Return the previous access only */
        *GrantedAccess = PreviouslyGrantedAccess;
        *AccessStatus = STATUS_SUCCESS;
        *Privileges = NULL;
        return TRUE;
    }

    /* Acquire the lock if needed */
    if (!SubjectContextLocked) SeLockSubjectContext(SubjectSecurityContext);

  /* Map given accesses */
  RtlMapGenericMask(&DesiredAccess, GenericMapping);
  if (PreviouslyGrantedAccess)
    RtlMapGenericMask(&PreviouslyGrantedAccess, GenericMapping);



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
      if ((DesiredAccess & ~VALID_INHERIT_FLAGS) == 
          (CurrentAccess & ~VALID_INHERIT_FLAGS))
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

  if (Sid && SepSidInToken(Token, Sid))
    {
      CurrentAccess |= (READ_CONTROL | WRITE_DAC);
      if ((DesiredAccess & ~VALID_INHERIT_FLAGS) == 
          (CurrentAccess & ~VALID_INHERIT_FLAGS))
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
      return FALSE;
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
              return FALSE;
            }
        }

      else if (CurrentAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
        {
          if (SepSidInToken(Token, Sid))
            {
              AccessMask = CurrentAce->AccessMask;
              RtlMapGenericMask(&AccessMask, GenericMapping);
              CurrentAccess |= AccessMask;
            }
        }
        else
        {
          DPRINT1("Unknown Ace type 0x%lx\n", CurrentAce->Header.AceType);
      }
        CurrentAce = (PACE)((ULONG_PTR)CurrentAce + CurrentAce->Header.AceSize);
    }

  if (SubjectContextLocked == FALSE)
    {
      SeUnlockSubjectContext(SubjectSecurityContext);
    }

  DPRINT("CurrentAccess %08lx\n DesiredAccess %08lx\n",
         CurrentAccess, DesiredAccess);

  *GrantedAccess = CurrentAccess & DesiredAccess;

  if (DesiredAccess & MAXIMUM_ALLOWED)
    {
      *GrantedAccess = CurrentAccess;
      *AccessStatus = STATUS_SUCCESS;
      return TRUE;
    }
  else if ((*GrantedAccess & ~VALID_INHERIT_FLAGS) == 
           (DesiredAccess & ~VALID_INHERIT_FLAGS))
    {
      *AccessStatus = STATUS_SUCCESS;
      return TRUE;
    }
  else
    {
      DPRINT1("Denying access for caller: granted 0x%lx, desired 0x%lx (generic mapping %p)\n",
        *GrantedAccess, DesiredAccess, GenericMapping);
      *AccessStatus = STATUS_ACCESS_DENIED;
      return FALSE;
    }
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
  SECURITY_SUBJECT_CONTEXT SubjectSecurityContext = {0};
  KPROCESSOR_MODE PreviousMode;
  PTOKEN Token;
  NTSTATUS Status;

  PAGED_CODE();

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

NTSTATUS
NTAPI
NtAccessCheckByType(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                    IN PSID PrincipalSelfSid,
                    IN HANDLE ClientToken,
                    IN ACCESS_MASK DesiredAccess,
                    IN POBJECT_TYPE_LIST ObjectTypeList,
                    IN ULONG ObjectTypeLength,
                    IN PGENERIC_MAPPING GenericMapping,
                    IN PPRIVILEGE_SET PrivilegeSet,
                    IN ULONG PrivilegeSetLength,
                    OUT PACCESS_MASK GrantedAccess,
                    OUT PNTSTATUS AccessStatus)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtAccessCheckByTypeAndAuditAlarm(IN PUNICODE_STRING SubsystemName,
                                 IN HANDLE HandleId,
                                 IN PUNICODE_STRING ObjectTypeName,
                                 IN PUNICODE_STRING ObjectName,
                                 IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                 IN PSID PrincipalSelfSid,
                                 IN ACCESS_MASK DesiredAccess,
                                 IN AUDIT_EVENT_TYPE AuditType,
                                 IN ULONG Flags,
                                 IN POBJECT_TYPE_LIST ObjectTypeList,
                                 IN ULONG ObjectTypeLength,
                                 IN PGENERIC_MAPPING GenericMapping,
                                 IN BOOLEAN ObjectCreation,
                                 OUT PACCESS_MASK GrantedAccess,
                                 OUT PNTSTATUS AccessStatus,
                                 OUT PBOOLEAN GenerateOnClose)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtAccessCheckByTypeResultList(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                              IN PSID PrincipalSelfSid,
                              IN HANDLE ClientToken,
                              IN ACCESS_MASK DesiredAccess,
                              IN POBJECT_TYPE_LIST ObjectTypeList,
                              IN ULONG ObjectTypeLength,
                              IN PGENERIC_MAPPING GenericMapping,
                              IN PPRIVILEGE_SET PrivilegeSet,
                              IN ULONG PrivilegeSetLength,
                              OUT PACCESS_MASK GrantedAccess,
                              OUT PNTSTATUS AccessStatus)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtAccessCheckByTypeResultListAndAuditAlarm(IN PUNICODE_STRING SubsystemName,
                                           IN HANDLE HandleId,
                                           IN PUNICODE_STRING ObjectTypeName,
                                           IN PUNICODE_STRING ObjectName,
                                           IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                           IN PSID PrincipalSelfSid,
                                           IN ACCESS_MASK DesiredAccess,
                                           IN AUDIT_EVENT_TYPE AuditType,
                                           IN ULONG Flags,
                                           IN POBJECT_TYPE_LIST ObjectTypeList,
                                           IN ULONG ObjectTypeLength,
                                           IN PGENERIC_MAPPING GenericMapping,
                                           IN BOOLEAN ObjectCreation,
                                           OUT PACCESS_MASK GrantedAccess,
                                           OUT PNTSTATUS AccessStatus,
                                           OUT PBOOLEAN GenerateOnClose)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtAccessCheckByTypeResultListAndAuditAlarmByHandle(IN PUNICODE_STRING SubsystemName,
                                                   IN HANDLE HandleId,
                                                   IN HANDLE ClientToken,
                                                   IN PUNICODE_STRING ObjectTypeName,
                                                   IN PUNICODE_STRING ObjectName,
                                                   IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                                   IN PSID PrincipalSelfSid,
                                                   IN ACCESS_MASK DesiredAccess,
                                                   IN AUDIT_EVENT_TYPE AuditType,
                                                   IN ULONG Flags,
                                                   IN POBJECT_TYPE_LIST ObjectTypeList,
                                                   IN ULONG ObjectTypeLength,
                                                   IN PGENERIC_MAPPING GenericMapping,
                                                   IN BOOLEAN ObjectCreation,
                                                   OUT PACCESS_MASK GrantedAccess,
                                                   OUT PNTSTATUS AccessStatus,
                                                   OUT PBOOLEAN GenerateOnClose)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID STDCALL
SeQuerySecurityAccessMask(IN SECURITY_INFORMATION SecurityInformation,
                          OUT PACCESS_MASK DesiredAccess)
{
    if (SecurityInformation & (OWNER_SECURITY_INFORMATION |
                               GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION))
    {
        *DesiredAccess |= READ_CONTROL;
    }
    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        *DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }
}

VOID STDCALL
SeSetSecurityAccessMask(IN SECURITY_INFORMATION SecurityInformation,
                        OUT PACCESS_MASK DesiredAccess)
{
    if (SecurityInformation & (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION))
    {
        *DesiredAccess |= WRITE_OWNER;
    }
    if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
        *DesiredAccess |= WRITE_DAC;
    }
    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        *DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }
}

/* EOF */
