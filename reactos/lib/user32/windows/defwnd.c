/* $Id: defwnd.c,v 1.68 2003/08/15 21:56:48 gvg Exp $
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
#include <menu.h>
#include <cursor.h>
#include <winpos.h>

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

BOOL
IsMaxBoxActive(HWND hWnd)
{
    ULONG uStyle = GetWindowLongW( hWnd, GWL_STYLE );
    return (uStyle & WS_MAXIMIZEBOX);
}

BOOL
IsCloseBoxActive( HWND hWnd )
{
    ULONG uStyle = GetWindowLongW(hWnd, GWL_STYLE );
    return ( uStyle & WS_SYSMENU );
}

BOOL
IsMinBoxActive( HWND hWnd )
{
    ULONG uStyle = GetWindowLongW( hWnd, GWL_STYLE );
    return (uStyle & WS_MINIMIZEBOX);
}

INT
UIGetFrameSizeX( HWND hWnd )
{
    ULONG uStyle = GetWindowLongW( hWnd, GWL_STYLE );

    if ( uStyle & WS_THICKFRAME )
        return GetSystemMetrics( SM_CXSIZEFRAME );
    else
        return GetSystemMetrics( SM_CXFRAME );
}

INT
UIGetFrameSizeY( HWND hWnd )
{
    ULONG uStyle = GetWindowLongW( hWnd, GWL_STYLE );

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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
HBRUSH STDCALL
GetSysColorBrush( int nIndex )
{
  return(CreateSolidBrush(SysColours[nIndex]));
}


/*
 * @unimplemented
 */
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

/*
 * @unimplemented
 */
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

