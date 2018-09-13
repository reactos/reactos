//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       sheetcol.cxx
//
//  Contents:   Support for collections of Cascading Style Sheets.. including:
//
//              CStyleSheetArray
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EANCHOR_HXX_
#define X_EANCHOR_HXX_
#include "eanchor.hxx"  // For CAnchorElement decl, for pseudoclasses
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#ifndef X_FONTFACE_HXX_
#define X_FONTFACE_HXX_
#include "fontface.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#define _cxx_
#include "sheetcol.hdl"

DeclareTag(tagStyleSheetApply,                    "Style Sheet Apply", "trace Style Sheet application")

//*********************************************************************
//      CStyleSheetArray
//*********************************************************************
const CStyleSheetArray::CLASSDESC CStyleSheetArray::s_classdesc =
{
    {
        &CLSID_HTMLStyleSheetsCollection,    // _pclsid
        0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
        NULL,                                   // _apClsidPages
#endif // NO_PROPERTY_PAGE
        NULL,                                   // _pcpi
        0,                                   // _dwFlags
        &IID_IHTMLStyleSheetsCollection,     // _piidDispinterface
        &s_apHdlDescs                        // _apHdlDesc
    },
    (void *)s_apfnIHTMLStyleSheetsCollection         // _apfnTearOff
};

//*********************************************************************
//      CStyleSheetArray::CStyleSheetArray()
//  When you create a CStyleSheetArray, the owner element specified will
//  have a subref added.  In the case of the top-level stylesheets collection,
//  the owner is the CDoc.  In the case of imports collections, the owner is
//  a CStyleSheet.
//*********************************************************************
CStyleSheetArray::CStyleSheetArray(
    CBase * const pOwner,             // CBase obj that controls our lifetime/does our ref-counting.
    CStyleSheetArray * pRuleManager,  // NULL if we manage our own rules' storage (root doc's CSSA only)
    CStyleID const sidParentSheet)    // ID of our owner's SS (non-zero for CSSA storing imported SS only)
    :
    _pOwner(pOwner),
    _pRulesArrays(NULL),
    _sidForOurSheets(sidParentSheet),
    _fInvalid(FALSE)
{
    unsigned int parentLevel = sidParentSheet.FindNestingLevel();

    // If we are constructed w/ a NULL manager, then we manage our own storage
    _pSSARuleManager = pRuleManager ? pRuleManager : this;

    // Allocate storage for rules if we are a rules manager
    if (!pRuleManager) {
        _pRulesArrays = new CStyleRuleArray[ETAG_LAST];
        if (!_pRulesArrays)
        {
            _fInvalid = TRUE;
            goto Cleanup;
        }
    }

    // We are one level deeper than our parents.
    _Level = parentLevel + 1;
    Assert( "Invalid level computed for stylesheet array! (informational only)" && _Level > 0 && _Level < 5 );
    if ( _Level > MAX_IMPORT_NESTING )
    {
        _Level = MAX_IMPORT_NESTING;
        _fInvalid = TRUE;
        goto Cleanup;
    }

    _sidForOurSheets.SetRule(0);  // Clear rule information

    // We want _sidForOurSheets to be the ID that should be assigned to stylesheets built by this array.
    // If we're being built to hold imported stylesheets (_Level > 1), patch the ID by lowering previous
    // level by 1.
    if ( _Level > 1 )
    {
        unsigned long parentLevelValue = sidParentSheet.GetLevel( parentLevel );
        Assert( "Nested stylesheets must have non-zero parent level value!" && parentLevelValue );
        _sidForOurSheets.SetLevel( parentLevel, parentLevelValue-1 );
    }

    // Add-ref ourselves before returning from constructor.  This will actually subref our owner.
    AddRef();

Cleanup:
    if (_fInvalid && _pRulesArrays)
        delete [] _pRulesArrays;
}

//*********************************************************************
//      CStyleSheetArray::~CStyleSheetArray()
//  Recall we don't maintain our own ref-count; this means our memory
//  doesn't go away until our owner's memory is going away.  In order
//  to do this, the owner has to call ->CBase::PrivateRelease() on us.
//  (see the CDoc destructor for example).
//  This works because: a) we start with an internal ref-count of 1,
//  which is keeping us alive, b) that internal ref-count is never
//  exposed to changes (all AddRef/Release calls are delegated out).
//  So in effect the internal ref-count of 1 is held by the owner.
//*********************************************************************
CStyleSheetArray::~CStyleSheetArray()
{
    Assert( "Must call Free() before destructor!" && _aStyleSheets.Size() == 0 );
    // Since we're actually going away entirely, free the rules arrays
    // BUGBUG: Eventually rules arrays will manage their own storage via ref-counting
    // so the destructor won't need to do anything!
    if (_pRulesArrays)
    {
        delete [] _pRulesArrays;    // Arrays should be empty; we're just releasing mem here
        _pRulesArrays = NULL;
    }
}

