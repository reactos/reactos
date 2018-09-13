//+---------------------------------------------------------------------
//
//   File:      eform.cxx
//
//  Contents:   Form element class, etc..
//
//  Classes:    CFormElement, etc..
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_CGUID_H_
#define X_CGUID_H_
#include <cguid.h>
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_EFORM_HXX_
#define X_EFORM_HXX_
#include "eform.hxx"
#endif

#ifndef X_BTNHLPER_HXX_
#define X_BTNHLPER_HXX_
#include "btnhlper.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_TEXTAREA_HXX_
#define X_TEXTAREA_HXX_
#include "textarea.hxx"
#endif

#ifndef X_INPUTBTN_HXX_
#define X_INPUTBTN_HXX_
#include "inputbtn.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_POSTDATA_HXX_
#define X_POSTDATA_HXX_
#include "postdata.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_INPUTLYT_HXX_
#define X_INPUTLYT_HXX_
#include "inputlyt.hxx"
#endif

#ifndef X_TAREALYT_HXX_
#define X_TAREALYT_HXX_
#include "tarealyt.hxx"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif


#if defined(_M_ALPHA)
#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include "tearoff.hxx"
#endif
#endif

#define _cxx_
#include "eform.hdl"

// Needs this for the IHTMLElementCollection interface
#define _hxx_
#include "collect.hdl"

DeclareTag(tagFormElement, "Form Element", "Form Element methods")

MtDefine(CFormElement, Elements, "CFormElement")
MtDefine(BldFormElementCol, PerfPigs, "Build CFormElement::FORM_ELEMENT_COLLECTION")
MtDefine(BldFormNamedImgCol, PerfPigs, "Build CFormElement::FORM_NAMED_IMG_COLLECTION")
MtDefine(BldFormSubmitCol, PerfPigs, "Build CFormElement::FORM_SUBMIT_COLLECTION")

#ifndef NO_MLANG
extern IMultiLanguage  *g_pMultiLanguage;
extern IMultiLanguage2 *g_pMultiLanguage2;
#endif

#ifndef NO_PROPERTY_PAGE
const CLSID * CFormElement::s_apclsidPages[] =
{
    // Browse-time pages
    NULL,
    // Edit-time pages
#if DBG==1    
    &CLSID_CCDGenericPropertyPage,
    &CLSID_CInlineStylePropertyPage,
#endif // DBG==1    
    NULL
};
#endif // NO_PROPERTY_PAGE


BEGIN_TEAROFF_TABLE(CFormElement, IProvideMultipleClassInfo)
    TEAROFF_METHOD(CFormElement, GetClassInfo, getclassinfo, (ITypeInfo ** ppTI))
    TEAROFF_METHOD(CFormElement, GetGUID, getguid, (DWORD dwGuidKind, GUID * pGUID))
    TEAROFF_METHOD(CFormElement, GetMultiTypeInfoCount, getmultitypeinfocount, (ULONG *pcti))
    TEAROFF_METHOD(CFormElement, GetInfoOfIndex, getinfoofindex, (
            ULONG iti,
            DWORD dwFlags,
            ITypeInfo** pptiCoClass,
            DWORD* pdwTIFlags,
            ULONG* pcdispidReserved,
            IID* piidPrimary,
            IID* piidSource))
END_TEAROFF_TABLE()


