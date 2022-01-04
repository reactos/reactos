/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    FxChildList.hpp

Abstract:

    Defines the interface for asynchronously creating and destroying child
    devices

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXDEVICELIST_H_
#define _FXDEVICELIST_H_

enum FxChildListIteratorIndexValues {
    DescriptionIndex = 0,
    ModificationIndex,
};

struct FxChildListCreateDeviceCallback : public FxCallback {

    FxChildListCreateDeviceCallback(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxCallback(FxDriverGlobals),
        m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFCHILDLIST DeviceList,
        __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
        __in PWDFDEVICE_INIT ChildInit
        )
    {
        NTSTATUS status;

        CallbackStart();
        status = m_Method(DeviceList, IdentificationDescription, ChildInit);
        CallbackEnd();

        return status;
    }

    PFN_WDF_CHILD_LIST_CREATE_DEVICE m_Method;
};

struct FxChildListScanForChildrenCallback : public FxCallback {

    FxChildListScanForChildrenCallback(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxCallback(FxDriverGlobals),
        m_Method(NULL)
    {
    }

    VOID
    Invoke(
        __in WDFCHILDLIST DeviceList
        )
    {
        if (m_Method != NULL) {
            CallbackStart();
            m_Method(DeviceList);
            CallbackEnd();
        }
    }

    PFN_WDF_CHILD_LIST_SCAN_FOR_CHILDREN m_Method;
};

enum FxChildListState {
    ListUnlocked = 1,
    ListLockedForEnum,
    ListLockedForParentRemove,
};

enum FxChildListScanTagStates {
    ScanTagUndefined = 0,
    ScanTagActive,
    ScanTagCancelled,
    ScanTagFinished,
};

class FxChildList : public FxNonPagedObject {
    friend struct FxDeviceDescriptionEntry;

public:

    static
    _Must_inspect_result_
    NTSTATUS
    _CreateAndInit(
        __out FxChildList** ChildList,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PWDF_OBJECT_ATTRIBUTES ListAttributes,
        __in size_t TotalDescriptionSize,
        __in CfxDevice* Device,
        __in PWDF_CHILD_LIST_CONFIG ListConfig,
        __in BOOLEAN Static = FALSE
        );

    WDFCHILDLIST
    GetHandle(
        VOID
        )
    {
        return (WDFCHILDLIST) GetObjectHandle();
    }

    MxEvent*
    GetScanEvent(
        VOID
        )
    {
        return m_ScanEvent.GetSelfPointer();
    }

    WDFDEVICE
    GetDevice(
        VOID
        );

    CfxDevice*
    GetDeviceFromId(
        __inout PWDF_CHILD_RETRIEVE_INFO Info
        );

    _Must_inspect_result_
    NTSTATUS
    GetAddressDescription(
        __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
        __out PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
        );

    VOID
    GetAddressDescriptionFromEntry(
        __in FxDeviceDescriptionEntry* Entry,
        __out PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
        );

    VOID
    BeginScan(
        __out_opt PULONG ScanTag = NULL
        );

    VOID
    EndScan(
        __inout_opt PULONG ScanTag = NULL
        );

    VOID
    CancelScan(
        __in BOOLEAN EndTheScan,
        __inout_opt PULONG ScanTag
        );

    VOID
    BeginIteration(
        __inout PWDF_CHILD_LIST_ITERATOR Iterator
        );

    VOID
    EndIteration(
        __inout PWDF_CHILD_LIST_ITERATOR Iterator
        );

    VOID
    InitIterator(
        __inout PWDF_CHILD_LIST_ITERATOR Iterator
        );

    _Must_inspect_result_
    NTSTATUS
    GetNextDevice(
        __out WDFDEVICE* Device,
        __inout PWDF_CHILD_LIST_ITERATOR Iterator,
        __inout_opt PWDF_CHILD_RETRIEVE_INFO Info
        );

    WDFDEVICE
    GetNextStaticDevice(
        __in WDFDEVICE PreviousDevice,
        __in ULONG Flags
        );

    BOOLEAN
    IsScanCancelled(
        __in PULONG ScanTag
        )
    {
        return *ScanTag == ScanTagCancelled ? TRUE : FALSE;
    }

    _Must_inspect_result_
    NTSTATUS
    UpdateAsMissing(
        __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Description
        );

    _Must_inspect_result_
    NTSTATUS
    UpdateDeviceAsMissing(
        __in CfxDevice* Device
        );

    VOID
    ReenumerateEntry(
        __inout FxDeviceDescriptionEntry* Entry
        );

    VOID
    UpdateAllAsPresent(
        __in_opt PULONG ScanTag = NULL
        );

