/* $Id: ShellCommandDACL.cpp,v 1.4 2001/01/13 23:55:36 narnaoud Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (C) 2000,2001 Nedko Arnaoudov <nedkohome@atia.com>
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

	const TCHAR *pszKey = NULL;
	BOOL blnDo = TRUE;
	BOOL blnBadParameter = FALSE;
	BOOL blnHelp = FALSE;
	const TCHAR *pchParameter;
	const TCHAR *pchCommandItself = rArguments.GetNextArgument();
	LONG nError;

	if ((_tcsnicmp(pchCommandItself,DACL_CMD _T(".."),DACL_CMD_LENGTH+2*sizeof(TCHAR)) == 0)||
		(_tcsnicmp(pchCommandItself,DACL_CMD _T("\\"),DACL_CMD_LENGTH+1*sizeof(TCHAR)) == 0))
	{
		pszKey = pchCommandItself + DACL_CMD_LENGTH;
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
			blnDo = pszKey != NULL;
		}
		else if (!pszKey)
		{
			pszKey = pchParameter;
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

	CRegistryKey Key;
  
  if (!m_rTree.GetKey(pszKey?pszKey:_T("."),KEY_QUERY_VALUE|READ_CONTROL,Key))
  {
    rConsole.Write(m_rTree.GetLastErrorDescription());
    blnDo = FALSE;
  }

	if (blnHelp)
	{
		rConsole.Write(GetHelpString());
	}

	if (blnDo&&blnHelp) rConsole.Write(_T("\n"));

	if (!blnDo)
    return 0;
  
  if (Key.IsRoot())
	{	// root key
    rConsole.Write(DACL_CMD COMMAND_NA_ON_ROOT);
    return 0;
  }
  
  DWORD dwSecurityDescriptorLength;
  rConsole.Write(_T("Key : "));
  rConsole.Write(_T("\\"));
  rConsole.Write(Key.GetKeyName());
  rConsole.Write(_T("\n"));
  PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
  TCHAR *pchName = NULL, *pchDomainName = NULL;
  try
  {
    nError = Key.GetSecurityDescriptorLength(&dwSecurityDescriptorLength);
    if (nError != ERROR_SUCCESS)
      throw nError;
				
    pSecurityDescriptor = (PSECURITY_DESCRIPTOR) new unsigned char [dwSecurityDescriptorLength];
    DWORD dwSecurityDescriptorLength1 = dwSecurityDescriptorLength;
    nError = Key.GetSecurityDescriptor((SECURITY_INFORMATION)DACL_SECURITY_INFORMATION,pSecurityDescriptor,&dwSecurityDescriptorLength1);
    if (nError != ERROR_SUCCESS)
      throw nError;
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
                DWORD dwError = GetLastError();
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

            BYTE bFlags;
            sd.GetCurrentACE_Flags(bFlags);
            wsprintf(Buffer,_T("\tFlags: 0x%02lX\n"),bFlags);
            rConsole.Write(Buffer);
            if (bFlags & CONTAINER_INHERIT_ACE)
            {
              rConsole.Write(_T("\t\tCONTAINER_INHERIT_ACE\n"));
            }
            if (bFlags & INHERIT_ONLY_ACE)
            {
              rConsole.Write(_T("\t\tINHERIT_ONLY_ACE\n"));
            }
            if (bFlags & INHERITED_ACE)
            {
              rConsole.Write(_T("\t\tINHERITED_ACE\n"));
            }
            if (bFlags & NO_PROPAGATE_INHERIT_ACE)
            {
              rConsole.Write(_T("\t\tNO_PROPAGATE_INHERIT_ACE\n"));
            }
            if (bFlags & OBJECT_INHERIT_ACE)
            {
              rConsole.Write(_T("\t\tOBJECT_INHERIT_ACE\n"));
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
