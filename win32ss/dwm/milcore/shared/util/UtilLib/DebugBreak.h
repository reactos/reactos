// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Contents:  Conditional DebugBreak implementation
//
//------------------------------------------------------------------------

#pragma once

/// <summary>
/// Breaks into the debugger if the library has been built under the vbl_wcp lab.
///
/// The behavior can be overriden by setting the following registry key: 
///
///     "HKLM\Software\Microsoft\Avalon.Graphics\BreakOnUnexpectedErrors"
///
/// to a non-zero value to enable and zero to disable breaking.
///
/// If breaking is disabled, output a warning message to the debugger.
/// </summary>
void MilUnexpectedError(HRESULT hr, LPCTSTR pszContext);

/// <summary>
/// Breaks into the debugger if the library has been built under the vbl_wcp lab.
///
/// The behavior can be overriden by setting the following registry key: 
///
///     "HKLM\Software\Microsoft\Avalon.Graphics\DisableInstrumentationBreaking"
///
/// to a non-zero value to disable and zero to enable breaking.
///
/// Additionally, an API (MilDisableInstrumentationBreaks) is provided 
/// to explicitly disable the breaking behavior in a global manner.
///
/// If breaking is disabled, no action is taken.
/// </summary>
void MilInstrumentationBreak(
    DWORD dwFlags,                      // MIL Instrumentation Flags
    bool fUseSimpleDebugBreak = false   // Avoid advanced Assert implementation
    );

/// <summary>
/// Explicitly disables breaking on instrumentation failures.
/// </summary>
void MilDisableInstrumentationBreaks();

