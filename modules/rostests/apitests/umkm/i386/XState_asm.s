/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Assembly helpers for extended state tests
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>

#ifdef _M_IX86
#define rcx ecx
#define get_SSE_state @get_SSE_state@4
#define set_SSE_state @set_SSE_state@4
#define get_AVX_state @get_AVX_state@4
#define set_AVX_state @set_AVX_state@4
#define get_AVX512_state @get_AVX512_state@4
#define set_AVX512_state @set_AVX512_state@4
.code
#else
.code64
#endif

// void __fastcall get_SSE_state(__m128i data[16]);
PUBLIC get_SSE_state
get_SSE_state:
    movaps [rcx + 0 * 16], xmm0
    movaps [rcx + 1 * 16], xmm1
    movaps [rcx + 2 * 16], xmm2
    movaps [rcx + 3 * 16], xmm3
    movaps [rcx + 4 * 16], xmm4
    movaps [rcx + 5 * 16], xmm5
    movaps [rcx + 6 * 16], xmm6
    movaps [rcx + 7 * 16], xmm7
#ifndef _M_IX86
    movaps [rcx + 8 * 16], xmm8
    movaps [rcx + 9 * 16], xmm9
    movaps [rcx + 10 * 16], xmm10
    movaps [rcx + 11 * 16], xmm11
    movaps [rcx + 12 * 16], xmm12
    movaps [rcx + 13 * 16], xmm13
    movaps [rcx + 14 * 16], xmm14
    movaps [rcx + 15 * 16], xmm15
#endif
    ret

// void __fastcall set_SSE_state(__m128i data[16]);
PUBLIC set_SSE_state
set_SSE_state:
    movaps xmm0, [rcx + 0 * 16]
    movaps xmm1, [rcx + 1 * 16]
    movaps xmm2, [rcx + 2 * 16]
    movaps xmm3, [rcx + 3 * 16]
    movaps xmm4, [rcx + 4 * 16]
    movaps xmm5, [rcx + 5 * 16]
    movaps xmm6, [rcx + 6 * 16]
    movaps xmm7, [rcx + 7 * 16]
#ifndef _M_IX86
    movaps xmm8, [rcx + 8 * 16]
    movaps xmm9, [rcx + 9 * 16]
    movaps xmm10, [rcx + 10 * 16]
    movaps xmm11, [rcx + 11 * 16]
    movaps xmm12, [rcx + 12 * 16]
    movaps xmm13, [rcx + 13 * 16]
    movaps xmm14, [rcx + 14 * 16]
    movaps xmm15, [rcx + 15 * 16]
#endif
    ret


// void __fastcall get_AVX_state(__m256i data[16]);
PUBLIC get_AVX_state
get_AVX_state:
    vmovaps [rcx + 0 * 32], ymm0
    vmovaps [rcx + 1 * 32], ymm1
    vmovaps [rcx + 2 * 32], ymm2
    vmovaps [rcx + 3 * 32], ymm3
    vmovaps [rcx + 4 * 32], ymm4
    vmovaps [rcx + 5 * 32], ymm5
    vmovaps [rcx + 6 * 32], ymm6
    vmovaps [rcx + 7 * 32], ymm7
#ifndef _M_IX86
    vmovaps [rcx + 8 * 32], ymm8
    vmovaps [rcx + 9 * 32], ymm9
    vmovaps [rcx + 10 * 32], ymm10
    vmovaps [rcx + 11 * 32], ymm11
    vmovaps [rcx + 12 * 32], ymm12
    vmovaps [rcx + 13 * 32], ymm13
    vmovaps [rcx + 14 * 32], ymm14
    vmovaps [rcx + 15 * 32], ymm15
#endif
    ret

// void __fastcall set_AVX_state(__m256i data[16]);
PUBLIC set_AVX_state
set_AVX_state:
    vmovaps ymm0, [rcx + 0 * 32]
    vmovaps ymm1, [rcx + 1 * 32]
    vmovaps ymm2, [rcx + 2 * 32]
    vmovaps ymm3, [rcx + 3 * 32]
    vmovaps ymm4, [rcx + 4 * 32]
    vmovaps ymm5, [rcx + 5 * 32]
    vmovaps ymm6, [rcx + 6 * 32]
    vmovaps ymm7, [rcx + 7 * 32]