    _Must_inspect_result_
    NTSTATUS
    Add(
        __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
        __in_opt PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription,
        __in_opt PULONG ScanTag = NULL
        );

    VOID
    UpdateAddressDescriptionFromEntry(
        __inout FxDeviceDescriptionEntry* Entry,
        __in PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
        );

    BOOLEAN
    HasAddressDescriptions(
        VOID
        )
    {
        return m_AddressDescriptionSize > 0 ? TRUE : FALSE;
    }

    ULONG
    GetAddressDescriptionSize(
        VOID
        )
    {
        return m_AddressDescriptionSize;
    }

    ULONG
    GetIdentificationDescriptionSize(
        VOID
        )
    {
        return m_IdentificationDescriptionSize;
    }

    VOID
    CopyId(
        __out PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Dest,
        __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Source
        )
    {
        if (m_EvtIdentificationDescriptionCopy != NULL) {
            m_EvtIdentificationDescriptionCopy(GetHandle(), Source, Dest);
        }
        else {
            RtlCopyMemory(Dest, Source, m_IdentificationDescriptionSize);
        }
    }

    _Must_inspect_result_
    NTSTATUS
    ProcessBusRelations(
        __inout PDEVICE_RELATIONS *DeviceRelations
        );

    VOID
    InvokeReportedMissingCallback(
        VOID
        );

    VOID
    PostParentToD0(
        VOID
        );

    VOID
    IndicateWakeStatus(
        __in NTSTATUS WakeWakeStatus
        );

    VOID
    ScanForChildren(
        VOID
        )
    {
        m_EvtScanForChildren.Invoke(GetHandle());
    }

    //
    // This notification is for the parent device.
    // Child pnp remove notifications occur on the FxDeviceDescriptionEntry
    //
    VOID
    NotifyDeviceSurpriseRemove(
        VOID
        );

    //
    // This notification is for the parent device.
    // Child pnp remove notifications occur on the FxDeviceDescriptionEntry
    //
    VOID
    NotifyDeviceRemove(
        __inout PLONG ChildCount
        );

    BOOLEAN
    IsStaticList(
        VOID
        )
    {
        return m_StaticList;
    }

