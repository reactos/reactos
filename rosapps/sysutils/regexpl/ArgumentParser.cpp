/* $Id: ArgumentParser.cpp,v 1.5 2001/04/16 04:58:31 narnaoud Exp $
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
   case _T('\\'):               // argument parser ignores escape sequences
     if (pch[1])
       pch++;
     break;
   case _T('\"'):
    blnLongArg = !blnLongArg;
		break;
   case _T(' '):
   case _T('\t'):
   case _T('\r'):
   case _T('\n'):
     if (!blnLongArg)
       *pch = 0;
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
	if (!m_pchArgument)
    m_pchArgument = m_pchArgumentList;

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
