/* $Id: RegistryExplorer.cpp,v 1.2 2000/10/24 20:17:41 narnaoud Exp $
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

// Registry Explorer.cpp : Defines the entry point for the console application.
//

#include "ph.h"
#include "RegistryExplorer.h"
#include "Console.h"
#include "RegistryKey.h"
#include "RegistryTree.h"
#include "SecurityDescriptor.h"
#include "ArgumentParser.h"
#include "ShellCommandsLinkedList.h"
#include "ShellCommandExit.h"
#include "ShellCommandVersion.h"
#include "ShellCommandHelp.h"
#include "ShellCommandDir.h"
#include "ShellCommandChangeKey.h"
#include "ShellCommandValue.h"
#include "ShellCommandOwner.h"
#include "ShellCommandDACL.h"
#include "ShellCommandSACL.h"
#include "ShellCommandDOKA.h"
#include "ShellCommandConnect.h"
#include "ShellCommandNewKey.h"
#include "ShellCommandDeleteKey.h"
#include "ShellCommandSetValue.h"
#include "ShellCommandDeleteValue.h"

TCHAR pchCurrentKey[PROMPT_BUFFER_SIZE];

CRegistryTree Tree(PROMPT_BUFFER_SIZE+1);
CConsole Console;

const TCHAR * ppchRootKeys[7] = 
{
	_T("HKEY_CLASSES_ROOT"),
	_T("HKEY_CURRENT_USER"),
	_T("HKEY_LOCAL_MACHINE"),
	_T("HKEY_USERS"),
	_T("HKEY_PERFORMANCE_DATA"),
	_T("HKEY_CURRENT_CONFIG"),
	_T("HKEY_DYN_DATA")		//win9x only?
};

BOOL g_blnCompletionCycle = TRUE;

const TCHAR * CompletionCallback(unsigned __int64 & rnIndex, const BOOL *pblnForward, const TCHAR *pchContext, const TCHAR *pchBegin)
{
#define COMPLETION_BUFFER_SIZE	4096
	static TCHAR pchBuffer[COMPLETION_BUFFER_SIZE];
	CRegistryKey *pKey = NULL;
	CRegistryTree *pTree = NULL;
	unsigned __int64 nTotalKeys = 0;
	unsigned __int64 nTotalValues = 0;
	unsigned __int64 nTotalItems = 0;
	class CompletionMatch
	{
	public:
		CompletionMatch(const TCHAR *pchText, CompletionMatch **pNext)
		{
			BOOL b = _tcschr(pchText,_T(' ')) != NULL;
			size_t s = _tcslen(pchText);
			m_pchText = new TCHAR [s+(b?3:1)];
			if (b)
			{
				m_pchText[0] = _T('\"');
				_tcscpy(m_pchText+1,pchText);
				m_pchText[s+1] = _T('\"');
				m_pchText[s+2] = 0;
			}
			else
			{
				_tcscpy(m_pchText,pchText);
			}
			if (m_pchText)
			{
				m_pNext = *pNext;
				*pNext = this;
			}
		}
		~CompletionMatch()
		{
			if (m_pchText)
				delete m_pchText;
			if (m_pNext)
				delete m_pNext;
		}
		const TCHAR *GetText(unsigned __int64 dwReverseIndex)
		{
			if (dwReverseIndex)
			{
				if (m_pNext)
					return m_pNext->GetText(dwReverseIndex-1);
				else
					return NULL;
			}
			return m_pchText;
		}
	private:
		TCHAR *m_pchText;
		CompletionMatch *m_pNext;
	};
	CompletionMatch *pRootMatch = NULL;

	BOOL blnCompletionOnKeys = TRUE;
	BOOL blnCompletionOnValues = TRUE;

	while(*pchContext && _istspace(*pchContext))
	{
		pchContext++;
	}

/*	if ((_tcsnicmp(pchContext,DIR_CMD,DIR_CMD_LENGTH) == 0)||
		(_tcsnicmp(pchContext,CD_CMD,CD_CMD_LENGTH) == 0)||
		(_tcsnicmp(pchContext,OWNER_CMD,OWNER_CMD_LENGTH) == 0)||
		(_tcsnicmp(pchContext,DACL_CMD,DACL_CMD_LENGTH) == 0)||
		(_tcsnicmp(pchContext,SACL_CMD,SACL_CMD_LENGTH) == 0))
	{
		blnCompletionOnValues = FALSE;
	}*/
