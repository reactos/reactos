/*
 * RichEdit20  functions
 *
 * Copyright 2004 by CodeWeavers (Aric Stewart)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winerror.h"
#include "riched20.h"
#include "richedit.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#define ID_EDIT      1

WINE_DEFAULT_DEBUG_CHANNEL(riched20);

HANDLE RICHED20_hHeap = NULL;
static WNDPROC lpfnEditWndProcA = NULL;
static WNDPROC lpfnEditWndProcW = NULL;
static INT RTFInfoOffsetA = 0;
static INT RTFInfoOffsetW = 0;

#define TRACE_EDIT_MSG32(str) \
        TRACE(\
                     "32 bit : " str ": hwnd=%p, wParam=%08x, lParam=%08x\n"\
                     , \
                     hwnd, (UINT)wParam, (UINT)lParam)

VOID RICHEDIT_InitEditControlInfo(void);

/***********************************************************************
 * DllMain [Internal] Initializes the internal 'RICHED20.DLL'.
 *
 * PARAMS
 *     hinstDLL    [I] handle to the DLL's instance
 *     fdwReason   [I]
 *     lpvReserved [I] reserved, must be NULL
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("\n");
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        /* create private heap */
        RICHED20_hHeap = HeapCreate (0, 0x10000, 0);
        /* Retrieve edit control class info */
        RICHEDIT_InitEditControlInfo();
        /* register the Rich Edit class */
        RICHED20_Register ();
        break;

    case DLL_PROCESS_DETACH:
        /* unregister all common control classes */
        RICHED20_Unregister ();
        HeapDestroy (RICHED20_hHeap);
        RICHED20_hHeap = NULL;
        break;
    }
    return TRUE;
}

/* Support routines for window procedure */
   INT RICHEDIT_GetTextRange(HWND hwnd,TEXTRANGEW *tr);
   INT RICHEDIT_GetSelText(HWND hwnd,LPWSTR lpstrBuffer);

typedef struct _RTFControl_info
{
    HWND hwndParent;
    WNDPROC lpfnEditWndProc;
} RTFControl_Info;


/*
 * The issue is that until we remove the edit control back end we need
 * to doctor some of the text, specifically if there is \r we need to change
 * that to \r\n
 */
LPWSTR static RE20_DoctorText(LPARAM lParam)
{
    LPWSTR input = (LPWSTR)lParam;
    INT size = strlenW(input)+1;
    LPWSTR ret = HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*size);
    LPWSTR ptr = input;

    if (strchrW(input,'\r'))
    {
        LPWSTR ptr2;
        static const WCHAR pszwN[]={'\n',0};

        /* resize it */
        while ((ptr2 = strchrW(ptr,'\r')))
        {
            strncpyW(&ret[strlenW(ret)],ptr,ptr2-ptr);
            ptr2++;
            if (*ptr2 != '\n')
            {
                size++;
                ret = HeapReAlloc(GetProcessHeap(),0,ret,sizeof(WCHAR)*size);
                strcatW(ret,pszwN);
            }
            else
                strncpyW(&ret[strlenW(ret)],ptr2,1);
            ptr = ptr2;
        } 
        strncpyW(&ret[strlenW(ret)],ptr,ptr2-ptr);
    }
    else
        strcpyW(ret,input);

    return ret;
}


/*
 *
 * DESCRIPTION:
 * Window procedure of the RichEdit control.
 *
 */
