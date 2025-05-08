// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//      Implementation of "Energized" CMilPointAndSize3F class
//
 
#include "precomp.hpp"
using namespace dxlayer;

//+------------------------------------------------------------------------
//
//  Function:  ToVector4Array
//
//  Synopsis:  Returns 8 points (as vector4_t) that represent the eight
//             corners of the cube suitable for transforming and/or
//             clipping.
//
//             The vertices we created are laid out in the following order:
//    
//                  7---6
//                 /|  /|    
//                3-+-2 |    
//                | | | |    
//                | 4-+-5    
//                |/  |/     
//                0---1      
//
//-------------------------------------------------------------------------

void
CMilPointAndSize3F::ToVector4Array(
    __out_ecount(1) vector4 (& rvec4PointsOut)[8]
    ) const
{
    // Lower Left
    rvec4PointsOut[0].x = X;
    rvec4PointsOut[0].y = Y;
    rvec4PointsOut[0].z = Z;
    rvec4PointsOut[0].w = 1;

    // Lower Right
    rvec4PointsOut[1].x = X + LengthX;
    rvec4PointsOut[1].y = Y;
    rvec4PointsOut[1].z = Z;
    rvec4PointsOut[1].w = 1;

    // Upper Right
    rvec4PointsOut[2].x = X + LengthX;
    rvec4PointsOut[2].y = Y + LengthY;
    rvec4PointsOut[2].z = Z;
    rvec4PointsOut[2].w = 1;

    // Upper Left
    rvec4PointsOut[3].x = X;
    rvec4PointsOut[3].y = Y + LengthY;
    rvec4PointsOut[3].z = Z;
    rvec4PointsOut[3].w = 1;

    // Copy the 4 corners shifted by the length of z for the other half
    // of the cube.
    for (int i = 4; i < 8; i++)
    {
        rvec4PointsOut[i].x = rvec4PointsOut[i-4].x;
        rvec4PointsOut[i].y = rvec4PointsOut[i-4].y;
        rvec4PointsOut[i].z = rvec4PointsOut[i-4].z + LengthZ;
        rvec4PointsOut[i].w = 1;
    }
}

const DWORD CMilPointAndSize3F::sc_rdwEdgeList[12][2] =
{
    // Front face
    {0, 1},
    {1, 2},
    {2, 3},
    {3, 0},

    // Right face
    {1, 5},
    {5, 6},
    {6, 2},

    // Back face
    {6, 7},
    {7, 4},
    {4, 5},

    // Left face
    {7, 3},
    {0, 4}

    // All four edges in Top and Bottom are shared, one each
    // with Front, Right, Back, and Left.
};

const CMilPointAndSize3F CMilPointAndSize3F::sc_boxEmpty = CMilPointAndSize3F(vector3(0.0f,0.0f,0.0f), vector3(0.0f,0.0f,0.0f));



