/*
 * Win32 advapi functions
 *
 * Copyright 1995 Sven Verdoolaege
 * Copyright 1998 Juergen Schmied
 * Copyright 2003 Mike Hearn
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define __USE_W32API
#define _WIN32_WINNT 0x0501
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "wine/winternl.h"

#define YDEBUG
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);
WINE_DECLARE_DEBUG_CHANNEL(eventlog);

/******************************************************************************
 * BackupEventLogA [ADVAPI32.@]
 */
BOOL WINAPI BackupEventLogA( HANDLE hEventLog, LPCSTR lpBackupFileName )
{
	FIXME("stub\n");
	return TRUE;
}

/******************************************************************************
 * BackupEventLogW [ADVAPI32.@]
 *
 * PARAMS
 *   hEventLog        []
 *   lpBackupFileName []
 */
BOOL WINAPI
BackupEventLogW( HANDLE hEventLog, LPCWSTR lpBackupFileName )
{
	FIXME("stub\n");
	return TRUE;
}

/******************************************************************************
 * ClearEventLogA [ADVAPI32.@]
 */
BOOL WINAPI ClearEventLogA ( HANDLE hEventLog, LPCSTR lpBackupFileName )
{
	FIXME("stub\n");
	return TRUE;
}

/******************************************************************************
 * ClearEventLogW [ADVAPI32.@]
 */
BOOL WINAPI ClearEventLogW ( HANDLE hEventLog, LPCWSTR lpBackupFileName )
{
	FIXME("stub\n");
	return TRUE;
}

/******************************************************************************
 * CloseEventLog [ADVAPI32.@]
 */
BOOL WINAPI CloseEventLog ( HANDLE hEventLog )
{
	FIXME("stub\n");
	return TRUE;
}

/******************************************************************************
 * DeregisterEventSource [ADVAPI32.@]
 * Closes a handle to the specified event log
 *
 * PARAMS
 *    hEventLog [I] Handle to event log
 *
 * RETURNS STD
 */
BOOL WINAPI DeregisterEventSource( HANDLE hEventLog )
{
    FIXME("(%p): stub\n",hEventLog);
    return TRUE;
}

/******************************************************************************
 * GetNumberOfEventLogRecords [ADVAPI32.@]
 *
 * PARAMS
 *   hEventLog       []
 *   NumberOfRecords []
 */
BOOL WINAPI
GetNumberOfEventLogRecords( HANDLE hEventLog, PDWORD NumberOfRecords )
{
	FIXME("stub\n");
	return TRUE;
}

/******************************************************************************
 * GetOldestEventLogRecord [ADVAPI32.@]
 *
 * PARAMS
 *   hEventLog    []
 *   OldestRecord []
 */
BOOL WINAPI
GetOldestEventLogRecord( HANDLE hEventLog, PDWORD OldestRecord )
{
	FIXME(":stub\n");
	return TRUE;
}

/******************************************************************************
 * NotifyChangeEventLog [ADVAPI32.@]
 *
 * PARAMS
 *   hEventLog []
 *   hEvent    []
 */
BOOL WINAPI NotifyChangeEventLog( HANDLE hEventLog, HANDLE hEvent )
{
	FIXME("stub\n");
	return TRUE;
}

/******************************************************************************
 * OpenBackupEventLogA [ADVAPI32.@]
 */
HANDLE WINAPI
OpenBackupEventLogA( LPCSTR lpUNCServerName, LPCSTR lpFileName )
{
	FIXME("stub\n");
	return (HANDLE)1;
}

/******************************************************************************
 * OpenBackupEventLogW [ADVAPI32.@]
 *
 * PARAMS
 *   lpUNCServerName []
 *   lpFileName      []
 */
HANDLE WINAPI
OpenBackupEventLogW( LPCWSTR lpUNCServerName, LPCWSTR lpFileName )
{
	FIXME("stub\n");
	return (HANDLE)1;
}

/******************************************************************************
 * OpenEventLogA [ADVAPI32.@]
 */
HANDLE WINAPI OpenEventLogA(LPCSTR uncname,LPCSTR source)
{
	FIXME("(%s,%s),stub!\n",uncname,source);
	return (HANDLE)0xcafe4242;
}

/******************************************************************************
 * OpenEventLogW [ADVAPI32.@]
 *
 * PARAMS
 *   uncname []
 *   source  []
 */
HANDLE WINAPI
OpenEventLogW( LPCWSTR uncname, LPCWSTR source )
{
	FIXME("stub\n");
	return (HANDLE)1;
}

/******************************************************************************
 * ReadEventLogA [ADVAPI32.@]
 */
BOOL WINAPI ReadEventLogA( HANDLE hEventLog, DWORD dwReadFlags, DWORD dwRecordOffset,
    LPVOID lpBuffer, DWORD nNumberOfBytesToRead, DWORD *pnBytesRead, DWORD *pnMinNumberOfBytesNeeded )
{
	FIXME("stub\n");
	return TRUE;
}

