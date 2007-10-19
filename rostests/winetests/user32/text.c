/*
 * DrawText tests
 *
 * Copyright (c) 2004 Zach Gorman
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA
 */

#include <assert.h>

#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"


static void test_DrawTextCalcRect(void)
{
    HWND hwnd;
    HDC hdc;
    HFONT hFont, hOldFont;
    LOGFONTA lf;
    const char text[] = "Example text for testing DrawText in "
      "MM_HIENGLISH mode";
    const WCHAR textW[] = {'W','i','d','e',' ','c','h','a','r',' ',
        's','t','r','i','n','g','\0'};
    const WCHAR emptystringW[] = { 0 };
    INT textlen,textheight;
    RECT rect = { 0, 0, 100, 0 };
    BOOL ret;

    /* Initialization */
    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP,
                           0, 0, 200, 200, 0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExA error %lu\n", GetLastError());
    hdc = GetDC(hwnd);
    ok(hdc != 0, "GetDC error %lu\n", GetLastError());
    trace("hdc %p\n", hdc);
    textlen = lstrlenA(text);

    /* LOGFONT initialization */
    memset(&lf, 0, sizeof(lf));
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfWeight = FW_DONTCARE;
    lf.lfHeight = 0; /* mapping mode dependent */
    lf.lfQuality = DEFAULT_QUALITY;
    lstrcpyA(lf.lfFaceName, "Arial");

    /* DrawText in MM_HIENGLISH with DT_CALCRECT */
    SetMapMode(hdc, MM_HIENGLISH);
    lf.lfHeight = 100 * 9 / 72; /* 9 point */
    hFont = CreateFontIndirectA(&lf);
    ok(hFont != 0, "CreateFontIndirectA error %lu\n",
       GetLastError());
    hOldFont = SelectObject(hdc, hFont);

    textheight = DrawTextA(hdc, text, textlen, &rect, DT_CALCRECT |
       DT_EXTERNALLEADING | DT_WORDBREAK | DT_NOCLIP | DT_LEFT |
       DT_NOPREFIX);
    ok( textheight, "DrawTextA error %lu\n", GetLastError());

    trace("MM_HIENGLISH rect.bottom %ld\n", rect.bottom);
    todo_wine ok(rect.bottom < 0, "In MM_HIENGLISH, DrawText with "
       "DT_CALCRECT should return a negative rectangle bottom. "
       "(bot=%ld)\n", rect.bottom);

    SelectObject(hdc, hOldFont);
    ret = DeleteObject(hFont);
    ok( ret, "DeleteObject error %lu\n", GetLastError());


    /* DrawText in MM_TEXT with DT_CALCRECT */
    SetMapMode(hdc, MM_TEXT);
    lf.lfHeight = -MulDiv(9, GetDeviceCaps(hdc,
       LOGPIXELSY), 72); /* 9 point */
    hFont = CreateFontIndirectA(&lf);
    ok(hFont != 0, "CreateFontIndirectA error %lu\n",
       GetLastError());
    hOldFont = SelectObject(hdc, hFont);

    textheight = DrawTextA(hdc, text, textlen, &rect, DT_CALCRECT |
       DT_EXTERNALLEADING | DT_WORDBREAK | DT_NOCLIP | DT_LEFT |
       DT_NOPREFIX);
    ok( textheight, "DrawTextA error %lu\n", GetLastError());

    trace("MM_TEXT rect.bottom %ld\n", rect.bottom);
    ok(rect.bottom > 0, "In MM_TEXT, DrawText with DT_CALCRECT "
       "should return a positive rectangle bottom. (bot=%ld)\n",
       rect.bottom);

    /* empty or null text should in some cases calc an empty rectangle */
    /* note: testing the function's return value is useless, it differs
     * ( 0 or 1) on every Windows version I tried */
    SetRect( &rect, 10,10, 100, 100);
    textheight = DrawTextExA(hdc, (char *) text, 0, &rect, DT_CALCRECT, NULL );
    ok( !(rect.left == rect.right && rect.bottom == rect.top),
            "rectangle should NOT be empty.\n");
    SetRect( &rect, 10,10, 100, 100);
    SetLastError( 0);
    textheight = DrawTextExA(hdc, "", -1, &rect, DT_CALCRECT, NULL );
    ok( (rect.left == rect.right && rect.bottom == rect.top),
            "rectangle should be empty.\n");
    SetRect( &rect, 10,10, 100, 100);
    SetLastError( 0);
    textheight = DrawTextExA(hdc, NULL, -1, &rect, DT_CALCRECT, NULL );
    ok( (rect.left == rect.right && rect.bottom == rect.top),
            "rectangle should be empty.\n");
    SetRect( &rect, 10,10, 100, 100);
    textheight = DrawTextExA(hdc, NULL, 0, &rect, DT_CALCRECT, NULL );
    ok( !(rect.left == rect.right && rect.bottom == rect.top),
            "rectangle should NOT be empty.\n");

    /* Wide char versions */
    SetRect( &rect, 10,10, 100, 100);
    SetLastError( 0);
    textheight = DrawTextExW(hdc, (WCHAR *) textW, 0, &rect, DT_CALCRECT, NULL );
    if( GetLastError() != ERROR_CALL_NOT_IMPLEMENTED) {
        ok( !(rect.left == rect.right && rect.bottom == rect.top),
                "rectangle should NOT be empty.\n");
        SetRect( &rect, 10,10, 100, 100);
        textheight = DrawTextExW(hdc, (WCHAR *) emptystringW, -1, &rect, DT_CALCRECT, NULL );
        ok( (rect.left == rect.right && rect.bottom == rect.top),
                "rectangle should be empty.\n");
        SetRect( &rect, 10,10, 100, 100);
        textheight = DrawTextExW(hdc, NULL, -1, &rect, DT_CALCRECT, NULL );
        ok( !(rect.left == rect.right && rect.bottom == rect.top),
                "rectangle should NOT be empty.\n");
        SetRect( &rect, 10,10, 100, 100);
        textheight = DrawTextExW(hdc, NULL, 0, &rect, DT_CALCRECT, NULL );
        ok( !(rect.left == rect.right && rect.bottom == rect.top),
                "rectangle should NOT be empty.\n");
    }

    SelectObject(hdc, hOldFont);
    ret = DeleteObject(hFont);
    ok( ret, "DeleteObject error %lu\n", GetLastError());

    /* Clean up */
    ret = ReleaseDC(hwnd, hdc);
    ok( ret, "ReleaseDC error %lu\n", GetLastError());
    ret = DestroyWindow(hwnd);
    ok( ret, "DestroyWindow error %lu\n", GetLastError());
}

