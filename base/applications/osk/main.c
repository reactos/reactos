/*
 * PROJECT:         ReactOS On-Screen Keyboard
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         On-screen keyboard.
 * COPYRIGHT:       Denis ROBERT
 *                  Copyright 2019-2020 George Bișoc (george.bisoc@reactos.org)
 *                  Baruch Rutman (peterooch at gmail dot com)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

/* GLOBALS ********************************************************************/

OSK_GLOBALS Globals;

OSK_KEYLEDINDICATOR LedKey[] =
{
    {VK_NUMLOCK, IDC_LED_NUM, 0x0145, FALSE},
    {VK_CAPITAL, IDC_LED_CAPS, 0x013A, FALSE},
    {VK_SCROLL, IDC_LED_SCROLL, 0x0146, FALSE}
};

/* FUNCTIONS ******************************************************************/

/***********************************************************************
 *
 *           OSK_SetImage
 *
 *  Set an image on a button
 */
int OSK_SetImage(int IdDlgItem, int IdResource)
{
    HICON hIcon;
    HWND hWndItem;

    hIcon = (HICON)LoadImageW(Globals.hInstance, MAKEINTRESOURCEW(IdResource),
                              IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    if (hIcon == NULL)
        return FALSE;

    hWndItem = GetDlgItem(Globals.hMainWnd, IdDlgItem);
    if (hWndItem == NULL)
    {
        DestroyIcon(hIcon);
        return FALSE;
    }

    SendMessageW(hWndItem, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hIcon);

    /* The system automatically deletes these resources when the process that created them terminates (MSDN) */

    return TRUE;
}

/***********************************************************************
 *
 *           OSK_SetText
 *
 *  Update the text of a button according to the relevant language resource
 */
void OSK_SetText(int IdDlgItem, int IdResource)
{
    WCHAR szText[MAX_PATH];
    HWND hWndItem;

    hWndItem = GetDlgItem(Globals.hMainWnd, IdDlgItem);

    if (hWndItem == NULL)
        return;

    LoadStringW(Globals.hInstance, IdResource, szText, _countof(szText));

    SetWindowTextW(hWndItem, szText);
}
/***********************************************************************
 *
 *          OSK_WarningProc
 *
 *  Function handler for the warning dialog box on startup
 */
INT_PTR CALLBACK OSK_WarningProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (Msg)
    {
        case WM_INITDIALOG:
        {
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_SHOWWARNINGCHECK:
                {
                    Globals.bShowWarning = !IsDlgButtonChecked(hDlg, IDC_SHOWWARNINGCHECK);
                    return TRUE;
                }

                case IDOK:
                case IDCANCEL:
                {
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}

/***********************************************************************
 *
 *          OSK_WarningDlgThread
 *
 *  Thread procedure routine for the warning dialog box
 */
DWORD WINAPI OSK_WarningDlgThread(LPVOID lpParameter)
{
    HINSTANCE hInstance = (HINSTANCE)lpParameter;

    DialogBoxW(hInstance, MAKEINTRESOURCEW(IDD_WARNINGDIALOG_OSK), Globals.hMainWnd, OSK_WarningProc);
    return 0;
}

/***********************************************************************
 *
 *          OSK_About
 *
 *  Initializes the "About" dialog box
 */
VOID OSK_About(VOID)
{
    WCHAR szAuthors[MAX_PATH];
    HICON OSKIcon;

    /* Load the icon */
    OSKIcon = LoadImageW(Globals.hInstance, MAKEINTRESOURCEW(IDI_OSK), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);

    /* Load the strings into the "About" dialog */
    LoadStringW(Globals.hInstance, IDS_AUTHORS, szAuthors, _countof(szAuthors));

    /* Finally, execute the "About" dialog by using the Shell routine */
    ShellAboutW(Globals.hMainWnd, Globals.szTitle, szAuthors, OSKIcon);

    /* Once done, destroy the icon */
    DestroyIcon(OSKIcon);
}

/***********************************************************************
 *
 *           OSK_DestroyKeys
 *
 *  Used in layout change or in shutdown
 */
VOID OSK_DestroyKeys(VOID)
{
    int i;
    /* Hide before destroying child controls */
    ShowWindow(Globals.hMainWnd, SW_HIDE);

    for (i = 0; i < Globals.Keyboard->KeyCount; i++)
    {
        DestroyWindow(Globals.hKeys[i]);
    }
    for (i = 0; i < _countof(LedKey); i++)
    {
        DestroyWindow(GetDlgItem(Globals.hMainWnd, LedKey[i].DlgResource));
    }

    HeapFree(GetProcessHeap(), 0, Globals.hKeys);
    Globals.hKeys = NULL;
    Globals.Keyboard = NULL;
}

/***********************************************************************
 *
 *           OSK_SetKeys
 *
 *  Create/Update button controls with the relevant keyboard values
 */
LRESULT OSK_SetKeys(int reason)
{
    WCHAR wKey[2];
    BYTE bKeyStates[256];
    LPCWSTR szKey;
    PKEY Keys;
    UINT uVirtKey;
    POINT LedPos;
    SIZE LedSize;
    int i, yPad;

    /* Get key states before doing anything */
    if (!GetKeyboardState(bKeyStates))
    {
        DPRINT("OSK_SetKeys(): GetKeyboardState() call failed.\n");
        return -1;
    }

    switch (reason)
    {
        case SETKEYS_LANG:
        {
            /* Keyboard language/caps change, just update the button texts */
            Keys = Globals.Keyboard->Keys;
            for (i = 0; i < Globals.Keyboard->KeyCount; i++)
            {
                if (!Keys[i].translate)
                    continue;

                uVirtKey = MapVirtualKeyW(Keys[i].scancode & SCANCODE_MASK, MAPVK_VSC_TO_VK);

                if (ToUnicode(uVirtKey, Keys[i].scancode & SCANCODE_MASK, bKeyStates, wKey, _countof(wKey), 0) >= 1)
                {
                    szKey = wKey;
                }
                else
                {
                    szKey = Keys[i].name;
                }

                /* Only one & the button will try to underline the next character... */
                if (wcsncmp(szKey, L"&", 1) == 0)
                    szKey = L"&&";

                SetWindowTextW(Globals.hKeys[i], szKey);
            }
            return 0;
        }
        case SETKEYS_LAYOUT:
        {
            /* Clear up current layout before applying a different one */
            OSK_DestroyKeys();
        }
        /* Fallthrough */
        case SETKEYS_INIT:
        {             
            if (Globals.bIsEnhancedKeyboard)
            {
                Globals.Keyboard = &EnhancedKeyboard;
            }
            else
            {
                Globals.Keyboard = &StandardKeyboard;
            }

            Globals.hKeys = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HWND) * Globals.Keyboard->KeyCount);

            if (!Globals.hKeys)
            {
                DPRINT("OSK_SetKeys(): Failed to allocate memory for button handles.\n");
                return -1;
            }

            Keys = Globals.Keyboard->Keys;

            /* Create key buttons */
            for (i = 0; i < Globals.Keyboard->KeyCount; i++)
            {
                uVirtKey = MapVirtualKeyW(Keys[i].scancode & SCANCODE_MASK, MAPVK_VSC_TO_VK);

                if (Keys[i].translate && ToUnicode(uVirtKey, Keys[i].scancode & SCANCODE_MASK, bKeyStates, wKey, _countof(wKey), 0) >= 1)
                {
                    szKey = wKey;
                }
                else
                {
                    szKey = Keys[i].name;
                }
                
                Globals.hKeys[i] = CreateWindowW(WC_BUTTONW,
                                                 szKey,
                                                 WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | Keys[i].flags,
                                                 Keys[i].x,
                                                 Keys[i].y,
                                                 Keys[i].cx,
                                                 Keys[i].cy,
                                                 Globals.hMainWnd,
                                                 (HMENU)Keys[i].scancode,
                                                 Globals.hInstance,
                                                 NULL);
                if (Globals.hFont)
                    SendMessageW(Globals.hKeys[i], WM_SETFONT, (WPARAM)Globals.hFont, 0);
            }

            /* Add additional padding for caption and menu */
            yPad = GetSystemMetrics(SM_CYSIZE) + GetSystemMetrics(SM_CYMENU);
            /* Size window according to layout */
            SetWindowPos(Globals.hMainWnd,
                         (Globals.bAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST),
                         0,
                         0,
                         Globals.Keyboard->Size.cx,
                         Globals.Keyboard->Size.cy + yPad,
                         SWP_NOMOVE);
            
            /* Create LEDs */
            LedPos  = Globals.Keyboard->LedStart;
            LedSize = Globals.Keyboard->LedSize;

            CreateWindowW(WC_STATICW, L"", WS_VISIBLE | WS_CHILD | SS_CENTER | SS_NOTIFY,
                LedPos.x, LedPos.y, LedSize.cx, LedSize.cy, Globals.hMainWnd,
                (HMENU)IDC_LED_NUM, Globals.hInstance, NULL);
            
            LedPos.x += Globals.Keyboard->LedGap;

            CreateWindowW(WC_STATICW, L"", WS_VISIBLE | WS_CHILD | SS_CENTER | SS_NOTIFY,
                LedPos.x, LedPos.y, LedSize.cx, LedSize.cy, Globals.hMainWnd,
                (HMENU)IDC_LED_CAPS, Globals.hInstance, NULL);

            LedPos.x += Globals.Keyboard->LedGap;

            CreateWindowW(WC_STATICW, L"", WS_VISIBLE | WS_CHILD | SS_CENTER | SS_NOTIFY,
                LedPos.x, LedPos.y, LedSize.cx, LedSize.cy, Globals.hMainWnd,
                (HMENU)IDC_LED_SCROLL, Globals.hInstance, NULL);

            /* Set system keys text */
            OSK_SetText(SCAN_CODE_110, IDS_ESCAPE);
            OSK_SetText(SCAN_CODE_124, IDS_PRN);
            OSK_SetText(SCAN_CODE_125, IDS_STOP);
            OSK_SetText(SCAN_CODE_126, IDS_ATTN);
            OSK_SetText(SCAN_CODE_90, IDS_NUMLOCKKEY);
            OSK_SetText(SCAN_CODE_75, IDS_INSERT);
            OSK_SetText(SCAN_CODE_76, IDS_DELETE);
            OSK_SetText(SCAN_CODE_81, IDS_END);
            OSK_SetText(SCAN_CODE_58, IDS_CTRL);     /* Left ctrl */
            OSK_SetText(SCAN_CODE_64, IDS_CTRL);     /* Right ctrl */
            OSK_SetText(SCAN_CODE_60, IDS_LEFTALT);
            OSK_SetText(SCAN_CODE_62, IDS_RIGHTALT);

            /* Set icon on visual buttons */
            OSK_SetImage(SCAN_CODE_15, IDI_BACK);
            OSK_SetImage(SCAN_CODE_16, IDI_TAB);
            OSK_SetImage(SCAN_CODE_30, IDI_CAPS_LOCK);
            OSK_SetImage(SCAN_CODE_43, IDI_RETURN);
            OSK_SetImage(SCAN_CODE_44, IDI_SHIFT);
            OSK_SetImage(SCAN_CODE_57, IDI_SHIFT);
            OSK_SetImage(SCAN_CODE_127, IDI_REACTOS);
            OSK_SetImage(SCAN_CODE_128, IDI_REACTOS);
            OSK_SetImage(SCAN_CODE_129, IDI_MENU);
            OSK_SetImage(SCAN_CODE_80, IDI_HOME);
            OSK_SetImage(SCAN_CODE_85, IDI_PG_UP);
            OSK_SetImage(SCAN_CODE_86, IDI_PG_DOWN);
            OSK_SetImage(SCAN_CODE_79, IDI_LEFT);
            OSK_SetImage(SCAN_CODE_83, IDI_TOP);
            OSK_SetImage(SCAN_CODE_84, IDI_BOTTOM);
            OSK_SetImage(SCAN_CODE_89, IDI_RIGHT);
        }
    }

    if (reason != SETKEYS_INIT)
    {
        ShowWindow(Globals.hMainWnd, SW_SHOW);
        UpdateWindow(Globals.hMainWnd);
    }

    return 0;
}

/***********************************************************************
 *
 *           OSK_Create
 *
 *  Handling of WM_CREATE
 */
LRESULT OSK_Create(HWND hwnd)
{
    HMONITOR monitor;
    MONITORINFO info;
    POINT Pt;
    RECT rcWindow, rcDlgIntersect;
    LOGFONTW lf = {0};

    /* Save handle */
    Globals.hMainWnd = hwnd;

    /* Init Font */
    lf.lfHeight = Globals.FontHeight;
    StringCchCopyW(lf.lfFaceName, _countof(Globals.FontFaceName), Globals.FontFaceName);
    Globals.hFont = CreateFontIndirectW(&lf);

    if (OSK_SetKeys(SETKEYS_INIT) == -1)
        return -1;

    /* Check the checked menu item before displaying the window */
    if (Globals.bIsEnhancedKeyboard)
    {
        /* Enhanced keyboard dialog chosen, set the respective menu item as checked */
        CheckMenuItem(GetMenu(hwnd), IDM_ENHANCED_KB, MF_BYCOMMAND | MF_CHECKED);
        CheckMenuItem(GetMenu(hwnd), IDM_STANDARD_KB, MF_BYCOMMAND | MF_UNCHECKED);
    }
    else
    {
        /* Standard keyboard dialog chosen, set the respective menu item as checked */
        CheckMenuItem(GetMenu(hwnd), IDM_STANDARD_KB, MF_BYCOMMAND | MF_CHECKED);
        CheckMenuItem(GetMenu(hwnd), IDM_ENHANCED_KB, MF_BYCOMMAND | MF_UNCHECKED);
    }

    /* Check if the "Click Sound" option was chosen before (and if so, then tick the menu item) */
    if (Globals.bSoundClick)
    {
        CheckMenuItem(GetMenu(hwnd), IDM_CLICK_SOUND, MF_BYCOMMAND | MF_CHECKED);
    }

    /* Get screen info */
    memset(&Pt, 0, sizeof(Pt));
    monitor = MonitorFromPoint(Pt, MONITOR_DEFAULTTOPRIMARY);
    info.cbSize = sizeof(info);
    GetMonitorInfoW(monitor, &info);
    GetWindowRect(hwnd, &rcWindow);

    /*
        If the coordination values are default then re-initialize using the specific formulas
        to move the dialog at the bottom of the screen.
    */
    if (Globals.PosX == CW_USEDEFAULT && Globals.PosY == CW_USEDEFAULT)
    {
        Globals.PosX = (info.rcMonitor.left + info.rcMonitor.right - (rcWindow.right - rcWindow.left)) / 2;
        Globals.PosY = info.rcMonitor.bottom - (rcWindow.bottom - rcWindow.top);
    }

    /*
        Calculate the intersection of two rectangle sources (dialog and work desktop area).
        If such sources do not intersect, then the dialog is deemed as "off screen".
    */
    if (IntersectRect(&rcDlgIntersect, &rcWindow, &info.rcWork) == 0)
    {
        Globals.PosX = (info.rcMonitor.left + info.rcMonitor.right - (rcWindow.right - rcWindow.left)) / 2;
        Globals.PosY = info.rcMonitor.bottom - (rcWindow.bottom - rcWindow.top);
    }
    else
    {
        /*
            There's still some intersection but we're not for sure if it is sufficient (the dialog could also be partially hidden).
            Therefore, check the remaining intersection if it's enough.
        */
        if (rcWindow.top < info.rcWork.top || rcWindow.left < info.rcWork.left || rcWindow.right > info.rcWork.right || rcWindow.bottom > info.rcWork.bottom)
        {
            Globals.PosX = (info.rcMonitor.left + info.rcMonitor.right - (rcWindow.right - rcWindow.left)) / 2;
            Globals.PosY = info.rcMonitor.bottom - (rcWindow.bottom - rcWindow.top);
        }
    }

    /*
        Place the window (with respective placement coordinates) as topmost, above
        every window which are not on top or are at the bottom of the Z order.
    */
    if (Globals.bAlwaysOnTop)
    {
        CheckMenuItem(GetMenu(hwnd), IDM_ON_TOP, MF_BYCOMMAND | MF_CHECKED);
        SetWindowPos(hwnd, HWND_TOPMOST, Globals.PosX, Globals.PosY, 0, 0, SWP_NOSIZE);
    }
    else
    {
        CheckMenuItem(GetMenu(hwnd), IDM_ON_TOP, MF_BYCOMMAND | MF_UNCHECKED);
        SetWindowPos(hwnd, HWND_NOTOPMOST, Globals.PosX, Globals.PosY, 0, 0, SWP_NOSIZE);
    }

    /* Create a green brush for leds */
    Globals.hBrushGreenLed = CreateSolidBrush(RGB(0, 255, 0));

    /* Set a timer for periodic tasks */
    Globals.iTimer = SetTimer(hwnd, 0, 100, NULL);

    /* If the member of the struct (bShowWarning) is set then display the dialog box */
    if (Globals.bShowWarning)
    {
        /* If for whatever reason the thread fails to be created then handle the dialog box in main thread... */
        if (CreateThread(NULL, 0, OSK_WarningDlgThread, (PVOID)Globals.hInstance, 0, NULL) == NULL)
        {
            DialogBoxW(Globals.hInstance, MAKEINTRESOURCEW(IDD_WARNINGDIALOG_OSK), Globals.hMainWnd, OSK_WarningProc);
        }
    }

    return 0;
}

/***********************************************************************
 *
 *           OSK_Close
 *
 *  Handling of WM_CLOSE
 */
int OSK_Close(void)
{
    KillTimer(Globals.hMainWnd, Globals.iTimer);

    /* Release Ctrl, Shift, Alt keys */
    OSK_ReleaseKey(SCAN_CODE_44); // Left shift
    OSK_ReleaseKey(SCAN_CODE_57); // Right shift
    OSK_ReleaseKey(SCAN_CODE_58); // Left ctrl
    OSK_ReleaseKey(SCAN_CODE_60); // Left alt
    OSK_ReleaseKey(SCAN_CODE_62); // Right alt
    OSK_ReleaseKey(SCAN_CODE_64); // Right ctrl

    /* Destroy child controls */
    OSK_DestroyKeys();

    /* delete GDI objects */
    if (Globals.hBrushGreenLed) DeleteObject(Globals.hBrushGreenLed);
    if (Globals.hFont) DeleteObject(Globals.hFont);

    /* Save the application's settings on registry */
    SaveSettings();

    return TRUE;
}

/***********************************************************************
 *
 *           OSK_RefreshLEDKeys
 *
 *  Updates (invalidates) the LED icon resources then the respective
 *  keys (Caps Lock, Scroll Lock or Num Lock) are being held down
 */
VOID OSK_RefreshLEDKeys(VOID)
{
    INT i;
    BOOL bKeyIsPressed;

    for (i = 0; i < _countof(LedKey); i++)
    {
        bKeyIsPressed = (GetAsyncKeyState(LedKey[i].vKey) & 0x8000) != 0;
        if (LedKey[i].bWasKeyPressed != bKeyIsPressed)
        {
            LedKey[i].bWasKeyPressed = bKeyIsPressed;
            InvalidateRect(GetDlgItem(Globals.hMainWnd, LedKey[i].DlgResource), NULL, FALSE);
        }
    }
}

/***********************************************************************
 *
 *           OSK_Timer
 *
 *  Handling of WM_TIMER
 */
int OSK_Timer(void)
{
    HWND hWndActiveWindow;
    DWORD dwThread;
    HKL hKeyboardLayout;

    hWndActiveWindow = GetForegroundWindow();
    if (hWndActiveWindow != NULL && hWndActiveWindow != Globals.hMainWnd)
    {
        /* FIXME: To be deleted when ReactOS will support WS_EX_NOACTIVATE */
        Globals.hActiveWnd = hWndActiveWindow;

        /* Grab the current keyboard layout from the foreground window */
        dwThread = GetWindowThreadProcessId(hWndActiveWindow, NULL);
        hKeyboardLayout = GetKeyboardLayout(dwThread);
        /* Activate the layout */
        ActivateKeyboardLayout(hKeyboardLayout, 0);
    }

    /*
        Update the LED key indicators accordingly to their state (if one
        of the specific keys is held down).
    */
    OSK_RefreshLEDKeys();
    /* Update the buttons */
    OSK_SetKeys(SETKEYS_LANG);

    return TRUE;
}

/***********************************************************************
 *
 *           OSK_ChooseFont
 *
 *  Change the font of which the keys are being displayed
 */
VOID OSK_ChooseFont(VOID)
{
    LOGFONTW lf = {0};
    CHOOSEFONTW cf = {0};
    HFONT hFont, hOldFont;
    int i;

    StringCchCopyW(lf.lfFaceName, _countof(Globals.FontFaceName), Globals.FontFaceName);
    lf.lfHeight = Globals.FontHeight;

    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = Globals.hMainWnd;
    cf.lpLogFont = &lf;
    cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_NOSTYLESEL;

    if (!ChooseFontW(&cf))
        return;

    hFont = CreateFontIndirectW(&lf);

    if (!hFont)
        return;

    /* Set font information */
    StringCchCopyW(Globals.FontFaceName, _countof(Globals.FontFaceName), lf.lfFaceName);
    Globals.FontHeight = lf.lfHeight;

    hOldFont = Globals.hFont;
    Globals.hFont = hFont;

    for (i = 0; i < Globals.Keyboard->KeyCount; i++)
        SendMessageW(Globals.hKeys[i], WM_SETFONT, (WPARAM)Globals.hFont, TRUE);

    DeleteObject(hOldFont);
}

/***********************************************************************
 *
 *           OSK_Command
 *
 *  All handling of commands
 */
BOOL OSK_Command(WPARAM wCommand, HWND hWndControl)
{
    WORD ScanCode;
    INPUT Input;
    BOOL bExtendedKey;
    BOOL bKeyDown;
    BOOL bKeyUp;
    LONG WindowStyle;
    INT i;

    /* FIXME: To be deleted when ReactOS will support WS_EX_NOACTIVATE */
    if (Globals.hActiveWnd)
    {
        MSG msg;

        SetForegroundWindow(Globals.hActiveWnd);
        while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    /* KeyDown and/or KeyUp ? */
    WindowStyle = GetWindowLongW(hWndControl, GWL_STYLE);
    if ((WindowStyle & BS_AUTOCHECKBOX) == BS_AUTOCHECKBOX)
    {
        /* 2-states key like Shift, Alt, Ctrl, ... */
        if (SendMessageW(hWndControl, BM_GETCHECK, 0, 0) == BST_CHECKED)
        {
            bKeyDown = TRUE;
            bKeyUp = FALSE;
        }
        else
        {
            bKeyDown = FALSE;
            bKeyUp = TRUE;
        }
    }
    else
    {
        /* Other key */
        bKeyDown = TRUE;
        bKeyUp = TRUE;
    }

    /* Get the key from dialog control key command */
    ScanCode = wCommand;

    /*
        The user could've pushed one of the key buttons of the dialog that
        can trigger particular function toggling (Caps Lock, Num Lock or Scroll Lock). Update
        (invalidate) the LED icon resources accordingly.
    */
    for (i = 0; i < _countof(LedKey); i++)
    {
        if (LedKey[i].wScanCode == ScanCode)
        {
            InvalidateRect(GetDlgItem(Globals.hMainWnd, LedKey[i].DlgResource), NULL, FALSE);
        }
    }

    /* Extended key ? */
    if (ScanCode & 0x0200)
        bExtendedKey = TRUE;
    else
        bExtendedKey = FALSE;
    ScanCode &= SCANCODE_MASK;

    /* Press and release the key */
    if (bKeyDown)
    {
        Input.type = INPUT_KEYBOARD;
        Input.ki.wVk = 0;
        Input.ki.wScan = ScanCode;
        Input.ki.time = GetTickCount();
        Input.ki.dwExtraInfo = GetMessageExtraInfo();
        Input.ki.dwFlags = KEYEVENTF_SCANCODE;
        if (bExtendedKey) Input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        SendInput(1, &Input, sizeof(Input));
    }

    if (bKeyUp)
    {
        Input.type = INPUT_KEYBOARD;
        Input.ki.wVk = 0;
        Input.ki.wScan = ScanCode;
        Input.ki.time = GetTickCount();
        Input.ki.dwExtraInfo = GetMessageExtraInfo();
        Input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
        if (bExtendedKey) Input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        SendInput(1, &Input, sizeof(Input));
    }

    /* Play the sound during clicking event (only if "Use Click Sound" menu option is ticked) */
    if (Globals.bSoundClick)
    {
        PlaySoundW(MAKEINTRESOURCEW(IDI_SOUNDCLICK), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
    }

    return TRUE;
}

/***********************************************************************
 *
 *           OSK_ReleaseKey
 *
 *  Release the key of ID wCommand
 */
BOOL OSK_ReleaseKey(WORD ScanCode)
{
    INPUT Input;
    BOOL bExtendedKey;
    LONG WindowStyle;
    HWND hWndControl;

    /* Is it a 2-states key ? */
    hWndControl = GetDlgItem(Globals.hMainWnd, ScanCode);
    WindowStyle = GetWindowLongW(hWndControl, GWL_STYLE);
    if ((WindowStyle & BS_AUTOCHECKBOX) != BS_AUTOCHECKBOX) return FALSE;

    /* Is the key down ? */
    if (SendMessageW(hWndControl, BM_GETCHECK, 0, 0) != BST_CHECKED) return TRUE;

    /* Extended key ? */
    if (ScanCode & 0x0200)
        bExtendedKey = TRUE;
    else
        bExtendedKey = FALSE;
    ScanCode &= SCANCODE_MASK;

    /* Release the key */
    Input.type = INPUT_KEYBOARD;
    Input.ki.wVk = 0;
    Input.ki.wScan = ScanCode;
    Input.ki.time = GetTickCount();
    Input.ki.dwExtraInfo = GetMessageExtraInfo();
    Input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    if (bExtendedKey) Input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    SendInput(1, &Input, sizeof(Input));

    return TRUE;
}

/***********************************************************************
 *
 *           OSK_Paint
 *
 *  Handles WM_PAINT messages
 */
LRESULT OSK_Paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    RECT rcText;
    HFONT hOldFont = NULL;
    WCHAR szTemp[MAX_PATH];

    HDC hdc = BeginPaint(hwnd, &ps);

    if (Globals.hFont)
        hOldFont = SelectObject(hdc, Globals.hFont);

    rcText.left   = Globals.Keyboard->LedTextStart.x;
    rcText.top    = Globals.Keyboard->LedTextStart.y;
    rcText.right  = rcText.left + Globals.Keyboard->LedTextSize.cx;
    rcText.bottom = rcText.top + Globals.Keyboard->LedTextSize.cy;

    LoadStringW(Globals.hInstance, IDS_NUMLOCK, szTemp, _countof(szTemp));
    DrawTextW(hdc, szTemp, -1, &rcText, DT_NOCLIP);

    OffsetRect(&rcText, Globals.Keyboard->LedTextOffset, 0);
    
    LoadStringW(Globals.hInstance, IDS_CAPSLOCK, szTemp, _countof(szTemp));
    DrawTextW(hdc, szTemp, -1, &rcText, DT_NOCLIP);

    OffsetRect(&rcText, Globals.Keyboard->LedTextOffset, 0);
 
    LoadStringW(Globals.hInstance, IDS_SCROLLLOCK, szTemp, _countof(szTemp));
    DrawTextW(hdc, szTemp, -1, &rcText, DT_NOCLIP);

    if (hOldFont)
        SelectObject(hdc, hOldFont);

    EndPaint(hwnd, &ps);

    return 0;
}
/***********************************************************************
 *
 *       OSK_WndProc
 */
LRESULT APIENTRY OSK_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
            return OSK_Create(hwnd);

        case WM_PAINT:
            return OSK_Paint(hwnd);

        case WM_TIMER:
            return OSK_Timer();

        case WM_CTLCOLORSTATIC:
            if ((HWND)lParam == GetDlgItem(hwnd, IDC_LED_NUM))
            {
                if (GetKeyState(VK_NUMLOCK) & 0x0001)
                    return (LRESULT)Globals.hBrushGreenLed;
                else
                    return (LRESULT)GetStockObject(BLACK_BRUSH);
            }
            if ((HWND)lParam == GetDlgItem(hwnd, IDC_LED_CAPS))
            {
                if (GetKeyState(VK_CAPITAL) & 0x0001)
                    return (LRESULT)Globals.hBrushGreenLed;
                else
                    return (LRESULT)GetStockObject(BLACK_BRUSH);
            }
            if ((HWND)lParam == GetDlgItem(hwnd, IDC_LED_SCROLL))
            {
                if (GetKeyState(VK_SCROLL) & 0x0001)
                    return (LRESULT)Globals.hBrushGreenLed;
                else
                    return (LRESULT)GetStockObject(BLACK_BRUSH);
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDM_EXIT:
                {
                    PostMessageW(hwnd, WM_CLOSE, 0, 0);
                    break;
                }

                case IDM_ENHANCED_KB:
                {
                    if (!Globals.bIsEnhancedKeyboard)
                    {
                        /*
                            The user attempted to switch to enhanced keyboard dialog type.
                            Set the member value as TRUE, destroy the dialog and save the data configuration into the registry.
                        */
                        Globals.bIsEnhancedKeyboard = TRUE;
                        SaveSettings();

                        /* Change the condition of enhanced keyboard item menu to checked */
                        CheckMenuItem(GetMenu(hwnd), IDM_ENHANCED_KB, MF_BYCOMMAND | MF_CHECKED);
                        CheckMenuItem(GetMenu(hwnd), IDM_STANDARD_KB, MF_BYCOMMAND | MF_UNCHECKED);

                        /* Finally, update the key layout */
                        LoadSettings();
                        OSK_SetKeys(SETKEYS_LAYOUT);
                    }

                    break;
                }

                case IDM_STANDARD_KB:
                {
                    if (Globals.bIsEnhancedKeyboard)
                    {
                        /*
                            The user attempted to switch to standard keyboard dialog type.
                            Set the member value as FALSE, destroy the dialog and save the data configuration into the registry.
                        */
                        Globals.bIsEnhancedKeyboard = FALSE;
                        SaveSettings();

                        /* Change the condition of standard keyboard item menu to checked */
                        CheckMenuItem(GetMenu(hwnd), IDM_ENHANCED_KB, MF_BYCOMMAND | MF_UNCHECKED);
                        CheckMenuItem(GetMenu(hwnd), IDM_STANDARD_KB, MF_BYCOMMAND | MF_CHECKED);

                        /* Finally, update the key layout */
                        LoadSettings();
                        OSK_SetKeys(SETKEYS_LAYOUT);
                    }

                    break;
                }

                case IDM_CLICK_SOUND:
                {
                    /*
                        This case is triggered when the user attempts to click on the menu item. Before doing anything,
                        we must check the condition state of such menu item so that we can tick/untick the menu item accordingly.
                    */
                    if (!Globals.bSoundClick)
                    {
                        Globals.bSoundClick = TRUE;
                        CheckMenuItem(GetMenu(hwnd), IDM_CLICK_SOUND, MF_BYCOMMAND | MF_CHECKED);
                    }
                    else
                    {
                        Globals.bSoundClick = FALSE;
                        CheckMenuItem(GetMenu(hwnd), IDM_CLICK_SOUND, MF_BYCOMMAND | MF_UNCHECKED);
                    }

                    break;
                }

                case IDM_ON_TOP:
                {
                    /*
                        Check the condition state before disabling/enabling the menu
                        item and change the topmost order.
                    */
                    if (!Globals.bAlwaysOnTop)
                    {
                        Globals.bAlwaysOnTop = TRUE;
                        CheckMenuItem(GetMenu(hwnd), IDM_ON_TOP, MF_BYCOMMAND | MF_CHECKED);
                        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
                    }
                    else
                    {
                        Globals.bAlwaysOnTop = FALSE;
                        CheckMenuItem(GetMenu(hwnd), IDM_ON_TOP, MF_BYCOMMAND | MF_UNCHECKED);
                        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
                    }

                    break;
                }

                case IDM_FONT:
                {
                    OSK_ChooseFont();
                    break;
                }

                case IDM_ABOUT:
                {
                    OSK_About();
                    break;
                }

                default:
                    OSK_Command(wParam, (HWND)lParam);
                    break;
            }
            return 0;

        case WM_THEMECHANGED:
            /* Redraw the dialog (and its control buttons) using the new theme */
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case WM_CLOSE:
            OSK_Close();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/***********************************************************************
 *
 *       WinMain
 */
int WINAPI wWinMain(HINSTANCE hInstance,
                    HINSTANCE prev,
                    LPWSTR cmdline,
                    int show)
{
    DWORD dwError;
    HANDLE hMutex;
    INITCOMMONCONTROLSEX iccex;
    WNDCLASSEXW wc = {0};
    MSG msg;
    HWND hwnd;

    UNREFERENCED_PARAMETER(prev);
    UNREFERENCED_PARAMETER(cmdline);
    UNREFERENCED_PARAMETER(show);

    /*
        Obtain a mutex for the program. This will ensure that
        the program is launched only once.
    */
    hMutex = CreateMutexW(NULL, FALSE, L"OSKRunning");

    if (hMutex)
    {
        /* Check if there's already a mutex for the program */
        dwError = GetLastError();

        if (dwError == ERROR_ALREADY_EXISTS)
        {
            /*
                A mutex with the object name has been created previously.
                Therefore, another instance is already running.
            */
            DPRINT("wWinMain(): Failed to create a mutex! The program instance is already running.\n");
            CloseHandle(hMutex);
            return 0;
        }
    }

    /* Load the common controls */
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&iccex);

    ZeroMemory(&Globals, sizeof(Globals));
    Globals.hInstance = hInstance;

    /* Load the application's settings from the registry */
    LoadSettings();

    /* Define the window class */
    wc.cbSize        = sizeof(wc);
    wc.hInstance     = Globals.hInstance;
    wc.lpfnWndProc   = OSK_WndProc;
    wc.lpszMenuName  = MAKEINTRESOURCEW(IDR_OSK_MENU);
    wc.lpszClassName = OSK_CLASS;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    /* Set the application's icon */
    wc.hIcon = LoadImageW(Globals.hInstance, MAKEINTRESOURCEW(IDI_OSK), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE);
    wc.hIconSm = CopyImage(wc.hIcon, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_COPYFROMRESOURCE);

    if (!RegisterClassExW(&wc))
        goto quit;

    /* Load window title */
    LoadStringW(Globals.hInstance, IDS_OSK, Globals.szTitle, _countof(Globals.szTitle));

    hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_APPWINDOW | WS_EX_NOACTIVATE,
                           OSK_CLASS,
                           Globals.szTitle,
                           WS_SYSMENU | WS_MINIMIZEBOX,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           NULL,
                           NULL,
                           Globals.hInstance,
                           NULL);

    if (!hwnd)
        goto quit;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

quit:
    /* Delete the mutex */
    if (hMutex)
    {
        CloseHandle(hMutex);
    }

    return 0;
}

/* EOF */