//*********************************************************************
//      CStyleSheetArray::PrivateQueryInterface()
//*********************************************************************
HRESULT
CStyleSheetArray::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
        default:
        {
            const CLASSDESC *pclassdesc = ElementDesc();

            if (pclassdesc &&
                pclassdesc->_apfnTearOff &&
                pclassdesc->_classdescBase._piidDispinterface &&
                (iid == *pclassdesc->_classdescBase._piidDispinterface))
            {
                HRESULT hr = THR(CreateTearOffThunk(this, (void *)(pclassdesc->_apfnTearOff), NULL, ppv));
                if (hr)
                    RRETURN(hr);
            }
        }
    }

    if (*ppv)
    {
        (*(IUnknown**)ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


//*********************************************************************
// CStyleSheetArray::Invoke, IDispatch
// Provides access to properties and members of the object
//
// Arguments:   [dispidMember] - Member id to invoke
//              [riid]         - Interface ID being accessed
//              [wFlags]       - Flags describing context of call
//              [pdispparams]  - Structure containing arguments
//              [pvarResult]   - Place to put result
//              [pexcepinfo]   - Pointer to exception information struct
//              [puArgErr]     - Indicates which argument is incorrect
//
// We override this to support ordinal and named member access to the
// elements of the collection.
//*********************************************************************

STDMETHODIMP
CStyleSheetArray::InvokeEx( DISPID       dispidMember,
                        LCID         lcid,
                        WORD         wFlags,
                        DISPPARAMS * pdispparams,
                        VARIANT *    pvarResult,
                        EXCEPINFO *  pexcepinfo,
                        IServiceProvider *pSrvProvider)
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    LPCTSTR pszName;
    long lIdx;

    // Is the dispid an ordinal index? (array access)
    if ( IsOrdinalSSDispID( dispidMember) )
    {
        if ( wFlags & DISPATCH_PROPERTYPUT )
        {
            // Stylesheets collection is readonly.
            // Inside OLE says return DISP_E_MEMBERNOTFOUND.
            goto Cleanup;
        }
        else if ( wFlags & DISPATCH_PROPERTYGET )
        {
            if (pvarResult)
            {
                lIdx = dispidMember - DISPID_STYLESHEETSCOLLECTION_ORDINAL_BASE;
                // item() will bounds check for us.
                hr = item( lIdx, (IHTMLStyleSheet **) &(pvarResult->pdispVal));
                if (hr)
                {
                    Assert( pvarResult->pdispVal == NULL );
                    pvarResult->vt = VT_NULL;
                }
                else
                {
                    Assert( pvarResult->pdispVal );
                    pvarResult->vt = VT_DISPATCH;
                }

            }
        }
    }
    else if ( IsNamedSSDispID( dispidMember) )
    {
        if ( wFlags & DISPATCH_PROPERTYPUT )
        {
            // Stylesheets collection is readonly.
            // Inside OLE says return DISP_E_MEMBERNOTFOUND.
            goto Cleanup;
        }
        else if ( wFlags & DISPATCH_PROPERTYGET )
        {
            if (pvarResult)
            {
                hr = GetAtomTable()->
                        GetNameFromAtom( dispidMember - DISPID_STYLESHEETSCOLLECTION_NAMED_BASE,
                                         &pszName );
                if (hr)
                {
                    // BUGBUG: Maybe not necessary to set return val to null?
                    pvarResult->pdispVal = NULL;
                    pvarResult->vt = VT_NULL;
                    goto Cleanup;
                }

                lIdx = FindSSByHTMLID( pszName, TRUE );
                // lIdx will be -1 if SS not found, in which case item will return an error.
                hr = item( lIdx, (IHTMLStyleSheet **) &(pvarResult->pdispVal));
                if (hr)
                {
                    Assert( pvarResult->pdispVal == NULL );
                    pvarResult->vt = VT_NULL;
                }
                else
                {
                    Assert( pvarResult->pdispVal );
                    pvarResult->vt = VT_DISPATCH;
                }
            }
        }
    }
    else
    {
        // CBase knows how to handle expando
        hr = THR_NOTRACE(super::InvokeEx( dispidMember,
                                        lcid,
                                        wFlags,
                                        pdispparams,
                                        pvarResult,
                                        pexcepinfo,
                                        pSrvProvider));
    }

Cleanup:
    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}

//*********************************************************************
//  CStyleSheetArray::GetDispID, IDispatchEx
//  Overridden to output a particular dispid range for ordinal access,
//  and another range for named member access.  Ordinal access dispids
//  range from DISPID_STYLESHEETSCOLLECTION_ORDINAL_BASE to
//  DISPID_STYLESHEETSCOLLECTION_ORDINAL_MAX.  Named access dispids
//  range from DISPID_STYLESHEETSCOLLECTION_NAMED_BASE to
//  DISPID_STYLESHEETSCOLLECTION_NAMED_MAX.
//********************************************************************

STDMETHODIMP
CStyleSheetArray::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT           hr = E_FAIL;
    long              lIdx = 0;
    long              lAtom = 0;
    DISPID            dispid = 0;

    Assert( bstrName && pid );

    // Could be an ordinal access
    hr = ttol_with_error(bstrName, &lIdx);
    if ( !hr )
    {
        dispid = DISPID_STYLESHEETSCOLLECTION_ORDINAL_BASE + lIdx;
        *pid = (dispid > DISPID_STYLESHEETSCOLLECTION_ORDINAL_MAX) ?
                        DISPID_UNKNOWN : dispid;
        hr = S_OK;
        goto Cleanup;
    }

    // Not ordinal access; could be named (via id) access if we're the top-level collection
    if ( _Level == 1 )
    {
        lIdx = FindSSByHTMLID( bstrName, (grfdex & fdexNameCaseSensitive) ? TRUE : FALSE );
        if ( lIdx != -1 )   // -1 means not found
        {
            // Found a matching ID!

            // Since we found the element in the elements collection,
            // update atom table.  This will just retrieve the atom if
            // the string is already in the table.
            Assert( bstrName );
            hr = GetAtomTable()->AddNameToAtomTable(bstrName, &lAtom);
            if ( hr )
                goto Cleanup;
            // lAtom is the index into the atom table.  Offset this by
            // base.
            lAtom += DISPID_STYLESHEETSCOLLECTION_NAMED_BASE;
            *pid = lAtom;
            if (lAtom > DISPID_STYLESHEETSCOLLECTION_NAMED_MAX)
            {
                hr = DISP_E_UNKNOWNNAME;
            }
            goto Cleanup;
        }
    }

    // Otherwise delegate to CBase impl for expando support etc.
    hr = THR_NOTRACE(super::GetDispID(bstrName, grfdex, pid));

Cleanup:
    RRETURN(THR_NOTRACE( hr ));
}

//*********************************************************************
//  CStyleSheetArray::GetNextDispID, IDispatchEx
//  Supports enumerating our collection indices in addition to the
//  collection's own properties.  Semantically this implementation is
//  known to be incorrect; prgbstr and prgid should match (both should
//  be the next dispid).  This is due to the current implementation in
//  the element collections code (collect.cxx); when that gets fixed,
//  this should be looked at again.  In particular, the way we begin
//  to enumerate indices is wonky; we should be using the start enum
//  DISPID value.
//*********************************************************************
STDMETHODIMP
CStyleSheetArray::GetNextDispID(DWORD grfdex,
                                DISPID id,
                                DISPID *prgid)
{
    HRESULT hr = S_OK;
    Assert( prgid );

    // Are we in the middle of enumerating our indices?
    if ( !IsOrdinalSSDispID(id) )
    {
        // No, so delegate to CBase for normal properties
        hr = super::GetNextDispID( grfdex, id, prgid );
        if (hr)
        {
            // normal properties are done, so let's start enumerating indices
            // if we aren't empty.  Return string for index 0,
            // and DISPID for index 1.
            if (_aStyleSheets.Size())
            {
                *prgid = DISPID_STYLESHEETSCOLLECTION_ORDINAL_BASE;
                hr = S_OK;
            }
        }
    }
    else
    {
        // Yes we're enumerating indices, so return string of current DISPID, and DISPID for next index,
        // or DISPID_UNKNOWN if we're out of bounds.
        if ( !IsOrdinalSSDispID(id+1) || (((long)(id+1-DISPID_STYLESHEETSCOLLECTION_ORDINAL_BASE)) >= _aStyleSheets.Size()) )
        {
            *prgid = DISPID_UNKNOWN;
            hr = S_FALSE;
            goto Cleanup;
        }

        ++id;
        *prgid = id;
    }

Cleanup:
    RRETURN1(THR_NOTRACE( hr ), S_FALSE);
}

