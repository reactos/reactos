// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on vectors of 4 32-bit values.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Struct:
//      u32x4
//
//  Synopsis:
//      Represents in-memory value of C_u32x4.
//
//------------------------------------------------------------------------------
struct u32x4
{
    UINT32 data[4];
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_u32x4
//
//  Synopsis:
//      Represents integer 128-bit variable in prototype program.
//      Single 128-bit value is treated as an array of 4 32-bit words.
//
//
//------------------------------------------------------------------------------
class C_u32x4 : public C_XmmValue
{
public:
    C_u32x4() {}

    C_u32x4(C_u32x4 const & origin);
    C_u32x4 const& operator=(C_u32x4 const& origin);

    C_u32x4(C_u32 const & origin);
    C_u32x4& operator=(C_u32 const& origin);

    C_u32x4(u32x4 const & origin);
    C_u32x4& operator=(u32x4 const& origin);

    C_u32x4(UINT32 const & origin);
    C_u32x4& operator=(UINT32 const& origin);

    C_u32x4 operator+ (C_u32x4 const& other) const { return BinaryOperation(otXmmDWordsAdd, other); }
    C_u32x4 operator- (C_u32x4 const& other) const { return BinaryOperation(otXmmDWordsSub, other); }
    C_u32x4 operator==(C_u32x4 const& other) const { return BinaryOperation(otXmmDWordsEqual, other); }
    C_u32x4 operator& (C_u32x4 const& other) const { return BinaryOperation(otXmmIntAnd, other); }
    C_u32x4 operator| (C_u32x4 const& other) const { return BinaryOperation(otXmmIntOr , other); }
    C_u32x4 operator^ (C_u32x4 const& other) const { return BinaryOperation(otXmmIntXor, other); }
    C_u32x4 operator* (C_u32x4 const& other) const { AssertSSE41(); return BinaryOperation(otXmmIntMul, other); }
    C_u32x4 Max(C_u32x4 const& other) const;
    C_u32x4 Min(C_u32x4 const& other) const;

    C_u64x2 UnsignedMul(C_u32x4 const& other) const;
    C_u64x2 SignedMul(C_u32x4 const& other) const;
    C_u64x2 InterleaveLow (C_u32x4 const& other) const { return BinaryOperation(otXmmDWordsInterleaveLow , other); }
    C_u64x2 InterleaveHigh(C_u32x4 const& other) const { return BinaryOperation(otXmmDWordsInterleaveHigh, other); }
    C_u16x8 PackSignedSaturate(C_u32x4 const &other) const;

    C_u32x4 operator+ (u32x4 const& src) const { return BinaryOperation(otXmmDWordsAdd, src); }
    C_u32x4 operator- (u32x4 const& src) const { return BinaryOperation(otXmmDWordsSub, src); }
    C_u32x4 operator==(u32x4 const& src) const { return BinaryOperation(otXmmDWordsEqual, src); }
    C_u32x4 operator& (u32x4 const& src) const { return BinaryOperation(otXmmIntAnd, src); }
    C_u32x4 operator| (u32x4 const& src) const { return BinaryOperation(otXmmIntOr , src); }
    C_u32x4 operator^ (u32x4 const& src) const { return BinaryOperation(otXmmIntXor, src); }
    C_u32x4 operator* (u32x4 const& src) const { AssertSSE41(); return BinaryOperation(otXmmIntMul, src); }

    C_u64x2 InterleaveLow (u32x4 const& src) const { return BinaryOperation(otXmmDWordsInterleaveLow , src); }
    C_u64x2 InterleaveHigh(u32x4 const& src) const { return BinaryOperation(otXmmDWordsInterleaveHigh, src); }
    C_u16x8 PackSignedSaturate(u32x4 const &src) const;

