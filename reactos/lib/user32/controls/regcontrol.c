/* $Id: regcontrol.c,v 1.14 2003/11/02 06:58:57 navaraf Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS User32
 * PURPOSE:          Built-in control registration
 * FILE:             lib/user32/controls/regcontrol.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 * REVISION HISTORY: 2003/06/16 GvG Created
 * NOTES:            Adapted from Wine
 */

#include "windows.h"
#include "user32/regcontrol.h"

static void RegisterBuiltinClass(const struct builtin_class_descr *Descr)
{
  WNDCLASSW wc;
  ATOM Class;
  
  wc.lpszClassName = Descr->name;
  wc.lpfnWndProc = Descr->procW;
  wc.style = Descr->style;
  wc.hInstance = NULL;
  wc.hIcon = NULL;
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
  Class = RegisterClassW(&wc);
#if 0
  DbgPrint("RegisterClassW = %d\n", Class);
#endif
}

/***********************************************************************
 *           ControlsInit
 *
 * Register the classes for the builtin controls
 */
void ControlsInit(void)
{
#if 0
  DbgPrint("ControlsInit()\n");
#endif

  RegisterBuiltinClass(&DIALOG_builtin_class);
  RegisterBuiltinClass(&POPUPMENU_builtin_class);
#if 0
  RegisterBuiltinClass(&COMBO_builtin_class);
  RegisterBuiltinClass(&COMBOLBOX_builtin_class);
  RegisterBuiltinClass(&DESKTOP_builtin_class);
#endif
  RegisterBuiltinClass(&MDICLIENT_builtin_class);
#if 0
  RegisterBuiltinClass(&MENU_builtin_class);
  RegisterBuiltinClass(&SCROLL_builtin_class);
#endif
  RegisterBuiltinClass(&BUTTON_builtin_class);
  RegisterBuiltinClass(&LISTBOX_builtin_class);
  RegisterBuiltinClass(&EDIT_builtin_class);
  RegisterBuiltinClass(&COMBO_builtin_class);
  RegisterBuiltinClass(&ICONTITLE_builtin_class);
  RegisterBuiltinClass(&STATIC_builtin_class);
}
