#include "common/fxchildlist.h"
#include "common/fxdevice.h"
#include <ntintsafe.h>


FxChildList::FxChildList(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in size_t TotalDescriptionSize,
    __in FxDevice* Device,
    __in BOOLEAN Static
    ) :
    FxNonPagedObject(FX_TYPE_CHILD_LIST,sizeof(FxChildList), FxDriverGlobals),
    m_TotalDescriptionSize(TotalDescriptionSize),
    m_EvtCreateDevice(FxDriverGlobals),
    m_EvtScanForChildren(FxDriverGlobals)
{
    //
    // Transaction link into list of FxChildList pointers maintained by
    // FxDevice's pnp package.
    //
    m_TransactionLink.SetTransactionedObject(this);

    m_Device = Device;

    m_IdentificationDescriptionSize = 0;
    m_AddressDescriptionSize = 0;

    m_EvtIdentificationDescriptionDuplicate = NULL;
    m_EvtIdentificationDescriptionCopy = NULL;
    m_EvtIdentificationDescriptionCleanup = NULL;
    m_EvtIdentificationDescriptionCompare = NULL;

    m_EvtAddressDescriptionDuplicate = NULL;
    m_EvtAddressDescriptionCopy = NULL;
    m_EvtAddressDescriptionCleanup = NULL;

    KeInitializeSpinLock(&m_ListLock);
    InitializeListHead(&m_DescriptionListHead);
    InitializeListHead(&m_ModificationListHead);

    m_State = ListUnlocked;

    m_InvalidationNeeded = FALSE;
    m_StaticList = Static;
    m_IsAdded = FALSE;

    m_EnumRetries = 0;

    m_ScanTag = NULL;
    m_ScanCount = 0;

    //
    // We want all waiters on the event to be satisfied, not just the first one
    //
    m_ScanEvent.Initialize(NotificationEvent, TRUE);

    MarkDisposeOverride(ObjectDoNotLock);
}

_Must_inspect_result_
VOID
FxChildList::Initialize(
    __in PWDF_CHILD_LIST_CONFIG Config
    )
{
    //
    // Driver cannot call WdfObjectDelete on this handle
    //
    MarkNoDeleteDDI();

    m_IdentificationDescriptionSize = Config->IdentificationDescriptionSize;
    m_AddressDescriptionSize = Config->AddressDescriptionSize;

    m_EvtCreateDevice.m_Method = Config->EvtChildListCreateDevice;
    m_EvtScanForChildren.m_Method = Config->EvtChildListScanForChildren;

    m_EvtIdentificationDescriptionDuplicate = Config->EvtChildListIdentificationDescriptionDuplicate;
    m_EvtIdentificationDescriptionCopy = Config->EvtChildListIdentificationDescriptionCopy;
    m_EvtIdentificationDescriptionCleanup = Config->EvtChildListIdentificationDescriptionCleanup;
    m_EvtIdentificationDescriptionCompare = Config->EvtChildListIdentificationDescriptionCompare;

    m_EvtAddressDescriptionDuplicate = Config->EvtChildListAddressDescriptionDuplicate;
    m_EvtAddressDescriptionCopy = Config->EvtChildListAddressDescriptionCopy;
    m_EvtAddressDescriptionCleanup = Config->EvtChildListAddressDescriptionCleanup;

    m_EvtChildListDeviceReenumerated = Config->EvtChildListDeviceReenumerated;

    //
    // Add this ChildList to the parent device's list of children lists.
    //
    m_Device->AddChildList(this);
    m_IsAdded = TRUE;
}

_Must_inspect_result_
NTSTATUS
FxChildList::_CreateAndInit(
    __out FxChildList** ChildList,
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_OBJECT_ATTRIBUTES ListAttributes,
    __in size_t TotalDescriptionSize,
    __in FxDevice* Device,
    __in PWDF_CHILD_LIST_CONFIG ListConfig,
    __in BOOLEAN Static
    )
{
    NTSTATUS ntStatus;
    FxChildList *childList = NULL;

    //
    // Initialize
    //
    *ChildList = NULL;

    //
    // Allocate a new child list object
    //
    childList = new(FxDriverGlobals, ListAttributes)
        FxChildList(FxDriverGlobals, TotalDescriptionSize, Device, Static);

    if (childList == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "No memory for WDFCHILDLIST handle, %!STATUS!",
                            ntStatus);
        goto exit;
    }

    //
    // Initialize the child list object
    //
    childList->Initialize(ListConfig);

    *ChildList = childList;
    ntStatus = STATUS_SUCCESS;

exit:
    if (!NT_SUCCESS(ntStatus))
    {
        if (NULL != childList)
        {
            childList->DeleteFromFailedCreate();
        }
    }
    return ntStatus;
}

_Must_inspect_result_
NTSTATUS
FxChildList::_ComputeTotalDescriptionSize(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_CHILD_LIST_CONFIG Config,
    __in size_t* TotalDescriptionSize
    )
{
    size_t addressAligned, idAligned;
    NTSTATUS status;

    *TotalDescriptionSize = 0;

    //
    // FxDeviceDescriptionEntry::operator new() will allocate a block of memory
    // that is
    // size =
    //  WDF_ALIGN_SIZE_UP(sizeof(FxDeviceDescriptionEntry), sizeof(PVOID)) +
    //  WDF_ALIGN_SIZE_UP(AddressDescriptionSize, sizeof(PVOID)) +
    //  WDF_ALIGN_SIZE_UP(IdentificationDescriptionSize, sizeof(PVOID));
    //
    // Validate the size now beforehand, not every we allocate
    //
    //
    // Test for overflow
    //
    idAligned = WDF_ALIGN_SIZE_UP(Config->IdentificationDescriptionSize,
                                  sizeof(PVOID));

    if (idAligned < Config->IdentificationDescriptionSize)
    {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Config->IdentificationDescriptionSize %d too large"
                            "%!STATUS!", Config->IdentificationDescriptionSize,
                            status);
        return status;
    }

    addressAligned = WDF_ALIGN_SIZE_UP(Config->AddressDescriptionSize,
                                       sizeof(PVOID));

    if (addressAligned < Config->AddressDescriptionSize)
    {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                        "Config->AddressDescriptionSize %d too large, %!STATUS!",
                        Config->AddressDescriptionSize, status);
        return status;
    }

    status = RtlSizeTAdd(
        WDF_ALIGN_SIZE_UP(sizeof(FxDeviceDescriptionEntry), sizeof(PVOID)),
        idAligned,
        TotalDescriptionSize
        );

    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Could not add ID description size to block size, %!STATUS!",
            status);
        return status;
    }

    status = RtlSizeTAdd(*TotalDescriptionSize,
                         addressAligned,
                         TotalDescriptionSize);

    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Could not add address description size to block size, %!STATUS!",
            status);
        return status;
    }

    return STATUS_SUCCESS;
}