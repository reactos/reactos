/* $Id: ShellCommandDACL.cpp,v 1.1 2000/10/04 21:04:30 ea Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (c) 1999-2000 Nedko Arnaoudov <nedkohome@atia.com>
 *
 * License: GNU GPL
 *
 */

// ShellCommandDACL.cpp: implementation of the CShellCommandDACL class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "ShellCommandDACL.h"
#include "RegistryExplorer.h"
#include "SecurityDescriptor.h"

#define DACL_CMD			_T("DACL")
#define DACL_CMD_LENGTH		COMMAND_LENGTH(DACL_CMD)
#define DACL_CMD_SHORT_DESC	DACL_CMD _T(" command is used to view")/*"/edit"*/_T(" key's DACL.\n")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellCommandDACL::CShellCommandDACL(CRegistryTree& rTree):m_rTree(rTree)
{

}

CShellCommandDACL::~CShellCommandDACL()
{

}

BOOL CShellCommandDACL::Match(const TCHAR *pchCommand)
{
	if (_tcsicmp(pchCommand,DACL_CMD) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,DACL_CMD _T(".."),DACL_CMD_LENGTH+2*sizeof(TCHAR)) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,DACL_CMD _T("/") ,DACL_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,DACL_CMD _T("\\"),DACL_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
		return TRUE;
	return FALSE;
}