/* replace tabs by \t */
static void strfmt( char *str, char *strout)
{
    unsigned int i,j ;
    for(i=0,j=0;i<=strlen(str);i++,j++)
        if((strout[j]=str[i])=='\t') {
            strout[j++]='\\';
            strout[j]='t';
        }
}


#define TABTEST( tabval, tabcount, string, _exp) \
{ int i,x_act, x_exp; char strdisp[64];\
    for(i=0;i<8;i++) tabs[i]=(i+1)*(tabval); \
    extent = GetTabbedTextExtentA( hdc, string, strlen( string), (tabcount), tabs); \
    strfmt( string, strdisp); \
 /*   trace( "Extent is %08lx\n", extent); */\
    x_act = LOWORD( extent); \
    x_exp = (_exp); \
    ok( x_act == x_exp, "Test case \"%s\". Text extent is %d, expected %d tab %d tabcount %d\n", \
        strdisp, x_act, x_exp, tabval, tabcount); \
} \


static void test_TabbedText(void)
{
    HWND hwnd;
    HDC hdc;
    BOOL ret;
    TEXTMETRICA tm;
    DWORD extent;
    INT tabs[8], cx, cy, tab, tabcount,t,align;

    /* Initialization */
    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP,
                           0, 0, 200, 200, 0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExA error %lu\n", GetLastError());
    hdc = GetDC(hwnd);
    ok(hdc != 0, "GetDC error %lu\n", GetLastError());

    ret = GetTextMetricsA( hdc, &tm);
    ok( ret, "GetTextMetrics error %lu\n", GetLastError());

    extent = GetTabbedTextExtentA( hdc, "x", 1, 1, tabs);
    cx = LOWORD( extent);
    cy = HIWORD( extent);
    trace( "cx is %d cy is %d\n", cx, cy);

    align=1;
    for( t=-1; t<=1; t++) { /* slightly adjust the 4 char tabstop, to
                               catch the one off errors */
        tab =  (cx *4 + t);
        /* test the special case tabcount =1 and the general array (80 of tabs */
        for( tabcount = 1; tabcount <= 8; tabcount +=7) {
            TABTEST( align * tab, tabcount, "\t", tab)
            TABTEST( align * tab, tabcount, "xxx\t", tab)
            TABTEST( align * tab, tabcount, "\tx", tab+cx)
            TABTEST( align * tab, tabcount, "\t\t", tab*2)
            TABTEST( align * tab, tabcount, "\tx\t", tab*2)
            TABTEST( align * tab, tabcount, "x\tx", tab+cx)
            TABTEST( align * tab, tabcount, "xx\tx", tab+cx)
            TABTEST( align * tab, tabcount, "xxx\tx", tab+cx)
            TABTEST( align * tab, tabcount, "xxxx\tx", t>0 ? tab + cx : 2*tab+cx)
            TABTEST( align * tab, tabcount, "xxxxx\tx", 2*tab+cx)
        }
    }
    align=-1;
    for( t=-1; t<=1; t++) { /* slightly adjust the 4 char tabstop, to
                               catch the one off errors */
        tab =  (cx *4 + t);
        /* test the special case tabcount =1 and the general array (8) of tabs */
        for( tabcount = 1; tabcount <= 8; tabcount +=7) {
            TABTEST( align * tab, tabcount, "\t", tab)
            TABTEST( align * tab, tabcount, "xxx\t", tab)
            TABTEST( align * tab, tabcount, "\tx", tab)
            TABTEST( align * tab, tabcount, "\t\t", tab*2)
            TABTEST( align * tab, tabcount, "\tx\t", tab*2)
            TABTEST( align * tab, tabcount, "x\tx", tab)
            TABTEST( align * tab, tabcount, "xx\tx", tab)
            TABTEST( align * tab, tabcount, "xxx\tx", 4 * cx >= tab ? 2*tab :tab)
            TABTEST( align * tab, tabcount, "xxxx\tx", 2*tab)
            TABTEST( align * tab, tabcount, "xxxxx\tx", 2*tab)
        }
    }


}

START_TEST(text)
{
    test_TabbedText();
    test_DrawTextCalcRect();
}
