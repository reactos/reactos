/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS User32
 * PURPOSE:          Built-in control registration
 * FILE:             win32ss/user/user32/controls/regcontrol.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 * REVISION HISTORY: 2003/06/16 GvG Created
 * NOTES:            Adapted from Wine
 */

#include <user32.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(user32);

DWORD RegisterDefaultClasses = FALSE;

static PFNCLIENT pfnClientA;
static PFNCLIENT pfnClientW;
static PFNCLIENTWORKER pfnClientWorker;


/***********************************************************************
 *           set_control_clipping
 *
 * Set clipping for a builtin control that uses CS_PARENTDC.
 * Return the previous clip region if any.
 */
HRGN set_control_clipping( HDC hdc, const RECT *rect )
{
    RECT rc = *rect;
    HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );

    if (GetClipRgn( hdc, hrgn ) != 1)
    {
        DeleteObject( hrgn );
        hrgn = 0;
    }
    DPtoLP( hdc, (POINT *)&rc, 2 );
    if (GetLayout( hdc ) & LAYOUT_RTL)  /* compensate for the shifting done by IntersectClipRect */
    {
        rc.left++;
        rc.right++;
    }
    IntersectClipRect( hdc, rc.left, rc.top, rc.right, rc.bottom );
    return hrgn;
}

static const struct
{
    const struct builtin_class_descr *desc;
    WORD fnid;
    WORD ClsId;
} g_SysClasses[] =
{
    { &DIALOG_builtin_class,    FNID_DIALOG,    ICLS_DIALOG},
/*    { &POPUPMENU_builtin_class, FNID_MENU,      ICLS_MENU},     // moved to win32k */
    { &COMBO_builtin_class,     FNID_COMBOBOX,  ICLS_COMBOBOX},
    { &COMBOLBOX_builtin_class, FNID_COMBOLBOX, ICLS_COMBOLBOX},
    { &MDICLIENT_builtin_class, FNID_MDICLIENT, ICLS_MDICLIENT},
#if 0
    { &MENU_builtin_class,      FNID_MENU,      ICLS_MENU},
#endif
/*    { &SCROLL_builtin_class,    FNID_SCROLLBAR, ICLS_SCROLLBAR}, // moved to win32k */
    { &BUTTON_builtin_class,    FNID_BUTTON,    ICLS_BUTTON},
    { &LISTBOX_builtin_class,   FNID_LISTBOX,   ICLS_LISTBOX},
    { &EDIT_builtin_class,      FNID_EDIT,      ICLS_EDIT},
/*    { &ICONTITLE_builtin_class, FNID_ICONTITLE, ICLS_ICONTITLE}, // moved to win32k */
    { &STATIC_builtin_class,    FNID_STATIC,    ICLS_STATIC},
};

BOOL WINAPI RegisterSystemControls(VOID)
{
    WNDCLASSEXW WndClass;
    UINT i;
    ATOM atom;

    if (RegisterDefaultClasses) return TRUE;

    ZeroMemory(&WndClass, sizeof(WndClass));

    WndClass.cbSize = sizeof(WndClass);

    for (i = 0; i != sizeof(g_SysClasses) / sizeof(g_SysClasses[0]); i++)
    {
        WndClass.lpszClassName = g_SysClasses[i].desc->name;

        // Set Global bit!
        WndClass.style = g_SysClasses[i].desc->style|CS_GLOBALCLASS;
        WndClass.lpfnWndProc = g_SysClasses[i].desc->procW;
        WndClass.cbWndExtra = g_SysClasses[i].desc->extra;
        WndClass.hCursor = LoadCursorW(NULL, g_SysClasses[i].desc->cursor);
        WndClass.hbrBackground= g_SysClasses[i].desc->brush;

        atom = RegisterClassExWOWW( &WndClass,
                                     0,
                                     g_SysClasses[i].fnid,
                                     0,
                                     FALSE);
        if (atom)
           RegisterDefaultClasses |= ICLASS_TO_MASK(g_SysClasses[i].ClsId);
    }

    if ( //gpsi->dwSRVIFlags & SRVINFO_IMM32 && Not supported yet, need NlsMbCodePageTag working in Win32k.
        !(RegisterDefaultClasses & ICLASS_TO_MASK(ICLS_IME))) // So, work like XP.
    {
       RegisterIMEClass();
    }

    return TRUE;
}

LRESULT
WINAPI
MsgWindowProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PWND pWnd;

    pWnd = ValidateHwnd(hwnd);
    if (pWnd)
    {
       if (!pWnd->fnid)
       {
          NtUserSetWindowFNID(hwnd, FNID_MESSAGEWND);
       }
    }

    if (message == WM_NCCREATE) return TRUE;

    if (message == WM_DESTROY)
       NtUserSetWindowFNID(hwnd, FNID_DESTROY);

    return DefWindowProc(hwnd, message, wParam, lParam );
}

LRESULT
WINAPI
DialogWndProc_common( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL unicode)
{
  if (unicode)
     return DefDlgProcW( hwnd, uMsg, wParam, lParam);
  return DefDlgProcA( hwnd, uMsg, wParam, lParam);
}

BOOL WINAPI RegisterClientPFN(VOID)
{
  NTSTATUS Status;

  pfnClientA.pfnScrollBarWndProc      = ScrollBarWndProcA;
  pfnClientW.pfnScrollBarWndProc      = ScrollBarWndProcW;
  pfnClientA.pfnTitleWndProc          = IconTitleWndProc;
  pfnClientW.pfnTitleWndProc          = IconTitleWndProc;
  pfnClientA.pfnMenuWndProc           = PopupMenuWndProcA;
  pfnClientW.pfnMenuWndProc           = PopupMenuWndProcW;
  pfnClientA.pfnDesktopWndProc        = DesktopWndProcA;
  pfnClientW.pfnDesktopWndProc        = DesktopWndProcW;
  pfnClientA.pfnDefWindowProc         = DefWindowProcA;
  pfnClientW.pfnDefWindowProc         = DefWindowProcW;
  pfnClientA.pfnMessageWindowProc     = MsgWindowProc;
  pfnClientW.pfnMessageWindowProc     = MsgWindowProc;
  pfnClientA.pfnSwitchWindowProc      = SwitchWndProcA;
  pfnClientW.pfnSwitchWindowProc      = SwitchWndProcW;
  pfnClientA.pfnButtonWndProc         = ButtonWndProcA;
  pfnClientW.pfnButtonWndProc         = ButtonWndProcW;
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
  pfnClientA.pfnImeWndProc            = ImeWndProcA;
  pfnClientW.pfnImeWndProc            = ImeWndProcW;
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
  pfnClientWorker.pfnDialogWndProc    = DialogWndProc_common;
  pfnClientWorker.pfnEditWndProc      = EditWndProc_common;
  pfnClientWorker.pfnListBoxWndProc   = ListBoxWndProc_common;
  pfnClientWorker.pfnMDIClientWndProc = MDIClientWndProc_common;
  pfnClientWorker.pfnStaticWndProc    = StaticWndProc_common;
  pfnClientWorker.pfnImeWndProc       = ImeWndProc_common;
  pfnClientWorker.pfnGhostWndProc     = User32DefWindowProc;
  pfnClientWorker.pfnCtfHookProc      = User32DefWindowProc;

  Status = NtUserInitializeClientPfnArrays( &pfnClientA,
                                            &pfnClientW,
                                            &pfnClientWorker,
                                            User32Instance);
  
  return NT_SUCCESS(Status) ? TRUE : FALSE;
}
