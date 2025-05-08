// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent pointer to u32x4 variable.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      R_u32x4
//
//  Synopsis:
//      Represents a reference to variable of type "C_u32x4" in prototype
//      program. Serves as intermediate calculation type for P_u32x4::operator[].
//
//------------------------------------------------------------------------------
class R_u32x4 : public R_void<R_u32x4, C_u32x4>
{
public:
    static const UINT32 IndexShift = 4;

    R_u32x4(UINT32 uBaseVarID, UINT32 uIndexVarID, UINT_PTR uDisplacement)
        : R_void<R_u32x4, C_u32x4>(uBaseVarID, uIndexVarID, uDisplacement) {}

    operator C_u32x4() const { return Load(otXmmIntLoad);}
    C_u32x4 const& operator=(C_u32x4 const& origin) const { return Store(origin, otXmmIntStore); }

    C_u32x4 operator+(C_u32x4 const& src) const {return ((C_u32x4)(*this)) + src;}
    C_u32x4 operator-(C_u32x4 const& src) const {return ((C_u32x4)(*this)) - src;}
    C_u32x4 operator&(C_u32x4 const& src) const {return ((C_u32x4)(*this)) & src;}
    C_u32x4 operator|(C_u32x4 const& src) const {return ((C_u32x4)(*this)) | src;}
    C_u32x4 operator^(C_u32x4 const& src) const {return ((C_u32x4)(*this)) ^ src;}
    C_u32x4 operator*(C_u32x4 const& src) const {return ((C_u32x4)(*this)) * src;}
    C_u32x4 Min(C_u32x4 const& src) const {return ((C_u32x4)(*this)).Min(src);}
    C_u32x4 Max(C_u32x4 const& src) const {return ((C_u32x4)(*this)).Max(src);}

    C_u32x4 operator+(R_u32x4 const& ref) const {return ((C_u32x4)(*this)) + ref;}
    C_u32x4 operator-(R_u32x4 const& ref) const {return ((C_u32x4)(*this)) - ref;}
    C_u32x4 operator&(R_u32x4 const& ref) const {return ((C_u32x4)(*this)) & ref;}
    C_u32x4 operator|(R_u32x4 const& ref) const {return ((C_u32x4)(*this)) | ref;}
    C_u32x4 operator^(R_u32x4 const& ref) const {return ((C_u32x4)(*this)) ^ ref;}
    C_u32x4 operator*(R_u32x4 const& ref) const {return ((C_u32x4)(*this)) * ref;}
    C_u32x4 Min(R_u32x4 const& ref) const {return ((C_u32x4)(*this)).Min(ref);}
    C_u32x4 Max(R_u32x4 const& ref) const {return ((C_u32x4)(*this)).Max(ref);}
};


//+-----------------------------------------------------------------------------
//
//  Class:
//      P_u32x4
//
//  Synopsis:
//      Represents variable of type "u32x4*" in prototype program.
//
//------------------------------------------------------------------------------
class P_u32x4 : public TIndexer<P_u32x4, R_u32x4>
{
public:
    P_u32x4() {}
    P_u32x4(void * pOrigin) : TIndexer<P_u32x4, R_u32x4>(pOrigin) {}
};