static LRESULT WINAPI RICHED20_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam, BOOL bUnicode)
{
    LONG newstyle = 0;
    LONG style = 0;
    RTFControl_Info *info;
    CHARRANGE *cr;

    if (bUnicode)
        info = (RTFControl_Info *) GetWindowLongW( hwnd, RTFInfoOffsetW );
    else
        info = (RTFControl_Info *) GetWindowLongW( hwnd, RTFInfoOffsetA );

    TRACE("uMsg: 0x%x hwnd: %p\n",uMsg,hwnd);

    switch (uMsg)
    {

    case WM_CREATE:
            TRACE_EDIT_MSG32("WM_CREATE Passed to default");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
        
    case WM_NCCREATE :
	    TRACE_EDIT_MSG32("WM_NCCREATE");

            info = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                              sizeof (RTFControl_Info));
            if (bUnicode)
            {
                SetWindowLongW( hwnd, RTFInfoOffsetW, (LONG)info );
                info->lpfnEditWndProc = lpfnEditWndProcW;
            }
            else
            {
                SetWindowLongA( hwnd, RTFInfoOffsetA, (LONG)info );
                info->lpfnEditWndProc = lpfnEditWndProcA;
            }

	    info->hwndParent = ((LPCREATESTRUCTA) lParam)->hwndParent;

	    /* remove SCROLLBARS from the current window style */
	    newstyle = style = ((LPCREATESTRUCTA) lParam)->style;
            newstyle &= ~WS_HSCROLL;
            newstyle &= ~WS_VSCROLL;
            newstyle &= ~ES_AUTOHSCROLL;
            newstyle &= ~ES_AUTOVSCROLL;
	    SetWindowLongW(hwnd,GWL_STYLE, newstyle);

            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);

    case WM_SETFOCUS :
	    TRACE_EDIT_MSG32("WM_SETFOCUS");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);

    case WM_SIZE :
            TRACE_EDIT_MSG32("WM_SIZE");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);

    case WM_COMMAND :
        TRACE_EDIT_MSG32("WM_COMMAND");
	switch(HIWORD(wParam)) {
		case EN_CHANGE:
		case EN_HSCROLL:
		case EN_KILLFOCUS:
		case EN_SETFOCUS:
		case EN_UPDATE:
		case EN_VSCROLL:
			return SendMessageW(info->hwndParent, WM_COMMAND,
				wParam, (LPARAM)(hwnd));

		case EN_ERRSPACE:
		case EN_MAXTEXT:
			MessageBoxA (hwnd, "RichEdit control out of space.",
                                  "ERROR", MB_OK | MB_ICONSTOP) ;
			return 0 ;
		}

    case EM_STREAMIN:
            TRACE_EDIT_MSG32("EM_STREAMIN");

            return 0;

