/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "imagehlp.h"

/***********************************************************************
 *		InitializeListHead
 */
VOID InitializeListHead(PLIST_ENTRY pListHead)
{
  pListHead->Flink = pListHead;
  pListHead->Blink = pListHead;
}

/***********************************************************************
 *		InsertHeadList
 */
VOID InsertHeadList(PLIST_ENTRY pListHead, PLIST_ENTRY pEntry)
{
  pEntry->Blink = pListHead;
  pEntry->Flink = pListHead->Flink;
  pListHead->Flink = pEntry;
}

/***********************************************************************
 *		InsertTailList
 */
VOID InsertTailList(PLIST_ENTRY pListHead, PLIST_ENTRY pEntry)
{
  pEntry->Flink = pListHead;
  pEntry->Blink = pListHead->Blink;
  pListHead->Blink = pEntry;
}

/***********************************************************************
 *		IsListEmpty
 */
BOOLEAN IsListEmpty(PLIST_ENTRY pListHead)
{
  return !pListHead;
}

/***********************************************************************
 *		PopEntryList
 */
PSINGLE_LIST_ENTRY PopEntryList(PSINGLE_LIST_ENTRY pListHead)
{
  pListHead->Next = NULL;
  return pListHead;
}

/***********************************************************************
 *		PushEntryList
 */
VOID PushEntryList(
  PSINGLE_LIST_ENTRY pListHead, PSINGLE_LIST_ENTRY pEntry)
{
  pEntry->Next=pListHead;
}

/***********************************************************************
 *		RemoveEntryList
 */
VOID RemoveEntryList(PLIST_ENTRY pEntry)
{
  pEntry->Flink->Blink = pEntry->Blink;
  pEntry->Blink->Flink = pEntry->Flink;
  pEntry->Flink = NULL;
  pEntry->Blink = NULL;
}

/***********************************************************************
 *		RemoveHeadList
 */
PLIST_ENTRY RemoveHeadList(PLIST_ENTRY pListHead)
{
  PLIST_ENTRY p = pListHead->Flink;

  if(p != pListHead)
    {
      RemoveEntryList(pListHead);
      return p;
    }
  else
    {
      pListHead->Flink = NULL;
      pListHead->Blink = NULL;
      return NULL;
    }
}

/***********************************************************************
 *		RemoveTailList
 */
PLIST_ENTRY RemoveTailList(PLIST_ENTRY pListHead)
{
  RemoveHeadList(pListHead->Blink);
  if(pListHead != pListHead->Blink)
    return pListHead;
  else
    return NULL;
}
