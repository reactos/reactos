
#include <headers.hxx>

#ifndef X_DMEMBMGR_HXX_
#define X_DMEMBMGR_HXX_
#include "dmembmgr.hxx"
#endif

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include <binder.hxx>       // for CDataSourceProvider
#endif

#ifndef X_DBTASK_HXX_
#define X_DBTASK_HXX_
#include "dbtask.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include <formkrnl.hxx>     // for safetylevel in safety.hxx (via olesite.hxx)
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include <olesite.hxx>
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include <evntprm.hxx>      // for eventparam (needed by fire_ondata*)
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include <elemdb.hxx>       // for DBSPEC
#endif

#ifndef X_ADO_ADOID_H_
#define X_ADO_ADOID_H_
#include <adoid.h>
#endif

#ifndef X_VBCURSOR_VBDSC_H_
#define X_VBCURSOR_VBDSC_H_
#include <vbcursor/vbdsc.h> // for iid_ivbdsc
#endif

#ifndef X_SIMPDATA_H_
#define X_SIMPDATA_H_
#include <simpdata.h>
#endif

#ifndef X_MSDATSRC_H_
#define X_MSDATSRC_H_
#include <msdatsrc.h>
#endif

#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include <tearoff.hxx>
#endif

#ifndef X_GENERIC_HXX_
#define X_GENERIC_HXX_
#include <generic.hxx>
#endif

#ifndef X_SIMPDC_H_
#define X_SIMPDC_H_
#include "simpdc.h"
#endif

DeclareTag(tagDataMemberMgr, "databinding", "DataMemberMgr methods");
DeclareTag(tagUseDebugSDC, "databinding", "implement SDC with VarChangeType");

MtDefine(COSPProxy, DataBind, "COSPProxy");
MtDefine(CDataMemberMgr, DataBind, "CDataMemberMgr");
MtDefine(CDataMemberMgr_aryDataMember_pv, DataBind, "CDataMemberMgr::_aryDataMember::_pv")

const IID IID_ISimpleDataConverter = {0x78667670,0x3C3D,0x11d2,0x91,0xF9,0x00,0x60,0x97,0xC9,0x7F,0x9B};

#if DBG == 1
MtDefine(CDbgSimpleDataConverter, DataBind, "CDbgSimpleDataConverter");

//+---------------------------------------------------------------------------
//
//  Class:      CDbgSimpleDataConverter
//
//  Purpose:    For debugging, we implement ISimpleDataConverter internally
//              by simply calling VariantChangeTypeEx for certain types
//              that we're interested in testing.
//
//----------------------------------------------------------------------------

class CDbgSimpleDataConverter : public ISimpleDataConverter
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDbgSimpleDataConverter));
    CDbgSimpleDataConverter(): _ulRefs(1) {}
    
    // IUnknown methods
    HRESULT STDMETHODCALLTYPE   QueryInterface(REFIID riid, void **ppv);
    ULONG STDMETHODCALLTYPE     AddRef() { return ++ _ulRefs; }
    ULONG STDMETHODCALLTYPE     Release();

    // ISimpleDataConverter methods
    HRESULT STDMETHODCALLTYPE ConvertData( 
        VARIANT varSrc,
        long vtDest,
        IUnknown __RPC_FAR *pUnknownElement,
        VARIANT __RPC_FAR *pvarDest);
    
    HRESULT STDMETHODCALLTYPE CanConvertData( 
        long vt1,
        long vt2);

private:
    ULONG   _ulRefs;        // refcount
};

HRESULT STDMETHODCALLTYPE
CDbgSimpleDataConverter::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr;
    
    if (ppv == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ISimpleDataConverter))
    {
        *ppv = this;
        AddRef();
        hr = S_OK;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

Cleanup:
    RRETURN(hr);
}


ULONG STDMETHODCALLTYPE
CDbgSimpleDataConverter::Release()
{
    ULONG ulRefs = --_ulRefs;
    if (ulRefs == 0)
    {
        delete this;
    }
    return ulRefs;
}

