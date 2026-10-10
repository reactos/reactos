/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Tests the interaction between menus, accelerators, and Alt+Numpad keys
 * COPYRIGHT:   Copyright 2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* HEADERS *******************************************************************/

/* C Headers */
#include <stdio.h>
#include <stdlib.h>

/* PSDK Headers */
#include <windef.h>
#include <winbase.h>
#include <winuser.h>

#include "resource.h"

/* GLOBALS *******************************************************************/

static HINSTANCE g_hInst;
static const PCWSTR szWindowClass = L"TESTAPP";

static HWND g_hEdit, g_hMsgDump;
static ATOM g_aHotKey1, g_aHotKey2;
static HHOOK g_hHook1, g_hHook2, g_hHook3;
static BOOL g_bShowEdit = TRUE;
static BOOL g_bXlatAccels = TRUE;

#define EDIT_STYLE_WRAP (WS_CHILD | WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL)
#define EDIT_STYLE      (EDIT_STYLE_WRAP | WS_HSCROLL | ES_AUTOHSCROLL)

/* FUNCTIONS *****************************************************************/

LRESULT CALLBACK
HookProcCallWnd(
    _In_ int nCode,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam);

LRESULT CALLBACK
HookProcGetMsg(
    _In_ int nCode,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam);

LRESULT CALLBACK
HookProcMessage(
    _In_ int nCode,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam);

LRESULT CALLBACK
WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static ATOM
MyRegisterClass(_In_ HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    HICON hSmIcon, hBgIcon;

    hSmIcon = LoadImageW(hInstance,
                         MAKEINTRESOURCEW(IDI_TESTAPP),
                         IMAGE_ICON,
                         GetSystemMetrics(SM_CXSMICON),
                         GetSystemMetrics(SM_CYSMICON),
                         LR_SHARED);

    hBgIcon = LoadImageW(hInstance,
                         MAKEINTRESOURCEW(IDI_TESTAPP),
                         IMAGE_ICON,
                         GetSystemMetrics(SM_CXICON),
                         GetSystemMetrics(SM_CYICON),
                         LR_SHARED);

    wcex.cbSize = sizeof(wcex);
    wcex.style  = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra  = 0;
    wcex.cbWndExtra  = 0;
    wcex.hInstance   = hInstance;
    wcex.hIcon   = hBgIcon;
    wcex.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName  = MAKEINTRESOURCEW(IDC_TESTAPP);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = hSmIcon;

    return RegisterClassExW(&wcex);
}

