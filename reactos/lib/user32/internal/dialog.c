/*
 * Dialog functions
 *
 * Copyright 1993, 1994, 1996 Alexandre Julliard
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "dialog.h"
#include "drive.h"
#include "heap.h"
#include "win.h"
#include "ldt.h"
#include "user.h"
#include "winproc.h"
#include "message.h"
#include "sysmetrics.h"
#include "debug.h"


  /* Dialog control information */
typedef struct
{
    DWORD      style;
    DWORD      exStyle;
    DWORD      helpId;
    INT16      x;
    INT16      y;
    INT16      cx;
    INT16      cy;
    UINT32     id;
    LPCSTR     className;
    LPCSTR     windowName;
    LPVOID     data;
} DLG_CONTROL_INFO;

  /* Dialog template */
typedef struct
{
    DWORD      style;
    DWORD      exStyle;
    DWORD      helpId;
    UINT16     nbItems;
    INT16      x;
    INT16      y;
    INT16      cx;
    INT16      cy;
    LPCSTR     menuName;
    LPCSTR     className;
    LPCSTR     caption;
    WORD       pointSize;
    WORD       weight;
    WINBOOL     italic;
    LPCSTR     faceName;
    WINBOOL     dialogEx;
} DLG_TEMPLATE;

  /* Dialog base units */
static WORD xBaseUnit = 0, yBaseUnit = 0;


/***********************************************************************
 *           DIALOG_Init
 *
 * Initialisation of the dialog manager.
 */
WINBOOL DIALOG_Init(void)
{
    TEXTMETRIC16 tm;
    HDC16 hdc;
    
      /* Calculate the dialog base units */

    if (!(hdc = CreateDC16( "DISPLAY", NULL, NULL, NULL ))) return FALSE;
    GetTextMetrics16( hdc, &tm );
    DeleteDC32( hdc );
    xBaseUnit = tm.tmAveCharWidth;
    yBaseUnit = tm.tmHeight;

      /* Dialog units are based on a proportional system font */
      /* so we adjust them a bit for a fixed font. */
    if (!(tm.tmPitchAndFamily & TMPF_FIXED_PITCH))
        xBaseUnit = xBaseUnit * 5 / 4;

    TRACE(dialog, "base units = %d,%d\n",
                    xBaseUnit, yBaseUnit );
    return TRUE;
}


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
    info->exStyle = 0;

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
	info->windowName = (LPCSTR)(UINT32)GET_WORD(p+1);
	p += 3;
    }
    else
    {
	info->windowName = p;
	p += strlen(p) + 1;
    }

    info->data = (LPVOID)(*p ? p + 1 : NULL);  /* FIXME: should be a segptr */
    p += *p + 1;

    if(int_id)
      TRACE(dialog,"   %s %04x %d, %d, %d, %d, %d, %08lx, %08lx\n", 
		      info->className,  LOWORD(info->windowName),
		      info->id, info->x, info->y, info->cx, info->cy,
		      info->style, (DWORD)info->data);
    else
      TRACE(dialog,"   %s '%s' %d, %d, %d, %d, %d, %08lx, %08lx\n", 
		      info->className,  info->windowName,
		      info->id, info->x, info->y, info->cx, info->cy,
		      info->style, (DWORD)info->data);

    return p;
}


/***********************************************************************
 *           DIALOG_GetControl32
 *
 * Return the class and text of the control pointed to by ptr,
 * fill the header structure and return a pointer to the next control.
 */
