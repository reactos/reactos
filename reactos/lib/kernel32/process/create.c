/* $Id: create.c,v 1.23 2000/03/16 01:14:37 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/proc/proc.c
 * PURPOSE:         Process functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#define WIN32_NO_PEHDR
#include <ddk/ntddk.h>
#include <windows.h>
#include <kernel32/proc.h>
#include <kernel32/thread.h>
#include <wchar.h>
#include <string.h>
#include <pe.h>
#include <internal/i386/segment.h>
#include <ntdll/ldr.h>
#include <internal/teb.h>
#include <ntdll/base.h>
#include <ntdll/rtl.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* FUNCTIONS ****************************************************************/

WINBOOL
STDCALL
CreateProcessA (
	LPCSTR			lpApplicationName,
	LPSTR			lpCommandLine,
	LPSECURITY_ATTRIBUTES	lpProcessAttributes,
	LPSECURITY_ATTRIBUTES	lpThreadAttributes,
	WINBOOL			bInheritHandles,
	DWORD			dwCreationFlags,
	LPVOID			lpEnvironment,
	LPCSTR			lpCurrentDirectory,
	LPSTARTUPINFOA		lpStartupInfo,
	LPPROCESS_INFORMATION	lpProcessInformation
	)
/*
 * FUNCTION: The CreateProcess function creates a new process and its
 * primary thread. The new process executes the specified executable file
 * ARGUMENTS:
 * 
 *     lpApplicationName = Pointer to name of executable module
 *     lpCommandLine = Pointer to command line string
 *     lpProcessAttributes = Process security attributes
 *     lpThreadAttributes = Thread security attributes
 *     bInheritHandles = Handle inheritance flag
 *     dwCreationFlags = Creation flags
 *     lpEnvironment = Pointer to new environment block
 *     lpCurrentDirectory = Pointer to current directory name
 *     lpStartupInfo = Pointer to startup info
 *     lpProcessInformation = Pointer to process information
 */
{
	UNICODE_STRING ApplicationNameU;
	UNICODE_STRING CurrentDirectoryU;
	UNICODE_STRING CommandLineU;
	ANSI_STRING ApplicationName;
	ANSI_STRING CurrentDirectory;
	ANSI_STRING CommandLine;
	WINBOOL Result;

	DPRINT("CreateProcessA\n");

	RtlInitAnsiString (&CommandLine,
	                   lpCommandLine);
	RtlInitAnsiString (&ApplicationName,
	                   (LPSTR)lpApplicationName);
	RtlInitAnsiString (&CurrentDirectory,
	                   (LPSTR)lpCurrentDirectory);

	/* convert ansi (or oem) strings to unicode */
	if (bIsFileApiAnsi)
	{
		RtlAnsiStringToUnicodeString (&CommandLineU,
		                              &CommandLine,
		                              TRUE);
		RtlAnsiStringToUnicodeString (&ApplicationNameU,
		                              &ApplicationName,
		                              TRUE);
		RtlAnsiStringToUnicodeString (&CurrentDirectoryU,
		                              &CurrentDirectory,
		                              TRUE);
	}
	else
	{
		RtlOemStringToUnicodeString (&CommandLineU,
		                             &CommandLine,
		                             TRUE);
		RtlOemStringToUnicodeString (&ApplicationNameU,
		                             &ApplicationName,
		                             TRUE);
		RtlOemStringToUnicodeString (&CurrentDirectoryU,
		                             &CurrentDirectory,
		                             TRUE);
	}

	Result = CreateProcessW (ApplicationNameU.Buffer,
	                         CommandLineU.Buffer,
	                         lpProcessAttributes,
	                         lpThreadAttributes,
	                         bInheritHandles,
	                         dwCreationFlags,
	                         lpEnvironment,
	                         CurrentDirectoryU.Buffer,
	                         (LPSTARTUPINFOW)lpStartupInfo,
	                         lpProcessInformation);

	RtlFreeUnicodeString (&ApplicationNameU);
	RtlFreeUnicodeString (&CommandLineU);
	RtlFreeUnicodeString (&CurrentDirectoryU);

	return Result;
}

#define STACK_TOP (0xb0000000)

