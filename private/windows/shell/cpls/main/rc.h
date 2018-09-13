/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    rc.h

Abstract:

    This module contains the header information for the resources of
    this project.

Revision History:

--*/



#ifndef _RC_H
#define _RC_H



//
//  Icons.
//

#define IDI_MOUSE                      100
#define IDI_KEYBD                      200
#define IDI_PRINT                      300
#define IDI_FONTS                      400
#define IDI_ADM                        500

#define IDI_PTTRAILS                   103
#define IDI_PTSPEED                    104

#define IDI_DELAY                      105
#define IDI_REPEAT                     106

#define IDI_SNAPDEF                    108

#define IDI_SGLCLICK                   109
#define IDI_DBLCLICK                   110

//
//  Bitmaps.
//

#define IDB_MOUSE                      100




//
//  Animations.
//

#define IDA_JACKNBOX                   100




//
//  Strings.
//

#define IDS_MOUSE_TITLE                100
#define IDS_MOUSE_EXPLAIN              101
#define IDS_KEYBD_TITLE                102
#define IDS_KEYBD_EXPLAIN              103
#define IDS_PRINT_TITLE                104
#define IDS_PRINT_EXPLAIN              105
#define IDS_FONTS_TITLE                106
#define IDS_FONTS_EXPLAIN              107
#define IDS_ADM_TITLE                  108
#define IDS_ADM_EXPLAIN                109

#define IDS_MOUSE_TSHOOT               110
#define IDS_KEYBD_TSHOOT               111

#define IDS_UNKNOWN                    198
#define IDS_KEYBD_NOSETSPEED           199

#define IDS_ANICUR_FILTER              200
#define IDS_NAME                       201
#define IDS_INFO                       202
#define IDS_CUR_NOMEM                  203
#define IDS_CUR_BADFILE                204
#define IDS_CUR_BROWSE                 205
#define IDS_CUR_FILTER                 206
#define IDS_ARROW                      207
#define IDS_WAIT                       208
#define IDS_APPSTARTING                209
#define IDS_NO                         210
#define IDS_IBEAM                      211
#define IDS_CROSS                      212
#define IDS_SIZENS                     213
#define IDS_SIZEWE                     214
#define IDS_SIZENWSE                   215
#define IDS_SIZENESW                   216
#define IDS_SIZEALL                    217
#define IDS_HELPCUR                    218
#define IDS_NWPEN                      219
#define IDS_UPARROW                    220
#define IDS_NONE                       221
#define IDS_SUFFIX                     222
#define IDS_OVERWRITE_TITLE            223
#define IDS_OVERWRITE_MSG              224
#define IDS_HANDCUR                    225

#define IDS_REMOVESCHEME               230
#define IDS_DEFAULTSCHEME              231

#define IDS_FIRSTSCHEME                1000
#define IDS_LASTSCHEME                 1017




//
//  Dialog Boxes.
//

#define DLG_MOUSE_POINTER_SCHEMESAVE   99
#define DLG_MOUSE_BUTTONS              100
#define DLG_MOUSE_POINTER              101
#define DLG_MOUSE_POINTER_BROWSE       102
#define DLG_MOUSE_MOTION               103
#define DLG_KEYBD_SPEED                104
#define DLG_KEYBD_POINTER              105
#define DLG_HARDWARE                   106



//
//  Dialog Controls.
//

#define IDC_GROUPBOX_1                 94   // Use in place of IDC_STATIC for
#define IDC_GROUPBOX_2                 95   // controls with no context Help
#define IDC_GROUPBOX_3                 96
#define IDC_GROUPBOX_4                 97
#define IDC_GROUPBOX_5                 98
#define IDC_GROUPBOX_6                 99




//
//  Mouse Button Page.
//

#define MOUSE_LEFTHAND                 100
#define MOUSE_RIGHTHAND                101
#define MOUSE_SELECTBMP                102
#define MOUSE_MOUSEBMP                 103
#define MOUSE_MENUBMP                  104
#define MOUSE_CLICKSCROLL              105
#define MOUSE_DBLCLKBMP                106
#define MOUSE_PTRCOLOR                 107
#define MOUSE_SIZESCROLL               108
#define IDC_SELECTDRAG                 109
#define IDC_OBJECTMENU                 110
#define MOUSE_CLICKICON                111
#define MOUSE_DBLCLICK                 112
#define MOUSE_SGLCLICK                 113




//
//  Mouse Pointer Page.
//

#define DLG_CURSORS                    100
#define ID_CURSORLIST                  101
#define ID_BROWSE                      102
#define ID_DEFAULT                     103
#define ID_TITLEH                      104
#define ID_CREATORH                    105
#define ID_FILEH                       106
#define ID_TITLE                       107
#define ID_CREATOR                     108
#define ID_FILE                        109
#define ID_PREVIEW                     110
#define ID_SAVESCHEME                  111
#define ID_REMOVESCHEME                112
#define ID_SCHEMECOMBO                 113
#define ID_CURSORSHADOW                114

#define ID_SCHEMEFILENAME              300

#define ID_CURSORPREVIEW               400




//
//  Mouse Motion Page.
//

#define MOUSE_SPEEDSCROLL              101
#define MOUSE_TRAILBMP                 102
#define MOUSE_TRAILS                   103
#define MOUSE_TRAILSCROLL              104
#define MOUSE_PTRTRAIL                 105
#define MOUSE_SPEEDBMP                 106
#define MOUSE_TRAILSCROLLTXT1          107
#define MOUSE_TRAILSCROLLTXT2          108
#define MOUSE_SNAPDEF                  109
#define MOUSE_PTRSNAPDEF               110
#define MOUSE_ACCELBMP                 111
#define MOUSE_ACCELNONE                112
#define MOUSE_ACCELLOW                 113
#define MOUSE_ACCELMEDIUM              114
#define MOUSE_ACCELHIGH                115



//
//  Keyboard Speed Page.
//

#define KDELAY_SCROLL                  100
#define KSPEED_SCROLL                  101
#define KREPEAT_EDIT                   102
#define KBLINK_EDIT                    103
#define KCURSOR_BLINK                  104
#define KCURSOR_SCROLL                 105
#define KDELAY_GROUP                   106
#define KBLINK_GROUP                   107




//
//  Keyboard Pointer Page.
//

#define KCHK_ON                        100
#define KNUM_BMP                       101
#define KBTN_NUMBER                    102
#define KBTN_ARROW                     103
#define KARROW_BMP                     104
#define KPSPEED_SCROLL                 105
#define KPACC_SCROLL                   106



#endif
