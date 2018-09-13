//+------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       regkey.cxx
//
//  Contents:   implementation of non-inline CRegKey functions
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_REGKEY_HXX_
#define X_REGKEY_HXX_
#include "regkey.hxx"
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegKey

LONG CRegKey::Close()
{
	LONG lRes = ERROR_SUCCESS;
	if (m_hKey != NULL)
	{
		lRes = RegCloseKey(m_hKey);
		m_hKey = NULL;
	}
	return lRes;
}

LONG CRegKey::Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
	LPTSTR lpszClass, DWORD dwOptions, REGSAM samDesired,
	LPSECURITY_ATTRIBUTES lpSecAttr, LPDWORD lpdwDisposition)
{
	_ASSERTE(hKeyParent != NULL);
	DWORD dw;
	HKEY hKey = NULL;
	LONG lRes = RegCreateKeyEx(hKeyParent, lpszKeyName, 0,
		lpszClass, dwOptions, samDesired, lpSecAttr, &hKey, &dw);
	if (lpdwDisposition != NULL)
		*lpdwDisposition = dw;
	if (lRes == ERROR_SUCCESS)
	{
		lRes = Close();
		m_hKey = hKey;
	}
	return lRes;
}

LONG CRegKey::Open(HKEY hKeyParent, LPCTSTR lpszKeyName, REGSAM samDesired)
{
	_ASSERTE(hKeyParent != NULL);
	HKEY hKey = NULL;
	LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, samDesired, &hKey);
	if (lRes == ERROR_SUCCESS)
	{
		lRes = Close();
		_ASSERTE(lRes == ERROR_SUCCESS);
		m_hKey = hKey;
	}
	return lRes;
}

LONG CRegKey::QueryValue(DWORD& dwValue, LPCTSTR lpszValueName)
{
	DWORD dwType = NULL;
	DWORD dwCount = sizeof(DWORD);
	LONG lRes = RegQueryValueEx(m_hKey, (LPTSTR)lpszValueName, NULL, &dwType,
		(LPBYTE)&dwValue, &dwCount);
	_ASSERTE((lRes!=ERROR_SUCCESS) || (dwType == REG_DWORD));
	_ASSERTE((lRes!=ERROR_SUCCESS) || (dwCount == sizeof(DWORD)));
	return lRes;
}

LONG CRegKey::QueryValue(LPTSTR szValue, LPCTSTR lpszValueName, DWORD* 
pdwCount)
{
	_ASSERTE(pdwCount != NULL);
	DWORD dwType = NULL;
	LONG lRes = RegQueryValueEx(m_hKey, (LPTSTR)lpszValueName, NULL, &dwType,
		(LPBYTE)szValue, pdwCount);
	_ASSERTE((lRes!=ERROR_SUCCESS) || (dwType == REG_SZ) ||
			 (dwType == REG_MULTI_SZ) || (dwType == REG_EXPAND_SZ));
	return lRes;
}

LONG WINAPI CRegKey::SetValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR 
lpszValue, LPCTSTR lpszValueName)
{
	_ASSERTE(lpszValue != NULL);
	CRegKey key;
	LONG lRes = key.Create(hKeyParent, lpszKeyName);
	if (lRes == ERROR_SUCCESS)
		lRes = key.SetValue(lpszValue, lpszValueName);
	return lRes;
}

LONG CRegKey::SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR 
lpszValueName)
{
	_ASSERTE(lpszValue != NULL);
	CRegKey key;
	LONG lRes = key.Create(m_hKey, lpszKeyName);
	if (lRes == ERROR_SUCCESS)
		lRes = key.SetValue(lpszValue, lpszValueName);
	return lRes;
}

LONG CRegKey::SetValue(DWORD dwValue, LPCTSTR lpszValueName)
{
	_ASSERTE(m_hKey != NULL);
	return RegSetValueEx(m_hKey, lpszValueName, NULL, REG_DWORD,
		(BYTE * const)&dwValue, sizeof(DWORD));
}

HRESULT CRegKey::SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
	_ASSERTE(lpszValue != NULL);
	_ASSERTE(m_hKey != NULL);
	return RegSetValueEx(m_hKey, lpszValueName, NULL, REG_SZ,
		(BYTE * const)lpszValue, (lstrlen(lpszValue)+1)*sizeof(TCHAR));
}

//RecurseDeleteKey is necessary because on NT RegDeleteKey doesn't work if the
//specified key has subkeys
LONG CRegKey::RecurseDeleteKey(LPCTSTR lpszKey)
{
	CRegKey key;
	LONG lRes = key.Open(m_hKey, lpszKey);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	FILETIME time;
	TCHAR szBuffer[256];
	DWORD dwSize = 256;
	while (RegEnumKeyEx(key.m_hKey, 0, szBuffer, &dwSize, NULL, NULL, NULL,
		&time)==ERROR_SUCCESS)
	{
		lRes = key.RecurseDeleteKey(szBuffer);
		if (lRes != ERROR_SUCCESS)
			return lRes;
		dwSize = 256;
	}
	key.Close();
	return DeleteSubKey(lpszKey);
}

