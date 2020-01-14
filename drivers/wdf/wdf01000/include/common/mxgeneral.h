#ifndef _MXGENERAL_H_
#define _MXGENERAL_H_

#include <ntddk.h>
#include <wdf.h>

//
// Placeholder macro for a no-op
//
#define DO_NOTHING()                            (0)

typedef PKTHREAD                MxThread;
typedef PDRIVER_OBJECT          MdDriverObject;
typedef PDEVICE_OBJECT          MdDeviceObject;
typedef PIO_REMOVE_LOCK         MdRemoveLock;
typedef PETHREAD                MdEThread;

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

};

#endif //_MXGENERAL_H_