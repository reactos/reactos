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
 *    07-Dec-1998 (Eric Kohl)
 *        First ReactOS release.
 *        Extended length of commandline buffers to 512.
 *
 *    13-Dec-1998 (Eric Kohl)
 *        Added COMSPEC environment variable.
 *        Added "/t" support (color) on cmd command line.
 *
 *    07-Jan-1999 (Eric Kohl)
 *        Added help text ("cmd /?").
 *
 *    25-Jan-1999 (Eric Kohl)
 *        Unicode and redirection safe!
 *        Fixed redirections and piping.
 *        Piping is based on temporary files, but basic support
 *        for anonymous pipes already exists.
 *
 *    27-Jan-1999 (Eric Kohl)
 *        Replaced spawnl() by CreateProcess().
 *
 *    22-Oct-1999 (Eric Kohl)
 *        Added break handler.
 *
 *    15-Dec-1999 (Eric Kohl)
 *        Fixed current directory
 *
 *    28-Dec-1999 (Eric Kohl)
 *        Restore window title after program/batch execution
 *
 *    03-Feb-2001 (Eric Kohl)
 *        Workaround because argc[0] is NULL under ReactOS
 *
 *    23-Feb-2001 (Carl Nettelblad <cnettel@hem.passagen.se>)
 *        %envvar% replacement conflicted with for.
 *
 *    30-Apr-2004 (Filip Navara <xnavara@volny.cz>)
 *       Make MakeSureDirectoryPathExistsEx unicode safe.
 *
 *    28-Mai-2004
 *       Removed MakeSureDirectoryPathExistsEx.
 *       Use the current directory if GetTempPath fails.
 *
 *    12-Jul-2004 (Jens Collin <jens.collin@lakhei.com>)
 *       Added ShellExecute call when all else fails to be able to "launch" any file.
 *
 *    02-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 *
 *    06-May-2005 (Klemens Friedl <frik85@gmail.com>)
 *        Add 'help' command (list all commands plus description)
 *
 *    06-jul-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        translate '%errorlevel%' to the internal value.
 *        Add proper memmory alloc ProcessInput, the error
 *        handling for memmory handling need to be improve
 */

#include <precomp.h>

#ifndef NT_SUCCESS
#define NT_SUCCESS(StatCode)  ((NTSTATUS)(StatCode) >= 0)
#endif

typedef NTSTATUS (WINAPI *NtQueryInformationProcessProc)(HANDLE, PROCESSINFOCLASS,
                                                          PVOID, ULONG, PULONG);
typedef NTSTATUS (WINAPI *NtReadVirtualMemoryProc)(HANDLE, PVOID, PVOID, ULONG, PULONG);

BOOL bExit = FALSE;       /* indicates EXIT was typed */
BOOL bCanExit = TRUE;     /* indicates if this shell is exitable */
BOOL bCtrlBreak = FALSE;  /* Ctrl-Break or Ctrl-C hit */
BOOL bIgnoreEcho = FALSE; /* Set this to TRUE to prevent a newline, when executing a command */
INT  nErrorLevel = 0;     /* Errorlevel of last launched external program */
BOOL bChildProcessRunning = FALSE;
DWORD dwChildProcessId = 0;
OSVERSIONINFO osvi;
HANDLE hIn;
HANDLE hOut;
HANDLE hConsole;
HANDLE CMD_ModuleHandle;
HMODULE NtDllModule;

static NtQueryInformationProcessProc NtQueryInformationProcessPtr = NULL;
static NtReadVirtualMemoryProc       NtReadVirtualMemoryPtr = NULL;

#ifdef INCLUDE_CMD_COLOR
WORD wColor;              /* current color */
WORD wDefColor;           /* default color */
#endif

/*
 * convert
 *
 * insert commas into a number
 */
INT
ConvertULargeInteger (ULARGE_INTEGER num, LPTSTR des, INT len, BOOL bPutSeperator)
{
	TCHAR temp[32];
	UINT  n, iTarget;

	if (len <= 1)
		return 0;

	if (num.QuadPart == 0)
	{
		des[0] = _T('0');
		des[1] = _T('\0');
		return 1;
	}

	n = 0;
	iTarget = nNumberGroups;
	if (!nNumberGroups)
		bPutSeperator = FALSE;

	while (num.QuadPart > 0)
	{
		if (iTarget == n && bPutSeperator)
		{
			iTarget += nNumberGroups + 1;
			temp[31 - n++] = cThousandSeparator;
		}
		temp[31 - n++] = (TCHAR)(num.QuadPart % 10) + _T('0');
		num.QuadPart /= 10;
	}
	if (n > len-1)
		n = len-1;

	memcpy(des, temp + 32 - n, n * sizeof(TCHAR));
	des[n] = _T('\0');

	return n;
}

/*
 * is character a delimeter when used on first word?
 *
 */
static BOOL IsDelimiter (TCHAR c)
{
	return (c == _T('/') || c == _T('=') || c == _T('\0') || _istspace (c));
}

/*
 * Is a process a console process?
 */
static BOOL IsConsoleProcess(HANDLE Process)
{
	NTSTATUS Status;
	PROCESS_BASIC_INFORMATION Info;
	PEB ProcessPeb;
	ULONG BytesRead;

	if (NULL == NtQueryInformationProcessPtr || NULL == NtReadVirtualMemoryPtr)
	{
		return TRUE;
	}

	Status = NtQueryInformationProcessPtr (
		Process, ProcessBasicInformation,
		&Info, sizeof(PROCESS_BASIC_INFORMATION), NULL);
	if (! NT_SUCCESS(Status))
	{
		WARN ("NtQueryInformationProcess failed with status %08x\n", Status);
		return TRUE;
	}
	Status = NtReadVirtualMemoryPtr (
		Process, Info.PebBaseAddress, &ProcessPeb,
		sizeof(PEB), &BytesRead);
	if (! NT_SUCCESS(Status) || sizeof(PEB) != BytesRead)
	{
		WARN ("Couldn't read virt mem status %08x bytes read %lu\n", Status, BytesRead);
		return TRUE;
	}

	return IMAGE_SUBSYSTEM_WINDOWS_CUI == ProcessPeb.ImageSubSystem;
}



