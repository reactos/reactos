#include "usbuhci.h"

//#define NDEBUG
#include <debug.h>

VOID
NTAPI
UhciRHGetRootHubData(IN PVOID uhciExtension,
                     IN PVOID rootHubData)
{
    DPRINT("UhciRHGetRootHubData: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
UhciRHGetStatus(IN PVOID uhciExtension,
                IN PUSHORT Status)
{
    DPRINT("UhciRHGetStatus: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
UhciRHGetPortStatus(IN PVOID uhciExtension,
                    IN USHORT Port,
                    IN PULONG PortStatus)
{
    DPRINT("UhciRHGetPortStatus: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
UhciRHGetHubStatus(IN PVOID uhciExtension,
                   IN PULONG HubStatus)
{
    DPRINT("UhciRHGetHubStatus: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
UhciRHSetFeaturePortReset(IN PVOID uhciExtension,
                          IN USHORT Port)
{
    DPRINT("UhciRHSetFeaturePortReset: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
UhciRHSetFeaturePortPower(IN PVOID uhciExtension,
                          IN USHORT Port)
{
    DPRINT("UhciRHSetFeaturePortPower: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
UhciRHSetFeaturePortEnable(IN PVOID uhciExtension,
                           IN USHORT Port)
{
    DPRINT("UhciRHSetFeaturePortEnable: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
UhciRHSetFeaturePortSuspend(IN PVOID uhciExtension,
                            IN USHORT Port)
{
    DPRINT("UhciRHSetFeaturePortSuspend: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
UhciRHClearFeaturePortEnable(IN PVOID uhciExtension,
                             IN USHORT Port)
{
    DPRINT("UhciRHClearFeaturePortEnable: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
UhciRHClearFeaturePortPower(IN PVOID uhciExtension,
                            IN USHORT Port)
{
    DPRINT("UhciRHClearFeaturePortPower: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
UhciRHClearFeaturePortSuspend(IN PVOID uhciExtension,
                              IN USHORT Port)
{
    DPRINT("UhciRHClearFeaturePortSuspend: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
UhciRHClearFeaturePortEnableChange(IN PVOID uhciExtension,
                                   IN USHORT Port)
{
    DPRINT("UhciRHClearFeaturePortEnableChange: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
UhciRHClearFeaturePortConnectChange(IN PVOID uhciExtension,
                                    IN USHORT Port)
{
    DPRINT("UhciRHClearFeaturePortConnectChange: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
UhciRHClearFeaturePortResetChange(IN PVOID uhciExtension,
                                  IN USHORT Port)
{
    DPRINT("UhciRHClearFeaturePortResetChange: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
UhciRHClearFeaturePortSuspendChange(IN PVOID uhciExtension,
                                    IN USHORT Port)
{
    DPRINT("UhciRHClearFeaturePortSuspendChange: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
UhciRHClearFeaturePortOvercurrentChange(IN PVOID uhciExtension,
                                        IN USHORT Port)
{
    DPRINT("UhciRHClearFeaturePortOvercurrentChange: UNIMPLEMENTED. FIXME\n");
    return 0;
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

