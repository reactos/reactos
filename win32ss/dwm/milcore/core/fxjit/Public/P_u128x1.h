// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent pointer to 128-bit XMM variable.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      R_u128x1
//
//  Synopsis:
//      Represents a reference to variable of type "C_u128x1" in prototype
//      program. Serves as intermediate calculation type for P_u128x1::operator[].
//
//------------------------------------------------------------------------------

class R_u128x1 : public R_void<R_u128x1, C_u128x1>
{
public:
    static const UINT32 IndexShift = 4;

    R_u128x1(UINT32 uBaseVarID, UINT32 uIndexVarID, UINT_PTR uDisplacement)
        : R_void<R_u128x1, C_u128x1>(uBaseVarID, uIndexVarID, uDisplacement) {}

    operator C_u128x1() const { return Load(otXmmIntLoad);}
    C_u128x1 const& operator=(C_u128x1 const& origin) const { return Store(origin, otXmmIntStore); }

    C_u128x1 operator&(C_u128x1 const& src) const {return ((C_u128x1)(*this)) & src;}
    C_u128x1 operator|(C_u128x1 const& src) const {return ((C_u128x1)(*this)) | src;}
    C_u128x1 operator^(C_u128x1 const& src) const {return ((C_u128x1)(*this)) ^ src;}

    C_u128x1 operator&(R_u128x1 const& ref) const {return ((C_u128x1)(*this)) & ref;}
    C_u128x1 operator|(R_u128x1 const& ref) const {return ((C_u128x1)(*this)) | ref;}
    C_u128x1 operator^(R_u128x1 const& ref) const {return ((C_u128x1)(*this)) ^ ref;}
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      P_u128x1
//
//  Synopsis:
//      Represents a pointer to integer 128-bits value in prototype program.
//
//------------------------------------------------------------------------------
class P_u128x1 : public TIndexer<P_u128x1, R_u128x1>
{
public:
    P_u128x1() {}
    P_u128x1(void * pOrigin) : TIndexer<P_u128x1, R_u128x1>(pOrigin) {}
};


