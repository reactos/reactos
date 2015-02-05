/*
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/windows/defwnd.c
 * PURPOSE:         Window management
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(user32);

LRESULT DefWndNCPaint(HWND hWnd, HRGN hRgn, BOOL Active);
LRESULT DefWndNCCalcSize(HWND hWnd, BOOL CalcSizeStruct, RECT *Rect);
LRESULT DefWndNCActivate(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT DefWndNCHitTest(HWND hWnd, POINT Point);
LRESULT DefWndNCLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT DefWndNCLButtonDblClk(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT NC_HandleNCRButtonDown( HWND hwnd, WPARAM wParam, LPARAM lParam );
void FASTCALL MenuInitSysMenuPopup(HMENU Menu, DWORD Style, DWORD ClsStyle, LONG HitTest );
void MENU_EndMenu( HWND );

/* GLOBALS *******************************************************************/

static short iF10Key = 0;
static short iMenuSysKey = 0;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
DWORD
WINAPI
DECLSPEC_HOTPATCH
GetSysColor(int nIndex)
{
  if(nIndex >= 0 && nIndex < NUM_SYSCOLORS)
  {
    return gpsi->argbSystem[nIndex];
  }

  SetLastError(ERROR_INVALID_PARAMETER);
  return 0;
}

/*
 * @implemented
 */
HBRUSH
WINAPI
DECLSPEC_HOTPATCH
GetSysColorBrush(int nIndex)
{
  if(nIndex >= 0 && nIndex < NUM_SYSCOLORS)
  {
    return gpsi->ahbrSystem[nIndex];
  }

  SetLastError(ERROR_INVALID_PARAMETER);
  return NULL;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetSysColors(
  int cElements,
  CONST INT *lpaElements,
  CONST COLORREF *lpaRgbValues)
{
  return NtUserSetSysColors(cElements, lpaElements, lpaRgbValues, 0);
}

BOOL
FASTCALL
DefSetText(HWND hWnd, PCWSTR String, BOOL Ansi)
{
  BOOL Ret;
  LARGE_STRING lsString;

  if ( String )
  {
     if ( Ansi )
        RtlInitLargeAnsiString((PLARGE_ANSI_STRING)&lsString, (PCSZ)String, 0);
     else
        RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&lsString, String, 0);
  }
  Ret = NtUserDefSetText(hWnd, (String ? &lsString : NULL));

  if (Ret)
     IntNotifyWinEvent(EVENT_OBJECT_NAMECHANGE, hWnd, OBJID_WINDOW, CHILDID_SELF, 0);

  return Ret;
}

void
UserGetInsideRectNC(PWND Wnd, RECT *rect)
{
    ULONG Style;
    ULONG ExStyle;

    Style = Wnd->style;
    ExStyle = Wnd->ExStyle;

    rect->top    = rect->left = 0;
    rect->right  = Wnd->rcWindow.right - Wnd->rcWindow.left;
    rect->bottom = Wnd->rcWindow.bottom - Wnd->rcWindow.top;

    if (Style & WS_ICONIC)
    {
        return;
    }

    /* Remove frame from rectangle */
    if (UserHasThickFrameStyle(Style, ExStyle ))
    {
        InflateRect(rect, -GetSystemMetrics(SM_CXFRAME), -GetSystemMetrics(SM_CYFRAME));
    }
    else
    {
        if (UserHasDlgFrameStyle(Style, ExStyle ))
        {
            InflateRect(rect, -GetSystemMetrics(SM_CXDLGFRAME), -GetSystemMetrics(SM_CYDLGFRAME));
            /* FIXME: this isn't in NC_AdjustRect? why not? */
            if (ExStyle & WS_EX_DLGMODALFRAME)
	            InflateRect( rect, -1, 0 );
        }
        else
        {
            if (UserHasThinFrameStyle(Style, ExStyle))
            {
                InflateRect(rect, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER));
            }
        }
    }
    /* We have additional border information if the window
     * is a child (but not an MDI child) */
    if ((Style & WS_CHILD) && !(ExStyle & WS_EX_MDICHILD))
    {
       if (ExStyle & WS_EX_CLIENTEDGE)
          InflateRect (rect, -GetSystemMetrics(SM_CXEDGE), -GetSystemMetrics(SM_CYEDGE));
       if (ExStyle & WS_EX_STATICEDGE)
          InflateRect (rect, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER));
    }
}

HWND FASTCALL
IntFindChildWindowToOwner(HWND hRoot, HWND hOwner)
{
   HWND Ret;
   PWND Child, OwnerWnd, Root, Owner;

   Root = ValidateHwnd(hRoot);
   Owner = ValidateHwnd(hOwner);

   for( Child = Root->spwndChild ? DesktopPtrToUser(Root->spwndChild) : NULL;
        Child;
        Child = Child->spwndNext ? DesktopPtrToUser(Child->spwndNext) : NULL )
   {
      OwnerWnd = Child->spwndOwner ? DesktopPtrToUser(Child->spwndOwner) : NULL;
      if(!OwnerWnd)
         continue;

      if (!(Child->style & WS_POPUP) || !(Child->style & WS_VISIBLE))
         continue;

      if(OwnerWnd == Owner)
      {
         Ret = Child->head.h;
         return Ret;
      }
   }
   ERR("IDCWTO Nothing found\n");
   return NULL;
}

/***********************************************************************
 *           DefWndTrackScrollBar
 *
 * Track a mouse button press on the horizontal or vertical scroll-bar.
 */
static  VOID
DefWndTrackScrollBar(HWND Wnd, WPARAM wParam, POINT Pt)
{
  INT ScrollBar;

  if (SC_HSCROLL == (wParam & 0xfff0))
    {
      if (HTHSCROLL != (wParam & 0x0f))
        {
          return;
        }
      ScrollBar = SB_HORZ;
    }
  else  /* SC_VSCROLL */
    {
      if (HTVSCROLL != (wParam & 0x0f))
        {
          return;
        }
      ScrollBar = SB_VERT;
    }
  ScrollTrackScrollBar(Wnd, ScrollBar, Pt );
}

