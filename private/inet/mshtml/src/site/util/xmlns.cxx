//+---------------------------------------------------------------------
//
//  File:       xmlns.cxx
//
//  Classes:    CXmlNamespaceTable, CXmlUrnAtomTable
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_XMLNS_HXX_
#define X_XMLNS_HXX_
#include "xmlns.hxx"
#endif

///////////////////////////////////////////////////////////////////////////
//
//  misc
//
///////////////////////////////////////////////////////////////////////////

MtDefine(CXmlNamespaceTable,                   Mem,                "CXmlNamespaceTable");
MtDefine(CXmlUrnAtomTable,                     Mem,                "CXmlUrnAtomTable");

MtDefine(CXmlNamespaceTable_CItemsArray,       CStringToAtomTable, "CStringToAtomTable::CItemsArray");

EXTERN_C const GUID SID_SXmlNamespaceMapping = {0x3050f628,0x98b5,0x11cf, {0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b}};
EXTERN_C const GUID CGID_XmlNamespaceMapping = {0x3050f629,0x98b5,0x11cf, {0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b}};
#define XMLNAMESPACEMAPPING_GETURN 1

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CXmlNamespaceTable::CItem
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CXmlNamespaceTable::CItem::Clear
//
//-------------------------------------------------------------------------

void
CXmlNamespaceTable::CItem::Clear()
{
    _cstr.Free();
}

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CXmlNamespaceTable
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CXmlNamespaceTable constructor
//
//-------------------------------------------------------------------------

CXmlNamespaceTable::CXmlNamespaceTable (CDoc * pDoc)
{
    _ulRefs = 1;
    _pDoc = pDoc;
}

//+------------------------------------------------------------------------
//
//  Member:     CXmlNamespaceTable destructor
//
//-------------------------------------------------------------------------

CXmlNamespaceTable::~CXmlNamespaceTable ()
{
    int     c;
    CItem * pItem;

    for (pItem = _aryItems, c = _aryItems.Size(); 0 < c; pItem++, c--)
    {
        pItem->Clear();
    }
    _aryItems.DeleteAll();
}

//+------------------------------------------------------------------------
//
//  Member:     CXmlNamespaceTable::QueryInterface
//
//-------------------------------------------------------------------------

HRESULT
CXmlNamespaceTable::QueryInterface(REFIID iid, void** ppv)
{
    if (!ppv)
        RRETURN(E_POINTER);

    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS(this, IUnknown)
    QI_INHERITS(this, IOleCommandTarget)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
        RRETURN(E_NOINTERFACE);
}

//+------------------------------------------------------------------------
//
//  Member:     CXmlNamespaceTable::Init
//
//-------------------------------------------------------------------------