int APIENTRY
wWinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    HWND hWnd;
    MSG msg;
    HACCEL hAccelTable;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    /* Register ourselves */
    if (!MyRegisterClass(hInstance))
        return FALSE;
    g_hInst = hInstance;

    /* Create the main window and load the accelerators */
    hWnd = CreateWindowW(szWindowClass, L"MyTestApp", WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                         NULL, NULL, hInstance, NULL);
    if (!hWnd)
        return FALSE;

    /* Show ourselves */
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    /* Set up the accelerators, hotkey, and window hooks */
    hAccelTable = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDC_TESTAPP));
    if (!hAccelTable)
        OutputDebugStringA("LoadAcceleratorsW() failed\n");

    g_aHotKey1 = GlobalAddAtomW(L"TESTAPP Alt 4 Hotkey");
    if (!RegisterHotKey(NULL, g_aHotKey1, MOD_ALT, VK_NUMPAD4))
        OutputDebugStringA("RegisterHotKey() failed\n"); //DPRINT1("RegisterHotKey failed with %lu\n", GetLastError());

    g_aHotKey2 = GlobalAddAtomW(L"TESTAPP Alt 5 Hotkey");
    if (!RegisterHotKey(hWnd, g_aHotKey2, MOD_ALT, VK_NUMPAD5))
        OutputDebugStringA("RegisterHotKey() failed\n"); //DPRINT1("RegisterHotKey failed with %lu\n", GetLastError());

    g_hHook1 = SetWindowsHookExW(WH_CALLWNDPROC, HookProcCallWnd, NULL, GetCurrentThreadId());
    g_hHook2 = SetWindowsHookExW(WH_GETMESSAGE, HookProcGetMsg, NULL, GetCurrentThreadId());
    g_hHook3 = SetWindowsHookExW(WH_MSGFILTER, HookProcMessage, NULL, GetCurrentThreadId());

    /* Pump the queue */
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if ((msg.message == WM_HOTKEY) && (msg.wParam == g_aHotKey1))
        {
            // if (LOWORD(msg.lParam) == MOD_ALT && HIWORD(msg.lParam) == VK_NUMPAD4)
            PostMessageW(hWnd, WM_COMMAND, MAKEWPARAM(IDM_FOUR, 1), 0);
            continue;
        }

        if (!g_bXlatAccels || !TranslateAcceleratorW(hWnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    /* Cleanup and return */
    UnhookWindowsHookEx(g_hHook3);
    UnhookWindowsHookEx(g_hHook2);
    UnhookWindowsHookEx(g_hHook1);

    UnregisterHotKey(NULL, g_aHotKey2);
    GlobalDeleteAtom(g_aHotKey2);
    UnregisterHotKey(NULL, g_aHotKey1);
    GlobalDeleteAtom(g_aHotKey1);

    return (int)msg.wParam;
}

static void
SetMenuString(
    _In_ HMENU hMenu,
    _In_ UINT uCommand,
    _In_ LPCWSTR pszString)
{
    WCHAR szMenu[128];

    if (IS_INTRESOURCE(pszString))
    {
        LoadStringW(g_hInst, PtrToUlong(pszString), szMenu, _countof(szMenu));
        pszString = szMenu;
    }
    ModifyMenuW(hMenu, uCommand, MF_BYCOMMAND | MF_STRING, uCommand, pszString);
}

static void
UpdateEditWnd(_In_ HWND hWndParent)
{
    /* Change the menu command string */
    SetMenuString(GetMenu(hWndParent), IDM_SHOWHIDEEDIT,
                  g_bShowEdit ? MAKEINTRESOURCEW(IDS_EDITHIDE)
                              : MAKEINTRESOURCEW(IDS_EDITSHOW));

    /* Show or hide the editable edit control and set focus to it if needed */
    ShowWindow(g_hEdit, g_bShowEdit ? SW_SHOW : SW_HIDE);
    if (g_bShowEdit)
        SetFocus(g_hEdit);

    /* Re-arrange controls */
    PostMessageW(hWndParent, WM_SIZE, 0, 0);
}

static void
UpdateAccelXlatMenu(_In_ HWND hWnd)
{
    /* Change the menu command string */
    SetMenuString(GetMenu(hWnd), IDM_ACCELXLAT,
                  g_bXlatAccels ? MAKEINTRESOURCEW(IDS_ACCELNOXLAT)
                                : MAKEINTRESOURCEW(IDS_ACCELXLAT));
}

static void
DoCreateEditChildren(_In_ HWND hWndParent)
{
    g_hEdit = CreateWindowW(/*WC_EDITW*/ L"Edit", L"", EDIT_STYLE_WRAP,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                            hWndParent, NULL, g_hInst, NULL);
    SendMessageW(g_hEdit, EM_LIMITTEXT, 0, 0);

    g_hMsgDump = CreateWindowW(/*WC_EDITW*/ L"Edit", L"", EDIT_STYLE_WRAP | ES_READONLY,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                            hWndParent, NULL, g_hInst, NULL);
    SendMessageW(g_hMsgDump, EM_LIMITTEXT, 0, 0);

    /* Finally shows new edit controls */
    ShowWindow(g_hMsgDump, SW_SHOW);
    UpdateEditWnd(hWndParent);

    /* Re-arrange controls */
    PostMessageW(hWndParent, WM_SIZE, 0, 0);
}


static void
AppendTextLineA(
    _In_ HWND hEdit,
    _In_ PCSTR pszLine)
{
    DWORD l, r;
    SendMessageW(hEdit, EM_GETSEL, (WPARAM)&l, (LPARAM)&r);
    //SendMessageW(hEdit, EM_SETSEL, -1, -1);
    SendMessageW(hEdit, EM_SETSEL, GetWindowTextLengthW(hEdit), -1);
    SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM)pszLine);
    SendMessageW(hEdit, EM_SETSEL, l, r);
}