    static
    _Must_inspect_result_
    NTSTATUS
    _ValidateConfig(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PWDF_CHILD_LIST_CONFIG Config,
        __in size_t* TotalDescriptionSize
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _ComputeTotalDescriptionSize(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PWDF_CHILD_LIST_CONFIG Config,
        __in size_t* TotalDescriptionSize
        );

    static
    size_t
    _ComputeRelationsSize(
        __in ULONG Count
        )
    {
        if (Count == 0) {
            return sizeof(((PDEVICE_RELATIONS) NULL)->Count);
        }
        else {
            return sizeof(DEVICE_RELATIONS) + (Count-1)*sizeof(PDEVICE_OBJECT);
        }
    }

    static
    FxChildList*
    _FromEntry(
        __in FxTransactionedEntry* Entry
        )
    {
        return CONTAINING_RECORD(Entry, FxChildList, m_TransactionLink);
    }

    ULONG
    GetScanCount(
        VOID
        )
    {
        return m_ScanCount;
    }

protected:

    FxChildList(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in size_t TotalDescriptionSize,
        __in CfxDevice* Device,
        __in BOOLEAN Static
        );

    virtual
    BOOLEAN
    Dispose(
        VOID
        );

    VOID
    Initialize(
        __in PWDF_CHILD_LIST_CONFIG Config
        );

    BOOLEAN
    ReenumerateEntryLocked(
        __inout FxDeviceDescriptionEntry* Entry,
        __in BOOLEAN FromQDR
        );

    BOOLEAN
    CloneEntryLocked(
        __inout PLIST_ENTRY FreeListHead,
        __inout FxDeviceDescriptionEntry* Entry,
        __in BOOLEAN FromQDR
        );

    VOID
    ProcessModificationsLocked(
        __inout PLIST_ENTRY FreeListHead
        );

    VOID
    MarkDescriptionNotPresentWorker(
        __inout FxDeviceDescriptionEntry* DescriptionEntry,
        __in BOOLEAN ModificationCanBeQueued
        );

    VOID
    MarkModificationNotPresentWorker(
        __inout PLIST_ENTRY FreeListHead,
        __inout FxDeviceDescriptionEntry* ModificationEntry
        );

    VOID
    DrainFreeListHead(
        __inout PLIST_ENTRY FreeListHead
        );

    FxDeviceDescriptionEntry*
    SearchBackwardsForMatchingModificationLocked(
        __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Id
        );

    FxDeviceDescriptionEntry*
    SearchBackwardsForMatchingDescriptionLocked(
        __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Id
        );

    _Must_inspect_result_
    NTSTATUS
    VerifyDescriptionEntry(
        __in PLIST_ENTRY Entry
        );

    _Must_inspect_result_
    NTSTATUS
    VerifyModificationEntry(
        __in PLIST_ENTRY Entry
        );

    BOOLEAN
    CreateDevice(
        __inout FxDeviceDescriptionEntry* Entry,
        __inout PBOOLEAN InvalidateRelations
        );

    _Must_inspect_result_
    NTSTATUS
    DuplicateId(
        __out PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Dest,
        __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Source
        )
    {
        if (m_EvtIdentificationDescriptionDuplicate != NULL) {
            return m_EvtIdentificationDescriptionDuplicate(GetHandle(),
                                                           Source,
                                                           Dest);
        }
        else {
            RtlCopyMemory(Dest, Source, m_IdentificationDescriptionSize);
            return STATUS_SUCCESS;
        }
    }

    BOOLEAN
    CompareId(
        __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Lhs,
        __in PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Rhs
        )
    {
        if (m_EvtIdentificationDescriptionCompare != NULL) {
            return m_EvtIdentificationDescriptionCompare(GetHandle(),
                                                         Lhs,
                                                         Rhs);
        }
        else {
            return (m_IdentificationDescriptionSize ==
                    RtlCompareMemory(Lhs,
                                     Rhs,
                                     m_IdentificationDescriptionSize)) ? TRUE
                                                                       : FALSE;
        }
    }

    _Must_inspect_result_
    NTSTATUS
    DuplicateAddress(
        __out PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER Dest,
        __in PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER Source
        )
    {
        if (m_EvtAddressDescriptionDuplicate != NULL) {
            return m_EvtAddressDescriptionDuplicate(GetHandle(), Source, Dest);
        }
        else {
            RtlCopyMemory(Dest, Source, m_AddressDescriptionSize);
            return STATUS_SUCCESS;
        }
    }

    VOID
    CopyAddress(
        __out PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER Dest,
        __in_opt PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER Source
        )
    {
        if (Source != NULL) {
            if (m_EvtAddressDescriptionCopy != NULL) {
                m_EvtAddressDescriptionCopy(GetHandle(), Source, Dest);
            }
            else {
                RtlCopyMemory(Dest, Source, m_AddressDescriptionSize);
            }
        }
    }

    VOID
    CleanupDescriptions(
        __in_opt PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdDescription,
        __in_opt PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddrDescription
        )
    {
        if (m_EvtAddressDescriptionCleanup != NULL && AddrDescription != NULL) {
            m_EvtAddressDescriptionCleanup(GetHandle(), AddrDescription);
        }

        if (m_EvtIdentificationDescriptionCleanup != NULL && IdDescription != NULL) {
            m_EvtIdentificationDescriptionCleanup(GetHandle(), IdDescription);
        }
    }

public:
    //
    // Link into list of FxChildList pointers maintained by the package.  This
    // differs from IFxStateChangeNotification's  link because that one links
    // into the list of pnp state change notifications for the device we are
    // associated with.
    //
    FxTransactionedEntry m_TransactionLink;

protected:
    size_t m_TotalDescriptionSize;

    ULONG m_IdentificationDescriptionSize;

    ULONG m_AddressDescriptionSize;

    FxChildListCreateDeviceCallback m_EvtCreateDevice;

    FxChildListScanForChildrenCallback m_EvtScanForChildren;

    //
    // These callbacks are not wrapped in FxCallback derived structures becase
    // they are called under the framework locking and not in a locking model
    // in which the driver writer can configure.
    //
    // copy one ID description to another
    PFN_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_COPY m_EvtIdentificationDescriptionCopy;

    // duplicate one ID description to another
    PFN_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_DUPLICATE m_EvtIdentificationDescriptionDuplicate;

    // cleanup an ID description
    PFN_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_CLEANUP m_EvtIdentificationDescriptionCleanup;

    // compare to ID descriptions
    PFN_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_COMPARE m_EvtIdentificationDescriptionCompare;

    // copy one address description to another
    PFN_WDF_CHILD_LIST_ADDRESS_DESCRIPTION_COPY m_EvtAddressDescriptionCopy;

    // duplicate one address description to another
    PFN_WDF_CHILD_LIST_ADDRESS_DESCRIPTION_DUPLICATE m_EvtAddressDescriptionDuplicate;

    // cleanup an address description
    PFN_WDF_CHILD_LIST_ADDRESS_DESCRIPTION_CLEANUP m_EvtAddressDescriptionCleanup;

    // clone notification
    PFN_WDF_CHILD_LIST_DEVICE_REENUMERATED m_EvtChildListDeviceReenumerated;

    KSPIN_LOCK m_ListLock;

    LIST_ENTRY m_DescriptionListHead;

    LIST_ENTRY m_ModificationListHead;

    FxChildListState m_State;

    BOOLEAN m_InvalidationNeeded;

    BOOLEAN m_StaticList;

    //
    // Whether the child list is added to the enumerated children list.
    //
    BOOLEAN m_IsAdded;

    UCHAR m_EnumRetries;

    PULONG m_ScanTag;

    ULONG m_ScanCount;

    MxEvent m_ScanEvent;
};

// begin_wpp enum
enum FxChildListModificationState {
    ModificationUnspecified = 0,
    ModificationInsert,
    ModificationRemove,
    ModificationRemoveNotify,
    ModificationClone,
    ModificationNeedsPnpRemoval,
};
// end_wpp

// begin_wpp enum
enum FxChildListDescriptionState {
    DescriptionUnspecified = 0,
    DescriptionPresentNeedsInstantiation,
    DescriptionInstantiatedHasObject,
    DescriptionReportedMissing,
    DescriptionNotPresent,
};
// end_wpp

// begin_wpp enum
enum FxChildListReportedMissingCallbackState : UCHAR {
    CallbackStateUnspecified = 0,
    CallbackNeedsToBeInvoked,
    CallbackInvoked,
};
// end_wpp

enum FxChildListValues {
    FX_CHILD_LIST_MAX_RETRIES = 3
};

struct FxDeviceDescriptionEntry : public FxStump {
    friend class FxDevice;
    friend class FxChildList;

public:
    FxDeviceDescriptionEntry(
        __inout FxChildList* DeviceList,
        __in ULONG AddressDescriptionSize,
        __in ULONG IdentificationDescriptionSize
        );

