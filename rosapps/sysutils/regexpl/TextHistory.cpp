/* $Id: TextHistory.cpp,v 1.1 2000/10/04 21:04:31 ea Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (c) 1999-2000 Nedko Arnaoudov <nedkohome@atia.com>
 *
 * License: GNU GPL
 *
 */

// TextHistory.cpp: implementation of the CTextHistory class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "TextHistory.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTextHistory::CTextHistory()
{
	m_pHistoryBuffer = NULL;
	m_dwMaxHistoryLines = 0;
}

CTextHistory::~CTextHistory()
{
	if (m_pHistoryBuffer) delete m_pHistoryBuffer;
}

BOOL CTextHistory::Init(DWORD dwMaxHistoryLineSize, DWORD dwMaxHistoryLines)
{
	if (!dwMaxHistoryLines)
	{
		ASSERT(FALSE);
		return FALSE;
	}
	if (m_pHistoryBuffer) delete m_pHistoryBuffer;
	m_dwFirstHistoryIndex = 0;
	m_dwLastHistoryIndex = 0;
	m_dwHisoryFull = 0;
	m_dwMaxHistoryLines = dwMaxHistoryLines;
	m_dwMaxHistoryLineSize = dwMaxHistoryLineSize;
	m_pHistoryBuffer = new TCHAR [m_dwMaxHistoryLines*dwMaxHistoryLineSize];
	if (!m_pHistoryBuffer) return FALSE;
	return TRUE;
}

void CTextHistory::AddHistoryLine(const TCHAR *pchLine)
{
	if (!m_pHistoryBuffer) return;
	if (_tcslen(pchLine) == 0) return;
	if (_tcslen(pchLine) >= m_dwMaxHistoryLineSize)
	{
		ASSERT(FALSE);
		return;
	}
	if (m_dwHisoryFull == m_dwMaxHistoryLines)	// if buffer is full, replace last
	{
		ASSERT(m_dwFirstHistoryIndex == m_dwLastHistoryIndex);
		m_dwLastHistoryIndex = (m_dwLastHistoryIndex+1)%m_dwMaxHistoryLines;
	}
	ASSERT(m_dwFirstHistoryIndex < m_dwMaxHistoryLines);
	_tcscpy(m_pHistoryBuffer+m_dwFirstHistoryIndex*m_dwMaxHistoryLineSize,pchLine);
	m_dwFirstHistoryIndex = (m_dwFirstHistoryIndex+1)%m_dwMaxHistoryLines;
	ASSERT(m_dwHisoryFull <= m_dwMaxHistoryLines);
	if (m_dwHisoryFull < m_dwMaxHistoryLines) m_dwHisoryFull++;
}

const TCHAR * CTextHistory::GetHistoryLine(DWORD dwIndex)
{
	if (!m_pHistoryBuffer) return NULL;
	ASSERT(m_dwHisoryFull <= m_dwMaxHistoryLines);
	if (dwIndex >= m_dwHisoryFull) return NULL;
	dwIndex = m_dwHisoryFull - dwIndex - 1;
	dwIndex = (dwIndex+m_dwLastHistoryIndex) % m_dwMaxHistoryLines;
	ASSERT(dwIndex < m_dwMaxHistoryLines);
	return m_pHistoryBuffer+dwIndex*m_dwMaxHistoryLineSize;
}
