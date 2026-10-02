// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Base class for integer 128-bit prototype variables.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_XmmValue
//
//  Synopsis:
//      Represents integer 128-bit variable in prototype program.
//      Operators of this class mostly correspond to SSE2 instructions of
//      IA-32 Intel Architecture
//
//------------------------------------------------------------------------------
class C_XmmValue : public C_Variable
{
public:
    C_XmmValue();
    C_XmmValue& operator=(C_XmmValue & origin);

    operator C_u128x1&() {return *reinterpret_cast<C_u128x1*>(this);}
    operator C_u64x2&() {return *reinterpret_cast<C_u64x2*>(this);}
    operator C_u32x4&() {return *reinterpret_cast<C_u32x4*>(this);}
    operator C_u16x8&() {return *reinterpret_cast<C_u16x8*>(this);}
    operator C_u8x16&() {return *reinterpret_cast<C_u8x16*>(this);}

    operator C_s32x4&() {return *reinterpret_cast<C_s32x4*>(this);}
    operator C_s16x8&() {return *reinterpret_cast<C_s16x8*>(this);}

    C_u128x1 const & AsC_u128x1() const {return *reinterpret_cast<C_u128x1 const *>(this);}
    C_u64x2 const & AsC_u64x2() const {return *reinterpret_cast<C_u64x2 const *>(this);}
    C_u32x4 const & AsC_u32x4() const {return *reinterpret_cast<C_u32x4 const *>(this);}
    C_u16x8 const & AsC_u16x8() const {return *reinterpret_cast<C_u16x8 const *>(this);}
    C_u8x16 const & AsC_u8x16() const {return *reinterpret_cast<C_u8x16 const *>(this);}

    C_s32x4 const & AsC_s32x4() const {return *reinterpret_cast<C_s32x4 const *>(this);}
    C_s16x8 const & AsC_s16x8() const {return *reinterpret_cast<C_s16x8 const *>(this);}

    C_u128x1 & AsC_u128x1() {return *reinterpret_cast<C_u128x1*>(this);}
    C_u64x2 & AsC_u64x2() {return *reinterpret_cast<C_u64x2*>(this);}
    C_u32x4 & AsC_u32x4() {return *reinterpret_cast<C_u32x4*>(this);}
    C_u16x8 & AsC_u16x8() {return *reinterpret_cast<C_u16x8*>(this);}
    C_u8x16 & AsC_u8x16() {return *reinterpret_cast<C_u8x16*>(this);}

    C_s32x4 & AsC_s32x4() {return *reinterpret_cast<C_s32x4*>(this);}
    C_s16x8 & AsC_s16x8() {return *reinterpret_cast<C_s16x8*>(this);}

#if WPFGFX_FXJIT_X86
    operator C_MmValue() const;
#endif

    C_u32 GetLowDWord();

    void Load64(P_u8 const & ptr);
    void Store64(P_u8 const & ptr) const;

    C_XmmValue& SetZero();

    void StoreNonTemporal(P_u128x1 const & ptr, INT32 nIndex = 0) const;
    void StoreNonTemporalMasked(P_u8 const & ptr, C_XmmValue const& mask) const;
};


