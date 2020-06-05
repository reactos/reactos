/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     KSE 'DriverScope' shim implementation
 * COPYRIGHT:   Copyright 2020 Hervé Poussineau (hpoussin@reactos.org)
 */

#include <ntoskrnl.h>
#include <initguid.h>

//#define NDEBUG
#include <debug.h>

// https://github.com/repnz/etw-providers-docs/blob/master/Manifests-Win10-17134/Microsoft-Windows-Kernel-ShimEngine.xml
// https://github.com/microsoft/Windows-driver-samples/tree/master/general/tracing/evntdrv

extern KSE_SHIM KmDsShim;

#define KSEP_DEBUG_EVENTS   0x0001
#define KSEP_DEBUG_MESSAGES 0x0002
#define KSEP_DEBUG_GENERAL  0x0100
#define KSEP_DEBUG_PNP      0x0200
#define KSEP_DEBUG_POWER    0x0400
#define KSEP_DEBUG_IRP      0x0800
#define KSEP_DEBUG_POOL     0x1000
#define KSEP_DEBUG_UNLOAD   0x2000
static ULONG KsepDebugFlags = ~0;

static NTSTATUS NTAPI
ShimDriverInit(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
    PDRIVER_INITIALIZE OriginalFunction;
    OriginalFunction = KmDsShim.KseCallbackRoutines->KseGetIoCallbacksRoutine(DriverObject)->DriverInit;
    return OriginalFunction(DriverObject, RegistryPath);
}

static VOID NTAPI
ShimDriverStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PDRIVER_OBJECT DriverObject = DeviceObject->DriverObject;
    PDRIVER_STARTIO OriginalFunction;
    OriginalFunction = KmDsShim.KseCallbackRoutines->KseGetIoCallbacksRoutine(DriverObject)->DriverStartIo;
    OriginalFunction(DeviceObject, Irp);
}

static VOID NTAPI
ShimDriverUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    PDRIVER_UNLOAD OriginalFunction;

    OriginalFunction = KmDsShim.KseCallbackRoutines->KseGetIoCallbacksRoutine(DriverObject)->DriverUnload;
    if (KsepDebugFlags & KSEP_DEBUG_GENERAL)
    {
        DPRINT1("[DriverScopeGeneral] DriverObject=%p [%wZ]\n",
            DriverObject, &DriverObject->DriverName);
    }
    OriginalFunction(DriverObject);
}

static NTSTATUS NTAPI
ShimDriverAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDRIVER_ADD_DEVICE OriginalFunction;
    NTSTATUS Status;

    OriginalFunction = KmDsShim.KseCallbackRoutines->KseGetIoCallbacksRoutine(DriverObject)->AddDevice;
    Status = OriginalFunction(DriverObject, PhysicalDeviceObject);

    if (KsepDebugFlags & KSEP_DEBUG_GENERAL)
    {
        DPRINT1("[DriverScopeGeneral] DriverObject=%p [%wZ] Pdo=%p Status=0x%08x\n",
            DriverObject, &DriverObject->DriverName, PhysicalDeviceObject, Status);
    }
    return Status;
}

static NTSTATUS NTAPI
ShimIoCreateDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG DeviceExtensionSize,
    IN PUNICODE_STRING DeviceName,
    IN DEVICE_TYPE DeviceType,
    IN ULONG DeviceCharacteristics,
    IN BOOLEAN Exclusive,
    OUT PDEVICE_OBJECT *DeviceObject)
{
    NTSTATUS (*NTAPI OriginalFunction)(PDRIVER_OBJECT, ULONG, PUNICODE_STRING, DEVICE_TYPE, ULONG, BOOLEAN, PDEVICE_OBJECT*);
    NTSTATUS Status;

    OriginalFunction = KmDsShim.HookCollectionsArray->HookArray[0].OriginalFunction;
    Status = OriginalFunction(DriverObject, DeviceExtensionSize, DeviceName, DeviceType, DeviceCharacteristics, Exclusive, DeviceObject);
    if (KsepDebugFlags & KSEP_DEBUG_GENERAL)
    {
        DPRINT1("[DriverScopeGeneral] DriverObject=%p [%wZ] Fdo=%p DeviceType=%d DeviceCharacteristics=0x%x Exclusive=%d Status=0x%08x\n",
            DriverObject, &DriverObject->DriverName, *DeviceObject, DeviceType, DeviceCharacteristics, Exclusive, Status);
    }
    return Status;
}

