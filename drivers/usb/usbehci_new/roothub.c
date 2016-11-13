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
    DPRINT("EHCI_RH_GetStatus: ... \n");
    *Status = 1;
    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_GetPortStatus(IN PVOID ehciExtension,
                      IN USHORT Port,
                      IN PULONG PortStatus)
{
    PEHCI_EXTENSION EhciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;
    USB20_PORT_STATUS status;
    ULONG PortMaskBits;

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;

    PortStatusReg = (EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);
    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

    if (PortSC.CurrentConnectStatus)
    {
        DPRINT("EHCI_RH_GetPortStatus: Port - %x, PortSC.AsULONG - %p\n", Port, PortSC.AsULONG);
    }

    *PortStatus = 0;

    if (PortSC.LineStatus == 1 && // K-state  Low-speed device
        PortSC.PortOwner != 1 && // Companion HC not owns and not controls this port
        (PortSC.PortEnabledDisabled | PortSC.Suspend) && // Enable or Suspend
        PortSC.CurrentConnectStatus == 1) // Device is present
    {
        DPRINT("EHCI_RH_GetPortStatus: LowSpeed device detected\n");
        PortSC.PortOwner = 1; // release ownership
        WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);
        return 0;
    }

    status.AsULONG = 0;

    status.ConnectStatus = PortSC.CurrentConnectStatus;
    status.EnableStatus = PortSC.PortEnabledDisabled;
    status.OverCurrent = PortSC.OverCurrentActive;
    status.ResetStatus = PortSC.PortReset;
    status.PowerStatus = PortSC.PortPower;
    status.SuspendStatus = PortSC.Suspend;
    // PortSC.PortOwner ??
    status.EnableStatusChange = PortSC.PortEnableDisableChange;
    status.OverCurrentChange = PortSC.OverCurrentChange;

    PortMaskBits = 1 << (Port - 1);

    if (status.ConnectStatus)
    {
        status.LsDeviceAttached = 0;
    }

    status.HsDeviceAttached = 1;

    if (PortSC.ConnectStatusChange)
    {
        EhciExtension->ConnectPortBits |= PortMaskBits;
    }

    if (EhciExtension->FinishResetPortBits & PortMaskBits)
    {
        status.ResetStatusChange = 1;
    }

    if (EhciExtension->ConnectPortBits & PortMaskBits)
    {
        status.ConnectStatusChange = 1;
    }

    if (EhciExtension->SuspendPortBits & PortMaskBits)
    {
        status.SuspendStatusChange = 1;
    }

    *PortStatus = status.AsULONG;

    if (status.ConnectStatus)
    {
        DPRINT("EHCI_RH_GetPortStatus: Port - %x, status.AsULONG - %p\n", Port, status.AsULONG);
    }

    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_GetHubStatus(IN PVOID ehciExtension,
                     IN PULONG HubStatus)
{
    DPRINT("EHCI_RH_GetHubStatus: ... \n");
    *HubStatus = 0;
    return 0;
}

