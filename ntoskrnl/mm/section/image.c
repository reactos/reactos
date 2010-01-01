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

/*
 TODO: not that great (declaring loaders statically, having to declare all of
 them, having to keep them extern, etc.), will fix in the future
*/
extern NTSTATUS NTAPI PeFmtCreateSection
(
 IN CONST VOID * FileHeader,
 IN SIZE_T FileHeaderSize,
 IN PVOID File,
 OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
 OUT PULONG Flags,
 IN PEXEFMT_CB_READ_FILE ReadFileCb,
 IN PEXEFMT_CB_ALLOCATE_SEGMENTS AllocateSegmentsCb
);

extern NTSTATUS NTAPI ElfFmtCreateSection
(
 IN CONST VOID * FileHeader,
 IN SIZE_T FileHeaderSize,
 IN PVOID File,
 OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
 OUT PULONG Flags,
 IN PEXEFMT_CB_READ_FILE ReadFileCb,
 IN PEXEFMT_CB_ALLOCATE_SEGMENTS AllocateSegmentsCb
);

/* TODO: this is a standard DDK/PSDK macro */
#ifndef RTL_NUMBER_OF
#define RTL_NUMBER_OF(ARR_) (sizeof(ARR_) / sizeof((ARR_)[0]))
#endif

static PEXEFMT_LOADER ExeFmtpLoaders[] =
{
 PeFmtCreateSection,
#ifdef __ELF
 ElfFmtCreateSection
#endif
};

static
PMM_SECTION_SEGMENT
NTAPI
ExeFmtpAllocateSegments(IN ULONG NrSegments)
{
 SIZE_T SizeOfSegments;
 PMM_SECTION_SEGMENT Segments;

 /* TODO: check for integer overflow */
 SizeOfSegments = sizeof(MM_SECTION_SEGMENT) * NrSegments;

 Segments = ExAllocatePoolWithTag(NonPagedPool,
                                  SizeOfSegments,
                                  TAG_MM_SECTION_SEGMENT);

 if(Segments)
  RtlZeroMemory(Segments, SizeOfSegments);

 return Segments;
}

static
NTSTATUS
NTAPI
ExeFmtpReadFile(IN PVOID File,
                IN PLARGE_INTEGER Offset,
                IN ULONG Length,
                OUT PVOID * Data,
                OUT PVOID * AllocBase,
                OUT PULONG ReadSize)
{
   NTSTATUS Status;
   LARGE_INTEGER FileOffset;
   ULONG AdjustOffset;
   ULONG OffsetAdjustment;
   ULONG BufferSize;
   ULONG UsedSize;
   PVOID Buffer;
   IO_STATUS_BLOCK Iosb;

   ASSERT_IRQL_LESS(DISPATCH_LEVEL);

   if(Length == 0)
   {
      ASSERT(FALSE);
   }

   FileOffset = *Offset;

   /* Negative/special offset: it cannot be used in this context */
   if(FileOffset.u.HighPart < 0)
   {
      ASSERT(FALSE);
   }

   AdjustOffset = PAGE_ROUND_DOWN(FileOffset.u.LowPart);
   OffsetAdjustment = FileOffset.u.LowPart - AdjustOffset;
   FileOffset.u.LowPart = AdjustOffset;

   BufferSize = Length + OffsetAdjustment;
   BufferSize = PAGE_ROUND_UP(BufferSize);

   /*
    * It's ok to use paged pool, because this is a temporary buffer only used in
    * the loading of executables. The assumption is that MmCreateSection is
    * always called at low IRQLs and that these buffers don't survive a brief
    * initialization phase
    */
   Buffer = ExAllocatePoolWithTag(PagedPool,
                                  BufferSize,
                                  'MmXr');

   UsedSize = 0;

   Status = MiSimpleRead(File, &FileOffset, Buffer, BufferSize, &Iosb);

   if(NT_SUCCESS(Status) && UsedSize < OffsetAdjustment)
   {
      Status = STATUS_IN_PAGE_ERROR;
      ASSERT(!NT_SUCCESS(Status));
   }

   UsedSize = Iosb.Information;

   if(NT_SUCCESS(Status))
   {
      *Data = (PVOID)((ULONG_PTR)Buffer + OffsetAdjustment);
      *AllocBase = Buffer;
      *ReadSize = UsedSize - OffsetAdjustment;
   }
   else
   {
      ExFreePool(Buffer);
   }

   return Status;
}

