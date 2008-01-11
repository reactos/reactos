/*
 *  Common Dialog Boxes interface (32 bit)
 *
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

#ifndef _WINE_DLL_CDLG_H
#define _WINE_DLL_CDLG_H

#include "dlgs.h"
#include "wownt32.h"

/* Common dialogs implementation globals */
#define COMDLG32_Atom	((ATOM)0xa000)	/* MS uses this one to identify props */

extern HINSTANCE	COMDLG32_hInstance;

void	COMDLG32_SetCommDlgExtendedError(DWORD err);
LPVOID	COMDLG32_AllocMem(int size);

/* handle<-handle16 conversion */
#define HINSTANCE_32(h16)           ((HINSTANCE)(ULONG_PTR)(h16))

/* Find/Replace local definitions */

#define FR_WINE_UNICODE		0x80000000
#define FR_WINE_REPLACE		0x40000000

typedef struct {
	FINDREPLACEA	fr;	/* Internally used structure */
        union {
		FINDREPLACEA	*fra;	/* Reference to the user supplied structure */
		FINDREPLACEW	*frw;
        } user_fr;
} COMDLG32_FR_Data;

#define PD32_PRINT_TITLE        7000

#define PD32_VALUE_UREADABLE                  1104
#define PD32_INVALID_PAGE_RANGE               1105
#define PD32_FROM_NOT_ABOVE_TO                1106
#define PD32_MARGINS_OVERLAP                  1107
#define PD32_NR_OF_COPIES_EMPTY               1108
#define PD32_TOO_LARGE_COPIES                 1109
#define PD32_PRINT_ERROR                      1110
#define PD32_NO_DEFAULT_PRINTER               1111
#define PD32_CANT_FIND_PRINTER                1112
#define PD32_OUT_OF_MEMORY                    1113
#define PD32_GENERIC_ERROR                    1114
#define PD32_DRIVER_UNKNOWN                   1115
#define PD32_NO_DEVICES                       1121

#define PD32_PRINTER_STATUS_READY             1536
#define PD32_PRINTER_STATUS_PAUSED            1537
#define PD32_PRINTER_STATUS_ERROR             1538
#define PD32_PRINTER_STATUS_PENDING_DELETION  1539
#define PD32_PRINTER_STATUS_PAPER_JAM         1540
#define PD32_PRINTER_STATUS_PAPER_OUT         1541
#define PD32_PRINTER_STATUS_MANUAL_FEED       1542
#define PD32_PRINTER_STATUS_PAPER_PROBLEM     1543
#define PD32_PRINTER_STATUS_OFFLINE           1544
#define PD32_PRINTER_STATUS_IO_ACTIVE         1545
#define PD32_PRINTER_STATUS_BUSY              1546
#define PD32_PRINTER_STATUS_PRINTING          1547
#define PD32_PRINTER_STATUS_OUTPUT_BIN_FULL   1548
#define PD32_PRINTER_STATUS_NOT_AVAILABLE     1549
#define PD32_PRINTER_STATUS_WAITING           1550
#define PD32_PRINTER_STATUS_PROCESSING        1551
#define PD32_PRINTER_STATUS_INITIALIZING      1552
#define PD32_PRINTER_STATUS_WARMING_UP        1553
#define PD32_PRINTER_STATUS_TONER_LOW         1554
#define PD32_PRINTER_STATUS_NO_TONER          1555
#define PD32_PRINTER_STATUS_PAGE_PUNT         1556
#define PD32_PRINTER_STATUS_USER_INTERVENTION 1557
#define PD32_PRINTER_STATUS_OUT_OF_MEMORY     1558
#define PD32_PRINTER_STATUS_DOOR_OPEN         1559
#define PD32_PRINTER_STATUS_SERVER_UNKNOWN    1560
#define PD32_PRINTER_STATUS_POWER_SAVE        1561

#define PD32_DEFAULT_PRINTER                  1582
#define PD32_NR_OF_DOCUMENTS_IN_QUEUE         1583

#define PD32_MARGINS_IN_INCHES                1585
#define PD32_MARGINS_IN_MILIMETERS            1586
#define PD32_MILIMETERS                       1587

/* Charset names string IDs */

#define IDS_CHARSET_ANSI        200
#define IDS_CHARSET_SYMBOL      201
#define IDS_CHARSET_JIS         202
#define IDS_CHARSET_HANGUL      203
#define IDS_CHARSET_GB2312      204
#define IDS_CHARSET_BIG5        205
#define IDS_CHARSET_GREEK       206
#define IDS_CHARSET_TURKISH     207
#define IDS_CHARSET_HEBREW      208
#define IDS_CHARSET_ARABIC      209
#define IDS_CHARSET_BALTIC      210
#define IDS_CHARSET_VIETNAMESE  211
#define IDS_CHARSET_RUSSIAN     212
#define IDS_CHARSET_EE          213
#define IDS_CHARSET_THAI        214
#define IDS_CHARSET_JOHAB       215
#define IDS_CHARSET_MAC         216
#define IDS_CHARSET_OEM         217
#define IDS_CHARSET_VISCII      218
#define IDS_CHARSET_TCVN        219
#define IDS_CHARSET_KOI8        220
#define IDS_CHARSET_ISO3        221
#define IDS_CHARSET_ISO4        222
#define IDS_CHARSET_ISO10       223
#define IDS_CHARSET_CELTIC      224

/* Color names string IDs */

