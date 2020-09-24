/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxVerifierLock.hpp

Abstract:

    This is the C++ header for the verifier FxLock

    This separate object allows the verifier overhead
    to only be allocated when WdfVerifierLock is on.

Author:



Revision History:


        Made it mode agnostic

        New failure paths:
            m_Mutex/m_Lock initialize
            To enforce initialization hidden constructors
            Callers are forced to use CreateAndInitialize methods

--*/

#ifndef _FXVERIFIERLOCK_HPP_
#define _FXVERIFIERLOCK_HPP_

extern "C" {
#if defined(EVENT_TRACING)
#include "FxVerifierLock.hpp.tmh"
#endif
}

/**
 *  These define the lock order used by verifier
 *  for basic objects internal to the driver frameworks.
 *
 *  Higher numbers are "lower" locks in the hierachy, which means
 *  a lock can be acquired if its number greater than or equal
 *  to the current one.
 *
 *  Correct Order:
 *
 *  FX_LOCK_ORDER_DRIVER -> FX_LOCK_ORDER_QUEUE -> FX_LOCK_ORDER_REQUEST
 *
 * Incorrect Order:
 *
 *  FX_LOCK_ORDER_DRIVER -> FX_LOCK_ORDER_QUEUE -> FX_LOCK_ORDER_DEVICE
 *
 * FX_LOCK_ORDER_UNKNOWN represents an object who has not (yet)
 * defined a lock order. It has the highest number, meaning it
 * can be acquired holding any other locks, including itself.
 * At some point in time, this will cause a verifier break point,
 * otherwise we can not fully test the frameworks.
 *
 * FX_LOCK_ORDER_NONE is a statement by the object that it will
 * not use its Lock/Unlock routines. Use of locks on this object
 * under verifier will cause a verifier breakpoint.
 *
 * There is a table mapping these from FX_TYPE_* to the lock orders
 * define here in fx\core\FxVerifierLock.cpp
 *
 */

//
// These locks are driver frameworks "internal" object
// locks and are not intended to be held across callbacks
// to the driver.
//
// They are acquired and released as the result of driver
// calls into the frameworks, which may be holding a driver
// callback lock.
//
#define FX_LOCK_ORDER_NONE                0x0000
#define FX_LOCK_ORDER_UNKNOWN             0xFFFF

#define FX_LOCK_ORDER_PACKAGE_PDO         0x1000
#define FX_LOCK_ORDER_PACKAGE_FDO         0x1000
#define FX_LOCK_ORDER_WMI_IRP_HANDLER     0x1000
#define FX_LOCK_ORDER_PACKAGE_GENERAL     0x1000

#define FX_LOCK_ORDER_IO_TARGET           0x1000

#define FX_LOCK_ORDER_WMI_PROVIDER        0x1001
#define FX_LOCK_ORDER_WMI_INSTANCE        0x1002

#define FX_LOCK_ORDER_DMA_ENABLER         0x1000
#define FX_LOCK_ORDER_DMA_TRANSACTION     0x1001
#define FX_LOCK_ORDER_COMMON_BUFFER       0x1001

//
// A USB device owns a bunch of pipes, so make sure that device can acquire a
// pipe lock while locked, but not vice versa
//
#define FX_LOCK_ORDER_USB_DEVICE_IO_TARGET 0x1000
#define FX_LOCK_ORDER_USB_PIPE_IO_TARGET   0x1001


#define FX_LOCK_ORDER_DRIVER              0x1010
#define FX_LOCK_ORDER_DEVICE              0x1020
#define FX_LOCK_ORDER_MP_DEVICE           0x1020
#define FX_LOCK_ORDER_DEFAULT_IRP_HANDLER 0x1030
#define FX_LOCK_ORDER_QUEUE               0x1030
#define FX_LOCK_ORDER_PACKAGE_IO          0x1031
#define FX_LOCK_ORDER_REQUEST             0x1040
#define FX_LOCK_ORDER_IRPQUEUE            0x1051
#define FX_LOCK_ORDER_TIMER               0x1059
#define FX_LOCK_ORDER_DPC                 0x1060
#define FX_LOCK_ORDER_WORKITEM            0x1060
#define FX_LOCK_ORDER_CLEANUPLIST         0x1060
#define FX_LOCK_ORDER_INTERRUPT           0x1060
#define FX_LOCK_ORDER_FILEOBJECT          0x1060
#define FX_LOCK_ORDER_DEVICE_LIST         0x1061
#define FX_LOCK_ORDER_COLLECTION          0x1070 // collection can be used in any
                                                 // of the above object's callbacks
#define FX_LOCK_ORDER_USEROBJECT          0x2000