#ifdef NASSERT
# define MmspAssertSegmentsSorted(OBJ_) ((void)0)
# define MmspAssertSegmentsNoOverlap(OBJ_) ((void)0)
# define MmspAssertSegmentsPageAligned(OBJ_) ((void)0)
#else
static
VOID
NTAPI
MmspAssertSegmentsSorted(IN PMM_IMAGE_SECTION_OBJECT ImageSectionObject)
{
   ULONG i;

   for( i = 1; i < ImageSectionObject->NrSegments; ++ i )
   {
      ASSERT(ImageSectionObject->Segments[i].Image.VirtualAddress >=
             ImageSectionObject->Segments[i - 1].Image.VirtualAddress);
   }
}

static
VOID
NTAPI
MmspAssertSegmentsNoOverlap(IN PMM_IMAGE_SECTION_OBJECT ImageSectionObject)
{
   ULONG i;

   MmspAssertSegmentsSorted(ImageSectionObject);

   for( i = 0; i < ImageSectionObject->NrSegments; ++ i )
   {
      ASSERT(ImageSectionObject->Segments[i].Length.QuadPart > 0);

      if(i > 0)
      {
         ASSERT(ImageSectionObject->Segments[i].Image.VirtualAddress >=
                (ImageSectionObject->Segments[i - 1].Image.VirtualAddress +
                 ImageSectionObject->Segments[i - 1].Length.QuadPart));
      }
   }
}

static
VOID
NTAPI
MmspAssertSegmentsPageAligned(IN PMM_IMAGE_SECTION_OBJECT ImageSectionObject)
{
   ULONG i;

   for( i = 0; i < ImageSectionObject->NrSegments; ++ i )
   {
      ASSERT((ImageSectionObject->Segments[i].Image.VirtualAddress % PAGE_SIZE) == 0);
      ASSERT((ImageSectionObject->Segments[i].Length.QuadPart % PAGE_SIZE) == 0);
   }
}
#endif

static
int
__cdecl
MmspCompareSegments(const void * x,
                    const void * y)
{
   const MM_SECTION_SEGMENT *Segment1 = (const MM_SECTION_SEGMENT *)x;
   const MM_SECTION_SEGMENT *Segment2 = (const MM_SECTION_SEGMENT *)y;

   return
      (Segment1->Image.VirtualAddress - Segment2->Image.VirtualAddress) >>
      ((sizeof(ULONG_PTR) - sizeof(int)) * 8);
}

/*
 * Ensures an image section's segments are sorted in memory
 */
static
VOID
NTAPI
MmspSortSegments(IN OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
                 IN ULONG Flags)
{
   if (Flags & EXEFMT_LOAD_ASSUME_SEGMENTS_SORTED)
   {
      MmspAssertSegmentsSorted(ImageSectionObject);
   }
   else
   {
      qsort(ImageSectionObject->Segments,
            ImageSectionObject->NrSegments,
            sizeof(ImageSectionObject->Segments[0]),
            MmspCompareSegments);
   }
}


/*
 * Ensures an image section's segments don't overlap in memory and don't have
 * gaps and don't have a null size. We let them map to overlapping file regions,
 * though - that's not necessarily an error
 */
