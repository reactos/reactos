#include "npfs.h"

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

    NpSetFileObject(FileObject, NpVcb, NULL, FALSE);
    ++NpVcb->ReferenceCount;

    Status.Information = FILE_OPENED;
    Status.Status = STATUS_SUCCESS;
    return Status;
}

IO_STATUS_BLOCK
NTAPI
NpOpenNamedPipeRootDirectory(IN PNP_DCB Dcb,
                             IN PFILE_OBJECT FileObject,
                             IN ACCESS_MASK DesiredAccess,
                             IN PLIST_ENTRY List)
{
    IO_STATUS_BLOCK Status;
    PNP_ROOT_DCB_FCB Ccb;
    PAGED_CODE();

    Status.Status = NpCreateRootDcbCcb(&Ccb);
    if (NT_SUCCESS(Status.Status))
    {
        NpSetFileObject(FileObject, Dcb, Ccb, FALSE);
        ++Dcb->CurrentInstances;

        Status.Information = FILE_OPENED;
        Status.Status = STATUS_SUCCESS;
    }
    else
    {
        Status.Information = 0;
    }

    return Status;
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

    IoStatus.Status = STATUS_SUCCESS;
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

    if (((AccessGranted & FILE_READ_DATA) && (NamedPipeConfiguration == FILE_PIPE_INBOUND)) ||
        ((AccessGranted & FILE_WRITE_DATA) && (NamedPipeConfiguration == FILE_PIPE_OUTBOUND)))
    {
        IoStatus.Status = STATUS_ACCESS_DENIED;
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
        return IoStatus;
    }

    IoStatus.Status = NpInitializeSecurity(Ccb, SecurityQos, Thread);
    if (!NT_SUCCESS(IoStatus.Status)) return IoStatus;

    IoStatus.Status = NpSetConnectedPipeState(Ccb, FileObject, List);
    if (!NT_SUCCESS(IoStatus.Status))
    {
        NpUninitializeSecurity(Ccb);
        return IoStatus;
    }

    Ccb->ClientSession = NULL;
    Ccb->Process = IoThreadToProcess(Thread);

    IoStatus.Information = FILE_OPENED;
    IoStatus.Status = STATUS_SUCCESS;
    return IoStatus;
}

