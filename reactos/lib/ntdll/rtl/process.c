/* $Id: process.c,v 1.31 2002/10/01 19:27:20 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/process.c
 * PURPOSE:         Process functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <napi/i386/segment.h>
#include <ntdll/ldr.h>
#include <ntdll/base.h>
#include <ntdll/rtl.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* FUNCTIONS ****************************************************************/

static NTSTATUS
RtlpCreateFirstThread(HANDLE ProcessHandle,
		      ULONG StackReserve,
		      ULONG StackCommit,
		      LPTHREAD_START_ROUTINE lpStartAddress,
		      PCLIENT_ID ClientId,
		      PHANDLE ThreadHandle)
{
  NTSTATUS Status;
  OBJECT_ATTRIBUTES ObjectAttributes;
  CONTEXT ThreadContext;
  INITIAL_TEB InitialTeb;
  ULONG OldPageProtection;
  CLIENT_ID Cid;
  ULONG InitialStack[5];
  ULONG ResultLength;
  
  ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
  ObjectAttributes.RootDirectory = NULL;
  ObjectAttributes.ObjectName = NULL;
  ObjectAttributes.Attributes = 0;
  ObjectAttributes.SecurityQualityOfService = NULL;

  if (StackReserve > 0x100000)
    InitialTeb.StackReserve = StackReserve;
  else
    InitialTeb.StackReserve = 0x100000; /* 1MByte */

  /* FIXME */
#if 0
  if (StackCommit > PAGE_SIZE)
    InitialTeb.StackCommit = StackCommit;
  else
    InitialTeb.StackCommit = PAGE_SIZE;
#endif
  InitialTeb.StackCommit = InitialTeb.StackReserve - PAGE_SIZE;

  /* add guard page size */
  InitialTeb.StackCommit += PAGE_SIZE;

  /* Reserve stack */
  InitialTeb.StackAllocate = NULL;
  Status = NtAllocateVirtualMemory(ProcessHandle,
				   &InitialTeb.StackAllocate,
				   0,
				   &InitialTeb.StackReserve,
				   MEM_RESERVE,
				   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Error reserving stack space!\n");
      return(Status);
    }

  DPRINT("StackAllocate: %p ReserveSize: 0x%lX\n",
	 InitialTeb.StackAllocate, InitialTeb.StackReserve);

  InitialTeb.StackBase = (PVOID)((ULONG)InitialTeb.StackAllocate + InitialTeb.StackReserve);
  InitialTeb.StackLimit = (PVOID)((ULONG)InitialTeb.StackBase - InitialTeb.StackCommit);

  DPRINT("StackBase: %p StackCommit: 0x%lX\n",
	 InitialTeb.StackBase, InitialTeb.StackCommit);

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

      DPRINT("Error comitting stack page(s)!\n");
      return(Status);
    }

  DPRINT("StackLimit: %p\n", InitialTeb.StackLimit);

  /* Protect guard page */
  Status = NtProtectVirtualMemory(ProcessHandle,
				  InitialTeb.StackLimit,
				  PAGE_SIZE,
				  PAGE_GUARD | PAGE_READWRITE,
				  &OldPageProtection);
  if (!NT_SUCCESS(Status))
    {
      /* release the stack space */
      NtFreeVirtualMemory(ProcessHandle,
			  InitialTeb.StackAllocate,
			  &InitialTeb.StackReserve,
			  MEM_RELEASE);

      DPRINT("Error comitting guard page!\n");
      return(Status);
    }

  memset(&ThreadContext,0,sizeof(CONTEXT));
  ThreadContext.Eip = (ULONG)lpStartAddress;
  ThreadContext.SegGs = USER_DS;
  ThreadContext.SegFs = TEB_SELECTOR;
  ThreadContext.SegEs = USER_DS;
  ThreadContext.SegDs = USER_DS;
  ThreadContext.SegCs = USER_CS;
  ThreadContext.SegSs = USER_DS;
  ThreadContext.Esp = (ULONG)InitialTeb.StackBase - 20;
  ThreadContext.EFlags = (1<<1) + (1<<9);

  DPRINT("ThreadContext.Eip %x\n",ThreadContext.Eip);

  /*
   * Write in the initial stack.
   */
  InitialStack[0] = 0;
  InitialStack[1] = PEB_BASE;
  Status = ZwWriteVirtualMemory(ProcessHandle,
				(PVOID)ThreadContext.Esp,
				InitialStack,
				sizeof(InitialStack),
				&ResultLength);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to write initial stack.\n");
      return(Status);
    }

  Status = NtCreateThread(ThreadHandle,
			  THREAD_ALL_ACCESS,
			  &ObjectAttributes,
			  ProcessHandle,
			  &Cid,
			  &ThreadContext,
			  &InitialTeb,
			  FALSE);
  if (!NT_SUCCESS(Status))
    {
      NtFreeVirtualMemory(ProcessHandle,
			  InitialTeb.StackAllocate,
			  &InitialTeb.StackReserve,
			  MEM_RELEASE);
      return(Status);
    }

  if (ClientId != NULL)
    {
      memcpy(&ClientId->UniqueThread, &Cid.UniqueThread, sizeof(ULONG));
    }

  return(STATUS_SUCCESS);
}

