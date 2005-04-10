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
#define IDC_DESK_ICON                   1

#define IDC_STATIC                      -1

#define IDD_BACKGROUND                  100
#define IDD_SCREENSAVER                 101
#define IDD_APPEARANCE                  102
#define IDD_SETTINGS                    103

/* Background Page */
#define IDC_BACKGROUND_LIST             1000
#define IDC_BACKGROUND_PREVIEW          1001
#define IDC_BROWSE_BUTTON               1002
#define IDC_COLOR_BUTTON                1003
#define IDC_PLACEMENT_COMBO             1004

#define IDS_CPLNAME                 2000
#define IDS_CPLDESCRIPTION          2001

#define IDS_NONE                    2002
#define IDS_CENTER                  2003
#define IDS_STRETCH                 2004
#define IDS_TILE                    2005

#define IDC_SETTINGS_DEVICE	       201
#define IDC_SETTINGS_BPP             202
#define IDC_SETTINGS_RESOLUTION      203
#define IDC_SETTINGS_RESOLUTION_TEXT 204
#define IDC_SETTINGS_ADVANCED        205

/* Settings Page */

#define IDS_PIXEL				2301

#define IDS_COLOR_4BIT			2904
#define IDS_COLOR_8BIT			2908
#define IDS_COLOR_16BIT			2916
#define IDS_COLOR_32BIT			2932  

#endif /* __CPL_DESK_RESOURCE_H__ */

