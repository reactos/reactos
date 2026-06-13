// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:  
//
//      SSE2-optimized blending functions. See soblend.cpp for C equivalents
//      (and more documentation).
//

#include "precomp.hpp"

#if !defined(_ARM_) && !defined(_ARM64_)
#include "xmmintrin.h"
#elif defined (_ARM64_) 
#include "arm64_neon.h"
#endif

// See soblend.cpp for a description of SrcOver and SrcOverAL

//+-----------------------------------------------------------------------------
//
//  Function:  SrcOver_128bppPABGR_128bppPABGR_SSE2
//
//  Synopsis:  SrcOver blend 128bppPABGR over 128bppPABGR; implemented using
//             SSE2 assembly.
//
//------------------------------------------------------------------------------

VOID FASTCALL
SrcOver_128bppPABGR_128bppPABGR_SSE2(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
#if defined(_X86_)
    DEFINE_POINTERS_BLEND(MilColorF, MilColorF)
    UINT uiCount = pPP->m_uiCount;

    Assert(uiCount>0);

    if ((INT_PTR) pDestOut & 0xF)
    {
        for (UINT i=0; i < uiCount; i++)
        {
            __m128 _xmm0 = _mm_loadu_ps( reinterpret_cast<const float *>(pSrc));
            __m128 _xmm1 = _mm_loadu_ps( reinterpret_cast<const float *>(pDestIn));
            __m128 _xmm3 = _xmm1;
            __m128 _xmm2 = _xmm0;
            _xmm2 = _mm_shuffle_ps(_xmm2,_xmm2, 0xFF);  // Source alpha in each channel
            _xmm1 = _mm_mul_ps(_xmm1, _xmm2);
            _xmm3 = _mm_sub_ps(_xmm3, _xmm1);           // dest - alpha*dest == (1 - alpha) * dest
            _xmm0 = _mm_add_ps(_xmm0, _xmm3);
            _mm_storeu_ps(reinterpret_cast<float *>(pDestOut), _xmm0);
            pDestOut ++;
            pSrc ++;
            pDestIn ++;     
        }
    }
    else
    {
        // We could remove these cases if we were guaranteed 16-byte alignment
        // of 128bpp data. Task #29789.
        
        if (((INT_PTR) pSrc | (INT_PTR) pDestIn) & 0xF)
        {
            for (UINT i=0; i < uiCount; i++)
            {
                __m128 _xmm0 = _mm_loadu_ps(reinterpret_cast<const float *>(pSrc));
                __m128 _xmm1 = _mm_loadu_ps(reinterpret_cast<const float *>(pDestIn));
                __m128 _xmm3 = _xmm1;
                __m128 _xmm2 = _xmm0;
                _xmm2 = _mm_shuffle_ps(_xmm2,_xmm2, 0xFF);  // Source alpha in each channel
                _xmm1 = _mm_mul_ps(_xmm1, _xmm2);
                _xmm3 = _mm_sub_ps(_xmm3, _xmm1);           // dest - alpha*dest == (1 - alpha) * dest
                _xmm0 = _mm_add_ps(_xmm0, _xmm3);
                _mm_store_ps(reinterpret_cast<float *>(pDestOut), _xmm0);
                pDestOut ++;
                pSrc ++;
                pDestIn ++;     
            }
        }
        else
        {
            for (UINT i=0; i < uiCount; i++)
            {
                __m128 _xmm0 = _mm_load_ps(reinterpret_cast<const float *>(pSrc));
                __m128 _xmm1 = _mm_load_ps(reinterpret_cast<const float *>(pDestIn));
                __m128 _xmm3 = _xmm1;
                __m128 _xmm2 = _xmm0;
                _xmm2 = _mm_shuffle_ps(_xmm2,_xmm2, 0xFF);  // Source alpha in each channel
                _xmm1 = _mm_mul_ps(_xmm1, _xmm2);
                _xmm3 = _mm_sub_ps(_xmm3, _xmm1);           // dest - alpha*dest == (1 - alpha) * dest
                _xmm0 = _mm_add_ps(_xmm0, _xmm3);
                _mm_store_ps(reinterpret_cast<float *>(pDestOut), _xmm0);
                pDestOut ++;
                pSrc ++;
                pDestIn ++;     
            }
        }
    }
#endif // defined(_X86_)
}


