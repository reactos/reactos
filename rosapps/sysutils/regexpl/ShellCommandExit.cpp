/* $Id: ShellCommandExit.cpp,v 1.3 2001/01/13 23:55:37 narnaoud Exp $
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

