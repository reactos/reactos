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
    movaps [rcx + 0], xmm0
    movaps [rcx + 16], xmm1
    movaps [rcx + 32], xmm2
    movaps [rcx + 48], xmm3
    movaps [rcx + 64], xmm4
    movaps [rcx + 80], xmm5
    movaps [rcx + 96], xmm6
    movaps [rcx + 112], xmm7
#ifndef _M_IX86
    movaps xmmword ptr [rcx + 128], xmm8
    movaps xmmword ptr [rcx + 144], xmm9
    movaps xmmword ptr [rcx + 160], xmm10
    movaps xmmword ptr [rcx + 176], xmm11
    movaps xmmword ptr [rcx + 192], xmm12
    movaps xmmword ptr [rcx + 208], xmm13
    movaps xmmword ptr [rcx + 224], xmm14
    movaps xmmword ptr [rcx + 240], xmm15
#endif
    ret

// void __fastcall set_SSE_state(__m128i data[16]);
PUBLIC set_SSE_state
set_SSE_state:
    movaps xmm0, [rcx + 0]
    movaps xmm1, [rcx + 16]
    movaps xmm2, [rcx + 32]
    movaps xmm3, [rcx + 48]
    movaps xmm4, [rcx + 64]
    movaps xmm5, [rcx + 80]
    movaps xmm6, [rcx + 96]
    movaps xmm7, [rcx + 112]
#ifndef _M_IX86
    movaps xmm8, [rcx + 128]
    movaps xmm9, [rcx + 144]
    movaps xmm10, [rcx + 160]
    movaps xmm11, [rcx + 176]
    movaps xmm12, [rcx + 192]
    movaps xmm13, [rcx + 208]
    movaps xmm14, [rcx + 224]
    movaps xmm15, [rcx + 240]
#endif
    ret


// void __fastcall get_AVX_state(__m256i data[16]);
PUBLIC get_AVX_state
get_AVX_state:
    vmovaps [rcx + 0], ymm0
    vmovaps [rcx + 32], ymm1
    vmovaps [rcx + 64], ymm2
    vmovaps [rcx + 96], ymm3
    vmovaps [rcx + 128], ymm4
    vmovaps [rcx + 160], ymm5
    vmovaps [rcx + 192], ymm6
    vmovaps [rcx + 224], ymm7
#ifndef _M_IX86
    vmovaps [rcx + 256], ymm8
    vmovaps [rcx + 288], ymm9
    vmovaps [rcx + 320], ymm10
    vmovaps [rcx + 352], ymm11
    vmovaps [rcx + 384], ymm12
    vmovaps [rcx + 416], ymm13
    vmovaps [rcx + 448], ymm14
    vmovaps [rcx + 480], ymm15
#endif
    ret

// void __fastcall set_AVX_state(__m256i data[16]);
PUBLIC set_AVX_state
set_AVX_state:
    vmovaps ymm0, [rcx + 0]
    vmovaps ymm1, [rcx + 32]
    vmovaps ymm2, [rcx + 64]
    vmovaps ymm3, [rcx + 96]
    vmovaps ymm4, [rcx + 128]
    vmovaps ymm5, [rcx + 160]
    vmovaps ymm6, [rcx + 192]
    vmovaps ymm7, [rcx + 224]
#ifndef _M_IX86
    vmovaps ymm8, [rcx + 256]
    vmovaps ymm9, [rcx + 288]
    vmovaps ymm10, [rcx + 320]
    vmovaps ymm11, [rcx + 352]
    vmovaps ymm12, [rcx + 384]
    vmovaps ymm13, [rcx + 416]
    vmovaps ymm14, [rcx + 448]
    vmovaps ymm15, [rcx + 480]
#endif
    ret


// void __fastcall get_AVX512_state(M512U64 data[16]);
PUBLIC get_AVX512_state
get_AVX512_state:
    vmovaps [rcx + 0], zmm0
    vmovaps [rcx + 64], zmm1
    vmovaps [rcx + 128], zmm2
    vmovaps [rcx + 192], zmm3
    vmovaps [rcx + 256], zmm4
    vmovaps [rcx + 320], zmm5
    vmovaps [rcx + 384], zmm6
    vmovaps [rcx + 448], zmm7
#ifndef _M_IX86
    vmovaps [rcx + 512], zmm8
    vmovaps [rcx + 576], zmm9
    vmovaps [rcx + 640], zmm10
    vmovaps [rcx + 704], zmm11
    vmovaps [rcx + 768], zmm12
    vmovaps [rcx + 832], zmm13
    vmovaps [rcx + 896], zmm14
    vmovaps [rcx + 960], zmm15
#endif
    ret

// void __fastcall set_AVX512_state(const M512U64 data[16]);
PUBLIC set_AVX512_state
set_AVX512_state:
    vmovaps zmm0, [rcx + 0]
    vmovaps zmm1, [rcx + 64]
    vmovaps zmm2, [rcx + 128]
    vmovaps zmm3, [rcx + 192]
    vmovaps zmm4, [rcx + 256]
    vmovaps zmm5, [rcx + 320]
    vmovaps zmm6, [rcx + 384]
    vmovaps zmm7, [rcx + 448]
#ifndef _M_IX86
    vmovaps zmm8, [rcx + 512]
    vmovaps zmm9, [rcx + 576]
    vmovaps zmm10, [rcx + 640]
    vmovaps zmm11, [rcx + 704]
    vmovaps zmm12, [rcx + 768]
    vmovaps zmm13, [rcx + 832]
    vmovaps zmm14, [rcx + 896]
    vmovaps zmm15, [rcx + 960]
#endif
    ret

END
