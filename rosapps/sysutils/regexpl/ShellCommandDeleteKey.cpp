/* $Id: ShellCommandDeleteKey.cpp,v 1.2 2000/10/24 20:17:41 narnaoud Exp $
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

// ShellCommandDeleteKey.cpp: implementation of the CShellCommandDeleteKey class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "ShellCommandDeleteKey.h"
#include "RegistryExplorer.h"

#define DK_CMD			_T("DK")
#define DK_CMD_SHORT_DESC	DK_CMD _T(" command is used to delete key(s).\n")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellCommandDeleteKey::CShellCommandDeleteKey(CRegistryTree& rTree):m_rTree(rTree)
{
}

CShellCommandDeleteKey::~CShellCommandDeleteKey()
{
}

BOOL CShellCommandDeleteKey::Match(const TCHAR *pchCommand)
{
	return _tcsicmp(pchCommand,DK_CMD) == 0;
}

int CShellCommandDeleteKey::Execute(CConsole &rConsole, CArgumentParser& rArguments)
{
	const TCHAR *pchKey = NULL, *pchArg;

	BOOL blnHelp = FALSE;
	BOOL blnExitAfterHelp = FALSE;
	BOOL blnRecursive = FALSE;
	
	while((pchArg = rArguments.GetNextArgument()) != NULL)
	{
		if ((_tcsicmp(pchArg,_T("/?")) == 0)
			||(_tcsicmp(pchArg,_T("-?")) == 0))
		{
			blnHelp = TRUE;
		}
		else if ((_tcsicmp(pchArg,_T("/s")) == 0)
			||(_tcsicmp(pchArg,_T("-s")) == 0))
		{
			blnRecursive = TRUE;
		}
		else
		{
			if (pchKey)
			{
				rConsole.Write(_T("Wrong parameter : \""));
				rConsole.Write(pchArg);
				rConsole.Write(_T("\"\n\n"));
				blnHelp = TRUE;
			}
			else
			{
				pchKey = pchArg;
			}
		}
	}

	if ((!blnHelp) && (!pchKey))
	{
		rConsole.Write(_T("Key name not specified !\n\n"));
		blnExitAfterHelp = TRUE;
	}

	if (blnHelp)
	{
		rConsole.Write(GetHelpString());
		if (blnExitAfterHelp)
			return 0;
		else
		rConsole.Write(_T("\n"));
	}

	if (!m_rTree.DeleteKey(pchKey,blnRecursive))
	{
		rConsole.Write(_T("Cannot delete key.\n"));
		rConsole.Write(m_rTree.GetLastErrorDescription());
	}
	return 0;
}

const TCHAR * CShellCommandDeleteKey::GetHelpString()
{
	return DK_CMD_SHORT_DESC
			_T("Syntax: ") DK_CMD _T(" [/s] [/?] Key_Name\n\n")
			_T("    /? - This help.\n\n")
			_T("    /s - Delete key and all subkeys.\n");
}

const TCHAR * CShellCommandDeleteKey::GetHelpShortDescriptionString()
{
	return DK_CMD_SHORT_DESC;
}
