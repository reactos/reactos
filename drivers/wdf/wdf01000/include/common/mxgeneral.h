#ifndef _MXGENERAL_H_
#define _MXGENERAL_H_

#include <ntddk.h>
#include <wdf.h>


//typedef EXT_CALLBACK MdExtCallbackType, *MdExtCallback;
typedef KDEFERRED_ROUTINE MdDeferredRoutineType, *MdDeferredRoutine;
#define FX_DEVICEMAP_PATH L"\\REGISTRY\\MACHINE\\HARDWARE\\DEVICEMAP\\"

//
// Placeholder macro for a no-op
//
#define DO_NOTHING()                            (0)

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

class Mx {

public:

    __inline
    static
    KIRQL
    MxGetCurrentIrql()
    {
        return KeGetCurrentIrql();
    }

    static
    VOID
    MxBugCheckEx(
        __in ULONG  BugCheckCode,
        __in ULONG_PTR  BugCheckParameter1,
        __in ULONG_PTR  BugCheckParameter2,
        __in ULONG_PTR  BugCheckParameter3,
        __in ULONG_PTR  BugCheckParameter4
    )
    {
        #pragma prefast(suppress:__WARNING_USE_OTHER_FUNCTION, "KeBugCheckEx is the intent.");
        KeBugCheckEx(
            BugCheckCode,
            BugCheckParameter1,
            BugCheckParameter2,
            BugCheckParameter3,
            BugCheckParameter4
            );
    }

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
    MxDbgBreakPoint()
    {
        DbgBreakPoint();
    }

    __inline
    static
    BOOLEAN
    IsUM()
    {
        #if FX_CORE_MODE==FX_CORE_KERNEL_MODE
        return FALSE;
        #else
        return TRUE;
        #endif
    }

    __inline
    static
    BOOLEAN
    IsKM()
    {
        #if FX_CORE_MODE==FX_CORE_KERNEL_MODE
        return TRUE;
        #else
        return FALSE;
        #endif        
    }

    __inline
    static
    VOID
    MxQueryTickCount(
        __out PLARGE_INTEGER  TickCount
        )
    {
        KeQueryTickCount(TickCount);
    }

    __inline
    static
    MxThread
    MxGetCurrentThread(
        )
    {
        return KeGetCurrentThread();
    }

    _Acquires_lock_(_Global_critical_region_)
    __inline
    static
    VOID
    MxEnterCriticalRegion(
        )
    {
        KeEnterCriticalRegion();
    }

    _Releases_lock_(_Global_critical_region_)
    __inline
    static
    VOID
    MxLeaveCriticalRegion(
        )
    {
        KeLeaveCriticalRegion();
    }

    __inline
    static
    VOID
    MxDelayExecutionThread(
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
    static
    VOID
    MxReleaseRemoveLock(
        __in MdRemoveLock  RemoveLock,
        __in PVOID  Tag 
        )
    {
        IoReleaseRemoveLock(RemoveLock, Tag);
    }

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
        )
    {
        return IoAllocateDriverObjectExtension(DriverObject,
                                                ClientIdentificationAddress,
                                                DriverObjectExtensionSize,
                                                DriverObjectExtension);
    }

    __inline
    static
    PVOID
    MxGetDriverObjectExtension(
        __in PDRIVER_OBJECT DriverObject,
        __in PVOID ClientIdentificationAddress
        )
    {
        return IoGetDriverObjectExtension(DriverObject, 
                                            ClientIdentificationAddress);
    }

    __inline
    static
    NTSTATUS
    MxAcquireRemoveLock(
        __in MdRemoveLock  RemoveLock,
        __in_opt PVOID  Tag 
        )
    {
        return IoAcquireRemoveLock(RemoveLock, Tag);
    }

    __inline
    static
    VOID
    MxDetachDevice(
        _Inout_ MdDeviceObject Device
        )
    {
        IoDetachDevice(Device);
    }

    __inline
    static
    VOID
    MxDereferenceObject(
        __in PVOID Object
        )
    {
        ObDereferenceObject(Object);
    }

