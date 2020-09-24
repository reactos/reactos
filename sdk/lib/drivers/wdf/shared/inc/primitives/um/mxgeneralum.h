/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxGeneralUm.h

Abstract:

    User mode implementation for general OS
    functions defined in MxGeneral.h

Author:



Revision History:



--*/

#pragma once

#define MAKE_MX_FUNC_NAME(x)    x








#define REMOVE_LOCK_RELEASE_TIMEOUT_IN_SECONDS 45

typedef VOID THREADPOOL_WAIT_CALLBACK (
    __inout     PTP_CALLBACK_INSTANCE Instance,
    __inout_opt PVOID                 Context,
    __inout     PTP_WAIT              Wait,
    __in        TP_WAIT_RESULT        WaitResult
    );

typedef THREADPOOL_WAIT_CALLBACK MdInterruptServiceRoutineType, *MdInterruptServiceRoutine;

typedef
BOOLEAN
InterruptSynchronizeRoutine (
    __in PVOID SynchronizeContext
    );

typedef InterruptSynchronizeRoutine MdInterruptSynchronizeRoutineType, *MdInterruptSynchronizeRoutine;

typedef struct _CALLBACK_OBJECT *PCALLBACK_OBJECT;

typedef
VOID
CALLBACK_FUNCTION(
        __in PVOID CallbackContext,
        __in_opt PVOID Argument1,
        __in_opt PVOID Argument2
        );

typedef CALLBACK_FUNCTION       MdCallbackFunctionType, *MdCallbackFunction;

//
// Define PnP notification event categories
//

typedef enum _IO_NOTIFICATION_EVENT_CATEGORY {
    EventCategoryReserved,
    EventCategoryHardwareProfileChange,
    EventCategoryDeviceInterfaceChange,
    EventCategoryTargetDeviceChange
} IO_NOTIFICATION_EVENT_CATEGORY;

#include "MxGeneral.h"

__inline
BOOLEAN
Mx::IsUM(
    )
{
    return TRUE;
}

__inline
BOOLEAN
Mx::IsKM(
    )
{
    return FALSE;
}

__inline
MxThread
Mx::MxGetCurrentThread(
    )
{
    //
    // We can't use GetCurrentThread as it returns a pseudo handle
    // which would have same numeric value for different threads
    // We could use DuplicateHandle to get real handle but that has the
    // following problems:
    //    1) It returns different handle values for the same thread
    //       if called again without closing handle.
    //    2) Makes the caller call CloseHandle making it inconvenient to
    //       call this function just to get an identifier for the thread
    //    3) More expensive than GetCurrentThreadId
    //
    // Since framework uses the thread only for comparison, logging
    // purposes GetCurrentThreadId works well.
    // It is cast to PVOID to match the pointer type PKTHREAD otherwise
    // trace functions complain of data type mismatch
    //

    return (PVOID) ::GetCurrentThreadId();
}

__inline
MdEThread
Mx::GetCurrentEThread(
    )
{
    //
    // See comments in MxGetCurrentThread.
    //
    return (PVOID) MxGetCurrentThread();
}

__inline
NTSTATUS
Mx::MxTerminateCurrentThread(
    __in NTSTATUS Status
    )
{
    #pragma prefast(suppress:__WARNING_USINGTERMINATETHREAD, "TerminateThread is the intent.");
    if (!TerminateThread(::GetCurrentThread(), HRESULT_FROM_NT(Status))) {
        DWORD err = GetLastError();
        return WinErrorToNtStatus(err);
    }
    return STATUS_SUCCESS;
}

__inline
KIRQL
Mx::MxGetCurrentIrql(
    )
{
    return PASSIVE_LEVEL;
}

__inline
VOID
#pragma prefast(suppress:__WARNING_UNMATCHED_DECL_ANNO, "Can't apply kernel mode annotations.");
Mx::MxRaiseIrql(
    __in KIRQL  NewIrql,
    __out PKIRQL  OldIrql
    )
{
    UNREFERENCED_PARAMETER(NewIrql);
    UNREFERENCED_PARAMETER(OldIrql);

    DO_NOTHING();
}

__inline
VOID
#pragma prefast(suppress:__WARNING_UNMATCHED_DECL_ANNO, "Can't apply kernel mode annotations.");
Mx::MxLowerIrql(
    __in KIRQL  NewIrql
    )
{
    UNREFERENCED_PARAMETER(NewIrql);

    DO_NOTHING();
}

