/*
 *  CMD.C - command-line interface.
 *
 *
 *  History:
 *
 *    17 Jun 1994 (Tim Norman)
 *        started.
 *
 *    08 Aug 1995 (Matt Rains)
 *        I have cleaned up the source code. changes now bring this source
 *        into guidelines for recommended programming practice.
 *
 *        A added the the standard FreeDOS GNU licence test to the
 *        initialize() function.
 *
 *        Started to replace puts() with printf(). this will help
 *        standardize output. please follow my lead.
 *
 *        I have added some constants to help making changes easier.
 *
 *    15 Dec 1995 (Tim Norman)
 *        major rewrite of the code to make it more efficient and add
 *        redirection support (finally!)
 *
 *    06 Jan 1996 (Tim Norman)
 *        finished adding redirection support!  Changed to use our own
 *        exec code (MUCH thanks to Svante Frey!!)
 *
 *    29 Jan 1996 (Tim Norman)
 *        added support for CHDIR, RMDIR, MKDIR, and ERASE, as per
 *        suggestion of Steffan Kaiser
 *
 *        changed "file not found" error message to "bad command or
 *        filename" thanks to Dustin Norman for noticing that confusing
 *        message!
 *
 *        changed the format to call internal commands (again) so that if
 *        they want to split their commands, they can do it themselves 
 *        (none of the internal functions so far need that much power, anyway)
 *
 *    27 Aug 1996 (Tim Norman)
 *        added in support for Oliver Mueller's ALIAS command
 *
 *    14 Jun 1997 (Steffan Kaiser)
 *        added ctrl-break handling and error level
 *
 *    16 Jun 1998 (Rob Lake)
 *        Runs command.com if /P is specified in command line.  Command.com
 *        also stays permanent.  If /C is in the command line, starts the
 *        program next in the line.
 *
 *    21 Jun 1998 (Rob Lake)
 *        Fixed up /C so that arguments for the program
 *
 *    08-Jul-1998 (John P. Price)
 *        Now sets COMSPEC environment variable
 *        misc clean up and optimization
 *        added date and time commands
 *        changed to using spawnl instead of exec.  exec does not copy the
 *        environment to the child process!
 *
 *    14 Jul 1998 (Hans B Pufal)
 *        Reorganised source to be more efficient and to more closely
 *        follow MS-DOS conventions. (eg %..% environment variable
 *        replacement works form command line as well as batch file.
 *
 *        New organisation also properly support nested batch files.
 *
 *        New command table structure is half way towards providing a
 *        system in which COMMAND will find out what internal commands
 *        are loaded
 *
 *    24 Jul 1998 (Hans B Pufal) [HBP_003]
 *        Fixed return value when called with /C option
 *
 *    27 Jul 1998  John P. Price
 *        added config.h include
 *
 *    28 Jul 1998  John P. Price
 *        added showcmds function to show commands and options available
 *
 *    07-Aug-1998 (John P Price <linux-guru@gcfl.net>)
 *        Fixed carrage return output to better match MSDOS with echo
 *        on or off. (marked with "JPP 19980708")
 *
 *    07-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        First ReactOS release.
 *        Extended length of commandline buffers to 512.
 *
 *    13-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added COMSPEC environment variable.
 *        Added "/t" support (color) on cmd command line.
 *
 *    07-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added help text ("cmd /?").
 *
 *    25-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection safe!
 *        Fixed redirections and piping.
 *        Piping is based on temporary files, but basic support
 *        for anonymous pipes already exists.
 *
 *    27-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Replaced spawnl() by CreateProcess().
 */

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#include "cmd.h"
#include "batch.h"


#define CMDLINE_LENGTH  512


BOOL bExit = FALSE;       /* indicates EXIT was typed */
BOOL bCanExit = TRUE;     /* indicates if this shell is exitable */
BOOL bCtrlBreak = FALSE;  /* Ctrl-Break or Ctrl-C hit */
BOOL bIgnoreEcho = FALSE; /* Ignore 'newline' before 'cls' */
INT  nErrorLevel = 0;     /* Errorlevel of last launched external program */
OSVERSIONINFO osvi;
HANDLE hIn;
HANDLE hOut;

