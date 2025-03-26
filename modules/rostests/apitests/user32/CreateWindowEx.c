/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CreateWindowEx
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 *                  Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

static void Test_Params(void)
{
    HWND hWnd;
    DWORD dwError;

    SetLastError(0x1234);
    hWnd = CreateWindowExW(0, L"BUTTON", NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    dwError = GetLastError();
    ok(hWnd != NULL, "hWnd = %p\n", hWnd);
    ok(dwError == 0, "error = %lu\n", dwError);
    DestroyWindow(hWnd);

    SetLastError(0x1234);
    hWnd = CreateWindowExW(0, L"BUTTON", NULL, 0, 0, 0, 0, 0, (HWND)(LONG_PTR)-1, NULL, NULL, NULL);
    dwError = GetLastError();
    ok(hWnd == NULL, "hWnd = %p\n", hWnd);
    ok(dwError == ERROR_INVALID_WINDOW_HANDLE, "error = %lu\n", dwError);

    SetLastError(0x1234);
    hWnd = CreateWindowExW(0, L"BUTTON", NULL, WS_CHILD, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    dwError = GetLastError();
    ok(hWnd == NULL, "hWnd = %p\n", hWnd);
    ok(dwError == ERROR_TLW_WITH_WSCHILD, "error = %lu\n", dwError);

    SetLastError(0x1234);
    hWnd = CreateWindowExW(0, L"BUTTON", NULL, WS_CHILD, 0, 0, 0, 0, (HWND)(LONG_PTR)-1, NULL, NULL, NULL);
    dwError = GetLastError();
    ok(hWnd == NULL, "hWnd = %p\n", hWnd);
    ok(dwError == ERROR_INVALID_WINDOW_HANDLE, "error = %lu\n", dwError);

    SetLastError(0x1234);
    hWnd = CreateWindowExW(0, L"BUTTON", NULL, WS_POPUP, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    dwError = GetLastError();
    ok(hWnd != NULL, "hWnd = %p\n", hWnd);
    ok(dwError == 0, "error = %lu\n", dwError);
    DestroyWindow(hWnd);

    SetLastError(0x1234);
    hWnd = CreateWindowExW(0, L"BUTTON", NULL, WS_POPUP, 0, 0, 0, 0, (HWND)(LONG_PTR)-1, NULL, NULL, NULL);
    dwError = GetLastError();
    ok(hWnd == NULL, "hWnd = %p\n", hWnd);
    ok(dwError == ERROR_INVALID_WINDOW_HANDLE, "error = %lu\n", dwError);

    SetLastError(0x1234);
    hWnd = CreateWindowExW(0, L"BUTTON", NULL, WS_CHILD|WS_POPUP, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    dwError = GetLastError();
    ok(hWnd != NULL, "hWnd = %p\n", hWnd);
    ok(dwError == 0, "error = %lu\n", dwError);
    DestroyWindow(hWnd);

    SetLastError(0x1234);
    hWnd = CreateWindowExW(0, L"BUTTON", NULL, WS_CHILD|WS_POPUP, 0, 0, 0, 0, (HWND)(LONG_PTR)-1, NULL, NULL, NULL);
    dwError = GetLastError();
    ok(hWnd == NULL, "hWnd = %p\n", hWnd);
    ok(dwError == ERROR_INVALID_WINDOW_HANDLE, "error = %lu\n", dwError);
}

static HWND g_TestWindow = NULL;
static HWND g_ChildWindow = NULL;
static HWND g_hwndMDIClient = NULL;

static int get_iwnd(HWND hWnd)
{
    if (!g_TestWindow)
        g_TestWindow = hWnd;
    if (!g_ChildWindow && hWnd != g_TestWindow)
        g_ChildWindow = hWnd;

    if (hWnd == g_TestWindow)
        return 1;
    else if (hWnd == g_ChildWindow)
        return 2;
    return 0;
}

static DWORD g_FaultLine = 0;
static DWORD g_NcExpectStyle = 0;
static DWORD g_NcExpectExStyle = 0;
static DWORD g_ExpectStyle = 0;
static DWORD g_ExpectExStyle = 0;

static DWORD g_ChildNcExpectStyle = 0;
static DWORD g_ChildNcExpectExStyle = 0;
static DWORD g_ChildExpectStyle = 0;
static DWORD g_ChildExpectExStyle = 0;

#undef ok_hex_
#define ok_hex_(expression, result) \
    do { \
        int _value = (expression); \
        ok_(__FILE__, g_FaultLine)(_value == (result), "Wrong value for '%s', expected: " #result " (0x%x), got: 0x%x\n", \
           #expression, (int)(result), _value); \
    } while (0)



static int g_ChangeStyle = 0;
static LRESULT CALLBACK MSGTestProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet;
    int iwnd = get_iwnd(hWnd);

    if(message > WM_USER || !iwnd || IsDWmMsg(message) || IseKeyMsg(message))
        return DefWindowProc(hWnd, message, wParam, lParam);

    switch(message)
    {
    case WM_IME_SETCONTEXT:
    case WM_IME_NOTIFY:
    case WM_GETICON:
    case WM_GETTEXT:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    case WM_NCCREATE:
    {
        LPCREATESTRUCT create = (LPCREATESTRUCT)lParam;
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        ok_hex_(create->style, g_NcExpectStyle);
        ok_hex_(create->dwExStyle, g_NcExpectExStyle);
        if (g_ChangeStyle)
        {
            DWORD dwStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
            dwStyle &= ~(WS_EX_CLIENTEDGE);
            SetWindowLong(hWnd, GWL_EXSTYLE, dwStyle);
            RECORD_MESSAGE(iwnd, message, MARKER, 0, 0);
            SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
                         SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_DRAWFRAME);

            RECORD_MESSAGE(iwnd, message, MARKER, 0, 0);
            ok_hex_(create->style, g_NcExpectStyle);
            ok_hex_(create->dwExStyle, g_NcExpectExStyle);
        }
    }
        break;
    case WM_CREATE:
    {
        LPCREATESTRUCT create = (LPCREATESTRUCT)lParam;
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        ok_hex_(create->style, g_ExpectStyle);
        ok_hex_(create->dwExStyle, g_ExpectExStyle);
    }
        break;
    case WM_NCCALCSIZE:
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
    case WM_SIZE:
        RECORD_MESSAGE(iwnd, message, SENT, wParam, 0);
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        ok(wParam == 0,"expected wParam=0\n");
        RECORD_MESSAGE(iwnd, message, SENT, ((WINDOWPOS*)lParam)->flags, 0);
        break;
    default:
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        break;
    }
    lRet = DefWindowProc(hWnd, message, wParam, lParam);
    RECORD_MESSAGE(iwnd, message, SENT_RET, 0, 0);
    return lRet;
}

static MSG_ENTRY create_chain[] =
{
    { 1, WM_GETMINMAXINFO, SENT },
    { 1, WM_GETMINMAXINFO, SENT_RET },
    { 1, WM_NCCREATE, SENT },
    { 1, WM_NCCREATE, SENT_RET },
    { 1, WM_NCCALCSIZE, SENT },
    { 1, WM_NCCALCSIZE, SENT_RET },
    { 1, WM_CREATE, SENT },
    { 1, WM_CREATE, SENT_RET },
    { 0, 0 }
};

static MSG_ENTRY create_chain_modify[] =
{
    { 1, WM_GETMINMAXINFO, SENT },
    { 1, WM_GETMINMAXINFO, SENT_RET },
    { 1, WM_NCCREATE, SENT },
        { 1, WM_STYLECHANGING, SENT, GWL_EXSTYLE },
        { 1, WM_STYLECHANGING, SENT_RET },
        { 1, WM_STYLECHANGED, SENT, GWL_EXSTYLE },
        { 1, WM_STYLECHANGED, SENT_RET },
    { 1, WM_NCCREATE, MARKER },
        { 1, WM_WINDOWPOSCHANGING, SENT, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED },
        { 1, WM_WINDOWPOSCHANGING, SENT_RET },
        { 1, WM_NCCALCSIZE, SENT, TRUE },
        { 1, WM_NCCALCSIZE, SENT_RET },
        { 1, WM_WINDOWPOSCHANGED, SENT, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE | SWP_FRAMECHANGED },
            { 1, WM_MOVE, SENT },
            { 1, WM_MOVE, SENT_RET },
            { 1, WM_SIZE, SENT },
            { 1, WM_SIZE, SENT_RET },
        { 1, WM_WINDOWPOSCHANGED, SENT_RET },
    { 1, WM_NCCREATE, MARKER },
    { 1, WM_NCCREATE, SENT_RET },
    { 1, WM_NCCALCSIZE, SENT },
    { 1, WM_NCCALCSIZE, SENT_RET },
    { 1, WM_CREATE, SENT },
    { 1, WM_CREATE, SENT_RET },
    { 0, 0 }
};

static MSG_ENTRY create_chain_modify_below8_nonsrv[] =
{
    { 1, WM_GETMINMAXINFO, SENT },
    { 1, WM_GETMINMAXINFO, SENT_RET },
    { 1, WM_NCCREATE, SENT },
        { 1, WM_STYLECHANGING, SENT, GWL_EXSTYLE },
        { 1, WM_STYLECHANGING, SENT_RET },
        { 1, WM_STYLECHANGED, SENT, GWL_EXSTYLE },
        { 1, WM_STYLECHANGED, SENT_RET },
    { 1, WM_NCCREATE, MARKER },
        { 1, WM_WINDOWPOSCHANGING, SENT, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED },
        { 1, WM_WINDOWPOSCHANGING, SENT_RET },
        { 1, WM_NCCALCSIZE, SENT, TRUE },
        { 1, WM_NCCALCSIZE, SENT_RET },
        { 1, WM_WINDOWPOSCHANGED, SENT, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE | SWP_FRAMECHANGED },
            { 1, WM_MOVE, SENT },
            { 1, WM_MOVE, SENT_RET },
            { 1, WM_SIZE, SENT },
            { 1, WM_SIZE, SENT_RET },
        { 1, WM_WINDOWPOSCHANGED, SENT_RET },
        { 1, WM_NCCALCSIZE, SENT, TRUE },
        { 1, WM_NCCALCSIZE, SENT_RET },
    { 1, WM_NCCREATE, MARKER },
    { 1, WM_NCCREATE, SENT_RET },
    { 1, WM_NCCALCSIZE, SENT },
    { 1, WM_NCCALCSIZE, SENT_RET },
    { 1, WM_CREATE, SENT },
    { 1, WM_CREATE, SENT_RET },
    { 0, 0 }
};


static BOOL
IsWindowsServer()
{
    OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, {0}, 0, 0, 0, VER_NT_WORKSTATION };
    DWORDLONG        const dwlConditionMask = VerSetConditionMask( 0, VER_PRODUCT_TYPE, VER_EQUAL );

    return !VerifyVersionInfoW(&osvi, VER_PRODUCT_TYPE, dwlConditionMask);
}


static BOOL
IsWindows8OrGreater()
{
    OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, {0}, 0, 0 };
    DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(
        VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL),
            VER_MINORVERSION, VER_GREATER_EQUAL),
            VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);

    osvi.dwMajorVersion = HIBYTE(_WIN32_WINNT_WIN8);
    osvi.dwMinorVersion = LOBYTE(_WIN32_WINNT_WIN8);

    return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask) != FALSE;
}

