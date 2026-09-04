// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Definitions of class CCoder86 and related stuff
//      for low level generating of binary code for IA-32 CPU.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Enum:
//      Scale32
//
//  Synopsis:
//      Provides the way to specify scaling factor for instructions
//      that access memory. See comments to class memptr.
//
//-----------------------------------------------------------------------------
enum Scale32
{
    scale_1 = 0,
    scale_2 = 1,
    scale_4 = 2,
    scale_8 = 3,
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      memptr
//
//  Synopsis:
//      Provides the way to specify memory operand for IA-32 instructions.
//      This class is only used to derive size-specific memory operand
//      classes (dword, mmxword and xmmword - see below).
//
//  Usage patterns: see examples for class dword.
//
//-----------------------------------------------------------------------------
class memptr
{
public:
    memptr(RegGPR base, RegGPR index, Scale32 scale, INT_PTR nDisplacement)
    {
        WarpAssert(index != gbp);
        m_base  = base;
        m_index = index;
        m_scale = scale;
        m_nDisplacement = nDisplacement;
    }
    memptr(RegGPR base, RegGPR index, Scale32 scale)
    {
        WarpAssert(index != gbp);
        m_base  = base;
        m_index = index;
        m_scale = scale;
        m_nDisplacement = 0;
    }
    memptr(RegGPR base, INT_PTR nDisplacement)
    {
        m_base  = base;
        m_index = gpr_none;
        m_scale = scale_1;
        m_nDisplacement = nDisplacement;
    }
    memptr(void * pBase, RegGPR index, Scale32 scale)
    {
        WarpAssert(index != gbp);
        m_base  = gpr_none;
        m_index = index;
        m_scale = scale;
        m_nDisplacement = (INT_PTR)pBase;
    }
    memptr(RegGPR index, Scale32 scale)
    {
        WarpAssert(index != gbp);
        m_base  = gpr_none;
        m_index = index;
        m_scale = scale;
        m_nDisplacement = 0;
    }
    memptr(void * pData)
    {
        m_base  = gpr_none;
        m_index = gpr_none;
        m_scale = scale_1;
        m_nDisplacement = (INT_PTR)pData;
    }

    Scale32 m_scale;
    RegGPR m_index;
    RegGPR m_base;
    INT_PTR m_nDisplacement;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      dword
//
//  Synopsis:
//      Provides the way to specify 32-bit memory operand.
//
//  Usage patterns:
//      Assuming the variable pCoder points to an instance of CCoder86.
//
//      pCoder->mov(ebx, dword(esi, index_gcx, scale_4));
//          corresponds to __asm mov ebx, dword ptr[esi + ecx*4];
//
//      pCoder->mov(ebx, dword(esi, index_gcx, scale_4, 0x20));
//          corresponds to __asm mov ebx, dword ptr[esi + ecx*4 + 0x20];
//
//      pCoder->mov(eax, dword(esp, 0x8));
//          corresponds to __asm mov eax, dword ptr[esp + 0x8];
//
//      pCoder->mov(eax, dword(esp));
//          corresponds to __asm mov eax, dword ptr[esp];
//
//      pCoder->mov(eax, dword(index_gcx, scale_8, 0x20));
//          corresponds to __asm mov eax, dword ptr[ecx*8 + 0x20];
//
//      pCoder->mov(eax, dword(index_gcx, scale_8));
//          corresponds to __asm mov eax, dword ptr[ecx*8];
//
//      pCoder->mov(eax, dword(0x12345678));
//          fetches 32-bit value from memory location with address ds:0x12345678
//
//-----------------------------------------------------------------------------
class dword : public memptr
{
public:
    dword(RegGPR base, RegGPR index, Scale32 scale, INT_PTR nDisplacement)
        : memptr(base, index, scale, nDisplacement) {}

    dword(RegGPR base, RegGPR index, Scale32 scale)
        : memptr(base, index, scale) {}

    dword(RegGPR base, INT_PTR nDisplacement)
        : memptr(base, nDisplacement) {}

    dword(void * pBase, RegGPR index, Scale32 scale)
        : memptr(pBase, index, scale) {}

    dword(RegGPR index, Scale32 scale)
        : memptr(index, scale) {}

    dword(void *pData)
        : memptr(pData) {}
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      mmxword
//
//  Synopsis:
//      Provides the way to specify 64-bit memory operand.
//
//  Usage patterns:
//      Assuming the variable pCoder points to an instance of CCoder86.
//
//      pCoder->paddd(mmx0, mmxword(esi, index_gcx, scale_4));
//          corresponds to __asm paddd mmx0, qword ptr[esi + ecx*4];
//
//
//-----------------------------------------------------------------------------
class mmxword : public memptr
{
public:
    mmxword(RegGPR base, RegGPR index, Scale32 scale, INT_PTR nDisplacement)
        : memptr(base, index, scale, nDisplacement) {}

    mmxword(RegGPR base, RegGPR index, Scale32 scale)
        : memptr(base, index, scale) {}

    mmxword(RegGPR base, INT_PTR nDisplacement)
        : memptr(base, nDisplacement) {}

    mmxword(void * pBase, RegGPR index, Scale32 scale)
        : memptr(pBase, index, scale) {}

    mmxword(RegGPR index, Scale32 scale)
        : memptr(index, scale) {}

    mmxword(void *pData)
        : memptr(pData) {}
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      xmmword
//
//  Synopsis:
//      Provides the way to specify 64-bit memory operand.
//
//  Usage patterns:
//      Assuming the variable pCoder points to an instance of CCoder86.
//
//      pCoder->paddd(xmm0, xmmword(esi, index_gcx, scale_16));
//          corresponds to __asm paddd xmm0, xmmword ptr[esi + ecx*16];
//
//-----------------------------------------------------------------------------
class xmmword : public memptr
{
public:
    xmmword(RegGPR base, RegGPR index, Scale32 scale, INT_PTR nDisplacement)
        : memptr(base, index, scale, nDisplacement) {}

    xmmword(RegGPR base, RegGPR index, Scale32 scale)
        : memptr(base, index, scale) {}

    xmmword(RegGPR base, INT_PTR nDisplacement)
        : memptr(base, nDisplacement) {}

    xmmword(void * pBase, RegGPR index, Scale32 scale)
        : memptr(pBase, index, scale) {}

    xmmword(RegGPR index, Scale32 scale)
        : memptr(index, scale) {}

