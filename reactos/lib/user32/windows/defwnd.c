/* $Id: defwnd.c,v 1.82 2003/09/08 02:14:20 weiden Exp $
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


static COLORREF SysColours[29] =
  {
    RGB(224, 224, 224) /* COLOR_SCROLLBAR */,
    RGB(58, 110, 165) /* COLOR_BACKGROUND */,
    RGB(0, 0, 128) /* COLOR_ACTIVECAPTION */,
    RGB(128, 128, 128) /* COLOR_INACTIVECAPTION */,
    RGB(192, 192, 192) /* COLOR_MENU */,
    RGB(255, 255, 255) /* COLOR_WINDOW */,
    RGB(0, 0, 0) /* COLOR_WINDOWFRAME */,
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
    ULONG uStyle = GetWindowLongW(hWnd, GWL_STYLE);
    return (uStyle & WS_MAXIMIZEBOX);
}

BOOL
IsCloseBoxActive(HWND hWnd)
{
    ULONG uStyle = GetWindowLongW(hWnd, GWL_STYLE);
    return (uStyle & WS_SYSMENU);
}

BOOL
IsMinBoxActive(HWND hWnd)
{
    ULONG uStyle = GetWindowLongW(hWnd, GWL_STYLE);
    return (uStyle & WS_MINIMIZEBOX);
}

INT
UIGetFrameSizeX(HWND hWnd)
{
    ULONG uStyle = GetWindowLongW(hWnd, GWL_STYLE);

    if ( uStyle & WS_THICKFRAME )
        return GetSystemMetrics(SM_CXSIZEFRAME);
    else
        return GetSystemMetrics(SM_CXFRAME);
}

INT
UIGetFrameSizeY(HWND hWnd)
{
    ULONG uStyle = GetWindowLongW(hWnd, GWL_STYLE);

    if (uStyle & WS_THICKFRAME)
        return GetSystemMetrics(SM_CYSIZEFRAME);
    else
        return GetSystemMetrics(SM_CYFRAME);
}

VOID
UserSetupInternalPos(VOID)
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

/*
 * @implemented
 */
HPEN STDCALL
GetSysColorPen(int nIndex)
{
    return CreatePen(PS_SOLID, 1, SysColours[nIndex]);
}

/*
 * @implemented
 */
HBRUSH STDCALL
GetSysColorBrush(int nIndex)
{
    return CreateSolidBrush(SysColours[nIndex]);
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
    return ((LRESULT)0);
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
    return ((LRESULT)0);
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
    PINTERNALPOS lpPos = (PINTERNALPOS)GetPropA(hWnd, (LPSTR)(DWORD)AtomInternalPos);

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
    return (!(Style & WS_CHILD) && GetMenu(hWnd) != 0);
}

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

WINBOOL
UserDrawSysMenuButton(HWND hWnd, HDC hDC, LPRECT Rect, BOOL down)
{
    HDC hDcMem;
    HBITMAP hSavedBitmap;

    if (!hbSysMenu)
    {
        hbSysMenu = (HBITMAP)LoadBitmapW(0, MAKEINTRESOURCEW(OBM_CLOSE));
    }
    hDcMem = CreateCompatibleDC(hDC);
    if (!hDcMem)
    {
        return FALSE;
    }
    hSavedBitmap = SelectObject(hDcMem, hbSysMenu);
    if (!hSavedBitmap)
    {
        DeleteDC(hDcMem);
        return FALSE;
    }

    BitBlt(hDC, Rect->left + 2, Rect->top + 3, 16, 14, hDcMem,
           (GetWindowLongW(hWnd, GWL_STYLE) & WS_CHILD) ?
	        GetSystemMetrics(SM_CXSIZE): 0, 0, SRCCOPY);

    SelectObject(hDcMem, hSavedBitmap);
    DeleteDC(hDcMem);
    return TRUE;
}

/*
 * FIXME:
 * Cache bitmaps, then just bitblt instead of calling DFC() (and
 * wasting precious CPU cycles) every time
 */
