// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Jolt JIT platform implementation
//
//-----------------------------------------------------------------------------
#include "precomp.h"

#include "windows.h"

#pragma warning(disable:4311) // pointer to int casts (needed for alignment)

CProgram *g_pProgram = NULL;

MtDefine(CJitterSupport, MILRender, "CJitterSupport");
MtDefine(WarpPlatform, MILRender, "WarpPlatform");


//-------------------------------------------------------------------------
//
//  Function:   WarpPlatform::BeginCompile
//
//  Synopsis:
//     Called when we start to compile a program.  Since all the collector
//     operator overloads are global, they need to access the current program.
// 
//     To handle multi-threaded call patterns, take a lock here
//
//-------------------------------------------------------------------------
void 
WarpPlatform::BeginCompile(CProgram* pProgram)
{
    g_pProgram = pProgram;
}

//-------------------------------------------------------------------------
//
//  Function:   WarpPlatform::BeginCompile
//
//  Synopsis:
//     Indicates the end of a compilation.  Release resources taken in 
//     BeginCompile.
//
//-------------------------------------------------------------------------
void 
WarpPlatform::EndCompile()
{
    g_pProgram = NULL;
}

//-------------------------------------------------------------------------
//
//  Function:   WarpPlatform::GetCurrentProgram
//
//  Synopsis:
//     Gets the current program being collected.
// 
//     Only valid to call this during a BeginCompile/EndCompile pair
//
//-------------------------------------------------------------------------
CProgram* 
WarpPlatform::GetCurrentProgram()
{
    return g_pProgram;
}

//-------------------------------------------------------------------------
//
//  Function:   CJitterSupport::GetCurrentProgram
//
//  Synopsis:
//     Gets the current program being collected.
// 
//     Only valid to call this during a BeginCompile/EndCompile pair
//
//-------------------------------------------------------------------------
CProgram* __STDCALL 
CJitterSupport::GetCurrentProgram()
{
    return g_pProgram;
}

//-------------------------------------------------------------------------
//
//  Function:   CJitterSupport::CodeAllocate
//
//  Synopsis:
//     Allocate memory for JIT'd program.  This memory must be 32 byte aligned
//     and in code pages that can be executed.  You need to use VirtualAlloc
//     to avoid data execption exceptions on AMD chips.
//
//-------------------------------------------------------------------------
HRESULT __STDCALL __checkReturn
CJitterSupport::CodeAllocate(__in UINT32 cbSize, __out UINT8 **ppAddress)
{
    HRESULT hr = S_OK;
    
    *ppAddress = (UINT8 *)VirtualAlloc(NULL, cbSize, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    IFCOOM(*ppAddress);

Cleanup:
    return hr;
}

//-------------------------------------------------------------------------
//
//  Function:   CJitterSupport::CodeFree
//
//  Synopsis:
//     Free code pages
//
//-------------------------------------------------------------------------
void __STDCALL 
CJitterSupport::CodeFree(__in void *pAddress)
{
    VirtualFree(pAddress, 0, MEM_RELEASE);
}

//-------------------------------------------------------------------------
//
//  Function:   CJitterSupport::MemoryAllocate
//
//  Synopsis:
//     Allocate regular heap memory for working space
//
//-------------------------------------------------------------------------
UINT8* __STDCALL 
CJitterSupport::MemoryAllocate(__in UINT32 cbSize, __out UINT32 & cbActualSize)
{
    UINT8 *pbResult;

    pbResult = (UINT8*)WPFAlloc(
        ProcessHeap,
        Mt(CJitterSupport),
        cbSize);

    if (pbResult != NULL)
    {
        cbActualSize = cbSize;
    }
    else
    {
        cbActualSize = 0;
    }

    return pbResult;
}

//-------------------------------------------------------------------------
//
//  Function:   CJitterSupport::MemoryFree
//
//  Synopsis:
//     Free heap memory
//
//-------------------------------------------------------------------------
void __STDCALL 
CJitterSupport::MemoryFree(__in void *pAddress)
{
    WPFFree(ProcessHeap, pAddress);
}

//-------------------------------------------------------------------------
//
//  Function:   WarpPlatform::AllocateMemory
//
//  Synopsis:
//     Allocate regular heap memory for working space
//
//-------------------------------------------------------------------------
void* 
WarpPlatform::AllocateMemory(size_t numBytes)
{
    return WPFAlloc(ProcessHeap, Mt(WarpPlatform), numBytes);
}

//-------------------------------------------------------------------------
//
//  Function:   WarpPlatform::FreeMemory
//
//  Synopsis:
//     Free heap memory
//
//-------------------------------------------------------------------------
void 
WarpPlatform::FreeMemory(void* pAddress)
{
    WPFFree(ProcessHeap, pAddress);
}

//-------------------------------------------------------------------------
//
//  Function:   WarpPlatform::ShaderTraceMessage
//
//  Synopsis:
//     Debug trace message
//
//-------------------------------------------------------------------------
void 
WarpPlatform::TraceMessage(
    __in_z const unsigned short *pzTraceMessage
    )
{
#if DBG
    OutputDebugString(pzTraceMessage);
    OutputDebugString(L"\n");
#endif
}

//-------------------------------------------------------------------------
//
//  Function:   WarpPlatform::ShaderAssertMessage
//
//  Synopsis:
//     Debug assert trigger
//
//-------------------------------------------------------------------------
void 
WarpPlatform::AssertMessage(
    __in_z const unsigned short *pzCondition,
    __in_z const unsigned short *pzFile,
    unsigned nLine
    )
{
#if DBG
    AssertMsg(false, "TODO: Need to implement this.");
#endif
}


WarpPlatform::LockHandle WarpPlatform::CreateLock()
{
    CRITICAL_SECTION* CS = (CRITICAL_SECTION*)WarpPlatform::AllocateMemory(sizeof(CRITICAL_SECTION));
    if(NULL == CS)
        return NULL;

    //
    // IFCW32 contains a comparison that may be constant depending on
    // compilation options. Skip the compiler warning for that.
    //

    // This call can throw an exception, but we ignore this in our 
    // CCriticalSection implementation too, so ignoring that for now.
    if (!InitializeCriticalSectionAndSpinCount(CS, 0))
    {
        return NULL;
    }

    return (WarpPlatform::LockHandle)CS;
}

void WarpPlatform::DeleteLock(LockHandle h)
{
    CRITICAL_SECTION* CS = (CRITICAL_SECTION*)h;

    if(CS)
    {
        DeleteCriticalSection(CS);
        WarpPlatform::FreeMemory(CS);
    }
}

void WarpPlatform::AcquireLock(LockHandle h)
{
    CRITICAL_SECTION* CS = (CRITICAL_SECTION*)h;
    WarpAssert(CS != NULL);

    EnterCriticalSection(CS);
}

void WarpPlatform::ReleaseLock(LockHandle h)
{
    CRITICAL_SECTION* CS = (CRITICAL_SECTION*)h;
    WarpAssert(CS != NULL);

    LeaveCriticalSection(CS);
}




