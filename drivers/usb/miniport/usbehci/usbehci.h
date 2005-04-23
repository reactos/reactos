/*
 * ReactOS USB EHCI miniport driver
 *
 * Copyright (C) 2004 Mark Tempel
 *			 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef USBEHCI_H
#define USBEHCI_H

/* INCLUDES *******************************************************************/

#include "stddef.h" 
#include "windef.h"
//#include <ddk/miniport.h>
#include <ddk/ntapi.h>

#ifdef DBG
#define DPRINT(arg) DbgPrint arg;
#else
#define DPRINT(arg)
#endif

// Export funcs here
/*
BOOL FASTCALL
VBESetColorRegisters(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_CLUT ColorLookUpTable,
   PSTATUS_BLOCK StatusBlock);
*/
#endif /* USBEHCI_H */
