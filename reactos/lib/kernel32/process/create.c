/* $Id: create.c,v 1.67 2003/04/29 02:16:59 hyperion Exp $
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

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* FUNCTIONS ****************************************************************/

__declspec(dllimport)
PRTL_BASE_PROCESS_START_ROUTINE RtlBaseProcessStartRoutine;

typedef NTSTATUS STDCALL (K32_MBSTR_TO_WCSTR)
(
 UNICODE_STRING *,
 ANSI_STRING *,
 BOOLEAN
);

NTSTATUS STDCALL K32MbStrToWcStr
(
 IN K32_MBSTR_TO_WCSTR * True,
 UNICODE_STRING * DestStr,
 ANSI_STRING * SourceStr,
 BOOLEAN Allocate
)
{
 if(SourceStr->Buffer == NULL)
 {
  DestStr->Length = DestStr->MaximumLength = 0;
  DestStr->Buffer = NULL;
  return STATUS_SUCCESS;
 }

 return True(DestStr, SourceStr, Allocate);
}

VOID STDCALL RtlRosR32AttribsToNativeAttribs
(
 OUT OBJECT_ATTRIBUTES * NativeAttribs,
 IN SECURITY_ATTRIBUTES * Ros32Attribs OPTIONAL
)
{
 NativeAttribs->Length = sizeof(*NativeAttribs);
 NativeAttribs->ObjectName = NULL;
 NativeAttribs->RootDirectory = NULL;
 NativeAttribs->Attributes = 0;
 NativeAttribs->SecurityQualityOfService = NULL;
 

 if(Ros32Attribs != NULL && Ros32Attribs->nLength >= sizeof(*Ros32Attribs))
 {
  NativeAttribs->SecurityDescriptor = Ros32Attribs->lpSecurityDescriptor;
  
  if(Ros32Attribs->bInheritHandle)
   NativeAttribs->Attributes |= OBJ_INHERIT;
 }
 else
  NativeAttribs->SecurityDescriptor = NULL;
}

VOID STDCALL RtlRosR32AttribsToNativeAttribsNamed
(
 OUT OBJECT_ATTRIBUTES * NativeAttribs,
 IN SECURITY_ATTRIBUTES * Ros32Attribs OPTIONAL,
 OUT UNICODE_STRING * NativeName OPTIONAL,
 IN WCHAR * Ros32Name OPTIONAL,
 IN HANDLE Ros32NameRoot OPTIONAL
)
{
 if(!NativeAttribs) return;

 RtlRosR32AttribsToNativeAttribs(NativeAttribs, Ros32Attribs);

 if(Ros32Name != NULL && NativeName != NULL)
 {
  RtlInitUnicodeString(NativeName, Ros32Name);

  NativeAttribs->ObjectName = NativeName;
  NativeAttribs->RootDirectory = Ros32NameRoot;
  NativeAttribs->Attributes |= OBJ_CASE_INSENSITIVE;
 }
}

