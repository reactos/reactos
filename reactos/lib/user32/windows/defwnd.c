/* $Id: defwnd.c,v 1.1 2002/01/14 01:11:58 dwelch Exp $
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
#include <debug.h>
#include <user32/wininternal.h>

/* GLOBALS *******************************************************************/

static HBITMAP hbitmapClose;
static HBITMAP hbitmapMinimize;
static HBITMAP hbitmapMinimizeD;
static HBITMAP hbitmapMaximize;
static HBITMAP hbitmapMaximizeD;
static HBITMAP hbitmapRestore;
static HBITMAP hbitmapRestoreD;

/* FUNCTIONS *****************************************************************/

BOOL 
UserRedrawIconTitle(HWND hWnd)
{
#if 0
  LPINTERNALPOS lpPos = (LPINTERNALPOS)GetPropA(hWnd, atomInternalPos);
  if (lpPos)
    {
      if (lpPos->hwndIconTitle)
	{
	  SendMessageA(lpPos->hwndIconTitle, WM_SHOWWINDOW, TRUE, 0);
	  InvalidateRect(lpPos->hwndIconTitle, NULL, TRUE);
	  return(TRUE);
	}
    }
  return(FALSE);
#endif
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

static void UserGetInsideRectNC( HWND hwnd, RECT *rect )
{
  RECT WindowRect;
  ULONG Style;
  ULONG ExStyle;

  Style = GetWindowLong(hwnd, GWL_STYLE);
  ExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
  GetWindowRect(hwnd, &WindowRect);
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
		InflateRect( rect, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER) );
	      }
	  }
      }
}

void UserDrawSysButton( HWND hwnd, HDC hdc, BOOL down )
{
    RECT rect;
    HDC hdcMem;
    HBITMAP hbitmap;
    ULONG Style;

    Style = GetWindowLong(hwnd, GWL_STYLE);
    UserGetInsideRectNC( hwnd, &rect );
    hdcMem = CreateCompatibleDC( hdc );
    hbitmap = SelectObject( hdcMem, hbitmapClose );
    BitBlt(hdc, rect.left, rect.top, GetSystemMetrics(SM_CXSIZE), GetSystemMetrics(SM_CYSIZE),
	   hdcMem, (Style & WS_CHILD) ? GetSystemMetrics(SM_CXSIZE) : 0, 0,
	   down ? NOTSRCCOPY : SRCCOPY );
    SelectObject( hdcMem, hbitmap );
    DeleteDC( hdcMem );
}

static void UserDrawMaxButton( HWND hwnd, HDC hdc, BOOL down )
{
    RECT rect;
    HDC hdcMem;

    UserGetInsideRectNC( hwnd, &rect );
    hdcMem = CreateCompatibleDC( hdc );
    SelectObject( hdcMem,  (IsZoomed(hwnd) 
			    ? (down ? hbitmapRestoreD : hbitmapRestore)
			    : (down ? hbitmapMaximizeD : hbitmapMaximize)) );
    BitBlt( hdc, rect.right - GetSystemMetrics(SM_CXSIZE) - 1, rect.top,
	    GetSystemMetrics(SM_CXSIZE) + 1, GetSystemMetrics(SM_CYSIZE), hdcMem, 0, 0,
	    SRCCOPY );
    DeleteDC( hdcMem );
}

static void UserDrawMinButton( HWND hwnd, HDC hdc, BOOL down)
{
  RECT rect;
  HDC hdcMem;
    
  UserGetInsideRectNC(hwnd, &rect);
  hdcMem = CreateCompatibleDC(hdc);
  SelectObject(hdcMem, (down ? hbitmapMinimizeD : hbitmapMinimize));
  if (GetWindowLong(hwnd, GWL_STYLE) & WS_MAXIMIZEBOX) 
    {
      rect.right -= GetSystemMetrics(SM_CXSIZE)+1;
    }
  BitBlt( hdc, rect.right - GetSystemMetrics(SM_CXSIZE) - 1, rect.top,
	  GetSystemMetrics(SM_CXSIZE) + 1, GetSystemMetrics(SM_CYSIZE), 
	  hdcMem, 0, 0,
	  SRCCOPY );
  DeleteDC( hdcMem );
}

