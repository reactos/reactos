/*++ BUILD Version: 0001

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:    perfutil.c

Abstract:
    This file implements the utility routines used to construct the
    common parts of a PERF_INSTANCE_DEFINITION (see winperf.h) and
    perform event logging functions.

Revision History:
--*/

//
//  include files
//

#include <windows.h>
#include <string.h>
#include <winperf.h>
#include "userctrs.h"     // error message definition
#include "perfmsg.h"
#include "perfutil.h"
#include "userdata.h"
#define INITIAL_SIZE     1024L
#define EXTEND_SIZE      1024L

//
// Global data definitions.
//

ULONG   ulInfoBufferSize = 0;
HANDLE  hEventLog   = NULL;    // event log handle for reporting events
                               // initialized in Open... routines
DWORD   dwLogUsers      = 0;   // count of functions using event log
DWORD   MESSAGE_LEVEL   = 0;
WCHAR   GLOBAL_STRING[]     = L"Global";
WCHAR   FOREIGN_STRING[]    = L"Foreign";
WCHAR   COSTLY_STRING[]     = L"Costly";
WCHAR   NULL_STRING[]       = L"\0";

//
// Global data from userdata.c
//
extern USER_DATA_DEFINITION UserDataDefinition;
extern CS_DATA_DEFINITION CSDataDefinition;

// pointer to null string
// test for delimiter, end of line and non-digit characters
// used by IsNumberInUnicodeList routine
//

#define DIGIT       1
#define DELIMITER   2
#define ENDOFSTRING 3
#define INVALID     4
#define EvalThisChar(c,d) ( \
     (c == d) ? DELIMITER : \
     (c == 0) ? ENDOFSTRING : \
     (c < (WCHAR)'0') ? INVALID : \
     (c > (WCHAR)'9') ? INVALID : \
     DIGIT)

HANDLE MonOpenEventLog ()
/*++
Routine Description:

    Reads the level of event logging from the registry and opens the
    channel to the event logger for subsequent event log entries.

Arguments:
        None

Return Value:
        Handle to the event log for reporting events.
        NULL if open not successful.
--*/
{
    HKEY hAppKey;
    TCHAR LogLevelKeyName[] = "SOFTWARE\\Microsoft\\WindowsNT\\CurrentVersion\\PerfLib";
    TCHAR LogLevelValueName[] = "EventLogLevel";
    LONG lStatus;
    DWORD dwLogLevel;
    DWORD dwValueType;
    DWORD dwValueSize;

    // if global value of the logging level not initialized or is disabled,
    //  check the registry to see if it should be updated.

    if (!MESSAGE_LEVEL) {
        lStatus = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                               LogLevelKeyName,
                               0,
                               KEY_READ,
                               &hAppKey);
        dwValueSize = sizeof (dwLogLevel);
        if (lStatus == ERROR_SUCCESS) {
            lStatus = RegQueryValueEx (hAppKey,
                               LogLevelValueName,
                               (LPDWORD)NULL,
                               &dwValueType,
                               (LPBYTE)&dwLogLevel,
                               &dwValueSize);
            if (lStatus == ERROR_SUCCESS) {
               MESSAGE_LEVEL = dwLogLevel;
            }
            else {
               MESSAGE_LEVEL = MESSAGE_LEVEL_DEFAULT;
            }
            RegCloseKey (hAppKey);
        }
        else {
            MESSAGE_LEVEL = MESSAGE_LEVEL_DEFAULT;
        }
    }
    if (hEventLog == NULL) {
        hEventLog = RegisterEventSource (
            (LPTSTR)NULL,            // Use Local Machine
            APP_NAME);               // event log app name to find in registry
         if (hEventLog != NULL) {
            REPORT_INFORMATION (UTIL_LOG_OPEN, LOG_DEBUG);
         }
    }
    if (hEventLog != NULL) {
         dwLogUsers++;           // increment count of perfctr log users
    }
    return (hEventLog);
}

VOID MonCloseEventLog ()
/*++
Routine Description:
      Closes the handle to the event logger if this is the last caller

Arguments:      None

Return Value:   None

--*/
{
    if (hEventLog != NULL) {
        dwLogUsers--;               // decrement usage
        if (dwLogUsers <= 0) {      // and if we're the last, then close up log
            REPORT_INFORMATION (UTIL_CLOSING_LOG, LOG_DEBUG);
            DeregisterEventSource (hEventLog);
        }
    }
}

