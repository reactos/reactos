// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+---------------------------------------------------------------------------
//

//
//  Description:
//      Declares aliased clip class
//
//----------------------------------------------------------------------------

#pragma once

class CAliasedClip
{
public:

    CAliasedClip(__in_ecount_opt(1) const MilRectF *prcAliasedClip)
    {
        if (prcAliasedClip == NULL)
        {
            m_fIsNullClip = TRUE;
        }
        else
        {
            m_fIsNullClip = FALSE;
            m_rcAliasedClip = *prcAliasedClip;
        }
    }

    BOOL IsEmptyClip() const { return !m_fIsNullClip && m_rcAliasedClip.IsEmpty(); }

    BOOL IsNullClip() const { return m_fIsNullClip; }

    void GetAsCMilRectF(
        __out_ecount(1) CMilRectF *prcRectF
        ) const
    {
        Assert(!m_fIsNullClip);
        *prcRectF = m_rcAliasedClip;
    }

private:
    BOOL m_fIsNullClip;
    CMilRectF m_rcAliasedClip;
};


