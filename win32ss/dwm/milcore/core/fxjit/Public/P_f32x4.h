// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent pointer to f32x4 variable.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      R_f32x4
//
//  Synopsis:
//      Represents a reference to variable of type "C_f32x4" in prototype
//      program. Serves as intermediate calculation type for P_f32x4::operator[].
//
//------------------------------------------------------------------------------

class R_f32x4 : public R_void<R_f32x4, C_f32x4>
{
public:
    static const UINT32 IndexShift = 4;

    R_f32x4(UINT32 uBaseVarID, UINT32 uIndexVarID, UINT_PTR uDisplacement)
        : R_void<R_f32x4, C_f32x4>(uBaseVarID, uIndexVarID, uDisplacement) {}

    operator C_f32x4() const { return Load(otXmmFloat4Load);}
    C_f32x4 const& operator=(C_f32x4 const& origin) const { return Store(origin, otXmmFloat4Store); }

    C_f32x4 operator+(C_f32x4 const& src) const {return ((C_f32x4)(*this)) + src;}
    C_f32x4 operator-(C_f32x4 const& src) const {return ((C_f32x4)(*this)) - src;}
    C_f32x4 operator*(C_f32x4 const& src) const {return ((C_f32x4)(*this)) * src;}
    C_f32x4 operator/(C_f32x4 const& src) const {return ((C_f32x4)(*this)) / src;}
    C_f32x4 operator&(C_f32x4 const& src) const {return ((C_f32x4)(*this)) & src;}
    C_f32x4 operator|(C_f32x4 const& src) const {return ((C_f32x4)(*this)) | src;}
    C_f32x4 operator^(C_f32x4 const& src) const {return ((C_f32x4)(*this)) ^ src;}
    C_f32x4       Min(C_f32x4 const& src) const {return ((C_f32x4)(*this)).Min(src);}
    C_f32x4       Max(C_f32x4 const& src) const {return ((C_f32x4)(*this)).Max(src);}
    C_f32x4 OrderedMin(C_f32x4 const& src) const {return ((C_f32x4)(*this)).OrderedMin(src);}
    C_f32x4 OrderedMax(C_f32x4 const& src) const {return ((C_f32x4)(*this)).OrderedMax(src);}

    C_f32x4 operator==(C_f32x4 const& src) const {return ((C_f32x4)(*this)) == src;}
    C_f32x4 operator< (C_f32x4 const& src) const {return ((C_f32x4)(*this)) <  src;}
    C_f32x4 operator<=(C_f32x4 const& src) const {return ((C_f32x4)(*this)) <= src;}
    C_f32x4 operator!=(C_f32x4 const& src) const {return ((C_f32x4)(*this)) != src;}
    C_f32x4 operator>=(C_f32x4 const& src) const {return ((C_f32x4)(*this)) >= src;}
    C_f32x4 operator> (C_f32x4 const& src) const {return ((C_f32x4)(*this)) >  src;}

    C_f32x4 operator+(R_f32x4 const& ref) const {return ((C_f32x4)(*this)) + ref;}
    C_f32x4 operator-(R_f32x4 const& ref) const {return ((C_f32x4)(*this)) - ref;}
    C_f32x4 operator*(R_f32x4 const& ref) const {return ((C_f32x4)(*this)) * ref;}
    C_f32x4 operator/(R_f32x4 const& ref) const {return ((C_f32x4)(*this)) / ref;}
    C_f32x4 operator&(R_f32x4 const& ref) const {return ((C_f32x4)(*this)) & ref;}
    C_f32x4 operator|(R_f32x4 const& ref) const {return ((C_f32x4)(*this)) | ref;}
    C_f32x4 operator^(R_f32x4 const& ref) const {return ((C_f32x4)(*this)) ^ ref;}
    C_f32x4       Min(R_f32x4 const& ref) const {return ((C_f32x4)(*this)).Min(ref);}
    C_f32x4       Max(R_f32x4 const& ref) const {return ((C_f32x4)(*this)).Max(ref);}
    C_f32x4 OrderedMin(R_f32x4 const& ref) const {return ((C_f32x4)(*this)).OrderedMin(ref);}
    C_f32x4 OrderedMax(R_f32x4 const& ref) const {return ((C_f32x4)(*this)).OrderedMax(ref);}

    C_f32x4 operator==(R_f32x4 const& ref) const {return ((C_f32x4)(*this)) == ref;}
    C_f32x4 operator< (R_f32x4 const& ref) const {return ((C_f32x4)(*this)) <  ref;}
    C_f32x4 operator<=(R_f32x4 const& ref) const {return ((C_f32x4)(*this)) <= ref;}
    C_f32x4 operator!=(R_f32x4 const& ref) const {return ((C_f32x4)(*this)) != ref;}
    C_f32x4 operator>=(R_f32x4 const& ref) const {return ((C_f32x4)(*this)) >= ref;}
    C_f32x4 operator> (R_f32x4 const& ref) const {return ((C_f32x4)(*this)) >  ref;}

    C_f32x4 Reciprocal() const { return UnaryOperation(otXmmFloat4Reciprocal); }
    C_f32x4 Sqrt      () const { return UnaryOperation(otXmmFloat4Sqrt      ); }
    C_f32x4 Rsqrt     () const { return UnaryOperation(otXmmFloat4Rsqrt     ); }
    C_u32x4 ToInt32x4 () const { return CrossOperation(otXmmFloat4ToInt32x4 ); }

    C_u32 ExtractSignBits() const {return C_f32x4(*this).ExtractSignBits();}

    // complex operations
    C_u32x4 IntFloor() const;
    C_u32x4 IntCeil() const;
    C_u32x4 IntNear() const;
    C_u32x4 Truncate() const {return ((C_f32x4)(*this)).Truncate();}
    C_f32x4 Fabs() const {return ((C_f32x4)(*this)).Fabs();}

private:
    friend class C_f32x4;
    friend class P_f32x4;

    C_f32x4 UnaryOperation(OpType ot) const;
    C_u32x4 CrossOperation(OpType ot) const;
};

inline C_f32x4 C_f32x4::operator>=(R_f32x4 const& ref) const { return ref <= *this; }
inline C_f32x4 C_f32x4::operator> (R_f32x4 const& ref) const { return ref <  *this; }

//+-----------------------------------------------------------------------------
//
//  Class:
//      P_f32x4
//
//  Synopsis:
//      Represents variable of type "f32x4*" in prototype program.
//
//------------------------------------------------------------------------------
class P_f32x4 : public TIndexer<P_f32x4, R_f32x4>
{
public:
    P_f32x4() {}
    P_f32x4(void * pOrigin) : TIndexer<P_f32x4, R_f32x4>(pOrigin) {}
};


