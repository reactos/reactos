#include "usbehci.h"

//#define NDEBUG
#include <debug.h>

#define NDEBUG_EHCI_ROOT_HUB
#include "dbg_ehci.h"

MPSTATUS
NTAPI
EHCI_RH_ChirpRootPort(IN PVOID ehciExtension,
                      IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;
    ULONG PortBit;
    ULONG ix;

    DPRINT_RH("EHCI_RH_ChirpRootPort: Port - %x\n", Port);
    ASSERT(Port != 0);

    PortStatusReg = ((PULONG)EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);
    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);
    DPRINT_RH("EHCI_RH_ChirpRootPort: PortSC - %p\n", PortSC.AsULONG);

    PortBit = 1 << (Port - 1);

    if (PortBit & EhciExtension->ResetPortBits)
    {
        DPRINT_RH("EHCI_RH_ChirpRootPort: Skip port - %x\n", Port);
        return MP_STATUS_SUCCESS;
    }

    if (PortSC.PortPower == 0)
    {
        DPRINT_RH("EHCI_RH_ChirpRootPort: Skip port - %x\n", Port);
        return MP_STATUS_SUCCESS;
    }

    if (PortSC.CurrentConnectStatus == 0 ||
        PortSC.PortEnabledDisabled == 1 ||
        PortSC.PortOwner == 1)
    {
        DPRINT_RH("EHCI_RH_ChirpRootPort: No port - %x\n", Port);
        return MP_STATUS_SUCCESS;
    }

    if (PortSC.LineStatus == 1 &&
        PortSC.Suspend == 0 &&
        PortSC.CurrentConnectStatus == 1)
    {
        /* Attached device is not a high-speed device.
           Release ownership of the port to a selected HC.
           Companion HC owns and controls the port. Section 4.2 */
        PortSC.PortOwner = 1;
        WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);

        DPRINT_RH("EHCI_RH_ChirpRootPort: Companion HC port - %x\n", Port);
        return MP_STATUS_SUCCESS;
    }

    DPRINT("EHCI_RH_ChirpRootPort: EhciExtension - %p, Port - %x\n",
           EhciExtension,
           Port);

    PortSC.PortEnabledDisabled = 0;
    PortSC.PortReset = 1;
    WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);

    RegPacket.UsbPortWait(EhciExtension, 10);

    do
    {
        PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

        PortSC.ConnectStatusChange = 0;
        PortSC.PortEnableDisableChange = 0;
        PortSC.OverCurrentChange = 0;
        PortSC.PortReset = 0;

        WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);

        for (ix = 0; ix <= 500; ix += 20)
        {
             KeStallExecutionProcessor(20);
             PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

             DPRINT_RH("EHCI_RH_ChirpRootPort: Reset port - %x\n", Port);

             if (PortSC.PortReset == 0)
                 break;
        }
    }
    while (PortSC.PortReset == 1);

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

    if (PortSC.PortEnabledDisabled == 1)
    {
        PortSC.ConnectStatusChange = 0;
        PortSC.PortEnabledDisabled = 0;
        PortSC.PortEnableDisableChange = 0;
        PortSC.OverCurrentChange = 0;

        RegPacket.UsbPortWait(EhciExtension, 10);

        EhciExtension->ResetPortBits |= PortBit;

        WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);
        DPRINT_RH("EHCI_RH_ChirpRootPort: Disable port - %x\n", Port);
    }
    else
    {
        PortSC.PortOwner = 1;
        WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);
        DPRINT_RH("EHCI_RH_ChirpRootPort: Companion HC port - %x\n", Port);
    }

    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
EHCI_RH_GetRootHubData(IN PVOID ehciExtension,
                       IN PVOID rootHubData)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PUSBPORT_ROOT_HUB_DATA RootHubData;
    USBPORT_HUB_20_CHARACTERISTICS HubCharacteristics;

    DPRINT_RH("EHCI_RH_GetRootHubData: EhciExtension - %p, rootHubData - %p\n",
              EhciExtension,
              rootHubData);

    RootHubData = rootHubData;

    RootHubData->NumberOfPorts = EhciExtension->NumberOfPorts;

    HubCharacteristics.AsUSHORT = 0;

    /* Logical Power Switching Mode */
    if (EhciExtension->PortPowerControl == 1)
    {
        /* Individual port power switching */
        HubCharacteristics.PowerControlMode = 1;
    }
    else
    {
        /* Ganged power switching (all ports’ power at once) */
        HubCharacteristics.PowerControlMode = 0;
    }

    HubCharacteristics.NoPowerSwitching = 0;

    /* EHCI RH is not part of a compound device */
    HubCharacteristics.PartOfCompoundDevice = 0; 

    /* Global Over-current Protection */
    HubCharacteristics.OverCurrentProtectionMode = 0;

    RootHubData->HubCharacteristics.Usb20HubCharacteristics = HubCharacteristics;

    RootHubData->PowerOnToPowerGood = 2; // Time (in 2 ms intervals)
    RootHubData->HubControlCurrent = 0;
}

