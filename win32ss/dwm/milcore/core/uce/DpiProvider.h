// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains DpiProvider and IDpiProvider declaration
//
//-----------------------------------------------------------------------------

#pragma once 
#include "shared\DpiUtil.h"
#include "shared\DpiScale.h"
#include "shared\DelegatingIUnknown.h"

#include <combaseapi.h>
#include <Unknwn.h>
#include <windef.h>
#include <ShellScalingApi.h>

#if !defined(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 (DPI_AWARENESS_CONTEXT)-4
#endif

DECLARE_DELEGATING_INTERFACE(IDpiProvider, "AB9362AC-E5EF-43DB-9D4A-556283341DC8")
{
    BEGIN_INTERFACE

    // IDpiProvider methods
    STDMETHOD_(const DPI_AWARENESS_CONTEXT, GetDpiAwarenessContext)(THIS) const PURE;
    STDMETHOD(GetCurrentDpi)(THIS_ DpiScale* pDpiScale) const PURE;
    STDMETHOD_(BOOL, IsPerMonitorDpiAware)(THIS) const PURE;

    END_INTERFACE
};

DEFINE_DELEGATING_INTERFACE(IDpiProvider, DpiProvider)
{
    private:
        std::shared_ptr<wpf::util::DpiAwarenessContext> m_pDpiAwarenessContext = nullptr;
        DpiScale m_dpi; 
    
    public:
    
        inline DpiProvider(IUnknown* pControllingUnk, const DpiScale& dpi) : 
            base(pControllingUnk),
            m_dpi(dpi)
        {}
    
        inline DpiProvider(IUnknown* pControllingUnk) : 
            base(pControllingUnk)
        {}
    
        // DpiProvider
        STDMETHOD_(const DPI_AWARENESS_CONTEXT, GetDpiAwarenessContext)(THIS) const override;
        STDMETHOD(GetCurrentDpi)(THIS_ DpiScale* pDpiScale) const override;
        STDMETHOD_(BOOL, IsPerMonitorDpiAware)(THIS) const override;
        
    protected:
        void UpdateDpi(const DpiScale& dpi);
        void SetDpiAwarenessContext(int context);
};

