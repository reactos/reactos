/* $Id: ShellCommandSACL.cpp,v 1.3 2001/01/10 01:25:29 narnaoud Exp $
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
#define ERROR_MSG_BUFFER_SIZE 1024
  TCHAR pszError_msg[ERROR_MSG_BUFFER_SIZE];
  pszError_msg[ERROR_MSG_BUFFER_SIZE-1] = 0;
  
  rArguments.ResetArgumentIteration();

  const TCHAR *pszKey = NULL;
  BOOL blnDo = TRUE;
  BOOL blnBadParameter = FALSE;
  BOOL blnHelp = FALSE;
  const TCHAR *pszParameter;
  const TCHAR *pszCommandItself = rArguments.GetNextArgument();
  DWORD dwError;
  PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
  CSecurityDescriptor sd;
  HANDLE hThreadToken = INVALID_HANDLE_VALUE;
  
  if ((_tcsnicmp(pszCommandItself,SACL_CMD _T(".."),SACL_CMD_LENGTH+2*sizeof(TCHAR)) == 0)||
      (_tcsnicmp(pszCommandItself,SACL_CMD _T("\\"),SACL_CMD_LENGTH+1*sizeof(TCHAR)) == 0))
  {
    pszKey = pszCommandItself + SACL_CMD_LENGTH;
  }
  else if (_tcsnicmp(pszCommandItself,SACL_CMD _T("/"),SACL_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
  {
    pszParameter = pszCommandItself + SACL_CMD_LENGTH;
    goto CheckSACLArgument;
  }
  
  while((pszParameter = rArguments.GetNextArgument()) != NULL)
  {
CheckSACLArgument:
    blnBadParameter = FALSE;
    if ((_tcsicmp(pszParameter,_T("/?")) == 0)||
        (_tcsicmp(pszParameter,_T("-?")) == 0))
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

  ASSERT(hThreadToken == INVALID_HANDLE_VALUE);
  
  // Open thread token
  if (!OpenThreadToken(GetCurrentThread(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,FALSE,&hThreadToken))
  {	// OpenThreadToken failed
    dwError = GetLastError();
    if (dwError != ERROR_NO_TOKEN)
    {
      _sntprintf(pszError_msg,ERROR_MSG_BUFFER_SIZE-1,_T("\nCannot open thread token.\nOpenThreadToken fails with error: %u\n"),(unsigned int)dwError);
      goto Error;
    }
    
    // If the thread does not have an access token, we'll examine the
    // access token associated with the process.
    if (!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&hThreadToken))
    {
      _sntprintf(pszError_msg,ERROR_MSG_BUFFER_SIZE-1,_T("\nCannot open process token.\nOpenProcessToken fails with error: %u\n"),(unsigned int)GetLastError());
      goto Error;
    }
  }
  
  ASSERT(hThreadToken != INVALID_HANDLE_VALUE);

  // enable SeSecurityPrivilege privilege
  TOKEN_PRIVILEGES priv;
  priv.PrivilegeCount = 1;
  priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  if (!LookupPrivilegeValue(
      NULL,							// lookup privilege on local system
      SE_SECURITY_NAME,				// privilege to lookup 
      &(priv.Privileges[0].Luid)))	// receives LUID of privilege
  {
    _sntprintf(pszError_msg,ERROR_MSG_BUFFER_SIZE-1,_T("\nCannot retrieve the locally unique identifier for %s privilege.\nLookupPrivilegeValue error: %u\n"),SE_SECURITY_NAME,(unsigned int)GetLastError());
    goto Error;
  }

  BOOL blnAdjRet;
  blnAdjRet = AdjustTokenPrivileges(
      hThreadToken,
      FALSE,
      &priv,
      sizeof(TOKEN_PRIVILEGES),
      (PTOKEN_PRIVILEGES)NULL,
      (PDWORD)NULL);
  dwError = GetLastError();
  if (!blnAdjRet)
  {
    _sntprintf(pszError_msg,ERROR_MSG_BUFFER_SIZE-1,_T("\nCannot enable %s privilege.\nAdjustTokenPrivileges fails with error: %u%s\n"),SE_SECURITY_NAME,(unsigned int)dwError,(dwError == 5)?_T(" (Access denied)"):_T(""));
    goto Error;
  }
  
  if (dwError != ERROR_SUCCESS)
  {
    if (dwError == ERROR_NOT_ALL_ASSIGNED)
    {
      _sntprintf(pszError_msg,ERROR_MSG_BUFFER_SIZE-1,_T("\nCannot enable %s privilege.\nThe token does not have the %s privilege\n"),SE_SECURITY_NAME,SE_SECURITY_NAME);
    }
    else
    {
      _sntprintf(pszError_msg,ERROR_MSG_BUFFER_SIZE-1,_T("\nCannot enable %s privilege.\nAdjustTokenPrivileges succeds with error: %u\n"),SE_SECURITY_NAME,(unsigned int)dwError);
    }
    
    goto Error;
  }
  
  if (!m_rTree.GetKey(pszKey?pszKey:_T("."),KEY_QUERY_VALUE|READ_CONTROL|ACCESS_SYSTEM_SECURITY,Key))
  {
    rConsole.Write(m_rTree.GetLastErrorDescription());
    blnDo = FALSE;
  }
  
  if (blnHelp)
  {
    rConsole.Write(GetHelpString());
  }

  if (blnDo&&blnHelp)
    rConsole.Write(_T("\n"));

  if (!blnDo)
    return 0;
  
  if (Key.IsRoot())
  {
    _tcsncpy(pszError_msg,SACL_CMD COMMAND_NA_ON_ROOT,ERROR_MSG_BUFFER_SIZE-1);
    goto Error;
  }
      
  DWORD dwSecurityDescriptorLength;
  rConsole.Write(_T("Key : "));
  rConsole.Write(_T("\\"));
  
  rConsole.Write(Key.GetKeyName());
  rConsole.Write(_T("\n"));
  dwError = Key.GetSecurityDescriptorLength(&dwSecurityDescriptorLength);
  if (dwError != ERROR_SUCCESS)
  {
    _sntprintf(pszError_msg,ERROR_MSG_BUFFER_SIZE-1,_T("\nCannot get security descriptor's length for current key.\nError: %u\n"),(unsigned int)dwError);
    goto Error;
  }
  
  pSecurityDescriptor = (PSECURITY_DESCRIPTOR) new unsigned char [dwSecurityDescriptorLength];
  if (!pSecurityDescriptor)
  {
      _tcsncpy(pszError_msg,_T("\nOut of memory.\n"),ERROR_MSG_BUFFER_SIZE-1);
      goto Error;
  }
  
  DWORD dwSecurityDescriptorLength1;
  dwSecurityDescriptorLength1 = dwSecurityDescriptorLength;
  dwError = Key.GetSecurityDescriptor((SECURITY_INFORMATION)SACL_SECURITY_INFORMATION,pSecurityDescriptor,&dwSecurityDescriptorLength1);
  if (dwError != ERROR_SUCCESS)
  {
    _sntprintf(pszError_msg,ERROR_MSG_BUFFER_SIZE-1,_T("\nCannot get security descriptor for current key.\nError: %u%s\n"),(unsigned int)dwError,(dwError == 1314)?_T("(A required privilege is not held by the client.)\n"):_T(""));
    goto Error;
  }
  
  sd.AssociateDescriptor(pSecurityDescriptor);
  sd.BeginSACLInteration();
  
  if ((!sd.DescriptorContainsSACL())||(sd.HasNULLSACL()))
  {
    _tcsncpy(pszError_msg,_T("Key has not SACL.\n"),ERROR_MSG_BUFFER_SIZE-1);
    goto Error;
  }
  
  if (!sd.HasValidSACL())
  {
    _tcsncpy(pszError_msg,_T("Invalid SACL.\n"),ERROR_MSG_BUFFER_SIZE-1);
    goto Error;
  }
  
  DWORD nACECount;
  nACECount = sd.GetSACLEntriesCount();
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
    
    if (blnFailed)
      rConsole.Write(_T("Failed access"));
    
    if (blnFailed && blnSuccessful)
      rConsole.Write(_T(" & "));
    if (blnSuccessful)
      rConsole.Write(_T("Successful access"));
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
    TCHAR *pszSID = new TCHAR[dwSIDStringSize];

    if (!pszSID)
    {
      _tcsncpy(pszError_msg,_T("\nOut of memory.\n"),ERROR_MSG_BUFFER_SIZE-1);
      goto Error;
    }
    
    if(!GetTextualSid(pSID,pszSID,&dwSIDStringSize))
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
      rConsole.Write(pszSID);
      rConsole.Write(_T("\n"));
    }
    delete pszSID;
    
    TCHAR *pszName, *pszDomainName;
    DWORD dwNameBufferLength, dwDomainNameBufferLength;
    dwNameBufferLength = 1024;
    dwDomainNameBufferLength = 1024;
    
    pszName = new TCHAR [dwNameBufferLength];
    if (!pszName)
    {
      _tcsncpy(pszError_msg,_T("\nOut of memory.\n"),ERROR_MSG_BUFFER_SIZE-1);
      goto Error;
    }
    
    pszDomainName = new TCHAR [dwDomainNameBufferLength];
    if (!pszDomainName)
    {
      _tcsncpy(pszError_msg,_T("\nOut of memory.\n"),ERROR_MSG_BUFFER_SIZE-1);
      goto Error;
    }
    
    DWORD dwNameLength = dwNameBufferLength, dwDomainNameLength = dwDomainNameBufferLength;
    SID_NAME_USE Use;
    if (!LookupAccountSid(NULL,pSID,pszName,&dwNameLength,pszDomainName,&dwDomainNameLength,&Use))
    {
      rConsole.Write(_T("Error "));
      TCHAR Buffer[256];
      rConsole.Write(_itot(GetLastError(),Buffer,10));
      rConsole.Write(_T("\n"));
    }
    else
    {
      rConsole.Write(_T("\tTrustee Domain: "));
      rConsole.Write(pszDomainName);
      rConsole.Write(_T("\n"));
      rConsole.Write(_T("\tTrustee Name: "));
      rConsole.Write(pszName);
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
  ASSERT(pSecurityDescriptor);
  delete pSecurityDescriptor;

  VERIFY(CloseHandle(hThreadToken));

	return 0;
Error:
  if (pSecurityDescriptor)
    delete pSecurityDescriptor;

  if (hThreadToken != INVALID_HANDLE_VALUE)
    VERIFY(CloseHandle(hThreadToken));
  
  rConsole.Write(pszError_msg);
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
