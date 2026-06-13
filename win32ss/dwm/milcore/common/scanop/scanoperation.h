// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:
//
//      Functions which operate on a "scan" of pixels.
//

struct ColorPalette;

//+-----------------------------------------------------------------------------
//
//  Structure:  PipelineParams
//
//  Synopsis:   Parameters that govern an entire pipeline of scan operations.
//
//------------------------------------------------------------------------------

struct PipelineParams
{
    INT m_iX, m_iY;       // x and y coordinates of the leftmost pixel of the
                          // scan.
    __range(>=,1)
    UINT m_uiCount;       // The length of the scan, in pixels.
    BOOL m_fDither16bpp;  // Used by Dither and 16bpp SrcOverAL. We put it here
                          // because we expect all dither operations in the
                          // pipeline (if any) to use the same setting.
};


//+-----------------------------------------------------------------------------
//
//  Structure:  OpSpecificData
//
//  Synopsis:   Data that is specific to the type of scan operation.
//
//------------------------------------------------------------------------------

struct OpSpecificData {

private:
#if DBG

#if defined(NO_RTTI)
#error RTTI is disabled. We use RTTI for debug-build type-checking assertions.
#endif

    // Force a vtable to be generated, so that RTTI works. This needed by
    // DYNCAST.

    virtual VOID __dummy() {}
#endif // DBG
};

//+-----------------------------------------------------------------------------
//
//  Structure:  COwnedOSD
//
//  Synopsis:   Add a virtual destructor to OpSpecificData, so that it can
//              be deleted polymorphically. Used for OSD types which are
//              dynamically allocated by CScanPipeline.
//
//------------------------------------------------------------------------------

class COwnedOSD : public OpSpecificData
{
public:
    virtual ~COwnedOSD() {}
};


//+-----------------------------------------------------------------------------
//
//  Structure:  ScanOpParams
//
//  Synopsis:   Parameters which may change value for each scan operation in the
//              pipeline, but are still general (i.e. not very specific to the
//              operation type).
//
//------------------------------------------------------------------------------

struct ScanOpParams
{
    VOID *m_pvDest;                // The destination buffer for this operation.

    const VOID *m_pvSrc1;          // The source buffer for this operation.

    const VOID *m_pvSrc2;          // Used only by pseudo-ternary operations,
                                   // like SrcOver. The convention, for
                                   // operations involving alpha-blending, is
                                   // for this to be the "DestIn" pointer.

    OpSpecificData *m_posd;        // Op-specific data (or NULL for some types
                                   // of scan operation.)
};

//+-----------------------------------------------------------------------------
//
//  Operation Description:
//
//      ScanOpFunc is the signature of every Scan Operation.
//
//  Arguments:
//
//      pPP:          Parameters which are constant for the entire pipeline.
//      pSOP:         Parameters which are different for each operation.
//
//  Return Value:
//
//      None
//
//  Notes:
//
//      The formats of the destination and source pointers in pSOP, depend on
//      the specific scan operation.
//
//      pSOP->m_pvDest and pSOP->m_pvSrc* must point to non-overlapping buffers.
//      The one exception is that they may be equal (but only some scan
//      operations allow this).
//
//  Unary, Binary, PTernary:
//
//      A scan operation is classified as one of "unary", "binary" or
//      "pseudo-ternary".
//
//      These terms count only the inputs/outputs in m_pvSrc1, m_pvSrc2, and
//      m_pvDest. Some operations may have other inputs/outputs, but we don't
//      count them because they aren't relevant when composing operations
//      together.
//
//      Unary:    It operates directly on the data in m_pvDest.
//      Binary:   It reads data from m_pvSrc1, and writes the result to
//                m_pvDest. The operation may be further classified as
//                "in-place", i.e. it may allow m_pvDest == m_pvSrc1.
//      PTernary: It reads data from m_pvSrc1 and m_pvSrc2, and optionally
//                writes the result to m_pvDest. The operation may further allow
//                m_pvDest == m_pvSrc2.
//
//                It is "pseudo"-ternary because it doesn't always write to
//                m_pvDest. In the blend operations, m_pvDest isn't written if
//                the source pixel is completely transparent. A true "ternary"
//                operation would always write to m_pvDest, but this way can be
//                more efficient.
//
//                The effect of this is that if m_pvDest != m_pvSrc2,
//                garbage can be introduced, which must be eliminated at the
//                end by using a WriteRMW operation.
//
//------------------------------------------------------------------------------

