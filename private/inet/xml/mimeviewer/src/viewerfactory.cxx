/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include "xml/dll/resource.h"
#include "utils.hxx"
#include "expando.hxx"
#include "viewerfactory.hxx"
#include "core/lang/string.hxx"
#include "core/util/resources.hxx"

#if XSLASYNC
#include "xtl/engine/xtlprocessor.hxx"
static UINT throttle = 128;
#endif
#include <stdio.h>
#include <mshtml.h>
#include <objsafe.h>

#ifndef _CORE_UTIL_CHARTYPE_HXX
#include "core/util/chartype.hxx"
#endif // _CORE_UTIL_CHARTYPE_HXX


#define    EOL  L"\r\n"

// Helper function to append a String onto a BSTR and return the new BSTR, 
// freeing the old BSTR.
BSTR AppendBSTR(BSTR bstr, String* s)
{
    s = String::add(String::newString(bstr), s, null);
    BSTR bstrCombined = s->getBSTR();
    if (bstrCombined)
    {
        ::SysFreeString(bstr);
        bstr = bstrCombined;
    }
    return bstr;
}

// Helper function for formatting XML source information for error messages.
// It returns NULL if it fails for some reason.
BSTR FormatSourceInfo(IXMLDOMParseError* pError)
{
    BSTR bstrSource = NULL;
    long lPos,lLine;

    TRY
    {
        pError->get_srcText(&bstrSource);
        pError->get_line(&lLine);
        pError->get_linepos(&lPos);
        if (bstrSource != NULL)
        {
            // Write out source text, converting tabs to spaces
            // and < to &lt; so the formatting works.
            long i,len;
            StringBuffer* msg = StringBuffer::newStringBuffer();
            len = ::SysStringLen(bstrSource);
            for (i = 0; i < len; i++)
            {
                WCHAR ch = bstrSource[i];
                switch (ch)
                {
                case '\t':
                    msg->append(L' ');
                    break;
                case '<':
                    msg->append(L"&lt;");
                    break;
                default:
                    msg->append(ch);
                    break;
                }
            }
            msg->append(EOL);
            if (lPos < len)
            {
                for (i = 1; i < lPos; i++)
                {
                    msg->append(L'-');
                }
                msg->append(L'^');
            }
            BSTR formattedSource = msg->toString()->getBSTR();
            if (formattedSource)
            {
                ::SysFreeString(bstrSource);
                bstrSource = formattedSource;
            }
        }
    }
    CATCH
    {
    }
    ENDTRY
    return bstrSource;
}

#if 0
/*
BSTR FormatSourceInfo(IXMLNodeSource* pSource)
{
    BSTR bstrSource = NULL;
    TRY
    {
        const WCHAR* pwcBuf = NULL;
        ULONG ulLen = 0, ulStart = 0;
        HRESULT hr = pSource->GetLineBuffer(&pwcBuf, &ulLen, &ulStart);
        if (pwcBuf != NULL)
        {                
            // Write out source text, converting tabs to spaces
            // and < to &lt; so the formatting works.
            ULONG i;
            StringBuffer* msg = StringBuffer::newStringBuffer();
            for (i = 0; i < ulLen; i++)
            {
                WCHAR ch = pwcBuf[i];
                switch (ch)
                {
                case '\t':
                    msg->append(L' ');
                    break;
                case '<':
                    msg->append(L"&lt;");
                    break;
                default:
                    msg->append(ch);
                    break;
                }
            }
            msg->append(EOL);
            for (i = 1; i < ulStart; i++)
            {
                msg->append(L'-');
            }
            msg->append(L'^');
            bstrSource = msg->toString()->getBSTR();
        }
    }
    CATCH
    {
    }
    ENDTRY
    return bstrSource;
}
*/
#endif

ViewerFactory::ViewerFactory(MIMEBufferedStream* pStm, IXMLNodeFactory *pDefaultFactory, IXMLDOMDocument *pDoc, IMoniker *pimkXML, CallbackMonitor *pMonitor, HANDLE evtLoad)
{

    _lcDTD = _lcEntity = 0;
    _piType = PITYPE_NONE;
    _fInAttr = false;
    _fElement = false;
    _fInProlog = true;
    _fBuildTree = true;
    _fRootElem = false;
    _dwErrorCode = 0;
    _fErrReported = false;
    _fGenericParse = false;
    _fXSLError = false;
    _fDocType = false;

    _pStm = pStm;
    _pStm->AddRef();

    _pDefaultFactory = pDefaultFactory;
    _pDefaultFactory->AddRef();

    _pDOMDocument = NULL;
    pDoc->QueryInterface(IID_IXMLDOMDocument, (void**)&_pDOMDocument);

    _pimkXML = pimkXML;
    _pimkXML->AddRef();

    _pXSLDocument = NULL;

    _ssType = SSTYPE_NONE;
    _wURL = NULL;       //!! here we may set a default style sheet

#if XSLASYNC
    _pIXTL = NULL;
    _nSignals = 0;
    _fInitAsync = false;
    _bXSLAsyncErr = NULL;
#endif

#if MIMEASYNC
    _fIsAsync = _fStopAsync = false;
    _evtLoad = evtLoad;
    _pbcsMonitor = pMonitor;
    _pbcsMonitor->AddRef();
#endif   

    // Write a unicode file so we don't lose any information.
    DWORD written;
    _pStm->Write(s_ByteOrderMarkTrident, sizeof(WCHAR), &written);

    CSSWrite(L"<html:html>");
    CSSWrite(L"<html:head>");
}

ViewerFactory::~ViewerFactory()
{
    SafeFreeString(_wURL);
    SafeRelease(_pXSLDocument);
    SafeRelease(_pDOMDocument);
    SafeRelease(_pimkXML);
    SafeRelease(_pDefaultFactory);
#if MIMEASYNC
    SafeRelease(_pbcsMonitor);
#endif
    freeStream();
#if XSLASYNC
    SafeFreeString(_bXSLAsyncErr);
    closeXSL();
#endif
}
    
