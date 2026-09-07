// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_d3d
//      $Keywords:
//
//  $Description:
//      Structures for Shader Functions
//
//  $ENDTAG
//
//  Work Items:
//      - Break Shaders into separate vertex and pixel shaders
//
//------------------------------------------------------------------------------


//+-----------------------------------------------------------------------------
//
//  Structure:
//      ConstantString
//
//  Synopsis:
//      Structure that holds a static, constant string and it length (excluding
//      null termninator).
//
//------------------------------------------------------------------------------

struct _ConstantString
{
    __field_bcount(cb + sizeof(char)) PCSTR sz;
    UINT cb;

    _ConstantString(
        __in_bcount(_cb) PCSTR _sz,
        UINT _cb
        ) : sz(_sz), cb(_cb - sizeof(char))
    { }

    operator __outro_bcount(cb + sizeof(char)) PCSTR() const { return sz; }

};

typedef const _ConstantString ConstantString;


//+-----------------------------------------------------------------------------
//
//  Enumeration:
//      VertexFunctionParameterType
//
//  Synopsis:
//      Parameters that can be used for a vertex function.
//
//------------------------------------------------------------------------------

namespace VertexFunctionParameter
{
    enum Enum
    {
        Position,
        Diffuse,
        VertexUV2,
        FunctionConstData,
        ShaderOutputStruct,
        Interpolator_TexCoord1,
        Interpolator_TexCoord2,
        Interpolator_TexCoord4,
        Normal,
        WorldViewTransform,
        WorldViewProjTransform,
        WorldViewAdjTransTransform,
        SpecularPower,
        Total
    };
};

//+-----------------------------------------------------------------------------
//
//  Enumeration:
//      FunctionConstantDataType
//
//  Synopsis:
//      Data types that can be used in the shader functions const data.
//
//------------------------------------------------------------------------------

namespace ShaderFunctionConstantData
{
    enum Enum
    {
        Float,
        Float2,
        Float3,
        Float4,
        Matrix3x2,
        Matrix4x4,
        Total
    };
};

//+-----------------------------------------------------------------------------
//
//  Structure:
//      FunctionConstDataParameter
//
//  Synopsis:
//      A data parameter which will be stored in a data structure specific to
//      each function.
//
//------------------------------------------------------------------------------

struct FunctionConstDataParameter
{
    PCSTR pszParameterName;
    ShaderFunctionConstantData::Enum Type;
};

//+-----------------------------------------------------------------------------
//
//  Enumeration:
//      PixelFunctionParameterType
//
//  Synopsis:
//      Parameters that can be used for a pixel function.
//
//------------------------------------------------------------------------------

namespace PixelFunctionParameter
{
    enum Enum
    {
        Interpolator_TexCoord1,
        Interpolator_TexCoord2,
        Interpolator_TexCoord4,
        Sampler,
        FragmentConstData,
        ShaderOutputStruct,
        Total    
    };
};

//+-----------------------------------------------------------------------------
//
//  Structure:
//      VertexShaderFunction
//
//  Synopsis:
//      Data for the Vertex Function.  Includes name, parameters and constant
//      data.
//
//------------------------------------------------------------------------------

struct VertexShaderFunction
{
    //
    // Name
    //
    ConstantString pszParamsAndBody;

    //
    // Parameters to the function.
    //
    UINT const NumFunctionParameters;
    __ecount(NumFunctionParameters) VertexFunctionParameter::Enum const * const rgVertexFunctionParameter;

    //
    // Constant data parameters.
    //
    UINT const NumConstDataParameters;
    __ecount(NumConstDataParameters) FunctionConstDataParameter const * const rgConstDataParameter;

    const bool fLoopable;

    VertexShaderFunction(
        __in_bcount(_cbParamsAndBody) PCSTR _pszParamsAndBody,
        UINT _cbParamsAndBody,
        UINT _NumFunctionParameters,
        __in_ecount(_NumFunctionParameters) VertexFunctionParameter::Enum const *_rgVertexFunctionParameter,
        UINT _NumConstDataParameters,
        __in_ecount(_NumConstDataParameters) FunctionConstDataParameter const *_rgConstDataParameter,
        bool _fLoopable
        )
    : pszParamsAndBody(_pszParamsAndBody, _cbParamsAndBody),
      NumFunctionParameters(_NumFunctionParameters),
      rgVertexFunctionParameter(_rgVertexFunctionParameter),
      NumConstDataParameters(_NumConstDataParameters),
      rgConstDataParameter(_rgConstDataParameter),
      fLoopable(_fLoopable)
    {
    }
};


