#include "common/fxwmiirphandler.h"
#include "common/fxdevice.h"
#include "common/fxwmiprovider.h"
#include "common/fxwmiinstance.h"
#include <wmistr.h>


const FxWmiMinorEntry FxWmiIrpHandler::m_WmiDispatchTable[] =
{
    NULL
    // TODO: Fill table
};

FxWmiIrpHandler::FxWmiIrpHandler(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in FxDevice *Device,
    __in WDFTYPE Type
    ) :
    FxPackage(FxDriverGlobals, Device, Type),
    m_NumProviders(0), m_RegisteredState(WmiUnregistered),
    m_WorkItem(NULL), m_WorkItemEvent(NULL), m_WorkItemQueued(FALSE),
    m_UpdateCount(1) // bias m_UpdateCount to 1, Deregister routine will 
                     // decrement this.
{
    InitializeListHead(&m_ProvidersListHead);
}

FxWmiIrpHandler::~FxWmiIrpHandler()
{
    //
    // If the device could not get past AddDevice or failed the initial start
    // device, we will be unregistered.  Otherwise we should be cleaned up.
    //
    ASSERT(m_RegisteredState != WmiRegistered);

    ASSERT(IsListEmpty(&m_ProvidersListHead));

    if (m_WorkItem != NULL)
    {
        IoFreeWorkItem(m_WorkItem);
    }
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::Dispatch(
    __in PIRP Irp
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxWmiProvider* pProvider;
    FxWmiInstance* pInstance;
    PIO_STACK_LOCATION stack;
    PDEVICE_OBJECT pAttached;
    NTSTATUS status;
    PVOID pTag;
    ULONG instanceIndex;
    KIRQL irql;
    BOOLEAN handled, completeNow;
    UCHAR minor;

    pFxDriverGlobals = GetDriverGlobals();

    FX_TRACK_DRIVER(pFxDriverGlobals);

    stack = IoGetCurrentIrpStackLocation(Irp);
    minor = stack->MinorFunction;
    pTag = UlongToPtr(minor);
    status = Irp->IoStatus.Status;

    pProvider = NULL;
    pInstance = NULL;

    handled = FALSE;
    completeNow = FALSE;

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
        "WDFDEVICE 0x%p !devobj 0x%p IRP_MJ_SYSTEM_CONTROL, %!sysctrl! IRP 0x%p",
        m_Device->GetHandle(), m_Device->GetDeviceObject(), minor, Irp);

    //
    // Verify the minor code is within range, there is hole in the table at 0xA.
    // This check works around the hole in the table.
    //
    if (minor > IRP_MN_EXECUTE_METHOD && minor != IRP_MN_REGINFO_EX)
    {
        goto Done;
    }

    //
    // If the irp is not targetted at this device, send it down the stack
    //
    if (stack->Parameters.WMI.ProviderId != (UINT_PTR) m_Device->GetDeviceObject())
    {
        goto Done;
    }

    if (minor == IRP_MN_REGINFO || minor == IRP_MN_REGINFO_EX)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        Lock(&irql);

        pProvider = FindProviderLocked((LPGUID)stack->Parameters.WMI.DataPath);

        if (pProvider != NULL)
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            //
            // check for WMI tracing (no pProvider)
            //
            status = STATUS_WMI_GUID_NOT_FOUND;
        }

        if (NT_SUCCESS(status) && m_WmiDispatchTable[minor].CheckInstance)
        {
            PWNODE_SINGLE_INSTANCE pSingle;

            pSingle = (PWNODE_SINGLE_INSTANCE) stack->Parameters.WMI.Buffer;

            instanceIndex = pSingle->InstanceIndex;

            //
            // Also possible bits set in Flags related to instance names
            // WNODE_FLAG_PDO_INSTANCE_NAMES
            //
            if (pSingle->WnodeHeader.Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES)
            {
                //
                // Try to get the instance
                //
                pInstance = pProvider->GetInstanceReferencedLocked(
                    instanceIndex, pTag);

                if (pInstance == NULL)
                {
                    status = STATUS_WMI_INSTANCE_NOT_FOUND;
                }
            }
            else
            {
                status = STATUS_WMI_INSTANCE_NOT_FOUND;
            }
        }

        if (NT_SUCCESS(status))
        {
            pProvider->ADDREF(pTag);
        }
        else
        {
            //
            // NULL out the provider so we don't deref it later.  We could not
            // use if (pProvider != NULL && NT_SUCCESS(status) in the Done: block
            // because we could have success here, but the dispatch function
            // returns error.
            //
            pProvider = NULL;
        }

        Unlock(irql);

        if (!NT_SUCCESS(status))
        {
            Irp->IoStatus.Status = status;
            completeNow = TRUE;
        }
    }

    if (NT_SUCCESS(status) && m_WmiDispatchTable[minor].Handler != NULL)
    {
        status = m_WmiDispatchTable[minor].Handler(this,
                                                   Irp,
                                                   pProvider,
                                                   pInstance);
        handled = TRUE;
    }

