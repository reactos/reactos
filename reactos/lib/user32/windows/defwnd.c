/* $Id: defwnd.c,v 1.54 2003/07/05 17:57:22 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/window.c
 * PURPOSE:         Window management
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <window.h>
#include <user32/wininternal.h>
#include <string.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static HBITMAP hbSysMenu;
/* TODO:  widgets will be cached here.
static HBITMAP hbClose;
static HBITMAP hbCloseD;
static HBITMAP hbMinimize;
static HBITMAP hbMinimizeD;
static HBITMAP hbRestore;
static HBITMAP hbRestoreD;
static HBITMAP hbMaximize;
static HBITMAP hbScrUp;
static HBITMAP hbScrDwn;
static HBITMAP hbScrLeft;
static HBITMAP hbScrRight;
*/
static COLORREF SysColours[] =
  {
    RGB(224, 224, 224) /* COLOR_SCROLLBAR */,
    RGB(58, 110, 165) /* COLOR_BACKGROUND */,
    RGB(0, 0, 128) /* COLOR_ACTIVECAPTION */,
    RGB(128, 128, 128) /* COLOR_INACTIVECAPTION */,
    RGB(192, 192, 192) /* COLOR_MENU */,
    RGB(192, 192, 192) /* COLOR_WINDOW */,
    RGB(192, 192, 192) /* COLOR_WINDOWFRAME */,
    RGB(0, 0, 0) /* COLOR_MENUTEXT */,
    RGB(0, 0, 0) /* COLOR_WINDOWTEXT */,
    RGB(255, 255, 255) /* COLOR_CAPTIONTEXT */,
    RGB(128, 128, 128) /* COLOR_ACTIVEBORDER */,
    RGB(255, 255, 255) /* COLOR_INACTIVEBORDER */,
    RGB(255, 255, 232) /* COLOR_APPWORKSPACE */,
    RGB(224, 224, 224) /* COLOR_HILIGHT */,
    RGB(0, 0, 128) /* COLOR_HILIGHTTEXT */,
    RGB(192, 192, 192) /* COLOR_BTNFACE */,
    RGB(128, 128, 128) /* COLOR_BTNSHADOW */,
    RGB(192, 192, 192) /* COLOR_GRAYTEXT */,
    RGB(0, 0, 0) /* COLOR_BTNTEXT */,
    RGB(192, 192, 192) /* COLOR_INACTIVECAPTIONTEXT */,
    RGB(255, 255, 255) /* COLOR_BTNHILIGHT */,
    RGB(32, 32, 32) /* COLOR_3DDKSHADOW */,
    RGB(192, 192, 192) /* COLOR_3DLIGHT */,
    RGB(0, 0, 0) /* COLOR_INFOTEXT */,
    RGB(255, 255, 192) /* COLOR_INFOBK */,
    RGB(184, 180, 184) /* COLOR_ALTERNATEBTNFACE */,
    RGB(0, 0, 255) /* COLOR_HOTLIGHT */,
    RGB(16, 132, 208) /* COLOR_GRADIENTACTIVECAPTION */,
    RGB(181, 181, 181) /* COLOR_GRADIENTINACTIVECAPTION */,
  };

static ATOM AtomInternalPos;

/* Bits in the dwKeyData */
#define KEYDATA_ALT   0x2000

/* FUNCTIONS *****************************************************************/

BOOL IsMaxBoxActive(HWND hWnd)
{
    ULONG uStyle = GetWindowLong( hWnd, GWL_STYLE );
    return (uStyle & WS_MAXIMIZEBOX);
}

BOOL IsCloseBoxActive( HWND hWnd )
{
    ULONG uStyle = GetWindowLong(hWnd, GWL_STYLE );
    return ( uStyle & WS_SYSMENU );
}

BOOL IsMinBoxActive( HWND hWnd )
{
    ULONG uStyle = GetWindowLong( hWnd, GWL_STYLE );
    return (uStyle & WS_MINIMIZEBOX);
}

INT UIGetFrameSizeX( HWND hWnd )
{
    ULONG uStyle = GetWindowLong( hWnd, GWL_STYLE );

    if ( uStyle & WS_THICKFRAME )
        return GetSystemMetrics( SM_CXSIZEFRAME );
    else
        return GetSystemMetrics( SM_CXFRAME );
}

INT UIGetFrameSizeY( HWND hWnd )
{
    ULONG uStyle = GetWindowLong( hWnd, GWL_STYLE );

    if ( uStyle & WS_THICKFRAME )
        return GetSystemMetrics( SM_CYSIZEFRAME );
    else
        return GetSystemMetrics( SM_CYFRAME );
}

VOID
UserSetupInternalPos( VOID )
{
  LPSTR Str = "SysIP";
  AtomInternalPos = GlobalAddAtomA(Str);
}

DWORD STDCALL
GetSysColor(int nIndex)
{
  return SysColours[nIndex];
}

HPEN STDCALL
GetSysColorPen( int nIndex )
{
  return(CreatePen(PS_SOLID, 1, SysColours[nIndex]));
}

HBRUSH STDCALL
GetSysColorBrush( int nIndex )
{
  return(CreateSolidBrush(SysColours[nIndex]));
}


LRESULT STDCALL
DefFrameProcA( HWND hWnd,
	      HWND hWndMDIClient,
	      UINT uMsg,
	      WPARAM wParam,
	      LPARAM lParam )
{
  UNIMPLEMENTED;
  return((LRESULT)0);
}

LRESULT STDCALL
DefFrameProcW(HWND hWnd,
	      HWND hWndMDIClient,
	      UINT uMsg,
	      WPARAM wParam,
	      LPARAM lParam)
{
  UNIMPLEMENTED;
  return((LRESULT)0);
}


BOOL
DefWndRedrawIconTitle(HWND hWnd)
{
  PINTERNALPOS lpPos = (PINTERNALPOS)GetPropA(hWnd,
					      (LPSTR)(DWORD)AtomInternalPos);
  if (lpPos != NULL)
    {
      if (lpPos->IconTitle != NULL)
	{
	  SendMessageA(lpPos->IconTitle, WM_SHOWWINDOW, TRUE, 0);
	  InvalidateRect(lpPos->IconTitle, NULL, TRUE);
	  return(TRUE);
	}
    }
  return(FALSE);
}

BOOL
UserHasMenu(HWND hWnd, ULONG Style)
{
  return(!(Style & WS_CHILD) && GetWindowLong(hWnd, GWL_ID) != 0);
}

