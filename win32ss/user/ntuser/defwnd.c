/*
 * PROJECT:     ReactOS Win32k subsystem
 * LICENSE:     See COPYING in the top level directory
 * PURPOSE:     Miscellaneous User functions
 * COPYRIGHT:   2008-2020 James Tabor <james.tabor@reactos.org>
 */

#include <win32k.h>
#include <windowsx.h>

DBG_DEFAULT_CHANNEL(UserDefwnd);

INT WINAPI DrawTextExWorker( HDC hdc, LPWSTR str, INT i_count,
                        LPRECT rect, UINT flags, LPDRAWTEXTPARAMS dtp );

INT WINAPI DrawTextW( HDC hdc, LPCWSTR str, INT count, LPRECT rect, UINT flags )
{
    DRAWTEXTPARAMS dtp;

    memset (&dtp, 0, sizeof(dtp));
    dtp.cbSize = sizeof(dtp);
    if (flags & DT_TABSTOP)
    {
        dtp.iTabLength = (flags >> 8) & 0xff;
        flags &= 0xffff00ff;
    }
    return DrawTextExWorker(hdc, (LPWSTR)str, count, rect, flags, &dtp);
}


HBRUSH FASTCALL
DefWndControlColor(HDC hDC, UINT ctlType)
{
  if (ctlType == CTLCOLOR_SCROLLBAR)
  {
      HBRUSH hb = IntGetSysColorBrush(COLOR_SCROLLBAR);
      COLORREF bk = IntGetSysColor(COLOR_3DHILIGHT);
      IntGdiSetTextColor(hDC, IntGetSysColor(COLOR_3DFACE));
      IntGdiSetBkColor(hDC, bk);

      /* if COLOR_WINDOW happens to be the same as COLOR_3DHILIGHT
       * we better use 0x55aa bitmap brush to make scrollbar's background
       * look different from the window background.
       */
      if ( bk == IntGetSysColor(COLOR_WINDOW))
          return gpsi->hbrGray;

      NtGdiUnrealizeObject( hb );
      return hb;
  }

  IntGdiSetTextColor(hDC, IntGetSysColor(COLOR_WINDOWTEXT));

  if ((ctlType == CTLCOLOR_EDIT) || (ctlType == CTLCOLOR_LISTBOX))
  {
      IntGdiSetBkColor(hDC, IntGetSysColor(COLOR_WINDOW));
  }
  else
  {
      IntGdiSetBkColor(hDC, IntGetSysColor(COLOR_3DFACE));
      return IntGetSysColorBrush(COLOR_3DFACE);
  }

  return IntGetSysColorBrush(COLOR_WINDOW);
}

LRESULT FASTCALL
DefWndHandleWindowPosChanging(PWND pWnd, WINDOWPOS* Pos)
{
    POINT maxTrack, minTrack;
    LONG style = pWnd->style;

    if (Pos->flags & SWP_NOSIZE) return 0;
    if ((style & WS_THICKFRAME) || ((style & (WS_POPUP | WS_CHILD)) == 0))
    {
        co_WinPosGetMinMaxInfo(pWnd, NULL, NULL, &minTrack, &maxTrack);
        Pos->cx = min(Pos->cx, maxTrack.x);
        Pos->cy = min(Pos->cy, maxTrack.y);
        if (!(style & WS_MINIMIZE))
        {
            if (Pos->cx < minTrack.x) Pos->cx = minTrack.x;
            if (Pos->cy < minTrack.y) Pos->cy = minTrack.y;
        }
    }
    else
    {
        Pos->cx = max(Pos->cx, 0);
        Pos->cy = max(Pos->cy, 0);
    }
    return 0;
}

/* Win: xxxHandleWindowPosChanged */
LRESULT FASTCALL
DefWndHandleWindowPosChanged(PWND pWnd, WINDOWPOS* Pos)
{
   RECT Rect;
   LONG style = pWnd->style;

   IntGetClientRect(pWnd, &Rect);
   IntMapWindowPoints(pWnd, (style & WS_CHILD ? IntGetParent(pWnd) : NULL), (LPPOINT) &Rect, 2);

   if (!(Pos->flags & SWP_NOCLIENTMOVE))
   {
      co_IntSendMessage(UserHMGetHandle(pWnd), WM_MOVE, 0, MAKELONG(Rect.left, Rect.top));
   }

   if (!(Pos->flags & SWP_NOCLIENTSIZE) || (Pos->flags & SWP_STATECHANGED))
   {
      if (style & WS_MINIMIZE) co_IntSendMessage(UserHMGetHandle(pWnd), WM_SIZE, SIZE_MINIMIZED, 0 );
      else
      {
         WPARAM wp = (style & WS_MAXIMIZE) ? SIZE_MAXIMIZED : SIZE_RESTORED;
         co_IntSendMessage(UserHMGetHandle(pWnd), WM_SIZE, wp, MAKELONG(Rect.right - Rect.left, Rect.bottom - Rect.top));
      }
   }
   return 0;
}

