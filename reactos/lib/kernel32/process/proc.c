/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/proc/proc.c
 * PURPOSE:         Process functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */
#define UNICODE
#include <windows.h>
#include <kernel32/proc.h>
#include <kernel32/thread.h>
#include <wstring.h>
#include <string.h>
#include <ddk/rtl.h>
#include <ddk/li.h>

NT_PEB CurrentPeb;



WaitForInputIdleType  lpfnGlobalRegisterWaitForInputIdle;

VOID RegisterWaitForInputIdle(WaitForInputIdleType  lpfnRegisterWaitForInputIdle);

wchar_t **CommandLineToArgvW(LPCWSTR   lpCmdLine, int * pNumArgs );

WINBOOL 
STDCALL
GetProcessId(HANDLE hProcess,	LPDWORD lpProcessId );




NT_PEB *GetCurrentPeb(VOID)
{
		return &CurrentPeb;
	
}

HANDLE STDCALL GetCurrentProcess(VOID)
{
	return (HANDLE)NtCurrentProcess();
}

HANDLE STDCALL GetCurrentThread(VOID)
{
	return (HANDLE)NtCurrentThread();
}

DWORD 
STDCALL
GetCurrentProcessId(VOID)
{
	
	return (DWORD)(GetTeb()->Cid).UniqueProcess;
	
	
}

unsigned char CommandLineA[MAX_PATH];

LPSTR
STDCALL
GetCommandLineA(
		VOID
		)
{
	WCHAR *CommandLineW;
	ULONG i = 0;
  	
	CommandLineW = GetCommandLineW();
   	while ((CommandLineW[i])!=0 && i < MAX_PATH)
     	{
		CommandLineA[i] = (unsigned char)CommandLineW[i];
		i++;
     	}
   	CommandLineA[i] = 0;
	return CommandLineA;
}
LPWSTR
STDCALL
GetCommandLineW(
		VOID
		)
{
	return GetCurrentPeb()->StartupInfo->CommandLine;
}

wchar_t **CommandLineToArgvW(LPCWSTR   lpCmdLine, int * pNumArgs )
{
	return NULL;
}	



WINBOOL 
STDCALL
GetExitCodeProcess(
    HANDLE hProcess,	
    LPDWORD lpExitCode 
   )
{
	NTSTATUS errCode;
	PROCESS_BASIC_INFORMATION ProcessBasic;
	ULONG BytesWritten;

	errCode = NtQueryInformationProcess(hProcess,ProcessBasicInformation,&ProcessBasic,sizeof(PROCESS_BASIC_INFORMATION),&BytesWritten);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	memcpy( lpExitCode ,&ProcessBasic.ExitStatus,sizeof(DWORD));
	return TRUE;
	
}

WINBOOL 
STDCALL
GetProcessId(
    HANDLE hProcess,	
    LPDWORD lpProcessId 
   )
{
	NTSTATUS errCode;
	PROCESS_BASIC_INFORMATION ProcessBasic;
	ULONG BytesWritten;

	errCode = NtQueryInformationProcess(hProcess,ProcessBasicInformation,&ProcessBasic,sizeof(PROCESS_BASIC_INFORMATION),&BytesWritten);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	memcpy( lpProcessId ,&ProcessBasic.UniqueProcessId,sizeof(DWORD));
	return TRUE;
	
}

WINBOOL
STDCALL
CreateProcessA(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    WINBOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFO lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
	WCHAR ApplicationNameW[MAX_PATH];
	WCHAR CommandLineW[MAX_PATH];
	WCHAR CurrentDirectoryW[MAX_PATH];
	
	
   	ULONG i;
  
	i = 0;
   	while ((*lpApplicationName)!=0 && i < MAX_PATH)
     	{
		ApplicationNameW[i] = *lpApplicationName;
		lpApplicationName++;
		i++;
     	}
   	ApplicationNameW[i] = 0;


	i = 0;
   	while ((*lpCommandLine)!=0 && i < MAX_PATH)
     	{
		CommandLineW[i] = *lpCommandLine;
		lpCommandLine++;
		i++;
     	}
   	CommandLineW[i] = 0;

	i = 0;
   	while ((*lpCurrentDirectory)!=0 && i < MAX_PATH)
     	{
		CurrentDirectoryW[i] = *lpCurrentDirectory;
		lpCurrentDirectory++;
		i++;
     	}
   	CurrentDirectoryW[i] = 0;
	
	return CreateProcessW(ApplicationNameW,CommandLineW, lpProcessAttributes,lpThreadAttributes,
		bInheritHandles,dwCreationFlags,lpEnvironment,CurrentDirectoryW,lpStartupInfo,
		lpProcessInformation);
		
		
}


