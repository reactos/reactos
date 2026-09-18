// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//      Implementation of the class that creates a gradient texture from
//      an array of gradient stops.
//

#include "precomp.hpp"

//+---------------------------------------------------------------------------
//
//  Function:   MILGradientStop operator >
//
//  Synopsis:   Determines if one gradient stop is greater than other by 
//              examining their positions.  
//
//              We define this method so that MILGradientStop's can be
//              sorted with ArrayInsertionSort
//
//  Returns:    true if s1 is greater than s2, 
//              false otherwise
//
//----------------------------------------------------------------------------
inline bool operator > (
    __in_ecount(1) const MILGradientStop& s1, 
    __in_ecount(1) const MILGradientStop& s2)
{
    return (s1.rPosition > s2.rPosition);
}

//+-----------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::CalculateTextureSizeAndMapping
//
//  Synopsis:   Determines the appropriate size texture to create from
//              a set of points the texture will be mapped to, and creates
//              a texture mapping matrix.
//
//              Memory for a texture of the output size should be allocated
//              and passed to GenerateGradientTexture. 
//
//  Returns:    S_OK
//
//------------------------------------------------------------------------
HRESULT 
CGradientTextureGenerator::CalculateTextureSizeAndMapping(
    __in_ecount(1) const MilPoint2F *pStartPointWorldSpace,            
        // Start (linear) or center (radial) point
    __in_ecount(1) const MilPoint2F *pEndPointWorldSpace,              
        // End point (linear) or radius extent (radial)
    __in_ecount(1) const MilPoint2F *pDirectionPointWorldSpace,        
        // Direction point (linear) or other radius extent (radial)
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> *pmatWorldToSampleSpace,
        // World to Sample space matrix
    BOOL fRadialGradient,                                   
        // Is this for a radial gradient?
    MilGradientWrapMode::Enum wrapMode,                           
        // Wrap mode for this gradient
    BOOL fNormalizeMatrix,                                  
        // Normalize the matrix to [0,1] space?
    __out_ecount(1) CGradientSpanInfo *pGradientSpanInfo,
        // information about the gradient span
    __out_ecount(1) CMILMatrix *pmatSampleSpaceToTextureMaybeNormalized
        // Output SampleSpace->Texture matrix
        // The texture space may be normalized if 'fNormalizeMatrix' is true
        // If texture space is not normalized, it is in HPC space.
    )
{
    bool fDegenerateLinearDirection = false;

    //
    // Transform the gradient points to sample space to determine what size realization to make.
    //
    CMilPoint2F rgBrushPointsSampleSpace[3] = { *pStartPointWorldSpace, *pEndPointWorldSpace, *pDirectionPointWorldSpace};
    pmatWorldToSampleSpace->Transform(rgBrushPointsSampleSpace, rgBrushPointsSampleSpace, 3);

    //
    // Eliminate skew from linear gradient brush points. Skew can be introduced
    // by the pmatWorld2DToSampleSpace transform. Eliminating the skew here
    // allows us to calculate a better realization size.
    //
    if (!fRadialGradient)
    {
        CMilPoint2F vecContourDirection =
              rgBrushPointsSampleSpace[2]   //   dir point
            - rgBrushPointsSampleSpace[0];  // - start point

        CMilPoint2F vecNewSpanDirection = vecContourDirection;
        vecNewSpanDirection.TurnRight();
        
        FLOAT flLengthOfNewSpanDirection = vecNewSpanDirection.Norm();
        if (IsNaNOrIsEqualTo(flLengthOfNewSpanDirection, 0.0f))
        {
            // The direction vector is NaN or is so small that it is impossible
            // to tell which way it is pointing. Treat this as a degenerate
            // case
            fDegenerateLinearDirection = true;
        }
        else
        {
            // unitize new span direction
            vecNewSpanDirection *= (1.0f / flLengthOfNewSpanDirection);

            CMilPoint2F vecOldSpan = 
                  rgBrushPointsSampleSpace[1]   //   end point
                - rgBrushPointsSampleSpace[0];  // - start point

            CMilPoint2F vecNewSpan = 
                  vecNewSpanDirection                 // newSpanDirection
                * (vecOldSpan * vecNewSpanDirection); // * dot(oldSpan, newSpanDirection))

            FLOAT flLengthOfNewSpan = vecNewSpan.Norm();
            
            if (IsNaNOrIsEqualTo(flLengthOfNewSpan, 0.0f))
            {
                // The new span length is so small (due to having a skew
                // matrix) that it is impossible to tell which way it is
                // oriented. Treat this as a degenerate case.
                fDegenerateLinearDirection = true;
            }
            else
            {
                CMilPoint2F newEndPoint = 
                      rgBrushPointsSampleSpace[0]           // start point
                    + vecNewSpan;
    
    
                rgBrushPointsSampleSpace[1] = newEndPoint;
            }
        }
    }

    //
    // Determine size of texture
    //
    CalculateTextureSize(
        rgBrushPointsSampleSpace,
        fRadialGradient,
        fDegenerateLinearDirection,
        wrapMode,
        pGradientSpanInfo
        );

    //
    // Calculate the texture mapping
    //
    {
        if (fRadialGradient)
        {
            CalculateTextureMappingForRadialGradient(
                pStartPointWorldSpace,
                pEndPointWorldSpace,
                pDirectionPointWorldSpace,        
                pmatWorldToSampleSpace,
                pGradientSpanInfo,
                pmatSampleSpaceToTextureMaybeNormalized // pmatSampleSpaceToTextureHPC
                );       
        }
        else
        {
            CalculateTextureMappingForLinearGradient(
                rgBrushPointsSampleSpace,
                fDegenerateLinearDirection,
                pGradientSpanInfo,
                pmatSampleSpaceToTextureMaybeNormalized
                );
        }

        //
        // Normalize matrix to [0,1] space from [0, TexelCount] space requested.
        //
        // The HW implementation uses texture coordinates normalized to the [0,1] range,
        // but the SW implemenation uses texture coordinates in the range [0, texelCount]
        //
        
        if (fNormalizeMatrix)
        {            
            REAL rScale = 1.0f / static_cast<FLOAT>(pGradientSpanInfo->GetTexelCount());

            pmatSampleSpaceToTextureMaybeNormalized->Scale(rScale, rScale);
        }               
    }

    RRETURN(S_OK);    
}

//+---------------------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::CalculateTextureSize
//
//  Synopsis:   This method calculates the texture size based on the number 
//              of pixels it will be covering.  Memory for a texture of this
//              size should be allocated and passed to GenerateGradientTexture.
//
//              Bilinear filtering will only interpolate 2 adjacent texels out of 
//              this texture, so if more texels map to a pixel than bilinear 
//              filtering handles, aliasing will result.  This method calculates 
//              the number of texels based on distance of the line being filled to 
//              avoid those artifacts.
//
//  Returns:    VOID
//
//----------------------------------------------------------------------------------
VOID 
CGradientTextureGenerator::CalculateTextureSize(
    __in_ecount(3) const CMilPoint2F *rgBrushPointsSampleSpace,
        // The three brush points in sample space, [begin, end, direction]
    BOOL fRadialGradient,
        // Is this for a radial gradient?   
    bool fDegenerateLinearDirection,
        // true if we've figured out that the linear gradient is degenerate
    MilGradientWrapMode::Enum wrapMode,
        // Gradient wrap mode    
    __out_ecount(1) CGradientSpanInfo *pGradientSpanInfo
        // information about the gradient span
    )
{
    UINT uTexelCount = 1;
    FLOAT flSpanStartTextureSpace = 0.0f;
    FLOAT flSpanEndTextureSpace = 0.0f;
    FLOAT flSpanLengthSampleSpace = 0.0f;
    FLOAT rDistance;
    UINT uSpanLength = 0;
    bool fAddedStartTexel = false;
    bool fAddedEndTexel = false;

    if (fDegenerateLinearDirection)
    {
        goto Cleanup;
    }

    // Calculate distance between the start & end points.  
    //
    // For linear gradients, this is the exact distance that the gradient texture 
    // will be mapped to.  We use this distance to avoid artifacts due to mapping 
    // too small or too large of a texture.  
    rDistance = Distance(rgBrushPointsSampleSpace[0], rgBrushPointsSampleSpace[1]);  

    // Calculate the distance between the start & direction point for radial gradients.
    //
    // The EndPoint & Direction Point lie at the X & Y extents of the gradient ellipse.
    // Since the gradient texture will be mapped using both X & Y coordinates (unlike
    // linear gradients, where only X is important), we create a texture at the largest
    // distance the gradient maps to so that color information for the entire range 
    // is maintained.  
    //
    // We specifically do not want to do this for linear gradients because the direction
    // point has no correlation to the distance the gradient is mapped to.
    if (fRadialGradient)
    {     
        rDistance = max(rDistance, Distance(rgBrushPointsSampleSpace[0], rgBrushPointsSampleSpace[2]));
    }

    // Double the distance for flip wrap mode
    //
    // For reflected gradients, the texture that we create maps to 2 * the distance
    // because we duplicate texels in reverse order
    if (wrapMode == MilGradientWrapMode::Flip)
    {
        rDistance *= 2.0f;
    }

    //
    // Ensure there is at least one texel to represent distances < 1.0
    // Also clamp to a value in the gradient range
    //
    // This guards against overflow in GpFloor.  It also clamps NaN's to 1.0f.
    //
    rDistance = ClampReal(rDistance, 0.0f, static_cast<FLOAT>(MAX_GRADIENTTEXEL_COUNT));
    flSpanLengthSampleSpace = rDistance;


    //
    // Add extra texels for extend
    //
    // Extend wrap mode creates up to two extra texels, one at each end of the
    // gradient texture. These texels contain the extend color(s).
    //
    if (wrapMode == MilGradientWrapMode::Extend)
    {
        if (rDistance >= 1.0f)
        {
            uTexelCount = GpFloor(rDistance);
        }
        else if (rDistance >= (1.0f / 256.0f))
        {
            // The gradient span is small, but still contributes color
            uTexelCount = 1;
        }
        else
        {
            // The gradient span is too small to contribute any color to the
            // brush. All colors will be derived from the start color and end
            // color.
            uTexelCount = 0;
        }

        if (fRadialGradient)
        {
            // Only end extend texel is added to textures for radial gradients
            fAddedEndTexel = true;
            uTexelCount += 1;
        }
        else
        {
            // Both start and end extend texels are added to textures for linear gradients
            fAddedStartTexel = true;
            fAddedEndTexel = true;
            uTexelCount += 2;
        }
    }
    else
    {
        //
        // Convert distance to a texel count.
        //
        uTexelCount = GpFloor(rDistance);

        // At least one texel is needed.
        uTexelCount = max(uTexelCount, static_cast<UINT>(1));

        // Avoid special cases for small span lengths when we don't care about
        // the texture mapping. (There is only one texel to choose from.)
        flSpanLengthSampleSpace = max(flSpanLengthSampleSpace, 1.0f);
    }

    if (uTexelCount >= MAX_GRADIENTTEXEL_COUNT)
    {
        // This can happen during extend mode... we end up clamping twice.

        // Clamp texture size to max.
        uTexelCount = MAX_GRADIENTTEXEL_COUNT;
    }
    else
    {                       
        // Round to the power of 2 >= rDistance.  
        //
        // Determine the maximum number of texels that can map to a pixel
        // which can be handled by bilinear filtering without aliasing.
        //
        // This method rounds to the next power of 2 because of current hardware
        // constraints. But those may be mitigated at some point.  If Task #24595
        // can come up with motivators for this number to be lower than 2, then
        // mitigating those hardware constraints will become more interesting.
        uTexelCount = RoundToPow2(uTexelCount);
    }

    Assert(uTexelCount <= MAX_GRADIENTTEXEL_COUNT);


    // Texel count must be evenly divisible by 2 so that we can  
    // flip the texels uniformly
    Assert( !(uTexelCount % 2) ||
            (uTexelCount == 1) );


    //
    // Calculate uSpanLength. (Necessary after rounding to nearest power of 2.)
    //
    uSpanLength = uTexelCount;
    if (fAddedStartTexel)
    {
        uSpanLength--;
    }
    if (fAddedEndTexel)
    {
        uSpanLength--;
    }
    Assert(uSpanLength <= MAX_GRADIENTTEXEL_COUNT);


    //
    // Calculate other span info
    //

    flSpanStartTextureSpace = 0.0f;
    flSpanEndTextureSpace = static_cast<FLOAT>(uSpanLength);
    if (fAddedStartTexel)
    {
        flSpanStartTextureSpace += 1.0f;
        flSpanEndTextureSpace += 1.0f;
    }
    else if (   wrapMode == MilGradientWrapMode::Flip
             && (uTexelCount > 1) // size==1 textures are not flipped
             )
    {
        // For flip wrap mode, the end point maps to 1/2 the texture width. This
        // is because the second half of the texture contains texels that are
        // duplicated in reverse order (i.e., flipped).
        
        Assert(!(uTexelCount % 2)); // Must be evenly divisible by 2

        Assert(!fAddedStartTexel);
        Assert(!fAddedEndTexel);
        flSpanEndTextureSpace /= 2.0f;
    }

Cleanup:

    // Set gradient span attribtes

    pGradientSpanInfo->SetTexelCount(uTexelCount);
    pGradientSpanInfo->SetSpanAttributes(
        flSpanStartTextureSpace,
        flSpanEndTextureSpace,
        flSpanLengthSampleSpace
        );
}

