/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS User32
 * PURPOSE:          Built-in control registration
 * FILE:             include/user32/regcontrol.h
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 * REVISION HISTORY: 2003/06/16 GvG Created
 * NOTES:
 */
#ifndef ROS_REGCONTROL_H
#define ROS_REGCONTROL_H

#define IS_ATOM(x) \
  (((ULONG_PTR)(x) > 0x0) && ((ULONG_PTR)(x) < 0x10000))

/* Built-in class descriptor */
struct builtin_class_descr
{
    LPCWSTR name;    /* class name */
    UINT    style;   /* class style */
    WNDPROC procW;   /* Unicode window procedure */
    WNDPROC procA;   /* Ansi window procedure */
    INT     extra;   /* window extra bytes */
    LPCWSTR cursor;  /* cursor name */
    HBRUSH  brush;   /* brush or system color */
};

extern BOOL FASTCALL ControlsInit(LPCWSTR ClassName);

extern const struct builtin_class_descr BUTTON_builtin_class;
extern const struct builtin_class_descr COMBO_builtin_class;
extern const struct builtin_class_descr COMBOLBOX_builtin_class;
extern const struct builtin_class_descr DIALOG_builtin_class;
extern const struct builtin_class_descr POPUPMENU_builtin_class;
extern const struct builtin_class_descr DESKTOP_builtin_class;
extern const struct builtin_class_descr EDIT_builtin_class;
extern const struct builtin_class_descr ICONTITLE_builtin_class;
extern const struct builtin_class_descr LISTBOX_builtin_class;
extern const struct builtin_class_descr MDICLIENT_builtin_class;
extern const struct builtin_class_descr MENU_builtin_class;
extern const struct builtin_class_descr SCROLL_builtin_class;
extern const struct builtin_class_descr STATIC_builtin_class;

#endif /* ROS_REGCONTROL_H */
