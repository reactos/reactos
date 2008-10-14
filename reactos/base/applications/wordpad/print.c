/*
 * Wordpad implementation - Printing and print preview functions
 *
 * Copyright 2007-2008 by Alexander N. Sørnes <alex@thehandofagony.com>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <windows.h>
#include <richedit.h>
#include <commctrl.h>

#include "wordpad.h"

typedef struct _previewinfo
{
    int page;
    int pages;
    HDC hdc;
    HDC hdc2;
    HDC hdcSized;
    HDC hdcSized2;
    RECT window;
    LPWSTR wszFileName;
} previewinfo, *ppreviewinfo;

static HGLOBAL devMode;
static HGLOBAL devNames;

static RECT margins;
static previewinfo preview;

static const WCHAR var_pagemargin[] = {'P','a','g','e','M','a','r','g','i','n',0};

static LPWSTR get_print_file_filter(HWND hMainWnd)
{
    static WCHAR wszPrintFilter[MAX_STRING_LEN*2+6+4+1];
    const WCHAR files_prn[] = {'*','.','P','R','N',0};
    const WCHAR files_all[] = {'*','.','*','\0'};
    LPWSTR p;
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hMainWnd, GWLP_HINSTANCE);

    p = wszPrintFilter;
    LoadStringW(hInstance, STRING_PRINTER_FILES_PRN, p, MAX_STRING_LEN);
    p += lstrlenW(p) + 1;
    lstrcpyW(p, files_prn);
    p += lstrlenW(p) + 1;
    LoadStringW(hInstance, STRING_ALL_FILES, p, MAX_STRING_LEN);
    p += lstrlenW(p) + 1;
    lstrcpyW(p, files_all);
    p += lstrlenW(p) + 1;
    *p = 0;

    return wszPrintFilter;
}

void registry_set_pagemargins(HKEY hKey)
{
    RegSetValueExW(hKey, var_pagemargin, 0, REG_BINARY, (LPBYTE)&margins, sizeof(RECT));
}

void registry_read_pagemargins(HKEY hKey)
{
    DWORD size = sizeof(RECT);

    if(!hKey || RegQueryValueExW(hKey, var_pagemargin, 0, NULL, (LPBYTE)&margins,
                     &size) != ERROR_SUCCESS || size != sizeof(RECT))
    {
        margins.top = 1417;
        margins.bottom = 1417;
        margins.left = 1757;
        margins.right = 1757;
    }
}

static void AddTextButton(HWND hRebarWnd, UINT string, UINT command, UINT id)
{
    REBARBANDINFOW rb;
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hRebarWnd, GWLP_HINSTANCE);
    WCHAR text[MAX_STRING_LEN];
    HWND hButton;

    LoadStringW(hInstance, string, text, MAX_STRING_LEN);
    hButton = CreateWindowW(WC_BUTTONW, text,
                            WS_VISIBLE | WS_CHILD, 5, 5, 100, 15,
                            hRebarWnd, (HMENU)ULongToHandle(command), hInstance, NULL);

    rb.cbSize = sizeof(rb);
    rb.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_CHILD | RBBIM_IDEALSIZE | RBBIM_ID;
    rb.fStyle = RBBS_NOGRIPPER | RBBS_VARIABLEHEIGHT;
    rb.hwndChild = hButton;
    rb.cyChild = rb.cyMinChild = 22;
    rb.cx = rb.cxMinChild = 90;
    rb.cxIdeal = 100;
    rb.wID = id;

    SendMessageW(hRebarWnd, RB_INSERTBAND, -1, (LPARAM)&rb);
}

static HDC make_dc(void)
{
    if(devNames && devMode)
    {
        LPDEVNAMES dn = GlobalLock(devNames);
        LPDEVMODEW dm = GlobalLock(devMode);
        HDC ret;

        ret = CreateDCW((LPWSTR)dn + dn->wDriverOffset,
                         (LPWSTR)dn + dn->wDeviceOffset, 0, dm);

        GlobalUnlock(dn);
        GlobalUnlock(dm);

        return ret;
    } else
    {
        return 0;
    }
}

static LONG twips_to_centmm(int twips)
{
    return MulDiv(twips, CENTMM_PER_INCH, TWIPS_PER_INCH);
}

static LONG centmm_to_twips(int mm)
{
    return MulDiv(mm, TWIPS_PER_INCH, CENTMM_PER_INCH);
}

static LONG twips_to_pixels(int twips, int dpi)
{
    return MulDiv(twips, dpi, TWIPS_PER_INCH);
}

static LONG devunits_to_twips(int units, int dpi)
{
    return MulDiv(units, TWIPS_PER_INCH, dpi);
}


static RECT get_print_rect(HDC hdc)
{
    RECT rc;
    int width, height;

    if(hdc)
    {
        int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
        int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
        width = devunits_to_twips(GetDeviceCaps(hdc, PHYSICALWIDTH), dpiX);
        height = devunits_to_twips(GetDeviceCaps(hdc, PHYSICALHEIGHT), dpiY);
    } else
    {
        width = centmm_to_twips(18500);
        height = centmm_to_twips(27000);
    }

    rc.left = margins.left;
    rc.right = width - margins.right;
    rc.top = margins.top;
    rc.bottom = height - margins.bottom;

    return rc;
}

void target_device(HWND hMainWnd, DWORD wordWrap)
{
    HWND hEditorWnd = GetDlgItem(hMainWnd, IDC_EDITOR);

    if(wordWrap == ID_WORDWRAP_MARGIN)
    {
        int width = 0;
        LRESULT result;
        HDC hdc = make_dc();
        RECT rc = get_print_rect(hdc);

        width = rc.right - rc.left;
        if(!hdc)
        {
            HDC hMaindc = GetDC(hMainWnd);
            hdc = CreateCompatibleDC(hMaindc);
            ReleaseDC(hMainWnd, hMaindc);
        }
        result = SendMessageW(hEditorWnd, EM_SETTARGETDEVICE, (WPARAM)hdc, width);
        DeleteDC(hdc);
        if (result)
            return;
        /* otherwise EM_SETTARGETDEVICE failed, so fall back on wrapping
         * to window using the NULL DC. */
    }

    if (wordWrap != ID_WORDWRAP_NONE) {
        SendMessageW(hEditorWnd, EM_SETTARGETDEVICE, 0, 0);
    } else {
        SendMessageW(hEditorWnd, EM_SETTARGETDEVICE, 0, 1);
    }

}

