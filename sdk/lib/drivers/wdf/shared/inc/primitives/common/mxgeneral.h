/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxGeneral.h

Abstract:

    Mode agnostic definitions for general OS
    functions used in framework code

    See MxGeneralKm.h and MxGeneralUm.h for mode
    specific implementations

Author:



Revision History:



--*/

#pragma once

//
// Placeholder macro for a no-op
//
#define DO_NOTHING()                            (0)

//
// We need to make these static functions of the class
// to force common definition to apply to um and km versions
//
// If we don't do this, um and km definitions can diverge
//
class Mx
{
public:

    //
    // IsUM/IsKM don't change at runtime
    // but defining them as functions makes it more convenient to check
    // for UM/KM as compared to using ifdef's in certain places
    //
    // Since they are forceinlined and return a constant value,
    // optimized code is no different than using an ifdef
    //
    // See FxPoolAllocator in WdfPool.cpp for example of such usage
    //
    __inline
    static
    BOOLEAN
    IsUM(
        );

    __inline
    static
    BOOLEAN
    IsKM(
        );

    __inline
    static
    MxThread
    MxGetCurrentThread(
        );

    __inline
    static
    MdEThread
    GetCurrentEThread(
        );

    NTSTATUS
    static
    MxTerminateCurrentThread(
        __in NTSTATUS Status
        );

    __inline
    static
    KIRQL
    MxGetCurrentIrql(
        );

    __drv_maxIRQL(HIGH_LEVEL)
    __drv_raisesIRQL(NewIrql)
    __inline
    static
    VOID
    MxRaiseIrql(
        __in KIRQL                              NewIrql,
        __out __deref __drv_savesIRQL PKIRQL    OldIrql
        );

    __drv_maxIRQL(HIGH_LEVEL)
    __inline
    static
    VOID
    MxLowerIrql(
        __in __drv_restoresIRQL __drv_nonConstant  KIRQL  NewIrql
        );

    __inline
    static
    VOID
    MxQueryTickCount(
        __out PLARGE_INTEGER  TickCount
        );

    __inline
    static
    ULONG
    MxQueryTimeIncrement(
        );








    static
    DECLSPEC_NORETURN
    VOID
    MxBugCheckEx(
        __in ULONG  BugCheckCode,
        __in ULONG_PTR  BugCheckParameter1,
        __in ULONG_PTR  BugCheckParameter2,
        __in ULONG_PTR  BugCheckParameter3,
        __in ULONG_PTR  BugCheckParameter4
    );

    __inline
    static
    VOID
    MxDbgBreakPoint(
        );

    static
    VOID
    MxDbgPrint(
        __drv_formatString(printf)
        __in PCSTR DebugMessage,
        ...
        );

    __inline
    static
    VOID
    MxAssert(
        __in BOOLEAN Condition
        );

    __inline
    static
    VOID
    MxAssertMsg(
        __in LPSTR Message,
        __in BOOLEAN Condition
        );

    _Acquires_lock_(_Global_critical_region_)
    __inline
    static
    VOID
    MxEnterCriticalRegion(
        );

    _Releases_lock_(_Global_critical_region_) //implies _Requires_lock_held_(_Global_critical_region_)
    __inline
    static
    VOID
    MxLeaveCriticalRegion(
        );

    __inline
    static
    VOID
    MxDelayExecutionThread(
        __in KPROCESSOR_MODE  WaitMode,
        __in BOOLEAN  Alertable,
        __in PLARGE_INTEGER  Interval
        );

    //
    // Mode agnostic function to get address of a system function
    // Should be used only for Rtl* functions applicable both to
    // kernel mode and user mode
    //
    // User mode version is assumed to reside in ntdll.dll
    //
    // The argument type is MxFuncName so that it can be defined
    // as LPCWSTR in kernel mode and LPCSTR in user mode
    // which is what MmGetSystemRoutineAddress and GetProcAddress
    // expect respectively
    //
    __inline
    static
    PVOID
    MxGetSystemRoutineAddress(
        __in MxFuncName FuncName
        );

    __inline
    static
    VOID
    MxReferenceObject(
        __in PVOID Object
        );

    __inline
    static
    VOID
    MxDereferenceObject(
        __in PVOID Object
        );

    __inline
    static
    VOID
    MxInitializeRemoveLock(
        __in MdRemoveLock  Lock,
        __in ULONG  AllocateTag,
        __in ULONG  MaxLockedMinutes,
        __in ULONG  HighWatermark
        );

    __inline
    static
    NTSTATUS
    MxAcquireRemoveLock(
        __in MdRemoveLock  RemoveLock,
        __in_opt PVOID  Tag
        );

