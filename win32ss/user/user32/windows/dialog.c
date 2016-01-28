/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/windows/dialog.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *                  Steven Edwards (Steven_Ed4153@yahoo.com)
 * UPDATE HISTORY:
 *      07-26-2003  Code ported from wine
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(user32);

/* MACROS/DEFINITIONS ********************************************************/

#define DF_END  0x0001
#define DF_OWNERENABLED 0x0002
#define DF_DIALOGACTIVE 0x4000 // ReactOS
#define DWLP_ROS_DIALOGINFO (DWLP_USER+sizeof(ULONG_PTR))
#define GETDLGINFO(hwnd) DIALOG_get_info(hwnd, FALSE)
#define SETDLGINFO(hwnd, info) SetWindowLongPtrW((hwnd), DWLP_ROS_DIALOGINFO, (LONG_PTR)(info))
#define GET_WORD(ptr)  (*(WORD *)(ptr))
#define GET_DWORD(ptr) (*(DWORD *)(ptr))
#define DLG_ISANSI 2

/* INTERNAL STRUCTS **********************************************************/

/* Dialog info structure */
typedef struct
{
    HWND      hwndFocus;   /* Current control with focus */
    HFONT     hUserFont;   /* Dialog font */
    HMENU     hMenu;       /* Dialog menu */
    UINT      xBaseUnit;   /* Dialog units (depends on the font) */
    UINT      yBaseUnit;
    INT       idResult;    /* EndDialog() result / default pushbutton ID */
    UINT      flags;       /* EndDialog() called for this dialog */
} DIALOGINFO;

/* Dialog control information */
typedef struct
{
    DWORD      style;
    DWORD      exStyle;
    DWORD      helpId;
    short      x;
    short      y;
    short      cx;
    short      cy;
    UINT       id;
    LPCWSTR    className;
    LPCWSTR    windowName;
    BOOL       windowNameFree; // ReactOS
    LPCVOID    data;
} DLG_CONTROL_INFO;

/* Dialog template */
typedef struct
{
    DWORD      style;
    DWORD      exStyle;
    DWORD      helpId;
    WORD       nbItems;
    short      x;
    short      y;
    short      cx;
    short      cy;
    LPCWSTR    menuName;
    LPCWSTR    className;
    LPCWSTR    caption;
    WORD       pointSize;
    WORD       weight;
    BOOL       italic;
    LPCWSTR    faceName;
    BOOL       dialogEx;
} DLG_TEMPLATE;

/* CheckRadioButton structure */
typedef struct
{
  UINT firstID;
  UINT lastID;
  UINT checkID;
} RADIOGROUP;


/*********************************************************************
 * dialog class descriptor
 */
const struct builtin_class_descr DIALOG_builtin_class =
{
    WC_DIALOG,       /* name */
    CS_SAVEBITS | CS_DBLCLKS, /* style  */
    DefDlgProcA,              /* procA */
    DefDlgProcW,              /* procW */
    DLGWINDOWEXTRA,           /* extra */
    (LPCWSTR) IDC_ARROW,      /* cursor */
    0                         /* brush */
};


/* INTERNAL FUNCTIONS ********************************************************/

/***********************************************************************
*               DIALOG_get_info
*
* Get the DIALOGINFO structure of a window, allocating it if needed
* and 'create' is TRUE.
*
* ReactOS
*/
DIALOGINFO *DIALOG_get_info( HWND hWnd, BOOL create )
{
    PWND pWindow;
    DIALOGINFO* dlgInfo;

    pWindow = ValidateHwnd( hWnd );
    if (!pWindow)
    {
       return NULL;
    }

    dlgInfo = (DIALOGINFO *)GetWindowLongPtrW( hWnd, DWLP_ROS_DIALOGINFO );

    if (!dlgInfo && create)
    {
        if (pWindow && pWindow->cbwndExtra >= DLGWINDOWEXTRA)
        {
            if (!(dlgInfo = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*dlgInfo) )))
                return NULL;

            dlgInfo->idResult = IDOK;
            SETDLGINFO( hWnd, dlgInfo );
       }
       else
       {
           return NULL;
       }
    }

    if (dlgInfo)
    {
        if (!(pWindow->state & WNDS_DIALOGWINDOW))
        {
           NtUserxSetDialogPointer( hWnd, dlgInfo );
        }
    }
    return dlgInfo;
}

/***********************************************************************
 *           DIALOG_EnableOwner
 *
 * Helper function for modal dialogs to enable again the
 * owner of the dialog box.
 */
void DIALOG_EnableOwner( HWND hOwner )
{
    /* Owner must be a top-level window */
    if (hOwner)
        hOwner = GetAncestor( hOwner, GA_ROOT );
    if (!hOwner) return;
    EnableWindow( hOwner, TRUE );
}


/***********************************************************************
 *           DIALOG_DisableOwner
 *
 * Helper function for modal dialogs to disable the
 * owner of the dialog box. Returns TRUE if owner was enabled.
 */
BOOL DIALOG_DisableOwner( HWND hOwner )
{
    /* Owner must be a top-level window */
    if (hOwner)
        hOwner = GetAncestor( hOwner, GA_ROOT );
    if (!hOwner) return FALSE;
    if (IsWindowEnabled( hOwner ))
    {
        EnableWindow( hOwner, FALSE );
        return TRUE;
    }
    else
        return FALSE;
}

/***********************************************************************
 *           DIALOG_GetControl32
 *
 * Return the class and text of the control pointed to by ptr,
 * fill the header structure and return a pointer to the next control.
 */
static const WORD *DIALOG_GetControl32( const WORD *p, DLG_CONTROL_INFO *info,
                                        BOOL dialogEx )
{
    if (dialogEx)
    {
        info->helpId  = GET_DWORD(p); p += 2;
        info->exStyle = GET_DWORD(p); p += 2;
        info->style   = GET_DWORD(p); p += 2;
    }
    else
    {
        info->helpId  = 0;
        info->style   = GET_DWORD(p); p += 2;
        info->exStyle = GET_DWORD(p); p += 2;
    }
    info->x       = GET_WORD(p); p++;
    info->y       = GET_WORD(p); p++;
    info->cx      = GET_WORD(p); p++;
    info->cy      = GET_WORD(p); p++;

    if (dialogEx)
    {
        /* id is a DWORD for DIALOGEX */
        info->id = GET_DWORD(p);
        p += 2;
    }
    else
    {
        info->id = GET_WORD(p);
        p++;
    }

    if (GET_WORD(p) == 0xffff)
    {
        static const WCHAR class_names[6][10] =
        {
            { 'B','u','t','t','o','n', },             /* 0x80 */
            { 'E','d','i','t', },                     /* 0x81 */
            { 'S','t','a','t','i','c', },             /* 0x82 */
            { 'L','i','s','t','B','o','x', },         /* 0x83 */
            { 'S','c','r','o','l','l','B','a','r', }, /* 0x84 */
            { 'C','o','m','b','o','B','o','x', }      /* 0x85 */
        };
        WORD id = GET_WORD(p+1);
        /* Windows treats dialog control class ids 0-5 same way as 0x80-0x85 */
        if ((id >= 0x80) && (id <= 0x85)) id -= 0x80;
        if (id <= 5)
        {
            info->className = class_names[id];
        }
        else
        {
            info->className = NULL;
            /* FIXME: load other classes here? */
            ERR("Unknown built-in class id %04x\n", id );
        }
        p += 2;
    }
    else
    {
        info->className = (LPCWSTR)p;
        p += strlenW( info->className ) + 1;
    }

    if (GET_WORD(p) == 0xffff)  /* Is it an integer id? */
    {
//// ReactOS Rev 6478
        info->windowName = HeapAlloc( GetProcessHeap(), 0, sizeof(L"#65535") );
        if (info->windowName != NULL)
        {
            wsprintf((LPWSTR)info->windowName, L"#%u", GET_WORD(p + 1));
            info->windowNameFree = TRUE;
        }
        else
        {
            info->windowNameFree = FALSE;
        }
        p += 2;
    }
    else
    {
        info->windowName = (LPCWSTR)p;
        info->windowNameFree = FALSE;
        p += strlenW( info->windowName ) + 1;
    }

    TRACE("    %s %s %ld, %d, %d, %d, %d, %08x, %08x, %08x\n",
          debugstr_w( info->className ), debugstr_w( info->windowName ),
          info->id, info->x, info->y, info->cx, info->cy,
          info->style, info->exStyle, info->helpId );

    if (GET_WORD(p))
    {
        info->data = p + 1;
        p += GET_WORD(p) / sizeof(WORD);
    }
    else info->data = NULL;
    p++;

    /* Next control is on dword boundary */
    return (const WORD *)(((UINT_PTR)p + 3) & ~3);
}


/***********************************************************************
 *           DIALOG_CreateControls32
 *
 * Create the control windows for a dialog.
 */
