#include "headers.hxx"

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_DOM_HXX_
#define X_DOM_HXX_
#include "dom.hxx"
#endif

#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif

#ifndef X_DOMCOLL_HXX_
#define X_DOMCOLL_HXX_
#include "domcoll.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#define _cxx_
#include "dom.hdl"

////////////////////////////////////////////////////////////////////////////////
//
// DOM Helper methods:
//
////////////////////////////////////////////////////////////////////////////////

static 
HRESULT 
CrackDOMNode ( IUnknown *pNode, CDOMTextNode**ppTextNode, CElement **ppElement, CDoc *pThisDoc )
{
    HRESULT hr = THR(pNode->QueryInterface(CLSID_CElement, (void **)ppElement));
    if (hr)
    {
        if (ppTextNode)
        {
            hr = THR(pNode->QueryInterface(CLSID_HTMLDOMTextNode, (void **)ppTextNode));
            if (!hr && (pThisDoc != (*ppTextNode)->Doc()))
                hr = E_INVALIDARG;
        }
    }
    else if (pThisDoc != (*ppElement)->Doc())
        hr = E_INVALIDARG;

    RRETURN ( hr );
}

static 
HRESULT 
CrackDOMNodeVARIANT ( VARIANT *pVarNode, CDOMTextNode**ppTextNode, CElement **ppElement, CDoc *pThisDoc )
{
    HRESULT hr = S_OK;

    switch (V_VT(pVarNode))
    {
    case VT_DISPATCH:
    case VT_UNKNOWN:
        if (V_UNKNOWN(pVarNode))
        {
            // Element OR Text Node ??
            hr = THR(CrackDOMNode(V_UNKNOWN(pVarNode), ppTextNode, ppElement, pThisDoc));
        }
        break;
    case VT_NULL:
    case VT_ERROR:
        hr = S_OK;
        break;

    default:
        hr = E_INVALIDARG;
        break;
    }   
    RRETURN ( hr );
}

static
HRESULT
GetDOMInsertHelper ( CElement *pRefElement, CDOMTextNode *pRefTextNode, CMarkupPointer *pmkptrPos )
{
    HRESULT hr = E_UNEXPECTED;

    Assert ( pRefElement || pRefTextNode );
    Assert ( !(pRefElement && pRefTextNode ) );

    if ( pRefElement )
    {
        // Element Insert
        if (!pRefElement->IsInMarkup())
            goto Cleanup;

        hr = THR(pmkptrPos->MoveAdjacentToElement ( pRefElement, ELEM_ADJ_BeforeBegin ));
        if ( hr )
           goto Cleanup;
    }
    else 
    {
        // Text Node Insert
        // Reposition the text node, then confirm we are it's parent
        CMarkupPointer *pmkpTextPtr;

        hr = THR(pRefTextNode->GetMarkupPointer ( &pmkpTextPtr ));
        if ( hr )
            goto Cleanup;
        hr = THR(pmkptrPos->MoveToPointer ( pmkpTextPtr ));
        if ( hr )
           goto Cleanup;
    }

Cleanup:
    RRETURN ( hr );
}


static 
HRESULT 
InsertDOMNodeHelper ( CElement *pNewElement, CDOMTextNode *pNewTextNode, CMarkupPointer *pmkptrTarget )
{
    HRESULT hr = S_OK;

    Assert ( pNewTextNode || pNewElement );

    if ( pNewElement )
    {
        CDoc *pDoc = pNewElement->Doc();

        // Insert/Move element with content
        if (pNewElement->IsInMarkup() && !pNewElement->IsNoScope()) 
        {
            CMarkupPointer mkptrStart(pDoc);
            CMarkupPointer mkptrEnd(pDoc);

            hr = THR(pNewElement->GetMarkupPtrRange (&mkptrStart, &mkptrEnd) );
            if ( hr )
                goto Cleanup;

            hr = THR(pDoc->Move(&mkptrStart, &mkptrEnd, pmkptrTarget, MUS_DOMOPERATION));
        }
        else
        {
            if (pNewElement->IsInMarkup())
            {
                hr = THR(pDoc->RemoveElement(pNewElement, MUS_DOMOPERATION));
                if ( hr )
                    goto Cleanup;
            }

            hr = THR(pDoc->InsertElement(pNewElement, pmkptrTarget, NULL, MUS_DOMOPERATION));
        }

        if (hr)
            goto Cleanup;
    }
    else
    {
        // Insert Text content
        hr = THR(pNewTextNode->MoveTo ( pmkptrTarget ));
        if (hr)
            goto Cleanup;       
    }

Cleanup:
    RRETURN(hr);
}

static 
HRESULT 
RemoveDOMNodeHelper ( CDoc *pDoc, CElement *pChildElement, CDOMTextNode *pChildTextNode )
{ 
    HRESULT hr = S_OK;
    CMarkup *pMarkupTarget = NULL;

    Assert ( pChildTextNode || pChildElement );

    if ( pChildTextNode )
    {
        // Removing a TextNode
        hr = THR(pChildTextNode->Remove());
    }
    else if (pChildElement->IsInMarkup())
    {
        // Removing an element
        if (!pChildElement->IsNoScope())
        {
            CMarkupPointer mkptrStart ( pDoc );
            CMarkupPointer mkptrEnd ( pDoc );
            CMarkupPointer mkptrTarget ( pDoc );

            hr = THR(pDoc->CreateMarkup(&pMarkupTarget));
            if ( hr )
                goto Cleanup;

            hr = THR(mkptrTarget.MoveToContainer(pMarkupTarget,TRUE));
            if (hr)
                goto Cleanup;

            hr = THR(pChildElement->GetMarkupPtrRange (&mkptrStart, &mkptrEnd) );
            if ( hr )
                goto Cleanup;

            hr = THR(pDoc->Move(&mkptrStart, &mkptrEnd, &mkptrTarget, MUS_DOMOPERATION));
        }
        else
            hr = THR(pDoc->RemoveElement(pChildElement, MUS_DOMOPERATION));
    }

Cleanup:
    ReleaseInterface ( (IUnknown*)pMarkupTarget ); // Creating it addref'd it once
    RRETURN(hr);
}


