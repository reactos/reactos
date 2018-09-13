//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//
//  Contents:   Helpids for Shell Help project (windows.hlp) - the help ids
//              in this DLL are for shell componentns (not browser only or redist)
//              ie.  Shdoc401 has a "fixed in stone" version of this headerfile
//              for the "update.hlp" help file shipped with IE.
//              ("iexplore.hlp" is used for shdocvw.dll/browseui.dll.  see iehelpid.h)
//
//  Please keep this file ordered by help ID.  That way we can
//  find space for new ids easily.
//

#define IDH_MYDOCS_TARGET       1101
#define IDH_MYDOCS_BROWSE       1102
#define IDH_MYDOCS_FIND_TARGET  1103
#define IDH_MYDOCS_RESET        1104

// Background Tab implemented in shell32.dll (Win2K version) which replaces
// the background tab implemented in desk.cpl
// (The corresponding help texts for these IDs are in "Display.hlp")

#define IDH_DISPLAY_BACKGROUND_MONITOR              4000
#define IDH_DISPLAY_BACKGROUND_WALLPAPERLIST        4001
#define IDH_DISPLAY_BACKGROUND_BROWSE_BUTTON        4002
#define IDH_DISPLAY_BACKGROUND_PICTUREDISPLAY       4003
#define IDH_DISPLAY_BACKGROUND_DISPLAY_TILE         4004
#define IDH_DISPLAY_BACKGROUND_DISPLAY_CENTER       4005
#define IDH_DISPLAY_BACKGROUND_DISPLAY_STRETCH      4006
#define IDH_DISPLAY_BACKGROUND_PATTERN_BUTTON       4007
#define IDH_DISPLAY_BACKGROUND_PATTERN_PATTERNLIST  4008
#define IDH_DISPLAY_BACKGROUND_PATTERN_PREVIEW      4009
#define IDH_DISPLAY_BACKGROUND_EDITPATTERN_BUTTON   4010
#define IDH_DISPLAY_BACKGROUND_EDITPATTERN_NAME     4011
#define IDH_DISPLAY_BACKGROUND_EDITPATTERN_SAMPLE   4012
#define IDH_DISPLAY_BACKGROUND_EDITPATTERN_PATTERN  4013
#define IDH_DISPLAY_BACKGROUND_EDITPATTERN_DONE     4177
#define IDH_DISPLAY_BACKGROUND_EDITPATTERN_ADD      4178
#define IDH_DISPLAY_BACKGROUND_EDITPATTERN_CHANGE   4179
#define IDH_DISPLAY_BACKGROUND_EDITPATTERN_REMOVE   4180

//
// Web Tab implemented in shell32.dll (Win2K version)
// (The corresponding help texts for these IDs are in "Display.hlp")
//

#define IDH_DISPLAY_WEB_GRAPHIC                     4500
#define IDH_DISPLAY_WEB_SHOWWEB_CHECKBOX            4501
#define IDH_DISPLAY_WEB_ACTIVEDESKTOP_LIST          4502
#define IDH_DISPLAY_WEB_NEW_BUTTON                  4503
#define IDH_DISPLAY_WEB_DELETE_BUTTON               4504
#define IDH_DISPLAY_WEB_PROPERTIES_BUTTON           4505


// For Display Properties, Background Tab (implemented in shdoc401.dll)
#define IDH_GROUPBOX                       51000
#define IDH_WALLPAPER_LIST                 51001
#define IDH_BROWSE_WALLPAPER               51002
#define IDH_DESKTOP_PATTERN                51003
#define IDH_DISPLAY_WALLPAPER              51004
#define IDH_DISABLE_ACTIVE_DESKTOP         51005
#define IDH_WALLPAPER_SAMPLE               51006

// For Properties button (implemented in shdoc401.dll)
#define IDH_DISPLAY_PATTERN                51010
#define IDH_EDIT_PATTERN                   51011

// For Pattern Editor (implemented in shdoc401.dll)
#define IDH_PATTERN_NAME                   51012
#define IDH_PATTERN_SAMPLE                 51013
#define IDH_PATTERN_EDIT                   51014
#define IDH_ADD_PATTERN                    51015
#define IDH_CHANGE_PATTERN                 51016
#define IDH_REMOVE_PATTERN                 51017

