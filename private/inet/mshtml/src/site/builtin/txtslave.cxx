//+---------------------------------------------------------------------
//
//  File:       txtslave.cxx
//
//  Contents:   CTxtSlave
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_SIZE_HXX_
#define X_SIZE_HXX_
#include "size.hxx"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_CGUID_H_
#define X_CGUID_H_
#include <cguid.h>
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include <treepos.hxx>
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_TXTSLAVE_HXX_
#define X_TXTSLAVE_HXX_
#include "txtslave.hxx"
#endif

MtDefine(CTxtSlave, Elements, "CTxtSlave")

const CElement::CLASSDESC CTxtSlave::s_classdesc =
{
    {
        NULL,                           // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_TEXTSITE |          // _dwFlags
        ELEMENTDESC_DONTINHERITSTYLE |
        ELEMENTDESC_SHOWTWS |
        ELEMENTDESC_VPADDING |
        ELEMENTDESC_HASDEFDESCENT,
        NULL,                           // _piidDispinterface
        NULL,                           // _apHdlDesc
    },
    NULL,                               // _pfnTearOff
    NULL,                               // _pAccelsDesign
    NULL                                // _pAccelsRun
};


HRESULT
CTxtSlave::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(ppElement);

    Assert(pht->GetTag() == ETAG_TXTSLAVE);
    *ppElement = new CTxtSlave(pht->GetTag(), pDoc);

    RRETURN ( (*ppElement) ? S_OK : E_OUTOFMEMORY);
}

HRESULT
CTxtSlave::Init2(CInit2Context * pContext)
{
    // CAUTION: Need to keep track of changes in Init2 of the
    // base classes and port them as appropriate.
    return S_OK;
}

DWORD
CTxtSlave::GetBorderInfo(  CDocInfo * pdci,
                                    CBorderInfo *pborderinfo,
                                    BOOL fAll)
{
    return MarkupMaster()->GetBorderInfo(pdci, pborderinfo, fAll);
}

