/*
 * Dialog functions
 *
 * Copyright 1993, 1994, 1996 Alexandre Julliard
 */

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define UNICODE
#include <windows.h>
#include <user32/dialog.h>
#include <user32/heapdup.h>
#include <user32/win.h>
#include <user32/sysmetr.h>
#include <user32/debug.h>
#include <user32/msg.h>
#include <user32/widgets.h>



  /* Dialog base units */
WORD xBaseUnit = 0, yBaseUnit = 0;


/***********************************************************************
 *           DIALOG_Init
 *
 * Initialisation of the dialog manager.
 */
WINBOOL DIALOG_Init(void)
{
    TEXTMETRIC tm;
    HDC hdc;
    
      /* Calculate the dialog base units */

    if (!(hdc = CreateDC( L"DISPLAY", NULL, NULL, NULL ))) return FALSE;
    GetTextMetrics( hdc, &tm );
    DeleteDC( hdc );
    xBaseUnit = tm.tmAveCharWidth;
    yBaseUnit = tm.tmHeight;

      /* Dialog units are based on a proportional system font */
      /* so we adjust them a bit for a fixed font. */
    if (!(tm.tmPitchAndFamily & TMPF_FIXED_PITCH))
        xBaseUnit = xBaseUnit * 5 / 4;

    DPRINT( "base units = %d,%d\n",
                    xBaseUnit, yBaseUnit );
    return TRUE;
}

/***********************************************************************
 *           DIALOG_DoDialogBox
 */
INT DIALOG_DoDialogBox( HWND hwnd, HWND owner )
{
    WND * wndPtr;
    DIALOGINFO * dlgInfo;
    MSG msg;
    INT retval;

      /* Owner must be a top-level window */
    owner = WIN_GetTopParent( owner );
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return -1;
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    EnableWindow( owner, FALSE );
    ShowWindow( hwnd, SW_SHOW );

    while (MSG_InternalGetMessage(&msg, hwnd, owner, MSGF_DIALOGBOX, PM_REMOVE,
                                  !(wndPtr->dwStyle & DS_NOIDLEMSG) ))
    {
	if (!IsDialogMessage( hwnd, &msg))
	{
	    TranslateMessage( &msg );
	    DispatchMessage( &msg );
	}
	if (dlgInfo->flags & DF_END) break;
    }
    retval = dlgInfo->idResult;
    EnableWindow( owner, TRUE );
    dlgInfo->flags |= DF_ENDING;   /* try to stop it being destroyed twice */
    DestroyWindow( hwnd );
    return retval;
}



/***********************************************************************
 *           DIALOG_ParseTemplate
 *
 * Fill a DLG_TEMPLATE structure from the dialog template, and return
 * a pointer to the first control.
 */
LPCDLGITEMTEMPLATE  DIALOG_ParseTemplate( LPCDLGTEMPLATE DlgTemplate, DLG_TEMPLATE * result, WINBOOL bUnicode )
{
    WORD *p; 
    
   
    result->dialogEx = FALSE;
    result->helpId   = 0;
    result->exStyle  = DlgTemplate->dwExtendedStyle;
    
    result->nbItems = DlgTemplate->cdit;
    result->x       = DlgTemplate->x;
    result->y       = DlgTemplate->y;
    result->cx      = DlgTemplate->cx;
    result->cy      = DlgTemplate->cy;
    
 
    p = &(DlgTemplate->cy);
    p++;

   
   /* Get the menu name */

    switch((WORD)*p)
    {
    case 0x0000:
        result->menuName = NULL;
        p++;
        break;
    case 0xffff:
	p++;
        result->menuName = (LPWSTR) *p; // Ordinal of Menu resource
        p++;
        break;
    default:
        result->menuName = (LPCWSTR)p;
        p += lstrlenW( (LPCWSTR)p );
	p++;
        break;
    }

    /* Get the class name */



    switch((WORD)*p)
    {
    case 0x0000:
	if ( bUnicode == TRUE )
        	result->className = DIALOG_CLASS_NAMEW;
	else
		result->className = DIALOG_CLASS_NAMEA;
        p++;
        break;
    case 0xffff:
	p++;
        result->className = (LPCWSTR)p;  // Ordinal of predefined class
        p++;
        break;
    default:
        result->className = (LPCWSTR)p;
        p += lstrlenW( (LPCWSTR)p );
	p++;
        break;
    }

    /* Get the window caption */
    if ( *p != 0 ) {
    	result->caption = (LPCWSTR)p;
    	p += lstrlenW( (LPCWSTR)p ) + 1;
 
    	/* Get the font name */

    	if (result->style & DS_SETFONT)
    	{
		result->pointSize = *p;
       		p++;
        	if (result->dialogEx)
        	{
            		result->weight = *p; p++;
            		result->italic = *p; p++;
        	}
        	else
        	{
            		result->weight = FW_DONTCARE;
            		result->italic = FALSE;
        	}
		result->faceName = (LPCWSTR)p;
        	p += lstrlenW( (LPCWSTR)p );
		p++;
	
    	}
    } else {
	result->caption = L"";
    }



    /* First control is on dword boundary */
    return (LPCDLGITEMTEMPLATE )((((int)p) + 3) & ~3);
}

