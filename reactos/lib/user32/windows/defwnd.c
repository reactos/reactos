/* $Id: defwnd.c,v 1.121 2004/01/12 20:38:59 gvg Exp $
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

LRESULT DefWndNCPaint(HWND hWnd, HRGN hRgn);
LRESULT DefWndNCCalcSize(HWND hWnd, BOOL CalcSizeStruct, RECT *Rect);
LRESULT DefWndNCActivate(HWND hWnd, WPARAM wParam);
LRESULT DefWndNCHitTest(HWND hWnd, POINT Point);
LRESULT DefWndNCLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT DefWndNCLButtonDblClk(HWND hWnd, WPARAM wParam, LPARAM lParam);
VOID DefWndTrackScrollBar(HWND hWnd, WPARAM wParam, POINT Point);

/* GLOBALS *******************************************************************/

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


static COLORREF SysColors[] =
  {
    RGB(192, 192, 192) /* COLOR_SCROLLBAR */,
    RGB(58, 110, 165) /* COLOR_BACKGROUND */,
    RGB(10, 36, 106) /* COLOR_ACTIVECAPTION */,
    RGB(128, 128, 128) /* COLOR_INACTIVECAPTION */,
    RGB(192, 192, 192) /* COLOR_MENU */,
    RGB(255, 255, 255) /* COLOR_WINDOW */,
    RGB(0, 0, 0) /* COLOR_WINDOWFRAME */,
    RGB(0, 0, 0) /* COLOR_MENUTEXT */,
    RGB(0, 0, 0) /* COLOR_WINDOWTEXT */,
    RGB(255, 255, 255) /* COLOR_CAPTIONTEXT */,
    RGB(192, 192, 192) /* COLOR_ACTIVEBORDER */,
    RGB(192, 192, 192) /* COLOR_INACTIVEBORDER */,
    RGB(128, 128, 128) /* COLOR_APPWORKSPACE */,
    RGB(0, 0, 128) /* COLOR_HILIGHT */,
    RGB(255, 255, 255) /* COLOR_HILIGHTTEXT */,
    RGB(192, 192, 192) /* COLOR_BTNFACE */,
    RGB(128, 128, 128) /* COLOR_BTNSHADOW */,
    RGB(128, 128, 128) /* COLOR_GRAYTEXT */,
    RGB(0, 0, 0) /* COLOR_BTNTEXT */,
    RGB(192, 192, 192) /* COLOR_INACTIVECAPTIONTEXT */,
    RGB(255, 255, 255) /* COLOR_BTNHILIGHT */,
    RGB(32, 32, 32) /* COLOR_3DDKSHADOW */,
    RGB(192, 192, 192) /* COLOR_3DLIGHT */,
    RGB(0, 0, 0) /* COLOR_INFOTEXT */,
    RGB(255, 255, 192) /* COLOR_INFOBK */,
    RGB(180, 180, 180) /* COLOR_ALTERNATEBTNFACE */,
    RGB(0, 0, 255) /* COLOR_HOTLIGHT */,
    RGB(166, 202, 240) /* COLOR_GRADIENTACTIVECAPTION */,
    RGB(192, 192, 192) /* COLOR_GRADIENTINACTIVECAPTION */,
  };

#define NUM_SYSCOLORS (sizeof(SysColors) / sizeof(SysColors[0]))

/* Bits in the dwKeyData */
#define KEYDATA_ALT   0x2000

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
DWORD STDCALL
GetSysColor(int nIndex)
{
    return SysColors[nIndex];
}

/*
 * @implemented
 */
HPEN STDCALL
GetSysColorPen(int nIndex)
{
  static HPEN SysPens[NUM_SYSCOLORS];

  if (nIndex < 0 || NUM_SYSCOLORS < nIndex)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return NULL;
    }

  /* FIXME should register this object with DeleteObject() so it
     can't be deleted */
  if (NULL == SysPens[nIndex])
    {
      SysPens[nIndex] = CreatePen(PS_SOLID, 1, SysColors[nIndex]);
    }

  return SysPens[nIndex];
}

