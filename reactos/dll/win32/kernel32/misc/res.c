/* $Id$
 *
 * COPYRIGHT: See COPYING in the top level directory
 * PROJECT  : ReactOS user mode libraries
 * MODULE   : kernel32.dll
 * FILE     : reactos/lib/kernel32/misc/res.c
 * AUTHOR   : Ariadne
 *            Eric Kohl
 *            Ge van Geldorp
 *            Gunnar Dalsnes
 *            David Welch
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>

#define STUB \
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); \
  DPRINT1("%s() is UNIMPLEMENTED!\n", __FUNCTION__)

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

	Status = LdrFindResource_U (hModule,
				    &ResourceInfo,
				    RESOURCE_DATA_LEVEL,
				    &ResourceDataEntry);
	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return NULL;
	}

	return (HRSRC)ResourceDataEntry;
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
	PIMAGE_RESOURCE_DATA_ENTRY ResInfo = (PIMAGE_RESOURCE_DATA_ENTRY)hResInfo;

	if (hModule == NULL)
	{
		hModule = (HINSTANCE)GetModuleHandleW(NULL);
	}

	Status = LdrAccessResource (hModule, ResInfo, &Data, NULL);
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
	STUB;
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
	STUB;
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
	STUB;
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
	STUB;
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
	STUB;
	return FALSE;
}



/* retrieve the resource name to pass to the ntdll functions */
static NTSTATUS get_res_nameA( LPCSTR name, UNICODE_STRING *str )
{
    if (!HIWORD(name))
    {
        str->Buffer = (LPWSTR)name;
        return STATUS_SUCCESS;
    }
    if (name[0] == '#')
    {
        ULONG value;
        if (RtlCharToInteger( name + 1, 10, &value ) != STATUS_SUCCESS || HIWORD(value))
            return STATUS_INVALID_PARAMETER;
        str->Buffer = (LPWSTR)value;
        return STATUS_SUCCESS;
    }
    RtlCreateUnicodeStringFromAsciiz( str, name );
    RtlUpcaseUnicodeString( str, str, FALSE );
    return STATUS_SUCCESS;
}

/* retrieve the resource name to pass to the ntdll functions */
static NTSTATUS get_res_nameW( LPCWSTR name, UNICODE_STRING *str )
{
    if (!HIWORD(name))
    {
        str->Buffer = (LPWSTR)name;
        return STATUS_SUCCESS;
    }
    if (name[0] == '#')
    {
        ULONG value;
        RtlInitUnicodeString( str, name + 1 );
        if (RtlUnicodeStringToInteger( str, 10, &value ) != STATUS_SUCCESS || HIWORD(value))
            return STATUS_INVALID_PARAMETER;
        str->Buffer = (LPWSTR)value;
        return STATUS_SUCCESS;
    }
    RtlCreateUnicodeString( str, name );
    RtlUpcaseUnicodeString( str, str, FALSE );
    return STATUS_SUCCESS;
}

/**********************************************************************
 * EnumResourceNamesA   (KERNEL32.@)
 */
BOOL STDCALL EnumResourceNamesA( HMODULE hmod, LPCSTR type, ENUMRESNAMEPROCA lpfun, LONG_PTR lparam )
{
    int i;
    BOOL ret = FALSE;
    DWORD len = 0, newlen;
    LPSTR name = NULL;
    NTSTATUS status;
    UNICODE_STRING typeW;
    LDR_RESOURCE_INFO info;
    PIMAGE_RESOURCE_DIRECTORY basedir, resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    const IMAGE_RESOURCE_DIR_STRING_U *str;

    DPRINT( "%p %s %p %lx\n", hmod, type, lpfun, lparam );

    if (!hmod) hmod = GetModuleHandleA( NULL );
    typeW.Buffer = NULL;
    if ((status = LdrFindResourceDirectory_U( hmod, NULL, 0, &basedir )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameA( type, &typeW )) != STATUS_SUCCESS)
        goto done;
    info.Type = (ULONG)typeW.Buffer;
    if ((status = LdrFindResourceDirectory_U( hmod, &info, 1, &resdir )) != STATUS_SUCCESS)
        goto done;

    et = (IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    for (i = 0; i < resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries; i++)
    {
        if (et[i].NameIsString)
        {
            str = (IMAGE_RESOURCE_DIR_STRING_U *) ((LPBYTE) basedir + et[i].NameOffset);
            newlen = WideCharToMultiByte(CP_ACP, 0, str->NameString, str->Length, NULL, 0, NULL, NULL);
            if (newlen + 1 > len)
            {
                len = newlen + 1;
                HeapFree( GetProcessHeap(), 0, name );
                if (!(name = HeapAlloc(GetProcessHeap(), 0, len + 1 )))
                {
                    ret = FALSE;
                    break;
                }
            }
            WideCharToMultiByte( CP_ACP, 0, str->NameString, str->Length, name, len, NULL, NULL );
            name[newlen] = 0;
            ret = lpfun(hmod,type,name,lparam);
        }
        else
        {
            ret = lpfun( hmod, type, (LPSTR)(int)et[i].Id, lparam );
        }
        if (!ret) break;
    }
done:
    HeapFree( GetProcessHeap(), 0, name );
    if (HIWORD(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/**********************************************************************
 * EnumResourceNamesW   (KERNEL32.@)
 */
BOOL STDCALL EnumResourceNamesW( HMODULE hmod, LPCWSTR type, ENUMRESNAMEPROCW lpfun, LONG_PTR lparam )
{
    int i, len = 0;
    BOOL ret = FALSE;
    LPWSTR name = NULL;
    NTSTATUS status;
    UNICODE_STRING typeW;
    LDR_RESOURCE_INFO info;
    PIMAGE_RESOURCE_DIRECTORY basedir, resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    const IMAGE_RESOURCE_DIR_STRING_U *str;

    DPRINT( "%p %s %p %lx\n", hmod, type, lpfun, lparam );

    if (!hmod) hmod = GetModuleHandleW( NULL );
    typeW.Buffer = NULL;
    if ((status = LdrFindResourceDirectory_U( hmod, NULL, 0, &basedir )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameW( type, &typeW )) != STATUS_SUCCESS)
        goto done;
    info.Type = (ULONG)typeW.Buffer;
    if ((status = LdrFindResourceDirectory_U( hmod, &info, 1, &resdir )) != STATUS_SUCCESS)
        goto done;

    et = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdir + 1);
    for (i = 0; i < resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries; i++)
    {
        if (et[i].NameIsString)
        {
            str = (IMAGE_RESOURCE_DIR_STRING_U *) ((LPBYTE) basedir + et[i].NameOffset);
            if (str->Length + 1 > len)
            {
                len = str->Length + 1;
                HeapFree( GetProcessHeap(), 0, name );
                if (!(name = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
                {
                    ret = FALSE;
                    break;
                }
            }
            memcpy(name, str->NameString, str->Length * sizeof (WCHAR));
            name[str->Length] = 0;
            ret = lpfun(hmod,type,name,lparam);
        }
        else
        {
            ret = lpfun( hmod, type, (LPWSTR)(int)et[i].Id, lparam );
        }
        if (!ret) break;
    }
done:
    HeapFree( GetProcessHeap(), 0, name );
    if (HIWORD(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
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
	STUB;
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
	STUB;
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
	STUB;
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
	STUB;
	return FALSE;
}

/* EOF */