#ifdef INCLUDE_CMD_COLOR
WORD wColor = FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN; /* current color */
WORD wDefColor = FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN; /* default color */
#endif


extern COMMAND cmds[];		/* The internal command table */


/*
 *  is character a delimeter when used on first word?
 *
 */

static BOOL IsDelimiter (TCHAR c)
{
	return (c == _T('/') || c == _T('=') || c == _T('\0') || _istspace (c));
}


/*
 * This command (in first) was not found in the command table
 *
 * first - first word on command line
 * rest  - rest of command line
 */

static VOID
Execute (LPTSTR first, LPTSTR rest)
{
	TCHAR szFullName[MAX_PATH];

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

		return;
	}

	/* get the PATH environment variable and parse it */
	/* search the PATH environment variable for the binary */
	if (!SearchForExecutable (first, szFullName))
	{
		error_bad_command ();
		return;
	}

	/* check if this is a .BAT or .CMD file */
	if (!_tcsicmp (_tcsrchr (szFullName, _T('.')), _T(".bat")) ||
		!_tcsicmp (_tcsrchr (szFullName, _T('.')), _T(".cmd")))
	{
#ifdef _DEBUG
		DebugPrintf ("[BATCH: %s %s]\n", szFullName, rest);
#endif
		Batch (szFullName, first, rest);
	}
	else
	{
		/* exec the program */
#ifndef __REACTOS__
		TCHAR szFullCmdLine [1024];
#endif
		PROCESS_INFORMATION prci;
		STARTUPINFO stui;

#ifdef _DEBUG
		DebugPrintf ("[EXEC: %s %s]\n", szFullName, rest);
#endif
#ifndef __REACTOS__
		/* build command line for CreateProcess() */
		_tcscpy (szFullCmdLine, szFullName);
		_tcscat (szFullCmdLine, _T(" "));
		_tcscat (szFullCmdLine, rest);
#endif

		/* fill startup info */
		memset (&stui, 0, sizeof (STARTUPINFO));
		stui.cb = sizeof (STARTUPINFO);
		stui.dwFlags = STARTF_USESHOWWINDOW;
		stui.wShowWindow = SW_SHOWDEFAULT;

#ifndef __REACTOS__                 
		if (CreateProcess (NULL, szFullCmdLine, NULL, NULL, FALSE,
		   0, NULL, NULL, &stui, &prci))
#else
		if (CreateProcess (szFullName, rest, NULL, NULL, FALSE,
                                   0, NULL, NULL, &stui, &prci))
#endif
		{
			DWORD dwExitCode;
			WaitForSingleObject (prci.hProcess, INFINITE);
			GetExitCodeProcess (prci.hProcess, &dwExitCode);
			nErrorLevel = (INT)dwExitCode;
			CloseHandle (prci.hThread);
			CloseHandle (prci.hProcess);
		}
		else
		{
			ErrorMessage (GetLastError (),
                                      "Error executing CreateProcess()!!\n");
		}
	}
}


/*
 * look through the internal commands and determine whether or not this
 * command is one of them.  If it is, call the command.  If not, call
 * execute to run it as an external program.
 *
 * line - the command line of the program to run
 *
 */

