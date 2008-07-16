/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Misc User funcs
 * FILE:             subsystem/win32/win32k/ntuser/defwnd.c
 * PROGRAMER:
 * REVISION HISTORY:
 *       2003/05/22  Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

LRESULT FASTCALL
IntDefWinHandleSysCommand( PWINDOW_OBJECT Window, WPARAM wParam, LPARAM lParam )
{
   DPRINT1("hwnd %p WM_SYSCOMMAND %lx %lx\n", Window->hSelf, wParam, lParam );

   if (co_HOOK_CallHooks(WH_CBT, HCBT_SYSCOMMAND, wParam, lParam))
      return 1;
   return 0;
}
/*
   Win32k counterpart of User DefWindowProc
 */
LRESULT FASTCALL
IntDefWindowProc(
   PWINDOW_OBJECT Window,
   UINT Msg,
   WPARAM wParam,
   LPARAM lParam)
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
          lResult = IntDefWinHandleSysCommand( Window, wParam, lParam );
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
   }

   return lResult;
}


/* EOF */
