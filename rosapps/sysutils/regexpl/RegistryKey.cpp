/* $Id: RegistryKey.cpp,v 1.2 2000/10/24 20:17:41 narnaoud Exp $
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

// RegistryKey.cpp: implementation of the CRegistryKey class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "RegistryKey.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRegistryKey::CRegistryKey(const TCHAR *pchKeyName, class CRegistryKey *pParent)
{
//	RegQueryInfoKeyW(m_hKey,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
//		&m_dwValueNameBufferSize,&m_dwMaxValueSize,&m_dwSecurityDescriptorSize,&m_ftLastWriteTime);
//	m_pchValueName = NULL;
	ASSERT(pParent != NULL);
	m_pParent = pParent;
	m_pChild = NULL;
	m_hKey = NULL;	// Open key with Open() member function
	ASSERT(pchKeyName != NULL);
	m_pchKeyName[MAX_PATH] = 0;
	*m_pszMachineName = 0;
	if (*pchKeyName == _T('\"'))
	{
		_tcsncpy(m_pchKeyName,pchKeyName+1,MAX_PATH);
		TCHAR *pch = _tcschr(m_pchKeyName,_T('\"'));
		if (pch)
		{
			*pch = 0;
		}
		else
		{
			ASSERT(FALSE);
		}
	}
	else
	{
		_tcsncpy(m_pchKeyName,pchKeyName,MAX_PATH);
	}
}

CRegistryKey::~CRegistryKey()
{
//	if (m_pchValueName) delete [] m_pchValueName;
	if((m_hKey != HKEY_CLASSES_ROOT)&&(m_hKey != HKEY_CURRENT_USER)&&
		(m_hKey != HKEY_LOCAL_MACHINE)&&(m_hKey != HKEY_USERS)
		&&(m_hKey != HKEY_PERFORMANCE_DATA)&&(m_hKey != HKEY_CURRENT_CONFIG)
		&&(m_hKey != HKEY_DYN_DATA)&&(m_hKey != NULL))
	{
		RegCloseKey(m_hKey);
	}
}

TCHAR * CRegistryKey::GetKeyName()
{
	return m_pchKeyName;
}

class CRegistryKey * CRegistryKey::GetChild()
{
	return m_pChild;
}

CRegistryKey * CRegistryKey::GetParent()
{
	return m_pParent;
}

CRegistryKey * CRegistryKey::UpOneLevel()
{
	CRegistryKey *pParent = m_pParent;
	ASSERT(m_pChild == NULL);
	if (pParent)
	{
		ASSERT(pParent->m_pChild == this);
		pParent->m_pChild = NULL;
	}
	delete this;
	return pParent;
}

void CRegistryKey::InitSubKeyEnumeration()
{
	m_dwCurrentSubKeyIndex = 0;
}

TCHAR * CRegistryKey::GetSubKeyName(DWORD& dwError)
{
	static TCHAR m_pchSubName[MAX_PATH+1];
	dwError = RegEnumKey(m_hKey,m_dwCurrentSubKeyIndex,m_pchSubName,MAX_PATH + 1);
	m_dwCurrentSubKeyIndex++;
	switch (dwError)
	{
	case ERROR_SUCCESS:
		return m_pchSubName;
	case ERROR_NO_MORE_ITEMS:
		return NULL;
	default:
		return NULL;
	}
}

void CRegistryKey::UpdateKeyNameCase()
{
	m_pParent->InitSubKeyEnumeration();
	TCHAR *pchSubKeyName;
	DWORD dwError;
	while ((pchSubKeyName = m_pParent->GetSubKeyName(dwError)) != NULL)
	{
		if (dwError != ERROR_SUCCESS)
		{
			return;
		}
		if (_tcsicmp(pchSubKeyName,m_pchKeyName) == 0)
		{
			_tcscpy(m_pchKeyName,pchSubKeyName);
			return;
		}
	}
}

void CRegistryKey::InitValueEnumeration()
{
	m_dwCurrentValueIndex = 0;
}


// On input dwValueNameSize is size in characters of buffer pointed by pchValueNameBuffer
// On output dwValueNameSize contains number of characters stored in buffer
DWORD CRegistryKey::GetNextValue(TCHAR *pchValueNameBuffer,DWORD& dwValueNameSize, 
							 DWORD *pdwType, LPBYTE lpValueDataBuffer, DWORD *pdwValueDataSize)
{
	DWORD dwRet = RegEnumValue(m_hKey,m_dwCurrentValueIndex,pchValueNameBuffer,&dwValueNameSize,NULL,
		pdwType,lpValueDataBuffer,pdwValueDataSize);
	m_dwCurrentValueIndex++;
	return dwRet;
}

void CRegistryKey::GetLastWriteTime(SYSTEMTIME &st)
{
	FILETIME ftLocal,ft;
	RegQueryInfoKeyW(m_hKey,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		&ft);
	FileTimeToLocalFileTime(&ft,&ftLocal);
	FileTimeToSystemTime(&ftLocal,&st);
}

TCHAR * CRegistryKey::GetLastWriteTime()
{
	SYSTEMTIME st;
	GetLastWriteTime(st);
	static TCHAR Buffer[256];
	_stprintf(Buffer,_T("%d.%d.%d %02d:%02d:%02d"),st.wDay,st.wMonth,st.wYear,st.wHour,st.wMinute,st.wSecond);
	return Buffer;
}

// Returns ErrorCode (ERROR_SUCCESS on success)
// dwMaxValueDataBuferSize receives the length, in bytes, 
// of the longest data component among the key's values.
DWORD CRegistryKey::GetMaxValueDataSize(DWORD& dwMaxValueDataBuferSize)
{
	return RegQueryInfoKeyW(m_hKey,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		NULL,&dwMaxValueDataBuferSize,NULL,NULL);
}

// Returns ErrorCode (ERROR_SUCCESS on success)
// dwMaxValueNameBuferSize receives the length, in characters, 
// of the key's longest value name.
// The count returned does not include the terminating null character.
DWORD CRegistryKey::GetMaxValueNameLength(DWORD& dwMaxValueNameBuferSize)
{
	return RegQueryInfoKeyW(m_hKey,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		&dwMaxValueNameBuferSize,NULL,NULL,NULL);
}

DWORD CRegistryKey::Open(REGSAM samDesired)
{
	if (IsPredefined())
		return ERROR_SUCCESS;

	DWORD dwRet;
	HKEY hKey = NULL;
	if (*m_pszMachineName)
	{
		ASSERT(ERROR_SUCCESS == 0);
		dwRet = RegConnectRegistry(m_pszMachineName,m_hKey,&m_hKey);
	}
	else
	{
		ASSERT((m_hKey != HKEY_CLASSES_ROOT)&&(m_hKey != HKEY_CURRENT_USER)&&
			(m_hKey != HKEY_LOCAL_MACHINE)&&(m_hKey != HKEY_USERS)
			&&(m_hKey != HKEY_PERFORMANCE_DATA)&&(m_hKey != HKEY_CURRENT_CONFIG)
			&&(m_hKey != HKEY_DYN_DATA));
		if (m_hKey)
		{
			RegCloseKey(m_hKey);
		}
		dwRet = RegOpenKeyEx(*m_pParent,m_pchKeyName,0,samDesired,&hKey);
	}
	if (dwRet == ERROR_SUCCESS)
	{
		m_hKey = hKey;
		UpdateKeyNameCase();
	}
	return dwRet;
}

CRegistryKey::CRegistryKey(HKEY hKey, LPCTSTR pszMachineName)
{
	TCHAR *pchKeyName = NULL;
	ASSERT((hKey == HKEY_CLASSES_ROOT)||(hKey == HKEY_CURRENT_USER)||
		(hKey == HKEY_LOCAL_MACHINE)||(hKey == HKEY_USERS)
		||(hKey == HKEY_PERFORMANCE_DATA)||(hKey == HKEY_CURRENT_CONFIG)
		||(hKey == HKEY_DYN_DATA));
	if(hKey == HKEY_CLASSES_ROOT)
	{
		pchKeyName = _T("HKEY_CLASSES_ROOT");
	}
	else if(hKey == HKEY_CURRENT_USER)
	{
		pchKeyName = _T("HKEY_CURRENT_USER");
	}
	else if (hKey == HKEY_LOCAL_MACHINE)
	{
		pchKeyName = _T("HKEY_LOCAL_MACHINE");
	}
	else if (hKey == HKEY_USERS)
	{
		pchKeyName = _T("HKEY_USERS");
	}
	else if (hKey == HKEY_PERFORMANCE_DATA)
	{
		pchKeyName = _T("HKEY_PERFORMANCE_DATA");
	}
	else if (hKey == HKEY_CURRENT_CONFIG)
	{
		pchKeyName = _T("HKEY_CURRENT_CONFIG");
	}
	else if (hKey == HKEY_DYN_DATA)
	{
		pchKeyName = _T("HKEY_DYN_DATA");
	}
	else
	{
		ASSERT(FALSE);
		return;
	}

	m_hKey = hKey;

	m_pParent = NULL;
	m_pChild = NULL;
	ASSERT(pchKeyName != NULL);
	m_pchKeyName[MAX_PATH] = 0;
	_tcsncpy(m_pchKeyName,pchKeyName,MAX_PATH);
	_tcsncpy(m_pszMachineName,pszMachineName?pszMachineName:_T(""),MAX_PATH);
}

BOOL CRegistryKey::IsPredefined()
{
	return ((m_hKey == HKEY_CLASSES_ROOT)||(m_hKey == HKEY_CURRENT_USER)||
		(m_hKey == HKEY_LOCAL_MACHINE)||(m_hKey == HKEY_USERS)
		||(m_hKey == HKEY_PERFORMANCE_DATA)||(m_hKey == HKEY_CURRENT_CONFIG)
		||(m_hKey == HKEY_DYN_DATA));
}

DWORD CRegistryKey::GetDefaultValue(DWORD *pdwType, LPBYTE lpValueDataBuffer, DWORD *pdwValueDataSize)
{
	return RegQueryValueEx(m_hKey,NULL,NULL,pdwType,lpValueDataBuffer,pdwValueDataSize);
}

void CRegistryKey::LinkParent()
{
	m_pParent->m_pChild = this;	// self link
}

const TCHAR * CRegistryKey::GetValueTypeName(DWORD dwType)
{
	switch(dwType)
	{
	case REG_NONE:
		return _T("REG_NONE\t\t");
	case REG_SZ:
		return _T("REG_SZ\t\t\t");
	case REG_EXPAND_SZ:
		return _T("REG_EXPAND_SZ\t\t");
	case REG_BINARY:
		return _T("REG_BINARY\t\t");
	case REG_DWORD_LITTLE_ENDIAN:
		return _T("REG_DWORD_LITTLE_ENDIAN\t");
	case REG_DWORD_BIG_ENDIAN:
		return _T("REG_DWORD_BIG_ENDIAN\t");
	case REG_LINK:
		return _T("REG_LINK\t\t");
	case REG_MULTI_SZ:
		return _T("REG_MULTI_SZ\t\t");
	case REG_RESOURCE_LIST:
		return _T("REG_RESOURCE_LIST\t");
	case REG_FULL_RESOURCE_DESCRIPTOR:
		return _T("REG_FULL_RESOURCE_DESCRIPTOR");
	case REG_RESOURCE_REQUIREMENTS_LIST:
		return _T("REG_RESOURCE_REQUIREMENTS_LIST");
	default:
		return _T("Unkown Type\t");
	}
}

DWORD CRegistryKey::GetValue(TCHAR *pchValueName, DWORD *pdwType, LPBYTE lpValueDataBuffer, DWORD *pdwValueDataSize)
{
	return RegQueryValueEx(m_hKey,pchValueName,NULL,pdwType,lpValueDataBuffer,pdwValueDataSize);
}

DWORD CRegistryKey::GetSecurityDescriptorLength(DWORD *pdwSecurityDescriptor)
{
	return RegQueryInfoKeyW(m_hKey,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		NULL,NULL,pdwSecurityDescriptor,NULL);
}

LONG CRegistryKey::GetSecurityDescriptor(SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor, LPDWORD lpcbSecurityDescriptor)
{
	return RegGetKeySecurity(m_hKey,SecurityInformation,pSecurityDescriptor,lpcbSecurityDescriptor);
}

DWORD CRegistryKey::GetSubKeyCount()
{
	DWORD nCount;
	DWORD nRet = RegQueryInfoKeyW(m_hKey,NULL,NULL,NULL,&nCount,NULL,NULL,NULL,
		NULL,NULL,NULL,NULL);
	if (nRet)
		return 0;
	return nCount;
}

DWORD CRegistryKey::GetValuesCount()
{
	DWORD nCount;
	if (RegQueryInfoKeyW(m_hKey,NULL,NULL,NULL,NULL,NULL,NULL,
		&nCount,NULL,NULL,NULL,NULL)) return 0;
	return nCount;
}

TCHAR * CRegistryKey::GetSubKeyNameByIndex(DWORD dwIndex)
{
	DWORD dwError;
	static TCHAR m_pchSubName[MAX_PATH+1];
	dwError = RegEnumKey(m_hKey,dwIndex,m_pchSubName,MAX_PATH + 1);
	switch (dwError)
	{
	case ERROR_SUCCESS:
		return m_pchSubName;
	case ERROR_NO_MORE_ITEMS:
		return NULL;
	default:
		return NULL;
	}
}

DWORD CRegistryKey::Create(REGSAM samDesired, DWORD *pdwDisposition, BOOL blnVolatile)
{
	ASSERT((m_hKey != HKEY_CLASSES_ROOT)&&(m_hKey != HKEY_CURRENT_USER)&&
		(m_hKey != HKEY_LOCAL_MACHINE)&&(m_hKey != HKEY_USERS)
		&&(m_hKey != HKEY_PERFORMANCE_DATA)&&(m_hKey != HKEY_CURRENT_CONFIG)
		&&(m_hKey != HKEY_DYN_DATA));
	if (m_hKey)
	{
		RegCloseKey(m_hKey);
	}

	HKEY hKey;

	DWORD dwRet = RegCreateKeyEx(*m_pParent,m_pchKeyName,0,NULL,
		blnVolatile?REG_OPTION_VOLATILE:REG_OPTION_NON_VOLATILE,
		samDesired,
		NULL,
		&hKey,
		pdwDisposition);
	if (dwRet == ERROR_SUCCESS)
	{
		m_hKey = hKey;
		UpdateKeyNameCase();
	}
	return dwRet;
}

DWORD CRegistryKey::DeleteSubkey(LPCTSTR pszSubKey, BOOL blnRecursive)
{
	CRegistryKey *pKey = new CRegistryKey(pszSubKey,this);
	if (!pKey)
		return ERROR_NOT_ENOUGH_MEMORY;

	DWORD dwRet = pKey->Open(KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE);

	if (!dwRet)
		dwRet = pKey->Delete(blnRecursive);

	delete pKey;

	return dwRet;
}

DWORD CRegistryKey::Delete(BOOL blnRecursive)
{
	DWORD dwRet;
	if (blnRecursive)
	{
		// Delete childs
		while(GetSubKeyCount())
		{
			TCHAR *pchKeyName = GetSubKeyNameByIndex(0);
			CRegistryKey *pKey = new CRegistryKey(pchKeyName,this);
			if (!pKey)
				return ERROR_NOT_ENOUGH_MEMORY;

			dwRet = pKey->Open(KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE);

			if (!dwRet)
				dwRet = pKey->Delete(blnRecursive);

			delete pKey;
		}
	}

	// Delete yourself
	return RegDeleteKey(m_pParent->m_hKey,m_pchKeyName);
}

DWORD CRegistryKey::SetValue(LPCTSTR pszValueName, DWORD dwType, BYTE *lpData, DWORD dwDataSize)
{
	return RegSetValueEx(m_hKey,pszValueName,0,dwType,lpData,dwDataSize);
}

DWORD CRegistryKey::DeleteValue(LPCTSTR pszValueName)
{
	return RegDeleteValue(m_hKey,pszValueName);
}
