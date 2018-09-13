//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:	sinklist.hxx
//
//  Contents:	Linked list class
//
//  Classes:	CSinkList
//				CConnectionPoint	
//
//  Functions:	
//
//  History:	24-Dec-95	SusiA	Created
//
//----------------------------------------------------------------------------

#ifndef __SINKLIST_HXX__
#define __SINKLIST_HXX__

#include "intfy.h"
//+---------------------------------------------------------------------------
//
//  Class:	CSinkList
//
//  Purpose:	Generic linked list class for use by async docfiles
//
//  Interface:	
//
//  History:	24-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------




class CSinkList
{
public:
    inline CSinkList();
    
    inline CSinkList *GetNext(void);
    inline void SetNext(CSinkList *psl);

    inline DWORD GetCookie(void);
    inline void SetCookie(DWORD dwCookie);
 
    inline IProgressNotify *GetProgressNotify(void);
    inline void SetProgressNotify(IProgressNotify *ppn);
   
   private:
   
    IProgressNotify *_ppn;
    DWORD _dwCookie;
    CSinkList *_pslNext;
};


inline CSinkList::CSinkList()
{
    _ppn = NULL;
    _dwCookie = 0;
    _pslNext = NULL;
    
}

inline CSinkList * CSinkList::GetNext(void)
{
    return _pslNext;
}

inline void CSinkList::SetNext(CSinkList *psl)
{
    _pslNext = psl;
}

inline DWORD CSinkList::GetCookie(void)
{
    return _dwCookie;
}

inline void CSinkList::SetCookie(DWORD dwCookie)
{
    _dwCookie = dwCookie;
}

inline IProgressNotify *CSinkList::GetProgressNotify(void)
{
    return _ppn;
}

inline void CSinkList::SetProgressNotify(IProgressNotify *ppn)
{
    _ppn = ppn;
}
//+---------------------------------------------------------------------------
//
//  Class:	CConnectionPoint
//
//  Purpose:	
//
//  Interface:	
//
//  History:	28-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------
class CConnectionPoint: public IConnectionPoint
{
public:
    CConnectionPoint();
    void Init(IConnectionPointContainer *pCPC);

    //From IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    //From IConnectionPoint
    STDMETHOD(GetConnectionInterface)(IID *pIID);
    STDMETHOD(GetConnectionPointContainer)
        (IConnectionPointContainer ** ppCPC);
    STDMETHOD(Advise)(IUnknown *pUnkSink, DWORD *pdwCookie);
    STDMETHOD(Unadvise)(DWORD dwCookie);
    STDMETHOD(EnumConnections)(IEnumConnections **ppEnum);

    inline CSinkList *GetHead(void);
    
private:
    DWORD _dwCookie;
    LONG _cReferences;
    CSinkList *_pSinkHead;
    IConnectionPointContainer *_pCPC;
};


inline CSinkList * CConnectionPoint::GetHead(void)
{
    return _pSinkHead;
}

#endif // #ifndef __SINKLIST_HXX__