static void Test_Messages(void)
{
    HWND hWnd;
    BOOL Below8NonServer = !IsWindows8OrGreater() && !IsWindowsServer();

    RegisterSimpleClass(MSGTestProc, L"Test_Message_Window_XX");

    g_ChangeStyle = 0;

    g_ExpectStyle = g_NcExpectStyle = 0;
    g_ExpectExStyle = g_NcExpectExStyle = WS_EX_CLIENTEDGE;
    g_FaultLine = __LINE__ + 1;
    hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Test_Message_Window_XX", L"", 0, 10, 20,
                           200, 210, NULL, NULL, 0, NULL);

    ok(hWnd == g_TestWindow, "We are testing with %p instead of %p\n", g_TestWindow, hWnd);
    COMPARE_CACHE(create_chain);
    DestroyWindow(hWnd);
    g_TestWindow = g_ChildWindow = NULL;
    EMPTY_CACHE();

    g_ExpectStyle = g_NcExpectStyle = WS_OVERLAPPEDWINDOW;
    g_ExpectExStyle = g_NcExpectExStyle = WS_EX_OVERLAPPEDWINDOW;
    g_FaultLine = __LINE__ + 1;
    hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Test_Message_Window_XX", L"", WS_OVERLAPPEDWINDOW, 10, 20,
                           200, 210, NULL, NULL, 0, NULL);

    ok(hWnd == g_TestWindow, "We are testing with %p instead of %p\n", g_TestWindow, hWnd);
    COMPARE_CACHE(create_chain);
    DestroyWindow(hWnd);
    g_TestWindow = g_ChildWindow = NULL;
    EMPTY_CACHE();

    g_ChangeStyle = 1;

    g_ExpectStyle = g_NcExpectStyle = 0;
    g_ExpectExStyle = g_NcExpectExStyle = WS_EX_CLIENTEDGE;
    g_FaultLine = __LINE__ + 1;
    hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Test_Message_Window_XX", L"", 0, 10, 20,
                           200, 210, NULL, NULL, 0, NULL);

    ok(hWnd == g_TestWindow, "We are testing with %p instead of %p\n", g_TestWindow, hWnd);
    COMPARE_CACHE(Below8NonServer ? create_chain_modify_below8_nonsrv : create_chain_modify);
    DestroyWindow(hWnd);
    g_TestWindow = g_ChildWindow = NULL;
    EMPTY_CACHE();

    g_ExpectStyle = g_NcExpectStyle = WS_OVERLAPPEDWINDOW;
    g_ExpectExStyle = g_NcExpectExStyle = WS_EX_OVERLAPPEDWINDOW;
    g_FaultLine = __LINE__ + 1;
    hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Test_Message_Window_XX", L"", WS_OVERLAPPEDWINDOW, 10, 20,
                           200, 210, NULL, NULL, 0, NULL);

    ok(hWnd == g_TestWindow, "We are testing with %p instead of %p\n", g_TestWindow, hWnd);
    COMPARE_CACHE(Below8NonServer ? create_chain_modify_below8_nonsrv : create_chain_modify);
    DestroyWindow(hWnd);
    g_TestWindow = g_ChildWindow = NULL;
    EMPTY_CACHE();

    UnregisterClassW(L"Test_Message_Window_XX", NULL);
}


