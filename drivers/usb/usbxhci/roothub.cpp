/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCX root hub callbacks
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbxhci.h"

#define NDEBUG
#include <debug.h>

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
XhciRootHubCreate(
    _In_ PXHCI_CONTROLLER Controller)
{
    UCX_ROOTHUB_CONFIG Config;
    NTSTATUS Status;

    UCX_ROOTHUB_CONFIG_INIT(&Config,
                            XhciEvtRootHubClearHubFeature,
                            XhciEvtRootHubClearPortFeature,
                            XhciEvtRootHubGetHubStatus,
                            XhciEvtRootHubGetPortStatus,
                            XhciEvtRootHubSetHubFeature,
                            XhciEvtRootHubSetPortFeature,
                            XhciEvtRootHubGetPortErrorCount,
                            XhciEvtRootHubInterruptTx,
                            XhciEvtRootHubGetInfo,
                            XhciEvtRootHubGet20PortInfo,
                            XhciEvtRootHubGet30PortInfo);

    Status = UcxRootHubCreate(Controller->UcxController,
                              &Config,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &Controller->UcxRootHub);
    if (!NT_SUCCESS(Status))
        DPRINT1("UcxRootHubCreate failed 0x%lx\n", Status);

    return Status;
}

/* Hub class requests addressed to the root hub */

VOID
NTAPI
XhciEvtRootHubClearHubFeature(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    UNREFERENCED_PARAMETER(UcxRootHub);
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}

VOID
NTAPI
XhciEvtRootHubClearPortFeature(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    UNREFERENCED_PARAMETER(UcxRootHub);
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}

VOID
NTAPI
XhciEvtRootHubGetHubStatus(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    UNREFERENCED_PARAMETER(UcxRootHub);
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}

VOID
NTAPI
XhciEvtRootHubGetPortStatus(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    UNREFERENCED_PARAMETER(UcxRootHub);
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}

VOID
NTAPI
XhciEvtRootHubSetHubFeature(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    UNREFERENCED_PARAMETER(UcxRootHub);
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}

VOID
NTAPI
XhciEvtRootHubSetPortFeature(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    UNREFERENCED_PARAMETER(UcxRootHub);
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}

VOID
NTAPI
XhciEvtRootHubGetPortErrorCount(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    UNREFERENCED_PARAMETER(UcxRootHub);
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}

/* Status change pipe and port layout queries */

VOID
NTAPI
XhciEvtRootHubInterruptTx(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    UNREFERENCED_PARAMETER(UcxRootHub);

    /* Park this until a port change event arrives */
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}

VOID
NTAPI
XhciEvtRootHubGetInfo(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    UNREFERENCED_PARAMETER(UcxRootHub);

    /* Fill ROOTHUB_INFO from the Supported Protocol capabilities */
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}

VOID
NTAPI
XhciEvtRootHubGet20PortInfo(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    UNREFERENCED_PARAMETER(UcxRootHub);
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}

VOID
NTAPI
XhciEvtRootHubGet30PortInfo(
    _In_ UCXROOTHUB UcxRootHub,
    _In_ WDFREQUEST Request)
{
    UNREFERENCED_PARAMETER(UcxRootHub);
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}
