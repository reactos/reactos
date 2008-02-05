/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/include/arch/arm/hardware.h
 * PURPOSE:         Implements routines to support booting from a RAM Disk
 * PROGRAMMERS:     alex@winsiderss.com
 */

#ifndef _ARM_HARDWARE_
#define __ARM_HARDWARE_

#ifndef __REGISTRY_H
#include "../../reactos/registry.h"
#endif

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

#endif
