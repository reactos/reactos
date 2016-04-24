/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dlls/win32/advapi32/misc/efs.c
 * PURPOSE:         Encrypted File System support
 * PROGRAMMER:      Christoph_vW
 */

#include <advapi32.h>

#include <winefs.h>

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
 * @unimplemented
 */
BOOL WINAPI
EncryptFileW(LPCWSTR lpFileName)
{
    FIXME("%s() not implemented!\n", __FUNCTION__);
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
