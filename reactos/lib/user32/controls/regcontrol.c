/* $Id: regcontrol.c,v 1.9 2003/08/15 15:12:14 weiden Exp $
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
  WNDCLASSA wca;
  
  wca.lpszClassName = Descr->name;
  wca.lpfnWndProc = Descr->procA;
  wca.style = Descr->style;
  wca.hInstance = NULL;
  wca.hIcon = NULL;
  wca.hCursor = LoadCursorA(NULL, Descr->cursor);
  wca.hbrBackground = Descr->brush;
  wca.lpszMenuName = NULL;
  wca.cbClsExtra = 0;
  wca.cbWndExtra = Descr->extra;

#if 1
  if(IS_ATOM(wca.lpszClassName))
    DbgPrint("Registering built-in class atom=0x%x\n", wca.lpszClassName);
  else
    DbgPrint("Registering built-in class %s\n", wca.lpszClassName);
  DbgPrint("RegisterClassA = %d\n", RegisterClassA(&wca));
#endif
}

/***********************************************************************
 *           ControlsInit
 *
 * Register the classes for the builtin controls
 */
void ControlsInit(void)
{
    DbgPrint("ControlsInit()\n");

  RegisterBuiltinClass(&BUTTON_builtin_class);
  RegisterBuiltinClass(&DIALOG_builtin_class);
  RegisterBuiltinClass(&POPUPMENU_builtin_class);
#if 0
  RegisterBuiltinClass(&COMBO_builtin_class);
  RegisterBuiltinClass(&COMBOLBOX_builtin_class);
  RegisterBuiltinClass(&DESKTOP_builtin_class);
  RegisterBuiltinClass(&LISTBOX_builtin_class);
  RegisterBuiltinClass(&MDICLIENT_builtin_class);
  RegisterBuiltinClass(&MENU_builtin_class);
  RegisterBuiltinClass(&SCROLL_builtin_class);
#endif
  RegisterBuiltinClass(&EDIT_builtin_class);
  RegisterBuiltinClass(&COMBO_builtin_class);
  RegisterBuiltinClass(&ICONTITLE_builtin_class);
  RegisterBuiltinClass(&STATIC_builtin_class);
}
