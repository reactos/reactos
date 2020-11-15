/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystems/mup/mup.c
 * PURPOSE:          Multi UNC Provider
 * PROGRAMMER:       Eric Kohl
 *                   Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "mup.h"

#define NDEBUG
#include <debug.h>

CODE_SEG("INIT")
NTSTATUS
NTAPI
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
);

CODE_SEG("INIT")
VOID
MupInitializeData(
    VOID
);

CODE_SEG("INIT")
VOID
MupInitializeVcb(
    PMUP_VCB Vcb
);

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, MupInitializeData)
#pragma alloc_text(INIT, MupInitializeVcb)
#endif

ERESOURCE MupGlobalLock;
ERESOURCE MupPrefixTableLock;
ERESOURCE MupCcbListLock;
ERESOURCE MupVcbLock;
LIST_ENTRY MupProviderList;
LIST_ENTRY MupPrefixList;
LIST_ENTRY MupMasterQueryList;
UNICODE_PREFIX_TABLE MupPrefixTable;
LARGE_INTEGER MupKnownPrefixTimeout;
ULONG MupProviderCount;
BOOLEAN MupOrderInitialized;
BOOLEAN MupEnableDfs;
PDEVICE_OBJECT mupDeviceObject;
NTSTATUS MupOrderedErrorList[] = { STATUS_UNSUCCESSFUL,
                                   STATUS_INVALID_PARAMETER,
                                   STATUS_REDIRECTOR_NOT_STARTED,
                                   STATUS_BAD_NETWORK_NAME,
                                   STATUS_BAD_NETWORK_PATH, 0 };

/* FUNCTIONS ****************************************************************/

CODE_SEG("INIT")
VOID
MupInitializeData(VOID)
{
    ExInitializeResourceLite(&MupGlobalLock);
    ExInitializeResourceLite(&MupPrefixTableLock);
    ExInitializeResourceLite(&MupCcbListLock);
    ExInitializeResourceLite(&MupVcbLock);
    MupProviderCount = 0;
    InitializeListHead(&MupProviderList);
    InitializeListHead(&MupPrefixList);
    InitializeListHead(&MupMasterQueryList);
    RtlInitializeUnicodePrefix(&MupPrefixTable);
    MupKnownPrefixTimeout.QuadPart = 9000000000LL;
    MupOrderInitialized = FALSE;
}

VOID
MupUninitializeData()
{
  ExDeleteResourceLite(&MupGlobalLock);
  ExDeleteResourceLite(&MupPrefixTableLock);
  ExDeleteResourceLite(&MupCcbListLock);
  ExDeleteResourceLite(&MupVcbLock);
}

CODE_SEG("INIT")
VOID
MupInitializeVcb(PMUP_VCB Vcb)
{
    RtlZeroMemory(Vcb, sizeof(MUP_VCB));
    Vcb->NodeType = NODE_TYPE_VCB;
    Vcb->NodeStatus = NODE_STATUS_HEALTHY;
    Vcb->NodeReferences = 1;
    Vcb->NodeSize = sizeof(MUP_VCB);
}

BOOLEAN
MuppIsDfsEnabled(VOID)
{
    HANDLE Key;
    ULONG Length;
    NTSTATUS Status;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    struct
    {
        KEY_VALUE_PARTIAL_INFORMATION KeyInfo;
        ULONG KeyValue;
    } KeyQueryOutput;

    /* You recognize FsRtlpIsDfsEnabled()! Congratz :-) */
    KeyName.Buffer = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Mup";
    KeyName.Length = sizeof(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Mup") - sizeof(UNICODE_NULL);
    KeyName.MaximumLength = sizeof(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Mup");

    /* Simply query registry to know whether we have to disable DFS.
     * Unless explicitly stated in registry, we will try to enable DFS.
     */
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&Key, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        return TRUE;
    }

    KeyName.Buffer = L"DisableDfs";
    KeyName.Length = sizeof(L"DisableDfs") - sizeof(UNICODE_NULL);
    KeyName.MaximumLength = sizeof(L"DisableDfs");

    Status = ZwQueryValueKey(Key, &KeyName, KeyValuePartialInformation, &KeyQueryOutput, sizeof(KeyQueryOutput), &Length);
    ZwClose(Key);
    if (!NT_SUCCESS(Status) || KeyQueryOutput.KeyInfo.Type != REG_DWORD)
    {
        return TRUE;
    }

    return ((ULONG_PTR)KeyQueryOutput.KeyInfo.Data != 1);
}

VOID
MupCalculateTimeout(PLARGE_INTEGER EntryTime)
{
    LARGE_INTEGER CurrentTime;

    /* Just get now + our allowed timeout */
    KeQuerySystemTime(&CurrentTime);
    EntryTime->QuadPart = CurrentTime.QuadPart + MupKnownPrefixTimeout.QuadPart;
}

PMUP_CCB
MupCreateCcb(VOID)
{
    PMUP_CCB Ccb;

    Ccb = ExAllocatePoolWithTag(PagedPool, sizeof(MUP_CCB), TAG_MUP);
    if (Ccb == NULL)
    {
        return NULL;
    }

    Ccb->NodeStatus = NODE_STATUS_HEALTHY;
    Ccb->NodeReferences = 1;
    Ccb->NodeType = NODE_TYPE_CCB;
    Ccb->NodeSize = sizeof(MUP_CCB);

    return Ccb;
}

PMUP_FCB
MupCreateFcb(VOID)
{
    PMUP_FCB Fcb;

    Fcb = ExAllocatePoolWithTag(PagedPool, sizeof(MUP_FCB), TAG_MUP);
    if (Fcb == NULL)
    {
        return NULL;
    }

    Fcb->NodeStatus = NODE_STATUS_HEALTHY;
    Fcb->NodeReferences = 1;
    Fcb->NodeType = NODE_TYPE_FCB;
    Fcb->NodeSize = sizeof(MUP_FCB);
    InitializeListHead(&Fcb->CcbList);

    return Fcb;
}

PMUP_MIC
MupAllocateMasterIoContext(VOID)
{
    PMUP_MIC MasterIoContext;

    MasterIoContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(MUP_MIC), TAG_MUP);
    if (MasterIoContext == NULL)
    {
        return NULL;
    }

    MasterIoContext->NodeStatus = NODE_STATUS_HEALTHY;
    MasterIoContext->NodeReferences = 1;
    MasterIoContext->NodeType = NODE_TYPE_MIC;
    MasterIoContext->NodeSize = sizeof(MUP_MIC);

    return MasterIoContext;
}

PMUP_UNC
MupAllocateUncProvider(ULONG RedirectorDeviceNameLength)
{
    PMUP_UNC UncProvider;

    UncProvider = ExAllocatePoolWithTag(PagedPool, sizeof(MUP_UNC) + RedirectorDeviceNameLength, TAG_MUP);
    if (UncProvider == NULL)
    {
        return NULL;
    }

    UncProvider->NodeReferences = 0;
    UncProvider->NodeType = NODE_TYPE_UNC;
    UncProvider->NodeStatus = NODE_STATUS_HEALTHY;
    UncProvider->NodeSize = sizeof(MUP_UNC) + RedirectorDeviceNameLength;
    UncProvider->Registered = FALSE;

    return UncProvider;
}

PMUP_PFX
MupAllocatePrefixEntry(ULONG PrefixLength)
{
    PMUP_PFX Prefix;
    ULONG PrefixSize;

    PrefixSize = sizeof(MUP_PFX) + PrefixLength;
    Prefix = ExAllocatePoolWithTag(PagedPool, PrefixSize, TAG_MUP);
    if (Prefix == NULL)
    {
        return NULL;
    }

    RtlZeroMemory(Prefix, PrefixSize);
    Prefix->NodeType = NODE_TYPE_PFX;
    Prefix->NodeStatus = NODE_STATUS_HEALTHY;
    Prefix->NodeReferences = 1;
    Prefix->NodeSize = PrefixSize;

    /* If we got a prefix, initialize the string */
    if (PrefixLength != 0)
    {
        Prefix->AcceptedPrefix.Buffer = (PVOID)((ULONG_PTR)Prefix + sizeof(MUP_PFX));
        Prefix->AcceptedPrefix.Length = PrefixLength;
        Prefix->AcceptedPrefix.MaximumLength = PrefixLength;
    }
    /* Otherwise, mark the fact that the allocation will be external */
    else
    {
        Prefix->ExternalAlloc = TRUE;
    }

    Prefix->KeepExtraRef = FALSE;
    /* Already init timeout to have one */
    MupCalculateTimeout(&Prefix->ValidityTimeout);

    return Prefix;
}

PMUP_MQC
MupAllocateMasterQueryContext(VOID)
{
    PMUP_MQC MasterQueryContext;

    MasterQueryContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(MUP_MQC), TAG_MUP);
    if (MasterQueryContext == NULL)
    {
        return NULL;
    }

    MasterQueryContext->NodeStatus = NODE_STATUS_HEALTHY;
    MasterQueryContext->NodeReferences = 1;
    MasterQueryContext->NodeType = NODE_TYPE_MQC;
    MasterQueryContext->NodeSize = sizeof(MUP_MQC);
    InitializeListHead(&MasterQueryContext->QueryPathList);
    InitializeListHead(&MasterQueryContext->MQCListEntry);
    ExInitializeResourceLite(&MasterQueryContext->QueryPathListLock);

    return MasterQueryContext;
}

ULONG
MupDecodeFileObject(PFILE_OBJECT FileObject, PMUP_FCB * pFcb, PMUP_CCB * pCcb)
{
    ULONG Type;
    PMUP_FCB Fcb;

    ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);
    *pFcb = FileObject->FsContext;
    *pCcb = FileObject->FsContext2;
    Fcb = *pFcb;

    if (Fcb == NULL)
    {
        ExReleaseResourceLite(&MupGlobalLock);
        return 0;
    }

    Type = Fcb->NodeType;
    if ((Type != NODE_TYPE_VCB && Type != NODE_TYPE_FCB) || (Fcb->NodeStatus != NODE_STATUS_HEALTHY && Fcb->NodeStatus != NODE_STATUS_CLEANUP))
    {
        *pFcb = 0;
        ExReleaseResourceLite(&MupGlobalLock);
        return 0;
    }

    ++Fcb->NodeReferences;
    ExReleaseResourceLite(&MupGlobalLock);

    return Type;
}

