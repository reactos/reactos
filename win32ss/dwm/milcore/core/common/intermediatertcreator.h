// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:        
//      Header file for render target creator class.
//
//-----------------------------------------------------------------------------

#pragma once

//+----------------------------------------------------------------------------
//
//  Struct:     IntermediateRTUsage
//
//  Synopsis:  
//      Struct for holding all the usage parameters needed to determine what
//      kind of interemediate render target to create
//
//-----------------------------------------------------------------------------

struct IntermediateRTUsage
{
    enum FlagsEnum
    {
        ForBlending          = 1,
        ForUseIn3D           = 2
    };
    typedef TMILFlagsEnum<FlagsEnum> Flags;

    Flags flags;
    MilBitmapWrapMode::Enum wrapMode;
};

//+----------------------------------------------------------------------------
//
//  Class:     CIntermediateRTCreator
//
//  Synopsis: 
//
//      Base class for things that know how to make render targets. It is
//      useful to separate this from the internal render target for
//      these reasons:
//      1. The smaller interface is safer to pass to brush realization code
//      2. The creator class can be used in situations where we have no
//         internal render target (for example, in the software rasterizer)
//      3. The logic for determining whether this object was used to create a
//         hardware render target can be consolidated.
//      4. This class and its subclasses can be given the context of the
//         drawing operation in order to make decisions about such things as
//         whether to create a hardware or software render target. (Currently
//         this is not done yet...)
//
//  Usage:
//
//      This class can be used like this:
//      1. Instantiate this object
//      2. Call ResetUsedState()
//      3. Pass to some code that might call CreateRenderTargetBitmap()
//           a) The CreateRenderTargetBitmap call might call
//           SetUsedToCreateHardwareRT
//      4. Use WasUsedToCreateHardwareRT to see if a hardware render target was
//         created
//
//-----------------------------------------------------------------------------


class CIntermediateRTCreator
{
public:
    CIntermediateRTCreator()
    {
        m_fUsedToCreateHardwareRT = false;
    }

    // Create a temporary offscreen render target that is expected to
    // be used later with this render target as a source bitmap.

    STDMETHOD(CreateRenderTargetBitmap)(
        UINT width,
        UINT height,
        IntermediateRTUsage usageInfo,
        MilRTInitialization::Flags dwFlags,
        __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap,
        __in_opt DynArray<bool> const *pActiveDisplays = NULL
        ) PURE;

    void ResetUsedState() { m_fUsedToCreateHardwareRT = false; }
    bool WasUsedToCreateHardwareRT() const { return m_fUsedToCreateHardwareRT; }

    STDMETHOD(ReadEnabledDisplays) (
        __inout DynArray<bool> *pEnabledDisplays
        ) PURE;
    
protected:

    void SetUsedToCreateHardwareRT() { m_fUsedToCreateHardwareRT = true; }

private:
    bool m_fUsedToCreateHardwareRT;
};


//+----------------------------------------------------------------------------
//
//  Class:     CNullIntermediateRTCreator
//
//  Synopsis:  This can be used in place of a real CIntermediateRTCreator when it
//             is known that the render target creator will never be used.
//
//-----------------------------------------------------------------------------

class CNullIntermediateRTCreator : public CIntermediateRTCreator
{
public:
    STDMETHOD(CreateRenderTargetBitmap)(
        UINT width,
        UINT height,
        IntermediateRTUsage usageInfo,
        MilRTInitialization::Flags dwFlags,
        __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap,
        __in_opt DynArray<bool> const *pActiveDisplays = NULL
        ) override
    {
        UNREFERENCED_PARAMETER(width);
        UNREFERENCED_PARAMETER(height);
        UNREFERENCED_PARAMETER(usageInfo);
        UNREFERENCED_PARAMETER(dwFlags);
        UNREFERENCED_PARAMETER(ppIRenderTargetBitmap);
        UNREFERENCED_PARAMETER(pActiveDisplays);
    
        RIPW(L"CNullIntermediateRTCreator mistakenly called to create an intermediate");
        RRETURN(WGXERR_INVALIDCALL);
    }
};

