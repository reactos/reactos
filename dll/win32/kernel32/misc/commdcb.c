/*
	Copyright (c) 2008 KJK::Hyperion

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.
*/

/* Parses a mode string for a serial port, in the same syntax as the mode.com command */

#if defined(__REACTOS__) && defined(_KERNEL32_)
#include <k32.h>

#define DCB_BuildCommDCBA            BuildCommDCBA
#define DCB_BuildCommDCBAndTimeoutsA BuildCommDCBAndTimeoutsA
#define DCB_BuildCommDCBW            BuildCommDCBW
#define DCB_BuildCommDCBAndTimeoutsW BuildCommDCBAndTimeoutsW
#else
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static
void DCB_SkipSpace(const char ** ppTail)
{
	while(**ppTail && isspace(**ppTail))
		++ *ppTail;
}

static
size_t DCB_SkipNonSpace(const char ** ppTail)
{
	const char * pOriginal = *ppTail;

	while(**ppTail && !isspace(**ppTail))
		++ *ppTail;

	return *ppTail - pOriginal;
}

static
BOOL DCB_SetBaudRate(unsigned long baudRate, LPDCB lpDCB)
{
	switch(baudRate)
	{
	case 11: lpDCB->BaudRate = 110; break;
	case 15: lpDCB->BaudRate = 150; break;
	case 30: lpDCB->BaudRate = 300; break;
	case 60: lpDCB->BaudRate = 600; break;
	case 12: lpDCB->BaudRate = 1200; break;
	case 24: lpDCB->BaudRate = 2400; break;
	case 48: lpDCB->BaudRate = 4800; break;
	case 96: lpDCB->BaudRate = 9600; break;
	case 19: lpDCB->BaudRate = 19200; break;
	default: lpDCB->BaudRate = baudRate; break;
	}

	return TRUE;
}