static LPWSTR dialog_print_to_file(HWND hMainWnd)
{
    OPENFILENAMEW ofn;
    static WCHAR file[MAX_PATH] = {'O','U','T','P','U','T','.','P','R','N',0};
    static const WCHAR defExt[] = {'P','R','N',0};
    static LPWSTR file_filter;

    if(!file_filter)
        file_filter = get_print_file_filter(hMainWnd);

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFilter = file_filter;
    ofn.lpstrFile = (LPWSTR)file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = (LPWSTR)defExt;

    if(GetSaveFileNameW(&ofn))
        return (LPWSTR)file;
    else
        return FALSE;
}

static int get_num_pages(HWND hEditorWnd, FORMATRANGE fr)
{
    int page = 0;
    fr.chrg.cpMin = 0;

    do
    {
        page++;
        fr.chrg.cpMin = SendMessageW(hEditorWnd, EM_FORMATRANGE, TRUE,
                                     (LPARAM)&fr);
    }
    while(fr.chrg.cpMin && fr.chrg.cpMin < fr.chrg.cpMax);

    return page;
}

static void char_from_pagenum(HWND hEditorWnd, FORMATRANGE *fr, int page)
{
    int i;

    fr->chrg.cpMin = 0;

    for(i = 1; i < page; i++)
    {
        fr->chrg.cpMin = SendMessageW(hEditorWnd, EM_FORMATRANGE, TRUE, (LPARAM)fr);
    }
}

