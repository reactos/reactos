/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for Button window class v6
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include "wine/test.h"
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <undocuser.h>
#include <msgtrace.h>
#include <user32testhelpers.h>

#define ok_rect(rc, l,r,t,b) ok((rc.left == (l)) && (rc.right == (r)) && (rc.top == (t)) && (rc.bottom == (b)), "Wrong rect. expected %d, %d, %d, %d got %ld, %ld, %ld, %ld\n", l,t,r,b, rc.left, rc.top, rc.right, rc.bottom)
#define ok_size(s, width, height) ok((s.cx == (width) && s.cy == (height)), "Expected size (%lu,%lu) got (%lu,%lu)\n", (LONG)width, (LONG)height, s.cx, s.cy)

void Test_TextMargin()
{
    RECT rc;
    BOOL ret;
    HWND hwnd1;

    hwnd1 = CreateWindowW(L"Button", L"Test1", 0, 10, 10, 100, 100, 0, NULL, NULL, NULL);
    ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");
    SetWindowTheme(hwnd1, L"", L"");

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

    hwnd1 = CreateWindowW(L"Button", L"Test1", BS_DEFPUSHBUTTON, 10, 10, 100, 100, 0, NULL, NULL, NULL);
    ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");
    SetWindowTheme(hwnd1, L"", L"");

    ret = SendMessageW(hwnd1, BCM_GETTEXTMARGIN, 0, (LPARAM)&rc);
    ok (ret == TRUE, "Expected BCM_GETTEXTMARGIN to succeed\n");
    ok_rect(rc, 1, 1, 1, 1);

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
    ok (ret == TRUE, "Expected BCM_SETIMAGELIST to fail\n");

    ret = SendMessageW(hwnd1, BCM_GETIMAGELIST, 0, (LPARAM)&imlData);
    ok (ret == TRUE, "Expected BCM_GETIMAGELIST to succeed\n");
    ok (imlData.himl == (HIMAGELIST)0xdead, "Expected 0 himl\n");
    ok (imlData.uAlign == 0, "Expected 0 uAlign\n");
    ok_rect(imlData.margin, 0, 0, 0, 1);

    imlData.himl = 0;
    ret = SendMessageW(hwnd1, BCM_SETIMAGELIST, 0, (LPARAM)&imlData);
    ok (ret == FALSE, "Expected BCM_SETIMAGELIST to fail\n"); /* This works in win10 */

    DestroyWindow(hwnd1);
}

