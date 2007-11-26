/*
 * Copyright 2000 Juergen Schmied
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

#ifndef __WINE_SHELL_RES_H
#define __WINE_SHELL_RES_H

#define IDC_STATIC                  -1

/* Bitmap ID's */
#define IDB_SHELL_ABOUT_LOGO_24BPP  131

/*
	columntitles for the shellview
*/
#define IDS_SHV_COLUMN1             7
#define IDS_SHV_COLUMN2             8
#define IDS_SHV_COLUMN3             9
#define IDS_SHV_COLUMN4             10
#define IDS_SHV_COLUMN5             11
#define IDS_SHV_COLUMN6             12
#define IDS_SHV_COLUMN7             13
#define IDS_SHV_COLUMN8             14
#define IDS_SHV_COLUMN9             15
#define IDS_SHV_COLUMN10            16
#define IDS_SHV_COLUMN11            17
#define IDS_SHV_COLUMN_DELFROM      18
#define IDS_SHV_COLUMN_DELDATE      19

#define IDS_DESKTOP                 20
#define IDS_MYCOMPUTER              21

#define IDS_SELECT                  22
#define IDS_OPEN                    23
#define IDS_VIEW_LARGE              24
#define IDS_VIEW_SMALL              25
#define IDS_VIEW_LIST               26
#define IDS_VIEW_DETAILS            27

#define IDS_RESTART_TITLE           40
#define IDS_RESTART_PROMPT          41
#define IDS_SHUTDOWN_TITLE          42
#define IDS_SHUTDOWN_PROMPT         43

#define IDS_PROGRAMS                45
#define IDS_PERSONAL                46
#define IDS_FAVORITES               47
#define IDS_STARTUP                 48
#define IDS_RECENT                  49
#define IDS_SENDTO                  50
#define IDS_STARTMENU               51
#define IDS_MYMUSIC                 52
#define IDS_MYVIDEO                 53
#define IDS_DESKTOPDIRECTORY        54
#define IDS_NETHOOD                 55
#define IDS_TEMPLATES               56
#define IDS_APPDATA                 57
#define IDS_PRINTHOOD               58
#define IDS_LOCAL_APPDATA           59
#define IDS_INTERNET_CACHE          60
#define IDS_COOKIES                 61
#define IDS_HISTORY                 62
#define IDS_PROGRAM_FILES           63
#define IDS_MYPICTURES              64
#define IDS_PROGRAM_FILES_COMMON    65
#define IDS_COMMON_DOCUMENTS        66
#define IDS_ADMINTOOLS              67
#define IDS_COMMON_MUSIC            68
#define IDS_COMMON_PICTURES         69
#define IDS_COMMON_VIDEO            70
#define IDS_CDBURN_AREA             71
#define IDS_DRIVE_FIXED             72
#define IDS_DRIVE_CDROM             73
#define IDS_DRIVE_NETWORK           74

#define IDS_CREATEFOLDER_DENIED     128
#define IDS_CREATEFOLDER_CAPTION    129
#define IDS_DELETEITEM_CAPTION      130
#define IDS_DELETEFOLDER_CAPTION    131
#define IDS_DELETEITEM_TEXT         132
#define IDS_DELETEMULTIPLE_TEXT     133
#define IDS_OVERWRITEFILE_CAPTION   134
#define IDS_OVERWRITEFILE_TEXT      135
#define IDS_DELETESELECTED_TEXT     136
#define IDS_TRASHFOLDER_TEXT        137
#define IDS_TRASHITEM_TEXT          138
#define IDS_TRASHMULTIPLE_TEXT      139
#define IDS_CANTTRASH_TEXT          140
#define IDS_OVERWRITEFOLDER_TEXT    141
#define IDS_OPEN_WITH               142
#define IDS_OPEN_WITH_CHOOSE        143
#define IDS_SHELL_ABOUT_AUTHORS     144
#define IDS_SHELL_ABOUT_BACK        145
#define FCIDM_SHVIEW_NEW            146
#define FCIDM_SHVIEW_VIEW           147
#define IDS_CONTROLPANEL            148