static HWND get_ruler_wnd(HWND hMainWnd)
{
    return GetDlgItem(GetDlgItem(hMainWnd, IDC_REBAR), IDC_RULER);
}

void redraw_ruler(HWND hRulerWnd)
{
    RECT rc;

    GetClientRect(hRulerWnd, &rc);
    InvalidateRect(hRulerWnd, &rc, TRUE);
}

static void update_ruler(HWND hRulerWnd)
{
     SendMessageW(hRulerWnd, WM_USER, 0, 0);
     redraw_ruler(hRulerWnd);
}

static void print(LPPRINTDLGW pd, LPWSTR wszFileName)
{
    FORMATRANGE fr;
    DOCINFOW di;
    HWND hEditorWnd = GetDlgItem(pd->hwndOwner, IDC_EDITOR);
    int printedPages = 0;

    fr.hdc = pd->hDC;
    fr.hdcTarget = pd->hDC;

    fr.rc = get_print_rect(fr.hdc);
    fr.rcPage.left = 0;
    fr.rcPage.right = fr.rc.right + margins.right;
    fr.rcPage.top = 0;
    fr.rcPage.bottom = fr.rc.bottom + margins.bottom;

    ZeroMemory(&di, sizeof(di));
    di.cbSize = sizeof(di);
    di.lpszDocName = wszFileName;

    if(pd->Flags & PD_PRINTTOFILE)
    {
        di.lpszOutput = dialog_print_to_file(pd->hwndOwner);
        if(!di.lpszOutput)
            return;
    }

    if(pd->Flags & PD_SELECTION)
    {
        SendMessageW(hEditorWnd, EM_EXGETSEL, 0, (LPARAM)&fr.chrg);
    } else
    {
        GETTEXTLENGTHEX gt;
        gt.flags = GTL_DEFAULT;
        gt.codepage = 1200;
        fr.chrg.cpMin = 0;
        fr.chrg.cpMax = SendMessageW(hEditorWnd, EM_GETTEXTLENGTHEX, (WPARAM)&gt, 0);

        if(pd->Flags & PD_PAGENUMS)
            char_from_pagenum(hEditorWnd, &fr, pd->nToPage);
    }

    StartDocW(fr.hdc, &di);
    do
    {
        if(StartPage(fr.hdc) <= 0)
            break;

        fr.chrg.cpMin = SendMessageW(hEditorWnd, EM_FORMATRANGE, TRUE, (LPARAM)&fr);

        if(EndPage(fr.hdc) <= 0)
            break;

        printedPages++;
        if((pd->Flags & PD_PAGENUMS) && (printedPages > (pd->nToPage - pd->nFromPage)))
            break;
    }
    while(fr.chrg.cpMin && fr.chrg.cpMin < fr.chrg.cpMax);

    EndDoc(fr.hdc);
    SendMessageW(hEditorWnd, EM_FORMATRANGE, FALSE, 0);
}

void dialog_printsetup(HWND hMainWnd)
{
    PAGESETUPDLGW ps;

    ZeroMemory(&ps, sizeof(ps));
    ps.lStructSize = sizeof(ps);
    ps.hwndOwner = hMainWnd;
    ps.Flags = PSD_INHUNDREDTHSOFMILLIMETERS | PSD_MARGINS;
    ps.rtMargin.left = twips_to_centmm(margins.left);
    ps.rtMargin.right = twips_to_centmm(margins.right);
    ps.rtMargin.top = twips_to_centmm(margins.top);
    ps.rtMargin.bottom = twips_to_centmm(margins.bottom);
    ps.hDevMode = devMode;
    ps.hDevNames = devNames;

    if(PageSetupDlgW(&ps))
    {
        margins.left = centmm_to_twips(ps.rtMargin.left);
        margins.right = centmm_to_twips(ps.rtMargin.right);
        margins.top = centmm_to_twips(ps.rtMargin.top);
        margins.bottom = centmm_to_twips(ps.rtMargin.bottom);
        devMode = ps.hDevMode;
        devNames = ps.hDevNames;
        update_ruler(get_ruler_wnd(hMainWnd));
    }
}