static BOOL DIALOG_CreateControls32( HWND hwnd, LPCSTR template, const DLG_TEMPLATE *dlgTemplate,
                                     HINSTANCE hInst, BOOL unicode )
{
    DIALOGINFO * dlgInfo;
    DLG_CONTROL_INFO info;
    HWND hwndCtrl, hwndDefButton = 0;
    INT items = dlgTemplate->nbItems;

    if (!(dlgInfo = GETDLGINFO(hwnd))) return FALSE;

    TRACE(" BEGIN\n" );
    while (items--)
    {
        template = (LPCSTR)DIALOG_GetControl32( (const WORD *)template, &info,
                                                dlgTemplate->dialogEx );
        info.style &= ~WS_POPUP;
        info.style |= WS_CHILD;

        if (info.style & WS_BORDER)
        {
            info.style &= ~WS_BORDER;
            info.exStyle |= WS_EX_CLIENTEDGE;
        }

        if (unicode)
        {
            hwndCtrl = CreateWindowExW( info.exStyle | WS_EX_NOPARENTNOTIFY,
                                        info.className, info.windowName,
                                        info.style | WS_CHILD,
                                        MulDiv(info.x, dlgInfo->xBaseUnit, 4),
                                        MulDiv(info.y, dlgInfo->yBaseUnit, 8),
                                        MulDiv(info.cx, dlgInfo->xBaseUnit, 4),
                                        MulDiv(info.cy, dlgInfo->yBaseUnit, 8),
                                        hwnd, (HMENU)(ULONG_PTR)info.id,
                                        hInst, (LPVOID)info.data );
        }
        else
        {
            LPSTR class = (LPSTR)info.className;
            LPSTR caption = (LPSTR)info.windowName;

            if (!IS_INTRESOURCE(class))
            {
                DWORD len = WideCharToMultiByte( CP_ACP, 0, info.className, -1, NULL, 0, NULL, NULL );
                class = HeapAlloc( GetProcessHeap(), 0, len );
                if (class != NULL)
                    WideCharToMultiByte( CP_ACP, 0, info.className, -1, class, len, NULL, NULL );
            }
            if (!IS_INTRESOURCE(caption))
            {
                DWORD len = WideCharToMultiByte( CP_ACP, 0, info.windowName, -1, NULL, 0, NULL, NULL );
                caption = HeapAlloc( GetProcessHeap(), 0, len );
                if (caption != NULL)
                    WideCharToMultiByte( CP_ACP, 0, info.windowName, -1, caption, len, NULL, NULL );
            }

            if (class != NULL && caption != NULL)
            {
                hwndCtrl = CreateWindowExA( info.exStyle | WS_EX_NOPARENTNOTIFY,
                                            class, caption, info.style | WS_CHILD,
                                            MulDiv(info.x, dlgInfo->xBaseUnit, 4),
                                            MulDiv(info.y, dlgInfo->yBaseUnit, 8),
                                            MulDiv(info.cx, dlgInfo->xBaseUnit, 4),
                                            MulDiv(info.cy, dlgInfo->yBaseUnit, 8),
                                            hwnd, (HMENU)(ULONG_PTR)info.id,
                                            hInst, (LPVOID)info.data );
            }
            else
                hwndCtrl = NULL;
            if (!IS_INTRESOURCE(class)) HeapFree( GetProcessHeap(), 0, class );
            if (!IS_INTRESOURCE(caption)) HeapFree( GetProcessHeap(), 0, caption );
        }

        if (info.windowNameFree)
        {
            HeapFree( GetProcessHeap(), 0, (LPVOID)info.windowName );
        }

        if (!hwndCtrl)
        {
            WARN("control %s %s creation failed\n", debugstr_w(info.className),
                  debugstr_w(info.windowName));
            if (dlgTemplate->style & DS_NOFAILCREATE) continue;
            return FALSE;
        }

        /* Send initialisation messages to the control */
        if (dlgInfo->hUserFont) SendMessageW( hwndCtrl, WM_SETFONT,
                                             (WPARAM)dlgInfo->hUserFont, 0 );
        if (SendMessageW(hwndCtrl, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON)
        {
            /* If there's already a default push-button, set it back */
            /* to normal and use this one instead. */
            if (hwndDefButton)
                SendMessageW( hwndDefButton, BM_SETSTYLE, BS_PUSHBUTTON, FALSE );
            hwndDefButton = hwndCtrl;
            dlgInfo->idResult = GetWindowLongPtrA( hwndCtrl, GWLP_ID );
        }
    }
    TRACE(" END\n" );
    return TRUE;
}


 /***********************************************************************
  *           DIALOG_IsAccelerator
  */
static BOOL DIALOG_IsAccelerator( HWND hwnd, HWND hwndDlg, WPARAM wParam )
{
    HWND hwndControl = hwnd;
    HWND hwndNext;
    INT dlgCode;
    WCHAR buffer[128];

    do
    {
        DWORD style = GetWindowLongPtrW( hwndControl, GWL_STYLE );
        if ((style & (WS_VISIBLE | WS_DISABLED)) == WS_VISIBLE)
        {
            dlgCode = SendMessageW( hwndControl, WM_GETDLGCODE, 0, 0 );
            if ( (dlgCode & (DLGC_BUTTON | DLGC_STATIC)) &&
                 GetWindowTextW( hwndControl, buffer, sizeof(buffer)/sizeof(WCHAR) ))
            {
                /* find the accelerator key */
                LPWSTR p = buffer - 2;

                do
                {
                    p = strchrW( p + 2, '&' );
                }
                while (p != NULL && p[1] == '&');

                /* and check if it's the one we're looking for */
                if (p != NULL && toupperW( p[1] ) == toupperW( wParam ) )
                {
                    if ((dlgCode & DLGC_STATIC) || (style & 0x0f) == BS_GROUPBOX )
                    {
                        /* set focus to the control */
                        SendMessageW( hwndDlg, WM_NEXTDLGCTL, (WPARAM)hwndControl, 1);
                        /* and bump it on to next */
                        SendMessageW( hwndDlg, WM_NEXTDLGCTL, 0, 0);
                    }
                    else if (dlgCode & DLGC_BUTTON)
                    {
                        /* send BM_CLICK message to the control */
                        SendMessageW( hwndControl, BM_CLICK, 0, 0 );
                    }
                    return TRUE;
                }
            }
            hwndNext = GetWindow( hwndControl, GW_CHILD );
        }
        else hwndNext = 0;

        if (!hwndNext) hwndNext = GetWindow( hwndControl, GW_HWNDNEXT );

        while (!hwndNext && hwndControl)
        {
            hwndControl = GetParent( hwndControl );
            if (hwndControl == hwndDlg)
            {
                if(hwnd==hwndDlg)   /* prevent endless loop */
                {
                    hwndNext=hwnd;
                    break;
                }
                hwndNext = GetWindow( hwndDlg, GW_CHILD );
            }
            else
                hwndNext = GetWindow( hwndControl, GW_HWNDNEXT );
        }
        hwndControl = hwndNext;
    }
    while (hwndControl && (hwndControl != hwnd));

    return FALSE;
}

 /***********************************************************************
  *           DIALOG_FindMsgDestination
  *
  * The messages that IsDialogMessage sends may not go to the dialog
  * calling IsDialogMessage if that dialog is a child, and it has the
  * DS_CONTROL style set.
  * We propagate up until we hit one that does not have DS_CONTROL, or
  * whose parent is not a dialog.
  *
  * This is undocumented behaviour.
  */
static HWND DIALOG_FindMsgDestination( HWND hwndDlg )
{
    while (GetWindowLongA(hwndDlg, GWL_STYLE) & DS_CONTROL)
    {
        PWND pWnd;
        HWND hParent = GetParent(hwndDlg);
        if (!hParent) break;
// ReactOS
        if (!IsWindow(hParent)) break;

        pWnd = ValidateHwnd(hParent);
        // FIXME: Use pWnd->fnid == FNID_DESKTOP
        if (!pWnd || !TestWindowProcess(pWnd) || hParent == GetDesktopWindow()) break;

        if (!(pWnd->state & WNDS_DIALOGWINDOW))
        {
            break;
        }

        hwndDlg = hParent;
    }

    return hwndDlg;
}

 /***********************************************************************
 *           DIALOG_DoDialogBox
 */
INT DIALOG_DoDialogBox( HWND hwnd, HWND owner )
{
    DIALOGINFO * dlgInfo;
    MSG msg;
    INT retval;
    HWND ownerMsg = GetAncestor( owner, GA_ROOT );
    BOOL bFirstEmpty;
    PWND pWnd;

    pWnd = ValidateHwnd(hwnd);
    if (!pWnd) return -1;

    if (!(dlgInfo = GETDLGINFO(hwnd))) return -1;

    bFirstEmpty = TRUE;
    if (!(dlgInfo->flags & DF_END)) /* was EndDialog called in WM_INITDIALOG ? */
    {
        for (;;)
        {
            if (!PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ))
            {
                if (bFirstEmpty)
                {
                    /* ShowWindow the first time the queue goes empty */
                    ShowWindow( hwnd, SW_SHOWNORMAL );
                    bFirstEmpty = FALSE;
                }
                if (!(GetWindowLongPtrW( hwnd, GWL_STYLE ) & DS_NOIDLEMSG))
               {
                    /* No message present -> send ENTERIDLE and wait */
                    if (ownerMsg) SendMessageW( ownerMsg, WM_ENTERIDLE, MSGF_DIALOGBOX, (LPARAM)hwnd );
                }
                GetMessageW( &msg, 0, 0, 0 );
            }

            if (msg.message == WM_QUIT)
            {
                PostQuitMessage( msg.wParam );
                if (!IsWindow( hwnd )) return 0;
                break;
            }

            /*
             * If the user is pressing Ctrl+C, send a WM_COPY message.
             * Guido Pola, CORE-4829, Is there another way to check if the Dialog is a MessageBox?
             */
            if (msg.message == WM_KEYDOWN &&
                pWnd->state & WNDS_MSGBOX && // Yes!
                GetForegroundWindow() == hwnd)
            {
                if (msg.wParam == L'C' && GetKeyState(VK_CONTROL) < 0)
                    SendMessageW(hwnd, WM_COPY, 0, 0);
            }

            if (!IsWindow( hwnd )) return 0;
            if (!(dlgInfo->flags & DF_END) && !IsDialogMessageW( hwnd, &msg))
            {
                TranslateMessage( &msg );
                DispatchMessageW( &msg );
            }
            if (!IsWindow( hwnd )) return 0;
            if (dlgInfo->flags & DF_END) break;

            if (bFirstEmpty && msg.message == WM_TIMER)
            {
                ShowWindow( hwnd, SW_SHOWNORMAL );
                bFirstEmpty = FALSE;
            }
        }
    }
    if (dlgInfo->flags & DF_OWNERENABLED) DIALOG_EnableOwner( owner );
    retval = dlgInfo->idResult;
    DestroyWindow( hwnd );
    return retval;
}

 /***********************************************************************
 *           DIALOG_ParseTemplate32
 *
 * Fill a DLG_TEMPLATE structure from the dialog template, and return
 * a pointer to the first control.
 */
static LPCSTR DIALOG_ParseTemplate32( LPCSTR template, DLG_TEMPLATE * result )
{
    const WORD *p = (const WORD *)template;
    WORD signature;
    WORD dlgver;

    dlgver = GET_WORD(p); p++;
    signature = GET_WORD(p); p++;

    if (dlgver == 1 && signature == 0xffff)  /* DIALOGEX resource */
    {
        result->dialogEx = TRUE;
        result->helpId   = GET_DWORD(p); p += 2;
        result->exStyle  = GET_DWORD(p); p += 2;
        result->style    = GET_DWORD(p); p += 2;
    }
    else
    {
        result->style = GET_DWORD(p - 2);
        result->dialogEx = FALSE;
        result->helpId   = 0;
        result->exStyle  = GET_DWORD(p); p += 2;
    }
    result->nbItems = GET_WORD(p); p++;
    result->x       = GET_WORD(p); p++;
    result->y       = GET_WORD(p); p++;
    result->cx      = GET_WORD(p); p++;
    result->cy      = GET_WORD(p); p++;
    TRACE("DIALOG%s %d, %d, %d, %d, %d\n",
           result->dialogEx ? "EX" : "", result->x, result->y,
           result->cx, result->cy, result->helpId );
    TRACE(" STYLE 0x%08x\n", result->style );
    TRACE(" EXSTYLE 0x%08x\n", result->exStyle );

    /* Get the menu name */

    switch(GET_WORD(p))
    {
        case 0x0000:
            result->menuName = NULL;
            p++;
            break;
        case 0xffff:
            result->menuName = (LPCWSTR)(UINT_PTR)GET_WORD( p + 1 );
            p += 2;
            TRACE(" MENU %04x\n", LOWORD(result->menuName) );
            break;
        default:
            result->menuName = (LPCWSTR)p;
            TRACE(" MENU %s\n", debugstr_w(result->menuName) );
            p += strlenW( result->menuName ) + 1;
            break;
    }

    /* Get the class name */

    switch(GET_WORD(p))
    {
        case 0x0000:
            result->className = WC_DIALOG;
            p++;
            break;
        case 0xffff:
            result->className = (LPCWSTR)(UINT_PTR)GET_WORD( p + 1 );
            p += 2;
            TRACE(" CLASS %04x\n", LOWORD(result->className) );
            break;
        default:
            result->className = (LPCWSTR)p;
            TRACE(" CLASS %s\n", debugstr_w( result->className ));
            p += strlenW( result->className ) + 1;
            break;
    }

    /* Get the window caption */

    result->caption = (LPCWSTR)p;
    p += strlenW( result->caption ) + 1;
    TRACE(" CAPTION %s\n", debugstr_w( result->caption ) );

    /* Get the font name */

    result->pointSize = 0;
    result->faceName = NULL;
    result->weight = FW_DONTCARE;
    result->italic = FALSE;

    if (result->style & DS_SETFONT)
    {
        result->pointSize = GET_WORD(p);
        p++;

        /* If pointSize is 0x7fff, it means that we need to use the font
         * in NONCLIENTMETRICSW.lfMessageFont, and NOT read the weight,
         * italic, and facename from the dialog template.
         */
        if (result->pointSize == 0x7fff)
        {
            /* We could call SystemParametersInfo here, but then we'd have
             * to convert from pixel size to point size (which can be
             * imprecise).
             */
            TRACE(" FONT: Using message box font\n");
        }
        else
        {
            if (result->dialogEx)
            {
                result->weight = GET_WORD(p); p++;
                result->italic = LOBYTE(GET_WORD(p)); p++;
            }
            result->faceName = (LPCWSTR)p;
            p += strlenW( result->faceName ) + 1;

            TRACE(" FONT %d, %s, %d, %s\n",
                  result->pointSize, debugstr_w( result->faceName ),
                  result->weight, result->italic ? "TRUE" : "FALSE" );
        }
    }

    /* First control is on dword boundary */
    return (LPCSTR)((((UINT_PTR)p) + 3) & ~3);
}