VOID
NTAPI
EHCI_RH_FinishReset(IN PVOID ehciExtension,
                    IN PUSHORT Port)
{
    PEHCI_EXTENSION EhciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT("EHCI_RH_FinishReset: *Port - %x\n", *Port);

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;

    PortStatusReg = (EhciExtension->OperationalRegs + EHCI_PORTSC) + (*Port - 1);
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

ULONG
NTAPI
EHCI_RH_PortResetComplete(IN PVOID ehciExtension,
                          IN PUSHORT Port)
{
    PEHCI_EXTENSION EhciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;
    ULONG ix;

    DPRINT("EHCI_RH_PortResetComplete: *Port - %x\n", *Port);

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;
    PortStatusReg = (EhciExtension->OperationalRegs + EHCI_PORTSC) + (*Port - 1);

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

    return RegPacket.UsbPortRequestAsyncCallback(EhciExtension,
                                                 50, // TimerValue
                                                 Port,
                                                 sizeof(Port),
                                                 (ULONG)EHCI_RH_FinishReset);
}

MPSTATUS
NTAPI
EHCI_RH_SetFeaturePortReset(IN PVOID ehciExtension,
                            IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT("EHCI_RH_SetFeaturePortReset: Port - %x\n", Port);

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;
    PortStatusReg = (EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);

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
                                          (ULONG_PTR)EHCI_RH_PortResetComplete);

    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_SetFeaturePortPower(IN PVOID ehciExtension,
                            IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT("EHCI_RH_SetFeaturePortPower: Port - %x\n", Port);

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;
    PortStatusReg = (EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

    PortSC.ConnectStatusChange = 0;
    PortSC.PortEnableDisableChange = 0;
    PortSC.OverCurrentChange = 0;
    PortSC.PortPower = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);

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
    PEHCI_EXTENSION EhciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT("EHCI_RH_SetFeaturePortSuspend: Port - %x\n", Port);

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;
    PortStatusReg = (EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

    PortSC.ConnectStatusChange = 0;
    PortSC.PortEnableDisableChange = 0;
    PortSC.OverCurrentChange = 0;
    PortSC.Suspend = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);
    KeStallExecutionProcessor(125);

    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortEnable(IN PVOID ehciExtension,
                               IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT("EHCI_RH_ClearFeaturePortEnable: Port - %x\n", Port);

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;
    PortStatusReg = (EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

    PortSC.ConnectStatusChange = 0;
    PortSC.PortEnabledDisabled = 0;
    PortSC.PortEnableDisableChange = 0;
    PortSC.OverCurrentChange = 0;

    WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);

    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortPower(IN PVOID ehciExtension,
                              IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT("EHCI_RH_ClearFeaturePortPower: Port - %x\n", Port);

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;
    PortStatusReg = (EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);
    PortSC.PortPower = 0;
    WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);

    return 0;
}

VOID
NTAPI
EHCI_RH_PortResumeComplete(IN PULONG ehciExtension,
                           IN PUSHORT Port)
{
    PEHCI_EXTENSION EhciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT("EHCI_RH_PortResumeComplete: *Port - %x\n", *Port);

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;
    PortStatusReg = (EhciExtension->OperationalRegs + EHCI_PORTSC) + (*Port - 1);

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
    PEHCI_EXTENSION EhciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT("EHCI_RH_ClearFeaturePortSuspend: Port - %x\n", Port);

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;
    PortStatusReg = (EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);
    EhciExtension->ResetPortBits |= 1 << (Port - 1);

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);
    PortSC.ForcePortResume = 1;
    WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);

    RegPacket.UsbPortRequestAsyncCallback(EhciExtension,
                                          50, // TimerValue
                                          &Port,
                                          sizeof(Port),
                                          (ULONG)EHCI_RH_PortResumeComplete);

    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortEnableChange(IN PVOID ehciExtension,
                                     IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT("EHCI_RH_ClearFeaturePortEnableChange: Port - %p\n", Port);

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;

    PortStatusReg = (EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

    PortSC.ConnectStatusChange = 0;
    PortSC.OverCurrentChange = 0;
    PortSC.PortEnableDisableChange = 1;

    WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);

    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortConnectChange(IN PVOID ehciExtension,
                                      IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension;
    PULONG PortStatusReg;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT("EHCI_RH_ClearFeaturePortConnectChange: Port - %x\n", Port);

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;
    PortStatusReg = (EhciExtension->OperationalRegs + EHCI_PORTSC) + (Port - 1);

    PortSC.AsULONG = READ_REGISTER_ULONG(PortStatusReg);

    if (PortSC.ConnectStatusChange)
    {
        PortSC.ConnectStatusChange = 1;
        PortSC.PortEnableDisableChange = 0;
        PortSC.OverCurrentChange = 0;

        WRITE_REGISTER_ULONG(PortStatusReg, PortSC.AsULONG);
    }

    EhciExtension->ConnectPortBits &= ~(1 << (Port - 1));

    return 0;
}

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortResetChange(IN PVOID ehciExtension,
                                    IN USHORT Port)
{
    PEHCI_EXTENSION EhciExtension;

    DPRINT("EHCI_RH_ClearFeaturePortConnectChange: Port - %x\n", Port);

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;
    EhciExtension->FinishResetPortBits &= ~(1 << (Port - 1));
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