    xmmword(void *pData)
        : memptr(pData) {}
};

//+----------------------------------------------------------------------------
//
//  enum OpcodeField and macro OPCODE provide a way to pack instruction opcode
//  into 32-bit value. Note that opcode with prefixes can take up to 5 bytes
//  (in 64-bit system with SSE4.1). Related routines:
//      CAssemblePass1::EmitOpcode
//      CAssemblePass2::EmitOpcode
//
//-----------------------------------------------------------------------------
enum OpcodeField : UINT32
{
    OpcSize     = 0x00000007,     OpcShiftSize     = 0, // size of opcode field, in bytes, without REX byte
    OpcIsLong   = 0x00000008,     OpcShiftIsLong   = 3,
    OpcPrefix   = 0x00000030,     OpcShiftPrefix   = 4,
    OpcReversed = 0x00000040,     OpcShiftReversed = 6,
    OpcReserved = 0x00000F80,     OpcShiftReserved = 7,
    OpcREX      = 0x0000F000,     OpcShiftREX      = 12,
    OpcByte1    = 0x00FF0000,     OpcShiftByte1    = 16,
    OpcByte2    = 0xFF000000,     OpcShiftByte2    = 24,
};

#define OPCODE_SIZE(prefix, opcode) ( ( (prefix == Prefix_None) ? 0 : 2 ) + ( (opcode > 0xFF) ? 2 : 1 ) )
#define OPCODE_IS_LONG(opcode) ( (opcode > 0xFF) ? 1 : 0 )
#define OPCODE_BYTE1(opcode) (UINT32)( (opcode >> 8) & 0xFF )
#define OPCODE_BYTE2(opcode) (UINT32)( (opcode     ) & 0xFF )

#define OPCODE(prefix, opcode) 0                        \
        | (OPCODE_SIZE(prefix, opcode) << OpcShiftSize) \
        | (prefix << OpcShiftPrefix)                    \
        | (OPCODE_IS_LONG(opcode) << OpcShiftIsLong)    \
        | (OPCODE_BYTE1(opcode) << OpcShiftByte1)       \
        | (OPCODE_BYTE2(opcode) << OpcShiftByte2)       \

enum Prefix : UINT32
{
    Prefix_None = 0,
    Prefix_F20F = 1,
    Prefix_F30F = 2,
    Prefix_660F = 3,
};

#if WPFGFX_FXJIT_X86
#else //_AMD64_
//
// REX prefix bits, shifted by OpcShiftREX for packing.
//
enum REX
{
    REX_B = 1 << OpcShiftREX,  // Extension of the ModR/M r/m field, SIB base field, or Opcode reg field
    REX_X = 2 << OpcShiftREX,  // Extension of the SIB index field
    REX_R = 4 << OpcShiftREX,  // Extension of the ModR/M reg field
    REX_W = 8 << OpcShiftREX,  // 1 = 64 Bit Operand Size
};

#endif


enum OpCode : UINT32
{
    mov_rm    = OPCODE(Prefix_None, 0x8B),  // move memory to general purpose register, 32 bits
    mov_mr    = OPCODE(Prefix_None, 0x89),  // move general purpose register to memory, 32 bits
    movnti_mr = OPCODE(Prefix_None, 0x0FC3),    // non-temporal move general purpose register to memory, 32 bits

    mov_rr    = mov_rm                   ,  // move general purpose register to another general purpose register, 32 bits

    movzx_rm8   = OPCODE(Prefix_None, 0x0FB6),  // move memory to general purpose register, zero extend, 8 bits
    movzx_rm16  = OPCODE(Prefix_None, 0x0FB7),  // move memory to general purpose register, zero extend, 16 bits

    add       = OPCODE(Prefix_None, 0x03),
    or        = OPCODE(Prefix_None, 0x0B),
    and       = OPCODE(Prefix_None, 0x23),
    sub       = OPCODE(Prefix_None, 0x2B),
    xor       = OPCODE(Prefix_None, 0x33),
    cmp       = OPCODE(Prefix_None, 0x3B),
    imul      = OPCODE(Prefix_None, 0x0FAF),

    lea       = OPCODE(Prefix_None, 0x8D),

#if WPFGFX_FXJIT_X86
    mov_ptr_rm = mov_rm,  // move memory to general purpose register, 32 bits
    mov_ptr_mr = mov_mr,  // move general purpose register to memory, 32 bits
    mov_ptr_rr = mov_rr,  // move general purpose register to another general purpose register, 32 bits

    lea_ptr    = lea,

    movd_mmx_rm = OPCODE(Prefix_None, 0x0F6E),
    movd_mmx_mr = OPCODE(Prefix_None, 0x0F7E),

    movq_mmx_rm = OPCODE(Prefix_None, 0x0F6F),
    movq_mmx_mr = OPCODE(Prefix_None, 0x0F7F),
    movq_mmx_rr = movq_mmx_rm,

    paddb_mmx       = OPCODE(Prefix_None, 0x0FFC),
    psubb_mmx       = OPCODE(Prefix_None, 0x0FF8),
    pcmpeqb_mmx     = OPCODE(Prefix_None, 0x0F74),
    punpcklbw_mmx   = OPCODE(Prefix_None, 0x0F60),
    punpckhbw_mmx   = OPCODE(Prefix_None, 0x0F68),

    paddw_mmx       = OPCODE(Prefix_None, 0x0FFD),
    paddusw_mmx     = OPCODE(Prefix_None, 0x0FDD),
    psubw_mmx       = OPCODE(Prefix_None, 0x0FF9),
    psubusw_mmx     = OPCODE(Prefix_None, 0x0FD9),
    pcmpeqw_mmx     = OPCODE(Prefix_None, 0x0F75),
    punpcklwd_mmx   = OPCODE(Prefix_None, 0x0F61),
    punpckhwd_mmx   = OPCODE(Prefix_None, 0x0F69),
    packsswb_mmx    = OPCODE(Prefix_None, 0x0F63),
    packuswb_mmx    = OPCODE(Prefix_None, 0x0F67),
    pmaddwd_mmx     = OPCODE(Prefix_None, 0x0FF5),
    pmullw_mmx      = OPCODE(Prefix_None, 0x0FD5),

