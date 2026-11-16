/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        hal/halx86/generic/usbhandoff.c
 * PURPOSE:     Release BIOS ownership of USB controllers
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * FreeLoader has already copied the kernel and the boot drivers into RAM.
 * The BIOS USB stack is no longer needed. Releasing it before the PIC and
 * the ACPI timer are programmed keeps a legacy SMI from resetting the machine.
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* UHCI legacy support is PCI_LEGSUP (0xC0) and PCI_LEGSUP_CLEAR_SMI (0x8F00)
 * in drivers/usb/usbuhci/hardware.h. USBCMD run bit 0x0001 and USBBASE at
 * 0x20 are in https://stuff.mit.edu/afs/sipb/contrib/doc/specs/protocol/usb/UHCI11D.PDF */
#define UHCI_USBLEGSUP              0xC0
#define UHCI_USBLEGSUP_RWC          0x8F00
#define UHCI_USBCMD_RUN             0x0001
#define UHCI_BAR_OFFSET             0x20

/* EHCI HCCPARAMS is at offset 8. EECP is bits 15:8. BIOS owned is bit 16
 * and OS owned is bit 24. Intel EHCI specification, USBLEGSUP and HCCPARAMS:
 * https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/ehci-specification-for-usb.pdf
 */
#define EHCI_HCC_PARAMS             8
#define EHCI_EECP_SHIFT             8
#define EHCI_EECP_MASK              0xFF
#define EHCI_USBLEGSUP_BIOS         (1UL << 16)
#define EHCI_USBLEGSUP_OS           (1UL << 24)
#define EHCI_USBLEGCTLSTS           4
#define EHCI_HANDOFF_SPIN           100000

#define USB_CLASS                   PCI_CLASS_SERIAL_BUS_CTLR
#define USB_SUBCLASS                0x03
#define USB_PROGIF_UHCI             0x00
#define USB_PROGIF_EHCI             0x20

/* FUNCTIONS *****************************************************************/

static
CODE_SEG("INIT")
VOID
HalpUsbConfig(
    _In_ ULONG Device,
    _In_ ULONG Function,
    _In_ ULONG Offset,
    _In_ ULONG Length,
    _Inout_ PVOID Buffer,
    _In_ BOOLEAN Write)
{
    PCI_SLOT_NUMBER Slot;

    Slot.u.AsULONG = 0;
    Slot.u.bits.DeviceNumber = Device;
    Slot.u.bits.FunctionNumber = Function;

    if (Write)
    {
        HalpPhase0SetPciDataByOffset(0, Slot, Buffer, Offset, Length);
    }
    else
    {
        HalpPhase0GetPciDataByOffset(0, Slot, Buffer, Offset, Length);
    }
}

static
CODE_SEG("INIT")
VOID
HalpUsbEnableDecode(
    _In_ ULONG Device,
    _In_ ULONG Function,
    _In_ USHORT DecodeBit)
{
    USHORT Command;

    HalpUsbConfig(Device,
                  Function,
                  FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                  sizeof(Command),
                  &Command,
                  FALSE);
    if (Command & DecodeBit)
        return;

    Command |= DecodeBit;
    HalpUsbConfig(Device,
                  Function,
                  FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                  sizeof(Command),
                  &Command,
                  TRUE);
}

static
CODE_SEG("INIT")
VOID
HalpUsbHandoffUhci(
    _In_ ULONG Device,
    _In_ ULONG Function)
{
    USHORT Legacy;
    ULONG Bar;
    USHORT UsbCmd;
    PUSHORT CommandPort;

    /*
     * Writing the write-1-to-clear status bits drops the SMI enables and
     * releases the BIOS semaphore. The reserved and read-only bits ignore it.
     */
    Legacy = UHCI_USBLEGSUP_RWC;
    HalpUsbConfig(Device,
                  Function,
                  UHCI_USBLEGSUP,
                  sizeof(Legacy),
                  &Legacy,
                  TRUE);

    HalpUsbConfig(Device,
                  Function,
                  UHCI_BAR_OFFSET,
                  sizeof(Bar),
                  &Bar,
                  FALSE);
    if (!(Bar & PCI_ADDRESS_IO_SPACE))
        return;

    CommandPort = UlongToPtr(Bar & PCI_ADDRESS_IO_ADDRESS_MASK);
    if (CommandPort == NULL)
        return;

    /* Stop the schedule. Leave the host-controller reset bit alone. */
    HalpUsbEnableDecode(Device, Function, PCI_ENABLE_IO_SPACE);
    UsbCmd = READ_PORT_USHORT(CommandPort);
    if (UsbCmd & UHCI_USBCMD_RUN)
    {
        UsbCmd &= ~UHCI_USBCMD_RUN;
        WRITE_PORT_USHORT(CommandPort, UsbCmd);
    }
}

