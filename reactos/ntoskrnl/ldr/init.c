/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002 ReactOS Team
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
/* $Id: init.c,v 1.50 2004/12/05 15:42:42 weiden Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/init.c
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
 *   EK   20021119  Create a process parameter block for the initial process.
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>


/* MACROS ******************************************************************/

#define DENORMALIZE(x,addr) {if(x) x=(VOID*)((ULONG)(x)-(ULONG)(addr));}
#define ALIGN(x,align)      (((ULONG)(x)+(align)-1UL)&(~((align)-1UL)))


/* FUNCTIONS *****************************************************************/

static NTSTATUS
LdrpMapProcessImage(PHANDLE SectionHandle,
		    PUNICODE_STRING ImagePath)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE FileHandle;
  NTSTATUS Status;

  /* Open image file */
  InitializeObjectAttributes(&ObjectAttributes,
			     ImagePath,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  DPRINT("Opening image file %S\n", ObjectAttributes.ObjectName->Buffer);
  Status = NtOpenFile(&FileHandle,
		      FILE_ALL_ACCESS,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      0,
		      FILE_SYNCHRONOUS_IO_ALERT);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtOpenFile() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Create a section for the image */
  DPRINT("Creating section\n");
  Status = NtCreateSection(SectionHandle,
			   SECTION_ALL_ACCESS,
			   NULL,
			   NULL,
			   PAGE_READWRITE,
			   SEC_COMMIT | SEC_IMAGE,
			   FileHandle);
  NtClose(FileHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtCreateSection() failed (Status %lx)\n", Status);
    }

  return(Status);
}


static NTSTATUS
LdrpCreateProcessEnvironment(HANDLE ProcessHandle,
			     PUNICODE_STRING ImagePath,
			     PVOID* ImageBaseAddress)
{
  PRTL_USER_PROCESS_PARAMETERS LocalPpb;
  PRTL_USER_PROCESS_PARAMETERS ProcessPpb;
  ULONG BytesWritten;
  ULONG Offset;
  ULONG Size;
  ULONG RegionSize;
  NTSTATUS Status;

  /* Calculate the PPB size */
  Size = sizeof(RTL_USER_PROCESS_PARAMETERS);
  Size += ALIGN(ImagePath->Length + sizeof(WCHAR), sizeof(ULONG));
  RegionSize = ROUND_UP(Size, PAGE_SIZE);
  DPRINT("Size %lu  RegionSize %lu\n", Size, RegionSize);

  /* Allocate the local PPB */
  LocalPpb = NULL;
  Status = NtAllocateVirtualMemory(NtCurrentProcess(),
				   (PVOID*)&LocalPpb,
				   0,
				   &RegionSize,
				   MEM_RESERVE | MEM_COMMIT,
				   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtAllocateVirtualMemory() failed (Status %lx)\n", Status);
      return(Status);
    }

  DPRINT("LocalPpb %p  AllocationSize %lu\n", LocalPpb, RegionSize);

  /* Initialize the local PPB */
  RtlZeroMemory(LocalPpb,
		RegionSize);
  LocalPpb->AllocationSize = RegionSize;
  LocalPpb->Size = Size;
  LocalPpb->ImagePathName.Length = ImagePath->Length;
  LocalPpb->ImagePathName.MaximumLength = ImagePath->Length + sizeof(WCHAR);
  LocalPpb->ImagePathName.Buffer = (PWCHAR)(LocalPpb + 1);

  /* Copy image path */
  RtlCopyMemory(LocalPpb->ImagePathName.Buffer,
		ImagePath->Buffer,
		ImagePath->Length);
  LocalPpb->ImagePathName.Buffer[ImagePath->Length / sizeof(WCHAR)] = L'\0';

  /* Denormalize the process parameter block */
  DENORMALIZE(LocalPpb->ImagePathName.Buffer, LocalPpb);
  LocalPpb->Flags &= ~PPF_NORMALIZED;

  /* Create the process PPB */
  ProcessPpb = NULL;
  Status = NtAllocateVirtualMemory(ProcessHandle,
				   (PVOID*)&ProcessPpb,
				   0,
				   &RegionSize,
				   MEM_RESERVE | MEM_COMMIT,
				   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtAllocateVirtualMemory() failed (Status %lx)\n", Status);

      /* Release the local PPB */
      RegionSize = 0;
      NtFreeVirtualMemory(NtCurrentProcess(),
			  (PVOID*)&LocalPpb,
			  &RegionSize,
			  MEM_RELEASE);
      return(Status);
    }

  /* Copy local PPB into the process PPB */
  NtWriteVirtualMemory(ProcessHandle,
		       ProcessPpb,
		       LocalPpb,
		       LocalPpb->AllocationSize,
		       &BytesWritten);

  /* Update pointer to process PPB in the process PEB */
  Offset = FIELD_OFFSET(PEB, ProcessParameters);
  NtWriteVirtualMemory(ProcessHandle,
		       (PVOID)(PEB_BASE + Offset),
		       &ProcessPpb,
		       sizeof(ProcessPpb),
		       &BytesWritten);

  /* Release local PPB */
  RegionSize = 0;
  NtFreeVirtualMemory(NtCurrentProcess(),
		      (PVOID*)&LocalPpb,
		      &RegionSize,
		      MEM_RELEASE);

  /* Read image base address. */
  Offset = FIELD_OFFSET(PEB, ImageBaseAddress);
  NtReadVirtualMemory(ProcessHandle,
		      (PVOID)(PEB_BASE + Offset),
		      ImageBaseAddress,
		      sizeof(PVOID),
		      &BytesWritten);

  return(STATUS_SUCCESS);
}

