// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//      Declaratoion of "Energized" CMilPointAndSize3F class
//
//------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:       CMilPointAndSize3F, MilPointAndSize3F
//
//  Synopsis:    An "energized" version of MilPointAndSize3F, which adds members and
//               operators.
//
//               Designed to have the same memory layout as MilPointAndSize3F, so that
//               you can cast between them.
//
//------------------------------------------------------------------------------

#include "factory.hpp"


class CMilPointAndSize3F : public MilPointAndSize3F
{
public:

    // Constructors
    CMilPointAndSize3F()
    {
        // We require that you can typecast between MilPointAndSize3F and CMilPointAndSize3F.
        // To achieve this, CMilPointAndSize3F must have no data members or virtual functions.

        Assert( sizeof(MilPointAndSize3F) == sizeof(CMilPointAndSize3F) );
    }

    CMilPointAndSize3F(__in_ecount(1) const dxlayer::vector3 &vecMin, __in_ecount(1) const dxlayer::vector3 &vecMax)
    {
        Assert( sizeof(MilPointAndSize3F) == sizeof(CMilPointAndSize3F) );
        
        X = vecMin.x;
        LengthX = vecMax.x - vecMin.x;

        Y = vecMin.y;
        LengthY = vecMax.y - vecMin.y;

        Z = vecMin.z;
        LengthZ = vecMax.z - vecMin.z;
    }

    void
    ToVector4Array(
        __out_ecount(1) dxlayer::vector4 (& rvec4PointsOut)[8]
        ) const;

    // This is the list of 12 edges between the 8 points returned by
    // ToVector4Array.
    //
    static const DWORD sc_rdwEdgeList[12][2];

    static const CMilPointAndSize3F sc_boxEmpty;  
};

