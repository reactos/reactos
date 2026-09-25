// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent pointer to XmmFloat variable.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      R_f32x1
//
//  Synopsis:
//      Represents a reference to variable of type "C_f32x1" in prototype
//      program. Serves as intermediate calculation type for P_f32x1::operator[].
//
//------------------------------------------------------------------------------

class R_f32x1 : public R_void<R_f32x1, C_f32x1>
{
public:
    static const UINT32 IndexShift = 2;
    static const RefType IndexScale = RefType_Index_4;

    R_f32x1(UINT32 uBaseVarID, UINT32 uIndexVarID, UINT_PTR uDisplacement)
        : R_void<R_f32x1, C_f32x1>(uBaseVarID, uIndexVarID, uDisplacement) {}

    operator C_f32x1() const { return Load(otXmmFloat1Load);}
    C_f32x1 const& operator=(C_f32x1 const& origin) const { return Store(origin, otXmmFloat1Store); }

    C_f32x1 operator+(C_f32x1 const& src) const {return ((C_f32x1)(*this)) + src;}
    C_f32x1 operator-(C_f32x1 const& src) const {return ((C_f32x1)(*this)) - src;}
    C_f32x1 operator*(C_f32x1 const& src) const {return ((C_f32x1)(*this)) * src;}
    C_f32x1 operator/(C_f32x1 const& src) const {return ((C_f32x1)(*this)) / src;}

    C_f32x1 operator+(R_f32x1 const& ref) const {return ((C_f32x1)(*this)) + ref;}
    C_f32x1 operator-(R_f32x1 const& ref) const {return ((C_f32x1)(*this)) - ref;}
    C_f32x1 operator*(R_f32x1 const& ref) const {return ((C_f32x1)(*this)) * ref;}
    C_f32x1 operator/(R_f32x1 const& ref) const {return ((C_f32x1)(*this)) / ref;}
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      P_f32x1
//
//  Synopsis:
//      Represents variable of type "float*" assuming it to be handled
//      via SSE registers.
//
//------------------------------------------------------------------------------

class P_f32x1 : public TIndexer<P_f32x1, R_f32x1>
{
public:

    P_f32x1() {}
    P_f32x1(void * pOrigin) : TIndexer<P_f32x1, R_f32x1>(pOrigin) {}
};

