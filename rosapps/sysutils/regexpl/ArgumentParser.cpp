/* $Id: ArgumentParser.cpp,v 1.1 2000/10/04 21:04:30 ea Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (c) 1999-2000 Nedko Arnaoudov <nedkohome@atia.com>
 *
 * License: GNU GPL
 *
 */

// ArgumentParser.cpp: implementation of the CArgumentParser class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "ArgumentParser.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CArgumentParser::CArgumentParser()
{
	m_pchArgumentList = NULL;
	m_pchArgumentListEnd = NULL;
	m_pchArgument = NULL;
}

CArgumentParser::~CArgumentParser()
{
}

void CArgumentParser::SetArgumentList(TCHAR *pchArguments)
{
	TCHAR *pch = m_pchArgumentList = pchArguments;
	m_pchArgumentListEnd = pchArguments + _tcslen(pchArguments);

	BOOL blnLongArg = FALSE;
	while (*pch)
	{
		switch(*pch)
		{
		case L'\"':
			if (blnLongArg) blnLongArg = FALSE;
			else blnLongArg = TRUE;
			break;
		case L' ':
		case L'\t':
		case L'\r':
		case L'\n':
			if (!blnLongArg) *pch = 0;
			break;
		}
		pch++;
	}

	ResetArgumentIteration();
}

TCHAR * CArgumentParser::GetNextArgument()
{
	ASSERT(m_pchArgumentList);		// call SetArgumentList() before calling this function
	ASSERT(m_pchArgumentListEnd);	// call SetArgumentList() before calling this function
	ASSERT(m_pchArgumentListEnd >= m_pchArgumentList);

	// if this is begin of iteration
	if (!m_pchArgument) m_pchArgument = m_pchArgumentList;

	while(m_pchArgument)
	{
		if (m_pchArgument > m_pchArgumentListEnd)
		{	// if end of arguments list reached
			ASSERT(m_pchArgument - 1 == m_pchArgumentListEnd);
			break;
		}

		TCHAR *pchArg = m_pchArgument;

		// Next argument
		m_pchArgument += _tcslen(m_pchArgument)+1;

		if(*pchArg)
		{	// if argument is not an empty string
			return pchArg;
		}
	}

	return NULL;
}

void CArgumentParser::ResetArgumentIteration()
{
	m_pchArgument = NULL;
}
