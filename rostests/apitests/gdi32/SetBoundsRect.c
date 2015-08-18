/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for SetBoundsRect
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org
 */

#include <apitest.h>
#include <winuser.h>
#include <wingdi.h>

START_TEST(SetBoundsRect)
{
    HDC hDC;
    UINT ret;
    DWORD error;

    hDC = CreateCompatibleDC(GetDC(NULL));
    if (hDC == NULL)
    {
        skip("No DC\n");
        return;
    }

    SetLastError(0xbeeffeed);
    ret = SetBoundsRect(hDC, NULL, 0);
    error = GetLastError();
    ok(ret == (DCB_DISABLE | DCB_RESET), "ret = %u\n", ret);
    ok(error == 0xbeeffeed, "error = %lu\n", error);

    SetLastError(0xbeeffeed);
    ret = SetBoundsRect(hDC, NULL, DCB_ACCUMULATE);
    error = GetLastError();
    ok(ret == (DCB_DISABLE | DCB_RESET), "ret = %u\n", ret);
    ok(error == 0xbeeffeed, "error = %lu\n", error);

    SetLastError(0xbeeffeed);
    ret = SetBoundsRect(hDC, NULL, DCB_DISABLE);
    error = GetLastError();
    ok(ret == (DCB_DISABLE | DCB_RESET), "ret = %u\n", ret);
    ok(error == 0xbeeffeed, "error = %lu\n", error);

    SetLastError(0xbeeffeed);
    ret = SetBoundsRect(hDC, NULL, DCB_ENABLE);
    error = GetLastError();
    ok(ret == (DCB_DISABLE | DCB_RESET), "ret = %u\n", ret);
    ok(error == 0xbeeffeed, "error = %lu\n", error);

    SetLastError(0xbeeffeed);
    ret = SetBoundsRect(hDC, NULL, DCB_RESET);
    error = GetLastError();
    ok(ret == (DCB_ENABLE | DCB_RESET), "ret = %u\n", ret);
    ok(error == 0xbeeffeed, "error = %lu\n", error);

    DeleteDC(hDC);
}
