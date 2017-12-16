/*
	vfdcmd.c

	Virtual Floppy Drive for Windows
	Driver control program (console version)

	Copyright (C) 2003-2008 Ken Kato
*/

#ifdef __cplusplus
#pragma message(__FILE__": Compiled as C++ for testing purpose.")
#endif	// __cplusplus

#define WIN32_LEAN_AND_MEAN
#define _CRTDBG_MAP_ALLOC
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <crtdbg.h>

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif	// INVALID_FILE_ATTRIBUTES

#include "vfdtypes.h"
#include "vfdapi.h"
#include "vfdver.h"
#include "vfdmsg.h"

//
//	current driver state
//
static DWORD driver_state = VFD_NOT_INSTALLED;

//
//	interactive flag
//
static const char *help_progname = "VFD.EXE ";

//
//	command functions return value
//
#define VFD_OK	0
#define VFD_NG	1

//
//	operation mode
//
#define OPERATION_ASK	0		//	ask user on error
#define OPERATION_QUIT	1		//	quits on error
#define OPERATION_FORCE 2		//	force on error

//
//	invalid target number
//
#define TARGET_NONE		(ULONG)-1

//
//	command processing functions
//
typedef int (*cmdfnc)(const char **args);

static int Install(const char **args);
static int Remove(const char **args);
static int Config(const char **args);
static int Start(const char **args);
static int Stop(const char **args);
static int Shell(const char **args);
static int Open(const char **args);
static int Close(const char **args);
static int Save(const char **args);
static int Protect(const char **args);
static int Format(const char **args);
static int Link(const char **args);
static int Unlink(const char **args);
static int Status(const char **args);
static int Help(const char **args);
static int Version(const char **args);

//
//	Command table
//
static const struct {
	char	*cmd;				// command string
	int		max_args;			// maximum allowed number of argc
	cmdfnc	func;				// command processing function
	DWORD	hint;				// command hint message id
}
Commands[] = {
	{"INSTALL", 2, Install,	MSG_HINT_INSTALL},
	{"REMOVE",	1, Remove,	MSG_HINT_REMOVE	},
	{"CONFIG",	1, Config,	MSG_HINT_CONFIG	},
	{"START",	0, Start,	MSG_HINT_START	},
	{"STOP",	1, Stop,	MSG_HINT_STOP	},
	{"SHELL",	1, Shell,	MSG_HINT_SHELL	},
	{"OPEN",	6, Open,	MSG_HINT_OPEN	},
	{"CLOSE",	2, Close,	MSG_HINT_CLOSE	},
	{"SAVE",	3, Save,	MSG_HINT_SAVE,	},
	{"PROTECT", 2, Protect,	MSG_HINT_PROTECT},
	{"FORMAT",	2, Format,	MSG_HINT_FORMAT	},
	{"LINK",	3, Link,	MSG_HINT_LINK	},
	{"ULINK",	1, Unlink,	MSG_HINT_ULINK	},
	{"STATUS",	0, Status,	MSG_HINT_STATUS	},
	{"HELP",	1, Help,	MSG_HELP_HELP	},
	{"?",		1, Help,	MSG_HELP_HELP	},
	{"VERSION", 0, Version,	MSG_HINT_VERSION},
	{0, 0, 0, 0}
};

//
//	Help message table
//
static const struct {
	char	*keyword;			//	help keyword
	DWORD	help;				//	help message id
}
HelpMsg[] = {
	{"GENERAL", MSG_HELP_GENERAL},
	{"CONSOLE",	MSG_HELP_CONSOLE},
	{"INSTALL", MSG_HELP_INSTALL},
	{"REMOVE",	MSG_HELP_REMOVE	},
	{"CONFIG",	MSG_HELP_CONFIG	},
	{"START",	MSG_HELP_START	},
	{"STOP",	MSG_HELP_STOP	},
	{"SHELL",	MSG_HELP_SHELL	},
	{"OPEN",	MSG_HELP_OPEN	},
	{"CLOSE",	MSG_HELP_CLOSE	},
	{"SAVE",	MSG_HELP_SAVE	},
	{"PROTECT", MSG_HELP_PROTECT},
	{"FORMAT",	MSG_HELP_FORMAT	},
	{"LINK",	MSG_HELP_LINK	},
	{"ULINK",	MSG_HELP_ULINK	},
	{"STATUS",	MSG_HELP_STATUS	},
	{"HELP",	MSG_HELP_HELP	},
	{"VERSION", MSG_HINT_VERSION},
	{0, 0}
};

//
//	local functions
//
static int InteractiveConsole();
static int ProcessCommandLine(int argc, const char **args);
static int ParseCommand(const char *cmd);
static int ParseHelpTopic(const char *topic);
static int CheckDriver();
static int InputChar(ULONG msg, PCSTR ans);
static void PrintImageInfo(HANDLE hDevice);
static void PrintDriveLetter(HANDLE hDevice, ULONG nDrive);
static void PrintMessage(UINT msg, ...);
static BOOL ConsolePager(char *pBuffer, BOOL bReset);
static const char *SystemError(DWORD err);
static void ConvertPathCase(char *src, char *dst);

//
//	utility macro
//
#define IS_WINDOWS_NT()		((GetVersion() & 0xff) < 5)

//
//	main
//
int main(int argc, const char **argv)
{
#ifdef _DEBUG

	//	output vfd.exe command reference text

	if (*(argv + 1) && !_stricmp(*(argv + 1), "doc")) {
		int idx = 0;
		char *buf = "";

		printf("\r\n  VFD.EXE Command Reference\r\n");

		while (HelpMsg[idx].keyword) {
			int len = strlen(HelpMsg[idx].keyword);

			printf(
				"\r\n\r\n"
				"====================\r\n"
				"%*s\r\n"
				"====================\r\n"
				"\r\n",
				(20 + len) / 2, HelpMsg[idx].keyword);

			FormatMessage(
				FORMAT_MESSAGE_FROM_HMODULE |
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_ARGUMENT_ARRAY,
				NULL, HelpMsg[idx].help, 0,
				(LPTSTR)&buf, 0, (va_list *)&help_progname);
			
			printf("%s", buf);

			LocalFree(buf);

			idx++;
		}

		return 0;
	}
#endif

	//	Reports memory leaks at process termination

	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);

	//	Check the operating system version

	if (!VfdIsValidPlatform()) {
		PrintMessage(MSG_WRONG_PLATFORM);
		return VFD_NG;
	}

	if (argc < 2) {
		//	If no parameter is given, enter the interactive mode

		return InteractiveConsole();
	}
	else {
		//	Perform a single operation

		return ProcessCommandLine(argc - 1, argv + 1);
	}
}

//
//	VFD interactive console
//
int InteractiveConsole()
{
	char		input[1024];	//	user input buffer

	int			argc;			//	number of args in the user input
	char		*args[10];		//	args to pass to command functions

	char		sepa;			//	argument separator
	char		*p;				//	work pointer

	//	Disable the system default Ctrl+C handler

	SetConsoleCtrlHandler(NULL, TRUE);

	//	Set the console title

	SetConsoleTitle(VFD_PRODUCT_DESC);

	//	print version information and the console hint text

	Version(NULL);

	PrintMessage(MSG_CONSOLE_HINT);

	//	set interactive flag to exclude "VFD.EXE" from help text

	help_progname = "";

	//	process user input

	for (;;) {

		//	print the prompt

		printf("[VFD] ");
		fflush(stdout);

		//	read user input

		fflush(stdin);
		p = fgets(input, sizeof(input), stdin);

		if (p == NULL) {

			//	most likely <ctrl+c>

			printf("exit\n");
			break;
		}

		//	skip leading blank characters

		while (*p == ' ' || *p == '\t' || *p == '\n') {
			p++;
		}
		
		if (*p == '\0') {

			//	empty input

			continue;
		}

		//	handle external commands

		if (!_strnicmp(p, "dir", 3) ||
			!_strnicmp(p, "attrib", 6)) {

			//	special cases - frequently used commands
			//	pass these to system() even without '.'

			system(p);
			printf("\n");
			continue;
		}
		else if (*p == '.') {

			//	external command

			system(p + 1);
			printf("\n");
			continue;
		}

		//	split the input line into parameters (10 parameters max)

		argc = 0;
		ZeroMemory(args, sizeof(args));

		do {
			//	top of a parameter

			args[argc++] = p;

			//	is the parameter quoted?

			if (*p == '\"' || *p == '\'') {
				sepa = *(p++);
			}
			else {
				sepa = ' ';
			}

			//	search the end of the parameter

			while (*p && *p != '\n') {
				if (sepa == ' ') {
					if (*p == '\t' || *p == ' ') {
						break;			//	tail of a non-quoted parameter
					}
				}
				else {
					if (*p == sepa) {
						sepa = ' ';		//	close quote
					}
				}
				p++;
			}

			//	terminate the parameter

			if (*p) {
				*(p++) = '\0';
			}

			//	skip trailing blank characters

			while (*p == ' ' || *p == '\t' || *p == '\n') {
				p++;
			}

			if (*p == '\0') {

				//	end of the input line - no more args

				break;
			}
		}
		while (argc < sizeof(args) / sizeof(args[0]));

		//	check the first parameter for special commands

		if (!_stricmp(args[0], "exit") ||
			!_stricmp(args[0], "quit") ||
			!_stricmp(args[0], "bye")) {

			//	exit command

			break;
		}
		else if (!_stricmp(args[0], "cd") ||
			!_stricmp(args[0], "chdir")) {

			//	internal change directory command

			if (args[1]) {
				char path[MAX_PATH];
				int i;

				//	ignore the /d option (of the standard cd command)

				if (_stricmp(args[1], "/d")) {
					i = 1;
				}
				else {
					i = 2;
				}

				p = args[i];

				if (*p == '\"' || *p == '\'') {

					//	the parameter is quoted -- remove quotations

					p++;

					while (*p && *p != *args[i]) {
						p++;
					}

					args[i]++;		// skip a leading quote
					*p = '\0';		// remove a trailing quote
				}
				else {

					//	the parameter is not quoted
					//	-- concatenate params to allow spaces in unquoted path

					while (i < argc - 1) {
						*(args[i] + strlen(args[i])) = ' ';
						i++;
					}
				}

				//	Match the case of the path to the name on the disk
				
				ConvertPathCase(p, path);

				if (!SetCurrentDirectory(path)) {
					DWORD ret = GetLastError();

					if (ret == ERROR_FILE_NOT_FOUND) {
						ret = ERROR_PATH_NOT_FOUND;
					}

					printf("%s", SystemError(ret));
				}
			}
			else {
				if (!GetCurrentDirectory(sizeof(input), input)) {
					printf("%s", SystemError(GetLastError()));
				}
				else {
					printf("%s\n", input);
				}
			}
		}
		else if (isalpha(*args[0]) &&
			*(args[0] + 1) == ':' &&
			*(args[0] + 2) == '\0') {

			//	internal change drive command

			*args[0] = (char)toupper(*args[0]);
			*(args[0] + 2) = '\\';
			*(args[0] + 3) = '\0';

			if (!SetCurrentDirectory(args[0])) {
				printf("%s", SystemError(GetLastError()));
			}
		}
		else {

			//	perform the requested VFD command

			ProcessCommandLine(argc, (const char **)args);
		}

		printf("\n");
	}
	
	return VFD_OK;
}

