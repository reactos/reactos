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
 *   EA   19990717  LdrGetSystemDirectory()
 *   EK   20000618  Using SystemRoot link instead of LdrGetSystemDirectory()
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/i386/segment.h>
#include <internal/module.h>
#include <internal/ntoskrnl.h>
#include <internal/ob.h>
#include <internal/ps.h>
#include <internal/ldr.h>
#include <napi/teb.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * FIXME: The location of the initial process should be configurable,
 * from command line or registry
 */
NTSTATUS LdrLoadInitialProcess (VOID)
{
   NTSTATUS Status;
   HANDLE ProcessHandle;
   UNICODE_STRING ProcessName;
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE FileHandle;
   HANDLE SectionHandle;
   PVOID LdrStartupAddr;
   PVOID StackBase;
   ULONG StackSize;
   PIMAGE_NT_HEADERS NTHeaders;
   PPEB Peb;
   PEPROCESS Process;
   CONTEXT Context;
   HANDLE ThreadHandle;
   
   /*
    * Get the absolute path to smss.exe using the
    * SystemRoot link.
    */
   RtlInitUnicodeString(&ProcessName,
			L"\\SystemRoot\\system32\\smss.exe");
   
   /*
    * Open process image to determine ImageBase
    * and StackBase/Size.
    */
   InitializeObjectAttributes(&ObjectAttributes,
			      &ProcessName,
			      0,
			      NULL,
			      NULL);
   DPRINT("Opening image file %S\n", ObjectAttributes.ObjectName->Buffer);
   Status = ZwOpenFile(&FileHandle,
		       FILE_ALL_ACCESS,
		       &ObjectAttributes,
		       NULL,
		       0,
		       0);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("Image open failed (Status was %x)\n", Status);
	return Status;
     }
   
   /*
    * Create a section for the image
    */
   DPRINT("Creating section\n");
   Status = ZwCreateSection(&SectionHandle,
			    SECTION_ALL_ACCESS,
			    NULL,
			    NULL,
			    PAGE_READWRITE,
			    SEC_COMMIT | SEC_IMAGE,
			    FileHandle);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("ZwCreateSection failed (Status %x)\n", Status);
	ZwClose(FileHandle);
	return(Status);
     }
   ZwClose(FileHandle);
   
   DPRINT("Creating process\n");
   Status = ZwCreateProcess(&ProcessHandle,
			    PROCESS_ALL_ACCESS,
			    NULL,
			    SystemProcessHandle,
			    FALSE,
			    SectionHandle,
			    NULL,
			    NULL);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("Could not create process\n");
	return Status;
     }

   /*
    * Create initial stack and thread
    */
   
   /*
    * Aargh!
    */
   LdrStartupAddr = (PVOID)LdrpGetSystemDllEntryPoint();
   DPRINT("LdrStartupAddr %x\n", LdrStartupAddr);
   
   /*
    * Create page backed section for stack
    */
   DPRINT("Allocating stack\n");
   
   DPRINT("Referencing process\n");
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_ALL_ACCESS,
				      PsProcessType,
				      KernelMode,
				      (PVOID*)&Process,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("ObReferenceObjectByProcess() failed (Status %x)\n", Status);
	return(Status);
     }
   
   DPRINT("Attaching to process\n");
   KeAttachProcess(Process);
   Peb = (PPEB)PEB_BASE;
   DPRINT("Peb %x\n", Peb);
   DPRINT("CurrentProcess %x Peb->ImageBaseAddress %x\n", 
	  PsGetCurrentProcess(),
	  Peb->ImageBaseAddress);
   NTHeaders = RtlImageNtHeader(Peb->ImageBaseAddress);
   DPRINT("NTHeaders %x\n", NTHeaders);
   StackSize = NTHeaders->OptionalHeader.SizeOfStackReserve;
   DPRINT("StackSize %x\n", StackSize);
   KeDetachProcess();
   DPRINT("Dereferencing process\n");
//   ObDereferenceObject(Process);
   
   StackBase = (PVOID)NULL;
   DPRINT("StackBase %x StackSize %x\n", StackBase, StackSize);
   DPRINT("Allocating virtual memory\n");
   Status = ZwAllocateVirtualMemory(ProcessHandle,
				    (PVOID*)&StackBase,
				    0,
				    &StackSize,
				    MEM_COMMIT,
				    PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("Stack allocation failed (Status %x)", Status);
	return Status;
     }
   
   DPRINT("Attaching to process\n");
   KeAttachProcess(Process);
   Peb = (PPEB)PEB_BASE;
   DPRINT("Peb %x\n", Peb);
   DPRINT("CurrentProcess %x Peb->ImageBaseAddress %x\n", 
	  PsGetCurrentProcess(),
	  Peb->ImageBaseAddress);
   KeDetachProcess();

   /*
    * Initialize context to point to LdrStartup
    */
   memset(&Context,0,sizeof(CONTEXT));
   Context.SegSs = USER_DS;
   Context.Esp = (ULONG)StackBase + StackSize - 20;
   Context.EFlags = 0x202;
   Context.SegCs = USER_CS;
   Context.Eip = (ULONG)LdrStartupAddr;
   Context.SegDs = USER_DS;
   Context.SegEs = USER_DS;
   Context.SegFs = TEB_SELECTOR;
   Context.SegGs = USER_DS;

   DPRINT("LdrStartupAddr %x\n",LdrStartupAddr);
   
   /*
    * FIXME: Create process and let 'er rip
    */
   DPRINT("Creating thread for initial process\n");
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
	DPRINT("Thread creation failed (Status %x)\n", Status);
	
	/* FIXME: destroy the stack memory block here  */
	/* FIXME: unmap the section here  */
	/* FIXME: destroy the section here  */
	
	return Status;
     }
   
   DPRINT("Attaching to process\n");
   KeAttachProcess(Process);
   Peb = (PPEB)PEB_BASE;
   DPRINT("Peb %x\n", Peb);
   DPRINT("CurrentProcess %x Peb->ImageBaseAddress %x\n", 
	  PsGetCurrentProcess(),
	  Peb->ImageBaseAddress);
   KeDetachProcess();
   
   return STATUS_SUCCESS;
}

/* EOF */
