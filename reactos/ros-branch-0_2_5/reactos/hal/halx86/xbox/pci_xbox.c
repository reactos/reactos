/* $Id: pci_xbox.c,v 1.3 2004/12/11 14:45:00 gvg Exp $
 *
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

#include <ddk/ntddk.h>
#include <hal.h>
#include <bus.h>
#include "halxbox.h"

#define NDEBUG
#include <internal/debug.h>

/* VARIABLES ***************************************************************/

static ULONG (STDCALL *GenericGetPciData)(PBUS_HANDLER BusHandler,
                                          ULONG BusNumber,
                                          ULONG SlotNumber,
                                          PVOID Buffer,
                                          ULONG Offset,
                                          ULONG Length);
static ULONG (STDCALL *GenericSetPciData)(PBUS_HANDLER BusHandler,
                                          ULONG BusNumber,
                                          ULONG SlotNumber,
                                          PVOID Buffer,
                                          ULONG Offset,
                                          ULONG Length);

/* FUNCTIONS ***************************************************************/

static ULONG STDCALL
HalpXboxGetPciData(PBUS_HANDLER BusHandler,
                   ULONG BusNumber,
                   ULONG SlotNumber,
                   PVOID Buffer,
                   ULONG Offset,
                   ULONG Length)
{
  DPRINT("HalpXboxGetPciData() called.\n");
  DPRINT("  BusNumber %lu\n", BusNumber);
  DPRINT("  SlotNumber %lu\n", SlotNumber);
  DPRINT("  Offset 0x%lx\n", Offset);
  DPRINT("  Length 0x%lx\n", Length);

  if ((0 == BusNumber && 0 == (SlotNumber & 0x1f) &&
       (1 == ((SlotNumber >> 5) & 0x07) || 2 == ((SlotNumber >> 5) & 0x07))) ||
      (1 == BusNumber && 0 != (SlotNumber & 0x1f)))
    {
      DPRINT("Blacklisted PCI slot\n");
      if (0 == Offset && 2 <= Length)
        {
          *(PUSHORT)Buffer = PCI_INVALID_VENDORID;
          return 2;
        }
      return 0;
    }

  return GenericGetPciData(BusHandler, BusNumber, SlotNumber, Buffer, Offset, Length);
}

static ULONG STDCALL
HalpXboxSetPciData(PBUS_HANDLER BusHandler,
                   ULONG BusNumber,
                   ULONG SlotNumber,
                   PVOID Buffer,
                   ULONG Offset,
                   ULONG Length)
{
  DPRINT("HalpXboxSetPciData() called.\n");
  DPRINT("  BusNumber %lu\n", BusNumber);
  DPRINT("  SlotNumber %lu\n", SlotNumber);
  DPRINT("  Offset 0x%lx\n", Offset);
  DPRINT("  Length 0x%lx\n", Length);

  if ((0 == BusNumber && 0 == (SlotNumber & 0x1f) &&
       (1 == ((SlotNumber >> 5) & 0x07) || 2 == ((SlotNumber >> 5) & 0x07))) ||
      (1 == BusNumber && 0 != (SlotNumber & 0x1f)))
    {
      DPRINT1("Trying to set data on blacklisted PCI slot\n");
      return 0;
    }

  return GenericSetPciData(BusHandler, BusNumber, SlotNumber, Buffer, Offset, Length);
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