static void
UserDrawCaptionButton(HWND hWnd, HDC hDC, BOOL bDown, ULONG Type)
{
    RECT rect;
    INT iBmpWidth = GetSystemMetrics(SM_CXSIZE) - 2;
    INT iBmpHeight = GetSystemMetrics(SM_CYSIZE) - 4;
    INT OffsetX = UIGetFrameSizeX(hWnd);
    INT OffsetY = UIGetFrameSizeY(hWnd);

    if (!(GetWindowLongW(hWnd, GWL_STYLE) & WS_SYSMENU))
    {
        return;
    }

    GetWindowRect(hWnd, &rect);

    rect.right = rect.right - rect.left;
    rect.bottom = rect.bottom - rect.top;
    rect.left = rect.top = 0;

    switch(Type)
    {
        case DFCS_CAPTIONMIN:
        {
            if ((GetWindowLongW(hWnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) == TRUE)
                return; /* ToolWindows don't have min/max buttons */

            SetRect(&rect, rect.right - OffsetX - (iBmpWidth * 3) - 5,
                    OffsetY + 2, rect.right - (iBmpWidth * 2) - OffsetX - 5,
                    rect.top + iBmpHeight + OffsetY + 2);
            DrawFrameControl(hDC, &rect, DFC_CAPTION,
                             DFCS_CAPTIONMIN | (bDown ? DFCS_PUSHED : 0) |
                             (IsMinBoxActive(hWnd) ? 0 : DFCS_INACTIVE));
            break;
        }
        case DFCS_CAPTIONMAX:
        {
            if ((GetWindowLongW(hWnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) == TRUE)
                return; /* ToolWindows don't have min/max buttons */

            SetRect(&rect, rect.right - OffsetX - (iBmpWidth * 2) - 5,
                    OffsetY + 2, rect.right - iBmpWidth - OffsetX - 5,
                    rect.top + iBmpHeight + OffsetY + 2);
            DrawFrameControl(hDC, &rect, DFC_CAPTION,
                             (IsZoomed(hWnd) ? DFCS_CAPTIONRESTORE : DFCS_CAPTIONMAX) |
                             (bDown ? DFCS_PUSHED : 0) |
                             (IsMaxBoxActive(hWnd) ? 0 : DFCS_INACTIVE));
            break;
        }
        case DFCS_CAPTIONCLOSE:
        {
            SetRect(&rect, rect.right - OffsetX - iBmpWidth - 3,
                    OffsetY + 2,	rect.right - OffsetX - 3,
                    rect.top + iBmpHeight + OffsetY + 2 );      
            DrawFrameControl(hDC, &rect, DFC_CAPTION,
                             (DFCS_CAPTIONCLOSE | (bDown ? DFCS_PUSHED : 0) |
                             (IsCloseBoxActive(hWnd) ? 0 : DFCS_INACTIVE)));
            break;
        }
    }
}

// Enabling this will cause captions to draw smoother, but slower:
// #define DOUBLE_BUFFER_CAPTION
// NOTE: Double buffering appears to be broken for this at the moment

/*
 * @implemented
 */
WINBOOL STDCALL
DrawCaption(
  HWND hWnd,
  HDC hDC,
  LPRECT lprc,
  UINT uFlags)
{
    NONCLIENTMETRICSW nclm;
    BOOL result = FALSE;
    RECT r = *lprc;
    UINT VCenter = 0, Padding = 0;
    WCHAR buffer[256];
    HFONT hFont = NULL;
    HFONT hOldFont = NULL;
    HBRUSH OldBrush = NULL;
    HDC MemDC = NULL;

#ifdef DOUBLE_BUFFER_CAPTION
    HBITMAP MemBMP = NULL, OldBMP = NULL;

    MemDC = CreateCompatibleDC(hDC);
    if (! MemDC) goto cleanup;
    MemBMP = CreateCompatibleBitmap(hDC, lprc->right - lprc->left, lprc->bottom - lprc->top);
    if (! MemBMP) goto cleanup;
    OldBMP = SelectObject(MemDC, MemBMP);
    if (! OldBMP) goto cleanup;
#else
    MemDC = hDC;

    OffsetViewportOrgEx(MemDC, lprc->left, lprc->top, NULL);
#endif

    // If DC_GRADIENT is specified, a Win 98/2000 style caption gradient should
    // be painted. For now, that flag is ignored:
    // Windows 98/Me, Windows 2000/XP: When this flag is set, the function uses
    // COLOR_GRADIENTACTIVECAPTION (if the DC_ACTIVE flag was set) or
    // COLOR_GRADIENTINACTIVECAPTION for the title-bar color. 

    // Draw the caption background
    if (uFlags & DC_INBUTTON)
    {
        OldBrush = SelectObject(MemDC, GetSysColorBrush(uFlags & DC_ACTIVE ? COLOR_BTNFACE : COLOR_BTNSHADOW) );
        if (! OldBrush) goto cleanup;
        if (! PatBlt(MemDC, 0, 0, lprc->right - lprc->left, lprc->bottom - lprc->top, PATCOPY )) goto cleanup;
    }
    else
    {
        // DC_GRADIENT check should go here somewhere
        OldBrush = SelectObject(MemDC, GetSysColorBrush(uFlags & DC_ACTIVE ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION) );
        if (! OldBrush) goto cleanup;
        if (! PatBlt(MemDC, 0, 0, lprc->right - lprc->left, lprc->bottom - lprc->top, PATCOPY )) goto cleanup;
    }

    VCenter = (lprc->bottom - lprc->top) / 2;
    Padding = VCenter - (GetSystemMetrics(SM_CYCAPTION) / 2);

    r.left = Padding;
    r.right = r.left + (lprc->right - lprc->left);
    r.top = Padding;
    r.bottom = r.top + (GetSystemMetrics(SM_CYCAPTION) / 2);

    if (uFlags & DC_ICON)
    {
        // For some reason the icon isn't centered correctly...
        r.top --;
        UserDrawSysMenuButton(hWnd, MemDC, &r, FALSE);
        r.top ++;
    }

    r.top ++;
    r.left += 2;

  if ((uFlags & DC_TEXT) && (GetWindowTextW( hWnd, buffer, sizeof(buffer)/sizeof(buffer[0]) )))
  {
    // Duplicate odd behaviour from Windows:
    if ((! uFlags & DC_SMALLCAP) || (uFlags & DC_ICON) || (uFlags & DC_INBUTTON) ||
        (! uFlags & DC_ACTIVE))
        r.left += GetSystemMetrics(SM_CXSIZE) + Padding;

    r.right = (lprc->right - lprc->left);

    nclm.cbSize = sizeof(nclm);
    if (! SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &nclm, 0)) goto cleanup;

    if (uFlags & DC_INBUTTON)
        SetTextColor(MemDC, SysColours[ uFlags & DC_ACTIVE ? COLOR_BTNTEXT : COLOR_GRAYTEXT]);
    else
        SetTextColor(MemDC, SysColours[ uFlags & DC_ACTIVE ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT]);

    SetBkMode( MemDC, TRANSPARENT );
    if (GetWindowLongW(hWnd, GWL_STYLE) & WS_EX_TOOLWINDOW)
//    if (uFlags & DC_SMALLCAP) // incorrect
        hFont = CreateFontIndirectW(&nclm.lfSmCaptionFont);
    else
        hFont = CreateFontIndirectW(&nclm.lfCaptionFont);

    if (! hFont) goto cleanup;

    hOldFont = SelectObject(MemDC, hFont);
    if (! hOldFont) goto cleanup;

    DrawTextW(MemDC, buffer, wcslen(buffer), &r, DT_VCENTER | DT_END_ELLIPSIS);
    // Old method:
    // TextOutW(hDC, r.left + (GetSystemMetrics(SM_CXDLGFRAME) * 2), lprc->top + (nclm.lfCaptionFont.lfHeight / 2), buffer, wcslen(buffer));
  }

    if (uFlags & DC_BUTTONS)
    {
        // Windows XP draws the caption buttons with DC_BUTTONS
//        r.left += GetSystemMetrics(SM_CXSIZE) + 1;
//        UserDrawCaptionButton( hWnd, hDC, FALSE, DFCS_CAPTIONCLOSE);
//        r.right -= GetSystemMetrics(SM_CXSMSIZE) + 1;
//        UserDrawCaptionButton( hWnd, hDC, FALSE, DFCS_CAPTIONMIN);
//        UserDrawCaptionButton( hWnd, hDC, FALSE, DFCS_CAPTIONMAX);
    }

#ifdef DOUBLE_BUFFER_CAPTION
    if (! BitBlt(hDC, lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top,
            MemDC, 0, 0, SRCCOPY)) goto cleanup;
#endif

    result = TRUE;

    cleanup :
        if (MemDC)
        {
            if (OldBrush) SelectObject(MemDC, OldBrush);
            if (hOldFont) SelectObject(MemDC, hOldFont);
            if (hFont) DeleteObject(hFont);
#ifdef DOUBLE_BUFFER_CAPTION
            if (OldBMP) SelectObject(MemDC, OldBMP);
            if (MemBMP) DeleteObject(MemBMP);
            DeleteDC(MemDC);
#else
            OffsetViewportOrgEx(MemDC, -lprc->left, -lprc->top, NULL);
#endif
        }

        return result;
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
  UINT capflags = 0;

    capflags = DC_ICON | DC_TEXT;
    capflags |= (active & DC_ACTIVE);

    if (GetWindowLongW(hWnd, GWL_STYLE) & WS_EX_TOOLWINDOW)
        capflags |= DC_SMALLCAP;

//  Old code:
//  PatBlt(hDC,rect->left + GetSystemMetrics(SM_CXFRAME), rect->top +
//     GetSystemMetrics(SM_CYFRAME), rect->right - (GetSystemMetrics(SM_CXFRAME) * 2), (rect->top + 
//     GetSystemMetrics(SM_CYCAPTION)) - 1, PATCOPY );

    r.left += GetSystemMetrics(SM_CXFRAME);
    r.top += GetSystemMetrics(SM_CYFRAME);
    r.right -= GetSystemMetrics(SM_CXFRAME);
    r.bottom = r.top + GetSystemMetrics(SM_CYCAPTION);
//     GetSystemMetrics(SM_CYCAPTION)) - 1, PATCOPY );

    DrawCaption(hWnd, hDC, &r, capflags);
  
  if (style & WS_SYSMENU)
  {
//    UserDrawSysMenuButton( hWnd, hDC, FALSE);
    r.left += GetSystemMetrics(SM_CXSIZE) + 1;
    UserDrawCaptionButton( hWnd, hDC, FALSE, DFCS_CAPTIONCLOSE);
    r.right -= GetSystemMetrics(SM_CXSMSIZE) + 1;
    UserDrawCaptionButton( hWnd, hDC, FALSE, DFCS_CAPTIONMIN);
    UserDrawCaptionButton( hWnd, hDC, FALSE, DFCS_CAPTIONMAX);
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
	      WindowRect.right -= GetSystemMetrics(SM_CXSIZE);
	    }
	  if (Point.x >= WindowRect.right)
	    {
	      return(HTMAXBUTTON);
	    }

	  if (Style & WS_MINIMIZEBOX)
	    {
	      WindowRect.right -= GetSystemMetrics(SM_CXSIZE);
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

VOID STATIC
DefWndDoButton(HWND hWnd, WPARAM wParam)
{
  MSG Msg;
  BOOL InBtn = TRUE, HasBtn = FALSE;
  ULONG Btn;
  WPARAM SCMsg, CurBtn = wParam, OrigBtn = wParam;
  
  switch(wParam)
  {
    case HTCLOSE:
      Btn = DFCS_CAPTIONCLOSE;
      SCMsg = SC_CLOSE;
      HasBtn = IsCloseBoxActive(hWnd);
      break;
    case HTMINBUTTON:
      Btn = DFCS_CAPTIONMIN;
      SCMsg = SC_MINIMIZE;
      HasBtn = IsMinBoxActive(hWnd);
      break;
    case HTMAXBUTTON:
      Btn = DFCS_CAPTIONMAX;
      SCMsg = SC_MAXIMIZE;
      HasBtn = IsMaxBoxActive(hWnd);
      break;
    default:
      return;
  }
  
  if(!HasBtn)
    return;
  
  SetCapture(hWnd);
  UserDrawCaptionButton( hWnd, GetWindowDC(hWnd), HasBtn , Btn);
  
  while(1)
  {
    GetMessageW(&Msg, 0, 0, 0);
    switch(Msg.message)
    {
      case WM_NCLBUTTONUP:
      case WM_LBUTTONUP:
        if(InBtn)
          goto done;
        else
        {
          ReleaseCapture();
          return;
        }
      case WM_NCMOUSEMOVE:
      case WM_MOUSEMOVE:
        CurBtn = DefWndHitTestNC(hWnd, Msg.pt);
        if(InBtn != (CurBtn == OrigBtn))
        {
          UserDrawCaptionButton( hWnd, GetWindowDC(hWnd), (CurBtn == OrigBtn) , Btn);
        }
        InBtn = CurBtn == OrigBtn;
        break;
    }
  }
  
done:
  UserDrawCaptionButton( hWnd, GetWindowDC(hWnd), FALSE , Btn);
  ReleaseCapture();
  SendMessageA(hWnd, WM_SYSCOMMAND, SCMsg, 0);
  return;
}

VOID STATIC
DefWndDoScrollBarDown(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  POINT Point;
  DWORD hit;
  Point.x = SLOWORD(lParam);
  Point.y = SHIWORD(lParam);
  
  hit = SCROLL_HitTest(hWnd, (wParam == HTHSCROLL) ? SB_HORZ : SB_VERT, Point, FALSE);
  
  if(hit)
    DbgPrint("SCROLL_HitTest() == 0x%x\n", hit);
  
  SendMessageA(hWnd, WM_SYSCOMMAND, Msg + (UINT)wParam, lParam);
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
//		  UserDrawSysMenuButton(hWnd, hDC, TRUE);
		  ReleaseDC(hWnd, hDC);
		}
	      SendMessageA(hWnd, WM_SYSCOMMAND, SC_MOUSEMENU + HTSYSMENU,
			   lParam);
	    }
	  break;
        }
        case HTMENU:
        {
            SendMessageA(hWnd, WM_SYSCOMMAND, SC_MOUSEMENU + HTMENU, lParam);
            break;
        }
        case HTHSCROLL:
        {
            DefWndDoScrollBarDown(hWnd, SC_HSCROLL, HTHSCROLL, lParam);
            //SendMessageA(hWnd, WM_SYSCOMMAND, SC_HSCROLL + HTHSCROLL, lParam);
            break;
        }
        case HTVSCROLL:
        {
            DefWndDoScrollBarDown(hWnd, SC_VSCROLL, HTVSCROLL, lParam);
            //SendMessageA(hWnd, WM_SYSCOMMAND, SC_VSCROLL + HTVSCROLL, lParam);
            break;
        }
        case HTMINBUTTON:
        case HTMAXBUTTON:
        case HTCLOSE:
        {
          DefWndDoButton(hWnd, wParam);
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
  UNIMPLEMENTED;
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
        if (UserHasMenu(hWnd, GetWindowLongW(hWnd, GWL_STYLE)))
        {
            Rect->top += MenuGetMenuBarHeight(hWnd, Rect->right - Rect->left,
                -TmpRect.left, -TmpRect.top) + 1;
        }
        if (Rect->top > Rect->bottom)
            Rect->bottom = Rect->top;
        if (Rect->left > Rect->right)
            Rect->right = Rect->left;
    }

    return (Result);
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
    RECT rect;

    GetClientRect(hWnd, &rect);

    if (!(Pos->flags & SWP_NOCLIENTMOVE))
        SendMessageW(hWnd, WM_MOVE, 0, MAKELONG(rect.left, rect.top));

    if (!(Pos->flags & SWP_NOCLIENTSIZE))
    {
        WPARAM wp = SIZE_RESTORED;
        if (IsZoomed(hWnd)) wp = SIZE_MAXIMIZED;
        else if (IsIconic(hWnd)) wp = SIZE_MINIMIZED;
        SendMessageW(hWnd, WM_SIZE, wp,
            MAKELONG(rect.right - rect.left, rect.bottom - rect.top));
    }

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
            return (DefWndPaintNC(hWnd, (HRGN)wParam));
        }

        case WM_NCCALCSIZE:
        {
            return (DefWndNCCalcSize(hWnd, (RECT*)lParam));
        }

        case WM_WINDOWPOSCHANGING:
        {
            return (DefWndHandleWindowPosChanging(hWnd, (WINDOWPOS*)lParam));
        }

        case WM_WINDOWPOSCHANGED:
        {
            return (DefWndHandleWindowPosChanged(hWnd, (WINDOWPOS*)lParam));
        }

        case WM_NCHITTEST:
        {
            POINT Point;
            Point.x = SLOWORD(lParam);
            Point.y = SHIWORD(lParam);
            return (DefWndHitTestNC(hWnd, Point));
        }

        case WM_NCLBUTTONDOWN:
        {
            return (DefWndHandleLButtonDownNC(hWnd, wParam, lParam));
        }

        case WM_NCLBUTTONUP:
        {
            return (DefWndHandleLButtonUpNC(hWnd, wParam, lParam));
        }

        case WM_LBUTTONDBLCLK:
        case WM_NCLBUTTONDBLCLK:
        {
            return (DefWndHandleLButtonDblClkNC(hWnd, wParam, lParam));
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
            return (DefWndHandleActiveNC(hWnd, wParam));
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
            if (0 == (((DWORD) hBrush) & 0xffff0000))
            {
                hBrush = GetSysColorBrush((DWORD) hBrush - 1);
            }
            GetClipBox((HDC)wParam, &Rect);
            FillRect((HDC)wParam, &Rect, hBrush);
            return (1);
        }

        /* FIXME: Implement colour controls. */
