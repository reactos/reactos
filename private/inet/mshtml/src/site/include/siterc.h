To be removed

//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       siterc.h
//
//  Contents:   Resource identifiers for Site directory tree
//
//----------------------------------------------------------------------------

#ifndef I_SITERC_H_
#define I_SITERC_H_
#pragma INCMSG("--- Beg 'siterc.h'")

//----------------------------------------------------------------------------
//
// Registered servers
//
//  00900 - 00999    Site registered servers
//  08000 - 08999    Site miscellaneous
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
// Registered servers
//
//----------------------------------------------------------------------------


// HTML Form

#define IDR_BASE_HTMLFORM           900
#define IDS_HTMLFORM_USERTYPEFULL   180
#define IDS_HTMLFORM_USERTYPESHORT  181
#define IDR_HTMLFORM_TOOLBOXBITMAP  905
#define IDR_HTMLFORM_ACCELS         906
#define IDR_HTMLFORM_MENUDESIGN     907
#define IDR_HTMLFORM_MENURUN        908
#define IDR_HTMLFORM_DOCDIR         909


//----------------------------------------------------------------------------
//
// Errors
//
//----------------------------------------------------------------------------

#define IDS_SITE_BASE      7999

#define IDS_ACT_UPDATEPROPS          (IDS_SITE_BASE + 1)
#define IDS_ACT_DDOCSTARTUP          (IDS_SITE_BASE + 2)
#define IDS_ERR_DDOCPOPULATEFAILED   (IDS_SITE_BASE + 3)
#define IDS_ERR_DDOCGENERAL          (IDS_SITE_BASE + 4)
#define IDS_ACT_DDOCSCROLLGENERAL    (IDS_SITE_BASE + 5)
#define IDS_ACT_DDOCSCROLLLINEUP     (IDS_SITE_BASE + 6)
#define IDS_ACT_DDOCSCROLLLINEDOWN   (IDS_SITE_BASE + 7)
#define IDS_ACT_DDOCSCROLLPAGEUP     (IDS_SITE_BASE + 8)
#define IDS_ACT_DDOCSCROLLPAGEDOWN   (IDS_SITE_BASE + 9)
#define IDS_ACT_DDOCSCROLLEND        (IDS_SITE_BASE + 10)

#define IDS_SYNTAX_ERROR             (IDS_SITE_BASE + 12)    // "Syntax error in file %s (line %n): %s"
#define IDS_LAYOUTSELTOOL            (IDS_SITE_BASE + 13)
#define IDS_ACT_DDOCAFTERLOAD        (IDS_SITE_BASE + 14)
#define IDS_LOADTEXT                 (IDS_SITE_BASE + 15)
#define IDS_LOADTEXTUNKNOWNPROP      (IDS_SITE_BASE + 16)
#define IDS_CONTAINERTEXT            (IDS_SITE_BASE + 17)
#define IDS_TEXTSITETEXT             (IDS_SITE_BASE + 18)
#define IDS_HTMLDEFAULTFONT          (IDS_SITE_BASE + 19)

#define IDS_NAMEDPROPERTIES          (IDS_SITE_BASE + 21)
#define IDS_UNNAMEDPROPERTIES        (IDS_SITE_BASE + 22)

#define IDS_ERR_SAVEPICTUREAS        (IDS_SITE_BASE + 26)
#define IDS_ERR_SETWALLPAPER         (IDS_SITE_BASE + 27)

#define IDS_RUNAWAYSCRIPT            (IDS_SITE_BASE + 29)
#define IDS_PROTECTEDFROMUNSAFEOCX   (IDS_SITE_BASE + 31)
#define IDS_PROTECTEDFROMOCXINIT     (IDS_SITE_BASE + 32)
#define IDS_REPOSTFORMDATA           (IDS_SITE_BASE + 33)
#define IDS_OCXDISABLED              (IDS_SITE_BASE + 34)
#define IDS_MISMATCHEDXML            (IDS_SITE_BASE + 35)
#define IDS_DEBUGCONTINUE            (IDS_SITE_BASE + 36)
#define IDS_FMTDEBUGCONTINUE         (IDS_SITE_BASE + 37)
#define IDS_ERR_SETDESKTOPITEM       (IDS_SITE_BASE + 38)

