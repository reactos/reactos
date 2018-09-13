//+------------------------------------------------------------------------
//
//  File:       FORMCOLL.CXX
//
//  Contents:   Implementation of collection for CDoc
//
//  Classes:    (part of) CDoc
//              (part of) CMarkup for the Collection Cache (tomfakes)
//
//-------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_HEADELEMS_HXX_
#define X_HEADELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"
#endif

#ifndef X_FRAMELYT_HXX_
#define X_FRAMELYT_HXX_
#include "framelyt.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EANCHOR_HXX_
#define X_EANCHOR_HXX_
#include "eanchor.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_ELINK_HXX_
#define X_ELINK_HXX_
#include "elink.hxx"
#endif

#ifndef X_ESTYLE_HXX_
#define X_ESTYLE_HXX_
#include "estyle.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_ROOTELEMENT_HXX_
#define X_ROOTELEMENT_HXX_
#include "rootelem.hxx"
#endif

DeclareTag(tagCollectionsAddToCollections, "Collections", "trace CMarkup::AddToCollections")

#define USE_TREE_FOR_ALL_COLLECTION 1

MtDefine(CDocRecomputeTreeCache_arySitesOriginal_pv, Locals, "CDoc::RecomputeTreeCache arySitesOriginal::_pv")
MtDefine(CDocRecomputeTreeCache_arySitesDetach_pv, Locals, "CDoc::RecomputeTreeCache arySitesDetach::_pv")
MtDefine(CDocRecomputeTreeCache_arySitesDormancy_pv, Locals, "CDoc::RecomputeTreeCAche arySitesDormancy::_pv")
MtDefine(CDocOnElementEnter_pTabs, Locals, "CDoc::OnElementEnter::_pTabs")
MtDefine(BldElementCol, PerfPigs, "Build CDoc::ELEMENT_COLLECTION")
MtDefine(BldFormsCol, PerfPigs, "Build CDoc::FORMS_COLLECTION")
MtDefine(BldAnchorsCol, PerfPigs, "Build CDoc::ANCHORS_COLLECTION")
MtDefine(BldLinksCol, PerfPigs, "Build CDoc::LINKS_COLLECTION")
MtDefine(BldImagesCol, PerfPigs, "Build CDoc::IMAGES_COLLECTION")
MtDefine(BldAppletsCol, PerfPigs, "Build CDoc::APPLETS_COLLECTION")
MtDefine(BldScriptsCol, PerfPigs, "Build CDoc::SCRIPTS_COLLECTION")
MtDefine(BldMapsCol, PerfPigs, "Build CDoc::MAPS_COLLECTION")
MtDefine(BldWindowCol, PerfPigs, "Build CDoc::WINDOW_COLLECTION")
MtDefine(BldEmbedsCol, PerfPigs, "Build CDoc::EMBEDS_COLLECTION")
MtDefine(BldRegionCol, PerfPigs, "Build CDoc::REGION_COLLECTION")
MtDefine(BldLabelCol, PerfPigs, "Build CDoc::LABEL_COLLECTION")
MtDefine(BldNavDocCol, PerfPigs, "Build CDoc::NAVDOCUMENT_COLLECTION")
MtDefine(BldFramesCol, PerfPigs, "Build CDoc::FRAMES_COLLECTION")
MtDefine(BldOtherCol, PerfPigs, "Build CDoc::OTHER_COLLECTION (Unknown)")
MtDefine(CAllCollectionCacheItem, CDoc, "CDoc::CAllCollectionCacheItem")

//+----------------------------------------------------------------------------
//
// Member:      RecomputeTreeCache
//
// Synopsis:    This member invalidates any cached information in the Doc and
//              causes a monster walk to take place to recalculate the cached
//              information.
//
//+----------------------------------------------------------------------------

class CTable;
class CTableRow;


class CCollectionBuildContext
{
public:
    CDoc *_pDoc;
    // Collection State
    long _lCollection;
    BOOL _fNeedNameID;
    BOOL _fNeedForm;

    CCollectionBuildContext (CDoc *pDoc)
    {
        _pDoc = pDoc;
        _lCollection = 0;
        _fNeedNameID = FALSE;
        _fNeedForm = FALSE;
    };
};


class CAllCollectionCacheItem : public CCollectionCacheItem
{
private:
    LONG _lCurrentIndex;
    CMarkup *GetMarkup ( void ) { return _pMarkup; }
    CMarkup *_pMarkup;
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CAllCollectionCacheItem))
    CElement *GetNext ( void );
    CElement *MoveTo ( long lIndex );
    CElement *GetAt ( long lIndex );
    long Length ( void );

    void SetMarkup(CMarkup *pMarkup)
        { _pMarkup = pMarkup; }
};


#if DBG == 1

void
CDoc::UpdateDocTreeVersion ( )
{
    __lDocTreeVersion++;
    UpdateDocContentsVersion();
}

void
CDoc::UpdateDocContentsVersion ( )
{
    __lDocContentsVersion++;
}

#endif

//+----------------------------------------------------------------------------
//
// Member:      EnsureCollectionCache
//
// Synopsis:    Ensures that the collection cache is built
//              NOTE: Ensures the _cache_, not the _collections_.
//
//+----------------------------------------------------------------------------

