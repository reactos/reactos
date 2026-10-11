/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel Shim Engine implementation
 * COPYRIGHT:   Copyright 2020 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2020-2026 Mark Jansen <mark.jansen@reactos.org>
 */

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

// http://www.geoffchappell.com/studies/windows/km/ntoskrnl/api/kshim/ksecore/index.htm

#define KSE_MAX_SHIMS_PER_DRIVER    10


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
KsepPoolAllocateNonPaged(
    _In_ SIZE_T NumberOfBytes)
{
    PVOID Ptr = ExAllocatePoolWithTag(NonPagedPool, NumberOfBytes, TAG_KSE_CALLBACKS);
    if (Ptr)
        return Ptr;

    DPRINT1("KSE: Unable to allocated %u bytes\n", NumberOfBytes);
    return NULL;
}

static PKSE_SHIM NTAPI
KsepFindShim(
    _In_ PCUNICODE_STRING Name)
{
    PKSE_PROVIDER ProviderEntry;
    PLIST_ENTRY ListEntry;
    UNICODE_STRING ShimName;

    for (ListEntry = KseEngine.ProvidersListHead.Flink;
         ListEntry != &KseEngine.ProvidersListHead;
         ListEntry = ListEntry->Flink)
    {
        ProviderEntry = CONTAINING_RECORD(ListEntry, KSE_PROVIDER, ProviderList);
        if (!ProviderEntry->Shim->ShimName)
            continue;

        RtlInitUnicodeString(&ShimName, ProviderEntry->Shim->ShimName);
        if (RtlEqualUnicodeString(&ShimName, Name, TRUE))
            return ProviderEntry->Shim;
    }

    return NULL;
}

static NTSTATUS NTAPI
KsepMultiSzToShims(
    IN PKEY_VALUE_FULL_INFORMATION KeyValueInformation,
    OUT PKSE_SHIM* Shims,
    OUT PULONG NumberOfShims)
{
    PWCHAR Current, End;
    UNICODE_STRING ShimName;
    SIZE_T Length;

    *NumberOfShims = 0;

    Current = (PWCHAR)((ULONG_PTR)KeyValueInformation + KeyValueInformation->DataOffset);
    End = (PWCHAR)((ULONG_PTR)Current + KeyValueInformation->DataLength);

    while (Current < End && *Current)
    {
        /* Find the end of this string, without running past the data */
        for (Length = 0; Current + Length < End && Current[Length]; Length++)
            ;

        ShimName.Buffer = Current;
        ShimName.Length = ShimName.MaximumLength = (USHORT)(Length * sizeof(WCHAR));

        if (*NumberOfShims >= KSE_MAX_SHIMS_PER_DRIVER)
        {
            DPRINT1("KSE: Too many shims, ignoring %wZ\n", &ShimName);
        }
        else
        {
            Shims[*NumberOfShims] = KsepFindShim(&ShimName);
            if (Shims[*NumberOfShims])
                (*NumberOfShims)++;
            else
                DPRINT1("KSE: Shim %wZ not found\n", &ShimName);
        }

        Current += Length + 1;
    }

    return *NumberOfShims ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}

