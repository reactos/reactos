/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    username.hxx

Abstract:

    Class for managing user name

Author:

    Richard L Firth (rfirth) 30-Aug-1997


Revision History:

    30-Aug-1997 rfirth
        Created

--*/

class CUserName {

private:

    CRITICAL_SECTION m_CritSec;
    ICSTRING m_UserName;

public:

    CUserName() {
        InitializeCriticalSection(&m_CritSec);
        m_UserName = NULL;
    }

    ~CUserName() {
        DeleteCriticalSection(&m_CritSec);
    }

    VOID Set(VOID) {

        EnterCriticalSection(&m_CritSec);
        if (!m_UserName.HaveString()) {

            char buf[256];
            DWORD length = sizeof(buf);

            if (GetUserName(buf, &length)) {
                m_UserName = buf;
            }
        }
        LeaveCriticalSection(&m_CritSec);
    }

    LPSTR Get(LPSTR lpszBuffer, DWORD dwLength) {

        EnterCriticalSection(&m_CritSec);
        if (!m_UserName.HaveString()) {
            Set();
        }
        if ((m_UserName.StringLength() < (int)dwLength) && (dwLength > 0)) {
            m_UserName.CopyTo(lpszBuffer, dwLength);
        } else {
            if (dwLength > 0) {
                *lpszBuffer = '\0';
            }
            lpszBuffer = "";
        }
        LeaveCriticalSection(&m_CritSec);
        return lpszBuffer;
    }

    LPSTR Get(LPSTR lpszBuffer, LPDWORD lpdwLength) {

        EnterCriticalSection(&m_CritSec);
        if (!m_UserName.HaveString()) {
            Set();
        }

        DWORD dwLength = *lpdwLength;

        if ((m_UserName.StringLength() < (int)dwLength) && (dwLength != 0)) {
            m_UserName.CopyTo(lpszBuffer, lpdwLength);
        } else {
            if (dwLength > 0) {
                *lpszBuffer = '\0';
            }
            lpszBuffer = "";
            *lpdwLength = 0;
        }
        LeaveCriticalSection(&m_CritSec);
        return lpszBuffer;
    }

    VOID Clear(VOID) {

        EnterCriticalSection(&m_CritSec);
        m_UserName = NULL;
        LeaveCriticalSection(&m_CritSec);
    }
};
