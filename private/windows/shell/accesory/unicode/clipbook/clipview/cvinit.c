
/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991,1992                    **/
/***************************************************************************/
// ts=4

// CVINIT.C - ClipBook viewer initialization routines
// 4-92 clausgi created

#define    OEMRESOURCE
#include "windows.h"
#include "clipbook.h"
#include <commctrl.h>
#include <stdio.h>
#include <math.h>

#ifdef JAPAN
TCHAR szSystem[] = TEXT("System");
#endif

#if 0

int abs(int x) {
return (x < 0 ? -x : x);
}
#endif


#define MAXINT 0X80000000

int Atoi(
TCHAR *pch)
{
int n;
int nSign;

n = 0;
if (*pch = TEXT('-'))
   {
   nSign = -1;
   pch++;
   }
else
   {
   nSign = 1;
   }

while (*pch)
   {
   n = n * 10 + (*pch - TEXT('0'));
   }

return n * nSign;
}


HWND hwndToolbar = NULL;
HWND hwndStatus = NULL;
HBITMAP hbmStatus = NULL;

// forwards
static int NEAR GetHeightFromPointsString(PTSTR szPoints);

TCHAR   szWindows[]   =   TEXT("Windows");

DWORD nIDs[] =
   {
   MH_BASE ,MH_POPUPBASE, 0, 0   /* This list must be NULL terminated */
   };

TBBUTTON tbButtons[] = {
   { 0, IDM_CONNECT,      TBSTATE_ENABLED, TBSTYLE_BUTTON,  0},
   { 1, IDM_DISCONNECT,   TBSTATE_ENABLED, TBSTYLE_BUTTON,  0},
   { 0, 0,                TBSTATE_ENABLED, TBSTYLE_SEP,         0},
   { 2, IDM_SHARE,          TBSTATE_ENABLED, TBSTYLE_BUTTON,     0},
   { 3, IDM_UNSHARE,      TBSTATE_ENABLED, TBSTYLE_BUTTON,     0},
   { 0, 0,                TBSTATE_ENABLED, TBSTYLE_SEP,         0},
#if 0
   { 4, IDM_UNDO,         TBSTATE_ENABLED, TBSTYLE_BUTTON,  0},
#endif
   { 5, IDM_COPY,          TBSTATE_ENABLED, TBSTYLE_BUTTON,  0},
   { 6, IDM_KEEP,         TBSTATE_ENABLED, TBSTYLE_BUTTON,  0},
   { 7, IDM_DELETE,      TBSTATE_ENABLED, TBSTYLE_BUTTON,  0},
   { 0, 0,                TBSTATE_ENABLED, TBSTYLE_SEP,         0},
   { 8, IDM_LISTVIEW,       TBSTATE_ENABLED, TBSTYLE_CHECKGROUP,  0},
   { 9, IDM_PREVIEWS,      TBSTATE_ENABLED, TBSTYLE_CHECKGROUP,  0},
   { 10, IDM_PAGEVIEW,      TBSTATE_ENABLED, TBSTYLE_CHECKGROUP,  0}
};


static int NEAR GetHeightFromPointsString(PTSTR szPoints)
{
   HDC hdc;
   int height;

   hdc = GetDC(NULL);
   height = MulDiv(-Atoi(szPoints), GetDeviceCaps(hdc, LOGPIXELSY), 72);
   ReleaseDC(NULL, hdc);

   return height;
}


VOID LoadIntlStrings ( VOID )
{
   LoadString ( hInst, IDS_HELV, szHelv,32 );
   LoadString ( hInst, IDS_APPNAME, szAppName, 32 );
   LoadString ( hInst, IDS_LOCALCLIP, szLocalClpBk, 32 );
   LoadString ( hInst, IDS_CLIPBOARD, szSysClpBrd, 32 );
   LoadString ( hInst, IDS_DATAUNAVAIL, szDataUnavail, 64 );
   LoadString ( hInst, IDS_READINGITEM, szReadingItem, 64 );

#ifdef JAPAN
   LoadString ( hInst, IDS_VIEWHELPFMT, szViewHelpFmt, 64 );
   LoadString ( hInst, IDS_ACTIVATEFMT, szActivateFmt, 64 );
#else
   LoadString ( hInst, IDS_VIEWHELPFMT, szViewHelpFmt, 32 );
   LoadString ( hInst, IDS_ACTIVATEFMT, szActivateFmt, 32 );
#endif

   LoadString ( hInst, IDS_RENDERING, szRendering, 64 );
   LoadString ( hInst, IDS_DEFFORMAT, szDefaultFormat, 64 );
   LoadString ( hInst, IDS_GETTINGDATA, szGettingData, 64 );
   LoadString ( hInst, IDS_ESTABLISHING, szEstablishingConn, 64 );
   LoadString ( hInst, IDS_CLIPBOOKONFMT, szClipBookOnFmt, 64 );
   LoadString ( hInst, IDS_PAGEFMT, szPageFmt, 32 );
   LoadString ( hInst, IDS_PAGEFMTPL, szPageFmtPl, 32 );
   LoadString ( hInst, IDS_PAGEOFPAGEFMT, szPageOfPageFmt, 32 );
   LoadString ( hInst, IDS_DELETE, szDelete, 32 );
   LoadString ( hInst, IDS_DELETECONFIRMFMT, szDeleteConfirmFmt, 32 );
   LoadString ( hInst, IDS_FILEFILTER, szFileFilter, 64 );
}


