/* $Id: regcontrol.c,v 1.4 2003/06/25 23:59:02 sedwards Exp $
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

  RegisterClassA(&wca);
  }

/***********************************************************************
 *           ControlsInit
 *
 * Register the classes for the builtin controls
 */
void ControlsInit(void)
{
#if 0
  RegisterBuiltinClass(&COMBO_builtin_class);
  RegisterBuiltinClass(&COMBOLBOX_builtin_class);
  RegisterBuiltinClass(&DESKTOP_builtin_class);
  RegisterBuiltinClass(&EDIT_builtin_class);
  RegisterBuiltinClass(&LISTBOX_builtin_class);
  RegisterBuiltinClass(&MDICLIENT_builtin_class);
  RegisterBuiltinClass(&MENU_builtin_class);
  RegisterBuiltinClass(&SCROLL_builtin_class);
#endif
  RegisterBuiltinClass(&COMBO_builtin_class);
  RegisterBuiltinClass(&BUTTON_builtin_class);
  RegisterBuiltinClass(&ICONTITLE_builtin_class);
  RegisterBuiltinClass(&STATIC_builtin_class);
}
