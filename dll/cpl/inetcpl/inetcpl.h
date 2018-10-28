/*
 * Internet control panel applet
 *
 * Copyright 2010 Detlef Riekenberg
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
 *
 */

#ifndef __WINE_INETCPL__
#define __WINE_INETCPL__

#include <windef.h>
#include <winuser.h>
#include <commctrl.h>

extern HMODULE hcpl;
INT_PTR CALLBACK connections_dlgproc(HWND, UINT, WPARAM, LPARAM) DECLSPEC_HIDDEN;
INT_PTR CALLBACK content_dlgproc(HWND, UINT, WPARAM, LPARAM) DECLSPEC_HIDDEN;
INT_PTR CALLBACK general_dlgproc(HWND, UINT, WPARAM, LPARAM) DECLSPEC_HIDDEN;
INT_PTR CALLBACK security_dlgproc(HWND, UINT, WPARAM, LPARAM) DECLSPEC_HIDDEN;

#define NUM_PROPERTY_PAGES 8

/* icons */
#define ICO_MAIN            100
#define ICO_INTERNET        1313
#ifdef __REACTOS__
#define ICO_CERTIFICATES    1314
#define ICO_HISTORY         1315
#define ICO_HOME            1316
#define ICO_TRUSTED         4480
#define ICO_RESTRICTED      4481
#endif

/* strings */
#define IDS_CPL_NAME        1
#define IDS_CPL_INFO        2
#define IDS_SEC_SETTINGS    0x10
#define IDS_SEC_LEVEL0      0x100
#define IDS_SEC_LEVEL1      0x101
#define IDS_SEC_LEVEL2      0x102
#define IDS_SEC_LEVEL3      0x103
#define IDS_SEC_LEVEL4      0x104
#define IDS_SEC_LEVEL5      0x105
#define IDS_SEC_LEVEL0_INFO 0x200
#define IDS_SEC_LEVEL1_INFO 0x210
#define IDS_SEC_LEVEL2_INFO 0x220
#define IDS_SEC_LEVEL3_INFO 0x230
#define IDS_SEC_LEVEL4_INFO 0x240
#define IDS_SEC_LEVEL5_INFO 0x250

/* dialogs */
#define IDC_STATIC          -1

#define IDD_GENERAL         1000
#define IDC_HOME_EDIT       1000
#define IDC_HOME_CURRENT    1001
#define IDC_HOME_DEFAULT    1002
#define IDC_HOME_BLANK      1003
#define IDC_HISTORY_DELETE     1004
#define IDC_HISTORY_SETTINGS   1005

#define IDD_DELETE_HISTORY     1010
#define IDC_DELETE_TEMP_FILES  1011
#define IDC_DELETE_COOKIES     1012
#define IDC_DELETE_HISTORY     1013
#define IDC_DELETE_FORM_DATA   1014
#define IDC_DELETE_PASSWORDS   1015

#define IDD_SECURITY        2000
#define IDC_SEC_LISTVIEW    2001
#define IDC_SEC_ZONE_INFO   2002
#define IDC_SEC_GROUP       2003
#define IDC_SEC_TRACKBAR    2004
#define IDC_SEC_LEVEL       2005
#define IDC_SEC_LEVEL_INFO  2006

#define IDD_CONTENT         4000
#define IDC_CERT            4100
#define IDC_CERT_PUBLISHER  4101

#define IDD_CONNECTIONS         5000
#define IDC_USE_WPAD            5100
#define IDC_USE_PAC_SCRIPT      5101
#define IDC_EDIT_PAC_SCRIPT     5102
#define IDC_USE_PROXY_SERVER    5200
#define IDC_EDIT_PROXY_SERVER   5201
#define IDC_EDIT_PROXY_PORT     5202

#endif
