/* $Id: ldr.c,v 1.4 1999/11/17 21:30:00 ariadne Exp $
 *
 * COPYRIGHT: See COPYING in the top level directory
 * PROJECT  : ReactOS user mode libraries
 * MODULE   : kernel32.dll
 * FILE     : reactos/lib/kernel32/misc/ldr.c
 * AUTHOR   : Boudewijn Dekker
 */
#define WIN32_NO_STATUS
#define WIN32_NO_PEHDR
#include <windows.h>
#include <ddk/ntddk.h>
#include <pe.h>
#include <ntdll/ldr.h>


HINSTANCE
STDCALL
LoadLibraryA( LPCSTR lpLibFileName )
{
	HINSTANCE hInst;
	int i;
	LPSTR lpDllName;
	NTSTATUS Status;
	
	
	if ( lpLibFileName == NULL )
		return NULL;
		
	i = lstrlen(lpLibFileName);
// full path specified
	if ( lpLibFileName[2] == ':' ) {
		lpDllName = HeapAlloc(GetProcessHeap(),0,i+3);
		lstrcpyA(lpDllName,"\\??\\");
		lstrcatA(lpDllName,lpLibFileName);
	}
// point at the end means no extension 
	else if ( lpLibFileName[i-1] == '.' ) {
		lpDllName = HeapAlloc(GetProcessHeap(),0,i+1);
		lstrcpyA(lpDllName,lpLibFileName);
		lpDllName[i-1] = 0;
	}
// no extension
	else if (i > 3 && lpLibFileName[i-3] != '.' ) {
		lpDllName = HeapAlloc(GetProcessHeap(),0,i+4);
		lstrcpyA(lpDllName,lpLibFileName);
		lstrcatA(lpDllName,".dll");	
	}
	else {
		lpDllName = HeapAlloc(GetProcessHeap(),0,i+1);
		lstrcpyA(lpDllName,lpLibFileName);
	}
	
	Status = LdrLoadDll((PDLL *)&hInst,lpDllName );
	HeapFree(GetProcessHeap(),0,lpDllName);
	if ( !NT_SUCCESS(Status))
	{
		SetLastError(RtlNtStatusToDosError(Status));
		return NULL;
	}
	
	return hInst;
}


FARPROC
STDCALL
GetProcAddress( HMODULE hModule, LPCSTR lpProcName )
{
	
	FARPROC fnExp;
	
	if ( HIWORD(lpProcName )  != 0 )
		fnExp = LdrGetExportByName (hModule,(LPSTR)lpProcName);
	else
		fnExp = LdrGetExportByOrdinal (hModule,(ULONG)lpProcName);

	return fnExp;
}


WINBOOL
STDCALL
FreeLibrary( HMODULE hLibModule )
{
	LdrUnloadDll(hLibModule);
	return TRUE;
}

VOID
STDCALL
FreeLibraryAndExitThread(
			 HMODULE hLibModule,
			 DWORD dwExitCode
			 )
{

	if ( FreeLibrary(hLibModule) )
		ExitThread(dwExitCode);
	return;
}


HMODULE
STDCALL
GetModuleHandleA ( LPCSTR lpModuleName )
{
	int len = 0;
	HMODULE hModule;
	WINBOOL restore = FALSE;
	if ( lpModuleName != NULL ) {

		len = lstrlenA(lpModuleName);
		if ( len > 0 && lpModuleName[len-1] == '.' ) {
			lpModuleName[len-1] = 0;
			restore = TRUE;
		}
	}
	hModule =  LoadLibraryA(lpModuleName);
	if ( restore == TRUE )
		lpModuleName[len-1] = '.';


	return hModule;
}


/* EOF */
