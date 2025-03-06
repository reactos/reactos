/*
 * PROJECT:     ReactOS USB xHCI Miniport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBXHCI main driver functions
 * COPYRIGHT:   Copyright 2023 Ian Marco Moffett <ian@vegaa.systems>
 */

#include "usbxhci.h"

#define NDEBUG
#include <debug.h>

#define XHCI_POLL_TIME_SET(Milliseconds) (Milliseconds * 10000)

/*
 * Do not use this directly!!! Use XHCI_GetPortSc()
 * instead.
 *
 * The spec states that the address of port status
 * and control (which can be many as many ports exist)
 * is Operational Base + (400h + (10h * (n–1)))
 * where: n = Port Number (Valid values are 1, 2, 3, ...MaxPorts
 */
#define XHCI_PORTSC(OperationalBase, PortNo) \
    (PVOID)((ULONG_PTR)OperationalBase + (0x400 + (0x10 * (PortNo - 1))))

/* Status codes for XHCI_PollCheck() */
#define POLL_STATUS_DONE     0      // Done polling successfully
#define POLL_STATUS_CONTINUE 1      // Nothing happened, continue polling
#define POLL_STATUS_TIMEOUT  2      // Timeout occurred

typedef LONG POLLSTATUS;

USBPORT_REGISTRATION_PACKET RegPacket;

/**
 * XHCI_PollTimeout - Poll with timeout
 *
 * This function polls `PollValue` until set or a timeout
 * happens.
 *
 * @CurrentTime: Used to see how far we are.
 * @EndTime: Max amount of time to wait, must be set by caller.
 * @Condition: Condition to test.
 */
POLLSTATUS
NTAPI
XHCI_PollTimeout(
    _In_ PLARGE_INTEGER EndTime,
    _In_ BOOLEAN Condition)
{
    LARGE_INTEGER CurrentTime;

    KeQuerySystemTime(&CurrentTime);

    if (Condition)
    {
        return POLL_STATUS_DONE;
    }

    if (CurrentTime.QuadPart >= EndTime->QuadPart)
    {
        return POLL_STATUS_TIMEOUT;
    }

    return MP_STATUS_ERROR;
}

PXHCI_PORTSC
NTAPI
XHCI_GetPortSc(
    _In_ PXHCI_HC_OPER_REGS OperRegs,
    _In_ DWORD PortNo,
    _In_ PVOID XhciExtension)
{
    PXHCI_EXTENSION XhciExt;

    XhciExt = XhciExtension;

    NT_ASSERTMSG("Invalid port number of '0'", PortNo != 0);
    NT_ASSERTMSG("Port number too high!", PortNo <= XhciExt->NumberOfPorts);

    return XHCI_PORTSC(OperRegs, PortNo);
}

/**
 * XHCI_InitPorts - Initialize USB ports
 *
 * @OperRegs: Operational registers
 * @XhciExtension: XhciExtension instance
 *
 * TODO: All this function does for now is give debugging
 *       information about each port. Implement logic to
 *       reset ports and initialize them.
 */
VOID
NTAPI
XHCI_InitPorts(
    _In_ PXHCI_HC_OPER_REGS OperRegs,
    _In_ PVOID XhciExtension)
{
    PXHCI_EXTENSION XhciExt;
    PXHCI_PORTSC PortScPtr;
    XHCI_PORTSC PortSc;

    XhciExt = XhciExtension;

    for (DWORD i = 1; i <= XhciExt->NumberOfPorts; ++i)
    {
        PortScPtr = XHCI_GetPortSc(OperRegs, i, XhciExtension);
        PortSc.AsULONG = READ_REGISTER_ULONG(&PortScPtr->AsULONG);

        /* XXX: Maybe remove this? */
        if (PortSc.CurrentConnectStatus && PortSc.DeviceRemovable)
        {
            DPRINT("Port %d has removable device connected\n", i);
        }
        else if (PortSc.CurrentConnectStatus)
        {
            DPRINT("Port %d has non-removable device connected\n", i);
        }

        PortSc.PortReset = 1;
        WRITE_REGISTER_ULONG(&PortScPtr->AsULONG, PortSc.AsULONG);
    }
}

