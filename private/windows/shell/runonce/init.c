//---------------------------------------------------------------------------
// Windows 4.0 Task Switcher. Copyright Microsoft Corp. 1993.
// Insept: May 1993     IanEl.
// Bastardised for RunOnce by FelixA.
//---------------------------------------------------------------------------
// #include <windows.h>
#include "runonce.h"

//---------------------------------------------------------------------------
// Global to everybody...
HINSTANCE g_hinst;
HWND g_hwndLB = NULL;
HWND g_hwndMain = NULL;
HWND g_hwndStatus = NULL;
const TCHAR g_szNull[] = TEXT("");

// Icon sizes.
int g_cxIcon = 0;
int g_cyIcon = 0;
int g_cxSmIcon = 0;
int g_cySmIcon = 0;
// Extent of text in buttons.
DWORD g_dwBTextExt = 0;
SIZE g_SizeTextExt;

//---------------------------------------------------------------------------
// Global to this file only...

HFONT g_hfont = NULL;
HFONT g_hBoldFont=NULL;

static int g_iItemCur = 0;
static TCHAR g_szLotsaWs[] = TEXT("WWWWWWWWWW");
HBRUSH g_hbrBkGnd = NULL;

//---------------------------------------------------------------------------
BOOL   CreateGlobals(HWND hwndCtl)
{
    LOGFONT lf;
    HDC hdc;
    HFONT hfontOld;

    g_cxIcon = GetSystemMetrics(SM_CXICON);
    g_cyIcon = GetSystemMetrics(SM_CYICON);
    g_cxSmIcon = GetSystemMetrics(SM_CXSMICON);
    g_cySmIcon = GetSystemMetrics(SM_CYSMICON);
    g_hbrBkGnd = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
//    if (SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0))
    if ( (hfontOld = (HFONT)(WORD)SendMessage( hwndCtl, WM_GETFONT, 0, 0L )) != NULL )
    {
        if ( GetObject( hfontOld, sizeof(LOGFONT), (LPTSTR) &lf ) )
        {
            lf.lfWeight=400;
            g_hfont = CreateFontIndirect(&lf);
            lf.lfWeight=700;
            // lf.lfItalic=TRUE;
            g_hBoldFont = CreateFontIndirect(&lf);
        }
    }
        
    if (g_hfont)
    {
        // Calc sensible size for text in buttons.
        hdc = GetDC(NULL);
        hfontOld = SelectObject(hdc, g_hfont);
        GetTextExtentPoint(hdc, g_szLotsaWs, lstrlen(g_szLotsaWs), &g_SizeTextExt);
        SelectObject(hdc, hfontOld);
        ReleaseDC(NULL, hdc);
        return TRUE;
    }
    return FALSE;
}

//---------------------------------------------------------------------------
VOID   DestroyGlobals(void)
{
        if (g_hfont)
                DeleteObject(g_hfont);
        if (g_hBoldFont)
                DeleteObject(g_hBoldFont);
        if (g_hbrBkGnd)
                DeleteObject(g_hbrBkGnd);
}

