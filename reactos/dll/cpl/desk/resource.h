#ifndef __CPL_DESK_RESOURCE_H__
#define __CPL_DESK_RESOURCE_H__

#include <commctrl.h>

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
#define IDC_SCREENS_CHOICES             1011
#define IDC_SCREENS_POWER_BUTTON        1012
#define IDC_SCREENS_SETTINGS            1013
#define IDC_SCREENS_TESTSC              1014
#define IDC_SCREENS_USEPASSCHK          1015
#define IDC_SCREENS_TIMEDELAY           1016
#define IDC_SCREENS_TIME                1017
#define IDC_SCREENS_DUMMY               5000
#define IDC_SCREENS_DUMMY2              5001

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

#define IDR_POPUP_MENU				2010
#define ID_MENU_CONFIG				2011
#define ID_MENU_PREVIEW				2012
#define ID_MENU_ADD					2013
#define ID_MENU_DELETE				2014


/* Settings Page */

#define IDS_PIXEL				2301

#define IDS_COLOR_4BIT			2904
#define IDS_COLOR_8BIT			2908
#define IDS_COLOR_16BIT			2916
#define IDS_COLOR_24BIT			2924
#define IDS_COLOR_32BIT			2932


/* Appearance Page */
#define IDC_APPEARANCE_PREVIEW 1500

#define IDS_INACTWIN    1510
#define IDS_ACTWIN      1511
#define IDS_WINTEXT     1512
#define IDS_MESSBOX     1513
#define IDS_MESSTEXT    1514
#define IDS_BUTTEXT     1515
#define IDS_NORMAL      1516
#define IDS_DISABLED    1517
#define IDS_SELECTED    1518


#endif /* __CPL_DESK_RESOURCE_H__ */

