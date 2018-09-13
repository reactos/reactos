//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       sheets.cxx
//
//  Contents:   Support for Cascading Style Sheets.. including:
//
//              CStyleSelector
//              CStyleRule
//              CStyleSheet
//              CStyleID
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_CBUFSTR_HXX_
#define X_CBUFSTR_HXX_
#include "cbufstr.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif


#ifndef X_RULESCOL_HXX_
#define X_RULESCOL_HXX_
#include "rulescol.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_HTMTAGS_HXX_
#define X_HTMTAGS_HXX_
#include "htmtags.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EANCHOR_HXX_
#define X_EANCHOR_HXX_
#include "eanchor.hxx"  // For CAnchorElement decl, for pseudoclasses
#endif

#ifndef X_ELINK_HXX_
#define X_ELINK_HXX_
#include "elink.hxx"    // for CLinkElement
#endif

#ifndef X_ESTYLE_HXX_
#define X_ESTYLE_HXX_
#include "estyle.hxx"   // for CStyleElement
#endif

#ifndef X_FONTFACE_HXX_
#define X_FONTFACE_HXX_
#include "fontface.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_ATBLOCKS_HXX_
#define X_ATBLOCKS_HXX_
#include "atblocks.hxx"
#endif


// BUGBUG: Right now we do a certain amount of dyncasting of _pParentElement to
// either CLinkElement or CStyleElement.  Consider developing an ABC (CStyleSheetElement?)
// that exposes the stylesheet support for elements (e.g. _pStyleSheet, SetReadyState() etc)?
// Might be more trouble than it's worth..

#define _cxx_
#include "sheet.hdl"

MtDefine(StyleSheets, Mem, "StyleSheets")
MtDefine(CStyleSheet, StyleSheets, "CStyleSheet")
MtDefine(CNamespace, CStyleSheet, "CStyleSheet::_pDefaultNamespace")
MtDefine(CStyleSheet_apRulesList_pv, CStyleSheet, "CStyleSheet::_apRulesList::_pv")
MtDefine(CStyleSheetArray, StyleSheets, "CStyleSheetArray")
MtDefine(CStyleSheetArray_aStyleSheets_pv, CStyleSheetArray, "CStyleSheetArray::_aStyleSheets::_pv")
MtDefine(CStyleSheetArray_apFontFaces_pv, CStyleSheetArray, "CStyleSheetArray::_apFontFaces::_pv")
MtDefine(CStyleRule, StyleSheets, "CStyleRule")
MtDefine(CStyleRuleArray, StyleSheets, "CStyleRuleArray")
MtDefine(CStyleRuleArray_pv, CStyleRuleArray, "CStyleRuleArray::_pv")
MtDefine(CStyleSelector, StyleSheets, "CStyleSelector")
MtDefine(CStyleClassIDCache, StyleSheets, "CStyleClassIDCache")
MtDefine(CachedStyleSheet, StyleSheets, "CachedStyleSheet")
MtDefine(CStyleSheetAddImportedStyleSheet_pszParsedURL, Locals, "CStyleSheet::AddImportedStyleSheet pszParsedURL")
MtDefine(CStyleSheetChangeStatus_pFound, Locals, "CStyleSheet::ChangeStatus pFound")


DeclareTag(tagStyleSheet,                    "Style Sheet", "trace Style Sheet operations")

//---------------------------------------------------------------------
//  Class Declaration:  CStyleSelector
//      This class implements a representation of a stylesheet selector -
//  that is, a particular tag/attribute situation to match a style rule.
//---------------------------------------------------------------------

//*********************************************************************
//  CStyleSelector::CStyleSelector()
//      The constructor for the CStyleSelector class initializes all
//  member variables.
//*********************************************************************
#ifdef XMV_PARSE
CStyleSelector::CStyleSelector( const TCHAR *pcszSelectorString, CStyleSelector *pParent, 
                                 BOOL fXMLGeneric )
#else
CStyleSelector::CStyleSelector( const TCHAR *pcszSelectorString, CStyleSelector *pParent)
#endif
{
    _pParent = pParent;
    _pSibling = NULL;
    _eElementType = ETAG_UNKNOWN;
    _ePseudoclass = pclassNone;
    _ePseudoElement = pelemNone;

    if ( pcszSelectorString )
#ifdef XMV_PARSE
        Init( pcszSelectorString, fXMLGeneric);
#else
        Init( pcszSelectorString);
#endif
}

//*********************************************************************
//  CStyleSelector::~CStyleSelector()
//*********************************************************************
CStyleSelector::~CStyleSelector( void )
{
    delete _pParent;
    delete _pSibling;
}

//*********************************************************************
//  CStyleSelector::Init()
//      This method parses a string selector and initializes all internal
//  member data variables.
//*********************************************************************
#ifdef XMV_PARSE
HRESULT CStyleSelector::Init( const TCHAR *pcszSelectorString, BOOL fXMLGeneric )
#else
HRESULT CStyleSelector::Init( const TCHAR *pcszSelectorString)
#endif
{
    HRESULT hr = S_OK;
    const TCHAR *pcszBegin;
    const TCHAR *pcszNext;

    Assert( pcszSelectorString != NULL );

    pcszNext = pcszBegin = pcszSelectorString;

    while ( *pcszBegin )
    {
        pcszNext = pcszBegin + 1;   // Need to skip over any special characters

        // Get the next token
        while ( *pcszNext && (*pcszNext != _T('#')) && (*pcszNext != _T(':')) && (*pcszNext != _T('.')))
        {
            if(*pcszNext == _T('\\') && *(pcszNext + 1) == _T(':'))
                break;
            pcszNext++;
        }

        if ( *pcszBegin == _T('*') )
        {
            if(_Nmsp.IsEmpty())
                _eElementType = ETAG_UNKNOWN;      // Wildcard
            else
                _eElementType = ETAG_GENERIC;
            _cstrTagName.Set (pcszBegin, pcszNext - pcszBegin);
        }
        else   if ( _istalnum( *pcszBegin ) )
        {   // tag name
#ifdef XMV_PARSE
            if (fXMLGeneric)
            {
                _eElementType = ETAG_NULL;
                if ((*pcszNext == _T('\\')) && (!_tcsnicmp( _T("HTML"), 4, pcszBegin, (int)(pcszNext - pcszBegin))))
                {
                    // from the html namespace
                    // just skip past the HTML\: and continue the normal HTML parse
                    fXMLGeneric = FALSE;
                    pcszBegin = pcszNext + 2;
                    continue;
                }
            }
            else
#endif
                _eElementType = EtagFromName ( pcszBegin, pcszNext - pcszBegin );

            if (_eElementType == ETAG_NULL || !_Nmsp.IsEmpty() || *pcszNext == _T('\\') )
            {
                _eElementType = ETAG_GENERIC;
                if(*pcszNext == _T('\\'))
                {
                    // The selector has scope\:name syntax
                    _Nmsp.SetShortName(pcszBegin, pcszNext - pcszBegin);
                    // + 2 to skip "\:"
                    pcszBegin = pcszNext + 2;
                    continue; 
                } 
                _cstrTagName.Set (pcszBegin, pcszNext - pcszBegin);
            }
        }
        else if ( *pcszBegin == _T(':') )
        {   // Pseudoclass or Pseudoelement
            pcszBegin++;
            if ( !_tcsnicmp( _T("active"), 6, pcszBegin, (int)(pcszNext - pcszBegin) ) )
                _ePseudoclass = pclassActive;
            else if ( !_tcsnicmp( _T("visited"), 7, pcszBegin, (int)(pcszNext - pcszBegin) ) )
                _ePseudoclass = pclassVisited;
            else if ( !_tcsnicmp( _T("hover"), 5, pcszBegin, (int)(pcszNext - pcszBegin) ) )
                _ePseudoclass = pclassHover;
            else if ( !_tcsnicmp( _T("link"), 4, pcszBegin, (int)(pcszNext - pcszBegin) ) )
                _ePseudoclass = pclassLink;
            else if ( !_tcsnicmp( _T("first-letter"), 12, pcszBegin, (int)(pcszNext - pcszBegin) ) )
                _ePseudoElement = pelemFirstLetter;
            else if ( !_tcsnicmp( _T("first-line"), 10, pcszBegin, (int)(pcszNext - pcszBegin) ) )
                _ePseudoElement = pelemFirstLine;
            else
            {
                // Unrecognized Pseudoclass/Pseudoelement name!
                _ePseudoElement = pelemUnknown;
            }
        }
        else if ( *pcszBegin == _T('#') )
        {   // ID
            pcszBegin++;
            _cstrID.Set( pcszBegin, pcszNext - pcszBegin );
        }
        else if ( *pcszBegin == _T('.') )
        {   // Class
            pcszBegin++;
            _cstrClass.Set( pcszBegin, pcszNext - pcszBegin );
        }
        else
        {
            // Unrecognized token in selector string!
            hr = S_FALSE;
        }

        pcszBegin = pcszNext;
    }
    RRETURN1( hr, S_FALSE );
}

//*********************************************************************
//  CStyleSelector::GetSpecificity()
//      This method computes the cascade order specificity from the
//  number of tagnames, IDs, classes, etc. in the selector.
//*********************************************************************
DWORD CStyleSelector::GetSpecificity( void )
{
    DWORD dwRet = 0;

    if ( _eElementType != ETAG_UNKNOWN )
        dwRet += SPECIFICITY_TAG;
    if ( _cstrClass.Length() )
        dwRet += SPECIFICITY_CLASS;
    if ( _cstrID.Length() )
        dwRet += SPECIFICITY_ID;
    switch ( _ePseudoclass )
    {
    case pclassActive:
    case pclassVisited:
    case pclassHover:
        dwRet += SPECIFICITY_PSEUDOCLASS;
        //Intentional fall-through
    case pclassLink:
        dwRet += SPECIFICITY_PSEUDOCLASS;
        break;
    }
    switch ( _ePseudoElement )
    {
    case pelemFirstLetter:
        dwRet += SPECIFICITY_PSEUDOELEMENT;
        //Intentional fall-through
    case pelemFirstLine:
        dwRet += SPECIFICITY_PSEUDOELEMENT;
        break;
    }

    if ( _pParent )
        dwRet += _pParent->GetSpecificity();

    return dwRet;
}


