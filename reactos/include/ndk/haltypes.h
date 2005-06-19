/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/haltypes.h
 * PURPOSE:         Definitions for HAL/BLDR types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _HALTYPES_H
#define _HALTYPES_H

/* DEPENDENCIES **************************************************************/
//#include <ddk/ntdddisk.h>

/* EXPORTED DATA *************************************************************/
extern ULONG NTOSAPI KdComPortInUse;

/* CONSTANTS *****************************************************************/

/* Boot Flags */
#define MB_FLAGS_MEM_INFO         (0x1)
#define MB_FLAGS_BOOT_DEVICE      (0x2)
#define MB_FLAGS_COMMAND_LINE     (0x4)
#define MB_FLAGS_MODULE_INFO      (0x8)
#define MB_FLAGS_AOUT_SYMS        (0x10)
#define MB_FLAGS_ELF_SYMS         (0x20)
#define MB_FLAGS_MMAP_INFO        (0x40)
#define MB_FLAGS_DRIVES_INFO      (0x80)
#define MB_FLAGS_CONFIG_TABLE     (0x100)
#define MB_FLAGS_BOOT_LOADER_NAME (0x200)
#define MB_FLAGS_APM_TABLE        (0x400)
#define MB_FLAGS_GRAPHICS_TABLE   (0x800)

/* ENUMERATIONS **************************************************************/
typedef enum _FIRMWARE_ENTRY
{
  HalHaltRoutine,
  HalPowerDownRoutine,
  HalRestartRoutine,
  HalRebootRoutine,
  HalInteractiveModeRoutine,
  HalMaximumRoutine
} FIRMWARE_REENTRY, *PFIRMWARE_REENTRY;

/* TYPES *********************************************************************/

typedef struct _LOADER_MODULE 
{
   ULONG ModStart;
   ULONG ModEnd;
   ULONG String;
   ULONG Reserved;
} LOADER_MODULE, *PLOADER_MODULE;

typedef struct _LOADER_PARAMETER_BLOCK
{
   ULONG Flags;
   ULONG MemLower;
   ULONG MemHigher;
   ULONG BootDevice;
   ULONG CommandLine;
   ULONG ModsCount;
   ULONG ModsAddr;
   UCHAR Syms[12];
   ULONG MmapLength;
   ULONG MmapAddr;
   ULONG DrivesCount;
   ULONG DrivesAddr;
   ULONG ConfigTable;
   ULONG BootLoaderName;
   ULONG PageDirectoryStart;
   ULONG PageDirectoryEnd;
   ULONG KernelBase;
} LOADER_PARAMETER_BLOCK, *PLOADER_PARAMETER_BLOCK;

#endif

