/* $Id: RegistryKey.cpp,v 1.3 2001/01/10 01:25:29 narnaoud Exp $
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

static TCHAR *g_ppszHiveNames[] =
{
  _T("HKEY_CLASSES_ROOT"),
  _T("HKEY_CURRENT_USER"),
  _T("HKEY_LOCAL_MACHINE"),
  _T("HKEY_USERS"),
  _T("HKEY_PERFORMANCE_DATA"),
  _T("HKEY_CURRENT_CONFIG"),
  _T("HKEY_DYN_DATA")
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRegistryKey::CRegistryKey()
{
  m_pszKeyName = NULL;
  m_pszMachineName = NULL;
	m_CurrentAccess = 0;
	m_hKey = NULL;
}

HRESULT CRegistryKey::InitRoot(const TCHAR *pszMachineName = NULL)
{
  if ((pszMachineName)&&
      ((_tcslen(pszMachineName) < 3)||
       (pszMachineName[0] != _T('\\'))||
       (pszMachineName[1] != _T('\\'))
      )
    )
  {
    return E_INVALIDARG;
  }
    
  HRESULT hr = Uninit();
  if (FAILED(hr))
    return hr;

  if (pszMachineName)
  { // copy machine name
    size_t size = _tcslen(pszMachineName);

    m_pszMachineName = new TCHAR [size+2];
    if (!m_pszMachineName)
      return E_OUTOFMEMORY;
    _tcscpy(m_pszMachineName,pszMachineName);
    m_pszMachineName[size] = _T('\\');
    m_pszMachineName[size+1] = 0;
  }
  else
  {
    m_pszMachineName = NULL; // local registry
  }
  
  ASSERT(m_pszKeyName == NULL);
	m_CurrentAccess = 0;
	ASSERT(m_hKey == NULL);

  return S_OK;
}

HRESULT CRegistryKey::Init(HKEY hKey, const TCHAR *pszPath, const TCHAR *pszKeyName, REGSAM CurrentAccess)
{
  HRESULT hr = Uninit();
  if (FAILED(hr))
    return hr;

  if (!pszKeyName || !hKey)
    return E_INVALIDARG;
  
  // copy key name name
  size_t size = _tcslen(pszKeyName);
  if (pszPath)
    size += _tcslen(pszPath);
  
  m_pszKeyName = new TCHAR [size+2];
  if (!m_pszKeyName)
    return E_OUTOFMEMORY;
  _stprintf(m_pszKeyName,_T("%s%s\\"),pszPath?pszPath:_T(""),pszKeyName);

	m_CurrentAccess = CurrentAccess;
	m_hKey = hKey;
  ASSERT(m_hKey);

  return S_OK;
}

HRESULT CRegistryKey::Uninit()
{
	if (m_pszKeyName)
  {
    delete [] m_pszKeyName;
    m_pszKeyName = NULL;
  }

	if (m_pszMachineName)
  {
    delete [] m_pszMachineName;
    m_pszMachineName = NULL;
  }

  LONG nError = ERROR_SUCCESS;
	if((m_hKey != NULL)&&(!IsHive(m_hKey)))
		nError = RegCloseKey(m_hKey);
     
  m_hKey = NULL;
  
  return (nError == ERROR_SUCCESS)?S_OK:E_FAIL;
}

BOOL CRegistryKey::IsHive(HKEY hKey)
{
	return ((hKey == HKEY_CLASSES_ROOT)||
          (hKey == HKEY_CURRENT_USER)||
          (hKey == HKEY_LOCAL_MACHINE)||
          (hKey == HKEY_USERS)||
          (hKey == HKEY_PERFORMANCE_DATA)||
          (hKey == HKEY_CURRENT_CONFIG)||
          (hKey == HKEY_DYN_DATA));
}

CRegistryKey::~CRegistryKey()
{
  Uninit();
}

const TCHAR * CRegistryKey::GetKeyName()
{
	return m_pszKeyName?m_pszKeyName:(m_pszMachineName?m_pszMachineName:_T("\\"));
}

BOOL CRegistryKey::IsRoot()
{
  return m_hKey == NULL;
}

LONG CRegistryKey::OpenSubkey(REGSAM samDesired, const TCHAR *pszSubkeyName, HKEY &rhKey)
{
	if (m_hKey == NULL)
  { // subkey of root key is hive root key.
    if ((_tcsicmp(pszSubkeyName,_T("HKCR")) == 0)||
				(_tcsicmp(pszSubkeyName,_T("HKEY_CLASSES_ROOT")) == 0))
    {
      rhKey = HKEY_CLASSES_ROOT;
      
      if (m_pszMachineName)
        return ERROR_FILE_NOT_FOUND;
            
      return ERROR_SUCCESS;
    }
    else if ((_tcsicmp(pszSubkeyName,_T("HKCU")) == 0)||
             (_tcsicmp(pszSubkeyName,_T("HKEY_CURRENT_USER")) == 0))
    {
      rhKey = HKEY_CURRENT_USER;
      
      if (m_pszMachineName)
        return ERROR_FILE_NOT_FOUND;
            
      return ERROR_SUCCESS;
    }
    else if ((_tcsicmp(pszSubkeyName,_T("HKLM")) == 0)||
             (_tcsicmp(pszSubkeyName,_T("HKEY_LOCAL_MACHINE")) == 0))
    {
      rhKey = HKEY_LOCAL_MACHINE;
      
      if (m_pszMachineName)
        return RegConnectRegistry(m_pszMachineName,rhKey,&rhKey);
      
      return ERROR_SUCCESS;
    }
    else if ((_tcsicmp(pszSubkeyName,_T("HKU")) == 0)||
             (_tcsicmp(pszSubkeyName,_T("HKEY_USERS")) == 0))
    {
      rhKey = HKEY_USERS;
      
      if (m_pszMachineName)
        return RegConnectRegistry(m_pszMachineName,rhKey,&rhKey);
      
      return ERROR_SUCCESS;
    }
    else if ((_tcsicmp(pszSubkeyName,_T("HKPD")) == 0)||
             (_tcsicmp(pszSubkeyName,_T("HKEY_PERFORMANCE_DATA")) == 0))
    {
      rhKey = HKEY_PERFORMANCE_DATA;
      
      if (m_pszMachineName)
        return RegConnectRegistry(m_pszMachineName,rhKey,&rhKey);
      
      return ERROR_SUCCESS;
    }
    else if ((_tcsicmp(pszSubkeyName,_T("HKDD")) == 0)||
             (_tcsicmp(pszSubkeyName,_T("HKEY_DYN_DATA")) == 0))
    {
      rhKey = HKEY_DYN_DATA;
      
      if (m_pszMachineName)
        return RegConnectRegistry(m_pszMachineName,rhKey,&rhKey);
      
      return ERROR_SUCCESS;
    }
    else if ((_tcsicmp(pszSubkeyName,_T("HKCC")) == 0)||
             (_tcsicmp(pszSubkeyName,_T("HKEY_CURRENT_CONFIG")) == 0))
    {
      rhKey = HKEY_CURRENT_CONFIG;
      
      if (m_pszMachineName)
      {
        TCHAR *pch = m_pszMachineName;
        while (*pch)
          pch++;
        pch--;
        
        ASSERT(*pch == _T('\\'));
        if (*pch != _T('\\'))
          return ERROR_INTERNAL_ERROR;

        *pch = 0;
        
        LONG nError = RegConnectRegistry(m_pszMachineName,rhKey,&rhKey);

        *pch = _T('\\');
        
        return nError;
      }
      
      return ERROR_SUCCESS;
    }
    else
    {
      return ERROR_FILE_NOT_FOUND;
    }
  }
  
	return RegOpenKeyEx(m_hKey,pszSubkeyName,0,samDesired,&rhKey);
}

LONG CRegistryKey::OpenSubkey(REGSAM samDesired, const TCHAR *pszSubkeyName, CRegistryKey &rKey)
{
  HKEY hKey;
  LONG nError = OpenSubkey(samDesired, pszSubkeyName, hKey);
  
  if (nError == ERROR_SUCCESS)
  {
    const TCHAR *pszKeyName = GetKeyName();
    size_t size = _tcslen(pszKeyName) + _tcslen(pszSubkeyName) + 1;
    TCHAR *pszSubkeyFullName = new TCHAR [size];
    if (!pszSubkeyFullName)
    {
      nError = RegCloseKey(hKey);
      ASSERT(nError == ERROR_SUCCESS);
      return ERROR_OUTOFMEMORY;
    }
    _tcscpy(pszSubkeyFullName,pszKeyName);
    _tcscat(pszSubkeyFullName,pszSubkeyName);
    HRESULT hr = rKey.Init(hKey,GetKeyName(),pszSubkeyName,samDesired);
    delete pszSubkeyName;
    if (FAILED(hr))
    {
      nError = RegCloseKey(hKey);
      ASSERT(nError == ERROR_SUCCESS);
      if (hr == (HRESULT)E_OUTOFMEMORY)
        return ERROR_OUTOFMEMORY;
      else
        return ERROR_INTERNAL_ERROR;
    }
  }

  return nError;
}

LONG CRegistryKey::GetSubkeyNameMaxLength(DWORD &rdwMaxSubkeyNameLength)
{
  if (m_hKey == NULL)
  { // root key
    rdwMaxSubkeyNameLength = 0;
    for(int i = 0; i < 7 ; i++)
    {
      size_t l = _tcslen(g_ppszHiveNames[i]);
      if (rdwMaxSubkeyNameLength < l)
        rdwMaxSubkeyNameLength = l;
    }
    
    rdwMaxSubkeyNameLength++; // terminating null
    
    return ERROR_SUCCESS;
  }
  
  LONG nRet;

  nRet = RegQueryInfoKey(m_hKey,NULL,NULL,NULL,NULL,&rdwMaxSubkeyNameLength,NULL,NULL,NULL,NULL,NULL,NULL);

  rdwMaxSubkeyNameLength = (nRet == ERROR_SUCCESS)?(rdwMaxSubkeyNameLength+1):0;
  
  return nRet;
}

void CRegistryKey::InitSubkeyEnumeration(TCHAR *pszSubkeyNameBuffer, DWORD dwBufferSize)
{
  m_pchSubkeyNameBuffer = pszSubkeyNameBuffer;
  m_dwSubkeyNameBufferSize = dwBufferSize;
  m_dwCurrentSubKeyIndex = 0;
}

LONG CRegistryKey::GetNextSubkeyName(DWORD *pdwActualSize)
{
  LONG nError;
  
  if (m_hKey == NULL)
  {
    if (m_dwCurrentSubKeyIndex < (DWORD)(m_pszMachineName?5:7))
    {
      DWORD dwIndex = m_pszMachineName?m_dwCurrentSubKeyIndex+2:m_dwCurrentSubKeyIndex;
      _tcsncpy(m_pchSubkeyNameBuffer,g_ppszHiveNames[dwIndex],m_dwSubkeyNameBufferSize);
      nError = ERROR_SUCCESS;
      if (pdwActualSize)
        *pdwActualSize = strlen(m_pchSubkeyNameBuffer);
    }
    else
    {
      nError = ERROR_NO_MORE_ITEMS;
    }
  }
  else
  {
    DWORD dwActualSize = m_dwSubkeyNameBufferSize;
    FILETIME ft;
    nError = RegEnumKeyEx(m_hKey,
                          m_dwCurrentSubKeyIndex,
                          m_pchSubkeyNameBuffer,
                          &dwActualSize,
                          NULL,
                          NULL,
                          NULL,
                          &ft);
    if (pdwActualSize)
      *pdwActualSize = dwActualSize;
  }
  
	m_dwCurrentSubKeyIndex++;

  if (pdwActualSize)
    *pdwActualSize = strlen(m_pchSubkeyNameBuffer);
  return nError;
}

LONG CRegistryKey::GetSubkeyCount(DWORD &rdwSubkeyCount)
{
	return RegQueryInfoKeyW(m_hKey,NULL,NULL,NULL,&rdwSubkeyCount,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
}

LONG CRegistryKey::GetMaxValueDataSize(DWORD& rdwMaxValueDataBuferSize)
{
	return RegQueryInfoKeyW(m_hKey,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,&rdwMaxValueDataBuferSize,NULL,NULL);
}

LONG CRegistryKey::GetMaxValueNameLength(DWORD& rdwMaxValueNameBuferSize)
{
  if (!m_hKey)
    return 0; // the root key abstraction has only subkeys (hives)
  
	LONG nError = RegQueryInfoKeyW(m_hKey,NULL,NULL,NULL,NULL,NULL,NULL,NULL,&rdwMaxValueNameBuferSize,NULL,NULL,NULL);

  rdwMaxValueNameBuferSize++;
  return nError;
}

void CRegistryKey::InitValueEnumeration(TCHAR *pszValueNameBuffer,
                                        DWORD dwValueNameBufferSize,
                                        BYTE *pbValueDataBuffer,
                                        DWORD dwValueDataBufferSize,
                                        DWORD *pdwType)
{
  m_pszValueNameBuffer = pszValueNameBuffer;
  m_dwValueNameBufferSize = dwValueNameBufferSize;
  m_pbValueDataBuffer = pbValueDataBuffer;
  m_dwValueDataBufferSize = dwValueDataBufferSize;
  m_pdwType = pdwType;
  
	m_dwCurrentValueIndex = 0;
}


// On input dwValueNameSize is size in characters of buffer pointed by pchValueNameBuffer
// On output dwValueNameSize contains number of characters stored in buffer
LONG CRegistryKey::GetNextValue(DWORD *pdwNameActualSize, DWORD *pdwDataActualSize)
{
  if (!m_hKey)
    return ERROR_NO_MORE_ITEMS; // the root key abstraction has only subkeys (hives)
  
	DWORD dwValueNameBufferSize = m_dwValueNameBufferSize;
	DWORD dwValueDataBufferSize = m_dwValueDataBufferSize;
  LONG nError = RegEnumValue(m_hKey,
                            m_dwCurrentValueIndex,
                            m_pszValueNameBuffer,
                            &dwValueNameBufferSize,
                            NULL,
                            m_pdwType,
                            m_pbValueDataBuffer,
                            &dwValueDataBufferSize);

  if (pdwNameActualSize)
    *pdwNameActualSize = dwValueNameBufferSize;
  
  if (pdwDataActualSize)
    *pdwDataActualSize = dwValueDataBufferSize;
  
	m_dwCurrentValueIndex++;
	return nError;
}

LONG CRegistryKey::GetValueCount(DWORD& rdwValueCount)
{
  if (!m_hKey)
    return 0; // the root key abstraction has only subkeys (hives)
  
  return RegQueryInfoKeyW(m_hKey,NULL,NULL,NULL,NULL,NULL,NULL,&rdwValueCount,NULL,NULL,NULL,NULL);
}

LONG CRegistryKey::GetDefaultValue(DWORD *pdwType,
                                   BYTE *pbValueDataBuffer,
                                   DWORD dwValueDataBufferSize,
                                   DWORD *pdwValueDataActualSize)
{
  DWORD dwBufferSize = dwValueDataBufferSize;
  
	LONG nError = RegQueryValueEx(m_hKey,NULL,NULL,pdwType,pbValueDataBuffer,&dwBufferSize);

  if (pdwValueDataActualSize && (nError == ERROR_SUCCESS))
    *pdwValueDataActualSize = dwBufferSize;

  return nError;
}

const TCHAR * CRegistryKey::GetValueTypeName(DWORD dwType)
{
	switch(dwType)
	{
	case REG_NONE:
		return _T("REG_NONE");
	case REG_SZ:
		return _T("REG_SZ");
	case REG_EXPAND_SZ:
		return _T("REG_EXPAND_SZ");
	case REG_BINARY:
		return _T("REG_BINARY");
	case REG_DWORD_LITTLE_ENDIAN:
		return _T("REG_DWORD_LITTLE_ENDIAN");
	case REG_DWORD_BIG_ENDIAN:
		return _T("REG_DWORD_BIG_ENDIAN");
	case REG_LINK:
		return _T("REG_LINK");
	case REG_MULTI_SZ:
		return _T("REG_MULTI_SZ");
	case REG_RESOURCE_LIST:
		return _T("REG_RESOURCE_LIST");
	case REG_FULL_RESOURCE_DESCRIPTOR:
		return _T("REG_FULL_RESOURCE_DESCRIPTOR");
	case REG_RESOURCE_REQUIREMENTS_LIST:
		return _T("REG_RESOURCE_REQUIREMENTS_LIST");
	default:
		return _T("Unkown Type");
	}
}

DWORD CRegistryKey::GetValue(TCHAR *pchValueName, DWORD *pdwType, LPBYTE lpValueDataBuffer, DWORD *pdwValueDataSize)
{
	return RegQueryValueEx(m_hKey,pchValueName,NULL,pdwType,lpValueDataBuffer,pdwValueDataSize);
}

LONG CRegistryKey::CreateSubkey(REGSAM samDesired,
                                const TCHAR *pszSubkeyName,
                                HKEY &rhKey,
                                BOOL *pblnOpened,
                                BOOL blnVolatile)
{
  DWORD dwDisposition;
  
	LONG nError = RegCreateKeyEx(
      m_hKey,
      pszSubkeyName,
      0,
      NULL,
      blnVolatile?REG_OPTION_VOLATILE:REG_OPTION_NON_VOLATILE,
      samDesired,
      NULL,
      &rhKey,
      &dwDisposition);

  if ((nError == ERROR_SUCCESS)&&(pblnOpened))
    *pblnOpened = dwDisposition == REG_OPENED_EXISTING_KEY;
  
	return nError;
}

LONG CRegistryKey::GetLastWriteTime(SYSTEMTIME &st)
{
	FILETIME ftLocal,ft;
	LONG nError = RegQueryInfoKeyW(m_hKey,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,&ft);

  if (nError == ERROR_SUCCESS)
  {
    FileTimeToLocalFileTime(&ft,&ftLocal);
    FileTimeToSystemTime(&ftLocal,&st);
  }

  return nError;
}

const TCHAR * CRegistryKey::GetLastWriteTime()
{
	SYSTEMTIME st;
	if (GetLastWriteTime(st) != ERROR_SUCCESS)
    return _T("(Cannot get time last write time)");
  
	static TCHAR Buffer[256];
	_stprintf(Buffer,_T("%d.%d.%d %02d:%02d:%02d"),st.wDay,st.wMonth,st.wYear,st.wHour,st.wMinute,st.wSecond);
	return Buffer;
}

LONG CRegistryKey::GetSecurityDescriptorLength(DWORD *pdwSecurityDescriptor)
{
	return RegQueryInfoKeyW(m_hKey,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		NULL,NULL,pdwSecurityDescriptor,NULL);
}

LONG CRegistryKey::GetSecurityDescriptor(SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor, LPDWORD lpcbSecurityDescriptor)
{
	return RegGetKeySecurity(m_hKey,SecurityInformation,pSecurityDescriptor,lpcbSecurityDescriptor);
}

LONG CRegistryKey::DeleteSubkey(const TCHAR *pszSubkeyName)
{
  return RegDeleteKey(m_hKey,pszSubkeyName);
}

LONG CRegistryKey::SetValue(LPCTSTR pszValueName, DWORD dwType, BYTE *lpData, DWORD dwDataSize)
{
	return RegSetValueEx(m_hKey,pszValueName,0,dwType,lpData,dwDataSize);
}

LONG CRegistryKey::DeleteValue(const TCHAR *pszValueName)
{
	return RegDeleteValue(m_hKey,pszValueName);
}
