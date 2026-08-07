// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Drop shadow  effect resource.
//
//-----------------------------------------------------------------------------


#include "precomp.hpp"

MtDefine(DropShadowEffectResource, MILRender, "DropShadowEffect Resource");
MtDefine(CMilDropShadowEffectDuce, DropShadowEffectResource, "CMilDropShadowEffectDuce");

CMilPixelShaderDuce* CMilDropShadowEffectDuce::s_pPixelShader = { 0 };

GenerateColorsBlur CMilDropShadowEffectDuce::s_pfnBlurGaussianAndColor = NULL;

//-----------------------------------------------------------------------------
//
// CMilDropShadowEffectDuce::~CMilDropShadowEffectDuce
//
//-----------------------------------------------------------------------------

CMilDropShadowEffectDuce::~CMilDropShadowEffectDuce()
{
    UnRegisterNotifiers();
}

//-----------------------------------------------------------------------------
//
// CMilDropShadowEffectDuce::Initialize
//
//-----------------------------------------------------------------------------

HRESULT 
CMilDropShadowEffectDuce::Initialize()
{
    HRESULT hr = S_OK;

    //
    // Map the shader byte code for the blur shader. 

    CMilPixelShaderDuce* pDropShadowShader = NULL;
    
    if (s_pPixelShader == NULL)
    {
        //
        // Shaders are organized as follows into the s_pShaderByteCodes array.
        //
        //   Position  Shader
        //    0          ShadowShader

        BYTE* pShaderByteCode = NULL;
        UINT  shaderByteCodeSize = 0;
        
        IFC(LockResource(PS_DropShadow, &pShaderByteCode, &shaderByteCodeSize));
        IFC(CMilPixelShaderDuce::Create(m_pComposition, ShaderEffectShaderRenderMode::HardwareOnly, shaderByteCodeSize, pShaderByteCode, &pDropShadowShader));

        s_pPixelShader = pDropShadowShader; // Transitioning ref to static pointer
        pDropShadowShader = NULL;
    }
        
Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(pDropShadowShader);
    }
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilDropShadowEffectDuce::ApplyEffectSw
//
// Synopsis: 
//      Renders the drop shadow effect into the currently selected render target
//      on the device.
//
//-----------------------------------------------------------------------------
HRESULT 
CMilDropShadowEffectDuce::ApplyEffectSw(
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

    IWGXBitmapLock *pImplicitInputLock = NULL;
    BYTE *pInputBuffer = NULL;    

    CSystemMemoryBitmap *pIntermediateBitmap = NULL;
    IWGXBitmapLock *pIntermediateBitmapLock = NULL;
    BYTE *pIntermediateBuffer = NULL;

    // pARGB input buffer and size
    UINT inputBufferSize = 0;
    MILRect lockRect = { 0, 0, uIntermediateWidth, uIntermediateHeight };
    
    // DropShadowEffect requires that the implicit input be realized.
    Assert(pImplicitInput);
                   
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
        UINT scaledRadius;
        CMilBlurEffectDuce::ApplyRadiusScaling(pScaleTransform, static_cast<UINT>(GetBlurRadius()), &scaledRadius);

        //
        // We assume that the surface has already been inflated by the radius, and the source was originally
        // at least 1x1 size.
        //
        Assert(uIntermediateWidth >= 2 * scaledRadius + 1);
        Assert(uIntermediateHeight >= 2 * scaledRadius + 1);


        auto dropShadowOpacity = static_cast<float const>(GetOpacity());
        
        if (IsCloseReal(dropShadowOpacity, 0.0))
        {            
            // Handle completely transparent shadow as pass through
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
            if (s_pfnBlurGaussianAndColor == NULL)
            {
                IFC(CMilBlurEffectDuce::InitializeBlurFunction(true, true, &s_pfnBlurGaussianAndColor));
                Assert(s_pfnBlurGaussianAndColor);
            }                        

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

            float offsetXFloat = 0.0f;
            float offsetYFloat = 0.0f;
            CalculateOffset(true, pScaleTransform, &offsetXFloat, &offsetYFloat);

            int offsetX = static_cast<int>(offsetXFloat);
            int offsetY = static_cast<int>(offsetYFloat);

            //
            // Need to calculate range limiting
            //
            // Because the drop shadow is 'cast' in a certain direction, we don't need to process the
            // entire surface when drawing the shadow. Indeed, if we try to, we'll eventually try to
            // sample a point for the input to the shadow which is actually outside the surface.
            // To avoid this (and get some optimization benefit by processing less pixels), we clamp the
            // range to avoid areas where the shadow isn't being drawn. We can do this by the full offset
            // because we know the code in GetLocalSpaceClipBounds increases the bounds in the direction
            // opposite the offsets to make sure there is sufficient source area in all clipped cases. 
            //
            // WARNING: If the clipping code is changed to produce tighter bounds (eg don't expand when
            // there is no actual clipping occurring), this code will need to be modified. The change
            // required would be to remove the offset adjustments below, and instead process those previously
            // ignored pixels separately by simply copying input to output for those areas.
            //
            UINT clampedWidth = uIntermediateWidth - 2 * scaledRadius;
            UINT clampedHeight = uIntermediateHeight - 2 * scaledRadius;
            UINT clampedStartX = scaledRadius;
            UINT clampedStartY = scaledRadius;

            //
            // offsetX positive means shadow is to the right of the original object.
            // offsetY positive means shadow is above.
            //
            clampedWidth -= abs(offsetX);
            clampedHeight -= abs(offsetY);

            clampedStartX += (offsetX > 0) ? offsetX : 0;
            clampedStartY += (offsetY < 0) ? -offsetY : 0;

            UINT offsetDistance = uIntermediateWidth * clampedStartY + clampedStartX;
            UINT *pInputBufferUINTLine = reinterpret_cast<UINT*>(pInputBuffer) + offsetDistance;
            UINT *pOutputBufferUINTLine = reinterpret_cast<UINT*>(pIntermediateBuffer) + offsetDistance;

            // Clear the pixels the dropshadow algorithm won't fill.
            IFC(CMilBlurEffectDuce::ClearMarginPixels(
                        reinterpret_cast<UINT*>(pIntermediateBuffer),
                        uIntermediateWidth,
                        uIntermediateHeight,
                        clampedStartX, 
                        clampedStartY, 
                        uIntermediateWidth - (clampedWidth + clampedStartX), 
                        uIntermediateHeight - (clampedHeight + clampedStartY)
                        ));

            //
            // Get a blur buffer and calculate Gaussian weights. This should probably be somewhere else, but it is
            // here for caching simplicity - we don't want to reallocate/calculate these per sample or per line, so 
            // a bit uglier for the sake of performance 
            //
            UINT *pBlurBuffer = reinterpret_cast<UINT*>WPFAlloc(ProcessHeap, Mt(CMilDropShadowEffectDuce), sizeof(UINT) * uIntermediateWidth);
            IFCOOM(pBlurBuffer);
            
            float *pGaussianWeights = reinterpret_cast<float*>WPFAlloc(ProcessHeap, Mt(CMilBlurEffectDuce), (sizeof(float) * (2*scaledRadius + 1)));
            IFCOOM(pGaussianWeights);

            CMilBlurEffectDuce::CalculateGaussianSamplingWeightsFullKernel(scaledRadius, &pGaussianWeights);

            UINT shadowColor = ConvertColor(GetColor());
            UINT opacity = static_cast<UINT>(GetOpacity() * 255.0);

            // For each line
            for (UINT i = clampedStartY; i < clampedHeight + clampedStartY; i++)
            {
                UINT *pInputBufferUINT = pInputBufferUINTLine;                    
                UINT *pOutputBufferUINT = pOutputBufferUINTLine;
                UINT pixelCount = 0;

                //
                // This is not actually an n^2 algorithm at this point, this inner 
                // loop will only execute as many times as the content changes from 
                // opaque to translucent, not for every pixel in clampedWidth
                //
                while (pixelCount < clampedWidth)
                {
                    //
                    // Copy opaque pixels - since the source is opaque and the shadow is 
                    // always "behind" the object, these will not change
                    //
                    while ((pixelCount < clampedWidth) && IsOpaque(*pInputBufferUINT))
                    {
                        *pOutputBufferUINT++ = *pInputBufferUINT++;
                        pixelCount++;
                    }

                    // If we're not at the end of the line, we've hit a translucent pixel
                    if ((pixelCount < clampedWidth) && IsTranslucent(*pInputBufferUINT))
                    {
                        UINT translucentPixelCount = 0;
                        UINT *pCurrentOutputBufferUINTSave = pOutputBufferUINT;
                        UINT *pCurrentInputBufferUINTSave = pInputBufferUINT;
                        
                        // Collect as many contiguous transparent pixels as are available 
                        while ((pixelCount < clampedWidth) && IsTranslucent(*pInputBufferUINT))
                        {
                            translucentPixelCount++;
                            pixelCount++;
                            pInputBufferUINT++;
                        }
                        pOutputBufferUINT += translucentPixelCount;

                        // Ok, now we have some source pixels
                        if (translucentPixelCount > 0)
                        {
                            // Input needs offset adjustment and blurring                                
                            UINT *pAdjustedInputUINT = AdjustSourcePointer(pCurrentInputBufferUINTSave, offsetX, offsetY, uIntermediateWidth);
                            
                            GaussianBlurLineOfPixels(pAdjustedInputUINT,
                                                     pBlurBuffer,
                                                     pCurrentOutputBufferUINTSave,
                                                     uIntermediateWidth,
                                                     uIntermediateHeight,
                                                     scaledRadius,
                                                     translucentPixelCount,
                                                     pGaussianWeights
                                                     );

                            //
                            // Color and opacity blending of blurred offset with source
                            //  This could be rolled up into SSE2 or MMX code, but it's pretty fast right now
                            // Ideally since the drop shadow algorithm only uses the blurred alpha channel of the offset source texture,
                            // the blur could be optimized to ignore the RGB channels and do vector processing of multiple alpha channels
                            // at once. 
                            //
                            for (UINT j = 0; j < translucentPixelCount; j++)
                            {
                                //
                                // Basic shadow algorithm.
                                // blurPixel is the blurred source offset at the appropriate offsetX and offsetY locations
                                // as generated by GaussianBlurLineOfPixels above
                                //
                                // blurPixel.rgba = blurPixel.a * opacity
                                // blurPixel.rgb *= shadowColor.rgb
                                // result = (1 - sourcePixel.a) * blurPixel + sourcePixel
                                //
                                // This is reordered to do the operations common to all color channels first, then the 
                                // channel specific calculations afterwards
                                //
                                
                                // Original source pixel
                                UINT sourcePixel = *pCurrentInputBufferUINTSave;
                                // Alpha channel of blurred offset pixel
                                UINT blurredPixelAlpha = MIL_COLOR_GET_ALPHA(*pCurrentOutputBufferUINTSave);
                                // (1 - sourcePixel.a)
                                UINT invertedSourceAlpha = 255 - MIL_COLOR_GET_ALPHA(sourcePixel);
                                // (1 - sourcePixel.a) * blurPixel.a * opacity
                                LONG combination = blurredPixelAlpha * invertedSourceAlpha * opacity / 65536;

                                // r = (1 - sourcePixel.a) * blurPixel.a * opacity * shadowColor.r + sourcePixel.r
                                UINT red = combination * MIL_COLOR_GET_RED(shadowColor) / 255 + MIL_COLOR_GET_RED(sourcePixel);
                                UINT blue = combination * MIL_COLOR_GET_BLUE(shadowColor) / 255 + MIL_COLOR_GET_BLUE(sourcePixel);
                                UINT green = combination * MIL_COLOR_GET_GREEN(shadowColor) / 255 + MIL_COLOR_GET_GREEN(sourcePixel);
                                UINT alpha = combination + MIL_COLOR_GET_ALPHA(sourcePixel);

                                // Overwrite previous output pixel
                                *pCurrentOutputBufferUINTSave = MIL_COLOR(alpha, red, green, blue);                                                                                                                                                
                                
                                pCurrentInputBufferUINTSave++;
                                pCurrentOutputBufferUINTSave++;
                            }                                
                        }

                        // All incrementing has already been done                            
                    }
                }

                pInputBufferUINTLine += uIntermediateWidth;
                pOutputBufferUINTLine += uIntermediateWidth;
            }


            WPFFree(ProcessHeap, pBlurBuffer);
            WPFFree(ProcessHeap, pGaussianWeights);

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
                                    pIntermediateBitmap,
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
    ReleaseInterface(pIntermediateBitmap);
    ReleaseInterface(pBrushBitmap);

    RRETURN(hr);
}


UINT * 
CMilDropShadowEffectDuce::AdjustSourcePointer(UINT *pBuffer, INT_PTR offsetX, INT_PTR offsetY, UINT width)
{
    // Positive Y moves up
    // Positive X moves right
    return pBuffer + offsetY * width - offsetX;
}


//-----------------------------------------------------------------------------
//
// CMilDropShadowEffectDuce::BlurLineOfPixels
//
// Synopsis: 
//      Produces a single contiguous output line of blurred pixels
//
// Arguments:
//      pInputBuffer    - Pointer to the kernel center for the first pixel on
//                        the input
//      pOutputBuffer   - Pointer to the first output location 
//      pBlurIntermediateBuffer - Intermediate line buffer. Must be at least 
//                        nPixels + 2 * radius in length
//      sourceHeight    - Height of the source
//      sourceWidth     - Width of the source
//      radius          - Blur radius
//      nPixels         - Number of output pixels to produce
//      pGaussianWeights - Gaussian weights
//
// Notes:
//      Efficiency of this function increases with nPixels. Since it performs
//      a 2 pass separable Gaussian blur, calling this function with nPixels == 1
//      is the least efficient case. As nPixels approaches sourceWidth, efficiency
//      increases.
//      Most efficient would be to change this implementation to, and the caller to support 
//      something like "GaussianBlurBlockOfPixels" and blur an n x m grid at once.
//
//-----------------------------------------------------------------------------
HRESULT
CMilDropShadowEffectDuce::GaussianBlurLineOfPixels(
    __in_ecount(sourceWidth * sourceHeight) UINT *pInputBuffer,
    __in_ecount(nPixels + 2 * radius) UINT *pBlurIntermediateBuffer,
    __inout_ecount(nPixels) UINT *pOutputBuffer,
    UINT sourceWidth,
    UINT sourceHeight,
    UINT radius,
    UINT nPixels,
    __in_ecount(2 * radius + 1) float *pGaussianWeights
    )
{
    UNREFERENCED_PARAMETER(sourceHeight);

    if (radius > 0)
    {
        //
        // Adjust to move input pointer back to the top left point for the blur grid.
        //
        // y  -  -  -  -
        // -  -  -  -  -
        // -  -  x  -  -
        // -  -  -  -  -
        // -  -  -  -  -
        //
        // If radius = 2, pInputBuffer initially points to x, we need it to point to y.
        //
        pInputBuffer -= (sourceWidth + 1) * radius;
        
        // pBlurIntermedaiteBuffer should have dimensions, width = sourceWidth, height = 1
        Assert(nPixels + 2 * radius <= sourceWidth);

        // Do vertical blur pass first
        GenerateColorsBlurParams arguments;
        arguments.pargbSource = reinterpret_cast<unsigned *>(pInputBuffer);
        arguments.pargbDestination = reinterpret_cast<unsigned *>(pBlurIntermediateBuffer);
        arguments.sourceWidth = sourceWidth;
        arguments.radius = radius;  
        // Need extra columns at the end for the horizontal pass to sample from, so add
        // 2 * radius to nPixels here
        arguments.nOutputPixelsPerLine = nPixels + radius * 2;  
        arguments.nOutputLines = 1;
        arguments.pBoxBlurLineBuffer = NULL;
        arguments.boxBlurLineBufferLength = 0;
        arguments.pGaussianWeights = pGaussianWeights;
        arguments.vertical = 1;
        
        (*s_pfnBlurGaussianAndColor)(&arguments);

        // Horizontal
        arguments.pargbSource = reinterpret_cast<unsigned *>(pBlurIntermediateBuffer);
        arguments.pargbDestination = reinterpret_cast<unsigned *>(pOutputBuffer);
        arguments.sourceWidth = sourceWidth;
        arguments.radius = radius;  
        arguments.nOutputPixelsPerLine = nPixels;
        arguments.nOutputLines = 1;
        arguments.pBoxBlurLineBuffer = NULL;
        arguments.boxBlurLineBufferLength = 0;
        arguments.pGaussianWeights = pGaussianWeights;
        arguments.vertical = 0;
        
        (*s_pfnBlurGaussianAndColor)(&arguments);
    }
    else
    {
        // Pass through for 0 radius
        for (UINT i = 0; i < nPixels; i++)
        {
            pOutputBuffer[i] = pInputBuffer[i];
        }
    }
    
    RRETURN(S_OK);    
}
    
//-----------------------------------------------------------------------------
//
// CMilDropShadowEffectDuce::ApplyEffect
//
// Synopsis: 
//      Renders the drop shadow effect into the currently selected render target
//      on the device.
//
//-----------------------------------------------------------------------------

HRESULT
CMilDropShadowEffectDuce::ApplyEffect(
    __in CContextState *pContextState, 
    __in CHwSurfaceRenderTarget *pDestRT,
    __in CMILMatrix *pScaleTransform,
    __in CD3DDeviceLevel1 *pDevice, 
    UINT uIntermediateWidth,
    UINT uIntermediateHeight,
    __in_opt CHwTextureRenderTarget *pImplicitInput
    )
{
    HRESULT hr = S_OK;
    IntermediateRTUsage rtUsage;
    rtUsage.flags = IntermediateRTUsage::ForBlending;
    rtUsage.wrapMode = MilBitmapWrapMode::Extend;
    
    CD3DVidMemOnlyTexture* pTextureNoRef_A = NULL;
    CD3DSurface* pSurface_A = NULL;

    CD3DVidMemOnlyTexture* pTexture_B = NULL;
    CD3DSurface* pSurface_B = NULL;
                    
    CMilBlurEffectDuce *pBlurEffect = NULL;

    // When drawing to the back buffer, we support either nearest-neighbor or
    // bilinear sampling.  We don't support Fant interpolation.  If we are
    // rotated, we force bilinear sampling to reduce aliasing artifacts.
    bool useLinearSampling = 
        pContextState->RenderState->InterpolationMode != MilBitmapInterpolationMode::NearestNeighbor ||
        !pContextState->WorldToDevice.IsTranslateOrScale();
    
    // DropShadowEffect requires that the implicit input be realized.
    Assert(pImplicitInput);
    
    pTextureNoRef_A = pImplicitInput->GetTextureNoRef();
    Assert(pTextureNoRef_A != NULL);

    if (!pImplicitInput->IsValid())
    {
        goto Cleanup;
    }
    
    IFC(CreateIntermediateRT(
        pDevice, 
        static_cast<UINT>(uIntermediateWidth), 
        static_cast<UINT>(uIntermediateHeight), 
        D3DFMT_A8R8G8B8,
        &pTexture_B));

    Assert(pTexture_B != NULL);

    IFC(pTextureNoRef_A->GetD3DSurfaceLevel(0, &pSurface_A));
    IFC(pTexture_B->GetD3DSurfaceLevel(0, &pSurface_B));

    //
    // Setup the vertex shader and vertex buffer on the device.
    IFC(pDevice->PrepareShaderEffectPipeline(false /* use vs_2_0 */));

    // Ensure the address mode is set to clamp for both samplers used by the drop shadow shader.
    // Set the sampling mode to nearest neighbor for all intermediate passes.
    for (int i = 0; i < 2; i += 1)
    {
        IFC(SetSamplerState(
            pDevice, 
            i, 
            true,     // set address mode to clamp
            false));  // use nearest neighbor
    }

    // If there is no visible shadow, simply draw the original texture.
    if (GetOpacity() == 0.0)
    { 
        IFC(SetupVertexTransform(
            pContextState, 
            pDevice, 
            static_cast<float>(uIntermediateWidth), 
            static_cast<float>(uIntermediateHeight), 
            true /* populate for rendering into the final destination */));
       
        // Use the original source texture
        IFC(pDevice->SetTexture(0, pTextureNoRef_A));

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
        
        IFC(pDestRT->EnsureState(pContextState));
        IFC(pDevice->SetPassThroughPixelShader());
        IFC(pDevice->SetAlphaBlendMode(&CD3DRenderState::sc_abmSrcOverPremultiplied));
        IFC(pDevice->DrawTriangleStrip(0, 2));
    }
    // If there is a visible shadow we will need to draw into the destination texture
    // twice, first for the shadow and again for the original texture.
    else  
    {
        // On the first pass, we render from the intermediate texture to our 
        // temporary texture to blur and color.
        {
            double radius = GetBlurRadius();
            
            IFC(CMilBlurEffectDuce::Create(radius, MilKernelType::Gaussian, m_data.m_RenderingBias, &pBlurEffect));
            // Radius scaling is handled by ApplyEffectInPipeline.
            IFC(pBlurEffect->ApplyEffectInPipeline(
                pContextState,
                pScaleTransform,
                pDevice,
                uIntermediateWidth,
                uIntermediateHeight,
                pImplicitInput,
                pTexture_B
                ));
        }

        // On the second pass, we run the DropShadow shader to blend the source
        // texture and the blurred shadow texture, which is offset and colored here.
        {
            // Prepare for rendering into final destination buffer
            IFC(SetupVertexTransform(
                pContextState, 
                pDevice, 
                static_cast<float>(uIntermediateWidth), 
                static_cast<float>(uIntermediateHeight), 
                true /* populate for rendering into final dest */));
            
            // Set source texture to s0 and blurred shadow texture to s1
            IFC(pDevice->SetTexture(0, pTextureNoRef_A));
            IFC(pDevice->SetTexture(1, pTexture_B));

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
            
            IFC(pDestRT->EnsureState(pContextState));
            // Set the shadow shader for this pass
            IFC(SetupShader(pDevice, pScaleTransform, static_cast<float>(uIntermediateWidth), static_cast<float>(uIntermediateHeight)));
            IFC(pDevice->SetAlphaBlendMode(&CD3DRenderState::sc_abmSrcOverPremultiplied));
            IFC(pDevice->DrawTriangleStrip(0, 2));
        }
     
    }
    

Cleanup:
    ReleaseInterface(pBlurEffect);
    
    ReleaseInterface(pTexture_B);
    ReleaseInterface(pSurface_A);
    ReleaseInterface(pSurface_B);
    
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
// CMilDropShadowEffectDuce::TransformBoundsForInflation
//
// Synopsis: 
//    Called by the composition layer to account for potential bounds
//    transformations by  effects.  The drop effect inflates the bounds
//    in direction of the shadow by the shadow depth, with additional inflation
//    to account for the softness of the shadow.
//
//-----------------------------------------------------------------------------
HRESULT 
CMilDropShadowEffectDuce::TransformBoundsForInflation(__inout CMilRectF *bounds)
{
    HRESULT hr = S_OK;

    IFC(TransformBoundsInternal(false, bounds));
    
Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
// CMilDropShadowEffectDuce::GetLocalSpaceClipBounds
//
// Synopsis: 
//    Called by the composition layer to allow effects to be applied
//    to clipped areas.  The drop shadow effect can be rendered into a clipped
//    subregion as long as we render into an inflated region to retain
//    sampling accuracy.
//
//-----------------------------------------------------------------------------
HRESULT 
CMilDropShadowEffectDuce::GetLocalSpaceClipBounds(
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
    // apply the drop shadow we can take samples beyond the edges of the clipped area.

    // We need to inflate in the opposite direction of the shadow.
    IFC(TransformBoundsInternal(true, &clippedBoundsLocalSpace));
   
    *pClippedBoundsLocalSpace = clippedBoundsLocalSpace;

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// CMilDropShadowEffectDuce::TransformBoundsInternal
//
// Synopsis: 
//    Called by GetLocalSpaceClipBounds and TransformBoundsForInflation
//    to handle inflating bounds to account for the expansion in size
//    needed to draw the drop shadow.
//    For bounding the result, we expand in the direction of the shadow.
//    For clipping the source texture, we expand in the opposite direction 
//    (so we can sample correctly when we execute the shader).
//
//-----------------------------------------------------------------------------
HRESULT
CMilDropShadowEffectDuce::TransformBoundsInternal(
    __in bool isForClipping,
    __inout CMilRectF *pBounds)
{
    HRESULT hr = S_OK;

    // If the bounds are empty, there is nothing to render so there is no need to inflate.
    if (pBounds->IsEmpty())
    {
        RRETURN(hr);
    }
    
    // 
    // Inflate the bounding box in all directions for the shadow's blur.

    float blurRadius = static_cast<float>(GetBlurRadius());
    pBounds->Inflate(blurRadius, blurRadius);

    //
    // Inflate the bounding box in the shadow direction by shadow depth.

    float offsetX = 0.0f;
    float offsetY = 0.0f;
    CalculateOffset(false, NULL, &offsetX, &offsetY);

    if (isForClipping)
    {
        // When we clip, we need to draw more of our source texture to sample correctly for the
        // shadow, which means we need to offset in the opposite direction of the shadow.  If we 
        // aren't clipping, this is being called to calculate a bounding box, in which case we 
        // want to inflate in the direction of the shadow to ensure it is drawn in the result.
        offsetX = - offsetX;
        offsetY = - offsetY;
    }
    
    // If the shadow is horizontally aligned or to the right of the original element...
    if (offsetX >= 0.0f)
    {
        pBounds->right += offsetX;
    }
    // If the shadow is to the left of the original element...
    else
    {
        pBounds->left += offsetX;
    }

    // If the shadow is vertically aligned or above the original element...
    if (offsetY >= 0.0f)
    {
        pBounds->top -= offsetY;
    }
    // If the shadow is below the original element...
    else
    {
        pBounds->bottom -= offsetY;
    }

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// CMilDropShadowEffectDuce::CalculateOffset
//
// Synopsis: 
//    Calculates the x and y offset of the shadow in relation to the element.
//    Scales the shadow depth to the minimum value of the scale in X and Y
//    directions if isScaled.  When drawing we need the scaled offset, when
//    calculating bounds we want the unscaled one (since bounds are scaled later).
//    A positive offsetX means the shadow is to the right of the element.
//    A positive offsetY means the shadow is above the element.
//
//-----------------------------------------------------------------------------
void
CMilDropShadowEffectDuce::CalculateOffset(
    __in bool isScaled, 
    __in_opt const CMILMatrix *pScaleTransform,
    __out float *offsetX,
    __out float *offsetY)
{
    double direction = GetDirection(); // in degrees
    double depth = GetShadowDepth();

    if (isScaled)
    {
        Assert(pScaleTransform && pScaleTransform->IsPure2DScale());

        REAL prScaleX = 0;
        REAL prScaleY = 0;
        pScaleTransform->GetScaleDimensions(&prScaleX, &prScaleY);
        // Scale the depth by the minimum scale value
        if (prScaleX <= prScaleY) 
        {
            depth = depth * prScaleX;
        }
        else
        {
            depth = depth * prScaleY;
        }
    }
    *offsetX = static_cast<float>(depth * cos(DegToRad(direction)));
    *offsetY = static_cast<float>(depth * sin(DegToRad(direction)));
}


//+----------------------------------------------------------------------------
//
// CMilDropShadowEffectDuce::GetBlurRadius
//
// Synopsis: 
//    Gets the blur radius property from managed code.
//    Note that the blur radius must be non-negative.
//
//-----------------------------------------------------------------------------
double
CMilDropShadowEffectDuce::GetBlurRadius()
{
    double blurRadius = m_data.m_BlurRadius;
    if (m_data.m_pBlurRadiusAnimation != NULL)
    {
        blurRadius = *(m_data.m_pBlurRadiusAnimation->GetValue());
    }
    // Must be non-negative
    return max(blurRadius, 0.0);
}

//+----------------------------------------------------------------------------
//
// CMilDropShadowEffectDuce::GetShadowDepth
//
// Synopsis: 
//    Gets the shadow depth property from managed code.
//    Note that the shadow depth must be non-negative.
//
//-----------------------------------------------------------------------------
double
CMilDropShadowEffectDuce::GetShadowDepth()
{
    double depth = m_data.m_ShadowDepth;
    if (m_data.m_pShadowDepthAnimation != NULL)
    {
        depth = *(m_data.m_pShadowDepthAnimation->GetValue());
    }
    // Must be non-negative
    return max(depth, 0.0);
}

//+----------------------------------------------------------------------------
//
// CMilDropShadowEffectDuce::GetDirection
//
// Synopsis: 
//    Gets the direction property from managed code.
//
//-----------------------------------------------------------------------------
double
CMilDropShadowEffectDuce::GetDirection()
{
    double direction = m_data.m_Direction;
    if (m_data.m_pDirectionAnimation!= NULL)
    {
        direction = *(m_data.m_pDirectionAnimation->GetValue());
    }
    return direction;
}

//+----------------------------------------------------------------------------
//
// CMilDropShadowEffectDuce::GetOpacity
//
// Synopsis: 
//    Gets the opacity property from managed code.
//    Note that the opacity must be in the range 0 to 1 inclusive.
//
//-----------------------------------------------------------------------------
double
CMilDropShadowEffectDuce::GetOpacity()
{
    double opacity = m_data.m_Opacity;
    if (m_data.m_pOpacityAnimation != NULL)
    {
        opacity = *(m_data.m_pOpacityAnimation->GetValue());
    }
    // Cap to range 0-1
    opacity = min(opacity, 1.0);
    opacity = max(opacity, 0.0);
    return opacity;
}

//+----------------------------------------------------------------------------
//
// CMilDropShadowEffectDuce::GetColor
//
// Synopsis: 
//    Gets the color property from managed code.
//
//-----------------------------------------------------------------------------
MilColorF
CMilDropShadowEffectDuce::GetColor()
{
    MilColorF color = m_data.m_Color;
    if (m_data.m_pColorAnimation != NULL)
    {
        color = *(m_data.m_pColorAnimation->GetValue());
    }
    return color;
}

//-----------------------------------------------------------------------------
//
// CMilDropShadowEffectDuce::SetupShader
//
//    Selects the shader for the pass.
//
// Arguments:
//    pDevice - valid pointer to a d3d device.
//        
//-----------------------------------------------------------------------------

HRESULT 
CMilDropShadowEffectDuce::SetupShader(
    __in CD3DDeviceLevel1 *pDevice,
    __in const CMILMatrix *pScaleTransform,
    __in float destinationWidth,
    __in float destinationHeight)
{
    HRESULT hr = S_OK;
    MilColorF color = { 0, 0, 0, 0 };

    IFC(s_pPixelShader->SetupShader(pDevice));

    //
    // Set pixel shader constants
    
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    CalculateOffset(true, pScaleTransform, &offsetX, &offsetY);
    // We need to negate offsetY to account for the fact that a positive offsetY moves up on screen, but moves
    // down in texture coord space.
    float arrOffset[4] = { offsetX / destinationWidth, -offsetY / destinationHeight, /*unused values*/ 0.0f, 0.0f };
    IFC(pDevice->SetPixelShaderConstantF(0, arrOffset, 1));
    
    color = GetColor();
    float arrColor[4] = { color.r, color.g, color.b, color.a };
    IFC(pDevice->SetPixelShaderConstantF(1, arrColor, 1));

    float opacity = static_cast<float>(GetOpacity());
    float arrOpacity[4] = { opacity, /*unused values*/ 0.0f, 0.0f, 0.0f };
    IFC(pDevice->SetPixelShaderConstantF(2, arrOpacity, 1));

Cleanup:
    RRETURN(hr);
}




