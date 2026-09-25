// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:  
//
//      The "SrcOver" and "SrcOverAL" scan operations. See scanoperation.h
//

#include "precomp.hpp"

//+-----------------------------------------------------------------------------
//
//  Operation Description:
//
//      SrcOver:    PTernary operation; does a SrcOver alpha-blend.
//      SrcOverAL:  "AL" stands for "assume linear". Does the same operation as
//                  SrcOver, but because it does so directly in a non-linear
//                  color space (sRGB), the result is not correct. For an sRGB
//                  destination, SrcOverAL is faster than true SrcOver.
//
//
//  Inputs:
//
//      pSOP->m_pvDest:   The destination scan - write using this pointer.
//      pSOP->m_pvSrc1:   The source data to be blended
//      pSOP->m_pvSrc2:   The destination scan - read using this pointer. May
//                        equal m_pvDest.
//      pPP->m_uiCount:   Scan length, in pixels.
//
//  Return Value:
//
//      None
//
//  Notes:
//
//      This is a pseudo-ternary operation. We take pixels from 'm_pvSrc2',
//      blend pixels from 'm_pvSrc1' over them, and write the result to
//      'm_pvDest'.
//
//      m_pv*:
//          Since the formats of the 'm_pvDest' and 'm_pvSrc2' scans are the
//          same for all the blend functions we implement, the naming is
//          simplified to list just the format of the source, then the format of
//          the destination.
//
//          m_pvDest and m_pvSrc2 may be equal; otherwise, they must point to
//          scans which do not overlap in memory.
//
//      WriteRMW:
//          The blend operation adheres to the following rule:
//          "If the blending color value is zero, do not write the destination
//          pixel."
//
//          In other words, it is also a 'WriteRMW' operation. This allows us to
//          avoid a separate 'WriteRMW' step in some cases. See SOReadRMW.cpp
//          and SOWriteRMW.cpp.
//
//      "Pseudo-ternary":
//          The impact of this is that Blend is not a "true" ternary operation.
//          Remember, if a blend pixel is transparent, NOTHING gets written to
//          the corresponding destination pixel - the m_pvSrc2 pixel is not
//          copied to the destination like a true ternary operation would.
//
//      "Superluminosity":
//          A premultiplied-alpha pixel is "superluminous" if one or more color
//          channel values is greater than the alpha value. We do not support
//          superluminosity, hence the following rules:
//
//          * Premultiplied colors input to milcore, need to be processed to
//            remove superluminosity.
//
//          * Internal operations in milcore, should not introduce
//            superluminosity.
//
//          * Alpha-blending implementations are free to handle superluminosity
//            in whatever way is most expedient.
//
//          * BUT: If it doesn't impact performance significantly,
//            alpha-blending implementations should choose to NOT support
//            superluminosity. This option presents itself particularly when the
//            alpha is zero.
//
//          See task #29908.
//

// SrcOverAL 32bppPARGB over 32bppPARGB

VOID FASTCALL
SrcOverAL_32bppPARGB_32bppPARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS_BLEND(ARGB, ARGB)

    UINT uiCount = pPP->m_uiCount;
    Assert(uiCount>0);

    UINT32 dstPixel;
    do {
        UINT32 blendPixel = *pSrc;
        UINT32 alpha = blendPixel >> 24;

        // If blendPixel is zero, skip everything, including writing the
        // destination pixel.
        
        // NOTICE-2006/04/06-milesc At one point we had an RMW optimization
        // that would only populate the texels of the destination buffer that
        // had alpha != 0. This would have caused a problem with superluminous
        // colors (though all we'd have to do is modify this code to use color
        // != 0 to get it to work.) Now however we no longer use the RMW
        // optimization, so we should not need to worry about it. I've included
        // this comment here in case someone brings it back and we see garbage
        // given superluminous colors.
        
        if (blendPixel != 0)
        {

            if (alpha == 255)
            {
                dstPixel = blendPixel;
            }
            else
            {
                //
                // dstPixel = blendPixel + (1-Alpha) * dstPixel
                //

                dstPixel = *pDestIn;

                ULONG Multa = 255 - alpha;
                ULONG _D1_00AA00GG = (dstPixel & 0xff00ff00) >> 8;
                ULONG _D1_00RR00BB = (dstPixel & 0x00ff00ff);

                ULONG _D2_AAAAGGGG = _D1_00AA00GG * Multa + 0x00800080;
                ULONG _D2_RRRRBBBB = _D1_00RR00BB * Multa + 0x00800080;

                ULONG _D3_00AA00GG = (_D2_AAAAGGGG & 0xff00ff00) >> 8;
                ULONG _D3_00RR00BB = (_D2_RRRRBBBB & 0xff00ff00) >> 8;

                ULONG _D4_AA00GG00 = (_D2_AAAAGGGG + _D3_00AA00GG) & 0xFF00FF00;
                ULONG _D4_00RR00BB = ((_D2_RRRRBBBB + _D3_00RR00BB) & 0xFF00FF00) >> 8;

                dstPixel = blendPixel + _D4_AA00GG00 + _D4_00RR00BB;

                //
                // Check for overflow caused by superluminosity
                //
                if ((dstPixel & 0x000000FF) < (blendPixel & 0x000000FF))
                {
                    dstPixel -= 0x00000100;
                    dstPixel |= 0x000000FF;
                }
                if ((dstPixel & 0x0000FF00) < (blendPixel & 0x0000FF00))
                {
                    dstPixel -= 0x00010000;
                    dstPixel |= 0x0000FF00;
                }
                if ((dstPixel & 0x00FF0000) < (blendPixel & 0x00FF0000))
                {
                    dstPixel -= 0x01000000;
                    dstPixel |= 0x00FF0000;
                }
            }

            *pDestOut = dstPixel;
        }

        pSrc++;
        pDestIn++;
        pDestOut++;
    } while (--uiCount != 0);
}

