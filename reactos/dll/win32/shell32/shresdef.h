/*
 * Copyright 2000 Juergen Schmied
 * Copyright 2006 Ged Murphy
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

#ifndef __WINE_SHELL_RES_H
#define __WINE_SHELL_RES_H

/*
	columntitles for the shellview
*/
#define IDS_SHV_COLUMN1		7
#define IDS_SHV_COLUMN2		8
#define IDS_SHV_COLUMN3		9
#define IDS_SHV_COLUMN4		10
#define IDS_SHV_COLUMN5		11
#define IDS_SHV_COLUMN6		12
#define IDS_SHV_COLUMN7		13
#define IDS_SHV_COLUMN8		14
#define IDS_SHV_COLUMN9		15
#define IDS_SHV_COLUMN10	16
#define IDS_SHV_COLUMN11	17

#define IDS_DESKTOP		20
#define IDS_MYCOMPUTER		21

#define IDS_SELECT		22
#define IDS_OPEN		23
#define IDS_VIEW_LARGE		24
#define IDS_VIEW_SMALL		25
#define IDS_VIEW_LIST		26
#define IDS_VIEW_DETAILS	27

#define IDS_CREATEFOLDER_DENIED 30
#define IDS_CREATEFOLDER_CAPTION 31
#define IDS_DELETEITEM_CAPTION	32
#define IDS_DELETEFOLDER_CAPTION 33
#define IDS_DELETEITEM_TEXT	34
#define IDS_DELETEMULTIPLE_TEXT	35
#define IDS_OVERWRITEFILE_CAPTION 36
#define IDS_OVERWRITEFILE_TEXT	37

#define IDS_RESTART_TITLE      40
#define IDS_RESTART_PROMPT     41
#define IDS_SHUTDOWN_TITLE     42
#define IDS_SHUTDOWN_PROMPT    43

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

/* browse for folder dialog box */
#define IDD_STATUS		0x3743
#define IDD_TITLE		0x3742
#define IDD_TREEVIEW		0x3741


/* 
 * Do not alter the icon, bitmap + avi resource 
 * numbers they sync with the Windows counterpart
 */
/* ICONS */
#define IDI_SHELL_DOCUMENT          1
#define IDI_SHELL_RICH_TEXT         2
#define IDI_SHELL_EXE               3
#define IDI_SHELL_FOLDER            4
#define IDI_SHELL_FOLDER_OPEN       5
#define IDI_SHELL_5_12_FLOPPY       6
#define IDI_SHELL_3_14_FLOPPY       7
#define IDI_SHELL_FLOPPY            8
#define IDI_SHELL_DRIVE             9
#define IDI_SHELL_NETDRIVE          10
#define IDI_SHELL_NETDRIVE2         11  //THIS SHOULD BE #define IDI_SHELL_NETDRIVE_OFF
#define IDI_SHELL_CDROM             12
#define IDI_SHELL_RAMDISK           13

#define IDI_SHELL_NETWORK           15
#define IDI_SHELL_MY_COMPUTER       16
#define IDI_SHELL_PRINTER           17
#define IDI_SHELL_MY_NETWORK_PLACES 18
#define IDI_SHELL_COMPUTERS_NEAR_ME 19
#define IDI_SHELL_PROGRAMS_FOLDER   20
#define IDI_SHELL_RECENT_DOCUMENTS  21
#define IDI_SHELL_CONTROL_PANEL     22
#define IDI_SHELL_SEARCH            23
#define IDI_SHELL_HELP              24
#define IDI_SHELL_RUN               25
#define IDI_SHELL_SHUTDOWN          28
#define IDI_SHELL_SHARE             29
#define IDI_SHELL_SHORTCUT          30
#define IDI_SHELL_EMPTY_RECYCLE_BIN 32
#define IDI_SHELL_FULL_RECYCLE_BIN  33
#define IDI_SHELL_DESKTOP           35
#define IDI_SHELL_CONTROL_PANEL2    36
#define IDI_SHELL_PROGRAMS_FOLDER2  37
#define IDI_SHELL_PRINTERS_FOLDER   38
#define IDI_SHELL_FONTS_FOLDER      39
#define IDI_SHELL_TSKBAR_STARTMENU  40
#define IDI_SHELL_CD_MUSIC          41
#define IDI_SHELL_FAVORITES         44
#define IDI_SHELL_LOGOFF            45
#define IDI_SHELL_EXPLORER          46

#define IDI_SHELL_LOCKED            48

#define IDI_SHELL_FIND_IN_FILE      134

