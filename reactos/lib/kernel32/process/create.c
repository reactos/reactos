/*
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
#include <windows.h>
#include <kernel32/proc.h>
#include <kernel32/thread.h>
#include <wchar.h>
#include <string.h>
#include <pe.h>
#include <internal/i386/segment.h>
#include <ntdll/ldr.h>
#include <internal/teb.h>

//#define NDEBUG
#include <kernel32/kernel32.h>

/* FUNCTIONS ****************************************************************/

WINBOOL STDCALL CreateProcessA(LPCSTR lpApplicationName,
			       LPSTR lpCommandLine,
			       LPSECURITY_ATTRIBUTES lpProcessAttributes,
			       LPSECURITY_ATTRIBUTES lpThreadAttributes,
			       WINBOOL bInheritHandles,
			       DWORD dwCreationFlags,
			       LPVOID lpEnvironment,
			       LPCSTR lpCurrentDirectory,
			       LPSTARTUPINFO lpStartupInfo,
			       LPPROCESS_INFORMATION lpProcessInformation)
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
   WCHAR ApplicationNameW[MAX_PATH];
   WCHAR CommandLineW[MAX_PATH];
   WCHAR CurrentDirectoryW[MAX_PATH];
   PWSTR PApplicationNameW;
   PWSTR PCommandLineW;
   PWSTR PCurrentDirectoryW;
   
   DPRINT("CreateProcessA\n");
   
   PApplicationNameW = InternalAnsiToUnicode(ApplicationNameW,
					     lpApplicationName,					     
					     MAX_PATH);
   PCommandLineW = InternalAnsiToUnicode(CommandLineW,
					 lpCommandLine,
					 MAX_PATH);
   PCurrentDirectoryW = InternalAnsiToUnicode(CurrentDirectoryW,
					      lpCurrentDirectory,
					      MAX_PATH);	
   return CreateProcessW(PApplicationNameW,
			 PCommandLineW, 
			 lpProcessAttributes,
			 lpThreadAttributes,
			 bInheritHandles,
			 dwCreationFlags,
			 lpEnvironment,
			 PCurrentDirectoryW,
			 lpStartupInfo,
			 lpProcessInformation);				
}

#define STACK_TOP (0xb0000000)

HANDLE STDCALL CreateFirstThread(HANDLE ProcessHandle,
				 LPSECURITY_ATTRIBUTES lpThreadAttributes,
				 DWORD dwStackSize,
				 LPTHREAD_START_ROUTINE lpStartAddress,
				 LPVOID lpParameter,
				 DWORD dwCreationFlags,
				 LPDWORD lpThreadId,
				 PWSTR lpCommandLine,
				 HANDLE NTDllSectionHandle,
				 HANDLE SectionHandle,
				 PVOID ImageBase)
{	
   NTSTATUS Status;
   HANDLE ThreadHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   CLIENT_ID ClientId;
   CONTEXT ThreadContext;
   INITIAL_TEB InitialTeb;
   BOOLEAN CreateSuspended = FALSE;
   PVOID BaseAddress;
   ULONG BytesWritten;
   HANDLE DupNTDllSectionHandle, DupSectionHandle;
   
   
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
   ThreadContext.Esp = STACK_TOP - 16;
   ThreadContext.EFlags = (1<<1) + (1<<9);
   
   DPRINT("ThreadContext.Eip %x\n",ThreadContext.Eip);
   
   NtDuplicateObject(NtCurrentProcess(),
		     &SectionHandle,
		     ProcessHandle,
		     &DupSectionHandle,
		     0,
		     FALSE,
		     DUPLICATE_SAME_ACCESS);
   NtDuplicateObject(NtCurrentProcess(),
		     &NTDllSectionHandle,
		     ProcessHandle,
		     &DupNTDllSectionHandle,
		     0,
		     FALSE,
		     DUPLICATE_SAME_ACCESS);

   NtWriteVirtualMemory(ProcessHandle,
			(PVOID)(STACK_TOP - 4),
			&DupNTDllSectionHandle,
			sizeof(DupNTDllSectionHandle),
			&BytesWritten);
   NtWriteVirtualMemory(ProcessHandle,
			(PVOID)(STACK_TOP - 8),
			&ImageBase,
			sizeof(ImageBase),
			&BytesWritten);
   NtWriteVirtualMemory(ProcessHandle,
			(PVOID)(STACK_TOP - 12),
			&DupSectionHandle,
			sizeof(DupSectionHandle),
			&BytesWritten);

   
    Status = NtCreateThread(&ThreadHandle,
			    THREAD_ALL_ACCESS,
			    &ObjectAttributes,
			    ProcessHandle,
			    &ClientId,
			    &ThreadContext,
			    &InitialTeb,
			    CreateSuspended);
   if ( lpThreadId != NULL )
     memcpy(lpThreadId, &ClientId.UniqueThread,sizeof(ULONG));
   
   return ThreadHandle;
}

