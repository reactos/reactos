/* $Id: ShellCommandConnect.cpp,v 1.4 2001/01/13 23:55:36 narnaoud Exp $
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

// ShellCommandConnect.cpp: implementation of the CShellCommandConnect class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "ShellCommandConnect.h"

#define CONNECT_CMD			_T("CONNECT")
#define CONNECT_CMD_SHORT_DESC	CONNECT_CMD _T(" command is used to connect to remote registry.\n")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellCommandConnect::CShellCommandConnect(CRegistryTree& rTree):m_rTree(rTree)
{
}

CShellCommandConnect::~CShellCommandConnect()
{
}

BOOL CShellCommandConnect::Match(const TCHAR *pchCommand)
{
	return _tcsicmp(pchCommand,CONNECT_CMD) == 0;
}

int CShellCommandConnect::Execute(CConsole &rConsole, CArgumentParser& rArguments)
{
	const TCHAR *pchArg;
	const TCHAR *pchMachine = NULL;
	BOOL blnHelp = FALSE;

	VERIFY(m_rTree.ChangeCurrentKey(_T("\\")));

	while ((pchArg = rArguments.GetNextArgument()) != NULL)
	{
		if ((_tcsicmp(pchArg,_T("/?")) == 0)
			||(_tcsicmp(pchArg,_T("-?")) == 0))
		{
			blnHelp = TRUE;
		}
//		else if ((_tcsicmp(pchArg,_T("-a")) == 0)||
//			(_tcsicmp(pchArg,_T("/a")) == 0))
//		{
//		}
		else
		{
			pchMachine = pchArg;
		}
	}

	if (blnHelp)
		rConsole.Write(GetHelpString());

	if (!m_rTree.SetMachineName(pchMachine))
  {
    rConsole.Write(m_rTree.GetLastErrorDescription());
    rConsole.Write(_T("\n"));
  }
  
	return 0;
}

const TCHAR * CShellCommandConnect::GetHelpString()
{
	return CONNECT_CMD_SHORT_DESC
//			_T("Syntax: ") CONNECT_CMD _T(" [Switches] [/?] machine\n\n")
			_T("Syntax: ") CONNECT_CMD _T(" /? | MACHINE\n\n")
			_T("    /? - This help.\n\n")
//			_T("Switches are:\n")
//			_T("    -a anonymous login.\n")
			_T("    MACHINE is name/ip of the remote machine.\n")
			_T("Example:\n")
			_T("    ") CONNECT_CMD _T(" BOB");
}

const TCHAR * CShellCommandConnect::GetHelpShortDescriptionString()
{
	return CONNECT_CMD_SHORT_DESC;
}