//
// Handle a WM_SYSCOMMAND message. Called from DefWindowProc().
//
// Win: xxxSysCommand
LRESULT FASTCALL
DefWndHandleSysCommand(PWND pWnd, WPARAM wParam, LPARAM lParam)
{
   LRESULT lResult = 0;
   BOOL Hook = FALSE;

   if (ISITHOOKED(WH_CBT) || (pWnd->head.rpdesk->pDeskInfo->fsHooks & HOOKID_TO_FLAG(WH_CBT)))
   {
      Hook = TRUE;
      lResult = co_HOOK_CallHooks(WH_CBT, HCBT_SYSCOMMAND, wParam, lParam);

      if (lResult) return lResult;
   }

   switch (wParam & 0xfff0)
   {
      case SC_MOVE:
      case SC_SIZE:
        DefWndDoSizeMove(pWnd, wParam);
        break;

      case SC_MINIMIZE:
        if (UserHMGetHandle(pWnd) == UserGetActiveWindow())
            IntShowOwnedPopups(pWnd,FALSE); // This is done in ShowWindow! Need to retest!
        co_WinPosShowWindow( pWnd, SW_MINIMIZE );
        break;

      case SC_MAXIMIZE:
        if (((pWnd->style & WS_MINIMIZE) != 0) && UserHMGetHandle(pWnd) == UserGetActiveWindow())
            IntShowOwnedPopups(pWnd,TRUE);
        co_WinPosShowWindow( pWnd, SW_MAXIMIZE );
        break;

      case SC_RESTORE:
        if (((pWnd->style & WS_MINIMIZE) != 0) && UserHMGetHandle(pWnd) == UserGetActiveWindow())
            IntShowOwnedPopups(pWnd,TRUE);
        co_WinPosShowWindow( pWnd, SW_RESTORE );
        break;

      case SC_CLOSE:
        return co_IntSendMessage(UserHMGetHandle(pWnd), WM_CLOSE, 0, 0);

      case SC_SCREENSAVE:
        ERR("Screensaver Called!\n");
        UserPostMessage(hwndSAS, WM_LOGONNOTIFY, LN_START_SCREENSAVE, 0); // always lParam 0 == not Secure
        break;

      case SC_HOTKEY:
        {
           USER_REFERENCE_ENTRY Ref;

           pWnd = ValidateHwndNoErr((HWND)lParam);
           if (pWnd)
           {
              if (pWnd->spwndLastActive)
              {
                 pWnd = pWnd->spwndLastActive;
              }
              UserRefObjectCo(pWnd, &Ref);
              co_IntSetForegroundWindow(pWnd);
              UserDerefObjectCo(pWnd);
              if (pWnd->style & WS_MINIMIZE)
              {
                 UserPostMessage(UserHMGetHandle(pWnd), WM_SYSCOMMAND, SC_RESTORE, 0);
              }
           }
        }
        break;

//      case SC_DEFAULT:
      case SC_MOUSEMENU:
        {
          POINT Pt;
          Pt.x = (short)LOWORD(lParam);
          Pt.y = (short)HIWORD(lParam);
          MENU_TrackMouseMenuBar(pWnd, wParam & 0x000f, Pt);
        }
        break;

      case SC_KEYMENU:
        MENU_TrackKbdMenuBar(pWnd, wParam, (WCHAR)lParam);
        break;

      default:
        // We do not support anything else here so we should return normal even when sending a hook.
        return 0;
   }

   return(Hook ? 1 : 0); // Don't call us again from user space.
}

PWND FASTCALL
co_IntFindChildWindowToOwner(PWND Root, PWND Owner)
{
   PWND Ret;
   PWND Child, OwnerWnd;

   for(Child = Root->spwndChild; Child; Child = Child->spwndNext)
   {
      OwnerWnd = Child->spwndOwner;
      if(!OwnerWnd)
         continue;

      if (!(Child->style & WS_POPUP) ||
          !(Child->style & WS_VISIBLE) ||
          /* Fixes CMD pop up properties window from having foreground. */
           Owner->head.pti->MessageQueue != Child->head.pti->MessageQueue)
         continue;

      if(OwnerWnd == Owner)
      {
         Ret = Child;
         return Ret;
      }
   }
   return NULL;
}

