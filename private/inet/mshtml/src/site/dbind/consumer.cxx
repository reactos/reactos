//+---------------------------------------------------------------------------
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1996-1996
//  All rights Reserved.
//  Information contained herein is Proprietary and Confidential.
//
//  Contents:   Data Consumer objects
//
//  Classes:    CDataSourceBinder::CConsumer (abstract)
//                  CNullConsumer
//                  CTableConsumer
//                  CCurrentRecordConsumer
//                  CRowsetConsumer
//                  CCursorConsumer
//
//  History:    10/1/96     (sambent) created

// The concrete classes derived from CDataSourceBinder::CConsumer are
// declared in this file, which makes them unknown to the rest of the world.
// The function CConsumer::Create acts as a
// factory -- creating the appropriate concrete consumer or provider.
//
// To support a new type of consumer, derive a new class from CConsumer, and
// add code to CConsumer::Create to create an instance.


#include <headers.hxx>

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include <formkrnl.hxx>     // for safetylevel in safety.hxx (via olesite.hxx)
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include <olesite.hxx>
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

#ifndef X_DBINDING_HXX_
#define X_DBINDING_HXX_
#include "dbinding.hxx"
#endif

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include "binder.hxx"
#endif

#ifndef X_ROWBIND_HXX_
#define X_ROWBIND_HXX_
#include "rowbind.hxx"
#endif

#ifndef X_VBCURSOR_OCDB_H_
#define X_VBCURSOR_OCDB_H_
#include <vbcursor/ocdb.h>
#endif

#ifndef X_VBCURSOR_OCDBID_H_
#define X_VBCURSOR_OCDBID_H_
#include <vbcursor/ocdbid.h>
#endif

#ifndef X_VBCURSOR_VBDSC_H_
#define X_VBCURSOR_VBDSC_H_
#include <vbcursor/vbdsc.h>
#endif

#ifndef X_VBCURSOR_OLEBIND_H_
#define X_VBCURSOR_OLEBIND_H_
#include <vbcursor/olebind.h>
#endif

#ifndef X_MSDATSRC_H_
#define X_MSDATSRC_H_
#include <msdatsrc.h>
#endif

EXTERN_C const IID IID_IADORecordset15; // for GetDataMember from Java
EXTERN_C const IID IID_DataSource;      // for QI impl of CDataSourceConsumer

/////////////////////////////////////////////////////////////////////////////
/////                       Consumer Classes                            /////
/////////////////////////////////////////////////////////////////////////////

/////-------------------------------------------------------------------/////
///// null consumer.  Used when binding fails.  Always returns error.   /////
/////-------------------------------------------------------------------/////

MtDefine(CNullConsumer, DataBind, "CNullConsumer")

class CNullConsumer: public CDataSourceBinder::CConsumer
{
    typedef CDataSourceBinder::CConsumer super;
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CNullConsumer))
    CNullConsumer(CDataSourceBinder *pBinder) : super(pBinder) {}
    virtual HRESULT Bind() { return E_FAIL; }
    virtual HRESULT UnBind() { return E_FAIL; }
    virtual void    FireOnDataReady(BOOL fReady) {}
};


/////-------------------------------------------------------------------/////
/////                   Table consumer                                  /////
/////-------------------------------------------------------------------/////

MtDefine(CTableConsumer, DataBind, "CTableConsumer")

class CTableConsumer: public CDataSourceBinder::CConsumer
{
    typedef CDataSourceBinder::CConsumer super;
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CTableConsumer))
    CTableConsumer(CDataSourceBinder *pBinder):
        super(pBinder), _pdlcCursor(NULL), _bstrDataSrc(NULL), _bstrDataFld(NULL) {}
    virtual HRESULT Bind();
    virtual HRESULT UnBind();
    virtual HRESULT GetDLCursor(CDataLayerCursor **pdlc);
    virtual HRESULT GetDataSrcAndFld(BSTR *, BSTR *);
    virtual void    FireOnDataReady(BOOL fReady);
    virtual ~CTableConsumer() {}
