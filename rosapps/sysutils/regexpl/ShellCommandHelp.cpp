/* $Id: ShellCommandHelp.cpp,v 1.3 2001/01/10 01:25:29 narnaoud Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (C) 2000 Nedko Arnaoudov <nedkohome@atia.com>
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

		return 0;
	}

	if (_tcsicmp(pchArg,_T("GPL")) == 0)
	{
		HRSRC hrcGPL;
		HGLOBAL hGPL;
		char *pchGPL;
		DWORD dwSize;
		if ((hrcGPL = FindResource(NULL, _T("GPL"), _T("LICENSE")))&&
			(hGPL = LoadResource(NULL,hrcGPL))&&
			(pchGPL = (char *)LockResource(hGPL))&&
			(dwSize = SizeofResource(NULL,hrcGPL)))
		{
			// save last char
			char pchSaved[2];
			pchSaved[0] = pchGPL[dwSize-1];
			pchSaved[1] = 0;

			// make null-terminated string
			pchGPL[dwSize-1] = 0;

			// replace all non-printable chars except CR, LF and HTAB with spaces
			for (DWORD i = 0; i < dwSize ; i++)
				if ((!isprint(pchGPL[i]))&&(pchGPL[i] != '\r')&&(pchGPL[i] != '\n')&&(pchGPL[i] != '\t'))
					pchGPL[i] = ' ';

			rConsole.Write(pchGPL);
			rConsole.Write(pchSaved);
		}
		else
		{
			rConsole.Write(_T("Internal error cannot load GPL.\n"));
		}
		return 0;
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
			_T("Syntax: ") HELP_CMD _T(" [<COMMAND>] [/?]\n")
			_T("    COMMAND    - Command for which help will be displayed.\n")
			_T("    /?    - This help.\n\n")
			_T("Without parameters, command lists available commands.\n");
}

const TCHAR * CShellCommandHelp::GetHelpShortDescriptionString()
{
	return HELP_CMD_SHORT_DESC;
}
