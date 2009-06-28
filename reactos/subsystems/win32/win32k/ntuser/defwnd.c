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
