// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Declaration of all MIL light classes:
//          - CMILLight
//          - CMILLightAmbient
//          - CMILLightDirectional
//          - CMILLightPoint
//          - CMILLightSpot
//

MtExtern(CMILLightAmbient);
MtExtern(CMILLightDirectional);
MtExtern(CMILLightPoint);
MtExtern(CMILLightSpot);

class CHwPipelineShader;

//+----------------------------------------------------------------------------
//
//  Class:     CMILLight
//
//  Synopsis:  Base class for all lights
//

class CMILLight
{
public:

    enum TransformType
    {
        TransformType_ViewSpace,
            // pTransform will take the light to ViewSpace
        TransformType_LightingSpace, 
            // pTransform will take the light to where we do lighting
        TransformType_Copy        
            // equivalent to LightingSpace with pTransform being the Identity
    };

    virtual HRESULT SendShaderData(
        __in_ecount(1) CHwPipelineShader *pShader,
        __in_ecount(1) const MilColorF &materialColor,
        __inout_ecount(1) MILSPHandle &hStartRegister
        ) const PURE;

    virtual void Transform(
        TransformType type,
        __in_ecount(1) const CMILMatrix *pTransform,
        float flScale
        ) PURE;

protected:
    CMILLight() 
    {
#if DBG_ANALYSIS
        m_fDbgHasBeenViewTransformed = false;
#endif
    };

#if DBG_ANALYSIS
    bool m_fDbgHasBeenViewTransformed;
#endif
};

//+----------------------------------------------------------------------------
//
//  Class:     CMILLightAmbient
//
//  Synopsis:  Ambient light class
//

class CMILLightAmbient :
    public CMILLight
{
public:
    CMILLightAmbient() {};

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILLightAmbient));

    void Set(
        __in_ecount(1) const MilColorF *pcolorAmbient
        );

    HRESULT SendShaderData(
        __in_ecount(1) CHwPipelineShader *pShader,
        __in_ecount(1) const MilColorF &materialColor,
        __inout_ecount(1) MILSPHandle &hStartRegister
        ) const override;

    void Transform(
        TransformType type,
        __in_ecount(1) const CMILMatrix *pTransform,
        float flScale
        ) override
    {
        UNREFERENCED_PARAMETER(type);
        UNREFERENCED_PARAMETER(pTransform);
        UNREFERENCED_PARAMETER(flScale);

        // nothing to transform
    }

    MilColorF m_colorDiffuse;
};

//+----------------------------------------------------------------------------
//
//  Class:     CMILLightDirectional
//
//  Synopsis:  Directional light class
//

class CMILLightDirectional :
    public CMILLightAmbient
{
public:
    CMILLightDirectional() {};

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILLightDirectional));
   
    void Set(
        __in_ecount(1) const MilColorF *pcolorDiffuse,
        __in_ecount(1) const dxlayer::vector3 *pvec3Direction
        );

    HRESULT SendShaderData(
        __in_ecount(1) CHwPipelineShader *pShader,
        __in_ecount(1) const MilColorF &materialColor,
        __inout_ecount(1) MILSPHandle &hStartRegister
        ) const override;

    void Transform(
        TransformType type,
        __in_ecount(1) const CMILMatrix *pTransform,
        float flScale
        ) override;

    dxlayer::vector3 m_vec3InvDirectionViewSpace;
    dxlayer::vector3 m_vec3InvDirection;
};

//+----------------------------------------------------------------------------
//
//  Class:     CMILLightPoint
//
//  Synopsis:  Point light class
//
//             Point inheriting from directional is kinda funky. However, spot
//             has both point and directional properties so to avoid multiple
//             inheritance or code duplication, point derives from directional.
//

class CMILLightPoint :
    public CMILLightDirectional
{
public:
    CMILLightPoint() {};

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILLightPoint));

    void Set(    
        __in_ecount(1) const MilColorF *pcolorDiffuse,
        __in_ecount(1) const dxlayer::vector3 *pvec3Position,
        float flRange,
        float flAttenuation0,
        float flAttenuation1,
        float flAttenuation2
        );

    virtual float GetSpotlightFactor(__in_ecount(1) dxlayer::vector3 const *vec3ToLight) const;
    virtual bool IsSpot() const;

    HRESULT SendShaderData(
        __in_ecount(1) CHwPipelineShader *pShader,
        __in_ecount(1) const MilColorF &materialColor,
        __inout_ecount(1) MILSPHandle &hStartRegister
        ) const override;

    void Transform(
        TransformType type,
        __in_ecount(1) const CMILMatrix *pTransform,
        float flScale
        ) override;

    dxlayer::vector3 m_vec3PositionViewSpace;
    dxlayer::vector3 m_vec3Position;

    float m_flRangeViewSpace;
    float m_flRange;    

    float m_flAttenuation0ViewSpace;
    float m_flAttenuation0;
    float m_flAttenuation1ViewSpace;
    float m_flAttenuation1;
    float m_flAttenuation2ViewSpace;
    float m_flAttenuation2;  

    float m_flFalloff;
    float m_flCosTheta;
    float m_flCosPhi;
};

//+----------------------------------------------------------------------------
//
//  Class:     CMILLightSpot
//
//  Synopsis:  Spot light class
//

class CMILLightSpot :
    public CMILLightPoint
{
public:
    CMILLightSpot() {};

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILLightSpot));

    void Set(
        __in_ecount(1) const MilColorF *pcolorDiffuse,
        __in_ecount(1) const dxlayer::vector3 *pvec3Direction,
        __in_ecount(1) const dxlayer::vector3 *pvec3Position,
        float flRange,
        float flTheta,
        float flPhi,
        float flAttenuation0,
        float flAttenuation1,
        float flAttenuation2
        );

    float GetSpotlightFactor(__in_ecount(1) dxlayer::vector3 const *vec3ToLight) const override;
    bool IsSpot() const override;

    HRESULT SendShaderData(
        __in_ecount(1) CHwPipelineShader *pShader,
        __in_ecount(1) const MilColorF &materialColor,
        __inout_ecount(1) MILSPHandle &hStartRegister
        ) const override;

    void Transform(
        TransformType type,
        __in_ecount(1) const CMILMatrix *pTransform,
        float flScale
        ) override;
};




