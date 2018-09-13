//=--------------------------------------------------------------------------=
// CtlPsst.Cpp
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// implementation of persistence interfaces for COleControl.
//
#include "IPServer.H"
#include "CtrlObj.H"

#include "CtlHelp.H"
#include "Util.H"

// this is the name of the stream we'll save our ole controls to.
//
const WCHAR wszCtlSaveStream [] = L"CONTROLSAVESTREAM";

// for ASSERT and FAIL
//
SZTHISFILE

//=--------------------------------------------------------------------------=
// to help with out stream save implementation ...
//
#define STREAMHDR_SIGNATURE 0x12344321  // Signature to identify our format (avoid crashes!)
#define IPROP_END 0xFF                  // Marker at end of property list
#define MAXAUTOBUF 3800                 // Best if < 1 page.

typedef struct tagSTREAMHDR {

    DWORD  dwSignature;     // Signature.
    size_t cbWritten;       // Number of bytes written

} STREAMHDR;

//=--------------------------------------------------------------------------=
// COleControl persistence interfaces
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// COleControl::Load    [IPersistPropertyBag]
//=--------------------------------------------------------------------------=
// IPersistPropertyBag.  we've got a property bag, so let's load our properties
// from it.
//
// Parameters:
//    IPropertyBag *      - [in] pbag from which to read props.
//    IErrorLog *         - [in] error log to write to
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::Load
(
    IPropertyBag *pPropertyBag,
    IErrorLog    *pErrorLog
)
{
    HRESULT hr;

    // load in our standard state first.  nothing serious here ... currently,
    // we've just got two properties, for cx and cy.
    //
    hr = LoadStandardState(pPropertyBag, pErrorLog);
    RETURN_ON_FAILURE(hr);

    // now call the user text load function, and get them to load in whatever
    // they're interested in.
    //
    hr = LoadTextState(pPropertyBag, pErrorLog);

    return hr;
}

