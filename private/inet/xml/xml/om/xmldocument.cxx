/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
// XMLDocument.cxx : Implementation of CXMLDocument
#include "core.hxx"
#pragma hdrstop

#include "xmldocument.h"
#include "xml/om/document.hxx"
#include "xml/islands/peer.hxx"
#include "xml/islands/xmlas.hxx"
#include "xml/dso/dsoctrl.hxx"

extern TAG tagMutex;

HRESULT __stdcall
CreateDocument(REFIID iid, void **ppvObj)
{
    HRESULT hr;

    TRY
    {
        Document * doc = Document::newDocument();
        doc->setDOM(false);
        hr = doc->QueryInterface(iid, ppvObj);

        // MSXML version 1 compatibility
        doc->setCaseInsensitive(true);
        doc->setIgnoreDTD(true);
        doc->setOmitWhiteSpaceElements(true);
        doc->setParseNamespaces(false);
        doc->setIe4Compatibility(true);
        doc->setResolveExternals(false);
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


HRESULT __stdcall
CreateDOMFreeThreadedDocument(REFIID iid, void **ppvObj)
{
    HRESULT hr;

    TRY
    {
        Document * doc = Document::newDocument();
        // Tell this doc to publish the DOM interface.
        doc->setAsync(true); // bug 15484
        doc->setDOM(true);
        hr = doc->QueryInterface(iid, ppvObj);
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


HRESULT __stdcall
CreateDOMDocument(REFIID iid, void **ppvObj)
{
#ifdef RENTAL_MODEL
    Model model(Rental);
#endif
    return CreateDOMFreeThreadedDocument(iid, ppvObj);
}


HRESULT __stdcall
CreateXMLIslandPeer(REFIID iid, void **ppvObj)
{
    HRESULT hr = E_FAIL;

#ifdef RENTAL_MODEL
    Model model(Rental);
#endif
    //EnableTag(tagMutex, true);
    
    TRY
    {
        DSODocument * doc = DSODocument::newDSODocument();
        // Tell this doc to publish the DOM interface.
        doc->setDOM(true);
        doc->setAsync(true);    // bug 37781
        CXMLIslandPeer * XMLIslandPeer = new CXMLIslandPeer(doc);
        hr = XMLIslandPeer->QueryInterface(iid, ppvObj);
        XMLIslandPeer->Release();
    }
    CATCH
    {
#ifdef RENTAL_MODEL
        model.Release();
#endif
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}


HRESULT __stdcall
CreateXMLScriptIsland(REFIID iid, void **ppvObj)
{
    HRESULT hr = E_FAIL;

#ifdef RENTAL_MODEL
    Model model(Rental);
#endif
    TRY
    {
        IUnknown *pUnk = CXMLScriptEngineConstruct();

        if (NULL != pUnk)
        {
            hr = pUnk->QueryInterface(iid, ppvObj);
            pUnk->Release();
        }
    }
    CATCH
    {
#ifdef RENTAL_MODEL
        model.Release();
#endif
        hr = ERESULT;
    }
    ENDTRY

    return hr;
}
