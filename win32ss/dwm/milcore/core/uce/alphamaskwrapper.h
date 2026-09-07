// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Abstract:
//    It contains the alpha mask and the bounds of the node in inner space
//    which has this alpha mask. We only want to store these bounds if a 
//    node has alpha mask and so we have this wrapper class.
//------------------------------------------------------------------------

MtExtern(CMilAlphaMaskWrapper);

class CMilAlphaMaskWrapper
{
public:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilAlphaMaskWrapper));

    static HRESULT Create(
        __deref_out_ecount(1) CMilAlphaMaskWrapper **ppAlphaMask
        );

    ~CMilAlphaMaskWrapper();

    //
    // Get/Set for the inner bounds of this node
    //
    void GetVisualPreviousInnerBounds(
        __out_ecount(1) CMilRectF *prcBounds
        ) const;

    void SetVisualPreviousInnerBounds(
        __in_ecount(1) const CMilRectF &prcBounds
        );


    //
    // Get/Set for the outer bounds of this node
    //
    void GetVisualPreviousOuterBounds(
        __out_ecount(1) CMilRectF *prcBounds
        ) const;

    void SetVisualPreviousOuterBounds(
        __in_ecount(1) const CMilRectF &prcBounds
        );

    //
    // Get/Set for the alpha mask for this node
    //
    __out_ecount(1) CMilBrushDuce* GetAlphaMask()
    {
        return m_pAlphaMask;
    }

    void SetAlphaMask(
        __in_ecount_opt(1) CMilBrushDuce *pAlphaMask
        );

private:

    // Ctor inaccessible to prevent inheritance.
    // Use the Create method to instantiate a CMilAlphaMaskWrapper.    
    CMilAlphaMaskWrapper();

    //
    // Store the bounds for this node. We use the previous inner bounds to compare if 
    // the bounds in inner space have changed or not when we calculate them again in precompctx.cpp.
    // If the bounds change, then we want to treat the node as dirty for render and 
    // so we also need the old bounds in outerspace when marking it dirty.
    //
    CMilRectF m_previousInnerBounds;
    CMilRectF m_previousOuterBounds;

    CMilBrushDuce *m_pAlphaMask;
};


