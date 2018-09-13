/*****************************************************************************
 *
 *	ftppf.cpp - Progress Feedback
 *
 *****************************************************************************/

#include "priv.h"

/*****************************************************************************
 *
 *	HPF - Handle to progress feedback
 *
 *	Shhh...  Don't tell anyone, but it's just a window handle.
 *
 *	It's the handle of the status bar window to use.  We use the
 *	second part (part number one, since they start at zero) to display
 *	connection feedback.
 *
 *	We don't use SIMPLE mode, because DefView uses SIMPLE mode to display
 *	menu feedback.
 *
 *****************************************************************************/

#define hwndNil	

/*****************************************************************************
 *
 *	FtpPf_Begin
 *
 *****************************************************************************/

HPF FtpPf_Begin(HWND hwndOwner)
{
    HWND hwnd;
    ASSERTNONCRITICAL;
    hwnd = Misc_FindStatusBar(hwndOwner);
    if (hwnd)
    {
	    SendMessage(hwnd, SB_SETTEXT, 1 | SBT_NOBORDERS, 0);
    }
    return (HPF)hwnd;
}

/*****************************************************************************
 *
 *	FtpPf_Status
 *
 *	ids = string to display in status bar
 *	ptsz = optional insert
 *
 *****************************************************************************/

void FtpPf_Status(HPF hpf, UINT ids, LPCTSTR pszParameters)
{
    HWND hwnd = (HWND)hpf;

    ASSERTNONCRITICAL;
    if (EVAL(hwnd))
    {
	    TCHAR szMsgTemplate[256];
	    TCHAR szMessage[1024];

	    LoadString(g_hinst, ids, szMsgTemplate, ARRAYSIZE(szMsgTemplate));
	    wnsprintf(szMessage, ARRAYSIZE(szMessage), szMsgTemplate, pszParameters);
	    SendMessage(hwnd, SB_SETTEXT, 1 | SBT_NOBORDERS, (LPARAM)szMessage);
	    UpdateWindow(hwnd);
    }
}

/*****************************************************************************
 *
 *	FtpPf_End
 *
 *****************************************************************************/

void FtpPf_End(HPF hpf)
{
    HWND hwnd;
    ASSERTNONCRITICAL;
    hwnd = (HWND)hpf;
    if (hwnd)
    {
	    SendMessage(hwnd, SB_SETTEXT, 1 | SBT_NOBORDERS, 0);
    }
}

