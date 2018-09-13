//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       eventobj.hxx
//
//  Contents:   The eventobject class and (object model)
//
//-------------------------------------------------------------------------

#ifndef I_EVENTOBJ_HXX_
#define I_EVENTOBJ_HXX_
#pragma INCMSG("--- Beg 'eventobj.hxx'")

#define _hxx_
#include "eventobj.hdl"

MtExtern(CEventObj)
MtExtern(CDataTransfer)

class CEventObj;
class COmWindow2;
class CDoc;

//+------------------------------------------------------------------------
//
//  Class:      CDataTransfer
//
//  Purpose:    The data transfer object
//
//-------------------------------------------------------------------------

class CDataTransfer : public CBase
{
    DECLARE_CLASS_TYPES(CDataTransfer, CBase)

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDataTransfer))

    CDataTransfer(CDoc *pDoc, IDataObject * pDataObj, BOOL fDragDrop);
    ~CDataTransfer();

    // CBase methods
    DECLARE_PLAIN_IUNKNOWN(CDataTransfer)
    DECLARE_PRIVATE_QI_FUNCS(CBase)
    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}

    #define _CDataTransfer_
    #include "eventobj.hdl"

    // helper functions

    // Data members
    CDoc *  _pDoc;
    IDataObject * _pDataObj;
    BOOL _fDragDrop;    // TRUE for window.event.dataTransfer, FALSE for window.clipboardData

    // Static members
    static const CLASSDESC s_classdesc;
};


//+------------------------------------------------------------------------
//
//  Class:      CEventObj
//
//  Purpose:    The tearoff event object for parameters of events
//
//-------------------------------------------------------------------------

class CEventObj : public CBase
{
    DECLARE_CLASS_TYPES(CEventObj, CBase)

    enum
    {
        EVENT_BOUND_ELEMENTS_COLLECTION = 0,
        NUMBER_OF_EVENT_COLLECTIONS = 1,
    };

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CEventObj))

    CEventObj(CDoc * pDoc);
    ~CEventObj();

    static HRESULT Create(
        IHTMLEventObj** ppEventObj,
        CDoc *          pDoc,
        BOOL            fCreateAttached = TRUE,
        LPTSTR          pchSrcUrn = NULL);

    class COnStackLock
    {
    public:
        COnStackLock(IHTMLEventObj*  pEventObj);
        ~COnStackLock();

        IHTMLEventObj *  _pEventObj;
        CEventObj *      _pCEventObj;
    };

    HRESULT SetAttributes(CDoc * pDoc);

    HRESULT GetParam(EVENTPARAM ** ppParam);

    // CBase methods
    DECLARE_PLAIN_IUNKNOWN(CEventObj)
    DECLARE_PRIVATE_QI_FUNCS(CBase)

    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}

    virtual CAtomTable * GetAtomTable (BOOL * pfExpando = NULL)
        {
            if (pfExpando)
            {
                *pfExpando = _pDoc->_fExpando;
            }
            return &_pDoc->_AtomTable;
        }

    #define _CEventObj_
    #include "eventobj.hdl"

    // collection functions
    NV_DECLARE_ENSURE_METHOD(EnsureCollections, ensurecollections, (long,long * plCollectionVersion));
    HRESULT         EnsureCollectionCache();

    // Data members
    CDoc *                              _pDoc;
    EVENTPARAM *                        _pparam;
    CCollectionCache *                  _pCollectionCache;

    BOOL                                _fReadWrite;    // TRUE if we allow to change all the event properties
                                                        // we do that for xTag events.
    
    // Static members
    static const CLASSDESC                    s_classdesc;

private:
    HRESULT PutUnknownPtr(DISPID dispid, IUnknown *pElement);
    HRESULT GetUnknownPtr(DISPID dispid, IUnknown **ppElement);

    HRESULT PutDispatchPtr(DISPID dispid, IDispatch *pElement);
    HRESULT GetDispatchPtr(DISPID dispid, IDispatch **ppElement);

    HRESULT GenericGetElement (IHTMLElement** ppElement, DISPID dispid, ULONG uOffset);
    HRESULT GenericPutElement (IHTMLElement* pElement, DISPID dispid);

    HRESULT GenericGetLong (long * pLong, ULONG uOffset);
};



#pragma INCMSG("--- End 'eventobj.hxx'")
#else
#pragma INCMSG("*** Dup 'eventobj.hxx'")
#endif
