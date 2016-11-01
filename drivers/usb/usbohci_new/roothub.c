#include "usbohci.h"

#define NDEBUG
#include <debug.h>

ULONG
NTAPI 
OHCI_ReadRhDescriptorA(IN POHCI_EXTENSION OhciExtension)
{
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    ULONG DescriptorA;
    ULONG ix = 0;

    OperationalRegs = OhciExtension->OperationalRegs;

    DPRINT("OHCI_ReadRhDescriptorA: OhciExtension - %p\n", OhciExtension);

    do
    {
        DescriptorA = READ_REGISTER_ULONG(&OperationalRegs->HcRhDescriptorA.AsULONG);

        if (DescriptorA && !(DescriptorA & 0xFFE0F0)) // Reserved bits
            break;

        DPRINT1("OHCI_ReadRhDescriptorA: ix - %p\n", ix);

        KeStallExecutionProcessor(5);

        ++ix;
    }
    while (ix < 10);

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
    USHORT HubCharacteristics;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;

    DPRINT("OHCI_RH_GetRootHubData: OhciExtension - %p, rootHubData - %p\n",
           OhciExtension,
           rootHubData);

    RootHubData = (PUSBPORT_ROOT_HUB_DATA)rootHubData;
    DescriptorA.AsULONG = OHCI_ReadRhDescriptorA(OhciExtension);

    RootHubData->NumberOfPorts = DescriptorA.NumberDownstreamPorts;

    PowerOnToPowerGoodTime = DescriptorA.PowerOnToPowerGoodTime;
    if (PowerOnToPowerGoodTime <= 25)
    {
        PowerOnToPowerGoodTime = 25;
    }
    RootHubData->PowerOnToPowerGood = PowerOnToPowerGoodTime;

    HubCharacteristics = (DescriptorA.AsULONG >> 8) & 0xFFFC;
    RootHubData->HubCharacteristics = HubCharacteristics;

    if (DescriptorA.PowerSwitchingMode)
    {
        RootHubData->HubCharacteristics = (HubCharacteristics & 0xFFFD) | 1;
    }

    RootHubData->HubControlCurrent = 0;
}

MPSTATUS
NTAPI
OHCI_RH_GetStatus(IN PVOID ohciExtension,
                  IN PUSHORT Status)
{
    DPRINT("OHCI_RH_GetStatus: \n");
    *Status = 1;
    return 0;
}

