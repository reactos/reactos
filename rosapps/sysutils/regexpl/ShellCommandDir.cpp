/* $Id: ShellCommandDir.cpp,v 1.2 2000/10/24 20:17:41 narnaoud Exp $
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

// ShellCommandDir.cpp: implementation of the CShellCommandDir class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "RegistryExplorer.h"
#include "ShellCommandDir.h"
#include "RegistryTree.h"
#include "RegistryKey.h"

// *** THIS SHOULD GO IN A MINGW/ROS HEADER (tchar.h ???) - Begin
#if 1	// #ifndef _ui64tot ???
	#ifdef  _UNICODE
			#define _ui64tot    _ui64tow
	#else
			#define _ui64tot    _ui64toa
	#endif
#endif
// *** THIS SHOULD GO IN A MINGW/ROS HEADER - End

#define DIR_CMD				_T("DIR")
#define DIR_CMD_LENGTH		COMMAND_LENGTH(DIR_CMD)
#define DIR_CMD_SHORT_DESC	DIR_CMD _T(" command lists keys and values of any key.\n")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellCommandDir::CShellCommandDir(CRegistryTree& rTree):m_rTree(rTree)
{
}

CShellCommandDir::~CShellCommandDir()
{
}

BOOL CShellCommandDir::Match(const TCHAR *pchCommand)
{
	if (_tcsicmp(pchCommand,DIR_CMD) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,DIR_CMD _T(".."),DIR_CMD_LENGTH+2*sizeof(TCHAR)) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,DIR_CMD _T("/"),DIR_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,DIR_CMD _T("\\"),DIR_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
		return TRUE;
	return FALSE;
}

int CShellCommandDir::Execute(CConsole &rConsole, CArgumentParser& rArguments)
{
	rArguments.ResetArgumentIteration();

	const TCHAR *pchKey = NULL;
	BOOL blnDo = TRUE,blnBadParameter, blnHelp = FALSE;
	const TCHAR *pchParameter;
	const TCHAR *pchCommandItself = rArguments.GetNextArgument();

	if ((_tcsnicmp(pchCommandItself,DIR_CMD _T(".."),DIR_CMD_LENGTH+2*sizeof(TCHAR)) == 0)||
		(_tcsnicmp(pchCommandItself,DIR_CMD _T("\\"),DIR_CMD_LENGTH+1*sizeof(TCHAR)) == 0))
	{
		pchKey = pchCommandItself + DIR_CMD_LENGTH;
	}
	else if (_tcsnicmp(pchCommandItself,DIR_CMD _T("/"),DIR_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
	{
		pchParameter = pchCommandItself + DIR_CMD_LENGTH;
		goto CheckDirArgument;
	}

	while((pchParameter = rArguments.GetNextArgument()) != NULL)
	{
CheckDirArgument:
		blnBadParameter = FALSE;
		if ((_tcsicmp(pchParameter,_T("/?")) == 0)
			||(_tcsicmp(pchParameter,_T("-?")) == 0))
		{
			blnHelp = TRUE;
			blnDo = pchKey != NULL;
		}
		else if (!pchKey)
		{
			pchKey = pchParameter;
			blnDo = TRUE;
		}
		else
		{
			blnBadParameter = TRUE;
		}
		if (blnBadParameter)
		{
			rConsole.Write(_T("Bad parameter: "));
			rConsole.Write(pchParameter);
			rConsole.Write(_T("\n"));
		}
	}
	
	CRegistryTree *pTree = NULL;
	CRegistryKey *pKey = NULL;
	if (pchKey)
	{
		pTree = new CRegistryTree(m_rTree);
		if ((_tcscmp(pTree->GetCurrentPath(),m_rTree.GetCurrentPath()) != 0)||
			(!pTree->ChangeCurrentKey(pchKey)))
		{
			rConsole.Write(_T("Cannot open key "));
			rConsole.Write(pchKey);
			rConsole.Write(_T("\n"));
			//blnHelp = TRUE;
			blnDo = FALSE;
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
	
	if (blnHelp)
	{
		rConsole.Write(GetHelpString());
	}

	DWORD dwError;
	
	if (blnDo)
	{
		rConsole.Write(_T("\n Key is "));
//		rConsole.Write(_T("\\"));
		rConsole.Write(pTree?pTree->GetCurrentPath():m_rTree.GetCurrentPath());
		if (pKey)
		{
			rConsole.Write(_T("\n Last modify time is "));
			rConsole.Write(pKey->GetLastWriteTime());
		}
		rConsole.Write(_T("\n\n"));
		unsigned __int64 nTotalItems = 0;
		if (!pKey)
		{
			rConsole.Write(_T("\t(KEY)\t\t\t\tHKEY_CLASSES_ROOT\\\n"));
			rConsole.Write(_T("\t(KEY)\t\t\t\tHKEY_CURRENT_USER\\\n"));
			rConsole.Write(_T("\t(KEY)\t\t\t\tHKEY_LOCAL_MACHINE\\\n"));
			rConsole.Write(_T("\t(KEY)\t\t\t\tHKEY_USERS\\\n"));
			rConsole.Write(_T("\t(KEY)\t\t\t\tHKEY_PERFORMANCE_DATA\\\n"));
			rConsole.Write(_T("\t(KEY)\t\t\t\tHKEY_CURRENT_CONFIG\\\n"));
			rConsole.Write(_T("\t(KEY)\t\t\t\tHKEY_DYN_DATA\\\n"));
			nTotalItems = 7;
		}
		else
		{
			dwError = ERROR_SUCCESS;
			try
			{
				if (!pKey->IsPredefined())
				{
					dwError = pKey->Open(KEY_QUERY_VALUE|KEY_READ);
					if (dwError != ERROR_SUCCESS) throw dwError;
				}
				
				ASSERT(nTotalItems == 0);
				rConsole.Write(_T("\t(KEY)\t\t\t\t..\\\n"));	// parent key abstraction
				nTotalItems = 1;
				
				pKey->InitSubKeyEnumeration();
				TCHAR *pchSubKeyName;
				while ((pchSubKeyName = pKey->GetSubKeyName(dwError)) != NULL)
				{
					rConsole.Write(_T("\t(KEY)\t\t\t\t"));
					rConsole.Write(pchSubKeyName);
					rConsole.Write(_T("\\\n"));
					nTotalItems++;
				}
				if ((dwError != ERROR_SUCCESS)&&(dwError != ERROR_NO_MORE_ITEMS)) throw dwError;
				
				pKey->InitValueEnumeration();
				TCHAR *pchValueName;
				DWORD dwValueNameLength, dwMaxValueNameLength;
				dwError = pKey->GetMaxValueNameLength(dwMaxValueNameLength);
				if (dwError != ERROR_SUCCESS) throw dwError;
				dwMaxValueNameLength++;
				pchValueName = new TCHAR [dwMaxValueNameLength];
				DWORD Type;
				for(;;)
				{
					dwValueNameLength = dwMaxValueNameLength;
					//dwValueSize = dwMaxValueSize;
					dwError = pKey->GetNextValue(pchValueName,dwValueNameLength,&Type,
						NULL,//pDataBuffer
						NULL//&dwValueSize
						);
					if (dwError == ERROR_NO_MORE_ITEMS) break;
					if (dwError != ERROR_SUCCESS) throw dwError;
					rConsole.Write(_T("\t"));
					rConsole.Write(CRegistryKey::GetValueTypeName(Type));
					rConsole.Write(_T("\t"));
					rConsole.Write((dwValueNameLength == 0)?_T("(Default)"):pchValueName);
					rConsole.Write(_T("\n"));
					nTotalItems++;
				}	// for
				delete [] pchValueName;
			}	// try
			catch (DWORD dwError)
			{
				rConsole.Write(_T("Error "));
				TCHAR Buffer[256];
				rConsole.Write(_itot(dwError,Buffer,10));
				rConsole.Write(_T("\n"));
			}
		}	// else (Tree.IsCurrentRoot())
		
		rConsole.Write(_T("\n Total: "));
		TCHAR Buffer[256];
		rConsole.Write(_ui64tot(nTotalItems,Buffer,10));
		rConsole.Write(_T(" item(s) listed.\n"));
		if (pTree) delete pTree;
	}	// if (blnDo)

	return 0;
}

const TCHAR * CShellCommandDir::GetHelpString()
{
	return DIR_CMD_SHORT_DESC
			_T("Syntax: ") DIR_CMD _T(" [<KEY>] [/?]\n\n")
			_T("    <KEY> - Optional relative path to the key on which command will be executed\n")
			_T("    /?    - This help.\n\n")
			_T("Without parameters, command lists keys and values of current key.\n");
}

const TCHAR * CShellCommandDir::GetHelpShortDescriptionString()
{
	return DIR_CMD_SHORT_DESC;
}
