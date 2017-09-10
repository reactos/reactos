/*
 * PROJECT:     ReactOS USB Port Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBPort isochronous transfer functions
 * COPYRIGHT:   Copyright 2017 Vadim Galyant <vgal@rambler.ru>
 */

#include "usbport.h"

#define NDEBUG
#include <debug.h>

USBD_STATUS
NTAPI
USBPORT_InitializeIsoTransfer(PDEVICE_OBJECT FdoDevice,
                              struct _URB_ISOCH_TRANSFER * Urb,
                              PUSBPORT_TRANSFER Transfer)
{
    DPRINT1("USBPORT_InitializeIsoTransfer: UNIMPLEMENTED. FIXME.\n");
    return USBD_STATUS_NOT_SUPPORTED;
}

ULONG
NTAPI
USBPORT_CompleteIsoTransfer(IN PVOID MiniPortExtension,
                            IN PVOID MiniPortEndpoint,
                            IN PVOID TransferParameters,
                            IN ULONG TransferLength)
{
    DPRINT1("USBPORT_CompleteIsoTransfer: UNIMPLEMENTED. FIXME.\n");
    return 0;
}

