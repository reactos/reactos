#define WIN32_NO_STATUS
#define WIN32_NO_PEHDR
#include <windows.h>
#include <ddk/ntddk.h>
#include <pe.h>
#include <ntdll/ldr.h>
#include <kernel32/kernel32.h>


HRSRC
STDCALL
FindResourceA (
	HINSTANCE	hModule,
	LPCSTR		lpName,
	LPCSTR		lpType
	)
{
	return FindResourceExA (hModule, lpName, lpType, 0);
}

HRSRC
STDCALL
FindResourceExA(
	HINSTANCE	hModule,
	LPCSTR		lpType,
	LPCSTR		lpName,
	WORD		wLanguage
	)
{
	UNICODE_STRING TypeU;
	UNICODE_STRING NameU;
	ANSI_STRING Type;
	ANSI_STRING Name;
	HRSRC Res;

	RtlInitUnicodeString (&NameU,
	                      NULL);
	RtlInitUnicodeString (&TypeU,
	                      NULL);

	if (HIWORD(lpName) != 0)
	{
		RtlInitAnsiString (&Name,
		                   (LPSTR)lpName);
		RtlAnsiStringToUnicodeString (&NameU,
		                              &Name,
		                              TRUE);
	}
	else
		NameU.Buffer = (PWSTR)lpName;

	if (HIWORD(lpType) != 0)
	{
		RtlInitAnsiString (&Type,
		                   (LPSTR)lpType);
		RtlAnsiStringToUnicodeString (&TypeU,
		                              &Type,
		                              TRUE);
	}
	else
		TypeU.Buffer = (PWSTR)lpType;

	Res = FindResourceExW (hModule,
	                       TypeU.Buffer,
	                       NameU.Buffer,
	                       wLanguage);

	if (HIWORD(lpName) != 0)
		RtlFreeUnicodeString (&NameU);

	if (HIWORD(lpType) != 0)
		RtlFreeUnicodeString (&TypeU);

	return Res;
}

HRSRC
STDCALL
FindResourceW (
	HINSTANCE	hModule,
	LPCWSTR		lpName,
	LPCWSTR		lpType
	)
{
	return FindResourceExW (hModule, lpName, lpType, 0);
}

HRSRC
STDCALL
FindResourceExW (
	HINSTANCE	hModule,
	LPCWSTR		lpType,
	LPCWSTR		lpName,
	WORD		wLanguage
	)
{
	IMAGE_RESOURCE_DATA_ENTRY *ResourceDataEntry;
	NTSTATUS Status;
	int i,l;
	ULONG nType = 0, nName = 0;
	
	if ( hModule == NULL )
		hModule = GetModuleHandle(NULL);

	if ( HIWORD(lpName) != 0 )  {
		if ( lpName[0] == L'#' ) {
			l = lstrlenW(lpName) -1;
		
			for(i=0;i<l;i++) {
				nName = lpName[i+1] - L'0';
				if ( i < l - 1 )
					nName*= 10;
			}

		}

		lpName = (LPWSTR)nName;
	}

	if ( HIWORD(lpType) != 0 )  {
		if ( lpType[0] == L'#' ) {
			l = lstrlenW(lpType);

			for(i=0;i<l;i++) {
				nType = lpType[i] - L'0';
				if ( i < l - 1 )
					nType*= 10;
			}
		}
		else
			return NULL;
	}
	else
		nType = (ULONG)lpType;

	Status = LdrFindResource_U(hModule,&ResourceDataEntry,lpName, nType,wLanguage);
	if ( !NT_SUCCESS(Status ) ) {
		SetLastError(RtlNtStatusToDosError(Status));
		return NULL;
	}
	return ResourceDataEntry;
}

HGLOBAL
STDCALL
LoadResource (
	HINSTANCE	hModule,
	HRSRC		hResInfo
	)
{
	void **Data;

	Data = HeapAlloc (GetProcessHeap (), 0, sizeof(void *));
	LdrAccessResource (hModule, hResInfo, Data);

	return *Data;
}

DWORD
STDCALL
SizeofResource (
	HINSTANCE	hModule,
	HRSRC		hResInfo
	)
{
	return ((PIMAGE_RESOURCE_DATA_ENTRY)hResInfo)->Size;
}

WINBOOL
STDCALL
FreeResource (
	HGLOBAL	hResData
	)
{
	HeapFree (GetProcessHeap (), 0, &hResData);
	return TRUE;
}

LPVOID
STDCALL
LockResource (
	HGLOBAL	hResData
	)
{
	return hResData;
}

/* EOF */