//+----------------------------------------------------------------------------
//
//  Member:     
//      CGradientTextureGenerator::CalculateTextureMappingForRadialGradient
//
//  Synopsis:
//      This method creates a matrix that maps 'XSpace' coordinates to the
//      texture.
//
//-----------------------------------------------------------------------------
VOID 
CGradientTextureGenerator::CalculateTextureMappingForRadialGradient(
    __in_ecount(1) const MilPoint2F *pStartPoint,    
        // World coordinate of gradient line start point
    __in_ecount(1)  const MilPoint2F *pEndPoint,      
        // World coordinate of gradient line end point
    __in_ecount(1)  const MilPoint2F *pDirectionPoint,
        // World coordinate of gradient line direction point
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> *pmatWorldToSampleSpace,
        // World->SampleSpace matrix
    __inout_ecount(1) CGradientSpanInfo *pGradientSpanInfo,
        // information about the gradient span
    __out_ecount(1) CMILMatrix *pmatSampleSpaceToTextureHPC
        // Output SampleSpace->Texture matrix
    )
{
    HRESULT hr = S_OK;

    // Copy of brush points that is passed to InferAffineMatrix
    MilPoint2F brushPoints[3] = { *pStartPoint, *pEndPoint, *pDirectionPoint };

    // Disable instrumentation breaking.
    // This method handles all of it's own failures.
    SET_MILINSTRUMENTATION_FLAGS(MILINSTRUMENTATIONFLAGS_DONOTHING);

    // Calculate the Device->Brush matrix if it's not trivial
    if (pGradientSpanInfo->GetTexelCount() > 1)
    {
        // Destination bounds the brush points are mapped to
        CRectF<CoordinateSpace::TextureSampling> gradientBounds;

        //
        // Infer Device -> Brush matrix
        //
        // Although this matrix is calculated similarly for linear gradients & radial
        // gradients, the manner in which the matrix is used is different.
        //
        // For radial gradients, we map the user-specified ellipse to a circle with
        // it's center at (0,0) and it's radius set to the width of the texture (minus
        // any extend/flip texels that exist for wrapping).  This is done so that the
        // texel index of any point within that circle can be determined by calculating
        // the distance to the center of the circle (0,0).  
        //
        // For linear gradients, the matrix maps directly to texture coordinates
        // (unlike radial gradients where an intermediate distance calculation is used
        // to index into the texture).  Because the texture has a height of 1, the height of 
        // the rectangle we map to can be any arbitrary length (only the X vector of the matrix 
        // is important because the Y index is always 0).  The only requirement of the height 
        // is that it is non-zero so that the matrix is invertible.        

        //
        // Create the destination rectangle that the brush points map to.
        //
        // We use the same non-zero height for linear & radial gradients since
        // we can map linear gradients to any arbitrary height.
        //
        gradientBounds.top = gradientBounds.left = pGradientSpanInfo->GetSpanStartTextureSpace();
        gradientBounds.bottom = gradientBounds.right = pGradientSpanInfo->GetSpanEndTextureSpace();

        //
        // Compute Texture -> World transform
        //

        //   We may wish to inflate the source rect slightly
        // to handle near-empty cases. See task 15687.

        CMatrix<CoordinateSpace::TextureSampling, CoordinateSpace::BaseSampling> matTextureToBrush;
        matTextureToBrush.InferAffineMatrix(gradientBounds, brushPoints);        

        //
        // Compute Texture -> Device transform by multiplying World -> Device transform
        //

        CMatrix<CoordinateSpace::TextureSampling, CoordinateSpace::Device> matTextureToDevice;
        matTextureToDevice.SetToMultiplyResult(
            matTextureToBrush,
            *pmatWorldToSampleSpace
            );

        // 
        // Invert the Texture -> Device transform to get the Device -> Texture transform
        //
        
        if (!pmatSampleSpaceToTextureHPC->Invert(matTextureToDevice))
        {
            IFC(WGXERR_NONINVERTIBLEMATRIX);
        }
    }
      
Cleanup:

    // Create a single texel mapping if a matrix operation failed.
    // 
    // If a matrix operation failed it is either because the points are either 
    // coincident or too large to compute.  Rather than returning an error
    // we continue processing by creating a single texel for this gradient.    
    //
    // This is the expected behavior for coincident points, and the best
    // thing we can do for large points since their matrix is no longer
    // calculable.
    if (FAILED(hr))
    {
        pGradientSpanInfo->SetTexelCount(1);
    }

    // Create a matrix that maps to (0,0) if only one texel is used
    if (pGradientSpanInfo->GetTexelCount() == 1)
    {
        pmatSampleSpaceToTextureHPC->SetToZeroMatrix();
    }    
}

