//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1993 - 1995.
//
//  File:       docdata.cxx
//
//  Implement Data transfer support for Doc object
//
//------------------------------------------------------------------------


#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_RTFTOHTM_HXX_
#define X_RTFTOHTM_HXX_
#include "rtftohtm.hxx"
#endif

#ifndef X_CODEPAGE_H_
#define X_CODEPAGE_H_
#include "codepage.h"
#endif

//+---------------------------------------------------------------
//
//  Member:     CDoc::GetPlaintext
//
//  Synopsis:   Helper function to get plaintext in a specified codepage.
//
//---------------------------------------------------------------

HRESULT
CDoc::GetPlaintext(
        CServer * pServer,
        LPFORMATETC pformatetc,
        LPSTGMEDIUM pmedium,
        BOOL fHere,
        CODEPAGE codepage)
{
    HRESULT     hr;

    if (!fHere)
    {
        // fill in the pmedium structure
        pmedium->tymed = TYMED_ISTREAM;
        pmedium->pUnkForRelease = NULL;

        hr = THR(CreateStreamOnHGlobal(NULL, TRUE, &pmedium->pstm));
        if (hr)
            goto Cleanup;
    }

    //
    // Save to the stream in plaintext mode, but do not word wrap (since we do
    //  not word wrap on the clipboard either).
    //
    hr = THR(DYNCAST(CDoc, pServer)->SaveToStream(pmedium->pstm,
        WBF_SAVE_PLAINTEXT|WBF_FORMATTED_PLAINTEXT|WBF_NUMBER_LISTS|
        WBF_FORMATTED|WBF_NO_WRAP,
        codepage));

Cleanup:
    //  If we failed somehow and yet created a docfile, then we will
    //      release the docfile to delete it

    if (hr && !fHere)
        ClearInterface(&pmedium->pstm);

    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::GetTEXT
//
//  Synopsis:   Get CF_TEXT format for the CDoc object
//
//---------------------------------------------------------------

HRESULT
CDoc::GetTEXT(
        CServer * pServer,
        LPFORMATETC pformatetc,
        LPSTGMEDIUM pmedium,
        BOOL fHere)
{
    RRETURN(GetPlaintext(pServer, pformatetc, pmedium, fHere, g_cpDefault));
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::GetUNICODETEXT
//
//  Synopsis:   Get CF_UNICODETEXT format for the CDoc object
//
//---------------------------------------------------------------

HRESULT
CDoc::GetUNICODETEXT(
        CServer * pServer,
        LPFORMATETC pformatetc,
        LPSTGMEDIUM pmedium,
        BOOL fHere)
{
    RRETURN(GetPlaintext(pServer, pformatetc, pmedium, fHere, CP_UCS_2));
}

#ifndef NO_RTF
//+---------------------------------------------------------------
//
//  Member:     CDoc::GetRTF
//
//  Synopsis:   Get CF_RTF format for the CDoc object
//
//---------------------------------------------------------------

HRESULT
CDoc::GetRTF(
        CServer * pServer,
        LPFORMATETC pformatetc,
        LPSTGMEDIUM pmedium,
        BOOL fHere)
{
    HRESULT                 hr = S_OK;
#ifndef WINCE
    CRtfToHtmlConverter *   pcnv = NULL;
    CDoc *                  pDoc = DYNCAST(CDoc, pServer);

    if (!pDoc->RtfConverterEnabled())
        return E_FAIL;

    if (!fHere)
    {
        // fill in the pmedium structure
        pmedium->tymed = TYMED_ISTREAM;
        pmedium->pUnkForRelease = NULL;

        hr = THR(CreateStreamOnHGlobal(NULL, TRUE, &pmedium->pstm));
        if (hr)
        {
            goto Cleanup;
        }
    }

    pcnv = new CRtfToHtmlConverter(pDoc);
    if (pcnv == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pcnv->InternalHtmlToStreamRtf(pmedium->pstm));
    if (hr)
    {
        goto Cleanup;
    }

Cleanup:
    //  If we failed somehow and yet created a docfile, then we will
    //      release the docfile to delete it

    if (hr && !fHere)
        ClearInterface(&pmedium->pstm);

    delete pcnv;

#endif // WINCE
    RRETURN1(hr, S_FALSE);
}
#endif