static LRESULT CALLBACK MSGChildProc2(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet;
    int iwnd = get_iwnd(hWnd);

    if(message > WM_USER || !iwnd || IsDWmMsg(message) || IseKeyMsg(message))
        return DefWindowProc(hWnd, message, wParam, lParam);

    switch(message)
    {
    case WM_IME_SETCONTEXT:
    case WM_IME_NOTIFY:
    case WM_GETICON:
    case WM_GETTEXT:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    case WM_NCCREATE:
    {
        LPCREATESTRUCT create = (LPCREATESTRUCT)lParam;
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        ok_hex_(create->style, g_ChildNcExpectStyle);
        ok_hex_(create->dwExStyle, g_ChildNcExpectExStyle);

        if (g_ChangeStyle)
        {
            DWORD dwStyle = GetWindowLong(g_TestWindow, GWL_EXSTYLE);
            dwStyle &= ~(WS_EX_CLIENTEDGE);
            SetWindowLong(g_TestWindow, GWL_EXSTYLE, dwStyle);
            RECORD_MESSAGE(iwnd, message, MARKER, 0, 0);
            SetWindowPos(g_TestWindow, NULL, 0, 0, 0, 0,
                         SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_DRAWFRAME);

            RECORD_MESSAGE(iwnd, message, MARKER, 0, 0);
            ok_hex_(create->style, g_ChildNcExpectStyle);
            ok_hex_(create->dwExStyle, g_ChildNcExpectExStyle);
        }
    }
    break;
    case WM_CREATE:
    {
        LPCREATESTRUCT create = (LPCREATESTRUCT)lParam;
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        ok_hex_(create->style, g_ChildExpectStyle);
        ok_hex_(create->dwExStyle, g_ChildExpectExStyle);
    }
    break;
    case WM_NCCALCSIZE:
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
    case WM_SIZE:
        RECORD_MESSAGE(iwnd, message, SENT, wParam, 0);
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        ok(wParam == 0,"expected wParam=0\n");
        RECORD_MESSAGE(iwnd, message, SENT, ((WINDOWPOS*)lParam)->flags, 0);
        break;
    default:
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        break;
    }
    lRet = DefWindowProc(hWnd, message, wParam, lParam);
    RECORD_MESSAGE(iwnd, message, SENT_RET, 0, 0);
    return lRet;
}


static LRESULT CALLBACK MSGTestProc2(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet;
    int iwnd = get_iwnd(hWnd);

    if(message > WM_USER || !iwnd || IsDWmMsg(message) || IseKeyMsg(message))
        return DefWindowProc(hWnd, message, wParam, lParam);

    switch(message)
    {
    case WM_IME_SETCONTEXT:
    case WM_IME_NOTIFY:
    case WM_GETICON:
    case WM_GETTEXT:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    case WM_NCCREATE:
    {
        HWND child;
        LPCREATESTRUCT create = (LPCREATESTRUCT)lParam;
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        ok_hex_(create->style, g_NcExpectStyle);
        ok_hex_(create->dwExStyle, g_NcExpectExStyle);

        child = CreateWindowExW(0, L"Test_Message_Window_Child2", L"", WS_CHILD, 0, 0, 10, 10, hWnd, NULL, 0, NULL);
        RECORD_MESSAGE(iwnd, message, MARKER, 0, 0);
        ok_(__FILE__, g_FaultLine)(g_ChildWindow == child, "Testing against the wrong child!\n");
    }
    break;
    case WM_NCDESTROY:
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        DestroyWindow(g_ChildWindow);
        break;
    case WM_CREATE:
    {
        LPCREATESTRUCT create = (LPCREATESTRUCT)lParam;
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        ok_hex_(create->style, g_ExpectStyle);
        ok_hex_(create->dwExStyle, g_ExpectExStyle);
    }
    break;
    case WM_NCCALCSIZE:
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
    case WM_SIZE:
        RECORD_MESSAGE(iwnd, message, SENT, wParam, 0);
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        ok(wParam == 0,"expected wParam=0\n");
        RECORD_MESSAGE(iwnd, message, SENT, ((WINDOWPOS*)lParam)->flags, 0);
        break;
    default:
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        break;
    }
    lRet = DefWindowProc(hWnd, message, wParam, lParam);
    RECORD_MESSAGE(iwnd, message, SENT_RET, 0, 0);
    return lRet;
}


MSG_ENTRY child_create_chain[] =
{
    { 1, WM_GETMINMAXINFO, SENT },
    { 1, WM_GETMINMAXINFO, SENT_RET },
    { 1, WM_NCCREATE, SENT },
        { 2, WM_NCCREATE, SENT },
        { 2, WM_NCCREATE, SENT_RET },
        { 2, WM_NCCALCSIZE, SENT },
        { 2, WM_NCCALCSIZE, SENT_RET },
        { 2, WM_CREATE, SENT },
        { 2, WM_CREATE, SENT_RET },
        { 2, WM_SIZE, SENT },
        { 2, WM_SIZE, SENT_RET },
        { 2, WM_MOVE, SENT },
        { 2, WM_MOVE, SENT_RET },
        { 1, WM_PARENTNOTIFY, SENT },
        { 1, WM_PARENTNOTIFY, SENT_RET },
    { 1, WM_NCCREATE, MARKER },
    { 1, WM_NCCREATE, SENT_RET },
    { 1, WM_NCCALCSIZE, SENT },
    { 1, WM_NCCALCSIZE, SENT_RET },
    { 1, WM_CREATE, SENT },
    { 1, WM_CREATE, SENT_RET },
    { 0, 0 }
};

