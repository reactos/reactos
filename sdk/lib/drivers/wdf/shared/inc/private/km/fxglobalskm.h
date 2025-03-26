/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxGlobalsKm.h

Abstract:

    This module contains kernel-mode specific globals definitions
    for the frameworks.

    For common definitions common between km and um please see
    FxGlobals.h

Author:



Environment:

    Kernel mode only

Revision History:


--*/
#ifdef __cplusplus
extern "C" {
#endif

extern PCHAR WdfLdrType;

#define WDF_LDR_STATIC_TYPE_STR     "WdfStatic"

// forward definitions
typedef struct _FX_DRIVER_GLOBALS *PFX_DRIVER_GLOBALS;
typedef struct _FX_DUMP_DRIVER_INFO_ENTRY *PFX_DUMP_DRIVER_INFO_ENTRY;

struct FxMdlDebugInfo {
    PMDL Mdl;
    FxObject* Owner;
    PVOID Caller;
};

#define NUM_MDLS_IN_INFO (16)

struct FxAllocatedMdls {
    FxMdlDebugInfo Info[NUM_MDLS_IN_INFO];
    ULONG Count;
    struct FxAllocatedMdls* Next;
};

#define DDI_ENTRY_IMPERSONATION_OK()
#define DDI_ENTRY()

typedef
BOOLEAN
(STDCALL *PFN_KD_REFRESH)(
    );

typedef
VOID
(STDCALL *PFN_KE_FLUSH_QUEUED_DPCS)(
    VOID
    );

typedef
NTSTATUS
(STDCALL *PFN_IO_SET_COMPLETION_ROUTINE_EX)(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp,
    __in PIO_COMPLETION_ROUTINE CompletionRoutine,
    __in PVOID Context,
    __in BOOLEAN InvokeOnSuccess,
    __in BOOLEAN InvokeOnError,
    __in BOOLEAN InvokeOnCancel
    );

typedef
BOOLEAN
(STDCALL *PFN_KE_REGISTER_BUGCHECK_REASON_CALLBACK) (
    __in PKBUGCHECK_REASON_CALLBACK_RECORD  CallbackRecord,
    __in PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine,
    __in KBUGCHECK_CALLBACK_REASON Reason,
    __in PUCHAR Component
    );

typedef
BOOLEAN
(STDCALL *PFN_KE_DEREGISTER_BUGCHECK_REASON_CALLBACK) (
    __in PKBUGCHECK_REASON_CALLBACK_RECORD  CallbackRecords
    );

typedef
NTSTATUS
(STDCALL *PFN_IO_CONNECT_INTERRUPT_EX)(
    __inout PIO_CONNECT_INTERRUPT_PARAMETERS Parameters
    );

typedef
NTSTATUS
(STDCALL *PFN_IO_DISCONNECT_INTERRUPT_EX)(
    __in PIO_DISCONNECT_INTERRUPT_PARAMETERS Parameters
    );

typedef
NTSTATUS
(STDCALL *PFN_IO_CONNECT_INTERRUPT)(
    __out PKINTERRUPT *InterruptObject,
    __in  PKSERVICE_ROUTINE ServiceRoutine,
    __in_opt PVOID ServiceContext,
    __in_opt PKSPIN_LOCK SpinLock,
    __in  ULONG Vector,
    __in  KIRQL Irql,
    __in  KIRQL SynchronizeIrql,
    __in  KINTERRUPT_MODE InterruptMode,
    __in  BOOLEAN ShareVector,
    __in  KAFFINITY ProcessorEnableMask,
    __in  BOOLEAN FloatingSave
    );

typedef
VOID
(STDCALL *PFN_IO_DISCONNECT_INTERRUPT)(
    __in PKINTERRUPT InterruptObject
    );

typedef
KIRQL
(FASTCALL *PFN_KF_RAISE_IRQL) (
    __in KIRQL NewIrql
    );

typedef
VOID
(FASTCALL *PFN_KF_LOWER_IRQL) (
    __in KIRQL NewIrql
    );

typedef
PSLIST_ENTRY
(FASTCALL *PFN_INTERLOCKED_POP_ENTRY_SLIST)(
    __inout PSLIST_HEADER ListHead
    );

typedef
PSLIST_ENTRY
(FASTCALL *PFN_INTERLOCKED_PUSH_ENTRY_SLIST)(
    __inout PSLIST_HEADER ListHead,
    __inout PSLIST_ENTRY ListEntry
    );

typedef
BOOLEAN
(STDCALL *PFN_PO_GET_SYSTEM_WAKE)(
    __in PIRP Irp
    );

typedef
VOID
(STDCALL *PFN_PO_SET_SYSTEM_WAKE)(
    __inout PIRP Irp
    );

typedef
KAFFINITY
(STDCALL *PFN_KE_QUERY_ACTIVE_PROCESSORS)(
    VOID
    );

typedef
VOID
(STDCALL *PFN_KE_SET_TARGET_PROCESSOR_DPC)(
    __in PRKDPC  Dpc,
    __in CCHAR  Number
    );

typedef
BOOLEAN
(STDCALL *PFN_KE_SET_COALESCABLE_TIMER)(
    __inout PKTIMER Timer,
    __in LARGE_INTEGER DueTime,
    __in ULONG Period,
    __in ULONG TolerableDelay,
    __in_opt PKDPC Dpc
    );

typedef
ULONG
(STDCALL *PFN_KE_GET_CURRENT_PROCESSOR_NUMBER)(
    VOID
    );

typedef
ULONG
(STDCALL *PFN_KE_GET_CURRENT_PROCESSOR_NUMBER_EX)(
    __out_opt PPROCESSOR_NUMBER ProcNumber
    );

typedef
ULONG
(STDCALL *PFN_KE_QUERY_MAXIMUM_PROCESSOR_COUNT_EX)(
    __in USHORT GroupNumber
    );

typedef
ULONG
(STDCALL *PFN_KE_QUERY_MAXIMUM_PROCESSOR_COUNT)(
    VOID
    );

typedef
BOOLEAN
(STDCALL *PFN_KE_ARE_APCS_DISABLED)(
    VOID
    );

typedef
ULONG
(STDCALL *PFN_KE_GET_RECOMMENDED_SHARED_DATA_ALIGNMENT)(
    VOID
    );

typedef
NTSTATUS
(STDCALL *PFN_IO_UNREGISTER_PLUGPLAY_NOTIFICATION_EX)(
    __in PVOID NotificationEntry
    );

typedef
NTSTATUS
(STDCALL *PFN_POX_REGISTER_DEVICE) (
    __in MdDeviceObject Pdo,
    __in PPO_FX_DEVICE PoxDevice,
    __out POHANDLE * Handle
    );

typedef
VOID
(STDCALL *PFN_POX_START_DEVICE_POWER_MANAGEMENT) (
    __in POHANDLE Handle
    );

typedef
VOID
(STDCALL *PFN_POX_UNREGISTER_DEVICE) (
    __in POHANDLE Handle
    );

typedef
VOID
(STDCALL *PFN_POX_ACTIVATE_COMPONENT) (
    __in POHANDLE Handle,
    __in ULONG Component,
    __in ULONG Flags
    );

typedef
VOID
(STDCALL *PFN_POX_IDLE_COMPONENT) (
    __in POHANDLE Handle,
    __in ULONG Component,
    __in ULONG Flags
    );

typedef
VOID
(STDCALL *PFN_POX_REPORT_DEVICE_POWERED_ON) (
    __in POHANDLE Handle
    );

typedef
VOID
(STDCALL *PFN_POX_COMPLETE_IDLE_STATE) (
    __in POHANDLE Handle,
    __in ULONG Component
    );

typedef
VOID
(STDCALL *PFN_POX_COMPLETE_IDLE_CONDITION) (
    __in POHANDLE Handle,
    __in ULONG Component
    );

typedef
VOID
(STDCALL *PFN_POX_COMPLETE_DEVICE_POWER_NOT_REQUIRED) (
    __in POHANDLE Handle
    );

typedef
VOID
(STDCALL *PFN_POX_SET_DEVICE_IDLE_TIMEOUT) (
    __in POHANDLE Handle,
    __in ULONGLONG IdleTimeout
    );

typedef
VOID
(STDCALL *PFN_IO_REPORT_INTERRUPT_ACTIVE) (
    _In_ PIO_REPORT_INTERRUPT_ACTIVE_STATE_PARAMETERS Parameters
    );

typedef
VOID
(STDCALL *PFN_IO_REPORT_INTERRUPT_INACTIVE) (
    _In_ PIO_REPORT_INTERRUPT_ACTIVE_STATE_PARAMETERS Parameters
    );

typedef
VOID
(STDCALL *PFN_VF_CHECK_NX_POOL_TYPE) (
    _In_ POOL_TYPE PoolType,
    _In_ PVOID CallingAddress,
    _In_ ULONG PoolTag
    );

VOID
FxRegisterBugCheckCallback(
    __inout PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in    PDRIVER_OBJECT DriverObject
    );

VOID
FxUnregisterBugCheckCallback(
    __inout PFX_DRIVER_GLOBALS FxDriverGlobals
    );

VOID
FxInitializeBugCheckDriverInfo();

VOID
FxUninitializeBugCheckDriverInfo();

VOID
FxCacheBugCheckDriverInfo(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    );

VOID
FxPurgeBugCheckDriverInfo(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    );

typedef struct _FX_DRIVER_TRACKER_CACHE_AWARE {
    //
    // Internal data types.
    //
private:
    typedef struct _FX_DRIVER_TRACKER_ENTRY {
         volatile PFX_DRIVER_GLOBALS FxDriverGlobals;
    } FX_DRIVER_TRACKER_ENTRY, *PFX_DRIVER_TRACKER_ENTRY;

    //
    // Public interface.
    //
public:
    _Must_inspect_result_
    NTSTATUS
    Initialize();

    VOID
    Uninitialize();

    _Must_inspect_result_
    NTSTATUS
    Register(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    VOID
    Deregister(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    //
    // Tracks the driver usage on the current processor.
    // KeGetCurrentProcessorNumberEx is called directly because the procgrp.lib
    // provides the downlevel support for Vista, XP and Win2000.
    //
    __inline
    VOID
    TrackDriver(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        )
    {
        ASSERT(m_PoolToFree != NULL);

        GetProcessorDriverEntryRef(
            KeGetCurrentProcessorIndex())->FxDriverGlobals =
                FxDriverGlobals;
    }

    //
    // Returns the tracked driver (globals) on the current processor.
    // KeGetCurrentProcessorNumberEx is called directly because the procgrp.lib
    // provides the downlevel support for Vista, XP and Win2000.
    //
    _Must_inspect_result_
    __inline
    PFX_DRIVER_GLOBALS
    GetCurrentTrackedDriver()
    {
        PFX_DRIVER_GLOBALS fxDriverGlobals = NULL;

        ASSERT(m_PoolToFree != NULL);

        fxDriverGlobals = GetProcessorDriverEntryRef(
            KeGetCurrentProcessorIndex())->FxDriverGlobals;

        return fxDriverGlobals;
    }

    //
    // Helper functions.
    //
private:
    //
    // Returns the per-processor cache-aligned driver usage ref structure for
    // given processor.
    //
    __inline
    PFX_DRIVER_TRACKER_ENTRY
    GetProcessorDriverEntryRef(
        __in ULONG Index
        )
    {
        return ((PFX_DRIVER_TRACKER_ENTRY) (((PUCHAR)m_DriverUsage) +
                    Index * m_EntrySize));
    }

    //
    // Data members.
    //
private:
    //
    // Pointer to array of cache-line aligned tracking driver structures.
    //
    PFX_DRIVER_TRACKER_ENTRY    m_DriverUsage;

    //
    // Points to pool of per-proc tracking entries that needs to be freed.
    //
    PVOID                       m_PoolToFree;

    //
    // Size of each padded tracking driver structure.
    //
    ULONG                       m_EntrySize;

    //
    // Indicates # of entries in the array of tracking driver structures.
    //
    ULONG                       m_Number;
} FX_DRIVER_TRACKER_CACHE_AWARE, *PFX_DRIVER_TRACKER_CACHE_AWARE;


#include "fxglobals.h"


//
// This inline function tracks driver usage; This info is used by the
// debug dump callback routine for selecting which driver's log to save
// in the minidump. At this time we track the following OS to framework calls:
//  (a) IRP dispatch (general, I/O, PnP, WMI).
//  (b) Request's completion callbacks.
//  (c) Work Item's (& System Work Item's) callback handlers.
//  (d) Timer's callback handlers.
//
__inline
VOID
FX_TRACK_DRIVER(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    if (FxDriverGlobals->FxTrackDriverForMiniDumpLog) {
        FxLibraryGlobals.DriverTracker.TrackDriver(FxDriverGlobals);
    }
}

_Must_inspect_result_
__inline
PVOID
FxAllocateFromNPagedLookasideListNoTracking (
    __in PNPAGED_LOOKASIDE_LIST Lookaside
    )

/*++

Routine Description:

    This function removes (pops) the first entry from the specified
    nonpaged lookaside list. This function was added to allow request allocated
    by a lookaside list to be freed by ExFreePool and hence do no tracking of statistics.

Arguments:

    Lookaside - Supplies a pointer to a nonpaged lookaside list structure.

Return Value:

    If an entry is removed from the specified lookaside list, then the
    address of the entry is returned as the function value. Otherwise,
    NULL is returned.

--*/

{

    PVOID Entry;

    Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);

    if (Entry == NULL) {
        Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                        Lookaside->L.Size,
                                        Lookaside->L.Tag);
    }

    return Entry;
}

__inline
VOID
FxFreeToNPagedLookasideListNoTracking (
    __in PNPAGED_LOOKASIDE_LIST Lookaside,
    __in PVOID Entry
    )
/*++

Routine Description:

    This function inserts (pushes) the specified entry into the specified
    nonpaged lookaside list. This function was added to allow request allocated
    by a lookaside list to be freed by ExFreePool and hence do no tracking of statistics.

Arguments:

    Lookaside - Supplies a pointer to a nonpaged lookaside list structure.

    Entry - Supples a pointer to the entry that is inserted in the
        lookaside list.

Return Value:

    None.

--*/

{
    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
        (Lookaside->L.Free)(Entry);
    }
    else {
        InterlockedPushEntrySList(&Lookaside->L.ListHead,
                                  (PSLIST_ENTRY)Entry);
    }
}