void Test_GetIdealSizeNoThemes()
{
    HWND hwnd1, hwnd2;
    BOOL ret;
    SIZE s, textent;
    HFONT font;
    HDC hdc;
    HANDLE hbmp;
    HIMAGELIST himl;
    BUTTON_IMAGELIST imlData;
    RECT rc;
    LOGFONTW lf;
    DWORD i;

    hwnd2 = CreateWindowW(L"Static", L"", 0, 0, 0, 100, 100, 0, NULL, NULL, NULL);
    ok (hwnd2 != NULL, "Expected CreateWindowW to succeed\n");

    hwnd1 = CreateWindowW(L"Button", L" ", WS_CHILD, 10, 10, 100, 100, hwnd2, NULL, NULL, NULL);
    ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");
    SetWindowTheme(hwnd1, L"", L"");

    font = (HFONT)SendMessageW(hwnd1, WM_GETFONT, 0, 0);
    hdc = GetDC(hwnd1);
    SelectObject(hdc, font);
    GetTextExtentPoint32W(hdc, L" ", 1, &textent);

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, textent.cx + 5 + 2,
               textent.cy + 7 + 2); /* the last +2 is the text margin */

    DestroyWindow(hwnd1);


    hwnd1 = CreateWindowW(L"Button", L" ", BS_USERBUTTON | WS_CHILD, 10, 10, 100, 100, hwnd2, NULL, NULL, NULL);
    ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");
    SetWindowTheme(hwnd1, L"", L"");

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, textent.cx + 5 + 2,
               textent.cy + 7 + 2); /* the last +2 is the text margin */

    DestroyWindow(hwnd1);



    hwnd1 = CreateWindowW(L"Button", L"", WS_CHILD, 10, 10, 100, 100, hwnd2, NULL, NULL, NULL);
    ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");
    SetWindowTheme(hwnd1, L"", L"");

    s.cx = 1;
    s.cy = 1;
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, 100, 100);

    hbmp = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(5), IMAGE_BITMAP, 0, 0, 0);
    ok (hbmp != 0, "Expected LoadImage to succeed\n");

    SendMessageW(hwnd1, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp);

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, 100, 100);

    himl = ImageList_LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(5), 1, 1, 0, IMAGE_BITMAP, 0);
    ok (himl != 0, "Expected ImageList_LoadImage to succeed\n");

    memset(&imlData, 0, sizeof(imlData));
    imlData.himl = himl;
    ret = SendMessageW(hwnd1, BCM_SETIMAGELIST, 0, (LPARAM)&imlData);
    ok (ret == TRUE, "Expected BCM_SETIMAGELIST to fail\n"); /* This works in win10 */

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, 100, 100);

    DestroyWindow(hwnd1);





    hwnd1 = CreateWindowW(L"Button", L"",  WS_CHILD, 10, 10, 5, 5, hwnd2, NULL, NULL, NULL);
    ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");
    SetWindowTheme(hwnd1, L"", L"");

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, 5, 5);

    DestroyWindow(hwnd1);




    hwnd1 = CreateWindowW(L"Button", L" ", BS_BITMAP | WS_CHILD, 10, 10, 100, 100, hwnd2, NULL, NULL, NULL);
    ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");
    SetWindowTheme(hwnd1, L"", L"");

    SendMessageW(hwnd1, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp);

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");

    /* In xp and 2k3 the image is ignored, in vista+ its width is added to the text width */
    ok_size(s, textent.cx + 5 + 2,
               textent.cy + 7 + 2); /* the last +2 is the text margin */

    DestroyWindow(hwnd1);



    hwnd1 = CreateWindowW(L"Button", L" ", WS_CHILD, 10, 10, 100, 100, hwnd2, NULL, NULL, NULL);
    ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");
    SetWindowTheme(hwnd1, L"", L"");

    SetRect(&rc, 0,0,0,0);
    ret = SendMessageW(hwnd1, BCM_SETTEXTMARGIN, 0, (LPARAM)&rc);
    ok (ret == TRUE, "Expected BCM_SETTEXTMARGIN to succeed\n");

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, textent.cx + 5,
               textent.cy + 7);

    SetRect(&rc, 50,50,50,50);
    ret = SendMessageW(hwnd1, BCM_SETTEXTMARGIN, 0, (LPARAM)&rc);
    ok (ret == TRUE, "Expected BCM_SETTEXTMARGIN to succeed\n");

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, textent.cx + 5 + 100,
               textent.cy + 7 + 100);

    SetRect(&rc, 1,1,1,1);
    ret = SendMessageW(hwnd1, BCM_SETTEXTMARGIN, 0, (LPARAM)&rc);
    ok (ret == TRUE, "Expected BCM_SETTEXTMARGIN to succeed\n");

    ret = SendMessageW(hwnd1, BCM_SETIMAGELIST, 0, (LPARAM)&imlData);
    ok (ret == TRUE, "Expected BCM_SETIMAGELIST to fail\n");

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, textent.cx + 5 + 2 + 1, /* we get an extra pixel due to the iml */
               textent.cy + 7 + 2);

    s.cx = 1;
    s.cy = 1;
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, textent.cx + 5 + 2 + 1,
               textent.cy + 7 + 2);

    s.cx = 100;
    s.cy = 100;
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, textent.cx + 5 + 2 + 1,
               textent.cy + 7 + 2);

    SetRect(&imlData.margin, 1,1,1,1);
    ret = SendMessageW(hwnd1, BCM_SETIMAGELIST, 0, (LPARAM)&imlData);
    ok (ret == TRUE, "Expected BCM_SETIMAGELIST to fail\n");

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    /* expected width = text width + hardcoded value + text margins + image width + image margins */
    ok_size(s, textent.cx + 5 + 2 + 1 + 2,
               textent.cy + 7 + 2);

    SetRect(&imlData.margin, 50,50,50,50);
    ret = SendMessageW(hwnd1, BCM_SETIMAGELIST, 0, (LPARAM)&imlData);
    ok (ret == TRUE, "Expected BCM_SETIMAGELIST to fail\n");

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    /* image + its margins is so big that the height is dictated by them */
    ok_size(s, textent.cx + 5 + 2 + 1 + 100, (LONG)101);

    DestroyWindow(hwnd1);






    hwnd1 = CreateWindowW(L"Button", L"Start", BS_VCENTER | WS_CHILD, 0, 0, 0, 0, hwnd2, NULL, NULL, NULL);
    ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");
    SetWindowTheme(hwnd1, L"", L"");

    font = (HFONT)SendMessageW(hwnd1, WM_GETFONT, 0, 0);
    hdc = GetDC(hwnd1);
    SelectObject(hdc, font);
    GetTextExtentPoint32W(hdc, L"Start", 5, &textent);

    SetRect(&rc, 0,0,0,0);
    ret = SendMessageW(hwnd1, BCM_SETTEXTMARGIN, 0, (LPARAM)&rc);
    ok (ret == TRUE, "Expected BCM_SETTEXTMARGIN to succeed\n");

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, textent.cx + 5, textent.cy + 7);

    DestroyWindow(hwnd1);




    /* Test again with some real text to see if the formula is correct */
    hwnd1 = CreateWindowW(L"Button", L"Some test text", WS_CHILD, 10, 10, 100, 100, hwnd2, NULL, NULL, NULL);
    ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");
    SetWindowTheme(hwnd1, L"", L"");

    font = (HFONT)SendMessageW(hwnd1, WM_GETFONT, 0, 0);
    hdc = GetDC(hwnd1);
    SelectObject(hdc, font);
    GetTextExtentPoint32W(hdc, L"Some test text", 14, &textent);

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, textent.cx + 5 + 2,  /* the last +2 is the text margin */
               textent.cy + 7 + 2);

    /* The hardcoded values are independent of the margin */
    lf.lfHeight = 200;
    lf.lfWidth = 200;
    lf.lfWeight = FW_BOLD;
    wcscpy(lf.lfFaceName, L"Arial");
    font = CreateFontIndirectW(&lf);
    ok(font != NULL, "\n");
    SendMessageW(hwnd1, WM_SETFONT, (WPARAM)font, FALSE);

    SelectObject(hdc, font);
    GetTextExtentPoint32W(hdc, L"Some test text", 14, &textent);

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, textent.cx + 5 + 2,  /* the last +2 is the text margin */
               textent.cy + 7 + 2);

    DestroyWindow(hwnd1);

    for (i = BS_PUSHBUTTON; i <= BS_OWNERDRAW; i++)
    {
        if (i == BS_USERBUTTON)
            continue;

        if (i >= BS_CHECKBOX)
        {
            hwnd1 = CreateWindowW(L"Button", L" ", i|WS_CHILD, 0, 0, 72, 72, hwnd2, NULL, NULL, NULL);
            ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");
            memset(&s, 0, sizeof(s));
            ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
            ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
            ok_size(s, 72, 72);

            SetWindowTheme(hwnd1, L"", L"");
            memset(&s, 0, sizeof(s));
            ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
            ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
            ok_size(s, 72, 72);
            DestroyWindow(hwnd1);

            hwnd1 = CreateWindowW(L"Button", L" ", i|WS_CHILD, 0, 0, 12, 12, hwnd2, NULL, NULL, NULL);
            ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");
            memset(&s, 0, sizeof(s));
            ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
            ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
            ok_size(s, 12, 12);
            DestroyWindow(hwnd1);
        }

        hwnd1 = CreateWindowW(L"Button", L"", i|WS_CHILD, 0, 0, 72, 72, hwnd2, NULL, NULL, NULL);
        ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");
        memset(&s, 0, sizeof(s));
        ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
        ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
        ok_size(s, 72, 72);
        DestroyWindow(hwnd1);

        hwnd1 = CreateWindowW(L"Button", L"", i|WS_CHILD, 0, 0, 150, 72, hwnd2, NULL, NULL, NULL);
        ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");
        memset(&s, 0, sizeof(s));
        ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
        ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
        ok_size(s, 150, 72);
        DestroyWindow(hwnd1);
    }
    DestroyWindow(hwnd2);
}


