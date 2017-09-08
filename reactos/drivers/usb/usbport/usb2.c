/*
 * PROJECT:     ReactOS USB Port Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBPort USB 2.0 functions
 * COPYRIGHT:   Copyright 2017 Vadim Galyant <vgal@rambler.ru>
 */

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
