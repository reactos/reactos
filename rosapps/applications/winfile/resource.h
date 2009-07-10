/*
 * Copyright 2000, 2003, 2005 Martin Fuchs
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

#include <windef.h>
#include <winuser.h>

/* images */

#define IDB_TOOLBAR                     100
#define IDB_DRIVEBAR                    101
#define IDB_IMAGES                      102
#define IDI_WINEFILE                    100


/* accelerators and menus */

#define IDA_WINEFILE                    101
#define IDM_WINEFILE                    102


/* dialogs */

#define IDD_EXECUTE                     103
#define IDD_SELECT_DESTINATION          104
#define IDD_DIALOG_VIEW_TYPE            105
#define IDD_DIALOG_PROPERTIES           106


/* control ids */

#define ID_ACTIVATE                     101
#define ID_EXECUTE                      105
#define ID_FILE_MOVE                    106
#define ID_FILE_COPY                    107
#define ID_FILE_DELETE                  108
#define ID_FILE_EXIT                    115
#define ID_FORMAT_DISK                  203
#define ID_CONNECT_NETWORK_DRIVE        252
#define ID_DISCONNECT_NETWORK_DRIVE     253
#define ID_VIEW_NAME                    401
#define ID_VIEW_ALL_ATTRIBUTES          402
#define ID_VIEW_SELECTED_ATTRIBUTES     403
#define ID_VIEW_SORT_NAME               404
#define ID_VIEW_SORT_TYPE               405
#define ID_VIEW_SORT_SIZE               406
#define ID_VIEW_SORT_DATE               407
#define ID_VIEW_FILTER                  409
#define ID_VIEW_SPLIT                   414
#define ID_SELECT_FONT                  510
#define ID_VIEW_TOOL_BAR                508
#define ID_VIEW_DRIVE_BAR               507
#define ID_VIEW_STATUSBAR               503
#define ID_VIEW_SAVESETTINGS            511

#define ID_ABOUT                        1803
#define ID_REFRESH                      1704
#define ID_EDIT_PROPERTIES              57656
#define ID_WINDOW_NEW                   0xE130
#define ID_WINDOW_ARRANGE               0xE131
#define ID_WINDOW_CASCADE               0xE132
#define ID_WINDOW_TILE_HORZ             0xE133
#define ID_WINDOW_TILE_VERT             0xE134
#define ID_WINDOW_SPLIT                 0xE135
#define ID_HELP_USING                   0xE144
#define ID_HELP                         0xE146

#define IDC_VIEW_PATTERN                1000
#define IDC_VIEW_TYPE_DIRECTORIES       1001
#define IDC_VIEW_TYPE_PROGRAMS          1002
#define IDC_VIEW_TYPE_DOCUMENTS         1003
#define IDC_VIEW_TYPE_OTHERS            1004
#define IDC_VIEW_TYPE_HIDDEN            1005

#define IDC_STATIC_PROP_FILENAME        1006
#define IDC_STATIC_PROP_PATH            1007
#define IDC_STATIC_PROP_LASTCHANGE      1008
#define IDC_STATIC_PROP_VERSION         1009
#define IDC_STATIC_PROP_COPYRIGHT       1010
#define IDC_STATIC_PROP_SIZE            1011
#define IDC_CHECK_READONLY              1012
#define IDC_CHECK_ARCHIVE               1013
#define IDC_CHECK_COMPRESSED            1014
#define IDC_CHECK_HIDDEN                1015
#define IDC_CHECK_SYSTEM                1016
#define IDC_LIST_PROP_VERSION_TYPES     1017
#define IDC_LIST_PROP_VERSION_VALUES    1018


/* winefile extensions */

#define ID_WINDOW_AUTOSORT              0x8003
#define ID_VIEW_FULLSCREEN              0x8004
#define ID_PREFERRED_SIZES              0x8005


/* string table */

#define IDS_FONT_SEL_DLG_NAME           1101
#define IDS_FONT_SEL_ERROR              1103

#define IDS_WINEFILE                    1200
#define IDS_ERROR                       1201
#define IDS_ROOT_FS                     1202
#define IDS_UNIXFS                      1203
#define IDS_DESKTOP                     1204
#define IDS_SHELL                       1205
#define IDS_TITLEFMT                    1206
#define IDS_NO_IMPL                     1207
#define IDS_WINE_FILE                   1208

#define IDS_COL_NAME                    1210
#define IDS_COL_SIZE                    1211
#define IDS_COL_CDATE                   1212
#define IDS_COL_ADATE                   1213
#define IDS_COL_MDATE                   1214
#define IDS_COL_IDX                     1215
#define IDS_COL_LINKS                   1216
#define IDS_COL_ATTR                    1217
#define IDS_COL_SEC                     1218
#define IDS_FREE_SPACE_FMT              1219

/* range for drive bar command ids: 0x9000..0x90FF */

#ifdef __WINE__
#define ID_DRIVE_UNIX_FS                0x9000
#endif
#define ID_DRIVE_SHELL_NS               0x9001

#define ID_DRIVE_FIRST                  0x9002
