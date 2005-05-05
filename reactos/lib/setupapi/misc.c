/*
 * Setupapi miscellaneous functions
 *
 * Copyright 2005 Eric Kohl
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "setupapi.h"

#include "wine/unicode.h"
#include "wine/debug.h"

#include "setupapi_private.h"


WINE_DEFAULT_DEBUG_CHANNEL(setupapi);


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
 * DuplicateString [SETUPAPI.@]
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
LPWSTR WINAPI DuplicateString(LPCWSTR lpSrc)
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

    TRACE("%lx %s %p %p %p\n",
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
 * IsUserAdmin [SETUPAPI.@]
 *
 * Checks whether the current user is a member of the Administrators group.
 *
 * PARAMS
 *     None
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI IsUserAdmin(VOID)
{
    SID_IDENTIFIER_AUTHORITY Authority = {SECURITY_NT_AUTHORITY};
    HANDLE hToken;
    DWORD dwSize;
    PTOKEN_GROUPS lpGroups;
    PSID lpSid;
    DWORD i;
    BOOL bResult = FALSE;

    TRACE("\n");

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        return FALSE;
    }

    if (!GetTokenInformation(hToken, TokenGroups, NULL, 0, &dwSize))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            CloseHandle(hToken);
            return FALSE;
        }
    }

    lpGroups = MyMalloc(dwSize);
    if (lpGroups == NULL)
    {
        CloseHandle(hToken);
        return FALSE;
    }

    if (!GetTokenInformation(hToken, TokenGroups, lpGroups, dwSize, &dwSize))
    {
        MyFree(lpGroups);
        CloseHandle(hToken);
        return FALSE;
    }

    CloseHandle(hToken);

    if (!AllocateAndInitializeSid(&Authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                  &lpSid))
    {
        MyFree(lpGroups);
        return FALSE;
    }

    for (i = 0; i < lpGroups->GroupCount; i++)
    {
        if (EqualSid(lpSid, &lpGroups->Groups[i].Sid))
        {
            bResult = TRUE;
            break;
        }
    }

    FreeSid(lpSid);
    MyFree(lpGroups);

    return bResult;
}


/**************************************************************************
 * MultiByteToUnicode [SETUPAPI.@]
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
LPWSTR WINAPI MultiByteToUnicode(LPCSTR lpMultiByteStr, UINT uCodePage)
{
    LPWSTR lpUnicodeStr;
    int nLength;

    TRACE("%s %lu\n", debugstr_a(lpMultiByteStr), uCodePage);

    nLength = MultiByteToWideChar(uCodePage, 0, lpMultiByteStr,
                                  -1, NULL, 0);
    if (nLength == 0)
        return NULL;

    lpUnicodeStr = MyMalloc(nLength * sizeof(WCHAR));
    if (lpUnicodeStr == NULL)
        return NULL;

    if (!MultiByteToWideChar(uCodePage, 0, lpMultiByteStr,
                             nLength, lpUnicodeStr, nLength))
    {
        MyFree(lpUnicodeStr);
        return NULL;
    }

    return lpUnicodeStr;
}


/**************************************************************************
 * UnicodeToMultiByte [SETUPAPI.@]
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
LPSTR WINAPI UnicodeToMultiByte(LPCWSTR lpUnicodeStr, UINT uCodePage)
{
    LPSTR lpMultiByteStr;
    int nLength;

    TRACE("%s %lu\n", debugstr_w(lpUnicodeStr), uCodePage);

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
 * EnablePrivilege [SETUPAPI.@]
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
BOOL WINAPI EnablePrivilege(LPCWSTR lpPrivilegeName, BOOL bEnable)
{
    TOKEN_PRIVILEGES Privileges;
    HANDLE hToken;
    BOOL bResult;

    TRACE("%s %s\n", debugstr_w(lpPrivilegeName), bEnable ? "TRUE" : "FALSE");

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
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
    if (OsVersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT)
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

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

    *pDst = DuplicateString(pSrc);

    return ERROR_SUCCESS;
}


/**************************************************************************
 * CaptureAndConvertAnsiArg [SETUPAPI.@]
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
DWORD WINAPI CaptureAndConvertAnsiArg(LPCSTR pSrc, LPWSTR *pDst)
{
    if (pDst == NULL)
        return ERROR_INVALID_PARAMETER;

    *pDst = MultiByteToUnicode(pSrc, CP_ACP);

    return ERROR_SUCCESS;
}


/**************************************************************************
 * OpenAndMapFileForRead [SETUPAPI.@]
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
DWORD WINAPI OpenAndMapFileForRead(LPCWSTR lpFileName,
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
 * UnmapAndCloseFile [SETUPAPI.@]
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
BOOL WINAPI UnmapAndCloseFile(HANDLE hFile, HANDLE hMapping, LPVOID lpBuffer)
{
    TRACE("%x %x %p\n",
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

    if (pOwner != NULL)
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

    SecDesc = (PSECURITY_DESCRIPTOR)MyMalloc(dwSize);
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

    SecDesc = (PSECURITY_DESCRIPTOR)MyRealloc(SecDesc, dwSize);
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