//	else if (_tcsnicmp(pchContext,VALUE_CMD,VALUE_CMD_LENGTH) == 0)
//	{
//		blnCompletionOnKeys = FALSE;
//	}

	const TCHAR *pchKey = _tcsrchr(pchBegin,_T('\\'));
	DWORD nBufferOffset = 0;
	if (pchKey)
	{
		nBufferOffset = pchKey-pchBegin+1;
		if (nBufferOffset >= COMPLETION_BUFFER_SIZE-1)
		{
			if (pRootMatch) delete pRootMatch;
			if (pTree)
				delete pTree;
			return _T("internal error");
		}
		_tcsncpy(pchBuffer,pchBegin,nBufferOffset);
		pchBuffer[nBufferOffset] = 0;
		pchBegin = pchKey+1;
		pTree = new CRegistryTree(Tree);
		if ((_tcscmp(pTree->GetCurrentPath(),Tree.GetCurrentPath()) != 0)||(!pTree->ChangeCurrentKey(pchBuffer)))
		{
			if (pTree)
				delete pTree;
			return NULL;
		}
		else
		{
			pKey = pTree->GetCurrentKey();
		}
	}
	else
	{
		pKey = Tree.GetCurrentKey();
	}

	if (!pKey)
	{
		for(unsigned int i = 0 ; i < sizeof(ppchRootKeys)/sizeof(TCHAR *) ; i++)
		{
			nTotalKeys++;
			nTotalItems++;
			CompletionMatch *p = new CompletionMatch(ppchRootKeys[i],&pRootMatch);
			if (!p || !p->GetText(0))
			{
				if (pRootMatch) delete pRootMatch;
				if (pTree)
					delete pTree;
				return _T("Out of memory");
			}
		}
	}
	else
	{
		CompletionMatch *p;
		DWORD dwError;
		if (blnCompletionOnKeys)
		{
/*			if (_tcslen(pchBegin) == 0)
			{
				p = new CompletionMatch(_T(".."),&pRootMatch);
				if (!p || !p->GetText(0))
				{
					if (pRootMatch) delete pRootMatch;
					if (pTree)
						delete pTree;
					return _T("Out of memory");
				}
				nTotalKeys++;
				nTotalItems++;
			}
*/			
			pKey->InitSubKeyEnumeration();
			TCHAR *pchSubKeyName;
			while ((pchSubKeyName = pKey->GetSubKeyName(dwError)) != NULL)
			{
				if (_tcsnicmp(pchSubKeyName,pchBegin,_tcslen(pchBegin)) == 0)
				{
					nTotalKeys++;
					nTotalItems++;
					p = new CompletionMatch(pchSubKeyName,&pRootMatch);
					if (!p || !p->GetText(0))
					{
						if (pRootMatch) delete pRootMatch;
						if (pTree)
							delete pTree;
						return _T("Out of memory");
					}
				}
			}
			if ((dwError != ERROR_SUCCESS)&&(dwError != ERROR_NO_MORE_ITEMS))
			{
				if (pRootMatch) delete pRootMatch;
				if (pTree)
					delete pTree;
				return _T("error");
			}
		}
		
		if (blnCompletionOnValues)
		{
			pKey->InitValueEnumeration();
			TCHAR *pchValueName;
			DWORD dwValueNameLength, dwMaxValueNameLength;
			dwError = pKey->GetMaxValueNameLength(dwMaxValueNameLength);
			if (dwError != ERROR_SUCCESS)
			{
				if (pRootMatch) delete pRootMatch;
				if (pTree)
					delete pTree;
				return _T("error");
			}
			
			dwMaxValueNameLength++;
			pchValueName = new TCHAR [dwMaxValueNameLength];
			if (!pchValueName)
			{
				if (pRootMatch) delete pRootMatch;
				if (pTree)
					delete pTree;
				return _T("Out of memory");
			}
			for(;;)
			{
				dwValueNameLength = dwMaxValueNameLength;
				//dwValueSize = dwMaxValueSize;
				dwError = pKey->GetNextValue(pchValueName,dwValueNameLength,NULL,NULL,NULL);
				if (dwError == ERROR_NO_MORE_ITEMS) break;
				if (dwError != ERROR_SUCCESS)
				{
					if (pRootMatch) delete pRootMatch;
					if (pTree)
						delete pTree;
					return _T("error");
				}
				
				if (((dwValueNameLength == 0) && (_tcslen(pchBegin) == 0))||
					(_tcsnicmp(pchValueName,pchBegin,_tcslen(pchBegin)) == 0))
				{
					nTotalValues++;
					nTotalItems++;
					p = new CompletionMatch((dwValueNameLength == 0)?_T("(Default)"):pchValueName,&pRootMatch);
					if (!p || !p->GetText(0))
					{
						if (pRootMatch) delete pRootMatch;
						if (pTree)
							delete pTree;
						return _T("Out of memory");
					}
				}
			}	// for
			delete [] pchValueName;
		}
	}
	if (rnIndex >= nTotalItems)
		rnIndex = nTotalItems-1;
	if (pblnForward)
	{
		if (*pblnForward)
		{
			rnIndex++;
			if (rnIndex >= nTotalItems)
			{
				if (g_blnCompletionCycle)
					rnIndex = 0;
				else
					rnIndex--;
			}
		}
		else
		{
			if (rnIndex)
				rnIndex--;
			else if (g_blnCompletionCycle)
				rnIndex = nTotalItems-1;
		}
	}
	
	const TCHAR *pchName;
	if (nTotalItems == 0)
	{
		if (pRootMatch)
			delete pRootMatch;
		if (pTree)
			delete pTree;
		return NULL;
	}
	ASSERT(rnIndex < nTotalItems);
	pchName = pRootMatch->GetText(nTotalItems-rnIndex-1);
	if (pchName == NULL)
	{
		if (pRootMatch)
			delete pRootMatch;
		if (pTree)
			delete pTree;
		return _T("internal error");
	}
	DWORD dwBufferFull = _tcslen(pchName);
	if (dwBufferFull >= COMPLETION_BUFFER_SIZE-nBufferOffset)
	{
		dwBufferFull = COMPLETION_BUFFER_SIZE-1-nBufferOffset;
	}
	_tcsncpy(pchBuffer+nBufferOffset,pchName,dwBufferFull);
	if ((dwBufferFull < COMPLETION_BUFFER_SIZE-1)&&(rnIndex < nTotalKeys))
		pchBuffer[nBufferOffset+dwBufferFull++] = _T('\\');
	pchBuffer[nBufferOffset+dwBufferFull] = 0;
	if (pTree)
		delete pTree;
	if (pRootMatch)
		delete pRootMatch;
	return pchBuffer;
}

