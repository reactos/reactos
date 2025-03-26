/*
 *    IEnumIDList
 *
 *    Copyright 1998    Juergen Schmied <juergen.schmied@metronet.de>
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

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

CEnumIDListBase::CEnumIDListBase() :
    mpFirst(NULL),
    mpLast(NULL),
    mpCurrent(NULL)
{
}

CEnumIDListBase::~CEnumIDListBase()
{
    DeleteList();
}

/**************************************************************************
 *  AddToEnumList()
 */
BOOL CEnumIDListBase::AddToEnumList(LPITEMIDLIST pidl)
{
    ENUMLIST *pNew;

    TRACE("(%p)->(pidl=%p)\n", this, pidl);

    if (!pidl)
        return FALSE;

    pNew = static_cast<ENUMLIST *>(SHAlloc(sizeof(ENUMLIST)));
    if (pNew)
    {
      /*set the next pointer */
      pNew->pNext = NULL;
      pNew->pidl = pidl;

      /*is This the first item in the list? */
      if (!mpFirst)
      {
        mpFirst = pNew;
        mpCurrent = pNew;
      }

      if (mpLast)
      {
        /*add the new item to the end of the list */
        mpLast->pNext = pNew;
      }

      /*update the last item pointer */
      mpLast = pNew;
      TRACE("-- (%p)->(first=%p, last=%p)\n", this, mpFirst, mpLast);
      return TRUE;
    }
    return FALSE;
}

/**************************************************************************
*   DeleteList()
*/
BOOL CEnumIDListBase::DeleteList()
{
    ENUMLIST                    *pDelete;

    TRACE("(%p)->()\n", this);

    while (mpFirst)
    {
        pDelete = mpFirst;
        mpFirst = pDelete->pNext;
        SHFree(pDelete->pidl);
        SHFree(pDelete);
    }
    mpFirst = NULL;
    mpLast = NULL;
    mpCurrent = NULL;
    return TRUE;
}

HRESULT CEnumIDListBase::AppendItemsFromEnumerator(IEnumIDList* pEnum)
{
    LPITEMIDLIST pidl;
    DWORD dwFetched;

    if (!pEnum)
        return E_INVALIDARG;

    pEnum->Reset();

    while((S_OK == pEnum->Next(1, &pidl, &dwFetched)) && dwFetched)
        AddToEnumList(pidl);

    return S_OK;
}

/**************************************************************************
 *  IEnumIDList_fnNext
 */

HRESULT WINAPI CEnumIDListBase::Next(
    ULONG celt,
    LPITEMIDLIST * rgelt,
    ULONG *pceltFetched)
{
    ULONG    i;
    HRESULT  hr = S_OK;
    LPITEMIDLIST  temp;

    TRACE("(%p)->(%d,%p, %p)\n", this, celt, rgelt, pceltFetched);

/* It is valid to leave pceltFetched NULL when celt is 1. Some of explorer's
 * subsystems actually use it (and so may a third party browser)
 */
    if(pceltFetched)
      *pceltFetched = 0;

    *rgelt=0;

    if(celt > 1 && !pceltFetched)
    { return E_INVALIDARG;
    }

    if(celt > 0 && !mpCurrent)
    { return S_FALSE;
    }

    for(i = 0; i < celt; i++)
    { if(!mpCurrent)
      { hr = S_FALSE;
        break;
      }

      temp = ILClone(mpCurrent->pidl);
      if (!temp)
      { hr = i ? S_FALSE : E_OUTOFMEMORY;
        break;
      }
      rgelt[i] = temp;
      mpCurrent = mpCurrent->pNext;
    }
    if(pceltFetched)
    {  *pceltFetched = i;
    }

    return hr;
}

/**************************************************************************
*  IEnumIDList_fnSkip
*/
HRESULT WINAPI CEnumIDListBase::Skip(
    ULONG celt)
{
    DWORD    dwIndex;
    HRESULT  hr = S_OK;

    TRACE("(%p)->(%u)\n", this, celt);

    for(dwIndex = 0; dwIndex < celt; dwIndex++)
    { if(!mpCurrent)
      { hr = S_FALSE;
        break;
      }
      mpCurrent = mpCurrent->pNext;
    }
    return hr;
}

/**************************************************************************
*  IEnumIDList_fnReset
*/
HRESULT WINAPI CEnumIDListBase::Reset()
{
    TRACE("(%p)\n", this);
    mpCurrent = mpFirst;
    return S_OK;
}

/**************************************************************************
*  IEnumIDList_fnClone
*/
HRESULT WINAPI CEnumIDListBase::Clone(LPENUMIDLIST *ppenum)
{
    TRACE("(%p)->() to (%p)->() E_NOTIMPL\n", this, ppenum);
    return E_NOTIMPL;
}

/**************************************************************************
 *  IEnumIDList_Folder_Constructor
 *
 */
HRESULT IEnumIDList_Constructor(IEnumIDList **enumerator)
{
    return ShellObjectCreator<CEnumIDListBase>(IID_PPV_ARG(IEnumIDList, enumerator));
}