//
// The function signature for all scan operations
//
//    [agodfrey] Why not refactor? There are a few potential refactorings of
//    scan operations, but this is the best I've found so far.
//    Bear in mind that:
//
//    * Some scan operations are very simple and are very perf sensitive. They
//      typically also have a NULL OpSpecificData *.
//
//    * Some scan operations share the same OpSpecificData struct. So to use a
//      virtual function call for a scan operation (instead of a function/method
//      pointer) would require boilerplate code that makes this solution a lot
//      less attractive than it first appears.
//
//    * "Object orientation" doesn't apply here. One can summarize the above 2
//      points by saying that scan operations are conceptually closer to
//      functions, not methods. However, one could improve packaging by using
//      namespaces. (In fact, I did use a single "ScanOperation" namespace, but
//      someone removed it.)
//
//    * This is more circumstantial, but still:
//
//      + Virtual function calls are slower than other function calls.
//
//      + On x86, __fastcall functions enregister 2 parameters - and we need
//        exactly 2 parameters. With member functions, in practice it seems that
//        only 1 parameter (the "this" pointer) is enregistered. [But, now that
//        we're using link-time code generation, this may not be as true.]
//

typedef VOID (FASTCALL *ScanOpFunc)(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP);

//
// OpSpecificData types:
//

struct OSDPalette : public OpSpecificData
{
    friend VOID FASTCALL Convert_1_32bppARGB(const PipelineParams *, const ScanOpParams *);
    friend VOID FASTCALL Convert_2_32bppARGB(const PipelineParams *, const ScanOpParams *);
    friend VOID FASTCALL Convert_4_32bppARGB(const PipelineParams *, const ScanOpParams *);
    friend VOID FASTCALL Convert_8_32bppARGB(const PipelineParams *, const ScanOpParams *);

    const ColorPalette *m_pPalette;
};


// Helper macros for defining scan operations

#define DEFINE_POINTERS(srctype, dsttype) \
    const srctype* pSrc = static_cast<const srctype *>(pSOP->m_pvSrc1); \
    dsttype* pDest = static_cast<dsttype *>(pSOP->m_pvDest);

#define DEFINE_POINTERS_BLEND_NO_IN(srctype, dsttype) \
    dsttype* pDestOut = static_cast<dsttype *>(pSOP->m_pvDest); \
    const srctype* pSrc = static_cast<const srctype *>(pSOP->m_pvSrc1);

#define DEFINE_POINTERS_BLEND(srctype, dsttype) \
    const dsttype* pDestIn = static_cast<const dsttype *>(pSOP->m_pvSrc2); \
    DEFINE_POINTERS_BLEND_NO_IN(srctype, dsttype)

// Not all scan operations are declared here. Look elsewhere for ColorSource,
// ConstantAlpha, MaskAlpha1, MaskAlpha2.

// Convert: Convert "losslessly" to another format.
//          ("losslessly": The operation is an injection.)
//          (Binary operation)

