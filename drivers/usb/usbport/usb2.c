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

VOID
NTAPI
USB2_InitController(IN PUSB2_HC_EXTENSION HcExtension)
{
    ULONG ix;
    ULONG jx;

    DPRINT("USB2_InitController: HcExtension - %p\n", HcExtension);

    HcExtension->MaxHsBusAllocation = USB2_MAX_MICROFRAME_ALLOCATION;

    for (ix = 0; ix < USB2_FRAMES; ix++)
    {
        for (jx = 0; jx < USB2_MICROFRAMES; jx++)
        {
            HcExtension->TimeUsed[ix][jx] = 0;
        }
    }

    HcExtension->HcDelayTime = USB2_CONTROLLER_DELAY;

    USB2_InitTT(HcExtension, &HcExtension->HcTt);
}
