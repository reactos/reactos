/* $Id: Settings.cpp,v 1.1 2001/04/16 05:03:29 narnaoud Exp $
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

// Settings.cpp : implemetation of CSettings class - user customizable settings for Registry Explorer

#include "ph.h"
#include "RegistryExplorer.h"
#include "Settings.h"
#include "Prompt.h"

#define DEFAULT_NORMAL_TEXT_ATTRIBUTES           FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED
#define DEFAULT_PROMPT_TEXT_ATTRIBUTES           FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED

CSettings::CSettings()
{
  m_pszPrompt = NULL;

  m_wNormalTextAttributes = DEFAULT_NORMAL_TEXT_ATTRIBUTES;
  m_wPromptTextAttributes = DEFAULT_PROMPT_TEXT_ATTRIBUTES;
}

CSettings::~CSettings()
{
  VERIFY(SUCCEEDED(Clean()));
}

HRESULT CSettings::Clean()
{
  if (m_pszPrompt)
  {
    delete m_pszPrompt;
    m_pszPrompt = NULL;
  }

  return S_OK;
}

HRESULT CSettings::Load(LPCTSTR pszLoadKey)
{
  HKEY hKey = NULL;
  HRESULT hr;
  DWORD dwType;
  DWORD dwSize;
  DWORD w;

  hr = Clean();
  if (FAILED(hr))
    return hr;

  hr = S_OK;

  LONG nError = RegOpenKeyEx(HKEY_CURRENT_USER,pszLoadKey,0,KEY_QUERY_VALUE,&hKey);
  if (nError != ERROR_SUCCESS)
    return S_FALSE;

  nError = RegQueryValueEx(hKey,PROMPT_VALUE_NAME,NULL,&dwType,NULL,&dwSize);
  if (nError == ERROR_SUCCESS && dwType == REG_SZ)
  {
    m_pszPrompt = (TCHAR *) new BYTE[dwSize];
    if (!m_pszPrompt)
    {
      hr = E_OUTOFMEMORY;
      goto Exit;
    }

    nError = RegQueryValueEx(hKey,PROMPT_VALUE_NAME,NULL,&dwType,(BYTE *)m_pszPrompt,&dwSize);
    if (nError != ERROR_SUCCESS || dwType != REG_SZ)
    {
      delete m_pszPrompt;
      m_pszPrompt = NULL;
      hr = S_FALSE;
    }
  }
  else
  {
    hr = S_FALSE;
  }

  dwSize = sizeof(DWORD);
  nError = RegQueryValueEx(hKey,NORMAL_TEXT_ATTRIBUTES_VALUE_NAME,NULL,&dwType,(BYTE *)&w,&dwSize);
  if (nError != ERROR_SUCCESS || dwType != REG_DWORD)
  {
    hr = S_FALSE;
  }
  else
  {
    m_wNormalTextAttributes = (WORD)w;
  }

  dwSize = sizeof(DWORD);
  nError = RegQueryValueEx(hKey,PROMPT_TEXT_ATTRIBUTES_VALUE_NAME,NULL,&dwType,(BYTE *)&w,&dwSize);
  if (nError != ERROR_SUCCESS || dwType != REG_DWORD)
  {
    hr = S_FALSE;
  }
  else
  {
    m_wPromptTextAttributes = (WORD)w;
  }

Exit:

  if (hKey)
    VERIFY(RegCloseKey(hKey) == ERROR_SUCCESS);

  return hr;
}

HRESULT CSettings::Store(LPCTSTR pszLoadKey)
{
  return S_OK;
}

LPCTSTR CSettings::GetPrompt()
{
  return m_pszPrompt?m_pszPrompt:CPrompt::GetDefaultPrompt();
}

WORD CSettings::GetNormalTextAttributes()
{
  return m_wNormalTextAttributes;
}

WORD CSettings::GetPromptTextAttributes()
{
  return m_wPromptTextAttributes;
}