BOOL blnCommandExecutionInProgress = FALSE;

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	switch(dwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		if (blnCommandExecutionInProgress)
		{
/*			Beep(1000,100);
			Beep(2000,100);
			Beep(3000,100);
			Beep(4000,100);
			Beep(5000,100);
			Beep(4000,100);
			Beep(3000,100);
			Beep(2000,100);
			Beep(1000,100);*/
			Console.Write(_T("\n"));
			Console.DisableWrite();
		}
		return TRUE;
	default:
		return FALSE;
	}
}

//int _tmain(/*int argc, TCHAR* argv[], TCHAR* envp[]*/)
int main ()
{
	CShellCommandsLinkedList CommandsList(Console);

	CShellCommandExit ExitCommand;
	CommandsList.AddCommand(&ExitCommand);

	CShellCommandVersion VersionCommand;
	CommandsList.AddCommand(&VersionCommand);

	CShellCommandHelp HelpCommand(CommandsList);
	CommandsList.AddCommand(&HelpCommand);

	CShellCommandDir DirCommand(Tree);
	CommandsList.AddCommand(&DirCommand);

	CShellCommandChangeKey ChangeKeyCommand(Tree);
	CommandsList.AddCommand(&ChangeKeyCommand);

	CShellCommandValue ValueCommand(Tree);
	CommandsList.AddCommand(&ValueCommand);

	CShellCommandOwner OwnerCommand(Tree);
	CommandsList.AddCommand(&OwnerCommand);

	CShellCommandDACL DACLCommand(Tree);
	CommandsList.AddCommand(&DACLCommand);

	CShellCommandSACL SACLCommand(Tree);
	CommandsList.AddCommand(&SACLCommand);

	CShellCommandDOKA DOKACommand(Tree);
	CommandsList.AddCommand(&DOKACommand);

	CShellCommandConnect ConnectCommand(Tree);
	CommandsList.AddCommand(&ConnectCommand);

	CShellCommandNewKey NewKeyCommand(Tree);
	CommandsList.AddCommand(&NewKeyCommand);

	CShellCommandDeleteKey DeleteKeyCommand(Tree);
	CommandsList.AddCommand(&DeleteKeyCommand);

	CShellCommandSetValue SetValueCommand(Tree);
	CommandsList.AddCommand(&SetValueCommand);

	CShellCommandDeleteValue DeleteValueCommand(Tree);
	CommandsList.AddCommand(&DeleteValueCommand);

	CArgumentParser Parser;

	int nRetCode = 0;

// input buffer size in chars
#define INPUT_BUFFER_SIZE	1024
//#define INPUT_BUFFER_SIZE	128
//#define INPUT_BUFFER_SIZE	10

	TCHAR *pchCommand;

	pchCommand = Console.Init(INPUT_BUFFER_SIZE,10);
	if (pchCommand == NULL)
	{
		_ftprintf(stderr,_T("Cannot initialize console.\n"));
		goto Abort;
	}

	Console.SetReplaceCompletionCallback(CompletionCallback);

	WORD wOldConsoleAttribute;
	if (!Console.GetTextAttribute(wOldConsoleAttribute)) goto Abort;

	Console.SetTitle(_T("Registry Explorer"));
	Console.SetTextAttribute(FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_INTENSITY);

	VERIFY(SetConsoleCtrlHandler((PHANDLER_ROUTINE)HandlerRoutine,TRUE));

	if (!Console.Write(HELLO_MSG
	//(_L(__TIMESTAMP__))
	)) goto Abort;

	Tree.SetDesiredOpenKeyAccess(KEY_READ);

GetCommand:
	// prompt
	// TODO: make prompt user-customizable
	Console.EnableWrite();
	_tcscpy(pchCurrentKey,Tree.GetCurrentPath());
	Console.Write(pchCurrentKey);
	Console.Write(_T("\n# "));
	Console.FlushInputBuffer();

	blnCommandExecutionInProgress = FALSE;

	// Set command line color
//	Console.SetTextAttribute(/*FOREGROUND_BLUE|*/FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_INTENSITY);
	if (!Console.ReadLine()) goto Abort;

	// Set normal color
//	Console.SetTextAttribute(FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_INTENSITY);

	Console.BeginScrollingOperation();
	blnCommandExecutionInProgress = TRUE;


	// Parse command line (1st step - convert to multi sz)
	Parser.SetArgumentList(pchCommand);

	int nCommandReturnValue;
	switch(CommandsList.Execute(Parser,nCommandReturnValue))
	{
	case -1:	// not recognized command
		{
			Parser.ResetArgumentIteration();
			TCHAR *pchCommandItself = Parser.GetNextArgument();
			size_t cmdlen = _tcslen(pchCommandItself);
			if ((!cmdlen)||
				(pchCommandItself[cmdlen-1] != _T('\\'))||
				(Parser.GetNextArgument())||
				(!Tree.ChangeCurrentKey(pchCommandItself)))
			{
				Console.Write(_T("Unknown command \""));
				Console.Write(pchCommandItself);
				Console.Write(_T("\"\n"));
			}
		}
	case -2:	// empty line
		goto GetCommand;
	case 0:	// exit command
		nRetCode = 0;
		Console.SetTextAttribute(wOldConsoleAttribute);
		goto Exit;
	default:
		Console.Write(_T("\n"));
		goto GetCommand;
	}

Abort:
	_ftprintf(stderr,_T("Abnormal program termination.\nPlease report bugs to ") EMAIL _T("\n"));
	nRetCode = 1;
Exit:
	return nRetCode;
}