HWND hWnd1, hWnd2;

#define MOVE_CURSOR(x,y) mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE ,                           \
                                     x*(65535/GetSystemMetrics(SM_CXVIRTUALSCREEN)),                     \
                                     y*(65535/GetSystemMetrics(SM_CYVIRTUALSCREEN)) , 0,0);

static int get_iwnd(HWND hWnd)
{
    if(hWnd == hWnd1) return 1;
    else if(hWnd == hWnd2) return 2;
    else return 0;
}

static LRESULT CALLBACK subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR ref_data)
{
    int iwnd = get_iwnd(hwnd);

    if(message > WM_USER || !iwnd )
        return DefSubclassProc(hwnd, message, wParam, lParam);

    switch(message)
    {
    case WM_IME_SETCONTEXT:
    case WM_IME_NOTIFY :
    case WM_GETICON :
    case WM_GETTEXT:
    case WM_GETTEXTLENGTH:
        break;
    case WM_NOTIFY:
    {
        NMHDR* pnmhdr = (NMHDR*)lParam;
        if (pnmhdr->code == NM_CUSTOMDRAW)
        {
            NMCUSTOMDRAW* pnmcd = (NMCUSTOMDRAW*)lParam;
            RECORD_MESSAGE(iwnd, message, SENT, pnmhdr->code, pnmcd->dwDrawStage);
        }
        else
        {
            RECORD_MESSAGE(iwnd, message, SENT, pnmhdr->idFrom,pnmhdr->code);
        }
        break;
    }
    default:
        RECORD_MESSAGE(iwnd, message, SENT, 0,0);
    }
    return DefSubclassProc(hwnd, message, wParam, lParam);
}

