// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Contents:  Implementation of MIL instrumentation functions
//
//------------------------------------------------------------------------
#include "Pch.h"

//
// Captured stack failures
//

struct StackCaptureFrame
{
    HRESULT hrFailure;
    DWORD   dwThreadId;
    UINT    uLineNumber;        
    PVOID   rgCapturedFrame[3];
};

StackCaptureFrame g_StackCaptureFrames[256] = { 0 };

volatile LONG g_nCurrentStackCaptureIndex = -1;


//
// Windows Error Reporting integration.
//

typedef HRESULT (WINAPI *PFNWERREGISTERMEMORYBLOCK)(
    __in  PVOID pvAddress,
    __in  DWORD dwSize
    );


//+-----------------------------------------------------------------------
//
//  Function:   EnsureStackCaptureRegisteredWithWER
//
//  Synopsis:   If WerRegisterMemoryBlock API is available, registers the
//              stack capture memory block with Windows Error Reporting
//              for inclusion in Watson reports.
//
//------------------------------------------------------------------------

void 
EnsureStackCaptureRegisteredWithWER()
{
    static volatile LONG s_lStackCaptureRegisteredWithWER = FALSE;

    if (InterlockedCompareExchange(&s_lStackCaptureRegisteredWithWER, TRUE, FALSE) == FALSE)
    {
        HMODULE hKernel32 = GetModuleHandle(L"kernel32.dll");

        if (hKernel32 != NULL)
        {
            PFNWERREGISTERMEMORYBLOCK pfnWerRegisterMemoryBlock = 
                reinterpret_cast<PFNWERREGISTERMEMORYBLOCK>(
                    GetProcAddress(
                        hKernel32,
                        "WerRegisterMemoryBlock"
                        ));

            // Down-level platforms do not have the WerRegisterMemoryBlock API.
            if (pfnWerRegisterMemoryBlock != NULL)
            {
                //
                // If the WerRegisterMemoryBlock API is present, register the memory
                // blocks containing stack capture frames and current stack capture 
                // index. This should guarantee that the stack capture information
                // will be present in Watson dumps.
                //

                IGNORE_HR(pfnWerRegisterMemoryBlock(g_StackCaptureFrames, sizeof(g_StackCaptureFrames)));
                IGNORE_HR(pfnWerRegisterMemoryBlock(const_cast<PLONG>(&g_nCurrentStackCaptureIndex), sizeof(g_nCurrentStackCaptureIndex)));
            }
        }        
    }
}


//+-----------------------------------------------------------------------
//
//  Function:   DoStackCapture
//
//  Synopsis:   Method that captures the stack to global memory
//                      
//  Returns:    void
//
//------------------------------------------------------------------------

#ifndef _AMD64_
// Disable frame pointer optimization in order to ensure that 
// there is a stack frame, making RtlCaptureStackBackTrace 
// to in a predictable manner.
//
// x64 doesn't have frame pointers and this pragma causes a warning
#pragma optimize("y", off) // disable FPO
#endif

__declspec(noinline) void 
DoStackCapture(
    UINT cFramesToSkip, // Distance between DoStackCapture and the first meaningful frame
    HRESULT hr,         // HRESULT that caused this stack capture
    UINT uLine          // Line number the failure occured on.
    )
{ 
    // Must not pass in S_OK because we assume this function will only be used
    // on non-S_OK HRESULTs.  With this assumption we use S_OK as the initial
    // value of g_hrStackCapture
    Assert(S_OK != hr);
    
    LONG nCurrentIndex, nNextIndex;

    // Make sure that the data relevant to the stack captures 
    // is registered for collection with Window Error Reporting.
    EnsureStackCaptureRegisteredWithWER();

    // Increment the current index in a thread-safe manner
    do
    {
        // Obtain the current index
        nCurrentIndex = g_nCurrentStackCaptureIndex;

        // Calculate the next index
        nNextIndex = (nCurrentIndex + 1) % ARRAY_SIZE(g_StackCaptureFrames);

        // Don't exchange unless g_nCurrentStackCaptureIndex is still nCurrentIndex
        //
        // Attempt an atomic assignment of the next index, until the current index 
        // hasn't changed since it was read
    } while ( nCurrentIndex != InterlockedCompareExchange(
                            &g_nCurrentStackCaptureIndex, 
                            nNextIndex, 
                            nCurrentIndex   
                            ));

    StackCaptureFrame *pCurrentFrame = g_StackCaptureFrames + nNextIndex;
    
    // Always save the bad HR that the caused the stack capture
    pCurrentFrame->hrFailure = hr;

    // Save the thread ID this error occured on.
    pCurrentFrame->dwThreadId = GetCurrentThreadId();        

    // Save the line number so we can isolate where in a function
    // a failure occurred if the compiler optimizes all calls to 
    // MilInstrumentationCheckHR within that function to a single location
    // (and thus all failures within the function would have the same 
    // return address on the stack).
    
    // Replace saving the line failures occur on with a more secure value
    pCurrentFrame->uLineNumber = uLine;

    // NULL the entire buffer so we don't confuse new 
    // frames with leftover frames from a previous capture
    ZeroMemory(pCurrentFrame->rgCapturedFrame, sizeof(pCurrentFrame->rgCapturedFrame));

    // Capture the stack
    USHORT cCapturedFrames = 
        RtlCaptureStackBackTrace(
            1 + cFramesToSkip,                          // Skip the instrumentation frames
            ARRAY_SIZE(pCurrentFrame->rgCapturedFrame), // Max # of frames
            pCurrentFrame->rgCapturedFrame,             // Place capture here
            NULL                                        // Ignored optional param
            );

    if (cCapturedFrames == 0)
    {
        // Set g_StackCapture to a recognizable invalid value if no frames were set.
        memset(pCurrentFrame->rgCapturedFrame, 0xE0, sizeof(pCurrentFrame->rgCapturedFrame));       
    }        
}