void get_default_printer_opts(void)
{
    PRINTDLGW pd;
    ZeroMemory(&pd, sizeof(pd));

    ZeroMemory(&pd, sizeof(pd));
    pd.lStructSize = sizeof(pd);
    pd.Flags = PD_RETURNDC | PD_RETURNDEFAULT;
    pd.hDevMode = devMode;

    PrintDlgW(&pd);

    devMode = pd.hDevMode;
    devNames = pd.hDevNames;
}

void print_quick(LPWSTR wszFileName)
{
    PRINTDLGW pd;

    ZeroMemory(&pd, sizeof(pd));
    pd.hDC = make_dc();

    print(&pd, wszFileName);
}

void dialog_print(HWND hMainWnd, LPWSTR wszFileName)
{
    PRINTDLGW pd;
    HWND hEditorWnd = GetDlgItem(hMainWnd, IDC_EDITOR);
    int from = 0;
    int to = 0;

    ZeroMemory(&pd, sizeof(pd));
    pd.lStructSize = sizeof(pd);
    pd.hwndOwner = hMainWnd;
    pd.Flags = PD_RETURNDC | PD_USEDEVMODECOPIESANDCOLLATE;
    pd.nMinPage = 1;
    pd.nMaxPage = -1;
    pd.hDevMode = devMode;
    pd.hDevNames = devNames;

    SendMessageW(hEditorWnd, EM_GETSEL, (WPARAM)&from, (LPARAM)&to);
    if(from == to)
        pd.Flags |= PD_NOSELECTION;

    if(PrintDlgW(&pd))
    {
        devMode = pd.hDevMode;
        devNames = pd.hDevNames;
        print(&pd, wszFileName);
        update_ruler(get_ruler_wnd(hMainWnd));
    }
}

static void preview_bar_show(HWND hMainWnd, BOOL show)
{
    HWND hReBar = GetDlgItem(hMainWnd, IDC_REBAR);
    int i;

    if(show)
    {
        REBARBANDINFOW rb;
        HWND hStatic;

        AddTextButton(hReBar, STRING_PREVIEW_PRINT, ID_PRINT, BANDID_PREVIEW_BTN1);
        AddTextButton(hReBar, STRING_PREVIEW_NEXTPAGE, ID_PREVIEW_NEXTPAGE, BANDID_PREVIEW_BTN2);
        AddTextButton(hReBar, STRING_PREVIEW_PREVPAGE, ID_PREVIEW_PREVPAGE, BANDID_PREVIEW_BTN3);
        AddTextButton(hReBar, STRING_PREVIEW_TWOPAGES, ID_PREVIEW_NUMPAGES, BANDID_PREVIEW_BTN4);
        AddTextButton(hReBar, STRING_PREVIEW_CLOSE, ID_FILE_EXIT, BANDID_PREVIEW_BTN5);

        hStatic = CreateWindowW(WC_STATICW, NULL,
                                WS_VISIBLE | WS_CHILD, 0, 0, 0, 0,
                                hReBar, NULL, NULL, NULL);

        rb.cbSize = sizeof(rb);
        rb.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_CHILD | RBBIM_IDEALSIZE | RBBIM_ID;
        rb.fStyle = RBBS_NOGRIPPER | RBBS_VARIABLEHEIGHT;
        rb.hwndChild = hStatic;
        rb.cyChild = rb.cyMinChild = 22;
        rb.cx = rb.cxMinChild = 90;
        rb.cxIdeal = 100;
        rb.wID = BANDID_PREVIEW_BUFFER;

        SendMessageW(hReBar, RB_INSERTBAND, -1, (LPARAM)&rb);
    } else
    {
        for(i = 0; i <= PREVIEW_BUTTONS; i++)
            SendMessageW(hReBar, RB_DELETEBAND, SendMessageW(hReBar, RB_IDTOINDEX, BANDID_PREVIEW_BTN1+i, 0), 0);
    }
}