/***********************************************************************
 *           DEFDLG_SetFocus
 *
 * Set the focus to a control of the dialog, selecting the text if
 * the control is an edit dialog that has DLGC_HASSETSEL.
 */
static void DEFDLG_SetFocus( HWND hwndCtrl )
{
    if (SendMessageW( hwndCtrl, WM_GETDLGCODE, 0, 0 ) & DLGC_HASSETSEL)
        SendMessageW( hwndCtrl, EM_SETSEL, 0, -1 );
    SetFocus( hwndCtrl );
}


/***********************************************************************
 *           DEFDLG_SaveFocus
 */
static void DEFDLG_SaveFocus( HWND hwnd )
{
    DIALOGINFO *infoPtr;
    HWND hwndFocus = GetFocus();

    if (!hwndFocus || !IsChild( hwnd, hwndFocus )) return;
    if (!(infoPtr = GETDLGINFO(hwnd))) return;
    infoPtr->hwndFocus = hwndFocus;
    /* Remove default button */
}


/***********************************************************************
 *           DEFDLG_RestoreFocus
 */
static void DEFDLG_RestoreFocus( HWND hwnd, BOOL justActivate )
{
    DIALOGINFO *infoPtr;

    if (IsIconic( hwnd )) return;
    if (!(infoPtr = GETDLGINFO(hwnd))) return;
    /* Don't set the focus back to controls if EndDialog is already called.*/
    if (infoPtr->flags & DF_END) return;
    if (!IsWindow(infoPtr->hwndFocus) || infoPtr->hwndFocus == hwnd) {
        if (justActivate) return;
        /* If no saved focus control exists, set focus to the first visible,
           non-disabled, WS_TABSTOP control in the dialog */
        infoPtr->hwndFocus = GetNextDlgTabItem( hwnd, 0, FALSE );
        /* If there are no WS_TABSTOP controls, set focus to the first visible,
           non-disabled control in the dialog */
        if (!infoPtr->hwndFocus) infoPtr->hwndFocus = GetNextDlgGroupItem( hwnd, 0, FALSE );
        if (!IsWindow( infoPtr->hwndFocus )) return;
    }
    if (justActivate)
        SetFocus( infoPtr->hwndFocus );
    else
        DEFDLG_SetFocus( infoPtr->hwndFocus );

    infoPtr->hwndFocus = NULL;
}

/***********************************************************************
 *           DIALOG_CreateIndirect
 *       Creates a dialog box window
 *
 *       modal = TRUE if we are called from a modal dialog box.
 *       (it's more compatible to do it here, as under Windows the owner
 *       is never disabled if the dialog fails because of an invalid template)
 */
static HWND DIALOG_CreateIndirect( HINSTANCE hInst, LPCVOID dlgTemplate,
                                   HWND owner, DLGPROC dlgProc, LPARAM param,
                                   BOOL unicode, BOOL modal )
{
    HWND hwnd;
    RECT rect;
    POINT pos;
    SIZE size;
    DLG_TEMPLATE template;
    DIALOGINFO * dlgInfo = NULL;
    DWORD units = GetDialogBaseUnits();
    BOOL ownerEnabled = TRUE;
    HMENU hMenu = 0;
    HFONT hUserFont = 0;
    UINT flags = 0;
    UINT xBaseUnit = LOWORD(units);
    UINT yBaseUnit = HIWORD(units);

      /* Parse dialog template */

    if (!dlgTemplate) return 0;
    dlgTemplate = DIALOG_ParseTemplate32( dlgTemplate, &template );

      /* Load menu */

    if (template.menuName) hMenu = LoadMenuW( hInst, template.menuName );

      /* Create custom font if needed */

    if (template.style & DS_SETFONT)
    {
        HDC dc = GetDC(0);

        if (template.pointSize == 0x7fff)
        {
            /* We get the message font from the non-client metrics */
            NONCLIENTMETRICSW ncMetrics;

            ncMetrics.cbSize = sizeof(NONCLIENTMETRICSW);
            if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS,
                                      sizeof(NONCLIENTMETRICSW), &ncMetrics, 0))
            {
                hUserFont = CreateFontIndirectW( &ncMetrics.lfMessageFont );
            }
        }
        else
        {
            /* We convert the size to pixels and then make it -ve.  This works
             * for both +ve and -ve template.pointSize */
            int pixels = MulDiv(template.pointSize, GetDeviceCaps(dc , LOGPIXELSY), 72);
            hUserFont = CreateFontW( -pixels, 0, 0, 0, template.weight,
                                              template.italic, FALSE, FALSE, DEFAULT_CHARSET, 0, 0,
                                              PROOF_QUALITY, FF_DONTCARE,
                                              template.faceName );
        }

        if (hUserFont)
        {
            SIZE charSize;
            HFONT hOldFont = SelectObject( dc, hUserFont );
            charSize.cx = GdiGetCharDimensions( dc, NULL, &charSize.cy );
            if (charSize.cx)
            {
                xBaseUnit = charSize.cx;
                yBaseUnit = charSize.cy;
            }
            SelectObject( dc, hOldFont );
        }
        ReleaseDC(0, dc);
        TRACE("units = %d,%d\n", xBaseUnit, yBaseUnit );
    }

    /* Create dialog main window */

    rect.left = rect.top = 0;
    rect.right = MulDiv(template.cx, xBaseUnit, 4);
    rect.bottom =  MulDiv(template.cy, yBaseUnit, 8);

    if (template.style & DS_CONTROL)
        template.style &= ~(WS_CAPTION|WS_SYSMENU);
    template.style |= DS_3DLOOK;
    if (template.style & DS_MODALFRAME)
        template.exStyle |= WS_EX_DLGMODALFRAME;
    if ((template.style & DS_CONTROL) || !(template.style & WS_CHILD))
        template.exStyle |= WS_EX_CONTROLPARENT;
    AdjustWindowRectEx( &rect, template.style, (hMenu != 0), template.exStyle );
    pos.x = rect.left;
    pos.y = rect.top;
    size.cx = rect.right - rect.left;
    size.cy = rect.bottom - rect.top;

    if (template.x == CW_USEDEFAULT16)
    {
        pos.x = pos.y = CW_USEDEFAULT;
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
            pos.x += MulDiv(template.x, xBaseUnit, 4);
            pos.y += MulDiv(template.y, yBaseUnit, 8);
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
        if (ownerEnabled) flags |= DF_OWNERENABLED;
    }

    if (unicode)
    {
        hwnd = CreateWindowExW(template.exStyle, template.className, template.caption,
                               template.style & ~WS_VISIBLE, pos.x, pos.y, size.cx, size.cy,
                               owner, hMenu, hInst, NULL );
    }
    else
    {
        LPCSTR class = (LPCSTR)template.className;
        LPCSTR caption = (LPCSTR)template.caption;
        LPSTR class_tmp = NULL;
        LPSTR caption_tmp = NULL;

        if (!IS_INTRESOURCE(class))
        {
            DWORD len = WideCharToMultiByte( CP_ACP, 0, template.className, -1, NULL, 0, NULL, NULL );
            class_tmp = HeapAlloc( GetProcessHeap(), 0, len );
            WideCharToMultiByte( CP_ACP, 0, template.className, -1, class_tmp, len, NULL, NULL );
            class = class_tmp;
        }
        if (!IS_INTRESOURCE(caption))
        {
            DWORD len = WideCharToMultiByte( CP_ACP, 0, template.caption, -1, NULL, 0, NULL, NULL );
            caption_tmp = HeapAlloc( GetProcessHeap(), 0, len );
            WideCharToMultiByte( CP_ACP, 0, template.caption, -1, caption_tmp, len, NULL, NULL );
            caption = caption_tmp;
        }
        hwnd = CreateWindowExA(template.exStyle, class, caption,
                               template.style & ~WS_VISIBLE, pos.x, pos.y, size.cx, size.cy,
                               owner, hMenu, hInst, NULL );
        HeapFree( GetProcessHeap(), 0, class_tmp );
        HeapFree( GetProcessHeap(), 0, caption_tmp );
    }

    if (!hwnd)
    {
        if (hUserFont) DeleteObject( hUserFont );
        if (hMenu) DestroyMenu( hMenu );
        if (modal && (flags & DF_OWNERENABLED)) DIALOG_EnableOwner(owner);
        return 0;
    }

    /* moved this from the top of the method to here as DIALOGINFO structure
    will be valid only after WM_CREATE message has been handled in DefDlgProc
    All the members of the structure get filled here using temp variables */
    dlgInfo = DIALOG_get_info( hwnd, TRUE );
    // ReactOS
    if (dlgInfo == NULL)
    {
        if (hUserFont) DeleteObject( hUserFont );
        if (hMenu) DestroyMenu( hMenu );
        if (modal && (flags & DF_OWNERENABLED)) DIALOG_EnableOwner(owner);
        return 0;
    }
    //
    dlgInfo->hwndFocus   = 0;
    dlgInfo->hUserFont   = hUserFont;
    dlgInfo->hMenu       = hMenu;
    dlgInfo->xBaseUnit   = xBaseUnit;
    dlgInfo->yBaseUnit   = yBaseUnit;
    dlgInfo->flags       = flags;

    if (template.helpId) SetWindowContextHelpId( hwnd, template.helpId );

    if (unicode) SetWindowLongPtrW( hwnd, DWLP_DLGPROC, (ULONG_PTR)dlgProc );
    else SetWindowLongPtrA( hwnd, DWLP_DLGPROC, (ULONG_PTR)dlgProc );

    if (dlgProc && dlgInfo->hUserFont)
        SendMessageW( hwnd, WM_SETFONT, (WPARAM)dlgInfo->hUserFont, 0 );

    /* Create controls */

    if (DIALOG_CreateControls32( hwnd, dlgTemplate, &template, hInst, unicode ))
    {
        /* Send initialisation messages and set focus */

        if (dlgProc)
        {
            HWND focus = GetNextDlgTabItem( hwnd, 0, FALSE );
            if (!focus) focus = GetNextDlgGroupItem( hwnd, 0, FALSE );
            if (SendMessageW( hwnd, WM_INITDIALOG, (WPARAM)focus, param ) && IsWindow( hwnd ) &&
                ((~template.style & DS_CONTROL) || (template.style & WS_VISIBLE)))
            {
                /* By returning TRUE, app has requested a default focus assignment.
                 * WM_INITDIALOG may have changed the tab order, so find the first
                 * tabstop control again. */
                focus = GetNextDlgTabItem( hwnd, 0, FALSE );
                if (!focus) focus = GetNextDlgGroupItem( hwnd, 0, FALSE );
                if (focus)
                    SetFocus( focus );
            }
//// ReactOS see 43396, Fixes setting focus on Open and Close dialogs to the FileName edit control in OpenOffice.
//// This now breaks test_SaveRestoreFocus.
            //DEFDLG_SaveFocus( hwnd );
////
        }
//// ReactOS Rev 30613 & 30644
        if (!(GetWindowLongPtrW( hwnd, GWL_STYLE ) & WS_CHILD))
            SendMessageW( hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_INITIALIZE, 0), 0);
