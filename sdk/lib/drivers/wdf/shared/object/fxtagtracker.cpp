/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxTagTracker.cpp

Abstract:

Author:

Environment:

    Both kernel and user mode

Revision History:







--*/

#include "fxobjectpch.hpp"

extern "C" {

#if defined(EVENT_TRACING)
#include "fxtagtracker.tmh"
#endif

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
//
// rtlsupportapi.h causes problems in AMD64 and ARM builds.
//
extern
_Success_(return != 0)
USHORT
RtlCaptureStackBackTrace(
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_to_(FramesToCapture, return) PVOID * BackTrace,
    _Out_opt_ PULONG BackTraceHash
    );
#endif

}

FxTagTracker::~FxTagTracker()
/*++

Routine Description:
    Destructor for this object.  Will verify that the object is being freed
    without any outstanding tags.

Arguments:
    None

Return Value:
    None

  --*/
{
    KIRQL irql;
    FxTagTrackingBlock *current, *next;

    if (m_TrackerType == FxTagTrackerTypeHandle) {
        FxDriverGlobalsDebugExtension* pExtension;

        CheckForAbandondedTags();

        pExtension = GetDriverGlobals()->DebugExtension;

        //
        // Remove this tracker from the list of allocated trackers
        //
        pExtension->AllocatedTagTrackersLock.Acquire(&irql);
        RemoveEntryList(&m_TrackerEntry);
        pExtension->AllocatedTagTrackersLock.Release(irql);
    } else {
        ASSERT(m_TrackerType == FxTagTrackerTypePower);
    }

    //
    // Delete any outstanding tracking blocks.
    //
    m_SpinLock.Acquire(&irql);

    current = m_Next;
    m_Next = NULL;

    while (current != NULL) {
        next = current->Next;
        delete current;
        current = next;
    }

    m_SpinLock.Release(irql);
}

VOID
FxTagTracker::CopyStackFrames(
    _Inout_ FxTagTrackingStackFrames** StackFrames,
    _In_ USHORT NumFrames,
    _In_reads_(NumFrames) PVOID* Frames
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxTagTrackingStackFrames* stackFrames;

    //
    // FxTagHistory structs are stored in a circular buffer and reused,
    // so we also reuse the FxTagTrackingStackFrames that each allocates.
    //
    stackFrames = *StackFrames;
    if (stackFrames == NULL) {
        pFxDriverGlobals = GetDriverGlobals();
        stackFrames = new(pFxDriverGlobals) FxTagTrackingStackFrames;
        if (stackFrames == NULL) {
            return;
        }

        *StackFrames = stackFrames;
    }

    stackFrames->NumFrames = NumFrames;

    for (int i = 0; i < NumFrames; i++) {
        stackFrames->Frames[i] = (ULONG64)Frames[i];
    }
}

VOID
FxTagTracker::UpdateTagHistory(
    __in        PVOID Tag,
    __in        LONG Line,
    __in_opt    PSTR File,
    __in        FxTagRefType RefType,
    __in        ULONG RefCount
    )
