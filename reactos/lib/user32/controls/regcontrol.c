/* $Id: regcontrol.c,v 1.21 2004/12/21 21:38:26 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS User32
 * PURPOSE:          Built-in control registration
 * FILE:             lib/user32/controls/regcontrol.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 * REVISION HISTORY: 2003/06/16 GvG Created
 * NOTES:            Adapted from Wine
 */

#include "user32.h"
#include "user32/regcontrol.h"

#define NDEBUG
#include <debug.h>

/***********************************************************************
 *           PrivateCsrssRegisterBuiltinSystemWindowClasses
 *
 * Register the classes for the builtin controls - Private to CSRSS!
 */
BOOL STDCALL
PrivateCsrssRegisterBuiltinSystemWindowClasses(HWINSTA hWindowStation)
{
  static const struct builtin_class_descr *ClassDescriptions[] =
    {
      &DIALOG_builtin_class,
      &POPUPMENU_builtin_class,
      &COMBO_builtin_class,
      &COMBOLBOX_builtin_class,
#if 0
      &DESKTOP_builtin_class,
#endif
      &MDICLIENT_builtin_class,
#if 0
      &MENU_builtin_class,
#endif
      &SCROLL_builtin_class,
      &BUTTON_builtin_class,
      &LISTBOX_builtin_class,
      &EDIT_builtin_class,
      &ICONTITLE_builtin_class,
      &STATIC_builtin_class
    };
  const struct builtin_class_descr *Descr;
  int i;
  
  for (i = 0; i < sizeof(ClassDescriptions) / sizeof(ClassDescriptions[0]); i++)
    {
       WNDCLASSEXW wc;
       UNICODE_STRING ClassName;
       UNICODE_STRING MenuName;

       Descr = ClassDescriptions[i];

       wc.cbSize = sizeof(WNDCLASSEXW);
       wc.lpszClassName = Descr->name;
       wc.lpfnWndProc = Descr->procW;
       wc.style = Descr->style;
       wc.hInstance = User32Instance;
       wc.hIcon = NULL;
       wc.hIconSm = NULL;
       /* don't load the cursor or icons! the system classes will load cursors
          and icons from the resources when duplicating the classes into the
          process class list - which happens when creating a window or
          overwriting the classes using RegisterClass! */
       wc.hCursor = (HCURSOR)Descr->cursor;
       wc.hbrBackground = Descr->brush;
       wc.lpszMenuName = NULL;
       wc.cbClsExtra = 0;
       wc.cbWndExtra = Descr->extra;

       MenuName.Length =
       MenuName.MaximumLength = 0;
       MenuName.Buffer = NULL;

       if (IS_ATOM(Descr->name))
       {
          ClassName.Length =
          ClassName.MaximumLength = 0;
          ClassName.Buffer = (LPWSTR)Descr->name;
       } else
       {
          RtlInitUnicodeString(&ClassName, Descr->name);
       }

       if(!NtUserRegisterClassEx(
          &wc,
          &ClassName,
          &MenuName,
          Descr->procA,
          REGISTERCLASS_SYSTEM,
          hWindowStation))
       {
         if(IS_ATOM(Descr->name))
         {
           DPRINT("Failed to register builtin class %ws\n", Descr->name);
         }
         else
         {
           DPRINT("Failed to register builtin class (Atom 0x%x)\n", Descr->name);
         }
         return FALSE;
       }
    }

  return TRUE;
}


/***********************************************************************
 *           PrivateCsrssRegisterSystemWindowClass
 *
 * Register a system window class - Private to CSRSS!
 */
ATOM STDCALL
PrivateCsrssRegisterSystemWindowClass(HWINSTA hWindowStation, WNDCLASSEXW *lpwcx, WNDPROC lpfnWndProcA)
{
   UNICODE_STRING ClassName;
   UNICODE_STRING MenuName;
   
   MenuName.Length =
   MenuName.MaximumLength = 0;
   MenuName.Buffer = NULL;

   if (IS_ATOM(lpwcx->lpszClassName))
   {
      ClassName.Length =
      ClassName.MaximumLength = 0;
      ClassName.Buffer = (LPWSTR)lpwcx->lpszClassName;
   } else
   {
      RtlInitUnicodeString(&ClassName, lpwcx->lpszClassName);
   }

   return NtUserRegisterClassEx(
      lpwcx,
      &ClassName,
      &MenuName,
      lpfnWndProcA,
      REGISTERCLASS_SYSTEM,
      hWindowStation);
}
