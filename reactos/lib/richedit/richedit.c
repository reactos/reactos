/*
 * RichEdit32  functions
 *
 * This module is a simple wrapper for the edit controls.
 * At the point, it is good only for application who use the RICHEDIT
 * control to display RTF text.
 *
 * Copyright 2000 by Jean-Claude Batista
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
#include "riched32.h"
#include "richedit.h"
#include "charlist.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"

#include "rtf.h"
#include "rtf2text.h"
#include "wine/debug.h"

#define ID_EDIT      1

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

HANDLE RICHED32_hHeap = NULL;
/* LPSTR  RICHED32_aSubclass = NULL; */

#define TRACE_EDIT_MSG32(str) \
        TRACE(\
                     "32 bit : " str ": hwnd=%p, wParam=%08x, lParam=%08x\n"\
                     , \
                     hwnd, (UINT)wParam, (UINT)lParam)

LPVOID* WINAPI CreateIRichEditOle();

/***********************************************************************
 * DllMain [Internal] Initializes the internal 'RICHED32.DLL'.
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
        RICHED32_hHeap = HeapCreate (0, 0x10000, 0);
        /* register the Rich Edit class */
        RICHED32_Register ();
        break;

    case DLL_PROCESS_DETACH:
        /* unregister all common control classes */
        RICHED32_Unregister ();
        HeapDestroy (RICHED32_hHeap);
        RICHED32_hHeap = NULL;
        break;
    }
    return TRUE;
}

/* Support routines for window procedure */
   INT RICHEDIT_GetTextRange(HWND hwnd,TEXTRANGEA *tr);
   INT RICHEDIT_GetSelText(HWND hwnd,LPSTR lpstrBuffer);


const WCHAR RichEditInfoStr[] = { '_','R','T','F','_','I','n','f','o', 0 };

typedef struct _RTFControl_info
{
    HWND hwndEdit;
    HWND hwndParent;
    char* rtfBuffer;
    RTF_Info *parser;
} RTFControl_Info;

/*
 *
 * DESCRIPTION:
 * Window procedure of the RichEdit control.
 *
 */