//+----------------------------------------------------------------------------
//
//  Member:    
//      CGradientTextureGenerator::CalculateTextureMappingForLinearGradient
//
//  Synopsis:  
//      Calculates the texture mapping
//
//-----------------------------------------------------------------------------
VOID
CGradientTextureGenerator::CalculateTextureMappingForLinearGradient(
    __in_ecount(3) const CMilPoint2F *rgBrushPointsSampleSpace,
        // The three brush points in sample space, [begin, end, direction]
    bool fDegenerateDirection,
        // true if we've figured out that the linear gradient is degenerate
    __inout_ecount(1) CGradientSpanInfo *pGradientSpanInfo,
        // information about the gradient span
    __out_ecount(1) CMILMatrix *pmatSampleSpaceToTextureHPC
        // Output SampleSpace->Texture matrix
    )
{
    Assert(pGradientSpanInfo->GetTexelCount() != 0);

    if (fDegenerateDirection)
    {
        pGradientSpanInfo->SetTexelCount(1);
    }
    else if(pGradientSpanInfo->GetTexelCount() > 1)
    {
        FLOAT flScaleX;
        FLOAT flScaleY;
        FLOAT flTranslate;
    
        CMilPoint2F vecSpanSampleSpace = 
              rgBrushPointsSampleSpace[1]  // end point
            - rgBrushPointsSampleSpace[0]; // start point

    
        FLOAT flSpanLengthTextureSpace = pGradientSpanInfo->GetSpanEndTextureSpace() - pGradientSpanInfo->GetSpanStartTextureSpace();
        if (pGradientSpanInfo->GetSpanLengthSampleSpace() < 1.0f)
        {
            FLOAT flTextureToSameSpaceScaleRatio;
            CMilPoint2F ptDegenerateStart = rgBrushPointsSampleSpace[0];

            // The span length is so small we omitted the texel with the span color
            // in it.
            
            FLOAT flSpanLengthSampleSpace = vecSpanSampleSpace.Norm();
            if (IsNaNOrIsEqualTo(flSpanLengthSampleSpace, 0.0f))
            {
                // We did check for this earlier in
                // CalculateTextureSizeAndMapping, but mathematical error might
                // have caused this to crop up again.
                pGradientSpanInfo->SetTexelCount(1);
                goto Cleanup;
            }
            vecSpanSampleSpace *= (1.0f / flSpanLengthSampleSpace);

            if (flSpanLengthTextureSpace > 0)
            {
                //
                // We will modify the stops such that the amount of gradient
                // space covered by the texture is 1 device units worth of
                // coverage. Currently we cover this 1 device unit with two
                // texels.
                //
                // Because we are modifying the stops, we must adjust the start
                // point to compensate.
                //
                // Note that we ignore the length of the gradient span here
                // when computing the scale factor because the length of the
                // gradient span is accounted for in the stop modificaiton
                // process.
                //
                // See LinearGradientNotes.xml for a more complete explanation.
                //
                Assert(flSpanLengthTextureSpace == 2.0f);
                Assert(pGradientSpanInfo->GetSpanStartTextureSpace() == 1.0f);
                Assert(pGradientSpanInfo->GetSpanEndTextureSpace() == 3.0f);

                flTextureToSameSpaceScaleRatio = flSpanLengthTextureSpace;
                
                FLOAT sampleSpaceShift = 0.5f * (1.0f - pGradientSpanInfo->GetSpanLengthSampleSpace());
                ptDegenerateStart -= vecSpanSampleSpace * sampleSpaceShift;
            }
            else
            {
                //
                // We have not modified the stops here, but it is as if we did.
                // Imagine that the gradient span is of 0 length. It may then
                // be placed in the end texel. The end texel covers 1 unit of
                // device space, so the scale ratio is 1.
                //
                Assert(flSpanLengthTextureSpace == 0.0f);
                Assert(pGradientSpanInfo->GetSpanStartTextureSpace() == 1.0f);
                Assert(pGradientSpanInfo->GetSpanEndTextureSpace() == 1.0f);

                flTextureToSameSpaceScaleRatio = 1.0f;
            }
    
            flScaleX = vecSpanSampleSpace.X * flTextureToSameSpaceScaleRatio;
            flScaleY = vecSpanSampleSpace.Y * flTextureToSameSpaceScaleRatio;
            flTranslate =   pGradientSpanInfo->GetSpanStartTextureSpace()
                          - (ptDegenerateStart * vecSpanSampleSpace) * flTextureToSameSpaceScaleRatio;
        }
        else
        {
            //
            // The following formula for computing the matrix may be found in
            // LinearGradientNotes.xml
            //

            FLOAT flSpanLengthSqrSampleSpace = vecSpanSampleSpace * vecSpanSampleSpace;
            if (IsNaNOrIsEqualTo(flSpanLengthSqrSampleSpace, 0.0f))
            {
                // We did check for this earlier in
                // CalculateTextureSizeAndMapping, but mathematical error might
                // have caused this to crop up again.
                pGradientSpanInfo->SetTexelCount(1);
                goto Cleanup;
            }

            FLOAT flMultiplier = flSpanLengthTextureSpace / flSpanLengthSqrSampleSpace;
    
            flScaleX = vecSpanSampleSpace.X * flMultiplier;
            flScaleY = vecSpanSampleSpace.Y * flMultiplier;
            flTranslate =   pGradientSpanInfo->GetSpanStartTextureSpace()
                          - (rgBrushPointsSampleSpace[0] * vecSpanSampleSpace) * flMultiplier;
        }

        pmatSampleSpaceToTextureHPC->SetToIdentity();
        pmatSampleSpaceToTextureHPC->SetM11(flScaleX);
        pmatSampleSpaceToTextureHPC->SetM21(flScaleY);
        pmatSampleSpaceToTextureHPC->SetDx(flTranslate);

        // Texture space is one dimensional.
        pmatSampleSpaceToTextureHPC->SetM22(0.0f);
    }

Cleanup:
    // Create a matrix that maps to (0,0) if only one texel is used
    if (pGradientSpanInfo->GetTexelCount() == 1)
    {
        pmatSampleSpaceToTextureHPC->SetToZeroMatrix();
    }
}

//+-----------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::GenerateGradientTexture
//
//  Synopsis:   Generates a gradient texture from the gradient stops &
//              other parameters.  
//
//  Returns:    HRESULT success or failure
//
//------------------------------------------------------------------------
template <class TFormat>
HRESULT 
CGradientTextureGenerator::GenerateGradientTexture(
    __in_ecount(uStopCount) const MilColorF *pColors,
        // Array of gradient stop colors
    __in_ecount(uStopCount) const FLOAT *pPositions,
        // Array of gradient stop positions
    __in_range(2, MAX_GRADIENTSTOP_COUNT) UINT uStopCount,
        // Number of elements in the color/position arrays
    BOOL fRadialGradient,
        // Is this texture for a radial gradient?               
    MilGradientWrapMode::Enum wrapMode,
        // Wrap mode
    MilColorInterpolationMode::Enum colorInterpolationMode,
        // Color interpolation mode
    __in_ecount(1) const CGradientSpanInfo *pGradientSpanInfo,
        // information about the gradient span
    __in_range(1, MAX_GRADIENTTEXEL_COUNT) UINT uBufferSizeInTexels,
        // Size of the output buffer
    __out_ecount_part(uBufferSizeInTexels, pGradientSpanInfo->m_uTexelCount) TFormat *pTexelBuffer                             
        // Destination texel buffer
    )
{    
    // Solid color textures that stem from zero or one gradient stop
    // should be considered a solid brush and handled by the caller
    Assert(uStopCount >= 2);

    // Don't break on WINCODEC_ERR_INVALIDPARAMETER
    BEGIN_MILINSTRUMENTATION_HRESULT_LIST_WITH_DEFAULTS
        WINCODEC_ERR_INVALIDPARAMETER
    END_MILINSTRUMENTATION_HRESULT_LIST

    // 
    // Local variable declarations 
    //
    
    HRESULT hr = S_OK;

    CGradientStopCollection rgGradientStops;

    MilColorF ptStartExtendColor;
    MilColorF ptEndExtendColor;      

    if (uBufferSizeInTexels < pGradientSpanInfo->GetTexelCount())
    {
        IFC(E_INVALIDARG);
    }

    if (uStopCount > MAX_GRADIENTSTOP_COUNT)
    {
        // User specified too many gradient stops
        IFC(WINCODEC_ERR_INVALIDPARAMETER);    
    }

    //
    // Delegate to implementation methods to create the texture
    // 

    IFC(CopyStops(
        pColors, 
        pPositions, 
        uStopCount, 
        &rgGradientStops));
    
    PrepareStopsForInterpolation(
        &rgGradientStops, 
        colorInterpolationMode);

    CreateWellFormedGradientArray(
        pGradientSpanInfo,
        &rgGradientStops, 
        colorInterpolationMode,
        true, // fSortStops
        &ptStartExtendColor,
        &ptEndExtendColor);    

    //
    // Note: For wrap modes other than extend/pad, the span length is
    //       artificially set to 1.
    //
    if (   pGradientSpanInfo->GetSpanLengthSampleSpace() < 1.0f
        && pGradientSpanInfo->GetSpanLengthSampleSpace() != 0.0f
        && pGradientSpanInfo->IsLinearGradient()
        )
    {
        // Future Consideration:  Some day we may wish to reposition the
        // stops for radial gradients. Today, we only do the operation for
        // linear gradients.

        IFC(RepositionStopsForSmallGradientSpans(
            pGradientSpanInfo,
            &ptStartExtendColor,
            &ptEndExtendColor,
            &rgGradientStops
            ));

        //
        // CreateWellFormedGradientArray must be called again to eliminate
        // stops which are coincident now, but weren't before.
        //
        CreateWellFormedGradientArray(
            pGradientSpanInfo,
            &rgGradientStops, 
            colorInterpolationMode,
            false, // fSortStops
            &ptStartExtendColor,
            &ptEndExtendColor
            );    
    }

    FillTexture(        
        &rgGradientStops,
        fRadialGradient,
        wrapMode,
        colorInterpolationMode,
        &ptStartExtendColor,
        &ptEndExtendColor,
        pGradientSpanInfo,
        uBufferSizeInTexels,
        pTexelBuffer);

Cleanup:

    RRETURN(hr);      
}

// Force instantiations
template 
HRESULT 
CGradientTextureGenerator::GenerateGradientTexture<MilColorB>(
    const MilColorF*,
    const FLOAT*,
    UINT,
    BOOL,              
    MilGradientWrapMode::Enum,
    MilColorInterpolationMode::Enum,
    const CGradientSpanInfo*,
    UINT,
    MilColorB*
    );

template 
HRESULT 
CGradientTextureGenerator::GenerateGradientTexture<AGRB64TEXEL>(
    const MilColorF*,
    const FLOAT*,
    UINT,
    BOOL,              
    MilGradientWrapMode::Enum,
    MilColorInterpolationMode::Enum,
    const CGradientSpanInfo*,
    UINT,
    AGRB64TEXEL*
    );

//+-----------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::CopyStops
//
//  Synopsis:   Copies the color & positions to a private array.  This
//              copy is required because 1) We have to expand it and 
//              2) these are constant in-params the callers do not
//              expect to be changed.
//
//  Returns:    HRESULT success or failure
//
//------------------------------------------------------------------------
HRESULT 
CGradientTextureGenerator::CopyStops(
    __in_ecount(uStopCount)  const MilColorF *pColors,                
        // Array of gradient stop colors
    __in_ecount(uStopCount)  const FLOAT *pPositions,                 
        // Array of gradient stop positions, or NULL
    __in_range(0, MAX_GRADIENTSTOP_COUNT) UINT uStopCount,                            
        // Number of elements in the color/position arrays
    __out_ecount(1) CGradientStopCollection *pGradientStops 
        // Destination gradient stop collection
    )
{
    HRESULT hr = S_OK;

    MILGradientStop tempStop; 
    
    //
    // Reserve space for four additional stops.  This is done to avoid
    // unncessary reallocations.
    //
    // A maximum of two additional stops are needed for each of the endpoints.
    //
    IFC((pGradientStops->ReserveSpace(uStopCount + 4)));    

    //
    // Copy user-specified positions
    //
    
    // Add each stop to the CGradientStopCollection
    for (UINT i = 0; i < uStopCount; i++)
    {
         tempStop.rPosition = pPositions[i];            
         tempStop.color = pColors[i];

         IFC(pGradientStops->Add(tempStop));
    }        

Cleanup:
    
    RRETURN(hr);        
}

