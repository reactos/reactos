// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*=========================================================================*\



    Module Name: MIL

\*=========================================================================*/

#pragma once


/*=========================================================================*\
    IMILRefCount - Base ref counting interface for MIL

        Note that this interface does not derive from IUnknown.
        
\*=========================================================================*/

#undef INTERFACE
#define INTERFACE IMILRefCount

DECLARE_INTERFACE(IMILRefCount)
{
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
};

/*=========================================================================*\
    CMILRefCountBase - Base ref counting object for MIL
     
        Note that this object sets m_cRefs==0 on construction and does 
        not derive from IUnknown.
        
\*=========================================================================*/

//
// [pfx_parse] - workaround for PREfix parse breaks
//
#if (defined(_PREFIX_)||defined(_PREFAST_)) && (!(defined(override)))
#define override
#endif

class CMILRefCountBase : public IMILRefCount
{
public:
    CMILRefCountBase();
    virtual ~CMILRefCountBase();
    
    // 
    // Ref count methods
    //

    STDMETHOD_(ULONG, AddRef)(void) override;
    STDMETHOD_(ULONG, Release)(void) override;

protected:
    ULONG volatile m_cRef;
};

/*=========================================================================*\

    DEFINE_REF_COUNT_BASE:

    Include this macro in the public methods list for all class multiply
    inheriting IMILRefCountBase and/or CMILRefCountBase

\*=========================================================================*/

#define DEFINE_REF_COUNT_BASE                                               \
    STDMETHOD_(ULONG, AddRef)(void) override {return CMILRefCountBase::AddRef();}    \
    STDMETHOD_(ULONG,Release)(void) override {return CMILRefCountBase::Release();}   \




