/* Based on GvG's static control test. */
#include <windows.h>

static LPSTR BUTTON_CLASS   = "BUTTON";
static LPSTR TEST_WND_CLASS = "TESTWND";

#ifdef NDEBUG
 #define DPRINT(s) (void)0
#else
 #define DPRINT(s) OutputDebugStringA("BUTTONTEST: " s "\n")
#endif

HINSTANCE AppInstance = NULL;

LRESULT WmCreate(
   HWND Wnd)
{
   UCHAR i;
   DPRINT("WM_CREATE (enter).");
   DPRINT("test 1");
   CreateWindowEx(0, BUTTON_CLASS, "PushButton", BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE,
      10, 10, 150, 30, Wnd, NULL, AppInstance, NULL);
   DPRINT("test 2");
   CreateWindowEx(0, BUTTON_CLASS, "DefPushButton", BS_DEFPUSHBUTTON | WS_CHILD | WS_VISIBLE,
      10, 40, 150, 30, Wnd, NULL, AppInstance, NULL);
   DPRINT("test 3");
   CreateWindowEx(0, BUTTON_CLASS, "AutoRadioButton", BS_AUTORADIOBUTTON | WS_CHILD | WS_VISIBLE,
      10, 70, 150, 30, Wnd, NULL, AppInstance, NULL);
   DPRINT("test 4");
   CreateWindowEx(0, BUTTON_CLASS, "AutoCheckBox", BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
      10, 100, 150, 30, Wnd, NULL, AppInstance, NULL);

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
      TEST_WND_CLASS, "Button test",
      WS_OVERLAPPEDWINDOW, 50, 50, 180, 365,
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
