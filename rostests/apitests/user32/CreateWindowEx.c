/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CreateWindowEx
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 *                  Mark Jansen
 */

#include <apitest.h>
#include <winuser.h>
#include <msgtrace.h>
#include <user32testhelpers.h>

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

HWND g_TestWindow = NULL;

static int get_iwnd(HWND hWnd)
{
    if (!g_TestWindow)
        g_TestWindow = hWnd;

    return hWnd == g_TestWindow ? 1 : 0;
}

DWORD g_FaultLine = 0;
DWORD g_NcExpectStyle = 0;
DWORD g_NcExpectExStyle = 0;
DWORD g_ExpectStyle = 0;
DWORD g_ExpectExStyle = 0;

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

MSG_ENTRY create_chain[] =
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

MSG_ENTRY create_chain_modify[] =
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

MSG_ENTRY create_chain_modify_below8_nonsrv[] =
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

    g_ExpectStyle = g_NcExpectStyle = 0;
    g_ExpectExStyle = g_NcExpectExStyle = WS_EX_CLIENTEDGE;
    g_FaultLine = __LINE__ + 1;
    hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Test_Message_Window_XX", L"", 0, 10, 20,
                           200, 210, NULL, NULL, 0, NULL);

    ok(hWnd == g_TestWindow, "We are testing with %p instead of %p\n", g_TestWindow, hWnd);
    COMPARE_CACHE(create_chain);
    DestroyWindow(hWnd);
    g_TestWindow = NULL;
    EMPTY_CACHE();

    g_ExpectStyle = g_NcExpectStyle = WS_OVERLAPPEDWINDOW;
    g_ExpectExStyle = g_NcExpectExStyle = WS_EX_OVERLAPPEDWINDOW;
    g_FaultLine = __LINE__ + 1;
    hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Test_Message_Window_XX", L"", WS_OVERLAPPEDWINDOW, 10, 20,
                           200, 210, NULL, NULL, 0, NULL);

    ok(hWnd == g_TestWindow, "We are testing with %p instead of %p\n", g_TestWindow, hWnd);
    COMPARE_CACHE(create_chain);
    DestroyWindow(hWnd);
    g_TestWindow = NULL;
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
    g_TestWindow = NULL;
    EMPTY_CACHE();

    g_ExpectStyle = g_NcExpectStyle = WS_OVERLAPPEDWINDOW;
    g_ExpectExStyle = g_NcExpectExStyle = WS_EX_OVERLAPPEDWINDOW;
    g_FaultLine = __LINE__ + 1;
    hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Test_Message_Window_XX", L"", WS_OVERLAPPEDWINDOW, 10, 20,
                           200, 210, NULL, NULL, 0, NULL);

    ok(hWnd == g_TestWindow, "We are testing with %p instead of %p\n", g_TestWindow, hWnd);
    COMPARE_CACHE(Below8NonServer ? create_chain_modify_below8_nonsrv : create_chain_modify);
    DestroyWindow(hWnd);
    g_TestWindow = NULL;
    EMPTY_CACHE();
}


START_TEST(CreateWindowEx)
{
    Test_Params();
    Test_Messages();
}
