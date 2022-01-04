/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxGeneralKm.h

Abstract:

    Kernel mode implementation for general OS
    functions defined in MxGeneral.h

Author:



Revision History:



--*/

#pragma once

#define MAKE_MX_FUNC_NAME(x)    L##x
typedef LPCWSTR                 MxFuncName;
typedef PKTHREAD                MxThread;
typedef PETHREAD                MdEThread;
typedef PDEVICE_OBJECT          MdDeviceObject;
typedef PDRIVER_OBJECT          MdDriverObject;
typedef PFILE_OBJECT            MdFileObject;
typedef PIO_REMOVE_LOCK         MdRemoveLock;
typedef PCALLBACK_OBJECT        MdCallbackObject;
typedef CALLBACK_FUNCTION       MdCallbackFunctionType, *MdCallbackFunction;
typedef PKINTERRUPT             MdInterrupt;
typedef KSERVICE_ROUTINE        MdInterruptServiceRoutineType, *MdInterruptServiceRoutine;
typedef KSYNCHRONIZE_ROUTINE    MdInterruptSynchronizeRoutineType, *MdInterruptSynchronizeRoutine;

#include "mxgeneral.h"
#include <ntstrsafe.h>

__inline
BOOLEAN
Mx::IsUM(
    )
{
    return FALSE;
}

__inline
BOOLEAN
Mx::IsKM(
    )
{
    return TRUE;
}

__inline
MxThread
Mx::MxGetCurrentThread(
    )
{
    return KeGetCurrentThread();
}

__inline
MdEThread
Mx::GetCurrentEThread(
    )
{
    return PsGetCurrentThread();
}

__inline
NTSTATUS
Mx::MxTerminateCurrentThread(
    __in NTSTATUS Status
    )
{
    return PsTerminateSystemThread(Status);
}

__inline
KIRQL
Mx::MxGetCurrentIrql(
    )
{
    return KeGetCurrentIrql();
}

__drv_maxIRQL(HIGH_LEVEL)
__drv_raisesIRQL(NewIrql)
__inline
VOID
Mx::MxRaiseIrql(
    __in KIRQL                              NewIrql,
    __out __deref __drv_savesIRQL PKIRQL    OldIrql
    )
{
    KeRaiseIrql(NewIrql, OldIrql);
}

__drv_maxIRQL(HIGH_LEVEL)
__inline
VOID
Mx::MxLowerIrql(
    __in __drv_restoresIRQL __drv_nonConstant KIRQL NewIrql
    )
{
    KeLowerIrql(NewIrql);
}

__inline
VOID
Mx::MxQueryTickCount(
    __out PLARGE_INTEGER  TickCount
    )
{
    KeQueryTickCount(TickCount);
}

__inline
ULONG
Mx::MxQueryTimeIncrement(
    )
{
    return KeQueryTimeIncrement();
}

__inline
VOID
Mx::MxBugCheckEx(
    __in ULONG  BugCheckCode,
    __in ULONG_PTR  BugCheckParameter1,
    __in ULONG_PTR  BugCheckParameter2,
    __in ULONG_PTR  BugCheckParameter3,
    __in ULONG_PTR  BugCheckParameter4
)
{
#ifdef _MSC_VER
    #pragma prefast(suppress:__WARNING_USE_OTHER_FUNCTION, "KeBugCheckEx is the intent.");
#endif
    KeBugCheckEx(
        BugCheckCode,
        BugCheckParameter1,
        BugCheckParameter2,
        BugCheckParameter3,
        BugCheckParameter4
        );

    UNREACHABLE;
}

__inline
VOID
Mx::MxDbgBreakPoint(
    )
{
    DbgBreakPoint();
}

__inline
VOID
Mx::MxAssert(
    __in BOOLEAN Condition
    )
{
    UNREFERENCED_PARAMETER(Condition);

    ASSERT(Condition); //this get defined as RtlAssert
}