// For Display Properties, Web tab (implemented in shdoc401.dll)
#define IDH_LIST_CHANNELS                  51020
#define IDH_NEW_CHANNEL                    51021
#define IDH_DELETE_CHANNEL                 51022
#define IDH_CHANNEL_PROPERTIES             51023
#define IDH_TRY_IT                         51024
#define IDH_RESET_ALL                      51025
#define IDH_DISPLAY_CHANNELS               51027
#define IDH_VIEW_AS_WEB_PAGE               51026
#define IDH_FOLDER_OPTIONS                 51029

// For Web tab, Properties button, Subscription tab
#define IDH_SUBSCRIBED_URL                 51030
#define IDH_SUBSCRIPTION_SUMMARY           51031
// Login button
#define IDH_CHANNEL_LOGIN                  51032
// Login Options dialog
#define IDH_LOGIN_USER_ID                  51033
#define IDH_LOGIN_PASSWORD                 51034


// For Web tab, Properties button, Receiving tab
#define IDH_EMAIL_NOTIFICATION             51035
#define IDH_DOWNLOAD                       51036
#define IDH_ADVANCED                       51028
// Change Address button
#define IDH_CHANGE_ADDRESS                 51037
// Mail Options dialog
#define IDH_EMAIL_ADDRESS                  51038
#define IDH_EMAIL_SERVER                   51039

// Advanced Download Options dialog
#define IDH_MAX_DOWNLOAD                   51040
#define IDH_HIGH_PRIORITY                  51041
#define IDH_DOWNLOAD_IMAGES                51042
#define IDH_DOWNLOAD_SOUND                 51043
#define IDH_DOWNLOAD_ACTIVEX               51044
#define IDH_DOWNLOAD_PAGES_DEEP            51045
#define IDH_FOLLOW_LINKS                   51046

// For Web tab, Properties button, Schedule tab
#define IDH_AUTO_SCHEDULE                  51050
#define IDH_CUSTOM_SCHEDULE                51051
#define IDH_MANUAL_SCHEDULE                51052

// For Custom Schedule dialog
#define IDH_NEW_NAME                       51053
#define IDH_SCHED_DAYS                     51054
#define IDH_SCHED_FREQUENCY                51055
#define IDH_SCHED_TIME                     51056
#define IDH_SCHED_REPEAT                   51057
#define IDH_VARY_START                     51058

// For View, Options, General tab, Folders and desktop (My Computer)
//#define IDH_SAMPLE_GRAPHIC                 51060 // shdoc401
//#define IDH_WEB_VIEW                       51061 // shdoc401

//  View\Options menu, Files Types tab, Add New File Type dialog box
#define  IDH_MIME_TYPE                     51063
#define  IDH_DEFAULT_EXT                   51064
#define  IDH_CONFIRM_OPEN                  51065
#define  IDH_SAME_WINDOW                   51066

//  View\Options menu, File Types tab
#define  IDH_EXTENSION                     51067
#define  IDH_OPENS_WITH                    51068

// For View, Options, View tab (My Computer)
//#define IDH_SHOW_MAP_NETWORK               51070 // shdoc401 selfreg
#define IDH_SHOW_FILE_ATTRIB               51071
#define IDH_ALLOW_UPPERCASE                51072 // shell32 selfreg
#define IDH_SMOOTH_EDGES                   51073 // shell32 selfreg
#define IDH_SHOW_WINDOW                    51074 // shell32 selfreg
#define IDH_RESTORE_DEFAULT                51075
#define IDH_VIEW_STATE                     51076 // shell32 selfreg
#define IDH_USE_CURRENT_FOLDER             51077
#define IDH_RESET_TO_ORIGINAL              51078
#define IDH_FOLDERS_IN_SEP_PROCESS         51079 // shell32 selfreg

// For Folder Properties, General tab
#define IDH_PROPERTIES_GENERAL_THUMBNAIL   51080

// For Browse for Folder (right-click taskbar, Toolbar, New Toolbar)
#define IDH_BROWSE_FOLDER_ADDRESS          51082

//   Display properties, Screen Saver tab, Channel Screen Saver settings
#define  IDH_CHANNELS_LIST                 51083
#define  IDH_SET_LENGTH                    51084
#define  IDH_PLAY_SOUNDS                   51085
#define  IDH_CLOSE_SCREENSAVER             51086

//  Subscription properties, Unsubscribe button
#define  IDH_UNSUBSCRIBE                   51087

//  Subscription properties, Schedule
#define  IDH_SCHEDULE_NEW                  51088
#define  IDH_SCHEDULE_REMOVE               51089

