/* $Id: ShellCommandValue.cpp,v 1.2 2000/10/24 20:17:42 narnaoud Exp $
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

// ShellCommandValue.cpp: implementation of the CShellCommandValue class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "RegistryExplorer.h"
#include "ShellCommandValue.h"
#include "RegistryTree.h"
#include "RegistryKey.h"

#define VALUE_CMD				_T("VV")
#define VALUE_CMD_LENGTH		COMMAND_LENGTH(VALUE_CMD)
#define VALUE_CMD_SHORT_DESC	VALUE_CMD _T(" command is used to view value.\n")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellCommandValue::CShellCommandValue(CRegistryTree& rTree):m_rTree(rTree)
{
}

CShellCommandValue::~CShellCommandValue()
{
}

BOOL CShellCommandValue::Match(const TCHAR *pchCommand)
{
	if (_tcsicmp(pchCommand,VALUE_CMD) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,VALUE_CMD _T(".."),VALUE_CMD_LENGTH+2*sizeof(TCHAR)) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,VALUE_CMD _T("/"),VALUE_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
		return TRUE;
	if (_tcsnicmp(pchCommand,VALUE_CMD _T("\\"),VALUE_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
		return TRUE;
	return FALSE;
}

int CShellCommandValue::Execute(CConsole &rConsole, CArgumentParser& rArguments)
{
	rArguments.ResetArgumentIteration();
	TCHAR *pchCommandItself = rArguments.GetNextArgument();

	TCHAR *pchParameter;
	TCHAR *pchValueFull = NULL;
	BOOL blnUnicodeDump = FALSE;
	BOOL blnBadParameter = FALSE;
	BOOL blnHelp = FALSE;
	DWORD dwError;
	DWORD dwValueSize;
	DWORD dwType = REG_NONE;
	BYTE *pDataBuffer = NULL;
	TCHAR *pchFilename = NULL;

	if ((_tcsnicmp(pchCommandItself,VALUE_CMD _T(".."),VALUE_CMD_LENGTH+2*sizeof(TCHAR)) == 0)||
		(_tcsnicmp(pchCommandItself,VALUE_CMD _T("\\"),VALUE_CMD_LENGTH+1*sizeof(TCHAR)) == 0))
	{
		pchValueFull = pchCommandItself + VALUE_CMD_LENGTH;
	}
	else if (_tcsnicmp(pchCommandItself,VALUE_CMD _T("/"),VALUE_CMD_LENGTH+1*sizeof(TCHAR)) == 0)
	{
		pchParameter = pchCommandItself + VALUE_CMD_LENGTH;
		goto CheckValueArgument;
	}

	while((pchParameter = rArguments.GetNextArgument()) != NULL)
	{
CheckValueArgument:
		blnBadParameter = FALSE;
		if ((_tcsicmp(pchParameter,_T("/?")) == 0)
			||(_tcsicmp(pchParameter,_T("-?")) == 0))
		{
			blnHelp = TRUE;
			break;
		}
		else if (_tcsicmp(pchParameter,_T("/u")) == 0)
		{
			blnUnicodeDump = TRUE;
		}
		else if ((*pchParameter == _T('/'))&&(*(pchParameter+1) == _T('f')))
		{
			pchFilename = pchParameter+2;
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
			goto ValueCommandNAonRoot;

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
			goto SkipValueCommand;
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
		rConsole.Write(_T("Value name : \""));
		rConsole.Write(_T("\\"));
		rConsole.Write(pTree?pTree->GetCurrentPath():m_rTree.GetCurrentPath());
		rConsole.Write(*pchValueName?pchValueName:_T("\"(Default)\""));
		rConsole.Write(_T("\"\n"));
		size_t l = _tcslen(pchValueName);
		if (l&&
			(*pchValueName == _T('\"'))&&
			(pchValueName[l-1] == _T('\"')))
		{
			pchValueName[l-1] = 0;
			pchValueName++;
		}
		dwError = pKey->GetValue(pchValueName,NULL,NULL,&dwValueSize);
		if (dwError == ERROR_SUCCESS)
		{
			pDataBuffer = new BYTE [dwValueSize];
			pKey->GetValue(pchValueName,&dwType,pDataBuffer,&dwValueSize);
			rConsole.Write(_T("Value type : "));
			rConsole.Write(CRegistryKey::GetValueTypeName(dwType));
			rConsole.Write(_T("\nValue data : "));
			switch(dwType)
			{
			case REG_DWORD_LITTLE_ENDIAN:
				{
					TCHAR Buffer[11];
					unsigned int n = *pDataBuffer;
					_stprintf(Buffer,_T("0x%08X\n"),n);
					rConsole.Write(Buffer);
				}
				break;
			case REG_DWORD_BIG_ENDIAN:
				{
					TCHAR Buffer[3];
					rConsole.Write(_T("0x"));
					for (unsigned int i = 0 ; i < dwValueSize ; i++)
					{
						_stprintf(Buffer,_T("%02X"),*(pDataBuffer+i));
						rConsole.Write(Buffer);
					}
				}
				rConsole.Write(_T("\n"));
				break;
			case REG_LINK:
				break;
			case REG_MULTI_SZ:
				{
					TCHAR *pchCurrentString = (TCHAR *)pDataBuffer;
					rConsole.Write(_T("\n"));
					while(*pchCurrentString)
					{
						rConsole.Write(_T("\""));
						rConsole.Write(pchCurrentString);
						rConsole.Write(_T("\"\n"));
						pchCurrentString += _tcslen(pchCurrentString)+1;
					}
				}
				break;
			case REG_RESOURCE_LIST:
				break;
			case REG_SZ:
			case REG_EXPAND_SZ:
				rConsole.Write(_T("\""));
				rConsole.Write((TCHAR *)pDataBuffer);
				rConsole.Write(_T("\"\n"));
				break;
			case REG_BINARY:
			default:
				{
					TCHAR Buffer[256];
					DWORD i;
					for (i = 0 ; i < dwValueSize ; i++)
					{
						if (i%16 == 0)
						{	// ok this is begining of line
							rConsole.Write(_T("\n"));
							// print offset
							_stprintf(Buffer,_T("0x%08X  "),i);
							rConsole.Write(Buffer);
						}
						else if (i%8 == 0)
						{	// this is the additional space between 7th and 8th byte in current line
							rConsole.Write(_T(" "));
						}

						// print current byte
						unsigned int n = *(pDataBuffer+i);
						_stprintf(Buffer,_T("%02X "),n);
						rConsole.Write(Buffer);

						if (i && (i%16 == 15))
						{	// if this is the last byte in line
							// Dump text representation
							for (DWORD j = i-15; j <= i; j += blnUnicodeDump?2:1)\
							{
								if ((j%8 == 0)&&(j%16 != 0))
								{	// this is the additional space between 7th and 8th byte in current line
									rConsole.Write(_T(" "));
								}
								ASSERT(i-j < 16);
								// write current char representation
								if (blnUnicodeDump)
								{
									ASSERT(j%2 == 0);
									wchar_t ch = *(TCHAR *)(pDataBuffer+j);
									_stprintf(Buffer,
#ifdef _UNICODE
										_T("%c"),
#else
										_T("%C"),
#endif
										iswprint(ch)?ch:L'.');
								}
								else
								{
									unsigned char ch = *(pDataBuffer+j);
									_stprintf(Buffer,
#ifdef _UNICODE
										_T("%C"),
#else
										_T("%c"),
#endif
										isprint(ch)?ch:'.');
								}
								rConsole.Write(Buffer);
							}	// for
						}	// if
					}	// for

					// print text representation of last line if it is not full (it have less than 16 bytes)
					// k is pseudo offset
					for (DWORD k = i; k%16 != 0; k++)
					{
						if (k%8 == 0)
						{	// this is the additional space between 7th and 8th byte in current line
							rConsole.Write(_T(" "));
						}
						_tcscpy(Buffer,_T("   "));	// the replacement of two digit of current byte + spacing
						rConsole.Write(Buffer);
						if (k && (k%16 == 15))
						{	// if this is the last byte in line
							ASSERT((k-15)%16 == 0);	// k-15 must point at begin of last line
							for (DWORD j = k-15; j < i; j += j += blnUnicodeDump?2:1)
							{
								if (blnUnicodeDump&&(j+1 >= i))
								{	// ok, buffer size is odd number, so we don't display last byte.
									ASSERT(j+1 == i);
									break;
								}
								if ((j%8 == 0)&&(j%16 != 0))
								{	// this is the additional space between 7th and 8th byte in current line
									rConsole.Write(_T(" "));
								}

								// write current char representation
								if (blnUnicodeDump)
								{
									ASSERT(j%2 == 0);
									wchar_t ch = *(TCHAR *)(pDataBuffer+j);
									_stprintf(Buffer,
#ifdef _UNICODE
										_T("%c"),
#else
										_T("%C"),
#endif
										iswprint(ch)?ch:L'.');
								}
								else
								{
									unsigned char ch = *(pDataBuffer+j);
									_stprintf(Buffer,
#ifdef _UNICODE
										_T("%C"),
#else
										_T("%c"),
#endif
										isprint(ch)?ch:'.');
								}
								rConsole.Write(Buffer);
							} // for
						} // if
					} // for
				} // default:
				rConsole.Write(_T("\n"));
			} // switch
			rConsole.Write(_T("\n"));

			if (pchFilename)
			{
				rConsole.Write(_T("Exporting value data to "));
				rConsole.Write(pchFilename);
				rConsole.Write(_T(" ...\n"));

				HANDLE hFile = CreateFile(pchFilename,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
				if (hFile == INVALID_HANDLE_VALUE)
				{
					rConsole.Write(_T("Cannot create new file "));
					rConsole.Write(pchFilename);
					rConsole.Write(_T("\n"));
					goto SkipValueCommand;
				}

				DWORD dwBytesWritten;
				if (!WriteFile(hFile,pDataBuffer,dwValueSize,&dwBytesWritten,NULL))
				{
					rConsole.Write(_T("Error writting file.\n"));
					VERIFY(CloseHandle(hFile));
					goto SkipValueCommand;
				}

				ASSERT(dwBytesWritten == dwValueSize);
				VERIFY(CloseHandle(hFile));
			}
		}
		else
		{
			rConsole.Write(_T("Error "));
			TCHAR Buffer[256];
			rConsole.Write(_itot(dwError,Buffer,10));
			rConsole.Write(_T("\n"));
			if (dwError == 2)
			{
				rConsole.Write(_T("(System Cannot find the value specified)\n"));
			}
		}
	} // if (pKey)
	else
	{
ValueCommandNAonRoot:
		rConsole.Write(VALUE_CMD COMMAND_NA_ON_ROOT);
	}

SkipValueCommand:
	if (pTree)
		delete pTree;

	if (pDataBuffer)
		delete pDataBuffer;
	return 0;
}

const TCHAR * CShellCommandValue::GetHelpString()
{
	return VALUE_CMD_SHORT_DESC
			_T("Syntax: ") VALUE_CMD _T(" [<PATH>][<VALUE_NAME>] [/u] [/?]\n\n")
			_T("    <PATH>       - Optional relative path of key which value will be processed.\n")
			_T("    <VALUE_NAME> - Name of key's value. Default is key's default value.\n")
			_T("    /u           - On binary dump view as Unicode.\n")
			_T("    /fFILE       - Export value data to FILE.\n")
			_T("    /?           - This help.\n\n")
			_T("Without parameters, command displays default value of current key.\n");
}

const TCHAR * CShellCommandValue::GetHelpShortDescriptionString()
{
	return VALUE_CMD_SHORT_DESC;
}
