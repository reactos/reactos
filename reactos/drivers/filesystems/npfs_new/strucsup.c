#include "npfs.h"

#define UNIMPLEMENTED

PWCHAR NpRootDCBName = L"\\";
PNP_VCB NpVcb;

RTL_GENERIC_COMPARE_RESULTS
NTAPI
NpEventTableCompareRoutine(IN PRTL_GENERIC_TABLE Table,
                           IN PVOID FirstStruct,
                           IN PVOID SecondStruct)
{
    UNIMPLEMENTED;
    return GenericEqual;
}

PVOID
NTAPI
NpEventTableAllocate(IN PRTL_GENERIC_TABLE Table,
                     IN CLONG ByteSize)
{
    UNIMPLEMENTED;
    return NULL;
}

VOID
NTAPI
NpEventTableDeallocate(IN PRTL_GENERIC_TABLE Table,
                       IN PVOID Buffer)
{
    UNIMPLEMENTED;
}

BOOLEAN
NTAPI
NpDeleteEventTableEntry(IN PRTL_GENERIC_TABLE Table,
                        IN PVOID Buffer)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
NTAPI
NpDeleteFcb(IN PNP_FCB Fcb,
            IN PLIST_ENTRY ListEntry)
{
    PNP_DCB Dcb;

    PAGED_CODE();

    Dcb = Fcb->ParentDcb;
    if (!Fcb->CurrentInstances) KeBugCheckEx(NPFS_FILE_SYSTEM, 0x17025F, 0, 0, 0);

    NpCancelWaiter(&NpVcb->WaitQueue,
                   &Fcb->FullName,
                   STATUS_OBJECT_NAME_NOT_FOUND,
                   ListEntry);
    
    RemoveEntryList(&Fcb->DcbEntry);

    if ( Fcb->SecurityDescriptor )
    {
        ObDereferenceSecurityDescriptor(Fcb->SecurityDescriptor, 1);
    }

    RtlRemoveUnicodePrefix(&NpVcb->PrefixTable, &Fcb->PrefixTableEntry);
    ExFreePool(Fcb->FullName.Buffer);
    ExFreePool(Fcb);
    NpCheckForNotify(Dcb, TRUE, ListEntry);
}

VOID
NTAPI
NpDeleteCcb(IN PNP_CCB Ccb,
            IN PLIST_ENTRY ListEntry)
{
    PNP_ROOT_DCB_FCB RootDcbCcb;
    PAGED_CODE();

    RootDcbCcb = (PNP_ROOT_DCB_FCB)Ccb;
    if ( Ccb->NodeType == NPFS_NTC_CCB )
    {
        RemoveEntryList(&Ccb->CcbEntry);
        --Ccb->Fcb->CurrentInstances;

        NpDeleteEventTableEntry(&NpVcb->EventTable,
                                Ccb->NonPagedCcb->EventBufferClient);
        NpDeleteEventTableEntry(&NpVcb->EventTable,
                                Ccb->NonPagedCcb->EventBufferServer);
        NpUninitializeDataQueue(&Ccb->InQueue);
        NpUninitializeDataQueue(&Ccb->OutQueue);
        NpCheckForNotify(Ccb->Fcb->ParentDcb, 0, ListEntry);
        ExDeleteResourceLite(&Ccb->NonPagedCcb->Lock);
        NpUninitializeSecurity(Ccb);
        if ( Ccb->ClientSession )
        {
            ExFreePool(Ccb->ClientSession);
            Ccb->ClientSession = 0;
        }
        ExFreePool(Ccb->NonPagedCcb);
    }
    else if ( RootDcbCcb->NodeType == NPFS_NTC_ROOT_DCB_CCB && RootDcbCcb->Unknown)
    {
        ExFreePool(RootDcbCcb->Unknown);
    }

    ExFreePool(Ccb);
}

