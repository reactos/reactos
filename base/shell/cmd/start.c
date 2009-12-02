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
#ifdef UNICODE
	DWORD dwCreationFlags = CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT;
#else
	DWORD dwCreationFlags = CREATE_NEW_CONSOLE;
#endif
	DWORD dwAffinityMask = 0;
	LPTSTR lpTitle = NULL;
	LPTSTR lpDirectory = NULL;
	LPTSTR lpEnvironment = NULL;
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
			else if (_totupper(*option) == _T('I'))
			{
				/* rest of the option is apparently ignored */
				lpEnvironment = lpOriginalEnvironment;
			}
			else if (!_tcsicmp(option, _T("MIN")))
			{
				wShowWindow = SW_MINIMIZE;
			}
			else if (!_tcsicmp(option, _T("MAX")))
			{
				wShowWindow = SW_MAXIMIZE;
			}
			else if (!_tcsicmp(option, _T("AFFINITY")))
			{
				TCHAR *end;
				while (_istspace(*Rest))
					Rest++;
				option = GetParameter(&Rest);
				/* Affinity mask is given in hexadecimal */
				dwAffinityMask = _tcstoul(option, &end, 16);
				if (*end != _T('\0') || dwAffinityMask == 0 ||
				    dwAffinityMask == (DWORD)-1)
				{
					ConErrResPrintf(STRING_ERROR_INVALID_PARAM_FORMAT, option);
					return 1;
				}
				dwCreationFlags |= CREATE_SUSPENDED;
			}
			else if (!_tcsicmp(option, _T("B")))
			{
				dwCreationFlags &= ~CREATE_NEW_CONSOLE;
				dwCreationFlags |= CREATE_NEW_PROCESS_GROUP;
			}
			else if (!_tcsicmp(option, _T("LOW")))
			{
				dwCreationFlags |= IDLE_PRIORITY_CLASS;
			}
			else if (!_tcsicmp(option, _T("NORMAL")))
			{
				dwCreationFlags |= NORMAL_PRIORITY_CLASS;
			}
			else if (!_tcsicmp(option, _T("HIGH")))
			{
				dwCreationFlags |= HIGH_PRIORITY_CLASS;
			}
			else if (!_tcsicmp(option, _T("REALTIME")))
			{
				dwCreationFlags |= REALTIME_PRIORITY_CLASS;
			}
			else if (!_tcsicmp(option, _T("ABOVENORMAL")))
			{
				dwCreationFlags |= ABOVE_NORMAL_PRIORITY_CLASS;
			}
			else if (!_tcsicmp(option, _T("BELOWNORMAL")))
			{
				dwCreationFlags |= BELOW_NORMAL_PRIORITY_CLASS;
			}
			else if (!_tcsicmp(option, _T("SEPARATE")))
			{
				dwCreationFlags |= CREATE_SEPARATE_WOW_VDM;
			}
			else if (!_tcsicmp(option, _T("SHARED")))
			{
				dwCreationFlags |= CREATE_SHARED_WOW_VDM;
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

		bCreate = CreateProcess(bBat ? comspec : szFullName,
		                        szFullCmdLine, NULL, NULL, TRUE, dwCreationFlags,
		                        lpEnvironment, lpDirectory, &stui, &prci);
		if (bCreate)
		{
			if (dwAffinityMask)
			{
				SetProcessAffinityMask(prci.hProcess, dwAffinityMask);
				ResumeThread(prci.hThread);
			}
			CloseHandle(prci.hThread);
		}
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
		DWORD flags = SEE_MASK_NOCLOSEPROCESS;
		if (!(dwCreationFlags & CREATE_NEW_CONSOLE))
			flags |= SEE_MASK_NO_CONSOLE;
		prci.hProcess = RunFile(flags, szFullName, param, lpDirectory, wShowWindow);
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