__inline
VOID
Mx::MxQueryTickCount(
    __out PLARGE_INTEGER  TickCount
    )
{
    TickCount->QuadPart = GetTickCount();
}

__inline
ULONG
Mx::MxQueryTimeIncrement(
    )
{
    //
    // The way to get absolute time is TickCount * TimeIncrement.
    // In UM, TickCount is expressed in miliseconds, so this
    // conversion ensures that absolute time is expressed
    // in 100ns units as it is in KM.
    //
    return 1000 * 10;
}

__inline
VOID
Mx::MxDbgBreakPoint(
    )
{
    DebugBreak();
}

__inline
VOID
Mx::MxAssert(
    __in BOOLEAN Condition
    )
{
    if (!Condition)
    {



        DebugBreak();
    }
}

__inline
VOID
Mx::MxAssertMsg(
    __in LPSTR Message,
    __in BOOLEAN Condition
    )
{
    UNREFERENCED_PARAMETER(Message);

    if (!Condition)
    {



        DebugBreak();
    }
}

__inline
VOID
#pragma prefast(suppress:__WARNING_UNMATCHED_DEFN, "Can't apply kernel mode annotations.");
Mx::MxEnterCriticalRegion(
    )
{





    // DO_NOTHING();
}

__inline
VOID
#pragma prefast(suppress:__WARNING_UNMATCHED_DEFN, "Can't apply kernel mode annotations.");
Mx::MxLeaveCriticalRegion(
    )
{





    // DO_NOTHING();
}

__inline
VOID
Mx::MxDelayExecutionThread(
    __in KPROCESSOR_MODE  WaitMode,
    __in BOOLEAN  Alertable,
    __in PLARGE_INTEGER  Interval
    )
{
    UNREFERENCED_PARAMETER(WaitMode);
    ASSERTMSG("Interval must be relative\n", Interval->QuadPart <= 0);

    LARGE_INTEGER intervalMillisecond;

    //
    // This function uses KeDelayExecutionThread's contract, where relative
    // intervals are negative numbers expressed in 100ns units. We must
    // flip the sign and convert to ms units before calling SleepEx.
    //
    intervalMillisecond.QuadPart = -1 * Interval->QuadPart;
    intervalMillisecond.QuadPart /= 10 * 1000;

    SleepEx((DWORD)intervalMillisecond.QuadPart, Alertable);
}

__inline
PVOID
Mx::MxGetSystemRoutineAddress(
    __in MxFuncName FuncName
    )
/*++
Description:

    This function is meant to be called only by mode agnostic code
    System routine is assumed to be in ntdll.dll.

    This is because system routines (Rtl*) that can be used both
    in kernel mode as well as user mode reside in ntdll.dll.
    Kernel32.dll contains the user mode only Win32 API.

Arguments:

    MxFuncName FuncName -

Return Value:

    NTSTATUS Status code.
--*/
{
    HMODULE hMod;

    hMod = GetModuleHandleW(L"ntdll.dll");

    return GetProcAddress(hMod, FuncName);
}

__inline
VOID
Mx::MxReferenceObject(
    __in PVOID Object
    )
{
    UNREFERENCED_PARAMETER(Object);






    // DO_NOTHING();
}

__inline
VOID
Mx::MxDereferenceObject(
    __in PVOID Object
    )
{
    UNREFERENCED_PARAMETER(Object);






    // DO_NOTHING();
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
    UNREFERENCED_PARAMETER(AllocateTag);
    UNREFERENCED_PARAMETER(MaxLockedMinutes);
    UNREFERENCED_PARAMETER(HighWatermark);

    ZeroMemory(Lock, sizeof(*Lock));
    Lock->IoCount = 1;
    Lock->Removed = FALSE;
    Lock->RemoveEvent = NULL;
    Lock->ReleaseRemLockAndWaitStatus = (DWORD)-1;
}

__inline
NTSTATUS
Mx::MxAcquireRemoveLock(
    __in MdRemoveLock  RemoveLock,
    __in_opt PVOID  Tag
    )
{
    UNREFERENCED_PARAMETER(Tag);
    LONG lockValue;
    NTSTATUS status;

    lockValue = InterlockedIncrement(&RemoveLock->IoCount);

    ASSERT(lockValue > 0);

    if (! RemoveLock->Removed) {
        return STATUS_SUCCESS;
    }
    else {
        if (0 == InterlockedDecrement(&RemoveLock->IoCount)) {
            if (! SetEvent(RemoveLock->RemoveEvent)) {
                Mx::MxBugCheckEx(WDF_VIOLATION,
                                0, 0, 0, 0);
            }
        }
        status = STATUS_DELETE_PENDING;
    }

    return status;
}

