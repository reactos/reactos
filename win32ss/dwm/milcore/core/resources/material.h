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

MtExtern(CMilMaterialDuce);

// Class: CMilMaterialDuce
class CMilMaterialDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

public:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilMaterialDuce));

    CMilMaterialDuce(__in_ecount(1) CComposition*)
    {
    }

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_MATERIAL;
    }

    virtual bool ShouldRender() = 0;

    virtual HRESULT Realize(
        __inout_ecount(1) CMILMesh3D *pMesh3D,
        __in_ecount(1) CDrawingContext *pDrawingContext,
        __in_ecount(1) CContextState *pContextState,
        __in_ecount(1) const BrushContext *pBrushContext,
        __deref_out_ecount_opt(1) CMILShader **ppShader
        ) = 0;

    virtual HRESULT Flatten(
        __inout_ecount(1) DynArray<CMilMaterialDuce *> *pMaterialList,
        __inout_ecount(1) bool *pfDiffuseMaterialFound,
        __inout_ecount(1) bool *pfSpecularMaterialFound,
        __out_ecount(1) float *pflFirstSpecularPower,
        __out_ecount(1) MilColorF *pFirstAmbientColor,
        __out_ecount(1) MilColorF *pFirstDiffuseColor,
        __out_ecount(1) MilColorF *pFirstSpecularColor
        ) = 0;
};


