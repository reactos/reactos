/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/except.c
 * PURPOSE:         Exception functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
		    modified from WINE [ Onno Hovers, (onno@stack.urc.tue.nl) ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <windows.h>
#include <ddk/ntddk.h>


typedef LONG (STDCALL *LPTOP_LEVEL_EXCEPTION_FILTER)(
	struct _EXCEPTION_POINTERS *ExceptionInfo
 	);

UINT GlobalErrMode;
LPTOP_LEVEL_EXCEPTION_FILTER GlobalTopLevelExceptionFilter;

UINT GetErrorMode(void);



UINT GetErrorMode(void)
{
	return GlobalErrMode;
}

UINT 
STDCALL
SetErrorMode(  UINT uMode  )
{
	UINT OldErrMode = GetErrorMode();
	GlobalErrMode = uMode;
	return OldErrMode;
}

LPTOP_LEVEL_EXCEPTION_FILTER
STDCALL
SetUnhandledExceptionFilter(
    LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter
    )
{
    LPTOP_LEVEL_EXCEPTION_FILTER OldTopLevelExceptionFilter =
					 GlobalTopLevelExceptionFilter;
    lpTopLevelExceptionFilter = GlobalTopLevelExceptionFilter;
    return OldTopLevelExceptionFilter;
}












LONG 
STDCALL
UnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	char message[80];
	DWORD	dbgRet;
	HANDLE DebugPort;
	NTSTATUS errCode;

	DWORD DebuggerPresent;


	if(ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION) {
		// might check read only resource
		// Is there a debugger running ?
		errCode = NtQueryInformationProcess(NtCurrentProcess(),ProcessDebugPort,&DebugPort,sizeof(HANDLE),NULL);
  		if ( !NT_SUCCESS(errCode) ) {
			SetLastError(RtlNtStatusToDosError(errCode));
			return EXCEPTION_EXECUTE_HANDLER;
		}
		if ( DebugPort ) {
			//return EXCEPTION_CONTINUE_SEARCH;
		}   
    	if(GlobalTopLevelExceptionFilter != NULL) {
		dbgRet = GlobalTopLevelExceptionFilter(ExceptionInfo);
        	if(dbgRet == EXCEPTION_EXECUTE_HANDLER) 
        		return EXCEPTION_EXECUTE_HANDLER;
        	else if(dbgRet == EXCEPTION_CONTINUE_EXECUTION) 
         		return EXCEPTION_CONTINUE_EXECUTION;
		}

	}
	
	//if ( GetErrorMode() & SEM_NOGPFAULTERRORBOX == SEM_NOGPFAULTERRORBOX ) {
		// produce a stack trace or pop a message
		//sprintf( message, "Unhandled exception 0x%08lx at address 0x%08lx.",
             	//	ExceptionInfo->ExceptionRecord->ExceptionCode,
             	//	(DWORD)ExceptionInfo->ExceptionRecord->ExceptionAddress );
		//MessageBoxA( 0, message, "Error", MB_OK | MB_ICONHAND );
	
	//}
	// Returning EXCEPTION_EXECUTE_HANDLER means that the code in 
	// the __execept block will be executed. Normally this will end up in a
	// Terminate process.

	return EXCEPTION_EXECUTE_HANDLER;
	
}