static LRESULT CALLBACK TestProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int iwnd = get_iwnd(hwnd);

    if (iwnd != 0  && message == WM_NOTIFY)
    {
        NMHDR* pnmhdr = (NMHDR*)lParam;
        if (pnmhdr->code == NM_CUSTOMDRAW)
        {
            NMCUSTOMDRAW* pnmcd = (NMCUSTOMDRAW*)lParam;
            RECORD_MESSAGE(iwnd, message, SENT, pnmhdr->code, pnmcd->dwDrawStage);
        }
        else
        {
            RECORD_MESSAGE(iwnd, message, SENT, pnmhdr->idFrom,pnmhdr->code);
        }
    }
    else if (iwnd != 0 && message < WM_USER && message != WM_GETICON)
    {
        RECORD_MESSAGE(iwnd, message, SENT, 0,0);
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

static void FlushMessages()
{
    MSG msg;

    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
    {
        int iwnd = get_iwnd(msg.hwnd);
        if(iwnd)
        {
            if(msg.message <= WM_USER && iwnd != 0)
                RECORD_MESSAGE(iwnd, msg.message, POST,0,0);
        }
        DispatchMessageW( &msg );
    }
}

MSG_ENTRY paint_sequence[]={
    {2, WM_PAINT, POST},
    {1, WM_ERASEBKGND},
    {1, WM_PRINTCLIENT},
    {1, WM_CTLCOLORBTN},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREERASE},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREPAINT},
    {0,0}};

MSG_ENTRY paint_nonthemed_sequence[]={
    {2, WM_PAINT, POST},
    {1, WM_CTLCOLORBTN},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREERASE},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREPAINT},
    {0,0}};

MSG_ENTRY redraw_sequence[]={
    {2, WM_PAINT, POST},
    {2, WM_ERASEBKGND},
    {1, WM_ERASEBKGND},
    {1, WM_PRINTCLIENT},
    {1, WM_CTLCOLORBTN},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREERASE},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREPAINT},
    {0,0}};

