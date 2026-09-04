// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Base class for integer 64-bit prototype variables.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_MmValue
//
//  Synopsis:
//      Represents integer 64-bit variable in prototype program.
//      Operators of this class mostly correspond to MMX instruction set
//      of IA-32 Intel Architecture.
//
//------------------------------------------------------------------------------
class C_MmValue : public C_Variable
{
public:
    C_MmValue();
    C_MmValue& operator=(C_MmValue const& origin);

    operator C_u64x1&() {return *reinterpret_cast<C_u64x1*>(this);}
    operator C_u32x2&() {return *reinterpret_cast<C_u32x2*>(this);}
    operator C_u16x4&() {return *reinterpret_cast<C_u16x4*>(this);}
    operator C_u8x8 &() {return *reinterpret_cast<C_u8x8 *>(this);}

    operator C_s32x2&() {return *reinterpret_cast<C_s32x2*>(this);}
    operator C_s16x4&() {return *reinterpret_cast<C_s16x4*>(this);}

    C_u64x1 const & AsC_u64x1() const {return *reinterpret_cast<C_u64x1 const *>(this);}
    C_u32x2 const & AsC_u32x2() const {return *reinterpret_cast<C_u32x2 const *>(this);}
    C_u16x4 const & AsC_u16x8() const {return *reinterpret_cast<C_u16x4 const *>(this);}
    C_u8x8  const & AsC_u8x8 () const {return *reinterpret_cast<C_u8x8  const *>(this);}

    C_s32x2 const & AsC_s32x2() const {return *reinterpret_cast<C_s32x2 const *>(this);}
    C_s16x4 const & AsC_s16x4() const {return *reinterpret_cast<C_s16x4 const *>(this);}

    C_u64x1& AsC_u64x1() {return *reinterpret_cast<C_u64x1*>(this);}
    C_u32x2& AsC_u32x4() {return *reinterpret_cast<C_u32x2*>(this);}
    C_u16x4& AsC_u16x8() {return *reinterpret_cast<C_u16x4*>(this);}
    C_u8x8 & AsC_u8x8 () {return *reinterpret_cast<C_u8x8 *>(this);}

    C_s32x2 & AsC_s32x2() {return *reinterpret_cast<C_s32x2*>(this);}
    C_s16x4 & AsC_s16x4() {return *reinterpret_cast<C_s16x4*>(this);}

    void StoreNonTemporal(C_pVoid const& address, INT32 nIndex = 0);
};


