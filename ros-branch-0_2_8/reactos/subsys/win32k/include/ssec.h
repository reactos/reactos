/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          shared sections
 * FILE:             include/ssec.h
 * PROGRAMMER:       Thomas Weidenmueller <w3seek@reactos.com>
 *
 */

#ifndef _WIN32K_SSEC_H
#define _WIN32K_SSEC_H

typedef struct _SHARED_SECTION
{
  PSECTION_OBJECT SectionObject;
  PVOID SystemMappedBase;
  ULONG ViewSize;
} SHARED_SECTION, *PSHARED_SECTION;

typedef struct _SHARED_SECTIONS_ARRAY
{
  struct _SHARED_SECTIONS_ARRAY *Next;
  ULONG nEntries;
  SHARED_SECTION SharedSection[0];
} SHARED_SECTIONS_ARRAY, *PSHARED_SECTIONS_ARRAY;

typedef struct _SHARED_SECTION_POOL
{
  FAST_MUTEX Lock;
  ULONG PoolSize;
  ULONG PoolFree;
  ULONG SharedSectionCount;
  SHARED_SECTIONS_ARRAY SectionsArray;
} SHARED_SECTION_POOL, *PSHARED_SECTION_POOL;

NTSTATUS INTERNAL_CALL
IntUserCreateSharedSectionPool(IN ULONG MaximumPoolSize,
                               IN PSHARED_SECTION_POOL *SharedSectionPool);

VOID INTERNAL_CALL
IntUserFreeSharedSectionPool(IN PSHARED_SECTION_POOL SharedSectionPool);

NTSTATUS INTERNAL_CALL
InUserDeleteSharedSection(IN PSHARED_SECTION_POOL SharedSectionPool,
                          IN PVOID SystemMappedBase);

NTSTATUS INTERNAL_CALL
IntUserCreateSharedSection(IN PSHARED_SECTION_POOL SharedSectionPool,
                           IN OUT PVOID *SystemMappedBase,
                           IN OUT ULONG *SharedSectionSize);

NTSTATUS INTERNAL_CALL
IntUserMapSharedSection(IN PSHARED_SECTION_POOL SharedSectionPool,
                        IN PEPROCESS Process,
                        IN PVOID SystemMappedBase,
                        IN PLARGE_INTEGER SectionOffset  OPTIONAL,
                        IN OUT PVOID *UserMappedBase,
                        IN PULONG ViewSize  OPTIONAL,
                        IN BOOLEAN ReadOnly);

NTSTATUS INTERNAL_CALL
IntUserUnMapSharedSection(IN PSHARED_SECTION_POOL SharedSectionPool,
                          IN PEPROCESS Process,
                          IN PVOID SystemMappedBase,
                          IN PVOID UserMappedBase);

extern PSHARED_SECTION_POOL SessionSharedSectionPool;

#endif /* ! defined(_WIN32K_SSEC_H) */

/* EOF */

