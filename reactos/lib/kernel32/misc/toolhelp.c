/* $Id: toolhelp.c,v 1.1 2003/01/05 10:07:08 robd Exp $
 *
 * KERNEL32.DLL toolhelp functions
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/toolhelp.c
 * PURPOSE:         Toolhelp functions
 * PROGRAMMER:      Robert Dickenson ( robd@mok.lvcm.com)
 * UPDATE HISTORY:
 *                  Created 05 January 2003
 */

#include <windows.h>
#include <tlhelp32.h>


BOOL
STDCALL
Process32First(HANDLE hSnapshot, LPPROCESSENTRY32 lppe)
{
    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}

BOOL
STDCALL
Process32Next(HANDLE hSnapshot, LPPROCESSENTRY32 lppe)
{
    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}

BOOL
STDCALL
Process32FirstW(HANDLE hSnapshot, LPPROCESSENTRY32W lppe)
{
    if (!lppe || lppe->dwSize != sizeof(PROCESSENTRY32W)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}

BOOL
STDCALL
Process32NextW(HANDLE hSnapshot, LPPROCESSENTRY32W lppe)
{
    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
}

HANDLE
STDCALL
CreateToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID)
{
    // return open handle to snapshot on success, -1 on failure
    // the snapshot handle behavies like an object handle
    HANDLE hSnapshot = -1;

    if (dwFlags & TH32CS_INHERIT) {
    }
//    if (dwFlags & TH32CS_SNAPALL) { // == (TH32CS_SNAPHEAPLIST + TH32CS_SNAPMODULE + TH32CS_SNAPPROCESS + TH32CS_SNAPTHREAD)
//    }
    if (dwFlags & TH32CS_SNAPHEAPLIST) {
    }
    if (dwFlags & TH32CS_SNAPMODULE) {
    }
    if (dwFlags & TH32CS_SNAPPROCESS) {
    }
    if (dwFlags & TH32CS_SNAPTHREAD) {
    }

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    // caller must use CloseHandle to destroy the returned snapshot handle
    return hSnapshot;
}

BOOL
WINAPI
Toolhelp32ReadProcessMemory(DWORD th32ProcessID,
  LPCVOID lpBaseAddress, LPVOID lpBuffer,
  DWORD cbRead, LPDWORD lpNumberOfBytesRead)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/* EOF */
