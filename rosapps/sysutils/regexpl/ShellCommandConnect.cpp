/* $Id: ShellCommandConnect.cpp,v 1.1 2000/10/04 21:04:30 ea Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (c) 1999-2000 Nedko Arnaoudov <nedkohome@atia.com>
 *
 * License: GNU GPL
 *
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

	m_rTree.SetMachineName(pchMachine);
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