static NTSTATUS
RtlpMapFile(PRTL_USER_PROCESS_PARAMETERS Ppb,
	    ULONG Attributes,
	    PHANDLE Section,
	    PCHAR ImageFileName)
{
   HANDLE hFile;
   IO_STATUS_BLOCK IoStatusBlock;
   OBJECT_ATTRIBUTES ObjectAttributes;
   PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
   NTSTATUS Status;
   PWCHAR s;
   PWCHAR e;
   ULONG i;
   
   hFile = NULL;

   RtlDeNormalizeProcessParams (Ppb);

//   DbgPrint("ImagePathName %x\n", Ppb->ImagePathName.Buffer);
   
   InitializeObjectAttributes(&ObjectAttributes,
			      &(Ppb->ImagePathName),
			      Attributes & (OBJ_CASE_INSENSITIVE | OBJ_INHERIT),
			      NULL,
			      SecurityDescriptor);

   RtlNormalizeProcessParams (Ppb);
   
   /*
    * 
    */
//   DbgPrint("ImagePathName %x\n", Ppb->ImagePathName.Buffer);
//   DbgPrint("ImagePathName %S\n", Ppb->ImagePathName.Buffer);
   s = wcsrchr(Ppb->ImagePathName.Buffer, '\\');
   if (s == NULL)
     {
	s = Ppb->ImagePathName.Buffer;
     }
   else
     {
	s++;
     }
   e = wcschr(s, '.');
   if (e != NULL)
     {
	*e = 0;
     }
   for (i = 0; i < 8; i++)
     {
	ImageFileName[i] = (CHAR)(s[i]);
     }
   if (e != NULL)
     {
	*e = '.';
     }
   
   /*
    * Try to open the executable
    */

   Status = NtOpenFile(&hFile,
			SYNCHRONIZE|FILE_EXECUTE|FILE_READ_DATA,
			&ObjectAttributes,
			&IoStatusBlock,
			FILE_SHARE_DELETE|FILE_SHARE_READ,
			FILE_SYNCHRONOUS_IO_NONALERT|FILE_NON_DIRECTORY_FILE);

   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   Status = NtCreateSection(Section,
			    SECTION_ALL_ACCESS,
			    NULL,
			    NULL,
			    PAGE_EXECUTE,
			    SEC_IMAGE,
			    hFile);
   NtClose(hFile);

   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   return(STATUS_SUCCESS);
}