NTSTATUS
NTAPI
NpFsdCreate(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp)
{
    PEXTENDED_IO_STACK_LOCATION IoStack;
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

    InitializeListHead(&DeferredList);
    IoStack = (PEXTENDED_IO_STACK_LOCATION)IoGetCurrentIrpStackLocation(Irp);
    FileObject = IoStack->FileObject;
    RelatedFileObject = FileObject->RelatedFileObject;
    FileName = FileObject->FileName;
    DesiredAccess = IoStack->Parameters.CreatePipe.SecurityContext->DesiredAccess;

    FsRtlEnterFileSystem();
    ExAcquireResourceExclusiveLite(&NpVcb->Lock, TRUE);

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
            Irp->IoStatus = NpOpenNamedPipeRootDirectory(NpVcb->RootDcb,
                                                            FileObject,
                                                            DesiredAccess,
                                                            &DeferredList);
            goto Quickie;
        }
    }
    else if (!(RelatedFileObject) || (Type == NPFS_NTC_VCB))
    {
        Irp->IoStatus = NpOpenNamedPipeFileSystem(FileObject,
                                                  DesiredAccess);
        goto Quickie;
    }
    else if (Type == NPFS_NTC_ROOT_DCB)
    {
        Irp->IoStatus = NpOpenNamedPipeRootDirectory(NpVcb->RootDcb,
                                                     FileObject,
                                                     DesiredAccess,
                                                     &DeferredList);
        goto Quickie;
    }

    // Status = NpTranslateAlias(&FileName);; // ignore this for now
    // if (!NT_SUCCESS(Status)) goto Quickie;
    if (RelatedFileObject)
    {
        if (Type == NPFS_NTC_ROOT_DCB)
        {
            Dcb = (PNP_DCB)Ccb;
            Irp->IoStatus.Status = NpFindRelativePrefix(Dcb,
                                                        &FileName,
                                                        1,
                                                        &Prefix,
                                                        &Fcb);
            if (!NT_SUCCESS(Irp->IoStatus.Status))
            {
                goto Quickie;
            }
        }
        else if ((Type != NPFS_NTC_CCB) || (FileName.Length))
        {
            Irp->IoStatus.Status = STATUS_OBJECT_NAME_INVALID;
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
            Irp->IoStatus.Status = STATUS_OBJECT_NAME_INVALID;
            goto Quickie;
        }

        Fcb = NpFindPrefix(&FileName, TRUE, &Prefix);
    }

    if (Prefix.Length)
    {
        Irp->IoStatus.Status = Fcb->NodeType != NPFS_NTC_FCB ?
                                STATUS_OBJECT_NAME_NOT_FOUND :
                                STATUS_OBJECT_NAME_INVALID;
        goto Quickie;
    }

    if (Fcb->NodeType != NPFS_NTC_FCB)
    {
        Irp->IoStatus.Status = STATUS_OBJECT_NAME_INVALID;
        goto Quickie;
    }

    if (!Fcb->ServerOpenCount)
    {
        Irp->IoStatus.Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Quickie;
    }

    Irp->IoStatus = NpCreateClientEnd(Fcb,
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
    ExReleaseResourceLite(&NpVcb->Lock);
    NpCompleteDeferredIrps(&DeferredList);
    FsRtlExitFileSystem();

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Irp->IoStatus.Status;
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

    Privileges = NULL;

    NamedPipeConfiguration = Fcb->NamedPipeConfiguration;

    SubjectSecurityContext = &AccessState->SubjectSecurityContext;
    SeLockSubjectContext(SubjectSecurityContext);

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
        AccessState->RemainingDesiredAccess &= ~(GrantedAccess | 0x2000000);
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

    if (Fcb->CurrentInstances >= Fcb->MaximumInstances)
    {
        IoStatus.Status = STATUS_INSTANCE_NOT_AVAILABLE;
        return IoStatus;
    }

    if (Disposition == FILE_CREATE)
    {
        IoStatus.Status = STATUS_ACCESS_DENIED;
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
        return IoStatus;
    }

    NpSetFileObject(FileObject, Ccb, Ccb->NonPagedCcb, TRUE);
    Ccb->FileObject[FILE_PIPE_SERVER_END] = FileObject;
    NpCheckForNotify(Fcb->ParentDcb, 0, List);

    IoStatus.Status = STATUS_SUCCESS;
    IoStatus.Information = 1;
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
                     IN PIO_STATUS_BLOCK IoStatus)
{
    NTSTATUS Status;
    USHORT NamedPipeConfiguration;
    PSECURITY_SUBJECT_CONTEXT SecurityContext;
    PSECURITY_DESCRIPTOR SecurityDescriptor, CachedSecurityDescriptor;
    PNP_CCB Ccb;
    PNP_FCB Fcb;
    PAGED_CODE();

    if (!(Parameters->TimeoutSpecified) ||
        !(Parameters->MaximumInstances) ||
        (Parameters->DefaultTimeout.HighPart >= 0))
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

    if (!Parameters->NamedPipeType && Parameters->ReadMode == 1)
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
    SeLockSubjectContext(&AccessState->SubjectSecurityContext);

    Status = SeAssignSecurity(0,
                              AccessState->SecurityDescriptor,
                              &SecurityDescriptor,
                              0,
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
    ExFreePool(SecurityDescriptor);

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

    return STATUS_SUCCESS;

Quickie:
    IoStatus->Information = 0;
    IoStatus->Status = Status;
    return Status;
}

NTSTATUS
NTAPI
NpFsdCreateNamedPipe(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    PEXTENDED_IO_STACK_LOCATION IoStack;
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

    InitializeListHead(&DeferredList);
    Process = IoGetRequestorProcess(Irp);

    IoStack = (PEXTENDED_IO_STACK_LOCATION) IoGetCurrentIrpStackLocation(Irp);
    FileObject = IoStack->FileObject;
    RelatedFileObject = FileObject->RelatedFileObject;

    Disposition = (IoStack->Parameters.CreatePipe.Options >> 24) & 0xFF;
    ShareAccess = IoStack->Parameters.CreatePipe.ShareAccess & 0xFFFF;
    Parameters = IoStack->Parameters.CreatePipe.Parameters;

    FileName.Buffer = FileObject->FileName.Buffer;
    FileName.Length = FileObject->FileName.Length;
    FileName.MaximumLength = FileObject->FileName.MaximumLength;

    IoStatus.Status = STATUS_SUCCESS;
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

        Fcb = NpFindPrefix(&FileName, TRUE, &Prefix);
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

    Irp->IoStatus = IoStatus;
    IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    return IoStatus.Status;
}

