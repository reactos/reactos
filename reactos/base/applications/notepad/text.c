/*
 *  Notepad (text.c)
 *
 *  Copyright 1998,99 Marcel Baur <mbaur@g26.ethz.ch>
 *  Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *  Copyright 2002 Andriy Palamarchuk
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <notepad.h>

static BOOL Append(LPWSTR *ppszText, DWORD *pdwTextLen, LPCWSTR pszAppendText, DWORD dwAppendLen)
{
	LPWSTR pszNewText;

	if (dwAppendLen > 0)
	{
		if (*ppszText)
		{
			pszNewText = (LPWSTR) HeapReAlloc(GetProcessHeap(), 0, *ppszText, (*pdwTextLen + dwAppendLen) * sizeof(WCHAR));
		}
		else
		{
			pszNewText = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, dwAppendLen * sizeof(WCHAR));
		}

		if (!pszNewText)
			return FALSE;

		memcpy(pszNewText + *pdwTextLen, pszAppendText, dwAppendLen * sizeof(WCHAR));
		*ppszText = pszNewText;
		*pdwTextLen += dwAppendLen;
	}
	return TRUE;
}

BOOL ReadText(HANDLE hFile, LPWSTR *ppszText, DWORD *pdwTextLen, int *piEncoding, int *piEoln)
{
	DWORD dwSize;
	LPBYTE pBytes = NULL;
	LPCWSTR pszText;
	LPWSTR pszAllocText = NULL;
	DWORD dwPos, i;
	DWORD dwCharCount;
	BOOL bSuccess = FALSE;
	BYTE b = 0;
	int iEncoding = ENCODING_ANSI;
	int iCodePage = 0;
	WCHAR szCrlf[2] = { '\r', '\n' };
	DWORD adwEolnCount[3] = { 0, 0, 0 };

	*ppszText = NULL;
	*pdwTextLen = 0;

	dwSize = GetFileSize(hFile, NULL);
	if (dwSize == INVALID_FILE_SIZE)
		goto done;

	pBytes = HeapAlloc(GetProcessHeap(), 0, dwSize + 2);
	if (!pBytes)
		goto done;

	if (!ReadFile(hFile, pBytes, dwSize, &dwSize, NULL))
		goto done;
	dwPos = 0;

	/* Make sure that there is a NUL character at the end, in any encoding */
	pBytes[dwSize + 0] = '\0';
	pBytes[dwSize + 1] = '\0';

	/* Look for Byte Order Marks */
	if ((dwSize >= 2) && (pBytes[0] == 0xFF) && (pBytes[1] == 0xFE))
	{
		iEncoding = ENCODING_UNICODE;
		dwPos += 2;
	}
	else if ((dwSize >= 2) && (pBytes[0] == 0xFE) && (pBytes[1] == 0xFF))
	{
		iEncoding = ENCODING_UNICODE_BE;
		dwPos += 2;
	}
	else if ((dwSize >= 3) && (pBytes[0] == 0xEF) && (pBytes[1] == 0xBB) && (pBytes[2] == 0xBF))
	{
		iEncoding = ENCODING_UTF8;
		dwPos += 3;
	}

	switch(iEncoding)
	{
	case ENCODING_UNICODE_BE:
		for (i = dwPos; i < dwSize-1; i += 2)
		{
			b = pBytes[i+0];
			pBytes[i+0] = pBytes[i+1];
			pBytes[i+1] = b;
		}
		/* fall through */

	case ENCODING_UNICODE:
		pszText = (LPCWSTR) &pBytes[dwPos];
		dwCharCount = (dwSize - dwPos) / sizeof(WCHAR);
		break;

	case ENCODING_ANSI:
	case ENCODING_UTF8:
		if (iEncoding == ENCODING_ANSI)
			iCodePage = CP_ACP;
		else if (iEncoding == ENCODING_UTF8)
			iCodePage = CP_UTF8;

		if ((dwSize - dwPos) > 0)
		{
			dwCharCount = MultiByteToWideChar(iCodePage, 0, (LPCSTR)&pBytes[dwPos], dwSize - dwPos, NULL, 0);
			if (dwCharCount == 0)
				goto done;
		}
		else
		{
			/* special case for files with no characters (other than BOMs) */
			dwCharCount = 0;
		}

		pszAllocText = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, (dwCharCount + 1) * sizeof(WCHAR));
		if (!pszAllocText)
			goto done;

		if ((dwSize - dwPos) > 0)
		{
			if (!MultiByteToWideChar(iCodePage, 0, (LPCSTR)&pBytes[dwPos], dwSize - dwPos, pszAllocText, dwCharCount))
				goto done;
		}

		pszAllocText[dwCharCount] = '\0';
		pszText = pszAllocText;
		break;
	}

	dwPos = 0;
	for (i = 0; i < dwCharCount; i++)
	{
		switch(pszText[i])
		{
		case '\r':
			if ((i < dwCharCount-1) && (pszText[i+1] == '\n'))
			{
				i++;
				adwEolnCount[EOLN_CRLF]++;
				break;
			}
			/* fall through */

		case '\n':
			if (!Append(ppszText, pdwTextLen, &pszText[dwPos], i - dwPos))
				return FALSE;
			if (!Append(ppszText, pdwTextLen, szCrlf, sizeof(szCrlf) / sizeof(szCrlf[0])))
				return FALSE;
			dwPos = i + 1;

			if (pszText[i] == '\r')
				adwEolnCount[EOLN_CR]++;
			else
				adwEolnCount[EOLN_LF]++;
			break;
		}
	}

	if (!*ppszText && (pszText == pszAllocText))
	{
		/* special case; don't need to reallocate */
		*ppszText = pszAllocText;
		*pdwTextLen = dwCharCount;
		pszAllocText = NULL;
	}
	else
	{
		/* append last remaining text */
		if (!Append(ppszText, pdwTextLen, &pszText[dwPos], i - dwPos + 1))
			return FALSE;
	}

	/* chose which eoln to use */
	*piEoln = EOLN_CRLF;
	if (adwEolnCount[EOLN_LF] > adwEolnCount[*piEoln])
		*piEoln = EOLN_LF;
	if (adwEolnCount[EOLN_CR] > adwEolnCount[*piEoln])
		*piEoln = EOLN_CR;
	*piEncoding = iEncoding;

	bSuccess = TRUE;

