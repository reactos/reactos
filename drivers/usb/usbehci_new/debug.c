/*
 * PROJECT:     ReactOS USB EHCI Miniport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBEHCI debugging functions
 * COPYRIGHT:   Copyright 2017-2018 Vadim Galyant <vgal@rambler.ru>
 */

#include "usbehci.h"

//#define NDEBUG
#include <debug.h>

VOID
NTAPI
EHCI_DumpHwTD(IN PEHCI_HCD_TD TD)
{
    while (TD)
    {
        DPRINT(": TD                       - %p\n", TD);
        DPRINT(": TD->PhysicalAddress      - %lx\n", TD->PhysicalAddress);
        DPRINT(": TD->HwTD.NextTD          - %lx\n", TD->HwTD.NextTD);
        DPRINT(": TD->HwTD.AlternateNextTD - %lx\n", TD->HwTD.AlternateNextTD);
        DPRINT(": TD->HwTD.Token.AsULONG   - %lx\n", TD->HwTD.Token.AsULONG);

        TD = TD->NextHcdTD;
    }
}

VOID
NTAPI
EHCI_DumpHwQH(IN PEHCI_HCD_QH QH)
{
    if (!QH)
        return;

    DPRINT(": QH->sqh.HwQH.CurrentTD       - %lx\n", QH->sqh.HwQH.CurrentTD);
    DPRINT(": QH->sqh.HwQH.NextTD          - %lx\n", QH->sqh.HwQH.NextTD);
    DPRINT(": QH->sqh.HwQH.AlternateNextTD - %lx\n", QH->sqh.HwQH.AlternateNextTD);
    DPRINT(": QH->sqh.HwQH.Token.AsULONG   - %lx\n", QH->sqh.HwQH.Token.AsULONG);
}
