/* $Id: Completion.cpp,v 1.1 2001/01/10 01:25:29 narnaoud Exp $
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

// Completion.cpp : Defines the completion related functions.
#include "ph.h"
#include "RegistryKey.h"
#include "RegistryTree.h"

#define COMPLETION_BUFFER_SIZE	4096

extern CRegistryTree Tree;

BOOL g_blnCompletionCycle = TRUE;

class CCompletionMatch
{
public:
  CCompletionMatch()
  {
    m_pNext = m_pPrev = NULL;
    m_pszText = NULL;
  };

  BOOL Init(const TCHAR *pszText)
  {
    // find the offset to "unique" part
    const TCHAR *pszUnique = _tcsrchr(pszText,_T('\\'));
    pszUnique = pszUnique?(pszUnique+1):pszText;
    BOOL b = _tcschr(pszUnique,_T(' ')) != NULL; // has it spaces in it ???
    size_t s = _tcslen(pszText);
    
    if (m_pszText)
      delete m_pszText;
    
    m_pszText = new TCHAR [s+(b?3:1)]; // if we have spaces in unique part, we need 2 addtional chars for "
    
    if (!m_pszText)
      return FALSE;

    ASSERT(pszText <= pszUnique);
    s = pszUnique - pszText; // calculate offset of the unique part
    if (s)
      _tcsncpy(m_pszText,pszText,s);

    if (b)
      m_pszText[s++] = _T('\"');
    
    _tcscpy(m_pszText+s,pszUnique);

    if (b)
      _tcscat(m_pszText,_T("\""));
    
    return TRUE;
  }
  
  ~CCompletionMatch()
  {
    if (m_pszText)
      delete m_pszText;
  }
  
private:
  TCHAR *m_pszText;
  BOOL m_blnIsKey;
  CCompletionMatch *m_pNext;
  CCompletionMatch *m_pPrev;
  friend class CCompletionList;
};

class CCompletionList
{
public:
  CCompletionList();
  ~CCompletionList();

  BOOL IsNewCompletion(const TCHAR *pszContext, const TCHAR *pszBegin, BOOL& rblnNew);
  BOOL Add(const TCHAR *pszText, BOOL blnIsKey);
  const TCHAR *Get(unsigned __int64 nIndex, BOOL& rblnIsKey);
  unsigned __int64 GetCount();
  const TCHAR * GetContext();
  const TCHAR * GetBegin();
  void DeleteList();
  void Invalidate();
    
private:
  CCompletionMatch *m_pHead; // head of completions linked list
  CCompletionMatch *m_pTail; // tail of completions linked list
  CCompletionMatch *m_pLastSearched;
  unsigned int m_nLastSearched;
  TCHAR *m_pszContext;
  TCHAR *m_pszBegin;
  TCHAR *m_pszCurrentKey;
  unsigned __int64 m_nCount;
} g_Completion;

// --- begin of CCompletionList implementation ---
CCompletionList::CCompletionList()
{
  m_pHead = NULL;
  m_pTail = NULL;
  m_nCount = 0;
  m_pLastSearched = NULL;
  m_pszContext = NULL;
  m_pszBegin = NULL;
  m_pszCurrentKey = NULL;
}

CCompletionList::~CCompletionList()
{
  DeleteList();
    
  if (m_pszContext)
    delete m_pszContext;
    
  if (m_pszBegin)
    delete m_pszBegin;

  if (m_pszCurrentKey)
    delete m_pszCurrentKey;
}

void CCompletionList::Invalidate()
{
  if (m_pszCurrentKey)
  {
    delete m_pszCurrentKey;
    m_pszCurrentKey = NULL;
  }
}

BOOL CCompletionList::IsNewCompletion(const TCHAR *pszContext, const TCHAR *pszBegin, BOOL& rblnNew)
{
  const TCHAR *pszCurrentKey = Tree.GetCurrentPath();
  if (!m_pszContext ||
      !m_pszBegin ||
      !m_pszCurrentKey ||
      (_tcscmp(m_pszContext,pszContext) != 0) ||
      (_tcscmp(m_pszBegin,pszBegin) != 0) ||
      (_tcscmp(m_pszCurrentKey,pszCurrentKey)))
  { // new completion
    DeleteList();
    rblnNew = TRUE;
    if (m_pszContext)
    {
      delete m_pszContext;
      m_pszContext = NULL;
    }

    if (m_pszBegin)
    {
      delete m_pszBegin;
      m_pszBegin = NULL;
    }

    if (m_pszCurrentKey)
    {
      delete m_pszCurrentKey;
      m_pszCurrentKey = NULL;
    }
      
    size_t s = _tcslen(pszContext);
    m_pszContext = new TCHAR[s+1];
    if (!m_pszContext)
      return FALSE;
    _tcscpy(m_pszContext,pszContext);

    s = _tcslen(pszBegin);
    m_pszBegin = new TCHAR[s+1];
    if (!m_pszBegin)
      return FALSE;
    _tcscpy(m_pszBegin,pszBegin);
      
    s = _tcslen(pszCurrentKey);
    m_pszCurrentKey = new TCHAR[s+1];
    if (!m_pszCurrentKey)
      return FALSE;
    _tcscpy(m_pszCurrentKey,pszCurrentKey);
      
    return TRUE;
  }

  rblnNew = FALSE;
  return TRUE;
}
  
BOOL CCompletionList::Add(const TCHAR *pszText, BOOL blnIsKey)
{
  if (_tcsnicmp(pszText,m_pszBegin,_tcslen(m_pszBegin)) != 0)
    return TRUE;
  CCompletionMatch *pNode = new CCompletionMatch;
  if (!pNode)
    return FALSE;
  if (!pNode->Init(pszText))
    return FALSE;

  ASSERT(pNode->m_pszText);
  ASSERT(pNode->m_pNext == NULL);

  // add new node to tail
  pNode->m_blnIsKey = blnIsKey;
  if (m_pTail)
  {
    pNode->m_pPrev = m_pTail;
    ASSERT(m_pTail->m_pNext == NULL);
    m_pTail->m_pNext = pNode;
    m_pTail = pNode;
  }
  else
  {
    ASSERT(m_pHead == NULL);
    m_pHead = m_pTail = pNode;
  }

  m_nCount++;

  m_pLastSearched = NULL;
    
  return TRUE;
}
  
const TCHAR * CCompletionList::Get(unsigned __int64 nIndex, BOOL& rblnIsKey)
{
  ASSERT(nIndex < m_nCount);
  BOOL blnForward;
  CCompletionMatch *pNode = NULL;

  unsigned __int64 nRelativeIndex;

  if (m_pLastSearched)
  {
    pNode = m_pLastSearched;
    blnForward = nIndex > m_nLastSearched;
    nRelativeIndex = blnForward?(nIndex-m_nLastSearched):(m_nLastSearched-nIndex);
    if ((nRelativeIndex > nIndex)||(nRelativeIndex > m_nCount-nIndex-1))
      pNode = NULL; // seraching from tail or from head is more effective
  }
    
  if (!pNode && (nIndex <= m_nCount/2))
  { // search from head
    pNode = m_pHead;
    blnForward = TRUE;
    nRelativeIndex = nIndex;
  }
    
  if (!pNode)
  { // search from tail
    pNode = m_pTail;
    blnForward = FALSE;
    nRelativeIndex = m_nCount-nIndex-1;
  }

  while(pNode)
  {
    if (nRelativeIndex == 0)
    {
      m_nLastSearched = nIndex;
      m_pLastSearched = pNode;
      rblnIsKey = pNode->m_blnIsKey;
      if (!pNode->m_pszText)
        ASSERT(FALSE);
      return pNode->m_pszText;
    }
      
    nRelativeIndex--;
      
    pNode = blnForward?(pNode->m_pNext):(pNode->m_pPrev);
  }

  ASSERT(FALSE);
  return NULL;
}

unsigned __int64 CCompletionList::GetCount()
{
  return m_nCount;
}

const TCHAR * CCompletionList::GetContext()
{
  return m_pszContext;
}

const TCHAR * CCompletionList::GetBegin()
{
  return m_pszBegin;
}
  
void CCompletionList::DeleteList()
{
  CCompletionMatch *pNode;
  while(m_pHead)
  {
    pNode = m_pHead;
    m_pHead = m_pHead->m_pNext;
    delete pNode;
  }
  m_pTail = NULL;
  ASSERT(m_pHead == NULL);
  m_nCount = 0;
}

// --- end of CCompletionList implementation ---

BOOL FillCompletion(const TCHAR *pszKey)
{
  g_Completion.DeleteList();
  LONG nError;
  CRegistryKey Key;
  TCHAR *pszSubkeyName = NULL;
  DWORD dwMaxSubkeyNameLength;
  TCHAR *pszValueName = NULL;
  DWORD dwMaxValueNameSize;

  if (!Tree.GetKey(pszKey?pszKey:_T("."),KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE,Key))
    return FALSE;
  
  BOOL blnCompletionOnKeys = TRUE;
  BOOL blnCompletionOnValues = TRUE;

/*	if ((_tcsnicmp(pchContext,DIR_CMD,DIR_CMD_LENGTH) == 0)||
		(_tcsnicmp(pchContext,CD_CMD,CD_CMD_LENGTH) == 0)||
		(_tcsnicmp(pchContext,OWNER_CMD,OWNER_CMD_LENGTH) == 0)||
		(_tcsnicmp(pchContext,DACL_CMD,DACL_CMD_LENGTH) == 0)||
		(_tcsnicmp(pchContext,SACL_CMD,SACL_CMD_LENGTH) == 0))
	{
		blnCompletionOnValues = FALSE;
	}*/
