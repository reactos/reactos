//#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static LPSTR STATIC_CLASS   = "STATIC";
static LPSTR TEST_WND_CLASS = "TESTWND";

#ifdef NDEBUG
 #define DPRINT(s) (void)0
#else
 #define DPRINT(s) OutputDebugStringA("STATICTEST: " s "\n")
#endif

HINSTANCE AppInstance = NULL;

LRESULT WmCreate(
   HWND Wnd)
{
   UCHAR i;
   DPRINT("WM_CREATE (enter).");
   // Test 1 - black rectangle.
   DPRINT("test 1");
   CreateWindowEx(0, STATIC_CLASS, NULL, WS_CHILD | WS_VISIBLE | SS_BLACKRECT,
      10, 10, 100, 20, Wnd, NULL, AppInstance, NULL);
   // Test 2 - black frame.
   DPRINT("test 2");
   CreateWindowEx(0, STATIC_CLASS, NULL, WS_CHILD | WS_VISIBLE | SS_BLACKFRAME,
      10, 40, 100, 20, Wnd, NULL, AppInstance, NULL);
   // Test 3 - gray rectangle.
   DPRINT("test 3");
   CreateWindowEx(0, STATIC_CLASS, NULL, WS_CHILD | WS_VISIBLE | SS_GRAYRECT,
      10, 70, 100, 20, Wnd, NULL, AppInstance, NULL);
   // Test 4 - gray frame.
   DPRINT("test 4");
   CreateWindowEx(0, STATIC_CLASS, NULL, WS_CHILD | WS_VISIBLE | SS_GRAYFRAME,
      10, 100, 100, 20, Wnd, NULL, AppInstance, NULL);
   // Test 5 - left-aligned text.
   DPRINT("test 5");
   CreateWindowEx(0, STATIC_CLASS,
      "&Left-aligned text &static control window",
      WS_CHILD | WS_VISIBLE | SS_LEFT,
      10, 130, 100, 50, Wnd, NULL, AppInstance, NULL);
   // Test 6 - right-aligned text.
   DPRINT("test 6");
   CreateWindowEx(0, STATIC_CLASS,
      "&Right-aligned text &static control window",
      WS_CHILD | WS_VISIBLE | SS_RIGHT,
      10, 185, 100, 50, Wnd, NULL, AppInstance, NULL);
   // Test 7 - centered text.
   DPRINT("test 7");
   CreateWindowEx(0, STATIC_CLASS,
      "&Centered text &static control window",
      WS_CHILD | WS_VISIBLE | SS_CENTER,
      10, 240, 100, 50, Wnd, NULL, AppInstance, NULL);
   // Test 8 - left-aligned text with no word wrap and no prefixes.
   DPRINT("test 8");
   CreateWindowEx(0, STATIC_CLASS,
      "&No prefix and no word wrapping",
      WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP | SS_NOPREFIX,
      10, 295, 100, 20, Wnd, NULL, AppInstance, NULL);
   // Test 9 - white rectangle.
   DPRINT("test 9");
   CreateWindowEx(0, STATIC_CLASS, NULL, WS_CHILD | WS_VISIBLE | SS_WHITERECT,
      120, 10, 100, 20, Wnd, NULL, AppInstance, NULL);
   // Test 10 - white frame.
   DPRINT("test 10");
   CreateWindowEx(0, STATIC_CLASS, NULL, WS_CHILD | WS_VISIBLE | SS_WHITEFRAME,
      120, 40, 100, 20, Wnd, NULL, AppInstance, NULL);
   // Test 11 - etched frame.
   DPRINT("test 11");
   CreateWindowEx(0, STATIC_CLASS, NULL, WS_CHILD | WS_VISIBLE
      | SS_ETCHEDFRAME, 120, 70, 100, 20, Wnd, NULL, AppInstance, NULL);
   // Test 12 - etched horizontal lines.
   DPRINT("test 12");
   for (i = 0; i < 5; ++i)
      CreateWindowEx(0, STATIC_CLASS, NULL, WS_CHILD | WS_VISIBLE
         | SS_ETCHEDHORZ, 120, 100 + (4L * i), 100, 4, Wnd,
         NULL, AppInstance, NULL);
   // Test 13 - etched vertical lines.
   DPRINT("test 13");
   for (i = 0; i < 25; ++i)
      CreateWindowEx(0, STATIC_CLASS, NULL, WS_CHILD | WS_VISIBLE
         | SS_ETCHEDVERT, 120 + (4L * i), 130, 4, 20, Wnd,
         NULL, AppInstance, NULL);
   // Test 14 - sunken border.
   DPRINT("test 14");
   CreateWindowEx(0, STATIC_CLASS,
      "Sunken frame and word ellipsis",
      WS_CHILD | WS_VISIBLE | SS_SUNKEN | SS_WORDELLIPSIS,
      120, 160, 100, 20, Wnd, NULL, AppInstance, NULL);
   DPRINT("WM_CREATE (leave).");
   return 0;
}

LRESULT CALLBACK TestWndProc(
   HWND Wnd,
   UINT Msg,
   WPARAM wParam,
   LPARAM lParam)
{
   switch (Msg) {
   case WM_CREATE:
      return WmCreate(Wnd);
   case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
   default:
      return DefWindowProc(Wnd, Msg, wParam, lParam);
   }
}

int STDCALL WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nShowCmd)
{
   ATOM Result;
   MSG Msg;
   HWND MainWindow;
   WNDCLASSEX TestWndClass = {0};
   DPRINT("Application starting up.");
   // Remember instance handle.
   AppInstance = GetModuleHandle(NULL);
   // Register test window class.
   TestWndClass.cbSize = sizeof(WNDCLASSEX);
   TestWndClass.lpfnWndProc = &TestWndProc;
   TestWndClass.hInstance = AppInstance;
   TestWndClass.hCursor = LoadCursor(0, IDC_ARROW);
   TestWndClass.hbrBackground = CreateSolidBrush(RGB(255,255,230));
   TestWndClass.lpszClassName = TEST_WND_CLASS;
   Result = RegisterClassEx(&TestWndClass);
   if (Result == 0) {
      DPRINT("Error registering class.");
      MessageBox(0, "Error registering test window class.",
         "Static control test", MB_ICONSTOP | MB_OK);
      ExitProcess(0);
   }
   // Create main window.
   DPRINT("Creating main window.");
   MainWindow = CreateWindowEx(WS_EX_APPWINDOW | WS_EX_CLIENTEDGE,
      TEST_WND_CLASS, "Static control test",
      WS_OVERLAPPEDWINDOW, 50, 50, 245, 365,
      NULL, NULL, AppInstance, NULL);
   if (MainWindow == 0) {
      DPRINT("Error creating main window.");
      UnregisterClass(TEST_WND_CLASS, AppInstance);
      MessageBox(0, "Error creating test window.",
         "Static control test", MB_ICONSTOP | MB_OK);
      ExitProcess(0);
   }
   DPRINT("Showing main window.");
   ShowWindow(MainWindow, SW_SHOWNORMAL);
   UpdateWindow(MainWindow);
   // Run message loop.
   DPRINT("Entering message loop.");
   while (GetMessage(&Msg, NULL, 0, 0) > 0) {
      TranslateMessage(&Msg);
      DispatchMessage(&Msg);
   }
   // Unregister window class.
   UnregisterClass(TEST_WND_CLASS, AppInstance);
   DPRINT("Exiting.");

   return Msg.wParam;
}