VOID SaveWindowPlacement (
PWINDOWPLACEMENT pwp )
{
int dir_num = 0;
HWND hwnd;
WINDOWPLACEMENT wp;

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

         lstrcat(lstrcpy(szBuf2, szWindows),
               GETMDIINFO(hwnd)->szBaseName);
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
LPTSTR szKey,
PWINDOWPLACEMENT pwp )
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

int SSplit[] = { 200, 500 };
int SBorders[3] = { 20, 0, 0 };

BOOL CreateTools( HWND hwnd )
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

   if ( !(hwndToolbar = CreateToolbar(hwnd, (fToolBar?WS_VISIBLE:0)|WS_BORDER,
                     IDC_TOOLBAR, 12, hInst, IDBITMAP, tbButtons,
                     sizeof(tbButtons)/sizeof(TBBUTTON))))
      return FALSE;

   // get rid of share buttons?
   if ( !fShareEnabled ) {
      SendMessage ( hwndToolbar, TB_DELETEBUTTON, 3,  0L );
      SendMessage ( hwndToolbar, TB_DELETEBUTTON, 3,  0L );
      SendMessage ( hwndToolbar, TB_DELETEBUTTON, 3,  0L );
   }

   // get rid of connect/disonnect buttons?
   if ( !fNetDDEActive ) {
      SendMessage ( hwndToolbar, TB_DELETEBUTTON, 0,  0L );
      SendMessage ( hwndToolbar, TB_DELETEBUTTON, 0,  0L );
      SendMessage ( hwndToolbar, TB_DELETEBUTTON, 0,  0L );
   }

   if ( fToolBar )
      ShowWindow ( hwndToolbar, SW_SHOW );

   // create status bar

   if ( !(hwndStatus = CreateStatusWindow(
               (fStatus?WS_VISIBLE:0)|WS_BORDER|WS_CHILD,
               szNull, hwnd, IDSTATUS ) ))
      return FALSE;

   // now build the parameters based on the font we will be using

   dyBorder = GetSystemMetrics(SM_CYBORDER);
   cx = GetSystemMetrics ( SM_CXVSCROLL );
   cy = GetSystemMetrics ( SM_CYHSCROLL );

   if ( hdc= GetDC(NULL) ) {
      // hPreviewBmp = CreateCompatibleBitmap ( hdc, 64, 64 );
      hPreviewBmp = CreateBitmap ( 64, 64, 1, 1, NULL );

      hBtnDC = CreateCompatibleDC ( hdc );

      hPgUpBmp = LoadBitmap ( hInst, (LPTSTR) MAKEINTRESOURCE(IBM_UPARROW) );
      hPgDnBmp = LoadBitmap ( hInst, (LPTSTR) MAKEINTRESOURCE(IBM_DNARROW) );
      hPgUpDBmp = LoadBitmap ( hInst, (LPTSTR) MAKEINTRESOURCE(IBM_UPARROWD) );
      hPgDnDBmp = LoadBitmap ( hInst, (LPTSTR) MAKEINTRESOURCE(IBM_DNARROWD) );
      // modify arrows
      // hOldBitmap = SelectObject ( hBtnDC, hPgUpBmp );
      // BitBlt ( hBtnDC, 4, cy/2 + 1, cx-8, (cy-8)/2, hBtnDC,
      //    4, cy/2 - 3, SRCCOPY );
      // SelectObject ( hBtnDC, hOldBitmap );

#ifdef JAPAN
      hFontPreview = CreateFont(GetHeightFromPointsString(TEXT("8")),
         0, 0, 0, 400, 0, 0, 0, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS,
         CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
         VARIABLE_PITCH | FF_SWISS, szSystem
      );
#else
      hFontPreview = CreateFont(GetHeightFromPointsString(TEXT("8")),
         0, 0, 0, 400, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
         CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
         VARIABLE_PITCH | FF_SWISS, szHelv
      );
#endif

      hOldFont = SelectObject(hdc, hFontPreview);

      GetTextMetrics(hdc, &tm);
      dyPrevFont = tm.tmHeight + tm.tmExternalLeading;

      if (hOldFont)
         SelectObject(hdc, hOldFont);

      // figure out where to put the first status bar splitpoint
      SendMessage ( hwndStatus, SB_GETBORDERS, 0, (LPARAM)(LPBYTE)&SBorders );

      if ( hTmpFont = (HFONT)SendMessage(hwndStatus, WM_GETFONT, 0, 0L )) {
         if ( hOldFont = SelectObject ( hdc, hTmpFont )) {
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


VOID DeleteTools ( HWND hwnd )
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