__inline
VOID
Mx::MxReleaseRemoveLock(
    __in MdRemoveLock  RemoveLock,
    __in PVOID  Tag
    )
{
    UNREFERENCED_PARAMETER(Tag);
    LONG lockValue;

    lockValue = InterlockedDecrement(&RemoveLock->IoCount);

    ASSERT(0 <= lockValue);

    if (0 == lockValue) {
        ASSERT (RemoveLock->Removed);

        //
        // The device needs to be removed.  Signal the remove event
        // that it's safe to go ahead.
        //
        if (! SetEvent(RemoveLock->RemoveEvent)) {
            Mx::MxBugCheckEx(WDF_VIOLATION,
                            0, 0, 0, 0);
        }
    }
}

__inline
VOID
Mx::MxReleaseRemoveLockAndWait(
    __in MdRemoveLock  RemoveLock,
    __in PVOID  Tag
    )
{
    UNREFERENCED_PARAMETER(Tag);
    LONG ioCount;
    DWORD retVal = ERROR_SUCCESS;

    RemoveLock->Removed = TRUE;

    ioCount = InterlockedDecrement (&RemoveLock->IoCount);
    ASSERT(0 < ioCount);

    if (0 < InterlockedDecrement (&RemoveLock->IoCount)) {
        retVal = WaitForSingleObject(RemoveLock->RemoveEvent,
                        REMOVE_LOCK_RELEASE_TIMEOUT_IN_SECONDS*1000);
        ASSERT(retVal == WAIT_OBJECT_0);
    }

    // This only serves as a debugging aid.
    RemoveLock->ReleaseRemLockAndWaitStatus = retVal;
}

__inline
BOOLEAN
Mx::MxHasEnoughRemainingThreadStack(
    VOID
    )
{




    //
    // Thread stack is not so scarce in UM so return TRUE always
    //
    return TRUE;
}

__inline
VOID
#pragma prefast(suppress:__WARNING_UNMATCHED_DECL_ANNO, "Can't apply kernel mode annotations.");
Mx::ReleaseCancelSpinLock(
    __in KIRQL  Irql
    )
{
    UNREFERENCED_PARAMETER(Irql);

    //
    // UMDF Host doesn't have cancel spinlock equivalent concept so do nothing.
    //
    DO_NOTHING();
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
    UNREFERENCED_PARAMETER(CallbackObject);
    UNREFERENCED_PARAMETER(ObjectAttributes);
    UNREFERENCED_PARAMETER(Create);
    UNREFERENCED_PARAMETER(AllowMultipleCallbacks);

    return STATUS_UNSUCCESSFUL;
}

__inline
PVOID
Mx::RegisterCallback(
    __in PCALLBACK_OBJECT  CallbackObject,
    __in MdCallbackFunction  CallbackFunction,
    __in PVOID  CallbackContext
    )
{
    UNREFERENCED_PARAMETER(CallbackObject);
    UNREFERENCED_PARAMETER(CallbackFunction);
    UNREFERENCED_PARAMETER(CallbackContext);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return NULL;
}