#define IDS_COLOR_BLACK                 1040
#define IDS_COLOR_MAROON                1041
#define IDS_COLOR_GREEN                 1042
#define IDS_COLOR_OLIVE                 1043
#define IDS_COLOR_NAVY                  1044
#define IDS_COLOR_PURPLE                1045
#define IDS_COLOR_TEAL                  1046
#define IDS_COLOR_GRAY                  1047
#define IDS_COLOR_SILVER                1048
#define IDS_COLOR_RED                   1049
#define IDS_COLOR_LIME                  1050
#define IDS_COLOR_YELLOW                1051
#define IDS_COLOR_BLUE                  1052
#define IDS_COLOR_FUCHSIA               1053
#define IDS_COLOR_AQUA                  1054
#define IDS_COLOR_WHITE                 1055

#define IDS_FONT_SIZE    1200
#define IDS_SAVE_BUTTON  1201
#define IDS_SAVE_IN      1202
#define IDS_SAVE         1203
#define IDS_SAVE_AS      1204
#define IDS_OPEN_FILE    1205

#define IDS_FAKEDOCTEXT  1300

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "shlobj.h"
#include "shellapi.h"

/* ITEMIDLIST */

extern LPITEMIDLIST (WINAPI *COMDLG32_PIDL_ILClone) (LPCITEMIDLIST);
extern LPITEMIDLIST (WINAPI *COMDLG32_PIDL_ILCombine)(LPCITEMIDLIST,LPCITEMIDLIST);
extern LPITEMIDLIST (WINAPI *COMDLG32_PIDL_ILGetNext)(LPITEMIDLIST);
extern BOOL (WINAPI *COMDLG32_PIDL_ILRemoveLastID)(LPCITEMIDLIST);
extern BOOL (WINAPI *COMDLG32_PIDL_ILIsEqual)(LPCITEMIDLIST, LPCITEMIDLIST);

/* SHELL */
extern LPVOID (WINAPI *COMDLG32_SHAlloc)(DWORD);
extern DWORD (WINAPI *COMDLG32_SHFree)(LPVOID);
extern BOOL (WINAPI *COMDLG32_SHGetFolderPathA)(HWND,int,HANDLE,DWORD,LPSTR);
extern BOOL (WINAPI *COMDLG32_SHGetFolderPathW)(HWND,int,HANDLE,DWORD,LPWSTR);

extern BOOL  WINAPI GetFileDialog95A(LPOPENFILENAMEA ofn,UINT iDlgType);
extern BOOL  WINAPI GetFileDialog95W(LPOPENFILENAMEW ofn,UINT iDlgType);

/*
 * Internal Functions
 * Do NOT Export to other programs and dlls
 */

BOOL CC_HookCallChk( const CHOOSECOLORW *lpcc );
int CC_MouseCheckResultWindow( HWND hDlg, LPARAM lParam );
LRESULT CC_WMLButtonDown( HWND hDlg, WPARAM wParam, LPARAM lParam );
LRESULT CC_WMLButtonUp( HWND hDlg, WPARAM wParam, LPARAM lParam );
LRESULT CC_WMCommand( HWND hDlg, WPARAM wParam, LPARAM lParam, WORD 
						notifyCode, HWND hwndCtl );
LRESULT CC_WMMouseMove( HWND hDlg, LPARAM lParam );
LRESULT CC_WMPaint( HWND hDlg, WPARAM wParam, LPARAM lParam );
void CC_SwitchToFullSize( HWND hDlg, COLORREF result, LPCRECT lprect );
void CC_PaintSelectedColor( HWND hDlg, COLORREF cr );
int CC_RGBtoHSL(char c, int r, int g, int b);
void CC_PaintCross( HWND hDlg, int x, int y);
void CC_PaintTriangle( HWND hDlg, int y);
int CC_CheckDigitsInEdit( HWND hwnd, int maxval );
void CC_EditSetHSL( HWND hDlg, int h, int s, int l );
int CC_HSLtoRGB(char c, int hue, int sat, int lum);
void CC_EditSetRGB( HWND hDlg, COLORREF cr );
void CC_PaintUserColorArray( HWND hDlg, int rows, int cols, const COLORREF* lpcr );

typedef struct
{
  HWND hWnd1;
  HWND hWnd2;
  LPCHOOSEFONTW lpcf32w;
  int  added;
} CFn_ENUMSTRUCT, *LPCFn_ENUMSTRUCT;

INT AddFontFamily(const ENUMLOGFONTEXW *lpElfex, const NEWTEXTMETRICEXW *lpNTM,
                  UINT nFontType, const CHOOSEFONTW *lpcf, HWND hwnd,
                  LPCFn_ENUMSTRUCT e);
INT AddFontStyle(const ENUMLOGFONTEXW *lpElfex, const NEWTEXTMETRICEXW *metrics,
                 UINT nFontType, const CHOOSEFONTW *lpcf, HWND hcmb2, HWND hcmb3,
                 HWND hDlg, BOOL iswin16);
void _dump_cf_flags(DWORD cflags);

LRESULT CFn_WMInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam,
                         LPCHOOSEFONTW lpcf);
LRESULT CFn_WMMeasureItem(HWND hDlg, WPARAM wParam, LPARAM lParam);
LRESULT CFn_WMDrawItem(HWND hDlg, WPARAM wParam, LPARAM lParam);
LRESULT CFn_WMCommand(HWND hDlg, WPARAM wParam, LPARAM lParam,
                      LPCHOOSEFONTW lpcf);
LRESULT CFn_WMPaint(HWND hDlg, WPARAM wParam, LPARAM lParam,
                      const CHOOSEFONTW *lpcf);

#endif /* _WINE_DLL_CDLG_H */
