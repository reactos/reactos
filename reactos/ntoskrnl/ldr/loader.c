/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/loader.c
 * PURPOSE:         Loaders for PE executables
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <internal/kernel.h>
#include <internal/linkage.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>
#include "pe.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS LdrProcessImage(HANDLE SectionHandle, PVOID BaseAddress)
{
  PIMAGE_DOS_HEADER dos_hdr = (PIMAGE_DOS_HEADER)BaseAddress;
  PIMAGE_NT_HEADERS hdr = (PIMAGE_NT_HEADERS)(BaseAddress
      + dos_hdr->e_lfanew);
  PIMAGE_SECTION_HEADER sections = (PIMAGE_SECTION_HEADER)(BaseAddress 
      + dos_hdr->e_lfanew + sizeof(IMAGE_NT_HEADERS));

  // FIXME: Check image signature
  // FIXME: Check architechture
  // FIXME: Build/Load image sections
  // FIXME: resolve imports
  // FIXME: do fixups
   
}

NTSTATUS LdrLoadDriver(PUNICODE_STRING FileName)
/*
 * FUNCTION: Loads a PE executable into the kernel
 * ARGUMENTS:
 *         FileName = Driver to load
 * RETURNS: Status
 */
{
  NTSTATUS Status;
  HANDLE FileHandle;
  HANDLE SectionHandle;
  ANSI_STRING AnsiFileName;
  UNICODE_STRING UnicodeFileName;
  OBJECT_ATTRIBUTES FileAttributes;
  PVOID BaseAddress;

  //  Open the image file or die
  RtlInitAnsiString(&AnsiFileName, FileName);
  RtlAnsiStringToUnicodeString(&UnicodeFileName, &AnsiFileName, TRUE);
  InitializeObjectAttributes(&FileAttributes,
                             &UnicodeFileName, 
                             0,
                             NULL,
                             NULL);
  FileHandle = ZwFileOpen(&FileHandle, 0, &FileAttributes, NULL, 0, 0);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }
  RtlFreeUnicodeString(&UnicodeFileName);

  //  Map the image into a section or die
  Status = ZwCreateSection(&SectionHandle, 
                           SECTION_MAP_READ, 
                           NULL,
                           NULL,
                           PAGE_READONLY,
                           SEC_IMAGE,
                           FileHandle);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  // FIXME: get the base address of the section

  ZwCloseFile(FileHandle);

  return LdrProcessImage(SectionHandle, BaseAddress);
}

NTSTATUS LdrLoadImage(PUNICODE_STRING FileName)
/*
 * FUNCTION: Loads a PE executable into the current process
 * ARGUMENTS:
 *        FileName = File to load
 * RETURNS: Status
 */
{
  NTSTATUS Status;
  HANDLE FileHandle;
  HANDLE SectionHandle;
  ANSI_STRING AnsiFileName;
  UNICODE_STRING UnicodeFileName;
  OBJECT_ATTRIBUTES FileAttributes;
  PVOID BaseAddress;

  //  Open the image file or die
  RtlInitAnsiString(&AnsiFileName, FileName);
  RtlAnsiStringToUnicodeString(&UnicodeFileName, &AnsiFileName, TRUE);
  InitializeObjectAttributes(&FileAttributes,
                             &UnicodeFileName, 
                             0,
                             NULL,
                             NULL);
  FileHandle = ZwFileOpen(&FileHandle, 0, &FileAttributes, NULL, 0, 0);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }
  RtlFreeUnicodeString(&UnicodeFileName);

  // FIXME: should DLLs be named sections?
  // FIXME: get current process and associate with section

  //  Map the image into a section or die
  Status = ZwCreateSection(&SectionHandle, 
                           SECTION_MAP_READ, 
                           NULL,
                           NULL,
                           PAGE_READONLY,
                           SEC_IMAGE,
                           FileHandle);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  // FIXME: get the base address of the section

  ZwCloseFile(FileHandle);

  // FIXME: initialize process context for image

  return LdrProcessImage(SectionHandle, BaseAddress);
}

