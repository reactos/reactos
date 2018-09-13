//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       sortpkgl.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#ifndef _SORTPKGLIST_HXX_
#define _SORTPKGLIST_HXX_


class CSortPkgList : public CListAgent
{
protected:
    struct PKG_ARG
    {
        CFileTime       date;
        CPackage *      pCPkg;
    };
private:

    CPackage * RemoveTail()
    {
        CLock lck(_mxs);
        NotfAssert(( _cElements && (_cElements ==  _XLocalList.GetCount()) ));

        _cElements--;
        return _XLocalList.RemoveTail().pCPkg;
    }

    POSITION GetTailPosition()
    {
        CLock lck(_mxs);
        return _XLocalList.GetTailPosition();
    }
    CPackage * GetPrev(POSITION& rPosition) const // return *Position--
    {
        return _XLocalList.GetPrev(rPosition).pCPkg;
    }
    CPackage * GetAt(POSITION position) const
    {
        return _XLocalList.GetAt(position).pCPkg;
    }

    CPackage * RemoveHead()
    {
        CLock lck(_mxs);
        //BUGBUG: the following assertion fire with urlbind -A test
        // need to investigate 
        //NotfAssert(( _cElements && (_cElements ==  _XLocalList.GetCount()) ));

        _cElements--;
        return _XLocalList.RemoveHead().pCPkg;
    }

    void RemoveAt(POSITION position)
    {
        CLock lck(_mxs);
        NotfAssert(( _cElements && (_cElements ==  _XLocalList.GetCount()) ));
        NotfAssert((position));

        _cElements--;
        _XLocalList.RemoveAt(position);
    }

    // iteration
    POSITION GetHeadPosition()
    {
        CLock lck(_mxs);
        return _XLocalList.GetHeadPosition();
    }

    BOOL IsEmpty()
    {
        CLock lck(_mxs);
        return GetCount() ? FALSE : TRUE;
    }
    HRESULT Synchronize(BOOL fForceResync = FALSE);
    BOOL IsSynchronized()
    {
        return !((BOOL)_rCNotfMgr.IsScheduleItemSyncd(_dateLastSync,_XLocalList.GetCount() ));
    }

public:
    virtual HRESULT HandlePackage(
                          CPackage    *pCPackage);

    virtual HRESULT RevokePackage(
                          PNOTIFICATIONCOOKIE packageCookie,
                          CPackage          **ppCPackage,
                          DWORD               dwMode
                          );
    virtual HRESULT OnWakeup(WAKEUPTYPE wt, CPackage *pCPackage = 0);

    virtual HRESULT FindFirstPackage(
                             PNOTIFICATIONCOOKIE packageCookie,
                             CPackage          **ppCPackage,
                             DWORD               dwMode
                             );

    virtual HRESULT FindNextPackage(
                            PNOTIFICATIONCOOKIE packageCookie,
                            CPackage          **ppCPackage,
                            DWORD               dwMode
                            );

    virtual HRESULT FindPackage(
                        PNOTIFICATIONCOOKIE packageCookie,
                        CPackage          **ppCPackage,
                        DWORD               dwMode
                        );

    virtual HRESULT GetPackageCount(ULONG          *pCount);
 
    //
    // no virtual functions
    //
    HRESULT FindFirstSchedulePackage(
                        PNOTIFICATIONCOOKIE pPosCookie,
                        CPackage          **ppCPackage,
                        DWORD               dwMode
                        );
    HRESULT RemoveSchedulePackage(
                        CPackage           *pCPackage,
                        DWORD               dwMode
                        );
                        

    // persiststream methods
    STDMETHODIMP GetClassID (CLSID *pClassID);
    STDMETHODIMP IsDirty(void);
    STDMETHODIMP Load(IStream *pStm);
    STDMETHODIMP Save(IStream *pStm,BOOL fClearDirty);
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);

public:
    ULONG GetCount()
    {
        CLock lck(_mxs);
        ULONG uCount;

        _rCNotfMgr.GetScheduleItemCount(&uCount);

        return uCount;
    }


    CSortPkgList(CGlobalNotfMgr &rCNotfMgr, LPCSTR pszWhere)
        : _rCNotfMgr(rCNotfMgr)
        , _pszWhere(pszWhere)
        , _cElements(0)
    {
        if (!_pszWhere)
        {
            _pszWhere = c_pszRegKey;
        }
    }

    ~CSortPkgList()
    {
#if DBG == 1
        if (_XLocalList.GetCount())
        {
            PLIST_DUMP(this, (DEB_TMEMORY, "CSortPkgList::~CSortPkgList - the following CPackages have leaked:\n"));
        }
#endif
        NotfAssert(((_cElements == 0) && (_XLocalList.GetCount() == 0)));
    }

