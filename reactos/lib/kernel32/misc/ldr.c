#define WIN32_NO_STATUS
#define WIN32_NO_PEHDR
#include <windows.h>
#include <ddk/ntddk.h>
#include <pe.h>
#include <ntdll/ldr.h>


HINSTANCE LoadLibraryA( LPCSTR lpLibFileName )
{
	HINSTANCE hInst;
	int i;
	LPSTR lpDllName;
	
	
	if ( lpLibFileName == NULL )
		return NULL;
		
	i = lstrlen(lpLibFileName);
	if ( lpLibFileName[i-1] == '.' ) {
		lpDllName = HeapAlloc(GetProcessHeap(),0,i+1);
		lstrcpy(lpDllName,lpLibFileName);
		lpDllName[i-1] = 0;
	}
	else if (i > 3 && lpLibFileName[i-3] != '.' ) {
		lpDllName = HeapAlloc(GetProcessHeap(),0,i+4);
		lstrcpy(lpDllName,lpLibFileName);
		lstrcat(lpDllName,".dll");	
	}
	else {
		lpDllName = HeapAlloc(GetProcessHeap(),0,i+1);
		lstrcpy(lpDllName,lpLibFileName);
	}
	
	if ( !NT_SUCCESS(LdrLoadDll((PDLL *)&hInst,lpDllName )))
	{
		return NULL;
	}
	
	return hInst;
}


FARPROC GetProcAddress( HMODULE hModule, LPCSTR lpProcName )
{
	
	FARPROC fnExp;
	
	if ( HIWORD(lpProcName )  != 0 )
		fnExp = LdrGetExportByName (hModule,(LPSTR)lpProcName);
	else
		fnExp = LdrGetExportByOrdinal (hModule,(ULONG)lpProcName);

	return fnExp;
}

WINBOOL FreeLibrary( HMODULE hLibModule )
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

HMODULE GetModuleHandleA ( LPCSTR lpModuleName )
{
	return LoadLibraryA(lpModuleName);
}