/* $Id: ShellCommandChangeKey.cpp,v 1.2 2000/10/24 20:17:41 narnaoud Exp $
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

// ShellCommandChangeKey.cpp: implementation of the CShellCommandChangeKey class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "RegistryExplorer.h"
#include "ShellCommandChangeKey.h"

#define CD_CMD				_T("CD")
#define CD_CMD_LENGTH		COMMAND_LENGTH(CD_CMD)
#define CD_CMD_SHORT_DESC	CD_CMD _T(" command changes current key.\n")
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellCommandChangeKey::CShellCommandChangeKey(CRegistryTree& rTree):m_rTree(rTree)
{

}

CShellCommandChangeKey::~CShellCommandChangeKey()
{

}

BOOL CShellCommandChangeKey::Match(const TCHAR *pchCommand)
{
	if (_tcsicmp(pchCommand,CD_CMD) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,CD_CMD _T(".."),CD_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,CD_CMD _T("\\"),CD_CMD_LENGTH+2*sizeof(TCHAR)) == 0)
		return TRUE;
	return FALSE;
}

int CShellCommandChangeKey::Execute(CConsole &rConsole, CArgumentParser& rArguments)
{
	BOOL blnHelp = FALSE;

	rArguments.ResetArgumentIteration();
	const TCHAR *pchCommandItself = rArguments.GetNextArgument();
	const TCHAR *pchPath = rArguments.GetNextArgument();

	if ((_tcsnicmp(pchCommandItself,CD_CMD _T(".."),CD_CMD_LENGTH+2*sizeof(TCHAR)) == 0)||
		(_tcsnicmp(pchCommandItself,CD_CMD _T("\\"),CD_CMD_LENGTH+1*sizeof(TCHAR)) == 0))
	{
		if (!pchPath) pchPath = pchCommandItself + CD_CMD_LENGTH;
		else blnHelp = TRUE;
	}

	if ((!blnHelp)&&(pchPath != NULL)&&(!rArguments.GetNextArgument()))
	{
		ASSERT(_tcslen(pchPath) <= PROMPT_BUFFER_SIZE);
		if (!m_rTree.ChangeCurrentKey(pchPath))
		{
			rConsole.Write(m_rTree.GetLastErrorDescription());
			rConsole.Write(_T("\n"));
		}
	}
	else
	{
		rConsole.Write(GetHelpString());
	}

	return 0;
}

const TCHAR * CShellCommandChangeKey::GetHelpString()
{
	return CD_CMD_SHORT_DESC
			_T("Syntax: ") CD_CMD _T(" <KEY>\n\n")
			_T("    <KEY> - Relative path of desired key.\n\n")
			_T("Without parameters, command displays this help.\n");

}

const TCHAR * CShellCommandChangeKey::GetHelpShortDescriptionString()
{
	return CD_CMD_SHORT_DESC;
}

