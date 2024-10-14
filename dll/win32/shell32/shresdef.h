/*
 * Copyright 2000 Juergen Schmied
 * Copyright 2017 Katayama Hirofumi MZ
 * Copyright 2021 Arnav Bhatt
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

#define IDC_STATIC -1

/* Accelerators */
#define IDA_SHELLVIEW 1
#define IDA_DESKBROWSER 3

/* Bitmaps */
#define IDB_REACTOS                 131
#define IDB_LINEBAR                 138
#define IDB_SHELL_IEXPLORE_LG       204
#define IDB_SHELL_IEXPLORE_LG_HOT   205
#define IDB_SHELL_IEXPLORE_SM       206
#define IDB_SHELL_IEXPLORE_SM_HOT   207
#define IDB_SHELL_EDIT_LG           225
#define IDB_SHELL_EDIT_LG_HOT       226
#define IDB_SHELL_EDIT_SM           227
#define IDB_SHELL_EDIT_SM_HOT       228

/* Bitmaps for fancy log off dialog box */
#define IDB_DLG_BG                  500
#define IDB_REACTOS_FLAG            501
#define IDB_IMAGE_STRIP             502

/* Strings */

/* Column titles for the shellview */
#define IDS_SHV_COLUMN_NAME        7
#define IDS_SHV_COLUMN_SIZE        8
#define IDS_SHV_COLUMN_TYPE        9
#define IDS_SHV_COLUMN_MODIFIED        10
#define IDS_SHV_COLUMN_ATTRIBUTES        11
#define IDS_SHV_COLUMN_DISK_CAPACITY        12
#define IDS_SHV_COLUMN_DISK_AVAILABLE        13
#define IDS_SHV_COLUMN_OWNER       16
#define IDS_SHV_COLUMN_GROUP       17
#define IDS_SHV_COLUMN_DELFROM 18
#define IDS_SHV_COLUMN_DELDATE 19
#define IDS_SHV_COLUMN_FONTTYPE  311
#define IDS_SHV_COLUMN_FILENAME         312
#define IDS_SHV_COLUMN_CATEGORY         313
#define IDS_SHV_COLUMN_WORKGROUP 314
#define IDS_SHV_COLUMN_NETLOCATION  315
#define IDS_SHV_COLUMN_DOCUMENTS 319
#define IDS_SHV_COLUMN_STATUS    320
#define IDS_SHV_COLUMN_COMMENTS  321
#define IDS_SHV_COLUMN_LOCATION  322
#define IDS_SHV_COLUMN_MODEL     323

#define IDS_DESKTOP 20

#define IDS_SELECT       22
#define IDS_OPEN         23
#define IDS_VIEW_LARGE   24
#define IDS_VIEW_SMALL   25
#define IDS_VIEW_LIST    26
#define IDS_VIEW_DETAILS 27

#define IDS_RESTART_TITLE   40
#define IDS_RESTART_PROMPT  41
#define IDS_SHUTDOWN_TITLE  42
#define IDS_SHUTDOWN_PROMPT 43

#define IDS_PROGRAMS             45
#define IDS_FAVORITES            47
#define IDS_STARTUP              48
#define IDS_RECENT               49
#define IDS_SENDTO               50
#define IDS_STARTMENU            51
#define IDS_MYMUSIC              52
#define IDS_MYVIDEO              53
#define IDS_DESKTOPDIRECTORY     54
#define IDS_NETHOOD              55
#define IDS_TEMPLATES            56
#define IDS_APPDATA              57
#define IDS_PRINTHOOD            58
#define IDS_LOCAL_APPDATA        59
#define IDS_INTERNET_CACHE       60
#define IDS_COOKIES              61
#define IDS_HISTORY              62
#define IDS_PROGRAM_FILES        63
#define IDS_MYPICTURES           64
#define IDS_PROGRAM_FILES_COMMON 65
#define IDS_COMMON_DOCUMENTS     66
#define IDS_ADMINTOOLS           67
#define IDS_COMMON_MUSIC         68
#define IDS_COMMON_PICTURES      69
#define IDS_COMMON_VIDEO         70
#define IDS_CDBURN_AREA          71
#define IDS_DRIVE_FIXED          72
#define IDS_DRIVE_CDROM          73
#define IDS_DRIVE_NETWORK        74
#define IDS_DRIVE_FLOPPY         75
#define IDS_DRIVE_REMOVABLE      76
#define IDS_FS_UNKNOWN           77

#define IDS_CREATEFILE_CAPTION    126
#define IDS_CREATEFILE_DENIED     127
#define IDS_CREATEFOLDER_DENIED   128
#define IDS_CREATEFOLDER_CAPTION  129
#define IDS_DELETEITEM_CAPTION    130
#define IDS_DELETEFOLDER_CAPTION  131
#define IDS_DELETEITEM_TEXT       132
#define IDS_DELETEMULTIPLE_TEXT   133
#define IDS_OVERWRITEFILE_CAPTION 134
#define IDS_OVERWRITEFILE_TEXT    135
#define IDS_DELETESELECTED_TEXT   136
#define IDS_TRASHFOLDER_TEXT      137
#define IDS_TRASHITEM_TEXT        138
#define IDS_TRASHMULTIPLE_TEXT    139
#define IDS_CANTTRASH_TEXT        140
#define IDS_OVERWRITEFOLDER_TEXT  141
#define IDS_OPEN_WITH             142
#define IDS_OPEN_WITH_CHOOSE      143
#define IDS_SHELL_ABOUT_AUTHORS   144
#define IDS_SHELL_ABOUT_BACK      145
#define FCIDM_SHVIEW_NEW          146
#define IDS_CONTROLPANEL          148
#define IDS_NEWFOLDER             149
#define IDS_NEWITEMFORMAT         150
#define IDS_COLUMN_EXTENSION      151
#define IDS_NO_EXTENSION          152
#define IDS_RECYCLEBIN_LOCATION   153
#define IDS_RECYCLEBIN_DISKSPACE  154
#define IDS_OPEN_WITH_FILTER      155
#define IDS_CANTLOCKVOLUME        156
#define IDS_CANTDISMOUNTVOLUME    157
#define IDS_CANTEJECTMEDIA        158
#define IDS_CANTSHOWPROPERTIES    159
#define IDS_CANTDISCONNECT        160
#define IDS_NONE                  161

