/*
 *	Systray
 *
 *	Copyright 1999 Kai Morich	<kai.morich@bigfoot.de>
 *
 *  Manage the systray window. That it actually appears in the docking
 *  area of KDE is handled in dlls/x11drv/window.c,
 *  X11DRV_set_wm_hints using KWM_DOCKWINDOW.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "shlobj.h"
#include "shellapi.h"
#include "shell32_main.h"
#include "commctrl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

typedef struct SystrayItem {
  HWND                  hWnd;
  HWND                  hWndToolTip;
  NOTIFYICONDATAW       notifyIcon;
  struct SystrayItem    *nextTrayItem;
} SystrayItem;

static SystrayItem *systray=NULL;
static int firstSystray=TRUE; /* defer creation of window class until first systray item is created */


#define ICON_SIZE GetSystemMetrics(SM_CXSMICON)
/* space around icon (forces icon to center of KDE systray area) */
#define ICON_BORDER  4



static BOOL SYSTRAY_ItemIsEqual(PNOTIFYICONDATAW pnid1, PNOTIFYICONDATAW pnid2)
{
  if (pnid1->hWnd != pnid2->hWnd) return FALSE;
  if (pnid1->uID  != pnid2->uID)  return FALSE;
  return TRUE;
}


static void SYSTRAY_ItemTerm(SystrayItem *ptrayItem)
{
  if(ptrayItem->notifyIcon.hIcon)
     DestroyIcon(ptrayItem->notifyIcon.hIcon);
  if(ptrayItem->hWndToolTip)
      DestroyWindow(ptrayItem->hWndToolTip);
  if(ptrayItem->hWnd)
    DestroyWindow(ptrayItem->hWnd);
  return;
}


static BOOL SYSTRAY_Delete(PNOTIFYICONDATAW pnid)
{
  SystrayItem **ptrayItem = &systray;

  while (*ptrayItem) {
    if (SYSTRAY_ItemIsEqual(pnid, &(*ptrayItem)->notifyIcon)) {
      SystrayItem *next = (*ptrayItem)->nextTrayItem;
      TRACE("%p: %p %s\n", *ptrayItem, (*ptrayItem)->notifyIcon.hWnd, debugstr_w((*ptrayItem)->notifyIcon.szTip));
      SYSTRAY_ItemTerm(*ptrayItem);

      HeapFree(GetProcessHeap(),0,*ptrayItem);
      *ptrayItem = next;

      return TRUE;
    }
    ptrayItem = &((*ptrayItem)->nextTrayItem);
  }

  return FALSE; /* not found */
}

static LRESULT CALLBACK SYSTRAY_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  HDC hdc;
  PAINTSTRUCT ps;

  switch (message) {
  case WM_PAINT:
  {
    RECT rc;
    SystrayItem  *ptrayItem = systray;

    while (ptrayItem) {
      if (ptrayItem->hWnd==hWnd) {
	if (ptrayItem->notifyIcon.hIcon) {
	  hdc = BeginPaint(hWnd, &ps);
	  GetClientRect(hWnd, &rc);
	  if (!DrawIconEx(hdc, rc.left+ICON_BORDER, rc.top+ICON_BORDER, ptrayItem->notifyIcon.hIcon,
			  ICON_SIZE, ICON_SIZE, 0, 0, DI_DEFAULTSIZE|DI_NORMAL)) {
	    ERR("Paint(SystrayWindow %p) failed -> removing SystrayItem %p\n", hWnd, ptrayItem);
	    SYSTRAY_Delete(&ptrayItem->notifyIcon);
	  }
	}
	break;
      }
      ptrayItem = ptrayItem->nextTrayItem;
    }
    EndPaint(hWnd, &ps);
  }
  break;

  case WM_MOUSEMOVE:
  case WM_LBUTTONDOWN:
  case WM_LBUTTONUP:
  case WM_RBUTTONDOWN:
  case WM_RBUTTONUP:
  case WM_MBUTTONDOWN:
  case WM_MBUTTONUP:
  {
    MSG msg;
    SystrayItem *ptrayItem = systray;

    while ( ptrayItem ) {
      if (ptrayItem->hWnd == hWnd) {
        msg.hwnd=hWnd;
        msg.message=message;
        msg.wParam=wParam;
        msg.lParam=lParam;
        msg.time = GetMessageTime ();
        msg.pt.x = LOWORD(GetMessagePos ());
        msg.pt.y = HIWORD(GetMessagePos ());

        SendMessageW(ptrayItem->hWndToolTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
      }
      ptrayItem = ptrayItem->nextTrayItem;
    }
  }
  /* fall through */

  case WM_LBUTTONDBLCLK:
  case WM_RBUTTONDBLCLK:
  case WM_MBUTTONDBLCLK:
  {
    SystrayItem *ptrayItem = systray;

    while (ptrayItem) {
      if (ptrayItem->hWnd == hWnd) {
	if (ptrayItem->notifyIcon.hWnd && ptrayItem->notifyIcon.uCallbackMessage) {
          if (!PostMessageW(ptrayItem->notifyIcon.hWnd, ptrayItem->notifyIcon.uCallbackMessage,
                            (WPARAM)ptrayItem->notifyIcon.uID, (LPARAM)message)) {
	      ERR("PostMessage(SystrayWindow %p) failed -> removing SystrayItem %p\n", hWnd, ptrayItem);
	      SYSTRAY_Delete(&ptrayItem->notifyIcon);
	    }
        }
	break;
      }
      ptrayItem = ptrayItem->nextTrayItem;
    }
  }
  break;

  default:
    return (DefWindowProcW(hWnd, message, wParam, lParam));
  }
  return (0);

}


