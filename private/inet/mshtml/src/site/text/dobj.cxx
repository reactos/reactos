#include "headers.hxx"

#ifndef X__DXFROBJ_H_
#define X__DXFROBJ_H_
#include "_dxfrobj.h" // for CEnumFormatEtc
#endif

#ifndef X_DOBJ_HXX_
#define X_DOBJ_HXX_
#include "dobj.hxx"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

MtDefine(CDataObject, Utilities, "CDataObject")
MtDefine(CDataObject_pFormats, CDataObject, "CDataObject::_pFormats")

//+------------------------------------------------------------------------
//
//  Member:     CDataObject constructor
//
//  Arguments:  lpszText    text to be contained in the object
//
//-------------------------------------------------------------------------

CDataObject::CDataObject (LPSTR lpszText) : super()
{
    HRESULT     hr = S_OK;

    //BUGBUG (alexz): this should be done in base class, but it is not
    _ulRefs = 1;
    _hObj = NULL;
    _pLinkDataObj = NULL;

    hr = THR(InitObj(lpszText));
    if (!OK(hr))
        goto Cleanup;

    IGNORE_HR(InitFormats());

Cleanup:
    NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CDataObject destructor
//
//-------------------------------------------------------------------------

CDataObject::~CDataObject ()
{
    if (_hObj)
        DoneObj ();
    if (_pFormats)
        DoneFormats ();
}

//+------------------------------------------------------------------------
//
//  Member:     CDataObject::InitObj
//
//  Synopsis:   allocates memory holding text and copies the text there
//
//  Arguments:  lpszText    text to be contained in the object
//
//-------------------------------------------------------------------------

HRESULT
CDataObject::InitObj (LPSTR lpszInitText)
{
    HRESULT     hr = S_OK;
    int         nText;
    LPSTR       lpszDestText;

    Assert (NULL == _hObj);

    nText = strlen(lpszInitText);
    if (0 == nText)
        return S_OK;

    _hObj = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, nText + 1);

    if (!_hObj)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    lpszDestText = (LPSTR) GlobalLock(_hObj);
    if (!lpszDestText)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // N.B. (johnv) Under Win95, GlobalSize() may return a different value
    // than requested by GlobalAlloc().  We therefore clear any extra bytes
    // that were allocated.
    strcpy( lpszDestText, lpszInitText );
    memset( lpszDestText + nText + 1, 0, GlobalSize(_hObj) - (nText + 1) );

Cleanup:
    if (_hObj)
        GlobalUnlock(_hObj);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDataObject::DoneObj
//
//  Synopsis:   deallocates memory holding object
//
//-------------------------------------------------------------------------

void
CDataObject::DoneObj ()
{
    Assert (_hObj);
    GlobalFree (_hObj);
}

//+------------------------------------------------------------------------
//
//  Member:     CDataObject::InitFormats
//
//  Synopsis:   initializes formats array
//
//
//-------------------------------------------------------------------------

HRESULT
CDataObject::InitFormats ()
{
    HRESULT     hr = S_OK;

    _cFormats = 1;
    _pFormats = new(Mt(CDataObject_pFormats)) FORMATETC[_cFormats];
    if (!_pFormats)
    {
        _cFormats = 0;
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    _pFormats[0] = g_rgFETC[iHTML]; // HTML

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDataObject::DoneFormats
//
//  Synopsis:   releases formats array
//
//-------------------------------------------------------------------------

void
CDataObject::DoneFormats ()
{
    Assert (_pFormats);
    delete _pFormats;
    _cFormats = 0;
}

//+------------------------------------------------------------------------
//
//  Member:     CDataObject::EnumFormatEtc
//
//  Synopsis:   creates and returns formats enumerator object
//
//-------------------------------------------------------------------------

HRESULT
CDataObject::EnumFormatEtc (DWORD dwDirection, IEnumFORMATETC ** ppenumFormatEtc)
{
    HRESULT hr = S_OK;

    if (dwDirection == DATADIR_GET)
    {
        hr = THR(CEnumFormatEtc::Create(_pFormats, _cFormats, ppenumFormatEtc));
    }
    else
        *ppenumFormatEtc = NULL;

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDataObject::GetData
//
//  Synopsis:   if can, returns data contained in the object
//
//  Arguments:  [in]pformatetc      format of data to return
//              [out]pmedium        returned data
//
//-------------------------------------------------------------------------

HRESULT
CDataObject::GetData (FORMATETC * pformatetc, STGMEDIUM * pmedium)
{
    if ((pformatetc->cfFormat == cf_HTML) &&
        (pformatetc->tymed & TYMED_HGLOBAL) )
    {
        //  Make sure the stgmed is empty, because we don't free it.
        Assert(pmedium->tymed == TYMED_NULL);
        Assert(pmedium->pUnkForRelease == NULL);

        pmedium->hGlobal = DuplicateHGlobal(_hObj);
        pmedium->tymed = TYMED_HGLOBAL;
        pmedium->pUnkForRelease = NULL;

        return S_OK;
    }
    else
        return DV_E_FORMATETC;
}

//+------------------------------------------------------------------------
//
//  Member:     CDataObject::QueryGetData
//
//  Synopsis:   used to query if the object supports format and media
//
//  Returns:    S_OK                if object supports the format and media
//              DV_E_FORMATETC      otherwise
//
//-------------------------------------------------------------------------

HRESULT
CDataObject::QueryGetData (FORMATETC * pformatetc )
{
    ULONG   nIdx;

    for (nIdx = 0; nIdx < _cFormats; nIdx++)
    {
        if ( (pformatetc->cfFormat == _pFormats[nIdx].cfFormat) && 
             (pformatetc->tymed & TYMED_HGLOBAL) )
        {
            return S_OK;
        }
    }

    return DV_E_FORMATETC;
}

//+------------------------------------------------------------------------
//
//  Member:     CDataObject::Create
//
//  Synopsis:   Static creator of text data objects
//
//  Arguments:  lpszText    text to be contained in the object
//              ppObj       created object
//
//-------------------------------------------------------------------------

HRESULT
CDataObject::Create(LPSTR lpszText, CDataObject ** ppObj)
{
    *ppObj = new CDataObject(lpszText);
    RRETURN (*ppObj ? S_OK : E_OUTOFMEMORY);
}

//+------------------------------------------------------------------------
//
//  Member:     CDataObject::Create
//
//  Synopsis:   Static creator of text data objects
//
//  Arguments:  lpszText        text to be contained in the object
//              ppDataObject    IDataObject interface of CDataObject
//
//-------------------------------------------------------------------------

HRESULT
CDataObject::CreateFromStr(LPSTR lpszText, IDataObject ** ppDataObject)
{
    HRESULT             hr;
    CDataObject     * pObj = NULL;

    hr = THR(Create(lpszText, &pObj));
    if (!OK(hr))
        goto Cleanup;
    
    * ppDataObject = (IDataObject*)pObj;

Cleanup:
    RRETURN (hr);
}