private:
    CDataLayerCursor    *_pdlcCursor;   // my DLCursor
    BSTR                _bstrDataSrc;   // my full dataSrc
    BSTR                _bstrDataFld;   // my full dataFld
};


//+-------------------------------------------------------------------------
// Member:              Bind (CTableConsumer, public)
//
// Synopsis:    set up the desired binding

HRESULT
CTableConsumer::Bind()
{
    HRESULT hr = S_OK;
    CDataSourceProvider *pProvider = NULL;
    IRowset *pRowset = NULL;
    CTable *pTable = DYNCAST(CTable, GetElementConsumer());
    Assert(pTable);
    Assert(GetProvider());
    LPCTSTR strDataSrc = pTable->GetAAdataSrc();
    LPCTSTR strDataFld = pTable->GetAAdataFld();
    
    if (strDataFld == NULL)
    {
        // normal table
        pProvider = GetProvider();
        FormsAllocString(strDataSrc, &_bstrDataSrc);
    }
    else
    {
        // hierarchical table
        BSTR bstrColumn=NULL, bstrJunk;
        LPCTSTR strTail;
        CElement *pElemOuter, *pElemRepeat;
        CRecordInstance *pRecInstance = NULL;
        CDataSourceProvider *pProviderParent = NULL;
        
        // look for enclosing repeated table
        hr = pTable->FindDatabindContext(strDataSrc, strDataFld,
                                            &pElemOuter, &pElemRepeat, &strTail);

        // if there is one, use its provider as the source for a subprovider
        if (!hr && pElemOuter)
        {
            CTable *pTableOuter = DYNCAST(CTable, pElemOuter);
            CTableRow *pRow = DYNCAST(CTableRow, pElemRepeat);
            LPCTSTR strDataFldOuter = pTableOuter->GetDataFld();
            UINT cDataFldLength;
            LPTSTR pch;
            
            pProviderParent = pTableOuter->GetProvider();
            hr = pTableOuter->GetInstanceForRow(pRow, &pRecInstance);
            if (hr)
                goto CleanupHierarchy;
            
            FormsAllocString(pTableOuter->GetDataSrc(), &_bstrDataSrc);
            FormsSplitFirstComponent(strTail, &bstrColumn, &bstrJunk);
            FormsFreeString(bstrJunk);
            
            cDataFldLength = (strDataFldOuter ? _tcslen(strDataFldOuter) : 0) +
                              1 +
                              (strTail ? _tcslen(strTail) : 0) +
                              1;
            pch = new TCHAR[cDataFldLength];
            if (pch == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto CleanupHierarchy;
            }
                
            if (strDataFldOuter)
            {
                _tcscpy(pch, strDataFldOuter);
                _tcscat(pch, _T("."));
                _tcscat(pch, bstrColumn);
            }
            else
            {
                _tcscpy(pch, bstrColumn);
            }
            FormsAllocString(pch, &_bstrDataFld);
            
            delete pch;
        }
        
        // if not, perhaps the table is current-record bound to hierarchy
        else if (!hr && strTail)
        {
            CCurrentRecordInstance *pCRI = NULL;
            hr = GetProvider()->QueryDataInterface(IID_ICurrentRecordInstance,
                                        (void**)&pCRI);
            if (!hr)
            {
                FormsAllocString(strDataSrc, &_bstrDataSrc);
                FormsAllocString(strDataFld, &_bstrDataFld);
                FormsSplitFirstComponent(strTail, &bstrColumn, &bstrJunk);
                FormsFreeString(bstrJunk);
                
                pProviderParent = GetProvider();
                hr = pCRI->GetCurrentRecordInstance(&pRecInstance);
            }
            ReleaseInterface(pCRI);
        }

        // if we found a hierarchical context, use the subprovider
        if (!hr && pRecInstance)
        {
            Assert(pProviderParent);
            HROW hrow = pRecInstance->GetHRow();
            CXfer *pXfer;

            // if there's a good HROW, get the provider we'll use to bind
            if (hrow != DB_NULL_HROW)
            {
                hr = pProviderParent->GetSubProvider(&pProvider, bstrColumn, hrow);
                if (!hr)
                    SubstituteProvider(pProvider);
                if (pProvider)
                    pProvider->Release();       // ref now owned by _pProvider
            }

            if (!hr)
            {
                hr = CXfer::CreateBinding(pTable, ID_DBIND_DEFAULT, strTail,
                                        pProviderParent, pRecInstance,
                                        &pXfer, /* fDontTransfer */ TRUE);
            }
        }

CleanupHierarchy:
        FormsFreeString(bstrColumn);
    }

    if (pProvider == NULL)
    {
        hr = E_FAIL;
    }
    if (hr)
        goto Cleanup;
    
    // get the provider's rowset
    hr = pProvider->QueryDataInterface(IID_IRowset, (void**)&pRowset);
    if (hr)
        goto Cleanup;

    // set up my DLCursor to use the rowset
    _pdlcCursor = new CDataLayerCursor(NULL);
    if (!_pdlcCursor)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = _pdlcCursor->Init(pRowset, pProvider->GetChapter());
    if (hr == E_NOINTERFACE)
    {
        hr = E_FAIL;        // E_NOINTERFACE from Bind() means "null provider"
    }

Cleanup:
    ReleaseInterface(pRowset);
    pTable->SetReadyStateTable(hr==S_OK ? READYSTATE_LOADING : READYSTATE_COMPLETE);

    return hr;
}

