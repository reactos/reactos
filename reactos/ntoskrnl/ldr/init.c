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
/* $Id: init.c,v 1.39 2003/04/26 23:13:31 hyperion Exp $
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


/* MACROS ******************************************************************/

#define DENORMALIZE(x,addr) {if(x) x=(VOID*)((ULONG)(x)-(ULONG)(addr));}
#define ALIGN(x,align)      (((ULONG)(x)+(align)-1UL)&(~((align)-1UL)))


/* FUNCTIONS *****************************************************************/

static NTSTATUS
LdrpMapProcessImage(PHANDLE SectionHandle,
		    PUNICODE_STRING ImagePath)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE FileHandle;
  NTSTATUS Status;

  /* Open image file */
  InitializeObjectAttributes(&ObjectAttributes,
			     ImagePath,
			     0,
			     NULL,
			     NULL);

  DPRINT("Opening image file %S\n", ObjectAttributes.ObjectName->Buffer);
  Status = NtOpenFile(&FileHandle,
		      FILE_ALL_ACCESS,
		      &ObjectAttributes,
		      NULL,
		      0,
		      0);
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
  LocalPpb->ImagePathName.Buffer[ImagePath->Length / sizeof(WCHAR)] = (WCHAR)0;

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

  /* Set image file name */
  Status = NtSetInformationProcess(ProcessHandle,
				   ProcessImageFileName,
				   "SMSS",
				   5);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtSetInformationProcess() failed (Status %lx)\n", Status);
      return(Status);
    }

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
 PUSER_STACK UserStack,
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

 UserStack->FixedStackBase = NULL;
 UserStack->FixedStackLimit =  NULL;
 UserStack->ExpandableStackBase = NULL;
 UserStack->ExpandableStackLimit = NULL; 
 UserStack->ExpandableStackBottom = NULL;

 /* FIXME: this code assumes a stack growing downwards */
 /* fixed stack */
 if(*StackCommit == *StackReserve)
 {
  DPRINT("Fixed stack\n");

  UserStack->FixedStackLimit = NULL;

  /* allocate the stack */
  nErrCode = NtAllocateVirtualMemory
  (
   ProcessHandle,
   &(UserStack->FixedStackLimit),
   0,
   StackReserve,
   MEM_RESERVE | MEM_COMMIT,
   PAGE_READWRITE
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) return nErrCode;

  /* store the highest (first) address of the stack */
  UserStack->FixedStackBase =
   (PUCHAR)(UserStack->FixedStackLimit) + *StackReserve;
 }
 /* expandable stack */
 else
 {
  ULONG_PTR nGuardSize = PAGE_SIZE;
  PVOID pGuardBase;

  DPRINT("Expandable stack\n");

  UserStack->FixedStackLimit = NULL;
  UserStack->FixedStackBase = NULL;
  UserStack->ExpandableStackBottom = NULL;

  /* reserve the stack */
  nErrCode = NtAllocateVirtualMemory
  (
   ProcessHandle,
   &(UserStack->ExpandableStackBottom),
   0,
   StackReserve,
   MEM_RESERVE,
   PAGE_READWRITE
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) return nErrCode;

  DPRINT("Reserved %08X bytes\n", *StackReserve);

  /* expandable stack base - the highest address of the stack */
  UserStack->ExpandableStackBase =
   (PUCHAR)(UserStack->ExpandableStackBottom) + *StackReserve;

  /* expandable stack limit - the lowest committed address of the stack */
  UserStack->ExpandableStackLimit =
   (PUCHAR)(UserStack->ExpandableStackBase) - *StackCommit;

  DPRINT("Stack base %p\n", UserStack->ExpandableStackBase);
  DPRINT("Stack limit %p\n", UserStack->ExpandableStackLimit);

  /* commit as much stack as requested */
  nErrCode = NtAllocateVirtualMemory
  (
   ProcessHandle,
   &(UserStack->ExpandableStackLimit),
   0,
   StackCommit,
   MEM_COMMIT,
   PAGE_READWRITE
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) goto l_Cleanup;

  DPRINT("Stack limit %p\n", UserStack->ExpandableStackLimit);

  pGuardBase = (PUCHAR)(UserStack->ExpandableStackLimit) - PAGE_SIZE;

  DPRINT("Guard base %p\n", UserStack->ExpandableStackBase);

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

  DPRINT("Guard base %p\n", UserStack->ExpandableStackBase);
 }

 return STATUS_SUCCESS;

 /* cleanup in case of failure */
l_Cleanup:
  if(UserStack->FixedStackLimit)
  pStackLowest = UserStack->FixedStackLimit;
 else if(UserStack->ExpandableStackBottom)
  pStackLowest = UserStack->ExpandableStackBottom;

 /* free the stack, if it was allocated */
 if(pStackLowest != NULL)
  NtFreeVirtualMemory(ProcessHandle, &pStackLowest, &nSize, MEM_RELEASE);

 return nErrCode;
}


NTSTATUS
LdrLoadInitialProcess(PHANDLE ProcessHandle,
		      PHANDLE ThreadHandle)
{
  SECTION_IMAGE_INFORMATION Sii;
  UNICODE_STRING ImagePath;
  HANDLE SectionHandle;
  CONTEXT Context;
  USER_STACK UserStack;
  ULONG_PTR nStackReserve = 0;
  ULONG_PTR nStackCommit = 0;
  PVOID pStackLowest;
  PVOID pStackBase;
  ULONG ResultLength;
  PVOID ImageBaseAddress;
  ULONG InitialStack[5];
  NTSTATUS Status;

  /* Get the absolute path to smss.exe. */
  RtlInitUnicodeStringFromLiteral(&ImagePath,
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
   &UserStack,
   &nStackReserve,
   &nStackCommit
  );

  if (!NT_SUCCESS(Status))
    {
      DPRINT("Failed to write initial stack.\n");
      NtClose(ProcessHandle);
      return(Status);
    }

  if(UserStack.FixedStackBase && UserStack.FixedStackLimit)
  {
   pStackBase = UserStack.FixedStackBase;
   pStackLowest = UserStack.FixedStackLimit;
  }
  else
  {
   pStackBase = UserStack.ExpandableStackBase;
   pStackLowest = UserStack.ExpandableStackBottom;
  }

  DPRINT("pStackBase = %p\n", pStackBase);
  DPRINT("pStackLowest = %p\n", pStackLowest);

  /*
   * Initialize context to point to LdrStartup
   */
#if defined(_M_IX86)
  memset(&Context,0,sizeof(CONTEXT));
  Context.ContextFlags = CONTEXT_FULL;
  Context.Eip = (ULONG_PTR)(ImageBaseAddress + (ULONG_PTR)Sii.EntryPoint);
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
			  &UserStack,
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
