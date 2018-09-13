//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       comment.cxx
//
//  Contents:   CCommentElement
//
//  History:    
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_COMMENT_HXX_
#define X_COMMENT_HXX_
#include "comment.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#define _cxx_
#include "comment.hdl"

MtDefine(CCommentElement, Elements, "CCommentElement")

//+-----------------------------------------------------------
//
//  Class: CCommentElement
//
//------------------------------------------------------------

const CElement::CLASSDESC CCommentElement::s_classdesc =
{
    {
        &CLSID_HTMLCommentElement,          // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLCommentElement,           // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLCommentElement,    // _apfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};



HRESULT CCommentElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    HRESULT hr = S_OK;

    CCommentElement *pElementComment;

    Assert(ppElement);

    pElementComment = new CCommentElement(pht->GetTag(), pDoc);
    if (!pElementComment)
        return E_OUTOFMEMORY;

    pElementComment->_fAtomic = pht->Is(ETAG_RAW_COMMENT);
    
    *ppElement = (CElement *)pElementComment;
    if (pElementComment->_fAtomic)
    {
        hr = pElementComment->_cstrText.Set(pht->GetPch(), pht->GetCch());
    }
    RRETURN(hr);
}

HRESULT
CCommentElement::Save( CStreamWriteBuff * pStreamWriteBuff, BOOL fEnd )
{
    HRESULT hr = S_OK;

    // Don't write out comments for end tags, plain text, RTF, or if
    // it's the DOCTYPE comment.
    if (fEnd || pStreamWriteBuff->TestFlag(WBF_SAVE_PLAINTEXT) ||
                pStreamWriteBuff->TestFlag(WBF_FOR_RTF_CONV) ||
        (GetFirstBranch() && GetFirstBranch()->Parent()->Tag() == ETAG_ROOT &&
         (_cstrText.Length() && StrCmpNIC(_T("<!DOCTYPE"), _cstrText, 9) == 0)) )
        return(hr);

    DWORD dwOldFlags = pStreamWriteBuff->ClearFlags(WBF_ENTITYREF);

    pStreamWriteBuff->SetFlags(WBF_SAVE_VERBATIM | WBF_NO_WRAP);

    // BUGBUG: hack to preserve line breaks
    pStreamWriteBuff->BeginPre();

    if (!_fAtomic)
    {
        // Save <tagname>
        hr = THR(super::Save(pStreamWriteBuff, FALSE));
        if (hr)
            goto Cleanup;
    }
    hr = THR(pStreamWriteBuff->Write((LPTSTR)_cstrText));

    if (hr)
        goto Cleanup;
    if (!_fAtomic)
    {
        // Save </tagname>
        hr = THR(super::Save(pStreamWriteBuff, TRUE));
        if (hr)
            goto Cleanup;
    }

    //BUGBUG see above
    pStreamWriteBuff->EndPre();

    if (!hr)
    {
        pStreamWriteBuff->RestoreFlags(dwOldFlags);
    }
Cleanup:
    RRETURN(hr);
}


