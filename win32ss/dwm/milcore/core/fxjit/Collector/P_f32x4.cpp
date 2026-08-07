// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent pointer to C_f32x4 variable.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::IntFloor
//
//  Synopsis:
//      Perform per-component conversion floating point values to closest
//      integers that are less than or equal to given.
//
//------------------------------------------------------------------------------
C_u32x4
R_f32x4::IntFloor() const
{
    return ((C_f32x4)(*this)).IntFloor();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::IntCeil
//
//  Synopsis:
//      Perform per-component conversion floating point values to closest
//      integers that are greater than or equal to given.
//
//------------------------------------------------------------------------------
C_u32x4
R_f32x4::IntCeil() const
{
    return ((C_f32x4)(*this)).IntCeil();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::IntNear
//
//  Synopsis:
//      Perform per-component conversion floating point values to closest
//      integers. Half-integer values are rounded up.
//
//------------------------------------------------------------------------------
C_u32x4
R_f32x4::IntNear() const
{
    return ((C_f32x4)(*this)).IntNear();
}


