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
    { &DIALOG_builtin_class,    FNID_DIALOG },
    { &POPUPMENU_builtin_class, FNID_MENU },
    { &COMBO_builtin_class,     FNID_COMBOBOX },
    { &COMBOLBOX_builtin_class, FNID_COMBOLBOX },
    { &DESKTOP_builtin_class,   FNID_DESKTOP },
    { &MDICLIENT_builtin_class, FNID_MDICLIENT },
#if 0
    { &MENU_builtin_class,      FNID_MENU },
#endif
    { &SCROLL_builtin_class,    FNID_SCROLLBAR },
    { &BUTTON_builtin_class,    FNID_BUTTON },
    { &LISTBOX_builtin_class,   FNID_LISTBOX },
    { &EDIT_builtin_class,      FNID_EDIT },
    { &ICONTITLE_builtin_class, FNID_ICONTITLE },
    { &STATIC_builtin_class,    FNID_STATIC },
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


#if 0
static PFNCLIENT pfnClientA;
static PFNCLIENT pfnClientW;
static PFNCLIENTWORKER pfnClientWorker;

BOOL WINAPI RegisterClientPFN(VOID)
{
  NTSTATUS Status;

  pfnClientA.pfnScrollBarWndProc      = ScrollBarWndProcA;
  pfnClientW.pfnScrollBarWndProc      = ScrollBarWndProcW;
  pfnClientA.pfnTitleWndProc          = IconTitleWndProc;
  pfnClientW.pfnTitleWndProc          = IconTitleWndProc;
  pfnClientA.pfnMenuWndProc           = PopupMenuWndProcA;
  pfnClientW.pfnMenuWndProc           = PopupMenuWndProcW;
  pfnClientA.pfnDesktopWndProc        = DesktopWndProc; // Fixme!
  pfnClientW.pfnDesktopWndProc        = DesktopWndProc;
  pfnClientA.pfnDefWindowProc         = DefWindowProcA;
  pfnClientW.pfnDefWindowProc         = DefWindowProcW;
  pfnClientA.pfnMessageWindowProc     = DefWindowProcA;
  pfnClientW.pfnMessageWindowProc     = DefWindowProcW;
  pfnClientA.pfnSwitchWindowProc      = DefWindowProcA;
  pfnClientW.pfnSwitchWindowProc      = DefWindowProcW;
  pfnClientA.pfnButtonWndProc         = ButtonWndProcA;
  pfnClientW.pfnButtonWndProc         = ButtonWndProcW
  pfnClientA.pfnComboBoxWndProc       = ComboWndProcA;
  pfnClientW.pfnComboBoxWndProc       = ComboWndProcW;
  pfnClientA.pfnComboListBoxProc      = ListBoxWndProcA;
  pfnClientW.pfnComboListBoxProc      = ListBoxWndProcW;
  pfnClientA.pfnDialogWndProc         = DefDlgProcA;
  pfnClientW.pfnDialogWndProc         = DefDlgProcW;
  pfnClientA.pfnEditWndProc           = EditWndProcA;
  pfnClientW.pfnEditWndProc           = EditWndProcW;
  pfnClientA.pfnListBoxWndProc        = ListBoxWndProcA;
  pfnClientW.pfnListBoxWndProc        = ListBoxWndProcW;
  pfnClientA.pfnMDIClientWndProc      = MDIClientWndProcA;
  pfnClientW.pfnMDIClientWndProc      = MDIClientWndProcW;
  pfnClientA.pfnStaticWndProc         = StaticWndProcA;
  pfnClientW.pfnStaticWndProc         = StaticWndProcW;
  pfnClientA.pfnImeWndProc            = DefWindowProcA;
  pfnClientW.pfnImeWndProc            = DefWindowProcW;
  pfnClientA.pfnGhostWndProc          = DefWindowProcA;
  pfnClientW.pfnGhostWndProc          = DefWindowProcW;
  pfnClientA.pfnHkINLPCWPSTRUCT       = DefWindowProcA;
  pfnClientW.pfnHkINLPCWPSTRUCT       = DefWindowProcW;
  pfnClientA.pfnHkINLPCWPRETSTRUCT    = DefWindowProcA;
  pfnClientW.pfnHkINLPCWPRETSTRUCT    = DefWindowProcW;
  pfnClientA.pfnDispatchHook          = DefWindowProcA;
  pfnClientW.pfnDispatchHook          = DefWindowProcW;
  pfnClientA.pfnDispatchDefWindowProc = DefWindowProcA;
  pfnClientW.pfnDispatchDefWindowProc = DefWindowProcW;
  pfnClientA.pfnDispatchMessage       = DefWindowProcA;
  pfnClientW.pfnDispatchMessage       = DefWindowProcW;
  pfnClientA.pfnMDIActivateDlgProc    = DefWindowProcA;
  pfnClientW.pfnMDIActivateDlgProc    = DefWindowProcW;

  pfnClientWorker.pfnButtonWndProc    = ButtonWndProc_common;
  pfnClientWorker.pfnComboBoxWndProc  = ComboWndProc_common;
  pfnClientWorker.pfnComboListBoxProc = ListBoxWndProc_common;
//  pfnClientWorker.pfnDialogWndProc    = DefDlgProc_common;
  pfnClientWorker.pfnEditWndProc      = EditWndProc_common;
  pfnClientWorker.pfnListBoxWndProc   = ListBoxWndProc_common;
  pfnClientWorker.pfnMDIClientWndProc = MDIClientWndProc_common;
  pfnClientWorker.pfnStaticWndProc    = StaticWndProc_common;
  pfnClientWorker.pfnImeWndProc       = User32DefWindowProc;
  pfnClientWorker.pfnGhostWndProc     = User32DefWindowProc;
  pfnClientWorker.pfnCtfHookProc      = User32DefWindowProc;

  Status = NtUserInitializeClientPfnArrays( &pfnClientA,
                                            &pfnClientW,
                                            &pfnClientWorker,
                                            User32Instance);
  
  return NT_SUCCESS(Status) ? TRUE : FALSE;
}
#endif
