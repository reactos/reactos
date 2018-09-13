//+---------------------------------------------------------------------
//
//   File:      ephrase.cxx
//
//  Contents:   Phrase element class
//
//  Classes:    CPhraseElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_EPHRASE_HXX_
#define X_EPHRASE_HXX_
#include "ephrase.hxx"
#endif

#ifndef X_EFONT_HXX_
#define X_EFONT_HXX_
#include "efont.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_E1D_HXX_
#define X_E1D_HXX_
#include "e1d.hxx"
#endif

#ifndef X_CODEPAGE_H_
#define X_CODEPAGE_H_
#include "codepage.h"
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

#define _cxx_
#include "phrase.hdl"

interface IHTMLControlElement;

extern "C" const IID IID_IControl;

MtDefine(CPhraseElement, Elements, "CPhraseElement")
MtDefine(CSpanElement, Elements, "CSpanElement")

const CElement::CLASSDESC CPhraseElement::s_classdesc =
{
    {
        &CLSID_HTMLPhraseElement,           // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        0,                                  // _dwFlags
        &IID_IHTMLPhraseElement,            // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnIHTMLPhraseElement,       //  _apfnTearOff

    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};

HRESULT
CPhraseElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    HRESULT    hr = S_OK;
    CElement * pElement;

    Assert(pht->Is(ETAG_B)        || pht->Is(ETAG_U)      ||
           pht->Is(ETAG_I)        || pht->Is(ETAG_STRONG) ||
           pht->Is(ETAG_BIG)      || pht->Is(ETAG_SMALL)  ||
           pht->Is(ETAG_BLINK)    || pht->Is(ETAG_TT)     ||
           pht->Is(ETAG_STRIKE)   || pht->Is(ETAG_VAR)    ||
           pht->Is(ETAG_SUP)      || pht->Is(ETAG_SUB)    ||
           pht->Is(ETAG_CITE)     || pht->Is(ETAG_CODE)   ||
           pht->Is(ETAG_KBD)      || pht->Is(ETAG_SAMP)   ||
           pht->Is(ETAG_DFN)      || pht->Is(ETAG_S)      ||
           pht->Is(ETAG_EM)       || pht->Is(ETAG_NOBR)   ||
           pht->Is(ETAG_ACRONYM)  || pht->Is(ETAG_Q)      ||
           pht->Is(ETAG_INS)      || pht->Is(ETAG_DEL)    ||
           pht->Is(ETAG_BDO)	  || pht->Is(ETAG_RUBY)   ||
           pht->Is(ETAG_RT)       || pht->Is(ETAG_RP));

    Assert(ppElement);
    pElement = new CPhraseElement(pht->GetTag(), pDoc);
    if (!pElement)
        goto MemoryError;

    *ppElement = pElement;

Cleanup:
    RRETURN(hr);

MemoryError:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

//+------------------------------------------------------------------------
//
//  Member:     CPhraseElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CPhraseElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr;

    *ppv = NULL;

    if IID_TEAROFF(this, IHTMLPhraseElement, NULL)
    else
    {
        RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    (*(IUnknown **)ppv)->AddRef();
    return S_OK;
}


HRESULT
CPhraseElement::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    pCFI->PrepareCharFormat();

    CCharFormat *pCF = &pCFI->_cf();
    LONG twips;

    switch(Tag())
    {
    case ETAG_I:
    case ETAG_CITE:
    case ETAG_DFN:
    case ETAG_EM:
    case ETAG_VAR:
        pCF->_fItalic = TRUE;
        break;

    case ETAG_U:
    case ETAG_INS:
        pCF->_fUnderline = TRUE;
        break;

    case ETAG_SMALL:
    case ETAG_BIG:
        pCF->ChangeHeightRelative( (Tag() == ETAG_BIG) ? 1 : -1 );
        break;

    case ETAG_B:
    case ETAG_STRONG:
        pCF->_fBold = TRUE;
        pCF->_wWeight = 700;
        break;

    case ETAG_S:
    case ETAG_STRIKE:
    case ETAG_DEL:
        pCF->_fStrikeOut = TRUE;
        break;
        

    case ETAG_KBD:
    case ETAG_CODE:
    case ETAG_SAMP:
    case ETAG_TT:
        pCF->_fBumpSizeDown = TRUE;
        {
            CDoc *  pDoc = Doc();
            CODEPAGESETTINGS * pCS = pDoc->_pCodepageSettings;
            CODEPAGE cp = pDoc->GetCodePage();

            // Thai does not have a fixed pitch font. Leave it as proportional
            if(cp != CP_THAI)
            {
                pCF->_bPitchAndFamily = FIXED_PITCH;
                pCF->_latmFaceName = pCS->latmFixedFontFace;
            }
            pCF->_bCharSet = pCS->bCharSet;
            pCF->_fNarrow = IsNarrowCharSet(pCS->bCharSet);
        }
        break;

    case ETAG_SUB:
        pCF->_fSubscript = TRUE;
        pCF->_fSubSuperSized = TRUE;
        break;

    case ETAG_SUP:
        pCF->_fSuperscript = TRUE;
        pCF->_fSubSuperSized = TRUE;
        break;

    case ETAG_NOBR:
        pCFI->_fNoBreak = TRUE;
        break;

    case ETAG_RUBY:
        pCF->_fIsRuby = TRUE;
        break;

    case ETAG_RT:
        if(pCF->_fIsRuby) 
        {
            pCF->_fIsRubyText = TRUE;
            twips = pCF->GetHeightInTwips( Doc() );
            pCF->SetHeightInTwips( twips / 2 );
        }
        break;

    case ETAG_BDO:
        pCFI->_fBidiEmbed = TRUE;
        pCFI->_fBidiOverride = TRUE;
        break;

    case ETAG_RP:
        if(pCF->_fIsRuby) 
        {
            pCF->_fDisplayNone = TRUE;
            if( !pCF->_fIsRubyText )
            {
                pCF->_fIsRubyText = TRUE;
                twips = pCF->GetHeightInTwips( Doc() );
                pCF->SetHeightInTwips( twips / 2 );
            }
        }
        break;
    }
    
    pCFI->UnprepareForDebug();

    RRETURN(super::ApplyDefaultFormat(pCFI));
}