////
        if (template.style & WS_VISIBLE && !(GetWindowLongPtrW( hwnd, GWL_STYLE ) & WS_VISIBLE))
        {
           ShowWindow( hwnd, SW_SHOWNORMAL );   /* SW_SHOW doesn't always work */
           IntNotifyWinEvent(EVENT_SYSTEM_DIALOGSTART, hwnd, OBJID_WINDOW, CHILDID_SELF, 0);
        }
        return hwnd;
    }
    if (modal && ownerEnabled) DIALOG_EnableOwner(owner);
    IntNotifyWinEvent(EVENT_SYSTEM_DIALOGEND, hwnd, OBJID_WINDOW, CHILDID_SELF, 0);
    if( IsWindow(hwnd) )
    {
      DestroyWindow( hwnd );
      //// ReactOS
      if (owner)
      {  ERR("DIALOG_CreateIndirect 1\n");
         if ( NtUserGetThreadState(THREADSTATE_FOREGROUNDTHREAD) && // Rule #1.
             !NtUserQueryWindow(owner, QUERY_WINDOW_FOREGROUND) )
         { ERR("DIALOG_CreateIndirect SFW\n");
            SetForegroundWindow(owner);
         }
      }
      ////
    }
    return 0;
}


/***********************************************************************
 *           DEFDLG_FindDefButton
 *
 * Find the current default push-button.
 */
static HWND DEFDLG_FindDefButton( HWND hwndDlg )
{
    HWND hwndChild, hwndTmp;

    hwndChild = GetWindow( hwndDlg, GW_CHILD );
    while (hwndChild)
    {
        if (SendMessageW( hwndChild, WM_GETDLGCODE, 0, 0 ) & DLGC_DEFPUSHBUTTON)
            break;

        /* Recurse into WS_EX_CONTROLPARENT controls */
        if (GetWindowLongPtrW( hwndChild, GWL_EXSTYLE ) & WS_EX_CONTROLPARENT)
        {
            LONG dsStyle = GetWindowLongPtrW( hwndChild, GWL_STYLE );
            if ((dsStyle & WS_VISIBLE) && !(dsStyle & WS_DISABLED) &&
                (hwndTmp = DEFDLG_FindDefButton(hwndChild)) != NULL)
           return hwndTmp;
        }
        hwndChild = GetWindow( hwndChild, GW_HWNDNEXT );
    }
    return hwndChild;
}


/***********************************************************************
 *           DEFDLG_SetDefId
 *
 * Set the default button id.
 */