__inline
VOID
Mx::MxAssertMsg(
    __in LPSTR Message,
    __in BOOLEAN Condition
    )
{
    UNREFERENCED_PARAMETER(Message);
    UNREFERENCED_PARAMETER(Condition);

    ASSERT(Condition);

    // ASSERTMSG(Message, Condition); TODO: wtf
}

_Acquires_lock_(_Global_critical_region_)
__inline
VOID
Mx::MxEnterCriticalRegion(
    )
{
    KeEnterCriticalRegion();
}

_Releases_lock_(_Global_critical_region_)
__inline
VOID
Mx::MxLeaveCriticalRegion(
    )
{
    KeLeaveCriticalRegion();
}

__inline
VOID
Mx::MxDelayExecutionThread(
    __in KPROCESSOR_MODE  WaitMode,
    __in BOOLEAN  Alertable,
    __in PLARGE_INTEGER  Interval
    )
{
    ASSERTMSG("Interval must be relative\n", Interval->QuadPart <= 0);

    KeDelayExecutionThread(
        WaitMode,
        Alertable,
        Interval
        );
}

__inline
PVOID
Mx::MxGetSystemRoutineAddress(
    __in MxFuncName FuncName
    )
{
    UNICODE_STRING funcName;

    RtlInitUnicodeString(&funcName, FuncName);
    return MmGetSystemRoutineAddress(&funcName);
}

__inline
VOID
Mx::MxReferenceObject(
    __in PVOID Object
    )
{
    ObReferenceObject(Object);
}

__inline
VOID
Mx::MxDereferenceObject(
    __in PVOID Object
    )
{
    ObDereferenceObject(Object);
}

__inline
VOID
Mx::MxInitializeRemoveLock(
    __in MdRemoveLock  Lock,
    __in ULONG  AllocateTag,
    __in ULONG  MaxLockedMinutes,
    __in ULONG  HighWatermark
    )
{
    IoInitializeRemoveLock(Lock, AllocateTag, MaxLockedMinutes, HighWatermark);
}

__inline
NTSTATUS
Mx::MxAcquireRemoveLock(
    __in MdRemoveLock  RemoveLock,
    __in_opt PVOID  Tag
    )
{
    return IoAcquireRemoveLock(RemoveLock, Tag);
}

__inline
VOID
Mx::MxReleaseRemoveLock(
    __in MdRemoveLock  RemoveLock,
    __in PVOID  Tag
    )
{
    IoReleaseRemoveLock(RemoveLock, Tag);
}

__inline
VOID
Mx::MxReleaseRemoveLockAndWait(
    __in MdRemoveLock  RemoveLock,
    __in PVOID  Tag
    )
{
    IoReleaseRemoveLockAndWait(RemoveLock, Tag);
}

__inline
BOOLEAN
Mx::MxHasEnoughRemainingThreadStack(
    VOID
    )
{
    return (IoGetRemainingStackSize() < KERNEL_STACK_SIZE/2) ? FALSE : TRUE;
}

_Releases_lock_(_Global_cancel_spin_lock_)
__drv_requiresIRQL(DISPATCH_LEVEL)
__inline
VOID
Mx::ReleaseCancelSpinLock(
    __in __drv_restoresIRQL __drv_useCancelIRQL  KIRQL  Irql
    )
{
    IoReleaseCancelSpinLock(Irql);
}

__inline
NTSTATUS
Mx::CreateCallback(
    __out PCALLBACK_OBJECT  *CallbackObject,
    __in POBJECT_ATTRIBUTES  ObjectAttributes,
    __in BOOLEAN  Create,
    __in BOOLEAN  AllowMultipleCallbacks
    )
{
    return ExCreateCallback(
        CallbackObject,
        ObjectAttributes,
        Create,
        AllowMultipleCallbacks);
}