static void dump_printf(const CHAR *fmt, ...)
{
    CHAR szText[512];
    int len;

    va_list va;
    va_start(va, fmt);
    len = _vsnprintf(szText, _countof(szText) - 2, fmt, va);
    if (len < 0)
        *szText = ANSI_NULL;
    va_end(va);

    /* Ensure the line ends with CR-LF */
    len = min(len, _countof(szText) - 3);
    szText[len + 0] = '\r';
    szText[len + 1] = '\n';
    szText[len + 2] = ANSI_NULL;

    /* Append the text line */
    AppendTextLineA(g_hMsgDump, szText);
    //DbgPrint("TEST: %s\n", szText);
    OutputDebugStringA(szText);
}
#define MSGDUMP_PRINTF dump_printf
//#include "msgdump.h"
#define MSGDUMP_PREFIX ""
#define MSGDUMP_API WINAPI

/* For 'PCSTR VkNames[]' */
#include "data.c"

static int
GetVkName(
    _In_ UINT vk,
    _Out_writes_(cchBuff) PSTR pszBuffer,
    _In_ SIZE_T cchBuff)
{
    PCSTR vkName;

    if (vk > 0xFF)
        vk = 0xFF; // VK_UNKNOWN
    vkName = VkNames[vk];

    if (!vkName)
    {
        return _snprintf(pszBuffer, cchBuff, "%lX", vk);
    }
    else
    {
        if ((0x30 <= vk && vk <= 0x39) || (0x41 <= vk && vk <= 0x5A))
            return _snprintf(pszBuffer, cchBuff, "'%s'", vkName);
        else
            return _snprintf(pszBuffer, cchBuff, "%s", vkName);
    }
}

static __inline void MSGDUMP_API
MD_OnKey(_In_ CHAR Pfx, _In_ HWND hwnd, _In_ BOOL fDown, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    UINT vk = (UINT)wParam;
    UINT cRepeat = (UINT)LOWORD(lParam);
    USHORT flags = HIWORD(lParam);
    UCHAR ScanCode = (flags & 0xFF);

    CHAR szVkName[100];
    GetVkName(vk, szVkName, _countof(szVkName));

    if (fDown)
    {
        MSGDUMP_PRINTF("%p  %c  %sWM_KEYDOWN\tnVirtKey:%s cRepeat:%u ScanCode:%02X fExtended:%u fAltDown:%u fRepeat:%u fUp:%u [wParam:%08lX lParam:%08lX]",
                      hwnd, Pfx, MSGDUMP_PREFIX, szVkName, cRepeat, ScanCode,
                      !!(flags & KF_EXTENDED), !!(flags & KF_ALTDOWN), !!(flags & KF_REPEAT), !!(flags & KF_UP),
                      wParam, lParam);
    }
    else
    {
        MSGDUMP_PRINTF("%p  %c  %sWM_KEYUP\tnVirtKey:%s cRepeat:%u ScanCode:%02X fExtended:%u fAltDown:%u fRepeat:%u fUp:%u [wParam:%08lX lParam:%08lX]",
                      hwnd, Pfx, MSGDUMP_PREFIX, szVkName, cRepeat, ScanCode,
                      !!(flags & KF_EXTENDED), !!(flags & KF_ALTDOWN), !!(flags & KF_REPEAT), !!(flags & KF_UP),
                      wParam, lParam);
    }
}

static __inline void MSGDUMP_API
MD_OnChar(_In_ CHAR Pfx, _In_ HWND hwnd, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    WCHAR ch = (WCHAR)wParam;
    UINT cRepeat = (UINT)LOWORD(lParam);
    USHORT flags = HIWORD(lParam);
    UCHAR ScanCode = (flags & 0xFF);
    MSGDUMP_PRINTF("%p  %c  %sWM_CHAR\tchCharCode:'%04X' (%u) cRepeat:%u ScanCode:%02X fExtended:%u fAltDown:%u fRepeat:%u fUp:%u [wParam:%08lX lParam:%08lX]",
                   hwnd, Pfx, MSGDUMP_PREFIX, ch, ch, cRepeat, ScanCode,
                   !!(flags & KF_EXTENDED), !!(flags & KF_ALTDOWN), !!(flags & KF_REPEAT), !!(flags & KF_UP),
                   wParam, lParam);
}

