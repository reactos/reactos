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
 *        Fixed carriage return output to better match MSDOS with echo
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
 *        Add proper memory alloc ProcessInput, the error
 *        handling for memory handling need to be improve
 */

#include "precomp.h"
#include <reactos/buildno.h>
#include <reactos/version.h>

typedef NTSTATUS (WINAPI *NtQueryInformationProcessProc)(HANDLE, PROCESSINFOCLASS,
                                                          PVOID, ULONG, PULONG);
typedef NTSTATUS (WINAPI *NtReadVirtualMemoryProc)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);

BOOL bExit = FALSE;       /* Indicates EXIT was typed */
BOOL bCanExit = TRUE;     /* Indicates if this shell is exitable */
BOOL bCtrlBreak = FALSE;  /* Ctrl-Break or Ctrl-C hit */
BOOL bIgnoreEcho = FALSE; /* Set this to TRUE to prevent a newline, when executing a command */
static BOOL fSingleCommand = 0; /* When we are executing something passed on the command line after /C or /K */
static BOOL bAlwaysStrip = FALSE;
INT  nErrorLevel = 0;     /* Errorlevel of last launched external program */
CRITICAL_SECTION ChildProcessRunningLock;
BOOL bDisableBatchEcho = FALSE;
BOOL bEnableExtensions = TRUE;
BOOL bDelayedExpansion = FALSE;
DWORD dwChildProcessId = 0;
LPTSTR lpOriginalEnvironment;
HANDLE CMD_ModuleHandle;

BOOL bTitleSet = FALSE; /* Indicates whether the console title has been changed and needs to be restored later */
TCHAR szCurTitle[MAX_PATH];

static NtQueryInformationProcessProc NtQueryInformationProcessPtr = NULL;
static NtReadVirtualMemoryProc       NtReadVirtualMemoryPtr = NULL;

/*
 * Default output file stream translation mode is UTF8, but CMD switches
 * allow to change it to either UTF16 (/U) or ANSI (/A).
 */
CON_STREAM_MODE OutputStreamMode = UTF8Text; // AnsiText;

#ifdef INCLUDE_CMD_COLOR
WORD wDefColor = 0;     /* Default color */
#endif

/*
 * convert
 *
 * insert commas into a number
 */
INT
ConvertULargeInteger(ULONGLONG num, LPTSTR des, UINT len, BOOL bPutSeparator)
{
    TCHAR temp[39];   /* maximum length with nNumberGroups == 1 */
    UINT  n, iTarget;

    if (len <= 1)
        return 0;

    n = 0;
    iTarget = nNumberGroups;
    if (!nNumberGroups)
        bPutSeparator = FALSE;

    do
    {
        if (iTarget == n && bPutSeparator)
        {
            iTarget += nNumberGroups + 1;
            temp[38 - n++] = cThousandSeparator;
        }
        temp[38 - n++] = (TCHAR)(num % 10) + _T('0');
        num /= 10;
    } while (num > 0);
    if (n > len-1)
        n = len-1;

    memcpy(des, temp + 39 - n, n * sizeof(TCHAR));
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
    SIZE_T BytesRead;

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
        WARN ("Couldn't read virt mem status %08x bytes read %Iu\n", Status, BytesRead);
        return TRUE;
    }

    return IMAGE_SUBSYSTEM_WINDOWS_CUI == ProcessPeb.ImageSubsystem;
}



#ifdef _UNICODE
#define SHELLEXECUTETEXT    "ShellExecuteExW"
#else
#define SHELLEXECUTETEXT    "ShellExecuteExA"
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


static VOID
SetConTitle(LPCTSTR pszTitle)
{
    TCHAR szNewTitle[MAX_PATH];

    if (!pszTitle)
        return;

    /* Don't do anything if we run inside a batch file, or we are just running a single command */
    if (bc || (fSingleCommand == 1))
        return;

    /* Save the original console title and build a new one */
    GetConsoleTitle(szCurTitle, ARRAYSIZE(szCurTitle));
    StringCchPrintf(szNewTitle, ARRAYSIZE(szNewTitle),
                    _T("%s - %s"), szCurTitle, pszTitle);
    bTitleSet = TRUE;
    ConSetTitle(szNewTitle);
}

static VOID
ResetConTitle(VOID)
{
    /* Restore the original console title */
    if (!bc && bTitleSet)
    {
        ConSetTitle(szCurTitle);
        bTitleSet = FALSE;
    }
}

/*
 * This command (in First) was not found in the command table
 *
 * Full  - output buffer to hold whole command line
 * First - first word on command line
 * Rest  - rest of command line
 */
