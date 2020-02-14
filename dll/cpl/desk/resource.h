#pragma once

/* ids */
#define IDC_DESK_ICON  40
#define IDC_DESK_ICON2 100 /* Needed for theme compatability with Windows. */

#ifndef IDC_STATIC
    #define IDC_STATIC -1
#endif

#include "../../win32/themeui/resource.h"

#define IDD_BACKGROUND 151

/* Background Page */
#define IDC_BACKGROUND_LIST          1000
#define IDC_MONITOR                  1001
#define IDC_BACKGROUND_PREVIEW       1002
#define IDC_BROWSE_BUTTON            1003
#define IDC_COLOR_BUTTON             1004
#define IDC_PLACEMENT_COMBO          1005
#define IDS_BACKGROUND_COMDLG_FILTER 1006

#define IDS_CPLNAME        2000
#define IDS_CPLDESCRIPTION 2001

#define IDS_NONE    2002
#define IDS_CENTER  2003
#define IDS_STRETCH 2004
#define IDS_TILE    2005
#define IDS_FIT     2006
#define IDS_FILL    2007

#define IDB_SPECTRUM_4  208
#define IDB_SPECTRUM_8  209
#define IDB_SPECTRUM_16 210

#define IDR_PREVIEW_MENU 2100
#define ID_MENU_NORMAL   2101
#define ID_MENU_DISABLED 2102
#define ID_MENU_SELECTED 2103

#define IDM_MONITOR_MENU   2110
#define ID_MENU_ATTACHED   2111
#define ID_MENU_PRIMARY    2112
#define ID_MENU_IDENTIFY   2113
#define ID_MENU_PROPERTIES 2114

/* Settings Page */
#define IDS_PIXEL       2301
#define IDS_COLOR_4BIT  2904
#define IDS_COLOR_8BIT  2908
#define IDS_COLOR_16BIT 2916
#define IDS_COLOR_24BIT 2924
#define IDS_COLOR_32BIT 2932

#define IDS_INACTWIN      1510
#define IDS_ACTWIN        1511
#define IDS_WINTEXT       1512
#define IDS_MESSBOX       1513
#define IDS_MESSTEXT      1514
#define IDS_BUTTEXT       1515
#define IDS_CLASSIC_THEME 1516

/* Update these IDs when you change the string id list */
#define IDS_ITEM_FIRST (IDS_ITEM_3D_OBJECTS)
#define IDS_ITEM_LAST  (IDS_ITEM_CAPTION_BUTTONS + 1)

#define IDS_ELEMENT_0  3200
#define IDS_ELEMENT_1  3201
#define IDS_ELEMENT_2  3202
#define IDS_ELEMENT_3  3203
#define IDS_ELEMENT_4  3204
#define IDS_ELEMENT_5  3205
#define IDS_ELEMENT_6  3206
#define IDS_ELEMENT_7  3207
#define IDS_ELEMENT_8  3208
#define IDS_ELEMENT_9  3209
#define IDS_ELEMENT_10 3210
#define IDS_ELEMENT_11 3211
#define IDS_ELEMENT_12 3212
#define IDS_ELEMENT_13 3213
#define IDS_ELEMENT_14 3214
#define IDS_ELEMENT_15 3215
#define IDS_ELEMENT_16 3216
#define IDS_ELEMENT_17 3217
// #define IDS_ELEMENT_18 3218

#define IDS_MULTIPLEMONITORS 3300
#define IDS_UNKNOWNMONITOR   3301
#define IDS_ADVANCEDTITLEFMT 3302

#define IDS_DISPLAY_SETTINGS 3400

#define IDS_APPLY_FAILED        3500
#define IDS_APPLY_NEEDS_RESTART 3501

#define IDS_SLIDEEFFECT 3701
#define IDS_FADEEFFECT  3702

#define IDS_STANDARDEFFECT  3711
#define IDS_CLEARTYPEEFFECT 3712

#define IDS_TIMEOUTTEXT      3800
