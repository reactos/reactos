/* $Id: machxbox.h,v 1.1 2004/11/08 22:02:47 gvg Exp $
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

#ifndef __I386_MACHXBOX_H_
#define __I386_MACHXBOX_H_

#ifndef __MEMORY_H
#include "mm.h"
#endif

U8 XboxFont8x16[256 * 16];

VOID XboxMachInit(VOID);

VOID XboxVideoInit(VOID);
VOID XboxVideoClearScreenAttr(U8 Attr);
VOID XboxVideoPutChar(int Ch);
VOID XboxVideoPutCharAttrAtLoc(int Ch, U8 Attr, unsigned X, unsigned Y);

VOID XboxMemInit(VOID);
PVOID XboxMemReserveMemory(U32 MbToReserve);
U32 XboxMemGetMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, U32 MaxMemoryMapSize);

#endif /* __I386_HWXBOX_H_ */

/* EOF */
