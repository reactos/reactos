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

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>
#include <stdarg.h>

#define NDEBUG
#include <debug.h>


/******************************************************************************
 * BackupEventLogA [ADVAPI32.@]
 */
BOOL WINAPI
BackupEventLogA (HANDLE hEventLog,
		 LPCSTR lpBackupFileName)
{
  DPRINT1("stub\n");
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
BackupEventLogW (HANDLE hEventLog,
		 LPCWSTR lpBackupFileName)
{
  DPRINT1("stub\n");
  return TRUE;
}


/******************************************************************************
 * ClearEventLogA [ADVAPI32.@]
 */
BOOL WINAPI
ClearEventLogA (HANDLE hEventLog,
		LPCSTR lpBackupFileName)
{
  DPRINT1("stub\n");
  return TRUE;
}


/******************************************************************************
 * ClearEventLogW [ADVAPI32.@]
 */
BOOL WINAPI
ClearEventLogW (HANDLE hEventLog,
		LPCWSTR lpBackupFileName)
{
  DPRINT1("stub\n");
  return TRUE;
}


/******************************************************************************
 * CloseEventLog [ADVAPI32.@]
 */
BOOL WINAPI
CloseEventLog (HANDLE hEventLog)
{
  DPRINT1("stub\n");
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
BOOL WINAPI
DeregisterEventSource (HANDLE hEventLog)
{
  DPRINT1("(%p): stub\n",hEventLog);
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
GetNumberOfEventLogRecords (HANDLE hEventLog,
			    PDWORD NumberOfRecords)
{
  DPRINT1("stub\n");
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
GetOldestEventLogRecord (HANDLE hEventLog,
			 PDWORD OldestRecord)
{
  DPRINT1("stub\n");
  return TRUE;
}


/******************************************************************************
 * NotifyChangeEventLog [ADVAPI32.@]
 *
 * PARAMS
 *   hEventLog []
 *   hEvent    []
 */
BOOL WINAPI
NotifyChangeEventLog (HANDLE hEventLog,
		      HANDLE hEvent)
{
  DPRINT1("stub\n");
  return TRUE;
}


/******************************************************************************
 * OpenBackupEventLogA [ADVAPI32.@]
 */
HANDLE WINAPI
OpenBackupEventLogA (LPCSTR lpUNCServerName,
		     LPCSTR lpFileName)
{
  DPRINT1("stub\n");
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
OpenBackupEventLogW (LPCWSTR lpUNCServerName,
		     LPCWSTR lpFileName)
{
  DPRINT1("stub\n");
  return (HANDLE)1;
}


/******************************************************************************
 * OpenEventLogA [ADVAPI32.@]
 */
HANDLE WINAPI
OpenEventLogA (LPCSTR lpUNCServerName,
	       LPCSTR lpSourceName)
{
  DPRINT1("(%s,%s),stub!\n",
	  lpUNCServerName, lpSourceName);
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
OpenEventLogW (LPCWSTR lpUNCServerName,
	       LPCWSTR lpSourceName)
{
  DPRINT1("stub\n");
  return (HANDLE)1;
}


/******************************************************************************
 * ReadEventLogA [ADVAPI32.@]
 */
BOOL WINAPI
ReadEventLogA (HANDLE hEventLog,
	       DWORD dwReadFlags,
	       DWORD dwRecordOffset,
	       LPVOID lpBuffer,
	       DWORD nNumberOfBytesToRead,
	       DWORD *pnBytesRead,
	       DWORD *pnMinNumberOfBytesNeeded)
{
  DPRINT1("stub\n");
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
ReadEventLogW (HANDLE hEventLog,
	       DWORD dwReadFlags,
	       DWORD dwRecordOffset,
	       LPVOID lpBuffer,
	       DWORD nNumberOfBytesToRead,
	       DWORD *pnBytesRead,
	       DWORD *pnMinNumberOfBytesNeeded)
{
  DPRINT1("stub\n");
  return TRUE;
}


/******************************************************************************
 * RegisterEventSourceA [ADVAPI32.@]
 */
HANDLE WINAPI
RegisterEventSourceA (LPCSTR lpUNCServerName,
		      LPCSTR lpSourceName)
{
  UNICODE_STRING UNCServerName;
  UNICODE_STRING SourceName;
  HANDLE ret;

  RtlCreateUnicodeStringFromAsciiz (&UNCServerName,
				    (PSTR)lpUNCServerName);
  RtlCreateUnicodeStringFromAsciiz (&SourceName,
				    (PSTR)lpSourceName);

  ret = RegisterEventSourceW (UNCServerName.Buffer,
			      SourceName.Buffer);

  RtlFreeUnicodeString (&UNCServerName);
  RtlFreeUnicodeString (&SourceName);

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
RegisterEventSourceW (LPCWSTR lpUNCServerName,
		      LPCWSTR lpSourceName)
{
  DPRINT1("(%S, %S): stub\n",
	  lpUNCServerName, lpSourceName);
  return (HANDLE)1;
}


/******************************************************************************
 * ReportEventA [ADVAPI32.@]
 */
BOOL WINAPI
ReportEventA (HANDLE hEventLog,
	      WORD wType,
	      WORD wCategory,
	      DWORD dwEventID,
	      PSID lpUserSid,
	      WORD wNumStrings,
	      DWORD dwDataSize,
	      LPCSTR *lpStrings,
	      LPVOID lpRawData)
{
  LPCWSTR *wideStrArray;
  UNICODE_STRING str;
  int i;
  BOOL ret;

  if (wNumStrings == 0)
     return TRUE;

  if (lpStrings == NULL)
     return TRUE;

  wideStrArray = HeapAlloc (GetProcessHeap (),
			    0,
			    sizeof(LPCWSTR) * wNumStrings);

  for (i = 0; i < wNumStrings; i++)
    {
        RtlCreateUnicodeStringFromAsciiz (&str,
					  (PSTR)lpStrings[i]);
        wideStrArray[i] = str.Buffer;
    }

  ret = ReportEventW (hEventLog,
		      wType,
		      wCategory,
		      dwEventID,
		      lpUserSid,
		      wNumStrings,
		      dwDataSize,
		      wideStrArray,
		      lpRawData);

  for (i = 0; i < wNumStrings; i++)
    {
      if (wideStrArray[i])
	{
	  HeapFree (GetProcessHeap (),
		    0,
		    (LPSTR)wideStrArray[i]);
	}
    }

  HeapFree (GetProcessHeap(),
	    0,
	    wideStrArray);

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
ReportEventW (HANDLE hEventLog,
	      WORD wType,
	      WORD wCategory,
	      DWORD dwEventID,
	      PSID lpUserSid,
	      WORD wNumStrings,
	      DWORD dwDataSize,
	      LPCWSTR *lpStrings,
	      LPVOID lpRawData)
{
  int i;

    /* partial stub */

  if (wNumStrings == 0)
    return TRUE;

  if (lpStrings == NULL)
    return TRUE;

  for (i = 0; i < wNumStrings; i++)
    {
      switch (wType)
        {
        case EVENTLOG_SUCCESS:
            DPRINT1("Success: %S\n", lpStrings[i]);
            break;

        case EVENTLOG_ERROR_TYPE:
            DPRINT1("Error: %S\n", lpStrings[i]);
            break;

        case EVENTLOG_WARNING_TYPE:
            DPRINT1("Warning: %S\n", lpStrings[i]);
            break;

        default:
            DPRINT1("Type %hu: %S\n", wType, lpStrings[i]);
            break;
        }
    }

  return TRUE;
}