#define IDS_EXPAND                170
#define IDS_COLLAPSE              171

/* Friendly File Type Names */
#define IDS_DIRECTORY             200
#define IDS_BAT_FILE              201
#define IDS_CMD_FILE              202
#define IDS_COM_FILE              203
#define IDS_CPL_FILE              204
#define IDS_CUR_FILE              205
#define IDS_DB__FILE              220
#define IDS_DLL_FILE              206
#define IDS_DRV_FILE              207
#define IDS_EFI_FILE              221
#define IDS_EXE_FILE              208
#define IDS_NLS_FILE              222
#define IDS_OCX_FILE              223
#define IDS_TLB_FILE              224
#define IDS_FON_FILE              209
#define IDS_TTF_FILE              210
#define IDS_OTF_FILE              211
#define IDS_HLP_FILE              212
#define IDS_ICO_FILE              213
#define IDS_INI_FILE              214
#define IDS_LNK_FILE              215
#define IDS_NT__FILE              225
#define IDS_PIF_FILE              216
#define IDS_SCR_FILE              217
#define IDS_SYS_FILE              218
#define IDS_VXD_FILE              219
#define IDS_ANY_FILE              299

#define IDS_EMPTY_BITBUCKET       172
#define IDS_SHLEXEC_NOASSOC       173
#define IDS_FILE_TYPES            174
#define IDS_FILE_DETAILS          175
#define IDS_FILE_DETAILSADV       176
#define IDS_BYTES_FORMAT          177
#define IDS_OPEN_WITH_RECOMMENDED 178
#define IDS_OPEN_WITH_OTHER       179

#define IDS_RUNDLG_ERROR          180
#define IDS_RUNDLG_BROWSE_ERROR   181
#define IDS_RUNDLG_BROWSE_CAPTION 182
#define IDS_RUNDLG_BROWSE_FILTER  183

/* Format Dialog strings */
#define IDS_FORMAT_TITLE          184
#define IDS_FORMAT_WARNING        185
#define IDS_FORMAT_COMPLETE       186

/* Warning format system drive dialog strings */
#define IDS_NO_FORMAT_TITLE       188
#define IDS_NO_FORMAT             189

#define IDS_UNKNOWN_APP     190
#define IDS_EXE_DESCRIPTION 191

#define IDS_OPEN_VERB    300
#define IDS_EXPLORE_VERB 301
#define IDS_RUNAS_VERB   302
#define IDS_EDIT_VERB    303
#define IDS_FIND_VERB    304
#define IDS_PRINT_VERB   305
#define IDS_CMD_VERB     306

#define IDS_FILE_FOLDER          308
#define IDS_CREATELINK           309
#define IDS_INSTALLNEWFONT       310
#define IDS_COPY                 316
#define IDS_DELETE               317
#define IDS_PROPERTIES           318
#define IDS_CUT                  324
#define IDS_RESTORE              325
#define IDS_DEFAULT_CLUSTER_SIZE 326
#define IDS_FORMATDRIVE          328
#define IDS_RENAME               329
#define IDS_PASTE                330
#define IDS_DESCRIPTION          331
#define IDS_COPY_OF              332

/* Strings for file operations */
#define IDS_FILEOOP_COPYING      333
#define IDS_FILEOOP_MOVING       334
#define IDS_FILEOOP_DELETING     335
#define IDS_FILEOOP_FROM_TO      336
#define IDS_FILEOOP_FROM         337
#define IDS_FILEOOP_PREFLIGHT    338

#define IDS_EJECT                339
#define IDS_DISCONNECT           340

#define IDS_OPENFILELOCATION     341
#define IDS_SENDTO_MENU          343
#define IDS_COPYASPATHMENU       30328

#define IDS_MOVEERRORTITLE       344
#define IDS_COPYERRORTITLE       345
#define IDS_MOVEERRORSAMEFOLDER  346
#define IDS_MOVEERRORSAME        347
#define IDS_COPYERRORSAME        348
#define IDS_MOVEERRORSUBFOLDER   349
#define IDS_COPYERRORSUBFOLDER   350
#define IDS_MOVEERROR            351
#define IDS_COPYERROR            352

/* Shortcut property sheet */
#define IDS_SHORTCUT_RUN_NORMAL  4167
#define IDS_SHORTCUT_RUN_MIN     4168
#define IDS_SHORTCUT_RUN_MAX     4169

#define IDS_MENU_EMPTY           34561

/* Note: those strings are referenced from the registry */
#define IDS_RECYCLEBIN_FOLDER_NAME 8964
#define IDS_PRINTERS_DESCRIPTION   12696
#define IDS_FONTS_DESCRIPTION      22920
#define IDS_ADMINISTRATIVETOOLS_DESCRIPTION 22921
#define IDS_FOLDER_OPTIONS_DESCRIPTION 22924
#define IDS_TASKBAR_OPTIONS_INFOTIP 30348
#define IDS_ADMINISTRATIVETOOLS    22982
#define IDS_FOLDER_OPTIONS         22985
#define IDS_FONTS                  22981
#define IDS_TASKBAR_OPTIONS        32517
#define IDS_PRINTERS               9319
#define IDS_MYCOMPUTER             9216
#define IDS_PERSONAL               9227
#define IDS_NETWORKPLACE           9217
#define IDS_OBJECTS                6466
#define IDS_OBJECTS_SELECTED       6477

