//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       eventlog.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCUI_EVENTLOG_H
#define _INC_CSCUI_EVENTLOG_H
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

        class CStrArray
        {
            public:
                CStrArray(void);
                ~CStrArray(void)
                    { Destroy(); }

                bool Append(LPCTSTR psz);

                void Clear(void)
                    { Destroy(); }

                int Count(void) const
                    { return m_cEntries; }

                LPCTSTR Get(int iEntry) const;

                operator LPCTSTR* () const
                    { return (LPCTSTR *)m_rgpsz; }

            private:
                enum { MAX_ENTRIES = 8 };

                int    m_cEntries;
                LPTSTR m_rgpsz[MAX_ENTRIES];

                void Destroy(void);
                //
                // Prevent copy.
                //
                CStrArray(const CStrArray& rhs);
                CStrArray& operator = (const CStrArray& rhs);
        };


        CEventLog(void)
            : m_hLog(NULL) 
              { }

        ~CEventLog(void);

        HRESULT Initialize(LPCTSTR pszEventSource);

        bool IsInitialized(void)
            { return NULL != m_hLog; }

        void Close(void);

        HRESULT ReportEvent(WORD wType,
                            WORD wCategory,
                            DWORD dwEventID,
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
        HANDLE    m_hLog;
        CStrArray m_rgstrText;

        //
        // Prevent copy.
        //
        CEventLog(const CEventLog& rhs);
        CEventLog& operator = (const CEventLog& rhs);
};



#endif // _INC_CSCUI_EVENTLOG_H

