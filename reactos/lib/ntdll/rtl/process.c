/* $Id: process.c,v 1.16 2000/02/25 23:58:03 ekohl Exp $
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

#define WIN32_NO_PEHDR
#include <ddk/ntddk.h>
#include <wchar.h>
#include <string.h>
#include <pe.h>
#include <internal/i386/segment.h>
#include <ntdll/ldr.h>
#include <internal/teb.h>
#include <ntdll/base.h>
#include <ntdll/rtl.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* FUNCTIONS ****************************************************************/

#define STACK_TOP (0xb0000000)

HANDLE STDCALL KlCreateFirstThread(HANDLE ProcessHandle,
				   DWORD dwStackSize,
				   LPTHREAD_START_ROUTINE lpStartAddress,
				   DWORD dwCreationFlags,
				   PCLIENT_ID ClientId)
{
   NTSTATUS Status;
   HANDLE ThreadHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   CONTEXT ThreadContext;
   INITIAL_TEB InitialTeb;
   BOOLEAN CreateSuspended = FALSE;
   PVOID BaseAddress;
   CLIENT_ID Cid;
   
   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = NULL;
   ObjectAttributes.ObjectName = NULL;
   ObjectAttributes.Attributes = 0;
   ObjectAttributes.SecurityQualityOfService = NULL;

   if (dwCreationFlags & CREATE_SUSPENDED)
     CreateSuspended = TRUE;
   else
     CreateSuspended = FALSE;


   BaseAddress = (PVOID)(STACK_TOP - dwStackSize);
   Status = NtAllocateVirtualMemory(ProcessHandle,
				    &BaseAddress,
				    0,
				    (PULONG)&dwStackSize,
				    MEM_COMMIT,
				    PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("Failed to allocate stack\n");
	return(NULL);
     }

   memset(&ThreadContext,0,sizeof(CONTEXT));
   ThreadContext.Eip = (ULONG)lpStartAddress;
   ThreadContext.SegGs = USER_DS;
   ThreadContext.SegFs = USER_DS;
   ThreadContext.SegEs = USER_DS;
   ThreadContext.SegDs = USER_DS;
   ThreadContext.SegCs = USER_CS;
   ThreadContext.SegSs = USER_DS;
   ThreadContext.Esp = STACK_TOP - 20;
   ThreadContext.EFlags = (1<<1) + (1<<9);

   DPRINT("ThreadContext.Eip %x\n",ThreadContext.Eip);

   Status = NtCreateThread(&ThreadHandle,
			   THREAD_ALL_ACCESS,
			   &ObjectAttributes,
			   ProcessHandle,
			   &Cid,
			   &ThreadContext,
			   &InitialTeb,
			   CreateSuspended);
   if (ClientId != NULL)
     {
	memcpy(&ClientId->UniqueThread, &Cid.UniqueThread, sizeof(ULONG));
     }

   return(ThreadHandle);
}

static NTSTATUS RtlpMapFile(
PRTL_USER_PROCESS_PARAMETERS	Ppb,
//PUNICODE_STRING ApplicationName,
			    PHANDLE Section)
{
   HANDLE hFile;
   IO_STATUS_BLOCK IoStatusBlock;
   OBJECT_ATTRIBUTES ObjectAttributes;
   PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
   NTSTATUS Status;

   hFile = NULL;

   RtlDeNormalizeProcessParams (Ppb);

   InitializeObjectAttributes(&ObjectAttributes,
//			      ApplicationName,
			      &(Ppb->ImagePathName),
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      SecurityDescriptor);

   RtlNormalizeProcessParams (Ppb);

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
			   PRTL_USER_PROCESS_PARAMETERS	Ppb)
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
					 MEM_COMMIT,
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
   PpbBase = (PVOID)PEB_STARTUPINFO;
   PpbSize = Ppb->MaximumLength;
   Status = NtAllocateVirtualMemory(ProcessHandle,
				    &PpbBase,
				    0,
				    &PpbSize,
				    MEM_COMMIT,
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

   return(STATUS_SUCCESS);
}

NTSTATUS
STDCALL
RtlCreateUserProcess (
	PUNICODE_STRING			CommandLine,		// verified
	ULONG				Unknown2,
	PRTL_USER_PROCESS_PARAMETERS	Ppb,			// verified
	PSECURITY_DESCRIPTOR		ProcessSd,
	PSECURITY_DESCRIPTOR		ThreadSd,
	WINBOOL				bInheritHandles,
	DWORD				dwCreationFlags,
	ULONG				Unknown8,
	ULONG				Unknown9,
	PRTL_USER_PROCESS_INFO		ProcessInfo		// verified
	)
{
   HANDLE hSection;
   HANDLE hThread;
   NTSTATUS Status;
   LPTHREAD_START_ROUTINE  lpStartAddress = NULL;
   PROCESS_BASIC_INFORMATION ProcessBasicInfo;
   ULONG retlen;

   DPRINT("RtlCreateUserProcess\n");
   
//   Status = RtlpMapFile(CommandLine,
   Status = RtlpMapFile(Ppb,
			&hSection);
   
   /*
    * Create a new process
    */
   
   Status = NtCreateProcess(&(ProcessInfo->ProcessHandle),
			    PROCESS_ALL_ACCESS,
			    NULL,
			    NtCurrentProcess(),
			    bInheritHandles,
			    hSection,
			    NULL,
			    NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   /*
    * Get some information about the process
    */
   
   ZwQueryInformationProcess(ProcessInfo->ProcessHandle,
			     ProcessBasicInformation,
			     &ProcessBasicInfo,
			     sizeof(ProcessBasicInfo),
			     &retlen);
   DPRINT("ProcessBasicInfo.UniqueProcessId %d\n",
	  ProcessBasicInfo.UniqueProcessId);
   ProcessInfo->ClientId.UniqueProcess = (HANDLE)ProcessBasicInfo.UniqueProcessId;

   /*
    * Create Process Environment Block
    */
   DPRINT("Creating peb\n");
   KlInitPeb(ProcessInfo->ProcessHandle, Ppb);

   DPRINT("Creating thread for process\n");
   lpStartAddress = (LPTHREAD_START_ROUTINE)
     ((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET(NTDLL_BASE))->
     AddressOfEntryPoint + 
     ((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET(NTDLL_BASE))->ImageBase;
   
   hThread =  KlCreateFirstThread(ProcessInfo->ProcessHandle,
//				  Headers.OptionalHeader.SizeOfStackReserve,
				  0x200000,
				  lpStartAddress,
				  dwCreationFlags,
				  &(ProcessInfo->ClientId));
   if (hThread == NULL)
   {
	DPRINT("Failed to create thread\n");
	return(STATUS_UNSUCCESSFUL);
   }
   return(STATUS_SUCCESS);
}

/* EOF */
