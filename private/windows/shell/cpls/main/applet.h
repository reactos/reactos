/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    applet.h

Abstract:

    This module contains the main header information for this project.

Revision History:

--*/



#ifndef _APPLETS_H
#define _APPLETS_H



//
//  The prototype for an applet function is:
//    int Applet(HINSTANCE instance, HWND parent, LPCTSTR cmdline);
//
//  instance - The instance handle of the control panel containing the applet.
//
//  parent   - Contains the handle of a parent window for the applet (if any).
//
//  cmdline  - Points to the command line for the applet (if available).
//             If the applet was launched without a command line,
//             'cmdline' contains NULL.
//

typedef int (*PFNAPPLET)(HINSTANCE, HWND, LPCTSTR);


//
//  The return value specifies any further action that must be taken:
//      APPLET_RESTART -- Windows must be restarted
//      APPLET_REBOOT  -- the machine must be rebooted
//      all other values are ignored
//

#define APPLET_RESTART            0x8
#define APPLET_REBOOT             (APPLET_RESTART | 0x4)


//
//  The prototype for an applet query functions is:
//      LRESULT AppletQuery(UINT Message);
//

typedef LRESULT (*PFNAPPLETQUERY)(HWND, UINT);

#define APPLET_QUERY_EXISTS       0   //  BOOL result
#define APPLET_QUERY_GETICON      1   //  HICON result



#endif
