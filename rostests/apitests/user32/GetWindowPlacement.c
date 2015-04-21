/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for GetWindowPlacement
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>
#include <winuser.h>

#define ALIGN_DOWN_BY(size, align) \
    ((ULONG_PTR)(size) & ~((ULONG_PTR)(align) - 1))

#define ALIGN_UP_BY(size, align) \
    (ALIGN_DOWN_BY(((ULONG_PTR)(size) + align - 1), align))

START_TEST(GetWindowPlacement)
{
    BYTE buffer[sizeof(WINDOWPLACEMENT) + 16];
    PWINDOWPLACEMENT wp = (PVOID)buffer;
    DWORD error;
    BOOL ret;

    SetLastError(0xfeedfab1);
    ret = GetWindowPlacement(NULL, NULL);
    error = GetLastError();
    ok(ret == FALSE, "ret = %d\n", ret);
    ok(error == ERROR_INVALID_WINDOW_HANDLE, "error = %lu\n", error);

    SetLastError(0xfeedfab1);
    ret = GetWindowPlacement(GetDesktopWindow(), NULL);
    error = GetLastError();
    ok(ret == FALSE, "ret = %d\n", ret);
    ok(error == ERROR_NOACCESS, "error = %lu\n", error);

    SetLastError(0xfeedfab1);
    ret = GetWindowPlacement(GetDesktopWindow(), (PVOID)(UINT_PTR)-4);
    error = GetLastError();
    ok(ret == FALSE, "ret = %d\n", ret);
    ok(error == ERROR_NOACCESS, "error = %lu\n", error);

    FillMemory(buffer, sizeof(buffer), 0x55);
    SetLastError(0xfeedfab1);
    ret = GetWindowPlacement(GetDesktopWindow(), (PVOID)(ALIGN_UP_BY(buffer, 16) + 1));
    error = GetLastError();
    ok(ret == TRUE, "ret = %d\n", ret);
    ok(error == 0xfeedfab1, "error = %lu\n", error);

    FillMemory(wp, sizeof(*wp), 0x55);
    wp->length = 0;
    SetLastError(0xfeedfab1);
    ret = GetWindowPlacement(NULL, wp);
    error = GetLastError();
    ok(ret == FALSE, "ret = %d\n", ret);
    ok(error == ERROR_INVALID_WINDOW_HANDLE, "error = %lu\n", error);
    ok(wp->length == 0, "wp.length = %u\n", wp->length);

    FillMemory(wp, sizeof(*wp), 0x55);
    wp->length = 0;
    SetLastError(0xfeedfab1);
    ret = GetWindowPlacement(GetDesktopWindow(), wp);
    error = GetLastError();
    ok(ret == TRUE, "ret = %d\n", ret);
    ok(error == 0xfeedfab1, "error = %lu\n", error);
    ok(wp->length == sizeof(*wp), "wp.length = %u\n", wp->length);
    ok(wp->flags == 0, "wp.flags = %x\n", wp->flags);

    FillMemory(wp, sizeof(*wp), 0x55);
    wp->length = 1;
    SetLastError(0xfeedfab1);
    ret = GetWindowPlacement(GetDesktopWindow(), wp);
    error = GetLastError();
    ok(ret == TRUE, "ret = %d\n", ret);
    ok(error == 0xfeedfab1, "error = %lu\n", error);
    ok(wp->length == sizeof(*wp), "wp.length = %u\n", wp->length);
    ok(wp->flags == 0, "wp.flags = %x\n", wp->flags);

    FillMemory(wp, sizeof(*wp), 0x55);
    wp->length = sizeof(*wp) - 1;
    SetLastError(0xfeedfab1);
    ret = GetWindowPlacement(GetDesktopWindow(), wp);
    error = GetLastError();
    ok(ret == TRUE, "ret = %d\n", ret);
    ok(error == 0xfeedfab1, "error = %lu\n", error);
    ok(wp->length == sizeof(*wp), "wp.length = %u\n", wp->length);
    ok(wp->flags == 0, "wp.flags = %x\n", wp->flags);

    FillMemory(wp, sizeof(*wp), 0x55);
    wp->length = sizeof(*wp) + 1;
    SetLastError(0xfeedfab1);
    ret = GetWindowPlacement(GetDesktopWindow(), wp);
    error = GetLastError();
    ok(ret == TRUE, "ret = %d\n", ret);
    ok(error == 0xfeedfab1, "error = %lu\n", error);
    ok(wp->length == sizeof(*wp), "wp.length = %u\n", wp->length);
    ok(wp->flags == 0, "wp.flags = %x\n", wp->flags);

    FillMemory(wp, sizeof(*wp), 0x55);
    wp->length = sizeof(*wp);
    SetLastError(0xfeedfab1);
    ret = GetWindowPlacement(GetDesktopWindow(), wp);
    error = GetLastError();
    ok(ret == TRUE, "ret = %d\n", ret);
    ok(error == 0xfeedfab1, "error = %lu\n", error);
    ok(wp->length == sizeof(*wp), "wp.length = %u\n", wp->length);
    ok(wp->flags == 0, "wp.flags = %x\n", wp->flags);
    ok(wp->showCmd == SW_SHOWNORMAL, "wp.showCmd = %u\n", wp->showCmd);
    ok(wp->ptMinPosition.x == -1, "wp.ptMinPosition.x = %ld\n", wp->ptMinPosition.x);
    ok(wp->ptMinPosition.y == -1, "wp.ptMinPosition.x = %ld\n", wp->ptMinPosition.y);
    ok(wp->ptMaxPosition.x == -1, "wp.ptMaxPosition.x = %ld\n", wp->ptMaxPosition.x);
    ok(wp->ptMaxPosition.y == -1, "wp.ptMaxPosition.y = %ld\n", wp->ptMaxPosition.y);
    ok(wp->rcNormalPosition.left == 0, "wp.rcNormalPosition.left = %ld\n", wp->rcNormalPosition.left);
    ok(wp->rcNormalPosition.top == 0, "wp.rcNormalPosition.top = %ld\n", wp->rcNormalPosition.top);
    ok(wp->rcNormalPosition.right != 0 &&
       wp->rcNormalPosition.right != 0x55555555, "wp.rcNormalPosition.right = %ld\n", wp->rcNormalPosition.right);
    ok(wp->rcNormalPosition.bottom != 0 &&
       wp->rcNormalPosition.bottom != 0x55555555, "wp.rcNormalPosition.bottom = %ld\n", wp->rcNormalPosition.bottom);
}
