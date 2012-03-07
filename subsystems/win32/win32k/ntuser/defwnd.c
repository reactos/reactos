/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Miscellaneous User functions
 * FILE:             subsystems/win32/win32k/ntuser/defwnd.c
 * PROGRAMER:
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserDefwnd);

// Client Shutdown messages
#define MCS_SHUTDOWNTIMERS  1
#define MCS_QUERYENDSESSION 2
// Client Shutdown returns
#define MCSR_GOODFORSHUTDOWN  1
#define MCSR_SHUTDOWNFINISHED 2
#define MCSR_DONOTSHUTDOWN    3

/*
 * Based on CSRSS and described in pages 1115 - 1118 "Windows Internals, Fifth Edition".
 * Apparently CSRSS sends out messages to do this w/o going into win32k internals.
 */
static
LRESULT FASTCALL
IntClientShutdown(
   PWND pWindow,
   WPARAM wParam,
   LPARAM lParam
)
{
   LPARAM lParams;
   BOOL KillTimers;
   INT i;
   LRESULT lResult = MCSR_GOODFORSHUTDOWN;
   HWND *List;

   lParams = wParam & (ENDSESSION_LOGOFF|ENDSESSION_CRITICAL|ENDSESSION_CLOSEAPP);
   KillTimers = wParam & MCS_SHUTDOWNTIMERS ? TRUE : FALSE;
/*
   First, send end sessions to children.
 */
   List = IntWinListChildren(pWindow);

   if (List)
   {
      for (i = 0; List[i]; i++)
      {
          PWND WndChild;

          if (!(WndChild = UserGetWindowObject(List[i])))
             continue;

          if (wParam & MCS_QUERYENDSESSION)
          {
             if (!co_IntSendMessage(WndChild->head.h, WM_QUERYENDSESSION, 0, lParams))
             {
                lResult = MCSR_DONOTSHUTDOWN;
                break;
             }
          }
          else
          {
             co_IntSendMessage(WndChild->head.h, WM_ENDSESSION, KillTimers, lParams);
             if (KillTimers)
             {
                DestroyTimersForWindow(WndChild->head.pti, WndChild);
             }
             lResult = MCSR_SHUTDOWNFINISHED;
          }
      }
      ExFreePool(List);
   }
   if (List && (lResult == MCSR_DONOTSHUTDOWN)) return lResult;
/*
   Send to the caller.
 */
   if (wParam & MCS_QUERYENDSESSION)
   {
      if (!co_IntSendMessage(pWindow->head.h, WM_QUERYENDSESSION, 0, lParams))
      {
         lResult = MCSR_DONOTSHUTDOWN;
      }
   }
   else
   {
      co_IntSendMessage(pWindow->head.h, WM_ENDSESSION, KillTimers, lParams);
      if (KillTimers)
      {
         DestroyTimersForWindow(pWindow->head.pti, pWindow);
      }
      lResult = MCSR_SHUTDOWNFINISHED;
   }
   return lResult;
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
      case SC_SCREENSAVE:
        ERR("Screensaver Called!\n");
        UserPostMessage(hwndSAS, WM_LOGONNOTIFY, LN_START_SCREENSAVE, 0); // always lParam 0 == not Secure
        break;

      default:
   // We do not support anything else here so we should return normal even when sending a hook.
        return 0;
   }

   return(Hook ? 1 : 0); // Don't call us again from user space.
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

      case WM_GETHOTKEY:
         return DefWndGetHotKey(UserHMGetHandle(Wnd));
      case WM_SETHOTKEY:
         return DefWndSetHotKey(Wnd, wParam);

      case WM_NCHITTEST:
      {
         POINT Point;
         Point.x = GET_X_LPARAM(lParam);
         Point.y = GET_Y_LPARAM(lParam);
         return GetNCHitEx(Wnd, Point);
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

static HICON NC_IconForWindow( PWND pWnd )
{
   HICON hIcon = 0;

   if (!pWnd->pcls || pWnd->fnid == FNID_DESKTOP) return hIcon;
   if (!hIcon) hIcon = pWnd->pcls->hIconSm;
   if (!hIcon) hIcon = pWnd->pcls->hIcon;

   if (!hIcon && pWnd->style & DS_MODALFRAME)
   { // Fake it out for now, we use it as a test.
      hIcon = (HICON)1;
   /* FIXME: Need to setup Registry System Cursor & Icons via Callbacks at init time! */
      if (!hIcon) hIcon = gpsi->hIconSmWindows; // Both are IDI_WINLOGO Small
      if (!hIcon) hIcon = gpsi->hIcoWindows;    // Reg size.
   }
   return hIcon;
}

DWORD FASTCALL
GetNCHitEx(PWND pWnd, POINT pt)
{
   RECT rcWindow, rcClient;
   DWORD Style, ExStyle;

   if (!pWnd) return HTNOWHERE;

   if (pWnd == UserGetDesktopWindow()) // pWnd->fnid == FNID_DESKTOP)
   {
      rcClient.left = rcClient.top = rcWindow.left = rcWindow.top = 0;
      rcWindow.right  = UserGetSystemMetrics(SM_CXSCREEN);
      rcWindow.bottom = UserGetSystemMetrics(SM_CYSCREEN);
      rcClient.right  = UserGetSystemMetrics(SM_CXSCREEN);
      rcClient.bottom = UserGetSystemMetrics(SM_CYSCREEN);
   }
   else
   {
      rcClient = pWnd->rcClient;
      rcWindow = pWnd->rcWindow;
   }

   if (!RECTL_bPointInRect(&rcWindow, pt.x, pt.y)) return HTNOWHERE;

   Style = pWnd->style;
   ExStyle = pWnd->ExStyle;

   if (Style & WS_MINIMIZE) return HTCAPTION;

   if (RECTL_bPointInRect( &rcClient,  pt.x, pt.y )) return HTCLIENT;

   /* Check borders */
   if (HAS_THICKFRAME( Style, ExStyle ))
   {
      RECTL_vInflateRect(&rcWindow, -UserGetSystemMetrics(SM_CXFRAME), -UserGetSystemMetrics(SM_CYFRAME) );
      if (!RECTL_bPointInRect(&rcWindow, pt.x, pt.y ))
      {
            /* Check top sizing border */
            if (pt.y < rcWindow.top)
            {
                if (pt.x < rcWindow.left+UserGetSystemMetrics(SM_CXSIZE)) return HTTOPLEFT;
                if (pt.x >= rcWindow.right-UserGetSystemMetrics(SM_CXSIZE)) return HTTOPRIGHT;
                return HTTOP;
            }
            /* Check bottom sizing border */
            if (pt.y >= rcWindow.bottom)
            {
                if (pt.x < rcWindow.left+UserGetSystemMetrics(SM_CXSIZE)) return HTBOTTOMLEFT;
                if (pt.x >= rcWindow.right-UserGetSystemMetrics(SM_CXSIZE)) return HTBOTTOMRIGHT;
                return HTBOTTOM;
            }
            /* Check left sizing border */
            if (pt.x < rcWindow.left)
            {
                if (pt.y < rcWindow.top+UserGetSystemMetrics(SM_CYSIZE)) return HTTOPLEFT;
                if (pt.y >= rcWindow.bottom-UserGetSystemMetrics(SM_CYSIZE)) return HTBOTTOMLEFT;
                return HTLEFT;
            }
            /* Check right sizing border */
            if (pt.x >= rcWindow.right)
            {
                if (pt.y < rcWindow.top+UserGetSystemMetrics(SM_CYSIZE)) return HTTOPRIGHT;
                if (pt.y >= rcWindow.bottom-UserGetSystemMetrics(SM_CYSIZE)) return HTBOTTOMRIGHT;
                return HTRIGHT;
            }
        }
    }
    else  /* No thick frame */
    {
        if (HAS_DLGFRAME( Style, ExStyle ))
            RECTL_vInflateRect(&rcWindow, -UserGetSystemMetrics(SM_CXDLGFRAME), -UserGetSystemMetrics(SM_CYDLGFRAME));
        else if (HAS_THINFRAME( Style, ExStyle ))
            RECTL_vInflateRect(&rcWindow, -UserGetSystemMetrics(SM_CXBORDER), -UserGetSystemMetrics(SM_CYBORDER));
        if (!RECTL_bPointInRect( &rcWindow, pt.x, pt.y  )) return HTBORDER;
    }

    /* Check caption */

    if ((Style & WS_CAPTION) == WS_CAPTION)
    {
        if (ExStyle & WS_EX_TOOLWINDOW)
            rcWindow.top += UserGetSystemMetrics(SM_CYSMCAPTION) - 1;
        else
            rcWindow.top += UserGetSystemMetrics(SM_CYCAPTION) - 1;
        if (!RECTL_bPointInRect( &rcWindow, pt.x, pt.y ))
        {
            BOOL min_or_max_box = (Style & WS_MAXIMIZEBOX) ||
                                  (Style & WS_MINIMIZEBOX);
            if (ExStyle & WS_EX_LAYOUTRTL)
            {
                /* Check system menu */
                if ((Style & WS_SYSMENU) && !(ExStyle & WS_EX_TOOLWINDOW) && NC_IconForWindow(pWnd))
                {
                    rcWindow.right -= UserGetSystemMetrics(SM_CYCAPTION) - 1;
                    if (pt.x > rcWindow.right) return HTSYSMENU;
                }

                /* Check close button */
                if (Style & WS_SYSMENU)
                {
                    rcWindow.left += UserGetSystemMetrics(SM_CYCAPTION);
                    if (pt.x < rcWindow.left) return HTCLOSE;
                }

                /* Check maximize box */
                /* In Win95 there is automatically a Maximize button when there is a minimize one */
                if (min_or_max_box && !(ExStyle & WS_EX_TOOLWINDOW))
                {
                    rcWindow.left += UserGetSystemMetrics(SM_CXSIZE);
                    if (pt.x < rcWindow.left) return HTMAXBUTTON;
                }

                /* Check minimize box */
                if (min_or_max_box && !(ExStyle & WS_EX_TOOLWINDOW))
                {
                    rcWindow.left += UserGetSystemMetrics(SM_CXSIZE);
                    if (pt.x < rcWindow.left) return HTMINBUTTON;
                }
            }
            else
            {
                /* Check system menu */
                if ((Style & WS_SYSMENU) && !(ExStyle & WS_EX_TOOLWINDOW) && NC_IconForWindow(pWnd))
                {
                    rcWindow.left += UserGetSystemMetrics(SM_CYCAPTION) - 1;
                    if (pt.x < rcWindow.left) return HTSYSMENU;
                }

                /* Check close button */
                if (Style & WS_SYSMENU)
                {
                    rcWindow.right -= UserGetSystemMetrics(SM_CYCAPTION);
                    if (pt.x > rcWindow.right) return HTCLOSE;
                }

                /* Check maximize box */
                /* In Win95 there is automatically a Maximize button when there is a minimize one */
                if (min_or_max_box && !(ExStyle & WS_EX_TOOLWINDOW))
                {
                    rcWindow.right -= UserGetSystemMetrics(SM_CXSIZE);
                    if (pt.x > rcWindow.right) return HTMAXBUTTON;
                }

                /* Check minimize box */
                if (min_or_max_box && !(ExStyle & WS_EX_TOOLWINDOW))
                {
                    rcWindow.right -= UserGetSystemMetrics(SM_CXSIZE);
                    if (pt.x > rcWindow.right) return HTMINBUTTON;
                }
            }
            return HTCAPTION;
        }
    }

      /* Check menu bar */

    if (HAS_MENU( pWnd, Style ) && (pt.y < rcClient.top) &&
        (pt.x >= rcClient.left) && (pt.x < rcClient.right))
        return HTMENU;

      /* Check vertical scroll bar */

    if (ExStyle & WS_EX_LAYOUTRTL) ExStyle ^= WS_EX_LEFTSCROLLBAR;
    if (Style & WS_VSCROLL)
    {
        if((ExStyle & WS_EX_LEFTSCROLLBAR) != 0)
            rcClient.left -= UserGetSystemMetrics(SM_CXVSCROLL);
        else
            rcClient.right += UserGetSystemMetrics(SM_CXVSCROLL);
        if (RECTL_bPointInRect( &rcClient, pt.x, pt.y )) return HTVSCROLL;
    }

      /* Check horizontal scroll bar */

    if (Style & WS_HSCROLL)
    {
        rcClient.bottom += UserGetSystemMetrics(SM_CYHSCROLL);
        if (RECTL_bPointInRect( &rcClient, pt.x, pt.y ))
        {
            /* Check size box */
            if ((Style & WS_VSCROLL) &&
                ((((ExStyle & WS_EX_LEFTSCROLLBAR) != 0) && (pt.x <= rcClient.left + UserGetSystemMetrics(SM_CXVSCROLL))) ||
                (((ExStyle & WS_EX_LEFTSCROLLBAR) == 0) && (pt.x >= rcClient.right - UserGetSystemMetrics(SM_CXVSCROLL)))))
                return HTSIZE;
            return HTHSCROLL;
        }
    }

    /* Has to return HTNOWHERE if nothing was found
       Could happen when a window has a customized non client area */
    return HTNOWHERE;
}

/* EOF */
