/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxSpinLock.hpp

Abstract:

Author:


Revision History:

--*/

#ifndef _FXSPINLOCK_H_
#define _FXSPINLOCK_H_

#define FX_SPIN_LOCK_NUM_HISTORY_ENTRIES (10)

struct FX_SPIN_LOCK_HISTORY_ENTRY {
    PVOID CallersAddress;
    LARGE_INTEGER AcquiredAtTime;
    LONGLONG LockedDuraction;
};

typedef struct FX_SPIN_LOCK_HISTORY_ENTRY *PFX_SPIN_LOCK_HISTORY_ENTRY;

struct FX_SPIN_LOCK_HISTORY {
    MxThread  OwningThread;

    PFX_SPIN_LOCK_HISTORY_ENTRY CurrentHistory;

    FX_SPIN_LOCK_HISTORY_ENTRY History[FX_SPIN_LOCK_NUM_HISTORY_ENTRIES];
};

typedef struct FX_SPIN_LOCK_HISTORY *PFX_SPIN_LOCK_HISTORY;

class FxSpinLock : public FxObject  {

public:
    FxSpinLock(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in USHORT ExtraSize
        );


    __drv_raisesIRQL(DISPATCH_LEVEL)
    __drv_maxIRQL(DISPATCH_LEVEL)
    VOID
    AcquireLock(
        __in PVOID CallersAddress
        );

    __drv_requiresIRQL(DISPATCH_LEVEL)
    VOID
    ReleaseLock(
        VOID
        );


    MdLock*
    GetLock(
        VOID
        )
    {
        return &m_SpinLock.Get();
    }

    VOID
    SetInterruptSpinLock(
        VOID
        )
    {
        m_InterruptLock = TRUE;
    }

    BOOLEAN
    IsInterruptLock(
        VOID
        )
    {
        return m_InterruptLock;
    }

protected:
    PFX_SPIN_LOCK_HISTORY
    GetHistory(
        VOID
        )
    {
        if (GetObjectSize() == WDF_ALIGN_SIZE_UP(sizeof(FxSpinLock),
                                                 MEMORY_ALLOCATION_ALIGNMENT)) {
            return NULL;
        }
        else {
            return WDF_PTR_ADD_OFFSET_TYPE(
                this,
                COMPUTE_RAW_OBJECT_SIZE(sizeof(FxSpinLock)),
                PFX_SPIN_LOCK_HISTORY);
        }
    }

protected:
    MxLock m_SpinLock;

    KIRQL m_Irql;

    //
    // TRUE, this lock is being used to synchronize FxInterrupts.  As such, the
    // caller is not allowed to actually call WDFSPINLOCK APIs on the handle
    // because it would then be acquiring a spinlock at DISPATCH_LEVEL and the
    // interrupt could be attempting to acquire the same lock at a higher IRQL
    // on the same processor.
    //
    BOOLEAN m_InterruptLock;
};

#endif // _FXSPINLOCK_H_
