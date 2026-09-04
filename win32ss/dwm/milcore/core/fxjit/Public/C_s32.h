// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent signed integer 32-bit variable.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_s32
//
//  Synopsis:
//      Represents signed integer 32-bit variable in prototype program.
//
//------------------------------------------------------------------------------
class C_s32 : public C_u32
{
public:
    C_s32 operator/(C_s32 const& other) const { return BinaryOperation(otINT32Div, other).Signed(); }
    C_s32 operator%(C_s32 const& other) const { return BinaryOperation(otINT32Rem, other).Signed(); }

    C_s32& operator/=(C_s32 const& other) { return BinaryAssignment(otINT32Div, other).Signed(); }
    C_s32& operator%=(C_s32 const& other) { return BinaryAssignment(otINT32Rem, other).Signed(); }

    C_s32 operator>>(C_u32 const& other) const { return BinaryOperation(otINT32ShiftRight, other).Signed(); }
    C_s32& operator>>=(C_u32 const& other) { return BinaryAssignment(otINT32ShiftRight, other).Signed(); }
};

