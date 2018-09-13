//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       eventlog.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "eventlog.h"

CEventLog::~CEventLog(
    void
    )
{
    Close();
}

//
// Register the specified event source.
// Note that the registry entries must already exist.
// HKLM\System\CurrentControlSet\Services\EventLog\Application\<pszEventSource>
//     Requires values "EventMessageFile" and "TypesSupported".
//        
HRESULT
CEventLog::Initialize(
    LPCTSTR pszEventSource
    )
{
    if (NULL != m_hLog)
    {
        return S_FALSE;
    }

    HRESULT hr = NOERROR;
    m_hLog = RegisterEventSource(NULL, pszEventSource);
    if (NULL == m_hLog)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

//
// Deregister the event source.
//
void
CEventLog::Close(
    void
    )
{
    if (NULL != m_hLog)
    {
        DeregisterEventSource(m_hLog);
        m_hLog = NULL;
    }
}


//
// Report an event.  No replaceable parameters explicitly specified.
// If msg string contains replaceable parameters, use Push() to 
// build list of replacement strings.
//
HRESULT
CEventLog::ReportEvent(
    WORD wType,
    WORD wCategory,
    DWORD dwEventID,
    PSID lpUserSid,    // [optional]
    LPVOID pvRawData,  // [optional]
    DWORD cbRawData    // [optional]
    )
{
    if (NULL == m_hLog)
        return E_FAIL;

    BOOL bResult = FALSE;
    HRESULT hr = NOERROR;

    if (!::ReportEvent(m_hLog,
                       wType,
                       wCategory,
                       dwEventID,
                       lpUserSid,
                       (WORD)m_rgstrText.Count(),
                       cbRawData,
                       m_rgstrText,
                       pvRawData))
    {
        //
        // Special-case ERROR_IO_PENDING.  ::ReportEvent will fail with
        // this error code even when it succeeds.  Don't know exactly why
        // but it does.  Treat this as success so we don't get unnecessary
        // debugger output.
        //
        DWORD dwError = GetLastError();
        if (ERROR_IO_PENDING != dwError)
        {
            hr = HRESULT_FROM_WIN32(dwError);
        }
    }

    m_rgstrText.Clear();

    return hr;
}


            

//
// Push an HRESULT value onto the stack of replacment strings.
//
void 
CEventLog::Push(
    HRESULT hr, 
    eFmt fmt
    )
{
    if (eFmtSysErr == fmt)
    {
        LPTSTR pszBuffer = NULL;
        int cchLoaded = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                        NULL,
                                        HRESULT_CODE(hr),
                                        0,
                                        (LPTSTR)&pszBuffer,
                                        1,
                                        NULL);
        if (NULL != pszBuffer)
        {
            if (0 != cchLoaded)
            {
                m_rgstrText.Append(pszBuffer);
            }
            LocalFree(pszBuffer);
        }
    }
    else
    {
        TCHAR szNumber[40];
        wsprintf(szNumber, eFmtDec == fmt ? TEXT("%d") : TEXT("0x%08X"), hr);
        m_rgstrText.Append(szNumber);
    }     
}

//
// Push a string onto the stack of replacement strings.
//
void 
CEventLog::Push(
    LPCTSTR psz
    )
{
    m_rgstrText.Append(psz);
}


CEventLog::CStrArray::CStrArray(
    void
    ) : m_cEntries(0)
{
    ZeroMemory(m_rgpsz, sizeof(m_rgpsz));
}


LPCTSTR
CEventLog::CStrArray::Get(
    int iEntry
    ) const
{
    TraceAssert(iEntry < m_cEntries);

    if (iEntry < m_cEntries)
        return m_rgpsz[iEntry];

    return NULL;
}


bool
CEventLog::CStrArray::Append(
    LPCTSTR psz
    )
{
    TraceAssert(m_cEntries < (ARRAYSIZE(m_rgpsz) - 1));

    if (m_cEntries < (ARRAYSIZE(m_rgpsz) - 1))
    {
        LPTSTR pszNew = new TCHAR[lstrlen(psz) + 1];
        if (NULL != pszNew)
        {
            lstrcpy(pszNew, psz);
            m_rgpsz[m_cEntries++] = pszNew;
            return true;
        }
    }
    return false;
}


void
CEventLog::CStrArray::Destroy(
    void
    )
{
    for (int i = 0; i < ARRAYSIZE(m_rgpsz); i++)
    {
        delete[] m_rgpsz[i];
        m_rgpsz[i] = NULL;
    }
    m_cEntries = 0;
}
