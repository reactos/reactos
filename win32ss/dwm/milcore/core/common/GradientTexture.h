// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//      Definition of the class that creates a gradient texture from
//      an array of gradient stops.
//
//------------------------------------------------------------------------

#pragma once

//
// Constant definitions
//

// Maximum number of gradient stops we can handle is INT_MAX-4.
//
// The count is limited to INT_MAX because we cast the stop count to an INT.
// The count is limited to INT_MAX-4 because capacity for two extra stops is
// required to insert derived stops at positions 0.0 and 1.0 in the array.
// Another two stops may be needed in case we are repositioning the stops for
// small gradient spans.
//
// This is a macro to support PREfast
#define MAX_GRADIENTSTOP_COUNT (INT_MAX - 4)

// Maximum texel count is 1024.  
//
// This value is the largest texture size that can be represenented on
// all supported hardware.
//
// This is a macro to support PREfast
#define MAX_GRADIENTTEXEL_COUNT 1024

// Gradient line first & last positions
const FLOAT GRADIENTLINE_FIRSTPOSITION = 0.0f;
const FLOAT GRADIENTLINE_LASTPOSITION = 1.0f;

//
// Type definitions
//

// Typedef a GradientStopCollection as a DynArray of gradient stops
typedef DynArray<MILGradientStop> CGradientStopCollection;

class CGradientTextureGeneratorTester;
class CMilPoint2F;

class CGradientSpanInfo
{
public:
    CGradientSpanInfo()
    {
        m_uTexelCount = 1;
        m_flSpanStartTextureSpace = 0.0f;
        m_flSpanEndTextureSpace = 0.0f;
        m_flSpanLengthSampleSpace = 0.0f;
    }

    void SetTexelCount(__range(1, MAX_GRADIENTTEXEL_COUNT) UINT uTexelCount)
    {
        m_uTexelCount = uTexelCount;
    }

    __range(1, MAX_GRADIENTTEXEL_COUNT) UINT
    GetTexelCount() const
    {
        return m_uTexelCount;
    }

    void SetSpanAttributes(
        FLOAT flSpanStartTextureSpace,
        FLOAT flSpanEndTextureSpace,
        FLOAT flSpanLengthSampleSpace
        )
    {
        m_flSpanStartTextureSpace = flSpanStartTextureSpace;
        m_flSpanEndTextureSpace = flSpanEndTextureSpace;
        m_flSpanLengthSampleSpace = flSpanLengthSampleSpace;
    }

    FLOAT GetSpanStartTextureSpace() const
    {
        return m_flSpanStartTextureSpace;
    }

    FLOAT GetSpanEndTextureSpace() const
    {
        return m_flSpanEndTextureSpace;
    }

    FLOAT GetSpanLengthSampleSpace() const
    {
        return m_flSpanLengthSampleSpace;
    }

    bool IsLinearGradient() const
    {
        return m_flSpanStartTextureSpace != 0.0f;
    }

private:

    UINT m_uTexelCount;
        // number of texels in the texture

    FLOAT m_flSpanStartTextureSpace;
        // Beginning of the gradient span in texture space (non-normalized)
        // Note: this is the beginning of the gradient span _post_ stop
        // modification. It is always an integer, represented as a float only
        // to avoid casting.

    FLOAT m_flSpanEndTextureSpace;
        // End of the gradient span in texture space (non-normalized)
        // Note: this is the beginning of the gradient span _post_ stop
        // modification. It is always an integer, represented as a float only
        // to avoid casting.

    FLOAT m_flSpanLengthSampleSpace;
        // Length of the gradient span in sample space
        // Can have 3 classes of values:
        // A) >= 1.0f
        //      No special cases necessary
        // B) < 1.0f
        //      Used for gradients with pad mode.
        //      We need to modify the stops.
        // C) == 0.0f
        //      Used for gradients with pad mode.
        //      The gradient span is so small it contributes no color
};

