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
 */

#include "config.h"

#ifdef INCLUDE_CMD_PATH

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>

#include "cmd.h"


/* size of environment variable buffer */
#define ENV_BUFFER_SIZE 1024


INT cmd_path (LPTSTR cmd, LPTSTR param)
{
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Displays or sets a search path for executable files.\n\n"
				   "PATH [[drive:]path[;...]]\nPATH ;\n\n"
				   "Type PATH ; to clear all search-path settings and direct the command shell\n"
				   "to search only in the current directory.\n"
				   "Type PATH without parameters to display the current path.\n"));
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
			ConErrPrintf ("CMD: Not in environment \"PATH\"\n");
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