__inline
PVOID
Mx::RegisterCallback(
    __in PCALLBACK_OBJECT  CallbackObject,
    __in MdCallbackFunction  CallbackFunction,
    __in PVOID  CallbackContext
    )
{
    return ExRegisterCallback(
        CallbackObject,
        CallbackFunction,
        CallbackContext);
}

__inline
VOID
Mx::UnregisterCallback(
    __in PVOID  CbRegistration
    )
{
    ExUnregisterCallback(CbRegistration);
}

__inline
VOID
Mx::MxUnlockPages(
    __in PMDL Mdl
    )
{
    MmUnlockPages(Mdl);
}

__inline
PVOID
Mx::MxGetSystemAddressForMdlSafe(
    __inout PMDL Mdl,
    __in    ULONG Priority
    )
{
    return MmGetSystemAddressForMdlSafe(Mdl, Priority);
}

__inline
VOID
Mx::MxBuildMdlForNonPagedPool(
    __inout PMDL Mdl
    )
{
    MmBuildMdlForNonPagedPool(Mdl);
}

__inline
PVOID
Mx::MxGetDriverObjectExtension(
    __in PDRIVER_OBJECT DriverObject,
    __in PVOID ClientIdentificationAddress
    )
{
    return IoGetDriverObjectExtension(DriverObject,
                                        ClientIdentificationAddress);
}

__inline
NTSTATUS
Mx::MxAllocateDriverObjectExtension(
    _In_  MdDriverObject DriverObject,
    _In_  PVOID ClientIdentificationAddress,
    _In_  ULONG DriverObjectExtensionSize,
    // When successful, this always allocates already-aliased memory.
    _Post_ _At_(*DriverObjectExtension, _When_(return==0,
    __drv_aliasesMem __drv_allocatesMem(Mem) _Post_notnull_))
    _When_(return == 0, _Outptr_result_bytebuffer_(DriverObjectExtensionSize))
    PVOID *DriverObjectExtension
    )
{
    return IoAllocateDriverObjectExtension(DriverObject,
                                            ClientIdentificationAddress,
                                            DriverObjectExtensionSize,
                                            DriverObjectExtension);
}

__inline
MdDeviceObject
Mx::MxGetAttachedDeviceReference(
    __in PDEVICE_OBJECT DriverObject
    )
{
    return IoGetAttachedDeviceReference(DriverObject);
}

__inline
VOID
Mx::MxDeleteSymbolicLink(
    __in PUNICODE_STRING Value
    )
{
    IoDeleteSymbolicLink(Value);
}

__inline
VOID
Mx::MxDeleteNPagedLookasideList(
    _In_ PNPAGED_LOOKASIDE_LIST LookasideList
    )
{
    ExDeleteNPagedLookasideList(LookasideList);
}

__inline
VOID
Mx::MxDeletePagedLookasideList(
    _In_ PPAGED_LOOKASIDE_LIST LookasideList
    )
{
    ExDeletePagedLookasideList(LookasideList);
}

__inline
VOID
Mx::MxInitializeNPagedLookasideList(
    _Out_     PNPAGED_LOOKASIDE_LIST Lookaside,
    _In_opt_  PALLOCATE_FUNCTION Allocate,
    _In_opt_  PFREE_FUNCTION Free,
    _In_      ULONG Flags,
    _In_      SIZE_T Size,
    _In_      ULONG Tag,
    _In_      USHORT Depth
    )
{
    ExInitializeNPagedLookasideList(Lookaside,
                                        Allocate,
                                        Free,
                                        Flags,
                                        Size,
                                        Tag,
                                        Depth);
}

__inline
VOID
Mx::MxInitializePagedLookasideList(
    _Out_     PPAGED_LOOKASIDE_LIST Lookaside,
    _In_opt_  PALLOCATE_FUNCTION Allocate,
    _In_opt_  PFREE_FUNCTION Free,
    _In_      ULONG Flags,
    _In_      SIZE_T Size,
    _In_      ULONG Tag,
    _In_      USHORT Depth
    )
{
    ExInitializePagedLookasideList(Lookaside,
                                        Allocate,
                                        Free,
                                        Flags,
                                        Size,
                                        Tag,
                                        Depth);
}