    C_u32x4& operator+=(C_u32x4 const& other) { return BinaryAssignment(otXmmDWordsAdd, other); }
    C_u32x4& operator-=(C_u32x4 const& other) { return BinaryAssignment(otXmmDWordsSub, other); }
    C_u32x4& operator&=(C_u32x4 const& other) { return BinaryAssignment(otXmmIntAnd, other); }
    C_u32x4& operator|=(C_u32x4 const& other) { return BinaryAssignment(otXmmIntOr , other); }
    C_u32x4& operator^=(C_u32x4 const& other) { return BinaryAssignment(otXmmIntXor, other); }
    C_u32x4& operator*=(C_u32x4 const& other) { AssertSSE41(); return BinaryAssignment(otXmmIntMul, other); }

    C_u32x4& operator+=(u32x4 const& src) { return BinaryAssignment(otXmmDWordsAdd, src); }
    C_u32x4& operator-=(u32x4 const& src) { return BinaryAssignment(otXmmDWordsSub, src); }
    C_u32x4& operator&=(u32x4 const& src) { return BinaryAssignment(otXmmIntAnd, src); }
    C_u32x4& operator|=(u32x4 const& src) { return BinaryAssignment(otXmmIntOr , src); }
    C_u32x4& operator^=(u32x4 const& src) { return BinaryAssignment(otXmmIntXor, src); }
    C_u32x4& operator*=(u32x4 const& src) { AssertSSE41(); return BinaryAssignment(otXmmIntMul, src); }

    C_u32x4 operator+ (R_u32x4 const& ref) const { return BinaryReference(otXmmDWordsAdd, ref); }
    C_u32x4 operator- (R_u32x4 const& ref) const { return BinaryReference(otXmmDWordsSub, ref); }
    C_u32x4 operator==(R_u32x4 const& ref) const { return BinaryReference(otXmmDWordsEqual, ref); }
    C_u32x4 operator& (R_u32x4 const& ref) const { return BinaryReference(otXmmIntAnd, ref); }
    C_u32x4 operator| (R_u32x4 const& ref) const { return BinaryReference(otXmmIntOr , ref); }
    C_u32x4 operator^ (R_u32x4 const& ref) const { return BinaryReference(otXmmIntXor, ref); }
    C_u32x4 operator* (R_u32x4 const& ref) const { AssertSSE41(); return BinaryReference(otXmmIntMul, ref); }

    C_u32x4 operator<<(C_u32x4 const& other) const;
    C_u32x4 operator<<(int shift) const;
    C_u32x4& operator<<=(int shift);

    C_u32x4 operator>>(int shift) const;
    C_u32x4& operator>>=(int shift);

    C_u32x4& ShiftRight();

    C_XmmValue operator~() const;

    C_f32x4 ToFloat4() const;
    C_f32x4 As_f32x4() const;

    C_u32x4 Shuffle(UINT8 uOrder) const;
    C_u32x4 ReplicateElement(UINT32 uIndex) const
    {
        WarpAssert(uIndex < 4);
        UINT8 uOrder = (UINT8)(uIndex * 0x55);
        return Shuffle(uOrder);
    }

