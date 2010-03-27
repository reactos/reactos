/*
 * ntagp.h
 *
 * NT AGP bus driver interface
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Gregor Anich <blight@blight.eu.org>
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

#ifndef __NTAGP_H
#define __NTAGP_H

#include "video.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AGP_BUS_INTERFACE_V1              1L
#define AGP_BUS_INTERFACE_V2              2L

/* Indicates wether the GART supports mapping of physical memory for the CPU */
#define AGP_CAPABILITIES_MAP_PHYSICAL     0x00000001L

typedef NTSTATUS
(DDKAPI *PAGP_BUS_COMMIT_MEMORY)(
  IN     PVOID  AgpContext,
  IN     PVOID  MapHandle,
  IN     ULONG  NumberOfPages,
  IN     ULONG  OffsetInPages,
  IN OUT PMDL  Mdl  OPTIONAL,
  OUT    PHYSICAL_ADDRESS  *MemoryBase);

typedef NTSTATUS
(DDKAPI *PAGP_BUS_FREE_MEMORY)(
  IN  PVOID  AgpContext,
  IN  PVOID  MapHandle,
  IN  ULONG  NumberOfPages,
  IN  ULONG  OffsetInPages);

typedef NTSTATUS
(DDKAPI *PAGP_BUS_RELEASE_MEMORY)(
  IN  PVOID  AgpContext,
  IN  PVOID  MapHandle);

typedef NTSTATUS
(DDKAPI *PAGP_BUS_RESERVE_MEMORY)(
  IN  PVOID  AgpContext,
  IN  ULONG  NumberOfPages,
  IN  MEMORY_CACHING_TYPE  MemoryType,
  OUT PVOID  *MapHandle,
  OUT PHYSICAL_ADDRESS  *PhysicalAddress  OPTIONAL);

typedef NTSTATUS
(DDKAPI *PAGP_BUS_SET_RATE)(
  IN  PVOID  AgpContext,
  IN  ULONG  AgpRate);

typedef NTSTATUS
(DDKAPI *PAGP_GET_MAPPED_PAGES)(
  IN  PVOID  AgpContext,
  IN  PVOID  MapHandle,
  IN  ULONG  NumberOfPages,
  IN  ULONG  OffsetInPages,
  OUT PMDL  Mdl);

typedef struct _AGP_BUS_INTERFACE_STANDARD {
  USHORT  Size;
  USHORT  Version;
  PVOID  AgpContext;
  PINTERFACE_REFERENCE  InterfaceReference;
  PINTERFACE_DEREFERENCE  InterfaceDereference;

  ULONG  Capabilities;
  PAGP_BUS_RESERVE_MEMORY  ReserveMemory;
  PAGP_BUS_RELEASE_MEMORY  ReleaseMemory;
  PAGP_BUS_COMMIT_MEMORY  CommitMemory;
  PAGP_BUS_FREE_MEMORY  FreeMemory;
  PAGP_GET_MAPPED_PAGES  GetMappedPages;
  PAGP_BUS_SET_RATE  SetRate;
} AGP_BUS_INTERFACE_STANDARD, *PAGP_BUS_INTERFACE_STANDARD;

#define AGP_BUS_INTERFACE_V2_SIZE sizeof(AGP_BUS_INTERFACE_STANDARD)
#define AGP_BUS_INTERFACE_V1_SIZE \
             (AGP_BUS_INTERFACE_V2_SIZE - sizeof(PAGP_BUS_SET_RATE))

#ifdef __cplusplus
}
#endif

#endif /* __NTAGP_H */

