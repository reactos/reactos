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
//      Contains the implementation for ConvertHwShaderFragmentsToHLSL
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"


MtDefine(HLSLSource, MILRender, "HLSL text");

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwShaderFragmentToHLSLConverter
//
//  Synopsis:
//      Class which takes an array of HwPipelineItems and converts the
//      collection to high level shader language (HLSL) source.
//
//------------------------------------------------------------------------------

class CHwShaderFragmentToHLSLConverter
{
public:

    CHwShaderFragmentToHLSLConverter(
        __in_ecount(uNumItems) HwPipelineItem const * const rgPipelineItems,
        __range(1,MAX_SHADER_FRAGMENT+1) UINT uNumItems
        );

    ~CHwShaderFragmentToHLSLConverter();

    HRESULT GetHLSLSize(
        __deref_out_range(1,UINT_MAX) UINT &cbHLSL
        );

    HRESULT Convert(
        __out_bcount_part(cbHLSL,1) PSTR pHLSL,
        __range(1,UINT_MAX) UINT cbHLSL
        );

private:

    //+-------------------------------------------------------------------------
    //
    //  Function:
    //      AppendHLSLSegment
    //
    //  Synopsis:
    //      Base HLSL accumulator that operates in two modes.  First mode just
    //      accumulates required lengths.  Next mode assumes sufficient buffer
    //      has been allocated and appends segment to HLSL accumulated thus far.
    //
    //--------------------------------------------------------------------------

    void AppendHLSLSegment(
        __in_bcount(cbBuffer) VOID const *pBuffer,
        UINT cbBuffer
        );


    //+-------------------------------------------------------------------------
    //
    //  Function:
    //      AppendConstantString, AppendVariableString, AppendNumber
    //
    //  Synopsis:
    //      Helpers to call AppendHLSLSegment
    //
    //  Note:
    //      Try to avoid AppendVariableString because it walks string to compute
    //      length.
    //
    //--------------------------------------------------------------------------

    void AppendConstantString(
        __in_ecount(1) ConstantString &String
        );

    void AppendVariableString(
        __in PCSTR pString
        );

    void AppendNumber(
        __range(0,99) UINT uNum
        );


    void AccumulateHLSL(
        );


    void DeclareSamplers(
        );  

    void WriteVertexShaderDataStructures(
        );    

    void WritePixelShaderDataStructures(
        );    

    void WriteVertexShaderOutputStruct(
        );   

    void WriteVertexShaderFunctions(
        );

    void WriteVertexShader(
        );

    void WritePixelShaderFunctions(
        );

    void WritePixelShader(
        );



    void AppendInterpolatorName(
        VertexFunctionParameter::Enum InterpolatorType,
        __range(0,MAX_SHADER_INTERPOLATOR) UINT uCurrentInterpolator
        );

    void AppendInterpolatorDesc(
        VertexFunctionParameter::Enum InterpolatorType,
        __range(0,MAX_SHADER_INTERPOLATOR) UINT uCurrentNumberOfInterpolatorsThisType
        );


    void WriteSamplerName(
        __in PCSTR szFragmentName,
        __range(0,MAX_SHADER_SAMPLER) UINT uFragmentNum
        );

    void WriteFragmentName(
        __in PCSTR szFragmentName,
        __range(0,MAX_SHADER_FRAGMENT) UINT uFragmentNum
        );

    void WriteVertexFragmentConstDataType(
        __in PCSTR szFragmentName,
        __range(0,MAX_SHADER_FRAGMENT) UINT uFragmentNum
        );

    void WriteVertexFragmentConstDataName(
        __in PCSTR szFragmentName,
        __range(0,MAX_SHADER_FRAGMENT) UINT uFragmentNum
        );

    void WritePixelFragmentConstDataType(
        __in PCSTR szFragmentName,
        __range(0,MAX_SHADER_FRAGMENT) UINT uFragmentNum
        );

    void WritePixelFragmentConstDataName(
        __in PCSTR szFragmentName,
        __range(0,MAX_SHADER_FRAGMENT) UINT uFragmentNum
        );

    void WriteVertexShaderFragmentName(
        __in PCSTR szFragmentName,
        __range(0,MAX_SHADER_FRAGMENT) UINT uFragmentNum
        );

    void WritePixelShaderFragmentName(
        __in PCSTR szFragmentName,
        __range(0,MAX_SHADER_FRAGMENT) UINT uFragmentNum
        );
    
private:

    //
    // Shader items to build HLSL from
    //

    HwPipelineItem const * const m_rgPipelineItems;
    UINT const m_uNumPipelineItems;

    //
    // Tracks error during accumulation.  Possible erros are:
    //
    //  WGXERR_INSUFFICIENTBUFFER: If required buffer size ever overflowed
    //                             during size accumulation pass or if the
    //                             given buffer was insufficient during actual
    //                             HLSL accumulation.
    //
    //  WGXERR_TOOMANYSHADERELEMENTS: If limit was reached for some type of
    //                                element such as number of interpolators
    //                                or texture coordinates.

    HRESULT m_hrAccumulation;

    //
    // Buffer to accumulate HLSL into
    //

    PSTR m_pHLSLBuffer;

    //
    // Size of buffer needed/remaining
    //  First pass this accumulates size required.
    //  Second pass this tracks how much buffer remains.
    //

    UINT m_cbCurrentBuffer;
};


//+-----------------------------------------------------------------------------
//
//  Function:
//      ConvertHwShaderFragmentsToHLSL
//
//  Synopsis:
//      Convert and array of shader fragments into an HLSL shader source
//