//=--------------------------------------------------------------------------=
// COleControl::Save    [IPersistPropertyBag]
//=--------------------------------------------------------------------------=
// given a property bag, save out all the relevant state information.
//
// Parameters:
//    IPropertyBag *        - [in] property to write to
//    BOOL                  - [in] do we clear the dirty bit?
//    BOOL                  - [in] do we write out default values anyhoo?
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::Save
(
    IPropertyBag *pPropertyBag,
    BOOL          fClearDirty,
    BOOL          fWriteDefault
)
{
    HRESULT hr;

    // save out standard state information
    //
    hr = SaveStandardState(pPropertyBag);
    RETURN_ON_FAILURE(hr);

    // now call the user function and get them to save out
    // all of their properties.
    //
    hr = SaveTextState(pPropertyBag, fWriteDefault);
    RETURN_ON_FAILURE(hr);

    // now clear the dirty flag and send out notification that we're
    // done.
    //
    if (fClearDirty)
        m_fDirty = FALSE;

    if (m_pOleAdviseHolder)
        m_pOleAdviseHolder->SendOnSave();

    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::GetClassID    [IPersistStreamInit]
//=--------------------------------------------------------------------------=
// returns the classid of this mamma
//
// Parameters:
//    CLSID *         - [out] where to put the clsid
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::GetClassID
(
    CLSID *pclsid
)
{
    CHECK_POINTER(pclsid);

    // copy the thing over
    //
    *pclsid = CLSIDOFOBJECT(m_ObjectType);
    return S_OK;
}


//=--------------------------------------------------------------------------=
// COleControl::IsDirty    [IPersistStreamInit]
//=--------------------------------------------------------------------------=
// asks if we're dirty or not.  duh.
//
// Output:
//    HRESULT        - S_OK: dirty, S_FALSE: not dirty
//
// Notes:
//
STDMETHODIMP COleControl::IsDirty
(
    void
)
{
    return (m_fDirty) ? S_OK : S_FALSE;
}

//=--------------------------------------------------------------------------=
// COleControl::InitNew    [IPersistStreamInit]
//=--------------------------------------------------------------------------=
// causes the control to intialize itself with a new bunch of state information
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::InitNew
(
    void
)
{
    BOOL f;

    // call the overridable function to do this work
    //
    f = InitializeNewState();
    return (f) ? S_OK : E_FAIL;
}

//=--------------------------------------------------------------------------=
// COleControl::GetSizeMax    [IPersistStreamInit]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    ULARGE_INTEGER *    - [out]
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::GetSizeMax
(
    ULARGE_INTEGER *pulMaxSize
)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// COleControl::Load    [IPersistStreamInit]
//=--------------------------------------------------------------------------=
// load from an IStream
//
// Parameters:
//    IStream *    - [in] stream from which to load
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::Load
(
    IStream *pStream
)
{
    HRESULT hr;

    // first thing to do is read in standard properties the user don't
    // persist themselves.
    //
    hr = LoadStandardState(pStream);
    RETURN_ON_FAILURE(hr);

    // load in the user properties.  this method is one they -have- to implement
    // themselves.
    //
    hr = LoadBinaryState(pStream);
    
    return hr;
}

//=--------------------------------------------------------------------------=
// COleControl::Save    [IPersistStreamInit]
//=--------------------------------------------------------------------------=
// saves out our state using streams
//
// Parameters:
//    IStream *        - [in]
//    BOOL             - [in] clear dirty bit?
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::Save
(
    IStream *pStream,
    BOOL     fClearDirty
)
{
    HRESULT hr;

    // use our helper routine that we share with the IStorage persistence
    // code.
    //
    hr = m_SaveToStream(pStream);
    RETURN_ON_FAILURE(hr);

    // clear out dirty flag [if appropriate] and notify that we're done
    // with save.
    //
    if (fClearDirty)
        m_fDirty = FALSE;
    if (m_pOleAdviseHolder)
        m_pOleAdviseHolder->SendOnSave();

    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::InitNew    [IPersistStorage]
//=--------------------------------------------------------------------------=
// ipersiststorage version of this.  fweee
//
// Parameters:
//    IStorage *    - [in] we don't use this
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::InitNew
(
    IStorage *pStorage
)
{
    // we already have an implementation of this [for IPersistStreamInit]
    //
    return InitNew();
}

//=--------------------------------------------------------------------------=
// COleControl::Load    [IPersistStorage]
//=--------------------------------------------------------------------------=
// Ipersiststorage version of this
//
// Parameters:
//    IStorage *    - [in] DUH.
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::Load(IStorage *pStorage)
{
    IStream *pStream;
    HRESULT  hr;

    // we're going to use IPersistStream::Load from the CONTENTS stream.
    //
    hr = pStorage->OpenStream(wszCtlSaveStream, 0, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &pStream);
    RETURN_ON_FAILURE(hr);

    // IPersistStreamInit::Load
    //
    hr = Load(pStream);
    pStream->Release();
    return hr;
}

//=--------------------------------------------------------------------------=
// COleControl::Save    [IPersistStorage]
//=--------------------------------------------------------------------------=
// save into the contents stream of the given storage object.
//
// Parameters:
//    IStorage *        - [in] 10 points if you figure it out
//    BOOL              - [in] is the storage the same as the load storage?
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::Save
(
    IStorage *pStorage,
    BOOL      fSameAsLoad
)
{
    IStream *pStream;
    HRESULT  hr;

    // we're just going to save out to the CONTENTES stream.
    //
    hr = pStorage->CreateStream(wszCtlSaveStream, STGM_WRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
                                0, 0, &pStream);
    RETURN_ON_FAILURE(hr);

    // use our helper routine.
    //
    hr = m_SaveToStream(pStream);
    m_fSaveSucceeded = (FAILED(hr)) ? FALSE : TRUE;
    pStream->Release();
    return hr;
}

//=--------------------------------------------------------------------------=
// COleControl::SaveCompleted    [IPersistStorage]
//=--------------------------------------------------------------------------=
// lets us clear out our flags.
//
// Parameters:
//    IStorage *    - ignored
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::SaveCompleted
(
    IStorage *pStorageNew
)
{
    // if our save succeeded, then we can do our post save work.
    //
    if (m_fSaveSucceeded) {
        m_fDirty = FALSE;
        if (m_pOleAdviseHolder)
            m_pOleAdviseHolder->SendOnSave();
    }

    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::HandsOffStorage    [IPersistStorage]
//=--------------------------------------------------------------------------=
// not interesting
//
// Output:
//    S_OK
//
// Notes:
//
STDMETHODIMP COleControl::HandsOffStorage
(
    void
)
{
    // we don't ever hold on to  a storage pointer, so this is remarkably
    // uninteresting to us.
    //
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::m_SaveToStream    [helper: IPersistStreamInit/IPersistStorage]
//=--------------------------------------------------------------------------=
// save ourselves to a stream
//
// Parameters:
//    IStream *        - figure it out
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT COleControl::m_SaveToStream
(
    IStream *pStream
)
{
    HRESULT hr;

    // save out standard state information that the user has no control
    // over
    //
    hr = SaveStandardState(pStream);
    RETURN_ON_FAILURE(hr);

    // save out user-specific satte information.  they MUST implement this
    // function
    //
    hr = SaveBinaryState(pStream);

    return hr;
}

//=--------------------------------------------------------------------------=
// COleControl::LoadStandardState    [ helper ]
//=--------------------------------------------------------------------------=
// reads in standard properties that all controls are going to have, using
// text persistence APIs.  there is another version for streams.
//
// Parameters:
//    IPropertyBag *    - [in]
//    IErrorLog *       - [in]
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT COleControl::LoadStandardState
(
    IPropertyBag *pPropertyBag,
    IErrorLog    *pErrorLog
)
{
    VARIANT v;
    HRESULT hr;
    SIZEL   slHiMetric = { 100, 50 };

    // currently, our only standard properties are related to size.
    // if we can't find them, then we'll just use some defaults.
    //
    v.vt = VT_I4;
    v.lVal = 0;
    hr = pPropertyBag->Read(L"_ExtentX", &v, pErrorLog);
    if (SUCCEEDED(hr)) slHiMetric.cx = v.lVal;

    v.lVal = 0;
    hr = pPropertyBag->Read(L"_ExtentY", &v, pErrorLog);
    if (SUCCEEDED(hr)) slHiMetric.cy = v.lVal;

    HiMetricToPixel(&slHiMetric, &m_Size);
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::LoadStandardState    [ helper ]
//=--------------------------------------------------------------------------=
// reads in standard properties that all controls are going to have, using
// stream persistence APIs.  there is another version for text.
//
// Parameters:
//    IStream *         - [in] 
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT COleControl::LoadStandardState
(
    IStream *pStream
)
{
    STREAMHDR stmhdr;
    HRESULT hr;
    SIZEL   slHiMetric;

    // look for our header structure, so we can verify stream validity.
    //
    hr = pStream->Read(&stmhdr, sizeof(STREAMHDR), NULL);
    RETURN_ON_FAILURE(hr);

    if (stmhdr.dwSignature != STREAMHDR_SIGNATURE)
        return E_UNEXPECTED;

    // currently, the only standard state we're writing out is
    // a SIZEL structure describing the control's size.
    //
    if (stmhdr.cbWritten != sizeof(m_Size))
        return E_UNEXPECTED;

    // we like the stream.  let's go load in our two properties.
    //
    hr = pStream->Read(&slHiMetric, sizeof(slHiMetric), NULL);
    RETURN_ON_FAILURE(hr);

    HiMetricToPixel(&slHiMetric, &m_Size);
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::SaveStandardState    [ helper ]
//=--------------------------------------------------------------------------=
// saves out standard properties that we're managing for a control using text
// persistence APIs.  there is another version for stream persistence.
//
// Parameters:
//    IPropertyBag *        - [in]
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT COleControl::SaveStandardState
(
    IPropertyBag *pPropertyBag
)
{
    HRESULT hr;
    VARIANT v;
    SIZEL   slHiMetric;

    // currently, the only standard proprerties we persist are Size related
    //
    PixelToHiMetric(&m_Size, &slHiMetric);

    v.vt = VT_I4;
    v.lVal = slHiMetric.cx;

    hr = pPropertyBag->Write(L"_ExtentX", &v);
    RETURN_ON_FAILURE(hr);

    v.lVal = slHiMetric.cy;

    hr = pPropertyBag->Write(L"_ExtentY", &v);

    return hr;
}

//=--------------------------------------------------------------------------=
// COleControl::SaveStandardState    [ helper ]
//=--------------------------------------------------------------------------=
// saves out standard properties that we're managing for a control using stream
// persistence APIs.  there is another version for text persistence.
//
// Parameters:
//    IStream *            - [in]
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT COleControl::SaveStandardState
(
    IStream *pStream
)
{
    STREAMHDR streamhdr = { STREAMHDR_SIGNATURE, sizeof(SIZEL) };
    HRESULT hr;
    SIZEL   slHiMetric;


    // first thing to do is write out our stream hdr structure.
    //
    hr = pStream->Write(&streamhdr, sizeof(STREAMHDR), NULL);
    RETURN_ON_FAILURE(hr);

    // the only properties we're currently persisting here are the size
    // properties for this control.  make sure we do that in HiMetric
    //
    PixelToHiMetric(&m_Size, &slHiMetric);

    hr = pStream->Write(&slHiMetric, sizeof(slHiMetric), NULL);
    return hr;
}

//=--------------------------------------------------------------------------=
// COleControl::InitializeNewState    [overridable]
//=--------------------------------------------------------------------------=
// the user can override this to initialize variables
//
// Output:
//    BOOL        - FALSE means couldn't do it.
//
// Notes:
//
BOOL COleControl::InitializeNewState
(
    void
)
{
    // we find this largely uninteresting
    //
    return TRUE;
}



