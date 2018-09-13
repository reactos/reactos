To be removed

//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       corerc.h
//
//  Contents:   Resource identifiers for Core project.
//
//----------------------------------------------------------------------------
//
//  Resource identifier ranges.
//
//  Low resource identifiers are reserved for servers listed in the registry.
//  We need to do this because the ExtractIcon API uses the index of the icon,
//  not the resource identifier of icon.  By reserving this range for registered
//  servers, we can insure that the registered icon indices are correct.
//
//  00000 - 00019    Core registered servers
//  00020 - 00159    Ctrl '96 registered servers
//  00160 - 00199    Form registered servers
//  00200 - 00899    Ctrl '97 registered servers
//  00900 - 00999    Site registered servers (overlaps with ddoc)
//
//  02000 - 03999    Core miscellaneous
//  04000 - 05999    Ctrl miscellaneous
//  06000 - 07999    Form miscellaneous
//  08000 - 09999    Site miscellaneous (overlaps with ddoc)
//
//  10000 - 19999    Menu help string = IDS_MENUHELP(idm)
//  20000 - 29999    Tooltip text = IDS_TOOLTIP(idm)
//
//----------------------------------------------------------------------------
//
//  Naming convention
//
//  IDR_     Resource. Id must be unique per resource type, prefer
//           unique across resource types.
//  IDM_     Menu item, unique across product.
//  IDI_     Dialog item. Must be unique in dialog.
//  IDS_     String table element.
//  IDS_EA_  Action part of error message.
//  IDS_EE_  Error part of of error message.
//  IDS_ES_  Solution part of error message.
//  IDS_MSG_ Informational message.
//  IDS_E_   HRESULT to text mapping.
//
//----------------------------------------------------------------------------

#ifndef I_CORERC_H_
#define I_CORERC_H_
#pragma INCMSG("--- Beg 'corerc.h'")

#define IDS_MENUHELP(idm) (10000 + (idm))
#define IDS_TOOLTIP(idm)  (20000 + (idm))

//  resource ID offsets for class descriptor information
#define IDOFF_TOOLBOXBITMAP   5
#define IDOFF_ACCELS          6
#define IDOFF_MENU            7
#define IDOFF_MGW             8

// unfortunately, these macros can't be used in defining
//  the symbols below: the resource compiler doesn't like them.
#define IDS_USERTYPEFULL(base)  ((base)/5)
#define IDS_USERTYPESHORT(base) ((base)/5 + 1)
#define IDR_MENU(base)          ((base) + IDOFF_MENU)
#define IDR_TOOLBOXBITMAP(base) ((base) + IDOFF_TOOLBOXBITMAP)
#define IDR_ACCELS(base)        ((base) + IDOFF_ACCELS)

//----------------------------------------------------------------------------
//
// Registered servers (00xx)
//
//----------------------------------------------------------------------------

// Form

#define IDR_FORM_ICON             5

#define IDR_ACCELS_SITE_RUN             40
#define IDR_ACCELS_SITE_DESIGN          41
#define IDR_ACCELS_INPUTTXT_RUN         42
#define IDR_ACCELS_INPUTTXT_DESIGN      43
#define IDR_ACCELS_TCELL_RUN            44
#define IDR_ACCELS_TCELL_DESIGN         45
#define IDR_ACCELS_FRAMESET_RUN         46
#define IDR_ACCELS_FRAMESET_DESIGN      47
#define IDR_ACCELS_BODY_RUN             48
#define IDR_ACCELS_BODY_DESIGN          49
#define IDR_ACCELS_TXTSITE_RUN          50
#define IDR_ACCELS_TXTSITE_DESIGN       51


//+-------------------------------------------------------------------------
//
//  Cursors (20xx)
//
//--------------------------------------------------------------------------

#define IDC_SPLITTERV                   2000    // Vertical splitter icon
#define IDC_SPLITTERH                   2001    // Horizontal splitter icon
#define IDC_SELBAR                      2008    // Text select cursor for text site
#define IDC_SCROLLEAST                  2015
#define IDC_SCROLLNE                    2016
#define IDC_SCROLLNORTH                 2017
#define IDC_SCROLLNW                    2018
#define IDC_SCROLLSE                    2019
#define IDC_SCROLLSOUTH                 2020
#define IDC_SCROLLSW                    2021
#define IDC_SCROLLWEST                  2022

#define IDB_NOTLOADED                   2030
#define IDB_MISSING                     2031

//----------------------------------------------------------------------------
//
// Squeeze files (21xx)
//
//----------------------------------------------------------------------------

#define RT_DOCFILE                      256
#define RT_FILE                         2110

//+------------------------------------------------------------------------
//
//  Strings -- packed for size reason; start at multiple of 16.
    //      No existing ID can be changed once localization occurs
//
//-------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  Error strings
//
//-------------------------------------------------------------------------