static BOOL SYSTRAY_RegisterClass(void)
{
  WNDCLASSW  wc;
  static const WCHAR WineSystrayW[] = { 'W','i','n','e','S','y','s','t','r','a','y',0 };

  wc.style         = CS_SAVEBITS|CS_DBLCLKS;
  wc.lpfnWndProc   = SYSTRAY_WndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = 0;
  wc.hIcon         = 0;
  wc.hCursor       = LoadCursorW(0, (LPWSTR)IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszMenuName  = NULL;
  wc.lpszClassName = WineSystrayW;

  if (!RegisterClassW(&wc)) {
    ERR("RegisterClass(WineSystray) failed\n");
    return FALSE;
  }
  return TRUE;
}


static BOOL SYSTRAY_ItemInit(SystrayItem *ptrayItem)
{
  RECT rect;
  static const WCHAR WineSystrayW[] = { 'W','i','n','e','S','y','s','t','r','a','y',0 };
  static const WCHAR Wine_SystrayW[] = { 'W','i','n','e','-','S','y','s','t','r','a','y',0 };

  /* Register the class if this is our first tray item. */
  if ( firstSystray ) {
    firstSystray = FALSE;
    if ( !SYSTRAY_RegisterClass() ) {
      ERR( "RegisterClass(WineSystray) failed\n" );
      return FALSE;
    }
  }

  /* Initialize the window size. */
  rect.left   = 0;
  rect.top    = 0;
  rect.right  = ICON_SIZE+2*ICON_BORDER;
  rect.bottom = ICON_SIZE+2*ICON_BORDER;

  ZeroMemory( ptrayItem, sizeof(SystrayItem) );
  /* Create tray window for icon. */
  ptrayItem->hWnd = CreateWindowExW( WS_EX_TRAYWINDOW,
                                WineSystrayW, Wine_SystrayW,
                                WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                rect.right-rect.left, rect.bottom-rect.top,
                                0, 0, 0, 0 );
  if ( !ptrayItem->hWnd ) {
    ERR( "CreateWindow(WineSystray) failed\n" );
    return FALSE;
  }

  /* Create tooltip for icon. */
  ptrayItem->hWndToolTip = CreateWindowW( TOOLTIPS_CLASSW,NULL,TTS_ALWAYSTIP,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     ptrayItem->hWnd, 0, 0, 0 );
  if ( !ptrayItem->hWndToolTip ) {
    ERR( "CreateWindow(TOOLTIP) failed\n" );
    return FALSE;
  }
  return TRUE;
}


static void SYSTRAY_ItemSetMessage(SystrayItem *ptrayItem, UINT uCallbackMessage)
{
  ptrayItem->notifyIcon.uCallbackMessage = uCallbackMessage;
}


static void SYSTRAY_ItemSetIcon(SystrayItem *ptrayItem, HICON hIcon)
{
  if(ptrayItem->notifyIcon.hIcon)
    DestroyIcon(ptrayItem->notifyIcon.hIcon);
  ptrayItem->notifyIcon.hIcon = CopyIcon(hIcon);
  InvalidateRect(ptrayItem->hWnd, NULL, TRUE);
}


static void SYSTRAY_ItemSetTip(SystrayItem *ptrayItem, const WCHAR* szTip, int modify)
{
  TTTOOLINFOW ti;

  lstrcpynW(ptrayItem->notifyIcon.szTip, szTip, sizeof(ptrayItem->notifyIcon.szTip)/sizeof(WCHAR));

  ti.cbSize = sizeof(TTTOOLINFOW);
  ti.uFlags = 0;
  ti.hwnd = ptrayItem->hWnd;
  ti.hinst = 0;
  ti.uId = 0;
  ti.lpszText = ptrayItem->notifyIcon.szTip;
  ti.rect.left   = 0;
  ti.rect.top    = 0;
  ti.rect.right  = ICON_SIZE+2*ICON_BORDER;
  ti.rect.bottom = ICON_SIZE+2*ICON_BORDER;

  if(modify)
    SendMessageW(ptrayItem->hWndToolTip, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);
  else
    SendMessageW(ptrayItem->hWndToolTip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
}


static BOOL SYSTRAY_Add(PNOTIFYICONDATAW pnid)
{
  SystrayItem **ptrayItem = &systray;
  static const WCHAR emptyW[] = { 0 };

  /* Find last element. */
  while( *ptrayItem ) {
    if ( SYSTRAY_ItemIsEqual(pnid, &(*ptrayItem)->notifyIcon) )
      return FALSE;
    ptrayItem = &((*ptrayItem)->nextTrayItem);
  }
  /* Allocate SystrayItem for element and add to end of list. */
  (*ptrayItem) = HeapAlloc(GetProcessHeap(),0,sizeof(SystrayItem));

  /* Initialize and set data for the tray element. */
  SYSTRAY_ItemInit( (*ptrayItem) );
  (*ptrayItem)->notifyIcon.uID = pnid->uID; /* only needed for callback message */
  (*ptrayItem)->notifyIcon.hWnd = pnid->hWnd; /* only needed for callback message */
  SYSTRAY_ItemSetIcon   (*ptrayItem, (pnid->uFlags&NIF_ICON)   ?pnid->hIcon           :0);
  SYSTRAY_ItemSetMessage(*ptrayItem, (pnid->uFlags&NIF_MESSAGE)?pnid->uCallbackMessage:0);
  SYSTRAY_ItemSetTip    (*ptrayItem, (pnid->uFlags&NIF_TIP)    ?pnid->szTip           :emptyW, FALSE);

  TRACE("%p: %p %s\n",  (*ptrayItem), (*ptrayItem)->notifyIcon.hWnd,
                                          debugstr_w((*ptrayItem)->notifyIcon.szTip));
  return TRUE;
}


static BOOL SYSTRAY_Modify(PNOTIFYICONDATAW pnid)
{
  SystrayItem *ptrayItem = systray;

  while ( ptrayItem ) {
    if ( SYSTRAY_ItemIsEqual(pnid, &ptrayItem->notifyIcon) ) {
      if (pnid->uFlags & NIF_ICON)
        SYSTRAY_ItemSetIcon(ptrayItem, pnid->hIcon);
      if (pnid->uFlags & NIF_MESSAGE)
        SYSTRAY_ItemSetMessage(ptrayItem, pnid->uCallbackMessage);
      if (pnid->uFlags & NIF_TIP)
        SYSTRAY_ItemSetTip(ptrayItem, pnid->szTip, TRUE);

      TRACE("%p: %p %s\n", ptrayItem, ptrayItem->notifyIcon.hWnd, debugstr_w(ptrayItem->notifyIcon.szTip));
      return TRUE;
    }
    ptrayItem = ptrayItem->nextTrayItem;
  }
  return FALSE; /* not found */
}


/*************************************************************************
 *
 */
BOOL SYSTRAY_Init(void)
{
  return TRUE;
}

/*************************************************************************
 * Shell_NotifyIconW			[SHELL32.298]
 */
BOOL WINAPI Shell_NotifyIconW(DWORD dwMessage, PNOTIFYICONDATAW pnid )
{
  BOOL flag=FALSE;
  TRACE("enter %p %d %ld\n", pnid->hWnd, pnid->uID, dwMessage);
  switch(dwMessage) {
  case NIM_ADD:
    flag = SYSTRAY_Add(pnid);
    break;
  case NIM_MODIFY:
    flag = SYSTRAY_Modify(pnid);
    break;
  case NIM_DELETE:
    flag = SYSTRAY_Delete(pnid);
    break;
  }
  TRACE("leave %p %d %ld=%d\n", pnid->hWnd, pnid->uID, dwMessage, flag);
  return flag;
}

/*************************************************************************
 * Shell_NotifyIconA			[SHELL32.297]
 * Shell_NotifyIcon			[SHELL32.296]
 */
BOOL WINAPI Shell_NotifyIconA (DWORD dwMessage, PNOTIFYICONDATAA pnid )
{
	BOOL ret;

	PNOTIFYICONDATAW p = HeapAlloc(GetProcessHeap(),0,sizeof(NOTIFYICONDATAW));
	memcpy(p, pnid, sizeof(NOTIFYICONDATAW));
        MultiByteToWideChar( CP_ACP, 0, pnid->szTip, -1, p->szTip, sizeof(p->szTip)/sizeof(WCHAR) );
        p->szTip[sizeof(p->szTip)/sizeof(WCHAR)-1] = 0;

	ret = Shell_NotifyIconW(dwMessage, p );

	HeapFree(GetProcessHeap(),0,p);
	return ret;
}
