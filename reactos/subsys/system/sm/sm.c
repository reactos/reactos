/*
 *  ReactOS Win32 Applications
 *  Copyright (C) 2005 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * COPYRIGHT : See COPYING in the top level directory
 * PROJECT   : ReactOS/Win32 Session Manager Control Tool
 * FILE      : subsys/system/sm/sm.c
 * PROGRAMMER: Emanuele Aliberti (ea@reactos.com)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include "resource.h"

#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include <sm/helper.h>

#define SM_CMD(n) cmd_##n
#define SM_CMD_DECL(n) int SM_CMD(n)(int argc, char * argv[])
#define SM_CMD_CALL(n,c,v) SM_CMD(n)((c),(v))

HANDLE hSmApiPort = (HANDLE) 0;

typedef struct _SM_CMD_DESCRIPTOR
{
	TCHAR Name[RC_STRING_MAX_SIZE];
	int (*EntryPoint)(int,TCHAR**);
	TCHAR Synopsis[RC_STRING_MAX_SIZE];
	TCHAR Description[RC_STRING_MAX_SIZE];
	
} SM_CMD_DESCRIPTOR, *PSM_CMD_DESCRIPTOR;

SM_CMD_DECL(boot);
SM_CMD_DECL(help);
SM_CMD_DECL(info);
SM_CMD_DECL(reboot);
SM_CMD_DECL(shutdown);

/* internal commands directory */
SM_CMD_DESCRIPTOR Command [] =
{
	{"boot",     SM_CMD(boot),     _T("boot subsystem_name"),   _T("bootstrap an optional environment subsystem;")},
	{"help",     SM_CMD(help),     _T("help [command]"),        _T("print help for command;")},
	{"info",     SM_CMD(info),     _T("info [subsystem_id]"),   _T("print information about a booted subsystem\n"
							        "if subsystem_id is omitted, a list of booted\n"
							        "environment subsystems is printed.")},
	{"reboot",   SM_CMD(reboot),   _T("reboot subsystem_id"),   _T("reboot an optional environment subsystem;")},
	{"shutdown", SM_CMD(shutdown), _T("shutdown subsystem_id"), _T("shutdown an optional environment subsystem;")},
};

TCHAR UsageMessage[RC_STRING_MAX_SIZE];
void loadlang(PSM_CMD_DESCRIPTOR );

PSM_CMD_DESCRIPTOR LookupCommand (const TCHAR * CommandName)
{
	int i;
	const int command_count = (sizeof Command / sizeof Command[0]);
	
	/* parse the command... */

	for (i=0; (i < command_count); i ++)
	{
		if (0 == _tcscmp(CommandName, Command[i].Name))
		{
			break;
		}
	}
	if (i == command_count)
	{
		LoadString( GetModuleHandle(NULL), IDS_Unknown, (LPTSTR) UsageMessage,RC_STRING_MAX_SIZE);

		_ftprintf(stderr, _T("%s '%s'.\n"), UsageMessage, CommandName);
		return NULL;
	}
	return & Command [i];
}

/* user commands */

SM_CMD_DECL(boot)
{
	int rc = EXIT_SUCCESS;
	ANSI_STRING ProgramA;
	UNICODE_STRING ProgramW;
	NTSTATUS Status = STATUS_SUCCESS;

	if (3 == argc)
	{
#ifndef _UNICODE
		RtlInitAnsiString (& ProgramA, argv[2]);
		RtlAnsiStringToUnicodeString (& ProgramW, & ProgramA, TRUE);
		Status = SmExecuteProgram (hSmApiPort, & ProgramW);
		RtlFreeUnicodeString (& ProgramW);
#else
		ProgramW = &argv[2];      
		Status = SmExecuteProgram (hSmApiPort, & ProgramW);		
#endif
		if (STATUS_SUCCESS != Status)
		{
			LoadString( GetModuleHandle(NULL), IDS_Status, (LPTSTR) UsageMessage,RC_STRING_MAX_SIZE);

			_tprintf(UsageMessage, Status);
		}

	}
	else
	{
		argv[2]=_T("boot");
		return SM_CMD_CALL(help,3,argv);
	}
	return rc;
}

SM_CMD_DECL(help)
{
	unsigned int i = 0;
	PSM_CMD_DESCRIPTOR cmd = NULL;
	int rc = EXIT_SUCCESS;

	switch (argc)
	{
	case 2:
		for (i=0; (i < (sizeof Command / sizeof Command[0])); i ++)
		{
			_tprintf(_T("%s\n"), Command[i].Synopsis);
		}
		break;
	case 3:
		cmd = LookupCommand (argv[2]);
		if (NULL == cmd)
		{
			rc = EXIT_FAILURE;
			break;
		}
		_tprintf(_T("%s\n%s\n\n%s\n"),
			cmd->Name,
			cmd->Synopsis,
			cmd->Description);
		break;
	}
	return rc;
}

