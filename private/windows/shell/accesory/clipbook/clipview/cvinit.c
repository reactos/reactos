
/*****************************************************************************

                    C L I P B O O K   I N I T

    Name:       cvinit.c
    Date:       21-Jan-1994
    Creator:    Unknown

    Description:

*****************************************************************************/



#define    OEMRESOURCE
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <math.h>
#include "common.h"
#include "clipbook.h"
#include "clipbrd.h"
#include "clipdsp.h"
#include "cvinit.h"
#include "debugout.h"





#define MAXINT 0X80000000



static int SSplit[] = { 200, 500 };
static int SBorders[3] = { 20, 0, 0 };





HWND    hwndToolbar = NULL;
HWND    hwndStatus  = NULL;
HBITMAP hbmStatus   = NULL;


TCHAR   szWindows[]   =   TEXT("Windows");



DWORD nIDs[] =
    {
    MH_BASE ,MH_POPUPBASE, 0, 0   /* This list must be NULL terminated */
    };

TBBUTTON tbButtons[] = {
    {0,  0,             TBSTATE_ENABLED, TBSTYLE_SEP,       0},
    {0,  IDM_CONNECT,   TBSTATE_ENABLED, TBSTYLE_BUTTON,    0},
    {1,  IDM_DISCONNECT,TBSTATE_ENABLED, TBSTYLE_BUTTON,    0},
    {0,  0,             TBSTATE_ENABLED, TBSTYLE_SEP,       0},
    {2,  IDM_SHARE,     TBSTATE_ENABLED, TBSTYLE_BUTTON,    0},
    {3,  IDM_UNSHARE,   TBSTATE_ENABLED, TBSTYLE_BUTTON,    0},
    {0,  0,             TBSTATE_ENABLED, TBSTYLE_SEP,       0},
#if 0
    {4,  IDM_UNDO,      TBSTATE_ENABLED, TBSTYLE_BUTTON,    0},
#endif
    {5,  IDM_COPY,      TBSTATE_ENABLED, TBSTYLE_BUTTON,    0},
    {6,  IDM_KEEP,      TBSTATE_ENABLED, TBSTYLE_BUTTON,    0},
    {7,  IDM_DELETE,    TBSTATE_ENABLED, TBSTYLE_BUTTON,    0},
    {0,  0,             TBSTATE_ENABLED, TBSTYLE_SEP,       0},
    {8,  IDM_LISTVIEW,  TBSTATE_ENABLED, TBSTYLE_CHECKGROUP,0},
    {9,  IDM_PREVIEWS,  TBSTATE_ENABLED, TBSTYLE_CHECKGROUP,0},
    {10, IDM_PAGEVIEW,  TBSTATE_ENABLED, TBSTYLE_CHECKGROUP,0}
};







static int atoi(
    TCHAR   *pch);


static int GetHeightFromPointsString(
    PTSTR   szPoints);








#if 0
static int abs(init x);

int abs(int x)
{
    return (x < 0 ? -x : x);
}
#endif






static int atoi(
    TCHAR   *pch)
{
int n;
int nSign = 1;

    n = 0;
    if (*pch == '-')
        {
        nSign = -1;
        pch++;
        }
    else if (*pch == '+')
        {
        pch++;
        }


    while (*pch)
        {
        n = n * 10 + (*pch - '0');
        pch++;
        }

    return n * nSign;

}