MSG_ENTRY child_create_chain_modify[] =
{
    { 1, WM_GETMINMAXINFO, SENT },
    { 1, WM_GETMINMAXINFO, SENT_RET },
    { 1, WM_NCCREATE, SENT },
        { 2, WM_NCCREATE, SENT },
            { 1, WM_STYLECHANGING, SENT, GWL_EXSTYLE },
            { 1, WM_STYLECHANGING, SENT_RET },
            { 1, WM_STYLECHANGED, SENT, GWL_EXSTYLE },
            { 1, WM_STYLECHANGED, SENT_RET },
        { 2, WM_NCCREATE, MARKER },
            { 1, WM_WINDOWPOSCHANGING, SENT, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED },
            { 1, WM_WINDOWPOSCHANGING, SENT_RET },
            { 1, WM_NCCALCSIZE, SENT, TRUE },
            { 1, WM_NCCALCSIZE, SENT_RET },
            { 1, WM_WINDOWPOSCHANGED, SENT, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE | SWP_FRAMECHANGED },
                { 1, WM_MOVE, SENT },
                { 1, WM_MOVE, SENT_RET },
                { 1, WM_SIZE, SENT },
                { 1, WM_SIZE, SENT_RET },
            { 1, WM_WINDOWPOSCHANGED, SENT_RET },
        { 2, WM_NCCREATE, MARKER },
        { 2, WM_NCCREATE, SENT_RET },
        { 2, WM_NCCALCSIZE, SENT },
        { 2, WM_NCCALCSIZE, SENT_RET },
        { 2, WM_CREATE, SENT },
        { 2, WM_CREATE, SENT_RET },
        { 2, WM_SIZE, SENT },
        { 2, WM_SIZE, SENT_RET },
        { 2, WM_MOVE, SENT },
        { 2, WM_MOVE, SENT_RET },
        { 1, WM_PARENTNOTIFY, SENT },
        { 1, WM_PARENTNOTIFY, SENT_RET },
    { 1, WM_NCCREATE, MARKER },
    { 1, WM_NCCREATE, SENT_RET },
    { 1, WM_NCCALCSIZE, SENT },
    { 1, WM_NCCALCSIZE, SENT_RET },
    { 1, WM_CREATE, SENT },
    { 1, WM_CREATE, SENT_RET },
    { 0, 0 }
};

MSG_ENTRY child_create_chain_modify_below8_nonsrv[] =
{
    { 1, WM_GETMINMAXINFO, SENT },
    { 1, WM_GETMINMAXINFO, SENT_RET },
    { 1, WM_NCCREATE, SENT },
        { 2, WM_NCCREATE, SENT },
            { 1, WM_STYLECHANGING, SENT, GWL_EXSTYLE },
            { 1, WM_STYLECHANGING, SENT_RET },
            { 1, WM_STYLECHANGED, SENT, GWL_EXSTYLE },
            { 1, WM_STYLECHANGED, SENT_RET },
        { 2, WM_NCCREATE, MARKER },
            { 1, WM_WINDOWPOSCHANGING, SENT, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED },
            { 1, WM_WINDOWPOSCHANGING, SENT_RET },
            { 1, WM_NCCALCSIZE, SENT, TRUE },
            { 1, WM_NCCALCSIZE, SENT_RET },
            { 1, WM_WINDOWPOSCHANGED, SENT, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE | SWP_FRAMECHANGED },
                { 1, WM_MOVE, SENT },
                { 1, WM_MOVE, SENT_RET },
                { 1, WM_SIZE, SENT },
                { 1, WM_SIZE, SENT_RET },
            { 1, WM_WINDOWPOSCHANGED, SENT_RET },
            { 1, WM_NCCALCSIZE, SENT, TRUE },
            { 1, WM_NCCALCSIZE, SENT_RET },
        { 2, WM_NCCREATE, MARKER },
        { 2, WM_NCCREATE, SENT_RET },
        { 2, WM_NCCALCSIZE, SENT },
        { 2, WM_NCCALCSIZE, SENT_RET },
        { 2, WM_CREATE, SENT },
        { 2, WM_CREATE, SENT_RET },
        { 2, WM_SIZE, SENT },
        { 2, WM_SIZE, SENT_RET },
        { 2, WM_MOVE, SENT },
        { 2, WM_MOVE, SENT_RET },
        { 1, WM_PARENTNOTIFY, SENT },
        { 1, WM_PARENTNOTIFY, SENT_RET },
    { 1, WM_NCCREATE, MARKER },
    { 1, WM_NCCREATE, SENT_RET },
    { 1, WM_NCCALCSIZE, SENT },
    { 1, WM_NCCALCSIZE, SENT_RET },
    { 1, WM_CREATE, SENT },
    { 1, WM_CREATE, SENT_RET },
    { 0, 0 }
};



