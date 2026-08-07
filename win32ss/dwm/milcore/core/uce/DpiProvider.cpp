// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#include "precomp.hpp"

using DpiUtil = wpf::util::DpiUtil;
using DpiAwarenessContext = wpf::util::DpiAwarenessContext;

/// <summary>
/// Sets <see cref="m_pContext"/> from an integer, by 
/// mapping it to <see cref="DPI_AWARENESS_CONTEXT"/> as follows: 
/// -1  :   DPI_AWARENESS_CONTEXT_UNAWARE
/// -2  :   DPI_AWARENESS_CONTEXT_SYSTEM_AWARE
/// -3  :   DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE
/// -4  :   DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
/// </summary>
void DpiProvider::SetDpiAwarenessContext(int context)
{
    m_pDpiAwarenessContext = std::make_shared<DpiAwarenessContext>(context);
}

void DpiProvider::UpdateDpi(const DpiScale& dpi)
{
    m_dpi = dpi;
}

const DPI_AWARENESS_CONTEXT DpiProvider::GetDpiAwarenessContext() const override
{
    return m_pDpiAwarenessContext ? *m_pDpiAwarenessContext : nullptr;
}

HRESULT DpiProvider::GetCurrentDpi(DpiScale* pDpiScale) const override
{
    if (m_dpi.DpiScaleX == 0 || m_dpi.DpiScaleY == 0)
    {
        return E_FAIL; 
    }

    if (pDpiScale == nullptr)
    {
        return E_INVALIDARG;
    }

    *pDpiScale = m_dpi;
    return S_OK;
}

BOOL DpiProvider::IsPerMonitorDpiAware() const override
{
    DPI_AWARENESS_CONTEXT dpiContext = GetDpiAwarenessContext();
    return 
        DpiUtil::AreDpiAwarenessContextsEqual(dpiContext, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE) || 
        DpiUtil::AreDpiAwarenessContextsEqual(dpiContext, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
}
