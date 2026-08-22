// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      CMILLightData declaration
//

MtExtern(CMILLightData);

//+----------------------------------------------------------------------------
//
//  Class:     CMILLightData
//
//  Synopsis:  This class encapsulates the current lighting state
//

class CHwShaderPipelineBuilder;
class CHwPipelineShader;

class CMILLightData
{
public:
    
    CMILLightData() { Reset(); }
    ~CMILLightData() { }

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILLightData));

    void Reset(bool fDeleteLights = false);    

    void EnableDiffuseAndSpecularCalculation(
        bool fCalcDiffuse,
        bool fCalcSpecular
        );

    bool IsDiffuseEnabled() const
    {
        return m_fCalcDiffuse;
    }

    bool IsSpecularEnabled() const
    {
        return m_fCalcSpecular;
    }
    
    bool SetMaterialSpecularPower(float flSpecularPower);
    bool SetMaterialAmbientColor(__in_ecount(1) const MilColorF &ambientColorScRGB);
    bool SetMaterialDiffuseColor(__in_ecount(1) const MilColorF &diffuseColorScRGB);
    bool SetMaterialSpecularColor(__in_ecount(1) const MilColorF &specularColorScRGB);
    void SetMaterialEmissiveColor(__in_ecount(1) const MilColorF &emissiveColorScRGB);

    MilColorF GetMaterialEmissiveColor() const;

    void SetCameraPosition(
        float flX,
        float flY,
        float flZ
        );

    void SetReflectNormals(
        bool fReflectNormals
        );

    void Transform(
        CMILLight::TransformType type,
        __in_ecount(1) const CMILMatrix *pmatTransform,
        float flDistanceScale
        );

    void GetLightContribution(
        __in_ecount(1) const dxlayer::vector3 *pvec3VertexPosition,
        __in_ecount(1) const dxlayer::vector3 *pvec3VertexNormal,
        __out_ecount(1) D3DCOLOR *pdwDiffuse,
        __out_ecount(1) D3DCOLOR *pdwSpecular
        );

    void AddAmbientLight(
        __in_ecount(1) const CMILLightAmbient *pAmbientLight
        );


    void SubtractAmbientLight(
        __in_ecount(1) const CMILLightAmbient *pAmbientLight
        );
    
    HRESULT AddDirectionalLight(
        __in_ecount(1) CMILLightDirectional *pDirectionalLight
        );

    HRESULT AddPointLight(
        __in_ecount(1) CMILLightPoint *pPointLight
        );

    HRESULT AddSpotLight(
        __in_ecount(1) CMILLightSpot *pSpotLight
        );

    HRESULT SendShaderData(
        MILSPHandle hParameter,
        __in_ecount(1) CHwPipelineShader *pShader
        ) const;

    void SetLightingPass(int lvLightingPass);

    float GetNormalScale() const
    {
        return m_flNormalScale;
    }

    int GetLightingPass() const
    {
        return m_lvLightingPass;
    }

    UINT GetNumDirectionalLights() const
    {
        return m_dynDirectionalLights.GetCount();
    }

    UINT GetNumPointLights() const
    {
        return m_uNumPointLights;
    }

    UINT GetNumSpotLights() const
    {
#if DBG
        Assert(m_uDbgNumSpotLights == (m_dynPointAndSpotLights.GetCount() - m_uNumPointLights));
#endif
        return m_dynPointAndSpotLights.GetCount() - m_uNumPointLights;
    }

    bool RequiresDestinationBlending() const;

    HRESULT Copy(__in_ecount(1) CMILLightData const &rvalue);

private:

    static void ComputeHalfVector(
        __in_ecount(1) const dxlayer::vector3 *pvec3CameraPos,
        __in_ecount(1) const dxlayer::vector3 *pvec3VertexPos,
        __in_ecount(1) const dxlayer::vector3 *pvec3L,
        __out_ecount(1) dxlayer::vector3 *pvec3H
        );

    HRESULT AddPointOrSpotLightInternal(
        __in_ecount(1) CMILLightPoint *pPointLight,
        bool &fLightAdded
        );

private:
    // Note: because this class has a Copy method any member changes likely
    //       require an update to the Copy implementation.

    bool m_fCalcDiffuse;
    bool m_fCalcSpecular;
    float m_flSpecularPower;

    // These colors are stored in premultiplied format
    MilColorF m_matAmbientColor;
    MilColorF m_matDiffuseColor;
    MilColorF m_matSpecularColor;
    MilColorF m_matEmissiveColor;

    dxlayer::vector3 m_vec3CameraPosition;
    float m_flNormalScale;
    
    CMILLightAmbient m_lightAmbient;
    DynArray<CMILLightDirectional *> m_dynDirectionalLights;
    DynArray<CMILLightPoint *> m_dynPointAndSpotLights;

    //
    // Because we use 1 array for both point and spot lights, keep track
    // of the number of each seperately.
    //
    UINT m_uNumPointLights;

#if DBG
    UINT m_uDbgNumSpotLights;
#endif

    int m_lvLightingPass;
    mutable MILSPHandle m_hFirstConstantParameter;
};




