/* fdebug.c : Defines the entry point for the application. */

#include <tchar.h>

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <commdlg.h>
#include <process.h>

#include "resource.h"
#include "rs232.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE    hInst;                                        // current instance
TCHAR        szTitle[MAX_LOADSTRING];                    // The title bar text
TCHAR        szWindowClass[MAX_LOADSTRING];                // The title bar text
HWND        hMainWnd;                                    // The main window handle
HWND        hDisplayWnd;                                // The window to display the incoming data
HWND        hEditWnd;                                    // The edit window to get input from the user
TCHAR        strComPort[MAX_PATH] = TEXT("COM1");        // The COM port to use
TCHAR        strBaudRate[MAX_PATH] = TEXT("115200");        // The baud rate to use
TCHAR        strCaptureFileName[MAX_PATH] = TEXT("");    // The file name to capture to
BOOL        bConnected = FALSE;                            // Tells us if we are currently connected
BOOL        bCapturing = FALSE;                            // Tells us if we are currently capturing data
BOOL        bLocalEcho = FALSE;                            // Tells us if local echo is currently enabled
HANDLE        hCaptureFile;                                // Handle to the capture file
DWORD        dwThreadId = 0;                                // Thread id of RS232 communication thread

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ConnectionDialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    CaptureDialogProc(HWND, UINT, WPARAM, LPARAM);
VOID                EnableFileMenuItemByID(UINT Id, BOOL Enable);
VOID                CheckLocalEchoMenuItem(BOOL Checked);
VOID __cdecl        Rs232Thread(VOID* Parameter);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR     lpCmdLine,
                       int       nCmdShow)
{
     // TODO: Place code here.
    MSG msg;
    HACCEL hAccelTable;

    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(hPrevInstance);

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_FDEBUG, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_FDEBUG);

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(hMainWnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(wcex);

    wcex.style            = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra        = 0;
    wcex.cbWndExtra        = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FDEBUG));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = NULL;//(HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName    = MAKEINTRESOURCE(IDC_FDEBUG);
    wcex.lpszClassName    = szWindowClass;
    wcex.hIconSm        = (HICON)LoadImage(hInstance,
                                           MAKEINTRESOURCE(IDI_FDEBUG),
                                           IMAGE_ICON,
                                           16,
                                           16,
                                           LR_SHARED);

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   hMainWnd = hWnd;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int                    wmId, wmEvent;
    PAINTSTRUCT            ps;
    HDC                    hdc;
    RECT                rc;
    TCHAR                WndText[MAX_PATH];
    DWORD                Index;
    NONCLIENTMETRICS    ncm;
    HFONT                hFont;

    switch (message)
    {
    case WM_CREATE:

        hEditWnd = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT(""), WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_AUTOHSCROLL|ES_LEFT|ES_MULTILINE, 0, 0, 0, 0, hWnd, NULL, hInst, NULL);
        hDisplayWnd = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT(""), WS_CHILD|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL|ES_MULTILINE, 0, 0, 0, 0, hWnd, NULL, hInst, NULL);

        ZeroMemory(&ncm, sizeof(ncm));
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

        hFont = CreateFontIndirect(&ncm.lfMessageFont);

        SendMessage(hEditWnd, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hDisplayWnd, WM_SETFONT, (WPARAM)hFont, TRUE);

        break;
    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);

        if (lParam == (LPARAM)hEditWnd && wmEvent == EN_CHANGE)
        {
            GetWindowText(hEditWnd, WndText, MAX_PATH);

            if (_tcslen(WndText) > 0)
            {
                SetWindowText(hEditWnd, TEXT(""));

                if (!bConnected)
                {
                    MessageBox(hWnd, TEXT("You are not currently connected!"), TEXT("Error"), MB_OK|MB_ICONSTOP);
                    break;
                }

                for (Index=0; Index<_tcslen(WndText); Index++)
                {
                    if (dwThreadId != 0)
                    {
                        PostThreadMessage(dwThreadId, WM_CHAR, (WPARAM)WndText[Index], (LPARAM)0);
                    }
                }
            }
        }

        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
           DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, About);
           break;
        case IDM_EXIT:
           DestroyWindow(hWnd);
           break;
        case IDM_FILE_CLEARDISPLAY:
            SetWindowText(hDisplayWnd, TEXT(""));
            break;
        case IDM_FILE_CONNECT:
            if (bConnected)
            {
                MessageBox(hWnd, TEXT("You are already connected!"), TEXT("Error"), MB_OK|MB_ICONSTOP);
            }
            else
            {
                if (DialogBox(hInst, (LPCTSTR)IDD_CONNECTION, hWnd, ConnectionDialogProc) == IDOK)
                {
                    bConnected = TRUE;
                    EnableFileMenuItemByID(IDM_FILE_DISCONNECT, TRUE);
                    EnableFileMenuItemByID(IDM_FILE_CONNECT, FALSE);
                    _beginthread(Rs232Thread, 0, NULL);
                }
            }
            break;
        case IDM_FILE_DISCONNECT:
            if (bConnected)
            {
                bConnected = FALSE;
                EnableFileMenuItemByID(IDM_FILE_DISCONNECT, FALSE);
                EnableFileMenuItemByID(IDM_FILE_CONNECT, TRUE);
            }
            else
            {
                MessageBox(hWnd, TEXT("You are not currently connected!"), TEXT("Error"), MB_OK|MB_ICONSTOP);
            }
            break;
        case IDM_FILE_STARTCAPTURE:
            if (DialogBox(hInst, (LPCTSTR)IDD_CAPTURE, hWnd, CaptureDialogProc) == IDOK)
            {
                bCapturing = TRUE;
                EnableFileMenuItemByID(IDM_FILE_STOPCAPTURE, TRUE);
                EnableFileMenuItemByID(IDM_FILE_STARTCAPTURE, FALSE);
                hCaptureFile = CreateFile(strCaptureFileName, FILE_APPEND_DATA, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            }
            break;
        case IDM_FILE_STOPCAPTURE:
            if (bCapturing)
            {
                bCapturing = FALSE;
                EnableFileMenuItemByID(IDM_FILE_STOPCAPTURE, FALSE);
                EnableFileMenuItemByID(IDM_FILE_STARTCAPTURE, TRUE);
                CloseHandle(hCaptureFile);
                hCaptureFile = NULL;
            }
            break;
        case IDM_FILE_LOCALECHO:
            if (bLocalEcho)
            {
                bLocalEcho = FALSE;
                CheckLocalEchoMenuItem(bLocalEcho);
            }
            else
            {
                bLocalEcho = TRUE;
                CheckLocalEchoMenuItem(bLocalEcho);
            }
            break;
        default:
           return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        (void)hdc; // FIXME
        EndPaint(hWnd, &ps);
        break;
    case WM_SIZE:

        GetClientRect(hWnd, &rc);

        MoveWindow(hDisplayWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top - 20, TRUE);
        MoveWindow(hEditWnd, rc.left, rc.bottom - 20, rc.right - rc.left, 20, TRUE);

        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND    hLicenseEditWnd;
    TCHAR    strLicense[0x1000];

    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:

        hLicenseEditWnd = GetDlgItem(hDlg, IDC_LICENSE_EDIT);

        LoadString(hInst, IDS_LICENSE, strLicense, 0x1000);

        SetWindowText(hLicenseEditWnd, strLicense);

        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    return FALSE;
}