/* Messages specific to Richedit controls */

    case EM_AUTOURLDETECT:
            TRACE_EDIT_MSG32("EM_AUTOURLDETECT Ignored");
	    return 0;

    case EM_CANPASTE:
            TRACE_EDIT_MSG32("EM_CANPASTE Ignored");
	    return 0;

    case EM_CANREDO:
            TRACE_EDIT_MSG32("EM_CANREDO Ignored");
	    return 0;

    case EM_DISPLAYBAND:
            TRACE_EDIT_MSG32("EM_DISPLAYBAND Ignored");
	    return 0;

    case EM_EXGETSEL:
            TRACE_EDIT_MSG32("EM_EXGETSEL -> EM_GETSEL");
            cr = (VOID *) lParam;
            CallWindowProcW(info->lpfnEditWndProc, hwnd, EM_GETSEL, (INT)&cr->cpMin, (INT)&cr->cpMax);
            TRACE("cpMin: 0x%x cpMax: 0x%x\n",(INT)cr->cpMin,(INT)cr->cpMax);
            return 0;

    case EM_EXLIMITTEXT:
        {
           DWORD limit = lParam;
           TRACE_EDIT_MSG32("EM_EXLIMITTEXT");
           if (limit > 65534)
           {
                limit = 0xFFFFFFFF;
           }
           return CallWindowProcW(info->lpfnEditWndProc, hwnd, EM_SETLIMITTEXT, limit, 0);
        }

    case EM_EXLINEFROMCHAR:
            TRACE_EDIT_MSG32("EM_EXLINEFROMCHAR -> LINEFROMCHAR");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, EM_LINEFROMCHAR, lParam, wParam);

    case EM_EXSETSEL:
            TRACE_EDIT_MSG32("EM_EXSETSEL -> EM_SETSEL");
            cr = (VOID *) lParam;
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, EM_SETSEL, cr->cpMin, cr->cpMax);

    case EM_FINDTEXT:
            TRACE_EDIT_MSG32("EM_FINDTEXT Ignored");
            return 0;

    case EM_FINDTEXTEX:
            TRACE_EDIT_MSG32("EM_FINDTEXTEX Ignored");
            return 0;

    case EM_FINDTEXTEXW:
            TRACE_EDIT_MSG32("EM_FINDTEXTEXW Ignored");
            return 0;

    case EM_FINDTEXTW:
            TRACE_EDIT_MSG32("EM_FINDTEXTW Ignored");
            return 0;

    case EM_FINDWORDBREAK:
            TRACE_EDIT_MSG32("EM_FINDWORDBREAK Ignored");
            return 0;

    case EM_FORMATRANGE:
            TRACE_EDIT_MSG32("EM_FORMATRANGE Ignored");
            return 0;

    case EM_GETAUTOURLDETECT:
            TRACE_EDIT_MSG32("EM_GETAUTOURLDETECT Ignored");
            return 0;

    case EM_GETBIDIOPTIONS:
            TRACE_EDIT_MSG32("EM_GETBIDIOPTIONS Ignored");
            return 0;

    case EM_GETCHARFORMAT:
            TRACE_EDIT_MSG32("EM_GETCHARFORMAT Ignored");
            return 0;

    case EM_GETEDITSTYLE:
            TRACE_EDIT_MSG32("EM_GETEDITSTYLE Ignored");
            return 0;

    case EM_GETEVENTMASK:
            TRACE_EDIT_MSG32("EM_GETEVENTMASK Ignored");
            return 0;

    case EM_GETIMECOLOR:
            TRACE_EDIT_MSG32("EM_GETIMECOLOR Ignored");
            return 0;

    case EM_GETIMECOMPMODE:
            TRACE_EDIT_MSG32("EM_GETIMECOMPMODE Ignored");
            return 0;

    case EM_GETIMEOPTIONS:
            TRACE_EDIT_MSG32("EM_GETIMEOPTIONS Ignored");
            return 0;

    case EM_GETLANGOPTIONS:
            TRACE_EDIT_MSG32("STUB: EM_GETLANGOPTIONS");
            return 0;

    case EM_GETOPTIONS:
            TRACE_EDIT_MSG32("EM_GETOPTIONS Ignored");
            return 0;

    case EM_GETPARAFORMAT:
            TRACE_EDIT_MSG32("EM_GETPARAFORMAT Ignored");
            return 0;

    case EM_GETPUNCTUATION:
            TRACE_EDIT_MSG32("EM_GETPUNCTUATION Ignored");
            return 0;

    case EM_GETREDONAME:
            TRACE_EDIT_MSG32("EM_GETREDONAME Ignored");
            return 0;

    case EM_GETSCROLLPOS:
            TRACE_EDIT_MSG32("EM_GETSCROLLPOS Ignored");
            return 0;

    case EM_GETSELTEXT:
            TRACE_EDIT_MSG32("EM_GETSELTEXT");
            return RICHEDIT_GetSelText(hwnd,(void *)lParam);

    case EM_GETTEXTEX:
            TRACE_EDIT_MSG32("EM_GETTEXTEX Ignored");
            return 0;

    case EM_GETTEXTLENGTHEX:
            TRACE_EDIT_MSG32("EM_GETTEXTLENGTHEX Ignored");
            return 0;

    case EM_GETTEXTMODE:
            TRACE_EDIT_MSG32("EM_GETTEXTMODE Ignored");
            return 0;

    case EM_GETTEXTRANGE:
            TRACE_EDIT_MSG32("EM_GETTEXTRANGE");
            return RICHEDIT_GetTextRange(hwnd,(TEXTRANGEW *)lParam);

    case EM_GETTYPOGRAPHYOPTIONS:
            TRACE_EDIT_MSG32("EM_GETTYPOGRAPHYOPTIONS Ignored");
            return 0;

    case EM_GETUNDONAME:
            TRACE_EDIT_MSG32("EM_GETUNDONAME Ignored");
            return 0;

    case EM_GETWORDBREAKPROCEX:
            TRACE_EDIT_MSG32("EM_GETWORDBREAKPROCEX Ignored");
            return 0;

    case EM_GETWORDWRAPMODE:
            TRACE_EDIT_MSG32("EM_GETWORDWRAPMODE Ignored");
            return 0;

    case EM_GETZOOM:
            TRACE_EDIT_MSG32("EM_GETZOOM Ignored");
            return 0;

    case EM_HIDESELECTION:
            TRACE_EDIT_MSG32("EM_HIDESELECTION Ignored");
            return 0;

    case EM_PASTESPECIAL:
            TRACE_EDIT_MSG32("EM_PASTESPECIAL Ignored");
            return 0;

    case EM_RECONVERSION:
            TRACE_EDIT_MSG32("EM_RECONVERSION Ignored");
            return 0;

    case EM_REDO:
            TRACE_EDIT_MSG32("EM_REDO Ignored");
            return 0;

    case EM_REQUESTRESIZE:
            TRACE_EDIT_MSG32("EM_REQUESTRESIZE Ignored");
            return 0;

    case EM_SELECTIONTYPE:
            TRACE_EDIT_MSG32("EM_SELECTIONTYPE Ignored");
            return 0;

    case EM_SETBIDIOPTIONS:
            TRACE_EDIT_MSG32("EM_SETBIDIOPTIONS Ignored");
            return 0;

    case EM_SETBKGNDCOLOR:
            TRACE_EDIT_MSG32("EM_SETBKGNDCOLOR Ignored");
            return 0;

    case EM_SETCHARFORMAT:
            TRACE_EDIT_MSG32("EM_SETCHARFORMAT Ignored");
            return 0;

    case EM_SETEDITSTYLE:
            TRACE_EDIT_MSG32("EM_SETEDITSTYLE Ignored");
            return 0;

    case EM_SETEVENTMASK:
            TRACE_EDIT_MSG32("EM_SETEVENTMASK Ignored");
            return 0;

    case EM_SETFONTSIZE:
            TRACE_EDIT_MSG32("EM_SETFONTSIZE Ignored");
            return 0;

    case EM_SETIMECOLOR:
            TRACE_EDIT_MSG32("EM_SETIMECOLO Ignored");
            return 0;

    case EM_SETIMEOPTIONS:
            TRACE_EDIT_MSG32("EM_SETIMEOPTIONS Ignored");
            return 0;

    case EM_SETLANGOPTIONS:
            TRACE_EDIT_MSG32("EM_SETLANGOPTIONS Ignored");
            return 0;

    case EM_SETOLECALLBACK:
            TRACE_EDIT_MSG32("EM_SETOLECALLBACK Ignored");
            return 0;

    case EM_SETOPTIONS:
            TRACE_EDIT_MSG32("EM_SETOPTIONS Ignored");
            return 0;

    case EM_SETPALETTE:
            TRACE_EDIT_MSG32("EM_SETPALETTE Ignored");
            return 0;

    case EM_SETPARAFORMAT:
            TRACE_EDIT_MSG32("EM_SETPARAFORMAT Ignored");
            return 0;

    case EM_SETPUNCTUATION:
            TRACE_EDIT_MSG32("EM_SETPUNCTUATION Ignored");
            return 0;

    case EM_SETSCROLLPOS:
            TRACE_EDIT_MSG32("EM_SETSCROLLPOS Ignored");
            return 0;

    case EM_SETTARGETDEVICE:
            TRACE_EDIT_MSG32("EM_SETTARGETDEVICE Ignored");
            return 0;

    case EM_SETTEXTEX:
            TRACE_EDIT_MSG32("EM_SETTEXTEX Ignored");
            return 0;

    case EM_SETTEXTMODE:
            TRACE_EDIT_MSG32("EM_SETTEXTMODE Ignored");
            return 0;

    case EM_SETTYPOGRAPHYOPTIONS:
            TRACE_EDIT_MSG32("EM_SETTYPOGRAPHYOPTIONS Ignored");
            return 0;

    case EM_SETUNDOLIMIT:
            TRACE_EDIT_MSG32("EM_SETUNDOLIMIT Ignored");
            return 0;

    case EM_SETWORDBREAKPROCEX:
            TRACE_EDIT_MSG32("EM_SETWORDBREAKPROCEX Ignored");
            return 0;

    case EM_SETWORDWRAPMODE:
            TRACE_EDIT_MSG32("EM_SETWORDWRAPMODE Ignored");
            return 0;

    case EM_SETZOOM:
            TRACE_EDIT_MSG32("EM_SETZOOM Ignored");
            return 0;

    case EM_SHOWSCROLLBAR:
            TRACE_EDIT_MSG32("EM_SHOWSCROLLBAR Ignored");
            return 0;

    case EM_STOPGROUPTYPING:
            TRACE_EDIT_MSG32("EM_STOPGROUPTYPING Ignored");
            return 0;

    case EM_STREAMOUT:
            TRACE_EDIT_MSG32("EM_STREAMOUT Ignored");
            return 0;

