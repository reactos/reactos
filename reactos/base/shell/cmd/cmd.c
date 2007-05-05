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
#include <malloc.h>
#include "resource.h"

#ifndef NT_SUCCESS
#define NT_SUCCESS(StatCode)  ((NTSTATUS)(StatCode) >= 0)
#endif

typedef NTSTATUS (WINAPI *NtQueryInformationProcessProc)(HANDLE, PROCESSINFOCLASS,
                                                          PVOID, ULONG, PULONG);
typedef NTSTATUS (WINAPI *NtReadVirtualMemoryProc)(HANDLE, PVOID, PVOID, ULONG, PULONG);

BOOL bExit = FALSE;       /* indicates EXIT was typed */
BOOL bCanExit = TRUE;     /* indicates if this shell is exitable */
BOOL bCtrlBreak = FALSE;  /* Ctrl-Break or Ctrl-C hit */
BOOL bIgnoreEcho = FALSE; /* Ignore 'newline' before 'cls' */
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
	INT c = 0;
	INT n = 0;

	if (len <= 1)
		return 0;

	if (num.QuadPart == 0)
	{
		des[0] = _T('0');
		des[1] = _T('\0');
		n = 1;
	}
	else
	{
		temp[31] = 0;
		while (num.QuadPart > 0)
		{
			if ((((c + 1) % (nNumberGroups + 1)) == 0) && (bPutSeperator))
				temp[30 - c++] = cThousandSeparator;
                        temp[30 - c++] = (TCHAR)(num.QuadPart % 10) + _T('0');
			num.QuadPart /= 10;
		}
        if (c>len)
			c=len;

		for (n = 0; n <= c; n++)
			des[n] = temp[31 - c + n];
	}

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
#ifdef _DEBUG
		DebugPrintf (_T("NtQueryInformationProcess failed with status %08x\n"), Status);