#ifdef _UNICODE
#define SHELLEXECUTETEXT   	"ShellExecuteW"
#else
#define SHELLEXECUTETEXT   	"ShellExecuteA"
#endif

typedef HINSTANCE (WINAPI *MYEX)(
	HWND hwnd,
	LPCTSTR lpOperation,
	LPCTSTR lpFile,
	LPCTSTR lpParameters,
	LPCTSTR lpDirectory,
	INT nShowCmd
);



static BOOL RunFile(LPTSTR filename)
{
	HMODULE     hShell32;
	MYEX        hShExt;
	HINSTANCE   ret;

	TRACE ("RunFile(%s)\n", debugstr_aw(filename));
	hShell32 = LoadLibrary(_T("SHELL32.DLL"));
	if (!hShell32)
	{
		WARN ("RunFile: couldn't load SHELL32.DLL!\n");
		return FALSE;
	}

	hShExt = (MYEX)(FARPROC)GetProcAddress(hShell32, SHELLEXECUTETEXT);
	if (!hShExt)
	{
		WARN ("RunFile: couldn't find ShellExecuteA/W in SHELL32.DLL!\n");
		FreeLibrary(hShell32);
		return FALSE;
	}

	TRACE ("RunFile: ShellExecuteA/W is at %x\n", hShExt);

	ret = (hShExt)(NULL, _T("open"), filename, NULL, NULL, SW_SHOWNORMAL);

	TRACE ("RunFile: ShellExecuteA/W returned %d\n", (DWORD)ret);

	FreeLibrary(hShell32);
	return (((DWORD)ret) > 32);
}



/*
 * This command (in first) was not found in the command table
 *
 * Full  - whole command line
 * First - first word on command line
 * Rest  - rest of command line
 */