    paddd_mmx       = OPCODE(Prefix_None, 0x0FFE),
    psubd_mmx       = OPCODE(Prefix_None, 0x0FFA),
    pcmpeqd_mmx     = OPCODE(Prefix_None, 0x0F76),
    punpckldq_mmx   = OPCODE(Prefix_None, 0x0F62),
    punpckhdq_mmx   = OPCODE(Prefix_None, 0x0F6A),
    packssdw_mmx    = OPCODE(Prefix_None, 0x0F6B),
    pcmpgtd_mmx     = OPCODE(Prefix_None, 0x0F66),

    paddq_mmx       = OPCODE(Prefix_None, 0x0FD4),
    psubq_mmx       = OPCODE(Prefix_None, 0x0FFB),
    pand_mmx        = OPCODE(Prefix_None, 0x0FDB),
    pandn_mmx       = OPCODE(Prefix_None, 0x0FDF),
    por_mmx         = OPCODE(Prefix_None, 0x0FEB),
    pxor_mmx        = OPCODE(Prefix_None, 0x0FEF),
#else //_AMD64_
    mov_64_rm = mov_rm | REX_W,  // move memory to general purpose register, 64 bits
    mov_64_mr = mov_mr | REX_W,  // move general purpose register to memory, 64 bits
    mov_64_rr = mov_rr | REX_W,  // move general purpose register to another general purpose register, 64 bits
    mov_ptr_rm = mov_64_rm,      // in 64-bit system
    mov_ptr_mr = mov_64_mr,      //     pointers are
    mov_ptr_rr = mov_64_rr,      //         64-bit values

    lea_64 = lea | REX_W,
    lea_ptr = lea_64,
#endif

    movd_xmm_rm = OPCODE(Prefix_660F, 0x6E),
    movd_xmm_xr = OPCODE(Prefix_660F, 0x6E),                 // gpr to xmm
    movd_xmm_mr = OPCODE(Prefix_660F, 0x7E),
    movd_xmm_rx = OPCODE(Prefix_660F, 0x7E) | OpcReversed,   // xmm to gpr

    movq_xmm_rm = OPCODE(Prefix_F30F, 0x7E),
    movq_xmm_mr = OPCODE(Prefix_660F, 0xD6),
    movq_xmm_rr = movq_xmm_rm,


    movss_rm  = OPCODE(Prefix_F30F, 0x10),
    movss_mr  = OPCODE(Prefix_F30F, 0x11),
    movss_rr  = movss_rm,

    addss     = OPCODE(Prefix_F30F, 0x58),
    subss     = OPCODE(Prefix_F30F, 0x5C),
    mulss     = OPCODE(Prefix_F30F, 0x59),
    divss     = OPCODE(Prefix_F30F, 0x5E),
    minss     = OPCODE(Prefix_F30F, 0x5D),
    maxss     = OPCODE(Prefix_F30F, 0x5F),

    rcpss     = OPCODE(Prefix_F30F, 0x53),
    sqrtss    = OPCODE(Prefix_F30F, 0x51),
    rsqrtss   = OPCODE(Prefix_F30F, 0x52),
    cvtsi2ss  = OPCODE(Prefix_F30F, 0x2A),

    movaps_rm = OPCODE(Prefix_None, 0x0F28),
    movaps_mr = OPCODE(Prefix_None, 0x0F29),
    movaps_rr = movaps_rm,

    addps     = OPCODE(Prefix_None, 0x0F58),
    subps     = OPCODE(Prefix_None, 0x0F5C),
    mulps     = OPCODE(Prefix_None, 0x0F59),
    divps     = OPCODE(Prefix_None, 0x0F5E),
    minps     = OPCODE(Prefix_None, 0x0F5D),
    maxps     = OPCODE(Prefix_None, 0x0F5F),
    andps     = OPCODE(Prefix_None, 0x0F54),
    andnps    = OPCODE(Prefix_None, 0x0F55),
    orps      = OPCODE(Prefix_None, 0x0F56),
    xorps     = OPCODE(Prefix_None, 0x0F57),
    cmpps     = OPCODE(Prefix_None, 0x0FC2),
    unpcklps  = OPCODE(Prefix_None, 0x0F14),
    unpckhps  = OPCODE(Prefix_None, 0x0F15),

    cvtdq2ps  = OPCODE(Prefix_None, 0x0F5B),
    cvtps2dq  = OPCODE(Prefix_660F, 0x5B),
    cvttps2dq = OPCODE(Prefix_F30F, 0x5B),
    rcpps     = OPCODE(Prefix_None, 0x0F53),
    sqrtps    = OPCODE(Prefix_None, 0x0F51),
    rsqrtps   = OPCODE(Prefix_None, 0x0F52),

    shufps    = OPCODE(Prefix_None, 0x0FC6),

    movntps   = OPCODE(Prefix_None, 0x0F2B),

    movntpd   = OPCODE(Prefix_660F, 0x2B),

    movdqa_rm = OPCODE(Prefix_660F, 0x6F),
    movdqa_mr = OPCODE(Prefix_660F, 0x7F),
    movdqa_rr = movdqa_rm,

    andpd     = OPCODE(Prefix_660F, 0x54),
    andnpd    = OPCODE(Prefix_660F, 0x55),

    paddq     = OPCODE(Prefix_660F, 0xD4),
    psubq     = OPCODE(Prefix_660F, 0xFB),
    punpcklqdq = OPCODE(Prefix_660F, 0x6C),
    punpckhqdq = OPCODE(Prefix_660F, 0x6D),

    paddd     = OPCODE(Prefix_660F, 0xFE),
    psubd     = OPCODE(Prefix_660F, 0xFA),
    pmuludq   = OPCODE(Prefix_660F, 0xF4),
    pcmpgtd   = OPCODE(Prefix_660F, 0x66),
    pcmpeqd   = OPCODE(Prefix_660F, 0x76),
    punpckldq = OPCODE(Prefix_660F, 0x62),
    punpckhdq = OPCODE(Prefix_660F, 0x6A),
    packssdw  = OPCODE(Prefix_660F, 0x6B),
    pand      = OPCODE(Prefix_660F, 0xDB),
    pandn     = OPCODE(Prefix_660F, 0xDF),
    por       = OPCODE(Prefix_660F, 0xEB),
    pxor      = OPCODE(Prefix_660F, 0xEF),

    pshufd    = OPCODE(Prefix_660F, 0x70),

