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

MtExtern(CMilOrthographicCameraDuce);

// Class: CMilOrthographicCameraDuce
class CMilOrthographicCameraDuce : public CMilProjectionCameraDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilOrthographicCameraDuce));

    CMilOrthographicCameraDuce(__in_ecount(1) CComposition* pComposition)
        : CMilProjectionCameraDuce(pComposition)
    {
    }

    virtual ~CMilOrthographicCameraDuce();

public:

    override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_ORTHOGRAPHICCAMERA || CMilProjectionCameraDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_ORTHOGRAPHICCAMERA* pCmd
        );

    HRESULT RegisterNotifiers(__in_ecount(1) CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    override HRESULT GetProjectionTransform(
        const double aspectRatio,
        const float flNearPlaneDistance,
        const float flFarPlaneDistance,
        __out_ecount(1) CMILMatrix *pProjectionMatrixOut) const;

    override virtual HRESULT GetViewTransform(
        __out_ecount(1) CMILMatrix *pViewMatrixOut) const;

    override virtual HRESULT ApplyToContextState(
        __inout_ecount(1) CContextState *pCtxState,     // Context state to modify
        const float flViewportWidth,
        const float flViewportHeight,
        const bool fUseComputedPlanes,
        const float flComputedNearPlane,
        const float flComputedFarPlane,
        __out_ecount(1) bool &fRenderRequiredOut
        ) const;

    override virtual HRESULT SynchronizeAnimations()
    {
        return SynchronizeAnimatedFields();
    }

    HRESULT SynchronizeAnimatedFields();

protected:    
    override HRESULT EnsureClippingPlaneDistance(
        __inout_ecount(1) float &flNearPlane,
        __inout_ecount(1) float &flFarPlane
        ) const;

public:
    CMilOrthographicCameraDuce_Data m_data;

};