PINTERNALPOS
UserGetInternalPos(HWND hWnd)
{
  PINTERNALPOS lpPos;
  lpPos = (PINTERNALPOS)GetPropA(hWnd, (LPSTR)(DWORD)AtomInternalPos);
  return(lpPos);
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
  return(!(Style & WS_CHILD) && GetMenu(hWnd) != 0);
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

void
UserGetInsideRectNC( HWND hWnd, RECT *rect )
{
  RECT WindowRect;
  ULONG Style;
  ULONG ExStyle;

  Style = GetWindowLongW(hWnd, GWL_STYLE);
  ExStyle = GetWindowLongW(hWnd, GWL_EXSTYLE);
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

VOID
UserDrawSysMenuButton( HWND hWnd, HDC hDC, BOOL down )
{
  RECT Rect;
  HDC hDcMem;
  HBITMAP hSavedBitmap;

  hbSysMenu = (HBITMAP)LoadBitmapW(0, MAKEINTRESOURCEW(OBM_CLOSE));
  UserGetInsideRectNC(hWnd, &Rect);
  hDcMem = CreateCompatibleDC(hDC);
  hSavedBitmap = SelectObject(hDcMem, hbSysMenu);
  BitBlt(hDC, Rect.left + 2, Rect.top +
         2, 16, 16, hDcMem,
         (GetWindowLongW(hWnd, GWL_STYLE) & WS_CHILD) ?
	 GetSystemMetrics(SM_CXSIZE): 0, 0, SRCCOPY);
  SelectObject(hDcMem, hSavedBitmap);
  DeleteDC(hDcMem);
}

/* FIXME:  Cache bitmaps, then just bitblt instead of calling DFC() (and
           wasting precious CPU cycles) every time */

static void
UserDrawCaptionButton( HWND hWnd, HDC hDC, BOOL bDown, ULONG Type )
{
  RECT rect;
  INT iBmpWidth = GetSystemMetrics(SM_CXSIZE) - 2;
  INT iBmpHeight = GetSystemMetrics(SM_CYSIZE) - 4;

  INT OffsetX = UIGetFrameSizeX( hWnd );
  INT OffsetY = UIGetFrameSizeY( hWnd );

  if(!(GetWindowLongW( hWnd, GWL_STYLE ) & WS_SYSMENU))
  {
    return;
  }

  GetWindowRect( hWnd, &rect );

  rect.right = rect.right - rect.left;
  rect.bottom = rect.bottom - rect.top;
  rect.left = rect.top = 0;

  switch(Type)
  {
  case DFCS_CAPTIONMIN:
    {
      if ((GetWindowLongW( hWnd, GWL_EXSTYLE ) & WS_EX_TOOLWINDOW) == TRUE)
	return;   /* ToolWindows don't have min/max buttons */

      SetRect(&rect,
	rect.right - OffsetX - (iBmpWidth*3) - 5,
	OffsetY + 2,
	rect.right - (iBmpWidth * 2) - OffsetX - 5,
	rect.top + iBmpHeight + OffsetY + 2 );  
      DrawFrameControl( hDC, &rect, DFC_CAPTION,
	DFCS_CAPTIONMIN | (bDown ? DFCS_PUSHED : 0) |
	(IsMinBoxActive(hWnd) ? 0 : DFCS_INACTIVE) );
      break;
    }
  case DFCS_CAPTIONMAX:
    {
      if ((GetWindowLongW( hWnd, GWL_EXSTYLE ) & WS_EX_TOOLWINDOW) == TRUE)
	return;   /* ToolWindows don't have min/max buttons */
      SetRect(&rect,
	rect.right - OffsetX - (iBmpWidth*2) - 5,
	OffsetY + 2,
	rect.right - iBmpWidth - OffsetX - 5,
	rect.top + iBmpHeight + OffsetY + 2 );
      
      DrawFrameControl( hDC, &rect, DFC_CAPTION,
	(IsZoomed(hWnd) ? DFCS_CAPTIONRESTORE : DFCS_CAPTIONMAX) |
	(bDown ? DFCS_PUSHED : 0) |
	(IsMaxBoxActive(hWnd) ? 0 : DFCS_INACTIVE) );
      break;
    }
  case DFCS_CAPTIONCLOSE:
    {
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
  }
}

static void
UserDrawCaptionNC (
	HDC hDC,
	RECT *rect,
	HWND hWnd,
	DWORD style,
	BOOL active )
{
  RECT r = *rect;
  WCHAR buffer[256];
  /* FIXME:  Implement and Use DrawCaption() */
  SelectObject( hDC, GetSysColorBrush(active ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION) );

  PatBlt(hDC,rect->left + GetSystemMetrics(SM_CXFRAME), rect->top +
     GetSystemMetrics(SM_CYFRAME), rect->right - (GetSystemMetrics(SM_CXFRAME) * 2), (rect->top + 
     GetSystemMetrics(SM_CYCAPTION)) - 1, PATCOPY );
  
  if (style & WS_SYSMENU)
  {
    UserDrawSysMenuButton( hWnd, hDC, FALSE);
    r.left += GetSystemMetrics(SM_CXSIZE) + 1;
    UserDrawCaptionButton( hWnd, hDC, FALSE, DFCS_CAPTIONCLOSE);
    r.right -= GetSystemMetrics(SM_CXSMSIZE) + 1;
    UserDrawCaptionButton( hWnd, hDC, FALSE, DFCS_CAPTIONMIN);
    UserDrawCaptionButton( hWnd, hDC, FALSE, DFCS_CAPTIONMAX);
  }
  if (GetWindowTextW( hWnd, buffer, sizeof(buffer)/sizeof(buffer[0]) ))
  {
    NONCLIENTMETRICSW nclm;
    HFONT hFont, hOldFont;

    nclm.cbSize = sizeof(nclm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
    SetTextColor(hDC, SysColours[ active ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT]);
    SetBkMode( hDC, TRANSPARENT );
    if (style & WS_EX_TOOLWINDOW)
        hFont = CreateFontIndirectW(&nclm.lfSmCaptionFont);
    else
        hFont = CreateFontIndirectW(&nclm.lfCaptionFont);
    hOldFont = SelectObject(hDC, hFont);
    TextOutW(hDC, r.left + (GetSystemMetrics(SM_CXDLGFRAME) * 2), rect->top + (nclm.lfCaptionFont.lfHeight / 2), buffer, wcslen(buffer));
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


void
SCROLL_DrawScrollBar (HWND hWnd, HDC hDC, INT nBar, BOOL arrows, BOOL interior);

VOID
DefWndDoPaintNC(HWND hWnd, HRGN clip)
{
  BOOL Active = FALSE;
  HDC hDC;
  RECT rect;
  ULONG Style;
  ULONG ExStyle;
  int wFrame = 0;

    // This won't work because it conflicts with BS_BITMAP :
  if (GetActiveWindow() == hWnd) Active = TRUE;
  Style = GetWindowLongW(hWnd, GWL_STYLE);
  ExStyle = GetWindowLongW(hWnd, GWL_EXSTYLE);

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
      wFrame = GetSystemMetrics(SM_CXSIZEFRAME);
    }
  else if (UserHasDlgFrameStyle(Style, ExStyle))
    {
      UserDrawFrameNC(hWnd, &rect, TRUE, Active);
      wFrame = GetSystemMetrics(SM_CXDLGFRAME);
    }
  if (Style & WS_CAPTION)
    {
      RECT r = rect;
      r.bottom = rect.top + GetSystemMetrics(SM_CYSIZE);
      rect.top += GetSystemMetrics(SM_CYSIZE) +
	GetSystemMetrics(SM_CYBORDER);
      UserDrawCaptionNC(hDC, &r, hWnd, Style, Active);
    }

  /*  Draw menu bar.  */
  if (UserHasMenu(hWnd, Style))
    {
      RECT r = rect;
      r.bottom = rect.top + GetSystemMetrics(SM_CYMENU);
      r.left += wFrame;
      r.right -= wFrame;
      rect.top += MenuDrawMenuBar(hDC, &r, hWnd, FALSE);
    }

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
  ULONG Style = GetWindowLongW(hWnd, GWL_STYLE);
  ULONG ExStyle = GetWindowLongW(hWnd, GWL_EXSTYLE);

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
	  if (GetWindowLongW(hWnd, GWL_STYLE) & WS_SYSMENU)
            {
	      if (!(GetWindowLongW(hWnd, GWL_STYLE) & WS_MINIMIZE))
		{
		  HDC hDC = GetWindowDC(hWnd);
		  UserDrawSysMenuButton(hWnd, hDC, TRUE);
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
            UserDrawCaptionButton( hWnd, GetWindowDC(hWnd), IsMinBoxActive(hWnd), DFCS_CAPTIONMIN);
            break;
        }
        case HTMAXBUTTON:
        {
            UserDrawCaptionButton( hWnd, GetWindowDC(hWnd), IsMaxBoxActive(hWnd), DFCS_CAPTIONMAX);
            break;
        }
        case HTCLOSE:
        {
            UserDrawCaptionButton( hWnd, GetWindowDC(hWnd), TRUE, DFCS_CAPTIONCLOSE);
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
    UserDrawCaptionButton( hWnd, GetWindowDC(hWnd), FALSE, DFCS_CAPTIONMIN);
    UserDrawCaptionButton( hWnd, GetWindowDC(hWnd), FALSE, DFCS_CAPTIONMAX);
    UserDrawCaptionButton( hWnd, GetWindowDC(hWnd), FALSE, DFCS_CAPTIONCLOSE);
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
	HICON hCursor = (HICON)GetClassLongW(hWnd, GCL_HCURSOR);
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

static LONG 
DefWndStartSizeMove(HWND hWnd, WPARAM wParam, POINT *capturePoint)
{
  LONG hittest = 0;
  POINT pt;
  MSG msg;
  RECT rectWindow;
  ULONG Style = GetWindowLongW(hWnd, GWL_STYLE); 
  
  GetWindowRect(hWnd, &rectWindow);

  if ((wParam & 0xfff0) == SC_MOVE)
    {
      /* Move pointer at the center of the caption */
      RECT rect;
      UserGetInsideRectNC(hWnd, &rect);
      if (Style & WS_SYSMENU)
	rect.left += GetSystemMetrics(SM_CXSIZE) + 1;
      if (Style & WS_MINIMIZEBOX)
	rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
      if (Style & WS_MAXIMIZEBOX)
	rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
      pt.x = rectWindow.left + (rect.right - rect.left) / 2;
      pt.y = rectWindow.top + rect.top + GetSystemMetrics(SM_CYSIZE)/2;
      hittest = HTCAPTION;
      *capturePoint = pt;
    }
  else  /* SC_SIZE */
    {
      while(!hittest)
	{
	  GetMessageW(&msg, NULL, 0, 0);
	  switch(msg.message)
	    {
	    case WM_MOUSEMOVE:
	      hittest = DefWndHitTestNC(hWnd, msg.pt);
	      if ((hittest < HTLEFT) || (hittest > HTBOTTOMRIGHT))
		hittest = 0;
	      break;
	      
	    case WM_LBUTTONUP:
	      return 0;
	      
	    case WM_KEYDOWN:
	      switch(msg.wParam)
		{
		case VK_UP:
		  hittest = HTTOP;
		  pt.x =(rectWindow.left+rectWindow.right)/2;
		  pt.y = rectWindow.top + GetSystemMetrics(SM_CYFRAME) / 2;
		  break;
		case VK_DOWN:
		  hittest = HTBOTTOM;
		  pt.x =(rectWindow.left+rectWindow.right)/2;
		  pt.y = rectWindow.bottom - GetSystemMetrics(SM_CYFRAME) / 2;
		  break;
		case VK_LEFT:
		  hittest = HTLEFT;
		  pt.x = rectWindow.left + GetSystemMetrics(SM_CXFRAME) / 2;
		  pt.y =(rectWindow.top+rectWindow.bottom)/2;
		  break;
		case VK_RIGHT:
		  hittest = HTRIGHT;
		  pt.x = rectWindow.right - GetSystemMetrics(SM_CXFRAME) / 2;
		  pt.y =(rectWindow.top+rectWindow.bottom)/2;
		  break;
		case VK_RETURN:
		case VK_ESCAPE: return 0;
		}
	    }
	}
      *capturePoint = pt;
    }
    SetCursorPos( pt.x, pt.y );
    DefWndHandleSetCursor(hWnd, (WPARAM)hWnd, MAKELONG(hittest, WM_MOUSEMOVE));
    return hittest;
}

#define ON_LEFT_BORDER(hit) \
 (((hit) == HTLEFT) || ((hit) == HTTOPLEFT) || ((hit) == HTBOTTOMLEFT))
#define ON_RIGHT_BORDER(hit) \
 (((hit) == HTRIGHT) || ((hit) == HTTOPRIGHT) || ((hit) == HTBOTTOMRIGHT))
#define ON_TOP_BORDER(hit) \
 (((hit) == HTTOP) || ((hit) == HTTOPLEFT) || ((hit) == HTTOPRIGHT))
#define ON_BOTTOM_BORDER(hit) \
 (((hit) == HTBOTTOM) || ((hit) == HTBOTTOMLEFT) || ((hit) == HTBOTTOMRIGHT))

VOID STATIC 
UserDrawWindowFrame(HDC hdc, const RECT *rect,
		    ULONG width, ULONG height, DWORD rop )
{
  HBRUSH hbrush = SelectObject( hdc, GetStockObject( GRAY_BRUSH ) );
  PatBlt( hdc, rect->left, rect->top,
	  rect->right - rect->left - width, height, rop );
  PatBlt( hdc, rect->left, rect->top + height, width,
	  rect->bottom - rect->top - height, rop );
  PatBlt( hdc, rect->left + width, rect->bottom - 1,
	  rect->right - rect->left - width, -height, rop );
  PatBlt( hdc, rect->right - 1, rect->top, -width,
	  rect->bottom - rect->top - height, rop );
  SelectObject( hdc, hbrush );
}

VOID STATIC
UserDrawMovingFrame(HDC hdc, RECT *rect, BOOL thickframe)
{
  if (thickframe)
    {
      UserDrawWindowFrame(hdc, rect, GetSystemMetrics(SM_CXFRAME),
			  GetSystemMetrics(SM_CYFRAME), PATINVERT );
    }
  else DrawFocusRect( hdc, rect );
}

VOID STATIC
DefWndDoSizeMove(HWND hwnd, WORD wParam)
{
  MSG msg;
  RECT sizingRect, mouseRect, origRect;
  HDC hdc;
  LONG hittest = (LONG)(wParam & 0x0f);
  HCURSOR hDragCursor = 0, hOldCursor = 0;
  POINT minTrack, maxTrack;
  POINT capturePoint, pt;
  ULONG Style = GetWindowLongW(hwnd, GWL_STYLE);
  ULONG ExStyle = GetWindowLongW(hwnd, GWL_EXSTYLE); 
  BOOL thickframe = UserHasThickFrameStyle(Style, ExStyle);
  BOOL iconic = Style & WS_MINIMIZE;
  BOOL moved = FALSE;
  DWORD dwPoint = GetMessagePos();
  BOOL DragFullWindows = FALSE;
  HWND hWndParent;

  SystemParametersInfoA(SPI_GETDRAGFULLWINDOWS, 0, &DragFullWindows, 0);
  
  pt.x = SLOWORD(dwPoint);
  pt.y = SHIWORD(dwPoint);
  capturePoint = pt;
  
  if (IsZoomed(hwnd) || !IsWindowVisible(hwnd))
    {
      return;
    }
  
  if ((wParam & 0xfff0) == SC_MOVE)
    {
      if (!hittest) 
	{
	  hittest = DefWndStartSizeMove(hwnd, wParam, &capturePoint);
	}
      if (!hittest)
	{
	  return;
	}
    }
  else  /* SC_SIZE */
    {
      if (!thickframe)
	{
	  return;
	}
      if (hittest && hittest != HTSYSMENU) 
	{
	  hittest += 2;
	}
      else
	{
	  SetCapture(hwnd);
	  hittest = DefWndStartSizeMove(hwnd, wParam, &capturePoint);
	  if (!hittest)
	    {
	      ReleaseCapture();
	      return;
	    }
	}
    }

  if (Style & WS_CHILD)
    {
      hWndParent = GetParent(hwnd);
    }
  
  /* Get min/max info */
  
  WinPosGetMinMaxInfo(hwnd, NULL, NULL, &minTrack, &maxTrack);
  GetWindowRect(hwnd, &sizingRect);
  origRect = sizingRect;
  if (Style & WS_CHILD)
    {
      GetClientRect(hWndParent, &mouseRect );
    }
  else 
    {
      SetRect(&mouseRect, 0, 0, GetSystemMetrics(SM_CXSCREEN), 
	      GetSystemMetrics(SM_CYSCREEN));
    }
  if (ON_LEFT_BORDER(hittest))
    {
      mouseRect.left  = max( mouseRect.left, sizingRect.right-maxTrack.x );
      mouseRect.right = min( mouseRect.right, sizingRect.right-minTrack.x );
    }
  else if (ON_RIGHT_BORDER(hittest))
    {
      mouseRect.left  = max( mouseRect.left, sizingRect.left+minTrack.x );
      mouseRect.right = min( mouseRect.right, sizingRect.left+maxTrack.x );
    }
  if (ON_TOP_BORDER(hittest))
    {
      mouseRect.top    = max( mouseRect.top, sizingRect.bottom-maxTrack.y );
      mouseRect.bottom = min( mouseRect.bottom,sizingRect.bottom-minTrack.y);
    }
  else if (ON_BOTTOM_BORDER(hittest))
    {
      mouseRect.top    = max( mouseRect.top, sizingRect.top+minTrack.y );
      mouseRect.bottom = min( mouseRect.bottom, sizingRect.top+maxTrack.y );
    }
  if (Style & WS_CHILD)
    {      
      MapWindowPoints( hWndParent, 0, (LPPOINT)&mouseRect, 2 );
    }
  SendMessageA( hwnd, WM_ENTERSIZEMOVE, 0, 0 );
  
  if (GetCapture() != hwnd) SetCapture( hwnd );    
  
  if (Style & WS_CHILD)
    {
      /* Retrieve a default cache DC (without using the window style) */
      hdc = GetDCEx(hWndParent, 0, DCX_CACHE);
    }
  else
    {
      hdc = GetDC( 0 );
    }
  
  if( iconic ) /* create a cursor for dragging */
    {
      HICON hIcon = (HICON)GetClassLongW(hwnd, GCL_HICON);
      if(!hIcon) hIcon = (HICON)SendMessageW( hwnd, WM_QUERYDRAGICON, 0, 0L);
      if( hIcon ) hDragCursor = CursorIconToCursor( hIcon, TRUE );
      if( !hDragCursor ) iconic = FALSE;
    }
  
  /* invert frame if WIN31_LOOK to indicate mouse click on caption */
  if( !iconic && !DragFullWindows)
    {
      UserDrawMovingFrame( hdc, &sizingRect, thickframe );
    }
  
  while(1)
    {
      int dx = 0, dy = 0;

      GetMessageW(&msg, 0, 0, 0);
      
      /* Exit on button-up, Return, or Esc */
      if ((msg.message == WM_LBUTTONUP) ||
	  ((msg.message == WM_KEYDOWN) && 
	   ((msg.wParam == VK_RETURN) || (msg.wParam == VK_ESCAPE)))) break;
      
      if (msg.message == WM_PAINT)
        {
	  if(!iconic && !DragFullWindows) UserDrawMovingFrame( hdc, &sizingRect, thickframe );
	  UpdateWindow( msg.hwnd );
	  if(!iconic && !DragFullWindows) UserDrawMovingFrame( hdc, &sizingRect, thickframe );
	  continue;
        }
      
      if ((msg.message != WM_KEYDOWN) && (msg.message != WM_MOUSEMOVE))
	continue;  /* We are not interested in other messages */
      
      pt = msg.pt;
      
      if (msg.message == WM_KEYDOWN) switch(msg.wParam)
	{
	case VK_UP:    pt.y -= 8; break;
	case VK_DOWN:  pt.y += 8; break;
	case VK_LEFT:  pt.x -= 8; break;
	case VK_RIGHT: pt.x += 8; break;		
	}
      
      pt.x = max( pt.x, mouseRect.left );
      pt.x = min( pt.x, mouseRect.right );
      pt.y = max( pt.y, mouseRect.top );
      pt.y = min( pt.y, mouseRect.bottom );
      
      dx = pt.x - capturePoint.x;
      dy = pt.y - capturePoint.y;
      
      if (dx || dy)
	{
	  if( !moved )
	    {
	      moved = TRUE;
	      
		if( iconic ) /* ok, no system popup tracking */
		  {
		    hOldCursor = SetCursor(hDragCursor);
		    ShowCursor( TRUE );
		    WinPosShowIconTitle( hwnd, FALSE );
		  } 
	    }
	  
	  if (msg.message == WM_KEYDOWN) SetCursorPos( pt.x, pt.y );
	  else
	    {
	      RECT newRect = sizingRect;
	      WPARAM wpSizingHit = 0;
	      
	      if (hittest == HTCAPTION) OffsetRect( &newRect, dx, dy );
	      if (ON_LEFT_BORDER(hittest)) newRect.left += dx;
	      else if (ON_RIGHT_BORDER(hittest)) newRect.right += dx;
	      if (ON_TOP_BORDER(hittest)) newRect.top += dy;
	      else if (ON_BOTTOM_BORDER(hittest)) newRect.bottom += dy;
	      if(!iconic && !DragFullWindows) UserDrawMovingFrame( hdc, &sizingRect, thickframe );
	      capturePoint = pt;
	      
	      /* determine the hit location */
	      if (hittest >= HTLEFT && hittest <= HTBOTTOMRIGHT)
		wpSizingHit = WMSZ_LEFT + (hittest - HTLEFT);
	      SendMessageA( hwnd, WM_SIZING, wpSizingHit, (LPARAM)&newRect );
	      
	      if (!iconic)
		{
		  if(!DragFullWindows)
		    UserDrawMovingFrame( hdc, &newRect, thickframe );
		  else {
		    /* To avoid any deadlocks, all the locks on the windows
		       structures must be suspended before the SetWindowPos */
		    SetWindowPos( hwnd, 0, newRect.left, newRect.top,
				  newRect.right - newRect.left,
				  newRect.bottom - newRect.top,
				  ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );
		  }
		}
	      sizingRect = newRect;
	    }
	}
    }
  
  ReleaseCapture();
  if( iconic )
    {
      if( moved ) /* restore cursors, show icon title later on */
	{
	  ShowCursor( FALSE );
	  SetCursor( hOldCursor );
	}
      DestroyCursor( hDragCursor );
    }
  else if(!DragFullWindows)
      UserDrawMovingFrame( hdc, &sizingRect, thickframe );
  
  if (Style & WS_CHILD)
    ReleaseDC( hWndParent, hdc );
  else
    ReleaseDC( 0, hdc );
  
  SendMessageA( hwnd, WM_EXITSIZEMOVE, 0, 0 );
  SendMessageA( hwnd, WM_SETVISIBLE, !IsIconic(hwnd), 0L);
  
  /* window moved or resized */
  if (moved)
    {
      /* if the moving/resizing isn't canceled call SetWindowPos
       * with the new position or the new size of the window
       */
      if (!((msg.message == WM_KEYDOWN) && (msg.wParam == VK_ESCAPE)) )
        {
	  /* NOTE: SWP_NOACTIVATE prevents document window activation in Word 6 */
	  if(!DragFullWindows)
	    SetWindowPos( hwnd, 0, sizingRect.left, sizingRect.top,
			  sizingRect.right - sizingRect.left,
			  sizingRect.bottom - sizingRect.top,
			  ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );
        }
      else { /* restore previous size/position */
	if(DragFullWindows)
	  SetWindowPos( hwnd, 0, origRect.left, origRect.top,
			origRect.right - origRect.left,
			origRect.bottom - origRect.top,
			( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );
      }      
    }
  
  if( IsWindow(hwnd) )
    if( Style & WS_MINIMIZE )
      {
	/* Single click brings up the system menu when iconized */
	
	if( !moved ) 
	  {
	    if( Style & WS_SYSMENU ) 
	      SendMessageA( hwnd, WM_SYSCOMMAND,
			    SC_MOUSEMENU + HTSYSMENU, MAKELONG(pt.x,pt.y));
	  }
	else WinPosShowIconTitle( hwnd, TRUE );
      }
}


LRESULT
DefWndHandleSysCommand(HWND hWnd, WPARAM wParam, POINT Pt)
{
  switch (wParam & 0xfff0)
    {
      case SC_MOVE:
      case SC_SIZE:
	DefWndDoSizeMove(hWnd, wParam);
	break;
      case SC_CLOSE:
        SendMessageA(hWnd, WM_CLOSE, 0, 0);
        break;
      case SC_MOUSEMENU:
        MenuTrackMouseMenuBar(hWnd, wParam, Pt);
	break;
      case SC_KEYMENU:
        MenuTrackKbdMenuBar(hWnd, wParam, Pt.x);
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

  if (!(GetWindowLongW(hWnd, GWL_STYLE) & WS_MINIMIZE))
    {
      DefWndAdjustRect(&TmpRect, GetWindowLongW(hWnd, GWL_STYLE),
		       FALSE, GetWindowLongW(hWnd, GWL_EXSTYLE));
      Rect->left -= TmpRect.left;
      Rect->top -= TmpRect.top;
      Rect->right -= TmpRect.right;
      Rect->bottom -= TmpRect.bottom;
      if (UserHasMenu(hWnd, GetWindowLongW(hWnd, GWL_EXSTYLE)))
	{
	  Rect->top += MenuGetMenuBarHeight(hWnd, 
					    Rect->right - Rect->left,
					    -TmpRect.left,
					    -TmpRect.top) + 1;
	}
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
	if (GetWindowLongW(hWnd, GWL_STYLE) & WS_CHILD)
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

	    if (GetWindowLongW(hWnd, GWL_STYLE) & WS_CHILD)
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
	    if (GetWindowLongW(hWnd, GWL_EXSTYLE) & WS_EX_CLIENTEDGE)
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
	if (GetWindowLongW(hWnd, GWL_STYLE) & WS_CHILD)
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
	    !(GetWindowLongW(hWnd, GWL_STYLE) & WS_MINIMIZE))
	  {
	    SetFocus(hWnd);
	  }
	break;
      }

    case WM_MOUSEWHEEL:
      {
	if (GetWindowLongW(hWnd, GWL_STYLE & WS_CHILD))
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
	if (NULL == hBrush)
	  {
	    return 0;
	  }
#if 0
	if (((DWORD) hBrush) <= 25)
	  {
	    hBrush = GetSysColorBrush((DWORD) hBrush - 1);
	  }
#endif
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
	if (GetWindowLongW(hWnd, GWL_STYLE) & WS_CHILD)
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
	    if ((hIcon = LoadIconW(NULL, MAKEINTRESOURCEW(Len))) != NULL)
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
	      (LPWSTR)(DWORD)GlobalAddAtomW(L"USER32!WindowTextAtomW");
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