/* Desktop icon titles */
#define IDS_TITLE_MYCOMP                            30386
#define IDS_TITLE_MYNET                             30387
#define IDS_TITLE_BIN_1                             30388
#define IDS_TITLE_BIN_0                             30389

/* Advanced settings */
#define IDS_ADVANCED_FOLDER                         30498
#define IDS_ADVANCED_NET_CRAWLER                    30509
#define IDS_ADVANCED_FOLDER_SIZE_TIP                30514
#define IDS_ADVANCED_FRIENDLY_TREE                  30511
#define IDS_ADVANCED_WEB_VIEW_BARRICADE             30510
#define IDS_ADVANCED_SHOW_FULL_PATH_ADDRESS         30505
#define IDS_ADVANCED_SHOW_FULL_PATH                 30504
#define IDS_ADVANCED_DISABLE_THUMB_CACHE            30517
#define IDS_ADVANCED_HIDDEN                         30499
#define IDS_ADVANCED_DONT_SHOW_HIDDEN               30501
#define IDS_ADVANCED_SHOW_HIDDEN                    30500
#define IDS_ADVANCED_HIDE_FILE_EXT                  30503
#define IDS_ADVANCED_SUPER_HIDDEN                   30508
#define IDS_ADVANCED_DESKTOP_PROCESS                30507
#define IDS_ADVANCED_CLASSIC_VIEW_STATE             30506
#define IDS_ADVANCED_PERSIST_BROWSERS               30513
#define IDS_ADVANCED_CONTROL_PANEL_IN_MY_COMPUTER   30497
#define IDS_ADVANCED_SHOW_COMP_COLOR                30512
#define IDS_ADVANCED_SHOW_INFO_TIP                  30502

/* These values must be synchronized with explorer */
#define IDS_ADVANCED_DISPLAY_FAVORITES              30466
#define IDS_ADVANCED_DISPLAY_LOG_OFF                30467
#define IDS_ADVANCED_EXPAND_CONTROL_PANEL           30468
#define IDS_ADVANCED_EXPAND_MY_DOCUMENTS            30469
#define IDS_ADVANCED_EXPAND_PRINTERS                30470
#define IDS_ADVANCED_EXPAND_MY_PICTURES             30472
#define IDS_ADVANCED_EXPAND_NET_CONNECTIONS         30473
#define IDS_ADVANCED_DISPLAY_RUN                    30474
#define IDS_ADVANCED_DISPLAY_ADMINTOOLS             30476
#define IDS_ADVANCED_SMALL_START_MENU               30477

#define IDS_NEWEXT_ADVANCED_LEFT                    30515
#define IDS_NEWEXT_ADVANCED_RIGHT                   30516
#define IDS_NEWEXT_NEW                              30518
#define IDS_NEWEXT_SPECIFY_EXT                      30519
#define IDS_NEWEXT_ALREADY_ASSOC                    30520
#define IDS_NEWEXT_EXT_IN_USE                       30521

#define IDS_SPECIFY_ACTION                          30523
#define IDS_INVALID_PROGRAM                         30524
#define IDS_REMOVE_ACTION                           30525
#define IDS_ACTION_EXISTS                           30526
#define IDS_EXE_FILTER                              30527
#define IDS_EDITING_ACTION                          30528

#define IDS_REMOVE_EXT                              30522

#define IDS_NO_ICONS                                30529
#define IDS_FILE_NOT_FOUND                          30530
#define IDS_LINK_INVALID                            30531
#define IDS_COPYTOMENU                              30532
#define IDS_COPYTOTITLE                             30533
#define IDS_COPYITEMS                               30534
#define IDS_COPYBUTTON                              30535
#define IDS_MOVETOMENU                              30536
#define IDS_MOVETOTITLE                             30537
#define IDS_MOVEITEMS                               30538
#define IDS_MOVEBUTTON                              30539

#define IDS_SYSTEMFOLDER                            30540

#define IDS_LOG_OFF_DESC                            35000
#define IDS_SWITCH_USER_DESC                        35001
#define IDS_LOG_OFF_TITLE                           35010
#define IDS_SWITCH_USER_TITLE                       35011

/* Dialogs */

/* Run dialog */
#define IDD_RUN                1
#define IDC_RUNDLG_DESCRIPTION 12289
#define IDC_RUNDLG_BROWSE      12288
#define IDC_RUNDLG_ICON        12297
#define IDC_RUNDLG_EDITPATH    12298
#define IDC_RUNDLG_LABEL       12305

/* ShellAbout dialog */
#define IDD_ABOUT                     2
#define IDC_ABOUT_ICON                0x3009
#define IDC_ABOUT_APPNAME             0x3500
#define IDS_ABOUT_VERSION_STRING      0x3501
#define IDC_ABOUT_VERSION             0x3502
#define IDC_ABOUT_OTHERSTUFF          0x350D
#define IDC_ABOUT_REG_TO              0x3506
#define IDC_ABOUT_REG_USERNAME        0x3507
#define IDC_ABOUT_REG_ORGNAME         0x3508
#define IDC_ABOUT_PHYSMEM             0x3503