/*
 * @implemented
 */
HBRUSH STDCALL
GetSysColorBrush(int nIndex)
{
  static HBRUSH SysBrushes[NUM_SYSCOLORS];

  if (nIndex < 0 || NUM_SYSCOLORS < nIndex)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return NULL;
    }

  /* FIXME should register this object with DeleteObject() so it
     can't be deleted */
  if (NULL == SysBrushes[nIndex])
    {
      SysBrushes[nIndex] = (HBRUSH) ((DWORD) CreateSolidBrush(SysColors[nIndex]) | 0x00800000);
    }

  return SysBrushes[nIndex];
}

/*
 * @unimplemented
 */
/*
LRESULT STDCALL
DefFrameProcA( HWND hWnd,
	      HWND hWndMDIClient,
	      UINT uMsg,
	      WPARAM wParam,
	      LPARAM lParam )
{
    UNIMPLEMENTED;
    return ((LRESULT)0);
}
*/

/*
 * @unimplemented
 */
/*
LRESULT STDCALL
DefFrameProcW(HWND hWnd,
	      HWND hWndMDIClient,
	      UINT uMsg,
	      WPARAM wParam,
	      LPARAM lParam)
{
    UNIMPLEMENTED;
    return ((LRESULT)0);
}
*/

ULONG
UserHasAnyFrameStyle(ULONG Style, ULONG ExStyle)
{
    return ((Style & (WS_THICKFRAME | WS_DLGFRAME | WS_BORDER)) ||
            (ExStyle & WS_EX_DLGMODALFRAME) ||
            (!(Style & (WS_CHILD | WS_POPUP))));
}

ULONG
UserHasDlgFrameStyle(ULONG Style, ULONG ExStyle)
{
    return ((ExStyle & WS_EX_DLGMODALFRAME) ||
            ((Style & WS_DLGFRAME) && (!(Style & WS_THICKFRAME))));
}

ULONG
UserHasThickFrameStyle(ULONG Style, ULONG ExStyle)
{
    return ((Style & WS_THICKFRAME) &&
            (!((Style & (WS_DLGFRAME | WS_BORDER)) == WS_DLGFRAME)));
}

ULONG
UserHasThinFrameStyle(ULONG Style, ULONG ExStyle)
{
    return ((Style & WS_BORDER) || (!(Style & (WS_CHILD | WS_POPUP))));
}

ULONG
UserHasBigFrameStyle(ULONG Style, ULONG ExStyle)
{
    return ((Style & (WS_THICKFRAME | WS_DLGFRAME)) ||
            (ExStyle & WS_EX_DLGMODALFRAME));
}

void
UserGetInsideRectNC(HWND hWnd, RECT *rect)
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
        InflateRect(rect, -GetSystemMetrics(SM_CXFRAME),
                    -GetSystemMetrics(SM_CYFRAME));
    }
    else
    {
        if (UserHasDlgFrameStyle(Style, ExStyle ))
        {
            InflateRect(rect, -GetSystemMetrics(SM_CXDLGFRAME),
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
DefWndSetRedraw(HWND hWnd, WPARAM wParam)
{
  UNIMPLEMENTED;
}


LRESULT
DefWndHandleSetCursor(HWND hWnd, WPARAM wParam, LPARAM lParam, ULONG Style)
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
	    Msg == WM_RBUTTONDOWN || Msg == WM_XBUTTONDOWN)
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
        if (Style & WS_MAXIMIZE)
        {
          break;
        }
	return((LRESULT)SetCursor(LoadCursorW(0, IDC_SIZEWE)));
      }

    case HTTOP:
    case HTBOTTOM:
      {
        if (Style & WS_MAXIMIZE)
        {
          break;
        }
	return((LRESULT)SetCursor(LoadCursorW(0, IDC_SIZENS)));
      }

    case HTTOPLEFT:
    case HTBOTTOMRIGHT:
      {
        if (Style & WS_MAXIMIZE)
        {
          break;
        }
	return((LRESULT)SetCursor(LoadCursorW(0, IDC_SIZENWSE)));
      }

    case HTBOTTOMLEFT:
    case HTTOPRIGHT:
      {
        if (GetWindowLongW(hWnd, GWL_STYLE) & WS_MAXIMIZE)
        {
          break;
        }
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
	      hittest = DefWndNCHitTest(hWnd, msg.pt);
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
    DefWndHandleSetCursor(hWnd, (WPARAM)hWnd, MAKELONG(hittest, WM_MOUSEMOVE), Style);
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
			  GetSystemMetrics(SM_CYFRAME), DSTINVERT);
    }
  else DrawFocusRect( hdc, rect );
}

