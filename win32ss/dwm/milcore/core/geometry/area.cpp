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
//  $Notes:
//      By Green's theorem the area of the region bounded by a simple path
//      (x(t), y(t)) is, up to a sign, 1/2 * integral (x*y' - y*x')dt.  The sign
//      depends on the path orientation.  If the path is not simple, i.e. if it
//      intersects itself, then this formula is incorrect because different
//      parts of the path may cancel each other as they contribute with
//      different signs.  The result for a figure 8 for example will be 0.  So
//      here we rely on the scanning to identify the correct sign of the
//      contribution of every piece of the boundary by looking at its
//      classification as left, right or redundant.  Our boundary is flattened,
//      so we are dealing with linear edges.  The edge emanting from (x,y) with
//      a vector (u, v) can be parameterized as {(x + tu, y + tv) : a < t < b},
//      and then the contribution of this edge is the integral from a to b of
//      x*v - y*u dt = (x*v - y*u) * (b - a).  This is added with the
//      appropriate sign, depending on whether we are on a left or right edge,
//      or ignored altogether if the edge is redundant.
//
//  $ENDTAG
//
//  Classes:
//      CArea.
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

//+-----------------------------------------------------------------------------
//
//  Member:
//      CArea::ProcessCurrentVertex
//
//  Synopsis:
//      Process the current vertex - CScanner method override
//
//------------------------------------------------------------------------------
HRESULT
CArea::ProcessCurrentVertex(
    __in_ecount(1) CChain *pChain)
        // The chain whose current vertex we're processing
{
    Assert(pChain);
    Assert(pChain->GetPreviousVertex());   // Otherwise we should not be called

    // Get the contribution of the edge that TERMINATES at the current verex
    UpdateWithEdge(*pChain, *pChain->GetPreviousVertex());
    
    return S_OK;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CArea::ProcessTheJunction
//
//  Synopsis:
//      Process all the tails of this junction - CScanner method override
//
//  Notes:
//      ProcessCurrentVertex is never called on the last (tail) vertex of a
//      chain, so here we get the contribution of the last edge of every chain
//      that terminates at this junction.
//
//------------------------------------------------------------------------------
HRESULT
CArea::ProcessTheJunction()
{

    const CChain *pRightMost = m_oJunction.GetRightmostTail(0);


    for (CChain *pChain = m_oJunction.GetLeftmostTail(0);
         pChain;
         pChain = pChain->GetRight())
    {
        if (!pChain->IsSelfRedundant())
        {
            // ProcessTheJunction is invoked either when some chain has reached its tail or when
            // we activate a new chain.  In either case, the current vertices of the chains that
            // terminate at this junction (= tail-chains) may be either at their tail or at the
            // tail's previous vertex.
            if (pChain->IsAtTail())
            {
                Assert(pChain->GetPreviousVertex());

                UpdateWithEdge(*pChain, *pChain->GetPreviousVertex());
            }
            else
            {
                Assert(pChain->GetCurrentVertex());

                // This is a tail-chain at a junction, and its current vertex is not the last
                // vertex, so it must be the second to last vertex
                Assert(pChain->GetCurrentVertex()->GetNext() == pChain->GetTail());
                
                UpdateWithEdge(*pChain, *pChain->GetCurrentVertex());
            }
        }

        if (pChain == pRightMost)
            break;
    }

    return S_OK;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CArea::UpdateWithEdge
//
//  Synopsis:
//      Add the contribution to the area of a given edge
//
//------------------------------------------------------------------------------
void
CArea::UpdateWithEdge(
    __in_ecount(1) const CChain  &chain,
        // The chain
    __in_ecount(1) const CVertex &vertex)
        // The top vertex of the edge in question 
{
    if (chain.IsSideRight())
    {
        m_rArea -= vertex.GetAreaContribution();
    }
    else
    {
        m_rArea += vertex.GetAreaContribution();
    }
}