/*
        case WM_CTLCOLORMSGBOX:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLOR:
*/

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
            return (DefWndHandleSetCursor(hWnd, wParam, lParam));
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
    static LPSTR WindowTextAtom = 0;
    PSTR WindowText;

    switch (Msg)
    {
        case WM_NCCREATE:
        {
            CREATESTRUCTA *Cs = (CREATESTRUCTA*)lParam;
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
            return (1);
        }

        case WM_GETTEXTLENGTH:
        {
            if (WindowTextAtom == 0 ||
                (WindowText = GetPropA(hWnd, WindowTextAtom)) == NULL)
            {
                return(0);
            }
            return (strlen(WindowText));
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
                return (0);
            }
            strncpy((LPSTR)lParam, WindowText, wParam);
            return (min(wParam, strlen(WindowText)));
        }

        case WM_SETTEXT:
        {
            if (0 == WindowTextAtom)
            {
                WindowTextAtom =
                    (LPSTR)(DWORD)GlobalAddAtomA("USER32!WindowTextAtomA");
	        }
            if (WindowTextAtom != 0 &&
                (WindowText = GetPropA(hWnd, WindowTextAtom)) == NULL)
	        {
                RtlFreeHeap(RtlGetProcessHeap(), 0, WindowText);
            }
            WindowText = RtlAllocateHeap(RtlGetProcessHeap(), 0,
                (strlen((PSTR)lParam) + 1) * sizeof(CHAR));
            strcpy(WindowText, (PSTR)lParam);
            SetPropA(hWnd, WindowTextAtom, WindowText);
            if (0 != (GetWindowLongW(hWnd, GWL_STYLE) & WS_CAPTION))
	        {
                DefWndPaintNC(hWnd, (HRGN) 1);
            }
            return (1);
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

        case WM_NCDESTROY:
        {
            if (WindowTextAtom != 0 &&
                (WindowText = RemovePropA(hWnd, WindowTextAtom)) == NULL)
            {
                RtlFreeHeap(GetProcessHeap(), 0, WindowText);
            }
            return(0);
        }
    }

    return User32DefWindowProc(hWnd, Msg, wParam, lParam, FALSE);
}


