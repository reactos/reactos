#include "usbohci.h"

#define NDEBUG
#include <debug.h>

OHCI_REG_RH_DESCRIPTORA
NTAPI 
OHCI_ReadRhDescriptorA(IN POHCI_EXTENSION OhciExtension)
{
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    OHCI_REG_RH_DESCRIPTORA DescriptorA;
    ULONG ix;

    OperationalRegs = OhciExtension->OperationalRegs;

    DPRINT("OHCI_ReadRhDescriptorA: OhciExtension - %p\n", OhciExtension);

    for (ix = 0; ix < 10; ix++)
    {
        DescriptorA.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcRhDescriptorA.AsULONG);

        if (DescriptorA.AsULONG != 0 &&
            DescriptorA.Reserved == 0 &&
            DescriptorA.NumberDownstreamPorts <= OHCI_MAX_PORT_COUNT) // Reserved bits
        {
            break;
        }

        DPRINT1("OHCI_ReadRhDescriptorA: DescriptorA - %lX, ix - %d\n",
                DescriptorA.AsULONG, ix);

        KeStallExecutionProcessor(5);
    }

    return DescriptorA;
}

VOID
NTAPI
OHCI_RH_GetRootHubData(IN PVOID ohciExtension,
                       IN PVOID rootHubData)
{
    POHCI_EXTENSION OhciExtension;
    PUSBPORT_ROOT_HUB_DATA RootHubData;
    OHCI_REG_RH_DESCRIPTORA DescriptorA;
    UCHAR PowerOnToPowerGoodTime;
    USBPORT_HUB_11_CHARACTERISTICS HubCharacteristics;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_GetRootHubData: OhciExtension - %p, rootHubData - %p\n",
           OhciExtension,
           rootHubData);

    RootHubData = rootHubData;
    DescriptorA = OHCI_ReadRhDescriptorA(OhciExtension);

    RootHubData->NumberOfPorts = DescriptorA.NumberDownstreamPorts;

    /* Waiting time (in 2 ms intervals) */
    PowerOnToPowerGoodTime = DescriptorA.PowerOnToPowerGoodTime;
    if (PowerOnToPowerGoodTime <= OHCI_MINIMAL_POTPGT)
    {
        PowerOnToPowerGoodTime = OHCI_MINIMAL_POTPGT;
    }
    RootHubData->PowerOnToPowerGood = PowerOnToPowerGoodTime;

    HubCharacteristics.AsUSHORT = 0;

    if (DescriptorA.PowerSwitchingMode)
    {
        /* Individual port power switching */
        HubCharacteristics.PowerControlMode = 1;
    }
    else
    {
        /* Ganged power switching */
        HubCharacteristics.PowerControlMode = 0;
    }

    HubCharacteristics.NoPowerSwitching = 0;

    /* always 0 (OHCI RH is not a compound device) */
    ASSERT(DescriptorA.DeviceType == 0);
    HubCharacteristics.PartOfCompoundDevice = DescriptorA.DeviceType;

    HubCharacteristics.OverCurrentProtectionMode = DescriptorA.OverCurrentProtectionMode;
    HubCharacteristics.NoOverCurrentProtection = DescriptorA.NoOverCurrentProtection;

    RootHubData->HubCharacteristics.Usb11HubCharacteristics = HubCharacteristics;
    RootHubData->HubControlCurrent = 0;
}