WINBOOL
STDCALL
CreateProcessW(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    WINBOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFO lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
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
	
	hFile = NULL;

	ApplicationNameString.Length = lstrlenW(lpApplicationName)*sizeof(WCHAR);
   
   	ApplicationNameString.Buffer = (WCHAR *)lpApplicationName;
   	ApplicationNameString.MaximumLength = ApplicationNameString.Length+sizeof(WCHAR);


	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
	ObjectAttributes.RootDirectory = NULL;
	
	

	if ( lpProcessAttributes != NULL ) {
		if ( lpProcessAttributes->bInheritHandle ) 
			ObjectAttributes.Attributes = OBJ_INHERIT;
		else
			ObjectAttributes.Attributes = 0;
		ObjectAttributes.SecurityDescriptor = lpProcessAttributes->lpSecurityDescriptor;
	}
	ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

	errCode = NtOpenFile(&hFile,(SYNCHRONIZE|FILE_EXECUTE), &ObjectAttributes,
		&IoStatusBlock,(FILE_SHARE_DELETE|FILE_SHARE_READ),(FILE_SYNCHRONOUS_IO_NONALERT|FILE_NON_DIRECTORY_FILE));
	
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}

	errCode = NtCreateSection(&hSection,SECTION_ALL_ACCESS,NULL,NULL,PAGE_EXECUTE,SEC_IMAGE,hFile);
	NtClose(hFile);

	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
		
	
	if ( lpProcessAttributes != NULL ) {
		if ( lpProcessAttributes->bInheritHandle ) 
			ObjectAttributes.Attributes = OBJ_INHERIT;
		else
			ObjectAttributes.Attributes = 0;
		ObjectAttributes.SecurityDescriptor = lpProcessAttributes->lpSecurityDescriptor;
	}

	errCode = NtCreateProcess(&hProcess,PROCESS_ALL_ACCESS, &ObjectAttributes,NtCurrentProcess(),bInheritHandles,hSection,NULL,NULL);
	NtClose(hSection);

	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}

	PriorityClass = NORMAL_PRIORITY_CLASS;	
	NtSetInformationProcess(hProcess,ProcessBasePriority,&PriorityClass,sizeof(KPRIORITY));

	if ( ( dwCreationFlags & CREATE_SUSPENDED  ) ==  CREATE_SUSPENDED)
		CreateSuspended = TRUE;
	else
		CreateSuspended = FALSE;

	hThread =  CreateRemoteThread(
  		hProcess,	
 		lpThreadAttributes,
    		4096, // 1 page ??	
    		lpStartAddress,	
    		lpParameter,	
    		CREATE_SUSPENDED,
    		&lpProcessInformation->dwThreadId
   	);


	if ( hThread == NULL )
		return FALSE;

	
	
	lpProcessInformation->hProcess = hProcess;
	lpProcessInformation->hThread = hThread;

	

	
	GetProcessId(hProcess,&lpProcessInformation->dwProcessId);

	

	return TRUE;
				
	
 }



HANDLE
STDCALL
OpenProcess(
	    DWORD dwDesiredAccess,
	    WINBOOL bInheritHandle,
	    DWORD dwProcessId
	    )
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

	errCode = NtOpenProcess ( &ProcessHandle, dwDesiredAccess, &ObjectAttributes, &ClientId);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return NULL;
	}
	return ProcessHandle;
}













UINT	WinExec ( LPCSTR  lpCmdLine,	UINT  uCmdShow 	)
{
	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION  ProcessInformation; 	

	HINSTANCE hInst;
	DWORD dosErr;



	StartupInfo.cb = sizeof(STARTUPINFO);
	StartupInfo.wShowWindow = uCmdShow ;
	StartupInfo.dwFlags = 0;

 	hInst = (HINSTANCE)CreateProcessA(NULL,(PVOID)lpCmdLine,NULL,NULL,FALSE,0,NULL,NULL,&StartupInfo, &ProcessInformation);
	if ( hInst == NULL ) {
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

DWORD 
STDCALL
WaitForInputIdle(
    HANDLE hProcess,	
    DWORD dwMilliseconds 	
   )
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
	LARGE_INTEGER Interval;
	NTSTATUS errCode;	
	SET_LARGE_INTEGER_LOW_PART(Interval,dwMilliseconds*1000);

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
    	lstrcpyW(lpStartupInfo->lpDesktop,(WCHAR *) pPeb->StartupInfo->Desktop); 
    	lstrcpyW(lpStartupInfo->lpTitle, (WCHAR *)pPeb->StartupInfo->Title);
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
		lpStartupInfo->lpDesktop[i] = (char)pPeb->StartupInfo->Desktop[i];
		i++;
     	}
  	lpStartupInfo->lpDesktop[i] = 0;
    	
	 i = 0;
	while ((pPeb->StartupInfo->Title[i])!=0 && i < MAX_PATH)
     	{
		lpStartupInfo->lpTitle[i] = (char)pPeb->StartupInfo->Title[i];
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

VOID
STDCALL
ExitProcess(
	    UINT uExitCode
	    ) 
{
	
	NtTerminateProcess(
		NtCurrentProcess() ,
		uExitCode
	);
	
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


	
VOID
STDCALL
FatalAppExitW(
    UINT uAction,
    LPCWSTR lpMessageText
    )
{
	return;	
}


HMODULE
STDCALL
GetModuleHandleA(
		 LPCSTR lpModuleName
		 )
{
	if ( lpModuleName == NULL )
		return  0x00010000; // starting address of current module
	else
		return NULL; // should return the address of the specified module

	return NULL;

}

HMODULE
STDCALL
GetModuleHandleW(
    LPCWSTR lpModuleName
    )
{
	if ( lpModuleName == NULL )
		return  0x00010000; // starting address of current module
	else
		return NULL; // should return the address of the specified module

	return NULL;
}
	