#define IDS_E_CMDNOTSUPPORTED           2206
#define IDS_ERR_OD_E_OUTOFMEMORY        2207

//+------------------------------------------------------------------------
//
//  cdbase strings
//
//-------------------------------------------------------------------------

// 2210 starts string table chunk
#define IDS_USERTYPEAPP                 2212    // Microsoft Forms 2.0
#define IDS_MESSAGE_BOX_TITLE           2213    // Microsoft Forms
#define IDS_ERROR_SOLUTION              2214    // Solution:\n<0s>
#define IDS_UNKNOWN_ERROR               2215    // Unknown error <0x>
#define IDS_EA_SETTING_PROPERTY         2216    // Could not set property ...
#define IDS_EA_GETTING_PROPERTY         2217    // Could not get property ...
#define IDS_EE_INVALID_PROPERTY_VALUE   2218    // The value entered is not valid ..
#define IDS_ES_ENTER_VALUE_IN_RANGE     2220    // Enter a value between..
#define IDS_ES_ENTER_VALUE_GT_ZERO      2221    // Enter a value greater than
#define IDS_ES_ENTER_VALUE_GE_ZERO      2222    // Enter a value greater than
#define IDS_MSG_SAVE_MODIFIED_OBJECT    2223    // Save modified object?
#define IDS_EE_INVALIDPICTURETYPE       2227    // CTL_E_INVALIDPICTURETYPE mouse icon must be a mouse icon
#define IDS_EE_SETNOTSUPPORTEDATRUNTIME 2321    // Set property is not support at runtime.
#define IDS_EE_CANTMOVEFOCUSTOCTRL      2322    // can't move focus to control because...
#define IDS_EE_METHODNOTAPPLICABLE      2324
#define IDS_ES_ENTER_PROPER_VALUE       2325    // Enter a proper value
#define IDS_EA_CALLING_METHOD           2326    // Could not call method ...
#define IDS_EE_CONTROLNEEDSFOCUS        2327    // The control needs to have the focus
#define IDS_ES_CONTROLNEEDSFOCUS        2328    // Try setting the focus to the control using the SetFocus method
#define IDS_EE_UNEXPECTED               2329    // E_UNEXPECTED
#define IDS_EE_FAIL                     2330    // E_FAIL
#define IDS_EE_INVALIDPICTURE           2331    // CTL_E_INVALIDPICTURE
#define IDS_EE_INVALIDPROPERTYARRAYINDEX 2332   // CTL_E_INVALIDPROPERTYARRAYINDEX
#define IDS_EE_INVALIDPROPERTYVALUE     2333    // CTL_E_INVALIDPROPERTYVALUE
#define IDS_EE_OVERFLOW                 2334    // CTL_E_OVERFLOW
#define IDS_EE_PERMISSIONDENIED         2335    // CTL_E_PERMISSIONDENIED
#define IDS_EE_INVALIDARG               2336    // E_INVALIDARG
#define IDS_EE_NOTLICENSED              2337    // CLASS_E_NOTLICENSED
#define IDS_EE_INVALIDPASTETARGET       2338    // CTL_E_INVALIDPASTETARGET
#define IDS_EE_INVALIDPASTESOURCE       2339    // CTL_E_INVALIDPASTESOURCE

#define IDS_UNKNOWN                     2340

#define IDS_EE_INTERNET_INVALID_URL         2341
#define IDS_EE_INTERNET_NAME_NOT_RESOLVED   2342
#define IDS_EE_INET_E_UNKNOWN_PROTOCOL      2343
#define IDS_EE_INET_E_REDIRECT_FAILED       2344
#define IDS_EE_MISMATCHEDTAG                2345    // CTL_E_MISMATCHEDTAG
#define IDS_EE_INCOMPATIBLEPOINTERS         2346    // CTL_E_INCOMPATIBLEPOINTERS
#define IDS_EE_UNPOSITIONEDPOINTER          2347    // CTL_E_UNPOSITIONEDPOINTER
#define IDS_EE_UNPOSITIONEDELEMENT          2348    // CTL_E_UNPOSITIONEDELEMENT

//+------------------------------------------------------------------------
//
//  formkrnl strings
//
//-------------------------------------------------------------------------

#define IDS_CTRLPROPERTIES              2229
#define IDS_NAMEDCTRLPROPERTIES         2230
#define IDS_EA_PASTE_CONTROL            2235
#define IDS_EA_INSERT_CONTROL           2236
#define IDS_MSG_FIND_DIALOG_HACK        2237

//----------------------------------------------------------------------------
//
// Misc strings
//
//----------------------------------------------------------------------------

//  The following IDS's must be kept in order, such that the
//    string for a given unit has the id IDS_UNITS_BASE + units
//    Otherwise, the StringToHimetric and HimetricToString functions
//    in himetric.cxx will break.  (chrisz)

