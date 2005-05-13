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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


/* images */

#define IDB_TOOLBAR						100
#define IDB_DRIVEBAR					101
#define IDB_IMAGES						102
#define IDI_WINEFILE					100


/* accellerators and menus */

#define IDA_WINEFILE					101
#define IDM_WINEFILE					102


/* dialogs */

#define IDD_EXECUTE						103
#define IDD_SELECT_DESTINATION			104


/* control ids */

#define ID_ACTIVATE						101
#define ID_EXECUTE						105
#define ID_FILE_MOVE					106
#define ID_FILE_EXIT					115
#define	ID_FORMAT_DISK					203
#define	ID_CONNECT_NETWORK_DRIVE		252
#define	ID_DISCONNECT_NETWORK_DRIVE		253
#define ID_VIEW_NAME					401
#define ID_VIEW_ALL_ATTRIBUTES			402
#define ID_VIEW_SELECTED_ATTRIBUTES		403
#define ID_SELECT_FONT					510
#define ID_VIEW_TOOL_BAR				508
#define ID_VIEW_DRIVE_BAR				507
#define ID_VIEW_STATUSBAR				503

#define ID_ABOUT						1803
#define ID_REFRESH						1704
#define ID_EDIT_PROPERTIES				57656
#define ID_WINDOW_NEW					0xE130
#define ID_WINDOW_ARRANGE				0xE131
#define ID_WINDOW_CASCADE				0xE132
#define ID_WINDOW_TILE_HORZ 			0xE133
#define ID_WINDOW_TILE_VERT 			0xE134
#define ID_WINDOW_SPLIT 				0xE135
#define ID_HELP_USING					0xE144
#define ID_HELP 						0xE146


/* winefile extensions */

#define ID_ABOUT_WINE					0x8000
#define ID_LICENSE						0x8001
#define ID_NO_WARRANTY					0x8002
#define ID_WINDOW_AUTOSORT				0x8003
#define ID_VIEW_FULLSCREEN				0x8004
#define ID_PREFERRED_SIZES				0x8005


/* string table */

#define IDS_FONT_SEL_DLG_NAME			1101
#define IDS_FONT_SEL_ERROR				1103

#define IDS_WINEFILE					1200
#define IDS_ERROR						1201
#define IDS_ROOT_FS						1202
#define IDS_UNIXFS						1203
#define IDS_DESKTOP						1204
#define IDS_SHELL						1205
#define IDS_TITLEFMT					1206
#define IDS_NO_IMPL						1207
#define IDS_WINE						1208
#define IDS_WINE_FILE					1209

#define IDS_COL_NAME					1210
#define IDS_COL_SIZE					1211
#define IDS_COL_CDATE					1212
#define IDS_COL_ADATE					1213
#define IDS_COL_MDATE					1214
#define IDS_COL_IDX						1215
#define IDS_COL_LINKS					1216
#define IDS_COL_ATTR					1217
#define IDS_COL_SEC						1218


/* range for drive bar command ids: 0x9000..0x90FF */

#ifdef __WINE__
#define ID_DRIVE_UNIX_FS				0x9000
#endif
#define ID_DRIVE_SHELL_NS				0x9001

#define ID_DRIVE_FIRST					0x9002
