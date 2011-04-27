/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Misc User funcs
 * FILE:             subsystem/win32/win32k/ntuser/defwnd.c
 * PROGRAMER:
 *
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

// Client Shutdown messages
#define MCS_SHUTDOWNTIMERS  1
#define MCS_QUERYENDSESSION 2
// Client Shutdown returns
#define MCSR_GOODFORSHUTDOWN  1
#define MCSR_SHUTDOWNFINISHED 2
#define MCSR_DONOTSHUTDOWN    3

/*
  Based on CSRSS and described in pages 1115 - 1118 "Windows Internals, Fifth Edition".
  Apparently CSRSS sends out messages to do this w/o going into win32k internals.
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
        DPRINT1("Screensaver Called!\n");
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
         DPRINT1("hwnd %p WM_SYSCOMMAND %lx %lx\n", Wnd->head.h, wParam, lParam );
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

      case WM_GETHOTKEY:
         return DefWndGetHotKey(UserHMGetHandle(Wnd));                
      case WM_SETHOTKEY:
         return DefWndSetHotKey(Wnd, wParam);

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