//	else if (_tcsnicmp(pchContext,VALUE_CMD,VALUE_CMD_LENGTH) == 0)
//	{
//		blnCompletionOnKeys = FALSE;
//	}

  size_t nKeyNameSize = 0;
  if (pszKey)
  {
    nKeyNameSize = _tcslen(pszKey);
    if (_tcscmp(pszKey,_T("\\")))
      nKeyNameSize++;
  } 
  
  if (blnCompletionOnKeys)
  {
    nError = Key.GetSubkeyNameMaxLength(dwMaxSubkeyNameLength);
    if (nError != ERROR_SUCCESS)
      return FALSE;

    pszSubkeyName = new TCHAR[nKeyNameSize+dwMaxSubkeyNameLength+1];
    if (!pszSubkeyName)
      goto Abort;

    if (pszKey)
      _stprintf(pszSubkeyName,_tcscmp(pszKey,_T("\\"))?_T("%s\\"):_T("%s"),pszKey);
      
    Key.InitSubkeyEnumeration(pszSubkeyName+nKeyNameSize,dwMaxSubkeyNameLength);
    while ((nError = Key.GetNextSubkeyName()) == ERROR_SUCCESS)
      if (!g_Completion.Add(pszSubkeyName,TRUE))
      {
        ASSERT(FALSE);
        goto Abort;
      }
    if ((nError != ERROR_SUCCESS)&&(nError != ERROR_NO_MORE_ITEMS))
    {
      ASSERT(FALSE);
      goto Abort;
    }
  }

  if (blnCompletionOnValues)
  {
    nError = Key.GetMaxValueNameLength(dwMaxValueNameSize);
    if (nError != ERROR_SUCCESS)
    {
      ASSERT(FALSE);
      goto Abort;
    }

    pszValueName = new TCHAR[nKeyNameSize+dwMaxValueNameSize+1];
    if (!pszValueName)
      goto Abort;
      
    if (pszKey)
      _stprintf(pszValueName,_tcscmp(pszKey,_T("\\"))?_T("%s\\"):_T("%s"),pszKey);
      
    Key.InitValueEnumeration(pszValueName+nKeyNameSize,dwMaxValueNameSize,NULL,0,NULL);
    while((nError = Key.GetNextValue()) == ERROR_SUCCESS)
      if (!g_Completion.Add(pszValueName,FALSE))
      {
        ASSERT(FALSE);
        goto Abort;
      }
    if ((nError != ERROR_SUCCESS)&&(nError != ERROR_NO_MORE_ITEMS))
    {
      ASSERT(FALSE);
      goto Abort;
    }
  }

  if (pszValueName)
    delete pszValueName;
  if (pszSubkeyName)
    delete pszSubkeyName;
  return TRUE;
