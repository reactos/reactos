/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for the window icon types of WM_GETICON/WM_SETICON
 */

#include "precomp.h"

#define TEST_CLASS_NAME L"GetWindowIconTestClass"
#define TEST_CLASS_SM_NAME L"GetWindowIconTestClassSm"

static HICON
CreateIconOfSize(INT cx, INT cy)
{
    ICONINFO IconInfo;
    HICON hIcon;

    ZeroMemory(&IconInfo, sizeof(IconInfo));
    IconInfo.fIcon = TRUE;
    IconInfo.hbmMask = CreateBitmap(cx, cy, 1, 1, NULL);
    IconInfo.hbmColor = CreateBitmap(cx, cy, 1, 32, NULL);
    hIcon = CreateIconIndirect(&IconInfo);
    DeleteObject(IconInfo.hbmMask);
    DeleteObject(IconInfo.hbmColor);
    return hIcon;
}

static BOOL
GetIconBitmapSize(HICON hIcon, INT *pcx, INT *pcy)
{
    ICONINFO IconInfo;
    BITMAP Bitmap;
    BOOL Ret;

    if (!GetIconInfo(hIcon, &IconInfo))
        return FALSE;

    Ret = GetObjectW(IconInfo.hbmColor, sizeof(Bitmap), &Bitmap) == sizeof(Bitmap);
    if (Ret)
    {
        *pcx = Bitmap.bmWidth;
        *pcy = Bitmap.bmHeight;
    }

    DeleteObject(IconInfo.hbmMask);
    DeleteObject(IconInfo.hbmColor);
    return Ret;
}

static HWND
CreateTestWindow(LPCWSTR lpszClassName, HINSTANCE hInstance)
{
    return CreateWindowExW(0, lpszClassName, L"GetWindowIcon", WS_OVERLAPPEDWINDOW,
                           100, 100, 200, 200, NULL, NULL, hInstance, NULL);
}

/* The class icon must not be reported as ICON_SMALL2 */
static void
TestClassIconIsNotUsed(HWND hWnd)
{
    HICON hIconSm2;

    hIconSm2 = (HICON)SendMessageW(hWnd, WM_GETICON, ICON_SMALL2, 0);
    ok(hIconSm2 == NULL, "Expected no ICON_SMALL2, got %p\n", hIconSm2);
}