static BOOL DEFDLG_SetDefId( HWND hwndDlg, DIALOGINFO *dlgInfo, WPARAM wParam)
{
    DWORD dlgcode=0; /* initialize just to avoid a warning */
    HWND hwndOld, hwndNew = GetDlgItem(hwndDlg, wParam);
    INT old_id = dlgInfo->idResult;

    dlgInfo->idResult = wParam;
    if (hwndNew &&
        !((dlgcode=SendMessageW(hwndNew, WM_GETDLGCODE, 0, 0 ))
            & (DLGC_UNDEFPUSHBUTTON | DLGC_BUTTON)))
        return FALSE;  /* Destination is not a push button */

    /* Make sure the old default control is a valid push button ID */
    hwndOld = GetDlgItem( hwndDlg, old_id );
    if (!hwndOld || !(SendMessageW( hwndOld, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON))
        hwndOld = DEFDLG_FindDefButton( hwndDlg );
    if (hwndOld && hwndOld != hwndNew)
        SendMessageW( hwndOld, BM_SETSTYLE, BS_PUSHBUTTON, TRUE );

    if (hwndNew)
    {
        if(dlgcode & DLGC_UNDEFPUSHBUTTON)
            SendMessageW( hwndNew, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE );
    }
    return TRUE;
}


/***********************************************************************
 *           DEFDLG_SetDefButton
 *
 * Set the new default button to be hwndNew.
 */
static BOOL DEFDLG_SetDefButton( HWND hwndDlg, DIALOGINFO *dlgInfo, HWND hwndNew )
{
    DWORD dlgcode=0; /* initialize just to avoid a warning */
    HWND hwndOld = GetDlgItem( hwndDlg, dlgInfo->idResult );

    if (hwndNew &&
        !((dlgcode=SendMessageW(hwndNew, WM_GETDLGCODE, 0, 0 ))
            & (DLGC_UNDEFPUSHBUTTON | DLGC_DEFPUSHBUTTON)))
    {
        /**
         * Need to draw only default push button rectangle.
         * Since the next control is not a push button, need to draw the push
         * button rectangle for the default control.
         */
        hwndNew = hwndOld;
        dlgcode = SendMessageW(hwndNew, WM_GETDLGCODE, 0, 0 );
    }

    /* Make sure the old default control is a valid push button ID */
    if (!hwndOld || !(SendMessageW( hwndOld, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON))
        hwndOld = DEFDLG_FindDefButton( hwndDlg );
    if (hwndOld && hwndOld != hwndNew)
        SendMessageW( hwndOld, BM_SETSTYLE, BS_PUSHBUTTON, TRUE );

    if (hwndNew)
    {
        if(dlgcode & DLGC_UNDEFPUSHBUTTON)
            SendMessageW( hwndNew, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE );
    }
    return TRUE;
}


/***********************************************************************
 *           DEFDLG_Proc
 *
 * Implementation of DefDlgProc(). Only handle messages that need special
 * handling for dialogs.
 */
static LRESULT DEFDLG_Proc( HWND hwnd, UINT msg, WPARAM wParam,
                            LPARAM lParam, DIALOGINFO *dlgInfo )
{
    switch(msg)
    {
        case WM_ERASEBKGND:
        {
            HBRUSH brush = GetControlColor( hwnd, hwnd, (HDC)wParam, WM_CTLCOLORDLG);
            if (brush)
            {
                RECT rect;
                HDC hdc = (HDC)wParam;
                GetClientRect( hwnd, &rect );
                DPtoLP( hdc, (LPPOINT)&rect, 2 );
                FillRect( hdc, &rect, brush );
            }
            return 1;
        }
        case WM_NCDESTROY:
//// ReactOS
            if ((dlgInfo = (DIALOGINFO *)SetWindowLongPtrW( hwnd, DWLP_ROS_DIALOGINFO, 0 )))
            {
                if (dlgInfo->hUserFont) DeleteObject( dlgInfo->hUserFont );
                if (dlgInfo->hMenu) DestroyMenu( dlgInfo->hMenu );
                HeapFree( GetProcessHeap(), 0, dlgInfo );
                NtUserSetThreadState(0,DF_DIALOGACTIVE);
                NtUserxSetDialogPointer( hwnd, 0 );
            }
             /* Window clean-up */
            return DefWindowProcA( hwnd, msg, wParam, lParam );

        case WM_SHOWWINDOW:
            if (!wParam) DEFDLG_SaveFocus( hwnd );
            return DefWindowProcA( hwnd, msg, wParam, lParam );

        case WM_ACTIVATE:
            { // ReactOS
               DWORD dwSetFlag;
               HWND hwndparent = DIALOG_FindMsgDestination( hwnd );
               // if WA_CLICK/ACTIVE ? set dialog is active.
               dwSetFlag = wParam ? DF_DIALOGACTIVE : 0;
               if (hwndparent != hwnd) NtUserSetThreadState(dwSetFlag, DF_DIALOGACTIVE);
            }
            if (wParam) DEFDLG_RestoreFocus( hwnd, TRUE );
            else DEFDLG_SaveFocus( hwnd );
            return 0;

        case WM_SETFOCUS:
            DEFDLG_RestoreFocus( hwnd, FALSE );
            return 0;

        case DM_SETDEFID:
            if (dlgInfo && !(dlgInfo->flags & DF_END))
                DEFDLG_SetDefId( hwnd, dlgInfo, wParam );
            return 1;

        case DM_GETDEFID:
            if (dlgInfo && !(dlgInfo->flags & DF_END))
            {
                HWND hwndDefId;
                if (dlgInfo->idResult)
                    return MAKELONG( dlgInfo->idResult, DC_HASDEFID );
                if ((hwndDefId = DEFDLG_FindDefButton( hwnd )))
                    return MAKELONG( GetDlgCtrlID( hwndDefId ), DC_HASDEFID);
            }
            return 0;

        case WM_NEXTDLGCTL:
            if (dlgInfo)
            {
                HWND hwndDest = (HWND)wParam;
                if (!lParam)
                    hwndDest = GetNextDlgTabItem(hwnd, GetFocus(), wParam);
                if (hwndDest) DEFDLG_SetFocus( hwndDest );
                DEFDLG_SetDefButton( hwnd, dlgInfo, hwndDest );
            }
            return 0;

        case WM_ENTERMENULOOP:
        case WM_LBUTTONDOWN:
        case WM_NCLBUTTONDOWN:
            {
                HWND hwndFocus = GetFocus();
                if (hwndFocus)
                {
                    /* always make combo box hide its listbox control */
                    if (!SendMessageW( hwndFocus, CB_SHOWDROPDOWN, FALSE, 0 ))
                        SendMessageW( GetParent(hwndFocus), CB_SHOWDROPDOWN, FALSE, 0 );
                }
            }
            return DefWindowProcA( hwnd, msg, wParam, lParam );

        case WM_GETFONT:
            return dlgInfo ? (LRESULT)dlgInfo->hUserFont : 0;

        case WM_CLOSE:
            PostMessageA( hwnd, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED),
                            (LPARAM)GetDlgItem( hwnd, IDCANCEL ) );
            return 0;
    }
    return 0;
}

/***********************************************************************
 *           DEFDLG_Epilog
 */
static LRESULT DEFDLG_Epilog(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, BOOL fResult, BOOL fAnsi)
{
    if ((msg >= WM_CTLCOLORMSGBOX && msg <= WM_CTLCOLORSTATIC) ||
         msg == WM_CTLCOLOR)
       {
          if (fResult) return fResult;

          return fAnsi ? DefWindowProcA(hwnd, msg, wParam, lParam):
                         DefWindowProcW(hwnd, msg, wParam, lParam);
       }
    if ( msg == WM_COMPAREITEM ||
         msg == WM_VKEYTOITEM || msg == WM_CHARTOITEM ||
         msg == WM_QUERYDRAGICON || msg == WM_INITDIALOG)
        return fResult;

    return GetWindowLongPtrW( hwnd, DWLP_MSGRESULT );
}

/***********************************************************************
 *           DIALOG_GetNextTabItem
 *
 * Helper for GetNextDlgTabItem
 */
static HWND DIALOG_GetNextTabItem( HWND hwndMain, HWND hwndDlg, HWND hwndCtrl, BOOL fPrevious )
{
    LONG dsStyle;
    LONG exStyle;
    UINT wndSearch = fPrevious ? GW_HWNDPREV : GW_HWNDNEXT;
    HWND retWnd = 0;
    HWND hChildFirst = 0;

    if(!hwndCtrl)
    {
        hChildFirst = GetWindow(hwndDlg,GW_CHILD);
        if(fPrevious) hChildFirst = GetWindow(hChildFirst,GW_HWNDLAST);
    }
    else if (IsChild( hwndMain, hwndCtrl ))
    {
        hChildFirst = GetWindow(hwndCtrl,wndSearch);
        if(!hChildFirst)
        {
            if(GetParent(hwndCtrl) != hwndMain)
                /* i.e. if we are not at the top level of the recursion */
                hChildFirst = GetWindow(GetParent(hwndCtrl),wndSearch);
            else
                hChildFirst = GetWindow(hwndCtrl, fPrevious ? GW_HWNDLAST : GW_HWNDFIRST);
        }
    }

    while(hChildFirst)
    {
        dsStyle = GetWindowLongPtrA(hChildFirst,GWL_STYLE);
        exStyle = GetWindowLongPtrA(hChildFirst,GWL_EXSTYLE);
        if( (exStyle & WS_EX_CONTROLPARENT) && (dsStyle & WS_VISIBLE) && !(dsStyle & WS_DISABLED))
        {
            HWND retWnd;
            retWnd = DIALOG_GetNextTabItem(hwndMain,hChildFirst,NULL,fPrevious );
            if (retWnd) return (retWnd);
        }
        else if( (dsStyle & WS_TABSTOP) && (dsStyle & WS_VISIBLE) && !(dsStyle & WS_DISABLED))
        {
            return (hChildFirst);
        }
        hChildFirst = GetWindow(hChildFirst,wndSearch);
    }
    if(hwndCtrl)
    {
        HWND hParent = GetParent(hwndCtrl);
        while(hParent)
        {
            if(hParent == hwndMain) break;
            retWnd = DIALOG_GetNextTabItem(hwndMain,GetParent(hParent),hParent,fPrevious );
            if(retWnd) break;
            hParent = GetParent(hParent);
        }
        if(!retWnd)
            retWnd = DIALOG_GetNextTabItem(hwndMain,hwndMain,NULL,fPrevious );
    }
    return retWnd ? retWnd : hwndCtrl;
}


/**********************************************************************
 *	    DIALOG_DlgDirListW
 *
 * Helper function for DlgDirList*W
 */
static INT DIALOG_DlgDirListW( HWND hDlg, LPWSTR spec, INT idLBox,
                                INT idStatic, UINT attrib, BOOL combo )
{
    HWND hwnd;
    LPWSTR orig_spec = spec;
    WCHAR any[] = {'*','.','*',0};

#define SENDMSG(msg,wparam,lparam) \
    ((attrib & DDL_POSTMSGS) ? PostMessageW( hwnd, msg, wparam, lparam ) \
                             : SendMessageW( hwnd, msg, wparam, lparam ))

    TRACE("%p %s %d %d %04x\n", hDlg, debugstr_w(spec), idLBox, idStatic, attrib );

    /* If the path exists and is a directory, chdir to it */
    if (!spec || !spec[0] || SetCurrentDirectoryW( spec )) spec = any;
    else
    {
        WCHAR *p, *p2;
        p = spec;
        if ((p2 = strchrW( p, ':' ))) p = p2 + 1;
        if ((p2 = strrchrW( p, '\\' ))) p = p2;
        if ((p2 = strrchrW( p, '/' ))) p = p2;
        if (p != spec)
        {
            WCHAR sep = *p;
            *p = 0;
            if (!SetCurrentDirectoryW( spec ))
            {
                *p = sep;  /* Restore the original spec */
                return FALSE;
            }
            spec = p + 1;
        }
    }

    TRACE( "mask=%s\n", spec );

    if (idLBox && ((hwnd = GetDlgItem( hDlg, idLBox )) != 0))
    {
        if (attrib == DDL_DRIVES) attrib |= DDL_EXCLUSIVE;

        SENDMSG( combo ? CB_RESETCONTENT : LB_RESETCONTENT, 0, 0 );
        if (attrib & DDL_DIRECTORY)
        {
            if (!(attrib & DDL_EXCLUSIVE))
            {
                SENDMSG( combo ? CB_DIR : LB_DIR,
                         attrib & ~(DDL_DIRECTORY | DDL_DRIVES),
                         (LPARAM)spec );
            }
            SENDMSG( combo ? CB_DIR : LB_DIR,
                   (attrib & (DDL_DIRECTORY | DDL_DRIVES)) | DDL_EXCLUSIVE,
                   (LPARAM)any );
        }
        else
        {
            SENDMSG( combo ? CB_DIR : LB_DIR, attrib, (LPARAM)spec );
        }
    }

    /* Convert path specification to uppercase */
    if (spec) CharUpperW(spec);

    if (idStatic && ((hwnd = GetDlgItem( hDlg, idStatic )) != 0))
    {
        WCHAR temp[MAX_PATH];
        GetCurrentDirectoryW( sizeof(temp)/sizeof(WCHAR), temp );
        CharLowerW( temp );
        /* Can't use PostMessage() here, because the string is on the stack */
        SetDlgItemTextW( hDlg, idStatic, temp );
    }

    if (orig_spec && (spec != orig_spec))
    {
        /* Update the original file spec */
        WCHAR *p = spec;
        while ((*orig_spec++ = *p++));
    }

    return TRUE;
#undef SENDMSG
}


/**********************************************************************
 *	    DIALOG_DlgDirListA
 *
 * Helper function for DlgDirList*A
 */
static INT DIALOG_DlgDirListA( HWND hDlg, LPSTR spec, INT idLBox,
                               INT idStatic, UINT attrib, BOOL combo )
{
    if (spec)
    {
        INT ret, len = MultiByteToWideChar( CP_ACP, 0, spec, -1, NULL, 0 );
        LPWSTR specW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if (specW == NULL)
            return FALSE;
        MultiByteToWideChar( CP_ACP, 0, spec, -1, specW, len );
        ret = DIALOG_DlgDirListW( hDlg, specW, idLBox, idStatic, attrib, combo );
        WideCharToMultiByte( CP_ACP, 0, specW, -1, spec, 0x7fffffff, NULL, NULL );
        HeapFree( GetProcessHeap(), 0, specW );
        return ret;
    }
    return DIALOG_DlgDirListW( hDlg, NULL, idLBox, idStatic, attrib, combo );
}

/**********************************************************************
 *           DIALOG_DlgDirSelect
 *
 * Helper function for DlgDirSelect*
 */
static BOOL DIALOG_DlgDirSelect( HWND hwnd, LPWSTR str, INT len,
                                 INT id, BOOL unicode, BOOL combo )
{
    WCHAR *buffer, *ptr;
    INT item, size;
    BOOL ret;
    HWND listbox = GetDlgItem( hwnd, id );

    TRACE("%p %s %d\n", hwnd, unicode ? debugstr_w(str) : debugstr_a((LPSTR)str), id );
    if (!listbox) return FALSE;

    item = SendMessageW(listbox, combo ? CB_GETCURSEL : LB_GETCURSEL, 0, 0 );
    if (item == LB_ERR) return FALSE;

    size = SendMessageW(listbox, combo ? CB_GETLBTEXTLEN : LB_GETTEXTLEN, item, 0 );
    if (size == LB_ERR) return FALSE;

    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, (size+2) * sizeof(WCHAR) ))) return FALSE;

    SendMessageW( listbox, combo ? CB_GETLBTEXT : LB_GETTEXT, item, (LPARAM)buffer );

    if ((ret = (buffer[0] == '[')))  /* drive or directory */
    {
        if (buffer[1] == '-')  /* drive */
        {
            buffer[3] = ':';
            buffer[4] = 0;
            ptr = buffer + 2;
        }
        else
        {
            buffer[strlenW(buffer)-1] = '\\';
            ptr = buffer + 1;
        }
    }
    else
    {
        /* Filenames without a dot extension must have one tacked at the end */
        if (strchrW(buffer, '.') == NULL)
        {
            buffer[strlenW(buffer)+1] = '\0';
            buffer[strlenW(buffer)] = '.';
        }
        ptr = buffer;
    }

    if (!unicode)
    {
        if (len > 0 && !WideCharToMultiByte( CP_ACP, 0, ptr, -1, (LPSTR)str, len, 0, 0 ))
            ((LPSTR)str)[len-1] = 0;
    }
    else lstrcpynW( str, ptr, len );
    HeapFree( GetProcessHeap(), 0, buffer );
    TRACE("Returning %d %s\n", ret, unicode ? debugstr_w(str) : debugstr_a((LPSTR)str) );
    return ret;
}


/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
HWND
WINAPI
CreateDialogIndirectParamAorW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE lpTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM lParamInit,
  DWORD Flags)
{
/* FIXME:
 *   This function might be obsolete since I don't think it is exported by NT
 *   Also wine has one more parameter identifying weather it should call
 *   the function with unicode or not
 */
  return DIALOG_CreateIndirect( hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit , Flags == DLG_ISANSI ? FALSE : TRUE, FALSE );
}


/*
 * @implemented
 */
HWND
WINAPI
CreateDialogIndirectParamA(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE lpTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM lParamInit)
{
  return CreateDialogIndirectParamAorW( hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit, DLG_ISANSI);
}


/*
 * @implemented
 */
HWND
WINAPI
CreateDialogIndirectParamW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE lpTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM lParamInit)
{
  return CreateDialogIndirectParamAorW( hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit, 0);
}


/*
 * @implemented
 */
HWND
WINAPI
CreateDialogParamA(
  HINSTANCE hInstance,
  LPCSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
	HRSRC hrsrc;
	LPCDLGTEMPLATE ptr;

    if (!(hrsrc = FindResourceA( hInstance, lpTemplateName, (LPCSTR)RT_DIALOG ))) return 0;
    if (!(ptr = (LPCDLGTEMPLATE)LoadResource(hInstance, hrsrc))) return 0;
	return CreateDialogIndirectParamA( hInstance, ptr, hWndParent, lpDialogFunc, dwInitParam );
}


/*
 * @implemented
 */
HWND
WINAPI
CreateDialogParamW(
  HINSTANCE hInstance,
  LPCWSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
	HRSRC hrsrc;
	LPCDLGTEMPLATE ptr;

	if (!(hrsrc = FindResourceW( hInstance, lpTemplateName, (LPCWSTR)RT_DIALOG ))) return 0;
	if (!(ptr = (LPCDLGTEMPLATE)LoadResource(hInstance, hrsrc))) return 0;
	return CreateDialogIndirectParamW( hInstance, ptr, hWndParent, lpDialogFunc, dwInitParam );
}