INT_PTR CALLBACK ConnectionDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:

        SetWindowText(GetDlgItem(hDlg, IDC_COMPORT), strComPort);
        SetWindowText(GetDlgItem(hDlg, IDC_BAUTRATE), strBaudRate);

        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            GetWindowText(GetDlgItem(hDlg, IDC_COMPORT), strComPort, MAX_PATH);
            GetWindowText(GetDlgItem(hDlg, IDC_BAUTRATE), strBaudRate, MAX_PATH);
        }

        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    return FALSE;
}

INT_PTR CALLBACK CaptureDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    OPENFILENAME    ofn;

    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:

        SetWindowText(GetDlgItem(hDlg, IDC_CAPTUREFILENAME), strCaptureFileName);

        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BROWSE)
        {
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hDlg;
            ofn.hInstance = hInst;
            ofn.lpstrFilter = NULL;
            ofn.lpstrCustomFilter = NULL;
            ofn.nMaxCustFilter = 0;
            ofn.nFilterIndex = 0;
            ofn.lpstrFile = strCaptureFileName;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.lpstrTitle = NULL;
            ofn.Flags = OFN_HIDEREADONLY|OFN_NOREADONLYRETURN;

            if (GetOpenFileName(&ofn))
            {
                SetWindowText(GetDlgItem(hDlg, IDC_CAPTUREFILENAME), strCaptureFileName);
            }
        }

        if (LOWORD(wParam) == IDOK)
        {
            GetWindowText(GetDlgItem(hDlg, IDC_CAPTUREFILENAME), strCaptureFileName, MAX_PATH);
        }

        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    return FALSE;
}