HRESULT
CMarkup::EnsureCollectionCache ( long lCollectionIndex )
{
    HRESULT hr;

    hr = THR(InitCollections());
    if (hr)
        goto Cleanup;

    hr = THR( CollectionCache()->EnsureAry( lCollectionIndex ) );

Cleanup:
    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Member:     EnsureCollections
//
//  Synopsis:   rebuild the collections based on the current state of the tree
//
//-----------------------------------------------------------------------------



HRESULT
CMarkup::EnsureCollections( long lIndex, long * plCollectionVersion )
{
    HRESULT hr = S_OK;
    CCollectionBuildContext collectionWalker ( _pDoc );
    long l,lSize;
    CElement *pElemCurrent;
    CCollectionCache *pCollectionCache;

#ifdef PERFMETER
    static PERFMETERTAG s_mpColMtr[] = { Mt(BldElementCol), Mt(BldFormsCol), Mt(BldAnchorsCol), Mt(BldLinksCol), Mt(BldImagesCol),
        Mt(BldAppletsCol), Mt(BldScriptsCol), Mt(BldMapsCol), Mt(BldWindowCol), Mt(BldEmbedsCol), Mt(BldRegionCol),
        Mt(BldLabelCol), Mt(BldNavDocCol), Mt(BldFramesCol) };
#endif

    pCollectionCache = CollectionCache();
    if ( !pCollectionCache )
        goto Cleanup;

    // Optimize the use of the region collection. The doc flag is set by
    // the CSS code whenever theres a position: attribute on an element in the doc
    // This is a temporary Beta1 Hack to avoid building the regions collection
    if(lIndex == REGION_COLLECTION && !_pDoc->_fRegionCollection)  //$$tomfakes - move this flag to CMarkup?
    {
        pCollectionCache->ResetAry( REGION_COLLECTION ); // To be safe
        goto Cleanup;
    }


    if(lIndex == ELEMENT_COLLECTION)
    {
        goto Update; // All collection is always up to date, update the version no & bail out
    }
	else if ( lIndex == REGION_COLLECTION )
    {
        // We ignore the collection _fIsValid flag for the REGION_COLLECTION
		// because it is unaffected by element name changes
        if (*plCollectionVersion == _pDoc->GetDocTreeVersion())
            goto Cleanup;
    }
    else
    {
        // Collections that are specificaly invalidated
        if (pCollectionCache->IsValidItem(lIndex))
		{
			// Doesn't change collection version, collections based on this one don't get rebuilt
            goto Cleanup; 
		}
    }

    MtAdd(lIndex < ARRAY_SIZE(s_mpColMtr) ? s_mpColMtr[lIndex] : Mt(BldOtherCol), +1, 0);

    collectionWalker._lCollection = lIndex;

    // Set flag to indicate whether or not we need to go get the name/ID of
    // elements during the walk
    if ( lIndex == NAVDOCUMENT_COLLECTION ||
        lIndex == ANCHORS_COLLECTION ||
		lIndex == WINDOW_COLLECTION )
    {
        collectionWalker._fNeedNameID = TRUE;
    }
    // Set flag to indicate whether or not we need to go get the containing form of
    // elements during the walk
    if ( lIndex == WINDOW_COLLECTION || lIndex == NAVDOCUMENT_COLLECTION ||
        lIndex == FORMS_COLLECTION || lIndex == FRAMES_COLLECTION )
    {
        collectionWalker._fNeedForm = TRUE;
    }

    //
    // Here we blow away any cached information which is stored in the doc.
    // Usually elements, themselves, will blow away their own cached info
    // when they are first visited, but because the doc is not an element,
    // it will not be visited, and must blow away the cached info before we
    // start the walk.
    //


    // every fixed collection is based on the all collection
    if ( lIndex != ELEMENT_COLLECTION )
    {
        pCollectionCache->ResetAry( lIndex );
    }

    // Otherwise all collection is up to date, so use it because its faster
    lSize = pCollectionCache->SizeAry ( ELEMENT_COLLECTION );
    for ( l = 0 ; l < lSize ; l++ )
    {
       hr = THR(pCollectionCache->GetIntoAry(
                ELEMENT_COLLECTION,
                l,
                &pElemCurrent));
        if ( hr )
            goto Cleanup;
        hr = THR (AddToCollections ( pElemCurrent, &collectionWalker ));
        if ( hr )
            goto Cleanup;
    }

Update:
    // Update the version on the collection
    if ( lIndex == REGION_COLLECTION || lIndex == ELEMENT_COLLECTION)
    {
        // Collection derived from tree
        *plCollectionVersion = _pDoc->GetDocTreeVersion();
    }
	else
	{
		(*plCollectionVersion)++;
	}

	pCollectionCache->ValidateItem ( lIndex );

Cleanup:
    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//
//  member : InFormCollection
//
//  Synopsis : helper function for determining if an element will appear in a
//     form element's collecion of if it should be scoped to the document's all
//     collection. this is also used by buildTypeInfo for hooking up VBSCRIPT
//
//----------------------------------------------------------------------------

CTreeNode *
CMarkup::InFormCollection(CTreeNode * pNode)
{
    CTreeNode * pNodeParentForm = NULL;

    if (!pNode)
        goto Cleanup;

    // NOTE: Forms are now promoted to the window, this is because the AddNamedItem
    //       no longer adds the form and we need to have access to the form for
    //       <SCRIPT FOR EVENT>
    switch (pNode->TagType())
    {
    case ETAG_INPUT:
    case ETAG_FIELDSET:
    case ETAG_SELECT:
#ifdef  NEVER
    case ETAG_HTMLAREA:
#endif
    case ETAG_TEXTAREA:
    case ETAG_IMG:
    case ETAG_BUTTON:
    case ETAG_OBJECT:
    case ETAG_EMBED:
        // COMMENT rgardner - A good optimization here would be to have the tree walker
        // retain the last scoping form element & pass in the pointer
        pNodeParentForm = pNode->SearchBranchToRootForTag( ETAG_FORM );
        break;
    }

Cleanup:
    return pNodeParentForm;
}

//+------------------------------------------------------------------------
//
//  Member:     AddToCollections
//
//  Synopsis:   add this element to the collection cache
//
//  lNumNestedTables The number of TABLE tags we're nested underneath - speeds
//                  up IMG tag handling
//-------------------------------------------------------------------------

//
// DEVNOTE rgardner
// This code is tighly couples with CElement::OnEnterExitInvalidateCollections and needs
// to be kept in sync with any changes in that function
//

HRESULT
CMarkup::AddToCollections ( CElement *pElement, CCollectionBuildContext *pMonsterWalk )
{
    int         i;
    HRESULT     hr = S_OK;
    CTreeNode * pNodeForm = NULL;
    LPCTSTR pszName = NULL;
    LPCTSTR pszID = NULL;
    CCollectionCache *pCollectionCache;

#if DBG == 1
    TraceTag((
        tagCollectionsAddToCollections,
        "CMarkup::AddToCollections, tag: %ls, tag id: %ls, collection: %X",
        pElement->TagName(), STRVAL(pElement->GetAAid()),
        pMonsterWalk->_lCollection));
#endif


    // Note here that the outer code has mapped the FRAMES_COLLECTION onto the same Index as
    // the WINDOW_COLLECTION, so both collections get built
    if (!pElement)
        goto Cleanup;

    // Names & ID's are needed by :
    // NAVDOCUMENT_COLLECTION | ANCHORS_COLLECTION
    // This flag is initialized in EnsureCollections()

    // _fIsNamed is always up to date
    if ( pMonsterWalk->_fNeedNameID && pElement->IsNamed() )
    {
        pElement->FindString ( STDPROPID_XOBJ_NAME, &pszName );
        pElement->FindString ( DISPID_CElement_id, &pszID );
    }

    //
    // Retrieve the Collection Cache
    //
    pCollectionCache = CollectionCache();

    //
    // Next check to see if element belongs in the window collection
    // Only elements which don't lie inside of forms (and are not forms) belong in here.
    // Things which are not inserted into the form's element collection
    // are also put into the window collection.  E.g. anchors.
    //
    if ( pMonsterWalk->_fNeedForm )
    {
        pNodeForm = InFormCollection(pElement->GetFirstBranch());
    }

    if (pMonsterWalk->_lCollection == WINDOW_COLLECTION  && !pNodeForm && pElement->IsNamed())
    {
        LPCTSTR pszUniqueName = NULL;

        if ( !pszName && !pszID )
            pElement->FindString ( DISPID_CElement_uniqueName, &pszUniqueName );

        if ( pszName || pszID || pszUniqueName )
        {
            if (!pElement->IsOverflowFrame())
            {
                hr = THR(pCollectionCache->SetIntoAry(WINDOW_COLLECTION, pElement));
                if (hr)
                    goto Cleanup;
            }
        }
    }

    // See if its a FORM within a FORM, Nav doesn't promote names of nested forms to the doc
    if ( pMonsterWalk->_fNeedForm && ETAG_FORM == pElement->Tag()  )
    {
        pNodeForm = pElement->GetFirstBranch()->SearchBranchToRootForTag( ETAG_FORM );
        if ( SameScope(pNodeForm, pElement->GetFirstBranch()) )
            pNodeForm = NULL;
    }

    // We use the NAVDOCUMENT_COLLECTION to resolve names on the document object
    // If you have a name you get promoted ala Netscape
    // If you have an ID you get promoted, regardless
    // If you have both, you get netscapes rules
    switch (pElement->TagType())
    {
    case ETAG_FORM:
        if (pNodeForm) // only count images, forms not in a form
            break;
        // fallthrough


    case ETAG_IMG:
    case ETAG_EMBED:
    case ETAG_IFRAME:
    case ETAG_APPLET:
    case ETAG_OBJECT:
        if ( pMonsterWalk->_lCollection == NAVDOCUMENT_COLLECTION )
        {
            if ( pszName )
            {
                hr = THR(pCollectionCache->SetIntoAry(NAVDOCUMENT_COLLECTION, pElement));
                if (hr)
                    goto Cleanup;
            }
            // IE30 compatability OBJECTs/APPLETs with IDs are prmoted to the document
            else if ( ( pElement->Tag() == ETAG_APPLET ||
                pElement->Tag() == ETAG_OBJECT ) && pszID )
            {
                hr = THR(pCollectionCache->SetIntoAry(NAVDOCUMENT_COLLECTION, pElement));
                if (hr)
                    goto Cleanup;
            }
        }
        break;

    case ETAG_INPUT:
        // just doing some houscleaining as long as we had a switch on tagtype...
        // If this is a radio button, we may need to clear the check
        // on other radio buttons in the same group
        // BUGBUG rgardner, deliberately breaking this behaviour, need to rework with Mohan
        /*
        if (CLSID_HTMLOptionButtonElement == *(pNode->Element()->BaseDesc()->_pclsid)
            && DYNCAST(CCheckboxElement, pNode->Element())->GetAAtype() == htmlInputRadio)
        {
            hr = THR(DYNCAST( CRadioElement, pNode->Element())->ClearGroup(
                        &pMonsterWalk->_RadioGroupsInternal ) );
            if ( hr )
                goto Cleanup;
        }
        */
        break;
    }

    //
    // See if this element is a region, which means its "container" attribute
    // is "moveable", "in-flow", or "positioned". If so then put it in the
    // region collection, which is used to ensure proper rendering. The BODY
    // should not be added to the region collection.
    //
    if (pMonsterWalk->_lCollection == REGION_COLLECTION &&
        pElement->Tag() != ETAG_BODY &&
        !pElement->IsPositionStatic())
    {
        hr = THR(pCollectionCache->SetIntoAry(REGION_COLLECTION, pElement));
        if (hr)
            goto Cleanup;
    }

    switch(pElement->TagType())
    {
    case ETAG_LABEL:
        i = LABEL_COLLECTION;
        break;

    case ETAG_FRAME:
    case ETAG_IFRAME:
        if (pElement->_fSynthesized)
            goto Cleanup;

        if ( pMonsterWalk->_lCollection == FRAMES_COLLECTION &&
             !pElement->IsOverflowFrame())
        {
            hr = THR(pCollectionCache->SetIntoAry(FRAMES_COLLECTION, pElement));
        }
        goto Cleanup;

    case ETAG_FORM:
        if ( pNodeForm )
        {
            // nested forms don't go into the forms collection
            goto Cleanup;
        }
        i = FORMS_COLLECTION;
        break;

    case ETAG_AREA:
        if ( pMonsterWalk->_lCollection == LINKS_COLLECTION )
        {
            LPCTSTR lpHRef = DYNCAST(CAreaElement, pElement)->GetAAhref();

            // If the AREA element has an href attribute even HREF="", add it to the
            // links' collection
            if( !lpHRef )
            {
                goto Cleanup;
            }
        }
        i = LINKS_COLLECTION;
        break;

    case ETAG_A:
        // If the A element has an non empty name/id attribute, add it to
        // the anchors' collection
        if ( pMonsterWalk->_lCollection == ANCHORS_COLLECTION )
        {
            LPCTSTR lpAnchorName = pszName;
            if (!lpAnchorName)
            {
                lpAnchorName = pszID;
            }
            if ( lpAnchorName && lpAnchorName[0] )
            {
                hr = THR(pCollectionCache->SetIntoAry(ANCHORS_COLLECTION, pElement));
                if ( hr )
                    goto Cleanup;
            }
        }

        if ( pMonsterWalk->_lCollection == LINKS_COLLECTION )
        {
            LPCTSTR lpHRef = DYNCAST(CAnchorElement, pElement)->GetAAhref();

            // If the A element has an href attribute even HREF="", add it to the
            // links' collection
            if( lpHRef )
            {
                hr = THR(pCollectionCache->SetIntoAry(LINKS_COLLECTION, pElement));
            }
        }
        goto Cleanup;

    case ETAG_IMG:
        // The document.images collection in Nav has a quirky bug, for every TABLE
        // above one Nav adds 2^n IMG elements.
        i = IMAGES_COLLECTION;
        break;

    case ETAG_EMBED:
        i = EMBEDS_COLLECTION;
        break;

    case ETAG_OBJECT:
    case ETAG_APPLET:
        i = APPLETS_COLLECTION;
        break;

    case ETAG_SCRIPT:
        i = SCRIPTS_COLLECTION;
        break;

    case ETAG_MAP:
        i = MAPS_COLLECTION;
        break;

    default:
        goto Cleanup;
    }

    if ( i == pMonsterWalk->_lCollection )
    {
        hr = THR(pCollectionCache->SetIntoAry(i, pElement));
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     GetCollection
//
//  Synopsis:   return collection based on index in the collection cache
//
//-------------------------------------------------------------------------

HRESULT
CMarkup::GetCollection(int iIndex, IHTMLElementCollection ** ppdisp)
{
    Assert((iIndex >= 0) && (iIndex < NUM_DOCUMENT_COLLECTIONS));

    HRESULT hr;

    if (!ppdisp)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppdisp = NULL;

    hr = EnsureCollectionCache(iIndex);
    if ( hr )
        goto Cleanup;

    hr = THR(CollectionCache()->GetDisp(iIndex, (IDispatch **)ppdisp));

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     GetElementByNameOrID
//
//  Synopsis:   look up an Element by its Name or ID
//
//  Returns:    S_OK, if it found the element.  *ppElement is set
//              S_FALSE, if multiple elements w/ name were found.
//                  *ppElement is set to the first element in list.
//              Other errors.
//-------------------------------------------------------------------------

HRESULT
CMarkup::GetElementByNameOrID(LPTSTR szName, CElement **ppElement)
{
    HRESULT hr;
    CElement * pElemTemp;

    hr = THR(EnsureCollectionCache(ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(CollectionCache()->GetIntoAry(
        ELEMENT_COLLECTION,
        szName,
        FALSE,
        &pElemTemp));
    *ppElement = pElemTemp;

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+------------------------------------------------------------------------
//
//  Member:     GetDispByNameOrID
//
//  Synopsis:   Retrieve an IDispatch by its Name or ID
//
//  Returns:    An IDispatch* to an element or a collection of elements.
//
//-------------------------------------------------------------------------

HRESULT
CMarkup::GetDispByNameOrID(LPTSTR szName, IDispatch **ppDisp, BOOL fAlwaysCollection)
{
    HRESULT hr;

    hr = THR(EnsureCollectionCache(ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(CollectionCache()->GetDisp(
        ELEMENT_COLLECTION,
        szName,
        CacheType_Named,
        ppDisp,
        FALSE,  // Case Insensitive
        NULL,
        fAlwaysCollection)); // Always return a collection (even if empty, or has 1 elem)

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::OnElementEnter
//
//  Synopsis:   Update the focus collection upon an element entering the tree
//
//-------------------------------------------------------------------------

HRESULT
CDoc::OnElementEnter(CElement *pElement)
{
    HRESULT     hr = S_OK;

    // Insert pElement in _aryAccessKeyItems if accessKey is defined.
    //
    
    LPCTSTR lpstrAccKey = pElement->GetAAaccessKey();
    if (lpstrAccKey && lpstrAccKey[0])
    {
        hr = InsertAccessKeyItem(pElement);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::OnElementExit
//
//  Synopsis:   Remove all references the doc may have to an element
//              that is exiting the tree
//
//-------------------------------------------------------------------------

HRESULT
CDoc::OnElementExit(CElement *pElement, DWORD dwExitFlags )
{
    long    i;

    
    if (_fVID && !_fOnControlInfoChangedPosted)
    {
        _fOnControlInfoChangedPosted = TRUE;
        IGNORE_HR(GWPostMethodCall(this, ONCALL_METHOD(CDoc, OnControlInfoChanged, oncontrolinfochanged), 0, FALSE, "CDoc::OnControlInfoChanged"));
    }

    if (_pMenuObject == pElement)
    {
        _pMenuObject = NULL;
    }

    // Clear the edit context, if it is going away
    if (    _pElemEditContext
        &&  (   _pElemEditContext == pElement
             || (   pElement->HasSlaveMarkupPtr()
                 && _pElemEditContext == pElement->GetSlaveMarkupPtr()->FirstElement()
                )
            )
        )
    {
        _pElemEditContext = NULL;
    }    

    //
    // marka - find the Element that will be current next - via adjusting the edit context
    // fire on Before Active Elemnet Change. If it's ok - do a SetEditContext on that elemnet,
    // and set _pElemCurrent to it. 
    //
    // Otherwise if FireOnBeforeActiveElement fails - 
    // we make the Body the current element, and call SetEditCOntext on that.
    //
    if (_pElemCurrent == pElement)
    {
        if( dwExitFlags & EXITTREE_DESTROY )
        {
            // BUGBUG: (jbeda) is this right?  What else do we have to do on markup destroy?
            _pElemCurrent = PrimaryRoot();
        }
        else
        {
            CElement* pElementEditContext = pElement;
            CElement* pCurElement = NULL;
        
            CTreeNode * pNodeSiteParent = pElement->GetFirstBranch()->GetUpdatedParentLayoutNode();
            if ( pNodeSiteParent )
                pCurElement = pNodeSiteParent->Element();

            if ( pCurElement )
            {
                GetEditContext( pCurElement ,
                                   & pElementEditContext ,
                                   NULL,
                                   NULL );
                
            }
            else
                pElementEditContext = PrimaryRoot();
                
            // if it's the top element, don't defer to itself

            _pElemCurrent = pElementEditContext;
            
            if (_pElemCurrent == pElement)
                _pElemCurrent = PrimaryRoot();

            if ( _pElemCurrent->IsEditable( FALSE ) && _pElemCurrent->_etag != ETAG_ROOT )
            {
                //
                // An editable element has just become current.
                // We tell the mshtmled.dll about this change in editing "context"
                //


                if (_pCaret)
                {
                    _pCaret->Show( FALSE );
                }
                IGNORE_HR( SetEditContext(( _pElemCurrent->HasSlaveMarkupPtr() ? 
                                            _pElemCurrent->GetSlaveMarkupPtr()->FirstElement() : 
                                            _pElemCurrent ), TRUE, FALSE, TRUE ));

            }
            
            IGNORE_HR(OnPropertyChange(DISPID_CDoc_activeElement, 0));
        }
    }

    // Make sure pElement site pointer is not cached by the document, which
    // can happen if we are in the middle of a drag-drop operation
    if (_pDragDropTargetInfo)
    {
        if (pElement == _pDragDropTargetInfo->_pElementTarget)
        {
            _pDragDropTargetInfo->_pElementTarget = NULL;
            _pDragDropTargetInfo->_pElementHit = NULL;
            _pDragDropTargetInfo->_pDispScroller = NULL;
        }
    }

    // Release capture if it owns it

    if (GetCaptureObject() == (void *)pElement)
    {
        if (_state >= OS_INPLACE)
        {
            pElement->TakeCapture(FALSE);
        }
        else
        {
            ClearMouseCapture((void*)pElement);
        }
    }


    if (_pNodeLastMouseOver)
    {
        ClearCachedNodeOnElementExit(&_pNodeLastMouseOver, pElement);
    }
    if (_pNodeGotButtonDown)
    {
        ClearCachedNodeOnElementExit(&_pNodeGotButtonDown, pElement);
    }

    // Reset _pElemUIActive if necessary
    if (_pElemUIActive == pElement)
    {
        // BUGBUG (MohanB) Need to call _pElemUIActive->YieldUI() here ?

        _pElemUIActive = NULL;
        if (_pInPlace && !_pInPlace->_fDeactivating && _pElemUIActive != PrimaryRoot())
        {
            IGNORE_HR(PrimaryRoot()->BecomeUIActive());
        }
    }

    //
    // Remove all traces of pElement from the focus item array
    // and accessKey array.
    //

    // BUGBUG: this could be N^2 on shutdown
    for (i = 0; i < _aryFocusItems.Size(); )
    {
        if (_aryFocusItems[i].pElement == pElement)
        {
            _aryFocusItems.Delete(i);
        }
        else
        {
            i++;
        }
    }

    for (i = 0; i < _aryAccessKeyItems.Size(); i ++)
    {
        if (_aryAccessKeyItems[i] == pElement)
        {
            _aryAccessKeyItems.Delete(i);
            break;
        }
    }

    return S_OK;
}

void
CDoc::ClearCachedNodeOnElementExit(CTreeNode ** ppNodeToTest, CElement * pElement)
{
    Assert(pElement && ppNodeToTest && *ppNodeToTest);

    CTreeNode * pNode = *ppNodeToTest;

    if (SameScope(pNode, pElement))
    {
        *ppNodeToTest = NULL;
        pNode->NodeRelease();
        return;
    }

    if (pElement->HasSlaveMarkupPtr())
    {
        // BUGBUG (MohanB) This will soon be generalized by DBau
        // through a new content markup notification (like
        // (NTYPE_MASTER_EXITVIEW or something)
        CMarkup * pMarkup = pNode->GetMarkup();

        while (pMarkup && pMarkup->Master())
        {
            if (pMarkup->Master() == pElement)
            {
                *ppNodeToTest = NULL;
                pNode->NodeRelease();
                return;
            }
            pMarkup = pMarkup->Master()->GetMarkup();
        }
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::InsertFocusArrayItem
//
//  Synopsis:   Insert CElement with tabindex defined in _aryFocusItems
//
//-------------------------------------------------------------------------

HRESULT
CDoc::InsertFocusArrayItem(CElement * pElement)
{
    HRESULT     hr = S_OK;
    long        lTabIndex  = pElement->GetAAtabIndex();
    long        c;
    long *      pTabs = NULL;
    long *      pTabsSet;
    FOCUS_ITEM  focusitem;
    long        i;
    long        j;

    //
    // The tabIndex attribute can have three values:
    //
    //      < 0     Does not participate in focus
    //     == 0     Tabindex as per source order
    //      > 0     Tabindex is the set tabindex
    //
    // Precedence is that given tab indices go first, then the elements
    // which don't have one assigned.  If not given a tab index, treat the
    // element same as tabIndex < 0 for those elements which don't have a
    // layout associated with them (e.g. <P>, <SPAN>).  Those that do
    // have a layout (e.g. <INPUT>, <BUTTON>) will be treated as tabindex == 0
    //

    //
    // Figure out if this element has subdivisions.  If so, then we need
    // multiple entries for this guy.
    //

    //
    // Areas and maps don't belong in here.
    //

    if (pElement->Tag() == ETAG_AREA || pElement->Tag() == ETAG_MAP)
        goto Cleanup;

    hr = THR(pElement->GetSubDivisionCount(&c));
    if (hr)
        goto Cleanup;

    // Subdivisions can have their own tabIndices. Therefore, if an element
    // has any subdivisions, the element's own tabIndex is ignored.
    if (!c)
    {
        if (lTabIndex <= 0)
            goto Cleanup;
        pElement->_fHasTabIndex = TRUE;
    }
    //
    // Find the location in the focus item array to insert this element.
    //

    focusitem.pElement = pElement;
    if (c)
    {
        pTabs = new long[c];
        if (!pTabs)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pElement->GetSubDivisionTabs(pTabs, c));
        if (hr)
            goto Cleanup;

        pTabsSet = pTabs;
    }
    else
    {
        Assert(lTabIndex > 0);
        pTabsSet = &lTabIndex;
        c = 1;
    }

    hr = THR(_aryFocusItems.EnsureSize(_aryFocusItems.Size() + c));
    if (hr)
        goto Cleanup;

    for (i = 0; i < c; i++)
    {
        //
        // This is here because subdivisions can also be set to have
        // either a negative tabindex or zero, which means they're in source
        // order.
        //

        if (pTabsSet[i] <= 0)
            continue;

        focusitem.lSubDivision = i;
        focusitem.lTabIndex = pTabsSet[i];

        //
        // Find correct location in _aryFocusItems to insert.
        //

        for (j = 0; j < _aryFocusItems.Size(); j++)
        {
            if (_aryFocusItems[j].lTabIndex > focusitem.lTabIndex ||
                _aryFocusItems[j].pElement->GetSourceIndex() >
                    focusitem.pElement->GetSourceIndex())
                break;
        }

        Verify(!_aryFocusItems.InsertIndirect(j, &focusitem));
    }

Cleanup:
    delete [] pTabs;
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::InsertAccessKeyItems
//
//  Synopsis:   Insert CElement with accessKey defined in _aryAccessKeyItems
//
//-------------------------------------------------------------------------
HRESULT
CDoc::InsertAccessKeyItem(CElement * pElement)
{
    HRESULT hr = S_OK;
    long    lSrcIndex = pElement->GetSourceIndex();
    long    lIndex;

    for (lIndex = _aryAccessKeyItems.Size() - 1; lIndex >= 0; lIndex --)
    {
        Assert(_aryAccessKeyItems[lIndex]->GetSourceIndex() != lSrcIndex);
        if (_aryAccessKeyItems[lIndex]->GetSourceIndex() < lSrcIndex)
            break;
    }

    hr = THR(_aryAccessKeyItems.EnsureSize(_aryAccessKeyItems.Size() + 1));
    if (hr)
        goto Cleanup;

    Verify(!_aryAccessKeyItems.Insert(lIndex + 1, pElement));

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::SearchAccessKeyArray
//
//  Synopsis:   Search accessKey array for matched access key
//
//-------------------------------------------------------------------------
void
CDoc::SearchAccessKeyArray(
                FOCUS_DIRECTION  dir,
                CElement       * pElemFirst,
                CElement      ** ppElemNext,
                CMessage       * pmsg)
{
    long       lIndex;
    long       lSize = _aryAccessKeyItems.Size();
    CElement * pElemIndex;

    Assert(ppElemNext);
    * ppElemNext = NULL;

    if (pElemFirst)
    {
        long lsrcIndex = pElemFirst->GetSourceIndex();

        for (lIndex = 0; lIndex < lSize; lIndex ++)
        {
            pElemIndex = _aryAccessKeyItems[lIndex];

            if (pElemIndex->GetSourceIndex() >= lsrcIndex)
            {
                if (pElemIndex->GetSourceIndex() == lsrcIndex)
                {
                    (dir == DIRECTION_FORWARD) ? (lIndex ++) : (lIndex --);
                }
                else if (dir == DIRECTION_BACKWARD)
                {
                    lIndex --;
                }
                break;
            }
        }
        if (lIndex == lSize && dir == DIRECTION_BACKWARD)
        {
            lIndex = -1;
        }
    }
    else
    {
        lIndex = dir == DIRECTION_FORWARD ? 0 : lSize - 1;
    }

    for (; (dir == DIRECTION_FORWARD) ? (lIndex < lSize) : (lIndex >= 0);
           (dir == DIRECTION_FORWARD) ? (lIndex ++) : (lIndex --))
    {
        pElemIndex = _aryAccessKeyItems[lIndex];
        Assert(pElemIndex);

        if (pElemIndex->MatchAccessKey(pmsg) && pElemIndex != pElemFirst)
        {
            FOCUS_ITEM fi = pElemIndex->GetMnemonicTarget();

            if (fi.pElement && fi.pElement->IsFocussable(fi.lSubDivision)) 
            {
                * ppElemNext = pElemIndex;
                break;
            }
        }
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::SearchFocusArray
//
//  Synopsis:   Search the focus array for the next focussable item
//
//  Returns:    FALSE if pElemFirst was not found in the focus arrya
//              TRUE and ppElemNext/plSubNext set to NULL if pElemFirst
//                  was found, but the next item was not present.
//              TRUE and ppElemNext/plSubNext set to valid values if
//                  pElemFirst was found and the next item is also present.
//
//-------------------------------------------------------------------------

BOOL
CDoc::SearchFocusArray(
    FOCUS_DIRECTION dir,
    CElement *pElemFirst,
    long lSubFirst,
    CElement **ppElemNext,
    long *plSubNext)
{
    int         i;
    BOOL        fFound = FALSE;
    CMarkup *   pMarkup = PrimaryMarkup();
    
    *ppElemNext = NULL;
    *plSubNext = 0;

    if (_lFocusTreeVersion != pMarkup->GetMarkupTreeVersion())
    {
        CCollectionCache *  pCollCache;
        CElement *          pElement;
        
        _lFocusTreeVersion = pMarkup->GetMarkupTreeVersion();
        _aryFocusItems.DeleteAll();

        pMarkup->EnsureCollectionCache(CMarkup::ELEMENT_COLLECTION);
        pCollCache = pMarkup->CollectionCache();
        if (!pCollCache)
            return FALSE;
            
        for (i = 0; i < pCollCache->SizeAry(CMarkup::ELEMENT_COLLECTION); i++)
        {
            pCollCache->GetIntoAry(CMarkup::ELEMENT_COLLECTION, i, &pElement);
            if (pElement)
            {
                InsertFocusArrayItem(pElement);
            }
        }
    }
    
    if (!pElemFirst)
    {
        if (DIRECTION_FORWARD == dir && _aryFocusItems.Size() > 0)
        {
            i = 0;
        }
        else if (DIRECTION_BACKWARD == dir && _aryFocusItems.Size() > 0)
        {
            i = _aryFocusItems.Size() - 1;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        //
        // Search for pElemFirst and lSubFirst in the array.
        //

        for (i = 0; i < _aryFocusItems.Size(); i++)
        {
            if (pElemFirst == _aryFocusItems[i].pElement &&
                lSubFirst == _aryFocusItems[i].lSubDivision)
            {
                fFound = TRUE;
                break;
            }
        }

        //
        // If pElemFirst is not in the array, just return FALSE.  This
        // will cause SearchFocusTree to get called.
        //

        if (!fFound)
            return FALSE;

        //
        // If pElemFirst is the first/last element in the array
        // return TRUE.  Otherwise return the next tabbable element.
        //

        if (DIRECTION_FORWARD == dir)
        {
            if (i == _aryFocusItems.Size() - 1)
                return TRUE;

            i++;    // Make i point to the item to return
        }
        else
        {
            if (i == 0)
                return TRUE;

            i--;
        }
    }

    *ppElemNext = _aryFocusItems[i].pElement;
    *plSubNext = _aryFocusItems[i].lSubDivision;
    return TRUE;
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::SearchFocusTree
//
//  Synopsis:   Update the focus collection upon an element exiting the tree
//
//  Returns:    See SearchFocusArray
//
//-------------------------------------------------------------------------

BOOL
CDoc::SearchFocusTree(
    FOCUS_DIRECTION dir,
    CElement *pElemFirst,
    long lSubFirst,
    CElement **ppElemNext,
    long *plSubNext)
{
    //
    // Use the all collection for now.  BUGBUG: (anandra) Fix ASAP.
    //

    HRESULT             hr = S_OK;
    long                i;
    long                cElems;
    CElement *          pElement;
    BOOL                fStopOnNextTab = FALSE;
    long                lSubNext;
    int                 iStep;
    BOOL                fFound = FALSE;
    CCollectionCache *  pCollectionCache;

    *ppElemNext = NULL;
    *plSubNext = 0;

    hr = THR(PrimaryMarkup()->EnsureCollectionCache(CMarkup::ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    pCollectionCache = PrimaryMarkup()->CollectionCache();

    cElems = pCollectionCache->SizeAry(CMarkup::ELEMENT_COLLECTION);
    if (!cElems)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // Search for pElemFirst
    //

    iStep = (DIRECTION_FORWARD == dir) ? 1 : -1;

    for (i = (DIRECTION_FORWARD == dir) ? 0 : cElems - 1;
         (DIRECTION_FORWARD == dir) ? (i < cElems) : (i >= 0);
         i += iStep)
    {
        hr = THR(pCollectionCache->GetIntoAry(
                CMarkup::ELEMENT_COLLECTION,
                i,
                &pElement));
        if (hr)
            goto Cleanup;

        Assert(pElement);

        //
        // If the _fHasTabIndex bit is set, this element has already
        // been looked at in SearchFocusArray.
        //

        if (pElement->_fHasTabIndex)
            continue;

        if (pElemFirst)
        {
            if (pElemFirst == pElement)
            {
                //
                // Found the element.  Now check if there are any further
                // subdivisions.  If so, then we need to return the next
                // subdivision.  If we're on the last subdivision already
                // or if there are no subdivisions, then we need to return
                // the next tabbable object.
                //

                fFound = TRUE;
                Assert(lSubFirst != -1);
                lSubNext = lSubFirst;

                for(;;)
                {
                    hr = THR(pElement->GetNextSubdivision(dir, lSubNext, &lSubNext));
                    if (hr)
                        goto Cleanup;

                    if (lSubNext == -1)
                        break;

                    if (pElement->IsTabbable(lSubNext))
                    {
                        *ppElemNext = pElement;
                        *plSubNext = lSubNext;
                        goto Cleanup;
                    }
                }

                fStopOnNextTab = TRUE;
                continue;
            }
        }
        else
        {
            fStopOnNextTab = TRUE;
        }

        if (fStopOnNextTab)
        {
            hr = THR(pElement->GetNextSubdivision(dir, -1, &lSubNext));
            if (hr)
                goto Cleanup;
            while (lSubNext != -1)
            {
                if (pElement->IsTabbable(lSubNext))
                {
                    fFound = TRUE;
                    *ppElemNext = pElement;
                    *plSubNext = lSubNext;
                    goto Cleanup;
                }
                hr = THR(pElement->GetNextSubdivision(dir, lSubNext, &lSubNext));
                if (hr)
                    goto Cleanup;
            }
        }
    }

Cleanup:
#if DBG == 1
    if (*ppElemNext)
    {
        Assert(fFound);
    }

    if (hr)
    {
        Assert(!fFound && !*ppElemNext);
    }
#endif
    return fFound;
}

HRESULT CMarkup::InitCollections ( void )
{
    HRESULT hr = S_OK;
    CAllCollectionCacheItem *pAllCollection = NULL;
    CCollectionCache *pCollectionCache;

    // InitCollections should not be called more than once successfully.
    if (HasCollectionCache())
        goto Cleanup;

    pCollectionCache =
        new CCollectionCache(
            this,
            _pDoc,
            ENSURE_METHOD(CMarkup, EnsureCollections, ensurecollections ) );

    if (!pCollectionCache)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Initialize from the FORM_COLLECTION onwards,. leaving out the ELEMENT_COLLECTION
    hr = THR(pCollectionCache->InitReservedCacheItems( NUM_DOCUMENT_COLLECTIONS, FORMS_COLLECTION ));
    if (hr)
        goto Cleanup;

    pAllCollection = new CAllCollectionCacheItem();
    if ( !pAllCollection )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pAllCollection->SetMarkup(this);

    hr = THR(pCollectionCache->InitCacheItem ( ELEMENT_COLLECTION, pAllCollection ));
    if (hr)
        goto Cleanup;

    //
    // Collection cache now owns this item & is responsible for freeing it
    //

    // Turn off the default name promotion behaviour on collections
    // that don't support it in Nav.
    pCollectionCache->DontPromoteNames(ANCHORS_COLLECTION);
    pCollectionCache->DontPromoteNames(LINKS_COLLECTION);

    // The frames collection resolves ordinal access on the window object
    // so turn off the WINDOW_COLLECTIOn resultion of ordinals
    pCollectionCache->DontPromoteOrdinals(WINDOW_COLLECTION);


    // Because of VBScript compatability issues we create a dynamic type library
    // (See CDoc::BuildObjectTypeInfo()). The dynamic typeinfo contains
    // DISPIDs starting from DISPID_COLLECTION_MIN, & occupying half the DISPID space.


    // We either get Invokes from these DISPIDs or from the WINDOW_COLLECTION GIN/GINEX name resolution
    // OR from the FRAMES_COLLECTION. So we divide the avaliable DISPID range up among these
    // Three 'collections' - 2 real collections & one hand-cooked collection.

    // Divide up the WINDOW_COLLECTION DISPID's so we can tell where the Invoke came from

    // Give the lowest third to the dynamic type library
    // DISPID_COLLECTION_MIN .. (DISPID_COLLECTION_MIN+DISPID_COLLECTION_MAX)/3

    // Give the next third to names resolved on the WINDOW_COLLECTION
    pCollectionCache->SetDISPIDRange ( WINDOW_COLLECTION,
            (DISPID_COLLECTION_MIN+DISPID_COLLECTION_MAX)/3+1,
            ((DISPID_COLLECTION_MIN+DISPID_COLLECTION_MAX)*2)/3 );

    // In COmWindowProxy::Invoke the security code allows DISPIDs from the frames collection
    // through with no security check. Check that the DISPIDs reserved for this range match up
    Assert ( FRAME_COLLECTION_MIN_DISPID == pCollectionCache->GetMaxDISPID(WINDOW_COLLECTION)+1 );
    Assert ( FRAME_COLLECTION_MAX_DISPID == DISPID_COLLECTION_MAX );

    // Give the final third to the FRAMES_COLLECTION
    pCollectionCache->SetDISPIDRange ( FRAMES_COLLECTION,
            pCollectionCache->GetMaxDISPID(WINDOW_COLLECTION)+1,
            DISPID_COLLECTION_MAX );

    // Like NAV, we want to return the last frame that matches the name asked for,
    // rather than returning a collection.
    pCollectionCache->AlwaysGetLastMatchingCollectionItem ( FRAMES_COLLECTION );

    // Like NAV, the images doesn't return sub-collections. Note that we might put multiple
    // entries in the IMG collection for the same IMG if the IMG is nested in a table - so
    // this also prevents us from retunring a sub-collection in this situation.
    pCollectionCache->AlwaysGetLastMatchingCollectionItem ( IMAGES_COLLECTION );

    // Setup the lookaside variable
    hr = THR(SetLookasidePtr(LOOKASIDE_COLLECTIONCACHE, pCollectionCache));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}



CElement *CAllCollectionCacheItem::GetNext ( void )
{
    return GetAt ( _lCurrentIndex++ );
}

CElement *CAllCollectionCacheItem::MoveTo ( long lIndex )
{
    _lCurrentIndex = lIndex;
    return NULL;
}

CElement *CAllCollectionCacheItem::GetAt ( long lIndex )
{
    CMarkup * pMarkup = GetMarkup();
    CTreePos *ptpBegin;

    Assert( pMarkup );

    Assert ( lIndex >= 0 );

    // Skip ETAG_ROOT which is always the zero'th element
    lIndex++;

    if ( lIndex >= pMarkup->NumElems() )
        return NULL;

    ptpBegin = pMarkup->TreePosAtSourceIndex ( lIndex );

    Assert (ptpBegin && ptpBegin->IsBeginElementScope() );

    return ptpBegin->Branch()->Element();
}

long CAllCollectionCacheItem::Length ( void )
{
    CMarkup *pMarkup = GetMarkup();
    Assert ( pMarkup && pMarkup->NumElems() >= 1 );
    return pMarkup->NumElems()-1;
}