/*
 FIXME: this sucks. Sucks sucks sucks. This code was duplicated, if you can
 believe it, in four different places - excluding this, and twice in the two
 DLLs that contained it (kernel32.dll and ntdll.dll). As much as I'd like to
 rip the whole RTL out of ntdll.dll and ntoskrnl.exe and into its own static
 library, ntoskrnl.exe is built separatedly from the rest of ReactOS, coming
 with its own linker scripts and specifications, and, save for changes and fixes
 to make it at least compile, I'm not going to touch any of it. If you feel
 brave enough, you're welcome [KJK::Hyperion]
*/
static NTSTATUS LdrpCreateStack
(
 HANDLE ProcessHandle,
 PINITIAL_TEB InitialTeb,
 PULONG_PTR StackReserve,
 PULONG_PTR StackCommit
)
{
 PVOID pStackLowest = NULL;
 ULONG_PTR nSize = 0;
 NTSTATUS nErrCode;

 if(StackReserve == NULL || StackCommit == NULL)
  return STATUS_INVALID_PARAMETER;

 /* FIXME: no SEH, no guard pages */
 *StackCommit = *StackReserve;

 InitialTeb->StackBase = NULL;
 InitialTeb->StackLimit =  NULL;
 InitialTeb->StackCommit = NULL;
 InitialTeb->StackCommitMax = NULL;
 InitialTeb->StackReserved = NULL;

 /* FIXME: this code assumes a stack growing downwards */
 /* fixed stack */
 if(*StackCommit == *StackReserve)
 {
  DPRINT("Fixed stack\n");

  InitialTeb->StackLimit = NULL;

  /* allocate the stack */
  nErrCode = NtAllocateVirtualMemory
  (
   ProcessHandle,
   &(InitialTeb->StackLimit),
   0,
   StackReserve,
   MEM_RESERVE | MEM_COMMIT,
   PAGE_READWRITE
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) return nErrCode;

  /* store the highest (first) address of the stack */
  InitialTeb->StackBase =
   (PUCHAR)(InitialTeb->StackLimit) + *StackReserve;
 }
 /* expandable stack */
 else
 {
  ULONG_PTR nGuardSize = PAGE_SIZE;
  PVOID pGuardBase;

  DPRINT("Expandable stack\n");

  InitialTeb->StackLimit = NULL;
  InitialTeb->StackBase = NULL;
  InitialTeb->StackReserved = NULL;

  /* reserve the stack */
  nErrCode = NtAllocateVirtualMemory
  (
   ProcessHandle,
   &(InitialTeb->StackReserved),
   0,
   StackReserve,
   MEM_RESERVE,
   PAGE_READWRITE
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) return nErrCode;

  DPRINT("Reserved %08X bytes\n", *StackReserve);

  /* expandable stack base - the highest address of the stack */
  InitialTeb->StackCommit =
   (PUCHAR)(InitialTeb->StackReserved) + *StackReserve;

  /* expandable stack limit - the lowest committed address of the stack */
  InitialTeb->StackCommitMax =
   (PUCHAR)(InitialTeb->StackCommit) - *StackCommit;

  DPRINT("Stack commit     %p\n", InitialTeb->StackCommit);
  DPRINT("Stack commit max %p\n", InitialTeb->StackCommitMax);

  /* commit as much stack as requested */
  nErrCode = NtAllocateVirtualMemory
  (
   ProcessHandle,
   &(InitialTeb->StackCommitMax),
   0,
   StackCommit,
   MEM_COMMIT,
   PAGE_READWRITE
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) goto l_Cleanup;

  DPRINT("Stack commit max %p\n", InitialTeb->StackCommitMax);

  pGuardBase = (PUCHAR)(InitialTeb->StackCommitMax) - PAGE_SIZE;

  DPRINT("Guard base %p\n", InitialTeb->StackCommit);

  /* set up the guard page */
  nErrCode = NtAllocateVirtualMemory
  (
   ProcessHandle,
   &pGuardBase,
   0,
   &nGuardSize,
   MEM_COMMIT,
   PAGE_READWRITE | PAGE_GUARD
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) goto l_Cleanup;

  DPRINT("Guard base %p\n", InitialTeb->StackCommit);
 }

 return STATUS_SUCCESS;

 /* cleanup in case of failure */
l_Cleanup:
  if(InitialTeb->StackLimit)
  pStackLowest = InitialTeb->StackLimit;
 else if(InitialTeb->StackReserved)
  pStackLowest = InitialTeb->StackReserved;

 /* free the stack, if it was allocated */
 if(pStackLowest != NULL)
  NtFreeVirtualMemory(ProcessHandle, &pStackLowest, &nSize, MEM_RELEASE);

 return nErrCode;
}


