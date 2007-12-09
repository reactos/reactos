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
// ARC Component Configuration Routines
//
VOID
NTAPI
FldrSetComponentInformation(
    IN FRLDRHKEY ComponentKey,
    IN IDENTIFIER_FLAG Flags,
    IN ULONG Key,
    IN ULONG Affinity
);

VOID
NTAPI
FldrSetIdentifier(
    IN FRLDRHKEY ComponentKey,
    IN PWCHAR Identifier
);

VOID
NTAPI
FldrCreateSystemKey(
    OUT FRLDRHKEY *SystemKey
);

VOID
NTAPI
FldrCreateComponentKey(
    IN FRLDRHKEY SystemKey,
    IN PWCHAR BusName,
    IN ULONG BusNumber,
    IN CONFIGURATION_CLASS Class,
    IN CONFIGURATION_TYPE Type,
    OUT FRLDRHKEY *ComponentKey
);

VOID
NTAPI
FldrSetConfigurationData(
    IN FRLDRHKEY ComponentKey,
    IN PVOID ConfigurationData,
    IN ULONG Size
);


/* PROTOTYPES ***************************************************************/

/* hardware.c */

VOID StallExecutionProcessor(ULONG Microseconds);

VOID HalpCalibrateStallExecution(VOID);

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
