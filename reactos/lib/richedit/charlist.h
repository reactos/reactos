/*
 * Character List
 *
 * Copyright (c) 2000 by Jean-Claude Batista
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

#ifndef _CHARLIST
#define _CHARLIST

typedef struct _tagCHARLISTENTRY
{
    struct _tagCHARLISTENTRY *pNext;
    char   myChar;
} CHARLISTENTRY;

typedef struct _tagCHARLIST
{
    unsigned int nCount; /* Entries Count; */
    CHARLISTENTRY *pHead;
    CHARLISTENTRY *pTail;
} CHARLIST;


void CHARLIST_Enqueue( CHARLIST* pCharList, char myChar);
void CHARLIST_Push( CHARLIST* pCharList, char myChar);
char CHARLIST_Dequeue(CHARLIST* pCharList);
int CHARLIST_GetNbItems(CHARLIST* pCharList);
void CHARLIST_FreeList(CHARLIST* pCharList);
int CHARLIST_CountChar(CHARLIST* pCharList, char myChar);
int CHARLIST_toBuffer(CHARLIST* pCharList, char* pBuffer, int nBufferSize);

#endif
