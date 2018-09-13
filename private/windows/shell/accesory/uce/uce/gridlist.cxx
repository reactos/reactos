/********************************************************************
 *
 *  Module Name : gridlist.c
 *
 *  List box (one dim) and Grid (two dim) controls for
 *  parsing and viewing subsets
 *
 *  History :
 *       Sep 03, 1997  [samera]    wrote it.
 *
 *  Copyright (c) 1997-1999 Microsoft Corporation. 
 **********************************************************************/


// BUGBUG: This stuff should really go into a precomp header
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "UCE.h"
#include "ucefile.h"

// global stuff

// Listbox
HWND  ghwnd     = NULL;
HWND  ghwndList = NULL;

const WCHAR wszListWndClass[]=L"LISTWNDCLASS";

#define LISTBOX_WIDTH    210
#define LISTBOX_HEIGHT   210

#define DIALOG_FONT_SIZE 8
#define DIALOG_FONT_NAME L"Microsoft Sans Serif"

// Grid Window
const WCHAR wszGridWndClass[]=L"GRIDWNDCLASS";
HWND  ghwndGrid = NULL;
HFONT hFont     = NULL;
HFONT hfList    = NULL;
HFONT hfStatic  = NULL;
HPEN  hpenDash  = NULL;

PUCE_MEMORY_FILE pGridUceMemFile = NULL;          // current active UCE file

// Grid Metrics
#define PAGE_SCROLL_SIZE 3
#define GRID_XANCHOR     5
#define GRID_YANCHOR     20
#define GRID_CELL_WIDTH  22
#define GRID_CELL_HEIGHT 22
#define GRID_MAX_COL     12
#define GRID_MAX_ROW     10
#define GRID_BOTTOM_SPACE 5
#define TITLE_X           5
#define TITLE_Y           2

INT  Grid_XAnchor;

INT  gnCurRow    = -1;
INT  gnCurCol    = -1;
INT  gnRowOffset = 0;
INT  gnColOffset = 0;
INT  gnMaxRow    = 0;
INT  gnMaxCol    = 0;
BOOL gbInDrag    = FALSE;
RECT grcGridWindow;
HWND ghwndHScroll, ghwndVScroll;
PWSTR gpwszWindowTitle;
WCHAR TableName[256];

WNDPROC fnOldListBox = NULL;
LRESULT CALLBACK EscapeProc (HWND, UINT, WPARAM, LPARAM);

/******************************Public*Routine******************************\
* CreateListWindow
*
* Create a list box in a window and set its title
*
* Return Value:
*
*   handle of create list window
*
\**************************************************************************/
HWND CreateListWindow( HWND hwndParent , PWSTR pwszWindowTitle )
{
  TEXTMETRIC tm;
  HDC   hDC;
  RECT  rc;
  HWND  hwnd;
  WCHAR buffer[64];
  int   xFrame, yFrame;
   LONG xpos;
   HWND dsktop;
   RECT drc;
   DWORD dwExStyle;

  gpwszWindowTitle = pwszWindowTitle;
  if(*pwszWindowTitle == L'0')
  {
     LoadString(hInst, _wtol(pwszWindowTitle), TableName, 256);
     gpwszWindowTitle = TableName;
  }

  // Let's know where to position it exactly
  if( NULL == ghwnd )
  {
    if( !UpdateListFont( hwndParent , ID_FONT ))
    {
      return NULL;
    }

    GetWindowRect( hwndParent , &rc );
    LoadString(hInst, IDS_GROUPBY,  buffer, 64);

      //fix for group by window too right
         dsktop = GetDesktopWindow();
         GetWindowRect(dsktop, &drc);
         if (rc.right + LISTBOX_WIDTH > drc.right)
            xpos = rc.left - LISTBOX_WIDTH;
         else
            xpos = rc.right;

    // Set the mirroring ExStyle if the main window has it.
    dwExStyle = GetWindowLong(hwndParent, GWL_EXSTYLE) & WS_EX_LAYOUTRTL;
    hwnd = CreateWindowEx( dwExStyle ,
                           wszListWndClass,
                           buffer,
                           WS_CAPTION | WS_BORDER | WS_VISIBLE |
                           WS_SYSMENU | WS_EX_CLIENTEDGE,
                           xpos,
                           rc.top,
                           LISTBOX_WIDTH,
                           LISTBOX_HEIGHT,
                           hwndParent, //if not use hwndParent, main window will come to top when activated
                           NULL,
                           hInst,
                           NULL
                         );

    xFrame = GetSystemMetrics( SM_CXSIZEFRAME );
    yFrame = GetSystemMetrics( SM_CYSIZEFRAME );

    if( NULL != hwnd )
    {
      HFONT hFontOld;
      ghwnd = hwnd;

      GetClientRect( hwnd , &rc );

      hDC = GetDC(hwndParent);
      hFontOld = (HFONT)SelectObject(hDC, hfStatic);

      // Find the height of the string
      GetTextMetrics(hDC, &tm);

      SelectObject(hDC, hFontOld);
      ReleaseDC(hwndParent, hDC);

         
      // Let's create the list box
      ghwndList = CreateWindowEx( WS_EX_CLIENTEDGE,
                                  L"LISTBOX",
                                  L"",
                                  WS_CHILD | WS_VISIBLE | LBS_NOTIFY
                                  | WS_VSCROLL | WS_HSCROLL,
                                  xFrame,
                                  tm.tmHeight + yFrame,
                                  rc.right - 2*xFrame,
                                  rc.bottom - tm.tmHeight - yFrame,
                                  hwnd,
                                  (HMENU)ID_LISTBOX,
                                  hInst,
                                  NULL
                                ) ;

      fnOldListBox = (WNDPROC)SetWindowLongPtr (ghwndList, GWLP_WNDPROC,
                                              (LPARAM) EscapeProc) ;

      SendMessage( ghwndList,
                   LB_RESETCONTENT,
                   0,
                   0
                 );
      ShowWindow( hwnd , SW_NORMAL );
      SetFocus(ghwndList);
    }
  }
  else
  {
      InvalidateRect( ghwnd , NULL , FALSE );
  }

  return ghwnd;
}

/******************************Public*Routine******************************\
* EscapeProc
*
* so that Escape key can close the window
*
\**************************************************************************/
LRESULT CALLBACK EscapeProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   if (iMsg == WM_KEYDOWN)
   {
      switch(wParam)
      {
        case VK_ESCAPE:
          {
              SendMessage(ghwnd, WM_CLOSE, 0, 0L);
              return 0L;
          }
          break;

        case VK_F6:
          {

              SetFocus(hwndCharGrid);
              return 0L;
          }
          break;
      }
   }

  return CallWindowProc (fnOldListBox, hwnd, iMsg, wParam, lParam) ;
}

/******************************Public*Routine******************************\
* InitListWindow
*
* Register ListBox main window
*
* Return Value:
*
*   TRUE if successful operation, FALSE otherwise
*
\**************************************************************************/
BOOL InitListWindow( HINSTANCE hInstance )
{
  WNDCLASS  wcListWnd;

  wcListWnd.style = 0;
  wcListWnd.lpfnWndProc = (WNDPROC)ListWndProc;
  wcListWnd.cbClsExtra = 0;
  wcListWnd.cbWndExtra = 0;
  wcListWnd.hInstance = hInstance;
  wcListWnd.hIcon = NULL;
  wcListWnd.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcListWnd.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wcListWnd.lpszMenuName =  NULL;
  wcListWnd.lpszClassName = wszListWndClass;

  return (RegisterClass(&wcListWnd));
}