// dispose list is very far down in the list because pretty much any item can
// be added to the dispose list while that object's lock is being held
#define FX_LOCK_ORDER_DISPOSELIST         0x8000

#define FX_LOCK_ORDER_SYSTEMWORKITEM      FX_LOCK_ORDER_UNKNOWN
#define FX_LOCK_ORDER_SYSTEMTHREAD        FX_LOCK_ORDER_UNKNOWN // No lock level

//
// These are the device driver callback locks
// used to synchronize callback to the device
// driver. They are "higher" than the internal
// frameworks locks since an internal frameworks lock
// should not be held when these are acquired.
//
// (which should only be due to FxCallback.Invoke to a driver
//  event handler)
//
// This means they must have a lower number than the internal frameworks
// locks.
//
// They may be held when a device driver is calling into a frameworks
// DDI while in an event callback handler.
//
// These levels enforce not only the level of locks acquired
// and released by and for the driver, but also the rules
// about holding internal frameworks locks when calling into
// a driver. If a frameworks bug is holding a frameworks lock
// when it goes to do a callback, the attempt to acquire the
// lower numbered lock will raise the error.
//
#define FX_CALLBACKLOCK_ORDER_DRIVER      0x10
#define FX_CALLBACKLOCK_ORDER_DEVICE      0x20
#define FX_CALLBACKLOCK_ORDER_PACKAGE     0x30
#define FX_CALLBACKLOCK_ORDER_QUEUE       0x31

#define FX_VERIFIER_LOCK_ENTRY(FX_OBJECT_TYPE, FX_LOCK_ORDER) { ##FX_OBJECT_TYPE, ##FX_LOCK_ORDER }

// Internal FxLock spinlock entries
#define FX_VERIFIER_LOCK_ENTRIES()                                                                  \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_DRIVER,              FX_LOCK_ORDER_DRIVER),              \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_DEVICE,              FX_LOCK_ORDER_DEVICE),              \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_MP_DEVICE,           FX_LOCK_ORDER_MP_DEVICE),           \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_PACKAGE_IO,          FX_LOCK_ORDER_PACKAGE_IO),          \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_WMI_INSTANCE,        FX_LOCK_ORDER_WMI_INSTANCE),        \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_WMI_PROVIDER,        FX_LOCK_ORDER_WMI_PROVIDER),        \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_QUEUE,               FX_LOCK_ORDER_QUEUE),               \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_REQUEST,             FX_LOCK_ORDER_REQUEST),             \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_IRPQUEUE,            FX_LOCK_ORDER_IRPQUEUE),            \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_PACKAGE_PDO,         FX_LOCK_ORDER_PACKAGE_PDO),         \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_PACKAGE_FDO,         FX_LOCK_ORDER_PACKAGE_FDO),         \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_WMI_IRP_HANDLER,     FX_LOCK_ORDER_WMI_IRP_HANDLER),     \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_PACKAGE_GENERAL,     FX_LOCK_ORDER_PACKAGE_GENERAL),     \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_DMA_ENABLER,         FX_LOCK_ORDER_DMA_ENABLER),         \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_DMA_TRANSACTION,     FX_LOCK_ORDER_DMA_TRANSACTION),     \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_COMMON_BUFFER,       FX_LOCK_ORDER_COMMON_BUFFER),       \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_IO_TARGET,           FX_LOCK_ORDER_IO_TARGET),           \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_IO_TARGET_SELF,      FX_LOCK_ORDER_IO_TARGET),           \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_IO_TARGET_USB_DEVICE,FX_LOCK_ORDER_USB_DEVICE_IO_TARGET),\
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_IO_TARGET_USB_PIPE,  FX_LOCK_ORDER_USB_PIPE_IO_TARGET),  \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_DPC,                 FX_LOCK_ORDER_DPC),                 \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_WORKITEM,            FX_LOCK_ORDER_WORKITEM),            \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_SYSTEMTHREAD,        FX_LOCK_ORDER_SYSTEMTHREAD),        \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_CLEANUPLIST,         FX_LOCK_ORDER_CLEANUPLIST),         \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_INTERRUPT,           FX_LOCK_ORDER_INTERRUPT),           \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_TIMER,               FX_LOCK_ORDER_TIMER),               \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_FILEOBJECT,          FX_LOCK_ORDER_FILEOBJECT),          \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_CHILD_LIST,         FX_LOCK_ORDER_DEVICE_LIST),          \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_SYSTEMWORKITEM,      FX_LOCK_ORDER_SYSTEMWORKITEM),      \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_DEFAULT_IRP_HANDLER, FX_LOCK_ORDER_DEFAULT_IRP_HANDLER), \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_COLLECTION,          FX_LOCK_ORDER_COLLECTION),          \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_DISPOSELIST,         FX_LOCK_ORDER_DISPOSELIST),         \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_USEROBJECT,          FX_LOCK_ORDER_USEROBJECT),         \
            FX_VERIFIER_LOCK_ENTRY(0,                           0)