MSG_ENTRY redraw_nonthemed_sequence[]={
    {2, WM_PAINT, POST},
    {2, WM_ERASEBKGND},
    {1, WM_CTLCOLORBTN},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREERASE},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREPAINT},
    {0,0}};

MSG_ENTRY printclnt_nonthemed_sequence[]={
    {2, WM_PRINTCLIENT},
    {1, WM_CTLCOLORBTN},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREERASE},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREPAINT},
    {0,0}};

MSG_ENTRY printclnt_sequence[]={
    {2, WM_PRINTCLIENT},
    {1, WM_CTLCOLORBTN},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREERASE},
    {0,0}};

MSG_ENTRY pseudomove_sequence[]={
    {2, WM_MOUSEMOVE},
    {1, WM_NOTIFY, SENT, 0, BCN_HOTITEMCHANGE},
    {2, WM_MOUSELEAVE, POST},
    {1, WM_NOTIFY, SENT, 0, BCN_HOTITEMCHANGE},
    {2, WM_PAINT, POST},
    {2, WM_ERASEBKGND},
    {1, WM_ERASEBKGND},
    {1, WM_PRINTCLIENT},
    {1, WM_CTLCOLORBTN},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREERASE},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREPAINT},
    {0,0}};

MSG_ENTRY pseudomove_nonthemed_sequence[]={
    {2, WM_MOUSEMOVE},
    {1, WM_NOTIFY, SENT, 0, BCN_HOTITEMCHANGE},
    {2, WM_MOUSELEAVE, POST},
    {1, WM_NOTIFY, SENT, 0, BCN_HOTITEMCHANGE},
    {2, WM_PAINT, POST},
    {2, WM_ERASEBKGND},
    {1, WM_CTLCOLORBTN},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREERASE},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREPAINT},
    {0,0}};

MSG_ENTRY pseudohover_sequence[]={
    {2, WM_MOUSEHOVER},
    {0,0}};

MSG_ENTRY pseudoleave_sequence[]={
    {2, WM_MOUSELEAVE},
    {0,0}};

MSG_ENTRY mouseenter_sequence[]={
    {2, WM_NCHITTEST},
    {2, WM_SETCURSOR},
    {1, WM_SETCURSOR},
    {2, WM_MOUSEMOVE, POST},
    {1, WM_NOTIFY, SENT, 0, BCN_HOTITEMCHANGE},
    {2, WM_PAINT, POST},
    {2, WM_ERASEBKGND},
    {1, WM_ERASEBKGND},
    {1, WM_PRINTCLIENT},
    {1, WM_CTLCOLORBTN},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREERASE},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREPAINT},
    {0,0}};

MSG_ENTRY mouseenter_nonthemed_sequence[]={
    {2, WM_NCHITTEST},
    {2, WM_SETCURSOR},
    {1, WM_SETCURSOR},
    {2, WM_MOUSEMOVE, POST},
    {1, WM_NOTIFY, SENT, 0, BCN_HOTITEMCHANGE},
    {2, WM_PAINT, POST},
    {2, WM_ERASEBKGND},
    {1, WM_CTLCOLORBTN},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREERASE},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREPAINT},
    {0,0}};

MSG_ENTRY mousemove_sequence[]={
    {2, WM_NCHITTEST},
    {2, WM_SETCURSOR},
    {1, WM_SETCURSOR},
    {2, WM_MOUSEMOVE, POST},
    {0,0}};

MSG_ENTRY mouseleave_sequence[]={
    {2, WM_MOUSELEAVE, POST},
    {1, WM_NOTIFY, SENT, 0, BCN_HOTITEMCHANGE},
    {2, WM_PAINT, POST},
    {2, WM_ERASEBKGND},
    {1, WM_ERASEBKGND},
    {1, WM_PRINTCLIENT},
    {1, WM_CTLCOLORBTN},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREERASE},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREPAINT},
    {0,0}};

