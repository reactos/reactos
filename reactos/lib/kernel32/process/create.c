/* $Id: create.c,v 1.46 2002/05/07 22:25:40 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/process/create.c
 * PURPOSE:         Process functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <windows.h>
#include <kernel32/proc.h>
#include <kernel32/thread.h>
#include <wchar.h>
#include <string.h>
#include <napi/i386/segment.h>
#include <ntdll/ldr.h>
#include <napi/teb.h>
#include <ntdll/base.h>
#include <ntdll/rtl.h>
#include <csrss/csrss.h>
#include <ntdll/csr.h>

#define NDEBUG
#include <kernel32/kernel32.h>
#include <kernel32/error.h>

/* FUNCTIONS ****************************************************************/

WINBOOL STDCALL
CreateProcessA (LPCSTR			lpApplicationName,
		LPSTR			lpCommandLine,
		LPSECURITY_ATTRIBUTES	lpProcessAttributes,
		LPSECURITY_ATTRIBUTES	lpThreadAttributes,
		WINBOOL			bInheritHandles,
		DWORD			dwCreationFlags,
		LPVOID			lpEnvironment,
		LPCSTR			lpCurrentDirectory,
		LPSTARTUPINFOA		lpStartupInfo,
		LPPROCESS_INFORMATION	lpProcessInformation)
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
	PWCHAR lpEnvironmentW = NULL;
	UNICODE_STRING ApplicationNameU;
	UNICODE_STRING CurrentDirectoryU;
	UNICODE_STRING CommandLineU;
	ANSI_STRING ApplicationName;
	ANSI_STRING CurrentDirectory;
	ANSI_STRING CommandLine;
	WINBOOL Result;
	CHAR TempCurrentDirectoryA[256];

	DPRINT("CreateProcessA(%s)\n", lpApplicationName);
	DPRINT("dwCreationFlags %x, lpEnvironment %x, lpCurrentDirectory %x, "
		"lpStartupInfo %x, lpProcessInformation %x\n", dwCreationFlags, 
		lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);

	if (lpEnvironment)
	{
		PCHAR ptr = lpEnvironment;
		ULONG len = 0;
		UNICODE_STRING EnvironmentU;
		ANSI_STRING EnvironmentA;
		while (*ptr)
		{
			RtlInitAnsiString(&EnvironmentA, ptr);
			if (bIsFileApiAnsi)
				len += RtlAnsiStringToUnicodeSize(&EnvironmentA) + sizeof(WCHAR);
			else
				len += RtlOemStringToUnicodeSize(&EnvironmentA) + sizeof(WCHAR);
			ptr += EnvironmentA.MaximumLength;
		}
		len += sizeof(WCHAR);
		lpEnvironmentW = (PWCHAR)RtlAllocateHeap(GetProcessHeap(),
			                                 HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY, 
							 len);
		if (lpEnvironmentW == NULL)
		{
			return FALSE;
		}
		ptr = lpEnvironment;
		EnvironmentU.Buffer = lpEnvironmentW;
		EnvironmentU.Length = 0;
		EnvironmentU.MaximumLength = len;
		while (*ptr)
		{
			RtlInitAnsiString(&EnvironmentA, ptr);
			if (bIsFileApiAnsi)
				RtlAnsiStringToUnicodeString(&EnvironmentU, &EnvironmentA, FALSE);
			else
				RtlOemStringToUnicodeString(&EnvironmentU, &EnvironmentA, FALSE);
			ptr += EnvironmentA.MaximumLength;
			EnvironmentU.Buffer += (EnvironmentU.Length / sizeof(WCHAR) + 1);
			EnvironmentU.MaximumLength -= (EnvironmentU.Length + sizeof(WCHAR));
			EnvironmentU.Length = 0;
		}

		EnvironmentU.Buffer[0] = 0;	
	}
    
	RtlInitAnsiString (&CommandLine,
	                   lpCommandLine);
	RtlInitAnsiString (&ApplicationName,
	                   (LPSTR)lpApplicationName);
	if (lpCurrentDirectory != NULL)
	  {
	    RtlInitAnsiString (&CurrentDirectory,
			       (LPSTR)lpCurrentDirectory);
	  }

	/* convert ansi (or oem) strings to unicode */
	if (bIsFileApiAnsi)
	{
		RtlAnsiStringToUnicodeString (&CommandLineU, &CommandLine, TRUE);
		RtlAnsiStringToUnicodeString (&ApplicationNameU, &ApplicationName, TRUE);
		if (lpCurrentDirectory != NULL)
			RtlAnsiStringToUnicodeString (&CurrentDirectoryU, &CurrentDirectory, TRUE);
	}
	else
	{
		RtlOemStringToUnicodeString (&CommandLineU, &CommandLine, TRUE);
		RtlOemStringToUnicodeString (&ApplicationNameU, &ApplicationName, TRUE);
		if (lpCurrentDirectory != NULL)  
			RtlOemStringToUnicodeString (&CurrentDirectoryU, &CurrentDirectory, TRUE);
	}

	Result = CreateProcessW (ApplicationNameU.Buffer,
	                         CommandLineU.Buffer,
	                         lpProcessAttributes,
	                         lpThreadAttributes,
	                         bInheritHandles,
	                         dwCreationFlags,
	                         lpEnvironmentW,
	                         (lpCurrentDirectory == NULL) ? NULL : CurrentDirectoryU.Buffer,
	                         (LPSTARTUPINFOW)lpStartupInfo,
	                         lpProcessInformation);

	RtlFreeUnicodeString (&ApplicationNameU);
	RtlFreeUnicodeString (&CommandLineU);
	if (lpCurrentDirectory != NULL)
		RtlFreeUnicodeString (&CurrentDirectoryU);

	if (lpEnvironmentW)
	{
		RtlFreeHeap(GetProcessHeap(), 0, lpEnvironmentW);
	}

	return Result;
}


