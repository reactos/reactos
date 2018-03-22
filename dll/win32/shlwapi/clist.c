/*
 * SHLWAPI DataBlock List functions
 *
 * Copyright 2002 Jon Griffiths
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "shlobj.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* dwSignature for contained DATABLOCK_HEADER items */
#define CLIST_ID_CONTAINER (~0U)

/*************************************************************************
 * NextItem
 *
 * Internal helper: move a DataBlock pointer to the next item.
 */
static inline LPDATABLOCK_HEADER NextItem(LPDBLIST lpList)
{
  char* address = (char*)lpList;
  address += lpList->cbSize;
  return (LPDATABLOCK_HEADER)address;
}

/*************************************************************************
 *      @	[SHLWAPI.20]
 *
 * Insert a new item into a DataBlock list.
 *
 * PARAMS
 *  lppList   [0] Pointer to the List
 *  lpNewItem [I] The new item to add to the list
 *
 * RETURNS
 *  Success: S_OK. The item is added to the list.
 *  Failure: An HRESULT error code.
 *
 * NOTES
 *  If the size of the element to be inserted is less than the size of a
 *  DATABLOCK_HEADER node, or the Id for the item is CLIST_ID_CONTAINER,
 *  the call returns S_OK but does not actually add the element.
 *  See SHWriteDataBlockList.
 */
BOOL WINAPI SHAddDataBlock(LPDBLIST* lppList, const DATABLOCK_HEADER *lpNewItem)
{
  LPDATABLOCK_HEADER lpInsertAt = NULL;
  ULONG ulSize;

  TRACE("(%p,%p)\n", lppList, lpNewItem);

  if(!lppList || !lpNewItem)
    return FALSE;

  if (lpNewItem->cbSize < sizeof(DATABLOCK_HEADER) ||
      lpNewItem->dwSignature == CLIST_ID_CONTAINER)
    return FALSE;

  ulSize = lpNewItem->cbSize;

  if(ulSize & 0x3)
  {
    /* Tune size to a ULONG boundary, add space for container element */
    ulSize = ((ulSize + 0x3) & 0xFFFFFFFC) + sizeof(DATABLOCK_HEADER);
    TRACE("Creating container item, new size = %d\n", ulSize);
  }

  if(!*lppList)
  {
    /* An empty list. Allocate space for terminal ulSize also */
    *lppList = LocalAlloc(LMEM_ZEROINIT, ulSize + sizeof(ULONG));
    lpInsertAt = *lppList;
  }
  else
  {
    /* Append to the end of the list */
    ULONG ulTotalSize = 0;
    LPDATABLOCK_HEADER lpIter = *lppList;

    /* Iterate to the end of the list, calculating the total size */
    while (lpIter->cbSize)
    {
      ulTotalSize += lpIter->cbSize;
      lpIter = NextItem(lpIter);
    }

    /* Increase the size of the list */
    lpIter = LocalReAlloc(*lppList, ulTotalSize + ulSize+sizeof(ULONG),
                                          LMEM_ZEROINIT | LMEM_MOVEABLE);
    if(lpIter)
    {
      *lppList = lpIter;
      lpInsertAt = (LPDATABLOCK_HEADER)((char*)lpIter + ulTotalSize); /* At end */
    }
  }

  if(lpInsertAt)
  {
    /* Copy in the new item */
    LPDATABLOCK_HEADER lpDest = lpInsertAt;

    if(ulSize != lpNewItem->cbSize)
    {
      lpInsertAt->cbSize = ulSize;
      lpInsertAt->dwSignature = CLIST_ID_CONTAINER;
      lpDest++;
    }
    memcpy(lpDest, lpNewItem, lpNewItem->cbSize);

    /* Terminate the list */
    lpInsertAt = NextItem(lpInsertAt);
    lpInsertAt->cbSize = 0;

    return TRUE;
  }
  return FALSE;
}