void init_preview(HWND hMainWnd, LPWSTR wszFileName)
{
    preview.page = 1;
    preview.hdc = 0;
    preview.hdc2 = 0;
    preview.wszFileName = wszFileName;
    preview_bar_show(hMainWnd, TRUE);
}

void close_preview(HWND hMainWnd)
{
    preview.window.right = 0;
    preview.window.bottom = 0;
    preview.page = 0;
    preview.pages = 0;

    preview_bar_show(hMainWnd, FALSE);
}

BOOL preview_isactive(void)
{
    return preview.page != 0;
}

static void add_ruler_units(HDC hdcRuler, RECT* drawRect, BOOL NewMetrics, long EditLeftmost)
{
    static HDC hdc;

    if(NewMetrics)
    {
        static HBITMAP hBitmap;
        int i, x, y, RulerTextEnd;
        int CmPixels;
        int QuarterCmPixels;
        HFONT hFont;
        WCHAR FontName[] = {'M','S',' ','S','a','n','s',' ','S','e','r','i','f',0};

        if(hdc)
        {
            DeleteDC(hdc);
            DeleteObject(hBitmap);
        }

        hdc = CreateCompatibleDC(0);

        CmPixels = twips_to_pixels(centmm_to_twips(1000), GetDeviceCaps(hdc, LOGPIXELSX));
        QuarterCmPixels = (int)((float)CmPixels / 4.0);

        hBitmap = CreateCompatibleBitmap(hdc, drawRect->right, drawRect->bottom);
        SelectObject(hdc, hBitmap);
        FillRect(hdc, drawRect, GetStockObject(WHITE_BRUSH));

        hFont = CreateFontW(10, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FontName);

        SelectObject(hdc, hFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextAlign(hdc, TA_CENTER);
        y = (int)(((float)drawRect->bottom - (float)drawRect->top) / 2.0) + 1;
        RulerTextEnd = drawRect->right - EditLeftmost + 1;
        for(i = 1, x = EditLeftmost; x < (drawRect->right - EditLeftmost + 1); i ++)
        {
            WCHAR str[3];
            WCHAR format[] = {'%','d',0};
            int x2 = x;

            x2 += QuarterCmPixels;
            if(x2 > RulerTextEnd)
                break;

            MoveToEx(hdc, x2, y, NULL);
            LineTo(hdc, x2, y+2);

            x2 += QuarterCmPixels;
            if(x2 > RulerTextEnd)
                break;

            MoveToEx(hdc, x2, y - 3, NULL);
            LineTo(hdc, x2, y + 3);

            x2 += QuarterCmPixels;
            if(x2 > RulerTextEnd)
                break;

            MoveToEx(hdc, x2, y, NULL);
            LineTo(hdc, x2, y+2);

            x += CmPixels;
            if(x > RulerTextEnd)
                break;

            wsprintfW(str, format, i);
            TextOutW(hdc, x, 5, str, lstrlenW(str));
        }
        DeleteObject(hFont);
    }

    BitBlt(hdcRuler, 0, 0, drawRect->right, drawRect->bottom, hdc, 0, 0, SRCAND);
}

static void paint_ruler(HWND hWnd, long EditLeftmost, BOOL NewMetrics)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    HDC hdcPrint = make_dc();
    RECT printRect = get_print_rect(hdcPrint);
    RECT drawRect;
    HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_MENU));

    GetClientRect(hWnd, &drawRect);
    FillRect(hdc, &drawRect, hBrush);

    drawRect.top += 3;
    drawRect.bottom -= 3;
    drawRect.left = EditLeftmost;
    drawRect.right = twips_to_pixels(printRect.right - margins.left, GetDeviceCaps(hdc, LOGPIXELSX));
    FillRect(hdc, &drawRect, GetStockObject(WHITE_BRUSH));

    drawRect.top--;
    drawRect.bottom++;
    DrawEdge(hdc, &drawRect, EDGE_SUNKEN, BF_RECT);

    drawRect.left = drawRect.right - 1;
    drawRect.right = twips_to_pixels(printRect.right + margins.right - margins.left, GetDeviceCaps(hdc, LOGPIXELSX));
    DrawEdge(hdc, &drawRect, EDGE_ETCHED, BF_RECT);

    drawRect.left = 0;
    drawRect.top = 0;
    add_ruler_units(hdc, &drawRect, NewMetrics, EditLeftmost);

    SelectObject(hdc, GetStockObject(BLACK_BRUSH));
    DeleteObject(hBrush);
    DeleteDC(hdcPrint);
    EndPaint(hWnd, &ps);
}

