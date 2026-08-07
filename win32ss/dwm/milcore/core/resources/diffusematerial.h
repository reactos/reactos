// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_material
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilDiffuseMaterialDuce);

// Class: CMilDiffuseMaterialDuce
class CMilDiffuseMaterialDuce : public CMilMaterialDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilDiffuseMaterialDuce));

    CMilDiffuseMaterialDuce(__in_ecount(1) CComposition* pComposition)
        : CMilMaterialDuce(pComposition)
    {
    }

    virtual ~CMilDiffuseMaterialDuce();

public:

    override bool ShouldRender();

    override HRESULT Realize(
        __inout_ecount(1) CMILMesh3D *pMesh3D,
        __in_ecount(1) CDrawingContext *pDrawingContext,
        __in_ecount(1) CContextState *pContextState,
        __in_ecount(1) const BrushContext *pBrushContext,
        __deref_out_ecount_opt(1) CMILShader **ppShader
        );

    override HRESULT Flatten(
        __inout_ecount(1) DynArray<CMilMaterialDuce *> *pMaterialList,
        __inout_ecount(1) bool *pfDiffuseMaterialFound,
        __inout_ecount(1) bool *pfSpecularMaterialFound,
        __out_ecount(1) float *pflFirstSpecularPower,
        __out_ecount(1) MilColorF *pFirstAmbientColor,
        __out_ecount(1) MilColorF *pFirstDiffuseColor,
        __out_ecount(1) MilColorF *pFirstSpecularColor
        );

public:
    
    override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_DIFFUSEMATERIAL || CMilMaterialDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_DIFFUSEMATERIAL* pCmd
        );

    HRESULT RegisterNotifiers(__in_ecount(1) CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    CMilDiffuseMaterialDuce_Data m_data;
};