    paddw     = OPCODE(Prefix_660F, 0xFD),
    paddusw   = OPCODE(Prefix_660F, 0xDD),
    psubw     = OPCODE(Prefix_660F, 0xF9),
    psubusw   = OPCODE(Prefix_660F, 0xD9),
    pcmpeqw   = OPCODE(Prefix_660F, 0x75),
    punpcklwd = OPCODE(Prefix_660F, 0x61),
    punpckhwd = OPCODE(Prefix_660F, 0x69),
    packsswb  = OPCODE(Prefix_660F, 0x63),
    packuswb  = OPCODE(Prefix_660F, 0x67),
    pmaddwd   = OPCODE(Prefix_660F, 0xF5),
    pmullw    = OPCODE(Prefix_660F, 0xD5),

    pminsw    = OPCODE(Prefix_660F, 0xEA),
    pmaxsw    = OPCODE(Prefix_660F, 0xEE),
    pshuflw   = OPCODE(Prefix_F20F, 0x70),
    pshufhw   = OPCODE(Prefix_F30F, 0x70),

    paddb     = OPCODE(Prefix_660F, 0xFC),
    psubb     = OPCODE(Prefix_660F, 0xF8),
    pcmpeqb   = OPCODE(Prefix_660F, 0x74),
    punpcklbw = OPCODE(Prefix_660F, 0x60),
    punpckhbw = OPCODE(Prefix_660F, 0x68),
    movntdq   = OPCODE(Prefix_660F, 0xE7),

    // SSE4.1
    pblendvb  = OPCODE(Prefix_660F, 0x3810),
    pmuldq    = OPCODE(Prefix_660F, 0x3828),
    pmovzxbw  = OPCODE(Prefix_660F, 0x3830),
    pmovzxwd  = OPCODE(Prefix_660F, 0x3833),
    pminsd    = OPCODE(Prefix_660F, 0x3839),
    pminud    = OPCODE(Prefix_660F, 0x383B),
    pmaxsd    = OPCODE(Prefix_660F, 0x383D),
    pmaxud    = OPCODE(Prefix_660F, 0x383F),
    pmulld    = OPCODE(Prefix_660F, 0x3840),

    roundps   = OPCODE(Prefix_660F, 0x3A08),

    pextrd    = OPCODE(Prefix_660F, 0x3A16) | OpcReversed,
    pinsrd    = OPCODE(Prefix_660F, 0x3A22),
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CCoder86
//
//  Synopsis:
//      Provides low-level code generation for IA-32 CPU.
//      It needs explicit call to generate every CPU instruction,
//      with explicit registers as call arguments.
//
//      For every CPU instruction CCoder86 has one (or several)
//      member routines that are named same way as assembler
//      instruction mnemonics.
//
//  Usage pattern:
//
//      1. Define a class derived from CCoder86 and implement
//          Emit() functions - see Assemble.h.
//
//      2. Create an instance of CMyCoder:
//          CMyCoder myCoder; // might require some arguments to initialize
//                            // arrays or files
//
//      3. Call CCoder86 methods, like following:
//          myCoder.push(eax);
//          myCoder.add(eax, ebx);
//          myCoder.pxor(xmm1,xmm2);
//
//          Each call will result with one or several calls to Emit* so that
//          ready to use binary code is available immediately.
//
//      4. Take the size of binary data as myCoder.GetCount().
//         This also can be done at intermediate moments to get offsets
//         to particular instructions, say in order to makeup jump
//         instructions.
//
//-----------------------------------------------------------------------------
class CCoder86
{
public:
    CCoder86() : m_uCount(0) {};

    virtual void Emit(UINT32 data) = 0;
    virtual void Emit4(UINT32 data) = 0;
    virtual void EmitOpcode(UINT32 opcode) = 0;
    virtual UINT_PTR GetBase() const = 0;
    UINT32 GetCount() const {return m_uCount;}
    void SetCount(UINT32 uCount) {m_uCount = uCount;}

private:
    //
    // Emit basic single register instruction.
    //
    void EmitCmdReg(UINT32 opcode, UINT32 reg, UINT32 immSize, UINT32 immData)
    {
#if WPFGFX_FXJIT_X86
#else //_AMD64_   
        // Compose REX prefix, correct register index
        if (reg & 8) { opcode |= REX_B; reg &= 7; }
#endif
        WarpAssert(reg < 8);
        EmitOpcode(opcode | (reg << OpcShiftByte2));

        switch (immSize)
        {
        case 0: break;
        case 1: Emit(immData); break;
        case 4: Emit4(immData); break;
        default: NO_DEFAULT;
        }
    }

    //
    // Emit basic register-register instruction.
    //
    void EmitCmdRegReg(UINT32 opcode, UINT32 dstReg, UINT32 srcReg, UINT32 immSize, UINT32 immData)
    {
        if (opcode & OpcReversed)
        {
            UINT32 t = dstReg; dstReg = srcReg; srcReg = t;
        }

#if WPFGFX_FXJIT_X86
#else //_AMD64_    
        // Compose REX prefix, correct register indices

        if (dstReg & 8) { opcode |= REX_R; dstReg &= 7; }
        if (srcReg & 8) { opcode |= REX_B; srcReg &= 7; }
#endif
        EmitOpcode(opcode);

        UINT8 mod = 3;

        Emit((mod<<6) | (dstReg << 3) | srcReg);

        switch (immSize)
        {
        case 0: break;
        case 1: Emit(immData); break;
        case 4: Emit4(immData); break;
        default: NO_DEFAULT;
        }
    }

    //
    //      void EmitCmdRegMem()
    // Emit basic register-memory instruction.
    //
    // This routine constructs an instruction consisting of following parts:
    //      - opcode (1 or more bytes)
    //      - ModR/M byte
    //      - optional SIB (scale-index-base) byte
    //      - optional 1- or 4-bytes displacement
    //
    //  ModR/M byte has three fields: (Mod << 6) | (dstReg << 3) | R/M.
    //  The field "Mod" defines the way how displacement is used:
    //      00: no displacement unless R/M == 5
    //      01: 1-byte displacement
    //      10: 4-byte displacement
    //  Case Mod == 00 has an exception: when R/M == 5 then 4-byte displacement is required.
    //
    //  The field "R/M" in most cases points to the base register.
    //  Exceptions are following:
    //      R/M = 100 means that SIB byte presents
    //                (this means that EBP can't be pointed by R/M;
    //                 however it still can be used as base register thru SIB)
    //      Mod = 00, R/M = 101 means no SIB and 4-byte displacement
    //                (this means that ESP can't be pointed by R/M without displacement)
    //
    // SIB byte has three fields: (scale<<6) | (index << 3) | base.
    // The meaning of "index" (index register) and "base" (base register)
    // also has a number of tricky exceptions commented in the code below.
    //