/***********************************************************************
 *           DIALOG_ParseTemplate
 *
 * Fill a DLG_TEMPLATE structure from the dialog template, and return
 * a pointer to the first control.
 */
LPCDLGITEMTEMPLATEEX  DIALOG_ParseTemplateEx( LPCDLGTEMPLATEEX DlgTemplate, DLG_TEMPLATE * result,WINBOOL bUnicode )
{
    WORD *p;


    result->dialogEx = TRUE;
    result->helpId   = DlgTemplate->helpID;
    result->exStyle  = DlgTemplate->exStyle;
    result->style    = DlgTemplate->style;
    
    result->nbItems = DlgTemplate->cDlgItems;
    result->x       = DlgTemplate->x;
    result->y       = DlgTemplate->y;
    result->cx      = DlgTemplate->cx;
    result->cy      = DlgTemplate->cy;

    p = &(DlgTemplate->cy);
    p++;

    /* Get the menu name */

    switch(*p)
    {
    case 0x0000:
        result->menuName = NULL;
        p++;
        break;
    case 0xffff:
	p++;
        result->menuName = (LPCWSTR)(WORD)*( p ); // Ordinal of Menu resource
        p++;
        break;
    default:
        result->menuName = (LPCWSTR)p; 
        p += lstrlenW( (LPCWSTR)p ) + 1;
        break;
    }

    /* Get the class name */

    switch(*p)
    {
    case 0x0000:
        if ( bUnicode == TRUE )
        	result->className = DIALOG_CLASS_NAMEW;
	else
		result->className = DIALOG_CLASS_NAMEA;
        p++;
        break;
    case 0xffff:
	p++;
        result->className = (LPCWSTR)(WORD)*( p );
        p ++;
	DPRINT( " CLASS %04x\n", LOWORD(result->className) );
        break;
    default:
        result->className = (LPCWSTR)p;
        DPRINT( " CLASS %s\n", debugstr_w( (LPCWSTR)p ));
        p += lstrlenW( (LPCWSTR)p ) + 1;
        break;
    }

    /* Get the window caption */
    if ( *p != 0 ) {
    	result->caption = (LPCWSTR)p;
    	p += lstrlenW( (LPCWSTR)p ) + 1;
 
    	/* Get the font name */

    	if (result->style & DS_SETFONT)
    	{
		result->pointSize = LOWORD(p);
       		p++;
        	if (result->dialogEx)
        	{
            		result->weight = *p; p++;
            		result->italic = LOBYTE(*p); p++;
        	}
        	else
        	{
            		result->weight = FW_DONTCARE;
            		result->italic = FALSE;
        	}
		result->faceName = (LPCWSTR)p;
        	p += lstrlenW( (LPCWSTR)p ) + 1;
	
    	}
    } else 
	result->caption = L"";
    

    /* First control is on dword boundary */
    return (LPDLGITEMTEMPLATEEX )((((int)p) + 3) & ~3);
}


