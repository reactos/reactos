/*
 * PROJECT:     ReactOS Kernel-Mode Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     VM detection code
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include <kmt_test.h>

/* FUNCTIONS ******************************************************************/

#if defined(_M_IX86) || defined(_M_AMD64)
extern BOOLEAN VmIsVMware(VOID);

static
BOOLEAN
VmIsHypervisorPresent(VOID)
{
    INT CpuInfo[4];

    __cpuid(CpuInfo, 1);

    return !!(CpuInfo[2] & 0x80000000);
}

static
BOOLEAN
VmIsVbox(VOID)
{
    ULONG PciId, BytesRead;
    PCI_SLOT_NUMBER Slot;

    Slot.u.AsULONG = 0;
    Slot.u.bits.DeviceNumber = 4;
    Slot.u.bits.FunctionNumber = 0;

    BytesRead = HalGetBusDataByOffset(PCIConfiguration,
                                      0,
                                      Slot.u.AsULONG,
                                      &PciId,
                                      FIELD_OFFSET(PCI_COMMON_HEADER, VendorID),
                                      sizeof(PciId));
    return (BytesRead == sizeof(PciId)) && (PciId == 0xCAFE80EE);
}
#endif // defined(_M_IX86) || defined(_M_AMD64)

/*
 * NOTE: We make no attempt to support every software in existence.
 * Only VMs used by Testman are checked.
 */
BOOLEAN
KmtDetectVirtualMachine(VOID)
{
#if defined(_M_IX86) || defined(_M_AMD64)
    return VmIsHypervisorPresent() || VmIsVbox() || VmIsVMware();
#else
    return FALSE;
#endif
}