VOID
NTAPI
NpInitializeVcb(VOID)
{
    PAGED_CODE();

    RtlZeroMemory(NpVcb, sizeof(*NpVcb));

    NpVcb->NodeType = NPFS_NTC_VCB;
    RtlInitializeUnicodePrefix(&NpVcb->PrefixTable);
    ExInitializeResourceLite(&NpVcb->Lock);
    RtlInitializeGenericTable(&NpVcb->EventTable,
                              NpEventTableCompareRoutine,
                              NpEventTableAllocate,
                              NpEventTableDeallocate,
                              0);
    NpInitializeWaitQueue(&NpVcb->WaitQueue);
}

NTSTATUS
NTAPI
NpCreateRootDcbCcb(IN PNP_ROOT_DCB_FCB* NewRootCcb)
{
    PNP_ROOT_DCB_FCB RootCcb;
    PAGED_CODE();

    RootCcb = ExAllocatePoolWithTag(PagedPool, sizeof(*RootCcb), 'CFpN');
    if (!RootCcb) return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(RootCcb, sizeof(*RootCcb));
    RootCcb->NodeType = NPFS_NTC_ROOT_DCB_CCB;
    *NewRootCcb = RootCcb;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpCreateRootDcb(VOID)
{
    PNP_DCB Dcb;
    PAGED_CODE();

    if (NpVcb->RootDcb)
    {
        KeBugCheckEx(0x25, 0x1700F3, 0, 0, 0);
    }

    NpVcb->RootDcb = ExAllocatePoolWithTag(PagedPool, sizeof(*Dcb), 'DFpN');
    if (!NpVcb->RootDcb)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Dcb = NpVcb->RootDcb;
    RtlZeroMemory(Dcb, sizeof(*Dcb));
    Dcb->NodeType = NPFS_NTC_ROOT_DCB;

    InitializeListHead(&Dcb->DcbEntry);
    InitializeListHead(&Dcb->NotifyList);
    InitializeListHead(&Dcb->NotifyList2);
    InitializeListHead(&Dcb->FcbList);

    Dcb->FullName.Buffer = NpRootDCBName;
    Dcb->FullName.Length = 2;
    Dcb->FullName.MaximumLength = 4;

    Dcb->ShortName.Length = Dcb->FullName.Length;
    Dcb->ShortName.MaximumLength = Dcb->FullName.MaximumLength;
    Dcb->ShortName.Buffer = Dcb->FullName.Buffer;

    if (!RtlInsertUnicodePrefix(&NpVcb->PrefixTable,
                                &Dcb->FullName,
                                &Dcb->PrefixTableEntry))
    {
        KeBugCheckEx(0x25u, 0x170128, 0, 0, 0);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpCreateFcb(IN PNP_DCB Dcb,
            IN PUNICODE_STRING PipeName,
            IN ULONG MaximumInstances,
            IN LARGE_INTEGER Timeout,
            IN USHORT NamedPipeConfiguration,
            IN USHORT NamedPipeType,
            OUT PNP_FCB *NewFcb)
{
    PNP_FCB Fcb;
    BOOLEAN RootPipe;
    PWCHAR NameBuffer;
    ULONG BufferOffset;
    USHORT PipeNameLength;
    PAGED_CODE();

    PipeNameLength = PipeName->Length;

    if ((PipeNameLength < sizeof(WCHAR)) ||
        ((PipeNameLength + sizeof(WCHAR)) < PipeNameLength) ||
        (PipeName->Buffer[0] != OBJ_NAME_PATH_SEPARATOR))
    {
        return STATUS_INVALID_PARAMETER;
    }

    RootPipe = FALSE;
    if (PipeNameLength == sizeof(WCHAR))
    {
        RootPipe = TRUE;
        PipeNameLength += sizeof(WCHAR);
    }

    Fcb = ExAllocatePoolWithTag(PagedPool, sizeof(*Fcb), 'FfpN');
    if (!Fcb) return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(Fcb, sizeof(*Fcb));
    Fcb->MaximumInstances = MaximumInstances;
    Fcb->Timeout = Timeout;
    Fcb->NodeType = NPFS_NTC_FCB;
    Fcb->ParentDcb = Dcb;
    InitializeListHead(&Fcb->CcbList);

    NameBuffer = ExAllocatePoolWithTag(PagedPool,
                                       PipeName->Length + (RootPipe ? 4 : 2),
                                       'nFpN');
    if (!NameBuffer)
    {
        ExFreePool(Fcb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InsertTailList(&Dcb->FcbList, &Fcb->DcbEntry);

    BufferOffset = 0;
    if (RootPipe)
    {
        *NameBuffer = OBJ_NAME_PATH_SEPARATOR;
        BufferOffset = 1;
    }

    RtlCopyMemory(NameBuffer + BufferOffset, PipeName->Buffer, PipeNameLength);
    NameBuffer[BufferOffset + (PipeNameLength / sizeof(WCHAR))] = UNICODE_NULL;

    Fcb->FullName.Length = PipeNameLength - sizeof(WCHAR);
    Fcb->FullName.MaximumLength = PipeNameLength;
    Fcb->FullName.Buffer = NameBuffer;

    Fcb->ShortName.MaximumLength = PipeNameLength - sizeof(WCHAR);
    Fcb->ShortName.Length = PipeNameLength - 2 * sizeof(WCHAR);
    Fcb->ShortName.Buffer = NameBuffer + 1;

    if (!RtlInsertUnicodePrefix(&NpVcb->PrefixTable,
                                &Fcb->FullName,
                                &Fcb->PrefixTableEntry))
    {
        KeBugCheckEx(NPFS_FILE_SYSTEM, 0x170222, 0, 0, 0);
    }

    Fcb->NamedPipeConfiguration = NamedPipeConfiguration;
    Fcb->NamedPipeType = NamedPipeType;
    *NewFcb = Fcb;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpCreateCcb(IN PNP_FCB Fcb,
            IN PFILE_OBJECT FileObject,
            IN UCHAR State,
            IN UCHAR ReadMode,
            IN UCHAR CompletionMode,
            IN ULONG InQuota,
            IN ULONG OutQuota,
            OUT PNP_CCB* NewCcb)
{
    PNP_CCB Ccb;
    PNP_NONPAGED_CCB CcbNonPaged;
    NTSTATUS Status;
    PAGED_CODE();

    Ccb = ExAllocatePoolWithTag(PagedPool, sizeof(*Ccb), 'cFpN');
    if (!Ccb) return STATUS_INSUFFICIENT_RESOURCES;

    CcbNonPaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(*CcbNonPaged), 'cFpN');
    if (!CcbNonPaged)
    {
        ExFreePool(Ccb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(CcbNonPaged, sizeof(*CcbNonPaged));
    CcbNonPaged->NodeType = NPFS_NTC_NONPAGED_CCB;

    RtlZeroMemory(Ccb, sizeof(*Ccb));
    Ccb->NodeType = NPFS_NTC_CCB;
    Ccb->NonPagedCcb = CcbNonPaged;
    Ccb->ServerFileObject = FileObject;
    Ccb->Fcb = Fcb;
    Ccb->NamedPipeState = State;
    Ccb->ServerReadMode = ReadMode;
    Ccb->ServerCompletionMode = CompletionMode;

    Status = NpInitializeDataQueue(&Ccb->InQueue, InQuota);
    if (!NT_SUCCESS(Status))
    {
        ExFreePool(CcbNonPaged);
        ExFreePool(Ccb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = NpInitializeDataQueue(&Ccb->OutQueue, InQuota);
    if (!NT_SUCCESS(Status))
    {
        NpUninitializeDataQueue(&Ccb->InQueue);
        ExFreePool(CcbNonPaged);
        ExFreePool(Ccb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InsertTailList(&Fcb->CcbList, &Ccb->CcbEntry);

    InitializeListHead(&Ccb->IrpList);
    ExInitializeResourceLite(&Ccb->NonPagedCcb->Lock);
    *NewCcb = Ccb;
    return STATUS_SUCCESS;
}