done:
	if (pBytes)
		HeapFree(GetProcessHeap(), 0, pBytes);
	if (pszAllocText)
		HeapFree(GetProcessHeap(), 0, pszAllocText);

	if (!bSuccess && *ppszText)
	{
		HeapFree(GetProcessHeap(), 0, *ppszText);
		*ppszText = NULL;
		*pdwTextLen = 0;
	}
	return bSuccess;
}

static BOOL WriteEncodedText(HANDLE hFile, LPCWSTR pszText, DWORD dwTextLen, int iEncoding)
{
	LPBYTE pBytes = NULL;
	LPBYTE pAllocBuffer = NULL;
	DWORD dwPos = 0;
	DWORD dwByteCount;
	BYTE buffer[1024];
	UINT iCodePage = 0;
	DWORD dwDummy, i;
	BOOL bSuccess = FALSE;
	int iBufferSize, iRequiredBytes;
	BYTE b;

	while(dwPos < dwTextLen)
	{
		switch(iEncoding)
		{
			case ENCODING_UNICODE:
				pBytes = (LPBYTE) &pszText[dwPos];
				dwByteCount = (dwTextLen - dwPos) * sizeof(WCHAR);
				dwPos = dwTextLen;
				break;

			case ENCODING_UNICODE_BE:
				dwByteCount = (dwTextLen - dwPos) * sizeof(WCHAR);
				if (dwByteCount > sizeof(buffer))
					dwByteCount = sizeof(buffer);

				memcpy(buffer, &pszText[dwPos], dwByteCount);
				for (i = 0; i < dwByteCount; i += 2)
				{
					b = buffer[i+0];
					buffer[i+0] = buffer[i+1];
					buffer[i+1] = b;
				}
				pBytes = (LPBYTE) &buffer[dwPos];
				dwPos += dwByteCount / sizeof(WCHAR);
				break;

			case ENCODING_ANSI:
			case ENCODING_UTF8:
				if (iEncoding == ENCODING_ANSI)
					iCodePage = CP_ACP;
				else if (iEncoding == ENCODING_UTF8)
					iCodePage = CP_UTF8;

				iRequiredBytes = WideCharToMultiByte(iCodePage, 0, &pszText[dwPos], dwTextLen - dwPos, NULL, 0, NULL, NULL);
				if (iRequiredBytes <= 0)
				{
					goto done;
				}
				else if (iRequiredBytes < sizeof(buffer))
				{
					pBytes = buffer;
					iBufferSize = sizeof(buffer);
				}
				else
				{
					pAllocBuffer = (LPBYTE) HeapAlloc(GetProcessHeap(), 0, iRequiredBytes);
					if (!pAllocBuffer)
						return FALSE;
					pBytes = pAllocBuffer;
					iBufferSize = iRequiredBytes;
				}

				dwByteCount = WideCharToMultiByte(iCodePage, 0, &pszText[dwPos], dwTextLen - dwPos, (LPSTR) pBytes, iBufferSize, NULL, NULL);
				if (!dwByteCount)
					goto done;

				dwPos = dwTextLen;
				break;

			default:
				goto done;
		}

		if (!WriteFile(hFile, pBytes, dwByteCount, &dwDummy, NULL))
			goto done;

		/* free the buffer, if we have allocated one */
		if (pAllocBuffer)
		{
			HeapFree(GetProcessHeap(), 0, pAllocBuffer);
			pAllocBuffer = NULL;
		}
	}
	bSuccess = TRUE;

done:
	if (pAllocBuffer)
		HeapFree(GetProcessHeap(), 0, pAllocBuffer);
	return bSuccess;
}

