/*
 * @(#)safectrl.cxx 1.0 8/14/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#ifndef _SAFECTRL_HXX
#include "safectrl.hxx"
#endif

#ifdef UNIX
#include "messages.h"
#endif

#include "xml/tokenizer/net/url.hxx"

DEFINE_CLASS_MEMBERS_NEWINSTANCE(CSafeControl, _T("CSafeControl"), GenericBase);


CSafeControl::CSafeControl()
{
}


void
CSafeControl::CloneBase(void *pvClone)
{
    CSafeControl *pscClone = (CSafeControl*)pvClone;

    pscClone->_pSite = _pSite;
    pscClone->_pBaseURL = (String*)_pBaseURL;
    pscClone->_pSecureBaseURL = (String*)_pSecureBaseURL;
    pscClone->_dwSafetyOptions = _dwSafetyOptions;
}


void
CSafeControl::finalize()
{
    _pSite = null;
    super::finalize();
}


void
CSafeControl::setSite(IUnknown* u)
{
    BOOL fSetURLs = (!_pSite && u);         // only do this the first time
    
    _pSite = u;
    
    if (fSetURLs)
    {
        SetBaseURLFromSite(_pSite);
        SetSecureBaseURLFromSite(_pSite);   // can be different.
    }
} 


void
CSafeControl::getSite(REFIID riid, void** pUnk)
{
    if (!_pSite)
        *pUnk=null;
    else
        checkhr(_pSite->QueryInterface(riid, pUnk));
}


//
// Security checking
//
// The Document and DownloadManager share the responsibilities of security checking:
// 1.What the Document does: 
//   1) checking safetyOptiones to decide whether security checking is required 
//   2) geting base URL and passing it to DownloadManager
// 2. What the DownloadManager does:
//   1) if base URL is null, no security checking
//   2) otherwise, check security violation
void
CSafeControl::getBaseURL()
{
    if (_pSite == null)
    {
        // Carl Edlund in Trident requires that if we have safety options but no site then
        // we return E_ACCESSDENIED since someone has requested safety but there is no
        // site to use to ensure that safety - so deny everything !
        if (_dwSafetyOptions != 0)
        {
            Exception::throwE(E_ACCESSDENIED, XMLOM_ACCESSDENIED, null);
        }
    }
    else
    {        
        if (_dwSafetyOptions != 0 && _pSecureBaseURL == null)
        {
            Exception::throwE(E_ACCESSDENIED, XMLOM_ACCESSDENIED, null);
        }

        if (_pBaseURL == null)
        {
            // Otherwise, if we have a site, we need to get the base URL so we can resolve
            // relative URL's.  We always need to do this regardless of safety options 
            // because in disabled security mode, IE does NOT set any safety options !!
            // Also Carl Edlund requires that if we already have a baseURL we don't blow
            // it away with the base url from the site.
            SetBaseURLFromSite(_pSite);
        }
    }
}

String* _getBaseURLFromSite(IUnknown* pSite)
{
    IServiceProvider*   pSP = NULL;
    IBindHost*          pBH = NULL;
    IMoniker*           pMk = NULL;
    IHTMLDocument2*     pHtmlDoc = NULL;
    IXMLDOMDocument*    pXmlDoc = NULL;
    LPOLESTR            pOleStr;
    BSTR                pDocURL;
    HRESULT             hr;
    String*             pResult = null;
    BSTR                bstrHref = ::SysAllocString(L"HREF");

    if (NULL == bstrHref)
    {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    TRY
    {
        if (FAILED(hr = pSite->QueryInterface(IID_IServiceProvider, (void**)&pSP)))
        {
            // see if we can get an HTMLDocument anyway, and if so we can try and find
            // the base URL manually.
            if (SUCCEEDED(hr = pSite->QueryInterface(IID_IHTMLDocument2, (void**)&pHtmlDoc)))
                goto manual;

            goto CleanUp;
        }

        if (SUCCEEDED(hr = pSP->QueryService(SID_SBindHost, IID_IBindHost, (void**)&pBH)))
        {
            // This is the <OBJECT> or <XML> tag approach...
            // This also is the path if we're instantiated from XSL Script
            if (FAILED(hr = pBH->CreateMoniker(_T("."), NULL, &pMk, 0)))
                goto CleanUp;

            if (SUCCEEDED(hr = pMk->GetDisplayName(NULL, NULL, &pOleStr)))
            {
                pResult = String::newString(pOleStr);
                ::CoTaskMemFree((LPVOID)pOleStr);
            }
        }
        else
        {
            // Well, so now we have to find the base URL manually because this has to
            // work under IE4 !!  This is done by walking the document all collection
            // until you see an element named "BASE".  We give up when we see the elemnet
            // named "BODY".  We cannot use getAttribtue(L"SRC",GETMEMBER_ABSOLUTE,...)
            // because this doesn't work under IE4.

            if (FAILED(hr = pSP->QueryService(SID_SContainerDispatch, IID_IHTMLDocument2, (void **)&pHtmlDoc)))
            {
                // Well, maybe we can go through the IID_IInternetHostSecurityManager interface.
                // (This happens in the scriptlet case).
                if (FAILED(pSP->QueryService(IID_IInternetHostSecurityManager, IID_IHTMLDocument2, (void **)&pHtmlDoc)))
                    goto CleanUp;
            }
manual:
            IHTMLElementCollection * pAll;
            hr = pHtmlDoc->get_all(&pAll);
            if (!hr && NULL != pAll)
            {
                long lLength = 0;
                pAll->get_length(&lLength);
                for (long i = 0; i < lLength; i++)
                {
                    VARIANT varIndex;
                    VariantInit(&varIndex);
                    varIndex.vt = VT_I4;
                    V_I4(&varIndex) = i;
                    VARIANT varEmpty;
                    VariantInit(&varEmpty);
                    IDispatch * pDisp = NULL;
                    hr = pAll->item(varIndex,varEmpty,&pDisp);
                    if (pDisp != NULL)
                    {
                        IHTMLElement* e = NULL;
                        hr = pDisp->QueryInterface(IID_IHTMLElement,(void**)&e);
                        if (e != NULL)
                        {
                            BSTR bstrTagName = NULL;
                            hr = e->get_tagName(&bstrTagName);
                            if (bstrTagName != NULL)
                            {
                                int len = ::SysStringLen(bstrTagName);
                                if (_tcsnicmp(bstrTagName, L"BASE",len) == 0)
                                {
                                    VARIANT varHref;
                                    VariantInit(&varHref);
                                    hr = e->getAttribute(bstrHref,0,&varHref);
                                    if (varHref.vt == VT_BSTR && NULL != V_BSTR(&varHref))
                                    {
                                        BSTR href = V_BSTR(&varHref);
                                        len = ::SysStringLen(href);
                                        pResult = String::newString(href,0,len);
                                    }
                                    VariantClear(&varHref);
                                    i = lLength; // make sure we terminate loop AND free BSTR
                                }
                                else if (_tcsnicmp(bstrTagName, L"BODY",len) == 0)
                                {
                                    i = lLength; // make sure we terminate loop AND free BSTR
                                }
                                ::SysFreeString(bstrTagName);
                            }
                            e->Release();
                        }
                        pDisp->Release();
                    }
                }
                pAll->Release();
            }

            // Now we may have a relative base url which we need to resolve
            // against the document url.
            if (FAILED(hr = pHtmlDoc->get_URL(&pDocURL)))
                goto CleanUp;

            if (pResult)
            {
                URL url;
                hr = url.set(pResult->getWCHARPtr(), pDocURL);
                if (!hr)
                {
                    pResult = String::newString(url.getResolved());
                }
            }
            if (! pResult)
            {
                pResult = String::newString(pDocURL);
            }
            ::SysFreeString(pDocURL);
        }
    }
    CATCH
    {
        // Oh, well.  No base URL then....
    }
    ENDTRY

CleanUp:
    release(&pMk);
    release(&pBH);
    release(&pSP);
    release(&pHtmlDoc);
    release(&pXmlDoc);
    ::SysFreeString(bstrHref);

    return pResult;

} 

// Also called from CXMLIslandPeer::Init()
void
CSafeControl::SetBaseURLFromSite(IUnknown* pSite)
{
    _pBaseURL = _getBaseURLFromSite(pSite);
}


// The base URL for use in security checking is always the IHTMLDocument page location
// regardless of <BASE> tag.
void
CSafeControl::getSecureBaseURL()
{
    if (!_dwSafetyOptions)
    {
        return;
    }
    if (_pSite != null)
        SetSecureBaseURLFromSite(_pSite);
}


HRESULT
CSafeControl::SetSecureBaseURLFromSite(IUnknown* pSite)
{
    IOleClientSite*    pClientSite = NULL;
    IMoniker*          pMk = NULL;
    IServiceProvider*  pSP = NULL;
    IHTMLDocument2*    pHtmlDoc = NULL;
    IXMLDOMDocument*   pXMLDOMDoc = NULL;
    BSTR               pDocURL = NULL;
    LPOLESTR           pOleStr;
    HRESULT            hr;

    TRY
    {

        // first try coolio document to document security
        // if the passed in site is actually an IDOMDocument, then grab the URL from it
        // this allows on xml document's security to based on anothers'
        if (SUCCEEDED(pSite->QueryInterface(IID_IXMLDOMDocument, (void **)&pXMLDOMDoc)))
        {
            Assert(pXMLDOMDoc != null);
            if (FAILED(hr = pXMLDOMDoc->get_url(&pDocURL)))
                goto Error;
            // BUGBUG this code is not low-memory exception safe (it will leak).
            _pSecureBaseURL = _pBaseURL = String::newString(pDocURL);
            ::SysFreeString(pDocURL);
        }            


        // The jscript "new ActiveXObject" and the intrinsic <OBJECT> tag
        // don't take us down the same road.  
        else if (SUCCEEDED(pSite->QueryInterface(IID_IOleClientSite, (void **)&pClientSite)))
        {
            Assert(pClientSite != null);
            
            //  <OBJECT> tag path, and <XML> tag path
            // This is the latest way to do this from Loren Kohnfelder on 5/28/98
            if (FAILED(hr = pClientSite->GetMoniker(OLEGETMONIKER_ONLYIFTHERE, OLEWHICHMK_CONTAINER, &pMk)))
                goto Error;
            
            if (FAILED(hr = pMk->GetDisplayName(NULL, NULL, &pOleStr)))
                goto Error;
            
            if (pOleStr != NULL)
            {
                _pSecureBaseURL = String::newString(pOleStr);
                ::CoTaskMemFree((LPVOID)pOleStr);
            }

// This is the old code.  Checking on scriptlet case now...
//            IOleContainer*     pContainer = NULL;
//            IHTMLDocument*     pHD = NULL;
//            IHTMLWindow2*      pHW = NULL;
//            IDispatch*         pdisp = NULL;
//            checkhr(pClientSite->GetContainer(&pContainer));
//            if (FAILED(pContainer->QueryInterface(IID_IHTMLDocument2, (void **)&pHtmlDoc)))
//            {
//            // Maybe we're in a scriptlet, so let's try the round-about approach
//
//                 checkhr(pContainer->QueryInterface(IID_IHTMLDocument, (LPVOID *)&pHD));
//                 checkhr(pHD->get_Script(&pdisp));
//                 checkhr(pdisp->QueryInterface(IID_IHTMLWindow2, (LPVOID *)&pHW));
//                 checkhr(pHW->get_document(&pHtmlDoc));
//            }
//            release(&pContainer);
//            release(&pHD);
//            release(&pHW);
//            release(&pdisp);
        }
        else
        {
            //  jscript path (new ActiveXObject("msxml")) 
            if (FAILED(hr = pSite->QueryInterface(IID_IServiceProvider, (void **)&pSP)))
                goto Error;

            // now check if we were instantiated as part of an XSL script
            if (SUCCEEDED(hr = pSP->QueryService(IID_IInternetHostSecurityManager, IID_IXMLDOMDocument, (void**)&pXMLDOMDoc)))
            {
                if (pXMLDOMDoc != null)
                {
                    if (FAILED(hr = pXMLDOMDoc->get_url(&pDocURL)))
                        goto Error;
                    _pSecureBaseURL = String::newString(pDocURL);
                    ::SysFreeString(pDocURL);
                }
            }

            /* now go for the html doc */
            else
            {
                if (FAILED(hr = pSP->QueryService(SID_SContainerDispatch, IID_IHTMLDocument2, (void **)&pHtmlDoc)))
                {
                    if (FAILED(hr = pSP->QueryService(IID_IInternetHostSecurityManager, IID_IHTMLDocument2, (void **)&pHtmlDoc)))
                        goto Error;
                }
                if (pHtmlDoc != null)
                {
                    if (FAILED(hr = pHtmlDoc->get_URL(&pDocURL)))
                        goto Error;
                    _pSecureBaseURL = String::newString(pDocURL);
                    ::SysFreeString(pDocURL);
                }
            }
        } /* jscript path */
        
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY

Error:
    release(&pClientSite);
    release(&pMk);
    release(&pSP);
    release(&pHtmlDoc);
    release(&pXMLDOMDoc);

    return hr;
}


void
CSafeControl::getInterfaceSafetyOptions(
                REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;
    *pdwEnabledOptions = _dwSafetyOptions & (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA);
}


void
CSafeControl::setInterfaceSafetyOptions(
                REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    _dwSafetyOptions = dwOptionSetMask & dwEnabledOptions;
}