#define IDI_SHELL_CONTROL_PANEL3    137
#define IDI_SHELL_PRINTER2          138
#define IDI_SHELL_INF_FILE          151
#define IDI_SHELL_TEXT_FILE         152
#define IDI_SHELL_BAT_FILE          153
#define IDI_SHELL_SYSTEM_FILE       154
#define IDI_SHELL_FONT_FILE         155
#define IDI_SHELL_TT_FONT_FILE      156
#define IDI_SHELL_FONT_FILE2        157
#define IDI_SHELL_RUN2              160

#define IDI_SHELL_NETWORK_FOLDER    172

#define IDI_SHELL_EMPTY_RECYCLE_BIN1 191
#define IDI_SHELL_FULL_RECYCLE_BIN1 192


#define IDI_SHELL_TURN_OFF          221
#define IDI_SHELL_DVD_ROM           222
#define IDI_SHELL_MOVIE_FILE        224
#define IDI_SHELL_MUSIC_FILE        225

#define IDI_SHELL_CD_MUSIC2         228

#define IDI_SHELL_MY_DOCUMENTS      235
#define IDI_SHELL_MY_PICTURES       236
#define IDI_SHELL_MY_MUSIC          237
#define IDI_SHELL_MY_MOVIES         238

#define IDI_SHELL_PRINTER3          245

#define IDI_SHELL_CAMERA            248
#define IDI_SHELL_OVER_HEAD_PROJ    249
#define IDI_SHELL_DISPLAY           250
#define IDI_SHELL_PRINT_PICS        252
#define IDI_SHELL_EMPTY_RECYCLE_BIN3 254

#define IDI_SHELL_HELP1             263
#define IDI_SHELL_SENDMAIL          265
#define IDI_SHELL_ACCESSABILITY     268
#define IDI_SHELL_USERS             269
#define IDI_SHELL_SCREEN_COLORS     270
#define IDI_SHELL_ADD_REM_PROGRAMS  271
#define IDI_SHELL_TUNES             277
#define IDI_SHELL_USER_ACCOUNTS     279
#define IDI_SHELL_HELP_FILE         289
#define IDI_SHELL_GO                290
#define IDI_SHELL_DVD_DRIVE         291
#define IDI_SHELL_CD_ADD_MUSIC      292
#define IDI_SHELL_CD                293
#define IDI_SHELL_CD_ROM            294
#define IDI_SHELL_CDR               295
#define IDI_SHELL_CDRW              296
#define IDI_SHELL_DVD_RAM           297
#define IDI_SHELL_DVDR_ROM          298
#define IDI_SHELL_MP3_PLAYER        299
#define IDI_SHELL_CD_ROM1           302
#define IDI_SHELL_DVD_ROM1          304

#define IDI_SHELL_CAMERA1           309
#define IDI_SHELL_SCANNER           315
#define IDI_SHELL_CAMCORDER         317
#define IDI_SHELL_DVDRW_ROM         318
#define IDI_SHELL_NEW_FOLDER        319
#define IDI_SHELL_FAVOTITES         322
#define IDI_SHELL_SEARCH1           323
#define IDI_SHELL_HELP2             324
#define IDI_SHELL_LOGOFF1           325
#define IDI_SHELL_PROGRAMS_FOLDER1  326
#define IDI_SHELL_RECENT_DOCUMENTS1 327
#define IDI_SHELL_RUN1              328
#define IDI_SHELL_SHUTDOWN1         329
#define IDI_SHELL_CONTROL_PANEL1    330
#define IDI_SHELL_IDEA              1001
#define IDI_SHELL_HELP_FILE1        1004
#define IDI_SHELL_SHUTDOWN2         8240

/* BITMAPS */
#define IDB_SHELL_IEXPLORE_LG       204
#define IDB_SHELL_IEXPLORE1_LG      205
#define IDB_SHELL_IEXPLORE_SM       206
#define IDB_SHELL_IEXPLORE1_SM      207
#define IDB_SHELL_EXPLORER_LG       214
#define IDB_SHELL_EXPLORER1_LG      215
#define IDB_SHELL_EXPLORER_SM       216
#define IDB_SHELL_EXPLORER1_SM      217
#define IDB_SHELL_UNKNOWN1          225
#define IDB_SHELL_UNKNOWN2          226
#define IDB_SHELL_UNKNOWN3          227
#define IDB_SHELL_UNKNOWN4          228
#define IDB_SHELL_UNKNOWN5          230
#define IDB_SHELL_UNKNOWN6          231
#define IDB_SHELL_UNKNOWN7          245

/* AVI */
#define IDA_SHELL_COPY              160
#define IDA_SHELL_COPY1             161
#define IDA_SHELL_COPY2             167
#define IDA_SHELL_COPY3             168
#define IDA_SHELL_RECYCLE           162
#define IDA_SHELL_EMPTY_RECYCLE     163
#define IDA_SHELL_DELETE            164
#define IDA_SHELL_DELETE1           169
#define IDA_SHELL_DOWNLOAD          170

#endif