VOID FASTCALL
SrcOverAL_32bppPARGB_32bppPARGB_MMX(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
#if defined(_X86_)
    const ARGB64* pDestIn = static_cast<const ARGB64 *>(pSOP->m_pvSrc2); \
    ARGB64* pDestOut = static_cast<ARGB64 *>(pSOP->m_pvDest); \
    const void* pSrc = pSOP->m_pvSrc1;
    
    static const ULONGLONG halfMask=0x0080008000800080;
    UINT uiCount = pPP->m_uiCount;

    _asm {
        mov        ecx,uiCount                  ; ecx=pixel counter
        mov        ebx,pSrc                    ; ebx=blend pixel pointer
        mov        esi,pDestIn                 ; esi=source pixel pointer
        mov        edi,pDestOut                ; edi=dest pixel pointer
        pxor       mm7,mm7                     ; mm7=[0|0|0|0]
        movq       mm3,halfMask

main_loop:
        mov        eax,DWORD ptr [ebx]
        mov        edx,eax                     ; eax=blend pixel
        shr        edx,24                      ; edx=alpha
        cmp        eax,0                       ; For some reason, doing a jz right after a shr stalls
        jz         alpha_blend_done            ; if blend pixel=0, no blending

        cmp        edx,0xFF
        jne        alpha_blend
        mov        [edi],eax                   ; if alpha=0xFF, copy pSrc to dest
        jmp        alpha_blend_done

alpha_blend:
        movd       mm4,eax

        mov        eax,[esi]                   ; eax=source
        movd       mm0,eax                     ; mm0=[0|0|AR|GB]
        punpcklbw  mm0,mm7                     ; mm0=[A|R|G|B]

        xor        edx,0xFF                    ; C=255-Alpha
        movd       mm2,edx                     ; mm2=[0|0|0|C]
        punpcklwd  mm2,mm2                     ; mm2=[0|0|C|C]
        punpckldq  mm2,mm2                     ; mm2=[C|C|C|C]

        pmullw     mm0,mm2
        paddw      mm0,mm3                     ; mm0=[AA|RR|GG|BB]
        movq       mm2,mm0                     ; mm2=[AA|RR|GG|BB]

        psrlw      mm0,8                       ; mm0=[A|R|G|B]
        paddw      mm0,mm2                     ; mm0=[AA|RR|GG|BB]
        psrlw      mm0,8                       ; mm0=[A|R|G|B]

        packuswb   mm0,mm0                     ; mm0=[AR|GB|AR|GB]
        paddusb    mm0,mm4                     ; Add the blend pixel
        movd       edx,mm0                     ; edx=[ARGB] -> result pixel
        mov        [edi],edx

alpha_blend_done:
        add        edi,4
        add        esi,4
        add        ebx,4
        dec        ecx
        jg         main_loop

        emms
    }
#endif
}

// SrcOver from 64bppPARGB to 64bppPARGB.

