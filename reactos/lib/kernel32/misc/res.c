/* $Id: res.c,v 1.13 2003/03/21 00:20:41 gdalsnes Exp $
 *
 * COPYRIGHT: See COPYING in the top level directory
 * PROJECT  : ReactOS user mode libraries
 * MODULE   : kernel32.dll
 * FILE     : reactos/lib/kernel32/misc/res.c
 * AUTHOR   : ???
 */

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>


HRSRC
STDCALL
FindResourceA (
	HINSTANCE	hModule,
	LPCSTR		lpName,
	LPCSTR		lpType
	)
{
	return FindResourceExA (hModule, lpType, lpName, 0);
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
	return FindResourceExW (hModule, lpType, lpName, 0);
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
	PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry = NULL;
	LDR_RESOURCE_INFO ResourceInfo;
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
		else
		{
			SetLastErrorByStatus (STATUS_INVALID_PARAMETER);
			return NULL;
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
		{
			SetLastErrorByStatus (STATUS_INVALID_PARAMETER);
			return NULL;
		}

		lpType = (LPWSTR)nType;
	}

	ResourceInfo.Type = (ULONG)lpType;
	ResourceInfo.Name = (ULONG)lpName;
	ResourceInfo.Language = (ULONG)wLanguage;

	Status = LdrFindResource_U (hModule,
				    &ResourceInfo,
				    RESOURCE_DATA_LEVEL,
				    &ResourceDataEntry);
	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
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
	NTSTATUS Status;
	PVOID Data;

   if (hModule == NULL)
   {
     hModule = GetModuleHandle(NULL);
   }

	Status = LdrAccessResource (hModule, hResInfo, &Data, NULL);
	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return NULL;
	}

	return Data;
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

HANDLE
STDCALL
BeginUpdateResourceW (
	LPCWSTR	pFileName,
	WINBOOL	bDeleteExistingResources
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


HANDLE
STDCALL
BeginUpdateResourceA (
	LPCSTR	pFileName,
	WINBOOL	bDeleteExistingResources
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
EndUpdateResourceW (
	HANDLE	hUpdate,
	WINBOOL	fDiscard
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EndUpdateResourceA (
	HANDLE	hUpdate,
	WINBOOL	fDiscard
	)
{
	return EndUpdateResourceW(
			hUpdate,
			fDiscard
			);
}

WINBOOL
STDCALL
EnumResourceLanguagesW (
	HINSTANCE	hModule,
	LPCWSTR		lpType,
	LPCWSTR		lpName,
	ENUMRESLANGPROC	lpEnumFunc,
	LONG		lParam
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumResourceLanguagesA (
	HINSTANCE	hModule,
	LPCSTR		lpType,
	LPCSTR		lpName,
	ENUMRESLANGPROC	lpEnumFunc,
	LONG		lParam
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumResourceNamesW (
	HINSTANCE	hModule,
	LPCWSTR		lpType,
	ENUMRESNAMEPROC	lpEnumFunc,
	LONG		lParam
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumResourceNamesA (
	HINSTANCE	hModule,
	LPCSTR		lpType,
	ENUMRESNAMEPROC	lpEnumFunc,
	LONG		lParam
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumResourceTypesW (
	HINSTANCE	hModule,
	ENUMRESTYPEPROC	lpEnumFunc,
	LONG		lParam
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



WINBOOL
STDCALL
EnumResourceTypesA (
	HINSTANCE	hModule,
	ENUMRESTYPEPROC	lpEnumFunc,
	LONG		lParam
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
UpdateResourceA (
	HANDLE	hUpdate,
	LPCSTR	lpType,
	LPCSTR	lpName,
	WORD	wLanguage,
	LPVOID	lpData,
	DWORD	cbData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
UpdateResourceW (
	HANDLE	hUpdate,
	LPCWSTR	lpType,
	LPCWSTR	lpName,
	WORD	wLanguage,
	LPVOID	lpData,
	DWORD	cbData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/* EOF */