static BOOL
Execute (LPTSTR Full, LPTSTR First, LPTSTR Rest)
{
	TCHAR *szFullName=NULL;
	TCHAR *first = NULL;
	TCHAR *rest = NULL;
	TCHAR *full = NULL;
	TCHAR *dot = NULL;
	TCHAR szWindowTitle[MAX_PATH];
	DWORD dwExitCode = 0;

	TRACE ("Execute: \'%s\' \'%s\'\n", debugstr_aw(first), debugstr_aw(rest));

	/* we need biger buffer that First, Rest, Full are already
	   need rewrite some code to use cmd_realloc when it need instead
	   of add 512bytes extra */

	first = cmd_alloc ( (_tcslen(First) + 512) * sizeof(TCHAR));
	if (first == NULL)
	{
		error_out_of_memory();
                nErrorLevel = 1;
		return FALSE;
	}

	rest = cmd_alloc ( (_tcslen(Rest) + 512) * sizeof(TCHAR));
	if (rest == NULL)
	{
		cmd_free (first);
		error_out_of_memory();
                nErrorLevel = 1;
		return FALSE;
	}

	full = cmd_alloc ( (_tcslen(Full) + 512) * sizeof(TCHAR));
	if (full == NULL)
	{
		cmd_free (first);
		cmd_free (rest);
		error_out_of_memory();
                nErrorLevel = 1;
		return FALSE;
	}

	szFullName = cmd_alloc ( (_tcslen(Full) + 512) * sizeof(TCHAR));
	if (full == NULL)
	{
		cmd_free (first);
		cmd_free (rest);
		cmd_free (full);
		error_out_of_memory();
                nErrorLevel = 1;
		return FALSE;
	}


	/* Though it was already parsed once, we have a different set of rules
	   for parsing before we pass to CreateProccess */
	if(!_tcschr(Full,_T('\"')))
	{
		_tcscpy(first,First);
		_tcscpy(rest,Rest);
		_tcscpy(full,Full);
	}
	else
	{
		UINT i = 0;
		BOOL bInside = FALSE;
		rest[0] = _T('\0');
		full[0] = _T('\0');
		first[0] = _T('\0');
		_tcscpy(first,Full);
		/* find the end of the command and start of the args */
		for(i = 0; i < _tcslen(first); i++)
		{
			if(!_tcsncmp(&first[i], _T("\""), 1))
				bInside = !bInside;
			if(!_tcsncmp(&first[i], _T(" "), 1) && !bInside)
			{
				_tcscpy(rest,&first[i]);
				first[i] = _T('\0');
				break;
			}

		}
		i = 0;
		/* remove any slashes */
		while(i < _tcslen(first))
		{
			if(first[i] == _T('\"'))
				memmove(&first[i],&first[i + 1], _tcslen(&first[i]) * sizeof(TCHAR));
			else
				i++;
		}
		/* Drop quotes around it just in case there is a space */
		_tcscpy(full,_T("\""));
		_tcscat(full,first);
		_tcscat(full,_T("\" "));
		_tcscat(full,rest);
	}

	/* check for a drive change */
	if ((_istalpha (first[0])) && (!_tcscmp (first + 1, _T(":"))))
	{
		BOOL working = TRUE;
		if (!SetCurrentDirectory(first))
		/* Guess they changed disc or something, handle that gracefully and get to root */
		{
			TCHAR str[4];
			str[0]=first[0];
			str[1]=_T(':');
			str[2]=_T('\\');
			str[3]=0;
			working = SetCurrentDirectory(str);
		}

		if (!working) ConErrResPuts (STRING_FREE_ERROR1);

		cmd_free (first);
		cmd_free (rest);
		cmd_free (full);
		cmd_free (szFullName);
                nErrorLevel = 1;
		return working;
	}

	/* get the PATH environment variable and parse it */
	/* search the PATH environment variable for the binary */
	if (!SearchForExecutable (first, szFullName))
	{
			error_bad_command ();
			cmd_free (first);
			cmd_free (rest);
			cmd_free (full);
			cmd_free (szFullName);
                        nErrorLevel = 1;
			return FALSE;

	}

	GetConsoleTitle (szWindowTitle, MAX_PATH);

	/* check if this is a .BAT or .CMD file */
	dot = _tcsrchr (szFullName, _T('.'));
	if (dot && (!_tcsicmp (dot, _T(".bat")) || !_tcsicmp (dot, _T(".cmd"))))
	{
		TRACE ("[BATCH: %s %s]\n", debugstr_aw(szFullName), debugstr_aw(rest));
		Batch (szFullName, first, rest, FALSE);
	}
	else
	{
		/* exec the program */
		PROCESS_INFORMATION prci;
		STARTUPINFO stui;

		TRACE ("[EXEC: %s %s]\n", debugstr_aw(full), debugstr_aw(rest));
		/* build command line for CreateProcess() */

		/* fill startup info */
		memset (&stui, 0, sizeof (STARTUPINFO));
		stui.cb = sizeof (STARTUPINFO);
		stui.dwFlags = STARTF_USESHOWWINDOW;
		stui.wShowWindow = SW_SHOWDEFAULT;

		// return console to standard mode
		SetConsoleMode (GetStdHandle(STD_INPUT_HANDLE),
		                ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT );

		if (CreateProcess (szFullName,
		                   full,
		                   NULL,
		                   NULL,
		                   TRUE,
		                   0,			/* CREATE_NEW_PROCESS_GROUP */
		                   NULL,
		                   NULL,
		                   &stui,
		                   &prci))

		{
			if (IsConsoleProcess(prci.hProcess))
			{
				/* FIXME: Protect this with critical section */
				bChildProcessRunning = TRUE;
				dwChildProcessId = prci.dwProcessId;

				WaitForSingleObject (prci.hProcess, INFINITE);

				/* FIXME: Protect this with critical section */
				bChildProcessRunning = FALSE;

				GetExitCodeProcess (prci.hProcess, &dwExitCode);
				nErrorLevel = (INT)dwExitCode;
			}
                        else
                        {
                            nErrorLevel = 0;
                        }
			CloseHandle (prci.hThread);
			CloseHandle (prci.hProcess);
		}
		else
		{
			TRACE ("[ShellExecute: %s]\n", debugstr_aw(full));
			// See if we can run this with ShellExecute() ie myfile.xls
			if (!RunFile(full))
			{
				TRACE ("[ShellExecute failed!: %s]\n", debugstr_aw(full));
				error_bad_command ();
                                nErrorLevel = 1;
			}
                        else
                        {
                                nErrorLevel = 0;
                        }
		}
		// restore console mode
		SetConsoleMode (
			GetStdHandle( STD_INPUT_HANDLE ),
			ENABLE_PROCESSED_INPUT );
	}

	/* Get code page if it has been change */
	InputCodePage= GetConsoleCP();
	OutputCodePage = GetConsoleOutputCP();
	SetConsoleTitle (szWindowTitle);

	cmd_free(first);
	cmd_free(rest);
	cmd_free(full);
	cmd_free (szFullName);
	return nErrorLevel == 0;
}


/*
 * look through the internal commands and determine whether or not this
 * command is one of them.  If it is, call the command.  If not, call
 * execute to run it as an external program.
 *
 * line - the command line of the program to run
 *
 */

BOOL
DoCommand (LPTSTR line)
{
	TCHAR *com = NULL;  /* the first word in the command */
	TCHAR *cp = NULL;
	LPTSTR cstart;
	LPTSTR rest;   /* pointer to the rest of the command line */
	INT cl;
	LPCOMMAND cmdptr;
	BOOL ret = TRUE;

	TRACE ("DoCommand: (\'%s\')\n", debugstr_aw(line));

	com = cmd_alloc( (_tcslen(line) +512)*sizeof(TCHAR) );
	if (com == NULL)
	{
		error_out_of_memory();
		return FALSE;
	}

	cp = com;
	/* Skip over initial white space */
	while (_istspace (*line))
		line++;
	rest = line;

	cstart = rest;

	/* Anything to do ? */
	if (*rest)
	{
		if (*rest == _T('"'))
		{
			/* treat quoted words specially */

			rest++;

			while(*rest != _T('\0') && *rest != _T('"'))
				*cp++ = _totlower (*rest++);
			if (*rest == _T('"'))
				rest++;
		}
		else
		{
			while (!IsDelimiter (*rest))
				*cp++ = _totlower (*rest++);
		}


		/* Terminate first word */
		*cp = _T('\0');

		/* Do not limit commands to MAX_PATH */
		/*
		if(_tcslen(com) > MAX_PATH)
		{
			error_bad_command();
			cmd_free(com);
			return;
		}
		*/

		/* Skip over whitespace to rest of line, exclude 'echo' command */
		if (_tcsicmp (com, _T("echo")))
		{
			while (_istspace (*rest))
			rest++;
		}

		/* Scan internal command table */
		for (cmdptr = cmds;; cmdptr++)
		{
			/* If end of table execute ext cmd */
			if (cmdptr->name == NULL)
			{
				ret = Execute (line, com, rest);
				break;
			}

			if (!_tcscmp (com, cmdptr->name))
			{
				cmdptr->func (rest);
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

				/* Call with new rest */
				cmdptr->func (cstart + cl);
				break;
			}
		}
	}
	cmd_free(com);
	return ret;
}


