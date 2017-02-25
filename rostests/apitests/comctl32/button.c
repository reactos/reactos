/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for Button window class v6
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include "wine/test.h"
#include <windows.h>
#include <commctrl.h>

#define ok_rect(rc, l,r,t,b) ok((rc.left == (l)) && (rc.right == (r)) && (rc.top == (t)) && (rc.bottom == (b)), "Wrong rect. expected %d, %d, %d, %d got %ld, %ld, %ld, %ld\n", l,t,r,b, rc.left, rc.top, rc.right, rc.bottom)

void Test_TextMargin()
{
    RECT rc;
    BOOL ret;
    HWND hwnd1;
    
    hwnd1 = CreateWindowW(L"Button", L"Test1", 0, 10, 10, 100, 100, 0, NULL, NULL, NULL);
    ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");
    
    ret = SendMessageW(hwnd1, BCM_GETTEXTMARGIN, 0, (LPARAM)&rc);
    ok (ret == TRUE, "Expected BCM_GETTEXTMARGIN to succeed\n");
    ok_rect(rc, 1, 1, 1, 1);
    
    SetRect(&rc, 0,0,0,0);
    ret = SendMessageW(hwnd1, BCM_SETTEXTMARGIN, 0, (LPARAM)&rc);
    ok (ret == TRUE, "Expected BCM_SETTEXTMARGIN to succeed\n");
    
    ret = SendMessageW(hwnd1, BCM_GETTEXTMARGIN, 0, (LPARAM)&rc);
    ok (ret == TRUE, "Expected BCM_GETTEXTMARGIN to succeed\n");
    ok_rect(rc, 0, 0, 0, 0);
    
    SetRect(&rc, -1,-1,-1,-1);
    ret = SendMessageW(hwnd1, BCM_SETTEXTMARGIN, 0, (LPARAM)&rc);
    ok (ret == TRUE, "Expected BCM_SETTEXTMARGIN to succeed\n");
    
    ret = SendMessageW(hwnd1, BCM_GETTEXTMARGIN, 0, (LPARAM)&rc);
    ok (ret == TRUE, "Expected BCM_GETTEXTMARGIN to succeed\n");
    ok_rect(rc, -1, -1, -1, -1);

    SetRect(&rc, 1000,1000,1000,1000);
    ret = SendMessageW(hwnd1, BCM_SETTEXTMARGIN, 0, (LPARAM)&rc);
    ok (ret == TRUE, "Expected BCM_SETTEXTMARGIN to succeed\n");
    
    ret = SendMessageW(hwnd1, BCM_GETTEXTMARGIN, 0, (LPARAM)&rc);
    ok (ret == TRUE, "Expected BCM_GETTEXTMARGIN to succeed\n");
    ok_rect(rc, 1000, 1000, 1000, 1000);
    
    DestroyWindow(hwnd1);
}

void Test_Imagelist()
{
    HWND hwnd1;
    BOOL ret;
    BUTTON_IMAGELIST imlData;

    hwnd1 = CreateWindowW(L"Button", L"Test2", 0, 10, 10, 100, 100, 0, NULL, NULL, NULL);
    ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");

    ret = SendMessageW(hwnd1, BCM_GETIMAGELIST, 0, (LPARAM)&imlData);
    ok (ret == TRUE, "Expected BCM_GETIMAGELIST to succeed\n");
    ok (imlData.himl == 0, "Expected 0 himl\n");
    ok (imlData.uAlign == 0, "Expected 0 uAlign\n");
    ok_rect(imlData.margin, 0, 0, 0, 0);
    
    SetRect(&imlData.margin, 0,0,0,1);
    ret = SendMessageW(hwnd1, BCM_SETIMAGELIST, 0, (LPARAM)&imlData);
    ok (ret == FALSE, "Expected BCM_SETIMAGELIST to fail\n"); /* This works in win10 */

    imlData.himl = (HIMAGELIST)0xdead;
    ret = SendMessageW(hwnd1, BCM_SETIMAGELIST, 0, (LPARAM)&imlData);
    ok (ret == TRUE, "Expected BCM_SETIMAGELIST to fail\n"); /* This works in win10 */
    
    ret = SendMessageW(hwnd1, BCM_GETIMAGELIST, 0, (LPARAM)&imlData);
    ok (ret == TRUE, "Expected BCM_GETIMAGELIST to succeed\n");
    ok (imlData.himl == (HIMAGELIST)0xdead, "Expected 0 himl\n");
    ok (imlData.uAlign == 0, "Expected 0 uAlign\n");
    ok_rect(imlData.margin, 0, 0, 0, 1);
}

START_TEST(button)
{
    LoadLibraryW(L"comctl32.dll"); /* same as statically linking to comctl32 and doing InitCommonControls */
    Test_TextMargin();
    Test_Imagelist();
}