//
//	process a single command
//
int ProcessCommandLine(int argc, const char **args)
{
	int		cmd;
	DWORD	ret;

	//
	//	Decide a command to perform
	//
	cmd = ParseCommand(*args);

	if (cmd < 0) {

		//	no matching command

		return VFD_NG;
	}

	if (*(++args) &&
		(!strcmp(*args, "/?") ||
		!_stricmp(*args, "/h"))) {

		//	print a short hint for the command

		PrintMessage(Commands[cmd].hint);
		return VFD_NG;
	}

	if (--argc > Commands[cmd].max_args) {

		// too many parameters for the command

		PrintMessage(MSG_TOO_MANY_ARGS);
		PrintMessage(Commands[cmd].hint);
		return VFD_NG;
	}

	//	Get the current driver state

	ret = VfdGetDriverState(&driver_state);

	if (ret != ERROR_SUCCESS) {
		PrintMessage(MSG_GET_STAT_NG);
		printf("%s", SystemError(ret));
		return VFD_NG;
	}

	//	Perform the requested operation

	return (*Commands[cmd].func)(args);
}

//
//	Install the Virtual Floppy Driver
//	Command Line Parameters:
//	(optional) driver file path	- default to executive's dir
//	(optional) auto start switch - default to demand start
//
int	Install(const char **args)
{
	const char *install_path = NULL;
	DWORD start_type = SERVICE_DEMAND_START;

	DWORD ret;

	//	process parameters

	while (args && *args) {

		if (!_stricmp(*args, "/a") ||
			!_stricmp(*args, "/auto")) {

			if (start_type != SERVICE_DEMAND_START) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}
/*
			if (IS_WINDOWS_NT()) {

				//	On Windows NT, SYSTEM start drivers must be placed
				//	under the winnt\system32 directory.  Since I don't
				//	care to handle driver file copying, I use the AUTO
				//	start method for Windows NT.

				start_type = SERVICE_AUTO_START;
			}
			else {

				//	On Windows XP, the VFD driver must be running when
				//	the shell starts -- otherwise the shell doesn't
				//	recognize the VFD drives.  Since Windows XP allows
				//	SYSTEM start drivers to be placed in any local
				//	directories, I use the SYSTEM start method here.
				//
				//	This is not an issue when the driver is started
				//	manually because in that case VFD.EXE and VFDWIN.EXE
				//	notify the shell of the VFD drives.
				//
				//	On Windows 2000 both SYSTEM and AUTO work fine.

				start_type = SERVICE_SYSTEM_START;
			}
*/
			//	On second thought -- Win2K / XP mount manager assigns
			//	arbitrary drive letters to all drives it finds during
			//	the system start up.  There is no way to prevent it
			//	until the driver is fully PnP compatible, so I'd settle
			//	for AUTO start for the time being.

			start_type = SERVICE_AUTO_START;
		}
		else if (**args == '/') {
			PrintMessage(MSG_UNKNOWN_OPTION, *args);
			PrintMessage(MSG_HINT_INSTALL, help_progname);
			return VFD_NG;
		}
		else {
			if (install_path) {
				PrintMessage(MSG_DUPLICATE_ARGS, "path");
				return VFD_NG;
			}

			install_path = *args;
		}

		args++;
	}

	//	already installed?

	if (driver_state != VFD_NOT_INSTALLED) {
		PrintMessage(MSG_DRIVER_EXISTS);
		return VFD_NG;
	}

	//	install the driver

	ret = VfdInstallDriver(
		install_path,
		start_type);

	if (ret != ERROR_SUCCESS) {
		PrintMessage(MSG_INSTALL_NG);
		printf("%s", SystemError(ret));
		return VFD_NG;
	}

	//	Get the latest driver state

	ret = VfdGetDriverState(&driver_state);

	if (ret != ERROR_SUCCESS) {
		PrintMessage(MSG_GET_STAT_NG);
		printf("%s", SystemError(ret));
		return VFD_NG;
	}

	//	operation successfull

	PrintMessage(MSG_INSTALL_OK);

	return VFD_OK;
}

//
//	Remove Virtual Floppy Driver from system
//	Command Line Parameters: 
//		[/F | /FORCE | /Q | /QUIT]
//		/F	forces remove operation if the driver cannot be stopped
//		/Q	quits remove operation if the driver cannot be stopped
//
int	Remove(const char **args)
{
	int			mode = OPERATION_ASK;
	const char	*stop_params[] = { NULL, NULL };
	DWORD		ret;
	int			idx;

	//	parse parameters

	while (args && *args) {

		if (!_stricmp(*args, "/f") ||
			!_stricmp(*args, "/force")) {

			if (mode != OPERATION_ASK) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			mode = OPERATION_FORCE;
			stop_params[0] = *args;
		}
		else if (!_stricmp(*args, "/q") ||
			!_stricmp(*args, "/quit")) {

			if (mode != OPERATION_ASK) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			mode = OPERATION_QUIT;
			stop_params[0] = *args;
		}
		else {
			PrintMessage(MSG_UNKNOWN_OPTION, *args);
			PrintMessage(MSG_HINT_REMOVE, help_progname);
			return VFD_NG;
		}

		args++;
	}

	//	ensure the driver is installed

	if (driver_state == VFD_NOT_INSTALLED) {
		PrintMessage(MSG_NOT_INSTALLED);
		return VFD_NG;
	}

	//	ensure the driver is stopped

	if (driver_state == SERVICE_RUNNING) {

		//	Try to stop with the same command line option (/F or /Q)

		while (Stop(stop_params) != VFD_OK) {

			//	stop failed

			if (mode == OPERATION_FORCE) {
				PrintMessage(MSG_REMOVE_FORCE);
				break;
			}
			else if (mode == OPERATION_QUIT) {
				PrintMessage(MSG_REMOVE_QUIT);
				return VFD_NG;
			}
			else {
				int c;

				PrintMessage(MSG_REMOVE_WARN);

				c = InputChar(MSG_RETRY_FORCE_CANCEL, "rfc");

				if (c == 'f') {			//	force
					break;
				}
				else if (c == 'c') {	//	cancel
					return VFD_NG;
				}
			}
		}
	}

	//	remove the driver

	ret = VfdRemoveDriver();

	if (ret != ERROR_SUCCESS) {
		PrintMessage(MSG_REMOVE_NG);
		printf("%s", SystemError(ret));
		return VFD_NG;
	}

	// Wait for the driver to be actually removed for 3 secs Max.

	for (idx = 0; idx < 10; idx++) {

		ret = VfdGetDriverState(&driver_state);

		if (ret != ERROR_SUCCESS) {
			PrintMessage(MSG_GET_STAT_NG);
			printf("%s", SystemError(ret));
			return VFD_NG;
		}

		if (driver_state == VFD_NOT_INSTALLED) {
			break;
		}

		Sleep(300);
	}

	if (driver_state != VFD_NOT_INSTALLED) {
		PrintMessage(MSG_REMOVE_PENDING);
		return VFD_NG;
	}

	//	operation successful

	PrintMessage(MSG_REMOVE_OK);

	return VFD_OK;
}

//
//	Configure the Virtual Floppy Driver
//	Command Line Parameters:
//	/auto, /manual
//
int	Config(const char **args)
{
	DWORD	start_type = SERVICE_DISABLED;
	DWORD	ret;

	while (args && *args) {
		if (!_stricmp(*args, "/a") ||
			!_stricmp(*args, "/auto")) {

			if (start_type != SERVICE_DISABLED) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			start_type = SERVICE_AUTO_START;
		}
		else if (!_stricmp(*args, "/m") ||
			!_stricmp(*args, "/manual")) {

			if (start_type != SERVICE_DISABLED) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			start_type = SERVICE_DEMAND_START;
		}
		else {
			PrintMessage(MSG_UNKNOWN_OPTION, *args);
			PrintMessage(MSG_HINT_CONFIG, help_progname);
			return VFD_NG;
		}

		args++;
	}

	if (start_type == SERVICE_DISABLED) {
		//	no parameter is specified
		PrintMessage(MSG_HINT_CONFIG, help_progname);
		return VFD_NG;
	}

	//	ensure that the driver is installed

	if (driver_state == VFD_NOT_INSTALLED) {
		PrintMessage(MSG_NOT_INSTALLED);
		return VFD_NG;
	}

	//	ensure that the driver is up to date

	if (CheckDriver() != VFD_OK) {
		return VFD_NG;
	}

	//	configure the driver

	ret = VfdConfigDriver(start_type);

	if (ret != ERROR_SUCCESS) {
		PrintMessage(MSG_CONFIG_NG);
		printf("%s", SystemError(ret));
		return VFD_NG;
	}

	//	operation successfull

	PrintMessage(MSG_CONFIG_OK);

	return VFD_OK;
}

