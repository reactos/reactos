/*
 *  SET.C - set internal command.
 *
 *
 *  History:
 *
 *    06/14/97 (Tim Norman)
 *        changed static var in set() to a malloc'd space to pass to putenv.
 *        need to find a better way to do this, since it seems it is wasting
 *        memory when variables are redefined.
 *
 *    07/08/1998 (John P. Price)
 *        removed call to show_environment in set command.
 *        moved test for syntax before allocating memory in set command.
 *        misc clean up and optimization.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    28-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added set_env function to set env. variable without needing set command
 *
 *    09-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added help text ("/?").
 *
 *    24-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Fixed Win32 environment handling.
 *        Unicode and redirection safe!
 *
 *    25-Feb-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Fixed little bug.
 */

#include "config.h"

#ifdef INCLUDE_CMD_SET

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>

#include "cmd.h"


/* initial size of environment variable buffer */
#define ENV_BUFFER_SIZE  1024


INT cmd_set (LPTSTR cmd, LPTSTR param)
{
	LPTSTR p;

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Displays, sets, or removes environment variables.\n\n"
					   "SET [variable[=][string]]\n\n"
					   "  variable  Specifies the environment-variable name.\n"
					   "  string    Specifies a series of characters to assign to the variable.\n\n"
					   "Type SET without parameters to display the current environment variables.\n"));
		return 0;
	}

	/* if no parameters, show the environment */
	if (param[0] == _T('\0'))
	{
		LPTSTR lpEnv;
		LPTSTR lpOutput;
		INT len;

		lpEnv = (LPTSTR)GetEnvironmentStrings ();
		if (lpEnv)
		{
			lpOutput = lpEnv;
			while (*lpOutput)
			{
				len = _tcslen(lpOutput);
				if (len)
				{
					if (*lpOutput != _T('='))
						ConOutPuts (lpOutput);
					lpOutput += (len + 1);
				}
			}
			FreeEnvironmentStrings (lpEnv);
		}

		return 0;
	}

	p = _tcschr (param, _T('='));
	if (p)
	{
		/* set or remove environment variable */
		*p = _T('\0');
		p++;

		SetEnvironmentVariable (param, p);
	}
	else
	{
		/* display environment variable */
		LPTSTR pszBuffer;
		DWORD dwBuffer;

		pszBuffer = (LPTSTR)malloc (ENV_BUFFER_SIZE * sizeof(TCHAR));
		dwBuffer = GetEnvironmentVariable (param, pszBuffer, ENV_BUFFER_SIZE);
		if (dwBuffer == 0)
		{
			ConErrPrintf ("CMD: Not in environment \"%s\"\n", param);
			return 0;
		}
		else if (dwBuffer > ENV_BUFFER_SIZE)
		{
			pszBuffer = (LPTSTR)realloc (pszBuffer, dwBuffer * sizeof (TCHAR));
			GetEnvironmentVariable (param, pszBuffer, dwBuffer * sizeof (TCHAR));
		}

		ConOutPrintf ("%s\n", pszBuffer);
		free (pszBuffer);

		return 0;
	}

	return 0;
}

#endif
