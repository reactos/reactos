////////////////////////////////////////////////////////////////////////////
// RESOURCE.H
//
// Resource IDs
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     08/26/96    Created
/////////////////////////////////////////////////////////////////////////////
#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#define IDS_SCREENSAVER_DESC                1
#define IDR_SCREENSAVER                     1
#define ID_APP                              100

/////////////////////////////////////////////////////////////////////////////
// Icons
/////////////////////////////////////////////////////////////////////////////
#define IDI_SCHEDULE                        0x0101

/////////////////////////////////////////////////////////////////////////////
// Bitmaps
/////////////////////////////////////////////////////////////////////////////
#define IDB_PREVIEW                         0x0100

#define IDB_PROPERTIES_DEFAULT              0x0101
#define IDB_PROPERTIES                      0x0102
#define IDB_CLOSE                           0x0103
#define IDB_IMGLIST_FIRST                   0x0104
#define IDB_CHECKED                         0x0104
#define IDB_UNCHECKED                       0x0105

/////////////////////////////////////////////////////////////////////////////
// Toolbar bitmap info
/////////////////////////////////////////////////////////////////////////////
#define CX_PROPERTIES_BUTTONBITMAP          20
#define CY_PROPERTIES_BUTTONBITMAP          20
#define CX_CLOSE_BUTTONBITMAP               12
#define CY_CLOSE_BUTTONBITMAP               11
    // Dimension of toolbar button images

#define IDTB_PROPERTIES                     0
#define IDTB_CLOSE                          0
    // Toolbar button bitmap offsets

/////////////////////////////////////////////////////////////////////////////
// String table definitions 
/////////////////////////////////////////////////////////////////////////////
#define IDS_DLG_CAPTION                     0x0100

#define IDS_TB_PROPERTIES_TOOLTIP           0x0120
#define IDS_TB_CLOSE_TOOLTIP                0x0121
    // ToolBar control ToolTip IDs. All IDs must match with the IDM_* below!

#define IDS_ERROR_REQUIREMENTSNOTMET        0x0300
    // Error message string IDs.

/////////////////////////////////////////////////////////////////////////////
// Dialog Box/Control definitions
/////////////////////////////////////////////////////////////////////////////
#define IDC_STATIC                          -1

#define IDD_PAGE1_DIALOG                    0x0100
    // Page #1 (General) dialog box

#define IDC_SUBSCRIPTION_LIST               0x0101
#define IDC_CHANNEL_TIME                    0x0102
#define IDC_CHANNEL_TIME_SPIN               0x0103
#define IDC_PLAY_SOUNDS                     0x0104
#define IDC_NAVIGATE_GROUP_BOX              0x0105
#define IDC_NAVIGATE_CLICK                  0x0106
#define IDC_NAVIGATE_ALTCLICK               0x0107
    // Page #1 (General) dialog box controls

#ifdef FEATURE_FONT_SETTINGS
#define IDC_FONT_SETTINGS                   0x0108
#endif  // FEATURE_FONT_SETTINGS

#define IDD_CREATESUBS_DIALOG               0x0200
    // Create subscription dialog box

#define IDC_DONTASKAGAIN                    0x0201
    // Create subscription dialog box controls

/////////////////////////////////////////////////////////////////////////////
// Menu/Control message IDs
/////////////////////////////////////////////////////////////////////////////
#define IDM_TOOLBAR_PROPERTIES              0x0120
#define IDM_TOOLBAR_CLOSE                   0x0121
    // Toobar button IDs

#endif  // __RESOURCE_H__

