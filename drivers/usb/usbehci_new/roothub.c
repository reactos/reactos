#include "usbehci.h"

//#define NDEBUG
#include <debug.h>

VOID
NTAPI
EHCI_RH_GetRootHubData(IN PVOID ehciExtension,
                       IN PVOID rootHubData)
{
    PEHCI_EXTENSION EhciExtension;
    PUSBPORT_ROOT_HUB_DATA RootHubData;

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;

    DPRINT("EHCI_RH_GetRootHubData: EhciExtension - %p, rootHubData - %p\n",
           EhciExtension,
           rootHubData);

    RootHubData = (PUSBPORT_ROOT_HUB_DATA)rootHubData;

    RootHubData->NumberOfPorts = EhciExtension->NumberOfPorts;

    /* Logical Power Switching Mode */
    if (EhciExtension->PortPowerControl == 1)
    {
        /* Individual port power switching */
        RootHubData->HubCharacteristics = (RootHubData->HubCharacteristics & ~2) | 1;
    }
    else
    {
        /* Ganged power switching (all ports’ power at once) */
        RootHubData->HubCharacteristics &= ~3;
    }

    /* 
        Identifies a Compound Device: Hub is not part of a compound device.
        Over-current Protection Mode: Global Over-current Protection.
    */
    RootHubData->HubCharacteristics &= 3;

    RootHubData->PowerOnToPowerGood = 2;
    RootHubData->HubControlCurrent = 0;
}

MPSTATUS
NTAPI
EHCI_RH_GetStatus(IN PVOID ehciExtension,
                  IN PUSHORT Status)
{
    DPRINT("EHCI_RH_GetStatus: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_GetPortStatus(IN PVOID ehciExtension,
                      IN USHORT Port,
                      IN PULONG PortStatus)
{
    DPRINT("EHCI_RH_GetPortStatus: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_GetHubStatus(IN PVOID ehciExtension,
                     IN PULONG HubStatus)
{
    DPRINT("EHCI_RH_GetHubStatus: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_SetFeaturePortReset(IN PVOID ehciExtension,
                            IN USHORT Port)
{
    DPRINT("EHCI_RH_SetFeaturePortReset: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_SetFeaturePortPower(IN PVOID ehciExtension,
                            IN USHORT Port)
{
    DPRINT("EHCI_RH_SetFeaturePortPower: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_SetFeaturePortEnable(IN PVOID ehciExtension,
                             IN USHORT Port)
{
    DPRINT("EHCI_RH_SetFeaturePortEnable: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_SetFeaturePortSuspend(IN PVOID ehciExtension,
                              IN USHORT Port)
{
    DPRINT("EHCI_RH_SetFeaturePortSuspend: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortEnable(IN PVOID ehciExtension,
                               IN USHORT Port)
{
    DPRINT("EHCI_RH_ClearFeaturePortEnable: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortPower(IN PVOID ehciExtension,
                              IN USHORT Port)
{
    DPRINT("EHCI_RH_ClearFeaturePortPower: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortSuspend(IN PVOID ehciExtension,
                                IN USHORT Port)
{
    DPRINT("EHCI_RH_ClearFeaturePortSuspend: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortEnableChange(IN PVOID ehciExtension,
                                     IN USHORT Port)
{
    DPRINT("EHCI_RH_ClearFeaturePortEnableChange: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortConnectChange(IN PVOID ehciExtension,
                                      IN USHORT Port)
{
    DPRINT("EHCI_RH_ClearFeaturePortConnectChange: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortResetChange(IN PVOID ehciExtension,
                                    IN USHORT Port)
{
    DPRINT("EHCI_RH_ClearFeaturePortResetChange: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortSuspendChange(IN PVOID ehciExtension,
                                      IN USHORT Port)
{
    DPRINT("EHCI_RH_ClearFeaturePortSuspendChange: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortOvercurrentChange(IN PVOID ehciExtension,
                                          IN USHORT Port)
{
    DPRINT("EHCI_RH_ClearFeaturePortOvercurrentChange: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
EHCI_RH_DisableIrq(IN PVOID ehciExtension)
{
    PEHCI_EXTENSION EhciExtension;
    PULONG IntrStsReg;
    EHCI_INTERRUPT_ENABLE IntrSts;

    DPRINT("EHCI_RH_DisableIrq: ... \n");

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;

    IntrStsReg = EhciExtension->OperationalRegs + EHCI_USBINTR;
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
    PEHCI_EXTENSION EhciExtension;
    PULONG IntrStsReg;
    EHCI_INTERRUPT_ENABLE IntrSts;

    DPRINT("EHCI_RH_EnableIrq: ... \n");

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;

    IntrStsReg = EhciExtension->OperationalRegs + EHCI_USBINTR;
    IntrSts.AsULONG = READ_REGISTER_ULONG(IntrStsReg);

    EhciExtension->InterruptMask.PortChangeInterrupt = 1;
    IntrSts.PortChangeInterrupt = 1;

    if (IntrSts.Interrupt)
    {
        WRITE_REGISTER_ULONG(IntrStsReg, IntrSts.AsULONG);
    }
}