#endif
		return TRUE;
	}
	Status = NtReadVirtualMemoryPtr (
		Process, Info.PebBaseAddress, &ProcessPeb,
		sizeof(PEB), &BytesRead);
	if (! NT_SUCCESS(Status) || sizeof(PEB) != BytesRead)
	{
#ifdef _DEBUG
		DebugPrintf (_T("Couldn't read virt mem status %08x bytes read %lu\n"), Status, BytesRead);
#endif
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

#ifdef _DEBUG
	DebugPrintf (_T("RunFile(%s)\n"), filename);
#endif
	hShell32 = LoadLibrary(_T("SHELL32.DLL"));
	if (!hShell32)
	{
#ifdef _DEBUG
		DebugPrintf (_T("RunFile: couldn't load SHELL32.DLL!\n"));
#endif
		return FALSE;
	}

	hShExt = (MYEX)(FARPROC)GetProcAddress(hShell32, SHELLEXECUTETEXT);
	if (!hShExt)
	{
#ifdef _DEBUG
		DebugPrintf (_T("RunFile: couldn't find ShellExecuteA/W in SHELL32.DLL!\n"));
#endif
		FreeLibrary(hShell32);
		return FALSE;
	}

#ifdef _DEBUG
	DebugPrintf (_T("RunFile: ShellExecuteA/W is at %x\n"), hShExt);
#endif

	ret = (hShExt)(NULL, _T("open"), filename, NULL, NULL, SW_SHOWNORMAL);

#ifdef _DEBUG
	DebugPrintf (_T("RunFile: ShellExecuteA/W returned %d\n"), (DWORD)ret);
#endif

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

static VOID
Execute (LPTSTR Full, LPTSTR First, LPTSTR Rest)
{
	TCHAR *szFullName=NULL;
	TCHAR *first = NULL;
	TCHAR *rest = NULL;
	TCHAR *full = NULL;
	TCHAR *dot = NULL;
	TCHAR szWindowTitle[MAX_PATH];
	DWORD dwExitCode = 0;

#ifdef _DEBUG
	DebugPrintf (_T("Execute: \'%s\' \'%s\'\n"), first, rest);
#endif

	/* we need biger buffer that First, Rest, Full are already
	   need rewrite some code to use realloc when it need instead
	   of add 512bytes extra */

	first = malloc ( (_tcslen(First) + 512) * sizeof(TCHAR));
	if (first == NULL)
	{
		error_out_of_memory();
                nErrorLevel = 1;
		return ;
	}

	rest = malloc ( (_tcslen(Rest) + 512) * sizeof(TCHAR));
	if (rest == NULL)
	{
		free (first);
		error_out_of_memory();
                nErrorLevel = 1;
		return ;
	}

	full = malloc ( (_tcslen(Full) + 512) * sizeof(TCHAR));
	if (full == NULL)
	{
		free (first);
		free (rest);
		error_out_of_memory();
                nErrorLevel = 1;
		return ;
	}

	szFullName = malloc ( (_tcslen(Full) + 512) * sizeof(TCHAR));
	if (full == NULL)
	{
		free (first);
		free (rest);
		free (full);
		error_out_of_memory();
                nErrorLevel = 1;
		return ;
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

		free (first);
		free (rest);
		free (full);
		free (szFullName);
                nErrorLevel = 1;
		return;
	}

	/* get the PATH environment variable and parse it */
	/* search the PATH environment variable for the binary */
	if (!SearchForExecutable (first, szFullName))
	{
			error_bad_command ();
			free (first);
			free (rest);
			free (full);
			free (szFullName);
                        nErrorLevel = 1;
			return;

	}

	GetConsoleTitle (szWindowTitle, MAX_PATH);

	/* check if this is a .BAT or .CMD file */
	dot = _tcsrchr (szFullName, _T('.'));
	if (dot && (!_tcsicmp (dot, _T(".bat")) || !_tcsicmp (dot, _T(".cmd"))))
	{
#ifdef _DEBUG
		DebugPrintf (_T("[BATCH: %s %s]\n"), szFullName, rest);
#endif
		Batch (szFullName, first, rest);
	}
	else
	{
		/* exec the program */
		PROCESS_INFORMATION prci;
		STARTUPINFO stui;

#ifdef _DEBUG
		DebugPrintf (_T("[EXEC: %s %s]\n"), full, rest);
#endif
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
#ifdef _DEBUG
			DebugPrintf (_T("[ShellExecute: %s]\n"), full);
#endif
			// See if we can run this with ShellExecute() ie myfile.xls
			if (!RunFile(full))
			{
#ifdef _DEBUG
				DebugPrintf (_T("[ShellExecute failed!: %s]\n"), full);
#endif
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

	free(first);
	free(rest);
	free(full);
	free (szFullName);
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
	TCHAR *com = NULL;  /* the first word in the command */
	TCHAR *cp = NULL;
	LPTSTR cstart;
	LPTSTR rest;   /* pointer to the rest of the command line */
	INT cl;
	LPCOMMAND cmdptr;

#ifdef _DEBUG
	DebugPrintf (_T("DoCommand: (\'%s\')\n"), line);
#endif /* DEBUG */

	com = malloc( (_tcslen(line) +512)*sizeof(TCHAR) );
	if (com == NULL)
	{
		error_out_of_memory();
		return;
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
			free(com);
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
				Execute (line, com, rest);
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
	free(com);
}


/*
 * process the command line and execute the appropriate functions
 * full input/output redirection and piping are supported
 */

VOID ParseCommandLine (LPTSTR cmd)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	TCHAR cmdline[CMDLINE_LENGTH];
	LPTSTR s;
#ifdef FEATURE_REDIRECTION
	TCHAR in[CMDLINE_LENGTH] = _T("");
	TCHAR out[CMDLINE_LENGTH] = _T("");
	TCHAR err[CMDLINE_LENGTH] = _T("");
	TCHAR szTempPath[MAX_PATH] = _T(".\\");
	TCHAR szFileName[2][MAX_PATH] = {_T(""), _T("")};
	HANDLE hFile[2] = {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE};
	LPTSTR t = NULL;
	INT  num = 0;
	INT  nRedirFlags = 0;
	INT  Length;
	UINT Attributes;
	BOOL bNewBatch = TRUE;
	HANDLE hOldConIn;
	HANDLE hOldConOut;
	HANDLE hOldConErr;
#endif /* FEATURE_REDIRECTION */

	_tcscpy (cmdline, cmd);
	s = &cmdline[0];

#ifdef _DEBUG
	DebugPrintf (_T("ParseCommandLine: (\'%s\')\n"), s);
#endif /* DEBUG */

#ifdef FEATURE_ALIASES
	/* expand all aliases */
	ExpandAlias (s, CMDLINE_LENGTH);
#endif /* FEATURE_ALIAS */

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

	if(bc && !_tcslen (in) && _tcslen (bc->In))
		_tcscpy(in, bc->In);
	if(bc && !out[0] && _tcslen(bc->Out))
	{
		nRedirFlags |= OUTPUT_APPEND;
		_tcscpy(out, bc->Out);
	}
	if(bc && !_tcslen (err) && _tcslen (bc->Err))
	{
		nRedirFlags |= ERROR_APPEND;
		_tcscpy(err, bc->Err);
	}


	/* Set up the initial conditions ... */
	/* preserve STDIN, STDOUT and STDERR handles */
	hOldConIn  = GetStdHandle (STD_INPUT_HANDLE);
	hOldConOut = GetStdHandle (STD_OUTPUT_HANDLE);
	hOldConErr = GetStdHandle (STD_ERROR_HANDLE);

	/* redirect STDIN */
	if (in[0])
	{
		HANDLE hFile;
		SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

	/* we need make sure the LastError msg is zero before calling CreateFile */
		SetLastError(0);

	/* Set up pipe for the standard input handler */
		hFile = CreateFile (in, GENERIC_READ, FILE_SHARE_READ, &sa, OPEN_EXISTING,
		                    FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			LoadString(CMD_ModuleHandle, STRING_CMD_ERROR1, szMsg, RC_STRING_MAX_SIZE);
			ConErrPrintf(szMsg, in);
			return;
		}

		if (!SetStdHandle (STD_INPUT_HANDLE, hFile))
		{
			LoadString(CMD_ModuleHandle, STRING_CMD_ERROR1, szMsg, RC_STRING_MAX_SIZE);
			ConErrPrintf(szMsg, in);
			return;
		}
#ifdef _DEBUG
		DebugPrintf (_T("Input redirected from: %s\n"), in);
#endif
	}

	/* Now do all but the last pipe command */
	*szFileName[0] = _T('\0');
	hFile[0] = INVALID_HANDLE_VALUE;

	while (num-- > 1)
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
			LoadString(CMD_ModuleHandle, STRING_CMD_ERROR2, szMsg, RC_STRING_MAX_SIZE);
			ConErrPrintf(szMsg);
			return;
		}

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

		/* we need make sure the LastError msg is zero before calling CreateFile */
		SetLastError(0);

		/* open new stdin file */
		hFile[0] = CreateFile (szFileName[0], GENERIC_READ, 0, &sa,
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
		SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

		/* we need make sure the LastError msg is zero before calling CreateFile */
		SetLastError(0);

		hFile = CreateFile (out, GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE, &sa,
		                    (nRedirFlags & OUTPUT_APPEND) ? OPEN_ALWAYS : CREATE_ALWAYS,
		                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			INT size = _tcslen(out)-1;

			if (out[size] != _T(':'))
			{
				LoadString(CMD_ModuleHandle, STRING_CMD_ERROR3, szMsg, RC_STRING_MAX_SIZE);
				ConErrPrintf(szMsg, out);
				return;
			}

			out[size]=_T('\0');
			hFile = CreateFile (out, GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE, &sa,
			                    (nRedirFlags & OUTPUT_APPEND) ? OPEN_ALWAYS : CREATE_ALWAYS,
			                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);

			if (hFile == INVALID_HANDLE_VALUE)
			{
				LoadString(CMD_ModuleHandle, STRING_CMD_ERROR3, szMsg, RC_STRING_MAX_SIZE);
				ConErrPrintf(szMsg, out);
				return;
			}

		}

		if (!SetStdHandle (STD_OUTPUT_HANDLE, hFile))
		{
			LoadString(CMD_ModuleHandle, STRING_CMD_ERROR3, szMsg, RC_STRING_MAX_SIZE);
			ConErrPrintf(szMsg, out);
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
		SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

		if (!_tcscmp (err, out))
		{
#ifdef _DEBUG
			DebugPrintf (_T("Stdout and stderr will use the same file!!\n"));
#endif
			DuplicateHandle (GetCurrentProcess (),
			                 GetStdHandle (STD_OUTPUT_HANDLE),
			                 GetCurrentProcess (),
			                 &hFile, 0, TRUE, DUPLICATE_SAME_ACCESS);
		}
		else
		{
			hFile = CreateFile (err,
			                    GENERIC_WRITE,
			                    FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
			                    &sa,
			                    (nRedirFlags & ERROR_APPEND) ? OPEN_ALWAYS : CREATE_ALWAYS,
			                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
			                    NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				LoadString(CMD_ModuleHandle, STRING_CMD_ERROR3, szMsg, RC_STRING_MAX_SIZE);
				ConErrPrintf(szMsg, err);
				return;
			}
		}

		if (!SetStdHandle (STD_ERROR_HANDLE, hFile))
		{
			LoadString(CMD_ModuleHandle, STRING_CMD_ERROR3, szMsg, RC_STRING_MAX_SIZE);
			ConErrPrintf(szMsg, err);
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

	if(bc)
		bNewBatch = FALSE;
#endif

	/* process final command */
	DoCommand (s);

#ifdef FEATURE_REDIRECTION
	if(bNewBatch && bc)
		AddBatchRedirection(in, out, err);
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

BOOL
GrowIfNecessary ( UINT needed, LPTSTR* ret, UINT* retlen )
{
	if ( *ret && needed < *retlen )
		return TRUE;
	*retlen = needed;
	if ( *ret )
		free ( *ret );
	*ret = (LPTSTR)malloc ( *retlen * sizeof(TCHAR) );
	if ( !*ret )
		SetLastError ( ERROR_OUTOFMEMORY );
	return *ret != NULL;
}

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

	GrowIfNecessary(_tcslen(varName) + 2, &ret, &retlen);
	_stprintf(ret,_T("%%%s%%"),varName);
	return ret; /* not found - return orginal string */
}

LPCTSTR
GetParsedEnvVar ( LPCTSTR varName, UINT* varNameLen, BOOL ModeSetA )
{
	static LPTSTR ret = NULL;
	static UINT retlen = 0;
	LPTSTR p, tmp;
	UINT size;

	if ( varNameLen )
		*varNameLen = 0;
	SetLastError(0);
	if ( *varName++ != '%' )
		return NULL;
	switch ( *varName )
	{
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
		if ((tmp = FindArg (*varName - _T('0'))))
		{
			if ( varNameLen )
				*varNameLen = 2;
			if ( !*tmp )
				return _T("");
			if ( !GrowIfNecessary ( _tcslen(tmp)+1, &ret, &retlen ) )
				return NULL;
			_tcscpy ( ret, tmp );
			return ret;
		}
		if ( !GrowIfNecessary ( 3, &ret, &retlen ) )
			return NULL;
		ret[0] = _T('%');
		ret[1] = *varName;
		ret[2] = 0;
		if ( varNameLen )
			*varNameLen = 2;
		return ret;
   
    case _T('*'):
        if(bc == NULL)
        {
            //
            // No batch file to see here, move along
            //
            if ( !GrowIfNecessary ( 3, &ret, &retlen ) )
                return NULL;
            ret[0] = _T('%');
            ret[1] = _T('*');
            ret[2] = 0;
            if ( varNameLen )
                *varNameLen = 2;
            return ret;
        }

        //
        // Copy over the raw params(not including the batch file name
        //
        if ( !GrowIfNecessary ( _tcslen(bc->raw_params)+1, &ret, &retlen ) )
            return NULL;
        if ( varNameLen )
            *varNameLen = 2;
        _tcscpy ( ret, bc->raw_params );
        return ret;

	case _T('%'):
		if ( !GrowIfNecessary ( 2, &ret, &retlen ) )
			return NULL;
		ret[0] = _T('%');
		ret[1] = 0;
		if ( varNameLen )
			*varNameLen = 2;
		return ret;

	case _T('?'):
		/* TODO FIXME 10 is only max size for 32-bit */
		if ( !GrowIfNecessary ( 11, &ret, &retlen ) )
			return NULL;
		_sntprintf ( ret, retlen, _T("%u"), nErrorLevel);
		ret[retlen-1] = 0;
		if ( varNameLen )
			*varNameLen = 2;
		return ret;
	}
	if ( ModeSetA )
	{
		/* HACK for set/a */
		if ( !GrowIfNecessary ( 2, &ret, &retlen ) )
			return NULL;
		ret[0] = _T('%');
		ret[1] = 0;
		if ( varNameLen )
			*varNameLen = 1;
		return ret;
	}
	p = _tcschr ( varName, _T('%') );
	if ( !p )
	{
		SetLastError ( ERROR_INVALID_PARAMETER );
		return NULL;
	}
	size = p-varName;
	if ( varNameLen )
		*varNameLen = size + 2;
	p = alloca ( (size+1) * sizeof(TCHAR) );
	memmove ( p, varName, size * sizeof(TCHAR) );
	p[size] = 0;
	varName = p;
	return GetEnvVarOrSpecial ( varName );
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
	LPTSTR ip;
	LPTSTR cp;
	LPCTSTR tmp;
	BOOL bEchoThisLine;
	BOOL bModeSetA;
        BOOL bIsBatch;

	do
	{
		/* if no batch input then... */
		if (!(ip = ReadBatchLine (&bEchoThisLine)))
		{
			if (bFlag)
				return nErrorLevel;

			ReadCommand (readline, CMDLINE_LENGTH);
			ip = readline;
			bEchoThisLine = FALSE;
            bIsBatch = FALSE;
		}
        else
        {
            bIsBatch = TRUE;
        }

		/* skip leading blanks */
		while ( _istspace(*ip) )
			++ip;

		cp = commandline;
		bModeSetA = FALSE;
		while (*ip)
		{
			if ( *ip == _T('%') )
			{
				UINT envNameLen;
				LPCTSTR envVal = GetParsedEnvVar ( ip, &envNameLen, bModeSetA );
				if ( envVal )
				{
					ip += envNameLen;
					cp = _stpcpy ( cp, envVal );
				}
			}

			if (_istcntrl (*ip))
				*ip = _T(' ');
			*cp++ = *ip++;

			/* HACK HACK HACK check whether bModeSetA needs to be toggled */
			*cp = 0;
			tmp = commandline;
			tmp += _tcsspn(tmp,_T(" \t"));
			/* first we find and skip and pre-redirections... */
			while (( tmp ) &&
				( _tcschr(_T("<>"),*tmp)
				|| !_tcsncmp(tmp,_T("1>"),2)
				|| !_tcsncmp(tmp,_T("2>"),2) ))
			{
				if ( _istdigit(*tmp) )
					tmp += 2;
				else
					tmp++;
				tmp += _tcsspn(tmp,_T(" \t"));
				if ( *tmp == _T('\"') )
				{
					tmp = _tcschr(tmp+1,_T('\"'));
					if ( tmp )
						++tmp;
				}
				else
					tmp = _tcspbrk(tmp,_T(" \t"));
				if ( tmp )
					tmp += _tcsspn(tmp,_T(" \t"));
			}
			/* we should now be pointing to the actual command
			 * (if there is one yet)*/
			if ( tmp )
			{
				/* if we're currently substituting ( which is default )
				 * check to see if we've parsed out a set/a. if so, we
				 * need to disable substitution until we come across a
				 * redirection */
				if ( !bModeSetA )
				{
					/* look for set /a */
					if ( !_tcsnicmp(tmp,_T("set"),3) )
					{
						tmp += 3;
						tmp += _tcsspn(tmp,_T(" \t"));
						if ( !_tcsnicmp(tmp,_T("/a"),2) )
							bModeSetA = TRUE;
					}
				}
				/* if we're not currently substituting, it means we're
				 * already inside a set /a. now we need to look for
				 * a redirection in order to turn redirection back on */
				else
				{
					/* look for redirector of some kind after the command */
					while ( (tmp = _tcspbrk ( tmp, _T("^<>|") )) )
					{
						if ( *tmp == _T('^') )
						{
							if ( _tcschr(_T("<>|&"), *++tmp ) && *tmp )
								++tmp;
						}
						else
						{
							bModeSetA = FALSE;
							break;
						}
					}
				}
			}
		}

		*cp = _T('\0');

		/* strip trailing spaces */
		while ((--cp >= commandline) && _istspace (*cp));

		*(cp + 1) = _T('\0');

		/* JPP 19980807 */
		/* Echo batch file line */
		if (bEchoThisLine)
		{
			PrintPrompt ();
			ConOutPuts (commandline);
		}

		if (!CheckCtrlBreak(BREAK_INPUT) && *commandline)
		{
			ParseCommandLine (commandline);
			if (bEcho && !bIgnoreEcho && (!bIsBatch || bEchoThisLine))
				ConOutChar ('\n');
			bIgnoreEcho = FALSE;
		}
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

/*
 * set up global initializations and process parameters
 *
 * argc - number of parameters to command.com
 * argv - command-line parameters
 *
 */
static VOID
Initialize (int argc, TCHAR* argv[])
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


#ifdef _DEBUG
	DebugPrintf (_T("[command args:\n"));
	for (i = 0; i < argc; i++)
	{
		DebugPrintf (_T("%d. %s\n"), i, argv[i]);
	}
	DebugPrintf (_T("]\n"));
#endif

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
		ExitProcess(0);
	}
	SetConsoleMode (hIn, ENABLE_PROCESSED_INPUT);

#ifdef INCLUDE_CMD_CHDIR
	InitLastPath ();
#endif

#ifdef FATURE_ALIASES
	InitializeAlias ();
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
					cmd_date (_T(""), _T(""));
#endif
#ifdef INCLUDE_CMD_TIME
					cmd_time (_T(""), _T(""));
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
					ExitProcess (ProcessInput (TRUE));
				}
				else
				{
					ExitProcess (0);
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

	/* run cmdstart.bat */
	if (IsExistingFile (_T("cmdstart.bat")))
	{
		ParseCommandLine (_T("cmdstart.bat"));
	}
	else if (IsExistingFile (_T("\\cmdstart.bat")))
	{
		ParseCommandLine (_T("\\cmdstart.bat"));
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
}


static VOID Cleanup (int argc, TCHAR *argv[])
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

#ifdef FEATURE_ALIASES
	DestroyAlias ();
#endif

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
#ifdef _UNICODE
int _main(void)
#else
int _main (int argc, char *argv[])
#endif
{
	TCHAR startPath[MAX_PATH];
	CONSOLE_SCREEN_BUFFER_INFO Info;
	INT nExitCode;
#ifdef _UNICODE
	PWCHAR * argv;
	int argc=0;
	argv = CommandLineToArgvW(GetCommandLineW(), &argc);
#endif

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

	return(nExitCode);
}

/* EOF */
