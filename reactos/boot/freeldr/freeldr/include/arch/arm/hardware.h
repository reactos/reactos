/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/include/arch/arm/hardware.h
 * PURPOSE:         Header for ARC definitions (to be cleaned up)
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#ifndef _ARM_HARDWARE_
#define __ARM_HARDWARE_

#ifndef __REGISTRY_H
#include "../../reactos/registry.h"
#endif

#include "../../../../../armllb/inc/osloader.h"
#include "../../../../../armllb/inc/machtype.h"

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
FldrCreateSystemKey(
    OUT PCONFIGURATION_COMPONENT_DATA *SystemKey
);

VOID
NTAPI
FldrCreateComponentKey(
    IN PCONFIGURATION_COMPONENT_DATA SystemKey,
    IN CONFIGURATION_CLASS Class,
    IN CONFIGURATION_TYPE Type,
    IN IDENTIFIER_FLAG Flags,
    IN ULONG Key,
    IN ULONG Affinity,
    IN PCHAR IdentifierString,
    IN PCM_PARTIAL_RESOURCE_LIST ResourceList,
    IN ULONG Size,
    OUT PCONFIGURATION_COMPONENT_DATA *ComponentKey
);

extern PARM_BOARD_CONFIGURATION_BLOCK ArmBoardBlock;

#endif
