/*
 *  START.C - start internal command.
 *
 *
 *  History:
 *
 *    24-Jul-1999 (Eric Kohl)
 *        Started.
 *
 *    30-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>

#ifdef INCLUDE_CMD_START

/* Find the end of an option, and turn it into a nul-terminated string
 * in place. (It's moved back one character, to make room for the nul) */
static TCHAR *GetParameter(TCHAR **pPointer)
{
	BOOL bInQuote = FALSE;
	TCHAR *start = *pPointer;
	TCHAR *p;
	for (p = start; *p; p++)
	{
		if (!bInQuote && (*p == _T('/') || _istspace(*p)))
			break;
		bInQuote ^= (*p == _T('"'));
		p[-1] = *p;
	}
	p[-1] = _T('\0');
	*pPointer = p;
	return start - 1;
}

INT cmd_start (LPTSTR Rest)
{
	TCHAR szFullName[CMDLINE_LENGTH];
	TCHAR szUnquotedName[CMDLINE_LENGTH];
	TCHAR *param = NULL;
	TCHAR *dot;
	INT size;
	LPTSTR comspec;
	BOOL bWait = FALSE;
	BOOL bBat  = FALSE;
	BOOL bCreate = FALSE;
	TCHAR szFullCmdLine [CMDLINE_LENGTH];
	PROCESS_INFORMATION prci;
	STARTUPINFO stui;
	INT i = 0;
	DWORD Priority = 0;
	LPTSTR lpTitle = NULL;
	LPTSTR lpDirectory = NULL;
	WORD wShowWindow = SW_SHOWNORMAL;

	while (1)
	{
		if (_istspace(*Rest))
		{
			Rest++;
		}
		else if (*Rest == _T('"') && !lpTitle)
		{
			lpTitle = GetParameter(&Rest);
			StripQuotes(lpTitle);
		}
		else if (*Rest == L'/')
		{
			LPTSTR option;
			Rest++;
			option = GetParameter(&Rest);
			if (*option == _T('?'))
			{
				ConOutResPaging(TRUE,STRING_START_HELP1);
				return 0;
			}
			else if (_totupper(*option) == _T('D'))
			{
				lpDirectory = option + 1;
				if (!*lpDirectory)
				{
					while (_istspace(*Rest))
						Rest++;
					lpDirectory = GetParameter(&Rest);
				}
				StripQuotes(lpDirectory);
			}
			else if (!_tcsicmp(option, _T("MIN")))
			{
				wShowWindow = SW_MINIMIZE;
			}
			else if (!_tcsicmp(option, _T("MAX")))
			{
				wShowWindow = SW_MAXIMIZE;
			}
			else if (!_tcsicmp(option, _T("LOW")))
			{
				Priority = IDLE_PRIORITY_CLASS;
			}
			else if (!_tcsicmp(option, _T("NORMAL")))
			{
				Priority = NORMAL_PRIORITY_CLASS;
			}
			else if (!_tcsicmp(option, _T("HIGH")))
			{
				Priority = HIGH_PRIORITY_CLASS;
			}
			else if (!_tcsicmp(option, _T("REALTIME")))
			{
				Priority = REALTIME_PRIORITY_CLASS;
			}
			else if (!_tcsicmp(option, _T("ABOVENORMAL")))
			{
				Priority = ABOVE_NORMAL_PRIORITY_CLASS;
			}
			else if (!_tcsicmp(option, _T("BELOWNORMAL")))
			{
				Priority = BELOW_NORMAL_PRIORITY_CLASS;
			}
			else if (!_tcsicmp(option, _T("W")) ||
			         !_tcsicmp(option, _T("WAIT")))
			{
				bWait = TRUE;
			}
			else
			{
				ConErrResPrintf(STRING_TYPE_ERROR1, option);
				return 0;
			}
		}
		else
		{
			/* It's not an option - must be the beginning of
			 * the actual command. Leave the loop. */
			break;
		}
	}

	/* get comspec */
	comspec = cmd_alloc ( MAX_PATH * sizeof(TCHAR));
	if (comspec == NULL)
	{
		error_out_of_memory();
		return 1;
	}
	SetLastError(0);
	size = GetEnvironmentVariable (_T("COMSPEC"), comspec, MAX_PATH);
	if(GetLastError() == ERROR_ENVVAR_NOT_FOUND)
	{
		_tcscpy(comspec, _T("cmd"));
	}
	else
	{
		if (size > MAX_PATH)
		{
			comspec = cmd_realloc(comspec,size * sizeof(TCHAR) );
			if (comspec==NULL)
			{
				return 1;
			}
			size = GetEnvironmentVariable (_T("COMSPEC"), comspec, size);
		}
	}

	nErrorLevel = 0;

	if (!*Rest)
	{
		Rest = _T("cmd.exe");
	}
	else
	/* Parsing the command that gets called by start, and it's parameters */
	{
		BOOL bInside = FALSE;

		/* find the end of the command and put the arguments in param */
		for (i = 0; Rest[i]; i++)
		{
			if (Rest[i] == _T('\"'))
				bInside = !bInside;
			if (_istspace(Rest[i]) && !bInside)
			{
				param = &Rest[i+1];
				Rest[i] = _T('\0');
				break;
			}
		}
	}

	_tcscpy(szUnquotedName, Rest);
	StripQuotes(szUnquotedName);

	/* get the PATH environment variable and parse it */
	/* search the PATH environment variable for the binary */
	if (SearchForExecutable(szUnquotedName, szFullName))
	{
		/* check if this is a .BAT or .CMD file */
		dot = _tcsrchr(szFullName, _T('.'));
		if (dot && (!_tcsicmp(dot, _T(".bat")) || !_tcsicmp(dot, _T(".cmd"))))
		{
			bBat = TRUE;
			_stprintf(szFullCmdLine, _T("\"%s\" /K %s"), comspec, Rest);
			TRACE ("[BATCH: %s %s]\n", debugstr_aw(szFullName), debugstr_aw(Rest));
		}
		else
		{
			TRACE ("[EXEC: %s %s]\n", debugstr_aw(szFullName), debugstr_aw(Rest));
			_tcscpy(szFullCmdLine, Rest);
		}

		/* build command line for CreateProcess() */
		if (param != NULL)
		{
			_tcscat(szFullCmdLine, _T(" "));
			_tcscat(szFullCmdLine, param);
		}

		/* fill startup info */
		memset (&stui, 0, sizeof (STARTUPINFO));
		stui.cb = sizeof (STARTUPINFO);
		stui.dwFlags = STARTF_USESHOWWINDOW;
		stui.lpTitle = lpTitle;
		stui.wShowWindow = wShowWindow;

		if (bBat == TRUE)
		{
		 bCreate = CreateProcess (NULL, szFullCmdLine, NULL, NULL, FALSE,
			 CREATE_NEW_CONSOLE | Priority, NULL, lpDirectory, &stui, &prci);
		}
		else
		{
				bCreate = CreateProcess (szFullName, szFullCmdLine, NULL, NULL, FALSE,
					CREATE_NEW_CONSOLE | Priority, NULL, lpDirectory, &stui, &prci);
		}
		if (bCreate)
			CloseHandle(prci.hThread);
	}
	else
	{
		/* The file name did not seem to be valid, but maybe it's actually a
		 * directory or URL, so we still want to pass it to ShellExecute. */
		_tcscpy(szFullName, szUnquotedName);
	}

	if (!bCreate)
	{
		/* CreateProcess didn't work; try ShellExecute */
		prci.hProcess = RunFile(SEE_MASK_NOCLOSEPROCESS, szFullName,
		                        param, lpDirectory, wShowWindow);
	}

	if (prci.hProcess != NULL)
	{
			if (bWait)
			{
				DWORD dwExitCode;
				WaitForSingleObject (prci.hProcess, INFINITE);
				GetExitCodeProcess (prci.hProcess, &dwExitCode);
				nErrorLevel = (INT)dwExitCode;
			}
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


	cmd_free(comspec);
	return 0;
}

#endif

/* EOF */
