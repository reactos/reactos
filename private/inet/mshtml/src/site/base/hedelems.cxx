//+---------------------------------------------------------------------
//
//   File:      hedelems.cxx
//
//  Contents:   Elements that are normally found in the HEAD of an HTML doc
//
//  Classes:    CMetaElement, CLinkElement, CIsIndexElement, CBaseElement, CNextIdElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_HTMLDLG_HXX_
#define X_HTMLDLG_HXX_
#include "htmldlg.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_BUFFER_HXX_
#define X_BUFFER_HXX_
#include "buffer.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#define _cxx_
#include "hedelems.hdl"

MtDefine(CTitleElement,   Elements, "CTitleElement")
MtDefine(CMetaElement,    Elements, "CMetaElement")
MtDefine(CBaseElement,    Elements, "CBaseElement")
MtDefine(CIsIndexElement, Elements, "CIsIndexElement")
MtDefine(CNextIdElement,  Elements, "CNextIdElement")
MtDefine(CHeadElement,    Elements, "CHeadElement")
MtDefine(CHtmlElement,    Elements, "CHtmlElement")

//+------------------------------------------------------------------------
//
//  Class:      CHtmlElement
//
//  Synopsis:   
//
//-------------------------------------------------------------------------

const CElement::CLASSDESC CHtmlElement::s_classdesc =
{
    {
        &CLSID_HTMLHtmlElement,            // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLElement,                  // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLElement,
};


//+----------------------------------------------------------------
//
//  Member:   CHtmlElement::CreateElement
//
//---------------------------------------------------------------

HRESULT
CHtmlElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);
    *ppElementResult = new CHtmlElement(pDoc);
    return (*ppElementResult ? S_OK : E_OUTOFMEMORY);
}

//+----------------------------------------------------------------
//
//  Member:   ApplyDefaultFormat
//
//  Synopsis: this is only interesting for the HTML element. in That
//            case, we apply in the attrarray from the document. this
//            allows for default values (like from the HTMLDlg code)
//            to be incorparated at this early point.
//
//---------------------------------------------------------------

HRESULT
CHtmlElement::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT         hr = S_OK;
    CDoc *          pDoc = Doc();

    Assert(pCFI && pCFI->_pNodeContext && SameScope(pCFI->_pNodeContext, this));

    // if we have history that saved document direction we begin the chain here
    // if direction has been explicitly set, this will be overridden.
    pCFI->PrepareCharFormat();
    pCFI->_cf()._fRTL = pDoc->_fRTLDocDirection;
    pCFI->UnprepareForDebug();
    
    hr = THR(super::ApplyDefaultFormat(pCFI));
    if(hr)
        goto Cleanup;
    
    if (pDoc->_fInHTMLDlg)
    {
        // we are in a HTML Dialog, so get its AttrArray to use
        //  in computing our values.
        IUnknown *      pUnkHTMLDlg = NULL;

        hr = THR_NOTRACE(pDoc->QueryService(IID_IHTMLDialog, IID_IUnknown, 
                                (void**)&pUnkHTMLDlg));
        if (hr)
            goto Cleanup;

        if (pUnkHTMLDlg)
        {
            CHTMLDlg *  pHTMLDlg;

            // weak QI
            hr = THR(pUnkHTMLDlg->QueryInterface(CLSID_HTMLDialog, (void**)&pHTMLDlg));
            Assert (!hr && pHTMLDlg);

            hr = THR(ApplyAttrArrayValues( pCFI, pHTMLDlg->GetAttrArray()));
        }

        ReleaseInterface (pUnkHTMLDlg);
    }
    
    // set up for potential EMs, ENs, and ES Conversions
    pCFI->PrepareParaFormat();
    pCFI->_pf()._lFontHeightTwips = pCFI->_pcf->GetHeightInTwips(pDoc);
    pCFI->UnprepareForDebug();
  
Cleanup:
    return (hr);
}