HRESULT STDMETHODCALLTYPE
CDbgSimpleDataConverter::ConvertData( 
    VARIANT varSrc,
    long vtDest,
    IUnknown *pUnknownElement,
    VARIANT *pvarDest)
{
    HRESULT hr;

    if (!pvarDest)
    {
        hr = S_OK;
    }
    else if (S_OK != CanConvertData(V_VT(&varSrc), vtDest))
    {
        hr = E_FAIL;
    }
    else
    {
        hr = VariantChangeTypeEx(pvarDest, &varSrc, g_lcidUserDefault, 0, vtDest);
    }
    
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE
CDbgSimpleDataConverter::CanConvertData( 
    long vt1,
    long vt2)
{
    HRESULT hr = S_FALSE;

    // one of the types must be BSTR
    if (vt1 != VT_BSTR)
    {
        long vtTemp = vt1;
        vt1 = vt2;
        vt2 = vtTemp;
    }

    if (vt1 != VT_BSTR)
        goto Cleanup;

    // the other can be on the list below
    switch (vt2)
    {
    case VT_DATE:
    case VT_CY:
    case VT_I2:
    case VT_I4:
    case VT_R4:
    case VT_R8:
    case VT_DECIMAL:
        hr = S_OK;
        break;

    default:
        break;
    }

Cleanup:
    return hr;
}

#endif DBG == 1


//+---------------------------------------------------------------------------
//
//  Class:      COSPProxy
//
//  Purpose:    This class serves a single purpose.  Namely, to delay
//              the Release() of the java ocx until we've released the
//              OSP object down to zero.
//
//----------------------------------------------------------------------------

class COSPProxy : OLEDBSimpleProvider
{
    int _refs;
    OLEDBSimpleProvider *_pOSPReal;
    IUnknown *_pUnkOther;
    public:
        DECLARE_MEMALLOC_NEW_DELETE(Mt(COSPProxy))
        COSPProxy() : _pOSPReal(NULL), _pUnkOther(NULL), _refs(1) { }

        HRESULT Init(IUnknown *pOSPReal, IUnknown *pUnkOther)
        {
            HRESULT hr = pOSPReal->QueryInterface(IID_OLEDBSimpleProvider, (void**)&_pOSPReal);
            if (hr)
                goto Cleanup;
            _pUnkOther = pUnkOther;
            _pUnkOther->AddRef();
        Cleanup:
            return hr;
        }

        virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
        {
            HRESULT hr = E_NOINTERFACE;
            *ppvObject = NULL;
            if (riid == IID_IUnknown)
                *ppvObject = (void**)this;
            else if (riid == IID_OLEDBSimpleProvider)
                *ppvObject = (void**)this;
            if (*ppvObject)
            {
                hr = S_OK;
                AddRef();
            }
            return hr;
        }
        
        virtual ULONG STDMETHODCALLTYPE AddRef( void)
        {
            return ++_refs;
        }
        
        virtual ULONG STDMETHODCALLTYPE Release( void)
        {
            if (--_refs == 0)
            {
                // this ordering is crucial
                _pOSPReal->Release();
                _pUnkOther->Release();
                delete this;
                return 0;
            }
            return _refs;
        }
        
        virtual HRESULT STDMETHODCALLTYPE getRowCount( 
            /* [retval][out] */ DBROWCOUNT __RPC_FAR *pcRows)
        {
            return _pOSPReal->getRowCount(pcRows);
        }
        
        virtual HRESULT STDMETHODCALLTYPE getColumnCount( 
            /* [retval][out] */ DB_LORDINAL __RPC_FAR *pcColumns)
        {
            return _pOSPReal->getColumnCount(pcColumns);
        }
        
        virtual HRESULT STDMETHODCALLTYPE getRWStatus( 
            /* [in] */ DBROWCOUNT iRow,
            /* [in] */ DB_LORDINAL iColumn,
            /* [retval][out] */ OSPRW __RPC_FAR *prwStatus)
        {
            return _pOSPReal->getRWStatus(iRow, iColumn, prwStatus);
        }
        
        virtual HRESULT STDMETHODCALLTYPE getVariant( 
            /* [in] */ DBROWCOUNT iRow,
            /* [in] */ DB_LORDINAL iColumn,
            /* [in] */ OSPFORMAT format,
            /* [retval][out] */ VARIANT __RPC_FAR *pVar)
        {
            return _pOSPReal->getVariant(iRow, iColumn, format, pVar);
        }
        
        virtual HRESULT STDMETHODCALLTYPE setVariant( 
            /* [in] */ DBROWCOUNT iRow,
            /* [in] */ DB_LORDINAL iColumn,
            /* [in] */ OSPFORMAT format,
            /* [in] */ VARIANT Var)
        {
            return _pOSPReal->setVariant(iRow, iColumn, format, Var);
        }
        
        virtual HRESULT STDMETHODCALLTYPE getLocale( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrLocale)
        {
            return _pOSPReal->getLocale(pbstrLocale);
        }
        
        virtual HRESULT STDMETHODCALLTYPE deleteRows( 
            /* [in] */ DBROWCOUNT iRow,
            /* [in] */ DBROWCOUNT cRows,
            /* [retval][out] */ DBROWCOUNT __RPC_FAR *pcRowsDeleted)
        {
            return _pOSPReal->deleteRows(iRow, cRows, pcRowsDeleted);
        }
        
        virtual HRESULT STDMETHODCALLTYPE insertRows( 
            /* [in] */ DBROWCOUNT iRow,
            /* [in] */ DBROWCOUNT cRows,
            /* [retval][out] */ DBROWCOUNT __RPC_FAR *pcRowsInserted)
        {
            return _pOSPReal->insertRows(iRow, cRows, pcRowsInserted);
        }
        
        virtual HRESULT STDMETHODCALLTYPE find( 
            /* [in] */ DBROWCOUNT iRowStart,
            /* [in] */ DB_LORDINAL iColumn,
            /* [in] */ VARIANT val,
            /* [in] */ OSPFIND findFlags,
            /* [in] */ OSPCOMP compType,
            /* [retval][out] */ DBROWCOUNT __RPC_FAR *piRowFound)
        {
            return _pOSPReal->find(iRowStart, iColumn, val, findFlags, compType, piRowFound);
        }
        
        virtual HRESULT STDMETHODCALLTYPE addOLEDBSimpleProviderListener( 
            /* [in] */ OLEDBSimpleProviderListener __RPC_FAR *pospIListener)
        {
            return _pOSPReal->addOLEDBSimpleProviderListener(pospIListener);
        }
        
        virtual HRESULT STDMETHODCALLTYPE removeOLEDBSimpleProviderListener( 
            /* [in] */ OLEDBSimpleProviderListener __RPC_FAR *pospIListener)
        {
            return _pOSPReal->removeOLEDBSimpleProviderListener(pospIListener);
        }
        
        virtual HRESULT STDMETHODCALLTYPE isAsync( 
            /* [retval][out] */ BOOL __RPC_FAR *pbAsynch)
        {
            return _pOSPReal->isAsync(pbAsynch);
        }
        
        virtual HRESULT STDMETHODCALLTYPE getEstimatedRows( 
            /* [retval][out] */ DBROWCOUNT __RPC_FAR *piRows)
        {
            return _pOSPReal->getEstimatedRows(piRows);
        }
        
        virtual HRESULT STDMETHODCALLTYPE stopTransfer( void)
        {
            return _pOSPReal->stopTransfer();
        }
};


//+---------------------------------------------------------------------------
//
//  Member:     Create (static, public)
//
//  Synopsis:   Create a CDataMemberMgr for a given element.
//
//----------------------------------------------------------------------------

HRESULT
CDataMemberMgr::Create(CElement *pElement, CDataMemberMgr **ppMgr)
{
    Assert(pElement && ppMgr);
    HRESULT hr = S_OK;
    CDataMemberMgr *pMgr = NULL;

    switch (pElement->Tag())
    {
    case ETAG_OBJECT:
    case ETAG_APPLET:
    case ETAG_EMBED:
        pMgr = new CDataMemberMgr(ET_OLESITE, pElement);
        break;

    case ETAG_GENERIC_LITERAL:
        if (0 == _tcsicmp(pElement->TagName(), _T("XML")))
        {
            pMgr = new CDataMemberMgr(ET_XML, pElement);
        }
        else
            hr = E_INVALIDARG;
        break;
        
    default:
        hr = E_INVALIDARG;
        break;
    }
    
    if (!hr && pMgr == NULL)
        hr = E_OUTOFMEMORY;

    *ppMgr = pMgr;
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     constructor (private)
//
//----------------------------------------------------------------------------

CDataMemberMgr::CDataMemberMgr(ELEMENT_TYPE et, CElement *pElement) :
    _ulRefs(1),
    _et(et),
    _pElementOwner(pElement),
    _aryDataMember(Mt(CDataMemberMgr_aryDataMember_pv)),
    _dispidDataBinding(DISPID_UNKNOWN)
{
    Assert(pElement);
    
    switch (_et)
    {
    case ET_OLESITE:
        _pOleSite = DYNCAST(COleSite, pElement);
        break;

    case ET_XML:
        _pXML = DYNCAST(CGenericElement, pElement);
        break;

    case ET_SCRIPT:
        break;
    }

    _pDoc = _pElementOwner->Doc();
    _pElementOwner->SubAddRef();
}


//+---------------------------------------------------------------------------
//
//  Member:     destructor (private)
//
//----------------------------------------------------------------------------

CDataMemberMgr::~CDataMemberMgr()
{
    _pElementOwner->SubRelease();
}


//+------------------------------------------------------------------------
//
//  Member:     QueryInterface (IUnknown)
//
//  Synopsis:   We support the following interfaces:
//
//                  IUnknown
//                  DataSourceListener
//                  IDATASRCListener
//
//  Arguments:  [iid]
//              [ppv]
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CDataMemberMgr::QueryInterface(REFIID iid, void ** ppv)
{
    if (iid == IID_IUnknown ||
        iid == IID_DataSourceListener)
    {
        *ppv = (DataSourceListener *) this;
    }
    else if (iid == IID_IDATASRCListener)
    {
        *ppv = (IDATASRCListener *) this;
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_NOINTERFACE);
    }

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG)
CDataMemberMgr::AddRef( )
{
    return ++_ulRefs;
}


STDMETHODIMP_(ULONG)
CDataMemberMgr::Release( )
{
    ULONG ulRefs = --_ulRefs;

    if (ulRefs == 0)
    {
        delete this;
    }
    return ulRefs;
}


//+---------------------------------------------------------------------------
//
//  Member:     IllegalCall (private)
//
//  Returns:    TRUE    illegal call (wrong thread, etc)
//              FALSE   legal call
//----------------------------------------------------------------------------

BOOL
CDataMemberMgr::IllegalCall(DWORD dwFlags)
{
    switch (_et)
    {
    case ET_OLESITE:
        Assert(_pOleSite);
        return _pOleSite->IllegalSiteCall(dwFlags);
        break;

    case ET_XML:
    case ET_SCRIPT:
        if (_pDoc->_dwTID != GetCurrentThreadId())
        {
            Assert(0 && "ActiveX control called MSHTML across apartment thread boundary (not an MSHTML bug)");
            return TRUE;
        }
        break;
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     IsReady (public)
//
//  Returns:    S_OK    ready
//              S_FALSE not ready
//              E_*     some bad kind of error occurred
//----------------------------------------------------------------------------

HRESULT
CDataMemberMgr::IsReady()
{
    HRESULT hr = S_OK;
    LONG lReadyState;
    
    switch (_et)
    {
    case ET_OLESITE:
        hr = _pOleSite->GetReadyState(&lReadyState);
        if (hr==S_OK && lReadyState < READYSTATE_LOADED)
        {
            hr = S_FALSE;
        }
        break;

    case ET_XML:
        break;
    
    case ET_SCRIPT:
        hr = E_NOTIMPL;    // BUGBUG notimpl
        break;
    }

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     GetDataMemberRecord (protected)
//
//  Synopsis:   looks up bstrName in the associative array, and returns
//              the corresponding data member record.
//
//----------------------------------------------------------------------------

CDataMemberMgr::CDataMemberRecord *
CDataMemberMgr::GetDataMemberRecord(BSTR bstrName)
{
    int i;
    CDataMemberRecord *pdmr;

    for (pdmr=_aryDataMember, i=_aryDataMember.Size(); i > 0; ++pdmr, --i)
    {
        if (FormsStringCmp(bstrName, pdmr->_bstrName) == 0)
        {
            return pdmr;
        }
    }

    return NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     AddDataMemberRecord (protected)
//
//  Synopsis:   adds a new data member record to the associative array,
//              with bstrName as the key.  Returns a pointer to the new record.
//
//----------------------------------------------------------------------------

CDataMemberMgr::CDataMemberRecord *
CDataMemberMgr::AddDataMemberRecord(BSTR bstrName)
{
    HRESULT hr;
    CDataMemberRecord *pdmrResult;

    pdmrResult = _aryDataMember.Append();
    if (pdmrResult)
    {
        hr = FormsAllocString(bstrName, &pdmrResult->_bstrName);
        if (!hr)
        {
            pdmrResult->_pdspProvider = NULL;
            pdmrResult->_punkDataBinding = PUNKDB_UNKNOWN;
        }
        else
        {
            pdmrResult = NULL;
            _aryDataMember.Delete(_aryDataMember.Size()-1);
        }
    }

    return pdmrResult;
}


//+---------------------------------------------------------------------------
//
//  Member:     GetDataSourceProvider (public)
//
//  Synopsis:   return a data-interface provider associated with this <OBJECT>
//              (and its control), creating it if needed.
//
//----------------------------------------------------------------------------

HRESULT
CDataMemberMgr::GetDataSourceProvider(BSTR bstrMember, CDataSourceProvider **ppdsp)
{
    Assert(ppdsp);

    HRESULT hr = S_OK;
    CDataMemberRecord *pdmr;

    *ppdsp = NULL;      // just in case

    // look up the data member
    pdmr = GetDataMemberRecord(bstrMember);

    // if not there, create one
    if (pdmr == NULL)
    {
        pdmr = AddDataMemberRecord(bstrMember);
        if (pdmr == NULL)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    // Create the top-level provider, if necessary.
    if (!pdmr->_pdspProvider)
    {
        hr = CDataSourceProvider::Create(this, Doc(), bstrMember, &pdmr->_pdspProvider);
        if (hr)
            goto Cleanup;
    }

    // return the answer
    *ppdsp = pdmr->_pdspProvider;
    if (*ppdsp)
        (*ppdsp)->AddRef();

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     ChangeDataBindingInterface (protected)
//
//  Synopsis:   React to changes in my control's databinding interface,
//              by replacing my provider.
//
//----------------------------------------------------------------------------

HRESULT
CDataMemberMgr::ChangeDataBindingInterface(BSTR bstrMember, BOOL fDataAvail)
{
    HRESULT hr = S_OK;
    CDataMemberRecord *pdmr = GetDataMemberRecord(bstrMember);
    CDataSourceProvider *pdspOldProvider;
    CDataSourceProvider *pdspNewProvider;

    // if my control isn't a data provider or we don't care about data member,
    // just ignore
    if (_dpt <= DPT_NOTAPROVIDER || pdmr == NULL)
        goto Cleanup;

    // remember the old provider, discard the old interface
    pdspOldProvider = pdmr->_pdspProvider;
    if (pdmr->_punkDataBinding != PUNKDB_UNKNOWN)
    {
        ClearInterface(&pdmr->_punkDataBinding);
    }
    pdmr->_punkDataBinding = PUNKDB_UNKNOWN;
    
    // get the new interface from the control
    if (fDataAvail)
    {
        IGNORE_HR(EnsureDataBindingInterface(bstrMember));
    }

    // let my provider know what happened
    if (!pdspOldProvider)   // no provider, nothing to do
        goto Cleanup;

    pdmr->_pdspProvider = NULL;                     // unhook the old provider
    hr = GetDataSourceProvider(bstrMember, &pdspNewProvider);   // hook up a new one
    if (hr)
    {
        pdmr->_pdspProvider = pdspOldProvider;      // if error, restore status quo
        goto Cleanup;
    }

    pdspOldProvider->ReplaceProvider(pdspNewProvider);  // notify provider's clients
    pdspOldProvider->Release();                     // let go of old provider
    pdspNewProvider->Release();                     // release local ref to new one

Cleanup:
    // the data member change may let bindings succeed that didn't before,
    // notably bindings to the inner datasets of a hierarchy.  So tell the
    // binding task to try again.
    Doc()->GetDataBindTask()->SetWaiting();

    return hr;
}


CDataMemberMgr::DATA_PROVIDER_TYPE
CDataMemberMgr::GetDataProviderType()
{
    // Do we already know what data provider type this site is hosting?
    if (_dpt==DPT_UNKNOWN)
    {
        // No. Try to find one.
        FindDataProviderType();
    }
    return _dpt;
}


//+---------------------------------------------------------------------------
//
//  Member:     EnsureDataBindingInterface (protected)
//
//  Synopsis:   Query the underlying control/applet for an interface
//              its clients can use for databinding
//
//----------------------------------------------------------------------------

HRESULT
CDataMemberMgr::EnsureDataBindingInterface(BSTR bstrMember)
{
    HRESULT hr = S_OK;
    CDataMemberRecord *pdmr;
    IUnknown *punkNew = NULL;

    // look up the data member
    pdmr = GetDataMemberRecord(bstrMember);

    // if not there, create one
    if (pdmr == NULL)
    {
        pdmr = AddDataMemberRecord(bstrMember);
        if (pdmr == NULL)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    // get the interface, if not there already
    if (pdmr->_punkDataBinding == PUNKDB_UNKNOWN)
    {
        CLASSINFO *pci;
        IServiceProvider *pServiceProvider = NULL;

        switch (GetDataProviderType())
        {
        case DPT_DATASOURCE:
            Assert(_pDataSource);
            hr = _pDataSource->getDataMember(bstrMember, IID_IUnknown, &punkNew);
            break;

        case DPT_COM:
        case DPT_JAVA:
            Assert(_et == ET_OLESITE);
            // the service provider (if needed) is the doc
            Doc()->PrivateQueryInterface(IID_IServiceProvider, (void**)&pServiceProvider);
            hr = CallDispMethod(pServiceProvider,
                                _pOleSite->_pDisp,
                                _dispidDataBinding,
                                g_lcidUserDefault,
                                VT_UNKNOWN,
                                &punkNew,
                                EVENT_PARAM(VTS_BSTR),
                                bstrMember
                                );
            ReleaseInterface(pServiceProvider);
            break;

        case DPT_ICURSOR:
            AssertSz(0, "ChangeDataBindingInterface called on ICursor provider");
            break;
            
        case DPT_PROPERTY:
            Assert(_et == ET_OLESITE);
            pci = _pOleSite->GetClassInfo();
            if (pci->dispidSTD != DISPID_UNKNOWN) // offers OLEDBSimpleProvider
            {
                hr = _pOleSite->GetInterfaceProperty(pci->uGetSTD, pci->dispidSTD,
                                            IID_OLEDBSimpleProvider, &punkNew);
            }
            else if (pci->dispidRowset != DISPID_UNKNOWN)        // offers IRowset
            {
                hr = _pOleSite->GetInterfaceProperty(pci->uGetRowset, pci->dispidRowset,
                                            IID_IRowset, &punkNew);
            }
            break;
        }

        // OSP providers are notoriously guilty of the following sin:  they
        // allow the control to die while references to OSPs are still outstanding.
        // To work around this problem, build a proxy object that artificially
        // increments the refcount of the control throughout the lifetime of the OSP.
        if (!hr && punkNew)
        {
            IUnknown *punkOther = NULL;
            COSPProxy *pProxy = new COSPProxy();

            // get a controlling unknown for the DSO
            switch (_et)
            {
            case ET_OLESITE:
                punkOther = _pOleSite->PunkCtrl();
                punkOther->AddRef();
                break;
            case ET_XML:
                if (S_OK != _pDataSource->QueryInterface(IID_IUnknown, (void**)&punkOther))
                    punkOther = NULL;
                break;
            default:
                break;
            }
            
            // If anything goes wrong (such as if Init() fails because
            // punkNew doesn't implement OLEDBSimpleProvider), then we'll
            // just proceed as normal.
            if (punkOther == NULL || pProxy == NULL || pProxy->Init(punkNew, punkOther))
            {
                delete pProxy;
            }
            else
            {
                punkNew->Release();
                punkNew = (IUnknown*)(void*)pProxy;
            }

            ReleaseInterface(punkOther);
        }

        pdmr->_punkDataBinding = punkNew;
    }

Cleanup:
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     GetDataBindingInterface (public)
//
//  Synopsis:   Query the underlying control/applet for an interface
//              its clients can use for databinding
//
//----------------------------------------------------------------------------

HRESULT
CDataMemberMgr::GetDataBindingInterface(BSTR bstrMember, IUnknown **ppunkDataBinding)
{
    HRESULT hr = S_OK;
    CDataMemberRecord *pdmr;
    IUnknown *punkDataBinding = NULL;

    // look up the data member
    pdmr = GetDataMemberRecord(bstrMember);

    // if this is first request, try to get the dataset
    if (pdmr == NULL || pdmr->_punkDataBinding == PUNKDB_UNKNOWN)
    {
        hr = EnsureDataBindingInterface(bstrMember);
        if (pdmr == NULL)
            pdmr = GetDataMemberRecord(bstrMember);
    }
    
    punkDataBinding = pdmr ? pdmr->_punkDataBinding : NULL;
    if (punkDataBinding == PUNKDB_UNKNOWN)
        punkDataBinding = NULL;

    // return the answer
    Assert(ppunkDataBinding);
    *ppunkDataBinding = punkDataBinding;
    if (punkDataBinding)
        punkDataBinding->AddRef();

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     FindDataProviderType (private helper)
//
//  Synopsis:   determine if the underlying DSO provides data, and if so, how
//
//----------------------------------------------------------------------------

void
CDataMemberMgr::FindDataProviderType()
{
    HRESULT hr = S_OK;
    CLASSINFO *pci;
    IDispatch *pDisp;
    DISPID dispid;
    IUnknown *punkInterface = 0;
    static OLECHAR * oszMsDATASRCObject = _T("msDataSourceObject");
    static OLECHAR * oszAddDataSrcListener = _T("addDataSourceListener");
    DataSource *pDataSource = 0;
    IServiceProvider *pServiceProvider = 0;
    
    Assert(_dpt == DPT_UNKNOWN);    // if we already know, why bother

    _dpt = DPT_NOTAPROVIDER;        // assume the worst

    switch (_et)
    {
    case ET_OLESITE:
        Assert(_pOleSite);
        // must have a valid dispatch for any of our methods
        _pOleSite->CacheDispatch();
        pDisp = _pOleSite->_pDisp;
        if (!pDisp)
            goto Cleanup;

        if (!_pOleSite->AccessAllowed(pDisp))
            goto Cleanup;

        // the service provider (if needed) is the doc
        Doc()->PrivateQueryInterface(IID_IServiceProvider, (void**)&pServiceProvider);

        switch (_pOleSite->OlesiteTag())
        {
        case COleSite::OSTAG_ACTIVEX:
            // method 5:  look for DataSource
            hr = _pOleSite->QueryControlInterface(IID_DataSource, (void**)&pDataSource);
            if (hr == S_OK)
            {
                _dpt = DPT_DATASOURCE;
                IGNORE_HR(pDataSource->addDataSourceListener((DataSourceListener*)this));
                _pDataSource = pDataSource;
                break;
            }

            // method 1:  look for the MsDatasrcInterface method (COM objects)
            hr = CallDispMethod(pServiceProvider,
                                _pOleSite->_pDisp,
                                DISPID_MSDATASRCINTERFACE,
                                g_lcidUserDefault,
                                VT_UNKNOWN,
                                &punkInterface,
                                EVENT_PARAM(VTS_BSTR),
                                (BSTR) 0
                                );
            if (!DISPID_NOT_FOUND(hr))
            {
                _dpt = DPT_COM;
                _dispidDataBinding = DISPID_MSDATASRCINTERFACE;
                
                // listen for DatasrcChanged
                IGNORE_HR(CallDispMethod(pServiceProvider,
                                        _pOleSite->_pDisp,
                                        DISPID_ADVISEDATASRCCHANGEEVENT,
                                        g_lcidUserDefault,
                                        VT_VOID,
                                        NULL,
                                        EVENT_PARAM(VTS_UNKNOWN),
                                        (IUnknown*)(DataSourceListener*) this
                                        ));
                break;
            }

            // method 3:  look for IVBDSC (VB ICursor controls)
            hr = _pOleSite->QueryControlInterface(IID_IVBDSC, (void**)&punkInterface);
            if (hr == S_OK)
            {
                _dpt = DPT_ICURSOR;
                break;
            }

            // method 4:  look for a databinding interface in a property (older controls)
            //      This is last so that we don't look at the typelib until all else fails.
            pci = _pOleSite->GetClassInfo();
            if (pci->dispidSTD != DISPID_UNKNOWN)           // offers ISimpleTabularData
            {
                _dpt = DPT_PROPERTY;
                break;
            }
            if (pci->dispidRowset != DISPID_UNKNOWN)        // offers IRowset
            {
                _dpt = DPT_PROPERTY;
                break;
            }
            
        break;

        case COleSite::OSTAG_APPLET:
            // method 2:  look for the msDATASRCObject method (Java applets)
            hr = _pOleSite->_pDisp->GetIDsOfNames(IID_NULL, &oszMsDATASRCObject, 1,
                                        g_lcidUserDefault, &dispid);
            if (hr == S_OK)
            {
                DataSourceListener *pDSL = 0;
                
                _dpt = DPT_JAVA;
                _dispidDataBinding = dispid;
                
                // listen for DatasrcChanged
                if (S_OK ==_pOleSite->_pDisp->GetIDsOfNames(IID_NULL, &oszAddDataSrcListener, 1,
                                        g_lcidUserDefault, &dispid))
                {
                    IGNORE_HR(CallDispMethod(pServiceProvider,
                                            _pOleSite->_pDisp,
                                            dispid,
                                            g_lcidUserDefault,
                                            VT_VOID,
                                            NULL,
                                            EVENT_PARAM(VTS_UNKNOWN),
                                            (IUnknown*)(DataSourceListener*) this
                                            ));
                    ReleaseInterface(pDSL);
                }
            }
            break;
            
        }
        break;

    case ET_XML:
    {
        HRESULT     hr;
        DataSource *pDataSource = NULL;
        DISPID      dispid;
        VARIANT     varRet;
        EXCEPINFO   excepinfo;
        DISPPARAMS  dispparams = {NULL, NULL, 0, 0};
        UINT        uiError;
        IXMLDocument *pXMLDocument = NULL;

#define QIforDataSource
#ifdef QIforDataSource
        // get DataSource from the data island
        hr = _pElementOwner->QueryInterface(IID_DataSource, (void**)&pDataSource);
        if (hr == S_OK)
        {
            _dpt = DPT_DATASOURCE;
            IGNORE_HR(pDataSource->addDataSourceListener((DataSourceListener*)this));
            _pDataSource = pDataSource;
            break;          // BUGBUG remove when QI(DataSource) works
        }
#endif

        // BUGBUG until QI(DataSource) works, continue to try the old way
        // get XMLDocument from the data island
        VariantInit(&varRet);
        hr = _pElementOwner->GetDispID(_T("XMLDocument"), fdexNameCaseSensitive, &dispid);
        if (!hr)
            hr = _pElementOwner->Invoke(
                                    dispid,
                                    IID_NULL,
                                    LOCALE_SYSTEM_DEFAULT,
                                    DISPATCH_PROPERTYGET,
                                    &dispparams,
                                    &varRet,
                                    &excepinfo,
                                    &uiError);
        if (!hr && (V_VT(&varRet) == VT_DISPATCH || V_VT(&varRet) == VT_UNKNOWN))
        {
            pXMLDocument = (IXMLDocument *) V_UNKNOWN(&varRet);
        }

        // get DataSource from the document
        if (pXMLDocument)
        {
            hr = pXMLDocument->QueryInterface(IID_DataSource, (void**)&pDataSource);
            if (hr == S_OK)
            {
                _dpt = DPT_DATASOURCE;
                IGNORE_HR(pDataSource->addDataSourceListener((DataSourceListener*)this));
                _pDataSource = pDataSource;
            }
        }

        ReleaseInterface(pXMLDocument);
        break;
    }
    
    default:
        break;
    }


    // if the DSO uses DataSource, it might also use ISimpleDataConverter
    if (_dpt == DPT_DATASOURCE)
    {
        ISimpleDataConverter *pSDC = NULL;

        hr = _pDataSource->QueryInterface(IID_ISimpleDataConverter, (void**)&pSDC);

#if DBG == 1
        // for debugging, use an internal mock-up of ISimpleDataConverter
        if (IsTagEnabled(tagUseDebugSDC))
        {
            ReleaseInterface(pSDC);
            pSDC = new CDbgSimpleDataConverter;
            hr = S_OK;
        }
#endif

        if (!hr && pSDC)
        {
            _pSDC = pSDC;
        }
    }
    
Cleanup:
    ReleaseInterface(punkInterface);
    ReleaseInterface(pServiceProvider);
    
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDataMemberMgr::DetachDataProviders (public)
//
//  Synopsis:   Release resources associated with data provider objects
//
//----------------------------------------------------------------------------
void
CDataMemberMgr::DetachDataProviders()
{
    int i;
    CDataMemberRecord *pdmr;

    // disconnect providers
    for (i=_aryDataMember.Size(), pdmr=_aryDataMember; i>0; --i, ++pdmr)
    {
        if (pdmr->_pdspProvider)
        {
            pdmr->_pdspProvider->Detach();
            pdmr->_pdspProvider->Release();
            pdmr->_pdspProvider = NULL;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDataMemberMgr::Detach (public)
//
//  Synopsis:   Release resources associated with data source objects
//
//----------------------------------------------------------------------------

void
CDataMemberMgr::Detach()
{
    int i;
    CDataMemberRecord *pdmr;
    
    // disconnect databinding stuff
    for (i=_aryDataMember.Size(), pdmr=_aryDataMember; i>0; --i, ++pdmr)
    {
        FormsFreeString(pdmr->_bstrName);
        
        if (pdmr->_pdspProvider)
        {
            pdmr->_pdspProvider->Detach();
            pdmr->_pdspProvider->Release();
            pdmr->_pdspProvider = NULL;
        }

        if (pdmr->_punkDataBinding != PUNKDB_UNKNOWN)
        {
            ClearInterface(&pdmr->_punkDataBinding);
        }
    }
    _aryDataMember.DeleteAll();

    switch (_dpt)
    {
    case DPT_DATASOURCE:
        IGNORE_HR(_pDataSource->removeDataSourceListener((DataSourceListener*)this));
        ClearInterface(&_pDataSource);
        ClearInterface(&_pSDC);
        break;

    case DPT_COM:
    case DPT_JAVA:
        Assert(_et==ET_OLESITE && _pOleSite);
    // BUGBUG the spec says we're supposed to call addDataSourceListener(NULL)
    // but TDC, and presumably other controls, barf if we do
#if defined(CONTROLS_KNOW_HOW_TO_HANDLE_AddDataSourceListener_NULL)
        Doc()->PrivateQueryInterface(IID_IServiceProvider, (void**)&pServiceProvider);
        IGNORE_HR(CallDispMethod(pServiceProvider,
                                _pOleSite->_pDisp,
                                DISPID_ADVISEDATASRCCHANGEEVENT,
                                g_lcidUserDefault,
                                VT_VOID,
                                NULL,
                                EVENT_PARAM(VTS_UNKNOWN),
                                NULL
                                ));
        ReleaseInterface(pServiceProvider);
#endif
        break;
        
    default:
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDataMemberMgr::Notify (public)
//
//  Synopsis:   handle notifications set to my element
//
//----------------------------------------------------------------------------

void
CDataMemberMgr::Notify(CNotification * pnf)
{
    CDataMemberRecord * pdmr;
    int                 i;

    switch (pnf->Type())
    {
    case NTYPE_STOP_1:
        if (_aryDataMember.Size())
            pnf->SetSecondChanceRequested();
        break;

    case NTYPE_STOP_2:
        // try to stop any data transfers that may be in progress
        for (i=_aryDataMember.Size(), pdmr=_aryDataMember;
             i > 0;
             --i, ++pdmr)
        {
            if (pdmr->_pdspProvider)
            {
                IGNORE_HR(pdmr->_pdspProvider->Stop());
            }
        }
        break;
        
    case NTYPE_BEFORE_UNLOAD:
        // Databinding spec requires us to fire onrowexit here..
        for (i=_aryDataMember.Size(), pdmr=_aryDataMember; i>0; --i, ++pdmr)
        {
            if (pdmr->_pdspProvider)          // Are we a data provider?
            {
                pdmr->_pdspProvider->FireDataEvent(_T("rowexit"),
                            DISPID_EVMETH_ONROWEXIT, DISPID_EVPROP_ONROWEXIT);
            }
        }
        break;

    case NTYPE_ELEMENT_EXITTREE_1:
        DetachDataProviders();
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     GetTridentAsOSP (public)
//
//  Synopsis:   If my element is a Trident-As-OSP, return a pointer to
//              the CDoc.
//
//  Returns:    S_OK        my element is a Trident-As-OSP.  *ppDocOSP set.
//              S_FALSE     my element isn't a Trident-As-OSP.
//
//  Note:       The returned doc is *not* refcounted.
//
//----------------------------------------------------------------------------

HRESULT
CDataMemberMgr::GetTridentAsOSP(CDoc **ppDocOSP)
{
    HRESULT hr = S_FALSE;

    if (_et == ET_OLESITE &&
        S_OK == _pOleSite->QueryControlInterface(CLSID_HTMLDocument,
                                                (void**)ppDocOSP))
    {
        hr = S_OK;
    }

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     EnsureDataEvents (public)
//
//  Synopsis:   make sure there's a provider to actually fire data events
//              (see IE5 bug 3201).
//
//----------------------------------------------------------------------------

void
CDataMemberMgr::EnsureDataEvents()
{
    CDataMemberRecord *pdmr = GetDataMemberRecord(NULL);
    CDataSourceBinder *pdsb;
    
    // if there's already a provider, no need to create a new one
    if (pdmr==NULL || pdmr->_pdspProvider == NULL)
    {
        // create a provider next time the databinding task list runs
        pdsb = new CDataSourceBinder(_pElementOwner, 0, BINDEROP_ENSURE_DATA_EVENTS);

        if (pdsb)
        {
            DBSPEC dbs;
            memset(&dbs, 0, sizeof(DBSPEC));
            pdsb->Register(&dbs);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDataMemberMgr::namedRecordset
//
//  Synopsis:   returns an ADO Recordset for the named data member.  Tunnels
//              into the hierarchy using the path, if given.
//
//  Arguments:  bstrDataMember  name of data member (NULL for default)
//              pvarHierarchy   BSTR path through hierarchy (optional)
//              pRecordSet      where to return the recordset.
//
//  Returns:    S_FALSE         not a provider
//
//----------------------------------------------------------------------------

HRESULT
CDataMemberMgr::namedRecordset(BSTR bstrDatamember,
                               VARIANT *pvarHierarchy,
                               IDispatch **ppRecordSet)
{
    HRESULT hr=S_OK;
    BSTR bstrHierarchy = NULL;

    if (ppRecordSet == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppRecordSet = NULL;                 // Make sure to null on failure.

    if (pvarHierarchy && V_VT(pvarHierarchy) == VT_BSTR)
    {
        bstrHierarchy = V_BSTR(pvarHierarchy);
    }
    
    // normal case, return ADO recordset based on my provider
    if (IsDataProvider())
    {
        CDataSourceProvider *pdsp = NULL;

        // get the top-level provider for the desired data member
        hr = GetDataSourceProvider(bstrDatamember, &pdsp);

        // find the inner level of hierarchy, if desired
        if (!hr && pdsp && !FormsIsEmptyString(bstrHierarchy))
        {
            CDataSourceProvider *pdspTop = pdsp;
            hr = pdspTop->GetSubProvider(&pdsp, bstrHierarchy);
            pdspTop->Release();
        }

        if (pdsp)
        {
            // We then query the CDataSourceProvider to give us back an ADO recordset,
            // if it can.
            hr = pdsp->QueryDataInterface(IID_IADORecordset15, (void **)ppRecordSet);
            pdsp->Release();

            // We clamp the most obvious failure mode here, so script authors can
            // simply test for recordset being NULL, and not get script errors.
            if (E_NOINTERFACE == hr)
            {
                hr = S_OK;
            }
        }
    }

    else
    {
        hr = S_FALSE;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     dataMemberChanged (DataSourceListener)
//
//  Synopsis:   DataSource notifies that a dataset has changed shape
//
//  Arguments:  bstrDM      name of the changed dataset (NULL means default)
//
//  Returns:    S_OK        success
//----------------------------------------------------------------------------

STDMETHODIMP
CDataMemberMgr::dataMemberChanged(BSTR bstrDM)
{
    TraceTag((tagDataMemberMgr, "CDataMemberMgr::dataMemberChanged SN=%ld", _pElementOwner->SN()));

    if (IllegalCall(COleSite::VALIDATE_LOADED))
        RRETURN(E_UNEXPECTED);

    HRESULT     hr;

    hr = THR(ChangeDataBindingInterface(bstrDM, TRUE));

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     dataMemberAdded (DataSourceListener)
//
//  Synopsis:   DataSource notifies that a new dataset has been born
//
//  Arguments:  bstrDM      name of the new dataset
//
//  Returns:    S_OK        success
//----------------------------------------------------------------------------

STDMETHODIMP
CDataMemberMgr::dataMemberAdded(BSTR bstrDM)
{
    TraceTag((tagDataMemberMgr, "CDataMemberMgr::dataMemberAdded SN=%ld", _pElementOwner->SN()));

    if (IllegalCall(COleSite::VALIDATE_LOADED))
        RRETURN(E_UNEXPECTED);

    RRETURN(S_OK);      // we don't care
}

//+---------------------------------------------------------------------------
//
//  Member:     dataMemberRemoved (DataSourceListener)
//
//  Synopsis:   DataSource notifies that a dataset has died
//
//  Arguments:  bstrDM      name of the dead dataset
//
//  Returns:    S_OK        success
//----------------------------------------------------------------------------

STDMETHODIMP
CDataMemberMgr::dataMemberRemoved(BSTR bstrDM)
{
    TraceTag((tagDataMemberMgr, "CDataMemberMgr::dataMemberRemoved SN=%ld", _pElementOwner->SN()));

    if (IllegalCall(COleSite::VALIDATE_LOADED))
        RRETURN(E_UNEXPECTED);

    CDataMemberRecord *pdmr = GetDataMemberRecord(bstrDM);

    if (pdmr)
    {
        if (pdmr->_pdspProvider)
        {
            pdmr->_pdspProvider->Detach();
            pdmr->_pdspProvider->Release();
            pdmr->_pdspProvider = NULL;
        }

        if (pdmr->_punkDataBinding != PUNKDB_UNKNOWN)
        {
            ClearInterface(&pdmr->_punkDataBinding);
        }
    }
        
    RRETURN(S_OK);
}


//+---------------------------------------------------------------------------
//
//  Member:     datasrcChanged (IDATASRCListener)
//
//  Synopsis:   DataSource notifies that a dataset has changed shape
//
//  Arguments:  bstrQualifier   name of the changed dataset (NULL means default)
//              fDataAvail      true if new dataset is available
//
//  Returns:    S_OK        success
//----------------------------------------------------------------------------

STDMETHODIMP
CDataMemberMgr::datasrcChanged(BSTR bstrQualifier, BOOL fDataAvail)
{
    TraceTag((tagDataMemberMgr, "CDataMemberMgr::datasrcChanged SN=%ld", _pElementOwner->SN()));

    if (IllegalCall(COleSite::VALIDATE_LOADED))
        RRETURN(E_UNEXPECTED);

    HRESULT     hr;

    hr = THR(ChangeDataBindingInterface(bstrQualifier, fDataAvail));

    RRETURN(hr);
}


// This method is implemented here (rather than in baseprop.cxx)
// because it uses OleSite methods.  Getting #include <olesite.hxx> to work
// from src\core\cdbase proved to be an insurmountable challenge.

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP CBase::put_DataEvent(VARIANT v)
{
    GET_THUNK_PROPDESC

    return put_DataEventHelper(v, pPropDesc);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

STDMETHODIMP CBase::put_DataEventHelper(VARIANT v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr)
{
    HRESULT     hr;

    hr = put_VariantHelper(v, pPropDesc,  ppAttr);

    if (!hr)
    {
        // if we add a data event to an element, make sure the event can fire
        CDataMemberMgr::EnsureDataEventsFor(this, pPropDesc->GetDispid());
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     EnsureDataEventsFor (static)
//
//  Synopsis:   Make sure that data events can fire for the given element
//
//  Arguments:  pBase       the given element
//              dispid      the dispid of the event being hooked up
//
//  Returns:    nothing
//----------------------------------------------------------------------------

void
CDataMemberMgr::EnsureDataEventsFor(CBase *pBase, DISPID dispid)
{
    CElement *pElement = NULL;
    CDataMemberMgr *pdmm;
    HRESULT hr;

    switch (dispid)
    {
    // this list must agree with the list of events marked "dataevent"
    // in element.pdl
    case DISPID_EVPROP_ONROWEXIT:
    case DISPID_EVPROP_ONROWENTER:
    case DISPID_EVPROP_ONDATASETCHANGED:
    case DISPID_EVPROP_ONDATAAVAILABLE:
    case DISPID_EVPROP_ONDATASETCOMPLETE:
    case DISPID_EVPROP_ONROWSDELETE:
    case DISPID_EVPROP_ONROWSINSERTED:
    case DISPID_EVPROP_ONCELLCHANGE:

        hr = pBase->PrivateQueryInterface(CLSID_CElement, (void**)&pElement);
        if (!hr && pElement)
        {
            pElement->EnsureDataMemberManager();
            pdmm = pElement->GetDataMemberManager();
            if (pdmm)
            {
                pdmm->EnsureDataEvents();
            }
        }
        break;

    default:
        break;
    }
}