HANDLE KERNEL32_MapFile(LPCWSTR lpApplicationName,
			LPCWSTR lpCommandLine,
			PIMAGE_NT_HEADERS Headers,
			PIMAGE_DOS_HEADER DosHeader)
{
   WCHAR TempApplicationName[256];
   WCHAR TempFileName[256];
   HANDLE hFile;
   IO_STATUS_BLOCK IoStatusBlock;
   LARGE_INTEGER FileOffset;
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
   
   Status = NtReadFile(hFile,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       DosHeader,
		       sizeof(IMAGE_DOS_HEADER),
		       NULL,
		       NULL);
   if (!NT_SUCCESS(Status))
     {
	SetLastError(RtlNtStatusToDosError(Status));
	return(NULL);	
     }
   
   FileOffset.u.LowPart = DosHeader->e_lfanew;
   FileOffset.u.HighPart = 0;

   Status = NtReadFile(hFile,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       Headers,
		       sizeof(IMAGE_NT_HEADERS),
		       &FileOffset,
		       NULL);
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

#define NTDLL_BASE (0x80000000)

static NTSTATUS CreatePeb(HANDLE ProcessHandle, PWSTR CommandLine)
{
   NTSTATUS Status;
   PVOID PebBase;
   ULONG PebSize;
   NT_PEB Peb;
   ULONG BytesWritten;
   PVOID StartupInfoBase;
   ULONG StartupInfoSize;
   PROCESSINFOW StartupInfo;
   
   PebBase = PEB_BASE;
   PebSize = 0x1000;
   Status = ZwAllocateVirtualMemory(ProcessHandle,
				    &PebBase,
				    0,
				    &PebSize,
				    MEM_COMMIT,
				    PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   
   memset(&Peb, 0, sizeof(Peb));
   Peb.StartupInfo = PEB_STARTUPINFO;

   ZwWriteVirtualMemory(ProcessHandle,
			(PVOID)PEB_BASE,
			&Peb,
			sizeof(Peb),
			&BytesWritten);
   
   StartupInfoBase = PEB_STARTUPINFO;
   StartupInfoSize = 0x1000;
   Status = ZwAllocateVirtualMemory(ProcessHandle,
				    &StartupInfoBase,
				    0,
				    &StartupInfoSize,
				    MEM_COMMIT,
				    PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   
   memset(&StartupInfo, 0, sizeof(StartupInfo));
   wcscpy(StartupInfo.CommandLine, CommandLine);
   
   DPRINT("StartupInfoSize %x\n",StartupInfoSize);
   ZwWriteVirtualMemory(ProcessHandle,
			(PVOID)PEB_STARTUPINFO,
			&StartupInfo,
			StartupInfoSize,
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
			       LPSTARTUPINFO lpStartupInfo,
			       LPPROCESS_INFORMATION lpProcessInformation)
{
   HANDLE hSection, hProcess, hThread;
   NTSTATUS Status;
   LPTHREAD_START_ROUTINE  lpStartAddress = NULL;
   LPVOID  lpParameter = NULL;
   WCHAR TempCommandLine[256];
   PVOID BaseAddress;
   LARGE_INTEGER SectionOffset;
   IMAGE_NT_HEADERS Headers;
   IMAGE_DOS_HEADER DosHeader;
   HANDLE NTDllSection;
   ULONG InitialViewSize;
   
   DPRINT("CreateProcessW(lpApplicationName '%w', lpCommandLine '%w')\n",
	   lpApplicationName,lpCommandLine);
   
   wcscpy(TempCommandLine, lpApplicationName);
   if (lpCommandLine != NULL)
     {
	wcscat(TempCommandLine, L" ");
	wcscat(TempCommandLine, lpCommandLine);
     }
   
   
   hSection = KERNEL32_MapFile(lpApplicationName, 
			       lpCommandLine,
			       &Headers, &DosHeader);
   
   Status = NtCreateProcess(&hProcess,
			    PROCESS_ALL_ACCESS, 
			    NULL,
			    NtCurrentProcess(),
			    bInheritHandles,
			    NULL,
			    NULL,
			    NULL);
   
   /*
    * Map NT DLL into the process
    */
   Status = LdrMapNTDllForProcess(hProcess,
				  &NTDllSection);
   
   InitialViewSize = DosHeader.e_lfanew + sizeof(IMAGE_NT_HEADERS) 
     + sizeof(IMAGE_SECTION_HEADER) * Headers.FileHeader.NumberOfSections;
   
   BaseAddress = (PVOID)Headers.OptionalHeader.ImageBase;
   SectionOffset.QuadPart = 0;
   Status = NtMapViewOfSection(hSection,
			       hProcess,
			       &BaseAddress,
			       0,
			       InitialViewSize,
			       &SectionOffset,
			       &InitialViewSize,
			       0,
			       MEM_COMMIT,
			       PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	SetLastError(RtlNtStatusToDosError(Status));
	return FALSE;
     }
   
   /*
    * 
    */
   DPRINT("Creating peb\n");
   CreatePeb(hProcess, TempCommandLine);
   
   DPRINT("Creating thread for process\n");
   lpStartAddress = (LPTHREAD_START_ROUTINE)
     ((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET(NTDLL_BASE))->
     AddressOfEntryPoint + 
     ((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET(NTDLL_BASE))->ImageBase;
   hThread =  CreateFirstThread(hProcess,	
				lpThreadAttributes,
				Headers.OptionalHeader.SizeOfStackReserve,
				lpStartAddress,	
				lpParameter,	
				dwCreationFlags,
				&lpProcessInformation->dwThreadId,
				TempCommandLine,
				NTDllSection,
				hSection,
				(PVOID)Headers.OptionalHeader.ImageBase);

   if ( hThread == NULL )
     return FALSE;
      
   lpProcessInformation->hProcess = hProcess;
   lpProcessInformation->hThread = hThread;

   return TRUE;				
}