static void Test_Messages_Child(void)
{
    HWND hWnd;
    BOOL Below8NonServer = !IsWindows8OrGreater() && !IsWindowsServer();

    RegisterSimpleClass(MSGTestProc2, L"Test_Message_Window_X2");
    RegisterSimpleClass(MSGChildProc2, L"Test_Message_Window_Child2");

    g_ChangeStyle = 0;

    g_ExpectStyle = g_NcExpectStyle = 0;
    g_ExpectExStyle = g_NcExpectExStyle = WS_EX_CLIENTEDGE;
    g_ChildExpectStyle = g_ChildNcExpectStyle = WS_CHILD;
    g_ChildExpectExStyle = g_ChildNcExpectExStyle = 0;
    g_FaultLine = __LINE__ + 1;
    hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Test_Message_Window_X2", L"", 0, 10, 20,
                           200, 210, NULL, NULL, 0, NULL);

    ok(hWnd == g_TestWindow, "We are testing with %p instead of %p\n", g_TestWindow, hWnd);
    COMPARE_CACHE(child_create_chain);
    DestroyWindow(hWnd);
    g_TestWindow = g_ChildWindow = NULL;
    EMPTY_CACHE();

    g_ExpectStyle = g_NcExpectStyle = WS_OVERLAPPEDWINDOW;
    g_ExpectExStyle = g_NcExpectExStyle = WS_EX_OVERLAPPEDWINDOW;
    g_ChildExpectStyle = g_ChildNcExpectStyle = WS_CHILD;
    g_ChildExpectExStyle = g_ChildNcExpectExStyle = 0;
    g_FaultLine = __LINE__ + 1;
    hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Test_Message_Window_X2", L"", WS_OVERLAPPEDWINDOW, 10, 20,
                           200, 210, NULL, NULL, 0, NULL);

    ok(hWnd == g_TestWindow, "We are testing with %p instead of %p\n", g_TestWindow, hWnd);
    COMPARE_CACHE(child_create_chain);
    DestroyWindow(hWnd);
    g_TestWindow = g_ChildWindow = NULL;
    EMPTY_CACHE();

    g_ChangeStyle = 1;

    g_ExpectStyle = g_NcExpectStyle = 0;
    g_ExpectExStyle = g_NcExpectExStyle = WS_EX_CLIENTEDGE;
    g_ChildExpectStyle = g_ChildNcExpectStyle = WS_CHILD;
    g_ChildExpectExStyle = g_ChildNcExpectExStyle = 0;
    g_FaultLine = __LINE__ + 1;
    hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Test_Message_Window_X2", L"", 0, 10, 20,
                           200, 210, NULL, NULL, 0, NULL);

    ok(hWnd == g_TestWindow, "We are testing with %p instead of %p\n", g_TestWindow, hWnd);
    COMPARE_CACHE(Below8NonServer ? child_create_chain_modify_below8_nonsrv : child_create_chain_modify);
    DestroyWindow(hWnd);
    g_TestWindow = g_ChildWindow = NULL;
    EMPTY_CACHE();

    g_ExpectStyle = g_NcExpectStyle = WS_OVERLAPPEDWINDOW;
    g_ExpectExStyle = g_NcExpectExStyle = WS_EX_OVERLAPPEDWINDOW;
    g_ChildExpectStyle = g_ChildNcExpectStyle = WS_CHILD;
    g_ChildExpectExStyle = g_ChildNcExpectExStyle = 0;
    g_FaultLine = __LINE__ + 1;
    hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Test_Message_Window_X2", L"", WS_OVERLAPPEDWINDOW, 10, 20,
                           200, 210, NULL, NULL, 0, NULL);

    ok(hWnd == g_TestWindow, "We are testing with %p instead of %p\n", g_TestWindow, hWnd);
    COMPARE_CACHE(Below8NonServer ? child_create_chain_modify_below8_nonsrv : child_create_chain_modify);
    DestroyWindow(hWnd);
    g_TestWindow = g_ChildWindow = NULL;
    EMPTY_CACHE();

    UnregisterClassW(L"Test_Message_Window_X2", NULL);
    UnregisterClassW(L"Test_Message_Window_Child2", NULL);
}



static LRESULT CALLBACK MSGTestProcMDI(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet;
    int iwnd = get_iwnd(hWnd);

    if(message > WM_USER || !iwnd || IsDWmMsg(message) || IseKeyMsg(message))
        return DefWindowProc(hWnd, message, wParam, lParam);

    switch(message)
    {
    case WM_IME_SETCONTEXT:
    case WM_IME_NOTIFY:
    case WM_GETICON:
    case WM_GETTEXT:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    case WM_NCCREATE:
    {
        LPCREATESTRUCT create = (LPCREATESTRUCT)lParam;
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        ok_hex_(create->style, g_NcExpectStyle);
        ok_hex_(create->dwExStyle, g_NcExpectExStyle);
    }
    break;
    case WM_CREATE:
    {
        CLIENTCREATESTRUCT ccs = { 0 };
        LPCREATESTRUCT create = (LPCREATESTRUCT)lParam;
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        ok_hex_(create->style, g_ExpectStyle);
        ok_hex_(create->dwExStyle, g_ExpectExStyle);

        g_hwndMDIClient = CreateWindow("MDICLIENT", (LPCTSTR) NULL,
                                     WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL,
                                     0, 0, 0, 0, hWnd, (HMENU) 0xCAC, NULL, (LPSTR) &ccs);

        ShowWindow(g_hwndMDIClient, SW_SHOW);

    }
    break;
    case WM_NCCALCSIZE:
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
    case WM_SIZE:
        RECORD_MESSAGE(iwnd, message, SENT, wParam, 0);
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        ok(wParam == 0,"expected wParam=0\n");
        RECORD_MESSAGE(iwnd, message, SENT, ((WINDOWPOS*)lParam)->flags, 0);
        break;
    default:
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        break;
    }
    lRet = DefWindowProc(hWnd, message, wParam, lParam);
    RECORD_MESSAGE(iwnd, message, SENT_RET, 0, 0);
    return lRet;
}

MSG_ENTRY create_chain_MDI[] =
{
    { 1, WM_GETMINMAXINFO, SENT },
    { 1, WM_GETMINMAXINFO, SENT_RET },
    { 1, WM_NCCREATE, SENT },
    { 1, WM_NCCREATE, SENT_RET },
    { 1, WM_NCCALCSIZE, SENT },
    { 1, WM_NCCALCSIZE, SENT_RET },
    { 1, WM_CREATE, SENT },
    { 1, WM_PARENTNOTIFY, SENT },
    { 1, WM_PARENTNOTIFY, SENT_RET },
    { 1, WM_CREATE, SENT_RET },
    { 0, 0 }
};

static void Test_Messages_MDI(void)
{
    HWND hWnd;

    RegisterSimpleClass(MSGTestProcMDI, L"Test_Message_Window_MDI_XX");

    g_ExpectStyle = g_NcExpectStyle = 0;
    g_ExpectExStyle = g_NcExpectExStyle = WS_EX_CLIENTEDGE;
    g_FaultLine = __LINE__ + 1;
    hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Test_Message_Window_MDI_XX", L"", 0, 10, 20,
                           200, 210, NULL, NULL, 0, NULL);

    ok(hWnd == g_TestWindow, "We are testing with %p instead of %p\n", g_TestWindow, hWnd);
    COMPARE_CACHE(create_chain_MDI);
    DestroyWindow(hWnd);
    g_TestWindow = g_hwndMDIClient = g_ChildWindow = NULL;
    EMPTY_CACHE();

    g_ExpectStyle = g_NcExpectStyle = WS_OVERLAPPEDWINDOW;
    g_ExpectExStyle = g_NcExpectExStyle = WS_EX_OVERLAPPEDWINDOW;
    g_FaultLine = __LINE__ + 1;
    hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Test_Message_Window_MDI_XX", L"", WS_OVERLAPPEDWINDOW, 10, 20,
                           200, 210, NULL, NULL, 0, NULL);

    ok(hWnd == g_TestWindow, "We are testing with %p instead of %p\n", g_TestWindow, hWnd);
    COMPARE_CACHE(create_chain_MDI);
    DestroyWindow(hWnd);
    g_TestWindow = g_hwndMDIClient = g_ChildWindow = NULL;
    EMPTY_CACHE();

    UnregisterClassW(L"Test_Message_Window_MDI_XX", NULL);
}