BOOL WriteText(HANDLE hFile, LPCWSTR pszText, DWORD dwTextLen, int iEncoding, int iEoln)
{
  WCHAR wcBom;
  BYTE bEoln[2];
  LPBYTE pbEoln = NULL;
  DWORD dwDummy, dwPos, dwNext, dwEolnSize = 0;

  /* Write the proper byte order marks if not ANSI */
  if (iEncoding != ENCODING_ANSI)
  {
    wcBom = 0xFEFF;
    if (!WriteEncodedText(hFile, &wcBom, 1, iEncoding))
      return FALSE;
  }

  /* Identify the proper eoln to use */
  switch(iEoln)
  {
      case EOLN_LF:
        bEoln[0] = '\n';
        pbEoln = (LPBYTE) &bEoln;
        dwEolnSize = 1;
        break;
      case EOLN_CR:
        bEoln[0] = '\r';
        pbEoln = (LPBYTE) &bEoln;
        dwEolnSize = 1;
        break;
      case EOLN_CRLF:
        bEoln[0] = '\r';
        bEoln[1] = '\n';
        pbEoln = (LPBYTE) &bEoln;
        dwEolnSize = 2;
        break;
      default:
        return FALSE;
  }

  dwPos = 0;

  /* pszText eoln are always \r\n */

  do
  {
    /* Find the next eoln */
    dwNext = dwPos;
    while(dwNext < dwTextLen)
    {
      if (pszText[dwNext] == '\r' && pszText[dwNext + 1] == '\n')
        break;
      dwNext++;
    }

    /* Write text (without eoln) */
    if (!WriteEncodedText(hFile, &pszText[dwPos], dwNext - dwPos, iEncoding))
      return FALSE;

    /* Write eoln */
    if (dwNext != dwTextLen)
    {
      if (!WriteFile(hFile, pbEoln, dwEolnSize, &dwDummy, NULL))
        return FALSE;
    }

    /* Skip \r\n */
    dwPos = dwNext + 2;
  }
  while (dwPos < dwTextLen);

  return TRUE;
}

