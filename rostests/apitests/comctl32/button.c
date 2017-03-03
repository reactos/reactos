/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for Button window class v6
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include "wine/test.h"
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>

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

    hwnd1 = CreateWindowW(L"Button", L" ", 0, 10, 10, 100, 100, 0, NULL, NULL, NULL);
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


    hwnd1 = CreateWindowW(L"Button", L" ", BS_USERBUTTON, 10, 10, 100, 100, 0, NULL, NULL, NULL);
    ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");
    SetWindowTheme(hwnd1, L"", L"");

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, textent.cx + 5 + 2, 
               textent.cy + 7 + 2); /* the last +2 is the text margin */

    DestroyWindow(hwnd1);



    hwnd1 = CreateWindowW(L"Button", L"", 0, 10, 10, 100, 100, 0, NULL, NULL, NULL);
    ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");
    SetWindowTheme(hwnd1, L"", L"");

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, 123, 100);

    s.cx = 1;
    s.cy = 1;
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, 123, 100);

    hbmp = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(5), IMAGE_BITMAP, 0, 0, 0);
    ok (hbmp != 0, "Expected LoadImage to succeed\n");

    SendMessageW(hwnd1, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp);

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, 123, 100);

    himl = ImageList_LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(5), 1, 1, 0, IMAGE_BITMAP, 0);
    ok (himl != 0, "Expected ImageList_LoadImage to succeed\n");

    memset(&imlData, 0, sizeof(imlData));
    imlData.himl = himl;
    ret = SendMessageW(hwnd1, BCM_SETIMAGELIST, 0, (LPARAM)&imlData);
    ok (ret == TRUE, "Expected BCM_SETIMAGELIST to fail\n"); /* This works in win10 */

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, 123, 100);

    DestroyWindow(hwnd1);





    hwnd1 = CreateWindowW(L"Button", L"", 0, 10, 10, 5, 5, 0, NULL, NULL, NULL);
    ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");
    SetWindowTheme(hwnd1, L"", L"");

    memset(&s, 0, sizeof(s));
    ret = SendMessageW(hwnd1, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
    ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
    ok_size(s, 123, 34);

    DestroyWindow(hwnd1);




    hwnd1 = CreateWindowW(L"Button", L" ", BS_BITMAP , 10, 10, 100, 100, 0, NULL, NULL, NULL);
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



    hwnd1 = CreateWindowW(L"Button", L" ", 0, 10, 10, 100, 100, 0, NULL, NULL, NULL);
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






    hwnd1 = CreateWindowW(L"Button", L"Start", BS_VCENTER, 0, 0, 0, 0, 0, NULL, NULL, NULL);
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
    hwnd1 = CreateWindowW(L"Button", L"Some test text", 0, 10, 10, 100, 100, 0, NULL, NULL, NULL);
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

    hwnd1 = CreateWindowW(L"Static", L"", 0, 0, 0, 100, 100, 0, NULL, NULL, NULL);
    ok (hwnd1 != NULL, "Expected CreateWindowW to succeed\n");

    for (i = BS_CHECKBOX; i <= BS_OWNERDRAW; i++)
    {
        if (i == BS_USERBUTTON)
            continue;

        hwnd2 = CreateWindowW(L"Button", L" ", i, 0, 0, 72, 72, hwnd1, NULL, NULL, NULL);
        ok (hwnd2 != NULL, "Expected CreateWindowW to succeed\n");
        memset(&s, 0, sizeof(s));
        ret = SendMessageW(hwnd2, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
        ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
        ok_size(s, 123, 72);

        SetWindowTheme(hwnd2, L"", L"");
        memset(&s, 0, sizeof(s));
        ret = SendMessageW(hwnd2, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
        ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
        ok_size(s, 123, 72);
        DestroyWindow(hwnd2);

        hwnd2 = CreateWindowW(L"Button", L" ", i, 0, 0, 12, 12, hwnd1, NULL, NULL, NULL);
        ok (hwnd2 != NULL, "Expected CreateWindowW to succeed\n");
        memset(&s, 0, sizeof(s));
        ret = SendMessageW(hwnd2, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
        ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
        ok_size(s, 123, 34);
        DestroyWindow(hwnd2);

        hwnd2 = CreateWindowW(L"Button", L"", i, 0, 0, 72, 72, hwnd1, NULL, NULL, NULL);
        ok (hwnd2 != NULL, "Expected CreateWindowW to succeed\n");
        memset(&s, 0, sizeof(s));
        ret = SendMessageW(hwnd2, BCM_GETIDEALSIZE, 0, (LPARAM)&s);
        ok (ret == TRUE, "Expected BCM_GETIDEALSIZE to succeed\n");
        ok_size(s, 123, 72);
        DestroyWindow(hwnd2);
    }
    DestroyWindow(hwnd1);
}

START_TEST(button)
{
    LoadLibraryW(L"comctl32.dll"); /* same as statically linking to comctl32 and doing InitCommonControls */
    Test_TextMargin();
    Test_Imagelist();
    Test_GetIdealSizeNoThemes();
}