STDMETHODIMP
CStyleSheetArray::GetMemberName(DISPID id, BSTR *pbstrName)
{
    TCHAR   ach[20];

    if (!pbstrName)
        return E_INVALIDARG;

    *pbstrName = NULL;

    // Are we in the middle of enumerating our indices?
    if ( !IsOrdinalSSDispID(id) )
    {
        // No, so delegate to CBase for normal properties
        super::GetMemberName(id, pbstrName);
    }
    else
    {
        // Yes we're enumerating indices, so return string of current DISPID, and DISPID for next index,
        // or DISPID_UNKNOWN if we're out of bounds.
        if ( !IsOrdinalSSDispID(id) || (((long)(id-DISPID_STYLESHEETSCOLLECTION_ORDINAL_BASE)) >= _aStyleSheets.Size()) )
            goto Cleanup;

        if (Format(0, ach, ARRAY_SIZE(ach), _T("<0d>"), (long)id-DISPID_STYLESHEETSCOLLECTION_ORDINAL_BASE))
            goto Cleanup;

        FormsAllocString(ach, pbstrName);
    }

Cleanup:
    return *pbstrName ? S_OK : DISP_E_MEMBERNOTFOUND;
}

// Helpers for determining whether dispids fall into particular ranges.
BOOL CStyleSheetArray::IsOrdinalSSDispID( DISPID dispidMember )
{
    return ((dispidMember >= DISPID_STYLESHEETSCOLLECTION_ORDINAL_BASE) &&
           (dispidMember <= DISPID_STYLESHEETSCOLLECTION_ORDINAL_MAX));
}

BOOL CStyleSheetArray::IsNamedSSDispID( DISPID dispidMember )
{
    return ((dispidMember >= DISPID_STYLESHEETSCOLLECTION_NAMED_BASE) &&
           (dispidMember <= DISPID_STYLESHEETSCOLLECTION_NAMED_MAX));
}


//*********************************************************************
//      CStyleSheetArray::Free()
//              Release all stylesheets we're holding, and clear our storage.
//  After this has been called, we are an empty array.  If we were
//      responsible for holding onto rules, they too have been emptied.
//*********************************************************************
void CStyleSheetArray::Free( void )
{
    // Forget all the CStyleSheets we're storing
    CStyleSheet **ppSheet;
    int z;
    int nSheets = _aStyleSheets.Size();
    for (ppSheet = (CStyleSheet **) _aStyleSheets, z=0; z<nSheets; ++z, ++ppSheet)
    {
         // We need to make sure that when imports stay alive after the collection
         // holding them dies, they don't point to their original parent (all sorts
         // of badness would occur because we wouldn't be able to tell that the
         // import is effectively out of the collection, since the parent chain would
         // be intact).
         (*ppSheet)->DisconnectFromParentSS();
         if ( (*ppSheet)->GetRootContainer() == this )
             (*ppSheet)->_pSSAContainer = NULL;

         (*ppSheet)->Release();
    }
    _aStyleSheets.DeleteAll();

    // Forget all the CFontFaces we're storing
    CFontFace **ppFontFace;
    int nFaces = _apFontFaces.Size();
    for (ppFontFace = (CFontFace **) _apFontFaces, z=0; z<nFaces; ++z, ++ppFontFace)
    {
        (*ppFontFace)->PrivateRelease();
    }
    _apFontFaces.DeleteAll();

    // Free all rules we're storing on behalf of our stylesheets if we're
    // a rule manager, _but_ don't free the array of CStyleRuleArrays.
    // That will be done only on destruction (CDoc::UnloadContents, which calls
    // this Free(), is called directly on refresh, w/o going through CDoc:Passivate)

    // BUGBUG: This needs to change once rules are ref-counted.  Rules need
    // a way to manage their own lifetime, since the lifetime of the CSSA
    // is managed by the doc, which lives essentially forever (refreshes don't destroy
    // the doc, yet we need to clear/mark dead/delete the rules somewhere).
    // Consider keeping ref-count info in each rule array, and have a trivial
    // impl of addref/release on the rules which directly delegate to their
    // containing array.  Then the SSA holds refs on the rules arrays directly,
    // which are released and forgotten here.  Hence rules would be kept
    // alive if their stylesheet were still alive, or if any ext. refs existed.
    // SS would hold indirect refs on the rules array by holding refs on
    // individual rules.

    if (_pRulesArrays)
    {
        for ( z=0 ; z < ETAG_LAST ; ++z )
            _pRulesArrays[z].Free( );
    }

    _cstrUserStylesheet.Free();
}

//*********************************************************************
//  CStyleSheetArray::CreateNewStyleSheet()
//      This method creates a new CStyleSheet object and adds it to the
//  array (at the end).  The newly created stylesheet has an ref count of 1
//  and has +1 subrefs on the parent element, which are considered to be
//  held by the stylesheet array.  If the caller decides to keep a pointer
//  to the newly created stylesheet (e.g. when a STYLE or LINK element
//  creates a new stylesheet and tracks it), it must call CreateNewStyleSheet
//  to get the pointer and then make sure to AddRef it.
//*********************************************************************

