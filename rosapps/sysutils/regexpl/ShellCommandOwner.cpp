/* $Id: ShellCommandOwner.cpp,v 1.1 2000/10/04 21:04:31 ea Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (c) 1999-2000 Nedko Arnaoudov <nedkohome@atia.com>
 *
 * License: GNU GPL
 *
 */

// ShellCommandOwner.cpp: implementation of the CShellCommandOwner class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "ShellCommandOwner.h"
#include "RegistryExplorer.h"
#include "SecurityDescriptor.h"

#define OWNER_CMD			_T("OWNER")
#define OWNER_CMD_LENGTH	COMMAND_LENGTH(OWNER_CMD)
#define OWNER_CMD_SHORT_DESC	OWNER_CMD _T(" command is used to view")/*"/change"*/_T(" key's owner.\n")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellCommandOwner::CShellCommandOwner(CRegistryTree& rTree):m_rTree(rTree)
{
}

CShellCommandOwner::~CShellCommandOwner()
{
}

BOOL CShellCommandOwner::Match(const TCHAR *pchCommand)
{
	if (_tcsicmp(pchCommand,OWNER_CMD) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,OWNER_CMD _T(".."),OWNER_CMD_LENGTH+2*sizeof(TCHAR)) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,OWNER_CMD _T("/") ,OWNER_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,OWNER_CMD _T("\\"),OWNER_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
		return TRUE;
	return FALSE;
}