int CShellCommandDACL::Execute(CConsole &rConsole, CArgumentParser& rArguments)
{
	rArguments.ResetArgumentIteration();

	const TCHAR *pchKey = NULL;
	BOOL blnDo = TRUE;
	BOOL blnBadParameter = FALSE;
	BOOL blnHelp = FALSE;
	const TCHAR *pchParameter;
	const TCHAR *pchCommandItself = rArguments.GetNextArgument();
	DWORD dwError;

	if ((_tcsnicmp(pchCommandItself,DACL_CMD _T(".."),DACL_CMD_LENGTH+2*sizeof(TCHAR)) == 0)||
		(_tcsnicmp(pchCommandItself,DACL_CMD _T("\\"),DACL_CMD_LENGTH+1*sizeof(TCHAR)) == 0))
	{
		pchKey = pchCommandItself + DACL_CMD_LENGTH;
	}
	else if (_tcsnicmp(pchCommandItself,DACL_CMD _T("/"),DACL_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
	{
		pchParameter = pchCommandItself + DACL_CMD_LENGTH;
		goto CheckDACLArgument;
	}

	while((pchParameter = rArguments.GetNextArgument()) != NULL)
	{
CheckDACLArgument:
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
			rConsole.Write(DACL_CMD COMMAND_NA_ON_ROOT);
		}
		else
		{
			DWORD dwSecurityDescriptorLength;
			rConsole.Write(_T("Key : "));
			rConsole.Write(_T("\\"));
			rConsole.Write(pTree?pTree->GetCurrentPath():m_rTree.GetCurrentPath());
			rConsole.Write(_T("\n"));
			PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
			TCHAR *pchName = NULL, *pchDomainName = NULL;
			try
			{
				dwError = pKey->GetSecurityDescriptorLength(&dwSecurityDescriptorLength);
				if (dwError != ERROR_SUCCESS) throw dwError;
				
				pSecurityDescriptor = (PSECURITY_DESCRIPTOR) new unsigned char [dwSecurityDescriptorLength];
				DWORD dwSecurityDescriptorLength1 = dwSecurityDescriptorLength;
				dwError = pKey->GetSecurityDescriptor((SECURITY_INFORMATION)DACL_SECURITY_INFORMATION,pSecurityDescriptor,&dwSecurityDescriptorLength1);
				if (dwError != ERROR_SUCCESS) throw dwError;
				CSecurityDescriptor sd;
				sd.AssociateDescriptor(pSecurityDescriptor);
				
				sd.BeginDACLInteration();
				ASSERT(sd.DescriptorContainsDACL());
				if (sd.HasNULLDACL())
				{
					rConsole.Write(_T("Key has not DACL.\n(This allows all access)\n"));
				}
				else
				{
					if (!sd.HasValidDACL())
					{
						rConsole.Write(_T("Invalid DACL.\n"));
					}
					else
					{
						DWORD nACECount = sd.GetDACLEntriesCount();
						rConsole.Write(_T("DACL has "));
						TCHAR Buffer[256];
						rConsole.Write(_itot(nACECount,Buffer,10));
						rConsole.Write(_T(" ACEs.\n"));
						if (nACECount == 0)
						{
							rConsole.Write(_T("(This denies all access)\n"));
						}
						else
						{
							for (DWORD i = 0 ; i < nACECount ; i++)
							{
								rConsole.Write(_T("\n"));
								rConsole.Write(_T("\tACE Index: "));
								rConsole.Write(_itot(i,Buffer,10));
								rConsole.Write(_T("\n"));
								rConsole.Write(_T("\tACE Type: "));
								switch (sd.GetDACLEntry(i))
								{
								case CSecurityDescriptor::AccessAlowed:
									rConsole.Write(_T("Access-allowed\n"));
									break;
								case CSecurityDescriptor::AccessDenied:
									rConsole.Write(_T("Access-denied\n"));
									break;
								default:
									rConsole.Write(_T("Unknown.\nCannot continue dumping of the ACE list.\n"));
									goto AbortDumpDACL;
								}
								PSID pSID = sd.GetCurrentACE_SID();
								if ((pSID == NULL)||(!IsValidSid(pSID)))
								{
									rConsole.Write(_T("\tInvalid SID.\n"));
								}
								else
								{
									DWORD dwSIDStringSize = 0;
									BOOL blnRet = GetTextualSid(pSID,NULL,&dwSIDStringSize);
									ASSERT(!blnRet);
									ASSERT(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
									TCHAR *pchSID = new TCHAR[dwSIDStringSize];
									if(!GetTextualSid(pSID,pchSID,&dwSIDStringSize))
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
									delete pchSID;
									DWORD dwNameBufferLength, dwDomainNameBufferLength;
									dwNameBufferLength = 1024;
									dwDomainNameBufferLength = 1024;
									pchName = new TCHAR [dwNameBufferLength];
									pchDomainName = new TCHAR [dwDomainNameBufferLength];
									DWORD dwNameLength = dwNameBufferLength, dwDomainNameLength = dwDomainNameBufferLength;
									SID_NAME_USE Use;
									if (!LookupAccountSid(NULL,pSID,pchName,&dwNameLength,pchDomainName,&dwDomainNameLength,&Use))
									{
										rConsole.Write(_T("Error "));
										TCHAR Buffer[256];
										rConsole.Write(_itot(GetLastError(),Buffer,10));
										rConsole.Write(_T("\n"));
									}
									else
									{
										rConsole.Write(_T("\tTrustee Domain: "));
										rConsole.Write(pchDomainName);
										rConsole.Write(_T("\n"));
										rConsole.Write(_T("\tTrustee Name: "));
										rConsole.Write(pchName);
										rConsole.Write(_T("\n\tSID type: "));
										rConsole.Write(GetSidTypeName(Use));
										rConsole.Write(_T("\n"));
									}
									delete [] pchName;
									pchName = NULL;
									delete [] pchDomainName;
									pchDomainName = NULL;
								}
								DWORD dwAccessMask;
								sd.GetCurrentACE_AccessMask(dwAccessMask);
								wsprintf(Buffer,_T("\tAccess Mask: 0x%08lX\n"),dwAccessMask);
								rConsole.Write(Buffer);
								if (dwAccessMask & GENERIC_READ)
								{
									rConsole.Write(_T("\t\tGENERIC_READ\n"));
								}
								if (dwAccessMask & GENERIC_WRITE)
								{
									rConsole.Write(_T("\t\tGENERIC_WRITE\n"));
								}
								if (dwAccessMask & GENERIC_EXECUTE)
								{
									rConsole.Write(_T("\t\tGENERIC_EXECUTE\n"));
								}
								if (dwAccessMask & GENERIC_ALL)
								{
									rConsole.Write(_T("\t\tGENERIC_ALL\n"));
								}
								if (dwAccessMask & SYNCHRONIZE)
								{
									rConsole.Write(_T("\t\tSYNCHRONIZE\n"));
								}
								if (dwAccessMask & WRITE_OWNER)
								{
									rConsole.Write(_T("\t\tWRITE_OWNER\n"));
								}
								if (dwAccessMask & WRITE_DAC)
								{
									rConsole.Write(_T("\t\tWRITE_DAC\n"));
								}
								if (dwAccessMask & READ_CONTROL)
								{
									rConsole.Write(_T("\t\tREAD_CONTROL\n"));
								}
								if (dwAccessMask & DELETE)
								{
									rConsole.Write(_T("\t\tDELETE\n"));
								}
								if (dwAccessMask & KEY_CREATE_LINK)
								{
									rConsole.Write(_T("\t\tKEY_CREATE_LINK\n"));
								}
								if (dwAccessMask & KEY_NOTIFY)
								{
									rConsole.Write(_T("\t\tKEY_NOTIFY\n"));
								}
								if (dwAccessMask & KEY_ENUMERATE_SUB_KEYS)
								{
									rConsole.Write(_T("\t\tKEY_ENUMERATE_SUB_KEYS\n"));
								}
								if (dwAccessMask & KEY_CREATE_SUB_KEY)
								{
									rConsole.Write(_T("\t\tKEY_CREATE_SUB_KEY\n"));
								}
								if (dwAccessMask & KEY_SET_VALUE)
								{
									rConsole.Write(_T("\t\tKEY_SET_VALUE\n"));
								}
								if (dwAccessMask & KEY_QUERY_VALUE)
								{
									rConsole.Write(_T("\t\tKEY_QUERY_VALUE\n"));
								}
							} // for
						} // else (nACECount == 0)
					} // else (!sd.HasValidDACL())
				} // else (sd.HasNULLDACL())
AbortDumpDACL:
				delete [] pSecurityDescriptor;
			}  // try
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

const TCHAR * CShellCommandDACL::GetHelpString()
{
	return DACL_CMD_SHORT_DESC
			_T("Syntax: ") DACL_CMD _T(" [<KEY>] [/?]\n\n")
			_T("    <KEY> - Optional relative path of desired key.\n")
			_T("    /?    - This help.\n\n")
			_T("Without parameters, command displays DACL of current key.\n");
}

const TCHAR * CShellCommandDACL::GetHelpShortDescriptionString()
{
	return DACL_CMD_SHORT_DESC;
}