HRESULT CStyleSheetArray::CreateNewStyleSheet( CElement *pParentElem, CStyleSheet **ppStyleSheet,
                                               long lPos /*=-1*/, long *plNewPos /*=NULL*/)
{
    // lPos == -1 indicates append.
    HRESULT hr = S_OK;

    if (!ppStyleSheet)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppStyleSheet = NULL;

    if ( lPos == _aStyleSheets.Size() )
        lPos = -1;

    if ( plNewPos )
        *plNewPos = -1;     // -1 means failed to create

    // If our level is full, then just don't create the requested stylesheet.
    if ( _aStyleSheets.Size() >= MAX_SHEETS_PER_LEVEL )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Create the stylesheet
    *ppStyleSheet = new CStyleSheet( pParentElem, _pSSARuleManager );
    if ( !*ppStyleSheet )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = AddStyleSheet(*ppStyleSheet, lPos, plNewPos);

    if (!hr)
        (*ppStyleSheet)->Release();       // Remove the extra one the Add put on to keep the count correct
Cleanup:
    if ( hr && ppStyleSheet && *ppStyleSheet )
    {
        delete *ppStyleSheet;
        *ppStyleSheet = NULL;
    }
    RRETURN( hr );
}


//*********************************************************************
//  CStyleSheetArray::AddStyleSheet()
//  This method tells the array to add the stylesheet to the array, like
//  CreateStyleSheet, but with an existing StyleSheet
//*********************************************************************
HRESULT CStyleSheetArray::AddStyleSheet( CStyleSheet * pStyleSheet, long lPos /* = -1 */, long *plNewPos /* = NULL */ )
{
    // lPos == -1 indicates append.
    HRESULT hr = S_OK;
    CStyleID id = _sidForOurSheets;      // id for new sheet
    long lValue;
    int t;
    int nRules, i;
    CStyleRuleArray *pRA;
    CStyleRule *pR;

    Assert ( pStyleSheet );
    Assert ( lPos >= -1 && lPos <= _aStyleSheets.Size() );

    if ( lPos == _aStyleSheets.Size() )
        lPos = -1;

    if ( plNewPos )
        *plNewPos = -1;     // -1 means failed to create

    // If our level is full, then just don't create the requested stylesheet.
    if ( _aStyleSheets.Size() >= MAX_SHEETS_PER_LEVEL )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Set value for current level of new sheet
    lValue = ((lPos == -1) ? _aStyleSheets.Size() : lPos) + 1;
    id.SetLevel( _Level, lValue );

    // Change the StyleSheet container (is a no-op when just created), including import sheets
    pStyleSheet->ChangeContainer(_pSSARuleManager);

    // Update all the rules for this StyleSheet with the new ID
    pStyleSheet->ChangeID(id);

    if (lPos == -1) // Append..
    {
        hr = _aStyleSheets.AppendIndirect( &pStyleSheet );
        // Return the index of the newly appended sheet
        if ( plNewPos )
            *plNewPos = _aStyleSheets.Size()-1;
    }
    else            // ..or insert
    {
        hr = _aStyleSheets.InsertIndirect( lPos, &pStyleSheet );
        if ( hr )
            goto Cleanup;

        // Return the index of the newly inserted sheet
        if ( plNewPos )
            *plNewPos = lPos;

        // Patch ids of all rules that will be shifted by new stylesheet's rules
        CStyleID sidLowerPatchBound = id;
        CStyleID sidUpperPatchBound = id;
        // Lower patch bound is the highest rule in the stylesheet immediately before us
        sidLowerPatchBound.SetLevel( _Level, lValue-1 );
        sidLowerPatchBound.SetRule( MAX_RULES_PER_SHEET );

        // Upper patch bound is the highest rule in the highest stylesheet at the same level
        // as the one being inserted
        sidUpperPatchBound.SetLevel( _Level, MAX_SHEETS_PER_LEVEL );
        sidUpperPatchBound.SetRule( MAX_RULES_PER_SHEET );

        // Now scan rules arrays for rules falling into the bounds.
        for (t=0 ; t < ETAG_LAST ; ++t)             // for all arrays..
        {
            pRA = &((_pSSARuleManager->_pRulesArrays)[t]);
            nRules = pRA->Size();
            for (i=0 ; i < nRules ; ++i)            // for all rules in this array
            {
                pR = (*pRA)[i];
                // If we hit a rule that needs patching.. do it!
                if ( pR->_sidRule <= sidUpperPatchBound && pR->_sidRule > sidLowerPatchBound )
                {
                    pR->_sidRule.SetLevel( _Level, pR->_sidRule.GetLevel(_Level)+1);
                }
            }
        }

        // Patch ids of sheets that got shifted up by the insertion.
        // Start patching at +1 from the insertion position.
        for ( ++lPos; lPos < _aStyleSheets.Size() ; ++lPos )
        {
            _aStyleSheets[lPos]->PatchID( _Level, lPos+1, FALSE  );
        }
    }

    // If the StyleSheet has existing rules, add them to the Rule Arrays
    hr = pStyleSheet->InsertExistingRules();
    pStyleSheet->AddRef();      // Reference for the Add or the Create

Cleanup:
    RRETURN( hr );
}

//*********************************************************************
//  CStyleSheetArray::ReleaseStyleSheet()
//  This method tells the array to release its reference to a stylesheet,
//  After this, as far as the array is concerned, the stylesheet does not
//  exist -- it no longer has a pointer to it, all other stylesheets have
//  their ids patched, and the rules of the released stylesheet are marked
//  as gone from the tree and have ids of 0.
//*********************************************************************
HRESULT CStyleSheetArray::ReleaseStyleSheet( CStyleSheet * pStyleSheet, BOOL fForceRender )
{
    HRESULT hr = E_FAIL;

    Assert( pStyleSheet );
    Assert( pStyleSheet->_sidSheet.FindNestingLevel() == _Level );

    long idx = _aStyleSheets.FindIndirect( &pStyleSheet );
    if (idx == -1) // idx == -1 if not found; e.g. an elem releasing its SS which has already been removed from SSC via OM.
        return hr;

    _aStyleSheets.Delete(idx);  // no return value

    Assert( pStyleSheet->_sidSheet.GetLevel( _Level ) == (unsigned long)(idx+1) );

    // Patch ids of remaining stylesheets (each SS that came after us has its level value reduced by 1)
    // Start patching at the idx where we just deleted (everyone was shifted down).
    while (idx < _aStyleSheets.Size())
    {
        _aStyleSheets[idx]->PatchID( _Level, idx+1, FALSE );
        ++idx;
    }

    // Make sure the appropriate rules get marked to reflect the fact this stylesheet is out of the
    // collection (a.k.a out of the tree).
    hr = pStyleSheet->ChangeStatus( CS_DETACHRULES, fForceRender, NULL );

    // Release that ref that used to be held by this collection/array
    pStyleSheet->Release();

    return hr;
}

