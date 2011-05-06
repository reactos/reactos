/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for DeferWindowPos function family
 * PROGRAMMERS:     Thomas Faber
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>

#define ok_windowpos(hwnd, x, y, w, h, wnd) do { RECT rt; GetWindowRect(hwnd, &rt); \
                                                 ok(rt.left == (x) && rt.top == (y) && rt.right == (x)+(w) && rt.bottom == (y)+(h), \
                                                    "Unexpected %s position: (%ld, %ld) - (%ld, %ld)\n", wnd, rt.left, rt.top, rt.right, rt.bottom); } while (0)

#define ok_lasterr(err, s) ok(GetLastError() == (err), "%s error = %lu\n", s, GetLastError())

static void Test_DeferWindowPos(HWND hWnd, HWND hWnd2)
{
    HDWP hDwp;
    BOOL ret;

    /* close invalid handles */
    SetLastError(DNS_ERROR_RCODE_NXRRSET);
    ret = EndDeferWindowPos(NULL);
    ok(ret == 0, "EndDeferWindowPos succeeded with invalid handle\n");
    ok_lasterr(ERROR_INVALID_DWP_HANDLE, "EndDeferWindowPos");

    SetLastError(DNS_ERROR_RCODE_NXRRSET);
    ret = EndDeferWindowPos((HDWP)-1);
    ok(ret == 0, "EndDeferWindowPos succeeded with invalid handle\n");
    ok_lasterr(ERROR_INVALID_DWP_HANDLE, "EndDeferWindowPos");
    
    /* negative window count */
    SetLastError(DNS_ERROR_RCODE_NXRRSET);
    hDwp = BeginDeferWindowPos(-1);
    ok(hDwp == NULL, "BeginDeferWindowPos failed\n");
    ok_lasterr(ERROR_INVALID_PARAMETER, "BeginDeferWindowPos");

    /* zero windows */
    SetLastError(DNS_ERROR_RCODE_NXRRSET);
    hDwp = BeginDeferWindowPos(0);
    ok(hDwp != NULL, "BeginDeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "BeginDeferWindowPos");
    ret = EndDeferWindowPos(hDwp);
    ok(ret != 0, "EndDeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "EndDeferWindowPos");

    /* more windows than expected */
    SetLastError(DNS_ERROR_RCODE_NXRRSET);
    hDwp = BeginDeferWindowPos(0);
    ok(hDwp != NULL, "BeginDeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "BeginDeferWindowPos");
    hDwp = DeferWindowPos(hDwp, hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
    ok(hDwp != NULL, "DeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "DeferWindowPos");
    ret = EndDeferWindowPos(hDwp);
    ok(ret != 0, "EndDeferWindowPos failed\n");
    ok(GetLastError() == ERROR_SUCCESS /* win7 sp1/x64 */
       || GetLastError() == ERROR_INVALID_WINDOW_HANDLE /* 2k3 sp1/x86 */,
       "EndDeferWindowPos error = %lu\n", GetLastError());
    ok_windowpos(hWnd, 10, 20, 200, 210, "Window 1");

    /* more windows than expected 2 */
    SetLastError(DNS_ERROR_RCODE_NXRRSET);
    hDwp = BeginDeferWindowPos(1);
    ok(hDwp != NULL, "BeginDeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "BeginDeferWindowPos");
    hDwp = DeferWindowPos(hDwp, hWnd, NULL, 30, 20, 190, 195, SWP_NOZORDER);
    ok(hDwp != NULL, "DeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "DeferWindowPos");
    hDwp = DeferWindowPos(hDwp, hWnd2, NULL, 20, 30, 195, 190, SWP_NOZORDER);
    ok(hDwp != NULL, "DeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "DeferWindowPos");
    ret = EndDeferWindowPos(hDwp);
    ok(ret != 0, "EndDeferWindowPos failed\n");
    ok_lasterr(ERROR_SUCCESS, "EndDeferWindowPos");
    ok_windowpos(hWnd, 30, 20, 190, 195, "Window 1");
    ok_windowpos(hWnd2, 20, 30, 195, 190, "Window 2");

    /* fewer windows than expected */
    MoveWindow(hWnd, 10, 20, 200, 210, FALSE);
    SetLastError(DNS_ERROR_RCODE_NXRRSET);
    hDwp = BeginDeferWindowPos(2);
    ok(hDwp != NULL, "BeginDeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "BeginDeferWindowPos");
    hDwp = DeferWindowPos(hDwp, hWnd, NULL, 20, 10, 210, 200, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
    ok(hDwp != NULL, "DeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "DeferWindowPos");
    ret = EndDeferWindowPos(hDwp);
    ok(ret != 0, "EndDeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "EndDeferWindowPos");
    ok_windowpos(hWnd, 10, 20, 200, 210, "Window 1");

    /* no operation, 1 window */
    SetLastError(DNS_ERROR_RCODE_NXRRSET);
    hDwp = BeginDeferWindowPos(1);
    ok(hDwp != NULL, "BeginDeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "BeginDeferWindowPos");
    hDwp = DeferWindowPos(hDwp, hWnd, NULL, 20, 10, 210, 200, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
    ok(hDwp != NULL, "DeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "DeferWindowPos");
    ret = EndDeferWindowPos(hDwp);
    ok(ret != 0, "EndDeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "EndDeferWindowPos");
    ok_windowpos(hWnd, 10, 20, 200, 210, "Window 1");

    /* same window twice */
    SetLastError(DNS_ERROR_RCODE_NXRRSET);
    hDwp = BeginDeferWindowPos(2);
    ok(hDwp != NULL, "BeginDeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "BeginDeferWindowPos");
    hDwp = DeferWindowPos(hDwp, hWnd, NULL, 80, 90, 220, 210, SWP_NOZORDER);
    ok(hDwp != NULL, "DeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "DeferWindowPos");
    hDwp = DeferWindowPos(hDwp, hWnd, NULL, 100, 110, 230, 250, SWP_NOZORDER);
    ok(hDwp != NULL, "DeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "DeferWindowPos");
    ret = EndDeferWindowPos(hDwp);
    ok(ret != 0, "EndDeferWindowPos failed\n");
    ok_lasterr(ERROR_SUCCESS, "EndDeferWindowPos");
    ok_windowpos(hWnd, 100, 110, 230, 250, "Window 1");

    /* move & resize operation, 1 window */
    SetLastError(DNS_ERROR_RCODE_NXRRSET);
    hDwp = BeginDeferWindowPos(1);
    ok(hDwp != NULL, "BeginDeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "BeginDeferWindowPos");
    hDwp = DeferWindowPos(hDwp, hWnd, NULL, 20, 10, 210, 200, SWP_NOZORDER);
    ok(hDwp != NULL, "DeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "DeferWindowPos");
    ret = EndDeferWindowPos(hDwp);
    ok(ret != 0, "EndDeferWindowPos failed\n");
    ok_lasterr(ERROR_SUCCESS, "EndDeferWindowPos");
    ok_windowpos(hWnd, 20, 10, 210, 200, "Window 1");

    /* move & resize operation, 2 windows */
    SetLastError(DNS_ERROR_RCODE_NXRRSET);
    hDwp = BeginDeferWindowPos(2);
    ok(hDwp != NULL, "BeginDeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "BeginDeferWindowPos");
    hDwp = DeferWindowPos(hDwp, hWnd, NULL, 50, 60, 230, 240, SWP_NOZORDER);
    ok(hDwp != NULL, "DeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "DeferWindowPos");
    hDwp = DeferWindowPos(hDwp, hWnd2, NULL, 70, 80, 250, 260, SWP_NOZORDER);
    ok(hDwp != NULL, "DeferWindowPos failed\n");
    ok_lasterr(DNS_ERROR_RCODE_NXRRSET, "DeferWindowPos");
    ret = EndDeferWindowPos(hDwp);
    ok(ret != 0, "EndDeferWindowPos failed\n");
    ok_lasterr(ERROR_SUCCESS, "EndDeferWindowPos");
    ok_windowpos(hWnd, 50, 60, 230, 240, "Window 1");
    ok_windowpos(hWnd2, 70, 80, 250, 260, "Window 2");
}

START_TEST(DeferWindowPos)
{
    HWND hWnd = CreateWindowExW(0, L"EDIT", L"abc", 0, 10, 20,
        200, 210, NULL, NULL, GetModuleHandle(NULL), NULL);
    HWND hWnd2 = CreateWindowExW(0, L"EDIT", L"def", 0, 30, 40,
        220, 230, NULL, NULL, GetModuleHandle(NULL), NULL);
    ok(hWnd != NULL, "CreateWindow failed\n");
    ok(hWnd2 != NULL, "CreateWindow failed\n");
    ok_windowpos(hWnd, 10, 20, 200, 210, "Window 1");
    ok_windowpos(hWnd2, 30, 40, 220, 230, "Window 2");

    Test_DeferWindowPos(hWnd, hWnd2);
    
    DestroyWindow(hWnd2);
    DestroyWindow(hWnd);
}