//
//	Start the Virtual Floppy Driver
//	Command Line Parameters: None
//
int	Start(const char **args)
{
	DWORD	ret;

	UNREFERENCED_PARAMETER(args);

	//	ensure that the driver is installed

	if (driver_state == VFD_NOT_INSTALLED &&
		Install(NULL) != VFD_OK) {
		return VFD_NG;
	}

	//	ensure that the driver is up to date

	if (CheckDriver() != VFD_OK) {
		return VFD_NG;
	}

	//	ensure that the driver is not running

	if (driver_state == SERVICE_RUNNING) {
		PrintMessage(MSG_ALREADY_RUNNING);
		return VFD_NG;
	}

	//	start the driver

	ret = VfdStartDriver(&driver_state);

	if (ret != ERROR_SUCCESS) {
		PrintMessage(MSG_START_NG);
		printf("%s", SystemError(ret));
		return VFD_NG;
	}

	//	operation successfull

	PrintMessage(MSG_START_OK);

	return VFD_OK;
}

//
//	Stop the Virtual Floppy Driver
//	Command Line Parameters:
//		/FORCE | /F		Forces the operation on error
//		/QUIT  | /Q		Quits the operation on error
//
int	Stop(const char **args)
{
	int			mode = OPERATION_ASK;
	const char	*close_params[] = { "*", NULL, NULL };
	DWORD		ret;

	while (args && *args) {
		if (!_stricmp(*args, "/f") ||
			!_stricmp(*args, "/force")) {

			if (mode != OPERATION_ASK) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			mode = OPERATION_FORCE;

			//	parameter to pass to the Close() function
			close_params[1] = *args;
		}
		else if (!_stricmp(*args, "/q") ||
			!_stricmp(*args, "/quit")) {

			if (mode != OPERATION_ASK) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			mode = OPERATION_QUIT;

			//	parameter to pass to the Close() function
			close_params[1] = *args;
		}
		else {
			PrintMessage(MSG_UNKNOWN_OPTION, *args);
			PrintMessage(MSG_HINT_STOP, help_progname);
			return VFD_NG;
		}

		args++;
	}

	//	ensure that the driver is installed

	if (driver_state == VFD_NOT_INSTALLED) {
		PrintMessage(MSG_NOT_INSTALLED);
		return VFD_NG;
	}

	//	ensure that the driver is running

	if (driver_state == SERVICE_STOPPED) {
		PrintMessage(MSG_NOT_STARTED);
		return VFD_NG;
	}

	//	ensure that all drives are empty

	if (driver_state == SERVICE_RUNNING) {

		//	Try to close drives with the same operation mode (/F or /Q)

		while (Close(close_params) != VFD_OK) {

			//	close failed

			if (mode == OPERATION_FORCE) {
				PrintMessage(MSG_STOP_FORCE);
				break;
			}
			else if (mode == OPERATION_QUIT) {
				PrintMessage(MSG_STOP_QUIT);
				return VFD_NG;
			}
			else {
				int c;

				PrintMessage(MSG_STOP_WARN);

				c = InputChar(MSG_RETRY_FORCE_CANCEL, "rfc");

				if (c == 'f') {			//	force
					break;
				}
				else if (c == 'c') {	//	cancel
					return VFD_NG;
				}
			}
		}
	}

	//	stop the driver

	ret = VfdStopDriver(&driver_state);

	if (ret != ERROR_SUCCESS) {
		PrintMessage(MSG_STOP_NG);
		printf("%s", SystemError(ret));
		return VFD_NG;
	}

	if (driver_state != SERVICE_STOPPED) {
		PrintMessage(MSG_STOP_PENDING);
		return VFD_NG;
	}

	//	operation successful

	PrintMessage(MSG_STOP_OK);

	return VFD_OK;
}

//
//	Enable / Disable the shell extension
//	Command Line Parameters:
//	(optional) /ON or /OFF
//
int Shell(const char **args)
{
	DWORD ret;

	ret = VfdCheckHandlers();

	if (ret != ERROR_SUCCESS &&
		ret != ERROR_PATH_NOT_FOUND &&
		ret != ERROR_FILE_NOT_FOUND) {
		PrintMessage(MSG_GET_SHELLEXT_NG);
		printf("%s", SystemError(ret));
		return VFD_NG;
	}

	if (args && *args) {
		if (_stricmp(*args, "/on") == 0) {
			if (ret != ERROR_SUCCESS) {
				ret = VfdRegisterHandlers();

				if (ret != ERROR_SUCCESS) {
					PrintMessage(MSG_SET_SHELLEXT_NG);
					printf("%s", SystemError(ret));
					return VFD_NG;
				}
			}
		}
		else if (_stricmp(*args, "/off") == 0) {
			if (ret == ERROR_SUCCESS) {
				ret = VfdUnregisterHandlers();

				if (ret != ERROR_SUCCESS) {
					PrintMessage(MSG_SET_SHELLEXT_NG);
					printf("%s", SystemError(ret));
					return VFD_NG;
				}
			}
		}
		else {
			PrintMessage(MSG_UNKNOWN_OPTION, *args);
			PrintMessage(MSG_HINT_SHELL, help_progname);
			return VFD_NG;
		}

		ret = VfdCheckHandlers();
	}

	if (ret == ERROR_PATH_NOT_FOUND ||
		ret == ERROR_FILE_NOT_FOUND) {
		PrintMessage(MSG_SHELLEXT_DISABLED);
	}
	else if (ret == ERROR_SUCCESS) {
		PrintMessage(MSG_SHELLEXT_ENABLED);
	}
	else {
		PrintMessage(MSG_GET_SHELLEXT_NG);
		printf("%s", SystemError(ret));
		return VFD_NG;
	}

	return VFD_OK;
}

//
//	Open an image file to a Virtual Floppy Drive
//	Command Line Parameters:
//	[drive:] [file] [/NEW] [/RAM] [/P | /W]
//	[/size] [/media] [/F | /FORCE | /Q | /QUIT]