HANDLE STDCALL KlCreateFirstThread(HANDLE ProcessHandle,
				   LPSECURITY_ATTRIBUTES lpThreadAttributes,
				   DWORD dwStackSize,
				   LPTHREAD_START_ROUTINE lpStartAddress,
				   DWORD dwCreationFlags,
				   LPDWORD lpThreadId)
{
   NTSTATUS Status;
   HANDLE ThreadHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   CLIENT_ID ClientId;
   CONTEXT ThreadContext;
   INITIAL_TEB InitialTeb;
   BOOLEAN CreateSuspended = FALSE;
   PVOID BaseAddress;

   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = NULL;
   ObjectAttributes.ObjectName = NULL;
   ObjectAttributes.Attributes = 0;
   if (lpThreadAttributes != NULL) 
     {
	if (lpThreadAttributes->bInheritHandle) 
	  ObjectAttributes.Attributes = OBJ_INHERIT;
	ObjectAttributes.SecurityDescriptor = 
	  lpThreadAttributes->lpSecurityDescriptor;
     }
   ObjectAttributes.SecurityQualityOfService = NULL;

   if ((dwCreationFlags & CREATE_SUSPENDED) == CREATE_SUSPENDED)
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
			   &ClientId,
			   &ThreadContext,
			   &InitialTeb,
			   CreateSuspended);
   if (lpThreadId != NULL)
     {
	memcpy(lpThreadId, &ClientId.UniqueThread,sizeof(ULONG));
     }

   return(ThreadHandle);
}

HANDLE KlMapFile(LPCWSTR lpApplicationName,
		 LPCWSTR lpCommandLine)
{
   WCHAR TempApplicationName[256];
   WCHAR TempFileName[256];
   HANDLE hFile;
   IO_STATUS_BLOCK IoStatusBlock;
   ULONG i;
   WCHAR TempDirectoryName[256];
   UNICODE_STRING ApplicationNameString;
   OBJECT_ATTRIBUTES ObjectAttributes;
   PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
   NTSTATUS Status;
   HANDLE hSection;
   DWORD len = 0;

   hFile = NULL;

   /*
    * Find the application name
    */
   TempApplicationName[0] = '\\';
   TempApplicationName[1] = '?';
   TempApplicationName[2] = '?';
   TempApplicationName[3] = '\\';
   TempApplicationName[4] = 0;
   
   DPRINT("TempApplicationName '%w'\n",TempApplicationName);

   if (lpApplicationName != NULL)
     {
	wcscpy(TempFileName, lpApplicationName);
	
	DPRINT("TempFileName '%w'\n",TempFileName);
     }
   else
     {
	wcscpy(TempFileName, lpCommandLine);
	
	DPRINT("TempFileName '%w'\n",TempFileName);
	
	for (i=0; TempFileName[i]!=' ' && TempFileName[i] != 0; i++);
	TempFileName[i]=0;
	
     }
   if (TempFileName[1] != ':')
     {
        len = GetCurrentDirectoryW(MAX_PATH,TempDirectoryName);
        if (TempDirectoryName[len - 1] != L'\\')
	  {
                TempDirectoryName[len] = L'\\';
                TempDirectoryName[len + 1] = 0;
	  }
	wcscat(TempApplicationName,TempDirectoryName);
     }
   wcscat(TempApplicationName,TempFileName);

   RtlInitUnicodeString(&ApplicationNameString, TempApplicationName);

   DPRINT("ApplicationName %w\n",ApplicationNameString.Buffer);

   InitializeObjectAttributes(&ObjectAttributes,
			      &ApplicationNameString,
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
	SetLastError(RtlNtStatusToDosError(Status));
	return(NULL);
     }

   Status = NtCreateSection(&hSection,
			    SECTION_ALL_ACCESS,
			    NULL,
			    NULL,
			    PAGE_EXECUTE,
			    SEC_IMAGE,
			    hFile);
   NtClose(hFile);

   if (!NT_SUCCESS(Status))
     {
	SetLastError(RtlNtStatusToDosError(Status));
	return(NULL);
     }

   return(hSection);
}