static LRESULT WINAPI RICHED32_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam)
{
    int RTFToBuffer(RTF_Info *parser, char* pBuffer, int nBufferSize);
    LONG newstyle = 0;
    LONG style = 0;
    RTFControl_Info *info;
    int rtfBufferSize;
    CHARRANGE *cr;

    info = GetPropW( hwnd, RichEditInfoStr );
    TRACE("uMsg: 0x%x hwnd: %p\n",uMsg,hwnd);

    switch (uMsg)
    {

    case WM_CREATE:
            TRACE_EDIT_MSG32("WM_CREATE Passed to default");
            DefWindowProcA( hwnd,uMsg,wParam,lParam);
            return 0 ;
        
    case WM_NCCREATE :
	    TRACE_EDIT_MSG32("WM_NCCREATE");

            info = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                              sizeof (RTFControl_Info));
            info->parser = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                              sizeof (RTF_Info));
            SetPropW(hwnd, RichEditInfoStr, (HANDLE)info);

	    /* remove SCROLLBARS from the current window style */
	    info->hwndParent = ((LPCREATESTRUCTA) lParam)->hwndParent;

	    newstyle = style = ((LPCREATESTRUCTA) lParam)->style;
            newstyle &= ~WS_HSCROLL;
            newstyle &= ~WS_VSCROLL;
            newstyle &= ~ES_AUTOHSCROLL;
            newstyle &= ~ES_AUTOVSCROLL;
	    SetWindowLongA(hwnd,GWL_STYLE, newstyle);

            TRACE("previous hwndEdit: %p\n",info->hwndEdit);
            info->hwndEdit = CreateWindowA ("edit", ((LPCREATESTRUCTA) lParam)->lpszName,
                                   style, 0, 0, 0, 0,
                                   hwnd, (HMENU) ID_EDIT,
                                   ((LPCREATESTRUCTA) lParam)->hInstance, NULL) ;
            TRACE("hwndEdit: %p hwnd: %p\n",info->hwndEdit,hwnd);

            if (info->hwndEdit)
                return TRUE ;
            else
                return FALSE ;

    case WM_SETFOCUS :
	    TRACE_EDIT_MSG32("WM_SETFOCUS");
            SetFocus (info->hwndEdit) ;
            return 0 ;

    case WM_SIZE :
            TRACE_EDIT_MSG32("WM_SIZE");
            MoveWindow (info->hwndEdit, 0, 0, LOWORD (lParam), HIWORD (lParam), TRUE) ;
            return 0 ;

    case WM_COMMAND :
        TRACE_EDIT_MSG32("WM_COMMAND");
	switch(HIWORD(wParam)) {
		case EN_CHANGE:
		case EN_HSCROLL:
		case EN_KILLFOCUS:
		case EN_SETFOCUS:
		case EN_UPDATE:
		case EN_VSCROLL:
			return SendMessageA(info->hwndParent, WM_COMMAND,
				wParam, (LPARAM)(hwnd));

		case EN_ERRSPACE:
		case EN_MAXTEXT:
			MessageBoxA (hwnd, "RichEdit control out of space.",
                                  "ERROR", MB_OK | MB_ICONSTOP) ;
			return 0 ;
		}

    case EM_STREAMIN:
            TRACE_EDIT_MSG32("EM_STREAMIN");

	    /* setup the RTF parser */
	    RTFSetEditStream(info->parser,( EDITSTREAM*)lParam);
	    info->parser->rtfFormat = wParam&(SF_TEXT|SF_RTF);
	    WriterInit(info->parser);
	    RTFInit (info->parser);
	    BeginFile(info->parser);

	    /* do the parsing */
	    RTFRead (info->parser);

	    rtfBufferSize = RTFToBuffer(info->parser,NULL, 0);
	    info->rtfBuffer = HeapAlloc(RICHED32_hHeap, 0,rtfBufferSize*sizeof(char));
	    if(info->rtfBuffer)
	    {
	    	RTFToBuffer(info->parser,info->rtfBuffer, rtfBufferSize);
            	SetWindowTextA(info->hwndEdit,info->rtfBuffer);
	    	HeapFree(RICHED32_hHeap, 0,info->rtfBuffer);
	    }
	    else
		WARN("Not enough memory for a allocating rtfBuffer\n");

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
            if (info->hwndEdit) SendMessageA( info->hwndEdit, EM_GETSEL, (INT)&cr->cpMin, (INT)&cr->cpMax);
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
           return SendMessageA(info->hwndEdit,EM_SETLIMITTEXT,limit,0);
        }

    case EM_EXLINEFROMCHAR:
            TRACE_EDIT_MSG32("EM_EXLINEFROMCHAR -> LINEFROMCHAR");
            if (info->hwndEdit) return SendMessageA( info->hwndEdit, EM_LINEFROMCHAR, lParam, wParam);
            return 0;

    case EM_EXSETSEL:
            TRACE_EDIT_MSG32("EM_EXSETSEL -> EM_SETSEL");
            cr = (VOID *) lParam;
            if (info->hwndEdit) SendMessageA( info->hwndEdit, EM_SETSEL, cr->cpMin, cr->cpMax);
            return 0;

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

    case EM_GETOLEINTERFACE:
            TRACE_EDIT_MSG32("EM_GETOLEINTERFACE Ignored");
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
            return RICHEDIT_GetSelText(info->hwndEdit,(void *)lParam);

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
            return RICHEDIT_GetTextRange(info->hwndEdit,(TEXTRANGEA *)lParam);

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
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_CHARFROMPOS:
            TRACE_EDIT_MSG32("EM_CHARFROMPOS Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_EMPTYUNDOBUFFER:
            TRACE_EDIT_MSG32("EM_EMPTYUNDOBUFFER Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_FMTLINES:
            TRACE_EDIT_MSG32("EM_FMTLINES Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_GETFIRSTVISIBLELINE:
            TRACE_EDIT_MSG32("EM_GETFIRSTVISIBLELINE Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_GETHANDLE:
            TRACE_EDIT_MSG32("EM_GETHANDLE Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
 /*    case EM_GETIMESTATUS:*/
     case EM_GETLIMITTEXT:
            TRACE_EDIT_MSG32("EM_GETLIMITTEXT Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_GETLINE:
            TRACE_EDIT_MSG32("EM_GETLINE Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_GETLINECOUNT:
            TRACE_EDIT_MSG32("EM_GETLINECOUNT Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_GETMARGINS:
            TRACE_EDIT_MSG32("EM_GETMARGINS Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_GETMODIFY:
            TRACE_EDIT_MSG32("EM_GETMODIFY Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_GETPASSWORDCHAR:
            TRACE_EDIT_MSG32("EM_GETPASSWORDCHAR Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_GETRECT:
            TRACE_EDIT_MSG32("EM_GETRECT Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_GETSEL:
            TRACE_EDIT_MSG32("EM_GETSEL Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_GETTHUMB:
            TRACE_EDIT_MSG32("EM_GETTHUMB Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_GETWORDBREAKPROC:
            TRACE_EDIT_MSG32("EM_GETWORDBREAKPROC Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_LINEFROMCHAR:
            TRACE_EDIT_MSG32("EM_LINEFROMCHAR Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_LINEINDEX:
            TRACE_EDIT_MSG32("EM_LINEINDEX Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_LINELENGTH:
            TRACE_EDIT_MSG32("EM_LINELENGTH Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_LINESCROLL:
            TRACE_EDIT_MSG32("EM_LINESCROLL Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_POSFROMCHAR:
            TRACE_EDIT_MSG32("EM_POSFROMCHAR Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_REPLACESEL:
            TRACE_EDIT_MSG32("case EM_REPLACESEL Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_SCROLL:
            TRACE_EDIT_MSG32("case EM_SCROLL Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_SCROLLCARET:
            TRACE_EDIT_MSG32("EM_SCROLLCARET Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_SETHANDLE:
            TRACE_EDIT_MSG32("EM_SETHANDLE Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
 /*    case EM_SETIMESTATUS:*/
     case EM_SETLIMITTEXT:
            TRACE_EDIT_MSG32("EM_SETLIMITTEXT Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_SETMARGINS:
            TRACE_EDIT_MSG32("case EM_SETMARGINS Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_SETMODIFY:
            TRACE_EDIT_MSG32("EM_SETMODIFY Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_SETPASSWORDCHAR:
            TRACE_EDIT_MSG32("EM_SETPASSWORDCHAR Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_SETREADONLY:
            TRACE_EDIT_MSG32("EM_SETREADONLY Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_SETRECT:
            TRACE_EDIT_MSG32("EM_SETRECT Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_SETRECTNP:
            TRACE_EDIT_MSG32("EM_SETRECTNP Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_SETSEL:
            TRACE_EDIT_MSG32("EM_SETSEL Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_SETTABSTOPS:
            TRACE_EDIT_MSG32("EM_SETTABSTOPS Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_SETWORDBREAKPROC:
            TRACE_EDIT_MSG32("EM_SETWORDBREAKPROC Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case EM_UNDO:
            TRACE_EDIT_MSG32("EM_UNDO Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);

     case WM_STYLECHANGING:
            TRACE_EDIT_MSG32("WM_STYLECHANGING Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case WM_STYLECHANGED:
            TRACE_EDIT_MSG32("WM_STYLECHANGED Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case WM_GETTEXT:
            TRACE_EDIT_MSG32("WM_GETTEXT Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case WM_GETTEXTLENGTH:
            TRACE_EDIT_MSG32("WM_GETTEXTLENGTH Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case WM_SETTEXT:
            TRACE_EDIT_MSG32("WM_SETTEXT Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case WM_CUT:
            TRACE_EDIT_MSG32("WM_CUT Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
     case WM_COPY:
            TRACE_EDIT_MSG32("WM_COPY Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);
    case WM_PASTE:
            TRACE_EDIT_MSG32("WM_PASTE Passed to edit control");
	    return SendMessageA( info->hwndEdit, uMsg, wParam, lParam);

    /* Messages passed to default handler. */
    case WM_NCCALCSIZE:
        /* A fundamental problem with embedding an edit control within another
           window to emulate the richedit control, is that normally, the 
           WM_NCCALCSIZE message window would return the client area of the 
           edit control.
           
           While we could send a message to the edit control here to get that size
           and return that value, this causes problems with the WM_SIZE message.
           That is because the WM_SIZE message uses the returned value of
           WM_NCCALCSIZE (via X11DRV_SetWindowSize) to determine the size to make 
           the edit control. If we return the size of the edit control client area
           here, the result is the symptom of the edit control being inset on the 
           right and bottom by the width of any existing scrollbars.
           
           The easy fix is to have WM_NCCALCSIZE return the true size of this 
           enclosing window, which is what we have done here. The more difficult 
           fix is to create a custom Richedit MoveWindow procedure for use in the 
           WM_SIZE message above. Since it is very unlikely that an app would call
           and use the WM_NCCALCSIZE message, we stick with the easy fix for now.
         */
        TRACE_EDIT_MSG32("WM_NCCALCSIZE Passed to default");
	return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_NCPAINT:
        TRACE_EDIT_MSG32("WM_NCPAINT Passed to default");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_PAINT:
        TRACE_EDIT_MSG32("WM_PAINT Passed to default");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_ERASEBKGND:
        TRACE_EDIT_MSG32("WM_ERASEBKGND Passed to default");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_KILLFOCUS:
        TRACE_EDIT_MSG32("WM_KILLFOCUS Passed to default");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_DESTROY:
        TRACE_EDIT_MSG32("WM_DESTROY Passed to default");
	HeapFree( GetProcessHeap(), 0, info->parser );
	HeapFree( GetProcessHeap(), 0, info );
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_CHILDACTIVATE:
	TRACE_EDIT_MSG32("WM_CHILDACTIVATE Passed to default");
	return DefWindowProcA( hwnd,uMsg,wParam,lParam);

    case WM_WINDOWPOSCHANGING:
        TRACE_EDIT_MSG32("WM_WINDOWPOSCHANGING Passed to default");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_WINDOWPOSCHANGED:
        TRACE_EDIT_MSG32("WM_WINDOWPOSCHANGED Passed to default");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
/*    case WM_INITIALUPDATE:
        TRACE_EDIT_MSG32("WM_INITIALUPDATE Passed to default");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam); */
    case WM_CTLCOLOREDIT:
        TRACE_EDIT_MSG32("WM_CTLCOLOREDIT Passed to default");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_SETCURSOR:
        TRACE_EDIT_MSG32("WM_SETCURSOR Passed to default");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_MOVE:
        TRACE_EDIT_MSG32("WM_MOVE Passed to default");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_SHOWWINDOW:
        TRACE_EDIT_MSG32("WM_SHOWWINDOW Passed to default");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_PARENTNOTIFY:
        TRACE_EDIT_MSG32("WM_PARENTNOTIFY Passed to default");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_SETREDRAW:
        TRACE_EDIT_MSG32("WM_SETREDRAW Passed to default");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_NCDESTROY:
        TRACE_EDIT_MSG32("WM_NCDESTROY Passed to default");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_NCHITTEST:
        TRACE_EDIT_MSG32("WM_NCHITTEST Passed to default");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_CTLCOLORSTATIC:
        TRACE_EDIT_MSG32("WM_CTLCOLORSTATIC Passed to default");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_NCMOUSEMOVE:
        TRACE_EDIT_MSG32("WM_NCMOUSEMOVE Passed to default");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_CLEAR:
        TRACE_EDIT_MSG32("WM_CLEAR Passed to default");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
   /*
    * used by IE in the EULA box
    */
    case WM_ALTTABACTIVE:
        TRACE_EDIT_MSG32("WM_ALTTABACTIVE");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_GETDLGCODE:
        TRACE_EDIT_MSG32("WM_GETDLGCODE");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);
    case WM_SETFONT:
        TRACE_EDIT_MSG32("WM_SETFONT");
        return DefWindowProcA( hwnd,uMsg,wParam,lParam);

    }

    if ((uMsg >= WM_USER) && (uMsg < WM_APP)) {
	FIXME("Unknown message 0x%x Passed to default hwnd=%p, wParam=%08x, lParam=%08x\n",
	       uMsg, hwnd, (UINT)wParam, (UINT)lParam);
    }

   return DefWindowProcA( hwnd,uMsg,wParam,lParam);
}

/***********************************************************************
 * DllGetVersion [RICHED32.2]
 *
 * Retrieves version information of the 'RICHED32.DLL'
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
RICHED32_DllGetVersion (DLLVERSIONINFO *pdvi)
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
VOID RICHED32_Register(void)
{
    WNDCLASSA wndClass;

    TRACE("\n");

    ZeroMemory(&wndClass, sizeof(WNDCLASSA));
    wndClass.style = CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
    wndClass.lpfnWndProc = (WNDPROC)RICHED32_WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0; /*(sizeof(RICHED32_INFO *);*/
    wndClass.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = RICHEDIT_CLASS10A; /* WC_RICHED32A; */

    RegisterClassA (&wndClass);
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
VOID RICHED32_Unregister(void)
{
    TRACE("\n");

    UnregisterClassA(RICHEDIT_CLASS10A, NULL);
}

INT RICHEDIT_GetTextRange(HWND hwnd,TEXTRANGEA *tr)
{
    UINT alloc_size, text_size, range_size;
    char *text;

    TRACE("start: 0x%x stop: 0x%x\n",(INT)tr->chrg.cpMin,(INT)tr->chrg.cpMax);

    if (!(alloc_size = SendMessageA(hwnd,WM_GETTEXTLENGTH,0,0))) return FALSE;
    if (!(text = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (alloc_size+1))))
		return FALSE;
    text_size = SendMessageA(hwnd,WM_GETTEXT,alloc_size,(INT)text);

    if (text_size > tr->chrg.cpMin)
    {
       range_size = (text_size> tr->chrg.cpMax) ? (tr->chrg.cpMax - tr->chrg.cpMin) : (text_size - tr->chrg.cpMin);
       TRACE("EditText: %.30s ...\n",text+tr->chrg.cpMin);
       memcpy(tr->lpstrText,text+tr->chrg.cpMin,range_size);
    }
    else range_size = 0;
    HeapFree(GetProcessHeap(), 0, text);

    return range_size;
}

INT RICHEDIT_GetSelText(HWND hwnd,LPSTR lpstrBuffer)
{
    TEXTRANGEA textrange;

    textrange.lpstrText = lpstrBuffer;
    SendMessageA(hwnd,EM_GETSEL,(INT)&textrange.chrg.cpMin,(INT)&textrange.chrg.cpMax);
    return RICHEDIT_GetTextRange(hwnd,&textrange);
}