    __inline
    static
    VOID
    MxReleaseRemoveLock(
        __in MdRemoveLock  RemoveLock,
        __in PVOID  Tag
        );

    __inline
    static
    VOID
    MxReleaseRemoveLockAndWait(
        __in MdRemoveLock  RemoveLock,
        __in PVOID  Tag
        );

    __inline
    static
    BOOLEAN
    MxHasEnoughRemainingThreadStack(
        VOID
        );


    _Releases_lock_(_Global_cancel_spin_lock_) //implies _Requires_lock_held_(_Global_cancel_spin_lock_)
    __drv_requiresIRQL(DISPATCH_LEVEL)
    __inline
    static
    VOID
    ReleaseCancelSpinLock(
        __in __drv_restoresIRQL __drv_useCancelIRQL KIRQL  Irql
        );

    __inline
    static
    NTSTATUS
    CreateCallback(
        __out PCALLBACK_OBJECT  *CallbackObject,
        __in POBJECT_ATTRIBUTES  ObjectAttributes,
        __in BOOLEAN  Create,
        __in BOOLEAN  AllowMultipleCallbacks
        );

    __inline
    static
    PVOID
    RegisterCallback(
        __in PCALLBACK_OBJECT  CallbackObject,
        __in MdCallbackFunction  CallbackFunction,
        __in PVOID  CallbackContext
        );

    __inline
    static
    VOID
    UnregisterCallback(
        __in PVOID  CbRegistration
        );

    static
    VOID
    MxGlobalInit(
        VOID
        );

    __inline
    static
    VOID
    MxUnlockPages(
        __in PMDL Mdl
        );

    __inline
    static
    PVOID
    MxGetSystemAddressForMdlSafe(
        __inout PMDL Mdl,
        __in    ULONG Priority
        );

    __inline
    static
    VOID
    MxBuildMdlForNonPagedPool(
        __inout PMDL Mdl
        );

    __inline
    static
    PVOID
    MxGetDriverObjectExtension(
        __in MdDriverObject DriverObject,
        __in PVOID ClientIdentificationAddress
        );

    __inline
    static
    NTSTATUS
    MxAllocateDriverObjectExtension(
        _In_  MdDriverObject DriverObject,
        _In_  PVOID ClientIdentificationAddress,
        _In_  ULONG DriverObjectExtensionSize,
        // When successful, this always allocates already-aliased memory.
        _Post_ _At_(*DriverObjectExtension, _When_(return==0,
        __drv_aliasesMem __drv_allocatesMem(Mem) _Post_notnull_))
        _When_(return == 0, _Outptr_result_bytebuffer_(DriverObjectExtensionSize))
        PVOID *DriverObjectExtension
        );

    __inline
    static
    MdDeviceObject
    MxGetAttachedDeviceReference(
        __in MdDeviceObject DriverObject
        );

    __inline
    static
    VOID
    MxDeleteSymbolicLink(
        __in PUNICODE_STRING Link
        );

    __inline
    static
    VOID
    MxDeleteNPagedLookasideList(
        _In_ PNPAGED_LOOKASIDE_LIST LookasideList
        );

    __inline
    static
    VOID
    MxDeletePagedLookasideList(
        _In_ PPAGED_LOOKASIDE_LIST LookasideList
        );

    __inline
    static
    VOID
    MxInitializeNPagedLookasideList(
        _Out_     PNPAGED_LOOKASIDE_LIST Lookaside,
        _In_opt_  PALLOCATE_FUNCTION Allocate,
        _In_opt_  PFREE_FUNCTION Free,
        _In_      ULONG Flags,
        _In_      SIZE_T Size,
        _In_      ULONG Tag,
        _In_      USHORT Depth
        );

    __inline
    static
    VOID
    MxInitializePagedLookasideList(
        _Out_     PPAGED_LOOKASIDE_LIST Lookaside,
        _In_opt_  PALLOCATE_FUNCTION Allocate,
        _In_opt_  PFREE_FUNCTION Free,
        _In_      ULONG Flags,
        _In_      SIZE_T Size,
        _In_      ULONG Tag,
        _In_      USHORT Depth
        );

    __inline
    static
    VOID
    MxDeleteDevice(
        _In_ MdDeviceObject Device
        );

    static
    VOID
    MxDetachDevice(
        _Inout_ MdDeviceObject Device
        );

    __inline
    static
    MdDeviceObject
    MxAttachDeviceToDeviceStack(
        _In_ MdDeviceObject SourceDevice,
        _In_ MdDeviceObject TargetDevice
        );