static
BOOLEAN
NTAPI
MmspCheckSegmentBounds
(
 IN OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
 IN ULONG Flags
)
{
   ULONG i;

   if (Flags & EXEFMT_LOAD_ASSUME_SEGMENTS_NO_OVERLAP)
   {
      MmspAssertSegmentsNoOverlap(ImageSectionObject);
      return TRUE;
   }

   ASSERT(ImageSectionObject->NrSegments >= 1);

   for ( i = 0; i < ImageSectionObject->NrSegments; ++ i )
   {
	   if(ImageSectionObject->Segments[i].Length.QuadPart == 0)
      {
         return FALSE;
      }

      if(i > 0)
      {
         /*
          * TODO: relax the limitation on gaps. For example, gaps smaller than a
          * page could be OK (Windows seems to be OK with them), and larger gaps
          * could lead to image sections spanning several discontiguous regions
          * (NtMapViewOfSection could then refuse to map them, and they could
          * e.g. only be allowed as parameters to NtCreateProcess, like on UNIX)
          */
         if ((ImageSectionObject->Segments[i - 1].Image.VirtualAddress +
              ImageSectionObject->Segments[i - 1].Length.QuadPart) !=
              ImageSectionObject->Segments[i].Image.VirtualAddress)
         {
            return FALSE;
         }
      }
   }

   return TRUE;
}

/*
 * Merges and pads an image section's segments until they all are page-aligned
 * and have a size that is a multiple of the page size
 */
static
BOOLEAN
NTAPI
MmspPageAlignSegments
(
 IN OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
 IN ULONG Flags
)
{
   ULONG i;
   ULONG LastSegment;
   BOOLEAN Initialized;
   PMM_SECTION_SEGMENT EffectiveSegment;

   if (Flags & EXEFMT_LOAD_ASSUME_SEGMENTS_PAGE_ALIGNED)
   {
      MmspAssertSegmentsPageAligned(ImageSectionObject);
      return TRUE;
   }

   Initialized = FALSE;
   LastSegment = 0;
   EffectiveSegment = &ImageSectionObject->Segments[LastSegment];

   for ( i = 0; i < ImageSectionObject->NrSegments; ++ i )
   {
      /*
       * The first segment requires special handling
       */
      if (i == 0)
      {
         ULONG_PTR VirtualAddress;
         ULONG_PTR VirtualOffset;

         VirtualAddress = EffectiveSegment->Image.VirtualAddress;

         /* Round down the virtual address to the nearest page */
         EffectiveSegment->Image.VirtualAddress = PAGE_ROUND_DOWN(VirtualAddress);

         /* Round up the virtual size to the nearest page */
         EffectiveSegment->Length.QuadPart = PAGE_ROUND_UP(VirtualAddress + EffectiveSegment->Length.QuadPart) -
                                    EffectiveSegment->Image.VirtualAddress;

         /* Adjust the raw address and size */
         VirtualOffset = VirtualAddress - EffectiveSegment->Image.VirtualAddress;

         if (EffectiveSegment->Image.FileOffset/*.QuadPart*/ < VirtualOffset)
         {
            return FALSE;
         }

         /*
          * Garbage in, garbage out: unaligned base addresses make the file
          * offset point in curious and odd places, but that's what we were
          * asked for
          */
         EffectiveSegment->Image.FileOffset/*.QuadPart*/ -= VirtualOffset;
         EffectiveSegment->RawLength.QuadPart += VirtualOffset;
      }
      else
      {
         PMM_SECTION_SEGMENT Segment = &ImageSectionObject->Segments[i];
         ULONG_PTR EndOfEffectiveSegment;

         EndOfEffectiveSegment = EffectiveSegment->Image.VirtualAddress + EffectiveSegment->Length.QuadPart;
         ASSERT((EndOfEffectiveSegment % PAGE_SIZE) == 0);

         /*
          * The current segment begins exactly where the current effective
          * segment ended, therefore beginning a new effective segment
          */
         if (EndOfEffectiveSegment == Segment->Image.VirtualAddress)
         {
            LastSegment ++;
            ASSERT(LastSegment <= i);
            ASSERT(LastSegment < ImageSectionObject->NrSegments);

            EffectiveSegment = &ImageSectionObject->Segments[LastSegment];

            if (LastSegment != i)
            {
               /*
                * Copy the current segment. If necessary, the effective segment
                * will be expanded later
                */
               *EffectiveSegment = *Segment;
            }

            /*
             * Page-align the virtual size. We know for sure the virtual address
             * already is
             */
            ASSERT((EffectiveSegment->Image.VirtualAddress % PAGE_SIZE) == 0);
            EffectiveSegment->Length.QuadPart = PAGE_ROUND_UP(EffectiveSegment->Length.QuadPart);
         }
         /*
          * The current segment is still part of the current effective segment:
          * extend the effective segment to reflect this
          */
         else if (EndOfEffectiveSegment > Segment->Image.VirtualAddress)
         {
            static const ULONG FlagsToProtection[16] =
            {
               PAGE_NOACCESS,
               PAGE_READONLY,
               PAGE_READWRITE,
               PAGE_READWRITE,
               PAGE_EXECUTE_READ,
               PAGE_EXECUTE_READ,
               PAGE_EXECUTE_READWRITE,
               PAGE_EXECUTE_READWRITE,
               PAGE_WRITECOPY,
               PAGE_WRITECOPY,
               PAGE_WRITECOPY,
               PAGE_WRITECOPY,
               PAGE_EXECUTE_WRITECOPY,
               PAGE_EXECUTE_WRITECOPY,
               PAGE_EXECUTE_WRITECOPY,
               PAGE_EXECUTE_WRITECOPY
            };

            unsigned ProtectionFlags;

            /*
             * Extend the file size
             */

            /* Unaligned segments must be contiguous within the file */
            if (Segment->Image.FileOffset/*.QuadPart*/ != 
				(EffectiveSegment->Image.FileOffset/*.QuadPart*/ +
		 EffectiveSegment->RawLength.QuadPart))
            {
               return FALSE;
            }

            EffectiveSegment->RawLength.QuadPart += Segment->RawLength.QuadPart;

            /*
             * Extend the virtual size
             */
            ASSERT(PAGE_ROUND_UP(Segment->Image.VirtualAddress + Segment->Length.QuadPart) >= EndOfEffectiveSegment);

            EffectiveSegment->Length.QuadPart = 
				PAGE_ROUND_UP(Segment->Image.VirtualAddress + Segment->Length.QuadPart) -
				EffectiveSegment->Image.VirtualAddress;

            /*
             * Merge the protection
             */
            EffectiveSegment->Protection |= Segment->Protection;

            /* Clean up redundance */
            ProtectionFlags = 0;

            if(EffectiveSegment->Protection & PAGE_IS_READABLE)
               ProtectionFlags |= 1 << 0;

            if(EffectiveSegment->Protection & PAGE_IS_WRITABLE)
               ProtectionFlags |= 1 << 1;

            if(EffectiveSegment->Protection & PAGE_IS_EXECUTABLE)
               ProtectionFlags |= 1 << 2;

            if(EffectiveSegment->Protection & PAGE_IS_WRITECOPY)
               ProtectionFlags |= 1 << 3;

            ASSERT(ProtectionFlags < 16);
            EffectiveSegment->Protection = FlagsToProtection[ProtectionFlags];

            /* If a segment was required to be shared and cannot, fail */
            if(!(Segment->Protection & PAGE_IS_WRITECOPY) &&
               EffectiveSegment->Protection & PAGE_IS_WRITECOPY)
            {
               return FALSE;
            }
         }
         /*
          * We assume no holes between segments at this point
          */
         else
         {
            ASSERT(FALSE);
         }
      }
   }
   ImageSectionObject->NrSegments = LastSegment + 1;

   return TRUE;
}

