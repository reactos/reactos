/* $Id: ShellCommandSetValue.cpp,v 1.1 2000/10/04 21:04:31 ea Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (c) 1999-2000 Nedko Arnaoudov <nedkohome@atia.com>
 *
 * License: GNU GPL
 *
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

BOOL CShellCommandSetValue::Match(const TCHAR *pchCommand)
{
	if (_tcsicmp(pchCommand,SET_VALUE_CMD) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,SET_VALUE_CMD _T(".."),SET_VALUE_CMD_LENGTH+2*sizeof(TCHAR)) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,SET_VALUE_CMD _T("/"),SET_VALUE_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,SET_VALUE_CMD _T("\\"),SET_VALUE_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
		return TRUE;
	return FALSE;
}

int CShellCommandSetValue::Execute(CConsole &rConsole, CArgumentParser& rArguments)
{
	rArguments.ResetArgumentIteration();
	TCHAR *pchCommandItself = rArguments.GetNextArgument();

	TCHAR *pchParameter;
	TCHAR *pchValueFull = NULL;
	TCHAR *pchValueData = NULL;
	BOOL blnBadParameter = FALSE;
	BOOL blnHelp = FALSE;
//	DWORD dwError;
	DWORD dwValueSize = 0;
	DWORD dwType = REG_NONE;
	BYTE *pDataBuffer = NULL;

	if ((_tcsnicmp(pchCommandItself,SET_VALUE_CMD _T(".."),SET_VALUE_CMD_LENGTH+2*sizeof(TCHAR)) == 0)||
		(_tcsnicmp(pchCommandItself,SET_VALUE_CMD _T("\\"),SET_VALUE_CMD_LENGTH+1*sizeof(TCHAR)) == 0))
	{
		pchValueFull = pchCommandItself + SET_VALUE_CMD_LENGTH;
	}
	else if (_tcsnicmp(pchCommandItself,SET_VALUE_CMD _T("/"),SET_VALUE_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
	{
		pchParameter = pchCommandItself + SET_VALUE_CMD_LENGTH;
		goto CheckValueArgument;
	}

	while((pchParameter = rArguments.GetNextArgument()) != NULL)
	{
CheckValueArgument:
		blnBadParameter = FALSE;
		if (((*pchParameter == _T('/'))||(*pchParameter == _T('-')))
			&&(*(pchParameter+1) == _T('?')))
		{
			blnHelp = TRUE;
		}
		else if (dwType == REG_NONE)
		{
			if (_tcsicmp(pchParameter,_T("b")) == 0)
			{
				dwType = REG_BINARY;
			}
			else if (_tcsicmp(pchParameter,_T("dw")) == 0)
			{
				dwType = REG_DWORD;
			}
			else if (_tcsicmp(pchParameter,_T("dwle")) == 0)
			{
				dwType = REG_DWORD_LITTLE_ENDIAN;
			}
			else if (_tcsicmp(pchParameter,_T("dwbe")) == 0)
			{
				dwType = REG_DWORD_BIG_ENDIAN;
			}
			else if (_tcsicmp(pchParameter,_T("sz")) == 0)
			{
				dwType = REG_SZ;
			}
			else if (_tcsicmp(pchParameter,_T("esz")) == 0)
			{
				dwType = REG_EXPAND_SZ;
			}
			else
			{
				blnBadParameter = TRUE;
			}
		}
		else if (pchValueData == NULL)
		{
			pchValueData = pchParameter;
		}
		else if (!pchValueFull)
		{
			pchValueFull = pchParameter;
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

	if (!pchValueData)
		blnHelp = TRUE;
	
	CRegistryTree *pTree = NULL;
	CRegistryKey *pKey = NULL;
	TCHAR *pchValueName;
	TCHAR *pchPath;
	
	if (blnHelp)
	{
		rConsole.Write(GetHelpString());

		if (pDataBuffer)
			delete pDataBuffer;

		return 0;
	}

	if (pchValueFull)
	{
		if (_tcscmp(pchValueFull,_T("\\")) == 0)
			goto CommandNAonRoot;

		TCHAR *pchSep = _tcsrchr(pchValueFull,_T('\\'));
		pchValueName = pchSep?(pchSep+1):(pchValueFull);
		pchPath = pchSep?pchValueFull:NULL;
				
		//if (_tcsrchr(pchValueName,_T('.')))
		//{
		//	pchValueName = _T("");
		//	pchPath = pchValueFull;
		//}
		//else
		if (pchSep)
			*pchSep = 0;
	}
	else
	{
		pchValueName = _T("");
		pchPath = NULL;
	}
		
	if (pchPath)
	{
		pTree = new CRegistryTree(m_rTree);
		if ((_tcscmp(pTree->GetCurrentPath(),m_rTree.GetCurrentPath()) != 0)
			||(!pTree->ChangeCurrentKey(pchPath)))
		{
			rConsole.Write(_T("Cannot open key "));
			rConsole.Write(pchPath);
			rConsole.Write(_T("\n"));
			goto SkipCommand;
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
	
	if (pKey)
	{	// not root key ???
		switch (dwType)
		{
		case REG_BINARY:
			{
				HANDLE hFile;
				DWORD dwBytesReaded;
				hFile = CreateFile(pchValueData,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
				if (hFile == INVALID_HANDLE_VALUE)
				{
					rConsole.Write(_T("Cannot open file "));
					rConsole.Write(pchValueData);
					rConsole.Write(_T("\n"));
					goto SkipCommand;
				}
				dwValueSize = GetFileSize(hFile,NULL);
				if (dwValueSize == -1)	// ok, that's right, we compare signed with unsigned here.
										// GetFileSize is documented and declared to return DWORD.
										// Error is indicated by checking if return is -1. Design->documentation bug ???
				{
					rConsole.Write(_T("Cannot get size of file "));
					rConsole.Write(pchValueData);
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
			if (!StringToDWORD(*(DWORD *)pDataBuffer,pchValueData))
			{
				rConsole.Write(_T("Cannot convert "));
				rConsole.Write(pchValueData);
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
			dwValueSize = _tcslen(pchValueData)+1;
			if (*pchValueData == _T('\"'))
			{
				dwValueSize -= 2;
				*(pchValueData+dwValueSize) = 0;
				pchValueData++;
			}
			dwValueSize *= sizeof(TCHAR);
			pDataBuffer = (BYTE *) new BYTE [dwValueSize];
			_tcscpy((TCHAR *)pDataBuffer,pchValueData);
			break;
		default:
			ASSERT(FALSE);
		}

		if (pKey->SetValue(pchValueName,dwType,pDataBuffer,dwValueSize) != ERROR_SUCCESS)
			rConsole.Write(_T("Cannot set value\n"));
	} // if (pKey)
	else
	{
CommandNAonRoot:
		rConsole.Write(SET_VALUE_CMD COMMAND_NA_ON_ROOT);
	}

SkipCommand:
	if (pTree)
		delete pTree;

	if (pDataBuffer)
		delete pDataBuffer;
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