/*
 * process the command line and execute the appropriate functions
 * full input/output redirection and piping are supported
 */

VOID ParseCommandLine (LPTSTR cmd)
{
	PARSED_COMMAND *Cmd = ParseCommand(cmd);
	if (Cmd)
	{
		ExecuteCommand(Cmd);
		FreeCommand(Cmd);
	}
}

static VOID
ExecutePipeline(PARSED_COMMAND *Cmd)
{
#ifdef FEATURE_REDIRECTION
	TCHAR szTempPath[MAX_PATH] = _T(".\\");
	TCHAR szFileName[2][MAX_PATH] = {_T(""), _T("")};
	HANDLE hFile[2] = {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE};
	INT  Length;
	UINT Attributes;
	HANDLE hOldConIn;
	HANDLE hOldConOut;
#endif /* FEATURE_REDIRECTION */

	//TRACE ("ParseCommandLine: (\'%s\')\n", debugstr_aw(s));

#ifdef FEATURE_REDIRECTION
	/* find the temp path to store temporary files */
	Length = GetTempPath (MAX_PATH, szTempPath);
	if (Length > 0 && Length < MAX_PATH)
	{
		Attributes = GetFileAttributes(szTempPath);
		if (Attributes == 0xffffffff ||
		    !(Attributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			Length = 0;
		}
	}
	if (Length == 0 || Length >= MAX_PATH)
	{
		_tcscpy(szTempPath, _T(".\\"));
	}
	if (szTempPath[_tcslen (szTempPath) - 1] != _T('\\'))
		_tcscat (szTempPath, _T("\\"));

	/* Set up the initial conditions ... */
	/* preserve STDIN and STDOUT handles */
	hOldConIn  = GetStdHandle (STD_INPUT_HANDLE);
	hOldConOut = GetStdHandle (STD_OUTPUT_HANDLE);

	/* Now do all but the last pipe command */
	*szFileName[0] = _T('\0');
	hFile[0] = INVALID_HANDLE_VALUE;

	while (Cmd->Type == C_PIPE)
	{
		SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

		/* Create unique temporary file name */
		GetTempFileName (szTempPath, _T("CMD"), 0, szFileName[1]);

		/* we need make sure the LastError msg is zero before calling CreateFile */
		SetLastError(0);

		/* Set current stdout to temporary file */
		hFile[1] = CreateFile (szFileName[1], GENERIC_WRITE, 0, &sa,
				       TRUNCATE_EXISTING, FILE_ATTRIBUTE_TEMPORARY, NULL);

		if (hFile[1] == INVALID_HANDLE_VALUE)
		{
			ConErrResPrintf(STRING_CMD_ERROR2);
			return;
		}

		SetStdHandle (STD_OUTPUT_HANDLE, hFile[1]);

		ExecuteCommand(Cmd->Subcommands);

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

		/* we need make sure the LastError msg is zero before calling CreateFile */
		SetLastError(0);

		/* open new stdin file */
		hFile[0] = CreateFile (szFileName[0], GENERIC_READ, 0, &sa,
		                       OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY, NULL);
		SetStdHandle (STD_INPUT_HANDLE, hFile[0]);

		Cmd = Cmd->Subcommands->Next;
	}

	/* Now set up the end conditions... */
	SetStdHandle(STD_OUTPUT_HANDLE, hOldConOut);

#endif

	/* process final command */
	ExecuteCommand(Cmd);

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
		WARN ("Can't restore STDIN! Is invalid!!\n", out);
	}
#endif  /* buggy implementation */


	if (hOldConIn != INVALID_HANDLE_VALUE)
	{
		HANDLE hIn = GetStdHandle (STD_INPUT_HANDLE);
		SetStdHandle (STD_INPUT_HANDLE, hOldConIn);
		if (hIn == INVALID_HANDLE_VALUE)
		{
			WARN ("Previous STDIN is invalid!!\n");
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
					WARN ("hFile[0] and hIn dont match!!!\n");
				}
			}
		}
	}
#endif /* FEATURE_REDIRECTION */
}

BOOL
ExecuteCommand(PARSED_COMMAND *Cmd)
{
	BOOL bNewBatch = TRUE;
	PARSED_COMMAND *Sub;
	BOOL Success = TRUE;

	if (!PerformRedirection(Cmd->Redirections))
		return FALSE;

	switch (Cmd->Type)
	{
	case C_COMMAND:
		if(bc)
			bNewBatch = FALSE;

		Success = DoCommand(Cmd->CommandLine);

		if(bNewBatch && bc)
			AddBatchRedirection(&Cmd->Redirections);
		break;
	case C_QUIET:
	case C_BLOCK:
	case C_MULTI:
		for (Sub = Cmd->Subcommands; Sub; Sub = Sub->Next)
			Success = ExecuteCommand(Sub);
		break;
	case C_IFFAILURE:
	case C_IFSUCCESS:
		Sub = Cmd->Subcommands;
		Success = ExecuteCommand(Sub);
		if (Success == (Cmd->Type - C_IFFAILURE))
		{
			Sub = Sub->Next;
			Success = ExecuteCommand(Sub);
		}
		break;
	case C_PIPE:
		ExecutePipeline(Cmd);
		break;
	}

	UndoRedirection(Cmd->Redirections, NULL);
	return Success;
}