//+-----------------------------------------------------------------------
//
// Class:       CGradientTextureGenerator
//
// Synopsis:    CGradientTextureGenerator is responsible for generating
//              a gradient texture from a user-defined collection of  
//              gradient stops.  
//
//              The gradient stops contain colors specified at positions,
//              which can be any floating point value.  This class
//              is responsible for sorting & normalizing those gradient
//              stops to the [0.0, 1.0] floating-point range by deriving 
//              gradient stops with positions at 0.0 and 1.0.
//
//              It does this in such a way that animating the position of 
//              gradient stops thru this range looks consistent.
//
//              CGradientTextureGenerator generates the gradient texture by mapping 
//              the [0.0, 1.0] normalized gradient range to the texture width.
//              Each texel is considered to represent a range (i.e., not
//              a point) along the gradient line.  The color of each texel
//              is determined by taking the average value of the normalized 
//              gradient range within that texel.
//
//              This is done computing the length of the texel if the
//              texel lies completely within 2 gradient stops.  If one
//              or more gradient stops maps to the texel, then the 
//              weighted average of each range determines the texel's color
//              value.
//
//              This class is also responsible for determining the proper
//              texture size and creating a matrix that maps device
//              coordinates to texture coordinates.  Because the texture
//              is resampled via bilinear filtering when determining
//              actual pixel values, the length of the texture must be within
//              the threshold that bilinear filtering can accurately 
//              reconstruct.  Otherwise aliasing will occur.  Thus, we determine 
//              the texture width based on the number of pixels it is mapped to.
//
//------------------------------------------------------------------------
class CGradientTextureGenerator
{
    // Allow the unit test to call & validate private methods
    friend class CGradientTextureGeneratorTester;

public:

    //
    // Public entry-points
    //

    static HRESULT CalculateTextureSizeAndMapping(
        __in_ecount(1) const MilPoint2F *pStartPointWorldSpace,
        __in_ecount(1) const MilPoint2F *pEndPointWorldSpace,
        __in_ecount(1) const MilPoint2F *pDirectionPointWorldSpace,
        __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> *pmatWorldToSampleSpace,
        BOOL fRadialGradient,        
        MilGradientWrapMode::Enum wrapMode,
        BOOL fNormalizeMatrix,
        __out_ecount(1) CGradientSpanInfo *pGradientSpanInfo,
        __out_ecount(1) CMILMatrix *pmatSampleSpaceToTextureMaybeNormalized
        );

    template <class TFormat> static HRESULT GenerateGradientTexture(
        __in_ecount(uStopCount) const MilColorF *pColors,
        __in_ecount(uStopCount) const FLOAT *pPositions,
        __in_range(2, MAX_GRADIENTSTOP_COUNT) UINT uStopCount,
        BOOL fRadialGradient,
        MilGradientWrapMode::Enum wrapMode,
        MilColorInterpolationMode::Enum colorInterpolationMode,
        __in_ecount(1) const CGradientSpanInfo *pGradientSpanInfo,
        __in_range(1, MAX_GRADIENTTEXEL_COUNT) UINT uBufferSizeInTexels,
        __out_ecount_part(uBufferSizeInTexels, pGradientSpanInfo->m_uTexelCount) TFormat *pTexelBuffer
        );
    
private:

    //
    // CalculateTextureSize implementation
    //

    static VOID CalculateTextureSize(
        __in_ecount(3) const CMilPoint2F *rgBrushPointsSampleSpace,
        BOOL fRadialGradient,                
        bool fDegenerateLinearDirection,
        MilGradientWrapMode::Enum wrapMode,
        __out_ecount(1) CGradientSpanInfo *pGradientSpanInfo
        );

    static VOID CalculateTextureMappingForRadialGradient(
        __in_ecount(1) const MilPoint2F *pStartPoint,
        __in_ecount(1) const MilPoint2F *pEndPoint,
        __in_ecount(1) const MilPoint2F *pDirectionPoint,
        __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> *pmatWorldToSampleSpace,
        __inout_ecount(1) CGradientSpanInfo *pGradientSpanInfo,
        __out_ecount(1) CMILMatrix *pmatSampleSpaceToTextureHPC
        );
    
