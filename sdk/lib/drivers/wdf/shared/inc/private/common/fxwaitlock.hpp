/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxWaitLock.hpp

Abstract:

--*/

#ifndef _FXWAITLOCK_HPP_
#define _FXWAITLOCK_HPP_

struct FxCREvent {
    FxCREvent(
        __in BOOLEAN InitialState = FALSE
        )
    {
    //
    // For kernel mode letting c'tor do the initialization to not churn the
    // non-shared code
    //
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
        m_Event.Initialize(SynchronizationEvent, InitialState);
#else
        UNREFERENCED_PARAMETER(InitialState);
#endif
    }





    FxCREvent(
        __in EVENT_TYPE Type,
        __in BOOLEAN InitialState
        )
    {
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
        m_Event.Initialize(Type, InitialState);
#else
        UNREFERENCED_PARAMETER(Type);
        UNREFERENCED_PARAMETER(InitialState);
#endif
    }

    CHECK_RETURN_IF_USER_MODE
    NTSTATUS
    Initialize(
        __in BOOLEAN InitialState = FALSE
        )
    {
        return m_Event.Initialize(SynchronizationEvent, InitialState);
    }

    CHECK_RETURN_IF_USER_MODE
    NTSTATUS
    Initialize(
        __in EVENT_TYPE Type,
        __in BOOLEAN InitialState
        )
    {
        return m_Event.Initialize(Type, InitialState);
    }

    __drv_valueIs(==0;==258)
    _Acquires_lock_(_Global_critical_region_)
    NTSTATUS
    EnterCRAndWait(
        VOID
        )
    {
        NTSTATUS status;

        Mx::MxEnterCriticalRegion();

        status = m_Event.WaitFor(Executive,
                                 KernelMode,
                                 FALSE,
                                 NULL);
        return status;
    }

    NTSTATUS
    EnterCRAndWaitAndLeave(
        VOID
        )
    {
        NTSTATUS status;

        status = EnterCRAndWait();
        LeaveCR();

        return status;
    }

    __drv_when(Timeout == NULL, __drv_valueIs(==0))
    __drv_when(Timeout != NULL, __drv_valueIs(==0;==258))
    __drv_when(Timeout != NULL, _Must_inspect_result_)
    _Acquires_lock_(_Global_critical_region_)
    NTSTATUS
    EnterCRAndWait(
        __in PLONGLONG Timeout
        )
    {
        NTSTATUS status;

        Mx::MxEnterCriticalRegion();

        status = m_Event.WaitFor(Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) Timeout);

        return status;
    }

    _Must_inspect_result_
    NTSTATUS
    EnterCRAndWaitAndLeave(
        __in PLONGLONG Timeout
        )
    {
        NTSTATUS status;

        status = EnterCRAndWait(Timeout);
        LeaveCR();

        return status;
    }

    _Releases_lock_(_Global_critical_region_)
    VOID
    LeaveCR(
        VOID
        )
    {
        Mx::MxLeaveCriticalRegion();
    }

    VOID
    Set(
        VOID
        )
    {
        m_Event.Set();
    }

    VOID
    Clear(
        VOID
        )
    {
        m_Event.Clear();
    }

    LONG
    ReadState(
        VOID
        )
    {
        return m_Event.ReadState();
    }

    //
    // Return the underlying event
    // PKEVENT in kernel mode and event HANDLE in user-mode
    //
    PVOID
    GetEvent(
        VOID
        )
    {
        return m_Event.GetEvent();
    }

    FxCREvent*
    GetSelfPointer(
        VOID
        )
    {
        //
        // Since operator& is hidden, we still need to be able to get a pointer
        // to this object, so we must make it an explicit method.
        //
        return this;
    }
private:
    FxCREvent* operator&(
        VOID
        )
    {
        //
        // By making the address of operator private, we make it harder (hopefully
        // impossible) to accidentally use this object in an improper fashion, ie
        // something like this is prevented:
        //
        // FxCREvent event;
        // KeWaitForSingleObject(&event, ...);
        //
        ASSERT(FALSE);
        return NULL;
    }

private:
    MxEvent m_Event;
};

//
// Non referencable object, just pure implementation
//
class FxWaitLockInternal {

public:

    FxWaitLockInternal(
        VOID
        )
    {
    //
    // For kernel mode letting c'tor do the initialization to not churn the
    // non-shared code
    //
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
        m_Event.Initialize(SynchronizationEvent, TRUE);
#endif

        m_OwningThread = NULL;
    }

    CHECK_RETURN_IF_USER_MODE
    NTSTATUS
    Initialize(
        )
    {
        return m_Event.Initialize(SynchronizationEvent, TRUE);
    }

    //

    //
    __drv_when(Timeout == NULL, __drv_valueIs(==0))
    __drv_when(Timeout != NULL, __drv_valueIs(==0;==258))
    __drv_when(Timeout != NULL, _Must_inspect_result_)
    _When_(return!=0x00000102L, _Acquires_lock_(_Global_critical_region_))
    NTSTATUS
    AcquireLock(
        __in        PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in_opt    PLONGLONG Timeout = NULL
        )
    {
        LARGE_INTEGER li;
        NTSTATUS status;

        UNREFERENCED_PARAMETER(FxDriverGlobals);

        ASSERT(m_OwningThread != Mx::MxGetCurrentThread() || (Timeout != NULL));

        if (Timeout != NULL) {
            li.QuadPart = *Timeout;
        }

        Mx::MxEnterCriticalRegion();
        status = m_Event.WaitFor(Executive,
                                 KernelMode,
                                 FALSE,
                                 Timeout == NULL ? NULL : &li);

        if (status == STATUS_TIMEOUT) {
            Mx::MxLeaveCriticalRegion();
        }
        else {
            m_OwningThread = Mx::MxGetCurrentThread();
        }

        return status;
    }

    static
    BOOLEAN
    IsLockAcquired(
        __in NTSTATUS Status
        )
    {
        //
        // STATUS_TIMEOUT will return TRUE for NT_SUCCESS so check explicitly
        //
        return (NT_SUCCESS(Status) && Status != STATUS_TIMEOUT) ? TRUE : FALSE;
    }

    _Releases_lock_(_Global_critical_region_)
    VOID
    ReleaseLock(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        )
    {
        UNREFERENCED_PARAMETER(FxDriverGlobals);

        ASSERT(m_OwningThread == Mx::MxGetCurrentThread());
        m_OwningThread = NULL;

        m_Event.Set();
        Mx::MxLeaveCriticalRegion();
    }

protected:
    MxEvent m_Event;

    MxThread m_OwningThread;
};


//
// Order is important here, FxObject *must* be the first class in the
// list so that &FxWaitWaitLock == &FxNonPagedObject.
//
class FxWaitLock : public FxObject, public FxWaitLockInternal  {

public:
    // Factory function
    _Must_inspect_result_
    static
    NTSTATUS
    _Create(
        __in PFX_DRIVER_GLOBALS         DriverGlobals,
        __in_opt PWDF_OBJECT_ATTRIBUTES Attributes,
        __in_opt FxObject*              ParentObject,
        __in BOOLEAN                    AssignDriverAsDefaultParent,
        __out WDFWAITLOCK*              LockHandle
        );


    CHECK_RETURN_IF_USER_MODE
    NTSTATUS
    Initialize(
        )
    {
        return FxWaitLockInternal::Initialize(); // __super call
    }

    FxWaitLock(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxObject(FX_TYPE_WAIT_LOCK, sizeof(FxWaitLock), FxDriverGlobals)
    {
    }
};

#endif // _FXWAITLOCK_HPP_