#if DBG == 1
    void Dump(DWORD dwFlags, const char *pszPrefix, HRESULT hr = 0xffffffff);
#endif

private:
    CRefCount       _cElements;     // # of elements
    POSITION        _pos;
    CFileTime       _dateLastSync;  // date of last ssync
    BOOL            _fDirty;
    
    CList<PKG_ARG, PKG_ARG &>                 _XLocalList;
    CSortList<CFileTime,SL_ITEM , SL_ITEM &>   _XDistList;
    CGlobalNotfMgr                           &_rCNotfMgr;
    
protected:
    LPCSTR          _pszWhere;

};  // CSortPkgList



class CSortItemList : public CSortPkgList
{
public:
    virtual HRESULT HandlePackage(
                CPackage    *pCPackage);

    virtual HRESULT RevokePackage(
                                  PNOTIFICATIONCOOKIE packageCookie,
                                  CPackage          **ppCPackage,
                                  DWORD               dwMode
                                  );

    virtual HRESULT OnWakeup(WAKEUPTYPE wt, CPackage *pCPackage = 0);

    virtual HRESULT FindFirstPackage(
                             PNOTIFICATIONCOOKIE packageCookie,
                             CPackage          **ppCPackage,
                             DWORD               dwMode
                             );

    virtual HRESULT FindNextPackage(
                            PNOTIFICATIONCOOKIE packageCookie,
                            CPackage          **ppCPackage,
                            DWORD               dwMode
                            );

    virtual HRESULT FindPackage(
                        PNOTIFICATIONCOOKIE packageCookie,
                        CPackage          **ppCPackage,
                        DWORD               dwMode
                        );

    virtual HRESULT GetPackageCount(ULONG          *pCount);
   
    CSortItemList(CGlobalNotfMgr &rCNotfMgr, SORTORDER SortOrder, DWORD dwStateFlags, LPCSTR pszWhere) 
        : CSortPkgList(rCNotfMgr, pszWhere)
    {
        _wt = WT_USERIDLE;
        _SortOrder = SortOrder;
        _StateFlags = dwStateFlags;

    }
    
    virtual ~CSortItemList()
    {}

#if DBG == 1
    void Dump(DWORD dwFlags, const char *pszPrefix, HRESULT hr = 0xffffffff);
#endif

private:
    CRefCount                                   _cElements;
    CList<PKG_ARG, PKG_ARG &>                   _XLocalList;
    SORTORDER                                   _SortOrder;
    DWORD                                       _StateFlags;

};  // CSortItemList

class CThrottleListAgent : public CSortItemList
{
public:
    virtual HRESULT HandlePackage(
                CPackage    *pCPackage);

    virtual HRESULT RevokePackage(
                                  PNOTIFICATIONCOOKIE packageCookie,
                                  CPackage          **ppCPackage,
                                  DWORD               dwMode
                                  );
    virtual HRESULT OnWakeup(WAKEUPTYPE wt, CPackage *pCPackage = 0);

    
    CThrottleListAgent(CGlobalNotfMgr &rCNotfMgr, SORTORDER SortOrder, DWORD dwStateFlags, LPCSTR pszWhere = 0) 
        : CSortItemList(rCNotfMgr, SortOrder, dwStateFlags, pszWhere)
        , _cWaiting(0)
    {
        _wt = WT_USERIDLE;
    }
    virtual ~CThrottleListAgent();

#if DBG == 1
    void Dump(DWORD dwFlags, const char *pszPrefix, HRESULT hr = 0xffffffff);
#endif

    // private helper methods
    HRESULT PrivateDeliverReport(NOTIFICATIONTYPE notfType, CPackage *pCPackage, DWORD dwMode);

private:
    CSortList<DWORD, CPackage *, CPackage *>    _XSortList;
    CRefCount                                   _cElements;
    CRefCount                                   _cWaiting;
};  // CThrottleListAgent


#endif // _SORTPKGLIST_HXX_