//+-----------------------------------------------------------------------------
//
//  Function:  SrcOverAL_32bppPARGB_32bppPARGB_SSE2_4Pixel
//
//  Synopsis:  SrcOverAL blend 32bppPARGB over 32bppPARGB, 4 pixels at a time.
//             Implemented using SSE2 assembly.
//
//------------------------------------------------------------------------------

// SrcOverAL 32bppPARGB over 32bppPARGB 4-pixel ASM inner loop

#if defined(_X86_)
void SrcOverAL_32bppPARGB_32bppPARGB_SSE2_4Pixel(const ARGB *pSrc, const ARGB *pDestIn, ARGB* pDestOut, UINT ui4PixelGroups)
{
    __declspec(align(16)) static const DWORD roundbits[4] = { 0x00800080, 0x00800080, 0x00800080, 0x00800080 }; 

    _asm {
        mov        ecx,ui4PixelGroups   // number of 4-pixel groups
        mov        esi,pSrc            // Src (blend) pixel pointer
        mov        ebx,pDestIn         // DestIn pixel pointer
        mov        edi,pDestOut        // destOut pixel pointer
        
        pxor       xmm7, xmm7
        movdqa     xmm5, roundbits

align 16
Four_pixel_loop:

        //
        // Early-out if all blend pixels are transparent
        //

        // Test to see if the source alpha for all 4 pixels is 0x00.  If so, we don't need to modify the
        // destination (the source is transparent) and can shortcut the computations below.
        mov        eax, [esi]
        mov        edx, eax
        or         eax, [esi+4]
        or         eax, [esi+8]
        or         eax, [esi+12]
        lea        esi, [esi+16]
        jz         prepare_next_loop
        
        //
        // Load 4 source pixels into xmm0
        //

        // This nasty little jump is a perf optimization.  Since alignment is highly predictable, we don't want to handle
        // misalignment unless we have to.

        test       esi, 0xF
        jne        misaligned_src1
        movdqa     xmm0, [esi-16]   // [Sa3,Sr3,Sg3,Sb3, Sa2,Sr2,Sg2,Sb2, Sa1,Sr1,Sg1,Sb1, Sa0,Sr0,Sg0,Sb0]
        jmp        cont_src1
misaligned_src1:
        // Avoid load splits for misaligned pSrc.
        // These 7 instructions replace:
        //
        //   movdqu xmm0, [esi-16]
        //
        
        movd       xmm0, [esi-16]
        movd       xmm1, [esi-16+4]
        movd       xmm2, [esi-16+8]
        movd       xmm3, [esi-16+12]
        punpckldq  xmm0, xmm1
        punpckldq  xmm2, xmm3
        punpcklqdq xmm0, xmm2

cont_src1:

        //
        // Early-out if all blend pixels are opaque. 
        // (Just copy the source to the destination.)
        //

        and        edx, [esi+4-16]     
        and        edx, [esi+8-16]
        and        edx, [esi+12-16]
        cmp        edx, 0xFF000000
        jae        store_pixels

        //
        // Load 4 destIn pixels into xmm1
        //
    
        // This nasty little jump is a perf optimization.  Since alignment is highly predictable, we don't want to handle
        // misalignment unless we have to.
        test       ebx, 0xF
        jne        misaligned_src2
        movdqa     xmm1, [ebx]      // [Da3,Dr3,Dg3,Db3, Da2,Dr2,Dg2,Db2, Da1,Dr1,Dg1,Db1, Da0,Dr0,Dg0,Db0]        
        jmp        cont_src2
misaligned_src2:
        // Avoid load splits for misaligned pDestIn.
        movd       xmm1, [ebx]
        movd       xmm2, [ebx+4]
        movd       xmm3, [ebx+8]
        movd       xmm4, [ebx+12]
        punpckldq  xmm1, xmm2
        punpckldq  xmm3, xmm4
        punpcklqdq xmm1, xmm3
cont_src2:
        //
        // Main alpha-blending code operating on 4 pixels
        //

        // Alpha blending algorithm is 
        //      temp = [255 - alpha(source)] * color(pDestIn)//
        //  pDestOut =  [ ((temp + 0x80) >> 8) + (temp + 0x80) ]>>8
        //   Which is similar to divide-by-255.

        // Unpack DestIn
        movdqa     xmm3, xmm1       
        punpcklbw  xmm1, xmm7       // xmm1 = [Da1,Dr1,Dg1,Db1, Da0,Dr0,Dg0,Db0]
        punpckhbw  xmm3, xmm7       // xmm3 = [Da3,Dr3,Dg3,Db3, Da2,Dr2,Dg2,Db2]
        
        // Calculate 255-alpha (abbreviated 'Sa').
        pxor       xmm2, xmm2       // break dependency
        pcmpeqb    xmm2, xmm2       // xmm2 = [255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255]
        psubb      xmm2, xmm0       // xmm2 = [Sa3 X X X Sa2 X X X Sa1 X X X Sa0 X X X]
        psrld      xmm2, 24         // xmm2 = [0 0 0 Sa3 0 0 0 Sa2 0 0 0 Sa1 0 0 0 Sa0]
                                    //      = [  0 Sa3   0 Sa2   0 Sa1   0 Sa0]

        // Unpack Sa

        pshuflw    xmm2, xmm2, 0xa0 // xmm2 = [  0 Sa3   0 Sa2 Sa1 Sa1 Sa0 Sa0]
        pshufhw    xmm2, xmm2, 0xa0 // xmm2 = [Sa3 Sa3 Sa2 Sa2 Sa1 Sa1 Sa0 Sa0]

        pshufd     xmm6, xmm2, 0xE  // xmm6 = [Sa0 Sa0 Sa0 Sa0 Sa3 Sa3 Sa2 Sa2]
        
        punpckldq  xmm2, xmm2       // xmm2 = [Sa1 Sa1 Sa1 Sa1 Sa0 Sa0 Sa0 Sa0]
        punpckldq  xmm6, xmm6       // xmm6 = [Sa3 Sa3 Sa3 Sa3 Sa2 Sa2 Sa2 Sa2]


        // Sa * DestIn

        pmullw     xmm1, xmm2       // xmm1 = [Da1*a1,Dr1*a1,Dg1*a1,Db1*a1,Da0*a0,Dr0*a0,Dg0*a0,Db0*a0]
                                    //      = [A1a1,R1r1,G1g1,B1b1,A0a0,R0r0,G0g0,B0b0]
    
        pmullw     xmm3, xmm6
        
        // Divide by 255 (using the approximation 257/65536 for 1/255)
        //
        // The approximation needs to be accurate for the "a == 0" case, since we require that
        // the output be numerically identical to the input in that case.
        
        paddw      xmm1, xmm5       // xmm1 = [A1a1+0x80, ..., B0b0+0x80]
        movdqa     xmm2, xmm1       // xmm2 = [A1a1+0x80, ..., B0b0+0x80]
        psrlw      xmm1, 8          // xmm1 = [A1,R1,G1,B0,A0,R0,G0,B0]
        paddw      xmm1, xmm2       // xmm1 = [((A1a1+0x80)*257)>>8,...,((B0b0+0x80)*257)>>8]
        psrlw      xmm1, 8          // xmm1 = (255-a)*Destin/255

        paddw      xmm3, xmm5
        movdqa     xmm2, xmm3
        psrlw      xmm3, 8
        paddw      xmm3, xmm2
        psrlw      xmm3, 8          // xmm3 = (255-a)*Destin/255

        // Pack, add source pixels, store
        packuswb   xmm1, xmm3
        paddusb      xmm0, xmm1

        // End loop
store_pixels:

        // Write destination        
        test       edi, 0xF
        jz         aligned_dest
        // Splitting misaligned writes into dwords avoids cacheline splits on
        // stores
        movd       [edi], xmm0
        psrldq     xmm0, 4
        movd       [edi+4], xmm0
        psrldq     xmm0, 4
        movd       [edi+8], xmm0
        psrldq     xmm0, 4
        movd       [edi+12], xmm0
        
        jmp        prepare_next_loop        
aligned_dest:
        movdqa     [edi], xmm0

prepare_next_loop:
        add        edi, 16
        add        ebx, 16
        sub        ecx, 1
        jg         Four_pixel_loop
    }
}
#endif