/*
 * @implemented
 */
LRESULT
WINAPI
DefDlgProcA(
  HWND hDlg,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
    DIALOGINFO *dlgInfo;
    WNDPROC dlgproc;
    BOOL result = FALSE;

    /* Perform DIALOGINFO initialization if not done */
    if(!(dlgInfo = DIALOG_get_info( hDlg, TRUE ))) return 0;

    SetWindowLongPtrW( hDlg, DWLP_MSGRESULT, 0 );

    if ((dlgproc = (WNDPROC)GetWindowLongPtrW( hDlg, DWLP_DLGPROC )))
    {
        /* Call dialog procedure */
        result = CallWindowProcA( dlgproc, hDlg, Msg, wParam, lParam );
    }

    if (!result && IsWindow(hDlg))
    {
        /* callback didn't process this message */

        switch(Msg)
        {
            case WM_ERASEBKGND:
            case WM_SHOWWINDOW:
            case WM_ACTIVATE:
            case WM_SETFOCUS:
            case DM_SETDEFID:
            case DM_GETDEFID:
            case WM_NEXTDLGCTL:
            case WM_GETFONT:
            case WM_CLOSE:
            case WM_NCDESTROY:
            case WM_ENTERMENULOOP:
            case WM_LBUTTONDOWN:
            case WM_NCLBUTTONDOWN:
                 return DEFDLG_Proc( hDlg, Msg, wParam, lParam, dlgInfo );
            case WM_INITDIALOG:
            case WM_VKEYTOITEM:
            case WM_COMPAREITEM:
            case WM_CHARTOITEM:
                 break;

            default:
                 return DefWindowProcA( hDlg, Msg, wParam, lParam );
        }
    }
    return DEFDLG_Epilog(hDlg, Msg, wParam, lParam, result, TRUE);
}


/*
 * @implemented
 */
LRESULT
WINAPI
DefDlgProcW(
  HWND hDlg,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
    DIALOGINFO *dlgInfo;
    WNDPROC dlgproc;
    BOOL result = FALSE;

    /* Perform DIALOGINFO initialization if not done */
    if(!(dlgInfo = DIALOG_get_info( hDlg, TRUE ))) return 0;

    SetWindowLongPtrW( hDlg, DWLP_MSGRESULT, 0 );

    if ((dlgproc = (WNDPROC)GetWindowLongPtrW( hDlg, DWLP_DLGPROC )))
    {
        /* Call dialog procedure */
        result = CallWindowProcW( dlgproc, hDlg, Msg, wParam, lParam );
    }

    if (!result && IsWindow(hDlg))
    {
        /* callback didn't process this message */

        switch(Msg)
        {
            case WM_ERASEBKGND:
            case WM_SHOWWINDOW:
            case WM_ACTIVATE:
            case WM_SETFOCUS:
            case DM_SETDEFID:
            case DM_GETDEFID:
            case WM_NEXTDLGCTL:
            case WM_GETFONT:
            case WM_CLOSE:
            case WM_NCDESTROY:
            case WM_ENTERMENULOOP:
            case WM_LBUTTONDOWN:
            case WM_NCLBUTTONDOWN:
                 return DEFDLG_Proc( hDlg, Msg, wParam, lParam, dlgInfo );
            case WM_INITDIALOG:
            case WM_VKEYTOITEM:
            case WM_COMPAREITEM:
            case WM_CHARTOITEM:
                 break;

            default:
                 return DefWindowProcW( hDlg, Msg, wParam, lParam );
        }
    }
    return DEFDLG_Epilog(hDlg, Msg, wParam, lParam, result, FALSE);
}


/*
 * @implemented
 */
INT_PTR
WINAPI
DialogBoxIndirectParamAorW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE hDialogTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam,
  DWORD Flags)
{
/* FIXME:
 *  This function might be obsolete since I don't think it is exported by NT
 *  Also wine has one more parameter identifying weather it should call
 *  the function with unicode or not
 */
  HWND hWnd = DIALOG_CreateIndirect( hInstance, hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam, Flags == DLG_ISANSI ? FALSE : TRUE, TRUE );
  if (hWnd) return DIALOG_DoDialogBox( hWnd, hWndParent );
  return -1;
}


/*
 * @implemented
 */
INT_PTR
WINAPI
DialogBoxIndirectParamA(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE hDialogTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  return DialogBoxIndirectParamAorW( hInstance, hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam, DLG_ISANSI);
}


/*
 * @implemented
 */
INT_PTR
WINAPI
DialogBoxIndirectParamW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE hDialogTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  return DialogBoxIndirectParamAorW( hInstance, hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam, 0);
}


/*
 * @implemented
 */
INT_PTR
WINAPI
DialogBoxParamA(
  HINSTANCE hInstance,
  LPCSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
    HWND hwnd;
    HRSRC hrsrc;
    LPCDLGTEMPLATE ptr;
//// ReactOS rev 33532
    if (!(hrsrc = FindResourceA( hInstance, lpTemplateName, (LPCSTR)RT_DIALOG )) ||
        !(ptr = LoadResource(hInstance, hrsrc)))
    {
        SetLastError(ERROR_RESOURCE_NAME_NOT_FOUND);
        return -1;
    }
    if (hWndParent != NULL && !IsWindow(hWndParent))
    {
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
        return 0;
    }
    hwnd = DIALOG_CreateIndirect(hInstance, ptr, hWndParent, lpDialogFunc, dwInitParam, FALSE, TRUE);
    if (hwnd) return DIALOG_DoDialogBox(hwnd, hWndParent);
    return -1;
}


/*
 * @implemented
 */
INT_PTR
WINAPI
DialogBoxParamW(
  HINSTANCE hInstance,
  LPCWSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
    HWND hwnd;
    HRSRC hrsrc;
    LPCDLGTEMPLATE ptr;
//// ReactOS rev 33532
    if (!(hrsrc = FindResourceW( hInstance, lpTemplateName, (LPCWSTR)RT_DIALOG )) ||
        !(ptr = LoadResource(hInstance, hrsrc)))
    {
        SetLastError(ERROR_RESOURCE_NAME_NOT_FOUND);
        return -1;
    }
    if (hWndParent != NULL && !IsWindow(hWndParent))
    {
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
        return 0;
    }
    hwnd = DIALOG_CreateIndirect(hInstance, ptr, hWndParent, lpDialogFunc, dwInitParam, TRUE, TRUE);
    if (hwnd) return DIALOG_DoDialogBox(hwnd, hWndParent);
    return -1;
}


/*
 * @implemented
 */
int
WINAPI
DlgDirListA(
  HWND hDlg,
  LPSTR lpPathSpec,
  int nIDListBox,
  int nIDStaticPath,
  UINT uFileType)
{
    return DIALOG_DlgDirListA( hDlg, lpPathSpec, nIDListBox, nIDStaticPath, uFileType, FALSE );
}


/*
 * @implemented
 */
int
WINAPI
DlgDirListComboBoxA(
  HWND hDlg,
  LPSTR lpPathSpec,
  int nIDComboBox,
  int nIDStaticPath,
  UINT uFiletype)
{
  return DIALOG_DlgDirListA( hDlg, lpPathSpec, nIDComboBox, nIDStaticPath, uFiletype, TRUE );
}


/*
 * @implemented
 */
int
WINAPI
DlgDirListComboBoxW(
  HWND hDlg,
  LPWSTR lpPathSpec,
  int nIDComboBox,
  int nIDStaticPath,
  UINT uFiletype)
{
  return DIALOG_DlgDirListW( hDlg, lpPathSpec, nIDComboBox, nIDStaticPath, uFiletype, TRUE );
}


/*
 * @implemented
 */
int
WINAPI
DlgDirListW(
  HWND hDlg,
  LPWSTR lpPathSpec,
  int nIDListBox,
  int nIDStaticPath,
  UINT uFileType)
{
  return DIALOG_DlgDirListW( hDlg, lpPathSpec, nIDListBox, nIDStaticPath, uFileType, FALSE );
}


/*
 * @implemented
 */
BOOL
WINAPI
DlgDirSelectComboBoxExA(
  HWND hDlg,
  LPSTR lpString,
  int nCount,
  int nIDComboBox)
{
  return DIALOG_DlgDirSelect( hDlg, (LPWSTR)lpString, nCount, nIDComboBox, FALSE, TRUE );
}


/*
 * @implemented
 */
BOOL
WINAPI
DlgDirSelectComboBoxExW(
  HWND hDlg,
  LPWSTR lpString,
  int nCount,
  int nIDComboBox)
{
  return DIALOG_DlgDirSelect( hDlg, (LPWSTR)lpString, nCount, nIDComboBox, TRUE, TRUE );
}


/*
 * @implemented
 */
BOOL
WINAPI
DlgDirSelectExA(
  HWND hDlg,
  LPSTR lpString,
  int nCount,
  int nIDListBox)
{
  return DIALOG_DlgDirSelect( hDlg, (LPWSTR)lpString, nCount, nIDListBox, FALSE, FALSE );
}


/*
 * @implemented
 */
BOOL
WINAPI
DlgDirSelectExW(
  HWND hDlg,
  LPWSTR lpString,
  int nCount,
  int nIDListBox)
{
  return DIALOG_DlgDirSelect( hDlg, lpString, nCount, nIDListBox, TRUE, FALSE );
}


/*
 * @implemented Modified for ReactOS.
 */
BOOL
WINAPI
EndDialog(
  HWND hwnd,
  INT_PTR retval)
{
    BOOL wasEnabled = TRUE;
    DIALOGINFO * dlgInfo;
    HWND owner;
    BOOL wasActive;

    TRACE("%p %ld\n", hwnd, retval );

    if (!(dlgInfo = GETDLGINFO(hwnd)))
    {
        ERR("got invalid window handle (%p); buggy app !?\n", hwnd);
        return FALSE;
    }
    wasActive = (hwnd == GetActiveWindow());
    dlgInfo->idResult = retval;
    dlgInfo->flags |= DF_END;
    wasEnabled = (dlgInfo->flags & DF_OWNERENABLED);

    owner = GetWindow( hwnd, GW_OWNER );
    if (wasEnabled && owner)
        DIALOG_EnableOwner( owner );

    /* Windows sets the focus to the dialog itself in EndDialog */

    if (wasActive && IsChild(hwnd, GetFocus()))
       SetFocus( hwnd );

    /* Don't have to send a ShowWindow(SW_HIDE), just do
       SetWindowPos with SWP_HIDEWINDOW as done in Windows */

    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE
                 | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);

    if (wasActive && owner)
    {
        /* If this dialog was given an owner then set the focus to that owner
           even when the owner is disabled (normally when a window closes any
           disabled windows cannot receive the focus). */
        SetActiveWindow(owner);
    }
    else if (hwnd == GetActiveWindow()) // Check it again!
    {
        NtUserCallNoParam(NOPARAM_ROUTINE_ZAPACTIVEANDFOUS);
    }

    /* unblock dialog loop */
    PostMessageA(hwnd, WM_NULL, 0, 0);
    return TRUE;
}


/*
 * @implemented
 */
