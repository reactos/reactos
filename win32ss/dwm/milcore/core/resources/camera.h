// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_camera
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilCameraDuce);

// Class: CMilCameraDuce
class CMilCameraDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilCameraDuce));

    CMilCameraDuce(__in_ecount(1) CComposition*)
    {
    }

public:
    virtual HRESULT GetViewTransform(
        __out_ecount(1) CMILMatrix *pViewMatrixOut
        ) const = 0;

    virtual bool ShouldComputeClipPlanes() const = 0;

    virtual HRESULT SynchronizeAnimations() = 0;

    virtual HRESULT ApplyToContextState(
        __inout_ecount(1) CContextState *pCtxState,     // Context state to modify
        const float flViewportWidth,
        const float flViewportHeight,
        const bool fUseComputedPlanes,
        const float flComputedNearPlane,
        const float flComputedFarPlane,
        bool &fRenderRequiredOut
        ) const = 0;

    override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_CAMERA;
    }

protected:
    static HRESULT PrependInverseTransform(
        __in_ecount_opt(1) CMilTransform3DDuce* pTransform,
        __inout_ecount(1) CMILMatrix *pViewMatrix
        );
};