static __inline void MSGDUMP_API
MD_OnDeadChar(_In_ CHAR Pfx, _In_ HWND hwnd, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    WCHAR ch = (WCHAR)wParam;
    UINT cRepeat = (UINT)LOWORD(lParam);
    USHORT flags = HIWORD(lParam);
    UCHAR ScanCode = (flags & 0xFF);
    MSGDUMP_PRINTF("%p  %c  %sWM_DEADCHAR\tchCharCode:'%04X' (%u) cRepeat:%u ScanCode:%02X fExtended:%u fAltDown:%u fRepeat:%u fUp:%u [wParam:%08lX lParam:%08lX]",
                   hwnd, Pfx, MSGDUMP_PREFIX, ch, ch, cRepeat, ScanCode,
                   !!(flags & KF_EXTENDED), !!(flags & KF_ALTDOWN), !!(flags & KF_REPEAT), !!(flags & KF_UP),
                   wParam, lParam);
}

static __inline void MSGDUMP_API
MD_OnSysKey(_In_ CHAR Pfx, _In_ HWND hwnd, _In_ BOOL fDown, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    UINT vk = (UINT)wParam;
    UINT cRepeat = (UINT)LOWORD(lParam);
    USHORT flags = HIWORD(lParam);
    UCHAR ScanCode = (flags & 0xFF);

    CHAR szVkName[100];
    GetVkName(vk, szVkName, _countof(szVkName));

    if (fDown)
    {
        MSGDUMP_PRINTF("%p  %c  %sWM_SYSKEYDOWN\tnVirtKey:%s cRepeat:%u ScanCode:%02X fExtended:%u fAltDown:%u fRepeat:%u fUp:%u [wParam:%08lX lParam:%08lX]",
                      hwnd, Pfx, MSGDUMP_PREFIX, szVkName, cRepeat, ScanCode,
                      !!(flags & KF_EXTENDED), !!(flags & KF_ALTDOWN), !!(flags & KF_REPEAT), !!(flags & KF_UP),
                      wParam, lParam);
    }
    else
    {
        MSGDUMP_PRINTF("%p  %c  %sWM_SYSKEYUP\tnVirtKey:%s cRepeat:%u ScanCode:%02X fExtended:%u fAltDown:%u fRepeat:%u fUp:%u [wParam:%08lX lParam:%08lX]",
                      hwnd, Pfx, MSGDUMP_PREFIX, szVkName, cRepeat, ScanCode,
                      !!(flags & KF_EXTENDED), !!(flags & KF_ALTDOWN), !!(flags & KF_REPEAT), !!(flags & KF_UP),
                      wParam, lParam);
    }
}

static __inline void MSGDUMP_API
MD_OnSysChar(_In_ CHAR Pfx, _In_ HWND hwnd, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    WCHAR ch = (WCHAR)wParam;
    UINT cRepeat = (UINT)LOWORD(lParam);
    USHORT flags = HIWORD(lParam);
    UCHAR ScanCode = (flags & 0xFF);
    MSGDUMP_PRINTF("%p  %c  %sWM_SYSCHAR\tchCharCode:'%04X' (%u) cRepeat:%u ScanCode:%02X fExtended:%u fAltDown:%u fRepeat:%u fUp:%u [wParam:%08lX lParam:%08lX]",
                   hwnd, Pfx, MSGDUMP_PREFIX, ch, ch, cRepeat, ScanCode,
                   !!(flags & KF_EXTENDED), !!(flags & KF_ALTDOWN), !!(flags & KF_REPEAT), !!(flags & KF_UP),
                   wParam, lParam);
}

static __inline void MSGDUMP_API
MD_OnSysDeadChar(_In_ CHAR Pfx, _In_ HWND hwnd, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    WCHAR ch = (WCHAR)wParam;
    UINT cRepeat = (UINT)LOWORD(lParam);
    USHORT flags = HIWORD(lParam);
    UCHAR ScanCode = (flags & 0xFF);
    MSGDUMP_PRINTF("%p  %c  %sWM_SYSDEADCHAR\tchCharCode:'%04X' (%u) cRepeat:%u ScanCode:%02X fExtended:%u fAltDown:%u fRepeat:%u fUp:%u [wParam:%08lX lParam:%08lX]",
                   hwnd, Pfx, MSGDUMP_PREFIX, ch, ch, cRepeat, ScanCode,
                   !!(flags & KF_EXTENDED), !!(flags & KF_ALTDOWN), !!(flags & KF_REPEAT), !!(flags & KF_UP),
                   wParam, lParam);
}