/******************************************************************************
 * ReadEventLogW [ADVAPI32.@]
 *
 * PARAMS
 *   hEventLog                []
 *   dwReadFlags              []
 *   dwRecordOffset           []
 *   lpBuffer                 []
 *   nNumberOfBytesToRead     []
 *   pnBytesRead              []
 *   pnMinNumberOfBytesNeeded []
 */
BOOL WINAPI
ReadEventLogW( HANDLE hEventLog, DWORD dwReadFlags, DWORD dwRecordOffset,
                 LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                 DWORD *pnBytesRead, DWORD *pnMinNumberOfBytesNeeded )
{
	FIXME("stub\n");
	return TRUE;
}

/******************************************************************************
 * RegisterEventSourceA [ADVAPI32.@]
 */
HANDLE WINAPI RegisterEventSourceA( LPCSTR lpUNCServerName, LPCSTR lpSourceName )
{
    
    UNICODE_STRING lpUNCServerNameW;
    UNICODE_STRING lpSourceNameW;
    HANDLE ret;
    RtlCreateUnicodeStringFromAsciiz(&lpUNCServerNameW, lpUNCServerName);
    RtlCreateUnicodeStringFromAsciiz(&lpSourceNameW, lpSourceName);
    ret = RegisterEventSourceW(lpUNCServerNameW.Buffer,lpSourceNameW.Buffer);
    RtlFreeUnicodeString (&lpUNCServerNameW);
    RtlFreeUnicodeString (&lpSourceNameW);
    return ret;
}

/******************************************************************************
 * RegisterEventSourceW [ADVAPI32.@]
 * Returns a registered handle to an event log
 *
 * PARAMS
 *   lpUNCServerName [I] Server name for source
 *   lpSourceName    [I] Source name for registered handle
 *
 * RETURNS
 *    Success: Handle
 *    Failure: NULL
 */
HANDLE WINAPI
RegisterEventSourceW( LPCWSTR lpUNCServerName, LPCWSTR lpSourceName )
{
    FIXME("(%s,%s): stub\n", debugstr_w(lpUNCServerName),
          debugstr_w(lpSourceName));
    return (HANDLE)1;
}

/******************************************************************************
 * ReportEventA [ADVAPI32.@]
 */
BOOL WINAPI ReportEventA ( HANDLE hEventLog, WORD wType, WORD wCategory, DWORD dwEventID,
    PSID lpUserSid, WORD wNumStrings, DWORD dwDataSize, LPCSTR *lpStrings, LPVOID lpRawData)
{
    LPCWSTR *wideStrArray;
    UNICODE_STRING str;
    int i;
    BOOL ret;

    if (wNumStrings == 0) return TRUE;
    if (!lpStrings) return TRUE;

    wideStrArray = HeapAlloc(GetProcessHeap(), 0, sizeof(LPCWSTR) * wNumStrings);
    for (i = 0; i < wNumStrings; i++)
    {
        RtlCreateUnicodeStringFromAsciiz(&str, lpStrings[i]);
        wideStrArray[i] = str.Buffer;
    }
    ret = ReportEventW(hEventLog, wType, wCategory, dwEventID, lpUserSid,
                       wNumStrings, dwDataSize, wideStrArray, lpRawData);
    for (i = 0; i < wNumStrings; i++)
    {
        if (wideStrArray[i]) HeapFree( GetProcessHeap(), 0, (LPSTR)wideStrArray[i] );
    }
    HeapFree(GetProcessHeap(), 0, wideStrArray);
    return ret;
}

/******************************************************************************
 * ReportEventW [ADVAPI32.@]
 *
 * PARAMS
 *   hEventLog   []
 *   wType       []
 *   wCategory   []
 *   dwEventID   []
 *   lpUserSid   []
 *   wNumStrings []
 *   dwDataSize  []
 *   lpStrings   []
 *   lpRawData   []
 */
BOOL WINAPI
ReportEventW( HANDLE hEventLog, WORD wType, WORD wCategory,
                DWORD dwEventID, PSID lpUserSid, WORD wNumStrings,
                DWORD dwDataSize, LPCWSTR *lpStrings, LPVOID lpRawData )
{
    int i;

    /* partial stub */

    if (wNumStrings == 0) return TRUE;
    if (!lpStrings) return TRUE;

    for (i = 0; i < wNumStrings; i++)
    {
        switch (wType)
        {
        case EVENTLOG_SUCCESS:
            TRACE_(eventlog)("%s\n", debugstr_w(lpStrings[i]));
            break;
        case EVENTLOG_ERROR_TYPE:
            ERR_(eventlog)("%s\n", debugstr_w(lpStrings[i]));
            break;
        case EVENTLOG_WARNING_TYPE:
            WARN_(eventlog)("%s\n", debugstr_w(lpStrings[i]));
            break;
        default:
            TRACE_(eventlog)("%s\n", debugstr_w(lpStrings[i]));
            break;
        }
    }
    return TRUE;

}
