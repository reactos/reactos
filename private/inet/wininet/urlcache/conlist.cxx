/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:  conlist.cxx

Abstract:

    Linked list of URL_CONTAINERs
    
Author:
    Adriaan Canter (adriaanc) 04-02-97
    
--*/

#include <cache.hxx>

/*------------------------ CConElem -----------------------------------------*/

/*-----------------------------------------------------------------------------
CConElem constructor
  ---------------------------------------------------------------------------*/
CConElem::CConElem(URL_CONTAINER* pUrlCon)
{
    _pUrlCon = pUrlCon;
    _pNext = NULL;
}

/*-----------------------------------------------------------------------------
CConElem destructor. Destructs URL_CONTAINER* member.
  ---------------------------------------------------------------------------*/
CConElem::~CConElem()
{
    delete _pUrlCon;
}


/*------------------------ CConList Private Functions------------------------*/


/*-----------------------------------------------------------------------------
CConList::Seek      Sets current pointer to element of index nElem.
  ---------------------------------------------------------------------------*/
BOOL CConList::Seek(DWORD nElem)
{   
    // Bad list or index too high.
    if (!_pHead || nElem > _n)
    {
        INET_ASSERT(FALSE);
        return FALSE;
    }

    // Seek to element from current.
    if (nElem > _nCur)        
    {
        while (_nCur < nElem)
        {
            _pCur = _pCur->_pNext;
            _nCur++;
        }
    }

//
// BUGBUG: VC5 optimizer assumes if (a < b), then (b > a), so check (a != b) instead
//
    else if (nElem != _nCur) // if (nElem < _nCur)
    {
        // Seek to element from head.
        _nCur = 0;
        _pCur = _pHead;
        while (_nCur < nElem)
        {
            _pCur = _pCur->_pNext;
            _nCur++;
        }
    }

    INET_ASSERT(_nCur != 0 || (_pCur == _pHead));

    return TRUE;
}


/*------------------------ CConList Public Functions------------------------*/


/*-----------------------------------------------------------------------------
CConList constructor.
  ---------------------------------------------------------------------------*/
CConList::CConList()
: _n(0), _nCur(0), _pCur(NULL), _pHead(NULL)
{
}

/*-----------------------------------------------------------------------------
CConList destructor.
  ---------------------------------------------------------------------------*/
CConList::~CConList()
{
}

/*-----------------------------------------------------------------------------
CConList::Size      Returns number of elements in list.
  ---------------------------------------------------------------------------*/
DWORD CConList::Size()
// THIS FUNCTION MUST BE CALLED WITH THE CACHE CRIT SEC
{
    DWORD n = (_pHead ? _n+1 : 0);
    return n;
}

/*-----------------------------------------------------------------------------
CConList::Free      Removes and destructs each element in list.
  ---------------------------------------------------------------------------*/
BOOL CConList::Free()
{
    LOCK_CACHE();

    DWORD i = Size();

    //  Delete CONTENT last, as we reference fields of it's header (dwChangeCount)
    //  in destructors of extensible containers
    while (i)
    {
        Remove(--i);
    }
    UNLOCK_CACHE();
    return TRUE;
}


/*-----------------------------------------------------------------------------
CConList::Add      Appends new element to list.
  ---------------------------------------------------------------------------*/
BOOL CConList::Add(URL_CONTAINER * pUrlCon)
{
    LOCK_CACHE();
    BOOL bSuccess = FALSE;
    CConElem *pNew;
    DWORD i;
    
    // Bad pointer.
    if (!pUrlCon)
    {
        INET_ASSERT(FALSE);
        goto exit;        
    }

    if (_pHead)
    {
        //  try to reuse a Container which has been deleted
        for (i = 0; i <= _n; i++)
        {
            if (Seek(i))
            {
                if (_pCur->_pUrlCon->GetDeleted())
                {
                    delete _pCur->_pUrlCon;
                    _pCur->_pUrlCon = pUrlCon;
                    bSuccess = TRUE;
                    goto exit;
                }
            }
        }
    }

    // Construct new element.
    pNew = new CConElem(pUrlCon);

    if (!pNew)
    {
        INET_ASSERT(FALSE);
        goto exit;
    }

    // If valid list, seek to last element and add element.
    if (_pHead)
    {
        if (_n == LARGEST_INDEX)
        {
            delete pNew;
            INET_ASSERT(FALSE);
            goto exit;        
        }
        Seek(_n);
        _pCur->_pNext = pNew;
        pNew->_pNext = _pHead;
        _n++;
    }
    // If empty list, set head and current to new element.
    else
    {
        _pHead = _pCur = pNew;
        pNew->_pNext = _pHead;
        _n = _nCur = 0;
    }
    
    bSuccess = TRUE;
exit:
    
    UNLOCK_CACHE();
    return bSuccess;
}

/*-----------------------------------------------------------------------------
CConList::Remove      Removes nElem'th element from list.
  ---------------------------------------------------------------------------*/
BOOL CConList::Remove(DWORD nElem)
// THIS FUNCTION MUST BE CALLED WITH THE CACHE CRIT SEC
{
    DWORD     nPrev;
    CConElem *pElem;
    BOOL bSuccess = FALSE;

    // Empty list or index too high.
    if (!_pHead || nElem > _n)
    {
        INET_ASSERT(FALSE);
        goto exit;
    }

    // Seek to previous element, or last if removing head.
    nPrev = (nElem == 0 ? _n : nElem - 1);
    Seek(nPrev);

    // Save pointer to element, update prevous' next pointer.
    pElem = _pCur->_pNext;
    _pCur->_pNext = _pCur->_pNext->_pNext;

    // Update head if necessary.
    if (nElem == 0)
        _pHead = _pHead->_pNext;

    // Decrement index of last, zero out values if empty.
    if (_n > 0)
        _n--;
    else
    {
        _pHead = _pCur = NULL;
        _n = _nCur = 0;
    }    
    
    // Destruct element.
    delete pElem;
    
    bSuccess = TRUE;
exit:
    return bSuccess;
}

/*-----------------------------------------------------------------------------
CConList::operator Get Returns Addref'ed reference to URL_CONTAINER* of index nElem.
  ---------------------------------------------------------------------------*/
// THIS FUNCTION MUST BE CALLED WITH THE CACHE CRIT SEC
URL_CONTAINER* CConList::Get (DWORD nElem)
{
    URL_CONTAINER* pUrlCon;
    if (Seek(nElem))
        pUrlCon = _pCur->_pUrlCon;
    else
        pUrlCon = NULL;
        
    if (pUrlCon) pUrlCon->AddRef();
    return pUrlCon;
}