static LRESULT CALLBACK MSGTestProcMDI2(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet;
    int iwnd = get_iwnd(hWnd);

    if(message > WM_USER || !iwnd || IsDWmMsg(message) || IseKeyMsg(message))
        return DefWindowProc(hWnd, message, wParam, lParam);

    switch(message)
    {
    case WM_IME_SETCONTEXT:
    case WM_IME_NOTIFY:
    case WM_GETICON:
    case WM_GETTEXT:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    case WM_NCCREATE:
    {
        LPCREATESTRUCT create = (LPCREATESTRUCT)lParam;
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        ok_hex_(create->style, g_NcExpectStyle);
        ok_hex_(create->dwExStyle, g_NcExpectExStyle);
    }
    break;
    case WM_CREATE:
    {
        MDICREATESTRUCT mcs = {0};
        CLIENTCREATESTRUCT ccs = { 0 };
        LPCREATESTRUCT create = (LPCREATESTRUCT)lParam;
        HWND hchild;
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        ok_hex_(create->style, g_ExpectStyle);
        ok_hex_(create->dwExStyle, g_ExpectExStyle);


        g_hwndMDIClient = CreateWindow("MDICLIENT", (LPCTSTR) NULL,
                                       WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL,
                                       0, 0, 0, 0, hWnd, (HMENU) 0xCAC, NULL, (LPSTR) &ccs);

        ShowWindow(g_hwndMDIClient, SW_SHOW);


        mcs.szClass = "Test_Message_MDI_Window_Child2";
        mcs.x = mcs.cx = CW_USEDEFAULT;
        mcs.y = mcs.cy = CW_USEDEFAULT;
        mcs.style = WS_MAXIMIZE;

        hchild = (HWND) SendMessage (g_hwndMDIClient, WM_MDICREATE, 0,
            (LPARAM)&mcs);
        ok(hchild == g_ChildWindow, "We are testing with %p instead of %p\n", g_ChildWindow, hchild);

    }
    break;
    case WM_NCCALCSIZE:
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
    case WM_SIZE:
        RECORD_MESSAGE(iwnd, message, SENT, wParam, 0);
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        ok(wParam == 0,"expected wParam=0\n");
        RECORD_MESSAGE(iwnd, message, SENT, ((WINDOWPOS*)lParam)->flags, 0);
        break;
    default:
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        break;
    }
    lRet = DefWindowProc(hWnd, message, wParam, lParam);
    RECORD_MESSAGE(iwnd, message, SENT_RET, 0, 0);
    return lRet;
}

static LRESULT CALLBACK MSGChildProcMDI2(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet;
    int iwnd = get_iwnd(hWnd);

    if(message > WM_USER || !iwnd || IsDWmMsg(message) || IseKeyMsg(message))
        return DefMDIChildProc(hWnd, message, wParam, lParam);

    switch(message)
    {
    case WM_IME_SETCONTEXT:
    case WM_IME_NOTIFY:
    case WM_GETICON:
    case WM_GETTEXT:
        return DefMDIChildProc(hWnd, message, wParam, lParam);
        break;
    case WM_NCCREATE:
    {
        LPCREATESTRUCT create = (LPCREATESTRUCT)lParam;
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        ok_hex_(create->style, g_ChildNcExpectStyle);
        ok_hex_(create->dwExStyle, g_ChildNcExpectExStyle);
    }
    break;
    case WM_CREATE:
    {
        LPCREATESTRUCT create = (LPCREATESTRUCT)lParam;
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        ok_hex_(create->style, g_ChildExpectStyle);
        ok_hex_(create->dwExStyle, g_ChildExpectExStyle);
    }
    break;
    case WM_NCCALCSIZE:
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
    case WM_SIZE:
        RECORD_MESSAGE(iwnd, message, SENT, wParam, 0);
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        ok(wParam == 0,"expected wParam=0\n");
        RECORD_MESSAGE(iwnd, message, SENT, ((WINDOWPOS*)lParam)->flags, 0);
        break;
    default:
        RECORD_MESSAGE(iwnd, message, SENT, 0, 0);
        break;
    }
    lRet = DefMDIChildProc(hWnd, message, wParam, lParam);
    RECORD_MESSAGE(iwnd, message, SENT_RET, 0, 0);
    return lRet;
}


static MSG_ENTRY child_create_chain_MDI[] =
{
    { 1, WM_GETMINMAXINFO, SENT },
    { 1, WM_GETMINMAXINFO, SENT_RET },
    { 1, WM_NCCREATE, SENT },
    { 1, WM_NCCREATE, SENT_RET },
    { 1, WM_NCCALCSIZE, SENT },
    { 1, WM_NCCALCSIZE, SENT_RET },
    { 1, WM_CREATE, SENT },
        { 1, WM_PARENTNOTIFY, SENT },
        { 1, WM_PARENTNOTIFY, SENT_RET },
        { 2, WM_GETMINMAXINFO, SENT },
        { 2, WM_GETMINMAXINFO, SENT_RET },
        { 2, WM_NCCREATE, SENT },
        { 2, WM_NCCREATE, SENT_RET },
        { 2, WM_NCCALCSIZE, SENT },
        { 2, WM_NCCALCSIZE, SENT_RET },
        { 2, WM_CREATE, SENT },
        { 2, WM_CREATE, SENT_RET },
        { 2, WM_SIZE, SENT },
        { 2, WM_SIZE, SENT_RET },
        { 2, WM_MOVE, SENT },
        { 2, WM_MOVE, SENT_RET },
        { 2, WM_GETMINMAXINFO, SENT },
        { 2, WM_GETMINMAXINFO, SENT_RET },
        { 2, WM_WINDOWPOSCHANGING, SENT, SWP_FRAMECHANGED | SWP_NOACTIVATE | 0x8000 },
        { 2, WM_GETMINMAXINFO, SENT },
        { 2, WM_GETMINMAXINFO, SENT_RET },
        { 2, WM_WINDOWPOSCHANGING, SENT_RET },
        { 2, WM_NCCALCSIZE, SENT, 1 },
        { 2, WM_NCCALCSIZE, SENT_RET },
        { 2, WM_WINDOWPOSCHANGED, SENT, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW | 0x8800 },
            { 2, WM_MOVE, SENT },
            { 2, WM_MOVE, SENT_RET },
            { 2, WM_SIZE, SENT, SIZE_MAXIMIZED },
                { 1, WM_WINDOWPOSCHANGING, SENT, SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED | SWP_NOACTIVATE },
                { 1, WM_WINDOWPOSCHANGING, SENT_RET },
                { 1, WM_NCCALCSIZE, SENT, TRUE },
                { 1, WM_NCCALCSIZE, SENT_RET },
                { 1, WM_WINDOWPOSCHANGED, SENT, SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOREDRAW | SWP_FRAMECHANGED | SWP_NOACTIVATE | 0x1800 },
                { 1, WM_WINDOWPOSCHANGED, SENT_RET },
            { 2, WM_SIZE, SENT_RET },
        { 2, WM_WINDOWPOSCHANGED, SENT_RET },
        { 2, WM_NCCALCSIZE, SENT, TRUE },
        { 2, WM_NCCALCSIZE, SENT_RET },
        { 2, WM_SHOWWINDOW, SENT },
        { 2, WM_SHOWWINDOW, SENT_RET },
        { 2, WM_WINDOWPOSCHANGING, SENT, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE },
        { 2, WM_WINDOWPOSCHANGING, SENT_RET },
        { 2, WM_CHILDACTIVATE, SENT },
        { 2, WM_WINDOWPOSCHANGING, SENT, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE },
        { 2, WM_WINDOWPOSCHANGING, SENT_RET },
        { 2, WM_MDIACTIVATE, SENT },
        { 2, WM_MDIACTIVATE, SENT_RET },
        { 2, WM_CHILDACTIVATE, SENT_RET },
        { 1, WM_WINDOWPOSCHANGING, SENT, SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_FRAMECHANGED },
        { 1, WM_WINDOWPOSCHANGING, SENT_RET },
        { 1, WM_NCCALCSIZE, SENT, TRUE },
        { 1, WM_NCCALCSIZE, SENT_RET },
        { 1, WM_WINDOWPOSCHANGED, SENT, SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOREDRAW | SWP_FRAMECHANGED | SWP_NOACTIVATE | 0x1800 },
        { 1, WM_WINDOWPOSCHANGED, SENT_RET },
    { 1, WM_CREATE, SENT_RET },
    { 0, 0 },
};

