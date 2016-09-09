/*
 * Copyright 2004 by Krzysztof Foltman
 * Copyright 2007-2008 by Alexander N. Sørnes <alex@thehandofagony.com>
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

#pragma once

#define MAX_STRING_LEN 255

#define TWIPS_PER_INCH 1440
#define CENTMM_PER_INCH 2540

#define ID_FILE_EXIT 1000
#define ID_FILE_OPEN 1001
#define ID_FILE_SAVE 1002
#define ID_FILE_NEW 1003
#define ID_FILE_SAVEAS 1004

#define ID_FILE_RECENT1 1005
#define ID_FILE_RECENT2 1006
#define ID_FILE_RECENT3 1007
#define ID_FILE_RECENT4 1008
#define ID_FILE_RECENT_SEPARATOR 1009

#define ID_PRINT 1010
#define ID_PREVIEW 1011
#define ID_PRINTSETUP 1012
#define ID_PRINT_QUICK 1013

#define ID_FIND 1014
#define ID_FIND_NEXT 1015
#define ID_REPLACE 1016

#define ID_PREVIEW_NEXTPAGE 1017
#define ID_PREVIEW_PREVPAGE 1018
#define ID_PREVIEW_NUMPAGES 1019
#define ID_PREVIEW_ZOOMIN 1020
#define ID_PREVIEW_ZOOMOUT 1021

#define ID_ALIGN_LEFT 1100
#define ID_ALIGN_CENTER 1101
#define ID_ALIGN_RIGHT 1102

#define ID_BACK_1 1200
#define ID_BACK_2 1201

#define ID_EDIT_SELECTALL 1300
#define ID_EDIT_SELECTIONINFO 1301
#define ID_EDIT_READONLY 1302
#define ID_EDIT_MODIFIED 1303
#define ID_EDIT_CHARFORMAT 1304
#define ID_EDIT_PARAFORMAT 1305
#define ID_EDIT_DEFCHARFORMAT 1306
#define ID_EDIT_UNDO 1307
#define ID_EDIT_REDO 1308
#define ID_EDIT_GETTEXT 1309
#define ID_EDIT_COPY 1310
#define ID_EDIT_CUT 1311
#define ID_EDIT_PASTE 1312
#define ID_EDIT_CLEAR 1313
#define ID_BULLET 1314

#define ID_FONTSETTINGS 1315

#define ID_FORMAT_BOLD 1400
#define ID_FORMAT_ITALIC 1401
#define ID_FORMAT_UNDERLINE 1402
#define ID_FORMAT_COLOR 1403

#define ID_TOGGLE_TOOLBAR 1500
#define ID_TOGGLE_FORMATBAR 1501
#define ID_TOGGLE_STATUSBAR 1502
#define ID_TOGGLE_RULER 1503

#define PREVIEW_BUTTONS 7

#define FILELIST_ENTRIES 4
#define FILELIST_ENTRY_LENGTH 33

#define BANDID_TOOLBAR 2
#define BANDID_FORMATBAR 3
#define BANDID_RULER 0
#define BANDID_STATUSBAR 1
#define BANDID_FONTLIST 4
#define BANDID_SIZELIST 5

#define BANDID_PREVIEW_BTN1 6
#define BANDID_PREVIEW_BTN2 7
#define BANDID_PREVIEW_BTN3 8
#define BANDID_PREVIEW_BTN4 9
#define BANDID_PREVIEW_BTN5 10
#define BANDID_PREVIEW_BTN6 11
#define BANDID_PREVIEW_BTN7 12
#define BANDID_PREVIEW_BUFFER 13

#define ID_WORDWRAP_NONE 0
#define ID_WORDWRAP_WINDOW 1
#define ID_WORDWRAP_MARGIN 2

#define ID_NEWFILE_ABORT 100

#define ID_TAB_ADD 100
#define ID_TAB_DEL 101
#define ID_TAB_EMPTY 102

#define IDC_PAGEFMT_TB 100
#define IDC_PAGEFMT_FB 101
#define IDC_PAGEFMT_RU 102
#define IDC_PAGEFMT_SB 103
#define IDC_PAGEFMT_WN 104
#define IDC_PAGEFMT_WW 105
#define IDC_PAGEFMT_WM 106
#define IDC_PAGEFMT_ID 107

#define ID_DATETIME 1600
#define ID_PARAFORMAT 1601
#define ID_TABSTOPS 1602

#define ID_ABOUT 1603
#define ID_VIEWPROPERTIES 1604

#define ID_COLOR_FIRST 1800
#define ID_COLOR_BLACK 1800
#define ID_COLOR_MAROON 1801
#define ID_COLOR_GREEN 1802
#define ID_COLOR_OLIVE 1803
#define ID_COLOR_NAVY 1804
#define ID_COLOR_PURPLE 1805
#define ID_COLOR_TEAL 1806
#define ID_COLOR_GRAY 1807
#define ID_COLOR_SILVER 1808
#define ID_COLOR_RED 1809
#define ID_COLOR_LIME 1810
#define ID_COLOR_YELLOW 1811
#define ID_COLOR_BLUE 1812
#define ID_COLOR_FUCHSIA 1813
#define ID_COLOR_AQUA 1814
#define ID_COLOR_WHITE 1815
#define ID_COLOR_AUTOMATIC 1816

#define IDC_STATUSBAR 2000
#define IDC_EDITOR 2001
#define IDC_TOOLBAR 2002
#define IDC_FORMATBAR 2003
#define IDC_REBAR 2004
#define IDC_COMBO 2005
#define IDC_DATETIME 2006
#define IDC_NEWFILE 2007
#define IDC_PARA_LEFT 2008
#define IDC_PARA_RIGHT 2009
#define IDC_PARA_FIRST 2010
#define IDC_PARA_ALIGN 2011
#define IDC_TABSTOPS 2012
#define IDC_FONTLIST 2013
#define IDC_SIZELIST 2014
#define IDC_RULER 2015
#define IDC_PREVIEW 2016

#define IDD_DATETIME 2100
#define IDD_NEWFILE 2101
#define IDD_PARAFORMAT 2102
#define IDD_TABSTOPS 2103
#define IDD_FORMATOPTS 2104

#define IDM_MAINMENU 2200
#define IDM_POPUP 2201
#define IDM_COLOR_POPUP 2202

#define IDB_TOOLBAR 100
#define IDB_FORMATBAR 101

#define IDI_WORDPAD 102

#ifdef __REACTOS__

#define IDI_RTFDOC  103
#define IDC_ZOOM 104

#else

#define IDC_ZOOM 103

#endif

#define STRING_ALL_FILES 1400
#define STRING_TEXT_FILES_TXT 1401
#define STRING_TEXT_FILES_UNICODE_TXT 1402
#define STRING_RICHTEXT_FILES_RTF 1403

#define STRING_NEWFILE_RICHTEXT 1404
#define STRING_NEWFILE_TXT 1405
#define STRING_NEWFILE_TXT_UNICODE 1406

#define STRING_PRINTER_FILES_PRN 1407

#define STRING_ALIGN_LEFT 1416
#define STRING_ALIGN_RIGHT 1417
#define STRING_ALIGN_CENTER 1418

#define STRING_VIEWPROPS_TITLE 1432
#define STRING_VIEWPROPS_TEXT 1433
#define STRING_VIEWPROPS_RICHTEXT 1434

#define STRING_PREVIEW_PRINT 1448
#define STRING_PREVIEW_NEXTPAGE 1449
#define STRING_PREVIEW_PREVPAGE 1450
#define STRING_PREVIEW_TWOPAGES 1451
#define STRING_PREVIEW_ONEPAGE 1452
#define STRING_PREVIEW_ZOOMIN 1453
#define STRING_PREVIEW_ZOOMOUT 1454
#define STRING_PREVIEW_CLOSE 1455
#define STRING_PREVIEW_PAGE 1456
#define STRING_PREVIEW_PAGES 1457

#define STRING_UNITS_CM 1458
#define STRING_UNITS_IN 1459
#define STRING_UNITS_INCH 1460
#define STRING_UNITS_PT 1461

#define STRING_DEFAULT_FILENAME 1700
#define STRING_PROMPT_SAVE_CHANGES 1701
#define STRING_SEARCH_FINISHED 1702
#define STRING_LOAD_RICHED_FAILED 1703
#define STRING_SAVE_LOSEFORMATTING 1704
#define STRING_INVALID_NUMBER 1705
#define STRING_OLE_STORAGE_NOT_SUPPORTED 1706
#define STRING_WRITE_FAILED 1707
#define STRING_WRITE_ACCESS_DENIED 1708
#define STRING_OPEN_FAILED 1709
#define STRING_OPEN_ACCESS_DENIED 1710
#define STRING_PRINTING_NOT_IMPLEMENTED 1711
#define STRING_MAX_TAB_STOPS 1712

/* wordpad.c */
LPWSTR file_basename(LPWSTR);

/* print.c */
void dialog_printsetup(HWND);
void dialog_print(HWND, LPWSTR);
void target_device(HWND, DWORD);
void print_quick(HWND, LPWSTR);
LRESULT preview_command(HWND, WPARAM);
void init_preview(HWND, LPWSTR);
void close_preview(HWND);
BOOL preview_isactive(void);
void get_default_printer_opts(void);
void registry_set_pagemargins(HKEY);
void registry_read_pagemargins(HKEY);
void registry_set_previewpages(HKEY hKey);
void registry_read_previewpages(HKEY hKey);
LRESULT CALLBACK ruler_proc(HWND, UINT, WPARAM, LPARAM);
void redraw_ruler(HWND);

/* registry.c */
int reg_formatindex(WPARAM);
void registry_read_filelist(HWND);
void registry_read_options(void);
void registry_read_formatopts_all(DWORD[], DWORD[]);
void registry_read_winrect(RECT*);
void registry_read_maximized(DWORD*);
void registry_set_filelist(LPCWSTR, HWND);
void registry_set_formatopts_all(DWORD[], DWORD[]);
void registry_set_options(HWND);

/* olecallback.c */
HRESULT setup_richedit_olecallback(HWND hEditorWnd);
