#ifndef _FXCHILDLIST_H_
#define _FXCHILDLIST_H_

#include "common/fxstump.h"
#include "common/fxcallback.h"
#include "common/fxnonpagedobject.h"
#include "common/mxevent.h"
#include "common/fxtransactionedlist.h"


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
        if (m_Method != NULL)
        {
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

class FxChildList;

struct FxDeviceDescriptionEntry : public FxStump {
    
    friend class FxDevice;
    friend class FxChildList;

public:
    FxDeviceDescriptionEntry(
        __inout FxChildList* DeviceList,
        __in ULONG IdentificationDescriptionSize,
        __in ULONG AddressDescriptionSize
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

protected:
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

    //FxChildListDescriptionState m_DescriptionState;

    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER m_IdentificationDescription;

    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER m_AddressDescription;

    LIST_ENTRY m_ModificationLink;

    //FxChildListModificationState m_ModificationState;

    CfxDevice* m_Pdo;

    FxChildList* m_DeviceList;

    BOOLEAN m_FoundInLastScan;

    BOOLEAN m_ProcessingSurpriseRemove;

    BOOLEAN m_PendingDeleteOnScanEnd;

    //FxChildListReportedMissingCallbackState m_ReportedMissingCallbackState;
};

class FxChildList : public FxNonPagedObject {

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

    static
    _Must_inspect_result_
    NTSTATUS
    _ComputeTotalDescriptionSize(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PWDF_CHILD_LIST_CONFIG Config,
        __in size_t* TotalDescriptionSize
        );

    WDFCHILDLIST
    GetHandle(
        VOID
        )
    {
        return (WDFCHILDLIST) GetObjectHandle();
    }

    VOID
    ScanForChildren(
        VOID
        )
    {
        m_EvtScanForChildren.Invoke(GetHandle());
    }

    VOID
    PostParentToD0(
        VOID
        );

protected:

    FxChildList(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in size_t TotalDescriptionSize,
        __in CfxDevice* Device,
        __in BOOLEAN Static
        );

    VOID
    Initialize(
        __in PWDF_CHILD_LIST_CONFIG Config
        );
        

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

#endif //_FXCHILDLIST_H_