MPSTATUS
NTAPI
OHCI_RH_GetPortStatus(IN PVOID ohciExtension,
                      IN USHORT Port,
                      IN PULONG PortStatus)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    ULONG portStatus;
    ULONG ix = 0;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;

    DPRINT("OHCI_RH_GetPortStatus: OhciExtension - %p, Port - %x, PortStatus - %p\n",
           OhciExtension,
           Port,
           *PortStatus);

    OperationalRegs = ((POHCI_EXTENSION)OhciExtension)->OperationalRegs;

    do
    {
        portStatus = READ_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG);

        //DPRINT("OHCI_RH_GetPortStatus: &portreg - %p\n",
        //       &OperationalRegs->HcRhPortStatus[Port-1].AsULONG);

        if ( portStatus && !(portStatus & 0xFFE0FCE0) )
            break;

        KeStallExecutionProcessor(5);

        ++ix;
    }
    while ( ix < 10 );

    *PortStatus = portStatus;

    DPRINT("OHCI_RH_GetPortStatus: OhciExtension - %p, Port - %x, PortStatus - %p\n",
           OhciExtension,
           Port,
           *PortStatus);

    if (0)//Port == 1)
    {
        DPRINT("HcRevision         - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcRevision));
        DPRINT("HcControl          - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcControl.AsULONG));
        DPRINT("HcCommandStatus    - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcCommandStatus.AsULONG));
        DPRINT("HcInterruptStatus  - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcInterruptStatus.AsULONG));
        DPRINT("HcInterruptEnable  - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcInterruptEnable.AsULONG));
        DPRINT("HcInterruptDisable - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcInterruptDisable.AsULONG));
        DPRINT("HcHCCA             - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcHCCA));
        DPRINT("HcPeriodCurrentED  - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcPeriodCurrentED));
        DPRINT("HcControlHeadED    - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcControlHeadED));
        DPRINT("HcControlCurrentED - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcControlCurrentED));
        DPRINT("HcBulkHeadED       - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcBulkHeadED));
        DPRINT("HcBulkCurrentED    - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcBulkCurrentED));
        DPRINT("HcDoneHead         - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcDoneHead));
        DPRINT("HcFmInterval       - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcFmInterval.AsULONG));
        DPRINT("HcFmRemaining      - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcFmRemaining));
        DPRINT("HcFmNumber         - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcFmNumber));
        DPRINT("HcPeriodicStart    - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcPeriodicStart));
        DPRINT("HcLSThreshold      - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcLSThreshold));
        DPRINT("HcRhDescriptorA    - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcRhDescriptorA.AsULONG));
        DPRINT("HcRhDescriptorB    - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcRhDescriptorB));
        DPRINT("HcRhStatus         - %p\n", READ_REGISTER_ULONG(&OperationalRegs->HcRhStatus.AsULONG));
    }

    return 0;
}

MPSTATUS
NTAPI
OHCI_RH_GetHubStatus(IN PVOID ohciExtension,
                     IN PULONG HubStatus)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;

    DPRINT("OHCI_RH_GetHubStatus: ohciExtension - %p, HubStatus - %x\n",
           ohciExtension,
           HubStatus);

    OperationalRegs = OhciExtension->OperationalRegs;

    *HubStatus &= ~0x10001;
    *HubStatus ^= (READ_REGISTER_ULONG(&OperationalRegs->HcRhStatus.AsULONG) ^
                  *HubStatus) & 0x20002;

    return 0;
}

MPSTATUS
NTAPI
OHCI_RH_SetFeaturePortReset(IN PVOID ohciExtension,
                            IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;

    DPRINT("OHCI_RH_SetFeaturePortReset: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         0x10);

    return 0;
}

MPSTATUS
NTAPI
OHCI_RH_SetFeaturePortPower(IN PVOID ohciExtension,
                            IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;

    DPRINT("OHCI_RH_SetFeaturePortPower: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         0x100);

    return 0;
}

MPSTATUS
NTAPI
OHCI_RH_SetFeaturePortEnable(IN PVOID ohciExtension,
                             IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;

    DPRINT("OHCI_RH_SetFeaturePortEnable: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         2);

    return 0;
}

MPSTATUS
NTAPI
OHCI_RH_SetFeaturePortSuspend(IN PVOID ohciExtension,
                              IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;

    DPRINT("OHCI_RH_SetFeaturePortSuspend: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         4);

    return 0;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortEnable(IN PVOID ohciExtension,
                               IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortEnable: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         1);

    return 0;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortPower(IN PVOID ohciExtension,
                              IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortPower: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         0x200);

    return 0;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortSuspend(IN PVOID ohciExtension,
                                IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortSuspend: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         8);

    return 0;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortEnableChange(IN PVOID ohciExtension,
                                     IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortEnableChange: ohciExtension - %p, Port - %x\n",
           ohciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         0x20000);

    return 0;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortConnectChange(IN PVOID ohciExtension,
                                      IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortConnectChange: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         0x10000);

    return 0;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortResetChange(IN PVOID ohciExtension,
                                    IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortResetChange: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         0x100000);
    return 0;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortSuspendChange(IN PVOID ohciExtension,
                                      IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;

    DPRINT("OHCI_RH_ClearFeaturePortSuspendChange: OhciExtension - %p, Port - %x\n",
           OhciExtension,
           Port);

    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhPortStatus[Port-1].AsULONG,
                         0x40000);

    return 0;
}

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortOvercurrentChange(IN PVOID ohciExtension,
                                          IN USHORT Port)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;

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

    return 0;
}

VOID
NTAPI
OHCI_RH_DisableIrq(IN PVOID ohciExtension)
{
    POHCI_EXTENSION  OhciExtension = (POHCI_EXTENSION)ohciExtension;
    DPRINT("OHCI_RH_DisableIrq: OhciExtension - %p\n", OhciExtension);

    WRITE_REGISTER_ULONG(&OhciExtension->OperationalRegs->HcInterruptDisable.AsULONG,
                         0x40);
}

VOID
NTAPI
OHCI_RH_EnableIrq(IN PVOID ohciExtension)
{
    POHCI_EXTENSION OhciExtension = (POHCI_EXTENSION)ohciExtension;
    DPRINT("OHCI_RH_EnableIrq: OhciExtension - %p\n", OhciExtension);

    WRITE_REGISTER_ULONG(&OhciExtension->OperationalRegs->HcInterruptEnable.AsULONG,
                         0x40);
}