/* About authors dialog */
#define IDD_ABOUT_AUTHORS         3
#define IDC_ABOUT_AUTHORS         0x4101
#define IDC_ABOUT_AUTHORS_LISTBOX 0x4102

/* Pick icon dialog */
#define IDD_PICK_ICON        4
#define IDC_PICKICON_LIST    0x4121
#define IDC_BUTTON_PATH      0x4122
#define IDC_EDIT_PATH        0x4123
#define IDS_PICK_ICON_TITLE  0x4124
#define IDS_PICK_ICON_FILTER 0x4125

/* Properties dialog */
#define IDD_FILE_PROPERTIES        8
#define IDD_FOLDER_PROPERTIES      9
#define IDD_DRIVE_PROPERTIES       10
#define IDD_DRIVE_TOOLS            11
#define IDD_DRIVE_HARDWARE         12
#define IDD_RECYCLE_BIN_PROPERTIES 13
#define IDD_SHORTCUT_PROPERTIES    14

/* File version */
#define IDD_FILE_VERSION 15

/* Shortcut */
#define IDD_SHORTCUT_EXTENDED_PROPERTIES 16

/* Folder Options */
#define IDD_FOLDER_OPTIONS_GENERAL         17
#define IDD_FOLDER_OPTIONS_VIEW            18
#define IDD_FOLDER_OPTIONS_FILETYPES       19
#define IDC_FOLDER_OPTIONS_TASKICON        30109
#define IDC_FOLDER_OPTIONS_FOLDERICON      30110
#define IDC_FOLDER_OPTIONS_CLICKICON       30111
#define IDC_FOLDER_OPTIONS_COMMONTASKS     14001
#define IDC_FOLDER_OPTIONS_CLASSICFOLDERS  14002
#define IDC_FOLDER_OPTIONS_SAMEWINDOW      14004
#define IDC_FOLDER_OPTIONS_OWNWINDOW       14005
#define IDC_FOLDER_OPTIONS_SINGLECLICK     14007
#define IDC_FOLDER_OPTIONS_DOUBLECLICK     14008
#define IDC_FOLDER_OPTIONS_ULBROWSER       14009
#define IDC_FOLDER_OPTIONS_ULPOINT         14010
#define IDC_FOLDER_OPTIONS_RESTORE         14011

/* Yes to all msgbox */
#define IDD_YESTOALL_MSGBOX  20
#define IDC_YESTOALL         0x3207
#define IDC_YESTOALL_ICON    0x4300
#define IDC_YESTOALL_MESSAGE 0x4301

/* Browse for folder dialog box */
#define IDD_BROWSE_FOR_FOLDER             21
#define IDD_BROWSE_FOR_FOLDER_NEW         22
#define IDC_BROWSE_FOR_FOLDER_NEW_FOLDER  0x3746
#define IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT 0x3745
#define IDC_BROWSE_FOR_FOLDER_FOLDER      0x3744
#define IDC_BROWSE_FOR_FOLDER_STATUS      0x3743
#define IDC_BROWSE_FOR_FOLDER_TITLE       0x3742
#define IDC_BROWSE_FOR_FOLDER_TREEVIEW    0x3741

/* Control IDs for IDD_FOLDER_OPTIONS_FILETYPES dialog */
#define IDC_FILETYPES_LISTVIEW              14000
#define IDC_FILETYPES_NEW                   14001
#define IDC_FILETYPES_DELETE                14002
#define IDC_FILETYPES_DETAILS_GROUPBOX      14003
#define IDC_FILETYPES_APPNAME               14005
#define IDC_FILETYPES_CHANGE                14006
#define IDC_FILETYPES_DESCRIPTION           14007
#define IDC_FILETYPES_ADVANCED              14008
#define IDC_FILETYPES_ICON                  14009

/* Control IDs for IDD_NEWEXTENSION dialog */
#define IDC_NEWEXT_EDIT                     14001
#define IDC_NEWEXT_ADVANCED                 14002
#define IDC_NEWEXT_COMBOBOX                 14003
#define IDC_NEWEXT_ASSOC                    14004

/* Control IDs for IDD_SHORTCUT_PROPERTIES dialog */
#define IDC_SHORTCUT_ICON                   14000
#define IDC_SHORTCUT_TEXT                   14001
#define IDC_SHORTCUT_TYPE                   14004
#define IDC_SHORTCUT_TYPE_EDIT              14005
#define IDC_SHORTCUT_LOCATION               14006
#define IDC_SHORTCUT_LOCATION_EDIT          14007
#define IDC_SHORTCUT_TARGET                 14008
#define IDC_SHORTCUT_TARGET_TEXT            14009
#define IDC_SHORTCUT_START_IN               14010
#define IDC_SHORTCUT_START_IN_EDIT          14011
#define IDC_SHORTCUT_KEY                    14014
#define IDC_SHORTCUT_KEY_HOTKEY             14015
#define IDC_SHORTCUT_RUN                    14016
#define IDC_SHORTCUT_RUN_COMBO              14017
#define IDC_SHORTCUT_COMMENT                14018
#define IDC_SHORTCUT_COMMENT_EDIT           14019
#define IDC_SHORTCUT_FIND                   14020
#define IDC_SHORTCUT_CHANGE_ICON            14021
#define IDC_SHORTCUT_ADVANCED               14022

/* Control IDs for IDD_SHORTCUT_EXTENDED_PROPERTIES dialog */
#define IDC_SHORTEX_RUN_DIFFERENT           14000
#define IDC_SHORTEX_RUN_SEPARATE            14001