BOOL STDCALL CreateProcessA
(
 LPCSTR lpApplicationName,
 LPSTR lpCommandLine,
 LPSECURITY_ATTRIBUTES lpProcessAttributes,
 LPSECURITY_ATTRIBUTES lpThreadAttributes,
 BOOL bInheritHandles,
 DWORD dwCreationFlags,
 LPVOID lpEnvironment,
 LPCSTR lpCurrentDirectory,
 LPSTARTUPINFOA lpStartupInfo,
 LPPROCESS_INFORMATION lpProcessInformation
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
 PWCHAR pwcEnv = NULL;
 UNICODE_STRING wstrApplicationName;
 UNICODE_STRING wstrCurrentDirectory;
 UNICODE_STRING wstrCommandLine;
 UNICODE_STRING wstrReserved;
 UNICODE_STRING wstrDesktop;
 UNICODE_STRING wstrTitle;
 ANSI_STRING strApplicationName;
 ANSI_STRING strCurrentDirectory;
 ANSI_STRING strCommandLine;
 ANSI_STRING strReserved;
 ANSI_STRING strDesktop;
 ANSI_STRING strTitle;
 BOOL bRetVal;
 STARTUPINFOW wsiStartupInfo;

 NTSTATUS STDCALL (*pTrue)
 (
  UNICODE_STRING *,
  ANSI_STRING *,
  BOOLEAN
 );

 ULONG STDCALL (*pRtlMbStringToUnicodeSize)(ANSI_STRING *);

 DPRINT("CreateProcessA(%s)\n", lpApplicationName);

 DPRINT
 (
   "dwCreationFlags %x, lpEnvironment %x, lpCurrentDirectory %x, "
   "lpStartupInfo %x, lpProcessInformation %x\n",
  dwCreationFlags, 
  lpEnvironment,
  lpCurrentDirectory,
  lpStartupInfo,
  lpProcessInformation
 );

 /* invalid parameter */
 if(lpStartupInfo == NULL)
 {
  SetLastError(ERROR_INVALID_PARAMETER);
  return FALSE;
 }

 /* multibyte strings are ANSI */
 if(bIsFileApiAnsi)
 {
  pTrue = RtlAnsiStringToUnicodeString;
  pRtlMbStringToUnicodeSize = RtlAnsiStringToUnicodeSize;
 }
 /* multibyte strings are OEM */
 else
 {
  pTrue = RtlOemStringToUnicodeString;
  pRtlMbStringToUnicodeSize = RtlOemStringToUnicodeSize;
 }

 /* convert the environment */
 if(lpEnvironment && !(dwCreationFlags & CREATE_UNICODE_ENVIRONMENT))
 {
  PCHAR pcScan;
  SIZE_T nEnvLen = 0;
  UNICODE_STRING wstrEnvVar;
  ANSI_STRING strEnvVar;

  /* scan the environment to calculate its Unicode size */
  for(pcScan = lpEnvironment; *pcScan; pcScan += strEnvVar.MaximumLength)
  {
   /* add the size of the current variable */
   RtlInitAnsiString(&strEnvVar, pcScan);
   nEnvLen += pRtlMbStringToUnicodeSize(&strEnvVar) + sizeof(WCHAR);
  }

  /* add the size of the final NUL character */
  nEnvLen += sizeof(WCHAR);

  /* environment too large */
  if(nEnvLen > ~((USHORT)0))
  {
   SetLastError(ERROR_OUTOFMEMORY);
   return FALSE;
  }

  /* allocate the Unicode environment */
  pwcEnv = (PWCHAR)RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, nEnvLen);

  /* failure */
  if(pwcEnv == NULL)
  {
   SetLastError(ERROR_OUTOFMEMORY);
   return FALSE;
  }

  wstrEnvVar.Buffer = pwcEnv;
  wstrEnvVar.Length = 0;
  wstrEnvVar.MaximumLength = nEnvLen;

  /* scan the environment to convert it */
  for(pcScan = lpEnvironment; *pcScan; pcScan += strEnvVar.MaximumLength)
  {
   /* convert the current variable */
   RtlInitAnsiString(&strEnvVar, pcScan);
   K32MbStrToWcStr(pTrue, &wstrEnvVar, &strEnvVar, FALSE);

   /* advance the buffer to the next variable */
   wstrEnvVar.Buffer += (wstrEnvVar.Length / sizeof(WCHAR) + 1);
   wstrEnvVar.MaximumLength -= (wstrEnvVar.Length + sizeof(WCHAR));
   wstrEnvVar.Length = 0;
  }

  /* final NUL character */
  wstrEnvVar.Buffer[0] = 0;	
 }

 /* convert the strings */
 RtlInitAnsiString(&strCommandLine, lpCommandLine);
 RtlInitAnsiString(&strApplicationName, (LPSTR)lpApplicationName);
 RtlInitAnsiString(&strCurrentDirectory, (LPSTR)lpCurrentDirectory);
 RtlInitAnsiString(&strReserved, (LPSTR)lpStartupInfo->lpReserved);
 RtlInitAnsiString(&strDesktop, (LPSTR)lpStartupInfo->lpDesktop);
 RtlInitAnsiString(&strTitle, (LPSTR)lpStartupInfo->lpTitle);

 K32MbStrToWcStr(pTrue, &wstrCommandLine, &strCommandLine, TRUE);
 K32MbStrToWcStr(pTrue, &wstrApplicationName, &strApplicationName, TRUE);
 K32MbStrToWcStr(pTrue, &wstrCurrentDirectory, &strCurrentDirectory, TRUE);
 K32MbStrToWcStr(pTrue, &wstrReserved, &strReserved, TRUE);
 K32MbStrToWcStr(pTrue, &wstrDesktop, &strDesktop, TRUE);
 K32MbStrToWcStr(pTrue, &wstrTitle, &strTitle, TRUE);

 /* convert the startup information */
 memcpy(&wsiStartupInfo, lpStartupInfo, sizeof(wsiStartupInfo));

 wsiStartupInfo.lpReserved = wstrReserved.Buffer;
 wsiStartupInfo.lpDesktop = wstrDesktop.Buffer;
 wsiStartupInfo.lpTitle = wstrTitle.Buffer;

 DPRINT("wstrApplicationName  %wZ\n", &wstrApplicationName);
 DPRINT("wstrCommandLine      %wZ\n", &wstrCommandLine);
 DPRINT("wstrCurrentDirectory %wZ\n", &wstrCurrentDirectory);
 DPRINT("wstrReserved         %wZ\n", &wstrReserved);
 DPRINT("wstrDesktop          %wZ\n", &wstrDesktop);
 DPRINT("wstrTitle            %wZ\n", &wstrTitle);

 DPRINT("wstrApplicationName.Buffer  %p\n", wstrApplicationName.Buffer);
 DPRINT("wstrCommandLine.Buffer      %p\n", wstrCommandLine.Buffer);
 DPRINT("wstrCurrentDirectory.Buffer %p\n", wstrCurrentDirectory.Buffer);
 DPRINT("wstrReserved.Buffer         %p\n", wstrReserved.Buffer);
 DPRINT("wstrDesktop.Buffer          %p\n", wstrDesktop.Buffer);
 DPRINT("wstrTitle.Buffer            %p\n", wstrTitle.Buffer);

 DPRINT("sizeof(STARTUPINFOA) %lu\n", sizeof(STARTUPINFOA));
 DPRINT("sizeof(STARTUPINFOW) %lu\n", sizeof(STARTUPINFOW));

 /* call the Unicode function */
 bRetVal = CreateProcessW
 (
  wstrApplicationName.Buffer,
  wstrCommandLine.Buffer,
  lpProcessAttributes,
  lpThreadAttributes,
  bInheritHandles,
  dwCreationFlags,
  dwCreationFlags & CREATE_UNICODE_ENVIRONMENT ? lpEnvironment : pwcEnv,
  wstrCurrentDirectory.Buffer,
  &wsiStartupInfo,
  lpProcessInformation
 );

 RtlFreeUnicodeString(&wstrApplicationName);
 RtlFreeUnicodeString(&wstrCommandLine);
 RtlFreeUnicodeString(&wstrCurrentDirectory);
 RtlFreeUnicodeString(&wstrReserved);
 RtlFreeUnicodeString(&wstrDesktop);
 RtlFreeUnicodeString(&wstrTitle);

 RtlFreeHeap(GetProcessHeap(), 0, pwcEnv);

 return bRetVal;
}

