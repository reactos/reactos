/* $Id$
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
#include <windows.h>
#include <napi/i386/segment.h>
#include <ntdll/ldr.h>
#include <ntdll/base.h>
#include <ntdll/rtl.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* FUNCTIONS ****************************************************************/


static NTSTATUS
RtlpMapFile(PUNICODE_STRING ImageFileName,
            PRTL_USER_PROCESS_PARAMETERS Ppb,
	    ULONG Attributes,
	    PHANDLE Section)
{
   HANDLE hFile;
   IO_STATUS_BLOCK IoStatusBlock;
   OBJECT_ATTRIBUTES ObjectAttributes;
   PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
   NTSTATUS Status;
   
   hFile = NULL;

   RtlDeNormalizeProcessParams (Ppb);

//   DbgPrint("ImagePathName %x\n", Ppb->ImagePathName.Buffer);
   
   InitializeObjectAttributes(&ObjectAttributes,
			      ImageFileName,
			      Attributes & (OBJ_CASE_INSENSITIVE | OBJ_INHERIT),
			      NULL,
			      SecurityDescriptor);

   RtlNormalizeProcessParams (Ppb);
   
   /*
    * Try to open the executable
    */

   Status = ZwOpenFile(&hFile,
			SYNCHRONIZE|FILE_EXECUTE|FILE_READ_DATA,
			&ObjectAttributes,
			&IoStatusBlock,
			FILE_SHARE_DELETE|FILE_SHARE_READ,
			FILE_SYNCHRONOUS_IO_NONALERT|FILE_NON_DIRECTORY_FILE);

   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   Status = ZwCreateSection(Section,
			    SECTION_ALL_ACCESS,
			    NULL,
			    NULL,
			    PAGE_EXECUTE,
			    SEC_IMAGE,
			    hFile);
   ZwClose(hFile);

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

   Status = ZwQueryVirtualMemory (NtCurrentProcess (),
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
   Status = ZwAllocateVirtualMemory(ProcessHandle,
					 &EnvPtr,
					 0,
					 &EnvSize,
					 MEM_RESERVE | MEM_COMMIT,
					 PAGE_READWRITE);
	if (!NT_SUCCESS(Status))
	  {
	     return(Status);
	  }

   ZwWriteVirtualMemory(ProcessHandle,
			     EnvPtr,
			     Ppb->Environment,
			     EnvSize,
			     &BytesWritten);
     }
   DPRINT("EnvironmentPointer %p\n", EnvPtr);

   /* create the PPB */
   PpbBase = NULL;
   PpbSize = Ppb->AllocationSize;

   Status = ZwAllocateVirtualMemory(ProcessHandle,
				    &PpbBase,
				    0,
				    &PpbSize,
				    MEM_RESERVE | MEM_COMMIT,
				    PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   DPRINT("Ppb->MaximumLength %x\n", Ppb->AllocationSize);

   /* write process parameters block*/
   RtlDeNormalizeProcessParams (Ppb);
   ZwWriteVirtualMemory(ProcessHandle,
			PpbBase,
			Ppb,
			Ppb->AllocationSize,

			&BytesWritten);
   RtlNormalizeProcessParams (Ppb);

   /* write pointer to environment */
   Offset = FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, Environment);
   ZwWriteVirtualMemory(ProcessHandle,
			(PVOID)(PpbBase + Offset),
			&EnvPtr,
			sizeof(EnvPtr),
			&BytesWritten);

   /* write pointer to process parameter block */
   Offset = FIELD_OFFSET(PEB, ProcessParameters);
   ZwWriteVirtualMemory(ProcessHandle,
			(PVOID)(PEB_BASE + Offset),
			&PpbBase,
			sizeof(PpbBase),
			&BytesWritten);

   /* Read image base address. */
   Offset = FIELD_OFFSET(PEB, ImageBaseAddress);
   ZwReadVirtualMemory(ProcessHandle,
		       (PVOID)(PEB_BASE + Offset),
		       ImageBaseAddress,
		       sizeof(PVOID),
		       &BytesWritten);

   return(STATUS_SUCCESS);
}


/*
 * @implemented
 *
 * Creates a process and its initial thread.
 *
 * NOTES:
 *  - The first thread is created suspended, so it needs a manual resume!!!
 *  - If ParentProcess is NULL, current process is used
 *  - ProcessParameters must be normalized
 *  - Attributes are object attribute flags used when opening the ImageFileName. 
 *    Valid flags are OBJ_INHERIT and OBJ_CASE_INSENSITIVE.
 *
 * -Gunnar
 */
NTSTATUS STDCALL
RtlCreateUserProcess(
   IN PUNICODE_STRING ImageFileName,
   IN ULONG Attributes,
   IN OUT PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
   IN PSECURITY_DESCRIPTOR ProcessSecurityDescriptor  OPTIONAL,
   IN PSECURITY_DESCRIPTOR ThreadSecurityDescriptor  OPTIONAL,
   IN HANDLE ParentProcess  OPTIONAL,
   IN BOOLEAN InheritHandles,
   IN HANDLE DebugPort  OPTIONAL,
   IN HANDLE ExceptionPort  OPTIONAL,
   OUT PRTL_PROCESS_INFO ProcessInfo
   )
{
   HANDLE hSection;
   NTSTATUS Status;
   PROCESS_BASIC_INFORMATION ProcessBasicInfo;
   ULONG retlen;
   SECTION_IMAGE_INFORMATION Sii;
   ULONG ResultLength;
   PVOID ImageBaseAddress;
   
   DPRINT("RtlCreateUserProcess\n");
   
   Status = RtlpMapFile(ImageFileName,
                        ProcessParameters,
			Attributes,
			&hSection);
   if( !NT_SUCCESS( Status ) )
     return Status;

   /*
    * Create a new process
    */
   if (ParentProcess == NULL)
     ParentProcess = NtCurrentProcess();

   Status = ZwCreateProcess(&(ProcessInfo->ProcessHandle),
			    PROCESS_ALL_ACCESS,
			    NULL,
			    ParentProcess,
             InheritHandles,
			    hSection,
			    DebugPort,
			    ExceptionPort);
   if (!NT_SUCCESS(Status))
     {
   ZwClose(hSection);
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
   KlInitPeb(ProcessInfo->ProcessHandle,
	     ProcessParameters,
	     &ImageBaseAddress);

   Status = ZwQuerySection(hSection,
			   SectionImageInformation,
			   &Sii,
			   sizeof(Sii),
			   &ResultLength);
   if (!NT_SUCCESS(Status) || ResultLength != sizeof(Sii))
     {
       DPRINT("Failed to get section image information.\n");
       ZwClose(hSection);
       return(Status);
     }

   DPRINT("Creating thread for process\n");
   Status = RtlCreateUserThread(
      ProcessInfo->ProcessHandle,
      NULL,
      TRUE, /* CreateSuspended? */
      0,
      &Sii.StackReserve,
      &Sii.StackCommit,
      ImageBaseAddress + (ULONG)Sii.EntryPoint,
      (PVOID)PEB_BASE,
      &ProcessInfo->ThreadHandle,
      &ProcessInfo->ClientId
      );

   ZwClose(hSection);
   
   if (!NT_SUCCESS(Status))
   {
	DPRINT("Failed to create thread\n");
	return(Status);
   }

   return(STATUS_SUCCESS);
}



/* EOF */