/*************************************************************************
 *      @	[SHLWAPI.17]
 *
 * Write a DataBlock list to an IStream object.
 *
 * PARAMS
 *  lpStream  [I] IStream object to write the list to
 *  lpList    [I] List of items to write
 *
 * RETURNS
 *  Success: S_OK. The object is written to the stream.
 *  Failure: An HRESULT error code
 *
 * NOTES
 *  Ordinals 17,18,19,20,21 and 22 are related and together provide a compact
 *  list structure (a "DataBlock List"), which may be stored and retrieved from
 *  an IStream object.
 *
 *  The exposed API consists of:
 *
 *  - SHWriteDataBlockList() - Write a DataBlock list to a stream,
 *  - SHReadDataBlockList() - Read and create a list from a stream,
 *  - SHFreeDataBlockList() - Free a list,
 *  - SHAddDataBlock() - Insert a new item into a list,
 *  - SHRemoveDataBlock() - Remove an item from a list,
 *  - SHFindDataBlock() - Find an item in a list.
 *
 *  The DataBlock list is stored packed into a memory array. Each element has a
 *  size and an associated ID. Elements must be less than 64k if the list is
 *  to be subsequently read from a stream.
 *
 *  Elements are aligned on DWORD boundaries. If an elements data size is not
 *  a DWORD size multiple, the element is wrapped by inserting a surrounding
 *  element with an Id of 0xFFFFFFFF, and size sufficient to pad to a DWORD boundary.
 *
 *  These functions are slow for large objects and long lists.
 */
HRESULT WINAPI SHWriteDataBlockList(IStream* lpStream, LPDBLIST lpList)
{
  ULONG ulSize;
  HRESULT hRet = S_OK;

  TRACE("(%p,%p)\n", lpStream, lpList);

  if(lpList)
  {
    while (lpList->cbSize)
    {
      LPDATABLOCK_HEADER lpItem = lpList;

      if(lpList->dwSignature == CLIST_ID_CONTAINER)
        lpItem++;

      hRet = IStream_Write(lpStream,lpItem,lpItem->cbSize,&ulSize);
      if (FAILED(hRet))
        return hRet;

      if(lpItem->cbSize != ulSize)
        return STG_E_MEDIUMFULL;

      lpList = NextItem(lpList);
    }
  }

  if(SUCCEEDED(hRet))
  {
    ULONG ulDummy;
    ulSize = 0;

    /* Write a terminating list entry with zero size */
    hRet = IStream_Write(lpStream, &ulSize,sizeof(ulSize),&ulDummy);
  }

  return hRet;
}

/*************************************************************************
 *      @	[SHLWAPI.18]
 *
 * Read and create a DataBlock list from an IStream object.
 *
 * PARAMS
 *  lpStream  [I] Stream to read the list from
 *  lppList   [0] Pointer to receive the new List
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: An HRESULT error code
 *
 * NOTES
 *  When read from a file, list objects are limited in size to 64k.
 *  See SHWriteDataBlockList.
 */
HRESULT WINAPI SHReadDataBlockList(IStream* lpStream, LPDBLIST* lppList)
{
  DATABLOCK_HEADER bBuff[128]; /* Temporary storage for new list item */
  ULONG ulBuffSize = sizeof(bBuff);
  LPDATABLOCK_HEADER pItem = bBuff;
  ULONG ulRead, ulSize;
  HRESULT hRet = S_OK;

  TRACE("(%p,%p)\n", lpStream, lppList);

  if(*lppList)
  {
    /* Free any existing list */
    LocalFree(*lppList);
    *lppList = NULL;
  }

  do
  {
    /* Read the size of the next item */
    hRet = IStream_Read(lpStream, &ulSize,sizeof(ulSize),&ulRead);

    if(FAILED(hRet) || ulRead != sizeof(ulSize) || !ulSize)
      break; /* Read failed or read zero size (the end of the list) */

    if(ulSize > 0xFFFF)
    {
      LARGE_INTEGER liZero;
      ULARGE_INTEGER ulPos;

      liZero.QuadPart = 0;

      /* Back the stream up; this object is too big for the list */
      if(SUCCEEDED(IStream_Seek(lpStream, liZero, STREAM_SEEK_CUR, &ulPos)))
      {
        liZero.QuadPart = ulPos.QuadPart - sizeof(ULONG);
        IStream_Seek(lpStream, liZero, STREAM_SEEK_SET, NULL);
      }
      break;
    }
    else if (ulSize >= sizeof(DATABLOCK_HEADER))
    {
      /* Add this new item to the list */
      if(ulSize > ulBuffSize)
      {
        /* We need more buffer space, allocate it */
        LPDATABLOCK_HEADER lpTemp;

        if (pItem == bBuff)
          lpTemp = LocalAlloc(LMEM_ZEROINIT, ulSize);
        else
          lpTemp = LocalReAlloc(pItem, ulSize, LMEM_ZEROINIT|LMEM_MOVEABLE);

        if(!lpTemp)
        {
          hRet = E_OUTOFMEMORY;
          break;
        }
        ulBuffSize = ulSize;
        pItem = lpTemp;
      }

      pItem->cbSize = ulSize;
      ulSize -= sizeof(pItem->cbSize); /* already read this member */

      /* Read the item Id and data */
      hRet = IStream_Read(lpStream, &pItem->dwSignature, ulSize, &ulRead);

      if(FAILED(hRet) || ulRead != ulSize)
        break;

      SHAddDataBlock(lppList, pItem); /* Insert Item */
    }
  } while(1);

  /* If we allocated space, free it */
  if(pItem != bBuff)
    LocalFree(pItem);

  return hRet;
}