__checkReturn HRESULT
ConvertHwShaderFragmentsToHLSL(
    __in_ecount(uNumPipelineItems) HwPipelineItem const * const rgPipelineItems,
    __range(1,MAX_SHADER_FRAGMENT) UINT uNumPipelineItems,
    __deref_outro_bcount(cbHLSL) PCSTR &pHLSLOut,
    __out_ecount(1) UINT &cbHLSL
    )
{
    HRESULT hr = S_OK;

    PSTR pHLSLSource = NULL;

    // Check for internal fragment limit
    if (uNumPipelineItems > MAX_SHADER_FRAGMENT)
    {
        IFC(WGXERR_TOOMANYSHADERELEMNTS);
    }

    //
    // Pass fragment array to converter and convert
    //

    {
        CHwShaderFragmentToHLSLConverter converter(
            rgPipelineItems,
            uNumPipelineItems
            );

        //
        // Call to find HSLS length
        //

        IFC(converter.GetHLSLSize(OUT cbHLSL));

        //
        // Allocate for actual HLSL accumulation
        //

        pHLSLSource =
            WPFAllocType(PSTR, ProcessHeap, Mt(HLSLSource), cbHLSL);
        IFCOOM(pHLSLSource);

        IFC(converter.Convert(pHLSLSource, cbHLSL));

        pHLSLOut = pHLSLSource;
        pHLSLSource = NULL;
    }

Cleanup:
    WPFFree(ProcessHeap, pHLSLSource);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderFragmentToHLSLConverter::CHwShaderFragmentToHLSLConverter
//
//  Synopsis:
//      ctor
//

CHwShaderFragmentToHLSLConverter::CHwShaderFragmentToHLSLConverter(
    __in_ecount(uNumItems) HwPipelineItem const * const rgPipelineItems,
    __range(0,99) UINT uNumItems
    ) :
    m_rgPipelineItems(rgPipelineItems),
    m_uNumPipelineItems(uNumItems)
{
    // The members are always initialized during GetHLSLSize and Convert
    // m_hrAccumulation = S_OK;
    // m_pHLSLBuffer = NULL;
    // m_cbCurrentBuffer = 0;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderFragmentToHLSLConverter::~CHwShaderFragmentToHLSLConverter
//
//  Synopsis:
//      dtor
//

CHwShaderFragmentToHLSLConverter::~CHwShaderFragmentToHLSLConverter()
{
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderFragmentToHLSLConverter::GetHLSLSize
//
//  Synopsis:
//      Convert current framents to HLSL, but just return it size.
//

HRESULT
CHwShaderFragmentToHLSLConverter::GetHLSLSize(
    __deref_out_range(1,UINT_MAX) UINT &cbHLSL
    )
{
    //
    // Setup accumulation pass
    //

    m_hrAccumulation = S_OK;
    m_pHLSLBuffer = NULL;

    //
    // This pass will just accumulate size.
    //
    // Start off with one element for NULL terminator
    //

    m_cbCurrentBuffer = sizeof(*m_pHLSLBuffer);

    //
    // Accumulate HLSL required size
    //

    AccumulateHLSL();

    //
    // Check accumulation results
    //

    HRESULT hr;

    IFC(m_hrAccumulation);

    // Return required size
    cbHLSL = m_cbCurrentBuffer;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderFragmentToHLSLConverter::Convert
//
//  Synopsis:
//      Convert current framents to HLSL and write results to given buffer.
//

HRESULT
CHwShaderFragmentToHLSLConverter::Convert(
    __out_bcount_part(cbHLSL,1) PSTR pHLSL,
    __range(1,UINT_MAX) UINT cbHLSL
    )
{
    //
    // Setup accumulation pass
    //

    m_hrAccumulation = S_OK;
    m_pHLSLBuffer = pHLSL;
    m_cbCurrentBuffer = cbHLSL;

    //
    // Accumulate HLSL
    //

    AccumulateHLSL();

    //
    // Check accumulation results
    //

    HRESULT hr;

    IFC(m_hrAccumulation);

    // m_pHLSLBuffer has advanced (during accumulation) to end of buffer. 
    // Null terminate it.
    m_pHLSLBuffer[0] = 0;

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderFragmentToHLSLConverter::AppendHLSLSegment
//
//  Synopsis:
//      Base HLSL accumulator that operates in two modes.  First mode just
//      accumulates required size.  Next mode checks for sufficient buffer and
//      appends segment to HLSL accumulated thus far.
//
//  Notes:
//      No error is returned.  Some caller must check m_hrAccumulation to
//      determine success.
//

void
CHwShaderFragmentToHLSLConverter::AppendHLSLSegment(
    __in_bcount(cbBuffer) const VOID *pBuffer,
    UINT cbBuffer
    )
{
    if (!m_pHLSLBuffer)
    {
        m_cbCurrentBuffer += cbBuffer;

        // Check for overflow
        if (m_cbCurrentBuffer < cbBuffer)
        {
            MIL_THRX(m_hrAccumulation, WGXERR_INSUFFICIENTBUFFER); 
        }
    }
    else
    {
        if (cbBuffer <= m_cbCurrentBuffer)
        {
            m_cbCurrentBuffer -= cbBuffer;
            RtlCopyMemory(m_pHLSLBuffer, pBuffer, cbBuffer);
            m_pHLSLBuffer = reinterpret_cast<PSTR>
                (reinterpret_cast<PBYTE>(m_pHLSLBuffer) + cbBuffer);
        }
        else
        {
            MIL_THRX(m_hrAccumulation, WGXERR_INSUFFICIENTBUFFER); 
        }
    }

    return;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderFragmentToHLSLConverter::AppendConstantString,
//      AppendVariableString
//
//  Synopsis:
//      Helpers to call AppendHLSLSegment
//
//  Note:
//      Try to avoid AppendVariableString because it walks string to compute
//      length.
//
//------------------------------------------------------------------------------

void
CHwShaderFragmentToHLSLConverter::AppendConstantString(
    __in_ecount(1) ConstantString &String
    )
{
    AppendHLSLSegment(String.sz, String.cb);
}


void
CHwShaderFragmentToHLSLConverter::AppendVariableString(
    __in PCSTR pString
    )
{
    UINT cbString = static_cast<UINT>(strlen(pString)*sizeof(*pString));
    AppendHLSLSegment(pString, cbString);
}


//+-----------------------------------------------------------------------------
//
//  Macro:
//      AppendString
//
//  Synopsis:
//      Helper to call AppendHLSLSegment with local string.  It assumes user is
//      a method within CHwShaderFragmentToHLSLConverter.
//
//------------------------------------------------------------------------------

#define AppendString(sz)                                        \
    do {                                                        \
        const char _sz[] = sz;                            \
        AppendHLSLSegment(_sz, sizeof(_sz)-sizeof(_sz[0]));     \
    } while ( UNCONDITIONAL_EXPR(false) )


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderFragmentToHLSLConverter::AppendNumber
//
//  Synopsis:
//      Helper to call AppendHLSLSegment with stringized number
//

void
CHwShaderFragmentToHLSLConverter::AppendNumber(
    __range(0,99) UINT uNum
    )
{
    const char rgDigit[] = "0123456789";

    Assert(uNum <= 99);

    if (uNum >= 10)
    {
        // Mod by 10 for safety
        UINT uDigit = (uNum / 10) % 10;
        AppendHLSLSegment(&rgDigit[uDigit], sizeof(rgDigit[0]));
    }

    AppendHLSLSegment(&rgDigit[uNum % 10], sizeof(rgDigit[0]));

    return;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderFragmentToHLSLConverter::AccumulateHLSL
//
//  Synopsis:
//      Accumulate HLSL segments based on the shader fragments.
//

void
CHwShaderFragmentToHLSLConverter::AccumulateHLSL(
    )
{
    AppendString(
        "//\n"
        "//\n"
        "\n"
        );

    DeclareSamplers();

    WriteVertexShaderDataStructures();

    WritePixelShaderDataStructures();

    WriteVertexShaderOutputStruct();

    WriteVertexShaderFunctions();

    WriteVertexShader();

    WritePixelShaderFunctions();

    WritePixelShader();

    AppendString(
        "//\n"
        "// Technique\n"
        "//\n"
        "\n"
        "technique T0\n"
        "{\n"
        "    pass P0\n"
        "    {\n"
        "        VertexShader = compile vs_2_0 VertexShaderImpl();\n"
        "        PixelShader  = compile ps_2_0 PixelShaderImpl();\n"
        "    }\n"
        "}\n"
        "\n"
        );


    AppendString(
        "//\n// End of Dynamic Shader Code\n//\n"
        );

    return;
}


void
CHwShaderFragmentToHLSLConverter::DeclareSamplers(
    )
{
    //
    // Declare samplers...
    //

    AppendString(
        "//\n"
        "// Samplers...\n"
        "//\n"
        "\n"
        );

    for (UINT uItem = 0; uItem < m_uNumPipelineItems; uItem++)
    {
        ShaderFunction const &fragment = *m_rgPipelineItems[uItem].pFragment;
        PixelShaderFunction const &pixelShader = fragment.PixelShader;

        bool fSamplersWritten = false;

        for (UINT uPSInput = 0; uPSInput < pixelShader.NumFunctionParameters; uPSInput++)
        {
            if (pixelShader.rgPixelFunctionParameter[uPSInput] == PixelFunctionParameter::Sampler)
            {
                AppendString(
                    "sampler "
                    );

                WriteSamplerName(
                    fragment.pszFunctionName,
                    uItem
                    );

                AppendString(
                    ";\n"
                    "\n"
                    );

                fSamplersWritten = true;
            }
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      SkipSameFragments
//
//  Synopsis:
//      Skips over consecutive fragments of the same type
//
//  Returns:
//      1) The next index value you should use for the loop
//      2) (opt) The number of same fragments
//
//------------------------------------------------------------------------------
static void
SkipSameFragments(
    __in_ecount(1) const UINT uCurrentFuncIndex,
    const UINT uFuncLoopMax,
    __in_pcount_in(uFuncLoopMax) HwPipelineItem const * const rgPipelineItems,
    __out_ecount(1) UINT *uNextDifferentFuncIndex,
    __out_ecount_opt(1) UINT *uNumCalls
    )
{
    Assert(uCurrentFuncIndex < uFuncLoopMax);

    // Loop until we get to the end or find a different func
    UINT i;
    for (i = uCurrentFuncIndex; i < uFuncLoopMax; i++)
    {
        if (rgPipelineItems[uCurrentFuncIndex].pFragment != rgPipelineItems[i].pFragment)
        {
            break;
        }
    }

    // i is guaranteed to be at least one since we start at
    // the current index so this subtraction is safe. We subtract
    // one because if there aren't any consecutive functions
    // we don't want to change the next function index
    *uNextDifferentFuncIndex = i - 1;

    if (uNumCalls)
    {
        // uCurrentFuncIndex <= i < uFuncLoopMax so this is safe
        *uNumCalls = i - uCurrentFuncIndex;       
    } 
}

void
CHwShaderFragmentToHLSLConverter::WriteVertexShaderDataStructures(
    )
{
    //
    // Declare VertexShader data structures
    //

    AppendString(
        "//\n"
        "// Vertex Fragment Data...\n"
        "//\n"
        "\n"
        );

    for (UINT uItem = 0; uItem < m_uNumPipelineItems; uItem++)
    {
        ShaderFunction const &fragment = *(m_rgPipelineItems[uItem].pFragment);
        VertexShaderFunction const &vertexShader = fragment.VertexShader;

        bool fInputDataFound = false;

        for (UINT uVSInput = 0; uVSInput < vertexShader.NumFunctionParameters; uVSInput++)
        {
            if (vertexShader.rgVertexFunctionParameter[uVSInput] == VertexFunctionParameter::FunctionConstData)
            {
                Assert(fInputDataFound == false);

                AppendString(
                    "struct "
                    );

                WriteVertexFragmentConstDataType(
                    fragment.pszFunctionName,
                    uItem
                    );

                AppendString(
                    "\n"
                    "{\n"
                    );


                for (UINT uDataNum = 0; uDataNum < vertexShader.NumConstDataParameters; uDataNum++)
                {
                    AppendString(
                        "    "
                        );

                    switch(vertexShader.rgConstDataParameter[uDataNum].Type)
                    {
                    case ShaderFunctionConstantData::Matrix4x4:
                        AppendString(
                            "float4x4 "
                            );
                        break;

                    case ShaderFunctionConstantData::Float3:
                        AppendString(
                            "float3 "
                            );
                        break;

                    case ShaderFunctionConstantData::Float4:
                        AppendString(
                            "float4 "
                            );
                        break;

                    case ShaderFunctionConstantData::Matrix3x2:
                        AppendString(
                            "float4x2 "
                            );
                        break;

                    default:
                        NO_DEFAULT("Unknown Const data type");
                    }

                    AppendVariableString(
                        vertexShader.rgConstDataParameter[uDataNum].pszParameterName
                        );

                    AppendString(
                        ";\n"
                        );
                }

                AppendString(
                    "};\n"
                    "\n"
                    );

                fInputDataFound = true;
            }
        }

        if (vertexShader.fLoopable)
        {
            SkipSameFragments(
                uItem,                  // in - current index
                m_uNumPipelineItems,      // in - loop max
                m_rgPipelineItems,     // in - fragment array
                &uItem,                 // out - next value for the loop index
                NULL                    // out_opt - number of calls
                );
        }
    }

    //
    // Output the Vertex Shader constant variable
    //

    AppendString(
        "//\n"
        "// Vertex Shader Constant Data\n"
        "//\n"
        "\n"
        );

    AppendString(
        "struct VertexShaderConstantData\n"
        "{\n"
        );


    for (UINT uItem = 0; uItem < m_uNumPipelineItems; uItem++)
    {
        ShaderFunction const &fragment = *(m_rgPipelineItems[uItem].pFragment);

        if (fragment.VertexShader.NumConstDataParameters > 0)
        {
            AppendString(
                "    "
                );
    
            WriteVertexFragmentConstDataType(
                fragment.pszFunctionName,
                uItem
                );
    
            AppendString(
                " "
                );
    
            WriteVertexFragmentConstDataName(
                fragment.pszFunctionName,
                uItem
                );

            if (fragment.VertexShader.fLoopable)
            {
                UINT uNumCalls;
                
                SkipSameFragments(
                    uItem,                  // in - current index
                    m_uNumPipelineItems,      // in - loop max
                    m_rgPipelineItems,     // in - fragment array
                    &uItem,                 // out - next value for the loop index
                    &uNumCalls              // out_opt - number of calls
                    );

                
                if (uNumCalls > 1)
                {
                    AppendString(
                        "["
                        );

                    AppendNumber(
                        uNumCalls
                        );

                    AppendString(
                        "]"
                        );
                }
            }
    
            AppendString(
                ";\n"
                );    
        }
    }

    AppendString(
        "};\n"
        "\n"
        );

    AppendString(
        "VertexShaderConstantData Data_VS;\n"
        "\n"
        );

    return;
}


void
CHwShaderFragmentToHLSLConverter::WritePixelShaderDataStructures(
    )
{
    bool fShaderConstDataFound = false;

    //
    // Declare PixelShader data structures
    //

    AppendString(
        "//\n"
        "// Pixel Fragment Data...\n"
        "//\n"
        "\n"
        );

    for (UINT uItem = 0; uItem < m_uNumPipelineItems; uItem++)
    {
        ShaderFunction const &fragment = *(m_rgPipelineItems[uItem].pFragment);
        PixelShaderFunction const &pixelShader = fragment.PixelShader;
#if DBG
        bool fDbgDataFound = false;
#endif 
        for (UINT uPSInput = 0; uPSInput < pixelShader.NumFunctionParameters; uPSInput++)
        {
            if (pixelShader.rgPixelFunctionParameter[uPSInput] == PixelFunctionParameter::FragmentConstData)
            {
                #if DBG
                Assert(fDbgDataFound == false);
                #endif

                AppendString(
                    "struct "
                    );

                WritePixelFragmentConstDataType(
                    fragment.pszFunctionName,
                    uItem
                    );

                AppendString(
                    "\n"
                    "{\n"
                    );



                for (UINT uDataNum = 0; uDataNum < pixelShader.NumConstDataParameters; uDataNum++)
                {
                    AppendString(
                        "    "
                        );

                    switch(pixelShader.rgConstDataParameter[uDataNum].Type)
                    {
                    case ShaderFunctionConstantData::Float:
                        AppendString(
                            "float "
                            );
                        break;

                    case ShaderFunctionConstantData::Float2:
                        AppendString(
                            "float2 "
                            );
                        break;

                    case ShaderFunctionConstantData::Matrix4x4:
                        AppendString(
                            "float4x4 "
                            );
                        break;

                    case ShaderFunctionConstantData::Float4:
                        AppendString(
                            "float4 "
                            );
                        break;

                    default:
                        NO_DEFAULT("Unknown Const data type");
                    }

                    AppendVariableString(
                        pixelShader.rgConstDataParameter[uDataNum].pszParameterName
                        );

                    AppendString(
                        ";\n"
                        );
                }

                AppendString(
                    "\n"
                    "};\n"
                    "\n"
                    );

#if DBG
                fDbgDataFound = true;
#endif
                fShaderConstDataFound = true;
            }
        }
    }

    //
    // Output the Vertex Shader constant variable
    //

    if (fShaderConstDataFound)
    {
        AppendString(
            "//\n"
            "// Pixel Shader Constant Data\n"
            "//\n"
            "\n"
            );
    
        AppendString(
            "struct PixelShaderConstantData\n"
            "{\n"
            );
    
    
        for (UINT uItem = 0; uItem < m_uNumPipelineItems; uItem++)
        {
            ShaderFunction const &fragment = *(m_rgPipelineItems[uItem].pFragment);
    
            if (fragment.PixelShader.NumConstDataParameters > 0)
            {
                AppendString(
                    "    "
                    );
        
                WritePixelFragmentConstDataType(
                    fragment.pszFunctionName,
                    uItem
                    );
        
                AppendString(
                    " "
                    );
        
                WritePixelFragmentConstDataName(
                    fragment.pszFunctionName,
                    uItem
                    );
        
                AppendString(
                    ";\n"
                    );    
            }
        }
    
        AppendString(
            "};\n"
            "\n"
            );
    
        AppendString(
            "PixelShaderConstantData Data_PS;\n"
            "\n"
            );
    }

    return;
}

void
CHwShaderFragmentToHLSLConverter::WriteVertexShaderOutputStruct(
    )
{
    AppendString(
        "\n"
        "struct VertexShaderOutput\n"
        "{\n"
        "    float4 Position : POSITION;\n"
        "    float4 Diffuse  : COLOR0;\n"
        );

    {
        UINT uNumTexCoords = 0;
        UINT uNumInterpolators = 0;

        for (UINT uItem = 0; uItem < m_uNumPipelineItems; uItem++)
        {
            ShaderFunction const &fragment = *(m_rgPipelineItems[uItem].pFragment);
            VertexShaderFunction const &vertexShader = fragment.VertexShader;

            for (UINT uInputNum = 0; uInputNum < vertexShader.NumFunctionParameters; uInputNum++)
            {
                if (IsVertexToPixelInterpolator(vertexShader.rgVertexFunctionParameter[uInputNum]))
                {
                    AppendString(
                        "    "
                        );
            
                    switch(vertexShader.rgVertexFunctionParameter[uInputNum])
                    {
                    case VertexFunctionParameter::Interpolator_TexCoord1:
                        AppendString(
                            "float  "
                            );
            
                        AppendInterpolatorName(
                            VertexFunctionParameter::Interpolator_TexCoord1,
                            uNumInterpolators
                            );
            
                        AppendString(
                            " : "
                            );
            
                        AppendInterpolatorDesc(
                            VertexFunctionParameter::Interpolator_TexCoord1,
                            uNumTexCoords
                            );
            
                        AppendString(
                            ";\n"
                            );
            
                        uNumTexCoords++;
            
                        break;                         

                    case VertexFunctionParameter::Interpolator_TexCoord2:
                        AppendString(
                            "float2 "
                            );
            
                        AppendInterpolatorName(
                            VertexFunctionParameter::Interpolator_TexCoord2,
                            uNumInterpolators
                            );
            
                        AppendString(
                            " : "
                            );
            
                        AppendInterpolatorDesc(
                            VertexFunctionParameter::Interpolator_TexCoord2,
                            uNumTexCoords
                            );
            
                        AppendString(
                            ";\n"
                            );
            
                        uNumTexCoords++;
            
                        break;

                    case VertexFunctionParameter::Interpolator_TexCoord4:
                        AppendString(
                            "float4 "
                            );

                        AppendInterpolatorName(
                            VertexFunctionParameter::Interpolator_TexCoord4,
                            uNumInterpolators);

                        AppendString(
                            " : "
                            );

                        AppendInterpolatorDesc(
                            VertexFunctionParameter::Interpolator_TexCoord4,
                            uNumTexCoords
                            );
            
                        AppendString(
                            ";\n"
                            );
            
                        uNumTexCoords++;

                        break;

                    default:
                        NO_DEFAULT("Error");
                    }

                    uNumInterpolators++;
                }
            }
        }
    }

    AppendString(
        "};\n"
        "\n"
        );

    return;
}

void
CHwShaderFragmentToHLSLConverter::WriteVertexShaderFunctions(
    )
{
    AppendString(
        "//\n"
        "// Fragment Vertex Shader functions...\n"
        "//\n"
        );


    for (UINT uItem = 0; uItem < m_uNumPipelineItems; uItem++)
    {
        ShaderFunction const &fragment = *(m_rgPipelineItems[uItem].pFragment);
        VertexShaderFunction const &vertexShader = fragment.VertexShader;

        if (vertexShader.pszParamsAndBody)
        {
            AppendString(
                "void\n"
                );

            WriteVertexShaderFragmentName(
                fragment.pszFunctionName,
                uItem
                );

            AppendConstantString(
                vertexShader.pszParamsAndBody
                );
            
            if (vertexShader.fLoopable)
            {
                SkipSameFragments(
                    uItem,                  // in - current index
                    m_uNumPipelineItems,      // in - loop max
                    m_rgPipelineItems,     // in - fragment array
                    &uItem,                 // out - next value for the loop index
                    NULL                    // out_opt - number of calls
                    );
            }
        }
    }

    return;
}

void
CHwShaderFragmentToHLSLConverter::WriteVertexShader(
    )
{
    UINT uInterpolatorNum = 0;

    //
    // Construct the Vertex Shader Code here
    //

    AppendString(
        "\n\n//\n"
        "// Main Vertex Shader\n"
        "//\n"
        "\n"
        "\n"
        "\n"
        "VertexShaderOutput\n"
        "VertexShaderImpl(\n"
        );

    //
    // Need to set whatever the geometry generator is sending...
    //

    AppendString(
        "    float4 Position : POSITION,\n"
        "    // Right now, only COLOR0 or NORMAL is used in a pass. The compiler\n"
        "    // optimizes away what's not used.\n"
        "    float4 Diffuse  : COLOR0,\n"
        "    float3 Normal   : NORMAL,\n"
        "    float2 UV_0     : TEXCOORD0,\n"
        "    float2 UV_1     : TEXCOORD1\n"
        "    )\n"
        "{\n"
        "    VertexShaderOutput Output = (VertexShaderOutput)0;\n"
        "\n"
        "    // These will be optimized away when not in use\n"
        "    float4x4 View, WorldView, WorldViewProj, WorldViewAdjTrans;\n"
        "    float    SpecularPower;\n"
        "\n"
        );

    //
    // Add calls to the vertex shader functions....
    //

    for (UINT uItem = 0; uItem < m_uNumPipelineItems; uItem++)
    {
        ShaderFunction const &fragment = *(m_rgPipelineItems[uItem].pFragment);
        VertexShaderFunction const &vertexShader = fragment.VertexShader;
        UINT uNextItem = uItem;

        if (vertexShader.pszParamsAndBody)
        {
            bool fParameterWritten = false;
            bool fLoopWritten = false;
            UINT uNumCalls = 0;

            if (fragment.VertexShader.fLoopable)
            {
                SkipSameFragments(
                    uItem,                  // in - current index
                    m_uNumPipelineItems,      // in - loop max
                    m_rgPipelineItems,     // in - fragment array
                    &uNextItem,             // out - next value for the loop index
                    &uNumCalls              // out_opt - number of calls
                    );

                if (uNumCalls > 1)
                {
                    AppendString(
                        "    for (int i = 0; i < "
                        );
                    
                    AppendNumber(
                        uNumCalls
                        );

                    AppendString(
                        "; ++i)\n    {\n    "
                        );

                    fLoopWritten = true;
                }
            }

            AppendString(
                "    "
                );

            WriteVertexShaderFragmentName(
                fragment.pszFunctionName,
                uItem
                );

            AppendString(
                "("
                );

            for (UINT uInputNum = 0; uInputNum < vertexShader.NumFunctionParameters; uInputNum++)
            {   
                if (fParameterWritten)
                {
                    AppendString(
                        ",\n"
                        );
                }
                else
                {
                    AppendString(
                        "\n"
                        );
                }

                AppendString(
                    "        "
                    );

                if (fLoopWritten)
                {
                    AppendString(
                        "    "
                        );
                }

                switch(vertexShader.rgVertexFunctionParameter[uInputNum])
                {
                case VertexFunctionParameter::ShaderOutputStruct:
                    AppendString(
                        "Output"
                        );
                    break;

                case VertexFunctionParameter::Position:
                    AppendString(
                        "Position"
                        );
                    break;

                case VertexFunctionParameter::Diffuse:
                    AppendString(
                        "Diffuse"
                        );
                    break;

                case VertexFunctionParameter::Normal:
                    AppendString(
                        "Normal"
                        );
                    break;

                case VertexFunctionParameter::VertexUV2:
                {
                    DWORD dwVertexTexCoordNum = 0;
                    VerifySUCCEEDED(CHwTexturedColorSource::MVFAttrToCoordIndex(
                                         m_rgPipelineItems[uItem].mvfaTextureCoordinates,
                                         &dwVertexTexCoordNum));
                    AppendInterpolatorName(
                        VertexFunctionParameter::Interpolator_TexCoord2,
                        dwVertexTexCoordNum
                        );
                    break;
                }
                
                case VertexFunctionParameter::FunctionConstData:
                    AppendString(
                        "Data_VS."
                        );

                    WriteVertexFragmentConstDataName(
                        fragment.pszFunctionName,
                        uItem
                        );

                    if (fLoopWritten)
                    {
                        AppendString(
                            "[i]"
                            );
                    }
                    
                    break;

                case VertexFunctionParameter::Interpolator_TexCoord1:
                    AppendString(
                        "Output."
                        );

                    AppendInterpolatorName(
                        VertexFunctionParameter::Interpolator_TexCoord1,
                        uInterpolatorNum
                        );

                    uInterpolatorNum++;
                    break;

                case VertexFunctionParameter::Interpolator_TexCoord2:
                    AppendString(
                        "Output."
                        );

                    AppendInterpolatorName(
                        VertexFunctionParameter::Interpolator_TexCoord2,
                        uInterpolatorNum
                        );

                    uInterpolatorNum++;
                    break;

                case VertexFunctionParameter::Interpolator_TexCoord4:
                    AppendString(
                        "Output."
                        );

                    AppendInterpolatorName(
                        VertexFunctionParameter::Interpolator_TexCoord4,
                        uInterpolatorNum
                        );

                    uInterpolatorNum++;
                    break;

                case VertexFunctionParameter::WorldViewTransform:
                    AppendString(
                        "WorldView"
                        );
                    break;

                case VertexFunctionParameter::WorldViewProjTransform:
                    AppendString(
                        "WorldViewProj"
                        );
                    break;

                case VertexFunctionParameter::WorldViewAdjTransTransform:
                    AppendString(
                        "WorldViewAdjTrans"
                        );
                    break;

                case VertexFunctionParameter::SpecularPower:
                    AppendString(
                        "SpecularPower"
                        );
                    break;

                default:
                    NO_DEFAULT("Unknown Vertex Shader Input Type");
                }

                fParameterWritten = true;
            }

            if (fLoopWritten)
            {
                AppendString(
                    "    "
                    );
            }

            AppendString(
                "\n"
                "        );\n"
                "\n"
                );

            if (fLoopWritten)
            {
                AppendString(
                    "    }\n"
                    );

                Assert(uNumCalls > 0);

                // Skip to the next different function
                uItem = uNextItem;
            }
        }
    }


    //
    // End the vertex shader
    //

    //
    // Remove clamp after drivers are fixed.
    //
    // As of nVidia driver 6.14.10.8715 from 2006/02/16 there is a bug in the 6000 series
    // of cards where the COLOR interpolators are not properly clamped.  This can cause
    // undesired artifacts including color saturation in 3D.
    //
    // This clamp should be compiled away in 2D scenarios and should be a neglible perf
    // impact in 3D.
    //

    AppendString(
        "    Output.Diffuse.rgb = min(Output.Diffuse.rgb, 1.0);\n"
        "\n"
        "    return Output;\n"
        "};\n"
        "\n"
        );

    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderFragmentToHLSLConverter::WritePixelShaderFunctions
//
//  Synopsis:
//

void
CHwShaderFragmentToHLSLConverter::WritePixelShaderFunctions(
    )
{
    AppendString(
        "//\n"
        "// Fragment Pixel Shader fragments...\n"
        "//\n"
        );


    for (UINT uItem = 0; uItem < m_uNumPipelineItems; uItem++)
    {
        ShaderFunction const &fragment = *(m_rgPipelineItems[uItem].pFragment);
        PixelShaderFunction const &pixelShader = fragment.PixelShader;

        if (pixelShader.pszParamsAndBody)
        {
            AppendString(
                "void\n"
                );

            WritePixelShaderFragmentName(
                fragment.pszFunctionName,
                uItem
                );

            AppendVariableString(
                pixelShader.pszParamsAndBody
                );
        }
    }

    return;
}

void
CHwShaderFragmentToHLSLConverter::WritePixelShader(
    )
{
    //
    // Construct the Pixel Shader Code here
    //

    AppendString(
        "\n"
        "\n"
        "//\n"
        "// Main Pixel Shader\n"
        "//\n"
        "\n"
        );

    AppendString(
        "\n"
        "float4\n"
        "PixelShaderImpl(\n"
        "    float4 Position : POSITION,\n"
        "    float4 Diffuse  : COLOR0"
        );

    {
        UINT uNumTexCoordInterpolators = 0;
        UINT uInterpolatorNum = 0;

        for (UINT uItem = 0; uItem < m_uNumPipelineItems; uItem++)
        {        
            ShaderFunction const &fragment = *(m_rgPipelineItems[uItem].pFragment);
            VertexShaderFunction const &vertexShader = fragment.VertexShader;
    
            for (UINT uInputNum = 0; uInputNum < vertexShader.NumFunctionParameters; uInputNum++)
            {
                if (IsVertexToPixelInterpolator(vertexShader.rgVertexFunctionParameter[uInputNum]))
                {
                    AppendString(
                        ",\n    "
                        );
            
                    switch(vertexShader.rgVertexFunctionParameter[uInputNum])
                    {

                    case VertexFunctionParameter::Interpolator_TexCoord1:
                        AppendString(
                            "float  "
                            );
    
                        AppendInterpolatorName(
                            VertexFunctionParameter::Interpolator_TexCoord1,
                            uInterpolatorNum
                            );
    
                        AppendString(
                            " : "
                            );
    
                        AppendInterpolatorDesc(
                            VertexFunctionParameter::Interpolator_TexCoord1,
                            uNumTexCoordInterpolators
                            );
    
                        uNumTexCoordInterpolators++;
                        break;

                    case VertexFunctionParameter::Interpolator_TexCoord2:
                        AppendString(
                            "float2 "
                            );
    
                        AppendInterpolatorName(
                            VertexFunctionParameter::Interpolator_TexCoord2,
                            uInterpolatorNum
                            );
    
                        AppendString(
                            " : "
                            );
    
                        AppendInterpolatorDesc(
                            VertexFunctionParameter::Interpolator_TexCoord2,
                            uNumTexCoordInterpolators
                            );
    
                        uNumTexCoordInterpolators++;
                        break;

                    case VertexFunctionParameter::Interpolator_TexCoord4:
                        AppendString(
                            "float4 "
                            );

                        AppendInterpolatorName(
                            VertexFunctionParameter::Interpolator_TexCoord4,
                            uInterpolatorNum
                            );

                        AppendString(
                            " : "
                            );

                        AppendInterpolatorDesc(
                            VertexFunctionParameter::Interpolator_TexCoord4,
                            uNumTexCoordInterpolators
                            );

                        uNumTexCoordInterpolators++;
                        break;
        
                    default:
                        NO_DEFAULT("Error");
                    }
    
                    uInterpolatorNum++;
                }
            }
        }
    }

    AppendString(
        "\n"
        "    ) : COLOR\n"
        "{\n"
        "    float4 curColor = Diffuse;\n\n"
        );

    {
        UINT uNumInterpolators = 0;

        for (UINT uItem = 0; uItem < m_uNumPipelineItems; uItem++)
        {
            ShaderFunction const &fragment = *(m_rgPipelineItems[uItem].pFragment);
            PixelShaderFunction const &pixelShader = fragment.PixelShader;

            bool fParameterAdded = false;

            if (pixelShader.pszParamsAndBody)
            {
                AppendString(
                    "    "
                    );
    
                WritePixelShaderFragmentName(
                    fragment.pszFunctionName,
                    uItem
                    );
    
                AppendString(
                    "(\n"
                    );
    
                for (UINT uInputNum = 0; uInputNum < pixelShader.NumFunctionParameters; uInputNum++)
                {
                    if (fParameterAdded)
                    {
                        AppendString(
                            ",\n        "
                            );
                    }
                    else
                    {
                        AppendString(
                            "        "
                            );
                    }
                    switch(pixelShader.rgPixelFunctionParameter[uInputNum])
                    {
                    case PixelFunctionParameter::Sampler:
                        WriteSamplerName(
                            fragment.pszFunctionName,
                            uItem);
                        break;
    
                    case PixelFunctionParameter::ShaderOutputStruct:
                        AppendString(
                            "curColor\n"
                            );
                        break;

                    case PixelFunctionParameter::Interpolator_TexCoord1:
                        AppendInterpolatorName(
                            VertexFunctionParameter::Interpolator_TexCoord1,
                            uNumInterpolators
                            );

                        uNumInterpolators++;
                        break;

                    case PixelFunctionParameter::Interpolator_TexCoord2:
                        AppendInterpolatorName(
                            VertexFunctionParameter::Interpolator_TexCoord2,
                            uNumInterpolators
                            );

                        uNumInterpolators++;
                        break;

                    case PixelFunctionParameter::Interpolator_TexCoord4:
                        AppendInterpolatorName(
                            VertexFunctionParameter::Interpolator_TexCoord4,
                            uNumInterpolators
                            );

                        uNumInterpolators++;
                        break;

                    case PixelFunctionParameter::FragmentConstData:
                        AppendString(
                            "Data_PS."
                            );

                        WritePixelFragmentConstDataName(
                            fragment.pszFunctionName,
                            uItem
                            );
                        break;

                    default:
                        NO_DEFAULT("Error - Unknown Pixel Shader Parameter Type");
                    }
    
                    fParameterAdded = true;
                }
    
                AppendString(
                    "        );\n\n"
                    );
            }
        }

    }

#if 1
    AppendString(
        "    return curColor;\n"
        );
#else

    AppendString(
        "    return float4(1.0f, 0.0f, 0.0f, 1.0f);\n"
        );
#endif

    AppendString(
        "};\n\n"
        );

    return;
}

void
CHwShaderFragmentToHLSLConverter::AppendInterpolatorName(
    VertexFunctionParameter::Enum InterpolatorType,
    __range(0,MAX_SHADER_INTERPOLATOR) UINT uCurrentInterpolator
    )
{
    if (uCurrentInterpolator > MAX_SHADER_INTERPOLATOR)
    {
        MIL_THRX(m_hrAccumulation, WGXERR_TOOMANYSHADERELEMNTS);
    }
    else
    {
        switch (InterpolatorType)
        {
        case VertexFunctionParameter::Interpolator_TexCoord1:
        case VertexFunctionParameter::Interpolator_TexCoord2:
        case VertexFunctionParameter::Interpolator_TexCoord4:
            AppendString(
                "UV_"
                );

            AppendNumber(
                uCurrentInterpolator
                );
            break;

        default:
            NO_DEFAULT("Unknown Vertex Interpolator Type");
        }
    }

    return;
}

void
CHwShaderFragmentToHLSLConverter::AppendInterpolatorDesc(
    VertexFunctionParameter::Enum InterpolatorType,
    __range(0,MAX_SHADER_INTERPOLATOR) UINT uCurrentOfInterpolatorsThisType
    )
{
    if (uCurrentOfInterpolatorsThisType > MAX_SHADER_INTERPOLATOR)
    {
        MIL_THRX(m_hrAccumulation, WGXERR_TOOMANYSHADERELEMNTS);
    }
    else
    {
        switch (InterpolatorType)
        {
        case VertexFunctionParameter::Interpolator_TexCoord1:
        case VertexFunctionParameter::Interpolator_TexCoord2:
        case VertexFunctionParameter::Interpolator_TexCoord4:
            AppendString(
                "TEXCOORD"
                );

            AppendNumber(
                uCurrentOfInterpolatorsThisType
                );
            break;

        default:
            NO_DEFAULT("Unknown Vertex Interpolator Type");
        }
    }

    return;
}

void
CHwShaderFragmentToHLSLConverter::WriteSamplerName(
    __in PCSTR szFragmentName,
    __range(0,MAX_SHADER_SAMPLER) UINT uFragmentNum
    )
{
    //
    // Currently we have a 1 to 0,1 mapping between fragments and samplers.
    // No fragment can have more than 1 sampler.  This will probably have
    // to change in the future.
    // 
    WriteFragmentName(
        szFragmentName,
        uFragmentNum
        );

    AppendString(
        "_Sampler"
        );

    return;
}


void
CHwShaderFragmentToHLSLConverter::WriteFragmentName(
    __in PCSTR szFragmentName,
    __range(0,MAX_SHADER_FRAGMENT) UINT uFragmentNum
    )
{
    AppendVariableString(
        szFragmentName
        );

    AppendNumber(
        uFragmentNum
        );

    return;
}

void
CHwShaderFragmentToHLSLConverter::WriteVertexFragmentConstDataType(
    __in PCSTR szFragmentName,
    __range(0,MAX_SHADER_FRAGMENT) UINT uFragmentNum
    )
{
    AppendVariableString(
        szFragmentName
        );

    AppendString(
        "_VS_ConstData"
        );

    return;
}

void
CHwShaderFragmentToHLSLConverter::WriteVertexFragmentConstDataName(
    __in PCSTR szFragmentName,
    __range(0,MAX_SHADER_FRAGMENT) UINT uFragmentNum
    )
{
    WriteVertexShaderFragmentName(
        szFragmentName,
        uFragmentNum
        );

    AppendString(
        "_ConstantTable"
        );

    return;
}

void
CHwShaderFragmentToHLSLConverter::WritePixelFragmentConstDataType(
    __in PCSTR szFragmentName,
    __range(0,MAX_SHADER_FRAGMENT) UINT uFragmentNum
    )
{
    AppendVariableString(
        szFragmentName
        );

    AppendString(
        "_PS_ConstData"
        );

    return;
}


void
CHwShaderFragmentToHLSLConverter::WritePixelFragmentConstDataName(
    __in PCSTR szFragmentName,
    __range(0,MAX_SHADER_FRAGMENT) UINT uFragmentNum
    )
{
    WritePixelShaderFragmentName(
        szFragmentName,
        uFragmentNum
        );

    AppendString(
        "_ConstantTable"
        );

    return;
}

void
CHwShaderFragmentToHLSLConverter::WriteVertexShaderFragmentName(
    __in PCSTR szFragmentName,
    __range(0,MAX_SHADER_FRAGMENT) UINT uFragmentNum
    )
{
    AppendVariableString(
        szFragmentName
        );

    AppendString(
        "_VS"
        );

    AppendNumber(
        uFragmentNum
        );

    return;
}

void
CHwShaderFragmentToHLSLConverter::WritePixelShaderFragmentName(
    __in PCSTR szFragmentName,
    __range(0,MAX_SHADER_FRAGMENT) UINT uFragmentNum
    )
{
    AppendVariableString(
        szFragmentName
        );

    AppendString(
        "_PS"
        );

    AppendNumber(
        uFragmentNum
        );

    return;
}




