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


//+---------------------------------------------------------------------------
//
//  Method:     CSortItemList::HandlePackage
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
HRESULT CSortItemList::HandlePackage(CPackage *pCPackage)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortItemList::HandlePackage\n", this));

    HRESULT hr = E_FAIL;
    NotfAssert((pCPackage));
    NotfAssert((pCPackage->GetNotification()));
    
    CLock lck(_mxs);

    //
    // make sure that there are not duplicated items in the list
    //
    POSITION pos = _XLocalList.GetHeadPosition();

    while (pos)
    {
        if (_XLocalList.GetAt(pos).pCPkg == pCPackage)
        {
            RELEASE(pCPackage);
            _XLocalList.RemoveAt(pos);
            break;
        }
        _XLocalList.GetNext(pos);
    }

    PKG_ARG newElement;
    newElement.date = pCPackage->GetSortDate(_SortOrder);
    newElement.pCPkg = pCPackage;

    // set the notification state to waiting
    //pCPackage->SetNotificationState(pCPackage->GetNotificationState() | _StateFlags);
    TransAssert(( pCPackage->GetNotificationState() | _StateFlags ));

    //
    // start at the tail and loop forward
    //
    pos = _XLocalList.GetTailPosition();

    if (pos)
    {
        PKG_ARG arg = _XLocalList.GetAt(pos);

        while (pos)
        {
            // check if current position is smaller if so
            if (arg.date < newElement.date)
            {
                hr = NOERROR;       //BUGBUG - What's this all about?
                if (hr == NOERROR)  //         ??????
                {
                    // found the place
                    POSITION posInsert = _XLocalList.InsertAfter(pos, newElement);
                    ADDREF(pCPackage);
                    _cElements++;
                    pos = 0;
                }
                else
                {
                    // stop looping!
                    pos = 0;
                }
            }
            else
            {
                arg = _XLocalList.GetPrev(pos); // return *Position--
            }
        }
    }

    // did not find the place
    // add the element at the front
    //
    if ((hr != NOERROR) && !pos)
    {
        pos = _XLocalList.AddHead(newElement);
        ADDREF(pCPackage);
        _cElements++;
        hr = NOERROR;
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortItemList::HandlePackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSortItemList::RevokePackage
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
HRESULT CSortItemList::RevokePackage(
                        PNOTIFICATIONCOOKIE packageCookie,
                        CPackage           **ppCPackage,
                        DWORD               dwMode
                        )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortItemList::RevokePackage\n", this));
    NotfAssert((!dwMode));
    HRESULT hr = E_FAIL;

    CLock lck(_mxs);
    POSITION pos = _XLocalList.GetHeadPosition();
    CPackage *pCPkg = 0;

    while (pos)
    {
        pCPkg = _XLocalList.GetAt(pos).pCPkg;
        NotfAssert((pCPkg && _pszWhere));
        if (pCPkg->GetNotificationCookie() == *packageCookie)
        {
            // the package is still in the list
            _XLocalList.RemoveAt(pos);

            if (ppCPackage)
            {
                *ppCPackage = pCPkg;
            }
            else
            {
                RELEASE(pCPkg);
            }
            pCPkg = 0;
            pos = 0;
            hr = NOERROR;
        }
        else
        {
            pCPkg = _XLocalList.GetNext(pos).pCPkg;
        }
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortItemList::RevokePackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSortItemList::FindPackage
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
HRESULT CSortItemList::FindPackage(
                            PNOTIFICATIONCOOKIE pPkgCookie,
                            CPackage          **ppCPackage,
                            DWORD               dwMode
                            )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortItemList::FindPackage\n", this));
    NotfAssert((pPkgCookie && ppCPackage));

    HRESULT hr = E_FAIL;
    CLock lck(_mxs);
    CPackage *pCPkg = 0;

    do
    {
        //
        // find the local package 
        // 
        POSITION pos = _XLocalList.GetHeadPosition();

        if (!pos)
        {
            break;
        }
        
        pCPkg = _XLocalList.GetAt(pos).pCPkg;

        if (pCPkg && pCPkg->GetNotificationCookie() == *pPkgCookie)
        {
            // done found the package
            break;
        }
        pCPkg = 0;

        if (!pos)
        {
            break;
        }
        
        do 
        {
            NotfAssert((pos));
            
            // loop until the package is found
            pCPkg = _XLocalList.GetNext(pos).pCPkg;
            
            if (pCPkg && pCPkg->GetNotificationCookie() == *pPkgCookie)
            {
                // found the package
                pos = 0;
            }
            else
            {
                pCPkg = 0;
            }
        } while (pos && !pCPkg);

        break;
    } 
    while (TRUE);

    if (pCPkg)
    {
        ADDREF(pCPkg);
        *ppCPackage = pCPkg;
        hr = NOERROR;
    }
    else
    {
        hr = E_FAIL;
    }
    

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortItemList::FindPackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSortItemList::FindFirstPackage
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
HRESULT CSortItemList::FindFirstPackage(
                    PNOTIFICATIONCOOKIE pPosCookie,
                    CPackage          **ppCPackage,
                    DWORD               dwMode
                    )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortItemList::FindFirstPackage\n", this));
    NotfAssert((ppCPackage));

    HRESULT hr = E_FAIL;
    CPackage *pCPkg = 0;

    do
    {
        CLock lck(_mxs);

        if (   !pPosCookie
            || !ppCPackage)
        {
            break;
        }
        POSITION pos = _XLocalList.GetHeadPosition();

        if (!pos)
        {
            break;
        }

        pCPkg = _XLocalList.GetAt(pos).pCPkg;

        if (pCPkg)
        {
            if ((pCPkg->GetNotificationState() | _StateFlags) )
            {   
                if (  !(dwMode & EF_NOPERSISTCHECK)
                    && (pCPkg->IsPersisted(_pszWhere) != NOERROR))
                {
                    pCPkg = 0;
                }
            }
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
                    if ((pCPkg->GetNotificationState() | _StateFlags) )
                    {
                        if (  !(dwMode & EF_NOPERSISTCHECK)
                            && (pCPkg->IsPersisted(_pszWhere) != NOERROR))
                        {
                            pCPkg = 0;
                        }
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
        *pPosCookie = pCPkg->GetNotificationCookie();
        hr = NOERROR;
    }
    else
    {
        hr = E_FAIL;
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortItemList::FindFirstPackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSortItemList::FindNextPackage
//
//  Synopsis:
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
HRESULT CSortItemList::FindNextPackage(
                    PNOTIFICATIONCOOKIE pPosCookie,
                    CPackage          **ppCPackage,
                    DWORD               dwMode
                    )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortItemList::FindNextPackage\n", this));
    NotfAssert((ppCPackage));

    HRESULT hr = E_FAIL;
    CPackage *pCPkg = 0;

    do
    {
        CLock lck(_mxs);

        if (   !ppCPackage
            || !pPosCookie
            || (*pPosCookie == COOKIE_NULL))
        {
            break;
        }
        
        POSITION pos = _XLocalList.GetHeadPosition();

        if (!pos)
        {
            break;
        }

        pCPkg = _XLocalList.GetAt(pos).pCPkg;

        do 
        {
            pCPkg = _XLocalList.GetNext(pos).pCPkg;

            if (pCPkg && pCPkg->GetNotificationCookie() == *pPosCookie)
            {
                hr = NOERROR;
            }
        
        } while (pos && (hr != NOERROR));

        
        pCPkg = 0;
        
        if (!pos)
        {
            // end of list
            *pPosCookie = COOKIE_NULL;
            hr = E_FAIL;
            break;
        }

        //
        // now we are at the last position
        //
        do 
        {   
            // loop until next correct package
            if (pos)
            {
                pCPkg = _XLocalList.GetNext(pos).pCPkg;
            }
            
            if (pCPkg)
            {
                // the package is still in the list
                if (   (pCPkg->GetNotificationCookie() != *pPosCookie)
                    && (pCPkg->GetNotificationState() | _StateFlags) )
                {
                    // check if it is still persisted
                    if (  !(dwMode & EF_NOPERSISTCHECK)
                        && (pCPkg->IsPersisted(_pszWhere) != NOERROR))
                    {
                        pCPkg = 0;
                    }
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
        
        break;
    } while (TRUE);
    
    if (pCPkg)
    {
        ADDREF(pCPkg);
        *ppCPackage = pCPkg;
        *pPosCookie = pCPkg->GetNotificationCookie();
        hr = NOERROR;
    }
    else
    {
        hr = E_FAIL;
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortItemList::FindNextPackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSortItemList::GetPackageCount
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
HRESULT CSortItemList::GetPackageCount(ULONG          *pCount)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortItemList::GetPackageCount\n", this));
    NotfAssert((pCount));
    HRESULT hr = E_FAIL;

    *pCount = GetCount();

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortItemList::GetPackageCount (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSortItemList::OnWakeup
//
//  Synopsis:   called on wakeup; also sets the next wakeup time
//
//  Arguments:  [wt] --
//
//  Returns:
//
//  History:    1-06-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CSortItemList::OnWakeup(WAKEUPTYPE wt, CPackage *pCPackageWakeup)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortItemList::OnWakeup\n", NULL));
    HRESULT hr = E_FAIL;

    NotfAssert((FALSE));

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortItemList::OnWakeup (hr:%lx)\n",this, hr));
    return hr;
}

#if DBG == 1
void CSortItemList::Dump(DWORD dwFlags, const char *pszPrefix, HRESULT hr)
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