#define IDC_SITE_CURSORBASE         9800
#define IDC_HYPERLINK               9801
#define IDC_HYPERLINK_OFFLINE       9802
#define IDC_HYPERLINK_WAIT          9803

// Menu index
#define MENU_INDEX_EDIT             1
#define MENU_INDEX_VIEW             2
#define MENU_INDEX_INSERT           3
#define MENU_INDEX_FORMAT           4
#define MENU_INDEX_TABLE            5

#define IDS_HTMLFORM_SAVE                   8114
#define IDS_2DFORM_SAVE                     8115
#define IDS_SAVEPICTUREAS_GIF               8116
#define IDS_SAVEPICTUREAS_JPG               8117
#define IDS_SAVEPICTUREAS_BMP               8118
#define IDS_SAVEPICTUREAS_XBM               8119
#define IDS_SAVEPICTUREAS_ART               8120
#define IDS_SAVEPICTUREAS_WMF               8121
#define IDS_SAVEPICTUREAS_EMF               8122
#define IDS_SAVEPICTUREAS_AVI               8123
#define IDS_SAVEPICTUREAS_MPG               8124
#define IDS_SAVEPICTUREAS_MOV               8125
#define IDS_SAVEPICTUREAS_ORIGINAL          8126
#define IDS_UNTITLED_BITMAP                 8127
#define IDS_UNTITLED                        8128
#define IDS_WALLPAPER_BMP                   8129

// default document security property
#define IDS_DEFAULT_DOC_SECURITY_PROP       8130

// Jave Script prompt() dialog
#define IDD_PROMPT                          8131
#define IDC_PROMPT_PROMPT                   8132
#define IDC_PROMPT_EDIT                     8133

// Plugin/ActiveX Viewer Not Installed dialog
#define IDD_PLUGIN_UPGRADE                  8134
#define IDC_PLUGIN_UPGRADE_CHECK            8135
#define IDC_PLUGIN_UPGRADE_EXTENSION        8136
#define IDC_PLUGIN_UPGRADE_MIME_TYPE        8137

// Save As string for PNG images
#define IDS_SAVEPICTUREAS_PNG               8138

//+----------------------------------------------------------------------------
//
// Progress status text strings
//
//-----------------------------------------------------------------------------

#define IDS_BINDSTATUS_DOWNLOADING                  8154
#define IDS_BINDSTATUS_DOWNLOADINGDATA_PICTURE      8155
#define IDS_BINDSTATUS_GENERATINGDATA_TEXT          8156
#define IDS_BINDSTATUS_DOWNLOADINGDATA_TEXT         8157
#define IDS_BINDSTATUS_INSTALLINGCOMPONENTS         8160
#define IDS_BINDSTATUS_DOWNLOADINGDATA_BITS         8167

#define IDS_DONE                                    8169

#define IDS_LOADINGTABLE                            8170
#define IDS_DATABINDING                             8171

#define IDS_FRIENDLYURL_SHORTCUTTO                  8172
#define IDS_FRIENDLYURL_AT                          8173
#define IDS_FRIENDLYURL_SENDSMAILTO                 8174
#define IDS_FRIENDLYURL_LOCAL                       8175
#define IDS_FRIENDLYURL_GOPHER                      8176
#define IDS_FRIENDLYURL_FTP                         8177
#define IDS_FRIENDLYURL_SECUREWEBSITE               8178

#define IDS_CANNOTLOAD                              8193
#define IDS_ONBEFOREUNLOAD_PREAMBLE                 8194
#define IDS_ONBEFOREUNLOAD_POSTAMBLE                8195

#define IDS_DEFAULT_ISINDEX_PROMPT                  8196


// Add controls to the save as dialog.
#define IDC_SAVE_CHARSET                            8194