static int _except_recursion_trap = 0;

struct _CONTEXT;
struct __EXCEPTION_RECORD;

static
EXCEPTION_DISPOSITION
__cdecl
_except_handler(
    struct _EXCEPTION_RECORD *ExceptionRecord,
    void * EstablisherFrame,
    struct _CONTEXT *ContextRecord,
    void * DispatcherContext )
{
 DPRINT1("Process terminated abnormally due to unhandled exception\n");

 if (3 < ++_except_recursion_trap)
    {
      DPRINT1("_except_handler(...) appears to be recursing.\n");
      DPRINT1("Process HALTED.\n");
      for (;;)
	{
	}
    }

  if (/* FIXME: */ TRUE) /* Not a service */
    {
      DPRINT("  calling ExitProcess(0) no, lets try ExitThread . . .\n");
      /* ExitProcess(0); */
      ExitThread(0);
    }
  else
    {
    DPRINT("  calling ExitThread(0) . . .\n");
    ExitThread(0);
    }

  DPRINT1("  We should not get to here !!!\n");
  /* We should not get to here */
  return ExceptionContinueSearch;
}

VOID STDCALL
BaseProcessStart(LPTHREAD_START_ROUTINE lpStartAddress,
		 DWORD lpParameter)
{
	UINT uExitCode = 0;

    DPRINT("BaseProcessStart(..) - setting up exception frame.\n");

	__try1(_except_handler)
	{
		uExitCode = (lpStartAddress)((PVOID)lpParameter);
	} __except1
	{
	}

    DPRINT("BaseProcessStart(..) - cleaned up exception frame.\n");

	ExitThread(uExitCode);
}


