#include <headers.hxx>

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_BMKCOLL_H_
#define X_BMKCOLL_H_
#include "bmkcoll.hxx"
#endif

#ifndef X_ADO_ADOID_H_
#define X_ADO_ADOID_H_
#include <adoid.h>
#endif

#ifndef X_ADO_ADOINT_H_
#define X_ADO_ADOINT_H_
#include <adoint.h>
#endif

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include "binder.hxx"
#endif

#define _cxx_
#include "bmkcoll.hdl"

EXTERN_C const GUID CLSID_HTMLDocument;
typedef Recordset15 IADORecordset;    // beats me why ADO doesn't use I...
typedef ADORecordsetConstruction IADORecordsetConstruction;

MtDefine(CBookmarkCollection, ObjectModel, "CBookmarkCollection");
MtDefine(CBookmarkCollection_aryBookmarks_pv, CBookmarkCollection, "CBookmarkCollection::_aryBookmarks::_pv");


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//      CBookmarkCollection
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+----------------------------------------------------------------
//
//  member : classdesc
//
//+----------------------------------------------------------------

const CBase::CLASSDESC CBookmarkCollection::s_classdesc =
{
    &CLSID_HTMLDocument,            // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLBookmarkCollection,   // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};


//+---------------------------------------------------------------
//
//  Member  : CBookmarkCollection::~CBookmarkCollection (destructor)
//
//  Synopsis : Release all the variants
//
//----------------------------------------------------------------

CBookmarkCollection::~CBookmarkCollection()
{
    VARIANT *pVar;
    int i;

    for (pVar=_aryBookmarks, i=_aryBookmarks.Size();  i>0;  ++pVar, --i)
    {
        VariantClear(pVar);
    }
    _aryBookmarks.DeleteAll();
}


//+---------------------------------------------------------------
//
//  Member  : CBookmarkCollection::Init
//
//  Synopsis : Convert a list of HROWs to ADO bookmarks, and use these
//             to fill the collection.
//
//----------------------------------------------------------------

HRESULT
CBookmarkCollection::Init(const HROW *rghRows, ULONG cRows, IADORecordset *pADO)
{
    HRESULT hr = S_OK;
    _ADORecordset *pADOClone = NULL;
    IADORecordsetConstruction *pADOConstruction = NULL;
    IRowPosition *pRowPos = NULL;
    const HROW *pHRow;
    HCHAPTER hChapter = DB_NULL_HCHAPTER;
    HROW hrow = DB_NULL_HROW;
    DWORD dwFlags;
    VARIANT varBmk;

    // get a copy of the recordset, so as not to disturb currency on the main one
    hr = pADO->_xClone(&pADOClone);

    // get the row position from the clone, and get its chapter
    if (!hr)
        hr = pADOClone->QueryInterface(IID_IADORecordsetConstruction,
                                       (void**)&pADOConstruction);
    if (!hr)
        hr = pADOConstruction->get_RowPosition((IUnknown**)&pRowPos);
    if (!hr)
        hr = pRowPos->GetRowPosition(&hChapter, &hrow, &dwFlags);

    // get room for the variants that hold the bookmarks
    Assert(_aryBookmarks.Size() == 0);
    if (!hr)
        hr = _aryBookmarks.EnsureSize(cRows);
    if (hr)
        goto Cleanup;

    // get an ADO bookmark for each row, and add it to the array
    VariantInit(&varBmk);
    for (pHRow = rghRows; cRows > 0; --cRows, ++pHRow)
    {
        hr = pRowPos->ClearRowPosition();
        if (!hr)
            hr = pRowPos->SetRowPosition(hChapter, *pHRow, DBPOSITION_OK);
        if (!hr)
            hr = pADOClone->get_Bookmark(&varBmk);
        if (!hr && V_VT(&varBmk) != VT_EMPTY)
        {
            VARIANT *pVar = NULL;
            hr = _aryBookmarks.AppendIndirect(NULL, &pVar);
            if (!hr && pVar)
            {
                VariantCopy(pVar, &varBmk);
            }
            VariantClear(&varBmk);
        }
    }

    hr = S_OK;      // clamp any errors found while building the array

Cleanup:
    ReleaseChapterAndRow(hChapter, hrow, pRowPos);
    ReleaseInterface(pRowPos);
    ReleaseInterface(pADOConstruction);
    ReleaseInterface(pADOClone);
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member  : CBookmarkCollection::PrivateQueryInterface
//
//  Synopsis : Vanilla implementation for this class
//
//----------------------------------------------------------------

HRESULT
CBookmarkCollection::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_INHERITS(this, IDispatch)
        QI_TEAROFF(this, IObjectIdentity, NULL)
    default:
        if (iid == IID_IHTMLBookmarkCollection)
        {
           *ppv = (IHTMLBookmarkCollection *) this;
        }
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


//+---------------------------------------------------------------
//
//  Member  : CBookmarkCollection::length
//
//  Sysnopsis : IHTMLBookmarkCollection interface method
//
//----------------------------------------------------------------

HRESULT
CBookmarkCollection::get_length(long * pLength)
{
    HRESULT hr = S_OK;

    if (!pLength)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pLength = _aryBookmarks.Size();

Cleanup:
    RRETURN( SetErrorInfo( hr ));

}


//+---------------------------------------------------------------
//
//  Member  : CBookmarkCollection::item
//
//  Sysnopsis : IHTMLBookmarkCollection interface method
//
//----------------------------------------------------------------

HRESULT
CBookmarkCollection::item(long lIndex, VARIANT *pVarBookmark)
{
    HRESULT hr = S_OK;

    if (!pVarBookmark)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (lIndex < 0 || lIndex >= _aryBookmarks.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = VariantCopy(pVarBookmark, &_aryBookmarks[lIndex]);

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+---------------------------------------------------------------
//
//  Member  : CBookmarkCollection::_newEnum
//
//  Sysnopsis : IHTMLBookmarkCollection interface method
//
//----------------------------------------------------------------

HRESULT
CBookmarkCollection::get__newEnum(IUnknown ** ppEnum)
{
    HRESULT hr = S_OK;

    if (!ppEnum)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppEnum = NULL;

    hr = THR(_aryBookmarks.EnumVARIANT(VT_I4,
                                (IEnumVARIANT**)ppEnum,
                                FALSE,
                                FALSE));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

