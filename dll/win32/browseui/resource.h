/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
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

#pragma once

#define IDM_FILE_CLOSE                   0xA021
#define IDM_FILE_EXPLORE_MENU            0xA027
#define IDM_BACKSPACE                    0xA032
#define IDM_EXPLORE_ITEM_FIRST           0xA470
#define IDM_EXPLORE_ITEM_LAST            0xA570
#define IDM_FILE_EXPLORE_SEP             0xA028
#define IDM_VIEW_TOOLBARS                0xA201
#define IDM_TOOLBARS_STANDARDBUTTONS     0xA204
#define IDM_TOOLBARS_ADDRESSBAR          0xA205
#define IDM_TOOLBARS_LINKSBAR            0xA206
#define IDM_TOOLBARS_LOCKTOOLBARS        0xA20C
#define IDM_TOOLBARS_CUSTOMIZE           0xA21D
#define IDM_TOOLBARS_TEXTLABELS          0xA207
#define IDM_TOOLBARS_GOBUTTON            0xA20B
#define IDM_VIEW_STATUSBAR               0xA202
#define IDM_VIEW_EXPLORERBAR             0xA230
#define IDM_EXPLORERBAR_SEARCH           0xA231
#define IDM_EXPLORERBAR_FAVORITES        0xA232
#define IDM_EXPLORERBAR_MEDIA            0xA237
#define IDM_EXPLORERBAR_HISTORY          0xA233
#define IDM_EXPLORERBAR_FOLDERS          0xA235
#define IDM_EXPLORERBAR_SEPARATOR        0xA23B
#define IDM_GOTO_BACK                    0xA121
#define IDM_GOTO_FORWARD                 0xA122
#define IDM_GOTO_UPONELEVEL              0xA022
#define IDM_GOTO_HOMEPAGE                0xA125
#define IDM_VIEW_REFRESH                 0xA220
#define IDM_FAVORITES_ADDTOFAVORITES     0xA173
#define IDM_FAVORITES_ORGANIZEFAVORITES  0xA172
#define IDM_FAVORITES_EMPTY              0xA17E
#define IDM_TOOLS_MAPNETWORKDRIVE        0xA081
#define IDM_TOOLS_DISCONNECTNETWORKDRIVE 0xA082
#define IDM_TOOLS_SYNCHRONIZE            0xA176
#define IDM_TOOLS_FOLDEROPTIONS          0xA123
#define IDM_HELP_ABOUT                   0xA102
#define IDM_TASKBAR_TOOLBARS                268
#define IDM_TASKBAR_TOOLBARS_DESKTOP          3
#define IDM_TASKBAR_TOOLBARS_QUICKLAUNCH      4
#define IDM_TASKBAR_TOOLBARS_NEW              1

#define IDM_BAND_MENU                       269
#define IDM_BAND_TITLE                   0xA200
#define IDM_BAND_CLOSE                   0xA201

#define IDM_POPUPMENU        2001
#define IDM_LARGE_ICONS      2002
#define IDM_SMALL_ICONS      2003
#define IDM_SHOW_TEXT        2004
#define IDM_VIEW_MENU        2005
#define IDM_OPEN_FOLDER      2006

/* Random id for band close button, feel free to change it */
#define IDM_BASEBAR_CLOSE                0xA200

/* User-installed explorer band IDs according to API Monitor traces */
#define IDM_EXPLORERBAND_BEGINCUSTOM     0xA240
#define IDM_EXPLORERBAND_ENDCUSTOM       0xA25C

#define IDM_GOTO_TRAVEL_FIRST       0xA141
#define IDM_GOTO_TRAVEL_LAST        0xA151
#define IDM_GOTO_TRAVEL_SEP         IDM_GOTO_TRAVEL_FIRST
#define IDM_GOTO_TRAVEL_FIRSTTARGET IDM_GOTO_TRAVEL_FIRST+1
#define IDM_GOTO_TRAVEL_LASTTARGET  IDM_GOTO_TRAVEL_LAST

#define IDM_CABINET_CONTEXTMENU 103
#define IDM_CABINET_MAINMENU    104

