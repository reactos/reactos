/* $Id: regnav.c,v 1.2 1999/05/28 19:49:46 ea Exp $
 * 
 * regnav.c
 * 
 * Copyright (c) 1998, 1999 Emanuele Aliberti
 * 
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; see the file COPYING.LIB. If
 * not, write to the Free Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 * ReactOS system registry console navigation tool.
 */
//#define UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <assert.h>
#include "win32err.h"

#define INPUT_BUFFER_SIZE	512
#define COMMAND_NOT_FOUND	NULL
#define CURRENT_PATH_SIZE	1024

LPCTSTR STR_HKEY_CLASSES_ROOT		= _TEXT("HKEY_CLASSES_ROOT");
LPCTSTR STR_HKEY_CURRENT_USER		= _TEXT("HKEY_CURRENT_USER");
LPCTSTR STR_HKEY_LOCAL_MACHINE		= _TEXT("HKEY_LOCAL_MACHINE");
LPCTSTR STR_HKEY_USERS			= _TEXT("HKEY_USERS");
LPCTSTR STR_HKEY_CURRENT_CONFIG		= _TEXT("HKEY_CURRENT_CONFIG");
LPCTSTR STR_HKEY_PERFORMANCE_DATA	= _TEXT("HKEY_PERFORMANCE_DATA");


LPTSTR	app_name = _TEXT("regnav");
LPCTSTR	app_ver = _TEXT("1.0.4");
HANDLE	CurrentWorkingKey = INVALID_HANDLE_VALUE;	/* \ */
TCHAR	CurrentPath [CURRENT_PATH_SIZE] = _TEXT("\\");
BOOL	Done = FALSE;
INT	LastExitCode = 0;


/* ===  COMMANDS  === */

#define CMDPROTOIF (int argc,LPTSTR argv[])
typedef int (*CommandFunction) CMDPROTOIF;
#define CMDPROTO(n) int n CMDPROTOIF


typedef
struct _COMMAND_DESCRIPTOR
{
	LPCTSTR		Name;
	LPCTSTR		ShortDescription;
	LPCTSTR		Usage;
	CommandFunction	Command;
	int		MinArgc;
	int		MaxArgc;
	
} COMMAND_DESCRIPTOR, * PCOMMAND_DESCRIPTOR;


CMDPROTO(cmd_ck);
CMDPROTO(cmd_exit);
CMDPROTO(cmd_help);
CMDPROTO(cmd_ls);
CMDPROTO(cmd_pwk);
CMDPROTO(cmd_ver);


COMMAND_DESCRIPTOR
CommandsTable [] =
{
 {
	_TEXT("ck"),
	_TEXT("Change the working key."),
	_TEXT("CK key\n\nChange the working key."),
	cmd_ck,
	1,
	2
 },
 {
	_TEXT("exit"),
	_TEXT("Terminate the application."),
	_TEXT("EXIT\n\nTerminate the application."),
	cmd_exit,
	1,
	1
 },
 {
	_TEXT("help"),
	_TEXT("Print this commands summary, or a command's synopsis."),
	_TEXT("HELP [command]\n\nPrint commands summary, or a command's synopsis."),
	cmd_help,
	1,
	2
 },
 {
	_TEXT("ls"),
	_TEXT("List a key's values and subkeys."),
	_TEXT("LS [key]\n\nList a key's values and subkeys."),
	cmd_ls,
	1,
	2
 },
 {
	_TEXT("pwk"),
	_TEXT("Print the current working key."),
	_TEXT("PWK\n\nPrint the current working key."),
	cmd_pwk,
	1,
	1
 },
 {
	_TEXT("ver"),
	_TEXT("Print version information."),
	_TEXT("VER\n\nPrint version information."),
	cmd_ver,
	1,
	1
 },
 /* End of array marker */
 { NULL }
};


/* ===  CMD MANAGER  === */


PCOMMAND_DESCRIPTOR
DecodeVerb( LPCTSTR Name )
{
	register int i;

	for (	i = 0;
		CommandsTable[i].Name;
		++i
		)
	{
		if (0 == lstrcmpi(CommandsTable[i].Name,Name))
		{
			return & CommandsTable[i];
		}
	}
	return COMMAND_NOT_FOUND;
}