/******************************Public*Routine******************************\
* ListWndProc
*
* ListBox's MainWindow Callback Proc
*
* Return Value:
*
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
*
\**************************************************************************/
LRESULT CALLBACK ListWndProc( HWND hWnd , UINT uMsg , WPARAM wParam , LPARAM lParam )
{
  switch( uMsg )
  {
  case WM_CREATE:
    {
      SendMessage(ghwndList, LB_SETHORIZONTALEXTENT, (WPARAM)LISTBOX_WIDTH,
      (LPARAM)0L);
    }
    break;

  case WM_SETFOCUS:
    {
      SetFocus(ghwndList);
    }
    break;

  case WM_COMMAND:
    {
      switch( LOWORD(wParam) )
      {
      case ID_LISTBOX:
        {
          if( HIWORD(wParam) == LBN_SELCHANGE )
          {
            SubSetChanged( hwndDialog );
          }
        }

      default:
        return (DefWindowProc(hWnd, uMsg, wParam, lParam));
        break;
      }
    }
    break;

  case WM_PAINT:
    {
      PAINTSTRUCT  ps;
      HDC          hPaintDC;
      HFONT        hOldFont;

      BeginPaint(hWnd, &ps);
      hPaintDC = ps.hdc;
      hOldFont = (HFONT)SelectObject(hPaintDC, hfStatic);
      SetBkColor(hPaintDC, GetSysColor(COLOR_BTNFACE));
      TextOut(hPaintDC, TITLE_X, TITLE_Y, gpwszWindowTitle,
        wcslen(gpwszWindowTitle));
      SelectObject(hPaintDC, hOldFont);
      EndPaint(hWnd, &ps);
    }
    break;

  case WM_CLOSE:
    {
      WCHAR buffer[64];

      SendMessage( GetDlgItem(hwndDialog, ID_UNICODESUBSET),
                   CB_SETCURSEL,
                   (WPARAM) 0,
                   (LPARAM) 0);
      LoadString(hInst, IDS_RESET, buffer, 64);
      SetDlgItemText(hwndDialog, ID_SEARCH, buffer);
      EnableWindow(GetDlgItem(hwndDialog, ID_SEARCH), TRUE);
      SetSearched();
      SetFocus(GetDlgItem(hwndDialog, ID_SEARCH));

      DestroyWindow(hWnd);
    }
    break;

  case WM_DESTROY:
    {
      EnableWindow(GetDlgItem(hwndDialog, ID_VIEW), TRUE);
      EnableSURControls(hwndDialog);
      ghwndList = ghwnd = NULL;

      // falls thru
    }

  default:
    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
  }

  return 0L;
}


/******************************Public*Routine******************************\
* FillGroupsInListBox
*
* Fill listbox with Groups of current UCE_MEMORY_FILE
*
* Return Value:
*
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
*
\**************************************************************************/
void FillGroupsInListBox( HWND hWnd , PUCE_MEMORY_FILE pUceMemFile )
{
  PSTR        pFile   = (char*) pUceMemFile->pvData;
  PUCE_HEADER pHeader = (PUCE_HEADER)pUceMemFile->pvData;
  PUCE_GROUP  pGroup  = (PUCE_GROUP)(pFile+sizeof(UCE_HEADER));
  INT         nGroups = ((PUCE_HEADER)pFile)->NumGroup;
  INT         i;
  INT         nIndex;

  // Reset content
  if(NULL == ghwndList)
  {
    // let's create everything!
    if( !CreateListWindow( hWnd , (PWSTR) (pFile+pHeader->OffsetTableName) ))
      return ;
  }
  else
  {
    SendMessage( ghwndList,
                 LB_RESETCONTENT,
                 0,
                 0
               );
  }

  // Set the drawing font
  SendMessage( ghwndList ,
               WM_SETFONT ,
               (WPARAM)hfList,
               (LPARAM)MAKELONG(TRUE,0)
             );

  // Fill list box
  i=0;
  while( i<nGroups )
  {
    WCHAR wcBuf[256];

    PWSTR pwszGroup = (PWSTR)(pFile+pGroup->OffsetGroupName);
    if(*pwszGroup == L'0')
    {
        LoadString(hInst, _wtol(pwszGroup), wcBuf, 255);
        pwszGroup = wcBuf;
    }
    nIndex = (INT)SendMessage( ghwndList,
                               LB_ADDSTRING,
                               (WPARAM) 0,
                               (LPARAM) pwszGroup
                             );

    if (nIndex != LB_ERR)
    {
      SendMessage( ghwndList,
                   LB_SETITEMDATA,
                   (WPARAM) nIndex,
                   (LPARAM) pGroup
                 );
    }
    pGroup++;
    i++;
  }

  // Set current selection to the 1st element
  SendMessage( ghwndList,
               LB_SETCURSEL,
               (WPARAM) 0,
               (LPARAM) 0L
             );

  return;
}


/******************************Public*Routine******************************\
* GetUnicodeCharsFromList
*
* Fills a buffer with unicode chars according to current group
* of (UCE_MEMORY_FILE) selection
*
* Return Value:
*   TRUE if successful, FALSE otherwise
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
*
\**************************************************************************/
BOOL GetUnicodeCharsFromList( HWND hWnd , PUCE_MEMORY_FILE pUceMemFile , PWSTR pwBuf  , UINT *puNum , BOOL *pbLineBreak)
{
  INT         nIndex;
  PUCE_GROUP  pGroup;
  BOOL        bRet=FALSE;
  PWSTR       pwCh;
  PSTR        pFile = (char*)pUceMemFile->pvData;


  // Reset content
  if(NULL == ghwndList)
  {
    PUCE_HEADER pHeader = (PUCE_HEADER)pFile;
    // let's create everything!
    if( !CreateListWindow( hWnd , (PWSTR) (pFile+pHeader->OffsetTableName) ))
      return FALSE;
  }

  nIndex = (INT)SendMessage( ghwndList,
                             LB_GETCURSEL,
                             (WPARAM) 0,
                             (LPARAM) 0L
                           );

  if( nIndex != LB_ERR )
  {
    pGroup = (PUCE_GROUP) SendMessage( ghwndList,
                                       LB_GETITEMDATA,
                                       (WPARAM) nIndex,
                                       (LPARAM) 0L
                                       );

    // Fill in buffer

    *puNum = pGroup->NumChar;
    pwCh   = (PWSTR) (pFile+pGroup->OffsetGroupChar);

    GetWChars( *puNum , pwCh , pwBuf , puNum , pbLineBreak);

    bRet = TRUE;
  }

  return bRet;
}


/******************************Public*Routine******************************\
* DestroyListWindow
*
* Destroy the list window, if active
*
* Return Value:

* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
void DestroyListWindow( void )
{
  if( ghwnd )
  {
    DestroyWindow( ghwnd );
  }

  return;
}


/******************************Public*Routine******************************\
* DestroyAllListWindows
*
* Destroy the list & grid window, if active. This is called when exiting
* UCE
*
* Return Value:
*
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
void DestroyAllListWindows( void )
{
  DestroyListWindow();
  DestroyGridWindow();

  return;
}


/******************************Public*Routine******************************\
* IsListWindow
*
* Checks type of grid needed (1d or 2d) according to current active
* UCE_MEMORY_FILE
*
* Return Value:
*
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
BOOL IsListWindow( PUCE_MEMORY_FILE pUceMemFile )
{
  BOOL        bRet = FALSE;
  PUCE_HEADER pHeader = (PUCE_HEADER)(pUceMemFile->pvData);

  if( pHeader->Row )
  {
    // Grid Window
    bRet = (ghwndGrid != NULL);
  }
  else
  {
    // List Window
    bRet = (ghwnd != NULL);
  }

  return bRet;
}


/*
 *******************************************
 *       Grid Window Control               *
 *******************************************/