//*********************************************************************
//  CStyleSheetArray::AddStyleRule()
//  Should only be called by CStyleSheet::AddStyleRule().  Since rules
//  are actually stored by CSSA's, this exposes that ability.
//*********************************************************************

HRESULT CStyleSheetArray::AddStyleRule( CStyleRule *pRule, BOOL fDefeatPrevious )
{
    Assert( "Must have rules array allocated to add rules!" && _pRulesArrays );
    return _pRulesArrays[ pRule->_pSelector->_eElementType ].InsertStyleRule( pRule, fDefeatPrevious );
}



CStyleSheet *
CStyleSheetArray::FindStyleSheetForSID(CStyleID sidTarget)
{
    unsigned long uLevel;
    unsigned long uWhichLevel;
    unsigned long uSheetTarget = sidTarget.GetSheet();

    CDataAry<CStyleSheet*> *pArray = &_aStyleSheets;
    CStyleSheet *pSheet;

    // For each level in the incoming SID
    for ( uLevel = 1 ;
        uLevel <= MAX_IMPORT_NESTING && pArray ;
        uLevel++ )
    {
        uWhichLevel = sidTarget.GetLevel(uLevel);

        Assert(uWhichLevel <= (unsigned long)pArray->Size());

        // Either we exactly match one in the current array at our index for this level
        if ( uWhichLevel && ((*pArray)[uWhichLevel-1])->_sidSheet.GetSheet() == uSheetTarget )
            return (*pArray)[uWhichLevel-1];
        // Or we need to dig into the imports array
        pSheet = (*pArray)[uWhichLevel];
        if ( !pSheet->_pImportedStyleSheets )
            break;
        pArray = &(pSheet->_pImportedStyleSheets->_aStyleSheets);
    }
    return NULL;
}



void
CachedStyleSheet::PrepareForCache (CStyleRule *pRule)
{
    _pRule = pRule;
}

LPTSTR
CachedStyleSheet::GetBaseURL(void)
{
    unsigned int    uSIdx;

    if (_pRule)
    {
        uSIdx = _pRule->GetID().GetSheet();
        if (!(_pCachedSS && _uCachedSheet == uSIdx))
        {
            _pCachedSS = _pssa->SheetInRule(_pRule);
            _uCachedSheet = uSIdx;
        }
        return _pCachedSS->_achAbsoluteHref;
    }
    else
    {
       return NULL;
    }

}


//*********************************************************************
//  CStyleSheetArray::Apply()
//      This method applies (in cascade order) all style rules in the
//  collection of all sheets in this Array that apply to this element
//  context to the formats passed in pStyleInfo.
//*********************************************************************
HRESULT CStyleSheetArray::Apply( CStyleInfo *pStyleInfo,
        ApplyPassType passType,
        EMediaType eMediaType,
        BOOL *pfContainsImportant /*=NULL*/ )
{
    HRESULT hr = S_OK;
    // Cache for class & ID on this element and potentially its parents (if they get walked)
    CStyleClassIDCache CIDCache;
    // rules applying to this type of tag
    CTreeNode *pNode = pStyleInfo->_pNodeContext;
    ELEMENT_TAG etag = pNode->TagType();
    CStyleRule **ppTagRules = (CStyleRule **)(_pRulesArrays[etag]);
    CStyleRule *pRule = NULL;
    int nTagRules = _pRulesArrays[etag].Size();
    // rules applying to any tags (based on class/id/etc.)
    CStyleRule **ppWildcardRules = (CStyleRule **)(_pRulesArrays[ ETAG_UNKNOWN ]);
    // If we _know_ there's no class/id on this elem, don't bother w/ wildcard rules
    int nWildcardRules = (CIDCache.GetClass(0) || CIDCache.GetID(0)) ?
        ((_pRulesArrays[ ETAG_UNKNOWN ]).Size()) : 0;
    CachedStyleSheet    cachedSS(this);

    // Move the pointers to the far end of the list
    ppTagRules += nTagRules - 1;
    ppWildcardRules += nWildcardRules - 1;

    while ( nTagRules || nWildcardRules )
    {
        if (nTagRules)
            pRule = *ppTagRules;

        // Walk back from end of the rules lists, looking for a rule that needs to be applied.
        while ( nTagRules && (!pRule->_pSelector ||
                        ( ( pRule->_dwFlags & (STYLERULEFLAG_NORENDER)) ||
            !(pRule->MediaTypeMatches( eMediaType )) ||
                        !pRule->_pSelector->Match( pNode, &CIDCache ) ) ) )
        {
            TraceTag((tagStyleSheetApply, "Check Tag Rule %08lX", pRule->_sidRule));
            nTagRules--;
            ppTagRules--;
            pRule = *ppTagRules;
        }

        if (nWildcardRules)
            pRule = *ppWildcardRules;

        while ( nWildcardRules && (!pRule->_pSelector ||
            ( (pRule->_dwFlags & (STYLERULEFLAG_NORENDER)) ||
            !(pRule->MediaTypeMatches( eMediaType )) ||
            !pRule->_pSelector->Match( pNode, &CIDCache ) ) ) )
        {
            TraceTag((tagStyleSheetApply, "Check Wildcard Rule %08lX", pRule->_sidRule));
            nWildcardRules--;
            ppWildcardRules--;
            pRule = *ppWildcardRules;
        }

        // When we get here, nTagRules and nWildcardRules index to rules that need to be applied.
        if ( nTagRules )
        {
            if ( nWildcardRules )
            {
                // If we get here, then we have a wildcard rule AND a tag rule that need to be applied.
                // BUGBUG: CWILSOBUG: This '>=' should eventually take source order into account.
                if ( (*ppTagRules)->_dwSpecificity >= (*ppWildcardRules)->_dwSpecificity )
                {
                    // If the specificity of the tag rule is greater or equal, apply the wildcard rule here,
                    // then we'll overwrite it by applying the tag rule later.
                    if ( (*ppWildcardRules)->_paaStyleProperties )
                    {
                        cachedSS.PrepareForCache(*ppWildcardRules);

                        TraceTag((tagStyleSheetApply, "Applying Wildcard Rule: %08lX to etag: %ls  id: %ls", 
                            (*ppWildcardRules)->_sidRule, pNode->_pElement->TagName(), STRVAL(pNode->_pElement->GetAAid())));

                        hr = THR( ApplyAttrArrayValues (
                            pStyleInfo,
                            &((*ppWildcardRules)->_paaStyleProperties),
                            &cachedSS,
                            passType,
                            pfContainsImportant ) );

                        if ( hr != S_OK )
                            break;
                    }
                    ppWildcardRules--;
                    nWildcardRules--;
                }
                else
                {
                    if ( (*ppTagRules)->_paaStyleProperties )
                    {
                        cachedSS.PrepareForCache(*ppTagRules);
                                        
                        TraceTag((tagStyleSheetApply, "Applying Tag Rule: %08lX to etag: %ls  id: %ls", 
                            (*ppTagRules)->_sidRule, pNode->_pElement->TagName(), STRVAL(pNode->_pElement->GetAAid())));

                        hr = THR( ApplyAttrArrayValues (
                            pStyleInfo,
                            &((*ppTagRules)->_paaStyleProperties),
                            &cachedSS,
                            passType,
                            pfContainsImportant ) );

                        if ( hr != S_OK )
                            break;
                    }
                    ppTagRules--;
                    nTagRules--;
                }
            }
            else
            {
                if ( (*ppTagRules)->_paaStyleProperties )
                {
                    cachedSS.PrepareForCache(*ppTagRules);
                                        
                    TraceTag((tagStyleSheetApply, "Applying Tag Rule: %08lX to etag: %ls  id: %ls", 
                        (*ppTagRules)->_sidRule, pNode->_pElement->TagName(), STRVAL(pNode->_pElement->GetAAid())));

                    hr = THR( ApplyAttrArrayValues (
                        pStyleInfo,
                        &((*ppTagRules)->_paaStyleProperties),
                        &cachedSS,
                        passType,
                        pfContainsImportant ) );

                    if ( hr != S_OK )
                        break;
                }
                ppTagRules--;
                nTagRules--;
            }
        }
        else if ( nWildcardRules )
        {
            if ( (*ppWildcardRules)->_paaStyleProperties )
            {
                cachedSS.PrepareForCache(*ppWildcardRules);
                                
                TraceTag((tagStyleSheetApply, "Applying Wildcard Rule: %08lX to etag: %ls  id: %ls", 
                    (DWORD) (*ppWildcardRules)->_sidRule, pNode->_pElement->TagName(), STRVAL(pNode->_pElement->GetAAid())));

                hr = THR( ApplyAttrArrayValues (
                    pStyleInfo,
                    &((*ppWildcardRules)->_paaStyleProperties),
                    &cachedSS,
                    passType,
                    pfContainsImportant ) );

                if ( hr != S_OK )
                    break;
            }
            ppWildcardRules--;
            nWildcardRules--;
        }
    }   // End of while(nTagRules || nWildcardRules) loop

    RRETURN( hr );
}

