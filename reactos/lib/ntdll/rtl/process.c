/*
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
//#include <windows.h>
//#include <kernel32/proc.h>
//#include <kernel32/thread.h>
#include <wchar.h>
#include <string.h>
#include <pe.h>
#include <internal/i386/segment.h>
#include <ntdll/ldr.h>
#include <internal/teb.h>
#include <ntdll/base.h>

//#define NDEBUG
#include <ntdll/ntdll.h>

/* FUNCTIONS ****************************************************************/


#define STACK_TOP (0xb0000000)

static HANDLE STDCALL
RtlpCreateFirstThread(HANDLE ProcessHandle,
                      PSECURITY_DESCRIPTOR SecurityDescriptor,
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
//   ObjectAttributes.Attributes = OBJ_INHERIT;
   ObjectAttributes.SecurityDescriptor = SecurityDescriptor;
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


static NTSTATUS
RtlpMapFile(PUNICODE_STRING ApplicationName,
            PIMAGE_NT_HEADERS Headers,
            PIMAGE_DOS_HEADER DosHeader,
            PHANDLE Section)
{
    HANDLE hFile;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER FileOffset;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    NTSTATUS Status;

    hFile = NULL;
    *Section = NULL;


    DPRINT("ApplicationName %w\n", ApplicationName->Buffer);

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
        return Status;

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
        return Status;

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
        return Status; 

    Status = NtCreateSection(Section,
                             SECTION_ALL_ACCESS,
                             NULL,
                             NULL,
                             PAGE_EXECUTE,
                             SEC_IMAGE,
                             hFile);
    NtClose(hFile);

    if (!NT_SUCCESS(Status)) 
        return Status;

    return STATUS_SUCCESS;
}


static NTSTATUS
RtlpCreatePeb(HANDLE ProcessHandle, PUNICODE_STRING CommandLine)
{
    NTSTATUS Status;
    PVOID PebBase;
    ULONG PebSize;
    NT_PEB Peb;
    ULONG BytesWritten;
    PVOID StartupInfoBase;
    ULONG StartupInfoSize;
    PROCESSINFOW StartupInfo;

    PebBase = (PVOID)PEB_BASE;
    PebSize = 0x1000;
    Status = NtAllocateVirtualMemory(ProcessHandle,
                                     &PebBase,
                                     0,
                                     &PebSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
	return(Status);

    memset(&Peb, 0, sizeof(Peb));
    Peb.StartupInfo = (PPROCESSINFOW)PEB_STARTUPINFO;

    NtWriteVirtualMemory(ProcessHandle,
                         (PVOID)PEB_BASE,
                         &Peb,
                         sizeof(Peb),
                         &BytesWritten);

    StartupInfoBase = (PVOID)PEB_STARTUPINFO;
    StartupInfoSize = 0x1000;
    Status = NtAllocateVirtualMemory(ProcessHandle,
                                     &StartupInfoBase,
                                     0,
                                     &StartupInfoSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
	return(Status);

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    wcscpy(StartupInfo.CommandLine, CommandLine->Buffer);

    DPRINT("StartupInfoSize %x\n",StartupInfoSize);
    NtWriteVirtualMemory(ProcessHandle,
                         (PVOID)PEB_STARTUPINFO,
                         &StartupInfo,
                         StartupInfoSize,
                         &BytesWritten);

    return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlCreateUserProcess(PUNICODE_STRING ApplicationName,
                     PSECURITY_DESCRIPTOR ProcessSd,
                     PSECURITY_DESCRIPTOR ThreadSd,
			       WINBOOL bInheritHandles,
			       DWORD dwCreationFlags,
//                               LPVOID lpEnvironment,
//                               LPCWSTR lpCurrentDirectory,
//                               LPSTARTUPINFO lpStartupInfo,
                               PCLIENT_ID ClientId,
                               PHANDLE ProcessHandle,
                               PHANDLE ThreadHandle)
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
   PROCESS_BASIC_INFORMATION ProcessBasicInfo;
   CLIENT_ID LocalClientId;
   ULONG retlen;

    DPRINT("RtlCreateUserProcess(ApplicationName '%w')\n",
           ApplicationName->Buffer);

    Status = RtlpMapFile(ApplicationName,
                         &Headers,
                         &DosHeader,
                         &hSection);

    Status = NtCreateProcess(&hProcess,
                             PROCESS_ALL_ACCESS, 
                             NULL,
                             NtCurrentProcess(),
                             bInheritHandles,
                             NULL,
                             NULL,
                             NULL);

    NtQueryInformationProcess(hProcess,
                              ProcessBasicInformation,
                              &ProcessBasicInfo,
                              sizeof(ProcessBasicInfo),
                              &retlen);
    DPRINT("ProcessBasicInfo.UniqueProcessId %d\n",
           ProcessBasicInfo.UniqueProcessId);
    LocalClientId.UniqueProcess = ProcessBasicInfo.UniqueProcessId;
   
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
       return Status;
   
   /*
    * 
    */
   DPRINT("Creating peb\n");
   RtlpCreatePeb(hProcess, ApplicationName);
   
   DPRINT("Creating thread for process\n");
   lpStartAddress = (LPTHREAD_START_ROUTINE)
     ((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET(NTDLL_BASE))->
     AddressOfEntryPoint + 
     ((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET(NTDLL_BASE))->ImageBase;
   hThread = RtlpCreateFirstThread(hProcess, 
                                   ThreadSd,
				Headers.OptionalHeader.SizeOfStackReserve,
				lpStartAddress,	
				lpParameter,	
				dwCreationFlags,
                                &LocalClientId.UniqueThread,
				TempCommandLine,
				NTDllSection,
				hSection,
				(PVOID)Headers.OptionalHeader.ImageBase);

    if ( hThread == NULL )
        return Status;

    if (ClientId)
    {
        ClientId->UniqueProcess = LocalClientId.UniqueProcess;
        ClientId->UniqueThread = LocalClientId.UniqueThread;
    }

    if (ProcessHandle)
        *ProcessHandle = hProcess;

    if (ThreadHandle)
        *ThreadHandle = hThread;

    return STATUS_SUCCESS;
}

