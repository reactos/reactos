/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for DeferWindowPos function family
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

static HWND hWnd1, hWnd2, hWnd3, hWnd4;

/* FIXME: test for HWND_TOP, etc...*/
static int get_iwnd(HWND hWnd)
{
    if(hWnd == hWnd1) return 1;
    else if(hWnd == hWnd2) return 2;
    else if(hWnd == hWnd3) return 3;
    else if(hWnd == hWnd4) return 4;
    else return 0;
}

LRESULT CALLBACK DWPTestProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int iwnd = get_iwnd(hWnd);

    if(message > WM_USER || !iwnd || IsDWmMsg(message) || IseKeyMsg(message))
        return DefWindowProc(hWnd, message, wParam, lParam);

    switch(message)
    {
    case WM_IME_SETCONTEXT:
    case WM_IME_NOTIFY :
    case WM_GETICON :
    case WM_GETTEXT:
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS* pwp = (WINDOWPOS*)lParam;
            ok(wParam==0,"expected wParam=0\n");
            RECORD_MESSAGE(iwnd, message, SENT, get_iwnd(pwp->hwndInsertAfter), pwp->flags);
            break;
        }
    default:
        RECORD_MESSAGE(iwnd, message, SENT, 0,0);
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

static void FlushMessages()
{
    MSG msg;

    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
    {
        int iwnd = get_iwnd(msg.hwnd);
        if(!(msg.message > WM_USER || !iwnd || IsDWmMsg(msg.message) || IseKeyMsg(msg.message)))
            RECORD_MESSAGE(iwnd, msg.message, POST,0,0);
        DispatchMessageA( &msg );
    }
}

/* unfortunately sometimes MSCTF windows get between the expected chain of windows
   so just checking the next window does not work*/
static BOOL IsNext(HWND hWnd1, HWND hWnd2)
{
    HWND hWndNext;

    hWndNext=hWnd1;
    while((hWndNext = GetWindow(hWndNext,GW_HWNDNEXT)))
    {
        if(hWndNext == hWnd2)
        {
            return TRUE;
        }
    }
    return FALSE;
}

#define EXPECT_NEXT(hWnd1, hWnd2)                                         \
       ok(IsNext(hWnd1, hWnd2),                                           \
       "After hwnd%d, expected hwnd%d not hwnd%d\n",                      \
        get_iwnd(hWnd1), get_iwnd(hWnd2),get_iwnd(GetWindow(hWnd1,GW_HWNDNEXT)) )

