// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      This module is used to query various processor features,
//      e.g. MMX, XMMI (SSE), etc.
//
//-----------------------------------------------------------------------------

#pragma once

class CCPUInfo
{
public:
    static void Initialize();

    static bool HasMMX()
    {
        AssertIsInitialized();
        return m_fHasMMX;
    }

    static bool HasSSE()
    {
        AssertIsInitialized();
        return m_fHasSSE;
    }

    static bool HasSSE2()
    {
        AssertIsInitialized();
        return m_fHasSSE2;
    }
    
    static bool HasCompareExchangeDouble()
    {
        AssertIsInitialized();
        return m_fHasCMPXCHG8B;
    }

    //  Refactor this class to make it clearer that the
    //            other flags are only valid for X86 builds.  The flag below
    //            is valid for both 32- and 64-bit builds.
    static bool HasSSE2ForEffects()
    {
        AssertIsInitialized();
        return m_fHasSSE2ForEffects;
    }
    
    static void AssertIsInitialized()
    {
#if DBG
        Assert(m_fDbgIsInitialized);
#endif
    }


private:
    static bool m_fHasMMX;  // supports MMX
    static bool m_fHasSSE;  // supports SSE instructions (Pentium 3+)
    static bool m_fHasSSE2; // supports SSE2 instructions (Pentium 4+)
    static bool m_fHasCMPXCHG8B; // supports cmpxchg8b instruction
    static bool m_fHasSSE2ForEffects; // supports SSE2 (both X86 and AMD64)

#if DBG
    static bool m_fDbgIsInitialized;
#endif //DBG
};


