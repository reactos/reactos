// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:  
//
//      Include file to generate either 5-5-5 or 5-6-5 versions of the dither
//      code.
//
//  Notes:
//
//      When DITHER_BLEND_555 is #defined to 1, then this file will generate
//      5-5-5 versions of the included routines.
//
//      When DITHER_BLEND_555 is #defined to 0, then we will generate 5-6-5
//      versions.
//

// warning C4740: flow in or out of inline asm code suppresses global optimization
#pragma warning(push)
#pragma warning(disable:4740)

#undef DITHER_ARRAY
#undef RED_SHIFT
#undef GREEN_SHIFT
#undef BLUE_SHIFT
#undef DITHERBLEND_FUNC
#undef DITHER_FUNC

#if DITHER_BLEND_555

    #define DITHER_ARRAY Dither555
    #define RED_SHIFT 9
    #define GREEN_SHIFT 6
    #define BLUE_SHIFT 3
    #define DITHERBLEND_FUNC SrcOverAL_32bppPARGB_555_MMX
    #define DITHER_FUNC Dither_32bppARGB_555_MMX

#else

    #define DITHER_ARRAY Dither565
    #define RED_SHIFT 8
    #define GREEN_SHIFT 5
    #define BLUE_SHIFT 3
    #define DITHERBLEND_FUNC SrcOverAL_32bppPARGB_565_MMX
    #define DITHER_FUNC Dither_32bppARGB_565_MMX
    
#endif

// Do a dithered blend to 16bpp using MMX.
// This one is not ternary - m_pvSrc2 is ignored