// Device Driver Callback lock entries
#define FX_VERIFIER_CALLBACKLOCK_ENTRIES()                                                \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_DRIVER,     FX_CALLBACKLOCK_ORDER_DRIVER),     \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_DEVICE,     FX_CALLBACKLOCK_ORDER_DEVICE),     \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_PACKAGE_IO, FX_CALLBACKLOCK_ORDER_PACKAGE),    \
            FX_VERIFIER_LOCK_ENTRY(FX_TYPE_QUEUE,      FX_CALLBACKLOCK_ORDER_QUEUE),      \
            FX_VERIFIER_LOCK_ENTRY(0,                  0)

//
// Mapping table structure between Fx object types and lock orders
//
struct FxVerifierOrderMapping {
    USHORT ObjectType;
    USHORT ObjectLockOrder;
};

typedef struct FxVerifierOrderMapping  *pFxVerifierOrderMapping;
//
// This must be a power of two for hash algorithm
//
// Table size is dependent on the number of active threads
// in the frameworks at a given time. This can not execeed
// the number of (virtual due to hyperthreading) CPU's in the system.
//
// Setting this number lower just increases collisions, but does
// not impede correct function.
//
#define VERIFIER_THREAD_HASHTABLE_SIZE 64


//
// This structure is used by verifier hash table implementation
// for linking records per thread
//
struct FxVerifierThreadTableEntry {
    MxThread        Thread;
    FxVerifierLock* PerThreadPassiveLockList;
    FxVerifierLock* PerThreadDispatchLockList;
    LIST_ENTRY      HashChain;
};

typedef struct FxVerifierThreadTableEntry *pFxVerifierThreadTableEntry;
/*
 * This lock performs the same actions as the base
 * FxLock, but provides additional tracking of lock
 * order to help detect and debug
 * deadlock and recursive lock situations.
 *
 * It does not inherit from FxLock since this would
 * cause a recursion chain.
 *
 * Since lock verification is already more costly than
 * basic lock use, this verifier lock supports both the
 * FAST_MUTEX and SpinLock modes in order to utilize common
 * code to support verification of FxCallback locks.
 *
 * FxVerifierLocks are only allocated when WdfVerifierLock
 * is turned on.
 */

class FxVerifierLock : public FxGlobalsStump {

private:
    // Standard NT pool object identification
    USHORT m_Type;
    USHORT m_Size;

    // Spinlock to perform our lock functionality
    MxLock          m_Lock;
    KIRQL           m_OldIrql;

    // Fast Mutex for thread level verifier locks
    MxPagedLock     m_Mutex;

    //
    // Verifier per lock working values, protected
    // by our m_Lock
    //
    FxObject*       m_ParentObject;
    MxThread        m_OwningThread;
    USHORT          m_Order;

    // True if its a Mutex based (thread context) lock
    BOOLEAN         m_UseMutex;

    // True if its used as a Callback lock
    BOOLEAN         m_CallbackLock;

    //
    // This is the link for the per thread lock chain.
    //
    FxVerifierLock* m_OwnedLink;

private:
    FxVerifierLock(
        PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxGlobalsStump(FxDriverGlobals)
    {
    }

    void
    InitializeLockOrder(
        VOID
        );

    void
    FxVerifierLock::FxVerifierLockDumpDetails(
        __in FxVerifierLock* Lock,
        __in PVOID           curThread,
        __in FxVerifierLock* PerThreadList
        );

private:
    //
    // This constructor is used by internal object
    // locks, which always use a spinlock.
    //
    FxVerifierLock(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxObject* ParentObject
        ) :
        FxGlobalsStump(FxDriverGlobals)
    {
        m_Type = FX_TYPE_VERIFIERLOCK;
        m_Size = sizeof(FxVerifierLock);

        m_ParentObject = ParentObject;

        m_OwningThread = NULL;
        m_OwnedLink    = NULL;
        m_Order        = FX_LOCK_ORDER_UNKNOWN;

        m_ThreadTableEntry.Thread = NULL;
        m_ThreadTableEntry.PerThreadPassiveLockList = NULL;
        m_ThreadTableEntry.PerThreadDispatchLockList = NULL;

        m_UseMutex = FALSE;
        m_CallbackLock = FALSE;

        InitializeLockOrder();
    }

    _Must_inspect_result_
    __inline
    NTSTATUS
    Initialize(
        )
    {
        NTSTATUS status;

        if (m_UseMutex) {
            status = m_Mutex.Initialize();

            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                    "Unable to initialize paged lock for VerifierLock 0x%p "
                                    "status %!STATUS!",
                                    this, status);
                return status;
            }
        }

