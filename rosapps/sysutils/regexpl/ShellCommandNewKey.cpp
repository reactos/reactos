/* $Id: ShellCommandNewKey.cpp,v 1.1 2000/10/04 21:04:31 ea Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (c) 1999-2000 Nedko Arnaoudov <nedkohome@atia.com>
 *
 * License: GNU GPL
 *
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
