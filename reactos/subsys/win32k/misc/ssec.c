/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998 - 2005 ReactOS Team
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
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          shared sections support
 * FILE:             subsys/win32k/misc/ssec.c
 * PROGRAMER:        Thomas Weidenmueller <w3seek@reactos.com>
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/*
 * FIXME - instead of mapping the memory into system space using
 *         MmMapViewInSystemSpace() we should rather use
 *         MmMapViewInSessionSpace() to map it into session space!
 */

NTSTATUS INTERNAL_CALL
IntUserCreateSharedSectionPool(IN ULONG MaximumPoolSize,
                               IN PSHARED_SECTION_POOL *SharedSectionPool)
{
  PSHARED_SECTION_POOL Pool;
  ULONG PoolStructSize;

  ASSERT(SharedSectionPool);

  PoolStructSize = ROUND_UP(sizeof(SHARED_SECTION_POOL), PAGE_SIZE);
  Pool = ExAllocatePoolWithTag(NonPagedPool,
                               PoolStructSize,
                               TAG_SSECTPOOL);
  if(Pool != NULL)
  {
    RtlZeroMemory(Pool, PoolStructSize);

    /* initialize the session heap */
    ExInitializeFastMutex(&Pool->Lock);
    Pool->PoolSize = ROUND_UP(MaximumPoolSize, PAGE_SIZE);
    Pool->PoolFree = Pool->PoolSize;
    Pool->SharedSectionCount = 0;
    Pool->SectionsArray.Next = NULL;
    Pool->SectionsArray.nEntries = ((PoolStructSize - sizeof(SHARED_SECTION_POOL)) /
                                    sizeof(SHARED_SECTION)) - 1;

    ASSERT(Pool->SectionsArray.nEntries > 0);

    *SharedSectionPool = Pool;

    return STATUS_SUCCESS;
  }

  return STATUS_INSUFFICIENT_RESOURCES;
}


VOID INTERNAL_CALL
IntUserFreeSharedSectionPool(IN PSHARED_SECTION_POOL SharedSectionPool)
{
  PSHARED_SECTIONS_ARRAY Array, OldArray;
  PSHARED_SECTION SharedSection, LastSharedSection;

  ASSERT(SharedSectionPool);

  Array = &SharedSectionPool->SectionsArray;

  ExAcquireFastMutex(&SharedSectionPool->Lock);
  while(SharedSectionPool->SharedSectionCount > 0 && Array != NULL)
  {
    for(SharedSection = Array->SharedSection, LastSharedSection = SharedSection + Array->nEntries;
        SharedSection != LastSharedSection && SharedSectionPool->SharedSectionCount > 0;
        SharedSection++)
    {
      if(SharedSection->SectionObject != NULL)
      {
        ASSERT(SharedSection->SystemMappedBase);

        /* FIXME - use MmUnmapViewInSessionSpace() once implemented! */
        MmUnmapViewInSystemSpace(SharedSection->SystemMappedBase);
        /* dereference the keep-alive reference so the section get's deleted */
        ObDereferenceObject(SharedSection->SectionObject);

        SharedSectionPool->SharedSectionCount--;
      }
    }

    OldArray = Array;
    Array = Array->Next;

    /* all shared sections in this array were freed, link the following array to
       the main session heap and free this array */
    SharedSectionPool->SectionsArray.Next = Array;
    ExFreePool(OldArray);
  }

  ASSERT(SharedSectionPool->SectionsArray.Next == NULL);
  ASSERT(SharedSectionPool->SharedSectionCount == 0);

  ExReleaseFastMutex(&SharedSectionPool->Lock);
}


