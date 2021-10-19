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

#include <user32.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

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

  return Ret;
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

  switch (wParam & 0xfff0)
    {
      case SC_MOVE:
      case SC_SIZE:
//      case SC_DEFAULT:
      case SC_MOUSEMENU:
      case SC_KEYMENU:
      case SC_SCREENSAVE:
      case SC_MINIMIZE:
      case SC_MAXIMIZE:
      case SC_RESTORE:
      case SC_CLOSE:
      case SC_HOTKEY:
        NtUserMessageCall( hWnd, WM_SYSCOMMAND, wParam, lParam, (ULONG_PTR)&lResult, FNID_DEFWINDOWPROC, FALSE);
        return 0;

      default:
        break;
    }

  if (ISITHOOKED(WH_CBT))
  {
     NtUserMessageCall( hWnd, WM_SYSCOMMAND, wParam, lParam, (ULONG_PTR)&lResult, FNID_DEFWINDOWPROC, FALSE);
     if (lResult) return 0;
  }

  switch (wParam & 0xfff0)
    {

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


      case SC_NEXTWINDOW:
      case SC_PREVWINDOW:
        DoAppSwitch( wParam, lParam);
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

static BOOL CALLBACK
UserSendUiUpdateMsg(HWND hwnd, LPARAM lParam)
{
    SendMessageW(hwnd, WM_UPDATEUISTATE, (WPARAM)lParam, 0);
    return TRUE;
}

/* WARNING: Redundant with /ntuser/defwnd.c!UserPaintCaption !!
   Use TWOPARAM_ROUTINE_REDRAWTITLE/REDRAWFRAME or HWNDLOCK_ROUTINE_REDRAWFRAMEANDHOOK .
 */
static void
UserPaintCaption(PWND pwnd, INT Flags)
{
  if ( pwnd->style & WS_VISIBLE && (pwnd->style & WS_CAPTION) == WS_CAPTION )
  {
     if (pwnd->state & WNDS_HASCAPTION && NtUserQueryWindow(UserHMGetHandle(pwnd), QUERY_WINDOW_FOREGROUND))
        Flags |= DC_ACTIVE;
    /* 
     * When themes are not enabled we can go on and paint the non client area.
     * However if we do that with themes enabled we will draw a classic frame.
     * This is solved by sending a themes specific message to notify the themes
     * engine that the caption needs to be redrawn 
     */
    if(gpsi->dwSRVIFlags & SRVINFO_APIHOOK)
    {
        /* 
         * This will cause uxtheme to either paint the themed caption or call
         * RealUserDrawCaption in order to draw the classic caption when themes
         * are disabled but the themes service is enabled
         */
        SendMessageW(UserHMGetHandle(pwnd), WM_NCUAHDRAWCAPTION, Flags, 0);
    }
    else
    {
        RECT rc = {0,0,0,0};
        HDC hDC = GetDCEx(UserHMGetHandle(pwnd), NULL, DCX_WINDOW|DCX_USESTYLE);
        NtUserDrawCaption(UserHMGetHandle(pwnd), hDC, &rc, DC_DRAWCAPTIONMD|Flags);
        ReleaseDC(UserHMGetHandle(pwnd), hDC);
    }
  }
  //NtUserCallTwoParam((DWORD_PTR)UserHMGetHandle(pwnd),Flags,TWOPARAM_ROUTINE_REDRAWTITLE)
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
            hIconRet = UserGetProp(UserHMGetHandle(pWnd), gpsi->atomIconProp, TRUE);
            break;
        case ICON_SMALL:
        case ICON_SMALL2:
            hIconRet = UserGetProp(UserHMGetHandle(pWnd), gpsi->atomIconSmProp, TRUE);
            break;
        default:
            break;
    }
    return (LRESULT)hIconRet;
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
        case WM_DEVICECHANGE:
            return TRUE;

        case WM_POPUPSYSTEMMENU:
        {
            /* This is an undocumented message used by the windows taskbar to
               display the system menu of windows that belong to other processes. */
            HMENU menu = GetSystemMenu(hWnd, FALSE);
            ERR("WM_POPUPSYSTEMMENU\n");
            if (menu)
            {
                SetForegroundWindow(hWnd);
                TrackPopupMenu(menu, TPM_LEFTBUTTON|TPM_RIGHTBUTTON|TPM_SYSTEM_MENU,
                               LOWORD(lParam), HIWORD(lParam), 0, hWnd, NULL);
            }
            return 0;
        }

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
           ERR("WM_NCRBUTTONUP\n");
            break;

        case WM_XBUTTONUP:
        case WM_NCXBUTTONUP:
            if (HIWORD(wParam) == XBUTTON1 || HIWORD(wParam) == XBUTTON2)
            {
               SendMessageW(hWnd, WM_APPCOMMAND, (WPARAM)hWnd,
                         MAKELPARAM(LOWORD(wParam), FAPPCOMMAND_MOUSE | HIWORD(wParam)));
            }
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
               goto GoSS;
            }
            break;
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

        case WM_COPYGLOBALDATA:
        {
            TRACE("WM_COPYGLOBALDATA hGlobal %p Size %d Flags 0x%x\n",lParam,wParam,GlobalFlags((HGLOBAL)lParam));
            return lParam;
        }