static void UserDrawCaptionNC( HDC hdc, RECT *rect, HWND hwnd,
			    DWORD style, BOOL active )
{
  RECT r = *rect;
  char buffer[256];

  if (!hbitmapClose)
    {
	if (!(hbitmapClose = LoadBitmapW( 0, MAKEINTRESOURCE(OBM_CLOSE) )))
        {    
	    return;
        }
	hbitmapMinimize  = LoadBitmapW( 0, MAKEINTRESOURCE(OBM_REDUCE) );
	hbitmapMinimizeD = LoadBitmapW( 0, MAKEINTRESOURCE(OBM_REDUCED) );
	hbitmapMaximize  = LoadBitmapW( 0, MAKEINTRESOURCE(OBM_ZOOM) );
	hbitmapMaximizeD = LoadBitmapW( 0, MAKEINTRESOURCE(OBM_ZOOMD) );
	hbitmapRestore   = LoadBitmapW( 0, MAKEINTRESOURCE(OBM_RESTORE) );
	hbitmapRestoreD  = LoadBitmapW( 0, MAKEINTRESOURCE(OBM_RESTORED) );
    }
    
  if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_DLGMODALFRAME)
    {
      HBRUSH hbrushOld = SelectObject(hdc, GetSysColorBrush(COLOR_WINDOW) );
      PatBlt( hdc, r.left, r.top, 1, r.bottom-r.top+1,PATCOPY );
      PatBlt( hdc, r.right-1, r.top, 1, r.bottom-r.top+1, PATCOPY );
      PatBlt( hdc, r.left, r.top-1, r.right-r.left, 1, PATCOPY );
      r.left++;
      r.right--;
      SelectObject( hdc, hbrushOld );
    }

  MoveToEx( hdc, r.left, r.bottom, NULL );
  LineTo( hdc, r.right, r.bottom );

  if (style & WS_SYSMENU)
    {
      UserDrawSysButton( hwnd, hdc, FALSE );
      r.left += GetSystemMetrics(SM_CXSIZE) + 1;
      MoveToEx( hdc, r.left - 1, r.top, NULL );
      LineTo( hdc, r.left - 1, r.bottom );
    }
  if (style & WS_MAXIMIZEBOX)
    {
      UserDrawMaxButton( hwnd, hdc, FALSE );
      r.right -= GetSystemMetrics(SM_CXSIZE) + 1;
    }
  if (style & WS_MINIMIZEBOX)
    {
      UserDrawMinButton( hwnd, hdc, FALSE );
      r.right -= GetSystemMetrics(SM_CXSIZE) + 1;
    }
  
  FillRect( hdc, &r, GetSysColorBrush(active ? COLOR_ACTIVECAPTION :
					    COLOR_INACTIVECAPTION) );
  
  if (GetWindowTextA( hwnd, buffer, sizeof(buffer) ))
    {
      if (active) SetTextColor( hdc, GetSysColor( COLOR_CAPTIONTEXT ) );
      else SetTextColor( hdc, GetSysColor( COLOR_INACTIVECAPTIONTEXT ) );
      SetBkMode( hdc, TRANSPARENT );
      DrawTextA( hdc, buffer, -1, &r,
		 DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX );
    }
}

