#ifndef __CPL_DESK_RESOURCE_H__
#define __CPL_DESK_RESOURCE_H__

/* metrics */
#define PROPSHEETWIDTH      246
#define PROPSHEETHEIGHT     228
#define PROPSHEETPADDING    6

#define SYSTEM_COLUMN       (18 * PROPSHEETPADDING)
#define LABELLINE(x)        (((PROPSHEETPADDING + 2) * x) + (x + 2))

#define ICONSIZE            16

/* ids */
#define IDC_DESK_ICON                   40
#define IDC_DESK_ICON2                  100 // Needed for theme compatability with Windows.

#define IDC_STATIC                      -1

#define IDD_BACKGROUND                  100
#define IDD_SCREENSAVER                 101
#define IDD_APPEARANCE                  102
#define IDD_SETTINGS                    103
#define IDD_ADVAPPEARANCE               104
#define IDD_ADVANCED_GENERAL            200

/* Background Page */
#define IDC_BACKGROUND_LIST             1000
#define IDC_MONITOR                     1001
#define IDC_BACKGROUND_PREVIEW          1002
#define IDC_BROWSE_BUTTON               1003
#define IDC_COLOR_BUTTON                1004
#define IDC_PLACEMENT_COMBO             1005
#define IDS_BACKGROUND_COMDLG_FILTER    1006
#define IDS_SUPPORTED_EXT               1007


/* Screensaver Page */
#define IDC_SCREENS_PREVIEW             1010
#define IDC_SCREENS_LIST                1011
#define IDC_SCREENS_POWER_BUTTON        1012
#define IDC_SCREENS_SETTINGS            1013
#define IDC_SCREENS_TESTSC              1014
#define IDC_SCREENS_USEPASSCHK          1015
#define IDC_SCREENS_TIMEDELAY           1016
#define IDC_SCREENS_TIME                1017
#define IDC_WAITTEXT                    1018
#define IDC_MINTEXT                     1019
#define IDC_SCREENS_DUMMY               5000
#define IDC_SCREENS_DUMMY2              5001

#define IDC_SCREENS_CHOICES -1

#define IDS_CPLNAME                 2000
#define IDS_CPLDESCRIPTION          2001

#define IDS_NONE                    2002
#define IDS_CENTER                  2003
#define IDS_STRETCH                 2004
#define IDS_TILE                    2005

#define IDC_SETTINGS_DEVICE          201
#define IDC_SETTINGS_BPP             202
#define IDC_SETTINGS_RESOLUTION      203
#define IDC_SETTINGS_RESOLUTION_TEXT 204
#define IDC_SETTINGS_ADVANCED        205
#define IDC_SETTINGS_MONSEL          206
#define IDC_SETTINGS_SPECTRUM        207
#define IDB_SPECTRUM                 208

#define IDR_PREVIEW_MENU     2100
#define ID_MENU_NORMAL       2101
#define ID_MENU_DISABLED     2102
#define ID_MENU_SELECTED     2103

#define IDM_MONITOR_MENU    2110
#define ID_MENU_ATTACHED    2111
#define ID_MENU_PRIMARY     2112
#define ID_MENU_IDENTIFY    2113
#define ID_MENU_PROPERTIES  2114

/* Settings Page */

#define IDS_PIXEL				2301

#define IDS_COLOR_4BIT			2904
#define IDS_COLOR_8BIT			2908
#define IDS_COLOR_16BIT			2916
#define IDS_COLOR_24BIT			2924
#define IDS_COLOR_32BIT			2932


/* Appearance Page */
#define IDC_APPEARANCE_PREVIEW   1500
#define IDC_APPEARANCE_UI_ITEM   1501
#define IDC_APPEARANCE_COLORSCHEME	1502
#define IDC_APPEARANCE_FONTSIZE		1503
#define IDC_APPEARANCE_EFFECTS		1504
#define IDC_APPEARANCE_ADVANCED		1505

#define IDS_INACTWIN    1510
#define IDS_ACTWIN      1511
#define IDS_WINTEXT     1512
#define IDS_MESSBOX     1513
#define IDS_MESSTEXT    1514
#define IDS_BUTTEXT     1515

/* Update these IDs when you change the string id list */
#define IDS_ITEM_FIRST           (IDS_ITEM_3D_OBJECTS)
#define IDS_ITEM_LAST            (IDS_ITEM_CAPTION_BUTTONS + 1)


/* Advanced Appearance Dialog */#

#define IDC_ADVAPPEARANCE_PREVIEW	3101
#define IDC_ADVAPPEARANCE_ELEMENT	3102
#define IDC_ADVAPPEARANCE_SIZE_T	3103
#define IDC_ADVAPPEARANCE_SIZE_E	3104
#define IDC_ADVAPPEARANCE_SIZE_UD	3105
#define IDC_ADVAPPEARANCE_COLOR1_T	3106
#define IDC_ADVAPPEARANCE_COLOR1_B	3107
#define IDC_ADVAPPEARANCE_COLOR2_T	3108
#define IDC_ADVAPPEARANCE_COLOR2_B	3109
#define IDC_ADVAPPEARANCE_FONT_T	3110
#define IDC_ADVAPPEARANCE_FONT_C	3111
#define IDC_ADVAPPEARANCE_FONTSIZE_T	3112
#define IDC_ADVAPPEARANCE_FONTSIZE_E	3113
#define IDC_ADVAPPEARANCE_FONTCOLOR_T	3114
#define IDC_ADVAPPEARANCE_FONTCOLOR_B	3115
#define IDC_ADVAPPEARANCE_FONTBOLD	3116
#define IDC_ADVAPPEARANCE_FONTITALIC	3117

#define IDS_ELEMENT_0				3200
#define IDS_ELEMENT_1				3201
#define IDS_ELEMENT_2				3202
#define IDS_ELEMENT_3				3203
#define IDS_ELEMENT_4				3204
#define IDS_ELEMENT_5				3205
#define IDS_ELEMENT_6				3206
#define IDS_ELEMENT_7				3207
#define IDS_ELEMENT_8				3208
#define IDS_ELEMENT_9				3209
#define IDS_ELEMENT_10				3210
#define IDS_ELEMENT_11				3211
#define IDS_ELEMENT_12				3212
#define IDS_ELEMENT_13				3213
#define IDS_ELEMENT_14				3214
#define IDS_ELEMENT_15				3215
#define IDS_ELEMENT_16				3216
#define IDS_ELEMENT_17				3217
#define IDS_ELEMENT_18				3218
#define IDS_ELEMENT_19				3219
#define IDS_ELEMENT_20				3220
#define IDS_ELEMENT_21				3221
#define IDS_ELEMENT_22				3222
#define IDS_ELEMENT_23				3223

#define IDS_MULTIPLEMONITORS	3300
#define IDS_UNKNOWNMONITOR	3301
#define IDS_ADVANCEDTITLEFMT	3302

#endif /* __CPL_DESK_RESOURCE_H__ */