/* Messages dispatched to the edit control */
     case EM_CANUNDO:
            TRACE_EDIT_MSG32("EM_CANUNDO Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_CHARFROMPOS:
            TRACE_EDIT_MSG32("EM_CHARFROMPOS Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_EMPTYUNDOBUFFER:
            TRACE_EDIT_MSG32("EM_EMPTYUNDOBUFFER Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_FMTLINES:
            TRACE_EDIT_MSG32("EM_FMTLINES Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_GETFIRSTVISIBLELINE:
            TRACE_EDIT_MSG32("EM_GETFIRSTVISIBLELINE Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_GETHANDLE:
            TRACE_EDIT_MSG32("EM_GETHANDLE Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
 /*    case EM_GETIMESTATUS:*/
     case EM_GETLIMITTEXT:
            TRACE_EDIT_MSG32("EM_GETLIMITTEXT Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_GETLINE:
            TRACE_EDIT_MSG32("EM_GETLINE Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_GETLINECOUNT:
            TRACE_EDIT_MSG32("EM_GETLINECOUNT Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_GETMARGINS:
            TRACE_EDIT_MSG32("EM_GETMARGINS Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_GETMODIFY:
            TRACE_EDIT_MSG32("EM_GETMODIFY Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_GETPASSWORDCHAR:
            TRACE_EDIT_MSG32("EM_GETPASSWORDCHAR Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_GETRECT:
            TRACE_EDIT_MSG32("EM_GETRECT Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_GETSEL:
            TRACE_EDIT_MSG32("EM_GETSEL Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_GETTHUMB:
            TRACE_EDIT_MSG32("EM_GETTHUMB Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_GETWORDBREAKPROC:
            TRACE_EDIT_MSG32("EM_GETWORDBREAKPROC Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_LINEFROMCHAR:
            TRACE_EDIT_MSG32("EM_LINEFROMCHAR Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_LINEINDEX:
            TRACE_EDIT_MSG32("EM_LINEINDEX Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_LINELENGTH:
            TRACE_EDIT_MSG32("EM_LINELENGTH Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_LINESCROLL:
            TRACE_EDIT_MSG32("EM_LINESCROLL Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_POSFROMCHAR:
            TRACE_EDIT_MSG32("EM_POSFROMCHAR Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_REPLACESEL:
            TRACE_EDIT_MSG32("case EM_REPLACESEL Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_SCROLL:
            TRACE_EDIT_MSG32("case EM_SCROLL Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_SCROLLCARET:
            TRACE_EDIT_MSG32("EM_SCROLLCARET Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_SETHANDLE:
            TRACE_EDIT_MSG32("EM_SETHANDLE Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
 /*    case EM_SETIMESTATUS:*/
     case EM_SETLIMITTEXT:
            TRACE_EDIT_MSG32("EM_SETLIMITTEXT Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_SETMARGINS:
            TRACE_EDIT_MSG32("case EM_SETMARGINS Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_SETMODIFY:
            TRACE_EDIT_MSG32("EM_SETMODIFY Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_SETPASSWORDCHAR:
            TRACE_EDIT_MSG32("EM_SETPASSWORDCHAR Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_SETREADONLY:
            TRACE_EDIT_MSG32("EM_SETREADONLY Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_SETRECT:
            TRACE_EDIT_MSG32("EM_SETRECT Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_SETRECTNP:
            TRACE_EDIT_MSG32("EM_SETRECTNP Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_SETSEL:
            TRACE_EDIT_MSG32("EM_SETSEL Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_SETTABSTOPS:
            TRACE_EDIT_MSG32("EM_SETTABSTOPS Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_SETWORDBREAKPROC:
            TRACE_EDIT_MSG32("EM_SETWORDBREAKPROC Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case EM_UNDO:
            TRACE_EDIT_MSG32("EM_UNDO Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);

     case WM_STYLECHANGING:
            TRACE_EDIT_MSG32("WM_STYLECHANGING Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case WM_STYLECHANGED:
            TRACE_EDIT_MSG32("WM_STYLECHANGED Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case WM_GETTEXT:
            TRACE_EDIT_MSG32("WM_GETTEXT Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case WM_GETTEXTLENGTH:
            TRACE_EDIT_MSG32("WM_GETTEXTLENGTH Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case WM_SETTEXT:
            {
                LRESULT rc;
                LPWSTR text = NULL;
                TRACE_EDIT_MSG32("WM_SETTEXT Passed to edit control");
                text = RE20_DoctorText(lParam);
                rc = CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam,
                                     (LPARAM) text);
                HeapFree(GetProcessHeap(),0,text);
                return rc;
            }
     case WM_CUT:
            TRACE_EDIT_MSG32("WM_CUT Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
     case WM_COPY:
            TRACE_EDIT_MSG32("WM_COPY Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_PASTE:
            TRACE_EDIT_MSG32("WM_PASTE Passed to edit control");
            return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);

    /* Messages passed to default handler. */
    case WM_NCCALCSIZE:
        TRACE_EDIT_MSG32("WM_NCCALCSIZE Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_NCPAINT:
        TRACE_EDIT_MSG32("WM_NCPAINT Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_PAINT:
        TRACE_EDIT_MSG32("WM_PAINT Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_ERASEBKGND:
        TRACE_EDIT_MSG32("WM_ERASEBKGND Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_KILLFOCUS:
        TRACE_EDIT_MSG32("WM_KILLFOCUS Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_DESTROY:
        TRACE_EDIT_MSG32("WM_DESTROY Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_CHILDACTIVATE:
	TRACE_EDIT_MSG32("WM_CHILDACTIVATE Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);

    case WM_WINDOWPOSCHANGING:
        TRACE_EDIT_MSG32("WM_WINDOWPOSCHANGING Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_WINDOWPOSCHANGED:
        TRACE_EDIT_MSG32("WM_WINDOWPOSCHANGED Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
/*    case WM_INITIALUPDATE:
        TRACE_EDIT_MSG32("WM_INITIALUPDATE Passed to default");
        return DefWindowProcW( hwnd,uMsg,wParam,lParam); */
    case WM_CTLCOLOREDIT:
        TRACE_EDIT_MSG32("WM_CTLCOLOREDIT Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_SETCURSOR:
        TRACE_EDIT_MSG32("WM_SETCURSOR Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_MOVE:
        TRACE_EDIT_MSG32("WM_MOVE Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_SHOWWINDOW:
        TRACE_EDIT_MSG32("WM_SHOWWINDOW Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_PARENTNOTIFY:
        TRACE_EDIT_MSG32("WM_PARENTNOTIFY Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_SETREDRAW:
        TRACE_EDIT_MSG32("WM_SETREDRAW Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_NCDESTROY:
    {
        LRESULT rc;
        TRACE_EDIT_MSG32("WM_NCDESTROY Passed to default");
        rc = CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
        HeapFree( GetProcessHeap(), 0, info );
        return rc;
    }

    case WM_NCHITTEST:
        TRACE_EDIT_MSG32("WM_NCHITTEST Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_CTLCOLORSTATIC:
        TRACE_EDIT_MSG32("WM_CTLCOLORSTATIC Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_NCMOUSEMOVE:
        TRACE_EDIT_MSG32("WM_NCMOUSEMOVE Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_CLEAR:
        TRACE_EDIT_MSG32("WM_CLEAR Passed to default");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
   /*
    * used by IE in the EULA box
    */
    case WM_ALTTABACTIVE:
        TRACE_EDIT_MSG32("WM_ALTTABACTIVE");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_GETDLGCODE:
        TRACE_EDIT_MSG32("WM_GETDLGCODE");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
    case WM_SETFONT:
        TRACE_EDIT_MSG32("WM_SETFONT");
        return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);

    }

    if ((uMsg >= WM_USER) && (uMsg < WM_APP)) {
	FIXME("Unknown message 0x%x Passed to default hwnd=%p, wParam=%08x, lParam=%08x\n",
	       uMsg, hwnd, (UINT)wParam, (UINT)lParam);
    }

   return CallWindowProcW(info->lpfnEditWndProc, hwnd, uMsg, wParam, lParam);
}


/*
 *
 * DESCRIPTION:
 * Ansi Window procedure of the RichEdit control.
 *
 */
static LRESULT WINAPI RICHED20_WindowProcA(HWND hwnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam)
{
    return RICHED20_WindowProc(hwnd, uMsg, wParam, lParam, FALSE);
}


/*
 *
 * DESCRIPTION:
 * Unicode Window procedure of the RichEdit control.
 *
 */
static LRESULT WINAPI RICHED20_WindowProcW(HWND hwnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam)
{
    return RICHED20_WindowProc(hwnd, uMsg, wParam, lParam, TRUE);
}


/***********************************************************************
 * DllGetVersion [RICHED20.2]
 *
 * Retrieves version information of the 'RICHED20.DLL'
 *
 * PARAMS
 *     pdvi [O] pointer to version information structure.
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: E_INVALIDARG
 *
 * NOTES
 *     Returns version of a comctl32.dll from IE4.01 SP1.
 */

HRESULT WINAPI
RICHED20_DllGetVersion (DLLVERSIONINFO *pdvi)
{
    TRACE("\n");

    if (pdvi->cbSize != sizeof(DLLVERSIONINFO)) {

	return E_INVALIDARG;
    }

    pdvi->dwMajorVersion = 4;
    pdvi->dwMinorVersion = 0;
    pdvi->dwBuildNumber = 0;
    pdvi->dwPlatformID = 0;

    return S_OK;
}

/***
 * DESCRIPTION:
 * Registers the window class.
 *
 * PARAMETER(S):
 * None
 *
 * RETURN:
 * None
 */
VOID RICHED20_Register(void)
{
    WNDCLASSW wndClassW;
    WNDCLASSA wndClassA;

    TRACE("\n");

    ZeroMemory(&wndClassW, sizeof(WNDCLASSW));
    wndClassW.style = CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
    wndClassW.lpfnWndProc = (WNDPROC)RICHED20_WindowProcW;
    wndClassW.cbClsExtra = 0;
    wndClassW.cbWndExtra = RTFInfoOffsetA + sizeof(RTFControl_Info*);
    wndClassW.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    wndClassW.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClassW.lpszClassName = RICHEDIT_CLASS20W;

    RegisterClassW (&wndClassW);

    ZeroMemory(&wndClassA, sizeof(WNDCLASSA));
    wndClassA.style = CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
    wndClassA.lpfnWndProc = (WNDPROC)RICHED20_WindowProcA;
    wndClassA.cbClsExtra = 0;
    wndClassA.cbWndExtra = RTFInfoOffsetA + sizeof(RTFControl_Info*);
    wndClassA.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    wndClassA.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClassA.lpszClassName = RICHEDIT_CLASS20A;

    RegisterClassA (&wndClassA);

}

/***
 * DESCRIPTION:
 * Unregisters the window class.
 *
 * PARAMETER(S):
 * None
 *
 * RETURN:
 * None
 */
VOID RICHED20_Unregister(void)
{
    TRACE("\n");

    UnregisterClassW(RICHEDIT_CLASS20W, NULL);
    UnregisterClassA(RICHEDIT_CLASS20A, NULL);
}


/***
 * DESCRIPTION:
 * Initialize edit control class info
 */
VOID RICHEDIT_InitEditControlInfo(void)
{
    WCHAR edit[] = {'e','d','i','t',0};
    WNDCLASSW wcEditW;
    WNDCLASSA wcEditA;

    if (GetClassInfoW(0, edit,  &wcEditW))
    {
        lpfnEditWndProcW = wcEditW.lpfnWndProc;
        RTFInfoOffsetW = wcEditW.cbWndExtra;
    }
    else
        ERR("Failed to retrieve edit control class info\n");

    if (GetClassInfoA(0, "edit",  &wcEditA))
    {
        lpfnEditWndProcA = wcEditA.lpfnWndProc;
        RTFInfoOffsetA = wcEditA.cbWndExtra;
    }
    else
        ERR("Failed to retrieve edit control class info\n");
}


INT RICHEDIT_GetTextRange(HWND hwnd,TEXTRANGEW *tr)
{
    UINT alloc_size, text_size, range_size;
    char *text;

    TRACE("start: 0x%x stop: 0x%x\n",(INT)tr->chrg.cpMin,(INT)tr->chrg.cpMax);

    if (!(alloc_size = SendMessageW(hwnd,WM_GETTEXTLENGTH,0,0))) return FALSE;
    if (!(text = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
                           sizeof(WCHAR)*(alloc_size+1))))
		return FALSE;
    text_size = SendMessageW(hwnd,WM_GETTEXT,alloc_size,(INT)text);

    if (text_size > tr->chrg.cpMin)
    {
       range_size = (text_size> tr->chrg.cpMax) ? (tr->chrg.cpMax - tr->chrg.cpMin) : (text_size - tr->chrg.cpMin);
       TRACE("EditText: %.30s ...\n",text+tr->chrg.cpMin);
       memcpy(tr->lpstrText,text+tr->chrg.cpMin,range_size*sizeof(WCHAR));
    }
    else range_size = 0;
    HeapFree(GetProcessHeap(), 0, text);

    return range_size;
}

INT RICHEDIT_GetSelText(HWND hwnd,LPWSTR lpstrBuffer)
{
    TEXTRANGEW textrange;

    textrange.lpstrText = lpstrBuffer;
    SendMessageW(hwnd,EM_GETSEL,(INT)&textrange.chrg.cpMin,(INT)&textrange.chrg.cpMax);
    return RICHEDIT_GetTextRange(hwnd,&textrange);
}