#define IDR_ACLMULTI             128
#define IDR_ADDRESSBAND          129
#define IDR_ADDRESSEDITBOX       130
#define IDR_BANDPROXY            131
#define IDR_BANDSITE             132
#define IDR_BANDSITEMENU         133
#define IDR_BRANDBAND            134
#define IDR_COMMONBROWSER        135
#define IDR_INTERNETTOOLBAR      136
#define IDR_GLOBALFOLDERSETTINGS 137
#define IDR_REGTREEOPTIONS       138
#define IDR_EXPLORERBAND         139
#define IDR_PROGRESSDIALOG       140
#define IDR_AUTOCOMPLETE         141
#define IDR_ACLISTISF            142
#define IDR_ISFBAND              143
#define IDR_ACLCUSTOMMRU         144
#define IDR_TASKBARLIST          145
#define IDR_FILESEARCHBAND       146
#define IDR_FINDFOLDER           147

#define IDS_SMALLICONS           12301
#define IDS_LARGEICONS           12302
#define IDS_SHOWTEXTLABELS       12303
#define IDS_NOTEXTLABELS         12304
#define IDS_SELECTIVETEXTONRIGHT 12305
#define IDS_BROWSEFORNEWTOOLAR   12387
#define IDS_TOOLBAR_ERR_TITLE    12388
#define IDS_TOOLBAR_ERR_TEXT     12389
#define IDS_GOBUTTONLABEL        12656
#define IDS_GOBUTTONTIPTEMPLATE  12657
#define IDS_SEARCHLABEL          12897
#define IDS_STANDARD_TOOLBAR     12624
#define IDS_ADDRESSBANDLABEL     12902
#define IDS_FOLDERSLABEL         12919
#define IDS_HISTORYTEXT          13169
#define IDS_UP                   58434
#define IDS_BACK                 58689
#define IDS_FORWARD              58690
#define IDS_FOLDER_OPTIONS       58691

#define IDS_CANCELLING           16
#define IDS_REMAINING            17
#define IDC_ANIMATION            100
#define IDC_PROGRESS_BAR         102
#define IDC_TEXT_LINE            103
#define IDD_PROGRESS_DLG         100
#define IDR_ACCELERATORS 256

#define IDI_CABINET 103

#define IDD_CUSTOMIZETOOLBAREX 256

#define IDC_TEXTOPTIONS 4096
#define IDC_ICONOPTIONS 4097

#define IDB_BANDBUTTONS 545
#define IDB_SHELL_EXPLORER_LG       214
#define IDB_SHELL_EXPLORER_LG_HOT   215
#define IDB_SHELL_EXPLORER_SM       216
#define IDB_SHELL_EXPLORER_SM_HOT   217
#define IDB_SHELL_GO                230
#define IDB_SHELL_GO_HOT            231
#define IDB_SHELL_BRANDBAND_SM_HI   240
/*#define IDB_SHELL_BRANDBAND_MD_HI   241
#define IDB_SHELL_BRANDBAND_LG_HI   242
#define IDB_SHELL_BRANDBAND_SM_LO   245
#define IDB_SHELL_BRANDBAND_MD_LO   246
#define IDB_SHELL_BRANDBAND_LG_LO   247*/

#define IDD_SEARCH_DLG          1000
#define IDC_SEARCH_LABEL        1001
#define IDC_SEARCH_FILENAME     1002
#define IDC_SEARCH_QUERY        1003
#define IDC_SEARCH_BUTTON       1004
#define IDC_SEARCH_STOP_BUTTON  1005
#define IDC_SEARCH_COMBOBOX     1006
#define IDC_SEARCH_HIDDEN       1007
#define IDS_SEARCHINVALID       4518
#define IDS_COL_NAME            8976
#define IDS_COL_LOCATION        8977
#define IDS_COL_RELEVANCE       8989
#define IDS_SEARCH_FILES_FOUND  9232
#define IDS_SEARCH_FOLDER       9234
#define IDS_SEARCH_RESULTS      30520
#define IDS_SEARCH_OPEN_FOLDER  40960

#define IDS_PARSE_ADDR_ERR_TITLE 9600
#define IDS_PARSE_ADDR_ERR_TEXT  9601