LRESULT WINAPI DoAppSwitch( WPARAM wParam, LPARAM lParam);

LRESULT
DefWndHandleSysCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  POINT Pt;
  LRESULT lResult;

  if (!IsWindowEnabled( hWnd )) return 0;

  if (ISITHOOKED(WH_CBT))
  {
     NtUserMessageCall( hWnd, WM_SYSCOMMAND, wParam, lParam, (ULONG_PTR)&lResult, FNID_DEFWINDOWPROC, FALSE);
     if (lResult) return 0;
  }

  switch (wParam & 0xfff0)
    {
      case SC_MOVE:
      case SC_SIZE:
	NtUserMessageCall( hWnd, WM_SYSCOMMAND, wParam, lParam, (ULONG_PTR)&lResult, FNID_DEFWINDOWPROC, FALSE);
	break;

    case SC_MINIMIZE:
        if (hWnd == GetActiveWindow())
            ShowOwnedPopups(hWnd,FALSE);
        ShowWindow( hWnd, SW_MINIMIZE );
        break;

    case SC_MAXIMIZE:
        if (IsIconic(hWnd) && hWnd == GetActiveWindow())
            ShowOwnedPopups(hWnd,TRUE);
        ShowWindow( hWnd, SW_MAXIMIZE );
        break;

    case SC_RESTORE:
        if (IsIconic(hWnd) && hWnd == GetActiveWindow())
            ShowOwnedPopups(hWnd,TRUE);
        ShowWindow( hWnd, SW_RESTORE );
        break;

      case SC_CLOSE:
        return SendMessageW(hWnd, WM_CLOSE, 0, 0);

//      case SC_DEFAULT:
      case SC_MOUSEMENU:
        {
          Pt.x = (short)LOWORD(lParam);
          Pt.y = (short)HIWORD(lParam);
          MenuTrackMouseMenuBar(hWnd, wParam & 0x000f, Pt);
        }
	break;
      case SC_KEYMENU:
        MenuTrackKbdMenuBar(hWnd, wParam, (WCHAR)lParam);
	break;
      case SC_VSCROLL:
      case SC_HSCROLL:
        {
          Pt.x = (short)LOWORD(lParam);
          Pt.y = (short)HIWORD(lParam);
          DefWndTrackScrollBar(hWnd, wParam, Pt);
        }
	break;

      case SC_TASKLIST:
        WinExec( "taskman.exe", SW_SHOWNORMAL );
        break;

      case SC_SCREENSAVE:
        NtUserMessageCall( hWnd, WM_SYSCOMMAND, wParam, lParam, (ULONG_PTR)&lResult, FNID_DEFWINDOWPROC, FALSE);
        break;

      case SC_NEXTWINDOW:
      case SC_PREVWINDOW:
        DoAppSwitch( wParam, lParam);
        break;

      case SC_HOTKEY:
        {
           HWND hwnd, hWndLastActive;
           PWND pWnd;

           hwnd = (HWND)lParam;
           pWnd = ValidateHwnd(hwnd);
           if (pWnd)
           {
              hWndLastActive = GetLastActivePopup(hwnd);
              if (hWndLastActive)
              {
                 hwnd = hWndLastActive;
                 pWnd = ValidateHwnd(hwnd);
              }
              SetForegroundWindow(hwnd);
              if (pWnd->style & WS_MINIMIZE)
              {
                 PostMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
              }
           }
        }
        break;

      default:
        FIXME("Unimplemented DefWndHandleSysCommand wParam 0x%x\n",wParam);
        break;
    }

  return(0);
}

/***********************************************************************
 *           DefWndControlColor
 *
 * Default colors for control painting.
 */
HBRUSH
DefWndControlColor(HDC hDC, UINT ctlType)
{
  if (ctlType == CTLCOLOR_SCROLLBAR)
  {
      HBRUSH hb = GetSysColorBrush(COLOR_SCROLLBAR);
      COLORREF bk = GetSysColor(COLOR_3DHILIGHT);
      SetTextColor(hDC, GetSysColor(COLOR_3DFACE));
      SetBkColor(hDC, bk);

      /* if COLOR_WINDOW happens to be the same as COLOR_3DHILIGHT
       * we better use 0x55aa bitmap brush to make scrollbar's background
       * look different from the window background.
       */
      if ( bk == GetSysColor(COLOR_WINDOW))
          return gpsi->hbrGray;

      UnrealizeObject( hb );
      return hb;
  }

  SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));

  if ((ctlType == CTLCOLOR_EDIT) || (ctlType == CTLCOLOR_LISTBOX))
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

static void DefWndPrint( HWND hwnd, HDC hdc, ULONG uFlags)
{
  /*
   * Visibility flag.
   */
  if ( (uFlags & PRF_CHECKVISIBLE) &&
       !IsWindowVisible(hwnd) )
      return;

  /*
   * Unimplemented flags.
   */
  if ( (uFlags & PRF_CHILDREN) ||
       (uFlags & PRF_OWNED)    ||
       (uFlags & PRF_NONCLIENT) )
  {
    FIXME("WM_PRINT message with unsupported flags\n");
  }

  /*
   * Background
   */
  if ( uFlags & PRF_ERASEBKGND)
    SendMessageW(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);

  /*
   * Client area
   */
  if ( uFlags & PRF_CLIENT)
    SendMessageW(hwnd, WM_PRINTCLIENT, (WPARAM)hdc, uFlags);
}

static BOOL CALLBACK
UserSendUiUpdateMsg(HWND hwnd, LPARAM lParam)
{
    SendMessageW(hwnd, WM_UPDATEUISTATE, (WPARAM)lParam, 0);
    return TRUE;
}

