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

#define UNICODE
#include <windows.h>
#include <kernel32/proc.h>
#include <kernel32/thread.h>
#include <wstring.h>
#include <string.h>
#include <ddk/rtl.h>
#include <ddk/li.h>
#include <internal/i386/segment.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* GLOBALS *****************************************************************/

static NT_PEB CurrentPeb;
static PROCESSINFOW ProcessInfo;

WaitForInputIdleType  lpfnGlobalRegisterWaitForInputIdle;

VOID RegisterWaitForInputIdle(WaitForInputIdleType  lpfnRegisterWaitForInputIdle);

/* FUNCTIONS ****************************************************************/

WINBOOL STDCALL GetProcessId(HANDLE hProcess, LPDWORD lpProcessId);

VOID InitializePeb(PWSTR CommandLine)
{
   DPRINT("InitializePeb(CommandLine %x)\n",CommandLine);
   DPRINT("ProcessInfo.CommandLine %x\n",ProcessInfo.CommandLine);
   wcscpy(ProcessInfo.CommandLine, CommandLine);
   CurrentPeb.StartupInfo = &ProcessInfo;
}

NT_PEB *GetCurrentPeb(VOID)
{
   return(&CurrentPeb);
}

HANDLE STDCALL GetCurrentProcess(VOID)
{
	return (HANDLE)NtCurrentProcess();
}

HANDLE STDCALL GetCurrentThread(VOID)
{
	return (HANDLE)NtCurrentThread();
}

DWORD STDCALL GetCurrentProcessId(VOID)
{	
	return (DWORD)(GetTeb()->Cid).UniqueProcess;		
}

WINBOOL STDCALL GetExitCodeProcess(HANDLE hProcess, LPDWORD lpExitCode )
{
   NTSTATUS errCode;
   PROCESS_BASIC_INFORMATION ProcessBasic;
   ULONG BytesWritten;
   
   errCode = NtQueryInformationProcess(hProcess,
				       ProcessBasicInformation,
				       &ProcessBasic,
				       sizeof(PROCESS_BASIC_INFORMATION),
				       &BytesWritten);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }
   memcpy( lpExitCode ,&ProcessBasic.ExitStatus,sizeof(DWORD));
   return TRUE;	
}

WINBOOL STDCALL GetProcessId(HANDLE hProcess, LPDWORD lpProcessId )
{
   NTSTATUS errCode;
   PROCESS_BASIC_INFORMATION ProcessBasic;
   ULONG BytesWritten;

   errCode = NtQueryInformationProcess(hProcess,
				       ProcessBasicInformation,
				       &ProcessBasic,
				       sizeof(PROCESS_BASIC_INFORMATION),
				       &BytesWritten);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }
   memcpy( lpProcessId ,&ProcessBasic.UniqueProcessId,sizeof(DWORD));
   return TRUE;	
}

PWSTR InternalAnsiToUnicode(PWSTR Out, LPCSTR In, ULONG MaxLength)
{
   ULONG i;
   
   if (In == NULL)
     {
	return(NULL);
     }
   else
     {
	i = 0;
   	while ((*In)!=0 && i < MaxLength)
	  {
	     Out[i] = *In;
	     In++;
	     i++;
	  }
   	Out[i] = 0;
	return(Out);
     }
}

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
   ULONG i;
   
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