VOID FASTCALL
SrcOver_64bppPARGB_64bppPARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS_BLEND(ARGB64, ARGB64)
    UINT uiCount = pPP->m_uiCount;
    
    while (uiCount--)
    {
        GpCC64 blendPixel;
        blendPixel.argb = *pSrc;
        UINT16 alpha = blendPixel.a;

        // If alpha is zero, skip everything, including writing the
        // destination pixel. This is needed for the RMW optimization.
        
        if (alpha != 0)
        {
            GpCC64 dstPixel;

            if (alpha == SRGB_ONE)
            {
                dstPixel.argb = blendPixel.argb;
            }
            else
            {
                //
                // Dst = Src + (1-Alpha) * Dst
                //

                dstPixel.argb = *pDestIn;

                INT Multa = SRGB_ONE - alpha;
                
                dstPixel.r = static_cast<UINT16>(((dstPixel.r * Multa + SRGB_HALF) >> SRGB_FRACTIONBITS) + blendPixel.r);
                dstPixel.g = static_cast<UINT16>(((dstPixel.g * Multa + SRGB_HALF) >> SRGB_FRACTIONBITS) + blendPixel.g);
                dstPixel.b = static_cast<UINT16>(((dstPixel.b * Multa + SRGB_HALF) >> SRGB_FRACTIONBITS) + blendPixel.b);
                dstPixel.a = static_cast<UINT16>(((dstPixel.a * Multa + SRGB_HALF) >> SRGB_FRACTIONBITS) + blendPixel.a);
            }

            *pDestOut = dstPixel.argb;
        }

        pSrc++;
        pDestIn++;
        pDestOut++;
    }
}

// SrcOver from 64bppPARGB to 64bppPARGB MMX.

VOID FASTCALL
SrcOver_64bppPARGB_64bppPARGB_MMX(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
#if defined(_X86_)
    const ARGB64* pDestIn = static_cast<const ARGB64 *>(pSOP->m_pvSrc2); \
    ARGB64* pDestOut = static_cast<ARGB64 *>(pSOP->m_pvDest); \
    const void* pSrc = pSOP->m_pvSrc1;
    static const ULONGLONG ullSRGBHalfMask=0x1000100010001000;
    UINT uiCount = pPP->m_uiCount;

    _asm {
        mov        ecx,uiCount                   ; ecx=pixel counter
        mov        ebx,pSrc                     ; ebx=blend pixel pointer
        mov        esi,pDestIn                       ; esi=source pixel pointer
        mov        edi,pDestOut                       ; edi=dest pixel pointer
        movq       mm4,ullSRGBHalfMask         ; mm4=mask with srgb half

main_loop:
        movsx      eax,word ptr [ebx+3*2]      ; eax=alpha
        or         eax,eax                     ; eax==0?
        jz         alpha_blend_done            ; if alpha=0, no blending

        movq       mm0,[ebx]                   ; mm0=blend pixel
        cmp        eax,SRGB_ONE                ; if alpha=SRGB_ONE, dest=blend
        jne        alpha_blend
        movq       [edi],mm0                   ; copy blend pixel to dest
        jmp        alpha_blend_done

alpha_blend:
        ; Get SRGB_ONE-Alpha
        neg        eax
        add        eax,SRGB_ONE                ; C=SRGB_ONE-Alpha
        movd       mm2, eax                    ; mm2=[0|0|0|C]
        punpcklwd  mm2, mm2
        punpckldq  mm2, mm2                    ; mm2=[C|C|C|C]

        ; Blend pixels
        movq       mm1,[esi]                   ; mm1=[A|R|G|B] source pixel
        movq       mm3,mm1                     ; mm3=[A|R|G|B] source pixel
        pmullw     mm1,mm2                     ; low word of source*C
        paddw      mm1,mm4                     ; add an srgb half for rounding
        psrlw      mm1,SRGB_FRACTIONBITS       ; truncate low SRGB_FRACTIONBITS
        pmulhw     mm3,mm2                     ; high word of source*C
        psllw      mm3,SRGB_INTEGERBITS        ; truncate high SRGB_INTEGERBITS
        por        mm1,mm3                     ; mm1=[A|R|G|B]
        paddw      mm1,mm0                     ; add blend pixel
        movq       [edi],mm1                   ; copy result to dest

alpha_blend_done:
        add        edi,8
        add        esi,8
        add        ebx,8

        dec        ecx
        jg         main_loop
        emms
    }
#endif
}

// Note: For SrcOverAL_32bppPARGB_555 and SrcOverAL_32bppPARGB_565, see
//       sodither.cpp.


// SrcOverAL from 32bppPARGB to RGB24.

