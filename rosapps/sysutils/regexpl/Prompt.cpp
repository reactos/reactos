/* $Id: Prompt.cpp,v 1.1 2001/04/16 04:53:31 narnaoud Exp $
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

// prompt for Registry Explorer

#include "ph.h"
#include "Prompt.h"

#define ESCAPE_CHAR                        _T("\\")
#define CURRENT_PATH_ALIAS_CHAR            _T("w")
#define CURRENT_PATH_ALIAS                 ESCAPE_CHAR CURRENT_PATH_ALIAS_CHAR
#define DEFAULT_PROMPT                     CURRENT_PATH_ALIAS _T("\n# ")

CPrompt::CPrompt(CRegistryTree& rTree, HRESULT& rhr):m_rTree(rTree)
{
  m_pszPrompt = new TCHAR[_tcslen(DEFAULT_PROMPT)+1];
  if (!m_pszPrompt)
  {
    rhr = E_OUTOFMEMORY;
    return;
  }

  _tcscpy(m_pszPrompt,DEFAULT_PROMPT);
}

HRESULT CPrompt::SetPrompt(LPCTSTR pszPrompt)
{
  if (!pszPrompt)
  {
    ASSERT(FALSE);
    return E_UNEXPECTED;
  }

  m_pszPrompt = new TCHAR[_tcslen(pszPrompt)+1];
  if (!m_pszPrompt)
    return E_OUTOFMEMORY;

  _tcscpy(m_pszPrompt,pszPrompt);

  return S_OK;
}

CPrompt::~CPrompt()
{
  if (m_pszPrompt)
    delete m_pszPrompt;
}

void CPrompt::ShowPrompt(CConsole &rConsole)
{
  TCHAR *pch = m_pszPrompt;
  TCHAR Buffer[2] = " ";

  const TCHAR *pszCurrentPath;

  while (*pch)
  {
    if (_tcsncmp(pch,CURRENT_PATH_ALIAS,_tcslen(CURRENT_PATH_ALIAS)) == 0)
    {
      pszCurrentPath = m_rTree.GetCurrentPath();
      rConsole.Write(pszCurrentPath?pszCurrentPath:_T("NULL (Internal Error)"));
      pch += _tcslen(CURRENT_PATH_ALIAS);
    }
    else
    {
      Buffer[0] = *pch++;
      rConsole.Write(Buffer);
    }
  }
}

LPCTSTR CPrompt::GetDefaultPrompt()
{
  return DEFAULT_PROMPT;
}
