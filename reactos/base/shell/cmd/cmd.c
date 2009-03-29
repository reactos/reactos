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
BOOL bUnicodeOutput = FALSE;
BOOL bDisableBatchEcho = FALSE;
BOOL bDelayedExpansion = FALSE;
DWORD dwChildProcessId = 0;
OSVERSIONINFO osvi;
HANDLE hIn;
HANDLE hOut;
HANDLE hConsole;
LPTSTR lpOriginalEnvironment;
HANDLE CMD_ModuleHandle;

static NtQueryInformationProcessProc NtQueryInformationProcessPtr = NULL;
static NtReadVirtualMemoryProc       NtReadVirtualMemoryPtr = NULL;

#ifdef INCLUDE_CMD_COLOR
WORD wDefColor;           /* default color */
#endif

/*
 * convert
 *
 * insert commas into a number
 */
INT
ConvertULargeInteger(ULONGLONG num, LPTSTR des, INT len, BOOL bPutSeperator)
{
	TCHAR temp[32];
	UINT  n, iTarget;

	if (len <= 1)
		return 0;

	n = 0;
	iTarget = nNumberGroups;
	if (!nNumberGroups)
		bPutSeperator = FALSE;

	do
	{
		if (iTarget == n && bPutSeperator)
		{
			iTarget += nNumberGroups + 1;
			temp[31 - n++] = cThousandSeparator;
		}
		temp[31 - n++] = (TCHAR)(num % 10) + _T('0');
		num /= 10;
	} while (num > 0);
	if (n > len-1)
		n = len-1;

	memcpy(des, temp + 32 - n, n * sizeof(TCHAR));
	des[n] = _T('\0');

	return n;
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
#define SHELLEXECUTETEXT   	"ShellExecuteExW"
#else
#define SHELLEXECUTETEXT   	"ShellExecuteExA"
#endif

typedef BOOL (WINAPI *MYEX)(LPSHELLEXECUTEINFO lpExecInfo);

HANDLE RunFile(DWORD flags, LPTSTR filename, LPTSTR params,
               LPTSTR directory, INT show)
{
	SHELLEXECUTEINFO sei;
	HMODULE     hShell32;
	MYEX        hShExt;
	BOOL        ret;

	TRACE ("RunFile(%s)\n", debugstr_aw(filename));
	hShell32 = LoadLibrary(_T("SHELL32.DLL"));
	if (!hShell32)
	{
		WARN ("RunFile: couldn't load SHELL32.DLL!\n");
		return NULL;
	}

	hShExt = (MYEX)(FARPROC)GetProcAddress(hShell32, SHELLEXECUTETEXT);
	if (!hShExt)
	{
		WARN ("RunFile: couldn't find ShellExecuteExA/W in SHELL32.DLL!\n");
		FreeLibrary(hShell32);
		return NULL;
	}

	TRACE ("RunFile: ShellExecuteExA/W is at %x\n", hShExt);

	memset(&sei, 0, sizeof sei);
	sei.cbSize = sizeof sei;
	sei.fMask = flags;
	sei.lpFile = filename;
	sei.lpParameters = params;
	sei.lpDirectory = directory;
	sei.nShow = show;
	ret = hShExt(&sei);

	TRACE ("RunFile: ShellExecuteExA/W returned 0x%p\n", ret);

	FreeLibrary(hShell32);
	return ret ? sei.hProcess : NULL;
}



/*
 * This command (in first) was not found in the command table
 *
 * Full  - buffer to hold whole command line
 * First - first word on command line
 * Rest  - rest of command line
 */

static BOOL
Execute (LPTSTR Full, LPTSTR First, LPTSTR Rest, PARSED_COMMAND *Cmd)
{
	TCHAR szFullName[MAX_PATH];
	TCHAR *first, *rest, *dot;
	TCHAR szWindowTitle[MAX_PATH];
	DWORD dwExitCode = 0;
	TCHAR *FirstEnd;

	TRACE ("Execute: \'%s\' \'%s\'\n", debugstr_aw(First), debugstr_aw(Rest));

	/* Though it was already parsed once, we have a different set of rules
	   for parsing before we pass to CreateProccess */
	if (First[0] == _T('/') || (First[0] && First[1] == _T(':')))
	{
		/* Use the entire first word as the program name (no change) */
		FirstEnd = First + _tcslen(First);
	}
	else
	{
		/* If present in the first word, spaces and ,;=/ end the program
		 * name and become the beginning of its parameters. */
		BOOL bInside = FALSE;
		for (FirstEnd = First; *FirstEnd; FirstEnd++)
		{
			if (!bInside && (_istspace(*FirstEnd) || _tcschr(_T(",;=/"), *FirstEnd)))
				break;
			bInside ^= *FirstEnd == _T('"');
		}
	}

	/* Copy the new first/rest into the buffer */
	first = Full;
	rest = &Full[FirstEnd - First + 1];
	_tcscpy(rest, FirstEnd);
	_tcscat(rest, Rest);
	*FirstEnd = _T('\0');
	_tcscpy(first, First);

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
		return working;
	}

	/* get the PATH environment variable and parse it */
	/* search the PATH environment variable for the binary */
	StripQuotes(First);
	if (!SearchForExecutable(First, szFullName))
	{
		error_bad_command(first);
		return FALSE;
	}

	GetConsoleTitle (szWindowTitle, MAX_PATH);

	/* check if this is a .BAT or .CMD file */
	dot = _tcsrchr (szFullName, _T('.'));
	if (dot && (!_tcsicmp (dot, _T(".bat")) || !_tcsicmp (dot, _T(".cmd"))))
	{
		while (*rest == _T(' '))
			rest++;
		TRACE ("[BATCH: %s %s]\n", debugstr_aw(szFullName), debugstr_aw(rest));
		Batch (szFullName, first, rest, Cmd);
	}
	else
	{
		/* exec the program */
		PROCESS_INFORMATION prci;
		STARTUPINFO stui;

		/* build command line for CreateProcess(): first + " " + rest */
		if (*rest)
			rest[-1] = _T(' ');

		TRACE ("[EXEC: %s]\n", debugstr_aw(Full));

		/* fill startup info */
		memset (&stui, 0, sizeof (STARTUPINFO));
		stui.cb = sizeof (STARTUPINFO);
		stui.dwFlags = STARTF_USESHOWWINDOW;
		stui.wShowWindow = SW_SHOWDEFAULT;

		// return console to standard mode
		SetConsoleMode (GetStdHandle(STD_INPUT_HANDLE),
		                ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT );

		if (CreateProcess (szFullName,
		                   Full,
		                   NULL,
		                   NULL,
		                   TRUE,
		                   0,			/* CREATE_NEW_PROCESS_GROUP */
		                   NULL,
		                   NULL,
		                   &stui,
		                   &prci))

		{
			CloseHandle(prci.hThread);
		}
		else
		{
			// See if we can run this with ShellExecute() ie myfile.xls
			prci.hProcess = RunFile(SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE,
			                        szFullName,
			                        rest,
			                        NULL,
			                        SW_SHOWNORMAL);
		}

		if (prci.hProcess != NULL)
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
			CloseHandle (prci.hProcess);
		}
		else
		{
			TRACE ("[ShellExecute failed!: %s]\n", debugstr_aw(Full));
			error_bad_command (first);
			nErrorLevel = 1;
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

	return nErrorLevel == 0;
}


/*
 * look through the internal commands and determine whether or not this
 * command is one of them.  If it is, call the command.  If not, call
 * execute to run it as an external program.
 *
 * first - first word on command line
 * rest  - rest of command line
 */

BOOL
DoCommand(LPTSTR first, LPTSTR rest, PARSED_COMMAND *Cmd)
{
	TCHAR com[_tcslen(first) + _tcslen(rest) + 2];  /* full command line */
	TCHAR *cp;
	LPTSTR param;   /* pointer to command's parameters */
	INT cl;
	LPCOMMAND cmdptr;
	BOOL nointernal = FALSE;

	TRACE ("DoCommand: (\'%s\' \'%s\')\n", debugstr_aw(first), debugstr_aw(rest));

	/* If present in the first word, these characters end the name of an
	 * internal command and become the beginning of its parameters. */
	cp = first + _tcscspn(first, _T("\t +,/;=[]"));

	for (cl = 0; cl < (cp - first); cl++)
	{
		/* These characters do it too, but if one of them is present,
		 * then we check to see if the word is a file name and skip
		 * checking for internal commands if so.
		 * This allows running programs with names like "echo.exe" */
		if (_tcschr(_T(".:\\"), first[cl]))
		{
			TCHAR tmp = *cp;
			*cp = _T('\0');
			nointernal = IsExistingFile(first);
			*cp = tmp;
			break;
		}
	}

	/* Scan internal command table */
	for (cmdptr = cmds; !nointernal && cmdptr->name; cmdptr++)
	{
		if (!_tcsnicmp(first, cmdptr->name, cl) && cmdptr->name[cl] == _T('\0'))
		{
			_tcscpy(com, first);
			_tcscat(com, rest);
			param = &com[cl];

			/* Skip over whitespace to rest of line, exclude 'echo' command */
			if (_tcsicmp(cmdptr->name, _T("echo")) != 0)
				while (_istspace(*param))
					param++;
			return !cmdptr->func(param);
		}
	}

	return Execute(com, first, rest, Cmd);
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

/* Execute a command without waiting for it to finish. If it's an internal
 * command or batch file, we must create a new cmd.exe process to handle it.
 * TODO: For now, this just always creates a cmd.exe process.
 *       This works, but is inefficient for running external programs,
 *       which could just be run directly. */
static HANDLE
ExecuteAsync(PARSED_COMMAND *Cmd)
{
	TCHAR CmdPath[MAX_PATH];
	TCHAR CmdParams[CMDLINE_LENGTH], *ParamsEnd;
	STARTUPINFO stui;
	PROCESS_INFORMATION prci;

	/* Get the path to cmd.exe */
	GetModuleFileName(NULL, CmdPath, MAX_PATH);

	/* Build the parameter string to pass to cmd.exe */
	ParamsEnd = _stpcpy(CmdParams, _T("/S/D/C\""));
	ParamsEnd = Unparse(Cmd, ParamsEnd, &CmdParams[CMDLINE_LENGTH - 2]);
	if (!ParamsEnd)
	{
		error_out_of_memory();
		return NULL;
	}
	_tcscpy(ParamsEnd, _T("\""));

	memset(&stui, 0, sizeof stui);
	stui.cb = sizeof(STARTUPINFO);
	if (!CreateProcess(CmdPath, CmdParams, NULL, NULL, TRUE, 0,
	                   NULL, NULL, &stui, &prci))
	{
		ErrorMessage(GetLastError(), NULL);
		return NULL;
	}

	CloseHandle(prci.hThread);
	return prci.hProcess;
}

static VOID
ExecutePipeline(PARSED_COMMAND *Cmd)
{
#ifdef FEATURE_REDIRECTION
	HANDLE hInput = NULL;
	HANDLE hOldConIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOldConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hProcess[MAXIMUM_WAIT_OBJECTS];
	INT nProcesses = 0;
	DWORD dwExitCode;

	/* Do all but the last pipe command */
	do
	{
		HANDLE hPipeRead, hPipeWrite;
		if (nProcesses > (MAXIMUM_WAIT_OBJECTS - 2))
		{
			error_too_many_parameters(_T("|"));
			goto failed;
		}

		/* Create the pipe that this process will write into.
		 * Make the handles non-inheritable initially, because this
		 * process shouldn't inherit the reading handle. */
		if (!CreatePipe(&hPipeRead, &hPipeWrite, NULL, 0))
		{
			error_no_pipe();
			goto failed;
		}

		/* The writing side of the pipe is STDOUT for this process */
		SetHandleInformation(hPipeWrite, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
		SetStdHandle(STD_OUTPUT_HANDLE, hPipeWrite);

		/* Execute it (error check is done later for easier cleanup) */
		hProcess[nProcesses] = ExecuteAsync(Cmd->Subcommands);
		CloseHandle(hPipeWrite);
		if (hInput)
			CloseHandle(hInput);

		/* The reading side of the pipe will be STDIN for the next process */
		SetHandleInformation(hPipeRead, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
		SetStdHandle(STD_INPUT_HANDLE, hPipeRead);
		hInput = hPipeRead;

		if (!hProcess[nProcesses])
			goto failed;
		nProcesses++;

		Cmd = Cmd->Subcommands->Next;
	} while (Cmd->Type == C_PIPE);

	/* The last process uses the original STDOUT */
	SetStdHandle(STD_OUTPUT_HANDLE, hOldConOut);
	hProcess[nProcesses] = ExecuteAsync(Cmd);
	if (!hProcess[nProcesses])
		goto failed;
	nProcesses++;
	CloseHandle(hInput);
	SetStdHandle(STD_INPUT_HANDLE, hOldConIn);

	/* Wait for all processes to complete */
	bChildProcessRunning = TRUE;
	WaitForMultipleObjects(nProcesses, hProcess, TRUE, INFINITE);
	bChildProcessRunning = FALSE;

	/* Use the exit code of the last process in the pipeline */
	GetExitCodeProcess(hProcess[nProcesses - 1], &dwExitCode);
	nErrorLevel = (INT)dwExitCode;

	while (--nProcesses >= 0)
		CloseHandle(hProcess[nProcesses]);
	return;

failed:
	if (hInput)
		CloseHandle(hInput);
	while (--nProcesses >= 0)
	{
		TerminateProcess(hProcess[nProcesses], 0);
		CloseHandle(hProcess[nProcesses]);
	}
	SetStdHandle(STD_INPUT_HANDLE, hOldConIn);
	SetStdHandle(STD_OUTPUT_HANDLE, hOldConOut);
#endif
}

BOOL
ExecuteCommand(PARSED_COMMAND *Cmd)
{
	PARSED_COMMAND *Sub;
	LPTSTR First, Rest;
	BOOL Success = TRUE;

	if (!PerformRedirection(Cmd->Redirections))
		return FALSE;

	switch (Cmd->Type)
	{
	case C_COMMAND:
		Success = FALSE;
		First = DoDelayedExpansion(Cmd->Command.First);
		if (First)
		{
			Rest = DoDelayedExpansion(Cmd->Command.Rest);
			if (Rest)
			{
				Success = DoCommand(First, Rest, Cmd);
				cmd_free(Rest);
			}
			cmd_free(First);
		}
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
	case C_IF:
		Success = ExecuteIf(Cmd);
		break;
	case C_FOR:
		Success = ExecuteFor(Cmd);
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

LPTSTR
GetEnvVar(LPCTSTR varName)
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
	return NULL;
}

LPCTSTR
GetEnvVarOrSpecial(LPCTSTR varName)
{
	static TCHAR ret[MAX_PATH];

	LPTSTR var = GetEnvVar(varName);
	if (var)
		return var;

	/* env var doesn't exist, look for a "special" one */
	/* %CD% */
	if (_tcsicmp(varName,_T("cd")) ==0)
	{
		GetCurrentDirectory(MAX_PATH, ret);
		return ret;
	}
	/* %TIME% */
	else if (_tcsicmp(varName,_T("time")) ==0)
	{
		return GetTimeString();
	}
	/* %DATE% */
	else if (_tcsicmp(varName,_T("date")) ==0)
	{
		return GetDateString();
	}

	/* %RANDOM% */
	else if (_tcsicmp(varName,_T("random")) ==0)
	{
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
		/* Set version number to 2 */
		_itot(2,ret,10);
		return ret;
	}

	/* %ERRORLEVEL% */
	else if (_tcsicmp(varName,_T("errorlevel")) ==0)
	{
		_itot(nErrorLevel,ret,10);
		return ret;
	}

	return NULL;
}

/* Handle the %~var syntax */
static LPTSTR
GetEnhancedVar(TCHAR **pFormat, LPTSTR (*GetVar)(TCHAR, BOOL *))
{
	static const TCHAR ModifierTable[] = _T("dpnxfsatz");
	enum {
		M_DRIVE = 1,   /* D: drive letter */
		M_PATH  = 2,   /* P: path */
		M_NAME  = 4,   /* N: filename */
		M_EXT   = 8,   /* X: extension */
		M_FULL  = 16,  /* F: full path (drive+path+name+ext) */
		M_SHORT = 32,  /* S: full path (drive+path+name+ext), use short names */
		M_ATTR  = 64,  /* A: attributes */
		M_TIME  = 128, /* T: modification time */
		M_SIZE  = 256, /* Z: file size */
	} Modifiers = 0;

	TCHAR *Format, *FormatEnd;
	TCHAR *PathVarName = NULL;
	LPTSTR Variable;
	TCHAR *VarEnd;
	BOOL VariableIsParam0;
	TCHAR FullPath[MAX_PATH];
	TCHAR FixedPath[MAX_PATH], *Filename, *Extension;
	HANDLE hFind;
	WIN32_FIND_DATA w32fd;
	TCHAR *In, *Out;

	static TCHAR Result[CMDLINE_LENGTH];

	/* There is ambiguity between modifier characters and FOR variables;
	 * the rule that cmd uses is to pick the longest possible match.
	 * For example, if there is a %n variable, then out of %~anxnd,
	 * %~anxn will be substituted rather than just %~an. */

	/* First, go through as many modifier characters as possible */
	FormatEnd = Format = *pFormat;
	while (*FormatEnd && _tcschr(ModifierTable, _totlower(*FormatEnd)))
		FormatEnd++;

	if (*FormatEnd == _T('$'))
	{
		/* $PATH: syntax */
		PathVarName = FormatEnd + 1;
		FormatEnd = _tcschr(PathVarName, _T(':'));
		if (!FormatEnd)
			return NULL;

		/* Must be immediately followed by the variable */
		Variable = GetVar(*++FormatEnd, &VariableIsParam0);
		if (!Variable)
			return NULL;
	}
	else
	{
		/* Backtrack if necessary to get a variable name match */
		while (!(Variable = GetVar(*FormatEnd, &VariableIsParam0)))
		{
			if (FormatEnd == Format)
				return NULL;
			FormatEnd--;
		}
	}

	for (; Format < FormatEnd && *Format != _T('$'); Format++)
		Modifiers |= 1 << (_tcschr(ModifierTable, _totlower(*Format)) - ModifierTable);

	*pFormat = FormatEnd + 1;

	/* Exclude the leading and trailing quotes */
	VarEnd = &Variable[_tcslen(Variable)];
	if (*Variable == _T('"'))
	{
		Variable++;
		if (VarEnd > Variable && VarEnd[-1] == _T('"'))
			VarEnd--;
	}

	if ((char *)VarEnd - (char *)Variable >= sizeof Result)
		return _T("");
	memcpy(Result, Variable, (char *)VarEnd - (char *)Variable);
	Result[VarEnd - Variable] = _T('\0');

	if (PathVarName)
	{
		/* $PATH: syntax - search the directories listed in the
		 * specified environment variable for the file */
		LPTSTR PathVar;
		FormatEnd[-1] = _T('\0');
		PathVar = GetEnvVar(PathVarName);
		FormatEnd[-1] = _T(':');
		if (!PathVar ||
		    !SearchPath(PathVar, Result, NULL, MAX_PATH, FullPath, NULL))
		{
			return _T("");
		}
	}
	else if (Modifiers == 0)
	{
		/* For plain %~var with no modifiers, just return the variable without quotes */
		return Result;
	}
	else if (VariableIsParam0)
	{
		/* Special case: If the variable is %0 and modifier characters are present,
		 * use the batch file's path (which includes the .bat/.cmd extension)
		 * rather than the actual %0 variable (which might not). */
		_tcscpy(FullPath, bc->BatchFilePath);
	}
	else
	{
		/* Convert the variable, now without quotes, to a full path */
		if (!GetFullPathName(Result, MAX_PATH, FullPath, NULL))
			return _T("");
	}

	/* Next step is to change the path to fix letter case (e.g.
	 * C:\ReAcToS -> C:\ReactOS) and, if requested with the S modifier,
	 * replace long filenames with short. */

	In = FullPath;
	Out = FixedPath;

	/* Copy drive letter */
	*Out++ = *In++;
	*Out++ = *In++;
	*Out++ = *In++;
	/* Loop over each \-separated component in the path */
	do {
		TCHAR *Next = _tcschr(In, _T('\\'));
		if (Next)
			*Next++ = _T('\0');
		/* Use FindFirstFile to get the correct name */
		if (Out + _tcslen(In) + 1 >= &FixedPath[MAX_PATH])
			return _T("");
		_tcscpy(Out, In);
		hFind = FindFirstFile(FixedPath, &w32fd);
		/* If it doesn't exist, just leave the name as it was given */
		if (hFind != INVALID_HANDLE_VALUE)
		{
			LPTSTR FixedComponent = w32fd.cFileName;
			if (*w32fd.cAlternateFileName &&
			    ((Modifiers & M_SHORT) || !_tcsicmp(In, w32fd.cAlternateFileName)))
			{
				FixedComponent = w32fd.cAlternateFileName;
			}
			FindClose(hFind);

			if (Out + _tcslen(FixedComponent) + 1 >= &FixedPath[MAX_PATH])
				return _T("");
			_tcscpy(Out, FixedComponent);
		}
		Filename = Out;
		Out += _tcslen(Out);
		*Out++ = _T('\\');

		In = Next;
	} while (In != NULL);
	Out[-1] = _T('\0');

	/* Build the result string. Start with attributes, modification time, and
	 * file size. If the file didn't exist, these fields will all be empty. */
	Out = Result;
	if (hFind != INVALID_HANDLE_VALUE)
	{
		if (Modifiers & M_ATTR)
		{
			static const struct {
				TCHAR Character;
				WORD  Value;
			} *Attrib, Table[] = {
				{ _T('d'), FILE_ATTRIBUTE_DIRECTORY },
				{ _T('r'), FILE_ATTRIBUTE_READONLY },
				{ _T('a'), FILE_ATTRIBUTE_ARCHIVE },
				{ _T('h'), FILE_ATTRIBUTE_HIDDEN },
				{ _T('s'), FILE_ATTRIBUTE_SYSTEM },
				{ _T('c'), FILE_ATTRIBUTE_COMPRESSED },
				{ _T('o'), FILE_ATTRIBUTE_OFFLINE },
				{ _T('t'), FILE_ATTRIBUTE_TEMPORARY },
				{ _T('l'), FILE_ATTRIBUTE_REPARSE_POINT },
			};
			for (Attrib = Table; Attrib != &Table[9]; Attrib++)
			{
				*Out++ = w32fd.dwFileAttributes & Attrib->Value
				         ? Attrib->Character
				         : _T('-');
			}
			*Out++ = _T(' ');
		}
		if (Modifiers & M_TIME)
		{
			FILETIME ft;
			SYSTEMTIME st;
			FileTimeToLocalFileTime(&w32fd.ftLastWriteTime, &ft);
			FileTimeToSystemTime(&ft, &st);

			Out += FormatDate(Out, &st, TRUE);
			*Out++ = _T(' ');
			Out += FormatTime(Out, &st);
			*Out++ = _T(' ');
		}
		if (Modifiers & M_SIZE)
		{
			ULARGE_INTEGER Size;
			Size.LowPart = w32fd.nFileSizeLow;
			Size.HighPart = w32fd.nFileSizeHigh;
			Out += _stprintf(Out, _T("%I64u "), Size.QuadPart);
		}
	}

	/* When using the path-searching syntax or the S modifier,
	 * at least part of the file path is always included.
	 * If none of the DPNX modifiers are present, include the full path */
	if (PathVarName || (Modifiers & M_SHORT))
		if ((Modifiers & (M_DRIVE | M_PATH | M_NAME | M_EXT)) == 0)
			Modifiers |= M_FULL;

	/* Now add the requested parts of the name.
	 * With the F modifier, add all parts to form the full path. */
	Extension = _tcsrchr(Filename, _T('.'));
	if (Modifiers & (M_DRIVE | M_FULL))
	{
		*Out++ = FixedPath[0];
		*Out++ = FixedPath[1];
	}
	if (Modifiers & (M_PATH | M_FULL))
	{
		memcpy(Out, &FixedPath[2], (char *)Filename - (char *)&FixedPath[2]);
		Out += Filename - &FixedPath[2];
	}
	if (Modifiers & (M_NAME | M_FULL))
	{
		while (*Filename && Filename != Extension)
			*Out++ = *Filename++;
	}
	if (Modifiers & (M_EXT | M_FULL))
	{
		if (Extension)
			Out = _stpcpy(Out, Extension);
	}

	/* Trim trailing space which otherwise would appear as a
	 * result of using the A/T/Z modifiers but no others. */
	while (Out != &Result[0] && Out[-1] == _T(' '))
		Out--;
	*Out = _T('\0');

	return Result;
}

LPCTSTR
GetBatchVar(TCHAR *varName, UINT *varNameLen)
{
	LPCTSTR ret;
	TCHAR *varNameEnd;
	BOOL dummy;

	*varNameLen = 1;

	switch ( *varName )
	{
	case _T('~'):
		varNameEnd = varName + 1;
		ret = GetEnhancedVar(&varNameEnd, FindArg);
		if (!ret)
		{
			error_syntax(varName);
			return NULL;
		}
		*varNameLen = varNameEnd - varName;
		return ret;
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
		return FindArg(*varName, &dummy);

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
SubstituteVars(TCHAR *Src, TCHAR *Dest, TCHAR Delim)
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
		if (bc && Delim == _T('%'))
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
			if (bc)
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
		if (!bc)
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

/* Search the list of FOR contexts for a variable */
static LPTSTR FindForVar(TCHAR Var, BOOL *IsParam0)
{
	FOR_CONTEXT *Ctx;
	*IsParam0 = FALSE;
	for (Ctx = fc; Ctx != NULL; Ctx = Ctx->prev)
		if ((UINT)(Var - Ctx->firstvar) < Ctx->varcount)
			return Ctx->values[Var - Ctx->firstvar];
	return NULL;
}

BOOL
SubstituteForVars(TCHAR *Src, TCHAR *Dest)
{
	TCHAR *DestEnd = &Dest[CMDLINE_LENGTH - 1];
	while (*Src)
	{
		if (Src[0] == _T('%'))
		{
			BOOL Dummy;
			LPTSTR End = &Src[2];
			LPTSTR Value = NULL;

			if (Src[1] == _T('~'))
				Value = GetEnhancedVar(&End, FindForVar);

			if (!Value)
				Value = FindForVar(Src[1], &Dummy);

			if (Value)
			{
				if (Dest + _tcslen(Value) > DestEnd)
					return FALSE;
				Dest = _stpcpy(Dest, Value);
				Src = End;
				continue;
			}
		}
		/* Not a variable; just copy the character */
		if (Dest >= DestEnd)
			return FALSE;
		*Dest++ = *Src++;
	}
	*Dest = _T('\0');
	return TRUE;
}

LPTSTR
DoDelayedExpansion(LPTSTR Line)
{
	TCHAR Buf1[CMDLINE_LENGTH];
	TCHAR Buf2[CMDLINE_LENGTH];

	/* First, substitute FOR variables */
	if (!SubstituteForVars(Line, Buf1))
		return NULL;

	if (!bDelayedExpansion || !_tcschr(Buf1, _T('!')))
		return cmd_dup(Buf1);

	/* FIXME: Delayed substitutions actually aren't quite the same as
	 * immediate substitutions. In particular, it's possible to escape
	 * the exclamation point using ^. */
	if (!SubstituteVars(Buf1, Buf2, _T('!')))
		return NULL;
	return cmd_dup(Buf2);
}


/*
 * do the prompt/input/process loop
 *
 */

BOOL
ReadLine (TCHAR *commandline, BOOL bMore)
{
	TCHAR readline[CMDLINE_LENGTH];
	LPTSTR ip;

	/* if no batch input then... */
	if (bc == NULL)
	{
		if (bMore)
		{
			ConOutResPrintf(STRING_MORE);
		}
		else
		{
			/* JPP 19980807 - if echo off, don't print prompt */
			if (bEcho)
			{
				if (!bIgnoreEcho)
					ConOutChar('\n');
				PrintPrompt();
			}
		}

		ReadCommand (readline, CMDLINE_LENGTH - 1);
		if (CheckCtrlBreak(BREAK_INPUT))
		{
			ConOutPuts(_T("\n"));
			return FALSE;
		}
		ip = readline;
	}
	else
	{
		ip = ReadBatchLine();
		if (!ip)
			return FALSE;
	}

	return SubstituteVars(ip, commandline, _T('%'));
}

static INT
ProcessInput()
{
	PARSED_COMMAND *Cmd;

	while (!bCanExit || !bExit)
	{
		Cmd = ParseCommand(NULL);
		if (!Cmd)
			continue;

		ExecuteCommand(Cmd);
		FreeCommand(Cmd);
	}

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
			if (*autorun)
				ParseCommandLine(autorun);
	    }
    }

	RegCloseKey(hkey);
}

/* Get the command that comes after a /C or /K switch */
static VOID
GetCmdLineCommand(TCHAR *commandline, TCHAR *ptr, BOOL AlwaysStrip)
{
	TCHAR *LastQuote;

	while (_istspace(*ptr))
		ptr++;

	/* Remove leading quote, find final quote */
	if (*ptr == _T('"') &&
	    (LastQuote = _tcsrchr(++ptr, _T('"'))) != NULL)
	{
		TCHAR *Space;
		/* Under certain circumstances, all quotes are preserved.
		 * CMD /? documents these conditions as follows:
		 *  1. No /S switch
		 *  2. Exactly two quotes
		 *  3. No "special characters" between the quotes
		 *     (CMD /? says &<>()@^| but parentheses did not
		 *     trigger this rule when I tested them.)
		 *  4. Whitespace exists between the quotes
		 *  5. Enclosed string is an executable filename
		 */
		*LastQuote = _T('\0');
		for (Space = ptr + 1; Space < LastQuote; Space++)
		{
			if (_istspace(*Space))                         /* Rule 4 */
			{
				if (!AlwaysStrip &&                        /* Rule 1 */
				    !_tcspbrk(ptr, _T("\"&<>@^|")) &&      /* Rules 2, 3 */
				    SearchForExecutable(ptr, commandline)) /* Rule 5 */
				{
					/* All conditions met: preserve both the quotes */
					*LastQuote = _T('"');
					_tcscpy(commandline, ptr - 1);
					return;
				}
				break;
			}
		}

		/* The conditions were not met: remove both the
		 * leading quote and the last quote */
		_tcscpy(commandline, ptr);
		_tcscpy(&commandline[LastQuote - ptr], LastQuote + 1);
		return;
	}

	/* No quotes; just copy */
	_tcscpy(commandline, ptr);
}

/*
 * set up global initializations and process parameters
 */
static VOID
Initialize()
{
	HMODULE NtDllModule;
	TCHAR commandline[CMDLINE_LENGTH];
	TCHAR ModuleName[_MAX_PATH + 1];
	TCHAR lpBuffer[2];

	//INT len;
	TCHAR *ptr, *cmdLine, option = 0;
	BOOL AlwaysStrip = FALSE;
	BOOL AutoRun = TRUE;

	/* get version information */
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx (&osvi);

	/* Some people like to run ReactOS cmd.exe on Win98, it helps in the
	 * build process. So don't link implicitly against ntdll.dll, load it
	 * dynamically instead */
	NtDllModule = GetModuleHandle(TEXT("ntdll.dll"));
	if (NtDllModule != NULL)
	{
		NtQueryInformationProcessPtr = (NtQueryInformationProcessProc)GetProcAddress(NtDllModule, "NtQueryInformationProcess");
		NtReadVirtualMemoryPtr = (NtReadVirtualMemoryProc)GetProcAddress(NtDllModule, "NtReadVirtualMemory");
	}

	cmdLine = GetCommandLine();
	TRACE ("[command args: %s]\n", debugstr_aw(cmdLine));

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


	SetConsoleMode (hIn, ENABLE_PROCESSED_INPUT);

#ifdef INCLUDE_CMD_CHDIR
	InitLastPath ();
#endif

	for (ptr = cmdLine; *ptr; ptr++)
	{
		if (*ptr == _T('/'))
		{
			option = _totupper(ptr[1]);
			if (option == _T('?'))
			{
				ConOutResPaging(TRUE,STRING_CMD_HELP8);
				cmd_exit(0);
			}
			else if (option == _T('P'))
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
			else if (option == _T('A'))
			{
				bUnicodeOutput = FALSE;
			}
			else if (option == _T('C') || option == _T('K') || option == _T('R'))
			{
				/* Remainder of command line is a command to be run */
				break;
			}
			else if (option == _T('D'))
			{
				AutoRun = FALSE;
			}
			else if (option == _T('Q'))
			{
				bDisableBatchEcho = TRUE;
			}
			else if (option == _T('S'))
			{
				AlwaysStrip = TRUE;
			}
#ifdef INCLUDE_CMD_COLOR
			else if (!_tcsnicmp(ptr, _T("/T:"), 3))
			{
				/* process /t (color) argument */
				wDefColor = (WORD)_tcstoul(&ptr[3], &ptr, 16);
				SetScreenColor(wDefColor, TRUE);
			}
#endif
			else if (option == _T('U'))
			{
				bUnicodeOutput = TRUE;
			}
			else if (option == _T('V'))
			{
				bDelayedExpansion = _tcsnicmp(&ptr[2], _T(":OFF"), 4);
			}
		}
	}

	if (!*ptr)
	{
		/* If neither /C or /K was given, display a simple version string */
		ConOutResPrintf(STRING_REACTOS_VERSION, 
			_T(KERNEL_RELEASE_STR),
			_T(KERNEL_VERSION_BUILD_STR));
		ConOutPuts(_T("(C) Copyright 1998-") _T(COPYRIGHT_YEAR) _T(" ReactOS Team."));
	}

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

	if (AutoRun)
		ExecuteAutoRunFile();

	if (*ptr)
	{
		/* Do the /C or /K command */
		GetCmdLineCommand(commandline, &ptr[2], AlwaysStrip);
		ParseCommandLine(commandline);
		if (option != _T('K'))
			bExit = TRUE;
	}
}


static VOID Cleanup()
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
}

/*
 * main function
 */
int cmd_main (int argc, const TCHAR *argv[])
{
	TCHAR startPath[MAX_PATH];
	CONSOLE_SCREEN_BUFFER_INFO Info;
	INT nExitCode;

	lpOriginalEnvironment = DuplicateEnvironment();

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
	wDefColor = Info.wAttributes;

	InputCodePage= GetConsoleCP();
	OutputCodePage = GetConsoleOutputCP();
	CMD_ModuleHandle = GetModuleHandle(NULL);

	/* check switches on command-line */
	Initialize();

	/* call prompt routine */
	nExitCode = ProcessInput();

	/* do the cleanup */
	Cleanup();

	cmd_free(lpOriginalEnvironment);

	cmd_exit(nExitCode);
	return(nExitCode);
}

/* EOF */
