/* $Id: ShellCommandSACL.cpp,v 1.1 2000/10/04 21:04:31 ea Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (c) 1999-2000 Nedko Arnaoudov <nedkohome@atia.com>
 *
 * License: GNU GPL
 *
 */

// ShellCommandSACL.cpp: implementation of the CShellCommandSACL class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "ShellCommandSACL.h"
#include "RegistryExplorer.h"
#include "SecurityDescriptor.h"

#define SACL_CMD			_T("SACL")
#define SACL_CMD_LENGTH		COMMAND_LENGTH(SACL_CMD)
#define SACL_CMD_SHORT_DESC	SACL_CMD _T(" command is used to view")/*"/edit"*/_T(" key's SACL.\n")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellCommandSACL::CShellCommandSACL(CRegistryTree& rTree):m_rTree(rTree)
{

}

CShellCommandSACL::~CShellCommandSACL()
{

}

BOOL CShellCommandSACL::Match(const TCHAR *pchCommand)
{
	if (_tcsicmp(pchCommand,SACL_CMD) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,SACL_CMD _T(".."),SACL_CMD_LENGTH+2*sizeof(TCHAR)) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,SACL_CMD _T("/") ,SACL_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,SACL_CMD _T("\\"),SACL_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
		return TRUE;
	return FALSE;
}