static NTSTATUS KlInitPeb (HANDLE ProcessHandle,
			   PRTL_USER_PROCESS_PARAMETERS	Ppb,
			   PVOID* ImageBaseAddress)
{
   NTSTATUS Status;
   PVOID PpbBase;
   ULONG PpbSize;
   ULONG BytesWritten;
   ULONG Offset;
   PVOID EnvPtr = NULL;
   ULONG EnvSize = 0;

   /* create the Environment */
   if (Ppb->Environment != NULL)
     {
	MEMORY_BASIC_INFORMATION MemInfo;

	Status = NtQueryVirtualMemory (NtCurrentProcess (),
	                               Ppb->Environment,
	                               MemoryBasicInformation,
	                               &MemInfo,
	                               sizeof(MEMORY_BASIC_INFORMATION),
	                               NULL);
	if (!NT_SUCCESS(Status))
	  {
	     return Status;
	  }
	EnvSize = MemInfo.RegionSize;
     }
   DPRINT("EnvironmentSize %ld\n", EnvSize);

   /* allocate and initialize new environment block */
   if (EnvSize != 0)
     {
	Status = NtAllocateVirtualMemory(ProcessHandle,
					 &EnvPtr,
					 0,
					 &EnvSize,
					 MEM_RESERVE | MEM_COMMIT,
					 PAGE_READWRITE);
	if (!NT_SUCCESS(Status))
	  {
	     return(Status);
	  }

	NtWriteVirtualMemory(ProcessHandle,
			     EnvPtr,
			     Ppb->Environment,
			     EnvSize,
			     &BytesWritten);
     }
   DPRINT("EnvironmentPointer %p\n", EnvPtr);

   /* create the PPB */
   PpbBase = NULL;
   PpbSize = Ppb->MaximumLength;
   Status = NtAllocateVirtualMemory(ProcessHandle,
				    &PpbBase,
				    0,
				    &PpbSize,
				    MEM_RESERVE | MEM_COMMIT,
				    PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   DPRINT("Ppb->MaximumLength %x\n", Ppb->MaximumLength);

   /* write process parameters block*/
   RtlDeNormalizeProcessParams (Ppb);
   NtWriteVirtualMemory(ProcessHandle,
			PpbBase,
			Ppb,
			Ppb->MaximumLength,
			&BytesWritten);
   RtlNormalizeProcessParams (Ppb);

   /* write pointer to environment */
   Offset = FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, Environment);
   NtWriteVirtualMemory(ProcessHandle,
			(PVOID)(PpbBase + Offset),
			&EnvPtr,
			sizeof(EnvPtr),
			&BytesWritten);

   /* write pointer to process parameter block */
   Offset = FIELD_OFFSET(PEB, ProcessParameters);
   NtWriteVirtualMemory(ProcessHandle,
			(PVOID)(PEB_BASE + Offset),
			&PpbBase,
			sizeof(PpbBase),
			&BytesWritten);

   /* Read image base address. */
   Offset = FIELD_OFFSET(PEB, ImageBaseAddress);
   NtReadVirtualMemory(ProcessHandle,
		       (PVOID)(PEB_BASE + Offset),
		       ImageBaseAddress,
		       sizeof(PVOID),
		       &BytesWritten);

   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlCreateUserProcess(PUNICODE_STRING ImageFileName,
		     ULONG Attributes,
		     PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
		     PSECURITY_DESCRIPTOR ProcessSecurityDescriptor,
		     PSECURITY_DESCRIPTOR ThreadSecurityDescriptor,
		     HANDLE ParentProcess,
		     BOOLEAN CurrentDirectory,
		     HANDLE DebugPort,
		     HANDLE ExceptionPort,
		     PRTL_PROCESS_INFO ProcessInfo)
{
   HANDLE hSection;
   NTSTATUS Status;
   LPTHREAD_START_ROUTINE lpStartAddress = NULL;
   PROCESS_BASIC_INFORMATION ProcessBasicInfo;
   ULONG retlen;
   CHAR FileName[8];
   ANSI_STRING ProcedureName;
   SECTION_IMAGE_INFORMATION Sii;
   ULONG ResultLength;
   PVOID ImageBaseAddress;
   
   DPRINT("RtlCreateUserProcess\n");
   
   Status = RtlpMapFile(ProcessParameters,
			Attributes,
			&hSection,
			FileName);
   if( !NT_SUCCESS( Status ) )
     return Status;

   /*
    * Create a new process
    */
   if (ParentProcess == NULL)
     ParentProcess = NtCurrentProcess();

   Status = NtCreateProcess(&(ProcessInfo->ProcessHandle),
			    PROCESS_ALL_ACCESS,
			    NULL,
			    ParentProcess,
			    CurrentDirectory,
			    hSection,
			    DebugPort,
			    ExceptionPort);
   if (!NT_SUCCESS(Status))
     {
	NtClose(hSection);
	return(Status);
     }
   
   /*
    * Get some information about the process
    */
   NtQueryInformationProcess(ProcessInfo->ProcessHandle,
			     ProcessBasicInformation,
			     &ProcessBasicInfo,
			     sizeof(ProcessBasicInfo),
			     &retlen);
   DPRINT("ProcessBasicInfo.UniqueProcessId %d\n",
	  ProcessBasicInfo.UniqueProcessId);
   ProcessInfo->ClientId.UniqueProcess = (HANDLE)ProcessBasicInfo.UniqueProcessId;
			  
   Status = NtSetInformationProcess(ProcessInfo->ProcessHandle,
				    ProcessImageFileName,
				    FileName,
				    8);

   /*
    * Create Process Environment Block
    */
   DPRINT("Creating peb\n");
   KlInitPeb(ProcessInfo->ProcessHandle,
	     ProcessParameters,
	     &ImageBaseAddress);

   Status = NtQuerySection(hSection,
			   SectionImageInformation,
			   &Sii,
			   sizeof(Sii),
			   &ResultLength);
   if (!NT_SUCCESS(Status) || ResultLength != sizeof(Sii))
     {
       DPRINT("Failed to get section image information.\n");
       NtClose(hSection);
       return(Status);
     }

   DPRINT("Creating thread for process\n");
   Status = RtlpCreateFirstThread(ProcessInfo->ProcessHandle,
				  Sii.StackReserve,
				  Sii.StackCommit,
				  ImageBaseAddress + (ULONG)Sii.EntryPoint,
				  &ProcessInfo->ClientId,
				  &ProcessInfo->ThreadHandle);
   if (!NT_SUCCESS(Status))
   {
	DPRINT("Failed to create thread\n");
	NtClose(hSection);
	return(Status);
   }
   NtClose(hSection);
   return(STATUS_SUCCESS);
}

/* EOF */
