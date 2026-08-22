// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent pointer to unsigned
//      integer 8-bit variable.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      R_u8
//
//  Purpose:
//      Represents a reference to 8-bit value in memory available
//      for reading and writing via C_u32 instance.
//
//------------------------------------------------------------------------------

class R_u8 : public R_void<R_u8, C_u32>
{
public:
    static const UINT32 IndexShift = 0;
    static const RefType IndexScale = RefType_Index_1;

    R_u8(UINT32 uBaseVarID, UINT32 uIndexVarID, UINT_PTR uDisplacement)
        : R_void<R_u8, C_u32>(uBaseVarID, uIndexVarID, uDisplacement) {}

    operator C_u32() const { return Load(otUINT32LoadByte);}
    //C_u32 const& operator=(C_u32 const& origin) { return Store(origin, ot...TODO); }
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      P_u8
//
//  Synopsis:
//      Represents variable of type "UINT8*" in prototype program.
//
//------------------------------------------------------------------------------
class P_u8 : public TIndexer<P_u8, R_u8>
{
public:
    P_u8() {}
    P_u8(void * pOrigin) : TIndexer<P_u8, R_u8>(pOrigin) {}
};


