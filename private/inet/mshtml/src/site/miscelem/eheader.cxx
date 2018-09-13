//+---------------------------------------------------------------------
//
//   File:      eheader.cxx
//
//  Contents:   Header Element class
//
//  Classes:    CHeaderElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_EHEADER_HXX_
#define X_EHEADER_HXX_
#include "eheader.hxx"
#endif

#ifndef X_EFONT_HXX_
#define X_EFONT_HXX_
#include "efont.hxx"
#endif

#ifndef X__DOC_H_
#define X__DOC_H_
#include "_doc.h"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#define _cxx_
#include "header.hdl"


const CElement::CLASSDESC CHeaderElement::s_classdesc =
{
    {
        &CLSID_HTMLHeaderElement,           // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        0,                                  // _dwFlags
        &IID_IHTMLHeaderElement,            // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLHeaderElement,     // _apfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};


HRESULT CHeaderElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert( pht->Is(ETAG_H1) || pht->Is(ETAG_H2) ||
            pht->Is(ETAG_H3) || pht->Is(ETAG_H4) ||
            pht->Is(ETAG_H5) || pht->Is(ETAG_H6));

    Assert(ppElementResult);
    *ppElementResult = new CHeaderElement(pht->GetTag(), pDoc);
    return *ppElementResult ? S_OK : E_OUTOFMEMORY;
}

//+------------------------------------------------------------------------
//
//  Member:     CHeaderElement::ApplyDefaultFormat
//
//  Synopsis:   Applies default formatting properties for that element to 
//              the char and para formats passed in
//
//  Arguments:  pCFI - Format Info needed for cascading
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
                                 
HRESULT                           
CHeaderElement::ApplyDefaultFormat (CFormatInfo *pCFI)
{

    if ( _nLevel >= 1 && _nLevel <= 6 )
    {
        // Note that only the before spacing is sensitive to font height.
        // Weird Nav 3.0 compat.
        pCFI->PrepareFancyFormat();
        ApplyDefaultBeforeSpace ( &pCFI->_ff() ); // No more nav compat, (pCFI->_cf._yHeight + 40) / 20 ); 
        ApplyDefaultAfterSpace ( &pCFI->_ff() );
        pCFI->UnprepareForDebug();

        pCFI->PrepareCharFormat();
        pCFI->_cf().SetHeightInTwips( ConvertHtmlSizeToTwips( 7-_nLevel ) );
        pCFI->_cf()._fBold = TRUE;
        pCFI->_cf()._wWeight = 700;
        pCFI->_cf()._fBumpSizeDown = FALSE; // Nav compat
        pCFI->UnprepareForDebug();
    }

    RRETURN(super::ApplyDefaultFormat(pCFI));
}