//+----------------------------------------------------------
//
//  Member :  OnPropertyChange
//
//  Synopsis  for the HTML element if we get one of the 
//            "interesting" properties changed in its style sheet
//            then if we are in an HTMLDlg that dialog needs to be 
//            notified in order to update its size. the Doc's OPC
//            will do just this.  More accuratly,the doc's OPC calls
//            CServers which does a PropertyNotification to the 
//            sinks, and a HTMLDlg registers a sink to catch this            
//
//-----------------------------------------------------------

HRESULT
CHtmlElement::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    hr = THR(super::OnPropertyChange( dispid, dwFlags ));
    if (hr)
        goto Cleanup;

    switch(dispid)
    { 
        case DISPID_A_FONT :
        case DISPID_A_FONTSIZE :
        case DISPID_A_FONTWEIGHT :
        case DISPID_A_FONTFACE :
        case DISPID_A_FONTSTYLE :
        case DISPID_A_FONTVARIANT :
        case STDPROPID_XOBJ_TOP : 
        case STDPROPID_XOBJ_LEFT :
        case STDPROPID_XOBJ_WIDTH :
        case STDPROPID_XOBJ_HEIGHT :
        case DISPID_A_DIR :
        case DISPID_A_DIRECTION:
            hr = THR(Doc()->OnPropertyChange(dispid, dwFlags));
            if (hr)
                goto Cleanup;
            break;

        default:
            break;
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Class:      CHeadElement
//
//  Synopsis:   
//
//-------------------------------------------------------------------------

const CElement::CLASSDESC CHeadElement::s_classdesc =
{
    {
        &CLSID_HTMLHeadElement,            // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLElement,                  // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLElement,
};


HRESULT
CHeadElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);
    *ppElementResult = new CHeadElement(pDoc);
    return (*ppElementResult ? S_OK : E_OUTOFMEMORY);
}

//+------------------------------------------------------------------------
//
//  Class:      CTitleElement
//
//  Synopsis:   Push new parser specifically for TITLE text
//
//-------------------------------------------------------------------------

const CElement::CLASSDESC CTitleElement::s_classdesc =
{
    {
        &CLSID_HTMLTitleElement,            // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLTitleElement,             // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLTitleElement,      // _pfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};


HRESULT
CTitleElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);
    *ppElementResult = new CTitleElement(pDoc);
    return (*ppElementResult ? S_OK : E_OUTOFMEMORY);
}

#define ISSPACE(ch) (((ch) == _T(' ')) || ((unsigned)((ch) - 9)) <= 13 - 9)
#define ISNSPAC(ch) (ch && !((ch) == _T(' ')) || ((unsigned)((ch) - 9)) <= 13 - 9)

#if DBG == 1
CTitleElement::~ CTitleElement ( )
{
}
#endif

