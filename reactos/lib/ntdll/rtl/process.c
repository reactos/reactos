/* $Id: process.c,v 1.8 2000/01/11 17:28:57 ekohl Exp $
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

static HANDLE STDCALL
RtlpCreateFirstThread(HANDLE ProcessHandle,
                      PSECURITY_DESCRIPTOR SecurityDescriptor,
				 DWORD dwStackSize,
				 LPTHREAD_START_ROUTINE lpStartAddress,
				 PPEB Peb,
				 DWORD dwCreationFlags,
				 LPDWORD lpThreadId,
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

   /* create the process stack (first thead) */
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
   NtWriteVirtualMemory(ProcessHandle,
			(PVOID)(STACK_TOP - 16),
			&Peb,
			sizeof(PPEB),
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


    DPRINT("ApplicationName %S\n", ApplicationName->Buffer);

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
RtlpCreatePpbAndPeb (
	PPEB	*PebPtr,
	HANDLE	ProcessHandle,
	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters)
{
    NTSTATUS Status;
    ULONG BytesWritten;
    PVOID PebBase;
    ULONG PebSize;
    PEB Peb;
    PVOID PpbBase;
    ULONG PpbSize;

	/* create the process parameter block */
	PpbBase = (PVOID)PEB_STARTUPINFO;
	PpbSize = ProcessParameters->MaximumLength;
	Status = NtAllocateVirtualMemory (
		ProcessHandle,
		&PpbBase,
		0,
		&PpbSize,
		MEM_COMMIT,
		PAGE_READWRITE);

	if (!NT_SUCCESS(Status))
		return(Status);

	DPRINT("Ppb size %x\n", PpbSize);
	NtWriteVirtualMemory (
		ProcessHandle,
		PpbBase,
		ProcessParameters,
		ProcessParameters->MaximumLength,
		&BytesWritten);

	/* create the PEB */
	PebBase = (PVOID)PEB_BASE;
	PebSize = 0x1000;
	Status = NtAllocateVirtualMemory (
		ProcessHandle,
		&PebBase,
		0,
		&PebSize,
		MEM_COMMIT,
		PAGE_READWRITE);

	memset (&Peb, 0, sizeof(PEB));
	Peb.ProcessParameters = (PRTL_USER_PROCESS_PARAMETERS)PpbBase;

	NtWriteVirtualMemory (
		ProcessHandle,
		PebBase,
		&Peb,
		sizeof(PEB),
		&BytesWritten);

	*PebPtr = (PPEB)PebBase;

	return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
RtlCreateUserProcess (
	PUNICODE_STRING		CommandLine,
	ULONG			Unknown1,
	PRTL_USER_PROCESS_PARAMETERS	Params,
	PSECURITY_DESCRIPTOR ProcessSd,
	PSECURITY_DESCRIPTOR ThreadSd,
	WINBOOL bInheritHandles,
	DWORD dwCreationFlags,
	PCLIENT_ID ClientId,
	PHANDLE ProcessHandle,
	PHANDLE ThreadHandle)
{
   HANDLE hSection, hProcess, hThread;
   NTSTATUS Status;
   LPTHREAD_START_ROUTINE  lpStartAddress = NULL;
   PVOID BaseAddress;
   LARGE_INTEGER SectionOffset;
   IMAGE_NT_HEADERS Headers;
   IMAGE_DOS_HEADER DosHeader;
   HANDLE NTDllSection;
   ULONG InitialViewSize;
   PROCESS_BASIC_INFORMATION ProcessBasicInfo;
   CLIENT_ID LocalClientId;
   ULONG retlen;
   PPEB Peb;

	DPRINT ("RtlCreateUserProcess(CommandLine '%S')\n",
		CommandLine->Buffer);

    Status = RtlpMapFile(CommandLine,
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
    LocalClientId.UniqueProcess = (HANDLE)ProcessBasicInfo.UniqueProcessId;

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
   DPRINT("Creating PPB and PEB\n");
   RtlpCreatePpbAndPeb (&Peb, hProcess, Params);

   DPRINT("Creating thread for process\n");
   lpStartAddress = (LPTHREAD_START_ROUTINE)
     ((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET(NTDLL_BASE))->
     AddressOfEntryPoint + 
     ((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET(NTDLL_BASE))->ImageBase;
	hThread = RtlpCreateFirstThread (
		hProcess,
		ThreadSd,
		Headers.OptionalHeader.SizeOfStackReserve,
		lpStartAddress,
		Peb,
		dwCreationFlags,
		(LPDWORD)&LocalClientId.UniqueThread,
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

VOID
STDCALL
RtlAcquirePebLock (VOID)
{

}

VOID
STDCALL
RtlReleasePebLock (VOID)
{

}

NTSTATUS
STDCALL
RtlCreateProcessParameters (
	PRTL_USER_PROCESS_PARAMETERS	*ProcessParameters,
	PUNICODE_STRING			CommandLine,
	PUNICODE_STRING			DllPath,
	PUNICODE_STRING			CurrentDirectory,
	PUNICODE_STRING			ImagePathName,
	PVOID				Environment,
	PUNICODE_STRING			WindowTitle,
	PUNICODE_STRING			DesktopInfo,
	PUNICODE_STRING			ShellInfo,
	PVOID				Reserved2
	)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PRTL_USER_PROCESS_PARAMETERS Params = NULL;
	ULONG RegionSize = 0;
	ULONG DataSize = 0;
	PWCHAR Dest;

	DPRINT ("RtlCreateProcessParameters\n");

	RtlAcquirePebLock ();

	/* size of process parameter block */
	DataSize = sizeof (RTL_USER_PROCESS_PARAMETERS);

	/* size of current directory buffer */
	DataSize += (MAX_PATH * sizeof(WCHAR));

	/* add string lengths */
	if (DllPath != NULL)
		DataSize += (DllPath->Length + sizeof(WCHAR));

	if (ImagePathName != NULL)
		DataSize += (ImagePathName->Length + sizeof(WCHAR));

	if (CommandLine != NULL)
		DataSize += (CommandLine->Length + sizeof(WCHAR));

	if (WindowTitle != NULL)
		DataSize += (WindowTitle->Length + sizeof(WCHAR));

	if (DesktopInfo != NULL)
		DataSize += (DesktopInfo->Length + sizeof(WCHAR));

	if (ShellInfo != NULL)
		DataSize += (ShellInfo->Length + sizeof(WCHAR));

	/* Calculate the required block size */
	RegionSize = ROUNDUP(DataSize, PAGESIZE);

	Status = NtAllocateVirtualMemory (
		NtCurrentProcess (),
		(PVOID*)&Params,
		0,
		&RegionSize,
		MEM_COMMIT,
		PAGE_READWRITE);
	if (!NT_SUCCESS(Status))
	{
		RtlReleasePebLock ();
		return Status;
	}

	DPRINT ("Process parameter block allocated\n");

	Params->MaximumLength = RegionSize;
	Params->Length = DataSize;
	Params->Flags = TRUE;
	Params->Environment = Environment;
//	Params->Unknown1 =
//	Params->Unknown2 =
//	Params->Unknown3 =
//	Params->Unknown4 =

	/* copy current directory */
	Dest = (PWCHAR)(((PBYTE)Params) +
	                sizeof(RTL_USER_PROCESS_PARAMETERS));

	Params->CurrentDirectory.DosPath.Buffer = Dest;
	if (CurrentDirectory != NULL)
	{
		Params->CurrentDirectory.DosPath.Length =
			CurrentDirectory->Length;
		Params->CurrentDirectory.DosPath.MaximumLength =
			CurrentDirectory->Length + sizeof(WCHAR);
		memcpy (Dest,
		        CurrentDirectory->Buffer,
		        CurrentDirectory->Length);
		Dest = (PWCHAR)(((PBYTE)Dest) + CurrentDirectory->Length);
	}
	*Dest = 0;

	Dest = (PWCHAR)(((PBYTE)Params) +
			sizeof(RTL_USER_PROCESS_PARAMETERS) +
			(MAX_PATH * sizeof(WCHAR)));

	/* copy dll path */
	Params->DllPath.Buffer = Dest;
	if (DllPath != NULL)
	{
		Params->DllPath.Length = DllPath->Length;
		memcpy (Dest,
		        DllPath->Buffer,
		        DllPath->Length);
		Dest = (PWCHAR)(((PBYTE)Dest) + DllPath->Length);
	}
	Params->DllPath.MaximumLength =
		Params->DllPath.Length + sizeof(WCHAR);
	*Dest = 0;
	Dest++;

	/* copy image path name */
	Params->ImagePathName.Buffer = Dest;
	if (ImagePathName != NULL)
	{
		Params->ImagePathName.Length = ImagePathName->Length;
		memcpy (Dest,
		        ImagePathName->Buffer,
		        ImagePathName->Length);
		Dest = (PWCHAR)(((PBYTE)Dest) + ImagePathName->Length);
	}
	Params->ImagePathName.MaximumLength =
		Params->ImagePathName.Length + sizeof(WCHAR);
	*Dest = 0;
	Dest++;

	/* copy command line */
	Params->CommandLine.Buffer = Dest;
	if (CommandLine != NULL)
	{
		Params->CommandLine.Length = CommandLine->Length;
		memcpy (Dest,
		        CommandLine->Buffer,
		        CommandLine->Length);
		Dest = (PWCHAR)(((PBYTE)Dest) + CommandLine->Length);
	}
	Params->CommandLine.MaximumLength =
		Params->CommandLine.Length + sizeof(WCHAR);
	*Dest = 0;
	Dest++;

	/* copy window title */
	Params->WindowTitle.Buffer = Dest;
	if (WindowTitle != NULL)
	{
		Params->WindowTitle.Length = WindowTitle->Length;
		memcpy (Dest,
		        WindowTitle->Buffer,
		        WindowTitle->Length);
		Dest = (PWCHAR)(((PBYTE)Dest) + WindowTitle->Length);
	}
	Params->WindowTitle.MaximumLength =
		Params->WindowTitle.Length + sizeof(WCHAR);
	*Dest = 0;
	Dest++;

	/* copy desktop info */
	Params->DesktopInfo.Buffer = Dest;
	if (DesktopInfo != NULL)
	{
		Params->DesktopInfo.Length = DesktopInfo->Length;
		memcpy (Dest,
		        DesktopInfo->Buffer,
		        DesktopInfo->Length);
		Dest = (PWCHAR)(((PBYTE)Dest) + DesktopInfo->Length);
	}
	Params->DesktopInfo.MaximumLength =
		Params->DesktopInfo.Length + sizeof(WCHAR);
	*Dest = 0;
	Dest++;

	/* copy shell info */
	Params->ShellInfo.Buffer = Dest;
	if (ShellInfo != NULL)
	{
		Params->ShellInfo.Length = ShellInfo->Length;
		memcpy (Dest,
		        ShellInfo->Buffer,
		        ShellInfo->Length);
		Dest = (PWCHAR)(((PBYTE)Dest) + ShellInfo->Length);
	}
	Params->ShellInfo.MaximumLength =
		Params->ShellInfo.Length + sizeof(WCHAR);
	*Dest = 0;
	Dest++;

	/* set runtime data */
	Params->RuntimeData.Length = 0;
	Params->RuntimeData.MaximumLength = 0;
	Params->RuntimeData.Buffer = NULL;

	RtlDeNormalizeProcessParams (Params);
	*ProcessParameters = Params;
	RtlReleasePebLock ();

	return Status;
}

VOID
STDCALL
RtlDestroyProcessParameters (
	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters
	)
{
	ULONG RegionSize = 0;

	NtFreeVirtualMemory (NtCurrentProcess (),
	                     (PVOID)ProcessParameters,
	                     &RegionSize,
	                     MEM_RELEASE);
}

/*
 * denormalize process parameters (Pointer-->Offset)
 */
VOID
STDCALL
RtlDeNormalizeProcessParams (
	PRTL_USER_PROCESS_PARAMETERS	Params
	)
{
	if (Params == NULL)
		return;

	if (Params->Flags == FALSE)
		return;

	if (Params->CurrentDirectory.DosPath.Buffer != NULL)
	{
		Params->CurrentDirectory.DosPath.Buffer =
			(PWSTR)((ULONG)Params->CurrentDirectory.DosPath.Buffer -
				(ULONG)Params);
	}

	if (Params->DllPath.Buffer != NULL)
	{
		Params->DllPath.Buffer =
			(PWSTR)((ULONG)Params->DllPath.Buffer -
				(ULONG)Params);
	}

	if (Params->ImagePathName.Buffer != NULL)
	{
		Params->ImagePathName.Buffer =
			(PWSTR)((ULONG)Params->ImagePathName.Buffer -
				(ULONG)Params);
	}

	if (Params->CommandLine.Buffer != NULL)
	{
		Params->CommandLine.Buffer =
			(PWSTR)((ULONG)Params->CommandLine.Buffer -
				(ULONG)Params);
	}

	if (Params->WindowTitle.Buffer != NULL)
	{
		Params->WindowTitle.Buffer =
			(PWSTR)((ULONG)Params->WindowTitle.Buffer -
				(ULONG)Params);
	}

	if (Params->DesktopInfo.Buffer != NULL)
	{
		Params->DesktopInfo.Buffer =
			(PWSTR)((ULONG)Params->DesktopInfo.Buffer -
				(ULONG)Params);
	}

	if (Params->ShellInfo.Buffer != NULL)
	{
		Params->ShellInfo.Buffer =
			(PWSTR)((ULONG)Params->ShellInfo.Buffer -
				(ULONG)Params);
	}

	Params->Flags = FALSE;
}

/*
 * normalize process parameters (Offset-->Pointer)
 */
VOID
STDCALL
RtlNormalizeProcessParams (
	PRTL_USER_PROCESS_PARAMETERS	Params
	)
{
	if (Params == NULL)
		return;

	if (Params->Flags == TRUE) // & PPF_NORMALIZED
		return;

	if (Params->CurrentDirectory.DosPath.Buffer != NULL)
	{
		Params->CurrentDirectory.DosPath.Buffer =
			(PWSTR)((ULONG)Params->CurrentDirectory.DosPath.Buffer +
				(ULONG)Params);
	}

	if (Params->DllPath.Buffer != NULL)
	{
		Params->DllPath.Buffer =
			(PWSTR)((ULONG)Params->DllPath.Buffer +
				(ULONG)Params);
	}

	if (Params->ImagePathName.Buffer != NULL)
	{
		Params->ImagePathName.Buffer =
			(PWSTR)((ULONG)Params->ImagePathName.Buffer +
				(ULONG)Params);
	}

	if (Params->CommandLine.Buffer != NULL)
	{
		Params->CommandLine.Buffer =
			(PWSTR)((ULONG)Params->CommandLine.Buffer +
				(ULONG)Params);
	}

	if (Params->WindowTitle.Buffer != NULL)
	{
		Params->WindowTitle.Buffer =
			(PWSTR)((ULONG)Params->WindowTitle.Buffer +
				(ULONG)Params);
	}

	if (Params->DesktopInfo.Buffer != NULL)
	{
		Params->DesktopInfo.Buffer =
			(PWSTR)((ULONG)Params->DesktopInfo.Buffer +
				(ULONG)Params);
	}

	if (Params->ShellInfo.Buffer != NULL)
	{
		Params->ShellInfo.Buffer =
			(PWSTR)((ULONG)Params->ShellInfo.Buffer +
				(ULONG)Params);
	}

	Params->Flags = TRUE;  // |= PPF_NORMALIZED;
}

/* EOF */
