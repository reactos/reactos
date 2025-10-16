/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CMD PCI IDE controller minidriver
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * CMD640 does not support 32-bit PCI configuration space writes.
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* GLOBALS ********************************************************************/

#define PCI_DEV_PCI0640            0x0640
#define PCI_DEV_PCI0643            0x0643
#define PCI_DEV_PCI0646            0x0646
#define PCI_DEV_PCI0648            0x0648
#define PCI_DEV_CMD0649            0x0649
#define PCI_DEV_SIL0680            0x0680

#define CMD_REG_CNTRL          0x51
#define CMD_REG_ARTTIM23       0x57

#define CMD_CONFIG_PREFETCH_DISABLE(Channel)  (0xC0 >> (4 * (Channel)))

/* FUNCTIONS ******************************************************************/

/*
 * Disable the Read-Ahead Mode.
 * https://web.archive.org/web/20250324132451/https://www.mindprod.com/jgloss/eideflaw.html
 */
static
VOID
Cmd640ControllerStart(
    _In_ PATA_CONTROLLER Controller)
{
    static const ULONG Registers[] = { CMD_REG_CNTRL, CMD_REG_ARTTIM23 };
    ULONG i;
    UCHAR Byte;

    for (i = 0; i < RTL_NUMBER_OF(Registers); ++i)
    {
        PciRead(Controller, &Byte, Registers[i], sizeof(Byte));
        Byte |= CMD_CONFIG_PREFETCH_DISABLE(i);
        PciWrite(Controller, &Byte, Registers[i], sizeof(Byte));
    }
}

CODE_SEG("PAGE")
NTSTATUS
CmdGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    NTSTATUS Status;

    PAGED_CODE();
    ASSERT(Controller->Pci.VendorID == PCI_VEN_CMD);

    if (Controller->Pci.DeviceID != PCI_DEV_PCI0640)
        return STATUS_NOT_SUPPORTED;

    if (Controller->Pci.DeviceID == PCI_DEV_PCI0640)
    {
        /*
         * The CMD-640 has one set of task registers per controller
         * and thus the two channels cannot be used simultaneously.
         * This also applies to PIO commands.
         * https://web.archive.org/web/20250324132451/https://www.mindprod.com/jgloss/eideflaw.html
         */
        Controller->Flags |= CTRL_FLAG_IS_SIMPLEX;

        Controller->Start = Cmd640ControllerStart;
    }

    Status = PciIdeCreateChannelData(Controller, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    return STATUS_SUCCESS;
}