/******************************Public*Routine******************************\
* InitGridWindow
*
* Register class of GridWindow
*
* Return Value:
*   TRUE if successful, FALSE otherwise
*
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
BOOL InitGridWindow( HINSTANCE hInstance )
{
  WNDCLASS  wcGridWnd;

  wcGridWnd.style = 0;
  wcGridWnd.lpfnWndProc = (WNDPROC)GridWndProc;
  wcGridWnd.cbClsExtra = 0;
  wcGridWnd.cbWndExtra = 0;
  wcGridWnd.hInstance = hInstance;
  wcGridWnd.hIcon = NULL ;
  wcGridWnd.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcGridWnd.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
  wcGridWnd.lpszMenuName =  NULL;
  wcGridWnd.lpszClassName = wszGridWndClass;

  return (RegisterClass(&wcGridWnd));
}


/******************************Public*Routine******************************\
* CreateGridWindow
*
* Create Grid window and fills it with proper values from current UCE_MEMORY_FILE
*
* Return Value:
*   handle to grid window
*
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
HWND CreateGridWindow( HWND hwndParent , UINT uID , PUCE_MEMORY_FILE pUceMemFile )
{
  RECT rc;
  HWND hwnd;
  WCHAR buffer[64];
   LONG xpos;
   HWND dsktop;
   RECT drc;

  // Let's know where to position it exactly
  if( NULL == ghwndGrid )
  {
    POINT pt;
    PUCE_HEADER pHeader = (PUCE_HEADER)(pUceMemFile->pvData);

    gpwszWindowTitle = (PWSTR)((PSTR)pHeader+pHeader->OffsetTableName);
    if(*gpwszWindowTitle == L'0')
    {
        LoadString(hInst, _wtol(gpwszWindowTitle), TableName, 256);
        gpwszWindowTitle = TableName;
    }

    GetWindowRect( hwndParent , &rc );
    GetWindowGridSize( pUceMemFile , &pt , &gnColOffset , &gnRowOffset );

    pGridUceMemFile = pUceMemFile;

    // Let's create the font that is in the list
    if( !UpdateGridFont( hwndParent , ID_FONT ))
    {
      return NULL;
    }

    LoadString(hInst, IDS_GROUPBY,  buffer, 64);
      
      //fix for group by window too right
      dsktop = GetDesktopWindow();
      GetWindowRect(dsktop, &drc);
      if (rc.right + pt.x > drc.right)
         xpos = rc.left - pt.x;
      else
         xpos = rc.right;

      //rc.left+(rc.right-rc.left),
    hwnd = CreateWindowEx( 0 ,
                           wszGridWndClass,
                           buffer,
                           WS_CAPTION | WS_BORDER | WS_SYSMENU | WS_EX_CLIENTEDGE,
                           xpos,
                           rc.top,
                           pt.x,
                           pt.y,
                           hwndParent,
                           NULL,
                           hInst,
                           NULL
                         );
    if( NULL != hwnd )
    {
      INT x,y;
      SCROLLINFO sinfo;

      sinfo.cbSize = sizeof(SCROLLINFO);
      ghwndGrid = hwnd;

      // let 's create the horiz scroll bar if needed
      if( gnColOffset )
      {
        // x =  GRID_XANCHOR;
        x =  Grid_XAnchor;
        y = (GRID_YANCHOR + (gnMaxRow*GRID_CELL_HEIGHT));
        ghwndHScroll = CreateWindowEx( 0,
                                       L"SCROLLBAR",
                                       L"",
                                       WS_CHILD | WS_VISIBLE | SBS_HORZ | SBS_TOPALIGN,
                                       x,
                                       y,
                                       (gnMaxCol*GRID_CELL_WIDTH),
                                       0,
                                       hwnd,
                                       (HMENU)ID_GRID_HSCROLL,
                                       hInst,
                                       NULL
                                     );

        // Let's adjust the scroll bar metrics
        sinfo.nMin = 0;
        sinfo.nMax = gnColOffset;
        sinfo.nPos = 0;
        sinfo.nPage= gnMaxCol;
        sinfo.fMask = SIF_RANGE|SIF_POS|SIF_PAGE;
        SetScrollInfo( ghwndHScroll , SB_CTL , &sinfo , TRUE );

        gnColOffset=0;
      }

      // let 's create the vert scroll bar if needed
      if( gnRowOffset )
      {
//      x = (GRID_XANCHOR + (gnMaxCol*GRID_CELL_WIDTH));
        x = (Grid_XAnchor + (gnMaxCol*GRID_CELL_WIDTH));
        y =  GRID_YANCHOR;
        ghwndVScroll = CreateWindowEx( 0,
                                       L"SCROLLBAR",
                                       L"",
                                       WS_CHILD | WS_VISIBLE | SBS_VERT | SBS_LEFTALIGN,
                                       x,
                                       y,
                                       0,
                                       (gnMaxRow*GRID_CELL_HEIGHT),
                                       hwnd,
                                       (HMENU)ID_GRID_VSCROLL,
                                       hInst,
                                       NULL
                                     );
        // Let's adjust the scroll bar metrics
        sinfo.nMin = 0;
        sinfo.nMax = gnRowOffset + gnMaxRow - 1;
        sinfo.nPos = 0;
        sinfo.nPage= gnMaxRow;
        sinfo.fMask = SIF_RANGE|SIF_POS|SIF_PAGE;
        SetScrollInfo( ghwndVScroll , SB_CTL , &sinfo , TRUE );

        gnRowOffset=0;
      }

      ShowWindow( hwnd , SW_NORMAL );
    }
  }

  return ghwndGrid;
}


/******************************Public*Routine******************************\
* GetWindowGridSize
*
* Get window size for grid, and if scroll bars are needed
*
* Return Value:
*
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
void GetWindowGridSize( PUCE_MEMORY_FILE pUceMemFile , POINT *pPt , INT *pnXScroll , INT *pnYScroll )
{
  PUCE_HEADER pHeader = (PUCE_HEADER)pUceMemFile->pvData;
  INT    xSize=0;
  INT    ySize=0;
  SIZE   size;
  HDC    hDC;
  HFONT  hFontOld;

  // some init stuff
  gnCurRow     =0;     // default to 1st row
  gnCurCol     =0;     // default to 1st col
  gnRowOffset  =0;
  gnColOffset  =0;
  gnMaxRow     =0;
  gnMaxCol     =0;
  gbInDrag     = FALSE;
  ghwndHScroll = ghwndVScroll = NULL;
  *pnXScroll   = *pnYScroll   = 0;

  // Calculate the basic (both edges)
  xSize += (2*GRID_XANCHOR);
  ySize += GRID_YANCHOR + GRID_BOTTOM_SPACE;

  xSize += (pHeader->Column * GRID_CELL_WIDTH);
  gnMaxCol = pHeader->Column;

  // See if wee need a horz scroll bar
  /*
  if( pHeader->Column > GRID_MAX_COL )
  {
    ySize += GetSystemMetrics( SM_CYHSCROLL );
    *pnXScroll = (pHeader->Column - GRID_MAX_COL);
    xSize += (GRID_CELL_WIDTH*GRID_MAX_COL);
    gnMaxCol = GRID_MAX_COL;
#if DBG
    OutputDebugString(L"\nShould use Horrizonal Scroll Bar");
#endif
  }
  else
  {
    xSize += (pHeader->Column * GRID_CELL_WIDTH);
    gnMaxCol = pHeader->Column;
#if DBG
    OutputDebugString(L"\nNo Horrizonal Scroll Bar Needed!");
#endif
  }
*/

  // See if we need a vert scroll bar
  if( pHeader->Row > GRID_MAX_ROW )
  {
    xSize += GetSystemMetrics( SM_CXVSCROLL );
    *pnYScroll = (pHeader->Row - GRID_MAX_ROW);
    ySize += (GRID_CELL_HEIGHT*GRID_MAX_ROW);
    gnMaxRow = GRID_MAX_ROW;
#if DBG
    OutputDebugString(L"\nShould use Vertical Scroll Bar");
#endif
  }
  else
  {
    ySize += (pHeader->Row * GRID_CELL_HEIGHT);
    gnMaxRow = pHeader->Row;
#if DBG
    OutputDebugString(L"\nNo Vertical Scroll Bar Needed!!");
#endif
  }

  // Update
  xSize += (2*GetSystemMetrics( SM_CXSIZEFRAME ));
  pPt->x = xSize;

  // Compute the size of the text which is the header for this grid
  hDC = GetDC(HWND_DESKTOP);
  hFontOld = (HFONT)SelectObject(hDC, hfStatic);
  GetTextExtentPoint32(hDC, gpwszWindowTitle,
    wcslen(gpwszWindowTitle), &size);
  SelectObject(hDC, hFontOld);
  ReleaseDC(HWND_DESKTOP, hDC);

  size.cx += (2*GRID_XANCHOR) + (2*GetSystemMetrics( SM_CXSIZEFRAME ));

  if( pPt->x < size.cx )            // kchang : for long Window Title
  {
      pPt->x = size.cx;
      Grid_XAnchor = ( pPt->x - gnMaxCol*GRID_CELL_WIDTH ) / 2
                      - GRID_XANCHOR;
  }
  else
  {
      Grid_XAnchor = GRID_XANCHOR;
  }

  // some add-ons
  ySize += (2*GetSystemMetrics( SM_CYSIZEFRAME ));
  ySize += GetSystemMetrics(SM_CYCAPTION);
  pPt->y = ySize;

  // Update our grid window control rect
  // grcGridWindow.left   = GRID_XANCHOR;
  grcGridWindow.left   = Grid_XAnchor;
  grcGridWindow.top    = GRID_YANCHOR;
  grcGridWindow.right  = (grcGridWindow.left + (gnMaxCol*GRID_CELL_WIDTH));
  grcGridWindow.bottom = (grcGridWindow.top  + (gnMaxRow*GRID_CELL_HEIGHT));

  return;
}