    void EmitCmdRegMem(UINT32 opcode, UINT32 dstReg, memptr & srcMem, UINT32 immSize, UINT32 immData)
    {
        // Check for IA-32 addressing mode limitation:
        // ESP (or RSP in 64-bit mode) can not be used as an index register.
        WarpAssert(srcMem.m_index != gsp);

        UINT8 index = srcMem.m_index;
        UINT8 base  = srcMem.m_base ;

#if WPFGFX_FXJIT_X86
#else //_AMD64_     
        // Compose REX prefix, correct register indices

        if (dstReg & 8) { opcode |= REX_R; dstReg &= 7; }

        // if any of base & index are not in use,
        // following logic should not be broken
        C_ASSERT((gpr_none & 8) == 0);

        if (base  & 8) { base  &= 7; opcode |= REX_B; }
        if (index & 8) { index &= 7; opcode |= REX_X; }
#endif

        INT_PTR disp = srcMem.m_nDisplacement;
        UINT8 mod = 0;
        UINT8 r_m = base;

        EmitOpcode(opcode);


        if (index == gpr_none)
        {
            index = 4;
            if (base == gpr_none)
            {
                r_m = 5;
                // When Mod == 00 then base == 101 is treated as "no base"
#if WPFGFX_FXJIT_X86
#else  //_AMD64_            
                // The combination of Mod == 00 and R/M = 101 has a special
                // meaning in 64-bit mode: RIP-relative addressing
                UINT_PTR ripAfter = GetBase() + GetCount() + 5 + immSize;
                disp -= ripAfter;
#endif
            }
            else if (disp)
            {
                // Mod 00 can't be used with EBP/RBP as an index
                mod = 2;
                if (disp >= -0x80 && disp < 0x80)
                    mod = 1;
            }
            else if (base == 5)
            {
                WarpAssert(r_m == 5);
                // Mod 00 and R/M 101 is an exception;
                // using ESP/RSP/ as base needs mode 01
                mod = 1;
            }
        }
        else
        {
            r_m = 4;
            if (base == gpr_none)
            {
                base = 5;
                // When Mod == 00 then base == 101 is treated as "no base"
            }
            else if (disp)
            {
                // In 32-bit mode base = 101 stands for EBP.
                // In 64-bit mode it might appear on both RBP and R13.
                // In both cases we can't use Mod 00 because it'll be
                // treated as "no base"
                mod = 2;
                if (disp >= -0x80 && disp < 0x80)
                    mod = 1;
            }
            else if (base == 5)
            {
                // In 32-bit mode base = 101 stands for EBP.
                // In 64-bit mode it might appear on both RBP and R13.
                // In both cases we can't use Mod 00 because it'll be
                // treated as "no base"
                mod = 1;
            }
        }

        // Compose ModR/M byte
        Emit((mod << 6) | (dstReg << 3) | r_m);

        // Compose SIB byte
        if (r_m == 4)
        {
            Emit((srcMem.m_scale<<6) | (index << 3) | base);
        }

        // Compose displacement
        if (mod == 1)
        {
            Emit(static_cast<UINT8>(disp));
        }
        else if (mod == 2 || r_m == 5 || (r_m == 4 && base == 5))
        {
#if WPFGFX_FXJIT_X86
            Emit4(disp);
#else //_AMD64_
            WarpAssert(GetBase() == 0 || (disp >= -(INT_PTR)0x80000000 && disp < (INT_PTR)0x80000000));
            Emit4(static_cast<UINT32>(disp));
#endif
        }

        switch (immSize)
        {
        case 0: break;
        case 1: Emit(immData); break;
        case 4: Emit4(immData); break;
        default: NO_DEFAULT;
        }
    }

    void EmitCmdMemReg(UINT32 opcode, memptr & dstMem, UINT32 srcReg, UINT32 immSize, UINT32 immData)
    {
        EmitCmdRegMem(opcode, srcReg, dstMem, immSize, immData);
    }

    void EmitCmdRegImm(UINT32 opcodeSmall, UINT32 opcodeLarge, UINT32 opcodeEx, UINT32 dstReg, int immData)
    {
        bool fIsSmall = immData >= -0x80 && immData < 0x80;

        UINT32 opcode = fIsSmall ? opcodeSmall : opcodeLarge;

#if WPFGFX_FXJIT_X86
#else //_AMD64_        
        // Compose REX prefix, correct register indices

        // Issue: imul instruction uses opcodeEx parameter for destination register
        // and dstReg for source register. Consider less clumsy coding.

        if (dstReg   & 8) { opcode |= REX_B;   dstReg &= 7; }
        if (opcodeEx & 8) { opcode |= REX_R; opcodeEx &= 7; }
#endif
        EmitOpcode(opcode);

        UINT8 mod = 3;

        WarpAssert(dstReg < 8);
        Emit((mod<<6) | (opcodeEx << 3) | dstReg);

        if (fIsSmall)
            Emit(immData);
        else
            Emit4(immData);
    }

public:

    void cmd(UINT32 opcode, CRegID dst, CRegID src, UINT32 immSize = 0, UINT32 immData = 0)
    {
        EmitCmdRegReg(opcode, dst.IndexInGroup(), src.IndexInGroup(), immSize, immData);
    }

    void cmd(UINT32 opcode, CRegID dst, memptr src, UINT32 immSize = 0, UINT32 immData = 0)
    {
        EmitCmdRegMem(opcode, dst.IndexInGroup(), src, immSize, immData);
    }

    void cmd(UINT32 opcode, memptr dst, CRegID src, UINT32 immSize = 0, UINT32 immData = 0)
    {
        EmitCmdMemReg(opcode, dst, src.IndexInGroup(), immSize, immData);
    }

    void cmd(UINT32 opcode, RegGPR dst, RegGPR src, UINT32 immSize = 0, UINT32 immData = 0)
    {
        EmitCmdRegReg(opcode, dst, src, immSize, immData);
    }
    void cmd(UINT32 opcode, RegGPR dst, dword src, UINT32 immSize = 0, UINT32 immData = 0)
    {
        EmitCmdRegMem(opcode, dst, src, immSize, immData);
    }
    void cmd(UINT32 opcode, dword dst, RegGPR src, UINT32 immSize = 0, UINT32 immData = 0)
    {
        EmitCmdMemReg(opcode, dst, src, immSize, immData);
    }