/* Note: this string is referenced from the registry */
#define IDS_RECYCLEBIN_FOLDER_NAME   8964

#define IDD_ICON                    0x4300
#define IDD_MESSAGE                 0x4301

/* these IDs are the same as on native */
#define IDD_YESTOALL                0x3207

/* browse for folder dialog box */
#define IDD_MAKENEWFOLDER           0x3746
#define IDD_FOLDERTEXT              0x3745
#define IDD_FOLDER                  0x3744
#define IDD_STATUS                  0x3743
#define IDD_TITLE                   0x3742
#define IDD_TREEVIEW                0x3741
#define SHELL_EXTENDED_SHORTCUT_DLG 0x4000

/* ID's of the ShellAbout controls */
// Part 1 - ID's identical to Windows Server 2003 SP1's shell32.dll
#define IDD_SHELL_ABOUT                   0x3810
#define IDC_SHELL_ABOUT_ICON              0x3009
#define IDC_SHELL_ABOUT_APPNAME           0x3500
#define IDC_SHELL_ABOUT_OTHERSTUFF        0x350D
#define IDC_SHELL_ABOUT_REG_USERNAME      0x3507
#define IDC_SHELL_ABOUT_REG_ORGNAME       0x3508
#define IDC_SHELL_ABOUT_PHYSMEM           0x3503

// Part 2 - ReactOS-specific ID's
#define IDD_SHELL_ABOUT_AUTHORS           0x4100
#define IDC_SHELL_ABOUT_AUTHORS           0x4101
#define IDC_SHELL_ABOUT_AUTHORS_LISTBOX   0x4102

#define IDI_SHELL_DOCUMENT           1
#define IDI_SHELL_FOLDER             4
#define IDI_SHELL_FOLDER_OPEN        5
#define IDI_SHELL_5_12_FLOPPY        6
#define IDI_SHELL_3_14_FLOPPY        7
#define IDI_SHELL_FLOPPY             8
#define IDI_SHELL_DRIVE              9
#define IDI_SHELL_NETDRIVE          10
#define IDI_SHELL_NETDRIVE2         11
#define IDI_SHELL_CDROM             12
#define IDI_SHELL_RAMDISK           13
#define IDI_SHELL_ENTIRE_NETWORK    14
#define IDI_SHELL_NETWORK           15
#define IDI_SHELL_MY_COMPUTER       16
#define IDI_SHELL_PRINTER           17
#define IDI_SHELL_MY_NETWORK_PLACES 18
#define IDI_SHELL_COMPUTERS_NEAR_ME 19
#define IDI_SHELL_FOLDER_SMALL_XP   20
#define IDI_SHELL_SEARCH            23
#define IDI_SHELL_HELP              24
#define IDI_SHELL_FOLDER_OPEN_LARGE 29
#define IDI_SHELL_SHORTCUT          30
#define IDI_SHELL_FOLDER_OPEN_SMALL 31
#define IDI_SHELL_EMPTY_RECYCLE_BIN 32
#define IDI_SHELL_FULL_RECYCLE_BIN  33
#define IDI_SHELL_DESKTOP           35
#define IDI_SHELL_CONTROL_PANEL     36
#define IDI_SHELL_PRINTERS_FOLDER   38
#define IDI_SHELL_FONTS_FOLDER      39
#define IDI_SHELL_OPEN_WITH        135
#define IDI_SHELL_TRASH_FILE       142
#define IDI_SHELL_CONFIRM_DELETE   161
#define IDI_SHELL_MY_DOCUMENTS     235
#define IDI_SHELL_CONTROL_PANEL1   330
/*
AVI resources, windows shell32 has 14 of them: 150-152 and 160-170
FIXME: Need to add them, but for now just let them use the same: searching.avi
(also to limit shell32's size)
*/
#define IDR_AVI_SEARCH             150
#define IDR_AVI_SEARCHING          151
#define IDR_AVI_FINDCOMPUTER       152
#define IDR_AVI_FILEMOVE           160
#define IDR_AVI_FILECOPY           161
#define IDR_AVI_FILENUKE           163
#define IDR_AVI_FILEDELETE         164

#endif