/******************************Public*Routine******************************\
* GridWndProc
*
* Grid Callback Window Proc
*
* Return Value:
*
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
LRESULT CALLBACK GridWndProc( HWND hWnd , UINT uMsg , WPARAM wParam , LPARAM lParam )
{
  switch( uMsg )
  {
  case WM_CREATE:
    {
    }
    break;

  case WM_CHAR:
    {
       INT   nCurRow = gnCurRow;
       INT   nCurCol = gnCurCol;
       HDC   hDC     = GetDC( hWnd );
       WCHAR wchar   = (WCHAR) wParam;
       RECT  rc;
       WCHAR wc;
       INT   nTmpRow;
       INT   nTmpCol;

       gnCurRow = gnMaxRow-1;
       gnCurCol = gnMaxCol-1;
       while(1)
       {
          if(IsCellEnabled(gnCurRow, gnCurCol))
          {
              GetCurrentGroupChar( &wc );

              if(CompareString(0,
                     NORM_IGNORECASE     | NORM_IGNOREKANATYPE |
                     NORM_IGNORENONSPACE | NORM_IGNOREWIDTH,
                     &wc, 1,
                     &wchar, 1) == 2) 
                  break; 
          }

          if(--gnCurCol < 0)
          {
              if(--gnCurRow < 0)
              {
                  gnCurRow = nCurRow;
                  gnCurCol = nCurCol;
                  ReleaseDC( hWnd , hDC );
                  return 0L;
              }
              gnCurCol = gnMaxCol-1;
          }
       }

       nTmpRow = gnCurRow;
       nTmpCol = gnCurCol;

       gnCurRow = nCurRow;
       gnCurCol = nCurCol;
       GetCurrentRect( &rc );
       GetCurrentGroupChar( &wc );
       DrawGridCell( hDC , &rc , FALSE , wc , TRUE) ;

       gnCurRow = nTmpRow;
       gnCurCol = nTmpCol;
       GetCurrentRect( &rc );
       GetCurrentGroupChar( &wc );
       DrawGridCell( hDC , &rc , TRUE , wc , TRUE);

       ReleaseDC( hWnd , hDC );
       SubSetChanged( hwndDialog );
    }
    break;

  case WM_KEYDOWN:
    {
    BOOL bForward;
    BOOL bThisLine;
    INT  nCurRow = gnCurRow;
    INT  nCurCol = gnCurCol;

    PUCE_HEADER pHeader = (PUCE_HEADER)pGridUceMemFile->pvData;

    // If hWnd is mirrored swap left and right keys.
    if (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) {
        if (wParam == VK_LEFT) {
            wParam = VK_RIGHT;
        } else if (wParam == VK_RIGHT) {
            wParam = VK_LEFT;
        }
    }

    switch( LOWORD(wParam) )
       {
       case VK_ESCAPE:
            SendMessage(ghwndGrid, WM_CLOSE, 0, 0L);
            return 0L;

       case VK_HOME:
            if(GetAsyncKeyState(VK_CONTROL))
            {
               if(ghwndVScroll)  GridVScroll(hWnd , uMsg , SB_TOP, 0);
                  nCurRow = 0;
            }
             nCurCol = 0;
            bForward = false;
            bThisLine = true;
            break;

       case VK_END:
            if(GetAsyncKeyState(VK_CONTROL))
            {
               if(ghwndVScroll)  GridVScroll(hWnd, uMsg, SB_BOTTOM, 0);
               nCurRow = pHeader->Row - 1;
            }
              nCurCol = gnMaxCol - 1;
            bForward = true;
            bThisLine = true;
            break;

vk_left:
       case VK_LEFT:
            if(nCurCol == 0)
            {
                 if (nCurRow > 0)
              {
                    nCurRow--;
                nCurCol = gnMaxCol-1;
                   if (ghwndVScroll) GridVScroll(hWnd , uMsg , SB_LINEUP, 0);
              }
            }
               else
                 nCurCol--;
            bForward = false;
            bThisLine = false;
          break;

       case VK_TAB:
            if(GetAsyncKeyState(VK_SHIFT)) goto vk_left;
       case VK_RIGHT:
               if (nCurCol == gnMaxCol - 1)
            {
                 if (nCurRow < pHeader->Row -1)
              {
                    nCurRow++;
                   nCurCol = 0;
                   if (ghwndVScroll) GridVScroll(hWnd , uMsg , SB_LINEDOWN, 0);
              }
            }
            else
            {
                 nCurCol++;
            }
            bForward = true;
            bThisLine = false;
            break;

       case VK_PRIOR:
            if(ghwndVScroll != NULL)
            {
               if(nCurRow < gnMaxRow)
                   nCurRow = 0;
               else
                   nCurRow = nCurRow - gnMaxRow;
               GridVScroll(hWnd , uMsg , SB_PAGEUP, 0);
            }
            else
            {
               nCurRow = 0;
            }
          bForward = false;
            bThisLine = true;
            break;

       case VK_NEXT:
            if(ghwndVScroll != NULL)
            {
               if(nCurRow >= (pHeader->Row - gnMaxRow))
                   nCurRow = pHeader->Row - 1;
               else
                   nCurRow += gnMaxRow;
               GridVScroll(hWnd , uMsg , SB_PAGEDOWN, 0);
            }
            else
            {
               nCurRow = gnMaxRow-1;
            }
         bForward = true;
            bThisLine = true;
            break;

       case VK_UP:
            if(ghwndVScroll != NULL)
            {
                GridVScroll(hWnd , uMsg , SB_LINEUP, 0);
            }
            if (nCurRow > 0) nCurRow--;
         bForward = false;
            bThisLine = true;
            break;

       case VK_DOWN:
            if(ghwndVScroll != NULL)
            {
                GridVScroll(hWnd , uMsg , SB_LINEDOWN, 0);
            }
         if (nCurRow < pHeader->Row - 1) nCurRow++;
         bForward = true;
            bThisLine = true;
            break;

       case VK_F6:
            SetFocus(hwndCharGrid);
            return 0L;
            break;

       default:
            return 0L;
       }

       if (!bForward)
       {
         while(!IsCellEnabled(nCurRow, nCurCol))
           if (nCurCol > 0)
               nCurCol--;
           else
             if (bThisLine)
             {
               ;
             }
             else
             {
                if (nCurRow > 0)
                {
                  nCurRow--;
                  nCurCol = gnMaxCol - 1;
                  if (ghwndVScroll != NULL) GridVScroll(hWnd , uMsg , SB_LINEUP, 0);
                }
             }
       }
       else
       {
         while(!IsCellEnabled(nCurRow, nCurCol))
         {
           if (nCurCol < gnMaxCol - 1)
             nCurCol++;
           else
             if (bThisLine) //went too far , has to come back
             {
               while(!IsCellEnabled(nCurRow, nCurCol))
                 nCurCol --;
             }
             else
             {
               if (nCurRow < pHeader->Row - 1)
               {
                 nCurRow++;
                 nCurCol = 0;
                 if (ghwndVScroll != NULL) GridVScroll(hWnd , uMsg , SB_LINEDOWN, 0);
               }
               else // went too far, has to come back
               {
                 while(!IsCellEnabled(nCurRow, nCurCol))
                   nCurCol --;
               }
             }
         }
       }

       {
          RECT  rc;
          WCHAR wc;
          HDC   hDC = GetDC( hWnd );

        //Erase focus frame of old cell 
          GetCurrentRect( &rc );
          GetCurrentGroupChar( &wc );
          DrawGridCell( hDC , &rc , FALSE , wc , TRUE) ;

        //draw focus frame of new cell
          gnCurRow = nCurRow;
          gnCurCol = nCurCol;
          GetCurrentRect( &rc );
          GetCurrentGroupChar( &wc );
          DrawGridCell( hDC , &rc , TRUE , wc , TRUE);

          ReleaseDC( hWnd , hDC );
        InvalidateRect( hWnd , NULL , TRUE );
          SubSetChanged( hwndDialog );
       }

    }
    break;

  case WM_COMMAND:
    {
        return (DefWindowProc(hWnd, uMsg, wParam, lParam));
    }
  break;

  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC         hDC;
      HFONT       hOldFont;

      hDC = BeginPaint( hWnd , &ps );

      // Actual stuff
      DoPaint( hWnd , hDC );

      hOldFont = (HFONT)SelectObject(hDC, hfStatic);
      SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));
      TextOut(hDC, TITLE_X, TITLE_Y, gpwszWindowTitle,
        wcslen(gpwszWindowTitle));
      SelectObject(hDC, hOldFont);

      EndPaint( hWnd , &ps );
    }
    break;

  // scroll bar
  case WM_HSCROLL:
      GridHScroll( hWnd , uMsg , wParam , lParam );
    break;

  case WM_VSCROLL:
      GridVScroll( hWnd , uMsg , wParam , lParam );
    break;


  // Mouse events
  case WM_LBUTTONDOWN:
    {
      // Check if in our area, and if so start tracking
      POINT pt;
      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);

      if( PtInRect( &grcGridWindow , pt ) && IsCellEnabled(  gnCurRow , gnCurCol ) )
      {
        RECT  rc;
        HDC   hDC;
        WCHAR wc;
        BOOL  bRet;
        INT   nCurRow,nCurCol;


        // Set capture & flag (need to check if this is the same cell or not
        SetCapture( hWnd );
        gbInDrag = TRUE;

        // Clear previous active
        bRet = GetCurrentRect( &rc );
        nCurRow = (((pt.y-GRID_YANCHOR)/GRID_CELL_HEIGHT)+gnRowOffset);
 //     nCurCol = (((pt.x-GRID_XANCHOR)/GRID_CELL_WIDTH)+gnColOffset);
        nCurCol = (((pt.x-Grid_XAnchor)/GRID_CELL_WIDTH)+gnColOffset);

        if( bRet && !IsCellEnabled(  nCurRow , nCurCol ) )
        {
          break;
        }

        hDC = GetDC( hWnd );
        if( bRet )
        {
          GetCurrentGroupChar( &wc );
          DrawGridCell( hDC , &rc , FALSE , wc , TRUE) ;
        }

        // Indicate active cell
        gnCurRow = nCurRow;
        gnCurCol = nCurCol;
        if( GetCurrentRect( &rc ) )
        {
          GetCurrentGroupChar( &wc );
#if DBG
          {
            WCHAR wsz[64];
            wsprintf(&wsz[0],L"CodePoint=%x\n",wc);
            OutputDebugString( wsz );
          }
#endif
          DrawGridCell( hDC , &rc , TRUE , wc , TRUE) ;
        }

        ReleaseDC( hWnd , hDC );

        // This could be expensive for a large grid
        SubSetChanged( hwndDialog );
      }
    }
    break;

  case WM_MOUSEMOVE:
    {
      POINT pt;
      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);
      // Move the rect as we drag in
      // get new active cell, if same then don't do anything
      // otherwise clear old and draw new one
      if( gbInDrag )
      {
        if( PtInRect( &grcGridWindow , pt ) )
        {
          WCHAR wc;
          RECT  rc;
          HDC   hDC;
          INT   nCurRow,nCurCol;
          BOOL  bRet=FALSE;

          // Some optimization so we don't draw all the time
          if( GridSamePointHit( pt ) )
            break;

          // Clear previous active
          nCurRow = (((pt.y-GRID_YANCHOR)/GRID_CELL_HEIGHT)+gnRowOffset);
//        nCurCol = (((pt.x-GRID_XANCHOR)/GRID_CELL_WIDTH)+gnColOffset);
          nCurCol = (((pt.x-Grid_XAnchor)/GRID_CELL_WIDTH)+gnColOffset);

          bRet = GetCurrentRect( &rc ) ;

          if( bRet && !IsCellEnabled(  nCurRow , nCurCol ) )
            break;

          hDC = GetDC( hWnd );
          if( bRet )
          {
            GetCurrentGroupChar( &wc );
            DrawGridCell( hDC , &rc , FALSE , wc , TRUE) ;
          }

          // Indicate active cell
          gnCurRow = nCurRow;
          gnCurCol = nCurCol;
          if( GetCurrentRect( &rc ) )
          {
            GetCurrentGroupChar( &wc );
            DrawGridCell( hDC , &rc , TRUE , wc , TRUE) ;
          }

          ReleaseDC( hWnd , hDC );

          // This could be expensive for a large grid
          SubSetChanged( hwndDialog );
        }
      }
    }
    break;

  case WM_LBUTTONUP:
    {
      // Check if we are in drag mode
      if( gbInDrag )
      {
        ReleaseCapture();
        gbInDrag = FALSE;

        // no need to update current selection since I hook
        // it on WM_MOUSEMOVE
      }
    }
    break;

  case WM_CLOSE:
    {
      WCHAR buffer[64];

      SendMessage( GetDlgItem(hwndDialog, ID_UNICODESUBSET),
                   CB_SETCURSEL,
                   (WPARAM) 0,
                   (LPARAM) 0);
      LoadString(hInst, IDS_RESET, buffer, 64);
      SetDlgItemText(hwndDialog, ID_SEARCH, buffer);
      EnableWindow(GetDlgItem(hwndDialog, ID_SEARCH), TRUE);
      SetSearched();
      SetFocus(GetDlgItem(hwndDialog, ID_SEARCH));

      DestroyWindow(hWnd);
    }
    break;

  case WM_DESTROY:
    {
      EnableWindow(GetDlgItem(hwndDialog, ID_VIEW), TRUE);
      EnableSURControls(hwndDialog);
      ghwndGrid = NULL;
      // fall through
    }

  default:
    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
  }

  return 0L;
}


/******************************Public*Routine******************************\
* GridHScroll
*
* Grid Callback handler for WM_HSCROLL
*
* Return Value:
*
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
BOOL GridHScroll( HWND hWnd , UINT uMsg , WPARAM wParam , LPARAM lParam )
{
  SCROLLINFO sinfo;

  sinfo.cbSize = sizeof( SCROLLINFO );
  sinfo.fMask = SIF_PAGE|SIF_POS|SIF_RANGE;
  GetScrollInfo( ghwndHScroll , SB_CTL , &sinfo );

  switch( LOWORD( wParam ) )
  {
  case SB_TOP:
    gnColOffset=0;
    break;

  case SB_BOTTOM:
    gnColOffset=sinfo.nMax;
    break;

  case SB_LINEUP:
    if( gnColOffset )
      gnColOffset--;
    break;

  case SB_LINEDOWN:
    if( gnColOffset < sinfo.nMax )
      gnColOffset++;
    break;

  case SB_PAGEUP:
    if( gnColOffset >= PAGE_SCROLL_SIZE )
      gnColOffset -= PAGE_SCROLL_SIZE;
    else
      gnColOffset = 0;
    break;

  case SB_PAGEDOWN:
    if( (gnColOffset + PAGE_SCROLL_SIZE) <= (INT)sinfo.nMax )
      gnColOffset += PAGE_SCROLL_SIZE;
    else
      gnColOffset = sinfo.nMax;
    break;

  default:
    return FALSE;
  }

  // update position
  sinfo.nPos = gnColOffset;
  sinfo.fMask = SIF_POS;
  SetScrollInfo( ghwndHScroll , SB_CTL , &sinfo , TRUE );

  // Update the window (silent)
  InvalidateRect( hWnd , NULL , FALSE );

  return TRUE;
}


/******************************Public*Routine******************************\
* GridVScroll
*
* Grid Callback handler for WM_VSCROLL
*
* Return Value:
*
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
BOOL GridVScroll( HWND hWnd , UINT uMsg , WPARAM wParam , LPARAM lParam )
{
  SCROLLINFO sinfo;

  sinfo.cbSize = sizeof( SCROLLINFO );
  sinfo.fMask = SIF_PAGE|SIF_POS|SIF_RANGE;
  GetScrollInfo( ghwndVScroll , SB_CTL , &sinfo );

  switch( LOWORD( wParam ) )
  {
  case SB_TOP:
     gnRowOffset = 0;
    break;

  case SB_BOTTOM:
     gnRowOffset = sinfo.nMax - gnMaxRow + 1;
    break;

  case SB_LINEUP:
    if(gnRowOffset)
      gnRowOffset--;
    break;

  case SB_LINEDOWN:
    if( gnRowOffset < sinfo.nMax - gnMaxRow + 1)
      gnRowOffset++;
    break;

  case SB_PAGEUP:
    if( gnRowOffset > gnMaxRow )
      gnRowOffset -= gnMaxRow;
    else
      gnRowOffset = 0;
    break;

  case SB_PAGEDOWN:
    if( gnRowOffset < sinfo.nMax - gnMaxRow + 1 - gnMaxRow)
      gnRowOffset += gnMaxRow;
    else
      gnRowOffset= sinfo.nMax - gnMaxRow + 1;
    break;

  case ( SB_THUMBTRACK ) :
  case ( SB_THUMBPOSITION ) :
  {
    gnRowOffset = (HIWORD(wParam));
    break;
  }

  default:
    return FALSE;
  }

  // update position
  sinfo.nPos = gnRowOffset;
  sinfo.fMask = SIF_POS;
  SetScrollInfo( ghwndVScroll , SB_CTL , &sinfo , TRUE );

  // Update the window (silent)
  InvalidateRect( hWnd , NULL , FALSE );

  return TRUE;
}


/******************************Public*Routine******************************\
* DestroyGridWindow
*
* Destroys the grid window
*
* Return Value:
*
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
void DestroyGridWindow( void )
{
  if( ghwndGrid )
  {
    DestroyWindow( ghwndGrid );
  }

  return;
}


/******************************Public*Routine******************************\
* DoPaint
*
* Grid's WM_PAINT handler
*
* Return Value:
*
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
void DoPaint( HWND hWnd , HDC hDC )
{
  RECT        rc;
  INT         i,j;
  PUCE_HEADER pHeader = (PUCE_HEADER)pGridUceMemFile->pvData;
  PUCE_GROUP  pGroup = (PUCE_GROUP)((PSTR)pHeader+sizeof(UCE_HEADER)),pRefGroup;
  PWSTR       pwCh;

  // Let's draw the grid first
  rc.top    = GRID_YANCHOR;
  rc.bottom = rc.top+GRID_CELL_HEIGHT;

  // set a reference group
  pRefGroup = pGroup;

  for( i=0 ; i<gnMaxRow ; i++ )
  {
//  rc.left   = GRID_XANCHOR;
    rc.left   = Grid_XAnchor;
    rc.right  = rc.left+GRID_CELL_WIDTH;

    for( j=0 ; j<gnMaxCol ; j++ )
    {
      // draw all cells
      pGroup  = pRefGroup + (((gnRowOffset+i)*pHeader->Column)+gnColOffset+j);
      pwCh    = (PWSTR)((PSTR)pGridUceMemFile->pvData+pGroup->OffsetGroupName) ;

      DrawGridCell( hDC ,
                    &rc ,
                    (((j+gnColOffset)==gnCurCol) && ((i+gnRowOffset)==gnCurRow) && (gnCurRow!=-1)) ,
                    *pwCh ,
                    IsCellEnabled((i+gnRowOffset) , (j+gnColOffset))
                  );
      rc.left  = rc.right;
      rc.right+= GRID_CELL_WIDTH;
    }

    // next row
//  rc.left   = GRID_XANCHOR;
    rc.left   = Grid_XAnchor;
    rc.right  = rc.left+GRID_CELL_WIDTH;
    rc.top    = rc.bottom;
    rc.bottom += GRID_CELL_HEIGHT;
  }

  return;
}



/******************************Public*Routine******************************\
* MyFrameRect
*
* Draw a dashed rect (replace FrameRect)
*
* Return Value:
*
* History:
*   Sept-04-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
void MyFrameRect( HDC hDC , RECT *prc )
{
  HPEN hOldPen = (HPEN)SelectObject( hDC , hpenDash );

  MoveToEx( hDC , prc->left  , prc->top , NULL ) ;
  LineTo(   hDC , prc->right , prc->top ) ;
  LineTo(   hDC , prc->right , prc->bottom );
  LineTo(   hDC , prc->left  , prc->bottom );
  LineTo(   hDC , prc->left  , prc->top );

  SelectObject( hDC , hOldPen );

  return;
}

/******************************Public*Routine******************************\
* DrawGridCell
*
* Draws a grid cell on the current hDC and *prc rect, and fills with
* the 'wc' text
*
* Return Value:
*
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
void DrawGridCell( HDC hDC , RECT *prc , BOOL bActive , WCHAR wc , BOOL bEnabled )
{
  HFONT    hOldFont;
  COLORREF oldTextColor,oldBkColor;
  INT      iWidth;
  INT      xAnchor=prc->left;


  // text
  if( bEnabled )
  {
    oldTextColor = SetTextColor( hDC , RGB(0,0,0) );
    oldBkColor   = SetBkColor( hDC , RGB(255,255,255) );
  }
  else
  {
    oldTextColor = SetTextColor( hDC , RGB(128,128,128) );
//    oldTextColor = SetTextColor( hDC , RGB(128,128,128) );
//    oldBkColor   = SetBkColor( hDC , RGB(192,192,192) );
  }

  hOldFont = (HFONT)SelectObject( hDC , hFont );

  // Check is disabled, put space
  if( (WCHAR)0 == wc )
    wc = (WCHAR)0x20;

  GetCharWidth32( hDC , wc , wc , &iWidth );
  if( (prc->right-prc->left) > (iWidth) )
    xAnchor += (((prc->right-prc->left)-(iWidth))/2);

  ExtTextOut( hDC ,
              xAnchor,
              prc->top+4,
              ETO_OPAQUE|ETO_CLIPPED,
              prc,
              &wc,
              1,
              NULL
            );


  // restore everything
  SelectObject( hDC , hOldFont );
  SetTextColor( hDC , oldTextColor );
  if( bEnabled )
    SetBkColor( hDC , oldBkColor );


  // rect everything else
  if( !bActive )
  {
    MyFrameRect( hDC , prc );//, GetStockObject( BLACK_BRUSH ) );
  }
  else
  {
    // Draw an inside focus if needed
    RECT rc;

    DrawFocusRect( hDC , prc );
    rc.left = prc->left+1;
    rc.right = prc->right-1;
    rc.top = prc->top+1;
    rc.bottom = prc->bottom-1;
    DrawFocusRect( hDC , &rc );
    rc.left += 1;
    rc.right -= 1;
    rc.top += 1;
    rc.bottom -= 1;
    DrawFocusRect( hDC , &rc );
  }

  return ;
}


/******************************Public*Routine******************************\
* GetCurrentRect
*
* Retreives the current active rect if visible
*
* Return Value:
*  TRUE if possible, otherwise FALSE
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
__inline BOOL GetCurrentRect( RECT *prc )
{
  if( (-1 == gnCurRow) ||
      (gnCurRow < gnRowOffset) || (gnCurRow >= (gnRowOffset+gnMaxRow)) ||
      (gnCurCol < gnColOffset) || (gnCurCol >= (gnColOffset+gnMaxCol))
    )
  {
    return FALSE;
  }

//prc->left = GRID_XANCHOR+((gnCurCol-gnColOffset)*GRID_CELL_WIDTH);
  prc->left = Grid_XAnchor+((gnCurCol-gnColOffset)*GRID_CELL_WIDTH);
  prc->right = prc->left+GRID_CELL_WIDTH;
  prc->top = GRID_YANCHOR+((gnCurRow-gnRowOffset)*GRID_CELL_HEIGHT);
  prc->bottom = prc->top+GRID_CELL_HEIGHT;

  return TRUE;
}


/******************************Public*Routine******************************\
* GetCurrentGroupChar
*
* Retreives the current char (codepoint) of the currently selected group
*
* Return Value:
*  TRUE if possible, otherwise FALSE
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
__inline void GetCurrentGroupChar( PWSTR pwCh )
{
  PUCE_HEADER pHeader = (PUCE_HEADER)pGridUceMemFile->pvData;
  PUCE_GROUP  pGroup = (PUCE_GROUP)((PSTR)pHeader+sizeof(UCE_HEADER));

  pGroup += (((gnCurRow)*pHeader->Column)+gnCurCol);
  *pwCh   = *(PWSTR)((PSTR)pGridUceMemFile->pvData+pGroup->OffsetGroupName) ;

  return;
}

/******************************Public*Routine******************************\
* IsCellEnabled
*
* Retreives the current char (codepoint) of the currently selected group
*
* Return Value:
*  TRUE if possible, otherwise FALSE
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/

__inline BOOL IsCellEnabled( INT nRow , INT nCol )
{
  DWORD       dwNum=0;
  DWORD       dwChOffset=0;
  PUCE_HEADER pHeader = (PUCE_HEADER)pGridUceMemFile->pvData;
  PUCE_GROUP  pGroup = (PUCE_GROUP)((PSTR)pHeader+sizeof(UCE_HEADER));

  if(nRow >= pHeader->Row || nCol >= pHeader->Column) return FALSE;

  pGroup += (((nRow)*pHeader->Column)+nCol);
  dwChOffset   = pGroup->OffsetGroupName ;
  dwNum        = pGroup->NumChar ;

  return ((dwNum>0) && (dwChOffset!=0));
}

/******************************Public*Routine******************************\
* GridSamePointHit
*
* Checks if the point is within the lastly drawn rect. This is a good optimization
*
* Return Value:
*  TRUE if in same rect, otherwise FALSE
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
__inline BOOL GridSamePointHit( POINT pt )
{
  INT iNewRow = (((pt.y-GRID_YANCHOR)/GRID_CELL_HEIGHT)+gnRowOffset);
//INT iNewCol = (((pt.x-GRID_XANCHOR)/GRID_CELL_WIDTH)+gnColOffset);
  INT iNewCol = (((pt.x-Grid_XAnchor)/GRID_CELL_WIDTH)+gnColOffset);

  if( (-1 == gnCurRow) ||
      ((iNewRow == gnCurRow) && (iNewCol == gnCurCol))
    )
  {
    return TRUE;
  }

  return FALSE;
}


/******************************Public*Routine******************************\
* GetUnicodeCharsFromGridWindow
*
* Reads the unicode content of the current selected group of the active
* UCE_MEMORY_FILE and fills in a buffer
*
* Return Value:
*  TRUE if successful, otherwise FALSE
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
BOOL GetUnicodeCharsFromGridWindow( HWND hWnd , PWSTR pwcBuf , UINT *puNum , BOOL *pbLineBreak)
{
  PUCE_HEADER pHeader = (PUCE_HEADER)pGridUceMemFile->pvData;
  PUCE_GROUP  pGroup = (PUCE_GROUP)((PSTR)pHeader+sizeof(UCE_HEADER));
  PWSTR       pwc;

  if( -1 == gnCurRow )   // if invalid sel
    return FALSE;


  pGroup += (((gnCurRow)*pHeader->Column)+gnCurCol);
  pwc     = (PWSTR)((PSTR)pHeader+pGroup->OffsetGroupChar);
  *puNum = pGroup->NumChar;

  GetWChars( pGroup->NumChar , pwc , pwcBuf , puNum , pbLineBreak );

  return TRUE;
}


/******************************Public*Routine******************************\
* GetWChars
*
* Unified place to read wc from/tp
*
* Return Value:
*
* History:
*   Sept-18-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
void GetWChars( INT nChars, WCHAR *pwc, WCHAR *pwcBuf, UINT *puNum, BOOL *pbLineBreak)
{
  DWORD dwI=0;
  DWORD dwCurCount=0;

  *pbLineBreak=FALSE;

  while( (INT)dwI<nChars )
  {
    // We need to fill the rest of row with spaces when we hit
    // a 0x0a. This is  to hack the current implementation
    // of the main grid
    if( (WCHAR)0x0a == *pwc )
    {
      DWORD dwLeft = ((dwCurCount)%20);
      dwLeft = 20L - dwLeft;
      *puNum += dwLeft;
      dwCurCount += dwLeft;
      while( dwLeft )
      {
        *pwcBuf = (WCHAR)' ';
        pwcBuf++;
        dwLeft--;
      }

      pwc++;
      *puNum -= 1;    // the 0x0a
      dwI++;
      *pbLineBreak=TRUE;
      continue;
    }

    // let's fill real stuff
    *pwcBuf = *pwc;

    // We need to fill the rest of row with spaces till we hit
    pwcBuf++;
    pwc++;
    dwI++;

    dwCurCount++;
  }
}

/******************************Public*Routine******************************\
* IsGridWindowAlive
*
* Checks if the grid window is created
*
* Return Value:
*  TRUE if so, otherwise FALSE
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
BOOL IsGridWindowAlive( void )
{
  return (ghwndGrid != NULL) ;
}


/******************************Public*Routine******************************\
* IsAnyListWindow
*
* Checks if any grid/list window is created
*
* Return Value:
*  TRUE if so, otherwise FALSE
* History:
*   Sept-16-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
BOOL IsAnyListWindow( void )
{
  return ((ghwndGrid != NULL) || (ghwndList != NULL));
}

/******************************Public*Routine******************************\
* CreateResources
*
* Creates any global resources the would be needed during the app lifetime
*
* Return Value:
*
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
void CreateResources( HINSTANCE hInst, HWND hWnd )
{
  HDC hDC;

  InitListWindow( hInst );
  InitGridWindow( hInst );

  hpenDash = CreatePen( PS_SOLID , 0 , RGB(128,128,128));

  hDC = GetDC(hWnd);

//  hfStatic = CreateFont( -MulDiv(DIALOG_FONT_SIZE,
//                           GetDeviceCaps(hDC, LOGPIXELSY), 72),
  hfStatic = CreateFont( GRID_CELL_HEIGHT-7,
                         0,
                         0,0,0,
                         0,0,0,0,
                         OUT_DEFAULT_PRECIS,
                         CLIP_DEFAULT_PRECIS,
                         DEFAULT_QUALITY,
                         DEFAULT_PITCH|FF_DONTCARE,
                         DIALOG_FONT_NAME
                       );

  ReleaseDC(hWnd, hDC);
}


/******************************Public*Routine******************************\
* DeleteResources
*
* Deletes any global resources that had been created through CreateResources
*
* Return Value:
*
* History:
*   Sept-03-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
void DeleteResources( void )
{
  if( hFont )
    DeleteObject( hFont );

  if( hfList )
    DeleteObject( hfList );

  if( hpenDash )
    DeleteObject( hpenDash );

  if( hfStatic )
    DeleteObject( hfStatic );

  UnregisterClass( wszListWndClass , hInst );
  UnregisterClass( wszGridWndClass , hInst );
}


/******************************Public*Routine******************************\
* CreateNewGridFont
*
* Creates a new font for the grid when the selection changes
*
* Return Value:
*
* History:
*   Sept-15-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
/*
BOOL CreateNewGridFont( HWND hWnd , UINT uID )
{
  if( ghwndGrid )
  {
    UpdateGridFont( hWnd , uID );
    InvalidateRect( ghwndGrid , NULL , FALSE );
  }

  return TRUE;
}
*/


