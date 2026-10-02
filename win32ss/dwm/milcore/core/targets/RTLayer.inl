// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_targets
//      $Keywords:
//
//  $Description:
//      Define render target layer stack implementation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------



//+-----------------------------------------------------------------------------
//
//  Member:
//      CRenderTargetLayerStack::~CRenderTargetLayerStack
//
//  Synopsis:
//      dtor
//

template <class TBounds, class TTargetSpecificData>
CRenderTargetLayerStack<TBounds, TTargetSpecificData>::~CRenderTargetLayerStack(
    )
{
    UINT i = m_RTLayerStack.GetCount();

    // destroy each element still on the stack
    while (i > 0)
    {
        i--;
        m_RTLayerStack[i].~CRenderTargetLayer();
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CRenderTargetLayerStack::Push
//
//  Synopsis:
//      Create a new element on top of stack
//

template <class TBounds, class TTargetSpecificData>
HRESULT
CRenderTargetLayerStack<TBounds, TTargetSpecificData>::Push(
    __deref_ecount(1) CRenderTargetLayer<TBounds, TTargetSpecificData> * &pRTLayer
    )
{
    HRESULT hr = S_OK;

    IFC(m_RTLayerStack.AddMultiple(1, &pRTLayer));

    // Initialize top new element (construct)
    new (pRTLayer) CRenderTargetLayer<TBounds, TTargetSpecificData>();

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CRenderTargetLayerStack::Top
//
//  Synopsis:
//      Return top element
//

template <class TBounds, class TTargetSpecificData>
__outro_ecount(1) CRenderTargetLayer<TBounds, TTargetSpecificData> const &
CRenderTargetLayerStack<TBounds, TTargetSpecificData>::Top(
    ) const
{
    return m_RTLayerStack.Last();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CRenderTargetLayerStack::Pop
//
//  Synopsis:
//      Remove top element from stack
//

template <class TBounds, class TTargetSpecificData>
void
CRenderTargetLayerStack<TBounds, TTargetSpecificData>::Pop(
    )
{
    // Destroy top element (destruct)
    m_RTLayerStack.Last().~CRenderTargetLayer();

    // Remove from stack
    m_RTLayerStack.AdjustCount(static_cast<UINT>(-1));

    return;
}