/*++

Routine Description:
    Update tag history and either create or free an existing tag tracking block.

Arguments:

    Tag - Unique tag associated with the reference

    Line - Line where the reference is referenced/released

    File - Buffer containing the file name

    RefType - Enumerated type ( AddRef or Release )

    RefCount - Approximate current reference count (see FxTagHistory.RefCount comment)

Return Value:

    VOID

--*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxTagHistory* pTagHistory;
    FxTagTrackingBlock* pBlock;
    LONG pos;
    KIRQL irql;
    USHORT numFrames = 0;
    PVOID frames[FRAMES_TO_CAPTURE];

    pFxDriverGlobals = GetDriverGlobals();

    pos = InterlockedIncrement(&m_CurRefHistory) - 1;
    pos %= TAG_HISTORY_DEPTH;

    //
    // Prefast reports that m_CurRefHistory can be negative which can lead to
    // underflow. But we know that m_CurRefHistory would never be negative.
    // Hence we assert for the condition below and assume that it would be true.
    //
    FX_ASSERT_AND_ASSUME_FOR_PREFAST(pos >= 0 && pos < TAG_HISTORY_DEPTH);

    pTagHistory = m_TagHistory + pos;

    pTagHistory->RefType = RefType;
    pTagHistory->RefCount = RefCount;
    pTagHistory->Line = Line;
    pTagHistory->Tag = Tag;
    pTagHistory->File = File;

    if (m_CaptureStack) {
        numFrames = RtlCaptureStackBackTrace(FRAMES_TO_SKIP,
                                             FRAMES_TO_CAPTURE,
                                             frames,
                                             NULL);
        if (numFrames > 0) {
            CopyStackFrames(&pTagHistory->StackFrames, numFrames, frames);
        }
    }

    //
    // We use the perf counter here and the tick count in the tracking block.
    // Use the tick count here as well until we decide that correlating the
    // perf counter to system time is not important.
    //
    // pTagHistory->Time = KeQueryPerformanceCounter(NULL);
    Mx::MxQueryTickCount(&pTagHistory->Time);

    if (RefType == TagAddRef) {

        //
        // Try to allocate some memory for the new block.  If unsuccessful,
        // fallback to a failed count increment.
        //
        pBlock = new(pFxDriverGlobals) FxTagTrackingBlock(Tag, Line, File);

        if (pBlock == NULL) {
            InterlockedIncrement(&m_FailedCount);
        }
        else {
            m_SpinLock.Acquire(&irql);
            pBlock->Next = m_Next;
            m_Next = pBlock;
            m_SpinLock.Release(irql);

            if (m_CaptureStack && numFrames > 0) {
                CopyStackFrames(&pBlock->StackFrames, numFrames, frames);
            }
        }
    }
    else {
        FxTagTrackingBlock **prev;

        //
        // Walk the list of current blocks and attempt to find the tag being
        // released.  If not found, decrement the failed count.  If no failed
        // tags exists, ASSERT immediately.
        //

        m_SpinLock.Acquire(&irql);
        prev = &m_Next;
        pBlock = *prev;

        while (pBlock != NULL) {
            if (pBlock->Tag == Tag) {
                *prev = pBlock->Next;
                break;
            }

            prev = &pBlock->Next;
            pBlock = pBlock->Next;
        }

        m_SpinLock.Release(irql);

        if (pBlock == NULL) {
            //
            // Check to see if we have any credits in our Low Memory Count.
            // In this fassion we can tell if we have acquired any locks without
            // the memory for adding tracking blocks.
            //
            if (InterlockedDecrement(&m_FailedCount) < 0) {
                //
                // We have just released a lock that neither had a corresponding
                // tracking block, nor a credit in LowMemoryCount.
                //
                InterlockedIncrement(&m_FailedCount);

                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "releasing %s %p on object %p that was not acquired, !wdftagtracker %p",
                    m_TrackerType == FxTagTrackerTypePower ? "power tag" : "tag",
                    Tag,
                    m_OwningObject,
                    this);

                FxVerifierDbgBreakPoint(pFxDriverGlobals);
            }
        }
        else {
            delete pBlock;
            pBlock = NULL;
        }
    }
}

VOID
FxTagTracker::CheckForAbandondedTags(
    VOID
    )
/*++

Routine Description:
    Iterates over any existing tags, dumping any existing tags to the debugger.
    Will assert if there any outstanding tags (assumes that the caller wants
    no current tags).

Arguments:
    None

Return Value:
    None

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxTagTrackingBlock *current, *next;
    LONG abandoned;
    KIRQL irql;
    BOOLEAN committed;

    pFxDriverGlobals = GetDriverGlobals();

    committed = m_OwningObject->IsCommitted();

    //
    // If the object was not committed, then it was an FxObject derived
    // class that was *embedded* as a field in another structure or class.
    // As such, we are allowing one outstanding reference at this time.  We will
    // catch > 1 outstanding references below in the while loop.
    //
    if (committed) {
        if (m_Next != NULL || m_FailedCount != 0) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "Dropped references on a tag tracker, "
                                "show references with: !wdftagtracker %p", this);
            //
            // If this ASSERT fails, look in the history .. you'll
            // likely find that you released more references than you had
            //
            ASSERT(m_Next == NULL && m_FailedCount == 0);
        }
    }

    m_SpinLock.Acquire(&irql);

    current = m_Next;
    abandoned = 0;

    while (current != NULL) {
        next = current->Next;

        if (committed) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "Abandonded ref on object %p tag %p (%s @ %d)",
                                m_OwningObject, current->Tag, current->File, current->Line);
            abandoned++;
        }

        if (committed == FALSE) {
            //
            // The next time we encounter an abandoned reference, we will complain
            // about it...we have used up our allowance of one leaked reference
            // because the object is an embedded object.
            //
            // NOTE:  we might be eating the real outstanding reference here
            //        and not tracing it and then tracing the initial creation
            //        reference as the leaked reference which will be confusing.
            //        This is b/c there is no way to distinguish what is the
            //        tracking block used to track the creatio of the object.
            //
            committed = TRUE;

            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGDEVICE,
                                "Possibly Abandonded ref on object %p tag %p (%s @ %d).  "
                                "Is benign unless there are other outstanding leaked references.",
                                m_OwningObject, current->Tag, current->File, current->Line);
        }

        current = next;
    }

    m_SpinLock.Release(irql);

    ASSERTMSG("Abandoned tags on ref\n", abandoned == 0);
}


