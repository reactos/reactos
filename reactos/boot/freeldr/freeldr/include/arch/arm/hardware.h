/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/freeldr/include/arch/arm/hardware.h
 * PURPOSE:         Header for ARC definitions (to be cleaned up)
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#pragma once

#ifndef __REGISTRY_H
//#include "../../reactos/registry.h"
#endif

#include "../../../../../armllb/inc/osloader.h"
#include "../../../../../armllb/inc/machtype.h"

#define FREELDR_BASE       0x0001F000
#define FREELDR_PE_BASE    0x0001F000
#define MAX_FREELDR_PE_SIZE 0xFFFFFF

extern PARM_BOARD_CONFIGURATION_BLOCK ArmBoardBlock;
extern ULONG FirstLevelDcacheSize;
extern ULONG FirstLevelDcacheFillSize;
extern ULONG FirstLevelIcacheSize;
extern ULONG FirstLevelIcacheFillSize;
extern ULONG SecondLevelDcacheSize;
extern ULONG SecondLevelDcacheFillSize;
extern ULONG SecondLevelIcacheSize;
extern ULONG SecondLevelIcacheFillSize;

extern ULONG gDiskReadBuffer, gFileSysBuffer;
#define DiskReadBuffer ((PVOID)gDiskReadBuffer)

#define DriveMapGetBiosDriveNumber(DeviceName) 0

FORCEINLINE VOID Reboot(VOID)
{
    DbgBreakPoint();
}

typedef struct _PAGE_TABLE_ARM
{
    HARDWARE_PTE_ARMV6 Pte[1024];
} PAGE_TABLE_ARM, *PPAGE_TABLE_ARM;
C_ASSERT(sizeof(PAGE_TABLE_ARM) == PAGE_SIZE);

typedef struct _PAGE_DIRECTORY_ARM
{
    union
    {
        HARDWARE_PDE_ARMV6 Pde[4096];
        HARDWARE_LARGE_PTE_ARMV6 Pte[4096];
    };
} PAGE_DIRECTORY_ARM, *PPAGE_DIRECTORY_ARM;
C_ASSERT(sizeof(PAGE_DIRECTORY_ARM) == (4 * PAGE_SIZE));

// FIXME: sync with NDK
typedef enum _ARM_DOMAIN
{
    FaultDomain,
    ClientDomain,
    InvalidDomain,
    ManagerDomain
} ARM_DOMAIN;

#define PDE_SHIFT 20