/***********************************************************************
 *           DIALOG_GetControl
 *
 * Return the class and text of the control pointed to by ptr,
 * fill the header structure and return a pointer to the next control.
 */
LPCDLGITEMTEMPLATE DIALOG_GetControl( LPCDLGITEMTEMPLATE DlgItemTemplate, DLG_CONTROL_INFO *info)
{
    WORD *p;
    WORD id;

    info->helpId  = 0;
    info->exStyle = DlgItemTemplate->dwExtendedStyle;
    info->style   = DlgItemTemplate->style;
    
    info->x       = DlgItemTemplate->x;
    info->y       = DlgItemTemplate->y;
    info->cx      = DlgItemTemplate->cx;
    info->cy      = DlgItemTemplate->cy;

  
    info->id = DlgItemTemplate->id;
    

    p = (char *)DlgItemTemplate + sizeof(DLGITEMTEMPLATE);
    p--;   

    if (*p == 0xffff)
    {

       static const WCHAR class_names[6][10] =
        {
            { BUTTON_CLASS_NAME },     /* 0x80 */
            { EDIT_CLASS_NAME },        /* 0x81 */
            { STATIC_CLASS_NAME  },     /* 0x82 */
            { LISTBOX_CLASS_NAME},     /* 0x83 */
            { SCROLLBAR_CLASS_NAME },  /* 0x84 */
            { COMBOBOX_CLASS_NAME }    /* 0x85 */
        };
	p++;
        id = (WORD)*(p);
        if ((id >= 0x80) && (id <= 0x85))
            info->className = (LPCSTR)HEAP_strdupW(GetProcessHeap(),0,class_names[id - 0x80]);
        else
            info->className = NULL;
       
	printf("%S\n",info->className);        

        p++;
    }
    else
    {
        info->className = (LPCWSTR)p;
        p += lstrlenW( (LPCWSTR)p ) + 1;
    }

    if (*p == 0xffff)  /* Is it an integer id? */
    {
	p++;
	info->windowName = (LPCWSTR)(WORD)*(p + 1);
	p++;
    }
    else
    {
	info->windowName = (LPCWSTR)p;
        p += lstrlenW( (LPCWSTR)p ) + 1;
    }



    if (*p)
    {
	p++;
     	info->data = (LPVOID)(p);
     	p += *p / sizeof(WORD);
    }
    else { 
	info->data = NULL;
    	p++;
    }

    /* Next control is on dword boundary */
    return (LPCDLGITEMTEMPLATE)((((int)p) + 3) & ~3);
}


/***********************************************************************
 *           DIALOG_GetControl
 *
 * Return the class and text of the control pointed to by ptr,
 * fill the header structure and return a pointer to the next control.
 */
