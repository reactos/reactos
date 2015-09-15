/*
 *  FreeLoader
 *
 *  Copyright (C) 2003  Eric Kohl
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#define CONFIG_CMD(bus, dev_fn, where) \
    (0x80000000 | (((ULONG)(bus)) << 16) | (((dev_fn) & 0x1F) << 11) | (((dev_fn) & 0xE0) << 3) | ((where) & ~3))

#define TAG_HW_RESOURCE_LIST 'lRwH'
#define TAG_HW_DISK_CONTEXT 'cDwH'

/* PROTOTYPES ***************************************************************/

/* hardware.c */

VOID StallExecutionProcessor(ULONG Microseconds);

VOID HalpCalibrateStallExecution(VOID);

/* hwacpi.c */
VOID DetectAcpiBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber);

/* hwapm.c */
VOID DetectApmBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber);

/* hwpci.c */
VOID DetectPciBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber);

/* i386pnp.S */
ULONG_PTR __cdecl PnpBiosSupported(VOID);
ULONG __cdecl PnpBiosGetDeviceNodeCount(ULONG *NodeSize,
                  ULONG *NodeCount);
ULONG __cdecl PnpBiosGetDeviceNode(UCHAR *NodeId,
             UCHAR *NodeBuffer);

/* i386pxe.S */
USHORT __cdecl PxeCallApi(USHORT Segment, USHORT Offset, USHORT Service, VOID* Parameter);

/* EOF */
