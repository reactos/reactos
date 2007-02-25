/*
 *  Notepad (settings.c)
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

static const TCHAR s_szRegistryKey[] = { 'S','o','f','t','w','a','r','e',
	'\\','M','i','c','r','o','s','o','f','t',
	'\\','N','o','t','e','p','a','d',0 };


static LONG HeightFromPointSize(DWORD dwPointSize)
{
	LONG lHeight;
	HDC hDC;

	hDC = GetDC(NULL);
	lHeight = -MulDiv(dwPointSize, GetDeviceCaps(hDC, LOGPIXELSY), 720);
	ReleaseDC(NULL, hDC);

	return lHeight;
}

static DWORD PointSizeFromHeight(LONG lHeight)
{
	DWORD dwPointSize;
	HDC hDC;

	hDC = GetDC(NULL);
	dwPointSize = -MulDiv(lHeight, 720, GetDeviceCaps(hDC, LOGPIXELSY));
	ReleaseDC(NULL, hDC);

	/* round to nearest multiple of 10 */
	dwPointSize += 5;
	dwPointSize -= dwPointSize % 10;

	return dwPointSize;
}

static BOOL QueryGeneric(HKEY hKey, LPCSTR pszValueName, DWORD dwExpectedType,
	LPVOID pvResult, DWORD dwResultSize)
{
	WCHAR szValueW[32];
	LPCTSTR pszValueNameT;
	DWORD dwType, cbData;
	LPVOID *pTemp;
	BOOL bSuccess = FALSE;

#ifdef UNICODE
	MultiByteToWideChar(CP_ACP, 0, pszValueName, -1, szValueW, sizeof(szValueW) / sizeof(szValueW[0]));
	pszValueNameT = szValueW;
#else
	pszValueNameT = pszValueName;
#endif

	pTemp = HeapAlloc(GetProcessHeap(), 0, dwResultSize);
	if (!pTemp)
		goto done;
	memset(pTemp, 0, dwResultSize);

	cbData = dwResultSize;
	if (RegQueryValueEx(hKey, pszValueNameT, NULL, &dwType, (LPBYTE) pTemp, &cbData) != ERROR_SUCCESS)
		goto done;

	if (dwType != dwExpectedType)
		goto done;

	memcpy(pvResult, pTemp, cbData);
	bSuccess = TRUE;

done:
	if (pTemp)
		HeapFree(GetProcessHeap(), 0, pTemp);
	return bSuccess;
}

static BOOL QueryDword(HKEY hKey, LPCSTR pszValueName, DWORD *pdwResult)
{
	return QueryGeneric(hKey, pszValueName, REG_DWORD, pdwResult, sizeof(*pdwResult));
}

static BOOL QueryByte(HKEY hKey, LPCSTR pszValueName, BYTE *pbResult)
{
	DWORD dwResult;
	if (!QueryGeneric(hKey, pszValueName, REG_DWORD, &dwResult, sizeof(dwResult)))
		return FALSE;
	if (dwResult >= 0x100)
		return FALSE;
	*pbResult = (BYTE) dwResult;
	return TRUE;
}

static BOOL QueryBool(HKEY hKey, LPCSTR pszValueName, BOOL *pbResult)
{
	DWORD dwResult;
	if (!QueryDword(hKey, pszValueName, &dwResult))
		return FALSE;
	*pbResult = dwResult ? TRUE : FALSE;
	return TRUE;
}

static BOOL QueryString(HKEY hKey, LPCSTR pszValueName, LPTSTR pszResult, DWORD dwResultSize)
{
	return QueryGeneric(hKey, pszValueName, REG_SZ, pszResult, dwResultSize * sizeof(*pszResult));
}