static NTSTATUS NTAPI
KsepGetShimsFromRegistry(
    IN PLDR_DATA_TABLE_ENTRY LdrEntry,
    OUT PKSE_SHIM* Shims,
    OUT PULONG NumberOfShims)
{
    static const UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"Shims");
    WCHAR KeyNameBuffer[256];
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    ULONG KeyValueSize = 0x800;
    HANDLE KeyHandle;
    NTSTATUS Status;

    *NumberOfShims = 0;

    RtlInitEmptyUnicodeString(&KeyName, KeyNameBuffer, sizeof(KeyNameBuffer));
    Status = RtlAppendUnicodeToString(&KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Compatibility\\Driver\\");
    if (NT_SUCCESS(Status))
        Status = RtlAppendUnicodeStringToString(&KeyName, &LdrEntry->BaseDllName);
    if (!NT_SUCCESS(Status))
        return Status;

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
        return Status;

    KeyValueInformation = ExAllocatePoolWithTag(PagedPool, KeyValueSize, TAG_KSE_REGISTRY);
    if (!KeyValueInformation)
    {
        ZwClose(KeyHandle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ZwQueryValueKey(KeyHandle,
                             (PUNICODE_STRING)&ValueName,
                             KeyValueFullInformation,
                             KeyValueInformation,
                             KeyValueSize,
                             &KeyValueSize);
    ZwClose(KeyHandle);

    if (NT_SUCCESS(Status))
    {
        if (KeyValueInformation->Type == REG_MULTI_SZ && KeyValueInformation->DataLength)
            Status = KsepMultiSzToShims(KeyValueInformation, Shims, NumberOfShims);
        else
            Status = STATUS_OBJECT_TYPE_MISMATCH;
    }

    ExFreePoolWithTag(KeyValueInformation, TAG_KSE_REGISTRY);
    return Status;
}

static NTSTATUS NTAPI
KsepGetShimsForDriver(
    _In_ PLDR_DATA_TABLE_ENTRY LdrEntry,
    _Out_ PKSE_SHIM* Shims,
    _Out_ PULONG NumberOfShims)
{
    /* TODO: Also match against the driver database (drvmain.sdb) */
    return KsepGetShimsFromRegistry(LdrEntry, Shims, NumberOfShims);
}


static NTSTATUS NTAPI
KsepGetShimCallbacksForDriver(
    _In_ PLDR_DATA_TABLE_ENTRY LdrEntry,
    _Out_ PKSE_DRIVER_IO_CALLBACKS pIoCallbacks)
{
    PKSE_SHIM Shims[KSE_MAX_SHIMS_PER_DRIVER];
    ULONG NumberOfShims, n;
    PKSE_HOOK_COLLECTION HookCollection;
    PKSE_HOOK Hook;
    NTSTATUS Status;

    RtlZeroMemory(pIoCallbacks, sizeof(*pIoCallbacks));

    Status = KsepGetShimsForDriver(LdrEntry, Shims, &NumberOfShims);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("KSE: No Shim found for %wZ\n", &LdrEntry->BaseDllName);
        return Status;
    }

    for (n = 0; n < NumberOfShims; n++)
    {
        for (HookCollection = Shims[n]->HookCollectionsArray;
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
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KseShimDriverIoCallbacks(
    _In_ PDRIVER_OBJECT DriverObject)
{
    KSE_DRIVER_IO_CALLBACKS IoCallbacks;
    PKSE_DRIVER_IO_CALLBACKS KseCallbacks;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    BOOLEAN InKernel;
    ULONG i;
    NTSTATUS Status;
    DPRINT("KseShimDriverIoCallbacks(%wZ)\n", &DriverObject->DriverName);

    if (KseEngine.State != KSE_STATE_INITIALIZED)
        return STATUS_SUCCESS;

    if (!KiPcToFileHeader(DriverObject->DriverStart, &LdrEntry, FALSE, &InKernel))
        return STATUS_NOT_FOUND;

    Status = KsepGetShimCallbacksForDriver(LdrEntry, &IoCallbacks);
    if (!NT_SUCCESS(Status))
        return Status;

    KseCallbacks = KsepPoolAllocateNonPaged(sizeof(*KseCallbacks));
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

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
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

static CODE_SEG("INIT") NTSTATUS NTAPI
KseShimDatabaseBootInitialize(VOID)
{
    UNIMPLEMENTED_ONCE;

    return STATUS_SUCCESS;
}

static CODE_SEG("INIT") NTSTATUS NTAPI
KsepMatchInitMachineInfo(VOID)
{
    //UNIMPLEMENTED_ONCE;

    return STATUS_SUCCESS;
}

static PKSE_DRIVER_IO_CALLBACKS NTAPI
KseGetIoCallbacks(
    _In_ PDRIVER_OBJECT DriverObject)
{
    return IoGetDrvObjExtension(DriverObject)->KseCallbacks;
}

static NTSTATUS NTAPI
KseSetCompletionHook(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PIO_COMPLETION_ROUTINE CompletionRoutine,
    _In_ PVOID Context)
{
    return IoSetCompletionRoutineEx(DeviceObject, Irp, CompletionRoutine, Context, TRUE, TRUE, TRUE);
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
KseInitialize(
    _In_ ULONG BootPhase,
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLOADER_PARAMETER_EXTENSION LoaderExtension;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Flags;

    if (BootPhase == 0)
    {
        if (InitSafeBootMode != 0 || /* Safe Boot enabled */
            NT_SUCCESS(MmIsVerifierEnabled(&Flags))) /* Driver Verifier enabled */
        {
            DPRINT("KSE: Not initializing (verifier or safe boot)\n");
            return STATUS_SUCCESS;
        }

        LoaderExtension = LoaderBlock->Extension;
        if (!LoaderExtension || /* No loader extension */
            LoaderExtension->Size < FIELD_OFFSET(LOADER_PARAMETER_EXTENSION, NetworkLoaderBlock) || /* Too small loader extension */
            !LoaderExtension->DrvDBImage) /* No driver database */
        {
            DPRINT("KSE: Not initializing (invalid DB)\n");
            return STATUS_SUCCESS;
        }

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
        /* Phase 0 bailed out */
        if (KseEngine.State != KSE_STATE_INITIALIZING)
            return STATUS_SUCCESS;

        /* Done initializing, so register the remainder of the shims */
        KseEngine.State = KSE_STATE_INITIALIZED;
        Status = STATUS_SUCCESS;
    }

    DPRINT1("KseInitialize(BootPhase %ld) => %08lx\n", BootPhase, Status);
    return Status;
}

NTSTATUS
NTAPI
KseRegisterShim(
    _In_ PKSE_SHIM Shim,
    _In_ PVOID Ignored,
    _In_ ULONG Flags)
{
    return KseRegisterShimEx(Shim, Ignored, Flags, NULL);
}

NTSTATUS
NTAPI
KseRegisterShimEx(
    _In_ PKSE_SHIM Shim,
    _In_ PVOID Ignored,
    _In_ ULONG Flags,
    _In_opt_ PVOID DriverObject)
{
    PKSE_PROVIDER ProviderEntry;
    PLIST_ENTRY ListEntry;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PKSE_HOOK_COLLECTION HookCollection;
    PKSE_HOOK Hook;
    BOOLEAN InKernel;

    UNREFERENCED_PARAMETER(Ignored);

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
    ProviderEntry = ExAllocatePoolWithTag(PagedPool, sizeof(*ProviderEntry), TAG_KSE_PROVIDER);
    if (!ProviderEntry)
        return STATUS_NO_MEMORY;

    ProviderEntry->Shim = Shim;
    InsertTailList(&KseEngine.ProvidersListHead, &ProviderEntry->ProviderList);

    /* Now, update the shim */
    Shim->KseCallbackRoutines = &KseEngine.KseCallbackRoutines;

    if (DriverObject)
        ObReferenceObject(DriverObject);

    DPRINT1("KSE: Registered shim %S\n", Shim->ShimName);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KseUnregisterShim(
    _In_ PKSE_SHIM Shim,
    _In_ PVOID Unknown1,
    _In_ PVOID Unknown2)
{
    UNIMPLEMENTED;
    return STATUS_NOT_SUPPORTED;
}


static BOOLEAN
KsepIsImportFromModule(
    _In_ PCSTR ImportName,
    _In_ PCWSTR ModuleName)
{
    while (*ImportName && *ModuleName)
    {
        if (RtlUpcaseUnicodeChar((WCHAR)(UCHAR)*ImportName) != RtlUpcaseUnicodeChar(*ModuleName))
            return FALSE;
        ImportName++;
        ModuleName++;
    }
    return (*ImportName == ANSI_NULL && *ModuleName == UNICODE_NULL);
}

static PLDR_DATA_TABLE_ENTRY
KsepFindExportingModule(
    _In_ PKSE_HOOK_COLLECTION HookCollection,
    _Out_ PCWSTR* ModuleName)
{
    PLIST_ENTRY ListEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    UNICODE_STRING ExportDriverName;

    /* The kernel is always the first loaded module, the HAL the second one */
    switch (HookCollection->Type)
    {
        case KseCollectionNtExport:
            *ModuleName = L"ntoskrnl.exe";
            ListEntry = PsLoadedModuleList.Flink;
            return CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        case KseCollectionHalExport:
            *ModuleName = L"hal.dll";
            ListEntry = PsLoadedModuleList.Flink->Flink;
            return CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        case KseCollectionDriverExport:
            if (!HookCollection->ExportDriverName)
                return NULL;

            *ModuleName = HookCollection->ExportDriverName;
            RtlInitUnicodeString(&ExportDriverName, HookCollection->ExportDriverName);
            for (ListEntry = PsLoadedModuleList.Flink;
                 ListEntry != &PsLoadedModuleList;
                 ListEntry = ListEntry->Flink)
            {
                LdrEntry = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
                if (RtlEqualUnicodeString(&LdrEntry->BaseDllName, &ExportDriverName, TRUE))
                    return LdrEntry;
            }
            return NULL;

        default:
            return NULL;
    }
}

/* Write an import address table entry. The table can already be write-protected
 * (for example for boot drivers), so write through a temporary mapping. */
static NTSTATUS
KsepWriteImportEntry(
    _In_ PIMAGE_THUNK_DATA Thunk,
    _In_ PVOID NewFunction)
{
    PMDL Mdl;
    PIMAGE_THUNK_DATA MappedThunk;
    NTSTATUS Status = STATUS_SUCCESS;

    Mdl = IoAllocateMdl(Thunk, sizeof(*Thunk), FALSE, FALSE, NULL);
    if (!Mdl)
        return STATUS_INSUFFICIENT_RESOURCES;

    _SEH2_TRY
    {
        MmProbeAndLockPages(Mdl, KernelMode, IoReadAccess);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        IoFreeMdl(Mdl);
        return Status;
    }

    MappedThunk = MmMapLockedPagesSpecifyCache(Mdl, KernelMode, MmCached, NULL, FALSE, NormalPagePriority);
    if (MappedThunk)
    {
        InterlockedExchangePointer((PVOID*)&MappedThunk->u1.Function, NewFunction);
        MmUnmapLockedPages(MappedThunk, Mdl);
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    MmUnlockPages(Mdl);
    IoFreeMdl(Mdl);
    return Status;
}

static VOID
KsepHookImports(
    _In_ PLDR_DATA_TABLE_ENTRY LdrEntry,
    _In_ PKSE_HOOK_COLLECTION HookCollection)
{
    PUCHAR DllBase = LdrEntry->DllBase;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    PIMAGE_THUNK_DATA OriginalThunk, FirstThunk;
    PIMAGE_IMPORT_BY_NAME ImportByName;
    PLDR_DATA_TABLE_ENTRY ExportEntry;
    PCWSTR ModuleName;
    PKSE_HOOK Hook;
    BOOLEAN Match;
    ULONG Size;
    NTSTATUS Status;

    ExportEntry = KsepFindExportingModule(HookCollection, &ModuleName);
    if (!ExportEntry)
    {
        DPRINT1("KSE: Exporting module for collection type %lu not found\n", HookCollection->Type);
        return;
    }

    /* Resolve the original functions. They are the same for every shimmed driver. */
    for (Hook = HookCollection->HookArray; Hook->Type != KseHookInvalid; Hook++)
    {
        if (Hook->Type != KseHookFunction || Hook->OriginalFunction)
            continue;

        Hook->OriginalFunction = RtlFindExportedRoutineByName(ExportEntry->DllBase, Hook->FunctionName);
        if (!Hook->OriginalFunction)
            DPRINT1("KSE: %s not exported by %S\n", Hook->FunctionName, ModuleName);
    }

    ImportDescriptor = RtlImageDirectoryEntryToData(DllBase, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &Size);
    if (!ImportDescriptor)
        return;

    for (; ImportDescriptor->Name && ImportDescriptor->FirstThunk; ImportDescriptor++)
    {
        if (ImportDescriptor->Name >= LdrEntry->SizeOfImage ||
            !KsepIsImportFromModule((PCSTR)(DllBase + ImportDescriptor->Name), ModuleName))
        {
            continue;
        }

        FirstThunk = (PIMAGE_THUNK_DATA)(DllBase + ImportDescriptor->FirstThunk);
        OriginalThunk = ImportDescriptor->OriginalFirstThunk ?
            (PIMAGE_THUNK_DATA)(DllBase + ImportDescriptor->OriginalFirstThunk) : NULL;

        for (; FirstThunk->u1.Function; FirstThunk++, OriginalThunk = OriginalThunk ? OriginalThunk + 1 : NULL)
        {
            /* Without a name table, match on the address the import was bound to */
            ImportByName = NULL;
            if (OriginalThunk &&
                !IMAGE_SNAP_BY_ORDINAL(OriginalThunk->u1.Ordinal) &&
                OriginalThunk->u1.AddressOfData < LdrEntry->SizeOfImage)
            {
                ImportByName = (PIMAGE_IMPORT_BY_NAME)(DllBase + OriginalThunk->u1.AddressOfData);
            }

            for (Hook = HookCollection->HookArray; Hook->Type != KseHookInvalid; Hook++)
            {
                if (Hook->Type != KseHookFunction || !Hook->OriginalFunction)
                    continue;

                if (ImportByName)
                    Match = !strcmp((PCSTR)ImportByName->Name, Hook->FunctionName);
                else
                    Match = (FirstThunk->u1.Function == (ULONG_PTR)Hook->OriginalFunction);

                if (!Match)
                    continue;

                /* FIXME: When two shims hook the same function, the last one wins */
                Status = KsepWriteImportEntry(FirstThunk, Hook->HookFunction);
                if (NT_SUCCESS(Status))
                    DPRINT1("KSE: Hooked %S!%s in %wZ\n", ModuleName, Hook->FunctionName, &LdrEntry->BaseDllName);
                else
                    DPRINT1("KSE: Unable to hook %S!%s in %wZ (0x%08lx)\n", ModuleName, Hook->FunctionName, &LdrEntry->BaseDllName, Status);
                break;
            }
        }
    }
}

static VOID
KsepApplyShimToDriver(
    _In_ PKSE_SHIM Shim,
    _In_ PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PKSE_HOOK_COLLECTION HookCollection;

    for (HookCollection = Shim->HookCollectionsArray;
         HookCollection->Type != KseCollectionInvalid;
         HookCollection++)
    {
        /* Callbacks are applied when the driver object is initialized */
        if (HookCollection->Type == KseCollectionCallback)
            continue;

        KsepHookImports(LdrEntry, HookCollection);
    }
}

NTSTATUS
NTAPI
KseDriverLoadImage(
    _In_ PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PKSE_SHIM Shims[KSE_MAX_SHIMS_PER_DRIVER];
    ULONG NumberOfShims, n;
    NTSTATUS Status;

    if (KseEngine.State != KSE_STATE_INITIALIZED)
        return STATUS_SUCCESS;

    Status = KsepGetShimsForDriver(LdrEntry, Shims, &NumberOfShims);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("KSE: No Shim found for %wZ\n", &LdrEntry->BaseDllName);
        return STATUS_SUCCESS;
    }

    for (n = 0; n < NumberOfShims; n++)
    {
        DPRINT1("KSE: Applying shim %S (0x%08lx) to %wZ\n",
                Shims[n]->ShimName, Shims[n]->ShimGuid->Data1, &LdrEntry->BaseDllName);

        KsepApplyShimToDriver(Shims[n], LdrEntry);

        if (Shims[n]->ShimmedDriverTargetedNotification)
        {
            PIMAGE_NT_HEADERS NtHeaders = RtlImageNtHeader(LdrEntry->DllBase);

            Shims[n]->ShimmedDriverTargetedNotification(&LdrEntry->BaseDllName,
                                                        LdrEntry->DllBase,
                                                        LdrEntry->SizeOfImage,
                                                        NtHeaders ? NtHeaders->FileHeader.TimeDateStamp : 0,
                                                        LdrEntry->CheckSum);
        }
    }

    return STATUS_SUCCESS;
}