LRESULT
DefWndHandleSetCursor(PWND pWnd, WPARAM wParam, LPARAM lParam)
{
   PWND pwndPopUP = NULL;
   WORD Msg = HIWORD(lParam);

   /* Not for child windows. */
   if (UserHMGetHandle(pWnd) != (HWND)wParam)
   {
      return FALSE;
   }

   switch((short)LOWORD(lParam))
   {
      case HTERROR:
      {
         //// This is the real fix for CORE-6129! This was a "Code hole".
         USER_REFERENCE_ENTRY Ref;

         if (Msg == WM_LBUTTONDOWN)
         {
            // Find a pop up window to bring active.
            pwndPopUP = co_IntFindChildWindowToOwner(UserGetDesktopWindow(), pWnd);
            if (pwndPopUP)
            {
               // Not a child pop up from desktop.
               if ( pwndPopUP != UserGetDesktopWindow()->spwndChild )
               {
                  // Get original active window.
                  PWND pwndOrigActive = gpqForeground->spwndActive;

                  co_WinPosSetWindowPos(pWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

                  UserRefObjectCo(pwndPopUP, &Ref);
                  //UserSetActiveWindow(pwndPopUP);
                  co_IntSetForegroundWindow(pwndPopUP); // HACK
                  UserDerefObjectCo(pwndPopUP);

                  // If the change was made, break out.
                  if (pwndOrigActive != gpqForeground->spwndActive)
                     break;
               }
            }
         }
         ////
         if (Msg == WM_LBUTTONDOWN || Msg == WM_MBUTTONDOWN ||
             Msg == WM_RBUTTONDOWN || Msg == WM_XBUTTONDOWN)
         {
             if (pwndPopUP)
             {
                 FLASHWINFO fwi =
                    {sizeof(FLASHWINFO),
                     UserHMGetHandle(pwndPopUP),
                     FLASHW_ALL,
                     gspv.dwForegroundFlashCount,
                     (gpsi->dtCaretBlink >> 3)};

                 // Now shake that window!
                 IntFlashWindowEx(pwndPopUP, &fwi);
             }
             UserPostMessage(hwndSAS, WM_LOGONNOTIFY, LN_MESSAGE_BEEP, 0);
         }
         break;
      }

      case HTCLIENT:
      {
         if (pWnd->pcls->spcur)
         {
            IntSystemSetCursor(pWnd->pcls->spcur);
         }
         return FALSE;
      }

      case HTLEFT:
      case HTRIGHT:
      {
         if (pWnd->style & WS_MAXIMIZE)
         {
            break;
         }
         IntSystemSetCursor(SYSTEMCUR(SIZEWE));
         return TRUE;
      }

      case HTTOP:
      case HTBOTTOM:
      {
         if (pWnd->style & WS_MAXIMIZE)
         {
            break;
         }
         IntSystemSetCursor(SYSTEMCUR(SIZENS));
         return TRUE;
       }

       case HTTOPLEFT:
       case HTBOTTOMRIGHT:
       {
         if (pWnd->style & WS_MAXIMIZE)
         {
            break;
         }
         IntSystemSetCursor(SYSTEMCUR(SIZENWSE));
         return TRUE;
       }

       case HTBOTTOMLEFT:
       case HTTOPRIGHT:
       {
         if (pWnd->style & WS_MAXIMIZE)
         {
            break;
         }
         IntSystemSetCursor(SYSTEMCUR(SIZENESW));
         return TRUE;
       }
   }
   IntSystemSetCursor(SYSTEMCUR(ARROW));
   return FALSE;
}

/* Win: xxxDWPPrint */
VOID FASTCALL DefWndPrint( PWND pwnd, HDC hdc, ULONG uFlags)
{
  /*
   * Visibility flag.
   */
  if ( (uFlags & PRF_CHECKVISIBLE) &&
       !IntIsWindowVisible(pwnd) )
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
    co_IntSendMessage(UserHMGetHandle(pwnd), WM_ERASEBKGND, (WPARAM)hdc, 0);

  /*
   * Client area
   */
  if ( uFlags & PRF_CLIENT)
    co_IntSendMessage(UserHMGetHandle(pwnd), WM_PRINTCLIENT, (WPARAM)hdc, uFlags);
}

BOOL
UserPaintCaption(PWND pWnd, INT Flags)
{
  BOOL Ret = FALSE;

  if ( (pWnd->style & WS_VISIBLE) && ((pWnd->style & WS_CAPTION) == WS_CAPTION) )
  {
      if (pWnd->state & WNDS_HASCAPTION && pWnd->head.pti->MessageQueue == gpqForeground)
         Flags |= DC_ACTIVE;
    /*
     * When themes are not enabled we can go on and paint the non client area.
     * However if we do that with themes enabled we will draw a classic frame.
     * This is solved by sending a themes specific message to notify the themes
     * engine that the caption needs to be redrawn.
     */
      if (gpsi->dwSRVIFlags & SRVINFO_APIHOOK)
      {
        /*
         * This will cause uxtheme to either paint the themed caption or call
         * RealUserDrawCaption in order to draw the classic caption when themes
         * are disabled but the themes service is enabled.
         */
         TRACE("UDCB Flags %08x\n", Flags);
         co_IntSendMessage(UserHMGetHandle(pWnd), WM_NCUAHDRAWCAPTION, Flags, 0);
      }
      else
      {
         HDC hDC = UserGetDCEx(pWnd, NULL, DCX_WINDOW|DCX_USESTYLE);
         UserDrawCaptionBar(pWnd, hDC, Flags | DC_FRAME); // DCFRAME added as fix for CORE-10855.
         UserReleaseDC(pWnd, hDC, FALSE);
      }
      Ret = TRUE;
   }
   // Support window tray
   return Ret;
}

// WM_SETICON
/* Win: xxxDWP_SetIcon */
LRESULT FASTCALL
DefWndSetIcon(PWND pWnd, WPARAM wParam, LPARAM lParam)
{
    HICON hIcon, hIconSmall, hIconOld;

    if ( wParam > ICON_SMALL2 )
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    hIconSmall = UserGetProp(pWnd, gpsi->atomIconSmProp, TRUE);
    hIcon      = UserGetProp(pWnd, gpsi->atomIconProp, TRUE);

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

    UserSetProp(pWnd, gpsi->atomIconProp, hIcon, TRUE);
    UserSetProp(pWnd, gpsi->atomIconSmProp, hIconSmall, TRUE);

    if ((pWnd->style & WS_CAPTION ) == WS_CAPTION)
       UserPaintCaption(pWnd, DC_ICON);

    return (LRESULT)hIconOld;
}

/* Win: DWP_GetIcon */
LRESULT FASTCALL
DefWndGetIcon(PWND pWnd, WPARAM wParam, LPARAM lParam)
{
    HICON hIconRet;
    if ( wParam > ICON_SMALL2 )
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    switch(wParam)
    {
        case ICON_BIG:
            hIconRet = UserGetProp(pWnd, gpsi->atomIconProp, TRUE);
            break;
        case ICON_SMALL:
        case ICON_SMALL2:
            hIconRet = UserGetProp(pWnd, gpsi->atomIconSmProp, TRUE);
            break;
        DEFAULT_UNREACHABLE;
    }
    return (LRESULT)hIconRet;
}

PWND FASTCALL
DWP_GetEnabledPopup(PWND pWnd)
{
    PWND pwndNode1;
    PTHREADINFO pti = pWnd->head.pti, ptiNode;
    BOOL bFoundNullNode = FALSE;

    for (pwndNode1 = pWnd->spwndNext; pwndNode1 != pWnd; )
    {
        if (!pwndNode1) /* NULL detected? */
        {
            if (bFoundNullNode)
                return NULL;
            bFoundNullNode = TRUE;
            /* Retry with parent's first child (once only) */
            pwndNode1 = pWnd->spwndParent->spwndChild;
            continue;
        }

        /*
         * 1. We want to detect the window that owns the same input target of pWnd.
         * 2. For non-16-bit apps, we need to check the two threads' input queues to
         *    see whether they are the same, while for 16-bit apps it's sufficient to
         *    only check the thread info pointers themselves (ptiNode and pti).
         * See also:
         *    https://devblogs.microsoft.com/oldnewthing/20060221-09/?p=32203
         *    https://github.com/reactos/reactos/pull/7700#discussion_r1939435931
         */
        ptiNode = pwndNode1->head.pti;
        if ((!(pti->TIF_flags & TIF_16BIT) && ptiNode->MessageQueue == pti->MessageQueue) ||
            ((pti->TIF_flags & TIF_16BIT) && ptiNode == pti))
        {
            DWORD style = pwndNode1->style;
            if ((style & WS_VISIBLE) && !(style & WS_DISABLED)) /* Visible and enabled? */
            {
                /* Does pwndNode1 have a pWnd as an ancestor? */
                PWND pwndNode2;
                for (pwndNode2 = pwndNode1->spwndOwner; pwndNode2;
                     pwndNode2 = pwndNode2->spwndOwner)
                {
                    if (pwndNode2 == pWnd)
                        return pwndNode1;
                }
            }
        }

        pwndNode1 = pwndNode1->spwndNext;
    }

    return NULL;
}

VOID FASTCALL
DefWndScreenshot(PWND pWnd)
{
    RECT rect;
    HDC hdc;
    INT w;
    INT h;
    HBITMAP hbitmap;
    HDC hdc2;
    SETCLIPBDATA scd = {FALSE, FALSE};

    UserOpenClipboard(UserHMGetHandle(pWnd));
    UserEmptyClipboard();

    hdc = UserGetWindowDC(pWnd);
    IntGetWindowRect(pWnd, &rect);
    w = rect.right - rect.left;
    h = rect.bottom - rect.top;

    hbitmap = NtGdiCreateCompatibleBitmap(hdc, w, h);
    hdc2 = NtGdiCreateCompatibleDC(hdc);
    NtGdiSelectBitmap(hdc2, hbitmap);

    NtGdiBitBlt(hdc2, 0, 0, w, h, hdc, 0, 0, SRCCOPY, 0, 0);

    UserSetClipboardData(CF_BITMAP, hbitmap, &scd);

    UserReleaseDC(pWnd, hdc, FALSE);
    UserReleaseDC(pWnd, hdc2, FALSE);

    UserCloseClipboard();
}

/*
   Win32k counterpart of User DefWindowProc
 */
LRESULT FASTCALL
IntDefWindowProc(
   PWND Wnd,
   UINT Msg,
   WPARAM wParam,
   LPARAM lParam,
   BOOL Ansi)
{
   PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
   LRESULT lResult = 0;
   USER_REFERENCE_ENTRY Ref;

   if (Msg > WM_USER) return 0;

   switch (Msg)
   {
      case WM_DEVICECHANGE:
            return TRUE;

      case WM_GETTEXTLENGTH:
      {
            PWSTR buf;
            ULONG len;

            if (Wnd != NULL && Wnd->strName.Length != 0)
            {
                buf = Wnd->strName.Buffer;
                if (buf != NULL &&
                    NT_SUCCESS(RtlUnicodeToMultiByteSize(&len,
                                                         buf,
                                                         Wnd->strName.Length)))
                {
                    lResult = (LRESULT) (Wnd->strName.Length / sizeof(WCHAR));
                }
            }

            break;
      }

      case WM_GETTEXT: // FIXME: Handle Ansi
      {
            PWSTR buf = NULL;
            PWSTR outbuf = (PWSTR)lParam;

            if (Wnd != NULL && wParam != 0)
            {
                if (Wnd->strName.Buffer != NULL)
                    buf = Wnd->strName.Buffer;
                else
                    outbuf[0] = L'\0';

                if (buf != NULL)
                {
                    if (Wnd->strName.Length != 0)
                    {
                        lResult = min(Wnd->strName.Length / sizeof(WCHAR), wParam - 1);
                        RtlCopyMemory(outbuf,
                                      buf,
                                      lResult * sizeof(WCHAR));
                        outbuf[lResult] = L'\0';
                    }
                    else
                        outbuf[0] = L'\0';
                }
            }
            break;
      }

      case WM_SETTEXT: // FIXME: Handle Ansi
      {
            DefSetText(Wnd, (PCWSTR)lParam);

            if ((Wnd->style & WS_CAPTION) == WS_CAPTION)
                UserPaintCaption(Wnd, DC_TEXT);
            IntNotifyWinEvent(EVENT_OBJECT_NAMECHANGE, Wnd, OBJID_WINDOW, CHILDID_SELF, 0);
            lResult = 1;
            break;
      }

      case WM_SYSCOMMAND:
      {
         TRACE("hwnd %p WM_SYSCOMMAND %lx %lx\n", UserHMGetHandle(Wnd), wParam, lParam );
         lResult = DefWndHandleSysCommand(Wnd, wParam, lParam);
         break;
      }

      case WM_SHOWWINDOW:
      {
         if ((Wnd->style & WS_VISIBLE) && wParam) break;
         if (!(Wnd->style & WS_VISIBLE) && !wParam) break;
         if (!Wnd->spwndOwner) break;
         if (LOWORD(lParam))
         {
            co_WinPosShowWindow(Wnd, wParam ? SW_SHOWNOACTIVATE : SW_HIDE);
         }
         break;
      }

      case WM_CLIENTSHUTDOWN:
         return IntClientShutdown(Wnd, wParam, lParam);

      case WM_APPCOMMAND:
         if ( (Wnd->style & (WS_POPUP|WS_CHILD)) != WS_CHILD &&
               Wnd != co_GetDesktopWindow(Wnd) )
         {
            if (!co_HOOK_CallHooks(WH_SHELL, HSHELL_APPCOMMAND, wParam, lParam))
               co_IntShellHookNotify(HSHELL_APPCOMMAND, wParam, lParam);
            break;
         }
         UserRefObjectCo(Wnd->spwndParent, &Ref);
         lResult = co_IntSendMessage(UserHMGetHandle(Wnd->spwndParent), WM_APPCOMMAND, wParam, lParam);
         UserDerefObjectCo(Wnd->spwndParent);
         break;

      case WM_KEYF1:
      {
         HELPINFO hi;
         HMENU hMenu = UlongToHandle(Wnd->IDMenu);
         PWND pwndActive = MENU_IsMenuActive();
         hi.cbSize = sizeof(HELPINFO);
         hi.MousePos = gpsi->ptCursor;
         hi.iContextType = HELPINFO_MENUITEM;
         hi.hItemHandle = pwndActive ? UserHMGetHandle(pwndActive) : UserHMGetHandle(Wnd);
         hi.iCtrlId = (Wnd->style & (WS_POPUP|WS_CHILD)) == WS_CHILD ? IntMenuItemFromPoint(Wnd, hMenu, hi.MousePos) : 0;
         hi.dwContextId = IntGetWindowContextHelpId(Wnd);

         co_IntSendMessage( UserHMGetHandle(Wnd), WM_HELP, 0, (LPARAM)&hi );
         break;
      }

      case WM_SETICON:
      {
         return DefWndSetIcon(Wnd, wParam, lParam);
      }

      case WM_GETICON:
      {
         return DefWndGetIcon(Wnd, wParam, lParam);
      }

      case WM_HELP:
      {
         PWND Parent = IntGetParent(Wnd);
         co_IntSendMessage(UserHMGetHandle(Parent), Msg, wParam, lParam);
         break;
      }

      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_MBUTTONDOWN:
          pti->MessageQueue->QF_flags &= ~(QF_FMENUSTATUS|QF_FMENUSTATUSBREAK);
          break;

      case WM_NCLBUTTONDOWN:
          return NC_HandleNCLButtonDown(Wnd, wParam, lParam);

      case WM_NCRBUTTONDOWN:
          return NC_HandleNCRButtonDown(Wnd, wParam, lParam);

      case WM_LBUTTONDBLCLK:
          return NC_HandleNCLButtonDblClk(Wnd, HTCLIENT, lParam);

      case WM_NCLBUTTONDBLCLK:
          return NC_HandleNCLButtonDblClk(Wnd, wParam, lParam);

      case WM_RBUTTONUP:
      {
            POINT Pt;

            Pt.x = GET_X_LPARAM(lParam);
            Pt.y = GET_Y_LPARAM(lParam);
            IntClientToScreen(Wnd, &Pt);
            lParam = MAKELPARAM(Pt.x, Pt.y);
            co_IntSendMessage(UserHMGetHandle(Wnd), WM_CONTEXTMENU, (WPARAM)UserHMGetHandle(Wnd), lParam);
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
              co_IntSendMessage(UserHMGetHandle(Wnd), WM_APPCOMMAND, (WPARAM)UserHMGetHandle(Wnd),
                                MAKELPARAM(LOWORD(wParam), FAPPCOMMAND_MOUSE | HIWORD(wParam)));
          }
          break;


      case WM_CONTEXTMENU:
      {
            if (Wnd->style & WS_CHILD)
            {
                co_IntSendMessage(UserHMGetHandle(IntGetParent(Wnd)), Msg, (WPARAM)UserHMGetHandle(Wnd), lParam);
            }
            else
            {
                POINT Pt;
                LONG_PTR Style;
                LONG HitCode;

                Style = Wnd->style;

                Pt.x = GET_X_LPARAM(lParam);
                Pt.y = GET_Y_LPARAM(lParam);
                if (Style & WS_CHILD)
                {
                    IntScreenToClient(IntGetParent(Wnd), &Pt);
                }

                HitCode = GetNCHitEx(Wnd, Pt);

                if (HitCode == HTCAPTION || HitCode == HTSYSMENU)
                {
                    PMENU SystemMenu;
                    UINT Flags;

                    if((SystemMenu = IntGetSystemMenu(Wnd, FALSE)))
                    {
                      MENU_InitSysMenuPopup(SystemMenu, Wnd->style, Wnd->pcls->style, HitCode);

                      if(HitCode == HTCAPTION)
                        Flags = TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
                      else
                        Flags = TPM_LEFTBUTTON;

                      IntTrackPopupMenuEx(SystemMenu, Flags|TPM_SYSTEM_MENU, Pt.x, Pt.y, Wnd, NULL);
                    }
                }
                if (HitCode == HTHSCROLL || HitCode == HTVSCROLL)
                {
                   WARN("Scroll Menu Not Supported\n");
                }
            }
            break;
      }

      case WM_KEYDOWN:
         if (wParam == VK_F10)
         {
            pti->MessageQueue->QF_flags |= QF_FF10STATUS;

            if (UserGetKeyState(VK_SHIFT) & 0x8000)
            {
               co_IntSendMessage(UserHMGetHandle(Wnd), WM_CONTEXTMENU, (WPARAM)UserHMGetHandle(Wnd), MAKELPARAM(-1, -1));
            }
         }
         if (g_bWindowSnapEnabled && (IS_KEY_DOWN(gafAsyncKeyState, VK_LWIN) || IS_KEY_DOWN(gafAsyncKeyState, VK_RWIN)))
         {
            HWND hwndTop = UserGetForegroundWindow();
            PWND topWnd = UserGetWindowObject(hwndTop);
            BOOL allowSnap;

            // MS Doc: foreground window can be NULL, e.g. when window is losing activation
            if (!topWnd)
               return 0;

            allowSnap = IntIsSnapAllowedForWindow(topWnd);
            /* Allow the minimize action if it has a minimize button, even if the window cannot be snapped (e.g. Calc.exe) */
            if (!allowSnap && (topWnd->style & (WS_MINIMIZEBOX|WS_THICKFRAME)) == WS_MINIMIZEBOX)
                allowSnap = wParam == VK_DOWN;

            if (allowSnap)
            {
               UINT snapped = IntGetWindowSnapEdge(topWnd);

               if (wParam == VK_DOWN)
               {
                   if (topWnd->style & WS_MAXIMIZE)
                       co_IntSendMessage(hwndTop, WM_SYSCOMMAND, SC_RESTORE, MAKELONG(0, 1));
                   else if (snapped)
                       co_IntUnsnapWindow(topWnd);
                   else
                       co_IntSendMessage(hwndTop, WM_SYSCOMMAND, SC_MINIMIZE, MAKELONG(0, 1));
               }
               else if (wParam == VK_UP)
               {
                   if (topWnd->style & WS_MINIMIZE)
                       co_IntSendMessage(hwndTop, WM_SYSCOMMAND, SC_RESTORE, MAKELONG(0, 1));
                   else
                       co_IntSendMessage(hwndTop, WM_SYSCOMMAND, SC_MAXIMIZE, MAKELONG(0, 1));
               }
               else if (wParam == VK_LEFT || wParam == VK_RIGHT)
               {
                  UINT edge = wParam == VK_LEFT ? HTLEFT : HTRIGHT;
                  UINT otherEdge = edge == HTLEFT ? HTRIGHT : HTLEFT;

                  if (topWnd->style & WS_MAXIMIZE)
                  {
                     /* SC_RESTORE + Snap causes the window to visually move twice, place it manually in the snap position */
                     RECT normalRect = topWnd->InternalPos.NormalRect;
                     co_IntCalculateSnapPosition(topWnd, edge, &topWnd->InternalPos.NormalRect); /* Calculate edge position */
                     IntSetSnapEdge(topWnd, edge); /* Tell everyone the edge we are snapped to */
                     co_IntSendMessage(hwndTop, WM_SYSCOMMAND, SC_RESTORE, MAKELONG(0, 1));
                     IntSetSnapInfo(topWnd, edge, &normalRect); /* Reset the real place to unsnap to */
                     snapped = HTNOWHERE; /* Force snap */
                  }
#if 0 /* Windows 8 does this but is it a good feature? */
                  else if (snapped == edge)
                  {
                     /* Already snapped to this edge, snap to the opposite side */
                     edge = otherEdge;
                  }
#endif

                  if (snapped == otherEdge)
                     co_IntUnsnapWindow(topWnd);
                  else
                     co_IntSnapWindow(topWnd, edge);
               }
            }
         }
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
                          wParam == VK_RMENU ) && !(pti->MessageQueue->QF_flags & QF_FMENUSTATUS)) //iMenuSysKey )
                       pti->MessageQueue->QF_flags |= QF_FMENUSTATUS; //iMenuSysKey = 1;
                   else
                       pti->MessageQueue->QF_flags &= ~QF_FMENUSTATUS; //iMenuSysKey = 0;
                }

                pti->MessageQueue->QF_flags &= ~QF_FF10STATUS; //iF10Key = 0;

                if (wParam == VK_F4) /* Try to close the window */
                {
                   PWND top = UserGetAncestor(Wnd, GA_ROOT);
                   if (!(top->pcls->style & CS_NOCLOSE))
                      UserPostMessage(UserHMGetHandle(top), WM_SYSCOMMAND, SC_CLOSE, 0);
                }
                else if (wParam == VK_SNAPSHOT) // Alt-VK_SNAPSHOT?
                {
                   PWND pwnd = Wnd;
                   while (IntGetParent(pwnd) != NULL)
                   {
                       pwnd = IntGetParent(pwnd);
                   }
                   ERR("DefWndScreenshot\n");
                   DefWndScreenshot(pwnd);
                }
                else if ( wParam == VK_ESCAPE || wParam == VK_TAB ) // Alt-Tab/ESC Alt-Shift-Tab/ESC
                {
                   WPARAM wParamTmp;
                   HWND Active = UserGetActiveWindow(); // Noticed MDI problem.
                   if (!Active)
                   {
                      FIXME("WM_SYSKEYDOWN VK_ESCAPE no active\n");
                      break;
                   }
                   wParamTmp = UserGetKeyState(VK_SHIFT) & 0x8000 ? SC_PREVWINDOW : SC_NEXTWINDOW;
                   co_IntSendMessage( Active, WM_SYSCOMMAND, wParamTmp, wParam );
                }
            }
            else if( wParam == VK_F10 )
            {
                if (UserGetKeyState(VK_SHIFT) & 0x8000)
                    co_IntSendMessage( UserHMGetHandle(Wnd), WM_CONTEXTMENU, (WPARAM)UserHMGetHandle(Wnd), MAKELPARAM(-1, -1) );
                pti->MessageQueue->QF_flags |= QF_FF10STATUS; //iF10Key = 1;
            }
            else if( wParam == VK_ESCAPE && (UserGetKeyState(VK_SHIFT) & 0x8000))
                  co_IntSendMessage( UserHMGetHandle(Wnd), WM_SYSCOMMAND, SC_KEYMENU, ' ' );
            break;
      }

      case WM_KEYUP:
      case WM_SYSKEYUP:
      {
           /* Press and release F10 or ALT */
            if (((wParam == VK_MENU || wParam == VK_LMENU || wParam == VK_RMENU)
                 && (pti->MessageQueue->QF_flags & (QF_FMENUSTATUS|QF_FMENUSTATUSBREAK)) == QF_FMENUSTATUS /*iMenuSysKey*/) ||
                 ((wParam == VK_F10) && pti->MessageQueue->QF_flags & QF_FF10STATUS /*iF10Key*/))
                co_IntSendMessage( UserHMGetHandle(UserGetAncestor( Wnd, GA_ROOT )), WM_SYSCOMMAND, SC_KEYMENU, 0L );
            pti->MessageQueue->QF_flags &= ~(QF_FMENUSTATUS|QF_FMENUSTATUSBREAK|QF_FF10STATUS); //iMenuSysKey = iF10Key = 0;
            break;
      }

      case WM_SYSCHAR:
      {
            pti->MessageQueue->QF_flags &= ~(QF_FMENUSTATUS|QF_FMENUSTATUSBREAK); //iMenuSysKey = 0;
            if (wParam == VK_RETURN && (Wnd->style & WS_MINIMIZE) != 0)
            {
                UserPostMessage( UserHMGetHandle(Wnd), WM_SYSCOMMAND, SC_RESTORE, 0L );
                break;
            }
            if ((HIWORD(lParam) & KF_ALTDOWN) && wParam)
            {
                if (wParam == VK_TAB || wParam == VK_ESCAPE) break;
                if (wParam == VK_SPACE && Wnd->style & WS_CHILD)
                    co_IntSendMessage( UserHMGetHandle(IntGetParent(Wnd)), Msg, wParam, lParam );
                else
                    co_IntSendMessage( UserHMGetHandle(Wnd), WM_SYSCOMMAND, SC_KEYMENU, wParam );
            }
            else /* check for Ctrl-Esc */
                if (wParam != VK_ESCAPE) UserPostMessage(hwndSAS, WM_LOGONNOTIFY, LN_MESSAGE_BEEP, 0); //MessageBeep(0);
            break;
      }

      case WM_CANCELMODE:
      {
         pti->MessageQueue->QF_flags &= ~(QF_FMENUSTATUS|QF_FMENUSTATUSBREAK);

         MENU_EndMenu( Wnd );
         if (IntGetCaptureWindow() == UserHMGetHandle(Wnd))
         {
            IntReleaseCapture();
         }
         break;
      }

      case WM_CLOSE:
         co_UserDestroyWindow(Wnd);
         break;

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

      case WM_SETCURSOR:
      {
         if (Wnd->style & WS_CHILD)
         {
             /* with the exception of the border around a resizable wnd,
              * give the parent first chance to set the cursor */
             if (LOWORD(lParam) < HTLEFT || LOWORD(lParam) > HTBOTTOMRIGHT)
             {
                 PWND parent = Wnd->spwndParent;//IntGetParent( Wnd );
                 if (parent != UserGetDesktopWindow() &&
                     co_IntSendMessage( UserHMGetHandle(parent), WM_SETCURSOR, wParam, lParam))
                    return TRUE;
             }
         }
         return DefWndHandleSetCursor(Wnd, wParam, lParam);
      }

      case WM_MOUSEACTIVATE:
         if (Wnd->style & WS_CHILD)
         {
             HWND hwndParent;
             PWND pwndParent = IntGetParent(Wnd);
             hwndParent = pwndParent ? UserHMGetHandle(pwndParent) : NULL;
             if (hwndParent)
             {
                 lResult = co_IntSendMessage(hwndParent, WM_MOUSEACTIVATE, wParam, lParam);
                 if (lResult)
                     break;
             }
         }
         return ( (HIWORD(lParam) == WM_LBUTTONDOWN && LOWORD(lParam) == HTCAPTION) ? MA_NOACTIVATE : MA_ACTIVATE );

      case WM_ACTIVATE:
       /* The default action in Windows is to set the keyboard focus to
        * the window, if it's being activated and not minimized */
         if (LOWORD(wParam) != WA_INACTIVE &&
              !(Wnd->style & WS_MINIMIZE))
         {
            //ERR("WM_ACTIVATE %p\n",hWnd);
            co_UserSetFocus(Wnd);
         }
         break;

      case WM_MOUSEWHEEL:
         if (Wnd->style & WS_CHILD)
         {
            HWND hwndParent;
            PWND pwndParent = IntGetParent(Wnd);
            hwndParent = pwndParent ? UserHMGetHandle(pwndParent) : NULL;
            return co_IntSendMessage( hwndParent, WM_MOUSEWHEEL, wParam, lParam);
         }
         break;

      case WM_ERASEBKGND:
      case WM_ICONERASEBKGND:
      {
         RECT Rect;
         HBRUSH hBrush = Wnd->pcls->hbrBackground;
         if (!hBrush) return 0;
         if (hBrush <= (HBRUSH)COLOR_MENUBAR)
         {
            hBrush = IntGetSysColorBrush(HandleToUlong(hBrush));
         }
         if (Wnd->pcls->style & CS_PARENTDC)
         {
            /* can't use GetClipBox with a parent DC or we fill the whole parent */
            IntGetClientRect(Wnd, &Rect);
            GreDPtoLP((HDC)wParam, (LPPOINT)&Rect, 2);
         }
         else
         {
            GdiGetClipBox((HDC)wParam, &Rect);
         }
         FillRect((HDC)wParam, &Rect, hBrush);
         return (1);
      }

      case WM_GETHOTKEY:
         //ERR("WM_GETHOTKEY\n");
         return DefWndGetHotKey(Wnd);
      case WM_SETHOTKEY:
         //ERR("WM_SETHOTKEY\n");
         return DefWndSetHotKey(Wnd, wParam);

      case WM_NCHITTEST:
      {
         POINT Point;
         Point.x = GET_X_LPARAM(lParam);
         Point.y = GET_Y_LPARAM(lParam);
         return GetNCHitEx(Wnd, Point);
      }

      case WM_PRINT:
      {
         DefWndPrint(Wnd, (HDC)wParam, lParam);
         return (0);
      }

      case WM_SYSCOLORCHANGE:
      {
         /* force to redraw non-client area */
         UserPaintCaption(Wnd, DC_NC);
         /* Use InvalidateRect to redraw client area, enable
          * erase to redraw all subcontrols otherwise send the
          * WM_SYSCOLORCHANGE to child windows/controls is required
          */
         co_UserRedrawWindow( Wnd, NULL, NULL, RDW_ALLCHILDREN|RDW_INVALIDATE|RDW_ERASE);
         return (0);
      }

      case WM_PAINTICON:
      case WM_PAINT:
      {
         PAINTSTRUCT Ps;
         HDC hDC;

         /* If already in Paint and Client area is not empty just return. */
         if (Wnd->state2 & WNDS2_STARTPAINT && !RECTL_bIsEmptyRect(&Wnd->rcClient))
         {
            ERR("In Paint and Client area is not empty!\n");
            return 0;
         }

         hDC = IntBeginPaint(Wnd, &Ps);
         if (hDC)
         {
             if (((Wnd->style & WS_MINIMIZE) != 0) && (Wnd->pcls->spicn))
             {
                 RECT ClientRect;
                 INT x, y;

                 ERR("Doing Paint and Client area is empty!\n");
                 IntGetClientRect(Wnd, &ClientRect);
                 x = (ClientRect.right - ClientRect.left - UserGetSystemMetrics(SM_CXICON)) / 2;
                 y = (ClientRect.bottom - ClientRect.top - UserGetSystemMetrics(SM_CYICON)) / 2;
                 UserReferenceObject(Wnd->pcls->spicn);
                 UserDrawIconEx(hDC, x, y, Wnd->pcls->spicn, 0, 0, 0, 0, DI_NORMAL | DI_COMPAT | DI_DEFAULTSIZE);
                 UserDereferenceObject(Wnd->pcls->spicn);
             }

             IntEndPaint(Wnd, &Ps);
         }
         return (0);
      }

      case WM_SYNCPAINT:
      {
         HRGN hRgn;
         Wnd->state &= ~WNDS_SYNCPAINTPENDING;
         TRACE("WM_SYNCPAINT\n");
         hRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
         if (hRgn)
         {
             if (co_UserGetUpdateRgn(Wnd, hRgn, FALSE) != NULLREGION)
             {
                PREGION pRgn = REGION_LockRgn(hRgn);
                if (pRgn) REGION_UnlockRgn(pRgn);
                if (!wParam)
                    wParam = (RDW_ERASENOW | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
                co_UserRedrawWindow(Wnd, NULL, pRgn, wParam);
             }
             GreDeleteObject(hRgn);
         }
         return 0;
      }

      case WM_SETREDRAW:
          if (wParam)
          {
             if (!(Wnd->style & WS_VISIBLE))
             {
                IntSetStyle( Wnd, WS_VISIBLE, 0 );
                Wnd->state |= WNDS_SENDNCPAINT;
             }
          }
          else
          {
             if (Wnd->style & WS_VISIBLE)
             {
                co_UserRedrawWindow( Wnd, NULL, NULL, RDW_ALLCHILDREN | RDW_VALIDATE );
                IntSetStyle( Wnd, 0, WS_VISIBLE );
             }
          }
          return 0;

      case WM_WINDOWPOSCHANGING:
      {
          return (DefWndHandleWindowPosChanging(Wnd, (WINDOWPOS*)lParam));
      }

      case WM_WINDOWPOSCHANGED:
      {
          return (DefWndHandleWindowPosChanged(Wnd, (WINDOWPOS*)lParam));
      }

      case WM_NCCALCSIZE:
      {
         return NC_HandleNCCalcSize( Wnd, wParam, (RECTL *)lParam, FALSE );
      }

      case WM_NCACTIVATE:
      {
          return NC_HandleNCActivate( Wnd, wParam, lParam );
      }

      //
      // NC Paint mode.
      //
      case WM_NCPAINT:
      {
          HDC hDC = UserGetDCEx(Wnd, (HRGN)wParam, DCX_WINDOW | DCX_INTERSECTRGN | DCX_USESTYLE | DCX_KEEPCLIPRGN);
          Wnd->state |= WNDS_FORCEMENUDRAW;
          NC_DoNCPaint(Wnd, hDC, -1);
          Wnd->state &= ~WNDS_FORCEMENUDRAW;
          UserReleaseDC(Wnd, hDC, FALSE);
          return 0;
      }
      //
      //  Draw Caption mode.
      //
      //  wParam are DC_* flags.
      //
      case WM_NCUAHDRAWCAPTION:
      {
          HDC hDC = UserGetDCEx(Wnd, NULL, DCX_WINDOW|DCX_USESTYLE);
          TRACE("WM_NCUAHDRAWCAPTION: wParam DC_ flags %08x\n",wParam);
          UserDrawCaptionBar(Wnd, hDC, wParam | DC_FRAME); // Include DC_FRAME to comp for drawing glitch.
          UserReleaseDC(Wnd, hDC, FALSE);
          return 0;
      }
      //
      //  Draw Frame mode.
      //
      //  wParam is HDC, lParam are DC_ACTIVE and or DC_REDRAWHUNGWND.
      //
      case WM_NCUAHDRAWFRAME:
      {
          TRACE("WM_NCUAHDRAWFRAME: wParam hDC %p lParam DC_ flags %08x\n",wParam,lParam);
          NC_DoNCPaint(Wnd, (HDC)wParam, lParam|DC_NC);
          return 0;
      }

      /* ReactOS only. */
      case WM_CBT:
      {
         switch (wParam)
         {
            case HCBT_MOVESIZE:
            {
               RECTL rt;

               if (lParam)
               {
                  _SEH2_TRY
                  {
                      ProbeForRead((PVOID)lParam,
                                   sizeof(RECT),
                                   1);

                      RtlCopyMemory(&rt,
                                    (PVOID)lParam,
                                    sizeof(RECT));
                  }
                  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                  {
                      lResult = 1;
                  }
                  _SEH2_END;
               }
               if (!lResult)
                  lResult = co_HOOK_CallHooks(WH_CBT, HCBT_MOVESIZE, (WPARAM)UserHMGetHandle(Wnd), lParam ? (LPARAM)&rt : 0);

               break;
            }
         }
         break;
      }
   }
   return lResult;
}

/* EOF */
