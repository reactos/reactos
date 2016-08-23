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

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winuser.h>
#include <richedit.h>
#include <commctrl.h>
#include <commdlg.h>

#include "wordpad.h"

typedef struct _previewinfo
{
    int page;
    int pages_shown;
    int saved_pages_shown;
    int *pageEnds, pageCapacity;
    int textlength;
    HDC hdc;
    HDC hdc2;
    RECT window;
    RECT rcPage;
    SIZE bmSize;
    SIZE bmScaledSize;
    SIZE spacing;
    float zoomratio;
    int zoomlevel;
    LPWSTR wszFileName;
} previewinfo, *ppreviewinfo;

static HGLOBAL devMode;
static HGLOBAL devNames;

static RECT margins;
static previewinfo preview;

extern const WCHAR wszPreviewWndClass[];

static const WCHAR var_pagemargin[] = {'P','a','g','e','M','a','r','g','i','n',0};
static const WCHAR var_previewpages[] = {'P','r','e','v','i','e','w','P','a','g','e','s',0};

static LPWSTR get_print_file_filter(HWND hMainWnd)
{
    static WCHAR wszPrintFilter[MAX_STRING_LEN*2+6+4+1];
    const WCHAR files_prn[] = {'*','.','P','R','N',0};
    const WCHAR files_all[] = {'*','.','*','\0'};
    LPWSTR p;
    HINSTANCE hInstance = GetModuleHandleW(0);

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
        SetRect(&margins, 1757, 1417, 1757, 1417);
}

void registry_set_previewpages(HKEY hKey)
{
    RegSetValueExW(hKey, var_previewpages, 0, REG_DWORD,
                   (LPBYTE)&preview.pages_shown, sizeof(DWORD));
}

void registry_read_previewpages(HKEY hKey)
{
    DWORD size = sizeof(DWORD);
    if(!hKey ||
       RegQueryValueExW(hKey, var_previewpages, 0, NULL,
                        (LPBYTE)&preview.pages_shown, &size) != ERROR_SUCCESS ||
       size != sizeof(DWORD))
    {
        preview.pages_shown = 1;
    } else {
        if (preview.pages_shown < 1) preview.pages_shown = 1;
        else if (preview.pages_shown > 2) preview.pages_shown = 2;
    }
}


static void AddTextButton(HWND hRebarWnd, UINT string, UINT command, UINT id)
{
    REBARBANDINFOW rb;
    HINSTANCE hInstance = GetModuleHandleW(0);
    WCHAR text[MAX_STRING_LEN];
    HWND hButton;

    LoadStringW(hInstance, string, text, MAX_STRING_LEN);
    hButton = CreateWindowW(WC_BUTTONW, text,
                            WS_VISIBLE | WS_CHILD, 5, 5, 100, 15,
                            hRebarWnd, ULongToHandle(command), hInstance, NULL);

    rb.cbSize = REBARBANDINFOW_V6_SIZE;
    rb.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_CHILD | RBBIM_IDEALSIZE | RBBIM_ID;
    rb.fStyle = RBBS_NOGRIPPER | RBBS_VARIABLEHEIGHT;
    rb.hwndChild = hButton;
    rb.cyChild = rb.cyMinChild = 22;
    rb.cx = rb.cxMinChild = 90;
    rb.cxIdeal = 100;
    rb.wID = id;

    SendMessageW(hRebarWnd, RB_INSERTBANDW, -1, (LPARAM)&rb);
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

    SetRect(&rc, margins.left, margins.top, width - margins.right, height - margins.bottom);

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
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = defExt;

    if(GetSaveFileNameW(&ofn))
        return file;
    else
        return FALSE;
}

static void char_from_pagenum(HWND hEditorWnd, FORMATRANGE *fr, int page)
{
    int i;

    fr->chrg.cpMin = 0;

    for(i = 1; i < page; i++)
    {
        int bottom = fr->rc.bottom;
        fr->chrg.cpMin = SendMessageW(hEditorWnd, EM_FORMATRANGE, FALSE, (LPARAM)fr);
        fr->rc.bottom = bottom;
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

static void add_ruler_units(HDC hdcRuler, RECT* drawRect, BOOL NewMetrics, LONG EditLeftmost)
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

static void paint_ruler(HWND hWnd, LONG EditLeftmost, BOOL NewMetrics)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    HDC hdcPrint = make_dc();
    RECT printRect = get_print_rect(hdcPrint);
    RECT drawRect;
    HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_MENU));

    GetClientRect(hWnd, &drawRect);
    FillRect(hdc, &drawRect, hBrush);

    InflateRect(&drawRect, 0, -3);
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
    static LONG EditLeftmost;
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

