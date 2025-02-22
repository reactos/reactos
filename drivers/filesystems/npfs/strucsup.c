/*
 * PROJECT:     ReactOS Named Pipe FileSystem
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/filesystems/npfs/strucsup.c
 * PURPOSE:
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "npfs.h"

// File ID number for NPFS bugchecking support
#define NPFS_BUGCHECK_FILE_ID   (NPFS_BUGCHECK_STRUCSUP)

/* GLOBALS ********************************************************************/

WCHAR NpRootDCBName[] = L"\\";
PNP_VCB NpVcb;

/* FUNCTIONS ******************************************************************/

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
    if (!Buffer) return FALSE;

    ObDereferenceObject(((PNP_EVENT_BUFFER)Buffer)->Event);
    return RtlDeleteElementGenericTable(Table, Buffer);
}

VOID
NTAPI
NpDeleteFcb(IN PNP_FCB Fcb,
            IN PLIST_ENTRY ListEntry)
{
    PNP_DCB Dcb;
    PAGED_CODE();

    Dcb = Fcb->ParentDcb;
    if (Fcb->CurrentInstances) NpBugCheck(0, 0, 0);

    NpCancelWaiter(&NpVcb->WaitQueue,
                   &Fcb->FullName,
                   STATUS_OBJECT_NAME_NOT_FOUND,
                   ListEntry);

    RemoveEntryList(&Fcb->DcbEntry);

    if (Fcb->SecurityDescriptor)
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
    if (Ccb->NodeType == NPFS_NTC_CCB)
    {
        RemoveEntryList(&Ccb->CcbEntry);
        --Ccb->Fcb->CurrentInstances;

        NpDeleteEventTableEntry(&NpVcb->EventTable,
                                Ccb->NonPagedCcb->EventBuffer[FILE_PIPE_CLIENT_END]);
        NpDeleteEventTableEntry(&NpVcb->EventTable,
                                Ccb->NonPagedCcb->EventBuffer[FILE_PIPE_SERVER_END]);
        NpUninitializeDataQueue(&Ccb->DataQueue[FILE_PIPE_INBOUND]);
        NpUninitializeDataQueue(&Ccb->DataQueue[FILE_PIPE_OUTBOUND]);
        NpCheckForNotify(Ccb->Fcb->ParentDcb, FALSE, ListEntry);
        ExDeleteResourceLite(&Ccb->NonPagedCcb->Lock);
        NpUninitializeSecurity(Ccb);
        if (Ccb->ClientSession)
        {
            ExFreePool(Ccb->ClientSession);
            Ccb->ClientSession = NULL;
        }
        ExFreePool(Ccb->NonPagedCcb);
    }
    else if (RootDcbCcb->NodeType == NPFS_NTC_ROOT_DCB_CCB && RootDcbCcb->Unknown)
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
NpCreateRootDcbCcb(IN PNP_ROOT_DCB_FCB *NewRootCcb)
{
    PNP_ROOT_DCB_FCB RootCcb;
    PAGED_CODE();

    RootCcb = ExAllocatePoolWithTag(PagedPool, sizeof(*RootCcb), NPFS_ROOT_DCB_CCB_TAG);
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
        NpBugCheck(0, 0, 0);
    }

    NpVcb->RootDcb = ExAllocatePoolWithTag(PagedPool, sizeof(*Dcb), NPFS_DCB_TAG);
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
    Dcb->FullName.Length = sizeof(NpRootDCBName) - sizeof(UNICODE_NULL);
    Dcb->FullName.MaximumLength = sizeof(NpRootDCBName);

    Dcb->ShortName.Length = Dcb->FullName.Length;
    Dcb->ShortName.MaximumLength = Dcb->FullName.MaximumLength;
    Dcb->ShortName.Buffer = Dcb->FullName.Buffer;

    if (!RtlInsertUnicodePrefix(&NpVcb->PrefixTable,
                                &Dcb->FullName,
                                &Dcb->PrefixTableEntry))
    {
        NpBugCheck(0, 0, 0);
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
    USHORT Length, MaximumLength;
    PAGED_CODE();

    Length = PipeName->Length;
    MaximumLength = Length + sizeof(UNICODE_NULL);

    if ((Length < sizeof(WCHAR)) || (MaximumLength < Length))
    {
        return STATUS_INVALID_PARAMETER;
    }

    RootPipe = FALSE;
    if (PipeName->Buffer[0] != OBJ_NAME_PATH_SEPARATOR)
    {
        Length += sizeof(OBJ_NAME_PATH_SEPARATOR);
        MaximumLength += sizeof(OBJ_NAME_PATH_SEPARATOR);
        RootPipe = TRUE;
        if (MaximumLength < sizeof(WCHAR))
        {
            return STATUS_INVALID_PARAMETER;
        }
    }

    Fcb = ExAllocatePoolWithTag(PagedPool, sizeof(*Fcb), NPFS_FCB_TAG);
    if (!Fcb) return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(Fcb, sizeof(*Fcb));
    Fcb->MaximumInstances = MaximumInstances;
    Fcb->Timeout = Timeout;
    Fcb->NodeType = NPFS_NTC_FCB;
    Fcb->ParentDcb = Dcb;
    InitializeListHead(&Fcb->CcbList);

    NameBuffer = ExAllocatePoolWithTag(PagedPool,
                                       MaximumLength,
                                       NPFS_NAME_BLOCK_TAG);
    if (!NameBuffer)
    {
        ExFreePool(Fcb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InsertTailList(&Dcb->FcbList, &Fcb->DcbEntry);

    if (RootPipe)
    {
        NameBuffer[0] = OBJ_NAME_PATH_SEPARATOR;
        RtlCopyMemory(NameBuffer + 1,
                      PipeName->Buffer,
                      PipeName->Length);
    }
    else
    {
        RtlCopyMemory(NameBuffer,
                      PipeName->Buffer,
                      PipeName->Length);
    }

    NameBuffer[Length / sizeof(WCHAR)] = UNICODE_NULL;

    Fcb->FullName.Length = Length;
    Fcb->FullName.MaximumLength = MaximumLength;
    Fcb->FullName.Buffer = NameBuffer;

    Fcb->ShortName.MaximumLength = Length;
    Fcb->ShortName.Length = Length - sizeof(OBJ_NAME_PATH_SEPARATOR);
    Fcb->ShortName.Buffer = NameBuffer + 1;

    if (!RtlInsertUnicodePrefix(&NpVcb->PrefixTable,
                                &Fcb->FullName,
                                &Fcb->PrefixTableEntry))
    {
        NpBugCheck(0, 0, 0);
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
            OUT PNP_CCB *NewCcb)
{
    PNP_CCB Ccb;
    PNP_NONPAGED_CCB CcbNonPaged;
    NTSTATUS Status;
    PAGED_CODE();

    Ccb = ExAllocatePoolWithTag(PagedPool, sizeof(*Ccb), NPFS_CCB_TAG);
    if (!Ccb) return STATUS_INSUFFICIENT_RESOURCES;

    CcbNonPaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(*CcbNonPaged), NPFS_CCB_TAG);
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
    Ccb->FileObject[FILE_PIPE_SERVER_END] = FileObject;
    Ccb->Fcb = Fcb;
    Ccb->NamedPipeState = State;
    Ccb->ReadMode[FILE_PIPE_SERVER_END] = ReadMode;
    Ccb->CompletionMode[FILE_PIPE_SERVER_END] = CompletionMode;

    Status = NpInitializeDataQueue(&Ccb->DataQueue[FILE_PIPE_INBOUND], InQuota);
    if (!NT_SUCCESS(Status))
    {
        ExFreePool(CcbNonPaged);
        ExFreePool(Ccb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = NpInitializeDataQueue(&Ccb->DataQueue[FILE_PIPE_OUTBOUND], OutQuota);
    if (!NT_SUCCESS(Status))
    {
        NpUninitializeDataQueue(&Ccb->DataQueue[FILE_PIPE_INBOUND]);
        ExFreePool(CcbNonPaged);
        ExFreePool(Ccb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InsertTailList(&Fcb->CcbList, &Ccb->CcbEntry);

    Fcb->CurrentInstances++;
    Fcb->ServerOpenCount++;
    InitializeListHead(&Ccb->IrpList);
    ExInitializeResourceLite(&Ccb->NonPagedCcb->Lock);
    *NewCcb = Ccb;
    return STATUS_SUCCESS;
}

/* EOF */