static VOID
DoCommand (LPTSTR line)
{
	TCHAR com[MAX_PATH];  /* the first word in the command */
	LPTSTR cp = com;
	LPTSTR cstart;
	LPTSTR rest = line;   /* pointer to the rest of the command line */
	INT cl;
	LPCOMMAND cmdptr;

	/* Skip over initial white space */
	while (isspace (*rest))
		rest++;

	cstart = rest;

	/* Anything to do ? */
	if (*rest)
	{
		/* Copy over 1st word as lower case */
		while (!IsDelimiter (*rest))
			*cp++ = _totlower (*rest++);

		/* Terminate first word */
		*cp = _T('\0');

		/* Skip over whitespace to rest of line */
		while (_istspace (*rest))
			rest++;

		/* Scan internal command table */
		for (cmdptr = cmds;; cmdptr++)
		{
			/* If end of table execute ext cmd */
			if (cmdptr->name == NULL)
			{
				Execute (com, rest);
				break;
			}

			if (!_tcscmp (com, cmdptr->name))
			{
				cmdptr->func (com, rest);
				break;
			}

			/* The following code handles the case of commands like CD which
			 * are recognised even when the command name and parameter are
			 * not space separated.
			 *
			 * e.g dir..
			 * cd\freda
			 */

			/* Get length of command name */
			cl = _tcslen (cmdptr->name);

			if ((cmdptr->flags & CMD_SPECIAL) &&
			    (!_tcsncmp (cmdptr->name, com, cl)) &&
			    (_tcschr (_T("\\.-"), *(com + cl))))
			{
				/* OK its one of the specials...*/

				/* Terminate first word properly */
				com[cl] = _T('\0');

				/* Call with new rest */
				cmdptr->func (com, cstart + cl);
				break;
			}
		}
	}
}


/*
 * process the command line and execute the appropriate functions
 * full input/output redirection and piping are supported
 */

