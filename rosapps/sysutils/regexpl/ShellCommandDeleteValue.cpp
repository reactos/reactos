/* $Id: ShellCommandDeleteValue.cpp,v 1.3 2001/01/10 01:25:29 narnaoud Exp $
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

BOOL CShellCommandDeleteValue::Match(const TCHAR *pszCommand)
{
	return _tcsicmp(pszCommand,DV_CMD) == 0;
}

int CShellCommandDeleteValue::Execute(CConsole &rConsole, CArgumentParser& rArguments)
{
	rArguments.ResetArgumentIteration();
	TCHAR *pszCommandItself = rArguments.GetNextArgument();

	TCHAR *pszParameter;
	TCHAR *pszValueFull = NULL;
	BOOL blnHelp = FALSE;
//	DWORD dwError;

	if ((_tcsnicmp(pszCommandItself,DV_CMD _T(".."),DV_CMD_LENGTH+2*sizeof(TCHAR)) == 0)||
		(_tcsnicmp(pszCommandItself,DV_CMD _T("\\"),DV_CMD_LENGTH+1*sizeof(TCHAR)) == 0))
	{
		pszValueFull = pszCommandItself + DV_CMD_LENGTH;
	}
	else if (_tcsnicmp(pszCommandItself,DV_CMD _T("/"),DV_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
	{
		pszParameter = pszCommandItself + DV_CMD_LENGTH;
		goto CheckValueArgument;
	}

	while((pszParameter = rArguments.GetNextArgument()) != NULL)
	{
CheckValueArgument:
		if ((_tcsicmp(pszParameter,_T("/?")) == 0)
			||(_tcsicmp(pszParameter,_T("-?")) == 0))
		{
			blnHelp = TRUE;
			break;
		}
		else if (!pszValueFull)
		{
			pszValueFull = pszParameter;
		}
		else
		{
			rConsole.Write(_T("Bad parameter: "));
			rConsole.Write(pszParameter);
			rConsole.Write(_T("\n"));
		}
	}
	
	CRegistryKey Key;
	TCHAR *pszValueName;
	const TCHAR *pszPath;
	
	if (blnHelp)
	{
		rConsole.Write(GetHelpString());
		return 0;
	}

	if (pszValueFull)
	{
		if (_tcscmp(pszValueFull,_T("\\")) == 0)
			goto CommandNAonRoot;

		TCHAR *pchSep = _tcsrchr(pszValueFull,_T('\\'));
		pszValueName = pchSep?(pchSep+1):(pszValueFull);
		pszPath = pchSep?pszValueFull:_T(".");
				
		//if (_tcsrchr(pszValueName,_T('.')))
		//{
		//	pszValueName = _T("");
		//	pszPath = pszValueFull;
		//}
		//else
		if (pchSep)
			*pchSep = 0;
	}
	else
	{
		pszValueName = _T("");
		pszPath = _T(".");
	}
  
  {
    size_t s = _tcslen(pszValueName);
    if (s && (pszValueName[0] == _T('\"'))&&(pszValueName[s-1] == _T('\"')))
    {
      pszValueName[s-1] = 0;
      pszValueName++;
    }
  }

  if (!m_rTree.GetKey(pszPath,KEY_SET_VALUE,Key))
  {
    rConsole.Write(m_rTree.GetLastErrorDescription());
    goto SkipCommand;
  }

	if (!Key.IsRoot())
	{	// not root key ???
		LONG nError = Key.DeleteValue(pszValueName);
		if (nError != ERROR_SUCCESS)
		{
			char Buffer[254];
			_stprintf(Buffer,_T("Cannot delete value. Error is %u\n"),(unsigned int)nError);
			rConsole.Write(Buffer);
		}
    else
    {
      InvalidateCompletion();
    }
	} // if (pKey)
	else
	{
CommandNAonRoot:
		rConsole.Write(DV_CMD COMMAND_NA_ON_ROOT);
	}

SkipCommand:
  //	if (pTree)
  //		delete pTree;
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
