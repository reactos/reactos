//---------------------------------------------------------------------
//
//   File:      xbag.cxx
//
//  Contents:   Xfer objects for selection and generic use
//
//  Classes:    CDropSource, CDummyDropSource, CBaseBag, CGenDataObject,
//              CEnumFormatEtc:
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_XBAG_HXX_
#define X_XBAG_HXX_
#include "xbag.hxx"
#endif

#ifndef X_DRAGDROP_HXX_
#define X_DRAGDROP_HXX_
#include "dragdrop.hxx"
#endif

#ifndef X_SHLOBJ_H_
#define X_SHLOBJ_H_
#include <shlobj.h>
#endif

#ifndef X__DXFROBJ_H_
#define X__DXFROBJ_H_
#include "_dxfrobj.h"
#endif

DeclareTagOther(tagIgnorePrivateCF, "FormKrnl", "Disable private CF's")

MtDefine(CDummyDropSource, ObjectModel, "CDummyDropSource")
MtDefine(CGenDataObject, ObjectModel, "CGenDataObject")
MtDefine(CGenDataObject_rgfmtc_pv, CGenDataObject, "CGenDataObject::_rgfmtc::_pv")
MtDefine(CGenDataObject_rgstgmed_pv, CGenDataObject, "CGenDataObject::_rgstgmed::_pv")
MtDefine(CEnumFormatEtc, ObjectModel, "CEnumFormatEtc")
MtDefine(CEnumFormatEtc_prgFormats, ObjectModel, "CEnumFormatEtc::_prgFormats")

//  NOTE that IFMTETC_PRIVATEFMT should be the first of our formats.
//    Other code in this file relies on this

#define IFMTETC_PRIVATEFMT  2
#define IFMTETC_CLSIDFMT    2
#define IFMTETC_PRIVTEXTFMT 3

// List of formats offered by our data transfer object via EnumFormatEtc
// NOTE: g_acfOleClipFormat is a global array of stock formats defined in
//       cdutil\dvutils.cxx and initialized in the Form class factory

// NOTE that the first two formats have the index to their clipformat
// rather than the actual clip format.  This value will be replaced in
// the call to InitFormClipFormats

static FORMATETC g_aGetFmtEtcs[] =
{
    { CF_COMMON(ICF_EMBEDSOURCE), NULL, DVASPECT_CONTENT, -1, TYMED_ISTORAGE },
    { CF_COMMON(ICF_OBJECTDESCRIPTOR), NULL, DVASPECT_CONTENT, -1L, TYMED_HGLOBAL },
    { CF_COMMON(ICF_FORMSCLSID), NULL, DVASPECT_CONTENT, -1L, TYMED_HGLOBAL},     //  IFMTETC_CLSIDFMT
    { CF_COMMON(ICF_FORMSTEXT), NULL, DVASPECT_CONTENT, -1L, TYMED_HGLOBAL},      //  IFMTETC_PRIVTEXTFMT
};


//+---------------------------------------------------------------------------
//
//  Function:   InitFormClipFormats
//
//  Synopsis:   Set registered clip formats into g_aGetFmtEtcs.
//
//----------------------------------------------------------------------------

void
InitFormClipFormats( )
{
    SetCommonClipFormats(g_aGetFmtEtcs, ARRAY_SIZE(g_aGetFmtEtcs));
}


//+-------------------------------------------------------------------------
//
//  Function:   GetcfCLSIDFmt
//
//  Synopsis:   Attempt to retrieve our private CLSID clipboard format
//              information from the given data object.  If it is not
//              available this function returns an error.
//
//--------------------------------------------------------------------------

