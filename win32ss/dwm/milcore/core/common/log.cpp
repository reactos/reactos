// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++



Abstract:

    runtime logging tools.

Environment:

    User mode only.

--*/

#include "precomp.hpp"

#if MIL_LOGGER

CLogger *g_pLog = NULL;

//
// 512 bytes per entry seems a reasonable maximum.
//

#define MAX_LOG_ENTRY 512

//
// Set the total log size to 1Mb
//

#define LOGGER_SIZE (1024*1024)


MtDefine(CLogger, MILRender, "CLogger");

CLogger::CLogger()
{
    m_pvLog = NULL;
    m_cbLog = 0;
    m_cbCurrent = 0;
    m_fDumpNext = FALSE;
}

CLogger::~CLogger()
{
    FreeHeap(m_pvLog);
}

HRESULT CLogger::Create(__deref_out_ecount(1) CLogger **ppLogger)
{
    HRESULT hr = S_OK;
    CLogger *p = new CLogger;

    if (p)
    {
        hr = p->Initialize();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        *ppLogger = p;
        p = NULL;
    }

    delete p;
    return hr;
}

HRESULT CLogger::Initialize()
{
    HRESULT hr = HrMalloc(Mt(CLogger), LOGGER_SIZE, &m_pvLog);

    if (SUCCEEDED(hr))
    {
        m_cbLog = LOGGER_SIZE;
    }

    return hr;
}

HRESULT CLogger::Print(__in PCSTR pFormat, ...)
{
    HRESULT hr = S_OK;

    //
    // If the dumpnext variable was set, emit the log to the log file and
    // clear the bit.
    //

    if (m_fDumpNext)
    {
        hr = Dump();
        m_fDumpNext = FALSE;
    }

    //
    // Make sure there's enough space left in the log to store this request.
    //

    if (SUCCEEDED(hr) &&
        m_cbLog - m_cbCurrent < MAX_LOG_ENTRY)
    {
        hr = E_FAIL;
    }

    //
    // Emit the text to the log.
    //

    if (SUCCEEDED(hr))
    {
        va_list arglist;

        va_start(arglist, pFormat);

        hr = StringCchVPrintfA(
            (CHAR*)m_pvLog + m_cbCurrent,
            MAX_LOG_ENTRY,
            pFormat,
            arglist
            );

        va_end(arglist);

        size_t cChars = 0;

        //
        // We've checked the remaining buffer size above and StringCchVPrintfA
        // will guarantee null termination. Ignore the HRESULT from the
        // StringCchLengthA function.
        //

        IGNORE_HR(StringCchLengthA(
            (CHAR*)m_pvLog + m_cbCurrent,
            MAX_LOG_ENTRY,
            &cChars
            ));

        m_cbCurrent += cChars;
    }

    return hr;
}

HRESULT CLogger::Dump()
{
    FILE *fp = NULL;
    if (fp = fopen("log.txt", "w"))
    {
        //
        // Emit the entire log to the file, and reset the current offest to the
        // beginning to indicate that the log is now empty again.
        //

        fwrite(m_pvLog, m_cbCurrent, 1, fp);
        fclose(fp);

        m_cbCurrent = 0;
    }
    else
    {
        return E_FAIL;
    }
    return S_OK;
}

#endif MIL_LOGGER




