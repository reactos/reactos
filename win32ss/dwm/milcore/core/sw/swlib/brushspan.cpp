// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_brush
//      $Keywords:
//
//  $Description:
//      Color sources which generate colors for various brush types. "Span" is
//      obsolete - these classes don't actually handle spans.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CConstantColorBrushSpan, MILRender, "CConstantColorBrushSpan");
MtDefine(CLinearGradientBrushSpan, MILRender, "CLinearGradientBrushSpan");
MtDefine(CLinearGradientBrushSpan_MMX, MILRender, "CLinearGradientBrushSpan_MMX");
MtDefine(CRadialGradientBrushSpan, MILRender, "CRadialGradientBrushSpan");
MtDefine(CFocalGradientBrushSpan, MILRender, "CFocalGradientBrushSpan");
MtDefine(CShaderEffectBrushSpan, MILRender, "CShaderEffectBrushSpan");

// # of fractional bits that we iterate across the texture with:

#define ONEDNUMFRACTIONALBITS 16

// Get the integer portion of our fixed point texture coordinate, using
// a floor function:

#define ONEDGETINTEGERBITS(x) ((x) >> ONEDNUMFRACTIONALBITS)

// Get the 8-bit fractional portion of our fixed point texture coordinate.
// We could round, but I can't be bothered:

#define ONEDGETFRACTIONAL8BITS(x) (((x) >> (ONEDNUMFRACTIONALBITS - 8)) & 0xff)

//
// sRGB color space spans.
//

//
// Implementation of the constant-color span class
//

CConstantColorBrushSpan::CConstantColorBrushSpan()
{
}

VOID 
FASTCALL ColorSource_Constant_32bppPARGB(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    const CConstantColorBrushSpan *pColorSource = DYNCAST(CConstantColorBrushSpan, pSOP->m_posd);
    Assert(pColorSource);

    FillMemoryInt32(pSOP->m_pvDest, pPP->m_uiCount, pColorSource->m_Color);
}

HRESULT 
CConstantColorBrushSpan::Initialize(
    __in_ecount(1) const MilColorF *pColor
    )
{
    m_Color = Premultiply(Convert_MilColorF_scRGB_To_MilColorB_sRGB(pColor));
    RRETURN(S_OK);
}

//
// Implementation of the gradient span classes
//

HRESULT
CGradientBrushSpan::InitializeTexture(
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
    __in_ecount(3) const MilPoint2F *pGradientPoints,
    __in BOOL fRadialGradient,
    __in_ecount(uCount) const MilColorF *pColors,
    __in_ecount(uCount) const FLOAT *pPositions,
    __in UINT uCount,
    __in MilGradientWrapMode::Enum wrapMode,
    __in MilColorInterpolationMode::Enum colorInterpolationMode,
    __out_ecount(1) CMILMatrix *pmatDeviceIPCtoGradientTextureHPC 
    )
{
    HRESULT hr = S_OK;

    CGradientSpanInfo gradientSpanInfo;

    // Save wrapMode in member variable for future reference
    m_wrapMode = wrapMode;

    //
    // In 2D the Sample Space is equivalent to the Device Space.
    //

    // Determine the number of texels required for the gradient texture
    // and get texture mapping matrix
    MIL_THR(CGradientTextureGenerator::CalculateTextureSizeAndMapping(
            pGradientPoints,
            pGradientPoints+1,
            pGradientPoints+2,
            pmatWorldHPCToDeviceHPC, // pmatWorldToSampleSpace
            fRadialGradient,
            wrapMode, 
            FALSE, // Don't normalize matrix for SW implementation
            &gradientSpanInfo,
            pmatDeviceIPCtoGradientTextureHPC // pmatSampleSpaceToTextureMaybeNormalized
            ));

    m_uTexelCount = gradientSpanInfo.GetTexelCount();
    m_flGradientSpanEnd = gradientSpanInfo.GetSpanEndTextureSpace();

    // NOTE created 2005/06/07-MikhailL
    // NOTE modified 2005/07/06-milesc
    //
    //  The following comment is to explain the half pixel fixup below.
    // 
    // The gradient-texture implementation needs to convert location of a pixel
    // indexed by integers (x,y) to corresponding location in a texture.
    //
    // The matrix returned by CalculateTextureSizeAndMapping takes points in HPC
    // device space and transforms them into HPC "texture" space. Note that this
    // is not the final texture space, as radial gradients still do more calculations
    // in this space.
    //
    // In order to use this matrix, we must first transform our points into HPC space
    // since they are given in IPC space. We do this by modifying the matrix below.
    // The modification is equal to TranslationMatrix(+0.5, +0.5) * pmatDeviceIPCtoGradientTextureHPC
    //
    // Note that this is the first step in CMILMatrix::AdjustForIPC
    // Note continued below...
    pmatDeviceIPCtoGradientTextureHPC->_41 += (pmatDeviceIPCtoGradientTextureHPC->_11 + pmatDeviceIPCtoGradientTextureHPC->_21)*0.5f;
    pmatDeviceIPCtoGradientTextureHPC->_42 += (pmatDeviceIPCtoGradientTextureHPC->_12 + pmatDeviceIPCtoGradientTextureHPC->_22)*0.5f;
    // ... note continuing
    //
    // With this matrix now adjusted, out IPC device points that are transformed by it become HPC "texture" space points.
    // We are not done yet though. The gradient code will expect IPC points in the end to do the texture lookup. However, we cannot
    // adjust the matrix to do this final texture space IPC -> HPC trnasformation because we are not really in texture space here.
    //
    // For example, for the radial gradient code we are in unit circle space. We don't get to real texture space until after the
    // distance operation. The final IPC -> HPC transformation must therefore be done later, by the individual gradient color sources.

    // The number of texels has to be a power of two:
    Assert((m_uTexelCount & (m_uTexelCount - 1)) == 0);    

    // Guard that the computed texel count isn't greater than
    // the allocated size.
    Assert(m_uTexelCount <= MAX_GRADIENTTEXEL_COUNT);

    m_uTexelCountMinusOne = m_uTexelCount - 1;
    
    // Generate the gradient texture
    if (SUCCEEDED(hr))
    {
        C_ASSERT(ARRAYSIZE(m_rgStartTexelAgrb) == ARRAYSIZE(m_rgEndTexelAgrb));

        hr = THR(CGradientTextureGenerator::GenerateGradientTexture(
            pColors,
            pPositions,
            uCount,
            fRadialGradient,
            wrapMode,
            colorInterpolationMode,
            &gradientSpanInfo,
            ARRAYSIZE(m_rgStartTexelAgrb),
            m_rgStartTexelAgrb));

            UINT uTexelCount = gradientSpanInfo.GetTexelCount();
            Assert(uTexelCount <= ARRAYSIZE(m_rgStartTexelAgrb));

            // Fill shifted buffer with elements of the texel buffer shifted
            // down one element
            for (UINT i = 0; i < (uTexelCount - 1); i++)
            {
                m_rgEndTexelAgrb[i] = m_rgStartTexelAgrb[i+1];
            }  

            // Start texel in original buffer wraps to the end.
            m_rgEndTexelAgrb[uTexelCount-1] = m_rgStartTexelAgrb[0];
    }    
    
    RRETURN(hr);
}

CLinearGradientBrushSpan::CLinearGradientBrushSpan()
    : CGradientBrushSpan()
{
}

CLinearGradientBrushSpan::~CLinearGradientBrushSpan()
{
}

