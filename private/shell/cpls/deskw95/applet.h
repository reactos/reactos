#ifndef _APPLETS_H
#define _APPLETS_H

///////////////////////////////////////////////////////////////////////////////
// APPLETS.H
///////////////////////////////////////////////////////////////////////////////

// The prototype for an applet functions is:
//  int Applet( HINSTANCE instance, HWND parent, LPCSTR cmdline );
//
// 'instance' the instance handle of the control panel containing the applet
//
// 'parent' contains the handle of a parent window for the applet (if any)
//
// 'cmdline' points to the command line for the applet (if available)
// if the applet was launched without a command line, 'cmdline' contains NULL

typedef int (*PFNAPPLET)( HINSTANCE, HWND, LPCSTR );

// the return value specifies any further action that must be taken
//  APPLET_RESTART -- Windows must be restarted
//  APPLET_REBOOT  -- the machine must be rebooted
//  all other values are ignored

#define APPLET_RESTART  0x8
#define APPLET_REBOOT   ( APPLET_RESTART | 0x4 )

extern BOOL CheckRestriction(HKEY hKey,LPCSTR lpszValueName);
extern const char szRestrictionKey[];
extern const char szNoDispCPL[];

///////////////////////////////////////////////////////////////////////////////

#endif