#define EXPECT_CHAIN(A,B,C,D)       \
    EXPECT_NEXT(hWnd##A, hWnd##B);  \
    EXPECT_NEXT(hWnd##B, hWnd##C);  \
    EXPECT_NEXT(hWnd##C, hWnd##D);

#define ok_windowpos(hwnd, x, y, w, h, wnd) do { RECT rt; GetWindowRect(hwnd, &rt); \
                                                 ok(rt.left == (x) && rt.top == (y) && rt.right == (x)+(w) && rt.bottom == (y)+(h), \
                                                    "Unexpected %s position: (%ld, %ld) - (%ld, %ld)\n", wnd, rt.left, rt.top, rt.right, rt.bottom); } while (0)

#define ok_lasterr(err, s) ok(GetLastError() == (err), "%s error = %lu\n", s, GetLastError())

static void set_default_pos()
{
    SetWindowPos(hWnd1, 0, 10,20,200,210, SWP_NOOWNERZORDER|SWP_SHOWWINDOW|SWP_NOACTIVATE);
    SetWindowPos(hWnd2, 0, 30,40,220,230, SWP_NOOWNERZORDER|SWP_SHOWWINDOW|SWP_NOACTIVATE);
    SetWindowPos(hWnd3, 0, 20,350,300,300, SWP_NOOWNERZORDER|SWP_SHOWWINDOW|SWP_NOACTIVATE);
    SetWindowPos(hWnd4, 0, 250,250,200,200, SWP_NOOWNERZORDER|SWP_SHOWWINDOW|SWP_NOACTIVATE);
    SetActiveWindow(hWnd4);
    FlushMessages();
    EMPTY_CACHE();
}

static void Test_DWP_Error(HWND hWnd, HWND hWnd2)
{
    HDWP hDwp;
    BOOL ret;

    ok_windowpos(hWnd1, 10, 20, 200, 210, "Window 1");
    ok_windowpos(hWnd2, 30, 40, 220, 230, "Window 2");

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

    FlushMessages();
    EMPTY_CACHE();
}

MSG_ENTRY move1_chain[]={
    {1,WM_WINDOWPOSCHANGING, SENT, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER},
    {1,WM_GETMINMAXINFO},
    {1,WM_WINDOWPOSCHANGED, SENT, 0, SWP_NOSIZE | SWP_NOACTIVATE|SWP_NOOWNERZORDER | SWP_NOCLIENTSIZE},
    {1,WM_MOVE},
    {0,0}};

MSG_ENTRY resize1_chain[]={
    {1,WM_WINDOWPOSCHANGING, SENT, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER},
    {1,WM_GETMINMAXINFO},
    {1,WM_NCCALCSIZE},
    {1,WM_WINDOWPOSCHANGED, SENT, 0, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE|SWP_NOOWNERZORDER | SWP_NOCLIENTMOVE},
    {1,WM_SIZE},
//    {1,WM_NCCALCSIZE}, // This doesn't occur on either WHS testbot or Windows 10
    {0,0}};

MSG_ENTRY move1_2_chain[]={
    {1,WM_WINDOWPOSCHANGING, SENT, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER},
    {1,WM_GETMINMAXINFO},
    {2,WM_WINDOWPOSCHANGING, SENT, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER},
    {2,WM_GETMINMAXINFO},
    {1,WM_WINDOWPOSCHANGED, SENT, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE|SWP_NOOWNERZORDER | SWP_NOCLIENTSIZE},
    {1,WM_MOVE},
    {2,WM_WINDOWPOSCHANGED, SENT, 0, SWP_NOSIZE | SWP_NOACTIVATE|SWP_NOOWNERZORDER | SWP_NOCLIENTSIZE},
    {2,WM_MOVE},
    {0,0}};


MSG_ENTRY ZOrder1_chain[]={
      {1,WM_WINDOWPOSCHANGING, SENT, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOSIZE},
      {1,WM_WINDOWPOSCHANGED,  SENT, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOSIZE | SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
      {0,0}};

MSG_ENTRY ZOrder1and2_chain[]={
      {1,WM_WINDOWPOSCHANGING, SENT, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOSIZE},
      {2,WM_WINDOWPOSCHANGING, SENT, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOSIZE},
      {1,WM_WINDOWPOSCHANGED,  SENT, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOSIZE | SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
      {2,WM_WINDOWPOSCHANGED,  SENT, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOSIZE | SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
      {0,0}};

static void Test_DWP_SimpleMsg(HWND hWnd1, HWND hWnd2)
{
    HDWP hdwp;
    BOOL ret;

    SetWindowPos(hWnd1, 0, 10,20,100,100,0);
    SetWindowPos(hWnd2, 0, 10,20,100,100,0);
    FlushMessages();
    EMPTY_CACHE();

    /* move hWnd1 */
    hdwp = BeginDeferWindowPos(1);
    ok(hdwp != NULL, "BeginDeferWindowPos failed\n");
    hdwp = DeferWindowPos(hdwp, hWnd1, HWND_TOP, 20, 30, 100, 100, SWP_NOACTIVATE|SWP_NOOWNERZORDER);
    ok(hdwp != NULL, "DeferWindowPos failed\n");
    FlushMessages();
    COMPARE_CACHE(empty_chain);
    ret = EndDeferWindowPos(hdwp);
    ok(ret != 0, "EndDeferWindowPos failed\n");
    FlushMessages();
    COMPARE_CACHE(move1_chain);

    /* resize hWnd1 */
    hdwp = BeginDeferWindowPos(1);
    ok(hdwp != NULL, "BeginDeferWindowPos failed\n");
    hdwp = DeferWindowPos(hdwp, hWnd1, HWND_TOP, 20, 30, 110, 110, SWP_NOACTIVATE|SWP_NOOWNERZORDER);
    ok(hdwp != NULL, "DeferWindowPos failed\n");
    FlushMessages();
    COMPARE_CACHE(empty_chain);
    ret = EndDeferWindowPos(hdwp);
    ok(ret != 0, "EndDeferWindowPos failed\n");
    FlushMessages();
    COMPARE_CACHE(resize1_chain);

    /* move both windows */
    hdwp = BeginDeferWindowPos(1);
    ok(hdwp != NULL, "BeginDeferWindowPos failed\n");
    hdwp = DeferWindowPos(hdwp, hWnd1, HWND_TOP, 30, 40, 110, 110, SWP_NOACTIVATE|SWP_NOOWNERZORDER);
    ok(hdwp != NULL, "DeferWindowPos failed\n");
    hdwp = DeferWindowPos(hdwp, hWnd2, HWND_TOP, 30, 40, 100, 100, SWP_NOACTIVATE|SWP_NOOWNERZORDER);
    ok(hdwp != NULL, "DeferWindowPos failed\n");
    FlushMessages();
    COMPARE_CACHE(empty_chain);
    ret = EndDeferWindowPos(hdwp);
    ok(ret != 0, "EndDeferWindowPos failed\n");
    FlushMessages();
    COMPARE_CACHE(move1_2_chain);

    /* change the z-order of the first window */
    hdwp = BeginDeferWindowPos(1);
    ok(hdwp != NULL, "BeginDeferWindowPos failed\n");
    hdwp = DeferWindowPos(hdwp, hWnd1, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOSIZE);
    ok(hdwp != NULL, "DeferWindowPos failed\n");
    FlushMessages();
    COMPARE_CACHE(empty_chain);
    ret = EndDeferWindowPos(hdwp);
    ok(ret != 0, "EndDeferWindowPos failed\n");
    FlushMessages();
    COMPARE_CACHE(ZOrder1_chain);

    /* change the z-order of both windows */
    hdwp = BeginDeferWindowPos(2);
    ok(hdwp != NULL, "BeginDeferWindowPos failed\n");
    hdwp = DeferWindowPos(hdwp, hWnd1, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOSIZE);
    ok(hdwp != NULL, "DeferWindowPos failed\n");
    hdwp = DeferWindowPos(hdwp, hWnd2, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOSIZE);
    ok(hdwp != NULL, "DeferWindowPos failed\n");
    FlushMessages();
    COMPARE_CACHE(empty_chain);
    ret = EndDeferWindowPos(hdwp);
    ok(ret != 0, "EndDeferWindowPos failed\n");
    FlushMessages();
    COMPARE_CACHE(ZOrder1and2_chain);

}

#define OwnerZOrderAParams SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOSIZE

MSG_ENTRY OwnerZOrder1A_chain[]={
      {4,WM_WINDOWPOSCHANGING, SENT,0, OwnerZOrderAParams},
      {3,WM_WINDOWPOSCHANGING, SENT,4, OwnerZOrderAParams},
      {1,WM_WINDOWPOSCHANGING, SENT,3, OwnerZOrderAParams},
      {0,0}};

MSG_ENTRY OwnerZOrder2A_chain[]={
      {2,WM_WINDOWPOSCHANGING, SENT, 0, OwnerZOrderAParams},
      {2,WM_WINDOWPOSCHANGED , SENT, 0, OwnerZOrderAParams | SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
      {0,0}};

MSG_ENTRY OwnerZOrder3A_chain[]={
      {3,WM_WINDOWPOSCHANGING, SENT, 0, OwnerZOrderAParams},
      {4,WM_WINDOWPOSCHANGING, SENT, 0, OwnerZOrderAParams},
      {1,WM_WINDOWPOSCHANGING, SENT, 4, OwnerZOrderAParams},
      {3,WM_WINDOWPOSCHANGED,  SENT, 0, OwnerZOrderAParams |SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
      {4,WM_WINDOWPOSCHANGED,  SENT, 0, OwnerZOrderAParams |SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
      {1,WM_WINDOWPOSCHANGED,  SENT, 4, OwnerZOrderAParams |SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
      {0,0}};

MSG_ENTRY OwnerZOrder4A_chain[]={
      {3,WM_WINDOWPOSCHANGING, SENT, 0 ,OwnerZOrderAParams},
      {4,WM_WINDOWPOSCHANGING, SENT, 0, OwnerZOrderAParams} ,
      {1,WM_WINDOWPOSCHANGING, SENT, 4, OwnerZOrderAParams},
      {0,0}};

MSG_ENTRY OwnerZOrder5A_chain[]={
      {4,WM_WINDOWPOSCHANGING, SENT, 0, OwnerZOrderAParams},
      {3,WM_WINDOWPOSCHANGING, SENT, 4, OwnerZOrderAParams},
      {1,WM_WINDOWPOSCHANGING, SENT, 3, OwnerZOrderAParams},
      {4,WM_WINDOWPOSCHANGED,  SENT, 0, OwnerZOrderAParams|SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
      {0,0}};

#define OwnerZOrderBParams SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOOWNERZORDER

MSG_ENTRY OwnerZOrder1B_chain[]={
      {1,WM_WINDOWPOSCHANGING, SENT, 0, OwnerZOrderBParams},
      {1,WM_WINDOWPOSCHANGED,  SENT, 0, OwnerZOrderBParams | SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
      {0,0}};

MSG_ENTRY OwnerZOrder2B_chain[]={
      {2,WM_WINDOWPOSCHANGING, SENT, 0, OwnerZOrderBParams},
      {2,WM_WINDOWPOSCHANGED,  SENT, 0, OwnerZOrderBParams | SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
      {0,0}};

MSG_ENTRY OwnerZOrder3B_chain[]={
      {3,WM_WINDOWPOSCHANGING, SENT, 0, OwnerZOrderBParams},
      {3,WM_WINDOWPOSCHANGED,  SENT, 0, OwnerZOrderBParams | SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
      {0,0}};

MSG_ENTRY OwnerZOrder4B_chain[]={
      {1,WM_WINDOWPOSCHANGING, SENT, 0, OwnerZOrderBParams},
      {1,WM_WINDOWPOSCHANGED,  SENT, 0, OwnerZOrderBParams | SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
      {0,0}};

MSG_ENTRY OwnerZOrder5B_chain[]={
      {4,WM_WINDOWPOSCHANGING, SENT, 0, OwnerZOrderBParams},
      {4,WM_WINDOWPOSCHANGED,  SENT, 0, OwnerZOrderBParams | SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE},
      {0,0}};

static void Test_DWP_OwnerZOrder()
{
    EXPECT_CHAIN(4,3,1,2);

    /* test how SetWindowPos can change the z order of owner windows */
    /* note that SWP_NOACTIVATE must be used because otherwise
       SetWindowPos will call SetWindowPos again with different parameters */
    SetWindowPos(hWnd1, 0, 0,0,0,0, OwnerZOrderAParams);
    FlushMessages();
    COMPARE_CACHE(OwnerZOrder1A_chain);
    EXPECT_CHAIN(4,3,1,2);

    SetWindowPos(hWnd2, 0, 0,0,0,0, OwnerZOrderAParams);
    FlushMessages();
    COMPARE_CACHE(OwnerZOrder2A_chain);
    EXPECT_CHAIN(2,4,3,1);

    SetWindowPos(hWnd3, 0, 0,0,0,0, OwnerZOrderAParams);
    FlushMessages();
    COMPARE_CACHE(OwnerZOrder3A_chain);
    EXPECT_CHAIN(3,4,1,2);

    SetWindowPos(hWnd1, 0, 0,0,0,0, OwnerZOrderAParams);
    FlushMessages();
    COMPARE_CACHE(OwnerZOrder4A_chain);
    EXPECT_CHAIN(3,4,1,2);

    SetWindowPos(hWnd4, 0, 0,0,0,0, OwnerZOrderAParams);
    FlushMessages();
    COMPARE_CACHE(OwnerZOrder5A_chain);
    EXPECT_CHAIN(4,3,1,2);

    /* now do the same thing one more time with SWP_NOOWNERZORDER */
    /* SWP_NOACTIVATE is needed because without it SetActiveWindow
       will be calledit and it will call SetWindowPos again
       WITHOUT SWP_NOOWNERZORDER. that means that
       in order for SWP_NOOWNERZORDER to have effect we have to use
       SWP_NOACTIVATE as well */
    set_default_pos();
    EXPECT_CHAIN(4,3,2,1);

    SetWindowPos(hWnd1, 0, 0,0,0,0, OwnerZOrderBParams);
    FlushMessages();
    COMPARE_CACHE(OwnerZOrder1B_chain);
    EXPECT_CHAIN(1,4,3,2);

    SetWindowPos(hWnd2, 0, 0,0,0,0, OwnerZOrderBParams);
    FlushMessages();
    COMPARE_CACHE(OwnerZOrder2B_chain);
    EXPECT_CHAIN(2,1,4,3);

    SetWindowPos(hWnd3, 0, 0,0,0,0, OwnerZOrderBParams);
    FlushMessages();
    COMPARE_CACHE(OwnerZOrder3B_chain);
    EXPECT_CHAIN(3,2,1,4);

    SetWindowPos(hWnd1, 0, 0,0,0,0, OwnerZOrderBParams);
    FlushMessages();
    COMPARE_CACHE(OwnerZOrder4B_chain);
    EXPECT_CHAIN(1,3,2,4);

    SetWindowPos(hWnd4, 0, 0,0,0,0, OwnerZOrderBParams);
    FlushMessages();
    COMPARE_CACHE(OwnerZOrder5B_chain);
    EXPECT_CHAIN(4,1,3,2);

}

START_TEST(DeferWindowPos)
{
    SetCursorPos(0,0);

    RegisterSimpleClass(DWPTestProc, L"ownertest");
    hWnd1 = CreateWindowExW(0, L"ownertest", L"abc", 0, 10, 20,
        200, 210, NULL, NULL, 0, NULL);
    hWnd2 = CreateWindowExW(0, L"ownertest", L"def", 0, 30, 40,
        220, 230, NULL, NULL, 0, NULL);
    hWnd3 = CreateWindowW(L"ownertest", L"ownertest", WS_OVERLAPPEDWINDOW,
        20, 350, 300, 300, hWnd1, NULL, 0, NULL);
    hWnd4 = CreateWindowW(L"ownertest", L"ownertest", WS_OVERLAPPEDWINDOW,
        250, 250, 200, 200, hWnd1, NULL, 0, NULL);

    ok(hWnd1 != NULL, "CreateWindow failed\n");
    ok(hWnd2 != NULL, "CreateWindow failed\n");
    ok(hWnd3 != NULL, "CreateWindow failed\n");
    ok(hWnd4 != NULL, "CreateWindow failed\n");

    set_default_pos();
    Test_DWP_Error(hWnd1, hWnd2);

    set_default_pos();
    Test_DWP_SimpleMsg(hWnd1, hWnd2);

    set_default_pos();
    Test_DWP_OwnerZOrder();

    DestroyWindow(hWnd4);
    DestroyWindow(hWnd3);
    DestroyWindow(hWnd2);
    DestroyWindow(hWnd1);
 }