static NTSTATUS NTAPI
ShimPoRequestPowerIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PREQUEST_POWER_COMPLETE CompletionFunction,
    IN PVOID Context,
    OUT PIRP *Irp)
{
    PDRIVER_OBJECT DriverObject = DeviceObject->DriverObject;
    NTSTATUS (*NTAPI OriginalFunction)(PDEVICE_OBJECT, UCHAR, POWER_STATE, PREQUEST_POWER_COMPLETE, PVOID, PIRP*);
    NTSTATUS Status;

    OriginalFunction = KmDsShim.HookCollectionsArray->HookArray[1].OriginalFunction;
    Status = OriginalFunction(DeviceObject, MinorFunction, PowerState, CompletionFunction, Context, Irp);
    if (KsepDebugFlags & KSEP_DEBUG_POWER)
    {
        DPRINT1("[DriverScopePower] DriverObject=%p Fdo=%p Irp=%p MinorCode=0x%02x PowerState=%u Status=0x%08x\n",
            DriverObject, DeviceObject, *Irp, MinorFunction, PowerState, Status);
    }
    return Status;
}

static PVOID NTAPI
ShimExAllocatePoolWithTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag)
{
    PVOID p;
    PVOID (*NTAPI OriginalFunction)(POOL_TYPE, SIZE_T, ULONG);

    OriginalFunction = KmDsShim.HookCollectionsArray->HookArray[2].OriginalFunction;

    p = OriginalFunction(PoolType, NumberOfBytes, Tag);
    if (KsepDebugFlags & KSEP_DEBUG_POOL)
    {
        DPRINT1("[DriverScopePool] ExAllocatePool: Address=%p Caller=%p Type=%d Size=%u Tag=0x%08x\n",
            p, _ReturnAddress(), PoolType, (ULONG)NumberOfBytes, Tag);
    }
    return p;
}

static VOID NTAPI
ShimExFreePoolWithTag(
    IN PVOID p,
    IN ULONG Tag)
{
    VOID (*NTAPI OriginalFunction)(PVOID, ULONG);

    OriginalFunction = KmDsShim.HookCollectionsArray->HookArray[3].OriginalFunction;

    if (KsepDebugFlags & KSEP_DEBUG_POOL)
    {
        DPRINT1("[DriverScopePool] ExFreePool: Address=%p Caller=%p Tag=0x%08x\n",
            p, _ReturnAddress(), Tag);
    }

    return OriginalFunction(p, Tag);
}

static PVOID NTAPI
ShimExAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes)
{
    PVOID p;
    PVOID (*NTAPI OriginalFunction)(POOL_TYPE, SIZE_T);

    OriginalFunction = KmDsShim.HookCollectionsArray->HookArray[4].OriginalFunction;

    p = OriginalFunction(PoolType, NumberOfBytes);
    if (KsepDebugFlags & KSEP_DEBUG_POOL)
    {
        DPRINT1("[DriverScopePool] ExAllocatePool: Address=%p Caller=%p Type=%d Size=%u Tag=0x%08x\n",
            p, _ReturnAddress(), PoolType, (ULONG)NumberOfBytes, 0);
    }

    return p;
}

static VOID NTAPI
ShimExFreePool(
    IN PVOID p)
{
    VOID (*NTAPI OriginalFunction)(PVOID);

    OriginalFunction = KmDsShim.HookCollectionsArray->HookArray[5].OriginalFunction;

    if (KsepDebugFlags & KSEP_DEBUG_POOL)
    {
        DPRINT1("[DriverScopePool] ExFreePool: Address=%p Caller=%p Tag=0x%08x\n",
            p, _ReturnAddress(), 0);
    }

    return OriginalFunction(p);
}