VOID
MupFreeNode(PVOID Node)
{
    ExFreePoolWithTag(Node, TAG_MUP);
}

VOID
MupDereferenceFcb(PMUP_FCB Fcb)
{
    /* Just dereference and delete if no references left */
    if (InterlockedDecrement(&Fcb->NodeReferences) == 0)
    {
        MupFreeNode(Fcb);
    }
}

VOID
MupDereferenceCcb(PMUP_CCB Ccb)
{
    /* Just dereference and delete (and clean) if no references left */
    if (InterlockedDecrement(&Ccb->NodeReferences) == 0)
    {
        ExAcquireResourceExclusiveLite(&MupCcbListLock, TRUE);
        RemoveEntryList(&Ccb->CcbListEntry);
        ExReleaseResourceLite(&MupCcbListLock);
        ObDereferenceObject(Ccb->FileObject);
        MupDereferenceFcb(Ccb->Fcb);
        MupFreeNode(Ccb);
    }
}

VOID
MupDereferenceVcb(PMUP_VCB Vcb)
{
    /* We cannot reach the point where no references are left */
    if (InterlockedDecrement(&Vcb->NodeReferences) == 0)
    {
        KeBugCheckEx(FILE_SYSTEM, 3, 0, 0, 0);
    }
}

NTSTATUS
MupDereferenceMasterIoContext(PMUP_MIC MasterIoContext,
                              PNTSTATUS NewStatus)
{
    PIRP Irp;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;

    Status = STATUS_SUCCESS;

    if (NewStatus != NULL)
    {
        /* Save last status, depending on whether it failed or not */
        if (!NT_SUCCESS(*NewStatus))
        {
            MasterIoContext->LastFailed = *NewStatus;
        }
        else
        {
            MasterIoContext->LastSuccess = STATUS_SUCCESS;
        }
    }

    if (InterlockedDecrement(&MasterIoContext->NodeReferences) != 0)
    {
        return STATUS_PENDING;
    }

    Irp = MasterIoContext->Irp;
    Stack = IoGetCurrentIrpStackLocation(Irp);
    if (Stack->MajorFunction == IRP_MJ_WRITE)
    {
        Irp->IoStatus.Information = Stack->Parameters.Write.Length;
    }
    else
    {
        Irp->IoStatus.Information = 0;
    }

    /* In case we never had any success (init is a failure), get the last failed status */
    if (!NT_SUCCESS(MasterIoContext->LastSuccess))
    {
        Status = MasterIoContext->LastFailed;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    MupDereferenceFcb(MasterIoContext->Fcb);
    MupFreeNode(MasterIoContext);

    return Status;
}

VOID
MupDereferenceUncProvider(PMUP_UNC UncProvider)
{
    InterlockedExchangeAdd(&UncProvider->NodeReferences, -1);
}

VOID
MupDereferenceKnownPrefix(PMUP_PFX Prefix)
{
    /* Just dereference and delete (and clean) if no references left */
    if (InterlockedDecrement(&Prefix->NodeReferences) == 0)
    {
        if (Prefix->InTable)
        {
            RtlRemoveUnicodePrefix(&MupPrefixTable, &Prefix->PrefixTableEntry);
            RemoveEntryList(&Prefix->PrefixListEntry);
        }

        if (Prefix->ExternalAlloc && Prefix->AcceptedPrefix.Buffer != NULL)
        {
            ExFreePoolWithTag(Prefix->AcceptedPrefix.Buffer, TAG_MUP);
        }

        if (Prefix->UncProvider)
        {
            MupDereferenceUncProvider(Prefix->UncProvider);
        }

        MupFreeNode(Prefix);
    }
}

VOID
MupRemoveKnownPrefixEntry(PMUP_PFX Prefix)
{
    RtlRemoveUnicodePrefix(&MupPrefixTable, &Prefix->PrefixTableEntry);
    RemoveEntryList(&Prefix->PrefixListEntry);
    Prefix->InTable = FALSE;
    MupDereferenceKnownPrefix(Prefix);
}

VOID
MupInvalidatePrefixTable(VOID)
{
    PMUP_PFX Prefix;
    PLIST_ENTRY Entry;

    /* Browse the prefix table */
    ExAcquireResourceExclusiveLite(&MupPrefixTableLock, TRUE);
    for (Entry = MupPrefixList.Flink;
         Entry != &MupPrefixList;
         Entry = Entry->Flink)
    {
        Prefix = CONTAINING_RECORD(Entry, MUP_PFX, PrefixListEntry);

        /* And remove any entry in it */
        if (Prefix->InTable)
        {
            MupRemoveKnownPrefixEntry(Prefix);
        }
    }
    ExReleaseResourceLite(&MupPrefixTableLock);
}

VOID
MupCleanupVcb(PDEVICE_OBJECT DeviceObject,
              PIRP Irp,
              PMUP_VCB Vcb)
{
    ExAcquireResourceExclusiveLite(&MupVcbLock, TRUE);

    /* Check we're not doing anything wrong first */
    if (Vcb->NodeStatus != NODE_STATUS_HEALTHY || Vcb->NodeType != NODE_TYPE_VCB)
    {
        ExRaiseStatus(STATUS_INVALID_HANDLE);
    }

    /* Remove share access */
    IoRemoveShareAccess(IoGetCurrentIrpStackLocation(Irp)->FileObject, &Vcb->ShareAccess);

    ExReleaseResourceLite(&MupVcbLock);
}

VOID
MupCleanupFcb(PDEVICE_OBJECT DeviceObject,
              PIRP Irp,
              PMUP_FCB Fcb)
{
    PLIST_ENTRY Entry;
    PMUP_CCB Ccb;

    ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);
    /* Check we're not doing anything wrong first */
    if (Fcb->NodeStatus != NODE_STATUS_HEALTHY || Fcb->NodeType != NODE_TYPE_FCB)
    {
        ExRaiseStatus(STATUS_INVALID_HANDLE);
    }
    Fcb->NodeStatus = NODE_STATUS_CLEANUP;
    ExReleaseResourceLite(&MupGlobalLock);

    /* Dereference any CCB associated with the FCB */
    ExAcquireResourceExclusiveLite(&MupCcbListLock, TRUE);
    for (Entry = Fcb->CcbList.Flink;
         Entry != &Fcb->CcbList;
         Entry = Entry->Flink)
    {
        Ccb = CONTAINING_RECORD(Entry, MUP_CCB, CcbListEntry);
        ExReleaseResourceLite(&MupCcbListLock);
        MupDereferenceCcb(Ccb);
        ExAcquireResourceExclusiveLite(&MupCcbListLock, TRUE);
    }
    ExReleaseResourceLite(&MupCcbListLock);
}

