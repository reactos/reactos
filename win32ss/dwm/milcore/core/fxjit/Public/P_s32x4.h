// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent pointer to s32x4 variable.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      R_s32x4
//
//  Synopsis:
//      Represents a reference to variable of type "C_s32x4" in prototype
//      program. Serves as intermediate calculation type for P_s32x4::operator[].
//
//------------------------------------------------------------------------------
class R_s32x4 : public R_void<R_s32x4, C_s32x4>
{
public:
    static const UINT32 IndexShift = 4;

    R_s32x4(UINT32 uBaseVarID, UINT32 uIndexVarID, UINT_PTR uDisplacement)
        : R_void<R_s32x4, C_s32x4>(uBaseVarID, uIndexVarID, uDisplacement) {}

    operator C_s32x4() const { return Load(otXmmIntLoad);}
    C_s32x4 const& operator=(C_s32x4 const& origin) const { return Store(origin, otXmmIntStore); }

    C_s32x4 operator>(C_s32x4 const& src) const {return ((C_s32x4)(*this)) > src;}
    C_s32x4 Min(C_s32x4 const& src) const {return ((C_s32x4)(*this)).Min(src);}
    C_s32x4 Max(C_s32x4 const& src) const {return ((C_s32x4)(*this)).Max(src);}

    C_s32x4 operator>(R_s32x4 const& ref) const {return ((C_s32x4)(*this)) > ref;}
    C_s32x4 Min(R_s32x4 const& ref) const {return ((C_s32x4)(*this)).Min(ref);}
    C_s32x4 Max(R_s32x4 const& ref) const {return ((C_s32x4)(*this)).Max(ref);}
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      P_s32x4
//
//  Synopsis:
//      Represents variable of type "s32x4*" in prototype program.
//
//------------------------------------------------------------------------------
class P_s32x4 : public TIndexer<P_s32x4, R_s32x4>
{
public:
    P_s32x4() {}
    P_s32x4(void * pOrigin) : TIndexer<P_s32x4, R_s32x4>(pOrigin) {}
};

