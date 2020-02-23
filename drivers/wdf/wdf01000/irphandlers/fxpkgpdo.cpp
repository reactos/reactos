#include "common/fxpkgpdo.h"
#include "common/fxdevicetext.h"



const PFN_PNP_POWER_CALLBACK FxPkgPdo::m_PdoPnpFunctionTable[IRP_MN_SURPRISE_REMOVAL + 1] =
{
    // TODO: Implement next functions

    /*_PnpStartDevice,                // IRP_MN_START_DEVICE
    _PnpQueryRemoveDevice,          // IRP_MN_QUERY_REMOVE_DEVICE
    _PnpRemoveDevice,               // IRP_MN_REMOVE_DEVICE
    _PnpCancelRemoveDevice,         // IRP_MN_CANCEL_REMOVE_DEVICE
    _PnpStopDevice,                 // IRP_MN_STOP_DEVICE
    _PnpQueryStopDevice,            // IRP_MN_QUERY_STOP_DEVICE
    _PnpCancelStopDevice,           // IRP_MN_CANCEL_STOP_DEVICE

    _PnpQueryDeviceRelations,       // IRP_MN_QUERY_DEVICE_RELATIONS
    _PnpQueryInterface,             // IRP_MN_QUERY_INTERFACE
    _PnpQueryCapabilities,          // IRP_MN_QUERY_CAPABILITIES
    _PnpQueryResources,             // IRP_MN_QUERY_RESOURCES
    _PnpQueryResourceRequirements,  // IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    _PnpQueryDeviceText,            // IRP_MN_QUERY_DEVICE_TEXT
    _PnpFilterResourceRequirements, // IRP_MN_FILTER_RESOURCE_REQUIREMENTS

    _PnpCompleteIrp,                // 0x0E

    _PnpCompleteIrp,                // IRP_MN_READ_CONFIG
    _PnpCompleteIrp,                // IRP_MN_WRITE_CONFIG
    _PnpEject,                      // IRP_MN_EJECT
    _PnpSetLock,                    // IRP_MN_SET_LOCK
    _PnpQueryId,                    // IRP_MN_QUERY_ID
    _PnpQueryPnpDeviceState,        // IRP_MN_QUERY_PNP_DEVICE_STATE
    _PnpQueryBusInformation,        // IRP_MN_QUERY_BUS_INFORMATION
    _PnpDeviceUsageNotification,    // IRP_MN_DEVICE_USAGE_NOTIFICATION
    _PnpSurpriseRemoval,            // IRP_MN_SURPRISE_REMOVAL*/
};

const PFN_PNP_POWER_CALLBACK  FxPkgPdo::m_PdoPowerFunctionTable[IRP_MN_QUERY_POWER + 1] =
{
    // TODO: Implement next functions

    //_DispatchWaitWake,      // IRP_MN_WAIT_WAKE
    //_DispatchPowerSequence, // IRP_MN_POWER_SEQUENCE
    //_DispatchSetPower,      // IRP_MN_SET_POWER
    //_DispatchQueryPower,    // IRP_MN_QUERY_POWER
};

FxPkgPdo::FxPkgPdo(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in CfxDevice *Device
    ) :
    FxPkgPnp(FxDriverGlobals, Device, FX_TYPE_PACKAGE_PDO)
/*++

Routine Description:

    This is the constructor for the FxPkgPdo.  Don't do any initialization
    that might fail here.

Arguments:

    none

Returns:

    none

--*/

{
    m_DeviceTextHead.Next = NULL;

    m_DeviceID   = NULL;
    m_InstanceID = NULL;
    m_HardwareIDs = NULL;
    m_CompatibleIDs = NULL;
    m_ContainerID = NULL;
    m_IDsAllocation = NULL;

    m_RawOK = FALSE;
    m_Static = FALSE;
    m_AddedToStaticList = FALSE;

    //
    // By default the PDO is the owner of wait wake irps (only case where this
    // wouldn't be the case is for a bus filter to be sitting above us).
    //
    m_SharedPower.m_WaitWakeOwner = TRUE;

    m_Description = NULL;
    m_OwningChildList = NULL;

    //m_EjectionDeviceList = NULL;
    m_DeviceEjectProcessed = NULL;

    m_CanBeDeleted = FALSE;
    m_EnableWakeAtBusInvoked = FALSE;
}

FxPkgPdo::~FxPkgPdo(
    VOID
    )
/*++

Routine Description:

    This is the destructor for the FxPkgPdo

Arguments:

    none

Returns:

    none

--*/
{
    //
    // If IoCreateDevice fails on a dynamically created PDO, m_Description will
    // be != NULL b/c we will not go through the pnp state machine in its entirety
    // for cleanup.  FxChildList will need a valid m_Description to cleanup upon
    // failure from EvtChildListDeviceCrate, so we just leave m_Description alone
    // here if != NULL.
    //
    // ASSERT(m_Description == NULL);

    FxDeviceText::_CleanupList(&m_DeviceTextHead);

    //
    // This will free the underlying memory for m_DeviceID, m_InstanceID,
    // m_HardwareIDs, m_CompatibleIDs and m_ContainerID
    //
    if (m_IDsAllocation != NULL)
    {
        FxPoolFree(m_IDsAllocation);
        m_IDsAllocation = NULL;
    }

    if (m_OwningChildList != NULL)
    {
        m_OwningChildList->RELEASE(this);
    }

    if (m_EjectionDeviceList != NULL)
    {
        delete m_EjectionDeviceList;
        m_EjectionDeviceList = NULL;
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::FireAndForgetIrp(
    __inout FxIrp *Irp
    )
/*++

Routine Description:

    Virtual override for sending a request down the stack and forgetting about
    it.  Since we are the bottom of the stack, just complete the request.

Arguments:

Return Value:

--*/
{
    NTSTATUS status;

    status = Irp->GetStatus();

    if (Irp->GetMajorFunction() == IRP_MJ_POWER)
    {
        return CompletePowerRequest(Irp, status);
    }
    else
    {
        return CompletePnpRequest(Irp, status);
    }
}