MPSTATUS
NTAPI
EHCI_RH_GetStatus(IN PVOID ehciExtension,
                  IN PUSHORT Status)
{
    DPRINT_RH("EHCI_RH_GetStatus: ... \n");
    *Status = 1;
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
EHCI_RH_GetPortStatus(IN PVOID ehciExtension,
                      IN USHORT Port,
                      IN PUSB_PORT_STATUS_AND_CHANGE PortStatus)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;
    USB_PORT_STATUS_AND_CHANGE status;
    ULONG PortMaskBits;

    PortStatusReg = ((PULONG)EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);
    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

    if (PortSC.CurrentConnectStatus)
    {
        DPRINT_RH("EHCI_RH_GetPortStatus: Port - %x, PortSC.AsULONG - %p\n",
                  Port,
                  PortSC.AsULONG);
    }

    PortStatus->AsUlong32 = 0;

    if (PortSC.LineStatus == 1 && // K-state  Low-speed device
        PortSC.PortOwner != 1 && // Companion HC not owns and not controls this port
        (PortSC.PortEnabledDisabled | PortSC.Suspend) && // Enable or Suspend
        PortSC.CurrentConnectStatus == 1) // Device is present
    {
        DPRINT("EHCI_RH_GetPortStatus: LowSpeed device detected\n");
        PortSC.PortOwner = 1; // release ownership
        WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);
        return MP_STATUS_SUCCESS;
    }

    status.AsUlong32 = 0;

    status.PortStatus.Usb20PortStatus.CurrentConnectStatus = PortSC.CurrentConnectStatus;
    status.PortStatus.Usb20PortStatus.PortEnabledDisabled = PortSC.PortEnabledDisabled;
    status.PortStatus.Usb20PortStatus.Suspend = PortSC.Suspend;
    status.PortStatus.Usb20PortStatus.OverCurrent = PortSC.OverCurrentActive;
    status.PortStatus.Usb20PortStatus.Reset = PortSC.PortReset;
    status.PortStatus.Usb20PortStatus.PortPower = PortSC.PortPower;
    status.PortStatus.Usb20PortStatus.Reserved1 = (PortSC.PortOwner == 1) ? 4 : 0;

    status.PortChange.Usb20PortChange.PortEnableDisableChange = PortSC.PortEnableDisableChange;
    status.PortChange.Usb20PortChange.OverCurrentIndicatorChange = PortSC.OverCurrentChange;

    PortMaskBits = 1 << (Port - 1);

    if (status.PortStatus.Usb20PortStatus.CurrentConnectStatus)
    {
        status.PortStatus.Usb20PortStatus.LowSpeedDeviceAttached = 0;
    }

    status.PortStatus.Usb20PortStatus.HighSpeedDeviceAttached = 1;

    if (PortSC.ConnectStatusChange)
    {
        EhciExtension->ConnectPortBits |= PortMaskBits;
    }

    if (EhciExtension->FinishResetPortBits & PortMaskBits)
    {
        status.PortChange.Usb20PortChange.ResetChange = 1;
    }

    if (EhciExtension->ConnectPortBits & PortMaskBits)
    {
        status.PortChange.Usb20PortChange.ConnectStatusChange = 1;
    }

    if (EhciExtension->SuspendPortBits & PortMaskBits)
    {
        status.PortChange.Usb20PortChange.SuspendChange = 1;
    }

    *PortStatus = status;

    if (status.PortStatus.Usb20PortStatus.CurrentConnectStatus)
    {
        DPRINT_RH("EHCI_RH_GetPortStatus: Port - %x, status.AsULONG - %p\n",
                  Port,
                  status.AsUlong32);
    }

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
EHCI_RH_GetHubStatus(IN PVOID ehciExtension,
                     IN PUSB_HUB_STATUS_AND_CHANGE HubStatus)
{
    DPRINT_RH("EHCI_RH_GetHubStatus: ... \n");
    HubStatus->AsUlong32 = 0;
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
EHCI_RH_FinishReset(IN PVOID ehciExtension,
                    IN PVOID Context)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;
    PUSHORT Port = Context;

    DPRINT("EHCI_RH_FinishReset: *Port - %x\n", *Port);

    PortStatusReg = ((PULONG)EhciExtension->OperationalRegs + EHCI_PORTSC) + (*Port - 1);
    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

    if (PortSC.AsULONG != -1)
    {
        if (!PortSC.CurrentConnectStatus)
        {
            DPRINT("EHCI_RH_FinishReset: PortSC.AsULONG - %p\n", PortSC.AsULONG);
        }
    
        if (PortSC.PortEnabledDisabled ||
            !PortSC.CurrentConnectStatus ||
            PortSC.ConnectStatusChange)
        {
            EhciExtension->FinishResetPortBits |= (1 << (*Port - 1));
            RegPacket.UsbPortInvalidateRootHub(EhciExtension);
        }
        else
        {
            PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);
            PortSC.PortOwner = 1;
            WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);
            EhciExtension->FinishResetPortBits |= (1 << (*Port - 1));
        }
    
        EhciExtension->ResetPortBits &= ~(1 << (*Port - 1));
    }
}

