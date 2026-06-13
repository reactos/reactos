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

MtExtern(CMilMatrixCameraDuce);

// Class: CMilMatrixCameraDuce
class CMilMatrixCameraDuce : public CMilCameraDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilMatrixCameraDuce));

    CMilMatrixCameraDuce(__in_ecount(1) CComposition* pComposition)
        : CMilCameraDuce(pComposition)
    {
    }

    virtual ~CMilMatrixCameraDuce();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_MATRIXCAMERA || CMilCameraDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_MATRIXCAMERA* pCmd
        );

    HRESULT RegisterNotifiers(__in_ecount(1) CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    override virtual bool ShouldComputeClipPlanes() const
    {
        // Our contract for MatrixCamera is to always render using the matrices
        // exactly as the user specified.  We do not auto-range the clip planes.
        return false;
    }

    override virtual HRESULT GetViewTransform(
        __out_ecount(1) CMILMatrix *pViewMatrixOut) const;

    override virtual HRESULT ApplyToContextState(
        __inout_ecount(1) CContextState *pCtxState,     // Context state to modify
        const float flViewportWidth,
        const float flViewportHeight,
        const bool fUseComputedPlanes,
        const float flComputedNearPlane,
        const float flComputedFarPlane,
        bool &fRenderRequiredOut
        ) const;

    override virtual HRESULT SynchronizeAnimations()
    {
        RRETURN(S_OK);
    }

    CMilMatrixCameraDuce_Data m_data;
};