DWORD GetQueryType (
    IN LPWSTR lpValue)
/*++
Routine Description:
    Returns the type of query described in the lpValue string so that
    the appropriate processing method may be used

Arguments
    IN lpValue
    string passed to PerfRegQuery Value for processing

Return Value

    QUERY_GLOBAL        if lpValue == 0 (null pointer)
           lpValue == pointer to Null string
           lpValue == pointer to "Global" string
    QUERY_FOREIGN
        if lpValue == pointer to "Foreign" string
    QUERY_COSTLY
        if lpValue == pointer to "Costly" string
    otherwise:

    QUERY_ITEMS

--*/

{
    WCHAR   *pwcArgChar, *pwcTypeChar;
    BOOL    bFound;
    if (lpValue == 0) {
        return QUERY_GLOBAL;
    }
    else if (*lpValue == 0) {
        return QUERY_GLOBAL;
    }

    // check for "Global" request
    pwcArgChar = lpValue;
    pwcTypeChar = GLOBAL_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string
    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }
    if (bFound) return QUERY_GLOBAL;

    // check for "Foreign" request
    pwcArgChar = lpValue;
    pwcTypeChar = FOREIGN_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string
    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }
    if (bFound) return QUERY_FOREIGN;

    // check for "Costly" request
    pwcArgChar = lpValue;
    pwcTypeChar = COSTLY_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string
    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }
    if (bFound) return QUERY_COSTLY;

    // if not Global and not Foreign and not Costly,
    // then it must be an item list

    return QUERY_ITEMS;
}

DWORD
IsNumberInUnicodeList (
    IN LPWSTR  lpwszUnicodeList
)
/*++
Routine Description:
    When querying counters, the collect function receives a list of Unicode strings
    representing object indexes.  This function parses the string and returns a bit
    combination representing the needed objects.

Arguments:
     IN lpwszUnicodeList
        Null terminated, space delimited list of decimal numbers

Return Value:
    The returned DWORD is a combination of QUERY_USER and QUERY_CS
--*/
{
    DWORD   dwThisNumber;
    DWORD   dwReturnValue;
    WCHAR   *pwcThisChar;
    WCHAR   wcDelimiter;
    BOOL    bValidNumber;
    BOOL    bNewItem;

    if (lpwszUnicodeList == 0) return 0;    // null pointer, # not found

    pwcThisChar = lpwszUnicodeList;
    dwThisNumber = 0;
    dwReturnValue = 0;
    wcDelimiter = (WCHAR)' ';
    bValidNumber = FALSE;
    bNewItem = TRUE;

    while (TRUE) {
        switch (EvalThisChar (*pwcThisChar, wcDelimiter)) {
            case DIGIT:
                // if this is the first digit after a delimiter, then
                // set flags to start computing the new number
                if (bNewItem) {
                    bNewItem = FALSE;
                    bValidNumber = TRUE;
                }
                if (bValidNumber) {
                    dwThisNumber *= 10;
                    dwThisNumber += (*pwcThisChar - (WCHAR)'0');
                }
                break;
            case ENDOFSTRING:
            case DELIMITER:
                // a delimiter is either the delimiter character or the
                // end of the string ('\0') if when the delimiter has been
                // reached a valid number was found, then compare it to the
                // number from the argument list. if this is the end of the
                // string and no match was found, then return.
                //
                if (bValidNumber) {
                    if (dwThisNumber == UserDataDefinition.UserObjectType.ObjectNameTitleIndex) {
                        dwReturnValue |= QUERY_USER;
                    }
                    else if (dwThisNumber == CSDataDefinition.CSObjectType.ObjectNameTitleIndex) {
                        dwReturnValue |= QUERY_CS;
                    }
                    bValidNumber = FALSE;
                }
                if (*pwcThisChar == 0) {
                    return dwReturnValue;
                }
                else {
                    bNewItem = TRUE;
                    dwThisNumber = 0;
                }
                break;

            case INVALID:
                // if an invalid character was encountered, ignore all
                // characters up to the next delimiter and then start fresh.
                // the invalid number is not compared.
                bValidNumber = FALSE;
                break;

            default:
                break;

        }
    pwcThisChar++;
    }

}   // IsNumberInUnicodeList