LONG
WINAPI
GetDialogBaseUnits(VOID)
{
    static DWORD units;

    if (!units)
    {
        HDC hdc;
        SIZE size;

        if ((hdc = GetDC(0)))
        {
            size.cx = GdiGetCharDimensions( hdc, NULL, &size.cy );
            if (size.cx) units = MAKELONG( size.cx, size.cy );
            ReleaseDC( 0, hdc );
        }
    }
    return units;
}


/*
 * @implemented
 */
int
WINAPI
GetDlgCtrlID(
  HWND hwndCtl)
{
	return GetWindowLongPtrW( hwndCtl, GWLP_ID );
}


/*
 * @implemented
 */
HWND
WINAPI
GetDlgItem(
  HWND hDlg,
  int nIDDlgItem)
{
    int i;
    HWND *list;
    HWND ret = 0;

    if (!hDlg) return 0; 

    list = WIN_ListChildren(hDlg);
    if (!list) return 0;

    for (i = 0; list[i]; i++) if (GetWindowLongPtrW(list[i], GWLP_ID) == nIDDlgItem) break;
    ret = list[i];
    HeapFree(GetProcessHeap(), 0, list);
//    if (!ret) SetLastError(ERROR_CONTROL_ID_NOT_FOUND);
    return ret;
}


/*
 * @implemented
 */
UINT
WINAPI
GetDlgItemInt(
  HWND hDlg,
  int nIDDlgItem,
  BOOL *lpTranslated,
  BOOL bSigned)
{
    char str[30];
    char * endptr;
    LONG_PTR result = 0;

    if (lpTranslated) *lpTranslated = FALSE;
    if (!SendDlgItemMessageA(hDlg, nIDDlgItem, WM_GETTEXT, sizeof(str), (LPARAM)str))
        return 0;
    if (bSigned)
    {
        result = strtol( str, &endptr, 10 );
        if (!endptr || (endptr == str))  /* Conversion was unsuccessful */
            return 0;
        if (((result == LONG_MIN) || (result == LONG_MAX)))
            return 0;
    }
    else
    {
        result = strtoul( str, &endptr, 10 );
        if (!endptr || (endptr == str))  /* Conversion was unsuccessful */
            return 0;
        if (result == ULONG_MAX) return 0;
    }
    if (lpTranslated) *lpTranslated = TRUE;
    return (UINT)result;
}


/*
 * @implemented
 */
UINT
WINAPI
GetDlgItemTextA(
  HWND hDlg,
  int nIDDlgItem,
  LPSTR lpString,
  int nMaxCount)
{
  HWND hWnd = GetDlgItem(hDlg, nIDDlgItem);
  if ( hWnd ) return GetWindowTextA(hWnd, lpString, nMaxCount);
  if ( nMaxCount ) lpString[0] = '\0';
  return 0;
}


/*
 * @implemented
 */
UINT
WINAPI
GetDlgItemTextW(
  HWND hDlg,
  int nIDDlgItem,
  LPWSTR lpString,
  int nMaxCount)
{
  HWND hWnd = GetDlgItem(hDlg, nIDDlgItem);
  if ( hWnd ) return GetWindowTextW(hWnd, lpString, nMaxCount);
  if ( nMaxCount ) lpString[0] = '\0';
  return 0;
}

/*
 * @implemented
 */
HWND
WINAPI
GetNextDlgGroupItem(
  HWND hDlg,
  HWND hCtl,
  BOOL bPrevious)
{
    HWND hwnd, hwndNext, retvalue, hwndLastGroup = 0;
    BOOL fLooped=FALSE;
    BOOL fSkipping=FALSE;

    if (hDlg == hCtl) hCtl = NULL;
    if (!hCtl && bPrevious) return 0;

        /* if the hwndCtrl is the child of the control in the hwndDlg,
         * then the hwndDlg has to be the parent of the hwndCtrl */

    if (hCtl)
    {
        if (!IsChild (hDlg, hCtl)) return 0;
        /* Make sure hwndCtrl is a top-level child */

    }
    else
    {
        /* No ctrl specified -> start from the beginning */
        if (!(hCtl = GetWindow( hDlg, GW_CHILD ))) return 0;
        /* MSDN is wrong. fPrevious does not result in the last child */

        /* No ctrl specified -> start from the beginning */
        if (!(hCtl = GetWindow( hDlg, GW_CHILD ))) return 0;

        /* MSDN is wrong. fPrevious does not result in the last child */

        /* Maybe that first one is valid.  If so then we don't want to skip it*/
        if ((GetWindowLongPtrW( hCtl, GWL_STYLE ) & (WS_VISIBLE|WS_DISABLED)) == WS_VISIBLE)
        {
            return hCtl;
        }

    }

    /* Always go forward around the group and list of controls; for the
     * previous control keep track; for the next break when you find one
     */
    retvalue = hCtl;
    hwnd = hCtl;
    while (hwndNext = GetWindow (hwnd, GW_HWNDNEXT),
           1)
    {
        while (!hwndNext)
        {
            /* Climb out until there is a next sibling of the ancestor or we
             * reach the top (in which case we loop back to the start)
             */
            if (hDlg == GetParent (hwnd))
            {
                /* Wrap around to the beginning of the list, within the same
                 * group. (Once only)
                 */
                if (fLooped) goto end;
                fLooped = TRUE;
                hwndNext = GetWindow (hDlg, GW_CHILD);
            }
            else
            {
                hwnd = GetParent (hwnd);
                hwndNext = GetWindow (hwnd, GW_HWNDNEXT);
            }
        }
        hwnd = hwndNext;

        /* Wander down the leading edge of controlparents */
        while ( (GetWindowLongPtrW (hwnd, GWL_EXSTYLE) & WS_EX_CONTROLPARENT) &&
                ((GetWindowLongPtrW (hwnd, GWL_STYLE) & (WS_VISIBLE | WS_DISABLED)) == WS_VISIBLE) &&
                (hwndNext = GetWindow (hwnd, GW_CHILD)))
            hwnd = hwndNext;
        /* Question.  If the control is a control parent but either has no
         * children or is not visible/enabled then if it has a WS_GROUP does
         * it count?  For that matter does it count anyway?
         * I believe it doesn't count.
         */

        if ((GetWindowLongPtrW (hwnd, GWL_STYLE) & WS_GROUP))
        {
            hwndLastGroup = hwnd;
            if (!fSkipping)
            {
                /* Look for the beginning of the group */
                fSkipping = TRUE;
            }
        }

        if (hwnd == hCtl)
        {
            if (!fSkipping) break;
            if (hwndLastGroup == hwnd) break;
            hwnd = hwndLastGroup;
            fSkipping = FALSE;
            fLooped = FALSE;
        }

        if (!fSkipping &&
            (GetWindowLongPtrW (hwnd, GWL_STYLE) & (WS_VISIBLE|WS_DISABLED)) ==
             WS_VISIBLE)
        {
            retvalue = hwnd;
            if (!bPrevious) break;
        }
    }
end:
    return retvalue;
}


/*
 * @implemented
 */
HWND
WINAPI
GetNextDlgTabItem(
  HWND hDlg,
  HWND hCtl,
  BOOL bPrevious)
{
    PWND pWindow;
      
    pWindow = ValidateHwnd( hDlg );
    if (!pWindow) return NULL;
    if (hCtl)
    {
       pWindow = ValidateHwnd( hCtl );
       if (!pWindow) return NULL;
    }

    /* Undocumented but tested under Win2000 and WinME */
    if (hDlg == hCtl) hCtl = NULL;

    /* Contrary to MSDN documentation, tested under Win2000 and WinME
     * NB GetLastError returns whatever was set before the function was
     * called.
     */
    if (!hCtl && bPrevious) return 0;

    return DIALOG_GetNextTabItem(hDlg, hDlg, hCtl, bPrevious);
}


#if 0
BOOL
WINAPI
IsDialogMessage(
  HWND hDlg,
  LPMSG lpMsg)
{
	return IsDialogMessageW(hDlg, lpMsg);
}
#endif

/***********************************************************************
 *              DIALOG_FixOneChildOnChangeFocus
 *
 * Callback helper for DIALOG_FixChildrenOnChangeFocus
 */