static MSG_ENTRY child_create_chain_MDI_below8[] =
{
    { 1, WM_GETMINMAXINFO, SENT },
    { 1, WM_GETMINMAXINFO, SENT_RET },
    { 1, WM_NCCREATE, SENT },
    { 1, WM_NCCREATE, SENT_RET },
    { 1, WM_NCCALCSIZE, SENT },
    { 1, WM_NCCALCSIZE, SENT_RET },
    { 1, WM_CREATE, SENT },
        { 1, WM_PARENTNOTIFY, SENT },
        { 1, WM_PARENTNOTIFY, SENT_RET },
        { 2, WM_GETMINMAXINFO, SENT },
        { 2, WM_GETMINMAXINFO, SENT_RET },
        { 2, WM_NCCREATE, SENT },
        { 2, WM_NCCREATE, SENT_RET },
        { 2, WM_NCCALCSIZE, SENT },
        { 2, WM_NCCALCSIZE, SENT_RET },
        { 2, WM_CREATE, SENT },
        { 2, WM_CREATE, SENT_RET },
        { 2, WM_SIZE, SENT },
        { 2, WM_SIZE, SENT_RET },
        { 2, WM_MOVE, SENT },
        { 2, WM_MOVE, SENT_RET },
        { 2, WM_GETMINMAXINFO, SENT },
        { 2, WM_GETMINMAXINFO, SENT_RET },
        { 2, WM_WINDOWPOSCHANGING, SENT, SWP_FRAMECHANGED | SWP_NOACTIVATE | 0x8000 },
        { 2, WM_GETMINMAXINFO, SENT },
        { 2, WM_GETMINMAXINFO, SENT_RET },
        { 2, WM_WINDOWPOSCHANGING, SENT_RET },
        { 2, WM_NCCALCSIZE, SENT, 1 },
        { 2, WM_NCCALCSIZE, SENT_RET },
        { 2, WM_WINDOWPOSCHANGED, SENT, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW | 0x8800 },
            { 2, WM_MOVE, SENT },
            { 2, WM_MOVE, SENT_RET },
            { 2, WM_SIZE, SENT, SIZE_MAXIMIZED },
                { 1, WM_WINDOWPOSCHANGING, SENT, SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED | SWP_NOACTIVATE },
                { 1, WM_WINDOWPOSCHANGING, SENT_RET },
                { 1, WM_NCCALCSIZE, SENT, TRUE },
                { 1, WM_NCCALCSIZE, SENT_RET },
                { 1, WM_WINDOWPOSCHANGED, SENT, SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOREDRAW | SWP_FRAMECHANGED | SWP_NOACTIVATE | 0x1800 },
                { 1, WM_WINDOWPOSCHANGED, SENT_RET },
            { 2, WM_SIZE, SENT_RET },
        { 2, WM_WINDOWPOSCHANGED, SENT_RET },
        //{ 2, WM_NCCALCSIZE, SENT, TRUE },
        //{ 2, WM_NCCALCSIZE, SENT_RET },
        { 2, WM_SHOWWINDOW, SENT },
        { 2, WM_SHOWWINDOW, SENT_RET },
        { 2, WM_WINDOWPOSCHANGING, SENT, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE },
        { 2, WM_WINDOWPOSCHANGING, SENT_RET },
        { 2, WM_CHILDACTIVATE, SENT },
        { 2, WM_WINDOWPOSCHANGING, SENT, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE },
        { 2, WM_WINDOWPOSCHANGING, SENT_RET },
        { 2, WM_MDIACTIVATE, SENT },
        { 2, WM_MDIACTIVATE, SENT_RET },
        { 2, WM_CHILDACTIVATE, SENT_RET },
        { 1, WM_WINDOWPOSCHANGING, SENT, SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_FRAMECHANGED },
        { 1, WM_WINDOWPOSCHANGING, SENT_RET },
        { 1, WM_NCCALCSIZE, SENT, TRUE },
        { 1, WM_NCCALCSIZE, SENT_RET },
        { 1, WM_WINDOWPOSCHANGED, SENT, SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOREDRAW | SWP_FRAMECHANGED | SWP_NOACTIVATE | 0x1800 },
        { 1, WM_WINDOWPOSCHANGED, SENT_RET },
    { 1, WM_CREATE, SENT_RET },
    { 0, 0 },
};