LRESULT STDCALL
DefWindowProcW(HWND hWnd,
	       UINT Msg,
	       WPARAM wParam,
	       LPARAM lParam)
{
    static LPWSTR WindowTextAtom = 0;
    PWSTR WindowText;

    switch (Msg)
    {
        case WM_NCCREATE:
        {
            CREATESTRUCTW* CreateStruct = (CREATESTRUCTW*)lParam;
            if (HIWORD(CreateStruct->lpszName))
            {
                if (0 == WindowTextAtom)
                {
                    WindowTextAtom =
                        (LPWSTR)(DWORD)GlobalAddAtomW(L"USER32!WindowTextAtomW");
                }
                WindowText = RtlAllocateHeap(RtlGetProcessHeap(), 0,
			        wcslen(CreateStruct->lpszName) * sizeof(WCHAR));
                wcscpy(WindowText, CreateStruct->lpszName);
                SetPropW(hWnd, WindowTextAtom, WindowText);
            }
            return (1);
        }

        case WM_GETTEXTLENGTH:
        {
            if (WindowTextAtom == 0 ||
                (WindowText = GetPropW(hWnd, WindowTextAtom)) == NULL)
            {
                return(0);
            }
            return (wcslen(WindowText));
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
               return (0);
            }
            wcsncpy((PWSTR)lParam, WindowText, wParam);
            return (min(wParam, wcslen(WindowText)));
        }

        case WM_SETTEXT:
        {
            if (WindowTextAtom == 0)
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
			    (wcslen((PWSTR)lParam) + 1) * sizeof(WCHAR));
            wcscpy(WindowText, (PWSTR)lParam);
            SetPropW(hWnd, WindowTextAtom, WindowText);
            if ((GetWindowLongW(hWnd, GWL_STYLE) & WS_CAPTION) == WS_CAPTION)
	        {
                DefWndPaintNC(hWnd, (HRGN)1);
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

        case WM_NCDESTROY:
        {
            if (WindowTextAtom != 0 &&
                (WindowText = RemovePropW(hWnd, WindowTextAtom)) == NULL)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, WindowText);
            }
            return (0);
        }
    }

    return User32DefWindowProc(hWnd, Msg, wParam, lParam, TRUE);
}