VOID STATIC
DefWndDoSizeMove(HWND hwnd, WORD wParam)
{
  MSG msg;
  RECT sizingRect, mouseRect, origRect, clipRect;
  HDC hdc;
  LONG hittest = (LONG)(wParam & 0x0f);
  HCURSOR hDragCursor = 0, hOldCursor = 0;
  POINT minTrack, maxTrack;
  POINT capturePoint, pt;
  ULONG Style = GetWindowLongW(hwnd, GWL_STYLE);
  ULONG ExStyle = GetWindowLongW(hwnd, GWL_EXSTYLE); 
  BOOL thickframe;
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
  
  thickframe = UserHasThickFrameStyle(Style, ExStyle) && !(Style & WS_MINIMIZE);
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
  if (Style & WS_CHILD)
    {
      MapWindowPoints( 0, hWndParent, (LPPOINT)&sizingRect, 2 );
      GetClientRect(hWndParent, &mouseRect );
      clipRect = mouseRect;
      MapWindowPoints(hWndParent, HWND_DESKTOP, (LPPOINT)&clipRect, 2);
    }
  else 
    {
      SetRect(&mouseRect, 0, 0, GetSystemMetrics(SM_CXSCREEN), 
	      GetSystemMetrics(SM_CYSCREEN));
      SystemParametersInfoW(SPI_GETWORKAREA, 0, &clipRect, 0);
    }
  ClipCursor(&clipRect);
  
  origRect = sizingRect;
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
  
  for(;;)
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
  ClipCursor(NULL);
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
      }
}