HANDLE STDCALL KlCreateFirstThread
(
 HANDLE ProcessHandle,
 LPSECURITY_ATTRIBUTES lpThreadAttributes,
 PSECTION_IMAGE_INFORMATION Sii,
 LPTHREAD_START_ROUTINE lpStartAddress,
 DWORD dwCreationFlags,
 LPDWORD lpThreadId
)
{
 OBJECT_ATTRIBUTES oaThreadAttribs;
 CLIENT_ID cidClientId;
 PVOID pTrueStartAddress;
 NTSTATUS nErrCode;
 HANDLE hThread;

 /* convert the thread attributes */
 RtlRosR32AttribsToNativeAttribs(&oaThreadAttribs, lpThreadAttributes);

 /* native image */
 if(Sii->Subsystem != IMAGE_SUBSYSTEM_NATIVE)
  pTrueStartAddress = (PVOID)BaseProcessStart;
 /* Win32 image */
 else
  pTrueStartAddress = (PVOID)RtlBaseProcessStartRoutine;

 DPRINT
 (
  "RtlRosCreateUserThreadVa\n"
  "(\n"
  " ProcessHandle    %p,\n"
  " ObjectAttributes %p,\n"
  " CreateSuspended  %d,\n"
  " StackZeroBits    %d,\n"
  " StackReserve     %lu,\n"
  " StackCommit      %lu,\n"
  " StartAddress     %p,\n"
  " ThreadHandle     %p,\n"
  " ClientId         %p,\n"
  " ParameterCount   %u,\n"
  " Parameters[0]    %p,\n"
  " Parameters[1]    %p\n"
  ")\n",
  ProcessHandle,
  &oaThreadAttribs,
  dwCreationFlags & CREATE_SUSPENDED,
  0,
  Sii->StackReserve,
  Sii->StackCommit,
  pTrueStartAddress,
  &hThread,
  &cidClientId,
  2,
  lpStartAddress,
  PEB_BASE
 );

 /* create the first thread */
 nErrCode = RtlRosCreateUserThreadVa
 (
  ProcessHandle,
  &oaThreadAttribs,
  dwCreationFlags & CREATE_SUSPENDED,
  0,
  &(Sii->StackReserve),
  &(Sii->StackCommit),
  pTrueStartAddress,
  &hThread,
  &cidClientId,
  2,
  (ULONG_PTR)lpStartAddress,
  (ULONG_PTR)PEB_BASE
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  SetLastErrorByStatus(nErrCode);
  return NULL;
 }
 
 DPRINT
 (
  "StackReserve          %p\n"
  "StackCommit           %p\n"
  "ThreadHandle          %p\n"
  "ClientId.UniqueThread %p\n",
  Sii->StackReserve,
  Sii->StackCommit,
  hThread,
  cidClientId.UniqueThread
 );

 /* success */
 if(lpThreadId) *lpThreadId = (DWORD)cidClientId.UniqueThread;
 return hThread;
}

