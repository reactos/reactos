/* $Id: ShellCommandExit.cpp,v 1.1 2000/10/04 21:04:31 ea Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (c) 1999-2000 Nedko Arnaoudov <nedkohome@atia.com>
 *
 * License: GNU GPL
 *
 */

// ShellCommandExit.cpp: implementation of the CShellCommandExit class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "ShellCommandExit.h"
#include "RegistryExplorer.h"

#define EXIT_CMD			_T("EXIT")
#define EXIT_CMD_SHORT_DESC	EXIT_CMD _T(" command termiantes current instance of Registry Explorer.\n")
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellCommandExit::CShellCommandExit()
{

}

CShellCommandExit::~CShellCommandExit()
{

}

BOOL CShellCommandExit::Match(const TCHAR *pchCommand)
{
	return _tcsicmp(pchCommand,EXIT_CMD) == 0;
}

int CShellCommandExit::Execute(CConsole &rConsole, CArgumentParser& rArguments)
{
	rConsole.Write(GOODBYE_MSG);
	return 0;
}

const TCHAR * CShellCommandExit::GetHelpString()
{
	return EXIT_CMD_SHORT_DESC	_T("Syntax: ") EXIT_CMD _T("\n");
}

const TCHAR * CShellCommandExit::GetHelpShortDescriptionString()
{
	return EXIT_CMD_SHORT_DESC;
}

