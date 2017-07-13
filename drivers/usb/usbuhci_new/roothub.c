#include "usbuhci.h"

//#define NDEBUG
#include <debug.h>

VOID
NTAPI
UhciRHGetRootHubData(IN PVOID uhciExtension,
                     IN PVOID rootHubData)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    PUSBPORT_ROOT_HUB_DATA RootHubData = rootHubData;
    USBPORT_HUB_11_CHARACTERISTICS HubCharacteristics;

    DPRINT("UhciRHGetRootHubData: ...\n");

    HubCharacteristics.AsUSHORT = 0;
    HubCharacteristics.PowerControlMode = TRUE;
    HubCharacteristics.NoPowerSwitching = TRUE;
    HubCharacteristics.OverCurrentProtectionMode = TRUE;

    if (UhciExtension->HcFlavor != UHCI_Piix4)
    {
        HubCharacteristics.NoOverCurrentProtection = TRUE;
    }

    RootHubData->NumberOfPorts = UHCI_NUM_ROOT_HUB_PORTS;
    RootHubData->HubCharacteristics.Usb11HubCharacteristics = HubCharacteristics;
    RootHubData->PowerOnToPowerGood = 1;
    RootHubData->HubControlCurrent = 0;
}

MPSTATUS
NTAPI
UhciRHGetStatus(IN PVOID uhciExtension,
                IN PUSHORT Status)
{
    DPRINT("UhciRHGetStatus: ...\n");
    *Status = 1;
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciRHGetPortStatus(IN PVOID uhciExtension,
                    IN USHORT Port,
                    IN PUSB_PORT_STATUS_AND_CHANGE PortStatus)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    PUHCI_HW_REGISTERS BaseRegister;
    PUSHORT PortControlRegister;
    UHCI_PORT_STATUS_CONTROL PortControl;
    ULONG port;
    USB_20_PORT_STATUS portStatus;
    USB_20_PORT_CHANGE portChange;

    //DPRINT("UhciRHGetPortStatus: ...\n");

    BaseRegister = UhciExtension->BaseRegister;
    PortControlRegister = &BaseRegister->PortControl[Port-1].AsUSHORT;
    PortControl.AsUSHORT = READ_PORT_USHORT(PortControlRegister);

    portStatus.AsUshort16 = 0;
    portChange.AsUshort16 = 0;

    portStatus.CurrentConnectStatus = PortControl.CurrentConnectStatus;
    portStatus.PortEnabledDisabled = PortControl.PortEnabledDisabled;

    if (PortControl.Suspend == TRUE &&
        PortControl.PortEnabledDisabled == TRUE)
    {
        portStatus.Suspend = TRUE;
    }
    else
    {
        portStatus.Suspend = FALSE;
    }

    // FIXME HcFlavor in usbport
    //if (UhciExtension->HcFlavor == UHCI_Piix4)
    if (TRUE)
    {
        portStatus.OverCurrent = PortControl.Reserved2 & 1;
        portStatus.PortPower = (~PortControl.Reserved2 & 1);
        portChange.OverCurrentIndicatorChange = (PortControl.Reserved2 & 2) != 0;
    }
    else
    {
        portStatus.OverCurrent = FALSE;
        portStatus.PortPower = TRUE;
        portChange.OverCurrentIndicatorChange = FALSE;
    }

    portStatus.HighSpeedDeviceAttached = FALSE;

    portStatus.Reset = PortControl.PortReset;
    portStatus.LowSpeedDeviceAttached = PortControl.LowSpeedDevice;
    portChange.ConnectStatusChange = PortControl.ConnectStatusChange;

    port = 1 << (Port - 1);

    if (UhciExtension->ResetPortMask & port)
    {
        portChange.ConnectStatusChange = FALSE;
        portChange.PortEnableDisableChange = FALSE;
    }
    else
    {
        portChange.PortEnableDisableChange = PortControl.PortEnableDisableChange;
    }

    if (UhciExtension->SuspendChangePortMask & port)
    {
        portChange.SuspendChange = TRUE;
    }

    if (UhciExtension->ResetChangePortMask & port)
    {
        portChange.ResetChange = TRUE;
    }

    PortStatus->PortStatus.Usb20PortStatus = portStatus;
    PortStatus->PortChange.Usb20PortChange = portChange;

    DPRINT("UhciRHGetPortStatus: PortControl.AsUSHORT[%x] - %X, PortStatus - %X\n",
           Port,
           PortControl.AsUSHORT,
           PortStatus->AsUlong32);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciRHGetHubStatus(IN PVOID uhciExtension,
                   IN PUSB_HUB_STATUS_AND_CHANGE HubStatus)
{
    DPRINT("UhciRHGetHubStatus: ...\n");
    HubStatus->AsUlong32 = 0;
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
UhciRHSetFeaturePortResetWorker(IN PUHCI_EXTENSION UhciExtension,
                                IN PUSHORT pPort)
{
    DPRINT("UhciRHSetFeaturePortResetWorker: UNIMPLEMENTED. FIXME\n");
    DbgBreakPoint();
}

MPSTATUS
NTAPI
UhciRHSetFeaturePortReset(IN PVOID uhciExtension,
                          IN USHORT Port)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    ULONG ResetPortMask;
    ULONG port;

    DPRINT("UhciRHSetFeaturePortReset: ...\n");

    ResetPortMask = UhciExtension->ResetPortMask;
    port = 1 << (Port - 1);

    if (ResetPortMask & port)
    {
        return MP_STATUS_FAILURE;
    }

    UhciExtension->ResetPortMask = ResetPortMask | port;

    if (UhciExtension->HcFlavor == UHCI_VIA &&
        UhciExtension->HcFlavor == UHCI_VIA_x01 &&
        UhciExtension->HcFlavor == UHCI_VIA_x02 &&
        UhciExtension->HcFlavor == UHCI_VIA_x03 &&
        UhciExtension->HcFlavor == UHCI_VIA_x04)
    {
        DPRINT1("UhciRHSetFeaturePortReset: Via chip. FIXME\n");
        return MP_STATUS_SUCCESS;
    }

    UhciRHSetFeaturePortResetWorker(UhciExtension, &Port);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciRHSetFeaturePortPower(IN PVOID uhciExtension,
                          IN USHORT Port)
{
    DPRINT("UhciRHSetFeaturePortPower: ...\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciRHSetFeaturePortEnable(IN PVOID uhciExtension,
                           IN USHORT Port)
{
    DPRINT("UhciRHSetFeaturePortEnable: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciRHSetFeaturePortSuspend(IN PVOID uhciExtension,
                            IN USHORT Port)
{
    DPRINT("UhciRHSetFeaturePortSuspend: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciRHClearFeaturePortEnable(IN PVOID uhciExtension,
                             IN USHORT Port)
{
    DPRINT("UhciRHClearFeaturePortEnable: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciRHClearFeaturePortPower(IN PVOID uhciExtension,
                            IN USHORT Port)
{
    DPRINT("UhciRHClearFeaturePortPower: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciRHClearFeaturePortSuspend(IN PVOID uhciExtension,
                              IN USHORT Port)
{
    DPRINT("UhciRHClearFeaturePortSuspend: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciRHClearFeaturePortEnableChange(IN PVOID uhciExtension,
                                   IN USHORT Port)
{
    DPRINT("UhciRHClearFeaturePortEnableChange: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciRHClearFeaturePortConnectChange(IN PVOID uhciExtension,
                                    IN USHORT Port)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    PUHCI_HW_REGISTERS BaseRegister;
    PUSHORT PortControlRegister;
    UHCI_PORT_STATUS_CONTROL PortControl;

    DPRINT("UhciRHClearFeaturePortConnectChange: Port - %04X\n", Port);

    BaseRegister = UhciExtension->BaseRegister;
    PortControlRegister = (PUSHORT)&BaseRegister->PortControl[Port - 1];
    PortControl.AsUSHORT = READ_PORT_USHORT(PortControlRegister);

    if (PortControl.ConnectStatusChange == TRUE)
    {
        /* WC (Write Clear) bits */
        PortControl.PortEnableDisableChange = FALSE;
        PortControl.ConnectStatusChange = TRUE;
        WRITE_PORT_USHORT(PortControlRegister, PortControl.AsUSHORT);
    }

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciRHClearFeaturePortResetChange(IN PVOID uhciExtension,
                                  IN USHORT Port)
{
    DPRINT("UhciRHClearFeaturePortResetChange: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciRHClearFeaturePortSuspendChange(IN PVOID uhciExtension,
                                    IN USHORT Port)
{
    DPRINT("UhciRHClearFeaturePortSuspendChange: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciRHClearFeaturePortOvercurrentChange(IN PVOID uhciExtension,
                                        IN USHORT Port)
{
    DPRINT("UhciRHClearFeaturePortOvercurrentChange: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
UhciRHDisableIrq(IN PVOID uhciExtension)
{
    DPRINT("UhciRHDisableIrq: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
UhciRHEnableIrq(IN PVOID uhciExtension)
{
    DPRINT("UhciRHEnableIrq: UNIMPLEMENTED. FIXME\n");
}