BOOL
GrowIfNecessary_dbg ( UINT needed, LPTSTR* ret, UINT* retlen, const char *file, int line )
{
	if ( *ret && needed < *retlen )
		return TRUE;
	*retlen = needed;
	if ( *ret )
		cmd_free ( *ret );
#ifdef _DEBUG_MEM
	*ret = (LPTSTR)cmd_alloc_dbg ( *retlen * sizeof(TCHAR), file, line );
#else
	*ret = (LPTSTR)cmd_alloc ( *retlen * sizeof(TCHAR) );
#endif
	if ( !*ret )
		SetLastError ( ERROR_OUTOFMEMORY );
	return *ret != NULL;
}
#define GrowIfNecessary(x, y, z) GrowIfNecessary_dbg(x, y, z, __FILE__, __LINE__)

LPCTSTR
GetEnvVarOrSpecial ( LPCTSTR varName )
{
	static LPTSTR ret = NULL;
	static UINT retlen = 0;
	UINT size;

	size = GetEnvironmentVariable ( varName, ret, retlen );
	if ( size > retlen )
	{
		if ( !GrowIfNecessary ( size, &ret, &retlen ) )
			return NULL;
		size = GetEnvironmentVariable ( varName, ret, retlen );
	}
	if ( size )
		return ret;

	/* env var doesn't exist, look for a "special" one */
	/* %CD% */
	if (_tcsicmp(varName,_T("cd")) ==0)
	{
		size = GetCurrentDirectory ( retlen, ret );
		if ( size > retlen )
		{
			if ( !GrowIfNecessary ( size, &ret, &retlen ) )
				return NULL;
			size = GetCurrentDirectory ( retlen, ret );
		}
		if ( !size )
			return NULL;
		return ret;
	}
	/* %TIME% */
	else if (_tcsicmp(varName,_T("time")) ==0)
	{
		SYSTEMTIME t;
		if ( !GrowIfNecessary ( MAX_PATH, &ret, &retlen ) )
			return NULL;
		GetSystemTime(&t);
		_sntprintf ( ret, retlen, _T("%02d%c%02d%c%02d%c%02d"),
			t.wHour, cTimeSeparator, t.wMinute, cTimeSeparator,
			t.wSecond, cDecimalSeparator, t.wMilliseconds / 10);
		return ret;
	}
	/* %DATE% */
	else if (_tcsicmp(varName,_T("date")) ==0)
	{

		if ( !GrowIfNecessary ( GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL, NULL, 0), &ret, &retlen ) )
			return NULL;

		size = GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL, ret, retlen);

		if ( !size )
			return NULL;
		return ret;
	}

	/* %RANDOM% */
	else if (_tcsicmp(varName,_T("random")) ==0)
	{
		if ( !GrowIfNecessary ( MAX_PATH, &ret, &retlen ) )
			return NULL;
		/* Get random number */
		_itot(rand(),ret,10);
		return ret;
	}

	/* %CMDCMDLINE% */
	else if (_tcsicmp(varName,_T("cmdcmdline")) ==0)
	{
		return GetCommandLine();
	}

	/* %CMDEXTVERSION% */
	else if (_tcsicmp(varName,_T("cmdextversion")) ==0)
	{
		if ( !GrowIfNecessary ( MAX_PATH, &ret, &retlen ) )
			return NULL;
		/* Set version number to 2 */
		_itot(2,ret,10);
		return ret;
	}

	/* %ERRORLEVEL% */
	else if (_tcsicmp(varName,_T("errorlevel")) ==0)
	{
		if ( !GrowIfNecessary ( MAX_PATH, &ret, &retlen ) )
			return NULL;
		_itot(nErrorLevel,ret,10);
		return ret;
	}

	return NULL;
}



LPCTSTR
GetBatchVar ( LPCTSTR varName, UINT* varNameLen )
{
	static LPTSTR ret = NULL;
	static UINT retlen = 0;
	DWORD len;

	*varNameLen = 1;

	switch ( *varName )
	{
	case _T('~'):
		varName++;
		if (_tcsncicmp(varName, _T("dp0"), 3) == 0)
		{
			*varNameLen = 4;
			len = _tcsrchr(bc->BatchFilePath, _T('\\')) + 1 - bc->BatchFilePath;
			if (!GrowIfNecessary(len + 1, &ret, &retlen))
				return NULL;
			memcpy(ret, bc->BatchFilePath, len * sizeof(TCHAR));
			ret[len] = _T('\0');
			return ret;
		}

		*varNameLen = 2;
		if (*varName >= _T('0') && *varName <= _T('9')) {
			LPTSTR arg = FindArg(*varName - _T('0'));

			if (*arg != _T('"'))
				return arg;

			/* Exclude the leading and trailing quotes */
			arg++;
			len = _tcslen(arg);
			if (arg[len - 1] == _T('"'))
				len--;

			if (!GrowIfNecessary(len + 1, &ret, &retlen))
				return NULL;
			memcpy(ret, arg, len * sizeof(TCHAR));
			ret[len] = _T('\0');
			return ret;
		}
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
		return FindArg(*varName - _T('0'));

    case _T('*'):
        //
        // Copy over the raw params(not including the batch file name
        //
        return bc->raw_params;

	case _T('%'):
		return _T("%");
	}
	return NULL;
}

BOOL
SubstituteVars(TCHAR *Src, TCHAR *Dest, TCHAR Delim, BOOL bIsBatch)
{
#define APPEND(From, Length) { \
	if (Dest + (Length) > DestEnd) \
		goto too_long; \
	memcpy(Dest, From, (Length) * sizeof(TCHAR)); \
	Dest += Length; }
