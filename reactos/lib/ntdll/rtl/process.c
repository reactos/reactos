/* $Id: process.c,v 1.13 2000/02/14 14:13:33 dwelch Exp $
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

static NTSTATUS RtlpMapFile(PUNICODE_STRING ApplicationName,
			    PHANDLE Section)
{
   HANDLE hFile;
   IO_STATUS_BLOCK IoStatusBlock;
   OBJECT_ATTRIBUTES ObjectAttributes;
   PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
   NTSTATUS Status;

   hFile = NULL;

   InitializeObjectAttributes(&ObjectAttributes,
			      ApplicationName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      SecurityDescriptor);

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
   
   /* create the PPB */
   PpbBase = (PVOID)PEB_STARTUPINFO;
   PpbSize = Ppb->TotalSize;
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

   DPRINT("Ppb->TotalSize %x\n", Ppb->TotalSize);
   NtWriteVirtualMemory(ProcessHandle,
			PpbBase,
			Ppb,
			Ppb->TotalSize,
			&BytesWritten);
   
   Offset = FIELD_OFFSET(PEB, ProcessParameters);
   
   NtWriteVirtualMemory(ProcessHandle,
			(PVOID)(PEB_BASE + Offset),
			&PpbBase,
			sizeof(PpbBase),
			&BytesWritten);

   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL RtlCreateUserProcess(PUNICODE_STRING		CommandLine,
				      ULONG			Unknown1,
				      PRTL_USER_PROCESS_PARAMETERS Ppb,
				      PSECURITY_DESCRIPTOR ProcessSd,
				      PSECURITY_DESCRIPTOR ThreadSd,
				      WINBOOL bInheritHandles,
				      DWORD dwCreationFlags,
				      PCLIENT_ID ClientId,
				      PHANDLE ProcessHandle,
				      PHANDLE ThreadHandle)
{
   HANDLE hSection;
   HANDLE hThread;
   NTSTATUS Status;
   LPTHREAD_START_ROUTINE  lpStartAddress = NULL;
   WCHAR TempCommandLine[256];
   PVOID BaseAddress;
   LARGE_INTEGER SectionOffset;
   ULONG InitialViewSize;
   PROCESS_BASIC_INFORMATION ProcessBasicInfo;
   ULONG retlen;
   DWORD len = 0;

   DPRINT("CreateProcessW(CommandLine '%w')\n", CommandLine->Buffer);
   
   Status = RtlpMapFile(CommandLine,
			&hSection);
   
   /*
    * Create a new process
    */
   
   Status = NtCreateProcess(ProcessHandle,
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
   
   ZwQueryInformationProcess(*ProcessHandle,
			     ProcessBasicInformation,
			     &ProcessBasicInfo,
			     sizeof(ProcessBasicInfo),
			     &retlen);
   DPRINT("ProcessBasicInfo.UniqueProcessId %d\n",
	  ProcessBasicInfo.UniqueProcessId);
   if (ClientId != NULL)
     {
	ClientId->UniqueProcess = (HANDLE)ProcessBasicInfo.UniqueProcessId;
     }

   /*
    * Create Process Environment Block
    */
   DPRINT("Creating peb\n");
   KlInitPeb(*ProcessHandle, Ppb);

   DPRINT("Creating thread for process\n");
   lpStartAddress = (LPTHREAD_START_ROUTINE)
     ((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET(NTDLL_BASE))->
     AddressOfEntryPoint + 
     ((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET(NTDLL_BASE))->ImageBase;
   
   hThread =  KlCreateFirstThread(*ProcessHandle,
//				  Headers.OptionalHeader.SizeOfStackReserve,
				  0x200000,
				  lpStartAddress,
				  dwCreationFlags,
				  ClientId);
   if (hThread == NULL)
   {
	DPRINT("Failed to create thread\n");
	return(STATUS_UNSUCCESSFUL);
   }
   return(STATUS_SUCCESS);
}

/* EOF */
