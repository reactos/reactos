/* $Id: RegistryTree.cpp,v 1.1 2000/10/04 21:04:30 ea Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (c) 1999-2000 Nedko Arnaoudov <nedkohome@atia.com>
 *
 * License: GNU GPL
 *
 */

// RegistryTree.cpp: implementation of the CRegistryTree class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "RegistryTree.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRegistryTree::CRegistryTree(unsigned int nMaxPathSize)
{
	m_pszMachineName = NULL;
	m_samDesiredOpenKeyAccess = 0;
	m_pCurrentKey = m_pRoot = NULL;
	m_ErrorMsg[ERROR_MSG_BUFFER_SIZE] = 0;
	m_ChangeKeyBuffer = new TCHAR[nMaxPathSize];
//#ifdef _DEBUG
	m_nMaxPathSize = nMaxPathSize;
//#endif
}

CRegistryTree::CRegistryTree(const CRegistryTree& Tree)
{
	m_pszMachineName = NULL;
	m_samDesiredOpenKeyAccess = Tree.GetDesiredOpenKeyAccess();
	m_pCurrentKey = m_pRoot = NULL;
	m_ErrorMsg[ERROR_MSG_BUFFER_SIZE] = 0;
	m_ChangeKeyBuffer = new TCHAR[m_nMaxPathSize = Tree.m_nMaxPathSize];
	_tcscpy(m_ChangeKeyBuffer,Tree.GetCurrentPath());
	ChangeCurrentKey(m_ChangeKeyBuffer);
}

CRegistryTree::~CRegistryTree()
{
	if (m_pszMachineName)
		delete m_pszMachineName;

	CRegistryKey *pNode;
	while(m_pRoot)
	{
		pNode = m_pRoot;
		m_pRoot = m_pRoot->GetChild();
		delete pNode;
	}

	delete [] m_ChangeKeyBuffer;
}

const TCHAR * CRegistryTree::GetCurrentPath() const
{
	ASSERT(m_nMaxPathSize > 0);
	unsigned int nBufferSize = m_nMaxPathSize;
	CRegistryKey *pNode = m_pRoot;

	nBufferSize--;
	m_ChangeKeyBuffer[nBufferSize] = 0;

	TCHAR *pchCurrentOffset = m_ChangeKeyBuffer;

	if (m_pszMachineName)
	{
		size_t m = _tcslen(m_pszMachineName+2);
		if (m > nBufferSize)
		{	// No enough space in buffer to store machine name
			ASSERT(FALSE);
		}
		else
		{
			_tcscpy(pchCurrentOffset,m_pszMachineName+2);
			pchCurrentOffset += m;
			nBufferSize -= m;
		}
	}

	if (2 > nBufferSize)
	{	// No enough space in buffer to store '\\'
		ASSERT(FALSE);
	}
	else
	{
		*pchCurrentOffset = _T('\\');
		pchCurrentOffset++;
		nBufferSize--;
	}

	while(pNode)
	{
		TCHAR *pchKeyName = pNode->GetKeyName();
		unsigned int nKeyNameLength = _tcslen(pchKeyName);
		if ((nKeyNameLength+1) > nBufferSize)
		{	// No enough space in buffer to store key name + '\\'
			ASSERT(FALSE);
			break;
		}
		_tcscpy(pchCurrentOffset,pchKeyName);
		pchCurrentOffset[nKeyNameLength] = '\\';
		pchCurrentOffset += nKeyNameLength+1;
		nBufferSize -= nKeyNameLength+1;
		pNode = pNode->GetChild();
	}
	*pchCurrentOffset = 0;
	return m_ChangeKeyBuffer;
}

BOOL CRegistryTree::IsCurrentRoot()
{
	return m_pRoot == NULL;
}