//+-----------------------------------------------------------------------------
//
//  Structure:
//      PixelShaderFunction
//
//  Synopsis:
//      Data for the Pixel Function.  Includes name, parameters and constant
//      data.
//
//------------------------------------------------------------------------------

struct PixelShaderFunction
{
    //
    // Parameters and implementation
    //
    ConstantString pszParamsAndBody;

    //
    // Parameters to the function.
    //
    UINT NumFunctionParameters;
    __ecount(NumFunctionParameters) PixelFunctionParameter::Enum const * const rgPixelFunctionParameter;

    //
    // Constant data parameters.
    //
    UINT NumConstDataParameters;
    __ecount(NumConstDataParameters) FunctionConstDataParameter const * const rgConstDataParameter;


    PixelShaderFunction(
        __in_bcount(_cbParamsAndBody) PCSTR _pszParamsAndBody,
        UINT _cbParamsAndBody,
        UINT _NumFunctionParameters,
        __in_ecount(_NumFunctionParameters) PixelFunctionParameter::Enum const *_rgPixelFunctionParameter,
        UINT _NumConstDataParameters,
        __in_ecount(_NumConstDataParameters) FunctionConstDataParameter const *_rgConstDataParameter
        )
    : pszParamsAndBody(_pszParamsAndBody, _cbParamsAndBody),
      NumFunctionParameters(_NumFunctionParameters),
      rgPixelFunctionParameter(_rgPixelFunctionParameter),
      NumConstDataParameters(_NumConstDataParameters),
      rgConstDataParameter(_rgConstDataParameter)
    {
    }
};

//+-----------------------------------------------------------------------------
//
//  Enumeration:
//      ShaderFragments
//
//  Synopsis:
//             All available shader functions.
//
//      *** MUST BE IN THE SAME ORDER AS THE FUNCTION ARRAY IN THE CPP ***
//
//------------------------------------------------------------------------------

namespace ShaderFunctions
{
    enum Enum
    {
        SystemVertexBuilderPassDiffuse, // AKA Null Function

        // Device Coordinate Only Functions
        Prepare2DTransform,

        // Pixel Functions
        MultiplyByInputDiffuse,
        MultiplyByInputDiffuse_NonPremultipledInput,
        MultiplyTexture_TransformFromVertexUV,
        MultiplyTexture_NoTransformFromTexCoord,
        MultiplyConstant,
        MultiplyAlpha,
        MultiplyAlpha_NonPremultiplied,
        MultiplyAlphaMask_TransformFromVertexUV,
        MultiplyAlphaMask_NoTransformFromTexCoord,
        MultiplyRadialGradientCentered,
        MultiplyRadialGradientNonCentered,

        // Vertex Only Functions
        Get3DTransforms,
        Prepare3DTransforms,
        CalcAmbientLighting,
        FlipNormal,
        CalcDiffuseDirectionalLighting,
        CalcDiffusePointLighting,
        CalcDiffuseSpotLighting,
        GetSpecularPower,
        CalcSpecularDirectionalLighting,
        CalcSpecularPointLighting,
        CalcSpecularSpotLighting,


        Total
    };
};


//+-----------------------------------------------------------------------------
//
//  Enumeration:
//      TransparencyEffect
//
//  Synopsis:
//      Describes whether the shader function has transparency in it, or whether
//      the transparency depends on the color source being used.
//
//------------------------------------------------------------------------------
namespace TransparencyEffect
{
    enum Enum
    {
        NoTransparency,
        BlendsColorSource,
        HasTransparency,
        Total
    };
};

//+-----------------------------------------------------------------------------
//
//  Structure:
//      ShaderFunction
//
//  Synopsis:
//      Contains the name of the function and references to a vertex and pixel
//      function.
//
//------------------------------------------------------------------------------

struct ShaderFunction
{
    const char *pszFunctionName;
    TransparencyEffect::Enum TransparencyEffect;

//    ConstantString pszFunctionName;

    __ecount(1) VertexShaderFunction const &VertexShader;
    __ecount(1) PixelShaderFunction  const &PixelShader;

