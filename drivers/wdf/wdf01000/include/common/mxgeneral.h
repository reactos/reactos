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

};

#endif //_MXGENERAL_H_