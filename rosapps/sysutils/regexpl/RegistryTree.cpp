/* $Id: RegistryTree.cpp,v 1.6 2001/04/24 22:32:31 narnaoud Exp $
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

// RegistryTree.cpp: implementation of the CRegistryTree class.

#include "ph.h"
#include "RegistryTree.h"
#include "Pattern.h"
#include "RegistryExplorer.h"

CRegistryTree::CRegistryTree()
{
	m_pszMachineName = NULL;
  VERIFY(SUCCEEDED(m_Root.m_Key.InitRoot()));
  m_Root.m_pUp = NULL;
	m_pCurrentKey = &m_Root;
  ASSERT(m_pCurrentKey->m_Key.IsRoot());
	m_ErrorMsg[ERROR_MSG_BUFFER_SIZE] = 0;
}

CRegistryTree::CRegistryTree(const CRegistryTree& Tree)
{
	m_pszMachineName = NULL;
  VERIFY(SUCCEEDED(m_Root.m_Key.InitRoot()));
  m_Root.m_pUp = NULL;
	m_pCurrentKey = &m_Root;
  ASSERT(m_pCurrentKey->m_Key.IsRoot());

  const TCHAR *pszPath = Tree.GetCurrentPath();
  if ((pszPath[0] == _T('\\')) && (pszPath[1] == _T('\\')))
  { // path has machine name
    pszPath += 2;
    while (*pszPath && (*pszPath != _T('\\')))
      pszPath++;
    
    ASSERT(*pszPath == _T('\\')); // if path begins with \\ it must be followed by machine name
  }

  if (Tree.m_pszMachineName)
    SetMachineName(Tree.m_pszMachineName);
  
  VERIFY(ChangeCurrentKey(pszPath));
}

CRegistryTree::~CRegistryTree()
{
	if (m_pszMachineName)
		delete m_pszMachineName;

	CNode *pNode;
	while(m_pCurrentKey->m_pUp)
	{
		pNode = m_pCurrentKey;
		m_pCurrentKey = m_pCurrentKey->m_pUp;
		delete pNode;
	}

  // We are on root
  ASSERT(m_pCurrentKey->m_Key.IsRoot());
  ASSERT(m_pCurrentKey == &m_Root);
}

const TCHAR * CRegistryTree::GetCurrentPath() const
{
  return m_pCurrentKey->m_Key.GetKeyName();
}

BOOL CRegistryTree::IsCurrentRoot()
{
	return m_pCurrentKey->m_Key.IsRoot();
}

BOOL CRegistryTree::ChangeCurrentKey(const TCHAR *pszRelativePath)
{
  if (*pszRelativePath == _T('\\'))
    GotoRoot();  // This is full absolute path.

  // split path to key names.
	TCHAR *pszSeps = _T("\\");

  // Make buffer and copy relative path into it.
  TCHAR *pszBuffer = new TCHAR[_tcslen(pszRelativePath)+1];
  if (!pszBuffer)
  {
    SetError(ERROR_OUTOFMEMORY);
    return FALSE;
  }

	_tcscpy(pszBuffer,pszRelativePath);

  // We accept names in form "\"blablabla\\blab  labla\"\\"
  size_t size = _tcslen(pszBuffer);
  if (size)
  {
    if (pszBuffer[size-1] == _T('\\'))
      pszBuffer[--size] = 0;
      
    TCHAR *psz;
    if (*pszBuffer == _T('\"') && (psz = _tcschr(pszBuffer+1,_T('\"'))) && size_t(psz-pszBuffer) == size-1)
    {
      size--;
      pszBuffer[size] = 0;

      pszBuffer++;
    }
  }

	TCHAR *pszNewKey = _tcstok(pszBuffer,pszSeps);

	if ((!pszNewKey)&&((*pszRelativePath != _T('\\'))||(*(pszRelativePath+1) != 0)))
	{
		SetError(_T("Invalid key name"));
		goto Abort;
	};

  // change keys
	while (pszNewKey)
	{
    if (!InternalChangeCurrentKey(pszNewKey,KEY_READ))
      goto Abort;  // InternalChangeCurrentKey sets last error description

		// Get next key name
		pszNewKey = _tcstok(NULL,pszSeps);
	}
  
	return TRUE;

Abort:
  delete pszBuffer;
  return FALSE;
}

const TCHAR * CRegistryTree::GetLastErrorDescription()
{
	return m_ErrorMsg;
}

void CRegistryTree::GotoRoot()
{
  // Delete current tree
  CNode *pNode;
  while(m_pCurrentKey->m_pUp)
  {
    pNode = m_pCurrentKey;
    m_pCurrentKey = m_pCurrentKey->m_pUp;
    delete pNode;
  }

  // We are on root
  ASSERT(m_pCurrentKey->m_Key.IsRoot());
  ASSERT(m_pCurrentKey == &m_Root);
}

BOOL CRegistryTree::SetMachineName(LPCTSTR pszMachineName)
{
  GotoRoot();
  
  // If we are going to local machine...
  if (pszMachineName == NULL)
  {
    // Delete previous machine name buffer if allocated.
    if (m_pszMachineName)
      delete m_pszMachineName;
    
    m_pszMachineName = NULL;
    m_Root.m_Key.InitRoot();
    return TRUE;
  }

  // Skip leading backslashes if any.
  while ((*pszMachineName)&&(*pszMachineName == _T('\\')))
    pszMachineName++;

  ASSERT(*pszMachineName);      // No machine name.

  TCHAR *pszNewMachineName = new TCHAR[_tcslen(pszMachineName)+3]; // two leading backslashes + terminating null

  if (!pszMachineName)
  {
    SetError(ERROR_OUTOFMEMORY);
    return FALSE;
  }
    
  // Delete previous machine name buffer if allocated.
  if (m_pszMachineName)
    delete m_pszMachineName;
  
  m_pszMachineName = pszNewMachineName;
  
  _tcscpy(m_pszMachineName,_T("\\\\")); // leading backslashes
  _tcscpy(m_pszMachineName+2,pszMachineName); // machine name itself
  _tcsupr(m_pszMachineName+2);  // upercase it
  
  VERIFY(SUCCEEDED(m_Root.m_Key.InitRoot(m_pszMachineName)));
  return TRUE;
}

BOOL CRegistryTree::NewKey(const TCHAR *pszKeyName, const TCHAR *pszPath, BOOL blnVolatile)
{
  if (!m_pCurrentKey)
  {
    SetErrorCommandNAOnRoot(_T("Creating new key "));
    return FALSE;
  }

  CRegistryTree Tree(*this);
	if (!Tree.ChangeCurrentKey(pszPath))
  {
    SetError(Tree.GetLastErrorDescription());
    return FALSE;
  }
  
	BOOL blnOpened;
  HKEY hKey;
  
	LONG nError = Tree.m_pCurrentKey->m_Key.CreateSubkey(KEY_READ,
                                                       pszKeyName,
                                                       hKey,
                                                       &blnOpened,
                                                       blnVolatile);
  if (nError == ERROR_SUCCESS)
  {
    LONG nError = RegCloseKey(hKey);
    ASSERT(nError == ERROR_SUCCESS);
  }
  
	if ((nError == ERROR_SUCCESS) && blnOpened)
	{
		SetError(_T("A key \"%s\" already exists."),pszKeyName);
    return FALSE;
	}
  
	if (nError != ERROR_SUCCESS)
	{
		SetError(_T("Cannot create key : %s%s\nError %d (%s)\n"),
             GetCurrentPath(),pszKeyName,nError,GetErrorDescription(nError));
    
    return FALSE;
	}

	return TRUE;
}

BOOL CRegistryTree::DeleteSubkeys(const TCHAR *pszKeyPattern, const TCHAR *pszPath, BOOL blnRecursive)
{
  CRegistryKey Key;
  if (!GetKey(pszPath,KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS|DELETE,Key))
    return FALSE;
  
  return DeleteSubkeys(Key, pszKeyPattern, blnRecursive);
}

BOOL CRegistryTree::DeleteSubkeys(CRegistryKey& rKey, const TCHAR *pszKeyPattern, BOOL blnRecursive)
{
  LONG nError;

  // enumerate subkeys
  DWORD dwMaxSubkeyNameLength;
  nError = rKey.GetSubkeyNameMaxLength(dwMaxSubkeyNameLength);
  if (nError != ERROR_SUCCESS)
  {
    SetError(_T("Cannot delete subkeys(s) of key %s.\nRequesting info about key failed.\nError %d (%s)\n"),
             rKey.GetKeyName(),nError,GetErrorDescription(nError));
    return FALSE;
  }

  TCHAR *pszSubkeyName = new TCHAR [dwMaxSubkeyNameLength];
  rKey.InitSubkeyEnumeration(pszSubkeyName, dwMaxSubkeyNameLength);
  BOOL blnKeyDeleted = FALSE;
  while ((nError = rKey.GetNextSubkeyName()) == ERROR_SUCCESS)
  {
    if (PatternMatch(pszKeyPattern,pszSubkeyName))
    {
      if (blnRecursive)
      { // deltion is recursive, delete subkey subkeys
        CRegistryKey Subkey;
        // open subkey
        nError = rKey.OpenSubkey(DELETE,pszSubkeyName,Subkey);
        // delete subkey subkeys
        if (DeleteSubkeys(Subkey, PATTERN_MATCH_ALL, TRUE))
        {
          AddErrorDescription(_T("Cannot delete subkey(s) of key %s. Subkey deletion failed.\n"),Subkey.GetKeyName());
          return FALSE;
        }
      }

      nError = rKey.DeleteSubkey(pszSubkeyName);
      if (nError != ERROR_SUCCESS)
      {
        SetError(_T("Cannot delete the %s subkey of key %s.\nError %d (%s)\n"),
                 pszSubkeyName,rKey.GetKeyName(),nError,GetErrorDescription(nError));
    
        return FALSE;
      }
      blnKeyDeleted = TRUE;
      rKey.InitSubkeyEnumeration(pszSubkeyName, dwMaxSubkeyNameLength); // reset iteration
    }
  }

  ASSERT(nError != ERROR_SUCCESS);
  if (nError != ERROR_NO_MORE_ITEMS)
  {
    SetError(_T("Cannot delete subkeys(s) of key %s.\nSubkey enumeration failed.\nError %d (%s)\n"),
             rKey.GetKeyName(),nError,GetErrorDescription(nError));
    return FALSE;
  }

  if (!blnKeyDeleted)
    SetError(_T("The key %s has no subkeys that match %s pattern.\n"),rKey.GetKeyName(),pszKeyPattern);

  return blnKeyDeleted;
}

const TCHAR * CRegistryTree::GetErrorDescription(LONG nError)
{
  switch(nError)
  {
  case ERROR_ACCESS_DENIED:
    return _T("Access denied");
  case ERROR_FILE_NOT_FOUND:
    return _T("The system cannot find the key specified");
  case ERROR_INTERNAL_ERROR:
    return _T("Internal error");
  case ERROR_OUTOFMEMORY:
    return _T("Out of memory");
  default:
    return _T("Unknown error");
  }
}

void CRegistryTree::SetError(LONG nError)
{
  SetError(_T("Error %u (%s)"),nError,GetErrorDescription(nError));
}

void CRegistryTree::SetError(const TCHAR *pszFormat, ...)
{
  va_list args;
  va_start(args,pszFormat);
  if (_vsntprintf(m_ErrorMsg,ERROR_MSG_BUFFER_SIZE,pszFormat,args) < 0)
    m_ErrorMsg[ERROR_MSG_BUFFER_SIZE] = 0;
  va_end(args);
}

void CRegistryTree::SetInternalError()
{
  SetError(_T("Internal Error"));
}

void CRegistryTree::AddErrorDescription(const TCHAR *pszFormat, ...)
{
  size_t size = _tcslen(m_ErrorMsg);
  if (size < ERROR_MSG_BUFFER_SIZE)
  {
    TCHAR *pszAdd = m_ErrorMsg+size;
    va_list args;
    va_start(args,pszFormat);
    size = ERROR_MSG_BUFFER_SIZE-size;
    if (_vsntprintf(pszAdd,size,pszFormat,args) < 0)
      m_ErrorMsg[size] = 0;
    va_end(args);
  }
}

void CRegistryTree::SetErrorCommandNAOnRoot(const TCHAR *pszCommand)
{
  ASSERT(pszCommand);
  if (pszCommand)
    SetError(_T("%s") COMMAND_NA_ON_ROOT,pszCommand);
  else
    SetInternalError();
}

BOOL CRegistryTree::InternalChangeCurrentKey(const TCHAR *pszSubkeyName, REGSAM DesiredAccess)
{
  size_t size = _tcslen(pszSubkeyName);
  TCHAR *pszSubkeyNameBuffer = new TCHAR[size+3];
  if (!pszSubkeyNameBuffer)
  {
    SetError(_T("Cannot open key : %s%s\nError %d (%s)\n"),
             GetCurrentPath(),pszSubkeyName,ERROR_OUTOFMEMORY,GetErrorDescription(ERROR_OUTOFMEMORY));
  }

  _tcscpy(pszSubkeyNameBuffer,pszSubkeyName);
  if (size && (pszSubkeyName[0] == _T('\"')) && (pszSubkeyName[size-1] == _T('\"')))
  {
    pszSubkeyNameBuffer[size-1] = 0;
    pszSubkeyName = pszSubkeyNameBuffer+1;
  }
  
  if (_tcscmp(pszSubkeyName,_T(".")) == 0)
  {
    delete pszSubkeyNameBuffer;
    return TRUE;
  }

  if (_tcscmp(pszSubkeyName,_T("..")) == 0)
  {
    // Up level abstraction
    if (m_pCurrentKey->m_Key.IsRoot())
    {
      // We are on root
      ASSERT(m_pCurrentKey->m_pUp == NULL);
      SetError(_T("Cannot open key. The root is not child.\n"));
      delete pszSubkeyNameBuffer;
      return FALSE;
    }
      
    ASSERT(m_pCurrentKey->m_pUp);
    if (!m_pCurrentKey->m_pUp)
    {
      SetInternalError();
      delete pszSubkeyNameBuffer;
      return FALSE;
    }
    CNode *pNode = m_pCurrentKey;
    m_pCurrentKey = m_pCurrentKey->m_pUp;
    delete pNode;
    delete pszSubkeyNameBuffer;
    return TRUE;
  }

  CNode *pNewKey = new CNode;
  if (!pNewKey)
  {
    SetError(_T("Cannot open key : %s%s\nError %d (%s)\n"),
             GetCurrentPath(),pszSubkeyName,ERROR_OUTOFMEMORY,GetErrorDescription(ERROR_OUTOFMEMORY));
    delete pszSubkeyNameBuffer;
    return FALSE;
  }

  if (!InternalGetSubkey(pszSubkeyName,DesiredAccess,pNewKey->m_Key))
  {
    delete pNewKey;
    delete pszSubkeyNameBuffer;
    return FALSE;
  }
  pNewKey->m_pUp = m_pCurrentKey;
  m_pCurrentKey = pNewKey;
  
  delete pszSubkeyNameBuffer;
  return TRUE;
}

BOOL CRegistryTree::InternalGetSubkey(const TCHAR *pszSubkeyName, REGSAM DesiredAccess, CRegistryKey& rKey)
{
  LONG nError;
  HKEY hNewKey = NULL;
  TCHAR *pszSubkeyNameCaseUpdated = NULL;

  nError = m_pCurrentKey->m_Key.OpenSubkey(DesiredAccess,pszSubkeyName,hNewKey);

  if (nError != ERROR_SUCCESS)
  {
		SetError(_T("Cannot open key : %s%s\nError %u (%s)\n"),
             GetCurrentPath(),pszSubkeyName,nError,GetErrorDescription(nError));

    return FALSE;
  }

  // enum subkeys to find the subkey and get its name in stored case.
  DWORD dwMaxSubkeyNameLength;
	nError = m_pCurrentKey->m_Key.GetSubkeyNameMaxLength(dwMaxSubkeyNameLength);
  if (nError != ERROR_SUCCESS)
    goto SkipCaseUpdate;
  
  pszSubkeyNameCaseUpdated = new TCHAR [dwMaxSubkeyNameLength];
  m_pCurrentKey->m_Key.InitSubkeyEnumeration(pszSubkeyNameCaseUpdated, dwMaxSubkeyNameLength);
  while ((nError = m_pCurrentKey->m_Key.GetNextSubkeyName()) == ERROR_SUCCESS)
    if (_tcsicmp(pszSubkeyNameCaseUpdated, pszSubkeyName) == 0)
      break;

  if (nError != ERROR_SUCCESS)
  {
    delete pszSubkeyNameCaseUpdated;
    pszSubkeyNameCaseUpdated = NULL;
  }

SkipCaseUpdate:

  HRESULT hr;
  ASSERT(hNewKey);
  if (pszSubkeyNameCaseUpdated)
  {
    hr = rKey.Init(hNewKey,GetCurrentPath(),pszSubkeyNameCaseUpdated,DesiredAccess);
    if (FAILED(hr))
    {
      if (hr == (HRESULT)E_OUTOFMEMORY)
        SetError(_T("Cannot open key : %s%s\nError %d (%s)\n"),
                 GetCurrentPath(),pszSubkeyName,ERROR_OUTOFMEMORY,GetErrorDescription(ERROR_OUTOFMEMORY));
      else
        SetError(_T("Cannot open key : %s%s\nUnknown error\n"), GetCurrentPath(), pszSubkeyName);

      goto Abort;
    }
    
    delete pszSubkeyNameCaseUpdated;
  }
  else
  {
    hr = rKey.Init(hNewKey,GetCurrentPath(),pszSubkeyName,DesiredAccess);
    if (FAILED(hr))
    {
      if (hr == (HRESULT)E_OUTOFMEMORY)
        SetError(_T("Cannot open key : %s%s\nError %d (%s)\n"),
                 GetCurrentPath(),pszSubkeyName,ERROR_OUTOFMEMORY,GetErrorDescription(ERROR_OUTOFMEMORY));
      else
        SetError(_T("Cannot open key : %s%s\nUnknown error \n"),
                 GetCurrentPath(),
                 pszSubkeyName);
      
      goto Abort;
    }
  }
  
  return TRUE;
Abort:
  if (pszSubkeyNameCaseUpdated)
    delete pszSubkeyNameCaseUpdated;

  if (hNewKey)
  {
    LONG nError = RegCloseKey(hNewKey);
    ASSERT(nError == ERROR_SUCCESS);
  }
  
  return FALSE;
}

BOOL CRegistryTree::GetKey(const TCHAR *pszRelativePath, REGSAM DesiredAccess, CRegistryKey& rKey)
{
  CRegistryTree Tree(*this);

  if (!Tree.ChangeCurrentKey(pszRelativePath))
  {
    SetError(Tree.GetLastErrorDescription());
    return FALSE;
  }

  if (Tree.m_pCurrentKey->m_Key.IsRoot())
  {
    HRESULT hr = rKey.InitRoot(m_pszMachineName);
    if (FAILED(hr))
    {
      if (hr == (HRESULT)E_OUTOFMEMORY)
        SetError(_T("\nError %d (%s)\n"),
                 ERROR_OUTOFMEMORY,GetErrorDescription(ERROR_OUTOFMEMORY));
      else
        SetInternalError();
      return FALSE;
    }
    
    return TRUE;
  }

  // open key with desired access
  
  // may be call to DuplicateHandle() is better.
  // registry key handles returned by the RegConnectRegistry function cannot be used in a call to DuplicateHandle.

  // Get short key name now...
  const TCHAR *pszKeyName = Tree.m_pCurrentKey->m_Key.GetKeyName();
  if (pszKeyName == NULL)
  {
    SetInternalError();
    return FALSE;
  }
  
  size_t size = _tcslen(pszKeyName);
  ASSERT(size);
  if (!size)
  {
    SetInternalError();
    return FALSE;
  }
  
  const TCHAR *pszShortKeyName_ = pszKeyName + size-1;
  pszShortKeyName_--; // skip ending backslash
  size = 0;
  while (pszShortKeyName_ >= pszKeyName)
  {
    if (*pszShortKeyName_ == _T('\\'))
      break;
    pszShortKeyName_--;
    size++;
  }
  
  if (!size || (*pszShortKeyName_ != _T('\\')))
  {
    ASSERT(FALSE);
    SetInternalError();
    return FALSE;
  }

  TCHAR *pszShortKeyName = new TCHAR [size+1];
  if (!pszShortKeyName)
  {
    SetError(ERROR_OUTOFMEMORY);
    return FALSE;
  }

  memcpy(pszShortKeyName,pszShortKeyName_+1,size*sizeof(TCHAR));
  pszShortKeyName[size] = 0;

  // change to parent key
	if (!Tree.InternalChangeCurrentKey(_T(".."),READ_CONTROL))
  {
    ASSERT(FALSE);
    SetInternalError();
    return FALSE;
  }
  
  // change back to target key
	if (!Tree.InternalGetSubkey(pszShortKeyName,DesiredAccess,rKey))
  {
    SetError(Tree.GetLastErrorDescription());
    return FALSE;
  }

  return TRUE;
}

