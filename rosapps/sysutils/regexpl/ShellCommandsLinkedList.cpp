/* $Id: ShellCommandsLinkedList.cpp,v 1.3 2001/01/13 23:55:37 narnaoud Exp $
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

// ShellCommandsLinkedList.cpp: implementation of the CShellCommandsLinkedList class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "ShellCommandsLinkedList.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellCommandsLinkedList::CShellCommandsLinkedList(CConsole& rConsole):m_rConsole(rConsole)
{
	m_pRoot = NULL;
}

CShellCommandsLinkedList::~CShellCommandsLinkedList()
{
}

void CShellCommandsLinkedList::AddCommand(CShellCommand *pCommand)
{
	// Create new node
	SNode *pNewNode = new SNode;
	if (pNewNode == NULL)
		return;

	pNewNode->m_pData = pCommand;

	// add new node to the end
	if (m_pRoot)
	{
		SNode *pLastNode = m_pRoot;

		while (pLastNode->m_pNext)
			pLastNode = pLastNode->m_pNext;

		pLastNode->m_pNext = pNewNode;
	}
	else
	{
		m_pRoot = pNewNode;
	}
}

int CShellCommandsLinkedList::Execute(CArgumentParser& rArgumentParser, int& nReturnValue)
{
	rArgumentParser.ResetArgumentIteration();

	const TCHAR *pchCommand = rArgumentParser.GetNextArgument();

	if (pchCommand == NULL)	// empty command line
		return -2;

	int i = -1;

	SNode *pNode = m_pRoot;
	
	while(pNode)
	{
		i++;
		if (pNode->m_pData->Match(pchCommand))
		{
			nReturnValue = pNode->m_pData->Execute(m_rConsole,rArgumentParser);
			return i;
		}
		pNode = pNode->m_pNext;
	}

	return -1;
}

CShellCommand * CShellCommandsLinkedList::Match(const TCHAR * pchCommand)
{
	SNode *pNode = m_pRoot;
	
	while(pNode)
	{
		if (pNode->m_pData->Match(pchCommand))
			return pNode->m_pData;
		pNode = pNode->m_pNext;
	}

	return NULL;
}

POSITION CShellCommandsLinkedList::GetFirstCommandPosition()
{
	return (POSITION)m_pRoot;
}

CShellCommand * CShellCommandsLinkedList::GetNextCommand(POSITION& rPos)
{
	CShellCommand * pCommand = ((SNode *)rPos)->m_pData;
	rPos = (POSITION)(((SNode *)rPos)->m_pNext);
	return pCommand;
}