__inline
VOID
Mx::UnregisterCallback(
    __in PVOID  CbRegistration
    )
{
    UNREFERENCED_PARAMETER(CbRegistration);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

__inline
VOID
Mx::MxUnlockPages(
    __in PMDL Mdl
    )
{
    UNREFERENCED_PARAMETER(Mdl);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

__inline
PVOID
Mx::MxGetSystemAddressForMdlSafe(
    __inout PMDL Mdl,
    __in    ULONG Priority
    )
{
    UNREFERENCED_PARAMETER(Mdl);
    UNREFERENCED_PARAMETER(Priority);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return NULL;
}

__inline
VOID
Mx::MxBuildMdlForNonPagedPool(
    __inout PMDL Mdl
    )
{
    UNREFERENCED_PARAMETER(Mdl);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

__inline
PVOID
Mx::MxGetDriverObjectExtension(
    __in MdDriverObject DriverObject,
    __in PVOID ClientIdentificationAddress
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(ClientIdentificationAddress);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return NULL;
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
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(ClientIdentificationAddress);
    UNREFERENCED_PARAMETER(DriverObjectExtensionSize);
    UNREFERENCED_PARAMETER(DriverObjectExtension);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_UNSUCCESSFUL;
}

__inline
MdDeviceObject
Mx::MxGetAttachedDeviceReference(
    __in MdDeviceObject DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return NULL;
}

__inline
VOID
Mx::MxDeleteSymbolicLink(
    __in PUNICODE_STRING Value
    )
{
    UNREFERENCED_PARAMETER(Value);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

__inline
VOID
Mx::MxDeleteNPagedLookasideList(
    _In_ PNPAGED_LOOKASIDE_LIST LookasideList
    )
{
    UNREFERENCED_PARAMETER(LookasideList);
}

__inline
VOID
Mx::MxDeletePagedLookasideList(
    _In_ PPAGED_LOOKASIDE_LIST LookasideList
    )
{
    UNREFERENCED_PARAMETER(LookasideList);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
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

    UNREFERENCED_PARAMETER(Lookaside);
    UNREFERENCED_PARAMETER(Allocate);
    UNREFERENCED_PARAMETER(Free);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(Size);
    UNREFERENCED_PARAMETER(Tag);
    UNREFERENCED_PARAMETER(Depth);

    //ASSERTMSG("Not implemented for UMDF\n", FALSE);

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

    UNREFERENCED_PARAMETER(Lookaside);
    UNREFERENCED_PARAMETER(Allocate);
    UNREFERENCED_PARAMETER(Free);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(Size);
    UNREFERENCED_PARAMETER(Tag);
    UNREFERENCED_PARAMETER(Depth);

    //ASSERTMSG("Not implemented for UMDF\n", FALSE);

}

__inline
VOID
Mx::MxDeleteDevice(
    _In_ MdDeviceObject Device
    )
{
    UNREFERENCED_PARAMETER(Device);




    //
    // Host's device stack object holds the only reference to the host devices.
    // The infrastructure controls the device object's lifetime.
    //
    DO_NOTHING();
}

__inline
NTSTATUS
Mx::MxCreateDeviceSecure(
      _In_      MdDriverObject DriverObject,
      _In_      ULONG DeviceExtensionSize,
      _In_opt_  PUNICODE_STRING DeviceName,
      _In_      DEVICE_TYPE DeviceType,
      _In_      ULONG DeviceCharacteristics,
      _In_      BOOLEAN Exclusive,
      _In_      PCUNICODE_STRING DefaultSDDLString,
      _In_opt_  LPCGUID DeviceClassGuid,
      _Out_opt_     MdDeviceObject *DeviceObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(DeviceExtensionSize);
    UNREFERENCED_PARAMETER(DeviceName);
    UNREFERENCED_PARAMETER(DeviceType);
    UNREFERENCED_PARAMETER(Exclusive);
    UNREFERENCED_PARAMETER(DeviceCharacteristics);
    UNREFERENCED_PARAMETER(DefaultSDDLString);
    UNREFERENCED_PARAMETER(DeviceClassGuid);
    UNREFERENCED_PARAMETER(DeviceObject);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_SUCCESS;
}

__inline
MdDeviceObject
Mx::MxAttachDeviceToDeviceStack(
    _In_ MdDeviceObject SourceDevice,
    _In_ MdDeviceObject TargetDevice
    )
{

    UNREFERENCED_PARAMETER(SourceDevice);
    UNREFERENCED_PARAMETER(TargetDevice);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return NULL;
}

__inline
NTSTATUS
Mx::MxCreateDevice(
    _In_      MdDriverObject DriverObject,
    _In_      ULONG DeviceExtensionSize,
    _In_opt_  PUNICODE_STRING DeviceName,
    _In_      DEVICE_TYPE DeviceType,
    _In_      ULONG DeviceCharacteristics,
    _In_      BOOLEAN Exclusive,
    _Out_opt_     MdDeviceObject *DeviceObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(DeviceExtensionSize);
    UNREFERENCED_PARAMETER(DeviceName);
    UNREFERENCED_PARAMETER(DeviceType);
    UNREFERENCED_PARAMETER(DeviceCharacteristics);
    UNREFERENCED_PARAMETER(Exclusive);
    UNREFERENCED_PARAMETER(DeviceObject);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_SUCCESS;

}

__inline
NTSTATUS
Mx::MxCreateSymbolicLink(
    _In_ PUNICODE_STRING SymbolicLinkName,
    _In_ PUNICODE_STRING DeviceName
    )
{
    UNREFERENCED_PARAMETER(SymbolicLinkName);
    UNREFERENCED_PARAMETER(DeviceName);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

__inline
VOID
Mx::MxFlushQueuedDpcs(
    )
{
    //
    // Not supported for UMDF
    //
}

__inline
NTSTATUS
Mx::MxOpenKey(
    _In_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    UNREFERENCED_PARAMETER(KeyHandle);
    UNREFERENCED_PARAMETER(DesiredAccess);
    UNREFERENCED_PARAMETER(ObjectAttributes);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

__inline
NTSTATUS
Mx::MxSetDeviceInterfaceState(
    _In_ PUNICODE_STRING SymbolicLinkName,
    _In_ BOOLEAN Enable
    )
{
    UNREFERENCED_PARAMETER(SymbolicLinkName);
    UNREFERENCED_PARAMETER(Enable);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
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
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);
    UNREFERENCED_PARAMETER(InterfaceClassGuid);
    UNREFERENCED_PARAMETER(ReferenceString);
    UNREFERENCED_PARAMETER(SymbolicLinkName);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

__inline
NTSTATUS
Mx::MxDeleteKey(
    _In_ HANDLE KeyHandle
    )

{
    UNREFERENCED_PARAMETER(KeyHandle);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

__inline
VOID
Mx::MxInitializeMdl(
    _In_  PMDL MemoryDescriptorList,
    _In_  PVOID BaseVa,
    _In_  SIZE_T Length
    )
{
    UNREFERENCED_PARAMETER(MemoryDescriptorList);
    UNREFERENCED_PARAMETER(BaseVa);
    UNREFERENCED_PARAMETER(Length);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

}

__inline
PVOID
Mx::MxGetMdlVirtualAddress(
    _In_ PMDL Mdl
    )
{
    UNREFERENCED_PARAMETER(Mdl);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return NULL;
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
    UNREFERENCED_PARAMETER(SourceMdl);
    UNREFERENCED_PARAMETER(TargetMdl);
    UNREFERENCED_PARAMETER(VirtualAddress);
    UNREFERENCED_PARAMETER(Length);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

__inline
VOID
Mx::MxQuerySystemTime(
    _Out_ PLARGE_INTEGER CurrentTime
    )
{
    UNREFERENCED_PARAMETER(CurrentTime);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
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
    UNREFERENCED_PARAMETER(KeyHandle);
    UNREFERENCED_PARAMETER(ValueName);
    UNREFERENCED_PARAMETER(TitleIndex);
    UNREFERENCED_PARAMETER(Type);
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(DataSize);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
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
    UNREFERENCED_PARAMETER(KeyHandle);
    UNREFERENCED_PARAMETER(ValueName);
    UNREFERENCED_PARAMETER(KeyValueInformationClass);
    UNREFERENCED_PARAMETER(KeyValueInformation);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(ResultLength);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

__inline
NTSTATUS
Mx::MxUnRegisterPlugPlayNotification(
    __in __drv_freesMem(Pool) PVOID NotificationEntry
    )
{
    UNREFERENCED_PARAMETER(NotificationEntry);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
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
    UNREFERENCED_PARAMETER(Handle);
    UNREFERENCED_PARAMETER(DesiredAccess);
    UNREFERENCED_PARAMETER(ObjectType);
    UNREFERENCED_PARAMETER(AccessMode);
    UNREFERENCED_PARAMETER(Object);
    UNREFERENCED_PARAMETER(HandleInformation);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

__inline
NTSTATUS
Mx::MxClose(
    __in HANDLE Handle
    )
{
    CloseHandle(Handle);

    return STATUS_SUCCESS;
}

__inline
KIRQL
Mx::MxAcquireInterruptSpinLock(
    _Inout_ PKINTERRUPT Interrupt
    )
{
    UNREFERENCED_PARAMETER(Interrupt);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
    return PASSIVE_LEVEL;
}

__inline
VOID
Mx::MxReleaseInterruptSpinLock(
    _Inout_ PKINTERRUPT Interrupt,
    _In_ KIRQL OldIrql
    )
{
    UNREFERENCED_PARAMETER(Interrupt);
    UNREFERENCED_PARAMETER(OldIrql);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

__inline
BOOLEAN
Mx::MxInsertQueueDpc(
  __inout   PRKDPC Dpc,
  __in_opt  PVOID SystemArgument1,
  __in_opt  PVOID SystemArgument2
)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
    return FALSE;
}