HANDLE STDCALL 
KlCreateFirstThread(HANDLE ProcessHandle,
		    LPSECURITY_ATTRIBUTES lpThreadAttributes,
		    ULONG StackReserve,
		    ULONG StackCommit,
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
  ULONG OldPageProtection;

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

  InitialTeb.StackReserve = (StackReserve < 0x100000) ? 0x100000 : StackReserve;
  /* FIXME: use correct commit size */
#if 0
  InitialTeb.StackCommit = (StackCommit < PAGESIZE) ? PAGESIZE : StackCommit;
#endif
  InitialTeb.StackCommit = InitialTeb.StackReserve - PAGESIZE;

  /* size of guard page */
  InitialTeb.StackCommit += PAGESIZE;

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
      SetLastErrorByStatus(Status);
      return(NULL);
    }

  DPRINT("StackAllocate: %p ReserveSize: 0x%lX\n",
	 InitialTeb.StackAllocate, InitialTeb.StackReserve);

  InitialTeb.StackBase = (PVOID)((ULONG)InitialTeb.StackAllocate + InitialTeb.StackReserve);
  InitialTeb.StackLimit = (PVOID)((ULONG)InitialTeb.StackBase - InitialTeb.StackCommit);

  DPRINT("StackBase: %p StackCommit: %p\n",
	 InitialTeb.StackBase, InitialTeb.StackCommit);

  /* Commit stack page(s) */
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
      SetLastErrorByStatus(Status);
      return(NULL);
    }

  DPRINT("StackLimit: %p\n",
	 InitialTeb.StackLimit);

  /* Protect guard page */
  Status = NtProtectVirtualMemory(ProcessHandle,
				  InitialTeb.StackLimit,
				  PAGESIZE,
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
      SetLastErrorByStatus(Status);
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
  ThreadContext.Esp = (ULONG)InitialTeb.StackBase - 20;
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
  if (!NT_SUCCESS(Status))
    {
      NtFreeVirtualMemory(ProcessHandle,
			  InitialTeb.StackAllocate,
			  &InitialTeb.StackReserve,
			  MEM_RELEASE);
      SetLastErrorByStatus(Status);
      return(NULL);
    }

  if (lpThreadId != NULL)
    {
      memcpy(lpThreadId, &ClientId.UniqueThread,sizeof(ULONG));
    }

  return(ThreadHandle);
}

HANDLE 
KlMapFile(LPCWSTR lpApplicationName)
{
   HANDLE hFile;
   IO_STATUS_BLOCK IoStatusBlock;
   UNICODE_STRING ApplicationNameString;
   OBJECT_ATTRIBUTES ObjectAttributes;
   PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
   NTSTATUS Status;
   HANDLE hSection;

   hFile = NULL;

   /*
    * Find the application name
    */

   if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpApplicationName,
                                      &ApplicationNameString,
                                      NULL,
                                      NULL))
	return NULL;

   DPRINT("ApplicationName %S\n",ApplicationNameString.Buffer);

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

   RtlFreeUnicodeString (&ApplicationNameString);

   if (!NT_SUCCESS(Status))
     {
	DPRINT("Failed to open file\n");
	SetLastErrorByStatus (Status);
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
	DPRINT("Failed to create section\n");
	SetLastErrorByStatus (Status);
	return(NULL);
     }

   return(hSection);
}