/* Control IDs for IDD_EDITTYPE dialog */
#define IDC_EDITTYPE_ICON                   14001
#define IDC_EDITTYPE_TEXT                   14002
#define IDC_EDITTYPE_CHANGE_ICON            14003
#define IDC_EDITTYPE_LISTBOX                14004
#define IDC_EDITTYPE_NEW                    14005
#define IDC_EDITTYPE_EDIT_BUTTON            14006
#define IDC_EDITTYPE_REMOVE                 14007
#define IDC_EDITTYPE_SET_DEFAULT            14008
#define IDC_EDITTYPE_CONFIRM_OPEN           14009
#define IDC_EDITTYPE_SHOW_EXT               14010
#define IDC_EDITTYPE_SAME_WINDOW            14011

/* Control IDs for IDD_ACTION dialog */
#define IDC_ACTION_ACTION                   14001
#define IDC_ACTION_APP                      14002
#define IDC_ACTION_BROWSE                   14003
#define IDC_ACTION_USE_DDE                  14004

/* Control IDs for IDD_FOLDER_OPTIONS_VIEW dialog */
#define IDC_VIEW_APPLY_TO_ALL               14001
#define IDC_VIEW_RESET_ALL                  14002
#define IDC_VIEW_TREEVIEW                   14003
#define IDC_VIEW_RESTORE_DEFAULTS           14004

/* Control IDs for IDD_LOG_OFF_FANCY dialog */
#define IDC_LOG_OFF_BUTTON                  15001
#define IDC_SWITCH_USER_BUTTON              15002
#define IDC_LOG_OFF_STATIC                  15003
#define IDC_SWITCH_USER_STATIC              15004
#define IDC_LOG_OFF_TEXT_STATIC             15005

/* Other dialogs */
#define IDD_RUN_AS       23
#define IDD_OPEN_WITH    24
#define IDD_FORMAT_DRIVE 25
#define IDD_CHECK_DISK   26
#define IDD_NOOPEN       27
#define IDD_NEWEXTENSION 28
#define IDD_EDITTYPE     36
#define IDD_ACTION       37
#define IDD_FOLDER_CUSTOMIZE    38
#define IDD_LINK_PROBLEM 39

/* Control IDs for IDD_FOLDER_CUSTOMIZE dialog */
#define IDC_FOLDERCUST_COMBOBOX             14001
#define IDC_FOLDERCUST_CHECKBOX             14002
#define IDC_FOLDERCUST_CHOOSE_PIC           14003
#define IDC_FOLDERCUST_RESTORE_DEFAULTS     14004
#define IDC_FOLDERCUST_PREVIEW_BITMAP       14005
#define IDC_FOLDERCUST_ICON                 14006
#define IDC_FOLDERCUST_CHANGE_ICON          14007

/* Control IDs for IDD_LINK_PROBLEM dialog */
#define IDC_LINK_PROBLEM_ICON               14008
#define IDC_LINK_PROBLEM_LABEL1             14009
#define IDC_LINK_PROBLEM_LABEL2             14010

/* Not used dialogs */
#define IDD_SHUTDOWN             29
#define IDD_LOG_OFF              30
#define IDD_DISCONNECT           31
#define IDD_CONFIRM_FILE_REPLACE 32
#define IDD_AUTOPLAY1            33
#define IDD_MIXED_CONTENT1       34
#define IDD_MIXED_CONTENT2       35
#define IDD_LOG_OFF_FANCY        600