static
CODE_SEG("INIT")
VOID
HalpUsbHandoffEhci(
    _In_ ULONG Device,
    _In_ ULONG Function)
{
    ULONG Bar;
    ULONG BarHigh;
    ULONG Capability;
    ULONG Legacy;
    ULONG Smi;
    ULONG Spin;
    UCHAR ExtendedCap;
    PHYSICAL_ADDRESS Physical;
    PVOID CapabilityRegs;

    HalpUsbConfig(Device,
                  Function,
                  FIELD_OFFSET(PCI_COMMON_HEADER, u.type0.BaseAddresses[0]),
                  sizeof(Bar),
                  &Bar,
                  FALSE);
    if (Bar & PCI_ADDRESS_IO_SPACE)
        return;

    Physical.LowPart = Bar & PCI_ADDRESS_MEMORY_ADDRESS_MASK;
    Physical.HighPart = 0;
    if ((Bar & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT)
    {
        HalpUsbConfig(Device,
                      Function,
                      FIELD_OFFSET(PCI_COMMON_HEADER, u.type0.BaseAddresses[1]),
                      sizeof(BarHigh),
                      &BarHigh,
                      FALSE);
        Physical.HighPart = BarHigh;
    }
    if (Physical.QuadPart == 0)
        return;

    HalpUsbEnableDecode(Device, Function, PCI_ENABLE_MEMORY_SPACE);
    CapabilityRegs = HalpMapPhysicalMemory64(Physical, 1);
    if (CapabilityRegs == NULL)
        return;

    Capability = READ_REGISTER_ULONG((PULONG)((PUCHAR)CapabilityRegs +
                                              EHCI_HCC_PARAMS));
    HalpUnmapVirtualAddress(CapabilityRegs, 1);

    ExtendedCap = (UCHAR)((Capability >> EHCI_EECP_SHIFT) & EHCI_EECP_MASK);
    if (ExtendedCap < 0x40)
        return;

    HalpUsbConfig(Device, Function, ExtendedCap, sizeof(Legacy), &Legacy, FALSE);
    Legacy |= EHCI_USBLEGSUP_OS;
    HalpUsbConfig(Device, Function, ExtendedCap, sizeof(Legacy), &Legacy, TRUE);

    /*
     * Stall calibration has not run, and interrupts are still off.
     * A bounded read of the same register is the wait.
     */
    for (Spin = 0; Spin < EHCI_HANDOFF_SPIN; Spin++)
    {
        HalpUsbConfig(Device,
                      Function,
                      ExtendedCap,
                      sizeof(Legacy),
                      &Legacy,
                      FALSE);
        if ((Legacy & EHCI_USBLEGSUP_BIOS) == 0)
            break;
    }

    Smi = 0;
    HalpUsbConfig(Device,
                  Function,
                  ExtendedCap + EHCI_USBLEGCTLSTS,
                  sizeof(Smi),
                  &Smi,
                  TRUE);
}

CODE_SEG("INIT")
VOID
NTAPI
HalpReleaseUsbLegacyOwnership(VOID)
{
    ULONG Device;
    ULONG Function;
    ULONG FunctionCount;
    USHORT Vendor;
    UCHAR HeaderType;
    ULONG ClassRev;
    UCHAR BaseClass;
    UCHAR SubClass;
    UCHAR ProgIf;

    for (Device = 0; Device < PCI_MAX_DEVICES; Device++)
    {
        HalpUsbConfig(Device,
                      0,
                      FIELD_OFFSET(PCI_COMMON_HEADER, VendorID),
                      sizeof(Vendor),
                      &Vendor,
                      FALSE);
        if (Vendor == PCI_INVALID_VENDORID)
            continue;

        HalpUsbConfig(Device,
                      0,
                      FIELD_OFFSET(PCI_COMMON_HEADER, HeaderType),
                      sizeof(HeaderType),
                      &HeaderType,
                      FALSE);
        FunctionCount = (HeaderType & PCI_MULTIFUNCTION) ? PCI_MAX_FUNCTION : 1;

        for (Function = 0; Function < FunctionCount; Function++)
        {
            if (Function != 0)
            {
                HalpUsbConfig(Device,
                              Function,
                              FIELD_OFFSET(PCI_COMMON_HEADER, VendorID),
                              sizeof(Vendor),
                              &Vendor,
                              FALSE);
                if (Vendor == PCI_INVALID_VENDORID)
                    continue;
            }

            HalpUsbConfig(Device,
                          Function,
                          FIELD_OFFSET(PCI_COMMON_HEADER, RevisionID),
                          sizeof(ClassRev),
                          &ClassRev,
                          FALSE);
            BaseClass = (UCHAR)(ClassRev >> 24);
            SubClass = (UCHAR)(ClassRev >> 16);
            ProgIf = (UCHAR)(ClassRev >> 8);
            if (BaseClass != USB_CLASS || SubClass != USB_SUBCLASS)
                continue;

            if (ProgIf == USB_PROGIF_UHCI)
                HalpUsbHandoffUhci(Device, Function);
            else if (ProgIf == USB_PROGIF_EHCI)
                HalpUsbHandoffEhci(Device, Function);
        }
    }
}