NTSTATUS
ExeFmtpCreateImageSection(PFILE_OBJECT FileObject,
                          PMM_IMAGE_SECTION_OBJECT ImageSectionObject)
{
   LARGE_INTEGER Offset;
   PVOID FileHeader;
   PVOID FileHeaderBuffer;
   ULONG FileHeaderSize;
   ULONG Flags;
   ULONG OldNrSegments;
   NTSTATUS Status;
   ULONG i;

   /*
    * Read the beginning of the file (2 pages). Should be enough to contain
    * all (or most) of the headers
    */
   Offset.QuadPart = 0;

   /* FIXME: use FileObject instead of FileHandle */
   Status = ExeFmtpReadFile (FileObject,
                             &Offset,
                             PAGE_SIZE * 2,
                             &FileHeader,
                             &FileHeaderBuffer,
                             &FileHeaderSize);

   if (!NT_SUCCESS(Status))
      return Status;

   if (FileHeaderSize == 0)
   {
      ExFreePool(FileHeaderBuffer);
      return STATUS_UNSUCCESSFUL;
   }

   /*
    * Look for a loader that can handle this executable
    */
   for (i = 0; i < RTL_NUMBER_OF(ExeFmtpLoaders); ++ i)
   {
      RtlZeroMemory(ImageSectionObject, sizeof(*ImageSectionObject));
      Flags = 0;

      /* FIXME: use FileObject instead of FileHandle */
      Status = ExeFmtpLoaders[i](FileHeader,
                                 FileHeaderSize,
                                 FileObject,
                                 ImageSectionObject,
                                 &Flags,
                                 ExeFmtpReadFile,
                                 ExeFmtpAllocateSegments);

      if (!NT_SUCCESS(Status))
      {
         if (ImageSectionObject->Segments)
         {
            ExFreePool(ImageSectionObject->Segments);
            ImageSectionObject->Segments = NULL;
         }
      }

      if (Status != STATUS_ROS_EXEFMT_UNKNOWN_FORMAT)
         break;
   }

   ExFreePool(FileHeaderBuffer);

   /*
    * No loader handled the format
    */
   if (Status == STATUS_ROS_EXEFMT_UNKNOWN_FORMAT)
   {
      Status = STATUS_INVALID_IMAGE_NOT_MZ;
      ASSERT(!NT_SUCCESS(Status));
   }

   if (!NT_SUCCESS(Status))
      return Status;

   ASSERT(ImageSectionObject->Segments != NULL);

   /*
    * Some defaults
    */
   /* FIXME? are these values platform-dependent? */
   if(ImageSectionObject->StackReserve == 0)
      ImageSectionObject->StackReserve = 0x40000;

   if(ImageSectionObject->StackCommit == 0)
      ImageSectionObject->StackCommit = 0x1000;

   if(ImageSectionObject->ImageBase == 0)
   {
      if(ImageSectionObject->ImageCharacteristics & IMAGE_FILE_DLL)
         ImageSectionObject->ImageBase = 0x10000000;
      else
         ImageSectionObject->ImageBase = 0x00400000;
   }

   /*
    * And now the fun part: fixing the segments
    */

   /* Sort them by virtual address */
   MmspSortSegments(ImageSectionObject, Flags);

   /* Ensure they don't overlap in memory */
   if (!MmspCheckSegmentBounds(ImageSectionObject, Flags))
      return STATUS_INVALID_IMAGE_FORMAT;

   /* Ensure they are aligned */
   OldNrSegments = ImageSectionObject->NrSegments;

   if (!MmspPageAlignSegments(ImageSectionObject, Flags))
      return STATUS_INVALID_IMAGE_FORMAT;

   /* Trim them if the alignment phase merged some of them */
   if (ImageSectionObject->NrSegments < OldNrSegments)
   {
      PMM_SECTION_SEGMENT Segments;
      SIZE_T SizeOfSegments;

      SizeOfSegments = sizeof(MM_SECTION_SEGMENT) * ImageSectionObject->NrSegments;

      Segments = ExAllocatePoolWithTag(PagedPool,
                                       SizeOfSegments,
                                       TAG_MM_SECTION_SEGMENT);

      if (Segments == NULL)
         return STATUS_INSUFFICIENT_RESOURCES;

      RtlCopyMemory(Segments, ImageSectionObject->Segments, SizeOfSegments);
      ExFreePool(ImageSectionObject->Segments);
      ImageSectionObject->Segments = Segments;
   }

   /* And finish their initialization */
   for ( i = 0; i < ImageSectionObject->NrSegments; ++ i )
   {
      ExInitializeFastMutex(&ImageSectionObject->Segments[i].Lock);
	  ImageSectionObject->Segments[i].Flags = MM_IMAGE_SEGMENT;
      ImageSectionObject->Segments[i].ReferenceCount = 1;
	  ImageSectionObject->Segments[i].FileObject = FileObject;
	  ImageSectionObject->Segments[i].WriteCopy = 
		  !(ImageSectionObject->Segments[i].Image.Characteristics & 
			IMAGE_SCN_MEM_SHARED);
	  MiInitializeSectionPageTable(&ImageSectionObject->Segments[i]);
   }

   DPRINT("ImageSection %x ref count %d\n", ImageSectionObject, ImageSectionObject->RefCount);

   ASSERT(NT_SUCCESS(Status));
   return Status;
}