ULONG
UserHasAnyFrameStyle(ULONG Style, ULONG ExStyle)
{
  return((Style & (WS_THICKFRAME | WS_DLGFRAME | WS_BORDER)) ||
	 (ExStyle & WS_EX_DLGMODALFRAME) ||
	 (!(Style & (WS_CHILD | WS_POPUP))));
}

ULONG
UserHasDlgFrameStyle(ULONG Style, ULONG ExStyle)
{
  return((ExStyle & WS_EX_DLGMODALFRAME) ||
	 ((Style & WS_DLGFRAME) && (!(Style & WS_THICKFRAME))));
}

ULONG
UserHasThickFrameStyle(ULONG Style, ULONG ExStyle)
{
  return((Style & WS_THICKFRAME) &&
	 (!((Style & (WS_DLGFRAME | WS_BORDER)) == WS_DLGFRAME)));
}

ULONG
UserHasThinFrameStyle(ULONG Style, ULONG ExStyle)
{
  return((Style & WS_BORDER) ||
	 (!(Style & (WS_CHILD | WS_POPUP))));
}

ULONG
UserHasBigFrameStyle(ULONG Style, ULONG ExStyle)
{
  return((Style & (WS_THICKFRAME | WS_DLGFRAME)) ||
	 (ExStyle & WS_EX_DLGMODALFRAME));
}

void UserGetInsideRectNC( HWND hWnd, RECT *rect )
{
  RECT WindowRect;
  ULONG Style;
  ULONG ExStyle;

  Style = GetWindowLong(hWnd, GWL_STYLE);
  ExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
  GetWindowRect(hWnd, &WindowRect);
  rect->top    = rect->left = 0;
  rect->right  = WindowRect.right - WindowRect.left;
  rect->bottom = WindowRect.bottom - WindowRect.top;

  if (Style & WS_ICONIC)
    {
      return;
    }

  /* Remove frame from rectangle */
  if (UserHasThickFrameStyle(Style, ExStyle ))
      {
	InflateRect( rect, -GetSystemMetrics(SM_CXFRAME),
		     -GetSystemMetrics(SM_CYFRAME) );
      }
    else
      {
	if (UserHasDlgFrameStyle(Style, ExStyle ))
	  {
	    InflateRect( rect, -GetSystemMetrics(SM_CXDLGFRAME),
			 -GetSystemMetrics(SM_CYDLGFRAME));
	    /* FIXME: this isn't in NC_AdjustRect? why not? */
	    if (ExStyle & WS_EX_DLGMODALFRAME)
	      InflateRect( rect, -1, 0 );
	  }
	else
	  {
	    if (UserHasThinFrameStyle(Style, ExStyle))
	      {
		InflateRect(rect, -GetSystemMetrics(SM_CXBORDER),
			    -GetSystemMetrics(SM_CYBORDER));
	      }
	  }
      }
}

void UserDrawSysMenuButton( HWND hWnd, HDC hDC, BOOL down )
{
  RECT Rect;
  HDC hDcMem;
  HBITMAP hSavedBitmap;

  hbSysMenu = LoadBitmap(0, MAKEINTRESOURCE(OBM_CLOSE));
  UserGetInsideRectNC(hWnd, &Rect);
  hDcMem = CreateCompatibleDC(hDC);
  hSavedBitmap = SelectObject(hDcMem, hbSysMenu);
  BitBlt(hDC, Rect.left + 2, Rect.top +
         2, 16, 16, hDcMem,
         (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD) ?
	 GetSystemMetrics(SM_CXSIZE): 0, 0, SRCCOPY);
  SelectObject(hDcMem, hSavedBitmap);
  DeleteDC(hDcMem);
}

/* FIXME:  Cache bitmaps, then just bitblt instead of calling DFC() (and
           wasting precious CPU cycles) every time */

static void UserDrawCloseButton ( HWND hWnd, HDC hDC, BOOL bDown )
{
    RECT rect;

    BOOL bToolWindow = GetWindowLongA( hWnd, GWL_EXSTYLE ) & WS_EX_TOOLWINDOW;
    INT iBmpWidth =  (bToolWindow ? GetSystemMetrics(SM_CXSMSIZE) :
                      GetSystemMetrics(SM_CXSIZE)) - 2;
    INT iBmpHeight = (bToolWindow ? GetSystemMetrics(SM_CYSMSIZE) :
                      GetSystemMetrics(SM_CYSIZE) - 4);
    INT OffsetX = UIGetFrameSizeY( hWnd );
    INT OffsetY = UIGetFrameSizeY( hWnd );
    
    
    if(!(GetWindowLong( hWnd, GWL_STYLE ) & WS_SYSMENU))
    {
        return;
    }
    GetWindowRect( hWnd, &rect );
    
    rect.right = rect.right - rect.left;
    rect.bottom = rect.bottom - rect.top;
    rect.left = rect.top = 0;
    SetRect(&rect,
            rect.right - OffsetX - iBmpWidth - 3,
            OffsetY + 2,
            rect.right - OffsetX - 3,
            rect.top + iBmpHeight + OffsetY + 2 );      
            
    DrawFrameControl( hDC, &rect, DFC_CAPTION,
                      (DFCS_CAPTIONCLOSE |
                       (bDown ? DFCS_PUSHED : 0) |
                       (IsCloseBoxActive(hWnd) ? 0 : DFCS_INACTIVE)) );
}
           
static void UserDrawMaxButton( HWND hWnd, HDC hDC, BOOL bDown )
{

    RECT rect;
    INT iBmpWidth = GetSystemMetrics(SM_CXSIZE) - 2;
    INT iBmpHeight = GetSystemMetrics(SM_CYSIZE) - 4;

    INT OffsetX = UIGetFrameSizeY( hWnd );
    INT OffsetY = UIGetFrameSizeY( hWnd );
    
    GetWindowRect( hWnd, &rect );

    if (!IsMinBoxActive(hWnd) && !IsMaxBoxActive(hWnd))
        return;    
    if ((GetWindowLongA( hWnd, GWL_EXSTYLE ) & WS_EX_TOOLWINDOW) == TRUE)
        return;   /* ToolWindows don't have min/max buttons */
        
    rect.right = rect.right - rect.left;
    rect.bottom = rect.bottom - rect.top;
    rect.left = rect.top = 0;
    SetRect(&rect,
            rect.right - OffsetX - (iBmpWidth*2) - 5,
            OffsetY + 2,
            rect.right - iBmpWidth - OffsetX - 5,
            rect.top + iBmpHeight + OffsetY + 2 );
    
    DrawFrameControl( hDC, &rect, DFC_CAPTION,
                     (IsZoomed(hWnd) ? DFCS_CAPTIONRESTORE : DFCS_CAPTIONMAX) |
                     (bDown ? DFCS_PUSHED : 0) |
                     (IsMaxBoxActive(hWnd) ? 0 : DFCS_INACTIVE) );
    
}

