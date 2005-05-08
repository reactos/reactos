/*
 * win2k.h
 *
 * Definitions only used in Windows 2000 and earlier versions
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __WIN2K_H
#define __WIN2K_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "ntddk.h"

#pragma pack(push,4)

typedef enum _BUS_DATA_TYPE {
  ConfigurationSpaceUndefined = -1,
  Cmos,
  EisaConfiguration,
  Pos,
  CbusConfiguration,
  PCIConfiguration,
  VMEConfiguration,
  NuBusConfiguration,
  PCMCIAConfiguration,
  MPIConfiguration,
  MPSAConfiguration,
  PNPISAConfiguration,
  SgiInternalConfiguration,
  MaximumBusDataType
} BUS_DATA_TYPE, *PBUS_DATA_TYPE;

NTOSAPI
VOID
DDKAPI
ExReleaseResourceForThreadLite(
  IN PERESOURCE  Resource,
  IN ERESOURCE_THREAD  ResourceThreadId);

NTOSAPI
NTSTATUS
DDKAPI
IoReadPartitionTable(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  SectorSize,
  IN BOOLEAN  ReturnRecognizedPartitions,
  OUT struct _DRIVE_LAYOUT_INFORMATION  **PartitionBuffer);

NTOSAPI
NTSTATUS
DDKAPI
IoSetPartitionInformation(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  SectorSize,
  IN ULONG  PartitionNumber,
  IN ULONG  PartitionType);

NTOSAPI
NTSTATUS
DDKAPI
IoWritePartitionTable(
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  SectorSize,
  IN ULONG  SectorsPerTrack,
  IN ULONG  NumberOfHeads,
  IN struct _DRIVE_LAYOUT_INFORMATION  *PartitionBuffer);

/*
 * PVOID MmGetSystemAddressForMdl(
 *   IN PMDL  Mdl); 
 */
#define MmGetSystemAddressForMdl(Mdl) \
  (((Mdl)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | \
    MDL_SOURCE_IS_NONPAGED_POOL)) ? \
      ((Mdl)->MappedSystemVa) : \
      (MmMapLockedPages((Mdl), KernelMode)))

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* __WIN2K_H */
