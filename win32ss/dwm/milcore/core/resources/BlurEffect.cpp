// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Blur effect resource.
//
//-----------------------------------------------------------------------------


#include "precomp.hpp"

MtDefine(BlurEffectResource, MILRender, "BlurEffect Resource");
MtDefine(CMilBlurEffectDuce, BlurEffectResource, "CMilBlurEffectDuce");
MtDefine(BoxBlurLineBuffer, BlurEffectResource, "BoxBlurLineBuffer");

CMilPixelShaderDuce* CMilBlurEffectDuce::s_pBlurPixelShaders[4] = { 0 };

GenerateColorsBlur CMilBlurEffectDuce::s_pfnBlurFunctionBox = NULL;
GenerateColorsBlur CMilBlurEffectDuce::s_pfnBlurFunctionGaussian = NULL;

const f32x4 c_rZero = {0.0f, 0.0f, 0.0f, 0.0f};
const u32x4 c_uZero = {0, 0, 0, 0};
const f32x4 c_rOne = {1.0f, 1.0f, 1.0f, 1.0f};
const u32x4 c_uOne = {1, 1, 1, 1};

// SSE2 memory operations require 16 byte boundary alignment
#define SSE2_ALIGNMENT_BOUNDARY 16

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::CMilBlurEffectDuce
//
// Synopsis: 
//      A constructor used internally if we need to create a blur effect to
//      run within another effect's pipeline.
//
//-----------------------------------------------------------------------------
CMilBlurEffectDuce::CMilBlurEffectDuce(
    DOUBLE radius, 
    MilKernelType::Enum kernelType,
    MilEffectRenderingBias::Enum renderingBias)
{
    m_data.m_Radius = radius;
    m_data.m_pRadiusAnimation = NULL;
    m_data.m_KernelType = kernelType;
    m_data.m_RenderingBias = renderingBias;
}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::~CMilBlurEffectDuce
//
//-----------------------------------------------------------------------------

CMilBlurEffectDuce::~CMilBlurEffectDuce()
{
    if (m_pBoxBlurLineBuffer)
    {
        WPFFree(ProcessHeap, m_pBoxBlurLineBuffer);
    }
    UnRegisterNotifiers();
}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::Create
//
// Synopsis: 
//      Creates a hardware-accelerated blur effect for use in other ImageEffect
//      pipelines.  This effect can be applied inside a pipeline by calling
//      ApplyEffectForPipeline, or as the final pipeline stage by calling 
//      ApplyEffect.
//
//-----------------------------------------------------------------------------

HRESULT 
CMilBlurEffectDuce::Create(
    DOUBLE radius, 
    MilKernelType::Enum kernelType,
    MilEffectRenderingBias::Enum renderingBias,
    __deref_out CMilBlurEffectDuce **ppBlur)
{
    HRESULT hr = S_OK;

    CMilBlurEffectDuce *pBlur = NULL;

    IFCOOM(pBlur = new CMilBlurEffectDuce(radius, kernelType, renderingBias));    
    pBlur->AddRef();
   
    IFC(pBlur->Initialize());

    *ppBlur = pBlur; // Transitioning ref count to out argument
    pBlur = NULL;
   
Cleanup:
    ReleaseInterface(pBlur);
    RRETURN(hr);

}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::Initialize
//
//-----------------------------------------------------------------------------

HRESULT 
CMilBlurEffectDuce::Initialize()
{
    HRESULT hr = S_OK;

    //
    // Map the shader byte code for the blur shader. 

    CMilPixelShaderDuce* pHorizontal = NULL;
    CMilPixelShaderDuce* pVertical = NULL;
    CMilPixelShaderDuce* pHorizontalMulti = NULL;
    CMilPixelShaderDuce* pVerticalMulti = NULL;
    
    if (s_pBlurPixelShaders[0] == NULL)
    {
        //
        // Shaders are organized as follows into the s_pShaderByteCodes array.
        //
        //   Position  Shader
        //    0        Horizontal
        //    1        Vertical
        //    2        Horizontal multi-input
        //    3        Vertical multi-input
        
        BYTE* pShaderByteCode = NULL;
        UINT shaderByteCodeSize = 0;
        IFC(LockResource(PS_BlurH, &pShaderByteCode, &shaderByteCodeSize));
        IFC(CMilPixelShaderDuce::Create(m_pComposition, ShaderEffectShaderRenderMode::HardwareOnly, shaderByteCodeSize, pShaderByteCode, &pHorizontal));
        
        pShaderByteCode = NULL;
        shaderByteCodeSize = 0;
        IFC(LockResource(PS_BlurV, &pShaderByteCode, &shaderByteCodeSize));
        IFC(CMilPixelShaderDuce::Create(m_pComposition, ShaderEffectShaderRenderMode::HardwareOnly, shaderByteCodeSize, pShaderByteCode, &pVertical));
        
        pShaderByteCode = NULL;
        shaderByteCodeSize = 0;
        IFC(LockResource(PS_BlurHMulti, &pShaderByteCode, &shaderByteCodeSize));
        IFC(CMilPixelShaderDuce::Create(m_pComposition, ShaderEffectShaderRenderMode::HardwareOnly, shaderByteCodeSize, pShaderByteCode, &pHorizontalMulti));
        
        pShaderByteCode = NULL;
        shaderByteCodeSize = 0;
        IFC(LockResource(PS_BlurVMulti, &pShaderByteCode, &shaderByteCodeSize));
        IFC(CMilPixelShaderDuce::Create(m_pComposition, ShaderEffectShaderRenderMode::HardwareOnly, shaderByteCodeSize, pShaderByteCode, &pVerticalMulti));

        s_pBlurPixelShaders[0] = pHorizontal; // Transitioning ref to static array
        pHorizontal = NULL;

        s_pBlurPixelShaders[1] = pVertical; // Transitioning ref to static array
        pVertical = NULL;

        s_pBlurPixelShaders[2] = pHorizontalMulti; // Transitioning ref to static array
        pHorizontalMulti = NULL;

        s_pBlurPixelShaders[3] = pVerticalMulti; // Transitioning ref to static array
        pVerticalMulti = NULL;
    }
        
Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(pHorizontal);
        ReleaseInterface(pVertical);
        ReleaseInterface(pHorizontalMulti);
        ReleaseInterface(pVerticalMulti);
    }
    RRETURN(hr);
}



//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::GetScaledRadius
//
// Synopsis: 
//      Returns the scaled radius for the blur to be executed in the shader.
//
//-----------------------------------------------------------------------------
void 
CMilBlurEffectDuce::GetScaledRadius(
    __in const CMILMatrix *pScaleTransform,
    __out UINT* pRadius
    )
{    
    // Determine the current radius.
    UINT localSpaceRadius = static_cast<UINT>(GetRadius());
    // We might have a scale transform applied to this element, so we need to
    // scale the radius as well.
    UINT scaledRadius = 0;
    ApplyRadiusScaling(pScaleTransform, localSpaceRadius, &scaledRadius);

    // Return the scaled radius
    *pRadius = scaledRadius;
}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::GetRadius
//
// Synopsis: 
//      Returns the value of the blur radius property from managed code.
//      Note that the blur radius must be non-negative.
//
//-----------------------------------------------------------------------------
double 
CMilBlurEffectDuce::GetRadius()
{    
    // Determine the current radius.
    double radius = m_data.m_Radius;
    if (m_data.m_pRadiusAnimation != NULL)
    {
        radius = *(m_data.m_pRadiusAnimation->GetValue());
    }
    // Radius must be non-negative
    return max(radius, 0.0);
}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::ApplyRadiusScaling
//
// Synopsis: 
//      Scales the radius by min(scaleX, scaleY) to match the scale transform
//      applied to the element.  We scale by the minimum scale to ensure that
//      the resultant realization fits within the scaled bounds of the element.
//
//-----------------------------------------------------------------------------

