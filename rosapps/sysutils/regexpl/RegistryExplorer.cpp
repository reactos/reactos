/* $Id: RegistryExplorer.cpp,v 1.5 2001/04/15 22:17:50 narnaoud Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (C) 2000,2001 Nedko Arnaoudov <nedkohome@atia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

// Registry Explorer.cpp : Defines the entry point for the console application.
//

#include "ph.h"
#include "RegistryExplorer.h"
#include "Console.h"
#include "RegistryKey.h"
#include "RegistryTree.h"
#include "SecurityDescriptor.h"
#include "ArgumentParser.h"
#include "ShellCommandsLinkedList.h"
#include "ShellCommandExit.h"
#include "ShellCommandVersion.h"
#include "ShellCommandHelp.h"
#include "ShellCommandDir.h"
#include "ShellCommandChangeKey.h"
#include "ShellCommandValue.h"
#include "ShellCommandOwner.h"
#include "ShellCommandDACL.h"
#include "ShellCommandSACL.h"
//#include "ShellCommandDOKA.h"
#include "ShellCommandConnect.h"
#include "ShellCommandNewKey.h"
#include "ShellCommandDeleteKey.h"
#include "ShellCommandSetValue.h"
#include "ShellCommandDeleteValue.h"

TCHAR pchCurrentKey[PROMPT_BUFFER_SIZE];

CRegistryTree Tree;
CConsole Console;

BOOL blnCommandExecutionInProgress = FALSE;

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	switch(dwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		if (blnCommandExecutionInProgress)
		{
/*			Beep(1000,100);
			Beep(2000,100);
			Beep(3000,100);
			Beep(4000,100);
			Beep(5000,100);
			Beep(4000,100);
			Beep(3000,100);
			Beep(2000,100);
			Beep(1000,100);*/
			Console.Write(_T("\n"));
			Console.DisableWrite();
		}
		return TRUE;
	default:
		return FALSE;
	}
}

//int _tmain(/*int argc, TCHAR* argv[], TCHAR* envp[]*/)
int main ()
{
	CShellCommandsLinkedList CommandsList(Console);

	CShellCommandExit ExitCommand;
	CommandsList.AddCommand(&ExitCommand);

	CShellCommandVersion VersionCommand;
	CommandsList.AddCommand(&VersionCommand);

	CShellCommandHelp HelpCommand(CommandsList);
	CommandsList.AddCommand(&HelpCommand);

	CShellCommandDir DirCommand(Tree);
	CommandsList.AddCommand(&DirCommand);

	CShellCommandChangeKey ChangeKeyCommand(Tree);
	CommandsList.AddCommand(&ChangeKeyCommand);

	CShellCommandValue ValueCommand(Tree);
	CommandsList.AddCommand(&ValueCommand);

	CShellCommandOwner OwnerCommand(Tree);
	CommandsList.AddCommand(&OwnerCommand);

	CShellCommandDACL DACLCommand(Tree);
	CommandsList.AddCommand(&DACLCommand);

	CShellCommandSACL SACLCommand(Tree);
	CommandsList.AddCommand(&SACLCommand);

	//CShellCommandDOKA DOKACommand(Tree);
	//CommandsList.AddCommand(&DOKACommand);

	CShellCommandConnect ConnectCommand(Tree);
	CommandsList.AddCommand(&ConnectCommand);

	CShellCommandNewKey NewKeyCommand(Tree);
	CommandsList.AddCommand(&NewKeyCommand);

	CShellCommandDeleteKey DeleteKeyCommand(Tree);
	CommandsList.AddCommand(&DeleteKeyCommand);

	CShellCommandSetValue SetValueCommand(Tree);
	CommandsList.AddCommand(&SetValueCommand);

	CShellCommandDeleteValue DeleteValueCommand(Tree);
	CommandsList.AddCommand(&DeleteValueCommand);

	CArgumentParser Parser;

	int nRetCode = 0;

// input buffer size in chars
#define INPUT_BUFFER_SIZE	1024
//#define INPUT_BUFFER_SIZE	128
//#define INPUT_BUFFER_SIZE	10

	TCHAR *pchCommand;

	pchCommand = Console.Init(INPUT_BUFFER_SIZE,10);
	if (pchCommand == NULL)
	{
		_ftprintf(stderr,_T("Cannot initialize console.\n"));
		goto Abort;
	}

	Console.SetReplaceCompletionCallback(CompletionCallback);

	WORD wOldConsoleAttribute;
	if (!Console.GetTextAttribute(wOldConsoleAttribute)) goto Abort;

	Console.SetTitle(_T("Registry Explorer"));
	Console.SetTextAttribute(FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED/*|FOREGROUND_INTENSITY*/);

	VERIFY(SetConsoleCtrlHandler((PHANDLER_ROUTINE)HandlerRoutine,TRUE));

	if (!Console.Write(HELLO_MSG
	//(_L(__TIMESTAMP__))
	)) goto Abort;

  //Tree.SetDesiredOpenKeyAccess(KEY_READ);

  const TCHAR *pszCurrentPath;
GetCommand:
	// prompt
	// TODO: make prompt user-customizable
	Console.EnableWrite();
  pszCurrentPath = Tree.GetCurrentPath();
	Console.Write(pszCurrentPath?pszCurrentPath:_T("NULL (Internal Error)"));
	Console.Write(_T("\n# "));
	Console.FlushInputBuffer();

	blnCommandExecutionInProgress = FALSE;

	// Set command line color
//	Console.SetTextAttribute(/*FOREGROUND_BLUE|*/FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_INTENSITY);
	if (!Console.ReadLine())
    goto Abort;

	// Set normal color
//	Console.SetTextAttribute(FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_INTENSITY);

	Console.BeginScrollingOperation();
	blnCommandExecutionInProgress = TRUE;

	// Parse command line (1st step - convert to multi sz)
	Parser.SetArgumentList(pchCommand);

	int nCommandReturnValue;
	switch(CommandsList.Execute(Parser,nCommandReturnValue))
	{
	case -1:	// not recognized command
		{
			Parser.ResetArgumentIteration();
			TCHAR *pchCommandItself = Parser.GetNextArgument();
			size_t cmdlen = _tcslen(pchCommandItself);
			if ((!cmdlen)||
				(pchCommandItself[cmdlen-1] != _T('\\'))||
				(Parser.GetNextArgument())||
				(!Tree.ChangeCurrentKey(pchCommandItself)))
			{
				Console.Write(_T("Unknown command \""));
				Console.Write(pchCommandItself);
				Console.Write(_T("\"\n"));
			}
		}
	case -2:	// empty line
		goto GetCommand;
	case 0:	// exit command
		nRetCode = 0;
		Console.SetTextAttribute(wOldConsoleAttribute);
		goto Exit;
	default:
		Console.Write(_T("\n"));
		goto GetCommand;
	}

Abort:
	_ftprintf(stderr,_T("Abnormal program termination.\nPlease report bugs to ") EMAIL _T("\n"));
	nRetCode = 1;
Exit:
	return nRetCode;
}
