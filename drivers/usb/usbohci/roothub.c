/*
 * PROJECT:     ReactOS USB OHCI Miniport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBOHCI root hub functions
 * COPYRIGHT:   Copyright 2017-2018 Vadim Galyant <vgal@rambler.ru>
 */

#include "usbohci.h"

#define NDEBUG
#include <debug.h>

OHCI_REG_RH_DESCRIPTORA
NTAPI
OHCI_ReadRhDescriptorA(IN POHCI_EXTENSION OhciExtension)
{
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    OHCI_REG_RH_DESCRIPTORA DescriptorA;
    PULONG DescriptorAReg;
    ULONG ix;

    OperationalRegs = OhciExtension->OperationalRegs;
    DescriptorAReg = (PULONG)&OperationalRegs->HcRhDescriptorA;

    DPRINT("OHCI_ReadRhDescriptorA: OhciExtension - %p\n", OhciExtension);

    for (ix = 0; ix < 10; ix++)
    {
        DescriptorA.AsULONG = READ_REGISTER_ULONG(DescriptorAReg);

        if (DescriptorA.AsULONG != 0 &&
            DescriptorA.Reserved == 0 &&
            DescriptorA.NumberDownstreamPorts <= OHCI_MAX_PORT_COUNT)
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
        PowerOnToPowerGoodTime = OHCI_MINIMAL_POTPGT;
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
    *Status = USB_GETSTATUS_SELF_POWERED;
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
    OHCI_REG_RH_PORT_STATUS OhciPortStatus;
    ULONG ix;
    ULONG Reserved;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_GetPortStatus: OhciExtension - %p, Port - %x, PortStatus - %lX\n",
           OhciExtension,
           Port,
           PortStatus->AsUlong32);

    ASSERT(Port > 0);

    OperationalRegs = OhciExtension->OperationalRegs;
    PortStatusReg = (PULONG)&OperationalRegs->HcRhPortStatus[Port-1];

    for (ix = 0; ix < 10; ix++)
    {
        OhciPortStatus.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

        Reserved = OhciPortStatus.Reserved1r |
                   OhciPortStatus.Reserved2r |
                   OhciPortStatus.Reserved3;

        if (OhciPortStatus.AsULONG && !Reserved)
            break;

        DPRINT("OHCI_RH_GetPortStatus: OhciPortStatus - %X\n", OhciPortStatus.AsULONG);

        KeStallExecutionProcessor(5);
    }

    PortStatus->AsUlong32 = OhciPortStatus.AsULONG;

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_GetHubStatus(IN PVOID ohciExtension,
                     IN PUSB_HUB_STATUS_AND_CHANGE HubStatus)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG RhStatusReg;
    OHCI_REG_RH_STATUS HcRhStatus;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_GetHubStatus: ohciExtension - %p, HubStatus - %lX\n",
           ohciExtension,
           HubStatus->AsUlong32);

    OperationalRegs = OhciExtension->OperationalRegs;
    RhStatusReg = (PULONG)&OperationalRegs->HcRhStatus;

    HcRhStatus.AsULONG = READ_REGISTER_ULONG(RhStatusReg);

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

    ASSERT(Port > 0);

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
    PULONG PortStatusReg;
    OHCI_REG_RH_PORT_STATUS PortStatus;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_SetFeaturePortPower: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    ASSERT(Port > 0);

    OperationalRegs = OhciExtension->OperationalRegs;
    PortStatusReg = (PULONG)&OperationalRegs->HcRhPortStatus[Port-1];

    PortStatus.AsULONG = 0;
    PortStatus.SetPortPower = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortStatus.AsULONG);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_SetFeaturePortEnable(IN PVOID ohciExtension,
                             IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG PortStatusReg;
    OHCI_REG_RH_PORT_STATUS PortStatus;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_SetFeaturePortEnable: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    ASSERT(Port > 0);

    OperationalRegs = OhciExtension->OperationalRegs;
    PortStatusReg = (PULONG)&OperationalRegs->HcRhPortStatus[Port-1];

    PortStatus.AsULONG = 0;
    PortStatus.SetPortEnable = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortStatus.AsULONG);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_SetFeaturePortSuspend(IN PVOID ohciExtension,
                              IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG PortStatusReg;
    OHCI_REG_RH_PORT_STATUS PortStatus;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_SetFeaturePortSuspend: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    ASSERT(Port > 0);

    OperationalRegs = OhciExtension->OperationalRegs;
    PortStatusReg = (PULONG)&OperationalRegs->HcRhPortStatus[Port-1];

    PortStatus.AsULONG = 0;
    PortStatus.SetPortSuspend = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortStatus.AsULONG);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortEnable(IN PVOID ohciExtension,
                               IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG PortStatusReg;
    OHCI_REG_RH_PORT_STATUS PortStatus;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortEnable: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    ASSERT(Port > 0);

    OperationalRegs = OhciExtension->OperationalRegs;
    PortStatusReg = (PULONG)&OperationalRegs->HcRhPortStatus[Port-1];

    PortStatus.AsULONG = 0;
    PortStatus.ClearPortEnable = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortStatus.AsULONG);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortPower(IN PVOID ohciExtension,
                              IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG PortStatusReg;
    OHCI_REG_RH_PORT_STATUS PortStatus;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortPower: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    ASSERT(Port > 0);

    OperationalRegs = OhciExtension->OperationalRegs;
    PortStatusReg = (PULONG)&OperationalRegs->HcRhPortStatus[Port-1];

    PortStatus.AsULONG = 0;
    PortStatus.ClearPortPower = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortStatus.AsULONG);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortSuspend(IN PVOID ohciExtension,
                                IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG PortStatusReg;
    OHCI_REG_RH_PORT_STATUS PortStatus;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortSuspend: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    ASSERT(Port > 0);

    OperationalRegs = OhciExtension->OperationalRegs;
    PortStatusReg = (PULONG)&OperationalRegs->HcRhPortStatus[Port-1];

    PortStatus.AsULONG = 0;
    PortStatus.ClearSuspendStatus = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortStatus.AsULONG);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortEnableChange(IN PVOID ohciExtension,
                                     IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG PortStatusReg;
    OHCI_REG_RH_PORT_STATUS PortStatus;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortEnableChange: ohciExtension - %p, Port - %x\n",
           ohciExtension,
           Port);

    ASSERT(Port > 0);

    OperationalRegs = OhciExtension->OperationalRegs;
    PortStatusReg = (PULONG)&OperationalRegs->HcRhPortStatus[Port-1];

    PortStatus.AsULONG = 0;
    PortStatus.PortEnableStatusChange = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortStatus.AsULONG);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortConnectChange(IN PVOID ohciExtension,
                                      IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG PortStatusReg;
    OHCI_REG_RH_PORT_STATUS PortStatus;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortConnectChange: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    ASSERT(Port > 0);

    OperationalRegs = OhciExtension->OperationalRegs;
    PortStatusReg = (PULONG)&OperationalRegs->HcRhPortStatus[Port-1];

    PortStatus.AsULONG = 0;
    PortStatus.ConnectStatusChange = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortStatus.AsULONG);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortResetChange(IN PVOID ohciExtension,
                                    IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG PortStatusReg;
    OHCI_REG_RH_PORT_STATUS PortStatus;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortResetChange: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    ASSERT(Port > 0);

    OperationalRegs = OhciExtension->OperationalRegs;
    PortStatusReg = (PULONG)&OperationalRegs->HcRhPortStatus[Port-1];

    PortStatus.AsULONG = 0;
    PortStatus.PortResetStatusChange = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortStatus.AsULONG);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortSuspendChange(IN PVOID ohciExtension,
                                      IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG PortStatusReg;
    OHCI_REG_RH_PORT_STATUS PortStatus;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortSuspendChange: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    ASSERT(Port > 0);

    OperationalRegs = OhciExtension->OperationalRegs;
    PortStatusReg = (PULONG)&OperationalRegs->HcRhPortStatus[Port-1];

    PortStatus.AsULONG = 0;
    PortStatus.PortSuspendStatusChange = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortStatus.AsULONG);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortOvercurrentChange(IN PVOID ohciExtension,
                                          IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG PortStatusReg;
    PULONG RhStatusReg;
    OHCI_REG_RH_PORT_STATUS PortStatus;
    OHCI_REG_RH_STATUS RhStatus;

    OhciExtension = ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortOvercurrentChange: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    if (Port)
    {
        /* USBPORT_RECIPIENT_PORT */
        PortStatus.AsULONG = 0;
        PortStatus.PortOverCurrentIndicatorChange = 1;

        PortStatusReg = (PULONG)&OperationalRegs->HcRhPortStatus[Port-1];
        WRITE_REGISTER_ULONG(PortStatusReg, PortStatus.AsULONG);
    }
    else
    {
        /* USBPORT_RECIPIENT_HUB */
        RhStatus.AsULONG = 0;
        RhStatus.OverCurrentIndicatorChangeW = 1;

        RhStatusReg = (PULONG)&OperationalRegs->HcRhStatus;
        WRITE_REGISTER_ULONG(RhStatusReg, RhStatus.AsULONG);
    }

    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
OHCI_RH_DisableIrq(IN PVOID ohciExtension)
{
    POHCI_EXTENSION  OhciExtension = ohciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG InterruptDisableReg;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE InterruptDisable;

    DPRINT("OHCI_RH_DisableIrq: OhciExtension - %p\n", OhciExtension);

    OperationalRegs = OhciExtension->OperationalRegs;
    InterruptDisableReg = (PULONG)&OperationalRegs->HcInterruptDisable;

    InterruptDisable.AsULONG = 0;
    InterruptDisable.RootHubStatusChange = 1;

    WRITE_REGISTER_ULONG(InterruptDisableReg, InterruptDisable.AsULONG);
}

VOID
NTAPI
OHCI_RH_EnableIrq(IN PVOID ohciExtension)
{
    POHCI_EXTENSION OhciExtension = ohciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG InterruptEnableReg;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE InterruptEnable;

    DPRINT("OHCI_RH_EnableIrq: OhciExtension - %p\n", OhciExtension);

    OperationalRegs = OhciExtension->OperationalRegs;
    InterruptEnableReg = (PULONG)&OperationalRegs->HcInterruptEnable;

    InterruptEnable.AsULONG = 0;
    InterruptEnable.RootHubStatusChange = 1;

    WRITE_REGISTER_ULONG(InterruptEnableReg, InterruptEnable.AsULONG);
}