static const WORD *DIALOG_GetControl32( const WORD *p, DLG_CONTROL_INFO *info,
                                        WINBOOL dialogEx )
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
        if ((id >= 0x80) && (id <= 0x85))
            info->className = (LPCSTR)class_names[id - 0x80];
        else
        {
            info->className = NULL;
            ERR( dialog, "Unknown built-in class id %04x\n", id );
        }
        p += 2;
    }
    else
    {
        info->className = (LPCSTR)p;
        p += lstrlen32W( (LPCWSTR)p ) + 1;
    }

    if (GET_WORD(p) == 0xffff)  /* Is it an integer id? */
    {
	info->windowName = (LPCSTR)(UINT32)GET_WORD(p + 1);
	p += 2;
    }
    else
    {
	info->windowName = (LPCSTR)p;
        p += lstrlen32W( (LPCWSTR)p ) + 1;
    }

    TRACE(dialog,"    %s %s %d, %d, %d, %d, %d, %08lx, %08lx, %08lx\n", 
          debugstr_w( (LPCWSTR)info->className ),
          debugres_w( (LPCWSTR)info->windowName ),
          info->id, info->x, info->y, info->cx, info->cy,
          info->style, info->exStyle, info->helpId );

    if (GET_WORD(p))
    {
        if (TRACE_ON(dialog))
        {
            WORD i, count = GET_WORD(p) / sizeof(WORD);
            TRACE(dialog, "  BEGIN\n");
            TRACE(dialog, "    ");
            for (i = 0; i < count; i++) DUMP( "%04x,", GET_WORD(p+i+1) );
            DUMP("\n");
            TRACE(dialog, "  END\n" );
        }
        info->data = (LPVOID)(p + 1);
        p += GET_WORD(p) / sizeof(WORD);
    }
    else info->data = NULL;
    p++;

    /* Next control is on dword boundary */
    return (const WORD *)((((int)p) + 3) & ~3);
}


/***********************************************************************
 *           DIALOG_CreateControls
 *
 * Create the control windows for a dialog.
 */