/* === Visual key name manager */

typedef
struct _SPLIT_KEY_NAME
{
	TCHAR	Host [32];
	HANDLE	Hive;
	TCHAR	SubKey [_MAX_PATH];
	
} SPLIT_KEY_NAME, * PSPLIT_KEY_NAME;


PSPLIT_KEY_NAME
ParseKeyName(
	LPTSTR		KeyName,
	PSPLIT_KEY_NAME k
	)
{
	TCHAR *r = KeyName;
	TCHAR *w = NULL;
	TCHAR SystemKey [64];

	assert(r && k);
	ZeroMemory( k, sizeof (SPLIT_KEY_NAME) );
	k->Hive = INVALID_HANDLE_VALUE;
	/* HOST */
	if (r[0] == _TEXT('\\') && r[1] == _TEXT('\\'))
	{
		for (	r += 2, w = k->Host;
			(*r && (*r != _TEXT('\\')));
			++r
		) {
			*w++ = *r;
		}
		if (w) *w = _TEXT('\0');
	}
	/* SYSTEM KEY */
	if (*r == _TEXT('\\')) ++r;
	for (	w = SystemKey;
		(*r && (*r != _TEXT('\\')));
		++r
	) {
		*w++ = *r;
	}
	if (w) *w = _TEXT('\0');
	if (0 == lstrcmpi(STR_HKEY_CLASSES_ROOT, SystemKey))
	{
		k->Hive = HKEY_CLASSES_ROOT;
	}
	else if (0 == lstrcmpi(STR_HKEY_CURRENT_USER, SystemKey))
	{
		k->Hive = HKEY_CURRENT_USER;
	}
	else if (0 == lstrcmpi(STR_HKEY_LOCAL_MACHINE, SystemKey))
	{
		k->Hive = HKEY_LOCAL_MACHINE;
	}
	else if (0 == lstrcmpi(STR_HKEY_USERS, SystemKey))
	{
		k->Hive = HKEY_USERS;
	}
	else if (0 == lstrcmpi(STR_HKEY_CURRENT_CONFIG, SystemKey))
	{
		k->Hive = HKEY_CURRENT_CONFIG;
	}
	else if (0 == lstrcmpi(STR_HKEY_PERFORMANCE_DATA, SystemKey))
	{
		k->Hive = HKEY_PERFORMANCE_DATA;
	}
	/* SUBKEY */
	if (*r == _TEXT('\\')) ++r;
	for (	w = k->SubKey;
		(*r);
		++r
	) {
		*w++ = *r;
	}
	if (w) *w = _TEXT('\0');
	/* OK */
	return k;
}

/* === COMMANDS === */


/**********************************************************************
 *	ck
 *
 * DESCRIPTION
 *	Change the current working key.
 */