VOID FASTCALL Convert_1_32bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_1BW_32bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_2_32bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_4_32bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_8_32bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_555_32bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_565_32bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_1555_32bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_24_32bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_24BGR_32bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_32RGB_32bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_32bppGray_128bppABGR(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_48_64bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_16bppGray_64bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_128RGB_128bppABGR(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_32bppRGB101010_128bppABGR(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_32bppCMYK_64bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_64bppARGB_48bppRGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_48bppRGB_64bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_64bppARGB_32bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Convert_32bppARGB_64bppARGB(const PipelineParams *, const ScanOpParams *);

// Quantize: Convert "lossily" to another format by quantizing (no dithering). 
//           ("Lossily": the operation is not an injection, and so is not
//           invertible without data loss).
//           (Binary operation)

VOID FASTCALL Quantize_32bppARGB_555(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Quantize_32bppARGB_565(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Quantize_32bppARGB_1555(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Quantize_32bppARGB_24(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Quantize_32bppARGB_24BGR(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Quantize_32bppARGB_32RGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Quantize_64bppARGB_48(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Quantize_64bppARGB_16bppGray(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Quantize_128bppABGR_128RGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Quantize_128bppABGR_32bppRGB101010(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Quantize_64bppARGB_32bppCMYK(const PipelineParams *, const ScanOpParams *);

// See Halftone.h for more color reduction functions.

// Dither: Dither to a 16bpp format. (Binary operation)

VOID FASTCALL Dither_32bppARGB_565(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Dither_32bppARGB_555(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Dither_32bppARGB_565_MMX(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Dither_32bppARGB_555_MMX(const PipelineParams *, const ScanOpParams *);

// Copy: Copy a scan, in the same format. (Binary operation)

VOID FASTCALL Copy_1(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Copy_4(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Copy_8(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Copy_16(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Copy_24(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Copy_32(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Copy_48(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Copy_64(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL Copy_128(const PipelineParams *, const ScanOpParams *);

// GammaConvert: Convert between formats with differing gamma ramps. (Binary
//               operation)

VOID FASTCALL GammaConvert_128bppABGR_32bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL GammaConvert_32bppARGB_128bppABGR(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL GammaConvert_128bppABGR_16bppGrayInt(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL GammaConvert_128bppABGR_32bppGrayFloat(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL GammaConvert_16bppGrayInt_128bppABGR(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL GammaConvert_32bppGrayFloat_128bppABGR(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL GammaConvert_128bppABGR_64bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL GammaConvert_64bppARGB_128bppABGR(const PipelineParams *, const ScanOpParams *);

// SrcOver: A 'SourceOver' alpha-blend operation. (PTernary operation)

VOID FASTCALL SrcOver_64bppPARGB_64bppPARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL SrcOver_64bppPARGB_64bppPARGB_MMX(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL SrcOver_128bppPABGR_128bppPABGR(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL SrcOver_128bppPABGR_128bppPABGR_SSE2(const PipelineParams *pPP, const ScanOpParams *pSOP);

// SrcOver: These operations SrcOver with an opaque source so they are independent of gamma.

VOID FASTCALL SrcOver_32bppRGB_32bppPARGB(const PipelineParams *pPP, const ScanOpParams *pSOP);
VOID FASTCALL SrcOver_32bppRGB_32bppRGB(const PipelineParams *pPP, const ScanOpParams *pSOP);


// SrcOverAL: "AL" stands for "assume linear" (see soblend.cpp). (PTernary
//            operation)

VOID FASTCALL SrcOverAL_32bppPARGB_555(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL SrcOverAL_32bppPARGB_555_MMX(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL SrcOverAL_32bppPARGB_24(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL SrcOverAL_32bppPARGB_24BGR(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL SrcOverAL_32bppPARGB_565(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL SrcOverAL_32bppPARGB_565_MMX(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL SrcOverAL_32bppPARGB_32bppPARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL SrcOverAL_32bppPARGB_32bppPARGB_MMX(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL SrcOverAL_32bppPARGB_32bppPARGB_SSE2(const PipelineParams *pPP, const ScanOpParams *pSOP);

// SrcOverAL_VA: "VA" stands for "vector alpha"
VOID FASTCALL SrcOverAL_VA_32bppPARGB_32bppPARGB(const PipelineParams *, const ScanOpParams *);

// AlphaMultiply: Multiply each component by the alpha value. (Binary operation)

VOID FASTCALL AlphaMultiply_32bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL AlphaMultiply_64bppARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL AlphaMultiply_128bppABGR(const PipelineParams *, const ScanOpParams *);

// AlphaDivide: Divide each component by the alpha value. (Binary operation)

VOID FASTCALL AlphaDivide_32bppPARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL AlphaDivide_64bppPARGB(const PipelineParams *, const ScanOpParams *);
VOID FASTCALL AlphaDivide_128bppPABGR(const PipelineParams *, const ScanOpParams *);

//
// Functions for returning particular kinds of scan operation
//

ScanOpFunc GetOp_SrcOver_or_SrcOverAL(MilPixelFormat::Enum fmtDest, MilPixelFormat::Enum fmtColorData);
ScanOpFunc GetOp_ConvertFormat_InterchangeToNonHalftoned(MilPixelFormat::Enum format);
ScanOpFunc GetOp_ConvertFormat_ToInterchange(MilPixelFormat::Enum format);
ScanOpFunc GetOp_Copy(MilPixelFormat::Enum format);

//
// Functions for finding/checking the 'interchange' format (i.e. used as an
// intermediate format when format converting.)
//

MilPixelFormat::Enum GetNearestInterchangeFormat(MilPixelFormat::Enum fmt);
BOOL IsInterchangeFormat(MilPixelFormat::Enum fmt);