//+-----------------------------------------------------------------------------
//
//  Function:  SrcOverAL_32bppPARGB_32bppPARGB_SSE2
//
//  Synopsis:  SrcOverAL blend 32bppPARGB over 32bppPARGB;
//             implemented using SSE2 assembly.
//
//------------------------------------------------------------------------------

void FASTCALL
SrcOverAL_32bppPARGB_32bppPARGB_SSE2(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
#if defined(_X86_)
    DEFINE_POINTERS_BLEND(ARGB, ARGB)

    UINT uiCount = pPP->m_uiCount;
    Assert(uiCount>0);

    // Early out for small line sizes.  The SSE2 code path is less efficient 
    // than the fallback code for very small spans (spans of 3 pixels or less)
    if (uiCount < 4)
    {
        SrcOverAL_32bppPARGB_32bppPARGB(
            pPP, pSOP);
        
        return;
    }

    // The ASM code below wants to use 16-byte aligned stores, but the
    // destination alignment is only guaranteed to 4 byte. We process the first
    // few pixels until we're at a 16-byte aligned starting point.  Then we
    // enter the ASM code and process 4 pixels per iteration. The last few
    // straggler pixels are cleaned up by another call to the "C" version of
    // this code.

    UINT uiDestOutOffset = static_cast<UINT>(reinterpret_cast<INT_PTR>(pDestOut) & 0xF);

    // To reduce overhead of this function for small loops, we bypass 
    // destination alignment when writing fewer than 12 pixels
    if (uiDestOutOffset && (uiCount > 11))
    {
        // We're guaranteed at least 4-byte alignment
        Assert((uiDestOutOffset & 3) == 0);

        UINT nAlignmentPixels = min(uiCount, (16 - uiDestOutOffset) >> 2);
    
        PipelineParams oPipelineParams;
        ScanOpParams oScanOpParams;

        oScanOpParams.m_pvSrc2 = pDestIn;
        oScanOpParams.m_pvDest = pDestOut;
        oScanOpParams.m_pvSrc1 = pSrc;
        oPipelineParams.m_uiCount = nAlignmentPixels;

        SrcOverAL_32bppPARGB_32bppPARGB(
            &oPipelineParams, &oScanOpParams);

        // update the pixel count to reflect the odd pixels at the beginning
        uiCount -= nAlignmentPixels;
        pDestIn += nAlignmentPixels;
        pDestOut += nAlignmentPixels;
        pSrc += nAlignmentPixels;
    }

    UINT ui4PixelGroups = uiCount >> 2;

    if (ui4PixelGroups)
    {
        SrcOverAL_32bppPARGB_32bppPARGB_SSE2_4Pixel(
            pSrc, 
            pDestIn, 
            pDestOut, 
            ui4PixelGroups);
    }

    if (uiCount & 3)
    {
        //
        // Blend the remaining few pixels 
        //

        // The main asm loop didn't update uiCount or the pointers, so
        // calculate the number of pixels it processed.

        UINT nPixelsProcessed = uiCount & ~3;
    
        PipelineParams oPipelineParams;
        ScanOpParams oScanOpParams;

        oScanOpParams.m_pvSrc2 = pDestIn + nPixelsProcessed;
        oScanOpParams.m_pvDest = pDestOut + nPixelsProcessed;
        oScanOpParams.m_pvSrc1 = pSrc + nPixelsProcessed;
        oPipelineParams.m_uiCount = uiCount & 3;

        SrcOverAL_32bppPARGB_32bppPARGB(
            &oPipelineParams, &oScanOpParams);
 
    }
#endif // defined(_X86_)
}




