/*
 * Common Dialog Boxes interface (16 bit implementation)
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 * Copyright 1998 Bertho A. Stultiens
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

#ifndef _WINE_DLL_CDLG16_H
#define _WINE_DLL_CDLG16_H

#include "dlgs.h"
#include "wine/windef16.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wownt32.h"

/* 16 bit api */

#include "pshpack1.h"

typedef UINT16 (CALLBACK *LPOFNHOOKPROC16)(HWND16,UINT16,WPARAM16,LPARAM);

typedef struct {
	DWORD		lStructSize;
	HWND16		hwndOwner;
	HINSTANCE16	hInstance;
	SEGPTR	        lpstrFilter;
	SEGPTR          lpstrCustomFilter;
	DWORD		nMaxCustFilter;
	DWORD		nFilterIndex;
	SEGPTR          lpstrFile;
	DWORD		nMaxFile;
	SEGPTR		lpstrFileTitle;
	DWORD		nMaxFileTitle;
	SEGPTR 		lpstrInitialDir;
	SEGPTR 		lpstrTitle;
	DWORD		Flags;
	UINT16		nFileOffset;
	UINT16		nFileExtension;
	SEGPTR		lpstrDefExt;
	LPARAM 		lCustData;
	LPOFNHOOKPROC16 lpfnHook;
	SEGPTR 		lpTemplateName;
}   OPENFILENAME16,*LPOPENFILENAME16;

typedef UINT16 (CALLBACK *LPCCHOOKPROC16) (HWND16, UINT16, WPARAM16, LPARAM);

typedef struct {
	DWORD		lStructSize;
	HWND16		hwndOwner;
	HWND16		hInstance;
	COLORREF	rgbResult;
	SEGPTR          lpCustColors;
	DWORD 		Flags;
	LPARAM		lCustData;
        LPCCHOOKPROC16  lpfnHook;
	SEGPTR 		lpTemplateName;
} CHOOSECOLOR16;
typedef CHOOSECOLOR16 *LPCHOOSECOLOR16;

typedef UINT16 (CALLBACK *LPFRHOOKPROC16)(HWND16,UINT16,WPARAM16,LPARAM);
typedef struct {
	DWORD		lStructSize; 		/* size of this struct 0x20 */
	HWND16		hwndOwner; 		/* handle to owner's window */
	HINSTANCE16	hInstance; 		/* instance handle of.EXE that  */
						/* contains cust. dlg. template */
	DWORD		Flags;                  /* one or more of the FR_?? */
	SEGPTR		lpstrFindWhat;          /* ptr. to search string    */
	SEGPTR		lpstrReplaceWith;       /* ptr. to replace string   */
	UINT16		wFindWhatLen;           /* size of find buffer      */
	UINT16 		wReplaceWithLen;        /* size of replace buffer   */
	LPARAM 		lCustData;              /* data passed to hook fn.  */
        LPFRHOOKPROC16  lpfnHook;
	SEGPTR 		lpTemplateName;         /* custom template name     */
} FINDREPLACE16, *LPFINDREPLACE16;

typedef UINT16 (CALLBACK *LPCFHOOKPROC16)(HWND16,UINT16,WPARAM16,LPARAM);
typedef struct
{
	DWORD			lStructSize;
	HWND16			hwndOwner;          /* caller's window handle   */
	HDC16          	        hDC;                /* printer DC/IC or NULL    */
	SEGPTR                  lpLogFont;          /* ptr. to a LOGFONT struct */
	short			iPointSize;         /* 10 * size in points of selected font */
	DWORD			Flags;              /* enum. type flags         */
	COLORREF		rgbColors;          /* returned text color      */
	LPARAM	                lCustData;          /* data passed to hook fn.  */
	LPCFHOOKPROC16          lpfnHook;
	SEGPTR			lpTemplateName;     /* custom template name     */
	HINSTANCE16		hInstance;          /* instance handle of.EXE that   */
					/* contains cust. dlg. template  */
	SEGPTR			lpszStyle;          /* return the style field here   */
					/* must be LF_FACESIZE or bigger */
	UINT16			nFontType;          /* same value reported to the    */
						    /* EnumFonts callback with the   */
						    /* extra FONTTYPE_ bits added    */
	short			nSizeMin;           /* minimum pt size allowed & */
	short			nSizeMax;           /* max pt size allowed if    */
					/* CF_LIMITSIZE is used      */
} CHOOSEFONT16, *LPCHOOSEFONT16;


typedef UINT16 (CALLBACK *LPPRINTHOOKPROC16) (HWND16, UINT16, WPARAM16, LPARAM);
typedef UINT16 (CALLBACK *LPSETUPHOOKPROC16) (HWND16, UINT16, WPARAM16, LPARAM);
typedef struct
{
    DWORD            lStructSize;
    HWND16           hwndOwner;
    HGLOBAL16        hDevMode;
    HGLOBAL16        hDevNames;
    HDC16            hDC;
    DWORD            Flags;
    WORD             nFromPage;
    WORD             nToPage;
    WORD             nMinPage;
    WORD             nMaxPage;
    WORD             nCopies;
    HINSTANCE16      hInstance;
    LPARAM           lCustData;
    LPPRINTHOOKPROC16 lpfnPrintHook;
    LPSETUPHOOKPROC16 lpfnSetupHook;
    SEGPTR           lpPrintTemplateName;
    SEGPTR           lpSetupTemplateName;
    HGLOBAL16        hPrintTemplate;
    HGLOBAL16        hSetupTemplate;
} PRINTDLG16, *LPPRINTDLG16;

BOOL16  WINAPI ChooseColor16(LPCHOOSECOLOR16 lpChCol);
HWND16  WINAPI FindText16( SEGPTR find);
BOOL16  WINAPI GetOpenFileName16(SEGPTR ofn);
BOOL16  WINAPI GetSaveFileName16(SEGPTR ofn);
BOOL16  WINAPI PrintDlg16( LPPRINTDLG16 print);
HWND16  WINAPI ReplaceText16( SEGPTR find);
BOOL16  WINAPI ChooseFont16(LPCHOOSEFONT16);
BOOL16 CALLBACK ColorDlgProc16( HWND16 hDlg16, UINT16 message, WPARAM16 wParam, LONG lParam );
BOOL16 CALLBACK FileSaveDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam);
BOOL16 CALLBACK FileOpenDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam);
INT16 WINAPI FontFamilyEnumProc16( SEGPTR logfont, SEGPTR metrics, UINT16 nFontType, LPARAM lParam );
INT16 WINAPI FontStyleEnumProc16( SEGPTR logfont, SEGPTR metrics, UINT16 nFontType, LPARAM lParam);
BOOL16 CALLBACK FormatCharDlgProc16(HWND16 hDlg16, UINT16 message, WPARAM16 wParam, LPARAM lParam);
short WINAPI GetFileTitle16(LPCSTR lpFile, LPSTR lpTitle, UINT16 cbBuf);
BOOL16 CALLBACK PrintDlgProc16(HWND16 hDlg16, UINT16 uMsg, WPARAM16 wParam, LPARAM lParam);
BOOL16 CALLBACK PrintSetupDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam);

#include "poppack.h"

#endif /* _WINE_DLL_CDLG16_H */
