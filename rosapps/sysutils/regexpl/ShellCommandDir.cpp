/* $Id: ShellCommandDir.cpp,v 1.3 2001/01/10 01:25:29 narnaoud Exp $
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

	BOOL blnDo = TRUE,blnBadParameter, blnHelp = FALSE;
	const TCHAR *pszParameter;
	const TCHAR *pszCommandItself = rArguments.GetNextArgument();
  const TCHAR *pszKey = NULL;

	if ((_tcsnicmp(pszCommandItself,DIR_CMD _T(".."),DIR_CMD_LENGTH+2*sizeof(TCHAR)) == 0)||
		(_tcsnicmp(pszCommandItself,DIR_CMD _T("\\"),DIR_CMD_LENGTH+1*sizeof(TCHAR)) == 0))
	{
		pszKey = pszCommandItself + DIR_CMD_LENGTH;
	}
	else if (_tcsnicmp(pszCommandItself,DIR_CMD _T("/"),DIR_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
	{
		pszParameter = pszCommandItself + DIR_CMD_LENGTH;
		goto CheckDirArgument;
	}

	while((pszParameter = rArguments.GetNextArgument()) != NULL)
	{
CheckDirArgument:
		blnBadParameter = FALSE;
		if ((_tcsicmp(pszParameter,_T("/?")) == 0)
			||(_tcsicmp(pszParameter,_T("-?")) == 0))
		{
			blnHelp = TRUE;
			blnDo = pszKey != NULL;
		}
		else if (!pszKey)
		{
			pszKey = pszParameter;
			blnDo = TRUE;
		}
		else
		{
			blnBadParameter = TRUE;
		}
		if (blnBadParameter)
		{
			rConsole.Write(_T("Bad parameter: "));
			rConsole.Write(pszParameter);
			rConsole.Write(_T("\n"));
		}
	}
	
	CRegistryKey Key;

  if (!m_rTree.GetKey(pszKey?pszKey:_T("."),KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE,Key))
  {
    const TCHAR *pszErrorMsg = m_rTree.GetLastErrorDescription();
    rConsole.Write(pszErrorMsg);
    blnDo = FALSE;
  }

	if (blnHelp)
	{
		rConsole.Write(GetHelpString());
	}

	LONG nError;
	
	if (!blnDo)
    return 0;
  
  rConsole.Write(_T("\n Key is "));
  rConsole.Write(Key.GetKeyName());

  if (!Key.IsRoot())
  {
    rConsole.Write(_T("\n Last modify time is "));
    rConsole.Write(Key.GetLastWriteTime());
  }
  
  rConsole.Write(_T("\n\n"));
  unsigned __int64 nTotalItems = 0;
  
  try
  {
    ASSERT(nTotalItems == 0);
    rConsole.Write(_T("\t(KEY)\t\t\t\t..\\\n"));	// parent key abstraction
    nTotalItems = 1;
				
    DWORD dwMaxSubkeyNameLength;
    nError = Key.GetSubkeyNameMaxLength(dwMaxSubkeyNameLength);
    if (nError != ERROR_SUCCESS)
      throw nError;

    TCHAR *pszSubkeyNameBuffer = new TCHAR[dwMaxSubkeyNameLength];
    if (!pszSubkeyNameBuffer)
      throw ERROR_OUTOFMEMORY;
    
    Key.InitSubkeyEnumeration(pszSubkeyNameBuffer,dwMaxSubkeyNameLength);
    while ((nError = Key.GetNextSubkeyName()) == ERROR_SUCCESS)
    {
      rConsole.Write(_T("\t(KEY)\t\t\t\t"));
      rConsole.Write(pszSubkeyNameBuffer);
      rConsole.Write(_T("\\\n"));
      nTotalItems++;
    }

    delete pszSubkeyNameBuffer;
    
    if (nError != ERROR_NO_MORE_ITEMS)
      throw nError;

    DWORD dwMaxValueNameBufferSize;
    nError = Key.GetMaxValueNameLength(dwMaxValueNameBufferSize);
    if (nError != ERROR_SUCCESS)
      throw nError;
    
    TCHAR *pchValueNameBuffer = new TCHAR[dwMaxValueNameBufferSize];
    if (!pchValueNameBuffer)
      throw ERROR_OUTOFMEMORY;
    

    DWORD Type;
    Key.InitValueEnumeration(pchValueNameBuffer,
                             dwMaxValueNameBufferSize,
                             NULL,
                             0,
                             &Type);

    DWORD dwValueNameActualLength;
    const TCHAR *pszValueTypeName;
    unsigned int nTabSize = rConsole.GetTabWidth();
    unsigned int nTabs;
    while((nError = Key.GetNextValue(&dwValueNameActualLength)) == ERROR_SUCCESS)
    {
      rConsole.Write(_T("\t"));
      pszValueTypeName = CRegistryKey::GetValueTypeName(Type);
      nTabs = _tcslen(pszValueTypeName)/nTabSize;
      nTabs = (nTabs < 4)?(4-nTabs):1;
      rConsole.Write(pszValueTypeName);
      while(nTabs--)
        rConsole.Write(_T("\t"));
      rConsole.Write((dwValueNameActualLength == 0)?_T("(Default)"):pchValueNameBuffer);
      rConsole.Write(_T("\n"));
      nTotalItems++;
    }
    
    delete pchValueNameBuffer;
    
    if (nError != ERROR_NO_MORE_ITEMS)
      throw nError;
    
  }	// try
  catch (LONG nError)
  {
    rConsole.Write(_T("Error "));
    TCHAR Buffer[256];
    rConsole.Write(_itot(nError,Buffer,10));
    rConsole.Write(_T("\n"));
  }
		
  rConsole.Write(_T("\n Total: "));
  TCHAR Buffer[256];
  rConsole.Write(_ui64tot(nTotalItems,Buffer,10));
  rConsole.Write(_T(" item(s) listed.\n"));

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
