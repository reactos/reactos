//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       eshow.cxx
//
//  Contents:   CNoShowElement
//
//  History:    15-Jul-1996     AnandRa     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ENOSHOW_HXX_
#define X_ENOSHOW_HXX_
#include "enoshow.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#define _cxx_
#include "noshow.hdl"

MtDefine(CNoShowElement, Elements, "CNoShowElement")
MtDefine(CShowElement, Elements, "CShowElement")

//+------------------------------------------------------------------------
//
//  Class:      CNoShowElement
//
//  Synopsis:   
//
//-------------------------------------------------------------------------

const CElement::CLASSDESC CNoShowElement::s_classdesc =
{
    {
        &CLSID_HTMLNoShowElement,           // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLNoShowElement,            // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnIHTMLNoShowElement,

    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};


HRESULT
CNoShowElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);
    *ppElementResult = new CNoShowElement(pht->GetTag(), pDoc);

    return (*ppElementResult ? S_OK : E_OUTOFMEMORY);
}

//+---------------------------------------------------------------------------
//
//  Member:     CNoShowElement::Save
//
//  Synopsis:   called twice: for opening <NOFRAMES> and for </NOFRAMES>.
//
//----------------------------------------------------------------------------

HRESULT
CNoShowElement::Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd)
{
    HRESULT hr;

    hr = THR(super::Save(pStreamWrBuff, fEnd));
    if (hr)
        goto Cleanup;

    if (!fEnd && !pStreamWrBuff->TestFlag(WBF_SAVE_PLAINTEXT))
    {
        DWORD dwOldFlags = pStreamWrBuff->ClearFlags(WBF_ENTITYREF);

        pStreamWrBuff->SetFlags(WBF_KEEP_BREAKS | WBF_NO_WRAP);

        if (_cstrContents.Length())
        {
            hr = THR(pStreamWrBuff->Write(_cstrContents));
            if (hr)
                goto Cleanup;
        }

        pStreamWrBuff->RestoreFlags(dwOldFlags);
    }

Cleanup:

    RRETURN1(hr, S_FALSE);
}


