/* $Id: ShellCommandVersion.cpp,v 1.2 2000/10/24 20:17:42 narnaoud Exp $
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