static void print(LPPRINTDLGW pd, LPWSTR wszFileName)
{
    FORMATRANGE fr;
    DOCINFOW di;
    HWND hEditorWnd = GetDlgItem(pd->hwndOwner, IDC_EDITOR);
    int printedPages = 0;

    fr.hdc = pd->hDC;
    fr.hdcTarget = pd->hDC;

    fr.rc = get_print_rect(fr.hdc);
    SetRect(&fr.rcPage, 0, 0, fr.rc.right + margins.right, fr.rc.bottom + margins.bottom);

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
    SetRect(&ps.rtMargin, twips_to_centmm(margins.left), twips_to_centmm(margins.top),
            twips_to_centmm(margins.right), twips_to_centmm(margins.bottom));
    ps.hDevMode = devMode;
    ps.hDevNames = devNames;

    if(PageSetupDlgW(&ps))
    {
        SetRect(&margins, centmm_to_twips(ps.rtMargin.left), centmm_to_twips(ps.rtMargin.top),
                centmm_to_twips(ps.rtMargin.right), centmm_to_twips(ps.rtMargin.bottom));
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

void print_quick(HWND hMainWnd, LPWSTR wszFileName)
{
    PRINTDLGW pd;

    ZeroMemory(&pd, sizeof(pd));
    pd.hwndOwner = hMainWnd;
    pd.hDC = make_dc();

    print(&pd, wszFileName);
    DeleteDC(pd.hDC);
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
        UINT num_pages_string = preview.pages_shown > 1 ? STRING_PREVIEW_ONEPAGE :
                                                          STRING_PREVIEW_TWOPAGES;

        AddTextButton(hReBar, STRING_PREVIEW_PRINT, ID_PRINT, BANDID_PREVIEW_BTN1);
        AddTextButton(hReBar, STRING_PREVIEW_NEXTPAGE, ID_PREVIEW_NEXTPAGE, BANDID_PREVIEW_BTN2);
        AddTextButton(hReBar, STRING_PREVIEW_PREVPAGE, ID_PREVIEW_PREVPAGE, BANDID_PREVIEW_BTN3);
        AddTextButton(hReBar, num_pages_string, ID_PREVIEW_NUMPAGES, BANDID_PREVIEW_BTN4);
        AddTextButton(hReBar, STRING_PREVIEW_ZOOMIN, ID_PREVIEW_ZOOMIN, BANDID_PREVIEW_BTN5);
        AddTextButton(hReBar, STRING_PREVIEW_ZOOMOUT, ID_PREVIEW_ZOOMOUT, BANDID_PREVIEW_BTN6);
        AddTextButton(hReBar, STRING_PREVIEW_CLOSE, ID_FILE_EXIT, BANDID_PREVIEW_BTN7);

        hStatic = CreateWindowW(WC_STATICW, NULL,
                                WS_VISIBLE | WS_CHILD, 0, 0, 0, 0,
                                hReBar, NULL, NULL, NULL);

        rb.cbSize = REBARBANDINFOW_V6_SIZE;
        rb.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_CHILD | RBBIM_IDEALSIZE | RBBIM_ID;
        rb.fStyle = RBBS_NOGRIPPER | RBBS_VARIABLEHEIGHT;
        rb.hwndChild = hStatic;
        rb.cyChild = rb.cyMinChild = 22;
        rb.cx = rb.cxMinChild = 90;
        rb.cxIdeal = 100;
        rb.wID = BANDID_PREVIEW_BUFFER;

        SendMessageW(hReBar, RB_INSERTBANDW, -1, (LPARAM)&rb);
    } else
    {
        for(i = 0; i <= PREVIEW_BUTTONS; i++)
            SendMessageW(hReBar, RB_DELETEBAND, SendMessageW(hReBar, RB_IDTOINDEX, BANDID_PREVIEW_BTN1+i, 0), 0);
    }
}

static const int min_spacing = 10;

static void update_preview_scrollbars(HWND hwndPreview, RECT *window)
{
    SCROLLINFO sbi;
    sbi.cbSize = sizeof(sbi);
    sbi.fMask = SIF_PAGE|SIF_RANGE;
    sbi.nMin = 0;
    if (preview.zoomlevel == 0)
    {
        /* Hide scrollbars when zoomed out. */
        sbi.nMax = 0;
        sbi.nPage = window->right;
        SetScrollInfo(hwndPreview, SB_HORZ, &sbi, TRUE);
        sbi.nPage = window->bottom;
        SetScrollInfo(hwndPreview, SB_VERT, &sbi, TRUE);
    } else {
        sbi.nMax = preview.bmScaledSize.cx * preview.pages_shown +
                   min_spacing * (preview.pages_shown + 1);
        sbi.nPage = window->right;
        SetScrollInfo(hwndPreview, SB_HORZ, &sbi, TRUE);
        /* Change in the horizontal scrollbar visibility affects the
         * client rect, so update the client rect. */
        GetClientRect(hwndPreview, window);
        sbi.nMax = preview.bmScaledSize.cy + min_spacing * 2;
        sbi.nPage = window->bottom;
        SetScrollInfo(hwndPreview, SB_VERT, &sbi, TRUE);
    }
}

static void update_preview_sizes(HWND hwndPreview, BOOL zoomLevelUpdated)
{
    RECT window;

    GetClientRect(hwndPreview, &window);

    /* The zoom ratio isn't updated for partial zoom because of resizing the window. */
    if (zoomLevelUpdated || preview.zoomlevel != 1)
    {
        float ratio, ratioHeight, ratioWidth;
        if (preview.zoomlevel == 2)
        {
            ratio = 1.0;
        } else {
            ratioHeight = (window.bottom - min_spacing * 2) / (float)preview.bmSize.cy;

            ratioWidth = (float)(window.right -
                                 min_spacing * (preview.pages_shown + 1)) /
                         (preview.pages_shown * preview.bmSize.cx);

            if(ratioWidth > ratioHeight)
                ratio = ratioHeight;
            else
                ratio = ratioWidth;

            if (preview.zoomlevel == 1)
                ratio += (1.0 - ratio) / 2;
        }
        preview.zoomratio = ratio;
    }

    preview.bmScaledSize.cx = preview.bmSize.cx * preview.zoomratio;
    preview.bmScaledSize.cy = preview.bmSize.cy * preview.zoomratio;

    preview.spacing.cy = max(min_spacing, (window.bottom - preview.bmScaledSize.cy) / 2);

    preview.spacing.cx = (window.right -
                          preview.bmScaledSize.cx * preview.pages_shown) /
                         (preview.pages_shown + 1);
    if (preview.spacing.cx < min_spacing)
        preview.spacing.cx = min_spacing;

    update_preview_scrollbars(hwndPreview, &window);
}

static void draw_margin_lines(HDC hdc, int x, int y, float ratio)
{
    HPEN hPen, oldPen;
    SIZE dpi;
    RECT page_margin = preview.rcPage;

    dpi.cx = GetDeviceCaps(hdc, LOGPIXELSX);
    dpi.cy = GetDeviceCaps(hdc, LOGPIXELSY);

    SetRect(&page_margin, preview.rcPage.left + margins.left, preview.rcPage.top + margins.top,
            preview.rcPage.right - margins.right, preview.rcPage.bottom - margins.bottom);

    page_margin.left = (int)((float)twips_to_pixels(page_margin.left, dpi.cx) * ratio);
    page_margin.top = (int)((float)twips_to_pixels(page_margin.top, dpi.cy) * ratio);
    page_margin.bottom = (int)((float)twips_to_pixels(page_margin.bottom, dpi.cy) * ratio);
    page_margin.right = (int)((float)twips_to_pixels(page_margin.right, dpi.cx) * ratio);

    OffsetRect(&page_margin, x, y);

    hPen = CreatePen(PS_DOT, 1, RGB(0,0,0));
    oldPen = SelectObject(hdc, hPen);

    MoveToEx(hdc, x, page_margin.top, NULL);
    LineTo(hdc, x + preview.bmScaledSize.cx, page_margin.top);
    MoveToEx(hdc, x, page_margin.bottom, NULL);
    LineTo(hdc, x + preview.bmScaledSize.cx, page_margin.bottom);

    MoveToEx(hdc, page_margin.left, y, NULL);
    LineTo(hdc, page_margin.left, y + preview.bmScaledSize.cy);
    MoveToEx(hdc, page_margin.right, y, NULL);
    LineTo(hdc, page_margin.right, y + preview.bmScaledSize.cy);

    SelectObject(hdc, oldPen);
    DeleteObject(hPen);
}

static BOOL is_last_preview_page(int page)
{
    return preview.pageEnds[page - 1] >= preview.textlength;
}

void init_preview(HWND hMainWnd, LPWSTR wszFileName)
{
    HINSTANCE hInstance = GetModuleHandleW(0);
    preview.page = 1;
    preview.hdc = 0;
    preview.hdc2 = 0;
    preview.wszFileName = wszFileName;
    preview.zoomratio = 0;
    preview.zoomlevel = 0;
    preview_bar_show(hMainWnd, TRUE);

    CreateWindowExW(0, wszPreviewWndClass, NULL,
            WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL,
            0, 0, 200, 10, hMainWnd, (HMENU)IDC_PREVIEW, hInstance, NULL);
}

void close_preview(HWND hMainWnd)
{
    HWND hwndPreview = GetDlgItem(hMainWnd, IDC_PREVIEW);
    preview.window.right = 0;
    preview.window.bottom = 0;
    preview.page = 0;
    HeapFree(GetProcessHeap(), 0, preview.pageEnds);
    preview.pageEnds = NULL;
    preview.pageCapacity = 0;
    if (preview.zoomlevel > 0)
        preview.pages_shown = preview.saved_pages_shown;
    if(preview.hdc) {
        HBITMAP oldbm = GetCurrentObject(preview.hdc, OBJ_BITMAP);
        DeleteDC(preview.hdc);
        DeleteObject(oldbm);
        preview.hdc = NULL;
    }
    if(preview.hdc2) {
        HBITMAP oldbm = GetCurrentObject(preview.hdc2, OBJ_BITMAP);
        DeleteDC(preview.hdc2);
        DeleteObject(oldbm);
        preview.hdc2 = NULL;
    }

    preview_bar_show(hMainWnd, FALSE);
    DestroyWindow(hwndPreview);
}

BOOL preview_isactive(void)
{
    return preview.page != 0;
}

static void draw_preview(HWND hEditorWnd, FORMATRANGE* lpFr, RECT* paper, int page)
{
    int bottom;

    if (!preview.pageEnds)
    {
        preview.pageCapacity = 32;
        preview.pageEnds = HeapAlloc(GetProcessHeap(), 0,
                                    sizeof(int) * preview.pageCapacity);
        if (!preview.pageEnds) return;
    } else if (page >= preview.pageCapacity) {
        int *new_buffer;
        new_buffer = HeapReAlloc(GetProcessHeap(), 0, preview.pageEnds,
                                 sizeof(int) * preview.pageCapacity * 2);
        if (!new_buffer) return;
        preview.pageCapacity *= 2;
        preview.pageEnds = new_buffer;
    }

    FillRect(lpFr->hdc, paper, GetStockObject(WHITE_BRUSH));
    if (page > 1 && is_last_preview_page(page - 1)) return;
    lpFr->chrg.cpMin = page <= 1 ? 0 : preview.pageEnds[page-2];
    bottom = lpFr->rc.bottom;
    preview.pageEnds[page-1] = SendMessageW(hEditorWnd, EM_FORMATRANGE, TRUE, (LPARAM)lpFr);

    /* EM_FORMATRANGE sets fr.rc.bottom to indicate the area printed in,
     * but we want to keep the original for drawing margins */
    lpFr->rc.bottom = bottom;
    SendMessageW(hEditorWnd, EM_FORMATRANGE, FALSE, 0);
}

static void update_preview_buttons(HWND hMainWnd)
{
    HWND hReBar = GetDlgItem(hMainWnd, IDC_REBAR);
    EnableWindow(GetDlgItem(hReBar, ID_PREVIEW_PREVPAGE), preview.page > 1);
    EnableWindow(GetDlgItem(hReBar, ID_PREVIEW_NEXTPAGE),
                 !is_last_preview_page(preview.page) &&
                 !is_last_preview_page(preview.page + preview.pages_shown - 1));
    EnableWindow(GetDlgItem(hReBar, ID_PREVIEW_NUMPAGES),
                 preview.pages_shown > 1 ||
                 (!is_last_preview_page(1) && preview.zoomlevel == 0));
    EnableWindow(GetDlgItem(hReBar, ID_PREVIEW_ZOOMIN), preview.zoomlevel < 2);
    EnableWindow(GetDlgItem(hReBar, ID_PREVIEW_ZOOMOUT), preview.zoomlevel > 0);
}

static LRESULT print_preview(HWND hwndPreview)
{
    HPEN hPen, oldPen;
    HDC hdc;
    HRGN back_rgn, excl_rgn;
    RECT window, background;
    PAINTSTRUCT ps;
    int x, y;

    hdc = BeginPaint(hwndPreview, &ps);
    GetClientRect(hwndPreview, &window);
    back_rgn = CreateRectRgnIndirect(&window);

    x = preview.spacing.cx - GetScrollPos(hwndPreview, SB_HORZ);
    y = preview.spacing.cy - GetScrollPos(hwndPreview, SB_VERT);

    /* draw page outlines */
    hPen = CreatePen(PS_SOLID|PS_INSIDEFRAME, 2, RGB(0,0,0));
    oldPen = SelectObject(hdc, hPen);
    SetRect(&background, x - 2, y - 2, x + preview.bmScaledSize.cx + 2,
            y + preview.bmScaledSize.cy + 2);
    Rectangle(hdc, background.left, background.top,
              background.right, background.bottom);
    excl_rgn = CreateRectRgnIndirect(&background);
    CombineRgn(back_rgn, back_rgn, excl_rgn, RGN_DIFF);
    if(preview.pages_shown > 1)
    {
        background.left += preview.bmScaledSize.cx + preview.spacing.cx;
        background.right += preview.bmScaledSize.cx + preview.spacing.cx;
        Rectangle(hdc, background.left, background.top,
                  background.right, background.bottom);
        SetRectRgn(excl_rgn, background.left, background.top,
                   background.right, background.bottom);
        CombineRgn(back_rgn, back_rgn, excl_rgn, RGN_DIFF);
    }
    SelectObject(hdc, oldPen);
    DeleteObject(hPen);
    FillRgn(hdc, back_rgn, GetStockObject(GRAY_BRUSH));
    DeleteObject(excl_rgn);
    DeleteObject(back_rgn);

    StretchBlt(hdc, x, y, preview.bmScaledSize.cx, preview.bmScaledSize.cy,
               preview.hdc, 0, 0, preview.bmSize.cx, preview.bmSize.cy, SRCCOPY);

    draw_margin_lines(hdc, x, y, preview.zoomratio);

    if(preview.pages_shown > 1)
    {
        if (!is_last_preview_page(preview.page)) {
            x += preview.spacing.cx + preview.bmScaledSize.cx;
            StretchBlt(hdc, x, y,
                       preview.bmScaledSize.cx, preview.bmScaledSize.cy,
                       preview.hdc2, 0, 0,
                       preview.bmSize.cx, preview.bmSize.cy, SRCCOPY);

            draw_margin_lines(hdc, x, y, preview.zoomratio);
        } else {
            InflateRect(&background, -2, -2);
            FillRect(hdc, &background, GetStockObject(WHITE_BRUSH));
        }
    }

    preview.window = window;

    EndPaint(hwndPreview, &ps);

    return 0;
}

static void update_preview_statusbar(HWND hMainWnd)
{
    HWND hStatusbar = GetDlgItem(hMainWnd, IDC_STATUSBAR);
    HINSTANCE hInst = GetModuleHandleW(0);
    WCHAR *p;
    WCHAR wstr[MAX_STRING_LEN];

    p = wstr;
    if (preview.pages_shown < 2 || is_last_preview_page(preview.page))
    {
        static const WCHAR fmt[] = {' ','%','d','\0'};
        p += LoadStringW(hInst, STRING_PREVIEW_PAGE, wstr, MAX_STRING_LEN);
        wsprintfW(p, fmt, preview.page);
    } else {
        static const WCHAR fmt[] = {' ','%','d','-','%','d','\0'};
        p += LoadStringW(hInst, STRING_PREVIEW_PAGES, wstr, MAX_STRING_LEN);
        wsprintfW(p, fmt, preview.page, preview.page + 1);
    }
    SetWindowTextW(hStatusbar, wstr);
}

/* Update for page changes. */
static void update_preview(HWND hMainWnd)
{
    RECT paper;
    HWND hEditorWnd = GetDlgItem(hMainWnd, IDC_EDITOR);
    HWND hwndPreview = GetDlgItem(hMainWnd, IDC_PREVIEW);
    HBITMAP hBitmapCapture;
    FORMATRANGE fr;
    HDC hdc = GetDC(hwndPreview);

    fr.hdcTarget = make_dc();
    fr.rc = fr.rcPage = preview.rcPage;
    fr.rc.left += margins.left;
    fr.rc.top += margins.top;
    fr.rc.bottom -= margins.bottom;
    fr.rc.right -= margins.right;

    fr.chrg.cpMin = 0;
    fr.chrg.cpMax = preview.textlength;

    SetRect(&paper, 0, 0, preview.bmSize.cx, preview.bmSize.cy);

    if (!preview.hdc) {
        preview.hdc = CreateCompatibleDC(hdc);
        hBitmapCapture = CreateCompatibleBitmap(hdc, preview.bmSize.cx, preview.bmSize.cy);
        SelectObject(preview.hdc, hBitmapCapture);
    }

    fr.hdc = preview.hdc;
    draw_preview(hEditorWnd, &fr, &paper, preview.page);

    if(preview.pages_shown > 1)
    {
        if (!preview.hdc2)
        {
            preview.hdc2 = CreateCompatibleDC(hdc);
            hBitmapCapture = CreateCompatibleBitmap(hdc,
                                                    preview.bmSize.cx,
                                                    preview.bmSize.cy);
            SelectObject(preview.hdc2, hBitmapCapture);
        }

        fr.hdc = preview.hdc2;
        draw_preview(hEditorWnd, &fr, &fr.rcPage, preview.page + 1);
    }
    DeleteDC(fr.hdcTarget);
    ReleaseDC(hwndPreview, hdc);

    InvalidateRect(hwndPreview, NULL, FALSE);
    update_preview_buttons(hMainWnd);
    update_preview_statusbar(hMainWnd);
}

static void toggle_num_pages(HWND hMainWnd)
{
    HWND hReBar = GetDlgItem(hMainWnd, IDC_REBAR);
    WCHAR name[MAX_STRING_LEN];
    HINSTANCE hInst = GetModuleHandleW(0);
    int nPreviewPages;

    preview.pages_shown = preview.pages_shown > 1 ? 1 : 2;

    nPreviewPages = preview.zoomlevel > 0 ? preview.saved_pages_shown :
                                            preview.pages_shown;

    LoadStringW(hInst, nPreviewPages > 1 ? STRING_PREVIEW_ONEPAGE :
                                           STRING_PREVIEW_TWOPAGES,
                name, MAX_STRING_LEN);

    SetWindowTextW(GetDlgItem(hReBar, ID_PREVIEW_NUMPAGES), name);
    update_preview_sizes(GetDlgItem(hMainWnd, IDC_PREVIEW), TRUE);
    update_preview(hMainWnd);
}

/* Returns the page shown that the point is in (1 or 2) or 0 if the point
 * isn't inside either page */
static int preview_page_hittest(POINT pt)
{
    RECT rc;
    rc.left = preview.spacing.cx;
    rc.right = rc.left + preview.bmScaledSize.cx;
    rc.top = preview.spacing.cy;
    rc.bottom = rc.top + preview.bmScaledSize.cy;
    if (PtInRect(&rc, pt))
        return 1;

    if (preview.pages_shown <= 1)
        return 0;

    rc.left += preview.bmScaledSize.cx + preview.spacing.cx;
    rc.right += preview.bmScaledSize.cx + preview.spacing.cx;
    if (PtInRect(&rc, pt))
        return is_last_preview_page(preview.page) ? 1 : 2;

    return 0;
}

LRESULT CALLBACK preview_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CREATE:
        {
            HWND hMainWnd = GetParent(hWnd);
            HWND hEditorWnd = GetDlgItem(hMainWnd, IDC_EDITOR);
            FORMATRANGE fr;
            GETTEXTLENGTHEX gt = {GTL_DEFAULT, 1200};
            HDC hdc = GetDC(hWnd);
            HDC hdcTarget = make_dc();

            fr.rc = preview.rcPage = get_print_rect(hdcTarget);
            preview.rcPage.bottom += margins.bottom;
            preview.rcPage.right += margins.right;
            preview.rcPage.top = preview.rcPage.left = 0;
            fr.rcPage = preview.rcPage;

            preview.bmSize.cx = twips_to_pixels(preview.rcPage.right, GetDeviceCaps(hdc, LOGPIXELSX));
            preview.bmSize.cy = twips_to_pixels(preview.rcPage.bottom, GetDeviceCaps(hdc, LOGPIXELSY));

            preview.textlength = SendMessageW(hEditorWnd, EM_GETTEXTLENGTHEX, (WPARAM)&gt, 0);

            fr.hdc = CreateCompatibleDC(hdc);
            fr.hdcTarget = hdcTarget;
            fr.chrg.cpMin = 0;
            fr.chrg.cpMax = preview.textlength;
            DeleteDC(fr.hdc);
            DeleteDC(hdcTarget);
            ReleaseDC(hWnd, hdc);

            update_preview_sizes(hWnd, TRUE);
            update_preview(hMainWnd);
            break;
        }

        case WM_PAINT:
            return print_preview(hWnd);

        case WM_SIZE:
        {
            update_preview_sizes(hWnd, FALSE);
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        }

        case WM_VSCROLL:
        case WM_HSCROLL:
        {
            SCROLLINFO si;
            RECT rc;
            int nBar = (msg == WM_VSCROLL) ? SB_VERT : SB_HORZ;
            int origPos;

            GetClientRect(hWnd, &rc);
            si.cbSize = sizeof(si);
            si.fMask = SIF_ALL;
            GetScrollInfo(hWnd, nBar, &si);
            origPos = si.nPos;
            switch(LOWORD(wParam))
            {
                case SB_TOP: /* == SB_LEFT */
                    si.nPos = si.nMin;
                    break;
                case SB_BOTTOM: /* == SB_RIGHT */
                    si.nPos = si.nMax;
                    break;
                case SB_LINEUP: /* == SB_LINELEFT */
                    si.nPos -= si.nPage / 10;
                    break;
                case SB_LINEDOWN: /* == SB_LINERIGHT */
                    si.nPos += si.nPage / 10;
                    break;
                case SB_PAGEUP: /* == SB_PAGELEFT */
                    si.nPos -= si.nPage;
                    break;
                case SB_PAGEDOWN: /* SB_PAGERIGHT */
                    si.nPos += si.nPage;
                    break;
                case SB_THUMBTRACK:
                    si.nPos = si.nTrackPos;
                    break;
            }
            si.fMask = SIF_POS;
            SetScrollInfo(hWnd, nBar, &si, TRUE);
            GetScrollInfo(hWnd, nBar, &si);
            if (si.nPos != origPos)
            {
                int amount = origPos - si.nPos;
                if (msg == WM_VSCROLL)
                    ScrollWindow(hWnd, 0, amount, NULL, NULL);
                else
                    ScrollWindow(hWnd, amount, 0, NULL, NULL);
            }
            return 0;
        }

        case WM_SETCURSOR:
        {
            POINT pt;
            RECT rc;
            int bHittest = 0;
            DWORD messagePos = GetMessagePos();
            pt.x = (short)LOWORD(messagePos);
            pt.y = (short)HIWORD(messagePos);
            ScreenToClient(hWnd, &pt);

            GetClientRect(hWnd, &rc);
            if (PtInRect(&rc, pt))
            {
                pt.x += GetScrollPos(hWnd, SB_HORZ);
                pt.y += GetScrollPos(hWnd, SB_VERT);
                bHittest = preview_page_hittest(pt);
            }

            if (bHittest)
                SetCursor(LoadCursorW(GetModuleHandleW(0),
                                      MAKEINTRESOURCEW(IDC_ZOOM)));
            else
                SetCursor(LoadCursorW(NULL, (WCHAR*)IDC_ARROW));

            return TRUE;
        }

        case WM_LBUTTONDOWN:
        {
            int page;
            POINT pt;
            pt.x = (short)LOWORD(lParam) + GetScrollPos(hWnd, SB_HORZ);
            pt.y = (short)HIWORD(lParam) + GetScrollPos(hWnd, SB_VERT);
            if ((page = preview_page_hittest(pt)) > 0)
            {
                HWND hMainWnd = GetParent(hWnd);

                /* Convert point from client coordinate to unzoomed page
                 * coordinate. */
                pt.x -= preview.spacing.cx;
                if (page > 1)
                    pt.x -= preview.bmScaledSize.cx + preview.spacing.cx;
                pt.y -= preview.spacing.cy;
                pt.x /= preview.zoomratio;
                pt.y /= preview.zoomratio;

                if (preview.zoomlevel == 0)
                    preview.saved_pages_shown = preview.pages_shown;
                preview.zoomlevel = (preview.zoomlevel + 1) % 3;
                preview.zoomratio = 0;
                if (preview.zoomlevel == 0 && preview.saved_pages_shown > 1)
                {
                    toggle_num_pages(hMainWnd);
                } else if (preview.pages_shown > 1) {
                    if (page >= 2) preview.page++;
                    toggle_num_pages(hMainWnd);
                } else {
                    update_preview_sizes(hWnd, TRUE);
                    InvalidateRect(hWnd, NULL, FALSE);
                    update_preview_buttons(hMainWnd);
                }

                if (preview.zoomlevel > 0) {
                    SCROLLINFO si;
                    /* Convert the coordinate back to client coordinate. */
                    pt.x *= preview.zoomratio;
                    pt.y *= preview.zoomratio;
                    pt.x += preview.spacing.cx;
                    pt.y += preview.spacing.cy;
                    /* Scroll to center view at that point on the page */
                    si.cbSize = sizeof(si);
                    si.fMask = SIF_PAGE;
                    GetScrollInfo(hWnd, SB_HORZ, &si);
                    pt.x -= si.nPage / 2;
                    SetScrollPos(hWnd, SB_HORZ, pt.x, TRUE);
                    GetScrollInfo(hWnd, SB_VERT, &si);
                    pt.y -= si.nPage / 2;
                    SetScrollPos(hWnd, SB_VERT, pt.y, TRUE);
                }
            }
        }

        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    return 0;
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
            toggle_num_pages(hWnd);
            break;

        case ID_PREVIEW_ZOOMIN:
            if (preview.zoomlevel < 2)
            {
                if (preview.zoomlevel == 0)
                    preview.saved_pages_shown = preview.pages_shown;
                preview.zoomlevel++;
                preview.zoomratio = 0;
                if (preview.pages_shown > 1)
                {
                    /* Forced switch to one page when zooming in. */
                    toggle_num_pages(hWnd);
                } else {
                    HWND hwndPreview = GetDlgItem(hWnd, IDC_PREVIEW);
                    update_preview_sizes(hwndPreview, TRUE);
                    InvalidateRect(hwndPreview, NULL, FALSE);
                    update_preview_buttons(hWnd);
                }
            }
            break;

        case ID_PREVIEW_ZOOMOUT:
            if (preview.zoomlevel > 0)
            {
                HWND hwndPreview = GetDlgItem(hWnd, IDC_PREVIEW);
                preview.zoomlevel--;
                preview.zoomratio = 0;
                if (preview.zoomlevel == 0 && preview.saved_pages_shown > 1) {
                    toggle_num_pages(hWnd);
                } else {
                    update_preview_sizes(hwndPreview, TRUE);
                    InvalidateRect(hwndPreview, NULL, FALSE);
                    update_preview_buttons(hWnd);
                }
            }
            break;

        case ID_PRINT:
            dialog_print(hWnd, preview.wszFileName);
            SendMessageW(hWnd, WM_CLOSE, 0, 0);
            break;
    }

    return 0;
}