    ShaderFunction(
        __in/*_bcount(cbFunctionName)*/ PCSTR _pszFunctionName,
//        UINT _cbFunctionName,
        TransparencyEffect::Enum _TransparencyEffect,
        __in_ecount(1) VertexShaderFunction const &_VertexShader,
        __in_ecount(1) PixelShaderFunction  const &_PixelShader
        )
    : pszFunctionName(_pszFunctionName),//, _cbFunctionName),
      TransparencyEffect(_TransparencyEffect),
      VertexShader(_VertexShader),
      PixelShader(_PixelShader)
    {
    }

};



extern ShaderFunction const *g_pHwHLSLShaderFunctions[ /*ShaderFunctions::Total*/ ];




//+-----------------------------------------------------------------------------
//
//  Trait Map:
//      ShaderConstantTraits<>
//
//  Synopsis:
//      Collection of ShaderConstant properties hashed by data type.
//
//  Traits:
//      RegisterSize: A value of 1 means 1 D3DShaderConstant Register, which is
//                    4 floats or 32 bytes.
//

template <ShaderFunctionConstantData::Enum>
struct ShaderConstantTraits;

template <>
struct ShaderConstantTraits<ShaderFunctionConstantData::Float>
{
    enum { RegisterSize = 1 };
};

template <>
struct ShaderConstantTraits<ShaderFunctionConstantData::Float2>
{
    enum { RegisterSize = 1 };
};

template <>
struct ShaderConstantTraits<ShaderFunctionConstantData::Float3>
{
    enum { RegisterSize = 1 };
};

template <>
struct ShaderConstantTraits<ShaderFunctionConstantData::Float4>
{
    enum { RegisterSize = 1 };
};

template <>
struct ShaderConstantTraits<ShaderFunctionConstantData::Matrix3x2>
{
    // Fills two constant registers
    enum { RegisterSize = 2 };
};

template <>
struct ShaderConstantTraits<ShaderFunctionConstantData::Matrix4x4>
{
    enum { RegisterSize = 4 };
};



//+-----------------------------------------------------------------------------
//
//  Function:
//      GetShaderConstantRegisterSize
//
//  Synopsis:
//      Gets the size of the shader constant data structure.  Size is based on
//      size of D3DShaderConstant Register, which is 4 floats. A value of 1
//      means 32 bytes.
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE
UINT 
GetShaderConstantRegisterSize(
    ShaderFunctionConstantData::Enum type
    )
{
    switch(type)
    {
    case ShaderFunctionConstantData::Float:
        return ShaderConstantTraits<ShaderFunctionConstantData::Float>::RegisterSize;

    case ShaderFunctionConstantData::Float2:
        return ShaderConstantTraits<ShaderFunctionConstantData::Float2>::RegisterSize;

    case ShaderFunctionConstantData::Float3:
        return ShaderConstantTraits<ShaderFunctionConstantData::Float3>::RegisterSize;
        
    case ShaderFunctionConstantData::Float4:
        return ShaderConstantTraits<ShaderFunctionConstantData::Float4>::RegisterSize;

    case ShaderFunctionConstantData::Matrix3x2:
        return ShaderConstantTraits<ShaderFunctionConstantData::Matrix3x2>::RegisterSize;

    case ShaderFunctionConstantData::Matrix4x4:
        return ShaderConstantTraits<ShaderFunctionConstantData::Matrix4x4>::RegisterSize;

    default:
        NO_DEFAULT("Unknown Function Constant Data Type");
    }
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      GetShaderConstantRegister
//
//  Synopsis:
//      Gets the ShaderConstant Register that the MILSPHandle refers to. We need
//      to do this because the D3D Constant registers are each 128 bytes, and we
//      want to try to track in 32 byte increments.
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE 
UINT 
GetShaderConstantRegister(
    MILSPHandle hParameter
    )
{
    return hParameter;
};

//+-----------------------------------------------------------------------------
//
//  Function:
//      IsVertexToPixelInterpolator
//
//  Synopsis:
//      Tells us if the vertex function parameter is passing data to the pixel
//      shader.
//

MIL_FORCEINLINE 
bool
IsVertexToPixelInterpolator(
    VertexFunctionParameter::Enum Type
    )
{
    switch(Type)
    {
    case VertexFunctionParameter::Interpolator_TexCoord1:
    case VertexFunctionParameter::Interpolator_TexCoord2:
    case VertexFunctionParameter::Interpolator_TexCoord4:
        return true;
        break;

    default:
        return false;
    }
}