/*************************************************************************
 *      @	[SHLWAPI.19]
 *
 * Free a DataBlock list.
 *
 * PARAMS
 *  lpList [I] List to free
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  See SHWriteDataBlockList.
 */
VOID WINAPI SHFreeDataBlockList(LPDBLIST lpList)
{
  TRACE("(%p)\n", lpList);

  if (lpList)
    LocalFree(lpList);
}

/*************************************************************************
 *      @	[SHLWAPI.21]
 *
 * Remove an item from a DataBlock list.
 *
 * PARAMS
 *  lppList     [O] List to remove the item from
 *  dwSignature [I] Id of item to remove
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE, If any parameters are invalid, or the item was not found.
 *
 * NOTES
 *  See SHWriteDataBlockList.
 */
BOOL WINAPI SHRemoveDataBlock(LPDBLIST* lppList, DWORD dwSignature)
{
  LPDATABLOCK_HEADER lpList = NULL;
  LPDATABLOCK_HEADER lpItem = NULL;
  LPDATABLOCK_HEADER lpNext;
  ULONG ulNewSize;

  TRACE("(%p,%d)\n", lppList, dwSignature);

  if(lppList && (lpList = *lppList))
  {
    /* Search for item in list */
    while (lpList->cbSize)
    {
      if(lpList->dwSignature == dwSignature ||
        (lpList->dwSignature == CLIST_ID_CONTAINER && lpList[1].dwSignature == dwSignature))
      {
        lpItem = lpList; /* Found */
        break;
      }
      lpList = NextItem(lpList);
    }
  }

  if(!lpItem)
    return FALSE;

  lpList = lpNext = NextItem(lpItem);

  /* Locate the end of the list */
  while (lpList->cbSize)
    lpList = NextItem(lpList);

  /* Resize the list */
  ulNewSize = LocalSize(*lppList) - lpItem->cbSize;

  /* Copy following elements over lpItem */
  memmove(lpItem, lpNext, (char *)lpList - (char *)lpNext + sizeof(ULONG));

  if(ulNewSize <= sizeof(ULONG))
  {
    LocalFree(*lppList);
    *lppList = NULL; /* Removed the last element */
  }
  else
  {
    lpList = LocalReAlloc(*lppList, ulNewSize, LMEM_ZEROINIT|LMEM_MOVEABLE);
    if(lpList)
      *lppList = lpList;
  }
  return TRUE;
}

/*************************************************************************
 *      @	[SHLWAPI.22]
 *
 * Find an item in a DataBlock list.
 *
 * PARAMS
 *  lpList      [I] List to search
 *  dwSignature [I] Id of item to find
 *
 * RETURNS
 *  Success: A pointer to the list item found
 *  Failure: NULL
 *
 * NOTES
 *  See SHWriteDataBlockList.
 */
DATABLOCK_HEADER* WINAPI SHFindDataBlock(LPDBLIST lpList, DWORD dwSignature)
{
  TRACE("(%p,%d)\n", lpList, dwSignature);

  if(lpList)
  {
    while(lpList->cbSize)
    {
      if(lpList->dwSignature == dwSignature)
        return lpList; /* Matched */
      else if(lpList->dwSignature == CLIST_ID_CONTAINER && lpList[1].dwSignature == dwSignature)
        return lpList + 1; /* Contained item matches */

      lpList = NextItem(lpList);
    }
  }
  return NULL;
}
