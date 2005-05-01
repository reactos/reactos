/*
 *  PATH.C - path internal command.
 *
 *
 *  History:
 *
 *    17 Jul 1998 (John P Price)
 *        Separated commands into individual files.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    09-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added help text ("/?").
 *
 *    18-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode ready!
 *
 *    18-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Redirection safe!
 *
 *    24-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Fixed Win32 environment handling.
 *
 *    30-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc  
 */
#include "precomp.h"
#include "resource.h"

#ifdef INCLUDE_CMD_PATH

/* size of environment variable buffer */
#define ENV_BUFFER_SIZE 1024


INT cmd_path (LPTSTR cmd, LPTSTR param)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	if (!_tcsncmp (param, _T("/?"), 2))
	{		
		LoadString( GetModuleHandle(NULL), STRING_PATH_HELP1, szMsg,sizeof(szMsg)/sizeof(TCHAR));    
		ConOutPuts (szMsg);	

		return 0;
	}

	/* if param is empty, display the PATH environment variable */
	if (!param || !*param)
	{
		DWORD  dwBuffer;
		LPTSTR pszBuffer;

		pszBuffer = (LPTSTR)malloc (ENV_BUFFER_SIZE * sizeof(TCHAR));
		dwBuffer = GetEnvironmentVariable (_T("PATH"), pszBuffer, ENV_BUFFER_SIZE);
		if (dwBuffer == 0)
		{
			LoadString( GetModuleHandle(NULL), STRING_PATH_ERROR, szMsg,sizeof(szMsg)/sizeof(TCHAR));    
			ConErrPrintf (szMsg);	
			return 0;
		}
		else if (dwBuffer > ENV_BUFFER_SIZE)
		{
			pszBuffer = (LPTSTR)realloc (pszBuffer, dwBuffer * sizeof (TCHAR));
			GetEnvironmentVariable (_T("PATH"), pszBuffer, ENV_BUFFER_SIZE);
		}

		ConOutPrintf (_T("PATH=%s\n"), pszBuffer);
		free (pszBuffer);

		return 0;
	}

	/* skip leading '=' */
	if (*param == _T('='))
		param++;

	/* set PATH environment variable */
	if (!SetEnvironmentVariable (_T("PATH"), param))
		return 1;

	return 0;
}

#endif