LRESULT CALLBACK ruler_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static WNDPROC pPrevRulerProc;
    static long EditLeftmost;
    static BOOL NewMetrics;

    switch(msg)
    {
        case WM_USER:
            if(wParam)
            {
                EditLeftmost = ((POINTL*)wParam)->x;
                pPrevRulerProc = (WNDPROC)lParam;
            }
            NewMetrics = TRUE;
            break;

        case WM_PAINT:
            paint_ruler(hWnd, EditLeftmost, NewMetrics);
            break;

        default:
            return CallWindowProcW(pPrevRulerProc, hWnd, msg, wParam, lParam);
    }

    return 0;
}

static void draw_preview_page(HDC hdc, HDC* hdcSized, FORMATRANGE* lpFr, float ratio, int bmNewWidth, int bmNewHeight, int bmWidth, int bmHeight)
{
    HBITMAP hBitmapScaled = CreateCompatibleBitmap(hdc, bmNewWidth, bmNewHeight);
    HPEN hPen;
    int TopMargin = (int)((float)twips_to_pixels(lpFr->rc.top, GetDeviceCaps(hdc, LOGPIXELSX)) * ratio);
    int BottomMargin = (int)((float)twips_to_pixels(lpFr->rc.bottom, GetDeviceCaps(hdc, LOGPIXELSX)) * ratio);
    int LeftMargin = (int)((float)twips_to_pixels(lpFr->rc.left, GetDeviceCaps(hdc, LOGPIXELSY)) * ratio);
    int RightMargin = (int)((float)twips_to_pixels(lpFr->rc.right, GetDeviceCaps(hdc, LOGPIXELSY)) * ratio);

    if(*hdcSized)
        DeleteDC(*hdcSized);
    *hdcSized = CreateCompatibleDC(hdc);
    SelectObject(*hdcSized, hBitmapScaled);

    StretchBlt(*hdcSized, 0, 0, bmNewWidth, bmNewHeight, hdc, 0, 0, bmWidth, bmHeight, SRCCOPY);

    /* Draw margin lines */
    hPen = CreatePen(PS_DOT, 1, RGB(0,0,0));
    SelectObject(*hdcSized, hPen);

    MoveToEx(*hdcSized, 0, TopMargin, NULL);
    LineTo(*hdcSized, bmNewWidth, TopMargin);
    MoveToEx(*hdcSized, 0, BottomMargin, NULL);
    LineTo(*hdcSized, bmNewWidth, BottomMargin);

    MoveToEx(*hdcSized, LeftMargin, 0, NULL);
    LineTo(*hdcSized, LeftMargin, bmNewHeight);
    MoveToEx(*hdcSized, RightMargin, 0, NULL);
    LineTo(*hdcSized, RightMargin, bmNewHeight);

}