NTSTATUS
CommonForwardedIoCompletionRoutine(PDEVICE_OBJECT DeviceObject,
                                   PIRP Irp,
                                   PFORWARDED_IO_CONTEXT FwdCtxt)
{
    NTSTATUS Status;

    Status = Irp->IoStatus.Status;

    /* Just free everything we had allocated */
    if (Irp->MdlAddress != NULL)
    {
        MmUnlockPages(Irp->MdlAddress);
        IoFreeMdl(Irp->MdlAddress);
    }

    if (Irp->Flags & IRP_DEALLOCATE_BUFFER)
    {
        ExFreePoolWithTag(Irp->AssociatedIrp.SystemBuffer, TAG_MUP);
    }

    IoFreeIrp(Irp);

    /* Dereference the master context
     * The upper IRP will be completed once all the lower IRPs are done
     * (and thus, references count reaches 0)
     */
    MupDereferenceCcb(FwdCtxt->Ccb);
    MupDereferenceMasterIoContext(FwdCtxt->MasterIoContext, &Status);
    ExFreePoolWithTag(FwdCtxt, TAG_MUP);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
NTAPI
DeferredForwardedIoCompletionRoutine(PVOID Context)
{
    PFORWARDED_IO_CONTEXT FwdCtxt = (PFORWARDED_IO_CONTEXT)Context;

    CommonForwardedIoCompletionRoutine(FwdCtxt->DeviceObject, FwdCtxt->Irp, Context);
}

NTSTATUS
NTAPI
ForwardedIoCompletionRoutine(PDEVICE_OBJECT DeviceObject,
                             PIRP Irp,
                             PVOID Context)
{
    PFORWARDED_IO_CONTEXT FwdCtxt;

    /* If we're at DISPATCH_LEVEL, we cannot complete, defer completion */
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        CommonForwardedIoCompletionRoutine(DeviceObject, Irp, Context);
    }
    else
    {
        FwdCtxt = (PFORWARDED_IO_CONTEXT)Context;

        ExInitializeWorkItem(&FwdCtxt->WorkQueueItem, DeferredForwardedIoCompletionRoutine, Context);
        ExQueueWorkItem(&FwdCtxt->WorkQueueItem, CriticalWorkQueue);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
BuildAndSubmitIrp(PIRP Irp,
                  PMUP_CCB Ccb,
                  PMUP_MIC MasterIoContext)
{
    PMDL Mdl;
    PIRP LowerIrp;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;
    PDEVICE_OBJECT DeviceObject;
    PFORWARDED_IO_CONTEXT FwdCtxt;

    Status = STATUS_SUCCESS;
    LowerIrp = NULL;
    Mdl = NULL;

    /* Allocate a context for the completion routine */
    FwdCtxt = ExAllocatePoolWithTag(NonPagedPool, sizeof(FORWARDED_IO_CONTEXT), TAG_MUP);
    if (FwdCtxt == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    /* Init it */
    FwdCtxt->DeviceObject = NULL;
    FwdCtxt->Irp = NULL;

    /* Allocate the IRP */
    DeviceObject = IoGetRelatedDeviceObject(Ccb->FileObject);
    LowerIrp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
    if (LowerIrp == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    /* Initialize it */
    LowerIrp->Tail.Overlay.OriginalFileObject = Ccb->FileObject;
    LowerIrp->Tail.Overlay.Thread = Irp->Tail.Overlay.Thread;
    LowerIrp->RequestorMode = KernelMode;

    /* Copy the stack of the request we received to the IRP we'll pass below */
    Stack = IoGetNextIrpStackLocation(LowerIrp);
    RtlMoveMemory(Stack, IoGetCurrentIrpStackLocation(Irp), sizeof(IO_STACK_LOCATION));
    Stack->FileObject = Ccb->FileObject;
    /* Setup flags according to the FO */
    if (Ccb->FileObject->Flags & FO_WRITE_THROUGH)
    {
        Stack->Flags = SL_WRITE_THROUGH;
    }

    _SEH2_TRY
    {
        /* Does the device requires we do buffered IOs? */
        if (DeviceObject->Flags & DO_BUFFERED_IO)
        {
            LowerIrp->AssociatedIrp.SystemBuffer = NULL;

            if (Stack->Parameters.Write.Length == 0)
            {
                LowerIrp->Flags = IRP_BUFFERED_IO;
            }
            /* If we have data to pass */
            else
            {
                /* If it's coming from usermode, probe first */
                if (Irp->RequestorMode == UserMode)
                {
                    ProbeForRead(Irp->UserBuffer, Stack->Parameters.Write.Length, sizeof(UCHAR));
                }

                /* Allocate the buffer */
                LowerIrp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithQuotaTag(PagedPoolCacheAligned,
                                                                                  Stack->Parameters.Write.Length,
                                                                                  TAG_MUP);
                if (LowerIrp->AssociatedIrp.SystemBuffer == NULL)
                {
                    ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                }

                /* And copy input (remember, we've to free!) */
                RtlMoveMemory(LowerIrp->AssociatedIrp.SystemBuffer, Irp->UserBuffer, Stack->Parameters.Write.Length);
                LowerIrp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
            }
        }
        else
        {
            if (!(DeviceObject->Flags & DO_DIRECT_IO))
            {
                LowerIrp->UserBuffer = Irp->UserBuffer;
            }
            else
            {
                /* For direct IOs, allocate an MDL and pass it */
                if (Stack->Parameters.Write.Length != 0)
                {
                    Mdl = IoAllocateMdl(Irp->UserBuffer, Stack->Parameters.Write.Length, FALSE, TRUE, LowerIrp);
                    if (Mdl == NULL)
                    {
                        ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                    }

                    MmProbeAndLockPages(Mdl, Irp->RequestorMode, IoReadAccess);
                }
            }
        }

        /* Fix flags in the IRP */
        if (Ccb->FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING)
        {
            LowerIrp->Flags |= IRP_WRITE_OPERATION | IRP_NOCACHE;
        }
        else
        {
            LowerIrp->Flags |= IRP_WRITE_OPERATION;
        }

        FwdCtxt->Ccb = Ccb;
        FwdCtxt->MasterIoContext = MasterIoContext;

        ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);
        ++MasterIoContext->NodeReferences;
        ExReleaseResourceLite(&MupGlobalLock);

        /* Set out completion routine */
        IoSetCompletionRoutine(LowerIrp, ForwardedIoCompletionRoutine, FwdCtxt, TRUE, TRUE, TRUE);
        /* And call the device with our brand new IRP */
        Status = IoCallDriver(Ccb->DeviceObject, LowerIrp);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (FwdCtxt != NULL)
        {
            ExFreePoolWithTag(FwdCtxt, TAG_MUP);
        }

        if (LowerIrp != NULL)
        {
            if (LowerIrp->AssociatedIrp.SystemBuffer == NULL)
            {
                ExFreePoolWithTag(LowerIrp->AssociatedIrp.SystemBuffer, TAG_MUP);
            }

            IoFreeIrp(LowerIrp);
        }

        if (Mdl != NULL)
        {
            IoFreeMdl(Mdl);
        }
    }

    return Status;
}

NTSTATUS
NTAPI
MupForwardIoRequest(PDEVICE_OBJECT DeviceObject,
                    PIRP Irp)
{
    PMUP_FCB Fcb;
    PMUP_CCB Ccb;
    NTSTATUS Status;
    PLIST_ENTRY Entry;
    PMUP_CCB FcbListCcb;
    BOOLEAN CcbLockAcquired;
    PMUP_MIC MasterIoContext;
    PIO_STACK_LOCATION Stack;

    /* If DFS is enabled, check if that's for DFS and is so relay */
    if (MupEnableDfs && DeviceObject->DeviceType == FILE_DEVICE_DFS)
    {
        return DfsVolumePassThrough(DeviceObject, Irp);
    }

    Stack = IoGetCurrentIrpStackLocation(Irp);

    FsRtlEnterFileSystem();

    /* Write request is only possible for a mailslot, we need a FCB */
    MupDecodeFileObject(Stack->FileObject, &Fcb, &Ccb);
    if (Fcb == NULL || Fcb->NodeType != NODE_TYPE_FCB)
    {
        FsRtlExitFileSystem();
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto Complete;
    }

    /* Allocate a context */
    MasterIoContext = MupAllocateMasterIoContext();
    if (MasterIoContext == NULL)
    {
        FsRtlExitFileSystem();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Complete;
    }

    /* Mark the IRP pending and init the context */
    IoMarkIrpPending(Irp);
    MasterIoContext->Irp = Irp;
    /* Init with a failure to catch if we ever succeed */
    MasterIoContext->LastSuccess = STATUS_UNSUCCESSFUL;
    /* Init with the worth failure possible */
    MasterIoContext->LastFailed = STATUS_BAD_NETWORK_PATH;
    MasterIoContext->Fcb = Fcb;

    _SEH2_TRY
    {
        ExAcquireResourceExclusiveLite(&MupCcbListLock, TRUE);
        CcbLockAcquired = TRUE;

        /* For all the CCB (ie, the mailslots) we have */
        for (Entry = Fcb->CcbList.Flink;
             Entry != &Fcb->CcbList;
             Entry = Entry->Flink)
        {
            FcbListCcb = CONTAINING_RECORD(Entry, MUP_CCB, CcbListEntry);
            ExReleaseResourceLite(&MupCcbListLock);
            CcbLockAcquired = FALSE;

            ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);
            ++FcbListCcb->NodeReferences;
            ExReleaseResourceLite(&MupGlobalLock);

            /* Forward the write request */
            BuildAndSubmitIrp(Irp, FcbListCcb, MasterIoContext);
            ExAcquireResourceExclusiveLite(&MupCcbListLock, TRUE);
            CcbLockAcquired = TRUE;
        }

        ExReleaseResourceLite(&MupCcbListLock);
        CcbLockAcquired = FALSE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        if (CcbLockAcquired)
        {
            ExReleaseResourceLite(&MupCcbListLock);
        }
    }
    _SEH2_END;

    /* And done */
    MupDereferenceMasterIoContext(MasterIoContext, NULL);
    FsRtlExitFileSystem();

    return STATUS_PENDING;

Complete:
    /* Failure case */
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    return Status;
}

PMUP_UNC
AddUnregisteredProvider(PCWSTR DeviceName,
                        ULONG ProviderOrder)
{
    PMUP_UNC UncProvider;
    ULONG StrLen, NameLen;

    /* Just allocate the node */
    NameLen = wcslen(DeviceName);
    StrLen = NameLen * sizeof(WCHAR);
    UncProvider = MupAllocateUncProvider(StrLen);
    if (UncProvider == NULL)
    {
        return NULL;
    }

    /* And init it */
    UncProvider->DeviceName.MaximumLength = StrLen;
    UncProvider->DeviceName.Length = StrLen;
    UncProvider->DeviceName.Buffer = (PWSTR)((ULONG_PTR)UncProvider + sizeof(MUP_UNC));
    UncProvider->ProviderOrder = ProviderOrder;
    RtlMoveMemory(UncProvider->DeviceName.Buffer, DeviceName, StrLen);

    /* And add it to the global list
     * We're using tail here so that when called from registry init,
     * the providers with highest priority will be in the head of
     * the list
     */
    ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);
    InsertTailList(&MupProviderList, &UncProvider->ProviderListEntry);
    ExReleaseResourceLite(&MupGlobalLock);

    return UncProvider;
}

VOID
InitializeProvider(PCWSTR ProviderName,
                   ULONG ProviderOrder)
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    UNICODE_STRING Key, Value;
    PKEY_VALUE_FULL_INFORMATION Info;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG NameLen, StrLen, ResultLength;

    /* Get the information about the provider from registry */
    NameLen = wcslen(ProviderName);
    StrLen = NameLen * sizeof(WCHAR) + sizeof(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\") + sizeof(L"\\NetworkProvider");
    Key.Buffer = ExAllocatePoolWithTag(PagedPool, StrLen, TAG_MUP);
    if (Key.Buffer == NULL)
    {
        return;
    }

    RtlMoveMemory(Key.Buffer, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\", sizeof(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"));
    Key.MaximumLength = StrLen;
    Key.Length = sizeof(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\") - sizeof(UNICODE_NULL);
    RtlAppendUnicodeToString(&Key, ProviderName);
    RtlAppendUnicodeToString(&Key, L"\\NetworkProvider");

    InitializeObjectAttributes(&ObjectAttributes,
                               &Key,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(&KeyHandle, KEY_QUERY_VALUE, &ObjectAttributes);
    ExFreePoolWithTag(Key.Buffer, TAG_MUP);
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    RtlInitUnicodeString(&Value, L"DeviceName");
    Status = ZwQueryValueKey(KeyHandle, &Value, KeyValueFullInformation, NULL, 0, &ResultLength);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        Info = ExAllocatePoolWithTag(PagedPool, ResultLength + sizeof(UNICODE_NULL), TAG_MUP);
        if (Info == NULL)
        {
            ZwClose(KeyHandle);
            return;
        }

        Status = ZwQueryValueKey(KeyHandle, &Value, KeyValueFullInformation, Info, ResultLength, &ResultLength);
    }
    else
    {
        Info = NULL;
    }

    ZwClose(KeyHandle);

    /* And create the provider
     * It will remain unregistered until FsRTL receives a registration request and forwards
     * it to MUP
     */
    if (NT_SUCCESS(Status))
    {
        ASSERT(Info != NULL);
        AddUnregisteredProvider((PWSTR)((ULONG_PTR)Info + Info->DataOffset), ProviderOrder);
    }

    if (Info != NULL)
    {
        ExFreePoolWithTag(Info, TAG_MUP);
    }
}

VOID
MupGetProviderInformation(VOID)
{
    BOOLEAN End;
    NTSTATUS Status;
    HANDLE KeyHandle;
    PWSTR Providers, Coma;
    PKEY_VALUE_FULL_INFORMATION Info;
    ULONG ResultLength, ProviderCount;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING NetworkProvider, ProviderOrder;

    /* Open the registry to get the order of the providers */
    RtlInitUnicodeString(&NetworkProvider, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\NetworkProvider\\Order");
    InitializeObjectAttributes(&ObjectAttributes,
                               &NetworkProvider,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(&KeyHandle, KEY_QUERY_VALUE, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    RtlInitUnicodeString(&ProviderOrder, L"ProviderOrder");
    Status = ZwQueryValueKey(KeyHandle, &ProviderOrder, KeyValueFullInformation, NULL, 0, &ResultLength);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        Info = ExAllocatePoolWithTag(PagedPool, ResultLength + sizeof(UNICODE_NULL), TAG_MUP);
        if (Info == NULL)
        {
            ZwClose(KeyHandle);
            return;
        }

        Status = ZwQueryValueKey(KeyHandle, &ProviderOrder, KeyValueFullInformation, Info, ResultLength, &ResultLength);
    }
    else
    {
        Info = NULL;
    }

    ZwClose(KeyHandle);

    if (NT_SUCCESS(Status))
    {
        ASSERT(Info != NULL);

        Providers = (PWSTR)((ULONG_PTR)Info + Info->DataOffset);
        End = FALSE;
        ProviderCount = 0;

        /* For all the providers we got (coma-separated list), just create a provider node with the right order
         * The order is just the order of the list
         * First has highest priority (0) and then, get lower and lower priority
         * The highest number is the lowest priority
         */
        do
        {
            Coma = wcschr(Providers, L',');
            if (Coma != NULL)
            {
                *Coma = UNICODE_NULL;
            }
            else
            {
                End = TRUE;
            }

            InitializeProvider(Providers, ProviderCount);
            ++ProviderCount;

            Providers = Coma + 1;
        } while (!End);
    }

    if (Info != NULL)
    {
        ExFreePoolWithTag(Info, TAG_MUP);
    }
}

PMUP_UNC
MupCheckForUnregisteredProvider(PUNICODE_STRING RedirectorDeviceName)
{
    PLIST_ENTRY Entry;
    PMUP_UNC UncProvider;

    /* Browse the list of all the providers nodes we have */
    ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);
    for (Entry = MupProviderList.Flink; Entry != &MupProviderList; Entry = Entry->Flink)
    {
        UncProvider = CONTAINING_RECORD(Entry, MUP_UNC, ProviderListEntry);

        /* If one matches the device and is not registered, that's ours! */
        if (!UncProvider->Registered && RtlEqualUnicodeString(RedirectorDeviceName, &UncProvider->DeviceName, TRUE))
        {
            UncProvider->NodeStatus = NODE_STATUS_HEALTHY;
            break;
        }
    }

    if (Entry == &MupProviderList)
    {
        UncProvider = NULL;
    }
    ExReleaseResourceLite(&MupGlobalLock);

    return UncProvider;
}

NTSTATUS
RegisterUncProvider(PDEVICE_OBJECT DeviceObject,
                    PIRP Irp)

{
    BOOLEAN New;
    PMUP_FCB Fcb;
    PMUP_CCB Ccb;
    NTSTATUS Status;
    PLIST_ENTRY Entry;
    PIO_STACK_LOCATION Stack;
    IO_STATUS_BLOCK IoStatusBlock;
    PMUP_UNC UncProvider, ListEntry;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING RedirectorDeviceName;
    OBJECT_HANDLE_INFORMATION HandleInfo;
    PMUP_PROVIDER_REGISTRATION_INFO RegInfo;

    DPRINT1("RegisterUncProvider(%p, %p)\n", DeviceObject, Irp);
    New = FALSE;

    /* Check whether providers order was already initialized */
    ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);
    if (MupOrderInitialized)
    {
        ExReleaseResourceLite(&MupGlobalLock);
    }
    else
    {
        /* They weren't, so do it */
        MupOrderInitialized = TRUE;
        ExReleaseResourceLite(&MupGlobalLock);
        MupGetProviderInformation();
    }

    Stack = IoGetCurrentIrpStackLocation(Irp);

    /* This can only happen with a volume open */
    if (MupDecodeFileObject(Stack->FileObject, &Fcb, &Ccb) != NODE_TYPE_VCB)
    {
        Irp->IoStatus.Status = STATUS_INVALID_HANDLE;
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);

        return STATUS_INVALID_HANDLE;
    }

    /* Get the registration information */
    RegInfo = (PMUP_PROVIDER_REGISTRATION_INFO)Irp->AssociatedIrp.SystemBuffer;
    _SEH2_TRY
    {
        RedirectorDeviceName.Length = RegInfo->RedirectorDeviceNameLength;
        RedirectorDeviceName.MaximumLength = RedirectorDeviceName.Length;
        RedirectorDeviceName.Buffer = (PWSTR)((ULONG_PTR)RegInfo + RegInfo->RedirectorDeviceNameOffset);

        /* Have we got already a node for it? (Like from previous init) */
        UncProvider = MupCheckForUnregisteredProvider(&RedirectorDeviceName);
        if (UncProvider == NULL)
        {
            /* If we don't, allocate a new one */
            New = TRUE;
            UncProvider = MupAllocateUncProvider(RegInfo->RedirectorDeviceNameLength);
            if (UncProvider == NULL)
            {
                Status = STATUS_INVALID_USER_BUFFER;
                _SEH2_LEAVE;
            }

            /* Set it up */
            UncProvider->DeviceName.Length = RedirectorDeviceName.Length;
            UncProvider->DeviceName.MaximumLength = RedirectorDeviceName.MaximumLength;
            UncProvider->DeviceName.Buffer = (PWSTR)((ULONG_PTR)UncProvider + sizeof(MUP_UNC));

            /* As it wasn't in registry for order, give the lowest priority possible */
            UncProvider->ProviderOrder = MAXLONG;
            RtlMoveMemory(UncProvider->DeviceName.Buffer, (PWSTR)((ULONG_PTR)RegInfo + RegInfo->RedirectorDeviceNameOffset), RegInfo->RedirectorDeviceNameLength);
        }

        /* Continue registration */
        UncProvider->MailslotsSupported = RegInfo->MailslotsSupported;
        ++UncProvider->NodeReferences;

        /* Open a handle to the device */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &UncProvider->DeviceName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = NtOpenFile(&UncProvider->DeviceHandle,
                            FILE_TRAVERSE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_DIRECTORY_FILE);
        if (NT_SUCCESS(Status))
        {
            Status = IoStatusBlock.Status;
        }

        /* And return the provider (as CCB) */
        if (NT_SUCCESS(Status))
        {
            Stack->FileObject->FsContext2 = UncProvider;
            Status = ObReferenceObjectByHandle(UncProvider->DeviceHandle, 0, NULL, KernelMode, (PVOID *)&UncProvider->FileObject, &HandleInfo);
            if (!NT_SUCCESS(Status))
            {
                NtClose(UncProvider->DeviceHandle);
            }
        }

        if (!NT_SUCCESS(Status))
        {
            MupDereferenceUncProvider(UncProvider);
        }
        else
        {
            UncProvider->DeviceObject = IoGetRelatedDeviceObject(UncProvider->FileObject);

            /* Now, insert the provider in our global list
             * They are sorted by order
             */
            ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);
            ++MupProviderCount;
            if (New)
            {
                for (Entry = MupProviderList.Flink; Entry != &MupProviderList; Entry = Entry->Flink)
                {
                    ListEntry = CONTAINING_RECORD(Entry, MUP_UNC, ProviderListEntry);

                    if (UncProvider->ProviderOrder < ListEntry->ProviderOrder)
                    {
                        break;
                    }
                }

                InsertTailList(Entry, &UncProvider->ProviderListEntry);
            }
            UncProvider->Registered = TRUE;
            ExReleaseResourceLite(&MupGlobalLock);
            Status = STATUS_SUCCESS;

            DPRINT1("UNC provider %wZ registered\n", &UncProvider->DeviceName);
        }
    }
    _SEH2_FINALLY
    {
        if (_SEH2_AbnormalTermination())
        {
            Status = STATUS_INVALID_USER_BUFFER;
        }

        MupDereferenceVcb((PMUP_VCB)Fcb);

        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    }
    _SEH2_END;

    return Status;
}

