/*
 * PROJECT:     ReactOS VGA Font Editor
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Header containing resource IDs
 * COPYRIGHT:   Copyright 2008 Colin Finck (colin@reactos.org)
 */

#ifndef __RESOURCE_H
#define __RESOURCE_H

#define IDC_STATIC                -1

// Icons
#define IDI_MAIN                  100
#define IDI_DOC                   101

// Accelerator Tables
#define IDA_MAINACCELERATORS      201

// Menus
#define IDM_MAINMENU              301

// Bitmaps
#define IDB_MAIN_TOOLBAR          401
#define IDB_EDIT_GLYPH_TOOLBOX    402

// Dialogs
#define IDD_ABOUT                 501
#define IDD_EDITGLYPH             502

// Dialog Controls
#define IDC_EDIT_GLYPH_TOOLBOX    601
#define IDC_EDIT_GLYPH_EDIT       602
#define IDC_EDIT_GLYPH_PREVIEW    603

// Command IDs
#define ID_FILE_NEW               1001
#define ID_FILE_OPEN              1002
#define ID_FILE_CLOSE             1003
#define ID_FILE_SAVE              1004
#define ID_FILE_SAVE_AS           1005
#define ID_FILE_EXIT              1006

#define ID_EDIT_GLYPH             2001
#define ID_EDIT_COPY              2002
#define ID_EDIT_PASTE             2003

#define ID_WINDOW_TILE_HORZ       3001
#define ID_WINDOW_TILE_VERT       3002
#define ID_WINDOW_CASCADE         3003
#define ID_WINDOW_NEXT            3004
#define ID_WINDOW_ARRANGE         3005

#define ID_HELP_ABOUT             4001

#define ID_TOOLBOX_PEN            5001

// Strings
#define IDS_OPENFILTER            10001
#define IDS_SAVEFILTER            10002
#define IDS_OPENERROR             10003
#define IDS_READERROR             10004
#define IDS_WRITEERROR            10005
#define IDS_UNSUPPORTEDFORMAT     10006
#define IDS_UNSUPPORTEDPSF        10007
#define IDS_DOCNAME               10008
#define IDS_SAVEPROMPT            10009
#define IDS_APPTITLE              10010
#define IDS_CLOSEEDIT             10011

#define IDS_TOOLTIP_NEW           11001
#define IDS_TOOLTIP_OPEN          11002
#define IDS_TOOLTIP_SAVE          11003
#define IDS_TOOLTIP_EDIT_GLYPH    11004
#define IDS_TOOLTIP_COPY          11005
#define IDS_TOOLTIP_PASTE         11006

#endif