VOID
UserDrawFrameNC(HDC hdc, RECT* rect, BOOL dlgFrame, BOOL active)
{
  INT width, height;
  
  if (dlgFrame)
    {
      width = GetSystemMetrics(SM_CXDLGFRAME) - 1;
      height = GetSystemMetrics(SM_CYDLGFRAME) - 1;
      SelectObject( hdc, GetSysColorBrush(active ? COLOR_ACTIVECAPTION :
					  COLOR_INACTIVECAPTION) );
    }
  else
    {
      width = GetSystemMetrics(SM_CXFRAME) - 2;
      height = GetSystemMetrics(SM_CYFRAME) - 2;
      SelectObject( hdc, GetSysColorBrush(active ? COLOR_ACTIVEBORDER :
					    COLOR_INACTIVEBORDER) );
    }
  
  /* Draw frame */
  PatBlt( hdc, rect->left, rect->top,
	  rect->right - rect->left, height, PATCOPY );
  PatBlt( hdc, rect->left, rect->top,
	  width, rect->bottom - rect->top, PATCOPY );
  PatBlt( hdc, rect->left, rect->bottom - 1,
	  rect->right - rect->left, -height, PATCOPY );
  PatBlt( hdc, rect->right - 1, rect->top,
	  -width, rect->bottom - rect->top, PATCOPY );

  if (dlgFrame)
    {
      InflateRect( rect, -width, -height );
    } 
  else
    {
      INT decYOff = GetSystemMetrics(SM_CXFRAME) + 
	GetSystemMetrics(SM_CXSIZE) - 1;
      INT decXOff = GetSystemMetrics(SM_CYFRAME) + 
	GetSystemMetrics(SM_CYSIZE) - 1;

      /* Draw inner rectangle */
      
      SelectObject( hdc, GetStockObject(NULL_BRUSH) );
      Rectangle( hdc, rect->left + width, rect->top + height,
		 rect->right - width , rect->bottom - height );
      
      /* Draw the decorations */
      
      MoveToEx( hdc, rect->left, rect->top + decYOff, NULL );
      LineTo( hdc, rect->left + width, rect->top + decYOff );
      MoveToEx( hdc, rect->right - 1, rect->top + decYOff, NULL );
      LineTo( hdc, rect->right - width - 1, rect->top + decYOff );
      MoveToEx( hdc, rect->left, rect->bottom - decYOff, NULL );
      LineTo( hdc, rect->left + width, rect->bottom - decYOff );
      MoveToEx( hdc, rect->right - 1, rect->bottom - decYOff, NULL );
      LineTo( hdc, rect->right - width - 1, rect->bottom - decYOff );
      
      MoveToEx( hdc, rect->left + decXOff, rect->top, NULL );
      LineTo( hdc, rect->left + decXOff, rect->top + height);
      MoveToEx( hdc, rect->left + decXOff, rect->bottom - 1, NULL );
      LineTo( hdc, rect->left + decXOff, rect->bottom - height - 1 );
      MoveToEx( hdc, rect->right - decXOff, rect->top, NULL );
      LineTo( hdc, rect->right - decXOff, rect->top + height );
      MoveToEx( hdc, rect->right - decXOff, rect->bottom - 1, NULL );
      LineTo( hdc, rect->right - decXOff, rect->bottom - height - 1 );
      
      InflateRect( rect, -width - 1, -height - 1 );
    }
}

VOID
UserDoPaintNC(HWND hWnd, HRGN clip)
{
  ULONG Active;
  HDC hDc;
  RECT rect;
  ULONG Style;
  ULONG ExStyle;

  Active = GetWindowLongW(hWnd, GWL_STYLE) & WIN_NCACTIVATED;
  Style = GetWindowLong(hWnd, GWL_STYLE);
  ExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

  hDc = GetDCEx(hWnd, (clip > 1) ? clip : 0, DCX_USESTYLE | DCX_WINDOW |
		((clip > 1) ? (DCX_INTERSECTRGN | DCX_KEEPCLIPRGN) : 0));
  if (hDc == 0)
    {
      return;
    }

  /* FIXME: Test whether we need to draw anything at all. */

  GetWindowRect(hWnd, &rect);
  rect.right = rect.right - rect.left;
  rect.bottom = rect.bottom - rect.top;
  rect.top = rect.left = 0;

  SelectObject(hDc, GetSysColorPen(COLOR_WINDOWFRAME));
  if (UserHasAnyFrameStyle(Style, ExStyle))
    {
      SelectObject(hDc, GetStockObject(NULL_BRUSH));
      Rectangle(hDc, 0, 0, rect.right, rect.bottom);
      InflateRect(&rect, -1, -1);
    }
  
  if (UserHasThickFrameStyle(Style, ExStyle))
    {
      UserDrawFrameNC(hDc, &rect, FALSE, Active);
    }
  else if (UserHasDlgFrameStyle(Style, ExStyle))
    {
      UserDrawFrameNC(hDc, &rect, TRUE, Active);
    }
  
  if (Style & WS_CAPTION)
    {
      RECT r = rect;
      r.bottom = rect.top + GetSystemMetrics(SM_CYSIZE);
      rect.top += GetSystemMetrics(SM_CYSIZE) + 
	GetSystemMetrics(SM_CYBORDER);
      UserDrawCaptionNC(hDc, &r, hWnd, Style, Active);
    }

  /* FIXME: Draw menu bar. */

  /* FIXME: Draw scroll bars. */

  /* FIXME: Draw size box. */
  
  ReleaseDC(hWnd, hDc);
}