static MSG_ENTRY child_create_chain_MDI_below8_nonsrv[] =
{
    { 1, WM_GETMINMAXINFO, SENT },
    { 1, WM_GETMINMAXINFO, SENT_RET },
    { 1, WM_NCCREATE, SENT },
    { 1, WM_NCCREATE, SENT_RET },
    { 1, WM_NCCALCSIZE, SENT },
    { 1, WM_NCCALCSIZE, SENT_RET },
    { 1, WM_CREATE, SENT },
        { 1, WM_PARENTNOTIFY, SENT },
        { 1, WM_PARENTNOTIFY, SENT_RET },
        { 2, WM_GETMINMAXINFO, SENT },
        { 2, WM_GETMINMAXINFO, SENT_RET },
        { 2, WM_NCCREATE, SENT },
        { 2, WM_NCCREATE, SENT_RET },
        { 2, WM_NCCALCSIZE, SENT },
        { 2, WM_NCCALCSIZE, SENT_RET },
        { 2, WM_CREATE, SENT },
        { 2, WM_CREATE, SENT_RET },
        { 2, WM_SIZE, SENT },
        { 2, WM_SIZE, SENT_RET },
        { 2, WM_MOVE, SENT },
        { 2, WM_MOVE, SENT_RET },
        { 2, WM_GETMINMAXINFO, SENT },
        { 2, WM_GETMINMAXINFO, SENT_RET },
        { 2, WM_WINDOWPOSCHANGING, SENT, SWP_FRAMECHANGED | SWP_NOACTIVATE | 0x8000 },
        { 2, WM_GETMINMAXINFO, SENT },
        { 2, WM_GETMINMAXINFO, SENT_RET },
        { 2, WM_WINDOWPOSCHANGING, SENT_RET },
        { 2, WM_NCCALCSIZE, SENT, 1 },
        { 2, WM_NCCALCSIZE, SENT_RET },
        { 2, WM_WINDOWPOSCHANGED, SENT, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW | 0x8800 },
            { 2, WM_MOVE, SENT },
            { 2, WM_MOVE, SENT_RET },
            { 2, WM_SIZE, SENT, SIZE_MAXIMIZED },
                { 1, WM_WINDOWPOSCHANGING, SENT, SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED | SWP_NOACTIVATE },
                { 1, WM_WINDOWPOSCHANGING, SENT_RET },
                { 1, WM_NCCALCSIZE, SENT, TRUE },
                { 1, WM_NCCALCSIZE, SENT_RET },
                { 1, WM_WINDOWPOSCHANGED, SENT, SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOREDRAW | SWP_FRAMECHANGED | SWP_NOACTIVATE | 0x1800 },
                { 1, WM_WINDOWPOSCHANGED, SENT_RET },
                // +
                { 1, WM_NCCALCSIZE, SENT, TRUE },
                { 1, WM_NCCALCSIZE, SENT_RET },
                // -
            { 2, WM_SIZE, SENT_RET },
        { 2, WM_WINDOWPOSCHANGED, SENT_RET },
        { 2, WM_NCCALCSIZE, SENT, TRUE },
        { 2, WM_NCCALCSIZE, SENT_RET },
        { 2, WM_SHOWWINDOW, SENT },
        { 2, WM_SHOWWINDOW, SENT_RET },
        { 2, WM_WINDOWPOSCHANGING, SENT, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE },
        { 2, WM_WINDOWPOSCHANGING, SENT_RET },
        { 2, WM_CHILDACTIVATE, SENT },
        { 2, WM_WINDOWPOSCHANGING, SENT, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE },
        { 2, WM_WINDOWPOSCHANGING, SENT_RET },
        { 2, WM_MDIACTIVATE, SENT },
        { 2, WM_MDIACTIVATE, SENT_RET },
        { 2, WM_CHILDACTIVATE, SENT_RET },
        { 1, WM_WINDOWPOSCHANGING, SENT, SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_FRAMECHANGED },
        { 1, WM_WINDOWPOSCHANGING, SENT_RET },
        { 1, WM_NCCALCSIZE, SENT, TRUE },
        { 1, WM_NCCALCSIZE, SENT_RET },
        { 1, WM_WINDOWPOSCHANGED, SENT, SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOREDRAW | SWP_FRAMECHANGED | SWP_NOACTIVATE | 0x1800 },
        { 1, WM_WINDOWPOSCHANGED, SENT_RET },
    { 1, WM_CREATE, SENT_RET },
    { 0, 0 },
};

static void Test_Messages_MDI_Child(void)
{
    HWND hWnd;

    BOOL Below8 = !IsWindows8OrGreater();
    BOOL Below8NonServer = !IsWindows8OrGreater() && !IsWindowsServer();

    RegisterSimpleClass(MSGTestProcMDI2, L"Test_Message_MDI_Window_X2");
    RegisterSimpleClass(MSGChildProcMDI2, L"Test_Message_MDI_Window_Child2");

    g_ExpectStyle = g_NcExpectStyle = 0;
    g_ExpectExStyle = g_NcExpectExStyle = WS_EX_CLIENTEDGE;
    g_ChildExpectStyle = g_ChildNcExpectStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_MAXIMIZE | WS_OVERLAPPEDWINDOW;
    g_ChildExpectExStyle = g_ChildNcExpectExStyle = WS_EX_WINDOWEDGE | WS_EX_MDICHILD;
    g_FaultLine = __LINE__ + 1;
    hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Test_Message_MDI_Window_X2", L"", 0, 10, 20,
                           200, 210, NULL, NULL, 0, NULL);

    ok(hWnd == g_TestWindow, "We are testing with %p instead of %p\n", g_TestWindow, hWnd);
    COMPARE_CACHE(Below8NonServer ? (child_create_chain_MDI_below8_nonsrv) : (Below8 ? child_create_chain_MDI_below8 : child_create_chain_MDI));
    DestroyWindow(hWnd);
    g_TestWindow = g_ChildWindow = NULL;
    EMPTY_CACHE();

    g_ExpectStyle = g_NcExpectStyle = WS_OVERLAPPEDWINDOW;
    g_ExpectExStyle = g_NcExpectExStyle = WS_EX_OVERLAPPEDWINDOW;
    g_ChildExpectStyle = g_ChildNcExpectStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_MAXIMIZE | WS_OVERLAPPEDWINDOW;
    g_ChildExpectExStyle = g_ChildNcExpectExStyle = WS_EX_WINDOWEDGE | WS_EX_MDICHILD;
    g_FaultLine = __LINE__ + 1;
    hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Test_Message_MDI_Window_X2", L"", WS_OVERLAPPEDWINDOW, 10, 20,
                           200, 210, NULL, NULL, 0, NULL);

    ok(hWnd == g_TestWindow, "We are testing with %p instead of %p\n", g_TestWindow, hWnd);
    COMPARE_CACHE(Below8NonServer ? (child_create_chain_MDI_below8_nonsrv) : (Below8 ? child_create_chain_MDI_below8 : child_create_chain_MDI));
    DestroyWindow(hWnd);
    g_TestWindow = g_ChildWindow = NULL;
    EMPTY_CACHE();

    UnregisterClassW(L"Test_Message_Window_X2", NULL);
    UnregisterClassW(L"Test_Message_MDI_Window_Child2", NULL);
}


START_TEST(CreateWindowEx)
{
    Test_Params();
    Test_Messages();
    Test_Messages_Child();
    Test_Messages_MDI();
    Test_Messages_MDI_Child();
}
