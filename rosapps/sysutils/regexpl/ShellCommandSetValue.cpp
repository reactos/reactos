/* $Id: ShellCommandSetValue.cpp,v 1.4 2001/01/13 23:55:37 narnaoud Exp $
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

// ShellCommandSetValue.cpp: implementation of the CShellCommandSetValue class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "ShellCommandSetValue.h"
#include "RegistryExplorer.h"
#include "RegistryTree.h"
#include "RegistryKey.h"

#define SET_VALUE_CMD				_T("SV")
#define SET_VALUE_CMD_LENGTH		COMMAND_LENGTH(SET_VALUE_CMD)
#define SET_VALUE_CMD_SHORT_DESC	SET_VALUE_CMD _T(" command is used to set value.\n")

BOOL StringToDWORD(DWORD& rdwOut, const TCHAR *pszIn)
{
	const TCHAR *pszDigits;
	const TCHAR *pszOctalNumbers = _T("01234567");
	const TCHAR *pszDecimalNumbers = _T("0123456789");
	const TCHAR *pszHexNumbers = _T("0123456789ABCDEF");
	const TCHAR *pszNumbers;
	unsigned int nBase = 0;
	if (*pszIn == _T('0'))
	{
		if ((*(pszIn+1) == _T('x'))||((*(pszIn+1) == _T('X'))))
		{	// hex
			nBase = 16;
			pszDigits = pszIn+2;
			pszNumbers = pszHexNumbers;
		}
		else
		{	// octal
			nBase = 8;
			pszDigits = pszIn+1;
			pszNumbers = pszOctalNumbers;
		}
	}
	else
	{	//decimal
		nBase = 10;
		pszDigits = pszIn;
		pszNumbers = pszDecimalNumbers;
	}

	const TCHAR *pszDigit = pszDigits;
	pszDigit += _tcslen(pszDigit);

	DWORD nMul = 1;
	rdwOut = 0;
	DWORD dwAdd;
	const TCHAR *pszNumber;
	while (pszDigit > pszDigits)
	{
		pszDigit--;
		pszNumber = _tcschr(pszNumbers,*pszDigit);
		if (!pszNumber)
			return FALSE;	// wrong char in input string

		dwAdd = (pszNumber-pszNumbers)*nMul;

		if (rdwOut + dwAdd < rdwOut)
			return FALSE;	// overflow
		rdwOut += dwAdd;

		if (nMul * nBase < nMul)
			return FALSE;	// overflow
		nMul *= nBase;
	};

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellCommandSetValue::CShellCommandSetValue(CRegistryTree& rTree):m_rTree(rTree)
{
}

CShellCommandSetValue::~CShellCommandSetValue()
{
}

BOOL CShellCommandSetValue::Match(const TCHAR *pszCommand)
{
	if (_tcsicmp(pszCommand,SET_VALUE_CMD) == 0)
		return TRUE;
	if (_tcsnicmp(pszCommand,SET_VALUE_CMD _T(".."),SET_VALUE_CMD_LENGTH+2*sizeof(TCHAR)) == 0)
		return TRUE;
	if (_tcsnicmp(pszCommand,SET_VALUE_CMD _T("/"),SET_VALUE_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
		return TRUE;
	if (_tcsnicmp(pszCommand,SET_VALUE_CMD _T("\\"),SET_VALUE_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
		return TRUE;
	return FALSE;
}

int CShellCommandSetValue::Execute(CConsole &rConsole, CArgumentParser& rArguments)
{
  LONG nError;
  
	rArguments.ResetArgumentIteration();
	TCHAR *pszCommandItself = rArguments.GetNextArgument();

	TCHAR *pszParameter;
	TCHAR *pszValueFull = NULL;
	TCHAR *pszValueData = NULL;
	BOOL blnBadParameter = FALSE;
	BOOL blnHelp = FALSE;
	DWORD dwValueSize = 0;
	DWORD dwType = REG_NONE;
	BYTE *pDataBuffer = NULL;

	if ((_tcsnicmp(pszCommandItself,SET_VALUE_CMD _T(".."),SET_VALUE_CMD_LENGTH+2*sizeof(TCHAR)) == 0)||
		(_tcsnicmp(pszCommandItself,SET_VALUE_CMD _T("\\"),SET_VALUE_CMD_LENGTH+1*sizeof(TCHAR)) == 0))
	{
		pszValueFull = pszCommandItself + SET_VALUE_CMD_LENGTH;
	}
	else if (_tcsnicmp(pszCommandItself,SET_VALUE_CMD _T("/"),SET_VALUE_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
	{
		pszParameter = pszCommandItself + SET_VALUE_CMD_LENGTH;
		goto CheckValueArgument;
	}

	while((pszParameter = rArguments.GetNextArgument()) != NULL)
	{
CheckValueArgument:
		blnBadParameter = FALSE;
		if (((*pszParameter == _T('/'))||(*pszParameter == _T('-')))
			&&(*(pszParameter+1) == _T('?')))
		{
			blnHelp = TRUE;
		}
		else if (dwType == REG_NONE)
		{
			if (_tcsicmp(pszParameter,_T("b")) == 0)
			{
				dwType = REG_BINARY;
			}
			else if (_tcsicmp(pszParameter,_T("dw")) == 0)
			{
				dwType = REG_DWORD;
			}
			else if (_tcsicmp(pszParameter,_T("dwle")) == 0)
			{
				dwType = REG_DWORD_LITTLE_ENDIAN;
			}
			else if (_tcsicmp(pszParameter,_T("dwbe")) == 0)
			{
				dwType = REG_DWORD_BIG_ENDIAN;
			}
			else if (_tcsicmp(pszParameter,_T("sz")) == 0)
			{
				dwType = REG_SZ;
			}
			else if (_tcsicmp(pszParameter,_T("esz")) == 0)
			{
				dwType = REG_EXPAND_SZ;
			}
			else
			{
				blnBadParameter = TRUE;
			}
		}
		else if (pszValueData == NULL)
		{
			pszValueData = pszParameter;
		}
		else if (!pszValueFull)
		{
			pszValueFull = pszParameter;
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

	if (!pszValueData)
		blnHelp = TRUE;
	
	CRegistryKey Key;
	TCHAR *pszValueName;
	const TCHAR *pszPath;
	
	if (blnHelp)
	{
		rConsole.Write(GetHelpString());

		if (pDataBuffer)
			delete pDataBuffer;

		return 0;
	}

	if (pszValueFull)
	{
		if (_tcscmp(pszValueFull,_T("\\")) == 0)
			goto CommandNAonRoot;

		TCHAR *pchSep = _tcsrchr(pszValueFull,_T('\\'));
		pszValueName = pchSep?(pchSep+1):(pszValueFull);
		pszPath = pchSep?pszValueFull:_T(".");
				
		//if (_tcsrchr(pszValueName,_T('.')))
		//{
		//	pszValueName = _T("");
		//	pszPath = pszValueFull;
		//}
		//else
		if (pchSep)
			*pchSep = 0;
	}
	else
	{
		pszValueName = _T("");
		pszPath = _T(".");
	}
		
  if (!m_rTree.GetKey(pszPath,KEY_SET_VALUE,Key))
  {
    rConsole.Write(m_rTree.GetLastErrorDescription());
    goto SkipCommand;
  }
	
	if (Key.IsRoot())
    goto CommandNAonRoot;
  
  switch (dwType)
  {
  case REG_BINARY:
    {
      HANDLE hFile;
      DWORD dwBytesReaded;
      hFile = CreateFile(pszValueData,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
      if (hFile == INVALID_HANDLE_VALUE)
      {
        rConsole.Write(_T("Cannot open file "));
        rConsole.Write(pszValueData);
        rConsole.Write(_T("\n"));
        goto SkipCommand;
      }
      dwValueSize = GetFileSize(hFile,NULL);
      if (dwValueSize == (DWORD)-1)	// ok, that's right, we compare signed with unsigned here.
        // GetFileSize is documented and declared to return DWORD.
        // Error is indicated by checking if return is -1. Design->documentation bug ???
      {
        rConsole.Write(_T("Cannot get size of file "));
        rConsole.Write(pszValueData);
        rConsole.Write(_T("\n"));
        VERIFY(CloseHandle(hFile));
        goto SkipCommand;
      }
      pDataBuffer = new BYTE [dwValueSize];
      if (!pDataBuffer)
      {
        rConsole.Write(_T("Cannot load file into memory. Out of memory.\n"));
        VERIFY(CloseHandle(hFile));
        goto SkipCommand;
      }
      if (!ReadFile(hFile,pDataBuffer,dwValueSize,&dwBytesReaded,NULL))
      {
        rConsole.Write(_T("Cannot load file into memory. Error reading file.\n"));
        VERIFY(CloseHandle(hFile));
        goto SkipCommand;
      }

      VERIFY(CloseHandle(hFile));
      ASSERT(dwBytesReaded == dwValueSize);
    }
    break;
  case REG_DWORD_LITTLE_ENDIAN:
  case REG_DWORD_BIG_ENDIAN:
    dwValueSize = 4;
    pDataBuffer = (BYTE *) new BYTE [dwValueSize];
    if (!StringToDWORD(*(DWORD *)pDataBuffer,pszValueData))
    {
      rConsole.Write(_T("Cannot convert "));
      rConsole.Write(pszValueData);
      rConsole.Write(_T(" to DWORD \n"));
      goto SkipCommand;
    }
    if (dwType == REG_DWORD_BIG_ENDIAN)
    {
      unsigned char nByte;
      nByte = *pDataBuffer;
      *pDataBuffer = *(pDataBuffer+3);
      *(pDataBuffer+3) = nByte;
      nByte = *(pDataBuffer+1);
      *(pDataBuffer+1) = *(pDataBuffer+2);
      *(pDataBuffer+2) = nByte;
    }
    break;
  case REG_SZ:
  case REG_EXPAND_SZ:
    dwValueSize = _tcslen(pszValueData)+1;
    if (*pszValueData == _T('\"'))
    {
      dwValueSize -= 2;
      *(pszValueData+dwValueSize) = 0;
      pszValueData++;
    }
    dwValueSize *= sizeof(TCHAR);
    pDataBuffer = (BYTE *) new BYTE [dwValueSize];
    _tcscpy((TCHAR *)pDataBuffer,pszValueData);
    break;
  default:
    ASSERT(FALSE);
  }

  {
    size_t s = _tcslen(pszValueName);
    if (s && (pszValueName[0] == _T('\"'))&&(pszValueName[s-1] == _T('\"')))
    {
      pszValueName[s-1] = 0;
      pszValueName++;
    }
  }

  nError = Key.SetValue(pszValueName,dwType,pDataBuffer,dwValueSize);
  if (nError != ERROR_SUCCESS)
  {
    char Buffer[254];
    _stprintf(Buffer,_T("Cannot set value. Error is %u\n"),(unsigned int)nError);
    rConsole.Write(Buffer);
  }
  else
  {
    InvalidateCompletion();
  }

SkipCommand:
	if (pDataBuffer)
		delete pDataBuffer;
	return 0;

CommandNAonRoot:
  rConsole.Write(SET_VALUE_CMD COMMAND_NA_ON_ROOT);
  return 0;
}

const TCHAR * CShellCommandSetValue::GetHelpString()
{
	return SET_VALUE_CMD_SHORT_DESC
			_T("Syntax: ") SET_VALUE_CMD _T(" <TYPE> <VALUE> [<PATH>][<VALUE_NAME>] [/?]\n\n")
			_T("    <TYPE>       - Type of value to be set. Must be one of following:\n")
			_T("                    b    - binary value.\n")
			_T("                    dw   - A 32-bit number.\n")
			_T("                    dwle - A 32-bit number in little-endian format.\n")
			_T("                    dwbe - A 32-bit number in big-endian format.\n")
			_T("                    sz   - A null-terminated string.\n")
			_T("                    esz  - A null-terminated string that contains unexpanded\n")
			_T("                           references to environment variables.\n")
//			_T("                    msz  - An array of null-terminated strings,\n")
//			_T("                           terminated by two null characters.\n")
			_T("    <VALUE>      - The data to be set. According to <TYPE>, <VALUE> means:\n")
			_T("                    b    - name of file from which to read binary data.\n")
			_T("                    dw   \\\n")
			_T("                    dwle - number with syntax: [0 [{ x | X }]] [digits]\n")
			_T("                    dwbe /\n")
			_T("                    sz   \\\n")
			_T("                    esz  - <VALUE> string is interpreted as string\n")
			_T("    <PATH>       - Optional relative path of key which value will be processed.\n")
			_T("    <VALUE_NAME> - Name of key's value. Default is key's default value.\n")
			_T("    /?           - This help.\n");
}

const TCHAR * CShellCommandSetValue::GetHelpShortDescriptionString()
{
	return SET_VALUE_CMD_SHORT_DESC;
}
