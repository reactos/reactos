/*
 * Copyright (C) 1998-2005 ReactOS Team (and the authors from the programmers section)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/section.c
 * PURPOSE:         Implements section objects
 *
 * PROGRAMMERS:     Rex Jolliff
 *                  David Welch
 *                  Eric Kohl
 *                  Emanuele Aliberti
 *                  Eugene Ingerman
 *                  Casper Hornstrup
 *                  KJK::Hyperion
 *                  Guido de Jong
 *                  Ge van Geldorp
 *                  Royce Mitchell III
 *                  Filip Navara
 *                  Aleksey Bragin
 *                  Jason Filby
 *                  Thomas Weidenmueller
 *                  Gunnar Andre' Dalsnes
 *                  Mike Nordell
 *                  Alex Ionescu
 *                  Gregor Anich
 *                  Steven Edwards
 *                  Herve Poussineau
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include <reactos/exeformat.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmCreatePhysicalMemorySection)
#pragma alloc_text(INIT, MmInitSectionImplementation)
#endif

extern KEVENT MmWaitPageEvent;

NTSTATUS
NTAPI
MmCreatePageFileSection
(PROS_SECTION_OBJECT *SectionObject,
 ACCESS_MASK DesiredAccess,
 POBJECT_ATTRIBUTES ObjectAttributes,
 PLARGE_INTEGER UMaximumSize,
 ULONG SectionPageProtection,
 ULONG AllocationAttributes)
/*
 * Create a section which is backed by the pagefile
 */
{
   LARGE_INTEGER MaximumSize;
   PROS_SECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   NTSTATUS Status;

   if (UMaximumSize == NULL)
   {
      return(STATUS_UNSUCCESSFUL);
   }
   MaximumSize = *UMaximumSize;

   /*
    * Create the section
    */
   Status = ObCreateObject(ExGetPreviousMode(),
                           MmSectionObjectType,
                           ObjectAttributes,
                           ExGetPreviousMode(),
                           NULL,
                           sizeof(ROS_SECTION_OBJECT),
                           0,
                           0,
                           (PVOID*)(PVOID)&Section);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   /*
    * Initialize it
    */
   Section->SectionPageProtection = SectionPageProtection;
   Section->AllocationAttributes = AllocationAttributes;
   Section->Segment = NULL;
   Section->FileObject = NULL;
   Section->MaximumSize = MaximumSize;
   Segment = ExAllocatePoolWithTag(NonPagedPool, sizeof(MM_SECTION_SEGMENT),
                                   TAG_MM_SECTION_SEGMENT);
   if (Segment == NULL)
   {
	  ObDereferenceObject(Section);
      return(STATUS_NO_MEMORY);
   }
   Section->Segment = Segment;
   Segment->FileObject = NULL;
   Segment->ReferenceCount = 1;
   ExInitializeFastMutex(&Segment->Lock);
   Segment->Protection = SectionPageProtection;
   Segment->RawLength.QuadPart = MaximumSize.QuadPart;
   Segment->Length.QuadPart = PAGE_ROUND_UP(MaximumSize.QuadPart);
   Segment->Flags = MM_PAGEFILE_SEGMENT;
   Segment->WriteCopy = FALSE;
   DPRINT1("Segment %x created (%x)\n", Segment, Segment->Flags);
   MiInitializeSectionPageTable(Segment);
   *SectionObject = Section;
   return(STATUS_SUCCESS);
}