//*********************************************************************
//      CStyleSheetArray::TestForPseudoclassEffect()
//              This method checks all the style rules in this collection of
//  style sheets to see if a change in pseudoclass will change any
//  properties.
//*********************************************************************
BOOL CStyleSheetArray::TestForPseudoclassEffect(
    CElement *pElem,
    BOOL fVisited,
    BOOL fActive,
    BOOL fOldVisited,
    BOOL fOldActive )
{
    Assert( pElem && "NULL element!" );

    CDoc *pDoc = pElem->Doc();
    Assert( "No Document attached to this Site!" && pDoc );

    if ( pDoc->_pOptionSettings && !pDoc->_pOptionSettings->fUseStylesheets )
        return FALSE;   // Stylesheets are turned off.

    // Begin walking rules..

    // Cache for class and ID of this element and potentially its parents
    CStyleClassIDCache CIDCache;
    // rules applying to this type of tag
    CStyleRule **ppTagRules = (CStyleRule **)(_pRulesArrays[ pElem->Tag() ]);
    int nTagRules = (_pRulesArrays[ pElem->Tag() ]).Size();
    // rules applying to any tags (based on class/id/etc.)
    CStyleRule **ppWildcardRules = (CStyleRule **)(_pRulesArrays[ ETAG_UNKNOWN ]);
    // If we _know_ there's no class/id on this elem, don't bother w/ wildcard rules
    int nWildcardRules = (CIDCache.GetClass(0) || CIDCache.GetID(0)) ?
        ((_pRulesArrays[ ETAG_UNKNOWN ]).Size()) : 0;
    EPseudoclass eOldClass = pclassLink;
    EPseudoclass eNewClass = pclassLink;

    CTreeNode * pNodeContext = pElem->GetFirstBranch();

    // Set up the pseudoclass types
    if ( fActive )
        eNewClass = pclassActive;
    else if ( fVisited )
        eNewClass = pclassVisited;

    if ( fOldActive )
        eOldClass = pclassActive;
    else if ( fOldVisited )
        eOldClass = pclassVisited;

    // Move the pointers to the far end of the list
    ppTagRules += nTagRules - 1;
    ppWildcardRules += nWildcardRules - 1;

    while ( nTagRules )
    {
        if ( (*ppTagRules)->_pSelector )
        {
            if ( (*ppTagRules)->_pSelector->Match( pNodeContext, &CIDCache, &eNewClass ) ^
                 (*ppTagRules)->_pSelector->Match( pNodeContext, &CIDCache, &eOldClass ) )
                return TRUE;
        }
        nTagRules--;
        ppTagRules--;
    }
    while ( nWildcardRules )
    {
        if ( (*ppWildcardRules)->_pSelector )
        {
            if ( (*ppWildcardRules)->_pSelector->Match( pNodeContext, &CIDCache, &eNewClass ) ^
                 (*ppWildcardRules)->_pSelector->Match( pNodeContext, &CIDCache, &eOldClass ) )
                return TRUE;
        }
        nWildcardRules--;
        ppWildcardRules--;
    }

    return FALSE;
}

//*********************************************************************
//      CStyleSheetArray::ChangeRulesStatus()
//  fDetachedFromTree is TRUE when the source stylesheet of the rules is _out_ of the SSA (ReleaseStyleSheet())
//  Should only be called from CStyleSheet::ChangeStatus
//*********************************************************************