    ~FxDeviceDescriptionEntry();

    _Must_inspect_result_
    PVOID
    operator new(
        __in size_t AllocatorBlock,
        __in PFX_DRIVER_GLOBALS DriverGlobals,
        __in size_t TotalDescriptionSize
        );

    FxChildList*
    GetParentList(
        VOID
        )
    {
        return m_DeviceList;
    }


    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER
    GetId(
        VOID
        )
    {
        return m_IdentificationDescription;
    }

    BOOLEAN
    IsDeviceReportedMissing(
        VOID
        );

    BOOLEAN
    IsDeviceRemoved(
        VOID
        );

    VOID
    ProcessDeviceRemoved(
        VOID
        );

    VOID
    DeviceSurpriseRemoved(
        VOID
        );

    _Must_inspect_result_
    FxDeviceDescriptionEntry*
    Clone(
        __inout PLIST_ENTRY FreeListHead
        );

    BOOLEAN
    MatchStateToFlags(
        __in ULONG Flags
        )
    {
        if (((Flags & WdfRetrievePresentChildren) &&
                        m_DescriptionState == DescriptionInstantiatedHasObject)
            ||
            ((Flags & WdfRetrieveMissingChildren) &&
                       (m_DescriptionState == DescriptionReportedMissing ||
                        m_DescriptionState == DescriptionNotPresent))
            ||
            ((Flags & WdfRetrievePendingChildren) &&
                    m_DescriptionState == DescriptionPresentNeedsInstantiation)
            ) {

            return TRUE;
        }

        return FALSE;
    }

protected:
    BOOLEAN
    __inline
    IsPresent(
        VOID
        )
    {
        if (m_DescriptionState == DescriptionPresentNeedsInstantiation ||
            m_DescriptionState == DescriptionInstantiatedHasObject) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }

    static
    FxDeviceDescriptionEntry*
    _FromDescriptionLink(
        __in PLIST_ENTRY Link
        )
    {
        return CONTAINING_RECORD(Link,
                                 FxDeviceDescriptionEntry,
                                 m_DescriptionLink);
    }

    static
    FxDeviceDescriptionEntry*
    _FromModificationLink(
        __in PLIST_ENTRY Link
        )
    {
        return CONTAINING_RECORD(Link,
                                 FxDeviceDescriptionEntry,
                                 m_ModificationLink);
    }

protected:
    LIST_ENTRY m_DescriptionLink;

    FxChildListDescriptionState m_DescriptionState;

    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER m_IdentificationDescription;

    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER m_AddressDescription;

    LIST_ENTRY m_ModificationLink;

    FxChildListModificationState m_ModificationState;

    CfxDevice* m_Pdo;

    FxChildList* m_DeviceList;

    BOOLEAN m_FoundInLastScan;

    BOOLEAN m_ProcessingSurpriseRemove;

    BOOLEAN m_PendingDeleteOnScanEnd;

    FxChildListReportedMissingCallbackState m_ReportedMissingCallbackState;
};

#endif // _FXDEVICELIST_H_