__inline
PVOID
FxAllocateFromNPagedLookasideList (
    _In_ PNPAGED_LOOKASIDE_LIST Lookaside,
    _In_opt_ size_t ElementSize = 0
    )

/*++

Routine Description:

    This function removes (pops) the first entry from the specified
    nonpaged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a nonpaged lookaside list structure.

Return Value:

    If an entry is removed from the specified lookaside list, then the
    address of the entry is returned as the function value. Otherwise,
    NULL is returned.

--*/

{

    PVOID Entry;

    UNREFERENCED_PARAMETER(ElementSize);

    Lookaside->L.TotalAllocates += 1;

    Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);

    if (Entry == NULL) {
        Lookaside->L.AllocateMisses += 1;
        Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                        Lookaside->L.Size,
                                        Lookaside->L.Tag);
    }

    return Entry;
}

__inline
VOID
FxFreeToNPagedLookasideList (
    __in PNPAGED_LOOKASIDE_LIST Lookaside,
    __in PVOID Entry
    )
/*++

Routine Description:

    This function inserts (pushes) the specified entry into the specified
    nonpaged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a nonpaged lookaside list structure.

    Entry - Supples a pointer to the entry that is inserted in the
        lookaside list.

Return Value:

    None.

--*/

{
    Lookaside->L.TotalFrees += 1;

    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
        Lookaside->L.FreeMisses += 1;
        (Lookaside->L.Free)(Entry);

    }
    else {
        InterlockedPushEntrySList(&Lookaside->L.ListHead,
                                  (PSLIST_ENTRY)Entry);
    }
}

