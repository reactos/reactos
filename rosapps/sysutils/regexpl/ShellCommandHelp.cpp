/* $Id: ShellCommandHelp.cpp,v 1.1 2000/10/04 21:04:31 ea Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (c) 1999-2000 Nedko Arnaoudov <nedkohome@atia.com>
 *
 * License: GNU GPL
 *
 */

// ShellCommandHelp.cpp: implementation of the CShellCommandHelp class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "ShellCommandHelp.h"

#define HELP_CMD			_T("HELP")
#define HELP_CMD_SHORT_DESC	HELP_CMD _T(" command provides help information about Registry Explorer commands.\n")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellCommandHelp::CShellCommandHelp(CShellCommandsLinkedList& rCommandsLinkedList):m_rCommandsLinkedList(rCommandsLinkedList)
{
}

CShellCommandHelp::~CShellCommandHelp()
{

}

BOOL CShellCommandHelp::Match(const TCHAR *pchCommand)
{
	return _tcsicmp(pchCommand,HELP_CMD) == 0;
}

int CShellCommandHelp::Execute(CConsole &rConsole, CArgumentParser& rArguments)
{
	const TCHAR *pchArg = rArguments.GetNextArgument();
	CShellCommand *pCommand;
	if (pchArg == NULL)
	{
		POSITION pos = m_rCommandsLinkedList.GetFirstCommandPosition();
		while(pos)
		{
			pCommand = m_rCommandsLinkedList.GetNextCommand(pos);
			rConsole.Write(pCommand->GetHelpShortDescriptionString());
		}
	}

	while (pchArg)
	{
		pCommand = m_rCommandsLinkedList.Match(pchArg);
		if ((!pCommand)&&((_tcsicmp(pchArg,_T("/?")) == 0)||(_tcsicmp(pchArg,_T("-?")) == 0)))
			pCommand = this;

		if (pCommand)
		{
			rConsole.Write(pCommand->GetHelpString());
		}
		else
		{
			rConsole.Write(_T("HELP: Unknown command \""));
			rConsole.Write(pchArg);
			rConsole.Write(_T("\"\n"));
		}

		pchArg = rArguments.GetNextArgument();
	}
	return 0;
}

const TCHAR * CShellCommandHelp::GetHelpString()
{
	return HELP_CMD_SHORT_DESC
			_T("Syntax: ") HELP_CMD _T("[<COMMAND>] [/?]\n")
			_T("    COMMAND    - Command for which help will be displayed.\n")
			_T("    /?    - This help.\n\n")
			_T("Without parameters, command lists available commands.\n");
}

const TCHAR * CShellCommandHelp::GetHelpShortDescriptionString()
{
	return HELP_CMD_SHORT_DESC;
}