static __inline void MSGDUMP_API
MD_OnUniChar(_In_ CHAR Pfx, _In_ HWND hwnd, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    WCHAR ch = (WCHAR)wParam;
    UINT cRepeat = (UINT)LOWORD(lParam);
    USHORT flags = HIWORD(lParam);
    UCHAR ScanCode = (flags & 0xFF);
    MSGDUMP_PRINTF("%p  %c  %sWM_UNICHAR\tchCharCode:'%04X' (%u) cRepeat:%u ScanCode:%02X fExtended:%u fAltDown:%u fRepeat:%u fUp:%u [wParam:%08lX lParam:%08lX]",
                   hwnd, Pfx, MSGDUMP_PREFIX, ch, ch, cRepeat, ScanCode,
                   !!(flags & KF_EXTENDED), !!(flags & KF_ALTDOWN), !!(flags & KF_REPEAT), !!(flags & KF_UP),
                   wParam, lParam);
}

static __inline void MSGDUMP_API
MD_OnHotKey(_In_ CHAR Pfx, _In_ HWND hwnd, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    int idHotKey = (int)wParam;
    UINT fuModifiers = (UINT)LOWORD(lParam);
    UINT vk = (UINT)HIWORD(lParam);

    CHAR szVkName[100];
    GetVkName(vk, szVkName, _countof(szVkName));
    // TODO: Convert fuModifiers to "human-readable" names.

    MSGDUMP_PRINTF("%p  %c  %sWM_HOTKEY\tidHotKey:%04X fuModifiers:%u nVirtKey:%s [wParam:%08lX lParam:%08lX]",
                   hwnd, Pfx, MSGDUMP_PREFIX, idHotKey, fuModifiers, szVkName,
                   wParam, lParam);
}

static __inline LRESULT MSGDUMP_API
MD_msgdump(_In_ CHAR Pfx, _In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_KEYDOWN:     MD_OnKey(Pfx, hwnd, TRUE, wParam, lParam);     break;
        case WM_KEYUP:       MD_OnKey(Pfx, hwnd, FALSE, wParam, lParam);    break;
        case WM_CHAR:        MD_OnChar(Pfx, hwnd, wParam, lParam);          break;
        case WM_DEADCHAR:    MD_OnDeadChar(Pfx, hwnd, wParam, lParam);      break;
        case WM_SYSKEYDOWN:  MD_OnSysKey(Pfx, hwnd, TRUE, wParam, lParam);  break;
        case WM_SYSKEYUP:    MD_OnSysKey(Pfx, hwnd, FALSE, wParam, lParam); break;
        case WM_SYSCHAR:     MD_OnSysChar(Pfx, hwnd, wParam, lParam);       break;
        case WM_SYSDEADCHAR: MD_OnSysDeadChar(Pfx, hwnd, wParam, lParam);   break;
        case WM_UNICHAR:     MD_OnUniChar(Pfx, hwnd, wParam, lParam);       break;
        case WM_HOTKEY:      MD_OnHotKey(Pfx, hwnd, wParam, lParam);        break;
        case WM_IME_CHAR:       break; // TODO
        case WM_IME_KEYDOWN:    break; // TODO
        case WM_IME_KEYUP:      break; // TODO
    }
    return 0;
}

LRESULT CALLBACK
HookProcCallWnd(
    _In_ int nCode,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    PCWPSTRUCT pcwp;

    //if (nCode < 0) /* Do not process the message */
    //    return CallNextHookEx(g_hHook1, nCode, wParam, lParam);

    pcwp = (PCWPSTRUCT)lParam;
    MD_msgdump('C', pcwp->hwnd, pcwp->message, pcwp->wParam, pcwp->lParam);

    return CallNextHookEx(g_hHook1, nCode, wParam, lParam);
}

