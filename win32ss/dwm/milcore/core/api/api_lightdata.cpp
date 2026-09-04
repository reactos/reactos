// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      CMILLightData implementation
//

#include "precomp.hpp"
#include <memory>

using namespace dxlayer;

MtDefine(CMILLightData, MILApi, "CMILLightData");

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::Reset
//
//  Synopsis: Reset the light state to no lights and reasonable default values.
//            Deletes the lights if fDeleteLights is true.
//
//-----------------------------------------------------------------------------
void 
CMILLightData::Reset(bool fDeleteLights)
{
    m_vec3CameraPosition.x = 0.0;
    m_vec3CameraPosition.y = 0.0;
    m_vec3CameraPosition.z = 0.0;
    m_lightAmbient.m_colorDiffuse.r = 0.0;
    m_lightAmbient.m_colorDiffuse.g = 0.0;
    m_lightAmbient.m_colorDiffuse.b = 0.0;
    m_lightAmbient.m_colorDiffuse.a = 1.0;
    m_flSpecularPower = 40.0;
    m_fCalcDiffuse = m_fCalcSpecular = false;

    // We don't ALWAYS need to delete the lights because normally the pointers 
    // point to the stack allocated instances contained by the DUCE light classes
    if (fDeleteLights)
    {
        for (UINT i = 0; i < m_dynDirectionalLights.GetCount(); i++)
        {
            delete m_dynDirectionalLights[i];
        }
        
        for (UINT i = 0; i < m_dynPointAndSpotLights.GetCount(); i++)
        {
            delete m_dynPointAndSpotLights[i];
        }
    }

    m_dynDirectionalLights.Reset(FALSE);
    m_dynPointAndSpotLights.Reset(FALSE);
    m_flNormalScale = -FLT_MAX;

    //
    // Tracking the number of pointlights allows us to quickly determine how many
    // of each light type are avaiable.  We need this because point and spot lights
    // are stored in the same array.
    //
    m_uNumPointLights = 0;

#if DBG
    m_uDbgNumSpotLights = 0;
#endif

    m_lvLightingPass = CHwShader::LV_None;
    m_hFirstConstantParameter = MILSP_INVALID_HANDLE;
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::EnableDiffuseAndSpecularCalculation
//
//  Synopsis: Enable/disable diffuse and specular computation in
//            GetLightContribution. 
//
//            Ambient calculation always happens regardless
//
//-----------------------------------------------------------------------------
void 
CMILLightData::EnableDiffuseAndSpecularCalculation(
    bool fCalcDiffuse,
    bool fCalcSpecular
    )
{
    m_fCalcDiffuse = fCalcDiffuse;
    m_fCalcSpecular = fCalcSpecular;
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::SetCameraPosition
//
//  Synopsis: If you want to do specular lighting, call this with the camera's
//            world space position. 
//
//            NOTE: CMILMesh3D::PrecomputeLighting() calls this for you
//
//-----------------------------------------------------------------------------
void 
CMILLightData::SetCameraPosition(
    float flX,
    float flY,
    float flZ
    )
{
    m_vec3CameraPosition.x = flX;
    m_vec3CameraPosition.y = flY;
    m_vec3CameraPosition.z = flZ;
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::SetFlipNormals
//
//  Synopsis: True if the normals should be reflected (* -1) during
//            GetLightContribution().  This is used to light backwards facing
//            triangle.
//
//            NOTE: CMILMesh3D::PrecomputeLighting() calls this for you
//
//-----------------------------------------------------------------------------
void 
CMILLightData::SetReflectNormals(
    bool fReflectNormals
    )
{    
    m_flNormalScale = fReflectNormals ? -1.0f : 1.0f;
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::AddAmbient
//
//  Synopsis: Adds the ambient light value to the scene
//
//-----------------------------------------------------------------------------
void 
CMILLightData::AddAmbientLight(
    __in_ecount(1) const CMILLightAmbient *pAmbientLight
    )
{ 
    Assert(pAmbientLight);

    m_lightAmbient.m_colorDiffuse.r += pAmbientLight->m_colorDiffuse.r; 
    m_lightAmbient.m_colorDiffuse.g += pAmbientLight->m_colorDiffuse.g;
    m_lightAmbient.m_colorDiffuse.b += pAmbientLight->m_colorDiffuse.b;
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::AddDirectionalLight
//
//  Synopsis: Add a directional light to the scene
//
//-----------------------------------------------------------------------------
HRESULT 
CMILLightData::AddDirectionalLight(
    __in_ecount(1) CMILLightDirectional *pDirectionalLight
    )
{ 
    HRESULT hr = S_OK;
    
    if (IsFiniteVec3(&pDirectionalLight->m_vec3InvDirection))
    {
        IFC(m_dynDirectionalLights.Add(pDirectionalLight));
    }

Cleanup: 
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::AddPointLight
//
//  Synopsis: Adds a point light to the scene. 
//
//-----------------------------------------------------------------------------
HRESULT
CMILLightData::AddPointLight(
    __in_ecount(1) CMILLightPoint *pPointLight
    )
{
    HRESULT hr = S_OK;
    bool fLightAdded = false;

    IFC(AddPointOrSpotLightInternal(
        pPointLight,
        fLightAdded
        ));

    if (fLightAdded)
    {
        m_uNumPointLights++;
    }

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::AddSpotLight
//
//  Synopsis: Adds a spot light to the scene. 
//
//      Note: If the attenuation and range values are negative or the
//            attenuation is infinite or if the direction isn't finite this
//            will return S_OK but not actually add the light.
//
//-----------------------------------------------------------------------------
HRESULT
CMILLightData::AddSpotLight(
    __in_ecount(1) CMILLightSpot *pSpotLight
    )
{
    HRESULT hr = S_OK;
    bool fLightAdded = false;

    if (IsFiniteVec3(&pSpotLight->m_vec3InvDirection))
    {
        IFC(AddPointOrSpotLightInternal(
                pSpotLight,
                fLightAdded
                ));
    }

#if DBG
    if (fLightAdded)
    {
        m_uDbgNumSpotLights++;
    }
#endif

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::RemoveAmbient
//
//  Synopsis: Subtracts the ambient light value from the scene
//
//-----------------------------------------------------------------------------
void 
CMILLightData::SubtractAmbientLight(
    __in_ecount(1) const CMILLightAmbient *pAmbientLight
    )
{ 
    Assert(pAmbientLight);

    m_lightAmbient.m_colorDiffuse.r -= pAmbientLight->m_colorDiffuse.r; 
    m_lightAmbient.m_colorDiffuse.g -= pAmbientLight->m_colorDiffuse.g;
    m_lightAmbient.m_colorDiffuse.b -= pAmbientLight->m_colorDiffuse.b;
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::SetMaterialSpecularPower
//
//  Synopsis: Sets the specular exponent used in the specular lighting 
//            computation. Specular power is a per-material property.
//
//  Returns:  False if flSpecularPower is different from the last specular
//            power passed to the light data. True otherwise.
//
//            **IMPORTANT** **IMPORTANT** **IMPORTANT**
//
//            If this returns false be sure to call InvalidateColorCache
//            (w/ spec enabled) oherwise your specular shine won't change!
//
//-----------------------------------------------------------------------------
bool
CMILLightData::SetMaterialSpecularPower(float flSpecularPower)
{
    if (flSpecularPower != m_flSpecularPower)
    {
        m_flSpecularPower = flSpecularPower;
        return false;
    }

    return true;
}

//+------------------------------------------------------------------------
//
//  Function:  RGBAreEqual
//
//  Synopsis:  Returns true if color1 and color2 have equal RGB values.
//
//  Assumptions: We do not care about the value of Alpha in lighting.
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE bool RGBAreEqual(
    __in_ecount(1) const MilColorF &color1,
    __in_ecount(1) const MilColorF &color2
    )
{
    return color1.r == color2.r && color1.g == color2.g && color1.b == color2.b;
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::SetMaterialAmbientColor
//
//  Synopsis: Sets the ambient color used in the ambient lighting 
//            computation. Ambient color is a per-material property.
//
//  Returns:  False if pAmbientColor is different from the last ambient
//            color passed to the light data. True otherwise.
//
//            **IMPORTANT** **IMPORTANT** **IMPORTANT**
//
//            If this returns false be sure to call InvalidateColorCache.
//
//-----------------------------------------------------------------------------
bool
CMILLightData::SetMaterialAmbientColor(__in_ecount(1) const MilColorF &ambientColorScRGB)
{
    // All 3D lighting is performed in sRGB space so we convert on set.
    // This is consistent with setting the light colors in api_lights.cpp
    MilColorF ambientColor = Convert_MilColorF_scRGB_To_MilColorF_sRGB(&ambientColorScRGB);
    Premultiply(&ambientColor);
    ambientColor.a = 0.0f;
    
    if (!RGBAreEqual(ambientColor, m_matAmbientColor))
    {
        m_matAmbientColor = ambientColor;
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::SetMaterialDiffuseColor
//
//  Synopsis: Sets the diffuse color used in the diffuse lighting 
//            computation. Diffuse color is a per-material property.
//
//  Returns:  False if pDiffuseColor is different from the last diffuse
//            color passed to the light data. True otherwise.
//
//            **IMPORTANT** **IMPORTANT** **IMPORTANT**
//
//            If this returns false be sure to call InvalidateColorCache.
//
//-----------------------------------------------------------------------------
bool
CMILLightData::SetMaterialDiffuseColor(__in_ecount(1) const MilColorF &diffuseColorScRGB)
{
    // All 3D lighting is performed in sRGB space so we convert on set.
    // This is consistent with setting the light colors in api_lights.cpp
    MilColorF diffuseColor = Convert_MilColorF_scRGB_To_MilColorF_sRGB(&diffuseColorScRGB);
    Premultiply(&diffuseColor);
    
    if (!RtlEqualMemory(&diffuseColor, &m_matDiffuseColor, sizeof(diffuseColor)))
    {
        m_matDiffuseColor = diffuseColor;
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::SetMaterialSpecularColor
//
//  Synopsis: Sets the specular color used in the specular lighting 
//            computation. Specular color is a per-material property.
//
//  Returns:  False if pSpecularColor is different from the last specular
//            color passed to the light data. True otherwise.
//
//            **IMPORTANT** **IMPORTANT** **IMPORTANT**
//
//            If this returns false be sure to call InvalidateColorCache.
//
//-----------------------------------------------------------------------------
bool
CMILLightData::SetMaterialSpecularColor(__in_ecount(1) const MilColorF &specularColorScRGB)
{
    // All 3D lighting is performed in sRGB space so we convert on set.
    // This is consistent with setting the light colors in api_lights.cpp
    MilColorF specularColor = Convert_MilColorF_scRGB_To_MilColorF_sRGB(&specularColorScRGB);
    Premultiply(&specularColor);
    specularColor.a = 0.0f;
    
    if (!RGBAreEqual(specularColor, m_matSpecularColor))
    {
        m_matSpecularColor = specularColor;
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::SetMaterialEmissiveColor
//
//  Synopsis: Sets the emissive color used in the emissive lighting 
//            computation. Emissive color is a per-material property.
//
//  Returns:  False if pEmissiveColor is different from the last emissive
//            color passed to the light data. True otherwise.
//
//            (Changes to EmissiveColor DO NOT require InvalidateColorCache.)
//
//-----------------------------------------------------------------------------
void
CMILLightData::SetMaterialEmissiveColor(__in_ecount(1) const MilColorF &emissiveColorScRGB)
{
    // All 3D lighting is performed in sRGB space so we convert on set.
    // This is consistent with setting the light colors in api_lights.cpp
    m_matEmissiveColor = Convert_MilColorF_scRGB_To_MilColorF_sRGB(&emissiveColorScRGB);
    Premultiply(&m_matEmissiveColor);
    m_matEmissiveColor.a = 0.0f;
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::GetMaterialEmissiveColor
//
//  Synopsis: Returns the emissive color used in the emissive lighting 
//            computation. Emissive color is a per-material property.
//
//-----------------------------------------------------------------------------
MilColorF
CMILLightData::GetMaterialEmissiveColor() const
{
    return m_matEmissiveColor;
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::Transform
//
//  Synopsis: Transforms the lights by pmatTransform and flScale
//
//  Here's how lighting transformation currently works:
//
//      1) Begin3D clears out the CMILLightData
//      2) Prerender... 
//          - Creates CMILLights
//          - Transforms lights to view space using TransformType_ViewSpace
//          - Adds them to the CMILLightData
//      3) DrawMesh3D
//          - Shader Path
//              - Sends view space data to the card
//          - Fixed Function
//              - if world view transform is a uniform SRT transform
//                  - transform lights to model space using 
//                    TransformType_LightingSpace
//              - else
//                  - copy view space light information to lighting space using
//                    TransformType_Copy
//
//  We need separate view space and model space lighting information because
//  the transformation to model space is different for each model.
//
//  The sw lighting path operates on only the lighting space
//  information so when we do sw lighting in view space we copy the view
//  space information to the lighting space variables using TransformType_Copy
//  to save some matrix mults.
//
//-----------------------------------------------------------------------------
void 
CMILLightData::Transform(
    CMILLight::TransformType type,
    __in_ecount(1) const CMILMatrix *pmatTransform,
    float flScale
    )
{  
    Assert(!(flScale == 0.0));
    
    // 1. Ambient Light -- no transform necessary

    // 2. Directional Lights
    for (UINT i = 0; i < m_dynDirectionalLights.GetCount(); i++)
    {
        m_dynDirectionalLights[i]->Transform(type, pmatTransform, flScale);
    }

    // 3. Point and Spot Lights
    for (UINT i = 0; i < m_dynPointAndSpotLights.GetCount(); i++)
    {
        m_dynPointAndSpotLights[i]->Transform(type, pmatTransform, flScale);
    }  
}

//-----------------------------------------------------------------------------
//
//  Function: AddColorRGB
//
//  Synopsis: Adds pcolIn to pcolOut
//
//  Assumptions: We do not care about the value of Alpha in lighting.
//
//-----------------------------------------------------------------------------
MIL_FORCEINLINE void
AddColorRGB(
    __inout_ecount(1) dxlayer::color *pcolOut, 
    __in_ecount(1) const MilColorF *pcolIn)
{
    pcolOut->r += pcolIn->r;
    pcolOut->g += pcolIn->g;
    pcolOut->b += pcolIn->b;
}

//-----------------------------------------------------------------------------
//
//  Function: MAddColorRGB
//
//  Synopsis: The other arguments are multiplied together and added to
//            pcolOut.  That is:
//
//            pcolOut += pcolLight * flScalar
//
//            Where:
//              pcolLight is the light color
//              flScalar is a float scalar (e.g., N dot L)
//
//  Assumptions: We do not care about the value of Alpha in lighting.
//
//-----------------------------------------------------------------------------
MIL_FORCEINLINE void
MAddColorRGB(
    __inout_ecount(1) MilColorF *pcolOut, 
    __in_ecount(1) const MilColorF *pcolLight,
    float flScalar)
{
    pcolOut->r += pcolLight->r * flScalar;
    pcolOut->g += pcolLight->g * flScalar;
    pcolOut->b += pcolLight->b * flScalar;
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::GetLightContribution
//
//  Synopsis: Computes the specular and diffuse color of pvec3VertexPosition
//            for all lights. This is called by PrecomputeLighting
//
//-----------------------------------------------------------------------------
void
CMILLightData::GetLightContribution(
    __in_ecount(1) const vector3 *pvec3VertexPosition, 
    __in_ecount(1) const vector3 *pvec3VertexNormal, 
    __out_ecount(1) D3DCOLOR *pdwDiffuse, 
    __out_ecount(1) D3DCOLOR *pdwSpecular
    )
{    
    // WPF specifies that Normals in a MeshGeometry3D are associated with the
    // CCW side of the triangle.  If we are drawing the CW side we reflect
    // the normal for lighting.  Rather than modify pvec3VertexNormal we do
    // this by negating the sign of the dotproducts involving N.

    // Did you forget to call SetReflectNormals?
    Assert(m_flNormalScale == 1 || m_flNormalScale == -1);

    Assert(!m_fCalcDiffuse || pdwDiffuse);
    Assert(!m_fCalcSpecular || pdwSpecular);

    MilColorF colorDiffuse, colorSpecular;
    colorDiffuse.a = 1.0f;
    colorDiffuse.r  = colorDiffuse.g  = colorDiffuse.b  = 0.0f;
    colorSpecular.a = 0.0f;
    colorSpecular.r = colorSpecular.g = colorSpecular.b = 0.0f;

    for (UINT i = 0; i < m_dynDirectionalLights.GetCount(); i++)
    {
        const CMILLightDirectional *light = m_dynDirectionalLights[i];

        float flNDotL = vector3::dot_product(*pvec3VertexNormal, light->m_vec3InvDirection) * m_flNormalScale;

        if (flNDotL > 0.0)
        {
            if (m_fCalcDiffuse)
            {
                MAddColorRGB(
                    &colorDiffuse, 
                    &light->m_colorDiffuse,
                    flNDotL
                    );
            }        

            if (m_fCalcSpecular)
            {
                vector3 vec3H; 
                ComputeHalfVector(
                    &m_vec3CameraPosition,
                    pvec3VertexPosition,
                    &light->m_vec3InvDirection,
                    &vec3H
                    ); 

                float flNDotH = vector3::dot_product(*pvec3VertexNormal, vec3H) * m_flNormalScale;

                if (flNDotH > 0.0)
                {   
                    // powf ends up using the built in slow pow
                    // (float)pow ends up using the faster GpIntrinsics::Pow
                    MAddColorRGB(
                        &colorSpecular, 
                        &light->m_colorDiffuse,
                        static_cast<float>(pow(flNDotH, m_flSpecularPower))
                        );
                }
            } 
        }
    }

    for (UINT i = 0; i < m_dynPointAndSpotLights.GetCount(); i++)
    {
        const CMILLightPoint *light = m_dynPointAndSpotLights[i];

        vector3 vec3L = light->m_vec3Position - *pvec3VertexPosition;
        float flDistance = vec3L.length_sq();

        if (flDistance > 0)
        {
            flDistance = static_cast<float>(sqrt(flDistance));
            // normalize vec3L
            vec3L *= 1.0f / flDistance;

            float flNDotL = vector3::dot_product(*pvec3VertexNormal, vec3L) * m_flNormalScale;

            if (flNDotL > 0.0)
            {
                if (flDistance <= light->m_flRange)
                {
                    float flAttenuationDenominator = 
                        light->m_flAttenuation0 + 
                        light->m_flAttenuation1 * flDistance +
                        light->m_flAttenuation2 * flDistance * flDistance;

                    //
                    // By using max of 1 and the Attenuation Values, we ensure that attenuation is only
                    // diminishing the light and not amplifying or negating it.
                    //
                    float flAttenuation = 1.0f / max(flAttenuationDenominator, 1.0f);

                    float flSpot = light->GetSpotlightFactor(&vec3L);

                    if (m_fCalcDiffuse)
                    {     
                        MAddColorRGB(
                            &colorDiffuse, 
                            &light->m_colorDiffuse,
                            flNDotL * flAttenuation * flSpot
                            );
                    }

                    if (m_fCalcSpecular)
                    {
                        vector3 vec3H; 
                        ComputeHalfVector(
                            &m_vec3CameraPosition,
                            pvec3VertexPosition,
                            &vec3L,
                            &vec3H
                            );                       

                        float flNDotH = vector3::dot_product(*pvec3VertexNormal, vec3H) * m_flNormalScale;

                        if (flNDotH > 0.0)
                        {   
                            // powf ends up using the built in slow pow
                            // (float)pow ends up using the faster GpIntrinsics::Pow
                            float flScalarFactor = 
                                static_cast<float>(pow(flNDotH, m_flSpecularPower)) * 
                                flAttenuation * flSpot;

                            MAddColorRGB(
                                &colorSpecular,
                                &light->m_colorDiffuse,
                                flScalarFactor
                                );
                        }                 
                    } 
                }
            }
        }
    }

    if (m_fCalcDiffuse)
    {
        Assert(pdwDiffuse);

        // Multiply the diffuse color knob
        Assert(colorDiffuse.a == 1.0f);
        colorDiffuse.a = m_matDiffuseColor.a;
        colorDiffuse.r *= m_matDiffuseColor.r;
        colorDiffuse.g *= m_matDiffuseColor.g;
        colorDiffuse.b *= m_matDiffuseColor.b;

        // Add the ambient lighting value (including color knob)
        colorDiffuse.r += m_lightAmbient.m_colorDiffuse.r * m_matAmbientColor.r;
        colorDiffuse.g += m_lightAmbient.m_colorDiffuse.g * m_matAmbientColor.g;
        colorDiffuse.b += m_lightAmbient.m_colorDiffuse.b * m_matAmbientColor.b;
        
        if (colorDiffuse.r > 1.0)
            colorDiffuse.r = 1.0;
        if (colorDiffuse.g > 1.0)
            colorDiffuse.g = 1.0;
        if (colorDiffuse.b > 1.0)
            colorDiffuse.b = 1.0;

        *pdwDiffuse = Convert_MilColorF_sRGB_To_D3DCOLOR_ZeroAlpha(&colorDiffuse);
        if (colorDiffuse.a == 1.0f)
        {
            // a minor perf optimization.
            *pdwDiffuse |= 0xFF000000;
        }
        else
        {
            Put_sRGB_Alpha_In_D3DCOLOR_WithNoAlpha(colorDiffuse.a, pdwDiffuse);
        }
    }

    if (m_fCalcSpecular)
    {
        Assert(pdwSpecular);

        // Multiply the specular color knob
        {
            colorSpecular.r *= m_matSpecularColor.r;
            colorSpecular.g *= m_matSpecularColor.g;
            colorSpecular.b *= m_matSpecularColor.b;
        }
        
        if (colorSpecular.r > 1.0)
            colorSpecular.r = 1.0;
        if (colorSpecular.g > 1.0)
            colorSpecular.g = 1.0;
        if (colorSpecular.b > 1.0)
            colorSpecular.b = 1.0;
        
        *pdwSpecular = Convert_MilColorF_sRGB_To_D3DCOLOR_ZeroAlpha(&colorSpecular);
    }
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::SendShaderData
//
//  Synopsis: Sends the shader constants
//
//-----------------------------------------------------------------------------
HRESULT
CMILLightData::SendShaderData(
    MILSPHandle hParameter,
    __in_ecount(1) CHwPipelineShader *pShader
    ) const
{
    HRESULT hr = S_OK;

    if (m_lvLightingPass == CHwShader::LV_Diffuse || m_lvLightingPass == CHwShader::LV_Specular)
    {
        // ORDER
        //
        // 1. ambient light or get spec power
        // 2. directional lights
        // 3. point lights
        // 4. spot lights

        MilColorF materialColor;

        if (m_lvLightingPass == CHwShader::LV_Diffuse)
        {
            Assert(m_fCalcDiffuse);

            MilColorF matAmbientColorModified;
            //
            // The ambient fragment is responsible for setting the initial
            // lighting color in pipeline. No other lighting fragment changes
            // the alpha component. All other lights just add color values. We
            // can therefore squeeze the diffuse material color alpha into the
            // ambient lighting fragment parameter in order to simulate
            // multiplying this value by the sum of all diffuse lighting
            // calculations (which produce alpha == 1).
            //
            // Note that that the diffuse color values are multiplied in a
            // different way. We mutiply the diffuse color values into every
            // light color before sending the light color to the shader.
            //
            matAmbientColorModified.a = m_matDiffuseColor.a;

            matAmbientColorModified.r = m_matAmbientColor.r;
            matAmbientColorModified.g = m_matAmbientColor.g;
            matAmbientColorModified.b = m_matAmbientColor.b;
            
            // 1. ambient light
            IFC(m_lightAmbient.SendShaderData(pShader, matAmbientColorModified, hParameter));

            //
            // It is not strictly necessary to set this to materialColor.a to 0
            // given that no shader fragment will attempt to use it.
            // Nevertheless, it is clearer to do so.
            //
            materialColor.a = 0.0f;
            materialColor.r = m_matDiffuseColor.r;
            materialColor.g = m_matDiffuseColor.g;
            materialColor.b = m_matDiffuseColor.b;
        }
        else
        {
            Assert(m_fCalcSpecular);
            
            // 1. get spec pow
            vector4 vec4SpecPower(m_flSpecularPower, 0.0f, 0.0f, 0.0f);
            IFC(pShader->SetFloat4(
                hParameter, 
                static_cast<const std::array<float,4>>(vec4SpecPower).data()
                ));

            hParameter += 
                GetShaderConstantRegisterSize(ShaderFunctionConstantData::Float4);    

            materialColor = m_matSpecularColor;
            Assert(materialColor.a == 0.0f);
        }
        
        // 2. directional lights
        for (UINT i = 0; i < m_dynDirectionalLights.GetCount(); i++)
        { 
            IFC(m_dynDirectionalLights[i]->SendShaderData(pShader, materialColor, hParameter));
        }

        // 3. point lights
        for (UINT i = 0; i < m_dynPointAndSpotLights.GetCount(); i++)
        {
            const CMILLightPoint *light = m_dynPointAndSpotLights[i];

            if (!light->IsSpot())
            {
                IFC(light->SendShaderData(pShader, materialColor, hParameter));   
            }
        }   

        // 4. spot lights
        for (UINT i = 0; i < m_dynPointAndSpotLights.GetCount(); i++)
        {
            const CMILLightPoint *light = m_dynPointAndSpotLights[i];

            if (light->IsSpot())
            {
                IFC(light->SendShaderData(pShader, materialColor, hParameter)); 
            }
        } 
    }
    else if (m_lvLightingPass == CHwShader::LV_Emissive)
    {
        IFC(pShader->SetFloat4(
            hParameter, 
            reinterpret_cast<const float *>(&m_matEmissiveColor)
            ));

        hParameter += 
            GetShaderConstantRegisterSize(ShaderFunctionConstantData::Float4); 
    }

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      RequiresDestinationBlending
//
//  Synopsis:  
//      Returns whether the rendering of this light will require blending with
//  the destination. If this function returns false, SrCopy could be used.
//

bool
CMILLightData::RequiresDestinationBlending() const
{
    bool fRequiresDestinationBlending;

    if (   m_lvLightingPass == CHwShader::LV_Diffuse
        && m_matDiffuseColor.a >= 1.0f
       )
    {
        fRequiresDestinationBlending = false;
    }
    else
    {
        fRequiresDestinationBlending = true;
    }

    return fRequiresDestinationBlending;
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::ComputeHalfVector
//
//  Synopsis: Computes the half vector used in specular lighting
//
//               H = norm(norm(C_p - V_p) + L)
//
//-----------------------------------------------------------------------------
MIL_FORCEINLINE void
CMILLightData::ComputeHalfVector(
    __in_ecount(1) const dxlayer::vector3 *pvec3CameraPos,
    __in_ecount(1) const dxlayer::vector3 *pvec3VertexPos,
    __in_ecount(1) const dxlayer::vector3 *pvec3L,
    __out_ecount(1) dxlayer::vector3 *pvec3H
    )
{
    vector3 result = *pvec3CameraPos - *pvec3VertexPos;
    result = result.normalize();

    result += *pvec3L;
    *pvec3H = result.normalize();
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::AddPointLightInternal
//
//  Synopsis: Adds a point/spot light to the scene. 
//
//      Note: If the attenuation and range values are negative or the
//            attenuation is infinite or if the direction isn't finite this
//            will return S_OK but not actually add the light.
//
//-----------------------------------------------------------------------------
HRESULT
CMILLightData::AddPointOrSpotLightInternal(
    __in_ecount(1) CMILLightPoint *pPointLight,
    bool &fLightAdded
    )
{
    HRESULT hr = S_OK;

    if (
         // attenuations are all non-negative
        (pPointLight->m_flAttenuation0 >= 0.0 && 
         pPointLight->m_flAttenuation1 >= 0.0 &&
         pPointLight->m_flAttenuation2 >= 0.0) &&

         // ... are all finite
        (_finite(pPointLight->m_flAttenuation0) && 
         _finite(pPointLight->m_flAttenuation1) &&
         _finite(pPointLight->m_flAttenuation2)) &&

         // ... and at least one is positive
        (pPointLight->m_flAttenuation0 > 0.0 || 
         pPointLight->m_flAttenuation1 > 0.0 || 
         pPointLight->m_flAttenuation2 > 0.0) &&
        
         // range is non-negative (and possibly infinite)
         pPointLight->m_flRange > 0.0 &&

         // position is finite
         IsFiniteVec3(&pPointLight->m_vec3Position)
       )
    {     
        IFC(m_dynPointAndSpotLights.Add(pPointLight));
        fLightAdded = true;
    }
    else
    {
        fLightAdded = false;
    }

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightData::SetLightingPass
//
//  Synopsis: The shader path uses this to tell the light data what type of
//            lighting pipeline items to add this pass (diffuse, specular, 
//            etc.)
//
//-----------------------------------------------------------------------------
void 
CMILLightData::SetLightingPass(int lvLightingPass)
{
    m_lvLightingPass = lvLightingPass;
}

//+----------------------------------------------------------------------------
// 
//  Member: 
//      CMILLightData::Copy 
// 
//  Synopsis: 
//      Copy data from another CMILLightData
// 
//--------------------------------------------------------------------------- 
HRESULT
CMILLightData::Copy(
    __in_ecount(1) CMILLightData const &rvalue
    )
{
    HRESULT hr;

    //
    // Copy dynamic array members
    //

    m_dynDirectionalLights.Reset(FALSE);
    IFC(m_dynDirectionalLights.AddMultipleAndSet(
        rvalue.m_dynDirectionalLights.GetDataBuffer(),
        rvalue.m_dynDirectionalLights.GetCount()
        ));

    m_dynPointAndSpotLights.Reset(FALSE);
    IFC(m_dynPointAndSpotLights.AddMultipleAndSet(
        rvalue.m_dynPointAndSpotLights.GetDataBuffer(),
        rvalue.m_dynPointAndSpotLights.GetCount()
        ));

    //
    // Copy other members
    // 

    RtlCopyMemory(this, &rvalue, offsetof(CMILLightData, m_dynDirectionalLights));

    size_t const offsetPostDynArrays =
        offsetof(CMILLightData,m_dynPointAndSpotLights)+sizeof(m_dynPointAndSpotLights);
    size_t const sizePostDynArrays =
        sizeof(CMILLightData)-offsetPostDynArrays;

    RtlCopyMemory(reinterpret_cast<BYTE *>(this)+offsetPostDynArrays,
                  reinterpret_cast<BYTE const *>(&rvalue)+offsetPostDynArrays,
                  sizePostDynArrays
                  );

Cleanup:
    RRETURN(hr);
}