HANDLE KlMapFile(LPCWSTR lpApplicationName)
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

static NTSTATUS KlInitPeb
(
 HANDLE ProcessHandle,
 PRTL_USER_PROCESS_PARAMETERS Ppb,
 PVOID * ImageBaseAddress
)
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

   //DPRINT("Ppb->MaximumLength %x\n", Ppb->MaximumLength);
   NtWriteVirtualMemory(ProcessHandle,
			PpbBase,
			Ppb,
			Ppb->AllocationSize,
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

   /* Read image base address. */
   Offset = FIELD_OFFSET(PEB, ImageBaseAddress);
   NtReadVirtualMemory(ProcessHandle,
		       (PVOID)(PEB_BASE + Offset),
		       ImageBaseAddress,
		       sizeof(PVOID),
		       &BytesWritten);

   return(STATUS_SUCCESS);
}


WINBOOL STDCALL 
CreateProcessW
(
 LPCWSTR lpApplicationName,
 LPWSTR lpCommandLine,
 LPSECURITY_ATTRIBUTES lpProcessAttributes,
 LPSECURITY_ATTRIBUTES lpThreadAttributes,
 WINBOOL bInheritHandles,
 DWORD dwCreationFlags,
 LPVOID lpEnvironment,
 LPCWSTR lpCurrentDirectory,
 LPSTARTUPINFOW lpStartupInfo,
 LPPROCESS_INFORMATION lpProcessInformation
)
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
   PVOID ImageBaseAddress;

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
         wcscpy(TempApplicationNameW, lpCommandLine + 1);
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
      s = wcsrchr(s, L'.');
      if (s == NULL)
	 wcscat(TempApplicationNameW, L".exe");
   }
   else
   {
      return FALSE;
   }

   DPRINT("CreateProcessW(lpApplicationName '%S', lpCommandLine '%S')\n",
	   lpApplicationName, lpCommandLine);

   if (!SearchPathW(NULL, TempApplicationNameW, NULL, sizeof(ImagePathName)/sizeof(WCHAR), ImagePathName, &s))
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
          if (!SearchPathW(NULL, TempApplicationNameW, NULL, sizeof(ImagePathName)/sizeof(WCHAR), ImagePathName, &s))
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
/////////////////////////////////////////
        /*
         * Inspect the image to determine executable flavour
         */
        IO_STATUS_BLOCK IoStatusBlock;
        UNICODE_STRING ApplicationNameString;
        OBJECT_ATTRIBUTES ObjectAttributes;
        PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
        IMAGE_DOS_HEADER DosHeader;
        IO_STATUS_BLOCK Iosb;
        LARGE_INTEGER Offset;
        HANDLE hFile = NULL;

        DPRINT("Inspecting Image Header for image type id\n");

        // Find the application name
        if (!RtlDosPathNameToNtPathName_U((LPWSTR)lpApplicationName,
                &ApplicationNameString, NULL, NULL)) {
            return FALSE;
        }
        DPRINT("ApplicationName %S\n",ApplicationNameString.Buffer);

        InitializeObjectAttributes(&ObjectAttributes,
			      &ApplicationNameString,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      SecurityDescriptor);

        // Try to open the executable
        Status = NtOpenFile(&hFile,
			SYNCHRONIZE|FILE_EXECUTE|FILE_READ_DATA,
			&ObjectAttributes,
			&IoStatusBlock,
			FILE_SHARE_DELETE|FILE_SHARE_READ,
			FILE_SYNCHRONOUS_IO_NONALERT|FILE_NON_DIRECTORY_FILE);

        RtlFreeUnicodeString(&ApplicationNameString);

        if (!NT_SUCCESS(Status)) {
            DPRINT("Failed to open file\n");
            SetLastErrorByStatus(Status);
            return FALSE;
        }

        // Read the dos header
        Offset.QuadPart = 0;
        Status = ZwReadFile(hFile,
		      NULL,
		      NULL,
		      NULL,
		      &Iosb,
		      &DosHeader,
		      sizeof(DosHeader),
		      &Offset,
		      0);

        if (!NT_SUCCESS(Status)) {
            DPRINT("Failed to read from file\n");
            SetLastErrorByStatus(Status);
            return FALSE;
        }
        if (Iosb.Information != sizeof(DosHeader)) {
            DPRINT("Failed to read dos header from file\n");
            SetLastErrorByStatus(STATUS_INVALID_IMAGE_FORMAT);
            return FALSE;
        }

        // Check the DOS signature
        if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
            DPRINT("Failed dos magic check\n");
            SetLastErrorByStatus(STATUS_INVALID_IMAGE_FORMAT);
            return FALSE;
        }
        NtClose(hFile);

        DPRINT("Launching VDM...\n");
        return CreateProcessW(L"ntvdm.exe",
	         (LPWSTR)lpApplicationName,
	         lpProcessAttributes,
	         lpThreadAttributes,
	         bInheritHandles,
	         dwCreationFlags,
	         lpEnvironment,
	         lpCurrentDirectory,
	         lpStartupInfo,
	         lpProcessInformation);
   }
