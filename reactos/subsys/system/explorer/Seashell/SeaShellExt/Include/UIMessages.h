//*******************************************************************************
// COPYRIGHT NOTES
// ---------------
// You may use this source code, compile or redistribute it as part of your application 
// for free. You cannot redistribute it as a part of a software development 
// library without the agreement of the author. If the sources are 
// distributed along with the application, you should leave the original 
// copyright notes in the source code without any changes.
// This code can be used WITHOUT ANY WARRANTIES at your own risk.
// 
// For the latest updates to this code, check this site:
// http://www.masmex.com 
// after Sept 2000
// 
// Copyright(C) 2000 Philip Oldaker <email: philip@masmex.com>
//*******************************************************************************

// General
#define WM_APP_UPDATE_ALL_VIEWS		WM_APP+1001
#define WM_APP_UPDATE_STATUS		WM_APP+1002
#define WM_APP_RESIZE				WM_APP+1003
#define WM_APP_SHELL_INIT_POPUP		WM_APP+1004
#define WM_APP_CB_IE_HIT_ENTER		WM_APP+1005
#define WM_APP_CB_IE_POPULATE		WM_APP+1006
#define WM_APP_CB_IE_SEL_CHANGE		WM_APP+1007
#define WM_APP_CB_IE_SET_EDIT_TEXT	WM_APP+1008
#define WM_UPDATEHEADERWIDTH		WM_APP+1020
#define WM_HEADERWIDTHCHANGED		WM_APP+1021
// Keyboard
#define WM_APP_ON_CONTEXT_MENU_KEY	WM_APP+1100
#define WM_APP_ON_BACKSPACE_KEY		WM_APP+1101
#define WM_APP_ON_EDIT_KEY			WM_APP+1102
#define WM_APP_ON_REFRESH_KEY		WM_APP+1103
#define WM_APP_ON_DELETE_KEY		WM_APP+1104
#define WM_APP_ON_PROPERTIES_KEY	WM_APP+1105
// Internet
#define WM_APP_INET_PAGE_READY		WM_APP+1501
#define WM_APP_INET_STATUS			WM_APP+1502
// OLE drag and drop 
#define WM_APP_OLE_DD_ENTER			WM_APP+2001
#define WM_APP_OLE_DD_LEAVE			WM_APP+2002
#define WM_APP_OLE_DD_DROP			WM_APP+2003
#define WM_APP_OLE_DD_OVER			WM_APP+2004
#define WM_APP_OLE_DD_DODRAGDROP	WM_APP+2005
// Tab views
#define WM_APP_TAB_SEL_CHANGE		WM_APP+3001 
#define WM_APP_TAB_UPDATE_VIEWS		WM_APP+3002
// Tree control
#define WM_APP_DD_ADD_FOLDER_ITEMS	WM_APP+4001
#define WM_APP_DD_NEW_FOLDER_ITEM	WM_APP+4002
#define WM_APP_NEW_FOLDER			WM_APP+4003
#define WM_APP_DIR_CHANGE_EVENT		WM_APP+4006
#define WM_APP_TIMER_SEL_CHANGE		WM_APP+4007
#define WM_APP_POPULATE_TREE		WM_APP+4008
// List Control
#define WM_APP_FILE_CHANGE_EVENT	WM_APP+5001
#define WM_APP_FILE_CHANGE_NEW_PATH WM_APP+5002
// Tray Icon
#define WM_APP_TRAY_NOTIFY			WM_APP+6001

// Hints for UpdateAllViews
#define HINT_BASE_CTRL_EXT 1000
#define HINT_TREE_SEL_CHANGED					HINT_BASE_CTRL_EXT+1
#define HINT_TREE_INTERNET_FOLDER_SELECTED		HINT_BASE_CTRL_EXT+2
#define HINT_SHELL_FILE_CHANGED					HINT_BASE_CTRL_EXT+3
#define HINT_SHELL_DIR_CHANGED					HINT_BASE_CTRL_EXT+4
#define HINT_STATUS_BAR_TEXT					HINT_BASE_CTRL_EXT+5

