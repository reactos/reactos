/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/loader.c
 * PURPOSE:         Loaders for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 * UPDATE HISTORY:
 *   DW   22/05/98  Created
 *   RJJ  10/12/98  Completed image loader function and added hooks for MZ/PE
 *   RJJ  10/12/98  Built driver loader function and added hooks for PE/COFF
 *   RJJ  10/12/98  Rolled in David's code to load COFF drivers
 *   JM   14/12/98  Built initial PE user module loader
 *   RJJ  06/03/99  Moved user PE loader into NTDLL
 */

/* INCLUDES *****************************************************************/

#include <windows.h>

#include <internal/i386/segment.h>
#include <internal/linkage.h>
#include <internal/module.h>
#include <internal/ntoskrnl.h>
#include <internal/ob.h>
#include <internal/ps.h>
#include <string.h>
#include <internal/string.h>
#include <internal/symbol.h>
#include <internal/teb.h>
#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

/*   LdrLoadImage
 * FUNCTION:
 *   Builds the initial environment for a process.  Should be used
 *   to load the initial user process.
 * ARGUMENTS:
 *   HANDLE   ProcessHandle  handle of the process to load the module into
 *   PUNICODE_STRING  Filename  name of the module to load
 * RETURNS: 
 *   NTSTATUS
 */

#define STACK_TOP (0xb0000000)