int	Open(const char **args)
{
	int				mode		= OPERATION_ASK;
	BOOL			create		= FALSE;
	ULONG			target		= TARGET_NONE;
	PCSTR			file_name	= NULL;
	VFD_DISKTYPE	disk_type	= VFD_DISKTYPE_FILE;
	CHAR			protect		= '\0';
	VFD_MEDIA		media_type	= VFD_MEDIA_NONE;
	BOOL			five_inch	= FALSE;
	VFD_FLAGS		media_flags	= 0;
	HANDLE			hDevice;
	CHAR			letter;
	DWORD			ret;

	//	process parameters

	while (args && *args) {

		if (!_stricmp(*args, "/f") ||
			!_stricmp(*args, "/force")) {

			if (mode != OPERATION_ASK) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			mode = OPERATION_FORCE;
		}
		else if (!_stricmp(*args, "/q") ||
			!_stricmp(*args, "/quit")) {

			if (mode != OPERATION_ASK) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			mode = OPERATION_QUIT;
		}

		else if (!_stricmp(*args, "/new")) {

			if (create) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			create = TRUE;
		}

		//	Disk type options

		else if (_stricmp(*args, "/ram") == 0) {

			if (disk_type != VFD_DISKTYPE_FILE) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			disk_type = VFD_DISKTYPE_RAM;
		}

		//	Protect options
		else if (_stricmp(*args, "/p") == 0 ||
			_stricmp(*args, "/w") == 0) {

			if (protect) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			protect = (CHAR)toupper(*(*args + 1));
		}

		//	media size options

		else if (strcmp(*args, "/160") == 0) {
			if (media_type) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			media_type = VFD_MEDIA_F5_160;
		}
		else if (strcmp(*args, "/180") == 0) {
			if (media_type) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			media_type = VFD_MEDIA_F5_180;
		}
		else if (strcmp(*args, "/320") == 0) {
			if (media_type) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			media_type = VFD_MEDIA_F5_320;
		}
		else if (strcmp(*args, "/360") == 0) {
			if (media_type) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			media_type = VFD_MEDIA_F5_360;
		}
		else if (strcmp(*args, "/640") == 0) {
			if (media_type) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			media_type = VFD_MEDIA_F3_640;
		}
		else if (strcmp(*args, "/720") == 0) {
			if (media_type) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			media_type = VFD_MEDIA_F3_720;
		}
		else if (strcmp(*args, "/820") == 0) {
			if (media_type) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			media_type = VFD_MEDIA_F3_820;
		}
		else if (strcmp(*args, "/120") == 0 ||
			strcmp(*args, "/1.20") == 0) {
			if (media_type) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			media_type = VFD_MEDIA_F3_1P2;
		}
		else if (strcmp(*args, "/144") == 0 ||
			strcmp(*args, "/1.44") == 0) {
			if (media_type) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			media_type = VFD_MEDIA_F3_1P4;
		}
		else if (strcmp(*args, "/168") == 0 ||
			strcmp(*args, "/1.68") == 0) {
			if (media_type) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			media_type = VFD_MEDIA_F3_1P6;
		}
		else if (strcmp(*args, "/172") == 0 ||
			strcmp(*args, "/1.72") == 0) {
			if (media_type) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			media_type = VFD_MEDIA_F3_1P7;
		}
		else if (strcmp(*args, "/288") == 0 ||
			strcmp(*args, "/2.88") == 0) {
			if (media_type) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			media_type = VFD_MEDIA_F3_2P8;
		}

		//	5.25 inch media

		else if (strcmp(*args, "/5") == 0 ||
			strcmp(*args, "/525") == 0 ||
			strcmp(*args, "/5.25") == 0) {

			if (five_inch) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			five_inch = TRUE;
		}

		//	target option

		else if (isalnum(**args) &&
			*(*args + 1) == ':' &&
			*(*args + 2) == '\0') {

			if (target != TARGET_NONE) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			target = toupper(**args);
		}

		//	filename

		else if (**args != '/') {
			if (file_name) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			file_name = *args;
		}
		else {
			PrintMessage(MSG_UNKNOWN_OPTION, *args);
			PrintMessage(MSG_HINT_OPEN, help_progname);
			return VFD_NG;
		}

		args++;
	}

	if (target == TARGET_NONE) {
		// default target
		target = '0';
		PrintMessage(MSG_TARGET_NOTICE, target);
	}

	//	check target file

	if (file_name) {
		DWORD			file_attr;
		VFD_FILETYPE	file_type;
		ULONG			image_size;
		BOOL			overwrite = FALSE;

		ret = VfdCheckImageFile(
			file_name, &file_attr, &file_type, &image_size);

		if (ret == ERROR_FILE_NOT_FOUND) {
			
			//	the target file does not exist
			
			if (!create) {				// create option not specified

				if (mode == OPERATION_FORCE) {
					PrintMessage(MSG_CREATE_NOTICE);
				}
				else {
					printf("%s", SystemError(ret));

					if (mode == OPERATION_QUIT ||
						InputChar(MSG_CREATE_CONFIRM, "yn") == 'n') {
						return VFD_NG;
					}
				}

				create = TRUE;
			}
		}
		else if (ret == ERROR_SUCCESS) {
			
			//	the target file exists

			if (create) {				//	create option is specified

				if (mode == OPERATION_FORCE) {
					PrintMessage(MSG_OVERWRITE_NOTICE);
				}
				else {
					printf("%s", SystemError(ERROR_FILE_EXISTS));

					if (mode == OPERATION_QUIT ||
						InputChar(MSG_OVERWRITE_CONFIRM, "yn") == 'n') {
						return VFD_NG;
					}
				}

				overwrite = TRUE;
			}
		}
		else {
			PrintMessage(MSG_OPEN_NG, file_name);
			printf("%s", SystemError(ret));
			return VFD_NG;
		}

		//
		//	create or overwrite the target file
		//

		if (create) {

			if (media_type == VFD_MEDIA_NONE) {

				if (mode == OPERATION_FORCE) {
					PrintMessage(MSG_CREATE144_NOTICE);
				}
				else {
					PrintMessage(MSG_FILE_MEDIA_UNKNOWN);

					if (mode == OPERATION_QUIT ||
						InputChar(MSG_CREATE144_CONFIRM, "yn") == 'n') {
						return VFD_NG;
					}
				}

				media_type = VFD_MEDIA_F3_1P4;
			}

			ret = VfdCreateImageFile(
				file_name, media_type, VFD_FILETYPE_RAW, overwrite);

			if (ret != ERROR_SUCCESS) {
				PrintMessage(MSG_CREATE_NG, file_name);
				printf("%s", SystemError(ret));
				return VFD_NG;
			}

			PrintMessage(MSG_FILE_CREATED);

			ret = VfdCheckImageFile(
				file_name, &file_attr, &file_type, &image_size);

			if (ret != ERROR_SUCCESS) {
				PrintMessage(MSG_OPEN_NG, file_name);
				printf("%s", SystemError(ret));
				return VFD_NG;
			}
		}
		else {
			//
			//	use the existing target file
			//	check image size and the media type
			//

			VFD_MEDIA	def_media;	//	default media for image size
			ULONG		media_size;	//	specified media size

			media_size	= VfdGetMediaSize(media_type);

			if (media_size > image_size) {

				//	specified media is too large for the image

				PrintMessage(MSG_IMAGE_TOO_SMALL);
				return VFD_NG;
			}

			def_media	= VfdLookupMedia(image_size);

			if (def_media == VFD_MEDIA_NONE) {

				//	image is too small for the smallest media

				PrintMessage(MSG_IMAGE_TOO_SMALL);
				return VFD_NG;
			}

			if (media_type == VFD_MEDIA_NONE) {

				//	media type is not specified

				ULONG def_size = VfdGetMediaSize(def_media);

				if (def_size != image_size) {

					//	image size does not match the largest media size

					PrintMessage(MSG_NO_MATCHING_MEDIA, image_size);

					if (mode == OPERATION_FORCE) {
						PrintMessage(MSG_MEDIATYPE_NOTICE, 
							VfdMediaTypeName(def_media), def_size);
					}
					else if (mode == OPERATION_QUIT) {
						return VFD_NG;
					}
					else {
						PrintMessage(MSG_MEDIATYPE_SUGGEST,
							VfdMediaTypeName(def_media), def_size);

						if (InputChar(MSG_MEDIATYPE_CONFIRM, "yn") == 'n') {
							return VFD_NG;
						}
					}
				}

				media_type = def_media;
			}
		}

		//	check file attributes against the disk type

		if (file_type == VFD_FILETYPE_ZIP ||
			(file_attr & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_ENCRYPTED))) {

			if (disk_type != VFD_DISKTYPE_RAM) {

				if (mode == OPERATION_FORCE) {
					PrintMessage(MSG_RAM_MODE_NOTICE);
				}
				else {
					PrintMessage(MSG_RAM_MODE_ONLY);

					if (mode == OPERATION_QUIT ||
						InputChar(MSG_RAM_MODE_CONFIRM, "yn") == 'n') {
						return VFD_NG;
					}
				}

				disk_type = VFD_DISKTYPE_RAM;
			}
		}

		if (disk_type != VFD_DISKTYPE_FILE) {
			if (!protect) {
				PrintMessage(MSG_DEFAULT_PROTECT);
				protect = 'P';
			}
		}
	}
	else {
		//
		//	pure RAM disk
		//
		disk_type = VFD_DISKTYPE_RAM;

		if (media_type == VFD_MEDIA_NONE) {

			if (mode == OPERATION_FORCE) {
				PrintMessage(MSG_CREATE144_NOTICE);
			}
			else {
				PrintMessage(MSG_RAM_MEDIA_UNKNOWN);

				if (mode == OPERATION_QUIT ||
					InputChar(MSG_CREATE144_CONFIRM, "yn") == 'n') {
					return VFD_NG;
				}
			}

			media_type = VFD_MEDIA_F3_1P4;
		}
	}

	if (protect == 'P') {
		media_flags |= VFD_FLAG_WRITE_PROTECTED;
	}

	if (five_inch &&
		VfdGetMediaSize(media_type) ==
		VfdGetMediaSize((VFD_MEDIA)(media_type + 1))) {
		media_type = (VFD_MEDIA)(media_type + 1);
	}

	//	ensure that the driver is installed

	if (driver_state == VFD_NOT_INSTALLED &&
		Install(NULL) != VFD_OK) {
		return VFD_NG;
	}

	//	ensure that the driver is up to date

	if (CheckDriver() != VFD_OK) {
		return VFD_NG;
	}

	//	ensure that the driver is running

	if (driver_state != SERVICE_RUNNING &&
		Start(NULL) != VFD_OK) {
		return VFD_NG;
	}

	//	Open the target device

	hDevice = VfdOpenDevice(target);

	if (hDevice == INVALID_HANDLE_VALUE) {
		ret = GetLastError();
		PrintMessage(MSG_ACCESS_NG, target);
		printf("%s", SystemError(ret));
		return VFD_NG;
	}

	//	Ensure that the drive is empty

	ret = VfdGetMediaState(hDevice);

	if (ret != ERROR_NOT_READY) {
		if (ret == ERROR_SUCCESS ||
			ret == ERROR_WRITE_PROTECT) {
			PrintMessage(MSG_DRIVE_BUSY);
		}
		else {
			PrintMessage(MSG_GET_MEDIA_NG);
			printf("%s", SystemError(ret));
		}

		CloseHandle(hDevice);
		return VFD_NG;
	}

	//	Open the image file

	ret = VfdOpenImage(hDevice, file_name,
		disk_type, media_type, media_flags);

	if (ret != ERROR_SUCCESS) {
		PrintMessage(MSG_OPEN_NG, file_name ? file_name : "<RAM>");
		printf("%s", SystemError(ret));

		CloseHandle(hDevice);
		return VFD_NG;
	}

	//	assign a drive letter if the drive has none

	VfdGetGlobalLink(hDevice, &letter);

	if (!isalpha(letter)) {
		VfdGetLocalLink(hDevice, &letter);
	}

	if (!isalpha(letter)) {
		VfdSetLocalLink(hDevice, VfdChooseLetter());
	}

	//	Get the actually opened image information.

	PrintImageInfo(hDevice);

	CloseHandle(hDevice);

	return VFD_OK;
}

//
//	Close the current virtual floppy image
//	Command Line Parameters:
//		drive number or drive letter
//		/F | /FORCE | /Q | /QUIT
//
int	Close(const char **args)
{
	ULONG			mode = OPERATION_ASK;

	ULONG			target_min = TARGET_NONE;
	ULONG			target_max = TARGET_NONE;
	HANDLE			hDevice;

	VFD_MEDIA		media_type;
	VFD_FLAGS		media_flags;

	DWORD			ret;

	//	check parameterS

	while (args && *args) {

		if (!_stricmp(*args, "/f") ||
			!_stricmp(*args, "/force")) {

			if (mode != OPERATION_ASK) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			mode = OPERATION_FORCE;
		}
		else if (!_stricmp(*args, "/q") ||
			!_stricmp(*args, "/quit")) {

			if (mode != OPERATION_ASK) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			mode = OPERATION_QUIT;
		}
		else if ((isalnum(**args) || **args == '*') &&
			(*(*args + 1) == ':' || *(*args + 1) == '\0')) {

			if (target_min != TARGET_NONE) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			if (**args == '*') {
				target_min = '0';
				target_max = '0' + VFD_MAXIMUM_DEVICES;
			}
			else {
				target_min = toupper(**args);
				target_max = target_min + 1;
			}
		}
		else {
			PrintMessage(MSG_UNKNOWN_OPTION, *args);
			PrintMessage(MSG_HINT_CLOSE, help_progname);
			return VFD_NG;
		}

		args++;
	}

	if (target_min == TARGET_NONE) {
		// default target = drive 0
		target_min = '0';
		target_max = '1';
		PrintMessage(MSG_TARGET_NOTICE, target_min);
	}

	//	ensure that the driver is installed

	if (driver_state == VFD_NOT_INSTALLED) {
		PrintMessage(MSG_NOT_INSTALLED);
		return VFD_NG;
	}

	//	ensure that the driver is running

	if (driver_state != SERVICE_RUNNING) {
		PrintMessage(MSG_NOT_STARTED);
		return VFD_NG;
	}

	//	Close the drive(s)

	while (target_min < target_max) {

		//	open the target device

		hDevice = VfdOpenDevice(target_min);

		if (hDevice == INVALID_HANDLE_VALUE) {
			ret = GetLastError();

			PrintMessage(MSG_ACCESS_NG, target_min);
			printf("%s", SystemError(ret));

			if (mode != OPERATION_FORCE) {
				return VFD_NG;
			}

			target_min++;
			continue;
		}

		//	get the current image information

		ret = VfdGetImageInfo(hDevice, NULL, NULL,
			&media_type, &media_flags, NULL, NULL);

		if (ret != ERROR_SUCCESS) {
			PrintMessage(MSG_ACCESS_NG, target_min);
			printf("%s", SystemError(ret));

			CloseHandle(hDevice);

			if (mode != OPERATION_FORCE) {
				return VFD_NG;
			}

			target_min++;
			continue;
		}

		if (media_type == VFD_MEDIA_NONE) {

			//	drive is empty

			CloseHandle(hDevice);
			target_min++;
			continue;
		}

		if (media_flags & VFD_FLAG_DATA_MODIFIED) {

			//	RAM disk data is modified

			PrintMessage(MSG_MEDIA_MODIFIED, target_min);

			if (mode == OPERATION_FORCE) {
				PrintMessage(MSG_CLOSE_FORCE);
			}
			else if (mode == OPERATION_QUIT) {
				PrintMessage(MSG_CLOSE_QUIT);
				CloseHandle(hDevice);
				return VFD_NG;
			}
			else {
				if (InputChar(MSG_CLOSE_CONFIRM, "yn") == 'n') {
					CloseHandle(hDevice);
					return VFD_NG;
				}
			}
		}

retry:
		ret = VfdCloseImage(
			hDevice, (mode == OPERATION_FORCE));
			
		if (ret == ERROR_ACCESS_DENIED) {

			PrintMessage(MSG_LOCK_NG, target_min);

			if (mode == OPERATION_QUIT) {
				CloseHandle(hDevice);
				return VFD_NG;
			}
			else if (mode == OPERATION_ASK) {

				int c;

				if (IS_WINDOWS_NT()) {
					c = InputChar(MSG_RETRY_CANCEL, "rc");
				}
				else {
					c = InputChar(MSG_RETRY_FORCE_CANCEL, "rfc");
				}

				if (c == 'f') {				//	force
					ret = VfdCloseImage(hDevice, TRUE);
				}
				else if (c == 'c') {		//	cancel
					CloseHandle(hDevice);
					return VFD_NG;
				}
				else {
					goto retry;
				}
			}
		}

		CloseHandle(hDevice);

		if (ret == ERROR_SUCCESS) {
			PrintMessage(MSG_CLOSE_OK, target_min);
		}
		else if (ret != ERROR_NOT_READY) {
			PrintMessage(MSG_CLOSE_NG, target_min);
			printf("%s", SystemError(ret));

			if (mode != OPERATION_FORCE) {
				return VFD_NG;
			}
		}

		target_min++;
	}

	return VFD_OK;
}