VOID FASTCALL
SrcOverAL_32bppPARGB_24(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS_BLEND(ARGB, BYTE)
    UINT uiCount = pPP->m_uiCount;
    
    Assert(uiCount>0);
    
    do {

        if (((UINT_PTR) pDestOut & 0x3) == 0)
        {
            while (uiCount >= 4)
            {
                BYTE *bb = (BYTE *) pSrc;

                if ((bb[3] & bb[7] & bb[11] & bb[15]) != 0xFF)
                {
                    break;
                }

                ((UINT32 *) pDestOut)[0] = (bb[4] << 24)  | (bb[2] << 16)  | (bb[1] << 8)  | bb[0];
                ((UINT32 *) pDestOut)[1] = (bb[9] << 24)  | (bb[8] << 16)  | (bb[6] << 8)  | bb[5];
                ((UINT32 *) pDestOut)[2] = (bb[14] << 24) | (bb[13] << 16) | (bb[12] << 8) | bb[10];

                uiCount -= 4;
                pSrc += 4;
                pDestOut += 12;
                pDestIn += 12;
            }
        }
        
        if (uiCount == 0)
        {
            break;
        }

        UINT32 blendPixel = *pSrc;
        UINT32 alpha = blendPixel >> 24;

        if (alpha != 0)
        {
            UINT32 dstPixel;

            if (alpha == 255)
            {
                dstPixel = blendPixel;
            }
            else
            {
                // Dst = Src + (1-Alpha) * Dst

                UINT32 multA = 255 - alpha;

                UINT32 D1_000000GG = *(pDestIn + 1);
                UINT32 D2_0000GGGG = D1_000000GG * multA + 0x00800080;
                UINT32 D3_000000GG = (D2_0000GGGG & 0xff00ff00) >> 8;
                UINT32 D4_0000GG00 = (D2_0000GGGG + D3_000000GG) & 0xFF00FF00;

                UINT32 D1_00RR00BB = *(pDestIn) | (ULONG) *(pDestIn + 2) << 16;
                UINT32 D2_RRRRBBBB = D1_00RR00BB * multA + 0x00800080;
                UINT32 D3_00RR00BB = (D2_RRRRBBBB & 0xff00ff00) >> 8;
                UINT32 D4_00RR00BB = ((D2_RRRRBBBB + D3_00RR00BB) & 0xFF00FF00) >> 8;

                dstPixel = (D4_0000GG00 | D4_00RR00BB) + blendPixel;
            }

            *(pDestOut)     = (BYTE) (dstPixel);
            *(pDestOut + 1) = (BYTE) (dstPixel >> 8);
            *(pDestOut + 2) = (BYTE) (dstPixel >> 16);
        }

        pSrc++;
        pDestOut += 3;
        pDestIn += 3;
    } while (--uiCount != 0);
}

// SrcOverAL from 32bppPARGB to 24bppBGR.

VOID FASTCALL
SrcOverAL_32bppPARGB_24BGR(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS_BLEND(ARGB, BYTE)
    UINT uiCount = pPP->m_uiCount;
    
    Assert(uiCount>0);
    
    do {
        UINT32 blendPixel = *pSrc;
        UINT32 alpha = blendPixel >> 24;

        if (alpha != 0)
        {
            UINT32 dstPixel;

            if (alpha == 255)
            {
                dstPixel = blendPixel;
            }
            else
            {
                // Dst = Src + (1-Alpha) * Dst

                UINT32 multA = 255 - alpha;

                UINT32 D1_000000GG = *(pDestIn + 1);
                UINT32 D2_0000GGGG = D1_000000GG * multA + 0x00800080;
                UINT32 D3_000000GG = (D2_0000GGGG & 0xff00ff00) >> 8;
                UINT32 D4_0000GG00 = (D2_0000GGGG + D3_000000GG) & 0xFF00FF00;

                UINT32 D1_00RR00BB = *(pDestIn + 2) | (ULONG) *(pDestIn) << 16;
                UINT32 D2_RRRRBBBB = D1_00RR00BB * multA + 0x00800080;
                UINT32 D3_00RR00BB = (D2_RRRRBBBB & 0xff00ff00) >> 8;
                UINT32 D4_00RR00BB = ((D2_RRRRBBBB + D3_00RR00BB) & 0xFF00FF00) >> 8;

                dstPixel = (D4_0000GG00 | D4_00RR00BB) + blendPixel;
            }

            *(pDestOut)     = (BYTE) (dstPixel >> 16);
            *(pDestOut + 1) = (BYTE) (dstPixel >> 8);
            *(pDestOut + 2) = (BYTE) (dstPixel);
        }

        pSrc++;
        pDestOut += 3;
        pDestIn += 3;
    } while (--uiCount != 0);
}