// returns TRUE on success and FALSE on fail
// on fail, extended information can be received  by calling GetLastErrorDescription();
BOOL CRegistryTree::ChangeCurrentKey(const TCHAR *pchRelativePath)
{
	if (*pchRelativePath == _T('\\'))
	{
		CRegistryKey *pNode;
		while(m_pRoot)
		{
			pNode = m_pRoot;
			m_pRoot = m_pRoot->GetChild();
			delete pNode;
		}
		m_pCurrentKey = NULL;
	}
	TCHAR *pchSeps = _T("\\");

	ASSERT(_tcslen(pchRelativePath) <= m_nMaxPathSize);
	_tcscpy(m_ChangeKeyBuffer,pchRelativePath);

	TCHAR *pchNewKey = _tcstok(m_ChangeKeyBuffer,pchSeps);

	if ((!pchNewKey)&&((*pchRelativePath != _T('\\'))||(*(pchRelativePath+1) != 0)))
	{
		_tcscpy(m_ErrorMsg,_T("Invalid key name"));
		return FALSE;
	};
	while (pchNewKey)
	{
		HKEY hNewKey;
		if (m_pRoot == NULL)
		{	// if this is the root key there are limations
			if ((_tcsicmp(pchNewKey,_T("HKCR")) == 0)||
				(_tcsicmp(pchNewKey,_T("HKEY_CLASSES_ROOT")) == 0))
			{
				hNewKey = HKEY_CLASSES_ROOT;
			}
			else if ((_tcsicmp(pchNewKey,_T("HKCU")) == 0)||
				(_tcsicmp(pchNewKey,_T("HKEY_CURRENT_USER")) == 0))
			{
				hNewKey = HKEY_CURRENT_USER;
			}
			else if ((_tcsicmp(pchNewKey,_T("HKLM")) == 0)||
				(_tcsicmp(pchNewKey,_T("HKEY_LOCAL_MACHINE")) == 0))
			{
				hNewKey = HKEY_LOCAL_MACHINE;
			}
			else if ((_tcsicmp(pchNewKey,_T("HKU")) == 0)||
				(_tcsicmp(pchNewKey,_T("HKEY_USERS")) == 0))
			{
				hNewKey = HKEY_USERS;
			}
			else if ((_tcsicmp(pchNewKey,_T("HKPD")) == 0)||
				(_tcsicmp(pchNewKey,_T("HKEY_PERFORMANCE_DATA")) == 0))
			{
				hNewKey = HKEY_PERFORMANCE_DATA;
			}
			else if ((_tcsicmp(pchNewKey,_T("HKDD")) == 0)||
				(_tcsicmp(pchNewKey,_T("HKEY_DYN_DATA")) == 0))
			{
				hNewKey = HKEY_DYN_DATA;
			}
			else if ((_tcsicmp(pchNewKey,_T("HKCC")) == 0)||
				(_tcsicmp(pchNewKey,_T("HKEY_CURRENT_CONFIG")) == 0))
			{
				hNewKey = HKEY_CURRENT_CONFIG;
			}
			else
			{
				_tcscpy(m_ErrorMsg,_T("Invalid key name."));
				return FALSE;
			}
			// Ok. Key to open is in hNewKey

			int nErr = ConnectRegistry(hNewKey);
			if (nErr)
			{
				_stprintf(m_ErrorMsg,_T("Cannot connect registry. Error is %d"),nErr);
				return FALSE;
			}
		}	// if (m_pRoot == NULL)
		else
		{	// current key is not root key
			if (_tcsicmp(pchNewKey,_T("..")) == 0)
			{
				m_pCurrentKey = m_pCurrentKey->UpOneLevel();
				if (m_pCurrentKey == NULL)
				{
					m_pRoot = NULL;
				}
			}
			else
			{	// Normal key name
				CRegistryKey *pNewKey = new CRegistryKey(pchNewKey,m_pCurrentKey);
				//RegOpenKeyExW(*m_pCurrentKey,pchNewKey,0,KEY_EXECUTE,&hNewKey)
				DWORD dwError = pNewKey->Open(m_samDesiredOpenKeyAccess);
				if (dwError != ERROR_SUCCESS)
				{
					TCHAR *pchPreErrorMsg = _T("Cannot open key : "), *pchCurrentOffset = m_ErrorMsg;
					_tcscpy(pchCurrentOffset,pchPreErrorMsg);
					pchCurrentOffset += _tcslen(pchPreErrorMsg);

					_tcscpy(pchCurrentOffset,pchNewKey);
					pchCurrentOffset += _tcslen(pchNewKey);

					TCHAR *pchMsg = _T("\nError ");
					_tcscpy(pchCurrentOffset,pchMsg);
					pchCurrentOffset += _tcslen(pchMsg);

					TCHAR Buffer[256];
					_tcscpy(pchCurrentOffset,_itot(dwError,Buffer,10));
					pchCurrentOffset += _tcslen(Buffer);

					pchMsg = _T("\n");
					_tcscpy(pchCurrentOffset,pchMsg);
					pchCurrentOffset += _tcslen(pchMsg);
					switch(dwError)
					{
					case 5:
						pchMsg = _T("(Access denied)");
						break;
					case 2:
						pchMsg = _T("(The system cannot find the key specified)");
						break;
					}

					_tcscpy(pchCurrentOffset,pchMsg);
					pchCurrentOffset += _tcslen(pchMsg);

					delete pNewKey;

					return FALSE;
				}
				pNewKey->LinkParent();
				m_pCurrentKey = pNewKey;
			}
		}
		// Get next key name
		pchNewKey = _tcstok(NULL,pchSeps);
	}
	return TRUE;
}

TCHAR * CRegistryTree::GetLastErrorDescription()
{
	return m_ErrorMsg;
}

CRegistryKey * CRegistryTree::GetCurrentKey()
{
	return m_pCurrentKey;
}

void CRegistryTree::SetDesiredOpenKeyAccess(REGSAM samDesired)
{
	m_samDesiredOpenKeyAccess = samDesired;
}

REGSAM CRegistryTree::GetDesiredOpenKeyAccess() const
{
	return m_samDesiredOpenKeyAccess;
}