    __inline
    static
    VOID
    MxDeleteSymbolicLink(
        __in PUNICODE_STRING Value
        )
    {
        IoDeleteSymbolicLink(Value);
    }

    __inline
    static
    VOID
    MxDeleteDevice(
        _In_ MdDeviceObject Device
        )
    {
        IoDeleteDevice(Device);
    }

    __inline
    static
    VOID
    MxAssert(
        __in BOOLEAN Condition
        )
    {
        UNREFERENCED_PARAMETER(Condition);

        ASSERT(Condition); //this get defined as RtlAssert
    }

    __inline
    static
    VOID
    MxReferenceObject(
        __in PVOID Object
        )
    {
        ObReferenceObject(Object);
    }

    __inline
    static
    VOID
    UnregisterCallback(
        __in PVOID  CbRegistration
        )
    {
        ExUnregisterCallback(CbRegistration);
    }

    __inline
    static
    MdEThread
    GetCurrentEThread(
        )
    {
        return PsGetCurrentThread();
    }

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
    static
    NTSTATUS
    CreateCallback(
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
    static
    PVOID
    RegisterCallback(
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
    static
    MdDeviceObject
    MxAttachDeviceToDeviceStack(
        _In_ MdDeviceObject SourceDevice,
        _In_ MdDeviceObject TargetDevice
        )
    {
        return IoAttachDeviceToDeviceStack(SourceDevice, TargetDevice);
    }

    /*__inline
    static
    NTSTATUS
    MxCreateDeviceSecure(
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
        return IoCreateDeviceSecure(DriverObject,
                    DeviceExtensionSize,
                    DeviceName,
                    DeviceType,
                    DeviceCharacteristics,
                    Exclusive,
                    DefaultSDDLString,
                    DeviceClassGuid,
                    DeviceObject);
    }*/

    __inline
    static
    NTSTATUS 
    MxCreateDevice(
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
    static
    VOID
    MxInitializeRemoveLock(
        __in MdRemoveLock  Lock,
        __in ULONG  AllocateTag,
        __in ULONG  MaxLockedMinutes,
        __in ULONG  HighWatermark
        )
    {
        IoInitializeRemoveLock(Lock, AllocateTag, MaxLockedMinutes, HighWatermark);
    }

    __inline
    static
    VOID
    MxDeleteNPagedLookasideList(
        _In_ PNPAGED_LOOKASIDE_LIST LookasideList
        )
    {
        ExDeleteNPagedLookasideList(LookasideList);
    }

    _Releases_lock_(_Global_cancel_spin_lock_)
    __drv_requiresIRQL(DISPATCH_LEVEL)
    __inline
    static
    VOID
    ReleaseCancelSpinLock(
        __in __drv_restoresIRQL __drv_useCancelIRQL  KIRQL  Irql
        )
    {
        IoReleaseCancelSpinLock(Irql);
    }

    __inline
    static
    PVOID
    MxGetSystemAddressForMdlSafe(
        __inout PMDL Mdl,
        __in    ULONG Priority
        )
    {
        return MmGetSystemAddressForMdlSafe(Mdl, (_MM_PAGE_PRIORITY)Priority);
    }

    __inline
    static
    NTSTATUS
    MxRegisterDeviceInterface(
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
    static
    VOID
    MxFlushQueuedDpcs(
        )
    {
        KeFlushQueuedDpcs();
    }

    __drv_maxIRQL(HIGH_LEVEL)
    __drv_raisesIRQL(NewIrql)
    __inline
    static
    VOID
    MxRaiseIrql(
        __in KIRQL                              NewIrql,
        __out __deref __drv_savesIRQL PKIRQL    OldIrql
        )
    {
        KeRaiseIrql(NewIrql, OldIrql);
    }

    __drv_maxIRQL(HIGH_LEVEL)
    __inline
    static
    VOID
    MxLowerIrql(
        __in __drv_restoresIRQL __drv_nonConstant KIRQL NewIrql
        )
    {
        KeLowerIrql(NewIrql);
    }

};

#endif //_MXGENERAL_H_