NTSTATUS
NTAPI
MupFsControl(PDEVICE_OBJECT DeviceObject,
             PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;

    Stack = IoGetCurrentIrpStackLocation(Irp);

    _SEH2_TRY
    {
        /* MUP only understands a single FSCTL code: registering UNC provider */
        if (Stack->Parameters.FileSystemControl.FsControlCode == FSCTL_MUP_REGISTER_PROVIDER)
        {
            /* It obviously has to come from a driver/kernelmode thread */
            if (Irp->RequestorMode == UserMode)
            {
                Status = STATUS_ACCESS_DENIED;

                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);

                _SEH2_LEAVE;
            }

            Status = RegisterUncProvider(DeviceObject, Irp);
        }
        else
        {
            /* If that's an unknown FSCTL code, maybe it's for DFS, pass it */
            if (!MupEnableDfs)
            {
                Status = STATUS_INVALID_PARAMETER;

                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);

                _SEH2_LEAVE;
            }

            Status = DfsFsdFileSystemControl(DeviceObject, Irp);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}

VOID
MupSetFileObject(PFILE_OBJECT FileObject,
                 PMUP_FCB Fcb,
                 PMUP_CCB Ccb)
{
    FileObject->FsContext = Fcb;
    FileObject->FsContext2 = Ccb;
}