static WINBOOL DIALOG_CreateControls( WND *pWnd, LPCSTR template,
                                     const DLG_TEMPLATE *dlgTemplate,
                                     HINSTANCE32 hInst, WINBOOL win32 )
{
    DIALOGINFO *dlgInfo = (DIALOGINFO *)pWnd->wExtra;
    DLG_CONTROL_INFO info;
    HWND32 hwndCtrl, hwndDefButton = 0;
    INT32 items = dlgTemplate->nbItems;

    TRACE(dialog, " BEGIN\n" );
    while (items--)
    {
        if (!win32)
        {
            HINSTANCE16 instance;
            template = DIALOG_GetControl16( template, &info );
            if (HIWORD(info.className) && !strcmp( info.className, "EDIT") &&
                ((pWnd->dwStyle & DS_LOCALEDIT) != DS_LOCALEDIT))
            {
                if (!dlgInfo->hDialogHeap)
                {
                    dlgInfo->hDialogHeap = GlobalAlloc16(GMEM_FIXED, 0x10000);
                    if (!dlgInfo->hDialogHeap)
                    {
                        ERR(dialog, "Insufficient memory to create heap for edit control\n" );
                        continue;
                    }
                    LocalInit(dlgInfo->hDialogHeap, 0, 0xffff);
                }
                instance = dlgInfo->hDialogHeap;
            }
            else instance = (HINSTANCE16)hInst;

            hwndCtrl = CreateWindowEx16( info.exStyle | WS_EX_NOPARENTNOTIFY,
                                         info.className, info.windowName,
                                         info.style | WS_CHILD,
                                         info.x * dlgInfo->xBaseUnit / 4,
                                         info.y * dlgInfo->yBaseUnit / 8,
                                         info.cx * dlgInfo->xBaseUnit / 4,
                                         info.cy * dlgInfo->yBaseUnit / 8,
                                         pWnd->hwndSelf, (HMENU16)info.id,
                                         instance, info.data );
        }
        else
        {
            template = (LPCSTR)DIALOG_GetControl32( (WORD *)template, &info,
                                                    dlgTemplate->dialogEx );
            hwndCtrl = CreateWindowEx32W( info.exStyle | WS_EX_NOPARENTNOTIFY,
                                          (LPCWSTR)info.className,
                                          (LPCWSTR)info.windowName,
                                          info.style | WS_CHILD,
                                          info.x * dlgInfo->xBaseUnit / 4,
                                          info.y * dlgInfo->yBaseUnit / 8,
                                          info.cx * dlgInfo->xBaseUnit / 4,
                                          info.cy * dlgInfo->yBaseUnit / 8,
                                          pWnd->hwndSelf, (HMENU32)info.id,
                                          hInst, info.data );
        }
        if (!hwndCtrl) return FALSE;

            /* Send initialisation messages to the control */
        if (dlgInfo->hUserFont) SendMessage32A( hwndCtrl, WM_SETFONT,
                                             (WPARAM32)dlgInfo->hUserFont, 0 );
        if (SendMessage32A(hwndCtrl, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON)
        {
              /* If there's already a default push-button, set it back */
              /* to normal and use this one instead. */
            if (hwndDefButton)
                SendMessage32A( hwndDefButton, BM_SETSTYLE32,
                                BS_PUSHBUTTON,FALSE );
            hwndDefButton = hwndCtrl;
            dlgInfo->idResult = GetWindowWord32( hwndCtrl, GWW_ID );
        }
    }    
    TRACE(dialog, " END\n" );
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
    result->exStyle = 0;
    result->nbItems = *p++;
    result->x       = GET_WORD(p);  p += sizeof(WORD);
    result->y       = GET_WORD(p);  p += sizeof(WORD);
    result->cx      = GET_WORD(p);  p += sizeof(WORD);
    result->cy      = GET_WORD(p);  p += sizeof(WORD);
    TRACE(dialog, "DIALOG %d, %d, %d, %d\n",
                    result->x, result->y, result->cx, result->cy );
    TRACE(dialog, " STYLE %08lx\n", result->style );

    /* Get the menu name */

    switch( (BYTE)*p )
    {
    case 0:
        result->menuName = 0;
        p++;
        break;
    case 0xff:
        result->menuName = (LPCSTR)(UINT32)GET_WORD( p + 1 );
        p += 3;
	TRACE(dialog, " MENU %04x\n", LOWORD(result->menuName) );
        break;
    default:
        result->menuName = p;
        TRACE(dialog, " MENU '%s'\n", p );
        p += strlen(p) + 1;
        break;
    }

    /* Get the class name */

    if (*p)
    {
        result->className = p;
        TRACE(dialog, " CLASS '%s'\n", result->className );
    }
    else result->className = DIALOG_CLASS_ATOM;
    p += strlen(p) + 1;

    /* Get the window caption */

    result->caption = p;
    p += strlen(p) + 1;
    TRACE(dialog, " CAPTION '%s'\n", result->caption );

    /* Get the font name */

    if (result->style & DS_SETFONT)
    {
	result->pointSize = GET_WORD(p);
        p += sizeof(WORD);
	result->faceName = p;
        p += strlen(p) + 1;
	TRACE(dialog, " FONT %d,'%s'\n",
                        result->pointSize, result->faceName );
    }
    return p;
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

    result->style = GET_DWORD(p); p += 2;
    if (result->style == 0xffff0001)  /* DIALOGEX resource */
    {
        result->dialogEx = TRUE;
        result->helpId   = GET_DWORD(p); p += 2;
        result->exStyle  = GET_DWORD(p); p += 2;
        result->style    = GET_DWORD(p); p += 2;
    }
    else
    {
        result->dialogEx = FALSE;
        result->helpId   = 0;
        result->exStyle  = GET_DWORD(p); p += 2;
    }
    result->nbItems = GET_WORD(p); p++;
    result->x       = GET_WORD(p); p++;
    result->y       = GET_WORD(p); p++;
    result->cx      = GET_WORD(p); p++;
    result->cy      = GET_WORD(p); p++;
    TRACE( dialog, "DIALOG%s %d, %d, %d, %d, %ld\n",
           result->dialogEx ? "EX" : "", result->x, result->y,
           result->cx, result->cy, result->helpId );
    TRACE( dialog, " STYLE 0x%08lx\n", result->style );
    TRACE( dialog, " EXSTYLE 0x%08lx\n", result->exStyle );

    /* Get the menu name */

    switch(GET_WORD(p))
    {
    case 0x0000:
        result->menuName = NULL;
        p++;
        break;
    case 0xffff:
        result->menuName = (LPCSTR)(UINT32)GET_WORD( p + 1 );
        p += 2;
	TRACE(dialog, " MENU %04x\n", LOWORD(result->menuName) );
        break;
    default:
        result->menuName = (LPCSTR)p;
        TRACE(dialog, " MENU %s\n", debugstr_w( (LPCWSTR)p ));
        p += lstrlen32W( (LPCWSTR)p ) + 1;
        break;
    }

    /* Get the class name */

    switch(GET_WORD(p))
    {
    case 0x0000:
        result->className = DIALOG_CLASS_ATOM;
        p++;
        break;
    case 0xffff:
        result->className = (LPCSTR)(UINT32)GET_WORD( p + 1 );
        p += 2;
	TRACE(dialog, " CLASS %04x\n", LOWORD(result->className) );
        break;
    default:
        result->className = (LPCSTR)p;
        TRACE(dialog, " CLASS %s\n", debugstr_w( (LPCWSTR)p ));
        p += lstrlen32W( (LPCWSTR)p ) + 1;
        break;
    }

    /* Get the window caption */

    result->caption = (LPCSTR)p;
    p += lstrlen32W( (LPCWSTR)p ) + 1;
    TRACE(dialog, " CAPTION %s\n", debugstr_w( (LPCWSTR)result->caption ) );

    /* Get the font name */

    if (result->style & DS_SETFONT)
    {
	result->pointSize = GET_WORD(p);
        p++;
        if (result->dialogEx)
        {
            result->weight = GET_WORD(p); p++;
            result->italic = LOBYTE(GET_WORD(p)); p++;
        }
        else
        {
            result->weight = FW_DONTCARE;
            result->italic = FALSE;
        }
	result->faceName = (LPCSTR)p;
        p += lstrlen32W( (LPCWSTR)p ) + 1;
	TRACE(dialog, " FONT %d, %s, %d, %s\n",
              result->pointSize, debugstr_w( (LPCWSTR)result->faceName ),
              result->weight, result->italic ? "TRUE" : "FALSE" );
    }

    /* First control is on dword boundary */
    return (LPCSTR)((((int)p) + 3) & ~3);
}


/***********************************************************************
 *           DIALOG_CreateIndirect
 */
HWND32 DIALOG_CreateIndirect( HINSTANCE32 hInst, LPCSTR dlgTemplate,
                              WINBOOL win32Template, HWND32 owner,
                              DLGPROC16 dlgProc, LPARAM param,
                              WINDOWPROCTYPE procType )
{
    HMENU16 hMenu = 0;
    HFONT16 hFont = 0;
    HWND32 hwnd;
    RECT32 rect;
    WND * wndPtr;
    DLG_TEMPLATE template;
    DIALOGINFO * dlgInfo;
    WORD xUnit = xBaseUnit;
    WORD yUnit = yBaseUnit;

      /* Parse dialog template */

    if (!dlgTemplate) return 0;
    if (win32Template)
        dlgTemplate = DIALOG_ParseTemplate32( dlgTemplate, &template );
    else
        dlgTemplate = DIALOG_ParseTemplate16( dlgTemplate, &template );

      /* Load menu */

    if (template.menuName)
    {
        if (!win32Template)
        {
            LPSTR str = SEGPTR_STRDUP( template.menuName );
	    hMenu = LoadMenu16( hInst, SEGPTR_GET(str) );
            SEGPTR_FREE( str );
	}
        else hMenu = LoadMenu32W( hInst, (LPCWSTR)template.menuName );
    }

      /* Create custom font if needed */

    if (template.style & DS_SETFONT)
    {
          /* The font height must be negative as it is a point size */
          /* (see CreateFont() documentation in the Windows SDK).   */

	if (win32Template)
	    hFont = CreateFont32W( -template.pointSize, 0, 0, 0,
                                   template.weight, template.italic, FALSE,
                                   FALSE, DEFAULT_CHARSET, 0, 0, PROOF_QUALITY,
                                   FF_DONTCARE, (LPCWSTR)template.faceName );
	else
	    hFont = CreateFont16( -template.pointSize, 0, 0, 0, FW_DONTCARE,
				  FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0,
				  PROOF_QUALITY, FF_DONTCARE,
				  template.faceName );
	if (hFont)
	{
	    TEXTMETRIC16 tm;
	    HFONT16 oldFont;

	    HDC32 hdc = GetDC32(0);
	    oldFont = SelectObject32( hdc, hFont );
	    GetTextMetrics16( hdc, &tm );
	    SelectObject32( hdc, oldFont );
	    ReleaseDC32( 0, hdc );
	    xUnit = tm.tmAveCharWidth;
	    yUnit = tm.tmHeight;
            if (!(tm.tmPitchAndFamily & TMPF_FIXED_PITCH))
                xBaseUnit = xBaseUnit * 5 / 4;  /* See DIALOG_Init() */
	}
    }
    
    /* Create dialog main window */

    rect.left = rect.top = 0;
    rect.right = template.cx * xUnit / 4;
    rect.bottom = template.cy * yUnit / 8;
    if (template.style & DS_MODALFRAME)
        template.exStyle |= WS_EX_DLGMODALFRAME;
    AdjustWindowRectEx32( &rect, template.style, 
                          hMenu ? TRUE : FALSE , template.exStyle );
    rect.right -= rect.left;
    rect.bottom -= rect.top;

    if ((INT16)template.x == CW_USEDEFAULT16)
    {
        rect.left = rect.top = (procType == WIN_PROC_16) ? CW_USEDEFAULT16
                                                         : CW_USEDEFAULT32;
    }
    else
    {
        if (template.style & DS_CENTER)
        {
            rect.left = (SYSMETRICS_CXSCREEN - rect.right) / 2;
            rect.top = (SYSMETRICS_CYSCREEN - rect.bottom) / 2;
        }
        else
        {
            rect.left += template.x * xUnit / 4;
            rect.top += template.y * yUnit / 8;
        }
        if ( !(template.style & WS_CHILD) )
	{
            INT16 dX, dY;

            if( !(template.style & DS_ABSALIGN) )
                ClientToScreen32( owner, (POINT32 *)&rect );
	    
            /* try to fit it into the desktop */

            if( (dX = rect.left + rect.right + SYSMETRICS_CXDLGFRAME 
                 - SYSMETRICS_CXSCREEN) > 0 ) rect.left -= dX;
            if( (dY = rect.top + rect.bottom + SYSMETRICS_CYDLGFRAME
                 - SYSMETRICS_CYSCREEN) > 0 ) rect.top -= dY;
            if( rect.left < 0 ) rect.left = 0;
            if( rect.top < 0 ) rect.top = 0;
        }
    }

    if (procType == WIN_PROC_16)
        hwnd = CreateWindowEx16(template.exStyle, template.className,
                                template.caption, template.style & ~WS_VISIBLE,
                                rect.left, rect.top, rect.right, rect.bottom,
                                owner, hMenu, hInst, NULL );
    else
        hwnd = CreateWindowEx32W(template.exStyle, (LPCWSTR)template.className,
                                 (LPCWSTR)template.caption,
                                 template.style & ~WS_VISIBLE,
                                 rect.left, rect.top, rect.right, rect.bottom,
                                 owner, hMenu, hInst, NULL );
	
    if (!hwnd)
    {
	if (hFont) DeleteObject32( hFont );
	if (hMenu) DestroyMenu32( hMenu );
	return 0;
    }
    wndPtr = WIN_FindWndPtr( hwnd );
    wndPtr->flags |= WIN_ISDIALOG;
    wndPtr->helpContext = template.helpId;

      /* Initialise dialog extra data */

    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    WINPROC_SetProc( &dlgInfo->dlgProc, dlgProc, procType, WIN_PROC_WINDOW );
    dlgInfo->hUserFont = hFont;
    dlgInfo->hMenu     = hMenu;
    dlgInfo->xBaseUnit = xUnit;
    dlgInfo->yBaseUnit = yUnit;
    dlgInfo->msgResult = 0;
    dlgInfo->idResult  = 0;
    dlgInfo->flags     = 0;
    dlgInfo->hDialogHeap = 0;

    if (dlgInfo->hUserFont)
        SendMessage32A( hwnd, WM_SETFONT, (WPARAM32)dlgInfo->hUserFont, 0 );

    /* Create controls */

    if (DIALOG_CreateControls( wndPtr, dlgTemplate, &template,
                               hInst, win32Template ))
    {
       /* Send initialisation messages and set focus */

	dlgInfo->hwndFocus = GetNextDlgTabItem32( hwnd, 0, FALSE );

	if (SendMessage32A( hwnd, WM_INITDIALOG, (WPARAM32)dlgInfo->hwndFocus, param ))
            SetFocus32( dlgInfo->hwndFocus );

	if (template.style & WS_VISIBLE && !(wndPtr->dwStyle & WS_VISIBLE)) 
	{
	   ShowWindow32( hwnd, SW_SHOWNORMAL );	/* SW_SHOW doesn't always work */
	   UpdateWindow32( hwnd );
	}
	return hwnd;
    }

    if( IsWindow32(hwnd) ) DestroyWindow32( hwnd );
    return 0;
}



/**********************************************************************
 *           DIALOG_DlgDirSelect
 *
 * Helper function for DlgDirSelect*
 */
static WINBOOL DIALOG_DlgDirSelect( HWND32 hwnd, LPSTR str, INT32 len,
                                   INT32 id, WINBOOL win32, WINBOOL unicode,
                                   WINBOOL combo )
{
    char *buffer, *ptr;
    INT32 item, size;
    WINBOOL ret;
    HWND32 listbox = GetDlgItem32( hwnd, id );

    TRACE(dialog, "%04x '%s' %d\n", hwnd, str, id );
    if (!listbox) return FALSE;
    if (win32)
    {
        item = SendMessage32A(listbox, combo ? CB_GETCURSEL32
                                             : LB_GETCURSEL32, 0, 0 );
        if (item == LB_ERR) return FALSE;
        size = SendMessage32A(listbox, combo ? CB_GETLBTEXTLEN32
                                             : LB_GETTEXTLEN32, 0, 0 );
        if (size == LB_ERR) return FALSE;
    }
    else
    {
        item = SendMessage32A(listbox, combo ? CB_GETCURSEL16
                                             : LB_GETCURSEL16, 0, 0 );
        if (item == LB_ERR) return FALSE;
        size = SendMessage32A(listbox, combo ? CB_GETLBTEXTLEN16
                                             : LB_GETTEXTLEN16, 0, 0 );
        if (size == LB_ERR) return FALSE;
    }

    if (!(buffer = SEGPTR_ALLOC( size+1 ))) return FALSE;

    if (win32)
        SendMessage32A( listbox, combo ? CB_GETLBTEXT32 : LB_GETTEXT32,
                        item, (LPARAM)buffer );
    else
        SendMessage16( listbox, combo ? CB_GETLBTEXT16 : LB_GETTEXT16,
                       item, (LPARAM)SEGPTR_GET(buffer) );

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
            buffer[strlen(buffer)-1] = '\\';
            ptr = buffer + 1;
        }
    }
    else ptr = buffer;

    if (unicode) lstrcpynAtoW( (LPWSTR)str, ptr, len );
    else lstrcpyn32A( str, ptr, len );
    SEGPTR_FREE( buffer );
    TRACE(dialog, "Returning %d '%s'\n", ret, str );
    return ret;
}