void
CMilBlurEffectDuce::ApplyRadiusScaling(
    __in const CMILMatrix *pScaleTransform,
    __in UINT localSpaceRadius,
    __out UINT *scaledRadiusOut
    )
{
    Assert(pScaleTransform->IsPure2DScale());

    REAL prScaleX = 0;
    REAL prScaleY = 0;
    pScaleTransform->GetScaleDimensions(&prScaleX, &prScaleY);

    REAL rLocalSpaceRadius = static_cast<REAL>(localSpaceRadius);
    UINT scaledRadius;

    if (prScaleX <= prScaleY) 
    {
        scaledRadius = static_cast<UINT>(rLocalSpaceRadius * prScaleX);
    }
    else
    {
        scaledRadius = static_cast<UINT>(rLocalSpaceRadius * prScaleY);
    }

    if (scaledRadius > MAX_RADIUS)
    {
        *scaledRadiusOut = MAX_RADIUS;
    }
    else
    {   
        *scaledRadiusOut = scaledRadius;
    }

}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::CalculateGaussianSamplingWeightsFullKernel
//
// Synopsis: 
//      Fills the array with sampling weights. Produces an array of 2* radius + 1 
//      weights. Gaussian only. 
//
//-----------------------------------------------------------------------------
HRESULT
CMilBlurEffectDuce::CalculateGaussianSamplingWeightsFullKernel(
    __in UINT radius,
    __deref_out_xcount(2*radius+1) float **ppSamplingWeightsReplicate
    )
{        
    HRESULT hr = S_OK;

    UINT size = (2 * radius + 1) * sizeof(float);

    float *pWeights = static_cast<float*>WPFAlloc(ProcessHeap, Mt(CMilBlurEffectDuce), size);
    IFCOOM(pWeights);

    CalculateSamplingWeights(radius, &pWeights, MilKernelType::Gaussian);

    float *pSamplingWeights = *ppSamplingWeightsReplicate;

    for (UINT i = 0; i <= radius; i++)
    {        
        pSamplingWeights[radius-i] = pWeights[i];
        pSamplingWeights[i+radius] = pWeights[i];
    }

Cleanup:
    if (pWeights)
    {
        WPFFree(ProcessHeap, pWeights);
    }
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::CalculateSamplingWeights
//
// Synopsis: 
//      Fills the array with sampling weights. Produces an array of radius + 1 
//      weights. (center weight + weights for 1 direction). Works for box or
//      Gaussian blur
//
//-----------------------------------------------------------------------------
void
CMilBlurEffectDuce::CalculateSamplingWeights(
    __in UINT radius,
    __deref_out_xcount(radius+1) float **ppSamplingWeights,
    __in MilKernelType::Enum kernelType
    )
{

    // Future Consideration:  We might want to cache the weight calculation in the future
    //                  since all weights are recalculated per-render now.
    float *pSamplingWeights = *ppSamplingWeights;
    Assert(pSamplingWeights != NULL);

    double sum = 0.0;
    double difference = 0.0;
    
    switch(kernelType)
    {
        case MilKernelType::Gaussian:
            // We need to calculate each weight for the Gaussian filter.
            for (UINT i = 0; i < radius+1; i++)
            {
                // Choosing a standard deviation of 1/3rd the radius is standard for a discrete
                // approximation of the gaussian function.
                double sd = radius / 3.0;
                double ind = static_cast<double>(i);
                double weight = (1.0 / (sd * sqrt(TWO_PI))) * exp(- (ind * ind) / (2.0 * sd * sd));
                // Sum the weights as we go so we can normalize them at the end to ensure conservation of intensity.
                if (i == 0)
                {
                    sum += static_cast<float>(weight);
                }
                else
                {
                    sum += static_cast<float>(weight);
                    sum += static_cast<float>(weight);
                }
                pSamplingWeights[i] = static_cast<float>(weight);
            }
            break;
            
        case MilKernelType::Box: 
            {
                // In a box filter, all the weights are equal.
                double boxWeight = 1.0 / (2*radius + 1);
                for (UINT i = 0; i < radius+1; i++)
                {
                    pSamplingWeights[i] = static_cast<float>(boxWeight);
                }
                // Sum the weights as we go so we can normalize them at the end to ensure conservation of intensity.
                sum = 2*radius*boxWeight + boxWeight;  // 2*radius + 1 identical weights
            }
            break;
            
        default: 
            // We should always have a valid KernelType.
            Assert(false);
    }

    // Normalize the weights so they add as closely to 1 as we can to account for rounding/floating point error.
    // If the weights do not add nearly to one then the image will gain or lose intensity.
    difference = (1-sum) / (2.0*radius+1.0);
    for (UINT j = 0; j < radius+1; j++)
    {
        pSamplingWeights[j] += static_cast<float>(difference);
    }

}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::ClearMarginPixels
//
// Synopsis:
//      Set the margins of a buffer to a value
//
// Example:
//      For width = 10, height = 5, leftMargin = 2, topMargin = 2, rightMargin = 3, bottomMargin = 1
//
//      x x x x x x x x x x
//      x x x x x x x x x x
//      x x 0 0 0 0 0 x x x
//      x x 0 0 0 0 0 x x x
//      x x x x x x x x x x
//
//      We need to set all the elements labeled x to 0
//
//-----------------------------------------------------------------------------

HRESULT 
CMilBlurEffectDuce::ClearMarginPixels(
                        __inout_ecount(width*height) UINT *pStart, 
                        UINT width, 
                        UINT height,
                        UINT leftMargin,
                        UINT topMargin,
                        UINT rightMargin,
                        UINT bottomMargin
                        )
{
    HRESULT hr = S_OK;    

    if ((leftMargin + rightMargin > width) ||
        (topMargin + bottomMargin > height))
    {
        IFC(E_INVALIDARG);
    }

    UINT *pCurrent = pStart;
    UINT clearSize, clearElements;
    
    // Do top rows
    IFC(UIntMult(width, topMargin, &clearElements));
    IFC(UIntMult(sizeof(UINT), clearElements, &clearSize));
    ZeroMemory(pCurrent, clearSize);

    // Do middle rows
    pCurrent = pStart + clearElements;

    UINT leftMarginSize;
    IFC(UIntMult(sizeof(UINT), leftMargin, &leftMarginSize));
    UINT rightMarginSize;
    IFC(UIntMult(sizeof(UINT), rightMargin, &rightMarginSize));    
 
    UINT distanceToRightMarginStart = width - rightMargin;    
    if ((leftMargin > 0) || (rightMargin > 0))
    {
        for (UINT j = 0; j < height - (topMargin + bottomMargin); j++)
        {
            ZeroMemory(pCurrent, leftMarginSize);
            ZeroMemory(pCurrent + distanceToRightMarginStart, rightMarginSize);
            pCurrent += width;
        }
    }

    // Do bottom rows
    pCurrent = pStart + width * (height - bottomMargin);
    IFC(UIntMult(width, bottomMargin, &clearElements));
    IFC(UIntMult(sizeof(UINT), clearElements, &clearSize));
    ZeroMemory(pCurrent, clearSize);    

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::ApplyEffect
//
// Synopsis: 
//      Renders the blur effect into the currently selected render target
//      on the device.  SourceRTNoRef may be overwritten.
//
//-----------------------------------------------------------------------------

HRESULT
CMilBlurEffectDuce::ApplyEffect(
    __in CContextState *pContextState, 
    __in CHwSurfaceRenderTarget *pDestRT,
    __in CMILMatrix *pScaleTransform,
    __in CD3DDeviceLevel1 *pDevice, 
    UINT uIntermediateWidth,
    UINT uIntermediateHeight,
    __in_opt CHwTextureRenderTarget *pImplicitInput
    )
{
    if (pImplicitInput == NULL)
    {
        FreAssert(FALSE);
        RRETURN(WGXERR_INTERNALERROR);
    }
    else
    {
        RRETURN(ApplyEffectImpl(pContextState, pScaleTransform, pDevice, uIntermediateWidth, uIntermediateHeight, pImplicitInput, pDestRT, NULL));
    }
}

//+----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::ApplyEffectSw
//
// Synopsis: Apply blur using SSE2
//
//-----------------------------------------------------------------------------

HRESULT
CMilBlurEffectDuce::ApplyEffectSw(
    __in CContextState *pContextState,
    __in CSwRenderTargetSurface *pDestRT,
    __in CMILMatrix *pScaleTransform,
    UINT uIntermediateWidth,
    UINT uIntermediateHeight,
    __in_opt IWGXBitmap *pImplicitInput
    )
{
    HRESULT hr = S_OK;

    bool fPushedInterpolationMode = false;
    MilBitmapInterpolationMode::Enum interpolationModeBackup = MilBitmapInterpolationMode::NearestNeighbor;

    CMILBrushBitmap *pBrushBitmap = NULL;

    CSystemMemoryBitmap *pIntermediateBitmap = NULL;
    IWGXBitmapLock *pImplicitInputLock = NULL;
    BYTE *pInputBuffer = NULL;    
   
    IWGXBitmapLock *pIntermediateBitmapLock = NULL;
    BYTE *pIntermediateBuffer = NULL;

     // BlurEffect requires that the implicit input be realized.
    Assert(pImplicitInput);
    
    // pARGB input buffer and size
    UINT inputBufferSize = 0;
    WICRect lockRect = { 0, 0, uIntermediateWidth, uIntermediateHeight };
    
    //
    // We acquire a lock for the whole surface. If dirty regions are being used, the implicit
    // input will be appropriately sized to the dirty region, so we always want access to blur
    // the whole size.
    //
    IFC(pImplicitInput->Lock(&lockRect, MilBitmapLock::Read, &pImplicitInputLock));

    IFC(pImplicitInputLock->GetDataPointer(&inputBufferSize, reinterpret_cast<byte**>(&pInputBuffer)));                                 

    {
        CRectangle rectangle;

        CRectF<CoordinateSpace::BaseSampling> rectangleShapeRect(0, 0, static_cast<float>(uIntermediateWidth), static_cast<float>(uIntermediateHeight), XYWH_Parameters);
        rectangle.Set(rectangleShapeRect, 0.0f );

        // The pass information for the current radius. 
        UINT radius = 0;
        GetScaledRadius(pScaleTransform, &radius);

        //
        // We assume that the surface has already been inflated by the radius, and the source was originally
        // at least 1x1 size.
        //
        Assert(uIntermediateWidth >= 2 * radius + 1);
        Assert(uIntermediateHeight >= 2 * radius + 1);

        if (radius == 0)
        {            
            // Handle no blur case as a pass through.
            LocalMILObject<CImmediateBrushRealizer> immediateRealizer;
            
            IFC(CMILBrushBitmap::Create(&pBrushBitmap));
            IFC(pBrushBitmap->SetBitmap(pImplicitInput));

            immediateRealizer.SetMILBrush(pBrushBitmap, NULL, true /* skip meta fix ups */);
            
            IFC(pDestRT->DrawPath(pContextState, NULL, &rectangle, NULL, NULL, &immediateRealizer));

            ReleaseInterface(pBrushBitmap);
            ReleaseInterface(pImplicitInputLock);
        }
        else
        {
            MilPixelFormat::Enum pxlFormat;
            IFC(pImplicitInput->GetPixelFormat(&pxlFormat));

            IFC(CSystemMemoryBitmap::Create(
                uIntermediateWidth,
                uIntermediateHeight,
                pxlFormat,
                /* fClear = */ TRUE,       // This probably isn't necessary
                /* fDynamic = */ FALSE,
                &pIntermediateBitmap
                ));

            // Output lock
            MILRect outputLock = {0, 0, uIntermediateWidth, uIntermediateHeight};
            IFC(pIntermediateBitmap->Lock(&outputLock, MilBitmapLock::Write, &pIntermediateBitmapLock));

            // Output buffer pointer            
            UINT intermediateBufferSize = 0;
            IFC(pIntermediateBitmapLock->GetDataPointer(&intermediateBufferSize, &pIntermediateBuffer));

            switch (m_data.m_KernelType)
            {
                case MilKernelType::Box:
                {
                    IFC(ApplyBoxBlurSw(pInputBuffer,
                                       pIntermediateBuffer,
                                       uIntermediateWidth,
                                       uIntermediateHeight,
                                       radius
                                       ));
                }
                break;

                case MilKernelType::Gaussian:
                {
                    IFC(ApplyGaussianBlurSw(pInputBuffer,
                                            pIntermediateBuffer,
                                            uIntermediateWidth,
                                            uIntermediateHeight,
                                            radius
                                            ));
                }
                break;

                default:
                    AssertMsg(false, "CMILBlurEffectDuce: Unrecognized kernel type");
                    IFC(E_INVALIDARG);
                    break;                                            
            }

            ReleaseInterface(pIntermediateBitmapLock);               
            ReleaseInterface(pImplicitInputLock);

            //
            // For box, output is in pIntermediateBitmap
            // For gaussian, it's 2 pass so it's back in the source, pImplicitInput
            //

            //
            // If we are rotated, we need to turn on bilinear sampling for drawing the
            // bitmap or we get ugly aliased edges.
            //
            if (!pContextState->WorldToDevice.IsTranslateOrScale())
            {
                interpolationModeBackup = pContextState->RenderState->InterpolationMode;
                pContextState->RenderState->InterpolationMode = MilBitmapInterpolationMode::Linear;
                fPushedInterpolationMode = true;
            }                    
            
            IFC(pDestRT->DrawBitmap(pContextState, 
                                    (m_data.m_KernelType == MilKernelType::Box) ? pIntermediateBitmap : pImplicitInput, 
                                    NULL
                                    ));                       

        }
    }   
    
Cleanup:
    if (fPushedInterpolationMode)
    {
        pContextState->RenderState->InterpolationMode = interpolationModeBackup;
    }

    ReleaseInterface(pImplicitInputLock);
    ReleaseInterface(pIntermediateBitmapLock);
    ReleaseInterface(pBrushBitmap);
    ReleaseInterface(pIntermediateBitmap);

    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::ApplyGaussianBlurSw
//
// Synopsis: 
//          Applies a 2 pass Gaussian blur and places the result in 
//          pInputOutputBuffer. pIntermediateBuffer is used for intermediate
//          staging.
//          Assumes that sourceWidth > 2 * radius + 1 and 
//          sourceHeight > 2 * radius + 1
//-----------------------------------------------------------------------------
HRESULT
CMilBlurEffectDuce::ApplyGaussianBlurSw(__in_ecount(sourceWidth * sourceHeight * 4) BYTE * pInputOutputBuffer,
                                        __in_ecount(sourceWidth * sourceHeight * 4) BYTE * pIntermediateBuffer,
                                        UINT sourceWidth,
                                        UINT sourceHeight,
                                        UINT radius
                                        )
{
    HRESULT hr = S_OK;
    
    if (s_pfnBlurFunctionGaussian == NULL)
    {
        IFC(InitializeBlurFunction(true, false, &s_pfnBlurFunctionGaussian));
        Assert(s_pfnBlurFunctionGaussian);
    }

    float *pGaussianWeights = reinterpret_cast<float*>WPFAlloc(ProcessHeap, Mt(CMilBlurEffectDuce), (sizeof(float) * (2*radius + 1)));
    IFCOOM(pGaussianWeights);

    CalculateGaussianSamplingWeightsFullKernel(radius, &pGaussianWeights);

    // Do vertical pass from source into intermediate
    BYTE *pPassInputBuffer = pInputOutputBuffer;
    BYTE *pPassOutputBuffer = pIntermediateBuffer;

    // Clear top and bottom rows since the vertical blur pass won't fill them.
    IFC(ClearMarginPixels(reinterpret_cast<UINT*>(pIntermediateBuffer), sourceWidth, sourceHeight, 0, radius, 0, radius));

    //
    // Adjust output buffer to location of first output pixel, for vertical pass, this
    // is the leftmost pixel radius lines down
    //
    pPassOutputBuffer += (sourceWidth * 4 * radius);

    GenerateColorsBlurParams arguments;
    arguments.pargbSource = reinterpret_cast<unsigned *>(pPassInputBuffer);
    arguments.pargbDestination = reinterpret_cast<unsigned *>(pPassOutputBuffer);
    arguments.sourceWidth = sourceWidth;
    arguments.radius = radius;  
    arguments.nOutputPixelsPerLine = sourceWidth;
    arguments.nOutputLines = sourceHeight - 2 * radius;
    arguments.pBoxBlurLineBuffer = NULL;
    arguments.boxBlurLineBufferLength = 0;
    arguments.pGaussianWeights = pGaussianWeights;
    arguments.vertical = 1;

    (*s_pfnBlurFunctionGaussian)(&arguments);

    // Do horizontal pass from intermediate back into source
    pPassInputBuffer = pIntermediateBuffer;
    pPassOutputBuffer = pInputOutputBuffer;

    //
    // Adjust output buffer to location of first output pixel, for horizontal pass, this
    // is the top line pixel, radius in from the edge
    //
    pPassOutputBuffer += (radius * 4);

    arguments.pargbSource = reinterpret_cast<unsigned *>(pPassInputBuffer);
    arguments.pargbDestination = reinterpret_cast<unsigned *>(pPassOutputBuffer);
    arguments.sourceWidth = sourceWidth;
    arguments.radius = radius;  
    arguments.nOutputPixelsPerLine = sourceWidth - radius * 2;
    arguments.nOutputLines = sourceHeight;
    arguments.pBoxBlurLineBuffer = NULL;
    arguments.boxBlurLineBufferLength = 0;
    arguments.pGaussianWeights = pGaussianWeights;
    arguments.vertical = 0;

    (*s_pfnBlurFunctionGaussian)(&arguments);
    
    WPFFree(ProcessHeap, pGaussianWeights);
    
Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::ApplyBoxBlurSw
//
// Synopsis: 
//          Applies a box blur and places the result in pOutputBuffer.
//          Assumes that sourceWidth > 2 * radius + 1 and 
//          sourceHeight > 2 * radius + 1
//          Also assumes that pInputBuffer and pOutputBuffer are same dimensions
//-----------------------------------------------------------------------------
HRESULT
CMilBlurEffectDuce::ApplyBoxBlurSw(__in_ecount(sourceWidth * sourceHeight * 4) BYTE * pInputBuffer,
                                   __in_ecount(sourceWidth * sourceHeight * 4) BYTE * pOutputBuffer,
                                   UINT sourceWidth,
                                   UINT sourceHeight,
                                   UINT radius)
{
    HRESULT hr = S_OK;
    
    if (s_pfnBlurFunctionBox == NULL)
    {
        IFC(InitializeBlurFunction(false, false, &s_pfnBlurFunctionBox));
        Assert(s_pfnBlurFunctionBox);
    }                        

    //
    // Need a buffer aligned to 16 byte boundary for SSE2 load/save operations,
    // so make sure there's space in allocation to align the pointer.
    //
    UINT alignedAllocationSize = (sourceWidth +1) * sizeof(u32x4);

    if (!m_pBoxBlurLineBuffer || (m_boxBlurLineBufferSize < alignedAllocationSize))
    {
        if (m_pBoxBlurLineBuffer)
        {
            WPFFree(ProcessHeap, m_pBoxBlurLineBuffer);
        }

        m_pBoxBlurLineBuffer = (BYTE *)WPFAlloc(ProcessHeap,
                                               Mt(BoxBlurLineBuffer),
                                               alignedAllocationSize);

        IFCOOM(m_pBoxBlurLineBuffer);

        m_boxBlurLineBufferSize = alignedAllocationSize;
    }

    // Clear the output pixels that the blur won't produce.
    IFC(ClearMarginPixels(reinterpret_cast<UINT*>(pOutputBuffer), sourceWidth, sourceHeight, radius, radius, radius, radius));
        
    void *pBoxBlurLineBufferAligned = reinterpret_cast<void *>(IncrAlignTo(reinterpret_cast<uintptr_t>(m_pBoxBlurLineBuffer), SSE2_ALIGNMENT_BOUNDARY));

    //
    // Adjust output buffer to location of first output pixel. This is at position (radius, radius),
    // since we know the input has been inflated by the radius beyond the output size.
    //
    pOutputBuffer += (sourceWidth * radius * 4) + (radius * 4);

    GenerateColorsBlurParams arguments;
    arguments.pargbSource = reinterpret_cast<unsigned *>(pInputBuffer);
    arguments.pargbDestination = reinterpret_cast<unsigned *>(pOutputBuffer);
    arguments.sourceWidth = sourceWidth;
    arguments.radius = radius;  
    arguments.nOutputPixelsPerLine = sourceWidth - radius * 2;
    arguments.nOutputLines = sourceHeight - 2 * radius;
    arguments.pBoxBlurLineBuffer = pBoxBlurLineBufferAligned;
    arguments.boxBlurLineBufferLength = sourceWidth;
    arguments.pGaussianWeights = NULL;
    arguments.vertical = 1;

    (*s_pfnBlurFunctionBox)(&arguments);

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::ApplyEffectInPipeline
//
// Synopsis: 
//      Renders the blur effect into the currently selected render target
//      on the device.  This public method allows a blur effect to be
//      rendered into an intermediate texture in another effect's ApplyEffect
//      pipeline.  This method guarantees that SourceRTNoRef is not overwritten.
//-----------------------------------------------------------------------------

HRESULT
CMilBlurEffectDuce::ApplyEffectInPipeline(
    __in const CContextState *pContextState, 
    __in const CMILMatrix *pScaleTransform,
    __in CD3DDeviceLevel1 *pDevice,
    UINT uIntermediateWidth,
    UINT uIntermediateHeight,
    __in CHwTextureRenderTarget *pSourceRT, 
    __in CD3DVidMemOnlyTexture *pDestRT
    )
{
   RRETURN(ApplyEffectImpl(pContextState, pScaleTransform, pDevice, uIntermediateWidth, uIntermediateHeight, pSourceRT, NULL, pDestRT));
}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::ApplyEffectImpl
//
// Synopsis: 
//      Renders the blur effect into the currently selected render target
//      on the device.  SourceRTNoRef may be overwritten.
//      If pFinalDestRT is populated, we're rendering a blur effect into the
//      final destination texture to be blended into the back buffer.
//      If pPipelineDestRT is populated, we're rendering into an intermediate
//      for another Effect's pipeline.
//
//-----------------------------------------------------------------------------

HRESULT
CMilBlurEffectDuce::ApplyEffectImpl(
    __in const CContextState *pContextState,
    __in const CMILMatrix *pScaleTransform,
    __in CD3DDeviceLevel1 *pDevice, 
    UINT uIntermediateWidth,
    UINT uIntermediateHeight,
    __in CHwTextureRenderTarget *pSourceRTNoRef, 
    __in_opt CHwSurfaceRenderTarget *pFinalDestRT,
    __in_opt CD3DVidMemOnlyTexture *pPipelineDestRT
    )
{
    HRESULT hr = S_OK;

    // The source texture and surface
    CD3DVidMemOnlyTexture* pTextureNoRef_A = NULL;
    CD3DSurface* pSurface_A = NULL;

    // The first internal intermediate texture and surface
    CD3DVidMemOnlyTexture* pTexture_B = NULL;
    CD3DSurface* pSurface_B = NULL;

    // The second internal intermediate texture and surface
    CD3DVidMemOnlyTexture* pTexture_C = NULL;
    CD3DSurface* pSurface_C = NULL;
    
    CD3DSurface* pPipelineDestSurface = NULL;

    float* pSamplingWeights = NULL;

    // Clear to transparent black.
    MilColorB colBlank = 0;

    // When drawing to the back buffer, we support either nearest-neighbor or
    // bilinear sampling.  We don't support Fant interpolation.  If we are
    // rotated, we force bilinear sampling to reduce aliasing artifacts.
    bool useLinearSampling = 
        pContextState->RenderState->InterpolationMode != MilBitmapInterpolationMode::NearestNeighbor ||
        !pContextState->WorldToDevice.IsTranslateOrScale();

    // BlurEffect requires that the implicit input be realized.
    Assert(pSourceRTNoRef);
    
    pTextureNoRef_A = pSourceRTNoRef->GetTextureNoRef();
    Assert(pTextureNoRef_A != NULL);

    // Ensure we've been passed exactly one destination argument.
    if ((pFinalDestRT != NULL && pPipelineDestRT != NULL) ||
        (pFinalDestRT == NULL && pPipelineDestRT == NULL))
    {
        // Since this method is only used internally, this should never occur.
        Assert(false);
        IFC(E_FAIL);
    }
    
    if (!pSourceRTNoRef->IsValid())
    {
        goto Cleanup;
    }

    IFC(pTextureNoRef_A->GetD3DSurfaceLevel(0, &pSurface_A));

    if (pPipelineDestRT != NULL)
    {
        pPipelineDestRT->GetD3DSurfaceLevel(0, &pPipelineDestSurface);
    }
    
    //
    // Prepare our device for running an effect with the shader pipeline.
    IFC(pDevice->PrepareShaderEffectPipeline(false /* use vs_2_0 */));

    // Ensure the address mode is set to clamp for both samplers used by the blur shader.
    // Set the sampling mode to nearest neighbor for all intermediate passes.
    for (int i = 0; i < 2; i += 1)
    {
        IFC(SetSamplerState(
            pDevice, 
            i, 
            true,     // set address mode to clamp
            false));  // use nearest neighbor
    }

    //
    // The pass information for the current radius. 
    UINT radius = 0;
    GetScaledRadius(pScaleTransform, &radius);

    // If the blur radius is zero, we skip the loop below and we just need to render the source into
    // the final destination texture with a pass-through shader.
    if (radius == 0) 
    {
        // Use the original source texture
        IFC(pDevice->SetTexture(0, pTextureNoRef_A));

        // We ensure state and set up the world transform if we're rendering into the final destination, 
        // or just set the render target if we're rendering into another effect's pipeline.
        if (pFinalDestRT != NULL)
        {
            IFC(SetupVertexTransform(
                pContextState, 
                pDevice, 
                static_cast<float>(uIntermediateWidth), 
                static_cast<float>(uIntermediateHeight), 
                true /* populate for rendering into the final destination */));

            // We used nearest-neighbor sampling for the intermediate surfaces,
            // now switch to linear if appropriate for the final pass.
            if (useLinearSampling)
            {
                IFC(SetSamplerState(
                    pDevice, 
                    0, 
                    false,   // don't set the address mode again
                    true));  // use bilinear
            }
            
            IFC(pFinalDestRT->EnsureState(pContextState));
        }
        else
        {
            Assert(pPipelineDestSurface != NULL);
            // Our destRT is just another intermediate
            IFC(SetupVertexTransform(
                pContextState, 
                pDevice, 
                static_cast<float>(uIntermediateWidth), 
                static_cast<float>(uIntermediateHeight), 
                false /* populate for rendering into an intermediate */));
            IFC(pDevice->SetRenderTargetForEffectPipeline(pPipelineDestSurface));
            IFC(pDevice->Clear(0, NULL, D3DCLEAR_TARGET, colBlank, 0, 0));
        }
        
        // Draw our final result
        IFC(pDevice->SetPassThroughPixelShader());
        IFC(pDevice->SetAlphaBlendMode(&CD3DRenderState::sc_abmSrcOverPremultiplied));
        IFC(pDevice->DrawTriangleStrip(0, 2));
    }
    // If the RenderingBias is Quality, we execute the blur with two intermediate FP textures.  
    // The effect is executed as follows:
    //      1) For each horizontal pass, render into one intermediate, then ping-pong and
    //         render from that intermediate into the other on the next pass.  We need to do this since
    //         alpha-blending is not supported for floating-point textures, and we need the precision to
    //         achieve the desired visual effect.  The source texture is not floating point so we cannot
    //         render back into it.
    //      2) a.  If we're in another effect's pipeline, render our new horizontally blurred texture into the
    //             destination intermediate texture.  This will become our new "source" for vertical passes.
    //         b.  Otherwise, render back into the source texture, which remains our source for vertical passes.
    //      3) Repeat 1) for the vertical passes.
    //      4) Run one final pass with the pass through shader to apply the world transform (if we're not
    //         in another effect's pipeline) and render back into the destination (A8R8G8B8) texture.
    else if (m_data.m_RenderingBias == MilEffectRenderingBias::Quality && 
            pDevice->Is128BitFPTextureSupported())
    {
        IFC(CreateIntermediateRT(
            pDevice, 
            static_cast<UINT>(uIntermediateWidth), 
            static_cast<UINT>(uIntermediateHeight), 
            D3DFMT_A32B32G32R32F,
            &pTexture_B));

        Assert(pTexture_B != NULL);
        IFC(pTexture_B->GetD3DSurfaceLevel(0, &pSurface_B));

        IFC(CreateIntermediateRT(
            pDevice, 
            static_cast<UINT>(uIntermediateWidth), 
            static_cast<UINT>(uIntermediateHeight), 
            D3DFMT_A32B32G32R32F,
            &pTexture_C));

        Assert(pTexture_C != NULL);
        IFC(pTexture_C->GetD3DSurfaceLevel(0, &pSurface_C));

        // Populate vertex buffer for all intermediate passes.     
        IFC(SetupVertexTransform(
            pContextState, 
            pDevice, 
            static_cast<float>(uIntermediateWidth), 
            static_cast<float>(uIntermediateHeight), 
            false /* populate for rendering into intermediates */));

        // Calculate the weights for each sample.  Since sample x and -x have the same weight
        // for both Gaussian and Box kernels, we need an array of size radius+1 (the additional 1
        // being the center sample which is not duplicated).
        pSamplingWeights = new float[radius+1];
        CalculateSamplingWeights(radius, &pSamplingWeights, m_data.m_KernelType);

        // 
        // 1) Horizontal passes
        //
            
        // The first sampler is bound to the source texture throughout all horizontal passes.
        IFC(pDevice->SetTexture(0, pTextureNoRef_A));

        IFC(ExecutePasses(
            pDevice, 
            true /* horizontal */, 
            true /* quality */,
            radius, 
            static_cast<float>(uIntermediateWidth) /* size = width */, 
            pSamplingWeights, 
            pTextureNoRef_A, 
            pTexture_B,
            pSurface_B,
            pTexture_C,
            pSurface_C));
        
        // 
        // 2) Fill our new "source" texture from the horizontally blurred intermediate.
        //
        
        // If we're rendering into a final destination, we can overwrite the source at this point.
        if (pFinalDestRT != NULL)
        {
            // We'll render back into our source texture
            IFC(pDevice->SetRenderTargetForEffectPipeline(pSurface_A));
        }
        else
        // If we're rendering into another effect's pipeline, we can't overwrite the source, but we
        // can use our destination intermediate.
        {
            Assert(pPipelineDestSurface != NULL);
            // We'll render directly into our second intermediate texture
            IFC(pDevice->SetRenderTargetForEffectPipeline(pPipelineDestSurface));               
        }
        
        IFC(pDevice->Clear(0, NULL, D3DCLEAR_TARGET, colBlank, 0, 0));

        // Draw horizontal result to new source.  We set the source texture in ExecutePasses.
        IFC(pDevice->SetPassThroughPixelShader());
        IFC(pDevice->SetAlphaBlendMode(&CD3DRenderState::sc_abmSrcOverPremultiplied));
        IFC(pDevice->DrawTriangleStrip(0, 2));
        
        //
        // 3) Vertical passes
        //

        // The first sampler is bound to the source texture throughout all horizontal passes.
        if (pFinalDestRT != NULL)
        {
            // We rendered back into our source texture
            IFC(pDevice->SetTexture(0, pTextureNoRef_A));
        }
        else
        {
            Assert(pPipelineDestSurface != NULL);
            // We rendered directly into our destination intermediate.
            IFC(pDevice->SetTexture(0, pPipelineDestRT));              
        }

        IFC(ExecutePasses(
            pDevice, 
            false /* vertical */, 
            true /* quality */,
            radius, 
            static_cast<float>(uIntermediateHeight) /* size = height */,
            pSamplingWeights, 
            pTextureNoRef_A, 
            pTexture_B,
            pSurface_B,
            pTexture_C,
            pSurface_C));

        //
        // 4) Final Pass
        //

        // If we're rendering into a final destination texture and not in another effect's pipeline, we need
        // to ensure the rendering state and set up the world transform.  In both cases we need to render
        // into the destination texture.  We set the source texture in ExecutePasses.
        if (pFinalDestRT != NULL)
        {
            IFC(SetupVertexTransform(
                pContextState, 
                pDevice, 
                static_cast<float>(uIntermediateWidth), 
                static_cast<float>(uIntermediateHeight), 
                true /* populate for rendering into the final destination */));

            // We used nearest-neighbor sampling for the intermediate surfaces,
            // now switch to linear if appropriate for the final pass.
            if (useLinearSampling)
            {
                for (int i = 0; i < 2; i += 1)
                {
                    IFC(SetSamplerState(
                        pDevice, 
                        i, 
                        false,   // don't set the address mode again
                        true));  // use bilinear
                }
            }
            
            IFC(pFinalDestRT->EnsureState(pContextState));
        }
        else
        {           
            Assert(pPipelineDestSurface != NULL);
            // We'll render directly into our destination intermediate texture
            IFC(pDevice->SetRenderTargetForEffectPipeline(pPipelineDestSurface));  
            // We need to clear this since we've drawn an intermediate result into it.
            IFC(pDevice->Clear(0, NULL, D3DCLEAR_TARGET, colBlank, 0, 0));
        }

        // Draw final result
        IFC(pDevice->SetPassThroughPixelShader());
        IFC(pDevice->SetAlphaBlendMode(&CD3DRenderState::sc_abmSrcOverPremultiplied));
        IFC(pDevice->DrawTriangleStrip(0, 2));

    }
    // If the RenderingBias is Performance, we can execute the blur with one intermediate texture
    // and only two render target switches.  The effect is executed as follows:
    //      1) Draw all horizontal passes into an intermediate.
    //      2) a.  If we're in another effects pipeline, draw all vertical passes directly into the destination
    //             intermediate.
    //         b.  Otherwise, draw back into the source texture, since we no longer need it.  Then run one
    //             final pass with the pass through shader to apply the world transform and render into 
    //             the destination texture.
    else
    {
        IFC(CreateIntermediateRT(
            pDevice, 
            static_cast<UINT>(uIntermediateWidth), 
            static_cast<UINT>(uIntermediateHeight), 
            D3DFMT_A8R8G8B8,
            &pTexture_B));

        Assert(pTexture_B != NULL);
        IFC(pTexture_B->GetD3DSurfaceLevel(0, &pSurface_B));
        
        // Populate vertex buffer for all intermediate passes.     
        IFC(SetupVertexTransform(
            pContextState, 
            pDevice, 
            static_cast<float>(uIntermediateWidth), 
            static_cast<float>(uIntermediateHeight), 
            false /* populate for rendering into intermediates */));

        // Calculate the weights for each sample.  Since sample x and -x have the same weight
        // for both Gaussian and Box kernels, we need an array of size radius+1 (the additional 1
        // being the center sample which is not duplicated).
        pSamplingWeights = new float[radius+1];
        CalculateSamplingWeights(radius, &pSamplingWeights, m_data.m_KernelType);

        // On the first pass, we set up the rendering state for all horizontal passes.

        // Set texture to source texture
        IFC(pDevice->SetTexture(0, pTextureNoRef_A));
        IFC(pDevice->SetRenderTargetForEffectPipeline(pSurface_B));

        // Set the blend mode to add.  The samples from each horizontal pass will
        // be added into the intermediate.
        IFC(pDevice->SetAlphaBlendMode(&CD3DRenderState::sc_abmAddSourceColor));

        IFC(pDevice->Clear(0, NULL, D3DCLEAR_TARGET, colBlank, 0, 0));

        //
        // Execute horizontal passes
        IFC(ExecutePasses(
            pDevice, 
            true /* horizontal */, 
            false /* performance */,
            radius, 
            static_cast<float>(uIntermediateWidth) /* size = width */, 
            pSamplingWeights, 
            pTextureNoRef_A, 
            pTexture_B,
            pSurface_B,
            NULL, /* the second intermediate is not created for performance passes */
            NULL));

        // If we're rendering into a final destination, we can overwrite the source at this point.
        // We're using texture B as our source (set in ExecutePasses).
        if (pFinalDestRT != NULL)
        {
            // We'll render back into our source texture
            IFC(pDevice->SetRenderTargetForEffectPipeline(pSurface_A));
        }
        else
        // If we're rendering into another effect's pipeline, we can't overwrite the source, but
        // since we don't need to apply the world transform at the end we can simply write all
        // vertical passes into the destination intermediate.
        {
            Assert(pPipelineDestSurface != NULL);
            // We'll render directly into our second intermediate texture
            IFC(pDevice->SetRenderTargetForEffectPipeline(pPipelineDestSurface));               
        }

        IFC(pDevice->Clear(0, NULL, D3DCLEAR_TARGET, colBlank, 0, 0));

        //
        // Execute vertical passes        
        IFC(ExecutePasses(
            pDevice, 
            false /* vertical */, 
            false /* performance */,
            radius, 
            static_cast<float>(uIntermediateHeight) /* size = height */, 
            pSamplingWeights, 
            pTextureNoRef_A, 
            pTexture_B,
            pSurface_B,
            NULL, /* the second intermediate is not created for performance passes */
            NULL));

        
        // Reset the blend mode since we're done blurring.
        IFC(pDevice->SetAlphaBlendMode(&CD3DRenderState::sc_abmSrcOverPremultiplied));  

        // If we're rendering into a final destination texture and not in another effect's pipeline, we need
        // to ensure the rendering state and set up the world transform.  If we're rendering into another
        // effects pipeline, we're done, since we rendered into the intermediate during the vertical passes.
        if (pFinalDestRT != NULL)
        {
            // Use the original source texture which now contains our fully blurred image
            IFC(pDevice->SetTexture(0, pTextureNoRef_A));
              
            IFC(SetupVertexTransform(
                pContextState, 
                pDevice, 
                static_cast<float>(uIntermediateWidth), 
                static_cast<float>(uIntermediateHeight), 
                true /* populate for rendering into the final destination */));

            // We used nearest-neighbor sampling for the intermediate surfaces,
            // now switch to linear if appropriate for the final pass.
            if (useLinearSampling)
            {
                for (int i = 0; i < 2; i += 1)
                {
                    IFC(SetSamplerState(
                        pDevice, 
                        i, 
                        false,   // don't set the address mode again
                        true));  // use bilinear
                }
            }
            
            IFC(pFinalDestRT->EnsureState(pContextState));

            // Draw final result
            IFC(pDevice->SetPassThroughPixelShader());
            IFC(pDevice->DrawTriangleStrip(0, 2));
             
        }

    }
        

Cleanup:
    if (pSamplingWeights != NULL)
    {
        delete [] pSamplingWeights;
        pSamplingWeights = NULL;
    }

    ReleaseInterface(pPipelineDestSurface);

    ReleaseInterface(pSurface_A);
    
    ReleaseInterface(pTexture_B);
    ReleaseInterface(pSurface_B);

    ReleaseInterface(pTexture_C);
    ReleaseInterface(pSurface_C);
    
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::ExecutePasses
//
// Synopsis: 
//    Executes a series of horizontal or vertical shader passes.
//    Sets the sampling source texture to the last intermediate rendered into,
//    which holds the result of the passes executed.
//
//-----------------------------------------------------------------------------

HRESULT
CMilBlurEffectDuce::ExecutePasses(
    __in CD3DDeviceLevel1 *pDevice, 
    __in bool isHorizontal,
    __in bool isQuality,
    __in UINT radius,
    __in float destinationSize,
    __in float* pSamplingWeights,
    __in CD3DVidMemOnlyTexture* pTextureNoRef_A,
    __in CD3DVidMemOnlyTexture* pTexture_B,
    __in CD3DSurface* pSurface_B,
    __in_opt CD3DVidMemOnlyTexture* pTexture_C,
    __in_opt CD3DSurface* pSurface_C
    )
{
    HRESULT hr = S_OK;
    
    MilColorB colBlank = 0;

    UINT samplesRemaining = 2*radius + 1;
    // The index of the first sample to take this pass, starting at -radius.
    // Ex. A radius of 10 would require 21 samples.  Our first pass would 
    // sample from  -10 to -1, the second from 0 to 9, and the last would 
    // take one sample for 10 (assuming MAX_SAMPLES_PER_PASS == 10).
    int sampleIndex = - static_cast<int>(radius);

    UINT passNumber = 1;
    
    //
    // Execute passes        
    while (samplesRemaining > 0)
    {
        // Calculate the number of samples to take this pass.
        // During this pass, we will take the following samples (cPassSamples total)
        //      sampleIndex, sampleIndex+1, ..., sampleIndex + (cPassSamples-1)
        UINT cPassSamples = 0;
        if (samplesRemaining > MAX_SAMPLES_PER_PASS)
        {
            cPassSamples = MAX_SAMPLES_PER_PASS;
            samplesRemaining -= MAX_SAMPLES_PER_PASS;
        }
        else
        {
            cPassSamples = samplesRemaining;
            samplesRemaining = 0;
        }

        // We have to ping-pong render targets for quality rendering.  For performance rendering we render
        // into the same intermediate each pass and use alpha blending to achieve the same result.
        if (isQuality)
        {
            // On odd passes, sample from C and draw into B. On even passes, sample from B and draw into C.
            IFC(pDevice->SetTexture(1, (passNumber%2 == 1) ? pTexture_C : pTexture_B));
            IFC(pDevice->SetRenderTargetForEffectPipeline((passNumber%2 == 1) ? pSurface_B : pSurface_C));

            IFC(pDevice->Clear(0, NULL, D3DCLEAR_TARGET, colBlank, 0, 0));
        }

        // We use the single input shaders for performance passes, and the first pass of quality passes.
        bool useMultiInputShader = (passNumber != 1) && isQuality;
        
        // Set the shader
        IFC(SetupShader(
            pDevice, 
            isHorizontal, 
            useMultiInputShader,
            destinationSize, 
            cPassSamples, 
            sampleIndex, 
            pSamplingWeights));

        IFC(pDevice->DrawTriangleStrip(0, 2));

         // Increment the sampleIndex for the next pass by adding the number we took on this pass.
        sampleIndex += cPassSamples;
        passNumber += 1; 
    }

    // The last intermediate drawn into contains the horizontally blurred image.
    // On odd quality passes, sample from C.  On even quality passes and
    // all performance passes, sample from B.
    bool useTextureCAsSource = isQuality && (passNumber%2 == 1);
    IFC(pDevice->SetTexture(0, (useTextureCAsSource) ? pTexture_C : pTexture_B));

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::TransformBoundsForInflation
//
// Synopsis: 
//    Called by the composition layer to account for potential bounds
//    transformations by  effects.  The blur effect inflates the bounds
//    in each direction by the blur radius.
//
//-----------------------------------------------------------------------------

HRESULT 
CMilBlurEffectDuce::TransformBoundsForInflation(__inout CMilRectF *bounds)
{
    HRESULT hr = S_OK;
    
    // If bounds are empty, there is no content to be rendered so we don't need to inflate.
    if (bounds->IsEmpty())
    {
        RRETURN(hr);
    }
    // Determine the current radius.
    float radius = static_cast<float>(GetRadius());
    bounds->Inflate(radius, radius);
    
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::GetLocalSpaceClipBounds
//
// Synopsis: 
//    Called by the composition layer to allow effects to be applied
//    to clipped areas.  The blur effect can be rendered into a clipped
//    subregion as long as we render into an inflated region to retain
//    sampling accuracy.
//
//-----------------------------------------------------------------------------

HRESULT 
CMilBlurEffectDuce::GetLocalSpaceClipBounds(
        __in CRectF<CoordinateSpace::LocalRendering> unclippedBoundsLocalSpace,
        __in CRectF<CoordinateSpace::PageInPixels> clip,
        __in const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *pWorldTransform,
        __out CRectF<CoordinateSpace::LocalRendering> *pClippedBoundsLocalSpace)
{
    HRESULT hr = S_OK;

    CRectF<CoordinateSpace::PageInPixels> boundsWorldSpace;
    CRectF<CoordinateSpace::LocalRendering> clippedBoundsLocalSpace;
    
    // First we clip the effect bounds in world space, then we invert back to local space.
    pWorldTransform->Transform2DBounds(unclippedBoundsLocalSpace, boundsWorldSpace);
    clip.Intersect(boundsWorldSpace);

    CMatrix<CoordinateSpace::PageInPixels,CoordinateSpace::LocalRendering> invWorldTransform;
    bool isInvertable = !!invWorldTransform.Invert(*pWorldTransform);
    
    // If we can't invert the world transform, just give up on clipping.
    if (!isInvertable)
    {
        *pClippedBoundsLocalSpace = unclippedBoundsLocalSpace;
        IFC(S_OK);
    }
   
    invWorldTransform.Transform2DBounds(clip, clippedBoundsLocalSpace);

    // We need to inflate the clipped local space bounds to draw so that when we 
    // apply the blur we can take samples beyond the edges of the clipped area.

    IFC(TransformBoundsForInflation(&clippedBoundsLocalSpace));

    *pClippedBoundsLocalSpace = clippedBoundsLocalSpace;

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::SetupShader
//
//    Selects the shader for the radius and pass type (horizontal and 
//    vertical pass).  Applies the shader constants.
//
// Arguments:
//    pDevice - valid pointer to a d3d device.
//    isHorizontalPass - determines if SetupShader is setting up a shader for
//              a horizontal or vertical pass.
//    isMultiInputPass - determines if SetupShader is setting up for a pass
//              that blends two inputs or not
//    destinationSize - width if horizontal, height if vertical of the
//              destination texture
//    cSamples - number of samples to take this pass
//    sampleIndex - kernel index to start taking samples from
//    arrSamplingWeights - array of weights for sampling
//        
//-----------------------------------------------------------------------------

HRESULT 
CMilBlurEffectDuce::SetupShader(
    __in CD3DDeviceLevel1 *pDevice, 
    bool isHorizontalPass,
    bool isMultiInputPass,
    float destinationSize,
    UINT cSamples,
    int samplingIndex,
    __in float *arrSamplingWeights)
{
    HRESULT hr = S_OK;
    CHwPixelShaderEffect *pHwPixelShaderEffect = NULL;

    // The shaders are assigned slots in the hw cache as follows:
    //   Slot    Shader
    //    0        Horizontal single-input
    //    1        Vertical single-input
    //    2        Horizontal multi-input
    //    3        Vertical multi-input
    //
    // This is the same scheme that is used to store the pointers for the
    // various shader byte codes in the s_pShaderByteCodes array.
    UINT shaderCacheSlot = isHorizontalPass ? 0 : 1;
    shaderCacheSlot = isMultiInputPass ? shaderCacheSlot + 2 : shaderCacheSlot;

    IFC(s_pBlurPixelShaders[shaderCacheSlot]->SetupShader(pDevice));

    //
    // Set up shader constants
    // Pixel shader registers are 4 floats, so we should fill them completely to prevent
    // arbitrary stack values being sent to the shader.
    
    // Set the texture size
    float arrSize[4] = { destinationSize, /* unused values */ 0.0f, 0.0f, 0.0f };
    IFC(pDevice->SetPixelShaderConstantF(0, arrSize, 1));

    // Set the number of samples and sampling index
    float arrSamplingIndex[4] = { static_cast<float>(samplingIndex), /* unused values */ 0.0f, 0.0f, 0.0f };
    IFC(pDevice->SetPixelShaderConstantF(1, arrSamplingIndex, 1));

    float arrcSamples[4] = { static_cast<float>(cSamples),  /* unused values */ 0.0f, 0.0f, 0.0f };
    IFC(pDevice->SetPixelShaderConstantF(2, arrcSamples, 1));

    float arrWeights[16];
    int weightIndex = samplingIndex;
    for (UINT i = 0; i < 16; i++)
    {
        if (i < cSamples)
        {
            // Sample -x has the same weight as sample x.
            arrWeights[i] = arrSamplingWeights[abs(weightIndex)];
            weightIndex += 1;
        }
        else
        {
            // Unused values.
            arrWeights[i] = 0;
        }
    }
    IFC(pDevice->SetPixelShaderConstantF(3, arrWeights, 4));

Cleanup:
    ReleaseInterface(pHwPixelShaderEffect);

    RRETURN(hr);
}


//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::InitializeBlurFunction
//
// SIMD JIT compiled implementation of Gaussian and box blur
//
// Arguments:
// fGaussian    - JIT compile time switch to determine whether to create a box
//                blur or Gaussian blur
//
//
// Algorithms:
//
// Gaussian blur:
//      Because Gaussian requires each sample to be weighted, it can't be implemented
//      incrementally as box blur can. It is separable though, so it's implemented as
//      a 1D blur that must be executed twice to produce a full blur. 
//      Complexity is O(radius * width * height). 
//      If it was non separable it would be O(radius^2 * width * height), so it could
//      be worse.
//
// Box blur:
//      Since the weights for the box blur are all equal, this blur can be implemented 
//      incrementally. The basic algorithm is to first produce a buffered row (of length sourceWidth) 
//      of values that are each sums of [sampleLength] elements of their respective column 
//      (for a particular sampleLength). To calculate the first pixel, the first [sampleLength] 
//      values (where sampleLength is the kernel size, and is equal to 2 * radius + 1) of the 
//      column sums are added, saved (as [totalSum]), then divided by [sampleLength^2] to produce a pixel value. 
//      To calculate the next pixel, [totalSum] can be taken, and the previous value in 
//      the column sums subtracted, and the next value added. After dividing this new sum, 
//      the next pixel result is obtained. This continues for the rest of the scanline.
//      Once we need to advance to the next scanline, the column sums must be recalculated. This
//      can also be done incrementally by adding and subtracting the next and last pixel input
//      values for each column. 
//      
//      Complexity is O(radius * width + width * height), which is ~= O(width * height) for
//      cases where height >> radius, which is going to be the usual case.
//        
//
// Master to do list:
// SSE4.1 optimizations for Gaussian blur (availability of integer multiply).
// Reordering of add/multiply for Gaussian blur to see what effect the int/float conversions
// have on performance.
// 
//-----------------------------------------------------------------------------


HRESULT
CMilBlurEffectDuce::InitializeBlurFunction(bool fGaussian, bool fColor, GenerateColorsBlur *pProgram)
{
    HRESULT hr = S_OK;
    
    BOOL     fEnteredJitter = FALSE;
    UINT8   *pBinaryCode    = NULL;

    // fColor necessarily implies fGaussian - only dropshadow uses fColor, and dropshadow doesn't use box.
    Assert(!fColor || fGaussian);

    // Start the JIT'er
    IFC(CJitterAccess::Enter(sizeof(GenerateColorsBlurParams*)));
    fEnteredJitter = TRUE;

    // Disable the use of negative stack offsets.  This will likely increase generated code
    // size, but is more compatible with debugging and profiling. 
    CJitterAccess::SetMode(CJitterAccess::sc_uidUseNegativeStackOffsets, 0);

    {
        C_pVoid pArguments = C_pVoid::GetpVoidArgument(0); // Get GenerateColorsBlurParams structure argument.
        
        //
        // Extract parameters from stack
        //

        //
        // Buffer pointers    
        // pOriginalSrc points to the start location where sampling should begin
        // pOriginalDst points to the start location for output pixels
        // Note that this means they will not point to the same x,y location within their respective buffers.
        //
        P_u32 pOriginalSrc = (pArguments.GetMemberPtr(OFFSET_OF(GenerateColorsBlurParams, pargbSource))).AsP_u32();
        P_u32 pOriginalDst = (pArguments.GetMemberPtr(OFFSET_OF(GenerateColorsBlurParams, pargbDestination))).AsP_u32();

        C_u32 sourceWidth = pArguments.GetMemberUINT32(OFFSET_OF(GenerateColorsBlurParams, sourceWidth));
        C_u32 radius = pArguments.GetMemberUINT32(OFFSET_OF(GenerateColorsBlurParams, radius));

        //
        // Output pixels per line and number of output lines. These should be sourceWidth - 2 *radius and
        // sourceHeight - 2 * radius respectively. This is not enforced, and no clamp checking is done when
        // sampling or writing as it causes significant perf degradation.
        //
        C_u32 uCountPerLine = pArguments.GetMemberUINT32(OFFSET_OF(GenerateColorsBlurParams, nOutputPixelsPerLine));
        C_u32 uCountLines = pArguments.GetMemberUINT32(OFFSET_OF(GenerateColorsBlurParams, nOutputLines));

        //
        // This is a buffer which is (sizeof(u32x4) * totalColumns) length. See box algorithm description for
        // details on how it is used. 
        //
        P_u32x4 pBoxBlurLineBuffer = (pArguments.GetMemberPtr(OFFSET_OF(GenerateColorsBlurParams, pBoxBlurLineBuffer))).AsP_u32x4();
        C_u32 boxBlurLineBufferLength = pArguments.GetMemberUINT32(OFFSET_OF(GenerateColorsBlurParams, boxBlurLineBufferLength));        

        //
        // Flags for Gaussian pass. pGaussianWeightsReplicate is an array whose length is (4 * 2 * radius + 1) and contains all
        // the weights for a single pass Gaussian blur. Each weight is replicated 4 times to avoid doing this replicate in the
        // sampling pass
        // verticalFlag indicates for a Gaussian pass whether this is a horizontal or vertical blur pass.
        //
        P_f32x1 pGaussianWeights = (pArguments.GetMemberPtr(OFFSET_OF(GenerateColorsBlurParams, pGaussianWeights))).AsP_f32x1();
        C_u32 verticalFlag = pArguments.GetMemberUINT32(OFFSET_OF(GenerateColorsBlurParams, vertical));

        //
        // Determine if we're doing a vertical or horizontal Gaussian pass and set per pixel source advance appropriately
        //
        C_u32 positionChange;
        if (fGaussian)
        {
            positionChange = 1;
            C_Branch directionBranch;
            directionBranch.BranchOnZero(verticalFlag);
            {
                positionChange = sourceWidth;
            }
            directionBranch.BranchHere();
        }

        // Precalculate some things that are constant per pass.
        C_u32 sampleLength = radius * 2 + 1;
        C_f32x4 sampleLengthSquareReplicate = (sampleLength * sampleLength).Replicate().ToFloat4();

        //
        // sampleSourceLineStart increments with each scanline
        // sampleSource increments with each pixel
        // pBufferMemory points to the start of the currently used section of pColumnsMemory
        //        
        P_u32 pSrcCurrentLine = pOriginalSrc;
        P_u32 pSrc = pSrcCurrentLine;
        P_u32 pDstCurrentLine = pOriginalDst;
        P_u32 pDst = pDstCurrentLine;

        P_u32x4 pBoxBlurLineBufferCurrent = pBoxBlurLineBuffer;
        C_u32x4 currentSumValue;

        // Box special case: Setup first line of column sums        
        C_u32 notFirstLine = 0;
        if (!fGaussian)
        {
            currentSumValue = SetupBox(pSrc,
                                       sourceWidth,
                                       sampleLength,
                                       sourceWidth,
                                       pBoxBlurLineBufferCurrent,
                                       boxBlurLineBufferLength
                                       );
        }

        // Loop per scanline
        C_Loop scanLineLoop;
        {
            // Reset variables to start-of-line values
            pSrc = pSrcCurrentLine;
            pDst = pDstCurrentLine;
            
            pBoxBlurLineBufferCurrent = pBoxBlurLineBuffer;
            C_u32 uCount = uCountPerLine;

            if (!fGaussian)
            {
                //
                // For the first scan line, we called SetupBox and we don't want to call
                // MoveBoxToNextLine yet
                //
                C_Branch firstLineBranch;
                firstLineBranch.BranchOnZero(notFirstLine);
                {
                    currentSumValue = MoveBoxToNextLine(pSrc,
                                                       sampleLength,
                                                       sourceWidth,
                                                       pBoxBlurLineBufferCurrent,
                                                       boxBlurLineBufferLength
                                                       );

                    //
                    // For the box case, sampleSourceStart actually points at the previous
                    // line that is being subtracted from the column sums in MoveBoxToNextLine
                    // This is because P_u32 does not support variable length subtract,
                    // only addition.
                    //
                    pSrcCurrentLine+= sourceWidth;
                }
                firstLineBranch.BranchHere();              

                //
                // After the first line, we want to call MoveBoxToNextLine
                //
                notFirstLine = 1;

                //
                // Either SetupBox or MoveBoxToNextLine has produced the first output
                // value for this scanline, save it and advance as required
                //
                *pDst = DivideAndPackResult(currentSumValue, sampleLengthSquareReplicate);
                ++pSrc;
                ++pDst;
                ++pBoxBlurLineBufferCurrent;
                --uCount;
            }
           
            // The main pixel loop per line
            C_Loop loop;    // do while (uCount != 0)
            {
                if (fGaussian)
                {
                    SampleGaussian(pSrc,
                                   positionChange,
                                   sampleLength,
                                   pDst, 
                                   pGaussianWeights
                                   );                
                }
                else
                {
                
                    SampleBox(sampleLength,
                              pDst, 
                              pBoxBlurLineBufferCurrent,
                              &currentSumValue,
                              sampleLengthSquareReplicate
                              );
                }
                
                // Advance
                ++pSrc;
                ++pDst;
                ++pBoxBlurLineBufferCurrent;
                --uCount;
            }
            loop.RepeatIfNonZero(uCount);

            --uCountLines;
            pDstCurrentLine += sourceWidth;

            if (fGaussian)
            {
                //
                // Add line increment to eval. Different for Gaussian and box
                // because of necessity to subtract previous values for the 
                // box filter
                //
                pSrcCurrentLine += sourceWidth;
            }
        }
        scanLineLoop.RepeatIfNonZero(uCountLines);
    }

    IFC(CJitterAccess::Compile(&pBinaryCode));

#ifdef DBG_DUMP
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->DbgDump();    
#endif

    TraceTag((tagMILWarning,
          "CMilBlurEffectDuce::InitializeColorsFunction, Pointer: %p",
          pBinaryCode
          ));

    *pProgram = (GenerateColorsBlur)pBinaryCode;

Cleanup:
    if (fEnteredJitter)
    {
        CJitterAccess::Leave();
    }
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::SetupBox
//
//  Sets up initial column sums for the incremental box filter, and returns the
//  first total pixel sum
//
// Arguments:
//  pSource       - Pointer to the source buffer to start taking samples from. 
//                  This pointer points to the top of the column of the first row
//                  of samples. 
//  sourcePositionDelta - Advance amount to add to pSource to get the next source pixel
//                  for a column. Usually this will be equal to sourceWidth
//  sampleLength  - Number of samples per column. Equivalent to box kernel size
//  sourceWidth   - Width of the source in pixels
//  pLineSumBuffer      - Output buffer to store column sums
//  lineSumBufferCount  - Size of columnSums
//-----------------------------------------------------------------------------
C_u32x4
CMilBlurEffectDuce::SetupBox(P_u32 pSource, 
                             C_u32 sourcePositionDelta,
                             C_u32 sampleLength,
                             C_u32 sourceWidth, 
                             P_u32x4 pLineSumBuffer, 
                             C_u32 lineSumBufferCount
                             )
{
    //
    // Should only execute this block once per box blur pass. It will setup
    // the initial line of column sums, then MoveBoxToNextLine can be used when
    // processing subsequent scanlines
    //

    C_u32x4 lineResult = c_uZero;
    P_u32x4 pLineSumBufferCurrent = pLineSumBuffer;

    // 
    // Calculate the sum of a vertical line of sampleLength pixels, in the direction perpendicular
    // to that of propagation. Store the result in the column sum buffer. 
    //
    C_Loop columnLoop;
    {
        TakeNSamples(sampleLength, pSource, sourcePositionDelta, &lineResult, NULL, FALSE);
        *pLineSumBufferCurrent = lineResult;

        pSource += 1;
        ++pLineSumBufferCurrent;        
        --lineSumBufferCount;
    }
    columnLoop.RepeatIfNonZero(lineSumBufferCount);

    //
    // Calculate the first total sum value for this scan line here, that is the sum of the first 
    // sampleLength column sums. This produces a grand total sum of all contributing pixels for a 
    // particular output pixels, which we can then divide to produce the blurred result.
    //
    // We calculate the first one here and pass it out, then the 
    // SampleBox function can just change it incrementally for the rest of the scan line by adding and 
    // subtracting the new and old column values.
    //
    C_u32x4 totalResult = c_uZero;    
    C_Loop calculateFirstLoop;
    {
        totalResult += *pLineSumBuffer;
        ++pLineSumBuffer;
        --sampleLength;
    }
    calculateFirstLoop.RepeatIfNonZero(sampleLength);

    return totalResult;
}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::MoveBoxToNextLine
//
//  Increments pLineSumBuffer to the next scanline by subtracting the row of 
//  values at pSource and adding the next row of values at 
//  pSource + sampleLength * sourceWidth
//
// Arguments:
//  pSource       - Pointer to the source buffer to start taking samples from. 
//                  This pointer points to the row above the top of the new column,
//                  so that the old contributing values can be subtracted
//  sampleLength  - Number of samples per column. Equivalent to box kernel size
//  sourceWidth   - Width of the source in pixels
//  pLineSumBuffer- Input/Output buffer to store column sums
//  lineSumBufferCount   - Size of columnSums
//-----------------------------------------------------------------------------
C_u32x4
CMilBlurEffectDuce::MoveBoxToNextLine(P_u32 pSource, 
                                     C_u32 sampleLength,
                                     C_u32 sourceWidth, 
                                     P_u32x4 pLineSumBuffer, 
                                     C_u32 lineSumBufferCount
                                     )
{
    // Should only execute this block the first time for each scanline
    // In this line generate not only the current pixel, but the sums for all pixels

    //
    // Variable length subtraction is not implemented for P_u32. Start with a base
    // address and add instead.
    //
    P_u32 pSourceTop = pSource;
    P_u32 pSourceBottom = pSource + sampleLength * sourceWidth;

    P_u32x4 pLineSumBufferCurrent = pLineSumBuffer;

    //
    // For each column, add the next contributor (at positionYXBottom) and subtract the previous 
    // contributor (positionYXTop) to produce the column sum for the new line
    //
    C_Loop columnLoop;
    {
        C_u32x4 newValue = *pLineSumBufferCurrent;    
        newValue -= Sample(pSourceTop);
        newValue += Sample(pSourceBottom);
        *pLineSumBufferCurrent = newValue;

        ++pSourceTop;
        ++pSourceBottom;
        ++pLineSumBufferCurrent;        
        --lineSumBufferCount;
    }
    columnLoop.RepeatIfNonZero(lineSumBufferCount);

    //
    // Calculate the first total sum value for this scan line here, that is the sum of the first 
    // sampleLength column sums. This produces a grand total sum of all contributing pixels for a 
    // particular output pixels, which we can then divide to produce the blurred result.
    //
    // We calculate the first one here and pass it out, then the 
    // SampleBox function can just change it incrementally for the rest of the scan line by adding and 
    // subtracting the new and old column values.
    //
    C_u32x4 totalResult = c_uZero;    
    C_Loop calculateFirstLoop;
    {
        totalResult += *pLineSumBuffer;
        ++pLineSumBuffer;
        --sampleLength;
    }
    calculateFirstLoop.RepeatIfNonZero(sampleLength);

    return totalResult;    
}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::SampleBox
//
// Increments *pPreviousSumValue along a pixel scan line using the column sums
// in pLineSumBuffer
//
// Arguments:
//  sampleLength        - Number of samples per column. Equivalent to box kernel size
//  pDst                - Destination buffer for resultant blurred pixel
//  pLineSumBuffer      - Column sums buffer, pointing to the current initial contributing
//                        pixel
//  pPreviousSumValue   - The previous sum value to increment
//  sampleLengthSquareReplicate - Sample length squared replicated into all four float values
//-----------------------------------------------------------------------------
void
CMilBlurEffectDuce::SampleBox(C_u32 sampleLength,
                              P_u32 pDst, 
                              P_u32x4 pLineSumBuffer,
                              C_u32x4 *pPreviousSumValue,
                              C_f32x4 sampleLengthSquareReplicate
                              )
{
    // We have the previous sum. Add the next sample along and subtract the previous one.
    *pPreviousSumValue += *(pLineSumBuffer + sampleLength - 1);
    *pPreviousSumValue -= *(pLineSumBuffer - 1);

    *pDst = DivideAndPackResult(*pPreviousSumValue, sampleLengthSquareReplicate);
}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::SampleGaussian
//
//  1D Gaussian blur sampler
//
// Arguments:
//  pSource             - Pointer to the source buffer to start taking samples from. 
//                        This pointer points to the top of the column of the first row
//                        of samples. 
//  sourcePositionDelta - Amount to increment pSource for each step in the kernel.
//                        Should be 1 for horizontal and sourceWidth for vertical
//                        NOTE: This is precalculated to avoid branching in
//                        these inner loops which is extremely expensive.
//  sampleLength        - Number of samples per column. Equivalent to kernel size
//  pDst                - Destination for output pixel
//  pGaussianWeights    - Gaussian weights. Length must be sampleLength
//  sampleDivisor       - Amount to divide kernel sum by to get pixel result
//-----------------------------------------------------------------------------
void
CMilBlurEffectDuce::SampleGaussian(P_u32 pSource, 
                                   C_u32 sourcePositionDelta,
                                   C_u32 sampleLength,
                                   P_u32 pDst, 
                                   P_f32x1 pGaussianWeights
                                   )
{   
    C_u32x4 lineResult = c_uZero;

    TakeNSamples(sampleLength, pSource, sourcePositionDelta, &lineResult, pGaussianWeights, TRUE);

    *pDst = PackResult(lineResult);
}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::PackResult
//
//  Pack C_u32x4 ARGB into 32-bit ARGB result 
//
// Arguments:
//  input       - Input value
//
//-----------------------------------------------------------------------------
C_u32
CMilBlurEffectDuce::PackResult(C_u32x4 input)
{
    // Pack into a DWORD
    C_u16x8 u16x8_ColorOutput = input.AsC_u16x8();
    u16x8_ColorOutput = u16x8_ColorOutput.PackUnsignedSaturate(u16x8_ColorOutput).AsC_u16x8();
    return u16x8_ColorOutput.PackUnsignedSaturate(u16x8_ColorOutput).GetLowDWord();    
}


//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::DivideAndPackResult
//
//  Take a pixel sum value and divide by a divisor, then pack into ARGB 
//  32-bit result 
//
// Arguments:
//  input       - Input sum value
//  divisor     - Float divisor
//
//-----------------------------------------------------------------------------
C_u32
CMilBlurEffectDuce::DivideAndPackResult(C_u32x4 input, C_f32x4 divisor)
{   
    // No divide defined for DWord integer math (only add, sub, and multiply on SSE4.1)
    C_f32x4 fResult = input.ToFloat4() / divisor;

    // Convert back from C_f32x4 a r g b to C_u32x1 argb
    C_u32x4 uResult = fResult.ToInt32x4();   

    return PackResult(uResult);
}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::TakeNSamples
//
//  Sample a row/column of pixels, optionally multiply them by weights, and set
//  the result in result.
//
// Arguments:
//  sampleCount     - Number of samples to take
//  pSource         - Start position for sampling
//  sourcePositionDelta   - Amount to advance pSource for next sample
//  result          - Resultant sum 
//  pGaussianWeights- Optional weights for each sampled pixel. Only used if
//                    fGaussian was true at compile time. pGaussianWeights must
//                    be of length sampleCount
//  fGaussian       - JIT Compile time switch, true if gaussian, false if box
//
//  TODO: For Gaussian: - optimize for SSE4.1
//                      - try storing innerResult as a C_f32x4 and converting at
//                        the end. Probably faster. 
//
//-----------------------------------------------------------------------------
void
CMilBlurEffectDuce::TakeNSamples(C_u32 sampleCount, 
                                   P_u32 pSource, 
                                   C_u32 sourcePositionDelta,
                                   C_u32x4 *result,
                                   P_f32x1 pGaussianWeights,
                                   bool fGaussian
                                   )
{
    C_u32 lineloopCount = sampleCount;
    C_u32x4 innerResult = c_uZero;
    C_f32x4 innerResultFloat = c_rZero;

    C_Loop sampleLoop;
    {
        if (fGaussian)  // JIT compile time branch
        {
            C_u32x4 sampleResult = Sample(pSource);
            C_f32x1 weight = pGaussianWeights[lineloopCount-1];
            //
            // TODO - Optimization: Multiply not available for integer types in SSE2. 
            // Special case this for SSE4.1 if there's time. This will make this a lot
            // faster for Core 2 DUO+ processors because we can avoid the float/int conversions
            // and because integer math is much faster than float math.
            //
            // TODO - Pre replicate weights
            //            
            innerResultFloat += (weight.Replicate() * sampleResult.ToFloat4());
        }
        else
        {
            innerResult += Sample(pSource);
        }

        // Increment source location 
        pSource += sourcePositionDelta;
        -- lineloopCount;
    }
    sampleLoop.RepeatIfNonZero(lineloopCount);

    if (fGaussian)
    {
        innerResult = innerResultFloat.ToInt32x4();
    }

    *result = innerResult;
}

//-----------------------------------------------------------------------------
//
// CMilBlurEffectDuce::Sample
//
//  Sample from a source buffer and expand into u32x4 SIMD register
//
// Arguments:
//  pSampleSource   - Source location
//
//-----------------------------------------------------------------------------
C_u32x4 
CMilBlurEffectDuce::Sample(P_u32 pSampleSource)
{
    // Convert to a 4x32 integer vector 0000 0000 0000 argb
    C_u32x4 u4Sample = *pSampleSource;

    // Interleave to get 0000 0000 aarr ggbb
    u4Sample = u4Sample.AsC_u8x16().InterleaveLow(u4Sample.AsC_u8x16());

    // Interleave to get aaaa rrrr gggg bbbb
    u4Sample = u4Sample.AsC_u16x8().InterleaveLow(u4Sample.AsC_u16x8());

    // Shift right to get 000a 000r 000g 000b
    u4Sample >>= 24;   

    return u4Sample;
}