static void UserDrawMinButton( HWND hWnd, HDC hDC, BOOL bDown )
{

    RECT rect;
    INT iBmpWidth = GetSystemMetrics(SM_CXSIZE) - 2;
    INT iBmpHeight = GetSystemMetrics(SM_CYSIZE) - 4;
    
    INT OffsetX = UIGetFrameSizeX( hWnd );
    INT OffsetY = UIGetFrameSizeY( hWnd );
    
    GetWindowRect( hWnd, &rect );
    
    if (!IsMinBoxActive(hWnd) && !IsMaxBoxActive(hWnd))
        return;    
    if ((GetWindowLongA( hWnd, GWL_EXSTYLE ) & WS_EX_TOOLWINDOW) == TRUE)
        return;   /* ToolWindows don't have min/max buttons */
        
    rect.right = rect.right - rect.left;
    rect.bottom = rect.bottom - rect.top;
    rect.left = rect.top = 0;
    SetRect(&rect,
            rect.right - OffsetX - (iBmpWidth*3) - 5,
            OffsetY + 2,
            rect.right - (iBmpWidth * 2) - OffsetX - 5,
            rect.top + iBmpHeight + OffsetY + 2 );  
    DrawFrameControl( hDC, &rect, DFC_CAPTION,
                     DFCS_CAPTIONMIN | (bDown ? DFCS_PUSHED : 0) |
                     (IsMinBoxActive(hWnd) ? 0 : DFCS_INACTIVE) );
}

static void UserDrawCaptionNC( HDC hDC, RECT *rect, HWND hWnd,
			    DWORD style, BOOL active )
{
    RECT r = *rect;
    char buffer[256];
    /* FIXME:  Implement and Use DrawCaption() */
    SelectObject( hDC, GetSysColorBrush(active ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION) );
    
	PatBlt(hDC,rect->left + GetSystemMetrics(SM_CXFRAME), rect->top +
           GetSystemMetrics(SM_CYFRAME), rect->right - (GetSystemMetrics(SM_CXFRAME) * 2), (rect->top + 
           GetSystemMetrics(SM_CYCAPTION)) - 1, PATCOPY );
    
    if (style & WS_SYSMENU)
    {
        UserDrawSysMenuButton( hWnd, hDC, FALSE);
        r.left += GetSystemMetrics(SM_CXSIZE) + 1;
        UserDrawCloseButton( hWnd, hDC, FALSE);
        r.right -= GetSystemMetrics(SM_CXSMSIZE) + 1;
        UserDrawMinButton(hWnd, hDC, FALSE);
        UserDrawMaxButton(hWnd, hDC, FALSE);
    }
    if (GetWindowTextA( hWnd, buffer, sizeof(buffer) ))
    {
        NONCLIENTMETRICS nclm;
        HFONT hFont, hOldFont;

        nclm.cbSize = sizeof(NONCLIENTMETRICS);
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
        SetTextColor(hDC, SysColours[ active ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT]);
        SetBkMode( hDC, TRANSPARENT );
        if (style & WS_EX_TOOLWINDOW)
            hFont = CreateFontIndirectW(&nclm.lfSmCaptionFont);
        else
            hFont = CreateFontIndirectW(&nclm.lfCaptionFont);
        hOldFont = SelectObject(hDC, hFont);
        TextOutA(hDC, r.left + (GetSystemMetrics(SM_CXDLGFRAME) * 2), rect->top + (nclm.lfCaptionFont.lfHeight / 2), buffer, strlen(buffer));
        DeleteObject (SelectObject (hDC, hOldFont));
    }
}

VOID
UserDrawFrameNC(HWND hWnd, RECT* rect, BOOL dlgFrame, BOOL active)
{
	HDC hDC = GetWindowDC(hWnd);
    SelectObject( hDC, GetSysColorBrush(COLOR_WINDOW) ); 
    DrawEdge(hDC, rect,EDGE_RAISED, BF_RECT | BF_MIDDLE);
}

void SCROLL_DrawScrollBar (HWND hWnd, HDC hDC, INT nBar, BOOL arrows, BOOL interior);

VOID
DefWndDoPaintNC(HWND hWnd, HRGN clip)
{
  ULONG Active;
  HDC hDC;
  RECT rect;
  ULONG Style;
  ULONG ExStyle;
  Active = GetWindowLongW(hWnd, GWL_STYLE) & WIN_NCACTIVATED;
  Style = GetWindowLong(hWnd, GWL_STYLE);
  ExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

  hDC = GetDCEx(hWnd, (clip > (HRGN)1) ? clip : 0, DCX_USESTYLE | DCX_WINDOW |
		((clip > (HRGN)1) ? (DCX_INTERSECTRGN | DCX_KEEPCLIPRGN) : 0));
  if (hDC == 0)
    {
      return;
    }

  /* FIXME: Test whether we need to draw anything at all. */

  GetWindowRect(hWnd, &rect);
  rect.right = rect.right - rect.left;
  rect.bottom = rect.bottom - rect.top;
  rect.top = rect.left = 0;
  SelectObject(hDC, GetSysColorPen(COLOR_WINDOWFRAME));
  if (UserHasThickFrameStyle(Style, ExStyle))
    {
      UserDrawFrameNC(hWnd, &rect, FALSE, Active);
    }
  else if (UserHasDlgFrameStyle(Style, ExStyle))
    {
      UserDrawFrameNC(hWnd, &rect, TRUE, Active);
    }
  if (Style & WS_CAPTION)
    {
      RECT r = rect;
      r.bottom = rect.top + GetSystemMetrics(SM_CYSIZE);
      rect.top += GetSystemMetrics(SM_CYSIZE) +
	GetSystemMetrics(SM_CYBORDER);
      UserDrawCaptionNC(hDC, &r, hWnd, Style, Active);
    }

/*  FIXME: Draw menu bar.  */

  DPRINT("drawing scrollbars..\n");
/*  Draw scrollbars */
  if (Style & WS_VSCROLL)
      SCROLL_DrawScrollBar(hWnd, hDC, SB_VERT, TRUE, TRUE);
  if (Style & WS_HSCROLL)
      SCROLL_DrawScrollBar(hWnd, hDC, SB_HORZ, TRUE, TRUE);

  /* FIXME: Draw size box.*/

  ReleaseDC(hWnd, hDC);
  
}

