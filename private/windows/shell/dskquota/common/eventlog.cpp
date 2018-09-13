///////////////////////////////////////////////////////////////////////////////
/*  File: eventlog.cpp

    Description: Implements a subset of the NT event log APIs as a C++ class.
        CEventLog is intended only to provide a convenient method for writing
        NT event log messages.  No reading of event log entries is supported.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    02/14/98    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include "eventlog.h"
#include "registry.h"

TCHAR CEventLog::m_szFmtDec[] = TEXT("%1!d!");
TCHAR CEventLog::m_szFmtHex[] = TEXT("0x%1!X!");


CEventLog::~CEventLog(
    void
    )
{
    DBGTRACE((DM_EVENTLOG, DL_MID, TEXT("CEventLog::~CEventLog")));
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
    DBGTRACE((DM_EVENTLOG, DL_MID, TEXT("CEventLog::Initialize")));
    if (NULL != m_hLog)
    {
        return S_FALSE;
    }

    HRESULT hr = NOERROR;
    m_hLog = RegisterEventSource(NULL, pszEventSource);
    if (NULL == m_hLog)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBGERROR((TEXT("Error 0x%08X registering event source \"%s\""), hr, pszEventSource));
        DBGERROR((TEXT("Run regsvr32 on dskquota.dll")));
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
    DBGTRACE((DM_EVENTLOG, DL_MID, TEXT("CEventLog::Close")));

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
    DBGTRACE((DM_EVENTLOG, DL_MID, TEXT("CEventLog::ReportEvent")));

    if (NULL == m_hLog)
    {
        DBGERROR((TEXT("Event log not initialized")));
        return E_FAIL;
    }

    BOOL bResult = false;
    HRESULT hr = NOERROR;
    if (0 < m_rgstrText.Count())
    {
        bResult = ReportEvent(wType,
                              wCategory,
                              dwEventID,
                              m_rgstrText,
                              lpUserSid,
                              pvRawData,
                              cbRawData);
        m_rgstrText.Clear();
    }
    else
    {
        bResult = ::ReportEvent(m_hLog,
                                wType,
                                wCategory,
                                dwEventID,
                                lpUserSid,
                                0,
                                cbRawData,
                                NULL,
                                pvRawData);
    }
    if (!bResult)
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
            DBGERROR((TEXT("Error 0x%08X reporting event"), hr));
        }
    }
    return hr;
}


//
// Report an event.  Replacement strings are explicitly specified
// through an array of CString objects.
//
HRESULT
CEventLog::ReportEvent(
    WORD wType,
    WORD wCategory,
    DWORD dwEventID,
    const CArray<CString>& rgstr,
    PSID lpUserSid,
    LPVOID pvRawData,
    DWORD cbRawData
    )
{
    DBGTRACE((DM_EVENTLOG, DL_MID, TEXT("CEventLog::ReportEvent [ with strings ]")));

    if (NULL == m_hLog)
    {
        DBGERROR((TEXT("Event log not initialized")));
        return E_FAIL;
    }

    HRESULT hr = NOERROR;
    int cStrings = rgstr.Count();
    array_autoptr<LPCTSTR> rgpsz;
    if (0 < cStrings)
    {
        rgpsz = new LPCTSTR[cStrings];
        for (int i = 0; i < cStrings; i++)
        {
            rgpsz[i] = rgstr[i].Cstr();
        }
    }
    if (!::ReportEvent(m_hLog,
                       wType,
                       wCategory,
                       dwEventID,
                       lpUserSid,
                       (WORD)cStrings,
                       cbRawData,
                       rgpsz.get(),
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
            DBGERROR((TEXT("Error 0x%08X reporting event"), hr));
        }
    }
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
    DBGTRACE((DM_EVENTLOG, DL_LOW, TEXT("CEventLog::Push [ integer ]")));

    CString s;
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
                try
                {
                    s = pszBuffer;
                }
                catch(...)
                {
                    // do nothing.
                    DBGERROR((TEXT("Exception caught copying error msg text.")));
                }
            }
            LocalFree(pszBuffer);
        }
    }
    else
    {
        s.Format(eFmtDec == fmt ? m_szFmtDec : m_szFmtHex, hr);
    }
    m_rgstrText.Append(s);
}

//
// Push a string onto the stack of replacement strings.
//
void
CEventLog::Push(
    LPCTSTR psz
    )
{
    DBGTRACE((DM_EVENTLOG, DL_LOW, TEXT("CEventLog::Push [ string ]")));
    m_rgstrText.Append(CString(psz));
}