VOID ParseCommandLine (LPTSTR s)
{
#ifdef FEATURE_REDIRECTION
	TCHAR in[CMDLINE_LENGTH] = "";
	TCHAR out[CMDLINE_LENGTH] = "";
	TCHAR err[CMDLINE_LENGTH] = "";
	TCHAR szTempPath[MAX_PATH] = _T(".\\");
	TCHAR szFileName[2][MAX_PATH] = {"", ""};
	HANDLE hFile[2] = {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE};
	LPTSTR t = NULL;
	INT  num = 0;
	INT  nRedirFlags = 0;

	HANDLE hOldConIn;
	HANDLE hOldConOut;
	HANDLE hOldConErr;
#endif /* FEATURE_REDIRECTION */

#ifdef _DEBUG
	DebugPrintf ("ParseCommandLine: (\'%s\')]\n", s);
#endif /* _DEBUG */

#ifdef FEATURE_ALIASES
	/* expand all aliases */
	ExpandAlias (s, CMDLINE_LENGTH);
#endif /* FEATURE_ALIAS */

#ifdef FEATURE_REDIRECTION
	/* find the temp path to store temporary files */
	GetTempPath (MAX_PATH, szTempPath);
	if (szTempPath[_tcslen (szTempPath) - 1] != _T('\\'))
		_tcscat (szTempPath, _T("\\"));

	/* get the redirections from the command line */
	num = GetRedirection (s, in, out, err, &nRedirFlags);

	/* more efficient, but do we really need to do this? */
	for (t = in; _istspace (*t); t++)
		;
	_tcscpy (in, t);

	for (t = out; _istspace (*t); t++)
		;
	_tcscpy (out, t);

	for (t = err; _istspace (*t); t++)
		;
	_tcscpy (err, t);

	/* Set up the initial conditions ... */
	/* preserve STDIN, STDOUT and STDERR handles */
	hOldConIn  = GetStdHandle (STD_INPUT_HANDLE);
	hOldConOut = GetStdHandle (STD_OUTPUT_HANDLE);
	hOldConErr = GetStdHandle (STD_ERROR_HANDLE);

	/* redirect STDIN */
	if (in[0])
	{
		HANDLE hFile;

		hFile = CreateFile (in, GENERIC_READ, 0, NULL, OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			ConErrPrintf ("Can't redirect input from file %s\n", in);
			return;
		}

		if (!SetStdHandle (STD_INPUT_HANDLE, hFile))
		{
			ConErrPrintf ("Can't redirect input from file %s\n", in);
			return;
		}
#ifdef _DEBUG
		DebugPrintf (_T("Input redirected from: %s\n"), in);
#endif
	}

	/* Now do all but the last pipe command */
	*szFileName[0] = '\0';
	hFile[0] = INVALID_HANDLE_VALUE;

	while (num-- > 1)
	{
		/* Create unique temporary file name */
		GetTempFileName (szTempPath, "CMD", 0, szFileName[1]);

		/* Set current stdout to temporary file */
		hFile[1] = CreateFile (szFileName[1], GENERIC_WRITE, 0, NULL,
							   TRUNCATE_EXISTING, FILE_ATTRIBUTE_TEMPORARY, NULL);
		SetStdHandle (STD_OUTPUT_HANDLE, hFile[1]);

		DoCommand (s);

		/* close stdout file */
		SetStdHandle (STD_OUTPUT_HANDLE, hOldConOut);
		if ((hFile[1] != INVALID_HANDLE_VALUE) && (hFile[1] != hOldConOut))
		{
			CloseHandle (hFile[1]);
			hFile[1] = INVALID_HANDLE_VALUE;
		}

		/* close old stdin file */
		SetStdHandle (STD_INPUT_HANDLE, hOldConIn);
		if ((hFile[0] != INVALID_HANDLE_VALUE) && (hFile[0] != hOldConIn))
		{
			/* delete old stdin file, if it is a real file */
			CloseHandle (hFile[0]);
			hFile[0] = INVALID_HANDLE_VALUE;
			DeleteFile (szFileName[0]);
			*szFileName[0] = _T('\0');
		}

		/* copy stdout file name to stdin file name */
		_tcscpy (szFileName[0], szFileName[1]);
		*szFileName[1] = _T('\0');

		/* open new stdin file */
		hFile[0] = CreateFile (szFileName[0], GENERIC_READ, 0, NULL,
							   OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY, NULL);
		SetStdHandle (STD_INPUT_HANDLE, hFile[0]);

		s = s + _tcslen (s) + 1;
	}

	/* Now set up the end conditions... */
	/* redirect STDOUT */
	if (out[0])
	{
		/* Final output to here */
		HANDLE hFile;

		hFile = CreateFile (out, GENERIC_WRITE, 0, NULL,
							(nRedirFlags & OUTPUT_APPEND) ? OPEN_ALWAYS : CREATE_ALWAYS,
							FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			ConErrPrintf ("Can't redirect to file %s\n", out);
			return;
		}

		if (!SetStdHandle (STD_OUTPUT_HANDLE, hFile))
		{
			ConErrPrintf ("Can't redirect to file %s\n", out);
			return;
		}

		if (nRedirFlags & OUTPUT_APPEND)
		{
			LONG lHighPos = 0;

			if (GetFileType (hFile) == FILE_TYPE_DISK)
				SetFilePointer (hFile, 0, &lHighPos, FILE_END);
		}
#ifdef _DEBUG
		DebugPrintf (_T("Output redirected to: %s\n"), out);
#endif
	}
	else if (hOldConOut != INVALID_HANDLE_VALUE)
	{
		/* Restore original stdout */
		HANDLE hOut = GetStdHandle (STD_OUTPUT_HANDLE);
		SetStdHandle (STD_OUTPUT_HANDLE, hOldConOut);
		if (hOldConOut != hOut)
			CloseHandle (hOut);
		hOldConOut = INVALID_HANDLE_VALUE;
	}

	/* redirect STDERR */
	if (err[0])
	{
		/* Final output to here */
		HANDLE hFile;

		if (!_tcscmp (err, out))
		{
#ifdef _DEBUG
			DebugPrintf (_T("Stdout and stderr will use the same file!!\n"));
#endif
			DuplicateHandle (GetCurrentProcess (), GetStdHandle (STD_OUTPUT_HANDLE), GetCurrentProcess (),
							 &hFile, 0, TRUE, DUPLICATE_SAME_ACCESS);
		}
		else
		{
			hFile =
				CreateFile (err, GENERIC_WRITE, 0, NULL,
							(nRedirFlags & ERROR_APPEND) ? OPEN_ALWAYS : CREATE_ALWAYS,
							FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				ConErrPrintf ("Can't redirect to file %s\n", err);
				return;
			}
		}
		if (!SetStdHandle (STD_ERROR_HANDLE, hFile))
		{
			ConErrPrintf ("Can't redirect to file %s\n", err);
			return;
		}

		if (nRedirFlags & ERROR_APPEND)
		{
			LONG lHighPos = 0;

			if (GetFileType (hFile) == FILE_TYPE_DISK)
				SetFilePointer (hFile, 0, &lHighPos, FILE_END);
		}
#ifdef _DEBUG
		DebugPrintf (_T("Error redirected to: %s\n"), err);
#endif
	}
	else if (hOldConErr != INVALID_HANDLE_VALUE)
	{
		/* Restore original stderr */
		HANDLE hErr = GetStdHandle (STD_ERROR_HANDLE);
		SetStdHandle (STD_ERROR_HANDLE, hOldConErr);
		if (hOldConErr != hErr)
			CloseHandle (hErr);
		hOldConErr = INVALID_HANDLE_VALUE;
	}
#endif  /* FEATURE_REDIRECTION */

	/* process final command */
	DoCommand (s);

#ifdef FEATURE_REDIRECTION
	/* close old stdin file */
#if 0  /* buggy implementation */
	SetStdHandle (STD_INPUT_HANDLE, hOldConIn);
	if ((hFile[0] != INVALID_HANDLE_VALUE) && 
		(hFile[0] != hOldConIn))
	{
		/* delete old stdin file, if it is a real file */
		CloseHandle (hFile[0]);
		hFile[0] = INVALID_HANDLE_VALUE;
		DeleteFile (szFileName[0]);
		*szFileName[0] = _T('\0');
	}

	/* Restore original STDIN */
	if (hOldConIn != INVALID_HANDLE_VALUE)
	{
		HANDLE hIn = GetStdHandle (STD_INPUT_HANDLE);
		SetStdHandle (STD_INPUT_HANDLE, hOldConIn);
		if (hOldConIn != hIn)
			CloseHandle (hIn);
		hOldConIn = INVALID_HANDLE_VALUE;
	}
	else
	{
#ifdef _DEBUG
		DebugPrintf (_T("Can't restore STDIN! Is invalid!!\n"), out);
#endif
	}
#endif  /* buggy implementation */


	if (hOldConIn != INVALID_HANDLE_VALUE)
	{
		HANDLE hIn = GetStdHandle (STD_INPUT_HANDLE);
		SetStdHandle (STD_INPUT_HANDLE, hOldConIn);
		if (hIn == INVALID_HANDLE_VALUE)
		{
#ifdef _DEBUG
			DebugPrintf (_T("Previous STDIN is invalid!!\n"));
#endif
		}
		else
		{
			if (GetFileType (hIn) == FILE_TYPE_DISK)
			{
				if (hFile[0] == hIn)
				{
					CloseHandle (hFile[0]);
					hFile[0] = INVALID_HANDLE_VALUE;
					DeleteFile (szFileName[0]);
					*szFileName[0] = _T('\0');
				}
				else
				{
#ifdef _DEBUG
					DebugPrintf (_T("hFile[0] and hIn dont match!!!\n"));
#endif
				}
			}
		}
	}


	/* Restore original STDOUT */
	if (hOldConOut != INVALID_HANDLE_VALUE)
	{
		HANDLE hOut = GetStdHandle (STD_OUTPUT_HANDLE);
		SetStdHandle (STD_OUTPUT_HANDLE, hOldConOut);
		if (hOldConOut != hOut)
			CloseHandle (hOut);
		hOldConOut = INVALID_HANDLE_VALUE;
	}

	/* Restore original STDERR */
	if (hOldConErr != INVALID_HANDLE_VALUE)
	{
		HANDLE hErr = GetStdHandle (STD_ERROR_HANDLE);
		SetStdHandle (STD_ERROR_HANDLE, hOldConErr);
		if (hOldConErr != hErr)
			CloseHandle (hErr);
		hOldConErr = INVALID_HANDLE_VALUE;
	}
#endif /* FEATURE_REDIRECTION */
}


