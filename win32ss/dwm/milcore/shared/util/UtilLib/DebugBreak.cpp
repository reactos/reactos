// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Contents:  Conditional DebugBreak implementation
//
//------------------------------------------------------------------------

#include "pch.h"
#include <dpfilter.h>

#ifdef __cplusplus
extern "C" {
#endif

// copied from wdm.h

NTSYSAPI
ULONG
__cdecl
DbgPrintEx (
    __in ULONG ComponentId,
    __in ULONG Level,
    __in_z __drv_formatString(printf) PCSTR Format,
    ...
    );

#ifdef __cplusplus
}
#endif

/// <summary>
/// If set to true, explicitly disables breaking on instrumentation failures.
/// </summary>
bool g_fDisableInstrumentationBreaks = false;

#ifdef PRERELEASE

// We will enable debug breaks for build names that have one of the following
// values as a substring. Please feel free to add more branches as appropriate.
static PCWCH g_szEnableDebugBreakForBuildSubstring[] = {
      L"fbl_dgt_dev1"
    , L"fbl_shell_dev1"
};

/// <summary>
/// True if the given branch name suggests this binary has been built under the vbl_wcp lab.
/// </summary>
static bool EnableDebugBreaks()
{
    static bool fEnableDebugBreaks = false;
    static bool fCached = false;

    if (!fCached)
    {
#define MIL_QUOTE(s) #s
#define MIL_STRINGIZE(s) MIL_QUOTE(s)

        static const WCHAR s_szBuildMachine[] = TEXT(MIL_STRINGIZE(__BUILDMACHINE__));

#undef MIL_STRINGIZE
#undef MIL_QUOTE
    
        // Make a copy of the build machine name string so that we can 
        // turn it into lower case in a thread-safe manner.
        WCHAR szBuildMachine[ARRAYSIZE(s_szBuildMachine)];

        Verify(SUCCEEDED(StringCchCopyW(szBuildMachine, ARRAY_SIZE(szBuildMachine), s_szBuildMachine)));

        _wcslwr(szBuildMachine);
 
        for (UINT i = 0; !fEnableDebugBreaks && i < ARRAY_SIZE(g_szEnableDebugBreakForBuildSubstring); i++)
        {
            fEnableDebugBreaks |= (wcsstr(szBuildMachine, g_szEnableDebugBreakForBuildSubstring[i]) != NULL);
        }

        fCached = true;
    }

    return fEnableDebugBreaks;
}

#endif


/// <summary>
/// Returns true if the desired behavior of MilUnexpectedError is to break into debugger.
/// </summary>
static bool BreakOnUnexpectedErrors()
{
    bool fResult = false;

    //
    // Check the registry key for the override.
    //

    DWORD dwOverride = false;

    if (RegGetHKLMDword(
        TEXT("Software\\Microsoft\\Avalon.Graphics"), 
        TEXT("BreakOnUnexpectedErrors"),
        &dwOverride
        ))
    {
        fResult = (dwOverride != 0);        
    }
#ifdef PRERELEASE
    else
    {
        // Selectively enable breaking behavior on certain builds.
        fResult = EnableDebugBreaks();
    }
#endif

    return fResult;
}


/// <summary>
/// Returns true if the desired behavior of MilInstrumentationBreak is to break into debugger.
/// </summary>
static bool BreakForInstrumentation()
{
    bool fResult = false;

    //
    // Check for the global instrumentation failure breaking override.
    //

    if (g_fDisableInstrumentationBreaks)
    {
        return false;
    }


    //
    // Check the registry key for the override.
    //

    DWORD dwOverride = false;

    if (RegGetHKLMDword(
        TEXT("Software\\Microsoft\\Avalon.Graphics"), 
        TEXT("DisableInstrumentationBreaking"),
        &dwOverride
        ))
    {
        fResult = (dwOverride == 0);        
    }
#ifdef PRERELEASE
    else
    {
        // Selectively enable breaking behavior on certain builds.
        fResult = EnableDebugBreaks();
    }
#endif

    return fResult;
}


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
void MilUnexpectedError(HRESULT hr, LPCTSTR pszContext)
{
    TCHAR szMessage[256];

    IGNORE_HR(StringCchPrintf(
        szMessage,
        ARRAY_SIZE(szMessage),
        TEXT("MIL FAILURE: Unexpected HRESULT 0x%08x in caller: %s"),
        hr, 
        pszContext));        

    DbgPrintEx(
        g_uDPFltrID,
        DPFLTR_ERROR_LEVEL,
        "%S\n",
        szMessage
        );

    if (BreakOnUnexpectedErrors())
    {
        //
        // NOTE TO FAILURE INVESTIGATORS:  
        // This break is due to an unexpected HRESULT in the caller, not this method.
        //

        FreRIPW(L"This break is due to an unexpected HRESULT in the caller, not this method.\n"
            L"***   Investigate the stack capture to determine the source of the HRESULT.\n");
    }
}


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
    DWORD dwFlags,      // MIL Instrumentation Flags
    bool fDebugBreak
    )
{
    if (   BreakForInstrumentation()
        && (   !(dwFlags & MILINSTRUMENTATIONFLAGS_NOBREAKUNLESSKDPRESENT)
               // Note: IsKernelDebuggerPresent = TRUE does NOT mean a user
               //       mode debugger won't intercept the break.
            || IsKernelDebuggerPresent()
               // Also allow case when KD is the only debugger that might catch
               // this and just is not present at the moment.
            || ( !IsDebuggerPresent() && IsKernelDebuggerEnabled() )
       )   )
    {
        // NOTE TO FAILURE INVESTIGATORS:  
        // This break is due to an unexpected HRESULT in the method that 
        // called the MilInstrumentation* method, not this code.

        if (fDebugBreak)
        {
            DebugBreak();
        }
        else
        {
            FreRIPW(L"Unexpected HRESULT in MilInstrumentation* caller");
        }
    }
}

/// <summary>
/// Explicitly disables breaking on instrumentation failures.
/// </summary>
void MilDisableInstrumentationBreaks()
{
    g_fDisableInstrumentationBreaks = true;
}