VOID FASTCALL
DITHERBLEND_FUNC(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
#if defined(_X86_)

    WORD* pDestOut = static_cast<WORD *>(pSOP->m_pvDest);
    const ARGB* pSrc = static_cast<const ARGB *>(pSOP->m_pvSrc1);
    UINT uiCount = pPP->m_uiCount;
    
    Assert(uiCount != 0);

    static const ULONGLONG redBlueMask = 0x00f800f800f800f8;
    static const ULONGLONG flipAlphaBits = 0x00ff00ff00ff00ff;

#if DITHER_BLEND_555
    static const ULONGLONG greenMask = 0x0000f8000000f800;
    static const ULONGLONG redBlueMultiplier = 0x0400000104000001;
#else
    static const ULONGLONG greenMask = 0x0000fc000000fc00;
    static const ULONGLONG redBlueMultiplier = 0x0800000108000001;
#endif

    INT x = pPP->m_iX;
    INT y = pPP->m_iY;

#pragma prefast (push)
#pragma prefast (disable: 37001 37002 37003, "The max value of 8 * (y & 3) is 24 which is within valid range.")
    const UINT32 *dither = &DITHER_ARRAY[8 * (y & 3)];
#pragma prefast (pop)
    UINT32 ditherIncrement = (x & 3) * 4;               

    _asm
    {
        ; ecx = uiCount
        ; esi = source
        ; edi = destination
        ; mm4 = red and blue mask (0xf800f8)
        ; mm5 = green mask (0x00fc00) (0x00f800 for 5-5-5)
        ; mm6 = C1 | C0 dither
        ; mm7 = C3 | C2 dither

        mov             eax, ditherIncrement
        mov             esi, pSrc
        mov             edi, pDestOut
        mov             ecx, uiCount
        movq            mm4, redBlueMask
        movq            mm5, greenMask
    
        ; We always want our qword reads from the screen to be aligned.
        ; So if the initial pixel is not qword-aligned, we handle up to
        ; three pixels up front to make it qword-aligned.
        ;
        ; (Note that as a consequence of us aligning to the destination,
        ; we're often doing unaligned reads on the source.  But it's
        ; a much bigger performance win to align operations to the screen
        ; than to system memory, due to the terrible screen read
        ; performance.)

alignment_loop:
        add             eax, dither
        test            edi, 6
        movq            mm6, [eax]      
        movq            mm7, [eax+8]    
        jz              done_start_alignment
        call            do_single_pixel        

        ; Adjust our pointers and load our new dither values:

        mov             eax, ditherIncrement
        add             eax, 4
        and             eax, 0x0000000F
        mov             ditherIncrement, eax
        add             esi, 4
        add             edi, 2
        dec             ecx
        jz              all_done
        jmp             alignment_loop

    done_start_alignment:
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    
    do_main_loop:
        sub             ecx, 4                  ; pre-decrement by 4
        jl              do_pair

        ; We do chunks of 4 pixels at a time so that we can unroll our
        ; dither loop (our dither repeats every 4 pixels).
        
    do_main_loop_2:
        mov             al, [esi+3]           
        and             al, [esi+7]
        and             al, [esi+11]
        and             al, [esi+15]
        inc             al                      ; if all alphas were 0xff, this
        jnz             do_pair                 ;   will wrap to zero
    

        ; The four pixels starting at [esi] are opaque.  We only need to
        ; dither them and convert to 16bpp.  The following codepath will
        ; process all four in parallel (two at a time) in order to optimize
        ; usage of the execution units and minimize dependencies between
        ; consecutive instructions.
            
        ; We start by reading the four pixels into mm0 and mm1, adding
        ; the dither component, and then breaking into group 0 (pixels 0
        ; and 2) and group 1 (pixels 1 and 3).  I will use **0** and **1**
        ; in the comments below to show which pixel group the instruction is
        ; processing

        movq            mm0, [esi]              ; mm0 = DW1 | DW0
        movq            mm1, [esi + 8]          ; mm1 = DW3 | DW2

        paddusb         mm0, mm6                ; add dither
        movq            mm2, mm0
        paddusb         mm1, mm7                ; add dither

        add             edi, 8
        
        punpckhdq       mm2, mm1         ; **1**  mm2 = DW3 | DW1
        punpckldq       mm0, mm1         ; **0**  mm0 = DW2 | DW0
        
        movq            mm3, mm2         ; **1**
        pand            mm2, mm4         ; **1**  red and blue
        
        movq            mm1, mm0         ; **0**
        pand            mm0, mm4         ; **0**  red and blue

        pand            mm3, mm5         ; **1**  green

        psrlw           mm0, 3           ; **0**  shift red and blue to lowest 
                                         ; 5 bits in register
        
        ; Note the use of the pmaddwd to simultaneously shift both the red and
        ; blue bits into their appropriate positions.  The constant 
        ; redBlueMultiplier contains four shorts, each of which is equal to
        ; 2^i where i is the number of bits that we need to shift that color
        ; component by in order to attain the correct position in the 16bpp
        ; color.  This is possible only because the red and blue
        ; components lie on different shorts in the 64bits register (green has
        ; been masked earlier), and so we can dedicate an entire 16bit short
        ; to red and to blue.

        pmaddwd         mm2, redBlueMultiplier  ; **1**
                
        add             esi, 16

        pand            mm1, mm5         ; **0**  green

        psrld           mm3, GREEN_SHIFT-3 ; **1**
        
        pmaddwd         mm0, redBlueMultiplier  ; **0**
            
        sub             ecx, 4                  ; pre-decrement for next iteration

        por             mm2, mm3         ; **1**  combine green with red/blue
                                         ;        mm2 = 0  | W3 | 0  | W1
        
        psrld           mm1, GREEN_SHIFT ; **0**

        psllq           mm2, 13          ; **1**  mm2 = W3 | 0  | W1 | 0

        por             mm0, mm1         ; **0**  combine green with red/blue
                                         ;        mm1 = 0  | W2 | 0  | W0

        por             mm0, mm2                ; mm2 = W3 | W2 | W1 | W0
        movq            [edi - 8], mm0

        jge             do_main_loop_2
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    
    do_pair:
        add             ecx, 2                  ; pre-decrement for this iteration
        jl              do_last_pixel
    
        ; We're doing only a single pair of pixels, so swap our dither
        ; values in preparation for the next iteration:
    
        pxor            mm6, mm7
        pxor            mm7, mm6
        pxor            mm6, mm7                ; swap mm6 and mm7
    
        mov             al, [esi+3]
        inc             al
        cmp             al, 1
        ja              do_pair_blend

        mov             al, [esi+7]
        inc             al
        cmp             al, 1
        ja              do_pair_blend

        mov             al, [esi+3]             ; Do we really want this here?
        or              al, [esi+7]
        jz              do_pair_done
    
        movq            mm0, [esi]
        paddusb         mm0, mm7                ; add dither
        movq            mm2, mm0
        pand            mm0, mm5                ; green
        pand            mm2, mm4                ; red and blue
        psrld           mm0, GREEN_SHIFT        ; green (6 for 5-5-5)
        movq            mm3, mm2
        psrld           mm3, BLUE_SHIFT         ; blue
        psrld           mm2, RED_SHIFT          ; red (9 for 5-5-5)
        por             mm0, mm3
        por             mm0, mm2                ; mm0 = X | C1 | X | C0
        movq            mm1, mm0
        psrlq           mm1, 32
        punpcklwd       mm0, mm1                ; mm0 = X | X | C1 | C0
    
        movd            eax, mm0
        cmp             byte ptr [esi+3], 0
        je              do_pair_done_first_write        
        mov             [edi], ax        
    do_pair_done_first_write:
        cmp             byte ptr [esi+7], 0
        je              do_pair_done_second_write
        shr             eax, 16
        mov             [edi+2], ax
    do_pair_done_second_write:
        add             edi, 4
        add             esi, 8
        jmp             do_main_loop
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    
    do_pair_blend:
        movd            mm1, [edi]              ; read destination, X | X | C1 | C0
        punpcklwd       mm1, mm1                ; C1 | C1 | C0 | C0
        psrld           mm1, 16                 ; 0 | C1 | 0 | C0
                                                ;  (trick using single red and
                                                ;  blue mask requires high bits
                                                ;  to be zero)
        movq            mm0, mm1
        movq            mm2, mm1
        pslld           mm1, BLUE_SHIFT         ; blue
        pslld           mm0, GREEN_SHIFT        ; green (6 for 5-5-5)
        pslld           mm2, RED_SHIFT          ; red (9 for 5-5-5)
        por             mm1, mm2                ; combine red and blue
        pand            mm1, mm4                ; leave valid red and blue bits
        pand            mm0, mm5                ; leave valid green bits
        por             mm1, mm0                ; mm1 = C1 | C0        
    
        ; Okay now we've got the destination read and split.  Handle the first 
        ; blend:
    
        movd            mm2, [esi]
        punpcklbw       mm2, mm2
        psrlw           mm2, 8                  ; mm2 = S
        movq            mm3, mm2
        punpckhwd       mm3, mm3
        punpckhdq       mm3, mm3                ; mm3 = alpha
        movq            mm0, mm1
        punpcklbw       mm0, mm0
        psrlw           mm0, 8                  ; mm0 = D
    #if NO_PREMULTIPLIED_ALPHA
        psubw           mm2, mm0               
        pmullw          mm2, mm3                ; mm2 = alpha * (S - D)
        movq            mm3, mm2
        psrlw           mm3, 8
        paddw           mm2, mm3                ; approximate x/255 by 257/65536
        psrlw           mm2, 8                  ; mm2 = alpha * (S - D)
        paddw           mm0, mm2                ; mm0 = C0 = D + alpha * (S - D)
    #else
        pxor            mm3, flipAlphaBits
        pmullw          mm0, mm3                ; mm2 = (255 - alpha) * D
        movq            mm3, mm0
        psrlw           mm0, 8                  ; approximate x/255 by 257/65536
        paddw           mm0, mm3                ; mm2 = (255 - alpha) * D / 255
        psrlw           mm0, 8                  ; don't care about rounding, not enough bits
        paddw           mm0, mm2                ; mm0 = C0 = S + (1 - alpha) * D
    #endif
    
        ; Handle the second blend (change mm0 to mm1):
    
        movd            mm2, [esi+4]
        punpcklbw       mm2, mm2
        psrlw           mm2, 8                  ; mm2 = S
        movq            mm3, mm2
        punpckhwd       mm3, mm3
        punpckhdq       mm3, mm3                ; mm3 = alpha
        punpckhbw       mm1, mm1
        psrlw           mm1, 8                  ; mm1 = D
    #if NO_PREMULTIPLIED_ALPHA
        psubw           mm2, mm1               
        pmullw          mm2, mm3                ; mm2 = alpha * (S - D)
        movq            mm3, mm2
        psrlw           mm3, 8
        paddw           mm2, mm3                ; approximate x/255 by 257/65536
        psrlw           mm2, 8                  ; mm2 = alpha * (S - D)
        paddw           mm1, mm2                ; mm1 = C1 = D + alpha * (S - D)
    #else
        pxor            mm3, flipAlphaBits    
        pmullw          mm1, mm3                ; mm2 = (255 - alpha) * D
        movq            mm3, mm1
        psrlw           mm1, 8                  ; approximate x/255 by 257/65536
        paddw           mm1, mm3                ; mm2 = (255 - alpha) * D / 255
        psrlw           mm1, 8                  ; don't care about rounding, not enough bits
        paddw           mm1, mm2                ; mm1 = C1 = S + (1 - alpha) * D
    #endif
        packuswb        mm0, mm1                ; mm0 = C1 | C0
    
        ; Dither and pack everything back up:
    
        paddusb         mm0, mm7                ; add dither
        movq            mm2, mm0
        pand            mm0, mm5                ; green
        pand            mm2, mm4                ; red and blue
        psrld           mm0, GREEN_SHIFT        ; green
        movq            mm3, mm2
        psrld           mm3, BLUE_SHIFT         ; blue
        psrld           mm2, RED_SHIFT          ; red
        por             mm0, mm3
        por             mm0, mm2                ; mm0 = X | C1 | X | C0
        movq            mm1, mm0
        psrlq           mm1, 32                 ; mm1 = 0 | 0 | X | C1
        punpcklwd       mm0, mm1                ; mm0 = X | X | C1 | C0
    
        movd            [edi], mm0
    
    do_pair_done:
        add             edi, 4
        add             esi, 8
        jmp             do_main_loop
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    
    do_single_pixel:
        movd            mm0, [esi]
        mov             al, [esi+3]
        inc             al
        jnz             do_single_blend         ; if not completely opaque 
    
        paddusb         mm0, mm6                ; add dither
        movq            mm2, mm0
        pand            mm0, mm5                ; green
        pand            mm2, mm4                ; red and blue
        psrld           mm0, GREEN_SHIFT
        movq            mm3, mm2
        psrld           mm3, BLUE_SHIFT
        psrld           mm2, RED_SHIFT
        por             mm0, mm3
        por             mm0, mm2                ; mm0 = X | C1 | X | C0
    
        movd            eax, mm0
        mov             [edi], ax        
    do_single_done:
        ret
    
    do_single_blend:
        dec             al
        jz              do_single_done          ; completely transparent pixel
    
        ; alpha is between 0 and 255
    
        movzx           eax, word ptr [edi]     ; do the destination read
        movd            mm1, eax                ; mm1 = 0 | 0 | 0 | C0
        movq            mm0, mm1
        movq            mm2, mm1
        pslld           mm1, BLUE_SHIFT         ; blue 
        pslld           mm0, GREEN_SHIFT        ; green (6 for 5-5-5)
        pslld           mm2, RED_SHIFT          ; red (9 for 5-5-5)
        por             mm1, mm2                ; combine red and blue
        pand            mm1, mm4                ; leave valid red and blue bits
        pand            mm0, mm5                ; leave valid green bits
        por             mm1, mm0                ; mm1 = C1 | C0        
    
        ; Okay now we've got the destination read and split.  Handle the first blend:
    
        movd            mm2, [esi]
        punpcklbw       mm2, mm2
        psrlw           mm2, 8                  ; mm2 = S
        movq            mm3, mm2
        punpckhwd       mm3, mm3
        punpckhdq       mm3, mm3                ; mm3 = alpha
        movq            mm0, mm1
        punpcklbw       mm0, mm0
        psrlw           mm0, 8                  ; mm0 = D
    #if NO_PREMULTIPLIED_ALPHA
        psubw           mm2, mm0               
        pmullw          mm2, mm3                ; mm2 = alpha * (S - D)
        movq            mm3, mm2
        psrlw           mm3, 8
        paddw           mm2, mm3                ; approximate x/255 by 257/65536
        psrlw           mm2, 8                  ; mm2 = alpha * (S - D)
        paddw           mm0, mm2                ; mm0 = C0 = D + alpha * (S - D)
    #else
        pxor            mm3, flipAlphaBits    
        pmullw          mm0, mm3                ; mm2 = (255 - alpha) * D
        movq            mm3, mm0
        psrlw           mm0, 8                  ; approximate x/255 by 257/65536
        paddw           mm0, mm3                ; mm2 = (255 - alpha) * D / 255
        psrlw           mm0, 8                  ; don't care about rounding, not enough bits
        paddw           mm0, mm2                ; mm0 = C0 = S + (1 - alpha) * D
    #endif
        packuswb        mm0, mm0                ; mm0 = C1 | C0
    
        ; Dither and pack everything back up:
    
        paddusb         mm0, mm6                ; add dither
        movq            mm2, mm0
        pand            mm0, mm5                ; green
        pand            mm2, mm4                ; red and blue
        psrld           mm0, GREEN_SHIFT
        movq            mm3, mm2
        psrld           mm3, BLUE_SHIFT
        psrld           mm2, RED_SHIFT
        por             mm0, mm3
        por             mm0, mm2                ; mm0 = X | C1 | X | C0
    
        movd            eax, mm0
        mov             [edi], ax        
        ret
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    
    do_last_pixel:
        test            ecx, 1
        jz              all_done
        call            do_single_pixel

    all_done:
        emms      
    }

#endif
}

