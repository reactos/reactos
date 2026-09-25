// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Contents:  Header file for the MIL debug instrumentation.  This file
//             contains definitions that aren't directly used by 
//             instrumented code (non-API level definitions).
//
//------------------------------------------------------------------------

#pragma once

// copied from ntstatus.h, which causes build errors if included

#define NTSTATUS LONG

#define STATUS_QUOTA_EXCEEDED            ((NTSTATUS)0xC0000044L)
#define STATUS_INSUFFICIENT_RESOURCES    ((NTSTATUS)0xC000009AL)     // ntsubauth
#define STATUS_COMMITMENT_LIMIT          ((NTSTATUS)0xC000012DL)

//
// Instrumentation flags
//

// Perform no processing for failures
#define MILINSTRUMENTATIONFLAGS_DONOTHING               0x00

// Break when a failure occurs
#define MILINSTRUMENTATIONFLAGS_BREAKONFAIL             0x01

/// HRESULT list contains the only HR's we want to break on
#define MILINSTRUMENTATIONFLAGS_BREAKINCLUDELIST        0x02

// HRESULT list contains HR's we don't won't to break on.
// This is the default behavior.  It is defined as 0 to 
// ensure that "if (MILINSTRUMENTATIONFLAGS)" evalutes
// to "if (0)" when instrumentation is disabled and is thus 
// optimized away by the compiler.
#define MILINSTRUMENTATIONFLAGS_BREAKEXCLUDELIST        0x00

// Capture the stack when a failure occurs
#define MILINSTRUMENTATIONFLAGS_CAPTUREONFAIL           0x04

// Only allow break on a fail when kernel debugger is present
#define MILINSTRUMENTATIONFLAGS_NOBREAKUNLESSKDPRESENT  0x08

// Send a notification on out of memory failure
#define MILINSTRUMENTATIONFLAGS_OOMEXITONFAIL           0x10

// Capture and break on failure, useful because breaking can
// be disabled in the registry and captures always occur for 
// failure HRESULTs, regardless of the HRESULT lists
#define MILINSTRUMENTATIONFLAGS_BREAKANDCAPTURE    (MILINSTRUMENTATIONFLAGS_BREAKONFAIL | MILINSTRUMENTATIONFLAGS_CAPTUREONFAIL)

// Default flags - Capture stack
#define MILINSTRUMENTATIONFLAGS_DEFAULT            MILINSTRUMENTATIONFLAGS_CAPTUREONFAIL

// Default HRESULTs to exclude from triggering a failure (none are currently defined)
#define MILINSTRUMENTATION_DEFAULTEXCLUDEHRS

// Default OOM HRESULTs to exclude from triggering a failure
#define MILINSTRUMENTATION_DEFAULTOOMHRS            \
    E_OUTOFMEMORY,                                  \
    HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY),          \
    HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),    \
    HRESULT_FROM_WIN32(ERROR_NO_SYSTEM_RESOURCES),  \
    HRESULT_FROM_WIN32(ERROR_COMMITMENT_LIMIT),     \
    HRESULT_FROM_NT(STATUS_INSUFFICIENT_RESOURCES), \
    HRESULT_FROM_NT(STATUS_COMMITMENT_LIMIT),       \
    HRESULT_FROM_NT(STATUS_NO_MEMORY),              \
    HRESULT_FROM_NT(STATUS_QUOTA_EXCEEDED)

// The following 2 constants, along with the MILINSTRUMENTATIONFLAGS constant
// defined by SET_MILINSTRUMENTATION_FLAGS, are the 3 core components which 
// make the debug instrumentation configurable.  This file defines the default
// values within global scope.  The configuration macros in InstrumentationConfig.h
// re-define these constants within a more limited scope to change the behavior 
// of the instrumentation (set different flags, define a different HRESULT list, 
// etc.).  The API-level macros (IFC, MIL_SUCCEEDED, etc.) then pass these 
// contants to the HRESULT check function
//
// To avoid gross duplication of the default HRESULT list in the DLL we pass NULL 
// and 0 indicating this case.
static const HRESULT * const MILINSTRUMENTATIONHRESULTLIST = NULL;
static const UINT MILINSTRUMENTATIONHRESULTLISTSIZE = 0;

// Forward declarations
__declspec(noinline) void MilInstrumentationCheckHR( 
    DWORD dwFlags,
    __in_ecount_opt(uHRListLength) const HRESULT * const pHRList,
    UINT uHRListLength,
    HRESULT hrFailure, 
    UINT uiLine
    );

__declspec(noinline) void DoStackCapture(UINT cFramesToSkip, HRESULT hr, UINT uLine);
__declspec(noinline) void DoStackCapture(HRESULT hr, UINT uLine);

bool IsOOM(HRESULT hr);

HRESULT
CheckGUIHandleQuota(
    DWORD dwGUIHandleType,
    HRESULT hrNearQuota,
    HRESULT hrOtherwise
    );

//+-----------------------------------------------------------------------
//
//  Function:   MilInstrumentationCallHRCheckFunction
//
//  Synopsis:   Inline function that optimizes an HRESULT check function
//              call based on the current instrumentation configuration.
//              
//  Returns:    void
//
//------------------------------------------------------------------------
#ifdef BUILD_FOR_CODE_COVERAGE
    // By removing this function, the compiler is able to simplify many macros
    // to trivial checks which don't add blocks to our dll. All we lose is stack
    // capture functionality.
    #define MilInstrumentationCallHRCheckFunction(hr,b,c,d,e) ((void)(hr,b,c,d,e))