static void draw_preview(HWND hEditorWnd, FORMATRANGE* lpFr, int bmWidth, int bmHeight, RECT* paper, int page)
{
    HBITMAP hBitmapCapture = CreateCompatibleBitmap(lpFr->hdc, bmWidth, bmHeight);

    char_from_pagenum(hEditorWnd, lpFr, page);
    SelectObject(lpFr->hdc, hBitmapCapture);
    FillRect(lpFr->hdc, paper, GetStockObject(WHITE_BRUSH));
    SendMessageW(hEditorWnd, EM_FORMATRANGE, TRUE, (LPARAM)lpFr);
    /* EM_FORMATRANGE sets fr.rc to indicate the area printed in, but we want to
       keep the original for drawing margins */
    lpFr->rc = get_print_rect(lpFr->hdcTarget);
    SendMessageW(hEditorWnd, EM_FORMATRANGE, FALSE, 0);
}

LRESULT print_preview(HWND hMainWnd)
{
    FORMATRANGE fr;
    GETTEXTLENGTHEX gt;
    HDC hdc;
    RECT window, background;
    int bmWidth, bmHeight, bmNewWidth, bmNewHeight;
    float ratioWidth, ratioHeight, ratio;
    int xOffset, yOffset;
    int barheight;
    float spacing = 20.0;
    HWND hReBar = GetDlgItem(hMainWnd, IDC_REBAR);
    PAINTSTRUCT ps;

    hdc = BeginPaint(hMainWnd, &ps);
    GetClientRect(hMainWnd, &window);

    fr.hdcTarget = make_dc();
    fr.rc = get_print_rect(fr.hdcTarget);
    fr.rcPage.left = 0;
    fr.rcPage.top = 0;
    fr.rcPage.bottom = fr.rc.bottom + margins.bottom;
    fr.rcPage.right = fr.rc.right + margins.right;

    bmWidth = twips_to_pixels(fr.rcPage.right, GetDeviceCaps(hdc, LOGPIXELSX));
    bmHeight = twips_to_pixels(fr.rcPage.bottom, GetDeviceCaps(hdc, LOGPIXELSY));

    if(!preview.hdc)
    {
        RECT paper;
        HWND hEditorWnd = GetDlgItem(hMainWnd, IDC_EDITOR);

        preview.hdc = CreateCompatibleDC(hdc);

        if(preview.hdc2)
        {
            if(preview.hdc2 != (HDC)-1)
                DeleteDC(preview.hdc2);
            preview.hdc2 = CreateCompatibleDC(hdc);
        }

        fr.hdc = preview.hdc;
        gt.flags = GTL_DEFAULT;
        gt.codepage = 1200;
        fr.chrg.cpMin = 0;
        fr.chrg.cpMax = SendMessageW(hEditorWnd, EM_GETTEXTLENGTHEX, (WPARAM)&gt, 0);

        paper.left = 0;
        paper.right = bmWidth;
        paper.top = 0;
        paper.bottom = bmHeight;

        if(!preview.pages)
            preview.pages = get_num_pages(hEditorWnd, fr);

        fr.hdc = preview.hdc;
        draw_preview(hEditorWnd, &fr, bmWidth, bmHeight, &paper, preview.page);

        if(preview.hdc2)
        {
            fr.hdc = preview.hdc2;
            draw_preview(hEditorWnd, &fr, bmWidth, bmHeight, &paper, preview.page + 1);
        }

        EnableWindow(GetDlgItem(hReBar, ID_PREVIEW_PREVPAGE), preview.page > 1);
        EnableWindow(GetDlgItem(hReBar, ID_PREVIEW_NEXTPAGE), preview.hdc2 ?
                                                              (preview.page + 1) < preview.pages :
                                                              preview.page < preview.pages);
        EnableWindow(GetDlgItem(hReBar, ID_PREVIEW_NUMPAGES), preview.pages > 1);
    }

    barheight = SendMessageW(hReBar, RB_GETBARHEIGHT, 0, 0);
    ratioHeight = ((float)window.bottom - spacing - (float)barheight) / (float)bmHeight;

    if(preview.hdc2)
        ratioWidth = ((float)window.right / 2.0 - spacing * 2.0) / (float)bmWidth;
    else
        ratioWidth = ((float)window.right - spacing * 3.0) / (float)bmWidth;

    if(ratioWidth > ratioHeight)
        ratio = ratioHeight;
    else
        ratio = ratioWidth;

    bmNewWidth = (int)((float)bmWidth * ratio);
    bmNewHeight = (int)((float)bmHeight * ratio);

    yOffset = ((window.bottom - bmNewHeight + barheight) / 2);

    if(!preview.hdc2)
        xOffset = (window.right - bmNewWidth) / 2;
    else
        xOffset = (window.right - bmNewWidth * 2) / 2;

    window.top = barheight;
    FillRect(hdc, &window, GetStockObject(GRAY_BRUSH));

    background.left = xOffset - 2;
    background.right = xOffset + bmNewWidth + 2;
    background.top = yOffset - 2;
    background.bottom = yOffset + bmNewHeight + 2;

    FillRect(hdc, &background, GetStockObject(BLACK_BRUSH));

    if(window.right != preview.window.right || window.bottom != preview.window.bottom)
    {
        draw_preview_page(preview.hdc, &preview.hdcSized, &fr, ratio, bmNewWidth, bmNewHeight, bmWidth, bmHeight);

        if(preview.hdc2)
        {
            background.left += bmNewWidth + spacing;
            background.right += bmNewWidth + spacing;

            FillRect(hdc, &background, GetStockObject(BLACK_BRUSH));

            draw_preview_page(preview.hdc2, &preview.hdcSized2, &fr, ratio, bmNewWidth, bmNewHeight, bmWidth, bmHeight);
        }
    }

    BitBlt(hdc, xOffset, yOffset, bmNewWidth, bmNewHeight, preview.hdcSized, 0, 0, SRCCOPY);

    if(preview.hdc2)
    {
        BitBlt(hdc, xOffset + bmNewWidth + spacing, yOffset, bmNewWidth, bmNewHeight, preview.hdcSized2, 0, 0, SRCCOPY);
    }

    DeleteDC(fr.hdcTarget);
    preview.window = window;

    EndPaint(hMainWnd, &ps);

    return 0;
}