//
//	Save the current image into a file
//
int	Save(const char **args)
{
	int				mode		= OPERATION_ASK;
	ULONG			target		= TARGET_NONE;
	CHAR			file_name[MAX_PATH]	= {0};
	BOOL			overwrite	= FALSE;
	BOOL			truncate	= FALSE;

	HANDLE			hDevice;
	CHAR			current[MAX_PATH] = {0};
	VFD_MEDIA		media_type;
	VFD_FLAGS		media_flags;
	VFD_FILETYPE	file_type;
	DWORD			file_attr;
	ULONG			image_size;
	DWORD			ret;

	//	check parameters

	while (args && *args) {

		if (!_stricmp(*args, "/f") ||
			!_stricmp(*args, "/force")) {

			if (mode != OPERATION_ASK) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			mode = OPERATION_FORCE;
		}
		else if (!_stricmp(*args, "/q") ||
			!_stricmp(*args, "/quit")) {

			if (mode != OPERATION_ASK) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			mode = OPERATION_QUIT;
		}
		else if (!_stricmp(*args, "/o") ||
			!_stricmp(*args, "/over")) {

			if (truncate || overwrite) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			overwrite = TRUE;
		}
		else if (!_stricmp(*args, "/t") ||
			!_stricmp(*args, "/trunc")) {

			if (truncate || overwrite) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			truncate = TRUE;
		}
		else if (isalnum(**args) &&
			*(*args + 1) == ':' &&
			*(*args + 2) == '\0') {

			if (target != TARGET_NONE) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			target = toupper(**args);
		}
		else if (**args == '/') {
			PrintMessage(MSG_UNKNOWN_OPTION, *args);
			PrintMessage(MSG_HINT_SAVE, help_progname);
			return VFD_NG;
		}
		else {
			strcpy(file_name, *args);
		}

		args++;
	}

	if (target == TARGET_NONE) {
		target = '0';
		PrintMessage(MSG_TARGET_NOTICE, target);
	}

	//	ensure that the driver is installed

	if (driver_state == VFD_NOT_INSTALLED) {
		PrintMessage(MSG_NOT_INSTALLED);
		return VFD_NG;
	}

	//	ensure that the driver is up to date

	if (CheckDriver() != VFD_OK) {
		return VFD_NG;
	}

	//	ensure that the driver is running

	if (driver_state != SERVICE_RUNNING) {
		PrintMessage(MSG_NOT_STARTED);
		return VFD_NG;
	}

	//	Open the target device

	hDevice = VfdOpenDevice(target);

	if (hDevice == INVALID_HANDLE_VALUE) {
		ret = GetLastError();
		PrintMessage(MSG_ACCESS_NG, target);
		printf("%s", SystemError(ret));
		return VFD_NG;
	}

	//	Get the current image info

	ret = VfdGetImageInfo(hDevice, current, NULL,
		&media_type, &media_flags, NULL, NULL);

	if (ret != ERROR_SUCCESS) {
		printf("%s", SystemError(ret));
		CloseHandle(hDevice);
		return VFD_NG;
	}

	if (media_type == VFD_MEDIA_NONE) {
		printf("%s", SystemError(ERROR_NOT_READY));
		CloseHandle(hDevice);
		return VFD_NG;
	}

	if (file_name[0] == '\0') {

		if (current[0] == '\0') {

			PrintMessage(MSG_TARGET_REQUIRED);
			CloseHandle(hDevice);

			return VFD_NG;
		}

		strcpy(file_name, current);
	}

	if (!_stricmp(file_name, current)) {

		//	target is the current image file

		if (!(media_flags & VFD_FLAG_DATA_MODIFIED)) {

			//	FILE disk (always up to date) or RAM disk is not modified

			PrintMessage(MSG_TARGET_UP_TO_DATE);
			CloseHandle(hDevice);

			return VFD_OK;
		}

		overwrite = TRUE;
	}

	//	check target file

	ret = VfdCheckImageFile(file_name,
		&file_attr, &file_type, &image_size);

	if (ret == ERROR_SUCCESS) {
		
		if (!overwrite && !truncate) {

			if (mode == OPERATION_FORCE) {
				PrintMessage(MSG_OVERWRITE_NOTICE);
				overwrite = TRUE;
			}
			else if (mode == OPERATION_QUIT) {
				printf("%s", SystemError(ERROR_FILE_EXISTS));
				CloseHandle(hDevice);

				return VFD_NG;
			}
			else {
				int c;

				printf("%s", SystemError(ERROR_FILE_EXISTS));

				c = InputChar(MSG_OVERWRITE_PROMPT, "otc");

				if (c == 'o') {
					overwrite = TRUE;
				}
				else if (c == 't') {
					truncate = TRUE;
				}
				else {
					CloseHandle(hDevice);
					return VFD_NG;
				}
			}
		}
	}
	else if (ret != ERROR_FILE_NOT_FOUND) {

		printf("%s", SystemError(ret));
		CloseHandle(hDevice);

		return VFD_NG;
	}

	if (file_type == VFD_FILETYPE_ZIP) {

		//	Cannot update a zip file

		PrintMessage(MSG_TARGET_IS_ZIP);
		CloseHandle(hDevice);

		return VFD_NG;
	}

retry:
	ret = VfdDismountVolume(
		hDevice, (mode == OPERATION_FORCE));

	if (ret == ERROR_ACCESS_DENIED) {

		PrintMessage(MSG_LOCK_NG, target);

		if (mode == OPERATION_FORCE) {
			PrintMessage(MSG_SAVE_FORCE);
		}
		else if (mode == OPERATION_QUIT) {
			PrintMessage(MSG_SAVE_QUIT);
			CloseHandle(hDevice);
			return VFD_NG;
		}
		else {
			int c = InputChar(MSG_RETRY_FORCE_CANCEL, "rfc");

			if (c == 'r') {			// retry
				goto retry;
			}
			else if (c == 'f') {	//	force
				VfdDismountVolume(hDevice, TRUE);
			}
			else {					//	cancel
				CloseHandle(hDevice);
				return VFD_NG;
			}
		}
	}
	else if (ret != ERROR_SUCCESS) {
		printf("%s", SystemError(ret));
		CloseHandle(hDevice);
		return VFD_NG;
	}

	ret = VfdSaveImage(hDevice, file_name,
		(overwrite || truncate), truncate);

	CloseHandle(hDevice);

	if (ret != ERROR_SUCCESS) {
		PrintMessage(MSG_SAVE_NG, target, file_name);
		printf("%s", SystemError(ret));

		return VFD_NG;
	}

	PrintMessage(MSG_SAVE_OK, target, file_name);

	return VFD_OK;
}

