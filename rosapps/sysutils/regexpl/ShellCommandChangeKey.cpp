/* $Id: ShellCommandChangeKey.cpp,v 1.1 2000/10/04 21:04:30 ea Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (c) 1999-2000 Nedko Arnaoudov <nedkohome@atia.com>
 *
 * License: GNU GPL
 *
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

