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

/* Common dialogs implementation globals */
#define COMDLG32_Atom   MAKEINTATOM(0xa000)     /* MS uses this one to identify props */

extern HINSTANCE	COMDLG32_hInstance DECLSPEC_HIDDEN;

void	COMDLG32_SetCommDlgExtendedError(DWORD err) DECLSPEC_HIDDEN;
LPVOID	COMDLG32_AllocMem(int size) __WINE_ALLOC_SIZE(1) DECLSPEC_HIDDEN;

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
#define PD32_MARGINS_IN_MILLIMETERS           1586
#define PD32_MILLIMETERS                      1587

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

/* Font styles */

#define IDS_FONT_REGULAR        256
#define IDS_FONT_BOLD           257
#define IDS_FONT_ITALIC         258
#define IDS_FONT_BOLD_ITALIC    259

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

/* Color dialog controls */
#define IDC_COLOR_LUMBAR 702
#define IDC_COLOR_EDIT_H 703
#define IDC_COLOR_EDIT_S 704
#define IDC_COLOR_EDIT_L 705
#define IDC_COLOR_EDIT_R 706
#define IDC_COLOR_EDIT_G 707
#define IDC_COLOR_EDIT_B 708
#define IDC_COLOR_RESULT 709
#define IDC_COLOR_GRAPH  710
#define IDC_COLOR_ADD    712
#define IDC_COLOR_RES    713
#define IDC_COLOR_DEFINE 719
#define IDC_COLOR_PREDEF 720
#define IDC_COLOR_USRDEF 721
#define IDC_COLOR_HL     723
#define IDC_COLOR_SL     724
#define IDC_COLOR_LL     725
#define IDC_COLOR_RL     726
#define IDC_COLOR_GL     727
#define IDC_COLOR_BL     728

#define IDS_FONT_SIZE    1200
#define IDS_SAVE_BUTTON  1201
#define IDS_SAVE_IN      1202
#define IDS_SAVE         1203
#define IDS_SAVE_AS      1204
#define IDS_OPEN_FILE    1205
#define IDS_SELECT_FOLDER 1206
#define IDS_FONT_SIZE_INPUT 1207

#define IDS_FAKEDOCTEXT  1300

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "shlobj.h"
#include "shellapi.h"

/* Constructors */
HRESULT FileOpenDialog_Constructor(IUnknown *pUnkOuter, REFIID riid, void **ppv) DECLSPEC_HIDDEN;
HRESULT FileSaveDialog_Constructor(IUnknown *pUnkOuter, REFIID riid, void **ppv) DECLSPEC_HIDDEN;

/* Shared helper functions */
void COMDLG32_GetCanonicalPath(PCIDLIST_ABSOLUTE pidlAbsCurrent, LPWSTR lpstrFile, LPWSTR lpstrPathAndFile) DECLSPEC_HIDDEN;
int FILEDLG95_ValidatePathAction(LPWSTR lpstrPathAndFile, IShellFolder **ppsf,
                                 HWND hwnd, DWORD flags, BOOL isSaveDlg, int defAction) DECLSPEC_HIDDEN;
int COMDLG32_SplitFileNames(LPWSTR lpstrEdit, UINT nStrLen, LPWSTR *lpstrFileList, UINT *sizeUsed) DECLSPEC_HIDDEN;
void FILEDLG95_OnOpenMessage(HWND hwnd, int idCaption, int idText) DECLSPEC_HIDDEN;

extern BOOL GetFileName31A( OPENFILENAMEA *lpofn, UINT dlgType ) DECLSPEC_HIDDEN;
extern BOOL GetFileName31W( OPENFILENAMEW *lpofn, UINT dlgType ) DECLSPEC_HIDDEN;

/* SHELL */
extern LPITEMIDLIST (WINAPI *COMDLG32_SHSimpleIDListFromPathAW)(LPCVOID);

#define ONOPEN_BROWSE 1
#define ONOPEN_OPEN   2
#define ONOPEN_SEARCH 3

#endif /* _WINE_DLL_CDLG_H */
