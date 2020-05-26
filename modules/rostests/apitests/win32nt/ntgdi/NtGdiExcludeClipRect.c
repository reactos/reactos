/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiExcludeClipRect
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtGdiExcludeClipRect)
{
    //HDC hdc;
// test what params are accepted for what operations
// 0? invalid? are params maybe ignored in some cases?
// LastError

    /* Preparation */
    //hdc = CreateCompatibleDC(NULL);

    /* Test NULL DC */
    SetLastError(0x12345);
    ok_int(NtGdiExcludeClipRect(NULL, 0, 0, 0, 0), ERROR);
    ok_int(GetLastError(), ERROR_INVALID_HANDLE);

    /* Test invalid DC */
    SetLastError(0x12345);
    ok_int(NtGdiExcludeClipRect((HDC)(ULONG_PTR)0x12345, 0, 0, 0, 0), ERROR);
    ok((GetLastError() == ERROR_INVALID_HANDLE), "ERROR_INVALID_HANDLE, got %ld\n", GetLastError());

}
