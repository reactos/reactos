/* $Id: ShellCommandDOKA.cpp,v 1.2 2000/10/24 20:17:41 narnaoud Exp $
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

// ShellCommandDOKA.cpp: implementation of the CShellCommandDOKA class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "ShellCommandDOKA.h"
#include "RegistryExplorer.h"
#include "SecurityDescriptor.h"

#define DOKA_CMD			_T("DOKA")
#define DOKA_CMD_SHORT_DESC	DOKA_CMD _T(" command is used to view/edit Desired Open Key Access.\n")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellCommandDOKA::CShellCommandDOKA(CRegistryTree& rTree):m_rTree(rTree)
{
}

CShellCommandDOKA::~CShellCommandDOKA()
{
}

BOOL CShellCommandDOKA::Match(const TCHAR *pchCommand)
{
	return _tcsicmp(pchCommand,DOKA_CMD) == 0;
}

int CShellCommandDOKA::Execute(CConsole &rConsole, CArgumentParser& rArguments)
{
	REGSAM Access = m_rTree.GetDesiredOpenKeyAccess();
	const TCHAR *pchParameter;
	BOOL blnBadParameter = FALSE;
	BOOL blnHelp = FALSE;

	while((pchParameter = rArguments.GetNextArgument()) != NULL)
	{
		blnBadParameter = FALSE;
//			Console.Write(_T("Processing parameter: \")");
//			Console.Write(pchParameter);
//			Console.Write(_T("\")\n");
		if ((_tcsicmp(pchParameter,_T("/?")) == 0)
			||(_tcsicmp(pchParameter,_T("-?")) == 0))
		{
			blnHelp = TRUE;
		}
		else if (*pchParameter == _T('-'))
		{
			TCHAR a = *(pchParameter+1);
			if (a == 0)
			{
				Access = 0;
			}
			else
			{
				if (*(pchParameter+2) != 0)
				{
					blnBadParameter = TRUE;
				}
				else
				{
					switch(a)
					{
					case _T('l'):	// KEY_CREATE_LINK
					case _T('L'):
						Access &= ~KEY_CREATE_LINK;
						break;
					case _T('c'):	// KEY_CREATE_SUB_KEY
					case _T('C'):
						Access &= ~KEY_CREATE_SUB_KEY;
						break;
					case _T('e'):	// KEY_ENUMERATE_SUB_KEYS
					case _T('E'):
						Access &= ~KEY_ENUMERATE_SUB_KEYS;
						break;
					case _T('n'):	// KEY_NOTIFY
					case _T('N'):
						Access &= ~KEY_NOTIFY;
						break;
					case _T('q'):	// KEY_QUERY_VALUE
					case _T('Q'):
						Access &= ~KEY_QUERY_VALUE;
						break;
					case _T('s'):	// KEY_SET_VALUE
					case _T('S'):
						Access &= ~KEY_SET_VALUE;
						break;
					default:
						blnBadParameter = TRUE;
					} // switch
				} // else (*(pchParameter+2) != 0)
			} // else (a == 0)
		} // if (*pchParameter == _T('-'))
		else if (*pchParameter == _T('+'))
		{
			TCHAR a = *(pchParameter+1);
			if (a == 0)
			{
				blnBadParameter = TRUE;
			}
			else
			{
				if (*(pchParameter+2) != 0)
				{
					blnBadParameter = TRUE;
				}
				else
				{
					switch(a)
					{
					case _T('a'):	// KEY_ALL_ACCESS
					case _T('A'):
						Access |= KEY_ALL_ACCESS;
						break;
					case _T('l'):	// KEY_CREATE_LINK
					case _T('L'):
						Access |= KEY_CREATE_LINK;
						break;
					case _T('c'):	// KEY_CREATE_SUB_KEY
					case _T('C'):
						Access |= KEY_CREATE_SUB_KEY;
						break;
					case _T('e'):	// KEY_ENUMERATE_SUB_KEYS
					case _T('E'):
						Access |= KEY_ENUMERATE_SUB_KEYS;
						break;
					case _T('n'):	// KEY_NOTIFY
					case _T('N'):
						Access |= KEY_NOTIFY;
						break;
					case _T('q'):	// KEY_QUERY_VALUE
					case _T('Q'):
						Access |= KEY_QUERY_VALUE;
						break;
					case _T('s'):	// KEY_SET_VALUE
					case _T('S'):
						Access |= KEY_SET_VALUE;
						break;
//						case _T('X'):	// KEY_EXECUTE
//						case _T('x'):
//							Access |= KEY_EXECUTE;
//							break;
					case _T('R'):	// KEY_READ
					case _T('r'):
						Access |= KEY_READ;
						break;
					default:
						blnBadParameter = TRUE;
					} // switch
				} // else (*(pchParameter+2) != 0)
			} // else (a == 0)
		} // if (*pchParameter == _T('-'))
		else
		{
			blnBadParameter = TRUE;
		}

		if (blnBadParameter)
		{
			rConsole.Write(_T("Bad parameter: "));
			rConsole.Write(pchParameter);
			rConsole.Write(_T("\n"));
			blnHelp = TRUE;
		}
	} // while((pchParameter = Parser.GetNextArgument()) != NULL)

	if (blnHelp)
	{
		rConsole.Write(GetHelpString());
	}
	else
	{
		m_rTree.SetDesiredOpenKeyAccess(Access);
		rConsole.Write(_T("Desired open key access:\n"));
		REGSAM Access = m_rTree.GetDesiredOpenKeyAccess();
		if (Access & KEY_CREATE_LINK)
		{
			rConsole.Write(_T("\tKEY_CREATE_LINK - Permission to create a symbolic link.\n"));
		}
		if (Access & KEY_CREATE_SUB_KEY)
		{
			rConsole.Write(_T("\tKEY_CREATE_SUB_KEY - Permission to create subkeys.\n"));
		}
		if (Access & KEY_ENUMERATE_SUB_KEYS)
		{
			rConsole.Write(_T("\tKEY_ENUMERATE_SUB_KEYS - Permission to enumerate subkeys.\n"));
		}
		if (Access & KEY_NOTIFY)
		{
			rConsole.Write(_T("\tKEY_NOTIFY - Permission for change notification.\n"));
		}
		if (Access & KEY_QUERY_VALUE)
		{
			rConsole.Write(_T("\tKEY_QUERY_VALUE - Permission to query subkey data.\n"));
		}
		if (Access & KEY_SET_VALUE)
		{
			rConsole.Write(_T("\tKEY_SET_VALUE - Permission to set subkey data.\n"));
		}
	}
	return 0;
}

const TCHAR * CShellCommandDOKA::GetHelpString()
{
	return DOKA_CMD_SHORT_DESC
			_T("Syntax: ") DOKA_CMD _T(" [Switches] [/?]\n\n")
			_T("    /? - This help.\n\n")
			_T("Switches are:\n")
			_T("    -  - Reset all permisions.\n")
			_T("    -l - Reset permission to create a symbolic link.\n")
			_T("    -c - Reset permission to create subkeys.\n")
			_T("    -e - Reset permission to enumerate subkeys.\n")
			_T("    -n - Reset permission for change notification.\n")
			_T("    -q - Reset permission to query subkey data.\n")
			_T("    -s - Reset permission to set subkey data.\n")
			_T("    +a - Set all permisions.\n")
			_T("    +l - Set permission to create a symbolic link.\n")
			_T("    +c - Set permission to create subkeys.\n")
			_T("    +e - Set permission to enumerate subkeys.\n")
			_T("    +n - Set permission for change notification.\n")
			_T("    +q - Set permission to query subkey data.\n")
			_T("    +s - Set permission to set subkey data.\n")
			_T("    +r - Equivalent to combination of +q , +e  and +n\n\n")
			_T("Without parameters, command displays current Desired Open Key Access.\n");
}

const TCHAR * CShellCommandDOKA::GetHelpShortDescriptionString()
{
	return DOKA_CMD_SHORT_DESC;
}