//
//	Enable/disable virtual media write protection
//
int	Protect(const char **args)
{
#define PROTECT_NONE	0
#define PROTECT_ON		1
#define PROTECT_OFF		2
	ULONG	protect	= PROTECT_NONE;
	ULONG	target	= TARGET_NONE;
	HANDLE	hDevice;
	DWORD	ret;

	//	check parameters

	while (args && *args) {

		//	Disk type options

		if (_stricmp(*args, "/on") == 0) {

			if (protect) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			protect = PROTECT_ON;
		}
		else if (_stricmp(*args, "/off") == 0) {

			if (protect) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			protect = PROTECT_OFF;
		}
		else if (isalnum(**args)) {
			
			if (target != TARGET_NONE) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			target = toupper(**args);
		}
		else {
			PrintMessage(MSG_UNKNOWN_OPTION, *args);
			PrintMessage(MSG_HINT_PROTECT, help_progname);
			return VFD_NG;
		}

		args++;
	}

	if (target == TARGET_NONE) {
		target = '0';
		PrintMessage(MSG_TARGET_NOTICE, target);
	}

	//	ensure that the driver is installed

	if (driver_state == VFD_NOT_INSTALLED) {
		PrintMessage(MSG_NOT_INSTALLED);
		return VFD_NG;
	}

	//	ensure that the driver is up to date

	if (CheckDriver() != VFD_OK) {
		return VFD_NG;
	}

	//	ensure that the driver is running

	if (driver_state != SERVICE_RUNNING) {
		PrintMessage(MSG_NOT_STARTED);
		return VFD_NG;
	}

	//	open the target drive

	hDevice = VfdOpenDevice(target);

	if (hDevice == INVALID_HANDLE_VALUE) {
		ret = GetLastError();
		PrintMessage(MSG_ACCESS_NG, target);
		printf("%s", SystemError(ret));
		return VFD_NG;
	}

	if (protect) {
		//	change protect state

		ret = VfdWriteProtect(
			hDevice, (protect == PROTECT_ON));

		if (ret != ERROR_SUCCESS) {
			PrintMessage(MSG_PROTECT_NG, target);
			printf("%s", SystemError(ret));

			CloseHandle(hDevice);
			return VFD_NG;
		}
	}

	//	get the current protect state

	ret = VfdGetMediaState(hDevice);

	CloseHandle(hDevice);

	if (ret == ERROR_SUCCESS) {
		PrintMessage(MSG_MEDIA_WRITABLE);
	}
	else if (ret == ERROR_WRITE_PROTECT) {
		PrintMessage(MSG_MEDIA_PROTECTED);
	}
	else {
		PrintMessage(MSG_GET_MEDIA_NG);
		printf("%s", SystemError(ret));
		return VFD_NG;
	}

	return VFD_OK;
}

//
//	Format the virtual media with FAT12
//
int	Format(const char **args)
{
	int		mode = OPERATION_ASK;
	ULONG	target	= TARGET_NONE;
	HANDLE	hDevice;
	DWORD	ret;

	//	check parameters

	while (args && *args) {

		if (!_stricmp(*args, "/f") ||
			!_stricmp(*args, "/force")) {

			if (mode != OPERATION_ASK) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			mode = OPERATION_FORCE;
		}
		else if (!_stricmp(*args, "/q") ||
			!_stricmp(*args, "/quit")) {

			if (mode != OPERATION_ASK) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			mode = OPERATION_QUIT;
		}
		else if (isalnum(**args)) {
			if (target != TARGET_NONE) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			target = toupper(**args);
		}
		else {
			PrintMessage(MSG_UNKNOWN_OPTION, *args);
			PrintMessage(MSG_HINT_FORMAT, help_progname);
			return VFD_NG;
		}

		args++;
	}

	if (target == TARGET_NONE) {
		target = '0';
		PrintMessage(MSG_TARGET_NOTICE, target);
	}

	//	ensure that the driver is installed

	if (driver_state == VFD_NOT_INSTALLED) {
		PrintMessage(MSG_NOT_INSTALLED);
		return VFD_NG;
	}

	//	ensure that the driver is up to date

	if (CheckDriver() != VFD_OK) {
		return VFD_NG;
	}

	//	ensure that the driver is running

	if (driver_state != SERVICE_RUNNING) {
		PrintMessage(MSG_NOT_STARTED);
		return VFD_NG;
	}

	//	Open the device

	hDevice = VfdOpenDevice(target);

	if (hDevice == INVALID_HANDLE_VALUE) {
		ret = GetLastError();
		PrintMessage(MSG_ACCESS_NG, target);
		printf("%s", SystemError(ret));
		return VFD_NG;
	}

	//	check if the media is writable

	ret = VfdGetMediaState(hDevice);

	if (ret != ERROR_SUCCESS) {
		PrintMessage(MSG_FORMAT_NG, target);
		printf("%s", SystemError(ret));

		CloseHandle(hDevice);
		return VFD_NG;
	}

	//	format the media

retry:
	ret = VfdDismountVolume(
		hDevice, (mode == OPERATION_FORCE));

	if (ret == ERROR_ACCESS_DENIED) {

		PrintMessage(MSG_LOCK_NG, target);

		if (mode == OPERATION_FORCE) {
			PrintMessage(MSG_FORMAT_FORCE);
		}
		else if (mode == OPERATION_QUIT) {
			PrintMessage(MSG_FORMAT_QUIT);
			CloseHandle(hDevice);
			return VFD_NG;
		}
		else {
			int c = InputChar(MSG_RETRY_FORCE_CANCEL, "rfc");

			if (c == 'r') {			// retry
				goto retry;
			}
			else if (c == 'f') {	//	force
				VfdDismountVolume(hDevice, TRUE);
			}
			else {					//	cancel
				CloseHandle(hDevice);
				return VFD_NG;
			}
		}
	}
	else if (ret != ERROR_SUCCESS) {
		PrintMessage(MSG_LOCK_NG, target);
		CloseHandle(hDevice);
		return VFD_NG;
	}

	ret = VfdFormatMedia(hDevice);

	CloseHandle(hDevice);

	if (ret != ERROR_SUCCESS) {
		PrintMessage(MSG_FORMAT_NG, target);
		printf("%s", SystemError(ret));
		return VFD_NG;
	}

	//	successful operation

	PrintMessage(MSG_FORMAT_OK);

	return VFD_OK;
}

//
//	Assign a drive letter to a Virtual Floppy Drive
//
int Link(const char **args)
{
	ULONG	target_min = TARGET_NONE;
	ULONG	target_max = TARGET_NONE;
	PCSTR	letters = NULL;
	BOOL	global	= TRUE;
	HANDLE	hDevice;
	DWORD	ret;

	while (args && *args) {
		if (!_stricmp(*args, "/g")) {
			global = TRUE;
		}
		else if (!_stricmp(*args, "/l")) {
			global = FALSE;
		}
		else if (isdigit(**args) || **args == '*') {
			if (target_min != TARGET_NONE) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			if (**args == '*') {
				target_min = '0';
				target_max = '0' + VFD_MAXIMUM_DEVICES;
			}
			else {
				target_min = **args;
				target_max = target_min + 1;
			}
		}
		else if (isalpha(**args)) {
			if (letters) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}
			letters = *args;
		}
		else {
			PrintMessage(MSG_UNKNOWN_OPTION, *args);
			PrintMessage(MSG_HINT_LINK, help_progname);
			return VFD_NG;
		}

		args++;
	}

	if (target_min == TARGET_NONE) {
		// default: drive 0
		target_min = '0';
		target_max = '1';
		PrintMessage(MSG_TARGET_NOTICE, target_min);
	}

	//	ensure that the driver is installed

	if (driver_state == VFD_NOT_INSTALLED) {
		PrintMessage(MSG_NOT_INSTALLED);
		return VFD_NG;
	}

	//	ensure that the driver is up to date

	if (CheckDriver() != VFD_OK) {
		return VFD_NG;
	}

	//	ensure that the driver is running

	if (driver_state != SERVICE_RUNNING) {
		PrintMessage(MSG_NOT_STARTED);
		return VFD_NG;
	}

	while (target_min < target_max) {
		ULONG number;
		CHAR letter;

		hDevice = VfdOpenDevice(target_min);

		if (hDevice == INVALID_HANDLE_VALUE) {
			ret = GetLastError();
			PrintMessage(MSG_ACCESS_NG, target_min);
			printf("%s", SystemError(ret));
			target_min++;
			continue;
		}

		ret = VfdGetDeviceNumber(hDevice, &number);

		if (ret != ERROR_SUCCESS) {
			PrintMessage(MSG_ACCESS_NG, target_min);
			printf("%s", SystemError(ret));
			CloseHandle(hDevice);
			target_min++;
			continue;
		}

		if (letters && isalpha(*letters)) {
			letter = (CHAR)toupper(*(letters++));
		}
		else {
			letter = VfdChooseLetter();
		}

		if (letter) {
			if (global) {
				ret = VfdSetGlobalLink(hDevice, letter);
			}
			else {
				ret = VfdSetLocalLink(hDevice, letter);
			}

			if (ret != ERROR_SUCCESS) {
				PrintMessage(MSG_LINK_NG, number, letter);
				printf("%s", SystemError(ret));
			}
		}
		else {
			PrintMessage(MSG_LINK_FULL);
		}

		PrintDriveLetter(hDevice, number);

		CloseHandle(hDevice);

		target_min++;
	}

	return VFD_OK;
}

//
//	Remove a drive letter from a Virtual Floppy Drive
//
int Unlink(const char **args)
{
	ULONG	target_min = TARGET_NONE;
	ULONG	target_max = TARGET_NONE;
	HANDLE	hDevice;
	DWORD	ret;

	while (args && *args) {
		if ((isalnum(**args) || **args == '*') &&
			(*(*args + 1) == ':' || *(*args + 1) == '\0')) {

			if (target_min != TARGET_NONE) {
				PrintMessage(MSG_DUPLICATE_ARGS, *args);
				return VFD_NG;
			}

			if (**args == '*') {
				target_min = '0';
				target_max = '0' + VFD_MAXIMUM_DEVICES;
			}
			else {
				target_min = **args;
				target_max = target_min + 1;
			}
		}
		else {
			PrintMessage(MSG_UNKNOWN_OPTION, *args);
			PrintMessage(MSG_HINT_ULINK, help_progname);
			return VFD_NG;
		}

		args++;
	}

	if (target_min == TARGET_NONE) {
		// default: drive 0
		target_min = '0';
		target_max = '1';
		PrintMessage(MSG_TARGET_NOTICE, target_min);
	}

	//	ensure that the driver is installed

	if (driver_state == VFD_NOT_INSTALLED) {
		PrintMessage(MSG_NOT_INSTALLED);
		return VFD_NG;
	}

	//	ensure that the driver is up to date

	if (CheckDriver() != VFD_OK) {
		return VFD_NG;
	}

	//	ensure that the driver is running

	if (driver_state != SERVICE_RUNNING) {
		PrintMessage(MSG_NOT_STARTED);
		return VFD_NG;
	}

	while (target_min < target_max) {
		ULONG number;

		hDevice = VfdOpenDevice(target_min);

		if (hDevice == INVALID_HANDLE_VALUE) {
			ret = GetLastError();
			PrintMessage(MSG_ACCESS_NG, target_min);
			printf("%s", SystemError(ret));
			target_min++;
			continue;
		}

		ret = VfdGetDeviceNumber(hDevice, &number);

		if (ret != ERROR_SUCCESS) {
			PrintMessage(MSG_ACCESS_NG, target_min);
			printf("%s", SystemError(ret));
			CloseHandle(hDevice);
			target_min++;
			continue;
		}

		VfdSetGlobalLink(hDevice, 0);
		VfdSetLocalLink(hDevice, 0);

		PrintDriveLetter(hDevice, number);

		CloseHandle(hDevice);

		target_min++;
	}

	return VFD_OK;
}