HANDLE STDCALL CreateFirstThread(HANDLE hProcess,
				 LPSECURITY_ATTRIBUTES lpThreadAttributes,
				 DWORD dwStackSize,
				 LPTHREAD_START_ROUTINE lpStartAddress,
				 LPVOID lpParameter,
				 DWORD dwCreationFlags,
				 LPDWORD lpThreadId,
				 PWSTR lpCommandLine)
{	
   NTSTATUS errCode;
   HANDLE ThreadHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   CLIENT_ID ClientId;
   CONTEXT ThreadContext;
   INITIAL_TEB InitialTeb;
   BOOLEAN CreateSuspended = FALSE;
   ULONG BaseAddress;
   ULONG BytesWritten;
   ULONG Temp;
   ULONG CommandLineLen;
   
   if (lpCommandLine == NULL)
     {
	lpCommandLine = "";
	CommandLineLen = 1;
     }
   else
     {
	CommandLineLen = wcslen(lpCommandLine) + 1;     
     }
   CommandLineLen = CommandLineLen * sizeof(WCHAR);
   CommandLineLen = (CommandLineLen & (~0x3)) + 4;
   DPRINT("CommandLineLen %d\n",CommandLineLen);
   
   
   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = NULL;
   ObjectAttributes.ObjectName = NULL;
   ObjectAttributes.Attributes = 0;
   if ( lpThreadAttributes != NULL ) {
      if ( lpThreadAttributes->bInheritHandle ) 
	ObjectAttributes.Attributes = OBJ_INHERIT;
      ObjectAttributes.SecurityDescriptor = lpThreadAttributes->lpSecurityDescriptor;
   }
   ObjectAttributes.SecurityQualityOfService = NULL;
   
   if ( ( dwCreationFlags & CREATE_SUSPENDED ) == CREATE_SUSPENDED )
		CreateSuspended = TRUE;
   else
     CreateSuspended = FALSE;

   BaseAddress = 0;
   ZwAllocateVirtualMemory(hProcess,
			   &BaseAddress,
			   0,
                           &dwStackSize,
			   MEM_COMMIT,
			   PAGE_READWRITE);
   

   memset(&ThreadContext,0,sizeof(CONTEXT));
   ThreadContext.Eip = lpStartAddress;
   ThreadContext.SegGs = USER_DS;
   ThreadContext.SegFs = USER_DS;
   ThreadContext.SegEs = USER_DS;
   ThreadContext.SegDs = USER_DS;
   ThreadContext.SegCs = USER_CS;
   ThreadContext.SegSs = USER_DS;        
   ThreadContext.Esp = BaseAddress + dwStackSize - CommandLineLen - 8;   
   ThreadContext.EFlags = (1<<1) + (1<<9);

   NtWriteVirtualMemory(hProcess,
			BaseAddress + dwStackSize - CommandLineLen,
			lpCommandLine,
			CommandLineLen,
			&BytesWritten);
   Temp = BaseAddress + dwStackSize - CommandLineLen;
   NtWriteVirtualMemory(hProcess,
			BaseAddress + dwStackSize - CommandLineLen - 4,
			&Temp,
			sizeof(Temp),
			&BytesWritten);

   errCode = NtCreateThread(&ThreadHandle,
			    THREAD_ALL_ACCESS,
			    &ObjectAttributes,
			    hProcess,
			    &ClientId,
			    &ThreadContext,
			    &InitialTeb,
			    CreateSuspended);
   if ( lpThreadId != NULL )
     memcpy(lpThreadId, &ClientId.UniqueThread,sizeof(ULONG));
   
   return ThreadHandle;
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
   HANDLE hFile, hSection, hProcess, hThread;
   KPRIORITY PriorityClass;
   OBJECT_ATTRIBUTES ObjectAttributes;
   IO_STATUS_BLOCK IoStatusBlock;
   BOOLEAN CreateSuspended;
   NTSTATUS errCode;
   UNICODE_STRING ApplicationNameString;
   LPTHREAD_START_ROUTINE  lpStartAddress = NULL;
   LPVOID  lpParameter = NULL;
   PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
   WCHAR TempApplicationName[256];
   WCHAR TempFileName[256];
   WCHAR TempDirectoryName[256];
   WCHAR TempCommandLine[256];
   ULONG i;
   ULONG BaseAddress;
   ULONG Size;
   LARGE_INTEGER SectionOffset;
   
   DPRINT("CreateProcessW(lpApplicationName '%w', lpCommandLine '%w')\n",
	   lpApplicationName,lpCommandLine);
   
   wcscpy(TempCommandLine, lpCommandLine);
   
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
	GetCurrentDirectoryW(MAX_PATH,TempDirectoryName);
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
   
   errCode = NtOpenFile(&hFile,
			SYNCHRONIZE|FILE_EXECUTE|FILE_READ_DATA,
			&ObjectAttributes,
			&IoStatusBlock,
			FILE_SHARE_DELETE|FILE_SHARE_READ,
			FILE_SYNCHRONOUS_IO_NONALERT|FILE_NON_DIRECTORY_FILE);
   
   if ( !NT_SUCCESS(errCode) ) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }

   errCode = NtCreateSection(&hSection,
			     SECTION_ALL_ACCESS,
			     NULL,
			     NULL,
			     PAGE_EXECUTE,
			     SEC_IMAGE,
			     hFile);
   NtClose(hFile);

   if ( !NT_SUCCESS(errCode) ) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }
			
   errCode = NtCreateProcess(&hProcess,
			     PROCESS_ALL_ACCESS, 
			     NULL,
			     NtCurrentProcess(),
			     bInheritHandles,
			     NULL,
			     NULL,
			     NULL);
   
   BaseAddress = (PVOID)0x10000;
   LARGE_INTEGER_QUAD_PART(SectionOffset) = 0;
   Size = 0x10000;
   NtMapViewOfSection(hSection,
		      hProcess,
		      &BaseAddress,
		      0,
                      Size,
		      &SectionOffset,
		      &Size,
		      0,
		      MEM_COMMIT,
		      PAGE_READWRITE);

   
   NtClose(hSection);

   if ( !NT_SUCCESS(errCode) ) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }
   