// This DoStackCapture implementation saves loading the distance
// parameter and should be used when the default HRESULT list is 
// not overriden -- in such case, the originating method can call 
// DoStackCapture directly.
__declspec(noinline) void 
DoStackCapture(
    HRESULT hr,         // HRESULT that caused this stack capture
    UINT uLine          // Line number the failure occured on.
    )
{
#if !(DBG || defined(_X86_))
    // Special case for amd64fre: for some reason, this function does not
    // generate a stack frame and RtlCaptureStackBackTrace does not take
    // it into account.
    DoStackCapture(0, hr, uLine);
#else
    DoStackCapture(1, hr, uLine);
#endif
}


//+-----------------------------------------------------------------------
//
//  Function:   MilInstrumentationHandleFailure
//
//  Synopsis:   Method that responds to an HRESULT failure with the
//              response specified by the instrumentation flags.  This method
//              is typically called by MilInstrumentationCheckHR but may
//              also be called directly (via the MILINSTRUMENTATION_HANDLEFAILEDHR
//              macro) when users want to explicitly trigger a failure.
//                      
//  Returns:    void
//
//------------------------------------------------------------------------

__declspec(noinline) void 
MilInstrumentationHandleFailure(
    UINT cFramesToSkip, // Distance between MilInstrumentationHandleFailure and first meaningful frame
    HRESULT hrFailed,   // FAILED HRESULT
    DWORD dwFlags,      // MIL Instrumentation Flags
    UINT uiLine         // Line number unexpected failure occured on
    )
{   
    // Stack capture trigger
    if (dwFlags & MILINSTRUMENTATIONFLAGS_CAPTUREONFAIL)
    {
        // Capture the stack due to an unexpected HR
        DoStackCapture(cFramesToSkip + 1, hrFailed, uiLine);
    }

    // OOM exit trigger
    if ((dwFlags & MILINSTRUMENTATIONFLAGS_OOMEXITONFAIL) && IsOOM(hrFailed) && !IsDebuggerPresent())
    {
        // Exit the process
        ExitProcess(hrFailed);
    }

    // Debug break trigger
    if (dwFlags & MILINSTRUMENTATIONFLAGS_BREAKONFAIL)
    {
        // Break w/ a STATUS_ASSERTION_FAILURE due to unexpected HR
        MilInstrumentationBreak(dwFlags);
    }
}

#ifndef _AMD64_
// Compiler docs say that turning on optimization back on doesn't actually turn it on
// but instead resets it to whatever was specified with /O
#pragma optimize("y", on) // disable FPO
#endif

//+-----------------------------------------------------------------------
//
//  Function:   IsHRInList
//
//  Synopsis:   Searches for an HRESULT in a HRESULT list
//              
//  Returns:    Whether or not an HR is in a HRESULT list
//
//------------------------------------------------------------------------
bool
IsHRInList(
    HRESULT hr,                                                // HRESULT to search
                                                               // for
    __in_ecount(uHRListLength) const HRESULT * const pHRList,  // HRESULT List to
                                                               // search through
    UINT uHRListLength                                         // Number of HRs in
                                                               // list
    )
{
    // For each HR
    for (UINT i = 0; i < uHRListLength; i++)
    {
        // If HR is found
        if (hr == pHRList[i])
        {
            // Return TRUE
            return true;
        }            
    }  

    // HR wasn't found
    return false;    
}

