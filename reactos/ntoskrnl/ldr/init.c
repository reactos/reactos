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
   PIMAGE_NT_HEADERS NTHeaders;
   PPEB Peb;
   PEPROCESS Process;
   CONTEXT Context;
   HANDLE ThreadHandle;
   INITIAL_TEB InitialTeb;
   
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
   InitialTeb.StackReserve = NTHeaders->OptionalHeader.SizeOfStackReserve;
   InitialTeb.StackCommit = NTHeaders->OptionalHeader.SizeOfStackCommit;
   /* add guard page size */
   InitialTeb.StackCommit += PAGESIZE;
   DPRINT("StackReserve 0x%lX  StackCommit 0x%lX\n",
	  InitialTeb.StackReserve, InitialTeb.StackCommit);
   KeDetachProcess();
   DPRINT("Dereferencing process\n");
//   ObDereferenceObject(Process);
   
  DPRINT("Allocating stack\n");
  InitialTeb.StackAllocate = NULL;
  Status = ZwAllocateVirtualMemory(ProcessHandle,
				   &InitialTeb.StackAllocate,
				   0,
				   &InitialTeb.StackReserve,
				   MEM_COMMIT, // MEM_RESERVE,
				   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Stack allocation failed (Status %x)", Status);
      return(Status);
    }

  DPRINT("StackAllocate: %p ReserveSize: 0x%lX\n",
	 InitialTeb.StackAllocate, InitialTeb.StackReserve);

  InitialTeb.StackBase = (PVOID)((ULONG)InitialTeb.StackAllocate + InitialTeb.StackReserve);
  InitialTeb.StackLimit = (PVOID)((ULONG)InitialTeb.StackBase - InitialTeb.StackCommit);

  DPRINT("StackBase: %p  StackCommit: 0x%lX\n",
	 InitialTeb.StackBase, InitialTeb.StackCommit);
#if 0
  /* Commit stack */
  Status = NtAllocateVirtualMemory(ProcessHandle,
				   &InitialTeb.StackLimit,
				   0,
				   &InitialTeb.StackCommit,
				   MEM_COMMIT,
				   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      /* release the stack space */
      NtFreeVirtualMemory(ProcessHandle,
			  InitialTeb.StackAllocate,
			  &InitialTeb.StackReserve,
			  MEM_RELEASE);

      DPRINT("Error comitting stack page!\n");
      return(Status);
    }

  DPRINT("StackLimit: %p\nStackCommit: 0x%lX\n",
         InitialTeb.StackLimit,
         InitialTeb.StackCommit);

  /* Protect guard page */
  Status = NtProtectVirtualMemory(ProcessHandle,
				  InitialTeb.StackLimit,
				  PAGESIZE,
				  PAGE_GUARD | PAGE_READWRITE,
				  &OldPageProtection);
  if (!NT_SUCCESS(Status))
    {
      /* release the stack space */
      NtFreeVirtualMemory(ProcessHandle,
			  InitialTeb.StackAllocate,
			  &InitialTeb.StackReserve,
			  MEM_RELEASE);

      DPRINT("Error protecting guard page!\n");
      return(Status);
    }
#endif
   
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
   Context.Eip = (ULONG)LdrStartupAddr;
   Context.SegCs = USER_CS;
   Context.SegDs = USER_DS;
   Context.SegEs = USER_DS;
   Context.SegFs = TEB_SELECTOR;
   Context.SegGs = USER_DS;
   Context.SegSs = USER_DS;
   Context.EFlags = 0x202;
   Context.Esp = (ULONG)InitialTeb.StackBase - 20;

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
			   &InitialTeb,
			   FALSE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Thread creation failed (Status %x)\n", Status);

      NtFreeVirtualMemory(ProcessHandle,
			  InitialTeb.StackAllocate,
			  &InitialTeb.StackReserve,
			  MEM_RELEASE);

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
