/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    main.h

Abstract:

    This module contains the header information for the main routines of the
    Control Panel interface of the 32bit MAIN.CPL.

Revision History:

--*/



#ifndef _MAIN_H
#define _MAIN_H

#define USECOMM
#define OEMRESOURCE
#define STRICT

#ifdef WIN32
#define INC_OLE2
#define CONST_VTABLE
#endif



//
//  Include Files.
//

#include <windows.h>
#include <windowsx.h>
#include <dlgs.h>
#include <cpl.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlobjp.h>
#include <commctrl.h>

#ifndef RC_INVOKED
#include <prsht.h>
#endif




//
//  Constant Declarations.
//

#define PATHMAX MAX_PATH
#define HELP_FILE TEXT("mouse.hlp")  // Help file for the mouse control panel




//
//  Typedef Declarations.
//

#ifndef NOARROWS
typedef struct
{
    short lineup;             // lineup/down, pageup/down are relative
    short linedown;           // changes.  top/bottom and the thumb
    short pageup;             // elements are absolute locations, with
    short pagedown;           // top & bottom used as limits.
    short top;
    short bottom;
    short thumbpos;
    short thumbtrack;
    BYTE  flags;              // flags set on return
} ARROWVSCROLL, NEAR *PARROWVSCROLL, FAR *LPARROWVSCROLL;

#define UNKNOWNCOMMAND 1
#define OVERFLOW       2
#define UNDERFLOW      4

#endif


#endif
