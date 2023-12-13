/*
 * Setupapi miscellaneous functions
 *
 * Copyright 2005 Eric Kohl
 * Copyright 2007 Hans Leidekker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "setupapi_private.h"

#include <winver.h>
#include <lzexpand.h>

/* Unicode constants */
static const WCHAR BackSlash[] = {'\\',0};
static const WCHAR TranslationRegKey[] = {'\\','V','e','r','F','i','l','e','I','n','f','o','\\','T','r','a','n','s','l','a','t','i','o','n',0};

/* Handles and critical sections for the SetupLog API */
static HANDLE setupact = INVALID_HANDLE_VALUE;
static HANDLE setuperr = INVALID_HANDLE_VALUE;
static CRITICAL_SECTION setupapi_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &setupapi_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": setupapi_cs") }
};
static CRITICAL_SECTION setupapi_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

DWORD
GetFunctionPointer(
    IN PWSTR InstallerName,
    OUT HMODULE* ModulePointer,
    OUT PVOID* FunctionPointer)
{
    HMODULE hModule = NULL;
    LPSTR FunctionNameA = NULL;
    PWCHAR Comma;
    DWORD rc;

    *ModulePointer = NULL;
    *FunctionPointer = NULL;

    Comma = wcschr(InstallerName, ',');
    if (!Comma)
    {
        rc = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    /* Load library */
    *Comma = '\0';
    hModule = LoadLibraryW(InstallerName);
    *Comma = ',';
    if (!hModule)
    {
        rc = GetLastError();
        goto cleanup;
    }

    /* Skip comma spaces */
    while (*Comma == ',' || iswspace(*Comma))
        Comma++;

    /* W->A conversion for function name */
    FunctionNameA = pSetupUnicodeToMultiByte(Comma, CP_ACP);
    if (!FunctionNameA)
    {
        rc = GetLastError();
        goto cleanup;
    }

    /* Search function */
    *FunctionPointer = GetProcAddress(hModule, FunctionNameA);
    if (!*FunctionPointer)
    {
        rc = GetLastError();
        goto cleanup;
    }

    *ModulePointer = hModule;
    rc = ERROR_SUCCESS;

cleanup:
    if (rc != ERROR_SUCCESS && hModule)
        FreeLibrary(hModule);
    MyFree(FunctionNameA);
    return rc;
}

DWORD
FreeFunctionPointer(
    IN HMODULE ModulePointer,
    IN PVOID FunctionPointer)
{
    if (ModulePointer == NULL)
        return ERROR_SUCCESS;
    if (FreeLibrary(ModulePointer))
       return ERROR_SUCCESS;
    else
       return GetLastError();
}

/**************************************************************************
 * MyFree [SETUPAPI.@]
 *
 * Frees an allocated memory block from the process heap.
 *
 * PARAMS
 *     lpMem [I] pointer to memory block which will be freed
 *
 * RETURNS
 *     None
 */
VOID WINAPI MyFree(LPVOID lpMem)
{
    TRACE("%p\n", lpMem);
    HeapFree(GetProcessHeap(), 0, lpMem);
}


/**************************************************************************
 * MyMalloc [SETUPAPI.@]
 *
 * Allocates memory block from the process heap.
 *
 * PARAMS
 *     dwSize [I] size of the allocated memory block
 *
 * RETURNS
 *     Success: pointer to allocated memory block
 *     Failure: NULL
 */
LPVOID WINAPI MyMalloc(DWORD dwSize)
{
    TRACE("%lu\n", dwSize);
    return HeapAlloc(GetProcessHeap(), 0, dwSize);
}


/**************************************************************************
 * MyRealloc [SETUPAPI.@]
 *
 * Changes the size of an allocated memory block or allocates a memory
 * block from the process heap.
 *
 * PARAMS
 *     lpSrc  [I] pointer to memory block which will be resized
 *     dwSize [I] new size of the memory block
 *
 * RETURNS
 *     Success: pointer to the resized memory block
 *     Failure: NULL
 *
 * NOTES
 *     If lpSrc is a NULL-pointer, then MyRealloc allocates a memory
 *     block like MyMalloc.
 */
LPVOID WINAPI MyRealloc(LPVOID lpSrc, DWORD dwSize)
{
    TRACE("%p %lu\n", lpSrc, dwSize);

    if (lpSrc == NULL)
        return HeapAlloc(GetProcessHeap(), 0, dwSize);

    return HeapReAlloc(GetProcessHeap(), 0, lpSrc, dwSize);
}


/**************************************************************************
 * pSetupDuplicateString [SETUPAPI.@]
 *
 * Duplicates a unicode string.
 *
 * PARAMS
 *     lpSrc  [I] pointer to the unicode string that will be duplicated
 *
 * RETURNS
 *     Success: pointer to the duplicated unicode string
 *     Failure: NULL
 *
 * NOTES
 *     Call MyFree() to release the duplicated string.
 */
LPWSTR WINAPI pSetupDuplicateString(LPCWSTR lpSrc)
{
    LPWSTR lpDst;

    TRACE("%s\n", debugstr_w(lpSrc));

    lpDst = MyMalloc((lstrlenW(lpSrc) + 1) * sizeof(WCHAR));
    if (lpDst == NULL)
        return NULL;

    strcpyW(lpDst, lpSrc);

    return lpDst;
}


/**************************************************************************
 * QueryRegistryValue [SETUPAPI.@]
 *
 * Retrieves value data from the registry and allocates memory for the
 * value data.
 *
 * PARAMS
 *     hKey        [I] Handle of the key to query
 *     lpValueName [I] Name of value under hkey to query
 *     lpData      [O] Destination for the values contents,
 *     lpType      [O] Destination for the value type
 *     lpcbData    [O] Destination for the size of data
 *
 * RETURNS
 *     Success: ERROR_SUCCESS
 *     Failure: Otherwise
 *
 * NOTES
 *     Use MyFree to release the lpData buffer.
 */
LONG WINAPI QueryRegistryValue(HKEY hKey,
                               LPCWSTR lpValueName,
                               LPBYTE  *lpData,
                               LPDWORD lpType,
                               LPDWORD lpcbData)
{
    LONG lError;

    TRACE("%p %s %p %p %p\n",
          hKey, debugstr_w(lpValueName), lpData, lpType, lpcbData);

    /* Get required buffer size */
    *lpcbData = 0;
    lError = RegQueryValueExW(hKey, lpValueName, 0, lpType, NULL, lpcbData);
    if (lError != ERROR_SUCCESS)
        return lError;

    /* Allocate buffer */
    *lpData = MyMalloc(*lpcbData);
    if (*lpData == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    /* Query registry value */
    lError = RegQueryValueExW(hKey, lpValueName, 0, lpType, *lpData, lpcbData);
    if (lError != ERROR_SUCCESS)
        MyFree(*lpData);

    return lError;
}


/**************************************************************************
 * pSetupMultiByteToUnicode [SETUPAPI.@]
 *
 * Converts a multi-byte string to a Unicode string.
 *
 * PARAMS
 *     lpMultiByteStr  [I] Multi-byte string to be converted
 *     uCodePage       [I] Code page
 *
 * RETURNS
 *     Success: pointer to the converted Unicode string
 *     Failure: NULL
 *
 * NOTE
 *     Use MyFree to release the returned Unicode string.
 */
LPWSTR WINAPI pSetupMultiByteToUnicode(LPCSTR lpMultiByteStr, UINT uCodePage)
{
    LPWSTR lpUnicodeStr;
    int nLength;

    TRACE("%s %d\n", debugstr_a(lpMultiByteStr), uCodePage);

    nLength = MultiByteToWideChar(uCodePage, 0, lpMultiByteStr,
                                  -1, NULL, 0);
    if (nLength == 0)
        return NULL;

    lpUnicodeStr = MyMalloc(nLength * sizeof(WCHAR));
    if (lpUnicodeStr == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    if (!MultiByteToWideChar(uCodePage, 0, lpMultiByteStr,
                             nLength, lpUnicodeStr, nLength))
    {
        MyFree(lpUnicodeStr);
        return NULL;
    }

    return lpUnicodeStr;
}


/**************************************************************************
 * pSetupUnicodeToMultiByte [SETUPAPI.@]
 *
 * Converts a Unicode string to a multi-byte string.
 *
 * PARAMS
 *     lpUnicodeStr  [I] Unicode string to be converted
 *     uCodePage     [I] Code page
 *
 * RETURNS
 *     Success: pointer to the converted multi-byte string
 *     Failure: NULL
 *
 * NOTE
 *     Use MyFree to release the returned multi-byte string.
 */
LPSTR WINAPI pSetupUnicodeToMultiByte(LPCWSTR lpUnicodeStr, UINT uCodePage)
{
    LPSTR lpMultiByteStr;
    int nLength;

    TRACE("%s %d\n", debugstr_w(lpUnicodeStr), uCodePage);

    nLength = WideCharToMultiByte(uCodePage, 0, lpUnicodeStr, -1,
                                  NULL, 0, NULL, NULL);
    if (nLength == 0)
        return NULL;

    lpMultiByteStr = MyMalloc(nLength);
    if (lpMultiByteStr == NULL)
        return NULL;

    if (!WideCharToMultiByte(uCodePage, 0, lpUnicodeStr, -1,
                             lpMultiByteStr, nLength, NULL, NULL))
    {
        MyFree(lpMultiByteStr);
        return NULL;
    }

    return lpMultiByteStr;
}


/**************************************************************************
 * DoesUserHavePrivilege [SETUPAPI.@]
 *
 * Check whether the current user has got a given privilege.
 *
 * PARAMS
 *     lpPrivilegeName  [I] Name of the privilege to be checked
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI DoesUserHavePrivilege(LPCWSTR lpPrivilegeName)
{
    HANDLE hToken;
    DWORD dwSize;
    PTOKEN_PRIVILEGES lpPrivileges;
    LUID PrivilegeLuid;
    DWORD i;
    BOOL bResult = FALSE;

    TRACE("%s\n", debugstr_w(lpPrivilegeName));

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return FALSE;

    if (!GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &dwSize))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            CloseHandle(hToken);
            return FALSE;
        }
    }

    lpPrivileges = MyMalloc(dwSize);
    if (lpPrivileges == NULL)
    {
        CloseHandle(hToken);
        return FALSE;
    }

    if (!GetTokenInformation(hToken, TokenPrivileges, lpPrivileges, dwSize, &dwSize))
    {
        MyFree(lpPrivileges);
        CloseHandle(hToken);
        return FALSE;
    }

    CloseHandle(hToken);

    if (!LookupPrivilegeValueW(NULL, lpPrivilegeName, &PrivilegeLuid))
    {
        MyFree(lpPrivileges);
        return FALSE;
    }

    for (i = 0; i < lpPrivileges->PrivilegeCount; i++)
    {
        if (lpPrivileges->Privileges[i].Luid.HighPart == PrivilegeLuid.HighPart &&
            lpPrivileges->Privileges[i].Luid.LowPart == PrivilegeLuid.LowPart)
        {
            bResult = TRUE;
        }
    }

    MyFree(lpPrivileges);

    return bResult;
}


/**************************************************************************
 * pSetupEnablePrivilege [SETUPAPI.@]
 *
 * Enables or disables one of the current users privileges.
 *
 * PARAMS
 *     lpPrivilegeName  [I] Name of the privilege to be changed
 *     bEnable          [I] TRUE: Enables the privilege
 *                          FALSE: Disables the privilege
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI pSetupEnablePrivilege(LPCWSTR lpPrivilegeName, BOOL bEnable)
{
    TOKEN_PRIVILEGES Privileges;
    HANDLE hToken;
    BOOL bResult;

    TRACE("%s %s\n", debugstr_w(lpPrivilegeName), bEnable ? "TRUE" : "FALSE");

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return FALSE;

    Privileges.PrivilegeCount = 1;
    Privileges.Privileges[0].Attributes = (bEnable) ? SE_PRIVILEGE_ENABLED : 0;

    if (!LookupPrivilegeValueW(NULL, lpPrivilegeName,
                               &Privileges.Privileges[0].Luid))
    {
        CloseHandle(hToken);
        return FALSE;
    }

    bResult = AdjustTokenPrivileges(hToken, FALSE, &Privileges, 0, NULL, NULL);

    CloseHandle(hToken);

    return bResult;
}


/**************************************************************************
 * DelayedMove [SETUPAPI.@]
 *
 * Moves a file upon the next reboot.
 *
 * PARAMS
 *     lpExistingFileName  [I] Current file name
 *     lpNewFileName       [I] New file name
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI DelayedMove(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName)
{
    return MoveFileExW(lpExistingFileName, lpNewFileName,
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT);
}


/**************************************************************************
 * FileExists [SETUPAPI.@]
 *
 * Checks whether a file exists.
 *
 * PARAMS
 *     lpFileName     [I] Name of the file to check
 *     lpNewFileName  [O] Optional information about the existing file
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI FileExists(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFileFindData)
{
    WIN32_FIND_DATAW FindData;
    HANDLE hFind;
    UINT uErrorMode;
    DWORD dwError;

    uErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    hFind = FindFirstFileW(lpFileName, &FindData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        dwError = GetLastError();
        SetErrorMode(uErrorMode);
        SetLastError(dwError);
        return FALSE;
    }

    FindClose(hFind);

    if (lpFileFindData)
        memcpy(lpFileFindData, &FindData, sizeof(WIN32_FIND_DATAW));

    SetErrorMode(uErrorMode);

    return TRUE;
}


/**************************************************************************
 * CaptureStringArg [SETUPAPI.@]
 *
 * Captures a UNICODE string.
 *
 * PARAMS
 *     lpSrc  [I] UNICODE string to be captured
 *     lpDst  [O] Pointer to the captured UNICODE string
 *
 * RETURNS
 *     Success: ERROR_SUCCESS
 *     Failure: ERROR_INVALID_PARAMETER
 *
 * NOTE
 *     Call MyFree to release the captured UNICODE string.
 */
DWORD WINAPI CaptureStringArg(LPCWSTR pSrc, LPWSTR *pDst)
{
    if (pDst == NULL)
        return ERROR_INVALID_PARAMETER;

    *pDst = pSetupDuplicateString(pSrc);

    return ERROR_SUCCESS;
}


/**************************************************************************
 * pSetupCaptureAndConvertAnsiArg [SETUPAPI.@]
 *
 * Captures an ANSI string and converts it to a UNICODE string.
 *
 * PARAMS
 *     lpSrc  [I] ANSI string to be captured
 *     lpDst  [O] Pointer to the captured UNICODE string
 *
 * RETURNS
 *     Success: ERROR_SUCCESS
 *     Failure: ERROR_INVALID_PARAMETER
 *
 * NOTE
 *     Call MyFree to release the captured UNICODE string.
 */
DWORD WINAPI pSetupCaptureAndConvertAnsiArg(LPCSTR pSrc, LPWSTR *pDst)
{
    if (pDst == NULL)
        return ERROR_INVALID_PARAMETER;

    *pDst = pSetupMultiByteToUnicode(pSrc, CP_ACP);

    return ERROR_SUCCESS;
}


/**************************************************************************
 * pSetupOpenAndMapFileForRead [SETUPAPI.@]
 *
 * Open and map a file to a buffer.
 *
 * PARAMS
 *     lpFileName [I] Name of the file to be opened
 *     lpSize     [O] Pointer to the file size
 *     lpFile     [0] Pointer to the file handle
 *     lpMapping  [0] Pointer to the mapping handle
 *     lpBuffer   [0] Pointer to the file buffer
 *
 * RETURNS
 *     Success: ERROR_SUCCESS
 *     Failure: Other
 *
 * NOTE
 *     Call UnmapAndCloseFile to release the file.
 */
DWORD WINAPI pSetupOpenAndMapFileForRead(LPCWSTR lpFileName,
                                         LPDWORD lpSize,
                                         LPHANDLE lpFile,
                                         LPHANDLE lpMapping,
                                         LPVOID *lpBuffer)
{
    DWORD dwError;

    TRACE("%s %p %p %p %p\n",
          debugstr_w(lpFileName), lpSize, lpFile, lpMapping, lpBuffer);

    *lpFile = CreateFileW(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                          OPEN_EXISTING, 0, NULL);
    if (*lpFile == INVALID_HANDLE_VALUE)
        return GetLastError();

    *lpSize = GetFileSize(*lpFile, NULL);
    if (*lpSize == INVALID_FILE_SIZE)
    {
        dwError = GetLastError();
        CloseHandle(*lpFile);
        return dwError;
    }

    *lpMapping = CreateFileMappingW(*lpFile, NULL, PAGE_READONLY, 0,
                                    *lpSize, NULL);
    if (*lpMapping == NULL)
    {
        dwError = GetLastError();
        CloseHandle(*lpFile);
        return dwError;
    }

    *lpBuffer = MapViewOfFile(*lpMapping, FILE_MAP_READ, 0, 0, *lpSize);
    if (*lpBuffer == NULL)
    {
        dwError = GetLastError();
        CloseHandle(*lpMapping);
        CloseHandle(*lpFile);
        return dwError;
    }

    return ERROR_SUCCESS;
}


/**************************************************************************
 * pSetupUnmapAndCloseFile [SETUPAPI.@]
 *
 * Unmap and close a mapped file.
 *
 * PARAMS
 *     hFile    [I] Handle to the file
 *     hMapping [I] Handle to the file mapping
 *     lpBuffer [I] Pointer to the file buffer
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI pSetupUnmapAndCloseFile(HANDLE hFile, HANDLE hMapping, LPVOID lpBuffer)
{
    TRACE("%p %p %p\n",
          hFile, hMapping, lpBuffer);

    if (!UnmapViewOfFile(lpBuffer))
        return FALSE;

    if (!CloseHandle(hMapping))
        return FALSE;

    if (!CloseHandle(hFile))
        return FALSE;

    return TRUE;
}


/**************************************************************************
 * StampFileSecurity [SETUPAPI.@]
 *
 * Assign a new security descriptor to the given file.
 *
 * PARAMS
 *     lpFileName          [I] Name of the file
 *     pSecurityDescriptor [I] New security descriptor
 *
 * RETURNS
 *     Success: ERROR_SUCCESS
 *     Failure: other
 */
DWORD WINAPI StampFileSecurity(LPCWSTR lpFileName, PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    TRACE("%s %p\n", debugstr_w(lpFileName), pSecurityDescriptor);

    if (!SetFileSecurityW(lpFileName, OWNER_SECURITY_INFORMATION |
                          GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                          pSecurityDescriptor))
        return GetLastError();

    return ERROR_SUCCESS;
}


/**************************************************************************
 * TakeOwnershipOfFile [SETUPAPI.@]
 *
 * Takes the ownership of the given file.
 *
 * PARAMS
 *     lpFileName [I] Name of the file
 *
 * RETURNS
 *     Success: ERROR_SUCCESS
 *     Failure: other
 */
DWORD WINAPI TakeOwnershipOfFile(LPCWSTR lpFileName)
{
    SECURITY_DESCRIPTOR SecDesc;
    HANDLE hToken = NULL;
    PTOKEN_OWNER pOwner = NULL;
    DWORD dwError;
    DWORD dwSize;

    TRACE("%s\n", debugstr_w(lpFileName));

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return GetLastError();

    if (!GetTokenInformation(hToken, TokenOwner, NULL, 0, &dwSize))
    {
        goto fail;
    }

    pOwner = (PTOKEN_OWNER)MyMalloc(dwSize);
    if (pOwner == NULL)
    {
        CloseHandle(hToken);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (!GetTokenInformation(hToken, TokenOwner, pOwner, dwSize, &dwSize))
    {
        goto fail;
    }

    if (!InitializeSecurityDescriptor(&SecDesc, SECURITY_DESCRIPTOR_REVISION))
    {
        goto fail;
    }

    if (!SetSecurityDescriptorOwner(&SecDesc, pOwner->Owner, FALSE))
    {
        goto fail;
    }

    if (!SetFileSecurityW(lpFileName, OWNER_SECURITY_INFORMATION, &SecDesc))
    {
        goto fail;
    }

    MyFree(pOwner);
    CloseHandle(hToken);

    return ERROR_SUCCESS;

fail:;
    dwError = GetLastError();

    MyFree(pOwner);

    if (hToken != NULL)
        CloseHandle(hToken);

    return dwError;
}


/**************************************************************************
 * RetreiveFileSecurity [SETUPAPI.@]
 *
 * Retrieve the security descriptor that is associated with the given file.
 *
 * PARAMS
 *     lpFileName [I] Name of the file
 *
 * RETURNS
 *     Success: ERROR_SUCCESS
 *     Failure: other
 */
DWORD WINAPI RetreiveFileSecurity(LPCWSTR lpFileName,
                                  PSECURITY_DESCRIPTOR *pSecurityDescriptor)
{
    PSECURITY_DESCRIPTOR SecDesc;
    DWORD dwSize = 0x100;
    DWORD dwError;

    TRACE("%s %p\n", debugstr_w(lpFileName), pSecurityDescriptor);

    SecDesc = MyMalloc(dwSize);
    if (SecDesc == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    if (GetFileSecurityW(lpFileName, OWNER_SECURITY_INFORMATION |
                         GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                         SecDesc, dwSize, &dwSize))
    {
      *pSecurityDescriptor = SecDesc;
      return ERROR_SUCCESS;
    }

    dwError = GetLastError();
    if (dwError != ERROR_INSUFFICIENT_BUFFER)
    {
        MyFree(SecDesc);
        return dwError;
    }

    SecDesc = MyRealloc(SecDesc, dwSize);
    if (SecDesc == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    if (GetFileSecurityW(lpFileName, OWNER_SECURITY_INFORMATION |
                         GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                         SecDesc, dwSize, &dwSize))
    {
      *pSecurityDescriptor = SecDesc;
      return ERROR_SUCCESS;
    }

    dwError = GetLastError();
    MyFree(SecDesc);

    return dwError;
}


/*
 * See: https://msdn.microsoft.com/en-us/library/bb432397(v=vs.85).aspx
 * for more information.
 */
DWORD GlobalSetupFlags = 0;

/***********************************************************************
 *		pSetupGetGlobalFlags  (SETUPAPI.@)
 */
DWORD WINAPI pSetupGetGlobalFlags(void)
{
    return GlobalSetupFlags;
}

/***********************************************************************
 *		pSetupModifyGlobalFlags  (SETUPAPI.@)
 */
void WINAPI pSetupModifyGlobalFlags( DWORD mask, DWORD flags )
{
    FIXME( "stub\n" );
    GlobalSetupFlags = (GlobalSetupFlags & ~mask) | (flags & mask);
}

/***********************************************************************
 *		pSetupSetGlobalFlags  (SETUPAPI.@)
 */
void WINAPI pSetupSetGlobalFlags( DWORD flags )
{
    pSetupModifyGlobalFlags(0xFFFFFFFF, flags);
}

/***********************************************************************
 *		SetupGetNonInteractiveMode  (SETUPAPI.@)
 */
BOOL WINAPI SetupGetNonInteractiveMode(VOID)
{
    return (GlobalSetupFlags & PSPGF_NONINTERACTIVE);
}

/***********************************************************************
 *		SetupSetNonInteractiveMode  (SETUPAPI.@)
 */
BOOL WINAPI SetupSetNonInteractiveMode(BOOL NonInteractiveFlag)
{
    BOOL OldValue;

    OldValue = (GlobalSetupFlags & PSPGF_NONINTERACTIVE);
    pSetupModifyGlobalFlags(PSPGF_NONINTERACTIVE,
                            NonInteractiveFlag ? PSPGF_NONINTERACTIVE : 0);

    return OldValue;
}

/***********************************************************************
 *              AssertFail  (SETUPAPI.@)
 *
 * Shows an assert fail error messagebox
 *
 * PARAMS
 *   lpFile [I]         file where assert failed
 *   uLine [I]          line number in file
 *   lpMessage [I]      assert message
 *
 */
VOID WINAPI AssertFail(LPSTR lpFile, UINT uLine, LPSTR lpMessage)
{
    CHAR szModule[MAX_PATH];
    CHAR szBuffer[2048];
    LPSTR lpName;
//    LPSTR lpBuffer;

    TRACE("%s %u %s\n", lpFile, uLine, lpMessage);

    GetModuleFileNameA(SETUPAPI_hInstance, szModule, MAX_PATH);
    lpName = strrchr(szModule, '\\');
    if (lpName != NULL)
        lpName++;
    else
        lpName = szModule;

    wsprintfA(szBuffer,
              "Assertion failure at line %u in file %s: %s\n\nCall DebugBreak()?",
              uLine, lpFile, lpMessage);

    if (MessageBoxA(NULL, szBuffer, lpName, MB_SETFOREGROUND |
                    MB_TASKMODAL | MB_ICONERROR | MB_YESNO) == IDYES)
        DebugBreak();
}


/**************************************************************************
 * GetSetFileTimestamp [SETUPAPI.@]
 *
 * Gets or sets a files timestamp.
 *
 * PARAMS
 *     lpFileName       [I]   File name
 *     lpCreationTime   [I/O] Creation time
 *     lpLastAccessTime [I/O] Last access time
 *     lpLastWriteTime  [I/O] Last write time
 *     bSetFileTime     [I]   TRUE: Set file times
 *                            FALSE: Get file times
 *
 * RETURNS
 *     Success: ERROR_SUCCESS
 *     Failure: other
 */
DWORD WINAPI GetSetFileTimestamp(LPCWSTR lpFileName,
                                 LPFILETIME lpCreationTime,
                                 LPFILETIME lpLastAccessTime,
                                 LPFILETIME lpLastWriteTime,
                                 BOOLEAN bSetFileTime)
{
    HANDLE hFile;
    BOOLEAN bRet;
    DWORD dwError = ERROR_SUCCESS;

    TRACE("%s %p %p %p %x\n", debugstr_w(lpFileName), lpCreationTime,
          lpLastAccessTime, lpLastWriteTime, bSetFileTime);

    hFile = CreateFileW(lpFileName,
                        bSetFileTime ? GENERIC_WRITE : GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return GetLastError();

    if (bSetFileTime)
        bRet = SetFileTime(hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime);
    else
        bRet = GetFileTime(hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime);

    if (bRet == FALSE)
        dwError = GetLastError();

     CloseHandle(hFile);

     return dwError;
}


/**************************************************************************
 * pSetupGetFileTitle [SETUPAPI.@]
 *
 * Returns a pointer to the last part (file name) of a full file path.
 *
 * PARAMS
 *     pFilePath [I] The fully qualified file path.
 *
 * RETURNS
 *     Pointer to the file name, if any, or the terminating NULL.
 */
PCWSTR WINAPI
pSetupGetFileTitle(PCWSTR pFilePath)
{
    PCWSTR ptr, ret;
    WCHAR c;

    TRACE("%s\n", debugstr_w(pFilePath));

    /* Skip the drive letter if any */
    ptr = pFilePath;
    if (*ptr && ptr[1] == L':')
        ptr += 2;

    /* Find the last path separator preceding the file name */
    for (ret = ptr; (c = *ptr);)
    {
        ++ptr;
        if (c == L'\\' || c == L'/')
            ret = ptr;
    }

    return ret;
}


/**************************************************************************
 * pSetupConcatenatePaths [SETUPAPI.@]
 *
 * Concatenates two paths.
 *
 * PARAMS
 *     lpPath         [I/O] Path to append path to
 *     lpAppend       [I]   Path to append
 *     dwBufferSize   [I]   Size of the path buffer
 *     lpRequiredSize [O]   Required size for the concatenated path. Optional
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI
pSetupConcatenatePaths(LPWSTR lpPath,
                       LPCWSTR lpAppend,
                       DWORD dwBufferSize,
                       LPDWORD lpRequiredSize)
{
    DWORD dwPathSize;
    DWORD dwAppendSize;
    DWORD dwTotalSize;
    BOOL bBackslash = FALSE;

    TRACE("%s %s %lu %p\n", debugstr_w(lpPath), debugstr_w(lpAppend),
          dwBufferSize, lpRequiredSize);

    dwPathSize = lstrlenW(lpPath);

    /* Ignore trailing backslash */
    if (lpPath[dwPathSize - 1] == (WCHAR)'\\')
        dwPathSize--;

    dwAppendSize = lstrlenW(lpAppend);

    /* Does the source string have a leading backslash? */
    if (lpAppend[0] == (WCHAR)'\\')
    {
        bBackslash = TRUE;
        dwAppendSize--;
    }

    dwTotalSize = dwPathSize + dwAppendSize + 2;
    if (lpRequiredSize != NULL)
        *lpRequiredSize = dwTotalSize;

    /* Append a backslash to the destination string */
    if (bBackslash == FALSE)
    {
        if (dwPathSize < dwBufferSize)
        {
            lpPath[dwPathSize - 1] = (WCHAR)'\\';
            dwPathSize++;
        }
    }

    if (dwPathSize + dwAppendSize < dwBufferSize)
    {
        lstrcpynW(&lpPath[dwPathSize],
                  lpAppend,
                  dwAppendSize);
    }

    if (dwBufferSize >= dwTotalSize)
        lpPath[dwTotalSize - 1] = 0;

    return (dwBufferSize >= dwTotalSize);
}


/**************************************************************************
 * pSetupCenterWindowRelativeToParent [SETUPAPI.@]
 *
 * Centers a window relative to its parent.
 *
 * PARAMS
 *     hwnd [I] Window to center.
 *
 * RETURNS
 *     None
 */
VOID WINAPI
pSetupCenterWindowRelativeToParent(HWND hwnd)
{
    HWND hwndOwner;
    POINT ptOrigin;
    RECT rcWindow;
    RECT rcOwner;
    INT nWindowWidth, nWindowHeight;
    INT nOwnerWidth, nOwnerHeight;
    INT posX, posY;

    hwndOwner = GetWindow(hwnd, GW_OWNER);
    if (hwndOwner == NULL)
        return;

    ptOrigin.x = 0;
    ptOrigin.y = 0;
    ClientToScreen(hwndOwner, &ptOrigin);

    GetWindowRect(hwnd, &rcWindow);
    GetClientRect(hwndOwner, &rcOwner);

    nWindowWidth = rcWindow.right - rcWindow.left;
    nWindowHeight = rcWindow.bottom - rcWindow.top;

    nOwnerWidth = rcOwner.right - rcOwner.left;
    nOwnerHeight = rcOwner.bottom - rcOwner.top;

    posX = ((nOwnerWidth - nWindowWidth) / 2) + ptOrigin.x;
    posY = ((nOwnerHeight - nWindowHeight) / 2) + ptOrigin.y;

    MoveWindow(hwnd, posX, posY, nWindowWidth, nWindowHeight, 0);
}


/**************************************************************************
 * pSetupGetVersionInfoFromImage [SETUPAPI.@]
 *
 * Retrieves version information for a given file.
 *
 * PARAMS
 *     lpFileName       [I] File name
 *     lpFileVersion    [O] Pointer to the full file version
 *     lpVersionVarSize [O] Pointer to the size of the variable version
 *                          information
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI
pSetupGetVersionInfoFromImage(LPWSTR lpFileName,
                              PULARGE_INTEGER lpFileVersion,
                              LPWORD lpVersionVarSize)
{
    DWORD dwHandle;
    DWORD dwSize;
    LPVOID lpInfo;
    UINT uSize;
    VS_FIXEDFILEINFO *lpFixedInfo;
    LPWORD lpVarSize;

    dwSize = GetFileVersionInfoSizeW(lpFileName, &dwHandle);
    if (dwSize == 0)
        return FALSE;

    lpInfo = MyMalloc(dwSize);
    if (lpInfo == NULL)
        return FALSE;

    if (!GetFileVersionInfoW(lpFileName, 0, dwSize, lpInfo))
    {
        MyFree(lpInfo);
        return FALSE;
    }

    if (!VerQueryValueW(lpInfo, BackSlash,
                        (LPVOID*)&lpFixedInfo, &uSize))
    {
        MyFree(lpInfo);
        return FALSE;
    }

    lpFileVersion->LowPart = lpFixedInfo->dwFileVersionLS;
    lpFileVersion->HighPart = lpFixedInfo->dwFileVersionMS;

    *lpVersionVarSize = 0;
    if (!VerQueryValueW(lpInfo, TranslationRegKey,
                        (LPVOID*)&lpVarSize, &uSize))
    {
        MyFree(lpInfo);
        return TRUE;
    }

    if (uSize >= 4)
    {
        *lpVersionVarSize = *lpVarSize;
    }

    MyFree(lpInfo);

    return TRUE;
}

/***********************************************************************
 *      SetupUninstallOEMInfW  (SETUPAPI.@)
 */
BOOL WINAPI SetupUninstallOEMInfW( PCWSTR inf_file, DWORD flags, PVOID reserved )
{
    static const WCHAR infW[] = {'\\','i','n','f','\\',0};
    WCHAR target[MAX_PATH];

    TRACE("%s, 0x%08x, %p\n", debugstr_w(inf_file), flags, reserved);

    if (!inf_file)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!GetWindowsDirectoryW( target, sizeof(target)/sizeof(WCHAR) )) return FALSE;

    strcatW( target, infW );
    strcatW( target, inf_file );

    if (flags & SUOI_FORCEDELETE)
        return DeleteFileW(target);

    FIXME("not deleting %s\n", debugstr_w(target));

    return TRUE;
}

/***********************************************************************
 *      SetupUninstallOEMInfA  (SETUPAPI.@)
 */
BOOL WINAPI SetupUninstallOEMInfA( PCSTR inf_file, DWORD flags, PVOID reserved )
{
    BOOL ret;
    WCHAR *inf_fileW = NULL;

    TRACE("%s, 0x%08x, %p\n", debugstr_a(inf_file), flags, reserved);

    if (inf_file && !(inf_fileW = strdupAtoW( inf_file ))) return FALSE;
    ret = SetupUninstallOEMInfW( inf_fileW, flags, reserved );
    HeapFree( GetProcessHeap(), 0, inf_fileW );
    return ret;
}

/***********************************************************************
 *      InstallCatalog  (SETUPAPI.@)
 */
DWORD WINAPI InstallCatalog( LPCSTR catalog, LPCSTR basename, LPSTR fullname )
{
    FIXME("%s, %s, %p\n", debugstr_a(catalog), debugstr_a(basename), fullname);
    return 0;
}

/***********************************************************************
 *      pSetupInstallCatalog  (SETUPAPI.@)
 */
DWORD WINAPI pSetupInstallCatalog( LPCWSTR catalog, LPCWSTR basename, LPWSTR fullname )
{
    HCATADMIN admin;
    HCATINFO cat;

    TRACE ("%s, %s, %p\n", debugstr_w(catalog), debugstr_w(basename), fullname);

    if (!CryptCATAdminAcquireContext(&admin,NULL,0))
        return GetLastError();

    if (!(cat = CryptCATAdminAddCatalog( admin, (PWSTR)catalog, (PWSTR)basename, 0 )))
    {
        DWORD rc = GetLastError();
        CryptCATAdminReleaseContext(admin, 0);
        return rc;
    }
    CryptCATAdminReleaseCatalogContext(admin, cat, 0);
    CryptCATAdminReleaseContext(admin,0);

    if (fullname)
        FIXME("not returning full installed catalog path\n");

    return NO_ERROR;
}

static UINT detect_compression_type( LPCWSTR file )
{
    DWORD size;
    HANDLE handle;
    UINT type = FILE_COMPRESSION_NONE;
    static const BYTE LZ_MAGIC[] = { 0x53, 0x5a, 0x44, 0x44, 0x88, 0xf0, 0x27, 0x33 };
    static const BYTE MSZIP_MAGIC[] = { 0x4b, 0x57, 0x41, 0x4a };
    static const BYTE NTCAB_MAGIC[] = { 0x4d, 0x53, 0x43, 0x46 };
    BYTE buffer[8];

    handle = CreateFileW( file, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
    if (handle == INVALID_HANDLE_VALUE)
    {
        ERR("cannot open file %s\n", debugstr_w(file));
        return FILE_COMPRESSION_NONE;
    }
    if (!ReadFile( handle, buffer, sizeof(buffer), &size, NULL ) || size != sizeof(buffer))
    {
        CloseHandle( handle );
        return FILE_COMPRESSION_NONE;
    }
    if (!memcmp( buffer, LZ_MAGIC, sizeof(LZ_MAGIC) )) type = FILE_COMPRESSION_WINLZA;
    else if (!memcmp( buffer, MSZIP_MAGIC, sizeof(MSZIP_MAGIC) )) type = FILE_COMPRESSION_MSZIP;
    else if (!memcmp( buffer, NTCAB_MAGIC, sizeof(NTCAB_MAGIC) )) type = FILE_COMPRESSION_MSZIP; /* not a typo */

    CloseHandle( handle );
    return type;
}

static BOOL get_file_size( LPCWSTR file, DWORD *size )
{
    HANDLE handle;

    handle = CreateFileW( file, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
    if (handle == INVALID_HANDLE_VALUE)
    {
        ERR("cannot open file %s\n", debugstr_w(file));
        return FALSE;
    }
    *size = GetFileSize( handle, NULL );
    CloseHandle( handle );
    return TRUE;
}

static BOOL get_file_sizes_none( LPCWSTR source, DWORD *source_size, DWORD *target_size )
{
    DWORD size;

    if (!get_file_size( source, &size )) return FALSE;
    if (source_size) *source_size = size;
    if (target_size) *target_size = size;
    return TRUE;
}

static BOOL get_file_sizes_lz( LPCWSTR source, DWORD *source_size, DWORD *target_size )
{
    DWORD size;
    BOOL ret = TRUE;

    if (source_size)
    {
        if (!get_file_size( source, &size )) ret = FALSE;
        else *source_size = size;
    }
    if (target_size)
    {
        INT file;
        OFSTRUCT of;

        if ((file = LZOpenFileW( (LPWSTR)source, &of, OF_READ )) < 0)
        {
            ERR("cannot open source file for reading\n");
            return FALSE;
        }
        *target_size = LZSeek( file, 0, 2 );
        LZClose( file );
    }
    return ret;
}

static UINT CALLBACK file_compression_info_callback( PVOID context, UINT notification, UINT_PTR param1, UINT_PTR param2 )
{
    DWORD *size = context;
    FILE_IN_CABINET_INFO_W *info = (FILE_IN_CABINET_INFO_W *)param1;

    switch (notification)
    {
    case SPFILENOTIFY_FILEINCABINET:
    {
        *size = info->FileSize;
        return FILEOP_SKIP;
    }
    default: return NO_ERROR;
    }
}

static BOOL get_file_sizes_cab( LPCWSTR source, DWORD *source_size, DWORD *target_size )
{
    DWORD size;
    BOOL ret = TRUE;

    if (source_size)
    {
        if (!get_file_size( source, &size )) ret = FALSE;
        else *source_size = size;
    }
    if (target_size)
    {
        ret = SetupIterateCabinetW( source, 0, file_compression_info_callback, target_size );
    }
    return ret;
}

/***********************************************************************
 *      SetupGetFileCompressionInfoExA  (SETUPAPI.@)
 *
 * See SetupGetFileCompressionInfoExW.
 */
BOOL WINAPI SetupGetFileCompressionInfoExA( PCSTR source, PSTR name, DWORD len, PDWORD required,
                                            PDWORD source_size, PDWORD target_size, PUINT type )
{
    BOOL ret;
    WCHAR *nameW = NULL, *sourceW = NULL;
    DWORD nb_chars = 0;
    LPSTR nameA;

    TRACE("%s, %p, %d, %p, %p, %p, %p\n", debugstr_a(source), name, len, required,
          source_size, target_size, type);

    if (!source || !(sourceW = pSetupMultiByteToUnicode( source, CP_ACP ))) return FALSE;

    if (name)
    {
        ret = SetupGetFileCompressionInfoExW( sourceW, NULL, 0, &nb_chars, NULL, NULL, NULL );
        if (!(nameW = HeapAlloc( GetProcessHeap(), 0, nb_chars * sizeof(WCHAR) )))
        {
            MyFree( sourceW );
            return FALSE;
        }
    }
    ret = SetupGetFileCompressionInfoExW( sourceW, nameW, nb_chars, &nb_chars, source_size, target_size, type );
    if (ret)
    {
        if ((nameA = pSetupUnicodeToMultiByte( nameW, CP_ACP )))
        {
            if (name && len >= nb_chars) lstrcpyA( name, nameA );
            else
            {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                ret = FALSE;
            }
            MyFree( nameA );
        }
    }
    if (required) *required = nb_chars;
    HeapFree( GetProcessHeap(), 0, nameW );
    MyFree( sourceW );

    return ret;
}

/***********************************************************************
 *      SetupGetFileCompressionInfoExW  (SETUPAPI.@)
 *
 * Get compression type and compressed/uncompressed sizes of a given file.
 *
 * PARAMS
 *  source      [I] File to examine.
 *  name        [O] Actual filename used.
 *  len         [I] Length in characters of 'name' buffer.
 *  required    [O] Number of characters written to 'name'.
 *  source_size [O] Size of compressed file.
 *  target_size [O] Size of uncompressed file.
 *  type        [O] Compression type.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI SetupGetFileCompressionInfoExW( PCWSTR source, PWSTR name, DWORD len, PDWORD required,
                                            PDWORD source_size, PDWORD target_size, PUINT type )
{
    UINT comp;
    BOOL ret = FALSE;
    DWORD source_len;

    TRACE("%s, %p, %d, %p, %p, %p, %p\n", debugstr_w(source), name, len, required,
          source_size, target_size, type);

    if (!source) return FALSE;

    source_len = lstrlenW( source ) + 1;
    if (required) *required = source_len;
    if (name && len >= source_len)
    {
        lstrcpyW( name, source );
        ret = TRUE;
    }
    else return FALSE;

    comp = detect_compression_type( source );
    if (type) *type = comp;

    switch (comp)
    {
    case FILE_COMPRESSION_MSZIP:
    case FILE_COMPRESSION_NTCAB:  ret = get_file_sizes_cab( source, source_size, target_size ); break;
    case FILE_COMPRESSION_NONE:   ret = get_file_sizes_none( source, source_size, target_size ); break;
    case FILE_COMPRESSION_WINLZA: ret = get_file_sizes_lz( source, source_size, target_size ); break;
    default: break;
    }
    return ret;
}

/***********************************************************************
 *      SetupGetFileCompressionInfoA  (SETUPAPI.@)
 *
 * See SetupGetFileCompressionInfoW.
 */
DWORD WINAPI SetupGetFileCompressionInfoA( PCSTR source, PSTR *name, PDWORD source_size,
                                           PDWORD target_size, PUINT type )
{
    BOOL ret;
    DWORD error, required;
    LPSTR actual_name;

    TRACE("%s, %p, %p, %p, %p\n", debugstr_a(source), name, source_size, target_size, type);

    if (!source || !name || !source_size || !target_size || !type)
        return ERROR_INVALID_PARAMETER;

    ret = SetupGetFileCompressionInfoExA( source, NULL, 0, &required, NULL, NULL, NULL );
    if (!(actual_name = MyMalloc( required ))) return ERROR_NOT_ENOUGH_MEMORY;

    ret = SetupGetFileCompressionInfoExA( source, actual_name, required, &required,
                                          source_size, target_size, type );
    if (!ret)
    {
        error = GetLastError();
        MyFree( actual_name );
        return error;
    }
    *name = actual_name;
    return ERROR_SUCCESS;
}

/***********************************************************************
 *      SetupGetFileCompressionInfoW  (SETUPAPI.@)
 *
 * Get compression type and compressed/uncompressed sizes of a given file.
 *
 * PARAMS
 *  source      [I] File to examine.
 *  name        [O] Actual filename used.
 *  source_size [O] Size of compressed file.
 *  target_size [O] Size of uncompressed file.
 *  type        [O] Compression type.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: Win32 error code.
 */
DWORD WINAPI SetupGetFileCompressionInfoW( PCWSTR source, PWSTR *name, PDWORD source_size,
                                           PDWORD target_size, PUINT type )
{
    BOOL ret;
    DWORD error, required;
    LPWSTR actual_name;

    TRACE("%s, %p, %p, %p, %p\n", debugstr_w(source), name, source_size, target_size, type);

    if (!source || !name || !source_size || !target_size || !type)
        return ERROR_INVALID_PARAMETER;

    ret = SetupGetFileCompressionInfoExW( source, NULL, 0, &required, NULL, NULL, NULL );
    if (!(actual_name = MyMalloc( required*sizeof(WCHAR) ))) return ERROR_NOT_ENOUGH_MEMORY;

    ret = SetupGetFileCompressionInfoExW( source, actual_name, required, &required,
                                          source_size, target_size, type );
    if (!ret)
    {
        error = GetLastError();
        MyFree( actual_name );
        return error;
    }
    *name = actual_name;
    return ERROR_SUCCESS;
}

static DWORD decompress_file_lz( LPCWSTR source, LPCWSTR target )
{
    DWORD ret;
    LONG error;
    INT src, dst;
    OFSTRUCT sof, dof;

    if ((src = LZOpenFileW( (LPWSTR)source, &sof, OF_READ )) < 0)
    {
        ERR("cannot open source file for reading\n");
        return ERROR_FILE_NOT_FOUND;
    }
    if ((dst = LZOpenFileW( (LPWSTR)target, &dof, OF_CREATE )) < 0)
    {
        ERR("cannot open target file for writing\n");
        LZClose( src );
        return ERROR_FILE_NOT_FOUND;
    }
    if ((error = LZCopy( src, dst )) >= 0) ret = ERROR_SUCCESS;
    else
    {
        WARN("failed to decompress file %d\n", error);
        ret = ERROR_INVALID_DATA;
    }

    LZClose( src );
    LZClose( dst );
    return ret;
}

struct callback_context
{
    BOOL has_extracted;
    LPCWSTR target;
};

static UINT CALLBACK decompress_or_copy_callback( PVOID context, UINT notification, UINT_PTR param1, UINT_PTR param2 )
{
    struct callback_context *context_info = context;
    FILE_IN_CABINET_INFO_W *info = (FILE_IN_CABINET_INFO_W *)param1;

    switch (notification)
    {
    case SPFILENOTIFY_FILEINCABINET:
    {
        if (context_info->has_extracted)
            return FILEOP_ABORT;

        TRACE("Requesting extraction of cabinet file %s\n",
              wine_dbgstr_w(info->NameInCabinet));
        strcpyW( info->FullTargetName, context_info->target );
        context_info->has_extracted = TRUE;
        return FILEOP_DOIT;
    }
    default: return NO_ERROR;
    }
}

static DWORD decompress_file_cab( LPCWSTR source, LPCWSTR target )
{
    struct callback_context context = {0, target};
    BOOL ret;

    ret = SetupIterateCabinetW( source, 0, decompress_or_copy_callback, &context );

    if (ret) return ERROR_SUCCESS;
    else return GetLastError();
}

/***********************************************************************
 *      SetupDecompressOrCopyFileA  (SETUPAPI.@)
 *
 * See SetupDecompressOrCopyFileW.
 */
DWORD WINAPI SetupDecompressOrCopyFileA( PCSTR source, PCSTR target, PUINT type )
{
    DWORD ret = 0;
    WCHAR *sourceW = NULL, *targetW = NULL;

    if (source && !(sourceW = pSetupMultiByteToUnicode( source, CP_ACP ))) return FALSE;
    if (target && !(targetW = pSetupMultiByteToUnicode( target, CP_ACP )))
    {
        MyFree( sourceW );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ret = SetupDecompressOrCopyFileW( sourceW, targetW, type );

    MyFree( sourceW );
    MyFree( targetW );

    return ret;
}

/***********************************************************************
 *      SetupDecompressOrCopyFileW  (SETUPAPI.@)
 *
 * Copy a file and decompress it if needed.
 *
 * PARAMS
 *  source [I] File to copy.
 *  target [I] Filename of the copy.
 *  type   [I] Compression type.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: Win32 error code.
 */
DWORD WINAPI SetupDecompressOrCopyFileW( PCWSTR source, PCWSTR target, PUINT type )
{
    UINT comp;
    DWORD ret = ERROR_INVALID_PARAMETER;

    if (!source || !target) return ERROR_INVALID_PARAMETER;

    if (!type) comp = detect_compression_type( source );
    else comp = *type;

    switch (comp)
    {
    case FILE_COMPRESSION_NONE:
        if (CopyFileW( source, target, FALSE )) ret = ERROR_SUCCESS;
        else ret = GetLastError();
        break;
    case FILE_COMPRESSION_WINLZA:
        ret = decompress_file_lz( source, target );
        break;
    case FILE_COMPRESSION_NTCAB:
    case FILE_COMPRESSION_MSZIP:
        ret = decompress_file_cab( source, target );
        break;
    default:
        WARN("unknown compression type %d\n", comp);
        break;
    }

    TRACE("%s -> %s %d\n", debugstr_w(source), debugstr_w(target), comp);
    return ret;
}

/*
 * implemented (used by pSetupGuidFromString)
 */
static BOOL TrimGuidString(PCWSTR szString, LPWSTR szNewString)
{
    WCHAR szBuffer[39];
    INT Index;

    if (wcslen(szString) == 38)
    {
        if ((szString[0] == L'{') && (szString[37] == L'}'))
        {
            for (Index = 0; Index < wcslen(szString); Index++)
                szBuffer[Index] = szString[Index + 1];

            szBuffer[36] = L'\0';
            wcscpy(szNewString, szBuffer);
            return TRUE;
        }
    }
    szNewString[0] = L'\0';
    return FALSE;
}

/*
 * implemented
 */
DWORD
WINAPI
pSetupGuidFromString(PCWSTR pString, LPGUID lpGUID)
{
    RPC_STATUS Status;
    WCHAR szBuffer[39];

    if (!TrimGuidString(pString, szBuffer))
    {
        return RPC_S_INVALID_STRING_UUID;
    }

    Status = UuidFromStringW(szBuffer, lpGUID);
    if (Status != RPC_S_OK)
    {
        return RPC_S_INVALID_STRING_UUID;
    }

    return NO_ERROR;
}

/*
 * implemented
 */
DWORD
WINAPI
pSetupStringFromGuid(LPGUID lpGUID, PWSTR pString, DWORD dwStringLen)
{
    RPC_STATUS Status;
    RPC_WSTR rpcBuffer;
    WCHAR szBuffer[39];

    if (dwStringLen < 39)
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    Status = UuidToStringW(lpGUID, &rpcBuffer);
    if (Status != RPC_S_OK)
    {
        return Status;
    }

    wcscpy(szBuffer, L"{");
    wcscat(szBuffer, rpcBuffer);
    wcscat(szBuffer, L"}");

    wcscpy(pString, szBuffer);

    RpcStringFreeW(&rpcBuffer);
    return NO_ERROR;
}

/*
 * implemented
 */
BOOL
WINAPI
pSetupIsGuidNull(LPGUID lpGUID)
{
    return IsEqualGUID(lpGUID, &GUID_NULL);
}

/*
 * implemented
 */
BOOL
WINAPI
pSetupIsUserAdmin(VOID)
{
    SID_IDENTIFIER_AUTHORITY Authority = {SECURITY_NT_AUTHORITY};
    BOOL bResult = FALSE;
    PSID lpSid;

    if (!AllocateAndInitializeSid(&Authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                  &lpSid))
    {
        return FALSE;
    }

    if (!CheckTokenMembership(NULL, lpSid, &bResult))
    {
        bResult = FALSE;
    }

    FreeSid(lpSid);

    return bResult;
}

/***********************************************************************
 *		SetupInitializeFileLogW(SETUPAPI.@)
 */
HSPFILELOG WINAPI SetupInitializeFileLogW(LPCWSTR LogFileName, DWORD Flags)
{
    struct FileLog * Log;
    HANDLE hLog;
    WCHAR Windir[MAX_PATH];
    DWORD ret;

    TRACE("%s, 0x%x\n",debugstr_w(LogFileName),Flags);

    if (Flags & SPFILELOG_SYSTEMLOG)
    {
        if (!pSetupIsUserAdmin() && !(Flags & SPFILELOG_QUERYONLY))
        {
            /* insufficient privileges */
            SetLastError(ERROR_ACCESS_DENIED);
            return INVALID_HANDLE_VALUE;
        }

        if (LogFileName || (Flags & SPFILELOG_FORCENEW))
        {
            /* invalid parameter */
            SetLastError(ERROR_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }

        ret = GetSystemWindowsDirectoryW(Windir, MAX_PATH);
        if (!ret || ret >= MAX_PATH)
        {
            /* generic failure */
            return INVALID_HANDLE_VALUE;
        }

        /* append path */
        wcscat(Windir, L"repair\\setup.log");
    }
    else
    {
        if (!LogFileName)
        {
            /* invalid parameter */
            SetLastError(ERROR_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }
        /* copy filename */
        wcsncpy(Windir, LogFileName, MAX_PATH);
    }

    if (FileExists(Windir, NULL))
    {
        /* take ownership */
        ret = TakeOwnershipOfFile(Windir);

        if (ret != ERROR_SUCCESS)
        {
            /* failed */
            SetLastError(ret);
            return INVALID_HANDLE_VALUE;
        }

        if (!SetFileAttributesW(Windir, FILE_ATTRIBUTE_NORMAL))
        {
            /* failed */
            return INVALID_HANDLE_VALUE;
        }

        if ((Flags & SPFILELOG_FORCENEW))
        {
            if (!DeleteFileW(Windir))
            {
                /* failed */
                return INVALID_HANDLE_VALUE;
            }
        }
    }

    /* open log file */
    hLog = CreateFileW(Windir,
                       (Flags & SPFILELOG_QUERYONLY) ? GENERIC_READ : GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hLog == INVALID_HANDLE_VALUE)
    {
        /* failed */
        return INVALID_HANDLE_VALUE;
    }

    /* close log handle */
    CloseHandle(hLog);

    /* allocate file log struct */
    Log = HeapAlloc(GetProcessHeap(), 0, sizeof(struct FileLog));
    if (!Log)
    {
        /* not enough memory */
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }

    /* initialize log */
    Log->LogName = HeapAlloc(GetProcessHeap(), 0, (wcslen(Windir)+1) * sizeof(WCHAR));
    if (!Log->LogName)
    {
        /* not enough memory */
        HeapFree(GetProcessHeap(), 0, Log);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }

    wcscpy(Log->LogName, Windir);
    Log->ReadOnly = (Flags & SPFILELOG_QUERYONLY);
    Log->SystemLog = (Flags & SPFILELOG_SYSTEMLOG);

    return (HSPFILELOG)Log;
}

/***********************************************************************
 *		SetupInitializeFileLogA(SETUPAPI.@)
 */
HSPFILELOG WINAPI SetupInitializeFileLogA(LPCSTR LogFileName, DWORD Flags)
{
    HSPFILELOG hLog;
    LPWSTR LogFileNameW = NULL;

    TRACE("%s, 0x%x\n",debugstr_a(LogFileName),Flags);

    if (LogFileName)
    {
        LogFileNameW = strdupAtoW(LogFileName);

        if (!LogFileNameW)
        {
            /* not enough memory */
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return INVALID_HANDLE_VALUE;
        }

        hLog = SetupInitializeFileLogW(LogFileNameW, Flags);
        HeapFree(GetProcessHeap(), 0, LogFileNameW);
    }
    else
    {
        hLog = SetupInitializeFileLogW(NULL, Flags);
    }

    return hLog;
}

/***********************************************************************
 *		SetupTerminateFileLog(SETUPAPI.@)
 */
BOOL WINAPI SetupTerminateFileLog(HANDLE FileLogHandle)
{
    struct FileLog * Log;

    TRACE ("%p\n",FileLogHandle);

    Log = (struct FileLog *)FileLogHandle;

    /* free file log handle */
    HeapFree(GetProcessHeap(), 0, Log->LogName);
    HeapFree(GetProcessHeap(), 0, Log);

    SetLastError(ERROR_SUCCESS);

    return TRUE;
}

/***********************************************************************
 *      SetupCloseLog(SETUPAPI.@)
 */
void WINAPI SetupCloseLog(void)
{
    EnterCriticalSection(&setupapi_cs);

    CloseHandle(setupact);
    setupact = INVALID_HANDLE_VALUE;

    CloseHandle(setuperr);
    setuperr = INVALID_HANDLE_VALUE;

    LeaveCriticalSection(&setupapi_cs);
}

/***********************************************************************
 *      SetupOpenLog(SETUPAPI.@)
 */
BOOL WINAPI SetupOpenLog(BOOL reserved)
{
    WCHAR path[MAX_PATH];

    static const WCHAR setupactlog[] = {'\\','s','e','t','u','p','a','c','t','.','l','o','g',0};
    static const WCHAR setuperrlog[] = {'\\','s','e','t','u','p','e','r','r','.','l','o','g',0};

    EnterCriticalSection(&setupapi_cs);

    if (setupact != INVALID_HANDLE_VALUE && setuperr != INVALID_HANDLE_VALUE)
    {
        LeaveCriticalSection(&setupapi_cs);
        return TRUE;
    }

    GetWindowsDirectoryW(path, MAX_PATH);
    lstrcatW(path, setupactlog);

    setupact = CreateFileW(path, FILE_GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
                           NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (setupact == INVALID_HANDLE_VALUE)
    {
        LeaveCriticalSection(&setupapi_cs);
        return FALSE;
    }

    SetFilePointer(setupact, 0, NULL, FILE_END);

    GetWindowsDirectoryW(path, MAX_PATH);
    lstrcatW(path, setuperrlog);

    setuperr = CreateFileW(path, FILE_GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
                           NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (setuperr == INVALID_HANDLE_VALUE)
    {
        CloseHandle(setupact);
        setupact = INVALID_HANDLE_VALUE;
        LeaveCriticalSection(&setupapi_cs);
        return FALSE;
    }

    SetFilePointer(setuperr, 0, NULL, FILE_END);

    LeaveCriticalSection(&setupapi_cs);

    return TRUE;
}

/***********************************************************************
 *      SetupLogErrorA(SETUPAPI.@)
 */
BOOL WINAPI SetupLogErrorA(LPCSTR message, LogSeverity severity)
{
    static const char null[] = "(null)";
    BOOL ret;
    DWORD written;
    DWORD len;

    EnterCriticalSection(&setupapi_cs);

    if (setupact == INVALID_HANDLE_VALUE || setuperr == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_FILE_INVALID);
        ret = FALSE;
        goto done;
    }

    if (message == NULL)
        message = null;

    len = lstrlenA(message);

    ret = WriteFile(setupact, message, len, &written, NULL);
    if (!ret)
        goto done;

    if (severity >= LogSevMaximum)
    {
        ret = FALSE;
        goto done;
    }

    if (severity > LogSevInformation)
        ret = WriteFile(setuperr, message, len, &written, NULL);

done:
    LeaveCriticalSection(&setupapi_cs);
    return ret;
}

/***********************************************************************
 *      SetupLogErrorW(SETUPAPI.@)
 */
BOOL WINAPI SetupLogErrorW(LPCWSTR message, LogSeverity severity)
{
    LPSTR msg = NULL;
    DWORD len;
    BOOL ret;

    if (message)
    {
        len = WideCharToMultiByte(CP_ACP, 0, message, -1, NULL, 0, NULL, NULL);
        msg = HeapAlloc(GetProcessHeap(), 0, len);
        if (msg == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        WideCharToMultiByte(CP_ACP, 0, message, -1, msg, len, NULL, NULL);
    }

    /* This is the normal way to proceed. The log files are ASCII files
     * and W is to be converted.
     */
    ret = SetupLogErrorA(msg, severity);

    HeapFree(GetProcessHeap(), 0, msg);
    return ret;
}
