/* $Id: ShellCommandDeleteValue.cpp,v 1.1 2000/10/04 21:04:31 ea Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (c) 1999-2000 Nedko Arnaoudov <nedkohome@atia.com>
 *
 * License: GNU GPL
 *
 */

// ShellCommandDeleteValue.cpp: implementation of the CShellCommandDeleteValue class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "ShellCommandDeleteValue.h"
#include "RegistryExplorer.h"

#define DV_CMD			_T("DV")
#define DV_CMD_LENGTH		COMMAND_LENGTH(DV_CMD)
#define DV_CMD_SHORT_DESC	DV_CMD _T(" command is used to delete value.\n")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellCommandDeleteValue::CShellCommandDeleteValue(CRegistryTree& rTree):m_rTree(rTree)
{
}

CShellCommandDeleteValue::~CShellCommandDeleteValue()
{
}

BOOL CShellCommandDeleteValue::Match(const TCHAR *pchCommand)
{
	return _tcsicmp(pchCommand,DV_CMD) == 0;
}

int CShellCommandDeleteValue::Execute(CConsole &rConsole, CArgumentParser& rArguments)
{
	rArguments.ResetArgumentIteration();
	TCHAR *pchCommandItself = rArguments.GetNextArgument();

	TCHAR *pchParameter;
	TCHAR *pchValueFull = NULL;
	BOOL blnHelp = FALSE;
//	DWORD dwError;

	if ((_tcsnicmp(pchCommandItself,DV_CMD _T(".."),DV_CMD_LENGTH+2*sizeof(TCHAR)) == 0)||
		(_tcsnicmp(pchCommandItself,DV_CMD _T("\\"),DV_CMD_LENGTH+1*sizeof(TCHAR)) == 0))
	{
		pchValueFull = pchCommandItself + DV_CMD_LENGTH;
	}
	else if (_tcsnicmp(pchCommandItself,DV_CMD _T("/"),DV_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
	{
		pchParameter = pchCommandItself + DV_CMD_LENGTH;
		goto CheckValueArgument;
	}

	while((pchParameter = rArguments.GetNextArgument()) != NULL)
	{
CheckValueArgument:
		if ((_tcsicmp(pchParameter,_T("/?")) == 0)
			||(_tcsicmp(pchParameter,_T("-?")) == 0))
		{
			blnHelp = TRUE;
			break;
		}
		else if (!pchValueFull)
		{
			pchValueFull = pchParameter;
		}
		else
		{
			rConsole.Write(_T("Bad parameter: "));
			rConsole.Write(pchParameter);
			rConsole.Write(_T("\n"));
		}
	}
	
	CRegistryTree *pTree = NULL;
	CRegistryKey *pKey = NULL;
	TCHAR *pchValueName;
	TCHAR *pchPath;
	
	if (blnHelp)
	{
		rConsole.Write(GetHelpString());
		return 0;
	}

	if (pchValueFull)
	{
		if (_tcscmp(pchValueFull,_T("\\")) == 0)
			goto CommandNAonRoot;

		TCHAR *pchSep = _tcsrchr(pchValueFull,_T('\\'));
		pchValueName = pchSep?(pchSep+1):(pchValueFull);
		pchPath = pchSep?pchValueFull:NULL;
				
		//if (_tcsrchr(pchValueName,_T('.')))
		//{
		//	pchValueName = _T("");
		//	pchPath = pchValueFull;
		//}
		//else
		if (pchSep)
			*pchSep = 0;
	}
	else
	{
		pchValueName = _T("");
		pchPath = NULL;
	}
		
	if (pchPath)
	{
		pTree = new CRegistryTree(m_rTree);
		if ((_tcscmp(pTree->GetCurrentPath(),m_rTree.GetCurrentPath()) != 0)
			||(!pTree->ChangeCurrentKey(pchPath)))
		{
			rConsole.Write(_T("Cannot open key "));
			rConsole.Write(pchPath);
			rConsole.Write(_T("\n"));
			goto SkipCommand;
		}
		else
		{
			pKey = pTree->GetCurrentKey();
		}
	}
	else
	{
		pKey = m_rTree.GetCurrentKey();
	}
	
	if (pKey)
	{	// not root key ???
	} // if (pKey)
	else
	{
CommandNAonRoot:
		rConsole.Write(DV_CMD COMMAND_NA_ON_ROOT);
	}

SkipCommand:
	if (pTree)
		delete pTree;
	return 0;
}

const TCHAR * CShellCommandDeleteValue::GetHelpString()
{
	return DV_CMD_SHORT_DESC
			_T("Syntax: ") DV_CMD _T(" [<PATH>][<VALUE_NAME>] [/?]\n\n")
			_T("    <PATH>       - Optional relative path of key which value will be delete.\n")
			_T("    <VALUE_NAME> - Name of key's value. Default is key's default value.\n")
			_T("    /? - This help.\n\n");
}

const TCHAR * CShellCommandDeleteValue::GetHelpShortDescriptionString()
{
	return DV_CMD_SHORT_DESC;
}
