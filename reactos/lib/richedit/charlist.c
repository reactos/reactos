/*
 *
 *  Character List
 *
 *  Copyright (c) 2000 by Jean-Claude Batista
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>

#include "charlist.h"
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

extern HANDLE RICHED32_hHeap;

void CHARLIST_Enqueue( CHARLIST* pCharList, char myChar )
{
    CHARLISTENTRY* pNewEntry = HeapAlloc(RICHED32_hHeap, 0,sizeof(CHARLISTENTRY));
    pNewEntry->pNext = NULL;
    pNewEntry->myChar = myChar;

    TRACE("\n");

    if(pCharList->pTail == NULL)
    {
         pCharList->pHead = pCharList->pTail = pNewEntry;
    }
    else
    {
         CHARLISTENTRY* pCurrent = pCharList->pTail;
         pCharList->pTail = pCurrent->pNext = pNewEntry;
    }

    pCharList->nCount++;
}

char CHARLIST_Dequeue(CHARLIST* pCharList)
{
    CHARLISTENTRY* pCurrent;
    char myChar;

    TRACE("\n");

    if(pCharList->nCount == 0)
      return 0;

    pCharList->nCount--;
    myChar = pCharList->pHead->myChar;
    pCurrent = pCharList->pHead->pNext;
    HeapFree(RICHED32_hHeap, 0,pCharList->pHead);

    if(pCharList->nCount == 0)
    {
        pCharList->pHead = pCharList->pTail = NULL;
    }
    else
    {
        pCharList->pHead = pCurrent;
    }

    return myChar;
}

int CHARLIST_GetNbItems(CHARLIST* pCharList)
{
    TRACE("\n");

    return pCharList->nCount;
}

void CHARLIST_FreeList(CHARLIST* pCharList){
    TRACE("\n");

    while(pCharList->nCount)
        CHARLIST_Dequeue(pCharList);
}

/* this function counts the number of occurrences of a caracter */
int CHARLIST_CountChar(CHARLIST* pCharList, char myChar)
{
    CHARLISTENTRY *pCurrent;
    int nCount = 0;

    TRACE("\n");

    for(pCurrent =pCharList->pHead ;pCurrent;pCurrent=pCurrent->pNext)
    	if(pCurrent->myChar == myChar)
	    nCount++;

    return nCount;
}

int CHARLIST_toBuffer(CHARLIST* pCharList, char* pBuffer, int nBufferSize)
{

   TRACE("\n");

   /* we add one to store a NULL caracter */
   if(nBufferSize < pCharList->nCount + 1)
        return pCharList->nCount;

   for(;pCharList->nCount;pBuffer++)
       *pBuffer = CHARLIST_Dequeue(pCharList);

   *pBuffer = '\0';

   return 0;
}
