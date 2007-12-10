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

#define NDEBUG
#include <debug.h>

VOID
XboxHwDetect(VOID)
{
  PCONFIGURATION_COMPONENT_DATA SystemKey;

  DbgPrint((DPRINT_HWDETECT, "DetectHardware()\n"));

  /* Create the 'System' key */
  FldrCreateSystemKey(&SystemKey);

  /* Set empty component information */
  FldrSetComponentInformation(SystemKey,
                              0x0,
                              0x0,
                              0xFFFFFFFF);

  /* TODO: Build actual xbox's hardware configuration tree */

  DbgPrint((DPRINT_HWDETECT, "DetectHardware() Done\n"));
}

/* EOF */
