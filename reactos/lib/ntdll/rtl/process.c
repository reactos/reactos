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

static NTSTATUS RtlpCreateFirstThread
(
 HANDLE ProcessHandle,
 ULONG StackReserve,
 ULONG StackCommit,
 LPTHREAD_START_ROUTINE lpStartAddress,
 PCLIENT_ID ClientId,
 PHANDLE ThreadHandle
)
{
 return RtlCreateUserThread
 (
  ProcessHandle,
  NULL,
  FALSE,
  0,
  &StackReserve,
  &StackCommit,
  lpStartAddress,
  (PVOID)PEB_BASE,
  ThreadHandle,
  ClientId
 );
}

PPEB
STDCALL
RtlpCurrentPeb(VOID)
{
    return NtCurrentPeb();
}

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
   PpbSize = Ppb->AllocationSize;

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

   DPRINT("Ppb->MaximumLength %x\n", Ppb->AllocationSize);

   /* write process parameters block*/
   RtlDeNormalizeProcessParams (Ppb);
   NtWriteVirtualMemory(ProcessHandle,
			PpbBase,
			Ppb,
			Ppb->AllocationSize,

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

/*
 * @implemented
 */
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

   NtClose(hSection);
   
   if (!NT_SUCCESS(Status))
   {
	DPRINT("Failed to create thread\n");
	return(Status);
   }

   return(STATUS_SUCCESS);
}


/*
* @implemented
*/
NTSTATUS STDCALL
RtlGetVersion(RTL_OSVERSIONINFOW *Info)
{
   if (Info->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOW) ||
       Info->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW))
   {
      PPEB Peb = NtCurrentPeb();
      
      Info->dwMajorVersion = Peb->OSMajorVersion;
      Info->dwMinorVersion = Peb->OSMinorVersion;
      Info->dwBuildNumber = Peb->OSBuildNumber;
      Info->dwPlatformId = Peb->OSPlatformId;
      if(((Peb->OSCSDVersion >> 8) & 0xFF) != 0)
      {
        int i = _snwprintf(Info->szCSDVersion,
                           (sizeof(Info->szCSDVersion) / sizeof(Info->szCSDVersion[0])) - 1,
                           L"Service Pack %d",
                           ((Peb->OSCSDVersion >> 8) & 0xFF));
        Info->szCSDVersion[i] = L'\0';
      }
      else
      {
        RtlZeroMemory(Info->szCSDVersion, sizeof(Info->szCSDVersion));
      }
      if (Info->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW))
      {
         RTL_OSVERSIONINFOEXW *InfoEx = (RTL_OSVERSIONINFOEXW *)Info;
         InfoEx->wServicePackMajor = (Peb->OSCSDVersion >> 8) & 0xFF;
         InfoEx->wServicePackMinor = Peb->OSCSDVersion & 0xFF;
         InfoEx->wSuiteMask = SharedUserData->SuiteMask;
         InfoEx->wProductType = SharedUserData->NtProductType;
      }

      return STATUS_SUCCESS;
   }

   return STATUS_INVALID_PARAMETER;
}

/* EOF */