//+----------------------------------------------------------------------------
//
// string used in conversion GetIDsOfNames
//
//----------------------------------------------------------------------------

#define IDS_DISPID_FIRST                8200                       // matches:
#define IDS_DISPID_FONTNAME             (IDS_DISPID_FIRST +  1)    // DISPID_CommonCtrl_FONTNAME
#define IDS_DISPID_FONTSIZE             (IDS_DISPID_FIRST +  2)    // DISPID_CommonCtrl_FONTSIZE
#define IDS_DISPID_FONTBOLD             (IDS_DISPID_FIRST +  3)    // DISPID_CommonCtrl_FONTBOLD
#define IDS_DISPID_FONTITAL             (IDS_DISPID_FIRST +  4)    // DISPID_CommonCtrl_FONTITAL
#define IDS_DISPID_FONTUNDER            (IDS_DISPID_FIRST +  5)    // DISPID_CommonCtrl_FONTUNDER
#define IDS_DISPID_FONTSTRIKE           (IDS_DISPID_FIRST +  6)    // DISPID_CommonCtrl_FONTSTRIKE
#define IDS_DISPID_BACKCOLOR            (IDS_DISPID_FIRST +  7)    // DISPID_BACKCOLOR
#define IDS_DISPID_BORDERCOLOR          (IDS_DISPID_FIRST +  9)    // DISPID_BORDERCOLOR
#define IDS_DISPID_BORDERSTYLE          (IDS_DISPID_FIRST + 10)    // DISPID_BORDERSTYLE
#define IDS_DISPID_TEXTALIGN            (IDS_DISPID_FIRST + 11)    // DISPID_CommonCtrl_TextAlign
#define IDS_DISPID_SPECIALEFFECT        (IDS_DISPID_FIRST + 12)    // DISPID_CommonCtrl_SpecialEffect
#define IDS_DISPID_FONTSUPERSCRIPT      (IDS_DISPID_FIRST + 13)    // DISPID_CommonCtrl_FONTSUPERSCRIPT
#define IDS_DISPID_FONTSUBSCRIPT        (IDS_DISPID_FIRST + 14)    // DISPID_CommonCtrl_FONTSUBSCRIPT

//+----------------------------------------------------------------------------
//
// string used by script window
//
//----------------------------------------------------------------------------

#define IDS_OMWINDOW_FIRST             8300

#define IDS_VAR2STR_VTERROR             (IDS_OMWINDOW_FIRST + 0)
#define IDS_VAR2STR_VTNULL              (IDS_OMWINDOW_FIRST + 1)
#define IDS_VAR2STR_VTBOOL_TRUE         (IDS_OMWINDOW_FIRST + 2)
#define IDS_VAR2STR_VTBOOL_FALSE        (IDS_OMWINDOW_FIRST + 3)

//+----------------------------------------------------------------------------
//
//  Printing constants
//
//----------------------------------------------------------------------------

#define IDS_PRINT_FIRST                 8400

#define IDS_PRINT_URLTITLE              8400
#define IDS_PRINT_URLCOL1HEAD           8401
#define IDS_PRINT_URLCOL2HEAD           8402
#define IDS_DEFAULTHEADER               8403
#define IDS_DEFAULTFOOTER               8404
#define IDS_DEFAULTMARGINTOP            8405
#define IDS_DEFAULTMARGINBOTTOM         8406
#define IDS_DEFAULTMARGINLEFT           8407
#define IDS_DEFAULTMARGINRIGHT          8408

#ifdef  UNIX
#define IDU_DEFAULTPRINTNAME  		8430
#define IDU_DEFAULTPRINTCOMMAND  	8431
#endif  // UNIX
//+----------------------------------------------------------------------------
//
//  Button caption constants
//
//----------------------------------------------------------------------------

#define IDS_BUTTONCAPTION_FIRST         8500

#define IDS_BUTTONCAPTION_RESET         8500
#define IDS_BUTTONCAPTION_SUBMIT        8501
#define IDS_BUTTONCAPTION_UPLOAD        8502