//+-------------------------------------------------------------------------
// Member:              UnBind (CTableConsumer, public)
//
// Synopsis:    tear down existing binding

HRESULT
CTableConsumer::UnBind()
{
    CElement *pelConsumer = GetElementConsumer();
    CDataBindingEvents *pdbe = pelConsumer->GetDBMembers()->GetDataBindingEvents();

    Assert(pdbe && "need a pdbe");

    // for hierarchical tables, detach from the RecordInstance
    pdbe->DetachBinding(pelConsumer, ID_DBIND_ALL);

    // all I gotta do is tell my DLCursor to die
    if (_pdlcCursor)
    {
        _pdlcCursor->Release();
        _pdlcCursor = NULL;
    }

    FormsFreeString(_bstrDataSrc);
    FormsFreeString(_bstrDataFld);
    _bstrDataSrc = _bstrDataFld = NULL;
    
    return S_OK;
}


//+-------------------------------------------------------------------------
// Member:              GetDLCursor (CTableConsumer, public)
//
// Synopsis:    return my DLCursor

HRESULT
CTableConsumer::GetDLCursor(CDataLayerCursor **pdlc)
{
    Assert(pdlc);
    *pdlc = _pdlcCursor;
    return _pdlcCursor ? S_OK : E_FAIL;
}


//+-------------------------------------------------------------------------
// Member:              GetDataSrcAndFld (CTableConsumer, public)
//
// Synopsis:    return my full dataSrc and dataFld

HRESULT
CTableConsumer::GetDataSrcAndFld(BSTR *pbstrDataSrc, BSTR *pbstrDataFld)
{
    HRESULT hr;
    BSTR bstrDataSrc, bstrDataFld = NULL;   // this is just to appease the LINT
    
    hr = FormsAllocString(_bstrDataSrc, &bstrDataSrc);
    if (!hr)
    {
        hr = FormsAllocString(_bstrDataFld, &bstrDataFld);
        if (hr)
            FormsFreeString(bstrDataSrc);
    }

    if (!hr)
    {
        *pbstrDataSrc = bstrDataSrc;
        *pbstrDataFld = bstrDataFld;
    }
    else
    {
        *pbstrDataSrc = NULL;
        *pbstrDataFld = NULL;
    }
        
    
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
// Member:              FireOnDataReady (CTableConsumer, public)
//
// Synopsis:    tell my element the data's ready to eat

void
CTableConsumer::FireOnDataReady(BOOL fReady)
{
    CElement *pelConsumer = GetElementConsumer();

    Assert(pelConsumer->GetDBindMethods());
    pelConsumer->GetDBindMethods()->OnDataReady(pelConsumer, fReady);
}


/////-------------------------------------------------------------------/////
/////                   CCurrentRecordConsumer                          /////
/////-------------------------------------------------------------------/////

MtDefine(CCurrentRecordConsumer, DataBind, "CCurrentRecordConsumer")

class CCurrentRecordConsumer: public CDataSourceBinder::CConsumer
{
    typedef CDataSourceBinder::CConsumer super;
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CCurrentRecordConsumer))
    CCurrentRecordConsumer(CDataSourceBinder *pBinder) : super(pBinder) {}
    virtual HRESULT Bind();
    virtual HRESULT UnBind();
    virtual void    FireOnDataReady(BOOL fReady);
    virtual ~CCurrentRecordConsumer() {}
};


