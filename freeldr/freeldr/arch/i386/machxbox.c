/* $Id: machxbox.c,v 1.1 2004/11/08 22:02:47 gvg Exp $
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

#include "freeldr.h"
#include "mm.h"
#include "machine.h"
#include "machxbox.h"

VOID
XboxMachInit(VOID)
{
  /* Initialize our stuff */
  XboxMemInit();
  XboxVideoInit();

  /* Setup vtbl */
  MachVtbl.ClearScreenAttr = XboxVideoClearScreenAttr;
  MachVtbl.PutChar = XboxVideoPutChar;
  MachVtbl.PutCharAttrAtLoc = XboxVideoPutCharAttrAtLoc;
  MachVtbl.GetMemoryMap = XboxMemGetMemoryMap;
}