NTSTATUS
MupRerouteOpen(PFILE_OBJECT FileObject,
               PMUP_UNC UncProvider)
{
    PWSTR FullPath;
    ULONG TotalLength;

    DPRINT1("Rerouting %wZ with %wZ\n", &FileObject->FileName, &UncProvider->DeviceName);

    /* Get the full path name (device name first, and requested file name appended) */
    TotalLength = UncProvider->DeviceName.Length + FileObject->FileName.Length;
    if (TotalLength > MAXUSHORT)
    {
        return STATUS_NAME_TOO_LONG;
    }

    /* Allocate a buffer big enough */
    FullPath = ExAllocatePoolWithTag(PagedPool, TotalLength, TAG_MUP);
    if (FullPath == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Create the full path */
    RtlMoveMemory(FullPath, UncProvider->DeviceName.Buffer, UncProvider->DeviceName.Length);
    RtlMoveMemory((PWSTR)((ULONG_PTR)FullPath + UncProvider->DeviceName.Length), FileObject->FileName.Buffer, FileObject->FileName.Length);

    /* And redo the path in the file object */
    ExFreePoolWithTag(FileObject->FileName.Buffer, 0);
    FileObject->FileName.Buffer = FullPath;
    FileObject->FileName.MaximumLength = TotalLength;
    FileObject->FileName.Length = FileObject->FileName.MaximumLength;

    /* Ob, please reparse to open the correct file at the right place, thanks! :-) */
    return STATUS_REPARSE;
}

NTSTATUS
BroadcastOpen(PIRP Irp)
{
    PMUP_FCB Fcb;
    HANDLE Handle;
    PLIST_ENTRY Entry;
    PMUP_CCB Ccb = NULL;
    PMUP_UNC UncProvider;
    UNICODE_STRING FullPath;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status, LastFailed;
    ULONG TotalLength, LastOrder;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    OBJECT_HANDLE_INFORMATION HandleInfo;
    BOOLEAN Locked, Referenced, CcbInitialized;

    Fcb = MupCreateFcb();
    if (Fcb == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Stack = IoGetCurrentIrpStackLocation(Irp);
    FileObject = Stack->FileObject;
    Locked = FALSE;
    Referenced = FALSE;
    CcbInitialized = FALSE;
    LastFailed = STATUS_NO_SUCH_FILE;
    LastOrder = (ULONG)-1;

    _SEH2_TRY
    {
        ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);
        Locked = TRUE;

        /* Associate our FCB with the FO */
        MupSetFileObject(FileObject, Fcb, NULL);
        Fcb->FileObject = FileObject;

        /* Now, broadcast the open to any UNC provider that supports mailslots */
        for (Entry = MupProviderList.Flink; Entry != &MupProviderList; Entry = Entry->Flink)
        {
            UncProvider = CONTAINING_RECORD(Entry, MUP_UNC, ProviderListEntry);
            ++UncProvider->NodeReferences;
            Referenced = TRUE;

            ExReleaseResourceLite(&MupGlobalLock);
            Locked = FALSE;

            TotalLength = UncProvider->DeviceName.Length + FileObject->FileName.Length;
            if (UncProvider->MailslotsSupported && TotalLength <= MAXUSHORT)
            {
                /* Provide the correct name for the mailslot (ie, happened the device name of the provider) */
                FullPath.Buffer = ExAllocatePoolWithTag(PagedPool, TotalLength, TAG_MUP);
                if (FullPath.Buffer == NULL)
                {
                    ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                }

                FullPath.Length = TotalLength;
                FullPath.MaximumLength = TotalLength;
                RtlMoveMemory(FullPath.Buffer, UncProvider->DeviceName.Buffer, UncProvider->DeviceName.Length);
                RtlMoveMemory((PWSTR)((ULONG_PTR)FullPath.Buffer + UncProvider->DeviceName.Length),
                              FileObject->FileName.Buffer,
                              FileObject->FileName.Length);

                /* And just forward the creation request */
                InitializeObjectAttributes(&ObjectAttributes,
                                           &FullPath,
                                           OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                           NULL,
                                           NULL);
                Status = IoCreateFile(&Handle,
                                      Stack->Parameters.Create.SecurityContext->DesiredAccess & FILE_SIMPLE_RIGHTS_MASK,
                                      &ObjectAttributes,
                                      &IoStatusBlock,
                                      NULL,
                                      Stack->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_VALID_FLAGS,
                                      Stack->Parameters.Create.ShareAccess & FILE_SHARE_VALID_FLAGS,
                                      FILE_OPEN,
                                      Stack->Parameters.Create.Options & FILE_VALID_SET_FLAGS,
                                      NULL,
                                      0,
                                      CreateFileTypeNone,
                                      NULL,
                                      IO_NO_PARAMETER_CHECKING);

                ExFreePoolWithTag(FullPath.Buffer, TAG_MUP);

                /* If opening succeed */
                if (NT_SUCCESS(Status))
                {
                    Status = IoStatusBlock.Status;

                    /* Create a CCB */
                    Ccb = MupCreateCcb();
                    if (Ccb == NULL)
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }

                    /* And associated a FO to it */
                    if (NT_SUCCESS(Status))
                    {
                        Status = ObReferenceObjectByHandle(Handle, 0, 0, 0, (PVOID *)&Ccb->FileObject, &HandleInfo);
                        ZwClose(Handle);
                    }
                }

                /* If we failed, remember the last failed status of the higher priority provider */
                if (!NT_SUCCESS(Status))
                {
                    if (UncProvider->ProviderOrder <= LastOrder)
                    {
                        LastOrder = UncProvider->ProviderOrder;
                        LastFailed = Status;
                    }
                }
                /* Otherwise, properly attach our CCB to the mailslot */
                else
                {
                    Ccb->DeviceObject = IoGetRelatedDeviceObject(Ccb->FileObject);
                    Ccb->Fcb = Fcb;

                    ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);
                    Locked = TRUE;
                    ++Fcb->NodeReferences;
                    ExReleaseResourceLite(&MupGlobalLock);
                    Locked = FALSE;
                    CcbInitialized = TRUE;

                    InsertTailList(&Fcb->CcbList, &Ccb->CcbListEntry);
                }
            }

            ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);
            Locked = TRUE;
            MupDereferenceUncProvider(UncProvider);
            Referenced = FALSE;
        }

        ExReleaseResourceLite(&MupGlobalLock);
        Locked = FALSE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* If we at least opened one mailslot, return success */
    Status = (CcbInitialized ? STATUS_SUCCESS : LastFailed);

    if (Referenced)
    {
        MupDereferenceUncProvider(UncProvider);
    }

    if (Locked)
    {
        ExReleaseResourceLite(&MupGlobalLock);
    }

    /* In case of failure, don't leak CCB */
    if (!NT_SUCCESS(Status) && Ccb != NULL)
    {
        MupFreeNode(Ccb);
    }

    return Status;
}

PIRP
MupBuildIoControlRequest(PFILE_OBJECT FileObject,
                         PVOID Context,
                         ULONG MajorFunction,
                         ULONG IoctlCode,
                         PVOID InputBuffer,
                         ULONG InputBufferSize,
                         PVOID OutputBuffer,
                         ULONG OutputBufferSize,
                         PIO_COMPLETION_ROUTINE CompletionRoutine)
{
    PIRP Irp;
    PIO_STACK_LOCATION Stack;
    PDEVICE_OBJECT DeviceObject;

    if (InputBuffer == NULL)
    {
        return NULL;
    }

    /* Get the device object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    /* Allocate the IRP (with one more location for us */
    Irp = IoAllocateIrp(DeviceObject->StackSize + 1, FALSE);
    if (Irp == NULL)
    {
        return NULL;
    }

    /* Skip our location */
    IoSetNextIrpStackLocation(Irp);
    /* Setup the IRP */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    IoSetCompletionRoutine(Irp, CompletionRoutine, Context, TRUE, TRUE, TRUE);

    /* Setup the stack */
    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MajorFunction = MajorFunction;
    Stack->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferSize;
    Stack->Parameters.DeviceIoControl.InputBufferLength = InputBufferSize;
    Stack->Parameters.DeviceIoControl.IoControlCode = IoctlCode;
    Stack->MinorFunction = 0;
    Stack->FileObject = FileObject;
    Stack->DeviceObject = DeviceObject;

    switch (IO_METHOD_FROM_CTL_CODE(IoctlCode))
    {
        case METHOD_BUFFERED:
            /* If it's buffered, just pass the buffers we got */
            Irp->MdlAddress = NULL;
            Irp->AssociatedIrp.SystemBuffer = InputBuffer;
            Irp->UserBuffer = OutputBuffer;
            Irp->Flags = IRP_BUFFERED_IO;

            if (OutputBuffer != NULL)
            {
                Irp->Flags |= IRP_INPUT_OPERATION;
            }
            break;

        case METHOD_IN_DIRECT:
        case METHOD_OUT_DIRECT:
            /* Otherwise, allocate an MDL */
            if (IoAllocateMdl(InputBuffer, InputBufferSize, FALSE, FALSE, Irp) == NULL)
            {
                IoFreeIrp(Irp);
                return NULL;
            }

            Irp->AssociatedIrp.SystemBuffer = InputBuffer;
            Irp->Flags = IRP_BUFFERED_IO;
            MmProbeAndLockPages(Irp->MdlAddress, KernelMode, IoReadAccess);
            break;

        case METHOD_NEITHER:
            /* Or pass the buffers */
            Irp->UserBuffer = OutputBuffer;
            Irp->MdlAddress = NULL;
            Irp->AssociatedIrp.SystemBuffer = NULL;
            Stack->Parameters.DeviceIoControl.Type3InputBuffer = InputBuffer;
            break;
    }

    return Irp;
}