VOID
NTAPI
EHCI_RH_PortResetComplete(IN PVOID ehciExtension,
                          IN PVOID Context)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;
    ULONG ix;
    PUSHORT Port = Context;

    DPRINT("EHCI_RH_PortResetComplete: *Port - %x\n", *Port);

    PortStatusReg = ((PULONG)EhciExtension->OperationalRegs + EHCI_PORTSC) + (*Port - 1);

START:

    ix = 0;

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

    PortSC.ConnectStatusChange = 0;
    PortSC.PortEnableDisableChange = 0;
    PortSC.OverCurrentChange = 0;
    PortSC.PortReset = 0;

    WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);

    do
    {
        KeStallExecutionProcessor(20);

        ix += 20;

        PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

        if (ix > 500)
        {
            goto START;
        }
    }
    while (PortSC.PortReset && (PortSC.AsULONG != -1));

    RegPacket.UsbPortRequestAsyncCallback(EhciExtension,
                                          50, // TimerValue
                                          Port,
                                          sizeof(Port),
                                          EHCI_RH_FinishReset);
}

MPSTATUS
NTAPI
EHCI_RH_SetFeaturePortReset(IN PVOID ehciExtension,
                            IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT("EHCI_RH_SetFeaturePortReset: Port - %x\n", Port);

    PortStatusReg = ((PULONG)EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);

    EhciExtension->ResetPortBits |= 1 << (Port - 1);

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

    PortSC.ConnectStatusChange = 0;
    PortSC.PortEnabledDisabled = 0;
    PortSC.PortEnableDisableChange = 0;
    PortSC.OverCurrentChange = 0;
    PortSC.PortReset = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);

    RegPacket.UsbPortRequestAsyncCallback(EhciExtension,
                                          50, // TimerValue
                                          &Port,
                                          sizeof(Port),
                                          EHCI_RH_PortResetComplete);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
