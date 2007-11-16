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

static const struct
{
    const struct builtin_class_descr *desc;
    UINT ClsId;
} g_SysClasses[] =
{
    { &DIALOG_builtin_class,    CLASS_DIALOG },
    { &POPUPMENU_builtin_class, CLASS_POPUPMENU },
    { &COMBO_builtin_class,     CLASS_COMBO },
    { &COMBOLBOX_builtin_class, CLASS_COMBOLBOX },
    { &DESKTOP_builtin_class,   CLASS_DESKTOP },
    { &MDICLIENT_builtin_class, CLASS_MDICLIENT },
#if 0
    { &MENU_builtin_class,      CLASS_MENU },
#endif
    { &SCROLL_builtin_class,    CLASS_SCROLL },
    { &BUTTON_builtin_class,    CLASS_BUTTON },
    { &LISTBOX_builtin_class,   CLASS_LISTBOX },
    { &EDIT_builtin_class,      CLASS_EDIT },
    { &ICONTITLE_builtin_class, CLASS_ICONTITLE },
    { &STATIC_builtin_class,    CLASS_STATIC },
};

BOOL WINAPI RegisterSystemControls(VOID)
{
    REGISTER_SYSCLASS cls[sizeof(g_SysClasses) / sizeof(g_SysClasses[0])];
    UINT i;

    ZeroMemory(cls, sizeof(cls));

    for (i = 0; i != sizeof(cls) / sizeof(cls[0]); i++)
    {
        if (IS_ATOM(g_SysClasses[i].desc->name))
            cls[i].ClassName.Buffer = (PWSTR)((ULONG_PTR)g_SysClasses[i].desc->name);
        else
            RtlInitUnicodeString(&cls[i].ClassName, g_SysClasses[i].desc->name);

        cls[i].Style = g_SysClasses[i].desc->style;
        cls[i].ProcW = g_SysClasses[i].desc->procW;
        cls[i].ProcA = g_SysClasses[i].desc->procA;
        cls[i].ExtraBytes = g_SysClasses[i].desc->extra;
        cls[i].hCursor = LoadCursorW(NULL, g_SysClasses[i].desc->cursor);
        cls[i].hBrush = g_SysClasses[i].desc->brush;
        cls[i].ClassId = g_SysClasses[i].ClsId;
    }

    return NtUserRegisterSystemClasses(sizeof(cls) / sizeof(cls[0]), cls);
}
