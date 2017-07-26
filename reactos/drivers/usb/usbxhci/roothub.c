#include "usbxhci.h"

//#define NDEBUG
#include <debug.h>

#define NDEBUG_XHCI_ROOT_HUB
#include "dbg_xhci.h"

VOID
NTAPI
XHCI_RH_GetRootHubData(IN PVOID xhciExtension,
                       IN PVOID rootHubData)
{
    DPRINT1("XHCI_RH_GetRootHubData: function initiated\n");
    PXHCI_EXTENSION XhciExtension;
    PUSBPORT_ROOT_HUB_DATA RootHubData;

    XhciExtension = (PXHCI_EXTENSION)xhciExtension;

    DPRINT_RH("XHCI_RH_GetRootHubData: XhciExtension - %p, rootHubData - %p\n",
              XhciExtension,
              rootHubData);

    RootHubData = (PUSBPORT_ROOT_HUB_DATA)rootHubData;

    RootHubData->NumberOfPorts = XhciExtension->NumberOfPorts;

   
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
XHCI_RH_GetStatus(IN PVOID xhciExtension,
                  IN PUSHORT Status)
{
    DPRINT1("XHCI_RH_GetStatus: function initiated\n");
    *Status = 1;
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_GetPortStatus(IN PVOID xhciExtension,
                      IN USHORT Port,
                      IN PULONG PortStatus)
{
    DPRINT1("XHCI_RH_GetPortStatus: function initiated\n");
    PXHCI_EXTENSION XhciExtension;
    PULONG PortStatusReg;
    XHCI_PORT_STATUS_CONTROL PortStatusRegister;
    //USB_30_PORT_STATUS x;
    XhciExtension = (PXHCI_EXTENSION)xhciExtension;
    PortStatusReg = (XhciExtension->OperationalRegs) + (XHCI_PORTSC + (Port - 1)*4);  
    PortStatusRegister.AsULONG = READ_REGISTER_ULONG(PortStatusReg) ;
    PortStatusRegister.AsULONG = 0;
    *PortStatus = PortStatusRegister.AsULONG;
    
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_GetHubStatus(IN PVOID xhciExtension,
                     IN PULONG HubStatus)
{
    DPRINT1("XHCI_RH_GetHubStatus: function initiated\n");
    *HubStatus = 0;
    return 0;
}

VOID
NTAPI
XHCI_RH_FinishReset(IN PVOID xhciExtension,
                    IN PUSHORT Port)
{
    DPRINT1("XHCI_RH_FinishReset: function initiated\n");
}

ULONG
NTAPI
XHCI_RH_PortResetComplete(IN PVOID xhciExtension,
                          IN PUSHORT Port)
{
    DPRINT1("XHCI_RH_PortResetComplete: function initiated\n");
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortReset(IN PVOID xhciExtension,
                            IN USHORT Port)
{
    DPRINT1("XHCI_RH_SetFeaturePortReset: function initiated\n");
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortPower(IN PVOID xhciExtension,
                            IN USHORT Port)
{
   DPRINT1("XHCI_RH_SetFeaturePortPower: function initiated\n");
    return 0;
}
MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortEnable(IN PVOID xhciExtension,
                             IN USHORT Port)
{
    DPRINT_RH("XHCI_RH_SetFeaturePortEnable: Not supported\n");
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortSuspend(IN PVOID xhciExtension,
                              IN USHORT Port)
{
    DPRINT1("XHCI_RH_SetFeaturePortSuspend: function initiated\n");
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortEnable(IN PVOID xhciExtension,
                               IN USHORT Port)
{
   DPRINT1("XHCI_RH_ClearFeaturePortEnable: function initiated\n");
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortPower(IN PVOID xhciExtension,
                              IN USHORT Port)
{
    DPRINT1("XHCI_RH_ClearFeaturePortPower: function initiated\n");
  
    return 0;
}

VOID
NTAPI
XHCI_RH_PortResumeComplete(IN PULONG xhciExtension,
                           IN PUSHORT Port)
{
   DPRINT1("XHCI_RH_PortResumeComplete: function initiated\n");
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortSuspend(IN PVOID xhciExtension,
                                IN USHORT Port)
{
    DPRINT1("XHCI_RH_ClearFeaturePortSuspend: function initiated\n");
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortEnableChange(IN PVOID xhciExtension,
                                     IN USHORT Port)
{
    DPRINT1("XHCI_RH_ClearFeaturePortEnableChange: function initiated\n");
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortConnectChange(IN PVOID xhciExtension,
                                      IN USHORT Port)
{
    DPRINT1("XHCI_RH_ClearFeaturePortConnectChange: function initiated\n");
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortResetChange(IN PVOID xhciExtension,
                                    IN USHORT Port)
{
    DPRINT1("XHCI_RH_ClearFeaturePortResetChange: function initiated\n");
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortSuspendChange(IN PVOID xhciExtension,
                                      IN USHORT Port)
{
    DPRINT1("XHCI_RH_ClearFeaturePortSuspendChange: function initiated\n");
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortOvercurrentChange(IN PVOID xhciExtension,
                                          IN USHORT Port)
{
    DPRINT1("XHCI_RH_ClearFeaturePortOvercurrentChange: function initiated\n");
    return 0;
}

VOID
NTAPI
XHCI_RH_DisableIrq(IN PVOID xhciExtension)
{
   DPRINT1("XHCI_RH_DisableIrq: function initiated\n");
   PXHCI_EXTENSION XhciExtension;
   PULONG OperationalRegs;
   XHCI_USB_COMMAND usbCommand;
   
   XhciExtension = (PXHCI_EXTENSION)xhciExtension;
   OperationalRegs = XhciExtension->OperationalRegs;
   usbCommand.AsULONG =READ_REGISTER_ULONG (OperationalRegs + XHCI_USBCMD);
   
   usbCommand.InterrupterEnable = 0;
   
   WRITE_REGISTER_ULONG(OperationalRegs + XHCI_USBCMD,usbCommand.AsULONG );
   DPRINT1("XHCI_RH_DisableIrq: Disable Interupts succesfull\n");
}

VOID
NTAPI
XHCI_RH_EnableIrq(IN PVOID xhciExtension)
{
   DPRINT1("XHCI_RH_EnableIrq: function initiated\n");
   PXHCI_EXTENSION XhciExtension;
   PULONG OperationalRegs;
   XHCI_USB_COMMAND usbCommand;
   
   XhciExtension = (PXHCI_EXTENSION)xhciExtension;
   OperationalRegs = XhciExtension->OperationalRegs;
   usbCommand.AsULONG =READ_REGISTER_ULONG (OperationalRegs + XHCI_USBCMD);
   
   usbCommand.InterrupterEnable = 1;
   
   WRITE_REGISTER_ULONG(OperationalRegs + XHCI_USBCMD,usbCommand.AsULONG );
   DPRINT1("XHCI_RH_EnableIrq: Enable Interupts\n");
   
}
