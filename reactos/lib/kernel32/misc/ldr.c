#define WIN32_NO_STATUS
#define WIN32_NO_PEHDR
#include <windows.h>
#include <ddk/ntddk.h>
#include <pe.h>
#include <ntdll/ldr.h>
#if 0
typedef struct _DLL
{
   PIMAGE_NT_HEADERS Headers;
   PVOID BaseAddress;
   HANDLE SectionHandle;
   struct _DLL* Prev;
   struct _DLL* Next;
} DLL, *PDLL;
#endif

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
		lpstrcpy(lpDllName,lpLibFileName);
		lpDllName[i-1] = 0;
	}
	else if (i > 3 && lpLibFileName[i-3] != '.' ) {
		lpDllName = HeapAlloc(GetProcessHeap(),0,i+4);
		lpstrcpy(lpDllName,lpLibFileName);
		lpstrcat(lpDllName,".dll");	
	}
	else {
		lpDllName = HeapAlloc(GetProcessHeap(),0,i+1);
		lpstrcpy(lpDllName,lpLibFileName);
	}
	
	if ( !NT_SUCCESS(LdrLoadDll(&hInst,lpDllName ))
	{
		return NULL;
	}
	
	return hInst;
}


FARPROC GetProcAddress( HMODULE hModule, LPCSTR lpProcName )
{
	
	FARPROC fnExp;
	ULONG Ordinal;
	
	if ( LOWORD(lpProcName ) 
		fnExp = LdrGetExportByOrdinal (hModule,Ordinal);
	else
		fnExp = LdrGetExportByName (hModule,lpProcName);

	return fnExp;
}

WINBOOL FreeLibrary( HMODULE hLibModule )
{
	LdrUnloadDll(hLibModule);
	return TRUE;
}


HMODULE GetModuleHandle ( LPCTSTR lpModuleName )
{
}