LRESULT
DefWndPaintNC(HWND hWnd, HRGN clip)
{
  if (IsWindowVisible(hWnd))
    {
      if (IsIconic(hWnd))
	{
	  DefWndRedrawIconTitle(hWnd);
	}
      else
	{
	  DefWndDoPaintNC(hWnd, clip);
	}
    }
  return(0);
}

LRESULT
DefWndHitTestNC(HWND hWnd, POINT Point)
{
  RECT WindowRect;
  ULONG Style = GetWindowLong(hWnd, GWL_STYLE);
  ULONG ExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

  GetWindowRect(hWnd, &WindowRect);
  if (!PtInRect(&WindowRect, Point))
    {
      
      return(HTNOWHERE);
    }
  if (Style & WS_MINIMIZE)
    {
      return(HTCAPTION);
    }
  if (UserHasThickFrameStyle(Style, ExStyle))
    {
      InflateRect(&WindowRect, -GetSystemMetrics(SM_CXFRAME),
		  -GetSystemMetrics(SM_CYFRAME));
      if (!PtInRect(&WindowRect, Point))
	{
	  if (Point.y < WindowRect.top)
	    {
	      if (Point.x < (WindowRect.left + GetSystemMetrics(SM_CXSIZE)))
		{
		  return(HTTOPLEFT);
		}
	      if (Point.x >= (WindowRect.right - GetSystemMetrics(SM_CXSIZE)))
		{
		  return(HTTOPRIGHT);
		}
	      return(HTTOP);
	    }
	  if (Point.y >= WindowRect.bottom)
	    {
	      if (Point.x < (WindowRect.left + GetSystemMetrics(SM_CXSIZE)))
		{
		  return(HTBOTTOMLEFT);
		}
	      if (Point.x >= (WindowRect.right - GetSystemMetrics(SM_CXSIZE)))
		{
		  return(HTBOTTOMRIGHT);
		}
	      return(HTBOTTOM);
	    }
	  if (Point.x < WindowRect.left)
	    {
	      if (Point.y < (WindowRect.top + GetSystemMetrics(SM_CYSIZE)))
		{
		  return(HTTOPLEFT);
		}
	      if (Point.y >= (WindowRect.bottom - GetSystemMetrics(SM_CYSIZE)))
		{
		  return(HTBOTTOMLEFT);
		}
	      return(HTLEFT);
	    }
	  if (Point.x >= WindowRect.right)
	    {
	      if (Point.y < (WindowRect.top + GetSystemMetrics(SM_CYSIZE)))
		{
		  return(HTTOPRIGHT);
		}
	      if (Point.y >= (WindowRect.bottom - GetSystemMetrics(SM_CYSIZE)))
		{
		  return(HTBOTTOMRIGHT);
		}
	      return(HTRIGHT);
	    }
	}
    }
  else
    {
      if (UserHasDlgFrameStyle(Style, ExStyle))
	{
	  InflateRect(&WindowRect, -GetSystemMetrics(SM_CXDLGFRAME),
		      -GetSystemMetrics(SM_CYDLGFRAME));
	}
      else if (UserHasThinFrameStyle(Style, ExStyle))
	{
	  InflateRect(&WindowRect, -GetSystemMetrics(SM_CXBORDER),
		      -GetSystemMetrics(SM_CYBORDER));
	}
      if (!PtInRect(&WindowRect, Point))
	{
	  return(HTBORDER);
	}
    }

  if ((Style & WS_CAPTION) == WS_CAPTION)
    {
      WindowRect.top += (GetSystemMetrics(SM_CYCAPTION) -
	GetSystemMetrics(SM_CYBORDER));
      if (!PtInRect(&WindowRect, Point))
	{
	  if ((Style & WS_SYSMENU) && !(ExStyle & WS_EX_TOOLWINDOW))
	    {
	      WindowRect.left += GetSystemMetrics(SM_CXSIZE);
	      WindowRect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
	    }
	  if (Point.x <= WindowRect.left)
	    {
	      return(HTSYSMENU);
	    }
	  if (WindowRect.right <= Point.x)
	    {
	      return(HTCLOSE);
	    }

	  if (Style & WS_MAXIMIZEBOX || Style & WS_MINIMIZEBOX)
	    {
	      WindowRect.right -= GetSystemMetrics(SM_CXSIZE) - 2;
	    }
	  if (Point.x >= WindowRect.right)
	    {
	      return(HTMAXBUTTON);
	    }

	  if (Style & WS_MINIMIZEBOX)
	    {
	      WindowRect.right -= GetSystemMetrics(SM_CXSIZE) - 2;
	    }
	  if (Point.x >= WindowRect.right)
	    {
	      return(HTMINBUTTON);
	    }
	  return(HTCAPTION);
	}
    }

  ScreenToClient(hWnd, &Point);
  GetClientRect(hWnd, &WindowRect);

  if (PtInRect(&WindowRect, Point))
    {
      return(HTCLIENT);
    }

  if (Style & WS_VSCROLL)
    {
      WindowRect.right += GetSystemMetrics(SM_CXVSCROLL);
      if (PtInRect(&WindowRect, Point))
	{
	  return(HTVSCROLL);
	}
    }

  if (Style & WS_HSCROLL)
    {
      WindowRect.bottom += GetSystemMetrics(SM_CYHSCROLL);
      if (PtInRect(&WindowRect, Point))
	{
	  if ((Style & WS_VSCROLL) &&
	      (Point.x >= (WindowRect.right - GetSystemMetrics(SM_CXVSCROLL))))
	    {
	      return(HTSIZE);
	    }
	  return(HTHSCROLL);
	}
    }

  if (UserHasMenu(hWnd, Style))
    {
      if (Point.y < 0 && Point.x >= 0 && Point.x <= WindowRect.right)
	{
	  return(HTMENU);
	}
    }

  return(HTNOWHERE);
}

VOID
DefWndDrawSysButton(HWND hWnd, HDC hDC, BOOL Down)
{
  UNIMPLEMENTED;
}