Abort:
  if (pszValueName)
    delete pszValueName;
  if (pszSubkeyName)
    delete pszSubkeyName;
  return FALSE;
}

const TCHAR * CompletionCallback(unsigned __int64 & rnIndex,
                                 const BOOL *pblnForward,
                                 const TCHAR *pszContext,
                                 const TCHAR *pszBegin)
{
  static TCHAR pszBuffer[COMPLETION_BUFFER_SIZE];
  
  // Find first non-white space in context
  while(*pszContext && _istspace(*pszContext))
    pszContext++;

  BOOL blnNewCompletion = TRUE;
  if (!g_Completion.IsNewCompletion(pszContext,pszBegin,blnNewCompletion))
  {
    ASSERT(FALSE);
    return NULL;
  }

  if (blnNewCompletion)
  {
    _tcsncpy(pszBuffer,pszBegin,COMPLETION_BUFFER_SIZE-1);
    pszBuffer[COMPLETION_BUFFER_SIZE-1] = 0;
    TCHAR *pszSeparator = pszBuffer; // set it to aby non null value
    if (_tcscmp(pszBuffer,_T("\\")))
    {
      pszSeparator = _tcsrchr(pszBuffer,_T('\\'));
      if (pszSeparator)
        *pszSeparator = 0;
    }
    
    if (!FillCompletion(pszSeparator?pszBuffer:NULL))
      return NULL;
  }
  
  unsigned __int64 nTotalItems = g_Completion.GetCount();
  if (nTotalItems == 0)
    return NULL;

  if (rnIndex >= nTotalItems)
    rnIndex = nTotalItems-1;
  
  if (pblnForward)
  {
    if (*pblnForward)
    {
      rnIndex++;
      if (rnIndex >= nTotalItems)
      {
        if (g_blnCompletionCycle)
          rnIndex = 0;
        else
          rnIndex--;
      }
    }
    else
    {
      if (rnIndex)
        rnIndex--;
      else if (g_blnCompletionCycle)
        rnIndex = nTotalItems-1;
    }
  }
  BOOL blnIsKey = FALSE;
  const TCHAR *pszName = g_Completion.Get(rnIndex,blnIsKey);

  ASSERT(pszName);

  _sntprintf(pszBuffer,COMPLETION_BUFFER_SIZE-1,_T("%s%s"),pszName,(blnIsKey?_T("\\"):_T("")));
  pszBuffer[COMPLETION_BUFFER_SIZE-1] = 0;
  return pszBuffer;
}

void InvalidateCompletion()
{
  g_Completion.Invalidate();
}