#if 0
   PriorityClass = NORMAL_PRIORITY_CLASS;	
   NtSetInformationProcess(hProcess,
			   ProcessBasePriority,
			   &PriorityClass,
			   sizeof(KPRIORITY));
#endif
   
   DPRINT("Creating thread for process\n");
   lpStartAddress = BaseAddress;
   hThread =  CreateFirstThread(hProcess,	
				lpThreadAttributes,
				16384, // 3 page ??
				lpStartAddress,	
				lpParameter,	
				dwCreationFlags,
				&lpProcessInformation->dwThreadId,
				TempCommandLine);

   if ( hThread == NULL )
     return FALSE;
      
   lpProcessInformation->hProcess = hProcess;
   lpProcessInformation->hThread = hThread;

   GetProcessId(hProcess,&lpProcessInformation->dwProcessId);

   return TRUE;				
}



HANDLE STDCALL OpenProcess(DWORD dwDesiredAccess,
			   WINBOOL bInheritHandle,
			   DWORD dwProcessId)
{
   NTSTATUS errCode;
   HANDLE ProcessHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   CLIENT_ID ClientId ;
   
   ClientId.UniqueProcess = (HANDLE)dwProcessId;
   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = (HANDLE)NULL;
   ObjectAttributes.SecurityDescriptor = NULL;
   ObjectAttributes.SecurityQualityOfService = NULL;
   
   if ( bInheritHandle == TRUE )
     ObjectAttributes.Attributes = OBJ_INHERIT;
   else
     ObjectAttributes.Attributes = 0;
   
   errCode = NtOpenProcess(&ProcessHandle, 
			   dwDesiredAccess, 
			   &ObjectAttributes, 
			   &ClientId);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return NULL;
     }
   return ProcessHandle;
}













UINT WinExec (LPCSTR  lpCmdLine, UINT  uCmdShow)
{
   STARTUPINFO StartupInfo;
   PROCESS_INFORMATION  ProcessInformation; 	
   HINSTANCE hInst;
   DWORD dosErr;

   StartupInfo.cb = sizeof(STARTUPINFO);
   StartupInfo.wShowWindow = uCmdShow ;
   StartupInfo.dwFlags = 0;
   
   hInst = (HINSTANCE)CreateProcessA(NULL,
				     (PVOID)lpCmdLine,
				     NULL,
				     NULL,
				     FALSE,
				     0,
				     NULL,
				     NULL,
				     &StartupInfo, 
				     &ProcessInformation);
   if ( hInst == NULL ) 
     {
	dosErr = GetLastError();
	return dosErr;
     }
   if ( lpfnGlobalRegisterWaitForInputIdle != NULL )
     lpfnGlobalRegisterWaitForInputIdle(ProcessInformation.hProcess,10000);
   NtClose(ProcessInformation.hProcess);
   NtClose(ProcessInformation.hThread);
   return 0;	
}



VOID RegisterWaitForInputIdle(WaitForInputIdleType  lpfnRegisterWaitForInputIdle)
{
	lpfnGlobalRegisterWaitForInputIdle = lpfnRegisterWaitForInputIdle; 
	return;
}

DWORD STDCALL WaitForInputIdle(HANDLE hProcess,	
			       DWORD dwMilliseconds)
{
   return 0;
}

VOID 
STDCALL
Sleep(
    DWORD dwMilliseconds 	
   )
{
	SleepEx(dwMilliseconds,FALSE);
	return;
}

DWORD 
STDCALL
SleepEx(
    DWORD dwMilliseconds,	
    BOOL bAlertable 	
   )
{
	TIME Interval;
	NTSTATUS errCode;	

	Interval.LowPart = dwMilliseconds * 1000;
	Interval.HighPart = 0;
	errCode = NtDelayExecution(bAlertable,&Interval);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return -1;
	}
	return 0;
}