LRESULT
DefWndHandleLButtonDownNC(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
        case HTCAPTION:
        {
	        HWND hTopWnd = GetAncestor(hWnd, GA_ROOT);
	        if (SetActiveWindow(hTopWnd) || GetActiveWindow() == hTopWnd)
	        {
	            SendMessageA(hWnd, WM_SYSCOMMAND, SC_MOVE + HTCAPTION, lParam);
	        }
	        break;
        }
        case HTSYSMENU:
        {
	        if (GetWindowLong(hWnd, GWL_STYLE) & WS_SYSMENU)
            {
	            if (!(GetWindowLong(hWnd, GWL_STYLE) & WS_MINIMIZE))
	            {
		            HDC hDC = GetWindowDC(hWnd);
		            DefWndDrawSysButton(hWnd, hDC, TRUE);
		            ReleaseDC(hWnd, hDC);
	            }
	        SendMessageA(hWnd, WM_SYSCOMMAND, SC_MOUSEMENU + HTSYSMENU,
			             lParam);
	        }
            break;
        }
        case HTMENU:
        {
            SendMessageA(hWnd, WM_SYSCOMMAND, SC_MOUSEMENU, lParam);
            break;
        }
        case HTHSCROLL:
        {
            SendMessageA(hWnd, WM_SYSCOMMAND, SC_HSCROLL + HTHSCROLL, lParam);
            break;
        }
        case HTVSCROLL:
        {
            SendMessageA(hWnd, WM_SYSCOMMAND, SC_VSCROLL + HTVSCROLL, lParam);
            break;
        }
        case HTMINBUTTON:
        {
            UserDrawMinButton(hWnd, GetWindowDC(hWnd), IsMinBoxActive(hWnd) );
            break;
        }
        case HTMAXBUTTON:
        {
            UserDrawMaxButton(hWnd,GetWindowDC(hWnd), IsMaxBoxActive(hWnd) );
            break;
        }
        case HTCLOSE:
        {
            UserDrawCloseButton(hWnd,GetWindowDC(hWnd),TRUE);
            break;
        }
        case HTLEFT:
        case HTRIGHT:
        case HTTOP:
        case HTBOTTOM:
        case HTTOPLEFT:
        case HTTOPRIGHT:
        case HTBOTTOMLEFT:
        case HTBOTTOMRIGHT:
        {
            SendMessageA(hWnd, WM_SYSCOMMAND, SC_SIZE + wParam - 2, lParam);
            break;
        }
    }
    return(0);
}

LRESULT
DefWndHandleLButtonDblClkNC(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  UNIMPLEMENTED;
  return(0);
}