VOID
MupFreeMasterQueryContext(PMUP_MQC MasterQueryContext)
{
    ExDeleteResourceLite(&MasterQueryContext->QueryPathListLock);
    ExFreePoolWithTag(MasterQueryContext, TAG_MUP);
}

NTSTATUS
MupDereferenceMasterQueryContext(PMUP_MQC MasterQueryContext)
{
    LONG References;
    NTSTATUS Status;
    BOOLEAN KeepExtraRef;

    ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);
    --MasterQueryContext->NodeReferences;
    References = MasterQueryContext->NodeReferences;
    ExReleaseResourceLite(&MupGlobalLock);

    if (References != 0)
    {
        DPRINT("Still having refs (%ld)\n", References);
        return STATUS_PENDING;
    }

    /* We HAVE an IRP to complete. It cannot be NULL
     * Please, help preserving kittens, don't provide NULL IRPs.
     */
    if (MasterQueryContext->Irp == NULL)
    {
        KeBugCheck(FILE_SYSTEM);
    }

    ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);
    RemoveEntryList(&MasterQueryContext->MQCListEntry);
    ExReleaseResourceLite(&MupGlobalLock);

    ExAcquireResourceExclusiveLite(&MupPrefixTableLock, TRUE);
    KeepExtraRef = MasterQueryContext->Prefix->KeepExtraRef;
    MupDereferenceKnownPrefix(MasterQueryContext->Prefix);

    /* We found a provider? */
    if (MasterQueryContext->LatestProvider != NULL)
    {
        /* With a successful status? */
        if (MasterQueryContext->LatestStatus == STATUS_SUCCESS)
        {
            /* Then, it's time to reroute, someone accepted to handle the file creation request! */
            if (!KeepExtraRef)
            {
                MupDereferenceKnownPrefix(MasterQueryContext->Prefix);
            }

            ExReleaseResourceLite(&MupPrefixTableLock);
            /* Reroute & complete :-) */
            Status = MupRerouteOpen(MasterQueryContext->FileObject, MasterQueryContext->LatestProvider);
            goto Complete;
        }
        else
        {
            MupDereferenceUncProvider(MasterQueryContext->LatestProvider);
        }
    }

    MupDereferenceKnownPrefix(MasterQueryContext->Prefix);
    ExReleaseResourceLite(&MupPrefixTableLock);

    /* Return the highest failed status we had */
    Status = MasterQueryContext->LatestStatus;

Complete:
    /* In finally, complete the IRP for real! */
    MasterQueryContext->Irp->IoStatus.Status = Status;
    IoCompleteRequest(MasterQueryContext->Irp, IO_DISK_INCREMENT);

    MasterQueryContext->Irp = NULL;
    MupFreeMasterQueryContext(MasterQueryContext);

    return Status;
}