NTSTATUS INIT_FUNCTION
LdrLoadInitialProcess(PHANDLE ProcessHandle,
		      PHANDLE ThreadHandle)
{
  SECTION_IMAGE_INFORMATION Sii;
  UNICODE_STRING ImagePath;
  HANDLE SectionHandle;
  CONTEXT Context;
  INITIAL_TEB InitialTeb;
  ULONG_PTR nStackReserve = 0;
  ULONG_PTR nStackCommit = 0;
  PVOID pStackLowest;
  PVOID pStackBase;
  ULONG ResultLength;
  PVOID ImageBaseAddress;
  ULONG InitialStack[5];
  HANDLE SystemProcessHandle;
  NTSTATUS Status;

  /* Get the absolute path to smss.exe. */
  RtlRosInitUnicodeStringFromLiteral(&ImagePath,
				  L"\\SystemRoot\\system32\\smss.exe");

  /* Map process image */
  Status = LdrpMapProcessImage(&SectionHandle,
			       &ImagePath);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("LdrpMapImage() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Get information about the process image. */
   Status = NtQuerySection(SectionHandle,
			   SectionImageInformation,
			   &Sii,
			   sizeof(Sii),
			   &ResultLength);
  if (!NT_SUCCESS(Status) || ResultLength != sizeof(Sii))
    {
      DPRINT("ZwQuerySection failed (Status %X)\n", Status);
      NtClose(ProcessHandle);
      NtClose(SectionHandle);
      return(Status);
    }

  Status = ObCreateHandle(PsGetCurrentProcess(),
                          PsInitialSystemProcess,
                          PROCESS_CREATE_PROCESS | PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION,
                          FALSE,
                          &SystemProcessHandle);
  if(!NT_SUCCESS(Status))
  {
    DPRINT1("Failed to create a handle for the system process!\n");
    return Status;
  }

  DPRINT("Creating process\n");
  Status = NtCreateProcess(ProcessHandle,
			   PROCESS_ALL_ACCESS,
			   NULL,
			   SystemProcessHandle,
			   FALSE,
			   SectionHandle,
			   NULL,
			   NULL);
  NtClose(SectionHandle);
  NtClose(SystemProcessHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtCreateProcess() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Create process environment */
  DPRINT("Creating the process environment\n");
  Status = LdrpCreateProcessEnvironment(*ProcessHandle,
					&ImagePath,
					&ImageBaseAddress);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("LdrpCreateProcessEnvironment() failed (Status %lx)\n", Status);
      NtClose(*ProcessHandle);
      return(Status);
    }
  DPRINT("ImageBaseAddress: %p\n", ImageBaseAddress);


  /* Calculate initial stack sizes */
  if (Sii.StackReserve > 0x100000)
    nStackReserve = Sii.StackReserve;
  else
    nStackReserve = 0x100000; /* 1MByte */

  /* FIXME */
#if 0
  if (Sii.StackCommit > PAGE_SIZE)
    nStackCommit =  Sii.StackCommit;
  else
    nStackCommit = PAGE_SIZE;
#endif
  nStackCommit = nStackReserve - PAGE_SIZE;

  DPRINT("StackReserve 0x%lX  StackCommit 0x%lX\n",
	 nStackReserve, nStackCommit);


  /* Create the process stack */
  Status = LdrpCreateStack
  (
   *ProcessHandle,
   &InitialTeb,
   &nStackReserve,
   &nStackCommit
  );

  if (!NT_SUCCESS(Status))
    {
      DPRINT("Failed to write initial stack.\n");
      NtClose(ProcessHandle);
      return(Status);
    }

  if(InitialTeb.StackBase && InitialTeb.StackLimit)
  {
   pStackBase = InitialTeb.StackBase;
   pStackLowest = InitialTeb.StackLimit;
  }
  else
  {
   pStackBase = InitialTeb.StackCommit;
   pStackLowest = InitialTeb.StackReserved;
  }

  DPRINT("pStackBase = %p\n", pStackBase);
  DPRINT("pStackLowest = %p\n", pStackLowest);

  /*
   * Initialize context to point to LdrStartup
   */
#if defined(_M_IX86)
  memset(&Context,0,sizeof(CONTEXT));
  Context.ContextFlags = CONTEXT_FULL;
  Context.FloatSave.ControlWord = 0xffff037f;
  Context.FloatSave.StatusWord = 0xffff0000;
  Context.FloatSave.TagWord = 0xffffffff;
  Context.FloatSave.DataSelector = 0xffff0000;
  Context.Eip = (ULONG_PTR)((char*)ImageBaseAddress + (ULONG_PTR)Sii.EntryPoint);
  Context.SegCs = USER_CS;
  Context.SegDs = USER_DS;
  Context.SegEs = USER_DS;
  Context.SegFs = TEB_SELECTOR;
  Context.SegGs = USER_DS;
  Context.SegSs = USER_DS;
  Context.EFlags = 0x202;
  Context.Esp = (ULONG_PTR)pStackBase - 20;
#else
#error Unsupported architecture
#endif

  /*
   * Write in the initial stack.
   */
  InitialStack[0] = 0;
  InitialStack[1] = PEB_BASE;
  Status = NtWriteVirtualMemory(*ProcessHandle,
				(PVOID)Context.Esp,
				InitialStack,
				sizeof(InitialStack),
				&ResultLength);
  if (!NT_SUCCESS(Status))
    {
      ULONG_PTR nSize = 0;

      DPRINT("Failed to write initial stack.\n");

      NtFreeVirtualMemory(*ProcessHandle,
			  pStackLowest,
			  &nSize,
			  MEM_RELEASE);
      NtClose(*ProcessHandle);
      return(Status);
    }

  /* Create initial thread */
  DPRINT("Creating thread for initial process\n");
  Status = NtCreateThread(ThreadHandle,
			  THREAD_ALL_ACCESS,
			  NULL,
			  *ProcessHandle,
			  NULL,
			  &Context,
			  &InitialTeb,
			  FALSE);
  if (!NT_SUCCESS(Status))
    {
      ULONG_PTR nSize = 0;

      DPRINT("NtCreateThread() failed (Status %lx)\n", Status);

      NtFreeVirtualMemory(*ProcessHandle,
			  pStackLowest,
			  &nSize,
			  MEM_RELEASE);

      NtClose(*ProcessHandle);
      return(Status);
    }

  DPRINT("Process created successfully\n");

  return(STATUS_SUCCESS);
}

/* EOF */
