/*
 *	First blast at Desktop App help.
 */

#include "windows.h"
#include <port1632.h>
#include "help.h"

/*
 *	EnumHelpWindowFind
 *
 *	Enumeration callback function to locate the Help window.
 *	This function MUST be exported in the application's .DEF
 *	file.  Assumes that lphwndHelp points the window handle
 *	of the main window of the calling application, and changes
 *	that window handle to the help application if found.
 */
BOOL FAR APIENTRY EnumHelpWindowFind(HWND hwnd, HWND FAR * lphwndHelp)
{
    if (SendMessage(hwnd,WM_HELP,HELP_FIND,MAKELONG(0,0)))
	{
	*lphwndHelp=hwnd;
	return FALSE;
	}
    return TRUE;
}

/*
 *	FRequestTopic
 *
 *	Requests help for a given topic.  Passed the application's main
 *	window and the topic ordinal of interest.  All desktop application
 *	help is assumed to be in the same file.
 *		a) attempt to locate the Help application by enumerating
 *		all main windows and sending WM_HELP/idFind messages to them.
 *		b) if not found, boot the help app and repeat
 *		c) send the appropriate topic message
 */
BOOL FAR APIENTRY FRequestHelp(HANDLE hInstance, HWND hwnd, WORD iTopic)
    {
    FARPROC lpfnEnum;
    HWND hwndHelp=hwnd;
    BOOL fWorked;

	CHAR *szMsg[80];

    lpfnEnum=MakeProcInstance(EnumHelpWindowFind,hInstance);
    if (!lpfnEnum)
	return FALSE;

    EnumWindows(lpfnEnum,(LONG)(LPSTR)&hwndHelp);

    FreeProcInstance(lpfnEnum);

    if (!hwndHelp || hwndHelp==hwnd)
	return FALSE;

    SendMessage(hwndHelp,WM_HELP,HELP_FOCUS,MAKELONG(GetFocus(),0));
    SendMessage(hwndHelp,WM_HELP,iTopic,MAKELONG(hwnd,0));

    return TRUE;	/* ignore errors _for_the_moment_ */

    }