/*
 * do the prompt/input/process loop
 *
 */

static INT 
ProcessInput (BOOL bFlag)
{
	TCHAR commandline[CMDLINE_LENGTH];
	TCHAR readline[CMDLINE_LENGTH];
	LPTSTR tp;
	LPTSTR ip;
	LPTSTR cp;

	/* JPP 19980807 - changed name so not to conflict with echo global */
	BOOL bEchoThisLine;

	do
	{
		/* if no batch input then... */
		if (!(ip = ReadBatchLine (&bEchoThisLine)))
		{
			if (bFlag)
				return 0;

			ReadCommand (readline, CMDLINE_LENGTH);
			ip = readline;
			bEchoThisLine = FALSE;
		}

		cp = commandline;
		while (*ip)
		{
			if (*ip == _T('%'))
			{
				switch (*++ip)
				{
					case _T('%'):
						*cp++ = *ip++;
						break;

					case _T('0'):
					case _T('1'):
					case _T('2'):
					case _T('3'):
					case _T('4'):
					case _T('5'):
					case _T('6'):
					case _T('7'):
					case _T('8'):
					case _T('9'):
						if ((tp = FindArg (*ip - _T('0'))))
						{
							cp = stpcpy (cp, tp);
							ip++;
						}
						else
							*cp++ = _T('%');
						break;

					case _T('?'):
						cp += _stprintf (cp, _T("%u"), nErrorLevel);
						ip++;
						break;

					default:
						if ((tp = _tcschr (ip, _T('%'))))
						{
							char evar[512];
							*tp = _T('\0');

							/* FIXME: This is just a quick hack!! */
							/* Do a proper memory allocation!! */
							if (GetEnvironmentVariable (ip, evar, 512))
								cp = stpcpy (cp, evar);

							ip = tp + 1;
						}
						break;
				}
				continue;
			}

			if (_istcntrl (*ip))
				*ip = _T(' ');
			*cp++ = *ip++;
		}

		*cp = _T('\0');

		/* strip trailing spaces */
		while ((--cp >= commandline) && _istspace (*cp))
			;

		*(cp + 1) = _T('\0');

		/* JPP 19980807 */
		/* Echo batch file line */
		if (bEchoThisLine)
		{
			PrintPrompt ();
			ConOutPuts (commandline);
		}

		if (*commandline)
		{
			ParseCommandLine (commandline);
			if (bEcho && !bIgnoreEcho)
				ConOutChar ('\n');
			bIgnoreEcho = FALSE;
		}
	}
	while (!bCanExit || !bExit);

	return 0;
}