//+-----------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::PrepareStopsForInterpolation
//
//  Synopsis:   Color converts & premultiplies the gradient stop colors
//              so that they can be properly interpolated.
//
//  Returns:    VOID
//
//------------------------------------------------------------------------
VOID 
CGradientTextureGenerator::PrepareStopsForInterpolation(
    __inout_ecount(1) CGradientStopCollection *pGradientStops,  // Gradient stop collection 
    MilColorInterpolationMode::Enum colorInterpolationMode            // Color interpolation mode
    )
{ 
    MILGradientStop *pStopBuffer = pGradientStops->GetDataBuffer();
    INT nStopCount = pGradientStops->GetCount();

    //
    //  Convert colors to sRGB if required and premultiply
    //

    // If the interpolation is to be done in sRGB (2.2 gamma) space, then we 
    // need to convert the scRGB input colors to sRGB.
    //
    // This conversion must be done before premultiplication. 
    //
    // The interpolation done by InterpolateColors must be done in non-pre-multiplied space.
    if (colorInterpolationMode == MilColorInterpolationMode::SRgbLinearInterpolation)
    {
        // Convert to sRGB & premultiply stops        
        for (INT i = 0 ; i < nStopCount; i++)
        {
            // Convert color to sRGB          
            pStopBuffer[i].color = 
                Convert_MilColorF_scRGB_To_MilColorF_sRGB(&(pStopBuffer[i].color));
        }        
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::CreateWellFormedGradientArray
//
//  Synopsis:   This method takes an array of gradient stops with positions in 
//              any order, at any value in the floating-point range, and with
//              any amount of coincident stops.
//
//              It's output is a well-formed gradient array, which is a set of
//              sorted stops with positions in in the range [0.0, 1.0],
//              that has redundant coincident stops removed (i.e., no more than 
//              2 stops have the same position), and no coincident stops at 0.0
//              or 1.0.  This method  also returns the 2 solid colors that extend 
//              pass that range (for extend wrap mode).
//
//              During FillGradientSpan, texels are filled by interpolating
//              between gradient stops.  The interpolation calculation requires
//              division by the distance.  To ensure meaningful colors values and
//              avoid dividing by 0, the distance between non-coincident stops
//              must be adequately large.  
//              
//              For this reason the method we use to determine coincidence, 
//              ArePositionsCoincident, doesn't use exact equality.  Instead, it 
//              returns TRUE if the distance between the stops is small enough that 
//              the interpolation calculation wouldn't be meaningful. 
//          
//  Returns:    VOID
//
//---------------------------------------------------------------------------
VOID
CGradientTextureGenerator::CreateWellFormedGradientArray(
    __in_ecount(1) const CGradientSpanInfo *pGradientSpanInfo,
        // information about the gradient span
    __inout_ecount(1) CGradientStopCollection *pGradientStops,
        // Gradient stop collection
    MilColorInterpolationMode::Enum colorInterpolationMode,
        // Color interpolation mode
    bool fSortStops,
        // Indicates whether to sort the stops
    __out_ecount(1) MilColorF *pStartExtendColor,
        // Output start extend color
    __out_ecount(1) MilColorF *pEndExtendColor
        // Output end extend color
    )
{
    MILGradientStop *pStopBuffer = pGradientStops->GetDataBuffer();
    UINT uStopCount = pGradientStops->GetCount();

    // Index of the current stop being examined.
    UINT uCurrentStopIndex;    

    // Index of next destination.  This is always <= uCurrentStopIndex, so we don't have
    // to guard aginst writing on top of a stop that still needs to be examined.
    UINT uNextFreeIndex;

    //
    // Sort the array with a stable sort to maintain order of coincident stops    
    //
    
    if (fSortStops)
    {
        ArrayInsertionSort(pStopBuffer, uStopCount);
    }

    //
    // Call SetFirstStop, which handles all gradient stops with positions <= 0.0
    //

    SetFirstStop(pGradientStops, &uCurrentStopIndex, pStartExtendColor);

    // The first stop is set so start the index at one        
    uNextFreeIndex = 1;
    // SetFirstStop may insert a stop so we need to get the count again
    uStopCount = pGradientStops->GetCount();

    if (uCurrentStopIndex < uStopCount)
    {        
        // Set the stops in between the first and last stops
        SetMiddleStops(pGradientStops, &uCurrentStopIndex, &uNextFreeIndex);        
    }

    // Set the last stop        
    SetLastStop(pGradientStops, uCurrentStopIndex, uNextFreeIndex, pEndExtendColor);    

    // Increment once for the last stop.  This is set by SetFirstStop if both
    // the first and last stops are the same, otherwise it is set during SetLastStop.
    uNextFreeIndex++;

    // Set the number of initialized gradient stops
    pGradientStops->SetCount(uNextFreeIndex);
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CGradientTextureGenerator::RepositionStopsForSmallGradientSpans
//
//  Synopsis:  
//      Repositions the stops such that the newly generated stops array has a
//      length of 1 in sample space.
//
//-----------------------------------------------------------------------------
HRESULT
CGradientTextureGenerator::RepositionStopsForSmallGradientSpans(
    __in_ecount(1) const CGradientSpanInfo *pGradientSpanInfo,
        // information about the gradient span
    __in_ecount(1) const MilColorF *pStartExtendColor,
        // Output start extend color
    __in_ecount(1) const MilColorF *pEndExtendColor,
        // Output end extend color
    __inout_ecount(1) CGradientStopCollection *pGradientStops
        // Gradient stop collection
    )
{
    HRESULT hr = S_OK;

    //
    // We may need to add two extra stops for the start and end colors because they
    // might not exist in the gradient color array. Therefore, they may be
    // different than stop 1 and stop n.
    //

    {
        // Add start color stop 1-2 times
        Assert(pGradientSpanInfo->IsLinearGradient());

        MILGradientStop firstStop;
        firstStop.rPosition = 0.0f;
        firstStop.color = *pStartExtendColor;

        if (!RtlEqualMemory(
            pStartExtendColor,
            &(*pGradientStops)[0].color,
            sizeof(*pStartExtendColor)
            ))
        {
            IFC(pGradientStops->InsertAt(
                firstStop,
                0
                ));
        }

        IFC(pGradientStops->InsertAt(
            firstStop,
            0
            ));
    }

    {
        // Add end color stop 1-2 times

        MILGradientStop lastStop;
        lastStop.rPosition = 1.0f;
        lastStop.color = *pEndExtendColor;

        if (!RtlEqualMemory(
            pEndExtendColor,
            &(*pGradientStops)[pGradientStops->GetCount() - 1].color,
            sizeof(*pEndExtendColor)
            ))
        {
            IFC(pGradientStops->Add(
                lastStop
                ));
        }
    
        IFC(pGradientStops->Add(
            lastStop
            ));
    }

    // Reposition the stops.
    {
        // Future Consideration:  Should we ever reposition the stops for
        // radial gradients, we would want the span to be positioned at the
        // beginning. flShift would == 0.
        Assert(pGradientSpanInfo->IsLinearGradient());

        // Linear gradients prefer the span to be positioned in the middle.
        FLOAT flShift = 0.5f * (1.0f - pGradientSpanInfo->GetSpanLengthSampleSpace());


        MILGradientStop *pStopBuffer = pGradientStops->GetDataBuffer();

        for (UINT i = 1; i < pGradientStops->GetCount() - 1; i++)
        {
            MILGradientStop *pCurrentStop = &pStopBuffer[i];
            pCurrentStop->rPosition = 
                  (pCurrentStop->rPosition * pGradientSpanInfo->GetSpanLengthSampleSpace())
                + flShift;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::SetFirstStop
//
//  Synopsis:   Derives a stop with a position of 0.0 from the input gradient stops
//              and determines the start extend color. It does this by examining
//              all stops <= 0.0, or the first stop > 0.0 if no stops <= 0.0 exist.
//
//  Notes:      This method requires the gradient stop array to already be sorted.
//
//              The stop count of pGradientStops could change during this method if 
//              a gradient stop needs to be inserted.  This value must be reexamined 
//              by the caller after SetFirstStop returns.
//
//  Returns:    None
//
//----------------------------------------------------------------------------------
VOID 
CGradientTextureGenerator::SetFirstStop(
    __inout_ecount(1) CGradientStopCollection *pGradientStops, 
        // Sorted gradient stop collection
    __out_ecount(1) UINT *puNextStopIndex,           
        // Index of stop the that follows the first stop.                          
    __out_ecount(1) MilColorF *pStartExtendColor           
        // Start extend color (for extend wrap mode)
    )
{      
    MILGradientStop *pStopBuffer = pGradientStops->GetDataBuffer();
    UINT uStopCount = pGradientStops->GetCount();

    // This method requires that pStopBuffer contain at least two gradient stops
    Assert(uStopCount >= 2);
    Assert(uStopCount <= MAX_GRADIENTSTOP_COUNT);

    UINT uCurrentIndex;

    //
    // This method handles all possible cominations to determine the gradient stop at 0.0
    //
    // 1. All stops are < 0.0 (Spec Case 23)
    // 2. A stop at 0.0 was specified (Spec Case 7 & 21)
    // 3. Stops exists both above and below 0.0, but not at 0.0 (Spec Case 2) 
    // 4. Initial stop is > 0.0 (Spec Case 9)
    //
   
    //
    // Initial Stop <= 0.0
    //
    if (IsPositionLessThanOrEqual(pStopBuffer[0].rPosition, GRADIENTLINE_FIRSTPOSITION))
    {                          
        // Move past any stops that are < 0.0 until we get to the first stop >= 0.0

        uCurrentIndex = 0; // Evaluate the first stop again because it may be equal
        while ( // We haven't reached the last stop
                (uCurrentIndex < uStopCount) &&
                // and the current position is <= 0.0
                IsPositionLessThan(pStopBuffer[uCurrentIndex].rPosition, GRADIENTLINE_FIRSTPOSITION) 
                )                   
        {
            uCurrentIndex++; // Move to the next stop
        }

        //
        // [CASE #1]. All stops are < 0.0 (Spec Case 23).
        // uCurrentIndex will equal nStopCount if there is not a stop > 0.0, and the 
        // above check did not find a gradient stop within epsilon of 0.0. Thus, there
        // are no stops >= 0.0.  The gradient line is a solid color specified
        // by the negative stop closest to 0.0.
        //
        if (uCurrentIndex == uStopCount)
        {
            // Set gradient stop at 0.0 that contains the color of the last stop
            pStopBuffer[0].rPosition = GRADIENTLINE_FIRSTPOSITION;
            pStopBuffer[0].color = pStopBuffer[uStopCount-1].color;

            // Set start extend colors
            *pStartExtendColor = pStopBuffer[0].color;
            
            // Set next index to 1 past last element to indicate that no additional stops 
            // should be considered                
            *puNextStopIndex = uStopCount;
        }       
        //
        // [CASE #2].  A stop at 0.0 was specified (Spec Case 7 & 21)
        //        
        else if (ArePositionsCoincident(pStopBuffer[uCurrentIndex].rPosition, GRADIENTLINE_FIRSTPOSITION))
        {
            // Set start extend color to left-most stop at 0.0
            *pStartExtendColor = pStopBuffer[uCurrentIndex].color;

            // Move past any coincident stops at 0.0 to the first stop > 0.0
            
            uCurrentIndex++;  // We've evaluated the current stop, move to next
            while ( (uCurrentIndex < uStopCount) &&
                    ArePositionsCoincident(pStopBuffer[uCurrentIndex].rPosition, GRADIENTLINE_FIRSTPOSITION))
            {
                uCurrentIndex++; // Move to next stop
            }
                
            // Copy the stop with a position of 0.0 to the first element
            pStopBuffer[0].rPosition = GRADIENTLINE_FIRSTPOSITION;
            pStopBuffer[0].color = pStopBuffer[uCurrentIndex-1].color;
            
            *puNextStopIndex = uCurrentIndex;
        }
        //
        // [CASE #3].  Stops exist both above and below 0.0, but not at 0.0 (Spec Case 2)
        // There is a stop greater, but not within epsilon of, 0.0.
        //
        else
        {
            // Assert stops are within the valid range
            Assert( (uCurrentIndex > 0 ) &&
                    (uCurrentIndex < uStopCount) );

            MilColorF interpolatedColor;
            
            // The color of the stop at 0.0 is interpolated between the negative and
            // positive stops closest to 0.0
            InterpolateStops(
                &(pStopBuffer[uCurrentIndex-1]),
                &(pStopBuffer[uCurrentIndex]),
                GRADIENTLINE_FIRSTPOSITION,
                &interpolatedColor);   

            // Set stop at 0.0 with interpolated color
            pStopBuffer[0].rPosition = GRADIENTLINE_FIRSTPOSITION;
            pStopBuffer[0].color = interpolatedColor;   
            *pStartExtendColor = interpolatedColor;

            // Set next stop to the first stop > 0.0.
            *puNextStopIndex = uCurrentIndex;
        }                                            
    }
    //
    // Initial Stop > 0.0
    //
    else
    {
        //
        // [CASE #4].  Initial stop is > 0.0 (Spec Case 9).
        // If the position of the initial stop is > 0.0, then the range [0.0, initial stop] is
        // a solid color.  Insert a stop a 0.0 with the same color to achieve this.
        //    
        
        // GenerateGradientTexture ensures that the Capacity is large enough to hold 
        // additional stops.  Assert this instead of reserving additional space.
        Assert(pGradientStops->GetCapacity() > pGradientStops->GetCount());                

        // Increase the stop count to include the stop we are about to insert.
        pGradientStops->SetCount(uStopCount+1);

        // Move all elements down one to make room for the new stop at the
        // beginning of the array
        for (UINT i = uStopCount; i > 0; --i)
        {
            pStopBuffer[i] = pStopBuffer[i-1];                    
        }       

        // Set duplicate stop at element 0 with a position of 0.0
        //
        // Color of stop at index 0 is the same as the the stop at index
        // 1 because the stop at 0 was copied to 1.
        pStopBuffer[0].rPosition = GRADIENTLINE_FIRSTPOSITION;           

        *pStartExtendColor = pStopBuffer[0].color;

        // Set next index to the second stop
        *puNextStopIndex = 1;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::SetMiddleStops
//
//  Synopsis:   After the initial stop has been set via SetFirstStop, this
//              method copies the remaining stops until we reach the
//              last stop or a stop with a position within epsilon of 1.0.
//
//              Because coincident stops create a 'hard' transition with no 
//              interpolation, only the two outside stops in a set of coincident 
//              stops add color information to the gradient.   SetMiddleStops is 
//              responsible for consolidating other redundant coincident stops 
//              'in-between' the two outer coincident stops.
//
//              It also ensures that coincident stops have positions that are 
//              exactly identical, even if the user-specified values weren't exact.  
//              This allows FillGradientSpan to use exact equality when checking 
//              for coincident stops.  It also guards against the case where many 
//              user-specified stops are approximately coincident with their neighbors, 
//              but not approximately coincident with other stops.  By setting the 
//              position of a subsequent nearly-equal coincident stop to the exact 
//              position of the original coincident stop, both stops of the pair, not 
//              just one stop, are guaranteed to never be coincident with stops 
//              preceding or following the pair. 
//
//  Returns:    None
//
//---------------------------------------------------------------------------
VOID 
CGradientTextureGenerator::SetMiddleStops(
    __inout_ecount(1) CGradientStopCollection *pGradientStops, // Gradient stop array
    __inout_ecount(1) UINT *puNextStopIndex,                   // Next stop to examine
    __inout_ecount(1) UINT *puNextFreeIndex                    // Index of next free stop
    )
{
    // This method checks if the current stop is coincident with the 
    // previous stop, so a previous stop must exist.
    Assert(*puNextStopIndex >= 1);

    MILGradientStop *pStopBuffer = pGradientStops->GetDataBuffer();
    UINT uStopCount = pGradientStops->GetCount();

    // The index of the current stop in the array
    UINT uCurrentIndex = *puNextStopIndex;

    // Next uninitialized index a gradient stop can be copied to
    UINT uNextFreeIndex = *puNextFreeIndex;    

    // Copy stops into buffer, compacting excess coincident stops, until
    // there are no stops or we reach a stop within epsilon of 1.0
    while ( (uCurrentIndex < uStopCount) &&
            IsPositionLessThan(pStopBuffer[uCurrentIndex].rPosition, GRADIENTLINE_LASTPOSITION)
         )
    {
        //
        // Check if this stop is coincident with the stop before it
        //
        if (AreStopsCoincident(
                &pStopBuffer[uCurrentIndex-1], 
                &pStopBuffer[uCurrentIndex])
            )
        {
            //
            // Move past any additional coincident stops.  
            //            
            UINT uNotCoincidentIndex = uCurrentIndex +1;           
            while ( // We haven't reached the end of the array
                    (uNotCoincidentIndex < uStopCount) &&
                    // And the current index is not within epsilon of 1.0.
                    //
                    // The stop at uNotCoincidentIndex will not be coincident 
                    // with the stop after it, because SetLastStop will set its 
                    // position to exactly 1.0.
                    IsPositionLessThan(
                        pStopBuffer[uNotCoincidentIndex].rPosition, 
                        GRADIENTLINE_LASTPOSITION) &&
                    // And the current stop is coincident with the
                    // original stop
                    AreStopsCoincident(
                        &pStopBuffer[uCurrentIndex-1],
                        &pStopBuffer[uNotCoincidentIndex]))
            {
                uNotCoincidentIndex++;
            }   

            // Back up to the last coincident stop.  The value that caused the while
            // loop to break was one-past the last coincident stop.
            uNotCoincidentIndex -= 1;

            // Set the position of the last coincident stop exactly equal to the
            // position of the original coincident stop.  
            //
            // This ensures that coincident stops processed by FillGradientSpan are
            // exactly equal.
            pStopBuffer[uNotCoincidentIndex].rPosition = pStopBuffer[uCurrentIndex-1].rPosition;
                
            // Set the current index to the last coincident stop so that it is copied
            // to the next free index.
            uCurrentIndex = uNotCoincidentIndex;
        }

        // Copy the stop at the current index into the stop buffer
        pStopBuffer[uNextFreeIndex++] = pStopBuffer[uCurrentIndex++];                                               
    }    

    *puNextFreeIndex = uNextFreeIndex; 
    *puNextStopIndex = uCurrentIndex;
}   
    
//+---------------------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::SetLastStop
//
//  Synopsis:   Derives the last stop with a position of 1.0 and end extend color 
//              by examining gradient stops with a position >= 1.0, or the last
//              stop if no stops >= 1.0 exist.
//
//  Notes:      This method requires the gradient stop array to already be sorted.
//
//  Returns:    None
//
//----------------------------------------------------------------------------------
VOID 
CGradientTextureGenerator::SetLastStop(
    __inout_ecount(1) CGradientStopCollection *pGradientStops,  // Sorted gradient stop collection
    UINT uCurrentStopIndex,                                     // Index of the current stop that is >= 1.0
    UINT uNextFreeIndex,                                        // Index of next free stop
    __out_ecount(1) MilColorF *pEndExtendColor                  // End extend color
    )
{

    MILGradientStop *pStopBuffer = pGradientStops->GetDataBuffer();
    UINT uStopCount = pGradientStops->GetCount();

    // This method requires that pStopBuffer contain at least two gradient stops
    Assert(uStopCount >= 2);
    Assert(uStopCount <= MAX_GRADIENTSTOP_COUNT);

    //
    // See the comment in SetFirstStop for the URL containing the Spec Case #'s
    // This method handles the remaining cominations of stops to determine the 
    // gradient stop at 1.0, including:
    //
    // 1. All stops are < 1.0 (Spec case 8)
    // 2. A stop was specified at 1.0 (Spec Case 7 & 22)
    // 3. Stops exist both below and above, but not at, 1.0. (Spec Case 3)
    //

    //
    // [CASE #1]. All stops are < 1.0 (Spec case 8).
    // If the last stop is < 1.0, the range [last stop, 1.0] is a solid color.  Duplicate
    // the last stop color to achieve this.    
    if (uCurrentStopIndex == uStopCount)
    {
        // If the last stop < 1.0, add a stop to the array.  

        // CopyStops ensures that the Capacity is large enough to hold additional
        // stops.  Assert this instead of reserving additional space.
        Assert(uNextFreeIndex < pGradientStops->GetCapacity());  

        // Set the duplicate stop at 1.0
        pStopBuffer[uNextFreeIndex].rPosition = GRADIENTLINE_LASTPOSITION;
        pStopBuffer[uNextFreeIndex].color = pStopBuffer[uStopCount-1].color;
        *pEndExtendColor = pStopBuffer[uStopCount-1].color;        
    }        
    //
    // [CASE #2]. A stop was specified at 1.0 (Spec Case 7 & 22).
    // If a stop exists at 1.0, and other stops were specified > 1.0, the other stops
    // do not contibute to the gradient line, but may contribute to the extend color.
    //
    else if (ArePositionsCoincident(pStopBuffer[uCurrentStopIndex].rPosition, GRADIENTLINE_LASTPOSITION) )
    {
        // Copy this stop to the next free index
        pStopBuffer[uNextFreeIndex].rPosition = GRADIENTLINE_LASTPOSITION;
        pStopBuffer[uNextFreeIndex].color = pStopBuffer[uCurrentStopIndex].color;

        // The current stop is coincident, start checking at the next stop
        uCurrentStopIndex++;

        // Move past all stops specified at 1.0         
        while ( (uCurrentStopIndex < uStopCount) &&
                 ArePositionsCoincident(pStopBuffer[uCurrentStopIndex].rPosition, GRADIENTLINE_LASTPOSITION) )
        {
            uCurrentStopIndex++;
        }

        // Set the extend color to the last stop specified at 1.0
        *pEndExtendColor = pStopBuffer[uCurrentStopIndex-1].color;        
    }
    //
    // [CASE #3].  Stops exist both below and above, but not at, 1.0. (Spec Case 3)
    //
    else // Current stop is > 1.0
    {
        MilColorF interpolatedColor;

        Assert(uCurrentStopIndex > 0);
        Assert(uCurrentStopIndex < uStopCount);

        // The color of the stop at 1.0 is interpolated between the stops immediately
        // above and below 1.0
        InterpolateStops(
        &(pStopBuffer[uCurrentStopIndex-1]),
        &(pStopBuffer[uCurrentStopIndex]),
        GRADIENTLINE_LASTPOSITION,
        &interpolatedColor);       

        // Set a stop at 1.0 with the interpolated color
        pStopBuffer[uNextFreeIndex].rPosition = GRADIENTLINE_LASTPOSITION;
        pStopBuffer[uNextFreeIndex].color = interpolatedColor;

        *pEndExtendColor = interpolatedColor;
    }
}

//+-----------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::FillTexture
//
//  Synopsis:   Fills the gradient texture using a well-formed gradient
//              stop collection.  
//
//              For extend wrap mode we add two texels containing the 
//              start and end colors to the texture.  For flip
//              wrap mode we duplicate the texels in reverse order.
//
//              This method delegates the actual generation of the texels
//              from the gradient stop collection to FillGradientSpan, but
//              handles the wrap modes itself.
//
//  Returns:    VOID
//
//------------------------------------------------------------------------
template <class TFormat>
VOID 
CGradientTextureGenerator::FillTexture(
    __in_ecount(1)  const CGradientStopCollection *pGradientStops,
        // Well-formed gradient stop collection
    BOOL fRadialGradient,
        // Is this texture for a radial gradient?               
    MilGradientWrapMode::Enum wrapMode,
        // Wrap mode
    MilColorInterpolationMode::Enum colorInterpolationMode,
        // Color interpolation mode
    __in_ecount(1) const MilColorF *pStartExtendColor,
        // Start color for extend wrap mode
    __in_ecount(1) const MilColorF *pEndExtendColor,
        // End color for extend wrap mode
    __in_ecount(1) const CGradientSpanInfo *pGradientSpanInfo,
        // information about the gradient span
    __in_range(1, MAX_GRADIENTTEXEL_COUNT) UINT uBufferSizeInTexels,
        // Size of the output buffer    
    __out_ecount_part(uBufferSizeInTexels, pGradientSpanInfo->m_uTexelCount) TFormat *pTexelBuffer
        // Destination texel buffer
    )
{
    // For reflect, we need to flip the texels after they are generated
    BOOL fReflectTexels = FALSE;

    const UINT uSpanTexelCount = pGradientSpanInfo->GetTexelCount();
    // To help PREfast
    Assert(uSpanTexelCount <= uBufferSizeInTexels);

    // The number of texels to actually generate (that aren't derived by the wrap mode)
    UINT uGenerateTexelCount = pGradientSpanInfo->GetTexelCount();

    // Number of texels before the start of the generated texels
    UINT uPresetCount = 0;

    //
    //
    // Make adjustments for flip & extend modes
    //
    //

    // Adjust for Flip wrapping mode
    //
    // Update the calling code to implement flip instead of duplicating texels. 
    if (wrapMode == MilGradientWrapMode::Flip)
    {
        // No need to reflect a single texel    
        if (uSpanTexelCount > 1)
        {
            // Texel count must be evenly divisible by 2 so that we can flip the texels uniformly.
            // This is ensured during CalculateTextureSize.
            Assert( !(uSpanTexelCount % 2));
                
            fReflectTexels = TRUE;
            
            // If the wrap mode is flip, we duplicate the generated texels.  Only 
            // generate 1/2 texel count so there is room to place the generated texels.        
            uGenerateTexelCount /= 2;
        }
    }
    // Adjust for Extend wrapping mode    
    else if (wrapMode == MilGradientWrapMode::Extend)
    {        
            //
            // Set the first & last texels equal to the extend colors
            //            

            // Set the first extend texel for 1) linear gradients that 2) contain more than one texel
            //
            // 1) Radial gradient's don't use start texels because the start texel maps
            // to the focal point of the gradient, contributing no additional color information.
            //
            // 2) When a linear gradient's line points are coincident we create a one-texel texture
            // that contains the end extend color (Spec Case #25).  Thus, only set the end texel
            // if this texture contains only 1 texel.
            if ( !fRadialGradient &&
                 (uSpanTexelCount > 1))
            {                           
                // Set first extend texel
                SetOutputTexel(
                    pStartExtendColor, 
                    colorInterpolationMode,
                    pTexelBuffer);

                // Move beginning of generated texture past the start texel we just set
                uPresetCount = 1;

                // Generate one less texel
                Assert(uGenerateTexelCount > 0);
                uGenerateTexelCount -= 1;
            }

            //
            // Always set the last extend texel, regardless of the texel count
            // or gradient type
            //

            // Set last texel
            SetOutputTexel(
                pEndExtendColor,
                colorInterpolationMode,
                &pTexelBuffer[uSpanTexelCount - 1]);
            
            Assert(uGenerateTexelCount > 0);
            // Generate one less texel
            uGenerateTexelCount -= 1;            
    }

    //
    //
    // Generate the texels for this gradient that aren't specific to the wrap mode
    //
    //
    
    if (uGenerateTexelCount > 0)
    {
        Assert(uPresetCount < uBufferSizeInTexels);
        
        FillGradientSpan(
            pGradientStops, 
            colorInterpolationMode, 
            uGenerateTexelCount,
            // Move past the preset texel
            &pTexelBuffer[uPresetCount]);
    }

    //
    //
    // Fill texels used to implement specific wrap modes
    //
    //

    // Set reflect texels
    if (fReflectTexels)  
    {
        Assert(wrapMode == MilGradientWrapMode::Flip);
        // Assert that we have enough memory to reflect each texel without
        // writing beyond the end of the passed-in buffer
        Assert(uSpanTexelCount >= uGenerateTexelCount*2);
        
        ReflectTexels(
            uGenerateTexelCount,
            pTexelBuffer);        
    }
}

//+-----------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::FillGradientSpan
//
//  Synopsis:   Generates texels from a well-formed gradient stop 
//              collection.  This function contains the actual texel
//              loop that generates the texture.
//
//  Returns:    None
//
//------------------------------------------------------------------------
template <class TFormat>
VOID 
CGradientTextureGenerator::FillGradientSpan(
    __in_ecount(1) const CGradientStopCollection *pGradientStops,
       // Well-formed gradient stop collection
    MilColorInterpolationMode::Enum colorInterpolationMode,
       // Color interpolation mode
    __in_range(1, MAX_GRADIENTTEXEL_COUNT) UINT uTexelCount,
        // Number of texels to generate
    __out_ecount(uTexelCount) TFormat *pTexelBuffer
        // Output texel buffer
    )
{
    // Assert required parameters
    Assert(pGradientStops->GetCount() >= 2);
    Assert(uTexelCount <= MAX_GRADIENTTEXEL_COUNT);   

    // Gradient stops
    MILGradientStop *pStopBuffer = pGradientStops->GetDataBuffer();
    UINT uStopCount = pGradientStops->GetCount();

    // Current position in the output texel buffer
    TFormat *pCurrentTexel = pTexelBuffer;

    // Index of texel whose color is currently being calculated
    INT nCurrentTexelIndex = 0;

    // Signed texel count.  
    // We use signed integers in this method because we convert signed floating points 
    // to integer values.
    INT nTexelCount = static_cast<INT>(uTexelCount);

    // Floating-point texel count
    // Cache the result of the floating-point cast.
    FLOAT rTexelCount = static_cast<FLOAT>(nTexelCount);

    // Width & half-width of a texel on the normalized [0.0, 1.0] gradient line
    FLOAT rTexelWidth = 1.0f / rTexelCount;
    FLOAT rHalfTexelWidth = rTexelWidth / 2.0f;

    // Pointers to the left & right stops of the current gradient pair as we move
    // from left to right thru the array
    MILGradientStop *pLeftStop = pStopBuffer;
    MILGradientStop *pRightStop = pStopBuffer + 1;
    // Pointer to the last stop in the array
    MILGradientStop *pLastStop = pStopBuffer + (uStopCount - 1);
        
    // Loop while there are still texels to generate
    //
    // Unlike the optimized inner loops that run through spans of texels when many texels are 
    // between two gradient stops, this this outer loop handles all cases.
    while (nCurrentTexelIndex < nTexelCount)
    {        
        // Calculate the index of texel that the right stop resides in 
        INT nRightStopTexelIndex = GpFloor( // Truncate decimal portion of:
                pRightStop->rPosition *          //   Right stop position *
                rTexelCount);//   the texel count

        // This should be handled during CreateWellFormedGradientArray.
        // Assert this anyways because a position outside of [0.0, 1.0] will cause 
        // ClampInteger to hide bugs, not fix rounding problems.
        Assert(IsPositionGreaterThanOrEqual(pRightStop->rPosition, GRADIENTLINE_FIRSTPOSITION) &&
               IsPositionLessThanOrEqual(pRightStop->rPosition, GRADIENTLINE_LASTPOSITION));

        // Guard against rounding error by clamping nRightStopTexelIndex to 
        // within the valid range
        nRightStopTexelIndex = ClampInteger(nRightStopTexelIndex, 0, nTexelCount);
        // Help PREfast realize that the below while loop is bounded
        Assert(nRightStopTexelIndex <= nTexelCount);
        
        //
        // One or more entire texels exist between the left & right gradient stops.
        // Calculate the texels within this gradient stop span.
        //
        if (nCurrentTexelIndex < nRightStopTexelIndex)
        {     
            MilColorF tempResult;               
            FLOAT rCurrentTexelCenter;
            
            // Calculate the distance between the stops only once for the entire span
            FLOAT rStopDistance = pRightStop->rPosition - pLeftStop->rPosition;

            while (nCurrentTexelIndex < nRightStopTexelIndex)
            {
                // Point sampling at the center of the texel will give us the
                // average color value of the area occupied by the texel
                rCurrentTexelCenter = nCurrentTexelIndex * rTexelWidth + rHalfTexelWidth;

                // Calculate the color of this texel by interpolating between
                // the gradient stops
                InterpolateColors(
                    &(pLeftStop->color),
                    &(pRightStop->color),
                    rCurrentTexelCenter,
                    rStopDistance,
                    pLeftStop->rPosition,
                    &tempResult);

                // Convert the texel to the output format & place in output buffer
                SetOutputTexel(
                    &tempResult,
                    colorInterpolationMode,
                    pCurrentTexel);           

                ++pCurrentTexel;
                ++nCurrentTexelIndex;
            }                  
        } 
        
        //
        // The next stop lies within this texel.  
        // Calculate this texel's color using the weighed contribution of the gradient stop
        // pairs that map to this texel. 
        //

        if (nCurrentTexelIndex < nTexelCount)
        {    
            FillSingleTexelGradientSpan(
                &pLeftStop,
                &pRightStop,
                pLastStop,
                colorInterpolationMode, 
                nCurrentTexelIndex,
                rTexelWidth,
                rTexelCount,
                pCurrentTexel);

                ++pCurrentTexel;
                ++nCurrentTexelIndex;               
        }                    
    }       
}

//+-----------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::FillSingleTexelGradientSpan
//
//  Synopsis:   Determines the color of a texel which has gradient
//              stops that map within it's range.
//
//              This method considers all stop pairs that map to this 
//              texel by summing their weights.  In doing this it 
//              becomes the method responsible for advancing to each
//              new pair of stops.
//
//  Returns:    None
//
//------------------------------------------------------------------------
template <class TFormat>
VOID 
CGradientTextureGenerator::FillSingleTexelGradientSpan(
    __deref_inout_ecount(1)  MILGradientStop **ppLeftStop,
        // Left stop pointer
    __deref_inout_ecount(1)  OUT MILGradientStop **ppRightStop,
        // Right stop pointer
    __in_ecount(1) const MILGradientStop *pLastStop,
        // Pointer to the last stop
    MilColorInterpolationMode::Enum colorInterpolationMode,
        // Color interpolation mode
    INT nCurrentTexelIndex,
        // Index of the current texel
    FLOAT rTexelWidth,
        // Width of a texel along the gradient line
    FLOAT rTexelCount,
        // Floating-point texel count    
    __out_ecount(1) TFormat *pTexel
        // Output destination texel
    )
{
    FLOAT rCurrentTexelLeft = static_cast<FLOAT>(nCurrentTexelIndex) * rTexelWidth;
    FLOAT rNextTexelLeft = static_cast<FLOAT>(nCurrentTexelIndex+1) * rTexelWidth;

    // Channels of resultantColor are the sum of the weighted rangeColor's channels. 
    // Initialize sums to 0.0
    MilColorF resultantColor = { 0.0f, 0.0f, 0.0f, 0.0f };            

    BOOL fMoreGradientStops = TRUE;

    //
    // Sum the weighted contributions of each gradient stop pair to this texel
    // until we get to a stop that doesn't lie within the current texel, or there
    // are no more gradient stops to consider.
    //
    while (fMoreGradientStops && ((*ppRightStop)->rPosition < rNextTexelLeft) ) 
    {
        // Add the weighted contribution for this pair of stops
        AddWeightedStopPairContribution(
            *ppLeftStop,
            *ppRightStop,
            rCurrentTexelLeft,
            rNextTexelLeft,
            rTexelCount,
            &resultantColor);
        
        // Advance to next pair of stops
        fMoreGradientStops = 
            MoveToNextStopPair(ppLeftStop, ppRightStop, pLastStop);
    }

    //
    // Add the contribution of the last gradient pair that maps to this texel
    // 
    // Once the position of the right stop is beyond the end of this texel,
    // we need to add the contribution of the span between the left & right stops 
    // since the left stop still resides in this texel.
    if (fMoreGradientStops)
    {
        AddWeightedStopPairContribution(    
            *ppLeftStop,
            *ppRightStop,
            rCurrentTexelLeft,
            rNextTexelLeft,
            rTexelCount,
            &resultantColor);      
    }

    //
    // Finally, set the output texel color to the derived value
    //

    SetOutputTexel(
        &resultantColor,
        colorInterpolationMode,
        pTexel);          
}

//+-----------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::AddWeightedStopPairContribution
//
//  Synopsis:   Adds the contribution a stop pair makes to the current
//              texel to the resultant color.
//
//  Returns:    None
//
//------------------------------------------------------------------------
VOID 
CGradientTextureGenerator::AddWeightedStopPairContribution(
    __in_ecount(1) const MILGradientStop *pLeftStop,
        // Left stop pointer
    __in_ecount(1) const MILGradientStop *pRightStop,
        // Right stop pointer
    FLOAT rCurrentTexelLeft,            
        // Position of the current texel's left extent
    FLOAT rNextTexelLeft,               
        // Position of the next texel's' left extent
    FLOAT rTexelCount,                  
        // Floating-point texel count        
    __out_ecount(1) MilColorF *pResultantColor   
        // Resultant color
    )
{
    FLOAT rStopRangeLeft, fStopRangeCenter, rStopRangeRight;
    FLOAT rStopRangeDistance, rStopRangeWeight;

    MilColorF rangeColor;
            
    //
    // Our "range of interest" is the intersection of the stop pair's range, with the texel's range.
    // Calculate the width of that range.
    //

    // To get the minimum extent of the current gradient range within this texel, 
    // clamp to be >= the position of this texel. (The left stop may be less
    // than rCurrentTexelLeft)
    rStopRangeLeft = max(rCurrentTexelLeft, pLeftStop->rPosition);

    // To get the maximum extent of the current gradient range within this texel, 
    // clamp it to be <= to the next texel 
    rStopRangeRight = min(rNextTexelLeft, pRightStop->rPosition);

    rStopRangeDistance = rStopRangeRight - rStopRangeLeft;

    // Guard against rounding error causing negative distances or
    // distances so small they aren't worth interpolating over    
    if (!IsDistanceLessThanOrEqual(rStopRangeDistance, 0.0f))        
    {        
        // Determine the average color over the "range of interest", and multiply by the width
        // of the range. Since the function is linear over this range, the average is easy to
        // calculate: Sample in the middle of the range.
        fStopRangeCenter = rStopRangeLeft + (rStopRangeDistance / 2.0f);

        // Determine weight of the current range
        //
        // Divide the partial range by the total range to determine the weight:
        // rStopRangeWeight = rStopRangeDistance / rTexelWidth
        //
        // But: rTexelWidth = 1 / rTexelCount
        // Thus: rStopRangeWeight = rStopRangeDistance * rTexelCount       
        rStopRangeWeight = rStopRangeDistance * rTexelCount;

        // Interpolate between the stops at the sample point
        InterpolateStops(
            pLeftStop,
            pRightStop,
            fStopRangeCenter,
            &rangeColor);

        // Add the weighted contribuation of this pair to the resultant color
        pResultantColor->a += (rangeColor.a * rStopRangeWeight);
        pResultantColor->r += (rangeColor.r * rStopRangeWeight);
        pResultantColor->g += (rangeColor.g * rStopRangeWeight);
        pResultantColor->b += (rangeColor.b * rStopRangeWeight);        
    }                        
}

//+-----------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::ReflectTexels
//
//  Synopsis:   Duplicates the texels in reverse order for reflect wrap
//              mode. 
//
//  Returns:    None
//
//------------------------------------------------------------------------
template <class TFormat>
VOID 
CGradientTextureGenerator::ReflectTexels(
    __in_range(1, MAX_GRADIENTTEXEL_COUNT / 2) UINT uGeneratedTexelCount,
        // Number of texels to reflect
    __inout_ecount(2 * uGeneratedTexelCount) TFormat *pTexelBuffer
        // Output texel buffer
    )
{
    for (UINT i = 0; i < uGeneratedTexelCount; i++)
    {
        pTexelBuffer[uGeneratedTexelCount+i] = pTexelBuffer[uGeneratedTexelCount-1-i];            
    }           
}

//+-----------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::MoveToNextStopPair
//
//  Synopsis:   Advances the left stop pointer & right stop pointer to
//              the next stop pair.  
//
//              If the next pair of stops are coincident, it moves past
//              them as well.
//
//  Returns:    TRUE if there is another pair of stops in the array
//              FALSE otherwise
//
//------------------------------------------------------------------------
BOOL
CGradientTextureGenerator::MoveToNextStopPair(
    __deref_inout_ecount(1) MILGradientStop **ppLeftStop,  // Left stop pointer
    __deref_inout_ecount(1) MILGradientStop **ppRightStop, // Right stop pointer
    __in_ecount(1) const MILGradientStop *pLastStop        // Pointer to the last stop
    )
{    
    //
    // Attempt to increment the right stop
    //
    if (IncrementStops(ppLeftStop, ppRightStop, pLastStop) )
    {
        //
        // Check if the new stop is coincident with the previous stop
        //
        if ((*ppLeftStop)->rPosition == (*ppRightStop)->rPosition)
        {
            //
            // Move past the pair of coincident stops
            //
            if (IncrementStops(ppLeftStop, ppRightStop, pLastStop))
            {
                // CreateWellFormedGradientArray guarantees that no more than two 
                // stop are coincident.
                Assert(!AreStopsCoincident(*ppLeftStop, *ppRightStop));
            }
            else
            {
                // The last stop should not be coincident with the stop before it.  This
                // is handled by CreateWellFormedGradientArray.
                Assert(FALSE);
                
                return FALSE;
            }                                
        }      
        else
        {
            // Stops must not be nearly equal if they aren't exactly equal.  Nearly-equal
            // stops are set to be exactly equal during CreateWellFormedGradientArray.
            Assert(!AreStopsCoincident(*ppLeftStop, *ppRightStop));            
        }
    }       
    else
    {
        // End of array was reached, cannot move to another stop pair.
        return FALSE;
    }

    return TRUE;
}

//+-----------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::IncrementStops
//
//  Synopsis:   Increments the left stop pointer & right stop pointer if
//              it isn't past the end of the array.
//
//  Returns:    TRUE if ppRightStop isn't already pointing to the last stop
//              FALSE otherwise
//
//------------------------------------------------------------------------
BOOL 
CGradientTextureGenerator::IncrementStops(
    __deref_inout_ecount(1) MILGradientStop **ppLeftStop,    // Left stop pointer
    __deref_inout_ecount(1) MILGradientStop **ppRightStop,   // Right stop pointer
    __in_ecount(1) const MILGradientStop *pLastStop     // Pointer to the last stop
    )
{
    if (*ppRightStop == pLastStop)
    {
        // We've been asked to move to the next set of stops, but none exist.          
        //
        // This case will only occur if rounding error in FillSingleTexelGradientSpan
        // causes it to determine that the end of the last texel is < 1.0.
        //
        // Because CreateWellFormedGradientArray ensures a stop exists at 1.0, and
        // we map the stop with a position of 1.0 to the end of the last texel, this
        // could never happen except due to rounding error.        
        return FALSE;    
    }
    else
    {
        // Increment left & right pointer index
        (*ppLeftStop)++;
        (*ppRightStop)++;

        return TRUE;
    }
}  

//+-----------------------------------------------------------------------
//
//  Member:     ClampAndPremultiply
//
//  Synopsis:   Clamps pColorNonPremultiplied at 1.0 and premultiplies
//
//  Returns:    None
//
//------------------------------------------------------------------------
VOID
ClampAndPremultiply(
    __in_ecount(1) const MilColorF *pColorNonPremultiplied,
    __out_ecount(1) MilColorF &colorPremultiplied
    )
{
    colorPremultiplied = *pColorNonPremultiplied;

    //
    // Clamp the color values to 1.0f before doing Premultiply. The
    // interpolation code sometimes throws values over 1.0f. If we wait to do
    // this clamp until after the premultiply operation, we can end up with
    // colors that are oversaturated. Thus we do the clamp now.
    //
    colorPremultiplied.a = min(colorPremultiplied.a, 1.0f);
    colorPremultiplied.r = min(colorPremultiplied.r, 1.0f);
    colorPremultiplied.g = min(colorPremultiplied.g, 1.0f);
    colorPremultiplied.b = min(colorPremultiplied.b, 1.0f);
    Premultiply(&colorPremultiplied);
}

//+-----------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::SetOutputTexel
//
//  Synopsis:   Sets a texel by converting a MilColorB to the
//              appropriate color space & texture format.
//
//  Returns:    None
//
//------------------------------------------------------------------------
template <>
VOID 
CGradientTextureGenerator::SetOutputTexel<MilColorB>(
    __in_ecount(1) const MilColorF *pColorNonPremultiplied,
       // Colors for each texel.  These are non-premultiplied and in the
       // color space specified by colorInterpolationMode
    MilColorInterpolationMode::Enum colorInterpolationMode, 
        // Color interpolation mode
    __out_ecount(1) MilColorB *pTexel                                 
        // Destination texel
    )
{
    MilColorF colorPremultiplied;
    ClampAndPremultiply(pColorNonPremultiplied, colorPremultiplied);

    // Convert from MilColorF sRGB colors to sRGB MilColorB's
    if (colorInterpolationMode == MilColorInterpolationMode::SRgbLinearInterpolation)
    {
        Inline_Convert_MilColorF_sRGB_To_MilColorB_sRGB(
            &colorPremultiplied,
            pTexel); // Place result directly in texel buffer
    }  
    // Convert from MilColorF scRGB colors to sRGB MilColorB's        
    else if (colorInterpolationMode == MilColorInterpolationMode::ScRgbLinearInterpolation)
    {
        Inline_Convert_Premultiplied_MilColorF_scRGB_To_Premultipled_MilColorB_sRGB(
            &colorPremultiplied,
            pTexel); // Place result directly in texel buffer               
    }
    else
    {
        RIP("Bad color interpolation mode");
    } 
}

//+-----------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::SetOutputTexel
//
//  Synopsis:   Sets a texel by converting a MilColorB to the
//              appropriate color space & texture format.
//
//  Returns:    None
//
//------------------------------------------------------------------------
template <>
VOID 
CGradientTextureGenerator::SetOutputTexel<AGRB64TEXEL>(
    __in_ecount(1) const MilColorF *pColorNonPremultiplied,
       // Colors for each texel.  These are non-premultiplied and in the
       // color space specified by colorInterpolationMode
    MilColorInterpolationMode::Enum colorInterpolationMode, 
        // Color interpolation mode
    __out_ecount(1) AGRB64TEXEL *pTexel                                 
        // Destination texel
    )
{
    MilColorF colorPremultiplied;
    ClampAndPremultiply(pColorNonPremultiplied, colorPremultiplied);

    MilColorB tempColor; 

    // Convert from MilColorF sRGB colors to sRGB AGRB64TEXEL's
    if (colorInterpolationMode == MilColorInterpolationMode::SRgbLinearInterpolation)
    {
        Inline_Convert_MilColorF_sRGB_To_MilColorB_sRGB(
            &colorPremultiplied,
            &tempColor);

        Inline_Convert_MilColorB_sRGB_To_AGRB64TEXEL_sRGB(
            tempColor,
            pTexel);         
    } 
    // Convert from MilColorF scRGB colors to sRGB AGRB64TEXEL's        
    else if (colorInterpolationMode == MilColorInterpolationMode::ScRgbLinearInterpolation)
    {
        Inline_Convert_Premultiplied_MilColorF_scRGB_To_Premultipled_MilColorB_sRGB(
            &colorPremultiplied,
            &tempColor);

        Inline_Convert_MilColorB_sRGB_To_AGRB64TEXEL_sRGB(
            tempColor,
            pTexel);
    }
    else
    {
        RIP("Bad color interpolation mode");
    }         
}


//+-----------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::InterpolateStops
//
//  Synopsis:   Linearly interpolates between two stops to determine
//              the color at a point between them
//
//  Returns:    None
//
//------------------------------------------------------------------------
VOID  
CGradientTextureGenerator::InterpolateStops(
    __in_ecount(1) const MILGradientStop *pLeftStop,  // Left stop
    __in_ecount(1) const MILGradientStop *pRightStop, // Right stop
    FLOAT rPosition,                                  // Position to determine value for
    __out_ecount(1) MilColorF *pInterpolatedColor     // Output interpolated color
    )
{
    // Position must be between the 2 stops
    Assert ((rPosition >= pLeftStop->rPosition) &&
            (rPosition <= pRightStop->rPosition));    

    // Calculate distance & call InterpolateColors        
    FLOAT rStopDistance = pRightStop->rPosition - pLeftStop->rPosition;

    InterpolateColors(
        &(pLeftStop->color),
        &(pRightStop->color),
        rPosition,
        rStopDistance,
        pLeftStop->rPosition,
        pInterpolatedColor);                  
}

//+-----------------------------------------------------------------------
//
//  Member:     CGradientTextureGenerator::InterpolateColors
//
//  Synopsis:   Linearly interpolates between two stops to determine
//              the color at a point between them.  
//
//              Only the values needed for interpolation are passed in to 
//              allow the caller to calculate the distance only once for 
//              many interpolations.
//
//  Returns:    None
//
//------------------------------------------------------------------------
VOID
CGradientTextureGenerator::InterpolateColors(
    __in_ecount(1) const MilColorF *pLeftColor,     // Left color
    __in_ecount(1) const MilColorF *pRightColor,    // Right color
    FLOAT rPosition,                                // Position determine value for
    FLOAT rStopDistance,                            // Distance between the stops
    FLOAT rLeftStopPosition,                        // Position of the left stop
    __out_ecount(1) MilColorF *pInterpolatedColor   // Output interpolated color
    )
{
    // Should not interpolate between stops that are coincident.  
    // This is guarded against as follows:
    //   1) The first stop pair is non-coincident, by the definition of "well-formed".
    //   2) MoveToNextStopPair skips non-coincident stops.
    Assert (!IsDistanceEqual(rStopDistance, 0.0f));    

    // 
    // Weight calculations
    // 

    FLOAT rLeftStopWeight, rRightStopWeight;

    //
    // The weight applied to the right stop is: distanceToLeftStop / rStopDistance
    // (i.e., as the distance between the left stop and position decreases, the
    // weight of the right stop decreases). 
    //
    rRightStopWeight = (rPosition - rLeftStopPosition) / rStopDistance;

    // Weight of the left stop is the inverse of the weight of the right stop    
    rLeftStopWeight = 1.0f - rRightStopWeight;
    
    //
    // Linearly interpolate between each channel of the two color values
    //
    
    pInterpolatedColor->a = (pLeftColor->a*rLeftStopWeight) + (pRightColor->a*rRightStopWeight);
    pInterpolatedColor->r = (pLeftColor->r*rLeftStopWeight) + (pRightColor->r*rRightStopWeight);    
    pInterpolatedColor->g = (pLeftColor->g*rLeftStopWeight) + (pRightColor->g*rRightStopWeight);
    pInterpolatedColor->b = (pLeftColor->b*rLeftStopWeight) + (pRightColor->b*rRightStopWeight);
}





