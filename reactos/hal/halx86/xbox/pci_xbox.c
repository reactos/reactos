/* $Id: pci_xbox.c,v 1.1 2004/12/04 22:52:59 gvg Exp $
 *
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          hal/halx86/xbox/pci_xbox.c
 * PURPOSE:       Xbox specific handling of PCI cards
 * PROGRAMMER:    Ge van Geldorp (gvg@reactos.com)
 * UPDATE HISTORY:
 *             2004/12/04: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <hal.h>
#include <bus.h>
#include "halxbox.h"

#define NDEBUG
#include <internal/debug.h>

/* VARIABLES ***************************************************************/

static ULONG (* STDCALL GenericGetPciData)(PBUS_HANDLER BusHandler,
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

  if (0 == BusNumber && (1 == ((SlotNumber >> 5) & 0x07) || 2 == ((SlotNumber >> 5) & 0x07)))
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

void
HalpXboxInitPciBus(ULONG BusNumber, PBUS_HANDLER BusHandler)
{
  if (0 == BusNumber)
    {
      GenericGetPciData = BusHandler->GetBusData;
      BusHandler->GetBusData = HalpXboxGetPciData;
    }
}

/* EOF */
