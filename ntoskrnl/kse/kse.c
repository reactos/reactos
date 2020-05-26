/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel Shim Engine implementation
 * COPYRIGHT:   Copyright 2020 Hervé Poussineau (hpoussin@reactos.org)
 */

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>


typedef struct _KSE_PROVIDER
{
    LIST_ENTRY ProviderList;
    PKSE_SHIM Shim;
} KSE_PROVIDER, *PKSE_PROVIDER;


typedef enum _KSE_DISABLE_FLAGS
{
    KSE_DISABLE_NONE = 0,
    KSE_DISABLE_DRIVER_SHIMS = (1 << 0),
    KSE_DISABLE_DEVICE_SHIMS = (1 << 1),
} KSE_DISABLE_FLAGS;


typedef enum _KSE_ENGINE_STATE
{
    KSE_STATE_UNINITIALIZED = 0,
    KSE_STATE_INITIALIZING = 1,
    KSE_STATE_INITIALIZED = 2,
} KSE_ENGINE_STATE;

typedef enum _KSE_ENGINE_FLAGS
{
    KSE_FLAGS_NONE =                0,
    KSE_FLAGS_GROUPPOLICY_OK =      (1 << 1),   // 0x0002
    KSE_FLAGS_DRV_SHIMS_ACTIVE =    (1 << 11),  // 0x0800
    KSE_FLAGS_DEV_SHIMS_ACTIVE =    (1 << 12),  // 0x1000
} KSE_ENGINE_FLAGS;

C_ASSERT(KSE_FLAGS_GROUPPOLICY_OK == 2);
C_ASSERT(KSE_FLAGS_DRV_SHIMS_ACTIVE == 0x800);
C_ASSERT(KSE_FLAGS_DEV_SHIMS_ACTIVE == 0x1000);

typedef struct _KSE_ENGINE
{
    KSE_DISABLE_FLAGS DisableFlags;
    KSE_ENGINE_STATE State;
    KSE_ENGINE_FLAGS Flags;
    LIST_ENTRY ProvidersListHead;       // list of KSE_PROVIDER
    LIST_ENTRY ShimmedDriversListHead;
    KSE_CALLBACK_ROUTINES KseCallbackRoutines;
    PVOID DeviceInfoCache;
    PVOID HardwareIdCache;
    PVOID ShimmedDriverHint;
} KSE_ENGINE, *PKSE_ENGINE;

static KSE_ENGINE KseEngine;


PVOID NTAPI
KsepPoolAllocateNonPaged(IN SIZE_T NumberOfBytes)
{
    PVOID Ptr = ExAllocatePoolWithTag(NonPagedPool, NumberOfBytes, 'KSEn');
    if (Ptr)
        return Ptr;

    DPRINT1("KSE: Unable to allocated %u bytes\n", NumberOfBytes);
    return NULL;
}

static NTSTATUS NTAPI
KsepGetShimForDriver(
    IN PVOID DriverStart,
    OUT PKSE_SHIM* Shim)
{
    UNIMPLEMENTED_ONCE;

    *Shim = NULL;
    return STATUS_NOT_IMPLEMENTED;
}


