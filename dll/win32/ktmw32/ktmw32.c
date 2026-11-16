/*
 * PROJECT:     VerOS NT System Libraries
 * LICENSE:     GNU Lesser General Public License, version 2.1 or later (LGPL-2.1-or-later)
 * FILE:        dll/win32/ktmw32/ktmw32.c
 * PURPOSE:     Kernel Transaction Manager user stubs for Firefox xul.dll
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 *
 * CreateTransaction / CommitTransaction:
 * https://learn.microsoft.com/en-us/windows/win32/api/ktmw32/nf-ktmw32-createtransaction
 * https://learn.microsoft.com/en-us/windows/win32/api/ktmw32/nf-ktmw32-committransaction
 * Firefox PE import: xul.dll -> ktmw32.dll. NtCreateTransaction is still a
 * stub in ntdll; return failure so callers take the non-transaction path.
 */

#include <windef.h>
#include <winbase.h>

/*
 * @implemented
 */
HANDLE
WINAPI
CreateTransaction(LPSECURITY_ATTRIBUTES lpTransactionAttributes,
                  LPGUID UOW,
                  DWORD CreateOptions,
                  DWORD IsolationLevel,
                  DWORD IsolationFlags,
                  DWORD Timeout,
                  LPWSTR Description)
{
    UNREFERENCED_PARAMETER(lpTransactionAttributes);
    UNREFERENCED_PARAMETER(UOW);
    UNREFERENCED_PARAMETER(CreateOptions);
    UNREFERENCED_PARAMETER(IsolationLevel);
    UNREFERENCED_PARAMETER(IsolationFlags);
    UNREFERENCED_PARAMETER(Timeout);
    UNREFERENCED_PARAMETER(Description);
    SetLastError(ERROR_NOT_SUPPORTED);
    return INVALID_HANDLE_VALUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
CommitTransaction(HANDLE TransactionHandle)
{
    UNREFERENCED_PARAMETER(TransactionHandle);
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
}
