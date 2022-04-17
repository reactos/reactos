#pragma once

#define IDC_STATIC -1

/*******************************************************************************\
|*                               Icon Resources                                *|
\*******************************************************************************/

#define IDI_MAIN            100
#define IDI_CABINET         101
#define IDI_PRINTER         102
#define IDI_DESKTOP         103
#define IDI_PRINTER_PROBLEM 104
#define IDI_STARTMENU       107
#define IDI_RECYCLEBIN      108
#define IDI_SHOWINFO        109
#define IDI_SHOWALERT       110
#define IDI_SHOWERROR       111
#define IDI_COMPUTER        205
#define IDI_ARROWLEFT       250
#define IDI_ARROWRIGHT      251
#define IDI_FOLDER          252
#define IDI_INTERNET        253
#define IDI_MAIL            254
#define IDI_MAILSMALL       256
#define IDI_STARTMENU2      257

/*******************************************************************************\
|*                               Bitmap Resources                              *|
\*******************************************************************************/

#define IDB_START                           143
#define IDB_TASKBARPROP_AUTOHIDE            145
#define IDB_TASKBARPROP_LOCK_GROUP_QL       146
#define IDB_TASKBARPROP_NOLOCK_GROUP_QL     147
#define IDB_TASKBARPROP_LOCK_NOGROUP_QL     148
#define IDB_TASKBARPROP_NOLOCK_NOGROUP_QL   149
#define IDB_TASKBARPROP_LOCK_GROUP_NOQL     150
#define IDB_TASKBARPROP_NOLOCK_GROUP_NOQL   151
#define IDB_TASKBARPROP_LOCK_NOGROUP_NOQL   152
#define IDB_TASKBARPROP_NOLOCK_NOGROUP_NOQL 153
#define IDB_SYSTRAYPROP_SHOW_SECONDS        154
#define IDB_SYSTRAYPROP_HIDE_SECONDS        155
#define IDB_STARTMENU                       158
#define IDB_STARTPREVIEW                    170
#define IDB_STARTPREVIEW_CLASSIC            171
#define IDB_SYSTRAYPROP_HIDE_CLOCK          180
#define IDB_SYSTRAYPROP_HIDE_NOCLOCK        181
#define IDB_SYSTRAYPROP_SHOW_CLOCK          182
#define IDB_SYSTRAYPROP_SHOW_NOCLOCK        183

/*******************************************************************************\
|*                                Menu Resources                               *|
\*******************************************************************************/

#define IDM_STARTMENU 204
#define IDM_TRAYWND   205

/* NOTE: The following constants may *NOT* be changed because
         they're hardcoded and need to be the exact values
         in order to get the start menu to work! */
#define IDM_PROGRAMS                504
#define IDM_FAVORITES               507
#define IDM_DOCUMENTS               501
#define IDM_SETTINGS                508
#define IDM_CONTROLPANEL            505
#define IDM_SECURITY                5001
#define IDM_NETWORKCONNECTIONS      557
#define IDM_PRINTERSANDFAXES        510
#define IDM_TASKBARANDSTARTMENU     413
#define IDM_SEARCH                  520
#define IDM_HELPANDSUPPORT          503
#define IDM_RUN                     401
#define IDM_SYNCHRONIZE             553
#define IDM_LOGOFF                  402
#define IDM_DISCONNECT              5000
#define IDM_UNDOCKCOMPUTER          410
#define IDM_SHUTDOWN                506
#define IDM_LASTSTARTMENU_SEPARATOR 450

/*******************************************************************************\
|*                               Dialog Resources                              *|
\*******************************************************************************/

#define IDD_TASKBARPROP_TASKBAR       6
#define IDD_NOTIFICATIONS_CUSTOMIZE   7
#define IDD_CLASSICSTART_CUSTOMIZE    9
#define IDD_FILENAME_WARNING          20
#define IDD_TASKBARPROP_STARTMENU     205
#define IDD_MODERNSTART_ADVANCED      1036
#define IDD_MODERNSTART_GENERAL       1037

/*******************************************************************************\
|*                               String Resources                              *|
\*******************************************************************************/

#define IDS_STARTUP_ERROR                  105
#define IDS_START                          595
#define IDS_OPEN_ALL_USERS                 718
#define IDS_EXPLORE_ALL_USERS              719
#define IDS_PROPERTIES                     720
#define IDS_HELP_COMMAND                   732
#define IDS_TASKBAR_STARTMENU_PROP_CAPTION 810
#define IDS_RESTORE_ALL                    811