/* Icons */
#define IDI_SHELL_DOCUMENT           1
#define IDI_SHELL_RICH_TEXT          2
#define IDI_SHELL_EXE                3
#define IDI_SHELL_FOLDER             4
#define IDI_SHELL_FOLDER_OPEN        5
#define IDI_SHELL_5_12_FLOPPY        6
#define IDI_SHELL_3_14_FLOPPY        7
#define IDI_SHELL_REMOVEABLE         8
#define IDI_SHELL_DRIVE              9
#define IDI_SHELL_NETDRIVE          10
#define IDI_SHELL_NETDRIVE_OFF      11
#define IDI_SHELL_CDROM             12
#define IDI_SHELL_RAMDISK           13
#define IDI_SHELL_ENTIRE_NETWORK    14
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
#define IDI_SHELL_SLEEP             26
#define IDI_SHELL_HARDWARE_REMOVE   27
#define IDI_SHELL_SHUTDOWN          28
#define IDI_SHELL_SHARE             29
#define IDI_SHELL_SHORTCUT          30
#define IDI_SHELL_FOLDER_WAIT       31
#define IDI_SHELL_EMPTY_RECYCLE_BIN 32
#define IDI_SHELL_FULL_RECYCLE_BIN  33
#define IDI_SHELL_DIALUP_FOLDER     34
#define IDI_SHELL_DESKTOP           35
#define IDI_SHELL_CONTROL_PANEL2    36
#define IDI_SHELL_PROGRAMS_FOLDER2  37
#define IDI_SHELL_PRINTERS_FOLDER   38
#define IDI_SHELL_FONTS_FOLDER      39
#define IDI_SHELL_TSKBAR_STARTMENU  40
#define IDI_SHELL_CD_MUSIC          41
#define IDI_SHELL_TREE              42
#define IDI_SHELL_COMPUTER_FOLDER   43
#define IDI_SHELL_FAVORITES         44
#define IDI_SHELL_LOGOFF            45
#define IDI_SHELL_EXPLORER          46
#define IDI_SHELL_UPDATE            47
#define IDI_SHELL_LOCKED            48
#define IDI_SHELL_DISCONN           49
#define IDI_SHELL_NONE_50           50
#define IDI_SHELL_NONE_51           51
#define IDI_SHELL_NONE_52           52
#define IDI_SHELL_NONE_53           53
#define IDI_SHELL_NOT_CONNECTED_HDD 54
#define IDI_SHELL_MULTIPLE_FILES   133
#define IDI_SHELL_OPEN_WITH        134
#define IDI_SHELL_FIND_COMPUTER    135
#define IDI_SHELL_CONTROL_PANEL3   137
#define IDI_SHELL_PRINTER2         138
#define IDI_SHELL_PRINTER_ADD      139
#define IDI_SHELL_NET_PRINTER      140
#define IDI_SHELL_FILE_PRINTER     141
#define IDI_SHELL_TRASH_FILE       142
#define IDI_SHELL_TRASH_FOLDER     143
#define IDI_SHELL_TRASH_FILE_FOLDER 144
#define IDI_SHELL_FILE_COPY_DELTE  145
#define IDI_SHELL_FOLDER_MOVE1     146
#define IDI_SHELL_FOLDER_RENAME    147
#define IDI_SHELL_SETTINGS_FOLDER  148
#define IDI_SHELL_INF_FILE         151
#define IDI_SHELL_TEXT_FILE        152
#define IDI_SHELL_BAT_FILE         153
#define IDI_SHELL_SYSTEM_FILE      154
#define IDI_SHELL_FONT_FILE        155
#define IDI_SHELL_TT_FONT_FILE     156
#define IDI_SHELL_FONT_FILE2       157
#define IDI_SHELL_RUN2             160
#define IDI_SHELL_CONFIRM_DELETE   161
#define IDI_SHELL_TOOLS_BACKUP     165
#define IDI_SHELL_TOOLS_CHKDSK     166
#define IDI_SHELL_TOOLS_DEFRAG     167
#define IDI_SHELL_PRINTER_OK       168
#define IDI_SHELL_NET_PRINTER_OK   169
#define IDI_SHELL_FILE_PRINTER_OK  170
#define IDI_SHELL_CHART            171
#define IDI_SHELL_NETWORK_FOLDER   172
#define IDI_SHELL_FAVORITES2       173
#define IDI_SHELL_EXTENDED_PROPERTIES 174
#define IDI_SHELL_NETWORK_CONNECTIONS 175
#define IDI_SHELL_NETWORK_CHART    176
#define IDI_SHELL_VIEW_SETTINGS    177
#define IDI_SHELL_INTERNET_CHART   178
#define IDI_SHELL_COMPUTER_SYNC    179
#define IDI_SHELL_COMPUTER_WINDOW  180
#define IDI_SHELL_COMPUTER_DESKTOP 181

/* Folder Options, General dialog */
#define IDI_SHELL_SHOW_COMMON_TASKS    182
#define IDI_SHELL_CLASSIC_FOLDERS      183
#define IDI_SHELL_OPEN_IN_SOME_WINDOW  184
#define IDI_SHELL_OPEN_IN_NEW_WINDOW   185
#define IDI_SHELL_SINGLE_CLICK_TO_OPEN 186
#define IDI_SHELL_DOUBLE_CLICK_TO_OPEN 187