HRESULT STDMETHODCALLTYPE 
ViewerFactory::NotifyEvent( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ XML_NODEFACTORY_EVENT iEvt)
{
    HRESULT hr = S_OK;
    AddRef();   /* add ref first - do not return without releasing this! */

    // ok let's delegate first
    if (_fBuildTree)
    {
        hr = _pDefaultFactory->NotifyEvent(pSource, iEvt);
        if (!SUCCEEDED(hr))
        {
            goto Cleanup;
        }
    }

    switch (iEvt)
    {
    case XMLNF_STARTDOCUMENT:
        hr = setGenericParse(VARIANT_TRUE);
        if (!SUCCEEDED(hr))
            break;    
        _dwErrorCode = 0;
        break;
    case XMLNF_STARTDTD:
    case XMLNF_STARTDTDSUBSET:
        _fDocType = false;  // let the dtd counting take over
        _lcDTD++;
        break;
    case XMLNF_ENDDTD:
    case XMLNF_ENDDTDSUBSET:
        _lcDTD--;
        break;
    case XMLNF_STARTENTITY:
        _lcEntity++;
        break;
    case XMLNF_ENDENTITY:
        _lcEntity--;
        break;
#if MIMEASYNC
    case XMLNF_DATAAVAILABLE:
        // if we're downloading on the worker thread, then return E_PENDING
        // so we get timesliced
        if (_fIsAsync /* && !_fInProlog */)
        {
            hr = E_PENDING;
            goto Cleanup;
        }
        break;
#endif
    case XMLNF_ENDPROLOG:
        if (_ssType == SSTYPE_CSS)
        {
            // inject special namespace for our dummy nodes
            CSSWrite(L"<XML:NAMESPACE prefix='XMV'/>");
            CSSWrite(L"</html:head>");
            _fInProlog = false;
            _fRootElem = true;
            CSSWrite(L"<html:body>");
            _pStm->EnableRead();
//          _fBuildTree = false;
        }
        else   // we're gonna load XSL
        {
            _fInProlog = false;

            hr = setGenericParse(VARIANT_FALSE);
            if (!SUCCEEDED(hr))
                break;

            // now let's set the document's security
            hr = setDocSecurity(_pDOMDocument);
            if (!SUCCEEDED(hr))
                break;

            // get the doc and load from the URL
            hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IXMLDOMDocument, (void**)&_pXSLDocument);
            if (!SUCCEEDED(hr))
                break;

            hr = setSecurity(_pDOMDocument, _pXSLDocument);
            if (!SUCCEEDED(hr))
                break;
            
            if (_ssType == SSTYPE_XSL)
                hr = loadSS(_pXSLDocument);
            else
            {
                // load the default stylesheet
                _ssType = SSTYPE_XSL;
                hr = loadSSDefault(_pXSLDocument);
            }

            if (!SUCCEEDED(hr))
                break;

            addExpandos();

            _pStm->Revert();

            // Write a unicode file so we don't lose any information.
            DWORD written;
            _pStm->Write(s_ByteOrderMarkTrident, sizeof(WCHAR), &written);
        }
        
#if MIMEASYNC
        if (StartAsync(pSource))
            hr = E_PENDING;     // stop parser on originator thread
#endif
        break;
    case XMLNF_ENDDOCUMENT:
        // this one's probably superfluous too now that we report everything through
        // error 
        if (_dwErrorCode != 0)
        {
            reportError(_dwErrorCode);
            break;
        }
        if (_ssType == SSTYPE_CSS)
        {
            CSSWrite(L"</html:body>");
            CSSWrite(L"</html:html>");
        }
        else
#if XSLASYNC
            hr = processXSLAsync(pSource, TRUE);
#else
        {
            IXMLDOMElement *pXSLRoot = NULL;
            IXMLDOMNode *pXSLTree = NULL;
            BSTR p = NULL;
            hr = _pXSLDocument->get_documentElement(&pXSLRoot);
            if (!SUCCEEDED(hr)) goto Backout2;
            if (!pXSLRoot)
            {
                hr = E_FAIL;        // hard failure, convert from S_FALSE
                goto Backout2;
            }
            hr = pXSLRoot->QueryInterface(IID_IXMLDOMNode, (void **)&pXSLTree);
            if (!SUCCEEDED(hr)) goto Backout2;
            hr = _pDOMDocument->transformNode(pXSLTree, &p);
            if (!SUCCEEDED(hr)) goto Backout2;
            if (!p)
            {
                hr = E_FAIL;        // hard failure, convert from S_FALSE
                goto Backout2;
            }
            // Write a unicode file so we don't lose any information.
            // Don't allow Trident to read until least one tag is present (first transform Node call)
            // otherwise it tries to wrap the HTML tag around the unicode mark and we get pure text
            DWORD written;
            _pStm->Write(s_ByteOrderMarkTrident, sizeof(WCHAR), &written);
            write(p);
            _pStm->EnableRead();
Backout2:
            SafeFreeString(p);
            SafeRelease(pXSLTree);
            SafeRelease(pXSLRoot);
        }
#endif
        if (SUCCEEDED(hr))
        {
            _pStm->Commit(0);
            // don't free the stream yet, need to use wnd to post to
//          freeStream();
        }
        else
            // since we're in END_DOCUMENT, we can't just return the error and expect our factory::Error to be
            // called.  We have to report the error manually ourselves. 
            // Is this assumption true anymore?
            reportError(hr);
        break;
    default:
        break;
    }

Cleanup:
    Release();  
    return hr;
}