// SrcOver 128bppPABGR over 128bppPABGR.

VOID FASTCALL
SrcOver_128bppPABGR_128bppPABGR(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS_BLEND(MilColorF, MilColorF)
    UINT uiCount = pPP->m_uiCount;

    Assert(uiCount>0);

    MilColorF dstPixel;
    do {
        MilColorF blendPixel = *pSrc;
        FLOAT OneMinusAlpha = (FLOAT)(1.0 - blendPixel.a);

        //
        // Dst = Src + (1-Alpha) * Dst
        //

        dstPixel = *pDestIn;

        dstPixel.r = dstPixel.r * OneMinusAlpha + blendPixel.r;
        dstPixel.g = dstPixel.g * OneMinusAlpha + blendPixel.g;
        dstPixel.b = dstPixel.b * OneMinusAlpha + blendPixel.b;
        dstPixel.a = dstPixel.a * OneMinusAlpha + blendPixel.a;

        *pDestOut = dstPixel;

        pSrc++;
        pDestIn++;
        pDestOut++;
    } while (--uiCount != 0);
}


// SrcOverAL_VA 32bppPARGB over 32bppPARGB.

VOID FASTCALL
SrcOverAL_VA_32bppPARGB_32bppPARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    const ARGB* pColors = static_cast<const ARGB *>(pSOP->m_pvSrc1);
    const ARGB* pAlphas = static_cast<const ARGB *>(pSOP->m_pvSrc2);
          ARGB* pDest   = static_cast<      ARGB *>(pSOP->m_pvDest);

    Assert(pPP->m_uiCount > 0);

    for (UINT i = 0, n = pPP->m_uiCount; i < n; i++)
    {
        UINT32& dest = pDest[i];
        UINT32 colors = pColors[i];
        UINT32 alphas = pAlphas[i];

        if (alphas == 0) continue;
        if (alphas == 0xFFFFFF)
        {
            dest = colors;
            continue;
        }

        //
        // Dest = Dest*(1-Alpha) + Color
        //


        ULONG alphaR = (alphas >> 16) & 0xff;
        ULONG alphaG = (alphas >>  8) & 0xff;
        ULONG alphaB = (alphas      ) & 0xff;

        ULONG _D1_00RR0000 = dest & 0x00ff0000;
        ULONG _D1_0000GG00 = dest & 0x0000ff00;
        ULONG _D1_000000BB = dest & 0x000000ff;

        ULONG _D2_00GGGG00 = _D1_0000GG00 * (255-alphaG) + 0x00008000;
        ULONG _D2_RRRRBBBB = _D1_00RR0000 * (255-alphaR) + _D1_000000BB * (255-alphaB) + 0x00800080;

        ULONG _D3_0000GG00 = (_D2_00GGGG00 & 0x00ff0000) >> 8;
        ULONG _D3_00RR00BB = (_D2_RRRRBBBB & 0xff00ff00) >> 8;

        ULONG _D4_00GG0000 = (_D2_00GGGG00 + _D3_0000GG00) & 0x00FF0000;
        ULONG _D4_RR00BB00 = (_D2_RRRRBBBB + _D3_00RR00BB) & 0xFF00FF00;

        ULONG _D5_00RRGGBB = (_D4_00GG0000 + _D4_RR00BB00) >> 8;
        dest = _D5_00RRGGBB + colors;
    }
}

// SrcOver 32bppRGB over 32bppPARGB
//
// These operations source over with an oqaque source so they are independent of gamma. 

VOID FASTCALL
SrcOver_32bppRGB_32bppPARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS_BLEND_NO_IN(ARGB, ARGB)

    UINT uiCount = pPP->m_uiCount;
    Assert(uiCount>0);

    do {
        *pDestOut = *pSrc | MIL_ALPHA_MASK;
        pSrc++;
        pDestOut++;
    } while (--uiCount != 0);
}

// SrcOver 32bppRGB over 32bppRGB

VOID FASTCALL
SrcOver_32bppRGB_32bppRGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS_BLEND_NO_IN(ARGB, ARGB)

    UINT uiCount = pPP->m_uiCount;
    Assert(uiCount>0);

    GpMemcpy(pDestOut, pSrc, uiCount*sizeof(ARGB));
}




