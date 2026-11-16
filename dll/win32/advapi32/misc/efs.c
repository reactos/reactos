/*
 * PROJECT:     VerOS NT System Libraries
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        dll/win32/advapi32/misc/efs.c
 * PURPOSE:     Encrypted File System support
 * COPYRIGHT:   See COPYING in the top level directory
 *              Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Christoph_vW
 *              Ryan Wilson <ryan@verosnt.com>
 */

#include <advapi32.h>

#include <winefs.h>

#ifndef FSCTL_SET_ENCRYPTION
#define FSCTL_SET_ENCRYPTION CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 53, METHOD_NEITHER, FILE_ANY_ACCESS)
#endif
#ifndef FILE_SET_ENCRYPTION
#define FILE_SET_ENCRYPTION 0x00000001
#endif

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

/*
 * @unimplemented
 */
DWORD WINAPI
AddUsersToEncryptedFile(LPCWSTR lpcwstr,
                        PENCRYPTION_CERTIFICATE_LIST pencryption_certificate_list)
{
    FIXME("%s() not implemented!\n", __FUNCTION__);
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
BOOL WINAPI
DecryptFileA(LPCSTR lpFileName,
             DWORD dwReserved)
{
    UNICODE_STRING FileName;
    BOOL ret;

    if (!RtlCreateUnicodeStringFromAsciiz(&FileName, lpFileName))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    ret = DecryptFileW(FileName.Buffer, dwReserved);

    if (FileName.Buffer != NULL)
        RtlFreeUnicodeString(&FileName);
    return ret;
}


/*
 * @unimplemented
 */
BOOL WINAPI
DecryptFileW(LPCWSTR lpFileName,
             DWORD dwReserved)
{
    FIXME("%s(%S) not implemented!\n", __FUNCTION__, lpFileName);
    return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
EncryptFileA(LPCSTR lpFileName)
{
    UNICODE_STRING FileName;
    BOOL ret;

    if (!RtlCreateUnicodeStringFromAsciiz(&FileName, lpFileName))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    ret = EncryptFileW(FileName.Buffer);

    if (FileName.Buffer != NULL)
        RtlFreeUnicodeString(&FileName);
    return ret;
}


/*
 * @implemented
 */
BOOL WINAPI
EncryptFileW(LPCWSTR lpFileName)
{
    /* FSCTL_SET_ENCRYPTION is METHOD_NEITHER. The $EFS stream is stored, and
     * the 32 bytes after it are the file key kept on the open file. */
    UCHAR Request[4 + 16 + 32];
    HANDLE File;
    DWORD Wrote = 0;
    DWORD Attributes;

    if (lpFileName == NULL || lpFileName[0] == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Attributes = GetFileAttributesW(lpFileName);
    if (Attributes == INVALID_FILE_ATTRIBUTES)
        return FALSE;

    File = CreateFileW(lpFileName,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       NULL,
                       OPEN_EXISTING,
                       (Attributes & FILE_ATTRIBUTE_DIRECTORY) ? FILE_FLAG_BACKUP_SEMANTICS : 0,
                       NULL);
    if (File == INVALID_HANDLE_VALUE)
        return FALSE;

    ZeroMemory(Request, sizeof(Request));
    *(ULONG *)Request = FILE_SET_ENCRYPTION;
    Request[4] = 1;
    if (!RtlGenRandom(Request + 4 + 16, 32))
    {
        DWORD Tick = GetTickCount();
        ULONG Index;
        for (Index = 0; Index < 32; Index++)
            Request[4 + 16 + Index] = (UCHAR)(Tick + Index * 17);
    }

    if (!DeviceIoControl(File, FSCTL_SET_ENCRYPTION, Request, sizeof(Request), NULL, 0, &Wrote, NULL))
    {
        CloseHandle(File);
        return FALSE;
    }

    CloseHandle(File);
    return TRUE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
EncryptionDisable(LPCWSTR DirPath,
                  BOOL Disable)
{
    FIXME("%s() not implemented!\n", __FUNCTION__);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @implemented
 */
BOOL WINAPI
FileEncryptionStatusA(LPCSTR lpFileName,
                      LPDWORD lpStatus)
{
    UNICODE_STRING FileName;
    BOOL ret = FALSE;

    TRACE("(%s, %p)\n", lpFileName, lpStatus);

    FileName.Buffer = NULL;

    if (!RtlCreateUnicodeStringFromAsciiz(&FileName, lpFileName))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }

    ret = FileEncryptionStatusW(FileName.Buffer, lpStatus);

cleanup:
    if (FileName.Buffer != NULL)
        RtlFreeUnicodeString(&FileName);

    return ret;
}

/*
 * @unimplemented
 */
BOOL WINAPI
FileEncryptionStatusW(LPCWSTR lpFileName,
                      LPDWORD lpStatus)
{
    FIXME("%s(%S) not implemented!\n", __FUNCTION__, lpFileName);

    if (!lpStatus)
        return FALSE;

    *lpStatus = FILE_SYSTEM_NOT_SUPPORT;

    return TRUE;
}


/*
 * @unimplemented
 */
VOID WINAPI
FreeEncryptionCertificateHashList(PENCRYPTION_CERTIFICATE_HASH_LIST pencryption_certificate_hash_list)
{
    FIXME("%s() not implemented!\n", __FUNCTION__);
    return;
}


/*
 * @unimplemented
 */
DWORD WINAPI
QueryRecoveryAgentsOnEncryptedFile(LPCWSTR lpctstr,
                                   PENCRYPTION_CERTIFICATE_HASH_LIST* pencryption_certificate_hash_list)
{
    FIXME("%s() not implemented!\n", __FUNCTION__);
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
DWORD WINAPI
QueryUsersOnEncryptedFile(LPCWSTR lpctstr,
                          PENCRYPTION_CERTIFICATE_HASH_LIST* pencryption_certificate_hash_list)
{
    FIXME("%s() not implemented!\n", __FUNCTION__);
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
DWORD WINAPI
RemoveUsersFromEncryptedFile(LPCWSTR lpcwstr,
                             PENCRYPTION_CERTIFICATE_HASH_LIST pencryption_certificate_hash_list)
{
    FIXME("%s() not implemented!\n", __FUNCTION__);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/* EOF */