void CStyleSheetArray::ChangeRulesStatus( DWORD dwAction,
                                          BOOL fForceRender, BOOL *pFound, CStyleID sidSheet )
{
    Assert( "ChangeStyleRules() can only be called on SSA's that are rule containers!" && _pRulesArrays );
    Assert( "Can't both enable and detach from tree!" && (!((CS_ENABLERULES & dwAction) && (CS_CLEARRULES & dwAction))) );

    CStyleID sidLowerDisableBound = sidSheet;   //      >this range needs to be marked as disabled/detached
    CStyleID sidUpperDisableBound = sidSheet;   //      >    >
    CStyleID sidUpperPatchBound = sidSheet;     //           >this range needs to be patched when detaching

    // Lower ID bound for the range of IDs that need to be disabled is the highest rule in the
    // stylesheet one level below us.
    unsigned long l = sidLowerDisableBound.FindNestingLevel();
    unsigned long lv = sidLowerDisableBound.GetLevel( l );

    Assert ( l && l <= MAX_IMPORT_NESTING );
    Assert ( lv && lv <= MAX_SHEETS_PER_LEVEL );

    sidLowerDisableBound.SetLevel( l, lv-1 );
    sidLowerDisableBound.SetRule( MAX_RULES_PER_SHEET );

    // Upper disable bound is the highest rule in the stylesheet being disabled or detached
    sidUpperDisableBound.SetRule( MAX_RULES_PER_SHEET );

    // Upper patch bound is the highest rule in the highest stylesheet at the same level
    // as the one being detached
    sidUpperPatchBound.SetLevel( l, MAX_SHEETS_PER_LEVEL );
    sidUpperPatchBound.SetRule( MAX_RULES_PER_SHEET );

    // BUGBUG: We want to look at dwAction and do the right
    // ref-counting (once rule ref-counting has been decided).

    // Now scan rules arrays for rules falling into the bounds.
    int t;
    int nRules, i;
    CStyleRuleArray *pRA;
    CStyleRule *pR;
    BOOL fChanged = FALSE;
    for (t=0 ; t < ETAG_LAST ; ++t)             // for all arrays..
    {
        if ( (dwAction & CS_PATCHRULES) || pFound[t] )  // if we're patching rules, anyone could need to be updated
        {
            pRA = &(_pRulesArrays[t]);
            nRules = pRA->Size();
            if(nRules)
                 fChanged = TRUE;
            for (i=0 ; i < nRules ; ++i)            // for all rules in this array
            {
                pR = pRA->Item(i);
                // If the rule falls in the disable bounds, mark it appropriately
                if (pR->_sidRule <= sidUpperDisableBound && pR->_sidRule > sidLowerDisableBound)
                {
                    if ( MEDIATYPE(dwAction) )
                    {   // Need to patch media type.
                        pR->_dwFlags &= ~MEDIA_Bits;
                        pR->_dwFlags |= MEDIATYPE( dwAction );
                    }
                    if ( (dwAction & CS_ENABLERULES) )
                    {
                        pR->_dwFlags &= ~STYLERULEFLAG_DISABLED;
                    }
                    else
                    {
                        if ( (dwAction & CS_CLEARRULES) )
                        {
                            // Actually delete the rule!
                            // _pRulesArrays[t][i].Free( );
                            _pRulesArrays[t].Delete( i );

                            // Now, make sure we don't skip the next one (which just shifted into
                            // our space), or run off the end of the newly shortened array.
                            nRules--;
                            i--;
                        }
                        else
                            pR->_dwFlags |= STYLERULEFLAG_DISABLED;
                    }
                }
                // If we are detaching rules, and we hit a rule that needs patching.. do it!
                else if ( (dwAction & CS_PATCHRULES) && pR->_sidRule <= sidUpperPatchBound && pR->_sidRule > sidUpperDisableBound )
                {
                    pR->_sidRule.SetLevel(l, pR->_sidRule.GetLevel(l)-1);
                }
            }
        }
    }

    // Force update of element formats to account for new set of rules
    if ( fForceRender && fChanged  && _pSSARuleManager->_pOwner)
    {
        Assert( _pSSARuleManager == this );

        CMarkup *pMarkup = DYNCAST(CMarkup, _pSSARuleManager->_pOwner);  // rule manager's owner is always the markup

        if (pMarkup)
            IGNORE_HR( pMarkup->OnCssChange(/*fStable = */ TRUE, /* fRecomputePeers = */ TRUE) );
    }
}

//*********************************************************************
//      CStyleSheetArray::ShiftRules()
//  This function is used to either patch holes in the rules numbering
//  scheme for a stylesheet, or make room for a new rule.  If we are
//  removing a rule, the rule entry is actually freed and deleted.
//*********************************************************************
void CStyleSheetArray::ShiftRules(
        ERulesShiftAction eAction,      // Are we removing or inserting?
        BOOL fForceRender,              // Should we force a format recalc?
        BOOL *pContainsChangedRule,     // Array of ETAG_LAST BOOLs that tell us
                                        // whether to look in that bucket or not
        CStyleID sidRule )              // The ID of the first rule to shift up
                                        // (when inserting) or the rule we're
                                        // removing (when deleting)
{
    int t;
    int nRules, i;
    CStyleRuleArray *pRA;
    CStyleRule *pR;
    CStyleID sidUpperPatchBound = sidRule;

    Assert( "ChangeStyleRules() can only be called on SSA's that are rule containers!" && _pRulesArrays );

    sidUpperPatchBound.SetRule( MAX_RULES_PER_SHEET );

    // Now scan rules arrays for rules falling into the bounds.
    for ( t = 0; t < ETAG_LAST; ++t )             // for all rule arrays..
    {
        if ( !pContainsChangedRule[t] )
            continue;

        pRA = &(_pRulesArrays[t]);
        nRules = pRA->Size();
        for ( i = 0; i < nRules ; ++i)            // for all rules in this array
        {
            pR = pRA->Item(i);

            // If the rule falls in the disable bounds, mark it appropriately
            if ( pR->_sidRule < sidUpperPatchBound && pR->_sidRule >= sidRule )
            {
                switch ( eAction )
                {
                case ruleRemove:
                    if ( pR->_sidRule == sidRule )
                    {
                        // Actually delete the rule!
                        // _pRulesArrays[t][i].Free(  );
                        _pRulesArrays[t].Delete( i );

                        // Now, make sure we don't skip the next one (which just shifted into
                        // our space), or run off the end of the newly shortened array.
                        nRules--;
                        i--;
                    }
                    else
                        pR->_sidRule.SetRule( pR->_sidRule.GetRule() - 1 );
                    break;

                case ruleInsert:
                    Assert( pR->_sidRule.GetRule() < MAX_RULES_PER_SHEET ); // Shouldn't happen, we'll throw
                                                                            // an assert and fail in AddRule.
                    pR->_sidRule.SetRule( pR->_sidRule.GetRule() + 1 );
                }
            }
        }
    }

    // Force update of element formats to account for new set of rules
    if ( fForceRender && _pSSARuleManager->_pOwner)
    {
        Assert( _pSSARuleManager == this );

        CMarkup *pMarkup = DYNCAST(CMarkup, _pSSARuleManager->_pOwner);  // rule manager's owner is always the markup

        if (pMarkup)
            IGNORE_HR( pMarkup->Doc()->ForceRelayout() );
    }
}

