#include "usbport.h"

//#define NDEBUG
#include <debug.h>

BOOLEAN
NTAPI
USBPORT_AllocateBandwidthUSB2(IN PDEVICE_OBJECT FdoDevice,
                              IN PUSBPORT_ENDPOINT Endpoint)
{
    DPRINT1("USBPORT_AllocateBandwidthUSB2: UNIMPLEMENTED. FIXME. \n");
    return TRUE;
}

VOID
NTAPI
USBPORT_FreeBandwidthUSB2(IN PDEVICE_OBJECT FdoDevice,
                          IN PUSBPORT_ENDPOINT Endpoint)
{
    DPRINT1("USBPORT_FreeBandwidthUSB2: UNIMPLEMENTED. FIXME. \n");
}