//+-------------------------------------------------------------------------
// Member:              Bind (CCurrentRecordConsumer, public)
//
// Synopsis:    set up the desired binding

HRESULT
CCurrentRecordConsumer::Bind()
{
    HRESULT hr;
    CDataSourceProvider *pProvider = GetProvider();
    CElement *pelConsumer = GetElementConsumer();
    CCurrentRecordInstance *pCRI = 0;
    CRecordInstance *priCurrent;

    Assert(pProvider);

    // get the provider's current record instance
    hr = pProvider->QueryDataInterface(IID_ICurrentRecordInstance,
                                        (void**)&pCRI);
    if (hr)
        goto Cleanup;
    hr = pCRI->GetCurrentRecordInstance(&priCurrent);
    if (hr)
        goto Cleanup;

    // attach an Xfer to the instance
    hr = CXfer::CreateBinding(pelConsumer, IdConsumer(), NULL, pProvider, priCurrent);
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pCRI);
    return hr;
}


//+-------------------------------------------------------------------------
// Member:              UnBind (CCurrentRecordConsumer, public)
//
// Synopsis:    tear down existing binding

HRESULT
CCurrentRecordConsumer::UnBind()
{
    CElement *pelConsumer = GetElementConsumer();
    CDataBindingEvents *pdbe = pelConsumer->GetDBMembers()->GetDataBindingEvents();

    Assert(pdbe && "need a pdbe");

    pdbe->DetachBinding(pelConsumer, IdConsumer());

    return S_OK;
}


//+-------------------------------------------------------------------------
// Member:              FireOnDataReady (CCurrentRecordConsumer, public)
//
// Synopsis:    tell my element the data's ready to eat

void
CCurrentRecordConsumer::FireOnDataReady(BOOL fReady)
{
    // nothing to do - once we've attached the Xfer to the instance,
    // the Xfer does all the work.  [CurrentRecord consumers use
    // "push model" data binding.]
    // The function does have to be here, so that this is an instantiable
    // class.
}


/////-------------------------------------------------------------------/////
/////                        CCursorConsumer                            /////
/////-------------------------------------------------------------------/////

MtDefine(CCursorConsumer, DataBind, "CCursorConsumer")

class CCursorConsumer: public CDataSourceBinder::CConsumer
{
    typedef CDataSourceBinder::CConsumer super;
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CCursorConsumer))
    CCursorConsumer(CDataSourceBinder *pBinder): super(pBinder), _pCursor(0) {}
    virtual HRESULT Bind();
    virtual HRESULT UnBind();
    virtual void    FireOnDataReady(BOOL fReady);
    virtual HRESULT GetICursor(ICursor **ppCursor);
    virtual ~CCursorConsumer() {}
private:
    ICursor     *_pCursor;      // my ICursor interface (from provider)
};


//+-------------------------------------------------------------------------
// Member:              Bind (CCursorConsumer, public)
//
// Synopsis:    set up the desired binding

HRESULT
CCursorConsumer::Bind()
{
    HRESULT hr;
    CDataSourceProvider *pProvider = GetProvider();

    Assert(pProvider);
    Assert(!_pCursor);

    // get the provider's cursor
    hr = pProvider->QueryDataInterface(IID_ICursor,
                                        (void**)&_pCursor);
    if (hr)
        goto Cleanup;

Cleanup:
    return hr;
}


