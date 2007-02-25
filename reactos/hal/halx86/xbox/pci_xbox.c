/*
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          hal/halx86/xbox/pci_xbox.c
 * PURPOSE:       Xbox specific handling of PCI cards
 * PROGRAMMER:    Ge van Geldorp (gvg@reactos.com)
 * UPDATE HISTORY:
 *             2004/12/04: Created
 *
 * Trying to get PCI config data from devices 0:0:1 and 0:0:2 will completely
 * hang the Xbox. Also, the device number doesn't seem to be decoded for the
 * video card, so it appears to be present on 1:0:0 - 1:31:0.
 * We hack around these problems by indicating "device not present" for devices
 * 0:0:1, 0:0:2, 1:1:0, 1:2:0, 1:3:0, ...., 1:31:0
 */

/* INCLUDES *****************************************************************/

#include <halxbox.h>

#define NDEBUG
#include <debug.h>

/* VARIABLES ***************************************************************/

static ULONG (NTAPI *GenericGetPciData)(IN PBUS_HANDLER BusHandler,
                                        IN PBUS_HANDLER RootHandler,
                                        IN PCI_SLOT_NUMBER SlotNumber,
                                        OUT PUCHAR Buffer,
                                        IN ULONG Offset,
                                        IN ULONG Length);
static ULONG (NTAPI *GenericSetPciData)(IN PBUS_HANDLER BusHandler,
                                        IN PBUS_HANDLER RootHandler,
                                        IN PCI_SLOT_NUMBER SlotNumber,
                                        IN PUCHAR Buffer,
                                        IN ULONG Offset,
                                        IN ULONG Length);

/* FUNCTIONS ***************************************************************/

static ULONG NTAPI
HalpXboxGetPciData(IN PBUS_HANDLER BusHandler,
                   IN PBUS_HANDLER RootHandler,
                   IN PCI_SLOT_NUMBER SlotNumber,
                   OUT PUCHAR Buffer,
                   IN ULONG Offset,
                   IN ULONG Length)
{
  ULONG BusNumber = BusHandler->BusNumber;

  DPRINT("HalpXboxGetPciData() called.\n");
  DPRINT("  BusNumber %lu\n", BusNumber);
  DPRINT("  SlotNumber %lu\n", SlotNumber);
  DPRINT("  Offset 0x%lx\n", Offset);
  DPRINT("  Length 0x%lx\n", Length);

  if ((0 == BusNumber && 0 == SlotNumber.u.bits.DeviceNumber &&
       (1 == SlotNumber.u.bits.FunctionNumber || 2 == SlotNumber.u.bits.FunctionNumber)) ||
      (1 == BusNumber && 0 != SlotNumber.u.bits.DeviceNumber))
    {
      DPRINT("Blacklisted PCI slot\n");
      if (0 == Offset && 2 <= Length)
        {
          *(PUSHORT)Buffer = PCI_INVALID_VENDORID;
          return 2;
        }
      return 0;
    }

  return GenericGetPciData(BusHandler, RootHandler, SlotNumber, Buffer, Offset, Length);
}

static ULONG NTAPI
HalpXboxSetPciData(IN PBUS_HANDLER BusHandler,
                   IN PBUS_HANDLER RootHandler,
                   IN PCI_SLOT_NUMBER SlotNumber,
                   IN PUCHAR Buffer,
                   IN ULONG Offset,
                   IN ULONG Length)
{
  ULONG BusNumber = BusHandler->BusNumber;

  DPRINT("HalpXboxSetPciData() called.\n");
  DPRINT("  BusNumber %lu\n", BusNumber);
  DPRINT("  SlotNumber %lu\n", SlotNumber);
  DPRINT("  Offset 0x%lx\n", Offset);
  DPRINT("  Length 0x%lx\n", Length);

  if ((0 == BusNumber && 0 == SlotNumber.u.bits.DeviceNumber &&
       (1 == SlotNumber.u.bits.FunctionNumber || 2 == SlotNumber.u.bits.FunctionNumber)) ||
      (1 == BusNumber && 0 != SlotNumber.u.bits.DeviceNumber))
    {
      DPRINT1("Trying to set data on blacklisted PCI slot\n");
      return 0;
    }

  return GenericSetPciData(BusHandler, RootHandler, SlotNumber, Buffer, Offset, Length);
}

void
HalpXboxInitPciBus(ULONG BusNumber, PBUS_HANDLER BusHandler)
{
  if (0 == BusNumber || 1 == BusNumber)
    {
      GenericGetPciData = BusHandler->GetBusData;
      BusHandler->GetBusData = HalpXboxGetPciData;
      GenericSetPciData = BusHandler->SetBusData;
      BusHandler->SetBusData = HalpXboxSetPciData;
    }
}

/* EOF */
