/* $Id: regcontrol.c,v 1.18 2004/03/27 10:46:32 gvg Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS User32
 * PURPOSE:          Built-in control registration
 * FILE:             lib/user32/controls/regcontrol.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 * REVISION HISTORY: 2003/06/16 GvG Created
 * NOTES:            Adapted from Wine
 */

#include <windows.h>
#include <wchar.h>
#include "user32/regcontrol.h"
#include "win32k/ntuser.h"

static void RegisterBuiltinClass(const struct builtin_class_descr *Descr)
{
  WNDCLASSEXW wc;
  ATOM Class;
  
  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.lpszClassName = Descr->name;
  wc.lpfnWndProc = Descr->procW;
  wc.style = Descr->style;
  wc.hInstance = NULL;
  wc.hIcon = NULL;
  wc.hIconSm = NULL;
  wc.hCursor = LoadCursorW(NULL, Descr->cursor);
  wc.hbrBackground = Descr->brush;
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = Descr->extra;


#if 0
  if(IS_ATOM(wc.lpszClassName))
    DbgPrint("Registering built-in class atom=0x%x\n", wc.lpszClassName);
  else
    DbgPrint("Registering built-in class %wS\n", wc.lpszClassName);
#endif
  Class = NtUserRegisterClassExWOW(&wc,TRUE,Descr->procA,0,0);
#if 0
  DbgPrint("RegisterClassW = %d\n", Class);
#endif
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