NTSTATUS
MmCreateImageSection
(PROS_SECTION_OBJECT *SectionObject,
 ACCESS_MASK DesiredAccess,
 POBJECT_ATTRIBUTES ObjectAttributes,
 PLARGE_INTEGER UMaximumSize,
 ULONG SectionPageProtection,
 ULONG AllocationAttributes,
 PFILE_OBJECT FileObject)
{
   PROS_SECTION_OBJECT Section;
   NTSTATUS Status;
   PMM_SECTION_SEGMENT SectionSegments;
   PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
   ULONG i;
   ULONG FileAccess = 0;

   /*
    * Specifying a maximum size is meaningless for an image section
    */
   if (UMaximumSize != NULL)
   {
	  DPRINT1("STATUS_INVALID_PARAMETER_4\n");
      return(STATUS_INVALID_PARAMETER_4);
   }

   /*
    * Check file access required
    */
   if (SectionPageProtection & PAGE_READWRITE ||
         SectionPageProtection & PAGE_EXECUTE_READWRITE)
   {
      FileAccess = FILE_READ_DATA | FILE_WRITE_DATA;
   }
   else
   {
      FileAccess = FILE_READ_DATA;
   }

   /*
    * Reference the file handle
    */
   ObReferenceObject(FileObject);

   /*
    * Create the section
    */
   Status = ObCreateObject 
	   (ExGetPreviousMode(),
		MmSectionObjectType,
		ObjectAttributes,
		ExGetPreviousMode(),
		NULL,
		sizeof(ROS_SECTION_OBJECT),
		0,
		0,
		(PVOID*)&Section);

   if (!NT_SUCCESS(Status))
   {
      ObDereferenceObject(FileObject);
	  DPRINT1("Failed - Status %x\n", Status);
      return(Status);
   }

   RtlZeroMemory(Section, sizeof(ROS_SECTION_OBJECT));

   /*
    * Initialize it
    */
   Section->FileObject = FileObject;
   Section->SectionPageProtection = SectionPageProtection;
   Section->AllocationAttributes = AllocationAttributes;

   if (FileObject->SectionObjectPointer->ImageSectionObject == NULL)
   {
      NTSTATUS StatusExeFmt;

      ImageSectionObject = ExAllocatePoolWithTag(PagedPool, sizeof(MM_IMAGE_SECTION_OBJECT), TAG_MM_SECTION_SEGMENT);
      if (ImageSectionObject == NULL)
      {
		 ObDereferenceObject(Section);
		 DPRINT1("STATUS_NO_MEMORY");
         return(STATUS_NO_MEMORY);
      }

      RtlZeroMemory(ImageSectionObject, sizeof(MM_IMAGE_SECTION_OBJECT));

      StatusExeFmt = ExeFmtpCreateImageSection(FileObject, ImageSectionObject);

      if (!NT_SUCCESS(StatusExeFmt))
      {
		 ObDereferenceObject(Section);
		 DPRINT1("StatusExeFmt %x\n", StatusExeFmt);
         return(StatusExeFmt);
      }

	  ImageSectionObject->RefCount = 1;
      Section->ImageSection = ImageSectionObject;
      ASSERT(ImageSectionObject->Segments);

      /*
       * Lock the file
       */
      Status = MmspWaitForFileLock(FileObject);
      if (!NT_SUCCESS(Status))
      {
		 ObDereferenceObject(Section);
		 DPRINT1("Status %x\n", Status);
         return(Status);
      }

      if (NULL != InterlockedCompareExchangePointer
		  (&FileObject->SectionObjectPointer->ImageSectionObject,
		   ImageSectionObject, NULL))
      {
         /*
          * An other thread has initialized the some image in the background
          */
         ExFreePool(ImageSectionObject->Segments);
         ExFreePool(ImageSectionObject);
         ImageSectionObject = FileObject->SectionObjectPointer->ImageSectionObject;
         Section->ImageSection = ImageSectionObject;
         SectionSegments = ImageSectionObject->Segments;
		 (void)InterlockedIncrementUL(&ImageSectionObject->RefCount);

		 Status = STATUS_SUCCESS;
      }
	  else
	  {
         for (i = 0; i < ImageSectionObject->NrSegments; i++)
         {
			 ImageSectionObject->Segments[i].FileObject = FileObject;
         }
	  }

      Status = StatusExeFmt;
   }
   else
   {
      /*
       * Lock the file
       */
      Status = MmspWaitForFileLock(FileObject);
      if (Status != STATUS_SUCCESS)
      {
		 ObDereferenceObject(Section);
		 DPRINT1("Status %x\n", Status);
         return(Status);
      }

      ImageSectionObject = FileObject->SectionObjectPointer->ImageSectionObject;
      Section->ImageSection = ImageSectionObject;
      SectionSegments = ImageSectionObject->Segments;
	  (void)InterlockedIncrementUL(&ImageSectionObject->RefCount);

      Status = STATUS_SUCCESS;
   }
   //KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
   *SectionObject = Section;
   return(Status);
}

