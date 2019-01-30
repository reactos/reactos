/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiArcInternal
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtGdiArcInternal)
{
    HDC hDC = CreateDCW(L"Display",NULL,NULL,NULL);

    SetLastError(ERROR_SUCCESS);
    ok_int(NtGdiArcInternal(0, 0, 0, 0, 0, 0, 0, 0, 0, 0), FALSE);
    ok_long(GetLastError(), ERROR_INVALID_HANDLE);

    SetLastError(ERROR_SUCCESS);
    ok_int(NtGdiArcInternal(0, hDC, 0, 0, 0, 0, 0, 0, 0, 0), TRUE);
    ok_int(NtGdiArcInternal(1, hDC, 0, 0, 0, 0, 0, 0, 0, 0), TRUE);
    ok_int(NtGdiArcInternal(2, hDC, 0, 0, 0, 0, 0, 0, 0, 0), TRUE);
    ok_int(NtGdiArcInternal(3, hDC, 0, 0, 0, 0, 0, 0, 0, 0), TRUE);
    ok_long(GetLastError(), ERROR_SUCCESS);

    SetLastError(ERROR_SUCCESS);
    ok_int(NtGdiArcInternal(4, hDC, 0, 0, 0, 0, 0, 0, 0, 0), FALSE);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);

    SetLastError(ERROR_SUCCESS);
    ok_int(NtGdiArcInternal(4, (HDC)10, 0, 0, 0, 0, 0, 0, 0, 0), FALSE);
    ok_long(GetLastError(), ERROR_INVALID_HANDLE);

    SetLastError(ERROR_SUCCESS);
    ok_int(NtGdiArcInternal(0, hDC, 10, 10, 0, 0, 0, 0, 0, 0), TRUE);
    ok_int(NtGdiArcInternal(0, hDC, 10, 10, -10, -10, 0, 0, 0, 0), TRUE);
    ok_int(NtGdiArcInternal(0, hDC, 0, 0, 0, 0, 10, 0, -10, 0), TRUE);

// was passiert, wenn left > right ? einfach tauschen?

    DeleteDC(hDC);
}
