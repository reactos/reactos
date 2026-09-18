// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent pointer to unsigned
//      integer 16-bit variable.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      R_u16
//
//  Purpose:
//      Represents a reference to 16-bit value in memory available
//      for reading and writing via C_u32 instance.
//
//------------------------------------------------------------------------------

class R_u16 : public R_void<R_u16, C_u32>
{
public:
    static const UINT32 IndexShift = 1;
    static const RefType IndexScale = RefType_Index_2;

    R_u16(UINT32 uBaseVarID, UINT32 uIndexVarID, UINT_PTR uDisplacement)
        : R_void<R_u16, C_u32>(uBaseVarID, uIndexVarID, uDisplacement) {}

    operator C_u32() const { return Load(otUINT32LoadWord);}
    //C_u32 const& operator=(C_u32 const& origin) { return Store(origin, ot...TODO); }
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      P_u16
//
//  Synopsis:
//      Represents variable of type "UINT16*" in prototype program.
//
//------------------------------------------------------------------------------
class P_u16 : public TIndexer<P_u16, R_u16>
{
public:
    P_u16() {}
    P_u16(void * pOrigin) : TIndexer<P_u16, R_u16>(pOrigin) {}
};