#else
__forceinline void
MilInstrumentationCallHRCheckFunction(
    HRESULT hr,                 // HRESULT to check
    const DWORD dwFailureFlags, // Instrumention configuration flags
    __in_ecount_opt(uiHRListSize) const HRESULT * const HRList,// HRESULT List    
    const UINT uiHRListSize,    // Number of HRs in HRList
    const UINT line             // Line number the failure occured on
    )
{
    // Because dwFailureFlags is a constant the if (dwFailureFlags) check is 
    // evaluated at compile-time, not run time. If dwFailureFlags is 0 
    // (MILINSTRUMENTATIONFLAGS_DONOTHING) no code is generated by this 
    // function.  Otherwise the compiler knows the if (dwFailureFlags) check
    // will always evaluate to true and isn't generated or evaluated at run-time,
    // but the rest of the function is generated.
    if (dwFailureFlags)
    {
        // If the default HRESULT list is being used, and the only flag specified
        // is CAPTUREONFAIL, then call the stack capture function instead.
        // This is an optimization that reduces the code size of each instrumented
        // macro by 6 bytes by passing 3 less parameters to the check fuction.  
        // Like the previous if statement, this check is optimized away by the compiler
        // because HRList and dwFailureFlags are constants.
        if ( (NULL == HRList) &&
            (dwFailureFlags == MILINSTRUMENTATIONFLAGS_CAPTUREONFAIL))
        {
            DoStackCapture(hr, line);
        }
        else
        {
            MilInstrumentationCheckHR(  dwFailureFlags, 
                                        HRList, 
                                        uiHRListSize, 
                                        hr,
                                        line); 
        }
    }
}
#endif


//+-----------------------------------------------------------------------
//
//  Function:   MilCheckHR
//
//  Synopsis:   Inline function that calls the HRESULT check method if
//              an FAILED HRESULT is passed in.
//              
//  Returns:    nothing
//
//------------------------------------------------------------------------
__forceinline void 
MilCheckHR(
    HRESULT hr,                 // HRESULT to check
    const DWORD dwFailureFlags, // Instrumention configuration flags
    __in_ecount_opt(uiHRListSize)
    const HRESULT HRList[],     // HRESULT List    
    const UINT uiHRListSize,    // Number of HRs in HRList
    const UINT line             // Line number the failure occured on
    )
{
    if (FAILED(hr))
    {
        MilInstrumentationCallHRCheckFunction(  hr, 
                                                dwFailureFlags, 
                                                HRList, 
                                                uiHRListSize, 
                                                line);                    
    }
    
    return;
}

//+-----------------------------------------------------------------------
//
//  Function:   MilCheckReturnValue
//
//  Synopsis:   Inline function that calls the HRESULT check method if
//              a FAILED HRESULT is passed in. It asserts when a non-S_OK
//              success HRESULT is being returned but wasn't specified by
//              using RRETURN1-3.
//              
//  Returns:    The same HRESULT passed in as a parameter
//
//------------------------------------------------------------------------
__forceinline void
MilCheckReturnValue(
    HRESULT hr,                 // HRESULT to check
    const DWORD dwFailureFlags, // Instrumention configuration flags
    __in_ecount_opt(uiHRListSize)
    const HRESULT HRList[],     // HRESULT List    
    const UINT uiHRListSize,    // Number of HRs in HRList
    const UINT line,            // Line number the failure occured on
    HRESULT s1,                 // First allowed non-SOK success HRESULT
    HRESULT s2,                 // Second allowed non-SOK success HRESULT
    HRESULT s3                  // Third allowed non-SOK success HRESULT
    )
{
    // Sometimes these are referenced. That's okay though, because
    // UNREFERENCED_PARAMETER does no harm.
    UNREFERENCED_PARAMETER(hr);
    UNREFERENCED_PARAMETER(dwFailureFlags);
    UNREFERENCED_PARAMETER(HRList);
    UNREFERENCED_PARAMETER(uiHRListSize);
    UNREFERENCED_PARAMETER(line);
    UNREFERENCED_PARAMETER(s1);
    UNREFERENCED_PARAMETER(s2);
    UNREFERENCED_PARAMETER(s3);

    // Assert if HR is an unspecified non-S_OK success code. 
    // Non-S_OK success codes shouldn't be used because they tend to be blindly propgated 
    // by the immediate caller to other callers that handle them incorrectly.  See the RRETURN
    // comments for a more in-depth example.
    //
    // Consider indicating limited success using OUT-PARAMs instead of non-S_OK success codes.
    // If a non-S_OK success code *must* be returned from a function and your hitting
    // this assert in RRETURN, use RRETURNX instead.  
    Assert( FAILED(hr) ||   // Must be failed
            (hr == S_OK) || // Or S_OK           
            (hr == s1) ||   // Or one of the specified success HRs
            (hr == s2) ||
            (hr == s3) );

    //
    // Enable stack capture on RRETURN for code with RRRETURNCAPTURE
    // defined. Normally this macro is defined in a sources file.
    //
#ifdef RRRETURNCAPTURE
    MilCheckHR(
        hr,
        dwFailureFlags,
        HRList,
        uiHRListSize,
        line);
#endif
}