__inline
VOID
Mx::MxDeleteDevice(
    _In_ MdDeviceObject Device
    )
{
    IoDeleteDevice(Device);
}

__inline
VOID
Mx::MxDetachDevice(
    _Inout_ MdDeviceObject Device
    )
{
    IoDetachDevice(Device);
}

__inline
MdDeviceObject
Mx::MxAttachDeviceToDeviceStack(
    _In_ MdDeviceObject SourceDevice,
    _In_ MdDeviceObject TargetDevice
    )
{
    return IoAttachDeviceToDeviceStack(SourceDevice, TargetDevice);
}

__inline
NTSTATUS
Mx::MxCreateDeviceSecure(
      _In_      PDRIVER_OBJECT DriverObject,
      _In_      ULONG DeviceExtensionSize,
      _In_opt_  PUNICODE_STRING DeviceName,
      _In_      DEVICE_TYPE DeviceType,
      _In_      ULONG DeviceCharacteristics,
      _In_      BOOLEAN Exclusive,
      _In_      PCUNICODE_STRING DefaultSDDLString,
      _In_opt_  LPCGUID DeviceClassGuid,
      _Out_     MdDeviceObject *DeviceObject
    )
{
#ifndef __REACTOS__ // we don't have wdmsec.lib
    return IoCreateDeviceSecure(DriverObject,
                DeviceExtensionSize,
                DeviceName,
                DeviceType,
                DeviceCharacteristics,
                Exclusive,
                DefaultSDDLString,
                DeviceClassGuid,
                DeviceObject);
#else
    return IoCreateDevice(
                DriverObject,
                DeviceExtensionSize,
                DeviceName,
                DeviceType,
                DeviceCharacteristics,
                Exclusive,
                DeviceObject);
#endif
}

__inline
NTSTATUS
Mx::MxCreateDevice(
    _In_      PDRIVER_OBJECT DriverObject,
    _In_      ULONG DeviceExtensionSize,
    _In_opt_  PUNICODE_STRING DeviceName,
    _In_      DEVICE_TYPE DeviceType,
    _In_      ULONG DeviceCharacteristics,
    _In_      BOOLEAN Exclusive,
    _Out_     MdDeviceObject *DeviceObject
)
{
    return IoCreateDevice(DriverObject,
                    DeviceExtensionSize,
                    DeviceName,
                    DeviceType,
                    DeviceCharacteristics,
                    Exclusive,
                    DeviceObject);

}

__inline
NTSTATUS
Mx::MxCreateSymbolicLink(
    _In_ PUNICODE_STRING SymbolicLinkName,
    _In_ PUNICODE_STRING DeviceName
    )
{
    return IoCreateSymbolicLink(SymbolicLinkName, DeviceName);
}

__inline
VOID
Mx::MxFlushQueuedDpcs(
    )
{
    KeFlushQueuedDpcs();
}

__inline
NTSTATUS
Mx::MxOpenKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    return ZwOpenKey(KeyHandle, DesiredAccess, ObjectAttributes);
}

__inline
NTSTATUS
Mx::MxSetDeviceInterfaceState(
    _In_ PUNICODE_STRING SymbolicLinkName,
    _In_ BOOLEAN Enable
    )
{
    return IoSetDeviceInterfaceState(SymbolicLinkName, Enable);
}


__inline
NTSTATUS
Mx::MxRegisterDeviceInterface(
    _In_      PDEVICE_OBJECT PhysicalDeviceObject,
    _In_      const GUID *InterfaceClassGuid,
    _In_opt_  PUNICODE_STRING ReferenceString,
    _Out_     PUNICODE_STRING SymbolicLinkName
    )
{
    return IoRegisterDeviceInterface(PhysicalDeviceObject,
                                     InterfaceClassGuid,
                                     ReferenceString,
                                     SymbolicLinkName);
}