// For View, Options, General tab, Folders and desktop (My Computer)
//#define IDH_CLASSIC_STYLE                  51090 // shdoc401
//#define IDH_CUSTOM                         51091 // shdoc401

// For View, Options, General Tab
#define IDH_BROWSE_SAME_WINDOW             51092
#define IDH_BROWSE_SEPARATE_WINDOWS        51093
#define IDH_SHOW_WEB_WHEN_POSSIBLE         51094
#define IDH_SHOW_WEB_WHEN_CHOOSE           51095
#define IDH_SINGLE_CLICK_MODE              51096
#define IDH_TITLES_LIKE_LINKS              51097
#define IDH_TITLES_WHEN_POINT              51098
#define IDH_DOUBLE_CLICK_MODE              51099

// For View, Folder Options, Advanced
#define IDH_FULL_PATH                      51100 // shell32 selfreg
#define IDH_HIDE_EXTENSIONS                51101 // shell32 selfreg
#define IDH_SHOW_TIPS                      51102 // shell32 selfreg
#define IDH_HIDE_HIDDEN_SYSTEM             51103 // shell32 selfreg
#define IDH_HIDE_HIDDEN_ONLY               51104 // shell32 selfreg
#define IDH_SHOW_ALL                       51105 // shell32 selfreg
#define IDH_HIDE_ICONS                     51106 // shell32 selfreg
#define IDH_FULL_PATH_ADDRESSBAR           51107 // shdoc401

// For View, Options, General Tab
#define IDH_ENABLE_WEB_CONTENT             51108
#define IDH_USE_WINDOWS_CLASSIC            51109
//#define IDH_CUSTOMIZE_ACTIVE_DESKTOP       51110 // shdoc401
#define IDH_ACTIVEDESKTOP_GEN              51111
#define IDH_WEB_VIEW_GEN                   51112
#define IDH_BROWSE_FOLDERS_GEN             51113
#define IDH_ICON_OPEN_GEN                  51114
#define IDH_RESTORE_DEFAULTS_GEN           51115


// For Folder Customization Wizard
// Start Page
#define IDH_FCW_CHOOSE_OR_EDIT_TEMPLATE    51116
#define IDH_FCW_CHOOSE_BACKGROUND_PICTURE  51117
#define IDH_FCW_REMOVE_CUST                51118
#define IDH_FCW_DESCRIBE_CHOICE            51119
// Template Page
#define IDH_FCW_TEMPLATE_LIST              51120
#define IDH_FCW_TEMPLATE_PREVIEW           51121
#define IDH_FCW_DESCRIBE_TEMPLATE          51122
#define IDH_FCW_ENABLE_EDITING             51123
// Background Page
#define IDH_FCW_BACKGROUND_PREVIEW         51124
#define IDH_FCW_BACKGROUND_LIST            51125
#define IDH_FCW_BACKGROUND_BROWSE          51126
#define IDH_FCW_ICON_TEXT_COLOR            51127
#define IDH_FCW_ENABLE_ICON_BACKGROUND_COLOR    51128
#define IDH_FCW_ICON_BACKGROUND_COLOR      51129

#define IDH_SHOW_COMP_COLOR                51130 // shell32 selfreg
#define IDH_HIDDEN_FILES_GROUP             51131 // shell32 selfreg
#define IDH_STARTMENU                      51132 // shell32 selfreg
#define IDH_STARTMENU_FAVORITES            51133 // shell32 selfreg
#define IDH_STARTMENU_LOGOFF               51134 // shell32 selfreg
#define IDH_STARTMENU_CONTROLPANEL         51135 // shell32 selfreg
#define IDH_STARTMENU_MYDOCUMENTS          51136 // shell32 selfreg
#define IDH_STARTMENU_PRINTERS             51137 // shell32 selfreg
#define IDH_STARTMENU_SCROLLPROGRAMS       51138 // shell32 selfreg
#define IDH_STARTMENU_INTELLIMENUS         51139 // shell32 selfreg
#define IDH_FILES_AND_FOLDERS              51140 // shell32 selfreg

#define IDH_SHOW_MY_DOCUMENTS              51141 // mydocs selfreg
#define IDH_SHOW_MY_COMPUTER               51142 // shell32 selfreg
#define IDH_SHOW_MY_NETPLACES              51143 // shell32 selfreg