LRESULT
DefWndHandleSysCommand(HWND hWnd, WPARAM wParam, POINT Pt)
{
  WINDOWPLACEMENT wp;
  
  switch (wParam & 0xfff0)
    {
      case SC_MOVE:
      case SC_SIZE:
	DefWndDoSizeMove(hWnd, wParam);
	break;
      case SC_MINIMIZE:
        wp.length = sizeof(WINDOWPLACEMENT);
        if(GetWindowPlacement(hWnd, &wp))
        {
          wp.showCmd = SW_MINIMIZE;
          SetWindowPlacement(hWnd, &wp);
        }
        break;
      case SC_MAXIMIZE:
        wp.length = sizeof(WINDOWPLACEMENT);
        if(GetWindowPlacement(hWnd, &wp))
        {
          wp.showCmd = SW_MAXIMIZE;
          SetWindowPlacement(hWnd, &wp);
        }
        break;
      case SC_RESTORE:
        wp.length = sizeof(WINDOWPLACEMENT);
        if(GetWindowPlacement(hWnd, &wp))
        {
          wp.showCmd = SW_RESTORE;
          SetWindowPlacement(hWnd, &wp);
        }
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

LRESULT
DefWndHandleWindowPosChanging(HWND hWnd, WINDOWPOS* Pos)
{
    POINT maxSize, minTrack;
    LONG style = GetWindowLongA(hWnd, GWL_STYLE);

    if (Pos->flags & SWP_NOSIZE) return 0;
    if ((style & WS_THICKFRAME) || ((style & (WS_POPUP | WS_CHILD)) == 0))
    {
        WinPosGetMinMaxInfo(hWnd, &maxSize, NULL, &minTrack, NULL);
        Pos->cx = min(Pos->cx, maxSize.x);
        Pos->cy = min(Pos->cy, maxSize.y);
        if (!(style & WS_MINIMIZE))
        {
            if (Pos->cx < minTrack.x) Pos->cx = minTrack.x;
            if (Pos->cy < minTrack.y) Pos->cy = minTrack.y;
        }
    }
    return 0;
}

/* Undocumented flags. */
#define SWP_NOCLIENTMOVE          0x0800
#define SWP_NOCLIENTSIZE          0x1000

LRESULT
DefWndHandleWindowPosChanged(HWND hWnd, WINDOWPOS* Pos)
{
  RECT Rect;

  GetClientRect(hWnd, &Rect);
  MapWindowPoints(hWnd, (GetWindowLongW(hWnd, GWL_STYLE) & WS_CHILD ?
                         GetParent(hWnd) : NULL), (LPPOINT) &Rect, 2);

  if (! (Pos->flags & SWP_NOCLIENTMOVE))
    {
      SendMessageW(hWnd, WM_MOVE, 0, MAKELONG(Rect.left, Rect.top));
    }

  if (! (Pos->flags & SWP_NOCLIENTSIZE))
    {
      WPARAM wp = SIZE_RESTORED;
      if (IsZoomed(hWnd))
        {
          wp = SIZE_MAXIMIZED;
        }
      else if (IsIconic(hWnd))
        {
          wp = SIZE_MINIMIZED;
        }
      SendMessageW(hWnd, WM_SIZE, wp,
                   MAKELONG(Rect.right - Rect.left, Rect.bottom - Rect.top));
    }

  return 0;
}

/***********************************************************************
 *           DefWndControlColor
 *
 * Default colors for control painting.
 */
HBRUSH
DefWndControlColor(HDC hDC, UINT ctlType)
{
  if (CTLCOLOR_SCROLLBAR == ctlType)
    {
      HBRUSH hb = GetSysColorBrush(COLOR_SCROLLBAR);
      COLORREF bk = GetSysColor(COLOR_3DHILIGHT);
      SetTextColor(hDC, GetSysColor(COLOR_3DFACE));
      SetBkColor(hDC, bk);

      /* if COLOR_WINDOW happens to be the same as COLOR_3DHILIGHT
       * we better use 0x55aa bitmap brush to make scrollbar's background
       * look different from the window background.
       */
      if (bk == GetSysColor(COLOR_WINDOW))
	{
          static const WORD wPattern55AA[] =
          {
              0x5555, 0xaaaa, 0x5555, 0xaaaa,
              0x5555, 0xaaaa, 0x5555, 0xaaaa
          };
          static HBITMAP hPattern55AABitmap = NULL;
          static HBRUSH hPattern55AABrush = NULL;
          if (hPattern55AABrush == NULL)
            {
              hPattern55AABitmap = CreateBitmap(8, 8, 1, 1, wPattern55AA);
              hPattern55AABrush = CreatePatternBrush(hPattern55AABitmap);
            }
          return hPattern55AABrush;
	}
      UnrealizeObject(hb);
      return hb;
    }

  SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));

  if ((CTLCOLOR_EDIT == ctlType) || (CTLCOLOR_LISTBOX == ctlType))
    {
      SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
    }
  else
    {
      SetBkColor(hDC, GetSysColor(COLOR_3DFACE));
      return GetSysColorBrush(COLOR_3DFACE);
    }

  return GetSysColorBrush(COLOR_WINDOW);
}