const CElement::CLASSDESC CFormElement::s_classdesc =
{
    {
        &CLSID_HTMLFormElement,             // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        0,                                  // _dwFlags
        &IID_IHTMLFormElement,              // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLFormElement,       // _pfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};

HRESULT
CFormElement::CreateElement(CHtmTag *pht, CDoc *pDoc, CElement **ppElement)
{
    Assert(pht->Is(ETAG_FORM));
    Assert(ppElement);
    *ppElement = new CFormElement(pDoc);
    return *ppElement ? S_OK: E_OUTOFMEMORY;
}


//+---------------------------------------------------------------------------
//
//  Member :    CFormElement::Init2
//
//  Synopsis:   Last chance to init.
//
//+---------------------------------------------------------------------------

HRESULT
CFormElement::Init2(CInit2Context * pContext)
{
    HRESULT hr = S_OK;
    CDoc *  pDoc = Doc();

    Assert (!_pTypeInfoElements);
    Assert (!_pTypeInfoCoClassElements);
    Assert (!_pTypeInfoImgs);
    Assert (!_pTypeInfoCoClassImgs);

    if (!pDoc || pDoc->_fDesignMode)
        goto Cleanup;

    //
    // Add this form as a named item to the script engine.
    // This is to enable scriptlets for children of the form.  Also
    // to keep compatibility with IE.
    //

    if (pDoc->_pScriptCollection)
    {
        hr = THR(pDoc->_pScriptCollection->AddNamedItem(this));
        if (hr)
            goto Cleanup;
    }

    hr = THR(super::Init2(pContext));

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member :    CFormElement::Passivate
//
//  Synopsis:   1st stage destructor.
//
//+---------------------------------------------------------------------------

void
CFormElement::Passivate()
{
    ClearInterface(&_pTypeInfoElements);
    ClearInterface(&_pTypeInfoCoClassElements);
    ClearInterface(&_pTypeInfoImgs);
    ClearInterface(&_pTypeInfoCoClassImgs);

    // Free Radio groups
    while (_pRadioGrpName)
    {
        RADIOGRPNAME  *pRadioGroup = _pRadioGrpName->_pNext;

        SysFreeString((BSTR)_pRadioGrpName->lpstrName);
        delete _pRadioGrpName;
        _pRadioGrpName = pRadioGroup;
    }

    super::Passivate();
}


//+---------------------------------------------------------------------------
//
//  Member :    CFormElement::PrivateQueryInterface
//
//  Synopsis:   per IPrivateUnknown.
//
//+---------------------------------------------------------------------------

HRESULT
CFormElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IProvideMultipleClassInfo, NULL)
        QI_TEAROFF2(this, IProvideClassInfo, IProvideMultipleClassInfo, NULL)
        QI_TEAROFF2(this, IProvideClassInfo2, IProvideMultipleClassInfo, NULL)
        QI_HTML_TEAROFF(this, IHTMLFormElement2, NULL);

        default:
            RRETURN(super::PrivateQueryInterface(iid, ppv));
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    (*(IUnknown **)ppv)->AddRef();

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     Getelements
//
//  Synopsis:   Return the form elements collection dispatch.  This is just
//              the "controls" in the form, not all elements.
//
//-------------------------------------------------------------------------

HRESULT
CFormElement::get_elements(IDispatch ** ppElemCol)
{
    TraceTag((tagFormElement, "Getelements"));

    HRESULT hr;

    if (!ppElemCol)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppElemCol = NULL;

    hr = THR_NOTRACE(QueryInterface(IID_IDispatch, (void**)
                ppElemCol));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+------------------------------------------------------------------------
//
//  Member:     EnsureCollectionCache
//
//  Synopsis:   Create the form's collection cache if needed.
//
//-------------------------------------------------------------------------

HRESULT
CFormElement::EnsureCollectionCache()
{
    TraceTag((tagFormElement, "EnsureCollectionCache"));

    HRESULT         hr = S_OK;

    if (!_pCollectionCache)
    {
        _pCollectionCache = new CCollectionCache(
                this,
                GetDocPtr(),
                ENSURE_METHOD(CFormElement, EnsureCollections, ensurecollections));
        if (!_pCollectionCache)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(_pCollectionCache->InitReservedCacheItems(FORM_NUM_COLLECTIONS,
            FORM_ELEMENT_COLLECTION /* Identitiy Collection Form == Form.elements */ ));
        if (hr)
            goto Error;

        //
        // To incorporate the VBScript engine dynamic type library, the form calls
        // BuildObjectTypeInfo to create a dynamic type info based on the FORM_ELEMENT_COLLECTION
        // starting at DISPID_COLLECTION_MIN
        //
        // We create two collections on the FORM element:-
        //
        // FORM_ELEMENT_COLLECTION is the "elements" collection used to resolve most names,
        // and serves as the identity collection
        //
        // FORM_NAMED_IMG_COLLECTION contains named IMGs scoped to the FORM. This mimics a Nav feature where named IMG's
        // appear in the name space of the FORM, but not as ordinals in the collection.
        //

        _pCollectionCache->SetDISPIDRange (
            FORM_ELEMENT_COLLECTION,
            DISPID_FORM_ELEMENT_GN_MIN,
            DISPID_FORM_ELEMENT_GN_MAX );

        // Turn off ordinal promotion on the named IMG collection, this stops us
        // resolving ordinals when we call GetIDsOfNamesEx in it.
        _pCollectionCache->DontPromoteOrdinals ( FORM_NAMED_IMG_COLLECTION );

        // Set the DISPID range for the second collection, so we can distinguish between the two
        // in Invoke

        _pCollectionCache->SetDISPIDRange (
            FORM_NAMED_IMG_COLLECTION,
            DISPID_FORM_NAMED_IMG_GN_MIN,
            DISPID_FORM_NAMED_IMG_GN_MAX );
    }

Cleanup:
    RRETURN(hr);

Error:
    delete _pCollectionCache;
    _pCollectionCache = NULL;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     EnsureCollections
//
//  Synopsis:   Refresh the form elements collection, if needed.
//
//-------------------------------------------------------------------------

HRESULT
CFormElement::EnsureCollections(long lIndex, long * plCollectionVersion)
{
    TraceTag((tagFormElement, "EnsureCollections Version:%d", *plCollectionVersion));

    CDoc *  pDoc = Doc();
    long    lSize;
    long    l;
    HRESULT hr;
    LPCTSTR szFormName;
    BOOL fAddToNamedImages = FALSE, fAddToSubmit = FALSE,fAddToElements = FALSE ;
    CCollectionCache *pCollectionCache;

#ifdef PERFMETER
    static PERFMETERTAG s_mpColMtr[] = { Mt(BldFormElementCol), Mt(BldFormNamedImgCol), Mt(BldFormSubmitCol) };
#endif

    hr = THR(EnsureInMarkup());
    if (hr)
        goto Cleanup;

    // For
    // Makes sure the doc's collections are up-to-date.
    l = *plCollectionVersion;
    hr = THR(GetMarkupPtr()->EnsureCollectionCache(CMarkup::ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    // Nothing to do so get out.
    if (*plCollectionVersion == pDoc->GetDocTreeVersion())
        return S_OK;

    MtAdd(s_mpColMtr[lIndex], +1, 0);

    // BUGBUG Rgardner - make sure that all dependent collections are ensured
    if ( lIndex == FORM_NAMED_IMG_COLLECTION )
        fAddToNamedImages = TRUE;
    else if ( lIndex == FORM_SUBMIT_COLLECTION )
        fAddToSubmit = TRUE;
    else if ( lIndex == FORM_ELEMENT_COLLECTION )
        fAddToElements = TRUE;

    pCollectionCache = GetMarkupPtr()->CollectionCache();

    // BUGBUG isn't this the same as the same as the srcIndex???
    //    I think we can remove this line.
    // Find this form element in the doc's elements collection (0).
    hr = THR(pCollectionCache->GetIntoAry(CMarkup::ELEMENT_COLLECTION, this, &l));
    if (hr)
        goto Cleanup;

    // Get the size of the doc's elements collection (0).
    lSize = pCollectionCache->SizeAry(CMarkup::ELEMENT_COLLECTION);

    // Reset the arrays before loading them.  This is the Member cache, not the Markup cache!
    _pCollectionCache->ResetAry(lIndex);

    szFormName = GetAAname();

    // Reload this collection.
    for (++l; l < lSize; ++l)
    {
        CElement * pElemCandidate;
        CTreeNode * pNodeCandidate;

        WHEN_DBG( pElemCandidate = NULL; )

        hr = THR(
            pCollectionCache->GetIntoAry( CMarkup::ELEMENT_COLLECTION, l, & pElemCandidate ) );

        if (hr)
            goto Error;

        Assert( pElemCandidate );
        pNodeCandidate = pElemCandidate->GetFirstBranch();
        Assert ( pNodeCandidate );

        //
        // Search for the element to ensure that it is within the form.  If not
        // then break out.
        //

        if (!pNodeCandidate->SearchBranchToRootForScope(this))
            break;              // Yes, so we're done.

        switch (pNodeCandidate->TagType())
        {
            //
            // If this ever changes, please change the
            // CDoc::AddToCollections list also
            //
        case ETAG_IMG:
            // Add named IMGs scoped by this
            if ( fAddToNamedImages )
            {
                hr = THR(_pCollectionCache->SetIntoAry( FORM_NAMED_IMG_COLLECTION,
                            pElemCandidate ));
                if (hr)
                    goto Error;
            }
            break;

        case ETAG_INPUT:
            // Ignore INPUT TYPE=IMAGE
            if (DYNCAST(CInput, pNodeCandidate->Element())->GetAAtype() == htmlInputImage)
            {
                if ( fAddToSubmit )
                {
                    hr = THR(_pCollectionCache->SetIntoAry( FORM_SUBMIT_COLLECTION,
                                pElemCandidate ));
                }
                break;
            }
            hr = S_OK;
            // otherwise intentional fall through

        case ETAG_FIELDSET :
        case ETAG_BUTTON:
        case ETAG_SELECT:
        case ETAG_TEXTAREA:
#ifdef  NEVER
        case ETAG_HTMLAREA:
#endif
        case ETAG_OBJECT:
        case ETAG_EMBED:
            if ( fAddToElements )
            {
                hr = THR(_pCollectionCache->SetIntoAry( FORM_ELEMENT_COLLECTION,
                            pElemCandidate ));
                if (hr)
                    goto Error;
            }
            if ( fAddToSubmit )
            {
                hr = THR(_pCollectionCache->SetIntoAry( FORM_SUBMIT_COLLECTION,
                            pElemCandidate ));
                if (hr)
                    goto Error;
            }
            break;
        }
    }

    *plCollectionVersion = pDoc->GetDocTreeVersion();

Cleanup:
    RRETURN(hr);

Error:
    _pCollectionCache->ResetAry(lIndex);
    goto Cleanup;
}


//+--------------------------------------------------------------
//
//  Member:     CFormElement::CallGetSubmitInfo
//
//  Synopsis:   Iterate thru elements, and call GetSubmitinfo on them.
//              this should construct the get/post data string in the
//              format: n=v[&n=v[...]]\0
//
//---------------------------------------------------------------
HRESULT
CFormElement::CallGetSubmitInfo( 
        CElement * pSubmitSite,
        CElement** ppInputImg,
        CPostData* pSubmitData,
        int      * pnMultiLines,
        int      * pnFieldsChanged,
        LPCTSTR  * ppchAction,
        BOOL     * pfSendAsPost,
        BOOL       fUseUtf8, /*= FALSE*/
        CODEPAGE   cp /*= NULL*/)
{
    HRESULT     hr;
    ULONG       i;
    int         cbSizeOld;
    BOOL        fCanUseUTF8     = Utf8InAcceptCharset();
    ULONG       cElements       = 0;
    CDoc *      pDoc            = Doc();

    *pfSendAsPost = (GetAAmethod() == htmlMethodPost);

    *ppchAction = GetAAaction();
    if (!(*ppchAction && *ppchAction[0]))
        *ppchAction = pDoc->_cstrUrl;

    //  Get the ENCTYPE attrib of the form. Set it into the SubmitData object
    pSubmitData->_encType = GetAAencoding();

    // Netscape compatibility: if encType==text/plain then make it a POST
    if (pSubmitData->_encType == htmlEncodingText && !*pfSendAsPost)
    {
        UINT    uProt = GetUrlScheme(*ppchAction);

        if (URL_SCHEME_HTTP == uProt ||
            URL_SCHEME_HTTPS == uProt)
        {
            *pfSendAsPost = TRUE;
        }
    }

    if (!*pfSendAsPost)
    {
        // GET only allows one possible encoding
        pSubmitData->_encType = htmlEncodingURL;
    }

    hr = pSubmitData->CreateHeader();
    if (hr)
        goto Cleanup;

    // get size of collection
    cElements = _pCollectionCache->SizeAry(FORM_SUBMIT_COLLECTION);

    if (fUseUtf8)
    {
        Assert(!cp);
        pSubmitData->_fUseUtf8 = TRUE;
    }

    if (cp)
    {
        Assert(!fUseUtf8);
        pSubmitData->_fUseCustomCodePage = TRUE;
        pSubmitData->_cpInit = cp;
    }   

    for (i = 0; i < cElements; i++)
    {
        CElement * pElement;
        const TCHAR *   pchName;

        hr = THR(_pCollectionCache->GetIntoAry(FORM_SUBMIT_COLLECTION,
                        i,
                        &pElement ) );

        if (hr)
            goto Cleanup;

        Assert(pElement->NeedsLayout());

        // Per W3C, don't submit anything for diabled controls. This
        // is incompatible with Nav3/4, which do not support disabled
        // controls at all.
        if (!pElement->IsEnabled())
            continue;

        switch(pElement->Tag())
        {
        // Always skip non-input images
        // submit
        case    ETAG_IMG:
            continue;

        case ETAG_INPUT:
            switch (DYNCAST(CInput, pElement)->GetType())
            {
            case    htmlInputSubmit:
                    // (NS compatibility) If this is a 'submit' button and has
                    // no name (value=NULL), then don't send any submit info
                    // for this control. BUT if the control has a blank name
                    // (value=""), then do send. (ie. "...&=Submit")
                    pchName = pElement->GetAAsubmitname();
                    if ( ! pchName )
                        continue;
            case    htmlInputReset:
            case    htmlInputButton:
                if (!pSubmitSite || pSubmitSite != pElement)
                {
                    continue;
                }
                break;

            // For input images,
            // 1) do not send data if they did not initiate the submit
            // 2) send data in the end, if they did initiate the submit
            case    htmlInputImage:
                if (!pSubmitSite || pSubmitSite != pElement)
                    continue;
                if (i == cElements - 1)
                    break;
                Assert(!*ppInputImg);
                *ppInputImg = pElement;
                continue;
            }

            break;
        }


        hr = THR(pSubmitData->AppendItemSeparator());
        if ( hr )
            goto Cleanup;

        cbSizeOld = pSubmitData->Size();
        hr = THR_NOTRACE(pElement->GetSubmitInfo(pSubmitData));

        hr = (hr == S_FALSE) ? S_OK : hr;
        if (hr)
            goto Cleanup;  // not OK, not E_NOTIMPL

        //
        //  If the form can use utf-8 and we've found an element
        //  that does not use the default charset, break out of the
        //  loop and use utf-8 instead.
        //  NOTE (krisma) if these conditions are all true, 
        //  CallGetSubmitInfo will be called again by DoSubmit.
        //  See the note in that function.
        //
        if (pSubmitData->_fCharsetNotDefault && fCanUseUTF8&& !fUseUtf8)
        {
            goto Cleanup;
        }

        // Count the number of text fields, etc. This info is used to throw
        // the appropriate security alert
        switch(pElement->Tag())
        {
        case ETAG_INPUT:
            if (DYNCAST(CInput, pElement)->_fTextChanged)
            {
                (*pnFieldsChanged)++;
            }
            break;
#ifdef  NEVER
        case ETAG_HTMLAREA:
#endif
        case ETAG_TEXTAREA:
            if (DYNCAST(CRichtext, pElement)->_fTextChanged)
            {
                (*pnFieldsChanged)++;
                *pnMultiLines += DYNCAST(CRichtext,
                            pElement)->Layout()->GetDisplay()->LineCount() - 1;
            }
            break;
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+--------------------------------------------------------------
//
//  Member:     CFormElement::submit
//
//  Synopsis:   Executes the submit method on the form
//              enumerates all contained sites
//              and calls the appropriated apis on the site
//              to get the submit infos
//              Then constructs the submit string and uses the
//              hyperlink apis to submit
//              This function is exclusively for use in scripts.
//              It does not fire onSubmit event. DoSubmit() must
//              be directly called for internal use.
//
//---------------------------------------------------------------
HRESULT
CFormElement::submit()
{
    RRETURN(DoSubmit(NULL, FALSE));
}

//+--------------------------------------------------------------
//
//  Member:     CFormElement::DoSubmit
//
//  Synopsis:   Executes the submit method on the form
//              enumerates all contained sites
//              and calls the appropriated apis on the site
//              to get the submit infos
//              Then constructs the submit string and uses the
//              hyperlink apis to submit
//
//---------------------------------------------------------------
HRESULT
CFormElement::DoSubmit(CElement *pSubmitSite, BOOL fFireEvent)
{
    HRESULT     hr              = S_OK;
    CPostData   SubmitData;
    CDwnPost *  pDwnPost        = NULL;
    CDoc *      pDoc            = Doc();
    //  BUGBUG(laszlog): The SubmitData object will refer back to the doc.
    //                   Should we CLock() the Doc?
    int         nMultiLines     = 0;
    int         nFieldsChanged  = 0;
    DWORD       dwSecurityCode  = ERROR_SUCCESS;
    int         cbSizeOld;
//  TCHAR *     pchExpandedUrl  = NULL;
    CElement *  pInputImg       = NULL;
    BOOL        fCanUseUTF8     = Utf8InAcceptCharset();
    BOOL        fUseUtf8        = FALSE;
    LPCTSTR     pchAction;
    BOOL        fSendAsPost;
    CODEPAGE    cp              = NULL;
    
    IMLangCodePages * pMLangCodePages = NULL;

    TraceTag((tagFormElement, "submit"));

    if (fFireEvent && !Fire_onsubmit())
        goto Cleanup;

/*
    BUGBUG (MohanB) - Comment this out for now. Netscape seems to
    send the submit data even for non-HTTP protocols and we must
    strive to be compatible ! However, if the shell's Navigate()
    cannot handle it, we will put this code back in.

    // Do not collect/send submit data if action is not an http
    // fully resolve URL
    hr = THR(pDoc->ExpandUrl(pchAction, &pchExpandedUrl, this));
    if (hr)
        goto Cleanup;
    switch (GetUrlScheme(pchExpandedUrl))
    {
    case URL_SCHEME_HTTPS:
    case URL_SCHEME_HTTP:
        break;
    default:
        hr = THR(pDoc->FollowHyperlink(pchAction,       // Action
                                       GetAAtarget(),   // Target
                                       this,            // pElementContext
                                       NULL,            // data to be submitted
                                       GetAAmethod()==htmlMethodPost));
        goto Cleanup;
    }
*/

    // Collect and send submit data
    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->EnsureAry(FORM_SUBMIT_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = THR(CallGetSubmitInfo(
        pSubmitSite,
        &pInputImg,
        &SubmitData,
        &nMultiLines,
        &nFieldsChanged,
        &pchAction,
        &fSendAsPost));

    if (hr)
        goto Cleanup;

    // Should convert the submit data to UTF-8?
    // NOTE (krisma) If the following conditions are true,
    // we ended the previous CallGetSubitInfo early.
    // see the note in that function.
    if (SubmitData._fCharsetNotDefault)
    {
        if (fCanUseUTF8)
        {
            fUseUtf8 = TRUE;
        }
        else if (!SubmitData._fCodePageError && !pDoc->HaveCodePageMetaTag())
        {
            hr = THR( EnsureMultiLanguage() );
            if (hr)
                goto Cleanup;

            Assert( g_pMultiLanguage );

            hr = THR( g_pMultiLanguage->QueryInterface( IID_IMLangCodePages, (void**)&pMLangCodePages ) );
            if (hr)
                goto Cleanup;

            hr = THR( pMLangCodePages->CodePagesToCodePage( SubmitData._dwCodePages, NULL, &cp ));
            if (hr)
                goto Cleanup;
        }

        SubmitData.DeleteAll();

        hr = THR(CallGetSubmitInfo(
            pSubmitSite,
            &pInputImg,
            &SubmitData,
            &nMultiLines,
            &nFieldsChanged,
            &pchAction,
            &fSendAsPost,
            fUseUtf8,
            cp));

        if (hr)
            goto Cleanup;
    }

    if (pInputImg)
    {
        SubmitData.AppendItemSeparator();
        cbSizeOld = SubmitData.Size();
        hr = THR_NOTRACE(pInputImg->GetSubmitInfo(&SubmitData));

        hr = (hr == S_FALSE) ? S_OK : hr;
        if (hr)
            goto Cleanup;  // not OK, not E_NOTIMPL
    }

    //  Finish up the SubmitData

    hr = THR(SubmitData.Finish());
    if ( hr )
        goto Cleanup;

        // The user has made a non-trivial change if (the heuristic we use is
    // same as what was used in IE3) he/she has edited at least two fields
    // or entered more than one line in any textarea.
    if (nFieldsChanged > 1 || nMultiLines > 0)
        dwSecurityCode = ERROR_INTERNET_CHG_POST_IS_NON_SECURE;
    else
        dwSecurityCode = ERROR_INTERNET_POST_IS_NON_SECURE;

    hr = THR(CDwnPost::Create(&SubmitData, &pDwnPost));
    if (hr)
        goto Cleanup;

    hr = THR(pDoc->FollowHyperlink(pchAction,       // Action
                                   GetAAtarget(),   // Target
                                   this,            // pElementContext
                                   pDwnPost,        // data to submit
                                   fSendAsPost,     // POST or GET
                                   FALSE,           // OpenInNewWindow
                                   NULL,            // pUnkFrame
                                   0,               // dwBindOptions
                                   dwSecurityCode));

    if ( E_ABORT == hr )
    {
        hr = S_OK;
    }

Cleanup:
    ReleaseInterface( pMLangCodePages );
    ReleaseInterface(pDwnPost);
    RRETURN(SetErrorInfo(hr));
}


//+--------------------------------------------------------------
//
//  Member:     CFormElement::Utf8InAcceptCharset
//
//  Synopsis:   Searches through the accept-charset attribute
//              for the string "utf-8" (case insensitive)
//
//---------------------------------------------------------------
BOOL
CFormElement::Utf8InAcceptCharset()
{
    BOOL    fRetVal = FALSE;
    TCHAR * pch = (TCHAR *) GetAAacceptCharset();

    while (pch && *pch)
    {
        if (_7csnipre(TEXT("utf-8"), 5, pch, -1))
        {
            fRetVal = TRUE;
            break;
        }
        pch++;
    }

    //
    // Let's make sure the next character is either a 
    // space, commma, or the end of the string
    //
    pch += 5;   //"utf-8" is 5 characters long
    if (fRetVal && *pch)
    {
        if (!_7csnipre(TEXT(" "), 1, pch, 1)
            && !_7csnipre(TEXT(","), 1, pch, 1))
            fRetVal=FALSE;
    }

    return fRetVal;
}

//+--------------------------------------------------------------
//
//  Member:     CFormElement::reset
//
//  Synopsis:   Executes the reset method on the form
//              enumerates all contained sites
//              and calls the reset API on the site
//              This function is exclusively for use in scripts.
//              DoReset() must be directly called for internal
//              use.
//
//---------------------------------------------------------------
HRESULT
CFormElement::reset()
{
    RRETURN(DoReset(TRUE));
}

//+--------------------------------------------------------------
//
//  Member:     CFormElement::DoReset
//
//  Synopsis:   Executes the reset method on the form
//              enumerates all contained sites
//              and calls the reset API on the site
//
//---------------------------------------------------------------
HRESULT
CFormElement::DoReset(BOOL fFireEvent)
{
    TraceTag((tagFormElement, "reset"));

    HRESULT     hr = S_OK;
    long        c =0;

    if (fFireEvent && !Fire_onreset())
        goto Cleanup;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->EnsureAry(FORM_ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    // get size of collection
    c = _pCollectionCache->SizeAry(FORM_ELEMENT_COLLECTION);

    //
    // Iterate thru elements, and call DoReset on them.
    //
    while (c--)
    {
        CElement * pElem;

        hr = THR(_pCollectionCache->GetIntoAry(FORM_ELEMENT_COLLECTION,
                        c,
                        &pElem ) );
        if (pElem->NeedsLayout() &&  !hr)
            IGNORE_HR(pElem->DoReset());
    }

    hr = S_OK;

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CFormElement::InvokeEx
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

#ifdef USE_STACK_SPEW
#pragma check_stack(off)
#endif

HRESULT
CFormElement::ContextThunk_InvokeEx (
    DISPID          dispidMember,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pdispparams,
    VARIANT *       pvarResult,
    EXCEPINFO *     pexcepinfo,
    IServiceProvider *pSrvProvider)
{
    IUnknown * pUnkContext;

    // Magic macro which pulls context out of nowhere (actually eax)
    CONTEXTTHUNK_SETCONTEXT

    TraceTag((tagFormElement, "Invoke dispid=0x%x", dispidMember));

    HRESULT hr;
    RETCOLLECT_KIND collectionCreation;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = DISP_E_MEMBERNOTFOUND;

    // If the DISPID came from the typeinfo, adjust the DISPID into GetIdsOfNames range,
    // and record that the Invoke should only return a single item, not a collection

    if ( dispidMember >= DISPID_COLLECTION_TI_MIN &&
         dispidMember <= DISPID_COLLECTION_TI_MAX )
    {
        dispidMember = DISPID_COLLECTION_TI_TO_GN(dispidMember);

        collectionCreation = RETCOLLECT_FIRSTITEM;
    }
    else
    {
        collectionCreation = RETCOLLECT_ALL;
    }


    // Ensuring the 
    if ( _pCollectionCache->IsDISPIDInCollection ( FORM_ELEMENT_COLLECTION , dispidMember ) )
    {
        hr = THR(_pCollectionCache->EnsureAry(FORM_ELEMENT_COLLECTION));
        if (hr)
            goto Cleanup;
        hr = _pCollectionCache->Invoke(FORM_ELEMENT_COLLECTION,
                                      dispidMember,
                                      IID_NULL,
                                      lcid,
                                      wFlags,
                                      pdispparams,
                                      pvarResult,
                                      pexcepinfo,
                                      NULL,
                                      collectionCreation);
    }
    else 
    {

        if ( _pCollectionCache->IsDISPIDInCollection ( FORM_NAMED_IMG_COLLECTION , dispidMember ) )
        {
            hr = THR(_pCollectionCache->EnsureAry(FORM_NAMED_IMG_COLLECTION));
            if (hr)
                goto Cleanup;
            hr = _pCollectionCache->Invoke(FORM_NAMED_IMG_COLLECTION,
                                          dispidMember,
                                          IID_NULL,
                                          lcid,
                                          wFlags,
                                          pdispparams,
                                          pvarResult,
                                          pexcepinfo,
                                          NULL);
        }
    }

    // If above didn't work then try to get the property/expando.
    if (hr)
    {
        hr = THR_NOTRACE(super::ContextInvokeEx (dispidMember,
                                   lcid,
                                   wFlags,
                                   pdispparams,
                                   pvarResult,
                                   pexcepinfo,
                                   pSrvProvider,
                                   pUnkContext ? pUnkContext : (IUnknown*)this));
    }

Cleanup:
    RRETURN(hr);
}

#ifdef USE_STACK_SPEW
#pragma check_stack(on)
#endif

HRESULT
CFormElement::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT     hr = DISP_E_UNKNOWNNAME;
    
    if(IsInMarkup())
    {
        // make sure the form element is inside the tree
        hr = THR(EnsureCollectionCache());
        if (hr)
            goto Cleanup;

        Assert (_pCollectionCache);


        hr = THR(_pCollectionCache->EnsureAry(FORM_ELEMENT_COLLECTION));
        if ( hr )
            goto Cleanup;

        // Try the FORM_ELEMENT_COLLECTION first
        hr = THR_NOTRACE(_pCollectionCache->GetDispID(FORM_ELEMENT_COLLECTION,
                                                      bstrName,
                                                      grfdex,
                                                      pid));

        // If not there try the FORM_NAMED_IMG_COLLECTION. Note that we've turned off
        // ordinal promotion on this collection.
        if ( hr == DISP_E_UNKNOWNNAME )
        {
            hr = THR(_pCollectionCache->EnsureAry(FORM_NAMED_IMG_COLLECTION));
            if ( hr )
                goto Cleanup;
            hr = THR_NOTRACE(_pCollectionCache->GetDispID(FORM_NAMED_IMG_COLLECTION,
                                                          bstrName,
                                                          grfdex,
                                                          pid));
        }
    }

    // The collectionCache GetDispID will return S_OK w/ DISPID_UNKNOWN
    // if the name isn't found, catastrophic errors are of course returned.
    if (hr || (!hr && *pid == DISPID_UNKNOWN))
    {
        // Don't allow vbscript fast event sinks to be hooked up.
        if (hr && (grfdex & fdexNameNoDynamicProperties))
        {
            *pid = DISPID_UNKNOWN;
            goto Cleanup;
        }

        hr = THR_NOTRACE(super::GetDispID(bstrName, grfdex, pid));
    }

Cleanup:
    RRETURN(hr);
}


HRESULT
CFormElement::GetNextDispID(
                DWORD grfdex,
                DISPID id,
                DISPID *prgid)
{
    HRESULT     hr;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    if(IsInMarkup())
    {
        hr = THR(_pCollectionCache->EnsureAry(FORM_ELEMENT_COLLECTION));
        if (hr)
            goto Cleanup;
    }

    hr = DispatchGetNextDispIDCollection(this,
#ifndef WIN16
                                         (GetNextDispIDPROC)&super::GetNextDispID,
#else
                                         CBase::GetNextDispID,
#endif
                                         _pCollectionCache,
                                         FORM_ELEMENT_COLLECTION,
                                         grfdex,
                                         id,
                                         prgid);

Cleanup:
    RRETURN1(hr, S_FALSE);
}

HRESULT
CFormElement::GetMemberName(
                DISPID id,
                BSTR *pbstrName)
{
    HRESULT     hr;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = DispatchGetMemberNameCollection(this,
#ifndef WIN16
                                         (GetGetMemberNamePROC)super::GetMemberName,
#else
                                         CBase::GetMemberName,
#endif
                                         _pCollectionCache,
                                         FORM_ELEMENT_COLLECTION,
                                         id,
                                         pbstrName);

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CFormElement::GetMultiTypeInfoCount
//
//  Synopsis:   per IProvideMultipleClassInfo
//
//---------------------------------------------------------------------------

HRESULT
CFormElement::GetMultiTypeInfoCount(ULONG *pc)
{
    TraceTag((tagFormElement, "GetMultiTypeInfoCount"));

    *pc = 3;
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Member:     CFormElement::GetInfoOfIndex
//
//  Synopsis:   per IProvideMultipleClassInfo
//
//---------------------------------------------------------------------------

HRESULT
CFormElement::GetInfoOfIndex(
    ULONG       iTI,
    DWORD       dwFlags,
    ITypeInfo** ppTICoClass,
    DWORD*      pdwTIFlags,
    ULONG*      pcdispidReserved,
    IID*        piidPrimary,
    IID*        piidSource)
{
    TraceTag((tagFormElement, "GetInfoOfIndex"));

    HRESULT         hr = S_OK;
    long            lIndex;
    DISPID          dispidMin;
    DISPID          dispidMax;
    ITypeInfo **    ppTypeInfo = NULL;
    ITypeInfo **    ppTypeInfoCoClass = NULL;

    //
    // First try the main typeinfo
    //

    if (dwFlags & MULTICLASSINFO_GETTYPEINFO)
    {
        //
        // If the type-info to be created on the fly has not yet
        // happened then create them
        //

        if (1 == iTI)
        {
            lIndex = FORM_ELEMENT_COLLECTION;
            dispidMin = DISPID_FORM_ELEMENT_TI_MIN;
            dispidMax = DISPID_FORM_ELEMENT_TI_MAX;
            ppTypeInfo        = &_pTypeInfoElements;
            ppTypeInfoCoClass = &_pTypeInfoCoClassElements;
        }
        else if (2 == iTI)
        {
            lIndex = FORM_NAMED_IMG_COLLECTION;
            dispidMin = DISPID_FORM_NAMED_IMG_TI_MIN;
            dispidMax = DISPID_FORM_NAMED_IMG_TI_MAX;
            ppTypeInfo        = &_pTypeInfoImgs;
            ppTypeInfoCoClass = &_pTypeInfoCoClassImgs;
        }
        else
        {
            goto Dosuper;
        }

        hr = THR(EnsureCollectionCache());
        if (hr)
            goto Cleanup;

        hr = THR(_pCollectionCache->EnsureAry(lIndex));
        if (hr)
            goto Cleanup;

        Assert (ppTypeInfo && ppTypeInfoCoClass);

        if (!(*ppTypeInfo) || !(*ppTypeInfoCoClass))
        {
            hr = THR(Doc()->BuildObjectTypeInfo(
                _pCollectionCache,
                lIndex,
                dispidMin,
                dispidMax,
                ppTypeInfo,
                ppTypeInfoCoClass));
            if (hr)
                goto Cleanup;
        }

        *ppTICoClass = *ppTypeInfoCoClass;
        (*ppTICoClass)->AddRef();

        //
        // Clear out these values so that we can use the base impl.
        //

        dwFlags &= ~MULTICLASSINFO_GETTYPEINFO;
        iTI = 0;
        ppTICoClass = NULL;
    }

Dosuper:
    hr = THR(super::GetInfoOfIndex(
            iTI,
            dwFlags,
            ppTICoClass,
            pdwTIFlags,
            pcdispidReserved,
            piidPrimary,
            piidSource));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+--------------------------------------------------------------
//
//  Member:     CFormElement::FOnlyTextbox
//
//  Synopsis:   Called by an input textbox to determine whether
//              it should cause the form to be submitted upon
//              receiving a VK_ENTER.
//
//---------------------------------------------------------------

HRESULT
CFormElement::FOnlyTextbox(CInput * pTextbox, BOOL * pfOnly)
{
    HRESULT     hr = S_OK;
    long        i;
    CElement *  pElem;

    Assert(pTextbox);
    Assert(pTextbox->Tag() == ETAG_INPUT);
    Assert(pTextbox->GetType() != htmlInputButton);
    Assert(pTextbox->GetType() != htmlInputReset);
    Assert(pTextbox->GetType() != htmlInputSubmit);

    Assert(pfOnly);
    *pfOnly = TRUE;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->EnsureAry(FORM_ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    for (i = _pCollectionCache->SizeAry(FORM_ELEMENT_COLLECTION) - 1;
            i >= 0 && *pfOnly; i--)
    {
        hr = THR(_pCollectionCache->GetIntoAry(FORM_ELEMENT_COLLECTION,
                        i,
                        &pElem));
        if (hr)
            goto Cleanup;

        if (pElem != pTextbox && pElem->Tag() == ETAG_INPUT)
        {
            CInput * pInput = DYNCAST(CInput, pElem);

            switch (pInput->GetType())
            {
            case htmlInputText:
            case htmlInputPassword:
                *pfOnly = FALSE;  // found another textbox in this form
            }
        }
    }
Cleanup:
    RRETURN(hr);
}

//+--------------------------------------------------------------
//
//  Member:     CFormElement::FormTraverseGroup
//
//  Synopsis:   Called by (e.g.)a radioButton to its form, this function
//      takes the groupname and queries the Form's collection for the rest
//      of the group and calls the provided CLEARGROUP function on that
//      element. in this way ANY Form GRoup can be cleared (or have some
//      opetation done on all its members)
//
//---------------------------------------------------------------

HRESULT
CFormElement::FormTraverseGroup(
    LPCTSTR strGroupName,
    PFN_VISIT pfn,
    DWORD_PTR dw,
    BOOL fForward)
{
    HRESULT     hr;
    long        i, c;
    CElement *  pElem;
    LPCTSTR     lpName;

    _fInTraverseGroup = TRUE;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->EnsureAry(FORM_ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    // get size of collection
    c = _pCollectionCache->SizeAry(FORM_ELEMENT_COLLECTION);

    if (fForward)
        i = 0;
    else
        i = c - 1;

    // if nothing is in the collection, default answer is S_FALSE.
    hr = S_FALSE;

    while (c--)
    {
        hr = THR(_pCollectionCache->GetIntoAry(FORM_ELEMENT_COLLECTION,
                        i,
                        &pElem));
        if (fForward)
            i++;
        else
            i--;

        if (hr)
            goto Cleanup;

        lpName = pElem->GetAAname();

        hr = S_FALSE;                   // default answer again.

        // is this item in the target group?
        if ( lpName && FormsStringICmp(strGroupName, lpName) == 0 )
        {
            // Call the function and stop if it doesn't return S_FALSE.
#ifdef WIN16
            hr = THR( (*pfn)(pElem, dw) );
#else
            hr = THR( CALL_METHOD( pElem, pfn, (dw)) );
#endif
            if (hr != S_FALSE)
                break;
        }
    }

Cleanup:
    _fInTraverseGroup = FALSE;
    RRETURN1(hr, S_FALSE);
}

//+--------------------------------------------------------------
//
//  Member:     CFormElement::FindDefaultElem
//
//  Synopsis:   find the default/Cancel button
//
//---------------------------------------------------------------

CElement *
CFormElement::FindDefaultElem(BOOL fDefault, BOOL fCurrent /* FALSE */)
{
    HRESULT     hr      = S_FALSE;
    long        i       = 0;
    long        c       = 0;
    CElement  * pElem   = NULL;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->EnsureAry(FORM_ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    // get size of collection
    c = _pCollectionCache->SizeAry(FORM_ELEMENT_COLLECTION);

    while (c--)
    {
        hr = THR(_pCollectionCache->GetIntoAry(FORM_ELEMENT_COLLECTION,
                        i++,
                        &pElem));

        if (hr)
        {
            pElem = NULL;
            goto Cleanup;
        }

        Assert(pElem);
        if (pElem->_fExittreePending)
            continue;

        if (fCurrent)
        {
            if (pElem->_fDefault)
                goto Cleanup;
            continue;
        }

        if ( pElem->TestClassFlag(fDefault?
                ELEMENTDESC_DEFAULT : ELEMENTDESC_CANCEL)
            && pElem->IsVisible(TRUE)
            && pElem->IsEnabled()
            )
        {
                goto Cleanup;
        }
    }
    pElem = NULL;

Cleanup:
    return pElem;
}

//+--------------------------------------------------------------
//
//  Member:     CFormElement::ApplyDefaultFormat
//
//  Synopsis: Provide for special formatting for the form. So far
//            the only interesting thing here is that <FORM>s generate
//            extra vertical white space in Netscape, but not in IE.
//            We imitate Netscape.
//
//---------------------------------------------------------------

HRESULT
CFormElement::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT         hr = S_OK;

    hr = THR(super::ApplyDefaultFormat(pCFI));
    if(hr)
        goto Cleanup;

    pCFI->PrepareFancyFormat();
    ApplyDefaultVerticalSpace(&pCFI->_ff());
    pCFI->UnprepareForDebug();

Cleanup:
    return (hr);
}

//+--------------------------------------------------------------
//
//  Member:     CFormElement::put_method
//
//  Synopsis: needed because of special get_method handling
//
//+--------------------------------------------------------------
STDMETHODIMP CFormElement::put_method(BSTR bstr)
{
    VARIANT v;
    v.vt = VT_BSTR;
    V_BSTR(&v) = bstr;
    RRETURN(SetErrorInfo(s_propdescCFormElementmethod.a.HandleEnumProperty(HANDLEPROP_SET |
                                                                 HANDLEPROP_AUTOMATION |
                                                                 (PROPTYPE_VARIANT << 16),
                                                                 &v,
                                                                 this,
                                                                 (CVoid *)(void *)(&_pAA))));
}
//+--------------------------------------------------------------
//
//  Member:     CFormElement::get_method
//
//  Synopsis: Through the OM when the value is notSet we return Get
//
//  N.B. this function assumes intimate knowledge of the htmlMethod enum.
//       and the possible values. if the number of enums changes, or the
//       indicies into the enum then this function will need attention
//+--------------------------------------------------------------
STDMETHODIMP CFormElement::get_method(BSTR * pbstr)
{
    HRESULT hr;

    if (!pbstr)
    {
        hr = E_POINTER;
        goto Cleanup;
    }



    if(GetAAmethod()!=htmlMethodPost)
    {
        // then return the "get" string
        hr = THR(FormsAllocString( s_enumdeschtmlMethod.aenumpairs[ htmlMethodGet ].pszName,
                                   pbstr));
    }
    else
    {
        // then return the "Post" string
        hr = THR(FormsAllocString( s_enumdeschtmlMethod.aenumpairs[ htmlMethodPost ].pszName,
                                   pbstr));
    }
    if (hr )
        goto Cleanup;


Cleanup:
    RRETURN(SetErrorInfo( hr));
}



//+------------------------------------------------------------------------
//
//  Member:     get_length
//
//  Synopsis:   collection object model, defers to Cache Helper
//
//-------------------------------------------------------------------------

HRESULT
CFormElement::get_length(long * plSize)
{
    HRESULT hr;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;


    hr = THR(_pCollectionCache->EnsureAry(FORM_ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->GetLength(FORM_ELEMENT_COLLECTION, plSize));

Cleanup:
    RRETURN(SetErrorInfo( hr));

}


//+------------------------------------------------------------------------
//
//  Member:     put_length
//
//  Synopsis:   collection object model, defers to Cache Helper
//
//-------------------------------------------------------------------------

HRESULT
CFormElement::put_length(long lSize)
{
    // supported only in area collection
    RRETURN(SetErrorInfo(E_NOTIMPL));
}


//+------------------------------------------------------------------------
//
//  Member:     item
//
//  Synopsis:   collection object model
//
//-------------------------------------------------------------------------

HRESULT
CFormElement::item(VARIANTARG var1, VARIANTARG var2, IDispatch** ppResult)
{
    HRESULT hr;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;


    hr = THR(_pCollectionCache->EnsureAry(FORM_ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->Item(FORM_ELEMENT_COLLECTION,
                        var1,
                        var2,
                        ppResult));

Cleanup:
    RRETURN(SetErrorInfo( hr));
}


//+------------------------------------------------------------------------
//
//  Member:     tags
//
//  Synopsis:   collection object model, this always returns a collection
//              and is named based on the tag, and searched based on tagname
//
//-------------------------------------------------------------------------

HRESULT
CFormElement::tags(VARIANT var1, IDispatch ** ppdisp)
{
    HRESULT hr;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;


    hr = THR(_pCollectionCache->EnsureAry(FORM_ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->Tags(FORM_ELEMENT_COLLECTION, var1, ppdisp));

Cleanup:
    RRETURN(SetErrorInfo( hr));
}


//+------------------------------------------------------------------------
//
//  Member:     urns
//
//  Synopsis:   collection object model, this always returns a collection
//              and is named based on the urn, and searched based on urn
//
//-------------------------------------------------------------------------

HRESULT
CFormElement::urns(VARIANT var1, IDispatch ** ppdisp)
{
    HRESULT hr;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;


    hr = THR(_pCollectionCache->EnsureAry(FORM_ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->Urns(FORM_ELEMENT_COLLECTION, var1, ppdisp));

Cleanup:
    RRETURN(SetErrorInfo( hr));
}

//+------------------------------------------------------------------------
//
//  Member:     Get_newEnum
//
//  Synopsis:   collection object model
//
//-------------------------------------------------------------------------

HRESULT
CFormElement::get__newEnum(IUnknown ** ppEnum)
{
    HRESULT hr;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    
    hr = THR(_pCollectionCache->EnsureAry(FORM_ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->GetNewEnum(FORM_ELEMENT_COLLECTION, ppEnum));

Cleanup:
    RRETURN(SetErrorInfo( hr));
}
