#include <process.h>
/*
 * Win32 Process Api functions
 * Author: Boudewijn Dekker
 * to do: many more to add ..
 * open matters:  ProcessInformation should be per process
		and part of larger structure.
 */




#define NT_CURRENT_PROCESS 	0xFFFFFFFF
#define NT_CURRENT_THREAD	0xFFFFFFFE


WINBASEAPI
HANDLE
WINAPI
GetCurrentProcess()
{
	return NT_CURRENT_PROCESS;
}

WINBASEAPI
DWORD
WINAPI
GetCurrentProcessId()
{
	return GetTeb()->dwProcessId; 
}


WINBASEAPI
HANDLE
WINAPI
GetCurrentThread()
{
	return NT_CURRENT_PROCESS;
}

WINBASEAPI
DWORD
WINAPI
GetCurrentThreadId()
{

	return GetTeb()->dwThreadId; 
}

UINT	WinExec ( LPCSTR  lpCmdLine,	UINT  uCmdShow 	)
{
	STARTUPINFO StartUpInfo;
	StartupInfo.wShowWindow = uCmdShow ;
	PROCESS_INFORMATION  ProcessInformation; 	
	StartupInfo.cb = sizeof(STARTUPINFO);
	StartupInfo.dwFlags = 0;

	HINSTANCE hInst = CreateProcess(NULL,lpCmdLine,NULL,NULL,FALSE,NULL,NULL,NULL,&StartupInfo, &ProcessInformation);
	if ( hInst == NULL ) {
		dosErr = GetLastError();
		if ( dosErr == 0x000000C1 )
			return 0; // out of resources 
		else
			return dosErr;
	}
	if ( lpfuncGlobalRegisterWaitForInputIdle != NULL )
		lpfuncGlobalRegisterWaitForInputIdle(0x00007530,ProcessInformation->hProcess);
	NtClose(ProcessInformation->hProcess);
	NtClose(ProcessInformation->hThread);
	return;
	
}



VOID RegisterWaitForInputIdle(lpfuncRegisterWaitForInputIdle)
{
	lpfuncGlobalRegisterWaitForInputIdle = lpfuncRegisterWaitForInputIdle; //77F450C8
}


#define STARTF_IO 0x00000700

VOID
STDCALL
GetStartupInfoW(
    LPSTARTUPINFO  lpStartupInfo 	
   )
{
	NT_PEB *pPeb = GetTeb()->pPeb;

	if (lpStartupInfo == NULL ) {
		SetLastError(-1);
		return;
	}

	
	lpStartupInfo->cb = pPeb->pPebInfo->cb; 
    	lpStartupInfo->lpReserved = pPeb->pPebInfo->lpReserved1; 
    	lpStartupInfo->lpDesktop = pPeb->pPebInfo->lpDesktop; 
    	lpStartupInfo->lpTitle = pPeb->pPebInfo->lpTitle; 
    	lpStartupInfo->dwX = pPeb->pPebInfo->dwX; 
    	lpStartupInfo->dwY = pPeb->pPebInfo->dwY; 
    	lpStartupInfo->dwXSize = pPeb->pPebInfo->dwXSize; 
    	lpStartupInfo->dwYSize = pPeb->pPebInfo->dwYSize; 
    	lpStartupInfo->dwXCountChars = pPeb->pPebInfo->dwXCountChars; 
    	lpStartupInfo->dwYCountChars = pPeb->pPebInfo->dwYCountChars; 
    	lpStartupInfo->dwFillAttribute = pPeb->pPebInfo->dwFillAttribute; 
    	lpStartupInfo->dwFlags = pPeb->pPebInfo->dwFlags; 
    	lpStartupInfo->wShowWindow = pPeb->pPebInfo->wShowWindow; 
    	lpStartupInfo->cbReserved2 = pPeb->pPebInfo->cbReserved; 
    	lpStartupInfo->lpReserved2 = pPeb->pPebInfo->lpReserved2; 
	if ( lpStartupInfo.dwFlags == STARTF_IO ) {
    		lpStartupInfo->hStdInput = pPeb->pPebInfo->hStdInput; 
    		lpStartupInfo->hStdOutput = pPeb->pPebInfo->hStdOutput; 
    		lpStartupInfo->hStdError = pPeb->pPebInfo->hStdError; 
	}


}


BOOL FlushInstructionCache(
  

    HANDLE  hProcess,	
    LPCVOID  lpBaseAddress,	
    DWORD  dwSize 	
   )
{
	errCode = NtFlushInstructionCache(hProcess,lpBaseAddress,dwSize);
	if ( errCode < 0 ) {
		CompatibleError(errCode);
		return FALSE;
	}
}