//
//	Print current driver state
//	Command Line Parameters: None
//
int	Status(const char **args)
{
	HANDLE	hDevice;
	TCHAR	path[MAX_PATH];
	DWORD	start_type;
	DWORD	version;
	ULONG	target;
	DWORD	ret;

	UNREFERENCED_PARAMETER(args);

	if (driver_state == VFD_NOT_INSTALLED) {
		PrintMessage(MSG_NOT_INSTALLED);
	}
	else {

		//	get current driver config

		ret = VfdGetDriverConfig(path, &start_type);

		if (ret != ERROR_SUCCESS) {
			PrintMessage(MSG_GET_CONFIG_NG);
			printf("%s", SystemError(ret));
			return VFD_NG;
		}

		//	print driver file path

		PrintMessage(MSG_DRIVER_FILE, path);

		//	print driver version
		version = 0;

		if (driver_state == SERVICE_RUNNING) {

			hDevice = VfdOpenDevice(0);

			if (hDevice != INVALID_HANDLE_VALUE) {
				ret = VfdGetDriverVersion(hDevice, &version);

				CloseHandle(hDevice);
			}

		}

		if (version == 0) {
			ret = VfdCheckDriverFile(path, &version);
		}

		if (ret == ERROR_SUCCESS) {
			PrintMessage(MSG_DRIVER_VERSION,
				HIWORD(version) & 0x7fff,
				LOWORD(version),
				(version & 0x80000000) ? "(debug)" : "");
		}
		else {
			PrintMessage(MSG_GET_VERSION_NG);
			printf("%s", SystemError(ret));
		}


		//	print driver start type

		PrintMessage(MSG_START_TYPE);

		switch (start_type) {
		case SERVICE_AUTO_START:
			PrintMessage(MSG_START_AUTO);
			break;

		case SERVICE_BOOT_START:
			PrintMessage(MSG_START_BOOT);
			break;

		case SERVICE_DEMAND_START:
			PrintMessage(MSG_START_DEMAND);
			break;

		case SERVICE_DISABLED:
			PrintMessage(MSG_START_DISABLED);
			break;

		case SERVICE_SYSTEM_START :
			PrintMessage(MSG_START_SYSTEM);
			break;

		default:
			PrintMessage(MSG_UNKNOWN_LONG, start_type);
			break;
		}

		//	print current driver state

		PrintMessage(MSG_DRIVER_STATUS);

		switch (driver_state) {
		case SERVICE_STOPPED:
			PrintMessage(MSG_STATUS_STOPPED);
			break;

		case SERVICE_START_PENDING:
			PrintMessage(MSG_STATUS_START_P);
			break;

		case SERVICE_STOP_PENDING:
			PrintMessage(MSG_STATUS_STOP_P);
			break;

		case SERVICE_RUNNING:
			PrintMessage(MSG_STATUS_RUNNING);
			break;

		case SERVICE_CONTINUE_PENDING:
			PrintMessage(MSG_STATUS_CONT_P);
			break;

		case SERVICE_PAUSE_PENDING:
			PrintMessage(MSG_STATUS_PAUSE_P);
			break;

		case SERVICE_PAUSED:
			PrintMessage(MSG_STATUS_PAUSED);
			break;

		default:
			PrintMessage(MSG_UNKNOWN_LONG, driver_state);
			break;
		}
	}

	//	print shell extension status

	printf("\n");

	if (VfdCheckHandlers() == ERROR_SUCCESS) {
		PrintMessage(MSG_SHELLEXT_ENABLED);
	}
	else {
		PrintMessage(MSG_SHELLEXT_DISABLED);
	}

	//	if driver is not running, no more info

	if (driver_state != SERVICE_RUNNING) {
		return VFD_OK;
	}

	//	print image information

	for (target = 0; target < VFD_MAXIMUM_DEVICES; target++) {
		HANDLE hDevice = VfdOpenDevice(target);

		if (hDevice == INVALID_HANDLE_VALUE) {
			ret = GetLastError();
			PrintMessage(MSG_ACCESS_NG, target + '0');
			printf("%s", SystemError(ret));
			return VFD_NG;
		}

		PrintImageInfo(hDevice);

		CloseHandle(hDevice);
	}

	return VFD_OK;
}

//
//	Print usage help
//
int	Help(const char **args)
{
	DWORD	msg = MSG_HELP_GENERAL;
	char	*buf = NULL;

	if (args && *args) {
		int cmd = ParseHelpTopic(*args);

		if (cmd < 0) {
			msg = MSG_HELP_HELP;
		}
		else {
			msg = HelpMsg[cmd].help;
		}
	}

	FormatMessage(
		FORMAT_MESSAGE_FROM_HMODULE |
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_ARGUMENT_ARRAY,
		NULL, msg, 0, (LPTSTR)&buf, 0,
		(va_list *)&help_progname);
	
	if (buf == NULL) {
		printf("%s", SystemError(GetLastError()));
		return VFD_NG;
	}

	ConsolePager(buf, TRUE);
	LocalFree(buf);

	return VFD_OK;
}

//
//	Print version information
//
int	Version(const char **args)
{
	UNREFERENCED_PARAMETER(args);

	printf(VFD_PRODUCT_DESC "\n" VFD_COPYRIGHT_STR "\n"
		"http://chitchat.at.infoseek.co.jp/vmware/vfd.html\n");

	return VFD_OK;
}

//
//	Parse command parameter
//
int ParseCommand(const char *cmd)
{
#define CMD_MATCH_NONE	-1
#define CMD_MATCH_MULTI	-2

	size_t len;
	int idx;
	int match;

	//	skip a leading '/'

	if (*cmd == '/') {
		cmd++;
	}

	if (*cmd == '\0') {

		//	empty command

		return CMD_MATCH_NONE;
	}

	//	find a match
	len = strlen(cmd);
	idx = 0;
	match = CMD_MATCH_NONE;

	while (Commands[idx].cmd) {

		if (strlen(Commands[idx].cmd) >= len &&
			!_strnicmp(cmd, Commands[idx].cmd, len)) {

			if (match == CMD_MATCH_NONE) {		//	first match
				match = idx;
			}
			else {								//	multiple matches
				if (match != CMD_MATCH_MULTI) {	//	first time
					PrintMessage(MSG_AMBIGUOUS_COMMAND, cmd);
					printf("> %s ", Commands[match].cmd);
					match = CMD_MATCH_MULTI;
				}

				printf("%s ", Commands[idx].cmd);
			}
		}

		idx++;
	}

	if (match == CMD_MATCH_NONE) {				//	match not found
		PrintMessage(MSG_UNKNOWN_COMMAND, cmd);
	}
	else if (match == CMD_MATCH_MULTI) {		//	multiple matches
		printf("\n");
	}

	return match;
}

int ParseHelpTopic(const char *topic)
{
	size_t	len;
	int		idx;
	int		match;

	if (*topic == '\0') {

		//	empty command

		return CMD_MATCH_NONE;
	}

	//	find a match
	len = strlen(topic);
	idx = 0;
	match = CMD_MATCH_NONE;

	while (HelpMsg[idx].keyword) {

		if (strlen(HelpMsg[idx].keyword) >= len &&
			!_strnicmp(topic, HelpMsg[idx].keyword, len)) {

			if (match == CMD_MATCH_NONE) {		//	first match
				match = idx;
			}
			else {								//	multiple matches
				if (match != CMD_MATCH_MULTI) {	//	first time
					PrintMessage(MSG_AMBIGUOUS_COMMAND, topic);
					printf("> %s ", HelpMsg[match].keyword);
					match = CMD_MATCH_MULTI;
				}

				printf("%s ", HelpMsg[idx].keyword);
			}
		}

		idx++;
	}

	if (match == CMD_MATCH_NONE) {				//	match not found
		PrintMessage(MSG_UNKNOWN_COMMAND, topic);
	}
	else if (match == CMD_MATCH_MULTI) {		//	multiple matches
		printf("\n");
	}

	return match;
}

//
//	Check driver version and update if necessary
//
int CheckDriver()
{
	char	path[MAX_PATH];
	DWORD	start;

	//	check installed driver file version

	if (VfdGetDriverConfig(path, &start) == ERROR_SUCCESS &&
		VfdCheckDriverFile(path, NULL) == ERROR_SUCCESS) {

		HANDLE hDevice;

		if (driver_state != SERVICE_RUNNING) {
			return VFD_OK;
		}

		//	check running driver version

		hDevice = VfdOpenDevice(0);

		if (hDevice != INVALID_HANDLE_VALUE) {
			CloseHandle(hDevice);
			return VFD_OK;
		}
	}

	PrintMessage(MSG_WRONG_DRIVER);
	return VFD_NG;
}

//
//	Print a prompt message and accept the reply input
//
int InputChar(ULONG msg, PCSTR ans)
{
	HANDLE			hStdIn;
	INPUT_RECORD	input;
	DWORD			result;
	int				reply;

	PrintMessage(msg);
	fflush(NULL);

	hStdIn	= GetStdHandle(STD_INPUT_HANDLE);

	FlushConsoleInputBuffer(hStdIn);

	for (;;) {
		ReadConsoleInput(hStdIn, &input, sizeof(input), &result);

		if (input.EventType == KEY_EVENT &&
			input.Event.KeyEvent.bKeyDown) {

			reply = tolower(input.Event.KeyEvent.uChar.AsciiChar);

			if (strchr(ans, reply)) {
				break;
			}
		}
	}

	printf("%c\n", reply);

	return reply;
}

