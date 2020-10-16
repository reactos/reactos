/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPkgPdoKM.cpp

Abstract:

    This module implements the Pnp package for Pdo devices.

Author:



Environment:

    Kernel mode only

Revision History:



--*/

#include "../pnppriv.hpp"
#include <wdmguid.h>

// Tracing support
#if defined(EVENT_TRACING)
extern "C" {
#include "FxPkgPdoKM.tmh"
}
#endif

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_PnpQueryResources(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
{
    return ((FxPkgPdo*) This)->PnpQueryResources(Irp);
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::PnpQueryResources(
    __inout FxIrp *Irp
    )

/*++

Routine Description:

    This method is invoked in response to a Pnp QueryResources IRP.  We return
    the resources that the device is currently consuming.

Arguments:

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/

{
    FxCmResList *pResList = NULL;
    PCM_RESOURCE_LIST pWdmResourceList;
    WDFCMRESLIST list;
    NTSTATUS status;

    //
    // It is only necessary to create a collection if the caller is interested
    // in this callback.
    //
    if (m_DeviceResourcesQuery.m_Method == NULL) {
        return CompletePnpRequest(Irp, Irp->GetStatus());
    }

    pWdmResourceList = NULL;

    status = FxCmResList::_CreateAndInit(&pResList,
                                         GetDriverGlobals(),
                                         m_Device,
                                         WDF_NO_OBJECT_ATTRIBUTES,
                                         FxResourceAllAccessAllowed);
    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    status = pResList->Commit(NULL, (PWDFOBJECT) &list);

    if (NT_SUCCESS(status)) {
        status = m_DeviceResourcesQuery.Invoke(
            m_Device->GetHandle(), list);

        if (NT_SUCCESS(status)) {
            //
            // Walk the resource collection and create the appropriate
            // CM_RESOURCE_LIST.
            //
            if (pResList->Count()) {
                pWdmResourceList = pResList->CreateWdmList();
            }
            else {
                //
                // The driver didn't add any resources, so we'll just
                // ignore this Irp.
                //
                // Return that status that was passed in.
                //
                status = Irp->GetStatus();
                pWdmResourceList = (PCM_RESOURCE_LIST) Irp->GetInformation();
            }
        }
    }

    pResList->DeleteObject();

    exit:
        Irp->SetInformation((ULONG_PTR) pWdmResourceList);
        return CompletePnpRequest(Irp, status);
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_PnpQueryResourceRequirements(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
{
    return ((FxPkgPdo*) This)->PnpQueryResourceRequirements(Irp);
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::PnpQueryResourceRequirements(
    __inout FxIrp *Irp
    )

/*++

Routine Description:

    This method is invoked in response to a Pnp QueryResourceRequirements IRP.
    We return the set (of sets) of possible resources that we could accept
    which would allow our device to work.

Arguments:

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/

{
    PIO_RESOURCE_REQUIREMENTS_LIST pWdmRequirementsList;
    FxIoResReqList *pIoResReqList = NULL;
    PSINGLE_LIST_ENTRY ple;
    NTSTATUS status;

    status = STATUS_SUCCESS;

    m_DeviceInterfaceLock.AcquireLock(GetDriverGlobals());




    // We know for sure that the PDO is known to pnp now because it has received
    // this query resource requirements request.
    m_Device->m_PdoKnown = TRUE;

    //
    // Now that it is a known PDO, we can register interfaces on it whose
    // registration was delayed because the PDO was unknown at the time of the
    // call to WdfDeviceCreateDeviceInterface. If it is not the first time
    // then we re-register (see comments below for reason).
    //
    for (ple = m_DeviceInterfaceHead.Next; ple != NULL; ple = ple->Next) {
        FxDeviceInterface *pDeviceInterface;

        pDeviceInterface = FxDeviceInterface::_FromEntry(ple);

        //
        // At this time the interface may be in registered or unregistered state.
        // The reason being that pnp may unregister the interface from underneath
        // during the life of PDO (note that drivers never unregister explicitly
        // as there is no unregistration API, and the scenarios in which it can
        // happen are given below). Therefore WDF needs to re-register the interface,
        // otherwise driver may end up with an unregistered interface and fail to
        // enable interface.
        //
        // Pnp can unregister the interface in following cases:
        // 1. The driver is uninstalled, and re-installed
        //    In this case, the stack is torn down but the PDO is not
        //    deleted. Pnp deletes the interface registration, however WDF
        //    doesn't delete the interface structure because it is deleted as
        //    part of PDO deletion in destructor. When the driver is re-installed
        //    Pnp sends another query resource requirements irp and WDF needs to
        //    re-register at this time.
        //
        // 2. Pnp couldn't find a driver and loaded a NULL driver while
        //    waiting for reinstall to happen from WU. This is similar to case above
        //    except that the pnp activities are transparent to user.
        //    In this case, PDO was never started. However it did get
        //    the query resource requirements irp and therefore its interface
        //    was registered by WDF. Pnp deleted the interface registration
        //    when installing NULL driver. When pnp finally finds a driver from
        //    WU, it sends another query resource requirement irp. At this time,
        //    WDF needs to re-register.
        //
        // In both the above cases, WDF has to re-register (and free previous
        // sym link) when query resource requirements irp arrives again. Note
        // that Pnp doesn't delete the interface registration during disable,
        // s/w surprise-removal, or resource rebalance. In case of resource
        // rebalance, query resource requirement irp is sent after stop, and
        // WDF will end up registering again even though pnp did not unregister
        // the interface. This is fine because kernel API for registration
        // allows multiple calls to register, and in case registration already
        // exists it returns informational status (not error status)
        // STATUS_OBJECT_NAME_EXISTS and also returns the same symbolic link.
        //
        //
        // Free the symbolic link if already present
        //
        if (pDeviceInterface->m_SymbolicLinkName.Buffer != NULL) {
            RtlFreeUnicodeString(&pDeviceInterface->m_SymbolicLinkName);
            RtlZeroMemory(&pDeviceInterface->m_SymbolicLinkName,
                          sizeof(pDeviceInterface->m_SymbolicLinkName));
        }

        //
        // Register. Note that if the interface was already registered, the
        // call to IoRegisterDeviceInterface will return informational status
        // (not error status) STATUS_OBJECT_NAME_EXISTS and also return the
        // symbolic link.
        //
        status = pDeviceInterface->Register(
            m_Device->GetPhysicalDevice()
            );

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "could not register device interface on PDO WDFDEVICE %p, "
                "!devobj %p, failing IRP_MN_QUERY_RESOURCE_REQUIREMENTS %!STATUS!",
                m_Device->GetHandle(),
                m_Device->GetDeviceObject(), status);
            break;
        }
    }

    m_DeviceInterfaceLock.ReleaseLock(GetDriverGlobals());

    if (!NT_SUCCESS(status)) {
        return CompletePnpRequest(Irp, status);
    }

    //
    // Driver writer is not interested in this callback, forgoe the allocation
    // of the FxCollection and complete the request here.
    //
    if (m_DeviceResourceRequirementsQuery.m_Method == NULL) {
        return CompletePnpRequest(Irp, status);
    }

    pWdmRequirementsList = NULL;

    //
    // Create a collection which will be populated by the
    // bus driver.
    //
    status = FxIoResReqList::_CreateAndInit(&pIoResReqList,
                                            GetDriverGlobals(),
                                            WDF_NO_OBJECT_ATTRIBUTES,
                                            FxResourceAllAccessAllowed);
    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    WDFIORESREQLIST reqlist;

    //
    // Get a handle to the collection that can be passed to the driver.
    //
    status = pIoResReqList->Commit(NULL, (PWDFOBJECT) &reqlist);

    //
    // We control the object's state, this should never fail
    //
    ASSERT(NT_SUCCESS(status));
    UNREFERENCED_PARAMETER(status);

    //
    // Call the driver.  The driver will populate the resource collection
    // with a set of child collections which will contain each of the
    // possible resource assignments.
    //
    status = m_DeviceResourceRequirementsQuery.Invoke(
        m_Device->GetHandle(), reqlist);

    if (NT_SUCCESS(status)) {
        if (pIoResReqList->Count()) {
            //
            // Create a IO_RESOURCE_REQUIREMENTS_LIST based on the
            // contents of our collection.
            //
            pWdmRequirementsList = pIoResReqList->CreateWdmList();

            if (pWdmRequirementsList != NULL) {
                Irp->SetInformation((ULONG_PTR) pWdmRequirementsList);
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else {
            //
            // The driver didn't add any resources, so we'll just
            // ignore this request.
            //
            // Return the status that was passed in.
            //
            status = Irp->GetStatus();
        }
    }

    pIoResReqList->DeleteObject();

    exit:
        return CompletePnpRequest(Irp, status);
}

_Must_inspect_result_
NTSTATUS
FxPkgPdo::_PnpFilterResourceRequirements(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )
/*++

Routine Description:
    Filter resource requirements for the PDO.  A chance to further muck with
    the resources assigned to the device.

Arguments:
    This - the package

    Irp - the request

Return Value:
    NTSTATUS

  --*/
{
    NTSTATUS status;

    //
    // Give the Framework objects a pass at the list.
    //
    status = ((FxPkgPdo*) This)->FilterResourceRequirements(
        (PIO_RESOURCE_REQUIREMENTS_LIST*)(&Irp->GetIrp()->IoStatus.Information)
        );

    if (NT_SUCCESS(status)) {
        //
        // Upon successful internal filtering, return the embedded status.
        //
        status = Irp->GetStatus();
    }
    else {
        //
        // Only on failure do we change the status of the irp
        //
        Irp->SetStatus(status);
    }

    return ((FxPkgPdo*) This)->CompletePnpRequest(Irp, status);
}