int CShellCommandOwner::Execute(CConsole &rConsole, CArgumentParser& rArguments)
{
	const TCHAR *pchKey = NULL;
	BOOL blnDo = TRUE;
	BOOL blnBadParameter = FALSE;
	BOOL blnHelp = FALSE;
	const TCHAR *pchParameter;
	DWORD dwError;

	rArguments.ResetArgumentIteration();
	const TCHAR *pchCommandItself = rArguments.GetNextArgument();

	if ((_tcsnicmp(pchCommandItself,OWNER_CMD _T(".."),OWNER_CMD_LENGTH+2*sizeof(TCHAR)) == 0)||
		(_tcsnicmp(pchCommandItself,OWNER_CMD _T("\\"),OWNER_CMD_LENGTH+1*sizeof(TCHAR)) == 0))
	{
		pchKey = pchCommandItself + OWNER_CMD_LENGTH;
	}
	else if (_tcsnicmp(pchCommandItself,OWNER_CMD _T("/"),OWNER_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
	{
		pchParameter = pchCommandItself + OWNER_CMD_LENGTH;
		goto CheckOwnerArgument;
	}

	while((pchParameter = rArguments.GetNextArgument()) != NULL)
	{
CheckOwnerArgument:
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
		if ((_tcscmp(pTree->GetCurrentPath(),m_rTree.GetCurrentPath()) != 0)||(!pTree->ChangeCurrentKey(pchKey)))
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

	if (blnDo&&blnHelp) rConsole.Write(_T("\n"));

	if (blnDo)
	{
		if (pKey == NULL)
		{	// root key
			rConsole.Write(OWNER_CMD COMMAND_NA_ON_ROOT);
		}
		else
		{
			PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
			TCHAR *pchName = NULL, *pchDomainName = NULL;
			try
			{
				DWORD dwSecurityDescriptorLength;
				rConsole.Write(_T("Key : "));
				rConsole.Write(_T("\\"));
				rConsole.Write(pTree?pTree->GetCurrentPath():m_rTree.GetCurrentPath());
				rConsole.Write(_T("\n"));
				dwError = pKey->GetSecurityDescriptorLength(&dwSecurityDescriptorLength);
				if (dwError != ERROR_SUCCESS) throw dwError;
				
				pSecurityDescriptor = (PSECURITY_DESCRIPTOR) new unsigned char [dwSecurityDescriptorLength];
				DWORD dwSecurityDescriptorLength1 = dwSecurityDescriptorLength;
				dwError = pKey->GetSecurityDescriptor((SECURITY_INFORMATION)OWNER_SECURITY_INFORMATION,pSecurityDescriptor,&dwSecurityDescriptorLength1);
				if (dwError != ERROR_SUCCESS) throw dwError;
				PSID psidOwner;
				BOOL blnOwnerDefaulted;
				if (!GetSecurityDescriptorOwner(pSecurityDescriptor,&psidOwner,&blnOwnerDefaulted))
					throw GetLastError();
				if (psidOwner == NULL)
				{
					rConsole.Write(_T("Key has no owner."));
				}
				else
				{
					if (!IsValidSid(psidOwner))
					{
						rConsole.Write(_T("Key has invalid owner SID."));
					}
					else
					{
						rConsole.Write(_T("Key Owner: \n"));
						DWORD dwSIDStringSize = 0;
						BOOL blnRet = GetTextualSid(psidOwner,NULL,&dwSIDStringSize);
						ASSERT(!blnRet);
						ASSERT(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
						TCHAR *pchSID = new TCHAR[dwSIDStringSize];
						if(!GetTextualSid(psidOwner,pchSID,&dwSIDStringSize))
						{
							dwError = GetLastError();
							ASSERT(dwError != ERROR_INSUFFICIENT_BUFFER);
							rConsole.Write(_T("Error "));
							TCHAR Buffer[256];
							rConsole.Write(_itot(dwError,Buffer,10));
							rConsole.Write(_T("\nGetting string representation of SID\n"));
						}
						else
						{
							rConsole.Write(_T("\tSID: "));
							rConsole.Write(pchSID);
							rConsole.Write(_T("\n"));
						}
						delete [] pchSID;
						DWORD dwNameBufferLength, dwDomainNameBufferLength;
						dwNameBufferLength = 1024;
						dwDomainNameBufferLength = 1024;
						pchName = new TCHAR [dwNameBufferLength];
						pchDomainName = new TCHAR [dwDomainNameBufferLength];
						DWORD dwNameLength = dwNameBufferLength, dwDomainNameLength = dwDomainNameBufferLength;
						SID_NAME_USE Use;
						if (!LookupAccountSid(NULL,psidOwner,pchName,&dwNameLength,pchDomainName,&dwDomainNameLength,&Use))
							throw GetLastError();
						else
						{
							rConsole.Write(_T("\tOwner Domain: "));
							rConsole.Write(pchDomainName);
							rConsole.Write(_T("\n"));
							rConsole.Write(_T("\tOwner Name: "));
							rConsole.Write(pchName);
							rConsole.Write(_T("\n\tSID type: "));
							rConsole.Write(GetSidTypeName(Use));
							rConsole.Write(_T("\n"));
							rConsole.Write(_T("\tOwner defaulted: "));
							rConsole.Write(blnOwnerDefaulted?_T("Yes"):_T("No"));
							rConsole.Write(_T("\n"));
						}
						delete [] pchName;
						pchName = NULL;
						delete [] pchDomainName;
						pchDomainName = NULL;
						
					}
				}
				delete [] pSecurityDescriptor;
			}
			catch (DWORD dwError)
			{
				rConsole.Write(_T("Error "));
				TCHAR Buffer[256];
				rConsole.Write(_itot(dwError,Buffer,10));
				rConsole.Write(_T("\n"));
				if (pchName) delete [] pchName;
				if (pchDomainName) delete [] pchDomainName;
				if (pSecurityDescriptor) delete [] pSecurityDescriptor;
			}
		} // else (pKey == NULL)
	} // if (blnDo)

	if (pTree)
		delete pTree;

	return 0;
}

const TCHAR * CShellCommandOwner::GetHelpString()
{
	return OWNER_CMD_SHORT_DESC
			_T("Syntax: ") OWNER_CMD _T(" [<KEY>] [/?]\n\n")
			_T("    <KEY> - Optional relative path of desired key.\n")
			_T("    /?    - This help.\n\n")
			_T("Without parameters, command displays information about owner of current key.\n");
}

const TCHAR * CShellCommandOwner::GetHelpShortDescriptionString()
{
	return OWNER_CMD_SHORT_DESC;
}