__inline
NTSTATUS
Mx::MxDeleteKey(
    _In_ HANDLE KeyHandle
    )
{
    return ZwDeleteKey(KeyHandle);
}

__inline
VOID
Mx::MxInitializeMdl(
    _In_  PMDL MemoryDescriptorList,
    _In_  PVOID BaseVa,
    _In_  SIZE_T Length
    )
{
    MmInitializeMdl(MemoryDescriptorList, BaseVa, Length);
}

__inline
PVOID
Mx::MxGetMdlVirtualAddress(
    _In_ PMDL Mdl
    )
{
    return MmGetMdlVirtualAddress(Mdl);
}

__inline
VOID
Mx::MxBuildPartialMdl(
    _In_     PMDL SourceMdl,
    _Inout_  PMDL TargetMdl,
    _In_     PVOID VirtualAddress,
    _In_     ULONG Length
    )
{
    IoBuildPartialMdl(SourceMdl,
                        TargetMdl,
                        VirtualAddress,
                        Length
                        );
}

__inline
VOID
Mx::MxQuerySystemTime(
    _Out_ PLARGE_INTEGER CurrentTime
    )
{
    KeQuerySystemTime(CurrentTime);
}

__inline
NTSTATUS
Mx::MxSetValueKey(
    _In_      HANDLE KeyHandle,
    _In_      PUNICODE_STRING ValueName,
    _In_opt_  ULONG TitleIndex,
    _In_      ULONG Type,
    _In_opt_  PVOID Data,
    _In_      ULONG DataSize
    )
{
    return ZwSetValueKey(KeyHandle,
                          ValueName,
                          TitleIndex,
                          Type,
                          Data,
                          DataSize
                          );
}

__inline
NTSTATUS
Mx::MxQueryValueKey(
    _In_       HANDLE KeyHandle,
    _In_       PUNICODE_STRING ValueName,
    _In_       KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    _Out_opt_  PVOID KeyValueInformation,
    _In_       ULONG Length,
    _Out_      PULONG ResultLength
)
{
    return ZwQueryValueKey(KeyHandle,
                            ValueName,
                            KeyValueInformationClass,
                            KeyValueInformation,
                            Length,
                            ResultLength
                            );
}

__inline
NTSTATUS
Mx::MxReferenceObjectByHandle(
    __in HANDLE Handle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_TYPE ObjectType,
    __in KPROCESSOR_MODE AccessMode,
    __out  PVOID *Object,
    __out_opt POBJECT_HANDLE_INFORMATION HandleInformation
    )
{
    return ObReferenceObjectByHandle(
                Handle,
                DesiredAccess,
                ObjectType,
                AccessMode,
                Object,
                HandleInformation);
}

__inline
NTSTATUS
Mx::MxUnRegisterPlugPlayNotification(
    __in __drv_freesMem(Pool) PVOID NotificationEntry
    )
{
    return IoUnregisterPlugPlayNotification(NotificationEntry);
}


__inline
NTSTATUS
Mx::MxClose(
    __in HANDLE Handle
    )
{
    return ZwClose(Handle);
}

__inline
KIRQL
Mx::MxAcquireInterruptSpinLock(
    _Inout_ PKINTERRUPT Interrupt
    )
{
    return KeAcquireInterruptSpinLock(Interrupt);
}

__inline
VOID
Mx::MxReleaseInterruptSpinLock(
    _Inout_ PKINTERRUPT Interrupt,
    _In_ KIRQL OldIrql
    )
{
    KeReleaseInterruptSpinLock(Interrupt, OldIrql);
}

__inline
BOOLEAN
Mx::MxInsertQueueDpc(
  __inout   PRKDPC Dpc,
  __in_opt  PVOID SystemArgument1,
  __in_opt  PVOID SystemArgument2
)
{
    return KeInsertQueueDpc(Dpc, SystemArgument1, SystemArgument2);
}