VOID
NTAPI
MiDeleteImageSection(PROS_SECTION_OBJECT Section)
{
	PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
	PMM_SECTION_SEGMENT SectionSegments;
	ULONG NrSegments;
	ULONG i;
	ULONG RefCount;
	
	/*
	 * NOTE: Section->ImageSection can be NULL for short time
	 * during the section creating. If we fail for some reason
	 * until the image section is properly initialized we shouldn't
	 * process further here.
	 */
	if (Section->ImageSection == NULL)
	{
		DPRINT1("No image section\n");
		return;
	}

	ImageSectionObject = (PMM_IMAGE_SECTION_OBJECT)Section->ImageSection;
	DPRINT1("Deleting image section (%x:%d)\n", ImageSectionObject, ImageSectionObject->RefCount);
	if ((RefCount = InterlockedDecrementUL(&ImageSectionObject->RefCount)) == 0)
	{
		DPRINT1("Finalize\n");
		NrSegments = ImageSectionObject->NrSegments;
		SectionSegments = ImageSectionObject->Segments;
		DPRINT1("Freeing section %wZ\n", &Section->FileObject->FileName);
		
		for (i = 0; i < NrSegments; i++)
		{
			MiFreePageTablesSectionSegment(&SectionSegments[i], MiFreeSegmentPage);
		}

		Section->FileObject->SectionObjectPointer->ImageSectionObject = NULL;
		Section->ImageSection = NULL;
		ExFreePool(ImageSectionObject->Segments);
		ExFreePool(ImageSectionObject);
		DPRINT1("Done\n");
	}

	DPRINT1("Done? %d\n", RefCount);
}

