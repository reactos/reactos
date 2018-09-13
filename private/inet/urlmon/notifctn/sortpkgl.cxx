//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       sortpkgl.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    1-13-1997   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <notiftn.h>

//
//+---------------------------------------------------------------------------
//
//  Method:     CSortPkgList::GetClassID
//
//  Synopsis:
//
//  Arguments:  [pClassID] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CSortPkgList::GetClassID (CLSID *pClassID)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortPkgList::GetClassID\n", this));
    NotfAssert((pClassID));
    HRESULT hr = NOERROR;

    if (!pClassID)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *pClassID = CLSID_StdNotificationMgr;
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortPkgList::GetClassID (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSortPkgList::IsDirty
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CSortPkgList::IsDirty()
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortPkgList::IsDirty\n", this));
    HRESULT hr = NOERROR;

    hr =  (_fDirty) ? S_OK : S_FALSE;

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortPkgList::IsDirty (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSortPkgList::Load
//
//  Synopsis:
//
//  Arguments:  [pStm] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CSortPkgList::Load(IStream *pStm)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortPkgList::Load\n", this));
    HRESULT hr = NOERROR;

    ULONG cCount = 0;

    do
    {
        ULONG i = 0;
        DWORD cbSaved = 0;

        if (!pStm)
        {
            hr = E_INVALIDARG;
            break;
        }

        // get the # of elements saved
        hr = pStm->Read(&cCount, sizeof(ULONG), &cbSaved);
        BREAK_ONERROR(hr);
        NotfAssert(( sizeof(ULONG) == cbSaved));

        // start at the head
        SL_ITEM listitem;

        for (i = 0; i < cCount; i++)
        {
            hr = pStm->Read(&listitem, sizeof(SL_ITEM), &cbSaved);
            if (hr == NOERROR)
            {
                NotfAssert(( sizeof(SL_ITEM) == cbSaved));
                _XDistList.AddVal(listitem.date,listitem);
            }
        }

        NotfAssert((i == cCount));

        break;
    } while (TRUE);

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortPkgList::Load (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSortPkgList::Save
//
//  Synopsis:
//
//  Arguments:  [pStm] --
//              [fClearDirty] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CSortPkgList::Save(IStream *pStm, BOOL fClearDirty)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortPkgList::Save\n", this));
    HRESULT hr = E_FAIL;
    CLock lck(_mxs);

    NotfAssert((pStm));

    ULONG cCount = GetCount();

    do
    {
        if (!pStm)
        {
            hr = E_INVALIDARG;
            break;
        }
        DWORD cbSaved;

        // save the # of elements in the map
        hr = pStm->Write(&cCount, sizeof(ULONG), &cbSaved);
        BREAK_ONERROR(hr);
        NotfAssert(( sizeof(ULONG) == cbSaved));

        // start at the head
        POSITION pos = _XDistList.GetHeadPosition();
        NotfAssert((pos));
        SL_ITEM listitem = _XDistList.GetAt(pos);

        while (pos)
        {
            hr = pStm->Write(&listitem, sizeof(SL_ITEM), &cbSaved);
            if (hr == NOERROR)
            {
                NotfAssert(( sizeof(SL_ITEM) == cbSaved));
                listitem = _XDistList.GetNext(pos);
            }
            else
            {
                pos = 0;
            }
        }

        if (fClearDirty)
        {
            _fDirty = FALSE;
        }

        break;
    } while (TRUE);

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortPkgList::Save (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSortPkgList::GetSizeMax
//
//  Synopsis:
//
//  Arguments:  [pcbSize] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CSortPkgList::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortPkgList::GetSizeMax\n", this));
    HRESULT hr = E_NOTIMPL;

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortPkgList::GetSizeMax (hr:%lx)\n",this, hr));
    return hr;
}

//
// sortlist methods
//

//+---------------------------------------------------------------------------
//
//  Method:     CSortPkgList::Synchronize
//
//  Synopsis:
//
//  Arguments:  [fForceResync] --
//
//  Returns:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CSortPkgList::Synchronize(BOOL fForceResync)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortPkgList::Synchronize\n", this));
    HRESULT hr = NOERROR;
    CLock lck(_mxs);
    do
    {
        if (!fForceResync && IsSynchronized())
        {
            // nothing to do
            break;
        }
        // the list of this process needs to resynchronized
        //
        SCHEDLISTITEMKEY *pSchItems = 0;
        ULONG cCount;

        hr = _rCNotfMgr.GetAllScheduleItems(&pSchItems, &cCount, _dateLastSync);
        BREAK_ONERROR(hr);

        if (cCount == 0)
        {
            NotfAssert((pSchItems == 0));
            if (pSchItems)
            {
                delete [] pSchItems;
            }
            break;
        }
        
        _XDistList.RemoveAll();
        for (ULONG i = 0; i < cCount; i++)
        {
            _XDistList.AddVal(pSchItems[i].date, pSchItems[i]);
        }
        NotfAssert((_XDistList.GetCount() == (int)cCount));
        
        delete [] pSchItems;

        break;
    } while (TRUE);

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortPkgList::Synchronize (hr:%lx)\n",this, hr));
    return hr;
}
//+---------------------------------------------------------------------------
//
//  Method:     CSortPkgList::HandlePackage
//
//  Synopsis:
//
//  Arguments:  [pCPackage] --
//
//  Returns:
//
//  History:    12-16-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CSortPkgList::HandlePackage(CPackage *pCPackage)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortPkgList::HandlePackage\n", this));

    HRESULT hr = E_FAIL;
    NotfAssert((pCPackage));
    NotfAssert((pCPackage->GetNotification()));
    
    CLock lck(_mxs);

    do 
    {
        POSITION UNALIGNED pos = 0;
        POSITION UNALIGNED posRemove = 0;
        
        pos = _XLocalList.GetHeadPosition();
        HRESULT hr1 = E_FAIL;

        while (pos)
        {
            if (_XLocalList.GetAt(pos).pCPkg == pCPackage)
            {
                hr1 = NOERROR;
                posRemove = pos;
                break;
            }
            _XLocalList.GetNext(pos);
        }

        if (hr1 == NOERROR)
        {
            _XLocalList.RemoveAt(posRemove);
            _cElements--;
            RELEASE(pCPackage);
            hr1 = RevokePackage(&pCPackage->GetNotificationCookie(), 0, 0);
        }

        PKG_ARG newElement;
        newElement.date = pCPackage->GetNextRunDate();
        newElement.pCPkg = pCPackage;

        // set the notification state to waiting
        //pCPackage->SetNotificationState(pCPackage->GetNotificationState() | PF_WAITING);

        //
        // start at the tail and loop forward
        //
        pos = _XLocalList.GetTailPosition();

        hr = E_FAIL;
        while (pos)
        {
            PKG_ARG arg = _XLocalList.GetAt(pos);

            // check if current position is smaller if so
            if (arg.date <= newElement.date)
            {
                // found the place
                _XLocalList.InsertAfter(pos, newElement);
                ADDREF(pCPackage);
                _cElements++;
                pos = 0;
                hr = NOERROR;
            }
            else
            {
                _XLocalList.GetPrev(pos);   //  pos --;
            }
        }

        // did not find the place
        // add the element at the front
        //
        if (hr != NOERROR)
        {
            _XLocalList.AddHead(newElement);
            ADDREF(pCPackage);
            _cElements++;
            hr = NOERROR;
        }

        //
        // add the item to the global list

        if (hr == NOERROR)
        {
            //
            // only add the date the cookie - object is not
            // addref'd
            hr = _rCNotfMgr.AddScheduleItem(pCPackage);
        }

        if (hr == NOERROR)
        {
            //
            // resync the distributed list
            //
            hr = Synchronize();
        }
        break;
    } while (TRUE);
    

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortPkgList::HandlePackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSortPkgList::FindFirstPackage
//
//  Synopsis:   finds the first package in the schedule list
//
//  Arguments:  [pPosCookie] -- out parameter used as position pointer
//              [ppCPackage] -- addref object
//              [dwMode] -- ignored for now
//
//  Returns:
//
//  History:    1-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CSortPkgList::FindFirstPackage(
                                 PNOTIFICATIONCOOKIE pPosCookie,
                                 CPackage          **ppCPackage,
                                 DWORD               dwMode
                                 )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortPkgList::FindFirstPackage\n", this));
    NotfAssert((pPosCookie && ppCPackage && !dwMode));

    HRESULT hr = E_FAIL;
    NotfAssert((pPosCookie));

    do
    {
        if (   !pPosCookie
            || !ppCPackage)
        {
            hr = E_INVALIDARG;
            break;
        }

        // set the position to null
        *pPosCookie = COOKIE_NULL;

        hr = Synchronize(TRUE);
        BREAK_ONERROR(hr);

        POSITION pos = _XDistList.GetHeadPosition();

        if (!pos)
        {
            break;
        }

        SCHEDLISTITEMKEY SchItems;
        CPackage *pCPackage = 0;

        hr = E_FAIL;
        do
        {
            SchItems = _XDistList.GetNext(pos);

            hr = _rCNotfMgr.LoadScheduleItemPackage(SchItems, &pCPackage);

        } while (pos && (hr != NOERROR) );

        if (hr == NOERROR)
        {
            NotfAssert(( pCPackage));
            // the package is addref'd already
            *ppCPackage = pCPackage;
            *pPosCookie = SchItems.notfCookie;


            hr =  NOERROR;
        }
        break;
    } while (TRUE);

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortPkgList::FindFirstPackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSortPkgList::FindNextPackage
//
//  Synopsis:   finds the next package in the list
//
//
//  Arguments:  [pPosCookie] -- used a position pointer
//              [ppCPackage] -- in/out parameter - used as position pointer
//              [dwMode] --
//
//  Returns:    NOERROR and the CPackage object
//              E_FAIL  and the packagecookie with CLSID_NULL
//
//  History:    1-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CSortPkgList::FindNextPackage(
                                PNOTIFICATIONCOOKIE pPosCookie,
                                CPackage          **ppCPackage,
                                DWORD               dwMode
                                )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortPkgList::FindNextPackage\n", this));
    NotfAssert((pPosCookie && ppCPackage && !dwMode));

    HRESULT hr = E_FAIL;

    do
    {
        if (   !pPosCookie
            || !ppCPackage)
        {
            hr = E_INVALIDARG;
            break;
        }
      
        if (*pPosCookie == COOKIE_NULL)
        {
            break;
        }
                
        POSITION pos = _XDistList.GetHeadPosition();

        SCHEDLISTITEMKEY SchItem;
        SchItem.notfCookie = COOKIE_NULL;
        CPackage *pCPackage = 0;
        
        hr = E_FAIL;

        while (pos)
        {
            SchItem = _XDistList.GetNext(pos); // return *Position++

            // bugbug; need to fix case where the current position gets deleted
            if (SchItem.notfCookie == *pPosCookie)
            {
                // found current position
                //
                if (pos)
                {
                    SchItem = _XDistList.GetNext(pos);
                    hr = _rCNotfMgr.LoadScheduleItemPackage(SchItem, &pCPackage);
                    pos = 0;
                }
            }
        }
        BREAK_ONERROR(hr);

        // ok found the correct spot
        *pPosCookie = SchItem.notfCookie;
        *ppCPackage = pCPackage;

        break;
    } while (TRUE);

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortPkgList::FindNextPackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSortPkgList::RevokePackage
//
//  Synopsis:   revokes and item from the list and the global table
//
//  Arguments:  [DWORD] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-06-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CSortPkgList::RevokePackage(
                                   PNOTIFICATIONCOOKIE pPkgCookie,
                                   CPackage          **ppCPackage,
                                   DWORD               dwMode
                                   )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortPkgList::RevokePackage\n", this));
    CLock lck(_mxs);

    NotfAssert((pPkgCookie && !dwMode));
    HRESULT hr = E_FAIL;
    CPackage *pCPackage = 0;

    do
    {
        // set the position to null

        hr = Synchronize(TRUE);
        BREAK_ONERROR(hr);
        
        hr = NOTFMGR_E_NOTIFICATION_NOT_FOUND;
        POSITION pos = _XDistList.GetHeadPosition();
        POSITION posRemove = 0;

        if (!pos)
        {
            break;
        }

        SCHEDLISTITEMKEY SchItem;
        SchItem.notfCookie = COOKIE_NULL;

        while (pos)
        {
            SchItem = _XDistList.GetNext(pos); // return *Position++

            if (SchItem.notfCookie == *pPkgCookie)
            {
                // found the object
                //
                posRemove = pos;
                pos = 0;
                hr = NOERROR;
            }
        }
        BREAK_ONERROR(hr);
        
        hr = _rCNotfMgr.LoadScheduleItemPackage(SchItem, &pCPackage);
        BREAK_ONERROR(hr);
         
        // set the state to no waiting
        pCPackage->SetNotificationState(pCPackage->GetNotificationState() & ~PF_WAITING);

        RemoveSchedulePackage(pCPackage, 0);
        
        hr = _rCNotfMgr.RemoveScheduleItem(SchItem, pCPackage);

        //
        // need to remove the package also from the running list
        //
        if (posRemove)
        {
            Synchronize(TRUE);
        }

        if (ppCPackage)
        {
            *ppCPackage = pCPackage;
            pCPackage = 0;
        }

        break;
    } while (TRUE);

    if (pCPackage)
    {
        RELEASE(pCPackage);
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortPkgList::RevokePackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSortPkgList::FindPackage
//
//  Synopsis:   finds a specific item - the process list is ssync'd with
//              the global table
//
//  Arguments:  [pPkgCookie] -- the cookie of the item
//              [ppCPackage] -- addref'd object
//              [dwMode] --
//
//  Returns:
//
//  History:    1-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CSortPkgList::FindPackage(
                            PNOTIFICATIONCOOKIE pPkgCookie,
                            CPackage          **ppCPackage,
                            DWORD               dwMode
                            )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortPkgList::FindPackage\n", this));
    NotfAssert((pPkgCookie && ppCPackage));

    HRESULT hr = E_FAIL;
    CLock lck(_mxs);
    CPackage *pCPackage = 0;

    do
    {
        if (dwMode & LM_LOCALCOPY)
        {
            //
            // find the local package 
            // 
            POSITION pos = _XLocalList.GetHeadPosition();

            while (pos) {
                pCPackage = _XLocalList.GetAt(pos).pCPkg;
                _XLocalList.GetNext(pos);
                
                if (pCPackage)  {
                    if (pCPackage->GetNotificationCookie() == *pPkgCookie)  {
                        pos = 0;
                    } else  {
                        pCPackage = NULL;
                    }
                } else  {
                    pos = 0;
                }
            }
            
            if (pCPackage)
            {
                ADDREF(pCPackage);
                hr = NOERROR;
            }
            else
            {
                hr = E_FAIL;
            }
        }
        else
        {
            hr = Synchronize(TRUE);
            BREAK_ONERROR(hr);

            POSITION pos = _XDistList.GetHeadPosition();

            if (!pos)
            {
                hr = E_FAIL;
                break;
            }

            SCHEDLISTITEMKEY SchItem;
            SchItem.notfCookie = COOKIE_NULL;
            hr = E_FAIL;

            while (pos)
            {
                SchItem = _XDistList.GetNext(pos); // return *Position++

                if (SchItem.notfCookie == *pPkgCookie)
                {
                    // found the object
                    //
                    pos = 0;
                    hr = NOERROR;
                }
            }

            BREAK_ONERROR(hr);

            hr = _rCNotfMgr.LoadScheduleItemPackage(SchItem, &pCPackage);
        }
        
        BREAK_ONERROR(hr);
        // object is addref'd
        *ppCPackage = pCPackage;

        break;
    } while (TRUE);

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortPkgList::FindPackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSortPkgList::FindFirstSchedulePackage
//
//  Synopsis:   finds the first scheduled package of this process
//              a global table lookup is done to check if the item
//              was not removed by another process
//
//  Arguments:  [ppCPackage] --
//              [dwMode] --
//
//  Returns:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CSortPkgList::FindFirstSchedulePackage(
                    PNOTIFICATIONCOOKIE pPosCookie,
                    CPackage          **ppCPackage,
                    DWORD               dwMode
                    )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortPkgList::FindFirstSchedulePackage\n", this));
    NotfAssert((ppCPackage));

    HRESULT hr = E_FAIL;
    CPackage *pCPkg = 0;

    do
    {

        CLock lck(_mxs);
        POSITION pos = _XLocalList.GetHeadPosition();

        if (!pos)
        {
            break;
        }

        pCPkg = _XLocalList.GetAt(pos).pCPkg;

        if (pCPkg)
        {
            if (!(pCPkg->GetNotificationState() & (PF_RUNNING | PF_DELIVERED)))
            {
                if (pCPkg->IsPersisted(_pszWhere) != NOERROR)
                {
                    RELEASE(pCPkg);
                    pCPkg = 0;
                    RemoveHead();
                    pos = _XLocalList.GetHeadPosition();
                }
        //  DZhang fix
                else if (pCPkg->GetTaskTrigger() == NULL)
                {
                    pos = _XLocalList.GetHeadPosition();
                    pCPkg = 0;
                }
            }
            else if (pCPkg->GetTaskTrigger() == NULL)
            {
                pos = _XLocalList.GetHeadPosition();
                pCPkg = 0;
            }
        //  End fix
            else
            {
                pCPkg = 0;
            }
        }
           
        if (!pCPkg)
        {
            do 
            {
                if (!pCPkg && pos)
                {
                    pCPkg = _XLocalList.GetNext(pos).pCPkg;
                }
                
                if (pCPkg)
                {
                    // the package is still in the list
                    // check if it is still persisted
                    if (!(pCPkg->GetNotificationState() & (PF_RUNNING | PF_DELIVERED)))
                    {
                        if (pCPkg->IsPersisted(_pszWhere) != NOERROR)
                        {
                            pCPkg = 0;
                        }
                    //  DZhang fix
                        else if (pCPkg->GetTaskTrigger() == NULL)
                        {
                            pCPkg = 0;
                        }
                    //  End fix
                        else
                        {
                            pos = 0;
                        }
                    }
                    else
                    {
                        pCPkg = 0;
                    }

                }
                else
                {
                    pos = 0;
                }
                
            } while (pos);
        }
        
        break;
    } while (TRUE);
    
    if (pCPkg)
    {
        ADDREF(pCPkg);
        *ppCPackage = pCPkg;
        if (pPosCookie)
        {
            *pPosCookie = pCPkg->GetNotificationCookie();
        }
        
        hr = NOERROR;
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortPkgList::FindFirstSchedulePackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSortPkgList::RemoveSchedulePackage
//
//  Synopsis:   removes the object from the process list
//              and from the global table
//
//  Arguments:  [pCPackage] --
//              [dwMode] --
//
//  Returns:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CSortPkgList::RemoveSchedulePackage(
                        CPackage           *pCPackage,
                        DWORD               dwMode
                        )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortPkgList::RemoveSchedulePackage\n", this));
    NotfAssert((pCPackage && !dwMode));
    HRESULT hr = E_FAIL;

    CLock lck(_mxs);
    POSITION pos = _XLocalList.GetHeadPosition();
    CPackage *pCPkg = 0;

    while (pos)
    {
        pCPkg = _XLocalList.GetAt(pos).pCPkg;
        NotfAssert((pCPkg && _pszWhere));
        if (pCPkg->GetNotificationCookie() == pCPackage->GetNotificationCookie())
        {
            // the package is still in the list
            pCPkg->RemovePersist(_pszWhere);
           
            _XLocalList.RemoveAt(pos);
            RELEASE(pCPkg);
            pCPkg = 0;
            pos = 0;
            hr = NOERROR;
        }
        else
        {
            pCPkg = _XLocalList.GetNext(pos).pCPkg;
        }
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortPkgList::RemoveSchedulePackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSortPkgList::GetPackageCount
//
//  Synopsis:   removes the object from the process list
//              and from the global table
//
//  Arguments:  [pCPackage] --
//              [dwMode] --
//
//  Returns:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CSortPkgList::GetPackageCount(ULONG          *pCount)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortPkgList::GetPackageCount\n", this));
    NotfAssert((pCount));
    HRESULT hr = E_FAIL;

    *pCount = GetCount();

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortPkgList::GetPackageCount (hr:%lx)\n",this, hr));
    return hr;
}

HRESULT CSortPkgList::OnWakeup(WAKEUPTYPE wt, CPackage *pCPackage)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortPkgList::OnWakeup\n", this));
    HRESULT hr = E_FAIL;

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortPkgList::OnWakeup (hr:%lx)\n",this, hr));
    return hr;
}

#if DBG == 1
void CSortPkgList::Dump(DWORD dwFlags, const char *pszPrefix, HRESULT hr)
{
    if (!(TNotfInfoLevel & dwFlags))
        return;     //  no point in wasting time here

    POSITION pos;
    int      i = 0;
    TCHAR szIdx[MAX_PATH];

    CLock lck(_mxs);

    pos = _XLocalList.GetHeadPosition();

    TNotfDebugOut((dwFlags, "%s 0x%08x\n", pszPrefix, hr));
    
    while (pos)
    {
        wsprintf(szIdx, "[%3d]", i++);

        PPKG_DUMP(_XLocalList.GetAt(pos).pCPkg, (dwFlags, szIdx));
        _XLocalList.GetNext(pos);

    }
}
#endif

