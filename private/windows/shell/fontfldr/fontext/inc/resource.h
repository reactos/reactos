/**************************************************************************
 * resource.h - Font assistant resource constants.
 *
 * Copyright (C) 1992-93 ElseWare Corporation.    All rights reserved.
 ***************************************************************************/

#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include <dlgs.h>

//*********************************************************************
// Icons
//
#define IDI_ICON                        1

#define IDI_FIRSTFONTICON               2  // First in image list.

#define IDI_TTF                         2
#define IDI_FON                         3
#define IDI_TTC                         4
#define IDI_T1                          5
#define IDI_OTFp                        6  // "OTp" icon
#define IDI_OTFt                        7  // "OTt" icon

#define IDI_LASTFONTICON                7  // Update this if you add another.

#define TYPE1ICON                     IDI_T1

   // Extracted from SHELL32.DLL
   //
#define IDI_X_LINK                      30
#define IDI_X_NUKE_FILE                 161
#define IDI_X_DELETE                    142


//*********************************************************************
// Bitmaps
//
#define IDB_TOOLICONS                   15

//*********************************************************************
// Accelerator table
//
#define ACCEL_DEF                       20

//*********************************************************************
// Menus and Menu Items
//
#define MENU_DEFSHELLVIEW               29

#define IDM_FILE_SAMPLE                 30
#define IDM_FILE_PRINT                  31
#define IDM_FILE_INSTALL                32
#define IDM_FILE_LINK                   33
#define IDM_FILE_DEL                    34
#define IDM_FILE_RENAME                 35
#define IDM_FILE_PROPERTIES             36

#define IDM_EDIT_SELECTALL              37
#define IDM_EDIT_SELECTINVERT           38
#define IDM_EDIT_CUT                    39
#define IDM_EDIT_COPY                   40
#define IDM_EDIT_PASTE                  41

// --------------------------------------------------------
// The following IDs need to be kept in sequetial order: 
//    IDM_VIEW_ICON to IDM_POINT_DOWN
//
#define IDM_VIEW_ICON                   42
#define IDM_VIEW_LIST                   43
#define IDM_VIEW_PANOSE                 44
#define IDM_VIEW_DETAILS                45

#define IDM_VIEW_ACTUAL                 46
#define IDM_EDIT_UNDO                   47  
#define IDM_VIEW_PREVIEW                48
// --------------------------------------------------------

#define IDM_VIEW_VARIATIONS             49

// Popups for context menus.
#define IDM_POPUPS                      58
#define IDM_POPUP_NOITEM                59
#define IDM_POPUP_DRAGDROP              60


// Other messages delivered through WM_COMMAND
//
#define IDM_IDLE                        61
#define IDM_POPUP_MOVE                  62
#define IDM_POPUP_COPY                  63
#define IDM_POPUP_LINK                  64
#define IDM_POPUP_CANCEL                65

#define IDM_HELP_TOPIC                  66
#define IDM_HELP_ABOUT                  67

//*********************************************************************
// Dialogs
//
#define ID_DLG_FONT2                   70  // Install dialog
#define ID_LB_ADD      ctlLast+1
#define ID_SS_PCT      ctlLast+2

#define ID_DLG_MAIN                    76
#define ID_TXT_SIM                     77
#define ID_CB_PANOSE                   78

#define ID_DLG_PROPPAGE                80

#define ID_DLG_OPTIONS                 85
#define IDC_TTONLY                     86

#define DLG_INSTALL                    90
#define IDD_INSTALL                    91
#define COLOR_SAVE                     92
#define IDD_HELP                       93

#define DLG_BROWSE                     100
#define IDD_BROWSE                     101

#define DLG_PROGRESS                   110
#define DLG_INSTALL_PS                 120

#define DLG_COPYRIGHT_NOTIFY           130
#define IDC_COPYRIGHT_FONTNAME         131
#define IDC_COPYRIGHT_VENDORNAME       132
//
// Help ids
//

#define IDH_HELPFIRST        5000
#define IDH_SYSMENU     (IDH_HELPFIRST + 2000)
#define IDH_MBFIRST     (IDH_HELPFIRST + 2001)
#define IDH_DLG_FONT2   (IDH_HELPFIRST + 2002)
#define IDH_MBLAST      (IDH_HELPFIRST + 2099)
#define IDH_DLGFIRST    (IDH_HELPFIRST + 3000)

//#define IDH_DLG_REMOVEFONT  (IDH_DLGFIRST + DLG_REMOVEFONT)
#define IDH_DLG_BROWSE      (IDH_DLGFIRST + DLG_BROWSE)

#define IDH_DLG_INSTALL_PS     (IDH_DLGFIRST + DLG_INSTALL_PS)
//#define IDH_DLG_REMOVEFONT_PS  (IDH_DLGFIRST + DLG_REMOVEFONT_PS)

//
//  Progress dialog control ids
//

#define ID_INSTALLMSG           42
#define ID_PROGRESSMSG          43
#define ID_BAR                  44
#define ID_OVERALL              45


//
//  Font dialogs control ids
//

#define IDD_YESALL        122

