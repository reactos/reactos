/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    drvaplet.h

Abstract:

    This module contains the header information for the driver routines
    for the project.

Revision History:

--*/



#ifndef _DRVAPLET_H
#define _DRVAPLET_H


//
//  HDAP: handle to a driver applet.
//

DECLARE_HANDLE(HDAP);


//
//  GetDriverModule: gets the module.
//
HMODULE GetDriverModule(LPCTSTR name);

//
// ReleaseDriverModule:  releases the module.
//
void ReleaseDriverModule(HMODULE module);

//
//  OpenDriverApplet: opens a handle to the named driver applet.
//
HDAP OpenDriverApplet(LPCTSTR);


//
//  CloseDriverApplet: closes a handle to a driver applet.
//
void CloseDriverApplet(HDAP);


//
//  GetDriverAppletIcon: get's a driver applet's icon (if any).
//
HICON GetDriverAppletIcon(HDAP);


//
//  CallDriverApplet: sends a message to the driver applet (CplApplet syntax).
//
LRESULT CallDriverApplet(HDAP, HWND, UINT, LPARAM, LPARAM);


//
//  RunDriverApplet: runs the driver applet.
//
#define RunDriverApplet(h, w)  CallDriverApplet(h, w, CPL_DBLCLK, 0L, 0L)


//
//  "CplApplet"
//
extern const TCHAR *c_szCplApplet;



#endif