HRESULT
CTxtSlave::ComputeFormats(CFormatInfo * pCFI, CTreeNode * pNodeTarget)
{
    SwitchesBegTimer(SWITCHES_TIMER_COMPUTEFORMATS);

    HRESULT       hr = S_OK;
    CTreeNode   * pNodeMaster = NULL;
    CElement    * pElemMaster = MarkupMaster();
    CDoc        * pDoc = Doc();
    THREADSTATE * pts  = GetThreadState();
    BOOL          fComputeFFOnly = pNodeTarget->_iCF != -1;
    COMPUTEFORMATSTYPE  eExtraValues = pCFI->_eExtraValues;

    Assert(pCFI);
    Assert(SameScope(this, pNodeTarget));
    Assert(eExtraValues != ComputeFormatsType_Normal || ((pNodeTarget->_iCF == -1 && pNodeTarget->_iPF == -1) || pNodeTarget->_iFF == -1));
    AssertSz(!TLS(fInInitAttrBag), "Trying to compute formats during InitAttrBag! This is bogus and must be corrected!");

    //TraceTag((tagRecalcStyle, "ComputeFormats"));

    if (pElemMaster)
    {
        pNodeMaster = pElemMaster->GetFirstBranch();

        //
        // Get the format of our master before applying our own format.
        //
        if (pNodeMaster)
        {

            //
            // If the master node has not computed formats yet, recursively compute them
            //

            if (    pNodeMaster->_iCF == -1
                ||  pNodeMaster->_iFF == -1
                ||  eExtraValues == ComputeFormatsType_GetInheritedValue )
            {
                SwitchesEndTimer(SWITCHES_TIMER_COMPUTEFORMATS);

                hr = THR(pElemMaster->ComputeFormats(pCFI, pNodeMaster));

                SwitchesBegTimer(SWITCHES_TIMER_COMPUTEFORMATS);

                if (hr)
                    goto Cleanup;
            }

            Assert(pNodeMaster->_iCF >= 0);
            Assert(pNodeMaster->_iPF >= 0);
            Assert(pNodeMaster->_iFF >= 0);
        }
    }

    //
    // NOTE: From this point forward any errors must goto Error instead of Cleanup!
    //

    pCFI->Reset();
    pCFI->_pNodeContext = pNodeTarget;

    if (pNodeMaster)
    {
        //
        // Inherit para format directly from the master node.
        pCFI->_iffSrc = pNodeMaster->_iFF;
        pCFI->_pffSrc = pCFI->_pff = &(*pts->_pFancyFormatCache)[pCFI->_iffSrc];
        pCFI->_fHasExpandos = (pCFI->_pff->_iExpandos >= 0);

        if (!fComputeFFOnly)
        {
            // Inherit the Char and Para formats from the master node
            pCFI->_icfSrc = pNodeMaster->_iCF;
            pCFI->_pcfSrc = pCFI->_pcf = &(*pts->_pCharFormatCache)[pCFI->_icfSrc];
            pCFI->_ipfSrc = pNodeMaster->_iPF;
            pCFI->_ppfSrc = pCFI->_ppf = &(*pts->_pParaFormatCache)[pCFI->_ipfSrc];

            // If the parent had layoutness, clear the inner formats

            if (pCFI->_pcf->_fHasDirtyInnerFormats)
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf().ClearInnerFormats();
                pCFI->UnprepareForDebug();
            }
            if (pCFI->_ppf->_fHasDirtyInnerFormats)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf().ClearInnerFormats();
                pCFI->UnprepareForDebug();
            }
            if (    pCFI->_ppf->_fPre != pCFI->_ppf->_fPreInner
                ||  pCFI->_ppf->_fInclEOLWhite != pCFI->_ppf->_fInclEOLWhiteInner
                ||  pCFI->_ppf->_bBlockAlign != pCFI->_ppf->_bBlockAlignInner)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fPre = pCFI->_pf()._fPreInner;
                pCFI->_pf()._fInclEOLWhite = pCFI->_pf()._fInclEOLWhiteInner;
                pCFI->_pf()._bBlockAlign = pCFI->_pf()._bBlockAlignInner;
                pCFI->UnprepareForDebug();
            }
            
            if (pCFI->_pcf->_fNoBreak != pCFI->_pcf->_fNoBreakInner)
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._fNoBreak = pCFI->_cf()._fNoBreakInner;
                pCFI->UnprepareForDebug();
            }

        }
        else
        {
            pCFI->_icfSrc = pDoc->_icfDefault;
            pCFI->_pcfSrc = pCFI->_pcf = pDoc->_pcfDefault;
            pCFI->_ipfSrc = pts->_ipfDefault;
            pCFI->_ppfSrc = pCFI->_ppf = pts->_ppfDefault;
        }
    }
    else
    {
        pCFI->_iffSrc = pts->_iffDefault;
        pCFI->_pffSrc = pCFI->_pff = pts->_pffDefault;
        pCFI->_icfSrc = pDoc->_icfDefault;
        pCFI->_pcfSrc = pCFI->_pcf = pDoc->_pcfDefault;
        pCFI->_ipfSrc = pts->_ipfDefault;
        pCFI->_ppfSrc = pCFI->_ppf = pts->_ppfDefault;

        Assert(pCFI->_pffSrc->_pszFilters == NULL);
    }

    if (    pCFI->_pff->_fHasLayout
        ||  !pCFI->_pff->_fBlockNess)
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._fHasLayout = FALSE;
        pCFI->_ff()._fBlockNess = TRUE;
        pCFI->UnprepareForDebug();
    }

    if(eExtraValues == ComputeFormatsType_Normal)
    {
        hr = THR(pNodeTarget->CacheNewFormats(pCFI));
        if (hr)
            goto Error;

        // Cache whether an element is a block element or not for fast retrieval.
        pNodeTarget->_fBlockNess = TRUE;

        pNodeTarget->_fHasLayout = FALSE;
    }

Cleanup:
    SwitchesEndTimer(SWITCHES_TIMER_COMPUTEFORMATS);
    RRETURN(hr);

Error:
    pCFI->Cleanup();
    goto Cleanup;
}

