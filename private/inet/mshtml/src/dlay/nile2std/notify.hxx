//+----------------------------------------------------------------------------
// notify.hxx:
//              CNileCP     - IConnectionPoint implementation
//
// Copyright: (c) 1994-1995, Microsoft Corporation
// Information contained herein is Proprietary and Confidential.
//
// Contents: this file contains the above refered to definitions.
//
//  History:
//  08/07/95    TerryLu     Creation of IConnectionPoint interface.
//

#ifndef __NOTIFY_HXX_
#define __NOTIFY_HXX_


//+----------------------------------------------------------------------------
//
//  Class CNileCP
//
//  Purpose:
//      IConnectionPoint interface
//
//  Currently contains:

class CNileCP : public CConnectionPoint
{
public:
    CNileCP (IConnectionPointContainer *pCPC, IID iid);

    //  IUnknown members
    STDMETHODIMP            QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IConnectionPoint members
    STDMETHODIMP    GetConnectionInterface (IID *piid);
    STDMETHODIMP    GetConnectionPointContainer (IConnectionPointContainer **);
    // Advise, Unadvise, and EnumConnections are implemented by CConnectionPoint

    //  Overrides
    virtual HRESULT QuerySink(IUnknown *pUnk, void **ppvSink);

    // Event firing helper functions:
#if defined(PRODUCT_97)
    HRESULT     FireChapterEvent(IRowset *pRowset,
                                 HCHAPTER hChapter,
                                 DBREASON eReason);
#endif
    HRESULT     FireFieldEvent(IRowset *pRowset, HROW hRow,
    							ULONG cColumns, ULONG aColumns,
    							DBREASON eReason, DBEVENTPHASE ePhase);
    HRESULT     FireRowEvent(IRowset *pRowset,
                             ULONG cRows, const HROW rghRows[],
                             DBREASON eReason, DBEVENTPHASE ePhase);
    HRESULT     FireRowsetEvent(IRowset *pRowset, DBREASON eReason,
    							DBEVENTPHASE ePhase);

private:
    CNileCP ()                              // Hide default constructor.
        {   }
    ~CNileCP ();                            // Hide the destructor use Release.

    ULONG                       _cRef;      // reference count
    IID                         _iid;       // IID supported for this CP
    IConnectionPointContainer   *_pCPC;     // Container associate with this CP
};

#endif  // __NOTIFY_HXX_