LRESULT
DefWndHandleLButtonUpNC(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    UserDrawMinButton(hWnd,GetWindowDC(hWnd),FALSE);
    UserDrawMaxButton(hWnd,GetWindowDC(hWnd),FALSE);
    UserDrawCloseButton(hWnd,GetWindowDC(hWnd),FALSE);
    switch (wParam)
    {
        case HTMINBUTTON:
        {
            SendMessageA(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            break;
        }
        case HTMAXBUTTON:
        {
            SendMessageA(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
            break;
        }
        case HTCLOSE:
        {
            SendMessageA(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
            break;
        }
    }
  return(0);
}

LRESULT
DefWndHandleActiveNC(HWND hWnd, WPARAM wParam)
{
  UNIMPLEMENTED;
  return(0);
}

VOID
DefWndSetRedraw(HWND hWnd, WPARAM wParam)
{
  UNIMPLEMENTED;
}

LRESULT
DefWndHandleSetCursor(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  /* Not for child windows. */
  if (hWnd != (HWND)wParam)
    {
      return(0);
    }

  switch(LOWORD(lParam))
    {
    case HTERROR:
      {
	WORD Msg = HIWORD(lParam);
	if (Msg == WM_LBUTTONDOWN || Msg == WM_MBUTTONDOWN ||
	    Msg == WM_RBUTTONDOWN)
	  {
	    MessageBeep(0);
	  }
	break;
      }

    case HTCLIENT:
      {
	HICON hCursor = (HICON)GetClassLong(hWnd, GCL_HCURSOR);
	if (hCursor)
	  {
	    SetCursor(hCursor);
	    return(TRUE);
	  }
	return(FALSE);
      }

    case HTLEFT:
    case HTRIGHT:
      {
	return((LRESULT)SetCursor(LoadCursorW(0, IDC_SIZEWE)));
      }

    case HTTOP:
    case HTBOTTOM:
      {
	return((LRESULT)SetCursor(LoadCursorW(0, IDC_SIZENS)));
      }

    case HTTOPLEFT:
    case HTBOTTOMRIGHT:
      {
	return((LRESULT)SetCursor(LoadCursorW(0, IDC_SIZENWSE)));
      }

    case HTBOTTOMLEFT:
    case HTTOPRIGHT:
      {
	return((LRESULT)SetCursor(LoadCursorW(0, IDC_SIZENESW)));
      }
    }
  return((LRESULT)SetCursor(LoadCursorW(0, IDC_ARROW)));
}

LRESULT
DefWndHandleSysCommand(HWND hWnd, WPARAM wParam, POINT Pt)
{
  switch (wParam)
    {
      case SC_CLOSE:
        SendMessageA(hWnd, WM_CLOSE, 0, 0);
        break;
      default:
      /* FIXME: Implement */
        UNIMPLEMENTED;
        break;
    }

  return(0);
}

VOID
DefWndAdjustRect(RECT* Rect, ULONG Style, BOOL Menu, ULONG ExStyle)
{
  if (Style & WS_ICONIC)
    {
      return;
    }

  if (UserHasThickFrameStyle(Style, ExStyle))
    {
      InflateRect(Rect, GetSystemMetrics(SM_CXFRAME),
		  GetSystemMetrics(SM_CYFRAME));
    }
  else if (UserHasDlgFrameStyle(Style, ExStyle))
    {
      InflateRect(Rect, GetSystemMetrics(SM_CXDLGFRAME),
		  GetSystemMetrics(SM_CYDLGFRAME));
    }
  else if (UserHasThinFrameStyle(Style, ExStyle))
    {
      InflateRect(Rect, GetSystemMetrics(SM_CXBORDER),
		  GetSystemMetrics(SM_CYBORDER));
    }
  if (Style & WS_CAPTION)
    {
      Rect->top -= (GetSystemMetrics(SM_CYCAPTION) -
	GetSystemMetrics(SM_CYBORDER)) + 1;
    }
  if (Menu)
    {
      Rect->top -= GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYBORDER);
    }
  if (Style & WS_VSCROLL)
    {
      Rect->right += GetSystemMetrics(SM_CXVSCROLL) - 1;
      if (UserHasAnyFrameStyle(Style, ExStyle))
	{
	  Rect->right++;
	}
    }
  if (Style & WS_HSCROLL)
    {
      Rect->bottom += GetSystemMetrics(SM_CYHSCROLL) - 1;
      if (UserHasAnyFrameStyle(Style, ExStyle))
	{
	  Rect->bottom++;
	}
    }
}

LRESULT STDCALL
DefWndNCCalcSize(HWND hWnd, RECT* Rect)
{
  LRESULT Result = 0;
  LONG Style = GetClassLongW(hWnd, GCL_STYLE);
  RECT TmpRect = {0, 0, 0, 0};

  if (Style & CS_VREDRAW)
    {
      Result |= WVR_VREDRAW;
    }
  if (Style & CS_HREDRAW)
    {
      Result |= WVR_HREDRAW;
    }

  if (!(GetWindowLong(hWnd, GWL_STYLE) & WS_MINIMIZE))
    {
      DefWndAdjustRect(&TmpRect, GetWindowLong(hWnd, GWL_STYLE),
		       FALSE, GetWindowLong(hWnd, GWL_EXSTYLE));
      Rect->left -= TmpRect.left;
      Rect->top -= TmpRect.top;
      Rect->right -= TmpRect.right;
      Rect->bottom -= TmpRect.bottom;
      /* FIXME: Adjust if the window has a menu. */
      Rect->bottom = max(Rect->top, Rect->bottom);
      Rect->right = max(Rect->left, Rect->right);
    }
  return(Result);
}

LRESULT
DefWndHandleWindowPosChanging(HWND hWnd, WINDOWPOS* Pos)
{
  UNIMPLEMENTED;
  return 0;
}

LRESULT STDCALL
User32DefWindowProc(HWND hWnd,
		    UINT Msg,
		    WPARAM wParam,
		    LPARAM lParam,
		    BOOL bUnicode)
{
  switch (Msg)
    {
    case WM_NCPAINT:
      {
	return(DefWndPaintNC(hWnd, (HRGN)wParam));
      }
    case WM_WINDOWPOSCHANGING:
      {
        DbgPrint("WM_WINDOWPOSCHANGING\n\n");
        break;
      }
    case WM_NCHITTEST:
      {
	POINT Point;
	Point.x = SLOWORD(lParam);
	Point.y = SHIWORD(lParam);
	return(DefWndHitTestNC(hWnd, Point));
      }

    case WM_NCLBUTTONDOWN:
      {
	return(DefWndHandleLButtonDownNC(hWnd, wParam, lParam));
      }

    case WM_NCLBUTTONUP:
      {
	return(DefWndHandleLButtonUpNC(hWnd, wParam, lParam));
      }

    case WM_LBUTTONDBLCLK:
    case WM_NCLBUTTONDBLCLK:
      {
	return(DefWndHandleLButtonDblClkNC(hWnd, wParam, lParam));
      }

    case WM_NCRBUTTONDOWN:
      {
	if (wParam == HTCAPTION)
	  {
	    SetCapture(hWnd);
	  }
	break;
      }
    case WM_LBUTTONUP:
    {
        break;
    }
    case WM_RBUTTONUP:
      {
	POINT Pt;
	if (hWnd == GetCapture())
	  {
	    ReleaseCapture();
	  }
	Pt.x = SLOWORD(lParam);
	Pt.y = SHIWORD(lParam);
	ClientToScreen(hWnd, &Pt);
	lParam = MAKELPARAM(Pt.x, Pt.y);
	if (bUnicode)
	  {
	    SendMessageW(hWnd, WM_CONTEXTMENU, (WPARAM)hWnd, lParam);
	  }
	else
	  {
	    SendMessageA (hWnd, WM_CONTEXTMENU, (WPARAM)hWnd, lParam);
	  }
	break;
      }

    case WM_NCRBUTTONUP:
      {
	/* Wine does nothing here. */
	break;
      }

    case WM_CONTEXTMENU:
      {
	if (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD)
	  {
	    if (bUnicode)
	      {
		SendMessageW(GetParent(hWnd), Msg, wParam, lParam);
	      }
	    else
	      {
		SendMessageA(hWnd, WM_CONTEXTMENU, wParam, lParam);
	      }
	  }
	else
	  {
	    LONG HitCode;
	    POINT Pt;

	    Pt.x = SLOWORD(lParam);
	    Pt.y = SHIWORD(lParam);

	    if (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD)
	      {
		ScreenToClient(GetParent(hWnd), &Pt);
	      }

	    HitCode = DefWndHitTestNC(hWnd, Pt);

	    if (HitCode == HTCAPTION || HitCode == HTSYSMENU)
	      {
		TrackPopupMenu(GetSystemMenu(hWnd, FALSE),
			       TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
			       Pt.x, Pt.y, 0, hWnd, NULL);
	      }
	  }
	break;
      }

    case WM_NCACTIVATE:
      {
	return(DefWndHandleActiveNC(hWnd, wParam));
      }

    case WM_NCDESTROY:
      {
	return(0);
      }

    case WM_PRINT:
      {
	return(0);
      }

    case WM_PAINTICON:
    case WM_PAINT:
      {
	PAINTSTRUCT Ps;
	HDC hDC = BeginPaint(hWnd, &Ps);
	if (hDC)
	  {
	  	
	    HICON hIcon;
	    if (GetWindowLongW(hWnd, GWL_STYLE) & WS_MINIMIZE &&
		(hIcon = (HICON)GetClassLongW(hWnd, GCL_HICON)) != NULL)
	      {
		RECT WindowRect;
		INT x, y;
		GetWindowRect(hWnd, &WindowRect);
		x = (WindowRect.right - WindowRect.left -
		     GetSystemMetrics(SM_CXICON)) / 2;
		y = (WindowRect.bottom - WindowRect.top -
		     GetSystemMetrics(SM_CYICON)) / 2;
		DrawIcon(hDC, x, y, hIcon);
	      } 
		if(GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_CLIENTEDGE)
		{
			RECT WindowRect;
			GetClientRect(hWnd, &WindowRect);
			DrawEdge(hDC, &WindowRect, EDGE_SUNKEN, BF_RECT);
		}
	    EndPaint(hWnd, &Ps);
	  }
	return(0);
      }

    case WM_SYNCPAINT:
      {
	HRGN hRgn;
	hRgn = CreateRectRgn(0, 0, 0, 0);
	if (GetUpdateRgn(hWnd, hRgn, FALSE) != NULLREGION)
	  {
	    RedrawWindow(hWnd, NULL, hRgn,
			 RDW_ERASENOW | RDW_ERASE | RDW_FRAME |
			 RDW_ALLCHILDREN);
	  }
	DeleteObject(hRgn);
	return(0);
      }

    case WM_SETREDRAW:
      {
	DefWndSetRedraw(hWnd, wParam);
	return(0);
      }

    case WM_CLOSE:
      {
	DestroyWindow(hWnd);
	return(0);
      }

    case WM_MOUSEACTIVATE:
      {
	if (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD)
	  {
	    LONG Ret;
	    if (bUnicode)
	      {
		Ret = SendMessageW(GetParent(hWnd), WM_MOUSEACTIVATE,
				   wParam, lParam);
	      }
	    else
	      {
		Ret = SendMessageA(GetParent(hWnd), WM_MOUSEACTIVATE,
				   wParam, lParam);
	      }
	    if (Ret)
	      {
		return(Ret);
	      }
	  }
	return((LOWORD(lParam) >= HTCLIENT) ? MA_ACTIVATE : MA_NOACTIVATE);
      }

    case WM_ACTIVATE:
      {
  /* Check if the window is minimized. */
	if (LOWORD(lParam) != WA_INACTIVE &&
	    !(GetWindowLong(hWnd, GWL_STYLE) & WS_MINIMIZE))
	  {
	    SetFocus(hWnd);
	  }
	break;
      }

    case WM_MOUSEWHEEL:
      {
	if (GetWindowLong(hWnd, GWL_STYLE & WS_CHILD))
	  {
	    if (bUnicode)
	      {
		return(SendMessageW(GetParent(hWnd), WM_MOUSEWHEEL,
				    wParam, lParam));
	      }
	    else
	      {
		return(SendMessageA(GetParent(hWnd), WM_MOUSEWHEEL,
				    wParam, lParam));
	      }
	  }
	break;
      }

    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
      {
      	
	RECT Rect;
	HBRUSH hBrush = (HBRUSH)GetClassLongW(hWnd, GCL_HBRBACKGROUND);
	GetClipBox((HDC)wParam, &Rect);
	FillRect((HDC)wParam, &Rect, hBrush);
	return(1);
      }

    case WM_GETDLGCODE:
      {
	return(0);
      }

      /* FIXME: Implement colour controls. */

    case WM_SETCURSOR:
      {
	if (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD)
	  {
	    if (LOWORD(lParam) < HTLEFT || LOWORD(lParam) > HTBOTTOMRIGHT)
	      {
		BOOL bResult;
		if (bUnicode)
		  {
		    bResult = SendMessageW(GetParent(hWnd), WM_SETCURSOR,
					   wParam, lParam);
		  }
		else
		  {
		    bResult = SendMessageA(GetParent(hWnd), WM_SETCURSOR,
					   wParam, lParam);
		  }
		if (bResult)
		  {
		    return(TRUE);
		  }
	      }
	  }
	return(DefWndHandleSetCursor(hWnd, wParam, lParam));
      }

    case WM_SYSCOMMAND:
      {
	POINT Pt;
	Pt.x = SLOWORD(lParam);
	Pt.y = SHIWORD(lParam);
	return(DefWndHandleSysCommand(hWnd, wParam, Pt));
      }

      /* FIXME: Handle key messages. */

    case WM_SHOWWINDOW:
      {
	if (lParam)
	  {
	    return(0);
	  }
	/* FIXME: Not done correctly */
	if ((GetWindowLongW(hWnd, GWL_STYLE) & WS_VISIBLE && !wParam) ||
	    (!(GetWindowLongW(hWnd, GWL_STYLE) & WS_VISIBLE) && wParam))
	  {
	    return(0);
	  }
	ShowWindow(hWnd, wParam ? SW_SHOWNA : SW_HIDE);
	break;
      }

    case WM_CANCELMODE:
      {
	/* FIXME: Check for a desktop. */
	if (GetCapture() == hWnd)
	  {
	    ReleaseCapture();
	  }
	break;
      }

    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
      return(-1);

    case WM_DROPOBJECT:
      /* FIXME: Implement this. */
      break;

    case WM_QUERYDROPOBJECT:
      {
	if (GetWindowLongW(hWnd, GWL_EXSTYLE) & WS_EX_ACCEPTFILES)
	  {
	    return(1);
	  }
	break;
      }

    case WM_QUERYDRAGICON:
      {
	UINT Len;
	HICON hIcon;

	hIcon = (HICON)GetClassLongW(hWnd, GCL_HICON);
	if (hIcon)
	  {
	    return((LRESULT)hIcon);
	  }
	for (Len = 1; Len < 64; Len++)
	  {
	    if ((hIcon = LoadIconW(NULL, MAKEINTRESOURCE(Len))) != NULL)
	      {
		return((LRESULT)hIcon);
	      }
	  }
	return((LRESULT)LoadIconW(0, IDI_APPLICATION));
      }

      /* FIXME: WM_ISACTIVEICON */

    case WM_NOTIFYFORMAT:
      {
	if (IsWindowUnicode(hWnd))
	  {
	    return(NFR_UNICODE);
	  }
	else
	  {
	    return(NFR_ANSI);
	  }
      }

    case WM_SETICON:
      {
	INT Index = (wParam != 0) ? GCL_HICON : GCL_HICONSM;
	HICON hOldIcon = (HICON)GetClassLongW(hWnd, Index);
	SetClassLongW(hWnd, Index, lParam);
	SetWindowPos(hWnd, 0, 0, 0, 0, 0,
		     SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
		     SWP_NOACTIVATE | SWP_NOZORDER);
	return((LRESULT)hOldIcon);
      }

    case WM_GETICON:
      {
	INT Index = (wParam != 0) ? GCL_HICON : GCL_HICONSM;
	return(GetClassLongW(hWnd, Index));
      }

    case WM_HELP:
      {
	if (bUnicode)
	  {
	    SendMessageW(GetParent(hWnd), Msg, wParam, lParam);
	  }
	else
	  {
	    SendMessageA(GetParent(hWnd), Msg, wParam, lParam);
	  }
	break;
      }

    case WM_SYSKEYDOWN:
    	if (HIWORD(lParam) & KEYDATA_ALT)
      	{
     	    if (wParam == VK_F4)  /* Try to close the window */
     	      {
              //HWND hTopWnd = GetAncestor(hWnd, GA_ROOT);
              HWND hTopWnd = hWnd;
              if (!(GetClassLongW(hTopWnd, GCL_STYLE) & CS_NOCLOSE))
                {
                	if (bUnicode)
                	  {
                	    PostMessageW(hTopWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
                	  }
                	else
                	  {
                	    PostMessageA(hTopWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
                	  }
                }
            }
      	}
      break;
    }
  return 0;
}

LRESULT STDCALL
DefWindowProcA(HWND hWnd,
	       UINT Msg,
	       WPARAM wParam,
	       LPARAM lParam)
{
  LRESULT Result;
  static LPSTR WindowTextAtom = 0;
  PSTR WindowText;

  switch (Msg)
    {
    case WM_NCCREATE:
      {
	CREATESTRUCTA* Cs = (CREATESTRUCTA*)lParam;
	if (HIWORD(Cs->lpszName))
	  {
	    if (0 == WindowTextAtom)
	      {
		WindowTextAtom =
		  (LPSTR)(ULONG)GlobalAddAtomA("USER32!WindowTextAtomA");
	      }
	    WindowText = RtlAllocateHeap(RtlGetProcessHeap(), 0,
					 strlen(Cs->lpszName) * sizeof(CHAR));
	    strcpy(WindowText, Cs->lpszName);
	    SetPropA(hWnd, WindowTextAtom, WindowText);
	  }
	return(1);
      }

    case WM_NCCALCSIZE:
      {
	return(DefWndNCCalcSize(hWnd, (RECT*)lParam));
      }

    case WM_WINDOWPOSCHANGING:
      {
	return(DefWndHandleWindowPosChanging(hWnd, (WINDOWPOS*)lParam));
      }

    case WM_GETTEXTLENGTH:
      {
	if (WindowTextAtom == 0 ||
	    (WindowText = GetPropA(hWnd, WindowTextAtom)) == NULL)
	  {
	    return(0);
	  }
	return(strlen(WindowText));
      }

    case WM_GETTEXT:
      {
	if (WindowTextAtom == 0 ||
	    (WindowText = GetPropA(hWnd, WindowTextAtom)) == NULL)
	  {
	    if (wParam > 1)
	      {
		*((PSTR)lParam) = '\0';
	      }
	    return(0);
	  }
	strncpy((LPSTR)lParam, WindowText, wParam);
	return(min(wParam, strlen(WindowText)));
      }

    case WM_SETTEXT:
      {
	if (0 == WindowTextAtom)
	  {
	    WindowTextAtom =
	      (LPSTR)(DWORD)GlobalAddAtomA("USER32!WindowTextAtomW");
	  }
	if (WindowTextAtom != 0 &&
	    (WindowText = GetPropA(hWnd, WindowTextAtom)) == NULL)
	  {
	    RtlFreeHeap(RtlGetProcessHeap(), 0, WindowText);
	  }
	WindowText = RtlAllocateHeap(RtlGetProcessHeap(), 0,
				     strlen((PSTR)lParam) * sizeof(CHAR));
	strcpy(WindowText, (PSTR)lParam);
	SetPropA(hWnd, WindowTextAtom, WindowText);
        if (0 != (GetWindowLongW(hWnd, GWL_STYLE) & WS_CAPTION))
	  {
	    DefWndPaintNC(hWnd, (HRGN) 1);
	  }
	Result = (LPARAM) TRUE;
	break;
      }

    case WM_NCDESTROY:
      {
	if (WindowTextAtom != 0 &&
	    (WindowText = RemovePropA(hWnd, WindowTextAtom)) == NULL)
	  {
	    RtlFreeHeap(GetProcessHeap(), 0, WindowText);
	  }
	return(0);
      }

    default:
      Result = User32DefWindowProc(hWnd, Msg, wParam, lParam, FALSE);
      break;
    }

  return(Result);
}

LRESULT STDCALL
DefWindowProcW(HWND hWnd,
	       UINT Msg,
	       WPARAM wParam,
	       LPARAM lParam)
{
  LRESULT Result;
  static LPWSTR WindowTextAtom = 0;
  PWSTR WindowText;

  switch (Msg)
    {
    case WM_NCCREATE:
      {
	CREATESTRUCTW* Cs = (CREATESTRUCTW*)lParam;
	if (HIWORD(Cs->lpszName))
	  {
	    if (0 == WindowTextAtom)
	      {
		WindowTextAtom =
		  (LPWSTR)(DWORD)GlobalAddAtomW(L"USER32!WindowTextAtomW");
	      }
	    WindowText = RtlAllocateHeap(RtlGetProcessHeap(), 0,
					 wcslen(Cs->lpszName) * sizeof(WCHAR));
	    wcscpy(WindowText, Cs->lpszName);
	    SetPropW(hWnd, WindowTextAtom, WindowText);
	  }
	return(1);
      }

    case WM_NCCALCSIZE:
      {
	return(DefWndNCCalcSize(hWnd, (RECT*)lParam));
      }

    case WM_WINDOWPOSCHANGING:
      {
	return(DefWndHandleWindowPosChanging(hWnd, (WINDOWPOS*)lParam));
      }

    case WM_GETTEXTLENGTH:
      {
	if (WindowTextAtom == 0 ||
	    (WindowText = GetPropW(hWnd, WindowTextAtom)) == NULL)
	  {
	    return(0);
	  }
	return(wcslen(WindowText));
      }

    case WM_GETTEXT:
      {
	if (WindowTextAtom == 0 ||
	    (WindowText = GetPropW(hWnd, WindowTextAtom)) == NULL)
	  {
	    if (wParam > 1)
	      {
		((PWSTR)lParam) = '\0';
	      }
	    return(0);
	  }
	wcsncpy((PWSTR)lParam, WindowText, wParam);
	return(min(wParam, wcslen(WindowText)));
      }

    case WM_SETTEXT:
      {
	if (WindowTextAtom != 0)
	  {
	    WindowTextAtom =
	      (LPWSTR)(DWORD)GlobalAddAtom(L"USER32!WindowTextAtomW");
	  }
	if (WindowTextAtom != 0 &&
	    (WindowText = GetPropW(hWnd, WindowTextAtom)) == NULL)
	  {
	    RtlFreeHeap(RtlGetProcessHeap(), 0, WindowText);
	  }
	WindowText = RtlAllocateHeap(RtlGetProcessHeap(), 0,
				     wcslen((PWSTR)lParam) * sizeof(WCHAR));
	wcscpy(WindowText, (PWSTR)lParam);
	SetPropW(hWnd, WindowTextAtom, WindowText);
        if (0 != (GetWindowLongW(hWnd, GWL_STYLE) & WS_CAPTION))
	  {
	    DefWndPaintNC(hWnd, (HRGN) 1);
	  }
	Result = (LPARAM) TRUE;
	break;
      }

    case WM_NCDESTROY:
      {
	if (WindowTextAtom != 0 &&
	    (WindowText = RemovePropW(hWnd, WindowTextAtom)) == NULL)
	  {
	    RtlFreeHeap(RtlGetProcessHeap(), 0, WindowText);
	  }
	return(0);
      }

    default:
      Result = User32DefWindowProc(hWnd, Msg, wParam, lParam, TRUE);
      break;
    }

  return(Result);
}
