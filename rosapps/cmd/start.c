/*
 *  START.C - start internal command.
 *
 *
 *  History:
 *
 *    24-Jul-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Started.
 */

#include "config.h"

#ifdef INCLUDE_CMD_START
#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "cmd.h"


INT cmd_start (LPTSTR first, LPTSTR rest)
{
	TCHAR szFullName[MAX_PATH];
	BOOL bWait = FALSE;
	TCHAR *param;

	if (_tcsncmp (rest, _T("/?"), 2) == 0)
	{
		ConOutPuts (_T("Starts a command.\n\n"
				   "START command \n\n"
				   "  command     Specifies the command to run.\n\n"
				   "At the moment all commands are started asynchronously.\n"));

		return 0;
	}

	/* check for a drive change */
	if (!_tcscmp (first + 1, _T(":")) && _istalpha (*first))
	{
		TCHAR szPath[MAX_PATH];

		_tcscpy (szPath, _T("A:"));
		szPath[0] = _totupper (*first);
		SetCurrentDirectory (szPath);
		GetCurrentDirectory (MAX_PATH, szPath);
		if (szPath[0] != (TCHAR)_totupper (*first))
			ConErrPuts (INVALIDDRIVE);

		return 0;
	}
	if( !*rest )
	  {
	    // FIXME: use comspec instead
	    rest = "cmd";
	  }
	/* get the PATH environment variable and parse it */
	/* search the PATH environment variable for the binary */
	param = strchr( rest, ' ' );  // skip program name to reach parameters
	if( param )
	  {
	    *param = 0;
	    param++;
	  }
	if (!SearchForExecutable (rest, szFullName))
	{
		error_bad_command ();
		return 1;
	}
	/* check if this is a .BAT or .CMD file */
	if (!_tcsicmp (_tcsrchr (szFullName, _T('.')), _T(".bat")) ||
		!_tcsicmp (_tcsrchr (szFullName, _T('.')), _T(".cmd")))
	{
#ifdef _DEBUG
		DebugPrintf ("[BATCH: %s %s]\n", szFullName, rest);
#endif
		ConErrPuts (_T("No batch support at the moment!"));
	}
	else
	{
		/* exec the program */
		TCHAR szFullCmdLine [CMDLINE_LENGTH];
		PROCESS_INFORMATION prci;
		STARTUPINFO stui;

#ifdef _DEBUG
		DebugPrintf ("[EXEC: %s %s]\n", szFullName, rest);
#endif
		/* build command line for CreateProcess() */
		_tcscpy (szFullCmdLine, first);
		if( param )
		  {
		    _tcscat(szFullCmdLine, " " );
		    _tcscat (szFullCmdLine, param);
		  }

		/* fill startup info */
		memset (&stui, 0, sizeof (STARTUPINFO));
		stui.cb = sizeof (STARTUPINFO);
		stui.dwFlags = STARTF_USESHOWWINDOW;
		stui.wShowWindow = SW_SHOWDEFAULT;
			
		if (CreateProcess (szFullName, szFullCmdLine, NULL, NULL, FALSE,
						   CREATE_NEW_CONSOLE, NULL, NULL, &stui, &prci))
		{
			if (bWait)
			{
				DWORD dwExitCode;
				WaitForSingleObject (prci.hProcess, INFINITE);
				GetExitCodeProcess (prci.hProcess, &dwExitCode);
				nErrorLevel = (INT)dwExitCode;
				CloseHandle (prci.hThread);
				CloseHandle (prci.hProcess);
			}
		}
		else
		{
			ErrorMessage (GetLastError (),
						  "Error executing CreateProcess()!!\n");
		}
	}

	return 0;
}

#endif

/* EOF */
