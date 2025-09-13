/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     AMD PCI IDE controller minidriver
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* GLOBALS ********************************************************************/

#define PCI_DEV_AMD_756                     0x7409
#define PCI_DEV_AMD_766                     0x7411
#define PCI_DEV_AMD_768                     0x7441
#define PCI_DEV_AMD_8111                    0x7469

#define AMD_CONFIG_OFFSET     0x40
#define NV_CONFIG_OFFSET      0x50

/* FUNCTIONS ******************************************************************/

CODE_SEG("PAGE")
NTSTATUS
AmdGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    PAGED_CODE();
    ASSERT(Controller->Pci.VendorID == PCI_VEN_AMD || Controller->Pci.VendorID == PCI_VEN_NVIDIA);

    /* TODO */
    return STATUS_NOT_IMPLEMENTED;
}