LPCDLGITEMTEMPLATEEX DIALOG_GetControlEx( LPCDLGITEMTEMPLATEEX DlgItemTemplate, DLG_CONTROL_INFO *info )
{
    WORD *p;
    WORD id;
    info->helpId  = DlgItemTemplate->helpID;
    info->exStyle = DlgItemTemplate->exStyle;
    info->style   = DlgItemTemplate->style;
  
    info->x       = DlgItemTemplate->x;
    info->y       = DlgItemTemplate->y;
    info->cx      = DlgItemTemplate->cx;
    info->cy      = DlgItemTemplate->cy;

  
    /* id is a DWORD for DIALOGEX */
    info->id = DlgItemTemplate->id;
  
    p = (char *)DlgItemTemplate + sizeof(DLGITEMTEMPLATEEX);
    p--;

    if (*p == 0xffff)
    {

        static const WCHAR class_names[6][10] =
        {
            { L"Button" },     /* 0x80 */
            { L"Edit"},        /* 0x81 */
            { L"Static" },     /* 0x82 */
            { L"ListBox"},     /* 0x83 */
            { L"ScrollBar" },  /* 0x84 */
            { L"ComboBox" }    /* 0x85 */
        };
	p++;
        id = (WORD)*(p);
        if ((id >= 0x80) && (id <= 0x85))
            info->className = (LPCSTR)HEAP_strdupW(GetProcessHeap(),0,class_names[id - 0x80]);
        else
            info->className = NULL;
        
        p++;
    }
    else
    {
        info->className = (LPCWSTR)p;
        p += lstrlenW( (LPCWSTR)p ) + 1;
    }

    if (*p == 0xffff)  /* Is it an integer id? */
    {
	p++;
	info->windowName = (LPCWSTR)(WORD)*(p);
	p++;
    }
    else
    {
	
	info->windowName = (LPCWSTR)p;
        p += lstrlenW( (LPCWSTR)p ) + 1;
    }



    if (*p) {

	p++;
     	info->data = (LPVOID)(p);
     	p += *p / sizeof(WORD);
    }
    else {
	info->data = NULL;
    	p++;
    }

    /* Next control is on dword boundary */
    return (LPCDLGITEMTEMPLATE)((((int)p) + 3) & ~3);
}


/***********************************************************************
 *           DIALOG_CreateControls
 *
 * Create the control windows for a dialog.
 */