    static VOID CalculateTextureMappingForLinearGradient(
        __in_ecount(3) const CMilPoint2F *rgBrushPointsSampleSpace,
        bool fDegenerateDirection,
        __inout_ecount(1) CGradientSpanInfo *pGradientSpanInfo,
        __out_ecount(1) CMILMatrix *pmatSampleSpaceToTextureHPC
        );

private:

    //
    // GenerateGradientTexture implementation
    //

   static HRESULT CopyStops(
        __in_ecount(uStopCount) const MilColorF *pColors,
        __in_ecount(uStopCount) const FLOAT *pPositions,
        __in_range(0, MAX_GRADIENTSTOP_COUNT) UINT uStopCount,
        __out_ecount(1) CGradientStopCollection *pGradientStops
        );

    static VOID PrepareStopsForInterpolation(
        __inout_ecount(1) CGradientStopCollection *pGradientStops,
        MilColorInterpolationMode::Enum colorInterpolationMode);      

    // CreateWellFormedGradientArray implementation

    
    static VOID CreateWellFormedGradientArray(
        __in_ecount(1) const CGradientSpanInfo *pGradientSpanInfo,   
        __inout_ecount(1) CGradientStopCollection *pGradientStops,        
        MilColorInterpolationMode::Enum colorInterpolationMode,
        bool fSortStops,
        __out_ecount(1) MilColorF *pStartExtendColor,
        __out_ecount(1) MilColorF *pEndExtendColor
        );         

    static HRESULT RepositionStopsForSmallGradientSpans(
        __in_ecount(1) const CGradientSpanInfo *pGradientSpanInfo,
        __in_ecount(1) const MilColorF *pStartExtendColor,
        __in_ecount(1) const MilColorF *pEndExtendColor,
        __inout_ecount(1) CGradientStopCollection *pGradientStops
        );

    static VOID SetFirstStop(
        __inout_ecount(1) CGradientStopCollection *pGradientStops,
        __out_ecount(1) UINT *puNextStopIndex,
        __out_ecount(1) MilColorF *pStartExtendColor);

    static VOID SetMiddleStops(
        __inout_ecount(1) CGradientStopCollection *pGradientStops,
        __inout_ecount(1) UINT *puNextStopIndex,
        __inout_ecount(1) UINT *puNextFreeIndex);               

    static VOID SetLastStop(
        __inout_ecount(1) CGradientStopCollection *pGradientStops,
        UINT nCurrentStopIndex,
        UINT uNextFreeIndex,
        __out_ecount(1) MilColorF *pEndExtendColor);

    // FillTexture implementation

    template <class TFormat> static VOID FillTexture(
        __in_ecount(1) const CGradientStopCollection *pGradientStops,
        BOOL fRadialGradient,
        MilGradientWrapMode::Enum wrapMode,
        MilColorInterpolationMode::Enum colorInterpolationMode,
        __in_ecount(1) const MilColorF *pStartExtendColor,
        __in_ecount(1) const MilColorF *pEndExtendColor,
        __in_ecount(1) const CGradientSpanInfo *pGradientSpanInfo,
        __in_range(1, MAX_GRADIENTTEXEL_COUNT) UINT uBufferSizeInTexels,
        __out_ecount_part(uBufferSizeInTexels, pGradientSpanInfo->m_uTexelCount) TFormat *pTexelBuffer
        );

    template <class TFormat> static VOID FillGradientSpan(
        __in_ecount(1) const CGradientStopCollection *pGradientStops,
        MilColorInterpolationMode::Enum colorInterpolationMode,
        __in_range(1, MAX_GRADIENTTEXEL_COUNT) UINT uTexelCount,     
        __out_ecount(uTexelCount) TFormat *pTexelBuffer);  
 
    template <class TFormat> static VOID FillSingleTexelGradientSpan(
        __deref_inout_ecount(1) MILGradientStop **ppLeftStop,
        __deref_inout_ecount(1) MILGradientStop **ppRightStop,
        __in_ecount(1) const MILGradientStop *pLastStop,
        MilColorInterpolationMode::Enum colorInterpolationMode,     
        INT nCurrentTexelIndex,
        FLOAT rTexelWidth,
        FLOAT rTexelCount,
        __out_ecount(1) TFormat *pTexel
        );        

