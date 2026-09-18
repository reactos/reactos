// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Abstract:
//      Compatibility flags passed down by the UI thread
//
//------------------------------------------------------------------------------

#pragma once

class CCompatSettings
{
public:
    inline CCompatSettings() {}

#pragma region ShouldRenderEvenWhenNoDisplayDevicesAreAvailable

private:
    // When set to true, we will attempt to render even when no valid displays are 
    // available.
    // 
    // See the summary comments on MediaContext.ShouldRenderEvenWhenNoDisplayDevicesAreAvailable 
    // for details. 
    bool m_fRenderEvenWhenNoDisplayDevicesAreAvailable = false;

public:
    inline void SetRenderPolicyForNonInteractiveMode(bool fForceRender)
    {
        m_fRenderEvenWhenNoDisplayDevicesAreAvailable = fForceRender;
    }

    inline bool ShouldRenderEvenWhenNoDisplayDevicesAreAvailable() const
    {
        return m_fRenderEvenWhenNoDisplayDevicesAreAvailable;
    }

#pragma endregion
};

