/*
 * Windows widgets (built-in window classes)
 *
 * Copyright 1993 Alexandre Julliard
 */

#define UNICODE
#include <windows.h>
#include <user32/widgets.h>
#include <user32/win.h>
#include <user32/button.h>
#include <user32/scroll.h>
#include <user32/static.h>
#include <user32/mdi.h>
#include <user32/dialog.h>
#include <user32/heapdup.h>


/* Built-in classes */

#define DLGWINDOWEXTRA sizeof(DIALOGINFO)


static WNDCLASS WIDGETS_BuiltinClasses[BIC_NB_CLASSES+1] =
{
    /* BIC_BUTTON */
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
      ButtonWndProc, 0, sizeof(BUTTONINFO), 0, 0,
      (HCURSOR)IDC_ARROW, 0, 0, BUTTON_CLASS_NAME },
    /* BIC_EDIT */
    { CS_GLOBALCLASS | CS_DBLCLKS /*| CS_PARENTDC*/,
      EditWndProc, 0, sizeof(void *), 0, 0,
      (HCURSOR)IDC_IBEAM, 0, 0, EDIT_CLASS_NAME  },
    /* BIC_LISTBOX */
    { CS_GLOBALCLASS | CS_DBLCLKS /*| CS_PARENTDC*/,
      ListBoxWndProc, 0, sizeof(void *), 0, 0,
      (HCURSOR)IDC_ARROW, 0, 0,  LISTBOX_CLASS_NAME },
    /* BIC_COMBO */
    { CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS, 
      ComboWndProc, 0, sizeof(void *), 0, 0,
      (HCURSOR)IDC_ARROW, 0, 0, COMBOBOX_CLASS_NAME },
    /* BIC_COMBOLB */
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS, ComboLBWndProc,
      0, sizeof(void *), 0, 0, (HCURSOR)IDC_ARROW, 0, 0,COMBOLBOX_CLASS_NAME},
    /* BIC_POPUPMENU */
//    { CS_GLOBALCLASS | CS_SAVEBITS, PopupMenuWndProc, 0, sizeof(HMENU),
//      0, 0, (HCURSOR)IDC_ARROW, NULL_BRUSH, 0, POPUPMENU_CLASS_NAME },
    /* BIC_STATIC */
    { CS_GLOBALCLASS | CS_PARENTDC, StaticWndProc,
      0, sizeof(STATICINFO), 0, 0, (HCURSOR)IDC_ARROW, 0, 0,  STATIC_CLASS_NAME  },
    /* BIC_SCROLL */
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
      ScrollBarWndProc, 0, sizeof(SCROLLBAR_INFO), 0, 0,
      (HCURSOR)IDC_ARROW, 0, 0, SCROLLBAR_CLASS_NAME},
    /* BIC_MDICLIENT */
//    { CS_GLOBALCLASS, MDIClientWndProc,
//      0, sizeof(MDICLIENTINFO), 0, 0, 0, LTGRAY_BRUSH, 0, "MDIClient" },
    /* BIC_DESKTOP */
 //   { CS_GLOBALCLASS, DesktopWndProc, 0, sizeof(DESKTOPINFO),
 //     0, 0, (HCURSOR)IDC_ARROW, 0, 0, DESKTOP_CLASS_NAME },
    /* BIC_DIALOG */
    { CS_GLOBALCLASS | CS_SAVEBITS, DefDlgProc, 100, 100,
      0, 0, (HCURSOR)IDC_ARROW, 0, 0, DIALOG_CLASS_NAMEW },
    /* BIC_ICONTITLE */
    { CS_GLOBALCLASS, IconTitleWndProc, 0, 0, 
      0, 0, (HCURSOR)IDC_ARROW, 0, 0, ICONTITLE_CLASS_NAME },
    /* BIC_DIALOG Ascii */
    { CS_GLOBALCLASS, DefDlgProcA, 100,  100,
      0, 0, (HCURSOR)IDC_ARROW, 0, 0, (LPWSTR)DIALOG_CLASS_NAME_A }
};


static ATOM bicAtomTable[BIC_NB_CLASSES+1];

/***********************************************************************
 *           WIDGETS_Init
 * 
 * Initialize the built-in window classes.
 */
WINBOOL WIDGETS_Init(void)
{
    int i;
    WNDCLASS *cls = WIDGETS_BuiltinClasses;

    /* Create builtin classes */

    for (i = 0; i < BIC_NB_CLASSES; i++)
    {

        cls[i].hCursor = LoadCursorW( 0, (LPCWSTR)cls[i].hCursor );
        if (!(bicAtomTable[i] = RegisterClassW( &cls[i] ))) return FALSE;
    }

    cls[i].hCursor = LoadCursorW( 0, (LPCWSTR)cls[i].hCursor );
    if (!(bicAtomTable[i] = RegisterClassA( &cls[i] ))) return FALSE;


    return TRUE;
}


/***********************************************************************
 *           WIDGETS_IsControl
 *
 * Check whether pWnd is a built-in control or not.
 */
WINBOOL	WIDGETS_IsControl( WND* pWnd, BUILTIN_CLASS cls )
{
    if(  cls >= BIC_NB_CLASSES )
	return FALSE;
    return (pWnd->class->atomName == bicAtomTable[cls]);
}
