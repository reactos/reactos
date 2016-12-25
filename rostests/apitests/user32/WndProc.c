/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for mismatch with function prototype in window procedure callback.
 * PROGRAMMERS:
 */

#include <apitest.h>

#include <wingdi.h>
#include <winuser.h>

/* Used wine Redraw test for proof in principle. */

/* Global variables to trigger exit from loop */
static int redrawComplete, WMPAINT_count;

/*
   Force stack corruption when calling from assumed window procedure callback.
   Adding (6 and) more will force exception faults and terminate the test program.
   The test is with five and this is safe for windows.

   But,,,, ReactOS compiled with GCC can handle this,,,,,,
 */
static LRESULT WINAPI redraw_window_procA(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, DWORD ExtraData,DWORD ExtraData1,DWORD ExtraData2,DWORD ExtraData3)
{
    switch (msg)
    {
    case WM_PAINT:
        trace("doing WM_PAINT %d\n", WMPAINT_count);
        WMPAINT_count++;
        if (WMPAINT_count > 10 && redrawComplete == 0)
        {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            return 1;
        }
   /*
       This will force one stack corruption "ret" fault with normal window
       procedure callback.
    */
#ifdef __MINGW32__
        asm ("movl $0, %eax\n\t"
             "leave\n\t"
             "ret");
#elif defined(_M_IX86)
//#ifdef _MSC_VER
        __asm
          {
             mov eax, 0
             leave
             ret
          }
#else
        trace("unimplemented\n");
#endif
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static void test_wndproc(void)
{
   WNDCLASSA cls;
   HWND hwndMain;

   cls.style = CS_DBLCLKS;
   cls.lpfnWndProc = (WNDPROC)redraw_window_procA;
   cls.cbClsExtra = 0;
   cls.cbWndExtra = 0;
   cls.hInstance = GetModuleHandleA(0);
   cls.hIcon = 0;
   cls.hCursor = LoadCursorA(0, IDC_ARROW);
   cls.hbrBackground = GetStockObject(WHITE_BRUSH);
   cls.lpszMenuName = NULL;
   cls.lpszClassName = "RedrawWindowClass";

   if (!RegisterClassA(&cls))
   {
       trace("Register failed %d\n", (int)GetLastError());
       return;
   }

   hwndMain = CreateWindowA("RedrawWindowClass", "Main Window", WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, 0, 100, 100, NULL, NULL, 0, NULL);

   ok( WMPAINT_count == 0, "Multiple unexpected WM_PAINT calls %d\n", WMPAINT_count);
   ShowWindow(hwndMain, SW_SHOW);
   ok( WMPAINT_count == 0, "Multiple unexpected WM_PAINT calls %d\n", WMPAINT_count);
   RedrawWindow(hwndMain, NULL,NULL,RDW_UPDATENOW | RDW_ALLCHILDREN);
   ok( WMPAINT_count == 1 || broken(WMPAINT_count == 0), /* sometimes on win9x */
       "Multiple unexpected WM_PAINT calls %d\n", WMPAINT_count);
   redrawComplete = TRUE;
   ok( WMPAINT_count < 10, "RedrawWindow (RDW_UPDATENOW) never completed (%d)\n", WMPAINT_count);

   /* clean up */
   DestroyWindow( hwndMain);
}

START_TEST(WndProc)
{
#ifdef __RUNTIME_CHECKS__
    skip("This test breaks MSVC runtime checks!");
    return;
#endif /* __RUNTIME_CHECKS__ */
   test_wndproc();
}