/*******************************************************************************\
|*                              Control Resources                              *|
\*******************************************************************************/

/* Taskbar Page */
#define IDC_TASKBARPROP_FIRST_CMD            1000
#define IDC_TASKBARPROP_HIDEICONS            1000
#define IDC_TASKBARPROP_ICONCUST             1007
#define IDC_TASKBARPROP_ONTOP                1101
#define IDC_TASKBARPROP_HIDE                 1102
#define IDC_TASKBARPROP_CLOCK                1103
#define IDC_TASKBARPROP_GROUP                1104
#define IDC_TASKBARPROP_LOCK                 1105
#define IDC_TASKBARPROP_SECONDS              1106
#define IDC_TASKBARPROP_SHOWQL               1107
#define IDC_TASKBARPROP_LAST_CMD             1107
#define IDC_TASKBARPROP_TASKBARBITMAP        1111
#define IDC_TASKBARPROP_NOTIFICATIONBITMAP   1112

/* Startmenu Page */
#define IDC_TASKBARPROP_STARTMENUCLASSICCUST 1130
#define IDC_TASKBARPROP_STARTMENUMODERNTEXT  1141
#define IDC_TASKBARPROP_STARTMENUCUST        1131
#define IDC_TASKBARPROP_STARTMENU            1132
#define IDC_TASKBARPROP_STARTMENUCLASSIC     1133
#define IDC_TASKBARPROP_STARTMENUCLASSICTEXT 1142
#define IDC_TASKBARPROP_STARTMENU_BITMAP     1134
#define IDC_STARTBTN                         1140

/* Customize Notifications Dialog */
#define IDC_TASKBARPROP_NOTIREST             1402
#define IDC_NOTIFICATION_LIST                1005
#define IDC_NOTIFICATION_BEHAVIOUR           1006

/* Customize classic start menu dialog */
#define IDC_CLASSICSTART_ADD                 1126
#define IDC_CLASSICSTART_REMOVE              1127
#define IDC_CLASSICSTART_ADVANCED            1128
#define IDC_CLASSICSTART_SORT                1124
#define IDC_CLASSICSTART_CLEAR               1125
#define IDC_CLASSICSTART_SETTINGS            1135

/* File Name Warning Dialog */
#define IDC_FILE_RENAME                      1006
#define IDC_NOTCHECK                         4610

/* Customize modern start menu Advanced Page */
#define IDC_AUTOOPEN                         1306
#define IDC_HIGHLIGHT                        4326
#define IDC_ITEMS                            1123
#define IDC_RECENTLY                         1308
#define IDC_CLEAR                            1309

/* Customize modern start menu General Page */
#define IDC_LARGEICON                       1301
#define IDC_SMALLICON                       1300
#define IDC_CHOOSELARGE                     1302
#define IDC_CHOOSESMALL                     1303
#define IDC_NUMBEROFPROGRAMS                1304
#define IDC_NUMBERUPDOWN                    1305
#define IDC_CLEARLIST                       1310
#define IDC_SHOWINTERNET                    1320
#define IDC_INTERNETDEFAULTAPP              1321
#define IDC_SHOWEMAIL                       1322
#define IDC_EMAILDEFAULTAPP                 1323

/*******************************************************************************\
|*                            Accelerator Resources                            *|
\*******************************************************************************/

#define IDA_TASKBAR 251
#define IDMA_START 305
#define IDMA_CYCLE_FOCUS 41008
#define IDMA_SEARCH 41093
#define IDMA_RESTORE_OPEN 416
#define IDMA_MINIMIZE_ALL 419

#define ID_SHELL_CMD_FIRST              0xF
#define ID_SHELL_CMD_LAST               0x7FEF
#define ID_SHELL_CMD_PROPERTIES         (401)
#define ID_SHELL_CMD_OPEN_ALL_USERS     (402)
#define ID_SHELL_CMD_EXPLORE_ALL_USERS  (403)
#define ID_LOCKTASKBAR                  (404)
#define ID_SHELL_CMD_OPEN_TASKMGR       (405)
#define ID_SHELL_CMD_UNDO_ACTION        (406)
#define ID_SHELL_CMD_SHOW_DESKTOP       (407)
#define ID_SHELL_CMD_TILE_WND_V         (408)
#define ID_SHELL_CMD_TILE_WND_H         (409)
#define ID_SHELL_CMD_CASCADE_WND        (410)
#define ID_SHELL_CMD_CUST_NOTIF         (411)
#define ID_SHELL_CMD_ADJUST_DAT         (412)
#define ID_SHELL_CMD_RESTORE_ALL        (413)