#define APPEND1(Char) { \
	if (Dest >= DestEnd) \
		goto too_long; \
	*Dest++ = Char; }

	TCHAR *DestEnd = Dest + CMDLINE_LENGTH - 1;
	const TCHAR *Var;
	int VarLength;
	TCHAR *SubstStart;
	TCHAR EndChr;
	while (*Src)
	{
		if (*Src != Delim)
		{
			APPEND1(*Src++)
			continue;
		}

		Src++;
		if (bIsBatch && Delim == _T('%'))
		{
			UINT NameLen;
			Var = GetBatchVar(Src, &NameLen);
			if (Var != NULL)
			{
				VarLength = _tcslen(Var);
				APPEND(Var, VarLength)
				Src += NameLen;
				continue;
			}
		}

		/* Find the end of the variable name. A colon (:) will usually
		 * end the name and begin the optional modifier, but not if it
		 * is immediately followed by the delimiter (%VAR:%). */
		SubstStart = Src;
		while (*Src != Delim && !(*Src == _T(':') && Src[1] != Delim))
		{
			if (!*Src)
				goto bad_subst;
			Src++;
		}

		EndChr = *Src;
		*Src = _T('\0');
		Var = GetEnvVarOrSpecial(SubstStart);
		*Src++ = EndChr;
		if (Var == NULL)
		{
			/* In a batch file, %NONEXISTENT% "expands" to an empty string */
			if (bIsBatch)
				continue;
			goto bad_subst;
		}
		VarLength = _tcslen(Var);

		if (EndChr == Delim)
		{
			/* %VAR% - use as-is */
			APPEND(Var, VarLength)
		}
		else if (*Src == _T('~'))
		{
			/* %VAR:~[start][,length]% - substring
			 * Negative values are offsets from the end */
			int Start = _tcstol(Src + 1, &Src, 0);
			int End = VarLength;
			if (Start < 0)
				Start += VarLength;
			Start = max(Start, 0);
			Start = min(Start, VarLength);
			if (*Src == _T(','))
			{
				End = _tcstol(Src + 1, &Src, 0);
				End += (End < 0) ? VarLength : Start;
				End = max(End, Start);
				End = min(End, VarLength);
			}
			if (*Src++ != Delim)
				goto bad_subst;
			APPEND(&Var[Start], End - Start);
		}
		else
		{
			/* %VAR:old=new% - replace all occurrences of old with new
			 * %VAR:*old=new% - replace first occurrence only,
			 *                  and remove everything before it */
			TCHAR *Old, *New;
			DWORD OldLength, NewLength;
			BOOL Star = FALSE;
			int LastMatch = 0, i = 0;

			if (*Src == _T('*'))
			{
				Star = TRUE;
				Src++;
			}

			/* the string to replace may contain the delimiter */
			Src = _tcschr(Old = Src, _T('='));
			if (Src == NULL)
				goto bad_subst;
			OldLength = Src++ - Old;
			if (OldLength == 0)
				goto bad_subst;

			Src = _tcschr(New = Src, Delim);
			if (Src == NULL)
				goto bad_subst;
			NewLength = Src++ - New;

			while (i < VarLength)
			{
				if (_tcsnicmp(&Var[i], Old, OldLength) == 0)
				{
					if (!Star)
						APPEND(&Var[LastMatch], i - LastMatch)
					APPEND(New, NewLength)
					i += OldLength;
					LastMatch = i;
					if (Star)
						break;
					continue;
				}
				i++;
			}
			APPEND(&Var[LastMatch], VarLength - LastMatch)
		}
		continue;

	bad_subst:
		Src = SubstStart;
		if (!bIsBatch)
			APPEND1(Delim)
	}
	*Dest = _T('\0');
	return TRUE;
too_long:
	ConOutResPrintf(STRING_ALIAS_ERROR);
	nErrorLevel = 9023;
	return FALSE;
#undef APPEND
#undef APPEND1
}


/*
 * do the prompt/input/process loop
 *
 */

BOOL bNoInteractive;
BOOL bIsBatch;

BOOL
ReadLine (TCHAR *commandline, BOOL bMore)
{
	TCHAR readline[CMDLINE_LENGTH];
	LPTSTR ip;

	/* if no batch input then... */
	if (!(ip = ReadBatchLine()))
	{
		if (bNoInteractive)
		{
			bExit = TRUE;
			return FALSE;
		}

		if (bMore)
		{
			ConOutPrintf(_T("More? "));
		}
		else
		{
			/* JPP 19980807 - if echo off, don't print prompt */
			if (bEcho)
				PrintPrompt();
		}

		ReadCommand (readline, CMDLINE_LENGTH - 1);
		if (CheckCtrlBreak(BREAK_INPUT))
		{
			ConOutPuts(_T("\n"));
			return FALSE;
		}
		ip = readline;
		bIsBatch = FALSE;
	}
	else
	{
		bIsBatch = TRUE;
	}

	if (!SubstituteVars(ip, commandline, _T('%'), bIsBatch))
		return FALSE;

	/* FIXME: !vars! should be substituted later, after parsing. */
	if (!SubstituteVars(commandline, readline, _T('!'), bIsBatch))
		return FALSE;
	_tcscpy(commandline, readline);

	return TRUE;
}

static INT
ProcessInput (BOOL bFlag)
{
	PARSED_COMMAND *Cmd;

	bNoInteractive = bFlag;
	do
	{
		Cmd = ParseCommand(NULL);
		if (!Cmd)
			continue;

		/* JPP 19980807 */
		/* Echo batch file line */
		if (bIsBatch && bEcho && Cmd->Type != C_QUIET)
		{
			PrintPrompt ();
			EchoCommand(Cmd);
			ConOutChar(_T('\n'));
		}

		ExecuteCommand(Cmd);
		if (bEcho && !bIgnoreEcho && (!bIsBatch || Cmd->Type != C_QUIET))
			ConOutChar ('\n');
		FreeCommand(Cmd);
		bIgnoreEcho = FALSE;
	}
	while (!bCanExit || !bExit);

	return nErrorLevel;
}