static NTSTATUS NTAPI
KsepGetShimCallbacksForDriver(
    IN PVOID DriverStart,
    OUT PKSE_DRIVER_IO_CALLBACKS pIoCallbacks)
{
    PKSE_SHIM Shim;
    PKSE_HOOK_COLLECTION HookCollection;
    PKSE_HOOK Hook;
    NTSTATUS Status;

    RtlZeroMemory(pIoCallbacks, sizeof(*pIoCallbacks));

    Status = KsepGetShimForDriver(DriverStart, &Shim);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("KSE: No Shim found for %p\n", DriverStart);
        return Status;
    }

    for (HookCollection = Shim->HookCollectionsArray;
         HookCollection->Type != KseCollectionInvalid;
         HookCollection++)
    {
        if (HookCollection->Type != KseCollectionCallback)
            continue;

        for (Hook = HookCollection->HookArray; Hook->Type != KseHookInvalid; Hook++)
        {
            if (Hook->Type != KseHookIRPCallback)
                continue;

            if (Hook->CallbackId == KseHookCallbackDriverInit)
            {
                pIoCallbacks->DriverInit = Hook->HookFunction;
            }
            else if (Hook->CallbackId == KseHookCallbackDriverStartIo)
            {
                pIoCallbacks->DriverStartIo = Hook->HookFunction;
            }
            else if (Hook->CallbackId == KseHookCallbackDriverUnload)
            {
                pIoCallbacks->DriverUnload = Hook->HookFunction;
            }
            else if (Hook->CallbackId == KseHookCallbackAddDevice)
            {
                pIoCallbacks->AddDevice = Hook->HookFunction;
            }
            else if (Hook->CallbackId >= KseHookCallbackMajorFunction &&
                     Hook->CallbackId <= KseHookCallbackMajorFunction + IRP_MJ_MAXIMUM_FUNCTION)
            {
                pIoCallbacks->MajorFunction[Hook->CallbackId - KseHookCallbackMajorFunction] = Hook->HookFunction;
            }
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KseShimDriverIoCallbacks(
    IN PDRIVER_OBJECT DriverObject)
{
    KSE_DRIVER_IO_CALLBACKS IoCallbacks;
    PKSE_DRIVER_IO_CALLBACKS KseCallbacks;
    ULONG i;
    NTSTATUS Status;
    DPRINT("KseShimDriverIoCallbacks(%wZ)\n", &DriverObject->DriverName);

    Status = KsepGetShimCallbacksForDriver(DriverObject->DriverStart, &IoCallbacks);
    if (!NT_SUCCESS(Status))
        return Status;

    KseCallbacks = KsepPoolAllocateNonPaged(sizeof(KSE_DRIVER_IO_CALLBACKS));
    if (!KseCallbacks)
        return STATUS_NO_MEMORY;

    RtlZeroMemory(KseCallbacks, sizeof(*KseCallbacks));
    if (DriverObject->DriverInit && IoCallbacks.DriverInit)
    {
        KseCallbacks->DriverInit = DriverObject->DriverInit;
        DriverObject->DriverInit = IoCallbacks.DriverInit;
    }
    if (DriverObject->DriverStartIo && IoCallbacks.DriverStartIo)
    {
        KseCallbacks->DriverStartIo = DriverObject->DriverStartIo;
        DriverObject->DriverStartIo = IoCallbacks.DriverStartIo;
    }
    if (DriverObject->DriverUnload && IoCallbacks.DriverUnload)
    {
        KseCallbacks->DriverUnload = DriverObject->DriverUnload;
        DriverObject->DriverUnload = IoCallbacks.DriverUnload;
    }
    if (DriverObject->DriverExtension->AddDevice && IoCallbacks.AddDevice)
    {
        KseCallbacks->AddDevice = DriverObject->DriverExtension->AddDevice;
        DriverObject->DriverExtension->AddDevice = IoCallbacks.AddDevice;
    }

    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION + 1; i++)
    {
        if (DriverObject->MajorFunction[i] && IoCallbacks.MajorFunction[i])
        {
            KseCallbacks->MajorFunction[i] = DriverObject->MajorFunction[i];
            DriverObject->MajorFunction[i] = IoCallbacks.MajorFunction[i];
        }
    }
    IoGetDrvObjExtension(DriverObject)->KseCallbacks = KseCallbacks;

    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI
KseShimDatabaseBootInitialize()
{
    UNIMPLEMENTED_ONCE;

    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI
KsepMatchInitMachineInfo()
{
    //UNIMPLEMENTED_ONCE;

    return STATUS_SUCCESS;
}

static PKSE_DRIVER_IO_CALLBACKS NTAPI
KseGetIoCallbacks(
    IN PDRIVER_OBJECT DriverObject)
{
    return IoGetDrvObjExtension(DriverObject)->KseCallbacks;
}

static NTSTATUS NTAPI
KseSetCompletionHook(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID Context)
{
    return IoSetCompletionRoutineEx(DeviceObject, Irp, CompletionRoutine, Context, TRUE, TRUE, TRUE);
}

NTSTATUS
NTAPI
KseInitialize(
    IN ULONG BootPhase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLOADER_PARAMETER_EXTENSION LoaderExtension = LoaderBlock->Extension;
    NTSTATUS Status;
    ULONG Flags;

    if (InitSafeBootMode != 0 || /* Safe Boot enabled */
        NT_SUCCESS(MmIsVerifierEnabled(&Flags))) /* Driver Verifier enabled */
    {
        DPRINT("KSE: Not initializing (verifier or safe boot)\n");
        return STATUS_SUCCESS;
    }
    if (!LoaderExtension || /* No loader extension */
        LoaderExtension->Size < FIELD_OFFSET(LOADER_PARAMETER_EXTENSION, NetworkLoaderBlock) || /* Too small loader extension */
        !LoaderExtension->DrvDBImage) /* No driver database */
    {
        DPRINT("KSE: Not initializing (invalid DB)\n");
        return STATUS_SUCCESS;
    }

    if (BootPhase == 0)
    {
        /* Temp mark us as initialized, so providers can be registered */
        KseEngine.State = KSE_STATE_INITIALIZED;
        InitializeListHead(&KseEngine.ProvidersListHead);
        InitializeListHead(&KseEngine.ShimmedDriversListHead);
        KseEngine.KseCallbackRoutines.KseGetIoCallbacksRoutine = KseGetIoCallbacks;
        KseEngine.KseCallbackRoutines.KseSetCompletionHookRoutine = KseSetCompletionHook;
        Status = KseShimDatabaseBootInitialize();
        if (NT_SUCCESS(Status))
        {
            Status = KsepMatchInitMachineInfo();
            if (NT_SUCCESS(Status))
            {
                Status = KseDriverScopeInitialize();
            }
        }
        /* We are not done yet! */
        KseEngine.State = KSE_STATE_INITIALIZING;
    }
    else if (BootPhase == 1)
    {
        /* Done initializing, so register the remainder of the shims */
        KseEngine.State = KSE_STATE_INITIALIZED;
        Status = KseVersionLieInitialize();
        //Status = KseRegisterShim(&KseSkipDriverUnloadShim);
    }

    DPRINT1("KseInitialize(BootPhase %ld) => %08lx\n", BootPhase, Status);
    return Status;
}

NTSTATUS
NTAPI
KseRegisterShim(
    IN PKSE_SHIM Shim,
    IN PVOID Ignored,
    IN ULONG Flags)
{
    return KseRegisterShimEx(Shim, Ignored, Flags, NULL);
}

NTSTATUS
NTAPI
KseRegisterShimEx(
    IN PKSE_SHIM Shim,
    IN PVOID Ignored,
    IN ULONG Flags,
    IN PVOID DriverObject OPTIONAL)
{
    PKSE_PROVIDER ProviderEntry;
    PLIST_ENTRY ListEntry;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PKSE_HOOK_COLLECTION HookCollection;
    PKSE_HOOK Hook;
    BOOLEAN InKernel;

    if (!Shim || !Shim->ShimGuid)
    {
        DPRINT1("KSE: Unable to register empty shim or shim without guid\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (KseEngine.State != KSE_STATE_INITIALIZED)
    {
        DPRINT1("KSE: Unable to register shim 0x%08x before engine is initialized\n", Shim->ShimGuid->Data1);
        return STATUS_UNSUCCESSFUL;
    }

    /* Search the loaded module associated to caller */
    if (!KiPcToFileHeader(_ReturnAddress(), &DataTableEntry, FALSE, &InKernel))
        return STATUS_NOT_FOUND;

    /* Check if all shim functions belong to caller */
    for (HookCollection = Shim->HookCollectionsArray; HookCollection->Type != KseCollectionInvalid; HookCollection++)
    {
        for (Hook = HookCollection->HookArray; Hook->Type != KseHookInvalid; Hook++)
        {
            if (Hook->HookFunction < DataTableEntry->DllBase)
                return STATUS_UNSUCCESSFUL;
            if ((ULONG_PTR)Hook->HookFunction - (ULONG_PTR)DataTableEntry->DllBase >= DataTableEntry->SizeOfImage)
                return STATUS_UNSUCCESSFUL;
        }
    }

    /* Search if shim is already loaded */
    for (ListEntry = KseEngine.ProvidersListHead.Flink;
         ListEntry != &KseEngine.ProvidersListHead;
         ListEntry = ListEntry->Flink)
    {
        ProviderEntry = CONTAINING_RECORD(ListEntry, KSE_PROVIDER, ProviderList);
        if (IsEqualGUID(ProviderEntry->Shim->ShimGuid, Shim->ShimGuid))
        {
            DPRINT1("KSE: GUID already registered\n");
            return STATUS_OBJECT_NAME_COLLISION;
        }
    }

    /* Allocate a new entry for this shim */
    ProviderEntry = ExAllocatePoolWithTag(PagedPool, sizeof(KSE_PROVIDER), 'KSEp');
    if (!ProviderEntry)
        return STATUS_NO_MEMORY;

    ProviderEntry->Shim = Shim;
    InsertTailList(&KseEngine.ProvidersListHead, &ProviderEntry->ProviderList);

    /* Now, update the shim */
    Shim->KseCallbackRoutines = &KseEngine.KseCallbackRoutines;

    if (DriverObject)
        ObReferenceObject(DriverObject);

    DPRINT1("KSE: Registered shim 0x%08x\n", Shim->ShimGuid->Data1);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KseUnregisterShim(
    IN PKSE_SHIM Shim,
    IN PVOID Unknown1,
    IN PVOID Unknown2)
{
    ASSERT(FALSE);
    return STATUS_NOT_SUPPORTED;
}


NTSTATUS
NTAPI
KseDriverLoadImage(
    IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PKSE_SHIM Shim;
    NTSTATUS Status;

    Status = KsepGetShimForDriver(LdrEntry->DllBase, &Shim);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("KSE: No Shim found for %wZ\n", &LdrEntry->BaseDllName);
        return Status;
    }

    DPRINT1("KSE: Found Shim 0x%08x for %wZ\n", Shim->ShimGuid->Data1, &LdrEntry->BaseDllName);

    //KsepGetShimsForDriver();
    ASSERT(FALSE);
    return STATUS_NOT_SUPPORTED;
}
