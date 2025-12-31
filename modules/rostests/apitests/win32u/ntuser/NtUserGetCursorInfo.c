/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for NtUserCreateAcceleratorTable
 * COPYRIGHT:   Copyright 2025 Max Korostil <mrmks04@yandex.ru>
 */

#include "../win32nt.h"

START_TEST(NtUserGetCursorInfo)
{
    CURSORINFO cursor;
    DWORD error;
    BOOL res;

    /* 1. NULL pointer */
    SetLastError(0);
    res = NtUserGetCursorInfo(NULL);
    error = GetLastError();

    ok_int(res, FALSE);
    ok_int(error, ERROR_NOACCESS);
    
    /* 2. Hardcoded invalid pointer */
    SetLastError(0);
    res = NtUserGetCursorInfo((PCURSORINFO)(ULONG_PTR)0xDEADBEEF);
    error = GetLastError();

    ok_int(res, FALSE);
    ok_int(error, ERROR_NOACCESS);

    /* 3. Invalid cbSize */
    SetLastError(0);
    cursor.cbSize = 0;
    res = NtUserGetCursorInfo(&cursor);
    error = GetLastError();

    ok_int(res, FALSE);
    ok_int(error, ERROR_INVALID_PARAMETER);

    /* 4. Single CURSORINFO pointer */
    SetLastError(0);
    cursor.cbSize = sizeof(cursor);
    res = NtUserGetCursorInfo(&cursor);
    error = GetLastError();

    ok_int(res, TRUE);
    ok_int(error, ERROR_SUCCESS);

    /* 5. Double CURSORINFO pointer */
    SetLastError(0);
    cursor.cbSize = sizeof(cursor) * 2;
    res = NtUserGetCursorInfo(&cursor);
    error = GetLastError();

    ok_int(res, FALSE);
    ok_int(error, ERROR_INVALID_PARAMETER);
}
