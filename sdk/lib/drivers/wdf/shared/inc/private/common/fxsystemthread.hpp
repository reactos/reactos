/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxSystemThread.hpp

Abstract:

    This class provides the safe handling for system threads
    in the driver frameworks.


Author:




Revision History:

--*/

#ifndef _FXSYSTEMTHREAD_H_
#define _FXSYSTEMTHREAD_H_

class FxSystemThread : public FxNonPagedObject {

private:

    MxEvent m_InitEvent;
    MxEvent m_WorkEvent;

    LIST_ENTRY m_WorkList;

    PVOID m_ThreadPtr;

    MdEThread m_PEThread;

    // Used for Async thread spinup and reaping
    //WORK_QUEUE_ITEM m_Spinup; // Async-Thread-Spinup
    WORK_QUEUE_ITEM m_Reaper;

    BOOLEAN m_Exit;
    BOOLEAN m_Initialized;

    FxSystemThread(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    //
    // Create the system thread in order to be able to service work items
    //
    // It is recommended this is done from the system process context
    // since the thread's handle is available to the user mode process
    // for a temporary window. XP and later supports OBJ_KERNELHANDLE, but
    // DriverFrameworks must support W2K with the same binary.
    //
    // It is safe to call this at DriverEntry which is in the system process
    // to create an initial driver thread, and this driver thread should be
    // used for creating any child driver threads on demand by using
    // Initialize(FxSystemThread* SpinupThread).
    //
    BOOLEAN
    Initialize(
        VOID
        );

public:

    _Must_inspect_result_
    static
    NTSTATUS
    _CreateAndInit(
        __deref_out FxSystemThread** SystemThread,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in WDFDEVICE Device,
        __in MdDeviceObject DeviceObject
        );

    virtual
    ~FxSystemThread(
        VOID
        );





















    //
    // This is called to tell the thread to exit.
    //
    // It must be called from thread context such as
    // the driver unload routine since it will wait for the
    // thread to exit.
    //
    // A worker thread will not exit unless it has processed
    // all of its queued work items. If processing of queued
    // workitems is no longer desired, then use CancelWorkItem
    // to remove the items before calling this method.
    //
    BOOLEAN
    ExitThread(
        VOID
        );

    //
    // This is called to tell the thread to exit.
    //
    // This may be called from any context since it
    // only signals the thread to exit, and does not
    // wait for it. It is safe to release the object
    // reference when this routine returns.
    //
    // This routine requires an FxSystemThread* to perform
    // final thread reaping. This allows a driver to ensure that
    // child threads have exited by waiting on their object pointer,
    // before exiting a top level driver thread.
    //
    // This is required since a frameworks object that contains a
    // system thread can be dereferenced, and thus destroyed
    // in any context, so waiting may not be available. But unless
    // a wait is done on the thread object pointer, no guarantee can be
    // made that the thread has actually run and exited before
    // the drivers code get unloaded, resulting in a system crash.
    //
    // Top level threads, such as the reaper, are exited using the
    // ExitThread() synchronous call that waits for it to exit. This
    // can be done in synchronous thread context such as DriverUnload
    // time.
    //
    // If Synchronous threading is configured in the frameworks, the
    // FxDriver object always contains a top level worker thread that
    // may be used as the reaper, and DriverUnload synchronously waits
    // for this thread to finish processing its work queue before
    // exiting.
    //
    // A worker thread will not exit unless it has processed
    // all of its queued work items. If processing of queued
    // workitems is no longer desired, then use CancelWorkItem
    // to remove the items before calling this method.
    //
    BOOLEAN
    ExitThreadAsync(
        __inout FxSystemThread* Reaper
        );

    //
    // Queue a work item to the thread
    //
    // It is valid to queue work items before thread
    // Initialize()/Initialize(FxSystemThread*) is called. The items
    // remain queued until the system thread is started.
    //
    BOOLEAN
    QueueWorkItem(
        __inout PWORK_QUEUE_ITEM WorkItem
        );

    //
    // Attempt to cancel the work item.
    //
    // Returns TRUE if success.
    //
    // If returns FALSE, the work item
    // routine either has been called, is running,
    // or is about to be called.
    //
    BOOLEAN
    CancelWorkItem(
        __inout PWORK_QUEUE_ITEM WorkItem
        );

    //
    // Determine if current thread is this
    // worker thread
    //
    __inline
    BOOLEAN
    IsCurrentThread()
    {
        return (Mx::GetCurrentEThread() == m_PEThread) ? TRUE : FALSE;
    }

    DECLARE_INTERNAL_NEW_OPERATOR();

#ifdef INLINE_WRAPPER_ALLOCATION
#if (FX_CORE_MODE==FX_CORE_USER_MODE)
    FORCEINLINE
    PVOID
    GetCOMWrapper(
        )
    {
        PBYTE ptr = (PBYTE) this;
        return (ptr + (USHORT) WDF_ALIGN_SIZE_UP(sizeof(*this), MEMORY_ALLOCATION_ALIGNMENT));
    }
#endif
#endif

private:

    //
    // This is the thread's main processing function
    //
    VOID
    Thread(
        VOID
        );














    //
    // This is the reaper method
    //
    VOID
    Reaper(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    CreateThread(
        VOID
        );

    //
    // This is the thunk used to get from the system
    // thread start callback into this class
    //
    static
    VOID
    STDCALL
    StaticThreadThunk(
        __inout PVOID Context
        );
















    //
    // This thunk is called from the workitem in another
    // thread that is reaping this thread
    //
    static
    VOID
    STDCALL
    StaticReaperThunk(
        __inout PVOID Context
        );
};

#endif // _FXSYSTEMTHREAD_H_