MSG_ENTRY mouseleave_nonthemed_sequence[]={
    {2, WM_MOUSELEAVE, POST},
    {1, WM_NOTIFY, SENT, 0, BCN_HOTITEMCHANGE},
    {2, WM_PAINT, POST},
    {2, WM_ERASEBKGND},
    {1, WM_CTLCOLORBTN},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREERASE},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREPAINT},
    {0,0}};

MSG_ENTRY themechanged_sequence[]={
    {2, WM_THEMECHANGED, SENT},
    {1, WM_NOTIFY, SENT, 0, NM_THEMECHANGED },
    {2, WM_PAINT, POST},
    {2, WM_ERASEBKGND},
    {1, WM_CTLCOLORBTN},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREERASE},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREPAINT},
    {0,0}};

MSG_ENTRY enable_nonthemed_sequence[]={
    {2, WM_ENABLE, SENT},
    {1, WM_CTLCOLORBTN},
    {1, WM_CTLCOLORBTN},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREERASE},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREPAINT},
    {0,0}};

MSG_ENTRY btndown_nonthemed_sequence[]={
    {2, WM_LBUTTONDOWN, SENT},
    {1, WM_KILLFOCUS, SENT},
    {1, WM_IME_SETCONTEXT, SENT},
    {2, WM_SETFOCUS, SENT},
    {2, BM_SETSTATE, SENT},
    {2, WM_PAINT, POST},
    {1, WM_CTLCOLORBTN},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREERASE},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREPAINT},
    {0,0}};

MSG_ENTRY btndown_repeat_nonthemed_sequence[]={
    {2, WM_LBUTTONDOWN, SENT},
    {2, BM_SETSTATE, SENT},
    {0,0}};

MSG_ENTRY btnclick_nonthemed_sequence[]={
    {2, BM_CLICK, SENT},
    {2, WM_LBUTTONDOWN, SENT},
    {2, BM_SETSTATE, SENT},
    {2, WM_LBUTTONUP, SENT},
    {2, BM_SETSTATE , SENT},
    {2, WM_CAPTURECHANGED, SENT},
    {1, WM_COMMAND, SENT},
    {2, WM_PAINT, POST},
    {1, WM_CTLCOLORBTN},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREERASE},
    {1, WM_NOTIFY, SENT, NM_CUSTOMDRAW, CDDS_PREPAINT},
    {0,0}};

MSG_ENTRY btnup_stray_sequence[]={
    {2, WM_LBUTTONUP, SENT},
    {0,0}};

