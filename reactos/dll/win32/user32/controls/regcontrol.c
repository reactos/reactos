/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS User32
 * PURPOSE:          Built-in control registration
 * FILE:             lib/user32/controls/regcontrol.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 * REVISION HISTORY: 2003/06/16 GvG Created
 * NOTES:            Adapted from Wine
 */

#include <user32.h>

#include <wine/debug.h>

static void RegisterBuiltinClass(const struct builtin_class_descr *Descr)
{
   WNDCLASSEXW wc;
   UNICODE_STRING ClassName;
   UNICODE_STRING MenuName;

   wc.cbSize = sizeof(WNDCLASSEXW);
   wc.lpszClassName = Descr->name;
   wc.lpfnWndProc = Descr->procW;
   wc.style = Descr->style;
   wc.hInstance = User32Instance;
   wc.hIcon = NULL;
   wc.hIconSm = NULL;
   wc.hCursor = LoadCursorW(NULL, Descr->cursor);
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

   NtUserRegisterClassEx(
      &wc,
      &ClassName,
      &MenuName,
      Descr->procA,
      REGISTERCLASS_SYSTEM,
      NULL);
}

/***********************************************************************
 *           ControlsInit
 *
 * Register the classes for the builtin controls
 */
BOOL FASTCALL
ControlsInit(LPCWSTR ClassName)
{
  static const struct builtin_class_descr *ClassDescriptions[] =
    {
      &DIALOG_builtin_class,
      &POPUPMENU_builtin_class,
      &COMBO_builtin_class,
      &COMBOLBOX_builtin_class,
      &DESKTOP_builtin_class,
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
  unsigned i;
  BOOL Register;

  Register = FALSE;
  if (IS_ATOM(ClassName))
    {
      for (i = 0;
           ! Register && i < sizeof(ClassDescriptions) / sizeof(ClassDescriptions[0]);
           i++)
        {
          if (IS_ATOM(ClassDescriptions[i]->name))
            {
              Register = (ClassName == ClassDescriptions[i]->name);
            }
        }
    }
  else
    {
      for (i = 0;
           ! Register && i < sizeof(ClassDescriptions) / sizeof(ClassDescriptions[0]);
           i++)
        {
          if (! IS_ATOM(ClassDescriptions[i]->name))
            {
              Register = (0 == _wcsicmp(ClassName, ClassDescriptions[i]->name));
            }
        }
    }

  if (Register)
    {
      for (i = 0; i < sizeof(ClassDescriptions) / sizeof(ClassDescriptions[0]); i++)
        {
          RegisterBuiltinClass(ClassDescriptions[i]);
        }
    }

  return Register;
}