/*
 * control-break handler.
 */
BOOL BreakHandler (DWORD dwCtrlType)
{
	if ((dwCtrlType == CTRL_C_EVENT) ||
		(dwCtrlType == CTRL_BREAK_EVENT))
	{
		bCtrlBreak = TRUE; /* indicate the break condition */
		return TRUE;
	}
	return FALSE;
}


/*
 * show commands and options that are available.
 *
 */
static VOID
ShowCommands (VOID)
{
	LPCOMMAND cmdptr;
	INT y;

	ConOutPrintf (_T("\nInternal commands available:\n"));
	y = 0;
	cmdptr = cmds;
	while (cmdptr->name)
	{
		if (++y == 8)
		{
			ConOutPuts (cmdptr->name);
			y = 0;
		}
		else
			ConOutPrintf (_T("%-10s"), cmdptr->name);

		cmdptr++;
	}

	if (y != 0)
		ConOutChar ('\n');

	/* print feature list */
	ConOutPuts ("\nFeatures available:");
#ifdef FEATURE_ALIASES
	ConOutPuts ("  [aliases]");
#endif
#ifdef FEATURE_HISTORY
	ConOutPuts ("  [history]");
#endif
#ifdef FEATURE_UNIX_FILENAME_COMPLETION
	ConOutPuts ("  [unix filename completion]");
#endif
#ifdef FEATURE_DIRECTORY_STACK
	ConOutPuts ("  [directory stack]");
#endif
#ifdef FEATURE_REDIRECTION
	ConOutPuts ("  [redirections and piping]");
#endif
	ConOutChar ('\n');
}


/*
 * set up global initializations and process parameters
 *
 * argc - number of parameters to command.com
 * argv - command-line parameters
 *
 */