/* A resource icon carries a ready small frame, which ICON_SMALL2 has to use */
static void
TestResourceWindowIcons(HWND hWnd, HICON hIconBig, HICON hIconSmall)
{
    HICON hIconSm2, hIconSm2Again;
    DWORD dwUsersBefore, dwUsersAfter;
    INT cx = 0, cy = 0, i;

    ok(SendMessageW(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig) == 0,
       "Expected no previous ICON_BIG\n");
    ok(SendMessageW(hWnd, WM_GETICON, ICON_SMALL, 0) == 0,
       "Expected no ICON_SMALL\n");

    hIconSm2 = (HICON)SendMessageW(hWnd, WM_GETICON, ICON_SMALL2, 0);
    ok(hIconSm2 != NULL, "Expected an ICON_SMALL2\n");
    ok(hIconSm2 != hIconBig, "ICON_SMALL2 must not be ICON_BIG\n");
    if (hIconSm2)
    {
        ok(GetIconBitmapSize(hIconSm2, &cx, &cy), "GetIconBitmapSize failed\n");
        ok(cx == GetSystemMetrics(SM_CXSMICON), "cx = %d\n", cx);
        ok(cy == GetSystemMetrics(SM_CYSMICON), "cy = %d\n", cy);

        hIconSm2Again = (HICON)SendMessageW(hWnd, WM_GETICON, ICON_SMALL2, 0);
        ok(hIconSm2Again == hIconSm2, "ICON_SMALL2 is not cached, %p != %p\n",
           hIconSm2Again, hIconSm2);
    }

    /* An explicit ICON_SMALL is what ICON_SMALL2 has to return */
    SendMessageW(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
    ok((HICON)SendMessageW(hWnd, WM_GETICON, ICON_SMALL, 0) == hIconSmall,
       "ICON_SMALL was not set\n");
    ok((HICON)SendMessageW(hWnd, WM_GETICON, ICON_SMALL2, 0) == hIconSmall,
       "ICON_SMALL2 must return ICON_SMALL\n");

    /* Each ICON_BIG invalidates the derived icon, window destruction releases it.
       The first one is set up separately so that the loop below starts and ends
       with exactly one derived icon */
    SendMessageW(hWnd, WM_SETICON, ICON_SMALL, 0);
    SendMessageW(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
    hIconSm2 = (HICON)SendMessageW(hWnd, WM_GETICON, ICON_SMALL2, 0);
    ok(hIconSm2 != NULL, "Expected an ICON_SMALL2 before the loop\n");

    dwUsersBefore = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
    for (i = 0; i < 5; i++)
    {
        SendMessageW(hWnd, WM_SETICON, ICON_SMALL, 0);
        SendMessageW(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
        hIconSm2 = (HICON)SendMessageW(hWnd, WM_GETICON, ICON_SMALL2, 0);
        ok(hIconSm2 != NULL, "Expected an ICON_SMALL2 in iteration %d\n", i);
    }
    dwUsersAfter = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
    ok(dwUsersAfter == dwUsersBefore,
       "ICON_SMALL2 leaks user objects: %lu -> %lu\n",
       dwUsersBefore, dwUsersAfter);
}

/* An icon that does not come from a resource still yields a copy to own */
static void
TestPlainWindowIcon(HWND hWnd, HICON hIconBig)
{
    HICON hIconSm2;

    ok(SendMessageW(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig) == 0,
       "Expected no previous ICON_BIG\n");
    hIconSm2 = (HICON)SendMessageW(hWnd, WM_GETICON, ICON_SMALL2, 0);
    ok(hIconSm2 != NULL, "Expected an ICON_SMALL2\n");
    ok(hIconSm2 != hIconBig, "ICON_SMALL2 must not be ICON_BIG\n");
    ok((HICON)SendMessageW(hWnd, WM_GETICON, ICON_SMALL2, 0) == hIconSm2,
       "ICON_SMALL2 is not cached\n");
}

START_TEST(GetWindowIcon)
{
    WNDCLASSEXW wc;
    HINSTANCE hInstance;
    HWND hWnd;
    HICON hIconBig, hIconSmall, hIconPlain;
    DWORD dwUsersBefore, dwUsersAfter;

    hInstance = GetModuleHandleW(NULL);
    hIconBig = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_TEST), IMAGE_ICON, 32, 32, LR_SHARED);
    ok(hIconBig != NULL, "LoadImageW failed\n");
    hIconSmall = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_TEST), IMAGE_ICON,
                                  GetSystemMetrics(SM_CXSMICON),
                                  GetSystemMetrics(SM_CYSMICON), 0);
    ok(hIconSmall != NULL, "LoadImageW failed\n");
    hIconPlain = CreateIconOfSize(32, 32);
    ok(hIconPlain != NULL, "CreateIconIndirect failed\n");
    if (!hIconBig || !hIconSmall || !hIconPlain)
        return;

    /* A class with a large icon only */
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = 0;
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = hInstance;
    wc.hIcon = hIconBig;
    wc.lpszClassName = TEST_CLASS_NAME;
    ok(RegisterClassExW(&wc) != 0, "RegisterClassExW failed\n");

    hWnd = CreateTestWindow(TEST_CLASS_NAME, hInstance);
    ok(hWnd != NULL, "CreateWindowExW failed\n");
    if (hWnd)
    {
        TestClassIconIsNotUsed(hWnd);
        TestResourceWindowIcons(hWnd, hIconBig, hIconSmall);
        ok(DestroyWindow(hWnd) != 0, "DestroyWindow failed\n");
    }

    hWnd = CreateTestWindow(TEST_CLASS_NAME, hInstance);
    ok(hWnd != NULL, "CreateWindowExW failed\n");
    if (hWnd)
    {
        TestPlainWindowIcon(hWnd, hIconPlain);
        ok(DestroyWindow(hWnd) != 0, "DestroyWindow failed\n");
    }

    /* Recreating the window must not accumulate user objects: the derived icon
       is released along with the window */
    dwUsersBefore = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
    hWnd = CreateTestWindow(TEST_CLASS_NAME, hInstance);
    ok(hWnd != NULL, "CreateWindowExW failed\n");
    if (hWnd)
    {
        TestPlainWindowIcon(hWnd, hIconPlain);
        ok(DestroyWindow(hWnd) != 0, "DestroyWindow failed\n");
    }
    dwUsersAfter = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
    ok(dwUsersAfter == dwUsersBefore,
       "Window with a derived icon leaks user objects: %lu -> %lu\n",
       dwUsersBefore, dwUsersAfter);

    /* A class with both a large and a small icon */
    wc.hIconSm = hIconSmall;
    wc.lpszClassName = TEST_CLASS_SM_NAME;
    ok(RegisterClassExW(&wc) != 0, "RegisterClassExW failed\n");

    hWnd = CreateTestWindow(TEST_CLASS_SM_NAME, hInstance);
    ok(hWnd != NULL, "CreateWindowExW failed\n");
    if (hWnd)
    {
        TestClassIconIsNotUsed(hWnd);
        ok(DestroyWindow(hWnd) != 0, "DestroyWindow failed\n");
    }

    UnregisterClassW(TEST_CLASS_NAME, hInstance);
    UnregisterClassW(TEST_CLASS_SM_NAME, hInstance);

    DestroyIcon(hIconSmall);
    DestroyIcon(hIconPlain);
}