#define IDS_UNITS_BASE                  2240
#define IDS_UNITS_INCH                  (IDS_UNITS_BASE+0)
#define IDS_UNITS_CM                    (IDS_UNITS_BASE+1)
#define IDS_UNITS_POINT                 (IDS_UNITS_BASE+2)


//----------------------------------------------------------------------------
//
// Undo strings
//
//----------------------------------------------------------------------------

#define IDS_UNDO                        (IDS_UNITS_POINT + 1)
#define IDS_REDO                        (IDS_UNDO + 1)
#define IDS_CANTUNDO                    (IDS_UNDO + 2)
#define IDS_CANTREDO                    (IDS_UNDO + 3)
#define IDS_UNDONEWCTRL                 (IDS_UNDO + 4)
#define IDS_UNDODELETE                  (IDS_UNDO + 5)
#define IDS_UNDOPROPCHANGE              (IDS_UNDO + 6)
#define IDS_UNDOMOVE                    (IDS_UNDO + 7)
#define IDS_UNDODRAGDROP                (IDS_UNDO + 9)
#define IDS_UNDOPASTE                   (IDS_UNDO + 15)
#define IDS_UNDOTYPING                  (IDS_UNDO + 16)
#define IDS_UNDOGENERICTEXT             (IDS_UNDO + 19)
#define IDS_UNDOCHANGEVALUE             (IDS_UNDO + 20)
#define IDS_UNDOBACKSPACE               (IDS_UNDO + 21)

//----------------------------------------------------------------------------
//
// Misc (25xx)
//
//----------------------------------------------------------------------------

#define IDR_SELTOOLBMP                  2500    // Iconbar selection tool.
#define IDB_DITHER                      2502
#define IDR_HATCHBMP                    2503    // Bitmap for border hatching

#define IDS_DRAGMOVEHERE                2508
#define IDS_DRAGCOPYHERE                2509
#define IDR_THKHATCHBMP                 2510    // Bitmap for thick border hatching
#define IDS_UNKNOWNPROTOCOL             2511    
#define IDS_SECURECONNECTIONINFO        2512
#define IDS_SECURE_LOW                  2513
#define IDS_SECURE_MEDIUM               2514
#define IDS_SECURE_HIGH                 2515
#define IDS_SECURESOURCE                2516

//----------------------------------------------------------------------------
//
// Icons
//
//----------------------------------------------------------------------------

#define RES_ICO_HTML                    2661

//----------------------------------------------------------------------------
//
// Misc resources
//
//----------------------------------------------------------------------------

#define IDR_CLICKSOUND              800
#define IDR_SITECONTEXT             24624  //0x6030  // bad id - not in core range

// dependencies - shdocvw\resource.h
#define IDR_FORM_CONTEXT_MENU       24640  //0x6040  // bad id - not in core range
#define IDR_BROWSE_CONTEXT_MENU     24641  //0x6041  // bad id - not in core range

#define IDR_DRAG_CONTEXT_MENU       24645  //0x6045  // bad id - not in core range

#define CX_CONTEXTMENUOFFSET    2
#define CY_CONTEXTMENUOFFSET    2




//
//  Form dialogs
//

//----------------------------------------------------------------------------
//
// Tab order dialog (3250 - 3260)
//
//----------------------------------------------------------------------------
#define IDR_TABORDERLBL             3250
#define IDR_TABORDERLSTBOX          3251
#define IDR_BTNMOVEUP               3252
#define IDR_BTNMOVEDOWN             3253
//#define IDR_BTNAUTOORDER            3254
#define IDR_TABORDERDLG             3255
#define IDR_TABORDERMOVELBL         3256

//+----------------------------------------------------------------------------
//
// HTML Block Format String
//
//-----------------------------------------------------------------------------

#define IDS_BLOCKFMT_NORMAL    1000
#define IDS_BLOCKFMT_PRE       1001
#define IDS_BLOCKFMT_ADDRESS   1002
#define IDS_BLOCKFMT_H1        1003
#define IDS_BLOCKFMT_H2        1004
#define IDS_BLOCKFMT_H3        1005
#define IDS_BLOCKFMT_H4        1006
#define IDS_BLOCKFMT_H5        1007
#define IDS_BLOCKFMT_H6        1008
#define IDS_BLOCKFMT_OL        1009
#define IDS_BLOCKFMT_UL        1010
#define IDS_BLOCKFMT_DIR       1011
#define IDS_BLOCKFMT_MENU      1012
#define IDS_BLOCKFMT_DT        1013
#define IDS_BLOCKFMT_DD        1014
#define IDS_BLOCKFMT_P         1016

#define IDS_HELPABOUT_STRING   1017
#define IDS_URLAUTODETECTOR_QUOTE_MSG 1018

//+----------------------------------------------------------------------------
//
// default title caption for untitled HTML documents
//
//-----------------------------------------------------------------------------

#define IDS_NULL_TITLE         1020

#pragma INCMSG("--- End 'corerc.h'")
#else
#pragma INCMSG("*** Dup 'corerc.h'")
#endif