CMDPROTO(cmd_ck)
{
	LONG		rv;
	SPLIT_KEY_NAME	k;

	if (0 == lstrcmp(argv[1], _TEXT("..")))
	{
		_tprintf( _TEXT("Change to parent not implemented yet.\n") );
		return EXIT_FAILURE;
	}
	if (INVALID_HANDLE_VALUE != CurrentWorkingKey)
	{
		RegCloseKey(CurrentWorkingKey);
		CurrentWorkingKey = INVALID_HANDLE_VALUE;
	}
	if (NULL == ParseKeyName(argv[1], &k))
	{
		return EXIT_FAILURE;
	}
	rv = RegOpenKeyEx(
			k.Hive,				/* handle of open key */
			k.SubKey,			/* address of name of subkey to open */
			0,				/* reserved */
			(REGSAM) KEY_ENUMERATE_SUB_KEYS,/* security access mask */
			& CurrentWorkingKey		/* address of handle of open key */
			);
	if (ERROR_SUCCESS != rv)
	{
		PrintWin32Error(L"RegOpenKeyEx",GetLastError());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}


/**********************************************************************
 *	exit
 *
 * DESCRIPTION
 *	Terminate.
 */
CMDPROTO(cmd_exit)
{
	Done = TRUE;
	_tprintf( _TEXT("Quitting...\n") );
	return EXIT_SUCCESS;
}


/**********************************************************************
 *	help
 *
 * DESCRIPTION
 *	Print help.
 */
CMDPROTO(cmd_help)
{
	PCOMMAND_DESCRIPTOR cd = NULL;

	if (1 == argc)
	{
		int CommandIndex;

		for (	CommandIndex = 0;
			(CommandsTable[CommandIndex].Name);
			CommandIndex++
			)
		{
			_tprintf(
				_TEXT("%s\t%s\n"),
				CommandsTable[CommandIndex].Name,
				CommandsTable[CommandIndex].ShortDescription
				);			
		}
		return EXIT_SUCCESS;
	}
	if ((cd = DecodeVerb(argv[1])))
	{
		_tprintf(
			_TEXT("%s\n"),
			cd->Usage
			);
		return EXIT_SUCCESS;
	}
	_tprintf(
		_TEXT("Unknown help item \"%s\".\n"),
		argv[1]
		);
	return EXIT_FAILURE;
}


/**********************************************************************
 *	ls
 *
 * DESCRIPTION
 *	List a key.
 */
CMDPROTO(cmd_ls)
{
	LONG		rv;
	DWORD		dwIndexK = 0;
	DWORD		dwIndexV = 0;
	UCHAR		Name [256];
	DWORD		cbName;
	UCHAR		Class [256];
	DWORD		cbClass;
	UCHAR		Data [1024];
	DWORD		cbData;
	DWORD		Type;
	FILETIME	ft;
	SYSTEMTIME	st;

	/* _self is always present */
	_tprintf( _TEXT(".\\\n") );
	/* _root directory? */
	if (INVALID_HANDLE_VALUE == CurrentWorkingKey)
	{
		_tprintf(
			_TEXT("%s\\\n"),
			STR_HKEY_CLASSES_ROOT
			);
		_tprintf(
			_TEXT("%s\\\n"),
			STR_HKEY_CURRENT_USER
			);
		_tprintf(
			_TEXT("%s\\\n"),
			STR_HKEY_LOCAL_MACHINE
			);
		_tprintf(
			_TEXT("%s\\\n"),
			STR_HKEY_USERS
			);
		_tprintf(
			_TEXT("%s\\\n"),
			STR_HKEY_CURRENT_CONFIG
			);
		_tprintf(
			_TEXT("%s\\\n"),
			STR_HKEY_PERFORMANCE_DATA
			);
		return EXIT_SUCCESS;
	}
	/* _parent is present only if _self != _root 
	 * (FIXME: change it when RegConnect... available)
	 */
	_tprintf( _TEXT("..\\\n") );
	/* Enumerate subkeys of the current key. */
	do {
		cbName = sizeof(Name);
		cbClass = sizeof(Class);
		rv = RegEnumKeyEx(
			CurrentWorkingKey,	/* handle of key to enumerate */
			dwIndexK,		/* index of subkey to enumerate */
			Name,			/* address of buffer for subkey name */
			& cbName,		/* address for size of subkey buffer */
			NULL,			/* reserved */
			Class,			/* address of buffer for class string */
			& cbClass,		/* address for size of class buffer */
			& ft	 		/* address for time key last written to */
			);
		if (ERROR_SUCCESS == rv)
		{
			FileTimeToSystemTime( & ft, & st );
			if (cbClass)
				_tprintf(
					_TEXT("%-32s\\ %4d-%02d-%02d %02d:%02d [%s]\n"),
					Name,
					st.wYear, st.wMonth, st.wDay,
					st.wHour, st.wMinute,
					Class
					);
			else
				_tprintf(
					_TEXT("%-32s\\ %4d-%02d-%02d %02d:%02d\n"),
					Name,
					st.wYear, st.wMonth, st.wDay,
					st.wHour, st.wMinute
					);
			++dwIndexK;
		}
	} while (ERROR_SUCCESS == rv);
	/* Enumerate key's values */
	do {
		cbName = sizeof(Name);
		cbData = sizeof(Data);
		rv = RegEnumValue(
			CurrentWorkingKey,	/* handle of key to query */
			dwIndexV,		/* index of value to query */
			Name,			/* address of buffer for value string */
			& cbName,		/* address for size of value buffer */
			NULL,			/* reserved */
			& Type,			/* address of buffer for type code */
			Data,			/* address of buffer for value data */
			& cbData 		/* address for size of data buffer */
			);
		if (ERROR_SUCCESS == rv)
		{
			switch (Type)
			{
			case REG_DWORD:
				_tprintf( 
					_TEXT("%s = *REG_DWORD*\n"),
					Name
					);
				break;
			case REG_EXPAND_SZ:
				/* expand env vars */
				break;
			case REG_LINK:
				/* reparse! */
				break;
			case REG_SZ:
				_tprintf(
					_TEXT("%s = \"%s\"\n"),
					Name,
					Data
					);
				break;
			}
			++dwIndexV;
		}
	} while (ERROR_SUCCESS == rv);
	return (UINT) dwIndexK + (UINT) dwIndexV;
}


/**********************************************************************
 *	pwk
 *
 * DESCRIPTION
 *	Print the current working key.
 */
CMDPROTO(cmd_pwk)
{
	if (INVALID_HANDLE_VALUE == CurrentWorkingKey)
	{
		_tprintf( _TEXT("[\\]\n") );
		return EXIT_SUCCESS;
	}
	_tprintf(
		_TEXT("[%s]\n"),
		CurrentPath
		);
	return EXIT_SUCCESS;
}


/**********************************************************************
 *	ver
 *
 * DESCRIPTION
 *	Print version information.
 */
CMDPROTO(cmd_ver)
{
	_tprintf(
		_TEXT("\
%s version %s (compiled on %s, at %s)\n\
ReactOS Console Registry Navigator\n\
Copyright (c) 1998, 1999 Emanuele Aliberti\n\n"),
		app_name,
		app_ver,
		_TEXT(__DATE__),
		_TEXT(__TIME__)
		);
	return EXIT_SUCCESS;
}


/* === UTILITIES === */


#define ARGV_SIZE 32

INT
ParseCommandLine(
	LPTSTR	InputBuffer,
	LPTSTR	argv[]
	)
{
	register INT	n = 0;
	register TCHAR	*c = InputBuffer;
	
	assert(InputBuffer);
	do
	{
		for (	;
			(	*c
				&& (	(*c == _TEXT(' '))
					|| (*c == _TEXT('\t'))
					|| (*c == _TEXT('\n'))
					)
				);
			++c
			);
		argv[n++] = c;
		if (*c)
		{
			for (	;
				(	*c
					&& (*c != _TEXT(' '))
					&& (*c != _TEXT('\t'))
					&& (*c != _TEXT('\n'))
					);
				++c
				);
			*c++ = _TEXT('\0');
		}
	} while ( *c );
	return n;
}


VOID
DisplayPrompt(VOID)
{
	_tprintf(
		_TEXT("[%s] "),
		CurrentPath
		);
}


/* ===  MAIN  === */


int
main(
	int	argc,
	char *	argv []
	)
{
	TCHAR			InputBuffer [INPUT_BUFFER_SIZE];
	PCOMMAND_DESCRIPTOR	cd;
	INT			LocalArgc;
	LPTSTR			LocalArgv [ARGV_SIZE];

	
	while (!Done)
	{
		DisplayPrompt();
		_fgetts(
			InputBuffer,
			(sizeof InputBuffer / sizeof (TCHAR)),
			stdin
			);
		if (0 == lstrlen(InputBuffer)) continue;
		LocalArgc = ParseCommandLine(InputBuffer, LocalArgv);
		if (LocalArgc && (cd = DecodeVerb(LocalArgv[0])))
		{
			if (LocalArgc < cd->MinArgc)
			{
				_tprintf(
					_TEXT("Too few arguments. Type \"HELP %s\".\n"),
					LocalArgv[0]
					);
				continue;
			}
			if (LocalArgc > cd->MaxArgc)
			{
				_tprintf(
					_TEXT("Too many arguments. Type \"HELP %s\".\n"),
					LocalArgv[0]
					);
				continue;
			}
			LastExitCode = cd->Command(
						LocalArgc,
						LocalArgv
						);
			continue;
		}
		_tprintf(
			_TEXT("Unknown command (\"%s\").\n"),
			LocalArgv[0]
			);
	}
	return EXIT_SUCCESS;
}


/* EOF */