static NTSTATUS KlInitPeb (HANDLE ProcessHandle,
			   PRTL_USER_PROCESS_PARAMETERS	Ppb)
{
   NTSTATUS Status;
   PVOID PpbBase;
   ULONG PpbSize;
   ULONG BytesWritten;
   ULONG Offset;
   PVOID ParentEnv = NULL;
   PVOID EnvPtr = NULL;
   ULONG EnvSize = 0;

   /* create the Environment */
   if (Ppb->Environment != NULL)
	ParentEnv = Ppb->Environment;
   else if (NtCurrentPeb()->ProcessParameters->Environment != NULL)
	ParentEnv = NtCurrentPeb()->ProcessParameters->Environment;

   if (ParentEnv != NULL)
     {
	MEMORY_BASIC_INFORMATION MemInfo;

	Status = NtQueryVirtualMemory (NtCurrentProcess (),
	                               ParentEnv,
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
			     ParentEnv,
			     EnvSize,
			     &BytesWritten);
     }

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
   NtWriteVirtualMemory(ProcessHandle,
			PpbBase,
			Ppb,
			Ppb->MaximumLength,
			&BytesWritten);

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


WINBOOL STDCALL CreateProcessW(LPCWSTR lpApplicationName,
			       LPWSTR lpCommandLine,
			       LPSECURITY_ATTRIBUTES lpProcessAttributes,
			       LPSECURITY_ATTRIBUTES lpThreadAttributes,
			       WINBOOL bInheritHandles,
			       DWORD dwCreationFlags,
			       LPVOID lpEnvironment,
			       LPCWSTR lpCurrentDirectory,
			       LPSTARTUPINFOW lpStartupInfo,
			       LPPROCESS_INFORMATION lpProcessInformation)
{
   HANDLE hSection, hProcess, hThread;
   NTSTATUS Status;
   LPTHREAD_START_ROUTINE  lpStartAddress = NULL;
   WCHAR TempCommandLine[256];
//   PVOID BaseAddress;
//   LARGE_INTEGER SectionOffset;
//   IMAGE_NT_HEADERS Headers;
//   IMAGE_DOS_HEADER DosHeader;
//   HANDLE NTDllSection;
//   ULONG InitialViewSize;
   PROCESS_BASIC_INFORMATION ProcessBasicInfo;
   ULONG retlen;
   DWORD len = 0;
   PRTL_USER_PROCESS_PARAMETERS Ppb;
   UNICODE_STRING CommandLine_U;

   DPRINT("CreateProcessW(lpApplicationName '%w', lpCommandLine '%w')\n",
	   lpApplicationName,lpCommandLine);
   
   /*
    * Process the application name and command line   
    */
   
   if (lpApplicationName[1] != ':')
     {
        len = GetCurrentDirectoryW(MAX_PATH,TempCommandLine);
        if (TempCommandLine[len - 1] != L'\\')
	  {
                TempCommandLine[len] = L'\\';
                TempCommandLine[len + 1] = 0;
	  }
        wcscat(TempCommandLine,lpApplicationName);
     }
   else
        wcscpy(TempCommandLine, lpApplicationName);

   if (lpCommandLine != NULL)
     {
	wcscat(TempCommandLine, L" ");
	wcscat(TempCommandLine, lpCommandLine);
     }
   
   /*
    * Create the PPB
    */
   
   RtlInitUnicodeString(&CommandLine_U, TempCommandLine);

   DPRINT("CommandLine_U %w\n", CommandLine_U.Buffer);

   RtlCreateProcessParameters(&Ppb,
			      &CommandLine_U,
			      NULL,
			      NULL,
			      NULL,
			      lpEnvironment,
			      NULL,
			      NULL,
			      NULL,
			      NULL);
   
   /*
    * Create a section for the executable
    */
   
   hSection = KlMapFile (lpApplicationName, lpCommandLine);
   if (hSection == NULL)
   {
	RtlDestroyProcessParameters (Ppb);
	return FALSE;
   }
   
   /*
    * Create a new process
    */
   
   Status = NtCreateProcess(&hProcess,
			    PROCESS_ALL_ACCESS,
			    NULL,
			    NtCurrentProcess(),
			    bInheritHandles,
			    hSection,
			    NULL,
			    NULL);
   
   /*
    * Get some information about the process
    */
   
   ZwQueryInformationProcess(hProcess,
			     ProcessBasicInformation,
			     &ProcessBasicInfo,
			     sizeof(ProcessBasicInfo),
			     &retlen);
   DPRINT("ProcessBasicInfo.UniqueProcessId %d\n",
	  ProcessBasicInfo.UniqueProcessId);
   lpProcessInformation->dwProcessId = ProcessBasicInfo.UniqueProcessId;

   /*
    * Create Process Environment Block
    */
   DPRINT("Creating peb\n");
   KlInitPeb(hProcess, Ppb);

   RtlDestroyProcessParameters (Ppb);

   DPRINT("Creating thread for process\n");
   lpStartAddress = (LPTHREAD_START_ROUTINE)
     ((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET(NTDLL_BASE))->
     AddressOfEntryPoint + 
     ((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET(NTDLL_BASE))->ImageBase;
   
   hThread =  KlCreateFirstThread(hProcess,
				  lpThreadAttributes,
//				  Headers.OptionalHeader.SizeOfStackReserve,
				  0x200000,
				  lpStartAddress,
				  dwCreationFlags,
				  &lpProcessInformation->dwThreadId);
   
   if (hThread == NULL)
     {
	return FALSE;
     }

   lpProcessInformation->hProcess = hProcess;
   lpProcessInformation->hThread = hThread;

   return TRUE;
}

/* EOF */
