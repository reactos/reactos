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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __I386_HARDWARE_H_
#define __I386_HARDWARE_H_

#ifndef __REGISTRY_H
#include "../../reactos/registry.h"
#endif

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.Flags */
#define CM_RESOURCE_PORT_MEMORY               0x0000
#define CM_RESOURCE_PORT_IO                   0x0001

#define CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE 0x0000
#define CM_RESOURCE_INTERRUPT_LATCHED         0x0001

typedef struct _CM_COMPONENT_INFORMATION
{
  ULONG Flags;
  ULONG Version;
  ULONG Key;
  ULONG Affinity;
} __attribute__((packed)) CM_COMPONENT_INFORMATION, *PCM_COMPONENT_INFORMATION;


/* CM_COMPONENT_INFORMATION.Flags */
#define Failed      0x00000001
//#define ReadOnly    0x00000002
#define Removable   0x00000004
#define ConsoleIn   0x00000008
#define ConsoleOut  0x00000010
#define Input       0x00000020
#define Output      0x00000040

#define CONFIG_CMD(bus, dev_fn, where) \
	(0x80000000 | (((ULONG)(bus)) << 16) | (((dev_fn) & 0x1F) << 11) | (((dev_fn) & 0xE0) << 3) | ((where) & ~3))

/* PROTOTYPES ***************************************************************/

/* hardware.c */

VOID StallExecutionProcessor(ULONG Microseconds);

VOID HalpCalibrateStallExecution(VOID);

VOID SetComponentInformation(FRLDRHKEY ComponentKey,
			     ULONG Flags,
			     ULONG Key,
			     ULONG Affinity);

/* hwacpi.c */
VOID DetectAcpiBios(FRLDRHKEY SystemKey, ULONG *BusNumber);

/* hwapm.c */
VOID DetectApmBios(FRLDRHKEY SystemKey, ULONG *BusNumber);

/* hwcpu.c */
VOID DetectCPUs(FRLDRHKEY SystemKey);

/* hwpci.c */
VOID DetectPciBios(FRLDRHKEY SystemKey, ULONG *BusNumber);

/* i386cpu.S */
ULONG CpuidSupported(VOID);
VOID GetCpuid(ULONG Level,
	      ULONG *eax,
	      ULONG *ebx,
	      ULONG *ecx,
	      ULONG *edx);
ULONGLONG RDTSC(VOID);

/* i386pnp.S */
ULONG PnpBiosSupported(VOID);
ULONG PnpBiosGetDeviceNodeCount(ULONG *NodeSize,
			      ULONG *NodeCount);
ULONG PnpBiosGetDeviceNode(UCHAR *NodeId,
			 UCHAR *NodeBuffer);

#endif /* __I386_HARDWARE_H_ */

/* EOF */