//*********************************************************************
//  CStyleSelector::MatchSimple()
//      This method compares a simple selector with an element.
//*********************************************************************
BOOL CStyleSelector::MatchSimple( CElement *pElement, CStyleClassIDCache *pCIDCache, int iCacheSlot, EPseudoclass *pePseudoclass )
{
    // If that element tag doesn't match what's specified in the selector, then NO match.
    if ( ( _eElementType != ETAG_UNKNOWN ) && ( _eElementType != pElement->TagType() ) )
        return FALSE;

    LPCTSTR pszClass;
    LPCTSTR pszID;

    // if an extended tag, match scope name and tag name
    if ((_eElementType == ETAG_GENERIC))
    {
        // CONSIDER: the _tcsicmp-s below is a potential perf problem. Use atom table for the
        // scope name and tag name to speed this up.
        if(0 != _tcsicmp(pElement->NamespaceHtml(), _Nmsp.GetNamespaceString()))
            // Namespaces don't match
            return FALSE;
        if(0 != _tcsicmp(_T("*"), _cstrTagName) &&                       // not a wild card and
             0 != _tcsicmp(pElement->TagName(), _cstrTagName))           //   tag name does not match
            return FALSE;                                                // then report no match
    }

    INT nLen = _cstrClass.Length();
    // Only worry about classes on the element if the selector has a class
    if ( nLen )
    {
        pszClass = pCIDCache->GetClass(iCacheSlot);
        // No match if the selector has a class but the element doesn't.
        if ( !pszClass )
            return FALSE;
        // If we don't know the class of the element, get it now.
        if (pszClass == UNKNOWN_CLASS_OR_ID)
        {
            HRESULT hr;

            hr = pElement->GetStringAt(
                    pElement->FindAAIndex(DISPID_CElement_className, CAttrValue::AA_Attribute),
                    &pszClass);

            if (S_OK != hr || !pszClass)
                return FALSE;

            pCIDCache->PutClass(pszClass, iCacheSlot);
        }

        CDataListEnumerator classNames ( pszClass );

        LPCTSTR pszThisClass = NULL; INT nThisLength = 0;

        // Allow for comma,seperated ClassName
        while ( classNames.GetNext ( &pszThisClass, &nThisLength ) )
        {
            if ( nThisLength && !_tcsnicmp ( (LPTSTR)_cstrClass, nLen, pszThisClass, nThisLength ) )
                goto CompareIDs;
        }
        return FALSE;
    }

CompareIDs:
    // Only worry about ids on the element if the selector has an id
    if ( _cstrID.Length() )
    {
        pszID = pCIDCache->GetID(iCacheSlot);
        // No match if the selector has an ID but the element doesn't.
        if ( !pszID )
            return FALSE;
        // If we don't know the ID of the element, get it now.
        if (pszID == UNKNOWN_CLASS_OR_ID)
        {
            if ( S_OK != pElement->GetStringAt( pElement->FindAAIndex( DISPID_CElement_id, CAttrValue::AA_Attribute ),
                &pszID) || !pszID )
                return FALSE;
            pCIDCache->PutID(pszID, iCacheSlot);
        }
        if ( _tcsicmp( _cstrID, pszID ) )
            return FALSE;
    }

    if ( _ePseudoclass != pclassNone )
    {
        AAINDEX idx;

        if ( pElement->TagType() != ETAG_A )    // BUGBUG: Eventually, we should allow other hyperlink types here, like form submit buttons or LINKs.
            return FALSE;                       // When we do, change the block below as well.

        if ( pePseudoclass )
        {
            if ( *pePseudoclass == _ePseudoclass )
                return TRUE;
            else
                return FALSE;
        }

        if ( !*(pElement->GetAttrArray()) )
            return FALSE;

                                                // The following is the block that has to change:
        idx = pElement->FindAAIndex( DISPID_CAnchorElement_href, CAttrValue::AA_Attribute );
        if ( idx == AA_IDX_UNKNOWN )
            return FALSE;   // No HREF - must be a target anchor, not a source anchor.

        CAnchorElement *pAElem = DYNCAST( CAnchorElement, pElement );
        EPseudoclass psc = pAElem->IsVisited() ? pclassVisited : pclassLink;

        // Hover and Active are applied in addition to either visited or link
        // Hover is ignored if anchor is active.
        if (    _ePseudoclass == psc
            ||  (pAElem->IsActive()  && _ePseudoclass == pclassActive)
            ||  (pAElem->IsHovered() && _ePseudoclass == pclassHover) )
            return TRUE;
        return FALSE;
    }

    if ( _ePseudoElement != pelemNone )
        return FALSE;   // BUGBUG: need to do pseudoelements here
    return TRUE;
}

//*********************************************************************
//  CStyleSelector::Match()
//      This method compares a contextual selector with an element context.
//*********************************************************************
BOOL CStyleSelector::Match( CTreeNode * pNode, CStyleClassIDCache *pCIDCache, EPseudoclass *pePseudoclass /*=NULL*/ )
{
    CStyleSelector *pCurrSelector = this;
    int iCacheSlot = 0;

    // Cache slot 0 stores the original elem's class/id
    if ( !pCurrSelector->MatchSimple( pNode->Element(), pCIDCache, iCacheSlot, pePseudoclass ) )
        return FALSE;

    pCurrSelector = pCurrSelector->_pParent;
    pNode = pNode->Parent();
    ++iCacheSlot;

    // We matched the innermost part of the contextual selector.  Now walk up
    // the remainder of the contextual selector (if it exists), testing whether our
    // element's containers satisfy the remainder of the contextual selector.  MatchSimple()
    // stores our containers' class/id in cache according to their containment level.
    while ( pCurrSelector && pNode )
    {
        if ( pCurrSelector->MatchSimple( pNode->Element(), pCIDCache, iCacheSlot, pePseudoclass ) )
            pCurrSelector = pCurrSelector->_pParent;

        pNode = pNode->Parent();
        ++iCacheSlot;
    }
    if ( !pCurrSelector )
        return TRUE;
    return FALSE;
}

HRESULT CStyleSelector::GetString( CStr *pResult )
{
    Assert( pResult );

    if ( _pParent )     // This buys us the context selectors.
        _pParent->GetString( pResult );

    switch ( _eElementType )
    {
    case ETAG_UNKNOWN:  // Wildcard - don't write tag name.
        break;
    case ETAG_NULL:     // Unknown tag name - write it out as "UNKNOWN".
        pResult->Append( _T("UNKNOWN") );
        break;
    case ETAG_GENERIC:     // Peer
        if(!_Nmsp.IsEmpty())
        {
            pResult->Append(_Nmsp.GetNamespaceString());
            pResult->Append(_T("\\:"));
        }
        pResult->Append(_cstrTagName);
        break;
    default:
        pResult->Append( NameFromEtag( _eElementType ) );
        break;
    }

    if ( _cstrClass.Length() )
    {
        pResult->Append( _T(".") );
        pResult->Append( _cstrClass );
    }
    if ( _cstrID.Length() )
    {
        pResult->Append( _T("#") );
        pResult->Append( _cstrID );
    }
    switch ( _ePseudoclass )
    {
    case pclassActive:
        pResult->Append( _T(":active") );
        break;
    case pclassVisited:
        pResult->Append( _T(":visited") );
        break;
    case pclassLink:
        pResult->Append( _T(":link") );
        break;
    case pclassHover:
        pResult->Append( _T(":hover") );
        break;
#ifdef DEBUG
    default:
        Assertsz(0, "Unknown pseudoclass");
#endif
    }
    switch ( _ePseudoElement )
    {
    case pelemFirstLetter:
        pResult->Append( _T(":first-letter") );
        break;
    case pelemFirstLine:
        pResult->Append( _T(":first-line") );
        break;
    case pelemUnknown:
        pResult->Append( _T(":unknown") );
        break;
#ifdef DEBUG
    default:
        Assertsz(0, "Unknown pseudoelement");
#endif
    }
    pResult->Append( _T(" ") );
    return S_OK;
}

//---------------------------------------------------------------------
//  Class Declaration:  CStyleRule
//      This class represents a single rule in the stylesheet - a pairing
//  of a selector (the situation) and a style (the collection of properties
//  affected in that situation).
//---------------------------------------------------------------------

//*********************************************************************
//  CStyleRule::CStyleRule()
//*********************************************************************
CStyleRule::CStyleRule( CStyleSelector *pSelector )
{
    _dwSpecificity = 0;
    _pSelector = NULL;
    if ( pSelector )
        SetSelector( pSelector );
    _paaStyleProperties = NULL;
    _sidRule = 0;
    _dwFlags = 0;
}

//*********************************************************************
//  CStyleRule::~CStyleRule()
//*********************************************************************
CStyleRule::~CStyleRule()
{
    // Make sure we don't die while still attached to a selector
    Assert( (_pSelector == NULL) || (_pSelector == (CStyleSelector*)(LONG_PTR)(-1)) );
}

//*********************************************************************
//  CStyleRule::Free()
//      This method deletes all members of CStyleRule.
//*********************************************************************
void CStyleRule::Free( void )
{
    if ( _pSelector )
    {
        delete( _pSelector );
        _pSelector = NULL;
    }

    if ( _paaStyleProperties )
    {
        delete( _paaStyleProperties );
        _paaStyleProperties  = NULL;
    }
#ifdef DBG
    _pSelector = (CStyleSelector *)(LONG_PTR)(-1);
    _paaStyleProperties = (CAttrArray *)(LONG_PTR)(-1);
#endif
}

//*********************************************************************
//  CStyleRule::SetSelector()
//      This method sets the selector used for this rule.  Note that
//  this method should only be called once on any given CStyleRule object.
//*********************************************************************
void CStyleRule::SetSelector( CStyleSelector *pSelector )
{
    Assert( "Selector is already set for this rule!" && !_pSelector );
    Assert( "Can't set a NULL selector!" && pSelector );

    _pSelector = pSelector;
    _dwSpecificity = pSelector->GetSpecificity();
}

HRESULT CStyleRule::GetString( CBase *pBase, CStr *pResult )
{
    HRESULT hr;
    BSTR bstr;

    Assert( pResult );

    if ( !_pSelector )
        return E_FAIL;
    _pSelector->GetString( pResult );

    pResult->Append( _T("{\r\n\t") );

    hr = WriteStyleToBSTR( pBase, _paaStyleProperties, &bstr, FALSE );
    if ( hr != S_OK )
        goto Cleanup;

    if ( bstr )
    {
        if ( *bstr )
            pResult->Append( bstr );
        FormsFreeString( bstr );
    }

    pResult->Append( _T("\r\n}\r\n") );
Cleanup:
    RRETURN(hr);
}


HRESULT CStyleRule::GetMediaString(DWORD dwCurMedia, CBufferedStr *pstrMediaString)
{
    HRESULT     hr = S_OK;
    int         i;
    BOOL        fFirst = TRUE;

    Assert(dwCurMedia != (DWORD)MEDIA_NotSet);

    dwCurMedia = dwCurMedia & (DWORD)MEDIA_Bits;

    if(dwCurMedia == (DWORD)MEDIA_All)
    {
        AssertSz(cssMediaTypeTable[0]._mediaType == MEDIA_All, "MEDIA_ALL must me element 0 in the array");
        hr = THR(pstrMediaString->Set(cssMediaTypeTable[0]._szName));
        goto Cleanup;
    }

    for(i = 1; i < ARRAY_SIZE(cssMediaTypeTable); i++)
    {
        if(cssMediaTypeTable[i]._mediaType & dwCurMedia)
        {
            if(fFirst)
            {
                pstrMediaString->Set(NULL);
                fFirst = FALSE;
            }
            else
            { 
                hr = THR(pstrMediaString->QuickAppend(_T(", ")));
            }
            if(hr)
                break;
            hr = THR(pstrMediaString->QuickAppend(cssMediaTypeTable[i]._szName));
            if(hr)
                break;
        }
    }

Cleanup:
    RRETURN(hr);
}


//*********************************************************************
//  CStyleRule::GetNamespace
//      Returns the namespace name that this rule belongs, NULL if none
//*********************************************************************
const CNamespace *
CStyleRule::GetNamespace() const
{
    Assert(_pSelector); 
    return _pSelector->GetNamespace();
}


//+---------------------------------------------------------------------------
//
// CStyleRuleArray.
//
//----------------------------------------------------------------------------
//*********************************************************************
//  CStyleRuleArray::Free()
//      This method deletes all members of CStyleRuleArray.
//*********************************************************************
void CStyleRuleArray::Free( )
{
    // Just forget about the Rules, they will be freed by the Style Sheet
    DeleteAll();
}