HRESULT
CTitleElement::SetTitle(TCHAR *pchTitle)
{
    // BUGBUG (dbau) Netscape doesn't allow document.title to be set!

    // In a <TITLE> tag, Netscape eliminates multiple spaces, etc, as follows:

    HRESULT hr;
    TCHAR *pchTemp = NULL;
    TCHAR *pch;
    TCHAR *pchTo;
    CDoc *  pDoc = Doc();
    
    if (!pchTitle)
        goto NULLSTR;

    hr = THR(MemAllocString(Mt(CTitleElement), pchTitle, &pchTemp));
    if (hr)
        goto Cleanup;

    pch = pchTo = pchTemp;
    
    // remove leading spaces
    goto LOOPSTART;

    do
    {
        *pchTo++ = _T(' ');
        
LOOPSTART:
        while (ISNSPAC(*pch))
            *pchTo++ = *pch++;

        // remove multiple/trailing spaces
        while (ISSPACE(*pch))
            pch++;
            
    } while (*pch);
    
    *pchTo = _T('\0');

NULLSTR:
    hr = THR(_cstrTitle.Set(pchTemp));
    if (hr)
        goto Cleanup;
    IGNORE_HR(pDoc->OnPropertyChange(DISPID_CDoc_title, SERVERCHNG_NOVIEWCHANGE));

    pDoc->DeferUpdateTitle();

Cleanup:
    MemFreeString(pchTemp);
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHeadElement::Save
//
//  Synopsis:   Standard element saver.  Includes hook for saving out
//              additional tags for printing such as BASE and STYLE tag
//
//  Arguments:  pStreamWriteBuff    The stream to write into
//              fEnd                If this is the end tag
//
//-------------------------------------------------------------------------

HRESULT
CHeadElement::Save(CStreamWriteBuff * pStreamWriteBuff, BOOL fEnd)
{
    HRESULT hr = THR(super::Save(pStreamWriteBuff, fEnd));
    if (hr || fEnd)
        goto Cleanup;

    if (pStreamWriteBuff->TestFlag(WBF_SAVE_FOR_PRINTDOC))
    {
        CDoc * pDoc = Doc();
        Assert(pDoc);
        hr = THR(pDoc->SaveHtmlHead(pStreamWriteBuff));
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CTitleElement::Save
//
//  Synopsis:   standard element saver
//
//  Arguments:  pStreamWriteBuff    The stream to write into
//              fEnd                If this is the end tag
//
//-------------------------------------------------------------------------

HRESULT
CTitleElement::Save(CStreamWriteBuff * pStreamWriteBuff, BOOL fEnd)
{
    HRESULT hr = S_OK;
    DWORD   dwOld;

    // if no title string and synthetic, don't save tags
    if ( _fSynthesized && !_cstrTitle.Length())
        goto Cleanup;

    // Save tagname and attributes.
    hr = THR(super::Save(pStreamWriteBuff, fEnd));
    if (hr)
        goto Cleanup;

    // Tell the write buffer to just write this string
    // literally, without checking for any entity references.
    
    dwOld = pStreamWriteBuff->ClearFlags(WBF_ENTITYREF);
    
    // Tell the stream to now not perform any fancy indenting
    // or such stuff.
    pStreamWriteBuff->BeginPre();

    if (!fEnd)
    {
        hr = THR(pStreamWriteBuff->Write((LPTSTR)_cstrTitle));
        if (hr)
            goto Cleanup;
    }
        
    pStreamWriteBuff->EndPre();
    pStreamWriteBuff->SetFlags(dwOld);
    
Cleanup:
    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Class: CMetaElement
//
//-------------------------------------------------------------------------

const CElement::CLASSDESC CMetaElement::s_classdesc =
{
    {
        &CLSID_HTMLMetaElement,             // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLMetaElement,              // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLMetaElement,       // _apfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};

HRESULT
CMetaElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);
    *ppElementResult = new CMetaElement(pDoc);
    return (*ppElementResult ? S_OK : E_OUTOFMEMORY);
}

HRESULT
CMetaElement::Init2(CInit2Context * pContext)
{
    HRESULT hr;

    hr = THR(super::Init2(pContext));
    if (hr)
        goto Cleanup;
        
#if 0 // commented out to fix bug 54067: allow meta-http-equiv to work inside the BODY
    if (SearchBranchToRootForTag(ETAG_HEAD))
#endif
    {
        CDoc *  pDoc = Doc();
        LPCTSTR pchHttpEquiv;
        LPCTSTR pchName;
        LPCTSTR pchContent;

        if (    pDoc
            &&  _pAA )
        {
            if(    _pAA->FindString(DISPID_CMetaElement_httpEquiv, &pchHttpEquiv)
               &&  pchHttpEquiv
               &&  pchHttpEquiv[0]
               &&  _pAA->FindString(DISPID_CMetaElement_content, &pchContent)
               &&  pchContent)
            {
                pDoc->ProcessHttpEquiv(pchHttpEquiv, pchContent);
            }
            else if (    _pAA->FindString(DISPID_CMetaElement_name, &pchName)
                     &&  pchName
                     &&  pchName[0]
                     &&  _pAA->FindString(DISPID_CMetaElement_content, &pchContent)
                     &&  pchContent)
            {
                pDoc->ProcessMetaName(pchName, pchContent);
            }

        }
    }

Cleanup:
    RRETURN(hr);
}

static BOOL LocateCodepageMeta ( CMetaElement * pMeta )
{
    return pMeta->IsCodePageMeta();
}

HRESULT
CMetaElement::Save(CStreamWriteBuff * pStmWrBuff, BOOL fEnd)
{
    CMetaElement * pMeta;

    if (    IsCodePageMeta() 
        &&  (   Doc()->PrimaryMarkup()->LocateHeadMeta(LocateCodepageMeta, &pMeta) == S_OK 
            &&  pMeta != this )
        ||  pStmWrBuff->TestFlag(WBF_NO_CHARSET_META) )
    {
        // Only write out the first charset meta tag in the head
        return S_OK;
    }

    return super::Save(pStmWrBuff, fEnd);
}

BOOL
CMetaElement::IsCodePageMeta( )
{
    return ( GetAAhttpEquiv() && !StrCmpIC(GetAAhttpEquiv(), _T("content-type")) &&
             GetAAcontent()) || 
             GetAAcharset();
}

CODEPAGE
CMetaElement::GetCodePageFromMeta( )
{
    if( GetAAhttpEquiv() && StrCmpIC( GetAAhttpEquiv( ), _T("content-type") ) == 0 &&
        GetAAcontent() )
    {
        // Check if we are of the form:
        //  <META HTTP-EQUIV="Content-Type" CONTENT="text/html;charset=xxx">
        return CodePageFromString( (LPTSTR) GetAAcontent(), TRUE );
    }
    else if ( GetAAcharset() )
    {
        // Check if we are either:
        //  <META HTTP-EQUIV CHARSET=xxx> or <META CHARSET=xxx>
        return CodePageFromString( (LPTSTR) GetAAcharset(), FALSE );
    }

    // Either this meta tag does not specify a codepage, or the codepage specified
    //  is unrecognized.
     return CP_UNDEFINED;
}
       
BOOL
CMetaElement::IsPersistMeta(long eState)
{
    BOOL fRes = FALSE;

    if (GetAAname() &&  GetAAcontent() &&
            (StrCmpNIC(GetAAname(), 
                        PERSISTENCE_META, 
                        ARRAY_SIZE(PERSISTENCE_META))==0))
    {
        int         cchNameLen = 0;
        LPCTSTR     pstrNameStart = NULL;
        CDataListEnumerator   dleContent(GetAAcontent(), _T(','));


        // for each name token in the content string check to see if
        //   it specifies the persistence type we are intereseted in
        while (dleContent.GetNext(&pstrNameStart, &cchNameLen))
        {
            switch (eState)
            {
            case 1:     // favorite
                fRes = (0 == _tcsnicmp(_T("favorite"), 8, (LPTSTR)pstrNameStart, cchNameLen));
                break;
            case 2:     // History
                fRes = (0 == _tcsnicmp(_T("history"), 7, (LPTSTR)pstrNameStart, cchNameLen));
                break;
            case 3:     // Snapshot
                fRes = (0 == _tcsnicmp(_T("snapshot"), 8, (LPTSTR)pstrNameStart, cchNameLen));
                break;
            default:
                fRes = FALSE;
                break;
            }

            // if we still don't have it, give the "all" type a chance
            if (!fRes)
                fRes = (0 == _tcsnicmp(_T("all"), 3, (LPTSTR)pstrNameStart, cchNameLen));
        }

    }

    return fRes;
}

HRESULT
CMetaElement::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    if( dispid == DISPID_CMetaElement_content &&
        IsCodePageMeta() )
    {
        CODEPAGE cp = GetCodePageFromMeta();

        if( cp != CP_UNDEFINED )
        {
            // BUGBUG (johnv) What should we do when we get this message?
        }
    }

    hr = THR(super::OnPropertyChange( dispid, dwFlags ));
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Class: CBaseElement
//
//-------------------------------------------------------------------------

const CElement::CLASSDESC CBaseElement::s_classdesc =
{
    {
        &CLSID_HTMLBaseElement,             // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLBaseElement,              // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLBaseElement,       // _apfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};

HRESULT
CBaseElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);
    *ppElementResult = new CBaseElement(pht->GetTag(), pDoc);
    return (*ppElementResult ? S_OK : E_OUTOFMEMORY);
}

HRESULT SetUrlDefaultScheme(const TCHAR *pchHref, CStr *pStr)
{
    HRESULT hr;
    TCHAR achQualifiedHref[pdlUrlLen];
    DWORD cchQualifiedHref = ARRAY_SIZE(achQualifiedHref);

    hr = UrlApplyScheme(pchHref, achQualifiedHref, &cchQualifiedHref,
        URL_APPLY_GUESSSCHEME | URL_APPLY_GUESSFILE | URL_APPLY_DEFAULT);
    if (hr)
        hr = THR(pStr->Set(pchHref));
    else
        hr = THR(pStr->Set(achQualifiedHref));

    RRETURN(hr);
}

HRESULT
CBaseElement::Init2(CInit2Context * pContext)
{
    HRESULT hr;

    hr = THR(super::Init2(pContext));
    if (hr)
        goto Cleanup;

    hr = SetUrlDefaultScheme(GetAAhref(), &_cstrBase);

Cleanup:
    RRETURN(hr);
}

void
CBaseElement::Notify( CNotification * pNF )
{
    super::Notify(pNF);

    switch (pNF->Type())
    {
    case NTYPE_ELEMENT_ENTERTREE:
        // we might be in the head...tell doc to look
        Doc()->_fHasBaseTag = TRUE;

        // only send the notification when the element is entering 
        // because of non-parsing related calls
        if ( !(pNF->DataAsDWORD() & ENTERTREE_PARSE) )
            BroadcastBaseUrlChange();
        break;

    case NTYPE_ELEMENT_EXITTREE_1:
        if ( !(pNF->DataAsDWORD() & EXITTREE_DESTROY) )
            BroadcastBaseUrlChange();
        break;
    }
}

HRESULT
CBaseElement::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr;

    if (dispid == DISPID_CBaseElement_href)
    {
        hr = SetUrlDefaultScheme(GetAAhref(), &_cstrBase);
        if (hr)
            goto Cleanup;

        // send notification to the descendants, if we
        // are in a markup
        if ( IsInMarkup() )
            BroadcastBaseUrlChange();
    }

    hr = super::OnPropertyChange(dispid, dwFlags);

Cleanup:
    RRETURN(hr);
}

void 
CBaseElement::BroadcastBaseUrlChange( )
{
    CNotification   nf;
    CDoc *          pDoc = Doc();

    // send the notification to change non-cached properties.
    SendNotification( NTYPE_BASE_URL_CHANGE );
        
    // Force a re-render
    THR(pDoc->ForceRelayout() );

    // and force recomputing behavior on the markup that contains this 
    // base element.
    nf.RecomputeBehavior( MarkupRoot() );
    
    pDoc->BroadcastNotify(&nf);
}


//+------------------------------------------------------------------------
//
//  Class: CIsIndexElement
//
//-------------------------------------------------------------------------

const CElement::CLASSDESC CIsIndexElement::s_classdesc =
{
    {
        &CLSID_HTMLIsIndexElement,          // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLIsIndexElement,           // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLIsIndexElement,    // _apfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};

HRESULT
CIsIndexElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);
    *ppElementResult = new CIsIndexElement(pDoc);
    return (*ppElementResult ? S_OK : E_OUTOFMEMORY);
}



//+------------------------------------------------------------------------
//
//  Class: CNextIdElement
//
//-------------------------------------------------------------------------

const CElement::CLASSDESC CNextIdElement::s_classdesc =
{
    {
        &CLSID_HTMLNextIdElement,           // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLNextIdElement,            // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLNextIdElement,     // _apfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};

HRESULT
CNextIdElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);
    *ppElementResult = new CNextIdElement(pDoc);
    return (*ppElementResult ? S_OK : E_OUTOFMEMORY);
}

