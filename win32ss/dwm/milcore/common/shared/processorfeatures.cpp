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

#include "precomp.hpp"

// We are not concerned of architecture other than X86.

bool CCPUInfo::m_fHasMMX       = false;
bool CCPUInfo::m_fHasSSE       = false;
bool CCPUInfo::m_fHasSSE2      = false;
bool CCPUInfo::m_fHasCMPXCHG8B = false;
bool CCPUInfo::m_fHasSSE2ForEffects = false;

#if DBG
bool CCPUInfo::m_fDbgIsInitialized = false;
#endif

//+----------------------------------------------------------------------------
//
//  Member:
//      CCPUInfo::Initialize();
//
//  Synopsis:
//      Build up the appropriate static data representing the processor features
//      detected on this CPU.
//
//-----------------------------------------------------------------------------


void CCPUInfo::Initialize()
{
    //
    // Future Consideration:
    // Consider excluding class CCPUInfo for architectures other than X86.
    // This will require revising all the instances of CCPUInfo:Has*() usage.
    // 
#if defined(_X86_)
    m_fHasMMX       = !!IsProcessorFeaturePresent(PF_MMX_INSTRUCTIONS_AVAILABLE);
    m_fHasSSE       = !!IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE);
    m_fHasSSE2      = !!IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE);
    m_fHasCMPXCHG8B = !!IsProcessorFeaturePresent(PF_COMPARE_EXCHANGE_DOUBLE);
    m_fHasSSE2ForEffects = m_fHasSSE2;
#endif //_X86_

#if defined(_AMD64_)
    m_fHasSSE2ForEffects = true;
#endif

#if DBG
    m_fDbgIsInitialized = true;
#endif
}