    __inline
    static
    NTSTATUS
    MxCreateDeviceSecure(
        _In_      MdDriverObject DriverObject,
        _In_      ULONG DeviceExtensionSize,
        _In_opt_  PUNICODE_STRING DeviceName,
        _In_      DEVICE_TYPE DeviceType,
        _In_      ULONG DeviceCharacteristics,
        _In_      BOOLEAN Exclusive,
        _In_      PCUNICODE_STRING DefaultSDDLString,
        _In_opt_  LPCGUID DeviceClassGuid,
        _Out_     MdDeviceObject *DeviceObject
        );

    __inline
    static
    NTSTATUS
    MxCreateDevice(
        _In_      MdDriverObject DriverObject,
        _In_      ULONG DeviceExtensionSize,
        _In_opt_  PUNICODE_STRING DeviceName,
        _In_      DEVICE_TYPE DeviceType,
        _In_      ULONG DeviceCharacteristics,
        _In_      BOOLEAN Exclusive,
        _Out_     MdDeviceObject *DeviceObject
    );

    __inline
    static
    NTSTATUS
    MxCreateSymbolicLink(
        _In_ PUNICODE_STRING SymbolicLinkName,
        _In_ PUNICODE_STRING DeviceName
        );

    __inline
    static
    VOID
    MxFlushQueuedDpcs(
        );

    __inline
    static
    NTSTATUS
    MxOpenKey(
        _Out_ PHANDLE KeyHandle,
        _In_ ACCESS_MASK DesiredAccess,
        _In_ POBJECT_ATTRIBUTES ObjectAttributes
        );

    __inline
    static
    NTSTATUS
    MxSetDeviceInterfaceState(
        _In_ PUNICODE_STRING SymbolicLinkName,
        _In_ BOOLEAN Enable
        );

    __inline
    static
    NTSTATUS
    MxRegisterDeviceInterface(
        _In_      PDEVICE_OBJECT PhysicalDeviceObject,
        _In_      const GUID *InterfaceClassGuid,
        _In_opt_  PUNICODE_STRING ReferenceString,
        _Out_     PUNICODE_STRING SymbolicLinkName
        );

    __inline
    static
    NTSTATUS
    MxDeleteKey(
        _In_ HANDLE KeyHandle
        );

    __inline
    static
    VOID
    MxInitializeMdl(
        _In_  PMDL MemoryDescriptorList,
        _In_  PVOID BaseVa,
        _In_  SIZE_T Length
        );

    __inline
    static
    PVOID
    MxGetMdlVirtualAddress(
        _In_ PMDL Mdl
        );

    __inline
    static
    VOID
    MxBuildPartialMdl(
        _In_     PMDL SourceMdl,
        _Inout_  PMDL TargetMdl,
        _In_     PVOID VirtualAddress,
        _In_     ULONG Length
    );

    __inline
    static
    VOID
    MxQuerySystemTime(
        _Out_ PLARGE_INTEGER CurrentTime
        );

    __inline
    static
    NTSTATUS
    MxSetValueKey(
        _In_      HANDLE KeyHandle,
        _In_      PUNICODE_STRING ValueName,
        _In_opt_  ULONG TitleIndex,
        _In_      ULONG Type,
        _In_opt_  PVOID Data,
        _In_      ULONG DataSize
        );

    __inline
    static
    NTSTATUS
    MxQueryValueKey(
        _In_       HANDLE KeyHandle,
        _In_       PUNICODE_STRING ValueName,
        _In_       KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
        _Out_opt_  PVOID KeyValueInformation,
        _In_       ULONG Length,
        _Out_      PULONG ResultLength
    );
    __inline
    static
    NTSTATUS
    MxReferenceObjectByHandle(
        __in HANDLE Handle,
        __in ACCESS_MASK DesiredAccess,
        __in_opt POBJECT_TYPE ObjectType,
        __in KPROCESSOR_MODE AccessMode,
        __out  PVOID *Object,
        __out_opt POBJECT_HANDLE_INFORMATION HandleInformation
        );

    __inline
    static
    NTSTATUS
    MxUnRegisterPlugPlayNotification(
        __in __drv_freesMem(Pool) PVOID NotificationEntry
        );

    __inline
    static
    NTSTATUS
    MxClose (
        __in HANDLE Handle
        );

    __inline
    static
    KIRQL
    MxAcquireInterruptSpinLock(
        _Inout_ PKINTERRUPT Interrupt
        );

    __inline
    static
    VOID
    MxReleaseInterruptSpinLock(
        _Inout_ PKINTERRUPT Interrupt,
        _In_ KIRQL OldIrql
        );

    __inline
    static
    BOOLEAN
    MxInsertQueueDpc(
      __inout   PRKDPC Dpc,
      __in_opt  PVOID SystemArgument1,
      __in_opt  PVOID SystemArgument2
    );
};
