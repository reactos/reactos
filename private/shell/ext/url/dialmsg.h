//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************

//
//	DIALMSG.H - window messages for dial monitor app
//			
//

//	HISTORY:
//	
//	4/19/95		jeremys		Created.
//

#ifndef _DIALMSG_H_
#define _DIALMSG_H_

#define WM_DIALMON_FIRST	WM_USER+100

// message sent to dial monitor app window indicating that there has been
// winsock activity and dial monitor should reset its idle timer
#define WM_WINSOCK_ACTIVITY		WM_DIALMON_FIRST + 0

// message sent to dial monitor app window when user changes timeout through
// UI, indicating that timeout value or status has changed
#define WM_REFRESH_SETTINGS		WM_DIALMON_FIRST + 1

// message sent to dial monitor app window to set the name of the connectoid
// to monitor and eventually disconnect.  lParam should be an LPSTR that
// points to the name of the connectoid.
#define WM_SET_CONNECTOID_NAME	WM_DIALMON_FIRST + 2

// message sent to dial monitor app window when app exits
#define WM_IEXPLORER_EXITING		WM_DIALMON_FIRST + 3

#endif // _DIALMSG_H_
