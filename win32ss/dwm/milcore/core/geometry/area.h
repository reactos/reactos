// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_geometry
//      $Keywords:
//
//  $Description:
//      Compute the area of a shape
//
//  $ENDTAG
//
//  Classes:
//      CArea
//
//------------------------------------------------------------------------------

class CArea  :   public CScanner
{
public:
    // Constructor/destructor
    CArea(double rTolerance)
        : CScanner(rTolerance)
    {
        m_rArea = 0;
    }

    virtual ~CArea()
    {
    }

    // CScanner override
    virtual HRESULT ProcessTheJunction();

    virtual HRESULT ProcessCurrentVertex(
        __in_ecount(1) CChain *pChain);
            // The chain whose current vertex we're processing

    // Get the result
    double GetResult() const
    {
        if (m_rArea < 0)
            return 0;
        else
            // The computations were done on a scaled copy of the geometry.  Multiplying 
            // the area by the squared inverse of the scale factor will correct that.
            // The factor .5 is explained at the header of area.cpp.
            return m_rArea * m_rInverseScale * m_rInverseScale * .5;
    }

protected:
    void UpdateWithEdge(
        __in_ecount(1) const CChain  &chain,
            // The chain
        __in_ecount(1) const CVertex &vertex);
            // The top vertex of the edge in question

    // Data
    double  m_rArea;       // The area
};