EHCI_RH_SetFeaturePortPower(IN PVOID ehciExtension,
                            IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT_RH("EHCI_RH_SetFeaturePortPower: Port - %x\n", Port);

    PortStatusReg = ((PULONG)EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

    PortSC.ConnectStatusChange = 0;
    PortSC.PortEnableDisableChange = 0;
    PortSC.OverCurrentChange = 0;
    PortSC.PortPower = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
EHCI_RH_SetFeaturePortEnable(IN PVOID ehciExtension,
                             IN USHORT Port)
{
    DPRINT_RH("EHCI_RH_SetFeaturePortEnable: Not supported\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
EHCI_RH_SetFeaturePortSuspend(IN PVOID ehciExtension,
                              IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT("EHCI_RH_SetFeaturePortSuspend: Port - %x\n", Port);

    PortStatusReg = ((PULONG)EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

    PortSC.ConnectStatusChange = 0;
    PortSC.PortEnableDisableChange = 0;
    PortSC.OverCurrentChange = 0;
    PortSC.Suspend = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);
    KeStallExecutionProcessor(125);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortEnable(IN PVOID ehciExtension,
                               IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT("EHCI_RH_ClearFeaturePortEnable: Port - %x\n", Port);

    PortStatusReg = ((PULONG)EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

    PortSC.ConnectStatusChange = 0;
    PortSC.PortEnabledDisabled = 0;
    PortSC.PortEnableDisableChange = 0;
    PortSC.OverCurrentChange = 0;

    WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortPower(IN PVOID ehciExtension,
                              IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT("EHCI_RH_ClearFeaturePortPower: Port - %x\n", Port);

    PortStatusReg = ((PULONG)EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);
    PortSC.PortPower = 0;
    WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);

    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
EHCI_RH_PortResumeComplete(IN PVOID ehciExtension,
                           IN PVOID Context)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;
    PUSHORT Port = Context;

    DPRINT("EHCI_RH_PortResumeComplete: *Port - %x\n", *Port);

    PortStatusReg = ((PULONG)EhciExtension->OperationalRegs + EHCI_PORTSC) + (*Port - 1);

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

    PortSC.ConnectStatusChange = 0;
    PortSC.PortEnableDisableChange = 0;
    PortSC.OverCurrentChange = 0;
    PortSC.ForcePortResume = 0;
    PortSC.Suspend = 0;

    WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);
    READ_REGISTER_ULONG(PortStatusReg);

    EhciExtension->SuspendPortBits |= 1 << (*Port - 1);
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortSuspend(IN PVOID ehciExtension,
                                IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT("EHCI_RH_ClearFeaturePortSuspend: Port - %x\n", Port);

    PortStatusReg = ((PULONG)EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);
    EhciExtension->ResetPortBits |= 1 << (Port - 1);

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);
    PortSC.ForcePortResume = 1;
    WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);

    RegPacket.UsbPortRequestAsyncCallback(EhciExtension,
                                          50, // TimerValue
                                          &Port,
                                          sizeof(Port),
                                          EHCI_RH_PortResumeComplete);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortEnableChange(IN PVOID ehciExtension,
                                     IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT("EHCI_RH_ClearFeaturePortEnableChange: Port - %p\n", Port);

    PortStatusReg = ((PULONG)EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

    PortSC.ConnectStatusChange = 0;
    PortSC.OverCurrentChange = 0;
    PortSC.PortEnableDisableChange = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortConnectChange(IN PVOID ehciExtension,
                                      IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT_RH("EHCI_RH_ClearFeaturePortConnectChange: Port - %x\n", Port);

    PortStatusReg = ((PULONG)EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

    if (PortSC.ConnectStatusChange)
    {
        PortSC.ConnectStatusChange = 1;
        PortSC.PortEnableDisableChange = 0;
        PortSC.OverCurrentChange = 0;

        WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);
    }

    EhciExtension->ConnectPortBits &= ~(1 << (Port - 1));

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortResetChange(IN PVOID ehciExtension,
                                    IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;

    DPRINT("EHCI_RH_ClearFeaturePortConnectChange: Port - %x\n", Port);

    EhciExtension->FinishResetPortBits &= ~(1 << (Port - 1));
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortSuspendChange(IN PVOID ehciExtension,
                                      IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;

    DPRINT("EHCI_RH_ClearFeaturePortSuspendChange: Port - %x\n", Port);

    EhciExtension->SuspendPortBits &= ~(1 << (Port - 1));
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortOvercurrentChange(IN PVOID ehciExtension,
                                          IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT_RH("EHCI_RH_ClearFeaturePortOvercurrentChange: Port - %x\n", Port);

    PortStatusReg = ((PULONG)EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

    PortSC.ConnectStatusChange = 0;
    PortSC.PortEnableDisableChange = 0;
    PortSC.OverCurrentChange = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);

    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
EHCI_RH_DisableIrq(IN PVOID ehciExtension)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PULONG IntrStsReg;
    EHCI_INTERRUPT_ENABLE IntrSts;

    DPRINT_RH("EHCI_RH_DisableIrq: ... \n");

    IntrStsReg = (PULONG)EhciExtension->OperationalRegs + EHCI_USBINTR;
    IntrSts.AsULONG = READ_REGISTER_ULONG(IntrStsReg);

    EhciExtension->InterruptMask.PortChangeInterrupt = 0;
    IntrSts.PortChangeInterrupt = 0;

    if (IntrSts.Interrupt)
    {
        WRITE_REGISTER_ULONG(IntrStsReg, IntrSts.AsULONG);
    }
}

VOID
NTAPI
EHCI_RH_EnableIrq(IN PVOID ehciExtension)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PULONG IntrStsReg;
    EHCI_INTERRUPT_ENABLE IntrSts;

    DPRINT_RH("EHCI_RH_EnableIrq: ... \n");

    IntrStsReg = (PULONG)EhciExtension->OperationalRegs + EHCI_USBINTR;
    IntrSts.AsULONG = READ_REGISTER_ULONG(IntrStsReg);

    EhciExtension->InterruptMask.PortChangeInterrupt = 1;
    IntrSts.PortChangeInterrupt = 1;

    if (IntrSts.Interrupt)
    {
        WRITE_REGISTER_ULONG(IntrStsReg, IntrSts.AsULONG);
    }
}