NTSTATUS
NTAPI
MiMapImageFileSection
(PMMSUPPORT AddressSpace,
 PROS_SECTION_OBJECT Section,
 PVOID *BaseAddress)
{
	ULONG i;
	ULONG NrSegments;
	ULONG_PTR ImageBase;
	ULONG ImageSize;
	NTSTATUS Status = STATUS_SUCCESS;
	PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
	PMM_SECTION_SEGMENT SectionSegments;
	
	ImageSectionObject = Section->ImageSection;
	SectionSegments = ImageSectionObject->Segments;
	NrSegments = ImageSectionObject->NrSegments;	

	ImageBase = (ULONG_PTR)*BaseAddress;
	if (ImageBase == 0)
	{
		ImageBase = ImageSectionObject->ImageBase;
	}
	
	ImageSize = 0;
	for (i = 0; i < NrSegments; i++)
	{
		if (!(SectionSegments[i].Image.Characteristics & IMAGE_SCN_TYPE_NOLOAD))
		{
            ULONG_PTR MaxExtent;
            MaxExtent = (ULONG_PTR)SectionSegments[i].Image.VirtualAddress +
				SectionSegments[i].Length.QuadPart;
            ImageSize = max(ImageSize, MaxExtent);
		}
	}
	
	ImageSectionObject->ImageSize = ImageSize;
	
	/* Check there is enough space to map the section at that point. */
	if ((AddressSpace != MmGetKernelAddressSpace() &&
		 (ULONG_PTR)ImageBase >= (ULONG_PTR)MM_HIGHEST_USER_ADDRESS) ||
		MmLocateMemoryAreaByRegion(AddressSpace, (PVOID)ImageBase,
								   PAGE_ROUND_UP(ImageSize)) != NULL)
	{
		/* Fail if the user requested a fixed base address. */
		if ((*BaseAddress) != NULL)
		{
            return(STATUS_UNSUCCESSFUL);
		}
		/* Otherwise find a gap to map the image. */
		ImageBase = (ULONG_PTR)MmFindGap(AddressSpace, PAGE_ROUND_UP(ImageSize), PAGE_SIZE, FALSE);
		if (ImageBase == 0)
		{
            return(STATUS_UNSUCCESSFUL);
		}
	}
	
	for (i = 0; i < NrSegments; i++)
	{
		if (!(SectionSegments[i].Image.Characteristics & IMAGE_SCN_TYPE_NOLOAD))
		{
            PVOID SBaseAddress = (PVOID)
				((char*)ImageBase + (ULONG_PTR)SectionSegments[i].Image.VirtualAddress);
            MmLockSectionSegment(&SectionSegments[i]);
            Status = MiMapViewOfSegment(AddressSpace,
                                        Section,
                                        &SectionSegments[i],
                                        &SBaseAddress,
                                        SectionSegments[i].Length.u.LowPart,
                                        SectionSegments[i].Protection,
                                        0,
                                        0);
            MmUnlockSectionSegment(&SectionSegments[i]);
		}
	}
	
	if (NT_SUCCESS(Status))
		*BaseAddress = (PVOID)ImageBase;
	
	return Status;
}