#ifndef _M_IX86
    vmovaps ymm8, [rcx + 8 * 32]
    vmovaps ymm9, [rcx + 9 * 32]
    vmovaps ymm10, [rcx + 10 * 32]
    vmovaps ymm11, [rcx + 11 * 32]
    vmovaps ymm12, [rcx + 12 * 32]
    vmovaps ymm13, [rcx + 13 * 32]
    vmovaps ymm14, [rcx + 14 * 32]
    vmovaps ymm15, [rcx + 15 * 32]
#endif
    ret


// void __fastcall get_AVX512_state(M512U64 data[16]);
PUBLIC get_AVX512_state
get_AVX512_state:
    vmovaps [rcx + 0 * 64], zmm0
    vmovaps [rcx + 1 * 64], zmm1
    vmovaps [rcx + 2 * 64], zmm2
    vmovaps [rcx + 3 * 64], zmm3
    vmovaps [rcx + 4 * 64], zmm4
    vmovaps [rcx + 5 * 64], zmm5
    vmovaps [rcx + 6 * 64], zmm6
    vmovaps [rcx + 7 * 64], zmm7
#ifndef _M_IX86
    vmovaps [rcx + 8 * 64], zmm8
    vmovaps [rcx + 9 * 64], zmm9
    vmovaps [rcx + 10 * 64], zmm10
    vmovaps [rcx + 11 * 64], zmm11
    vmovaps [rcx + 12 * 64], zmm12
    vmovaps [rcx + 13 * 64], zmm13
    vmovaps [rcx + 14 * 64], zmm14
    vmovaps [rcx + 15 * 64], zmm15
    vmovaps [rcx + 16 * 64], zmm16
    vmovaps [rcx + 17 * 64], zmm17
    vmovaps [rcx + 18 * 64], zmm18
    vmovaps [rcx + 19 * 64], zmm19
    vmovaps [rcx + 20 * 64], zmm20
    vmovaps [rcx + 21 * 64], zmm21
    vmovaps [rcx + 22 * 64], zmm22
    vmovaps [rcx + 23 * 64], zmm23
    vmovaps [rcx + 24 * 64], zmm24
    vmovaps [rcx + 25 * 64], zmm25
    vmovaps [rcx + 26 * 64], zmm26
    vmovaps [rcx + 27 * 64], zmm27
    vmovaps [rcx + 28 * 64], zmm28
    vmovaps [rcx + 29 * 64], zmm29
    vmovaps [rcx + 30 * 64], zmm30
    vmovaps [rcx + 31 * 64], zmm31
#endif
    ret

// void __fastcall set_AVX512_state(const M512U64 data[16]);
PUBLIC set_AVX512_state
set_AVX512_state:
    vmovaps zmm0, [rcx + 0 * 64]
    vmovaps zmm1, [rcx + 1 * 64]
    vmovaps zmm2, [rcx + 2 * 64]
    vmovaps zmm3, [rcx + 3 * 64]
    vmovaps zmm4, [rcx + 4 * 64]
    vmovaps zmm5, [rcx + 5 * 64]
    vmovaps zmm6, [rcx + 6 * 64]
    vmovaps zmm7, [rcx + 7 * 64]
#ifndef _M_IX86
    vmovaps zmm8, [rcx + 8 * 64]
    vmovaps zmm9, [rcx + 9 * 64]
    vmovaps zmm10, [rcx + 10 * 64]
    vmovaps zmm11, [rcx + 11 * 64]
    vmovaps zmm12, [rcx + 12 * 64]
    vmovaps zmm13, [rcx + 13 * 64]
    vmovaps zmm14, [rcx + 14 * 64]
    vmovaps zmm15, [rcx + 15 * 64]
    vmovaps zmm16, [rcx + 16 * 64]
    vmovaps zmm17, [rcx + 17 * 64]
    vmovaps zmm18, [rcx + 18 * 64]
    vmovaps zmm19, [rcx + 19 * 64]
    vmovaps zmm20, [rcx + 20 * 64]
    vmovaps zmm21, [rcx + 21 * 64]
    vmovaps zmm22, [rcx + 22 * 64]
    vmovaps zmm23, [rcx + 23 * 64]
    vmovaps zmm24, [rcx + 24 * 64]
    vmovaps zmm25, [rcx + 25 * 64]
    vmovaps zmm26, [rcx + 26 * 64]
    vmovaps zmm27, [rcx + 27 * 64]
    vmovaps zmm28, [rcx + 28 * 64]
    vmovaps zmm29, [rcx + 29 * 64]
    vmovaps zmm30, [rcx + 30 * 64]
    vmovaps zmm31, [rcx + 31 * 64]
#endif
    ret

END