NTSTATUS INTERNAL_CALL
IntUserCreateSharedSection(IN PSHARED_SECTION_POOL SharedSectionPool,
                           IN OUT PVOID *SystemMappedBase,
                           IN OUT ULONG *SharedSectionSize)
{
  PSHARED_SECTIONS_ARRAY Array, LastArray;
  PSHARED_SECTION FreeSharedSection, SharedSection, LastSharedSection;
  LARGE_INTEGER SectionSize;
  ULONG Size;
  NTSTATUS Status;

  ASSERT(SharedSectionPool && SharedSectionSize && (*SharedSectionSize) > 0 && SystemMappedBase);

  FreeSharedSection = NULL;

  Size = ROUND_UP(*SharedSectionSize, PAGE_SIZE);

  ExAcquireFastMutex(&SharedSectionPool->Lock);

  if(Size > SharedSectionPool->PoolFree)
  {
    ExReleaseFastMutex(&SharedSectionPool->Lock);
    DPRINT1("Shared Section Pool limit (0x%x KB) reached, attempted to allocate a 0x%x KB shared section!\n",
            SharedSectionPool->PoolSize / 1024, (*SharedSectionSize) / 1024);
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  /* walk the array to find a free entry */
  for(Array = &SharedSectionPool->SectionsArray, LastArray = Array;
      Array != NULL && FreeSharedSection == NULL;
      Array = Array->Next)
  {
    LastArray = Array;

    for(SharedSection = Array->SharedSection, LastSharedSection = SharedSection + Array->nEntries;
        SharedSection != LastSharedSection;
        SharedSection++)
    {
      if(SharedSection->SectionObject == NULL)
      {
        FreeSharedSection = SharedSection;
        break;
      }
    }

    if(Array->Next != NULL)
    {
      LastArray = Array;
    }
  }

  ASSERT(LastArray);

  if(FreeSharedSection == NULL)
  {
    ULONG nNewEntries;
    PSHARED_SECTIONS_ARRAY NewArray;

    ASSERT(LastArray->Next == NULL);

    /* couldn't find a free entry in the array, extend the array */

    nNewEntries = ((PAGE_SIZE - sizeof(SHARED_SECTIONS_ARRAY)) / sizeof(SHARED_SECTION)) + 1;
    NewArray = ExAllocatePoolWithTag(NonPagedPool,
                                     sizeof(SHARED_SECTIONS_ARRAY) + ((nNewEntries - 1) *
                                                                      sizeof(SHARED_SECTION)),
                                     TAG_SSECTPOOL);
    if(NewArray == NULL)
    {
      ExReleaseFastMutex(&SharedSectionPool->Lock);
      DPRINT1("Failed to allocate new array for shared sections!\n");
      return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewArray->nEntries = nNewEntries;
    NewArray->Next = NULL;
    LastArray->Next = NewArray;

    Array = NewArray;
    FreeSharedSection = &Array->SharedSection[0];
  }

  ASSERT(FreeSharedSection);

  /* now allocate a real section */

  SectionSize.QuadPart = Size;
  Status = MmCreateSection(&FreeSharedSection->SectionObject,
                           SECTION_ALL_ACCESS,
                           NULL,
                           &SectionSize,
                           PAGE_EXECUTE_READWRITE,
                           SEC_COMMIT,
                           NULL,
                           NULL);
  if(NT_SUCCESS(Status))
  {
    Status = MmMapViewInSystemSpace(FreeSharedSection->SectionObject,
                                    &FreeSharedSection->SystemMappedBase,
                                    &FreeSharedSection->ViewSize);
    if(NT_SUCCESS(Status))
    {
      (*SharedSectionSize) -= Size;
      SharedSectionPool->SharedSectionCount++;

      *SystemMappedBase = FreeSharedSection->SystemMappedBase;
      *SharedSectionSize = FreeSharedSection->ViewSize;
    }
    else
    {
      ObDereferenceObject(FreeSharedSection->SectionObject);
      FreeSharedSection->SectionObject = NULL;
      DPRINT1("Failed to map the shared section into system space! Status 0x%x\n", Status);
    }
  }

  ExReleaseFastMutex(&SharedSectionPool->Lock);

  return Status;
}


NTSTATUS INTERNAL_CALL
InUserDeleteSharedSection(PSHARED_SECTION_POOL SharedSectionPool,
                          PVOID SystemMappedBase)
{
  PSHARED_SECTIONS_ARRAY Array;
  PSECTION_OBJECT SectionObject;
  PSHARED_SECTION SharedSection, LastSharedSection;
  NTSTATUS Status;

  ASSERT(SharedSectionPool && SystemMappedBase);

  SectionObject = NULL;

  ExAcquireFastMutex(&SharedSectionPool->Lock);

  for(Array = &SharedSectionPool->SectionsArray;
      Array != NULL && SectionObject == NULL;
      Array = Array->Next)
  {
    for(SharedSection = Array->SharedSection, LastSharedSection = SharedSection + Array->nEntries;
        SharedSection != LastSharedSection;
        SharedSection++)
    {
      if(SharedSection->SystemMappedBase == SystemMappedBase)
      {
        SectionObject = SharedSection->SectionObject;
        SharedSection->SectionObject = NULL;
        SharedSection->SystemMappedBase = NULL;

        ASSERT(SharedSectionPool->SharedSectionCount > 0);
        SharedSectionPool->SharedSectionCount--;
        break;
      }
    }
  }

  ExReleaseFastMutex(&SharedSectionPool->Lock);

  if(SectionObject != NULL)
  {
    Status = MmUnmapViewInSystemSpace(SystemMappedBase);
    ObDereferenceObject(SectionObject);
  }
  else
  {
    DPRINT1("Couldn't find and delete a shared section with SystemMappedBase=0x%x!\n", SystemMappedBase);
    Status = STATUS_UNSUCCESSFUL;
  }

  return Status;
}


NTSTATUS INTERNAL_CALL
IntUserMapSharedSection(IN PSHARED_SECTION_POOL SharedSectionPool,
                        IN PEPROCESS Process,
                        IN PVOID SystemMappedBase,
                        IN PLARGE_INTEGER SectionOffset  OPTIONAL,
                        IN OUT PVOID *UserMappedBase,
                        IN PULONG ViewSize  OPTIONAL,
                        IN BOOLEAN ReadOnly)
{
  PSHARED_SECTIONS_ARRAY Array;
  PSECTION_OBJECT SectionObject;
  PSHARED_SECTION SharedSection, LastSharedSection;
  NTSTATUS Status;

  ASSERT(SharedSectionPool && Process && SystemMappedBase && UserMappedBase);

  SectionObject = NULL;
  SharedSection = NULL;

  ExAcquireFastMutex(&SharedSectionPool->Lock);

  for(Array = &SharedSectionPool->SectionsArray;
      Array != NULL && SectionObject == NULL;
      Array = Array->Next)
  {
    for(SharedSection = Array->SharedSection, LastSharedSection = SharedSection + Array->nEntries;
        SharedSection != LastSharedSection;
        SharedSection++)
    {
      if(SharedSection->SystemMappedBase == SystemMappedBase)
      {
        SectionObject = SharedSection->SectionObject;
        break;
      }
    }
  }

  if(SectionObject != NULL)
  {
    ULONG RealViewSize = (ViewSize ? min(*ViewSize, SharedSection->ViewSize) : SharedSection->ViewSize);

    ObReferenceObjectByPointer(SectionObject,
                               (ReadOnly ? SECTION_MAP_READ : SECTION_MAP_READ | SECTION_MAP_WRITE),
                               NULL,
                               KernelMode);

    Status = MmMapViewOfSection(SectionObject,
                                Process,
                                UserMappedBase,
                                0,
                                0,
                                SectionOffset,
                                &RealViewSize,
                                ViewUnmap, /* not sure if we should inherit it... */
                                MEM_COMMIT,
                                (ReadOnly ? PAGE_READONLY : PAGE_READWRITE));
    if(!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to map shared section (readonly=%d) into user memory! Status: 0x%x\n", ReadOnly, Status);
    }
  }
  else
  {
    DPRINT1("Couldn't find and map a shared section with SystemMappedBase=0x%x!\n", SystemMappedBase);
    Status = STATUS_UNSUCCESSFUL;
  }

  ExReleaseFastMutex(&SharedSectionPool->Lock);

  return Status;
}


NTSTATUS INTERNAL_CALL
IntUserUnMapSharedSection(IN PSHARED_SECTION_POOL SharedSectionPool,
                          IN PEPROCESS Process,
                          IN PVOID SystemMappedBase,
                          IN PVOID UserMappedBase)
{
  PSHARED_SECTIONS_ARRAY Array;
  PSECTION_OBJECT SectionObject;
  PSHARED_SECTION SharedSection, LastSharedSection;
  NTSTATUS Status;

  ASSERT(SharedSectionPool && Process && SystemMappedBase && UserMappedBase);

  SectionObject = NULL;

  ExAcquireFastMutex(&SharedSectionPool->Lock);

  for(Array = &SharedSectionPool->SectionsArray;
      Array != NULL && SectionObject == NULL;
      Array = Array->Next)
  {
    for(SharedSection = Array->SharedSection, LastSharedSection = SharedSection + Array->nEntries;
        SharedSection != LastSharedSection;
        SharedSection++)
    {
      if(SharedSection->SystemMappedBase == SystemMappedBase)
      {
        SectionObject = SharedSection->SectionObject;
        break;
      }
    }
  }

  ExReleaseFastMutex(&SharedSectionPool->Lock);

  if(SectionObject != NULL)
  {
    Status = MmUnmapViewOfSection(Process,
                                  UserMappedBase);
    ObDereferenceObject(SectionObject);
    if(!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to unmap shared section UserMappedBase=0x%x! Status: 0x%x\n", UserMappedBase, Status);
    }
  }
  else
  {
    DPRINT1("Couldn't find and unmap a shared section with SystemMappedBase=0x%x!\n", SystemMappedBase);
    Status = STATUS_UNSUCCESSFUL;
  }

  return Status;
}
