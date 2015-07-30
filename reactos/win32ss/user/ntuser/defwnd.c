/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Miscellaneous User functions
 * FILE:             win32ss/user/ntuser/defwnd.c
 * PROGRAMER:
 */

#include <win32k.h>
#include <windowsx.h>

DBG_DEFAULT_CHANNEL(UserDefwnd);


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
            UserSetCursor(pWnd->pcls->spcur, FALSE);
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
         UserSetCursor(SYSTEMCUR(SIZEWE), FALSE);
         return TRUE;
      }

      case HTTOP:
      case HTBOTTOM:
      {
         if (pWnd->style & WS_MAXIMIZE)
         {
            break;
         }
         UserSetCursor(SYSTEMCUR(SIZENS), FALSE);
         return TRUE;
       }

       case HTTOPLEFT:
       case HTBOTTOMRIGHT:
       {
         if (pWnd->style & WS_MAXIMIZE)
         {
            break;
         }
         UserSetCursor(SYSTEMCUR(SIZENWSE), FALSE);
         return TRUE;
       }

       case HTBOTTOMLEFT:
       case HTTOPRIGHT:
       {
         if (pWnd->style & WS_MAXIMIZE)
         {
            break;
         }
         UserSetCursor(SYSTEMCUR(SIZENESW), FALSE);
         return TRUE;
       }
   }
   UserSetCursor(SYSTEMCUR(ARROW), FALSE);
   return FALSE;
}

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
   LRESULT lResult = 0;
   USER_REFERENCE_ENTRY Ref;

   if (Msg > WM_USER) return 0;

   switch (Msg)
   {
      case WM_SYSCOMMAND:
      {
         ERR("hwnd %p WM_SYSCOMMAND %lx %lx\n", Wnd->head.h, wParam, lParam );
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
            if (wParam)
            {
               if (!(Wnd->state & WNDS_HIDDENPOPUP)) break;
               Wnd->state &= ~WNDS_HIDDENPOPUP;
            }
            else
                Wnd->state |= WNDS_HIDDENPOPUP;

            co_WinPosShowWindow(Wnd, wParam ? SW_SHOWNOACTIVATE : SW_HIDE);
         }
      }
      break;

      case WM_CLIENTSHUTDOWN:
         return IntClientShutdown(Wnd, wParam, lParam);

      case WM_APPCOMMAND:
         ERR("WM_APPCOMMAND\n");
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
            hBrush = IntGetSysColorBrush((INT)hBrush);
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
                 UserDrawIconEx(hDC, x, y, Wnd->pcls->spicn, 0, 0, 0, 0, DI_NORMAL | DI_COMPAT | DI_DEFAULTSIZE);
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
          ERR("WM_SETREDRAW\n");
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
                  lResult = co_HOOK_CallHooks(WH_CBT, HCBT_MOVESIZE, (WPARAM)Wnd->head.h, lParam ? (LPARAM)&rt : 0);
           }
            break;
         }
         break;
      }
      break;
   }
   return lResult;
}

/* EOF */
