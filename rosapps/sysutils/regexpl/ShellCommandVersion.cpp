/* $Id: ShellCommandVersion.cpp,v 1.1 2000/10/04 21:04:31 ea Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (c) 1999-2000 Nedko Arnaoudov <nedkohome@atia.com>
 *
 * License: GNU GPL
 *
 */

// ShellCommandVersion.cpp: implementation of the CShellCommandVersion class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "ShellCommandVersion.h"
#include "RegistryExplorer.h"

#define VER_CMD				_T("VER")
#define VER_CMD_SHORT_DESC	VER_CMD _T(" command displays information about this program.\n")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellCommandVersion::CShellCommandVersion()
{
}

CShellCommandVersion::~CShellCommandVersion()
{
}

BOOL CShellCommandVersion::Match(const TCHAR *pchCommand)
{
	return _tcsicmp(pchCommand,VER_CMD) == 0;
}

int CShellCommandVersion::Execute(CConsole &rConsole, CArgumentParser& rArguments)
{
	rConsole.Write(HELLO_MSG);
	rConsole.Write(VER_MSG);
	return 0;
}

const TCHAR * CShellCommandVersion::GetHelpString()
{
	return VER_CMD_SHORT_DESC _T("Syntax: ") VER_CMD _T("\n");
}

const TCHAR * CShellCommandVersion::GetHelpShortDescriptionString()
{
	return VER_CMD_SHORT_DESC;
}
