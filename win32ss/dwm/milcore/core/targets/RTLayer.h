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
//      Declare render target layer template and stack
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#pragma once


template <typename TBounds, typename TTargetSpecificData>
class CRenderTargetLayerStack;


//+-----------------------------------------------------------------------------
//
//  Class:
//      CRenderTargetLayer
//
//  Synopsis:
//      Retains basic information about a render target layer
//
//------------------------------------------------------------------------------

template <typename TBounds, typename TTargetSpecificData>
class CRenderTargetLayer
{
    friend class CRenderTargetLayerStack<TBounds,TTargetSpecificData>;

private:

    CRenderTargetLayer()
    {
        pGeometricMaskShape = NULL;
        rAlpha = 1.0f;
        pAlphaMaskBrush = NULL;
        fSavedClearTypeHint = false;
    }

    ~CRenderTargetLayer()
    {
        delete pGeometricMaskShape;
        ReleaseInterfaceNoNULL(pAlphaMaskBrush);
    }


public:

    TBounds rcLayerBounds;              // Bounds of this layer

    TBounds rcPrevBounds;               // Bounds of previous layer which may
                                        // be the target itself.

    CShape *pGeometricMaskShape;        // Geometric mask, if present

    MilAntiAliasMode::Enum AntiAliasMode;     // Antialiasing mode to use when
                                        // generating geometric mask coverage.

    FLOAT rAlpha;                       // Constant alpha value to apply when
                                        // this layer ends.

    CBrushRealizer *pAlphaMaskBrush;    // OpacityMask, if present

    bool fSavedClearTypeHint;               // Saved ClearTypeHint, forcing ClearType can be disabled temporarily for the layer

    TTargetSpecificData oTargetData;    // Render target type specific data

};



//+-----------------------------------------------------------------------------
//
//  Class:
//      CRenderTargetLayerStack
//
//  Synopsis:
//      Maintains a stack of CRenderTargetLayer objects
//
//------------------------------------------------------------------------------

template <typename TBounds, typename TTargetSpecificData>
class CRenderTargetLayerStack
{

public:

    ~CRenderTargetLayerStack();

    HRESULT Push(
        __deref_ecount(1) CRenderTargetLayer<TBounds, TTargetSpecificData> * &pRTLayer
        );

    UINT GetCount() const
    {
        return m_RTLayerStack.GetCount();
    }    

    __outro_ecount(1) CRenderTargetLayer<TBounds, TTargetSpecificData> const &Top() const;

    void Pop();

private:

    DynArrayIANoCtor<
        CRenderTargetLayer<TBounds, TTargetSpecificData>,
        16,
        false
        >
        m_RTLayerStack;

};


#include "RTLayer.inl"



