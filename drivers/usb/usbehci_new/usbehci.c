#include "usbehci.h"

//#define NDEBUG
#include <debug.h>

USBPORT_REGISTRATION_PACKET RegPacket;


MPSTATUS
NTAPI
EHCI_OpenEndpoint(IN PVOID ehciExtension,
                  IN PVOID endpointParameters,
                  IN PVOID ehciEndpoint)
{
    DPRINT("EHCI_OpenEndpoint: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_ReopenEndpoint(IN PVOID ehciExtension,
                    IN PVOID endpointParameters,
                    IN PVOID ehciEndpoint)
{
    DPRINT("EHCI_ReopenEndpoint: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
EHCI_QueryEndpointRequirements(IN PVOID ehciExtension,
                               IN PVOID endpointParameters,
                               IN PULONG EndpointRequirements)
{
    DPRINT("EHCI_QueryEndpointRequirements: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_CloseEndpoint(IN PVOID ehciExtension,
                   IN PVOID ehciEndpoint,
                   IN BOOLEAN IsDoDisablePeriodic)
{
    DPRINT("EHCI_CloseEndpoint: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
EHCI_StartController(IN PVOID ehciExtension,
                     IN PUSBPORT_RESOURCES Resources)
{
    DPRINT("EHCI_StartController: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
EHCI_StopController(IN PVOID ehciExtension,
                    IN BOOLEAN IsDoDisableInterrupts)
{
    DPRINT("EHCI_StopController: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_SuspendController(IN PVOID ehciExtension)
{
    DPRINT("EHCI_SuspendController: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
EHCI_ResumeController(IN PVOID ehciExtension)
{
    DPRINT("EHCI_ResumeController: UNIMPLEMENTED. FIXME\n");
    return 0;
}

BOOLEAN
NTAPI
EHCI_InterruptService(IN PVOID ehciExtension)
{
    DPRINT("EHCI_InterruptService: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
EHCI_InterruptDpc(IN PVOID ehciExtension,
                  IN BOOLEAN IsDoEnableInterrupts)
{
    DPRINT("EHCI_InterruptDpc: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
EHCI_SubmitTransfer(IN PVOID ehciExtension,
                    IN PVOID ehciEndpoint,
                    IN PVOID transferParameters,
                    IN PVOID ehciTransfer,
                    IN PVOID sgList)
{
    DPRINT("EHCI_SubmitTransfer: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_SubmitIsoTransfer(IN PVOID ehciExtension,
                       IN PVOID ehciEndpoint,
                       IN PVOID transferParameters,
                       IN PVOID ehciTransfer,
                       IN PVOID isoParameters)
{
    DPRINT("EHCI_SubmitIsoTransfer: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
EHCI_AbortTransfer(IN PVOID ehciExtension,
                   IN PVOID ehciEndpoint,
                   IN PVOID ehciTransfer,
                   IN PULONG CompletedLength)
{
    DPRINT("EHCI_AbortTransfer: UNIMPLEMENTED. FIXME\n");
}

ULONG
NTAPI
EHCI_GetEndpointState(IN PVOID ehciExtension,
                      IN PVOID ehciEndpoint)
{
    DPRINT("EHCI_GetEndpointState: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
EHCI_SetEndpointState(IN PVOID ehciExtension,
                      IN PVOID ehciEndpoint,
                      IN ULONG EndpointState)
{
    DPRINT("EHCI_SetEndpointState: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_PollEndpoint(IN PVOID ohciExtension,
                  IN PVOID ohciEndpoint)
{
    DPRINT("EHCI_PollEndpoint: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_CheckController(IN PVOID ehciExtension)
{
    DPRINT("EHCI_CheckController: UNIMPLEMENTED. FIXME\n");
}

ULONG
NTAPI
EHCI_Get32BitFrameNumber(IN PVOID ehciExtension)
{
    DPRINT("EHCI_Get32BitFrameNumber: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
EHCI_InterruptNextSOF(IN PVOID ehciExtension)
{
    DPRINT("EHCI_InterruptNextSOF: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_EnableInterrupts(IN PVOID ehciExtension)
{
    DPRINT("EHCI_EnableInterrupts: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_DisableInterrupts(IN PVOID ehciExtension)
{
    DPRINT("EHCI_DisableInterrupts: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_PollController(IN PVOID ehciExtension)
{
    DPRINT("EHCI_PollController: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_SetEndpointDataToggle(IN PVOID ehciExtension,
                           IN PVOID ehciEndpoint,
                           IN ULONG DataToggle)
{
    DPRINT("EHCI_SetEndpointDataToggle: UNIMPLEMENTED. FIXME\n");
}

ULONG
NTAPI
EHCI_GetEndpointStatus(IN PVOID ehciExtension,
                       IN PVOID ehciEndpoint)
{
    DPRINT("EHCI_GetEndpointStatus: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
EHCI_SetEndpointStatus(IN PVOID ehciExtension,
                       IN PVOID ehciEndpoint,
                       IN ULONG EndpointStatus)
{
    DPRINT("EHCI_SetEndpointStatus: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_ResetController(IN PVOID ehciExtension)
{
    DPRINT("EHCI_ResetController: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
EHCI_StartSendOnePacket(IN PVOID ehciExtension,
                        IN PVOID PacketParameters,
                        IN PVOID Data,
                        IN PULONG pDataLength,
                        IN PVOID BufferVA,
                        IN PVOID BufferPA,
                        IN ULONG BufferLength,
                        IN USBD_STATUS * pUSBDStatus)
{
    DPRINT("EHCI_StartSendOnePacket: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_EndSendOnePacket(IN PVOID ehciExtension,
                      IN PVOID PacketParameters,
                      IN PVOID Data,
                      IN PULONG pDataLength,
                      IN PVOID BufferVA,
                      IN PVOID BufferPA,
                      IN ULONG BufferLength,
                      IN USBD_STATUS * pUSBDStatus)
{
    DPRINT("EHCI_EndSendOnePacket: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_PassThru(IN PVOID ehciExtension,
              IN PVOID passThruParameters,
              IN ULONG ParameterLength,
              IN PVOID pParameters)
{
    DPRINT("EHCI_PassThru: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
EHCI_RebalanceEndpoint(IN PVOID ohciExtension,
                       IN PVOID endpointParameters,
                       IN PVOID ohciEndpoint)
{
    DPRINT("EHCI_RebalanceEndpoint: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_FlushInterrupts(IN PVOID ohciExtension)
{
    DPRINT("EHCI_FlushInterrupts: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
EHCI_RH_ChirpRootPort(IN PVOID ehciExtension,
                      IN USHORT Port)
{
    DPRINT("EHCI_RH_ChirpRootPort: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
EHCI_TakePortControl(IN PVOID ohciExtension)
{
    DPRINT("EHCI_TakePortControl: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_Unload()
{
    DPRINT("EHCI_Unload: UNIMPLEMENTED. FIXME\n");
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    DPRINT("DriverEntry: DriverObject - %p, RegistryPath - %wZ\n", DriverObject, RegistryPath);

    if (USBPORT_GetHciMn() != 0x10000001)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(&RegPacket, sizeof(USBPORT_REGISTRATION_PACKET));

    RegPacket.MiniPortVersion = USB_MINIPORT_VERSION_EHCI;

    RegPacket.MiniPortFlags = USB_MINIPORT_FLAGS_INTERRUPT |
                              USB_MINIPORT_FLAGS_MEMORY_IO |
                              USB_MINIPORT_FLAGS_USB2 |
                              USB_MINIPORT_FLAGS_POLLING |
                              USB_MINIPORT_FLAGS_WAKE_SUPPORT;

    RegPacket.MiniPortBusBandwidth = 400000;

    RegPacket.MiniPortExtensionSize = 0; //sizeof(EHCI_EXTENSION)
    RegPacket.MiniPortEndpointSize = 0; //sizeof(EHCI_ENDPOINT);
    RegPacket.MiniPortTransferSize = 0; //sizeof(EHCI_TRANSFER);
    RegPacket.MiniPortResourcesSize = 0; //sizeof(EHCI_HC_RESOURCES);

    RegPacket.OpenEndpoint = EHCI_OpenEndpoint;
    RegPacket.ReopenEndpoint = EHCI_ReopenEndpoint;
    RegPacket.QueryEndpointRequirements = EHCI_QueryEndpointRequirements;
    RegPacket.CloseEndpoint = EHCI_CloseEndpoint;
    RegPacket.StartController = EHCI_StartController;
    RegPacket.StopController = EHCI_StopController;
    RegPacket.SuspendController = EHCI_SuspendController;
    RegPacket.ResumeController = EHCI_ResumeController;
    RegPacket.InterruptService = EHCI_InterruptService;
    RegPacket.InterruptDpc = EHCI_InterruptDpc;
    RegPacket.SubmitTransfer = EHCI_SubmitTransfer;
    RegPacket.SubmitIsoTransfer = EHCI_SubmitIsoTransfer;
    RegPacket.AbortTransfer = EHCI_AbortTransfer;
    RegPacket.GetEndpointState = EHCI_GetEndpointState;
    RegPacket.SetEndpointState = EHCI_SetEndpointState;
    RegPacket.PollEndpoint = EHCI_PollEndpoint;
    RegPacket.CheckController = EHCI_CheckController;
    RegPacket.Get32BitFrameNumber = EHCI_Get32BitFrameNumber;
    RegPacket.InterruptNextSOF = EHCI_InterruptNextSOF;
    RegPacket.EnableInterrupts = EHCI_EnableInterrupts;
    RegPacket.DisableInterrupts = EHCI_DisableInterrupts;
    RegPacket.PollController = EHCI_PollController;
    RegPacket.SetEndpointDataToggle = EHCI_SetEndpointDataToggle;
    RegPacket.GetEndpointStatus = EHCI_GetEndpointStatus;
    RegPacket.SetEndpointStatus = EHCI_SetEndpointStatus;
    RegPacket.RH_GetRootHubData = EHCI_RH_GetRootHubData;
    RegPacket.RH_GetStatus = EHCI_RH_GetStatus;
    RegPacket.RH_GetPortStatus = EHCI_RH_GetPortStatus;
    RegPacket.RH_GetHubStatus = EHCI_RH_GetHubStatus;
    RegPacket.RH_SetFeaturePortReset = EHCI_RH_SetFeaturePortReset;
    RegPacket.RH_SetFeaturePortPower = EHCI_RH_SetFeaturePortPower;
    RegPacket.RH_SetFeaturePortEnable = EHCI_RH_SetFeaturePortEnable;
    RegPacket.RH_SetFeaturePortSuspend = EHCI_RH_SetFeaturePortSuspend;
    RegPacket.RH_ClearFeaturePortEnable = EHCI_RH_ClearFeaturePortEnable;
    RegPacket.RH_ClearFeaturePortPower = EHCI_RH_ClearFeaturePortPower;
    RegPacket.RH_ClearFeaturePortSuspend = EHCI_RH_ClearFeaturePortSuspend;
    RegPacket.RH_ClearFeaturePortEnableChange = EHCI_RH_ClearFeaturePortEnableChange;
    RegPacket.RH_ClearFeaturePortConnectChange = EHCI_RH_ClearFeaturePortConnectChange;
    RegPacket.RH_ClearFeaturePortResetChange = EHCI_RH_ClearFeaturePortResetChange;
    RegPacket.RH_ClearFeaturePortSuspendChange = EHCI_RH_ClearFeaturePortSuspendChange;
    RegPacket.RH_ClearFeaturePortOvercurrentChange = EHCI_RH_ClearFeaturePortOvercurrentChange;
    RegPacket.RH_DisableIrq = EHCI_RH_DisableIrq;
    RegPacket.RH_EnableIrq = EHCI_RH_EnableIrq;
    RegPacket.StartSendOnePacket = EHCI_StartSendOnePacket;
    RegPacket.EndSendOnePacket = EHCI_EndSendOnePacket;
    RegPacket.PassThru = EHCI_PassThru;
    RegPacket.RebalanceEndpoint = EHCI_RebalanceEndpoint;
    RegPacket.FlushInterrupts = EHCI_FlushInterrupts;
    RegPacket.RH_ChirpRootPort = EHCI_RH_ChirpRootPort;
    RegPacket.TakePortControl = EHCI_TakePortControl;

    DriverObject->DriverUnload = (PDRIVER_UNLOAD)EHCI_Unload;

    return USBPORT_RegisterUSBPortDriver(DriverObject, 200, &RegPacket);
}