HRESULT
GetcfCLSIDFmt(LPDATAOBJECT pDataObj, TCHAR * tszClsid)
{
    HRESULT     hr;
    STGMEDIUM   stgmedium;
    TCHAR *     pszText;

    stgmedium.tymed = TYMED_HGLOBAL;
    stgmedium.hGlobal = NULL;
    stgmedium.pUnkForRelease = NULL;

    hr = pDataObj->GetData(&g_aGetFmtEtcs[IFMTETC_CLSIDFMT], &stgmedium);
    if (hr)
        goto Cleanup;

    if (TYMED_NULL == stgmedium.tymed)
    {
        hr = DV_E_FORMATETC;
        goto Cleanup;
    }

    // STGFIX: t-gpease 8-13-97
    Assert(stgmedium.tymed == TYMED_HGLOBAL);

    pszText = (TCHAR *) GlobalLock(stgmedium.hGlobal);
    Assert(pszText != NULL);

    memcpy(tszClsid, pszText, CLSID_STRLEN*sizeof(TCHAR));
    tszClsid[CLSID_STRLEN] = 0;
    GlobalUnlock(stgmedium.hGlobal);
    GlobalFree(stgmedium.hGlobal);

Cleanup:
    return hr;
}


//+------------------------------------------------------------------------
//
//  Function:   FindLegalCF
//
//  Synopsis:   Returns S_OK if the given data object contains a format
//              we can parse, or S_FALSE if it doesn't.  If the DO
//              returns an unexpected error, we return that instead.
//
//  Arguments:  [pDO]
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
FindLegalCF(IDataObject * pDO)
{
    HRESULT     hr;
    int         c;
    FORMATETC * pformatetc;

#if defined(_MAC)
    if (!pDO)
        return S_OK;
#endif

    Assert(pDO);

    for (c = ARRAY_SIZE(g_aGetFmtEtcs), pformatetc = g_aGetFmtEtcs;
         c > 0;
         c--, pformatetc++)
    {
#if DBG == 1
        if (IsTagEnabled(tagIgnorePrivateCF))
        {
            if (c <= ARRAY_SIZE(g_aGetFmtEtcs) - IFMTETC_PRIVATEFMT)
                continue;
        }
#endif

        hr = pDO->QueryGetData(pformatetc);
        switch (hr)
        {
        case DV_E_FORMATETC:
        case DV_E_TYMED:
        case DV_E_DVASPECT:
            break;

        case DV_E_CLIPFORMAT:   //  This isn't a specified return value,
                                //    but the standard handler seems to
                                //    return it anyhow
            break;

        default:
            RRETURN(hr);
        }
    }

    return S_FALSE;
}


//
// CDropSource
// Implements IDropSource interface.

