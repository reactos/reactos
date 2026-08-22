// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on vectors of 8 signed integer 16-bit values.
//
//-----------------------------------------------------------------------------

#pragma once
//+-----------------------------------------------------------------------------
//
//  Struct:
//      s16x8
//
//  Synopsis:
//      Represents in-memory value of C_s16x8.
//
//------------------------------------------------------------------------------
struct s16x8
{
    INT16 words[8];
};


//+-----------------------------------------------------------------------------
//
//  Class:
//      C_s16x8
//
//  Synopsis:
//      Represents integer 128-bit variable in prototype program.
//      Single 128-bit value is treated as an array of 8 signed 16-bit words.
//
//
//------------------------------------------------------------------------------
class C_s16x8 : public C_u16x8
{
public:
    C_s16x8 Min(C_s16x8 const& other) const { return BinaryOperation(otXmmWordsSignedMin, other); }
    C_s16x8 Max(C_s16x8 const& other) const { return BinaryOperation(otXmmWordsSignedMax, other); }

    C_s16x8 Min(s16x8 const& src) const { return BinaryOperation(otXmmWordsSignedMin, src); }
    C_s16x8 Max(s16x8 const& src) const { return BinaryOperation(otXmmWordsSignedMax, src); }

    C_s16x8 operator>>(int shift) const;
    C_s16x8& operator>>=(int shift);

private:
    C_s16x8 BinaryOperation(OpType ot, C_s16x8 const& other) const;
    C_s16x8 BinaryOperation(OpType ot, s16x8 const& src) const;
};