//+----------------------------------------------------------------------------
//
//   MailTo constants
//
//----------------------------------------------------------------------------

#define IDS_MAILTO_FIRST                8600
#define IDS_MAILTO_DEFAULTSUBJECT       8600
#define IDS_MAILTO_MAILCLIENTNOTFOUND   8601
#define IDS_MAILTO_SUBMITALERT          8602


//+-------------------------------------------------------------------------
//
//  Property frame
//  (peterlee) moved to shdocvw
//--------------------------------------------------------------------------
#ifdef NEVER
        #define IDD_PROPFRM_DELAYCOMMIT     5400
        #define IDD_PROPFRM_IMMEDCOMMIT     5401
        #define IDC_PROPFRM_TABS            5402
        #define IDC_PROPFRM_APPLY           5403
        #define IDR_PROPFRM_ICON            5404
        #define IDI_PROPDLG_COMBO           44
        #define IDI_PROPDLG_TABS            IDC_PROPFRM_TABS
        #define IDI_PROPDLG_APPLY           IDC_PROPFRM_APPLY
        #define IDR_PROPERTIES_DIALOG       IDD_PROPFRM_DELAYCOMMIT

        #define IDS_PROPFRM_APPLY           5416
        #define IDS_PROPFRM_CLOSE           5417
        #define IDS_PROPFRM_CHANGES         5418
        #define IDS_PROPFRM_DEFCAPTION      5419
        #define IDS_PROPFRM_MULTOBJCAPTION  5420
        #define IDS_PROPFRM_TYPECAPTION     5421
        #define IDS_PROPFRM_UNDOCHANGE      5422
        #define IDS_PROPFRM_SIZETOFIT       5423
#endif // NEVER

#define IDS_CONTAINERPAGE_ACCEL     5424

#if DBG == 1

#define IDS_COLOR_BLACK             5430
#define IDS_COLOR_NAVY              5431
#define IDS_COLOR_BLUE              5432
#define IDS_COLOR_CYAN              5433
#define IDS_COLOR_RED               5434
#define IDS_COLOR_LIME              5435
#define IDS_COLOR_GRAY              5436
#define IDS_COLOR_GREEN             5437
#define IDS_COLOR_YELLOW            5438
#define IDS_COLOR_PINK              5439
#define IDS_COLOR_VIOLET            5440
#define IDS_COLOR_WHITE             5441
// 5442 and 5443 replaced by html dialogs

#endif // DBG == 1

#define IDS_UPLOADFILE              5444

#ifdef UNIX
#  define IDS_PRINT_ERROR             5445
#  define IDS_PRINT_ERROR_MSG         5446
#endif

#ifndef NO_HTML_DIALOG
// find resources are now located in shdocvw (peterlee)
//#define IDR_FINDDIALOG              _T("find.dlg")
//#define IDR_BIDIFINDDIALOG          _T("bidifind.dlg")
#define IDR_REPLACEDIALOG           _T("replace.dlg")
#define IDR_FORPARDIALOG            _T("forpar.dlg")
#define IDR_FORCHARDIALOG           _T("forchar.dlg")
#define IDR_GOBOOKDIALOG            _T("gobook.dlg")
#define IDR_INSIMAGEDIALOG          _T("insimage.dlg")
#define IDR_EDLINKDIALOG            _T("edlink.dlg")
#define IDR_EDBOOKDIALOG            _T("edbook.dlg")
#endif // NO_HTML_DIALOG

// property grids removed (peterlee)
#ifdef NEVER
        #ifndef NO_PROPERTY_PAGE
        #define IDR_BACKGRNDPPG             _T("backgrnd.ppg")
        #endif // NO_PROPERTY_PAGE
#endif // NEVER

// New Encoding Menu
#define RES_STRING_ENCODING_MORE            4700

#pragma INCMSG("--- End 'siterc.h'")
#else
#pragma INCMSG("*** Dup 'siterc.h'")
#endif
