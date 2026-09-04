// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Classes and enums to represent IA-32 registers.
//
//-----------------------------------------------------------------------------
#pragma once

#if WPFGFX_FXJIT_X86
enum RegGPR : UINT8
{
    reg_eax = 0,
    reg_ecx = 1,
    reg_edx = 2,
    reg_ebx = 3,
    reg_esp = 4,
    reg_ebp = 5,
    reg_esi = 6,
    reg_edi = 7,

    gpr_none = 8, // used in memptr to denote unused base or index register

    gsp = reg_esp,  // cross-platform
    gbp = reg_ebp,  // cross-platform
    gax = reg_eax,  // cross-platform
    gcx = reg_ecx,  // cross-platform
    gdx = reg_edx,  // cross-platform
    gdi = reg_edi,  // cross-platform
};

enum RegMMX : UINT8
{
    reg_mm0 = 0,
    reg_mm1 = 1,
    reg_mm2 = 2,
    reg_mm3 = 3,
    reg_mm4 = 4,
    reg_mm5 = 5,
    reg_mm6 = 6,
    reg_mm7 = 7,
};

enum RegXMM : UINT8
{
    reg_xmm0 = 0,
    reg_xmm1 = 1,
    reg_xmm2 = 2,
    reg_xmm3 = 3,
    reg_xmm4 = 4,
    reg_xmm5 = 5,
    reg_xmm6 = 6,
    reg_xmm7 = 7,
};

enum RegisterType : UINT8
{
    rtGPR = 0,
    rtMMX = 1,
    rtXMM = 2,
};

static const UINT32 g_uRegsInGroup = 8;
static const UINT32 g_uRegsTotal = 24;
static const UINT32 g_uRegMask = 0x07;
static const UINT32 g_uRegGroupMask = 0x18;
static const UINT32 g_uRegGroupOffset = 3;

#else //_AMD64_

enum RegGPR : UINT8
{
    reg_rax = 0,
    reg_rcx = 1,
    reg_rdx = 2,
    reg_rbx = 3,
    reg_rsp = 4,
    reg_rbp = 5,
    reg_rsi = 6,
    reg_rdi = 7,
    reg_r8 = 8,
    reg_r9 = 9,
    reg_r10 = 10,
    reg_r11 = 11,
    reg_r12 = 12,
    reg_r13 = 13,
    reg_r14 = 14,
    reg_r15 = 15,

    gpr_none = 16, // used in memptr to denote unused base or index register

    gsp = reg_rsp,  // cross-platform
    gbp = reg_rbp,  // cross-platform
    gax = reg_rax,  // cross-platform
    gcx = reg_rcx,  // cross-platform
    gdx = reg_rdx,  // cross-platform
    gdi = reg_rdi,  // cross-platform
};

enum RegXMM : UINT8
{
    reg_xmm0 = 0,
    reg_xmm1 = 1,
    reg_xmm2 = 2,
    reg_xmm3 = 3,
    reg_xmm4 = 4,
    reg_xmm5 = 5,
    reg_xmm6 = 6,
    reg_xmm7 = 7,
    reg_xmm8 = 8,
    reg_xmm9 = 9,
    reg_xmm10 = 10,
    reg_xmm11 = 11,
    reg_xmm12 = 12,
    reg_xmm13 = 13,
    reg_xmm14 = 14,
    reg_xmm15 = 15,
};

enum RegisterType : UINT8
{
    rtGPR = 0,
    rtXMM = 1,
};

static const UINT32 g_uRegsTotal = 32;
static const UINT32 g_uRegsInGroup = 16;
static const UINT32 g_uRegMask = 0x0F;
static const UINT32 g_uRegGroupMask = 0x10;
static const UINT32 g_uRegGroupOffset = 4;

#endif

//+------------------------------------------------------------------------------
//
//  Class:
//      CRegID
//
//  Synopsis:
//      Represent one of IA-32 register, including general purpose,
//      MMX and XMM registers.
//      Provides safe casting to particular register type.
//-------------------------------------------------------------------------------
class CRegID
{
public:
    CRegID() { m_bData = g_uRegsTotal; } // >= g_uRegsTotal means "undefined"

    CRegID(RegGPR r) { m_bData = (UINT8)(r | (rtGPR << g_uRegGroupOffset)); }
    CRegID(RegXMM r) { m_bData = (UINT8)(r | (rtXMM << g_uRegGroupOffset)); }

    CRegID(UINT32 r) { m_bData = (UINT8)r; }
    CRegID(RegisterType rt, UINT32 uIndex)
    {
        WarpAssert(uIndex < g_uRegsInGroup);
        m_bData = (UINT8)((rt << g_uRegGroupOffset) | uIndex);
    }

    void Clear() { m_bData = g_uRegsTotal; }

    RegGPR GPR() const { WarpAssert(GetRegType() == rtGPR); return (RegGPR)(m_bData & g_uRegMask); }
    RegXMM XMM() const { WarpAssert(GetRegType() == rtXMM); return (RegXMM)(m_bData & g_uRegMask); }

    UINT32 Index() const { WarpAssert(IsDefined()); return m_bData; }
    UINT32 IndexInGroup() const { WarpAssert(IsDefined()); return m_bData & g_uRegMask; }

    bool IsDefined() const { return m_bData < g_uRegsTotal; }
    RegisterType GetRegType() const { return (RegisterType)(m_bData >> g_uRegGroupOffset); }

    CRegID& operator=(RegGPR r) { m_bData = (UINT8)(r | (rtGPR << g_uRegGroupOffset)); }
    CRegID& operator=(RegXMM r) { m_bData = (UINT8)(r | (rtXMM << g_uRegGroupOffset)); }

    CRegID& operator=(UINT32 r) { m_bData = (UINT8)r; return *this; }

    bool operator==(CRegID const & other) const { return m_bData == other.m_bData; }
    bool operator!=(CRegID const & other) const { return m_bData != other.m_bData; }

#if WPFGFX_FXJIT_X86
    CRegID(RegMMX r) { m_bData = (UINT8)(r | (rtMMX << g_uRegGroupOffset)); }
    RegMMX MMX() const { WarpAssert(GetRegType() == rtMMX); return (RegMMX)(m_bData & g_uRegMask); }
    CRegID& operator=(RegMMX r) { m_bData = (UINT8)(r | (rtMMX << g_uRegGroupOffset)); }
#endif

private:
    UINT8 m_bData;
};


