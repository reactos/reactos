#include "pch.h"
#pragma hdrstop

#include "filetime.h"

//-----------------------------------------------------------------------------
// class FileTime
//-----------------------------------------------------------------------------
//
// Format the filetime numeric value to a text string.
// Uses the same format as the shell's defview.
//
void FileTime::GetString(
    CString *pstr
    ) const
{
    DBGASSERT((NULL != pstr));
    SYSTEMTIME st;

    //
    // This code matches the shell's date/time display format.
    //
    if (FileTimeToSystemTime(&m_time, &st))
    {
        int cchBuf = 40;
        LPTSTR pszBuf = pstr->GetBuffer(40);

        int cch = GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, pszBuf, cchBuf);
        cchBuf -= cch;
        pszBuf += cch - 1;

        *pszBuf++ = TEXT(' ');
        *pszBuf = 0;          // (in case GetTimeFormat doesn't add anything)
        cchBuf--;

        GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, pszBuf, cchBuf);
        pstr->ReleaseBuffer();
    }
    else
    {
        DBGERROR((TEXT("FileTimeToSystemTime failed with error 0x%08X.\n\tLODWORD: 0x%08X, HIDWORD: 0x%08X"), 
                 GetLastError(), m_time.dwLowDateTime, m_time.dwHighDateTime));
    }
}