void Test_MessagesNonThemed()
{
    DWORD state;

    MOVE_CURSOR(0,0);
    EMPTY_CACHE();

    RegisterSimpleClass(TestProc, L"testClass");
    hWnd1 = CreateWindowW(L"testClass", L"Test parent", WS_POPUP | WS_VISIBLE, 100, 100, 200, 200, 0, NULL, NULL, NULL);
    ok (hWnd1 != NULL, "Expected CreateWindowW to succeed\n");
    SetWindowTheme(hWnd1, L"", L"");
    UpdateWindow(hWnd1);

    hWnd2 = CreateWindowW(L"Button", L"test button", /*BS_RADIOBUTTON | */WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hWnd1, NULL, NULL, NULL);
    ok (hWnd2 != NULL, "Expected CreateWindowW to succeed\n");
    SetWindowTheme(hWnd2, L"", L"");
    SetWindowSubclass(hWnd2, subclass_proc, 0, 0);
    UpdateWindow(hWnd2);

    FlushMessages();
    EMPTY_CACHE();

    RedrawWindow(hWnd2, NULL, NULL, RDW_ERASE);
    FlushMessages();
    COMPARE_CACHE(empty_chain);
    FlushMessages();
    COMPARE_CACHE(empty_chain);

    RedrawWindow(hWnd2, NULL, NULL, RDW_FRAME);
    FlushMessages();
    COMPARE_CACHE(empty_chain);

    RedrawWindow(hWnd2, NULL, NULL, RDW_INTERNALPAINT);
    FlushMessages();
    COMPARE_CACHE(paint_nonthemed_sequence);

    RedrawWindow(hWnd2, NULL, NULL, RDW_INVALIDATE);
    FlushMessages();
    COMPARE_CACHE(paint_nonthemed_sequence);

    RedrawWindow(hWnd2, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
    FlushMessages();
    COMPARE_CACHE(redraw_nonthemed_sequence);

    SendMessageW(hWnd2, WM_PRINTCLIENT, 0, PRF_ERASEBKGND);
    FlushMessages();
    COMPARE_CACHE(printclnt_nonthemed_sequence);

    SendMessageW(hWnd2, WM_MOUSEMOVE, 0, 0);
    FlushMessages();
    COMPARE_CACHE(pseudomove_nonthemed_sequence);

    SendMessageW(hWnd2, WM_MOUSEHOVER, 0, 0);
    FlushMessages();
    COMPARE_CACHE(pseudohover_sequence);

    SendMessageW(hWnd2, WM_MOUSELEAVE, 0, 0);
    FlushMessages();
    COMPARE_CACHE(pseudoleave_sequence);

    state = SendMessageW(hWnd2, BM_GETSTATE,0,0);
    ok_hex(state, 0);
    EMPTY_CACHE();

    MOVE_CURSOR(150,150);
    FlushMessages();
    COMPARE_CACHE(mouseenter_nonthemed_sequence);

    state = SendMessageW(hWnd2, BM_GETSTATE,0,0);
    ok_hex(state, BST_HOT);
    EMPTY_CACHE();

    MOVE_CURSOR(151,151);
    FlushMessages();
    COMPARE_CACHE(mousemove_sequence);

    state = SendMessageW(hWnd2, BM_GETSTATE,0,0);
    ok_hex(state, BST_HOT);
    EMPTY_CACHE();

    MOVE_CURSOR(0,0);
    FlushMessages();
    COMPARE_CACHE(empty_chain);
    FlushMessages();
    COMPARE_CACHE(mouseleave_nonthemed_sequence);

    state = SendMessageW(hWnd2, BM_GETSTATE,0,0);
    ok(state == 0, "Expected state 0, got %lu\n", state);
    EMPTY_CACHE();

    SendMessageW(hWnd2, WM_THEMECHANGED, 1, 0);
    FlushMessages();
    COMPARE_CACHE(themechanged_sequence);

    SendMessageW(hWnd2, WM_ENABLE, TRUE, 0);
    FlushMessages();
    COMPARE_CACHE(enable_nonthemed_sequence);

    SendMessageW(hWnd2, WM_LBUTTONDOWN, 0, 0);
    FlushMessages();
    COMPARE_CACHE(btndown_nonthemed_sequence);

    state = SendMessageW(hWnd2, BM_GETSTATE,0,0);
    ok_hex(state, BST_PUSHED | BST_FOCUS | 0x20 | 0x40);
    EMPTY_CACHE();

    SendMessageW(hWnd2, WM_LBUTTONDOWN, 0, 0);
    FlushMessages();
    COMPARE_CACHE(btndown_repeat_nonthemed_sequence);

    state = SendMessageW(hWnd2, BM_GETSTATE,0,0);
    ok_hex(state, BST_PUSHED | BST_FOCUS | 0x20 | 0x40);
    EMPTY_CACHE();

    SendMessageW(hWnd2, BM_CLICK, 0, 0);
    FlushMessages();
    COMPARE_CACHE(btnclick_nonthemed_sequence);

    state = SendMessageW(hWnd2, BM_GETSTATE,0,0);
    ok_hex(state, BST_FOCUS);
    EMPTY_CACHE();

    SendMessageW(hWnd2, WM_LBUTTONUP, 0, 0);
    FlushMessages();
    COMPARE_CACHE(btnup_stray_sequence);

    state = SendMessageW(hWnd2, BM_GETSTATE,0,0);
    ok_hex(state, BST_FOCUS);
    EMPTY_CACHE();

    DestroyWindow(hWnd1);
    DestroyWindow(hWnd2);
}

void Test_MessagesThemed()
{
    DWORD state;

    MOVE_CURSOR(0,0);
    EMPTY_CACHE();

    RegisterSimpleClass(TestProc, L"testClass");
    hWnd1 = CreateWindowW(L"testClass", L"Test parent", WS_POPUP | WS_VISIBLE, 100, 100, 200, 200, 0, NULL, NULL, NULL);
    ok (hWnd1 != NULL, "Expected CreateWindowW to succeed\n");
    UpdateWindow(hWnd1);

    hWnd2 = CreateWindowW(L"Button", L"test button", /*BS_RADIOBUTTON | */WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hWnd1, NULL, NULL, NULL);
    ok (hWnd2 != NULL, "Expected CreateWindowW to succeed\n");
    SetWindowSubclass(hWnd2, subclass_proc, 0, 0);
    UpdateWindow(hWnd2);

    FlushMessages();
    EMPTY_CACHE();

    RedrawWindow(hWnd2, NULL, NULL, RDW_ERASE);
    FlushMessages();
    COMPARE_CACHE(empty_chain);
    FlushMessages();
    COMPARE_CACHE(empty_chain);

    RedrawWindow(hWnd2, NULL, NULL, RDW_FRAME);
    FlushMessages();
    COMPARE_CACHE(empty_chain);

    RedrawWindow(hWnd2, NULL, NULL, RDW_INTERNALPAINT);
    FlushMessages();
    COMPARE_CACHE(paint_sequence);

    RedrawWindow(hWnd2, NULL, NULL, RDW_INVALIDATE);
    FlushMessages();
    COMPARE_CACHE(paint_sequence);

    RedrawWindow(hWnd2, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
    FlushMessages();
    COMPARE_CACHE(redraw_sequence);

    SendMessageW(hWnd2, WM_PRINTCLIENT, 0, PRF_ERASEBKGND);
    FlushMessages();
    COMPARE_CACHE(printclnt_sequence);

    SendMessageW(hWnd2, WM_PRINTCLIENT, 0, PRF_CLIENT);
    FlushMessages();
    COMPARE_CACHE(printclnt_sequence);

    SendMessageW(hWnd2, WM_MOUSEMOVE, 0, 0);
    FlushMessages();
    COMPARE_CACHE(pseudomove_sequence);

    SendMessageW(hWnd2, WM_MOUSEHOVER, 0, 0);
    FlushMessages();
    COMPARE_CACHE(pseudohover_sequence);

    SendMessageW(hWnd2, WM_MOUSELEAVE, 0, 0);
    FlushMessages();
    COMPARE_CACHE(pseudoleave_sequence);

    state = SendMessageW(hWnd2, BM_GETSTATE,0,0);
    ok(state == 0, "Expected state 0, got %lu", state);
    EMPTY_CACHE();

    MOVE_CURSOR(150,150);
    FlushMessages();
    COMPARE_CACHE(mouseenter_sequence);

    state = SendMessageW(hWnd2, BM_GETSTATE,0,0);
    ok(state == BST_HOT, "Expected state BST_HOT, got %lu", state);
    EMPTY_CACHE();

    MOVE_CURSOR(151,151);
    FlushMessages();
    COMPARE_CACHE(mousemove_sequence);

    state = SendMessageW(hWnd2, BM_GETSTATE,0,0);
    ok(state == BST_HOT, "Expected state BST_HOT, got %lu", state);
    EMPTY_CACHE();

    MOVE_CURSOR(0,0);
    FlushMessages();
    COMPARE_CACHE(empty_chain);
    FlushMessages();
    COMPARE_CACHE(mouseleave_sequence);

    state = SendMessageW(hWnd2, BM_GETSTATE,0,0);
    ok(state == 0, "Expected state 0, got %lu", state);
    EMPTY_CACHE();

    DestroyWindow(hWnd1);
    DestroyWindow(hWnd2);
}

START_TEST(button)
{
    LoadLibraryW(L"comctl32.dll"); /* same as statically linking to comctl32 and doing InitCommonControls */
    Test_TextMargin();
    Test_Imagelist();
    Test_GetIdealSizeNoThemes();

    Test_MessagesNonThemed();
    if (IsThemeActive())
        Test_MessagesThemed();
    else
        skip("No active theme, skipping Test_MessagesThemed\n");

}