#define IDI_SHELL_EMPTY_RECYCLE_BIN1 191
#define IDI_SHELL_FULL_RECYCLE_BIN1  192
#define IDI_SHELL_WEB_FOLDERS      193
#define IDI_SHELL_SECURITY         194
#define IDI_SHELL_FAX              196
#define IDI_SHELL_FAX_OK           197
#define IDI_SHELL_NET_FAX_OK       198
#define IDI_SHELL_NET_FAX          199
#define IDI_SHELL_NO               200
#define IDI_SHELL_FOLDER_OPTIONS   210
#define IDI_SHELL_USERS2           220
#define IDI_SHELL_TURN_OFF         221
#define IDI_SHELL_DVD_ROM          222
#define IDI_SHELL_PAGES            223
#define IDI_SHELL_MOVIE_FILE       224
#define IDI_SHELL_MUSIC_FILE       225
#define IDI_SHELL_PICTURE_FILE     226
#define IDI_SHELL_MULTIMEDIA_FILE  227
#define IDI_SHELL_CD_MUSIC2        228
#define IDI_SHELL_MOUSE            229
#define IDI_SHELL_ZIP_DRIVE2       230
#define IDI_SHELL_ARROWN_DOWN      231
#define IDI_SHELL_ARROWN_DOWN1     232
#define IDI_SHELL_FDD              233
#define IDI_SHELL_NO_DRIVE         234
#define IDI_SHELL_MY_DOCUMENTS     235
#define IDI_SHELL_MY_PICTURES      236
#define IDI_SHELL_MY_MUSIC         237
#define IDI_SHELL_MY_MOVIES        238
#define IDI_SHELL_WEB_BROWSER2     239
#define IDI_SHELL_FULL_RECYCLE_BIN2 240
#define IDI_SHELL_FILE_MOVE        241
#define IDI_SHELL_RENAME           242
#define IDI_SHELL_FILES            243
#define IDI_SHELL_MOVE_INTERNET    244
#define IDI_SHELL_PRINTER3         245
#define IDI_SHELL_ICON_246         246
#define IDI_SHELL_WEB_MUSIC        247
#define IDI_SHELL_CAMERA           248
#define IDI_SHELL_SLIDE_SHOW       249
#define IDI_SHELL_DISPLAY          250
#define IDI_SHELL_WEB_PICS         251
#define IDI_SHELL_PRINT_PICS       252
#define IDI_SHELL_FILE_CHECK       253
#define IDI_SHELL_EMPTY_RECYCLE_BIN3 254
#define IDI_SHELL_ICON_255         255
#define IDI_SHELL_FOLDER_MOVE2     256
#define IDI_SHELL_NETWORK_CONNECTIONS2 257
#define IDI_SHELL_NEW_NETWORK_FOLDER 258
#define IDI_SHELL_NETWORK_HOME     259
#define IDI_SHELL_CD_EDIT          260
#define IDI_SHELL_CD_ERASE         261
#define IDI_SHELL_CD_DELETE        262
#define IDI_SHELL_HELP1            263
#define IDI_SHELL_FOLDER_POINT     264
#define IDI_SHELL_SENDMAIL         265
#define IDI_SHELL_CD_POINT         266
#define IDI_SHELL_SHARED_FOLDER    267
#define IDI_SHELL_ACCESSABILITY    268
#define IDI_SHELL_USERS            269
#define IDI_SHELL_SCREEN_COLORS    270
#define IDI_SHELL_ADD_REM_PROGRAMS 271
#define IDI_SHELL_PRINTER_MOUSE    272
#define IDI_SHELL_INTERNET_STATUS  273
#define IDI_SHELL_SYSTEM_GEAR      274
#define IDI_SHELL_PIE_CHART        275
#define IDI_SHELL_INTERNET_DATE    276
#define IDI_SHELL_TUNES            277
#define IDI_SHELL_CMD              278
#define IDI_SHELL_CMD              278
#define IDI_SHELL_USER_ACCOUNTS    279
#define IDI_SHELL_WINDOWS_SEARCH   281
#define IDI_SHELL_COMPUTER_TEXT    282
#define IDI_SHELL_OSK              283
#define IDI_SHELL_ICON_284         284
#define IDI_SHELL_HELP_FILE        289
#define IDI_SHELL_GO               290
#define IDI_SHELL_DVD_DRIVE        291
#define IDI_SHELL_CD_ADD_MUSIC     292
#define IDI_SHELL_CD               293
#define IDI_SHELL_CD_ROM           294
#define IDI_SHELL_CDR              295
#define IDI_SHELL_CDRW             296
#define IDI_SHELL_DVD_RAM          297
#define IDI_SHELL_DVDR_ROM         298
#define IDI_SHELL_MP3_PLAYER       299
#define IDI_SHELL_SERVER           300
#define IDI_SHELL_SERVER1          301
#define IDI_SHELL_CD_ROM1          302
#define IDI_SHELL_COMPACT_FLASH    303
#define IDI_SHELL_DVD_ROM1         304
#define IDI_SHELL_FDD2             305
/* TODO: 306.ico */
#define IDI_SHELL_SD_MMC           307
#define IDI_SHELL_SMART_MEDIA      308
#define IDI_SHELL_CAMERA1          309
#define IDI_SHELL_PHONE            310
#define IDI_SHELL_WEB_PRINTER      311
#define IDI_SHELL_JAZ_DRIVE        312
#define IDI_SHELL_ZIP_DRIVE        313
#define IDI_SHELL_PDA              314
#define IDI_SHELL_SCANNER          315
#define IDI_SHELL_IMAGING_DEVICES  316
#define IDI_SHELL_CAMCORDER        317
#define IDI_SHELL_DVDRW_ROM        318
#define IDI_SHELL_NEW_FOLDER       319
#define IDI_SHELL_CD_POINT1        320
#define IDI_SHELL_CONTROL_PANEL4   321
#define IDI_SHELL_FAVOTITES        322
#define IDI_SHELL_SEARCH1          323
#define IDI_SHELL_HELP2            324
#define IDI_SHELL_LOGOFF1          325
#define IDI_SHELL_PROGRAMS_FOLDER1 326
#define IDI_SHELL_RECENT_DOCUMENTS1 327
#define IDI_SHELL_RUN1             328
#define IDI_SHELL_SHUTDOWN1        329
#define IDI_SHELL_CONTROL_PANEL1   330
#define IDI_SHELL_DISCONNECT       331
#define IDI_SHELL_SEARCH_DIRECTORY 337
#define IDI_SHELL_DELETE_PERMANENTLY 338
#define IDI_SHELL_WEB_BROWSER      512
#define IDI_SHELL_IDEA             1001
#define IDI_SHELL_PRINTER_OK2      1002
#define IDI_SHELL_SERVER_OK        1003
#define IDI_SHELL_HELP_FILE1       1004
#define IDI_SHELL_FILE_MOVE1       1005
#define IDI_SHELL_FILE_PRINT       1006
#define IDI_SHELL_FILE_OK          1007
#define IDI_SHELL_PRINT_PAUSE      1008
#define IDI_SHELL_PRINT_PLAY       1009
#define IDI_SHELL_PRINT_SHARE      1010
#define IDI_SHELL_FAX2             1011
#define IDI_SHELL_SHUTDOWN2        8240

#define IDI_SHELL_DELETE1 16710
#define IDI_SHELL_DELETE2 16715
#define IDI_SHELL_DELETE3 16717
#define IDI_SHELL_DELETE4 16718
#define IDI_SHELL_DELETE5 16721

/*
 * AVI resources
 *
 * windows shell32 has 14 of them: 150-152 and 160-170
 * FIXME: Add 165, 166
 */

#define IDA_SHELL_AUTOPLAY        150
#define IDA_SHELL_SEARCHING_INDEX 151
#define IDA_SHELL_COMPUTER_SEARCH 152
#define IDA_SHELL_COPY            160
#define IDA_SHELL_COPY1           161
#define IDA_SHELL_RECYCLE         162
#define IDA_SHELL_EMPTY_RECYCLE   163
#define IDA_SHELL_DELETE          164
#define IDA_SHELL_COPY2           167
#define IDA_SHELL_COPY3           168
#define IDA_SHELL_DELETE1         169
#define IDA_SHELL_DOWNLOAD        170