//*********************************************************************
//  CStyleRuleArray::InsertStyleRule()
//      This method adds a new rule to the rule array, putting it in
//  order in the stylesheet.  Note that the rules are kept in order of
//  DESCENDING preference (most important rules first) - this is due to
//  the mechanics of our Apply functions.
//*********************************************************************
HRESULT CStyleRuleArray::InsertStyleRule( CStyleRule *pNewRule, BOOL fDefeatPrevious )
{
    int z;
    CStyleRule *pRule;

    Assert( "Style Rule is NULL!" && pNewRule );
    Assert( "Style Selector is NULL!" && pNewRule->_pSelector );
    Assert( "Style ID isn't set!" && (pNewRule->_sidRule != 0) );

    // Sort by specificity, then by source order (i.e. rule ID).  Note that
    // rule ID of 0 indicates a dead rule, and we want to skip by them for
    // insertion purposes.

    for ( z = 0; z < Size(); ++z)
    {
        pRule = Item(z);
        if (pRule)
        {
            if ( fDefeatPrevious )
            {
                if ( pRule->_dwSpecificity == pNewRule->_dwSpecificity )
                {
                    if ( pRule->_sidRule && (pRule->_sidRule < pNewRule->_sidRule) )
                    {
                        break;
                    }
                }
                else if ( pRule->_dwSpecificity < pNewRule->_dwSpecificity )
                    break;
            }
            else
            {
                if ( pRule->_dwSpecificity < pNewRule->_dwSpecificity )
                {
                    if ( pRule->_sidRule && (pRule->_sidRule < pNewRule->_sidRule) )
                    {
                        break;
                    }
                }
            }
        }
    }


    return Insert( z, pNewRule );
}

//---------------------------------------------------------------------
//  Class Declaration:  CStyleSheet
//      This class represents a complete stylesheet.  It (will) hold a
//  list of ptrs to style rules, sorted by source order within the sheet.
//  Storage for the rules is managed by the stylesheet's containing array
//  (a CStyleSheetArray), which sorts them by tag and specificity
//  for fast application to the data.
//---------------------------------------------------------------------

const CStyleSheet::CLASSDESC CStyleSheet::s_classdesc =
{
    {
        &CLSID_HTMLStyleSheet,               // _pclsid
        0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
        NULL,                                // _apClsidPages
#endif // NO_PROPERTY_PAGE
        NULL,                                // _pcpi
        0,                                   // _dwFlags
        &IID_IHTMLStyleSheet,                // _piidDispinterface
        &s_apHdlDescs                        // _apHdlDesc
    },
    (void *)s_apfnIHTMLStyleSheet                    // _apfnTearOff
};

//*********************************************************************
//  CStyleSheet::CStyleSheet()
//  You probably don't want to be calling this yourself to create style
//  sheet objects -- use CStyleSheetArray::CreateNewStyleSheet().
//
//  BUGBUG: Make the constructor private and declare CStyleSheetArray
//  a friend?  But then we lose access protection from CSSA..
//*********************************************************************
CStyleSheet::CStyleSheet(
    CElement *pParentElem,
    CStyleSheetArray * const pSSAContainer)
    :
    _pParentElement(pParentElem),
    _pParentStyleSheet(NULL),
    _pImportedStyleSheets(NULL),
    _pSSAContainer(pSSAContainer),
    _sidSheet(0),
    _fDisabled(FALSE),
    _pBitsCtx(NULL),
    _nExpectedImports(0),
    _nCompletedImports(0),
    _eParsingStatus(CSSPARSESTATUS_NOTSTARTED),
    _dwStyleCookie(0),
    _dwScriptCookie(0),
    _pOMRulesArray(NULL),
    _achAbsoluteHref(NULL)
{
    Assert( "Stylesheet must have container!" && _pSSAContainer );
    // Stylesheet starts internally w/ ref count of 1, and subrefs its parent.
    // This maintains consistency with the addref/release implementations.
    if (_pParentElement)
        _pParentElement->SubAddRef();
    _eMediaType = MEDIA_All;
    _eLastAtMediaType = MEDIA_NotSet;
}

CStyleSheet::~CStyleSheet()
{
    // This will free the Imports because ins ref counts are always one
    //  (it ref counts its parent for other purposes
    if(_pImportedStyleSheets)
        _pImportedStyleSheets->CBase::PrivateRelease(); 
}

//*********************************************************************
//  CStyleSheet::Passivate()
//*********************************************************************
void CStyleSheet::Passivate()
{
    // The mucking about with _pImportedStyleSheets in Free()
    // will subrel us, which _may_ cause self-destruction.
    // To get around this, we subaddref ourselves and then subrelease.
    SubAddRef();
    Free();
    SubRelease();

    MemFreeString(_achAbsoluteHref);
    _achAbsoluteHref = NULL;

    // Perform CBase passivation last - we need access to the SSA container.
    super::Passivate();
 }

//*********************************************************************
//  CStyleSheet::Free()
//  Storage for the stylesheet's rules is managed by the containing array,
//  so all the stylesheet itself is responsible for is any imported style
//  sheets.  It may be advisable to take over management of our own rules?
//*********************************************************************
void CStyleSheet::Free( void )
{
    SetBitsCtx(NULL);
    if ( _pImportedStyleSheets )
    {
        _pImportedStyleSheets->Free( );  // Force our stylesheets collection to release its
                                            // refs on imported stylesheets.
        _pImportedStyleSheets->Release();   // this will subrel us

    }
    _pParentElement = NULL;
    _pParentStyleSheet = NULL;

    if ( _pOMRulesArray )
        _pOMRulesArray->StyleSheetRelease();
    _pOMRulesArray = NULL;
    int idx = _apRulesList.Size();
    while ( idx )
    {
        if ( _apRulesList[ idx-1 ].pAutomationRule )
        {
            _apRulesList[ idx-1 ].pAutomationRule->StyleSheetRelease();
            _apRulesList[ idx-1 ].pAutomationRule = NULL;
        }

        _apRulesList[ idx-1]._pRule->Free();
        delete _apRulesList[ idx-1]._pRule;
        idx--;
    }
    _apRulesList.DeleteAll();

    // Free any font faces.
    CStyleSheetArray *pContainer = GetRootContainer();
    if ( pContainer )
    {
        int n = pContainer->_apFontFaces.Size();
        int i;
        CFontFace *pFace;

        for( i=0; i < n; )
        {
            pFace = ((CFontFace **)(pContainer->_apFontFaces))[ i ];
            if ( pFace->ParentStyleSheet() == this )
            {
                pFace->PrivateRelease();
                pContainer->_apFontFaces.Delete( i );
                n--;
            }
            else
                i++;
        }
    }
}