static void
UserPaintCaption(HWND hwnd)
{
    /* FIXME: this is not 100% correct */

    /* 
     * When themes are not enabled we can go on and paint the non client area.
     * However if we do that with themes enabled we will draw a classic frame.
     * This is sovled by sending a themes specific message to notify the themes
     * engine that the caption needs to be redrawn 
     */
    if(gpsi->dwSRVIFlags & SRVINFO_APIHOOK)
    {
        SendMessage(hwnd, WM_NCUAHDRAWCAPTION,0,0);
    }
    else
    {
        DefWndNCPaint(hwnd, HRGN_WINDOW, -1);
    }
}

// WM_SETICON
LRESULT FASTCALL
DefWndSetIcon(PWND pWnd, WPARAM wParam, LPARAM lParam)
{
    HICON hIcon, hIconSmall, hIconOld;

    if ( wParam > ICON_SMALL2 )
    {  
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    hIconSmall = UserGetProp(UserHMGetHandle(pWnd), gpsi->atomIconSmProp);
    hIcon = UserGetProp(UserHMGetHandle(pWnd), gpsi->atomIconProp);

    hIconOld = wParam == ICON_BIG ? hIcon : hIconSmall;

    switch(wParam)
    {
        case ICON_BIG:
            hIcon = (HICON)lParam;
            break;
        case ICON_SMALL:
            hIconSmall = (HICON)lParam;
            break;
        case ICON_SMALL2:
            ERR("FIXME: Set ICON_SMALL2 support!\n");
        default:
            break;
    }

    NtUserSetProp(UserHMGetHandle(pWnd), gpsi->atomIconProp, hIcon);
    NtUserSetProp(UserHMGetHandle(pWnd), gpsi->atomIconSmProp, hIconSmall);

    if ((pWnd->style & WS_CAPTION ) == WS_CAPTION)
       UserPaintCaption(UserHMGetHandle(pWnd));  /* Repaint caption */

    return (LRESULT)hIconOld;
}

LRESULT FASTCALL
DefWndGetIcon(PWND pWnd, WPARAM wParam, LPARAM lParam)
{
    HICON hIconRet;
    if ( wParam > ICON_SMALL2 )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    switch(wParam)
    {
        case ICON_BIG:
            hIconRet = UserGetProp(UserHMGetHandle(pWnd), gpsi->atomIconProp);
            break;
        case ICON_SMALL:
        case ICON_SMALL2:
            hIconRet = UserGetProp(UserHMGetHandle(pWnd), gpsi->atomIconSmProp);
            break;
        default:
            break;
    }
    return (LRESULT)hIconRet;
}

VOID FASTCALL
DefWndScreenshot(HWND hWnd)
{
    RECT rect;
    HDC hdc;
    INT w;
    INT h;
    HBITMAP hbitmap;
    HDC hdc2;

    OpenClipboard(hWnd);
    EmptyClipboard();

    hdc = GetWindowDC(hWnd);
    GetWindowRect(hWnd, &rect);
    w = rect.right - rect.left;
    h = rect.bottom - rect.top;

    hbitmap = CreateCompatibleBitmap(hdc, w, h);
    hdc2 = CreateCompatibleDC(hdc);
    SelectObject(hdc2, hbitmap);

    BitBlt(hdc2, 0, 0, w, h,
           hdc, 0, 0,
           SRCCOPY);

    SetClipboardData(CF_BITMAP, hbitmap);

    ReleaseDC(hWnd, hdc);
    ReleaseDC(hWnd, hdc2);

    CloseClipboard();
}



LRESULT WINAPI
User32DefWindowProc(HWND hWnd,
		    UINT Msg,
		    WPARAM wParam,
		    LPARAM lParam,
		    BOOL bUnicode)
{
    PWND pWnd = NULL;
    if (hWnd)
    {
       pWnd = ValidateHwnd(hWnd);
       if (!pWnd) return 0;
    }

    switch (Msg)
    {
	case WM_NCPAINT:
	{
            return DefWndNCPaint(hWnd, (HRGN)wParam, -1);
        }

        case WM_NCCALCSIZE:
        {
            return DefWndNCCalcSize(hWnd, (BOOL)wParam, (RECT*)lParam);
        }

        case WM_POPUPSYSTEMMENU:
        {
            /* This is an undocumented message used by the windows taskbar to
               display the system menu of windows that belong to other processes. */
            HMENU menu = GetSystemMenu(hWnd, FALSE);

            if (menu)
                TrackPopupMenu(menu, TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
                               LOWORD(lParam), HIWORD(lParam), 0, hWnd, NULL);
            return 0;
        }

        case WM_NCACTIVATE:
        {
            return DefWndNCActivate(hWnd, wParam, lParam);
        }

        case WM_NCHITTEST:
        {
            POINT Point;
            Point.x = GET_X_LPARAM(lParam);
            Point.y = GET_Y_LPARAM(lParam);
            return (DefWndNCHitTest(hWnd, Point));
        }

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            iF10Key = iMenuSysKey = 0;
            break;

        case WM_NCLBUTTONDOWN:
        {
            return (DefWndNCLButtonDown(hWnd, wParam, lParam));
        }

        case WM_LBUTTONDBLCLK:
            return (DefWndNCLButtonDblClk(hWnd, HTCLIENT, lParam));

        case WM_NCLBUTTONDBLCLK:
        {
            return (DefWndNCLButtonDblClk(hWnd, wParam, lParam));
        }

        case WM_NCRBUTTONDOWN:
            return NC_HandleNCRButtonDown( hWnd, wParam, lParam );

        case WM_RBUTTONUP:
        {
            POINT Pt;

            Pt.x = GET_X_LPARAM(lParam);
            Pt.y = GET_Y_LPARAM(lParam);
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

        case WM_NCRBUTTONUP:
          /*
           * FIXME : we must NOT send WM_CONTEXTMENU on a WM_NCRBUTTONUP (checked
           * in Windows), but what _should_ we do? According to MSDN :
           * "If it is appropriate to do so, the system sends the WM_SYSCOMMAND
           * message to the window". When is it appropriate?
           */
            break;

        case WM_CONTEXTMENU:
        {
            if (GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_CHILD)
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
                POINT Pt;
                LONG_PTR Style;
                LONG HitCode;

                Style = GetWindowLongPtrW(hWnd, GWL_STYLE);

                Pt.x = GET_X_LPARAM(lParam);
                Pt.y = GET_Y_LPARAM(lParam);
                if (Style & WS_CHILD)
                {
                    ScreenToClient(GetParent(hWnd), &Pt);
                }

                HitCode = DefWndNCHitTest(hWnd, Pt);

                if (HitCode == HTCAPTION || HitCode == HTSYSMENU)
                {
                    HMENU SystemMenu;
                    UINT Flags;

                    if((SystemMenu = GetSystemMenu(hWnd, FALSE)))
                    {
                      MenuInitSysMenuPopup(SystemMenu, GetWindowLongPtrW(hWnd, GWL_STYLE),
                                           GetClassLongPtrW(hWnd, GCL_STYLE), HitCode);

                      if(HitCode == HTCAPTION)
                        Flags = TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
                      else
                        Flags = TPM_LEFTBUTTON;

                      TrackPopupMenu(SystemMenu, Flags,
                                     Pt.x, Pt.y, 0, hWnd, NULL);
                    }
                }
	    }
            break;
        }

        case WM_PRINT:
        {
            DefWndPrint(hWnd, (HDC)wParam, lParam);
            return (0);
        }

        case WM_SYSCOLORCHANGE:
        {
            /* force to redraw non-client area */
            DefWndNCPaint(hWnd, HRGN_WINDOW, -1);
            /* Use InvalidateRect to redraw client area, enable
             * erase to redraw all subcontrols otherwise send the
             * WM_SYSCOLORCHANGE to child windows/controls is required
             */
            InvalidateRect(hWnd,NULL,TRUE);
            return (0);
        }

        case WM_PAINTICON:
        case WM_PAINT:
        {
            PAINTSTRUCT Ps;
            HDC hDC;

            /* If already in Paint and Client area is not empty just return. */
            if (pWnd->state2 & WNDS2_STARTPAINT && !IsRectEmpty(&pWnd->rcClient))
            {
               ERR("In Paint and Client area is not empty!\n");
               return 0;
            }

            hDC = BeginPaint(hWnd, &Ps);
            if (hDC)
            {
                HICON hIcon;

                if (IsIconic(hWnd) && ((hIcon = (HICON)GetClassLongPtrW( hWnd, GCLP_HICON))))
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

        case WM_CLOSE:
            DestroyWindow(hWnd);
            return (0);

        case WM_MOUSEACTIVATE:
            if (GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_CHILD)
            {
                LONG Ret = SendMessageW(GetParent(hWnd), WM_MOUSEACTIVATE, wParam, lParam);
                if (Ret) return (Ret);
            }
            return ( (HIWORD(lParam) == WM_LBUTTONDOWN && LOWORD(lParam) == HTCAPTION) ? MA_NOACTIVATE : MA_ACTIVATE );

        case WM_ACTIVATE:
            /* The default action in Windows is to set the keyboard focus to
             * the window, if it's being activated and not minimized */
            if (LOWORD(wParam) != WA_INACTIVE &&
                !(GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_MINIMIZE))
            {
                //ERR("WM_ACTIVATE %p\n",hWnd);
                SetFocus(hWnd);
            }
            break;

        case WM_MOUSEWHEEL:
            if (GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_CHILD)
                return SendMessageW( GetParent(hWnd), WM_MOUSEWHEEL, wParam, lParam);
            break;

        case WM_ERASEBKGND:
        case WM_ICONERASEBKGND:
        {
            RECT Rect;
            HBRUSH hBrush = (HBRUSH)GetClassLongPtrW(hWnd, GCL_HBRBACKGROUND);

            if (NULL == hBrush)
            {
                return 0;
            }
            if (GetClassLongPtrW(hWnd, GCL_STYLE) & CS_PARENTDC)
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

        case WM_CTLCOLOR:
            return (LRESULT) DefWndControlColor((HDC)wParam, HIWORD(lParam));

        case WM_SYSCOMMAND:
            return (DefWndHandleSysCommand(hWnd, wParam, lParam));

        case WM_KEYDOWN:
            if(wParam == VK_F10) iF10Key = VK_F10;
            break;

        case WM_SYSKEYDOWN:
        {
            if (HIWORD(lParam) & KF_ALTDOWN)
            {   /* Previous state, if the key was down before this message,
                   this is a cheap way to ignore autorepeat keys. */
                if ( !(HIWORD(lParam) & KF_REPEAT) )
                {
                   if ( ( wParam == VK_MENU  ||
                          wParam == VK_LMENU ||
                          wParam == VK_RMENU ) && !iMenuSysKey )
                       iMenuSysKey = 1;
                   else
                       iMenuSysKey = 0;
                }

                iF10Key = 0;

                if (wParam == VK_F4) /* Try to close the window */
                {
                   HWND top = GetAncestor(hWnd, GA_ROOT);
                   if (!(GetClassLongPtrW(top, GCL_STYLE) & CS_NOCLOSE))
                      PostMessageW(top, WM_SYSCOMMAND, SC_CLOSE, 0);
                }
                else if (wParam == VK_SNAPSHOT) // Alt-VK_SNAPSHOT?
                {
                   HWND hwnd = hWnd;
                   while (GetParent(hwnd) != NULL)
                   {
                       hwnd = GetParent(hwnd);
                   }
                   DefWndScreenshot(hwnd);
                }
                else if ( wParam == VK_ESCAPE || wParam == VK_TAB ) // Alt-Tab/ESC Alt-Shift-Tab/ESC
                {
                   WPARAM wParamTmp;
                   HWND Active = GetActiveWindow(); // Noticed MDI problem.
                   if (!Active)
                   {
                      FIXME("WM_SYSKEYDOWN VK_ESCAPE no active\n");
                      break;
                   }
                   wParamTmp = GetKeyState(VK_SHIFT) & 0x8000 ? SC_PREVWINDOW : SC_NEXTWINDOW;
                   SendMessageW( Active, WM_SYSCOMMAND, wParamTmp, wParam );
                }
            }
            else if( wParam == VK_F10 )
            {
                if (GetKeyState(VK_SHIFT) & 0x8000)
                    SendMessageW( hWnd, WM_CONTEXTMENU, (WPARAM)hWnd, MAKELPARAM(-1, -1) );
                iF10Key = 1;
            }
            break;
        }

        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
           /* Press and release F10 or ALT */
            if (((wParam == VK_MENU || wParam == VK_LMENU || wParam == VK_RMENU)
                 && iMenuSysKey) || ((wParam == VK_F10) && iF10Key))
                SendMessageW( GetAncestor( hWnd, GA_ROOT ), WM_SYSCOMMAND, SC_KEYMENU, 0L );
            iMenuSysKey = iF10Key = 0;
            break;
        }

        case WM_SYSCHAR:
        {
            iMenuSysKey = 0;
            if (wParam == VK_RETURN && IsIconic(hWnd))
            {
                PostMessageW( hWnd, WM_SYSCOMMAND, SC_RESTORE, 0L );
                break;
            }
            if ((HIWORD(lParam) & KF_ALTDOWN) && wParam)
            {
                if (wParam == VK_TAB || wParam == VK_ESCAPE) break;
                if (wParam == VK_SPACE && (GetWindowLongPtrW( hWnd, GWL_STYLE ) & WS_CHILD))
                    SendMessageW( GetParent(hWnd), Msg, wParam, lParam );
                else
                    SendMessageW( hWnd, WM_SYSCOMMAND, SC_KEYMENU, wParam );
            }
            else /* check for Ctrl-Esc */
                if (wParam != VK_ESCAPE) MessageBeep(0);
            break;
        }

        case WM_CANCELMODE:
        {
            iMenuSysKey = 0;
            /* FIXME: Check for a desktop. */
            //if (!(GetWindowLongPtrW( hWnd, GWL_STYLE ) & WS_CHILD)) EndMenu();
            MENU_EndMenu( hWnd );
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
            return DRAG_FILE;
*/
        case WM_QUERYDROPOBJECT:
        {
            if (GetWindowLongPtrW(hWnd, GWL_EXSTYLE) & WS_EX_ACCEPTFILES)
            {
                return(1);
            }
            break;
        }

        case WM_QUERYDRAGICON:
        {
            UINT Len;
            HICON hIcon;

            hIcon = (HICON)GetClassLongPtrW(hWnd, GCL_HICON);
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

        case WM_ISACTIVEICON:
        {
           BOOL isai;
           isai = (pWnd->state & WNDS_ACTIVEFRAME) != 0;
           return isai;
        }

        case WM_NOTIFYFORMAT:
        {
            if (lParam == NF_QUERY)
                return IsWindowUnicode(hWnd) ? NFR_UNICODE : NFR_ANSI;
            break;
        }

        case WM_SETICON:
        {
           return DefWndSetIcon(pWnd, wParam, lParam);
        }

        case WM_GETICON:
        {
           return DefWndGetIcon(pWnd, wParam, lParam);
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
              if(NtUserxSwitchCaretShowing(&CaretInfo) && (CaretInfo.hWnd == hWnd))
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

        case WM_INPUTLANGCHANGEREQUEST:
        {
            HKL NewHkl;

            if(wParam & INPUTLANGCHANGE_BACKWARD
               && wParam & INPUTLANGCHANGE_FORWARD)
            {
                return FALSE;
            }

            //FIXME: What to do with INPUTLANGCHANGE_SYSCHARSET ?

            if(wParam & INPUTLANGCHANGE_BACKWARD) NewHkl = (HKL) HKL_PREV;
            else if(wParam & INPUTLANGCHANGE_FORWARD) NewHkl = (HKL) HKL_NEXT;
            else NewHkl = (HKL) lParam;

            NtUserActivateKeyboardLayout(NewHkl, 0);

            return TRUE;
        }

        case WM_INPUTLANGCHANGE:
        {
            int count = 0;
            HWND *win_array = WIN_ListChildren( hWnd );

            if (!win_array)
                break;
            while (win_array[count])
                SendMessageW( win_array[count++], WM_INPUTLANGCHANGE, wParam, lParam);
            HeapFree(GetProcessHeap(),0,win_array);
            break;
        }

        case WM_QUERYUISTATE:
        {
            LRESULT Ret = 0;
            PWND Wnd = ValidateHwnd(hWnd);
            if (Wnd != NULL)
            {
                if (Wnd->HideFocus)
                    Ret |= UISF_HIDEFOCUS;
                if (Wnd->HideAccel)
                    Ret |= UISF_HIDEACCEL;
            }
            return Ret;
        }

        case WM_CHANGEUISTATE:
        {
            BOOL AlwaysShowCues = FALSE;
            WORD Action = LOWORD(wParam);
            WORD Flags = HIWORD(wParam);
            PWND Wnd;

            SystemParametersInfoW(SPI_GETKEYBOARDCUES, 0, &AlwaysShowCues, 0);
            if (AlwaysShowCues)
                break;

            Wnd= ValidateHwnd(hWnd);
            if (!Wnd || lParam != 0)
                break;

            if (Flags & ~(UISF_HIDEFOCUS | UISF_HIDEACCEL | UISF_ACTIVE))
                break;

            if (Flags & UISF_ACTIVE)
            {
                WARN("WM_CHANGEUISTATE does not yet support UISF_ACTIVE!\n");
            }

            if (Action == UIS_INITIALIZE)
            {
                PDESKTOPINFO Desk = GetThreadDesktopInfo();
                if (Desk == NULL)
                    break;

                Action = Desk->LastInputWasKbd ? UIS_CLEAR : UIS_SET;
                Flags = UISF_HIDEFOCUS | UISF_HIDEACCEL;

                /* We need to update wParam in case we need to send out messages */
                wParam = MAKEWPARAM(Action, Flags);
            }

            switch (Action)
            {
                case UIS_SET:
                    /* See if we actually need to change something */
                    if ((Flags & UISF_HIDEFOCUS) && !Wnd->HideFocus)
                        break;
                    if ((Flags & UISF_HIDEACCEL) && !Wnd->HideAccel)
                        break;

                    /* Don't need to do anything... */
                    return 0;

                case UIS_CLEAR:
                    /* See if we actually need to change something */
                    if ((Flags & UISF_HIDEFOCUS) && Wnd->HideFocus)
                        break;
                    if ((Flags & UISF_HIDEACCEL) && Wnd->HideAccel)
                        break;

                    /* Don't need to do anything... */
                    return 0;

                default:
                    WARN("WM_CHANGEUISTATE: Unsupported Action 0x%x\n", Action);
                    break;
            }

            if ((Wnd->style & WS_CHILD) && Wnd->spwndParent != NULL)
            {
                /* We're a child window and we need to pass this message down until
                   we reach the root */
                hWnd = UserHMGetHandle((PWND)DesktopPtrToUser(Wnd->spwndParent));
            }
            else
            {
                /* We're a top level window, we need to change the UI state */
                Msg = WM_UPDATEUISTATE;
            }

            if (bUnicode)
                return SendMessageW(hWnd, Msg, wParam, lParam);
            else
                return SendMessageA(hWnd, Msg, wParam, lParam);
        }

        case WM_UPDATEUISTATE:
        {
            BOOL Change = TRUE;
            BOOL AlwaysShowCues = FALSE;
            WORD Action = LOWORD(wParam);
            WORD Flags = HIWORD(wParam);
            PWND Wnd;

            SystemParametersInfoW(SPI_GETKEYBOARDCUES, 0, &AlwaysShowCues, 0);
            if (AlwaysShowCues)
                break;

            Wnd = ValidateHwnd(hWnd);
            if (!Wnd || lParam != 0)
                break;

            if (Flags & ~(UISF_HIDEFOCUS | UISF_HIDEACCEL | UISF_ACTIVE))
                break;

            if (Flags & UISF_ACTIVE)
            {
                WARN("WM_UPDATEUISTATE does not yet support UISF_ACTIVE!\n");
            }

            if (Action == UIS_INITIALIZE)
            {
                PDESKTOPINFO Desk = GetThreadDesktopInfo();
                if (Desk == NULL)
                    break;

                Action = Desk->LastInputWasKbd ? UIS_CLEAR : UIS_SET;
                Flags = UISF_HIDEFOCUS | UISF_HIDEACCEL;

                /* We need to update wParam for broadcasting the update */
                wParam = MAKEWPARAM(Action, Flags);
            }

            switch (Action)
            {
                case UIS_SET:
                    /* See if we actually need to change something */
                    if ((Flags & UISF_HIDEFOCUS) && !Wnd->HideFocus)
                        break;
                    if ((Flags & UISF_HIDEACCEL) && !Wnd->HideAccel)
                        break;

                    /* Don't need to do anything... */
                    Change = FALSE;
                    break;

                case UIS_CLEAR:
                    /* See if we actually need to change something */
                    if ((Flags & UISF_HIDEFOCUS) && Wnd->HideFocus)
                        break;
                    if ((Flags & UISF_HIDEACCEL) && Wnd->HideAccel)
                        break;

                    /* Don't need to do anything... */
                    Change = FALSE;
                    break;

                default:
                    WARN("WM_UPDATEUISTATE: Unsupported Action 0x%x\n", Action);
                    return 0;
            }

            /* Pack the information and call win32k */
            if (Change)
            {
                if (!NtUserxUpdateUiState(hWnd, Flags | ((DWORD)Action << 3)))
                    break;
            }

            /* Always broadcast the update to all children */
            EnumChildWindows(hWnd,
                             UserSendUiUpdateMsg,
                             (LPARAM)wParam);

            break;
        }

/* Move to Win32k !*/
        case WM_SHOWWINDOW:
            if (!lParam) break; // Call when it is necessary.
        case WM_SYNCPAINT:
        case WM_SETREDRAW:
        case WM_CLIENTSHUTDOWN:
        case WM_GETHOTKEY:
        case WM_SETHOTKEY:
        case WM_WINDOWPOSCHANGING:
        case WM_WINDOWPOSCHANGED:
        case WM_APPCOMMAND:
        {
            LRESULT lResult;
            NtUserMessageCall( hWnd, Msg, wParam, lParam, (ULONG_PTR)&lResult, FNID_DEFWINDOWPROC, !bUnicode);
            return lResult;
        }
    }
    return 0;
}


/*
 * helpers for calling IMM32 (from Wine 10/22/2008)
 *
 * WM_IME_* messages are generated only by IMM32,
 * so I assume imm32 is already LoadLibrary-ed.
 */
static HWND
DefWndImmGetDefaultIMEWnd(HWND hwnd)
{
    HINSTANCE hInstIMM = GetModuleHandleW(L"imm32\0");
    HWND (WINAPI *pFunc)(HWND);
    HWND hwndRet = 0;

    if (!hInstIMM)
    {
        ERR("cannot get IMM32 handle\n");
        return 0;
    }

    pFunc = (void*) GetProcAddress(hInstIMM, "ImmGetDefaultIMEWnd");
    if (pFunc != NULL)
        hwndRet = (*pFunc)(hwnd);

    return hwndRet;
}


static BOOL
DefWndImmIsUIMessageA(HWND hwndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HINSTANCE hInstIMM = GetModuleHandleW(L"imm32\0");
    BOOL (WINAPI *pFunc)(HWND,UINT,WPARAM,LPARAM);
    BOOL fRet = FALSE;

    if (!hInstIMM)
    {
        ERR("cannot get IMM32 handle\n");
        return FALSE;
    }

    pFunc = (void*) GetProcAddress(hInstIMM, "ImmIsUIMessageA");
    if (pFunc != NULL)
        fRet = (*pFunc)(hwndIME, msg, wParam, lParam);

    return fRet;
}


static BOOL
DefWndImmIsUIMessageW(HWND hwndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HINSTANCE hInstIMM = GetModuleHandleW(L"imm32\0");
    BOOL (WINAPI *pFunc)(HWND,UINT,WPARAM,LPARAM);
    BOOL fRet = FALSE;

    if (!hInstIMM)
    {
        ERR("cannot get IMM32 handle\n");
        return FALSE;
    }

    pFunc = (void*) GetProcAddress(hInstIMM, "ImmIsUIMessageW");
    if (pFunc != NULL)
        fRet = (*pFunc)(hwndIME, msg, wParam, lParam);

    return fRet;
}


LRESULT WINAPI
RealDefWindowProcA(HWND hWnd,
                   UINT Msg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    LRESULT Result = 0;
    PWND Wnd;

    Wnd = ValidateHwnd(hWnd);

    if ( !Wnd &&
         Msg != WM_CTLCOLORMSGBOX &&
         Msg != WM_CTLCOLORBTN    &&
         Msg != WM_CTLCOLORDLG    &&
         Msg != WM_CTLCOLORSTATIC )
       return 0;

    SPY_EnterMessage(SPY_DEFWNDPROC, hWnd, Msg, wParam, lParam);
    switch (Msg)
    {
        case WM_NCCREATE:
        {
            if ( Wnd &&
                 Wnd->style & (WS_HSCROLL | WS_VSCROLL) )
            {
               if (!Wnd->pSBInfo)
               {
                  SCROLLINFO si = {sizeof si, SIF_ALL, 0, 100, 0, 0, 0};
                  SetScrollInfo( hWnd, SB_HORZ, &si, FALSE );
                  SetScrollInfo( hWnd, SB_VERT, &si, FALSE );
               }
            }

            if (lParam)
            {
                LPCREATESTRUCTA cs = (LPCREATESTRUCTA)lParam;
                /* check for string, as static icons, bitmaps (SS_ICON, SS_BITMAP)
                 * may have child window IDs instead of window name */
                if (HIWORD(cs->lpszName))
                {
                    DefSetText(hWnd, (PCWSTR)cs->lpszName, TRUE);
                }
                Result = 1;
            }
            break;
        }

        case WM_GETTEXTLENGTH:
        {
            PWSTR buf;
            ULONG len;

            if (Wnd != NULL && Wnd->strName.Length != 0)
            {
                buf = DesktopPtrToUser(Wnd->strName.Buffer);
                if (buf != NULL &&
                    NT_SUCCESS(RtlUnicodeToMultiByteSize(&len,
                                                         buf,
                                                         Wnd->strName.Length)))
                {
                    Result = (LRESULT) len;
                }
            }
            else Result = 0L;

            break;
        }

        case WM_GETTEXT:
        {
            PWSTR buf = NULL;
            PSTR outbuf = (PSTR)lParam;
            UINT copy;

            if (Wnd != NULL && wParam != 0)
            {
                if (Wnd->strName.Buffer != NULL)
                    buf = DesktopPtrToUser(Wnd->strName.Buffer);
                else
                    outbuf[0] = L'\0';

                if (buf != NULL)
                {
                    if (Wnd->strName.Length != 0)
                    {
                        copy = min(Wnd->strName.Length / sizeof(WCHAR), wParam - 1);
                        Result = WideCharToMultiByte(CP_ACP,
                                                     0,
                                                     buf,
                                                     copy,
                                                     outbuf,
                                                     wParam,
                                                     NULL,
                                                     NULL);
                        outbuf[Result] = '\0';
                    }
                    else
                        outbuf[0] = '\0';
                }
            }
            break;
        }

        case WM_SETTEXT:
        {
            DefSetText(hWnd, (PCWSTR)lParam, TRUE);

            if ((GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_CAPTION) == WS_CAPTION)
                UserPaintCaption(hWnd);
            Result = 1;
            break;
        }

        case WM_IME_KEYDOWN:
        {
            Result = PostMessageA(hWnd, WM_KEYDOWN, wParam, lParam);
            break;
        }

        case WM_IME_KEYUP:
        {
            Result = PostMessageA(hWnd, WM_KEYUP, wParam, lParam);
            break;
        }

        case WM_IME_CHAR:
        {
            if (HIBYTE(wParam))
                PostMessageA(hWnd, WM_CHAR, HIBYTE(wParam), lParam);
            PostMessageA(hWnd, WM_CHAR, LOBYTE(wParam), lParam);
            break;
        }

        case WM_IME_STARTCOMPOSITION:
        case WM_IME_COMPOSITION:
        case WM_IME_ENDCOMPOSITION:
        case WM_IME_SELECT:
        case WM_IME_NOTIFY:
        case WM_IME_CONTROL:
        {
            HWND hwndIME;

            hwndIME = DefWndImmGetDefaultIMEWnd(hWnd);
            if (hwndIME)
                Result = SendMessageA(hwndIME, Msg, wParam, lParam);
            break;
        }

        case WM_IME_SETCONTEXT:
        {
            HWND hwndIME;

            hwndIME = DefWndImmGetDefaultIMEWnd(hWnd);
            if (hwndIME)
                Result = DefWndImmIsUIMessageA(hwndIME, Msg, wParam, lParam);
            break;
        }

        /* fall through */
        default:
            Result = User32DefWindowProc(hWnd, Msg, wParam, lParam, FALSE);
    }

    SPY_ExitMessage(SPY_RESULT_DEFWND, hWnd, Msg, Result, wParam, lParam);
    return Result;
}


LRESULT WINAPI
RealDefWindowProcW(HWND hWnd,
                   UINT Msg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    LRESULT Result = 0;
    PWND Wnd;

    Wnd = ValidateHwnd(hWnd);

    if ( !Wnd &&
         Msg != WM_CTLCOLORMSGBOX &&
         Msg != WM_CTLCOLORBTN    &&
         Msg != WM_CTLCOLORDLG    &&
         Msg != WM_CTLCOLORSTATIC )
       return 0;

    SPY_EnterMessage(SPY_DEFWNDPROC, hWnd, Msg, wParam, lParam);
    switch (Msg)
    {
        case WM_NCCREATE:
        {
            if ( Wnd &&
                 Wnd->style & (WS_HSCROLL | WS_VSCROLL) )
            {
               if (!Wnd->pSBInfo)
               {
                  SCROLLINFO si = {sizeof si, SIF_ALL, 0, 100, 0, 0, 0};
                  SetScrollInfo( hWnd, SB_HORZ, &si, FALSE );
                  SetScrollInfo( hWnd, SB_VERT, &si, FALSE );
               }
            }

            if (lParam)
            {
                LPCREATESTRUCTW cs = (LPCREATESTRUCTW)lParam;
                /* check for string, as static icons, bitmaps (SS_ICON, SS_BITMAP)
                 * may have child window IDs instead of window name */
                if (HIWORD(cs->lpszName))
                {
                    DefSetText(hWnd, cs->lpszName, FALSE);
                }
                Result = 1;
            }
            break;
        }

        case WM_GETTEXTLENGTH:
        {
            PWSTR buf;
            ULONG len;

            if (Wnd != NULL && Wnd->strName.Length != 0)
            {
                buf = DesktopPtrToUser(Wnd->strName.Buffer);
                if (buf != NULL &&
                    NT_SUCCESS(RtlUnicodeToMultiByteSize(&len,
                                                         buf,
                                                         Wnd->strName.Length)))
                {
                    Result = (LRESULT) (Wnd->strName.Length / sizeof(WCHAR));
                }
            }
            else Result = 0L;

            break;
        }

        case WM_GETTEXT:
        {
            PWSTR buf = NULL;
            PWSTR outbuf = (PWSTR)lParam;

            if (Wnd != NULL && wParam != 0)
            {
                if (Wnd->strName.Buffer != NULL)
                    buf = DesktopPtrToUser(Wnd->strName.Buffer);
                else
                    outbuf[0] = L'\0';

                if (buf != NULL)
                {
                    if (Wnd->strName.Length != 0)
                    {
                        Result = min(Wnd->strName.Length / sizeof(WCHAR), wParam - 1);
                        RtlCopyMemory(outbuf,
                                      buf,
                                      Result * sizeof(WCHAR));
                        outbuf[Result] = L'\0';
                    }
                    else
                        outbuf[0] = L'\0';
                }
            }
            break;
        }

        case WM_SETTEXT:
        {
            DefSetText(hWnd, (PCWSTR)lParam, FALSE);

            if ((GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_CAPTION) == WS_CAPTION)
                UserPaintCaption(hWnd);
            Result = 1;
            break;
        }

        case WM_IME_CHAR:
        {
            PostMessageW(hWnd, WM_CHAR, wParam, lParam);
            Result = 0;
            break;
        }

        case WM_IME_KEYDOWN:
        {
            Result = PostMessageW(hWnd, WM_KEYDOWN, wParam, lParam);
            break;
        }

        case WM_IME_KEYUP:
        {
            Result = PostMessageW(hWnd, WM_KEYUP, wParam, lParam);
            break;
        }

        case WM_IME_STARTCOMPOSITION:
        case WM_IME_COMPOSITION:
        case WM_IME_ENDCOMPOSITION:
        case WM_IME_SELECT:
        case WM_IME_NOTIFY:
        case WM_IME_CONTROL:
        {
            HWND hwndIME;

            hwndIME = DefWndImmGetDefaultIMEWnd(hWnd);
            if (hwndIME)
                Result = SendMessageW(hwndIME, Msg, wParam, lParam);
            break;
        }

        case WM_IME_SETCONTEXT:
        {
            HWND hwndIME;

            hwndIME = DefWndImmGetDefaultIMEWnd(hWnd);
            if (hwndIME)
                Result = DefWndImmIsUIMessageW(hwndIME, Msg, wParam, lParam);
            break;
        }

        default:
            Result = User32DefWindowProc(hWnd, Msg, wParam, lParam, TRUE);
    }
    SPY_ExitMessage(SPY_RESULT_DEFWND, hWnd, Msg, Result, wParam, lParam);

    return Result;
}

LRESULT WINAPI
DefWindowProcA(HWND hWnd,
	       UINT Msg,
	       WPARAM wParam,
	       LPARAM lParam)
{
   BOOL Hook, msgOverride = FALSE;
   LRESULT Result = 0;

   LoadUserApiHook();

   Hook = BeginIfHookedUserApiHook();
   if (Hook)
   {
      msgOverride = IsMsgOverride(Msg, &guah.DefWndProcArray);
      if(msgOverride == FALSE)
      {
          EndUserApiHook();
      }
   }

   /* Bypass SEH and go direct. */
   if (!Hook || !msgOverride)
      return RealDefWindowProcA(hWnd, Msg, wParam, lParam);

   _SEH2_TRY
   {
      Result = guah.DefWindowProcA(hWnd, Msg, wParam, lParam);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
   }
   _SEH2_END;

   EndUserApiHook();

   return Result;
}

LRESULT WINAPI
DefWindowProcW(HWND hWnd,
	       UINT Msg,
	       WPARAM wParam,
	       LPARAM lParam)
{
   BOOL Hook, msgOverride = FALSE;
   LRESULT Result = 0;

   LoadUserApiHook();

   Hook = BeginIfHookedUserApiHook();
   if (Hook)
   {
      msgOverride = IsMsgOverride(Msg, &guah.DefWndProcArray);
      if(msgOverride == FALSE)
      {
          EndUserApiHook();
      }
   }

   /* Bypass SEH and go direct. */
   if (!Hook || !msgOverride)
      return RealDefWindowProcW(hWnd, Msg, wParam, lParam);

   _SEH2_TRY
   {
      Result = guah.DefWindowProcW(hWnd, Msg, wParam, lParam);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
   }
   _SEH2_END;

   EndUserApiHook();

   return Result;
}
