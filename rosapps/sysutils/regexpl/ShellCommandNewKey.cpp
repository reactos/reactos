/* $Id: ShellCommandNewKey.cpp,v 1.2 2000/10/24 20:17:41 narnaoud Exp $
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

// ShellCommandNewKey.cpp: implementation of the CShellCommandNewKey class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "ShellCommandNewKey.h"
#include "RegistryExplorer.h"

#define NK_CMD			_T("NK")
#define NK_CMD_SHORT_DESC	NK_CMD _T(" command is used to create new key.\n")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellCommandNewKey::CShellCommandNewKey(CRegistryTree& rTree):m_rTree(rTree)
{
}

CShellCommandNewKey::~CShellCommandNewKey()
{
}

BOOL CShellCommandNewKey::Match(const TCHAR *pchCommand)
{
	return _tcsicmp(pchCommand,NK_CMD) == 0;
}

int CShellCommandNewKey::Execute(CConsole &rConsole, CArgumentParser& rArguments)
{
	const TCHAR *pchNewKey = NULL, *pchArg;

	BOOL blnHelp = FALSE;
	BOOL blnExitAfterHelp = FALSE;
	BOOL blnVolatile = FALSE;
	
	while((pchArg = rArguments.GetNextArgument()) != NULL)
	{
		if ((_tcsicmp(pchArg,_T("/?")) == 0)
			||(_tcsicmp(pchArg,_T("-?")) == 0))
		{
			blnHelp = TRUE;
		}
		else if ((_tcsicmp(pchArg,_T("/v")) == 0)
			||(_tcsicmp(pchArg,_T("-v")) == 0))
		{
			blnVolatile = TRUE;
		}
		else
		{
			if (pchNewKey)
			{
				rConsole.Write(_T("Wrong parameter : \""));
				rConsole.Write(pchArg);
				rConsole.Write(_T("\"\n\n"));
				blnHelp = TRUE;
			}
			else
			{
				pchNewKey = pchArg;
			}
		}
	}

	if ((!blnHelp) && (!pchNewKey))
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

	if (m_rTree.IsCurrentRoot())
	{	// root key
		rConsole.Write(NK_CMD COMMAND_NA_ON_ROOT);
		return 0;
	}

	if (!m_rTree.NewKey(pchNewKey,blnVolatile))
	{
		rConsole.Write(_T("Cannot create key.\n"));
		rConsole.Write(m_rTree.GetLastErrorDescription());
	}
	return 0;
}

const TCHAR * CShellCommandNewKey::GetHelpString()
{
	return NK_CMD_SHORT_DESC
			_T("Syntax: ") NK_CMD _T(" [/v] [/?] Key_Name\n\n")
			_T("    /? - This help.\n\n")
			_T("    /v - Create volatile key. The information is stored in memory and is not\n")
			_T("         preserved when the corresponding registry hive is unloaded.\n");
}

const TCHAR * CShellCommandNewKey::GetHelpShortDescriptionString()
{
	return NK_CMD_SHORT_DESC;
}
