//-----------------------------------------------------------------------------
//
// Microsoft Forms
// Copyright: (c) 1994-1995, Microsoft Corporation
// All rights Reserved.
// Information contained herein is Proprietary and Confidential.
//
// File         DATADOC.CXX
//
// Contents     Dataodc-specific functionality of the Form
//
// Classes      CDoc
//
//
//-------------------------------------------------------------------------

#include "headers.hxx"
#include "dfrm.hxx"

#include "loadtxt.hxx"

#ifdef PRODUCT_97

STDAPI
DDocLoadFromTextFile(IForm * pForm, LPTSTR pstrFile)
{
    HRESULT     hr;
    CDoc *      pDoc;

    Verify(!pForm->QueryInterface(CLSID_CHtmlForm, (void **) &pDoc));

    hr = pDoc->LoadDataFrameFromTextFile(pstrFile);

    RRETURN(hr);
}


STDMETHODIMP
CDoc::LoadDataFrameFromTextFile(LPTSTR pstrFile)
{
    HRESULT     hr;
    CLoadText * pLoadText;

    pLoadText = new CLoadText;
    if (!pLoadText)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pLoadText->Init(&_RootSite));
    if (hr)
        goto Cleanup;

    // BUGBUG: we probably want to do this earlier somewhere,
    //  and fail the Close operation on error
    // IGNORE_HR(CommitData());

    hr = THR(_RootSite._Controls.Clear());
    if (hr)
        goto Cleanup;

    hr = THR(pLoadText->FileLoad(pstrFile));
    if (hr)
        goto Cleanup;

    // GenerateInstances();

Cleanup:
    _RootSite.Invalidate(NULL, 0);
    delete pLoadText;

    RRETURN(hr);
}

#endif // PRODUCT_9


//
//  ****    End of file
//
///////////////////////////////////////////////////////////////////////////////