//+-----------------------------------------------------------------------
//
//  Function:   MilInstrumentationCheckHR
//
//  Synopsis:   HRESULT check function that compares an unsuccessful HRESULT  
//              to either an INCLUDE or EXCLUDE list of HRESULTs to determine 
//              if an unexpected failure occured.  Success HRESULTS are
//              checked for inline before this function is called.
//              
//  Returns:   void
//
//------------------------------------------------------------------------

__declspec(noinline) void 
MilInstrumentationCheckHR(
    DWORD dwFlags,                  // MIL Instrumentation flags to control behavior 
    __in_ecount_opt(uiHRListLength) const HRESULT * const pHRList,
                                    // Inclusive or exclusive HRESULT list, or NULL
    UINT uiHRListLength,            // Number HRESULTs in pHRList
    HRESULT hr,                     // Unsuccessful hr to check again the HR list
    UINT uiLine                     // Line number failure occured on
    )
{
    // uiHRListLength must be 0 if pHRList is NULL
    Assert(pHRList || (uiHRListLength == 0));

    bool fTriggerFailure = false;
    bool fIsOOM = IsOOM(hr);

    if ((dwFlags & MILINSTRUMENTATIONFLAGS_OOMEXITONFAIL) && fIsOOM)
    {
        fTriggerFailure = true;
    }
    else if (dwFlags & MILINSTRUMENTATIONFLAGS_BREAKINCLUDELIST)
    {
        // Must specify an HRESULT list if the BREAKINCLUDELIST flag is set
        Assert(pHRList);

        // Trigger a failure if the HR is in the list 
        if (IsHRInList(hr, pHRList, uiHRListLength))
        {
            fTriggerFailure = true;
        }
    }
    else  // Default to BREAKEXCLUDELIST
    {
        // Define default exclude list
        static const HRESULT defaultExcludeList[] =
        { 
            MILINSTRUMENTATION_DEFAULTOOMHRS,
            MILINSTRUMENTATION_DEFAULTEXCLUDEHRS
        };

        // Choose which exclude HRESULT list to compare hr against
        const HRESULT * pExcludeList;
        UINT uiExcludeListLength;
        if (!pHRList || (uiHRListLength == 0))
        {
            // Use default exclude list if an HRESULT list isn't passed in.
            // This optimization allows callers to not create & pass in an HRESULT list
            // if they are a common case.
            pExcludeList = defaultExcludeList;
            uiExcludeListLength = ARRAY_SIZE(defaultExcludeList);
        }
        else
        {
            // Use HRESULT that was passed in
            pExcludeList = pHRList;
            uiExcludeListLength = uiHRListLength;
        }

        // Trigger a failure if the HR isn't in the list 
        if (!IsHRInList(hr, pExcludeList, uiExcludeListLength))
        {
            fTriggerFailure = true;
        }
    }

    if (fTriggerFailure)
    {
        MilInstrumentationHandleFailure(1, hr, dwFlags, uiLine);
    }
    // When capture is enabled, capture E_OUTOFMEMORY, ERROR_NOT_ENOUGH_MEMORY,
    // and other OOM errors, even if the HRESULT was ignored.
    //
    // Triggering on OOM HRESULTs is disabled by placing them in the 'expected'
    // HRESULT list. This is because we must be able to recover from OOM (i.e.,
    // not int 3). But if for some reason we don't recover later on, we will
    // need the stack capture to determine the cause of the failure.
    else if (   (dwFlags & MILINSTRUMENTATIONFLAGS_CAPTUREONFAIL)  // Capturing is enabled
             && fIsOOM)                                            // and the failure is OOM
    {
        DoStackCapture(1, hr, uiLine);
    }
}


//+----------------------------------------------------------------------------
//
//  Function:  CheckGUIHandleQuota
//
//  Synopsis:  Check handle usage of given resource type by this process
//             against quota.  The return value is hrNearQuota when the count
//             is near the quota.  The check is "near" the quota, because there
//             is not atomic technique to create a resource and check count; so
//             we allow for others to have freed resources, but still detect
//             that we probably failed due to the quota.
//

