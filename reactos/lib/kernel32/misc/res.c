/* $Id: res.c,v 1.19 2004/01/23 17:15:23 ekohl Exp $
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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

	if ( hModule == NULL )
		hModule = (HINSTANCE)GetModuleHandleW(NULL);

	if ( !IS_INTRESOURCE(lpName) && lpName[0] == L'#' ) {
		lpName = MAKEINTRESOURCEW(wcstoul(lpName + 1, NULL, 10));
	}
	if ( !IS_INTRESOURCE(lpType) && lpType[0] == L'#' ) {
		lpType = MAKEINTRESOURCEW(wcstoul(lpType + 1, NULL, 10));
	}

	ResourceInfo.Type = (ULONG)lpType;
	ResourceInfo.Name = (ULONG)lpName;
	ResourceInfo.Language = (ULONG)wLanguage;
	if (ResourceInfo.Language == (ULONG) MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)) {
		ResourceInfo.Language = (ULONG) GetUserDefaultLangID();
		if (ResourceInfo.Language == (ULONG) MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)) {
			ResourceInfo.Language = (ULONG) MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
		}
	}

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


/*
 * @implemented
 */
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
     hModule = (HINSTANCE)GetModuleHandleW(NULL);
   }

	Status = LdrAccessResource (hModule, hResInfo, &Data, NULL);
	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return NULL;
	}

	return Data;
}


/*
 * @implemented
 */
DWORD
STDCALL
SizeofResource (
	HINSTANCE	hModule,
	HRSRC		hResInfo
	)
{
	return ((PIMAGE_RESOURCE_DATA_ENTRY)hResInfo)->Size;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
FreeResource (
	HGLOBAL	hResData
	)
{
	return TRUE;
}


/*
 * @unimplemented
 */
LPVOID
STDCALL
LockResource (
	HGLOBAL	hResData
	)
{
	return hResData;
}


/*
 * @unimplemented
 */
HANDLE
STDCALL
BeginUpdateResourceW (
	LPCWSTR	pFileName,
	BOOL	bDeleteExistingResources
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
HANDLE
STDCALL
BeginUpdateResourceA (
	LPCSTR	pFileName,
	BOOL	bDeleteExistingResources
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EndUpdateResourceW (
	HANDLE	hUpdate,
	BOOL	fDiscard
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EndUpdateResourceA (
	HANDLE	hUpdate,
	BOOL	fDiscard
	)
{
	return EndUpdateResourceW(
			hUpdate,
			fDiscard
			);
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumResourceLanguagesW (
	HINSTANCE		hModule,
	LPCWSTR			lpType,
	LPCWSTR			lpName,
	ENUMRESLANGPROCW	lpEnumFunc,
	LONG			lParam
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
EnumResourceLanguagesA (
	HINSTANCE		hModule,
	LPCSTR			lpType,
	LPCSTR			lpName,
	ENUMRESLANGPROCA	lpEnumFunc,
	LONG			lParam
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumResourceNamesW (
	HINSTANCE		hModule,
	LPCWSTR			lpType,
	ENUMRESNAMEPROCW	lpEnumFunc,
	LONG			lParam
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumResourceNamesA (
	HINSTANCE		hModule,
	LPCSTR			lpType,
	ENUMRESNAMEPROCA	lpEnumFunc,
	LONG			lParam
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumResourceTypesW (
	HINSTANCE		hModule,
	ENUMRESTYPEPROCW	lpEnumFunc,
	LONG			lParam
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumResourceTypesA (
	HINSTANCE		hModule,
	ENUMRESTYPEPROCA	lpEnumFunc,
	LONG			lParam
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
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


/*
 * @unimplemented
 */
BOOL
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