int CShellCommandSACL::Execute(CConsole &rConsole, CArgumentParser& rArguments)
{
	TCHAR err_msg[1024];

	rArguments.ResetArgumentIteration();

	const TCHAR *pchKey = NULL;
	BOOL blnDo = TRUE;
	BOOL blnBadParameter = FALSE;
	BOOL blnHelp = FALSE;
	const TCHAR *pchParameter;
	const TCHAR *pchCommandItself = rArguments.GetNextArgument();
	DWORD dwError;

	if ((_tcsnicmp(pchCommandItself,SACL_CMD _T(".."),SACL_CMD_LENGTH+2*sizeof(TCHAR)) == 0)||
		(_tcsnicmp(pchCommandItself,SACL_CMD _T("\\"),SACL_CMD_LENGTH+1*sizeof(TCHAR)) == 0))
	{
		pchKey = pchCommandItself + SACL_CMD_LENGTH;
	}
	else if (_tcsnicmp(pchCommandItself,SACL_CMD _T("/"),SACL_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
	{
		pchParameter = pchCommandItself + SACL_CMD_LENGTH;
		goto CheckSACLArgument;
	}

	while((pchParameter = rArguments.GetNextArgument()) != NULL)
	{
CheckSACLArgument:
		blnBadParameter = FALSE;
//			Console.Write(_T("Processing parameter: \")");
//			Console.Write(pchParameter);
//			Console.Write(_T("\")\n");
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
		try
		{
			if (!pKey)
				throw (SACL_CMD COMMAND_NA_ON_ROOT);

			HANDLE hThreadToken = INVALID_HANDLE_VALUE;
			if (!OpenThreadToken(GetCurrentThread(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,FALSE,&hThreadToken))
			{	// OpenThreadToken fails
				dwError = GetLastError();
				if (dwError != ERROR_NO_TOKEN)
				{
					_stprintf(err_msg,_T("\nCannot open thread token.\nOpenThreadToken fails with error: %u\n"),dwError);
					throw err_msg;
				}
				// If the thread does not have an access token, we'll examine the
				// access token associated with the process.
				if (!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&hThreadToken))
				{
					_stprintf(err_msg,_T("\nCannot open process token.\nOpenProcessToken fails with error: %u\n"),GetLastError());
					throw err_msg;
				}
			}
			ASSERT(hThreadToken != INVALID_HANDLE_VALUE);
			TOKEN_PRIVILEGES priv;
			priv.PrivilegeCount = 1;
			priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			if (!LookupPrivilegeValue(
				NULL,							// lookup privilege on local system
				SE_SECURITY_NAME,				// privilege to lookup 
				&(priv.Privileges[0].Luid)))	// receives LUID of privilege
			{
				_stprintf(err_msg,_T("\nCannot retrieve the locally unique identifier for %s privilege.\nLookupPrivilegeValue error: %u\n"),SE_SECURITY_NAME,GetLastError());
				throw err_msg;
			}
			BOOL blnAdjRet = AdjustTokenPrivileges(
				hThreadToken,
				FALSE,
				&priv,
				sizeof(TOKEN_PRIVILEGES),
				(PTOKEN_PRIVILEGES)NULL,
				(PDWORD)NULL);
			dwError = GetLastError();
			if (!blnAdjRet)
			{
				_stprintf(err_msg,_T("\nCannot enable %s privilege.\nAdjustTokenPrivileges fails with error: %u%s\n"),SE_SECURITY_NAME,dwError,(dwError == 5)?_T(" (Access denied)"):_T(""));
				throw err_msg;
			}
			if (dwError != ERROR_SUCCESS)
			{
				if (dwError == ERROR_NOT_ALL_ASSIGNED)
				{
					_stprintf(err_msg,_T("\nCannot enable %s privilege.\nThe token does not have the %s privilege\n"),SE_SECURITY_NAME,SE_SECURITY_NAME);
				}
				else
				{
					_stprintf(err_msg,_T("\nCannot enable %s privilege.\nAdjustTokenPrivileges succeds with error: %u\n"),SE_SECURITY_NAME,dwError);
				}
				throw err_msg;
			}
			if (!pKey->IsPredefined())
			{
				dwError = pKey->Open(m_rTree.GetDesiredOpenKeyAccess()|ACCESS_SYSTEM_SECURITY);
				if (dwError != ERROR_SUCCESS)
				{
					_stprintf(err_msg,_T("\nCannot reopen current key.\nError %u%s\n"),dwError,(dwError == 5)?_T(" (Access denied)"):_T(""));
					throw err_msg;
				}
			}
			DWORD dwSecurityDescriptorLength;
			rConsole.Write(_T("Key : "));
			rConsole.Write(_T("\\"));
			rConsole.Write(pTree?pTree->GetCurrentPath():m_rTree.GetCurrentPath());
			rConsole.Write(_T("\n"));
			dwError = pKey->GetSecurityDescriptorLength(&dwSecurityDescriptorLength);
			if (dwError != ERROR_SUCCESS)
			{
				_stprintf(err_msg,_T("\nCannot get security descriptor's length for current key.\nError: %u\n"),dwError);
				throw err_msg;
			}
			PSECURITY_DESCRIPTOR pSecurityDescriptor = (PSECURITY_DESCRIPTOR) new unsigned char [dwSecurityDescriptorLength];
			DWORD dwSecurityDescriptorLength1 = dwSecurityDescriptorLength;
			dwError = pKey->GetSecurityDescriptor((SECURITY_INFORMATION)SACL_SECURITY_INFORMATION,pSecurityDescriptor,&dwSecurityDescriptorLength1);
			if (dwError != ERROR_SUCCESS)
			{
				_stprintf(err_msg,_T("\nCannot get security descriptor for current key.\nError: %u%s\n"),dwError,(dwError == 1314)?_T("(A required privilege is not held by the client.)\n"):_T(""));
				throw err_msg;
			}
			CSecurityDescriptor sd;
			sd.AssociateDescriptor(pSecurityDescriptor);
			sd.BeginSACLInteration();
			
			if ((!sd.DescriptorContainsSACL())||(sd.HasNULLSACL()))
			{
				throw _T("Key has not SACL.\n");
			}
			if (!sd.HasValidSACL())
			{
				throw _T("Invalid SACL.\n");
			}
			DWORD nACECount = sd.GetSACLEntriesCount();
			rConsole.Write(_T("SACL has "));
			TCHAR Buffer[256];
			rConsole.Write(_itot(nACECount,Buffer,10));
			rConsole.Write(_T(" ACEs.\n"));
			ASSERT(sizeof(ACL) == 8);
			rConsole.Write(_T("\n"));
			for (DWORD i = 0 ; i < nACECount ; i++)
			{
				rConsole.Write(_T("\n"));
				rConsole.Write(_T("\tACE Index: "));
				rConsole.Write(_itot(i,Buffer,10));
				rConsole.Write(_T("\n"));
				rConsole.Write(_T("\tAudit Type: "));
				BOOL blnFailed, blnSuccessful;
				if (sd.GetSACLEntry(i,blnFailed,blnSuccessful) != CSecurityDescriptor::SystemAudit)
				{
					rConsole.Write(_T("Unknown ACE type.\nCannot continue ACE list dump.\n"));
					goto AbortDumpSACL;
				}
				if (blnFailed) rConsole.Write(_T("Failed access"));
				if (blnFailed && blnSuccessful) rConsole.Write(_T(" & "));
				if (blnSuccessful) rConsole.Write(_T("Successful access"));
				rConsole.Write(_T("\n"));
				PSID pSID = sd.GetCurrentACE_SID();
				if ((pSID == NULL)||(!IsValidSid(pSID)))
				{
					rConsole.Write(_T("\tInvalid SID.\n"));
				}
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
				delete [] pchSID;
				TCHAR *pchName, *pchDomainName;
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
			}	// for
AbortDumpSACL:
			delete pSecurityDescriptor;
		}
		catch(TCHAR *pchError)
		{
			rConsole.Write(pchError);
		}
	} // if (blnDo)

	return 0;
}

const TCHAR * CShellCommandSACL::GetHelpString()
{
	return SACL_CMD_SHORT_DESC
			_T("Syntax: ") SACL_CMD _T(" [<KEY>] [/?]\n\n")
			_T("    <KEY> - Optional relative path of desired key.\n")
			_T("    /?    - This help.\n\n")
			_T("Without parameters, command displays SACL of current key.\n");
}

const TCHAR * CShellCommandSACL::GetHelpShortDescriptionString()
{
	return SACL_CMD_SHORT_DESC;
}