static NTSTATUS NTAPI
ShimIrpComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PDRIVER_OBJECT DriverObject = DeviceObject->DriverObject;
    ULONG MajorFunction = IoGetCurrentIrpStackLocation(Irp)->MajorFunction;
    NTSTATUS Status = Irp->IoStatus.Status;

    if (KsepDebugFlags & KSEP_DEBUG_IRP)
    {
        DPRINT1("[DriverScopeIrps] DriverObject=%p [%wZ] Fdo=%p Irp=%p MajorCode=0x%02x Status=0x%08x\n",
            DriverObject, &DriverObject->DriverName, DeviceObject, Irp, MajorFunction, Status);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI
ShimIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PDRIVER_OBJECT DriverObject = DeviceObject->DriverObject;
    PDRIVER_DISPATCH OriginalFunction;
    ULONG MajorFunction = IoGetCurrentIrpStackLocation(Irp)->MajorFunction;
    NTSTATUS Status;

    OriginalFunction = KmDsShim.KseCallbackRoutines->KseGetIoCallbacksRoutine(DriverObject)->MajorFunction[MajorFunction];
    KmDsShim.KseCallbackRoutines->KseSetCompletionHookRoutine(DeviceObject, Irp, ShimIrpComplete, NULL);
    Status = OriginalFunction(DeviceObject, Irp);

    if (KsepDebugFlags & KSEP_DEBUG_IRP)
    {
        if (Status == STATUS_PENDING)
        {
            DPRINT1("[DriverScopeIrps] DriverObject=%p [%wZ] Fdo=%p Irp=%p\n",
                DriverObject, &DriverObject->DriverName, DeviceObject, Irp);
        }
        else
        {
            DPRINT1("[DriverScopeIrps] DriverObject=%p [%wZ] Fdo=%p Irp=%p MajorCode=0x%02x Status=0x%08x\n",
                DriverObject, &DriverObject->DriverName, DeviceObject, Irp, MajorFunction, Status);
        }
    }
    return Status;
}

static NTSTATUS NTAPI
ShimIrpPnpComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PDRIVER_OBJECT DriverObject = DeviceObject->DriverObject;
    NTSTATUS Status = Irp->IoStatus.Status;

    if (KsepDebugFlags & KSEP_DEBUG_PNP)
    {
        DPRINT1("[DriverScopePnp] DriverObject=%p [%wZ] Fdo=%p Irp=%p Status=0x%08x\n",
            DriverObject, &DriverObject->DriverName, DeviceObject, Irp, Status);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI
ShimIrpPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PDRIVER_OBJECT DriverObject = DeviceObject->DriverObject;
    PDRIVER_DISPATCH OriginalFunction;
    ULONG MajorFunction = IoGetCurrentIrpStackLocation(Irp)->MajorFunction;
    ULONG MinorFunction = IoGetCurrentIrpStackLocation(Irp)->MinorFunction;
    NTSTATUS Status;

    OriginalFunction = KmDsShim.KseCallbackRoutines->KseGetIoCallbacksRoutine(DriverObject)->MajorFunction[MajorFunction];
    KmDsShim.KseCallbackRoutines->KseSetCompletionHookRoutine(DeviceObject, Irp, ShimIrpPnpComplete, NULL);
    Status = OriginalFunction(DeviceObject, Irp);

    if (KsepDebugFlags & KSEP_DEBUG_PNP)
    {
        if (Status == STATUS_PENDING)
        {
            DPRINT1("[DriverScopePnp] DriverObject=%p [%wZ] Fdo=%p Irp=%p\n",
                DriverObject, &DriverObject->DriverName, DeviceObject, Irp);
        }
        else
        {
            DPRINT1("[DriverScopePnp] DriverObject=%p [%wZ] Fdo=%p Irp=%p MinorCode=0x%02x Status=0x%08x\n",
                DriverObject, &DriverObject->DriverName, DeviceObject, Irp, MinorFunction, Status);
        }
    }

    return Status;
}