/* Move to Win32k !*/
        case WM_SHOWWINDOW:
            if (!lParam) break; // Call when it is necessary.
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_NCLBUTTONDOWN:
        case WM_NCRBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_NCLBUTTONDBLCLK:
        case WM_KEYF1:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_SYSCHAR:
        case WM_CANCELMODE:
        case WM_PAINTICON:
        case WM_PAINT:
        case WM_PRINT:
        case WM_SETICON:
        case WM_SYSCOLORCHANGE:
        case WM_NCUAHDRAWCAPTION:
        case WM_NCUAHDRAWFRAME:
        case WM_NCPAINT:
        case WM_NCACTIVATE:
        case WM_NCCALCSIZE:
        case WM_NCHITTEST:
        case WM_SYNCPAINT:
        case WM_SETREDRAW:
        case WM_CLIENTSHUTDOWN:
        case WM_GETHOTKEY:
        case WM_SETHOTKEY:
        case WM_WINDOWPOSCHANGING:
        case WM_WINDOWPOSCHANGED:
        case WM_APPCOMMAND:
        case WM_SETCURSOR:
GoSS:
        {
            LRESULT lResult;
            NtUserMessageCall( hWnd, Msg, wParam, lParam, (ULONG_PTR)&lResult, FNID_DEFWINDOWPROC, !bUnicode);
            return lResult;
        }
    }
    return 0;
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
            SIZE_T copy;

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
            {
                UserPaintCaption(Wnd, DC_TEXT);
                IntNotifyWinEvent(EVENT_OBJECT_NAMECHANGE, hWnd, OBJID_WINDOW, CHILDID_SELF, 0);
            }
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

        case WM_IME_COMPOSITION:
        if (lParam & GCS_RESULTSTR)
        {
            LONG size, i;
            unsigned char lead = 0;
            char *buf = NULL;
            HIMC himc = ImmGetContext( hWnd );

            if (himc)
            {
                if ((size = ImmGetCompositionStringA( himc, GCS_RESULTSTR, NULL, 0 )))
                {
                    if (!(buf = HeapAlloc( GetProcessHeap(), 0, size ))) size = 0;
                    else size = ImmGetCompositionStringA( himc, GCS_RESULTSTR, buf, size );
                }
                ImmReleaseContext( hWnd, himc );

                for (i = 0; i < size; i++)
                {
                    unsigned char c = buf[i];
                    if (!lead)
                    {
                        if (IsDBCSLeadByte( c ))
                            lead = c;
                        else
                            SendMessageA( hWnd, WM_IME_CHAR, c, 1 );
                    }
                    else
                    {
                        SendMessageA( hWnd, WM_IME_CHAR, MAKEWORD(c, lead), 1 );
                        lead = 0;
                    }
                }
                HeapFree( GetProcessHeap(), 0, buf );
            }
        }
        /* fall through */
        case WM_IME_STARTCOMPOSITION:
        case WM_IME_ENDCOMPOSITION:
        case WM_IME_SELECT:
        case WM_IME_NOTIFY:
        case WM_IME_CONTROL:
        {
            HWND hwndIME;

            hwndIME = ImmGetDefaultIMEWnd(hWnd);
            if (hwndIME)
                Result = SendMessageA(hwndIME, Msg, wParam, lParam);
            break;
        }

        case WM_IME_SETCONTEXT:
        {
            HWND hwndIME;

            hwndIME = ImmGetDefaultIMEWnd(hWnd);
            if (hwndIME)
                Result = ImmIsUIMessageA(hwndIME, Msg, wParam, lParam);
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
                  if (Wnd->style & WS_HSCROLL)
                     SetScrollInfo( hWnd, SB_HORZ, &si, FALSE );
                  if (Wnd->style & WS_VSCROLL)
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
                UserPaintCaption(Wnd, DC_TEXT);
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

        case WM_IME_COMPOSITION:
        if (lParam & GCS_RESULTSTR)
        {
            LONG size, i;
            WCHAR *buf = NULL;
            HIMC himc = ImmGetContext( hWnd );

            if (himc)
            {
                if ((size = ImmGetCompositionStringW( himc, GCS_RESULTSTR, NULL, 0 )))
                {
                    if (!(buf = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) size = 0;
                    else size = ImmGetCompositionStringW( himc, GCS_RESULTSTR, buf, size * sizeof(WCHAR) );
                }
                ImmReleaseContext( hWnd, himc );

                for (i = 0; i < size / sizeof(WCHAR); i++)
                    SendMessageW( hWnd, WM_IME_CHAR, buf[i], 1 );
                HeapFree( GetProcessHeap(), 0, buf );
            }
        }
        /* fall through */
        case WM_IME_STARTCOMPOSITION:
        case WM_IME_ENDCOMPOSITION:
        case WM_IME_SELECT:
        case WM_IME_NOTIFY:
        case WM_IME_CONTROL:
        {
            HWND hwndIME;

            hwndIME = ImmGetDefaultIMEWnd(hWnd);
            if (hwndIME)
                Result = SendMessageW(hwndIME, Msg, wParam, lParam);
            break;
        }

        case WM_IME_SETCONTEXT:
        {
            HWND hwndIME;

            hwndIME = ImmGetDefaultIMEWnd(hWnd);
            if (hwndIME)
                Result = ImmIsUIMessageW(hwndIME, Msg, wParam, lParam);
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
       ERR("Got exception in hooked DefWindowProcA!\n");
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
       ERR("Got exception in hooked DefWindowProcW!\n");
   }
   _SEH2_END;

   EndUserApiHook();

   return Result;
}