/******************************Public*Routine******************************\
* UpdateGridFont
*
* Creates a new hFont based on the current font selection
*
* Return Value:
*
* History:
*   Sept-15-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
BOOL UpdateGridFont( HWND hwndParent , UINT uID )
{
/*
  INT  nIndex;
  INT  iCharset=0;
  BOOL bRet=FALSE;

  // Let's create the font that is in the list
  nIndex = SendDlgItemMessage( hwndParent,
                               uID,
                               CB_GETCURSEL,
                               (WPARAM) 0,
                               (LPARAM) 0L
                             );
  if( CB_ERR != nIndex )
  {
    WCHAR wszFontName[64];
    SendDlgItemMessage( hwndParent,
                        uID,
                        CB_GETLBTEXT,
                        (WPARAM) nIndex,
                        (LPARAM) (LPWSTR) &wszFontName[0]
                      );

    // if we have already created one, let's delete it
    if( hFont )
      DeleteObject( hFont );

    // Let's try grab a correct charset, if possible
    iCharset = Font_GetSelFontCharSet( hwndParent , ID_FONT , nIndex );

    hFont = CreateFont( (GRID_CELL_HEIGHT-6) ,
                        0,
                        0,0,0,
                        0,0,0,iCharset,
                        OUT_DEFAULT_PRECIS,
                        CLIP_DEFAULT_PRECIS,
                        PROOF_QUALITY,
                        VARIABLE_PITCH|FF_MODERN,
                        &wszFontName[0]
                       );

    bRet = TRUE;
  }

  return bRet;
  */
  if( hFont )
    DeleteObject( hFont );

  hFont = CreateFont( (GRID_CELL_HEIGHT-6) ,
                      0,
                      0,0,0,
                      0,0,0,DEFAULT_CHARSET,
                      OUT_DEFAULT_PRECIS,
                      CLIP_DEFAULT_PRECIS,
                      PROOF_QUALITY,
                      VARIABLE_PITCH|FF_MODERN,
                      TEXT("MS Shell Dlg")
                     );

  return TRUE;
}

