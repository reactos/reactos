/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#ifdef PRFDATA

#include "msxmlprfcounters.h"
#include "msxmlprfdatamap.hxx"

// None of this is protected by any locks ON PURPOSE - because locks make
// this too expensive - thereby changing the characteristics of the runtime
// environment too much (Heisenburg's principle in action).  Therefore I 
// willingly take the risk of corrupt counters (which happens VERY infrequently)

bool g_PerfTrace = false;
CPrfData::INSTID g_PerfInstId;

// Maintain static counters also so we can start keeping records
// BEFORE runtime_init has called PrfInitCounters.
static long g_GCPerfCount = 0;
static long g_ObjectPerfCount = 0;
static long g_PagesPerfCount = 0;
static long g_HeapSizPerfCount = 0;
static long g_OutOfMemoryCount = 0;

void PrfInitCounters()
{
    // Call this once on process startup
    if (! g_PerfTrace)
    {
        g_PrfData.Activate();

        // We have one set of these static counters per process, so use the
        // command line to be the counter instance name.  Must be < 20 chars.
        LPTSTR cmdline = ::GetCommandLine();
        TCHAR name[20];
        LPTSTR instname = name;

        // find the space char
        long i;
        for (i = 0; cmdline[i] && cmdline[i] != L' '; i++)
            ;

        // search backwards for '\'
        long j;
        for (j = i; j > 0 && cmdline[j] != L'\\'; j--)
            ;

        if (cmdline[j] == L'\\')
            j++;

        // copy first 19 alpha characters.
        long k;
        for (k = 0; j < i && k < 19 && ::IsCharAlpha(cmdline[j]); j++,k++)
            name[k] = cmdline[j];
        name[k] = '\0';
        if (k == 0)
        {
            instname = L"msxml"; // pick a name...
        }

        g_PerfInstId = g_PrfData.AddInstance(PRFOBJ_MSXML, instname);
        g_PerfTrace = (g_PerfInstId != (CPrfData::INSTID) -1);
        if (g_PerfTrace)
        {
            g_PrfData.GetCtr32(PRFCTR_GCCOUNT, g_PerfInstId) = g_GCPerfCount;
            g_PrfData.GetCtr32(PRFCTR_OBJECTS, g_PerfInstId) = g_ObjectPerfCount;
            g_PrfData.GetCtr32(PRFCTR_PAGES, g_PerfInstId) = g_PagesPerfCount;
            g_PrfData.GetCtr32(PRFCTR_HEAPSIZE, g_PerfInstId) = g_HeapSizPerfCount;
            g_PrfData.GetCtr32(PRFCTR_OOM, g_PerfInstId) = g_OutOfMemoryCount;
        }
    }
}

void PrfCleanupCounters()
{
    // Call this once on process exit
    if (g_PerfTrace)
    {
        g_PerfTrace = false;
        // Zero out the counters.
        g_PrfData.GetCtr32(PRFCTR_GCCOUNT, g_PerfInstId) = 0;
        g_PrfData.GetCtr32(PRFCTR_OBJECTS, g_PerfInstId) = 0;
        g_PrfData.GetCtr32(PRFCTR_PAGES, g_PerfInstId) = 0;
        g_PrfData.GetCtr32(PRFCTR_HEAPSIZE, g_PerfInstId) = 0;
        g_PrfData.GetCtr32(PRFCTR_OOM, g_PerfInstId) = 0;

        g_PrfData.RemoveInstance(PRFOBJ_MSXML, g_PerfInstId);
    }
}

void PrfCountGC( long delta )
{
    g_GCPerfCount += delta;
    if (g_GCPerfCount)
        g_PrfData.GetCtr32(PRFCTR_GCCOUNT, g_PerfInstId) = 
        g_GCPerfCount > 0 ? g_GCPerfCount : 0;
}

void PrfCountObjects( long delta )
{
    g_ObjectPerfCount += delta;
    if (g_PerfTrace)
        g_PrfData.GetCtr32(PRFCTR_OBJECTS, g_PerfInstId) =
        g_ObjectPerfCount > 0 ? g_ObjectPerfCount : 0;
}

void PrfCountCommittedPages( long delta )
{
    g_PagesPerfCount += delta;
    if (g_PerfTrace)
        g_PrfData.GetCtr32(PRFCTR_PAGES, g_PerfInstId) = 
        g_PagesPerfCount > 0 ? g_PagesPerfCount : 0;
}

void PrfCountHeapSize( long delta )
{
    g_HeapSizPerfCount += delta;
    if (g_PerfTrace)
        g_PrfData.GetCtr32(PRFCTR_HEAPSIZE, g_PerfInstId) = 
        g_HeapSizPerfCount > 0 ? g_HeapSizPerfCount : 0;
}

void PrfCountOOM( long delta )
{
    g_OutOfMemoryCount += delta;
    if (g_PerfTrace)
        g_PrfData.GetCtr32(PRFCTR_OOM, g_PerfInstId) = 
        g_OutOfMemoryCount > 0 ? g_OutOfMemoryCount : 0;
}

LPVOID PrfHeapAlloc(
  HANDLE hHeap,  // handle to the private heap block
  DWORD dwFlags, // heap allocation control flags
  DWORD dwBytes  // number of bytes to allocate
)
{
    PVOID pv = HeapAlloc(hHeap, dwFlags, dwBytes);
    if (pv) 
    {
        PrfCountHeapSize((long)(HeapSize(hHeap, dwFlags, pv)));
    }
/*
    PVOID pv = HeapAlloc(hHeap, dwFlags, dwBytes+sizeof(DWORD));
    if (pv) 
    {
        DWORD* pdw = (DWORD*)pv;
        *pdw++ = dwBytes; // store size in block & skip past it
        PrfCountHeapSize(dwBytes);
        pv = pdw;
    }
*/
    return pv;
}

BOOL PrfHeapFree(
  HANDLE hHeap,  // handle to the heap
  DWORD dwFlags, // heap freeing flags
  LPVOID lpMem   // pointer to the memory to free
)
{
    long size = (long)HeapSize(hHeap, dwFlags, lpMem); 
/*
    DWORD* pdw = (DWORD*)lpMem;
    pdw--; // go back to the WORD we saved in PrfHeapAlloc
    DWORD size = *pdw;
    lpMem = pdw;
*/
    BOOL rc = HeapFree(hHeap, dwFlags, lpMem);
    if (rc) 
    {
        PrfCountHeapSize(-(long)size);
    }
    return rc;
}


#endif