/*
 * control-break handler.
 */
BOOL WINAPI BreakHandler (DWORD dwCtrlType)
{

	DWORD			dwWritten;
	INPUT_RECORD	rec;
	static BOOL SelfGenerated = FALSE;

 	if ((dwCtrlType != CTRL_C_EVENT) &&
	    (dwCtrlType != CTRL_BREAK_EVENT))
	{
		return FALSE;
	}
	else
	{
		if(SelfGenerated)
		{
			SelfGenerated = FALSE;
			return TRUE;
		}
	}

	if (bChildProcessRunning == TRUE)
	{
		SelfGenerated = TRUE;
		GenerateConsoleCtrlEvent (dwCtrlType, 0);
		return TRUE;
	}


    rec.EventType = KEY_EVENT;
    rec.Event.KeyEvent.bKeyDown = TRUE;
    rec.Event.KeyEvent.wRepeatCount = 1;
    rec.Event.KeyEvent.wVirtualKeyCode = _T('C');
    rec.Event.KeyEvent.wVirtualScanCode = _T('C') - 35;
    rec.Event.KeyEvent.uChar.AsciiChar = _T('C');
    rec.Event.KeyEvent.uChar.UnicodeChar = _T('C');
    rec.Event.KeyEvent.dwControlKeyState = RIGHT_CTRL_PRESSED;

    WriteConsoleInput(
        hIn,
        &rec,
        1,
		&dwWritten);

	bCtrlBreak = TRUE;
	/* FIXME: Handle batch files */

	//ConOutPrintf(_T("^C"));


	return TRUE;
}


VOID AddBreakHandler (VOID)
{
	SetConsoleCtrlHandler ((PHANDLER_ROUTINE)BreakHandler, TRUE);
}


VOID RemoveBreakHandler (VOID)
{
	SetConsoleCtrlHandler ((PHANDLER_ROUTINE)BreakHandler, FALSE);
}


/*
 * show commands and options that are available.
 *
 */
#if 0
static VOID
ShowCommands (VOID)
{
	/* print command list */
	ConOutResPuts(STRING_CMD_HELP1);
	PrintCommandList();

	/* print feature list */
	ConOutResPuts(STRING_CMD_HELP2);

#ifdef FEATURE_ALIASES
	ConOutResPuts(STRING_CMD_HELP3);
#endif
#ifdef FEATURE_HISTORY
	ConOutResPuts(STRING_CMD_HELP4);
#endif
#ifdef FEATURE_UNIX_FILENAME_COMPLETION
	ConOutResPuts(STRING_CMD_HELP5);
#endif
#ifdef FEATURE_DIRECTORY_STACK
	ConOutResPuts(STRING_CMD_HELP6);
#endif
#ifdef FEATURE_REDIRECTION
	ConOutResPuts(STRING_CMD_HELP7);
#endif
	ConOutChar(_T('\n'));
}
#endif

static VOID
ExecuteAutoRunFile (VOID)
{
    TCHAR autorun[MAX_PATH];
	DWORD len = MAX_PATH;
	HKEY hkey;

    if( RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                    _T("SOFTWARE\\Microsoft\\Command Processor"),
                    0, 
                    KEY_READ, 
                    &hkey ) == ERROR_SUCCESS)
    {
	    if(RegQueryValueEx(hkey, 
                           _T("AutoRun"),
                           0, 
                           0, 
                           (LPBYTE)autorun, 
                           &len) == ERROR_SUCCESS)
	    {
		    ParseCommandLine (autorun);
	    }
    }

	RegCloseKey(hkey);
}

/*
 * set up global initializations and process parameters
 *
 * argc - number of parameters to command.com
 * argv - command-line parameters
 *
 */
static VOID
Initialize (int argc, const TCHAR* argv[])
{
	TCHAR commandline[CMDLINE_LENGTH];
	TCHAR ModuleName[_MAX_PATH + 1];
	INT i;
	TCHAR lpBuffer[2];

	//INT len;
	//TCHAR *ptr, *cmdLine;

	/* get version information */
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx (&osvi);

	/* Some people like to run ReactOS cmd.exe on Win98, it helps in the
	 * build process. So don't link implicitly against ntdll.dll, load it
	 * dynamically instead */

	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		/* ntdll is always present on NT */
		NtDllModule = GetModuleHandle(TEXT("ntdll.dll"));
	}
	else
	{
		/* not all 9x versions have a ntdll.dll, try to load it */
		NtDllModule = LoadLibrary(TEXT("ntdll.dll"));
	}

	if (NtDllModule != NULL)
	{
		NtQueryInformationProcessPtr = (NtQueryInformationProcessProc)GetProcAddress(NtDllModule, "NtQueryInformationProcess");
		NtReadVirtualMemoryPtr = (NtReadVirtualMemoryProc)GetProcAddress(NtDllModule, "NtReadVirtualMemory");
	}


	TRACE ("[command args:\n");
	for (i = 0; i < argc; i++)
	{
		TRACE ("%d. %s\n", i, debugstr_aw(argv[i]));
	}
	TRACE ("]\n");

	InitLocale ();

	/* get default input and output console handles */
	hOut = GetStdHandle (STD_OUTPUT_HANDLE);
	hIn  = GetStdHandle (STD_INPUT_HANDLE);

	/* Set EnvironmentVariable PROMPT if it does not exists any env value.
	   for you can change the EnvirommentVariable for prompt before cmd start
	   this patch are not 100% right, if it does not exists a PROMPT value cmd should use
	   $P$G as defualt not set EnvirommentVariable PROMPT to $P$G if it does not exists */
	if (GetEnvironmentVariable(_T("PROMPT"),lpBuffer, sizeof(lpBuffer) / sizeof(lpBuffer[0])) == 0)
	    SetEnvironmentVariable (_T("PROMPT"), _T("$P$G"));


	if (argc >= 2 && !_tcsncmp (argv[1], _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_CMD_HELP8);
		cmd_exit(0);
	}
	SetConsoleMode (hIn, ENABLE_PROCESSED_INPUT);