MPSTATUS
NTAPI
XHCI_HaltController(_In_ PXHCI_HC_OPER_REGS OperRegisters)
{
    XHCI_USB_STATUS Status;
    XHCI_USB_COMMAND Command;
    LARGE_INTEGER EndTime;
    BOOLEAN IsPolling;

    /* Don't halt if Status.HCHalted is set */
    Status.AsULONG = READ_REGISTER_ULONG(&OperRegisters->UsbStatus.AsULONG);
    if (Status.HcHalted)
    {
        DPRINT("XHCI_HaltController: HC already halted; pretending\n");
        return MP_STATUS_ERROR;
    }

    Command.AsULONG = READ_REGISTER_ULONG(&OperRegisters->UsbCmd.AsULONG);
    Command.RunStop = 0;

    WRITE_REGISTER_ULONG(&OperRegisters->UsbCmd.AsULONG,
                         Command.AsULONG);

    /*
     * According to the spec, the HC should
     * halt within about 16ms. To be sure,
     * we can give it 24ms and see what
     * happens. Will assume a dead/dying
     * HC on failure although a delay
     * could be for many reasons e.g
     * HC quirks.
     *
     * XXX: Should probably handle quirks
     */
    KeQuerySystemTime(&EndTime);
    EndTime.QuadPart += XHCI_POLL_TIME_SET(24);
    IsPolling = TRUE;

    while (IsPolling)
    {
        Status.AsULONG = READ_REGISTER_ULONG(&OperRegisters->UsbStatus.AsULONG);
        switch (XHCI_PollTimeout(&EndTime, Status.HcHalted))
        {
            case POLL_STATUS_DONE:
                IsPolling = FALSE;
                break;
            case POLL_STATUS_TIMEOUT:
                DPRINT1("XHCI_HaltController: Timeout while halting HC\n");
                return MP_STATUS_ERROR;
        }
    }

    DPRINT("XHCI_HaltController: HC is now in a halted state\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_ResetController(_In_ PXHCI_HC_OPER_REGS OperRegisters)
{
    XHCI_USB_COMMAND Command;
    XHCI_USB_STATUS Status;
    LARGE_INTEGER EndTime;
    BOOLEAN IsPolling;

    Command.AsULONG = READ_REGISTER_ULONG(&OperRegisters->UsbCmd.AsULONG);
    Command.HcReset = 1;

    /* The HC should be halted before a reset */
    XHCI_HaltController(OperRegisters);

    DPRINT("XHCI_ResetController: RESET\n");
    WRITE_REGISTER_ULONG(&OperRegisters->UsbCmd.AsULONG,
                         Command.AsULONG);

    /*
     * The spec does not specify
     * how long we must wait before
     * the HC becomes ready again.
     * 25ms should suffice.
     */
    KeQuerySystemTime(&EndTime);
    EndTime.QuadPart += XHCI_POLL_TIME_SET(25);
    IsPolling = TRUE;

    while (IsPolling)
    {
        Status.AsULONG = READ_REGISTER_ULONG(&OperRegisters->UsbStatus.AsULONG);
        switch (XHCI_PollTimeout(&EndTime, !Status.ControllerNotReady))
        {
            case POLL_STATUS_DONE:
                IsPolling = FALSE;
                break;
            case POLL_STATUS_TIMEOUT:
                DPRINT1("XHCI_ResetController: Timeout while resetting HC\n");
                return MP_STATUS_ERROR;
        }
    }

    DPRINT("XHCI_ResetController: Reset complete\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_InitController(
    _In_ PXHCI_HC_OPER_REGS OperRegisters,
    _In_ PUSBPORT_RESOURCES Resources,
    _In_ PVOID XhciExtension,
    _In_ PXHCI_HC_CAPABILITY_REGISTERS CapabilityRegisters,
    _In_ UCHAR CapLength)
{
    PXHCI_HC_RESOURCES HcResources;
    PXHCI_EXTENSION XhciExt;
    XHCI_HC_STRUCTURAL_PARAMS1 StructParams1;
    XHCI_HC_CONFIG Config;
    ULONG DcbaaBasePA;
    ULONG CommandRingBasePA;
    ULONG_PTR BaseVA;
    ULONG_PTR BasePA;
    UCHAR MaxDeviceSlots;
    MPSTATUS RetStatus;

    BaseVA = Resources->StartVA;
    BasePA = Resources->StartPA;

    HcResources = (PXHCI_HC_RESOURCES)BaseVA;
    DcbaaBasePA = (ULONG)BasePA + FIELD_OFFSET(XHCI_HC_RESOURCES, Dcbaa);
    CommandRingBasePA = (ULONG)BasePA + FIELD_OFFSET(XHCI_HC_RESOURCES, CommandRing);

    RtlZeroMemory(HcResources, sizeof(*HcResources));

    /*
     * Ensure our Dcbaa + other
     * specific structures are on
     * a 64-byte boundary or *weird*
     * things may happen.
     */
    NT_ASSERTMSG("Dcbaa unaligned", (DcbaaBasePA & 0x3F) == 0);
    NT_ASSERTMSG("Command ring unaligned", (CommandRingBasePA & 0x3F) == 0);

    WRITE_REGISTER_ULONG(&OperRegisters->DcbaaPtr, DcbaaBasePA);
    WRITE_REGISTER_ULONG(&OperRegisters->CmdRingControl, CommandRingBasePA);

    XhciExt = XhciExtension;

    StructParams1.AsULONG = READ_REGISTER_ULONG(&CapabilityRegisters->StructParams1.AsULONG);
    MaxDeviceSlots = StructParams1.MaxDeviceSlots;

    DPRINT("XHCI_StartController: CapabilityRegisters - %p\n", CapabilityRegisters);
    DPRINT("XHCI_StartController: OperRegisters - %p\n", OperRegisters);
    DPRINT("XHCI_StartController: HC supports max %d device slot(s)\n", MaxDeviceSlots);

    XhciExt->NumberOfPorts = StructParams1.MaxPorts;

    /* xHCI spec says we need to perform a chip hardware reset */
    RetStatus = XHCI_ResetController(OperRegisters);
    if (RetStatus != MP_STATUS_SUCCESS)
    {
        return RetStatus;
    }

    Config.AsULONG = READ_REGISTER_ULONG(&OperRegisters->Config.AsULONG);

    /* Enable all device slots */
    Config.MaxSlotsEn = MaxDeviceSlots;
    DPRINT("XHCI_StartController: %d device slot(s) are to be used\n", Config.MaxSlotsEn);

    WRITE_REGISTER_ULONG(&OperRegisters->Config.AsULONG, Config.AsULONG);

    /* TODO: There is much more to do */

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_StartController(
    _In_ PVOID XhciExtension,
    _In_ PUSBPORT_RESOURCES Resources)
{
    PXHCI_HC_OPER_REGS OperRegisters;
    PXHCI_HC_CAPABILITY_REGISTERS CapabilityRegisters;
    XHCI_USB_COMMAND Command;
    XHCI_USB_STATUS Status;
    BOOLEAN IsPolling;
    LARGE_INTEGER EndTime;

    MPSTATUS RetStatus;
    UCHAR CapLength;

    DPRINT("XHCI_StartController: Starting...\n");

    /* Check resources */
    if ((Resources->ResourcesTypes & (USBPORT_RESOURCES_MEMORY | USBPORT_RESOURCES_INTERRUPT)) !=
                                     (USBPORT_RESOURCES_MEMORY | USBPORT_RESOURCES_INTERRUPT))
    {
        DPRINT1("XHCI_StartController: Controller does not meet resource requirements (have %x)...\n",
                Resources->ResourcesTypes);

        return MP_STATUS_ERROR;
    }

    CapabilityRegisters = Resources->ResourceBase;
    CapLength = READ_REGISTER_UCHAR(&CapabilityRegisters->CapLength);
    OperRegisters = (PXHCI_HC_OPER_REGS)((ULONG_PTR)CapabilityRegisters +
                                                    CapLength);

    Command.AsULONG = READ_REGISTER_ULONG(&OperRegisters->UsbCmd.AsULONG);

    RetStatus = XHCI_InitController(OperRegisters,
                                    Resources,
                                    XhciExtension,
                                    CapabilityRegisters,
                                    CapLength);

    if (RetStatus != MP_STATUS_SUCCESS)
    {
        return RetStatus;
    }

    Command.RunStop = 1;
    WRITE_REGISTER_ULONG(&OperRegisters->UsbCmd.AsULONG, Command.AsULONG);

    /*
     * Just to be safe, we will wait 16ms
     * to allow the HC to start up
     *
     * XXX: Is it risky not to wait?
     */
    KeQuerySystemTime(&EndTime);
    EndTime.QuadPart += XHCI_POLL_TIME_SET(16);

    IsPolling = TRUE;
    while (IsPolling)
    {
        Status.AsULONG = READ_REGISTER_ULONG(&OperRegisters->UsbStatus.AsULONG);
        switch (XHCI_PollTimeout(&EndTime, !Status.HcHalted))
        {
            case POLL_STATUS_DONE:
                IsPolling = FALSE;
                break;
            case POLL_STATUS_TIMEOUT:
                DPRINT1("XHCI_StartController: Timeout while starting HC\n");
                return MP_STATUS_ERROR;
        }
    }

    /* Startup the ports now */
    XHCI_InitPorts(OperRegisters, XhciExtension);

    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
XHCI_StopController(
    _In_ PVOID XhciExtension,
    _In_ BOOLEAN DisableInterrupts)
{
    DPRINT("XHCI_StopController: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
XHCI_OpenEndpoint(
    _In_ PVOID XhciExtension,
    _In_ PUSBPORT_ENDPOINT_PROPERTIES EndpointProps,
    _In_ PVOID Endpoint)
{
    DPRINT("XHCI_OpenEndpoint: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_FAILURE;
}

VOID
NTAPI
XHCI_QueryEndpointRequirements(
    _In_ PVOID XhciExtension,
    _In_ PUSBPORT_ENDPOINT_PROPERTIES EndpointProps,
    _In_ PUSBPORT_ENDPOINT_REQUIREMENTS EndpointRequirements)
{
    DPRINT("XHCI_QueryEndpointRequirements: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
XHCI_SuspendController(_In_ PVOID XhciExtension)
{
    DPRINT("XHCI_SuspendController: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
XHCI_ResumeController(_In_ PVOID XhciExtension)
{
    DPRINT("XHCI_SuspendController: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

BOOLEAN
NTAPI
XHCI_InterruptService(_In_ PVOID XhciExtension)
{
    DPRINT("XHCI_InterruptService: UNIMPLEMENTED. FIXME\n");
    return FALSE;
}

VOID
NTAPI
XHCI_InterruptDpc(
    _In_ PVOID XhciExtension,
    _In_ BOOLEAN EnableInterrupts)
{
    DPRINT("XHCI_InterruptDpc: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
XHCI_SubmitTransfer(
    _In_ PVOID XhciExtension,
    _In_ PVOID XhciEndpoint,
    _In_ PUSBPORT_TRANSFER_PARAMETERS TransferParameters,
    _In_ PVOID XhciTransfer,
    _In_ PUSBPORT_SCATTER_GATHER_LIST SgList)
{
    DPRINT("XHCI_SubmitTransfer: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_SubmitIsoTransfer(
    _In_ PVOID XhciExtension,
    _In_ PVOID XhciEndpoint,
    _In_ PUSBPORT_TRANSFER_PARAMETERS TransferParameters,
    _In_ PVOID XhciTransfer,
    _In_ PVOID isoParameters)
{
    DPRINT("XHCI_SubmitIsoTransfer: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
XHCI_AbortTransfer(
    _In_ PVOID XhciExtension,
    _In_ PVOID XhciEndpoint,
    _In_ PVOID XhciTransfer,
    _In_ PULONG CompletedLength)
{
    DPRINT("XHCI_AbortTransfer: UNIMPLEMENTED. FIXME\n");
}

ULONG
NTAPI
XHCI_GetEndpointState(
    _In_ PVOID XhciExtension,
    _In_ PVOID XhciEndpoint)
{
    DPRINT("XHCI_GetEndpointState: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
XHCI_SetEndpointState(
    _In_ PVOID XhciExtension,
    _In_ PVOID XhciEndpoint,
    _In_ ULONG EndpointState)
{
    DPRINT("XHCI_SetEndpointState: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
XHCI_PollEndpoint(
    _In_ PVOID XhciExtension,
    _In_ PVOID XhciEndpoint)
{
    DPRINT("XHCI_PollEndpoint: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
XHCI_CheckController(_In_ PVOID XhciExtension)
{
    DPRINT("XHCI_CheckController: UNIMPLEMENTED. FIXME\n");
}

ULONG
NTAPI
XHCI_Get32BitFrameNumber(_In_ PVOID XhciExtension)
{
    DPRINT("XHCI_Get32BitFrameNumber: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
XHCI_InterruptNextSOF(_In_ PVOID XhciExtension)
{
    DPRINT("XHCI_InterruptNextSOF: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
XHCI_EnableInterrupts(_In_ PVOID XhciExtension)
{
    DPRINT("XHCI_EnableInterrupts: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
XHCI_DisableInterrupts(_In_ PVOID XhciExtension)
{
    DPRINT("XHCI_DisableInterrupts: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
XHCI_PollController(_In_ PVOID XhciExtension)
{
    DPRINT("XHCI_PollController: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
XHCI_SetEndpointDataToggle(
    _In_ PVOID XhciExtension,
    _In_ PVOID XhciEndpoint,
    _In_ ULONG DataToggle)
{
    DPRINT("XHCI_SetEndpointDataToggle: UNIMPLEMENTED. FIXME\n");
}

ULONG
NTAPI
XHCI_GetEndpointStatus(
    _In_ PVOID XhciExtension,
    _In_ PVOID XhciEndpoint)
{
    DPRINT("XHCI_GetEndpointStatus: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
XHCI_SetEndpointStatus(
    _In_ PVOID XhciExtension,
    _In_ PVOID XhciEndpoint,
    _In_ ULONG EndpointStatus)
{
    DPRINT("XHCI_SetEndpointStatus: UNIMPLEMENTED. FIXME\n");
}

NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    DPRINT("DriverEntry: DriverObject - %p, RegistryPath - %wZ\n", DriverObject, RegistryPath);

    RtlZeroMemory(&RegPacket, sizeof(RegPacket));
    RegPacket.MiniPortVersion = USB_MINIPORT_VERSION_XHCI;
    RegPacket.StartController = XHCI_StartController;
    RegPacket.StopController = XHCI_StopController;
    RegPacket.OpenEndpoint = XHCI_OpenEndpoint;
    RegPacket.QueryEndpointRequirements = XHCI_QueryEndpointRequirements;
    RegPacket.SuspendController = XHCI_SuspendController;
    RegPacket.ResumeController = XHCI_ResumeController;
    RegPacket.InterruptService = XHCI_InterruptService;
    RegPacket.InterruptDpc = XHCI_InterruptDpc;
    RegPacket.SubmitTransfer = XHCI_SubmitTransfer;
    RegPacket.AbortTransfer = XHCI_AbortTransfer;
    RegPacket.GetEndpointState = XHCI_GetEndpointState;
    RegPacket.SetEndpointState = XHCI_SetEndpointState;
    RegPacket.PollEndpoint = XHCI_PollEndpoint;
    RegPacket.CheckController = XHCI_CheckController;
    RegPacket.Get32BitFrameNumber = XHCI_Get32BitFrameNumber;
    RegPacket.InterruptNextSOF = XHCI_InterruptNextSOF;
    RegPacket.EnableInterrupts = XHCI_EnableInterrupts;
    RegPacket.DisableInterrupts = XHCI_DisableInterrupts;
    RegPacket.PollController = XHCI_PollController;
    RegPacket.SetEndpointDataToggle = XHCI_SetEndpointDataToggle;
    RegPacket.GetEndpointStatus = XHCI_GetEndpointStatus;
    RegPacket.SetEndpointStatus = XHCI_SetEndpointStatus;
    RegPacket.RH_GetRootHubData = XHCI_RH_GetRootHubData;
    RegPacket.RH_GetStatus = XHCI_RH_GetStatus;
    RegPacket.RH_GetPortStatus = XHCI_RH_GetPortStatus;
    RegPacket.RH_GetHubStatus = XHCI_RH_GetHubStatus;
    RegPacket.RH_SetFeaturePortReset = XHCI_RH_SetFeaturePortReset;
    RegPacket.RH_SetFeaturePortPower = XHCI_RH_SetFeaturePortPower;
    RegPacket.RH_SetFeaturePortEnable = XHCI_RH_SetFeaturePortEnable;
    RegPacket.RH_SetFeaturePortSuspend = XHCI_RH_SetFeaturePortSuspend;
    RegPacket.RH_ClearFeaturePortEnable = XHCI_RH_ClearFeaturePortEnable;
    RegPacket.RH_ClearFeaturePortPower = XHCI_RH_ClearFeaturePortPower;
    RegPacket.RH_ClearFeaturePortSuspend = XHCI_RH_ClearFeaturePortSuspend;
    RegPacket.RH_ClearFeaturePortEnableChange = XHCI_RH_ClearFeaturePortEnableChange;
    RegPacket.RH_ClearFeaturePortConnectChange = XHCI_RH_ClearFeaturePortConnectChange;
    RegPacket.RH_ClearFeaturePortResetChange = XHCI_RH_ClearFeaturePortResetChange;
    RegPacket.RH_ClearFeaturePortSuspendChange = XHCI_RH_ClearFeaturePortSuspendChange;
    RegPacket.RH_ClearFeaturePortOvercurrentChange = XHCI_RH_ClearFeaturePortOvercurrentChange;
    RegPacket.RH_DisableIrq = XHCI_RH_DisableIrq;
    RegPacket.RH_EnableIrq = XHCI_RH_EnableIrq;

    RegPacket.MiniPortVersion = USB_MINIPORT_VERSION_XHCI;
    RegPacket.MiniPortFlags = USB_MINIPORT_FLAGS_MEMORY_IO;

    RegPacket.MiniPortResourcesSize = sizeof(XHCI_HC_RESOURCES);

    return USBPORT_RegisterUSBPortDriver(DriverObject,
                                         USB30_MINIPORT_INTERFACE_VERSION,
                                         &RegPacket);
}