#define FONT_REMOVEMSG    418
#define FONT_REMOVECHECK  419
#define FONT_TRUETYPEONLY 420
#define FONT_CONVERT_PS   431
#define FONT_INSTALL_PS   432
#define FONT_COPY_PS      433
#define FONT_REMOVE_PS    434
#define FONT_INSTALLMSG   435




//*********************************************************************
// Strings
//

/* General messsages.  If the message is a constant string, we call
 * it IDS_MSG_xxx or IDX_TXT_xxx.
 * If we substitute something (via sprintf or similar), its IDS_FMT_xxx
 */

#define IDS_FONTS_FOLDER        140    // The name of the fonts folder in the windows dir.

#define IDS_MSG_CAPTION         151
#define IDS_MSG_NOVERSION       153
#define IDS_MSG_NSFMEM          154

#define IDS_MSG_PANOSE          163

#define IDS_MSG_ALLFILTER       165
#define IDS_MSG_NORMALFILTER    166


#define IDS_FMT_VERSION         177

/* Font installer messages */

#define IDSI_CAP_NOINSTALL      210
#define IDSI_CAP_NOCREATE       211
#define IDSE_CAP_CREATERR       212

#define IDSI_MSG_TTDISABLED     215
#define IDSI_MSG_NOFONTS        216
#define IDSI_MSG_COPYCONFIRM    217
#define IDSI_MSG_DEFDIR         218

#define IDSI_FMT_BADINSTALL     222
#define IDSI_FMT_ISINSTALLED    223
#define IDSI_FMT_RETRIEVE       224
#define IDSI_FMT_COMPRFILE      225
#define IDSI_FMT_FILEFNF        229
#define IDSI_UNKNOWN_VENDOR     232



#define INSTALL0        250
#define INSTALL1        251
#define INSTALL2        252
#define INSTALL3        253
#define INSTALL4        254
#define INSTALL5        255
#define INSTALL6        256
#define INSTALL7        257
#define INSTALL8        258
#define INSTALL9        259

// COLUMN String info for the List Views
#define IDS_PAN_COL1          301
#define IDS_PAN_COL2          302

#define IDS_FILE_COL1         303
#define IDS_FILE_COL2         304
#define IDS_FILE_COL3         305
#define IDS_FILE_COL4         306
#define IDS_FILE_COL5         307
#define IDS_ATTRIB_CHARS      308
#define IDS_FONTSAMPLE_TEXT   309
#define IDS_FONTSAMPLE_SYMBOLS 310

#define IDS_VIEW_ICON         311
#define IDS_VIEW_LIST         312
#define IDS_VIEW_PANOSE       313
#define IDS_VIEW_DETAILS      314


// File types
#define IDS_FONT_FILE         407
#define IDS_TT_FILE           408

#define IDS_NO_PAN_INFO       409
#define IDS_PAN_VERY_SIMILAR  410
#define IDS_PAN_SIMILAR       411
#define IDS_PAN_NOT_SIMILAR   412
#define IDS_FMT_FILEK         413
#define IDS_TTC_CONCAT        414


// Strings for Shell Extension
//
#define IDS_EXT_INSTALL       420
#define IDS_EXT_INSTALL_HELP  421

//
// Strings for font file validation errors.
// FVS = Font Validation Status.
//
#define IDS_FMT_FVS_PREFIX      450
#define IDS_FMT_FVS_FILEOPEN    451
#define IDS_FMT_FVS_FILECREATE  452
#define IDS_FMT_FVS_FILEEXISTS  453
#define IDS_FMT_FVS_BADVERSION  454
#define IDS_FMT_FVS_INVFONTFILE 455
#define IDS_FMT_FVS_FILEIO      466
#define IDS_FMT_FVS_INTERNAL    467


// Status messages.
//

#define IDST_FILE_SAMPLE       490
#define IDST_FILE_PRINT        491
#define IDST_FILE_INSTALL      492
#define IDST_FILE_LINK         493

#define IDST_FILE_DEL          494
#define IDST_FILE_RENAME       495
#define IDST_FILE_PROPERTIES   496
                                
#define IDST_EDIT_SELECTALL    497
#define IDST_EDIT_SELECTINVERT 498
#define IDST_EDIT_CUT          499
#define IDST_EDIT_COPY         500
#define IDST_EDIT_PASTE        501

#define IDST_VIEW_ICON         502
#define IDST_VIEW_LIST         503
#define IDST_VIEW_PANOSE       504
#define IDST_VIEW_DETAILS      505

#define IDST_EDIT_UNDO         506
                                
#define IDST_VIEW_VARIATIONS   509
#define IDST_VIEW_PREVIEW      510
#define IDST_HELP_TOPICS       511

#define IDS_SELECTED_FONT_COUNT         520
#define IDS_TOTAL_FONT_COUNT            521
#define IDS_TOTAL_AND_HIDDEN_FONT_COUNT 522
#define IDSI_FMT_DELETECONFIRM          523

#define IDS_NEXTREBOOT          524
#define IDS_INSTALL_MUTEX_WAIT_FAILED 525

#define INSTALLIT               530     /* Two messages for installing */

#define MYFONT                  600

#define IDC_LIST_OF_FONTS		601
#endif   //  __RESOURCE_H__