    void mov(RegGPR dst, RegGPR src)    { EmitCmdRegReg(OPCODE(Prefix_None, 0x8B), dst, src, 0, 0); }
    void mov(RegGPR dst, dword  src)    { EmitCmdRegMem(OPCODE(Prefix_None, 0x8B), dst, src, 0, 0); }
    void mov(dword  dst, RegGPR src)    { EmitCmdMemReg(OPCODE(Prefix_None, 0x89), dst, src, 0, 0); }

    void movImm     (RegGPR dst, int value) { EmitCmdReg(OPCODE(Prefix_None, 0xB8), dst, 4, value); }
    void movImmWhole(RegGPR dst, INT_PTR value)
    {
#if WPFGFX_FXJIT_X86
        EmitCmdReg(OPCODE(Prefix_None, 0xB8), dst, 4, value);
#else //_AMD64_
        // "mov immediate" instruction in 64-bit system, dislike other
        // instructions, accepts 64-bit immediate data
        UINT32 opcode = OPCODE(Prefix_None, 0xB8) | REX_W;
        UINT32 reg = dst;

        if (reg & 8) { opcode |= REX_B; reg &= 7; }

        WarpAssert(reg < 8);
        EmitOpcode(opcode | (reg << OpcShiftByte2));

        Emit4(static_cast<INT32>(value));
        Emit4(static_cast<INT32>(value >> 32));
#endif
    }

    void movImm(dword dst, int value) { EmitCmdRegMem(OPCODE(Prefix_None, 0xC7), 0, dst, 4, value); }

    void addImm(RegGPR dst, UINT32 imm)  { EmitCmdRegImm(OPCODE(Prefix_None, 0x83), OPCODE(Prefix_None, 0x81), 0, dst, imm); }
    void  orImm(RegGPR dst, UINT32 imm)  { EmitCmdRegImm(OPCODE(Prefix_None, 0x83), OPCODE(Prefix_None, 0x81), 1, dst, imm); }
    void andImm(RegGPR dst, UINT32 imm)  { EmitCmdRegImm(OPCODE(Prefix_None, 0x83), OPCODE(Prefix_None, 0x81), 4, dst, imm); }
    void subImm(RegGPR dst, UINT32 imm)  { EmitCmdRegImm(OPCODE(Prefix_None, 0x83), OPCODE(Prefix_None, 0x81), 5, dst, imm); }
    void xorImm(RegGPR dst, UINT32 imm)  { EmitCmdRegImm(OPCODE(Prefix_None, 0x83), OPCODE(Prefix_None, 0x81), 6, dst, imm); }
    void cmpImm(RegGPR dst, UINT32 imm)  { EmitCmdRegImm(OPCODE(Prefix_None, 0x83), OPCODE(Prefix_None, 0x81), 7, dst, imm); }

#if WPFGFX_FXJIT_X86
    void addImmWhole(RegGPR dst, UINT32 imm)  { addImm(dst, imm); }
    void  orImmWhole(RegGPR dst, UINT32 imm)  {  orImm(dst, imm); }
    void andImmWhole(RegGPR dst, UINT32 imm)  { andImm(dst, imm); }
    void subImmWhole(RegGPR dst, UINT32 imm)  { subImm(dst, imm); }
    void xorImmWhole(RegGPR dst, UINT32 imm)  { xorImm(dst, imm); }
    void cmpImmWhole(RegGPR dst, UINT32 imm)  { cmpImm(dst, imm); }
#else // _AMD64_
    void addImmWhole(RegGPR dst, UINT32 imm)  { EmitCmdRegImm(OPCODE(Prefix_None, 0x83) | REX_W, OPCODE(Prefix_None, 0x81) | REX_W, 0, dst, imm); }
    void  orImmWhole(RegGPR dst, UINT32 imm)  { EmitCmdRegImm(OPCODE(Prefix_None, 0x83) | REX_W, OPCODE(Prefix_None, 0x81) | REX_W, 1, dst, imm); }
    void andImmWhole(RegGPR dst, UINT32 imm)  { EmitCmdRegImm(OPCODE(Prefix_None, 0x83) | REX_W, OPCODE(Prefix_None, 0x81) | REX_W, 4, dst, imm); }
    void subImmWhole(RegGPR dst, UINT32 imm)  { EmitCmdRegImm(OPCODE(Prefix_None, 0x83) | REX_W, OPCODE(Prefix_None, 0x81) | REX_W, 5, dst, imm); }
    void xorImmWhole(RegGPR dst, UINT32 imm)  { EmitCmdRegImm(OPCODE(Prefix_None, 0x83) | REX_W, OPCODE(Prefix_None, 0x81) | REX_W, 6, dst, imm); }
    void cmpImmWhole(RegGPR dst, UINT32 imm)  { EmitCmdRegImm(OPCODE(Prefix_None, 0x83) | REX_W, OPCODE(Prefix_None, 0x81) | REX_W, 7, dst, imm); }
#endif

    void test(RegGPR dst, RegGPR src) { EmitCmdRegReg(OPCODE(Prefix_None, 0x85), src, dst, 0, 0); } // src & dst reversed on purpose
    void test( dword dst, RegGPR src) { EmitCmdMemReg(OPCODE(Prefix_None, 0x85), dst, src, 0, 0); }
    //??void test(RegGPR src, UINT32 imm) { EmitCmdRegImm(OPCODE(Prefix_None, 0xF7), 0, src, 4, imm); }

    void imulImm(RegGPR dst, RegGPR src, int imm) { EmitCmdRegImm(OPCODE(Prefix_None, 0x6B), OPCODE(Prefix_None, 0x69), (UINT8)dst, src, imm); }
    void div(RegGPR src, bool fSigned) { EmitCmdRegReg(OPCODE(Prefix_None, 0xF7), fSigned ? 7 : 6, src, 0, 0); }
    void div(memptr src, bool fSigned) { EmitCmdRegMem(OPCODE(Prefix_None, 0xF7), fSigned ? 7 : 6, src, 0, 0); }
    void cdq() { Emit(0x99); }

#if WPFGFX_FXJIT_X86
    void inc(RegGPR dst) { Emit((0x40) | dst); }
    void dec(RegGPR dst) { Emit((0x48) | dst); }

#else //_AMD64_
    // code values 0x04* in 64-bit mode are reserved for REX prefixes
    void inc(RegGPR dst) { EmitCmdRegReg(OPCODE(Prefix_None, 0xFF), dst, 0, 0, 0); }
    void dec(RegGPR dst) { EmitCmdRegReg(OPCODE(Prefix_None, 0xFF), dst, 1, 0, 0); }
#endif