    C_u32 GetElement(UINT32 uIndex) const;
    C_u32 ExtractElement(UINT32 uIndex) const;
    C_u32x4 InsertElement(C_u32 const & src, UINT32 uIndex) const;
    C_u32x4 Blend(C_u32x4 const & src, C_u32x4 const & mask) const;

protected:
    C_u32x4 BinaryOperation(OpType ot, C_u32x4 const& other) const;
    C_u32x4 BinaryOperation(OpType ot, u32x4 const& src) const;
    C_u32x4& BinaryAssignment(OpType ot, C_u32x4 const& other);
    C_u32x4& BinaryAssignment(OpType ot, u32x4 const& src);
    C_u32x4 BinaryReference(OpType ot, R_u32x4 const& ref) const;
};


//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::UnsignedMul
//
//  Synopsis:
//      Performs two 32x32 bits unsigned multiplications producing two 64-bit
//      results. Low dword of "this" instances is multiplied by low dword of
//      "other"; the result is stored in low qword of result. Third dword of
//      "this" instances is multiplied by third dword of "other"; the result
//      is stored in high qword of result. Second and fourth dwords of
//      "this" and "other" are ignored.
//
//------------------------------------------------------------------------------
inline C_u64x2
C_u32x4::UnsignedMul(C_u32x4 const& other) const
{
    return BinaryOperation(otXmmDWordsUnsignedMul, other);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::SignedMul
//
//  Synopsis:
//      Performs two 32x32 bits signed multiplications producing two 64-bit
//      results. Low dword of "this" instances is multiplied by low dword of
//      "other"; the result is stored in low qword of result. Third dword of
//      "this" instances is multiplied by third dword of "other"; the result
//      is stored in high qword of result. Second and fourth dwords of
//      "this" and "other" are ignored.
//
//  This function is available on CPUs with SSE4.1 extension.
//  Here is the way to emulate SignedMul on SSE2 via UnsignedMul.
//      C_s32x4 A,B;
//      C_u64x2 UnsignedProduct = A.UnsignedMul(B);
//      C_s32x4 MaskA = A >> 31;
//      C_s32x4 MaskB = B >> 31;
//      C_u32x4 CorrectionA = (MaskA & B).Shuffle(0xB1); // (*)
//      C_u32x4 CorrectionB = (MaskB & A).Shuffle(0xB1); // (*)
//      C_u64x2 SignedProduct = UnsignedProduct - CorrectionA - CorrectionB;
//
//  (*) This example assumes A and B to contain zeros in 2nd and 4th dwords.
//
//  If the values in A or B are known to be positive, the routine
//  can be simplified by excluding CorrectionA or CorrectionB.
//
//  The computations above is based on following formulas defining the
//  numerical value of 32-bit binary representation:
//      UnsignedValue =  (1 << 31) * bit31 + (1 << 30) * bit30 + etc.;
//        SignedValue = -(1 << 31) * bit31 + (1 << 30) * bit30 + same;
//      or
//        SignedValue = (-(1 << 32) + (1 << 31)) * bit31 + (1 << 30) * bit30 + same;
//      or
//        SignedValue = -(1 << 32) * bit31 + UnsignedValue;
//
//      SignedProduct = SignedValueA * SignedValueB
//                    = (-(1 << 32) * bit31A + UnsignedValueA) * (-(1 << 32) * bit31B + UnsignedValueB)
//                    = (1 << 64)
//                    - (1 << 32) * bit31A * UnsignedValueB
//                    - (1 << 32) * bit31B * UnsignedValueA
//                    + UnsignedValueA * UnsignedValueA
//  In the latter form "UnsignedValueA * UnsignedValueA" is the result of A.UnsignedMul(B);
//  the value (1 << 64) is out of 64-bit field and might be ignored.
//------------------------------------------------------------------------------
inline C_u64x2
C_u32x4::SignedMul(C_u32x4 const& other) const
{
    AssertSSE41();
    return BinaryOperation(otXmmDWordsSignedMul  , other);
}

//
// Unpacks 1 C_u32x4 into 4 C_u32
//
class IntValueUnpacker
{
public:
    IntValueUnpacker(C_u32x4 const & src);
    C_u32 GetValue();

private:
    UINT32 m_uCount;
    C_u32x4 m_data;
    bool m_fUseSSE41;
    bool m_fAvoidMOVDs;
};


//
// Packs 4 C_u32 into 1 C_u32x4
// Packing is done on the fly to reduce register pressure
//
class IntValuePacker
{
public:
    IntValuePacker();
    void AddValue(const C_u32& src);
    C_u32x4 Result();

private:
    UINT32 m_uCount;
    C_u32   m_temp;
    C_u64x2 m_low;
    C_u64x2 m_high;
    C_u32x4 m_result;
    bool m_fUseSSE41;
};