//+-------------------------------------------------------------------------
// Member:              UnBind (CCursorConsumer, public)
//
// Synopsis:    tear down existing binding

HRESULT
CCursorConsumer::UnBind()
{
    return S_OK;
}


//+-------------------------------------------------------------------------
// Member:              FireOnDataReady (CCursorConsumer, public)
//
// Synopsis:    tell my element the data's ready to eat

void
CCursorConsumer::FireOnDataReady(BOOL fReady)
{
    HRESULT             hr;
    COleSite            *pOleSiteConsumer = DYNCAST(COleSite, GetElementConsumer());
    CLASSINFO           *pci= pOleSiteConsumer->GetClassInfo();
    BOOL                fBound;
    BOOL                bOwnedXfer;
    IBoundObject        *pBO = 0;
    VARIANT             var;

    Assert(pci);
    Assert(pci->dispidCursor != DISPID_UNKNOWN);

    if (!fReady)
        ClearInterface(&_pCursor);

    // See if my control supports IBoundObject
    hr = THR_NOTRACE(pOleSiteConsumer->QueryControlInterface(IID_IBoundObject,
                                                      (void **)&pBO));
    if (!hr)
    {
        // IBoundObject supported: signal the cursor is changing.
        // This will cause my control to call the COleSite::Client's
        // IBoundObjectSite::GetCursor.
        Assert(pBO);
        fBound = (_pCursor != 0);
        hr = THR(pBO->OnSourceChanged(pci->dispidCursor, fBound, &bOwnedXfer));
        if (hr)
            goto Cleanup;
    }
    else
    {
        // IBoundObject not supported: use dispatch to set consumer's Cursor
        VariantInit(&var);
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = _pCursor;
        pOleSiteConsumer->CacheDispatch();
        hr = pOleSiteConsumer->SetProperty((UINT)(~0UL),
                            pci->dispidCursor, VT_UNKNOWN, &var);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pBO);
}


//+-------------------------------------------------------------------------
// Member:              GetICursor (CCursorConsumer, public)
//
// Synopsis:    return my ICursor - called by control's IBoundObjectSite

HRESULT
CCursorConsumer::GetICursor(ICursor **ppCursor)
{
    *ppCursor = _pCursor;
    if (_pCursor)
        _pCursor->AddRef();
    return S_OK;
}



/////-------------------------------------------------------------------/////
/////                        CDataSourceConsumer                        /////
/////-------------------------------------------------------------------/////

MtDefine(CDataSourceConsumer, DataBind, "CDataSourceConsumer")
MtDefine(CDataSourceConsumer_aryListeners_pv, CDataSourceConsumer, "CDataSourceConsumer::_aryListeners::_pv")

class CDataSourceConsumer: public CDataSourceBinder::CConsumer, public DataSource
{
    typedef CDataSourceBinder::CConsumer super;
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDataSourceConsumer))
    CDataSourceConsumer(CDataSourceBinder *pBinder):
        super(pBinder), _fPropertySet(FALSE), _aryListeners(Mt(CDataSourceConsumer_aryListeners_pv)) {}
    virtual HRESULT Bind();
    virtual HRESULT UnBind();
    virtual void    FireOnDataReady(BOOL fReady);
    virtual void    Passivate();

    // IUnknown members
    ULONG   STDMETHODCALLTYPE AddRef() { return super::AddRef(); }
    ULONG   STDMETHODCALLTYPE Release() { return super::Release(); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv);

    // DataSource members
    HRESULT STDMETHODCALLTYPE getDataMember(DataMember bstrDM, REFIID riid, IUnknown** ppunk);
    HRESULT STDMETHODCALLTYPE getDataMemberName(long lIndex, DataMember* pbstrDM);
    HRESULT STDMETHODCALLTYPE getDataMemberCount(long* plCount);
    HRESULT STDMETHODCALLTYPE addDataSourceListener(DataSourceListener* pDSL);
    HRESULT STDMETHODCALLTYPE removeDataSourceListener(DataSourceListener* pDSL);

