/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/backup.c
 * PURPOSE:         Backup functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <k32.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * @unimplemented
 */
BOOL
WINAPI
BackupRead(
    HANDLE hFile,
    LPBYTE lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    BOOL bAbort,
    BOOL bProcessSecurity,
    LPVOID *lpContext)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
BackupSeek(
    HANDLE hFile,
    DWORD dwLowBytesToSeek,
    DWORD dwHighBytesToSeek,
    LPDWORD lpdwLowByteSeeked,
    LPDWORD lpdwHighByteSeeked,
    LPVOID *lpContext)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
BackupWrite(
    HANDLE hFile,
    LPBYTE lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    BOOL bAbort,
    BOOL bProcessSecurity,
    LPVOID *lpContext)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

/* EOF */
