//+=============================================================================
//
//  File :  persist.cxx
//
//  contents : implementation of CPersistData xtag
//
//=============================================================================
#include "headers.h"

#ifndef X_MSHTML_H_
#define X_MSHTML_H_
#include "mshtml.h"       // for IHTML*ELement*
#endif

#ifndef X_PERHIST_H_
#define X_PERHIST_H_
#include "perhist.h"      // For IPersistHistory
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include "dispex.h"       // For IDispatchEx
#endif

#ifndef X_PERSIST_HXX_
#define X_PERSIST_HXX_
#include "persist.hxx"
#endif

#ifndef __X_IEXTAG_H_
#define __X_IEXTAG_H_
#include "iextag.h"
#endif

#ifndef __X_UTILS_HXX_
#define __X_UTILS_HXX_
#include "utils.hxx"
#endif


//+----------------------------------------------------------------------------
//
//  Member : Init
//
//  Synopsis : this method is called by MSHTML.dll to initialize peer object
//
//-----------------------------------------------------------------------------

HRESULT
CPersistDataPeer::Init(IElementBehaviorSite * pPeerSite)
{
    _pPeerSite = pPeerSite;
    _pPeerSite->AddRef();

    _pPeerSite->QueryInterface(IID_IElementBehaviorSiteOM, (void**)&_pPeerSiteOM);

    // now register our 2 events
    if (_pPeerSiteOM)
    {
        _pPeerSiteOM->RegisterEvent (_T("onload"), 0, NULL);
        _pPeerSiteOM->RegisterEvent (_T("onsave"), 0, NULL);
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member : Notify
//
//  Synopsis : when document is ready for modifications, setup timer calls
//
//-----------------------------------------------------------------------------

HRESULT
CPersistDataPeer::Notify(LONG lEvent, VARIANT *)
{
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  member : queryType
//
//  Synopsis : IPersistData method, this is used by the calling routines in order
//      to avoid cocreating the XML object.  This way they can query any of the
//      persist Tags, and find out what persist behaiors it supprots. 
//-----------------------------------------------------------------------------
HRESULT
CPersistDataPeer::queryType(long lType, VARIANT_BOOL * pfSupportsType)
{
    if (!pfSupportsType)
        return E_POINTER;

    *pfSupportsType = (lType == (long)_eState) ? VB_TRUE : VB_FALSE;

    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member : Save
//
//  Synopsis : Implementation of the IHTMLPersistData method
//
//-----------------------------------------------------------------------------

HRESULT
CPersistDataPeer::save(IUnknown * pUnk, 
                       long lType, 
                       VARIANT_BOOL *pfBroadcast)
{
    HRESULT hr;
    BSTR    bstrEvent;

    if (!pfBroadcast)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pfBroadcast = VB_TRUE;

    // cache the OM pointers
    InitOM(pUnk, lType);

    bstrEvent = SysAllocString(L"onsave");

    if (!bstrEvent)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // allow script to save data
    hr = (FireEvent(bstrEvent, pfBroadcast, TRUE));
    SysFreeString(bstrEvent);

Cleanup:
    return( hr );
}


//+----------------------------------------------------------------------------
//
//  Member : Load
//
//  Synopsis : Implementation of the IHTMLPersistData method
//
//-----------------------------------------------------------------------------

HRESULT
CPersistDataPeer::load(IUnknown * pUnk, 
                       long lType, 
                       VARIANT_BOOL *pfContinue)
{
    HRESULT hr;
    BSTR    bstrEvent;

    if (!pfContinue)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pfContinue = VB_TRUE;

    // cache the OM pointers
    InitOM(pUnk, lType, INIT_USE_CACHED);

    bstrEvent = SysAllocString(L"onload");
    if (!bstrEvent)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // allow event handlers the chance to load from the cach
    hr = (FireEvent(bstrEvent, pfContinue, FALSE));
    SysFreeString(bstrEvent);

Cleanup:
    return( hr );
}

//+--------------------------------------------------------------------------------
//
//  Member : FireEvent
//
//  Synopsis : helper method to fire the persistence events
//
//---------------------------------------------------------------------------------

HRESULT
CPersistDataPeer::FireEvent(BSTR bstrEvent, VARIANT_BOOL * pfContinue, BOOL fIsSaveEvent)
{
    IHTMLEventObj  * pEventObj = NULL;
    IHTMLEventObj2 * pEO2 = NULL;
    LONG             lCookie;
    HRESULT          hr = E_PENDING;
    VARIANT          varRet;

    if (!_pPeerSiteOM)
        goto Cleanup;

    VariantInit(&varRet);

    // create an event object
    hr = _pPeerSiteOM->CreateEventObject(&pEventObj);
    if (hr || !pEventObj)
        goto Cleanup;

    // Now populate the event object with whatever properties are
    // appropriate for this event:
    hr = pEventObj->QueryInterface(IID_IHTMLEventObj2, (void**)&pEO2);
    if (hr==S_OK)
    {
        BSTR bstrEventType = SysAllocString( (fIsSaveEvent) ? L"save" : L"load" );

        // we need to set the event type, the event type is either
        // "load" or "save".
        if (bstrEventType)
        {
            pEO2->put_type( bstrEventType );
            SysFreeString(bstrEventType);
        }
    }

    // get the event cookie to fire the event
    hr = _pPeerSiteOM->GetEventCookie (bstrEvent, &lCookie);
    if (hr)
        goto Cleanup;

    hr = _pPeerSiteOM->FireEvent (lCookie, pEventObj);

    if (pfContinue)
    {
        hr = pEventObj->get_returnValue(&varRet);
        if (!hr)
            *pfContinue =  ((V_VT(&varRet) == VT_BOOL) && 
            (V_BOOL(&varRet) == VB_FALSE))? VB_FALSE : VB_TRUE;
    }

Cleanup:
    VariantClear(&varRet);
    ReleaseInterface(pEventObj);
    ReleaseInterface(pEO2);
    return ( hr );
}


//+-----------------------------------------------------------------------
//
//  Member : ClearOMInterfaces ()
//
//  Synopsis : this helper function is called after the Save/Load persistenceCache
//      operations are finished, and is responsible for freeing up any of hte OM 
//      interfaces that were cached.
//
//------------------------------------------------------------------------
void
CPersistDataPeer::ClearOMInterfaces()
{
    ClearInterface(&_pRoot);
    ClearInterface(&_pInnerXMLDoc);
}

//---------------------------------------------------------------------------
//
//  Member:     CPersistDataPeer::InitOM
//
//  Synopsis:   IHTMLPersistData OM method implementation
//
//---------------------------------------------------------------------------

HRESULT
CPersistDataPeer::InitOM(IUnknown * pUnk, long lType, DWORD dwFlags /* ==0*/)
{
    HRESULT            hr = S_OK;
    IHTMLElement     * pPeerElement = NULL;
    long               lElemID = -1;
    BSTR               bstrTag =NULL;
    CBufferedStr       cbsID;
    IXMLDOMNodeList  * pChildren = NULL;
    IXMLDOMElement   * pNewChild = NULL;
    IXMLDOMNode      * pSubTree = NULL;

    if (!pUnk)
        return E_INVALIDARG;

    if (_pInnerXMLDoc)
    {
        if (dwFlags & INIT_USE_CACHED)
            goto Cleanup;
        else
            ClearOMInterfaces();
    }

    // make sure we know what we have been given.
    hr = (pUnk->QueryInterface(IID_IXMLDOMDocument, (void**)&_pInnerXMLDoc));
    if (hr)
        goto Cleanup;

    // find the data value...
    hr = _pInnerXMLDoc->get_documentElement( &_pRoot );
    if (hr)
        goto Cleanup;

    // but in most cases the xmlObject is a big bucket, and we would like
    // to set the _pRoot to the subtree associated with our own _pPeerElement.
    // if this root doesn't exist, create it.  we try to find an ID first, if 
    // if it is not there, then create a unique name for this child subtree 
    // but appending the srcID to the tagName.

    _pPeerSite->GetElement(&pPeerElement);
    if (!pPeerElement)
        goto Cleanup;

    hr = pPeerElement->get_id(&bstrTag);
    if (hr || !bstrTag || !SysStringLen(bstrTag))
    {
        SysFreeString(bstrTag);

        hr = pPeerElement->get_sourceIndex(&lElemID);
        if (hr)
            goto Cleanup;

        hr = pPeerElement->get_tagName(&bstrTag);
        if (hr)
            goto Cleanup;

        hr = cbsID.QuickAppend(bstrTag);
        SysFreeString(bstrTag);
        if (hr)
            goto Cleanup;

        hr = cbsID.QuickAppend(lElemID);
        if (hr)
            goto Cleanup;

        bstrTag = SysAllocString(cbsID);
    }

    if (!bstrTag)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // now we can actually see if there is a child of this name
    //   We need to do this by looping over the childNode collection
    //   and looking for one with our name.
    //--------------------------------------------------------------
    hr = _pRoot->get_childNodes(&pChildren);
    if (FAILED(hr))
        goto Cleanup;

    if (hr == S_OK)
    {
        long i, iLen;
        
        hr = pChildren->get_length(&iLen);
        if (FAILED(hr))
            goto Cleanup;

        for (i=0; i < iLen; i++)
        {
            IXMLDOMNode * pTemp = NULL;
            BSTR       bstrNodeName = NULL;
            BOOL       fEqual = FALSE;

            hr= pChildren->get_item(i, &pTemp);
            if (FAILED(hr))
                break;
            
            hr = pTemp->get_nodeName( &bstrNodeName );
            if (FAILED(hr))
                break;

            fEqual = ! _wcsicmp(bstrTag, bstrNodeName);

            SysFreeString(bstrNodeName);

            if (fEqual)
            {
                // transfer ownership
                pSubTree = pTemp;
                break;
            }

            ClearInterface( &pTemp );
        }
    }

    if(pSubTree)
    {
        // yes there's a child so use it, we have a domnode so we need to
        // qi for the element
        ClearInterface(&_pRoot);
        hr = pSubTree->QueryInterface(IID_IXMLDOMElement, (void**)&_pRoot);
    }
    else
    {
        // no child yet so lets create one.
        hr = _pInnerXMLDoc->createElement(bstrTag, &pNewChild);
        if (hr || ! pNewChild)
            goto Cleanup;

        hr = _pRoot->appendChild(pNewChild, NULL);
        if (hr)
            goto Cleanup;

        ClearInterface(&_pRoot);
        _pRoot=pNewChild;
        pNewChild = NULL;   // transfer ownership
    }


Cleanup:
    ReleaseInterface(pPeerElement);
    SysFreeString(bstrTag);
    ReleaseInterface(pChildren);
    ReleaseInterface(pSubTree);
    ReleaseInterface(pNewChild);
    return( hr );
}


//---------------------------------------------------------------------------
//
//  Member:     CPersistDataPeer::getAttribute
//
//  Synopsis:   IHTMLPersistData OM method implementation
//
//---------------------------------------------------------------------------

HRESULT
CPersistDataPeer::getAttribute (BSTR strName, VARIANT * pvarValue )
{
    HRESULT hr = S_OK;

    if (!pvarValue)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    VariantClear(pvarValue);

    if (!strName)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = InitOM();
    if (hr) 
    {
        // no OM pointers so fail silently
        hr = S_OK;
        goto Cleanup;
    }

    // get the child of the root that has the name strName
    if (_pRoot)
    {
        hr = _pRoot->getAttribute(strName, pvarValue);
        if (hr ==S_FALSE)
            hr = S_OK;
    }

Cleanup:
    return hr ;
}


//---------------------------------------------------------------------------
//
//  Member:     CPersistDataPeer::setAttribute
//
//  Synopsis:   IHTMLPersistData OM method implementation
//
//---------------------------------------------------------------------------

HRESULT
CPersistDataPeer::setAttribute (BSTR strName, VARIANT varValue)
{
    HRESULT  hr = S_OK;

    if (!strName)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = InitOM();
    if (hr) 
    {
        // no OM pointers so fail silently
        hr = S_OK;
        goto Cleanup;
    }

    // save this value as an attribute on the root.
    if (_pRoot)
    {
        //BUGBUG - look into just passing the variant into the xml object
        // and let them worry about all the types... why should we have to 
        // even do this processing at all?   .. to be safe, that's why.
        VARIANT * pvar;
        CVariant cvarTemp;

        if ((V_VT(&varValue)==VT_BSTR) || 
             V_VT(&varValue)==(VT_BYREF|VT_BSTR))
        {
            pvar = (V_VT(&varValue) & VT_BYREF) ?
                    V_VARIANTREF(&varValue) : &varValue;
        }
        else if ((V_VT(&varValue)==VT_BOOL ||
                 V_VT(&varValue)==(VT_BYREF|VT_BOOL)))
        {
            // sadly, do our own bool conversion...
            VARIANT_BOOL vbFlag = (V_VT(&varValue)==VT_BOOL) ?
                                   V_BOOL(&varValue) :
                                   V_BOOL( V_VARIANTREF(&varValue) );

            V_VT(&cvarTemp) = VT_BSTR;
            V_BSTR(&cvarTemp) = vbFlag ? SysAllocString(L"true") :
                                         SysAllocString(L"false");

            pvar = & cvarTemp;
        }
        else
        {
            pvar = &varValue;

            hr = VariantChangeTypeEx(pvar, pvar, LCID_SCRIPTING, 0, VT_BSTR);
            if (hr)
                goto Cleanup;
        }


        hr = _pRoot->setAttribute(strName, *pvar);
        if (hr ==S_FALSE)
            hr = S_OK;
    }

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Member:     CPersistDataPeer::removeDataValue
//
//  Synopsis:   IHTMLPersistData OM method implementation
//
//---------------------------------------------------------------------------

HRESULT
CPersistDataPeer::removeAttribute (BSTR strName)
{
    HRESULT hr = S_OK;

    if (!strName)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = InitOM();
    if (hr) 
    {
        // no OM pointers so fail silently
        hr = S_OK;
        goto Cleanup;
    }

    // get the child of the root that has the name strName
    if (_pRoot)
    {
        hr = _pRoot->removeAttribute(strName);
        if (hr ==S_FALSE)
            hr = S_OK;
    }

Cleanup:
    return hr ;
}

//---------------------------------------------------------------------------
//
//  Member:     CPersistDataPeer::get_XMLDocument
//
//  Synopsis:   IHTMLPersistData OM proeprty implementation. this is the default 
//                  property for this object, and as such it exposes the XMLOM
//                  of the user data.
//
//---------------------------------------------------------------------------

HRESULT
CPersistDataPeer::get_XMLDocument (IDispatch ** ppDisp)
{
    HRESULT hr = S_OK;

    if (!ppDisp)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppDisp = NULL;

    hr = InitOM();
    if (hr) 
    {
        // no OM pointers so fail silently
        hr = S_OK;
        goto Cleanup;
    }

    if (_pInnerXMLDoc)
    {
        hr = _pInnerXMLDoc->QueryInterface(IID_IDispatch, 
                                           (void**)ppDisp);
    }

Cleanup:
    return hr;
}


//+-----------------------------------------------------------
//
//  Member : GetSaveCategory
//
//  Synopsis : this helper function turns the tagName fo the 
//      pPeerElement, into the category for its save operations.
//
//+-----------------------------------------------------------
ENUM_SAVE_CATEGORY
CPersistDataPeer::GetSaveCategory()
{
    HRESULT            hr = S_OK;
    CVariant           cvarTag;
    IHTMLElement     * pPeerElement = NULL;
    ENUM_SAVE_CATEGORY escRet = ESC_UNKNOWN;

    _pPeerSite->GetElement(&pPeerElement);
    if (!pPeerElement)
        goto Cleanup;

    V_VT(&cvarTag) = VT_BSTR;
    hr = pPeerElement->get_tagName(&V_BSTR(&cvarTag));
    if (hr)
        goto Cleanup;

    // there's got to be a better way to do this, but for
    // now I will try to put the most common ones in front.
    if (0==_wcsicmp(V_BSTR(&cvarTag), L"input"))
    {
        // but wait!!!! don't save password type inputs
        BSTR     bstrType = SysAllocString(L"type");
        CVariant cvarVal;

        escRet = ESC_INTRINSIC;
        if (!bstrType)
            goto Cleanup;

        hr = (pPeerElement->getAttribute(bstrType, 0, &cvarVal));
        SysFreeString(bstrType);
        if (hr)
            goto Cleanup;

        if (V_VT(&cvarVal) == VT_BSTR &&
            0==_wcsicmp(V_BSTR(&cvarVal), L"password"))
        {
            escRet = ESC_PASSWORD;
            goto Cleanup;
        }
    }
    else if (0==_wcsicmp(V_BSTR(&cvarTag), L"script"))
    {
        escRet = ESC_SCRIPT;
    }
    else if (0==_wcsicmp(V_BSTR(&cvarTag), L"select")   ||
             0==_wcsicmp(V_BSTR(&cvarTag), L"textarea") ||
             0==_wcsicmp(V_BSTR(&cvarTag), L"richtext") ||
             0==_wcsicmp(V_BSTR(&cvarTag), L"button")   ||
             0==_wcsicmp(V_BSTR(&cvarTag), L"fieldset") 
            )
    {
        escRet = ESC_INTRINSIC;
    }
    else if (0==_wcsicmp(V_BSTR(&cvarTag), L"object") || 
             0==_wcsicmp(V_BSTR(&cvarTag), L"embed")  ||
             0==_wcsicmp(V_BSTR(&cvarTag), L"applet")
            )
    {
        escRet = ESC_CONTROL;
    }


Cleanup:
    ReleaseInterface(pPeerElement);
    return escRet;
}

//+-----------------------------------------------------------
//
//  member : GetEngineClsidForLanguage ()
//
//  synopsis : another helper method that returns the CLSID of the
//   scripdEngine associated with the language progID.
//
//+-----------------------------------------------------------
HRESULT
CPersistDataPeer::GetEngineClsidForLanguage(CLSID * pclsid, 
                                            IHTMLDocument2 * pBrowseDoc)
{
    HRESULT hr = S_OK;
    BSTR    bstrLanguage = NULL;
    IHTMLElement * pPeerElement = NULL;


    if (!pclsid)
        return E_POINTER;

    *pclsid = IID_NULL;

    _pPeerSite->GetElement(&pPeerElement);
    if (!pPeerElement)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // what language script engine to instantiate?
    hr = (pPeerElement->get_language(&bstrLanguage));
    if (hr)
        goto Cleanup;

    if (!bstrLanguage || !SysStringLen(bstrLanguage))
    {
        IHTMLElementCollection * pCollection = NULL;

        hr = E_FAIL;

        if (bstrLanguage)
        {
            SysFreeString(bstrLanguage);
            bstrLanguage = NULL;
        }

        // use the default language, we get this from the language of 
        // first script block in the document.
        if (!FAILED(pBrowseDoc->get_scripts(&pCollection)))
        {
            CVariant    cvarID;
            CVariant    cvarEmpty;
            IDispatch * pDisp;

            V_VT(&cvarID) = VT_I4;
            V_I4(&cvarID) = 0;

            if (!FAILED(pCollection->item(cvarID, cvarEmpty, &pDisp)))
            {
                IHTMLElement     * pFirstScript = NULL;

                if (pDisp &&
                    !FAILED(pDisp->QueryInterface(IID_IHTMLElement, 
                                                      (void**)&pFirstScript)))
                {
                    pFirstScript->get_language(&bstrLanguage);
                    if (bstrLanguage && SysStringLen(bstrLanguage))
                        hr = CLSIDFromProgID ( bstrLanguage, pclsid );

                    ReleaseInterface(pFirstScript);
                }

                ReleaseInterface(pDisp);
            }

            ReleaseInterface(pCollection);
        }

        if (hr)
        {
            hr = CLSIDFromProgID ( L"JScript", pclsid );
        }
    }
    else
    {
        hr = CLSIDFromProgID ( bstrLanguage, pclsid );
    }

Cleanup:
    SysFreeString(bstrLanguage);
    ReleaseInterface(pPeerElement);
    return ( hr );
}
//+-----------------------------------------------------------
//
//  Member  GetScriptEngine
//
//  Synopsis : helper method - this creates the appropriate script 
//      engine, and parses in the text of hte script block that we
//      are interested in.
//
//+-----------------------------------------------------------

IActiveScript * 
CPersistDataPeer::GetScriptEngine(IHTMLDocument2 * pBrowseDoc, ULONG * puFlags)
{
    HRESULT              hr = S_OK;
    BSTR                 bstrCode = NULL;
    CLSID                clsID;
    CLSID                clsIDTarget;
    IActiveScript      * pScriptEngine = NULL;
    IActiveScriptSite  * pScriptSite = NULL;
    IActiveScriptParse * pASP = NULL;
    IHTMLScriptElement * pScriptElem = NULL;
    IHTMLElement       * pPeerElement = NULL;


    hr = (GetEngineClsidForLanguage(&clsID, pBrowseDoc));
    if (hr)
        goto Cleanup;

    // set the return flags, so that these tests only need to be done once
    // some callers need to know if the script block is jscript, vbscript,
    // of something else
    if (!FAILED(CLSIDFromProgID ( _T("JScript"), &clsIDTarget )) &&
        (clsID == clsIDTarget))
    {
        *puFlags = SCRIPT_ENGINE_JSCRIPT;
    }
    else if (!FAILED(CLSIDFromProgID ( _T("VBScript"), &clsIDTarget )) &&
        (clsID == clsIDTarget))
    {
        *puFlags = SCRIPT_ENGINE_VBSCRIPT;
    }
    else
        *puFlags = SCRIPT_ENGINE_OTHER;


    // create the script engine 
    hr = CoCreateInstance( clsID, NULL, CLSCTX_INPROC_SERVER,
            IID_IActiveScript,(void **)&pScriptEngine);
    if ( hr )
        goto Cleanup;

    // get the scriptSite interface from ourselves.
    hr = (QueryInterface(IID_IActiveScriptSite, (void**) &pScriptSite));
    if (hr)
        goto Cleanup;

    hr = pScriptEngine->SetScriptSite(pScriptSite);
    ReleaseInterface(pScriptSite);  // engine should have addref'd 
    if ( FAILED(hr) )
        goto Error;

    // prepare to load out text 
    hr = (pScriptEngine->QueryInterface (IID_IActiveScriptParse, (void**)&pASP));
    if ( hr )
        goto Error;

    hr = (pASP->InitNew());
    if ( hr )
        goto Error;

    _pPeerSite->GetElement(&pPeerElement);
    if (!pPeerElement)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // get the text of the script element
    hr = (pPeerElement->QueryInterface(IID_IHTMLScriptElement, (void**)&pScriptElem));
    if ( hr )
        goto Error;

    hr = (pScriptElem->get_text(&bstrCode));
    if ( hr || !bstrCode)
        goto Error;

    hr = pASP->ParseScriptText( bstrCode,  
                                NULL,                 // Item Name (Namespace)
                                NULL,                 // context
                                NULL,                 // delimiter
                                0,                    // srcContext Cookie
                                0,                    // ulStarting Line
                                SCRIPTTEXT_ISVISIBLE, //dwFlags
                                NULL,                 // pVarResult
                                NULL );               // pException Info
    if ( hr )
        goto Error;

    hr = pScriptEngine->SetScriptState(SCRIPTSTATE_CONNECTED);
    if ( FAILED(hr) )
        goto Error;

Cleanup:
    SysFreeString(bstrCode);
    ReleaseInterface(pPeerElement);
    ReleaseInterface(pScriptElem);
    ReleaseInterface(pASP);
    return pScriptEngine;

Error:
    ClearInterface(&pScriptEngine);
    goto Cleanup;
}


//+-----------------------------------------------------------
//
//  Member  BuildNewScriptBlock
//
//  Synopsis : helper method, this does the real work of snapshotting a 
//      script block.  IF the script is VBSCRIPT or JSCRIPT the appropriate 
//      syntax is used.  for any other language, we convert to JScript.
//
//+-----------------------------------------------------------
HRESULT
CPersistDataPeer::BuildNewScriptBlock(CBufferedStr * pstrBuffer, ULONG *puFlags)
{
    HRESULT          hr    = S_OK;
    IDispatch      * pScriptNameSpace = NULL;
    IDispatchEx    * pDispScript    = NULL;
    IDispatchEx    * pBrowseWin     = NULL;
    DISPID           dispidName;
    BSTR             bstrName       = NULL;
    IHTMLWindow2   * pWin           = NULL;
    IActiveScript  * pScriptEngine  = NULL;
    IDispatch      * pdispBrowseDoc = NULL;
    IHTMLDocument2 * pBrowseDoc     = NULL;
    IHTMLElement   * pPeerElement   = NULL;


    _pPeerSite->GetElement(&pPeerElement);
    if (!pPeerElement)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = (pPeerElement->get_document(&pdispBrowseDoc));
    if (hr)
        goto Cleanup;

    hr = (pdispBrowseDoc->QueryInterface(IID_IHTMLDocument2, 
                                            (void**)&pBrowseDoc));
    if (hr)
        goto Cleanup;

    pScriptEngine = GetScriptEngine(pBrowseDoc, puFlags);
    if (!pScriptEngine)
        goto Cleanup;   // fail silently


    // loop over all the dispatches in the script engine's name 
    //   space and get thier values.from..pSrcDoc
    hr = pScriptEngine->GetScriptDispatch( NULL, &pScriptNameSpace );
    if ( hr )
        goto Cleanup;

    hr = (pScriptNameSpace->QueryInterface( IID_IDispatchEx, (void**)&pDispScript));
    if ( hr )
        goto Cleanup;

    // inorder to derefernce the script variables we need the browse
    // documents window.
    hr = (pBrowseDoc->get_parentWindow(&pWin));
    if(hr)
        goto Cleanup;

    hr = (pWin->QueryInterface(IID_IDispatchEx, (void**)&pBrowseWin));
    if (hr)
        goto Cleanup;

    // now run through all the objects in this script engine and 
    // find their values 
    hr = (pDispScript->GetNextDispID(0, DISPID_STARTENUM, &dispidName));
    if (hr)
        goto Cleanup;

    while (dispidName)
    {
        hr = (pDispScript->GetMemberName(dispidName, &bstrName));
        if (!hr && bstrName)
        {
            DISPID  dispidBrowser;

            if (!FAILED(pBrowseWin->GetDispID(bstrName, fdexNameCaseSensitive, &dispidBrowser)))
            {
                DISPPARAMS dp;
                CVariant cvarRes;
                BOOL fNeedsQuotesInScript = FALSE;

                dp.rgvarg = NULL;
                dp.rgdispidNamedArgs = NULL;
                dp.cArgs = 0;
                dp.cNamedArgs = 0;

                hr = (pBrowseWin->Invoke(dispidBrowser,
                                             IID_NULL,  
                                             LCID_SCRIPTING,   
                                             DISPATCH_PROPERTYGET,
                                             &dp,                 
                                             &cvarRes,            
                                             NULL,                
                                             NULL));


                fNeedsQuotesInScript = (V_VT(&cvarRes) == VT_BSTR);

                // Filter out VT_IDISPATCH & VT_UNKNOWN
                if (!hr && 
                    V_VT(&cvarRes) != VT_DISPATCH &&
                    V_VT(&cvarRes) != VT_UNKNOWN   &&
                    !FAILED(cvarRes.CoerceVariantArg(VT_BSTR)))
                {
                    // assume JSCript if not VBScript
                    LPTSTR  pstrAssign = (*puFlags & SCRIPT_ENGINE_VBSCRIPT) ? 
                                                _T("\n\r    dim ") : 
                                                _T("\n\r    var ");


                    // concatenate on the end of the buffer.
                    //  "\n\r    var <name> = " <value>";  "
                    hr = pstrBuffer->QuickAppend(pstrAssign);
                    if (hr)
                        goto Cleanup;
                    hr = pstrBuffer->QuickAppend(bstrName);
                    if (hr)
                        goto Cleanup;

                    // if vbscript, put assignment on its own line.
                    if (*puFlags & SCRIPT_ENGINE_VBSCRIPT)
                    {
                        hr = pstrBuffer->QuickAppend(_T("\n\r        "));
                        if (hr)
                            goto Cleanup;
                        hr = pstrBuffer->QuickAppend(bstrName);
                        if (hr)
                            goto Cleanup;
                    }

                    // now handle outputing the assignment itself, careful
                    // to only put quotes around things that were strings.
                    hr = pstrBuffer->QuickAppend(_T(" = "));
                    if (hr)
                        goto Cleanup;

                    if (fNeedsQuotesInScript )
                    {
                        hr = pstrBuffer->QuickAppend(_T("\""));
                        if (hr)
                            goto Cleanup;
                    }

                    if (V_VT(&cvarRes) != VT_EMPTY)
                    {
                        hr = pstrBuffer->QuickAppend(V_BSTR(&cvarRes));
                    }
                    else
                    {
                        // someone had dim x = Empty in their script
                        // or var x = document.expando
                        // and this doesn't coerc to a bstr (returned S_FALSE)
                        // don't reset the use quotes flag,  
                        // for jscript convert to null, for vb
                        if (!(*puFlags & SCRIPT_ENGINE_VBSCRIPT))
                        {
                            hr = pstrBuffer->QuickAppend(_T("null"));
                        }
                        else
                            hr = pstrBuffer->QuickAppend(_T("Empty"));
                    }
                    if (hr)
                        goto Cleanup;

                    if (fNeedsQuotesInScript )
                    {
                        hr = pstrBuffer->QuickAppend(_T("\""));
                        if (hr)
                            goto Cleanup;
                    }

                    // VBSCRIPT doesn need ';' at the eol
                    if (!(*puFlags & SCRIPT_ENGINE_VBSCRIPT))
                    {
                        hr = pstrBuffer->QuickAppend(_T(";"));
                    }

                    if (hr)
                        goto Cleanup;
                }
            }

            SysFreeString(bstrName);
            bstrName = NULL;
        }
            
        hr = (pDispScript->GetNextDispID(0, dispidName, &dispidName));
        if (hr==S_FALSE)
        {  
            hr = S_OK;
            break;
        }
        else if (hr)
            goto Cleanup;
    }

 
Cleanup:
    if (pScriptEngine)
        pScriptEngine->Close();
    if (bstrName)
        SysFreeString(bstrName);
    ReleaseInterface(pPeerElement);
    ReleaseInterface(pBrowseDoc);
    ReleaseInterface(pScriptEngine);
    ReleaseInterface(pdispBrowseDoc);
    ReleaseInterface(pWin);
    ReleaseInterface(pBrowseWin);
    ReleaseInterface(pDispScript);
    ReleaseInterface(pScriptNameSpace);
    // no partial string in an error case... 
    if (hr)
        pstrBuffer->Set();

    return( hr );
}




//+-----------------------------------------------------------
//
// member : SaveHandler_GenericTag
//
// synopsis : this does the generic handling when a tag has the
//      persistence peer. It simply gets the outerHTML and saves that
//      in the XML object.  on the load the element will be hit
//      with a put_outerHTML of this string.
//
//+-----------------------------------------------------------
HRESULT
CPersistDataPeer::SaveHandler_GenericTag()
{
    HRESULT        hr = S_OK;
    CVariant       cvarOuter;
    IHTMLElement * pPeerParent = NULL;
    IHTMLElement * pPeerElement = NULL;

    _pPeerSite->GetElement(&pPeerElement);
    if (!pPeerElement)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // for elements in the head, we don't want to do this since
    // there is a tree limitation that doesn't allow the outerHTML
    // to be set.
    hr = pPeerElement->get_parentElement(&pPeerParent);
    if (hr)
        goto Cleanup;

    // the right way to do this is to ask the question:
    // are we in the body/frameset, NOT is our parent the
    // head, so...
    while (pPeerParent)
    {
        CVariant       cvarTag;
        IHTMLElement * pTemp = NULL;

        V_VT(&cvarTag) = VT_BSTR;
        hr = pPeerParent->get_tagName(&V_BSTR(&cvarTag));
        if (hr)
            goto Cleanup;

        if (0==_wcsicmp(V_BSTR(&cvarTag), L"body") ||
            0==_wcsicmp(V_BSTR(&cvarTag), L"frameset"))
            break;

        hr = pPeerParent->get_parentElement(&pTemp);
        if (hr)
            goto Cleanup;

        ClearInterface(&pPeerParent);
        pPeerParent = pTemp;
    }

    if (!pPeerParent)
        goto Cleanup;

    hr = pPeerElement->get_outerHTML(&V_BSTR(&cvarOuter));
    if (hr)
        goto Cleanup;

    V_VT(&cvarOuter) = VT_BSTR;

    // now save the outerHTML string
    hr = _pRoot->setAttribute(_T("__NEW_TAG_OUTER"), cvarOuter);
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pPeerElement);
    ReleaseInterface(pPeerParent);
    return( hr );
}


//+----------------------------------------------------------------------------
//
//  Member : LoadHandler_GenericTag()
//
//  Synopsis : this restores the outerHTML of the tag as it is loaded
//
//+----------------------------------------------------------------------------
HRESULT
CPersistDataPeer::LoadHandler_GenericTag()
{
    HRESULT          hr = S_OK;
    CVariant         cvarOuter;
    IHTMLElement   * pPeerElement   = NULL;

   if (!_pRoot)
        goto Cleanup;

    _pPeerSite->GetElement(&pPeerElement);
    if (!pPeerElement)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // now get the outerHTML string
    hr = _pRoot->getAttribute(_T("__NEW_TAG_OUTER"), &cvarOuter);
    if (hr)
        goto Cleanup;

    hr = cvarOuter.CoerceVariantArg(VT_BSTR);
    if (hr)
        goto Cleanup;

    hr = pPeerElement->put_outerHTML(V_BSTR(&cvarOuter));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pPeerElement);
    return( hr );
}


//+------------------------------------------------------------------------------
//
//  Member : SaveHandler_ScriptTag()
//
//  Synopsis : saves the script into the xml store.  We do this in the standard way:
//      1- create a private script engin,
//      2- load it with the text of the peer's script block
//      3 - walk through its namespace pulling out the variables
//      4 - create a list of name value pairs as the new script block
//      5 - save this block in the appropriate place
//
//+------------------------------------------------------------------------------

HRESULT
CPersistDataPeer::SaveHandler_ScriptTag()
{
    HRESULT          hr = S_OK;
    CVariant         cvarText;
    BSTR             bstrLanguage = NULL;
    CLSID            clsIDTarget;
    CLSID            clsIDLang;
    CBufferedStr     cbsNewScript;
    IDispatch      * pdispBrowseDoc = NULL;
    IHTMLDocument2 * pBrowseDoc     = NULL;
    IHTMLElement   * pPeerElement   = NULL;
    ULONG            uFlags = 0;


    if (!_pRoot)
        goto Cleanup;

    _pPeerSite->GetElement(&pPeerElement);
    if (!pPeerElement)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pPeerElement->get_document(&pdispBrowseDoc);
    if (hr)
        goto Cleanup;

    hr = pdispBrowseDoc->QueryInterface(IID_IHTMLDocument2, 
                                            (void**)&pBrowseDoc);
    if (hr)
        goto Cleanup;

    // get the new script block, and the language that it
    // originated from
    hr = BuildNewScriptBlock(&cbsNewScript, &uFlags);
    if (hr)
        goto Cleanup;

    // Language can only be set in design mode,  as a result
    // we can't change the script block since we don't know the
    //  syntax for an arbitrary script engine.  so, only save
    //  script if this is javascript or vbscript.
    if (uFlags & SCRIPT_ENGINE_OTHER)
        goto Cleanup;

    V_VT(&cvarText) = VT_BSTR;
    V_BSTR(&cvarText) = SysAllocString((LPTSTR)cbsNewScript);
    if (!V_BSTR(&cvarText))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    hr = _pRoot->setAttribute(_T("__NEW_SCRIPT_TEXT"), cvarText);
    if (hr)
        goto Cleanup;

Cleanup:
    SysFreeString(bstrLanguage);
    ReleaseInterface(pPeerElement);
    ReleaseInterface(pBrowseDoc);
    ReleaseInterface(pdispBrowseDoc);
    return( hr );
}


//+----------------------------------------------------------------------------
//
//  Member : LoadHandler_ScriptTag()
//
//  Synopsis : does the load - but wait... 
//      NYI - put_text is design time only, so the peer needs a different way
//      of sliding this in.
//
//+----------------------------------------------------------------------------

HRESULT
CPersistDataPeer::LoadHandler_ScriptTag()
{
    HRESULT   hr = S_OK;
    CVariant  cvarText;

    if (!_pRoot)
        goto Cleanup;

    // try to get the new text from the saved XML
    hr = _pRoot->getAttribute(_T("__NEW_SCRIPT_TEXT"), &cvarText);
    if (hr)
        goto Cleanup;

    // if there is script text, then we need to set this into the 
    // browse window's namespace


Cleanup:
    if (hr == S_FALSE) hr = S_OK;
    return( hr );
}


//===============================================================
//
//  Class : CPersistShortcut
//
//  synopsis : derives from CPersistDataPeer and does the special handling 
//       related to the shortcut functionality
//
//===============================================================



//+----------------------------------------------------------------
//
// Member : save
//
//  synopsis: over ride of parent functionality to deal with shortcut
//      specific stuff...
//
//-----------------------------------------------------------------
HRESULT 
CPersistShortcut::save(IUnknown * pUnk, 
                       long lType, 
                       VARIANT_BOOL *pfContinue)
{
    HRESULT hr;

    if (lType != (long)htmlPersistStateFavorite)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = super::save(pUnk, lType, pfContinue);

    if (hr || (*pfContinue == VB_FALSE))
        goto Cleanup;

    // do the particular tag handling related to shortcuts
    switch (GetSaveCategory())
    {
    case ESC_UNKNOWN:
    case ESC_INTRINSIC:
    case ESC_CONTROL:
        hr = SaveHandler_GenericTag();
        break;

//    case ESC_SCRIPT:
//        hr = SaveHandler_ScriptTag();
//        break;

    default:
        break;
    }

Cleanup:
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member : Load
//
//  Synopsis : Implementation of the IHTMLPersistenceCache method
//
//-----------------------------------------------------------------------------

HRESULT
CPersistShortcut::load(IUnknown * pUnk, 
                       long lType, 
                       VARIANT_BOOL *pfDoDefault)
{
    HRESULT          hr;

    if (lType != (long)htmlPersistStateFavorite)
    {
        hr = S_OK;
        goto Cleanup;
    }

    // fire the event first, since the pPeerElement is about to be
    //  removed from the tree entirely.  Of course, if the user sets 
    // expandoes on this, they will get lost, but the alternative is to
    // loose access to the xmlcache.  So, if you think about it, they get
    // outerHTML behaviour, and using XMLCache behavior means
    // that you better cancel default behavior.
    hr = super::load(pUnk, lType, pfDoDefault);
    // is default load behavior canceled ?
    if (hr || (*pfDoDefault == VB_FALSE))
        goto Cleanup;

    switch (GetSaveCategory())
    {
    case ESC_UNKNOWN:
    case ESC_INTRINSIC:
    case ESC_CONTROL:
        hr = LoadHandler_GenericTag();
        break;

//    case ESC_SCRIPT:
//        hr = LoadHandler_ScriptTag();
//        break;

    default:
        break;
    }


Cleanup:
    return hr;
}


//===============================================================
//
//  Class : CPersistHistory
//
//  synopsis : derives from CPersistDataPeer and does the special handling 
//       related to the History functionality
//
//===============================================================


//+----------------------------------------------------------------
//
// Member : save
//
//  synopsis: over ride of parent functionality to deal with shortcut
//      specific stuff...
//
//-----------------------------------------------------------------
HRESULT 
CPersistHistory::save(IUnknown * pUnk, 
                       long lType, 
                       VARIANT_BOOL *pfContinue)
{
    HRESULT hr;

    if (lType != (long)htmlPersistStateHistory)
    {
        hr = S_OK;
        goto Cleanup;
    }

     hr = super::save(pUnk, lType, pfContinue);
    if (hr || (*pfContinue == VB_FALSE))
        goto Cleanup;

    // do any tag specific handling that history needs
    switch (GetSaveCategory())
    {
        // if we don't know anything special to do with this tag, just
        // transfer over its outerHTML on the assumption that the author
        // knows what they are doing
    case ESC_UNKNOWN:
    case ESC_INTRINSIC:
        hr = SaveHandler_GenericTag();
        break;

//    case ESC_SCRIPT:
//        hr = SaveHandler_ScriptTag();
//        break;

    default :
        break;
    }

Cleanup:
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member : Load
//
//  Synopsis : Implementation of the IHTMLPersistenceCache method
//
//-----------------------------------------------------------------------------

HRESULT
CPersistHistory::load(IUnknown * pUnk, 
                       long lType, 
                       VARIANT_BOOL *pfDoDefault)
{
    HRESULT          hr;

    if (lType != (long)htmlPersistStateHistory)
    {
        hr = S_OK;
        goto Cleanup;
    }

    // cache the OM pointers
    hr = InitOM(pUnk, lType);
    if (hr)
        goto Cleanup;

    hr = super::load(pUnk, lType, pfDoDefault);
    // is default load behavior canceled ?
    if (hr || (*pfDoDefault == VB_FALSE))
        goto Cleanup;

//BUGBUG (carled) NYI - script retoration presents a unique challenge 
//    because this needs to execute inline instead of the original 
//    script block text.
    switch(GetSaveCategory())
    {
//    case ESC_SCRIPT:
//        hr = LoadHandler_ScriptTag();
//        break;

    case ESC_UNKNOWN:
    case ESC_INTRINSIC:
        hr = LoadHandler_GenericTag();
        break;

    default:
        break;
    }

Cleanup:
    return hr;
}


//===============================================================
//
//  Class : CPersistSnapshot
//
//  synopsis : derives from CPersistDataPeer and does the special handling 
//       related to the run-time same (snapshot) functionality
//
//===============================================================


//+----------------------------------------------------------------
//
// Member : CPersistSnapshot::save
//
//  synopsis: over ride of parent functionality to deal with shortcut
//      specific stuff...
//      Since it doesn't make much sense to allow theXMLOM to be acccessed
//      in this case, calling super:: pretty much only fires the event.
//
//-----------------------------------------------------------------
HRESULT 
CPersistSnapshot::save(IUnknown * pUnk, 
                       long lType, 
                       VARIANT_BOOL *pfContinue)
{
    HRESULT          hr;
    CVariant         cvarTag;
    IHTMLDocument2 * pDesignDoc = NULL;

    if (lType != (long)htmlPersistStateSnapshot)
    {
        hr = S_OK;
        goto Cleanup;
    }

    if (!pUnk)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = pUnk->QueryInterface(IID_IHTMLDocument2, (void**)&pDesignDoc);
    if (hr)
        goto Cleanup;

    // allow the onsave event to be fired.
    hr = super::save(NULL, lType, pfContinue);
    if (hr || (*pfContinue == VB_FALSE))
        goto Cleanup;

    // now that the author has had the oppurtunity to set expandoes,
    // and value properties, we start the real work of snapshot saveing.
    switch (GetSaveCategory())
    {
    case ESC_CONTROL:
        hr = TransferControlValues(pDesignDoc);
        break;

    case ESC_SCRIPT:
        hr = TransferScriptValues(pDesignDoc);
        break;

    case ESC_UNKNOWN:
        // if we don't know anything special to do with this tag, just
        // transfer over its outerHTML on the assumption that the author
        // knows what they are doing
    case ESC_INTRINSIC:
        hr = TransferIntrinsicValues(pDesignDoc);
        break;

    default:
        break;
    }

Cleanup:
    ReleaseInterface(pDesignDoc);
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member : CPersistSnapshot::Load
//
//  Synopsis : Implementation of the IHTMLPersistenceCache method
//      Since it doesn't make much sense to allow theXMLOM to be acccessed
//      in this case, calling super:: pretty much only fires the event.
//
//-----------------------------------------------------------------------------

HRESULT
CPersistSnapshot::load(IUnknown * pUnk, 
                       long lType, 
                       VARIANT_BOOL *pfDoDefault)
{
    HRESULT          hr;

    if (lType != (long)htmlPersistStateSnapshot)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = super::load(pUnk, lType, pfDoDefault);

Cleanup:
    return hr;
}


//+-----------------------------------------------------------
//
// member : GetDesignElem
//
// Synposis : this helper function is responsible for finding the
//      design document's element counterpart to our pPeerElement.
//
//+-----------------------------------------------------------
IHTMLElement *
CPersistSnapshot::GetDesignElem(IHTMLDocument2 * pDesignDoc)
{
    HRESULT                  hr = S_OK;
    CVariant                 cvarID;
    CVariant                 cvarEmpty;
    IDispatch              * pDisp = NULL;
    IHTMLElement           * pRetElem = NULL;
    IHTMLElement           * pPeerElement = NULL;
    IHTMLElementCollection * pCollection = NULL;

    _pPeerSite->GetElement(&pPeerElement);
    if (!pPeerElement)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    V_VT(&cvarID) = VT_BSTR;
    hr = pPeerElement->get_id(&V_BSTR(&cvarID));
    if (hr)
        goto Cleanup;

    // first get the documents all collection.
    hr = pDesignDoc->get_all(&pCollection);
    if (hr)
        goto Cleanup;

    // now find out elements coutner part inthe design document, 
    // if it exits.  Elements that were created by inline scripts,
    // or other script driven means, may not be found.
    hr = pCollection->item(cvarID, cvarEmpty, &pDisp);
    if (hr || !pDisp)
        goto Cleanup;

    // make sure that we do not have a collection...
    hr = pDisp->QueryInterface(IID_IHTMLElement, (void**)&pRetElem);

Cleanup:
    ReleaseInterface(pPeerElement);
    ReleaseInterface(pDisp);
    ReleaseInterface(pCollection);
    return pRetElem;
}

//+-----------------------------------------------------------
//
// member : TransferIntrinsicValues 
//
// synopsis : this transfers the value of the browse-control to the 
//      design control by setting the outerHTML of the design control
//      to that of the browseControl
//
//+-----------------------------------------------------------
HRESULT
CPersistSnapshot::TransferIntrinsicValues (IHTMLDocument2 *pDesignDoc)
{
    HRESULT        hr = S_OK;
    BSTR           bstrOuter = NULL;
    IHTMLElement * pDesignElem = NULL;
    IHTMLElement * pPeerElement = NULL;

    _pPeerSite->GetElement(&pPeerElement);
    if (!pPeerElement)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pPeerElement->get_outerHTML(&bstrOuter);
    if (hr)
        goto Cleanup;

    pDesignElem = GetDesignElem(pDesignDoc);
    if (!pDesignElem)
        goto Cleanup;

    hr = pDesignElem->put_outerHTML(bstrOuter);

Cleanup:
    SysFreeString(bstrOuter);
    ReleaseInterface(pPeerElement);
    ReleaseInterface(pDesignElem);
    return( hr );
}


//+-----------------------------------------------------------
//
//  Member : TransferControlValues
//
//  Synopsis : Transfers the values of the control by cooking up a 
//      stream and calling IPersitHistory::Save on the browse-control
//      and then feeding that stream into IPersistHistory::Load on
//      the design-control,  Then when the design document is saved,
//      IPersistPropertyBag::save will get called (it is this that finally
//      allows the controls state to be written out as the altHTML attribute.
//
//+-----------------------------------------------------------
HRESULT
CPersistSnapshot::TransferControlValues (IHTMLDocument2 *pDesignDoc)
{
    HRESULT           hr = S_OK;
    IHTMLElement    * pDesignElem = NULL;
    IHTMLElement    * pPeerElement = NULL;
    IPersistHistory * pSrcPH = NULL;
    IPersistHistory * pDesignPH = NULL;
    IStream         * pStream = NULL;
    HGLOBAL           hg = NULL;
    static LARGE_INTEGER i64Zero = {0, 0};

    _pPeerSite->GetElement(&pPeerElement);
    if (!pPeerElement)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pDesignElem = GetDesignElem(pDesignDoc);
    if (!pDesignElem)
        goto Cleanup;

    // step 1. get the appropriate IPersistHistory interfaces.
    hr = pPeerElement->QueryInterface(IID_IPersistHistory, (void**)&pSrcPH);
    if (hr)
        goto Cleanup;

    hr = (pDesignElem->QueryInterface(IID_IPersistHistory, (void**)&pDesignPH));
    if (hr)
        goto Cleanup;

    // step 2. create a temporary stream
    hg = GlobalAlloc(GMEM_MOVEABLE, 0);
    if (!hg)
    {  
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = (CreateStreamOnHGlobal(hg, TRUE, &pStream));
    if (hr)
        goto Cleanup;

    // Step 3. save the browse object
    hr = (pSrcPH->SaveHistory(pStream));
    if (hr)
        goto Cleanup;

    // Step 4. reset the stream
    hr = (pStream->Seek(i64Zero, STREAM_SEEK_SET, NULL));
    if (hr)
        goto Cleanup;

    // Stpe 5. Load the design object.
    hr = (pDesignPH->LoadHistory(pStream, NULL));
    if (hr)
        goto Cleanup;

Cleanup:
    // BUGBUG (carled) is there a memory leak here? the hGlobal was allocated,
    //   a stream was created, and freed but where does the memory get freed?
    ReleaseInterface(pPeerElement);
    ReleaseInterface(pStream);
    ReleaseInterface(pSrcPH);
    ReleaseInterface(pDesignPH);
    ReleaseInterface(pDesignElem);
    return( (hr == E_NOINTERFACE)? S_OK : hr );
}


//+-----------------------------------------------------------
//
//  Member : TransferScriptValues
//
//  Synopsis: transfers the values of script blocks, by loading this
//      script into a private script engine, and then iterating over all
//      the values and getting the current values from the browse document.
//      These (Variable, Value) pairs are then spit out as the new innerHTML
//      of the design-script element.
//
//+-----------------------------------------------------------

HRESULT
CPersistSnapshot::TransferScriptValues (IHTMLDocument2 *pDesignDoc)
{ 
    HRESULT          hr = S_OK;
    IHTMLElement   * pDesignElem  = NULL;
    BSTR             bstrNewBlock = NULL;
    CBufferedStr     cbsNewScript;
    ULONG            uFlags       = 0;
    IHTMLScriptElement * pScriptElem = NULL;

    pDesignElem = GetDesignElem(pDesignDoc);
    if (!pDesignElem)
        goto Cleanup;

    // get the text of the new script block
    hr = (BuildNewScriptBlock(&cbsNewScript, &uFlags));
    if (hr)
        goto Cleanup;

    bstrNewBlock = SysAllocString((LPTSTR)cbsNewScript);
    if (!bstrNewBlock)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Set the new script assignments 
    hr = (pDesignElem->QueryInterface(IID_IHTMLScriptElement, (void**)&pScriptElem));
    if ( hr )
        goto Cleanup;

    hr = (pScriptElem->put_text(bstrNewBlock));
    if ( hr )
        goto Cleanup;

    // since all the namespaces exist on the "window"
    // we need to make sure that the script block we
    // write out is in a language whose syntax we know. As
    // a result a script block from any ActiveScript language
    // will get persisted, but unless it is VBScript or JScript, 
    // in the saveing, it is translated into JScript.
    if (uFlags & SCRIPT_ENGINE_OTHER)
    {
        BSTR  bstrForceLanguage = SysAllocString(L"JScript");
        if (!bstrForceLanguage)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = (pDesignElem->put_language(bstrForceLanguage));
        SysFreeString(bstrForceLanguage);
        if (hr) 
            goto Cleanup;
    }
    
Cleanup:
    SysFreeString(bstrNewBlock);
    ReleaseInterface(pScriptElem);
    ReleaseInterface(pDesignElem);
    return( hr );
}