private:
    CPtrAry<DataSourceListener *>  _aryListeners;  // DataSource Listeners
    unsigned    _fPropertySet:1;    // true if I've pushed myself into control's property
};


//+-------------------------------------------------------------------------
// Member:              Bind (CDataSourceConsumer, public)
//
// Synopsis:    set up the desired binding

HRESULT
CDataSourceConsumer::Bind()
{
    HRESULT hr = S_OK;

    if (!_fPropertySet)
    {
        // first time - push myself into control's DataSource property
        VARIANT var;
        IUnknown *punkThis = (IUnknown*)(DataSource*)this;

        CElement *pelConsumer = GetElementConsumer();
        LONG id = IdConsumer();
        const CDBindMethods *pdbm = pelConsumer->GetDBindMethods();

        VariantInit(&var);
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = punkThis;
        punkThis->AddRef();
        hr = pdbm->BoundValueToElement(pelConsumer, id, /* fHTML */ FALSE,
                                       &var);
        VariantClear(&var);
        if (hr)
            goto Cleanup;
        _fPropertySet = TRUE;

        // Databinding should not save undo information
        pelConsumer->Doc()->FlushUndoData();
    }
    else
    {
        // already connected - notify listeners that data member is changed
        for (int i=_aryListeners.Size()-1; i>=0; --i)
        {
            _aryListeners[i]->dataMemberChanged(NULL);
        }
    }

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
// Member:              UnBind (CDataSourceConsumer, public)
//
// Synopsis:    tear down existing binding

HRESULT
CDataSourceConsumer::UnBind()
{
    // nothing to do.
    // It may seem tempting to notify my listeners that the data member has changed,
    // but we shouldn't do that until Bind() is called.  When the provider is
    // changing, UnBind() is followed by Bind();  the notification should happen
    // during Bind() because most likely the listeners are going to immediately
    // call GetDataMember, and we want to be hooked up to the new provider when
    // that happens.
    //
    // The only other time UnBind() is called is while shutting down the binding
    // completely, in which case there's no point in notifying the listeners.

    return S_OK;
}


//+-------------------------------------------------------------------------
// Member:              FireOnDataReady (CDataSourceConsumer, public)
//
// Synopsis:    tell my element the data's ready to eat

void
CDataSourceConsumer::FireOnDataReady(BOOL fReady)
{
    // nothing to do.  Function exists so class can instantiate.
}


//+-------------------------------------------------------------------------
// Member:              Passivate (CDataSourceConsumer, public)
//
// Synopsis:    release my resources

void
CDataSourceConsumer::Passivate()
{
    // release my listeners.  Array is deleted/freed in destructor.
    for (int i=_aryListeners.Size()-1; i>=0; --i)
    {
        _aryListeners[i]->Release();
    }

    super::Passivate();
}


//+-------------------------------------------------------------------------
// Member:              QueryInterface (CDataSourceConsumer, IUnknown, public)
//
// Synopsis:    return desired interface

HRESULT
CDataSourceConsumer::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr;
    IUnknown *punk = 0;

    if (IsEqualIID(riid, IID_DataSource))
    {
        punk = (DataSource*) this;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        punk = (IUnknown*)(DataSource*)this;
    }

    if (punk)
    {
        punk->AddRef();
        hr = S_OK;
    } else
        hr = E_NOINTERFACE;

    *ppv = punk;
    return hr;
}


//+-------------------------------------------------------------------------
// Member:              getDataMember (CDataSourceConsumer, DataSource, public)
//
// Synopsis:    return desired data-transfer interface