static BOOL CALLBACK DIALOG_FixOneChildOnChangeFocus (HWND hwndChild,
        LPARAM lParam)
{
    /* If a default pushbutton then no longer default */
    if (DLGC_DEFPUSHBUTTON & SendMessageW (hwndChild, WM_GETDLGCODE, 0, 0))
        SendMessageW (hwndChild, BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
    return TRUE;
}

/***********************************************************************
 *              DIALOG_FixChildrenOnChangeFocus
 *
 * Following the change of focus that occurs for example after handling
 * a WM_KEYDOWN VK_TAB in IsDialogMessage, some tidying of the dialog's
 * children may be required.
 */
static void DIALOG_FixChildrenOnChangeFocus (HWND hwndDlg, HWND hwndNext)
{
    INT dlgcode_next = SendMessageW (hwndNext, WM_GETDLGCODE, 0, 0);
    /* INT dlgcode_dlg  = SendMessageW (hwndDlg, WM_GETDLGCODE, 0, 0); */
    /* Windows does ask for this.  I don't know why yet */

    EnumChildWindows (hwndDlg, DIALOG_FixOneChildOnChangeFocus, 0);

    /* If the button that is getting the focus WAS flagged as the default
     * pushbutton then ask the dialog what it thinks the default is and
     * set that in the default style.
     */
    if (dlgcode_next & DLGC_DEFPUSHBUTTON)
    {
        DWORD def_id = SendMessageW (hwndDlg, DM_GETDEFID, 0, 0);
        if (HIWORD(def_id) == DC_HASDEFID)
        {
            HWND hwndDef;
            def_id = LOWORD(def_id);
            hwndDef = GetDlgItem (hwndDlg, def_id);
            if (hwndDef)
            {
                INT dlgcode_def = SendMessageW (hwndDef, WM_GETDLGCODE, 0, 0);
                /* I know that if it is a button then it should already be a
                 * UNDEFPUSHBUTTON, since we have just told the buttons to
                 * change style.  But maybe they ignored our request
                 */
                if ((dlgcode_def & DLGC_BUTTON) &&
                        (dlgcode_def &  DLGC_UNDEFPUSHBUTTON))
                {
                    SendMessageW (hwndDef, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
                }
            }
        }
    }
    else if ((dlgcode_next & DLGC_BUTTON) && (dlgcode_next & DLGC_UNDEFPUSHBUTTON))
    {
        SendMessageW (hwndNext, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
        /* I wonder why it doesn't send a DM_SETDEFID */
    }
}

/***********************************************************************
 *           DIALOG_IdToHwnd
 *
 * A recursive version of GetDlgItem
 *
 * RETURNS
 *  The HWND for a Child ID.
 */
static HWND DIALOG_IdToHwnd( HWND hwndDlg, INT id )
{
    int i;
    HWND *list = WIN_ListChildren( hwndDlg );
    HWND ret = 0;

    if (!list) return 0;

    for (i = 0; list[i]; i++)
    {
        if (GetWindowLongPtrW( list[i], GWLP_ID ) == id)
        {
            ret = list[i];
            break;
        }

        /* Recurse into every child */
        if ((ret = DIALOG_IdToHwnd( list[i], id ))) break;
    }

    HeapFree( GetProcessHeap(), 0, list );
    return ret;
}


/*
 * @implemented
 */
BOOL
WINAPI
IsDialogMessageW(
  HWND hDlg,
  LPMSG lpMsg)
{
    INT dlgCode = 0;

    if (CallMsgFilterW( lpMsg, MSGF_DIALOGBOX )) return TRUE;

    if (hDlg == GetDesktopWindow()) return FALSE;
    if ((hDlg != lpMsg->hwnd) && !IsChild( hDlg, lpMsg->hwnd )) return FALSE;

     hDlg = DIALOG_FindMsgDestination(hDlg);

     switch(lpMsg->message)
     {
     case WM_KEYDOWN:
        dlgCode = SendMessageW( lpMsg->hwnd, WM_GETDLGCODE, lpMsg->wParam, (LPARAM)lpMsg );
         if (dlgCode & DLGC_WANTMESSAGE) break;

         switch(lpMsg->wParam)
         {
         case VK_TAB:
            if (!(dlgCode & DLGC_WANTTAB))
            {
                BOOL fIsDialog = TRUE;
                WND *pWnd = ValidateHwnd(hDlg);

                if (pWnd && TestWindowProcess(pWnd))
                {
                    fIsDialog = (GETDLGINFO(hDlg) != NULL);
                }
  
                SendMessageW(hDlg, WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS), 0);

                /* I am not sure under which circumstances the TAB is handled
                 * each way.  All I do know is that it does not always simply
                 * send WM_NEXTDLGCTL.  (Personally I have never yet seen it
                 * do so but I presume someone has)
                 */
                if (fIsDialog)
                    SendMessageW( hDlg, WM_NEXTDLGCTL, (GetKeyState(VK_SHIFT) & 0x8000), 0 );
                else
                {
                    /* It would appear that GetNextDlgTabItem can handle being
                     * passed hwndDlg rather than NULL but that is undocumented
                     * so let's do it properly
                     */
                    HWND hwndFocus = GetFocus();
                    HWND hwndNext = GetNextDlgTabItem (hDlg,
                            hwndFocus == hDlg ? NULL : hwndFocus,
                            GetKeyState (VK_SHIFT) & 0x8000);
                    if (hwndNext)
                    {
                        dlgCode = SendMessageW (hwndNext, WM_GETDLGCODE,
                                lpMsg->wParam, (LPARAM)lpMsg);
                        if (dlgCode & DLGC_HASSETSEL)
                        {
                            INT maxlen = 1 + SendMessageW (hwndNext, WM_GETTEXTLENGTH, 0, 0);
                            WCHAR *buffer = HeapAlloc (GetProcessHeap(), 0, maxlen * sizeof(WCHAR));
                            if (buffer)
                            {
                                INT length;
                                SendMessageW (hwndNext, WM_GETTEXT, maxlen, (LPARAM) buffer);
                                length = strlenW (buffer);
                                HeapFree (GetProcessHeap(), 0, buffer);
                                SendMessageW (hwndNext, EM_SETSEL, 0, length);
                            }
                        }
                        SetFocus (hwndNext);
                        DIALOG_FixChildrenOnChangeFocus (hDlg, hwndNext);
                    }
                    else
                        return FALSE;
                }
                return TRUE;
            }
            break;

         case VK_RIGHT:
         case VK_DOWN:
         case VK_LEFT:
         case VK_UP:
             if (!(dlgCode & DLGC_WANTARROWS))
             {
                 BOOL fPrevious = (lpMsg->wParam == VK_LEFT || lpMsg->wParam == VK_UP);
                 HWND hwndNext = GetNextDlgGroupItem (hDlg, GetFocus(), fPrevious );
                 SendMessageW( hDlg, WM_NEXTDLGCTL, (WPARAM)hwndNext, 1 );
                 return TRUE;
             }
             break;

         case VK_CANCEL:
         case VK_ESCAPE:
             SendMessageW( hDlg, WM_COMMAND, IDCANCEL, (LPARAM)GetDlgItem( hDlg, IDCANCEL ) );
             return TRUE;

         case VK_EXECUTE:
         case VK_RETURN:
              {
                 DWORD dw;
                 if ((GetFocus() == lpMsg->hwnd) &&
                     (SendMessageW (lpMsg->hwnd, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON))
                 {
                     SendMessageW (hDlg, WM_COMMAND, MAKEWPARAM (GetDlgCtrlID(lpMsg->hwnd),BN_CLICKED), (LPARAM)lpMsg->hwnd);
                 }
                 else if (DC_HASDEFID == HIWORD(dw = SendMessageW (hDlg, DM_GETDEFID, 0, 0)))
                 {
                    HWND hwndDef = DIALOG_IdToHwnd(hDlg, LOWORD(dw));
                    if (!hwndDef || IsWindowEnabled(hwndDef))
                        SendMessageW( hDlg, WM_COMMAND, MAKEWPARAM( LOWORD(dw), BN_CLICKED ), (LPARAM)hwndDef);
                 }
                 else
                 {
                     SendMessageW( hDlg, WM_COMMAND, IDOK, (LPARAM)GetDlgItem( hDlg, IDOK ) );

                 }
             }
             return TRUE;
         }
         break;

     case WM_CHAR:
         /* FIXME Under what circumstances does WM_GETDLGCODE get sent?
          * It does NOT get sent in the test program I have
          */
         dlgCode = SendMessageW( lpMsg->hwnd, WM_GETDLGCODE, lpMsg->wParam, (LPARAM)lpMsg );
         if (dlgCode & (DLGC_WANTCHARS|DLGC_WANTMESSAGE)) break;
         if (lpMsg->wParam == '\t' && (dlgCode & DLGC_WANTTAB)) break;
         /* drop through */

     case WM_SYSCHAR:
         if (DIALOG_IsAccelerator( lpMsg->hwnd, hDlg, lpMsg->wParam ))
         {
             /* don't translate or dispatch */
             return TRUE;
         }
         break;
//// ReactOS
     case WM_SYSKEYDOWN:
         /* If the ALT key is being pressed display the keyboard cues */
         if ( HIWORD(lpMsg->lParam) & KF_ALTDOWN &&
             !(gpsi->dwSRVIFlags & SRVINFO_KBDPREF) && !(gpsi->PUSIFlags & PUSIF_KEYBOARDCUES) )
             SendMessageW(hDlg, WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEACCEL | UISF_HIDEFOCUS), 0);
         break;

     case WM_SYSCOMMAND:
         /* If the ALT key is being pressed display the keyboard cues */
         if ( lpMsg->wParam == SC_KEYMENU &&
             !(gpsi->dwSRVIFlags & SRVINFO_KBDPREF) && !(gpsi->PUSIFlags & PUSIF_KEYBOARDCUES) )
         {
            SendMessageW(hDlg, WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEACCEL | UISF_HIDEFOCUS), 0);
         }
         break;
     }

     TranslateMessage( lpMsg );
     DispatchMessageW( lpMsg );
     return TRUE;
}


/*
 * @implemented
 */
UINT
WINAPI
IsDlgButtonChecked(
  HWND hDlg,
  int nIDButton)
{
  return (UINT)SendDlgItemMessageW( hDlg, nIDButton, BM_GETCHECK, 0, 0 );
}


/*
 * @implemented
 */
BOOL
WINAPI
MapDialogRect(
  HWND hDlg,
  LPRECT lpRect)
{
	DIALOGINFO * dlgInfo;
	if (!(dlgInfo = GETDLGINFO(hDlg))) return FALSE;
	lpRect->left   = MulDiv(lpRect->left, dlgInfo->xBaseUnit, 4);
	lpRect->right  = MulDiv(lpRect->right, dlgInfo->xBaseUnit, 4);
	lpRect->top    = MulDiv(lpRect->top, dlgInfo->yBaseUnit, 8);
	lpRect->bottom = MulDiv(lpRect->bottom, dlgInfo->yBaseUnit, 8);
	return TRUE;
}


/*
 * @implemented
 */
LRESULT
WINAPI
SendDlgItemMessageA(
  HWND hDlg,
  int nIDDlgItem,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
	HWND hwndCtrl;
	if ( hDlg == HWND_TOPMOST || hDlg == HWND_BROADCAST ) return 0; // ReactOS
	hwndCtrl = GetDlgItem( hDlg, nIDDlgItem );
	if (hwndCtrl) return SendMessageA( hwndCtrl, Msg, wParam, lParam );
	else return 0;
}


/*
 * @implemented
 */
LRESULT
WINAPI
SendDlgItemMessageW(
  HWND hDlg,
  int nIDDlgItem,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
	HWND hwndCtrl;
	if ( hDlg == HWND_TOPMOST || hDlg == HWND_BROADCAST ) return 0; // ReactOS
	hwndCtrl = GetDlgItem( hDlg, nIDDlgItem );
	if (hwndCtrl) return SendMessageW( hwndCtrl, Msg, wParam, lParam );
	else return 0;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetDlgItemInt(
  HWND hDlg,
  int nIDDlgItem,
  UINT uValue,
  BOOL bSigned)
{
	char str[20];

	if (bSigned) sprintf( str, "%d", (INT)uValue );
	else sprintf( str, "%u", uValue );
	SendDlgItemMessageA( hDlg, nIDDlgItem, WM_SETTEXT, 0, (LPARAM)str );
	return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetDlgItemTextA(
  HWND hDlg,
  int nIDDlgItem,
  LPCSTR lpString)
{
  HWND hwndCtrl = GetDlgItem( hDlg, nIDDlgItem ); // ReactOS Themes
  if (hwndCtrl) return SetWindowTextA( hwndCtrl, lpString );
  return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetDlgItemTextW(
  HWND hDlg,
  int nIDDlgItem,
  LPCWSTR lpString)
{
  HWND hwndCtrl = GetDlgItem( hDlg, nIDDlgItem ); // ReactOS Themes
  if (hwndCtrl) return SetWindowTextW( hwndCtrl, lpString );
  return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
CheckDlgButton(
  HWND hDlg,
  int nIDButton,
  UINT uCheck)
{
	SendDlgItemMessageW( hDlg, nIDButton, BM_SETCHECK, uCheck, 0 );
	return TRUE;
}

static BOOL CALLBACK CheckRB(HWND hwnd, LPARAM lParam)
{
  LONG lChildID = GetWindowLongPtrW(hwnd, GWLP_ID);
  RADIOGROUP *lpRadioGroup = (RADIOGROUP *)lParam;

  if((lChildID >= lpRadioGroup->firstID) &&
     (lChildID <= lpRadioGroup->lastID))
  {
    if (lChildID == lpRadioGroup->checkID)
    {
      SendMessageW(hwnd, BM_SETCHECK, BST_CHECKED, 0);
    }
    else
    {
      SendMessageW(hwnd, BM_SETCHECK, BST_UNCHECKED, 0);
    }
  }

   return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
CheckRadioButton(
  HWND hDlg,
  int nIDFirstButton,
  int nIDLastButton,
  int nIDCheckButton)
{
  RADIOGROUP radioGroup;

  radioGroup.firstID = nIDFirstButton;
  radioGroup.lastID = nIDLastButton;
  radioGroup.checkID = nIDCheckButton;

  return EnumChildWindows(hDlg, CheckRB, (LPARAM)&radioGroup);
}