//*********************************************************************
//      CStyleSheet::PrivateQueryInterface()
//*********************************************************************
HRESULT
CStyleSheet::PrivateQueryInterface(REFIID iid, void **ppv)
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
                iid == *pclassdesc->_classdescBase._piidDispinterface)
            {
                HRESULT hr = THR(CreateTearOffThunk(this, (void *)(pclassdesc->_apfnTearOff), NULL, ppv));
                if (hr)
                    RRETURN(hr);
            }
        }
    }

    if (*ppv)
    {
        Assert( _pParentElement );
        if ( !_pParentStyleSheet && ( _pParentElement->Tag() == ETAG_STYLE ) )
        {
            CStyleElement *pStyle = DYNCAST( CStyleElement, _pParentElement );
            pStyle->SetDirty(); // Force us to build from our internal representation for persisting.
        }
        (*(IUnknown**)ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG CStyleSheet::PrivateAddRef( void )
{
    if (_pParentElement)
        _pParentElement->SubAddRef();
    return CBase::PrivateAddRef();
}

ULONG CStyleSheet::PrivateRelease( void )
{
    if (_pParentElement)
        _pParentElement->SubRelease();
    return CBase::PrivateRelease();
}

CStyleSheetArray *CStyleSheet::GetRootContainer( void )
{
    return _pSSAContainer ? _pSSAContainer->_pSSARuleManager : NULL;
}

CDoc *CStyleSheet::GetDocument( void )
{
    return _pParentElement->Doc();
}

CMarkup *CStyleSheet::GetMarkup( void )
{
    return _pParentElement->GetMarkup();
}

//*********************************************************************
//  CStyleSheet::AddStyleRule()
//      This method adds a new rule to the correct CStyleRuleArray in
//  the hash table (hashed by element (tag) number) (The CStyleRuleArrays
//  are stored in the containing CStyleSheetArray).  This method is
//  responsible for splitting apart selector groups and storing them
//  as separate rules.  May also handle important! by creating new rules.
//
//  We also maintain a list in source order of the rules inserted by
//  this stylesheet -- the list has the rule ID (rule info only, no
//  import nesting info) and etag (no pointers)
//
//  NOTE:  If there are any problems, the CStyleRule will auto-destruct.
//*********************************************************************
HRESULT CStyleSheet::AddStyleRule( CStyleRule *pRule, BOOL fDefeatPrevious /*=TRUE*/, long lIdx /*=-1*/ )
{
    HRESULT hr = S_OK;
    CStyleSelector *pNextSelector;
    CRuleEntry re;

    Assert( "Style Rule is NULL!" && pRule );
    Assert( "Style Selector is NULL!" && pRule->_pSelector );
    Assert( "Stylesheet must have a container!" && _pSSAContainer );
    Assert( "Style ID for StyleSheet has not been set" && _sidSheet );

    if (pRule->_paaStyleProperties)
    {
        //
        // If the rule has the behavior attribute set, turn on the flag on the doc
        // which forever enables behaviors.
        //

        // Only do the Find if PeersPossible is currently not set
        if (_pParentElement && !_pParentElement->Doc()->_fPeersPossible)
            if (pRule->_paaStyleProperties->Find(DISPID_A_BEHAVIOR))
                _pParentElement->Doc()->SetPeersPossible();
    }
    
    do
    {
        CStyleRule *pSiblingRule = NULL;

        pNextSelector = pRule->_pSelector->_pSibling;
        pRule->_pSelector->_pSibling = NULL;

        if ( _apRulesList.Size() >= MAX_RULES_PER_SHEET )
        {
            hr = E_INVALIDARG;
            break;
        }

        // Set the appropriate flags on the rule based on the stylesheet's state
        if (_fDisabled) {
            pRule->_dwFlags |= STYLERULEFLAG_DISABLED;
        }

        pRule->_dwFlags |= _eMediaType;

        pRule->_dwAtMediaTypeFlags = GetLastAtMediaTypeValue();

        // Set the style id for this rule.  Rules #'s start at 1 for orthogonality
        // with level #'s, though technically they could start at 0.
        pRule->_sidRule = _sidSheet;        // add the sheet level fields

        if ( ( lIdx < 0 ) || ( lIdx >= _apRulesList.Size() ) )   // Add at the end
        {
            lIdx = _apRulesList.Size();
            pRule->_sidRule.SetRule( lIdx + 1 );
        }
        else
        {
            CRuleEntry *pRE;
            BOOL abFound[ ETAG_LAST ];
            int i, nRules;

            memset( abFound, 0, sizeof(BOOL) * ETAG_LAST );
            pRule->_sidRule.SetRule( lIdx + 1 );

            for ( pRE = (CRuleEntry *)_apRulesList + lIdx, i = lIdx, nRules = _apRulesList.Size();
                  i < nRules; i++, pRE++ )
            {
                abFound[ (int)(pRE->_eTag) ] = TRUE;
                if ( pRE->pAutomationRule )
                {   // Need to fix up automation rule's sid.
                    pRE->pAutomationRule->_dwID++;
                }
            }

            _pSSAContainer->ShiftRules( CStyleSheetArray::ruleInsert, FALSE, abFound, pRule->_sidRule );
        }

#if DBG==1
    if (IsTagEnabled(tagStyleSheet))
    {
        CStr        strRule;
        CHAR        szBuffer[2000];

        pRule->GetString(this, &strRule);
        WideCharToMultiByte(CP_ACP, 0, strRule, -1, szBuffer, sizeof(szBuffer), NULL, NULL);

        TraceTag((tagStyleSheet, "AddStyleRule  SheetID: %08lX  RuleID: %08lX  TagName: %s  Rule: %s", 
            (DWORD) _sidSheet, (DWORD) pRule->_sidRule, pRule->_pSelector->_cstrTagName, szBuffer));
    }
#endif

        // _Track_ the rule in our internal list.  This list lets us enumerate in
        // source order the rules that we've added.  It makes a copy of the rule
        // entry, so it's OK for re to be on the stack.

        re._pRule = pRule;
        re._eTag = pRule->_pSelector->_eElementType;
        re.pAutomationRule = NULL;
        if ( ( lIdx < 0 ) || ( lIdx >= _apRulesList.Size() ) )   // Add at the end
            _apRulesList.AppendIndirect( &re );
        else
            _apRulesList.InsertIndirect( lIdx, &re );

        // _Store_ the rule in our container
        hr = _pSSAContainer->AddStyleRule( pRule, fDefeatPrevious );
        if ( hr != S_OK )
            break;
        if ( pNextSelector )
        {
            pSiblingRule = new CStyleRule( pNextSelector );
            if ( !pSiblingRule )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            // Copy the recognized style properties from the original rule's attr array
            pSiblingRule->_paaStyleProperties = NULL;
            if ( pRule->_paaStyleProperties )
                hr = pRule->_paaStyleProperties->Clone( &(pSiblingRule->_paaStyleProperties) );
            if ( hr != S_OK )
                break;

            // Copy any unknown properties
            //hr = pRule->_uplUnknowns.Duplicate( pSiblingRule->_uplUnknowns );
            //if ( hr != S_OK )
            //  break;
        }

        if ( pNextSelector )
            pRule = pSiblingRule;

    } while ( pNextSelector );

Cleanup:
    RRETURN( hr );
}


//*********************************************************************
//  CStyleSheet::AddImportedStyleSheet()
//      This method adds an imported stylesheet to our imported list
//  (which is created if necessary), and kicks off a download of the
//  imported stylesheet.
//*********************************************************************
HRESULT CStyleSheet::AddImportedStyleSheet( TCHAR *pszURL, long lPos /*=-1*/, long *plNewPos /*=NULL*/ )
{
    CStyleSheet *pStyleSheet = NULL;
    CBitsCtx *pBitsCtx = NULL;
    HRESULT hr = E_FAIL;

    // Do not support imported SS in User SS as there is no doc!
    if (!_pParentElement)
        goto Cleanup;

    if ( plNewPos )
        *plNewPos = -1;     // -1 means failed to add sheet

    // Bail if we're max'ed out on nesting.
    if ((long)_sidSheet.FindNestingLevel() >= MAX_IMPORT_NESTING)
    {
        Assert( "Maximum import nesting exceeded! (informational)" && FALSE );
        goto Cleanup;
    }

    // Check if our URL is a CSS url(xxx) string.  If so, strip "url(" off front,
    // and ")" off back.  Otherwise assume it'a a straight URL.
    // This function modifies the parameter string pointer
    hr = RemoveStyleUrlFromStr(&pszURL);
    if(FAILED(hr))
        goto Cleanup;
    
    hr = E_FAIL;

    // The imports array could already exist because of previous @imports,
    // or because the imports collection was previously requested from script.
    if ( !_pImportedStyleSheets )
    {
        // Imported stylesheets don't manage their own rules.
        _pImportedStyleSheets = new CStyleSheetArray( this, _pSSAContainer, _sidSheet );
        Assert( "Failure to allocate imported stylesheets array! (informational)" && _pImportedStyleSheets );
        if (!_pImportedStyleSheets)
            goto Cleanup;
        if (_pImportedStyleSheets->_fInvalid)
        {
            _pImportedStyleSheets->CBase::PrivateRelease();
            goto Cleanup;
        }
    }

    // Create the stylesheet in the "imported array".  Note that its rules will be kept in the
    // root doc's CSSA since when we created the "imported array" above, we designated a container
    hr = _pImportedStyleSheets->CreateNewStyleSheet( _pParentElement, &pStyleSheet, lPos, plNewPos );
    if ( hr == S_OK )
    {
        // Fix up new SS to reflect the fact that it's imported (give it parent, set its import href)
        pStyleSheet->_pParentStyleSheet = this;
        pStyleSheet->_cstrImportHref.Set(pszURL);

        // Kick off the download of the imported sheet
        if ( pszURL && pszURL[0])
        {
            CDoc *  pDoc = _pParentElement->Doc();

            Assert(!pStyleSheet->_achAbsoluteHref && "absoluteHref already computed.");
            TCHAR *pAbsURL;

            if(pStyleSheet->_pParentStyleSheet->_achAbsoluteHref)
            {
                hr = ExpandUrlWithBaseUrl(pStyleSheet->_pParentStyleSheet->_achAbsoluteHref,
                                          pszURL,
                                          &pStyleSheet->_achAbsoluteHref);
                if (hr)
                    goto Cleanup;
                pAbsURL = pStyleSheet->_achAbsoluteHref;
            }
            else
            {
                pAbsURL = pszURL;
            }

            Assert(pAbsURL);

            // We will track this @import statement.
            ++_nExpectedImports;
            hr = THR(pDoc->NewDwnCtx(DWNCTX_BITS, pAbsURL, _pParentElement,
                                    (CDwnCtx **)&pBitsCtx));
            if (hr == S_OK)
            {
                // For rendering purposes, having an @imported sheet pending is just like having
                // a linked sheet pending.
                pDoc->EnterStylesheetDownload(&(pStyleSheet->_dwStyleCookie));
                _pParentElement->GetMarkup()->EnterScriptDownload(&(pStyleSheet->_dwScriptCookie));

                // Give ownership of bitsctx to the newly created (empty) stylesheet, since it's
                // the one that will need to be filled in by the @import'ed sheet.
                pStyleSheet->SetBitsCtx(pBitsCtx);
                pBitsCtx->Release();
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//*********************************************************************
//  CStyleSheet::ChangeStatus()
//  Enable or disable this stylesheet and its imported children
//*********************************************************************
HRESULT CStyleSheet::ChangeStatus(
    DWORD dwAction,               // CS_ flags as defined in sheets.hxx
    BOOL fForceRender /*=TRUE*/,  // should we force everyone to update their formats and re-render?
                                  // We want to avoid forcing a re-render when the doc/tree is unloading/dying etc.
    BOOL *pFound /*=NULL*/)     // array of bools used to track which tags have rules to be disabled
                                // Should only be non-NULL on recursive calls (ext. callers use NULL)
{
    CRuleEntry *pRE;
    int nRules;
    int z;
    BOOL bRootStyleSheet = FALSE;
    CStyleSheet *pSS;

    // Allocate array if we're the first call and we're not detaching.
    if (!pFound)
    {
        if (!(dwAction & CS_PATCHRULES))
        {
            pFound = new(Mt(CStyleSheetChangeStatus_pFound)) BOOL[ ETAG_LAST ];
            if (!pFound)
                return E_OUTOFMEMORY;
            memset(pFound, 0, sizeof(BOOL)*ETAG_LAST);
        }
        else
        {
            pFound = (BOOL *)(-1);
        }
        bRootStyleSheet = TRUE;
    }

    // Mark this stylesheet's status
    _fDisabled = !(dwAction & CS_ENABLERULES);

    // Scan rules directly specified by this SS and mark the appropriate bool
    if (!(dwAction & CS_PATCHRULES))
    {
        for (pRE=(CRuleEntry *)_apRulesList, z=0, nRules=_apRulesList.Size() ; z < nRules ; ++z, ++pRE)
        {
            pFound[ (int)(pRE->_eTag) ] = TRUE;
        }
    }

    // Recursively scan rules of our imports
    if (_pImportedStyleSheets)
    {
        for ( z=0 ; (pSS=(_pImportedStyleSheets->Get(z))) != NULL ; ++z)
        {
            pSS->ChangeStatus( dwAction, fForceRender, pFound );
        }
    }

    if ( MEDIATYPE(dwAction) )
    {   // Need to set media type.
        _eMediaType = (EMediaType)MEDIATYPE( dwAction );
    }

    if (bRootStyleSheet) {
        _pSSAContainer->ChangeRulesStatus( dwAction, fForceRender, pFound, _sidSheet );
        if (!(dwAction & CS_PATCHRULES))
            delete [] pFound;
    }

    return S_OK;
}

//*********************************************************************
//  CStyleSheet::InsertExistingRules()
//
//  When a StyleSheet moves to another StyleSheetArray, this method adds
//  the rules that the StyleSheet has into that new Array
//*********************************************************************
HRESULT
CStyleSheet::InsertExistingRules()
{
    HRESULT         hr = S_OK;
    int             i;
    CStyleRule *    pRule;
    CStyleSheet *   pSS;

    for (i = 0; i < _apRulesList.Size(); i++)
    {
        pRule = _apRulesList[i]._pRule;
        Assert(pRule);

        hr = THR(_pSSAContainer->AddStyleRule(pRule, TRUE));
        if (hr)
            goto Cleanup;
    }

    if (_pImportedStyleSheets)
    {
        for ( i = 0 ; (pSS=(_pImportedStyleSheets->Get(i))) != NULL ; ++i)
        {
            hr = THR(pSS->InsertExistingRules());
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}


//*********************************************************************
//  CStyleSheet::ChangeContainer()
//
//  When a StyleSheet moves to another StyleSheetArray, this method changes
//  the container StyleSheetArray, including the one for imported sheets
//*********************************************************************
void    
CStyleSheet::ChangeContainer(CStyleSheetArray * pSSANewContainer)
{
    int             z;
    CStyleSheet *   pSS;

    Assert(pSSANewContainer);
    _pSSAContainer = pSSANewContainer;

    if (_pImportedStyleSheets)
    {
        for ( z=0 ; (pSS=(_pImportedStyleSheets->Get(z))) != NULL ; ++z)
        {
            pSS->ChangeContainer(pSSANewContainer);
        }
    }
}

MtDefine(LoadFromURL, Utilities, "CStyleSheet::LoadFromURL");

//*********************************************************************
//  CStyleSheet::LoadFromURL()
//  Marks as "dead" any rules that this stylesheet currently has, and
//  loads a new set of rules from the specified URL.  Because of our
//  ref-counting scheme, if this sheet has an imports collection allocated,
//  that collection object is reused (i.e. any existing imported SS are
//  released).
//*********************************************************************
HRESULT CStyleSheet::LoadFromURL( const TCHAR *pszURL, BOOL fAutoEnable /*=FALSE*/ )
{
    HRESULT     hr;
    CBitsCtx    *pBitsCtx = NULL;

    BOOL fDisabled = _fDisabled;    // remember our current disable value.

    // Force all our rules (and rules of our imports) to be marked as out of the tree ("dead"),
    // but don't patch other rules to fill in the ID hole.
    // Don't force a re-render since we will immediately be loading new rules.
    hr = ChangeStatus( CS_CLEARRULES, FALSE, NULL );   // disabling, detached from tree, no re-render
    if ( hr )
        goto Cleanup;

    // Note: the ChangeStatus call above will have set us as disabled.  Restore our original disable value.
    _fDisabled = fAutoEnable? FALSE : fDisabled;

    // If we have an imports collection, clear it out, but keep the collection
    // object itself (i.e. don't release _pImportedStyleSheets).
    if (_pImportedStyleSheets)
    {
        _pImportedStyleSheets->Free( );
    }

    // Clear our local rules tracking list
    CStyleRule *    pRule;
    int             i;
    for (i = 0; i < _apRulesList.Size(); i++)
    {
        pRule = _apRulesList[i]._pRule;
        pRule->Free();
        delete pRule;
    }
    _apRulesList.DeleteAll();

    // Clear our readystate information:
    _eParsingStatus = CSSPARSESTATUS_PARSING;
    _nExpectedImports = 0;
    _nCompletedImports = 0;

    if(_achAbsoluteHref)
    {
        MemFreeString(_achAbsoluteHref);
        _achAbsoluteHref = NULL;
    }

    // That's all the cleanup we need; our parent element, parent stylesheet, sheet ID etc.
    // stay the same!


    // Kick off the download of the URL
    if ( pszURL && pszURL[0] )
    {
        CDoc *  pDoc = _pParentElement->Doc();
        TCHAR   cBuf[pdlUrlLen];

        hr = pDoc->ExpandUrl(pszURL, ARRAY_SIZE(cBuf), cBuf, NULL);
        if (hr)
            goto Cleanup;
        MemAllocString(Mt(LoadFromURL), cBuf, &_achAbsoluteHref);
        if (_achAbsoluteHref == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pDoc->NewDwnCtx(DWNCTX_BITS, pszURL,
                    _pParentElement, (CDwnCtx **)&pBitsCtx));
        if (hr == S_OK)
        {
            // Block rendering while we load..
            pDoc->EnterStylesheetDownload(&_dwStyleCookie);
            pDoc->PrimaryMarkup()->EnterScriptDownload(&_dwScriptCookie);
            if ( IsAnImport() )
                (_pParentStyleSheet->_nCompletedImports)--;

            // We own the bits context..
            SetBitsCtx(pBitsCtx);
        }
    }
    else
    {
        SetBitsCtx( NULL );
        Fire_onerror();
        CheckImportStatus();
    }

Cleanup:
    if (pBitsCtx)
        pBitsCtx->Release();

    RRETURN( hr );
}

//*********************************************************************
//  CStyleSheet::PatchID()
//  Patches the ID of this stylesheet and all nested imports
//*********************************************************************

void CStyleSheet::PatchID(
    unsigned long ulLevel,       // Level that will change
    unsigned long ulValue,       // Value level will be given
    BOOL fRecursiveCall)        // Is this a recursive call? (FALSE for ext. callers).
{
    Assert (ulLevel > 0 && ulLevel <= MAX_IMPORT_NESTING );
    Assert (ulValue >= 0 && ulValue <= MAX_SHEETS_PER_LEVEL );

    long i = 0;
    CStyleSheet *pISS;

    // Fix our own ID
    _sidSheet.SetLevel( ulLevel, ulValue );
    Assert( _sidSheet );    // should never become 0

    // Everyone nested below the sheet for which PatchID was first called needs
    // to be patched at the same level with a value that's 1 less.
    if (!fRecursiveCall)
    {
        fRecursiveCall = TRUE;
        --ulValue;
    }

    // If we have imports, ask them to fix all of their IDs.
    if ( _pImportedStyleSheets )
    {
        // Fix ID that imports collection will use to build new imports
        _pImportedStyleSheets->_sidForOurSheets.SetLevel( ulLevel, ulValue );
        // Recursively fix imported stylesheets
        while ( (pISS = _pImportedStyleSheets->Get(i++)) != NULL )
        {
            pISS->PatchID( ulLevel, ulValue, fRecursiveCall );
        }
    }
}

//*********************************************************************
//  CStyleSheet::ChangeID()
//  Changes the ID for this StyleSheet and all the rules
//*********************************************************************
void CStyleSheet::ChangeID(CStyleID const idNew)
{
    int             i;
    CStyleRule *    pRule;
    CStyleID        idNewLevel;
    CStyleSheet *   pSS;

    // Clear the low bits, as we only want the level numbers
    idNewLevel = idNew & ~RULE_MASK;

    // Change all the Rules
    // Assume that the rules are *NOT* in a RuleArray, as that won't be updated with these changes
    for (i = 0; i < _apRulesList.Size(); i++)
    {
        pRule = _apRulesList[i]._pRule;
        Assert(pRule);

        pRule->_sidRule = (pRule->_sidRule & RULE_MASK) | idNewLevel; 
    }

    // Change the Imported sheets
    if (_pImportedStyleSheets)
    {
        for ( i = 0 ; (pSS=(_pImportedStyleSheets->Get(i))) != NULL ; ++i)
        {
            pSS->ChangeID(idNew);
        }
    }

    // Update our ID
    _sidSheet = idNew;
}

//*********************************************************************
//  CStyleSheet::SetBitsCtx()
//  Sets ownership and callback information for a BitsCtx.  A stylesheet
//  will have a non-NULL BitsCtx if it's @import'ed.
//*********************************************************************
void CStyleSheet::SetBitsCtx(CBitsCtx * pBitsCtx)
{    
    if (_pBitsCtx)
    {   // If we're tromping on an in-progress download, fix the completed count up.

        if (!_fComplete)
        {
            if (IsAnImport())
                (_pParentStyleSheet->_nCompletedImports)++;
            _pParentElement->Doc()->LeaveStylesheetDownload(&_dwStyleCookie);
        }
        _pBitsCtx->SetProgSink(NULL); // detach download from document's load progress
        _pBitsCtx->Disconnect();
        _pBitsCtx->Release();
    }

    _fComplete = FALSE;
    _pBitsCtx = pBitsCtx;

    if (pBitsCtx)
    {
        pBitsCtx->AddRef();

        pBitsCtx->AddRef();     // Keep ourselves alive
        SetReadyState( READYSTATE_LOADING );

        if ( pBitsCtx == _pBitsCtx )    // Make sure we're still the bitsctx for this stylesheet -
        {                               // it's possible SetReadyState fired and changed the bitsctx.
            if (pBitsCtx->GetState() & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED))
                OnDwnChan(pBitsCtx);
            else
            {
                pBitsCtx->SetProgSink(_pParentElement->Doc()->GetProgSink());
                pBitsCtx->SetCallback(OnDwnChanCallback, this);
                pBitsCtx->SelectChanges(DWNCHG_COMPLETE, 0, TRUE);
            }
        }

        pBitsCtx->Release();     // Stop keeping ourselves alive
    }
}

//*********************************************************************
//  CStyleSheet::OnDwnChan()
//  Callback used by BitsCtx once it's downloaded the @import'ed
//  stylesheet data that will be used to fill us out.  Also used when
//  the HREF on a linked stylesheet changes (we reuse the CStyleSheet
//  object and setup a new bitsctx on it)
//*********************************************************************
void CStyleSheet::OnDwnChan(CDwnChan * pDwnChan)
{
    ULONG ulState = _pBitsCtx->GetState();
    CMarkup *pMarkup;
    CDoc *pDoc;

    Assert(_pParentElement);
    _pParentElement->EnsureInMarkup();

    pMarkup = _pParentElement->GetMarkup();
    Assert(pMarkup);

    pDoc = pMarkup->Doc();

    if ( IsAnImport() )
        _fDisabled = _pParentStyleSheet->_fDisabled;

    if ( ulState & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED) )
    {
        _fComplete = TRUE;
        
        pDoc->LeaveStylesheetDownload(&_dwStyleCookie);

        // This sheet has finished, one way or another.
        if ( IsAnImport() )
            _pParentStyleSheet->_nCompletedImports++;

        if (ulState & DWNLOAD_COMPLETE)
        {
            IStream * pStream;

            // If unsecure download, may need to remove lock icon on Doc
            pDoc->OnSubDownloadSecFlags(_pBitsCtx->GetUrl(), _pBitsCtx->GetSecFlags());
        
            if ( S_OK == _pBitsCtx->GetStream(&pStream) )
            {
#ifdef XMV_PARSE
                CCSSParser parser( this, NULL, IsXML() );
#else
                CCSSParser parser( this, NULL );
#endif

                parser.LoadFromStream( pStream, pDoc->GetCodePage() );
                pStream->Release();
            }
            else
            {
                _eParsingStatus = CSSPARSESTATUS_DONE;   // Need to make sure we'll walk up to our parent in CheckImportStatus()
                TraceTag((tagError, "CStyleSheet::OnChan bitsctx failed to get file!"));
                Fire_onerror();
            }
        }
        else
        {
            _eParsingStatus = CSSPARSESTATUS_DONE;      // Need to make sure we'll walk up to our parent in CheckImportStatus()
            TraceTag((tagError, "CStyleSheet::OnChan bitsctx failed to complete!"));
            if ( ulState & DWNLOAD_ERROR )
                Fire_onerror();
        }

        // Relayout the doc; new rules may have been loaded (e.g. DWNLOAD_COMPLETE),
        // or we may have killed off rules w/o re-rendering before starting the load
        // for this sheet (e.g. changing src of an existing sheet).
        IGNORE_HR( pMarkup->OnCssChange( /*fStable = */ TRUE, /* fRecomputePeers = */ TRUE) );

        CheckImportStatus();    // Needed e.g. if all imports were cached, their OnChan's would all be called
                                // before parsing finished.

        pMarkup->LeaveScriptDownload(&_dwScriptCookie);
        
        _pBitsCtx->SetProgSink(NULL); // detach download from document's load progress
    }
    else
    {
        Assert( "Unknown result returned from CStyleSheet's bitsCtx!" && FALSE );
    }
}

//*********************************************************************
//  CStyleSheet::Fire_onerror()
//      Handles firing onerror on our parent element.
//*********************************************************************
void CStyleSheet::Fire_onerror()
{
    if ( !_pParentElement )
        return;    // In case we're a user stylesheet

    if (_pParentElement->Tag() == ETAG_STYLE)
    {
        CStyleElement *pStyle = DYNCAST( CStyleElement, _pParentElement );
        pStyle->Fire_onerror();
    }
    else
    {
        Assert( _pParentElement->Tag() == ETAG_LINK );
        CLinkElement *pLink = DYNCAST( CLinkElement, _pParentElement );
        pLink->Fire_onerror();
    }
}

#ifdef XMV_PARSE
//*********************************************************************
//  CStyleSheet::IsXML()
//      Says whether this stylesheet should follow xml generic parsing rules
//*********************************************************************
BOOL CStyleSheet::IsXML(void)
{
    // the parent element may have been detached, is there a problem here?
    return _pParentElement && _pParentElement->IsInMarkup() && _pParentElement->GetMarkupPtr()->IsXML();
}
#endif

//*********************************************************************
//  CStyleSheet::SetReadyState()
//      Handles passing readystate changes to our parent element, which
//  may cause our parent element to fire the onreadystatechange event.
//*********************************************************************
HRESULT CStyleSheet::SetReadyState( long readyState )
{
    if ( !_pParentElement )
        return S_OK;    // In case we're a user stylesheet

    if (_pParentElement->Tag() == ETAG_STYLE)
    {
        CStyleElement *pStyle = DYNCAST( CStyleElement, _pParentElement );
        return pStyle->SetReadyStateStyle( readyState );
    }
    else
    {
        Assert( _pParentElement->Tag() == ETAG_LINK );
        CLinkElement *pLink = DYNCAST( CLinkElement, _pParentElement );
        return pLink->SetReadyStateLink( readyState );
    }
}

//*********************************************************************
//  CStyleSheet::CheckImportStatus()
//  Checks whether all our imports have come in, and notify our parent
//  if necessary.  This ultimately includes causing our parent element
//  to fire events.
//*********************************************************************
void CStyleSheet::CheckImportStatus( void )
{
    // If all stylesheets nested below us are finished d/ling..
    if ( _eParsingStatus != CSSPARSESTATUS_PARSING && (_nExpectedImports == _nCompletedImports) )
    {
        if ( IsAnImport() )
        {
            // If we've hit a break in our parentSS chain, just stop..
            if ( !IsDisconnectedFromParentSS() )
            {
                // Notify our parent that we are finished.
                _pParentStyleSheet->CheckImportStatus();
            }
        }
        else
        {
            // We are a top-level SS!  Since everything below us is
            // finished, we can fire.
            SetReadyState( READYSTATE_COMPLETE );
        }
    }
}

//*********************************************************************
//  CStyleSheet::StopDownloads
//      Halt all downloading of stylesheets, including all nested imports.
//*********************************************************************
void CStyleSheet::StopDownloads( BOOL fReleaseBitsCtx )
{
    long z;
    CStyleSheet *pSS;

    if ( _pBitsCtx )
    {
        CMarkup *pMarkup;

        _pBitsCtx->SetProgSink(NULL); // detach download from document's load progress
        _pBitsCtx->SetLoad( FALSE, NULL, FALSE );

        pMarkup = _pParentElement->GetMarkup();
        if(pMarkup)
            pMarkup->Doc()->LeaveStylesheetDownload(&_dwStyleCookie);
        
         // This sheet has finished, one way or another.
         if ( IsAnImport() )
             _pParentStyleSheet->_nCompletedImports++;
         _eParsingStatus = CSSPARSESTATUS_DONE;   // Need to make sure we'll walk up to our parent in CheckImportStatus()

        if ( fReleaseBitsCtx )
            SetBitsCtx(NULL);
    }


    if ( _pImportedStyleSheets )
    {
        for ( z=0 ; (pSS=(_pImportedStyleSheets->Get(z))) != NULL ; ++z)
            pSS->StopDownloads( fReleaseBitsCtx );
    }

#ifndef NO_FONT_DOWNLOAD
    // If we initiated any downloads of embedded fonts, stop those too
    CStyleSheetArray *pContainer = GetRootContainer();
    if ( pContainer )
    {
        int n = pContainer->_apFontFaces.Size();
        int i;
        CFontFace *pFace;

        for( i=0; i < n; i++)
        {
            pFace = ((CFontFace **)(pContainer->_apFontFaces))[ i ];
            if ( pFace->ParentStyleSheet() == this )
            {
                pFace->StopDownloads();
            }
        }
    }
#endif // NO_FONT_DOWNLOAD
}

//*********************************************************************
//  CStyleSheet::GetOMRule
//      Returns the automation object for a given rule number, creating
//  the automation object if necessary and caching it in the RuleEntry.
//  If the index is out of range, may return NULL.
//*********************************************************************
HRESULT CStyleSheet::GetOMRule( long lIdx, IHTMLStyleSheetRule **ppSSRule )
{
    Assert( ppSSRule );

    *ppSSRule = NULL;
    if ( lIdx < 0 || lIdx >= _apRulesList.Size() )
        return E_INVALIDARG;

    if ( !_apRulesList[ lIdx ].pAutomationRule )
    {
        _apRulesList[ lIdx ].pAutomationRule = new CStyleSheetRule( this,
                ((DWORD)_apRulesList[ lIdx ].GetStyleID())|((DWORD)_sidSheet) , _apRulesList[ lIdx ]._eTag );
        if ( !_apRulesList[ lIdx ].pAutomationRule )
            return E_OUTOFMEMORY;
    }
    return _apRulesList[ lIdx ].pAutomationRule->QueryInterface( IID_IHTMLStyleSheetRule, (void **)ppSSRule );
}

//*********************************************************************
//  CStyleSheet::title
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::get_title(BSTR *pBSTR)
{
    HRESULT hr = S_OK;

    if (!pBSTR)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pBSTR = NULL;

    Assert( "All sheets must have a parent element!" && _pParentElement );

    // Imports don't support the title property; just return NULL string.
    if ( IsAnImport() )
        goto Cleanup;

    hr = _pParentElement->get_PropertyHelper( pBSTR, (PROPERTYDESC *)&s_propdescCElementtitle );

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

HRESULT
CStyleSheet::put_title(BSTR bstr)
{
    HRESULT hr = S_OK;

    Assert( "All sheets must have a parent element!" && _pParentElement );

    // We don't support the title prop on imports.
    if ( IsAnImport() )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = _pParentElement->put_StringHelper( bstr, (PROPERTYDESC *)&s_propdescCElementtitle );

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::media
//      IHTMLStyleSheet interface method
//*********************************************************************
HRESULT
CStyleSheet::get_media(BSTR *pBSTR)
{
    HRESULT hr = S_OK;

    if (!pBSTR)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pBSTR = NULL;

    Assert( "All sheets must have a parent element!" && _pParentElement );

    if (_pParentElement->Tag() == ETAG_STYLE)
    {
        CStyleElement *pStyle = DYNCAST( CStyleElement, _pParentElement );
        hr = pStyle->get_PropertyHelper( pBSTR, (PROPERTYDESC *)&s_propdescCStyleElementmedia );
    }
    else
    {
        Assert( _pParentElement->Tag() == ETAG_LINK );
        CLinkElement *pLink = DYNCAST( CLinkElement, _pParentElement );
        hr = pLink->get_PropertyHelper( pBSTR, (PROPERTYDESC *)&s_propdescCLinkElementmedia );
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

HRESULT
CStyleSheet::put_media(BSTR bstr)
{
    HRESULT hr = S_OK;

    Assert( "All sheets must have a parent element!" && _pParentElement );

    if (_pParentElement->Tag() == ETAG_STYLE)
    {
        CStyleElement *pStyle = DYNCAST( CStyleElement, _pParentElement );
        hr = pStyle->put_StringHelper( bstr, (PROPERTYDESC *)&s_propdescCStyleElementmedia );
    }
    else
    {
        Assert( _pParentElement->Tag() == ETAG_LINK );
        CLinkElement *pLink = DYNCAST( CLinkElement, _pParentElement );
        hr = pLink->put_StringHelper( bstr, (PROPERTYDESC *)&s_propdescCLinkElementmedia );
    }

    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::get_cssText
//      IHTMLStyleSheet interface method
//*********************************************************************
HRESULT
CStyleSheet::get_cssText(BSTR *pBSTR)
{
    HRESULT hr = S_OK;
    CStr cstr;

    if (!pBSTR)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pBSTR = NULL;

    hr = GetString( &cstr );

    if ( hr != S_OK )
        goto Cleanup;

    hr = cstr.AllocBSTR( pBSTR );

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::put_cssText
//      IHTMLStyleSheet interface method
//*********************************************************************
HRESULT
CStyleSheet::put_cssText(BSTR bstr)
{
    HRESULT hr = S_OK;
    CElement *pParentElement = _pParentElement;
    CStyleSheet *pParentSS = _pParentStyleSheet;
    CMarkup *pMarkup;
    BOOL fDisabled = _fDisabled;
    CCSSParser *parser;
    Assert(pParentElement);

    // Remove all the rules
    hr = ChangeStatus( CS_CLEARRULES, FALSE, NULL );   // disabling, detached from tree, no re-render
    if ( hr )
        goto Cleanup;
    // Now go destroy our rules ref list, release its OMs, and destroy associated font-faces.
    Free();

    // Now restore a few bits of information that don't actually change for us.
    _pParentElement = pParentElement;
    _pParentStyleSheet = pParentSS;
    _fDisabled = fDisabled;

    // Parse the new style string!
#ifdef XMV_PARSE
    parser = new CCSSParser( this, NULL, IsXML());
#else
    parser = new CCSSParser( this, NULL);
#endif
    if ( !parser )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    parser->Open();
    parser->Write( bstr, _tcslen( bstr ) );
    parser->Close();
    delete parser;

    // Reformat and rerender.

    pMarkup = pParentElement->GetMarkup();
    hr = THR( pMarkup->OnCssChange(/*fStable = */ TRUE, /* fRecomputePeers = */ TRUE) );

Cleanup:
    RRETURN( hr );
}

//*********************************************************************
//  CStyleSheet::parentStyleSheet
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::get_parentStyleSheet(IHTMLStyleSheet** ppHTMLStyleSheet)
{
    HRESULT hr = S_OK;

    if (!ppHTMLStyleSheet)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppHTMLStyleSheet = NULL;

    // BUGBUG: Just return self if we're disconnected?
    if ( IsAnImport() && !IsDisconnectedFromParentSS() )
    {
        hr = _pParentStyleSheet->QueryInterface(IID_IHTMLStyleSheet,
                                              (void**)ppHTMLStyleSheet);
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::owningElement
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::get_owningElement(IHTMLElement** ppHTMLElement)
{
    HRESULT hr = S_OK;

    if (!ppHTMLElement)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppHTMLElement = NULL;

    Assert( "All sheets must have a parent element!" && _pParentElement );

    hr = _pParentElement->QueryInterface(IID_IHTMLElement,
                                          (void**)ppHTMLElement);

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::disabled
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::get_disabled(VARIANT_BOOL* pvbDisabled)
{
    HRESULT hr = S_OK;

    if (!pvbDisabled)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pvbDisabled = (_fDisabled ? VB_TRUE : VB_FALSE);

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

HRESULT
CStyleSheet::put_disabled(VARIANT_BOOL vbDisabled)
{
    HRESULT hr = S_OK;
    DWORD   dwAction;

    // If the enable/disable status isn't changing, do nothing.
    if ( (_fDisabled ? VB_TRUE : VB_FALSE) != vbDisabled )
    {
        dwAction = (vbDisabled ? 0 : CS_ENABLERULES);   // 0 means disable rules
        hr = ChangeStatus( dwAction, TRUE, NULL);   // Force a rerender

        // We have to stuff these into the AA by hand in order to avoid
        // firing an OnPropertyChange (which would put us into a recursive loop).
        if ( _pParentElement->Tag() == ETAG_STYLE )
            hr = THR(s_propdescCElementdisabled.b.SetNumber( _pParentElement,
                     CVOID_CAST(_pParentElement->GetAttrArray()), vbDisabled, 0 ));
        else
        {
            Assert( _pParentElement->Tag() == ETAG_LINK );
            hr = THR(s_propdescCElementdisabled.b.SetNumber( _pParentElement,
                     CVOID_CAST(_pParentElement->GetAttrArray()), vbDisabled, 0 ));
        }
    }

    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::readonly
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::get_readOnly(VARIANT_BOOL* pvbReadOnly)
{
    HRESULT hr = S_OK;

    if (!pvbReadOnly)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    Assert( "All sheets must have a parent element!" && _pParentElement );

    // Imports are readonly.  Also, if we have a parent element of type LINK, we must
    // be a linked stylesheet, and thus readonly.

    *pvbReadOnly = ( IsAnImport() || (_pParentElement->Tag() == ETAG_LINK) ) ?
                        VB_TRUE : VB_FALSE;

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::imports
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::get_imports(IHTMLStyleSheetsCollection** ppHTMLStyleSheetsCollection)
{
    HRESULT hr = S_OK;

    if (!ppHTMLStyleSheetsCollection)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppHTMLStyleSheetsCollection = NULL;

    // If we don't already have an imports collection instantiated, do so now.
    if ( !_pImportedStyleSheets )
    {
        // Imported stylesheets don't manage their own rules.
        _pImportedStyleSheets = new CStyleSheetArray( this, _pSSAContainer, _sidSheet );
        Assert( "Failure to allocate imported stylesheets array! (informational)" && _pImportedStyleSheets );
        if (!_pImportedStyleSheets)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        if (_pImportedStyleSheets->_fInvalid)
        {
            hr = E_OUTOFMEMORY;
            _pImportedStyleSheets->CBase::PrivateRelease();
            goto Cleanup;
        }
    }

    hr = _pImportedStyleSheets->QueryInterface(IID_IHTMLStyleSheetsCollection,
                                            (void**)ppHTMLStyleSheetsCollection);

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::rules
//      IHTMLStyleSheet interface method
//*********************************************************************
HRESULT
CStyleSheet::get_rules(IHTMLStyleSheetRulesCollection** ppHTMLStyleSheetRulesCollection)
{
    HRESULT hr = S_OK;

    if (!ppHTMLStyleSheetRulesCollection)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppHTMLStyleSheetRulesCollection = NULL;

    // If we don't already have a rules collection instantiated, do so now.
    if ( !_pOMRulesArray )
    {
        _pOMRulesArray = new CStyleSheetRuleArray( this );
        if ( !_pOMRulesArray )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    hr = _pOMRulesArray->QueryInterface( IID_IHTMLStyleSheetRulesCollection,
                                            (void**)ppHTMLStyleSheetRulesCollection);

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::href
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::get_href(BSTR *pBSTR)
{
    HRESULT hr = S_OK;

    if (!pBSTR)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pBSTR = NULL;

    Assert( "All sheets must have a parent element!" && _pParentElement );

    // If we're an import..
    if ( IsAnImport() )
    {
        hr = _cstrImportHref.AllocBSTR( pBSTR );
    }
    else if ( _pParentElement->Tag() == ETAG_LINK ) // .. if we're a <link>
    {
        CLinkElement *pLink = DYNCAST( CLinkElement, _pParentElement );
        hr = pLink->get_UrlHelper( pBSTR, (PROPERTYDESC *)&s_propdescCLinkElementhref );
    }
    else    // .. we must be a <style>, and have no href.
    {
        Assert( "Bad element type associated with stylesheet!" && _pParentElement->Tag() == ETAG_STYLE );
        goto Cleanup;
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

HRESULT
CStyleSheet::put_href(BSTR bstr)
{
    HRESULT hr = S_OK;

    // Are we an import?
    if ( IsAnImport() )
    {
        if ( _pParentStyleSheet->IsAnImport() || (_pParentElement->Tag() == ETAG_LINK) )
        {
            // If we're an import, but our parent isn't a top-level stylesheet,
            // (i.e. our parent is also an import), or if we're an import of a linked
            // stylesheet then our href is readonly.
            goto Cleanup;
        }

        hr = LoadFromURL( (LPTSTR)bstr );
        if ( hr )
            goto Cleanup;

        _cstrImportHref.Set( bstr );
    }
    // Are we a linked stylesheet?
    else if ( _pParentElement->Tag() == ETAG_LINK )
    {        
        CLinkElement *pLink = DYNCAST( CLinkElement, _pParentElement );
        hr = pLink->put_UrlHelper( bstr, (PROPERTYDESC *)&s_propdescCLinkElementhref );
    }
    // Otherwise we must be a <style>, and have no href.
    else
    {
        Assert( "Bad element type associated with stylesheet!" && _pParentElement->Tag() == ETAG_STYLE );
        goto Cleanup;
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::get_type
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::get_type(BSTR *pBSTR)
{
    HRESULT hr = S_OK;

    if (!pBSTR)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pBSTR = NULL;

    Assert( _pParentElement );

    // BUGBUG: we currently return the top-level SS's HTML type attribute for imports;
    // this is OK for now since we only support text/css, but in theory stylesheets of
    // one type could import stylesheets of a different type.

    if ( _pParentElement->Tag() == ETAG_STYLE )
    {
        CStyleElement *pStyle = DYNCAST( CStyleElement, _pParentElement );
        hr = pStyle->get_PropertyHelper( pBSTR, (PROPERTYDESC *)&s_propdescCStyleElementtype );
    }
    else if ( _pParentElement->Tag() == ETAG_LINK )
    {
        CLinkElement *pLink = DYNCAST( CLinkElement, _pParentElement );
        hr = pLink->get_PropertyHelper( pBSTR, (PROPERTYDESC *)&s_propdescCLinkElementtype );
    }
    else
    {
        Assert( "Bad element type associated with stylesheet!" && FALSE );
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::get_id
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::get_id(BSTR *pBSTR)
{
    HRESULT hr = S_OK;

    if (!pBSTR)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pBSTR = NULL;

    Assert( _pParentElement );

    // Imports have no id; we don't return the parent element's id
    // because that would suggest you could use the id to get to the
    // import (when it would actually get you to the top-level SS).
    if ( IsAnImport() )
    {
        goto Cleanup;
    }

    hr = THR( _pParentElement->get_PropertyHelper( pBSTR, (PROPERTYDESC *)&s_propdescCElementid ) );

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::addImport
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::addImport(BSTR bstrURL, long lIndex, long *plNewIndex)
{
    HRESULT hr = S_OK;

    if ( !plNewIndex )
    {
        hr = E_POINTER;

        goto Cleanup;
    }

    // Return value of -1 indicates failure to insert.
    *plNewIndex = -1;

    // Check for zero-length URL, which we ignore.
    if ( FormsStringLen(bstrURL) == 0 )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // If requested index out of bounds, just append the import
    if ( (lIndex < -1) ||
         (_pImportedStyleSheets && (lIndex > _pImportedStyleSheets->Size())) ||
         (!_pImportedStyleSheets && (lIndex > 0)) )
    {
        lIndex = -1;
    }

    hr = AddImportedStyleSheet( (LPTSTR)bstrURL, lIndex, plNewIndex );
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

HRESULT CStyleSheet::removeImport( long lIndex )
{
    HRESULT hr = S_OK;

    CStyleSheet *pImportedStyleSheet;

    // If requested index out of bounds, error out
    if ( !_pImportedStyleSheets || (lIndex < 0) || (lIndex >= _pImportedStyleSheets->Size()))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pImportedStyleSheet = _pImportedStyleSheets->_aStyleSheets[lIndex];
    pImportedStyleSheet->StopDownloads(TRUE);
    hr = _pImportedStyleSheets->ReleaseStyleSheet( pImportedStyleSheet, TRUE );

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::addRule
//      IHTMLStyleSheet interface method
//*********************************************************************

HRESULT
CStyleSheet::addRule(BSTR bstrSelector, BSTR bstrStyle, long lIndex, long *plNewIndex)
{
    HRESULT         hr = E_OUTOFMEMORY;
    CStyleSelector *pNewSelector = NULL;
    CStyleSelector *pChildSelector = NULL;
    CStyleRule     *pNewRule = NULL;
    CCSSParser     *ps = NULL;
    BOOL           fEatingWhitespace = TRUE;
    TCHAR          chChar;
    CStr           csSelectorText;
    LPTSTR         lpszSelectorText;
    LPTSTR         lpszSelectorWalker;
    LPTSTR         lpszStyleText = (LPTSTR)bstrStyle;

    if ( !plNewIndex || !bstrSelector || !bstrStyle )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *plNewIndex = -1;

    if ( !(*bstrSelector) || !(*bstrStyle) )
    {
        // Strings shouldn't be empty
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Make a copy of the string, so it is not changed during parsing (we insert 0s)
    hr = THR(csSelectorText.Set((LPTSTR)bstrSelector));
    if(hr)
        goto Cleanup;

    lpszSelectorText = lpszSelectorWalker = (LPTSTR)csSelectorText;

    // Parse selector string to handle contextual selectors
    do
    {
        chChar = *lpszSelectorWalker;
        if ( _istalnum( chChar ) || ( chChar == _T('.') ) || ( chChar == _T(':') ) || ( chChar == _T('-') ) || ( chChar == _T('#') ) )
            fEatingWhitespace = FALSE;      // after we've started a selector, whitespace acts a delimiter
        else if (chChar == _T(','))
        {
            // We do not support grouping of selectors through addRule
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        else if ( _istspace( chChar ) || (chChar == _T('\0')) )   // we allow contextual selectors, but not siblings
        {
            if ( fEatingWhitespace )
                ++lpszSelectorText;
            else
            {
                // Done with this selector name.  Terminate the selector with a NUL.
                *lpszSelectorWalker = _T('\0');
#ifdef XMV_PARSE
                // the parent element may have been detached, is there a problem here?
                pChildSelector = new CStyleSelector( lpszSelectorText, pNewSelector, IsXML());
#else
                pChildSelector = new CStyleSelector( lpszSelectorText, pNewSelector );
#endif
                if ( !pChildSelector )
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
                pNewSelector = pChildSelector;
                lpszSelectorText = lpszSelectorWalker+1;    // step 1 past the selector we just handled
                fEatingWhitespace = TRUE;   // go back to eating whitespace until we start another selector
            }
        }
        ++lpszSelectorWalker;
    } while ( chChar != _T('\0') );

    if ( !pNewSelector )
    {   // Selector was invalid or empty
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pNewRule = new CStyleRule( pNewSelector );
    if ( !pNewRule )
        goto Cleanup;

    // Actually parse the style text
#ifdef XMV_PARSE
    ps = new CCSSParser( this, &(pNewRule->_paaStyleProperties), IsXML(), eSingleStyle, &CStyle::s_apHdlDescs,
                       this, HANDLEPROP_SET|HANDLEPROP_VALUE );
#else
    ps = new CCSSParser( this, &(pNewRule->_paaStyleProperties), eSingleStyle, &CStyle::s_apHdlDescs,
                       this, HANDLEPROP_SET|HANDLEPROP_VALUE );
#endif
    if ( !ps )
        goto Cleanup;

    ps->Open();
    ps->Write( lpszStyleText, lstrlen( lpszStyleText ) );
    ps->Close();

    delete ps;

    // Add the rule to our stylesheet, and get the index
    hr = AddStyleRule( pNewRule, TRUE, lIndex );
    // The AddStyleRule call will have deleted the new rule for us, so we're OK.
    if ( hr )
    {
        pNewRule = NULL;    // already deleted by AddStyleRule _even when it fails_
        goto Cleanup;
    }

    hr = THR(_pParentElement->OnCssChange( /*fStable = */ TRUE, /* fRecomputePeers = */ TRUE));

Cleanup:
    if ( hr )
    {
        if ( pNewSelector )
            delete pNewSelector;
        if ( pNewRule )
            delete pNewRule;
    }

    RRETURN(SetErrorInfo( hr ));
}

//*********************************************************************
//  CStyleSheet::removeRule
//      IHTMLStyleSheet interface method
//      This method remove a rule from CStyleRuleArray in the hash table
//  as well as the CRuleEntryArray.
//*********************************************************************
HRESULT CStyleSheet::removeRule( long lIndex )
{
    CRuleEntry *pRE;
    BOOL abFound[ ETAG_LAST ];
    int i, nRules;
    CStyleID sid;
    HRESULT     hr = S_OK;

    Assert( "Stylesheet must have a container!" && _pSSAContainer );

    // Make sure the index is valid.
    if ( ( lIndex < 0 ) || ( lIndex >= _apRulesList.Size() ) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Cook up the absolute id of the rule to destroy.  We could index
    // in the array and get it, but this is quicker.
    sid = _sidSheet;
    sid.SetRule( lIndex + 1 );

    // Cook up an array of which slots we need to search for rules in.
    memset( abFound, 0, sizeof(BOOL) * ETAG_LAST );
    for ( pRE = (CRuleEntry *)_apRulesList + lIndex, i = lIndex, nRules = _apRulesList.Size();
          i < nRules; i++, pRE++ )
    {
        abFound[ (int)(pRE->_eTag) ] = TRUE;
        if ( pRE->pAutomationRule )
        {   // Need to fix up automation rule's sid.
            pRE->pAutomationRule->_dwID--;
        }
    }

    if(_apRulesList[lIndex].pAutomationRule)
    {
        _apRulesList[lIndex].pAutomationRule->StyleSheetRelease();
        _apRulesList[lIndex].pAutomationRule = NULL;
    }
  
    // Get rid of the rule in the big pool of rules - this will actually destroy it.
    _pSSAContainer->ShiftRules( CStyleSheetArray::ruleRemove, TRUE, abFound, sid );

    _apRulesList[lIndex]._pRule->Free();
    delete _apRulesList[lIndex]._pRule;
    _apRulesList.Delete( lIndex );

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT CStyleSheet::GetString( CStr *pResult )
{
    CStyleRule *pRule;
    CRuleEntry *pRE;
    int i, nRules;

    // Handle @imports.
    if ( _pImportedStyleSheets )
    {
        int nImports = _pImportedStyleSheets->Size();
        for ( i = 0; i < nImports; i++ )
        {
            CStyleSheet *pImport = _pImportedStyleSheets->Get( i );
            if ( pImport )
            {
                pResult->Append( _T("@import url( ") );
                pResult->Append( pImport->_cstrImportHref );
                pResult->Append( _T(" );\r\n") );
            }
        }
    }

    int nFonts = _pSSAContainer->_apFontFaces.Size();
    CFontFace **ppFaces = (CFontFace **)_pSSAContainer->_apFontFaces;
    CFontFace *pFace;
    LPCTSTR pcszURL;

    for( i=0; i < nFonts; i++ )
    {
        pFace = ppFaces[i];
        if ( pFace->ParentStyleSheet() == this )
        {
            pcszURL = pFace->GetSrc();
            pResult->Append( _T("@font-face {\r\n\tfont-family: ") );
            pResult->Append( pFace->GetFriendlyName() );
            if ( pcszURL )
            {
                pResult->Append( _T(";\r\n\tsrc:url(") );
                pResult->Append( pcszURL );
                pResult->Append( _T(")") );
            }
            pResult->Append( _T(";\r\n}\r\n") );
        }
    }

    DWORD           dwPrevMedia = (DWORD)MEDIA_NotSet;
    DWORD           dwCurMedia;
    CBufferedStr    strMediaString;

    // Handle rules.
    for ( pRE = (CRuleEntry *)_apRulesList, i = 0, nRules = _apRulesList.Size();
          i < nRules; i++, pRE++ )
    {
        CStyleID sid(((DWORD)_sidSheet)|((DWORD)pRE->GetStyleID()));
        pRule = pRE->_pRule;    // _pSSAContainer->GetRule( pRE->_eTag, sid );
        if ( pRule )
        {
            // Write the media type string if it has changed from previous rule
            dwCurMedia = pRule->GetLastAtMediaTypeBits();
            if(dwCurMedia != MEDIA_NotSet) 
            {
                if(dwCurMedia != dwPrevMedia)
                {
                    // Media type has changed, close the previous if needed and open a new one
                    if(dwPrevMedia != MEDIA_NotSet)
                        pResult->Append( _T("\r\n}\r\n") );

                    pResult->Append( _T("\r\n@media ") );
                    pRule->GetMediaString(dwCurMedia, &strMediaString);
                    pResult->Append(strMediaString);
                    pResult->Append( _T("    \r\n{\r\n") );
                }
            }
            else
            {
                // Close the old one if it is there
                if(dwPrevMedia != MEDIA_NotSet)
                    pResult->Append( _T("    }\r\n") );
            }

            // Save the new namespace as the current one
            dwPrevMedia = dwCurMedia;
            
            // Now append the rest of the rule
            pRule->GetString( this, pResult );
        }
    }

    // If we have not closed the last namespace, close it
    if(dwPrevMedia != MEDIA_NotSet)
        pResult->Append( _T("\r\n}\r\n") );

    return S_OK;
}


CStyleRule *CStyleSheet::GetRule( ELEMENT_TAG eTag, CStyleID ruleID )
{
    CStyleRule *pRule = NULL;
    CRuleEntry *pRE;
    int i, nRules;

    for ( pRE = (CRuleEntry *)_apRulesList, i = 0, nRules = _apRulesList.Size();
          (i < nRules) && !pRule; 
          i++, pRE++ )
    {
        if ((pRE->_eTag == eTag) && (pRE->GetStyleID() == ruleID))
            pRule = pRE->_pRule;
    }
 
    return pRule;
};

//---------------------------------------------------------------------
//  Class Declaration:  CStyleID
//
//  A 32-bit cookie that uniquely identifies a style rule's position
//  in the cascade order (i.e. it encodes the source order within its
//  containing stylesheet, as well as the nesting depth position of
//  its containing stylesheet within the entire stylesheet tree.
//
//  The source order of the rule within the sheet is encoded in the
//  Rules field (12 bits).
//
//  We allow up to MAX_IMPORT_NESTING (4) levels of @import nesting
//  (including the topmost HTML document level).  The position within
//  each nesting level is encoded in 5 bits.
//
//---------------------------------------------------------------------
CStyleID::CStyleID(const unsigned long l1, const unsigned long l2,
                    const unsigned long l3, const unsigned long l4, const unsigned long r)
{
    Assert( "Maximum of 31 stylesheets per level!" && l1 <= MAX_SHEETS_PER_LEVEL && l2 <= MAX_SHEETS_PER_LEVEL && l3 <= MAX_SHEETS_PER_LEVEL && l4 <= MAX_SHEETS_PER_LEVEL );
    Assert( "Maximum of 4095 rules per stylesheet!" && r <= MAX_RULES_PER_SHEET );

    _dwID = ((l1<<27) & LEVEL1_MASK) | ((l2<<22) & LEVEL2_MASK) | ((l3<<17) & LEVEL3_MASK) |
            ((l4<<12) & LEVEL4_MASK) | (r & RULE_MASK);
}

//*********************************************************************
// CStyleID::SetLevel()
// Sets the value of a particular nesting level
//*********************************************************************
void CStyleID::SetLevel(const unsigned long level, const unsigned long value)
{
    Assert( "Maximum of 31 stylesheets per level!" && value <= MAX_SHEETS_PER_LEVEL );
    switch( level )
    {
        case 1:
            _dwID &= ~LEVEL1_MASK;
            _dwID |= ((value<<27) & LEVEL1_MASK);
            break;
        case 2:
            _dwID &= ~LEVEL2_MASK;
            _dwID |= ((value<<22) & LEVEL2_MASK);
            break;
        case 3:
            _dwID &= ~LEVEL3_MASK;
            _dwID |= ((value<<17) & LEVEL3_MASK);
            break;
        case 4:
            _dwID &= ~LEVEL4_MASK;
            _dwID |= ((value<<12) & LEVEL4_MASK);
            break;
        default:
            Assert( "Invalid Level for style ID" && FALSE );
    }
}

//*********************************************************************
// CStyleID::GetLevel()
// Gets the value of a particular nesting level
//*********************************************************************
unsigned long CStyleID::GetLevel(const unsigned long level) const
{
    switch( level )
    {
        case 1:
            return ((_dwID>>27)&0x1F);
        case 2:
            return ((_dwID>>22)&0x1F);
        case 3:
            return ((_dwID>>17)&0x1F);
        case 4:
            return ((_dwID>>12)&0x1F);
        default:
            Assert( "Invalid Level for style ID" && FALSE );
            return 0;
    }
}

//*********************************************************************
// TranslateMediaTypeString()
//      Parses a MEDIA attribute and builds the correct EMediaType from it.
//*********************************************************************
DWORD TranslateMediaTypeString( LPCTSTR pcszMedia )
{
    DWORD dwRet = 0;
    LPTSTR pszMedia = _tcsdup ( pcszMedia );
    LPTSTR pszString = pszMedia;
    LPTSTR pszNextToken;
    LPTSTR pszLastChar;

    for ( ; pszString && *pszString; pszString = pszNextToken )
    {
        while ( _istspace( *pszString ) )
            pszString++;
        pszNextToken = pszString;
        while ( *pszNextToken && *pszNextToken != _T(',') )
            pszNextToken++;
        if ( pszNextToken > pszString )
        {
            pszLastChar = pszNextToken - 1;
            while ( isspace( *pszLastChar ) && ( pszLastChar >= pszString ) )
                *pszLastChar-- = _T('\0');
        }
        if ( *pszNextToken )
            *pszNextToken++ = _T('\0');
        if ( !*pszString )
            continue;   // This is so empty MEDIA strings will default to All instead of Unknown.
        dwRet |= CSSMediaTypeFromName(pszString);
    }

    if ( !dwRet )
        dwRet = MEDIA_All;
    MemFree( pszMedia );
    return ( dwRet );
}

HRESULT
CDoc::createStyleSheet ( BSTR bstrHref /*=""*/, long lIndex/*=-1*/, IHTMLStyleSheet ** ppnewStyleSheet )
{
    Assert(_pPrimaryMarkup);
    RRETURN(_pPrimaryMarkup->createStyleSheet(bstrHref, lIndex, ppnewStyleSheet));
}


// 
//*********************************************************************
// CNamespace::SetNamespace
// Parses given string and stores into member variables depending on the type
//*********************************************************************
HRESULT 
CNamespace::SetNamespace(LPCTSTR pchStr)
{
    HRESULT         hr;
    CBufferedStr    strWork;
    LPTSTR          pStr;

    strWork.Set(pchStr);

    pStr = (LPTSTR)strWork;

    hr = THR(RemoveStyleUrlFromStr(&pStr));
    if(SUCCEEDED(hr))
    {
        _strNamespace.Set(pStr);
        hr = S_OK;
    }
    //_strNameSpace will be empty if we fail

    RRETURN(hr);
}



//*********************************************************************
// CNamespace::IsEqual
// Returns TRUE if the namspaces  are equal
//*********************************************************************
BOOL 
CNamespace::IsEqual(const CNamespace * pNmsp) const
{
    if(!pNmsp || pNmsp->IsEmpty())
    {
        if(IsEmpty())
            return TRUE;
        else
            return FALSE;
    }

    return (_tcsicmp(_strNamespace, pNmsp->_strNamespace) == 0);
}


const CNamespace& 
CNamespace::operator=(const CNamespace & nmsp)
{
    if(&nmsp != this)
    {
        _strNamespace.Set(nmsp._strNamespace);
    }

    return *this;
}