Done:
    if (pInstance != NULL)
    {
        pInstance->RELEASE(pTag);
        pInstance = NULL;
    }

    if (pProvider != NULL)
    {
        pProvider->RELEASE(pTag);
        pProvider = NULL;
    }

    if (handled == FALSE)
    {
        pAttached = m_Device->GetAttachedDevice();
        if (completeNow || pAttached == NULL)
        {
            //
            // Sent to a PDO, error in the FDO handling, or controller devobj
            // style devobj, complete it here
            //
            status = Irp->IoStatus.Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
        else
        {
            //
            // Request sent to PNP device object that is not a PDO, send down
            // the stack
            //
            IoSkipCurrentIrpStackLocation(Irp);
            status = IoCallDriver(pAttached, Irp);
        }
    }

    //
    // Only release the remove lock *after* we have removed the thread entry
    // from the list because the list lifetime is tied to the FxDevice lifetime
    // and the remlock controls the lifetime of FxDevice.
    //
    // Since we never pend the wmi request, we can release the remove lock in
    // this Dispatch routine.  If we ever pended the IRPs, this would have to
    // have some more logic involved.
    //
    IoReleaseRemoveLock(
        &FxDevice::_GetFxWdmExtension(m_Device->GetDeviceObject())->IoRemoveLock,
        Irp
        );

    return status;
}

_Must_inspect_result_
FxWmiProvider*
FxWmiIrpHandler::FindProviderLocked(
    __in LPGUID Guid
    )
{
    FxWmiProvider* pFound;
    PLIST_ENTRY ple;

    pFound = NULL;

    for (ple = m_ProvidersListHead.Flink;
         ple != &m_ProvidersListHead;
         ple = ple->Flink)
    {

        FxWmiProvider* pProvider;

        pProvider = CONTAINING_RECORD(ple, FxWmiProvider, m_ListEntry);

        if (RtlCompareMemory(&pProvider->m_Guid,
                             Guid,
                             sizeof(GUID)) == sizeof(GUID))
        {
            pFound = pProvider;
            break;
        }
    }

    return pFound;
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::PostCreateDeviceInitialize(
    VOID
    )
{
    m_WorkItem = IoAllocateWorkItem(GetDevice()->GetDeviceObject());
    
    if (m_WorkItem == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::Register(
    VOID
    )
{
    NTSTATUS status;
    KIRQL irql;

    //
    // We rely on the PnP state machine to manage our state transitions properly
    // so that we don't have to do any state checking here.
    //
    Lock(&irql);
    ASSERT(m_RegisteredState == WmiUnregistered ||
              m_RegisteredState == WmiDeregistered);
    m_RegisteredState = WmiRegistered;
    Unlock(irql);

    status = IoWMIRegistrationControl(GetDevice()->GetDeviceObject(),
                                      WMIREG_ACTION_REGISTER);

    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "could not register WMI with OS, %!STATUS!", status);

        Lock(&irql);
        m_RegisteredState = WmiUnregistered;
        Unlock(irql);
    }

    return status;
}