static INT
Execute(LPTSTR Full, LPTSTR First, LPTSTR Rest, PARSED_COMMAND *Cmd)
{
    TCHAR *first, *rest, *dot;
    DWORD dwExitCode = 0;
    TCHAR *FirstEnd;
    TCHAR szFullName[MAX_PATH];
    TCHAR szFullCmdLine[CMDLINE_LENGTH];

    TRACE ("Execute: \'%s\' \'%s\'\n", debugstr_aw(First), debugstr_aw(Rest));

    /* Though it was already parsed once, we have a different set of rules
       for parsing before we pass to CreateProcess */
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
    rest = &Full[FirstEnd - First + 1];
    _tcscpy(rest, FirstEnd);
    _tcscat(rest, Rest);
    first = Full;
    *FirstEnd = _T('\0');
    _tcscpy(first, First);

    /* check for a drive change */
    if ((_istalpha (first[0])) && (!_tcscmp (first + 1, _T(":"))))
    {
        BOOL working = TRUE;
        if (!SetCurrentDirectory(first))
        {
            /* Guess they changed disc or something, handle that gracefully and get to root */
            TCHAR str[4];
            str[0]=first[0];
            str[1]=_T(':');
            str[2]=_T('\\');
            str[3]=0;
            working = SetCurrentDirectory(str);
        }

        if (!working) ConErrResPuts (STRING_FREE_ERROR1);
        return !working;
    }

    /* get the PATH environment variable and parse it */
    /* search the PATH environment variable for the binary */
    StripQuotes(First);
    if (!SearchForExecutable(First, szFullName))
    {
        error_bad_command(first);
        return 1;
    }

    /* Set the new console title */
    FirstEnd = first + (FirstEnd - First); /* Point to the separating NULL in the full built string */
    *FirstEnd = _T(' ');
    SetConTitle(Full);

    /* check if this is a .BAT or .CMD file */
    dot = _tcsrchr (szFullName, _T('.'));
    if (dot && (!_tcsicmp (dot, _T(".bat")) || !_tcsicmp (dot, _T(".cmd"))))
    {
        while (*rest == _T(' '))
            rest++;

        *FirstEnd = _T('\0');
        TRACE ("[BATCH: %s %s]\n", debugstr_aw(szFullName), debugstr_aw(rest));
        dwExitCode = Batch(szFullName, first, rest, Cmd);
    }
    else
    {
        /* exec the program */
        PROCESS_INFORMATION prci;
        STARTUPINFO stui;

        /* build command line for CreateProcess(): FullName + " " + rest */
        BOOL quoted = !!_tcschr(First, _T(' '));
        _tcscpy(szFullCmdLine, quoted ? _T("\"") : _T(""));
        _tcsncat(szFullCmdLine, First, CMDLINE_LENGTH - _tcslen(szFullCmdLine) - 1);
        _tcsncat(szFullCmdLine, quoted ? _T("\"") : _T(""), CMDLINE_LENGTH - _tcslen(szFullCmdLine) - 1);

        if (*rest)
        {
            _tcsncat(szFullCmdLine, _T(" "), CMDLINE_LENGTH - _tcslen(szFullCmdLine) - 1);
            _tcsncat(szFullCmdLine, rest, CMDLINE_LENGTH - _tcslen(szFullCmdLine) - 1);
        }

        TRACE ("[EXEC: %s]\n", debugstr_aw(szFullCmdLine));

        /* fill startup info */
        memset(&stui, 0, sizeof(stui));
        stui.cb = sizeof(stui);
        stui.lpTitle = Full;
        stui.dwFlags = STARTF_USESHOWWINDOW;
        stui.wShowWindow = SW_SHOWDEFAULT;

        /* Set the console to standard mode */
        SetConsoleMode(ConStreamGetOSHandle(StdIn),
                       ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);

        if (CreateProcess(szFullName,
                          szFullCmdLine,
                          NULL,
                          NULL,
                          TRUE,
                          0,
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

        *FirstEnd = _T('\0');

        if (prci.hProcess != NULL)
        {
            if (bc != NULL || fSingleCommand != 0 || IsConsoleProcess(prci.hProcess))
            {
                /* when processing a batch file or starting console processes: execute synchronously */
                EnterCriticalSection(&ChildProcessRunningLock);
                dwChildProcessId = prci.dwProcessId;

                WaitForSingleObject(prci.hProcess, INFINITE);

                LeaveCriticalSection(&ChildProcessRunningLock);

                GetExitCodeProcess(prci.hProcess, &dwExitCode);
                nErrorLevel = (INT)dwExitCode;
            }
            CloseHandle(prci.hProcess);
        }
        else
        {
            TRACE ("[ShellExecute failed!: %s]\n", debugstr_aw(Full));
            error_bad_command(first);
            dwExitCode = 1;
        }

        /* Restore the default console mode */
        SetConsoleMode(ConStreamGetOSHandle(StdIn),
                       ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
        SetConsoleMode(ConStreamGetOSHandle(StdOut),
                       ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
    }

    /* Update the local code page cache */
    {
        UINT uNewInputCodePage  = GetConsoleCP();
        UINT uNewOutputCodePage = GetConsoleOutputCP();

        if ((InputCodePage  != uNewInputCodePage) ||
            (OutputCodePage != uNewOutputCodePage))
        {
            InputCodePage  = uNewInputCodePage;
            OutputCodePage = uNewOutputCodePage;

            /* Reset the current thread UI language */
            if (IsConsoleHandle(ConStreamGetOSHandle(StdOut)) ||
                IsConsoleHandle(ConStreamGetOSHandle(StdErr)))
            {
                ConSetThreadUILanguage(0);
            }
            /* Update the streams cached code page */
            ConStdStreamsSetCacheCodePage(InputCodePage, OutputCodePage);

            /* Update the locale as well */
            InitLocale();
        }
    }

    /* Restore the original console title */
    ResetConTitle();

    return dwExitCode;
}


/*
 * Look through the internal commands and determine whether or not this
 * command is one of them. If it is, call the command. If not, call
 * execute to run it as an external program.
 *
 * first - first word on command line
 * rest  - rest of command line
 */
INT
DoCommand(LPTSTR first, LPTSTR rest, PARSED_COMMAND *Cmd)
{
    TCHAR *com;
    TCHAR *cp;
    LPTSTR param;   /* Pointer to command's parameters */
    INT cl;
    LPCOMMAND cmdptr;
    BOOL nointernal = FALSE;
    INT ret;

    TRACE ("DoCommand: (\'%s\' \'%s\')\n", debugstr_aw(first), debugstr_aw(rest));

    /* Full command line */
    com = cmd_alloc((_tcslen(first) + _tcslen(rest) + 2) * sizeof(TCHAR));
    if (com == NULL)
    {
        error_out_of_memory();
        return 1;
    }

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
            {
                while (_istspace(*param))
                    param++;
            }

            /* Set the new console title */
            SetConTitle(com);

            ret = cmdptr->func(param);

            /* Restore the original console title */
            ResetConTitle();

            cmd_free(com);
            return ret;
        }
    }

    ret = Execute(com, first, rest, Cmd);
    cmd_free(com);
    return ret;
}


/*
 * process the command line and execute the appropriate functions
 * full input/output redirection and piping are supported
 */
INT ParseCommandLine(LPTSTR cmd)
{
    INT Ret = 0;
    PARSED_COMMAND *Cmd;

    Cmd = ParseCommand(cmd);
    if (!Cmd)
    {
        /* Return an adequate error code */
        return (!bParseError ? 0 : 1);
    }

    Ret = ExecuteCommand(Cmd);
    FreeCommand(Cmd);
    return Ret;
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
    GetModuleFileName(NULL, CmdPath, ARRAYSIZE(CmdPath));

    /* Build the parameter string to pass to cmd.exe */
    ParamsEnd = _stpcpy(CmdParams, _T("/S/D/C\""));
    ParamsEnd = UnparseCommand(Cmd, ParamsEnd, &CmdParams[CMDLINE_LENGTH - 2]);
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

static INT
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
    EnterCriticalSection(&ChildProcessRunningLock);
    WaitForMultipleObjects(nProcesses, hProcess, TRUE, INFINITE);
    LeaveCriticalSection(&ChildProcessRunningLock);

    /* Use the exit code of the last process in the pipeline */
    GetExitCodeProcess(hProcess[nProcesses - 1], &dwExitCode);
    nErrorLevel = (INT)dwExitCode;

    while (--nProcesses >= 0)
        CloseHandle(hProcess[nProcesses]);
    return nErrorLevel;

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

    return nErrorLevel;
}

INT
ExecuteCommand(
    IN PARSED_COMMAND *Cmd)
{
#define SeenGoto() \
    (bc && bc->current == NULL)

    PARSED_COMMAND *Sub;
    LPTSTR First, Rest;
    INT Ret = 0;

    /* If we don't have any command, or if this is REM, ignore it */
    if (!Cmd || (Cmd->Type == C_REM))
        return 0;
    /*
     * Do not execute any command if we are about to exit CMD, or about to
     * change batch execution context, e.g. in case of a CALL / GOTO / EXIT.
     */
    if (bExit || SeenGoto())
        return 0;

    if (!PerformRedirection(Cmd->Redirections))
        return 1;

    switch (Cmd->Type)
    {
    case C_COMMAND:
        Ret = 1;
        First = DoDelayedExpansion(Cmd->Command.First);
        if (First)
        {
            Rest = DoDelayedExpansion(Cmd->Command.Rest);
            if (Rest)
            {
                Ret = DoCommand(First, Rest, Cmd);
                cmd_free(Rest);
            }
            cmd_free(First);
        }
        /* Fall through */
    case C_REM:
        break;

    case C_QUIET:
    case C_BLOCK:
    case C_MULTI:
        for (Sub = Cmd->Subcommands; Sub && !SeenGoto(); Sub = Sub->Next)
            Ret = ExecuteCommand(Sub);
        break;

    case C_OR:
        Sub = Cmd->Subcommands;
        Ret = ExecuteCommand(Sub);
        if ((Ret != 0) && !SeenGoto())
        {
            nErrorLevel = Ret;
            Ret = ExecuteCommand(Sub->Next);
        }
        break;

    case C_AND:
        Sub = Cmd->Subcommands;
        Ret = ExecuteCommand(Sub);
        if ((Ret == 0) && !SeenGoto())
            Ret = ExecuteCommand(Sub->Next);
        break;

    case C_PIPE:
        Ret = ExecutePipeline(Cmd);
        break;

    case C_FOR:
        Ret = ExecuteFor(Cmd);
        break;

    case C_IF:
        Ret = ExecuteIf(Cmd);
        break;
    }

    UndoRedirection(Cmd->Redirections, NULL);
    return Ret;

#undef SeenGoto
}

INT
ExecuteCommandWithEcho(
    IN PARSED_COMMAND *Cmd)
{
    /* Echo the reconstructed command line */
    if (bEcho && !bDisableBatchEcho && Cmd && (Cmd->Type != C_QUIET))
    {
        if (!bIgnoreEcho)
            ConOutChar(_T('\n'));
        PrintPrompt();
        EchoCommand(Cmd);
        ConOutChar(_T('\n'));
    }

    /* Run the command */
    return ExecuteCommand(Cmd);
}

LPTSTR
GetEnvVar(LPCTSTR varName)
{
    static LPTSTR ret = NULL;
    UINT size;

    cmd_free(ret);
    ret = NULL;
    size = GetEnvironmentVariable(varName, NULL, 0);
    if (size > 0)
    {
        ret = cmd_alloc(size * sizeof(TCHAR));
        if (ret != NULL)
            GetEnvironmentVariable(varName, ret, size + 1);
    }
    return ret;
}

LPCTSTR
GetEnvVarOrSpecial(LPCTSTR varName)
{
    static TCHAR ret[MAX_PATH];

    LPTSTR var = GetEnvVar(varName);
    if (var)
        return var;

    /* The environment variable doesn't exist, look for
     * a "special" one only if extensions are enabled. */
    if (!bEnableExtensions)
        return NULL;

    /* %CD% */
    if (_tcsicmp(varName, _T("CD")) == 0)
    {
        GetCurrentDirectory(ARRAYSIZE(ret), ret);
        return ret;
    }
    /* %DATE% */
    else if (_tcsicmp(varName, _T("DATE")) == 0)
    {
        return GetDateString();
    }
    /* %TIME% */
    else if (_tcsicmp(varName, _T("TIME")) == 0)
    {
        return GetTimeString();
    }
    /* %RANDOM% */
    else if (_tcsicmp(varName, _T("RANDOM")) == 0)
    {
        /* Get random number */
        _itot(rand(), ret, 10);
        return ret;
    }
    /* %CMDCMDLINE% */
    else if (_tcsicmp(varName, _T("CMDCMDLINE")) == 0)
    {
        return GetCommandLine();
    }
    /* %CMDEXTVERSION% */
    else if (_tcsicmp(varName, _T("CMDEXTVERSION")) == 0)
    {
        /* Set Command Extensions version number to CMDEXTVERSION */
        _itot(CMDEXTVERSION, ret, 10);
        return ret;
    }
    /* %ERRORLEVEL% */
    else if (_tcsicmp(varName, _T("ERRORLEVEL")) == 0)
    {
        _itot(nErrorLevel, ret, 10);
        return ret;
    }
#if (NTDDI_VERSION >= NTDDI_WIN7)
    /* Available in Win7+, even if the underlying API is available in Win2003+ */
    /* %HIGHESTNUMANODENUMBER% */
    else if (_tcsicmp(varName, _T("HIGHESTNUMANODENUMBER")) == 0)
    {
        ULONG NumaNodeNumber = 0;
        GetNumaHighestNodeNumber(&NumaNodeNumber);
        _itot(NumaNodeNumber, ret, 10);
        return ret;
    }
#endif

    return NULL;
}

/* Handle the %~var syntax */
static PCTSTR
GetEnhancedVar(
    IN OUT PCTSTR* pFormat,
    IN BOOL (*GetVar)(TCHAR, PCTSTR*, BOOL*))
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

    PCTSTR Format, FormatEnd;
    PCTSTR PathVarName = NULL;
    PCTSTR Variable;
    PCTSTR VarEnd;
    BOOL VariableIsParam0;
    TCHAR FullPath[MAX_PATH];
    TCHAR FixedPath[MAX_PATH];
    PTCHAR Filename, Extension;
    HANDLE hFind;
    WIN32_FIND_DATA w32fd;
    PTCHAR In, Out;

    static TCHAR Result[CMDLINE_LENGTH];

     /* Check whether the current character is a recognized variable.
      * If it is not, then restore the previous one: there is indeed
      * ambiguity between modifier characters and FOR variables;
      * the rule that CMD uses is to pick the longest possible match.
      * This case can happen if we have a FOR-variable specification
      * of the following form:
      *
      *   %~<modifiers><actual FOR variable character><other characters>
      *
      * where the FOR variable character is also a similar to a modifier,
      * but should not be interpreted as is, and the following other
      * characters are not part of the possible modifier characters, and
      * are unrelated to the FOR variable (they can be part of a command).
      * For example, if there is a %n variable, then out of %~anxnd,
      * %~anxn will be substituted rather than just %~an.
      *
      * In the following examples, all characters 'd','p','n','x' are valid modifiers.
      *
      * 1. In this example, the FOR variable character is 'x' and the actual
      *    modifiers are 'dpn'. Parsing will first determine that 'dpnx'
      *    are modifiers, with the possible (last) valid variable being 'x',
      *    and will stop at the letter 'g'. Since 'g' is not a valid
      *    variable, then the actual variable is the lattest one 'x',
      *    and the modifiers are then actually 'dpn'.
      *    The FOR-loop will then display the %x variable formatted with 'dpn'
      *    and will append any other characters following, 'g'.
      *
      *  C:\Temp>for %x in (foo.exe bar.txt) do @echo %~dpnxg
      *  C:\Temp\foog
      *  C:\Temp\barg
      *
      *
      * 2. In this second example, the FOR variable character is 'g' and
      *    the actual modifiers are 'dpnx'. Parsing will determine also that
      *    the possible (last) valid variable could be 'x', but since it's
      *    not present in the FOR-variables list, it won't be the case.
      *    This means that the actual FOR variable character must follow,
      *    in this case, 'g'.
      *
      *  C:\Temp>for %g in (foo.exe bar.txt) do @echo %~dpnxg
      *  C:\Temp\foo.exe
      *  C:\Temp\bar.txt
      */

    /* First, go through as many modifier characters as possible */
    FormatEnd = Format = *pFormat;
    while (*FormatEnd && _tcschr(ModifierTable, _totlower(*FormatEnd)))
        ++FormatEnd;

    if (*FormatEnd == _T('$'))
    {
        /* $PATH: syntax */
        PathVarName = FormatEnd + 1;
        FormatEnd = _tcschr(PathVarName, _T(':'));
        if (!FormatEnd)
            return NULL;

        /* Must be immediately followed by the variable */
        if (!GetVar(*++FormatEnd, &Variable, &VariableIsParam0))
            return NULL;
    }
    else
    {
        /* Backtrack if necessary to get a variable name match */
        while (!GetVar(*FormatEnd, &Variable, &VariableIsParam0))
        {
            if (FormatEnd == Format)
                return NULL;
            --FormatEnd;
        }
    }

    *pFormat = FormatEnd + 1;

    /* If the variable is empty, return an empty string */
    if (!Variable || !*Variable)
        return _T("");

    /* Exclude the leading and trailing quotes */
    VarEnd = &Variable[_tcslen(Variable)];
    if (*Variable == _T('"'))
    {
        ++Variable;
        if (VarEnd > Variable && VarEnd[-1] == _T('"'))
            --VarEnd;
    }

    if ((ULONG_PTR)VarEnd - (ULONG_PTR)Variable >= sizeof(Result))
        return _T("");
    memcpy(Result, Variable, (ULONG_PTR)VarEnd - (ULONG_PTR)Variable);
    Result[VarEnd - Variable] = _T('\0');

    /* Now determine the actual modifiers */
    for (; Format < FormatEnd && *Format != _T('$'); ++Format)
        Modifiers |= 1 << (_tcschr(ModifierTable, _totlower(*Format)) - ModifierTable);

    if (PathVarName)
    {
        /* $PATH: syntax - search the directories listed in the
         * specified environment variable for the file */
        PTSTR PathVar;
        ((PTSTR)FormatEnd)[-1] = _T('\0'); // FIXME: HACK!
        PathVar = GetEnvVar(PathVarName);
        ((PTSTR)FormatEnd)[-1] = _T(':');
        if (!PathVar ||
            !SearchPath(PathVar, Result, NULL, ARRAYSIZE(FullPath), FullPath, NULL))
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
        ASSERT(bc);
        _tcscpy(FullPath, bc->BatchFilePath);
    }
    else
    {
        /* Convert the variable, now without quotes, to a full path */
        if (!GetFullPathName(Result, ARRAYSIZE(FullPath), FullPath, NULL))
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
        if (Out + _tcslen(In) + 1 >= &FixedPath[ARRAYSIZE(FixedPath)])
            return _T("");
        _tcscpy(Out, In);
        hFind = FindFirstFile(FixedPath, &w32fd);
        /* If it doesn't exist, just leave the name as it was given */
        if (hFind != INVALID_HANDLE_VALUE)
        {
            PTSTR FixedComponent = w32fd.cFileName;
            if (*w32fd.cAlternateFileName &&
                ((Modifiers & M_SHORT) || !_tcsicmp(In, w32fd.cAlternateFileName)))
            {
                FixedComponent = w32fd.cAlternateFileName;
            }
            FindClose(hFind);

            if (Out + _tcslen(FixedComponent) + 1 >= &FixedPath[ARRAYSIZE(FixedPath)])
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
#if (NTDDI_VERSION >= NTDDI_WIN8)
                { _T('v'), FILE_ATTRIBUTE_INTEGRITY_STREAM },
                { _T('x'), FILE_ATTRIBUTE_NO_SCRUB_DATA /* 0x20000 */ },
#endif
            };
            for (Attrib = Table; Attrib != &Table[ARRAYSIZE(Table)]; Attrib++)
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
        memcpy(Out, &FixedPath[2], (ULONG_PTR)Filename - (ULONG_PTR)&FixedPath[2]);
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

static PCTSTR
GetBatchVar(
    IN PCTSTR varName,
    OUT PUINT varNameLen)
{
    PCTSTR ret;
    PCTSTR varNameEnd;

    *varNameLen = 1;

    switch (*varName)
    {
    case _T('~'):
    {
        varNameEnd = varName + 1;
        ret = GetEnhancedVar(&varNameEnd, FindArg);
        if (!ret)
        {
            ParseErrorEx(varName);
            return NULL;
        }
        *varNameLen = varNameEnd - varName;
        return ret;
    }

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
    {
        BOOL dummy;
        if (!FindArg(*varName, &ret, &dummy))
            return NULL;
        else
            return ret;
    }

    case _T('*'):
        /* Copy over the raw params (not including the batch file name) */
        return bc->raw_params;

    case _T('%'):
        return _T("%");
    }
    return NULL;
}

BOOL
SubstituteVar(
    IN PCTSTR Src,
    OUT size_t* SrcIncLen, // VarNameLen
    OUT PTCHAR Dest,
    IN PTCHAR DestEnd,
    OUT size_t* DestIncLen,
    IN TCHAR Delim)
{
#define APPEND(From, Length) \
do { \
    if (Dest + (Length) > DestEnd) \
        goto too_long; \
    memcpy(Dest, (From), (Length) * sizeof(TCHAR)); \
    Dest += (Length); \
} while (0)

#define APPEND1(Char) \
do { \
    if (Dest >= DestEnd) \
        goto too_long; \
    *Dest++ = (Char); \
} while (0)

    PCTSTR Var;
    PCTSTR Start, End, SubstStart;
    TCHAR EndChr;
    size_t VarLength;

    Start = Src;
    End = Dest;
    *SrcIncLen = 0;
    *DestIncLen = 0;

    if (!Delim)
        return FALSE;
    if (*Src != Delim)
        return FALSE;

    ++Src;

    /* If we are already at the end of the string, fail the substitution */
    SubstStart = Src;
    if (!*Src || *Src == _T('\r') || *Src == _T('\n'))
        goto bad_subst;

    if (bc && Delim == _T('%'))
    {
        UINT NameLen;
        Var = GetBatchVar(Src, &NameLen);
        if (!Var && bParseError)
        {
            /* Return the partially-parsed command to be
             * echoed for error diagnostics purposes. */
            APPEND1(Delim);
            APPEND(Src, _tcslen(Src) + 1);
            return FALSE;
        }
        if (Var != NULL)
        {
            VarLength = _tcslen(Var);
            APPEND(Var, VarLength);
            Src += NameLen;
            goto success;
        }
    }

    /* Find the end of the variable name. A colon (:) will usually
     * end the name and begin the optional modifier, but not if it
     * is immediately followed by the delimiter (%VAR:%). */
    SubstStart = Src;
    while (*Src && *Src != Delim && !(*Src == _T(':') && Src[1] != Delim))
    {
        ++Src;
    }
    /* If we are either at the end of the string, or the delimiter
     * has been repeated more than once, fail the substitution. */
    if (!*Src || Src == SubstStart)
        goto bad_subst;

    EndChr = *Src;
    *(PTSTR)Src = _T('\0'); // FIXME: HACK!
    Var = GetEnvVarOrSpecial(SubstStart);
    *(PTSTR)Src++ = EndChr;
    if (Var == NULL)
    {
        /* In a batch context, %NONEXISTENT% "expands" to an
         * empty string, otherwise fail the substitution. */
        if (bc)
            goto success;
        goto bad_subst;
    }
    VarLength = _tcslen(Var);

    if (EndChr == Delim)
    {
        /* %VAR% - use as-is */
        APPEND(Var, VarLength);
    }
    else if (*Src == _T('~'))
    {
        /* %VAR:~[start][,length]% - Substring.
         * Negative values are offsets from the end.
         */
        SSIZE_T Start = _tcstol(Src + 1, (PTSTR*)&Src, 0);
        SSIZE_T End = (SSIZE_T)VarLength;
        if (Start < 0)
            Start += VarLength;
        Start = min(max(Start, 0), VarLength);
        if (*Src == _T(','))
        {
            End = _tcstol(Src + 1, (PTSTR*)&Src, 0);
            End += (End < 0) ? VarLength : Start;
            End = min(max(End, Start), VarLength);
        }
        if (*Src++ != Delim)
            goto bad_subst;
        APPEND(&Var[Start], End - Start);
    }
    else
    {
        /* %VAR:old=new%  - Replace all occurrences of old with new.
         * %VAR:*old=new% - Replace first occurrence only,
         *                  and remove everything before it.
         */
        PCTSTR Old, New;
        size_t OldLength, NewLength;
        BOOL Star = FALSE;
        size_t LastMatch = 0, i = 0;

        if (*Src == _T('*'))
        {
            Star = TRUE;
            Src++;
        }

        /* The string to replace may contain the delimiter */
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
                    APPEND(&Var[LastMatch], i - LastMatch);
                APPEND(New, NewLength);
                i += OldLength;
                LastMatch = i;
                if (Star)
                    break;
                continue;
            }
            i++;
        }
        APPEND(&Var[LastMatch], VarLength - LastMatch);
    }

success:
    *SrcIncLen = (Src - Start);
    *DestIncLen = (Dest - End);
    return TRUE;

bad_subst:
    Src = SubstStart;
    /* Only if no batch context active do we echo the delimiter */
    if (!bc)
        APPEND1(Delim);
    goto success;

too_long:
    ConOutResPrintf(STRING_ALIAS_ERROR);
    nErrorLevel = 9023;
    return FALSE;

#undef APPEND
#undef APPEND1
}

BOOL
SubstituteVars(
    IN PCTSTR Src,
    OUT PTSTR Dest,
    IN TCHAR Delim)
{
#define APPEND(From, Length) \
do { \
    if (Dest + (Length) > DestEnd) \
        goto too_long; \
    memcpy(Dest, (From), (Length) * sizeof(TCHAR)); \
    Dest += (Length); \
} while (0)

#define APPEND1(Char) \
do { \
    if (Dest >= DestEnd) \
        goto too_long; \
    *Dest++ = (Char); \
} while (0)

    PTCHAR DestEnd = Dest + CMDLINE_LENGTH - 1;
    PCTSTR End;
    size_t SrcIncLen, DestIncLen;

    while (*Src /* && (Dest < DestEnd) */)
    {
        if (*Src != Delim)
        {
            End = _tcschr(Src, Delim);
            if (End == NULL)
                End = Src + _tcslen(Src);
            APPEND(Src, End - Src);
            Src = End;
            continue;
        }

        if (!SubstituteVar(Src, &SrcIncLen, Dest, DestEnd, &DestIncLen, Delim))
        {
            return FALSE;
        }
        else
        {
            Src += SrcIncLen;
            Dest += DestIncLen;
        }
    }
    APPEND1(_T('\0'));
    return TRUE;

too_long:
    ConOutResPrintf(STRING_ALIAS_ERROR);
    nErrorLevel = 9023;
    return FALSE;

#undef APPEND
#undef APPEND1
}

/* Search the list of FOR contexts for a variable */
static BOOL
FindForVar(
    IN TCHAR Var,
    OUT PCTSTR* VarPtr,
    OUT BOOL* IsParam0)
{
    PFOR_CONTEXT Ctx;

    *VarPtr = NULL;
    *IsParam0 = FALSE;

    for (Ctx = fc; Ctx != NULL; Ctx = Ctx->prev)
    {
        if ((UINT)(Var - Ctx->firstvar) < Ctx->varcount)
        {
            *VarPtr = Ctx->values[Var - Ctx->firstvar];
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
SubstituteForVars(
    IN PCTSTR Src,
    OUT PTSTR Dest)
{
    PTCHAR DestEnd = &Dest[CMDLINE_LENGTH - 1];
    while (*Src)
    {
        if (Src[0] == _T('%'))
        {
            BOOL Dummy;
            PCTSTR End = &Src[2];
            PCTSTR Value = NULL;

            if (Src[1] == _T('~'))
                Value = GetEnhancedVar(&End, FindForVar);

            if (!Value && Src[1])
            {
                if (FindForVar(Src[1], &Value, &Dummy) && !Value)
                {
                    /* The variable is empty, return an empty string */
                    Value = _T("");
                }
            }

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

PTSTR
DoDelayedExpansion(
    IN PCTSTR Line)
{
    TCHAR Buf1[CMDLINE_LENGTH];
    TCHAR Buf2[CMDLINE_LENGTH];
    PTCHAR Src, Dst;
    PTCHAR DestEnd = Buf2 + CMDLINE_LENGTH - 1;
    size_t SrcIncLen, DestIncLen;

    /* First, substitute FOR variables */
    if (!SubstituteForVars(Line, Buf1))
        return NULL;

    if (!bDelayedExpansion || !_tcschr(Buf1, _T('!')))
        return cmd_dup(Buf1);

    /*
     * Delayed substitutions are not actually completely the same as
     * immediate substitutions. In particular, it is possible to escape
     * the exclamation point using the escape caret.
     */

    /*
     * Perform delayed expansion: expand variables around '!',
     * and reparse escape carets.
     */

#define APPEND1(Char) \
do { \
    if (Dst >= DestEnd) \
        goto too_long; \
    *Dst++ = (Char); \
} while (0)

    Src = Buf1;
    Dst = Buf2;
    while (*Src && (Src < &Buf1[CMDLINE_LENGTH]))
    {
        if (*Src == _T('^'))
        {
            ++Src;
            if (!*Src || !(Src < &Buf1[CMDLINE_LENGTH]))
                break;

            APPEND1(*Src++);
        }
        else if (*Src == _T('!'))
        {
            if (!SubstituteVar(Src, &SrcIncLen, Dst, DestEnd, &DestIncLen, _T('!')))
            {
                return NULL; // Got an error during parsing.
            }
            else
            {
                Src += SrcIncLen;
                Dst += DestIncLen;
            }
        }
        else
        {
            APPEND1(*Src++);
        }
        continue;
    }
    APPEND1(_T('\0'));

    return cmd_dup(Buf2);

too_long:
    ConOutResPrintf(STRING_ALIAS_ERROR);
    nErrorLevel = 9023;
    return NULL;

#undef APPEND1
}


/*
 * Do the prompt/input/process loop.
 */
BOOL
ReadLine(TCHAR *commandline, BOOL bMore)
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
                    ConOutChar(_T('\n'));
                PrintPrompt();
            }
        }

        if (!ReadCommand(readline, CMDLINE_LENGTH - 1))
        {
            bExit = TRUE;
            return FALSE;
        }

        if (readline[0] == _T('\0'))
            ConOutChar(_T('\n'));

        if (CheckCtrlBreak(BREAK_INPUT))
            return FALSE;

        if (readline[0] == _T('\0'))
            return FALSE;

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
ProcessInput(VOID)
{
    INT Ret = 0;
    PARSED_COMMAND *Cmd;

    while (!bCanExit || !bExit)
    {
        /* Reset the Ctrl-Break / Ctrl-C state */
        bCtrlBreak = FALSE;

        Cmd = ParseCommand(NULL);
        if (!Cmd)
            continue;

        Ret = ExecuteCommand(Cmd);
        FreeCommand(Cmd);
    }

    return Ret;
}


/*
 * Control-break handler.
 */
static BOOL
WINAPI
BreakHandler(IN DWORD dwCtrlType)
{
    DWORD dwWritten;
    INPUT_RECORD rec;

    if ((dwCtrlType != CTRL_C_EVENT) &&
        (dwCtrlType != CTRL_BREAK_EVENT))
    {
        return FALSE;
    }

    if (!TryEnterCriticalSection(&ChildProcessRunningLock))
    {
        /* Child process is running and will have received the control event */
        return TRUE;
    }
    else
    {
        LeaveCriticalSection(&ChildProcessRunningLock);
    }

    bCtrlBreak = TRUE;

    rec.EventType = KEY_EVENT;
    rec.Event.KeyEvent.bKeyDown = TRUE;
    rec.Event.KeyEvent.wRepeatCount = 1;
    rec.Event.KeyEvent.wVirtualKeyCode = _T('C');
    rec.Event.KeyEvent.wVirtualScanCode = _T('C') - 35;
    rec.Event.KeyEvent.uChar.AsciiChar = _T('C');
    rec.Event.KeyEvent.uChar.UnicodeChar = _T('C');
    rec.Event.KeyEvent.dwControlKeyState = RIGHT_CTRL_PRESSED;

    WriteConsoleInput(ConStreamGetOSHandle(StdIn),
                      &rec,
                      1,
                      &dwWritten);

    /* FIXME: Handle batch files */

    // ConOutPrintf(_T("^C"));

    return TRUE;
}


VOID AddBreakHandler(VOID)
{
    SetConsoleCtrlHandler(BreakHandler, TRUE);
}


VOID RemoveBreakHandler(VOID)
{
    SetConsoleCtrlHandler(BreakHandler, FALSE);
}


/*
 * Show commands and options that are available.
 */
#if 0
static VOID
ShowCommands(VOID)
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
LoadRegistrySettings(HKEY hKeyRoot)
{
    LONG lRet;
    HKEY hKey;
    DWORD dwType, len;
    /*
     * Buffer big enough to hold the string L"4294967295",
     * corresponding to the literal 0xFFFFFFFF (MAXULONG) in decimal.
     */
    DWORD Buffer[6];

    lRet = RegOpenKeyEx(hKeyRoot,
                        _T("Software\\Microsoft\\Command Processor"),
                        0,
                        KEY_QUERY_VALUE,
                        &hKey);
    if (lRet != ERROR_SUCCESS)
        return;

#ifdef INCLUDE_CMD_COLOR
    len = sizeof(Buffer);
    lRet = RegQueryValueEx(hKey,
                           _T("DefaultColor"),
                           NULL,
                           &dwType,
                           (LPBYTE)&Buffer,
                           &len);
    if (lRet == ERROR_SUCCESS)
    {
        /* Overwrite the default attributes */
        if (dwType == REG_DWORD)
            wDefColor = (WORD)*(PDWORD)Buffer;
        else if (dwType == REG_SZ)
            wDefColor = (WORD)_tcstol((PTSTR)Buffer, NULL, 0);
    }
    // else, use the default attributes retrieved before.
#endif

#if 0
    len = sizeof(Buffer);
    lRet = RegQueryValueEx(hKey,
                           _T("DisableUNCCheck"),
                           NULL,
                           &dwType,
                           (LPBYTE)&Buffer,
                           &len);
    if (lRet == ERROR_SUCCESS)
    {
        /* Overwrite the default setting */
        if (dwType == REG_DWORD)
            bDisableUNCCheck = !!*(PDWORD)Buffer;
        else if (dwType == REG_SZ)
            bDisableUNCCheck = (_ttol((PTSTR)Buffer) == 1);
    }
    // else, use the default setting set globally.
#endif

    len = sizeof(Buffer);
    lRet = RegQueryValueEx(hKey,
                           _T("DelayedExpansion"),
                           NULL,
                           &dwType,
                           (LPBYTE)&Buffer,
                           &len);
    if (lRet == ERROR_SUCCESS)
    {
        /* Overwrite the default setting */
        if (dwType == REG_DWORD)
            bDelayedExpansion = !!*(PDWORD)Buffer;
        else if (dwType == REG_SZ)
            bDelayedExpansion = (_ttol((PTSTR)Buffer) == 1);
    }
    // else, use the default setting set globally.

    len = sizeof(Buffer);
    lRet = RegQueryValueEx(hKey,
                           _T("EnableExtensions"),
                           NULL,
                           &dwType,
                           (LPBYTE)&Buffer,
                           &len);
    if (lRet == ERROR_SUCCESS)
    {
        /* Overwrite the default setting */
        if (dwType == REG_DWORD)
            bEnableExtensions = !!*(PDWORD)Buffer;
        else if (dwType == REG_SZ)
            bEnableExtensions = (_ttol((PTSTR)Buffer) == 1);
    }
    // else, use the default setting set globally.

    len = sizeof(Buffer);
    lRet = RegQueryValueEx(hKey,
                           _T("CompletionChar"),
                           NULL,
                           &dwType,
                           (LPBYTE)&Buffer,
                           &len);
    if (lRet == ERROR_SUCCESS)
    {
        /* Overwrite the default setting */
        if (dwType == REG_DWORD)
            AutoCompletionChar = (TCHAR)*(PDWORD)Buffer;
        else if (dwType == REG_SZ)
            AutoCompletionChar = (TCHAR)_tcstol((PTSTR)Buffer, NULL, 0);
    }
    // else, use the default setting set globally.

    /* Validity check */
    if (IS_COMPLETION_DISABLED(AutoCompletionChar))
    {
        /* Disable autocompletion */
        AutoCompletionChar = 0x20;
    }

    len = sizeof(Buffer);
    lRet = RegQueryValueEx(hKey,
                           _T("PathCompletionChar"),
                           NULL,
                           &dwType,
                           (LPBYTE)&Buffer,
                           &len);
    if (lRet == ERROR_SUCCESS)
    {
        /* Overwrite the default setting */
        if (dwType == REG_DWORD)
            PathCompletionChar = (TCHAR)*(PDWORD)Buffer;
        else if (dwType == REG_SZ)
            PathCompletionChar = (TCHAR)_tcstol((PTSTR)Buffer, NULL, 0);
    }
    // else, use the default setting set globally.

    /* Validity check */
    if (IS_COMPLETION_DISABLED(PathCompletionChar))
    {
        /* Disable autocompletion */
        PathCompletionChar = 0x20;
    }

    /* Adjust completion chars */
    if (PathCompletionChar >= 0x20 && AutoCompletionChar < 0x20)
        PathCompletionChar = AutoCompletionChar;
    else if (AutoCompletionChar >= 0x20 && PathCompletionChar < 0x20)
        AutoCompletionChar = PathCompletionChar;

    RegCloseKey(hKey);
}

static VOID
ExecuteAutoRunFile(HKEY hKeyRoot)
{
    LONG lRet;
    HKEY hKey;
    DWORD dwType, len;
    TCHAR AutoRun[2048];

    lRet = RegOpenKeyEx(hKeyRoot,
                        _T("Software\\Microsoft\\Command Processor"),
                        0,
                        KEY_QUERY_VALUE,
                        &hKey);
    if (lRet != ERROR_SUCCESS)
        return;

    len = sizeof(AutoRun);
    lRet = RegQueryValueEx(hKey,
                           _T("AutoRun"),
                           NULL,
                           &dwType,
                           (LPBYTE)&AutoRun,
                           &len);
    if ((lRet == ERROR_SUCCESS) && (dwType == REG_EXPAND_SZ || dwType == REG_SZ))
    {
        if (*AutoRun)
            ParseCommandLine(AutoRun);
    }

    RegCloseKey(hKey);
}

/* Get the command that comes after a /C or /K switch */
static VOID
GetCmdLineCommand(
    OUT LPTSTR commandline,
    IN LPCTSTR ptr,
    IN BOOL AlwaysStrip)
{
    TCHAR* LastQuote;

    while (_istspace(*ptr))
        ++ptr;

    /* Remove leading quote, find final quote */
    if (*ptr == _T('"') &&
        (LastQuote = _tcsrchr(++ptr, _T('"'))) != NULL)
    {
        const TCHAR* Space;
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
        for (Space = ptr + 1; Space < LastQuote; ++Space)
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
 * Set up global initializations and process parameters.
 * Return a pointer to the command line if present.
 */
static LPCTSTR
Initialize(VOID)
{
    HMODULE NtDllModule;
    HANDLE hIn, hOut;
    LPTSTR ptr, cmdLine;
    TCHAR option = 0;
    BOOL AutoRun = TRUE;
    TCHAR ModuleName[MAX_PATH + 1];

    /* Get version information */
    InitOSVersion();

    /* Some people like to run ReactOS cmd.exe on Win98, it helps in the
     * build process. So don't link implicitly against ntdll.dll, load it
     * dynamically instead */
    NtDllModule = GetModuleHandle(TEXT("ntdll.dll"));
    if (NtDllModule != NULL)
    {
        NtQueryInformationProcessPtr = (NtQueryInformationProcessProc)GetProcAddress(NtDllModule, "NtQueryInformationProcess");
        NtReadVirtualMemoryPtr = (NtReadVirtualMemoryProc)GetProcAddress(NtDllModule, "NtReadVirtualMemory");
    }

    /* Load the registry settings */
    LoadRegistrySettings(HKEY_LOCAL_MACHINE);
    LoadRegistrySettings(HKEY_CURRENT_USER);

    /* Initialize our locale */
    InitLocale();

    /* Initialize prompt support */
    InitPrompt();

#ifdef FEATURE_DIRECTORY_STACK
    /* Initialize directory stack */
    InitDirectoryStack();
#endif

#ifdef FEATURE_HISTORY
    /* Initialize history */
    InitHistory();
#endif

    /* Set COMSPEC environment variable */
    if (GetModuleFileName(NULL, ModuleName, ARRAYSIZE(ModuleName)) != 0)
    {
        ModuleName[MAX_PATH] = _T('\0');
        SetEnvironmentVariable (_T("COMSPEC"), ModuleName);
    }

    /* Add ctrl break handler */
    AddBreakHandler();

    /* Set the default console mode */
    hOut = ConStreamGetOSHandle(StdOut);
    hIn  = ConStreamGetOSHandle(StdIn);
    SetConsoleMode(hOut, 0); // Reinitialize the console output mode
    SetConsoleMode(hOut, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
    SetConsoleMode(hIn , ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);

    cmdLine = GetCommandLine();
    TRACE ("[command args: %s]\n", debugstr_aw(cmdLine));

    for (ptr = cmdLine; *ptr; ++ptr)
    {
        if (*ptr == _T('/'))
        {
            option = _totupper(ptr[1]);
            if (option == _T('?'))
            {
                ConOutResPaging(TRUE, STRING_CMD_HELP8);
                nErrorLevel = 1;
                bExit = TRUE;
                return NULL;
            }
            else if (option == _T('P'))
            {
                if (!IsExistingFile(_T("\\autoexec.bat")))
                {
#ifdef INCLUDE_CMD_DATE
                    cmd_date(_T(""));
#endif
#ifdef INCLUDE_CMD_TIME
                    cmd_time(_T(""));
#endif
                }
                else
                {
                    ParseCommandLine(_T("\\autoexec.bat"));
                }
                bCanExit = FALSE;
            }
            else if (option == _T('A'))
            {
                OutputStreamMode = AnsiText;
            }
            else if (option == _T('C') || option == _T('K') || option == _T('R'))
            {
                /* Remainder of command line is a command to be run */
                fSingleCommand = ((option == _T('K')) << 1) | 1;
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
                bAlwaysStrip = TRUE;
            }
#ifdef INCLUDE_CMD_COLOR
            else if (!_tcsnicmp(ptr, _T("/T:"), 3))
            {
                /* Process /T (color) argument; overwrite any previous settings */
                wDefColor = (WORD)_tcstoul(&ptr[3], &ptr, 16);
            }
#endif
            else if (option == _T('U'))
            {
                OutputStreamMode = UTF16Text;
            }
            else if (option == _T('V'))
            {
                // FIXME: Check validity of the parameter given to V !
                bDelayedExpansion = _tcsnicmp(&ptr[2], _T(":OFF"), 4);
            }
            else if (option == _T('E'))
            {
                // FIXME: Check validity of the parameter given to E !
                bEnableExtensions = _tcsnicmp(&ptr[2], _T(":OFF"), 4);
            }
            else if (option == _T('X'))
            {
                /* '/X' is identical to '/E:ON' */
                bEnableExtensions = TRUE;
            }
            else if (option == _T('Y'))
            {
                /* '/Y' is identical to '/E:OFF' */
                bEnableExtensions = FALSE;
            }
        }
    }

#ifdef INCLUDE_CMD_COLOR
    if (wDefColor == 0)
    {
        /*
         * If we still do not have the console colour attribute set,
         * retrieve the default one.
         */
        ConGetDefaultAttributes(&wDefColor);
    }

    if (wDefColor != 0)
        ConSetScreenColor(ConStreamGetOSHandle(StdOut), wDefColor, TRUE);
#endif

    /* Reset the output Standard Streams translation modes and code page caches */
    // ConStreamSetMode(StdIn , OutputStreamMode, InputCodePage );
    ConStreamSetMode(StdOut, OutputStreamMode, OutputCodePage);
    ConStreamSetMode(StdErr, OutputStreamMode, OutputCodePage);

    if (!*ptr)
    {
        /* If neither /C or /K was given, display a simple version string */
        ConOutChar(_T('\n'));
        ConOutResPrintf(STRING_REACTOS_VERSION,
                        _T(KERNEL_VERSION_STR),
                        _T(KERNEL_VERSION_BUILD_STR));
        ConOutPuts(_T("(C) Copyright 1998-") _T(COPYRIGHT_YEAR) _T(" ReactOS Team.\n"));
    }

    if (AutoRun)
    {
        ExecuteAutoRunFile(HKEY_LOCAL_MACHINE);
        ExecuteAutoRunFile(HKEY_CURRENT_USER);
    }

    /* Returns the rest of the command line */
    return ptr;
}


static VOID Cleanup(VOID)
{
    /* Run cmdexit.bat */
    if (IsExistingFile(_T("cmdexit.bat")))
    {
        ConErrResPuts(STRING_CMD_ERROR5);
        ParseCommandLine(_T("cmdexit.bat"));
    }
    else if (IsExistingFile(_T("\\cmdexit.bat")))
    {
        ConErrResPuts(STRING_CMD_ERROR5);
        ParseCommandLine(_T("\\cmdexit.bat"));
    }

    /* Remove ctrl break handler */
    RemoveBreakHandler();

    /* Restore the default console mode */
    SetConsoleMode(ConStreamGetOSHandle(StdIn),
                   ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    SetConsoleMode(ConStreamGetOSHandle(StdOut),
                   ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);


#ifdef _DEBUG_MEM
#ifdef FEATURE_DIRECTORY_STACK
    /* Destroy directory stack */
    DestroyDirectoryStack();
#endif

#ifdef FEATURE_HISTORY
    CleanHistory();
#endif

    /* Free GetEnvVar's buffer */
    GetEnvVar(NULL);
#endif /* _DEBUG_MEM */

    DeleteCriticalSection(&ChildProcessRunningLock);
}

/*
 * main function
 */
int _tmain(int argc, const TCHAR *argv[])
{
    INT nExitCode;
    LPCTSTR pCmdLine;
    TCHAR startPath[MAX_PATH];

    InitializeCriticalSection(&ChildProcessRunningLock);
    lpOriginalEnvironment = DuplicateEnvironment();

    GetCurrentDirectory(ARRAYSIZE(startPath), startPath);
    _tchdir(startPath);

    SetFileApisToOEM();
    InputCodePage  = GetConsoleCP();
    OutputCodePage = GetConsoleOutputCP();

    /* Initialize the Console Standard Streams */
    ConStreamInit(StdIn , GetStdHandle(STD_INPUT_HANDLE) , /*OutputStreamMode*/ AnsiText, InputCodePage);
    ConStreamInit(StdOut, GetStdHandle(STD_OUTPUT_HANDLE), OutputStreamMode, OutputCodePage);
    ConStreamInit(StdErr, GetStdHandle(STD_ERROR_HANDLE) , OutputStreamMode, OutputCodePage);
    /* Reset the current thread UI language */
    if (IsConsoleHandle(ConStreamGetOSHandle(StdOut)) ||
        IsConsoleHandle(ConStreamGetOSHandle(StdErr)))
    {
        ConSetThreadUILanguage(0);
    }

    CMD_ModuleHandle = GetModuleHandle(NULL);

    /*
     * Perform general initialization, parse switches on command-line.
     * Initialize the exit code with the errorlevel as Initialize() can set it.
     */
    pCmdLine = Initialize();
    nExitCode = nErrorLevel;

    if (pCmdLine && *pCmdLine)
    {
        TCHAR commandline[CMDLINE_LENGTH];

        /* Do the /C or /K command */
        GetCmdLineCommand(commandline, &pCmdLine[2], bAlwaysStrip);
        nExitCode = ParseCommandLine(commandline);
        if (fSingleCommand == 1)
        {
            // nErrorLevel = nExitCode;
            bExit = TRUE;
        }
        fSingleCommand = 0;
    }
    if (!bExit)
    {
        /* Call prompt routine */
        nExitCode = ProcessInput();
    }

    /* Do the cleanup */
    Cleanup();
    cmd_free(lpOriginalEnvironment);

    cmd_exit(nExitCode);
    return nExitCode;
}

/* EOF */
