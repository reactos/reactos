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
    PULONG PortStatusRegPointer;
    XHCI_PORT_STATUS_CONTROL PortStatusRegister;
    USBHUB_PORT_STATUS portstatus;
    
    XhciExtension = (PXHCI_EXTENSION)xhciExtension;
    PortStatusRegPointer = (XhciExtension->OperationalRegs) + (XHCI_PORTSC + (Port - 1)*4);  
    PortStatusRegister.AsULONG = READ_REGISTER_ULONG(PortStatusRegPointer) ;
    
    /*
    ULONG ConnectStatus          : 1; // Current Connect Status
    ULONG EnableStatus           : 1; // Port Enabled/Disabled
    ULONG SuspendStatus          : 1;
    ULONG OverCurrent            : 1;
    ULONG ResetStatus            : 1;
    ULONG Reserved1              : 3;
    ULONG PowerStatus            : 1;
    ULONG LsDeviceAttached       : 1; // Low-Speed Device Attached
    ULONG HsDeviceAttached       : 1; // High-speed Device Attached
    ULONG TestMode               : 1; // Port Test Mode
    ULONG IndicatorControl       : 1; // Port Indicator Control
    ULONG Reserved2              : 3;
    ULONG ConnectStatusChange    : 1;
    ULONG EnableStatusChange     : 1;
    ULONG SuspendStatusChange    : 1;
    ULONG OverCurrentChange      : 1;
    ULONG ResetStatusChange      : 1;
    ULONG Reserved3              : 3;
    ULONG PowerStatusChange      : 1;
    ULONG LsDeviceAttachedChange : 1;
    ULONG HsDeviceAttachedChange : 1;
    ULONG TestModeChange         : 1;
    ULONG IndicatorControlChange : 1;
    ULONG Reserved4              : 3;
    */
    /*
    ULONG CurrentConnectStatus                  : 1;
    ULONG PortEnableDisable                     : 1;
    ULONG RsvdZ1                                : 1;
    ULONG OverCurrentActive                     : 1;
    ULONG PortReset                             : 1;
    ULONG PortLinkState                         : 4;
    ULONG PortPower                             : 1;
    ULONG PortSpeed                             : 4;
    ULONG PortIndicatorControl                  : 2;
    ULONG LinkWriteStrobe                       : 1;
    ULONG ConnectStatusChange                   : 1;
    ULONG PortEnableDisableChange               : 1;
    ULONG WarmResetChange                       : 1;
    ULONG OverCurrentChange                     : 1;
    ULONG PortResetChange                       : 1;
    ULONG PortLinkStateChange                   : 1;
    ULONG ConfigErrorChange                     : 1;
    ULONG ColdAttachStatus                      : 1;
    ULONG WakeONConnectEnable                   : 1;
    ULONG WakeONDisconnectEnable                : 1;
    ULONG WakeONOverCurrentEnable               : 1;
    ULONG RsvdZ2                                : 2;
    ULONG DeviceRemovable                       : 1;
    ULONG WarmPortReset                         : 1;
    */
    portstatus.AsULONG = 0;
    portstatus.UsbPortStatus.ConnectStatus = PortStatusRegister.CurrentConnectStatus;
    portstatus.UsbPortStatus.EnableStatus = PortStatusRegister.PortEnableDisable;
    portstatus.UsbPortStatus.SuspendStatus = 0;//PortStatusRegister.PortEnableDisable;
    portstatus.UsbPortStatus.OverCurrent = PortStatusRegister.OverCurrentActive;
    portstatus.UsbPortStatus.ResetStatus = PortStatusRegister.PortReset;
    portstatus.UsbPortStatus.PowerStatus = PortStatusRegister.PortPower;
    portstatus.UsbPortStatus.LsDeviceAttached = 0;//PortStatusRegister.PortEnableDisable;
   // if (PortStatusRegister.PortSpeed) {
        portstatus.UsbPortStatus.HsDeviceAttached =  PortStatusRegister.CurrentConnectStatus;
   // }
    portstatus.UsbPortStatus.TestMode = 0;//PortStatusRegister.PortPower;
    portstatus.UsbPortStatus.IndicatorControl = 0;//PortStatusRegister.PortIndicatorControl;
    
    portstatus.UsbPortStatusChange.ConnectStatusChange = PortStatusRegister.ConnectStatusChange;
    portstatus.UsbPortStatusChange.EnableStatusChange = PortStatusRegister.PortEnableDisableChange;
    portstatus.UsbPortStatusChange.SuspendStatusChange = 0;//PortStatusRegister.ConnectStatusChange;
    portstatus.UsbPortStatusChange.OverCurrentChange = PortStatusRegister.OverCurrentChange;
    portstatus.UsbPortStatusChange.ResetStatusChange = PortStatusRegister.PortResetChange;
    portstatus.UsbPortStatusChange.PowerStatusChange = 0;//PortStatusRegister.ConnectStatusChange;
    portstatus.UsbPortStatusChange.LsDeviceAttachedChange = 0;//PortStatusRegister.ConnectStatusChange;
    portstatus.UsbPortStatusChange.HsDeviceAttachedChange = PortStatusRegister.ConnectStatusChange;
    portstatus.UsbPortStatusChange.TestModeChange = 0;//PortStatusRegister.ConnectStatusChange;
    portstatus.UsbPortStatusChange.IndicatorControlChange =0;// PortStatusRegister.ConnectStatusChange;
   
    *PortStatus = portstatus.AsULONG;
    
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
  //DPRINT1("XHCI_RH_DisableIrq: Disable Interupts succesfull\n");
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
   //DPRINT1("XHCI_RH_EnableIrq: Enable Interupts\n");
   
}
