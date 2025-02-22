/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for mismatch with function prototype in window procedure callback.
 * PROGRAMMERS:
 */

#include "precomp.h"

/* Used wine Redraw test for proof in principle. */

#define WMPAINT_COUNT_THRESHOLD 10

/* Global variables to trigger exit from loop */
static int redrawComplete, WMPAINT_count;

/*
 * Force stack corruption when calling from assumed window procedure callback.
 * Adding (6 and) more will force exception faults and terminate the test program.
 * The test is with five and this is safe for windows.
 *
 * But,,,, ReactOS compiled with GCC can handle this,,,,,,
 */
static LRESULT WINAPI redraw_window_procA(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam)
{
    switch (msg)
    {
        case WM_PAINT:
            break;
            WMPAINT_count++;
            trace("Doing WM_PAINT %d/%d\n", WMPAINT_count, WMPAINT_COUNT_THRESHOLD);

            if (WMPAINT_count > WMPAINT_COUNT_THRESHOLD && redrawComplete == 0)
            {
                PAINTSTRUCT ps;

                trace("Calling *Paint()\n");
                BeginPaint(hwnd, &ps);
                EndPaint(hwnd, &ps);
                return 1;
            }

            // This will force one stack corruption "ret" fault with normal window
            // procedure callback.
#ifdef __MINGW32__
            trace("Executing __MINGW32__ stack corruption code\n");
            asm ("movl $0, %eax\n\t"
                 "leave\n\t"
                 "ret");
#elif defined(_M_IX86)
//#ifdef _MSC_VER
            trace("Executing MSVC x86 stack corruption code\n");
            __asm
              {
                 mov eax, 0
                 leave
                 ret
              }
#else
            ok(FALSE, "FIXME: stack corruption code is unimplemented\n");
#endif

            break;
        default:
            trace("Doing empty default: msg = %u\n", msg);
    }

    trace("Calling DefWindowProc()\n");
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static void test_wndproc(void)
{
    WNDCLASSA cls;
    ATOM clsAtom;
    HWND hwndMain;

    cls.style = CS_DBLCLKS;
    cls.lpfnWndProc = redraw_window_procA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = NULL;
    cls.hCursor = LoadCursorA(NULL, IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "RedrawWindowClass";

    clsAtom = RegisterClassA(&cls);
    ok(clsAtom != 0, "RegisterClassA() failed: LastError = %lu\n", GetLastError());

    if (clsAtom == 0)
    {
        skip("No Class atom\n");
        return;
    }

    hwndMain = CreateWindowA(cls.lpszClassName, "Main Window", WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, 0, 100, 100, NULL, NULL, NULL, NULL);

    ok(hwndMain != NULL, "CreateWindowA() failed: LastError = %lu\n", GetLastError());

    ok(WMPAINT_count == 0,
       "Multiple unexpected WM_PAINT calls = %d\n", WMPAINT_count);

    if (hwndMain == NULL)
    {
        skip("No Window\n");
        ok(UnregisterClassA(cls.lpszClassName, cls.hInstance) != 0,
           "UnregisterClassA() failed: LastError = %lu\n", GetLastError());
        return;
    }

    ShowWindow(hwndMain, SW_SHOW);
    ok(WMPAINT_count == 0,
       "Multiple unexpected WM_PAINT calls = %d\n", WMPAINT_count);

    RedrawWindow(hwndMain, NULL, NULL, RDW_UPDATENOW | RDW_ALLCHILDREN);
    ok(WMPAINT_count == 1 || broken(WMPAINT_count == 0), /* sometimes on win9x */
       "Multiple unexpected WM_PAINT calls = %d\n", WMPAINT_count);

    redrawComplete = TRUE;
    ok(WMPAINT_count < WMPAINT_COUNT_THRESHOLD,
       "RedrawWindow (RDW_UPDATENOW) never completed (%d/%d)\n",
       WMPAINT_count, WMPAINT_COUNT_THRESHOLD);

    ok(DestroyWindow(hwndMain) != 0,
       "DestroyWindow() failed: LastError = %lu\n", GetLastError());

    ok(UnregisterClassA(cls.lpszClassName, cls.hInstance) != 0,
       "UnregisterClassA() failed: LastError = %lu\n", GetLastError());
}

static void test_get_wndproc(void)
{
    LONG_PTR ret;
    SetLastError(0xfeedf00d);
    ret = GetWindowLongPtrA(GetShellWindow(), GWLP_WNDPROC);
    ok (ret == 0, "Should return NULL\n");
    ok (GetLastError() == ERROR_ACCESS_DENIED, "Wrong return error!\n");
    SetLastError(0xfeedf00d);
    ret = GetWindowLongPtrW(GetShellWindow(), GWLP_WNDPROC);
    ok (ret == 0, "Should return NULL\n");
    ok (GetLastError() == ERROR_ACCESS_DENIED, "Wrong return error!\n");
    SetLastError(0xfeedf00d);
    ret = GetWindowLongPtrA(GetShellWindow(), GWLP_WNDPROC);
    ok (ret == 0, "Should return NULL\n");
    ok (GetLastError() == ERROR_ACCESS_DENIED, "Wrong return error!\n");
    SetLastError(0xfeedf00d);
    ret = GetWindowLongPtrW(GetShellWindow(), GWLP_WNDPROC);
    ok (ret == 0, "Should return NULL\n");
    ok (GetLastError() == ERROR_ACCESS_DENIED, "Wrong return error!\n");
}

START_TEST(WndProc)
{
#ifdef __RUNTIME_CHECKS__
    skip("This test breaks MSVC runtime checks!\n");
    return;
#endif /* __RUNTIME_CHECKS__ */

    test_get_wndproc();
    test_wndproc();
}