/******************************Public*Routine******************************\
* CreateNewListFont
*
* Creates a new font for the list when the selection changes
*
* Return Value:
*
* History:
*   Sept-15-1997  Samer Arafeh  [samera]
*    wrote it
\**************************************************************************/
/*
BOOL CreateNewListFont( HWND hWnd , UINT uID )
{
  if( ghwndGrid )
  {
    UpdateListFont( hWnd , uID );
    InvalidateRect( ghwndList , NULL , FALSE );
  }

  return TRUE;
}
*/


/******************************Public*Routine******************************\
* UpdateListFont
*
* Creates a new hFont based on the current font selection
*
* Return Value:
*
* History:
\**************************************************************************/
BOOL UpdateListFont( HWND hwndParent , UINT uID )
{
/*
  INT  nIndex;
  INT  iCharset=0;
  BOOL bRet=FALSE;

  // Let's create the font that is in the list
  nIndex = SendDlgItemMessage( hwndParent,
                               uID,
                               CB_GETCURSEL,
                               (WPARAM) 0,
                               (LPARAM) 0L
                             );
  if( CB_ERR != nIndex )
  {
    // if we have already created one, let's delete it
    if( hfList )
      DeleteObject( hfList );

    // Let's try grab a correct charset, if possible
    iCharset = Font_GetSelFontCharSet( hwndParent , ID_FONT , nIndex );

    hfList = CreateFont( GRID_CELL_HEIGHT-7,            // 12,
                        0,
                        0,0,0,
                        0,0,0,iCharset,
                        OUT_DEFAULT_PRECIS,
                        CLIP_DEFAULT_PRECIS,
                        PROOF_QUALITY,
                        VARIABLE_PITCH|FF_MODERN,
                        DIALOG_FONT_NAME
                       );

    bRet = TRUE;
  }

  return bRet;
*/
    int nHight = GRID_CELL_HEIGHT-7;

    if( hfList )
      DeleteObject( hfList );

    // If it is a BiDi localized use a one pixel biger font.
    if (GetWindowLongPtr(hwndParent, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) {
        nHight = GRID_CELL_HEIGHT-6;
    }

    hfList = CreateFont( nHight,            // 12,
                        0,
                        0,0,0,
                        0,0,0,DEFAULT_CHARSET,
                        OUT_DEFAULT_PRECIS,
                        CLIP_DEFAULT_PRECIS,
                        PROOF_QUALITY,
                        VARIABLE_PITCH|FF_MODERN,
                        DIALOG_FONT_NAME
                       );

    return TRUE;
}
