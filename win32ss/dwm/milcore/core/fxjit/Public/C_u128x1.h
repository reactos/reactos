// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on 128-bit values.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_u128x1
//
//  Synopsis:
//      Represents integer 128-bit variable in prototype program.
//
//
//------------------------------------------------------------------------------
class C_u128x1 : public C_XmmValue
{
public:
    C_u128x1() {}

    C_u128x1(C_u128x1 const & origin);
    C_u128x1& operator=(C_u128x1 const& origin);

    C_u128x1 operator&(C_u128x1 const& other) const { return BinaryOperation(otXmmIntAnd, other); }
    C_u128x1 operator|(C_u128x1 const& other) const { return BinaryOperation(otXmmIntOr , other); }
    C_u128x1 operator^(C_u128x1 const& other) const { return BinaryOperation(otXmmIntXor, other); }

    C_u128x1 operator&(C_f32x4 const& other) const { return BinaryOperationWithFloat(otXmmIntAnd, other); }
    C_u128x1 operator|(C_f32x4 const& other) const { return BinaryOperationWithFloat(otXmmIntOr , other); }
    C_u128x1 operator^(C_f32x4 const& other) const { return BinaryOperationWithFloat(otXmmIntXor, other); }

    C_u128x1& operator&=(C_u128x1 const& other) { return BinaryAssignment(otXmmIntAnd, other); }
    C_u128x1& operator|=(C_u128x1 const& other) { return BinaryAssignment(otXmmIntOr , other); }
    C_u128x1& operator^=(C_u128x1 const& other) { return BinaryAssignment(otXmmIntXor, other); }

    C_u128x1 operator&(R_u128x1 const& ref) const { return BinaryReference(otXmmIntAnd, ref); }
    C_u128x1 operator|(R_u128x1 const& ref) const { return BinaryReference(otXmmIntOr , ref); }
    C_u128x1 operator^(R_u128x1 const& ref) const { return BinaryReference(otXmmIntXor, ref); }

private:
    C_u128x1 BinaryOperation(OpType ot, C_u128x1 const& other) const;
    C_u128x1 BinaryOperationWithFloat(OpType ot, C_f32x4 const& other) const;
    C_u128x1& BinaryAssignment(OpType ot, C_u128x1 const& other);
    C_u128x1 BinaryReference(OpType ot, R_u128x1 const& ref) const;
};