VOID EnableFileMenuItemByID(UINT Id, BOOL Enable)
{
    HMENU    hMenuBar;
    HMENU    hFileMenu;

    hMenuBar = GetMenu(hMainWnd);
    hFileMenu = GetSubMenu(hMenuBar, 0);
    EnableMenuItem(hFileMenu, Id, MF_BYCOMMAND|(Enable ? MF_ENABLED : MF_GRAYED));
}

VOID CheckLocalEchoMenuItem(BOOL Checked)
{
    HMENU    hMenuBar;
    HMENU    hFileMenu;

    hMenuBar = GetMenu(hMainWnd);
    hFileMenu = GetSubMenu(hMenuBar, 0);
    CheckMenuItem(hFileMenu, IDM_FILE_LOCALECHO, MF_BYCOMMAND|(Checked ? MF_CHECKED : MF_UNCHECKED));
}

VOID __cdecl Rs232Thread(VOID* Parameter)
{
    BYTE    Byte;
    TCHAR    String[MAX_PATH];
    MSG        msg;
    DWORD    dwNumberOfBytesWritten;

    UNREFERENCED_PARAMETER(Parameter);

    dwThreadId = GetCurrentThreadId();

    if (!Rs232OpenPortWin32(strComPort))
    {
        MessageBox(hMainWnd, TEXT("Error opening port!"), TEXT("Error"), MB_OK|MB_ICONSTOP);
        bConnected = FALSE;
        return;
    }

    _stprintf(String, TEXT("%s,n,8,1"), strBaudRate);
    if (!Rs232ConfigurePortWin32(String))
    {
        MessageBox(hMainWnd, TEXT("Error configuring port!"), TEXT("Error"), MB_OK|MB_ICONSTOP);
        bConnected = FALSE;
        return;
    }

    while (bConnected)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_CHAR)
            {
                Rs232WriteByteWin32((BYTE)msg.wParam);

                if (bLocalEcho && msg.wParam != (WPARAM)TEXT('\r'))
                {
                    PostMessage(hDisplayWnd, WM_CHAR, (WPARAM)msg.wParam, (LPARAM)0);

                    if (hCaptureFile)
                    {
                        WriteFile(hCaptureFile, &msg.wParam, sizeof(TCHAR), &dwNumberOfBytesWritten, NULL);
                    }
                }
            }
        }

        if (Rs232ReadByteWin32(&Byte))
        {
            _stprintf(String, TEXT("%c"), Byte);

            PostMessage(hDisplayWnd, WM_CHAR, (WPARAM)String[0], (LPARAM)0);

            if (hCaptureFile)
            {
                WriteFile(hCaptureFile, &String[0], sizeof(TCHAR), &dwNumberOfBytesWritten, NULL);
            }
        }
    }

    dwThreadId = 0;
    Rs232ClosePortWin32();
}
