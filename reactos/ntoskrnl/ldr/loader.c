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

/*
 * FUNCTION: Loads a PE executable into the specified process
 * ARGUMENTS:
 *        Filename = File to load
 *        ProcessHandle = handle 
 * RETURNS: Status
 */

NTSTATUS 
LdrLoadImage(PUNICODE_STRING Filename, HANDLE ProcessHandle)
{
  char BlockBuffer[512];
  NTSTATUS Status;
  HANDLE FileHandle;
  OBJECT_ATTRIBUTES FileObjectAttributes;
  PIMAGE_DOS_HEADER PEDosHeader;
  PIMAGE_NT_HEADERS PEHeader;

  HANDLE SectionHandle;
  PVOID BaseAddress;

  /*  Open the image file  */
  InitializeObjectAttributes(&FileObjectAttributes,
                             &Filename, 
                             0,
                             NULL,
                             NULL);
  Status = ZwFileOpen(&FileHandle, 0, &FileObjectAttributes, NULL, 0, 0);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  /*  Read first block of image to determine type  */
  Status = ZwReadFile(FileHandle, 0, 0, 0, 0, BlockBuffer, 512, 0, 0);
  if (!NT_SUCCESS(Status))
    {
      ZwClose(FileHandle);
      return Status;
    }    

  /*  If MZ header exists  */
  PEDosHeader = (PIMAGE_DOS_HEADER) BlockBuffer;
  if (PEDosHeader->e_magic == 0x54AD)
    {
      /*  FIXME: if PE header exists  */
          /* FIXME: load PE image  */
      /*  FIXME: else  */
          /* FIXME: load MZ image  */
    }
  else /*  Assume bin format and load  */
 /* FIXME: could check for a.out, ELF, COFF, etc. images here... */
    {
      Status = ZwCreateSection(&SectionHandle,
                               SECTION_ALL_ACCESS,
                               NULL,
                               NULL,
                               PAGE_READWRITE,
                               MEM_COMMIT,
                               FileHandle);
      ZwClose(FileHandle);
      if (!NT_SUCCESS(Status))
        {
          return Status;
        }

      BaseAddress = (PVOID)0x10000;
      SectionOffset.HighPart = 0;
      SectionOffset.LowPart = 0;

      /*  FIXME: get the size of the file  */
      Size = 0x8000;

      ZwMapViewOfSection(SectionHandle,
                         ProcessHandle,
                         &BaseAddress,
                         0,
                         0x8000,
                         &SectionOffset,
                         &Size,
                         0,
                         MEM_COMMIT,
                         PAGE_READWRITE);
   
      memset(&Context,0,sizeof(CONTEXT));
   
      Context.SegSs = USER_DS;
      Context.Esp = 0x2000;
      Context.EFlags = 0x202;
      Context.SegCs = USER_CS;
      Context.Eip = 0x10000;
      Context.SegDs = USER_DS;
      Context.SegEs = USER_DS;
      Context.SegFs = USER_DS;
      Context.SegGs = USER_DS;
   
      BaseAddress = 0x1000;
      StackSize = 0x1000;
      ZwAllocateVirtualMemory(ProcessHandle,
                              &BaseAddress,
                              0,
                              &StackSize,
                              MEM_COMMIT,
                              PAGE_READWRITE);
      ZwCreateThread(&ThreadHandle,
                     THREAD_ALL_ACCESS,
                     NULL,
                     ShellHandle,
                     NULL,
                     &Context,
                     NULL,
                     FALSE);
    }

  /*  FIXME: should DLLs be named sections?  */
  /*  FIXME: get current process and associate with section  */

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