int CRegistryTree::ConnectRegistry(HKEY hKey)
{
	CRegistryKey *pKey = new CRegistryKey(hKey,m_pszMachineName);
	int ret = pKey->Open(m_samDesiredOpenKeyAccess);

	if (ret == 0)
	{
		CRegistryKey *pNode;
		while(m_pRoot)
		{
			pNode = m_pRoot;
			m_pRoot = m_pRoot->GetChild();
			delete pNode;
		}
		m_pCurrentKey = NULL;
		m_pRoot = m_pCurrentKey = pKey;
	}
	else
	{
		delete pKey;
	}

	return ret;
}

void CRegistryTree::SetMachineName(LPCTSTR pszMachineName)
{
	if (m_pszMachineName)
		delete m_pszMachineName;

	if (pszMachineName == NULL)
	{
		m_pszMachineName = NULL;
		return;
	}

	while ((*pszMachineName)&&(*pszMachineName == _T('\\')))
		pszMachineName++;

	if (*pszMachineName == 0)
	{
		ASSERT(FALSE);
	}

	m_pszMachineName = new TCHAR[_tcslen(pszMachineName)+3];
	_tcscpy(m_pszMachineName,_T("\\\\"));
	_tcscpy(m_pszMachineName+2,pszMachineName);
	_tcsupr(m_pszMachineName+2);
}

BOOL CRegistryTree::NewKey(const TCHAR *pchKeyName, BOOL blnVolatile)
{
	CRegistryKey *pNewKey = new CRegistryKey(pchKeyName,m_pCurrentKey);
	DWORD dwDisposition;
	DWORD dwError = pNewKey->Create(0,&dwDisposition,blnVolatile);
	switch (dwDisposition)
	{
	case REG_OPENED_EXISTING_KEY:
		_sntprintf(m_ErrorMsg,ERROR_MSG_BUFFER_SIZE,_T("A key \"%s\" already exists."),pchKeyName);
		delete pNewKey;
		return FALSE;
	case REG_CREATED_NEW_KEY:
		break;
	default:
		ASSERT(FALSE);
	}
	if (dwError != ERROR_SUCCESS)
	{
		TCHAR *pchCurrentOffset = m_ErrorMsg;
		TCHAR *pchPreErrorMsg = _T("Cannot create key : ");
		_tcscpy(pchCurrentOffset,pchPreErrorMsg);
		pchCurrentOffset += _tcslen(pchPreErrorMsg);

		_tcscpy(pchCurrentOffset,pchKeyName);
		pchCurrentOffset += _tcslen(pchKeyName);

		TCHAR *pchMsg = _T("\nError ");
		_tcscpy(pchCurrentOffset,pchMsg);
		pchCurrentOffset += _tcslen(pchMsg);

		TCHAR Buffer[256];
		_tcscpy(pchCurrentOffset,_itot(dwError,Buffer,10));
		pchCurrentOffset += _tcslen(Buffer);

		pchMsg = _T("\n");
		_tcscpy(pchCurrentOffset,pchMsg);
		pchCurrentOffset += _tcslen(pchMsg);
		switch(dwError)
		{
		case 5:
			pchMsg = _T("(Access denied)");
			break;
		case 2:
			pchMsg = _T("(The system cannot find the key specified)");
			break;
		}

		_tcscpy(pchCurrentOffset,pchMsg);
		pchCurrentOffset += _tcslen(pchMsg);

		delete pNewKey;

		return FALSE;
	}

	delete pNewKey;

	return TRUE;
}

BOOL CRegistryTree::DeleteKey(const TCHAR *pchKeyName, BOOL blnRecursive)
{
	DWORD dwError = m_pCurrentKey->DeleteSubkey(pchKeyName,blnRecursive);
	if (dwError != ERROR_SUCCESS)
	{
		TCHAR *pchPreErrorMsg = _T("Cannot open key : "), *pchCurrentOffset = m_ErrorMsg;
		_tcscpy(pchCurrentOffset,pchPreErrorMsg);
		pchCurrentOffset += _tcslen(pchPreErrorMsg);

		_tcscpy(pchCurrentOffset,pchKeyName);
		pchCurrentOffset += _tcslen(pchKeyName);

		TCHAR *pchMsg = _T("\nError ");
		_tcscpy(pchCurrentOffset,pchMsg);
		pchCurrentOffset += _tcslen(pchMsg);

		TCHAR Buffer[256];
		_tcscpy(pchCurrentOffset,_itot(dwError,Buffer,10));
		pchCurrentOffset += _tcslen(Buffer);

		pchMsg = _T("\n");
		_tcscpy(pchCurrentOffset,pchMsg);
		pchCurrentOffset += _tcslen(pchMsg);
		switch(dwError)
		{
		case 5:
			pchMsg = _T("(Access denied)");
			break;
		case 2:
			pchMsg = _T("(The system cannot find the key specified)");
			break;
		}

		_tcscpy(pchCurrentOffset,pchMsg);
		pchCurrentOffset += _tcslen(pchMsg);

		return FALSE;
	}
	return TRUE;
}