static VOID Initialize (int argc, char *argv[])
{
	INT i;

	/* Added by Rob Lake 06/16/98.  This enables the command.com
	 * to run the autoexec.bat at startup */

#ifdef _DEBUG
	INT x;

	DebugPrintf ("[command args:\n");
	for (x = 0; x < argc; x++)
	{
		DebugPrintf ("%d. %s\n", x, argv[x]);
	}
	DebugPrintf ("]\n");
#endif

	/* get version information */
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx (&osvi);

	InitLocale ();

	/* get default input and output console handles */
	hOut = GetStdHandle (STD_OUTPUT_HANDLE);
	hIn  = GetStdHandle (STD_INPUT_HANDLE);

#ifdef INCLUDE_CMD_CHDIR
	InitLastPath ();
#endif

	if (argc >= 2)
	{
		if (!_tcsncmp (argv[1], _T("/?"), 2))
		{
			ConOutPuts (_T("Starts a new instance of the ReactOS command line interpreter\n\n"
						   "CMD [/P][/C]...\n\n"
						   "  /P  ...\n"
						   "  /C  ..."));
			ExitProcess (0);
		}
		else
		{
			for (i = 1; i < argc; i++)
			{
				if (!_tcsicmp (argv[i], _T("/p")))
				{
					if (!IsValidFileName (_T("\\autoexec.bat")))
					{
#ifdef INCLUDE_CMD_DATE
						cmd_date ("", "");
#endif
#ifdef INCLUDE_CMD_TIME
						cmd_time ("", "");
#endif
					}
					else
						ParseCommandLine ("\\autoexec.bat");
					bCanExit = FALSE;
				}
				else if (!_tcsicmp (argv[i], _T("/c")))
				{
					/* This just runs a program and exits, RL: 06/16,21/98 */
					char commandline[CMDLINE_LENGTH];
					++i;
					strcpy(commandline, argv[i]);
					while (argv[++i])
					{
						strcat(commandline, " ");
						strcat(commandline, argv[i]);
					}

					ParseCommandLine(commandline);

					/* HBP_003 { Fix return value when /C used }*/
					ExitProcess (ProcessInput (TRUE));
				}

#ifdef INCLUDE_CMD_COLOR
				else if (!_tcsnicmp (argv[i], _T("/t:"), 3))
				{
					/* process /t (color) argument */
					wDefColor = (WORD)strtoul (&argv[i][3], NULL, 16);
					wColor = wDefColor;
				}
#endif
			}
		}
	}

#ifdef FEATURE_DIR_STACK
	/* initialize directory stack */
	InitDirectoryStack ();
#endif

#ifdef INCLUDE_CMD_COLOR
	/* set default colors */
	SetScreenColor (wColor);
#endif

	ShortVersion ();
	ShowCommands ();

	/* Set COMSPEC environment variable */
#ifndef __REACTOS__
	if (argv)
		SetEnvironmentVariable (_T("COMSPEC"), argv[0]);
#endif

	/* add ctrl handler */
#if 0
	SetConsoleCtrlHandler (NULL, TRUE);
#endif
}


static VOID Cleanup (VOID)
{

#ifdef FEATURE_DIECTORY_STACK
	/* destroy directory stack */
	DestroyDirectoryStack ();
#endif

#ifdef INCLUDE_CMD_CHDIR
	FreeLastPath ();
#endif

	/* remove ctrl handler */
#if 0
	SetConsoleCtrlHandler ((PHANDLER_ROUTINE)&BreakHandler, FALSE);
#endif
}


/*
 * main function
 */
int main (int argc, char *argv[])
{
	INT nExitCode;

	AllocConsole ();
#ifndef __REACTOS__
	SetFileApisToOEM ();
#endif

#ifdef __REACTOS__
	SetCurrentDirectory (_T("C:\\"));
#endif

	/* check switches on command-line */
	Initialize (argc, argv);

	/* call prompt routine */
	nExitCode = ProcessInput (FALSE);

	/* do the cleanup */
	Cleanup ();
	FreeConsole ();

	return nExitCode;
}