/* Desktop Browser commands */
#define FCIDM_DESKBROWSER_CLOSE      0xA004
#define FCIDM_DESKBROWSER_FOCUS      0xA030
#define FCIDM_DESKBROWSER_SEARCH     0xA085
#define FCIDM_DESKBROWSER_REFRESH    0xA220

/* Shell view commands */
#define FCIDM_SHVIEW_ARRANGE         0x7001
#define FCIDM_SHVIEW_VIEW            0x7002
#define FCIDM_SHVIEW_DELETE          0x7011
#define FCIDM_SHVIEW_PROPERTIES      0x7013
#define FCIDM_SHVIEW_CUT             0x7018
#define FCIDM_SHVIEW_COPY            0x7019
#define FCIDM_SHVIEW_INSERT          0x701A
#define FCIDM_SHVIEW_UNDO            0x701B
#define FCIDM_SHVIEW_INSERTLINK      0x701C
#define FCIDM_SHVIEW_COPYTO          0x701E
#define FCIDM_SHVIEW_MOVETO          0x701F
#define FCIDM_SHVIEW_SELECTALL       0x7021
#define FCIDM_SHVIEW_INVERTSELECTION 0x7022

#define FCIDM_SHVIEW_BIGICON     0x7029 //FIXME
#define FCIDM_SHVIEW_SMALLICON   0x702A //FIXME
#define FCIDM_SHVIEW_LISTVIEW    0x702B //FIXME
#define FCIDM_SHVIEW_REPORTVIEW  0x702C //FIXME
/* 0x7030-0x703f are used by the shellbrowser */
#define FCIDM_SHVIEW_AUTOARRANGE 0x7031
#define FCIDM_SHVIEW_SNAPTOGRID  0x7032
#define FCIDM_SHVIEW_ALIGNTOGRID 0x7033

#define FCIDM_SHVIEW_HELP       0x7041
#define FCIDM_SHVIEW_RENAME     0x7050
#define FCIDM_SHVIEW_CREATELINK 0x7051
#define FCIDM_SHVIEW_NEWLINK    0x7052
#define FCIDM_SHVIEW_NEWFOLDER  0x7053

#define FCIDM_SHVIEW_REFRESH 0x7100 /* FIXME */
#define FCIDM_SHVIEW_EXPLORE 0x7101 /* FIXME */
#define FCIDM_SHVIEW_OPEN    0x7102 /* FIXME */

#define FCIDM_TB_UPFOLDER   0xA001
#define FCIDM_TB_NEWFOLDER  0xA002
#define FCIDM_TB_SMALLICON  0xA003
#define FCIDM_TB_REPORTVIEW 0xA004
#define FCIDM_TB_DESKTOP    0xA005  /* FIXME */

#define IDM_UNDO (FCIDM_SHVIEW_UNDO - 0x7000)
#define IDM_CUT (FCIDM_SHVIEW_CUT - 0x7000)
#define IDM_COPY (FCIDM_SHVIEW_COPY - 0x7000)
#define IDM_INSERT (FCIDM_SHVIEW_INSERT - 0x7000)
#define IDM_CREATELINK (FCIDM_SHVIEW_CREATELINK - 0x7000)
#define IDM_DELETE (FCIDM_SHVIEW_DELETE - 0x7000)
#define IDM_RENAME (FCIDM_SHVIEW_RENAME - 0x7000)
#define IDM_PROPERTIES (FCIDM_SHVIEW_PROPERTIES - 0x7000)
#define IDM_COPYTO (FCIDM_SHVIEW_COPYTO - 0x7000)
#define IDM_MOVETO (FCIDM_SHVIEW_MOVETO - 0x7000)

#define IDM_DRAGFILE 0xce
#define IDM_COPYHERE 0x7
#define IDM_MOVEHERE 0x8
#define IDM_LINKHERE 0xB
#define IDM_DVSELECT 0x104

#define IDM_MYDOCUMENTS 516
#define IDM_MYPICTURES 518

/* Registrar scripts (RGS) */
#define IDR_ADMINFOLDERSHORTCUT 128
#define IDR_CONTROLPANEL        130
#define IDR_DRAGDROPHELPER      131
#define IDR_FOLDEROPTIONS       132
#define IDR_FOLDERSHORTCUT      133
#define IDR_FONTSFOLDERSHORTCUT 134
#define IDR_MENUBANDSITE        135
#define IDR_MYCOMPUTER          136
#define IDR_MYDOCUMENTS         137
#define IDR_NETWORKPLACES       138
#define IDR_NEWMENU             139
#define IDR_PRINTERS            140
#define IDR_RECYCLEBIN          141
#define IDR_SHELLDESKTOP        142
#define IDR_SHELLFSFOLDER       143
#define IDR_SHELLLINK           144
#define IDR_STARTMENU           145
#define IDR_OPENWITHMENU        146
#define IDR_FILEDEFEXT          147
#define IDR_DRVDEFEXT           148
#define IDR_MENUBAND            149
#define IDR_MENUDESKBAR         150
#define IDR_EXEDROPHANDLER      151
#define IDR_QUERYASSOCIATIONS   152
#define IDR_MERGEDFOLDER        153
#define IDR_REBARBANDSITE       154
#define IDR_USERNOTIFICATION    155
#define IDR_SHELL               156
#define IDR_ACTIVEDESKTOP       157
#define IDR_SENDTOMENU          158
#define IDR_COPYTOMENU          159
#define IDR_MOVETOMENU          160
#define IDR_COPYASPATHMENU      161