MPSTATUS
NTAPI
OHCI_RH_GetStatus(IN PVOID ohciExtension,
                  IN PUSHORT Status)
{
    DPRINT("OHCI_RH_GetStatus: \n");
    *Status = 1;
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_GetPortStatus(IN PVOID ohciExtension,
                      IN USHORT Port,
                      IN PUSB_PORT_STATUS_AND_CHANGE PortStatus)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG PortStatusReg;
    OHCI_REG_RH_PORT_STATUS portStatus;
    ULONG ix;
    ULONG Reserved;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_GetPortStatus: OhciExtension - %p, Port - %x, PortStatus - %lX\n",
           OhciExtension,
           Port,
           PortStatus->AsUlong32);

    OperationalRegs = OhciExtension->OperationalRegs;
    PortStatusReg = (PULONG)&OperationalRegs->HcRhPortStatus[Port-1];

    for (ix = 0; ix < 10; ix++)
    {
        portStatus.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

        Reserved = portStatus.Reserved1r |
                   portStatus.Reserved2r |
                   portStatus.Reserved3;

        if (portStatus.AsULONG && !Reserved)
        {
            break;
        }

        DPRINT("OHCI_RH_GetPortStatus: portStatus - %X\n", portStatus.AsULONG);

        KeStallExecutionProcessor(5);
    }

    PortStatus->AsUlong32 = portStatus.AsULONG;

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_GetHubStatus(IN PVOID ohciExtension,
                     IN PUSB_HUB_STATUS_AND_CHANGE HubStatus)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    OHCI_REG_RH_STATUS HcRhStatus;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_GetHubStatus: ohciExtension - %p, HubStatus - %lX\n",
           ohciExtension,
           HubStatus->AsUlong32);

    OperationalRegs = OhciExtension->OperationalRegs;
    HcRhStatus.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcRhStatus.AsULONG);

    HubStatus->HubStatus.LocalPowerLost = HcRhStatus.LocalPowerStatus;
    HubStatus->HubChange.LocalPowerChange = HcRhStatus.LocalPowerStatusChange;

    HubStatus->HubStatus.OverCurrent = HcRhStatus.OverCurrentIndicator;
    HubStatus->HubChange.OverCurrentChange = HcRhStatus.OverCurrentIndicatorChangeR;

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_SetFeaturePortReset(IN PVOID ohciExtension,
                            IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG PortStatusReg;
    OHCI_REG_RH_PORT_STATUS PortStatus;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_SetFeaturePortReset: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;
    PortStatusReg = (PULONG)&OperationalRegs->HcRhPortStatus[Port-1];

    PortStatus.AsULONG = 0;
    PortStatus.SetPortReset = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortStatus.AsULONG);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_SetFeaturePortPower(IN PVOID ohciExtension,
                            IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_SetFeaturePortPower: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         0x100);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_SetFeaturePortEnable(IN PVOID ohciExtension,
                             IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_SetFeaturePortEnable: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         2);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_SetFeaturePortSuspend(IN PVOID ohciExtension,
                              IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_SetFeaturePortSuspend: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         4);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortEnable(IN PVOID ohciExtension,
                               IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortEnable: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         1);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortPower(IN PVOID ohciExtension,
                              IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortPower: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         0x200);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortSuspend(IN PVOID ohciExtension,
                                IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortSuspend: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         8);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortEnableChange(IN PVOID ohciExtension,
                                     IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortEnableChange: ohciExtension - %p, Port - %x\n",
           ohciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         0x20000);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortConnectChange(IN PVOID ohciExtension,
                                      IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortConnectChange: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         0x10000);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortResetChange(IN PVOID ohciExtension,
                                    IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortResetChange: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         0x100000);
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortSuspendChange(IN PVOID ohciExtension,
                                      IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortSuspendChange: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         0x40000);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortOvercurrentChange(IN PVOID ohciExtension,
                                          IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortOvercurrentChange: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    if (Port)
    {
        WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                             0x80000);
    }
    else
    {
        WRITE_REGISTER_ULONG(&OperationalRegs->HcRhStatus.AsULONG,
                             0x20000);
    }

    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
OHCI_RH_DisableIrq(IN PVOID ohciExtension)
{
    POHCI_EXTENSION  OhciExtension = ohciExtension;
    DPRINT("OHCI_RH_DisableIrq: OhciExtension - %p\n", OhciExtension);

    WRITE_REGISTER_ULONG(&OhciExtension->OperationalRegs->HcInterruptDisable.AsULONG,
                         0x40);
}

VOID
NTAPI
OHCI_RH_EnableIrq(IN PVOID ohciExtension)
{
    POHCI_EXTENSION OhciExtension = ohciExtension;
    DPRINT("OHCI_RH_EnableIrq: OhciExtension - %p\n", OhciExtension);

    WRITE_REGISTER_ULONG(&OhciExtension->OperationalRegs->HcInterruptEnable.AsULONG,
                         0x40);
}
