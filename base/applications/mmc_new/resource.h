/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Resource header
 * COPYRIGHT:   Copyright 2006-2007 Thomas Weidenmueller
 *              Copyright 2017-2020 Mark Jansen (mark.jansen@reactos.org)
 */

#pragma once

#define IDS_APPTITLE     101
#define IDS_CONSOLETITLE 102
#define IDS_MODULE       103
#define IDS_VENDOR       104
#define IDS_ABOUTTITLE   105

#define IDI_MAINAPP 101
#define IDI_FOLDERICON 102
#define IDB_TOOLBAR             105

#define IDA_MMC         500

#define IDM_CONSOLE_SMALL      1000
#define IDM_CONSOLE_LARGE      1001

#define IDM_FILE_NEW           1010
#define IDM_FILE_OPEN          1011
#define IDM_FILE_SAVE          1012
#define IDM_FILE_SAVEAS        1013
#define IDM_FILE_ADD           1014
#define IDM_FILE_EXIT          1015

#define IDM_ACTION_NEW         1050
#define IDM_ACTION_RENAME      1051
#define IDM_ACTION_EXPORT_LIST 1052

#define IDM_VIEW_COLUMNS       1100
#define IDM_VIEW_LARGE_ICONS   1101
#define IDM_VIEW_SMALL_ICONS   1102
#define IDM_VIEW_LIST          1103
#define IDM_VIEW_DETAILS       1104
#define IDM_VIEW_CUSTOMIZE     1105

#define IDM_FAVORITES_ADD      1150
#define IDM_FAVORITES_ORGANIZE 1151

#define IDM_WINDOWS_NEW        1200
#define IDM_WINDOWS_CASCADE    1201
#define IDM_WINDOWS_TILE       1202
#define IDM_WINDOWS_ARRANGE    1203

#define IDM_HELP_ABOUT         1251
#define IDM_MDI_FIRSTCHILD     9500

#define IDM_TB_UNDO            1500
#define IDM_TB_REDO            1501
#define IDM_TB_UP              1502
#define IDM_TB_SCOPE_PANE      1503
#define IDM_TB_HELP            1504
#define IDM_TB_ACTIONS_PANE    1505

#define IDD_DIALOG_ADD                3000
#define IDC_BUTTON_ADD                3001
#define IDC_BUTTON_REMOVE             3002
#define IDC_LIST_AVAILABLE            3003
#define IDC_LIST_SELECTED             3004
#define IDC_DESCRIPTION               3005

#define IDD_CUSTOMIZE                 3100
#define IDC_CUSTOMIZE_SCOPEPANE       3101
#define IDC_CUSTOMIZE_MENUS           3102
#define IDC_CUSTOMIZE_TOOLBAR         3103
#define IDC_CUSTOMIZE_STATUSBAR       3104
#define IDC_CUSTOMIZE_DESCRIPTIONBAR  3105
#define IDC_CUSTOMIZE_TABS            3106
#define IDC_CUSTOMIZE_ACTIONSPANE     3107
#define IDC_CUSTOMIZE_SNAPIN_MENUS    3108
#define IDC_CUSTOMIZE_SNAPIN_TOOLBARS 3109

#ifndef IDC_STATIC
#define IDC_STATIC  -1
#endif
