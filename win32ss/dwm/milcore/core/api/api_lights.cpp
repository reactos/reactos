// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Definition of all MIL light classes:
//          - CMILLight
//          - CMILLightAmbient
//          - CMILLightDirectional
//          - CMILLightPoint
//          - CMILLightSpot
//

#include "precomp.hpp"
using namespace dxlayer;

MtDefine(CMILLightAmbient,     MILApi, "CMILLightAmbient");
MtDefine(CMILLightDirectional, MILApi, "CMILLightDirectional");
MtDefine(CMILLightPoint,       MILApi, "CMILLightPoint");
MtDefine(CMILLightSpot,        MILApi, "CMILLightSpot");

//-----------------------------------------------------------------------------
//
//  Function: CMILLightAmbient::Set
//
//  Synopsis: Sets the ambient light's color
//
//-----------------------------------------------------------------------------
void 
CMILLightAmbient::Set(
    __in_ecount(1) const MilColorF *pcolorAmbient
    )
{   
    m_colorDiffuse = Convert_MilColorF_scRGB_To_MilColorF_sRGB(pcolorAmbient); 

#if DBG_ANALYSIS
    m_fDbgHasBeenViewTransformed = false;
#endif
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightAmbient::SendShaderData
//
//  Synopsis: Sends the light's data to the shader. This must be in the same 
//            order as the shader function declaration in 
//            hwhlslshaderfragments.cpp
//
//-----------------------------------------------------------------------------
override HRESULT 
CMILLightAmbient::SendShaderData(
    __in_ecount(1) CHwPipelineShader *pShader,
    __in_ecount(1) const MilColorF &materialColor,
    __inout_ecount(1) MILSPHandle &hCurrentRegister
    ) const
{ 
    HRESULT hr = S_OK;

    //
    // The Material's color modulates the light's color.  Rather than
    // pass both colors to the shader we multiply them before hand.
    //

    MilColorF modulatedLightColor;

    //
    // For an explanation of why we put the material color alpha in the light
    // color, see the ambient light comment in CMILLightData::SendShaderData 
    //
    modulatedLightColor.a = materialColor.a;

    modulatedLightColor.r = m_colorDiffuse.r * materialColor.r;
    modulatedLightColor.g = m_colorDiffuse.g * materialColor.g;
    modulatedLightColor.b = m_colorDiffuse.b * materialColor.b;
    
    IFC(pShader->SetFloat4(
        hCurrentRegister, 
        reinterpret_cast<const float *>(&modulatedLightColor)
        ));

    hCurrentRegister += 
        GetShaderConstantRegisterSize(ShaderFunctionConstantData::Float4); 

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightDirectional::Set
//
//  Synopsis: Sets the properties of the directional light
//
//-----------------------------------------------------------------------------
void 
CMILLightDirectional::Set(
    __in_ecount(1) const MilColorF *pcolorDiffuse,
    __in_ecount(1) const vector3 *pvec3Direction
    )
{ 
    CMILLightAmbient::Set(pcolorDiffuse);

    // reverse the direction now to make dot products easier later
    m_vec3InvDirection = -1.0 * *pvec3Direction; 
    m_vec3InvDirection = m_vec3InvDirection.normalize();
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightDirectional::SendShaderData
//
//  Synopsis: Sends the light's data to the shader. This must be in the same 
//            order as the shader function declaration in 
//            hwhlslshaderfragments.cpp
//
//-----------------------------------------------------------------------------
override HRESULT 
CMILLightDirectional::SendShaderData(
    __in_ecount(1) CHwPipelineShader *pShader,
    __in_ecount(1) const MilColorF &materialColor,
    __inout_ecount(1) MILSPHandle &hCurrentRegister
    ) const
{ 
    HRESULT hr = S_OK;

    // This sends diffuse
    IFC(CMILLightAmbient::SendShaderData(pShader, materialColor, hCurrentRegister));

    IFC(pShader->SetFloat3(
        hCurrentRegister, 
        static_cast<const std::array<float,3>>(m_vec3InvDirectionViewSpace).data(),
        0.0
        ));

    hCurrentRegister += 
        GetShaderConstantRegisterSize(ShaderFunctionConstantData::Float3); 

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightDirectional::Transform
//
//  Synopsis: Transforms the light by pTransform and flScale. See
//            CMILLightData::Transform for more information.
//
//-----------------------------------------------------------------------------
override void 
CMILLightDirectional::Transform(
    TransformType type,
    __in_ecount(1) const CMILMatrix *pTransform,
    float flScale
    )
{
    Assert(!(flScale == 0.0));    

    if (type == TransformType_LightingSpace)
    {
        Assert(m_fDbgHasBeenViewTransformed);
        
        m_vec3InvDirection = 
            math_extensions::transform_normal(m_vec3InvDirectionViewSpace, *pTransform)
            .normalize();
    }
    else if (type == TransformType_ViewSpace)
    {
        Assert(!m_fDbgHasBeenViewTransformed);
        
        m_vec3InvDirectionViewSpace =
            math_extensions::transform_normal(m_vec3InvDirection, *pTransform)
            .normalize();
#if DBG_ANALYSIS
        m_fDbgHasBeenViewTransformed = true;
#endif  
    }
    else if (type == TransformType_Copy)
    {
        Assert(m_fDbgHasBeenViewTransformed);
        
        m_vec3InvDirection = m_vec3InvDirectionViewSpace;
    }
    else
    {
        RIP("CMILLightDirectional::Transform -- unknown TransformType given");
    }
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightPoint::Set
//
//  Synopsis: Sets the properties of the point light
//
//-----------------------------------------------------------------------------
void 
CMILLightPoint::Set(    
    __in_ecount(1) const MilColorF *pcolorDiffuse,
    __in_ecount(1) const vector3 *pvec3Position,
    float flRange,
    float flAttenuation0,
    float flAttenuation1,
    float flAttenuation2
    )
{ 
    CMILLightAmbient::Set(pcolorDiffuse);

    m_vec3Position = *pvec3Position; 
    // D3D imposed limit
    m_flRange = max(min(flRange, sqrtf(FLT_MAX)), 0.0f);
    m_flAttenuation0 = flAttenuation0; 
    m_flAttenuation1 = flAttenuation1; 
    m_flAttenuation2 = flAttenuation2; 

    m_flCosTheta   = 0.0;
    m_flCosPhi     = 0.0;
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightPoint::SendShaderData
//
//  Synopsis: Sends the light's data to the shader. This must be in the same 
//            order as the shader function declaration in 
//            hwhlslshaderfragments.cpp
//
//-----------------------------------------------------------------------------
override HRESULT 
CMILLightPoint::SendShaderData(
    __in_ecount(1) CHwPipelineShader *pShader,
    __in_ecount(1) const MilColorF &materialColor,
    __inout_ecount(1) MILSPHandle &hCurrentRegister
    ) const
{ 
    HRESULT hr = S_OK;

    vector4 vec4Position(m_vec3PositionViewSpace, 1.0f);

    vector4 vec4AttenAndRange = 
    {
        m_flAttenuation0ViewSpace,
        m_flAttenuation1ViewSpace,
        m_flAttenuation2ViewSpace,
        m_flRangeViewSpace
    };

    // This sends diffuse
    IFC(CMILLightAmbient::SendShaderData(pShader, materialColor, hCurrentRegister));
    
    IFC(pShader->SetFloat4(
        hCurrentRegister, 
        static_cast<const std::array<float,4>>(vec4Position).data()
        ));

    hCurrentRegister += 
        GetShaderConstantRegisterSize(ShaderFunctionConstantData::Float4); 

    IFC(pShader->SetFloat4(
        hCurrentRegister, 
        static_cast<const std::array<float,4>>(vec4AttenAndRange).data()
        ));

    hCurrentRegister += 
        GetShaderConstantRegisterSize(ShaderFunctionConstantData::Float4);    

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightPoint::Transform
//
//  Synopsis: Transforms the light by pTransform and flScale. See
//            CMILLightData::Transform for more information
//
//-----------------------------------------------------------------------------
override void 
CMILLightPoint::Transform(
    TransformType type,
    __in_ecount(1) const CMILMatrix *pTransform,
    float flScale
    )
{
    // Assert(flScale > 0) but let NaNs through
    Assert(!(flScale <= 0.0));

    if (type == TransformType_LightingSpace)
    {
        Assert(m_fDbgHasBeenViewTransformed);
        m_vec3Position = 
            math_extensions::transform_coord(m_vec3PositionViewSpace, *pTransform);

        m_flRange = m_flRangeViewSpace * flScale;

        float flInvScale = 1.0f / flScale;

        m_flAttenuation0 = m_flAttenuation0ViewSpace;
        m_flAttenuation1 = m_flAttenuation1ViewSpace * flInvScale;
        m_flAttenuation2 = m_flAttenuation2ViewSpace * flInvScale * flInvScale;         
    }
    else if (type == TransformType_ViewSpace)
    {
        Assert(!m_fDbgHasBeenViewTransformed);
        m_vec3PositionViewSpace =
            math_extensions::transform_coord(m_vec3Position, *pTransform);

        m_flRangeViewSpace = m_flRange * flScale;

        float flInvScale = 1.0f / flScale;

        m_flAttenuation0ViewSpace = m_flAttenuation0;
        m_flAttenuation1ViewSpace = m_flAttenuation1 * flInvScale;
        m_flAttenuation2ViewSpace = m_flAttenuation2 * flInvScale * flInvScale;  

#if DBG_ANALYSIS
        m_fDbgHasBeenViewTransformed = true;
#endif  
    }
    else if (type == TransformType_Copy)
    {
        Assert(m_fDbgHasBeenViewTransformed);
                
        m_vec3Position = m_vec3PositionViewSpace;

        m_flRange = m_flRangeViewSpace;

        m_flAttenuation0 = m_flAttenuation0ViewSpace;
        m_flAttenuation1 = m_flAttenuation1ViewSpace;
        m_flAttenuation2 = m_flAttenuation2ViewSpace;
    }
    else
    {
        RIP("CMILLightPoint::Transform -- unknown TransformType given");
    }
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightPoint::GetSpotlightFactor
//
//  Synopsis: Calculates the spotlight contribution. For a point light, this 
//            is always 1.0f.
//
//  Returns:  1.0f
//
//-----------------------------------------------------------------------------
float 
CMILLightPoint::GetSpotlightFactor(
    __in_ecount(1) vector3 const *vec3ToLight
    ) const
{
    return 1.0f;
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightPoint::IsSpot
//
//  Synopsis: Returns true if this light is a spot light
//
//  Returns:  false
//
//-----------------------------------------------------------------------------
bool
CMILLightPoint::IsSpot() const
{
    return false;
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightSpot::Set
//
//  Synopsis: Sets the properties of the spot light
//
//-----------------------------------------------------------------------------
void 
CMILLightSpot::Set(
    __in_ecount(1) const MilColorF *pcolorDiffuse,
    __in_ecount(1) const vector3 *pvec3Direction,
    __in_ecount(1) const vector3 *pvec3Position,
    float flRange,
    float flTheta,
    float flPhi,
    float flAttenuation0,
    float flAttenuation1,
    float flAttenuation2
    )
{
    CMILLightPoint::Set(
        pcolorDiffuse, 
        pvec3Position, 
        flRange, 
        flAttenuation0, 
        flAttenuation1, 
        flAttenuation2
        );

    // reverse the direction to make dot products easier later on
    m_vec3InvDirection = -1.0 * *pvec3Direction; 
    m_vec3InvDirection = m_vec3InvDirection.normalize();
    
    // D3D imposed limits
    flPhi   = max(min(flPhi, static_cast<float>(M_PI)), 0.0f);
    flTheta = max(min(flTheta, flPhi), 0.0f);

    m_flCosTheta = cosf(0.5f * flTheta);
    m_flCosPhi   = cosf(0.5f * flPhi);
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightSpot::SendShaderData
//
//  Synopsis: Sends the light's data to the shader. This must be in the same 
//            order as the shader function declaration in 
//            hwhlslshaderfragments.cpp
//
//-----------------------------------------------------------------------------
override HRESULT 
CMILLightSpot::SendShaderData(
    __in_ecount(1) CHwPipelineShader *pShader,
    __in_ecount(1) const MilColorF &materialColor,
    __inout_ecount(1) MILSPHandle &hCurrentRegister
    ) const
{ 
    HRESULT hr = S_OK;

    vector4 vec4CosHalfPhiAndCosDiff
    (
        m_flCosPhi,                // x
        m_flCosTheta - m_flCosPhi, // y
        0.0f,                      // z
        0.0f                       // w
    );

    // This sends diffuse, position, atten, and range
    IFC(CMILLightPoint::SendShaderData(pShader, materialColor, hCurrentRegister));

    // To send the direction, we can't call CMILLightDirectional::SendShaderData
    // because it will sends the color as well which we just did above.
    IFC(pShader->SetFloat3(
        hCurrentRegister, 
        static_cast<std::array<float,3>>(m_vec3InvDirectionViewSpace).data(),
        0.0
        ));

    hCurrentRegister += 
        GetShaderConstantRegisterSize(ShaderFunctionConstantData::Float3); 

    IFC(pShader->SetFloat4(
        hCurrentRegister, 
        static_cast<const std::array<float,4>>(vec4CosHalfPhiAndCosDiff).data()
        ));

    hCurrentRegister += 
        GetShaderConstantRegisterSize(ShaderFunctionConstantData::Float4);

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightSpot::Transform
//
//  Synopsis: Transforms the light by pTransform and flScale. See
//            CMILLightData::Transform for more information
//
//-----------------------------------------------------------------------------
override void 
CMILLightSpot::Transform(
    TransformType type,
    __in_ecount(1) const CMILMatrix *pTransform,
    float flScale
    )
{
    // Assert(flScale > 0) but let NaNs through
    Assert(!(flScale <= 0.0));

    // This transforms the direction
    CMILLightDirectional::Transform(type, pTransform, flScale);

#if DBG_ANALYSIS
    // The above transform call set this to true already which will
    // cause the second transform call to fail if we don't reset it.
    if (type == TransformType_ViewSpace)
    {
        m_fDbgHasBeenViewTransformed = false;
    }
#endif
    
    // This transforms the point, atten, and range
    CMILLightPoint::Transform(type, pTransform, flScale);

    // Spotlight angles are not affected by uniform scales, so we don't
    // transform them.
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightSpot::GetSpotlightFactor
//
//  Synopsis: Computes the spot light contribution based on D3D9's formula.
//            This will be faster if the falloff is 1.0.
//
//  Returns:  0.0f - 1.0f
//
//-----------------------------------------------------------------------------
float 
CMILLightSpot::GetSpotlightFactor(
    __in_ecount(1) vector3 const *vec3ToLight
    ) const
{        
    float flRho = vector3::dot_product(*vec3ToLight, m_vec3InvDirection);
    
    if (flRho > m_flCosTheta)
    {
        return 1.0f;
    }
    else if (flRho <= m_flCosPhi)
    {
        return 0.0f;
    }
    else
    {
        return (flRho - m_flCosPhi) / (m_flCosTheta - m_flCosPhi);
    }
}

//-----------------------------------------------------------------------------
//
//  Function: CMILLightSpot::IsSpot
//
//  Synopsis: Returns true if this light is a spot light
//
//  Returns:  true
//
//-----------------------------------------------------------------------------
bool
CMILLightSpot::IsSpot() const
{
    return true;
}



