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

#define NTOS_MODE_USER
#include <ntos.h>
#include <sm/helper.h>

#define SM_CMD(n) cmd_##n
#define SM_CMD_DECL(n) int SM_CMD(n)(int argc, char * argv[])
#define SM_CMD_CALL(n,c,v) SM_CMD(n)((c),(v))

HANDLE hSmApiPort = (HANDLE) 0;

typedef struct _SM_CMD_DESCRIPTOR
{
	const char * Name;
	int (*EntryPoint)(int,char**);
	const char * Synopsis;
	const char * Description;
	
} SM_CMD_DESCRIPTOR, *PSM_CMD_DESCRIPTOR;

SM_CMD_DECL(boot);
SM_CMD_DECL(help);
SM_CMD_DECL(info);
SM_CMD_DECL(reboot);
SM_CMD_DECL(shutdown);

/* internal commands directory */
SM_CMD_DESCRIPTOR Command [] =
{
	{"boot",     SM_CMD(boot),     "boot subsystem_name",   "bootstrap an optional environment subsystem;"},
	{"help",     SM_CMD(help),     "help [command]",        "print help for command;"},
	{"info",     SM_CMD(info),     "info [subsystem_id]",   "print information about a booted subsystem\n"
							        "if subsystem_id is omitted, a list of booted\n"
							        "environment subsystems is printed."},
	{"reboot",   SM_CMD(reboot),   "reboot subsystem_id",   "reboot an optional environment subsystem;"},
	{"shutdown", SM_CMD(shutdown), "shutdown subsystem_id", "shutdown an optional environment subsystem;"},
};

PSM_CMD_DESCRIPTOR LookupCommand (const char * CommandName)
{
	int i;
	const int command_count = (sizeof Command / sizeof Command[0]);
	
	/* parse the command... */

	for (i=0; (i < command_count); i ++)
	{
		if (0 == strcmp(CommandName, Command[i].Name))
		{
			break;
		}
	}
	if (i == command_count)
	{
		fprintf(stderr, "Unknown command '%s'.\n", CommandName);
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
		RtlInitAnsiString (& ProgramA, argv[2]);
		RtlAnsiStringToUnicodeString (& ProgramW, & ProgramA, TRUE);
		Status = SmExecuteProgram (hSmApiPort, & ProgramW);
		RtlFreeUnicodeString (& ProgramW);
		if (STATUS_SUCCESS != Status)
		{
			printf ("Status 0x%08lx\n", Status);
		}
	}
	else
	{
		argv[2]="boot";
		return SM_CMD_CALL(help,3,argv);
	}
	return rc;
}

SM_CMD_DECL(help)
{
	int i = 0;
	PSM_CMD_DESCRIPTOR cmd = NULL;
	int rc = EXIT_SUCCESS;

	switch (argc)
	{
	case 2:
		for (i=0; (i < (sizeof Command / sizeof Command[0])); i ++)
		{
			printf("%s\n", Command[i].Synopsis);
		}
		break;
	case 3:
		cmd = LookupCommand (argv[2]);
		if (NULL == cmd)
		{
			rc = EXIT_FAILURE;
			break;
		}
		printf("%s\n%s\n\n%s\n",
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
	SM_BASIC_INFORMATION bi = {0,};
	ULONG ReturnDataLength = sizeof bi;

	Status = SmQueryInformation (hSmApiPort,
				     SmBasicInformation,
				     & bi,
				     sizeof bi,
				     & ReturnDataLength);
	if (STATUS_SUCCESS == Status)
	{
		int i = 0;

		printf ("SSID PID      Flags\n");
		for (i = 0; i < bi.SubSystemCount; i ++)
		{
			printf ("%04x %08lx %04x\n",
					bi.SubSystem[i].Id,
			       		bi.SubSystem[i].ProcessId,
					bi.SubSystem[i].Flags);
		}
	}
	else
	{
		printf ("Status 0x%08lx\n", Status);
		rc = EXIT_FAILURE;
	}
	return rc;
}

SM_CMD_DECL(shutdown)
{
	int rc = EXIT_SUCCESS;
	
	fprintf(stderr,"not implemented\n");
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
int print_synopsys (int argc, char *argv[])
{
	fprintf (stderr, "ReactOS/Win32 Session Manager Control Tool\n\n");
	printf ("Usage:\n"
		"\tsm\n"
		"\tsm help [command]\n"
		"\tsm command [arguments]\n\n"
		"'sm help' will print the list of valid commands.\n");
	return EXIT_SUCCESS;
}

/* parse and execute */
int pande (int argc, char *argv[])
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
	fprintf (stderr, "Failed to connect to the Session Manager! (Status=0x%08lx)\n", Status);
	return EXIT_FAILURE;
}

int main (int argc, char *argv[])
{
	return (1==argc)
		? print_synopsys (argc, argv)
		: pande (argc, argv);
}
/* EOF */
