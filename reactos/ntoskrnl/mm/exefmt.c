/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/* $Id: exefmt.c,v 1.1.2.1 2004/12/08 20:01:41 hyperion Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/pe.c
 * PURPOSE:         Multiple executable format support
 * PROGRAMMER:      KJK::Hyperion <hackbunny@reactos.com>
 * UPDATE HISTORY:
 *                  2004-12-06 Created
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

typedef NTSTATUS (NTAPI * PEXEFMT_CREATE_SECTION)
(
 IN CONST VOID * FileHeader,
 IN SIZE_T FileHeaderSize,
 IN PFILE_OBJECT File,
 IN PLARGE_INTEGER FileSize,
 OUT PMM_IMAGE_SECTION_OBJECT * ImageSectionObject,
 IN PEXEFMT_LOADER_READ_FILE ReadFileFunc,
 IN PEXEFMT_LOADER_ALLOCATE_SECTION AllocateSectionFunc
);

static const PEXEFMT_CREATE_SECTION ExeFmtpCreateSectionFormats[] =
{
 PeFmtCreateSection
};

PMM_IMAGE_SECTION_OBJECT
NTAPI
ExeFmtpAllocateSection
(
 IN ULONG NrSegments
)
{
 SIZE_T cbSize;
 PMM_IMAGE_SECTION_OBJECT p;

 cbSize = 
  FIELD_OFFSET(MM_IMAGE_SECTION_OBJECT, Segments) +
  NrSegments * sizeof(MM_SECTION_SEGMENT);

 p = ExAllocatePoolWithTag(NonPagedPool, cbSize, TAG_MM_SECTION_SEGMENT);

 if(p)
 {
  RtlZeroMemory(p, cbSize);
  p->NrSegments = NrSegments;
 }

 return p;
}

NTSTATUS
NTAPI
ExeFmtCreateSection
(
 IN CONST VOID * FileHeader,
 IN SIZE_T FileHeaderSize,
 IN PFILE_OBJECT File,
 IN PLARGE_INTEGER FileSize,
 IN OUT PMM_IMAGE_SECTION_OBJECT * ImageSectionObject
)
{
 SIZE_T i;
 NTSTATUS nStatus;

 for(i = 0; i < RTL_NUMBER_OF(ExeFmtpCreateSectionFormats); ++ i)
 {
  *ImageSectionObject = NULL;

  nStatus = ExeFmtpCreateSectionFormats[i]
  (
   FileHeader,
   FileHeaderSize,
   File,
   FileSize,
   ImageSectionObject,
   MmspPageRead,
   MmspAllocateImageSection
  );

  if(nStatus != STATUS_ROS_EXEFMT_UNKNOWN_FORMAT && NT_SUCCESS(nStatus))
  {
   ASSERT(*ImageSectionObject);
   return nStatus;
  }

  if(*ImageSectionObject)
   ExFreePool(*ImageSectionObject);
 }

 return STATUS_INVALID_IMAGE_FORMAT;
}

/* EOF */