//
//	Print image information on a Virtual Floppy Drive
//
void PrintImageInfo(
	HANDLE			hDevice)
{
	ULONG			device_number;
	CHAR			file_name[MAX_PATH];
	CHAR			file_desc[MAX_PATH];
	VFD_DISKTYPE	disk_type;
	VFD_MEDIA		media_type;
	VFD_FLAGS		media_flags;
	VFD_FILETYPE	file_type;
	ULONG			image_size;
	DWORD			ret;

	printf("\n");

	//	get current device number

	ret = VfdGetDeviceNumber(hDevice, &device_number);

	if (ret != ERROR_SUCCESS) {
		PrintMessage(MSG_GET_LINK_NG);
		printf("%s", SystemError(ret));
		device_number = (ULONG)-1;
	}

	//	get current drive letters

	PrintDriveLetter(hDevice, device_number);

	//	image file information

	ret = VfdGetImageInfo(hDevice, file_name, &disk_type,
		&media_type, &media_flags, &file_type, &image_size);

	if (ret != ERROR_SUCCESS) {
		PrintMessage(MSG_GET_FILE_NG);
		printf("%s", SystemError(ret));
		return;
	}

	//	print image file information
	if (media_type == VFD_MEDIA_NONE) {
		PrintMessage(MSG_IMAGE_NONE);
		return;
	}

	if (file_name[0]) {
		PrintMessage(MSG_IMAGE_NAME, file_name);

		VfdMakeFileDesc(file_desc, sizeof(file_desc),
			file_type, image_size, GetFileAttributes(file_name));
	}
	else {
		PrintMessage(MSG_IMAGE_NAME, "<RAM>");

		VfdMakeFileDesc(file_desc, sizeof(file_desc),
			VFD_FILETYPE_NONE, image_size, 0);
	}

	PrintMessage(MSG_FILE_DESC, file_desc);

	if (disk_type == VFD_DISKTYPE_FILE) {
		PrintMessage(MSG_DISKTYPE_FILE);
	}
	else {
		if (media_flags & VFD_FLAG_DATA_MODIFIED) {
			PrintMessage(MSG_DISKTYPE_RAM_DIRTY);
		}
		else {
			PrintMessage(MSG_DISKTYPE_RAM_CLEAN);
		}
	}

	//	print other file info

	PrintMessage(MSG_MEDIA_TYPE, VfdMediaTypeName(media_type));

	if (media_flags & VFD_FLAG_WRITE_PROTECTED) {
		PrintMessage(MSG_MEDIA_PROTECTED);
	}
	else {
		PrintMessage(MSG_MEDIA_WRITABLE);
	}
}

//
//	Print drive letters on a virtual floppy drive
//
void PrintDriveLetter(
	HANDLE			hDevice,
	ULONG			nDrive)
{
	CHAR			letter;

	PrintMessage(MSG_DRIVE_LETTER, nDrive);

	VfdGetGlobalLink(hDevice, &letter);

	if (isalpha(letter)) {
		PrintMessage(MSG_PERSISTENT, toupper(letter));
	}

	while (VfdGetLocalLink(hDevice, &letter) == ERROR_SUCCESS &&
		isalpha(letter)) {
		PrintMessage(MSG_EPHEMERAL, toupper(letter));
	}

	printf("\n");
}

//
//	Prints a text on screen a page a time
//
BOOL ConsolePager(char *pBuffer, BOOL bReset)
{
	static int	rows = 0;
	char		prompt[80];
	int			prompt_len = 0;
	HANDLE		hStdOut;
	HANDLE		hStdIn;

	//
	//	prepare the console input and output handles
	//
	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	hStdIn	= GetStdHandle(STD_INPUT_HANDLE);

	for (;;) {
		CONSOLE_SCREEN_BUFFER_INFO	info;
		INPUT_RECORD	input;
		DWORD			result;
		DWORD			mode;
		int				cols;
		char			*cur;
		char			save;

		//
		//	Get the current console screen information
		//
		GetConsoleScreenBufferInfo(hStdOut, &info);

		if (bReset || rows <= 0) {
			rows = info.srWindow.Bottom - info.srWindow.Top - 1;
		}

		cols = info.dwSize.X;

		//	console window is too small for paging

		if (rows <= 0) {
			//	print all text and exit
			printf("%s", pBuffer);
			break;
		}

		//
		//	find the tail of the text to be printed this time
		//
		cur = pBuffer;
		save = '\0';

		while (*cur) {
			if (*(cur++) == '\n' || (cols--) == 0) {
				//	reached the end of a line
				if (--rows == 0) {
					//	reached the end of a page
					//	insert a terminating NULL char
					save = *cur;
					*cur = '\0';
					break;
				}

				cols = info.dwSize.X;
			}
		}

		//	print the current page
		printf("%s", pBuffer);

		//	end of the whole text?
		if (save == '\0') {
			break;
		}

		//
		//	prompt for the next page
		//

		//	prepare the prompt text

		if (prompt_len == 0) {

			prompt_len = FormatMessage(
				FORMAT_MESSAGE_FROM_HMODULE |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, MSG_PAGER_PROMPT, 0,
				prompt, sizeof(prompt), NULL);

			if (prompt_len == 0) {
				strcpy(prompt, "Press any key to continue...");
				prompt_len = strlen(prompt);
			}
		}

		//	get the current console input mode

		GetConsoleMode(hStdIn, &mode);

		//	change the mode to receive Ctrl+C as a regular input

		SetConsoleMode(hStdIn, (mode & ~ENABLE_PROCESSED_INPUT));

		//	get the current cursor position

		GetConsoleScreenBufferInfo(hStdOut, &info);

		//	print the prompt text

		WriteConsoleOutputCharacter(hStdOut, prompt,
			prompt_len, info.dwCursorPosition, &result);

		//	reverse the text color

		FillConsoleOutputAttribute(hStdOut, 
			(WORD)(info.wAttributes | COMMON_LVB_REVERSE_VIDEO),
			prompt_len, info.dwCursorPosition, &result);

		//	move cursor to the end of the prompt text

		info.dwCursorPosition.X =
			(short)(info.dwCursorPosition.X + prompt_len);

		SetConsoleCursorPosition(hStdOut, info.dwCursorPosition);

		//	wait for a key press event

		FlushConsoleInputBuffer(hStdIn);

		do {
			ReadConsoleInput(hStdIn, &input, sizeof(input), &result);
		}
		while (input.EventType != KEY_EVENT ||
			!input.Event.KeyEvent.bKeyDown ||
			!input.Event.KeyEvent.uChar.AsciiChar);

		//	restore the original cursor position

		info.dwCursorPosition.X =
			(short)(info.dwCursorPosition.X - prompt_len);

		SetConsoleCursorPosition(hStdOut, info.dwCursorPosition);

		//	delete the prompt text

		FillConsoleOutputCharacter(hStdOut, ' ',
			prompt_len, info.dwCursorPosition, &result);

		//	restore the text attribute to norml

		FillConsoleOutputAttribute(hStdOut, info.wAttributes,
			prompt_len, info.dwCursorPosition, &result);

		//	restore the original console mode

		SetConsoleMode(hStdIn, mode);

		//	check if the input was 'q', <esc> or <Ctrl+C> ?

		if (input.Event.KeyEvent.uChar.AsciiChar == VK_CANCEL ||
			input.Event.KeyEvent.uChar.AsciiChar == VK_ESCAPE ||
			tolower(input.Event.KeyEvent.uChar.AsciiChar) == 'q') {

			//	cancelled by the user
			return FALSE;
		}

		//
		//	process the next page
		//
		*cur = save;
		pBuffer = cur;
	}

	return TRUE;
}

//
//	Format and print a message text
//
void PrintMessage(UINT msg, ...)
{
	char *buf = NULL;
	va_list list;

	va_start(list, msg);

	if (FormatMessage(
		FORMAT_MESSAGE_FROM_HMODULE |
		FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL, msg, 0, (LPTSTR)&buf, 0, &list)) {

		printf("%s", buf);
	}
	else {
		printf("Unknown Message ID %u\n", msg);
	}

	va_end(list);

	if (buf) {
		LocalFree(buf);
	}
}

//
//	Return a system error message text
//
const char *SystemError(DWORD err)
{
	static char msg[256];

	if (!FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, err, 0, msg, sizeof(msg), NULL)) {
#ifndef __REACTOS__
		sprintf(msg, "Unknown system error %lu (0x%08x)\n", err, err);
#else
		sprintf(msg, "Unknown system error %lu (0x%08lx)\n", err, err);
#endif
	}

	return msg;
}

//
//	Convert a path to match the case of names on the disk
//
void ConvertPathCase(char *src, char *dst)
{
	HANDLE hFind;
	WIN32_FIND_DATA find;
	char *p;

	p = dst;

	if (*src == '\"') {
		src++;
	}

	if (*(src + strlen(src) - 1) == '\"') {
		*(src + strlen(src) - 1) = '\0';
	}

	//
	//	handle drive / remote server name
	//
	if (isalpha(*src) && *(src + 1) == ':') {

		//	drive name

		*(p++) = (char)toupper(*src);
		strcpy(p++, ":\\");

		src += 2;
	}
	else if (*src == '\\' || *src == '/') {

		//	absolute path or remote name

		if ((*(src + 1) == '\\' || *(src + 1) == '/') &&
			*(src + 2) && *(src + 2) != '\\' && *(src + 2) != '/') {

			//	remote path

			*(p++) = '\\';
			*(p++) = '\\';
			src += 2;

			while (*src && *src != '\\' && *src != '/') {
				*(p++) = *(src++);
			}
		}

		strcpy(p, "\\");
	}
	else {
		*p = '\0';
	}

	//	skip redundant '\'

	while (*src == '\\' || *src == '/') {
		src++;
	}

	//	process the path

	while (*src) {

		char *q = src;

		//	separate the next part

		while (*q && *q != '\\' && *q != '/') {
			q++;
		}

		if ((q - src) == 2 && !strncmp(src, "..", 2)) {
			//	parent dir - copy as it is
			if (p != dst) {
				*p++ = '\\';
			}

			strcpy(p, "..");
			p += 2;
		}
		else if ((q - src) > 1 || *src != '.') {
			//	path name other than "."
			if (p != dst) {
				*(p++) = '\\';
			}

			strncpy(p, src, (q - src));
			*(p + (q - src)) = '\0';

			hFind = FindFirstFile(dst, &find);

			if (hFind == INVALID_HANDLE_VALUE) {
				strcpy(p, src);
				break;
			}

			FindClose(hFind);

			strcpy(p, find.cFileName);
			p += strlen(p);
		}

		//	skip trailing '\'s

		while (*q == '\\' || *q == '/') {
			q++;
		}

		src = q;
	}
}
