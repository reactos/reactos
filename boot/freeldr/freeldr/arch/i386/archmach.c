/* $Id$
 *
 *  FreeLoader
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>

VOID
MachInit(const char *CmdLine)
{
  ULONG PciId;

  memset(&MachVtbl, 0, sizeof(MACHVTBL));

  /* Check for Xbox by identifying device at PCI 0:0:0, if it's
   * 0x10de/0x02a5 then we're running on an Xbox */
  WRITE_PORT_ULONG((ULONG*) 0xcf8, CONFIG_CMD(0, 0, 0));
  PciId = READ_PORT_ULONG((ULONG*) 0xcfc);
  if (0x02a510de == PciId)
    {
      XboxMachInit(CmdLine);
    }
  else
    {
      PcMachInit(CmdLine);
    }

  HalpCalibrateStallExecution();
}

/* EOF */
