/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPkgFdo.cpp

Abstract:

    This module implements the pnp/power package for the driver
    framework.

Author:



Environment:

    Kernel mode only

Revision History:



--*/

#include "..\pnppriv.hpp"

#include <initguid.h>
#include <wdmguid.h>


#if defined(EVENT_TRACING)
// Tracing support
extern "C" {
#include "FxPkgFdoKm.tmh"
}
#endif

_Must_inspect_result_
NTSTATUS
FxPkgFdo::PnpFilterResourceRequirements(
    __inout FxIrp *Irp
    )

/*++

Routine Description:

    This method is invoked in response to a Pnp FilterResourceRequirements IRP.

Arguments:

    Device - a pointer to the FxDevice

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/

{
    PIO_RESOURCE_REQUIREMENTS_LIST pWdmRequirementsList;
    PIO_RESOURCE_REQUIREMENTS_LIST pNewWdmList;
    NTSTATUS status;
    FxIoResReqList *pIoResReqList;
    WDFIORESREQLIST reqlist;

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Entering FilterResourceRequirements handler");

    if (m_DeviceFilterRemoveResourceRequirements.m_Method != NULL) {

        pWdmRequirementsList = (PIO_RESOURCE_REQUIREMENTS_LIST) Irp->GetInformation();

        status = STATUS_INSUFFICIENT_RESOURCES;

        pIoResReqList = FxIoResReqList::_CreateFromWdmList(GetDriverGlobals(),
                                                           pWdmRequirementsList,
                                                           FxResourceAllAccessAllowed);

        if (pIoResReqList != NULL) {
            status = pIoResReqList->Commit(NULL, (PWDFOBJECT) &reqlist);

            // Commit should never fail because we own all object state
            ASSERT(NT_SUCCESS(status));
            UNREFERENCED_PARAMETER(status);

            status = m_DeviceFilterRemoveResourceRequirements.Invoke(
                m_Device->GetHandle(), pIoResReqList->GetHandle());

            if (NT_SUCCESS(status) && pIoResReqList->IsChanged()) {
                pNewWdmList = pIoResReqList->CreateWdmList();

                if (pNewWdmList != NULL) {
                    //
                    // List could be missing previously
                    //
                    if (pWdmRequirementsList != NULL) {
                        //
                        // Propagate BusNumber to our new list.
                        //
                        pNewWdmList->BusNumber = pWdmRequirementsList->BusNumber;

                        MxMemory::MxFreePool(pWdmRequirementsList);
                    }

                    Irp->SetInformation((ULONG_PTR) pNewWdmList);
                }
                else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            //
            // No matter what, free the resource requirements list object.  If
            // we need another one when adding resources, another one will be
            // allocated.
            //
            pIoResReqList->DeleteObject();
            pIoResReqList = NULL;
        }
    }
    else {
        //
        // No filtering on the way down, set status to STATUS_SUCCESS so we
        // send the irp down the stack.
        //
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status)) {
        status = SendIrpSynchronously(Irp);
    }

    //
    // If we do not handle the IRP on the way down and the PDO does not handle
    // the IRP, we can have a status of STATUS_NOT_SUPPORTED.  We still want to
    // process the irp in this state.
    //
    if (NT_SUCCESS(status) || status == STATUS_NOT_SUPPORTED) {
        NTSTATUS filterStatus;

        //
        // Give the Framework objects a pass at the list.
        //
        filterStatus = FxPkgPnp::FilterResourceRequirements(
            (PIO_RESOURCE_REQUIREMENTS_LIST*)(&Irp->GetIrp()->IoStatus.Information)
            );

        if (!NT_SUCCESS(filterStatus)) {
            status = filterStatus;
        }
        else if (m_DeviceFilterAddResourceRequirements.m_Method != NULL) {
            //
            // Now give the driver a shot at it.
            //
            pWdmRequirementsList = (PIO_RESOURCE_REQUIREMENTS_LIST)
                Irp->GetInformation();

            pIoResReqList = FxIoResReqList::_CreateFromWdmList(
                GetDriverGlobals(), pWdmRequirementsList, FxResourceAllAccessAllowed);

            if (pIoResReqList != NULL) {
                status = pIoResReqList->Commit(NULL, (PWDFOBJECT) &reqlist);
                UNREFERENCED_PARAMETER(status);

                //
                // Since we absolutely control the lifetime of pIoResReqList, this
                // should never fail
                //
                ASSERT(NT_SUCCESS(status));

                status = m_DeviceFilterAddResourceRequirements.Invoke(
                    m_Device->GetHandle(), reqlist);

                //
                // It is possible the child driver modified the resource list,
                // and if so we need to update the requirements list.
                //
                if (NT_SUCCESS(status) && pIoResReqList->IsChanged()) {
                    pNewWdmList = pIoResReqList->CreateWdmList();

                    if (pNewWdmList != NULL) {
                        //
                        // List could be missing previously
                        //
                        if (pWdmRequirementsList != NULL) {
                            //
                            // Propagate BusNumber to our new list.
                            //
                            pNewWdmList->BusNumber = pWdmRequirementsList->BusNumber;

                            ExFreePool(pWdmRequirementsList);
                        }

                        Irp->SetInformation((ULONG_PTR) pNewWdmList);
                    }
                    else {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                pIoResReqList->DeleteObject();
                pIoResReqList = NULL;
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    CompletePnpRequest(Irp, status);

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Exiting FilterResourceRequirements handler, %!STATUS!",
                        status);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::_PnpQueryCapabilitiesCompletionRoutine(
    __in    MdDeviceObject DeviceObject,
    __inout MdIrp Irp,
    __inout PVOID Context
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(Context);

    ASSERTMSG("Not implemented for KMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::PnpQueryCapabilities(
    __inout FxIrp *Irp
    )

/*++

Routine Description:

    This method is invoked in response to a Pnp QueryCapabilities IRP.

Arguments:

    Device - a pointer to the FxDevice

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/

{
    NTSTATUS status;

    HandleQueryCapabilities(Irp);

    status = SendIrpSynchronously(Irp);

    //
    // Now that the IRP has returned to us,  we modify what the bus driver
    // set up.
    //
    if (NT_SUCCESS(status)) {
        HandleQueryCapabilitiesCompletion(Irp);
    }

    CompletePnpRequest(Irp, status);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::_PnpQueryPnpDeviceStateCompletionRoutine(
    __in    MdDeviceObject DeviceObject,
    __inout MdIrp Irp,
    __inout PVOID Context
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(Context);

    ASSERTMSG("Not implemented for KMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::_PnpQueryPnpDeviceState(
    __inout FxPkgPnp* This,
    __inout FxIrp *Irp
    )

/*++

Routine Description:

    This method is invoked in response to a Pnp QueryPnpDeviceState IRP.

Arguments:

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/

{
    FxPkgFdo* pThis;
    NTSTATUS status;

    pThis = (FxPkgFdo*) This;

    status = pThis->SendIrpSynchronously(Irp);

    if (status == STATUS_NOT_SUPPORTED) {
        //
        // Morph into a successful code so that we process the request
        //
        status = STATUS_SUCCESS;
        Irp->SetStatus(status);
    }

    if (NT_SUCCESS(status)) {
        pThis->HandleQueryPnpDeviceStateCompletion(Irp);
    }
    else {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Lower stack returned error for query pnp device state, %!STATUS!",
            status);
    }

    //
    // Since we already sent the request down the stack, we must complete it
    // now.
    //
    return pThis->CompletePnpRequest(Irp, status);
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::Initialize(
    __in PWDFDEVICE_INIT DeviceInit
    )
/*++








Routine Description:

    After creating a FxPkgFdo, the driver writer will initialize it by passing
    a set of driver callbacks that allow the driver writer to customize the
    behavior when handling certain IRPs.

    This is the place to do any initialization that might fail.

Arguments:

    Device - a pointer to the FxDevice

    DispatchTable - a driver supplied table of callbacks

Returns:

    NTSTATUS

--*/
{
    PFX_DRIVER_GLOBALS pGlobals;
    WDF_CHILD_LIST_CONFIG config;
    size_t totalDescriptionSize = 0;
    WDFCHILDLIST hList;
    NTSTATUS status;

    pGlobals = GetDriverGlobals();

    status = FxPkgPnp::Initialize(DeviceInit);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = AllocateEnumInfo();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    #pragma prefast(suppress: __WARNING_PASSING_FUNCTION_UNEXPECTED_NULL, "Static child lists do not use the EvtChildListCreateDevice callback")
    WDF_CHILD_LIST_CONFIG_INIT(&config,
                               sizeof(FxStaticChildDescription),
                               NULL);

    status = FxChildList::_ComputeTotalDescriptionSize(pGlobals,
                                                       &config,
                                                       &totalDescriptionSize);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxChildList::_CreateAndInit(&m_StaticDeviceList,
                                         pGlobals,
                                         WDF_NO_OBJECT_ATTRIBUTES,
                                         totalDescriptionSize,
                                         m_Device,
                                         &config,
                                         TRUE);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = m_StaticDeviceList->Commit(WDF_NO_OBJECT_ATTRIBUTES,
                                        (WDFOBJECT*) &hList,
                                        m_Device);

    if (!NT_SUCCESS(status)) {
        m_StaticDeviceList->DeleteFromFailedCreate();
        m_StaticDeviceList = NULL;

        return status;
    }

    //
    // This will be released in the destructor
    //
    m_StaticDeviceList->ADDREF(this);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::QueryForDsfInterface(
    VOID
    )
{
    WDF_DSF_INTERFACE dsfInterface;
    NTSTATUS status;
    BOOLEAN derefQI = FALSE;

    RtlZeroMemory(&dsfInterface, sizeof(dsfInterface));

    //
    // Since there are some stacks that are not PnP re-entrant (like USBHUB,
    // xpsp2), we specify that the QI goes only to our attached device and
    // not to the top of the stack as a normal QI irp would.
    //
    // We also do this a preventative measure for other stacks we don't know
    // about internally and do not have access to when testing.
    //
    status = m_Device->QueryForInterface(&GUID_WDF_DSF_INTERFACE,
        (PINTERFACE) &dsfInterface,
        sizeof(dsfInterface),
        WDM_DSF_INTERFACE_V1_0,
        NULL,
        m_Device->GetAttachedDevice()
        );

    if (status == STATUS_NOT_SUPPORTED) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
            "Lower stack does not have a DSF interface");
        status = STATUS_SUCCESS;
        goto Done;
    }

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Lower stack returned an error for query DSF interface, %!STATUS!",
            status);
        goto Done;
    }

    derefQI = TRUE;

    //
    // Basic run time checks.
    //
    if (dsfInterface.Interface.Version != WDM_DSF_INTERFACE_V1_0) {
        status = STATUS_REVISION_MISMATCH;
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Lower DSF stack supports v(%x), requested v(%x), %!STATUS!",
            dsfInterface.Interface.Version,
            WDM_DSF_INTERFACE_V1_0,
            status);
        goto Done;
    }

    //
    // Ex functions should be both set or cleared.
    // Active/Inactive functions should be both set or cleared.
    // Ex function must be present.
    // Note: !!(ptr) expression below converts ptr value to true/false value.
    //       I.e., ptr==NULL to false and ptr!=NULL to true.
    //
    if (!((!!(dsfInterface.IoConnectInterruptEx) ==
              !!(dsfInterface.IoDisconnectInterruptEx)) &&
          (!!(dsfInterface.IoReportInterruptActive) ==
              !!(dsfInterface.IoReportInterruptInactive)) &&
          (dsfInterface.IoConnectInterruptEx != NULL)
              )) {
        status = STATUS_DATA_ERROR;
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Function mismatch detected in DSF interface, %!STATUS!",
            status);
        goto Done;
    }

    //
    // Info is correct.
    //
    m_IoConnectInterruptEx      = dsfInterface.IoConnectInterruptEx;
    m_IoDisconnectInterruptEx   = dsfInterface.IoDisconnectInterruptEx;

    //
    // If DSF interface provides active/inactive functions then use them
    //
    if (dsfInterface.IoReportInterruptActive != NULL)
    {
        m_IoReportInterruptActive   = dsfInterface.IoReportInterruptActive;
        m_IoReportInterruptInactive = dsfInterface.IoReportInterruptInactive;
    }

Done:

    //
    // The contract with the DSF layer is to release the interface right away;
    // the embedded interrupt function ptrs will be valid until this driver is
    // unloaded.
    //
    if (derefQI) {
        dsfInterface.Interface.InterfaceDereference(dsfInterface.Interface.Context);
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgFdo::AskParentToRemoveAndReenumerate(
    VOID
    )
/*++

Routine Description:
    This routine asks the PDO to ask its parent bus driver to Surprise-Remove
    and re-enumerate the PDO.  This will be done only at the point of
    catastrophic software failure, and occasionally after catastrophic hardware
    failure.

Arguments:
    None

Return Value:
    status

  --*/
{
    PREENUMERATE_SELF_INTERFACE_STANDARD pInterface;

    pInterface = &m_SurpriseRemoveAndReenumerateSelfInterface;

    if (pInterface->SurpriseRemoveAndReenumerateSelf != NULL) {
        pInterface->SurpriseRemoveAndReenumerateSelf(pInterface->Context);

        return STATUS_SUCCESS;
    }

    return STATUS_NOT_SUPPORTED;
}