#ifdef INCLUDE_CMD_CHDIR
	InitLastPath ();
#endif

	if (argc >= 2)
	{
		for (i = 1; i < argc; i++)
		{
			if (!_tcsicmp (argv[i], _T("/p")))
			{
				if (!IsExistingFile (_T("\\autoexec.bat")))
				{
#ifdef INCLUDE_CMD_DATE
					cmd_date (_T(""));
#endif
#ifdef INCLUDE_CMD_TIME
					cmd_time (_T(""));
#endif
				}
				else
				{
					ParseCommandLine (_T("\\autoexec.bat"));
				}
				bCanExit = FALSE;
			}
			else if (!_tcsicmp (argv[i], _T("/c")))
			{
				/* This just runs a program and exits */
				++i;
				if (i < argc)
				{
					_tcscpy (commandline, argv[i]);
					while (++i < argc)
					{
						_tcscat (commandline, _T(" "));
						_tcscat (commandline, argv[i]);
					}

					ParseCommandLine(commandline);
					cmd_exit (ProcessInput (TRUE));
				}
				else
				{
					cmd_exit (0);
				}
			}
			else if (!_tcsicmp (argv[i], _T("/k")))
			{
				/* This just runs a program and remains */
				++i;
				if (i < argc)
				{
					_tcscpy (commandline, _T("\""));
					_tcscat (commandline, argv[i]);
					_tcscat (commandline, _T("\""));
					while (++i < argc)
					{
						_tcscat (commandline, _T(" "));
						_tcscat (commandline, argv[i]);
					}
					ParseCommandLine(commandline);
				}
			}
#ifdef INCLUDE_CMD_COLOR
			else if (!_tcsnicmp (argv[i], _T("/t:"), 3))
			{
				/* process /t (color) argument */
				wDefColor = (WORD)_tcstoul (&argv[i][3], NULL, 16);
				wColor = wDefColor;
				SetScreenColor (wColor, TRUE);
			}
#endif
		}
	}
    else
    {
        /* Display a simple version string */
        ConOutPrintf(_T("ReactOS Operating System [Version %s-%s]\n"), 
            _T(KERNEL_RELEASE_STR),
            _T(KERNEL_VERSION_BUILD_STR));

	    ConOutPuts (_T("(C) Copyright 1998-") _T(COPYRIGHT_YEAR) _T(" ReactOS Team.\n"));
    }

    ExecuteAutoRunFile ();

#ifdef FEATURE_DIR_STACK
	/* initialize directory stack */
	InitDirectoryStack ();
#endif


#ifdef FEATURE_HISTORY
	/*initialize history*/
	InitHistory();
#endif

	/* Set COMSPEC environment variable */
	if (0 != GetModuleFileName (NULL, ModuleName, _MAX_PATH + 1))
	{
		ModuleName[_MAX_PATH] = _T('\0');
		SetEnvironmentVariable (_T("COMSPEC"), ModuleName);
	}

	/* add ctrl break handler */
	AddBreakHandler ();
}


static VOID Cleanup (int argc, const TCHAR *argv[])
{
	/* run cmdexit.bat */
	if (IsExistingFile (_T("cmdexit.bat")))
	{
		ConErrResPuts(STRING_CMD_ERROR5);

		ParseCommandLine (_T("cmdexit.bat"));
	}
	else if (IsExistingFile (_T("\\cmdexit.bat")))
	{
		ConErrResPuts (STRING_CMD_ERROR5);
		ParseCommandLine (_T("\\cmdexit.bat"));
	}

#ifdef FEATURE_DIECTORY_STACK
	/* destroy directory stack */
	DestroyDirectoryStack ();
#endif

#ifdef INCLUDE_CMD_CHDIR
	FreeLastPath ();
#endif

#ifdef FEATURE_HISTORY
	CleanHistory();
#endif


	/* remove ctrl break handler */
	RemoveBreakHandler ();
	SetConsoleMode( GetStdHandle( STD_INPUT_HANDLE ),
			ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT );

	if (NtDllModule != NULL)
	{
		FreeLibrary(NtDllModule);
	}
}

/*
 * main function
 */
int cmd_main (int argc, const TCHAR *argv[])
{
	TCHAR startPath[MAX_PATH];
	CONSOLE_SCREEN_BUFFER_INFO Info;
	INT nExitCode;

	GetCurrentDirectory(MAX_PATH,startPath);
	_tchdir(startPath);

	SetFileApisToOEM();
	InputCodePage= 0;
	OutputCodePage = 0;

	hConsole = CreateFile(_T("CONOUT$"), GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, 0, NULL);
	if (GetConsoleScreenBufferInfo(hConsole, &Info) == FALSE)
	{
		ConErrFormatMessage(GetLastError());
		return(1);
	}
	wColor = Info.wAttributes;
	wDefColor = wColor;

	InputCodePage= GetConsoleCP();
	OutputCodePage = GetConsoleOutputCP();
	CMD_ModuleHandle = GetModuleHandle(NULL);

	/* check switches on command-line */
	Initialize(argc, argv);

	/* call prompt routine */
	nExitCode = ProcessInput(FALSE);

	/* do the cleanup */
	Cleanup(argc, argv);

	cmd_exit(nExitCode);
	return(nExitCode);
}

/* EOF */