VOID FASTCALL
DefWndScreenshot(HWND hWnd)
{
   
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
            return DefWndNCPaint(hWnd, (HRGN)wParam);
        }

        case WM_NCCALCSIZE:
        {
            return DefWndNCCalcSize(hWnd, (BOOL)wParam, (RECT*)lParam);
        }

        case WM_NCACTIVATE:
        {
            return DefWndNCActivate(hWnd, wParam);
        }

        case WM_NCHITTEST:
        {
            POINT Point;
            Point.x = SLOWORD(lParam);
            Point.y = SHIWORD(lParam);
            return (DefWndNCHitTest(hWnd, Point));
        }

        case WM_NCLBUTTONDOWN:
        {
            return (DefWndNCLButtonDown(hWnd, wParam, lParam));
        }

        case WM_NCLBUTTONDBLCLK:
        {
            return (DefWndNCLButtonDblClk(hWnd, wParam, lParam));
        }

        case WM_WINDOWPOSCHANGING:
        {
            return (DefWndHandleWindowPosChanging(hWnd, (WINDOWPOS*)lParam));
        }

        case WM_WINDOWPOSCHANGED:
        {
            return (DefWndHandleWindowPosChanged(hWnd, (WINDOWPOS*)lParam));
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
                SendMessageA(hWnd, WM_CONTEXTMENU, (WPARAM)hWnd, lParam);
            }
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
                    SendMessageA(GetParent(hWnd), WM_CONTEXTMENU, wParam, lParam);
                }
            }
            else
            {
                LONG HitCode;
                POINT Pt;
                DWORD Style;
                
                Style = GetWindowLongW(hWnd, GWL_STYLE);
                
                Pt.x = SLOWORD(lParam);
                Pt.y = SHIWORD(lParam);
                if (Style & WS_CHILD)
                {
                    ScreenToClient(GetParent(hWnd), &Pt);
                }

                HitCode = DefWndNCHitTest(hWnd, Pt);

                if (HitCode == HTCAPTION || HitCode == HTSYSMENU)
                {
                    HMENU SystemMenu;
                    UINT DefItem = SC_CLOSE;
                    
                    if((SystemMenu = GetSystemMenu(hWnd, FALSE)))
                    {
                      if(HitCode == HTCAPTION)
                        DefItem = ((Style & (WS_MAXIMIZE | WS_MINIMIZE)) ? 
                                   SC_RESTORE : SC_MAXIMIZE);
                      
                      SetMenuDefaultItem(SystemMenu, DefItem, MF_BYCOMMAND);
                      
                      TrackPopupMenu(SystemMenu,
                                     TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                                     Pt.x, Pt.y, 0, hWnd, NULL);
                    }
                }
	    }
            break;
        }
        
        case WM_PRINT:
        {
            /* FIXME: Implement. */
            return (0);
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
                    RECT ClientRect;
                    INT x, y;
                    GetClientRect(hWnd, &ClientRect);
                    x = (ClientRect.right - ClientRect.left -
                         GetSystemMetrics(SM_CXICON)) / 2;
                    y = (ClientRect.bottom - ClientRect.top -
                         GetSystemMetrics(SM_CYICON)) / 2;
                    DrawIcon(hDC, x, y, hIcon);
                } 
                EndPaint(hWnd, &Ps);
            }
            return (0);
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
            return (0);
        }

        case WM_SETREDRAW:
        {
            DefWndSetRedraw(hWnd, wParam);
            return (0);
        }

        case WM_CLOSE:
        {
            DestroyWindow(hWnd);
            return (0);
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
                    return (Ret);
                }
            }
            return ((LOWORD(lParam) >= HTCLIENT) ? MA_ACTIVATE : MA_NOACTIVATE);
        }

        case WM_ACTIVATE:
        {
            /* Check if the window is minimized. */
            if (LOWORD(wParam) != WA_INACTIVE &&
                !(GetWindowLongW(hWnd, GWL_STYLE) & WS_MINIMIZE))
            {
                SetFocus(hWnd);
            }
            break;
        }

        case WM_MOUSEWHEEL:
        {
            if (GetWindowLongW(hWnd, GWL_STYLE) & WS_CHILD)
            {
                if (bUnicode)
                {
                    return (SendMessageW(GetParent(hWnd), WM_MOUSEWHEEL,
                                         wParam, lParam));
                }
                else
                {
                    return (SendMessageA(GetParent(hWnd), WM_MOUSEWHEEL,
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
            if (GetClassLongW(hWnd, GCL_STYLE) & CS_PARENTDC)
            {
                /* can't use GetClipBox with a parent DC or we fill the whole parent */
                GetClientRect(hWnd, &Rect);
                DPtoLP((HDC)wParam, (LPPOINT)&Rect, 2);
            }
            else
            {
                GetClipBox((HDC)wParam, &Rect);
            }
            FillRect((HDC)wParam, &Rect, hBrush);
            return (1);
        }
        
        case WM_CTLCOLORMSGBOX:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORSCROLLBAR:
	    return (LRESULT) DefWndControlColor((HDC)wParam, Msg - WM_CTLCOLORMSGBOX);

        case WM_SETCURSOR:
        {
            ULONG Style = GetWindowLongW(hWnd, GWL_STYLE);
            
            if (Style & WS_CHILD)
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
            return (DefWndHandleSetCursor(hWnd, wParam, lParam, Style));
        }

        case WM_SYSCOMMAND:
        {
            POINT Pt;
            Pt.x = SLOWORD(lParam);
            Pt.y = SHIWORD(lParam);
            return (DefWndHandleSysCommand(hWnd, wParam, Pt));
        }

        /* FIXME: Handle key messages. */
/*
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        case WM_SYSCHAR:
*/

        /* FIXME: This is also incomplete. */
        case WM_SYSKEYDOWN:
        {
            if (HIWORD(lParam) & KEYDATA_ALT)
            {
                if (wParam == VK_F4) /* Try to close the window */
                {
                    HWND top = GetAncestor(hWnd, GA_ROOT);
                    if (!(GetClassLongW(top, GCL_STYLE) & CS_NOCLOSE))
                    {
                        if (bUnicode)
                            PostMessageW(top, WM_SYSCOMMAND, SC_CLOSE, 0);
                        else
                            PostMessageA(top, WM_SYSCOMMAND, SC_CLOSE, 0);
                    }
                }
                else if (wParam == VK_SNAPSHOT)
                {
                    DefWndScreenshot(hWnd);
                }
            }
            break;
        }
        
        case WM_SHOWWINDOW:
        {
            LONG Style;

            if (!lParam)
                return 0;
            Style = GetWindowLongW(hWnd, GWL_STYLE);
            if (!(Style & WS_POPUP))
                return 0;
            if ((Style & WS_VISIBLE) && wParam)
                return 0;
            if (!(Style & WS_VISIBLE) && !wParam)
                return 0;
            if (!GetWindow(hWnd, GW_OWNER))
                return 0;
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
            return (-1);
/*
        case WM_DROPOBJECT:
  
            break;
*/
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
                return ((LRESULT)hIcon);
            }
            for (Len = 1; Len < 64; Len++)
            {
                if ((hIcon = LoadIconW(NULL, MAKEINTRESOURCEW(Len))) != NULL)
                {
                    return((LRESULT)hIcon);
                }
            }
            return ((LRESULT)LoadIconW(0, IDI_APPLICATION));
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
           return ((LRESULT)hOldIcon);
        }

        case WM_GETICON:
        {
            INT Index = (wParam != 0) ? GCL_HICON : GCL_HICONSM;
            return (GetClassLongW(hWnd, Index));
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
        
        case WM_SYSTIMER:
        {
          THRDCARETINFO CaretInfo;
          switch(wParam)
          {
            case 0xffff: /* Caret timer */
              /* switch showing byte in win32k and get information about the caret */
              if(NtUserCallOneParam((DWORD)&CaretInfo, ONEPARAM_ROUTINE_SWITCHCARETSHOWING) && (CaretInfo.hWnd == hWnd))
              {
                DrawCaret(hWnd, &CaretInfo);
              }
              break;
          }
          break;
        }

        case WM_QUERYOPEN:
        case WM_QUERYENDSESSION:
        {
            return (1);
        }
    }
    return 0;
}


LRESULT STDCALL
DefWindowProcA(HWND hWnd,
	       UINT Msg,
	       WPARAM wParam,
	       LPARAM lParam)
{
    switch (Msg)
    {
        case WM_NCCREATE:
        {
            return TRUE;
        }

        case WM_GETTEXTLENGTH:
        {
            return InternalGetWindowText(hWnd, NULL, 0);
        }

        case WM_GETTEXT:
        {
            UNICODE_STRING UnicodeString;
            LPSTR AnsiBuffer = (LPSTR)lParam;
            BOOL Result;

            if (wParam > 1)
            {
                *((PWSTR)lParam) = '\0';
            }
            UnicodeString.Length = UnicodeString.MaximumLength =
                wParam * sizeof(WCHAR);
            UnicodeString.Buffer = HeapAlloc(GetProcessHeap(), 0,
                UnicodeString.Length);
            if (!UnicodeString.Buffer)
                return FALSE;
            Result = InternalGetWindowText(hWnd, UnicodeString.Buffer, wParam);
            if (wParam > 0 &&
                !WideCharToMultiByte(CP_ACP, 0, UnicodeString.Buffer, -1,
                AnsiBuffer, wParam, NULL, NULL))
            {
                AnsiBuffer[wParam - 1] = 0;
            }
            HeapFree(GetProcessHeap(), 0, UnicodeString.Buffer);

            return Result;
        }

        case WM_SETTEXT:
        {
            ANSI_STRING AnsiString;
            RtlInitAnsiString(&AnsiString, (LPSTR)lParam);
            NtUserDefSetText(hWnd, &AnsiString);
            if ((GetWindowLongW(hWnd, GWL_STYLE) & WS_CAPTION) == WS_CAPTION)
            {
                DefWndNCPaint(hWnd, (HRGN)1);
            }
            return TRUE;
        }

/*
        FIXME: Implement these.
        case WM_IME_CHAR:
        case WM_IME_KEYDOWN:
        case WM_IME_KEYUP:
        case WM_IME_STARTCOMPOSITION:
        case WM_IME_COMPOSITION:
        case WM_IME_ENDCOMPOSITION:
        case WM_IME_SELECT:
        case WM_IME_SETCONTEXT:
*/
    }

    return User32DefWindowProc(hWnd, Msg, wParam, lParam, FALSE);
}


LRESULT STDCALL
DefWindowProcW(HWND hWnd,
	       UINT Msg,
	       WPARAM wParam,
	       LPARAM lParam)
{
    switch (Msg)
    {
        case WM_NCCREATE:
        {
            return TRUE;
        }

        case WM_GETTEXTLENGTH:
        {
            return InternalGetWindowText(hWnd, NULL, 0);
        }

        case WM_GETTEXT:
        {
            DWORD Result;
            if (wParam > 1)
            {
                *((PWSTR)lParam) = '\0';
            }
            Result = InternalGetWindowText(hWnd, (PWSTR)lParam, wParam);
            return Result;
        }

        case WM_SETTEXT:
        {
            UNICODE_STRING UnicodeString;
            ANSI_STRING AnsiString;

            RtlInitUnicodeString(&UnicodeString, (LPWSTR)lParam);
            RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, TRUE);
            NtUserDefSetText(hWnd, &AnsiString);
            RtlFreeAnsiString(&AnsiString);
            if ((GetWindowLongW(hWnd, GWL_STYLE) & WS_CAPTION) == WS_CAPTION)
            {
                DefWndNCPaint(hWnd, (HRGN)1);
            }
            return (1);
        }

        case WM_IME_CHAR:
        {
            SendMessageW(hWnd, WM_CHAR, wParam, lParam);
            return (0);
        }

        case WM_IME_SETCONTEXT:
        {
            /* FIXME */
            return (0);
        }
    }

    return User32DefWindowProc(hWnd, Msg, wParam, lParam, TRUE);
}
