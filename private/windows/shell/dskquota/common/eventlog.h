#ifndef _INC_DSKQUOTA_EVENTLOG_H
#define _INC_DSKQUOTA_EVENTLOG_H
///////////////////////////////////////////////////////////////////////////////
/*  File: eventlog.h

    Description: Header for eventlog.cpp.
        See eventlog.cpp for functional description.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    02/14/98    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _WINDOWS_
#   include <windows.h>
#endif

//
// This class provides basic NT event logging capability.  It provides only
// a subset of the full capability provided by the NT event logging APIs.
// I wanted a simple way to write messages to the event log.  No reading
// of event log entries is supported.
//
class CEventLog
{
    public:
        //
        // Number conversion formats.
        //
        enum eFmt { 
                     eFmtDec,       // Display as decimal.
                     eFmtHex,       // Display as hex
                     eFmtSysErr     // Display as win32 error text string.
                  };

        CEventLog(void)
            : m_hLog(NULL) 
              { DBGTRACE((DM_EVENTLOG, DL_MID, TEXT("CEventLog::CEventLog"))); }

        ~CEventLog(void);

        HRESULT Initialize(LPCTSTR pszEventSource);

        void Close(void);

        HRESULT ReportEvent(WORD wType,
                            WORD wCategory,
                            DWORD dwEventID,
                            PSID lpUserSid = NULL,
                            LPVOID pvRawData = NULL,
                            DWORD cbRawData = 0);

        HRESULT ReportEvent(WORD wType,
                            WORD wCategory,
                            DWORD dwEventID,
                            const CArray<CString>& rgstr,
                            PSID lpUserSid = NULL,
                            LPVOID pvRawData = NULL,
                            DWORD cbRawData = 0);

        //
        // Push replacement data onto a stack to replace the
        // %1, %2 etc. parameters in the message strings.
        //
        void Push(HRESULT hr, eFmt = eFmtDec);
        void Push(LPCTSTR psz);

    private:
        HANDLE          m_hLog;
        CArray<CString> m_rgstrText;

        static TCHAR m_szFmtDec[];
        static TCHAR m_szFmtHex[];

        //
        // Prevent copy.
        //
        CEventLog(const CEventLog& rhs);
        CEventLog& operator = (const CEventLog& rhs);
};



#endif // _INC_DSKQUOTA_EVENTLOG_H