static int GetHeightFromPointsString(PTSTR szPoints)
{
HDC hdc;
int height;

    hdc = GetDC(NULL);
    height = MulDiv(-atoi(szPoints), GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(NULL, hdc);

    return height;
}






VOID LoadIntlStrings (void)
{
    LoadString (hInst,   IDS_HELV, szHelv,32);
    LoadString (hInst, IDS_APPNAME, szAppName, 32);
    LoadString (hInst, IDS_LOCALCLIP, szLocalClpBk, 32);
    LoadString (hInst, IDS_CLIPBOARD, szSysClpBrd, 32);
    LoadString (hInst, IDS_DATAUNAVAIL, szDataUnavail, 64);
    LoadString (hInst, IDS_READINGITEM, szReadingItem, 64);
    LoadString (hInst, IDS_VIEWHELPFMT,
                szViewHelpFmt, sizeof(szViewHelpFmt)/sizeof(szViewHelpFmt[0]));
    LoadString (hInst, IDS_ACTIVATEFMT,
                szActivateFmt, sizeof(szActivateFmt)/sizeof(szActivateFmt[0]));
    LoadString (hInst, IDS_RENDERING, szRendering, 64);
    LoadString (hInst, IDS_DEFFORMAT, szDefaultFormat, 64);
    LoadString (hInst, IDS_GETTINGDATA, szGettingData, 64);
    LoadString (hInst, IDS_ESTABLISHING, szEstablishingConn, 64);
    LoadString (hInst, IDS_CLIPBOOKONFMT, szClipBookOnFmt, 64);
    LoadString (hInst, IDS_PAGEFMT, szPageFmt, 32);
    LoadString (hInst, IDS_PAGEFMTPL, szPageFmtPl, 32);
    LoadString (hInst, IDS_PAGEOFPAGEFMT, szPageOfPageFmt, 32);
    LoadString (hInst, IDS_DELETE, szDelete, 32 );
    LoadString (hInst, IDS_DELETECONFIRMFMT, szDeleteConfirmFmt, 32);
    LoadString (hInst, IDS_FILEFILTER, szFileFilter, 64);
}






VOID SaveWindowPlacement (
    PWINDOWPLACEMENT    pwp )
{
WINDOWPLACEMENT wp;
int             dir_num = 0;
HWND            hwnd;


    // save main window placement

    if (hkeyRoot != NULL)
       {
       pwp->length = sizeof(WINDOWPLACEMENT);
       lstrcat(lstrcpy(szBuf2, szWindows), szAppName);
       RegSetValueEx(hkeyRoot,
             szBuf2,
             0L,
             REG_BINARY,
             (LPBYTE)pwp,
             sizeof(WINDOWPLACEMENT));

       // write out dir window strings in reverse order
       // so that when we read them back in we get the same Z order
       wp.length = sizeof (WINDOWPLACEMENT);
       wp.flags = 0;

       for (hwnd = GetWindow(hwndMDIClient, GW_CHILD); hwnd;
                hwnd = GetWindow(hwnd, GW_HWNDNEXT))
          {
          // don't save MDI icon title windows or search windows,
          // or any dir window which is currently recursing

          if (GetWindow(hwnd, GW_OWNER) == NULL &&
              GetWindowPlacement(hwnd, &wp) )
             {
             wp.length = sizeof(WINDOWPLACEMENT);
             wp.flags = (wp.showCmd == SW_SHOWMINIMIZED)
                   ? WPF_SETMINPOSITION : 0;

             if (GETMDIINFO(hwnd))
                 lstrcat(lstrcpy(szBuf2, szWindows), GETMDIINFO(hwnd)->szBaseName);

             RegSetValueEx(hkeyRoot,
                   szBuf2,
                   0L,
                   REG_BINARY,
                   (LPBYTE)&wp,
                   sizeof(wp));
             }
          }
       }

}







BOOL ReadWindowPlacement(
    LPTSTR              szKey,
    PWINDOWPLACEMENT    pwp)
{

    // AnsiToOem ( szKey, szBuf2 );
    if (hkeyRoot != NULL)
        {
        DWORD dwBufSize = sizeof(WINDOWPLACEMENT);

        lstrcat(lstrcpy(szBuf2, szWindows), szKey);
        RegQueryValueEx(hkeyRoot, szBuf2, NULL, NULL, (LPBYTE)pwp, &dwBufSize);

        if (pwp->length == sizeof(WINDOWPLACEMENT))
           {
           return TRUE;
           }
        else
           {
           PINFO(TEXT("ReadWindowPlacement: QueryValue failed\n\r"));
           }
        return FALSE;
        }

    PINFO(TEXT("ReadWindowPlacement: no entry\n\r"));

    return FALSE;

}









BOOL CreateTools(
    HWND    hwnd)
{
HDC hdc;
TEXTMETRIC tm;
int dyBorder;
int cx,cy;
HFONT hTmpFont;
SIZE  size;




    if ( !(hbrBackground = CreateSolidBrush ( GetSysColor(COLOR_WINDOW) )))
        return FALSE;


    if ( !(hbmStatus = CreateMappedBitmap(hInst, IDSTATUS, FALSE, NULL, 0)))
        return FALSE;


    // create toolbar and status bar windows
    // has all buttons initially...

    if ( !(hwndToolbar = CreateToolbarEx (hwnd,
                                          (fToolBar?WS_VISIBLE:0)|WS_BORDER|TBSTYLE_TOOLTIPS,
                                          IDC_TOOLBAR,
                                          11,
                                          hInst,
                                          IDBITMAP,
                                          tbButtons,
                                          sizeof(tbButtons)/sizeof(TBBUTTON),
                                          0,0,0,0,sizeof(TBBUTTON))))
        return FALSE;



    // get rid of share buttons?
    if (!fShareEnabled)
        {
        SendMessage (hwndToolbar, TB_DELETEBUTTON, 3,  0L);
        SendMessage (hwndToolbar, TB_DELETEBUTTON, 3,  0L);
        SendMessage (hwndToolbar, TB_DELETEBUTTON, 3,  0L);
        }


    // get rid of connect/disonnect buttons?
    if (!fNetDDEActive)
        {
        SendMessage (hwndToolbar, TB_DELETEBUTTON, 0,  0L);
        SendMessage (hwndToolbar, TB_DELETEBUTTON, 0,  0L);
        SendMessage (hwndToolbar, TB_DELETEBUTTON, 0,  0L);
        }


    if ( fToolBar )
        ShowWindow ( hwndToolbar, SW_SHOW );




    // create status bar

    if ( !(hwndStatus = CreateStatusWindow ((fStatus?WS_VISIBLE:0)|WS_BORDER|WS_CHILD|WS_CLIPSIBLINGS,
                                            szNull,
                                            hwnd,
                                            IDSTATUS )))
       return FALSE;


    // now build the parameters based on the font we will be using

    dyBorder = GetSystemMetrics(SM_CYBORDER);
    cx = GetSystemMetrics (SM_CXVSCROLL);
    cy = GetSystemMetrics (SM_CYHSCROLL);





    if ( hdc= GetDC(NULL) )
        {
        CHARSETINFO csi;
        DWORD dw = GetACP();
        LOGFONT lfDef; 

        GetObject( GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lfDef );

        if (!TranslateCharsetInfo((DWORD*)(DWORD_PTR)dw, &csi, TCI_SRCCODEPAGE))
            csi.ciCharset = ANSI_CHARSET;

        // hPreviewBmp = CreateCompatibleBitmap ( hdc, 64, 64 );
        hPreviewBmp = CreateBitmap ( 64, 64, 1, 1, NULL );

        hBtnDC = CreateCompatibleDC ( hdc );

        hPgUpBmp = LoadBitmap ( hInst, MAKEINTRESOURCE(IBM_UPARROW) );
        hPgDnBmp = LoadBitmap ( hInst, MAKEINTRESOURCE(IBM_DNARROW) );
        hPgUpDBmp = LoadBitmap ( hInst, MAKEINTRESOURCE(IBM_UPARROWD) );
        hPgDnDBmp = LoadBitmap ( hInst, MAKEINTRESOURCE(IBM_DNARROWD) );
        // modify arrows
        // hOldBitmap = SelectObject ( hBtnDC, hPgUpBmp );
        // BitBlt ( hBtnDC, 4, cy/2 + 1, cx-8, (cy-8)/2, hBtnDC,
        //    4, cy/2 - 3, SRCCOPY );
        // SelectObject ( hBtnDC, hOldBitmap );

        hFontPreview = CreateFont (lfDef.lfHeight,
                                   0,
                                   0,
                                   0,
                                   400,
                                   0,
                                   0,
                                   0,
                                   csi.ciCharset,
                                   OUT_DEFAULT_PRECIS,
                                   CLIP_DEFAULT_PRECIS,
                                   DEFAULT_QUALITY,
                                   VARIABLE_PITCH | FF_SWISS,
                                   szHelv);


        hOldFont = SelectObject(hdc, hFontPreview);

        GetTextMetrics(hdc, &tm);
        dyPrevFont = tm.tmHeight + tm.tmExternalLeading;



        if (hOldFont)
           SelectObject(hdc, hOldFont);

        // figure out where to put the first status bar splitpoint
        SendMessage ( hwndStatus, SB_GETBORDERS, 0, (LPARAM)(LPBYTE)&SBorders );

        if ( hTmpFont = (HFONT)SendMessage(hwndStatus, WM_GETFONT, 0, 0L ))
            {
            if ( hOldFont = SelectObject ( hdc, hTmpFont ))
                {
                wsprintf ( szBuf, szPageOfPageFmt, 888, 888 );

                GetTextExtentPoint(hdc, szBuf, lstrlen(szBuf),&size);
                SSplit[0] = size.cx + 2 * GetSystemMetrics(SM_CXBORDER)
                   + 2 * SBorders[0];   //BUGBUG doesn't quite wash

                if ( hOldFont )
                    SelectObject( hdc, hOldFont );
                }
            }
        ReleaseDC(NULL, hdc);

        if ( !hTmpFont || !hPgUpBmp || !hPgDnBmp || !hPgUpDBmp ||
           !hFontPreview || !hPreviewBmp || !hBtnDC )
        return FALSE;
        }
    else
        return FALSE;

    //second split point is fixed for now.

    SendMessage ( hwndStatus, SB_SETPARTS, 2, (LPARAM)(LPBYTE)&SSplit );
    return TRUE;

}








VOID DeleteTools (
    HWND    hwnd)
{
    DeleteDC ( hBtnDC );

    DeleteObject ( hPreviewBmp );

    DeleteObject ( hbmStatus );
    DeleteObject ( hFontPreview );
    DeleteObject ( hbrBackground );

    DeleteObject ( hPgUpBmp );
    DeleteObject ( hPgDnBmp );
    DeleteObject ( hPgUpDBmp );
    DeleteObject ( hPgDnDBmp );

    DestroyWindow(hwndToolbar);
    DestroyWindow(hwndStatus);
}