/////////////////////////////////////////
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
   if (Ppb->CurrentDirectoryHandle)
   {
      Status = NtDuplicateObject (NtCurrentProcess(),
	                 Ppb->CurrentDirectoryHandle,
			 hProcess,
			 &Ppb->CurrentDirectoryHandle,
			 0,
			 TRUE,
			 DUPLICATE_SAME_ACCESS);
   }

   if (Ppb->hConsole)
   {
      Status = NtDuplicateObject (NtCurrentProcess(), 
	                 Ppb->hConsole,
			 hProcess,
			 &Ppb->hConsole,
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
   NtQueryInformationProcess(hProcess,
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
   if (Sii.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI)
   {
      /* Do not create a console for GUI applications */
      dwCreationFlags &= ~CREATE_NEW_CONSOLE;
      dwCreationFlags |= DETACHED_PROCESS;
   }
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
   Ppb->hStdInput = NtCurrentPeb()->ProcessParameters->hStdInput;
   Ppb->hStdOutput = NtCurrentPeb()->ProcessParameters->hStdOutput;
   Ppb->hStdError = NtCurrentPeb()->ProcessParameters->hStdError;

   if (lpStartupInfo && (lpStartupInfo->dwFlags & STARTF_USESTDHANDLES))
   {
      if (lpStartupInfo->hStdInput)
	 Ppb->hStdInput = lpStartupInfo->hStdInput;
      if (lpStartupInfo->hStdOutput)
	 Ppb->hStdOutput = lpStartupInfo->hStdOutput;
      if (lpStartupInfo->hStdError)
	 Ppb->hStdError = lpStartupInfo->hStdError;
   }

   if (IsConsoleHandle(Ppb->hStdInput))
   {
      Ppb->hStdInput = CsrReply.Data.CreateProcessReply.InputHandle;
   }
   else
   {
      DPRINT("Duplicate input handle\n");
      Status = NtDuplicateObject (NtCurrentProcess(), 
	                          Ppb->hStdInput,
			          hProcess,
			          &Ppb->hStdInput,
			          0,
			          TRUE,
			          DUPLICATE_SAME_ACCESS);
      if(!NT_SUCCESS(Status))
      {
	 DPRINT("NtDuplicateObject failed, status %x\n", Status);
      }
   }

   if (IsConsoleHandle(Ppb->hStdOutput))
   {
      Ppb->hStdOutput = CsrReply.Data.CreateProcessReply.OutputHandle;
   }
   else
   {
      DPRINT("Duplicate output handle\n");
      Status = NtDuplicateObject (NtCurrentProcess(), 
	                          Ppb->hStdOutput,
			          hProcess,
			          &Ppb->hStdOutput,
			          0,
			          TRUE,
			          DUPLICATE_SAME_ACCESS);
      if(!NT_SUCCESS(Status))
      {
	 DPRINT("NtDuplicateObject failed, status %x\n", Status);
      }
   }
   if (IsConsoleHandle(Ppb->hStdError))
   {
      CsrRequest.Type = CSRSS_DUPLICATE_HANDLE;
      CsrRequest.Data.DuplicateHandleRequest.ProcessId = ProcessBasicInfo.UniqueProcessId;
      CsrRequest.Data.DuplicateHandleRequest.Handle = CsrReply.Data.CreateProcessReply.OutputHandle;
      Status = CsrClientCallServer(&CsrRequest, 
			  &CsrReply,
			  sizeof(CSRSS_API_REQUEST),
			  sizeof(CSRSS_API_REPLY));
      if (!NT_SUCCESS(Status) || !NT_SUCCESS(CsrReply.Status))
      {
	 Ppb->hStdError = INVALID_HANDLE_VALUE;
      }
      else
      {
         Ppb->hStdError = CsrReply.Data.DuplicateHandleReply.Handle;
      }
   }
   else
   {
      DPRINT("Duplicate error handle\n");
      Status = NtDuplicateObject (NtCurrentProcess(), 
	                          Ppb->hStdError,
			          hProcess,
			          &Ppb->hStdError,
			          0,
			          TRUE,
			          DUPLICATE_SAME_ACCESS);
      if(!NT_SUCCESS(Status))
      {
	 DPRINT("NtDuplicateObject failed, status %x\n", Status);
      }
   }

   /*
    * Initialize some other fields in the PPB
    */
   if (lpStartupInfo)
     {
       Ppb->dwFlags = lpStartupInfo->dwFlags;
       if (Ppb->dwFlags & STARTF_USESHOWWINDOW)
	 {
	   Ppb->wShowWindow = lpStartupInfo->wShowWindow;
	 }
       else
	 {
	   Ppb->wShowWindow = SW_SHOWDEFAULT;
	 }
       Ppb->dwX = lpStartupInfo->dwX;
       Ppb->dwY = lpStartupInfo->dwY;
       Ppb->dwXSize = lpStartupInfo->dwXSize;
       Ppb->dwYSize = lpStartupInfo->dwYSize;
       Ppb->dwFillAttribute = lpStartupInfo->dwFillAttribute;
     }
   else
     {
       Ppb->Flags = 0;
     }
   
   /*
    * Create Process Environment Block
    */
   DPRINT("Creating peb\n");

   KlInitPeb(hProcess, Ppb, &ImageBaseAddress);

   RtlDestroyProcessParameters (Ppb);

   Status = NtSetInformationProcess(hProcess,
				    ProcessImageFileName,
				    ImageFileName,
				    8);
   /*
    * Create the thread for the kernel
    */
   DPRINT("Creating thread for process (EntryPoint = 0x%.08x)\n",
    ImageBaseAddress + (ULONG)Sii.EntryPoint);
   hThread =  KlCreateFirstThread(hProcess,
				  lpThreadAttributes,
          &Sii,
          ImageBaseAddress + (ULONG)Sii.EntryPoint,
				  dwCreationFlags,
				  &lpProcessInformation->dwThreadId);
   if (hThread == INVALID_HANDLE_VALUE)
     {
	return FALSE;
     }

   lpProcessInformation->hProcess = hProcess;
   lpProcessInformation->hThread = hThread;

   return TRUE;
}

/* EOF */
