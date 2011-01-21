/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dlls/win32/advapi32/misc/efs.c
 * PURPOSE:         Encrypted File System support
 * PROGRAMMER:      Christoph_vW
 */

#include <advapi32.h>
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);


/*
 * @implemented
 */
BOOL WINAPI
DecryptFileA(LPCSTR lpFileName, DWORD dwReserved)
{
    UNICODE_STRING FileName;
    NTSTATUS Status;
    BOOL ret;

    Status = RtlCreateUnicodeStringFromAsciiz(&FileName, lpFileName);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
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
BOOL WINAPI DecryptFileW(LPCWSTR lpFileName, DWORD dwReserved)
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
    NTSTATUS Status;
    BOOL ret;

    Status = RtlCreateUnicodeStringFromAsciiz(&FileName, lpFileName);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
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

/* EOF */