    static VOID AddWeightedStopPairContribution(
        __in_ecount(1) const MILGradientStop *pCurrentStop,
        __in_ecount(1) const MILGradientStop *pNextStop,
        FLOAT rCurrentTexelLeft,
        FLOAT rNextTexelLeft,
        FLOAT rTexelCount,        
        __out_ecount(1) MilColorF *pResultantColor);

    template <class TFormat> static VOID ReflectTexels(                  
        __in_range(1, MAX_GRADIENTTEXEL_COUNT / 2) UINT uGeneratedTexelCount,
        __inout_ecount(2 * uGeneratedTexelCount) TFormat *pTexelBuffer);

    static BOOL MoveToNextStopPair(
        __deref_inout_ecount(1) MILGradientStop **ppCurrentStop,
        __deref_inout_ecount(1) MILGradientStop **ppNextStop,
        __in_ecount(1) const MILGradientStop *pLastStop);

private:

    //
    // Inline helper methods
    //

    inline static BOOL IncrementStops(
        __deref_inout_ecount(1) MILGradientStop **ppCurrentStop,
        __deref_inout_ecount(1) MILGradientStop **ppNextStop,
        __in_ecount(1) const MILGradientStop *pLastStop);

    template <class TFormat> inline static VOID SetOutputTexel(
        __in_ecount(1) const MilColorF *pColorNonPremultiplied,  
        MilColorInterpolationMode::Enum colorInterpolationMode,
        __out_ecount(1) TFormat *pbTexel);   

    inline static VOID InterpolateStops(
        __in_ecount(1) const MILGradientStop *pLeftStop, 
        __in_ecount(1) const MILGradientStop *pRightStop,
        FLOAT position,
        __out_ecount(1) MilColorF *pInterpolatedColor);

    inline static VOID InterpolateColors(
        __in_ecount(1) const MilColorF *pLeftColor, 
        __in_ecount(1) const MilColorF *pRightColor,
        FLOAT rPosition,    
        FLOAT rStopDistance,
        FLOAT rLeftStopPosition,
        __out_ecount(1) MilColorF *pInterpolatedColor);

private:
    
    //
    //  Inline epsilon comparators
    //
    
    inline static BOOL ArePositionsCoincident(
        FLOAT rFirst,
        FLOAT rSecond)
    {
        return IsCloseReal(rFirst, rSecond);
    }

    inline static BOOL AreStopsCoincident(
        __in_ecount(1) const MILGradientStop *pLeftStop,
        __in_ecount(1) const MILGradientStop *pRightStop)
    {
        return ArePositionsCoincident(pLeftStop->rPosition, pRightStop->rPosition);
    }

    inline static BOOL IsPositionGreaterThanOrEqual(
        FLOAT rPosition,
        FLOAT rCompareValue)
    {
        return ( (rPosition > rCompareValue) ||
                  ArePositionsCoincident(rPosition, rCompareValue));
    }

    inline static BOOL IsPositionLessThan(
        FLOAT rPosition,
        FLOAT rCompareValue)

    {
        return ( (rPosition < rCompareValue) &&
                  !ArePositionsCoincident(rPosition, rCompareValue));
    }

    inline static BOOL IsPositionLessThanOrEqual(
        FLOAT rPosition,
        FLOAT rCompareValue)
    {
        return ( (rPosition < rCompareValue) ||
                  ArePositionsCoincident(rPosition, rCompareValue));
    }

    inline static BOOL IsDistanceLessThanOrEqual(
        FLOAT rDistance,
        FLOAT rCompareValue)
    {
        return IsPositionLessThanOrEqual(rDistance, rCompareValue);
    }

    inline static BOOL IsDistanceEqual(
        FLOAT rDistance,
        FLOAT rCompareValue)
    {
        return ArePositionsCoincident(rDistance, rCompareValue);
    }

private:

    //
    // HRESULT failures that occur in this class need to be investigated
    //

    SET_MILINSTRUMENTATION_FLAGS(MILINSTRUMENTATIONFLAGS_BREAKANDCAPTURE);    
};


