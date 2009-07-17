/*
 * 16-bit dialog functions
 *
 * Copyright 1993, 1994, 1996, 2003 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wownt32.h"
#include "wine/winuser16.h"
#include "controls.h"
#include "win.h"
#include "user_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dialog);

/* Dialog control information */
typedef struct
{
    DWORD      style;
    INT16      x;
    INT16      y;
    INT16      cx;
    INT16      cy;
    UINT       id;
    LPCSTR     className;
    LPCSTR     windowName;
    LPCVOID    data;
} DLG_CONTROL_INFO;

  /* Dialog template */
typedef struct
{
    DWORD      style;
    UINT16     nbItems;
    INT16      x;
    INT16      y;
    INT16      cx;
    INT16      cy;
    LPCSTR     menuName;
    LPCSTR     className;
    LPCSTR     caption;
    INT16      pointSize;
    LPCSTR     faceName;
} DLG_TEMPLATE;


/***********************************************************************
 *           DIALOG_GetControl16
 *
 * Return the class and text of the control pointed to by ptr,
 * fill the header structure and return a pointer to the next control.
 */
static LPCSTR DIALOG_GetControl16( LPCSTR p, DLG_CONTROL_INFO *info )
{
    static char buffer[10];
    int int_id;

    info->x       = GET_WORD(p);  p += sizeof(WORD);
    info->y       = GET_WORD(p);  p += sizeof(WORD);
    info->cx      = GET_WORD(p);  p += sizeof(WORD);
    info->cy      = GET_WORD(p);  p += sizeof(WORD);
    info->id      = GET_WORD(p);  p += sizeof(WORD);
    info->style   = GET_DWORD(p); p += sizeof(DWORD);

    if (*p & 0x80)
    {
        switch((BYTE)*p)
        {
            case 0x80: strcpy( buffer, "BUTTON" ); break;
            case 0x81: strcpy( buffer, "EDIT" ); break;
            case 0x82: strcpy( buffer, "STATIC" ); break;
            case 0x83: strcpy( buffer, "LISTBOX" ); break;
            case 0x84: strcpy( buffer, "SCROLLBAR" ); break;
            case 0x85: strcpy( buffer, "COMBOBOX" ); break;
            default:   buffer[0] = '\0'; break;
        }
        info->className = buffer;
        p++;
    }
    else
    {
        info->className = p;
        p += strlen(p) + 1;
    }

    int_id = ((BYTE)*p == 0xff);
    if (int_id)
    {
        /* Integer id, not documented (?). Only works for SS_ICON controls */
        info->windowName = MAKEINTRESOURCEA(GET_WORD(p+1));
        p += 3;
    }
    else
    {
        info->windowName = p;
        p += strlen(p) + 1;
    }

    if (*p) info->data = p + 1;
    else info->data = NULL;

    p += *p + 1;

    TRACE("   %s %s %d, %d, %d, %d, %d, %08x, %p\n",
          debugstr_a(info->className),  debugstr_a(info->windowName),
          info->id, info->x, info->y, info->cx, info->cy,
          info->style, info->data );

    return p;
}


/***********************************************************************
 *           DIALOG_CreateControls16
 *
 * Create the control windows for a dialog.
 */