static KSE_HOOK KmDsShimHooksNT[] = {
    { KseHookFunction, { "IoCreateDevice" }, (PVOID)(ULONG_PTR)ShimIoCreateDevice },
    { KseHookFunction, { "PoRequestPowerIrp" }, (PVOID)(ULONG_PTR)ShimPoRequestPowerIrp },
    { KseHookFunction, { "ExAllocatePoolWithTag" }, (PVOID)(ULONG_PTR)ShimExAllocatePoolWithTag },
    { KseHookFunction, { "ExFreePoolWithTag" }, (PVOID)(ULONG_PTR)ShimExFreePoolWithTag },
    { KseHookFunction, { "ExAllocatePool" }, (PVOID)(ULONG_PTR)ShimExAllocatePool },
    { KseHookFunction, { "ExFreePool" }, (PVOID)(ULONG_PTR)ShimExFreePool },
    { KseHookInvalid },
};
static KSE_HOOK KmDsShimHooksCB[] = {
    { KseHookIRPCallback, { (PCHAR)(ULONG_PTR)KseHookCallbackDriverInit }, (PVOID)(ULONG_PTR)ShimDriverInit },
    { KseHookIRPCallback, { (PCHAR)(ULONG_PTR)KseHookCallbackDriverStartIo }, (PVOID)(ULONG_PTR)ShimDriverStartIo },
    { KseHookIRPCallback, { (PCHAR)(ULONG_PTR)KseHookCallbackDriverUnload }, (PVOID)(ULONG_PTR)ShimDriverUnload },
    { KseHookIRPCallback, { (PCHAR)(ULONG_PTR)KseHookCallbackAddDevice }, (PVOID)(ULONG_PTR)ShimDriverAddDevice },
    { KseHookIRPCallback, { (PCHAR)(ULONG_PTR)(KseHookCallbackMajorFunction + IRP_MJ_CREATE) }, (PVOID)(ULONG_PTR)ShimIrp },
    { KseHookIRPCallback, { (PCHAR)(ULONG_PTR)(KseHookCallbackMajorFunction + IRP_MJ_CLOSE) }, (PVOID)(ULONG_PTR)ShimIrp },
    { KseHookIRPCallback, { (PCHAR)(ULONG_PTR)(KseHookCallbackMajorFunction + IRP_MJ_POWER) }, (PVOID)(ULONG_PTR)ShimIrp },
    { KseHookIRPCallback, { (PCHAR)(ULONG_PTR)(KseHookCallbackMajorFunction + IRP_MJ_PNP) }, (PVOID)(ULONG_PTR)ShimIrpPnp },
    { KseHookIRPCallback, { (PCHAR)(ULONG_PTR)(KseHookCallbackMajorFunction + IRP_MJ_DEVICE_CONTROL) }, (PVOID)(ULONG_PTR)ShimIrp },
    { KseHookInvalid },
};
static KSE_HOOK_COLLECTION KmDsShimCollections[] = {
    { KseCollectionNtExport, NULL, KmDsShimHooksNT },
    { KseCollectionCallback, NULL, KmDsShimHooksCB },
    { KseCollectionInvalid },
};

DEFINE_GUID(KmDsShimGuid, 0xbc04ab45, 0xea7e, 0x4a11, 0xa7, 0xbb, 0x97, 0x76, 0x15, 0xf4, 0xca, 0xae);

KSE_SHIM KmDsShim = { sizeof(KSE_SHIM), &KmDsShimGuid, L"DriverScope", NULL, NULL, NULL, KmDsShimCollections };

NTSTATUS
NTAPI
KseDriverScopeInitialize(VOID)
{
    NTSTATUS Status;

    Status = KseRegisterShim(&KmDsShim, NULL, 0);
    if (NT_SUCCESS(Status))
        DPRINT("KSE: driver scope shim registered.\n");

    return Status;
}
