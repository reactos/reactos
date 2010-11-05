/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS shutdown/logoff utility
 * FILE:            base/application/shutdown/shutdown.c
 * PURPOSE:         Initiate logoff, shutdown or reboot of the system
 */

#include "precomp.h"

// Print information about which commandline arguments the program accepts.
static void PrintUsage() {
	LPTSTR lpUsage = NULL;
	DWORD errLength; // error message length
	LPTSTR resMsg; // for error message in OEM symbols

	if( AllocAndLoadString( &lpUsage,
							GetModuleHandle(NULL),
							IDS_USAGE ) )
	{
		errLength = strlen(lpUsage) + 1;
		resMsg = (LPTSTR)LocalAlloc(LPTR, errLength * sizeof(TCHAR));
		CharToOemBuff(lpUsage, resMsg, errLength);

		_putts( resMsg );

		LocalFree(lpUsage);
		LocalFree(resMsg);
	}
}

struct CommandLineOptions {
	BOOL abort; // Not used yet
	BOOL force;
	BOOL logoff;
	BOOL restart;
	BOOL shutdown;
};

struct ExitOptions {
	// This flag is used to distinguish between a user-initiated LOGOFF (which has value 0)
	// and an underdetermined situation because user didn't give an argument to start Exit.
	BOOL shouldExit;
	// flags is the type of shutdown to do - EWX_LOGOFF, EWX_REBOOT, EWX_POWEROFF, etc..
	UINT flags;
	// reason is the System Shutdown Reason code. F.instance SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED.
	DWORD reason;
};

// Takes the commandline arguments, and creates a struct which matches the arguments supplied.
static struct CommandLineOptions ParseArguments(int argc, TCHAR *argv[])
{
	struct CommandLineOptions opts;
	int i;

	// Reset all flags in struct
	opts.abort = FALSE;
	opts.force = FALSE;
	opts.logoff = FALSE;
	opts.restart = FALSE;
	opts.shutdown = FALSE;

	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-' || argv[i][0] == '/')
		{
			switch(argv[i][1]) {
				case '?':
					PrintUsage();
					exit(0);
				case 'f':
				case 'F':
					opts.force = TRUE;
					break;
				case 'l':
				case 'L':
					opts.logoff = TRUE;
					break;
				case 'r':
				case 'R':
					opts.restart = TRUE;
					break;
				case 's':
				case 'S':
					opts.shutdown = TRUE;
					break;
				default:
					// Unknown arguments will exit program.
					PrintUsage();
					exit(0);
					break;
			}
		}
	}

	return opts;
}

// Converts the commandline arguments to flags used to shutdown computer
static struct ExitOptions ParseCommandLineOptionsToExitOptions(struct CommandLineOptions opts)
{
	struct ExitOptions exitOpts;
	exitOpts.shouldExit = TRUE;

	// Sets ONE of the exit type flags
	if (opts.logoff)
		exitOpts.flags = EWX_LOGOFF;
	else if (opts.restart)
		exitOpts.flags = EWX_REBOOT;
	else if(opts.shutdown)
		exitOpts.flags = EWX_POWEROFF;
	else
	{
		exitOpts.flags = 0;
		exitOpts.shouldExit = FALSE;
	}

	// Sets additional flags
	if (opts.force)
	{
		exitOpts.flags = exitOpts.flags | EWX_FORCE;

		// This makes sure that we log off, also if there is only the "-f" option specified.
		// The Windows shutdown utility does it the same way.
		exitOpts.shouldExit = TRUE;
	}

	// Reason for shutdown
	// Hardcoded to "Other (Planned)"
	exitOpts.reason = SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED;

	return exitOpts;
}

// Writes the last error as both text and error code to the console.
void DisplayLastError()
{
	int errorCode = GetLastError();
	LPTSTR lpMsgBuf = NULL;
	DWORD errLength; // error message length
	LPTSTR resMsg; // for error message in OEM symbols

	// Display the error message to the user
	errLength = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		errorCode,
		LANG_USER_DEFAULT,
		(LPTSTR) &lpMsgBuf,
		0,
		NULL) + 1;

	resMsg = (LPTSTR)LocalAlloc(LPTR, errLength * sizeof(TCHAR));
	CharToOemBuff(lpMsgBuf, resMsg, errLength);

	_ftprintf(stderr, resMsg);
	_ftprintf(stderr, _T("Error code: %d\n"), errorCode);

	LocalFree(lpMsgBuf);
	LocalFree(resMsg);
}

void EnableShutdownPrivileges()
{
	HANDLE token;
	TOKEN_PRIVILEGES privs;

	// Check to see if the choosen action is allowed by the user. Everyone can call LogOff, but only privilieged users can shutdown/restart etc.
	if (! OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token))
	{
		DisplayLastError();
		exit(1);
	}

	// Get LUID (Locally Unique Identifier) for the privilege we need
	if (!LookupPrivilegeValue(
			NULL, // system - NULL is localsystem
			SE_SHUTDOWN_NAME, // name of the privilege
			&privs.Privileges[0].Luid) // output
		)
	{
		DisplayLastError();
		exit(1);
	}
	// and give our current process (i.e. shutdown.exe) the privilege to shutdown the machine.
	privs.PrivilegeCount = 1;
	privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (AdjustTokenPrivileges(
			token,
			FALSE,
			&privs,
			0,
			(PTOKEN_PRIVILEGES)NULL, // previous state. Set to NULL, we don't care about previous state.
			NULL
			) == 0) // return value 0 means failure
		{
			DisplayLastError();
			exit(1);
		}
}

 // Main entry for program
int _tmain(int argc, TCHAR *argv[])
{
	struct CommandLineOptions opts;
	struct ExitOptions exitOpts;

	if (argc == 1) // i.e. no commandline arguments given
	{
		PrintUsage();
		exit(0);
	}

	opts = ParseArguments(argc, argv);
	exitOpts = ParseCommandLineOptionsToExitOptions(opts);

	// Perform the shutdown/restart etc. action
	if (exitOpts.shouldExit)
	{
		EnableShutdownPrivileges();

		if (!ExitWindowsEx(exitOpts.flags, exitOpts.reason))
		{
			DisplayLastError();
			exit(1);
		}
	}
	return 0;
}

// EOF