    void push(RegGPR src) { EmitCmdReg(OPCODE(Prefix_None, 0x50), src, 0, 0); }
    void pop (RegGPR dst) { EmitCmdReg(OPCODE(Prefix_None, 0x58), dst, 0, 0); }

    void push(int value) { Emit(0x68); Emit4(value); }

    void shr(RegGPR dst, UINT32 immed)
    {
        if(1 == immed)
        {
            EmitCmdRegReg(OPCODE(Prefix_None, 0xD1), 5, dst, 0, 0);
        }
        else
        {
            EmitCmdRegReg(OPCODE(Prefix_None, 0xC1), 5, dst, 1, immed);
        }
    }

    void shl(RegGPR dst, UINT32 immed)
    {
        if(1 == immed)
        {
            EmitCmdRegReg(OPCODE(Prefix_None, 0xD1), 4, dst, 0, 0);
        }
        else
        {
            EmitCmdRegReg(OPCODE(Prefix_None, 0xC1), 4, dst, 1, immed);
        }
    }
    void shl(RegGPR dst)  { EmitCmdRegReg(OPCODE(Prefix_None, 0xD3), 4, dst, 0, 0); } // dst <<= cl
    void shr(RegGPR dst)  { EmitCmdRegReg(OPCODE(Prefix_None, 0xD3), 5, dst, 0, 0); } // dst >>= cl
    void sal(RegGPR dst)  { EmitCmdRegReg(OPCODE(Prefix_None, 0xD3), 4, dst, 0, 0); } // dst <<= cl
    void sar(RegGPR dst)  { EmitCmdRegReg(OPCODE(Prefix_None, 0xD3), 7, dst, 0, 0); } // dst >>= cl

#if WPFGFX_FXJIT_X86
#else // _AMD64_
    void shrWhole(RegGPR dst, UINT32 immed) { EmitCmdRegReg(OPCODE(Prefix_None, 0xC1) | REX_W, 5, dst, 1, immed); }
    void shrWhole(RegGPR dst              ) { EmitCmdRegReg(OPCODE(Prefix_None, 0xD3) | REX_W, 5, dst, 0, 0    ); } // dst >>= cl
    void shlWhole(RegGPR dst, UINT32 immed) { EmitCmdRegReg(OPCODE(Prefix_None, 0xC1) | REX_W, 4, dst, 1, immed); }
    void shlWhole(RegGPR dst             )  { EmitCmdRegReg(OPCODE(Prefix_None, 0xD3) | REX_W, 4, dst, 0, 0    ); } // dst <<= cl
#endif

    // the "label" argument in following methods is an offset relative to code snippet base address
    void je(UINT32 label)
    {
        Emit(0x0F);
        Emit(0x84);
        UINT32 offset = label - (GetCount() + 4);
        Emit4(offset);
    }

    void jne(UINT32 label)
    {
        Emit(0x0F);
        Emit(0x85);
        UINT32 offset = label - (GetCount() + 4);
        Emit4(offset);
    }

    void jc(UINT32 label)
    {
        Emit(0x0F);
        Emit(0x82);
        UINT32 offset = label - (GetCount() + 4);
        Emit4(offset);
    }

    void jnc(UINT32 label)
    {
        Emit(0x0F);
        Emit(0x83);
        UINT32 offset = label - (GetCount() + 4);
        Emit4(offset);
    }

    void jmp(UINT32 label)
    {
        Emit(0xE9);
        UINT32 offset = label - (GetCount() + 4);
        Emit4(offset);
    }

    void jmp(CRegID address)
    {
        EmitCmdRegReg(OPCODE(Prefix_None, 0xFF), 4, address.IndexInGroup(), 0, 0);
    }

#if WPFGFX_FXJIT_X86
    //
    // 64-bit mode has a constraint: offset can be only 32-bit
    // signed integer, so not every address is reachable.
    //
    void callImm(int label)
    {
        Emit(0xE8);
        INT_PTR offset = label - (GetCount() + 4) - GetBase();
        Emit4(offset);
    }
#endif //WPFGFX_FXJIT_X86

    void call(CRegID address)
    {
        EmitCmdRegReg(OPCODE(Prefix_None, 0xFF), 2, address.IndexInGroup(), 0, 0);
    }

    void ret(UINT32 popBytes)
    {
        WarpAssert(popBytes <= 0xFFFF);

        if (0 == popBytes)
        {
            Emit(0xC3);
        }
        else
        {
            Emit(0xC2);
            Emit((UINT8)popBytes);
            Emit((UINT8)(popBytes>>8));
        }
    }

#if WPFGFX_FXJIT_X86
    // MMX
    void movd(RegMMX dst, RegGPR src) { EmitCmdRegReg(OPCODE(Prefix_None, 0x0F6E), dst, src, 0, 0); }
    void movd(RegMMX dst, dword src)  { EmitCmdRegMem(OPCODE(Prefix_None, 0x0F6E), dst, src, 0, 0); }
    void movd(RegGPR dst, RegMMX src) { EmitCmdRegReg(OPCODE(Prefix_None, 0x0F7E), dst, src, 0, 0); }
    void movd(dword dst, RegMMX src)  { EmitCmdMemReg(OPCODE(Prefix_None, 0x0F7E), dst, src, 0, 0); }

    void pminsw( RegMMX dst,  RegMMX src) { EmitCmdRegReg(OPCODE(Prefix_None, 0x0FEA), dst, src, 0, 0); }
    void pminsw( RegMMX dst, mmxword src) { EmitCmdRegMem(OPCODE(Prefix_None, 0x0FEA), dst, src, 0, 0); }

    void psrlw(RegMMX dst, int immed) { EmitCmdRegReg(OPCODE(Prefix_None, 0x0F71), 2, dst, 1, immed); }
    void psraw(RegMMX dst, int immed) { EmitCmdRegReg(OPCODE(Prefix_None, 0x0F71), 4, dst, 1, immed); }
    void psllw(RegMMX dst, int immed) { EmitCmdRegReg(OPCODE(Prefix_None, 0x0F71), 6, dst, 1, immed); }

