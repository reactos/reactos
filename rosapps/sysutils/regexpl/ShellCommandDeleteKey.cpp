/* $Id: ShellCommandDeleteKey.cpp,v 1.3 2001/01/10 01:25:29 narnaoud Exp $
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
	TCHAR *pchKey = NULL, *pchArg;

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

  // search for last key name token
  TCHAR *pch = pchKey;
  while(*pch)
    pch++;

  if (pch > pchKey)
    pch--;

  while(*pch == _T('\\'))
    *pch = 0;

  while((pch > pchKey)&&(*pch != _T('\\')))
    pch--;

  ASSERT(pch >= pchKey);

  const TCHAR *pszPath;
  TCHAR *pszPattern = pch+1;
  if (pch == pchKey)
  {
    pszPath = _T(".");
  }
  else
  {
    if (pch-1 == pchKey)
    {
      rConsole.Write(DK_CMD COMMAND_NA_ON_ROOT);
      return 0;
    }
    else
    {
      *pch = 0;
      pszPath = pchKey;
    }
  }
  
  {
    size_t s = _tcslen(pszPattern);
    if (s && (pszPattern[0] == _T('\"'))&&(pszPattern[s-1] == _T('\"')))
    {
      pszPattern[s-1] = 0;
      pszPattern++;
    }
  }
  
	if (!m_rTree.DeleteSubkeys(pszPattern,pszPath,blnRecursive))
	{
		rConsole.Write(_T("Cannot delete key(s).\n"));
		rConsole.Write(m_rTree.GetLastErrorDescription());
	}
  else
  {
    InvalidateCompletion();
  }
  
	return 0;
}

const TCHAR * CShellCommandDeleteKey::GetHelpString()
{
	return DK_CMD_SHORT_DESC
			_T("Syntax: ") DK_CMD _T(" [/s] [/?] [PATH]KEY_NAME\n\n")
      _T("    PATH     - optional path to key which subkey(s) will be deleted. Default is current key.")
      _T("    KEY_NAME - name of key to be deleted. Wildcards can be used.")
			_T("    /?       - This help.\n\n")
			_T("    /s       - Delete key and all subkeys.\n");
}

const TCHAR * CShellCommandDeleteKey::GetHelpShortDescriptionString()
{
	return DK_CMD_SHORT_DESC;
}