HRESULT
CDataSourceConsumer::getDataMember(DataMember bstrDM, REFIID riid, IUnknown** ppunk)
{
    HRESULT hr = E_NOINTERFACE;
    IUnknown *punkInterface = 0;
    CDataSourceProvider *pProvider = GetProvider();

    if (IsEqualIID(riid, IID_IUnknown))
    {
        switch (DYNCAST(COleSite, GetElementConsumer())->OlesiteTag())
        {
        case COleSite::OSTAG_ACTIVEX:
            hr = pProvider->QueryDataInterface(IID_IRowPosition, (void**)&punkInterface);
            break;

        case COleSite::OSTAG_APPLET:
            hr = pProvider->QueryDataInterface(IID_IADORecordset15, (void**)&punkInterface);
            break;
        }
    }
    else
    {
        hr = pProvider->QueryDataInterface(riid, (void**)&punkInterface);
    }

    // if the provider returns E_NOINTERFACE, we'll just say we have no data
    if (hr == E_NOINTERFACE)
    {
        hr = S_OK;
        Assert(punkInterface == NULL);
    }
    
    if (!hr && punkInterface)
    {
        *ppunk = punkInterface;
    }

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
// Member:              getDataMemberName (CDataSourceConsumer, DataSource, public)
//
// Synopsis:    return name of given data member

HRESULT
CDataSourceConsumer::getDataMemberName(long lIndex, DataMember* pbstrDM)
{
    return E_NOTIMPL;       // we return Count=0, so this should never get called
}


//+-------------------------------------------------------------------------
// Member:              getDataMemberCount (CDataSourceConsumer, DataSource, public)
//
// Synopsis:    return number of data members

HRESULT
CDataSourceConsumer::getDataMemberCount(long* plCount)
{
    if (plCount)
        *plCount = 0;
    return S_OK;
}


//+-------------------------------------------------------------------------
// Member:              addDataSourceListener (CDataSourceConsumer, DataSource, public)
//
// Synopsis:    add a DataSourceListener to my notification list

HRESULT
CDataSourceConsumer::addDataSourceListener(DataSourceListener* pDSL)
{
    if (pDSL)
    {
        pDSL->AddRef();
        _aryListeners.Append(pDSL);
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
// Member:              removeDataSourceListener (CDataSourceConsumer, DataSource, public)
//
// Synopsis:    remove a DataSourceListener from my notification list

HRESULT
CDataSourceConsumer::removeDataSourceListener(DataSourceListener* pDSL)
{
    int i = _aryListeners.Find(pDSL);
    if (i>=0)
    {
        _aryListeners[i]->Release();
        _aryListeners.Delete(i);
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
// Member:              Release (CConsumer, IUnknown, public)
//
// Synopsis:    decrease refcount, passivate and die if 0

ULONG
CDataSourceBinder::CConsumer::Release()
{
    ULONG ulRefs = --_ulRefs;

    if (_ulRefs == 0)
    {
        _ulRefs = ULREF_IN_DESTRUCTOR;
        Passivate();
        delete this;
    }

    return ulRefs;
}


//+-------------------------------------------------------------------------
// Member:              Create (CConsumer, static public)
//
// Synopsis:    factory method - create a consumer

HRESULT
CDataSourceBinder::CConsumer::Create(CDataSourceBinder *pBinder,
                                     CConsumer **ppConsumer)
{
    CElement *pelConsumer = pBinder->GetElementConsumer();
    LONG id = pBinder->IdConsumer();
    HRESULT hr = S_OK;
    CConsumer *pConsumer = NULL;

    Assert(pelConsumer && ppConsumer);

    switch (CDBindMethods::DBindKind(pelConsumer, id, NULL))
    {
    case DBIND_SINGLEVALUE:
        pConsumer = new CCurrentRecordConsumer(pBinder);
        break;

    case DBIND_IDATASOURCE:
        pConsumer = new CDataSourceConsumer(pBinder);
        break;

    case DBIND_ICURSOR:
        pConsumer = new CCursorConsumer(pBinder);
        break;

    case DBIND_IROWSET:
        pConsumer = new CNullConsumer(pBinder);
        break;

    case DBIND_TABLE:
        pConsumer = new CTableConsumer(pBinder);
        break;

    default:
        pConsumer = new CNullConsumer(pBinder);
        break;
    }

    if (!pConsumer)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:
    *ppConsumer = pConsumer;
    return hr;
}