static NTSTATUS 
KlInitPeb (HANDLE ProcessHandle,
	   PRTL_USER_PROCESS_PARAMETERS	Ppb)
{
   NTSTATUS Status;
   PVOID PpbBase;
   ULONG PpbSize;
   ULONG BytesWritten;
   ULONG Offset;
   PVOID ParentEnv = NULL;
   PVOID EnvPtr = NULL;
   PWCHAR ptr;
   ULONG EnvSize = 0, EnvSize1 = 0;

   /* create the Environment */
   if (Ppb->Environment != NULL)
   {
      ParentEnv = Ppb->Environment;
      ptr = ParentEnv;
      while (*ptr)
      {
	  while(*ptr++);
      }
      ptr++;
      EnvSize = (PVOID)ptr - ParentEnv;
   }
   else if (NtCurrentPeb()->ProcessParameters->Environment != NULL)
   {
      MEMORY_BASIC_INFORMATION MemInfo;
      ParentEnv = NtCurrentPeb()->ProcessParameters->Environment;

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
	EnvSize1 = EnvSize;
	Status = NtAllocateVirtualMemory(ProcessHandle,
					 &EnvPtr,
					 0,
					 &EnvSize1,
					 MEM_RESERVE | MEM_COMMIT,
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


WINBOOL STDCALL 
CreateProcessW(LPCWSTR lpApplicationName,
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
   WCHAR ImagePathName[256];
   UNICODE_STRING ImagePathName_U;
   PROCESS_BASIC_INFORMATION ProcessBasicInfo;
   ULONG retlen;
   PRTL_USER_PROCESS_PARAMETERS Ppb;
   UNICODE_STRING CommandLine_U;
   CSRSS_API_REQUEST CsrRequest;
   CSRSS_API_REPLY CsrReply;
   CHAR ImageFileName[8];
   PWCHAR s, e;
   ULONG i, len;
   ANSI_STRING ProcedureName;
   UNICODE_STRING CurrentDirectory_U;
   SECTION_IMAGE_INFORMATION Sii;
   WCHAR TempCurrentDirectoryW[256];
   WCHAR TempApplicationNameW[256];
   WCHAR TempCommandLineNameW[256];
   UNICODE_STRING RuntimeInfo_U;

   DPRINT("CreateProcessW(lpApplicationName '%S', lpCommandLine '%S')\n",
	   lpApplicationName, lpCommandLine);

   if (lpApplicationName != NULL && lpApplicationName[0] != 0)
   {
      wcscpy (TempApplicationNameW, lpApplicationName);
      i = wcslen(TempApplicationNameW);
      if (TempApplicationNameW[i - 1] == L'.')
      {
         TempApplicationNameW[i - 1] = 0;
      }
      else
      {
         s = max(wcsrchr(TempApplicationNameW, L'\\'), wcsrchr(TempApplicationNameW, L'/'));
         if (s == NULL)
         {
            s = TempApplicationNameW;
         }
	 else
	 {
	    s++;
	 }
         e = wcsrchr(s, L'.');
         if (e == NULL)
         {
            wcscat(s, L".exe");
            e = wcsrchr(s, L'.');
         }
      }
   }
   else if (lpCommandLine != NULL && lpCommandLine[0] != 0)
   {
      if (lpCommandLine[0] == L'"')
      {
         wcscpy(TempApplicationNameW, &lpCommandLine[0]);
         s = wcschr(TempApplicationNameW, L'"');
         if (s == NULL)
         {
           return FALSE;
         }
         *s = 0;
      }
      else
      {
         wcscpy(TempApplicationNameW, lpCommandLine);
         s = wcschr(TempApplicationNameW, L' ');
         if (s != NULL)
         {
           *s = 0;
         }
      }
      s = max(wcsrchr(TempApplicationNameW, L'\\'), wcsrchr(TempApplicationNameW, L'/'));
      if (s == NULL)
      {
         s = TempApplicationNameW;
      }
      s = wcsrchr(TempApplicationNameW, L'.');
      if (s == NULL)
	 wcscat(TempApplicationNameW, L".exe");
   }
   else
   {
      return FALSE;
   }

   if (!SearchPathW(NULL, TempApplicationNameW, NULL, sizeof(ImagePathName), ImagePathName, &s))
   {
     return FALSE;
   }

   e = wcsrchr(s, L'.');
   if (e != NULL && (!_wcsicmp(e, L".bat") || !_wcsicmp(e, L".cmd")))
   {
       // the command is a batch file
       if (lpApplicationName != NULL && lpApplicationName[0])
       {
	  // FIXME: use COMSPEC for the command interpreter
	  wcscpy(TempCommandLineNameW, L"cmd /c ");
	  wcscat(TempCommandLineNameW, lpApplicationName);
	  lpCommandLine = TempCommandLineNameW;
	  wcscpy(TempApplicationNameW, L"cmd.exe");
          if (!SearchPathW(NULL, TempApplicationNameW, NULL, sizeof(ImagePathName), ImagePathName, &s))
	  {
	     return FALSE;
	  }
       }
       else
       {
	  return FALSE;
       }
   }

   /*
    * Store the image file name for the process
    */
   e = wcschr(s, L'.');
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
    * Process the application name and command line
    */
   RtlInitUnicodeString(&ImagePathName_U, ImagePathName);
   RtlInitUnicodeString(&CommandLine_U, lpCommandLine);

   DPRINT("ImagePathName_U %S\n", ImagePathName_U.Buffer);
   DPRINT("CommandLine_U %S\n", CommandLine_U.Buffer);

   /* Initialize the current directory string */
   if (lpCurrentDirectory != NULL)
     {
       RtlInitUnicodeString(&CurrentDirectory_U,
			    lpCurrentDirectory);
     }
   else
     {
       GetCurrentDirectoryW(256, TempCurrentDirectoryW);
       RtlInitUnicodeString(&CurrentDirectory_U,
			    TempCurrentDirectoryW);
     }

   
   /*
    * Create a section for the executable
    */
   
   hSection = KlMapFile (ImagePathName);
   if (hSection == NULL)
   {
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
   if (lpStartupInfo)
   {
      if (lpStartupInfo->lpReserved2)
      {
         ULONG i, Count = *(ULONG*)lpStartupInfo->lpReserved2;
         HANDLE * hFile;  
	 HANDLE hTemp;
	 PRTL_USER_PROCESS_PARAMETERS CurrPpb = NtCurrentPeb()->ProcessParameters;  


         /* FIXME:
	  *    ROUND_UP(xxx,2) + 2 is a dirty hack. RtlCreateProcessParameters assumes that
	  *    the runtimeinfo is a unicode string and use RtlCopyUnicodeString for duplication.
	  *    If is possible that this function overwrite the last information in runtimeinfo
	  *    with the null terminator for the unicode string.
	  */
	 RuntimeInfo_U.Length = RuntimeInfo_U.MaximumLength = ROUND_UP(lpStartupInfo->cbReserved2, 2) + 2;
	 RuntimeInfo_U.Buffer = RtlAllocateHeap(GetProcessHeap(), 0, RuntimeInfo_U.Length);
	 memcpy(RuntimeInfo_U.Buffer, lpStartupInfo->lpReserved2, lpStartupInfo->cbReserved2);
      }
   } 

   /*
    * Create the PPB
    */
   RtlCreateProcessParameters(&Ppb,
			      &ImagePathName_U,
			      NULL,
			      lpCurrentDirectory ? &CurrentDirectory_U : NULL,
			      &CommandLine_U,
			      lpEnvironment,
			      NULL,
			      NULL,
			      NULL,
			      lpStartupInfo && lpStartupInfo->lpReserved2 ? &RuntimeInfo_U : NULL);

   if (lpStartupInfo && lpStartupInfo->lpReserved2)
	RtlFreeHeap(GetProcessHeap(), 0, RuntimeInfo_U.Buffer);
  

   /*
    * Translate some handles for the new process
    */
   if (Ppb->CurrentDirectory.Handle)
   {
      Status = NtDuplicateObject (NtCurrentProcess(), 
	                 Ppb->CurrentDirectory.Handle,
			 hProcess,
			 &Ppb->CurrentDirectory.Handle,
			 0,
			 TRUE,
			 DUPLICATE_SAME_ACCESS);
   }

   if (Ppb->ConsoleHandle)
   {
      Status = NtDuplicateObject (NtCurrentProcess(), 
	                 Ppb->ConsoleHandle,
			 hProcess,
			 &Ppb->ConsoleHandle,
			 0,
			 TRUE,
			 DUPLICATE_SAME_ACCESS);
   }

   /*
    * Get some information about the executable
    */
   Status = ZwQuerySection(hSection,
			   SectionImageInformation,
			   &Sii,
			   sizeof(Sii),
			   &i);
   /*
    * Close the section
    */
   NtClose(hSection);

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
    * Tell the csrss server we are creating a new process
    */
   CsrRequest.Type = CSRSS_CREATE_PROCESS;
   CsrRequest.Data.CreateProcessRequest.NewProcessId = 
     ProcessBasicInfo.UniqueProcessId;
   CsrRequest.Data.CreateProcessRequest.Flags = dwCreationFlags;
   Status = CsrClientCallServer(&CsrRequest, 
				&CsrReply,
				sizeof(CSRSS_API_REQUEST),
				sizeof(CSRSS_API_REPLY));
   if (!NT_SUCCESS(Status) || !NT_SUCCESS(CsrReply.Status))
     {
	DbgPrint("Failed to tell csrss about new process. Expect trouble.\n");
     }

   // Set the child console handles 
   Ppb->InputHandle = NtCurrentPeb()->ProcessParameters->InputHandle;
   Ppb->OutputHandle = NtCurrentPeb()->ProcessParameters->OutputHandle;
   Ppb->ErrorHandle = NtCurrentPeb()->ProcessParameters->ErrorHandle;

   if (lpStartupInfo && (lpStartupInfo->dwFlags & STARTF_USESTDHANDLES))
   {
      if (lpStartupInfo->hStdInput)
	 Ppb->InputHandle = lpStartupInfo->hStdInput;
      if (lpStartupInfo->hStdOutput)
	 Ppb->OutputHandle = lpStartupInfo->hStdOutput;
      if (lpStartupInfo->hStdError)
	 Ppb->ErrorHandle = lpStartupInfo->hStdError;
   }

   if (IsConsoleHandle(Ppb->InputHandle))
   {
      Ppb->InputHandle = CsrReply.Data.CreateProcessReply.InputHandle;
   }
   else
   {
      DPRINT("Duplicate input handle\n");
      Status = NtDuplicateObject (NtCurrentProcess(), 
	                          Ppb->InputHandle,
			          hProcess,
			          &Ppb->InputHandle,
			          0,
			          TRUE,
			          DUPLICATE_SAME_ACCESS);
      if(!NT_SUCCESS(Status))
      {
	 DPRINT("NtDuplicateObject failed, status %x\n", Status);
      }
   }

   if (IsConsoleHandle(Ppb->OutputHandle))
   {
      Ppb->OutputHandle = CsrReply.Data.CreateProcessReply.OutputHandle;
   }
   else
   {
      DPRINT("Duplicate output handle\n");
      Status = NtDuplicateObject (NtCurrentProcess(), 
	                          Ppb->OutputHandle,
			          hProcess,
			          &Ppb->OutputHandle,
			          0,
			          TRUE,
			          DUPLICATE_SAME_ACCESS);
      if(!NT_SUCCESS(Status))
      {
	 DPRINT("NtDuplicateObject failed, status %x\n", Status);
      }
   }
   if (IsConsoleHandle(Ppb->ErrorHandle))
   {
      Ppb->ErrorHandle = CsrReply.Data.CreateProcessReply.OutputHandle;
   }
   else
   {
      DPRINT("Duplicate error handle\n");
      Status = NtDuplicateObject (NtCurrentProcess(), 
	                          Ppb->ErrorHandle,
			          hProcess,
			          &Ppb->ErrorHandle,
			          0,
			          TRUE,
			          DUPLICATE_SAME_ACCESS);
      if(!NT_SUCCESS(Status))
      {
	 DPRINT("NtDuplicateObject failed, status %x\n", Status);
      }
   }
   
   /*
    * Create Process Environment Block
    */
   DPRINT("Creating peb\n");

   KlInitPeb(hProcess, Ppb);

   RtlDestroyProcessParameters (Ppb);

   Status = NtSetInformationProcess(hProcess,
				    ProcessImageFileName,
				    ImageFileName,
				    8);
   /*
    * Retrieve the start address
    */
   DPRINT("Retrieving entry point address\n");
   RtlInitAnsiString (&ProcedureName, "LdrInitializeThunk");
   Status = LdrGetProcedureAddress ((PVOID)NTDLL_BASE,
				    &ProcedureName,
				    0,
				    (PVOID*)&lpStartAddress);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint ("LdrGetProcedureAddress failed (Status %x)\n", Status);
	return FALSE;
     }
   DPRINT("lpStartAddress 0x%08lx\n", (ULONG)lpStartAddress);

   /*
    * Create the thread for the kernel
    */
   DPRINT("Creating thread for process\n");
   hThread =  KlCreateFirstThread(hProcess,
				  lpThreadAttributes,
				  //Sii.StackReserve,
				  0x200000,
				  //Sii.StackCommit,
				  0x1000,
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