struct GUIHandleQuotaInfo {
    DWORD dwTestBar;
    PCTSTR szRegValue;
} g_GUIHandleQuota[] =
{
  /* GR_GDIOBJECTS  */ { 0, _T("GDIProcessHandleQuota") },
  /* GR_USEROBJECTS */ { 0, _T("USERProcessHandleQuota") }
};

HRESULT
CheckGUIHandleQuota(
    DWORD dwGUIHandleType,  // Type of GUI object to check quota of.  Must be
                            // either GR_GDIOBJECTS of GR_USEROBJECTS
    HRESULT hrNearQuota,    // HRESULT to return when count is near quota
    HRESULT hrOtherwise     // HRESULT to return when count is NOT near quota
    )
{
    C_ASSERT(GR_GDIOBJECTS == 0);
    C_ASSERT(GR_USEROBJECTS == 1);

    Assert(dwGUIHandleType < ARRAY_SIZE(g_GUIHandleQuota));

    // Default to "otherwise" error if quota doesn't appear to be the problem
    // in later checks.
    HRESULT hr = hrOtherwise;

    //
    // Query current process usage
    //
    DWORD dwCount = GetGuiResources(GetCurrentProcess(), dwGUIHandleType);

    if (   dwGUIHandleType < ARRAY_SIZE(g_GUIHandleQuota) // Make PREfast happy
        && dwCount >= g_GUIHandleQuota[dwGUIHandleType].dwTestBar)
    {
        if (!g_GUIHandleQuota[dwGUIHandleType].dwTestBar)
        {
            // Set to default limit for XP
            DWORD dwQuota = 10000;
            HKEY hKey;

            //
            // Try to read quota from registry.
            //
            // Note that there is a slight chance this value is not the same as
            // what win32k.sys is using, because it may be changed at any time.
            // So all callers should be aware that this is only a guess.
            //
            // We also don't worry what happens if multiple threads try to
            // initialize a test bar at the same time - any result is fine.
            //
            // One benefit of this delayed read is that when debugging a zero
            // test bar value indicates that there have been no generic win32
            // create failures (assuming all failure points are properly
            // tested.)
            //

            if (RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows"),
                0,
                KEY_QUERY_VALUE,
                &hKey) == ERROR_SUCCESS)
            {
                DWORD dwType;
                DWORD dwValueRead;
                DWORD dwDataSize = sizeof(dwValueRead);

                if (   (RegQueryValueEx(
                        hKey,
                        g_GUIHandleQuota[dwGUIHandleType].szRegValue,
                        NULL,
                        &dwType,
                        reinterpret_cast<LPBYTE>(&dwValueRead),
                        &dwDataSize
                        ) == ERROR_SUCCESS)
                    && (dwType == REG_DWORD)
                    && (dwValueRead > 0))
                {
                    dwQuota = dwValueRead;
                }

                RegCloseKey(hKey);
            }

            //
            // Allow for a 12.5% margin of handle cleanup, but still detect
            // failure as reaching handle limit.  Use simple shift and
            // subtraction to avoid possible overflow/underflow situations and
            // always keep the result > 0.
            //
            // 12.5% is picked because it allows fast shift and is otherwise a
            // pretty random guess from JasonHa about when we are reaching
            // limits.  Since we try to minimize GDI object usage it doesn't
            // seem likely that we'd get near the quota anyway except under
            // extreme conditions.
            //
            g_GUIHandleQuota[dwGUIHandleType].dwTestBar =
                dwQuota - (dwQuota >> 3);

            Assert(g_GUIHandleQuota[dwGUIHandleType].dwTestBar > 0);
        }

        //
        // Check against bar again in case this is the first time it is set
        //
        if (dwCount >= g_GUIHandleQuota[dwGUIHandleType].dwTestBar)
        {
            hr = hrNearQuota;
        }
    }

    // Not RRETURN as this is part of instrumentation.
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:     IsOOM
//
//  Synopsis:   Determines if an exception code is an OOM exception code
//
//------------------------------------------------------------------------
bool IsOOM(HRESULT hr)
{
    bool fOOM = false;

    // RaiseException requires us to clear FACILITY_NT_BIT and add APPLICATION_ERROR_MASK
    static const HRESULT rghr[] =
    {
        MILINSTRUMENTATION_DEFAULTOOMHRS
    };
    
    for (UINT i = 0; i < ARRAYSIZE(rghr); i++)
    {
        if (rghr[i] == hr)
        {
            fOOM = true;
            break;
        }
    }

    return fOOM;
}




