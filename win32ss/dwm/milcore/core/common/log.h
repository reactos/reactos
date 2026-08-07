// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+--------------------------------------------------------------------------
//

//
//  Abstract:
//      runtime logging tools.
//
//----------------------------------------------------------------------------
    
#pragma once

#define MIL_LOGGER 0

#if MIL_LOGGER

MtExtern(CLogger);

class CLogger
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CLogger));

    static HRESULT Create(__deref_out_ecount(1) CLogger **ppLogger);
    ~CLogger();

    //
    // Print some text to the log file. This will append after the current
    // location. The format is the same as printf.
    //

    HRESULT Print(__in PCSTR pFormat, ...);

    //
    // Dump the log to a file.
    //

    HRESULT Dump();

protected:

    CLogger();
    HRESULT Initialize();

protected:

    //
    // Pointer to the actual log memory and the count of bytes allocated.
    //

    VOID *m_pvLog;
    UINT m_cbLog;

    //
    // Current byte offset from the beginning of the log to the first unused
    // byte
    //

    UINT m_cbCurrent;

    //
    // If this is set, the next call to Print() will dump the entire log to
    // a log file. This is set in the debugger.
    //

    BOOL m_fDumpNext;
};

extern CLogger *g_pLog;

#endif MIL_LOGGER


