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

   if (Msg > WM_USER) return 0;

   Wnd = Window->Wnd;
   if (!Wnd) return 0;

   switch (Msg)
   {
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

   return 0;
}


/* EOF */