static
BYTE DCB_SetParity(char parity, LPDCB lpDCB)
{
	switch(parity)
	{
	case 'N':
	case 'n':
		lpDCB->Parity = 0;
		break;

	case 'O':
	case 'o':
		lpDCB->Parity = 1;
		break;

	case 'E':
	case 'e':
		lpDCB->Parity = 2;
		break;

	case 'M':
	case 'm':
		lpDCB->Parity = 3;
		break;

	case 'S':
	case 's':
		lpDCB->Parity = 4;
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

static
BYTE DCB_SetDataBits(unsigned long dataBits, LPDCB lpDCB)
{
	BOOL bRet;

	bRet = dataBits >= 5 && dataBits <= 8;

	if(!bRet)
		return bRet;

	lpDCB->ByteSize = (BYTE)dataBits;
	return bRet;
}

static
BOOL DCB_ParseOldSeparator(const char ** ppTail)
{
	BOOL bRet;

	bRet = **ppTail == 0;

	if(bRet)
		return bRet;

	bRet = **ppTail == ',';

	if(bRet)
	{
		++ *ppTail;
		return bRet;
	}

	return bRet;
}

static
unsigned long DCB_ParseOldNumber(const char ** ppTail, unsigned long nDefault)
{
	char * pNumTail;
	unsigned long number;

	DCB_SkipSpace(ppTail);

	if(!isdigit(**ppTail))
		return nDefault;

	number = strtoul(*ppTail, &pNumTail, 10);
	*ppTail = pNumTail;

	DCB_SkipSpace(ppTail);
	return number;
}

static
char DCB_ParseOldCharacter(const char ** ppTail, char cDefault)
{
	char character;

	DCB_SkipSpace(ppTail);

	if(**ppTail == 0 || **ppTail == ',')
		return cDefault;

	character = **ppTail;
	++ *ppTail;

	DCB_SkipSpace(ppTail);
	return character;
}

static
const char * DCB_ParseOldString(const char ** ppTail, const char * pDefault, size_t * pLength)
{
	const char * string;

	DCB_SkipSpace(ppTail);

	if(**ppTail == 0 || **ppTail == ',')
		return pDefault;

	string = *ppTail;

	*pLength = 0;

	while(**ppTail != 0 && **ppTail != ',' && !isspace(**ppTail))
	{
		++ *ppTail;
		++ *pLength;
	}

	DCB_SkipSpace(ppTail);
	return string;
}

static
BOOL
DCB_ParseOldMode(const char * pTail, LPDCB lpDCB, LPCOMMTIMEOUTS lpCommTimeouts)
{
	BOOL bRet;

	unsigned long baudRate;
	char parity;
	unsigned long dataBits;
	size_t stopBitsLength;
	const char * stopBits;
	char retry;

	/* Baud rate */
	baudRate = DCB_ParseOldNumber(&pTail, 0);
	bRet = DCB_ParseOldSeparator(&pTail);
	bRet = bRet && DCB_SetBaudRate(baudRate, lpDCB);

	if(!bRet)
		return bRet;

	/* Parity */
	parity = DCB_ParseOldCharacter(&pTail, 'E');
	bRet = DCB_ParseOldSeparator(&pTail);
	bRet = bRet && DCB_SetParity(parity, lpDCB);

	if(!bRet)
		return bRet;

	/* Data bits */
	dataBits = DCB_ParseOldNumber(&pTail, 7);
	bRet = DCB_ParseOldSeparator(&pTail);
	bRet = bRet && DCB_SetDataBits(dataBits, lpDCB);

	if(!bRet)
		return bRet;

	/* Stop bits */
	stopBitsLength = 1;
	stopBits = DCB_ParseOldString(&pTail, baudRate == 110 ? "2" : "1", &stopBitsLength);
	bRet = DCB_ParseOldSeparator(&pTail);

	if(!bRet)
		return bRet;

	if(strncmp(stopBits, "1", stopBitsLength) == 0)
		lpDCB->StopBits = 0;
	else if(strncmp(stopBits, "1.5", stopBitsLength) == 0)
		lpDCB->StopBits = 1;
	else if(strncmp(stopBits, "2", stopBitsLength) == 0)
		lpDCB->StopBits = 2;
	else
		return FALSE;

	/* Retry */
	retry = DCB_ParseOldCharacter(&pTail, 0);
	bRet = *pTail == 0;

	if(!bRet)
		return bRet;

	switch(retry)
	{
	case 0:
		lpDCB->fInX = FALSE;
		lpDCB->fOutX = FALSE;
		lpDCB->fOutxCtsFlow = FALSE;
		lpDCB->fOutxDsrFlow = FALSE;
		lpDCB->fDtrControl = DTR_CONTROL_ENABLE;
		lpDCB->fRtsControl = RTS_CONTROL_ENABLE;
		break;

	case 'p':
	case 'P':
		lpDCB->fInX = FALSE;
		lpDCB->fOutX = FALSE;
		lpDCB->fOutxCtsFlow = TRUE;
		lpDCB->fOutxDsrFlow = TRUE;
		lpDCB->fDtrControl = DTR_CONTROL_HANDSHAKE;
		lpDCB->fRtsControl = RTS_CONTROL_HANDSHAKE;
		break;

	case 'x':
	case 'X':
		lpDCB->fInX = TRUE;
		lpDCB->fOutX = TRUE;
		lpDCB->fOutxCtsFlow = FALSE;
		lpDCB->fOutxDsrFlow = FALSE;
		lpDCB->fDtrControl = DTR_CONTROL_ENABLE;
		lpDCB->fRtsControl = RTS_CONTROL_ENABLE;
		break;

	default:
		return FALSE;
	}

	return bRet;
}

static
BOOL DCB_ParseNewNumber(const char * pString, size_t cchString, unsigned long * pNumber)
{
	BOOL bRet;
	char * pStringEnd;
	unsigned long number;

	bRet = cchString > 0;

	if(!bRet)
		return bRet;

	number = strtoul(pString, &pStringEnd, 10);

	bRet = pStringEnd - pString == cchString;

	if(!bRet)
		return bRet;

	*pNumber = number;
	return bRet;
}

static
BOOL DCB_ParseNewBoolean(const char * pString, size_t cchString, BOOL * pBoolean)
{
	BOOL bRet;

	bRet = _strnicmp(pString, "on", cchString) == 0;

	if(bRet)
	{
		*pBoolean = bRet;
		return bRet;
	}

	bRet = _strnicmp(pString, "off", cchString) == 0;

	if(bRet)
	{
		*pBoolean = !bRet;
		return bRet;
	}

	return bRet;
}

static
BOOL
DCB_ParseNewMode(const char * pTail, LPDCB lpDCB, LPCOMMTIMEOUTS lpCommTimeouts)
{
	BOOL bRet;
	BOOL stopBitsSet = FALSE;

	lpDCB->StopBits = 0;

	while(*pTail)
	{
		const char * pArg;
		size_t cchArg;
		size_t cchArgName;
		size_t cchArgValue;
		const char * pArgName;
		const char * pArgValue;
		unsigned long nArgValue;
		BOOL fArgValue;

		pArg = pTail;
		cchArg = DCB_SkipNonSpace(&pTail);
		DCB_SkipSpace(&pTail);

		for(cchArgName = 0; cchArgName < cchArg; ++ cchArgName)
		{
			if(pArg[cchArgName] == '=')
				break;
		}

		bRet = cchArgName < cchArg;

		if(!bRet)
			return bRet;

		cchArgValue = cchArg - cchArgName - 1;
		pArgName = pArg;
		pArgValue = pArg + cchArgName + 1;

		if(_strnicmp(pArgName, "baud", cchArgName) == 0)
		{
			bRet = DCB_ParseNewNumber(pArgValue, cchArgValue, &nArgValue);
			bRet = bRet && DCB_SetBaudRate(nArgValue, lpDCB);

			if(bRet)
			{
				if(lpDCB->BaudRate == 110 && !stopBitsSet)
					lpDCB->StopBits = 2;
				else
					lpDCB->StopBits = 0;
			}
		}
		else if(_strnicmp(pArgName, "parity", cchArgName) == 0)
		{
			bRet = cchArgValue == 1;
			bRet = bRet && DCB_SetParity(pArgValue[0], lpDCB);
		}
		else if(_strnicmp(pArgName, "data", cchArgName) == 0)
		{
			bRet = DCB_ParseNewNumber(pArgValue, cchArgValue, &nArgValue);
			bRet = bRet && DCB_SetDataBits(nArgValue, lpDCB);
		}
		else if(_strnicmp(pArgName, "stop", cchArgName) == 0)
		{
			stopBitsSet = TRUE;

			if(strncmp(pArgValue, "1", cchArgValue) == 0)
				lpDCB->StopBits = 0;
			else if(strncmp(pArgValue, "1.5", cchArgValue) == 0)
				lpDCB->StopBits = 1;
			else if(strncmp(pArgValue, "2", cchArgValue) == 0)
				lpDCB->StopBits = 2;
			else
				bRet = FALSE;
		}
		else if(_strnicmp(pArgName, "to", cchArgName) == 0)
		{
			bRet = DCB_ParseNewBoolean(pArgValue, cchArgValue, &fArgValue);

			if(bRet)
			{
				if(lpCommTimeouts)
				{
					memset(lpCommTimeouts, 0, sizeof(*lpCommTimeouts));

					if(fArgValue)
						lpCommTimeouts->WriteTotalTimeoutConstant = 60000;
				}
			}
		}
		else if(_strnicmp(pArgName, "xon", cchArgName) == 0)
		{
			bRet = DCB_ParseNewBoolean(pArgValue, cchArgValue, &fArgValue);

			if(bRet)
			{
				lpDCB->fInX = !!fArgValue;
				lpDCB->fOutX = !!fArgValue;
			}
		}
		else if(_strnicmp(pArgName, "odsr", cchArgName) == 0)
		{
			bRet = DCB_ParseNewBoolean(pArgValue, cchArgValue, &fArgValue);

			if(bRet)
				lpDCB->fOutxDsrFlow = !!fArgValue;
		}
		else if(_strnicmp(pArgName, "octs", cchArgName) == 0)
		{
			bRet = DCB_ParseNewBoolean(pArgValue, cchArgValue, &fArgValue);

			if(bRet)
				lpDCB->fOutxCtsFlow = !!fArgValue;
		}
		else if(_strnicmp(pArgName, "dtr", cchArgName) == 0)
		{
			bRet = DCB_ParseNewBoolean(pArgValue, cchArgValue, &fArgValue);

			if(bRet)
			{
				if(fArgValue)
					lpDCB->fDtrControl = DTR_CONTROL_ENABLE;
				else
					lpDCB->fDtrControl = DTR_CONTROL_DISABLE;
			}
			else
			{
				bRet = _strnicmp(pArgValue, "hs", cchArgValue) == 0;

				if(bRet)
					lpDCB->fDtrControl = DTR_CONTROL_HANDSHAKE;
			}
		}
		else if(_strnicmp(pArgName, "rts", cchArgName) == 0)
		{
			bRet = DCB_ParseNewBoolean(pArgValue, cchArgValue, &fArgValue);

			if(bRet)
			{
				if(fArgValue)
					lpDCB->fRtsControl = RTS_CONTROL_ENABLE;
				else
					lpDCB->fRtsControl = RTS_CONTROL_DISABLE;
			}
			else
			{
				bRet = _strnicmp(pArgValue, "hs", cchArgValue) == 0;

				if(bRet)
					lpDCB->fRtsControl = RTS_CONTROL_HANDSHAKE;
				else
				{
					bRet = _strnicmp(pArgValue, "tg", cchArgValue) == 0;

					if(bRet)
						lpDCB->fRtsControl = RTS_CONTROL_TOGGLE;
				}
			}
		}
		else if(_strnicmp(pArgName, "idsr", cchArgName) == 0)
		{
			bRet = DCB_ParseNewBoolean(pArgValue, cchArgValue, &fArgValue);

			if(bRet)
				lpDCB->fDsrSensitivity = !!fArgValue;
		}
		else
			bRet = FALSE;

		if(!bRet)
			return bRet;
	}

	return TRUE;
}

static
BOOL
DCB_ValidPort(unsigned long nPort)
{
	BOOL bRet;
	DWORD dwErr;
	WCHAR szPort[3 + 10 + 1];

	dwErr = GetLastError();

	_snwprintf(szPort, sizeof(szPort) / sizeof(szPort[0]), L"COM%lu", nPort);

	bRet = QueryDosDeviceW(szPort, NULL, 0) == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER;

	if(!bRet)
		dwErr = ERROR_INVALID_PARAMETER;

	SetLastError(dwErr);
	return bRet;
}

/*
 * @implemented
 */
BOOL
WINAPI
DCB_BuildCommDCBAndTimeoutsA(LPCSTR lpDef, LPDCB lpDCB, LPCOMMTIMEOUTS lpCommTimeouts)
{
	BOOL bRet;
	LPCSTR pTail = lpDef;
	DCB DCBCopy;

	if(_strnicmp(pTail, "COM", 3) == 0)
	{
		char * pNumTail;
		unsigned long nPort;

		pTail += 3;

		if(!isdigit(*pTail))
			return FALSE;

		nPort = strtoul(pTail, &pNumTail, 10);
		pTail = pNumTail;

		bRet = DCB_ValidPort(nPort);

		if(!bRet)
			return bRet;

		DCB_SkipSpace(&pTail);

		if(*pTail == ':')
			++ pTail;

		DCB_SkipSpace(&pTail);
	}

	DCBCopy = *lpDCB;

	if(isdigit(*pTail))
		bRet = DCB_ParseOldMode(pTail, &DCBCopy, lpCommTimeouts);
	else
		bRet = DCB_ParseNewMode(pTail, &DCBCopy, lpCommTimeouts);

	if(!bRet)
		SetLastError(ERROR_INVALID_PARAMETER);
	else
		*lpDCB = DCBCopy;

	return bRet;
}

/*
 * @implemented
 */
BOOL
WINAPI
DCB_BuildCommDCBA(LPCSTR lpDef, LPDCB lpDCB)
{
    return DCB_BuildCommDCBAndTimeoutsA(lpDef, lpDCB, NULL);
}

/*
 * @implemented
 */
BOOL
WINAPI
DCB_BuildCommDCBAndTimeoutsW(LPCWSTR lpDef, LPDCB lpDCB, LPCOMMTIMEOUTS lpCommTimeouts)
{
	BOOL bRet;
	HANDLE hHeap;
	BOOL bInvalidChars;
	LPSTR pszAscii;
	int cchAscii;
	DWORD dwErr;

	dwErr = ERROR_INVALID_PARAMETER;
	cchAscii = WideCharToMultiByte(20127, 0, lpDef, -1, NULL, 0, NULL, NULL);

	bRet = cchAscii > 0;

	if(bRet)
	{
		hHeap = GetProcessHeap();
		pszAscii = HeapAlloc(hHeap, 0, cchAscii);

		bRet = pszAscii != NULL;

		if(bRet)
		{
			bInvalidChars = FALSE;
			cchAscii = WideCharToMultiByte(20127, 0, lpDef, -1, pszAscii, cchAscii, NULL, &bInvalidChars);

			bRet = cchAscii > 0 && !bInvalidChars;

			if(bRet)
				bRet = DCB_BuildCommDCBAndTimeoutsA(pszAscii, lpDCB, lpCommTimeouts);

			HeapFree(hHeap, 0, pszAscii);
		}
		else
			dwErr = ERROR_OUTOFMEMORY;
	}

	if(!bRet)
		SetLastError(dwErr);

	return bRet;
}

/*
 * @implemented
 */
BOOL
WINAPI
DCB_BuildCommDCBW(LPCWSTR lpDef, LPDCB lpDCB)
{
    return DCB_BuildCommDCBAndTimeoutsW(lpDef, lpDCB, NULL);
}

/* EOF */