static BOOL DIALOG_CreateControls16( HWND hwnd, LPCSTR template,
                                     const DLG_TEMPLATE *dlgTemplate, HINSTANCE16 hInst )
{
    DIALOGINFO *dlgInfo = DIALOG_get_info( hwnd, TRUE );
    DLG_CONTROL_INFO info;
    HWND hwndCtrl, hwndDefButton = 0;
    INT items = dlgTemplate->nbItems;

    TRACE(" BEGIN\n" );
    while (items--)
    {
        HINSTANCE16 instance = hInst;
        SEGPTR segptr;

        template = DIALOG_GetControl16( template, &info );
        if (HIWORD(info.className) && !strcmp( info.className, "EDIT") &&
            !(GetWindowLongW( hwnd, GWL_STYLE ) & DS_LOCALEDIT))
        {
            if (!dlgInfo->hDialogHeap)
            {
                dlgInfo->hDialogHeap = GlobalAlloc16(GMEM_FIXED, 0x10000);
                if (!dlgInfo->hDialogHeap)
                {
                    ERR("Insufficient memory to create heap for edit control\n" );
                    continue;
                }
                LocalInit16(dlgInfo->hDialogHeap, 0, 0xffff);
            }
            instance = dlgInfo->hDialogHeap;
        }

        segptr = MapLS( info.data );
        hwndCtrl = WIN_Handle32( CreateWindowEx16( WS_EX_NOPARENTNOTIFY,
                                                   info.className, info.windowName,
                                                   info.style | WS_CHILD,
                                                   MulDiv(info.x, dlgInfo->xBaseUnit, 4),
                                                   MulDiv(info.y, dlgInfo->yBaseUnit, 8),
                                                   MulDiv(info.cx, dlgInfo->xBaseUnit, 4),
                                                   MulDiv(info.cy, dlgInfo->yBaseUnit, 8),
                                                   HWND_16(hwnd), (HMENU16)info.id,
                                                   instance, (LPVOID)segptr ));
        UnMapLS( segptr );

        if (!hwndCtrl)
        {
            if (dlgTemplate->style & DS_NOFAILCREATE) continue;
            return FALSE;
        }

            /* Send initialisation messages to the control */
        if (dlgInfo->hUserFont) SendMessageA( hwndCtrl, WM_SETFONT,
                                             (WPARAM)dlgInfo->hUserFont, 0 );
        if (SendMessageA(hwndCtrl, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON)
        {
              /* If there's already a default push-button, set it back */
              /* to normal and use this one instead. */
            if (hwndDefButton)
                SendMessageA( hwndDefButton, BM_SETSTYLE,
                                BS_PUSHBUTTON,FALSE );
            hwndDefButton = hwndCtrl;
            dlgInfo->idResult = GetWindowLongPtrA( hwndCtrl, GWLP_ID );
        }
    }
    TRACE(" END\n" );
    return TRUE;
}


/***********************************************************************
 *           DIALOG_ParseTemplate16
 *
 * Fill a DLG_TEMPLATE structure from the dialog template, and return
 * a pointer to the first control.
 */
static LPCSTR DIALOG_ParseTemplate16( LPCSTR p, DLG_TEMPLATE * result )
{
    result->style   = GET_DWORD(p); p += sizeof(DWORD);
    result->nbItems = (unsigned char) *p++;
    result->x       = GET_WORD(p);  p += sizeof(WORD);
    result->y       = GET_WORD(p);  p += sizeof(WORD);
    result->cx      = GET_WORD(p);  p += sizeof(WORD);
    result->cy      = GET_WORD(p);  p += sizeof(WORD);

    TRACE("DIALOG %d, %d, %d, %d\n", result->x, result->y, result->cx, result->cy );
    TRACE(" STYLE %08x\n", result->style );

    /* Get the menu name */

    switch( (BYTE)*p )
    {
    case 0:
        result->menuName = 0;
        p++;
        break;
    case 0xff:
        result->menuName = MAKEINTRESOURCEA(GET_WORD( p + 1 ));
        p += 3;
        TRACE(" MENU %04x\n", LOWORD(result->menuName) );
        break;
    default:
        result->menuName = p;
        TRACE(" MENU '%s'\n", p );
        p += strlen(p) + 1;
        break;
    }

    /* Get the class name */

    if (*p)
    {
        result->className = p;
        TRACE(" CLASS '%s'\n", result->className );
    }
    else result->className = (LPCSTR)DIALOG_CLASS_ATOM;
    p += strlen(p) + 1;

    /* Get the window caption */

    result->caption = p;
    p += strlen(p) + 1;
    TRACE(" CAPTION '%s'\n", result->caption );

    /* Get the font name */

    result->pointSize = 0;
    result->faceName = NULL;

    if (result->style & DS_SETFONT)
    {
        result->pointSize = GET_WORD(p);
        p += sizeof(WORD);
        result->faceName = p;
        p += strlen(p) + 1;
        TRACE(" FONT %d,'%s'\n", result->pointSize, result->faceName );
    }
    return p;
}


/***********************************************************************
 *           DIALOG_CreateIndirect16
 *
 * Creates a dialog box window
 *
 * modal = TRUE if we are called from a modal dialog box.
 * (it's more compatible to do it here, as under Windows the owner
 * is never disabled if the dialog fails because of an invalid template)
 */
static HWND DIALOG_CreateIndirect16( HINSTANCE16 hInst, LPCVOID dlgTemplate,
                                     HWND owner, DLGPROC16 dlgProc, LPARAM param,
                                     BOOL modal )
{
    HWND hwnd;
    RECT rect;
    POINT pos;
    SIZE size;
    WND * wndPtr;
    DLG_TEMPLATE template;
    DIALOGINFO * dlgInfo;
    BOOL ownerEnabled = TRUE;
    DWORD exStyle = 0;
    DWORD units = GetDialogBaseUnits();

      /* Parse dialog template */

    dlgTemplate = DIALOG_ParseTemplate16( dlgTemplate, &template );

      /* Initialise dialog extra data */

    if (!(dlgInfo = HeapAlloc( GetProcessHeap(), 0, sizeof(*dlgInfo) ))) return 0;
    dlgInfo->hwndFocus   = 0;
    dlgInfo->hUserFont   = 0;
    dlgInfo->hMenu       = 0;
    dlgInfo->xBaseUnit   = LOWORD(units);
    dlgInfo->yBaseUnit   = HIWORD(units);
    dlgInfo->idResult    = 0;
    dlgInfo->flags       = 0;
    dlgInfo->hDialogHeap = 0;

      /* Load menu */

    if (template.menuName)
    {
        dlgInfo->hMenu = HMENU_32(LoadMenu16( hInst, template.menuName ));
    }

      /* Create custom font if needed */

    if (template.style & DS_SETFONT)
    {
          /* We convert the size to pixels and then make it -ve.  This works
           * for both +ve and -ve template.pointSize */
        HDC dc;
        int pixels;
        dc = GetDC(0);
        pixels = MulDiv(template.pointSize, GetDeviceCaps(dc , LOGPIXELSY), 72);
        dlgInfo->hUserFont = CreateFontA( -pixels, 0, 0, 0, FW_DONTCARE,
                                          FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0,
                                          PROOF_QUALITY, FF_DONTCARE, template.faceName );
        if (dlgInfo->hUserFont)
        {
            SIZE charSize;
            HFONT hOldFont = SelectObject( dc, dlgInfo->hUserFont );
            charSize.cx = GdiGetCharDimensions( dc, NULL, &charSize.cy );
            if (charSize.cx)
            {
                dlgInfo->xBaseUnit = charSize.cx;
                dlgInfo->yBaseUnit = charSize.cy;
            }
            SelectObject( dc, hOldFont );
        }
        ReleaseDC(0, dc);
        TRACE("units = %d,%d\n", dlgInfo->xBaseUnit, dlgInfo->yBaseUnit );
    }

    /* Create dialog main window */

    rect.left = rect.top = 0;
    rect.right = MulDiv(template.cx, dlgInfo->xBaseUnit, 4);
    rect.bottom =  MulDiv(template.cy, dlgInfo->yBaseUnit, 8);
    if (template.style & DS_MODALFRAME) exStyle |= WS_EX_DLGMODALFRAME;
    AdjustWindowRectEx( &rect, template.style, (dlgInfo->hMenu != 0), exStyle );
    pos.x = rect.left;
    pos.y = rect.top;
    size.cx = rect.right - rect.left;
    size.cy = rect.bottom - rect.top;

    if (template.x == CW_USEDEFAULT16)
    {
        pos.x = pos.y = CW_USEDEFAULT16;
    }
    else
    {
        HMONITOR monitor = 0;
        MONITORINFO mon_info;

        mon_info.cbSize = sizeof(mon_info);
        if (template.style & DS_CENTER)
        {
            monitor = MonitorFromWindow( owner ? owner : GetActiveWindow(), MONITOR_DEFAULTTOPRIMARY );
            GetMonitorInfoW( monitor, &mon_info );
            pos.x = (mon_info.rcWork.left + mon_info.rcWork.right - size.cx) / 2;
            pos.y = (mon_info.rcWork.top + mon_info.rcWork.bottom - size.cy) / 2;
        }
        else if (template.style & DS_CENTERMOUSE)
        {
            GetCursorPos( &pos );
            monitor = MonitorFromPoint( pos, MONITOR_DEFAULTTOPRIMARY );
            GetMonitorInfoW( monitor, &mon_info );
        }
        else
        {
            pos.x += MulDiv(template.x, dlgInfo->xBaseUnit, 4);
            pos.y += MulDiv(template.y, dlgInfo->yBaseUnit, 8);
            if (!(template.style & (WS_CHILD|DS_ABSALIGN))) ClientToScreen( owner, &pos );
        }
        if ( !(template.style & WS_CHILD) )
        {
            INT dX, dY;

            /* try to fit it into the desktop */

            if (!monitor)
            {
                SetRect( &rect, pos.x, pos.y, pos.x + size.cx, pos.y + size.cy );
                monitor = MonitorFromRect( &rect, MONITOR_DEFAULTTOPRIMARY );
                GetMonitorInfoW( monitor, &mon_info );
            }
            if ((dX = pos.x + size.cx + GetSystemMetrics(SM_CXDLGFRAME) - mon_info.rcWork.right) > 0)
                pos.x -= dX;
            if ((dY = pos.y + size.cy + GetSystemMetrics(SM_CYDLGFRAME) - mon_info.rcWork.bottom) > 0)
                pos.y -= dY;
            if( pos.x < mon_info.rcWork.left ) pos.x = mon_info.rcWork.left;
            if( pos.y < mon_info.rcWork.top ) pos.y = mon_info.rcWork.top;
        }
    }

    if (modal)
    {
        ownerEnabled = DIALOG_DisableOwner( owner );
        if (ownerEnabled) dlgInfo->flags |= DF_OWNERENABLED;
    }

    hwnd = WIN_Handle32( CreateWindowEx16(exStyle, template.className,
                                          template.caption, template.style & ~WS_VISIBLE,
                                          pos.x, pos.y, size.cx, size.cy,
                                          HWND_16(owner), HMENU_16(dlgInfo->hMenu),
                                          hInst, NULL ));
    if (!hwnd)
    {
        if (dlgInfo->hUserFont) DeleteObject( dlgInfo->hUserFont );
        if (dlgInfo->hMenu) DestroyMenu( dlgInfo->hMenu );
        if (modal && (dlgInfo->flags & DF_OWNERENABLED)) DIALOG_EnableOwner(owner);
        HeapFree( GetProcessHeap(), 0, dlgInfo );
        return 0;
    }
    wndPtr = WIN_GetPtr( hwnd );
    wndPtr->flags |= WIN_ISDIALOG;
    wndPtr->dlgInfo = dlgInfo;
    WIN_ReleasePtr( wndPtr );

    SetWindowLong16( HWND_16(hwnd), DWLP_DLGPROC, (LONG)dlgProc );

    if (dlgInfo->hUserFont)
        SendMessageA( hwnd, WM_SETFONT, (WPARAM)dlgInfo->hUserFont, 0 );

    /* Create controls */

    if (DIALOG_CreateControls16( hwnd, dlgTemplate, &template, hInst ))
    {
        HWND hwndPreInitFocus;

        /* Send initialisation messages and set focus */

        dlgInfo->hwndFocus = GetNextDlgTabItem( hwnd, 0, FALSE );

        hwndPreInitFocus = GetFocus();
        if (SendMessageA( hwnd, WM_INITDIALOG, (WPARAM)dlgInfo->hwndFocus, param ))
        {
            /* check where the focus is again,
	     * some controls status might have changed in WM_INITDIALOG */
            dlgInfo->hwndFocus = GetNextDlgTabItem( hwnd, 0, FALSE);
            if( dlgInfo->hwndFocus )
                SetFocus( dlgInfo->hwndFocus );
        }
        else
        {
            /* If the dlgproc has returned FALSE (indicating handling of keyboard focus)
               but the focus has not changed, set the focus where we expect it. */
            if ((GetFocus() == hwndPreInitFocus) &&
                (GetWindowLongW( hwnd, GWL_STYLE ) & WS_VISIBLE))
            {
                dlgInfo->hwndFocus = GetNextDlgTabItem( hwnd, 0, FALSE);
                if( dlgInfo->hwndFocus )
                    SetFocus( dlgInfo->hwndFocus );
            }
        }

        if (template.style & WS_VISIBLE && !(GetWindowLongW( hwnd, GWL_STYLE ) & WS_VISIBLE))
        {
            ShowWindow( hwnd, SW_SHOWNORMAL );  /* SW_SHOW doesn't always work */
        }
        return hwnd;
    }
    if( IsWindow(hwnd) ) DestroyWindow( hwnd );
    if (modal && ownerEnabled) DIALOG_EnableOwner(owner);
    return 0;
}


/***********************************************************************
 *		DialogBox (USER.87)
 */
INT16 WINAPI DialogBox16( HINSTANCE16 hInst, LPCSTR dlgTemplate,
                          HWND16 owner, DLGPROC16 dlgProc )
{
    return DialogBoxParam16( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/**************************************************************************
 *              EndDialog   (USER.88)
 */
BOOL16 WINAPI EndDialog16( HWND16 hwnd, INT16 retval )
{
    return EndDialog( WIN_Handle32(hwnd), retval );
}


/***********************************************************************
 *		CreateDialog (USER.89)
 */
HWND16 WINAPI CreateDialog16( HINSTANCE16 hInst, LPCSTR dlgTemplate,
                              HWND16 owner, DLGPROC16 dlgProc )
{
    return CreateDialogParam16( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/**************************************************************************
 *              GetDlgItem   (USER.91)
 */
HWND16 WINAPI GetDlgItem16( HWND16 hwndDlg, INT16 id )
{
    return HWND_16( GetDlgItem( WIN_Handle32(hwndDlg), (UINT16) id ));
}


/**************************************************************************
 *              SetDlgItemText   (USER.92)
 */
void WINAPI SetDlgItemText16( HWND16 hwnd, INT16 id, SEGPTR lpString )
{
    SendDlgItemMessage16( hwnd, id, WM_SETTEXT, 0, (LPARAM)lpString );
}


/**************************************************************************
 *              GetDlgItemText   (USER.93)
 */
INT16 WINAPI GetDlgItemText16( HWND16 hwnd, INT16 id, SEGPTR str, UINT16 len )
{
    return (INT16)SendDlgItemMessage16( hwnd, id, WM_GETTEXT, len, (LPARAM)str );
}


/**************************************************************************
 *              SetDlgItemInt   (USER.94)
 */
void WINAPI SetDlgItemInt16( HWND16 hwnd, INT16 id, UINT16 value, BOOL16 fSigned )
{
    SetDlgItemInt( WIN_Handle32(hwnd), (UINT)(UINT16)id,
             (UINT)(fSigned ? (INT16) value : value), fSigned );
}


/**************************************************************************
 *              GetDlgItemInt   (USER.95)
 */
UINT16 WINAPI GetDlgItemInt16( HWND16 hwnd, INT16 id, BOOL16 *translated, BOOL16 fSigned )
{
    UINT result;
    BOOL ok;

    if (translated) *translated = FALSE;
    result = GetDlgItemInt( WIN_Handle32(hwnd), (UINT)(UINT16)id, &ok, fSigned );
    if (!ok) return 0;
    if (fSigned)
    {
        if (((INT)result < -32767) || ((INT)result > 32767)) return 0;
    }
    else
    {
        if (result > 65535) return 0;
    }
    if (translated) *translated = TRUE;
    return (UINT16)result;
}


/**************************************************************************
 *              CheckRadioButton   (USER.96)
 */
BOOL16 WINAPI CheckRadioButton16( HWND16 hwndDlg, UINT16 firstID,
                                  UINT16 lastID, UINT16 checkID )
{
    return CheckRadioButton( WIN_Handle32(hwndDlg), firstID, lastID, checkID );
}


/**************************************************************************
 *              CheckDlgButton   (USER.97)
 */
BOOL16 WINAPI CheckDlgButton16( HWND16 hwnd, INT16 id, UINT16 check )
{
    SendDlgItemMessage16( hwnd, id, BM_SETCHECK16, check, 0 );
    return TRUE;
}


/**************************************************************************
 *              IsDlgButtonChecked   (USER.98)
 */
UINT16 WINAPI IsDlgButtonChecked16( HWND16 hwnd, UINT16 id )
{
    return (UINT16)SendDlgItemMessage16( hwnd, id, BM_GETCHECK16, 0, 0 );
}


/**************************************************************************
 *              DlgDirSelect   (USER.99)
 */
BOOL16 WINAPI DlgDirSelect16( HWND16 hwnd, LPSTR str, INT16 id )
{
    return DlgDirSelectEx16( hwnd, str, 128, id );
}


/**************************************************************************
 *              DlgDirList   (USER.100)
 */
INT16 WINAPI DlgDirList16( HWND16 hDlg, LPSTR spec, INT16 idLBox,
                           INT16 idStatic, UINT16 attrib )
{
    /* according to Win16 docs, DDL_DRIVES should make DDL_EXCLUSIVE
     * be set automatically (this is different in Win32, and
     * DIALOG_DlgDirList sends Win32 messages to the control,
     * so do it here) */
    if (attrib & DDL_DRIVES) attrib |= DDL_EXCLUSIVE;
    return DlgDirListA( WIN_Handle32(hDlg), spec, idLBox, idStatic, attrib );
}


/**************************************************************************
 *              SendDlgItemMessage   (USER.101)
 */
LRESULT WINAPI SendDlgItemMessage16( HWND16 hwnd, INT16 id, UINT16 msg,
                                     WPARAM16 wParam, LPARAM lParam )
{
    HWND16 hwndCtrl = GetDlgItem16( hwnd, id );
    if (hwndCtrl) return SendMessage16( hwndCtrl, msg, wParam, lParam );
    else return 0;
}


/**************************************************************************
 *              MapDialogRect   (USER.103)
 */
void WINAPI MapDialogRect16( HWND16 hwnd, LPRECT16 rect )
{
    RECT rect32;
    MapDialogRect( WIN_Handle32(hwnd), &rect32 );
    rect->left   = rect32.left;
    rect->right  = rect32.right;
    rect->top    = rect32.top;
    rect->bottom = rect32.bottom;
}


/**************************************************************************
 *              DlgDirSelectComboBox   (USER.194)
 */
BOOL16 WINAPI DlgDirSelectComboBox16( HWND16 hwnd, LPSTR str, INT16 id )
{
    return DlgDirSelectComboBoxEx16( hwnd, str, 128, id );
}


/**************************************************************************
 *              DlgDirListComboBox   (USER.195)
 */
INT16 WINAPI DlgDirListComboBox16( HWND16 hDlg, LPSTR spec, INT16 idCBox,
                                   INT16 idStatic, UINT16 attrib )
{
    return DlgDirListComboBoxA( WIN_Handle32(hDlg), spec, idCBox, idStatic, attrib );
}


/***********************************************************************
 *		DialogBoxIndirect (USER.218)
 */
INT16 WINAPI DialogBoxIndirect16( HINSTANCE16 hInst, HANDLE16 dlgTemplate,
                                  HWND16 owner, DLGPROC16 dlgProc )
{
    return DialogBoxIndirectParam16( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *		CreateDialogIndirect (USER.219)
 */
HWND16 WINAPI CreateDialogIndirect16( HINSTANCE16 hInst, LPCVOID dlgTemplate,
                                      HWND16 owner, DLGPROC16 dlgProc )
{
    return CreateDialogIndirectParam16( hInst, dlgTemplate, owner, dlgProc, 0);
}


/**************************************************************************
 *              GetNextDlgGroupItem   (USER.227)
 */
HWND16 WINAPI GetNextDlgGroupItem16( HWND16 hwndDlg, HWND16 hwndCtrl,
                                     BOOL16 fPrevious )
{
    return HWND_16( GetNextDlgGroupItem( WIN_Handle32(hwndDlg), WIN_Handle32(hwndCtrl), fPrevious ));
}


/**************************************************************************
 *              GetNextDlgTabItem   (USER.228)
 */
HWND16 WINAPI GetNextDlgTabItem16( HWND16 hwndDlg, HWND16 hwndCtrl,
                                   BOOL16 fPrevious )
{
    return HWND_16( GetNextDlgTabItem( WIN_Handle32(hwndDlg), WIN_Handle32(hwndCtrl), fPrevious ));
}


/***********************************************************************
 *		DialogBoxParam (USER.239)
 */
INT16 WINAPI DialogBoxParam16( HINSTANCE16 hInst, LPCSTR template,
                               HWND16 owner16, DLGPROC16 dlgProc, LPARAM param )
{
    HWND hwnd = 0;
    HRSRC16 hRsrc;
    HGLOBAL16 hmem;
    LPCVOID data;
    int ret = -1;

    if (!(hRsrc = FindResource16( hInst, template, (LPSTR)RT_DIALOG ))) return 0;
    if (!(hmem = LoadResource16( hInst, hRsrc ))) return 0;
    if ((data = LockResource16( hmem )))
    {
        HWND owner = WIN_Handle32(owner16);
        hwnd = DIALOG_CreateIndirect16( hInst, data, owner, dlgProc, param, TRUE );
        if (hwnd) ret = DIALOG_DoDialogBox( hwnd, owner );
        GlobalUnlock16( hmem );
    }
    FreeResource16( hmem );
    return ret;
}


/***********************************************************************
 *		DialogBoxIndirectParam (USER.240)
 */
INT16 WINAPI DialogBoxIndirectParam16( HINSTANCE16 hInst, HANDLE16 dlgTemplate,
                                       HWND16 owner16, DLGPROC16 dlgProc, LPARAM param )
{
    HWND hwnd, owner = WIN_Handle32( owner16 );
    LPCVOID ptr;

    if (!(ptr = GlobalLock16( dlgTemplate ))) return -1;
    hwnd = DIALOG_CreateIndirect16( hInst, ptr, owner, dlgProc, param, TRUE );
    GlobalUnlock16( dlgTemplate );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *		CreateDialogParam (USER.241)
 */
HWND16 WINAPI CreateDialogParam16( HINSTANCE16 hInst, LPCSTR dlgTemplate,
                                   HWND16 owner, DLGPROC16 dlgProc, LPARAM param )
{
    HWND16 hwnd = 0;
    HRSRC16 hRsrc;
    HGLOBAL16 hmem;
    LPCVOID data;

    TRACE("%04x,%s,%04x,%p,%ld\n",
          hInst, debugstr_a(dlgTemplate), owner, dlgProc, param );

    if (!(hRsrc = FindResource16( hInst, dlgTemplate, (LPSTR)RT_DIALOG ))) return 0;
    if (!(hmem = LoadResource16( hInst, hRsrc ))) return 0;
    if (!(data = LockResource16( hmem ))) hwnd = 0;
    else hwnd = CreateDialogIndirectParam16( hInst, data, owner, dlgProc, param );
    FreeResource16( hmem );
    return hwnd;
}


/***********************************************************************
 *		CreateDialogIndirectParam (USER.242)
 */
HWND16 WINAPI CreateDialogIndirectParam16( HINSTANCE16 hInst, LPCVOID dlgTemplate,
                                           HWND16 owner, DLGPROC16 dlgProc, LPARAM param )
{
    if (!dlgTemplate) return 0;
    return HWND_16( DIALOG_CreateIndirect16( hInst, dlgTemplate, WIN_Handle32(owner),
                                             dlgProc, param, FALSE ));
}


/**************************************************************************
 *              DlgDirSelectEx   (USER.422)
 */
BOOL16 WINAPI DlgDirSelectEx16( HWND16 hwnd, LPSTR str, INT16 len, INT16 id )
{
    return DlgDirSelectExA( WIN_Handle32(hwnd), str, len, id );
}


/**************************************************************************
 *              DlgDirSelectComboBoxEx   (USER.423)
 */
BOOL16 WINAPI DlgDirSelectComboBoxEx16( HWND16 hwnd, LPSTR str, INT16 len,
                                        INT16 id )
{
    return DlgDirSelectComboBoxExA( WIN_Handle32(hwnd), str, len, id );
}
