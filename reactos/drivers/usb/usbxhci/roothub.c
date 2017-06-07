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
    
}

MPSTATUS
NTAPI
XHCI_RH_GetStatus(IN PVOID xhciExtension,
                  IN PUSHORT Status)
{
    
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_GetPortStatus(IN PVOID xhciExtension,
                      IN USHORT Port,
                      IN PULONG PortStatus)
{
    

    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_GetHubStatus(IN PVOID xhciExtension,
                     IN PULONG HubStatus)
{
    
    return 0;
}

VOID
NTAPI
XHCI_RH_FinishReset(IN PVOID xhciExtension,
                    IN PUSHORT Port)
{
    
}

ULONG
NTAPI
XHCI_RH_PortResetComplete(IN PVOID xhciExtension,
                          IN PUSHORT Port)
{
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortReset(IN PVOID xhciExtension,
                            IN USHORT Port)
{
    
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortPower(IN PVOID xhciExtension,
                            IN USHORT Port)
{
   
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
    
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortEnable(IN PVOID xhciExtension,
                               IN USHORT Port)
{
   
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortPower(IN PVOID xhciExtension,
                              IN USHORT Port)
{
  
    return 0;
}

VOID
NTAPI
XHCI_RH_PortResumeComplete(IN PULONG xhciExtension,
                           IN PUSHORT Port)
{
   
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortSuspend(IN PVOID xhciExtension,
                                IN USHORT Port)
{
  
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortEnableChange(IN PVOID xhciExtension,
                                     IN USHORT Port)
{
    
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortConnectChange(IN PVOID xhciExtension,
                                      IN USHORT Port)
{
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortResetChange(IN PVOID xhciExtension,
                                    IN USHORT Port)
{
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortSuspendChange(IN PVOID xhciExtension,
                                      IN USHORT Port)
{
    
    return 0;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortOvercurrentChange(IN PVOID xhciExtension,
                                          IN USHORT Port)
{
    
    return 0;
}

VOID
NTAPI
XHCI_RH_DisableIrq(IN PVOID xhciExtension)
{
   
}

VOID
NTAPI
XHCI_RH_EnableIrq(IN PVOID xhciExtension)
{
   
}