HRESULT 
CLinearGradientBrushSpan::Initialize(
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
    __in_ecount(3) const MilPoint2F *pGradientPoints,
    __in_ecount(uCount) const MilColorF *pColors,
    __in_ecount(uCount) const FLOAT *pPositions,
    __in UINT uCount,
    __in MilGradientWrapMode::Enum wrapMode,
    __in MilColorInterpolationMode::Enum colorInterpolationMode
    )
{
    HRESULT hr = S_OK;

    CMILMatrix matDeviceIPCtoNormBrushHPC;
           
    if (SUCCEEDED(hr))
    {
        MIL_THR(InitializeTexture(
                pmatWorldHPCToDeviceHPC,
                pGradientPoints,
                FALSE,  // Not a radial gradient
                pColors,
                pPositions,
                uCount,
                wrapMode,
                colorInterpolationMode,
                &matDeviceIPCtoNormBrushHPC
                ));
    }

    if (SUCCEEDED(hr))
    {    
        // Convert the transform to fixed point texture units.  Since the
        // texture space is equal to the normalized bursh space all that is
        // needed is the fixed point conversion.

        m_nM11 = MatrixValueToFix16(matDeviceIPCtoNormBrushHPC.GetM11());
        m_nM21 = MatrixValueToFix16(matDeviceIPCtoNormBrushHPC.GetM21());

        // our matrix transform points to textureHPC space, so the - 0.5
        // transforms them to IPC space
        m_nDx  = MatrixValueToFix16(matDeviceIPCtoNormBrushHPC.GetDx() - 0.5f);

        // For every pixel that we step one to the right in device space,
        // we need to know the corresponding x-increment in texture (err,
        // I mean gradient) space.  Take a (1, 0) device vector, pop-it
        // through the device-to-normalized transform, and you get this
        // as the xIncrement result:

        m_nXIncrement = m_nM11;
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLinearGradientBrushSpan::MatrixValueToFix16
//
//  Synopsis:
//      Converts a matrix value from FLOAT to Fix16. Depending on wrap mode,
//      this will do the conversion differently for values which exceed Fix16
//      range.
//

INT
CLinearGradientBrushSpan::MatrixValueToFix16(FLOAT value)
{
    INT fix16Value;
    if (m_wrapMode == MilGradientWrapMode::Extend)
    {
        //
        // We shouldn't really need to worry about range issues here as that
        // should be handled before we even start sampling. It shouldn't hurt
        // though, so we do it just in case.
        //
        fix16Value = CFloatFPU::RoundSat(value * FIX16_ONE);
    }
    else
    {
        //
        // The linear gradient sampler does not need to concern it with large
        // values. All the calculations are mathematically equivalent when we
        // add or subtract multiples of the texel count.
        //
        FLOAT moduloValue = fmodf(value, static_cast<float>(m_uTexelCount));
        fix16Value = CFloatFPU::Round(moduloValue * FIX16_ONE);
    }

    return fix16Value;
}


VOID 
FASTCALL ColorSource_LinearGradient_32bppPARGB(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    CLinearGradientBrushSpan *pColorSource = 
        DYNCAST(CLinearGradientBrushSpan, pSOP->m_posd);
    Assert(pColorSource);

    pColorSource->GenerateColors(
        pPP->m_iX, 
        pPP->m_iY, 
        pPP->m_uiCount, 
        (ARGB*) pSOP->m_pvDest
        );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLinearGradientBrushSpan::ReleaseExpensiveResources, CColorSource
//
//  Synopsis:
//      Release expensive resources
//
//------------------------------------------------------------------------------

VOID 
CLinearGradientBrushSpan::ReleaseExpensiveResources()
{
   // This class doesn't hold onto resources that need to be released
}

void 
CLinearGradientBrushSpan::GenerateColors(
    __in INT nX, 
    __in INT nY, 
    __in INT nCount, 
    __out_ecount_full(nCount) ARGB *pArgbDest
    )
{
    // Copy some class stuff to local variables for faster access in
    // our inner loop:

    const AGRB64TEXEL *pStartTexels = &m_rgStartTexelAgrb[0];
    const AGRB64TEXEL *pEndTexels = &m_rgEndTexelAgrb[0];
    
    const AGRB64TEXEL *pStartTexel;
    const AGRB64TEXEL *pEndTexel;
    
    UINT uWeightA = 256;
    UINT uWeightB = 0;

    // Given our start point in device space, figure out the corresponding 
    // texture pixel.  Note that this is expressed as a fixed-point number 
    // with FRACTIONBITS bits of fractional precision:

    INT nTexturePositionIPC;
    INT nXIncrement;

    GenerateColorsInit(
        nX,
        nY,
        nCount,
        &nTexturePositionIPC,
        &nXIncrement
        );

    BOOL extendMode = (m_wrapMode == MilGradientWrapMode::Extend);
    INT nTexelCountMinusOne = m_uTexelCountMinusOne;

    do {
        // We want to linearly interpolate between two pixels,
        // A and B (where A is the floor pixel, B the ceiling pixel).
        // 'uWeightA' is the fraction of pixel A that we want, and
        // 'uWeightB' is the fraction of pixel B that we want:

        uWeightB = ONEDGETFRACTIONAL8BITS(nTexturePositionIPC);   
        uWeightA = 256 - uWeightB;

        // We could actually do a big lookup table right off of 'nTexturePosition'
        // for however many bits of precision we wanted to do.  But that
        // would be too much work in the setup.

        // __bound indicates that this value is carefully bounds checked.  In this case
        // nTextureIndex is bounded at 0..nTexelCountMinusOne explicitly by the first two cases
        // and in a trickier manner by the &= operator in the last else clause below.
        __bound INT nTextureIndex = ONEDGETINTEGERBITS(nTexturePositionIPC);

        if (extendMode)
        {
            if (nTextureIndex < 0)
            {
                nTextureIndex = 0;
                uWeightA = 256;
                uWeightB = 0;
            }
            else if (nTextureIndex >= nTexelCountMinusOne)
            {
                nTextureIndex = nTexelCountMinusOne;
                uWeightA = 256;
                uWeightB = 0;
            }
        }
        else
        {
            // Because the texel count is a power of 2, we can accomplish a mod (%)
            // operation using a bitwise &.  That is:
            // 'value % NumberOfTexels' == 'value & NumberOfTexelsMinusOne'            
            nTextureIndex &= nTexelCountMinusOne;
        }

        pStartTexel = &pStartTexels[nTextureIndex];
        pEndTexel = &pEndTexels[nTextureIndex];

        // Note that we can gamma correct the texels so that we don't
        // have to do gamma correction here.  The addition of constants
        // here are to accomplish rounding:

        UINT rrrrbbbb = (pStartTexel->A00rr00bb * uWeightA) 
                        + (pEndTexel->A00rr00bb * uWeightB)
                        + 0x00800080;

        UINT aaaagggg = (pStartTexel->A00aa00gg * uWeightA)
                        + (pEndTexel->A00aa00gg * uWeightB)
                        + 0x00800080;

        *pArgbDest++ = (aaaagggg & 0xff00ff00) + ((rrrrbbbb & 0xff00ff00) >> 8);
        nTexturePositionIPC += nXIncrement;
    } while (--nCount != 0);
}

CLinearGradientBrushSpan_MMX::CLinearGradientBrushSpan_MMX()
    : CLinearGradientBrushSpan()
{
}

HRESULT 
CLinearGradientBrushSpan_MMX::Initialize(
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
    __in_ecount(3) const MilPoint2F *pGradientPoints,
    __in_ecount(uCount) const MilColorF *pColors,
    __in_ecount(uCount) const FLOAT *pPositions,
    __in UINT uCount,
    __in MilGradientWrapMode::Enum wrapMode,
    __in MilColorInterpolationMode::Enum colorInterpolationMode
    )
{
    HRESULT hr = S_OK;

    MIL_THR(CLinearGradientBrushSpan::Initialize(
        pmatWorldHPCToDeviceHPC,
        pGradientPoints,
        pColors,
        pPositions,
        uCount,
        wrapMode, 
        colorInterpolationMode
        ));

#if defined(_X86_)
    if (SUCCEEDED(hr))
    {
        UINT uTexelCount = m_uTexelCount;
        ULONGLONG *pStartTexelArgb = &m_rgStartTexelArgb[0];
        ULONGLONG *pEndTexelArgb = &m_rgEndTexelArgb[0];
        static const ULONGLONG u64OneHalf8dot8 = 0x0080008000800080;

        // The C constructor creates the colors in AGRB order, but we
        // want them in ARGB order, so swap R and G for every pixel:

        USHORT *p = reinterpret_cast<USHORT*>(pStartTexelArgb);
        for (UINT i = 0; i < uTexelCount; i++, p += 4)
        {
            USHORT tmp = *(p + 1);
            *(p + 1) = *(p + 2);
            *(p + 2) = tmp;
        }

        p = reinterpret_cast<USHORT*>(pEndTexelArgb);
        for (UINT i = 0; i < uTexelCount; i++, p += 4)
        {
            USHORT tmp = *(p + 1);
            *(p + 1) = *(p + 2);
            *(p + 2) = tmp;
        }

        // Make some more adjustments for our MMX routine:
        //
        //     m_rgEndTexelArgb[i] -= m_rgStartTexelArgb[i]
        //     m_rgStartTexelArgb[i] = 256 * m_rgStartTexelArgb[i] + OneHalf

        _asm
        {
            mov     ecx, uTexelCount
            mov     esi, pStartTexelArgb
            mov     edi, pEndTexelArgb

        MoreTexels:
            movq    mm0, [esi]
            movq    mm1, [edi]
            psubw   mm1, mm0
            psllw   mm0, 8
            paddw   mm0, u64OneHalf8dot8
            movq    [esi], mm0
            movq    [edi], mm1
            add     esi, 8
            add     edi, 8
            dec     ecx
            jnz     MoreTexels

            emms
        }
    }
#endif

    RRETURN(hr);
}

VOID 
FASTCALL ColorSource_LinearGradient_32bppPARGB_MMX(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    CLinearGradientBrushSpan_MMX *pColorSource = 
        DYNCAST(CLinearGradientBrushSpan_MMX, pSOP->m_posd);
    Assert(pColorSource);

    pColorSource->GenerateColors(
        pPP->m_iX, 
        pPP->m_iY, 
        pPP->m_uiCount, 
        (ARGB*) pSOP->m_pvDest
        );
}

VOID 
CLinearGradientBrushSpan_MMX::GenerateColors(
    __in INT nX, 
    __in INT nY, 
    __in INT nCount, 
    __out_ecount_full(nCount) ARGB *pArgbDest
    )
{
#if defined(_X86_)

    // Copy some class stuff to local variables for faster access in
    // our inner loop:

    AGRB64TEXEL *pStartTexels = &m_rgStartTexelAgrb[0];
    AGRB64TEXEL *pEndTexels = &m_rgEndTexelAgrb[0];

    // Given our start point in device space, figure out the corresponding 
    // texture pixel.  Note that this is expressed as a fixed-point number 
    // with FRACTIONBITS bits of fractional precision:

    INT nTexturePositionIPC;
    INT nXIncrement;

    GenerateColorsInit(
        nX,
        nY,
        nCount,
        &nTexturePositionIPC,
        &nXIncrement
        );

    BOOL extendMode = (m_wrapMode == MilGradientWrapMode::Extend);
    UINT uTexelCountMinusOne = m_uTexelCountMinusOne;

    // Because the texel count is a power of 2, we can accomplish a mod (%) operation 
    // using a bitwise &.  That is 'value % NumberOfTexels' == 'value & NumberOfTexelsMinusOne'    
    UINT uIntervalMask = m_uTexelCountMinusOne;

    // Prepare for the three stages:
    // stage1: QWORD align the destination
    // stage2: process 2 pixels at a time
    // stage3: process the last pixel if present

    UINT stage1_count = 0, stage2_count = 0, stage3_count = 0;
    if (nCount > 0)
    {
        // If destination is not QWORD aligned, process the first pixel
        // in stage 1.

        if (((UINT_PTR) pArgbDest) & 0x4)
        {
            stage1_count = 1;
            nCount--;
        }

        stage2_count = nCount >> 1;
        stage3_count = nCount - 2 * stage2_count;

        _asm 
        {
            // eax = pointer to interval-start array
            // ebx = pointer to interval-end array
            // ecx = shift count
            // edx = scratch
            // esi = count
            // edi = destination
            // mm0 = interval counter
            // mm1 = interval incrementer
            // mm2 = fractional counter
            // mm3 = fractional incrementer
            // mm4 = temp
            // mm5 = temp

            dec         stage1_count

            mov         eax, pStartTexels
            mov         ebx, pEndTexels
            mov         ecx, 16             
            mov         esi, stage2_count
            mov         edi, pArgbDest
            movd        mm0, nTexturePositionIPC
            movd        mm1, nXIncrement

            movd        mm2, nTexturePositionIPC           // 0 | 0 | 0 | 0 || x | x | mult | lo
            movd        mm3, nXIncrement
            punpcklwd   mm2, mm2                // 0 | x | 0 | x || mult | lo | mult | lo
            punpcklwd   mm3, mm3
            punpckldq   mm2, mm2                // mult | lo | mult | lo || mult | lo | mult | lo
            punpckldq   mm3, mm3

            // This preparation normally happens inside the loop:

            movq        mm4, mm2                // mult | x | mult | x || mult | x | mult | x
            movd        edx, mm0

            jnz         pre_stage2_loop         // the flags for this are set in the "dec stage1_count" above

// stage1_loop:
  
            psrlw       mm4, 8                  // 0 | mult | 0 | mult || 0 | mult | 0 | mult
            sar         edx, cl

            cmp         extendMode, 0           // apply wrap mode to the pixel if needed
            jnz         stage1_extend_left
            and         edx, uIntervalMask
            jmp         stage1_after_extend

stage1_extend_left:

            cmp         edx, 0
            jge         stage1_extend_right
            mov         edx, 0
            pxor        mm4, mm4

stage1_extend_right:

            cmp         edx, uTexelCountMinusOne
            jl          stage1_after_extend
            mov         edx, uTexelCountMinusOne
            pxor        mm4, mm4

stage1_after_extend:

            pmullw      mm4, [ebx + edx*8]
            paddd       mm0, mm1                // interval counter += interval increment

            add         edi, 4                  // pArgbDest++

            paddw       mm4, [eax + edx*8]
            movd        edx, mm0                // Prepare for next iteration

            paddw       mm2, mm3                // fractional counter += fractional increment

            psrlw       mm4, 8                  // 0 | a | 0 | r || 0 | g | 0 | b        

            packuswb    mm4, mm4                // a | r | g | b || a | r | g | b

            movd        [edi - 4], mm4        
            movq        mm4, mm2                // Prepare for next iteration

pre_stage2_loop:

            cmp         esi, 0
            jz          stage3_loop             // Do we need to execute the stage2_loop?

stage2_loop:

            psrlw       mm4, 8                  // 0 | mult | 0 | mult || 0 | mult | 0 | mult
            sar         edx, cl

            cmp         extendMode, 0           // apply wrap mode to the pixel if needed
            jnz         stage2_extend_left_1
            and         edx, uIntervalMask
            jmp         stage2_after_extend_1

stage2_extend_left_1:

            cmp         edx, 0
            jge         stage2_extend_right_1
            mov         edx, 0
            pxor        mm4, mm4

stage2_extend_right_1:

            cmp         edx, uTexelCountMinusOne
            jl          stage2_after_extend_1
            mov         edx, uTexelCountMinusOne
            pxor        mm4, mm4

stage2_after_extend_1:

            paddd       mm0, mm1                // interval counter += interval increment
            pmullw      mm4, [ebx + edx*8]

            add         edi, 8                  // pArgbDest++

            paddw       mm2, mm3                // fractional counter += fractional increment

            paddw       mm4, [eax + edx*8]
            
            movd        edx, mm0                // Prepare for next iteration
            sar         edx, cl

            movq        mm5, mm2                
            psrlw       mm5, 8                  // 0 | mult | 0 | mult || 0 | mult | 0 | mult
            psrlw       mm4, 8                  // 0 | a | 0 | r || 0 | g | 0 | b        

            cmp         extendMode, 0           // apply wrap mode to the pixel if needed
            jnz         stage2_extend_left_2
            and         edx, uIntervalMask
            jmp         stage2_after_extend_2

stage2_extend_left_2:

            cmp         edx, 0
            jge         stage2_extend_right_2
            mov         edx, 0
            pxor        mm5, mm5

stage2_extend_right_2:

            cmp         edx, uTexelCountMinusOne
            jl          stage2_after_extend_2
            mov         edx, uTexelCountMinusOne
            pxor        mm5, mm5

stage2_after_extend_2:

            paddd       mm0, mm1                // interval counter += interval increment
            pmullw      mm5, [ebx + edx*8]
            dec         esi                     // count--

            paddw       mm5, [eax + edx*8]
            movd        edx, mm0                // Prepare for next iteration

            paddw       mm2, mm3                // fractional counter += fractional increment

            psrlw       mm5, 8                  // 0 | a | 0 | r || 0 | g | 0 | b        
  
            packuswb    mm4, mm5
            
            movq        [edi - 8], mm4        
            movq        mm4, mm2                // Prepare for next iteration
            jnz         stage2_loop

stage3_loop:

            dec         stage3_count
            jnz         skip_stage3_loop

            psrlw       mm4, 8                  // 0 | mult | 0 | mult || 0 | mult | 0 | mult
            sar         edx, cl

            cmp         extendMode, 0           // apply wrap mode to the pixel if needed
            jnz         stage3_extend_left
            and         edx, uIntervalMask
            jmp         stage3_after_extend

stage3_extend_left:

            cmp         edx, 0
            jge         stage3_extend_right
            mov         edx, 0
            pxor        mm4, mm4

stage3_extend_right:

            cmp         edx, uTexelCountMinusOne
            jl          stage3_after_extend
            mov         edx, uTexelCountMinusOne
            pxor        mm4, mm4

stage3_after_extend:

            pmullw      mm4, [ebx + edx*8]
            paddd       mm0, mm1                // interval counter += interval increment

            paddw       mm4, [eax + edx*8]
            movd        edx, mm0                // Prepare for next iteration

            paddw       mm2, mm3                // fractional counter += fractional increment

            psrlw       mm4, 8                  // 0 | a | 0 | r || 0 | g | 0 | b        

            packuswb    mm4, mm4                // a | r | g | b || a | r | g | b
  
            movd        [edi], mm4        

skip_stage3_loop:

            emms
        }
    }

#endif
}

CRadialGradientBrushSpan::CRadialGradientBrushSpan()
    : CGradientBrushSpan()
{
}

CRadialGradientBrushSpan::~CRadialGradientBrushSpan()
{
}

HRESULT 
CRadialGradientBrushSpan::Initialize(
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
    __in_ecount(3) const MilPoint2F *pGradientPoints,
    __in_ecount(uCount) const MilColorF *pColors,
    __in_ecount(uCount) const FLOAT *pPositions,
    __in UINT uCount,
    __in MilGradientWrapMode::Enum wrapMode,
    __in MilColorInterpolationMode::Enum colorInterpolationMode
    )
{
    HRESULT hr = S_OK;

    CMILMatrix matDeviceIPCtoNormBrushHPC;

    if (SUCCEEDED(hr))
    {
        MIL_THR(InitializeTexture(
                pmatWorldHPCToDeviceHPC,
                pGradientPoints,
                TRUE, // Is a radial gradient
                pColors,
                pPositions,
                uCount,
                wrapMode,
                colorInterpolationMode,
                &matDeviceIPCtoNormBrushHPC                          
                ));
    }
 
    if (SUCCEEDED(hr))
    {
        m_rM11 = matDeviceIPCtoNormBrushHPC.GetM11();
        m_rM21 = matDeviceIPCtoNormBrushHPC.GetM21();
        m_rDx  = matDeviceIPCtoNormBrushHPC.GetDx();

        m_rM12 = matDeviceIPCtoNormBrushHPC.GetM12();
        m_rM22 = matDeviceIPCtoNormBrushHPC.GetM22();
        m_rDy  = matDeviceIPCtoNormBrushHPC.GetDy();
    }

    RRETURN(hr);
}

class TypeSSE { bool SSE; };
class TypeNoSSE {};

VOID 
FASTCALL ColorSource_RadialGradient_32bppPARGB(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    CRadialGradientBrushSpan *pColorSource = 
        DYNCAST(CRadialGradientBrushSpan, pSOP->m_posd);
    Assert(pColorSource);

#if defined(_X86_)
    if (CCPUInfo::HasSSE())
    {
        pColorSource->GenerateColors<TypeSSE>(
            pPP->m_iX,
            pPP->m_iY, 
            pPP->m_uiCount, 
            (ARGB*) pSOP->m_pvDest
            );
    }
    else
    {
        pColorSource->GenerateColors<TypeNoSSE>(
            pPP->m_iX,
            pPP->m_iY,
            pPP->m_uiCount,
            (ARGB*) pSOP->m_pvDest
            );
    }
#elif defined(_AMD64_)
    pColorSource->GenerateColors<TypeSSE>(
        pPP->m_iX,
        pPP->m_iY,
        pPP->m_uiCount,
        (ARGB*) pSOP->m_pvDest
        );
#else
    pColorSource->GenerateColors<TypeNoSSE>(
        pPP->m_iX,
        pPP->m_iY,
        pPP->m_uiCount,
        (ARGB*) pSOP->m_pvDest
        );
#endif
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CRadialGradientBrushSpan::ReleaseExpensiveResources, CColorSource
//
//  Synopsis:
//      Release expensive resources
//
//------------------------------------------------------------------------------

VOID 
CRadialGradientBrushSpan::ReleaseExpensiveResources()
{
   // This class doesn't hold onto resources that need to be released
}

template<typename TPlatform>
VOID
CRadialGradientBrushSpan::GenerateColors(
    __in INT nX, 
    __in INT nY, 
    __in INT nCount, 
    __out_ecount_full(nCount) ARGB *pArgbDest
    )
{
    // Copy some class stuff to local variables for faster access in
    // our inner loop:

    AGRB64TEXEL *pStartTexels = &m_rgStartTexelAgrb[0];
    AGRB64TEXEL *pEndTexels = &m_rgEndTexelAgrb[0];
    
    AGRB64TEXEL *pStartTexel;
    AGRB64TEXEL *pEndTexel;
    
    UINT uWeightA = 256;
    UINT uWeightB = 0;

    BOOL extendMode = (m_wrapMode == MilGradientWrapMode::Extend);
    INT nTexelCountMinusOne = static_cast<INT>(m_uTexelCountMinusOne);

    // Given our start point in device space, figure out the corresponding
    // normalized brush point and then the texel(s).

    __if_exists(TPlatform::SSE)
    {
        CXmmFloat rXIncrement = m_rM11;
        CXmmFloat rYIncrement = m_rM12;

        CXmmFloat x(nX), y(nY);

        CXmmFloat rXPositionHPC = x * rXIncrement + y * m_rM21 + m_rDx;
        CXmmFloat rYPositionHPC = x * rYIncrement + y * m_rM22 + m_rDy;
    }
    __if_not_exists(TPlatform::SSE)
    {
        FLOAT rXIncrement = m_rM11;
        FLOAT rYIncrement = m_rM12;

        FLOAT rXPositionHPC = nX * rXIncrement + nY * m_rM21 + m_rDx;
        FLOAT rYPositionHPC = nX * rYIncrement + nY * m_rM22 + m_rDy;
    }
            
    //
    // Both TexelCount and FIXED16_INT_MAX + 1 should be powers of 2. This
    // means that FIXED16_INT_MAX + 1 is divisible by TexelCount, so
    // FIXED16_INT_MAX % m_uTexelCount == m_uTexelCount - 1.
    //
    // We use this property to clamp real numbers to fix 16 range, choosing the
    // last texel color when doing so.
    //
    Assert(m_uTexelCount == RoundToPow2(m_uTexelCount));
    Assert((FIXED16_INT_MAX + 1) == RoundToPow2(FIXED16_INT_MAX + 1));
    Assert((FIXED16_INT_MAX % m_uTexelCount) == static_cast<UINT>(nTexelCountMinusOne));

    INT nDistanceIPC;

    do {
        // Calculate the distance of the current point to the center in
        // normalized brush space.  The distance is the unwrapped index into
        // the texture.

        __if_exists(TPlatform::SSE)
        {
            CXmmFloat rDistanceHPC = CXmmFloat::Sqrt( rXPositionHPC * rXPositionHPC + rYPositionHPC * rYPositionHPC );
            rXPositionHPC += rXIncrement;
            rYPositionHPC += rYIncrement;

            // Clamping to FIXED16_INT_MAX will cause us to choose the last
            // texel in the texture because
            // (FIXED16_INT_MAX % m_uTexelCount) == (m_uTexelCount - 1)
            // See assertions above.
            CXmmFloat rDistanceIPC = CXmmFloat::Min(rDistanceHPC - 0.5f, CXmmFloat(FIXED16_INT_MAX));
            rDistanceIPC = rDistanceIPC * FIX16_ONE;

            nDistanceIPC = rDistanceIPC.Round();
        }
        __if_not_exists(TPlatform::SSE)
        {
            FLOAT rDistanceHPC = sqrtf( rXPositionHPC * rXPositionHPC + rYPositionHPC * rYPositionHPC );
            rXPositionHPC += rXIncrement;
            rYPositionHPC += rYIncrement;

            // Clamping to FIXED16_INT_MAX will cause us to choose the last
            // texel in the texture because
            // (FIXED16_INT_MAX % m_uTexelCount) == (m_uTexelCount - 1)
            // See assertions above.
            FLOAT rDistanceIPC = min(rDistanceHPC - 0.5f, static_cast<float>(FIXED16_INT_MAX));
            nDistanceIPC = GpRealToFix16(rDistanceIPC);
        }

        {
            // We want to linearly interpolate between two texels,
            // A and B (where A is the floor pixel, B the ceiling texel).
            // 'uWeightA' is the fraction of texel A that we want, and
            // 'uWeightB' is the fraction of texel B that we want:

            uWeightB = ONEDGETFRACTIONAL8BITS(nDistanceIPC);   
            uWeightA = 256 - uWeightB;

            // We could actually do a big lookup table right off of 'xTexture'
            // for however many bits of precision we wanted to do.  But that
            // would be too much work in the setup.

            // __bound indicates that this value is carefully bounds checked.  In this case
            // nTextureIndex is bounded at 0..nTexelCountMinusOne explicitly by the first two cases
            // and in a trickier manner by the &= operator in the last else clause below.
            __bound INT nTextureIndex = ONEDGETINTEGERBITS(nDistanceIPC);

            // Check to see that we are sampling within the first half texel.
            // Remember that nTextureIndex is in IPC space so if it is negative
            // then we are really just less than 0.5 in HPC space
            if (nTextureIndex < 0)
            { 
                // In the first half-texel, we should always choose the first texel color. Otherwise
                // we may end up interpolating with the last texel color near the origin of the gradient.

                nTextureIndex = 0;
                uWeightA = 256;
                uWeightB = 0;
            }
            else if (extendMode)
            {
                // Clamp the end of the radial gradient to 1
                if (nTextureIndex >= nTexelCountMinusOne)
                {
                    nTextureIndex = nTexelCountMinusOne;
                    uWeightA = 256;
                    uWeightB = 0;
                }
            }
            else
            {
                // Because the texel count is a power of 2, we can accomplish a mod (%)
                // operation using a bitwise &.  That is:
                // 'value % NumberOfTexels' == 'value & NumberOfTexelsMinusOne'                            
                nTextureIndex &= nTexelCountMinusOne;
            }

            pStartTexel = &pStartTexels[nTextureIndex];
            pEndTexel = &pEndTexels[nTextureIndex];

            // Note that we can gamma correct the texels so that we don't
            // have to do gamma correction here.  The addition of constants
            // here are to accomplish rounding:

            UINT rrrrbbbb = (pStartTexel->A00rr00bb * uWeightA) 
                            + (pEndTexel->A00rr00bb * uWeightB)
                            + 0x00800080;

            UINT aaaagggg = (pStartTexel->A00aa00gg * uWeightA)
                            + (pEndTexel->A00aa00gg * uWeightB)
                            + 0x00800080;

            *pArgbDest++ = (aaaagggg & 0xff00ff00) + ((rrrrbbbb & 0xff00ff00) >> 8);
        }
    } while (--nCount != 0);
}

CFocalGradientBrushSpan::CFocalGradientBrushSpan()
    : CRadialGradientBrushSpan()
{
}

CFocalGradientBrushSpan::~CFocalGradientBrushSpan()
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGradientTextureGen::TransformPointFromWorldHPCToGradientCircle
//
//  Synopsis:
//      Transforms a point from world space to gradient circle space
//      Note that this space is not normalized.
//
//------------------------------------------------------------------------------
void CFocalGradientBrushSpan::TransformPointFromWorldHPCToGradientCircle(
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatLocalHPCtoDeviceHPC,
    __in_ecount(1) const MilPoint2F *pptWorldHPC,
    __out_ecount(1) FLOAT *pflXUnitCircle,
    __out_ecount(1) FLOAT *pflYUnitCircle
    ) const
{
    MilPoint2F ptDeviceIPC;

    // after transformation ptDeviceIPC is actually in HPC space. Fixup in just a bit
    pmatLocalHPCtoDeviceHPC->Transform(pptWorldHPC, &ptDeviceIPC, 1);
    
    // convert ptDevice from HPC to IPC space
    ptDeviceIPC.X -= 0.5f;
    ptDeviceIPC.Y -= 0.5f;

    *pflXUnitCircle = m_rM11 * ptDeviceIPC.X + m_rM21 * ptDeviceIPC.Y + m_rDx;
    *pflYUnitCircle = m_rM12 * ptDeviceIPC.X + m_rM22 * ptDeviceIPC.Y + m_rDy; 
}

HRESULT 
CFocalGradientBrushSpan::Initialize(
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
    __in_ecount(3) const MilPoint2F *pGradientPoints,
    __in_ecount(uCount) const MilColorF *pColors,
    __in_ecount(uCount) const FLOAT *pPositions,
    __in UINT uCount,
    __in MilGradientWrapMode::Enum wrapMode,
    __in MilColorInterpolationMode::Enum colorInterpolationMode,
    __in_ecount(1) const MilPoint2F *pFocalPoint
    )
{
    HRESULT hr = S_OK;

    MIL_THR(CRadialGradientBrushSpan::Initialize(
        pmatWorldHPCToDeviceHPC,
        pGradientPoints,
        pColors,
        pPositions,
        uCount,
        wrapMode,
        colorInterpolationMode
        ));

    if (SUCCEEDED(hr))
    {
        TransformPointFromWorldHPCToGradientCircle(
            pmatWorldHPCToDeviceHPC,
            pFocalPoint,
            &m_rXFocalHPC,
            &m_rYFocalHPC
            );

        {
            //
            //  Calculate the center of the region in unit circle space that contains the
            //  color of the first texel, not interpolated with anything else.
            //  
            //  See comment in the Addendum to the notes in
            //  CFocalGradientBrushSpan::GenerateColors for more details about this
            //  calculation.
            //


            FLOAT t = 0.5f / m_flGradientSpanEnd;

            m_rXFirstTexelRegionCenter = m_rXFocalHPC * (1 - t);
            m_rYFirstTexelRegionCenter = m_rYFocalHPC * (1 - t);
        }
    }

    RRETURN(hr);
}

VOID 
FASTCALL ColorSource_FocalGradient_32bppPARGB(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    CFocalGradientBrushSpan *pColorSource = 
        DYNCAST(CFocalGradientBrushSpan, pSOP->m_posd);
    Assert(pColorSource);

    pColorSource->GenerateColors(
        pPP->m_iX,
        pPP->m_iY,
        pPP->m_uiCount,
        (ARGB*) pSOP->m_pvDest
        );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CFocalGradientBrushSpan::GenerateColors
//
//  Synopsis:
//      Calculates the color values for a span being filled with a focal
//      gradient brush.
//
//  Notes:
//      For each sample point, we need to determine the texture index into the
//      gradient texture.  To accomplish this we must first determine where
//      that sample point lies on the [0.0, 1.0] gradient span (where the
//      color at 0.0 is defined by gradient stop offset 0.0, and likewise for
//      1.0).  Once this is known, we can map the gradient span to the texture.
//
//      The focal point defines the location where the the gradient span begins
//      (the point where the gradient stop offset of 0.0 maps to).  The end of
//      the gradient line (the point where the gradient stop offset of 1.0 maps
//      to) is a point on the perimeter of the user-specified ellipse that is on
//      the same line containing the focal & sample points.  Thus, to determine
//      the location on the gradient span of any given sample point, we must
//      know the focal point, sample point, and the intersection of the line
//      containing those two points with the perimeter of the ellipse.
//
//      Since we already know the focal (start) and sample points, all that is
//      left to determine is where the line containing the sample point & start
//      point intersects the ellipse.  Thus, we solve the ellipse equation in
//      terms of the line equation.
//
//      There are a few simplifications in this implementation that must be
//      noted. First, to simplify the ellipse equation that we are solving, we
//      derive a transform that 1) Scales in the X & Y direction non-uniformly
//      such that the ellipse becomes a circle, 2) Places the center of the
//      circle at the origin (0, 0).
//
//      Secondly, instead of determining the exact (x,y) coordinate where the
//      intersection occurs, we use the parametric line equations and solve for
//      the positive value of t where the line intersects the circle, 'T'.  We
//      set the line equation up such that t=1 at the sample point, t=0 at the
//      beginning of the gradient span, and t=T and the end of gradient span
//      (also the intersection). Thus, to determine the location of the sample
//      point on the gradient span, we simply calculate the ratio tAtSamplePoint
//      / tAtIntersection, or 1/T.
//
//      Finally, the gradient span needs to be mapped to the gradient texture. 
//      For tile wrap mode this is a direct mapping -- every texel in the
//      texture represents a portion of the gradient span.  Thus, [0.0, 1.0]
//      maps to [0, texelCount]. Because this mapping is simply a scale by
//      texelCount, we can multiply 1/T by the texel count to get a texel index
//      from the gradient span.
//
//      To implement extend wrap mode we add an extra texel to the end of the
//      texture with the extend color, so the mapping is from [0.0, 1.0] to [0,
//      texelCount-1]. For reflect wrap mode we duplicate the texels in reverse
//      order, so the gradient span to texture mapping is [0.0, 1.0] to [0,
//      texelCount/2].
//
//      Function Derivation:
//
//      Circle Equation: x^2 + y^2 = r^2
//      Parametric line equations: x = x' + (x'' - x') * t, y = y' + (y'' - y') * t
//
//      Let (xf, yf) be the focal point [named (m_rXFocalHPC, m_rYFocalHPC) in
//      the implementation] and (xs, ys) be the sample point [named (x, y) in
//      the implementation]
//
//      We setup the line equation such that the focal point, which is also the
//      origin of the gradient texture), is at t=0 and the sample point is at at
//      t=1.  Thus, (x', y') = (xf, yf) and (x'', y'') = (xs, ys).
//
//      Substituting the sample & focal point into the parametric line equation:
//
//                  x = xf + (xs - xf)*t, y = yf + (ys - yf)*t
//
//      Let, dX = xs - xf, and dY = ys - yf:
//
//                  x = xf + dX*t, y = yf + dY*t
//
//      Subtituting the parametric line equations into the circle equation for x
//      & y:
//
//                  (xf + dX*t)^2 + (yf + dY*t)^2 = r^2
//
//      Expanding the squared terms:
//
//        [xf^2 + 2*xf*dX*t + dX^2*t^2] + [yf^2 + 2*yf*dY*t + dY^2*t^2] = r^2
//
//      Factoring out 't' from the expanded terms, and subtracting r^2 from both
//      sides:
//
//          t^2*(dX^2 + dY^2) + 2t*(xf*dX + yf*dY) + xf^2 + yf^2 - r^2 = 0
//
//      Next, we define the A, B, and C terms to use in the quadratic equation:
//      Let A = dX^2 + dY^2, B = 2*(xf*dX + yf*dY), and C = xf^2 + yf^2 - r^2
//
//      Substitute A, B, & C into the quadratic equation.  We are only
//      interested in the positive root because the positive root is the end
//      point of the gradient span. Another way to view this (for sample points
//      within the circle) is that the the sample point lies between the focal
//      point and the positive root (this isn't true for the negative root).
//
//             T =  [-B + sqrt(B^2 - 4*A*C) ] / 2 * A
//
//      Because the sample point on the gradient span is given by 1/T, we invert
//      the numerator & denominator:
//
//             samplePoint = 2 * A / [sqrt(B^2 - 4*A*C) - B]
//
//      The equation above gives us the location of the sample point on the
//      normalized [0.0, 1.0] gradient span.  The last step is to multiply that
//      value by the number of texels in the gradient span to obtain the proper
//      index into the texture.
//
//             texelIndex = 2 * A * m_uGradientSpanTexelCount /  [sqrt(B^2 - 4*A*C) - B]
//
//      The final equation above is what is solved by this method to determine
//      the the location of the sample point on the gradient span.  Once this is
//      determined, we map the gradient span to the texels in the texture that
//      represent the gradient span.  The number of texels representing the
//      gradient span is texelCount for tile wrap mode, texelCount/2 for flip
//      wrap mode, and texelCount-1 for extend wrap mode.
//
//      Calculation of the determinant to avoid numerical instability:
//
//      It is better when calculating the determinant not to use the simple
//      formula B^2 - 4AC. Our terms, A, B, and C share components in such a way
//      that B^2 - 4AC has some terms in it which cancel each other out. These
//      terms are actually on the order of 1/RadiusX^4 or 1/RadiusY^4, so if one
//      of the radii are very small, that means that we would have precision
//      problems by letting floating point operations take care of this
//      cancellation. The other components of B^2 would be lost.
//
//      See mil\core\docs\RadialGradientNotes.mht for a derivation of the new
//      formula using vectors. For a dervation using the vector components that
//      are in the code, see below
//
//          d_x = rDeltaX
//          d_y = rDeltaY
//          f_x = m_rXFocalHPC
//          f_y = m_rYFocalHPC
//          g = m_flGradientSpanEnd
//
//          a = d_x^2 + d_y^2
//          b = 2(f_x * d_x + f_y * d_y)
//          c = f_x^2 + f_y^2 - g^2
//
//          b^2 = 4(f_x^2*d_x^2 + 2*f_x*d_x*f_y*d_y + f_y^2*d_y^2)
//          4ac = 4(f_x^2*d_x^2 + f_y^2*d_x^2 - d_x^2*g^2 + f_x^2*d_y^2 + f_y^2*d_y^2 - d_y^2g^2)
//
//          When we evaluate b^2 - 4ac, the f_x^2*d_x^2 and f_y^2*d_y^2 terms cancel out.
//
//          b^2 - 4ac = 4(g^2 (d_x^2 + d_y^2) - (d_x*f_y - d_y*f_x)^2)
//                    = 4(g^2 * a - rSampleToOriginCrossOriginNorm^2)
//
//          It is these terms
//              f_x^2*d_x^2 and f_y^2*d_y^2
//          that are very large when one of the radii becomes small.
//
//  Addendum:
//      The above algorithm works well when the sample point is a distance away
//      from the gradient origin. However, when the sample point gets very close
//      to or equal to the gradient origin, the result of the quadratic equation
//      approaches 0 / 0. Indeed, if you look at how the equation was derived,
//      we have an equation where we are solving for T, but T is dropping out of
//      the equation.
//
//      What do we do then? Fortunately, we don't have to solve the quadratic
//      equation, because one of our rendering rules is that we clamp any
//      texelIndex less than 1/2 of the first texel to 0.5 in non-normalized
//      texture space, thereby choosing the first texel color. This rule exists
//      to prevent us from wrapping around, interpolating from the colors at
//      stops 0 and 1 for texelIndices < 0.5. Without this rule, such a wrapping
//      would produce a funny color at the origin of the radial gradient.
//
//      How does this rule help? If you think of the gradient as having contour
//      lines, near the origin we have a flat region. Any sample point that
//      falls inside the innermost region of radius 0.5 should always produce
//      the same color, the color of the first gradient texel. For these sample
//      points we can skip the quadratic equation altogether and just test to
//      see if we are in this inner region.
//
//      We are fortunate that this inner region takes on the shape of a circle
//      in gradient circle space. This can be proved geometrically by noticing
//      that a triangle containing three points of this circle is a right
//      triangle, being similar to a right triangle formed by three points in
//      the larger circle.
//
//      The center for this inner circle can be calculated using a linear
//      interpolation between the gradient origin and the gradient center. This
//      circle represents a contour line in the gradient. The countour lines
//      form concentric, offset circles, where the center of the really small
//      circles (with gradient positions near 0.0) are near the gradient origin
//      and the center for circles with gradient positions near 1.0 are near the
//      gradient center. This gives us the equation
//
//              RegionCenter = Origin * (1 - t) + Center * t
//
//      where t is the center of the first texel in normalized texture space, or
//      0.5 / texel count.
//
//      The radius of the region is also = t, though the software code, not
//      using normalized space (sometimes), may actually use radius = 0.5
//      instead of 0.5 / texel count.
//
//   The region is not a circle when the gradient origin
//  is outside the end point circle. In the case the region is a "pie slice",
//  formed by cutting the circle with the two lines that are tangent to the end
//  point circle and containing the gradient origin. Because the region we care
//  about is so small, this issue has never been perceived. Additionally, we
//  don't care so much about this region in this case, since it is unlikely
//  customers will like to look at it.
//
//------------------------------------------------------------------------------
VOID 
CFocalGradientBrushSpan::GenerateColors(
    __in INT nX, 
    __in INT nY, 
    __in INT nCount, 
    __out_ecount_full(nCount) ARGB *pArgbDest
    )
{
    // Copy some class stuff to local variables for faster access in
    // our inner loop:

    const AGRB64TEXEL *pStartTexels = &m_rgStartTexelAgrb[0];
    const AGRB64TEXEL *pEndTexels = &m_rgEndTexelAgrb[0];
    
    const AGRB64TEXEL *pStartTexel;
    const AGRB64TEXEL *pEndTexel;
    
    UINT uWeightA = 256;
    UINT uWeightB = 0;

    FLOAT rXIncrement = m_rM11;
    FLOAT rYIncrement = m_rM12;

    MilGradientWrapMode::Enum wrapMode = m_wrapMode;
    
    INT nTexelCountMinusOne = static_cast<INT>(m_uTexelCountMinusOne);

    // The number of texels in the texture that represent the gradient span
    // (and not the wrap modes).
    FLOAT rGradientSpanLength_x_2 = m_flGradientSpanEnd * 2.0f;
    FLOAT rGradientSpanLength_sqr = m_flGradientSpanEnd * m_flGradientSpanEnd;

    FLOAT rFirstTexelRegionRadiusSquared = 0.25f; // 0.5 is the radius is one half texel 

    // Calculate dX (rDeltaX) and dY (rDeltaY)
    //
    // Transform x & y into the circular (non-elliptical) brush space,
    // and subtract m_rXFocalHPC/m_rYFocalHPC to get dX and dY
    FLOAT rDeltaX = (nX * m_rM11) + (nY * m_rM21) + m_rDx - m_rXFocalHPC;
    FLOAT rDeltaY = (nX * m_rM12) + (nY * m_rM22) + m_rDy - m_rYFocalHPC;    

    FLOAT rDeltaToRegionCenterX = m_rXFocalHPC - m_rXFirstTexelRegionCenter;
    FLOAT rDeltaToRegionCenterY = m_rYFocalHPC - m_rYFirstTexelRegionCenter;

    do {

        INT nGradientSpanPositionIPC;

        // Calculate the A term we plug into the quadratic equation
        FLOAT rA = rDeltaX * rDeltaX + rDeltaY * rDeltaY;

        if (   (rA < 0.0001)
            && (  (rDeltaX + rDeltaToRegionCenterX) * (rDeltaX + rDeltaToRegionCenterX)
                + (rDeltaY + rDeltaToRegionCenterY) * (rDeltaY + rDeltaToRegionCenterY)
                < rFirstTexelRegionRadiusSquared)
           )
        {
            //
            // Skip the quadratic equation when it is in danger of breaking down numerically.
            // In the first half-texel region, we always choose the first texel. See comment above for
            // an explanation of how we know that this region is a circle with these dimensions.
            //
            // We do not need this rA < 0.0001 check, but it helps perf considerably.
            //

            nGradientSpanPositionIPC = 0;
        }
        else
        {
            FLOAT rGradientSpanPositionHPC;
    
            
            // Calculate the B term we plug into the quadratic equation
            FLOAT rB = 2 * (m_rXFocalHPC * rDeltaX + m_rYFocalHPC * rDeltaY);
    
            //
            // Calculate the determinant of the quadratic equation. See the
            // method synopis for an explanation of this code.
            //
            FLOAT rSampleToOriginCrossOriginNorm = (rDeltaX * m_rYFocalHPC - rDeltaY * m_rXFocalHPC);
            FLOAT rDeterminant = 4.0f * (rGradientSpanLength_sqr * rA - rSampleToOriginCrossOriginNorm * rSampleToOriginCrossOriginNorm);

    
            // Note: sometimes this produces NaN. That's okay, because...
            // (read comment for if statement)
            rGradientSpanPositionHPC =  (rA * rGradientSpanLength_x_2) / (sqrtf(rDeterminant) - rB);

            // The -0.5 transforms from HPC to IPC space
            FLOAT rGradientSpanPositionIPC = rGradientSpanPositionHPC - 0.5f;

            // This form causes us to enter the if statement when rGradientSpanPositionHPC is NaN.
            if (   !(rGradientSpanPositionHPC >= 0.0f)
                || rGradientSpanPositionIPC > FIXED16_INT_MAX
               )
            {
                //
                // The complex region of the gradient and the negative region are visible
                // when the gradient origin is outside the gradient circle. These regions
                // are outside the tangent lines from the gradient orgin to the gradient
                // circle. No color really makes sense in these regions because the ray
                // beginning at the gradient origin and going through the sample point
                // does not intersect the end point circle. We choose the last texel to
                // be predictable. This choice also goes well with the extend wrap
                // mode for most of the brush space.
                //

                nGradientSpanPositionIPC = GpIntToFix16(nTexelCountMinusOne);
            }
            else
            {
                //
                // Compute the position of the sample point along the [0.0, 1.0] gradient span
                //
                
                nGradientSpanPositionIPC = GpRealToFix16(rGradientSpanPositionIPC);
            }
        }

        {
            // We want to linearly interpolate between two pixels,
            // A and B (where A is the floor pixel, B the ceiling pixel).
            // 'uWeightA' is the fraction of pixel A that we want, and
            // 'uWeightB' is the fraction of pixel B that we want:

            uWeightB = ONEDGETFRACTIONAL8BITS(nGradientSpanPositionIPC);   
            uWeightA = 256 - uWeightB;

            // We could actually do a big lookup table right off of 'xTexture'
            // for however many bits of precision we wanted to do.  But that
            // would be too much work in the setup.

            // __bound indicates that this value is carefully bounds checked.  In this case
            // nTextureIndex is bounded at 0..nTexelCountMinusOne explicitly by the first two cases
            // and in a trickier manner by the &= operator in the last clause below.
            __bound INT nTextureIndex = ONEDGETINTEGERBITS(nGradientSpanPositionIPC);

            // Check to see that we are sampling within the first half texel.
            // Remember that nTextureIndex is in IPC space and that we have pruned out
            // indices in negative HPC space already. If nTextureIndex is negative
            // then we are really just less than 0.5 in HPC space
            if (nTextureIndex < 0)
            { 
                // In the first half-texel, we should always choose the first texel color. Otherwise
                // we may end up interpolating with the last texel color near the origin of the gradient.

                nTextureIndex = 0;
                uWeightA = 256;
                uWeightB = 0;
            }
            else if (MilGradientWrapMode::Extend == wrapMode)
            {
                // Clamp the end of the radial gradient to the last texel color for extend mode
                if (nTextureIndex >= nTexelCountMinusOne)
                {
                    nTextureIndex = nTexelCountMinusOne;
                    uWeightA = 256;
                    uWeightB = 0;
                }
            }
            else
            {
                Assert( (MilGradientWrapMode::Flip == wrapMode) ||
                        (MilGradientWrapMode::Tile == wrapMode));
                
                // This operation is valid for both flip & tile wrap modes.
                //
                // This is true for flip wrap mode because the texture index 
                // has been mapped to the correct location in the 'flipped' 
                // texture, and it only needs to wrap back to the beginning 
                // of the texture after it reaches the last texel.
                
                nTextureIndex &= nTexelCountMinusOne;
            }

            pStartTexel = &pStartTexels[nTextureIndex];
            pEndTexel = &pEndTexels[nTextureIndex];

            // Note that we can gamma correct the texels so that we don't
            // have to do gamma correction here.  The addition of constants
            // here are to accomplish rounding:

            UINT rrrrbbbb = (pStartTexel->A00rr00bb * uWeightA) 
                            + (pEndTexel->A00rr00bb * uWeightB)
                            + 0x00800080;

            UINT aaaagggg = (pStartTexel->A00aa00gg * uWeightA)
                            + (pEndTexel->A00aa00gg * uWeightB)
                            + 0x00800080;

            *pArgbDest++ = (aaaagggg & 0xff00ff00) + ((rrrrbbbb & 0xff00ff00) >> 8);

        }

        // increment rDeltaX, rDeltaY

        rDeltaX += rXIncrement;
        rDeltaY += rYIncrement;
    } while (--nCount != 0);
}


VOID 
FASTCALL ColorSource_ShaderEffect_32bppPARGB(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    CShaderEffectBrushSpan *pColorSource = 
        DYNCAST(CShaderEffectBrushSpan, pSOP->m_posd);
    Assert(pColorSource);

    pColorSource->GenerateColors(
        pPP->m_iX, 
        pPP->m_iY, 
        pPP->m_uiCount, 
        (ARGB*) pSOP->m_pvDest
        );
}

//+-----------------------------------------------------------------------------
//
//  CShaderEffectBrushSpan::ctor
//
//------------------------------------------------------------------------------

CShaderEffectBrushSpan::CShaderEffectBrushSpan()
{
    m_pfnGenerateColorsEffectWeakRef = NULL;
    m_pShaderEffectBrushNoRef = NULL;
    m_pPixelShaderCompiler = NULL;
}

//+-----------------------------------------------------------------------------
//
//  CShaderEffectBrushSpan::ReleaseExpensiveResources
//
//------------------------------------------------------------------------------

void 
CShaderEffectBrushSpan::ReleaseExpensiveResources()
{
    m_pfnGenerateColorsEffectWeakRef = NULL;
    ReleaseInterface(m_pPixelShaderCompiler);
    m_pShaderEffectBrushNoRef = NULL;
}

//+-----------------------------------------------------------------------------
//
//  CShaderEffectBrushSpan::Initialize
//
//  Synopsis:
//      Initializes the shader effect brush span.
//
//------------------------------------------------------------------------------

HRESULT 
CShaderEffectBrushSpan::Initialize(
    __in const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::DeviceHPC> *pRealizationSamplingToDevice,
    __inout CMILBrushShaderEffect* pShaderEffectBrush)
{
    HRESULT hr = S_OK;

    m_pShaderEffectBrushNoRef = pShaderEffectBrush;
    m_pixelShaderState = CPixelShaderState(); // Initialize the pixel shader state.   

    IFC(pShaderEffectBrush->PreparePass(
        pRealizationSamplingToDevice,
        OUT &m_pixelShaderState, 
        OUT &m_pPixelShaderCompiler));
    
    m_pfnGenerateColorsEffectWeakRef = m_pPixelShaderCompiler->GetGenerateColorsFunction();

Cleanup:
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  CShaderEffectBrushSpan::GenerateColors
//
//  Synopsis:
//      Calculates the color values for a span being filled with a a shader 
//      effect.
//------------------------------------------------------------------------------

void
CShaderEffectBrushSpan::GenerateColors(
    __in INT nX, 
    __in INT nY, 
    __in INT nCount, 
    __out_ecount_full(nCount) ARGB *pArgbDest)
{
    Assert(m_pfnGenerateColorsEffectWeakRef);
    
    GenerateColorsEffectParams params;
    params.pPixelShaderState = &m_pixelShaderState;
    params.nX = nX;
    params.nY = nY;
    params.nCount = nCount;
    params.pPargbBuffer = reinterpret_cast<unsigned *>(pArgbDest);
    
    (*m_pfnGenerateColorsEffectWeakRef)(&params);
}