STDMETHODIMP
CDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    HRESULT hr = S_OK;

   if (_pDoc->_pDragStartInfo && !_pDoc->_pDragStartInfo->_pElementDrag->Fire_ondrag())
   {
        hr = DRAGDROP_S_CANCEL;
        goto Cleanup;
   }

   if (fEscapePressed)
    {
        hr = DRAGDROP_S_CANCEL;
        goto Cleanup;
    }

    // initialize ourself with the drag begin button
    if (_dwButton == 0)
        _dwButton = (grfKeyState & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON));

    // Assert(_dwButton); no need for this assert

    if (!(grfKeyState & _dwButton))
    {
        //
        // A button is released.
        //
        hr = DRAGDROP_S_DROP;
    }
    else if (_dwButton != (grfKeyState & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
    {
        //
        //  If the button state is changed (except the drop case, which we handle
        // above, cancel the drag&drop.
        //
        hr = DRAGDROP_S_CANCEL;
    }

Cleanup:
    if (hr != S_OK)
    {
        // reset cursor here ?
        // SetCursor(LoadCursor(NULL, IDC_ARROW));
    }

    return hr;
}


STDMETHODIMP
CDropSource::GiveFeedback(DWORD dwEffect)
{
    // let OLE put up the right cursor
    return DRAGDROP_S_USEDEFAULTCURSORS;
}

//
// CDummyDropSource
// Wraps a trivial instantiable class around CDropSource

STDMETHODIMP
CDummyDropSource::QueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IUnknown || riid == IID_IDropSource)
    {
        *ppv = this;
        ((IUnknown *)*ppv)->AddRef();
        return NOERROR;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


HRESULT
CDummyDropSource::Create(DWORD dwKeyState, CDoc * pDoc, IDropSource ** ppDropSrc)
{
    HRESULT         hr;

    *ppDropSrc = new CDummyDropSource;
    if (!*ppDropSrc)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    ((CDummyDropSource *)(*ppDropSrc))->_dwButton =
        dwKeyState & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON);
    ((CDummyDropSource *)(*ppDropSrc))->_pDoc = pDoc;
    hr = S_OK;
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CBaseBag::QueryInterface, public
//
//  Synopsis:   Expose our IFaces
//
//---------------------------------------------------------------

STDMETHODIMP
CBaseBag::QueryInterface(REFIID riid, void ** ppv)
{
    if (IsEqualIID(riid,IID_IUnknown))
    {
        *ppv = (IUnknown *)(IDataObject *)this;
    }
    else if (IsEqualIID(riid,IID_IDataObject))
    {
        *ppv = (IDataObject *)this;
    }
    else if (IsEqualIID(riid,IID_IDropSource))
    {
        *ppv = (IDropSource *)this;
    }
    else if (IsEqualIID(riid,IID_IOleCommandTarget))
    {
        *ppv = (IOleCommandTarget *)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    ((IUnknown FAR*) *ppv)->AddRef();
    return NOERROR;
}

//+----------------------------------------------------------------------------
//
// CBaseBag IOleCommandTarget support
//
//-----------------------------------------------------------------------------
HRESULT
CBaseBag::QueryStatus(
            const GUID * pguidCmdGroup,
            ULONG        cCmds,
            OLECMD       rgCmds[],
            OLECMDTEXT * pcmdtext)
{
    RRETURN(OLECMDERR_E_UNKNOWNGROUP);
}

HRESULT
CBaseBag::Exec(
            const GUID * pguidCmdGroup,
            DWORD        nCmdID,
            DWORD        nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut)
{
    HRESULT  hr = OLECMDERR_E_UNKNOWNGROUP;

    if (* pguidCmdGroup == CGID_DATAOBJECTEXEC)
    {
        hr = OLECMDERR_E_NOTSUPPORTED;

        switch (nCmdID)
        {
        case IDM_SETSECURITYDOMAIN:
            if (pvarargIn && (V_VT(pvarargIn) == VT_BSTR))
            {
                Assert(FormsStringLen(V_BSTR(pvarargIn)) == MAX_SIZE_SECURITY_ID);
                memcpy(_abSID, V_BSTR(pvarargIn), MAX_SIZE_SECURITY_ID);
                hr = S_OK;
            }
            break;

        case IDM_CHECKSECURITYDOMAIN:
            if (pvarargIn && (V_VT(pvarargIn) == VT_BSTR))
            {
                Assert(FormsStringLen(V_BSTR(pvarargIn)) == MAX_SIZE_SECURITY_ID);
                hr = !memcmp(_abSID, V_BSTR(pvarargIn), MAX_SIZE_SECURITY_ID) ?
                        S_OK : OLECMDERR_E_DISABLED;
            }
            break;
        }
    }
    RRETURN (hr);
}

/*
 * CGenDataObject - Generic IDataObject implementation.
 */


/********************************** Methods **********************************/


CGenDataObject::CGenDataObject(CDoc * pDoc)
    : _rgfmtc(Mt(CGenDataObject_rgfmtc_pv)),
      _rgstgmed(Mt(CGenDataObject_rgstgmed_pv))
{
    _ulRefs = 1;

    _pDoc = pDoc;
    _pLinkDataObj = NULL;
    _fLinkFormatAppended = FALSE;

    _dwPreferredEffect              = DROPEFFECT_NONE;

#ifndef WIN16
    _fmtcPreferredEffect.cfFormat   = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
#endif //!WIN16
    _fmtcPreferredEffect.ptd        = NULL;
    _fmtcPreferredEffect.dwAspect   = DVASPECT_CONTENT;
    _fmtcPreferredEffect.lindex     = -1;
    _fmtcPreferredEffect.tymed      = TYMED_HGLOBAL;
}


CGenDataObject::~CGenDataObject()
{
    int         i;
    STGMEDIUM * pstgmed;

    for (i = _rgstgmed.Size(), pstgmed = _rgstgmed; i > 0; i--, pstgmed++)
        ReleaseStgMedium(pstgmed);
    
    TLS(pDataClip) = NULL;
}


HRESULT
CGenDataObject::DeleteFormatData(CLIPFORMAT cfFormat)
{
    FORMATETC * pfmtc;
    int i;

    for (i = _rgfmtc.Size() - 1, pfmtc = _rgfmtc + i; i >= 0; --i, --pfmtc)
    {
        if (pfmtc->cfFormat == cfFormat)
        {
            _rgfmtc.Delete(i);
            _rgstgmed.Delete(i);
        }
    }

    return S_OK;
}

HRESULT
CGenDataObject::AppendFormatData(CLIPFORMAT cfFormat, HGLOBAL hGlobal)
{
    HRESULT hr;
    FORMATETC fmtc;
    STGMEDIUM stgmed;

    fmtc.cfFormat = cfFormat;
    fmtc.ptd = NULL;
    fmtc.dwAspect = DVASPECT_CONTENT;
    fmtc.lindex = -1;
    fmtc.tymed = TYMED_HGLOBAL;

    stgmed.tymed = TYMED_HGLOBAL;
    stgmed.hGlobal = hGlobal;
    stgmed.pUnkForRelease = NULL;

    hr = _rgfmtc.AppendIndirect(&fmtc);
    if (!hr)
    {
        hr = _rgstgmed.AppendIndirect(&stgmed);
        if (hr)
            _rgfmtc.DeleteByValueIndirect(&fmtc);
    }

    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE
CGenDataObject::GetData(LPFORMATETC pfmtetc, LPSTGMEDIUM pstgmed)
{
    HRESULT     hr = S_OK;
    HGLOBAL hGlobal;
    DWORD *     pdw;
    int         i, c;
    FORMATETC * pfmtc;

    memset(pstgmed, 0, sizeof(*pstgmed));

    if (_dwPreferredEffect != DROPEFFECT_NONE &&
        FORMATETCMatchesRequest(pfmtetc, &_fmtcPreferredEffect))
    {
        hGlobal = GlobalAlloc(GPTR, sizeof(DWORD));
        if (!hGlobal)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        IF_WIN16(pdw = (DWORD *)GlobalLock(hGlobal));
        IF_WIN32(pdw = (DWORD *)hGlobal);
        *pdw = _dwPreferredEffect;
        IF_WIN16(GlobalUnlock(hGlobal));

        pstgmed->tymed          = TYMED_HGLOBAL;
        pstgmed->hGlobal        = hGlobal;
        pstgmed->pUnkForRelease = NULL;
        goto Cleanup;
    }

    c = _rgfmtc.Size();
    for (i = 0, pfmtc = _rgfmtc; i < c; i++, pfmtc++)
    {
        if (FORMATETCMatchesRequest(pfmtetc, pfmtc))
            break;
    }
    if (i == c)
    {
        hr = DV_E_FORMATETC;
        goto Cleanup;
    }
    hr = THR(CloneStgMedium(&_rgstgmed[i], pstgmed));

Cleanup:
    if (hr && _pLinkDataObj)
        hr = _pLinkDataObj->GetData(pfmtetc, pstgmed);

    RRETURN(hr);
}


HRESULT STDMETHODCALLTYPE CGenDataObject::QueryGetData(LPFORMATETC pfmtetc)
{
    HRESULT     hr = S_OK;
    int         i;
    FORMATETC * pfmtc;

    if (_dwPreferredEffect != DROPEFFECT_NONE &&
        FORMATETCMatchesRequest(pfmtetc, &_fmtcPreferredEffect))
    {
        goto Cleanup;
    }
    for (i = _rgfmtc.Size(), pfmtc = _rgfmtc; i > 0; i--, pfmtc++)
    {
        if (FORMATETCMatchesRequest(pfmtetc, pfmtc))
            goto Cleanup;
    }
    hr = DV_E_FORMATETC;

Cleanup:
    if (hr && _pLinkDataObj)
        hr = _pLinkDataObj->QueryGetData(pfmtetc);

    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE CGenDataObject::SetData(LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium, BOOL fRelease)
{
    HRESULT hr;

    hr = DeleteFormatData(pformatetc->cfFormat);
    if (hr)
        goto Cleanup;

    if (pmedium->hGlobal)
        hr = AppendFormatData(pformatetc->cfFormat, pmedium->hGlobal);

Cleanup:
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE
CGenDataObject::EnumFormatEtc(DWORD                 dwDirection,
                              LPENUMFORMATETC FAR * ppenumFormatEtc)
{
    if (_pLinkDataObj && !_fLinkFormatAppended)
    {
        _fLinkFormatAppended = TRUE;
        AppendFormatData(cf_FILEDESCA, NULL);
        AppendFormatData(cf_FILEDESCW, NULL);
        AppendFormatData(cf_FILECONTENTS, NULL);
        AppendFormatData(cf_UNIFORMRESOURCELOCATOR, NULL);
    }

    *ppenumFormatEtc = NULL;
    RRETURN((dwDirection == DATADIR_GET) ?
        CEnumFormatEtc::Create(_rgfmtc, _rgfmtc.Size(), ppenumFormatEtc) :
        E_NOTIMPL);
}

void
CGenDataObject::SetBtnState(DWORD dwKeyState)
{
    _dwButton = dwKeyState & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON);
}

//
//  CEnumFormatEtc PUBLIC methods
//

/*
 *  CEnumFormatEtc::QueryInterface (riid, ppvObj)
 *
 *  @mfunc
 *      IUnknown method
 *
 *  @rdesc
 *      HRESULT
 */

STDMETHODIMP CEnumFormatEtc::QueryInterface(
    REFIID riid,            // @parm Reference to requested interface ID
    void ** ppv)            // @parm out parm for interface ptr
{
    HRESULT     hresult = E_NOINTERFACE;

    *ppv = NULL;

    if( IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IEnumFORMATETC) )
    {
        *ppv = this;
        AddRef();
        hresult = NOERROR;
    }
    return hresult;
}

/*
 *  CEnumFormatEtc::AddRef()
 *
 *  @mfunc
 *      IUnknown method
 *
 *  @rdesc
 *      ULONG - incremented reference count
 */

STDMETHODIMP_(ULONG) CEnumFormatEtc::AddRef( )
{
    return ++_crefs;
}

/*
 *  CEnumFormatEtc::Release()
 *
 *  @mfunc
 *      IUnknown method
 *
 *  @rdesc
 *      ULONG - decremented reference count
 */

STDMETHODIMP_(ULONG) CEnumFormatEtc::Release( )
{
    _crefs--;

    if( _crefs == 0 )
    {
        delete this;
        return 0;
    }

    return _crefs;
}

/*
 *  CEnumFormatEtc::Next (celt, rgelt, pceltFetched)
 *
 *  @mfunc
 *      fetches the next [celt] elements in our formatetc collection
 *
 *  @rdesc
 *      HRESULT
 */

STDMETHODIMP CEnumFormatEtc::Next( ULONG celt, FORMATETC *rgelt,
        ULONG *pceltFetched)
{
    HRESULT     hresult = NOERROR;
    ULONG       cFetched;

    if( pceltFetched == NULL && celt != 1 )
    {
        // the spec says that if pceltFetched == NULL, then
        // the count of elements to fetch must be 1
        return E_INVALIDARG;
    }

    // we can only grab as many elements as there are left

    if( celt > _cTotal - _iCurrent )
    {
        cFetched = _cTotal - _iCurrent;
        hresult = S_FALSE;
    }
    else
    {
        cFetched = celt;
    }

    // Only copy if we have elements to copy

    if( cFetched > 0 )
    {
        memcpy( rgelt, _prgFormats + _iCurrent,
            cFetched * sizeof(FORMATETC) );
    }

    _iCurrent += cFetched;

    if( pceltFetched )
    {
        *pceltFetched = cFetched;
    }

    return hresult;
}

/*
 *  CEnumFormatEtc::Skip
 *
 *  @mfunc
 *      skips the next [celt] formats
 *
 *  @rdesc
 *      HRESULT
 */
STDMETHODIMP CEnumFormatEtc::Skip( ULONG celt )
{
    HRESULT     hresult = NOERROR;

    _iCurrent += celt;

    if( _iCurrent > _cTotal )
    {
        // whoops, skipped too far ahead.  Set us to the max limit.
        _iCurrent = _cTotal;
        hresult = S_FALSE;
    }

    return hresult;
}

/*
 *  CEnumFormatEtc::Reset
 *
 *  @mfunc
 *      resets the seek pointer to zero
 *
 *  @rdesc
 *      HRESULT
 */
STDMETHODIMP CEnumFormatEtc::Reset( void )
{
    _iCurrent = 0;

    return NOERROR;
}

/*
 *  CEnumFormatEtc::Clone
 *
 *  @mfunc
 *      clones the enumerator
 *
 *  @rdesc
 *      HRESULT
 */

STDMETHODIMP CEnumFormatEtc::Clone( IEnumFORMATETC **ppIEnum )
{
    return CEnumFormatEtc::Create(_prgFormats, _cTotal, ppIEnum);
}

/*
 *  CEnumFormatEtc::Create (prgFormats, cTotal, hr)
 *
 *  @mfunc
 *      creates a new format enumerator
 *
 *  @rdesc
 *      HRESULT
 *
 *  @devnote
 *      *copies* the formats passed in.  We do this as it simplifies
 *      memory management under OLE object liveness rules
 */

HRESULT CEnumFormatEtc::Create( FORMATETC *prgFormats, ULONG cTotal,
    IEnumFORMATETC **ppenum )
{
    CEnumFormatEtc *penum = new CEnumFormatEtc();

    if( penum != NULL )
    {
        // _iCurrent, _crefs are set in the constructor

        if( cTotal > 0 )
        {
            penum->_prgFormats = new(Mt(CEnumFormatEtc_prgFormats)) FORMATETC[cTotal];
            if( penum->_prgFormats )
            {
                penum->_cTotal = cTotal;
                memcpy(penum->_prgFormats, prgFormats,
                        cTotal * sizeof(FORMATETC));
                *ppenum = penum;
                return NOERROR;
            }
        }
    }

    return E_OUTOFMEMORY;
}

//
// CEnumFormatEtc PRIVATE methods
//

/*
 *  CEnumFormatEtc::CEnumFormatEtc()
 *
 *  @mfunc
 *      Private constructor
 */

CEnumFormatEtc::CEnumFormatEtc()
{
    _cTotal = 0;
    _crefs  = 1;
    _prgFormats = NULL;
    _iCurrent = 0;
}


/*
 *  CEnumFormatEtc::~CEnumFormatEtc()
 *
 *  @mfunc
 *      Private destructor
 */

CEnumFormatEtc::~CEnumFormatEtc( void )
{
    if( _prgFormats )
    {
        delete [] _prgFormats;
    }
}