static
HRESULT 
ReplaceDOMNodeHelper ( CDoc *pDoc, CElement *pTargetElement, CDOMTextNode *pTargetNode,
    CElement *pSourceElement, CDOMTextNode *pSourceTextNode )
{
    CMarkupPointer mkptrInsert ( pDoc );
    HRESULT hr;

    // Position ourselves in the right spot
    hr = THR(GetDOMInsertHelper ( pTargetElement, pTargetNode, &mkptrInsert ));
    if ( hr )
        goto Cleanup;

    mkptrInsert.SetGravity ( POINTER_GRAVITY_Left );

    {
        // Lock the markup, to prevent it from going away in case the entire contents are being removed.
        CMarkup::CLock MarkupLock(mkptrInsert.Markup());

        // Remove myself
        hr = THR(RemoveDOMNodeHelper ( pDoc, pTargetElement, pTargetNode ));
        if ( hr )
            goto Cleanup;

        // Insert the new element & all its content
        hr = THR(InsertDOMNodeHelper( pSourceElement, pSourceTextNode, &mkptrInsert ));
        if ( hr )
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

static
HRESULT
SwapDOMNodeHelper ( CDoc *pDoc, CElement *pElem, CDOMTextNode *pTextNode,
                   CElement *pElemSwap, CDOMTextNode *pTextNodeSwap )
{
    CMarkupPointer  mkptrThisInsert ( pDoc );
    CMarkupPointer  mkptrSwapInsert ( pDoc );
    HRESULT hr;

    // Position ourselves in the right spot
    if (!pElem || pElem->IsInMarkup())
    {
        hr = THR(GetDOMInsertHelper ( pElem, pTextNode, &mkptrThisInsert ));
        if (hr)
            goto Cleanup;
    }

    if (!pElemSwap || pElemSwap->IsInMarkup())
    {
        hr = THR(GetDOMInsertHelper ( pElemSwap, pTextNodeSwap, &mkptrSwapInsert ));
        if (hr)
            goto Cleanup;
    }

    // Lock the markup, to prevent it from going away in case the entire contents are being removed.
    if (mkptrSwapInsert.Markup())
        mkptrSwapInsert.Markup()->AddRef();

    // Insert the new element & all its content
    if (mkptrThisInsert.IsPositioned())
        hr = THR(InsertDOMNodeHelper( pElemSwap, pTextNodeSwap, &mkptrThisInsert ));
    else
        hr = THR(RemoveDOMNodeHelper(pDoc, pElemSwap, pTextNodeSwap));

    if ( hr )
        goto Cleanup;

    // Insert the new element & all its content
    if (mkptrSwapInsert.IsPositioned())
    {
        hr = THR(InsertDOMNodeHelper( pElem, pTextNode, &mkptrSwapInsert ));
        mkptrSwapInsert.Markup()->Release();
    }
    else
        hr = THR(RemoveDOMNodeHelper(pDoc, pElem, pTextNode));

Cleanup:
    RRETURN(hr);
}

static
HRESULT
CreateTextNode ( CDoc *pDoc, CMarkupPointer *pmkpTextEnd, long lTextID, CDOMTextNode **ppTextNode ) 
{                        
    CMarkupPointer *pmarkupWalkBegin = NULL;
    HRESULT hr = S_OK;

    Assert ( ppTextNode );
    Assert ( !(*ppTextNode) );

    if ( lTextID != 0 )
    {
        *ppTextNode = (CDOMTextNode *)pDoc->
            _HtPvPvDOMTextNodes.Lookup ( (void *)(DWORD_PTR)(lTextID<<4) );
    }

    if ( !(*ppTextNode) )
    {
        // Need to create a Text Node
        pmarkupWalkBegin =  new CMarkupPointer (pDoc); 
        if ( !pmarkupWalkBegin )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pmarkupWalkBegin->MoveToPointer ( pmkpTextEnd ));
        if ( hr )
            goto Cleanup;

        hr = THR(pmarkupWalkBegin->Left( TRUE, NULL, NULL, NULL, NULL, &lTextID));
        if ( hr )
            goto Cleanup;

        if ( lTextID == 0 )
        {
            hr = THR(pmarkupWalkBegin->SetTextIdentity(pmkpTextEnd, &lTextID));
            if ( hr )
                goto Cleanup;
        }

        // Stick to the text
        hr = THR(pmarkupWalkBegin->SetGravity (POINTER_GRAVITY_Right));
        if ( hr )
            goto Cleanup;   
        // BUGBUG Need to set Glue Also!
        
        Assert( lTextID != 0 );

        *ppTextNode = new CDOMTextNode ( lTextID, pDoc, pmarkupWalkBegin );
        if ( !*ppTextNode )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        // Now give ownership of this markup pointer to the text Node
        pmarkupWalkBegin = NULL;
        
        hr = THR(pDoc->_HtPvPvDOMTextNodes.Insert ( (void*)(DWORD_PTR)(lTextID<<4), (void*)*ppTextNode ) );
        if ( hr )
            goto Cleanup;
    }
    else
    {
        (*ppTextNode)->AddRef();
    }


Cleanup:
    delete pmarkupWalkBegin; 
    RRETURN(hr);
}


static
HRESULT
GetPreviousHelper ( CDoc *pDoc, CMarkupPointer *pmarkupWalk, IHTMLDOMNode **ppNode )
{
    HRESULT hr = S_OK;
    CTreeNode *pNodeLeft;
    MARKUP_CONTEXT_TYPE context;
    long lTextID;

    if (!ppNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppNode = NULL;
    
    hr = THR(pmarkupWalk->Left( FALSE, &context, &pNodeLeft, NULL, NULL, &lTextID));
    if ( hr )
        goto Cleanup;

    switch ( context )
    {
    case CONTEXT_TYPE_Text:
        {
            // Text Node
            CDOMTextNode *pTextNode = NULL;

            // If we find text we ccannot have left the scope of our parent
            hr = THR(CreateTextNode (pDoc, pmarkupWalk, lTextID, 
                &pTextNode ));
            if ( hr )
                goto Cleanup;

            hr = THR ( pTextNode->QueryInterface ( IID_IHTMLDOMNode, (void **)ppNode ) );
            if ( hr )
                goto Cleanup;
            pTextNode->Release();
        }
        break;

    case CONTEXT_TYPE_EnterScope:
    case CONTEXT_TYPE_NoScope:
        // Return Disp to Element
        hr = THR(pNodeLeft->GetElementInterface ( IID_IHTMLDOMNode, (void **) ppNode  ));
        if ( hr )
            goto Cleanup;
        break;

    case CONTEXT_TYPE_ExitScope:
    case CONTEXT_TYPE_None:
        break;

    default:
        Assert(FALSE); // Should never happen
        break;
    }
Cleanup:
    RRETURN(hr);
}


static
HRESULT
GetNextHelper ( CDoc *pDoc, CMarkupPointer *pmarkupWalk, IHTMLDOMNode **ppNode )
{
    HRESULT hr = S_OK;
    CTreeNode           *pnodeRight;
    MARKUP_CONTEXT_TYPE context;
    long lTextID;

    if (!ppNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppNode = NULL;
   
    hr = THR( pmarkupWalk->Right( TRUE, &context, &pnodeRight, NULL, NULL, &lTextID));
    if ( hr )
        goto Cleanup;

    switch ( context )
    {
    case CONTEXT_TYPE_Text:
        {
            // Text Node
            CDOMTextNode *pTextNode = NULL;

            // If we find text we ccannot have left the scope of our parent
            hr = THR(CreateTextNode (pDoc, pmarkupWalk, lTextID, 
                &pTextNode ));
            if ( hr )
                goto Cleanup;

            hr = THR ( pTextNode->QueryInterface ( IID_IHTMLDOMNode, (void **)ppNode ) );
            if ( hr )
                goto Cleanup;
            pTextNode->Release();
        }
        break;

    case CONTEXT_TYPE_EnterScope:
    case CONTEXT_TYPE_NoScope:
        // Return Disp to Element
        hr = THR(pnodeRight->GetElementInterface ( IID_IHTMLDOMNode, (void **) ppNode  ));
        if ( hr )
            goto Cleanup;
        break;

    case CONTEXT_TYPE_ExitScope:
    case CONTEXT_TYPE_None:
        break;

    default:
        Assert(FALSE); // Should never happen
        break;
    }
Cleanup:
    RRETURN(hr);
}
////////////////////////////////////////////////////////////////////////////////
//
// IHTMLDOMTextNode methods:
//
////////////////////////////////////////////////////////////////////////////////

MtDefine(CDOMTextNode, ObjectModel, "CDOMTextNode")

//+----------------------------------------------------------------
//
//  member : classdesc
//
//+----------------------------------------------------------------

const CBase::CLASSDESC CDOMTextNode::s_classdesc =
{
    &CLSID_HTMLDOMTextNode,                           // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLDOMTextNode,         // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

HRESULT
CDOMTextNode::PrivateQueryInterface(REFIID iid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;

    if ( !ppv )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IHTMLDOMTextNode, NULL)
        QI_TEAROFF(this, IHTMLDOMNode, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
    }

    if ( iid == CLSID_HTMLDOMTextNode )
    {
        *ppv = (void*)this;
        return S_OK;
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }
Cleanup:
    return hr;
}

HRESULT
CDOMTextNode::get_data(BSTR *pbstrData)
{
    HRESULT hr = S_OK;
    MARKUP_CONTEXT_TYPE context;
    long lCharCount = -1;
    long lTextID;


    if ( !pbstrData )
        goto Cleanup;

    *pbstrData = NULL;

    // Move the markup pointer adjacent to the Text
    hr = THR( _pMkpPtr->FindTextIdentity ( _lTextID, (CMarkupPointer * )NULL ) );
    if ( hr )
        goto Cleanup;

    // Walk right (hopefully) over text, inital pass, don't move just get count of chars
    hr = THR( _pMkpPtr->Right( FALSE, &context, NULL, &lCharCount, NULL, &lTextID));
    if ( hr )
        goto Cleanup;

    Assert ( lTextID == _lTextID );

    if ( context == CONTEXT_TYPE_Text )
    {
        // Alloc memory
        hr = FormsAllocStringLen ( NULL, lCharCount, pbstrData );
        if ( hr )
            goto Cleanup;
        hr = THR( _pMkpPtr->Right( FALSE, &context, NULL, &lCharCount, *pbstrData, &lTextID));
        if ( hr )
            goto Cleanup;
    }  

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

CDOMTextNode::CDOMTextNode ( long lTextID, CDoc *pDoc, CMarkupPointer *pmkptr )
{ 
    Assert(pDoc);
    Assert(pmkptr);
    _pDoc = pDoc ; 
    _lTextID = lTextID ;
    _pDoc->AddRef();                // Due to lookaside table
    _pMkpPtr = pmkptr;
    // markup ptr will manage Markup in which text node is in.
    _pMkpPtr->SetKeepMarkupAlive(TRUE);
    // Add glue so that tree services can manage ptr movement
    // between markups automatically during splice operations
    _pMkpPtr->SetCling(TRUE);
}


CDOMTextNode::~CDOMTextNode()
{

    // tidy up lookaside
    _pDoc->_HtPvPvDOMTextNodes.Remove ( (void *)(DWORD_PTR)(_lTextID<<4) );

    // Tidy up markup ptr/markup
    _pMkpPtr->SetKeepMarkupAlive(FALSE);
    _pMkpPtr->SetCling(FALSE);

    _pDoc->Release();
    delete _pMkpPtr;
}


HRESULT STDMETHODCALLTYPE
CDOMTextNode::IsEqualObject(IUnknown * pUnk)
{
    HRESULT         hr;
    IUnknown   *    pUnkThis = NULL;
    CDOMTextNode *  pTargetTextNode;

    if (!pUnk)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // Test standard COM identity first
    hr = THR_NOTRACE(QueryInterface(IID_IUnknown, (void **)&pUnkThis));
    if (hr)
        goto Cleanup;

    if (pUnk == pUnkThis)
    {
        hr = S_OK;
        goto Cleanup;
    }

    // Do the dodgy CLSID QI
    hr = THR(pUnk->QueryInterface(CLSID_HTMLDOMTextNode, (void**)&pTargetTextNode));
    if ( hr )
        goto Cleanup;

    hr = (_lTextID == pTargetTextNode->_lTextID) ? S_OK : S_FALSE;

Cleanup:
    ReleaseInterface(pUnkThis);
    RRETURN1(hr, S_FALSE);
}

HRESULT
CDOMTextNode::cloneNode(VARIANT_BOOL fDeep, IHTMLDOMNode **ppNodeCloned)
{
    HRESULT hr;
    CMarkupPointer mkpRight (_pDoc );
    CMarkupPointer mkpTarget (_pDoc );
    CMarkup *pMarkup = NULL;
    CMarkupPointer *pmkpStart = NULL;

    if ( !ppNodeCloned )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(_pDoc->CreateMarkup ( &pMarkup ));
    if ( hr )
        goto Cleanup;

    // Move the markup pointer adjacent to the Text
    hr = THR( _pMkpPtr->FindTextIdentity ( _lTextID, &mkpRight ) );
    if ( hr )
        goto Cleanup;

    hr = THR(mkpTarget.MoveToContainer( pMarkup, TRUE ) );
    if ( hr )
        goto Cleanup;

    mkpTarget.SetGravity ( POINTER_GRAVITY_Right );
    
    // Copy the markup across to the new markup container
    // (mkpTarget gets moved to the right of the text as a result)
    hr = THR( _pDoc->Copy ( _pMkpPtr, &mkpRight, &mkpTarget, MUS_DOMOPERATION ) );  
    if ( hr )
        goto Cleanup;

    // Create the new text node markup pointer - will be handed off to text node
    pmkpStart = new CMarkupPointer ( _pDoc );
    if ( !pmkpStart )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pmkpStart->MoveToContainer( pMarkup, TRUE ) );
    if ( hr )
        goto Cleanup;

    // Create the new text node
    hr = THR(_pDoc->CreateDOMTextNodeHelper(pmkpStart, &mkpTarget, ppNodeCloned ));
    if ( hr )
        goto Cleanup;

    pmkpStart = NULL; // Text Node owns the pointer

Cleanup:
    ReleaseInterface ( (IUnknown*)pMarkup ); // Text node addref'ed it
    delete pmkpStart;
    RRETURN( hr );
}

HRESULT
CDOMTextNode::splitText(long lOffset, IHTMLDOMNode**ppRetNode )
{
    HRESULT hr = S_OK;
    CMarkupPointer mkpRight (_pDoc);
    CMarkupPointer mkpEnd (_pDoc);
    CMarkupPointer *pmkpStart = NULL;
    long lMove = lOffset;
    long lTextID;
    
    if ( lOffset < 0 || !ppRetNode)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    // Split the text at the given offset to make a new text node, return the new text node

    pmkpStart = new CMarkupPointer ( _pDoc );
    if ( !pmkpStart )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Move the markup pointer adjacent to the Text
    hr = THR( _pMkpPtr->FindTextIdentity ( _lTextID, &mkpRight ) );
    if ( hr )
        goto Cleanup;

    hr = THR(pmkpStart->MoveToPointer ( _pMkpPtr ));
    if ( hr )
        goto Cleanup;

    lMove = lOffset;

    hr = THR(pmkpStart->Right ( TRUE, NULL, NULL, &lOffset, NULL, &lTextID ));
    if ( hr )
        goto Cleanup;

    // Position at point of split 
    hr = THR(mkpEnd.MoveToPointer (pmkpStart));
    if ( hr )
        goto Cleanup;

    // If my TextID changed, I moved outside my text node
    if ( lOffset != lMove )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Create the new text node
    hr = THR(_pDoc->CreateDOMTextNodeHelper(pmkpStart, &mkpRight, ppRetNode ));
    if ( hr )
        goto Cleanup;

    pmkpStart = NULL; // New Text Node owns pointer

    // Re-Id the split text for original text node
    hr = THR(_pMkpPtr->SetTextIdentity(&mkpEnd, &lTextID));
    if ( hr )
        goto Cleanup;

    // Update the Doc Text Node Ptr Lookup
    _pDoc->_HtPvPvDOMTextNodes.Remove ( (void *)(DWORD_PTR)(_lTextID<<4) );
    _lTextID = lTextID;
    hr = THR(_pDoc->_HtPvPvDOMTextNodes.Insert ( (void*)(DWORD_PTR)(_lTextID<<4), (void*)this ) );
    if ( hr )
        goto Cleanup;

Cleanup:
    delete pmkpStart;

    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::put_data(BSTR bstrData)
{
    HRESULT hr = S_OK;
    CMarkupPointer mkpRight (_pDoc);
    CMarkupPointer mkpStart (_pDoc);
    long lNewTextID;

    // Move the markup pointer adjacent to the Text
    hr = THR( _pMkpPtr->FindTextIdentity ( _lTextID, &mkpRight ) );
    if ( hr )
        goto Cleanup;

    Assert(_pMkpPtr->Cling());

    // Set left gravity so that _pMkpPtr is not unpositioned, due to remove.
    _pMkpPtr->SetGravity ( POINTER_GRAVITY_Left );

    // Remove the old Text
    hr = THR(_pDoc->Remove ( _pMkpPtr,  &mkpRight, MUS_DOMOPERATION ));
    if ( hr )
        goto Cleanup;

    // OK, old text is out, put the new stuff in
    hr = THR(mkpStart.MoveToPointer ( _pMkpPtr ));
    if ( hr )
        goto Cleanup;
    
    // restore gravity of _pMkpPtr to right, so that mkpStart (which has left
    // gravity) can be positioned after _pMkpPtr and text when new text is inserted
    _pMkpPtr->SetGravity ( POINTER_GRAVITY_Right );

    hr = THR( _pMkpPtr->Doc()->InsertText( _pMkpPtr, bstrData, -1, MUS_DOMOPERATION ) );
    if ( hr )
        goto Cleanup;

    hr = THR(_pMkpPtr->SetTextIdentity(&mkpStart, &lNewTextID));
    if ( hr )
        goto Cleanup;

    // Update the Doc Text Node Ptr Lookup
    _pDoc->_HtPvPvDOMTextNodes.Remove ( (void *)(DWORD_PTR)(_lTextID<<4) );
    _lTextID = lNewTextID;
    hr = THR(_pDoc->_HtPvPvDOMTextNodes.Insert ( (void*)(DWORD_PTR)(_lTextID<<4), (void*)this ) );
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::toString(BSTR *pbstrData)
{
    RRETURN(SetErrorInfo(get_data(pbstrData)));
}

HRESULT
CDOMTextNode::get_length(long *pLength)
{
    HRESULT hr = S_OK;
    long lCharCount = -1;
    long lTextID;
    MARKUP_CONTEXT_TYPE context;

    if ( !pLength )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pLength = 0;

    // Move the markup pointer adjacent to the Text
    hr = THR( _pMkpPtr->FindTextIdentity ( _lTextID, (CMarkupPointer *)NULL ) );
    if ( hr )
        goto Cleanup;

    // Walk right (hopefully) over text, inital pass, don't move just get count of chars
    hr = THR( _pMkpPtr->Right( FALSE, &context, NULL, &lCharCount, NULL, &lTextID));
    if ( hr )
        goto Cleanup;

    Assert ( lTextID == _lTextID );

    if ( context == CONTEXT_TYPE_Text )
    {
        *pLength = lCharCount;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CDOMTextNode::get_nodeType(long *pnodeType)
{
    HRESULT hr = S_OK;

    if (!pnodeType)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *pnodeType = 3; // DOM_TEXTNODE
Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CDOMTextNode::get_nodeName(BSTR *pbstrNodeName)
{
    HRESULT hr = S_OK;
    if (!pbstrNodeName)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrNodeName = NULL;
    
    hr = THR(FormsAllocString ( _T("#text"), pbstrNodeName ));
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::get_nodeValue(VARIANT *pvarValue)
{
    HRESULT hr = S_OK;
    if (!pvarValue)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    pvarValue->vt = VT_BSTR;

    hr = THR(get_data(&V_BSTR(pvarValue)));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::put_nodeValue(VARIANT varValue)
{
    HRESULT hr = S_OK;
    CVariant varBSTRValue;

    hr = THR(varBSTRValue.CoerceVariantArg ( &varValue, VT_BSTR ));
    if ( hr )
        goto Cleanup;

    hr = THR( put_data ( V_BSTR((VARIANT *)&varBSTRValue)));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::get_firstChild ( IHTMLDOMNode **ppNode )
{
    if (ppNode)
        *ppNode = NULL;

    RRETURN(SetErrorInfo(S_OK));
}

HRESULT
CDOMTextNode::get_lastChild ( IHTMLDOMNode **ppNode )
{
    if (ppNode)
        *ppNode = NULL;

    RRETURN(SetErrorInfo(S_OK));
}

HRESULT
CDOMTextNode::get_previousSibling ( IHTMLDOMNode **ppNode )
{
    HRESULT hr = S_OK;
    CMarkupPointer markupWalk ( _pDoc );

    // Move the markup pointer adjacent to the Text
    hr = THR(_pMkpPtr->FindTextIdentity ( _lTextID, (CMarkupPointer *)NULL ));
    if ( hr )
        goto Cleanup;

    hr = THR(markupWalk.MoveToPointer(_pMkpPtr));
    if ( hr )
        goto Cleanup;

    hr = THR(GetPreviousHelper ( _pDoc, &markupWalk, ppNode ));
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::get_nextSibling ( IHTMLDOMNode **ppNode )
{
    HRESULT hr = S_OK;
    CMarkupPointer markupWalk ( _pDoc );

    // Move the markup pointer adjacent to the Text
    hr = THR(_pMkpPtr->FindTextIdentity ( _lTextID, &markupWalk ));
    if ( hr )
        goto Cleanup;

    hr = THR(GetNextHelper ( _pDoc, &markupWalk, ppNode ));
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CDOMTextNode::get_parentNode(IHTMLDOMNode **pparentNode)
{
    HRESULT             hr = S_OK;
    CTreeNode           *pNode;

    if (!pparentNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pparentNode = NULL;

    // Move the markup pointer adjacent to the Text
    hr = THR( _pMkpPtr->FindTextIdentity ( _lTextID, (CMarkupPointer *)NULL ) );
    if ( hr )
        goto Cleanup;

    pNode = _pMkpPtr->CurrentScope();

    if (!pNode || pNode->Tag() == ETAG_ROOT)
    {
        goto Cleanup;
    }

    hr = THR(pNode->GetElementInterface ( IID_IHTMLDOMNode, (void**) pparentNode ));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::hasChildNodes(VARIANT_BOOL *p)
{
    HRESULT hr = S_OK;
    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *p = VARIANT_FALSE;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

CDOMChildrenCollection *
CDOMTextNode::EnsureDOMChildrenCollectionPtr ( )
{
    CDOMChildrenCollection *pDOMPtr = NULL;
    GetPointerAt ( FindAAIndex ( DISPID_INTERNAL_CDOMCHILDRENPTRCACHE,CAttrValue::AA_Internal ),
        (void **)&pDOMPtr );
    if ( !pDOMPtr )
    {
        pDOMPtr = new CDOMChildrenCollection ( this, FALSE /* fIsElement */ );
        if ( pDOMPtr )
        {
            AddPointer ( DISPID_INTERNAL_CDOMCHILDRENPTRCACHE,
                (void *)pDOMPtr,
                CAttrValue::AA_Internal );
        }
    }
    else
    {
        pDOMPtr->AddRef();
    }
    return pDOMPtr;
}


HRESULT
CDOMTextNode::get_childNodes(IDispatch **ppChildCollection)
{
    HRESULT hr = S_OK;
    CDOMChildrenCollection *pChildren;

    if ( !ppChildCollection )
        goto Cleanup;

    *ppChildCollection = NULL;

    pChildren = EnsureDOMChildrenCollectionPtr();
    if ( !pChildren )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    hr = THR(pChildren->QueryInterface (IID_IDispatch,(void **)ppChildCollection));
    if ( hr )
        goto Cleanup;

    pChildren->Release(); // Lifetime is controlled by extrenal ref.

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::get_attributes(IDispatch **ppAttrCollection)
{
    HRESULT hr = S_OK;
    if (!ppAttrCollection)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppAttrCollection = NULL;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::removeNode(VARIANT_BOOL fDeep,IHTMLDOMNode** ppnewNode)
{
    HRESULT hr = THR ( Remove () );
    if ( hr )
        goto Cleanup;

    if ( ppnewNode )
    {
        hr = THR(QueryInterface (IID_IHTMLDOMNode, (void**)ppnewNode ));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::replaceNode(IHTMLDOMNode *pNodeReplace, IHTMLDOMNode **ppNodeReplaced)
{
    HRESULT hr;
    CDOMTextNode *pNewTextNode = NULL;
    CElement *pNewElement = NULL;

    if ( ppNodeReplaced )
        *ppNodeReplaced = NULL;

    if ( !pNodeReplace )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(CrackDOMNode((IUnknown*)pNodeReplace, &pNewTextNode, &pNewElement, _pDoc));
    if ( hr )
        goto Cleanup;

    hr = THR(ReplaceDOMNodeHelper ( _pDoc, NULL, this, pNewElement, pNewTextNode ));
    if ( hr )
        goto Cleanup;

    if ( ppNodeReplaced )
    {
        hr = THR(QueryInterface(IID_IHTMLDOMNode, (void**)ppNodeReplaced));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CDOMTextNode::swapNode(IHTMLDOMNode *pNodeSwap, IHTMLDOMNode **ppNodeSwapped)
{
    CElement *      pSwapElement = NULL;
    CDOMTextNode *  pSwapText = NULL;
    HRESULT         hr;

    if ( ppNodeSwapped )
        *ppNodeSwapped = NULL;

    if ( !pNodeSwap )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(CrackDOMNode((IUnknown*)pNodeSwap, &pSwapText, &pSwapElement, _pDoc ));
    if ( hr )
        goto Cleanup;
 
    hr = THR (SwapDOMNodeHelper ( _pDoc, NULL, this, pSwapElement, pSwapText ));
    if ( hr )
        goto Cleanup;

    if ( ppNodeSwapped )
    {
        hr = THR(QueryInterface(IID_IHTMLDOMNode, (void**)ppNodeSwapped));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDOMTextNode::insertBefore(IHTMLDOMNode *pNewChild, VARIANT refChild, IHTMLDOMNode **ppRetNode)
{
    if (ppRetNode)
        *ppRetNode = NULL;
    // Don't expect this method to be called on a text node
    RRETURN(SetErrorInfo(E_UNEXPECTED));
}

HRESULT
CDOMTextNode::appendChild(IHTMLDOMNode *pNewChild, IHTMLDOMNode **ppRetNode)
{
    if (ppRetNode)
        *ppRetNode = NULL;
    // Don't expect this method to be called on a text node
    RRETURN(SetErrorInfo(E_UNEXPECTED));
}

HRESULT
CDOMTextNode::replaceChild(IHTMLDOMNode *pNewChild, IHTMLDOMNode *pOldChild, 
                           IHTMLDOMNode **ppRetNode)
{
    if (ppRetNode)
        *ppRetNode = NULL;
    // Don't expect this method to be called on a text node
    RRETURN(SetErrorInfo(E_UNEXPECTED));
}

HRESULT
CDOMTextNode::removeChild(IHTMLDOMNode *pOldChild, IHTMLDOMNode **ppRetNode)
{
    if (ppRetNode)
        *ppRetNode = NULL;
    // Don't expect this method to be called on a text node
    RRETURN(SetErrorInfo(E_UNEXPECTED));
}


HRESULT 
CDOMTextNode::GetMarkupPointer( CMarkupPointer **ppMkp )
{
    HRESULT hr;

    // Move the markup pointer adjacent to the Text
    hr = THR( _pMkpPtr->FindTextIdentity ( _lTextID, (CMarkupPointer *)NULL ) );
    if ( hr )
        goto Cleanup;

    *ppMkp = _pMkpPtr;

Cleanup:
    RRETURN(hr);
}

HRESULT
CDOMTextNode::Remove ( void )
{
    // Move it into it's own Markup container
    HRESULT hr;
    CMarkup *pMarkup = NULL;
    CMarkupPointer mkpPtr (_pDoc);

    hr = THR( _pDoc->CreateMarkup( &pMarkup, NULL ) );
    if (hr)
        goto Cleanup;

    hr = THR(mkpPtr.MoveToContainer( pMarkup, TRUE ) );
    if ( hr )
        goto Cleanup;

    hr = THR(MoveTo ( &mkpPtr ));
    if ( hr )
        goto Cleanup;


Cleanup:
    // Leave markup refcount at 1, text node is keeping it alive
    ReleaseInterface ( (IUnknown*)pMarkup );
    RRETURN(hr);
}

HRESULT 
CDOMTextNode::MoveTo ( CMarkupPointer *pmkptrTarget )
{
    HRESULT         hr;
    CMarkupPointer  mkpEnd(_pDoc);
    long            lCount;

    hr = THR(get_length(&lCount));
    if ( hr )
        goto Cleanup;

    // Move the markup pointer adjacent to the Text
    hr = THR( _pMkpPtr->FindTextIdentity ( _lTextID, &mkpEnd ) );
    if ( hr )
        goto Cleanup;

    pmkptrTarget->SetGravity ( POINTER_GRAVITY_Left );

    // should have right gravity, which with cling will move it also along with text
    Assert(_pMkpPtr->Gravity());    
    Assert(_pMkpPtr->Cling());    
    hr = THR(_pDoc->Move ( _pMkpPtr, &mkpEnd, pmkptrTarget, MUS_DOMOPERATION )); 

Cleanup:
    RRETURN(hr);
}

////////////////////////////////////////////////////////////////////////////////
//
// IHTMLDOMAttribute methods:
//
////////////////////////////////////////////////////////////////////////////////

MtDefine(CAttribute, ObjectModel, "CAttribute")

//+----------------------------------------------------------------
//
//  member : classdesc
//
//+----------------------------------------------------------------

const CBase::CLASSDESC CAttribute::s_classdesc =
{
    &CLSID_HTMLDOMAttribute,        // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLDOMAttribute,         // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

CAttribute::CAttribute(const PROPERTYDESC * const *ppPropDesc, CElement *pElem)
{
    _ppPropDesc = ppPropDesc;
    _pElem = pElem;
    pElem->AddRef();
}

CAttribute::~CAttribute()
{
    Assert(_pElem);
    _pElem->Release();
}

HRESULT
CAttribute::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IHTMLDOMAttribute, NULL)
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

HRESULT
CAttribute::get_nodeName(BSTR *pbstrName)
{
    HRESULT hr = E_POINTER;

    if (!pbstrName)
        goto Cleanup;

    *pbstrName = NULL;
    hr = THR(FormsAllocString((*_ppPropDesc)->pstrName, pbstrName));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CAttribute::get_nodeValue(VARIANT *pvarValue)
{
    HRESULT hr = E_POINTER;
    IDispatchEx *pDEX = NULL;
    WORD wPropType;

    if (!pvarValue)
        goto Cleanup;

    Assert(_ppPropDesc);
    Assert(*_ppPropDesc);
    Assert(_pElem);

    wPropType = (*_ppPropDesc)->GetBasicPropParams()->wInvFunc;
    if (IDX_G_PropEnum == wPropType || IDX_GS_PropEnum == wPropType)
    {
        DISPPARAMS dp = { NULL, NULL, 0, 0 };

        hr = THR(_pElem->PrivateQueryInterface(IID_IDispatchEx, (void**)&pDEX));
        if ( hr )
            goto Cleanup;

        hr = THR (pDEX->Invoke((*_ppPropDesc)->GetDispid(),
                               IID_NULL,
                               g_lcidUserDefault,
                               DISPATCH_PROPERTYGET,
                               &dp,
                               pvarValue,
                               NULL,
                               NULL));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR(_pElem->GetVariantAt(_pElem->FindAAIndex((*_ppPropDesc)->GetDispid(), CAttrValue::AA_Attribute), pvarValue));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pDEX);
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CAttribute::put_nodeValue(VARIANT varValue)
{
    HRESULT hr;
    IDispatchEx *pDEX = NULL;
    DISPPARAMS dp;

    Assert(_ppPropDesc);
    Assert(_pElem);

    hr = THR(_pElem->PrivateQueryInterface(IID_IDispatchEx, (void**)&pDEX));
    if ( hr )
        goto Cleanup;

    dp.rgvarg = &varValue;
    dp.rgdispidNamedArgs = NULL;
    dp.cArgs = 1;
    dp.cNamedArgs = 0;

    hr = THR (pDEX->Invoke((*_ppPropDesc)->GetDispid(),
                           IID_NULL,
                           g_lcidUserDefault,
                           DISPATCH_PROPERTYPUT,
                           &dp,
                           NULL,
                           NULL,
                           NULL));

Cleanup:
    ReleaseInterface(pDEX);
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CAttribute::get_specified(VARIANT_BOOL *pbSpecified)
{
    HRESULT hr = S_OK;

    if (!pbSpecified)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    Assert(_ppPropDesc);

    // not there in AA
    *pbSpecified = (_pElem->FindAAIndex((*_ppPropDesc)->GetDispid(), CAttrValue::AA_Attribute) == AA_IDX_UNKNOWN) ? VARIANT_FALSE : VARIANT_TRUE;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


////////////////////////////////////////////////////////////////////////////////
//
// IHTMLDOMNode methods:
//
////////////////////////////////////////////////////////////////////////////////

HRESULT
CElement::get_nodeType(long *pnodeType)
{
    HRESULT hr = S_OK;

    if (!pnodeType)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *pnodeType = 1; // ELEMENT_NODE

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_nodeName(BSTR *pbstrNodeName)
{
#ifdef VSTUDIO7
    HRESULT hr = THR(GettagName ( pbstrNodeName ));
#else
    HRESULT hr = THR(get_tagName ( pbstrNodeName ));
#endif //VSTUDIO7

    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_nodeValue(VARIANT *pvarValue)
{
    HRESULT hr = S_OK;
    if (!pvarValue)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    pvarValue->vt = VT_NULL;
Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::put_nodeValue(VARIANT varValue)
{
    HRESULT hr = S_OK;

    RRETURN(SetErrorInfo(hr));
}


HRESULT
CElement::get_lastChild ( IHTMLDOMNode **ppNode )
{
    HRESULT hr = S_OK;
    CMarkupPointer markupWalk ( Doc() );

    if (!ppNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppNode = NULL;

    if ( IsNoScope() )
        goto Cleanup;

    if ( !IsInMarkup() ) // Can just return NULL
        goto Cleanup;

    hr = THR(markupWalk.MoveAdjacentToElement( this, ELEM_ADJ_BeforeEnd));
    if ( hr )
        goto Cleanup;

    hr = THR(GetPreviousHelper ( Doc(), &markupWalk, ppNode ));
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CElement::get_previousSibling ( IHTMLDOMNode **ppNode )
{
    HRESULT hr = S_OK;
    CMarkupPointer markupWalk ( Doc() );

    if (!ppNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppNode = NULL;

    if ( !IsInMarkup() ) // Can just return NULL
        goto Cleanup;

    hr = THR( markupWalk.MoveAdjacentToElement( this, ELEM_ADJ_BeforeBegin));
    if ( hr )
        goto Cleanup;

    hr = THR(GetPreviousHelper ( Doc(), &markupWalk, ppNode ));
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CElement::get_nextSibling ( IHTMLDOMNode **ppNode )
{
    HRESULT hr = S_OK;
    CMarkupPointer markupWalk ( Doc() );

    if (!ppNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppNode = NULL;

    if ( !IsInMarkup() ) // Can just return NULL
        goto Cleanup;
    
    hr = THR( markupWalk.MoveAdjacentToElement( this, ELEM_ADJ_AfterEnd));
    if ( hr )
        goto Cleanup;

    hr = THR(GetNextHelper ( Doc(), &markupWalk, ppNode ));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_firstChild ( IHTMLDOMNode **ppNode )
{
    HRESULT hr = S_OK;
    CMarkupPointer markupWalk ( Doc() );

    if (!ppNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppNode = NULL;

    if ( !IsInMarkup() ) // Can just return NULL
        goto Cleanup;

    if ( IsNoScope() )
        goto Cleanup;

    hr = THR( markupWalk.MoveAdjacentToElement( this, ELEM_ADJ_AfterBegin));
    if ( hr )
        goto Cleanup;

    hr = THR(GetNextHelper ( Doc(), &markupWalk, ppNode ));


Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_parentNode(IHTMLDOMNode **pparentNode)
{
    HRESULT             hr = S_OK;
    CTreeNode           *pNode;

    if (!pparentNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pparentNode = NULL;

    pNode = GetFirstBranch();
    if ( !pNode )
        goto Cleanup;

    pNode = pNode->Parent();

    // don't hand out root node
    if (pNode->Tag() == ETAG_ROOT)
        goto Cleanup;

    hr = THR(pNode->GetElementInterface ( IID_IHTMLDOMNode, (void**) pparentNode ));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}



HRESULT 
CElement::DOMWalkChildren ( long lWantItem, long *plCount, IDispatch **ppDispItem )
{
    HRESULT             hr = S_OK;
    CMarkupPointer      markupWalk (Doc());
    CMarkupPointer      markupWalkEnd (Doc());
    CTreeNode           *pnodeRight;
    MARKUP_CONTEXT_TYPE context;
    long                lCount = 0;
    BOOL                fWantItem = (lWantItem == -1 ) ? FALSE : TRUE;
    long                lTextID;
    BOOL                fDone = FALSE;

    if (ppDispItem)
        *ppDispItem = NULL;

    if (!IsInMarkup())
        goto Cleanup;

    hr = THR( markupWalk.MoveAdjacentToElement( this, Tag() == ETAG_ROOT ? ELEM_ADJ_AfterBegin : ELEM_ADJ_BeforeBegin));
    if ( hr )
        goto Cleanup;

    hr = THR( markupWalkEnd.MoveAdjacentToElement( this, Tag() == ETAG_ROOT ? ELEM_ADJ_BeforeEnd : ELEM_ADJ_AfterEnd));
    if ( hr )
        goto Cleanup;

    do
    {
        hr = THR( markupWalk.Right( TRUE, &context, &pnodeRight, NULL, NULL, &lTextID));
        if ( hr )
            goto Cleanup;

        switch ( context )
        {
            case CONTEXT_TYPE_None:
                fDone = TRUE;
                break;
                
            case CONTEXT_TYPE_ExitScope://don't care ...
                if ( pnodeRight->Element() == this )
                    goto NotFound;
                break;

            case CONTEXT_TYPE_EnterScope:
                if ( pnodeRight->Element() != this )
                {
                    // Add to our child list
                    lCount++;
                    // Go to the end of this element, since we will handle it as a container
                    hr = THR( markupWalk.MoveAdjacentToElement( pnodeRight->Element(), ELEM_ADJ_AfterEnd));
                    if ( hr )
                        goto Cleanup;

                    // Need to test we haven't left the scope of my parent.
                    // Overlapping could cause us to move outside
                    if ( markupWalkEnd.IsLeftOf( & markupWalk ) )
                    {
                        fDone = TRUE;
                    }
                }
                break;

            case CONTEXT_TYPE_Text:
                lCount++;
                break;

            case CONTEXT_TYPE_NoScope:
                if ( pnodeRight->Element() == this )
                {
                    // Left scope of current noscope element
                    goto NotFound;
                }
                else
                {
                    // Add to our child list
                    lCount++;
                }
                break;
        }
        if (fWantItem && (lCount-1 == lWantItem ))
        {
            // Found the item we're looking for
            if ( !ppDispItem )
                goto Cleanup; // Didn't want an item, just validating index

            if ( context == CONTEXT_TYPE_Text )
            {
                // Text Node
                CDOMTextNode *pTextNode = NULL;

                hr = THR(CreateTextNode (Doc(), &markupWalk, lTextID, &pTextNode ));
                if ( hr )
                    goto Cleanup;

                hr = THR ( pTextNode->QueryInterface ( IID_IDispatch, (void **)ppDispItem ) );
                if ( hr )
                    goto Cleanup;
                pTextNode->Release();
            }
            else
            {
                // Return Disp to Element
                hr = THR(pnodeRight->GetElementInterface ( IID_IDispatch, (void **) ppDispItem  ));
                if ( hr )
                    goto Cleanup;
            }
            goto Cleanup;
        }
        // else just looking for count
    } while( !fDone );

NotFound:
    if ( fWantItem )
    {
        // Didn't find it - index must be beyond range
        hr = E_INVALIDARG;
    }

Cleanup:
    if ( plCount )
        *plCount = lCount;
    return hr;
}


HRESULT
CElement::hasChildNodes(VARIANT_BOOL *p)
{
    HRESULT hr = S_OK;

    if (!p)
        goto Cleanup;

    *p = VARIANT_FALSE;

    if ( !IsInMarkup() ) // Can just return FALSE
        goto Cleanup;

    // See if we have a child at Index == 0

    hr = THR( DOMWalkChildren ( 0, NULL, NULL ));
    if ( hr == E_INVALIDARG )
    {
        // Invalid Index
        hr = S_OK;
        goto Cleanup;
    }
    else if ( hr )
        goto Cleanup;

    *p = VARIANT_TRUE;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_childNodes(IDispatch **ppChildCollection)
{
    HRESULT hr = S_OK;
    CDOMChildrenCollection *pChildren;

    if ( !ppChildCollection )
        goto Cleanup;

    *ppChildCollection = NULL;

    pChildren = EnsureDOMChildrenCollectionPtr();
    if ( !pChildren )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    hr = THR(pChildren->QueryInterface (IID_IDispatch,(void **)ppChildCollection));
    if ( hr )
        goto Cleanup;

    pChildren->Release(); // Lifetime is controlled by extrenal ref.

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_attributes(IDispatch **ppAttrCollection)
{
    HRESULT hr;
    CAttrCollectionator *pAttrCollator = NULL;

    if (!ppAttrCollection)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *ppAttrCollection = NULL;

    pAttrCollator = new CAttrCollectionator(this);
    if (!pAttrCollator)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    hr = THR(pAttrCollator->EnsureCollection());
    if (hr)
        goto Cleanup;

    hr = THR(pAttrCollator->QueryInterface(IID_IDispatch, (void **)ppAttrCollection));

Cleanup:    
    if (pAttrCollator)
       pAttrCollator->Release();

    RRETURN(SetErrorInfo(hr));
}


HRESULT 
CElement::GetDOMInsertPosition ( CElement *pRefElement, CDOMTextNode *pRefTextNode, CMarkupPointer *pmkptrPos )
{
    HRESULT hr = S_OK;

    if ( !pRefElement && !pRefTextNode )
    {
        // As per DOM spec, if refChild is NULL Insert at end of child list
        // but only if the elem into which it is to be inserted can have children!
        if (IsNoScope())
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }

        hr = THR(EnsureInMarkup());
        if ( hr )
            goto Cleanup;
        hr = THR(pmkptrPos->MoveAdjacentToElement ( this, ELEM_ADJ_BeforeEnd ));
    }
    else
    {
        hr = GetDOMInsertHelper (pRefElement, pRefTextNode, pmkptrPos ); 
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CElement::appendChild(IHTMLDOMNode *pNewChild, IHTMLDOMNode **ppRetNode)
{
    CVariant varEmpty(VT_NULL);

    return insertBefore ( pNewChild, *(VARIANT *)&varEmpty, ppRetNode );
}

static BOOL IsChild(CElement *pelParent, CElement *pelChild, CDOMTextNode *pTextNode)
{
    Assert(pelParent);
    Assert(!pelChild || !pTextNode);

    if (pelChild)
    {
        CTreeNode *pNode = pelChild->GetFirstBranch();
        if (!pNode || !pNode->Parent() || (pNode->Parent()->Element() != pelParent))
            return FALSE;
    }
    else if (pTextNode)
    {
        CElement *pelTxtNodeParent = NULL;
        IHTMLElement *pITxtNodeParent = NULL;
        Assert(pTextNode->MarkupPtr());
        Assert(pTextNode->MarkupPtr()->Cling()); // Must have Glue!
        Assert(pTextNode->MarkupPtr()->Gravity()); // Must have right gravity.
        pTextNode->MarkupPtr()->CurrentScope(&pITxtNodeParent);
        if (pITxtNodeParent)
            pITxtNodeParent->QueryInterface(CLSID_CElement, (void **)&pelTxtNodeParent);
        ReleaseInterface(pITxtNodeParent);
        if (pelTxtNodeParent != pelParent)
            return FALSE;
    }

    return TRUE;
}

HRESULT
CElement::insertBefore(IHTMLDOMNode *pNewChild, VARIANT refChild, IHTMLDOMNode **ppRetNode)
{
    HRESULT                 hr;
    CElement *              pRefElement = NULL;
    CDOMTextNode *          pRefTextNode = NULL;
    CElement *              pNewElement = NULL;
    CDOMTextNode *          pNewTextNode = NULL;
    CDoc *                  pDoc = Doc();
    CMarkupPointer          mkpPtr (pDoc);

    if (!pNewChild)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(CrackDOMNodeVARIANT(&refChild, &pRefTextNode, &pRefElement, pDoc ));
    if ( hr )
        goto Cleanup;

    hr = THR(CrackDOMNode((IUnknown*)pNewChild, &pNewTextNode, &pNewElement, pDoc ));
    if ( hr )
        goto Cleanup;

    if (!IsChild(this, pRefElement, pRefTextNode))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Position ourselves in the right spot
    hr = THR(GetDOMInsertPosition ( pRefElement, pRefTextNode, &mkpPtr ));
    if ( hr )
        goto Cleanup;

    // Now mkpPtr is Positioned at the insertion point, insert the new content
    hr = THR(InsertDOMNodeHelper( pNewElement, pNewTextNode, &mkpPtr ));
    if ( hr )
        goto Cleanup;

    if ( ppRetNode )
        hr = THR(pNewChild->QueryInterface ( IID_IHTMLDOMNode, (void**)ppRetNode));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::replaceChild(IHTMLDOMNode *pNewChild, IHTMLDOMNode *pOldChild, IHTMLDOMNode **pRetNode)
{
    HRESULT                 hr;
    CElement *              pOldElement = NULL;
    CDOMTextNode *          pOldTextNode = NULL;
    CElement *              pNewElement = NULL;
    CDOMTextNode *          pNewTextNode = NULL;
    CDoc *                  pDoc = Doc();
    CMarkupPointer          mkpPtr(pDoc);

    // Pull pOldChild, and all its contents, out of the tree, into its own tree
    // replace it with pNewChild, and all its contents

    if (!pNewChild || !pOldChild)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(CrackDOMNode((IUnknown*)pOldChild, &pOldTextNode, &pOldElement, pDoc ));
    if ( hr )
        goto Cleanup;

    hr = THR(CrackDOMNode((IUnknown*)pNewChild, &pNewTextNode, &pNewElement, pDoc ));
    if ( hr )
        goto Cleanup;

    if (!IsChild(this, pOldElement, pOldTextNode))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Position ourselves in the right spot
    hr = THR(GetDOMInsertPosition ( pOldElement, pOldTextNode, &mkpPtr ));
    if ( hr )
        goto Cleanup;

    mkpPtr.SetGravity ( POINTER_GRAVITY_Left );

    {
        // Lock the markup, to prevent it from going away in case the entire contents are being removed.
        CMarkup::CLock MarkupLock(mkpPtr.Markup());

        hr = THR(RemoveDOMNodeHelper ( pDoc, pOldElement, pOldTextNode ));
        if ( hr )
            goto Cleanup;

        // Now mkpPtr is Positioned at the insertion point, insert the new content
        hr = THR(InsertDOMNodeHelper( pNewElement, pNewTextNode, &mkpPtr ));
        if ( hr )
            goto Cleanup;

        // Return the node being replaced
        if ( pRetNode )
            hr = THR(pOldChild->QueryInterface ( IID_IHTMLDOMNode, (void**)pRetNode));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}



HRESULT
CElement::removeChild(IHTMLDOMNode *pOldChild, IHTMLDOMNode **pRetNode)
{
    HRESULT hr;
    CDOMTextNode *pChildTextNode = NULL;
    CElement *pChildElement = NULL;
    CDoc *pDoc = Doc();

    // Remove the child from the tree, putting it in its own tree

    if ( !pOldChild )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(CrackDOMNode((IUnknown*)pOldChild, &pChildTextNode, &pChildElement, pDoc ));
    if ( hr )
        goto Cleanup;

    if (!IsChild(this, pChildElement, pChildTextNode))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(RemoveDOMNodeHelper ( pDoc, pChildElement, pChildTextNode ));
    if ( hr )
        goto Cleanup;
    
    // Return the node being removed
    if ( pRetNode )
        hr = THR(pOldChild->QueryInterface ( IID_IHTMLDOMNode, (void**)pRetNode));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//
// IHTMLDocument3 DOM methods
//
HRESULT CDoc::CreateDOMTextNodeHelper ( CMarkupPointer *pmkpStart, CMarkupPointer *pmkpEnd,
                                       IHTMLDOMNode **ppTextNode)
{
    HRESULT hr;
    long lTextID;
    CDOMTextNode *pTextNode;

    // ID it
    hr = THR(pmkpStart->SetTextIdentity( pmkpEnd, &lTextID ));
    if ( hr )
        goto Cleanup;

    // set right gravity so that an adjacent text node's markup ptr can never
    // move accidentally.
    hr = THR(pmkpStart->SetGravity ( POINTER_GRAVITY_Right ));
    if (hr)
        goto Cleanup;

    // Text Node takes ownership of pmkpStart
    pTextNode = new CDOMTextNode ( lTextID, this, pmkpStart ); // AddRef's the Markup
    if ( !pTextNode )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if ( ppTextNode )
    {
        hr = THR(pTextNode->QueryInterface ( IID_IHTMLDOMNode, (void**)ppTextNode ));
        if ( hr )
            goto Cleanup;
    }

    pTextNode->Release(); // Creating AddREf'd it once - refcount owned by external AddRef

    hr = THR(_HtPvPvDOMTextNodes.Insert ( (void*)(DWORD_PTR)(lTextID<<4), (void*)pTextNode ) );
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

HRESULT CDoc::createTextNode(BSTR text, IHTMLDOMNode **ppTextNode)
{
    HRESULT             hr = S_OK;
    CMarkup *           pMarkup = NULL;
    CMarkupPointer *    pmkpPtr = NULL;
    long                lTextID;
    long                lLen = -1;
    CMarkupPointer      mkpEnd ( this );

    if (!ppTextNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppTextNode = NULL;

    // Because Perf is not our primary concern right now, I'm going to create 
    // a markup container to hold the text. If we had more time, I'd just store the string internaly 
    // and special case access to the data in all the method calls
    hr = THR( CreateMarkup( &pMarkup, NULL ) );
    if (hr)
        goto Cleanup;

    pmkpPtr = new CMarkupPointer ( this );
    if ( !pmkpPtr )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pmkpPtr->MoveToContainer( pMarkup, TRUE ));
    if ( hr )
        goto Cleanup;

    pmkpPtr->SetGravity ( POINTER_GRAVITY_Left );

    // Put the text in
    hr = THR(pmkpPtr->Doc()->InsertText ( pmkpPtr, text, -1, MUS_DOMOPERATION ) );
    if ( hr )
        goto Cleanup;

    // Position the end pointer to the extreme right of the text
    hr = THR(mkpEnd.MoveToPointer ( pmkpPtr));
    if ( hr )
        goto Cleanup;

    // Move right by the number of chars inserted
    hr = THR(mkpEnd.Right( TRUE, NULL, NULL, &lLen, NULL, &lTextID));
    if ( hr )
        goto Cleanup;

    hr = THR( CreateDOMTextNodeHelper ( pmkpPtr, &mkpEnd, ppTextNode ) );
    if ( hr )
        goto Cleanup;

    pmkpPtr = NULL; // Text Node now owns the pointer


Cleanup:
    ReleaseInterface ( (IUnknown*)(pMarkup) ); // Text Node keeps the markup alive
    delete pmkpPtr;
    
    RRETURN(SetErrorInfo(hr));
}

HRESULT CDoc::get_documentElement(IHTMLElement **ppRootElem)
{
    return _pPrimaryMarkup->get_documentElement(ppRootElem);
}

HRESULT CMarkup::get_documentElement(IHTMLElement **ppRootElem)
{
    HRESULT hr = S_OK;
    CHtmlElement *pRootElem = NULL;

    if (!ppRootElem)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppRootElem = NULL;

    pRootElem = GetHtmlElement();
    if (pRootElem)
        hr = THR(pRootElem->QueryInterface(IID_IHTMLElement, (void **)ppRootElem));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//
//  IE5 XOM. Extensions for document construction
//

HRESULT 
CElement::GetMarkupPtrRange(CMarkupPointer *pmkptrStart, CMarkupPointer *pmkptrEnd, BOOL fEnsureMarkup)
{
    HRESULT hr;
    
    Assert(pmkptrStart);
    Assert(pmkptrEnd);
    
    if (fEnsureMarkup)
    {
        hr = THR(EnsureInMarkup());
        if (hr)
            goto Cleanup;
    }

    hr = THR(pmkptrStart->MoveAdjacentToElement(this, ELEM_ADJ_BeforeBegin));
    if (hr)
        goto Cleanup;

    hr = THR(pmkptrEnd->MoveAdjacentToElement(this, ELEM_ADJ_AfterEnd));

Cleanup:
    return hr;
}


HRESULT
CElement::cloneNode(VARIANT_BOOL fDeep, IHTMLDOMNode **ppNodeCloned)
{
    HRESULT                 hr = E_OUTOFMEMORY;
    CDoc *                  pDoc = Doc();
    CMarkup *               pMarkupTarget = NULL;
    CElement *              pElement = NULL;
    CTreeNode   *           pNode;

    if ( !ppNodeCloned )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppNodeCloned = NULL;

    if (!fDeep || !IsInMarkup() || IsNoScope())
    {
        hr = THR(Clone(&pElement, Doc()));
        if (hr)
            goto Cleanup;
    
        hr = THR(pElement->PrivateQueryInterface(IID_IHTMLDOMNode, (void **)ppNodeCloned));
        if (hr)
            goto Cleanup;

        pElement->Release();
    }
    else
    {
        CMarkupPointer mkptrStart ( pDoc );
        CMarkupPointer mkptrEnd ( pDoc );
        CMarkupPointer mkptrTarget ( pDoc );

        // Get src ptrs
        hr = THR(GetMarkupPtrRange(&mkptrStart, &mkptrEnd));
        if (hr)
            goto Cleanup;

        // create new markup
        hr = THR(pDoc->CreateMarkup(&pMarkupTarget));
        if (hr)
            goto Cleanup;

        // Get target ptr

        hr = THR(mkptrTarget.MoveToContainer(pMarkupTarget,TRUE));
        if (hr)
            goto Cleanup;

        // Copy src -> target
        hr = THR(pDoc->Copy(&mkptrStart, &mkptrEnd, &mkptrTarget, MUS_DOMOPERATION));
        if (hr)
            goto Cleanup;

        // This addrefs the markup too!

        // Go Right to pick up the new node ptr
        hr = THR(mkptrTarget.Right(FALSE, NULL, &pNode, 0, NULL,NULL));
        if (hr)
            goto Cleanup;

        hr = THR(pNode->Element()->PrivateQueryInterface(IID_IHTMLDOMNode, (void **)ppNodeCloned));
        if (hr)
            goto Cleanup;

    }

Cleanup:

    // release extra markup lock
    if (pMarkupTarget)
        pMarkupTarget->Release();

    RRETURN(SetErrorInfo(hr));

}

    // Surgicaly remove pElemApply out of its current context,
    // Apply it over this element

HRESULT
CElement::applyElement(IHTMLElement *pElemApply, BSTR bstrWhere, IHTMLElement **ppElementApplied)
{
    HRESULT hr;
    CDoc *pDoc = Doc();
    CMarkupPointer mkptrStart(pDoc);
    CMarkupPointer mkptrEnd (pDoc);
    CElement *pElement;
    htmlApplyLocation where = htmlApplyLocationOutside;

    if ( ppElementApplied )
        *ppElementApplied = NULL;

    if ( !pElemApply )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!pDoc->IsOwnerOf(pElemApply))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(pElemApply->QueryInterface ( CLSID_CElement, (void**) &pElement ));
    if (hr)
        goto Cleanup;

    // No scoped elements cannot be applied as they cannot have children!
    if (pElement->IsNoScope())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Get src ptrs
    ENUMFROMSTRING(htmlApplyLocation, bstrWhere, (long *)&where);

    hr = THR(EnsureInMarkup());
    if (hr)
        goto Cleanup;

    hr = THR(mkptrStart.MoveAdjacentToElement(this, where ? ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterBegin));
    if (hr)
        goto Cleanup;

    hr = THR(mkptrEnd.MoveAdjacentToElement(this, where ? ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeEnd));
    if (hr)
        goto Cleanup;

    // Surgically remove the elem to be applied if in a markup.
    if (pElement->IsInMarkup())
    {
        hr = THR(pDoc->RemoveElement(pElement, MUS_DOMOPERATION));
        if ( hr )
            goto Cleanup;
    }

    hr = THR(pDoc->InsertElement ( pElement, &mkptrStart, &mkptrEnd, MUS_DOMOPERATION ));
    if (hr)
        goto Cleanup;

    if ( ppElementApplied )
        hr = THR(pElemApply->QueryInterface ( IID_IHTMLElement, (void**) ppElementApplied ));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CElement::removeNode (VARIANT_BOOL fDeep, IHTMLDOMNode **ppNodeRemoved)
{
    HRESULT hr = S_OK;

    // Pull element out of the tree
    if ( ppNodeRemoved )
        *ppNodeRemoved = NULL;

    if ( fDeep )
    {
        hr = THR(RemoveDOMNodeHelper ( Doc(), this, NULL ));
        if ( hr )
            goto Cleanup;
    }
    else if (IsInMarkup())
    {
        // Surgical removal
        hr = THR(Doc()->RemoveElement ( this, MUS_DOMOPERATION ) );
        if ( hr )
            goto Cleanup;
    }

    if ( ppNodeRemoved )
    {
        hr = THR(QueryInterface(IID_IHTMLDOMNode, (void**)ppNodeRemoved));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CElement::replaceNode(IHTMLDOMNode *pNodeReplace, IHTMLDOMNode **ppNodeReplaced)
{
    HRESULT hr;
    CDOMTextNode *pNewTextNode = NULL;
    CElement *pNewElement = NULL;
    CDoc *pDoc = Doc();

    if ( ppNodeReplaced )
        *ppNodeReplaced = NULL;

    if ( !pNodeReplace )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(CrackDOMNode((IUnknown*)pNodeReplace, &pNewTextNode, &pNewElement, pDoc ));
    if ( hr )
        goto Cleanup;

    hr = THR(ReplaceDOMNodeHelper ( pDoc, this, NULL, pNewElement, pNewTextNode ));
    if ( hr )
        goto Cleanup;

    if ( ppNodeReplaced )
    {
        hr = THR(QueryInterface(IID_IHTMLDOMNode, (void**)ppNodeReplaced));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}    

HRESULT
CElement::swapNode(IHTMLDOMNode *pNodeSwap, IHTMLDOMNode **ppNodeSwapped)
{
    CElement *      pSwapElement = NULL;
    CDOMTextNode *  pSwapText = NULL;
    CDoc *          pDoc = Doc();
    HRESULT         hr;

    if ( ppNodeSwapped )
        *ppNodeSwapped = NULL;

    if ( !pNodeSwap )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(CrackDOMNode((IUnknown*)pNodeSwap, &pSwapText, &pSwapElement, pDoc ));
    if ( hr )
        goto Cleanup;
 
    hr = THR (SwapDOMNodeHelper ( pDoc, this, NULL, pSwapElement, pSwapText ));
    if ( hr )
        goto Cleanup;

    if ( ppNodeSwapped )
    {
        hr = THR(QueryInterface(IID_IHTMLDOMNode, (void**)ppNodeSwapped));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::insertAdjacentElement(BSTR bstrWhere, IHTMLElement *pElemInsert, IHTMLElement **ppElementInserted)
{
    HRESULT                     hr;
    htmlAdjacency               where;
    ELEMENT_ADJACENCY           adj;
    CDoc *pDoc =                Doc();
    CMarkupPointer mkptrTarget(pDoc);
    CElement *pElement = NULL;

    if (ppElementInserted)
        *ppElementInserted = NULL;

    if (!pElemInsert)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!pDoc->IsOwnerOf( pElemInsert ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(ENUMFROMSTRING(htmlAdjacency, bstrWhere, (long *)&where));
    if (hr)
        goto Cleanup;

    hr = THR(pElemInsert->QueryInterface(CLSID_CElement, (void **)&pElement));
    if (hr)
        goto Cleanup;

    switch (where)
    {
    case htmlAdjacencyBeforeBegin:
    default:
        adj = ELEM_ADJ_BeforeBegin;
        break;
    case htmlAdjacencyAfterBegin:
        adj = ELEM_ADJ_AfterBegin;
        break;
    case htmlAdjacencyBeforeEnd:
        adj = ELEM_ADJ_BeforeEnd;
        break;
    case htmlAdjacencyAfterEnd:
        adj = ELEM_ADJ_AfterEnd;
        break;
    }

    // Get target ptr
    hr = THR(EnsureInMarkup());
    if (hr)
        goto Cleanup;

    hr = THR(mkptrTarget.MoveAdjacentToElement(this, adj));
    if (hr)
        goto Cleanup;

    // Move src -> target
    hr = THR(InsertDOMNodeHelper(pElement, NULL, &mkptrTarget));
    if (hr)
        goto Cleanup;

    if ( ppElementInserted )
    {
        *ppElementInserted = pElemInsert;
        (*ppElementInserted)->AddRef();
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

static
HRESULT
SetAdjacentTextPointer ( CElement *pElem, htmlAdjacency where, 
    MARKUP_CONTEXT_TYPE *pContext, CMarkupPointer *pmkptrStart, long *plCharCount)
{
    ELEMENT_ADJACENCY           adj;
    BOOL fLeft;
    HRESULT hr;

    switch (where)
    {
    case htmlAdjacencyBeforeBegin:
    default:
        adj = ELEM_ADJ_BeforeBegin;
        fLeft = TRUE;
        break;
    case htmlAdjacencyAfterBegin:
        adj = ELEM_ADJ_AfterBegin;
        fLeft = FALSE;
        break;
    case htmlAdjacencyBeforeEnd:
        adj = ELEM_ADJ_BeforeEnd;
        fLeft = TRUE;
        break;
    case htmlAdjacencyAfterEnd:
        adj = ELEM_ADJ_AfterEnd;
        fLeft = FALSE;
        break;
    }

    hr = THR(pmkptrStart->MoveAdjacentToElement(pElem, adj));
    if (hr)
        goto Cleanup;

    if ( fLeft )
    {
        // Need to move the pointer to the start of the text
        hr = THR(pmkptrStart->Left ( TRUE, pContext, NULL, plCharCount, NULL, NULL ));
        if ( hr )
            goto Cleanup;
    }
    else if ( plCharCount )
    {
        // Need to do a non-moving Right to get the text length
        hr = THR(pmkptrStart->Right ( FALSE, pContext, NULL, plCharCount, NULL, NULL ));
        if ( hr )
            goto Cleanup;
    }
Cleanup:
    RRETURN(hr);
}


HRESULT
CElement::getAdjacentText( BSTR bstrWhere, BSTR *pbstrText )
{
    HRESULT                     hr = S_OK;
    CMarkupPointer              mkptrStart ( Doc() );
    htmlAdjacency               where;
    long                        lCharCount = -1;
    MARKUP_CONTEXT_TYPE         context;

    hr = THR(ENUMFROMSTRING(htmlAdjacency, bstrWhere, (long *)&where));
    if (hr)
        goto Cleanup;

    if ( !pbstrText )
        goto Cleanup;

    *pbstrText = NULL;

    hr = THR(SetAdjacentTextPointer ( this, where, &context, &mkptrStart, &lCharCount ));
    if ( hr )
        goto Cleanup;

    // Is there any text to return
    if ( context != CONTEXT_TYPE_Text || lCharCount == 0 )
        goto Cleanup;

    // Alloc memory
    hr = FormsAllocStringLen ( NULL, lCharCount, pbstrText );
    if ( hr )
        goto Cleanup;

    // Read it into the buffer
    hr = THR(mkptrStart.Right( FALSE, &context, NULL, &lCharCount, *pbstrText, NULL));
    if ( hr )
        goto Cleanup;
    
Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::replaceAdjacentText( BSTR bstrWhere, BSTR bstrText, BSTR *pbstrText )
{
    HRESULT                     hr = S_OK;
    CMarkupPointer              mkptrStart ( Doc() );
    CMarkupPointer              mkptrEnd ( Doc() );
    htmlAdjacency               where;
    long                        lCharCount = -1;
    MARKUP_CONTEXT_TYPE         context;

    hr = THR(ENUMFROMSTRING(htmlAdjacency, bstrWhere, (long *)&where));
    if (hr)
        goto Cleanup;

    if ( pbstrText )
    {
        hr = THR (getAdjacentText(bstrWhere, pbstrText ));
        if ( hr )
            goto Cleanup;
    }

    hr = THR(SetAdjacentTextPointer ( this, where, &context, &mkptrStart, &lCharCount ));
    if ( hr )
        goto Cleanup;

    hr = THR(mkptrEnd.MoveToPointer ( &mkptrStart ) );
    if ( hr )
        goto Cleanup;

    if ( context == CONTEXT_TYPE_Text && lCharCount > 0 )
    {
        hr = THR( mkptrEnd.Right ( TRUE, &context, NULL, &lCharCount, NULL, NULL ));
        if ( hr )
            goto Cleanup;        
    }

    hr = THR(Doc()->Remove ( &mkptrStart, &mkptrEnd, MUS_DOMOPERATION ));
    if ( hr )
        goto Cleanup;

    hr = THR(mkptrStart.Doc()->InsertText( & mkptrStart, bstrText, -1, MUS_DOMOPERATION ));
    if ( hr )
        goto Cleanup;
    
Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_canHaveChildren(VARIANT_BOOL *pvb)
{
    *pvb = IsNoScope() ? VARIANT_FALSE : VARIANT_TRUE;
    return S_OK;
}
