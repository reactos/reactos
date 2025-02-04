

#include <asm.inc>

.code64

__xmm_data: // An array of random numbers
.long HEX(0293d426), HEX(20307f0d), HEX(b7b92f7f), HEX(1492f8c6)
.long HEX(08367ed0), HEX(30fd2307), HEX(f9e995e4), HEX(31f56b10)


// void set_SSE2_state(__m128i data[16]);
PUBLIC set_SSE2_state
set_SSE2_state:
    movdqa xmm0,xmmword ptr [rcx + 0]
    movdqa xmm1,xmmword ptr [rcx + 16]
    movdqa xmm2,xmmword ptr [rcx + 32]
    movdqa xmm3,xmmword ptr [rcx + 48]
    movdqa xmm4,xmmword ptr [rcx + 64]
    movdqa xmm5,xmmword ptr [rcx + 80]
    movdqa xmm6,xmmword ptr [rcx + 96]
    movdqa xmm7,xmmword ptr [rcx + 112]
    movdqa xmm8,xmmword ptr [rcx + 128]
    movdqa xmm9,xmmword ptr [rcx + 144]
    movdqa xmm10,xmmword ptr [rcx + 160]
    movdqa xmm11,xmmword ptr [rcx + 176]
    movdqa xmm12,xmmword ptr [rcx + 192]
    movdqa xmm13,xmmword ptr [rcx + 208]
    movdqa xmm14,xmmword ptr [rcx + 224]
    movdqa xmm15,xmmword ptr [rcx + 240]
    ret

// void get_SSE2_state(__m128i data[16]);
PUBLIC get_SSE2_state
get_SSE2_state:
    movdqa xmmword ptr [rcx + 0],xmm0
    movdqa xmmword ptr [rcx + 16],xmm1
    movdqa xmmword ptr [rcx + 32],xmm2
    movdqa xmmword ptr [rcx + 48],xmm3
    movdqa xmmword ptr [rcx + 64],xmm4
    movdqa xmmword ptr [rcx + 80],xmm5
    movdqa xmmword ptr [rcx + 96],xmm6
    movdqa xmmword ptr [rcx + 112],xmm7
    movdqa xmmword ptr [rcx + 128],xmm8
    movdqa xmmword ptr [rcx + 144],xmm9
    movdqa xmmword ptr [rcx + 160],xmm10
    movdqa xmmword ptr [rcx + 176],xmm11
    movdqa xmmword ptr [rcx + 192],xmm12
    movdqa xmmword ptr [rcx + 208],xmm13
    movdqa xmmword ptr [rcx + 224],xmm14
    movdqa xmmword ptr [rcx + 240],xmm15
    ret


// void set_AVX_state(__m256i data[16]);
PUBLIC set_AVX_state
set_AVX_state:
    vmovdqa ymm0,ymmword ptr [rcx + 0]
    vmovdqa ymm1,ymmword ptr [rcx + 32]
    vmovdqa ymm2,ymmword ptr [rcx + 64]
    vmovdqa ymm3,ymmword ptr [rcx + 96]
    vmovdqa ymm4,ymmword ptr [rcx + 128]
    vmovdqa ymm5,ymmword ptr [rcx + 160]
    vmovdqa ymm6,ymmword ptr [rcx + 192]
    vmovdqa ymm7,ymmword ptr [rcx + 224]
    vmovdqa ymm8,ymmword ptr [rcx + 256]
    vmovdqa ymm9,ymmword ptr [rcx + 288]
    vmovdqa ymm10,ymmword ptr [rcx + 320]
    vmovdqa ymm11,ymmword ptr [rcx + 352]
    vmovdqa ymm12,ymmword ptr [rcx + 384]
    vmovdqa ymm13,ymmword ptr [rcx + 416]
    vmovdqa ymm14,ymmword ptr [rcx + 448]
    vmovdqa ymm15,ymmword ptr [rcx + 480]
    ret

// void get_AVX_state(__m256i data[16]);
PUBLIC get_AVX_state
get_AVX_state:
    vmovdqa ymmword ptr [rcx + 0],ymm0
    vmovdqa ymmword ptr [rcx + 32],ymm1
    vmovdqa ymmword ptr [rcx + 64],ymm2
    vmovdqa ymmword ptr [rcx + 96],ymm3
    vmovdqa ymmword ptr [rcx + 128],ymm4
    vmovdqa ymmword ptr [rcx + 160],ymm5
    vmovdqa ymmword ptr [rcx + 192],ymm6
    vmovdqa ymmword ptr [rcx + 224],ymm7
    vmovdqa ymmword ptr [rcx + 256],ymm8
    vmovdqa ymmword ptr [rcx + 288],ymm9
    vmovdqa ymmword ptr [rcx + 320],ymm10
    vmovdqa ymmword ptr [rcx + 352],ymm11
    vmovdqa ymmword ptr [rcx + 384],ymm12
    vmovdqa ymmword ptr [rcx + 416],ymm13
    vmovdqa ymmword ptr [rcx + 448],ymm14
    vmovdqa ymmword ptr [rcx + 480],ymm15
    ret

END