HRESULT
CXmlNamespaceTable::Init()
{
    HRESULT     hr;

    hr = THR(RegisterNamespace(COMPONENT_NAMESPACE, COMPONENT_URN, XMLNAMESPACETYPE_ATTR));
    if (hr)
        goto Cleanup;

    _urnAtomComponent = 0;

#if DBG == 1
    LPTSTR pchUrnComponent;
    Assert (_pDoc->_pXmlUrnAtomTable);
    Assert (S_OK == THR(_pDoc->_pXmlUrnAtomTable->GetUrn(_urnAtomComponent, &pchUrnComponent)));
    Assert (0 == StrCmp(COMPONENT_URN, pchUrnComponent));
#endif

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CXmlNamespaceTable::FindItem
//
//-------------------------------------------------------------------------

CXmlNamespaceTable::CItem *
CXmlNamespaceTable::FindItem(LPTSTR pch)
{
    int         c;
    CItem *     pItem;

    // check cached value
    if (_pLastItem)
    {
        if (0 == StrCmpIC(_pLastItem->_cstr, pch))
            return _pLastItem;
    }

    // do the search
    for (pItem = _aryItems, c = _aryItems.Size(); 0 < c; pItem++, c--)
    {
        Assert (!pItem->_cstr.IsNull());
        if (pItem != _pLastItem && 0 == StrCmpIC(pItem->_cstr, pch))
        {
            // succeeded
            _pLastItem = pItem;
            return pItem;
        }
    }

    // failed
    _pLastItem = NULL;
    return NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CXmlNamespaceTable::EnsureItem
//
//-------------------------------------------------------------------------

HRESULT
CXmlNamespaceTable::EnsureItem(LPTSTR pch, CItem ** ppItem, BOOL * pfNew)
{
    HRESULT     hr = S_OK;

    (*ppItem) = FindItem(pch);

    if (!(*ppItem))
    {
        (*ppItem) = _aryItems.Append();
        if (!(*ppItem))
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        (*ppItem)->_atom = -1;
        (*ppItem)->_type = XMLNAMESPACETYPE_NULL;

        hr = THR((*ppItem)->_cstr.Set(pch));
        if (hr)
            goto Cleanup;

        if (pfNew)
        {
            *pfNew = TRUE;
        }
    }
    else
    {
        if (pfNew)
        {
            *pfNew = FALSE;
        }
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CXmlNamespaceTable::RegisterNamespace
//
//-------------------------------------------------------------------------

HRESULT
CXmlNamespaceTable::RegisterNamespace(
    LPTSTR pchNamespace, LPTSTR pchUrn, XMLNAMESPACETYPE namespaceType, BOOL * pfChangeDetected)
{
    HRESULT             hr;
    CItem *             pItem;
    CXmlUrnAtomTable *  pUrnAtomTable;

    //
    // startup
    //

    Assert (pchNamespace && pchNamespace[0]);   // namespace should be present and non-empty

    if (pchUrn && 0 == pchUrn[0])               // don't allow empty urns
        pchUrn = NULL;

    //
    // ensure table and item in the table
    //

    hr = THR(_pDoc->EnsureXmlUrnAtomTable(&pUrnAtomTable));
    if (hr)
        goto Cleanup;

    hr = THR(EnsureItem(pchNamespace, &pItem, /* pfNew = */pfChangeDetected));
    if (hr)
        goto Cleanup;

    pItem->_type = namespaceType;

    //
    // setup urn
    //

    if (pchUrn)
    {
        LPTSTR  pchCurrentUrn;

        if (-1 != pItem->_atom)         // if currently the namespace maps to a urn
        {
            hr = THR(pUrnAtomTable->GetUrn(pItem->_atom, &pchCurrentUrn));
            if (hr)
                goto Cleanup;

            if (0 == StrCmpIC(pchUrn, pchCurrentUrn))   // if the urn this namespace currently maps to is the same as requested
                goto Cleanup;                           // done - nothing more todo
        }

        // Assert (pchUrn is different from urn specified in pItem);

        hr = THR(pUrnAtomTable->EnsureUrnAtom(pchUrn, &pItem->_atom));
        if (hr)
            goto Cleanup;

        if (pfChangeDetected)
        {
            *pfChangeDetected = TRUE;
        }
    }


Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CXmlNamespaceTable::IsXmlSprinkle
//
//-------------------------------------------------------------------------

ELEMENT_TAG
CXmlNamespaceTable::IsXmlSprinkle (LPTSTR pchNamespace)
{
    CItem * pItem = FindItem(pchNamespace);

    if (pItem)
    {
        if (pItem->_atom == _urnAtomComponent)
        {
            return ETAG_GENERIC_BUILTIN;
        }
        else
        {
            return ETAG_GENERIC;
        }
    }

    return ETAG_UNKNOWN;
}

//+------------------------------------------------------------------------
//
//  Member:     CXmlNamespaceTable::GetUrnAtom
//
//-------------------------------------------------------------------------

HRESULT
CXmlNamespaceTable::GetUrnAtom(LPTSTR pchNamespace, LONG * pAtom)
{
    CItem *     pItem = FindItem(pchNamespace);

    Assert (pAtom);

    *pAtom = pItem ? pItem->_atom : -1;

    RRETURN (S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:     CXmlNamespaceTable::GetUrn
//
//-------------------------------------------------------------------------

HRESULT
CXmlNamespaceTable::GetUrn(LONG urnAtom, LPTSTR * ppchUrn)
{
    HRESULT     hr = S_OK;

    Assert (ppchUrn);

    if (-1 != urnAtom)
    {
        Assert (_pDoc->_pXmlUrnAtomTable);      // should have been ensured when we stored urnAtom

        hr = THR(_pDoc->_pXmlUrnAtomTable->GetUrn(urnAtom, ppchUrn));
    }
    else
    {
        *ppchUrn = NULL;
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CXmlNamespaceTable::GetUrn
//
//-------------------------------------------------------------------------

HRESULT
CXmlNamespaceTable::GetUrn(LPTSTR pchNamespace, LPTSTR * ppchUrn)
{
    HRESULT     hr;
    LONG        urnAtom;

    hr = GetUrnAtom(pchNamespace, &urnAtom);
    if (hr)
        goto Cleanup;

    hr = THR(GetUrn(urnAtom, ppchUrn));

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CXmlNamespaceTable::SaveNamespaceAttrs
//
//-------------------------------------------------------------------------

HRESULT
CXmlNamespaceTable::SaveNamespaceAttrs (CStreamWriteBuff * pStreamWriteBuff)
{
    HRESULT     hr = S_OK;
    int         c;
    CItem *     pItem;
    LPTSTR      pchUrn;

    for (pItem = _aryItems, c = _aryItems.Size(); 0 < c; pItem++, c--)
    {
        if (XMLNAMESPACETYPE_ATTR == pItem->_type)
        {
            if (_urnAtomComponent == pItem->_atom &&
                0 == StrCmpIC(COMPONENT_NAMESPACE, pItem->_cstr))
                continue;

            if (-1 != pItem->_atom)
            {
                Assert (_pDoc->_pXmlUrnAtomTable);

                hr = THR(_pDoc->_pXmlUrnAtomTable->GetUrn (pItem->_atom, &pchUrn));
                if (hr)
                    goto Cleanup;
            }
            else
            {
                pchUrn = NULL;
            }

            hr = THR(pStreamWriteBuff->EnsureNamespaceSaved(
                _pDoc, pItem->_cstr, pchUrn, XMLNAMESPACETYPE_ATTR));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CXmlNamespaceTable::Exec
//
//-------------------------------------------------------------------------

HRESULT
CXmlNamespaceTable::Exec(
    const GUID *    pguidCmdGroup,
    DWORD           nCmdID,
    DWORD           nCmdExecOpt,
    VARIANT *       pvarArgIn,
    VARIANT *       pvarArgOut)
{
    HRESULT     hr;

    if (!pguidCmdGroup)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (IsEqualGUID(CGID_XmlNamespaceMapping, *pguidCmdGroup))
    {
        hr = OLECMDERR_E_NOTSUPPORTED;

        switch (nCmdID)
        {
        case XMLNAMESPACEMAPPING_GETURN:

            if (!pvarArgOut ||                                              // if no out arg or
                !pvarArgIn ||                                               // no in arg or
                VT_BSTR != V_VT(pvarArgIn) ||                               // in arg is not a string or
                !V_BSTR(pvarArgIn) || 0 == ((LPTSTR)V_BSTR(pvarArgIn))[0])  // the string is empty
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }

            {
                LPTSTR  pchUrn;

                hr = THR(GetUrn(V_BSTR(pvarArgIn), &pchUrn));
                if (hr)
                    goto Cleanup;

                V_VT  (pvarArgOut) = VT_BSTR;
                if (pchUrn)
                {
                    hr = THR(FormsAllocString(pchUrn, &V_BSTR(pvarArgOut)));
                }
                else
                {
                    V_BSTR(pvarArgOut) = NULL;
                    hr = S_OK;
                }
            }

            break;
        }
    }
    else
    {
        hr = OLECMDERR_E_UNKNOWNGROUP;
    }

Cleanup:
    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CXmlUrnAtomTable
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CXmlUrnAtomTable constructor
//
//-------------------------------------------------------------------------

CXmlUrnAtomTable::CXmlUrnAtomTable ()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CXmlUrnAtomTable destructor
//
//-------------------------------------------------------------------------

CXmlUrnAtomTable::~CXmlUrnAtomTable ()
{
    Free();
}

//+------------------------------------------------------------------------
//
//  Member:     CXmlUrnAtomTable::EnsureUrnAtom
//
//-------------------------------------------------------------------------

HRESULT
CXmlUrnAtomTable::EnsureUrnAtom(LPTSTR pchUrn, LONG * pAtom)
{
    HRESULT     hr;

    Assert (pAtom);

    hr = THR(AddNameToAtomTable(pchUrn, pAtom));

    RRETURN (hr);
}
