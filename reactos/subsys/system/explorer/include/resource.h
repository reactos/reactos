/*
 * Copyright 2000 Martin Fuchs
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
#define	IDB_TOOLBAR					100
#define	IDB_DRIVEBAR					101
#define	IDB_IMAGES					102
#define	IDI_WINEFILE					100


/* accellerators and menus */

#define	IDA_WINEFILE					101
#define	IDM_WINEFILE					102


/* dialogs */

#define	IDD_EXECUTE					103


/* control ids */

#define	ID_ACTIVATE					101
#define	ID_EXECUTE					105
#define	ID_FILE_EXIT					115
#define	ID_VIEW_NAME					401
#define	ID_VIEW_ALL_ATTRIBUTES				402
#define	ID_VIEW_SELECTED_ATTRIBUTES			403
#define	ID_VIEW_TOOL_BAR				508
#define	ID_VIEW_DRIVE_BAR				507
#define	ID_VIEW_STATUSBAR				503

#define	ID_ABOUT					1803
#define	ID_REFRESH					1704
#define	ID_EDIT_PROPERTIES				57656
#define	ID_WINDOW_NEW					0xE130
#define	ID_WINDOW_ARRANGE				0xE131
#define	ID_WINDOW_CASCADE				0xE132
#define	ID_WINDOW_TILE_HORZ 				0xE133
#define	ID_WINDOW_TILE_VERT 				0xE134
#define	ID_WINDOW_SPLIT 				0xE135
#define	ID_HELP_USING					0xE144
#define	ID_HELP 					0xE146

/* range for drive bar command ids: 0x9000..0x90FF */
#define	ID_DRIVE_FIRST					0x9001


/* winefile extensions */
#define	ID_ABOUT_WINE					0x8000
#define	ID_LICENSE					0x8001
#define	ID_NO_WARRANTY					0x8002
#define	ID_WINDOW_AUTOSORT				0x8003
#define	ID_VIEW_FULLSCREEN				0x8004
#define	ID_PREFERED_SIZES				0x8005

#ifdef __linux__
#define	ID_DRIVE_UNIX_FS				0x9000
#endif
