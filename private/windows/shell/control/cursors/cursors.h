/** FILE: cursors.h ********** Module Header *******************************
 *
 *  Control panel applet for Cursors configuration.  This file holds
 *  definitions and other common include file items that deal with the
 *  Cursors Dialog of Control Panel.
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-    Steve Cathcart   [stevecat]
 *      Took base code from Win 3.1 source
 * 12-22-91 DarrinM     Created from MOUSE.H
 * 29-Apr-1993 JonPa    added string definitions
 *
 *  Copyright (C) 1990-1991 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                              Include files
//==========================================================================

#include "dialogs.h"

//==========================================================================
//                          Definitions and Typedefs
//==========================================================================

#define IDS_NAME        1
#define IDS_INFO        2
#define IDS_CUR_NOMEM   3
#define IDS_CUR_BADFILE 4
#define IDS_CUR_BROWSE  5
#define IDS_CUR_FILTER  6
#define IDS_ARROW       7
#define IDS_WAIT        8
#define IDS_APPSTARTING 9
#define IDS_NO          10
#define IDS_IBEAM       11
#define IDS_CROSS       12
#define IDS_SIZENS      13
#define IDS_SIZEWE      14
#define IDS_SIZENWSE    15
#define IDS_SIZENESW    16
#define IDS_SIZEALL     17
#define IDS_HELP        18

#define IDS_REMOVESCHEME 20
#define IDS_DEFAULTSCHEME   21


#define IDS_FIRSTSCHEME 1000
#define IDS_LASTSCHEME  1017

#define CURSORSICON     1

/* Help stuff */
#define IDH_DLG_CURSORS (IDH_DLGFIRST + DLG_CURSORS)


//==========================================================================
//                          External Declarations
//==========================================================================

extern TCHAR gszNoMem[256];
extern TCHAR xszControlHlp[];

//==========================================================================
//                              Macros
//==========================================================================
#define IsPathSep(ch)   ((ch) == TEXT('\\') || (ch) == TEXT('/'))

//==========================================================================
//                          Function Prototypes
//==========================================================================

BOOL CALLBACK CursorsDlgProc (HWND hwnd, UINT msg, WPARAM wParam,
                              LPARAM lParam);
LRESULT CALLBACK PreviewWndProc (HWND hwnd, UINT msg, WPARAM wParam,
                                 LPARAM lParam);
