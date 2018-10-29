/*
 * PROJECT:     ReactOS Named Pipe FileSystem
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/filesystems/npfs/create.c
 * PURPOSE:     Pipes Creation
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "npfs.h"

// File ID number for NPFS bugchecking support
#define NPFS_BUGCHECK_FILE_ID   (NPFS_BUGCHECK_CREATE)

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
NpCheckForNotify(IN PNP_DCB Dcb,
                 IN BOOLEAN SecondList,
                 IN PLIST_ENTRY List)
{
    PLIST_ENTRY NextEntry, ListHead;
    PIRP Irp;
    ULONG i;
    PAGED_CODE();

    ListHead = &Dcb->NotifyList;
    for (i = 0; i < 2; i++)
    {
        ASSERT(IsListEmpty(ListHead));
        while (!IsListEmpty(ListHead))
        {
            NextEntry = RemoveHeadList(ListHead);

            Irp = CONTAINING_RECORD(NextEntry, IRP, Tail.Overlay.ListEntry);

            if (IoSetCancelRoutine(Irp, NULL))
            {
                Irp->IoStatus.Status = STATUS_SUCCESS;
                InsertTailList(List, NextEntry);
            }
            else
            {
                InitializeListHead(NextEntry);
            }
        }

        if (!SecondList) break;
        ListHead = &Dcb->NotifyList2;
    }
}

IO_STATUS_BLOCK
NTAPI
NpOpenNamedPipeFileSystem(IN PFILE_OBJECT FileObject,
                          IN ACCESS_MASK DesiredAccess)
{
    IO_STATUS_BLOCK Status;
    PAGED_CODE();
    TRACE("Entered\n");

    NpSetFileObject(FileObject, NpVcb, NULL, FALSE);
    ++NpVcb->ReferenceCount;

    Status.Information = FILE_OPENED;
    Status.Status = STATUS_SUCCESS;
    TRACE("Leaving, Status.Status = %lx\n", Status.Status);
    return Status;
}

IO_STATUS_BLOCK
NTAPI
NpOpenNamedPipeRootDirectory(IN PNP_DCB Dcb,
                             IN PFILE_OBJECT FileObject,
                             IN ACCESS_MASK DesiredAccess,
                             IN PLIST_ENTRY List)
{
    IO_STATUS_BLOCK IoStatus;
    PNP_ROOT_DCB_FCB Ccb;
    PAGED_CODE();
    TRACE("Entered\n");

    IoStatus.Status = NpCreateRootDcbCcb(&Ccb);
    if (NT_SUCCESS(IoStatus.Status))
    {
        NpSetFileObject(FileObject, Dcb, Ccb, FALSE);
        ++Dcb->CurrentInstances;

        IoStatus.Information = FILE_OPENED;
        IoStatus.Status = STATUS_SUCCESS;
    }
    else
    {
        IoStatus.Information = 0;
    }

    TRACE("Leaving, IoStatus.Status = %lx\n", IoStatus.Status);
    return IoStatus;
}

IO_STATUS_BLOCK
NTAPI
NpCreateClientEnd(IN PNP_FCB Fcb,
                  IN PFILE_OBJECT FileObject,
                  IN ACCESS_MASK DesiredAccess,
                  IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
                  IN PACCESS_STATE AccessState,
                  IN KPROCESSOR_MODE PreviousMode,
                  IN PETHREAD Thread,
                  IN PLIST_ENTRY List)
{
    PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
    BOOLEAN AccessGranted;
    ACCESS_MASK GrantedAccess;
    PPRIVILEGE_SET Privileges;
    UNICODE_STRING ObjectTypeName;
    IO_STATUS_BLOCK IoStatus;
    USHORT NamedPipeConfiguration;
    PLIST_ENTRY NextEntry, ListHead;
    PNP_CCB Ccb = NULL;
    TRACE("Entered\n");

    IoStatus.Information = 0;
    Privileges = NULL;

    NamedPipeConfiguration = Fcb->NamedPipeConfiguration;

    SubjectSecurityContext = &AccessState->SubjectSecurityContext;
    SeLockSubjectContext(SubjectSecurityContext);

    AccessGranted = SeAccessCheck(Fcb->SecurityDescriptor,
                                  SubjectSecurityContext,
                                  TRUE,
                                  DesiredAccess & ~4,
                                  0,
                                  &Privileges,
                                  IoGetFileObjectGenericMapping(),
                                  PreviousMode,
                                  &GrantedAccess,
                                  &IoStatus.Status);

    if (Privileges)
    {
        SeAppendPrivileges(AccessState, Privileges);
        SeFreePrivileges(Privileges);
    }

    if (AccessGranted)
    {
        AccessState->PreviouslyGrantedAccess |= GrantedAccess;
        AccessState->RemainingDesiredAccess &= ~(GrantedAccess | MAXIMUM_ALLOWED);
    }

    ObjectTypeName.Buffer = L"NamedPipe";
    ObjectTypeName.Length = 18;
    SeOpenObjectAuditAlarm(&ObjectTypeName,
                           NULL,
                           &FileObject->FileName,
                           Fcb->SecurityDescriptor,
                           AccessState,
                           FALSE,
                           AccessGranted,
                           PreviousMode,
                           &AccessState->GenerateOnClose);
    SeUnlockSubjectContext(SubjectSecurityContext);
    if (!AccessGranted) return IoStatus;

    if (((GrantedAccess & FILE_READ_DATA) && (NamedPipeConfiguration == FILE_PIPE_INBOUND)) ||
        ((GrantedAccess & FILE_WRITE_DATA) && (NamedPipeConfiguration == FILE_PIPE_OUTBOUND)))
    {
        IoStatus.Status = STATUS_ACCESS_DENIED;
        TRACE("Leaving, IoStatus.Status = %lx\n", IoStatus.Status);
        return IoStatus;
    }

    if (!(GrantedAccess & (FILE_READ_DATA | FILE_WRITE_DATA))) SecurityQos = NULL;

    ListHead = &Fcb->CcbList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        Ccb = CONTAINING_RECORD(NextEntry, NP_CCB, CcbEntry);
        if (Ccb->NamedPipeState == FILE_PIPE_LISTENING_STATE) break;

        NextEntry = NextEntry->Flink;
    }

    if (NextEntry == ListHead)
    {
        IoStatus.Status = STATUS_PIPE_NOT_AVAILABLE;
        TRACE("Leaving, IoStatus.Status = %lx\n", IoStatus.Status);
        return IoStatus;
    }

    IoStatus.Status = NpInitializeSecurity(Ccb, SecurityQos, Thread);
    if (!NT_SUCCESS(IoStatus.Status)) return IoStatus;

    IoStatus.Status = NpSetConnectedPipeState(Ccb, FileObject, List);
    if (!NT_SUCCESS(IoStatus.Status))
    {
        NpUninitializeSecurity(Ccb);
        TRACE("Leaving, IoStatus.Status = %lx\n", IoStatus.Status);
        return IoStatus;
    }

    Ccb->ClientSession = NULL;
    Ccb->Process = IoThreadToProcess(Thread);

    IoStatus.Information = FILE_OPENED;
    IoStatus.Status = STATUS_SUCCESS;
    TRACE("Leaving, IoStatus.Status = %lx\n", IoStatus.Status);
    return IoStatus;
}

NTSTATUS
NTAPI
NpTranslateAlias(
    PUNICODE_STRING PipeName)
{
    WCHAR UpcaseBuffer[MAX_INDEXED_LENGTH + 1];
    UNICODE_STRING UpcaseString;
    ULONG Length;
    PNPFS_ALIAS CurrentAlias;
    NTSTATUS Status;
    BOOLEAN BufferAllocated, BackSlash;
    LONG Result;
    PAGED_CODE();

    /* Get the pipe name length and check for empty string */
    Length = PipeName->Length;
    if (Length == 0)
    {
        return STATUS_SUCCESS;
    }

    /* Check if the name starts with a path separator */
    BackSlash = (PipeName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR);
    if (BackSlash)
    {
        /* We are only interested in the part after the backslash */
        Length -= sizeof(WCHAR);
    }

    /* Check if the length is within our indexed list bounds */
    if ((Length >= MIN_INDEXED_LENGTH * sizeof(WCHAR)) &&
        (Length <= MAX_INDEXED_LENGTH * sizeof(WCHAR)))
    {
        /* Length is within bounds, use the list by length */
        CurrentAlias = NpAliasListByLength[(Length / sizeof(WCHAR)) - MIN_INDEXED_LENGTH];
    }
    else
    {
        /* We use the generic list, search for an entry of the right size */
        CurrentAlias = NpAliasList;
        while ((CurrentAlias != NULL) && (CurrentAlias->Name.Length != Length))
        {
            /* Check if we went past the desired length */
            if (CurrentAlias->Name.Length > Length)
            {
                /* In this case there is no matching alias, return success */
                return STATUS_SUCCESS;
            }

            /* Go to the next alias in the list */
            CurrentAlias = CurrentAlias->Next;
        }
    }

    /* Did we find any alias? */
    if (CurrentAlias == NULL)
    {
        /* Nothing found, no matching alias */
        return STATUS_SUCCESS;
    }

    /* Check whether we can use our stack buffer */
    if (Length <= MAX_INDEXED_LENGTH * sizeof(WCHAR))
    {
        /* Initialize the upcased string */
        UpcaseString.Buffer = UpcaseBuffer;
        UpcaseString.MaximumLength = sizeof(UpcaseBuffer);

        /* Upcase the pipe name */
        Status = RtlUpcaseUnicodeString(&UpcaseString, PipeName, FALSE);
        NT_ASSERT(NT_SUCCESS(Status));
        BufferAllocated = FALSE;
    }
    else
    {
        /* Upcase the pipe name, allocate the string buffer */
        Status = RtlUpcaseUnicodeString(&UpcaseString, PipeName, TRUE);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        BufferAllocated = TRUE;
    }

    /* Did the original name start with a backslash? */
    if (BackSlash)
    {
        /* Skip it for the comparison */
        UpcaseString.Buffer++;
        UpcaseString.Length -= sizeof(WCHAR);
    }

    /* Make sure the length matches the "raw" length */
    NT_ASSERT(UpcaseString.Length == Length);
    NT_ASSERT(CurrentAlias->Name.Length == Length);

    /* Loop while we have aliases */
    do
    {
        /* Compare the names and check if they match */
        Result = NpCompareAliasNames(&UpcaseString, &CurrentAlias->Name);
        if (Result == 0)
        {
            /* The names match, use the target name */
            *PipeName = *CurrentAlias->TargetName;

            /* Did the original name start with a backslash? */
            if (!BackSlash)
            {
                /* It didn't, so skip it in the target name as well */
                PipeName->Buffer++;
                PipeName->Length -= sizeof(WCHAR);
            }
            break;
        }

        /* Check if we went past all string candidates */
        if (Result < 0)
        {
            /* Nothing found, we're done */
            break;
        }

        /* Go to the next alias */
        CurrentAlias = CurrentAlias->Next;

        /* Keep looping while we have aliases of the right length */
    } while ((CurrentAlias != NULL) && (CurrentAlias->Name.Length == Length));

    /* Did we allocate a buffer? */
    if (BufferAllocated)
    {
        /* Free the allocated buffer */
        ASSERT(UpcaseString.Buffer != UpcaseBuffer);
        RtlFreeUnicodeString(&UpcaseString);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpFsdCreate(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp)
{
    IO_STATUS_BLOCK IoStatus;
    PIO_STACK_LOCATION IoStack;
    UNICODE_STRING FileName;
    PFILE_OBJECT FileObject;
    PFILE_OBJECT RelatedFileObject;
    NODE_TYPE_CODE Type;
    PNP_CCB Ccb;
    PNP_FCB Fcb;
    PNP_DCB Dcb;
    ACCESS_MASK DesiredAccess;
    LIST_ENTRY DeferredList;
    UNICODE_STRING Prefix;
    TRACE("Entered\n");

    InitializeListHead(&DeferredList);
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    FileObject = IoStack->FileObject;
    RelatedFileObject = FileObject->RelatedFileObject;
    FileName = FileObject->FileName;
    DesiredAccess = IoStack->Parameters.CreatePipe.SecurityContext->DesiredAccess;

    IoStatus.Information = 0;

    FsRtlEnterFileSystem();
    NpAcquireExclusiveVcb();

    if (RelatedFileObject)
    {
        Type = NpDecodeFileObject(RelatedFileObject, (PVOID*)&Fcb, &Ccb, FALSE);
    }
    else
    {
        Type = 0;
        Fcb = NULL;
        Ccb = NULL;
    }

    if (FileName.Length)
    {
        if ((FileName.Length == sizeof(OBJ_NAME_PATH_SEPARATOR)) &&
            (FileName.Buffer[0] == OBJ_NAME_PATH_SEPARATOR) &&
            !(RelatedFileObject))
        {
            IoStatus = NpOpenNamedPipeRootDirectory(NpVcb->RootDcb,
                                                    FileObject,
                                                    DesiredAccess,
                                                    &DeferredList);
            goto Quickie;
        }
    }
    else if (!(RelatedFileObject) || (Type == NPFS_NTC_VCB))
    {
        IoStatus = NpOpenNamedPipeFileSystem(FileObject,
                                             DesiredAccess);
        goto Quickie;
    }
    else if (Type == NPFS_NTC_ROOT_DCB)
    {
        IoStatus = NpOpenNamedPipeRootDirectory(NpVcb->RootDcb,
                                                FileObject,
                                                DesiredAccess,
                                                &DeferredList);
        goto Quickie;
    }

    IoStatus.Status = NpTranslateAlias(&FileName);
    if (!NT_SUCCESS(IoStatus.Status)) goto Quickie;

    if (RelatedFileObject)
    {
        if (Type == NPFS_NTC_ROOT_DCB)
        {
            Dcb = (PNP_DCB)Ccb;
            IoStatus.Status = NpFindRelativePrefix(Dcb,
                                                   &FileName,
                                                   1,
                                                   &Prefix,
                                                   &Fcb);
            if (!NT_SUCCESS(IoStatus.Status))
            {
                goto Quickie;
            }
        }
        else if ((Type != NPFS_NTC_CCB) || (FileName.Length))
        {
            IoStatus.Status = STATUS_OBJECT_NAME_INVALID;
            goto Quickie;
        }
        else
        {
            Prefix.Length = 0;
        }
    }
    else
    {
        if ((FileName.Length <= sizeof(OBJ_NAME_PATH_SEPARATOR)) ||
            (FileName.Buffer[0] != OBJ_NAME_PATH_SEPARATOR))
        {
            IoStatus.Status = STATUS_OBJECT_NAME_INVALID;
            goto Quickie;
        }

        Fcb = NpFindPrefix(&FileName, 1, &Prefix);
    }

    if (Prefix.Length)
    {
        IoStatus.Status = Fcb->NodeType != NPFS_NTC_FCB ?
                           STATUS_OBJECT_NAME_NOT_FOUND :
                           STATUS_OBJECT_NAME_INVALID;
        goto Quickie;
    }

    if (Fcb->NodeType != NPFS_NTC_FCB)
    {
        IoStatus.Status = STATUS_OBJECT_NAME_INVALID;
        goto Quickie;
    }

    if (!Fcb->ServerOpenCount)
    {
        IoStatus.Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Quickie;
    }

    IoStatus = NpCreateClientEnd(Fcb,
                                 FileObject,
                                 DesiredAccess,
                                 IoStack->Parameters.CreatePipe.
                                 SecurityContext->SecurityQos,
                                 IoStack->Parameters.CreatePipe.
                                 SecurityContext->AccessState,
                                 IoStack->Flags &
                                 SL_FORCE_ACCESS_CHECK ?
                                 UserMode : Irp->RequestorMode,
                                 Irp->Tail.Overlay.Thread,
                                 &DeferredList);

Quickie:
    NpReleaseVcb();
    NpCompleteDeferredIrps(&DeferredList);
    FsRtlExitFileSystem();

    Irp->IoStatus = IoStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    TRACE("Leaving, IoStatus.Status = %lx\n", IoStatus.Status);
    return IoStatus.Status;
}

IO_STATUS_BLOCK
NTAPI
NpCreateExistingNamedPipe(IN PNP_FCB Fcb,
                          IN PFILE_OBJECT FileObject,
                          IN ACCESS_MASK DesiredAccess,
                          IN PACCESS_STATE AccessState,
                          IN KPROCESSOR_MODE PreviousMode,
                          IN ULONG Disposition,
                          IN ULONG ShareAccess,
                          IN PNAMED_PIPE_CREATE_PARAMETERS Parameters,
                          IN PEPROCESS Process,
                          OUT PLIST_ENTRY List)
{
    PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
    IO_STATUS_BLOCK IoStatus;
    UNICODE_STRING ObjectTypeName;
    ACCESS_MASK GrantedAccess;
    PNP_CCB Ccb;
    PPRIVILEGE_SET Privileges;
    USHORT NamedPipeConfiguration, CheckShareAccess;
    BOOLEAN AccessGranted;
    PAGED_CODE();
    TRACE("Entered\n");

    Privileges = NULL;

    NamedPipeConfiguration = Fcb->NamedPipeConfiguration;

    SubjectSecurityContext = &AccessState->SubjectSecurityContext;
    SeLockSubjectContext(SubjectSecurityContext);

    IoStatus.Information = 0;

    AccessGranted = SeAccessCheck(Fcb->SecurityDescriptor,
                                  SubjectSecurityContext,
                                  TRUE,
                                  DesiredAccess | 4,
                                  0,
                                  &Privileges,
                                  IoGetFileObjectGenericMapping(),
                                  PreviousMode,
                                  &GrantedAccess,
                                  &IoStatus.Status);

    if (Privileges)
    {
        SeAppendPrivileges(AccessState, Privileges);
        SeFreePrivileges(Privileges);
    }

    if (AccessGranted)
    {
        AccessState->PreviouslyGrantedAccess |= GrantedAccess;
        AccessState->RemainingDesiredAccess &= ~(GrantedAccess | MAXIMUM_ALLOWED);
    }

    ObjectTypeName.Buffer = L"NamedPipe";
    ObjectTypeName.Length = 18;
    SeOpenObjectAuditAlarm(&ObjectTypeName,
                           NULL,
                           &FileObject->FileName,
                           Fcb->SecurityDescriptor,
                           AccessState,
                           FALSE,
                           AccessGranted,
                           PreviousMode,
                           &AccessState->GenerateOnClose);

    SeUnlockSubjectContext(SubjectSecurityContext);
    if (!AccessGranted)
    {
        TRACE("Leaving, IoStatus.Status = %lx\n", IoStatus.Status);
        return IoStatus;
    }

    if (Fcb->CurrentInstances >= Fcb->MaximumInstances)
    {
        IoStatus.Status = STATUS_INSTANCE_NOT_AVAILABLE;
        TRACE("Leaving, IoStatus.Status = %lx\n", IoStatus.Status);
        return IoStatus;
    }

    if (Disposition == FILE_CREATE)
    {
        IoStatus.Status = STATUS_ACCESS_DENIED;
        TRACE("Leaving, IoStatus.Status = %lx\n", IoStatus.Status);
        return IoStatus;
    }

    CheckShareAccess = 0;
    if (NamedPipeConfiguration == FILE_PIPE_FULL_DUPLEX)
    {
        CheckShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;
    }
    else if (NamedPipeConfiguration == FILE_PIPE_OUTBOUND)
    {
        CheckShareAccess = FILE_SHARE_READ;
    }
    else if (NamedPipeConfiguration == FILE_PIPE_INBOUND)
    {
        CheckShareAccess = FILE_SHARE_WRITE;
    }

    if (CheckShareAccess != ShareAccess)
    {
        IoStatus.Status = STATUS_ACCESS_DENIED;
        TRACE("Leaving, IoStatus.Status = %lx\n", IoStatus.Status);
        return IoStatus;
    }

    IoStatus.Status = NpCreateCcb(Fcb,
                                  FileObject,
                                  FILE_PIPE_LISTENING_STATE,
                                  Parameters->ReadMode & 0xFF,
                                  Parameters->CompletionMode & 0xFF,
                                  Parameters->InboundQuota,
                                  Parameters->OutboundQuota,
                                  &Ccb);
    if (!NT_SUCCESS(IoStatus.Status)) return IoStatus;

    IoStatus.Status = NpCancelWaiter(&NpVcb->WaitQueue,
                                     &Fcb->FullName,
                                     FALSE,
                                     List);
    if (!NT_SUCCESS(IoStatus.Status))
    {
        --Ccb->Fcb->CurrentInstances;
        NpDeleteCcb(Ccb, List);
        TRACE("Leaving, IoStatus.Status = %lx\n", IoStatus.Status);
        return IoStatus;
    }

    NpSetFileObject(FileObject, Ccb, Ccb->NonPagedCcb, TRUE);
    Ccb->FileObject[FILE_PIPE_SERVER_END] = FileObject;
    NpCheckForNotify(Fcb->ParentDcb, 0, List);

    IoStatus.Status = STATUS_SUCCESS;
    IoStatus.Information = FILE_OPENED;
    TRACE("Leaving, IoStatus.Status = %lx\n", IoStatus.Status);
    return IoStatus;
}

NTSTATUS
NTAPI
NpCreateNewNamedPipe(IN PNP_DCB Dcb,
                     IN PFILE_OBJECT FileObject,
                     IN UNICODE_STRING PipeName,
                     IN ACCESS_MASK DesiredAccess,
                     IN PACCESS_STATE AccessState,
                     IN USHORT Disposition,
                     IN USHORT ShareAccess,
                     IN PNAMED_PIPE_CREATE_PARAMETERS Parameters,
                     IN PEPROCESS Process,
                     IN PLIST_ENTRY List,
                     OUT PIO_STATUS_BLOCK IoStatus)
{
    NTSTATUS Status;
    USHORT NamedPipeConfiguration;
    PSECURITY_SUBJECT_CONTEXT SecurityContext;
    PSECURITY_DESCRIPTOR SecurityDescriptor, CachedSecurityDescriptor;
    PNP_CCB Ccb;
    PNP_FCB Fcb;
    PAGED_CODE();
    TRACE("Entered\n");

    if (!(Parameters->TimeoutSpecified) ||
        !(Parameters->MaximumInstances) ||
        (Parameters->DefaultTimeout.QuadPart >= 0))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    if (Disposition == FILE_OPEN)
    {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Quickie;
    }

    if (ShareAccess == (FILE_SHARE_READ | FILE_SHARE_WRITE))
    {
        NamedPipeConfiguration = FILE_PIPE_FULL_DUPLEX;
    }
    else if (ShareAccess == FILE_SHARE_READ)
    {
        NamedPipeConfiguration = FILE_PIPE_OUTBOUND;
    }
    else if (ShareAccess == FILE_SHARE_WRITE)
    {
        NamedPipeConfiguration = FILE_PIPE_INBOUND;
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    if (Parameters->NamedPipeType == FILE_PIPE_BYTE_STREAM_TYPE &&
        Parameters->ReadMode == FILE_PIPE_MESSAGE_MODE)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    Status = NpCreateFcb(Dcb,
                         &PipeName,
                         Parameters->MaximumInstances,
                         Parameters->DefaultTimeout,
                         NamedPipeConfiguration,
                         Parameters->NamedPipeType & 0xFFFF,
                         &Fcb);
    if (!NT_SUCCESS(Status)) goto Quickie;

    Status = NpCreateCcb(Fcb,
                         FileObject,
                         FILE_PIPE_LISTENING_STATE,
                         Parameters->ReadMode & 0xFF,
                         Parameters->CompletionMode & 0xFF,
                         Parameters->InboundQuota,
                         Parameters->OutboundQuota,
                         &Ccb);
    if (!NT_SUCCESS(Status))
    {
        NpDeleteFcb(Fcb, List);
        goto Quickie;
    }

    SecurityContext = &AccessState->SubjectSecurityContext;
    SeLockSubjectContext(SecurityContext);

    Status = SeAssignSecurity(NULL,
                              AccessState->SecurityDescriptor,
                              &SecurityDescriptor,
                              FALSE,
                              SecurityContext,
                              IoGetFileObjectGenericMapping(),
                              PagedPool);
    SeUnlockSubjectContext(SecurityContext);
    if (!NT_SUCCESS(Status))
    {
        NpDeleteCcb(Ccb, List);
        NpDeleteFcb(Fcb, List);
        goto Quickie;
    }

    Status = ObLogSecurityDescriptor(SecurityDescriptor,
                                     &CachedSecurityDescriptor,
                                     1);
    ExFreePoolWithTag(SecurityDescriptor, 0);

    if (!NT_SUCCESS(Status))
    {
        NpDeleteCcb(Ccb, List);
        NpDeleteFcb(Fcb, List);
        goto Quickie;
    }

    Fcb->SecurityDescriptor = CachedSecurityDescriptor;

    NpSetFileObject(FileObject, Ccb, Ccb->NonPagedCcb, TRUE);
    Ccb->FileObject[FILE_PIPE_SERVER_END] = FileObject;

    NpCheckForNotify(Dcb, TRUE, List);

    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = FILE_CREATED;

    TRACE("Leaving, STATUS_SUCCESS\n");
    return STATUS_SUCCESS;

Quickie:
    TRACE("Leaving, Status = %lx\n", Status);
    IoStatus->Information = 0;
    IoStatus->Status = Status;
    return Status;
}

NTSTATUS
NTAPI
NpFsdCreateNamedPipe(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PFILE_OBJECT FileObject;
    PFILE_OBJECT RelatedFileObject;
    USHORT Disposition, ShareAccess;
    PEPROCESS Process;
    LIST_ENTRY DeferredList;
    UNICODE_STRING FileName;
    PNP_FCB Fcb;
    UNICODE_STRING Prefix;
    PNAMED_PIPE_CREATE_PARAMETERS Parameters;
    IO_STATUS_BLOCK IoStatus;
    TRACE("Entered\n");

    InitializeListHead(&DeferredList);
    Process = IoGetRequestorProcess(Irp);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    FileObject = IoStack->FileObject;
    RelatedFileObject = FileObject->RelatedFileObject;

    Disposition = (IoStack->Parameters.CreatePipe.Options >> 24) & 0xFF;
    ShareAccess = IoStack->Parameters.CreatePipe.ShareAccess & 0xFFFF;
    Parameters = IoStack->Parameters.CreatePipe.Parameters;

    FileName = FileObject->FileName;

    IoStatus.Information = 0;

    FsRtlEnterFileSystem();
    NpAcquireExclusiveVcb();

    if (RelatedFileObject)
    {
        Fcb = (PNP_FCB)((ULONG_PTR)RelatedFileObject->FsContext & ~1);
        if (!(Fcb) ||
            (Fcb->NodeType != NPFS_NTC_ROOT_DCB) ||
            (FileName.Length < sizeof(WCHAR)) ||
            (FileName.Buffer[0] == OBJ_NAME_PATH_SEPARATOR))
        {
            IoStatus.Status = STATUS_OBJECT_NAME_INVALID;
            goto Quickie;
        }

        IoStatus.Status = NpFindRelativePrefix(RelatedFileObject->FsContext,
                                               &FileName,
                                               TRUE,
                                               &Prefix,
                                               &Fcb);
        if (!NT_SUCCESS(IoStatus.Status))
        {
            goto Quickie;
        }
    }
    else
    {
        if (FileName.Length <= sizeof(OBJ_NAME_PATH_SEPARATOR) ||
            FileName.Buffer[0] != OBJ_NAME_PATH_SEPARATOR)
        {
            IoStatus.Status = STATUS_OBJECT_NAME_INVALID;
            goto Quickie;
        }

        Fcb = NpFindPrefix(&FileName, 1, &Prefix);
    }

    if (Prefix.Length)
    {
        if (Fcb->NodeType == NPFS_NTC_ROOT_DCB)
        {
            IoStatus.Status = NpCreateNewNamedPipe((PNP_DCB)Fcb,
                                                   FileObject,
                                                   FileName,
                                                   IoStack->Parameters.CreatePipe.
                                                   SecurityContext->DesiredAccess,
                                                   IoStack->Parameters.CreatePipe.
                                                   SecurityContext->AccessState,
                                                   Disposition,
                                                   ShareAccess,
                                                   Parameters,
                                                   Process,
                                                   &DeferredList,
                                                   &IoStatus);
            goto Quickie;
        }
        else
        {
            IoStatus.Status = STATUS_OBJECT_NAME_INVALID;
            goto Quickie;
        }
    }

    if (Fcb->NodeType != NPFS_NTC_FCB)
    {
        IoStatus.Status = STATUS_OBJECT_NAME_INVALID;
        goto Quickie;
    }

    IoStatus = NpCreateExistingNamedPipe(Fcb,
                                         FileObject,
                                         IoStack->Parameters.CreatePipe.
                                         SecurityContext->DesiredAccess,
                                         IoStack->Parameters.CreatePipe.
                                         SecurityContext->AccessState,
                                         IoStack->Flags &
                                         SL_FORCE_ACCESS_CHECK ?
                                         UserMode : Irp->RequestorMode,
                                         Disposition,
                                         ShareAccess,
                                         Parameters,
                                         Process,
                                         &DeferredList);

Quickie:
    NpReleaseVcb();
    NpCompleteDeferredIrps(&DeferredList);
    FsRtlExitFileSystem();

    TRACE("Leaving, IoStatus.Status = %lx\n", IoStatus.Status);
    Irp->IoStatus = IoStatus;
    IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    return IoStatus.Status;
}

/* EOF */