const CElement::CLASSDESC CSpanElement::s_classdesc =
{
    {
        &CLSID_HTMLSpanElement,             // _pclsid
        0,                                  // _idrBase
        s_apclsidPages,                     // _apClsidPages
        s_acpi,                             // _pcpi
        0,                                  // _dwFlags
        &IID_IHTMLSpanElement,              // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnIHTMLSpanElement,         // _apfnTearOff

    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};

HRESULT
CSpanElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(pht->Is(ETAG_SPAN));
    Assert(ppElement);
    *ppElement = new CSpanElement(ETAG_SPAN, pDoc);
    return *ppElement ? S_OK : E_OUTOFMEMORY;
}

#ifndef NO_DATABINDING
#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

const CDBindMethods *
CSpanElement::GetDBindMethods()
{
    return &DBindMethodsTextRichRO;
}


//+----------------------------------------------------------------------------
//
//  Member:     CSpanElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-----------------------------------------------------------------------------

HRESULT
CSpanElement::PrivateQueryInterface ( REFIID iid, void ** ppv )
{
    HRESULT hr;

    *ppv = NULL;

    // IE4 shipped the interface IHTMLControlElement with the same GUID as
    // IControl.  Unfortunately, IControl is a forms^3 interface, which is bad.
    // To resolve this problem Trident's GUID for IHTMLControlElement has
    // changed however, the old GUID remembered in the QI for CSite to return
    // IHTMLControlElement.  The only side affect is that using the old GUID
    // will not marshall the interface correctly only the new GUID has the
    // correct marshalling code.  So, the solution is that QI'ing for
    // IID_IControl or IID_IHTMLControlElement will return IHTMLControlElement.

    // For VB page designer we need to emulate IE4 behavior (fail the QI if not a site)
    if(iid == IID_IControl && Doc()->_fVB && !HasLayout())
        RRETURN(E_NOINTERFACE);

    if (iid == IID_IHTMLControlElement || iid == IID_IControl)
    {

        hr = CreateTearOffThunk(this,
                                s_apfnpdIHTMLControlElement,
                                NULL,
                                ppv,
                                (void *)s_ppropdescsInVtblOrderIHTMLControlElement);
        if (hr)
            RRETURN(hr);
    }
    else
    {
        RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}

#endif
