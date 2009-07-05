/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Misc User funcs
 * FILE:             subsystem/win32/win32k/ntuser/defwnd.c
 * PROGRAMER:
 *
 */

#include <w32k.h>

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
   PWINDOW_OBJECT pWindow,
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
          PWINDOW_OBJECT WndChild;

          if (!(WndChild = UserGetWindowObject(List[i])) || !WndChild->Wnd)
             continue;

          if (wParam & MCS_QUERYENDSESSION)
          {
             if (!co_IntSendMessage(WndChild->hSelf, WM_QUERYENDSESSION, 0, lParams))
             {
                lResult = MCSR_DONOTSHUTDOWN;
                break;
             }
          }
          else          
          {
             co_IntSendMessage(WndChild->hSelf, WM_ENDSESSION, KillTimers, lParams);
             if (KillTimers)
             {
                MsqRemoveTimersWindow(WndChild->MessageQueue, WndChild->hSelf);
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
      if (!co_IntSendMessage(pWindow->hSelf, WM_QUERYENDSESSION, 0, lParams))
      {
         lResult = MCSR_DONOTSHUTDOWN;
      }
   }
   else          
   {
      co_IntSendMessage(pWindow->hSelf, WM_ENDSESSION, KillTimers, lParams);
      if (KillTimers)
      {
         MsqRemoveTimersWindow(pWindow->MessageQueue, pWindow->hSelf);
      }
      lResult = MCSR_SHUTDOWNFINISHED;
   }
   return lResult;
}

/*
   Win32k counterpart of User DefWindowProc
 */
LRESULT FASTCALL
IntDefWindowProc(
   PWINDOW_OBJECT Window,
   UINT Msg,
   WPARAM wParam,
   LPARAM lParam,
   BOOL Ansi)
{
   PWINDOW Wnd;
   LRESULT lResult = 0;

   if (Msg > WM_USER) return 0;

   Wnd = Window->Wnd;
   if (!Wnd) return 0;

   switch (Msg)
   {
      case WM_SYSCOMMAND:
      {
         DPRINT1("hwnd %p WM_SYSCOMMAND %lx %lx\n", Window->hSelf, wParam, lParam );
         if (!ISITHOOKED(WH_CBT)) break;
         lResult = co_HOOK_CallHooks(WH_CBT, HCBT_SYSCOMMAND, wParam, lParam);
         break;
      }
      case WM_SHOWWINDOW:
      {
         if ((Wnd->Style & WS_VISIBLE) && wParam) break;
         if (!(Wnd->Style & WS_VISIBLE) && !wParam) break;
         if (!Window->hOwner) break;
         if (LOWORD(lParam))
         {
            if (wParam)
            {
               if (!(Window->Flags & WIN_NEEDS_SHOW_OWNEDPOPUP)) break;
               Window->Flags &= ~WIN_NEEDS_SHOW_OWNEDPOPUP;
            }
            else
                Window->Flags |= WIN_NEEDS_SHOW_OWNEDPOPUP;

            co_WinPosShowWindow(Window, wParam ? SW_SHOWNOACTIVATE : SW_HIDE);
         }
      }
      break;
      case WM_CLIENTSHUTDOWN:
         return IntClientShutdown(Window, wParam, lParam);

      case WM_CBT:
      {
         if (!ISITHOOKED(WH_CBT)) break;

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
                  lResult = co_HOOK_CallHooks(WH_CBT, HCBT_MOVESIZE, (WPARAM)Window->hSelf, lParam ? (LPARAM)&rt : 0);
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
