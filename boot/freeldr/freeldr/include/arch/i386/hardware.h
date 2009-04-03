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

#define CONFIG_CMD(bus, dev_fn, where) \
	(0x80000000 | (((ULONG)(bus)) << 16) | (((dev_fn) & 0x1F) << 11) | (((dev_fn) & 0xE0) << 3) | ((where) & ~3))


//
// Static heap for ARC Hardware Component Tree
// 16KB oughta be enough for anyone.
//
#define HW_MAX_ARC_HEAP_SIZE 16 * 1024

//
// ARC Component Configuration Routines
//
VOID
NTAPI
FldrSetComponentInformation(
    IN PCONFIGURATION_COMPONENT_DATA ComponentKey,
    IN IDENTIFIER_FLAG Flags,
    IN ULONG Key,
    IN ULONG Affinity
);

VOID
NTAPI
FldrSetIdentifier(
    IN PCONFIGURATION_COMPONENT_DATA ComponentKey,
    IN PCHAR Identifier
);

VOID
NTAPI
FldrCreateSystemKey(
    OUT PCONFIGURATION_COMPONENT_DATA *SystemKey
);

VOID
NTAPI
FldrCreateComponentKey(
    IN PCONFIGURATION_COMPONENT_DATA SystemKey,
    IN PWCHAR BusName,
    IN ULONG BusNumber,
    IN CONFIGURATION_CLASS Class,
    IN CONFIGURATION_TYPE Type,
    OUT PCONFIGURATION_COMPONENT_DATA *ComponentKey
);

VOID
NTAPI
FldrSetConfigurationData(
    IN PCONFIGURATION_COMPONENT_DATA ComponentKey,
    IN PCM_PARTIAL_RESOURCE_LIST ResourceList,
    IN ULONG Size
);


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