WINBOOL DIALOG_CreateControls( HANDLE hWndDialog, DIALOGINFO *dlgInfo , 
                                     void *template, INT items,
                                     HINSTANCE hInst, WINBOOL bDialogEx)
{
    DLG_CONTROL_INFO info;
    HWND hwndCtrl, hwndDefButton = 0;
   
    while (items--)
    {
       
	if ( bDialogEx)
         	template = (void *)DIALOG_GetControlEx( (LPDLGITEMTEMPLATEEX)template, &info );
	else
		template = (void *)DIALOG_GetControl( (LPDLGITEMTEMPLATE)template, &info );

        hwndCtrl = CreateWindowExW( info.exStyle | WS_EX_NOPARENTNOTIFY,
                          (LPCWSTR)info.className,
                          (LPCWSTR)info.windowName,
                          info.style | WS_CHILD | WS_THICKFRAME | WS_VISIBLE,
                          info.x * dlgInfo->xBaseUnit / 4,
                          info.y * dlgInfo->yBaseUnit / 8,
                          info.cx * dlgInfo->xBaseUnit / 4 ,
                          info.cy * dlgInfo->yBaseUnit / 8,
                          hWndDialog, (HMENU)info.id,
                          hInst, info.data );
      
	
        if (hwndCtrl) {

            /* Send initialisation messages to the control */
        	if (dlgInfo->hUserFont) SendMessage( hwndCtrl, WM_SETFONT,
                                             (WPARAM)dlgInfo->hUserFont, 0 );
        	if (SendMessage(hwndCtrl, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON)
        	{
              	/* If there's already a default push-button, set it back */
              	/* to normal and use this one instead. */
            		if (hwndDefButton)
                		SendMessage( hwndDefButton, BM_SETSTYLE, BS_PUSHBUTTON,FALSE );
            		hwndDefButton = hwndCtrl;
            		dlgInfo->idResult = GetWindowLong( hwndCtrl, GWL_ID );
        	}
	}
    }    
    return TRUE;
}





/***********************************************************************
 *           DIALOG_CreateIndirect
 */
HWND DIALOG_CreateIndirect( HINSTANCE hInst, void *dlgTemplate, HWND owner,
                              DLGPROC dlgProc, LPARAM param,
                              WINBOOL bUnicode )
{
    HMENU hMenu = 0;
    HFONT hFont = 0;
    HWND hwnd;
    RECT rect;
    WND * wndPtr;
    DLG_TEMPLATE template;
    DIALOGINFO * dlgInfo;
    WORD xUnit = xBaseUnit;
    WORD yUnit = yBaseUnit;
    void *dlgItemTemplate;



    if ( xBaseUnit == 0 )
	DIALOG_Init();

    xUnit = xBaseUnit;
    yUnit = yBaseUnit;

      /* Parse dialog template */

 
    if (((LPDLGTEMPLATEEX)dlgTemplate)->signature != 0xffff)  /* DIALOGEX resource */
	dlgItemTemplate = (void *)DIALOG_ParseTemplate((LPCDLGTEMPLATE) dlgTemplate, &template, bUnicode );	
    else 
    	dlgItemTemplate = (void *)DIALOG_ParseTemplateEx( (LPCDLGTEMPLATEEX)dlgTemplate, &template, bUnicode );	
    

      /* Load menu */
    if (template.menuName)
   	hMenu = LoadMenuW( hInst, (LPCWSTR)template.menuName );


      /* Create custom font if needed */

    if (template.style & DS_SETFONT)
    {
          /* The font height must be negative as it is a point size */
          /* (see CreateFont() documentation in the Windows SDK).   */

	
	hFont = CreateFontW( -template.pointSize, 0, 0, 0,
                                   template.weight, template.italic, FALSE,
                                   FALSE, DEFAULT_CHARSET, 0, 0, PROOF_QUALITY,
                                   FF_DONTCARE, (LPCWSTR)template.faceName );
	
	if (hFont)
	{
	    TEXTMETRIC tm;
	    HFONT oldFont;

	    HDC hdc = GetDC(0);
	    oldFont = SelectObject( hdc, hFont );
	    GetTextMetrics( hdc, &tm );
	    SelectObject( hdc, oldFont );
	    ReleaseDC( 0, hdc );
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
    AdjustWindowRectEx( &rect, template.style, 
                          hMenu ? TRUE : FALSE , template.exStyle );
    rect.right -= rect.left;
    rect.bottom -= rect.top;

    if ((INT)template.x == CW_USEDEFAULT)
    {
        rect.left = rect.top = CW_USEDEFAULT;
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
            INT dX, dY;

            if( !(template.style & DS_ABSALIGN) )
                ClientToScreen( owner, (POINT *)&rect );
	    
            /* try to fit it into the desktop */

            if( (dX = rect.left + rect.right + SYSMETRICS_CXDLGFRAME  - SYSMETRICS_CXSCREEN) > 0 ) 
		rect.left -= dX;
            if( (dY = rect.top + rect.bottom + SYSMETRICS_CYDLGFRAME  - SYSMETRICS_CYSCREEN) > 0 ) 
		rect.top -= dY;
            if( rect.left < 0 ) rect.left = 0;
            if( rect.top < 0 ) rect.top = 0;
        }
    }


// template.style & ~WS_VISIBLE

template.style |= WS_VISIBLE;
template.style |= WS_THICKFRAME;
    hwnd = CreateWindowExW(template.exStyle, (LPCWSTR)template.className,
                                 (LPCWSTR)template.caption,
                                 template.style ,
                                 rect.left, rect.top, rect.right, rect.bottom ,
                                 owner, hMenu, hInst, NULL );
  
    
	
    if (!hwnd)
    {
	if (hFont) DeleteObject( hFont );
	if (hMenu) DestroyMenu( hMenu );
	return 0;
    }
    wndPtr = WIN_FindWndPtr( hwnd );
    wndPtr->flags |= WIN_ISDIALOG;
    wndPtr->helpContext = template.helpId;
    wndPtr->winproc = dlgProc;

      /* Initialise dialog extra data */

    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    dlgInfo->dlgProc   = dlgProc;
    dlgInfo->hUserFont = hFont;
    dlgInfo->hMenu     = hMenu;
    dlgInfo->xBaseUnit = xUnit;
    dlgInfo->yBaseUnit = yUnit;
    dlgInfo->msgResult = 0;
    dlgInfo->idResult  = 0;
    dlgInfo->flags     = 0;
    dlgInfo->hDialogHeap = 0;

    if (dlgInfo->hUserFont)
        MSG_SendMessage( wndPtr, WM_SETFONT, (WPARAM)dlgInfo->hUserFont, 0L);

    /* Create controls */



    if (!DIALOG_CreateControls( hwnd, dlgInfo, dlgItemTemplate, template.nbItems, hInst , template.dialogEx))
    {
	DestroyWindow( hwnd );
	if (hFont) DeleteObject( hFont );
	if (hMenu) DestroyMenu( hMenu );
   	return 0;
    }
   
	
       /* Send initialisation messages and set focus */

    dlgInfo->hwndFocus = GetNextDlgTabItem( hwnd, 0, FALSE );

    if (MSG_SendMessage( wndPtr, WM_INITDIALOG, (WPARAM)dlgInfo->hwndFocus, param))
            SetFocus( dlgInfo->hwndFocus );


    //if (template.style & WS_VISIBLE && !(wndPtr->dwStyle & WS_VISIBLE)) 
    //{
	   ShowWindow( hwnd, SW_SHOWNORMAL );	/* SW_SHOW doesn't always work */
	   UpdateWindow( hwnd );
   // }


  PAINT_RedrawWindow( wndPtr->hwndSelf, NULL, 0,
                                RDW_INVALIDATE | RDW_ALLCHILDREN |
                                RDW_FRAME | RDW_ERASENOW | RDW_ERASE, 0 );
    return hwnd;
 
}






/***********************************************************************
 *           DIALOG_IsAccelerator
 */
WINBOOL DIALOG_IsAccelerator( HWND hwnd, HWND hwndDlg, WPARAM vKey )
{
    HWND hwndControl = hwnd;
    HWND hwndNext;
    WND *wndPtr;
    WINBOOL RetVal = FALSE;
    INT dlgCode;

    if (vKey == VK_SPACE)
    {
        dlgCode = SendMessage( hwndControl, WM_GETDLGCODE, 0, 0 );
        if (dlgCode & DLGC_BUTTON)
        {
            SendMessage( hwndControl, WM_LBUTTONDOWN, 0, 0);
            SendMessage( hwndControl, WM_LBUTTONUP, 0, 0);
            RetVal = TRUE;
        }
    }
    else
    {
        do
        {
            wndPtr = WIN_FindWndPtr( hwndControl );
            if (wndPtr != NULL && wndPtr->text != NULL && 
                    (wndPtr->dwStyle & (WS_VISIBLE | WS_DISABLED)) == WS_VISIBLE)
            {
                dlgCode = SendMessage( hwndControl, WM_GETDLGCODE, 0, 0 );
                if (dlgCode & (DLGC_BUTTON | DLGC_STATIC))
                {
                    /* find the accelerator key */
                    LPSTR p = wndPtr->text - 2;
                    do
                    {
                        p = strchr( p + 2, '&' );
                    }
                    while (p != NULL && p[1] == '&');

                    /* and check if it's the one we're looking for */
                    if (p != NULL && toupper( p[1] ) == toupper( vKey ) )
                    {
                        if ((dlgCode & DLGC_STATIC) || 
                            (wndPtr->dwStyle & 0x0f) == BS_GROUPBOX )
                        {
                            /* set focus to the control */
                            SendMessage( hwndDlg, WM_NEXTDLGCTL,
                                    hwndControl, 1);
                            /* and bump it on to next */
                            SendMessage( hwndDlg, WM_NEXTDLGCTL, 0, 0);
                        }
                        else if (dlgCode & 
			    (DLGC_DEFPUSHBUTTON | DLGC_UNDEFPUSHBUTTON))
                        {
                            /* send command message as from the control */
                            SendMessage( hwndDlg, WM_COMMAND, 
                                MAKEWPARAM( LOWORD(wndPtr->wIDmenu), 
                                    BN_CLICKED ),
                                (LPARAM)hwndControl );
                        }
                        else
                        {
                            /* click the control */
                            SendMessage( hwndControl, WM_LBUTTONDOWN, (WPARAM) 0, (LPARAM)0);
                            SendMessage( hwndControl, WM_LBUTTONUP, (WPARAM)0, (LPARAM)0);
                        }
                        RetVal = TRUE;
                        break;
                    }
                }
            }
	    hwndNext = GetWindow( hwndControl, GW_CHILD );
	    if (!hwndNext)
	    {
	        hwndNext = GetWindow( hwndControl, GW_HWNDNEXT );
	    }
	    while (!hwndNext)
	    {
		hwndControl = GetParent( hwndControl );
		if (hwndControl == hwndDlg)
		{
		    hwndNext = GetWindow( hwndDlg, GW_CHILD );
		}
		else
		{
		    hwndNext = GetWindow( hwndControl, GW_HWNDNEXT );
		}
	    }
            hwndControl = hwndNext;
        }
        while (hwndControl != hwnd);
    }
    return RetVal;
}
 

/***********************************************************************
 *           DIALOG_IsDialogMessage
 */
WINBOOL DIALOG_IsDialogMessage( HWND hwnd, HWND hwndDlg,
                                      UINT message, WPARAM wParam,
                                      LPARAM lParam, WINBOOL *translate,
                                      WINBOOL *dispatch, INT dlgCode )
{
    *translate = *dispatch = FALSE;

    if (message == WM_PAINT)
    {
        /* Apparently, we have to handle this one as well */
        *dispatch = TRUE;
        return TRUE;
    }

      /* Only the key messages get special processing */
    if ((message != WM_KEYDOWN) &&
        (message != WM_SYSCHAR) &&
	(message != WM_CHAR))
        return FALSE;

    if (dlgCode & DLGC_WANTMESSAGE)
    {
        *translate = *dispatch = TRUE;
        return TRUE;
    }

    switch(message)
    {
    case WM_KEYDOWN:
        switch(wParam)
        {
        case VK_TAB:
            if (!(dlgCode & DLGC_WANTTAB))
            {
                SendMessageA( hwndDlg, WM_NEXTDLGCTL,
                                (GetKeyState(VK_SHIFT) & 0x8000), 0 );
                return TRUE;
            }
            break;
            
        case VK_RIGHT:
        case VK_DOWN:
        case VK_LEFT:
        case VK_UP:
            if (!(dlgCode & DLGC_WANTARROWS))
            {
                WINBOOL fPrevious = (wParam == VK_LEFT || wParam == VK_UP);
                HWND hwndNext = 
                    GetNextDlgGroupItem (hwndDlg, GetFocus(), fPrevious );
                SendMessageA( hwndDlg, WM_NEXTDLGCTL, hwndNext, 1 );
                return TRUE;
            }
            break;

        case VK_ESCAPE:
            SendMessageA( hwndDlg, WM_COMMAND, IDCANCEL,
                            (LPARAM)GetDlgItem( hwndDlg, IDCANCEL ) );
            return TRUE;

        case VK_RETURN:
            {
                DWORD dw = SendMessage( hwndDlg, DM_GETDEFID, 0, 0 );
                if (HIWORD(dw) == DC_HASDEFID)
                {
                    SendMessageA( hwndDlg, WM_COMMAND, 
                                    MAKEWPARAM( LOWORD(dw), BN_CLICKED ),
                                    (LPARAM)GetDlgItem(hwndDlg, LOWORD(dw)));
                }
                else
                {
                    SendMessageA( hwndDlg, WM_COMMAND, IDOK,
                                    (LPARAM)GetDlgItem( hwndDlg, IDOK ) );
    
                }
            }
            return TRUE;
        }
        *translate = TRUE;
        break; /* case WM_KEYDOWN */

    case WM_CHAR:
        if (dlgCode & DLGC_WANTCHARS) break;
        /* drop through */

    case WM_SYSCHAR:
        if (DIALOG_IsAccelerator( hwnd, hwndDlg, wParam ))
        {
            /* don't translate or dispatch */
            return TRUE;
        }
        break;
    }

    /* If we get here, the message has not been treated specially */
    /* and can be sent to its destination window. */
    *dispatch = TRUE;
    return TRUE;
}



