// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+---------------------------------------------------------------------------------
//

//
//  Description:
//
//         A rectangle with a zIndex used by the CCoverageSet to store a rectangle 
//         with Z annotation.
//
//----------------------------------------------------------------------------------

class CZOrderedRect : public CMilRectF
{
public:
    CZOrderedRect() : CMilRectF()
    {
        ZIndex = 0;
    }

    CZOrderedRect(const MilRectF &r, int zIndex) : CMilRectF(r)
    {
        ZIndex = zIndex;
    }

    int ZIndex;
};