LRESULT
UserPaintNC(HWND hWnd, HRGN clip)
{
  if (IsWindowVisible(hWnd))
    {
      if (IsIconic(hWnd))
	{
	  UserRedrawIconTitle(hWnd);
	}
      else
	{
	  UserDoPaintNC(hWnd, clip);
	}
    }
  return(0);
}

LRESULT
UserHitTestNC(HWND hWnd, POINT Point)
{
}

LRESULT
UserHandleLButtonDownNC(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
}

LRESULT
UserHandleLButtonDblClkNC(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
}

LRESULT
UserHandleActiveNC(HWND hWnd, WPARAM wParam)
{
}

VOID
UserSetRedrawDefWnd(HWND hWnd, WPARAM wParam)
{
}

LRESULT STDCALL
User32DefWindowProc(HWND hWnd,
		    UINT Msg,
		    WPARAM wParam,
		    LPARAM lParam)
{
  switch (Msg)
    {
    case WM_NCPAINT:
      {
	return(UserPaintNC(hWnd, (HRGN)wParam));
      }

    case WM_NCHITTEST:
      {
	POINT Point;
	Point.x = SLOWORD(lParam);
	Point.y = SHIWORD(lParam);
	return(UserHitTestNC(hWnd, Point));
      }

    case WM_NCLBUTTONDOWN:
      {
	return(UserHandleLButtonDownNC(hWnd, wParam, lParam));
      }

    case WM_LBUTTONDBLCLK:
    case WM_NCLBUTTONDBLCLK:
      {
	return(UserHandleLButtonDblClkNC(hWnd, wParam, lParam));
      }

    case WM_NCRBUTTONDOWN:
      {
	if (wParam == HTCAPTION)
	  {
	    SetCapture(hWnd);
	  }
	break;
      }

    case WM_RBUTTONUP:
      {
	if (hWnd == GetCapture())
	  {
	    ReleaseCapture();
	  }
	break;
      }

    case WM_NCRBUTTONUP:
      {
	break;
      }

    case WM_CONTEXTMENU:
      {
	break;
      }

    case WM_NCACTIVATE:
      {
	return(UserHandleActiveNC(hWnd, wParam));
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
	/* FIXME: Paint the icon if minimized otherwise just paint nothing. */
	return(0);
      }
#if 0
    case WM_SYNCPAINT:
      {
	return(0);
      }
#endif
    case WM_SETREDRAW:
      {
	UserSetRedrawDefWnd(hWnd, wParam);
	return(0);
      }

    case WM_CLOSE:
      {
	DestroyWindow(hWnd);
	return(0);
      }

    case WM_MOUSEACTIVATE:
      {
	/* FIXME: Send to parent if child. */
	return((LOWORD(lParam) >= HTCLIENT) ? MA_ACTIVATE : MA_NOACTIVATE);
      }

    case WM_ACTIVATE:
      {
	if (LOWORD(lParam) != WA_INACTIVE)
	  {
	    /* Check if the window is minimized. */
	    SetFocus(hWnd);
	  }
	break;
      }
#if 0
    case WM_MOUSEWHEEL:
      {
	return(0);
      }
#endif
    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
      {
	RECT rect;
	HBRUSH hbr = (HBRUSH)GetClassLongW(hWnd, GCL_HBRBACKGROUND);
	if (!hbr) return 0;
	
	/*  Since WM_ERASEBKGND may receive either a window dc or a    */ 
	/*  client dc, the area to be erased has to be retrieved from  */
	/*  the device context.      				   */
	GetClipBox( (HDC)wParam, &rect );
	
	/* Always call the Win32 variant of FillRect even on Win16,
	 * since despite the fact that Win16, as well as Win32,
	 * supports special background brushes for a window class,
	 * the Win16 variant of FillRect does not.
	 */
	FillRect( (HDC) wParam, &rect, hbr );
	return 1;
      }
#if 0
    case WM_GETDLGCODE:
	return 0;

    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORSCROLLBAR:
	return (LRESULT)DEFWND_ControlColor( (HDC)wParam, msg - WM_CTLCOLORMSGBOX );

    case WM_CTLCOLOR:
	return (LRESULT)DEFWND_ControlColor( (HDC)wParam, HIWORD(lParam) );
	
    case WM_SETCURSOR:
	if (wndPtr->dwStyle & WS_CHILD)
	{
            /* with the exception of the border around a resizable wnd,
             * give the parent first chance to set the cursor */
            if ((LOWORD(lParam) < HTSIZEFIRST) || (LOWORD(lParam) > HTSIZELAST))
            {
                if (pSendMessage(wndPtr->parent->hwndSelf, WM_SETCURSOR, wParam, lParam))
                    return TRUE;
            }
        }
	return NC_HandleSetCursor( wndPtr->hwndSelf, wParam, lParam );

    case WM_SYSCOMMAND:
        {
            POINT pt;
            pt.x = SLOWORD(lParam);
            pt.y = SHIWORD(lParam);
            return NC_HandleSysCommand( wndPtr->hwndSelf, wParam, pt );
        }

    case WM_KEYDOWN:
	if(wParam == VK_F10) iF10Key = VK_F10;
	break;

    case WM_SYSKEYDOWN:
	if( HIWORD(lParam) & KEYDATA_ALT )
	{
	    /* if( HIWORD(lParam) & ~KEYDATA_PREVSTATE ) */
	      if( wParam == VK_MENU && !iMenuSysKey )
		iMenuSysKey = 1;
	      else
		iMenuSysKey = 0;
	    
	    iF10Key = 0;

	    if( wParam == VK_F4 )	/* try to close the window */
	    {
		HWND hWnd = WIN_GetTopParent( wndPtr->hwndSelf );
		wndPtr = WIN_FindWndPtr( hWnd );
		if( wndPtr && !(wndPtr->clsStyle & CS_NOCLOSE) )
		    pPostMessage( hWnd, WM_SYSCOMMAND, SC_CLOSE, 0 );
                WIN_ReleaseWndPtr(wndPtr);
	    }
	} 
	else if( wParam == VK_F10 )
	        iF10Key = 1;
	     else
	        if( wParam == VK_ESCAPE && (GetKeyState(VK_SHIFT) & 0x8000))
		    pSendMessage( wndPtr->hwndSelf, WM_SYSCOMMAND, SC_KEYMENU, VK_SPACE );
	break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
	/* Press and release F10 or ALT */
	if (((wParam == VK_MENU) && iMenuSysKey) ||
            ((wParam == VK_F10) && iF10Key))
	      pSendMessage( WIN_GetTopParent(wndPtr->hwndSelf),
                             WM_SYSCOMMAND, SC_KEYMENU, 0L );
	iMenuSysKey = iF10Key = 0;
        break;

    case WM_SYSCHAR:
	iMenuSysKey = 0;
	if (wParam == VK_RETURN && (wndPtr->dwStyle & WS_MINIMIZE))
        {
	    pPostMessage( wndPtr->hwndSelf, WM_SYSCOMMAND, SC_RESTORE, 0L );
	    break;
        } 
	if ((HIWORD(lParam) & KEYDATA_ALT) && wParam)
        {
	    if (wParam == VK_TAB || wParam == VK_ESCAPE) break;
	    if (wParam == VK_SPACE && (wndPtr->dwStyle & WS_CHILD))
		pSendMessage( wndPtr->parent->hwndSelf, msg, wParam, lParam );
	    else
		pSendMessage( wndPtr->hwndSelf, WM_SYSCOMMAND, SC_KEYMENU, wParam );
        } 
	else /* check for Ctrl-Esc */
            if (wParam != VK_ESCAPE) MessageBeep(0);
	break;

    case WM_SHOWWINDOW:
        if (!lParam) return 0; /* sent from ShowWindow */
        if (!(wndPtr->dwStyle & WS_POPUP) || !wndPtr->owner) return 0;
        if ((wndPtr->dwStyle & WS_VISIBLE) && wParam) return 0;
	else if (!(wndPtr->dwStyle & WS_VISIBLE) && !wParam) return 0;
        ShowWindow( wndPtr->hwndSelf, wParam ? SW_SHOWNOACTIVATE : SW_HIDE );
	break; 

    case WM_CANCELMODE:
	if (wndPtr->parent == WIN_GetDesktop()) EndMenu();
	if (GetCapture() == wndPtr->hwndSelf) ReleaseCapture();
        WIN_ReleaseDesktop();
	break;

    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
	return -1;

    case WM_DROPOBJECT:
	return DRAG_FILE;  

    case WM_QUERYDROPOBJECT:
	if (wndPtr->dwExStyle & WS_EX_ACCEPTFILES) return 1;
	break;

    case WM_QUERYDRAGICON:
        {
            UINT len;

            HICON hIcon = GetClassLongW( wndPtr->hwndSelf, GCL_HICON );
            if (hIcon) return hIcon;
            for(len=1; len<64; len++)
                if((hIcon = LoadIconW(wndPtr->hInstance, MAKEINTRESOURCEW(len))))
                    return (LRESULT)hIcon;
            return (LRESULT)LoadIconW(0, IDI_APPLICATIONW);
        }
        break;

    case WM_ISACTIVEICON:
	return ((wndPtr->flags & WIN_NCACTIVATED) != 0);

    case WM_NOTIFYFORMAT:
      if (IsWindowUnicode(wndPtr->hwndSelf)) return NFR_UNICODE;
      else return NFR_ANSI;
        
    case WM_QUERYOPEN:
    case WM_QUERYENDSESSION:
	return 1;

    case WM_SETICON:
	{
		int index = (wParam != ICON_SMALL) ? GCL_HICON : GCL_HICONSM;
		HICON hOldIcon = GetClassLongW(wndPtr->hwndSelf, index); 
		SetClassLongW(wndPtr->hwndSelf, index, lParam);

		SetWindowPos(wndPtr->hwndSelf, 0, 0, 0, 0, 0, SWP_FRAMECHANGED
			 | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE
			 | SWP_NOZORDER);

		if( wndPtr->flags & WIN_NATIVE )
		    wndPtr->pDriver->pSetHostAttr(wndPtr, HAK_ICONS, 0);

		return hOldIcon;
	}

    case WM_GETICON:
	{
		int index = (wParam != ICON_SMALL) ? GCL_HICON : GCL_HICONSM;
		return GetClassLongW(wndPtr->hwndSelf, index); 
	}

    case WM_HELP:
	pSendMessage( wndPtr->parent->hwndSelf, msg, wParam, lParam );
	break;    
#endif
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

  switch (Msg)
    {
    case WM_NCCREATE:
      {
	
      }

    case WM_NCCALCSIZE:
      {
      }

    case WM_WINDOWPOSCHANGING:
      {
      }

    case WM_WINDOWPOSCHANGED:
      {
      }

    case WM_GETTEXTLENGTH:
      {
      }

    case WM_GETTEXT:
      {
      }

    case WM_SETTEXT:
      {
      }

    default:
      Result = User32DefWindowProc(hWnd, Msg, wParam, lParam);
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

  switch (Msg)
    {
    case WM_NCCREATE:
      {
	
      }

    case WM_NCCALCSIZE:
      {
      }

    case WM_WINDOWPOSCHANGING:
      {
      }

    case WM_WINDOWPOSCHANGED:
      {
      }

    case WM_GETTEXTLENGTH:
      {
      }

    case WM_GETTEXT:
      {
      }

    case WM_SETTEXT:
      {
      }

    default:
      Result = User32DefWindowProc(hWnd, Msg, wParam, lParam);
      break;
    }

  return(Result);
}
