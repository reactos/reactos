/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PC-Tech RZ1000 PCI IDE controller minidriver
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* GLOBALS ********************************************************************/

#define PCI_DEV_RZ1000      0x1000
#define PCI_DEV_RZ1001      0x1001

#define REG_CONFIG          0x40
#define CONFIG_PREFETCH     0x2000

/* FUNCTIONS ******************************************************************/

/*
 * Errata: Disable the Read-Ahead Mode.
 * https://web.archive.org/web/19961222024423/http://www.intel.com/procs/support/rz1000/rztech.htm
 * https://web.archive.org/web/20250324132451/https://www.mindprod.com/jgloss/eideflaw.html
 */
static
VOID
PcTechControllerStart(
    _In_ PATA_CONTROLLER Controller)
{
    USHORT ConfigReg;

    ConfigReg = PciRead16(Controller, REG_CONFIG);
    ConfigReg &= ~CONFIG_PREFETCH;
    PciWrite16(Controller, REG_CONFIG, ConfigReg);
}

CODE_SEG("PAGE")
NTSTATUS
PcTechGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    NTSTATUS Status;

    PAGED_CODE();
    ASSERT(Controller->Pci.VendorID == PCI_VEN_PC_TECH);

    if (Controller->Pci.DeviceID != PCI_DEV_RZ1000 && Controller->Pci.DeviceID != PCI_DEV_RZ1001)
        return STATUS_NO_MATCH;

    Status = PciIdeCreateChannelData(Controller, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    Controller->Start = PcTechControllerStart;

    return STATUS_SUCCESS;
}
