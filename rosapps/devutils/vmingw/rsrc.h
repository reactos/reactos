/********************************************************************
*	Module:	resource.h. This is part of Visual-MinGW.
*
*	License:	Visual-MinGW is covered by GNU General Public License, 
*			Copyright (C) 2001  Manu B.
*			See license.htm for more details.
*
********************************************************************/
#define APP_VERSION "0.43a"
#define FULL_APP_VERSION "0.43 alpha"

// Static control ----------------------------
#define IDC_STATIC				-1

// Bitmaps ----------------------------------
#define IDB_TOOLBAR				10
#define IDB_TREEVIEW				11

#define IDAPPLY					20

// Preferences Dialog -----------------------
#define IDD_PREFERENCES			100
#define IDC_PREF_TABS			101

#define IDD_ENVIRON				110
#define IDC_SET_CCBIN			111
#define IDC_CCBIN				112
#define IDC_BROWSE_CC			113
#define IDC_SET_CMDBIN			114
#define IDC_CMDBIN				115
#define IDC_BROWSE_CMD			116
#define IDC_AUTOEXEC				117
#define IDC_ENV_VIEW				118
/*#define IDC_CC_INCDIR			101
#define IDC_BROWSE				102*/

// Find Dialog -------------------------------
#define IDD_FIND					120
#define IDC_FINDWHAT				121
#define IDC_WHOLEWORD			122
#define IDC_MATCHCASE			123
#define IDC_REGEXP				124
#define IDC_WRAP				125
#define IDC_UNSLASH				126
#define IDC_DIRECTIONUP			127
#define IDC_DIRECTIONDOWN		128

// Replace ----------------------------------
#define IDD_REPLACE				130
#define IDC_REPLACEWITH			131
#define IDC_REPLACE				132
#define IDC_REPLACEALL			133
#define IDC_REPLACEINSEL			134

// Find in files -------------------------------
#define IDD_GREP				135
#define IDC_GFILTER				136
#define IDC_GDIR					137
#define IDC_GBROWSE				139

// New module ------------------------------
#define IDD_NEW_MODULE			140
#define IDC_HEADER				141

// Options ----------------------------------
#define IDD_OPTION				150
#define IDC_OPTION_TABS			151
#define IDC_HELP_BTN				152

// General tab ---------------------------------
#define IDD_GENERAL_PANE			160
#define IDC_STATLIB				161
#define IDC_DLL					162
#define IDC_CONSOLE				163
#define IDC_GUIEXE				164
#define IDC_DBGSYM				165
#define IDC_LANGC				166
#define IDC_LANGCPP				167
#define IDC_MKF_NAME			168
#define IDC_MKF_DIR				169
#define IDC_USER_MKF				170
#define IDC_TGT_NAME			171
#define IDC_TGT_DIR				172

// Compiler tab -----------------------------
#define IDD_COMPILER				180
#define IDC_CPPFLAGS				181
#define IDC_WARNING				182
#define IDC_OPTIMIZ				183
#define IDC_CFLAGS				184
#define IDC_INCDIRS				185

// Linker tab --------------------------------
#define IDD_LINKER				190
#define IDC_LDSTRIP				191
#define IDC_LDOPTS				192
#define IDC_LDLIBS				193
#define IDC_LIBDIRS				194

#define IDD_ZIP					200
#define IDC_INFOZIP				201
#define IDC_TAR_GZIP				202
#define IDC_TAR_BZ2				203
#define IDC_ZIP_TEST				204
#define IDC_ZIP_DIR				205
#define IDC_ZIPFLAGS				206
// About ------------------------------------
#define IDD_ABOUT				210

#define IDD_COMMAND				220
#define IDC_CMDLINE				221
// Menu -------------------------------------
#define ID_MENU                         	1000
#define IDM_NEW                         	1001
#define IDM_OPEN                        	1002
//#define IDM_CLOSE                       	1003
#define IDM_NEW_PROJECT			1004
#define IDM_OPEN_PROJECT			1005
#define IDM_SAVE_PROJECT			1006
#define IDM_CLOSE_PROJECT			1007
#define IDM_SAVE                        	1008
#define IDM_SAVEAS                      	1009
#define IDM_SAVEALL				1010
#define IDM_PREFERENCES			1011
#define IDM_PAGESETUP			1012
#define IDM_PRINT                       	1013
#define IDM_QUIT                        	1014

#define IDM_UNDO                        	1020
#define IDM_REDO                       	1021
#define IDM_CUT                         	1022
#define IDM_COPY                        	1023
#define IDM_PASTE                       	1024
#define IDM_SELECTALL			1025

#define IDM_FIND                        	1030
#define IDM_REPLACE				1031
#define IDM_GREP				1032

#define IDM_CASCADE				1040
#define IDM_TILEHORZ                    	1041
#define IDM_TILEVERT                    	1042
#define IDM_ARRANGE				1043

#define IDM_NEW_MODULE			1050
#define IDM_ADD					1051
#define IDM_REMOVE_FILE			1052
#define IDM_REMOVE_MODULE		1053
#define IDM_OPTION				1054
#define IDM_ZIP_SRCS				1055
#define IDM_EXPLORE				1056

#define IDM_BUILD				1060
#define IDM_REBUILDALL			1061
#define IDM_RUN_TARGET			1062
#define IDM_MKCLEAN				1063
#define IDM_MKF_BUILD			1064
#define IDM_RUN_CMD				1065

#define IDM_TEST				1070

#define IDM_HELP                        	1080
#define IDM_ABOUT                       	1081

#define ID_POPMENU                      	1100

#define ID_FIRSTCHILD				2000

