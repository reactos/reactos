/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxTagTracker.hpp

Abstract:

    This is the C++ header for the FxTagTracker

Author:




Revision History:











--*/

#ifndef _FXTAGTRACKER_HPP_
#define _FXTAGTRACKER_HPP_

extern "C" {

#if defined(EVENT_TRACING)
#include "FxTagTracker.hpp.tmh"
#endif

}

enum FxTagTrackerType : UCHAR {
    FxTagTrackerTypeHandle = 0,
    FxTagTrackerTypePower
};

enum FxTagRefType {
   TagAddRef = 0,
   TagRelease
};

#define FRAMES_TO_CAPTURE 16
#define FRAMES_TO_SKIP 3

struct FxTagTrackingStackFrames : public FxStump {
    USHORT NumFrames;
    ULONG64 Frames[FRAMES_TO_CAPTURE];
};

//
// Tracks outstanding references
//

struct FxTagTrackingBlock : public FxStump {

    FxTagTrackingBlock(
        __in        PVOID Tag,
        __in        LONG Line,
        __in_opt    PSTR File,
        __in_opt    BOOLEAN Initial = FALSE
        ) :
        Tag(Tag),
        Line(Line),
        File(File),
        StackFrames(NULL),
        Next(NULL)
    {
        Mx::MxQueryTickCount(&TimeLocked);

        if (Initial == FALSE) {
            //
            // !wdftagtracker identifies a reference with Line == 0
            // as the initial reference. However, references taken
            // in framework code may leave the File/Line empty yet
            // not be the initial references. They could still
            // capture stack frames, so they are useful to track.
            //
            // Leaving File == NULL but setting Line = 1 unless this
            // was explicitly labeled an Initial Ref works around this.
            //
            if (File == NULL && Line == 0) {
                this->Line = 1;
            }
        }
    }

    ~FxTagTrackingBlock(
        )
    {
        if (StackFrames != NULL) {
            delete StackFrames;
            StackFrames = NULL;
        }
    }

    struct FxTagTrackingBlock* Next;
    PVOID Tag;
    PCHAR File;
    LONG Line;
    LARGE_INTEGER TimeLocked;
    FxTagTrackingStackFrames* StackFrames;
};

//
// Tracks reference and release history
//
struct FxTagHistory {
    FxTagRefType RefType;
    //
    // Note: RefCount may be inaccurate when multiple FxObject::Release
    //       calls execute concurrently. This value should be considered
    //       an approximation and is for wdfkd consumption only.
    //
    ULONG RefCount;
    PCHAR File;
    LONG Line;
    PVOID Tag;
    LARGE_INTEGER Time;
    FxTagTrackingStackFrames* StackFrames;

    FxTagHistory(
        )
    {
        StackFrames = NULL;
    }

    ~FxTagHistory(
        )
    {
        if (StackFrames != NULL) {
            delete StackFrames;
            StackFrames = NULL;
        }
    }
};


#define TAG_HISTORY_DEPTH (25)

class FxTagTracker : public FxGlobalsStump {

private:

    //
    // Making constructor private to enforce usage
    // of CreateAndInitialize
    //

    FxTagTracker(
        __in        PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in        FxTagTrackerType Type,
        __in        BOOLEAN CaptureStack,
        __in        FxObject* Owner,
        __in_opt    PVOID CreateTag = NULL
        ) :
        FxGlobalsStump(FxDriverGlobals),
        m_TrackerType(Type),
        m_CaptureStack(CaptureStack),
        m_Next(NULL),
        m_FailedCount(0),
        m_CurRefHistory(0),
        m_OwningObject(Owner)
    {
        RtlZeroMemory(m_TagHistory, sizeof(m_TagHistory));

        //
        // We keep handle reference trackers in a list,
        // which wdfkd uses to identify potential handle leaks.
        //
        if (m_TrackerType == FxTagTrackerTypeHandle) {
            FxDriverGlobalsDebugExtension* pExtension;
            KIRQL irql;

            pExtension = GetDriverGlobals()->DebugExtension;
            ASSERT(pExtension != NULL);

            //
            // Insert the tag tracker into the list of allocated trackers
            //
            pExtension->AllocatedTagTrackersLock.Acquire(&irql);
            InsertTailList(&pExtension->AllocatedTagTrackersListHead,
                           &m_TrackerEntry);
            pExtension->AllocatedTagTrackersLock.Release(irql);

            //
            // Handle references default to 1 outstanding ref (the object creation ref)
            //
            m_Next = new(FxDriverGlobals) FxTagTrackingBlock(CreateTag, 0, NULL, TRUE);
            if (m_Next == NULL) {
                m_FailedCount = 1;
            }
        }
        else {
            ASSERT(m_TrackerType == FxTagTrackerTypePower);
        }
    }

    VOID
    CopyStackFrames(
        _Inout_ FxTagTrackingStackFrames** StackFrames,
        _In_ USHORT NumFrames,
        _In_reads_(NumFrames) PVOID* Frames
        );

public:

    _Must_inspect_result_
    static
    NTSTATUS
    __inline
    CreateAndInitialize(
        __out       FxTagTracker ** TagTracker,
        __in        PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in        FxTagTrackerType Type,
        __in        BOOLEAN CaptureStack,
        __in        FxObject* Owner,
        __in_opt    PVOID CreateTag = NULL
        )
    {
        NTSTATUS status;

        FxTagTracker * tagTracker = new(FxDriverGlobals)
            FxTagTracker(FxDriverGlobals, Type, CaptureStack, Owner, CreateTag);

        if (NULL == tagTracker) {
            status = STATUS_INSUFFICIENT_RESOURCES;

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "Failed to allocate tag tracker, returning %!STATUS!", status);

            goto exit;
        }

        *TagTracker = tagTracker;
        status = STATUS_SUCCESS;

    exit:
        return status;
    }

    ~FxTagTracker();

    VOID
    CheckForAbandondedTags(
        VOID
        );

    VOID
    UpdateTagHistory(
        __in        PVOID Tag,
        __in        LONG Line,
        __in_opt    PSTR File,
        __in        FxTagRefType RefType,
        __in        ULONG RefCount
        );

protected:
    //
    // Whether this tracks handle references or power references.
    //
    FxTagTrackerType m_TrackerType;

    //
    // Whether to capture stack frames for each ref/release.
    //
    BOOLEAN m_CaptureStack;

    //
    // Owner object for this FxTagTracker. An FxDevice if this tracks power refs.
    //
    FxObject* m_OwningObject;

    //
    // Link into list of allocated tag trackers kept in the driver's globals
    //
    LIST_ENTRY m_TrackerEntry;

    //
    // Number of times we failed to alloc a tracking block
    //
    LONG m_FailedCount;

    //
    // Lock to guard the insertion/removal of tracking blocks
    //
    MxLock m_SpinLock;

    //
    // List head for tracking blocks
    //
    FxTagTrackingBlock* m_Next;

    //
    // Last TAG_HISTORY_DEPTH addrefs and releases on the object
    //
    FxTagHistory m_TagHistory[TAG_HISTORY_DEPTH];

    //
    // Current index into RefHistory
    //
    LONG m_CurRefHistory;
};

#endif // _FXTAGTRACKER_HPP_

