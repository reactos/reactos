/*
 *  VER.C - ver internal command.
 *
 *
 *  History:
 *
 *    06/30/98 (Rob Lake)
 *        rewrote ver command to accept switches, now ver alone prints
 *        copyright notice only.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    30-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added text about where to send bug reports and get updates.
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection safe!
 *
 *    26-Feb-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        New version info and some output changes.
 */

#include "precomp.h"
#include "resource.h"


VOID ShortVersion (VOID)
{
	OSVERSIONINFO VersionInfo;
	unsigned RosVersionLen;
	LPTSTR RosVersion;

	ConOutPuts (_T("\n"
	               SHELLINFO));
	VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
#ifdef _UNICODE
		ConOutPrintf(_T("%S"), SHELLVER);
#else
		ConOutPrintf(_T("%s"), SHELLVER);
#endif /* _UNICODE */
	memset(VersionInfo.szCSDVersion, 0, sizeof(VersionInfo.szCSDVersion));
	if (GetVersionEx(&VersionInfo))
	{
		RosVersion = VersionInfo.szCSDVersion + _tcslen(VersionInfo.szCSDVersion) + 1;
		RosVersionLen = sizeof(VersionInfo.szCSDVersion) / sizeof(VersionInfo.szCSDVersion[0]) -
	                        (RosVersion - VersionInfo.szCSDVersion);
		if (7 <= RosVersionLen && 0 == _tcsnicmp(RosVersion, _T("ReactOS"), 7))
		{
			ConOutPrintf(_T(" running on %s"), RosVersion);
		}
	}
	ConOutPuts (_T("\n"));
}


#ifdef INCLUDE_CMD_VER

/*
 *  display shell version info internal command.
 *
 *
 */
INT cmd_ver (LPTSTR cmd, LPTSTR param)
{
	INT i;
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	TCHAR rosdev[RC_STRING_MAX_SIZE];
	TCHAR fredev[RC_STRING_MAX_SIZE];

	if (_tcsstr (param, _T("/?")) != NULL)
	{
		LoadString( GetModuleHandle(NULL), STRING_VERSION_HELP1, (LPTSTR) szMsg,sizeof(szMsg));
		ConOutPuts (szMsg);	

		return 0;
	}

	ShortVersion();
	ConOutPuts (_T("Copyright (C) 1994-1998 Tim Norman and others."));
	ConOutPuts (_T("Copyright (C) 1998-2001 Eric Kohl and others."));

	/* Basic copyright notice */
	if (param[0] == _T('\0'))
	{
		ConOutPuts (_T("\n"SHELLINFO));
		LoadString( GetModuleHandle(NULL), STRING_VERSION_HELP2, (LPTSTR) szMsg,sizeof(szMsg));
		ConOutPuts (szMsg);	
	}
	else
	{
		for (i = 0; param[i]; i++)
		{
			/* skip spaces */
			if (param[i] == _T(' '))
				continue;

			if (param[i] == _T('/'))
			{
				/* is this a lone '/' ? */
				if (param[i + 1] == 0)
				{
					error_invalid_switch (_T(' '));
					return 1;
				}
				continue;
			}

			if (_totupper (param[i]) == _T('W'))
			{
				/* Warranty notice */
				LoadString( GetModuleHandle(NULL), STRING_VERSION_HELP3, (LPTSTR) szMsg,sizeof(szMsg));
				ConOutPuts (szMsg);	
			}
			else if (_totupper (param[i]) == _T('R'))
			{
				/* Redistribution notice */
				LoadString( GetModuleHandle(NULL), STRING_VERSION_HELP4, (LPTSTR) szMsg,sizeof(szMsg));
				ConOutPuts (szMsg);					
			}
			else if (_totupper (param[i]) == _T('C'))
			{
				/* Developer listing */
				
				LoadString( GetModuleHandle(NULL), STRING_REACTOS_DEV, rosdev,sizeof(rosdev)/sizeof(TCHAR));
				LoadString( GetModuleHandle(NULL), STRING_FreeDOS_DEV, fredev,sizeof(fredev)/sizeof(TCHAR));
				LoadString( GetModuleHandle(NULL), STRING_VERSION_HELP6, (LPTSTR) szMsg,sizeof(szMsg));	                
				ConOutPrintf ((LPTSTR)szMsg,fredev,rosdev);
				
				/*
				ConOutPuts (_T("\n"
				               "FreeDOS version written by:\n"
				               "    Tim Norman      Matt Rains\n"
				               "    Evan Jeffrey    Steffen Kaiser\n"
				               "    Svante Frey     Oliver Mueller\n"
				               "    Aaron Kaufman   Marc Desrochers\n"
				               "    Rob Lake        John P Price\n"
				               "    Hans B Pufal\n"
				               "\n"
				               "ReactOS version written by:\n"
				               "    Eric Kohl       Emanuele Aliberti\n"
				               "    Paolo Pantaleo  Phillip Susi\n"));
			   */
			}
			else
			{
				error_invalid_switch ((TCHAR)_totupper (param[i]));
				return 1;
			}
		}
	}

	LoadString( GetModuleHandle(NULL), STRING_VERSION_HELP5, (LPTSTR) szMsg,sizeof(szMsg));
	ConOutPuts (szMsg);	
	return 0;
}

#endif