        return STATUS_SUCCESS;
    }

    //
    // This constructor is used by the Callback lock
    // functions when verifer is on.
    //
    // The lock utlizes the callback lock order table entries to
    // set its level.
    //
    FxVerifierLock(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxObject* ParentObject,
        __in BOOLEAN UseMutex
        ) :
        FxGlobalsStump(FxDriverGlobals)
    {

        m_Type = FX_TYPE_VERIFIERLOCK;
        m_Size = sizeof(FxVerifierLock);

        m_ParentObject = ParentObject;

        m_OwningThread = NULL;
        m_OwnedLink    = NULL;
        m_Order        = FX_LOCK_ORDER_UNKNOWN;

        m_ThreadTableEntry.Thread = NULL;
        m_ThreadTableEntry.PerThreadPassiveLockList = NULL;
        m_ThreadTableEntry.PerThreadDispatchLockList = NULL;

        // Different verifier table
        m_CallbackLock = TRUE;

        if (UseMutex) {
            m_UseMutex = TRUE;
        }
        else {
            m_UseMutex = FALSE;
        }

        InitializeLockOrder();
    }

public:

    _Must_inspect_result_
    __inline
    static
    NTSTATUS
    CreateAndInitialize(
        __out   FxVerifierLock ** VerifierLock,
        __in    PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in    FxObject* ParentObject,
        __in    BOOLEAN UseMutex
        )
    {
        NTSTATUS status;
        FxVerifierLock * verifierLock;

        verifierLock = new (FxDriverGlobals) FxVerifierLock(FxDriverGlobals,
                                                            ParentObject,
                                                            UseMutex);
        if (NULL == verifierLock) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "Failed to allocate verifier lock, returning %!STATUS!",
                                status);
            goto exit;
        }

        status = verifierLock->Initialize();
        if (!NT_SUCCESS(status)) {
            delete verifierLock;
            goto exit;
        }

        *VerifierLock = verifierLock;

    exit:
        return status;
    }

    _Must_inspect_result_
    __inline
    static
    NTSTATUS
    CreateAndInitialize(
        __out   FxVerifierLock ** VerifierLock,
        __in    PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in    FxObject* ParentObject
        )
    {
        NTSTATUS status;
        FxVerifierLock * verifierLock;

        verifierLock = new (FxDriverGlobals) FxVerifierLock(FxDriverGlobals,
                                                            ParentObject);
        if (NULL == verifierLock) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "Failed to allocate verifier lock, returning %!STATUS!",
                                status);
            goto exit;
        }

        status = verifierLock->Initialize();
        if (!NT_SUCCESS(status)) {
            delete verifierLock;
            goto exit;
        }

        *VerifierLock = verifierLock;

    exit:
        return status;
    }

    ~FxVerifierLock(
        VOID
        )
    {
        if (m_OwningThread != NULL) {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "Lock 0x%p is being destroyed while owned by "
                                "thread 0x%p, Owning Object 0x%p",
                                this, m_OwningThread, m_ParentObject);
            FxVerifierDbgBreakPoint(GetDriverGlobals());
        }
    }

    VOID
    Lock(
        __out PKIRQL PreviousIrql,
        __in BOOLEAN AtDpc
        );

    VOID
    Unlock(
        __in KIRQL PreviousIrql,
        __in BOOLEAN AtDpc
        );

    KIRQL
    GetLockPreviousIrql(
        VOID
        );

    //
    // Instance data needed for the hash table chaining
    //
    FxVerifierThreadTableEntry m_ThreadTableEntry;

    //
    // Static data and methods for hash table chaining
    //
    static ULONG ThreadTableSize;

    static KSPIN_LOCK ThreadTableLock;

    static PLIST_ENTRY ThreadTable;

    static
    void
    AllocateThreadTable(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    static
    void
    FreeThreadTable(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    static
    void
    DumpDetails(
        __in FxVerifierLock* Lock,
        __in MxThread        curThread,
        __in FxVerifierLock* PerThreadList
        );

    static
    pFxVerifierThreadTableEntry
    GetThreadTableEntry(
        __in MxThread        curThread,
        __in FxVerifierLock* pLock,
        __in BOOLEAN         LookupOnly
        );

    static
    void
    ReleaseOrReplaceThreadTableEntry(
        __in MxThread        curThread,
        __in FxVerifierLock* pLock
        );
};

#endif // _FXVERIFIERLOCK_HPP_