static void update_preview(HWND hWnd)
{
    RECT rc;

    DeleteDC(preview.hdc);
    preview.hdc = 0;

    preview.window.right = 0;

    GetClientRect(hWnd, &rc);
    rc.top += SendMessageW(GetDlgItem(hWnd, IDC_REBAR), RB_GETBARHEIGHT, 0, 0);
    InvalidateRect(hWnd, &rc, TRUE);
}

LRESULT preview_command(HWND hWnd, WPARAM wParam)
{
    switch(LOWORD(wParam))
    {
        case ID_FILE_EXIT:
            PostMessageW(hWnd, WM_CLOSE, 0, 0);
            break;

        case ID_PREVIEW_NEXTPAGE:
        case ID_PREVIEW_PREVPAGE:
        {
            if(LOWORD(wParam) == ID_PREVIEW_NEXTPAGE)
                preview.page++;
            else
                preview.page--;

            update_preview(hWnd);
        }
        break;

        case ID_PREVIEW_NUMPAGES:
        {
            HWND hReBar = GetDlgItem(hWnd, IDC_REBAR);
            WCHAR name[MAX_STRING_LEN];
            HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(hWnd, GWLP_HINSTANCE);

            if(preview.hdc2)
            {
                DeleteDC(preview.hdc2);
                preview.hdc2 = 0;
            } else
            {
                if(preview.page == preview.pages)
                    preview.page--;
                preview.hdc2 = (HDC)-1;
            }

            LoadStringW(hInst, preview.hdc2 ? STRING_PREVIEW_ONEPAGE : STRING_PREVIEW_TWOPAGES,
                        name, MAX_STRING_LEN);

            SetWindowTextW(GetDlgItem(hReBar, ID_PREVIEW_NUMPAGES), name);
            update_preview(hWnd);
        }
        break;

        case ID_PRINT:
            dialog_print(hWnd, preview.wszFileName);
            SendMessageW(hWnd, WM_CLOSE, 0, 0);
            break;
    }

    return 0;
}
