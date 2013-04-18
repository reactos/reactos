/*
 * Resource identifiers for ieframe.dll
 *
 * Copyright 2010 Alexander N. Sørnes <alex@thehandofagony.com>
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

#define IDR_BROWSE_MAIN_MENU           1000
#define IDD_BROWSE_OPEN                1001
#define IDC_BROWSE_OPEN_URL            1002
#define IDC_BROWSE_REBAR               1003
#define IDC_BROWSE_ADDRESSBAR          1004
#define IDC_BROWSE_STATUSBAR           1005
#define IDC_BROWSE_TOOLBAR             1006

#define ID_BROWSE_NEW_WINDOW           275
#define ID_BROWSE_OPEN                 256
#define ID_BROWSE_SAVE                 257
#define ID_BROWSE_SAVE_AS              258
#define ID_BROWSE_PRINT_FORMAT         259
#define ID_BROWSE_PRINT                260
#define ID_BROWSE_PRINT_PREVIEW        277
#define ID_BROWSE_PROPERTIES           262
#define ID_BROWSE_QUIT                 278
#define ID_BROWSE_ABOUT                336

#define ID_BROWSE_ADDFAV               1200
#define ID_BROWSE_HOME                 1201
#define ID_BROWSE_BACK                 1202
#define ID_BROWSE_FORWARD              1203
#define ID_BROWSE_STOP                 1204
#define ID_BROWSE_REFRESH              1205

#define ID_BROWSE_BAR_STD              1300
#define ID_BROWSE_BAR_ADDR             1301

#define ID_BROWSE_GOTOFAV_FIRST        2000
#define ID_BROWSE_GOTOFAV_MAX          65000

#define IDS_TB_BACK                    1100
#define IDS_TB_FORWARD                 1101
#define IDS_TB_STOP                    1102
#define IDS_TB_REFRESH                 1103
#define IDS_TB_HOME                    1104
#define IDS_TB_PRINT                   1105

#define IDS_ADDRESS                    1106

#define IDB_IETOOLBAR                  1400

/* update status text in BINDSTATUS_* callback */
#define IDS_STATUSFMT_FIRST            4096
#define IDS_FINDINGRESOURCE            (IDS_STATUSFMT_FIRST + 1)
#define IDS_BEGINDOWNLOADDATA          (IDS_STATUSFMT_FIRST + 4)
#define IDS_ENDDOWNLOADDATA            (IDS_STATUSFMT_FIRST + 6)
#define IDS_SENDINGREQUEST             (IDS_STATUSFMT_FIRST + 11)
#define IDS_STATUSFMT_MAXLEN           256