_Must_inspect_result_
__inline
PVOID
FxAllocateFromPagedLookasideList (
    __in PPAGED_LOOKASIDE_LIST Lookaside
    )

/*++

Routine Description:

    This function removes (pops) the first entry from the specified
    paged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a paged lookaside list structure.

Return Value:

    If an entry is removed from the specified lookaside list, then the
    address of the entry is returned as the function value. Otherwise,
    NULL is returned.

--*/

{

    PVOID Entry;

    Lookaside->L.TotalAllocates += 1;

    Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
    if (Entry == NULL) {
        Lookaside->L.AllocateMisses += 1;
        Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                        Lookaside->L.Size,
                                        Lookaside->L.Tag);
    }

    return Entry;
}

__inline
VOID
FxFreeToPagedLookasideList (
    __in PPAGED_LOOKASIDE_LIST Lookaside,
    __in PVOID Entry
    )
/*++

Routine Description:

    This function inserts (pushes) the specified entry into the specified
    paged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a paged lookaside list structure.

    Entry - Supples a pointer to the entry that is inserted in the
        lookaside list.

Return Value:

    None.

--*/

{
    Lookaside->L.TotalFrees += 1;

    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
        Lookaside->L.FreeMisses += 1;
        (Lookaside->L.Free)(Entry);

    } else {
        InterlockedPushEntrySList(&Lookaside->L.ListHead,
                                  (PSLIST_ENTRY)Entry);
    }
}

_Must_inspect_result_
__inline
BOOLEAN
FxIsProcessorGroupSupported(
    VOID
    )
{
    //
    // Groups are supported in Win 7 and forward.
    //
    return FxLibraryGlobals.ProcessorGroupSupport;
}

#ifdef __cplusplus
}
#endif
