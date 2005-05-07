/*
 *  START.C - start internal command.
 *
 *
 *  History:
 *
 *    24-Jul-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Started.
 *
 *    30-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc  
 */

#include "precomp.h"
#include "resource.h"

#ifdef INCLUDE_CMD_START


INT cmd_start (LPTSTR first, LPTSTR rest)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	TCHAR szFullName[MAX_PATH];
	BOOL bWait = FALSE;
	TCHAR *param;

	if (_tcsncmp (rest, _T("/?"), 2) == 0)
	{
		LoadString(GetModuleHandle(NULL), STRING_START_HELP1, szMsg, RC_STRING_MAX_SIZE);
		ConOutPuts(szMsg);
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
	    rest = _T("cmd");
	  }

	/* get the PATH environment variable and parse it */
	/* search the PATH environment variable for the binary */
	param = _tcschr( rest, _T(' ') );  // skip program name to reach parameters
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
		DebugPrintf (_T("[BATCH: %s %s]\n"), szFullName, rest);
#endif

		LoadString(GetModuleHandle(NULL), STRING_START_ERROR1, szMsg, RC_STRING_MAX_SIZE);
		ConErrPuts(szMsg);
	}
	else
	{
		/* exec the program */
		TCHAR szFullCmdLine [CMDLINE_LENGTH];
		PROCESS_INFORMATION prci;
		STARTUPINFO stui;

#ifdef _DEBUG
		DebugPrintf (_T("[EXEC: %s %s]\n"), szFullName, rest);
#endif
		/* build command line for CreateProcess() */
		_tcscpy (szFullCmdLine, first);
		if( param )
		  {
		    _tcscat(szFullCmdLine, _T(" ") );
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
			}
			CloseHandle (prci.hThread);
			CloseHandle (prci.hProcess);
		/* Get New code page if it has change */
		InputCodePage= GetConsoleCP();
        OutputCodePage = GetConsoleOutputCP();
		}
		else
		{
			ErrorMessage(GetLastError (),
			              _T("Error executing CreateProcess()!!\n"));
		}
	}

	return 0;
}

#endif

/* EOF */