LRESULT CALLBACK
HookProcGetMsg(
    _In_ int nCode,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    PMSG pmsg;

    //if (nCode < 0) /* Do not process the message */
    //    return CallNextHookEx(g_hHook2, nCode, wParam, lParam);

    pmsg = (PMSG)lParam;
    MD_msgdump('G', pmsg->hwnd, pmsg->message, pmsg->wParam, pmsg->lParam);

    return CallNextHookEx(g_hHook2, nCode, wParam, lParam);
}

LRESULT CALLBACK
HookProcMessage(
    _In_ int nCode,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    PMSG pmsg;

    //if (nCode < 0) /* Do not process the message */
    //    return CallNextHookEx(g_hHook3, nCode, wParam, lParam);

    pmsg = (PMSG)lParam;
    MD_msgdump('M', pmsg->hwnd, pmsg->message, pmsg->wParam, pmsg->lParam);

    return CallNextHookEx(g_hHook3, nCode, wParam, lParam);
}


LRESULT CALLBACK
WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // MD_msgdump(hWnd, uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_CREATE:
        DoCreateEditChildren(hWnd);
        UpdateAccelXlatMenu(hWnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_HOTKEY:
        if (wParam == g_aHotKey1) // && LOWORD(lParam) == MOD_ALT && HIWORD(lParam) == VK_NUMPAD4
            PostMessageW(hWnd, WM_COMMAND, MAKEWPARAM(IDM_FOUR, 1), 0);
        else if (wParam == g_aHotKey2) // && LOWORD(lParam) == MOD_ALT && HIWORD(lParam) == VK_NUMPAD5
            PostMessageW(hWnd, WM_COMMAND, MAKEWPARAM(IDM_FIVE, 1), 0);
        break;

    case WM_COMMAND:
    {
        WORD wmId    = LOWORD(wParam);
        //WORD wmEvent = HIWORD(wParam);
        switch (wmId)
        {
        case IDM_ONE: case IDM_TWO: case IDM_THREE:
        case IDM_FOUR: case IDM_FIVE:
        case IDM_THIRTEEN: case IDM_FOURTEEN: case IDM_FIFTEEN:
        {
            /* See `IDC_TESTAPP ACCELERATORS` */
            static const PCWSTR Messages[] =
                {L"One (Alt '1' ASCII)", L"Two (Alt 2 VIRTKEY)", L"Three (Alt VK_NUMPAD3 VIRTKEY)",
                 L"Four (Alt VK_NUMPAD4 -- WM_HOTKEY thread)", L"Five (Alt VK_NUMPAD5 -- WM_HOTKEY hWnd)",
                 L"Thirteen (Alt 'D' ASCII)", L"Fourteen (Alt E VIRTKEY)", L"Fifteen (Alt F VIRTKEY)"};

            if (IDM_ONE <= wmId && wmId <= IDM_FIVE)
                wmId -= IDM_ONE;
            else if (IDM_THIRTEEN <= wmId && wmId <= IDM_FIFTEEN)
                wmId -= IDM_THIRTEEN - (IDM_FIVE - IDM_ONE + 1);
            MessageBoxW(hWnd, Messages[wmId], L"Info", MB_OK);
            break;
        }

        case IDM_SHOWHIDEEDIT:
            g_bShowEdit = !g_bShowEdit;
            UpdateEditWnd(hWnd);
            break;

        case IDM_ACCELXLAT:
            g_bXlatAccels = !g_bXlatAccels;
            UpdateAccelXlatMenu(hWnd);
            break;

        case IDM_CLEARLOG:
            SendMessageW(g_hMsgDump, WM_SETTEXT, 0, (LPARAM)L"");
            break;

        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
        }
        break;
    }

    case WM_SIZE:
    {
        RECT rc;
        GetClientRect(hWnd, &rc);
        if (g_bShowEdit)
        {
            MoveWindow(g_hEdit, 0, 0, rc.right, rc.bottom / 2, TRUE);
            MoveWindow(g_hMsgDump, 0, rc.bottom / 2, rc.right, rc.bottom / 2, TRUE);
        }
        else
        {
            MoveWindow(g_hMsgDump, 0, 0, rc.right, rc.bottom, TRUE);
        }
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;
    }

    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

/* EOF */