NTSTATUS
NTAPI
MiUnmapImageSection
(PMMSUPPORT AddressSpace, PMEMORY_AREA MemoryArea, PVOID BaseAddress)
{
	PMM_SECTION_SEGMENT Segment;
	NTSTATUS Status = STATUS_SUCCESS;
    PVOID ImageBaseAddress = 0;
	PROS_SECTION_OBJECT Section;

	DPRINT1("MiUnmapImageSection @ %x\n", BaseAddress);
	MemoryArea->DeleteInProgress = TRUE;
	Section = MemoryArea->Data.SectionData.Section;
	Segment = MemoryArea->Data.SectionData.Segment;
	MemoryArea->Data.SectionData.Segment = NULL;
	
	/* Search for the current segment within the section segments
	 * and calculate the image base address */
	DPRINT1("unmapping segment %x\n", Segment);
	ImageBaseAddress = (char*)BaseAddress - Segment->Image.VirtualAddress;
	{
		PLIST_ENTRY CurrentEntry;
		PMM_REGION CurrentRegion;
		PLIST_ENTRY RegionListHead;
		PVOID Context[2];
		
		DPRINT1("Unmap segment @ %x\n", BaseAddress);

		MmLockSectionSegment(Segment);
		
		RegionListHead = &MemoryArea->Data.SectionData.RegionListHead;
		while (!IsListEmpty(RegionListHead))
		{
			CurrentEntry = RemoveHeadList(RegionListHead);
			CurrentRegion = CONTAINING_RECORD(CurrentEntry, MM_REGION, RegionListEntry);
			ExFreePool(CurrentRegion);
		}

		Context[0] = AddressSpace;
		Context[1] = Segment;
		
		Status = MmFreeMemoryArea
			(AddressSpace, MemoryArea, MmFreeSectionPage, Context);
		
		MmUnlockSectionSegment(Segment);
	}
	DPRINT1("Found and unmapped: %x\n", Status);

	/* Notify debugger */
	if (BaseAddress == ImageBaseAddress) 
		DbgkUnMapViewOfSection(ImageBaseAddress);

	ObDereferenceObject(Section);

	//DPRINT("MiUnmapImageSection Status %x ImageBase %x\n", Status, ImageBaseAddress);
	return Status;
}