//*********************************************************************
//      CStyleSheetArray::Get()
//  Acts like the array operator.
//*********************************************************************
CStyleSheet * CStyleSheetArray::Get( long lIndex )
{
    if (lIndex < 0 || lIndex >= _aStyleSheets.Size())
    {
        return NULL;
    }

    // BUGBUG: AddRef here or require caller to addref?
    return _aStyleSheets[ lIndex ];
}

CStyleRule * const CStyleSheetArray::GetRule( ELEMENT_TAG eTag, CStyleID ruleID )
{
    int i;

    for ( i = 0; i < _pRulesArrays[eTag].Size(); i++ )
    {
        if ( (DWORD)(_pRulesArrays[eTag][i]->GetID()) == (DWORD)ruleID )
            return (_pRulesArrays[eTag][i]);
    }
    return NULL;
}


//*********************************************************************
//  CStyleSheetArray::length
//      IHTMLStyleSheetsCollection interface method
//*********************************************************************

HRESULT
CStyleSheetArray::get_length(long * pLength)
{
    HRESULT hr = S_OK;

    if (!pLength)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pLength = _aStyleSheets.Size();

Cleanup:
    RRETURN( SetErrorInfo( hr ));

}

//*********************************************************************
//  CStyleSheetArray::item
//      IHTMLStyleSheetsCollection interface method.  This overload is
//  not exposed via the PDL; it's purely internal.  Automation clients
//  like VBScript will access the overload that takes variants.
//*********************************************************************

HRESULT
CStyleSheetArray::item(long lIndex, IHTMLStyleSheet** ppHTMLStyleSheet)
{
    HRESULT   hr;

    if (!ppHTMLStyleSheet)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppHTMLStyleSheet = NULL;

    // Just exit if access is out of bounds.
    if (lIndex < 0 || lIndex >= _aStyleSheets.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = _aStyleSheets[lIndex]->QueryInterface(IID_IHTMLStyleSheet, (void**)ppHTMLStyleSheet);

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

HRESULT
CStyleSheetArray::item(VARIANTARG *pvarArg1, VARIANTARG * pvarRes)
{
    HRESULT             hr = S_OK;
    long                lIndex;
    IHTMLStyleSheet     *pHTMLStyleSheet;
    CVariant            cvarArg;

    if (!pvarRes)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // Clear return value
    VariantInit (pvarRes);

    if (VT_EMPTY == V_VT(pvarArg1))
    {
        Assert("Don't know how to deal with this right now!" && FALSE);
        goto Cleanup;
    }

    // first attempt ordinal access...
    hr = THR(cvarArg.CoerceVariantArg(pvarArg1, VT_I4));
    if (hr==S_OK)
    {
        lIndex = V_I4(&cvarArg);

        // Just exit if access is out of bounds.
        if (lIndex < 0 || lIndex >= _aStyleSheets.Size())
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        hr = _aStyleSheets[lIndex]->QueryInterface(IID_IHTMLStyleSheet, (void **)&pHTMLStyleSheet);
        if (hr)
            goto Cleanup;
    }
    else
    {
        // not a number so try a name
        hr = THR_NOTRACE(cvarArg.CoerceVariantArg(pvarArg1, VT_BSTR));
        if (hr)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        else
        {
            // its a string, so handle named access
            if ( _Level != 1 )
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }

            lIndex = FindSSByHTMLID( (LPTSTR)V_BSTR(pvarArg1), FALSE ); // not case sensitive for VBScript
            if ( lIndex == -1 )
            {
                hr = DISP_E_MEMBERNOTFOUND;
                goto Cleanup;
            }

            hr = _aStyleSheets[lIndex]->QueryInterface(IID_IHTMLStyleSheet, (void **)&pHTMLStyleSheet);
            if (hr)
                goto Cleanup;

        }
    }

    V_VT(pvarRes) = VT_DISPATCH;
    V_DISPATCH(pvarRes) = pHTMLStyleSheet;

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheetArray::_newEnum
//      IHTMLStyleSheetsCollection interface method
//*********************************************************************

HRESULT
CStyleSheetArray::get__newEnum(IUnknown ** ppEnum)
{
    HRESULT hr = S_OK;

    if (!ppEnum)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppEnum = NULL;

    hr = THR(_aStyleSheets.EnumVARIANT(VT_DISPATCH,
                                      (IEnumVARIANT**)ppEnum,
                                      FALSE,
                                      FALSE));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheetArray::FindSSByHTMLID
//  Searches the array for a stylesheet whose parent element has
//  the specified HTML ID.  Returns index if found, -1 if not found.
//*********************************************************************

long
CStyleSheetArray::FindSSByHTMLID( LPCTSTR pszID, BOOL fCaseSensitive )
{
    HRESULT   hr;
    CElement *pElem;
    BSTR      bstrID;
    long      lIdx;

    for ( lIdx = 0 ; lIdx < _aStyleSheets.Size() ; ++lIdx )
    {
        pElem = (_aStyleSheets[lIdx])->GetParentElement();
        Assert( "Must always have parent element!" && pElem );
        // BUGBUG perf: more efficient way to get id?
        hr = pElem->get_PropertyHelper( &bstrID, (PROPERTYDESC *)&s_propdescCElementid );
        if ( hr )
            return -1;
        if ( !(fCaseSensitive ? _tcscmp( pszID, bstrID ) : _tcsicmp( pszID, bstrID )) )
        {
            FormsFreeString(bstrID);
            return lIdx;
        }
        FormsFreeString(bstrID);
    }
    return -1;
}