void LoadSettings(void)
{
	HKEY hKey = NULL;
	HFONT hFont;
	DWORD dwPointSize = 0;

	if (RegOpenKey(HKEY_CURRENT_USER, s_szRegistryKey, &hKey) == ERROR_SUCCESS)
	{
		QueryByte(hKey,		"lfCharSet",		&Globals.lfFont.lfCharSet);
		QueryByte(hKey,		"lfClipPrecision",	&Globals.lfFont.lfClipPrecision);
		QueryDword(hKey,	"lfEscapement",		(DWORD*)&Globals.lfFont.lfEscapement);
		QueryString(hKey,	"lfFaceName",		Globals.lfFont.lfFaceName, sizeof(Globals.lfFont.lfFaceName) / sizeof(Globals.lfFont.lfFaceName[0]));
		QueryByte(hKey,		"lfItalic",			&Globals.lfFont.lfItalic);
		QueryDword(hKey,	"lfOrientation",	(DWORD*)&Globals.lfFont.lfOrientation);
		QueryByte(hKey,		"lfOutPrecision",	&Globals.lfFont.lfOutPrecision);
		QueryByte(hKey,		"lfPitchAndFamily",	&Globals.lfFont.lfPitchAndFamily);
		QueryByte(hKey,		"lfQuality",		&Globals.lfFont.lfQuality);
		QueryByte(hKey,		"lfStrikeOut",		&Globals.lfFont.lfStrikeOut);
		QueryByte(hKey,		"lfUnderline",		&Globals.lfFont.lfUnderline);
		QueryDword(hKey,	"lfWeight",			(DWORD*)&Globals.lfFont.lfWeight);
		QueryDword(hKey,	"iPointSize",		&dwPointSize);
		QueryBool(hKey,     "fWrap",            &Globals.bWrapLongLines);

		if (dwPointSize != 0)
			Globals.lfFont.lfHeight = HeightFromPointSize(dwPointSize);

		hFont = CreateFontIndirect(&Globals.lfFont);
		if (hFont)
		{
			if (Globals.hFont)
				DeleteObject(Globals.hFont);
			Globals.hFont = hFont;
		}

		RegCloseKey(hKey);
	}
}

static BOOL SaveDword(HKEY hKey, LPCSTR pszValueName, DWORD dwValue)
{
	WCHAR szValueW[32];
	LPCTSTR pszValueNameT;

#ifdef UNICODE
	MultiByteToWideChar(CP_ACP, 0, pszValueName, -1, szValueW, sizeof(szValueW) / sizeof(szValueW[0]));
	pszValueNameT = szValueW;
#else
	pszValueNameT = pszValueName;
#endif

	return RegSetValueEx(hKey, pszValueNameT, 0, REG_DWORD, (LPBYTE) &dwValue, sizeof(dwValue)) == ERROR_SUCCESS;
}

static BOOL SaveString(HKEY hKey, LPCSTR pszValueName, LPCTSTR pszValue)
{
	WCHAR szValueW[32];
	LPCTSTR pszValueNameT;

#ifdef UNICODE
	MultiByteToWideChar(CP_ACP, 0, pszValueName, -1, szValueW, sizeof(szValueW) / sizeof(szValueW[0]));
	pszValueNameT = szValueW;
#else
	pszValueNameT = pszValueName;
#endif

	return RegSetValueEx(hKey, pszValueNameT, 0, REG_SZ, (LPBYTE) pszValue, (DWORD) _tcslen(pszValue) * sizeof(*pszValue)) == ERROR_SUCCESS;
}

void SaveSettings(void)
{
	HKEY hKey;
	DWORD dwDisposition;

	if (RegCreateKeyEx(HKEY_CURRENT_USER, s_szRegistryKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition)
		== ERROR_SUCCESS)
	{
		SaveDword(hKey,		"lfCharSet",		Globals.lfFont.lfCharSet);
		SaveDword(hKey,		"lfClipPrecision",	Globals.lfFont.lfClipPrecision);
		SaveDword(hKey,		"lfEscapement",		Globals.lfFont.lfEscapement);
		SaveString(hKey,	"lfFaceName",		Globals.lfFont.lfFaceName);
		SaveDword(hKey,		"lfItalic",			Globals.lfFont.lfItalic);
		SaveDword(hKey,		"lfOrientation",	Globals.lfFont.lfOrientation);
		SaveDword(hKey,		"lfOutPrecision",	Globals.lfFont.lfOutPrecision);
		SaveDword(hKey,		"lfPitchAndFamily",	Globals.lfFont.lfPitchAndFamily);
		SaveDword(hKey,		"lfQuality",		Globals.lfFont.lfQuality);
		SaveDword(hKey,		"lfStrikeOut",		Globals.lfFont.lfStrikeOut);
		SaveDword(hKey,		"lfUnderline",		Globals.lfFont.lfUnderline);
		SaveDword(hKey,		"lfWeight",			Globals.lfFont.lfWeight);
		SaveDword(hKey,		"iPointSize",		PointSizeFromHeight(Globals.lfFont.lfHeight));
		SaveDword(hKey,		"fWrap",			Globals.bWrapLongLines ? 1 : 0);

		RegCloseKey(hKey);
	}

}
