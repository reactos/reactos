/*
 * @(#)DSOCTRL.cxx 1.0 6/16/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#include "document.hxx"
#include "dsoctrl.hxx"
#include "dtd.hxx"

// {165D4E40-3621-11d2-801B-0000F87A6CDF}
static const GUID CLSID_DSODocument = 
{ 0x165d4e40, 0x3621, 0x11d2, { 0x80, 0x1b, 0x0, 0x0, 0xf8, 0x7a, 0x6c, 0xdf 
} };

//----------------------------------------------------------------------
DEFINE_CLASS_MEMBERS(DSODocument, _T("DSODocument"), Document);

DSODocument::DSODocument(): super()
{
    setAsync(true);
}

HRESULT
DSODocument::QIHelper(DOMDocumentWrapper * pDOMDoc, IDocumentWrapper * pIE4Doc,
                   REFIID riid, void **ppvObject)
{
    STACK_ENTRY_OBJECT(this);
    HRESULT hr = S_OK;
    
    CHECK_ARG_INIT( ppvObject);

    if (riid == CLSID_DSODocument)
    {
        *ppvObject = this;      // no refcount
    }
    else
    {
        hr = super::QIHelper(pDOMDoc, pIE4Doc, riid, ppvObject);
    }

    return hr;
}

void 
DSODocument::setReadyStatus(int state)
{
    Assert(GetTlsData()->_iRunning > 0 && "No stack entry on this thread !");

    if (getReadyStatus() != state)
    {
        super::setReadyStatus(state);

        if (_pDSO)
        {
            if (state == READYSTATE_COMPLETE)
            {
                // Data set just changed !

                // this tosses the old shape and create a new one,
                // informing the DataSourceListener that there is a new dataset
               _pDSO->FireDataMemberChanged();

                XMLRowsetProvider * pProvider = _pDSO->getProvider();
                if (pProvider)                              //if we have provider to announce to
                    pProvider->fireTransferComplete();          //tell it transfer is done
            }
            else
            {
                _pDSO->DiscardShape();
            }
        }
    }
}

void
DSODocument::onDataAvailable()
{
    if (isAsync())
    {
        if (_pDSO) // dso can be null in async case.
        {
            XMLRowsetProvider * pProvider = _pDSO->getProvider();

            if (getRoot() && !pProvider)                                 //if we have data but no provider 
            {
                _pDSO->FireDataMemberChanged();             //tell listener that data set has changed
                pProvider = _pDSO->getProvider();           //this gets the new provider created with notifyListener()
            }

            if (pProvider)                                  //if we have provider
                pProvider->findNewRows();                   //find new data
        }
    }
    
    super::onDataAvailable();
}


void
DSODocument::onEndProlog()                  //called when xml document prolog is complete
{
    super::onEndProlog();

    if (_pDSO)
        _pDSO->makeShape();                 //this checks for compat flag, then checks for dtds and makes schema
}

void
DSODocument::AddListener(IXMLDocumentNotify *pListener)
{
    RemoveListener(_pListener);     // remove the existing listener, if any
    _pListener = pListener;         // add the new one
}


void
DSODocument::RemoveListener(IXMLDocumentNotify *pListener)
{
    if (_pListener == pListener)
    {
        _pListener = null;
    }
}


void
DSODocument::NotifyListener(XMLNotifyReason eReason, XMLNotifyPhase ePhase,
                            Node *pNode, Node *pNodeParent, Node *pNodeBefore)
{
    if (_pListener && pNodeParent && !pNodeParent->isFloating() &&
            ePhase!=XML_PHASE_FailedToDo)
    {
        _pListener->OnNodeChange(eReason, ePhase,
                                (IUnknown *)pNode, (IUnknown *)pNodeParent,
                                (IUnknown *)pNodeBefore);
    }
}


//------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CreateDSOControl(REFIID iid, void **ppvObj)
{
    HRESULT hr = E_FAIL;
    STACK_ENTRY;
    TRY
    {
        // The XMLDSO is the controlling IUnknown,
        // and the DSOControl is a wrapper.
        XMLDSO *pDSO = new XMLDSO();
        CXMLDSOControl *pControl = new CXMLDSOControl(pDSO);

        // Create an empty document, and give it to the control
        DSODocument *doc = new DSODocument();
        doc->setDOM(true);
        pDSO->AddDocument(doc);

        // Finally, get the desired interface on the control
        hr = pControl->QueryInterface(iid, ppvObj);
        pControl->Release();
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


//----------------------------------------------------------------------
DISPATCHINFO _dispatchexport<XMLDSO, IXMLDSOControl, &LIBID_MSXML, ORD_MSXML, &IID_IXMLDSOControl>::s_dispatchinfo = 
{
    NULL, &IID_IXMLDSOControl, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL
};


CXMLDSOControl::CXMLDSOControl(XMLDSO * p) :
    super(p)
{
}


/* [id][propget] */ HRESULT STDMETHODCALLTYPE 
CXMLDSOControl::get_XMLDocument( 
        /* [retval][out] */ IXMLDOMDocument __RPC_FAR *__RPC_FAR *ppDoc)
{
    HRESULT hr;
    STACK_ENTRY_IUNKNOWN(this);
    CHECK_ARG_INIT( ppDoc);
    TRY 
    {
        DSODocument *pDoc = getWrapped()->getDoc();
        hr = pDoc->QueryInterface(IID_IXMLDOMDocument, (void**)ppDoc);
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY
    return hr;
}
    
/* [id][propput] */ HRESULT STDMETHODCALLTYPE 
CXMLDSOControl::put_XMLDocument( 
        /* [in] */ IXMLDOMDocument __RPC_FAR *ppDoc)
{
    STACK_ENTRY_IUNKNOWN(this);
    HRESULT hr;
    TRY 
    {
        DSODocument* pDoc = NULL;
        if (SUCCEEDED(hr = ppDoc->QueryInterface(IID_Document, (void**)&pDoc)))
        {
            DSODocument *pDSODoc;
            if (SUCCEEDED(hr = pDoc->QueryInterface(CLSID_DSODocument, (void**)&pDSODoc)))
            {
                getWrapped()->AddDocument(pDSODoc);
            }
            else
            {
                Exception::throwE(E_INVALIDARG);
            }
        }
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY
    return hr;
}
    
    
/* [id][propget] */ HRESULT STDMETHODCALLTYPE
CXMLDSOControl::get_JavaDSOCompatible( 
        /* [retval][out] */ BOOL __RPC_FAR *fJavaDSOCompatible)
{
    STACK_ENTRY_IUNKNOWN(this);
    HRESULT hr;
    TRY 
    {
        *fJavaDSOCompatible = getWrapped()->getJavaDSOCompatible();
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY
    return hr;
}
    
/* [id][propput] */ HRESULT STDMETHODCALLTYPE
CXMLDSOControl::put_JavaDSOCompatible( 
        /* [in] */ BOOL fJavaDSOCompatible)
{
    STACK_ENTRY_IUNKNOWN(this);
    HRESULT hr;
    TRY 
    {
        getWrapped()->setJavaDSOCompatible(!!fJavaDSOCompatible);
        hr = S_OK;
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY
    return hr;
}
        
 /* [hidden] */ HRESULT STDMETHODCALLTYPE 
CXMLDSOControl::get_readyState( 
    /* [retval][out] */ long __RPC_FAR *state)
{
    HRESULT hr;
    STACK_ENTRY_IUNKNOWN(this);
    TRY
    {
        *state = getWrapped()->getDoc()->getReadyStatus();
        hr = S_OK;
    } 
    CATCH 
    {
        hr = ERESULTINFO;
    }
    ENDTRY
    return hr;
}