HRESULT STDMETHODCALLTYPE 
ViewerFactory::BeginChildren( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
{
    HRESULT hr = S_OK;

#if MIMEASYNC
    if (_fStopAsync)
        return E_FAIL;
#endif

    // do the XSL part, build the tree
    if (_fBuildTree)
    {
        hr = _pDefaultFactory->BeginChildren(pSource, pNodeInfo);
        if (!SUCCEEDED(hr))
            return hr;
    }

    CSSWrite(L">");
#if XSLASYNC
    hr = signalAsync(pSource);
#endif
    _fInAttr = false;
    _fElement = false;

    return hr;
}


HRESULT STDMETHODCALLTYPE 
ViewerFactory::EndChildren( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ BOOL fEmpty,
    /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
{
    HRESULT hr = S_OK;

#if MIMEASYNC
    if (_fStopAsync)
        return E_FAIL;
#endif
    
    if (_fBuildTree)
    {
        hr = _pDefaultFactory->EndChildren(pSource, fEmpty, pNodeInfo);
        if (!SUCCEEDED(hr))
            return hr;
    }

    switch (pNodeInfo->dwType)
    {
    case XML_PI:
    case XML_XMLDECL:
        _piType = PITYPE_NONE;
        break;
#if 0    
    case XML_ATTRIBUTE:
    case XML_XMLLANG:
    case XML_XMLSPACE:
        if (_piType != PITYPE_XML)
            CSSWrite(L"'");
        break;
#endif
    case XML_ELEMENT:
        // trident CSS rules mess up if we have <TAG/>, so write explicit </TAG> instead
        if (fEmpty)
        {
            CSSWrite(L">");
#if XSLASYNC
            hr = signalAsync(pSource);
#endif
        }
        CSSWrite(L"</");
        CSSWriteInfo(pNodeInfo);
        CSSWrite(L">");
        break;
    }
    _fInAttr = false;
    _fElement = false;
    return hr;
}

HRESULT STDMETHODCALLTYPE 
ViewerFactory::CreateNode( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ PVOID pNodeParent,
    /* [in] */ USHORT cNumRecs,
    /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
{
    HRESULT hr;
    BOOL fGenNMSP;

#if MIMEASYNC
    if (_fStopAsync)
        return E_FAIL;
#endif
    
    fGenNMSP = ((*apNodeInfo)->pReserved != NULL);   // because delegation will change pNodeInfo
    if (_fBuildTree)
    {
        hr = _pDefaultFactory->CreateNode(pSource, pNodeParent, cNumRecs, apNodeInfo);
        if (!SUCCEEDED(hr))
            return hr;
    }
    XML_NODE_INFO* pNodeInfo = *apNodeInfo;

    // may need to parse the node for namespaces
    if (_fRootElem && pNodeInfo->dwType == XML_ELEMENT)
    {
        _fRootElem = false;
        _lcDTD = _lcEntity = 0;     // just in case prolog processing left us inbalanced
        if (fGenNMSP) // optimization that tells node factory that there
                                  // is an xmlns attribute.
        {
            hr = generateNamespaces(cNumRecs, apNodeInfo); 
            if (!SUCCEEDED(hr))
                return hr;
        }
    }
    
    // now generate the tags
    return generateTag(cNumRecs, apNodeInfo);
}
    
HRESULT 
ViewerFactory::generateTag(USHORT cNumRecs, XML_NODE_INFO **apNodeInfo)
{
    USHORT cRecs = cNumRecs;
    
    while (cRecs-- > 0)
    {
        XML_NODE_INFO *pInfo = *apNodeInfo++;

        // do the detection and CSS parts
        switch (pInfo->dwType)
        {
        case XML_PI:
            _piType = PITYPE_OTHER;
            if (_fInProlog) 
            {                
                if (pInfo->ulNsPrefixLen==0)
                {
                    if ( (StrCmpNI(pInfo->pwcText, L"xml-stylesheet", 14)== 0) ||
                         (StrCmpNI(pInfo->pwcText, L"xml:stylesheet", 14)== 0) )
                            _piType = PITYPE_STYLESHEET;
                }
            }
            else  /* inject a dummy PI into the object model */
            {
                CSSWrite(L"<XMV:XMVPI omn='");
                CSSWriteInfo(pInfo);
                CSSWrite(L"' ");
                _piType = PITYPE_BODY;
            }
            _fElement = false;
            break;
        case XML_XMLDECL:
            _piType = PITYPE_XML;
            _fElement = false;
            break;
        case XML_ELEMENT:
            CSSWrite(L"<");
            CSSWriteInfo(pInfo);
            _fElement = true;
            _fInAttr = false;
            break;
        case XML_ATTRIBUTE:
        case XML_XMLLANG:
        case XML_XMLSPACE:
            if (_piType != PITYPE_XML)
            {
                if (_fInAttr)
                    // gotta close the previous one
                    CSSWrite(&_chQuote,1);
                _fInAttr = true;
                _chQuote = 0;
                CSSWrite(L" ");
                CSSWriteInfo(pInfo);
                CSSWrite(L"=");
            }
            break;
        case XML_PCDATA:
            if (_lcEntity == 0 && _piType == PITYPE_NONE)
            {
                if (_fInAttr && ! _chQuote)
                {
                    _chQuote = (WCHAR)pInfo->pReserved;
                    CSSWrite(&_chQuote,1);
                }
                CSSWriteInfo(pInfo);
            }
            break;
        case XML_COMMENT:
            CSSWrite(L"<!--");
            CSSWriteInfo(pInfo);
            CSSWrite(L"-->");
            break;
            // the next set of node types, we need to create dummy nodes
        case XML_CDATA:
            if (_piType == PITYPE_STYLESHEET)
            {
                if (_ssType != SSTYPE_XSL)
                    detectStylesheet(pInfo->pwcText, pInfo->ulLen);
                CSSWrite(L"<html:LINK rel=\"stylesheet\" ");
                CSSWriteInfo(pInfo);
                CSSWrite(L">");
            }
            else if (_piType == PITYPE_BODY)
            {
                CSSWrite(L"omv='");
                CSSWriteInfo(pInfo);
                CSSWrite(L"'></XMV:XMVPI>");
            }
            else if (_piType != PITYPE_XML && _piType != PITYPE_OTHER)
            {
                CSSWrite(L"<XMV:XMVCD>");
                CSSWriteCDATA(pInfo);
                CSSWrite(L"</XMV:XMVCD>");
            }
            break;
        case XML_ENTITYREF:
//          Comment out the ghost entity node until CSS scripting is truly supported
//          if (!_fInAttr)
//          {
//              CSSWrite(L"<XMV:XMVER omn='");
//              CSSWriteInfo(pInfo);
//              CSSWrite(L"'></XMV:XMVER>");
//          }
            if (!_fInProlog && _lcEntity == 0)
                expandEntity(pInfo);
            break;
        case XML_WHITESPACE:
            // only write if not in prolog, for perf
            if (!_fInProlog)
                CSSWriteInfo(pInfo);
            break;
        case XML_DOCTYPE:
            _fDocType = true;
            break;
        }
        pInfo++;
    }
    if (_fInAttr)
        // gotta close up the last one
        CSSWrite(&_chQuote,1);

    _fInAttr = false;
    return S_OK;
}

HRESULT 
ViewerFactory::generateNamespaces(USHORT cNumRecs, XML_NODE_INFO **apNodeInfo)
{
    USHORT cRecs = cNumRecs;
    
    while (cRecs-- > 0)
    {
        XML_NODE_INFO *pInfo = *apNodeInfo++;

        switch (pInfo->dwType)
        {
        case XML_ELEMENT:
            _fElement = true;
            break;
        case XML_ATTRIBUTE:
            _fInAttr = true;
            // check for namespace declarations, don't support defaults
            if (pInfo->ulNsPrefixLen == 5 && pInfo->ulLen > 6 &&       
                StrCmpNI(pInfo->pwcText, L"xmlns", 5) == 0)
            {
                CSSWrite(L"<XML:NAMESPACE prefix='");
                CSSWrite(&pInfo->pwcText[6], pInfo->ulLen - 6);
                CSSWrite(L"'/>");

            }
            break;
        }

        pInfo++;
    }
    _fInAttr = false;
    _fElement = false;
    return S_OK;
}

HRESULT 
ViewerFactory::expandEntity(XML_NODE_INFO *pNodeInfo)
{
    HRESULT hr;
    BSTR bstrName = NULL;
    BSTR bstrText = NULL;
    IXMLDOMDocumentType *pDocType = NULL;
    IXMLDOMNamedNodeMap *pEntityMap = NULL;
    IXMLDOMNode *pEntity = NULL;

    Assert(_pDOMDocument);

    bstrName = ::SysAllocStringLen(pNodeInfo->pwcText, pNodeInfo->ulLen);
    CHECKALLOC(hr, bstrName);
    hr = _pDOMDocument->get_doctype(&pDocType);
    CHECKHRPTR(hr, pDocType);
    hr = pDocType->get_entities(&pEntityMap);
    CHECKHRPTR(hr, pEntityMap);
    hr = pEntityMap->getNamedItem(bstrName, &pEntity);
    CHECKHRPTR(hr, pEntity);
    hr = pEntity->get_text(&bstrText);
    CHECKHRPTR(hr, bstrText);
    CSSWrite((LPCWSTR)bstrText);
CleanUp:
    SafeFreeString(bstrText);
    SafeRelease(pEntity);
    SafeRelease(pEntityMap);
    SafeRelease(pDocType);
    SafeFreeString(bstrName);
    return hr;
}

  
HRESULT STDMETHODCALLTYPE 
ViewerFactory::Error( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ HRESULT hrErrorCode,
    /* [in] */ USHORT cNumRecs,
    /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)

{
    IXMLDOMParseError* pError = NULL;

    TRY
    {
        _dwErrorCode = hrErrorCode; // remember that we saw an error.

        // Now see if we have an parse error to report, and if so, report it now, 
        // before the factories change it
        IXMLDOMDocument* pDoc = _fXSLError ? _pXSLDocument : _pDOMDocument;
        HRESULT hr = pDoc->get_parseError( &pError);
        if (SUCCEEDED(hr)) 
        {
            pError->get_errorCode(&hr);
            if (S_OK != hr)
            {
                reportError(hr);
            }
            SafeRelease(pError);
        }     

        if (_fBuildTree)
            _pDefaultFactory->Error(pSource, hrErrorCode, cNumRecs, apNodeInfo);

        // if we didn't have good parse error information before and now
        // we do as a result of delegating to the default factories, 
        // then give it another crack.
        //
        // !fIsAsync may be superflous.  In that case we wait until 
        // END_DOCUMENT is called (if it is), or until we wait until we return
        // from Parser->Run.  I see no reason why I can't just report it here, 
        // but to be safe....
#if MIMEASYNC
        if (!SUCCEEDED(_dwErrorCode)  && !_fIsAsync && !isErrReported())
#else
        if (!SUCCEEDED(_dwErrorCode) && !isErrReported())
#endif
            reportError(_dwErrorCode);
    }
    CATCH
    {
    }
    ENDTRY

CleanUp:
    return hrErrorCode;
}


WCHAR heLT[] = L"&lt;";
WCHAR heGT[] = L"&gt;";
WCHAR heQUOT[] = L"&quot;";
WCHAR heAMP[]= L"&amp;";

BOOL ViewerFactory::IsCSSWrite(void)
{
    return ((!_fDocType) && (_lcDTD == 0) && (_fInProlog || _ssType != SSTYPE_XSL));
}


void ViewerFactory::CSSWriteCDATA(XML_NODE_INFO *pInfo)
{
    ULONG i;
    WCHAR *pCur, *pWrite;
    LPCWSTR pEnt;

    // perf don't scan if we're not writing
    if (IsCSSWrite())
    {    
        pCur = pWrite = (WCHAR *)pInfo->pwcText;
        for (i = 0; i < pInfo->ulLen; i++, pCur++) {
            switch (*pCur)
            {
            case L'<':
                pEnt = heLT;
                break;
            case L'>':
                pEnt = heGT;
                break;
            case L'"':
                pEnt = heQUOT;
                break;
            case L'&':
                pEnt = heAMP;
                break;
            default:
                pEnt = NULL;
                break;
            }
            if (pEnt)
            {
                // write string up to the entity
                if (pWrite != pCur)
                    CSSWrite(pWrite, (ULONG)(pCur-pWrite));
                CSSWrite(pEnt);
                pWrite = pCur + 1;
            }
        }
        // write out last string, this will be the normal case
        if (pWrite != pCur)
            CSSWrite(pWrite, (ULONG)(pCur-pWrite));
    }
}


void ViewerFactory::CSSWriteInfo(XML_NODE_INFO *pInfo)
{
    if (pInfo->ulLen)
        CSSWrite(pInfo->pwcText, pInfo->ulLen);
}



void
ViewerFactory::CSSWrite(const WCHAR* text, ULONG len)
{
    if (IsCSSWrite())
        write(text, len); 
}
    
void
ViewerFactory::write(const WCHAR* text, ULONG len)
{
    if (len == 0 && *text != 0)
    {
        len = lstrlenW(text);
    }
    if (len > 0 && _pStm != null)
    {
        ULONG written = 0;
        _pStm->Write(text, len*sizeof(WCHAR), &written);
    }
}



void 
ViewerFactory::writeError(BSTR bstrError, BSTR bstrSource)
{
    if (!_fErrReported && _pStm != null)
    {
        _fErrReported = true;
        String* s = Resources::FormatMessage(XMLMIME_ERROR,
                        _fGenericParse ? String::newString(L"html:") : String::emptyString(),
                        bstrError ? String::newString(bstrError) : String::emptyString(),
                        _ssType == SSTYPE_NONE ? String::emptyString() : String::newString((_ssType == SSTYPE_CSS) ? L"CSS" : L"XSL"),
                        bstrSource ? String::newString(bstrSource) : String::emptyString(),
                        null);

        ATCHAR* atext = s->toCharArrayZ();
        write(atext->getData());
        _pStm->EnableRead();

        _pStm->Commit(0);
#if XSLASYNC
        SafeFreeString(_bXSLAsyncErr);
#endif
    }
}

void
ViewerFactory::reportError(HRESULT errorCode)
{
    BSTR bstrError = NULL;
    BSTR bstrURL = NULL;
    IXMLDOMParseError* pError = NULL;
    BSTR bstrSource = NULL;
    long lPos,lLine = 0;
    TRY
    {
        HRESULT hr;

        // Ok, first we check the document to see if it contains the error.
        // (like duplicate ID's or something).
        IXMLDOMDocument* pDoc = _fXSLError ? _pXSLDocument : _pDOMDocument;

        hr = pDoc->get_parseError( &pError);
        if (FAILED(hr)) {
            // couldn't get the error info itself.  So display the error
            // getting error, formatting it ourselves.
            reportError2(hr);
            goto CleanUp;
        }
        
        pError->get_errorCode(&hr);
        if (hr == 0)
        {
            // Doc has no error - so fall back on formatting the error ourselves.
            reportError2(errorCode);
            goto CleanUp;
        }

        pError->get_reason(&bstrError);

        if (_fXSLError)
        {
            pError->get_url(&bstrURL);
            if (bstrURL != NULL && *bstrURL)
            {
                String* s = Resources::FormatMessage(XML_E_RESOURCE, String::newString(bstrURL), null);
                bstrError = AppendBSTR(bstrError, s);
            }
        }

        pError->get_line(&lLine);
        pError->get_linepos(&lPos);
        if (lLine > 0)
        {
            // Append the line number information to the error message.
            String* s = Resources::FormatMessage(XMLMIME_LINEPOS,
                String::valueOf((int)lLine),
                String::valueOf((int)lPos), null);
            bstrError = AppendBSTR(bstrError, s);
        }

        bstrSource = FormatSourceInfo(pError);

    }    
    CATCH
    {
        bstrError = NULL;
    }
    ENDTRY

    writeError(bstrError, bstrSource);

CleanUp:
    SafeRelease(pError);
    ::SysFreeString(bstrError);
    ::SysFreeString(bstrSource);
    ::SysFreeString(bstrURL);
}

void
ViewerFactory::reportError2(HRESULT hr)
{
    BSTR bstrError = NULL;
    IErrorInfo * pIErrorInfo = NULL;

    TRY
    {
        String *s;
        if ((ULONG)hr <= 0xC00CEFFF && (ULONG)hr >= 0xC00CE000)
            s = Resources::FormatMessage((ResourceID)hr, NULL);
        else if (::GetErrorInfo(0, &pIErrorInfo) == S_OK && pIErrorInfo && pIErrorInfo->GetDescription(&bstrError) == S_OK)
        {
            s = NULL;
        }
#if XSLASYNC
        // BUGBUG this is treated specially because the XSL async interface does
        // not support the ISupportErrorInfo.  If it does and sets the error info on ::execute(), 
        // this can be removed
        else if ((_bXSLAsyncErr) && ((bstrError = ::SysAllocString((OLECHAR *)_bXSLAsyncErr)) != NULL))
        {
            s = NULL;
        }
#endif
        else
        {
            s = Resources::FormatSystemMessage((long)hr);
        }

        if (s)
        {
            bstrError = s->getBSTR();
        }
    }    
    CATCH
    {
        bstrError = NULL;
    }
    ENDTRY

CleanUp:
    SafeRelease(pIErrorInfo);
    writeError(bstrError,NULL);
    ::SysFreeString(bstrError);
}


#ifdef UNIX
typedef const unsigned char * PUSHCAST;
#else
typedef const char * PUSHCAST; 
#endif

void
ViewerFactory::detectStylesheet(const WCHAR *pwcText, ULONG ulLen)
{
    IXMLParser *pParser = NULL;
    BSTR bURL;
    SSTYPE type;

    // allocate buffer for string for pushdata

    HRESULT hr = CoCreateInstance(CLSID_XMLParser, NULL, CLSCTX_INPROC_SERVER,
                                    IID_IXMLParser, (void**)&pParser);
    if (hr)
        return;     // simply display default factory
    
    DetectFactory *pDetect = new_ne DetectFactory();

    if (!pDetect)
        goto CleanUp;    // ditto if memory failure here

    CHECKHR(pParser->PushData((PUSHCAST)s_ByteOrderMark, sizeof(WCHAR), false)); 
    CHECKHR(pParser->PushData((PUSHCAST)(L"<STYLESHEET "), 12 * sizeof(WCHAR), false));
    CHECKHR(pParser->PushData((PUSHCAST)pwcText, ulLen * sizeof(WCHAR), false));
    CHECKHR(pParser->PushData((PUSHCAST)L"/>", 2 * sizeof(WCHAR), true));

    CHECKHR(pParser->SetFactory(pDetect));
    pDetect->Release();
    hr = pParser->Run(-1);   // if the directive is bad let it fail

    // get factory results
    type = pDetect->GetType();
    bURL = pDetect->GetURL();
    
    // if the URL is null for any reason, then we can't use XSL cause we need it
    if (type!=SSTYPE_XSL || bURL)
    {
        _ssType = type;
        _wURL = bURL;
    }

CleanUp:
    // now release the parser which will destroy the node factory
    SafeRelease(pParser);

}

HRESULT 
ViewerFactory::addExpandos(void)
{
    HRESULT hr;
    IDispatch *pIDoc = NULL;
    IDispatch *pIXSL = NULL;
    IHTMLDocument2 *pIHTML = NULL;
    BSTR bstrXML = NULL;
    BSTR bstrXSL = NULL;

    IUnknown* pTrident = (_pStm == null) ? null : _pStm->getTrident();
    if (!pTrident)
        goto CleanUp;
    
    // get the doc object to add expandos to
    hr = pTrident->QueryInterface(IID_IHTMLDocument2, (void **)&pIHTML);
    CHECKHR(hr); 
    
    // add access to the xml document
    bstrXML = ::SysAllocString(XMLTREE);
    if (!bstrXML)
    {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }
    hr = _pDOMDocument->QueryInterface(IID_IDispatch, (void **)&pIDoc);
    CHECKHR(hr);
    hr = AddDOCExpandoProperty(bstrXML, pIHTML, pIDoc);
    CHECKHR(hr);

    // add access to the xsl document
    bstrXSL = ::SysAllocString(XSLTREE);
    if (!bstrXSL)
    {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }
    hr = _pXSLDocument->QueryInterface(IID_IDispatch, (void **)&pIXSL);
    CHECKHR(hr);
    hr = AddDOCExpandoProperty(bstrXSL, pIHTML, pIXSL);

CleanUp:
    SafeRelease(pIDoc);
    SafeRelease(pIXSL);
    SafeRelease(pIHTML);
    SafeFreeString(bstrXML);
    SafeFreeString(bstrXSL);
    SafeRelease(pTrident);
    return hr;
}


HRESULT 
ViewerFactory::loadSS(IXMLDOMDocument *pXSLDocument)
{
    HRESULT hr;
    IMoniker *pIXSL = NULL;
    LPOLESTR pszName = NULL;
    VARIANT_BOOL bSuccess;
    VARIANT vURL;
    BSTR bURL = NULL;
            
    _fXSLError = true;
    hr = CreateURLMoniker(_pimkXML, _wURL, &pIXSL);
    CHECKHRPTR(hr, pIXSL);
    hr = pIXSL->GetDisplayName(NULL, NULL, &pszName);
    CHECKHRPTR(hr, pszName);
    hr = pXSLDocument->put_async(VARIANT_FALSE);
    CHECKHR(hr);

    bURL = ::SysAllocString(pszName);
    if (!bURL)
    {
        hr = E_FAIL;
        goto CleanUp;
    }
    hr = pXSLDocument->put_validateOnParse(VARIANT_FALSE);
    CHECKHR(hr);
    hr = pXSLDocument->put_resolveExternals(VARIANT_TRUE);
    CHECKHR(hr);
    vURL.vt = VT_BSTR;
    V_BSTR(&vURL) = bURL;
    hr = pXSLDocument->load(vURL, &bSuccess);
    CHECKHR(hr);
    if (bSuccess == VARIANT_FALSE)
    {
        hr = E_FAIL;
        goto CleanUp;
    }
    _fXSLError = false;
CleanUp:
    if (pszName)
        CoTaskMemFree(pszName);
    SafeRelease(pIXSL);
    SafeFreeString(bURL);
    return hr;
}


HRESULT
ViewerFactory::loadSSDefault(IXMLDOMDocument *pXSLDocument)
{
    HRESULT hr;
    VARIANT vbstrDef; vbstrDef.vt = VT_NULL;
    WCHAR *wStr = NULL;
    VARIANT_BOOL bSuccess;
    char *pResource;
    int lenW;
    DWORD dwSizeOfResource;

    if ((pResource = (char *)(Resources::GetUserResource("DEFAULTSS.XSL", (const char*)RT_HTML, &dwSizeOfResource))) == NULL)
    {
        hr = XML_E_INTERNALERROR;
        goto CleanUp;
    }

    lenW = MultiByteToWideChar(CP_ACP, 0, pResource, dwSizeOfResource, NULL, 0);
    wStr = new_ne WCHAR[lenW + 1];
    if (!wStr)
    {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }
    if (MultiByteToWideChar(CP_ACP, 0, pResource, dwSizeOfResource, wStr, lenW) == 0)
    {
        hr = E_FAIL;
        goto CleanUp;
    }

    // Null-terminate the string
    wStr[lenW] = 0;

    V_BSTR(&vbstrDef) = ::SysAllocString(wStr);
    if (V_BSTR(&vbstrDef) == NULL) {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }
    _fXSLError = true;
    hr = pXSLDocument->put_validateOnParse(VARIANT_FALSE);
    CHECKHR(hr);
    hr = pXSLDocument->put_resolveExternals(VARIANT_TRUE);
    CHECKHR(hr);
    vbstrDef.vt = VT_BSTR;
    hr = pXSLDocument->loadXML(V_BSTR(&vbstrDef), &bSuccess);
    CHECKHR(hr);
    if (bSuccess == VARIANT_FALSE)
        hr = E_FAIL;
    _fXSLError = false;

CleanUp:
    ::VariantClear(&vbstrDef);
    SafeDelete(wStr);
    return hr;
}


// we essentially want to set the security of the document to "itself"
// since that's the page that is loaded
// this is for the expando property we add for xsl to access the document

HRESULT
ViewerFactory::setDocSecurity(IXMLDOMDocument *pDocument)
{
    HRESULT hr = S_OK;
// MOVED to Viewer::reload.  It is too late to do this here.
#if 0 
    IXMLDOMDocument *iDocExpando = NULL;
    BSTR bURL = NULL;

    hr = pDocument->get_url(&bURL);
    CHECKHR(hr);
    CHECKALLOC(hr, bURL);
    iDocExpando = new_ne ExpandoDocument(bURL);
    CHECKALLOC(hr, iDocExpando);
    hr = setSecurity(iDocExpando, pDocument);
CleanUp:
    SysFreeString(bURL);
    SafeRelease(iDocExpando);
#endif
    return hr;
}


HRESULT
ViewerFactory::setSecurity(IXMLDOMDocument *pBaseDocument, IXMLDOMDocument *pDocument)
{
    HRESULT hr;
    DWORD dwXSetMask, dwXOptions;
    IObjectWithSite *pIOWS = NULL;
    IObjectSafety *pSafe = NULL;

    // set the site in the document, relative to the base document
    hr = pDocument->QueryInterface(IID_IObjectWithSite, (void **)&pIOWS);
    CHECKHR(hr);
    hr = pIOWS->SetSite(pBaseDocument);
    CHECKHR(hr);

    // set up the object safety options in the document
    hr = pDocument->QueryInterface(IID_IObjectSafety, (void **)&pSafe);
    CHECKHR(hr);
    dwXSetMask = dwXOptions = INTERFACESAFE_FOR_UNTRUSTED_DATA;
    hr = pSafe->SetInterfaceSafetyOptions(IID_IUnknown, dwXSetMask, dwXOptions);

CleanUp:
    SafeRelease(pSafe);
    SafeRelease(pIOWS);
    return hr;    
}

void
ViewerFactory::freeStream(void)
{
    if (_pStm)
        _pStm->SetAbortCB(NULL, NULL);
    SafeRelease(_pStm);
}


HRESULT 
ViewerFactory::setGenericParse(VARIANT_BOOL f)
{
    HRESULT hr = S_OK;
    IXMLGenericParse *pIXMGP = NULL;

    IUnknown* pTrident = (_pStm == null) ? null : _pStm->getTrident();
    if (!pTrident)
        goto CleanUp;

    hr = pTrident->QueryInterface(IID_IXMLGenericParse, (void **)&pIXMGP);
    if (SUCCEEDED(hr) && pIXMGP)
    {
        hr = pIXMGP->SetGenericParse(f);
        if (!hr)
            _fGenericParse = (f == VARIANT_TRUE);
    }

CleanUp:
    SafeRelease(pIXMGP);
    SafeRelease(pTrident);
    return hr;
}


#if XSLASYNC
HRESULT ViewerFactory::initXSLAsync(void)
{
    HRESULT hr;
    IXMLDOMNode *pXSL = NULL;
    IXMLDOMNode *pXML = NULL;

    Assert(_ssType == SSTYPE_XSL);
    
    hr = _pXSLDocument->QueryInterface(IID_IXMLDOMNode, (void **)&pXSL);
    CHECKHR(hr);
    hr = _pDOMDocument->QueryInterface(IID_IXMLDOMNode, (void **)&pXML);
    CHECKHR(hr);

    hr = CreateXTLProcessor(&_pIXTL);
    CHECKHR(hr);
    Assert(_pIXTL != NULL);

    hr = _pIXTL->Init(pXSL, pXML, _pStm);
    if (!SUCCEEDED(hr))
    {
        closeXSL();
        goto CleanUp;
    }

    // Write a unicode file so we don't lose any information.
    // Already done now in NotifyEvent after the call to Revert().
//    DWORD written;
//    _pStm->Write(s_ByteOrderMarkTrident, sizeof(WCHAR), &written);

CleanUp:
    SafeRelease(pXSL);
    SafeRelease(pXML);
    return hr;
}

HRESULT ViewerFactory::processXSLAsync(IXMLNodeSource *pSource, BOOL fLast)
{
    HRESULT hr = S_OK;
    BSTR bstrErr = NULL;

    if (_ssType == SSTYPE_XSL) {

        if (!_fInitAsync)
        {
            hr = initXSLAsync();
            if (!SUCCEEDED(hr))
                return hr;
            _fInitAsync = true;
        }
    
        if (_pIXTL)
        {
            _pStm->EnableRead();
            
            hr = _pIXTL->Execute(&bstrErr);
            if (hr == S_OK)
            {
                // we're done
                closeXSL();
            }
            else if (hr == E_PENDING && !fLast)
            {
                // keep processing, unless we expect to be done
                hr = S_OK;
            }
            else  // a hard error
            {
                // note that if we don't have a msg, we'll still return an error and bubble
                // it up through the parser.  The parser will set and report it's own error.
                // In this case we lose the xsl error but at least something is reported to
                // the user.
                if (bstrErr)
                {
                    _fXSLError = true;
                    pSource->Abort(bstrErr);
                }
            }
        }
    }
    if (fLast && hr!=S_OK)
    {
        SafeFreeString(_bXSLAsyncErr);
        if (bstrErr)
            _bXSLAsyncErr = ::SysAllocString(bstrErr);
    }
    SafeFreeString(bstrErr);
    return hr;
}

HRESULT ViewerFactory::signalAsync(IXMLNodeSource *pSource)
{
    HRESULT hr = S_OK;

    _nSignals++;
    if (_nSignals >= throttle)
    {
        _nSignals = 0;
        hr = processXSLAsync(pSource, FALSE);
    }
    return hr;
}

#endif

void ViewerFactory::closeXSL(void)
{
    if (_pIXTL)
    {
        // Must always call Close on the XSL processor before releasing the
        // processor to guarentee that the MIMEBufferedStream doesn't get sucked
        // into the GC object zero list - otherwise we get GC latency on
        // cleaning up the MIME GUI Windows -- which is a bad thing (causes
        // IE to crash eventually).
        _pIXTL->Close();
        SafeRelease(_pIXTL);
    }
}


#if MIMEASYNC    
BOOL ViewerFactory::StartAsync(IXMLNodeSource *pSource)
{
    // ok, now let's see if we can process the rest on the worker thread
    // first initialize the events, worker thread etc, get the parser interface, 
    // add it to the queue, then wake the event up
    // if we fail we revert to synchronous download
    BOOL bRet = FALSE;
    IXMLParser *pIXP = NULL;
    MDHANDLE mdh = NULL;
    MimeDwnParserAction *pMPA = NULL;
    if (SUCCEEDED(InitializeMimeDwn()))
    {
        EnterCriticalSection(&g_MimeDwnCSWndMgr);
        if ((mdh = g_pMimeDwnWndMgr->AddGUIWnd()) != NULL)
            if (SUCCEEDED(pSource->QueryInterface(IID_IXMLParser, (void **)&pIXP)))
                if (pIXP != NULL)
                    if ((pMPA = new_ne MimeDwnParserAction(mdh, pIXP, this, _evtLoad)) != NULL)
                        if (g_pMimeDwnQueue->Add(pMPA))
                            if (SetEvent(g_MimeDwnEvents[MDEVT_PROCESS]))
                            {
                                _pStm->TurnOnAsync(mdh);
                                _pbcsMonitor->TurnOnAsync();
                                _fIsAsync = true;
                                bRet = TRUE;
                            }
        LeaveCriticalSection(&g_MimeDwnCSWndMgr);
    }
    SafeRelease(pIXP);
    if (!_fIsAsync)
    {
        SafeDelete(pMPA);
    }
    if (mdh != NULL && g_pMimeDwnWndMgr)
    {
        // Now release the refcount that AddGUIWnd takes on the window since this
        // method is finished with the window now.
        g_pMimeDwnWndMgr->ReleaseGUIWnd(mdh);
    }
    return bRet;
}


// Release pointers to the documents so that we ourselves may be freed
// The Document may have a ref to the parser, which will only get
// released when the doc is finalized. The parser itself has a ref to the
// ViewerFactory.  The ViewerFactory has a ref back to the doc.
// So to break the chain we release our hold on the doc when it's safe to do so.
// CALL this only AFTER we know we are done processing the xsl and xml documents
// and no longer need to use them
void ViewerFactory::releaseDocs(void)
{
    Assert(_fIsAsync && _pIXTL == NULL);
    SafeRelease(_pXSLDocument);
    SafeRelease(_pDOMDocument);
}


#endif    
    
//============================================================================================
DetectFactory::DetectFactory()
{
    _bURL = NULL;
    _bAttribute = NULL;
    _ssType = SSTYPE_NONE;
}

DetectFactory::~DetectFactory()
{
    SafeFreeString(_bURL);
    SafeFreeString(_bAttribute);
}


BSTR DetectFactory::GetURL(void)
{
    if (_bURL)
        return ::SysAllocString(_bURL);
    else
        return NULL;
}

HRESULT STDMETHODCALLTYPE 
DetectFactory::CreateNode( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ PVOID pNodeParent,
    /* [in] */ USHORT cNumRecs,
    /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
{
    while (cNumRecs-- > 0)
    {
        XML_NODE_INFO* pNodeInfo = *apNodeInfo++;
        switch (pNodeInfo->dwType) {
        case XML_ATTRIBUTE:
            SafeFreeString(_bAttribute);
            _bAttribute = ::SysAllocStringLen(pNodeInfo->pwcText, pNodeInfo->ulLen);
            _ulAttrLen = pNodeInfo->ulLen;
            break;
        case XML_PCDATA:
            if (_bAttribute)
            {
                if (_ulAttrLen == 4 && StrCmpNI(_bAttribute, L"href", 4) == 0)
                {
                    SafeFreeString(_bURL);
                    _bURL = ::SysAllocStringLen(pNodeInfo->pwcText, pNodeInfo->ulLen);
                    // if the alloc fails then _bURL => detection failed => no stylesheet
                }
                else if (_ulAttrLen == 4 && StrCmpNI(_bAttribute, L"type", 4) == 0)
                {
                    if (pNodeInfo->ulLen == 8)
                    {
                        if (StrCmpNI(pNodeInfo->pwcText, L"text/css", 8) == 0)
                            _ssType = SSTYPE_CSS;
                        else if (StrCmpNI(pNodeInfo->pwcText, L"text/xsl", 8) == 0)
                            _ssType = SSTYPE_XSL;
                    }
//                  if (pNodeInfo->ulLen == 9)
//                  {
//                      if (StrCmpNI(pNodeInfo->pwcText, L"text/css2", 9) == 0)
//                          _ssType = SSTYPE_CSS;
//                  }
                }
            }
            break;
        }
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE 
DetectFactory::Error( 
    /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
    /* [in] */ HRESULT hrErrorCode,
    /* [in] */ USHORT cNumRecs,
    /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
{
    return hrErrorCode;
}