    void psrld(RegMMX dst, int immed) { EmitCmdRegReg(OPCODE(Prefix_None, 0x0F72), 2, dst, 1, immed); }
    void psrad(RegMMX dst, int immed) { EmitCmdRegReg(OPCODE(Prefix_None, 0x0F72), 4, dst, 1, immed); }
    void pslld(RegMMX dst, int immed) { EmitCmdRegReg(OPCODE(Prefix_None, 0x0F72), 6, dst, 1, immed); }

    void movntq(mmxword dst, RegMMX src) { EmitCmdMemReg(OPCODE(Prefix_None, 0x0FE7), dst, src, 0, 0); }
    void maskmovq(RegMMX src, RegMMX msk) { EmitCmdRegReg(OPCODE(Prefix_660F, 0xF7), src, msk, 0, 0); }

#endif //WPFGFX_FXJIT_X86

    // SSE

    void cmd(UINT32 opcode, RegXMM dst, RegXMM src, UINT32 immSize, UINT32 immData)
    {
        EmitCmdRegReg(opcode, dst, src, immSize, immData);
    }
    void cmd(UINT32 opcode, RegXMM dst, memptr src, UINT32 immSize, UINT32 immData)
    {
        EmitCmdRegMem(opcode, dst, src, immSize, immData);
    }
    void cmd(UINT32 opcode, memptr dst, RegXMM src, UINT32 immSize, UINT32 immData)
    {
        EmitCmdMemReg(opcode, dst, src, immSize, immData);
    }

    void movups(RegXMM dst, RegXMM src) { EmitCmdRegReg(OPCODE(Prefix_None, 0x0F10), dst, src, 0, 0); }
    void movups(RegXMM dst, xmmword src) { EmitCmdRegMem(OPCODE(Prefix_None, 0x0F10), dst, src, 0, 0); }
    void movups(xmmword dst, RegXMM src) { EmitCmdMemReg(OPCODE(Prefix_None, 0x0F11), dst, src, 0, 0); }

    void movhlps(RegXMM dst, RegXMM src) { EmitCmdRegReg(OPCODE(Prefix_None, 0x0F12), dst, src, 0, 0); }
    void movhlps(RegXMM dst, xmmword src) { EmitCmdRegMem(OPCODE(Prefix_None, 0x0F12), dst, src, 0, 0); }

    void movlps(RegXMM dst, mmxword src) { EmitCmdRegMem(OPCODE(Prefix_None, 0x0F12), dst, src, 0, 0); }
    void movlps(mmxword dst, RegXMM src) { EmitCmdMemReg(OPCODE(Prefix_None, 0x0F13), dst, src, 0, 0); }

    void movlhps(RegXMM dst, RegXMM src) { EmitCmdRegReg(OPCODE(Prefix_None, 0x0F16), dst, src, 0, 0); }
    void movlhps(RegXMM dst, xmmword src) { EmitCmdRegMem(OPCODE(Prefix_None, 0x0F16), dst, src, 0, 0); }

    void movhps(RegXMM dst, mmxword src) { EmitCmdRegMem(OPCODE(Prefix_None, 0x0F16), dst, src, 0, 0); }
    void movhps(mmxword dst, RegXMM src) { EmitCmdMemReg(OPCODE(Prefix_None, 0x0F17), dst, src, 0, 0); }

    void movmskps(RegGPR dst, RegXMM src) { EmitCmdRegReg(OPCODE(Prefix_None, 0x0F50), dst, src, 0, 0); }

    // SSE2

#if WPFGFX_FXJIT_X86
    void movq2dq(RegXMM dst, RegMMX src) { EmitCmdRegReg(OPCODE(Prefix_F30F, 0xD6), dst, src, 0, 0); }
    void movdq2q(RegMMX dst, RegXMM src) { EmitCmdRegReg(OPCODE(Prefix_F20F, 0xD6), dst, src, 0, 0); }
#endif

    void maskmovdqu(RegXMM src, RegXMM msk) { EmitCmdRegReg(OPCODE(Prefix_660F, 0xF7), src, msk, 0, 0); }

    void psubd(RegXMM dst, RegXMM src) { EmitCmdRegReg(OPCODE(Prefix_660F, 0xFA), dst, src, 0, 0); }
    void psubd(RegXMM dst, xmmword src) { EmitCmdRegMem(OPCODE(Prefix_660F, 0xFA), dst, src, 0, 0); }

    void psrlw(RegXMM dst, int immed) { EmitCmdRegReg(OPCODE(Prefix_660F, 0x71), 2, dst, 1, immed); }
    void psraw(RegXMM dst, int immed) { EmitCmdRegReg(OPCODE(Prefix_660F, 0x71), 4, dst, 1, immed); }
    void psllw(RegXMM dst, int immed) { EmitCmdRegReg(OPCODE(Prefix_660F, 0x71), 6, dst, 1, immed); }

    void psrld(RegXMM dst, int immed) { EmitCmdRegReg(OPCODE(Prefix_660F, 0x72), 2, dst, 1, immed); }
    void psrad(RegXMM dst, int immed) { EmitCmdRegReg(OPCODE(Prefix_660F, 0x72), 4, dst, 1, immed); }
    void pslld(RegXMM dst, int immed) { EmitCmdRegReg(OPCODE(Prefix_660F, 0x72), 6, dst, 1, immed); }

    void pslld(RegXMM dst, RegXMM src) { EmitCmdRegReg(OPCODE(Prefix_660F, 0xF2), dst, src, 0, 0); }

    void psrldq(RegXMM dst, int immed) { EmitCmdRegReg(OPCODE(Prefix_660F, 0x73), 3, dst, 1, immed); }
    void pslldq(RegXMM dst, int immed) { EmitCmdRegReg(OPCODE(Prefix_660F, 0x73), 7, dst, 1, immed); }


    // SSE4.1
    void ptest(RegXMM dst, RegXMM src) { EmitCmdRegReg(OPCODE(Prefix_660F, 0x3817), dst, src, 0, 0); }

    // Mix

    void emms()
    {
        Emit(0x0F);
        Emit(0x77);
    }
    void mfence()
    {
        Emit(0x0F);
        Emit(0xAE);
        Emit(0xF0);
    }

    void BreakPoint()
    {
        Emit(0xCC); // __asm int 3;
    }

protected:
    UINT32 m_uCount;
};