// Dither to 16bpp using MMX

VOID FASTCALL
DITHER_FUNC(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
#if defined(_X86_)

    DEFINE_POINTERS(ARGB, WORD);
    UINT uiCount = pPP->m_uiCount;
    
    Assert(uiCount != 0);

    static const ULONGLONG redBlueMask = 0x00f800f800f800f8;
    static const ULONGLONG flipAlphaBits = 0x00ff00ff00ff00ff;

#if DITHER_BLEND_555
    static const ULONGLONG greenMask = 0x0000f8000000f800;
#else
    static const ULONGLONG greenMask = 0x0000fc000000fc00;
#endif

    INT x = pPP->m_iX;
    INT y = pPP->m_iY;

    const UINT32 *dither = (pPP->m_fDither16bpp) 
                   ? &DITHER_ARRAY[8 * (y & 3) + (x & 3)] 
                   : &DitherNone[0];
                   
    _asm
    {
        ; ecx = uiCount
        ; esi = source
        ; edi = destination
        ; mm4 = red and blue mask (0xf800f8)
        ; mm5 = green mask (0x00fc00) (0x00f800 for 5-5-5)
        ; mm6 = C1 | C0 dither
        ; mm7 = C3 | C2 dither

        mov             eax, dither
        mov             esi, pSrc
        mov             edi, pDest
        mov             ecx, uiCount
        movq            mm4, redBlueMask
        movq            mm5, greenMask
        movq            mm6, [eax]      
        movq            mm7, [eax+8]    
        sub             ecx, 4                  ; pre-decrement by 4
        jl              do_last_3_pixels_or_less
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        ; We do chunks of 4 pixels at a time so that we can unroll our
        ; dither loop (our dither repeats every 4 pixels).

    do_main_loop:
        movq            mm0, [esi]
        paddusb         mm0, mm6                ; add dither
        movq            mm2, mm0
        pand            mm0, mm5                ; green
        pand            mm2, mm4                ; red and blue
        psrld           mm0, GREEN_SHIFT        ; green (6 for 5-5-5)
        movq            mm3, mm2
        psrld           mm3, BLUE_SHIFT         ; blue
        psrld           mm2, RED_SHIFT          ; red (9 for 5-5-5)
        por             mm0, mm3
        por             mm0, mm2                ; mm0 = X | C1 | X | C0
        movq            mm1, mm0
        psrlq           mm1, 32                 ; mm1 = X | X | X | C1
        punpcklwd       mm0, mm1                ; mm0 = X | X | C1 | C0
        movd            [edi], mm0
    
        movq            mm0, [esi+8]
        paddusb         mm0, mm7                ; add dither
        movq            mm2, mm0
        pand            mm0, mm5                ; green
        pand            mm2, mm4                ; red and blue
        psrld           mm0, GREEN_SHIFT
        movq            mm3, mm2
        psrld           mm3, BLUE_SHIFT
        psrld           mm2, RED_SHIFT
        por             mm0, mm3
        por             mm0, mm2                ; mm0 = X | C1 | X | C0
        movq            mm1, mm0
        psrlq           mm1, 32
        punpcklwd       mm0, mm1                ; mm0 = X | X | C1 | C0
        movd            [edi+4], mm0
    
        add             edi, 8
        add             esi, 16
        sub             ecx, 4                  ; pre-decrement for next iteration
        jge             do_main_loop
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    
    do_last_3_pixels_or_less:
        add             ecx, 4                  ; get back 'real' uiCount
        jz              all_done

        dec             ecx                     ; if exactly 1 pixel left
        jz              do_last_pixel
        
    ; do 2 pixels
        ; we'll decrement ecx again later

        movq            mm0, [esi]
        paddusb         mm0, mm6                ; add dither
        movq            mm2, mm0
        pand            mm0, mm5                ; green
        pand            mm2, mm4                ; red and blue
        psrld           mm0, GREEN_SHIFT        ; green (6 for 5-5-5)
        movq            mm3, mm2
        psrld           mm3, BLUE_SHIFT         ; blue
        psrld           mm2, RED_SHIFT          ; red (9 for 5-5-5)
        por             mm0, mm3
        por             mm0, mm2                ; mm0 = X | C1 | X | C0
        movq            mm1, mm0
        psrlq           mm1, 32                 ; mm1 = X | X | X | C1
        punpcklwd       mm0, mm1                ; mm0 = X | X | C1 | C0
        movd            eax, mm0
        mov             [edi], eax
        
        dec             ecx
        jz              all_done
        
        add             esi, 8
        add             edi, 4

    do_last_pixel:    
        movd            mm0, [esi]
        paddusb         mm0, mm7                ; add dither
        movq            mm2, mm0
        pand            mm0, mm5                ; green
        pand            mm2, mm4                ; red and blue
        psrld           mm0, GREEN_SHIFT
        movq            mm3, mm2
        psrld           mm3, BLUE_SHIFT
        psrld           mm2, RED_SHIFT
        por             mm0, mm3
        por             mm0, mm2                ; mm0 = X | C1 | X | C0
        movq            mm1, mm0
        psrlq           mm1, 32
        punpcklwd       mm0, mm1                ; mm0 = X | X | C1 | C0
        movd            eax, mm0
        mov             [edi], ax

    all_done:
        emms      
    }

#endif
}

#pragma warning(pop) // reset C4740