VOID
STDCALL
GetStartupInfoW(
    LPSTARTUPINFO  lpStartupInfo 	
   )
{
	NT_PEB *pPeb = GetCurrentPeb();

	if (lpStartupInfo == NULL ) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return;
	}

	lpStartupInfo->cb = sizeof(STARTUPINFO);
    	lstrcpyW(lpStartupInfo->lpDesktop, pPeb->StartupInfo->Desktop); 
    	lstrcpyW(lpStartupInfo->lpTitle, pPeb->StartupInfo->Title);
    	lpStartupInfo->dwX = pPeb->StartupInfo->dwX; 
    	lpStartupInfo->dwY = pPeb->StartupInfo->dwY; 
    	lpStartupInfo->dwXSize = pPeb->StartupInfo->dwXSize; 
    	lpStartupInfo->dwYSize = pPeb->StartupInfo->dwYSize; 
    	lpStartupInfo->dwXCountChars = pPeb->StartupInfo->dwXCountChars; 
    	lpStartupInfo->dwYCountChars = pPeb->StartupInfo->dwYCountChars; 
    	lpStartupInfo->dwFillAttribute = pPeb->StartupInfo->dwFillAttribute; 
    	lpStartupInfo->dwFlags = pPeb->StartupInfo->dwFlags; 
    	lpStartupInfo->wShowWindow = pPeb->StartupInfo->wShowWindow; 
    	//lpStartupInfo->cbReserved2 = pPeb->StartupInfo->cbReserved; 
	//lpStartupInfo->lpReserved = pPeb->StartupInfo->lpReserved1; 
    	//lpStartupInfo->lpReserved2 = pPeb->StartupInfo->lpReserved2; 
	
    	lpStartupInfo->hStdInput = pPeb->StartupInfo->hStdInput; 
    	lpStartupInfo->hStdOutput = pPeb->StartupInfo->hStdOutput; 
    	lpStartupInfo->hStdError = pPeb->StartupInfo->hStdError; 
	
	
	
	return;
  


}


VOID
STDCALL
GetStartupInfoA(
    LPSTARTUPINFO  lpStartupInfo 	
   )
{
	NT_PEB *pPeb = GetCurrentPeb();
	ULONG i = 0;
	if (lpStartupInfo == NULL ) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return;
	}

	
	lpStartupInfo->cb = sizeof(STARTUPINFO);
	i = 0;
  
   	while ((pPeb->StartupInfo->Desktop[i])!=0 && i < MAX_PATH)
     	{
		lpStartupInfo->lpDesktop[i] = (unsigned char)pPeb->StartupInfo->Desktop[i];
		i++;
     	}
  	lpStartupInfo->lpDesktop[i] = 0;
    	
	 i = 0;
	while ((pPeb->StartupInfo->Title[i])!=0 && i < MAX_PATH)
     	{
		lpStartupInfo->lpTitle[i] = (unsigned char)pPeb->StartupInfo->Title[i];
		i++;
     	}
  	lpStartupInfo->lpTitle[i] = 0;

    	lpStartupInfo->dwX = pPeb->StartupInfo->dwX; 
    	lpStartupInfo->dwY = pPeb->StartupInfo->dwY; 
    	lpStartupInfo->dwXSize = pPeb->StartupInfo->dwXSize; 
    	lpStartupInfo->dwYSize = pPeb->StartupInfo->dwYSize; 
    	lpStartupInfo->dwXCountChars = pPeb->StartupInfo->dwXCountChars; 
    	lpStartupInfo->dwYCountChars = pPeb->StartupInfo->dwYCountChars; 
    	lpStartupInfo->dwFillAttribute = pPeb->StartupInfo->dwFillAttribute; 
    	lpStartupInfo->dwFlags = pPeb->StartupInfo->dwFlags; 
    	lpStartupInfo->wShowWindow = pPeb->StartupInfo->wShowWindow; 
    	//lpStartupInfo->cbReserved2 = pPeb->StartupInfo->cbReserved; 
	//lpStartupInfo->lpReserved = pPeb->StartupInfo->lpReserved1; 
    	//lpStartupInfo->lpReserved2 = pPeb->StartupInfo->lpReserved2; 
	
    	lpStartupInfo->hStdInput = pPeb->StartupInfo->hStdInput; 
    	lpStartupInfo->hStdOutput = pPeb->StartupInfo->hStdOutput; 
    	lpStartupInfo->hStdError = pPeb->StartupInfo->hStdError; 
	
	return;
}

BOOL 
STDCALL
FlushInstructionCache(
  

    HANDLE  hProcess,	
    LPCVOID  lpBaseAddress,	
    DWORD  dwSize 	
   )
{
	NTSTATUS errCode;
	errCode = NtFlushInstructionCache(hProcess,(PVOID)lpBaseAddress,dwSize);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;
}

VOID STDCALL ExitProcess(UINT uExitCode) 
{	
   NtTerminateProcess(NtCurrentProcess() ,uExitCode);
}

VOID
STDCALL
FatalAppExitA(
	      UINT uAction,
	      LPCSTR lpMessageText
	      )
{
	WCHAR MessageTextW[MAX_PATH];
	UINT i;
	i = 0;
   	while ((*lpMessageText)!=0 && i < 35)
     	{
		MessageTextW[i] = *lpMessageText;
		lpMessageText++;
		i++;
     	}
   	MessageTextW[i] = 0;
	
	return FatalAppExitW(uAction,MessageTextW);
}


	
VOID STDCALL FatalAppExitW(UINT uAction, LPCWSTR lpMessageText)
{
   return;	
}