SM_CMD_DECL(info)
{
	int rc = EXIT_SUCCESS;
	NTSTATUS Status = STATUS_SUCCESS;
	SM_INFORMATION_CLASS InformationClass = SmBasicInformation;
	union {
	SM_BASIC_INFORMATION bi;
	SM_SUBSYSTEM_INFORMATION ssi;
	} Info;
	ULONG DataLength = 0;
	ULONG ReturnDataLength = 0;
	INT i = 0;

	RtlZeroMemory (& Info, sizeof Info);
	switch (argc)
	{
	case 2: /* sm info */
		InformationClass = SmBasicInformation;
		DataLength = sizeof Info.bi;
		break;
	case 3: /* sm info id */
		InformationClass = SmSubSystemInformation;
		DataLength = sizeof Info.ssi;
		Info.ssi.SubSystemId = atol(argv[2]);
		break;
	default:
		return EXIT_FAILURE;
		break;
	}
	Status = SmQueryInformation (hSmApiPort,
				     InformationClass,
				     & Info,
				     DataLength,
				     & ReturnDataLength);
	if (STATUS_SUCCESS != Status)
	{
		LoadString( GetModuleHandle(NULL), IDS_Status, (LPTSTR) UsageMessage,RC_STRING_MAX_SIZE);
		_tprintf(UsageMessage, Status);
		return EXIT_FAILURE;
	}
	switch (argc)
	{
	case 2:
        LoadString( GetModuleHandle(NULL), IDS_SM1, (LPTSTR) UsageMessage,RC_STRING_MAX_SIZE);		
		_tprintf(UsageMessage);
        
		LoadString( GetModuleHandle(NULL), IDS_SM2, (LPTSTR) UsageMessage,RC_STRING_MAX_SIZE);		
		for (i = 0; i < Info.bi.SubSystemCount; i ++)
		{
			_tprintf(UsageMessage,
				Info.bi.SubSystem[i].Id,
		       		Info.bi.SubSystem[i].ProcessId,
				Info.bi.SubSystem[i].Flags);
		}
		break;
	case 3:
        LoadString( GetModuleHandle(NULL), IDS_ID, (LPTSTR) UsageMessage,RC_STRING_MAX_SIZE);		
		
		_tprintf  (UsageMessage, Info.ssi.SubSystemId, Info.ssi.Flags,  Info.ssi.ProcessId);
		wprintf(L"  NSRootNode: '%s'\n", Info.ssi.NameSpaceRootNode);
		break;
	default:
		break;
	}
	return rc;
}

SM_CMD_DECL(shutdown)
{
	int rc = EXIT_SUCCESS;

	LoadString( GetModuleHandle(NULL), IDS_Not_Imp, (LPTSTR) UsageMessage,RC_STRING_MAX_SIZE);		
 	
	_ftprintf(stderr,UsageMessage);
	return rc;
}

SM_CMD_DECL(reboot)
{
	int rc = SM_CMD(shutdown)(argc,argv);
	if(EXIT_SUCCESS == rc)
	{
		rc = SM_CMD(boot)(argc,argv);
	}
	return rc;
}

/* print command's synopsys */
int print_synopsys (int argc, TCHAR *argv[])
{
	LoadString( GetModuleHandle(NULL), IDS_Mangers, (LPTSTR) UsageMessage,RC_STRING_MAX_SIZE);
	_ftprintf (stderr, UsageMessage);

	LoadString( GetModuleHandle(NULL), IDS_USING, (LPTSTR) UsageMessage,RC_STRING_MAX_SIZE);	
	_tprintf (UsageMessage);
	return EXIT_SUCCESS;
}

/* parse and execute */
int pande (int argc, TCHAR *argv[])
{
	PSM_CMD_DESCRIPTOR Command = NULL;
	NTSTATUS Status = STATUS_SUCCESS;

	/* Lookup the user command... */	
	Command = LookupCommand (argv[1]);
	if (NULL == Command)
	{
		return EXIT_FAILURE;
	} 
	/* Connect to the SM in non-registering mode. */
	Status = SmConnectApiPort (0, 0, 0, & hSmApiPort);
	if (STATUS_SUCCESS == Status)
	{	
		/* ...and execute it */
		return Command->EntryPoint (argc, argv);
	}
    LoadString( GetModuleHandle(NULL), IDS_FAILS_MNG, (LPTSTR) UsageMessage,RC_STRING_MAX_SIZE);		
	_ftprintf (stderr, UsageMessage, Status);

	return EXIT_FAILURE;
}

void loadlang(PSM_CMD_DESCRIPTOR cmd)
{
 int i=0;
 if (cmd==NULL) return;
  for (i=0;i < 5; i++)
  {
	LoadString( GetModuleHandle(NULL), IDS_boot+i, (LPTSTR) &cmd->Synopsis[i],RC_STRING_MAX_SIZE);
  }
}

int _tmain (int argc, TCHAR *argv[])
{ 
   loadlang(Command);

	return (1==argc)
		? print_synopsys (argc, argv)
		: pande (argc, argv);
}
/* EOF */