/**********************************************************************
 *	    DIALOG_DlgDirList
 *
 * Helper function for DlgDirList*
 */
static INT32 DIALOG_DlgDirList( HWND32 hDlg, LPSTR spec, INT32 idLBox,
                                INT32 idStatic, UINT32 attrib, WINBOOL combo )
{
    int drive;
    HWND32 hwnd;
    LPSTR orig_spec = spec;

#define SENDMSG(msg,wparam,lparam) \
    ((attrib & DDL_POSTMSGS) ? PostMessage32A( hwnd, msg, wparam, lparam ) \
                             : SendMessage32A( hwnd, msg, wparam, lparam ))

    TRACE(dialog, "%04x '%s' %d %d %04x\n",
                    hDlg, spec ? spec : "NULL", idLBox, idStatic, attrib );

    if (spec && spec[0] && (spec[1] == ':'))
    {
        drive = toupper( spec[0] ) - 'A';
        spec += 2;
        if (!DRIVE_SetCurrentDrive( drive )) return FALSE;
    }
    else drive = DRIVE_GetCurrentDrive();

    /* If the path exists and is a directory, chdir to it */
    if (!spec || !spec[0] || DRIVE_Chdir( drive, spec )) spec = "*.*";
    else
    {
        char *p, *p2;
        p = spec;
        if ((p2 = strrchr( p, '\\' ))) p = p2;
        if ((p2 = strrchr( p, '/' ))) p = p2;
        if (p != spec)
        {
            char sep = *p;
            *p = 0;
            if (!DRIVE_Chdir( drive, spec ))
            {
                *p = sep;  /* Restore the original spec */
                return FALSE;
            }
            spec = p + 1;
        }
    }

    TRACE(dialog, "path=%c:\\%s mask=%s\n",
                    'A' + drive, DRIVE_GetDosCwd(drive), spec );

    if (idLBox && ((hwnd = GetDlgItem32( hDlg, idLBox )) != 0))
    {
        SENDMSG( combo ? CB_RESETCONTENT32 : LB_RESETCONTENT32, 0, 0 );
        if (attrib & DDL_DIRECTORY)
        {
            if (!(attrib & DDL_EXCLUSIVE))
            {
                if (SENDMSG( combo ? CB_DIR32 : LB_DIR32,
                             attrib & ~(DDL_DIRECTORY | DDL_DRIVES),
                             (LPARAM)spec ) == LB_ERR)
                    return FALSE;
            }
            if (SENDMSG( combo ? CB_DIR32 : LB_DIR32,
                       (attrib & (DDL_DIRECTORY | DDL_DRIVES)) | DDL_EXCLUSIVE,
                         (LPARAM)"*.*" ) == LB_ERR)
                return FALSE;
        }
        else
        {
            if (SENDMSG( combo ? CB_DIR32 : LB_DIR32, attrib,
                         (LPARAM)spec ) == LB_ERR)
                return FALSE;
        }
    }

    if (idStatic && ((hwnd = GetDlgItem32( hDlg, idStatic )) != 0))
    {
        char temp[512];
        int drive = DRIVE_GetCurrentDrive();
        strcpy( temp, "A:\\" );
        temp[0] += drive;
        lstrcpyn32A( temp + 3, DRIVE_GetDosCwd(drive), sizeof(temp)-3 );
        CharLower32A( temp );
        /* Can't use PostMessage() here, because the string is on the stack */
        SetDlgItemText32A( hDlg, idStatic, temp );
    }

    if (orig_spec && (spec != orig_spec))
    {
        /* Update the original file spec */
        char *p = spec;
        while ((*orig_spec++ = *p++));
    }

    return TRUE;
#undef SENDMSG
}


/**********************************************************************
 *	    DIALOG_DlgDirListW
 *
 * Helper function for DlgDirList*32W
 */
static INT32 DIALOG_DlgDirListW( HWND32 hDlg, LPWSTR spec, INT32 idLBox,
                                 INT32 idStatic, UINT32 attrib, WINBOOL combo )
{
    if (spec)
    {
        LPSTR specA = HEAP_strdupWtoA( GetProcessHeap(), 0, spec );
        INT32 ret = DIALOG_DlgDirList( hDlg, specA, idLBox, idStatic,
                                       attrib, combo );
        lstrcpyAtoW( spec, specA );
        HeapFree( GetProcessHeap(), 0, specA );
        return ret;
    }
    return DIALOG_DlgDirList( hDlg, NULL, idLBox, idStatic, attrib, combo );
}