static NTSTATUS LdrCreatePeb(HANDLE ProcessHandle)
{
   NTSTATUS Status;
   PVOID PebBase;
   ULONG PebSize;
   NT_PEB Peb;
   ULONG BytesWritten;
   
   PebBase = (PVOID)PEB_BASE;
   PebSize = 0x1000;
   Status = ZwAllocateVirtualMemory(ProcessHandle,
				    &PebBase,
				    0,
				    &PebSize,
				    MEM_COMMIT,
				    PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   
   memset(&Peb, 0, sizeof(Peb));
   Peb.StartupInfo = (PPROCESSINFOW)PEB_STARTUPINFO;

   ZwWriteVirtualMemory(ProcessHandle,
			(PVOID)PEB_BASE,
			&Peb,
			sizeof(Peb),
			&BytesWritten);
      
   return(STATUS_SUCCESS);
}

NTSTATUS LdrLoadImage(HANDLE ProcessHandle, PUNICODE_STRING Filename)
{
   char BlockBuffer[1024];
   DWORD ImageBase, LdrStartupAddr, StackBase;
   ULONG ImageSize, StackSize;
   NTSTATUS Status;
   OBJECT_ATTRIBUTES FileObjectAttributes;
   HANDLE FileHandle, SectionHandle, NTDllSectionHandle, ThreadHandle;
   HANDLE DupNTDllSectionHandle;
   CONTEXT Context;
   UNICODE_STRING DllPathname;
   PIMAGE_DOS_HEADER DosHeader;
   PIMAGE_NT_HEADERS NTHeaders;
   ULONG BytesWritten;
   ULONG InitialViewSize;
   ULONG i;
   HANDLE DupSectionHandle;
   
    /*  Locate and open NTDLL to determine ImageBase and LdrStartup  */
   RtlInitUnicodeString(&DllPathname,L"\\??\\C:\\reactos\\system\\ntdll.dll"); 
   InitializeObjectAttributes(&FileObjectAttributes,
			      &DllPathname, 
			      0,
			      NULL,
			      NULL);
   DPRINT("Opening NTDLL\n");
   Status = ZwOpenFile(&FileHandle, 
		       FILE_ALL_ACCESS, 
		       &FileObjectAttributes, 
		       NULL, 
		       0, 
		       0);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NTDLL open failed ");
	DbgPrintErrorMessage(Status);
	
	return Status;
     }
  Status = ZwReadFile(FileHandle, 0, 0, 0, 0, BlockBuffer, 1024, 0, 0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NTDLL header read failed ");
      DbgPrintErrorMessage(Status);
      ZwClose(FileHandle);

      return Status;
    }    
    /* FIXME: this will fail if the NT headers are more than 1024 bytes from start */
  DosHeader = (PIMAGE_DOS_HEADER) BlockBuffer;
  if (DosHeader->e_magic != IMAGE_DOS_MAGIC || 
      DosHeader->e_lfanew == 0L ||
      *(PULONG)((PUCHAR)BlockBuffer + DosHeader->e_lfanew) != IMAGE_PE_MAGIC)
    {
      DPRINT("NTDLL format invalid\n");
      ZwClose(FileHandle);

      return STATUS_UNSUCCESSFUL;
    }
  NTHeaders = (PIMAGE_NT_HEADERS)(BlockBuffer + DosHeader->e_lfanew);
   ImageBase = NTHeaders->OptionalHeader.ImageBase;
  ImageSize = NTHeaders->OptionalHeader.SizeOfImage;
    /* FIXME: retrieve the offset of LdrStartup from NTDLL  */
   DPRINT("ImageBase %x\n",ImageBase);
  LdrStartupAddr = ImageBase + NTHeaders->OptionalHeader.AddressOfEntryPoint;

    /* Create a section for NTDLL */
  Status = ZwCreateSection(&NTDllSectionHandle,
                           SECTION_ALL_ACCESS,
                           NULL,
                           NULL,
                           PAGE_READWRITE,
                           MEM_COMMIT,
                           FileHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NTDLL create section failed ");
      DbgPrintErrorMessage(Status);
      ZwClose(FileHandle);

      return Status;
    }

  /*  Map the NTDLL into the process  */
   InitialViewSize = DosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS) 
     + sizeof(IMAGE_SECTION_HEADER) * NTHeaders->FileHeader.NumberOfSections;
  Status = ZwMapViewOfSection(NTDllSectionHandle,
                              ProcessHandle,
                              (PVOID *)&ImageBase,
                              0,
                              InitialViewSize,
                              NULL,
                              &InitialViewSize,
                              0,
                              MEM_COMMIT,
                              PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NTDLL map view of secion failed ");
      DbgPrintErrorMessage(Status);

      /* FIXME: destroy the section here  */

      ZwClose(FileHandle);

      return Status;
    }
   for (i=0; i<NTHeaders->FileHeader.NumberOfSections; i++)
     {
	PIMAGE_SECTION_HEADER Sections;
	LARGE_INTEGER Offset;
	ULONG Base;
	
	Sections = (PIMAGE_SECTION_HEADER)SECHDROFFSET(BlockBuffer);
	Base = Sections[i].VirtualAddress + ImageBase;
        Offset.u.LowPart = Sections[i].PointerToRawData;
        Offset.u.HighPart = 0;
	Status = ZwMapViewOfSection(NTDllSectionHandle,
				    ProcessHandle,
				    (PVOID *)&Base,
				    0,
				    Sections[i].Misc.VirtualSize,
				    &Offset,
				    (PULONG)&Sections[i].Misc.VirtualSize,
				    0,
				    MEM_COMMIT,
				    PAGE_READWRITE);
	if (!NT_SUCCESS(Status))
	  {
	     DPRINT("NTDLL map view of secion failed ");
	     DbgPrintErrorMessage(Status);

	     /* FIXME: destroy the section here  */
	     
	     ZwClose(FileHandle);
	     return Status;
	  }
     }
  ZwClose(FileHandle);

    /*  Open process image to determine ImageBase and StackBase/Size  */
  InitializeObjectAttributes(&FileObjectAttributes,
                             Filename, 
                             0,
                             NULL,
                             NULL);
   DPRINT("Opening image file %w\n",FileObjectAttributes.ObjectName->Buffer);
  Status = ZwOpenFile(&FileHandle, FILE_ALL_ACCESS, &FileObjectAttributes, 
		      NULL, 0, 0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Image open failed ");
      DbgPrintErrorMessage(Status);

      return Status;
    }
  Status = ZwReadFile(FileHandle, 0, 0, 0, 0, BlockBuffer, 1024, 0, 0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Image header read failed ");
      DbgPrintErrorMessage(Status);
      ZwClose(FileHandle);

      return Status;
    }    

  /* FIXME: this will fail if the NT headers are more than 1024 bytes from start */

  DosHeader = (PIMAGE_DOS_HEADER) BlockBuffer;
  if (DosHeader->e_magic != IMAGE_DOS_MAGIC || 
      DosHeader->e_lfanew == 0L ||
      *(PULONG)((PUCHAR)BlockBuffer + DosHeader->e_lfanew) != IMAGE_PE_MAGIC)
    {
      DPRINT("Image invalid format rc=%08lx\n", Status);
      ZwClose(FileHandle);

      return STATUS_UNSUCCESSFUL;
    }
  NTHeaders = (PIMAGE_NT_HEADERS)(BlockBuffer + DosHeader->e_lfanew);
  ImageBase = NTHeaders->OptionalHeader.ImageBase;
  ImageSize = NTHeaders->OptionalHeader.SizeOfImage;

    /* Create a section for the image */
  Status = ZwCreateSection(&SectionHandle,
                           SECTION_ALL_ACCESS,
                           NULL,
                           NULL,
                           PAGE_READWRITE,
                           MEM_COMMIT,
                           FileHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Image create section failed ");
      DbgPrintErrorMessage(Status);
      ZwClose(FileHandle);

      return Status;
    }

    /*  Map the image into the process  */
   InitialViewSize = DosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS) 
     + sizeof(IMAGE_SECTION_HEADER) * NTHeaders->FileHeader.NumberOfSections;
   DPRINT("InitialViewSize %x\n",InitialViewSize);
   Status = ZwMapViewOfSection(SectionHandle,
			       ProcessHandle,
			       (PVOID *)&ImageBase,
			       0,
			       InitialViewSize,
			       NULL,
			       &InitialViewSize,
			       0,
			       MEM_COMMIT,
			       PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Image map view of section failed ");
      DbgPrintErrorMessage(Status);

      /* FIXME: destroy the section here  */

      ZwClose(FileHandle);

      return Status;
    }
  ZwClose(FileHandle);

    /*  Create page backed section for stack  */
  StackBase = (STACK_TOP - NTHeaders->OptionalHeader.SizeOfStackReserve);
  StackSize = NTHeaders->OptionalHeader.SizeOfStackReserve;
  Status = ZwAllocateVirtualMemory(ProcessHandle,
                                   (PVOID *)&StackBase,
                                   0,
                                   &StackSize,
                                   MEM_COMMIT,
                                   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Stack allocation failed ");
      DbgPrintErrorMessage(Status);

      /* FIXME: unmap the section here  */
      /* FIXME: destroy the section here  */

      return Status;
    }
   
   ZwDuplicateObject(NtCurrentProcess(),
		     &SectionHandle,
		     ProcessHandle,
		     &DupSectionHandle,
		     0,
		     FALSE,
		     DUPLICATE_SAME_ACCESS);
   ZwDuplicateObject(NtCurrentProcess(),
		     &NTDllSectionHandle,
		     ProcessHandle,
		     &DupNTDllSectionHandle,
		     0,
		     FALSE,
		     DUPLICATE_SAME_ACCESS);
   
   ZwWriteVirtualMemory(ProcessHandle,
			(PVOID)(STACK_TOP - 4),
			&DupNTDllSectionHandle,
			sizeof(DupNTDllSectionHandle),
			&BytesWritten);
   ZwWriteVirtualMemory(ProcessHandle,
			(PVOID)(STACK_TOP - 8),
			&ImageBase,
			sizeof(ImageBase),
			&BytesWritten);
   ZwWriteVirtualMemory(ProcessHandle,
			(PVOID)(STACK_TOP - 12),
			&DupSectionHandle,
			sizeof(DupSectionHandle),
			&BytesWritten);
   
   
   /*
    * Create a peb (grungy)
    */ 
   Status = LdrCreatePeb(ProcessHandle);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("LDR: Failed to create initial peb\n");
	return(Status);
     }
   
    /*  Initialize context to point to LdrStartup  */
  memset(&Context,0,sizeof(CONTEXT));
  Context.SegSs = USER_DS;
  Context.Esp = STACK_TOP - 16;
  Context.EFlags = 0x202;
  Context.SegCs = USER_CS;
  Context.Eip = LdrStartupAddr;
  Context.SegDs = USER_DS;
  Context.SegEs = USER_DS;
  Context.SegFs = USER_DS;
  Context.SegGs = USER_DS;
   
   DPRINT("LdrStartupAddr %x\n",LdrStartupAddr);
  /* FIXME: Create process and let 'er rip  */
  Status = ZwCreateThread(&ThreadHandle,
                          THREAD_ALL_ACCESS,
                          NULL,
                          ProcessHandle,
                          NULL,
                          &Context,
                          NULL,
                          FALSE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Thread creation failed ");
      DbgPrintErrorMessage(Status);

      /* FIXME: destroy the stack memory block here  */
      /* FIXME: unmap the section here  */
      /* FIXME: destroy the section here  */

      return Status;
    }

  return STATUS_SUCCESS;
}

NTSTATUS LdrLoadInitialProcess(VOID)
/*
 * FIXME: The location of the initial process should be configurable,
 * from command line or registry
 */
{
  NTSTATUS Status;
  HANDLE ProcessHandle;
  UNICODE_STRING ProcessName;

  Status = ZwCreateProcess(&ProcessHandle,
                           PROCESS_ALL_ACCESS,
                           NULL,
                           SystemProcessHandle,
                           FALSE,
                           NULL,
                           NULL,
                           NULL);
  if (!NT_SUCCESS(Status))
    {
       DbgPrint("Could not create process\n");
       return Status;
    }

  RtlInitUnicodeString(&ProcessName, L"\\??\\C:\\reactos\\system\\shell.exe");
  Status = LdrLoadImage(ProcessHandle, &ProcessName);
   
  return Status;
}
