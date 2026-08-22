// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains implementation for Clip stack
//

#include "precomp.hpp"

//-----------------------------------------------------------------------------
//
// Member:      CBaseClipStack::Clear
//
// Description: Clear the clip stack.
//
//-----------------------------------------------------------------------------

VOID
CBaseClipStack::Clear()
{
    m_clipStack.Clear();
}


//-----------------------------------------------------------------------------
//
// Member:      CBaseClipStack::Push
//
// Description: Intersect the clip with the previous clipping and set it to the
//              top of the stack.
//
//           stack empty:
//              [] => [*prcClip] 
//
//           stack non-empty:
//              [rcTopClip | <rest of stack>] 
//                    ==> [*prcClip & rcTopClip | rcTopClip | <rest of stack>]
//-----------------------------------------------------------------------------

HRESULT 
CBaseClipStack::Push(
    __in_ecount(1) const MilRectF &rcClip
    )
{
    HRESULT hr = S_OK;

    CMilRectF rcPrevClip;

    // Get the previous clip from the stack
    Top(&rcPrevClip);

    rcPrevClip.Intersect(rcClip);

    IFC(m_clipStack.Push(rcPrevClip));

Cleanup:
    RRETURN(hr);
}


//-----------------------------------------------------------------------------
//
// Member:      CBaseClipStack::PushExact
//
// Description: Push an exact clip on the top of the stack.  No intesection.
//
//              [<current stack>] 
//                    ==> [*prcClip | <current stack>]
//-----------------------------------------------------------------------------

HRESULT 
CBaseClipStack::PushExact(
    __in_ecount(1) const MilRectF &rcClip
    )
{
    RRETURN(THR(m_clipStack.Push(rcClip)));
}


//-----------------------------------------------------------------------------
//
// Member:      CBaseClipStack::Pop
//
// Description: Pops the clip at the top off the stack. The function assumes
//              that the stack is not empty.
//
//              [rcTopClip | <rest of stack>] -> [ <rest of stack> ]
//-----------------------------------------------------------------------------

VOID
CBaseClipStack::Pop()
{
    Verify(m_clipStack.Pop(NULL));
}


//-----------------------------------------------------------------------------
//
// Member:      CBaseClipStack::Top
//
// Description: Returns the clip at the top of the stack.  When the stack is
//              Empty an unbounded (infinite) clip is returned.
//
//-----------------------------------------------------------------------------

void 
CBaseClipStack::Top(
    __out_ecount(1) CMilRectF *prcClip
    )
{
    if (m_clipStack.IsEmpty())
    {
        *prcClip = prcClip->sc_rcInfinite;
    }
    else
    {
        Verify(SUCCEEDED(m_clipStack.Top(prcClip)));
    }
}