NTSTATUS
NTAPI
QueryPathCompletionRoutine(PDEVICE_OBJECT DeviceObject,
                           PIRP Irp,
                           PVOID Context)
{
    PMUP_PFX Prefix;
    ULONG LatestPos, Pos;
    PWSTR AcceptedPrefix;
    PMUP_MQC MasterQueryContext;
    NTSTATUS Status, TableStatus;
    PQUERY_PATH_CONTEXT QueryContext;
    PQUERY_PATH_RESPONSE QueryResponse;

    /* Get all the data from our query to the provider */
    QueryContext = (PQUERY_PATH_CONTEXT)Context;
    QueryResponse = (PQUERY_PATH_RESPONSE)QueryContext->QueryPathRequest;
    MasterQueryContext = QueryContext->MasterQueryContext;
    Status = Irp->IoStatus.Status;

    DPRINT("Reply from %wZ: %u (Status: %lx)\n", &QueryContext->UncProvider->DeviceName, QueryResponse->LengthAccepted, Status);

    ExAcquireResourceExclusiveLite(&MasterQueryContext->QueryPathListLock, TRUE);
    RemoveEntryList(&QueryContext->QueryPathListEntry);

    /* If the driver returned a success, and an acceptance length */
    if (NT_SUCCESS(Status) && QueryResponse->LengthAccepted > 0)
    {
        Prefix = MasterQueryContext->Prefix;

        /* Check if we already found a provider from a previous iteration */
        if (MasterQueryContext->LatestProvider != NULL)
        {
           /* If the current provider has a lower priority (ie, a greater order), then, bailout and keep previous one */
            if (QueryContext->UncProvider->ProviderOrder >= MasterQueryContext->LatestProvider->ProviderOrder)
            {
                MupDereferenceUncProvider(QueryContext->UncProvider);
                goto Cleanup;
            }

            /* Otherwise, if the prefix was in the prefix table, just drop it:
             * we have a provider which supersedes the accepted prefix, so leave
             * room for the new prefix/provider
             */
            ExAcquireResourceExclusiveLite(&MupPrefixTableLock, TRUE);
            if (Prefix->InTable)
            {
                RtlRemoveUnicodePrefix(&MupPrefixTable, &Prefix->PrefixTableEntry);
                RemoveEntryList(&Prefix->PrefixListEntry);
                Prefix->InTable = FALSE;
            }
            ExReleaseResourceLite(&MupPrefixTableLock);

            Prefix->KeepExtraRef = FALSE;

            /* Release data associated with the current prefix, if any
             * We'll renew them with the new accepted prefix
             */
            if (Prefix->AcceptedPrefix.Length != 0 && Prefix->AcceptedPrefix.Buffer != NULL)
            {
                ExFreePoolWithTag(Prefix->AcceptedPrefix.Buffer, TAG_MUP);
                Prefix->AcceptedPrefix.MaximumLength = 0;
                Prefix->AcceptedPrefix.Length = 0;
                Prefix->AcceptedPrefix.Buffer = NULL;
                Prefix->ExternalAlloc = FALSE;
            }

            /* If there was also a provider, drop it, the new one
             * is different
             */
            if (Prefix->UncProvider != NULL)
            {
                MupDereferenceUncProvider(Prefix->UncProvider);
                Prefix->UncProvider = NULL;
            }
        }

        /* Now, set our information about the provider that accepted the prefix */
        MasterQueryContext->LatestProvider = QueryContext->UncProvider;
        MasterQueryContext->LatestStatus = Status;

        if (MasterQueryContext->FileObject->FsContext2 != (PVOID)DFS_DOWNLEVEL_OPEN_CONTEXT)
        {
            /* Allocate a buffer for the prefix */
            AcceptedPrefix = ExAllocatePoolWithTag(PagedPool, QueryResponse->LengthAccepted, TAG_MUP);
            if (AcceptedPrefix == NULL)
            {
                Prefix->InTable = FALSE;
            }
            else
            {
                /* Set it up to the accepted length */
                RtlMoveMemory(AcceptedPrefix, MasterQueryContext->FileObject->FileName.Buffer, QueryResponse->LengthAccepted);
                Prefix->UncProvider = MasterQueryContext->LatestProvider;
                Prefix->AcceptedPrefix.Buffer = AcceptedPrefix;
                Prefix->AcceptedPrefix.Length = QueryResponse->LengthAccepted;
                Prefix->AcceptedPrefix.MaximumLength = QueryResponse->LengthAccepted;
                Prefix->ExternalAlloc = TRUE;

                /* Insert the accepted prefix in the table of known prefixes */
                DPRINT1("%wZ accepted %wZ\n", &Prefix->UncProvider->DeviceName, &Prefix->AcceptedPrefix);
                ExAcquireResourceExclusiveLite(&MupPrefixTableLock, TRUE);
                if (RtlInsertUnicodePrefix(&MupPrefixTable, &Prefix->AcceptedPrefix, &Prefix->PrefixTableEntry))
                {
                    InsertHeadList(&MupPrefixList, &Prefix->PrefixListEntry);
                    Prefix->InTable = TRUE;
                    Prefix->KeepExtraRef = TRUE;
                }
                else
                {
                    Prefix->InTable = FALSE;
                }
                ExReleaseResourceLite(&MupPrefixTableLock);
            }
        }
    }
    else
    {
        MupDereferenceUncProvider(QueryContext->UncProvider);

        /* We failed and didn't find any provider over the latest iterations */
        if (MasterQueryContext->LatestProvider == NULL)
        {
            /* If we had a success though (broken provider?) set our failed status */
            if (NT_SUCCESS(MasterQueryContext->LatestStatus))
            {
                MasterQueryContext->LatestStatus = Status;
            }
            else
            {
                TableStatus = MupOrderedErrorList[0];
                LatestPos = 0;

                /* Otherwise, time to compare statuses, between the latest failed
                 * and the current failure.
                 * We have an order table of failed status: the deeper you go in the
                 * table, the more the error is critical.
                 * Our goal is to return the most critical status that was returned by
                 * any of the providers
                 */

                /* Look for latest status position */
                while (TableStatus != 0 && TableStatus != MasterQueryContext->LatestStatus)
                {
                    ++LatestPos;
                    TableStatus = MupOrderedErrorList[LatestPos];
                }

                /* If at pos 0, the new status is likely more critical */
                if (LatestPos == 0)
                {
                    MasterQueryContext->LatestStatus = Status;
                }
                else
                {
                    /* Otherwise, find position of the new status in the table */
                    Pos = 0;
                    do
                    {
                        if (Status == MupOrderedErrorList[Pos])
                        {
                            break;
                        }

                        ++Pos;
                    }
                    while (Pos < LatestPos);

                    /* If it has a higher position (more critical), return it */
                    if (Pos >= LatestPos)
                    {
                        MasterQueryContext->LatestStatus = Status;
                    }
                }
            }
        }
    }

Cleanup:
    ExFreePoolWithTag(QueryResponse, TAG_MUP);
    ExFreePoolWithTag(QueryContext, TAG_MUP);
    IoFreeIrp(Irp);

    ExReleaseResourceLite(&MasterQueryContext->QueryPathListLock);
    MupDereferenceMasterQueryContext(MasterQueryContext);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
CreateRedirectedFile(PIRP Irp,
                     PFILE_OBJECT FileObject,
                     PIO_SECURITY_CONTEXT SecurityContext)
{
    LONG Len;
    WCHAR Cur;
    PWSTR Name;
    PIRP QueryIrp;
    NTSTATUS Status;
    PMUP_PFX Prefix;
    PLIST_ENTRY Entry;
    PMUP_UNC UncProvider;
    PIO_STACK_LOCATION Stack;
    LARGE_INTEGER CurrentTime;
    PMUP_MQC MasterQueryContext;
    PQUERY_PATH_CONTEXT QueryContext;
    PQUERY_PATH_REQUEST QueryPathRequest;
    PUNICODE_PREFIX_TABLE_ENTRY TableEntry;
    BOOLEAN Locked, Referenced, BreakOnFirst;

    /* We cannot open a file without a name */
    if (FileObject->FileName.Length == 0)
    {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    DPRINT1("Request for opening: %wZ\n", &FileObject->FileName);

    Referenced = FALSE;
    BreakOnFirst = TRUE;
    Status = STATUS_BAD_NETWORK_PATH;

    ExAcquireResourceExclusiveLite(&MupPrefixTableLock, TRUE);
    /* First, try to see if that's a prefix we already know */
    TableEntry = RtlFindUnicodePrefix(&MupPrefixTable, &FileObject->FileName, 1);
    if (TableEntry != NULL)
    {
        Prefix = CONTAINING_RECORD(TableEntry, MUP_PFX, PrefixTableEntry);

        DPRINT("Matching prefix found: %wZ\n", &Prefix->AcceptedPrefix);

        /* If so, check whether the prefix is still valid */
        KeQuerySystemTime(&CurrentTime);
        if (Prefix->ValidityTimeout.QuadPart < CurrentTime.QuadPart)
        {
            /* It is: so, update its validity period and reroute file opening */
            MupCalculateTimeout(&Prefix->ValidityTimeout);
            Status = MupRerouteOpen(FileObject, Prefix->UncProvider);
            ExReleaseResourceLite(&MupPrefixTableLock);

            if (Status == STATUS_REPARSE)
            {
                Irp->IoStatus.Information = FILE_SUPERSEDED;
            }

            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);

            return Status;
        }

        /* When here, we found a matching prefix, but expired, remove it from the table
         * We'll redo a full search
         */
        if (Prefix->InTable)
        {
            MupRemoveKnownPrefixEntry(Prefix);
        }
    }
    ExReleaseResourceLite(&MupPrefixTableLock);

    Stack = IoGetCurrentIrpStackLocation(Irp);
    /* First of all, start looking for a mailslot */
    if (FileObject->FileName.Buffer[0] == L'\\' && Stack->MajorFunction != IRP_MJ_CREATE)
    {
        Name = &FileObject->FileName.Buffer[1];
        Len = FileObject->FileName.Length;

        /* Skip the remote destination name */
        do
        {
            Len -= sizeof(WCHAR);
            if (Len <= 0)
            {
                break;
            }

            Cur = *Name;
            ++Name;
        } while (Cur != L'\\');
        Len -= sizeof(WCHAR);

        /* If we still have room for "Mailslot" to fit */
        if (Len >= (sizeof(L"Mailslot") - sizeof(UNICODE_NULL)))
        {
            /* Get the len in terms of chars count */
            Len /= sizeof(WCHAR);
            if (Len > ((sizeof(L"Mailslot") - sizeof(UNICODE_NULL)) / sizeof(WCHAR)))
            {
                Len = (sizeof(L"Mailslot") - sizeof(UNICODE_NULL)) / sizeof(WCHAR);
            }

            /* It's indeed a mailslot opening! */
            if (_wcsnicmp(Name, L"Mailslot", Len) == 0)
            {
                /* Broadcast open */
                Status = BroadcastOpen(Irp);
                if (Status == STATUS_REPARSE)
                {
                    Irp->IoStatus.Information = FILE_SUPERSEDED;
                }

                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);

                return Status;
            }
        }
    }

    /* Ok, at that point, that's a regular MUP opening (if no DFS) */
    if (!MupEnableDfs || FileObject->FsContext2 == (PVOID)DFS_DOWNLEVEL_OPEN_CONTEXT)
    {
        /* We won't complete immediately */
        IoMarkIrpPending(Irp);

        /* Allocate a new prefix for our search */
        Prefix = MupAllocatePrefixEntry(0);
        if (Prefix == NULL)
        {
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);

            return STATUS_PENDING;
        }

        /* Allocate a context for our search */
        MasterQueryContext = MupAllocateMasterQueryContext();
        if (MasterQueryContext == NULL)
        {
            MupFreeNode(Prefix);

            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);

            return STATUS_PENDING;
        }

        MasterQueryContext->Irp = Irp;
        MasterQueryContext->FileObject = FileObject;
        MasterQueryContext->LatestProvider = NULL;
        MasterQueryContext->Prefix = Prefix;
        MasterQueryContext->LatestStatus = STATUS_BAD_NETWORK_PATH;
        ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);
        InsertTailList(&MupMasterQueryList, &MasterQueryContext->MQCListEntry);
        ++Prefix->NodeReferences;
        ExReleaseResourceLite(&MupGlobalLock);

        _SEH2_TRY
        {
            ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);
            Locked = TRUE;

            /* Now, we will browse all the providers we know, to ask for their accepted prefix regarding the path */
            for (Entry = MupProviderList.Flink; Entry != &MupProviderList; Entry = Entry->Flink)
            {
                UncProvider = CONTAINING_RECORD(Entry, MUP_UNC, ProviderListEntry);

                ++UncProvider->NodeReferences;
                Referenced = TRUE;

                ExReleaseResourceLite(&MupGlobalLock);
                Locked = FALSE;

                /* We will obviously only query registered providers */
                if (UncProvider->Registered)
                {
                    /* We will issue an IOCTL_REDIR_QUERY_PATH, so allocate input buffer */
                    QueryPathRequest = ExAllocatePoolWithTag(PagedPool, FileObject->FileName.Length + sizeof(QUERY_PATH_REQUEST), TAG_MUP);
                    if (QueryPathRequest == NULL)
                    {
                        ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                    }

                    /* Allocate a context for IRP completion routine
                     * In case a prefix matches the path, the reroute will happen
                     * in the completion routine, when we have return from the provider
                     */
                    QueryContext = ExAllocatePoolWithTag(PagedPool, sizeof(QUERY_PATH_CONTEXT), TAG_MUP);
                    if (QueryContext == NULL)
                    {
                        ExFreePoolWithTag(QueryPathRequest, TAG_MUP);
                        ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                    }

                    InitializeListHead(&QueryContext->QueryPathListEntry);
                    QueryContext->MasterQueryContext = MasterQueryContext;
                    QueryContext->QueryPathRequest = QueryPathRequest;
                    QueryPathRequest->PathNameLength = FileObject->FileName.Length;
                    QueryPathRequest->SecurityContext = SecurityContext;
                    RtlMoveMemory(QueryPathRequest->FilePathName, FileObject->FileName.Buffer, FileObject->FileName.Length);

                    /* Build our IRP for the query */
                    QueryIrp = MupBuildIoControlRequest(UncProvider->FileObject,
                                                        QueryContext,
                                                        IRP_MJ_DEVICE_CONTROL,
                                                        IOCTL_REDIR_QUERY_PATH,
                                                        QueryPathRequest,
                                                        FileObject->FileName.Length + sizeof(QUERY_PATH_REQUEST),
                                                        QueryPathRequest,
                                                        sizeof(QUERY_PATH_RESPONSE),
                                                        QueryPathCompletionRoutine);
                    if (QueryIrp == NULL)
                    {
                        ExFreePoolWithTag(QueryContext, TAG_MUP);
                        ExFreePoolWithTag(QueryPathRequest, TAG_MUP);
                        ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                    }

                    QueryIrp->RequestorMode = KernelMode;
                    QueryContext->UncProvider = UncProvider;
                    QueryContext->Irp = QueryIrp;

                    ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);
                    ++UncProvider->NodeReferences;
                    ++MasterQueryContext->NodeReferences;
                    ExReleaseResourceLite(&MupGlobalLock);

                    ExAcquireResourceExclusiveLite(&MasterQueryContext->QueryPathListLock, TRUE);
                    InsertTailList(&MasterQueryContext->QueryPathList, &QueryContext->QueryPathListEntry);
                    ExReleaseResourceLite(&MasterQueryContext->QueryPathListLock);

                    /* Query the provider !*/
                    DPRINT1("Requesting UNC provider: %wZ\n", &UncProvider->DeviceName);
                    DPRINT("Calling: %wZ\n", &UncProvider->DeviceObject->DriverObject->DriverName);
                    Status = IoCallDriver(UncProvider->DeviceObject, QueryIrp);
                }

                ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);
                Locked = TRUE;

                /* We're done with that provider */
                MupDereferenceUncProvider(UncProvider);
                Referenced = FALSE;

                /* If query went fine on the first request, just break and leave */
                if (BreakOnFirst && Status == STATUS_SUCCESS)
                {
                    break;
                }

                BreakOnFirst = FALSE;
            }
        }
        _SEH2_FINALLY
        {
            if (_SEH2_AbnormalTermination())
            {
                MasterQueryContext->LatestStatus = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (Referenced)
            {
                MupDereferenceUncProvider(UncProvider);
            }

            if (Locked)
            {
                ExReleaseResourceLite(&MupGlobalLock);
            }

            MupDereferenceMasterQueryContext(MasterQueryContext);

            Status = STATUS_PENDING;
        }
        _SEH2_END;
    }
    else
    {
        UNIMPLEMENTED;
        Status = STATUS_NOT_IMPLEMENTED;
    }

    return Status;
}

