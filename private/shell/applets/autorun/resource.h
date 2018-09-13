//////////////////////////////////////////////////////////////////////////
//
//  resource.h
//
//      This file contains all of the resource ids used by the application.
//
//  (C) Copyright 1997 by Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

#ifndef _RESOURCE_H_
#define _RESOURCE_H_

#include <winuser.h>

//
//  icons.
//
#define IDI_WEBAPP          1

//
//  cursors.
//
#define IDC_BRHAND          1

//
//  bitmaps.
//
// The server versions always use 16 color images
#define IDB_BANNER          1
#if BUILD_SERVER_VERSION | BUILD_ADVANCED_SERVER_VERSION | BUILD_DATACENTER_VERSION
#define IDB_BANNER16        IDB_BANNER
#else
#define IDB_BANNER16        2
#endif
#define IDB_BKGND0          3
#define IDB_BKGND1          4
#define IDB_BKGND2          5
#define IDB_BKGND3          6
#define IDB_BKGND4          7
#define IDB_256MENU         8
#define IDB_256BORDER       9
#define IDB_16MENU          10
#define IDB_16BORDER        11

//
//  string table entries.
//
#define IDS_TITLE           1000
#define IDS_CHECKTEXT       1001
#define IDS_DEFTITLE        1002
#define IDS_DEFBODY         1003
#define IDS_FONTFACE        1004

#define IDS_PSCTITLE        1005
#define IDS_PSCDESC         1006
#define IDS_PSCCONFIG       1007
#define IDS_PSCARGS         1008
#define IDS_PSCMENU         1009

#define IDS_TITLE0          1010
#define IDS_TITLE1          1011
#define IDS_TITLE2          1012
#define IDS_TITLE3          1013

#define IDS_MENU0           1020
#define IDS_MENU1           1021
#define IDS_MENU2           1022
#define IDS_MENU3           1023

#define IDS_DESC0           1030
#define IDS_DESC1           1031
#define IDS_DESC2           1032
#define IDS_DESC3           1033

#define IDS_CONFIG0         1040
#define IDS_CONFIG1         1041
#define IDS_CONFIG2         1042
#define IDS_CONFIG3         1043

#define IDS_ARGS0           1050
#define IDS_ARGS1           1051
#define IDS_ARGS2           1052
#define IDS_ARGS3           1053

#define IDS_OLDCDROM        1060
#define IDS_NEWCDROM        1061

#define IDS_CYMENUITEMFONT  1200
#define IDS_CYTITLEFONT     1201
#define IDS_CYBODYFONT      1202
#define IDS_CYCHECKTEST     1203

//
//  commands.
//
#define IDM_SHOWCHECK       100
#define IDM_MENUITEM1       101
#define IDM_MENUITEM2       102
#define IDM_MENUITEM3       103
#define IDM_MENUITEM4       104
#define IDM_MENUITEM5       105
#define IDM_MENUITEM6       106
#define IDM_MENUITEM7       107

#endif