NTSTATUS
OpenMupFileSystem(PMUP_VCB Vcb,
                  PFILE_OBJECT FileObject,
                  ACCESS_MASK DesiredAccess,
                  USHORT ShareAccess)
{
    NTSTATUS Status;

    DPRINT1("Opening MUP\n");

    ExAcquireResourceExclusiveLite(&MupVcbLock, TRUE);
    _SEH2_TRY
    {
        /* Update share access, increase reference count, and associated VCB to the FO, that's it! */
        Status = IoCheckShareAccess(DesiredAccess, ShareAccess, FileObject, &Vcb->ShareAccess, TRUE);
        if (NT_SUCCESS(Status))
        {
            ++Vcb->NodeReferences;
            MupSetFileObject(FileObject, (PMUP_FCB)Vcb, NULL);
            Status = STATUS_SUCCESS;
        }
    }
    _SEH2_FINALLY
    {
        ExReleaseResourceLite(&MupVcbLock);
    }
    _SEH2_END;

    return Status;
}

NTSTATUS
NTAPI
MupCreate(PDEVICE_OBJECT DeviceObject,
          PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;
    PFILE_OBJECT FileObject, RelatedFileObject;

    FsRtlEnterFileSystem();

    _SEH2_TRY
    {
        /* If DFS is enabled, check if that's for DFS and is so relay */
        if (MupEnableDfs && (DeviceObject->DeviceType == FILE_DEVICE_DFS || DeviceObject->DeviceType == FILE_DEVICE_DFS_FILE_SYSTEM))
        {
            Status = DfsFsdCreate(DeviceObject, Irp);
        }
        else
        {
            Stack = IoGetCurrentIrpStackLocation(Irp);
            FileObject = Stack->FileObject;
            RelatedFileObject = FileObject->RelatedFileObject;

            /* If we have a file name or if the associated FCB of the related FO isn't the VCB, then, it's a regular opening */
            if (FileObject->FileName.Length != 0 || (RelatedFileObject != NULL && ((PMUP_FCB)(RelatedFileObject->FsContext))->NodeType != NODE_TYPE_VCB))
            {
                Status = CreateRedirectedFile(Irp, FileObject, Stack->Parameters.Create.SecurityContext);
            }
            /* Otherwise, it's just a volume open */
            else
            {
                Status = OpenMupFileSystem(DeviceObject->DeviceExtension,
                                           FileObject,
                                           Stack->Parameters.Create.SecurityContext->DesiredAccess,
                                           Stack->Parameters.Create.ShareAccess);

                Irp->IoStatus.Information = FILE_OPENED;
                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();

        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    }
    _SEH2_END;

    FsRtlExitFileSystem();

    return Status;
}

VOID
MupCloseUncProvider(PMUP_UNC UncProvider)
{
    ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);

    /* If the node was still valid, reregister the UNC provider */
    if (UncProvider->NodeStatus == NODE_STATUS_HEALTHY)
    {
        UncProvider->NodeStatus = NODE_STATUS_CLEANUP;
        UncProvider->Registered = FALSE;
        ExReleaseResourceLite(&MupGlobalLock);

        if (UncProvider->FileObject != NULL)
        {
            ZwClose(UncProvider->DeviceHandle);
            ObDereferenceObject(UncProvider->FileObject);
        }
    }
    else
    {
        ExReleaseResourceLite(&MupGlobalLock);
    }
}

NTSTATUS
NTAPI
MupCleanup(PDEVICE_OBJECT DeviceObject,
           PIRP Irp)
{
    ULONG Type;
    PMUP_FCB Fcb;
    PMUP_CCB Ccb;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;

    /* If DFS is enabled, check if that's for DFS and is so relay */
    if (MupEnableDfs)
    {
        if (DeviceObject->DeviceType == FILE_DEVICE_DFS || DeviceObject->DeviceType == FILE_DEVICE_DFS_FILE_SYSTEM)
        {
            return DfsFsdCleanup(DeviceObject, Irp);
        }
    }

    FsRtlEnterFileSystem();

    _SEH2_TRY
    {
        Stack = IoGetCurrentIrpStackLocation(Irp);
        Type = MupDecodeFileObject(Stack->FileObject, &Fcb, &Ccb);
        switch (Type)
        {
            case NODE_TYPE_VCB:
                /* If we got a VCB, clean it up */
                MupCleanupVcb(DeviceObject, Irp, (PMUP_VCB)Fcb);

                Irp->IoStatus.Status = STATUS_SUCCESS;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);

                MupDereferenceVcb((PMUP_VCB)Fcb);

                /* If Ccb is not null, then, it's a UNC provider node */
                if (Ccb)
                {
                    /* Close it, and dereference */
                    MupCloseUncProvider((PMUP_UNC)Ccb);
                    MupDereferenceUncProvider((PMUP_UNC)Ccb);
                    ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);
                    --MupProviderCount;
                    ExReleaseResourceLite(&MupGlobalLock);
                }

                Status = STATUS_SUCCESS;
                break;

            case NODE_TYPE_FCB:
                /* If the node wasn't already cleaned, do it */
                if (Fcb->NodeStatus == NODE_STATUS_HEALTHY)
                {
                    MupCleanupFcb(DeviceObject, Irp, Fcb);
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    Status = STATUS_INVALID_HANDLE;
                }

                Irp->IoStatus.Status = STATUS_SUCCESS;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);

                MupDereferenceFcb(Fcb);
                break;

            default:
                Status = STATUS_INVALID_HANDLE;

                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);

                break;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    FsRtlExitFileSystem();

    return Status;
}

NTSTATUS
MupCloseVcb(PDEVICE_OBJECT DeviceObject,
            PIRP Irp,
            PMUP_VCB Vcb,
            PFILE_OBJECT FileObject)
{
    ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);

    /* Remove FCB, UNC from FO */
    MupSetFileObject(FileObject, NULL, NULL);
    MupDereferenceVcb(Vcb);

    ExReleaseResourceLite(&MupGlobalLock);

    return STATUS_SUCCESS;
}

NTSTATUS
MupCloseFcb(PDEVICE_OBJECT DeviceObject,
            PIRP Irp,
            PMUP_FCB Fcb,
            PFILE_OBJECT FileObject)
{
    ExAcquireResourceExclusiveLite(&MupGlobalLock, TRUE);

    /* Remove FCB, CCB from FO */
    MupSetFileObject(FileObject, NULL, NULL);
    MupDereferenceFcb(Fcb);

    ExReleaseResourceLite(&MupGlobalLock);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MupClose(PDEVICE_OBJECT DeviceObject,
         PIRP Irp)
{
    PMUP_FCB Fcb;
    PMUP_CCB Ccb;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;

    /* If DFS is enabled, check if that's for DFS and is so relay */
    if (MupEnableDfs)
    {
        if (DeviceObject->DeviceType == FILE_DEVICE_DFS || DeviceObject->DeviceType == FILE_DEVICE_DFS_FILE_SYSTEM)
        {
            return DfsFsdClose(DeviceObject, Irp);
        }
    }

    FsRtlEnterFileSystem();

    _SEH2_TRY
    {
        /* Get our internal structures from FO */
        Stack = IoGetCurrentIrpStackLocation(Irp);
        MupDecodeFileObject(Stack->FileObject, &Fcb, &Ccb);
        if (Fcb == NULL)
        {
            Status = STATUS_INVALID_HANDLE;

            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);

            _SEH2_LEAVE;
        }

        /* If we got the VCB, that's a volume close */
        if (Fcb->NodeType == NODE_TYPE_VCB)
        {
            Status = MupCloseVcb(DeviceObject, Irp, (PMUP_VCB)Fcb, Stack->FileObject);
        }
        /* Otherwise close the FCB */
        else if (Fcb->NodeType == NODE_TYPE_FCB)
        {
            MupDereferenceFcb(Fcb);
            Status = MupCloseFcb(DeviceObject, Irp, Fcb, Stack->FileObject);
        }
        else
        {
            Status = STATUS_INVALID_HANDLE;

            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);

            _SEH2_LEAVE;
        }

        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    FsRtlExitFileSystem();

    return Status;
}

VOID
NTAPI
MupUnload(PDRIVER_OBJECT DriverObject)
{
    IoDeleteDevice(mupDeviceObject);

    if (MupEnableDfs)
    {
        DfsUnload(DriverObject);
    }

    MupUninitializeData();
}

/*
 * FUNCTION: Called by the system to initialize the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
CODE_SEG("INIT")
NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    UNICODE_STRING MupString;
    PDEVICE_OBJECT DeviceObject;

    /* Only initialize global state of the driver
     * Other inits will happen when required
     */
    MupInitializeData();

    /* Check if DFS is disabled */
    MupEnableDfs = MuppIsDfsEnabled();
    /* If it's not disabled but when cannot init, disable it */
    if (MupEnableDfs && !NT_SUCCESS(DfsDriverEntry(DriverObject, RegistryPath)))
    {
        MupEnableDfs = FALSE;
    }

    /* Create the MUP device */
    RtlInitUnicodeString(&MupString, L"\\Device\\Mup");
    Status = IoCreateDevice(DriverObject, sizeof(MUP_VCB), &MupString, FILE_DEVICE_MULTI_UNC_PROVIDER, 0, FALSE, &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        if (MupEnableDfs)
        {
            DfsUnload(DriverObject);
        }

        MupUninitializeData();

        return Status;
    }

    /* Set our MJ */
    DriverObject->DriverUnload = MupUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = MupCreate;
    DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] = MupCreate;
    DriverObject->MajorFunction[IRP_MJ_CREATE_MAILSLOT] = MupCreate;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = MupForwardIoRequest;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = MupFsControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = MupCleanup;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = MupClose;

    /* And finish init */
    mupDeviceObject = DeviceObject;
    MupInitializeVcb(DeviceObject->DeviceExtension);

    return STATUS_SUCCESS;
}
