/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

	SNMPTRLG.CPP


Abstract:

	This module is the tracing and logging routines for the SNMP Event Log
	Extension Agent DLL.

Author:

	Randy G. Braze (Braze Computing Services) Created 7 February 1996


Revision History:


--*/

extern "C" {

#include <windows.h>		// windows definitions
#include <stdio.h>			// standard I/O functions
#include <stdlib.h>			// standard library definitions
#include <stdarg.h>			// variable length arguments stuff
#include <string.h>			// string declarations
#include <time.h>			// time declarations

#include <snmp.h>			// snmp definitions
#include "snmpelea.h"		// global dll definitions
#include "snmptrlg.h"		// module specific definitions
#include "snmpelmg.h"		// message definitions

}


VOID
TraceWrite(
	IN CONST BOOL  fDoFormat,	// flag for message formatting
	IN CONST BOOL  fDoTime,		// flag for date/time prefixing
    IN CONST LPSTR szFormat,	// trace message to write
    IN OPTIONAL ...				// other printf type operands
    )

/*++

Routine Description:

	TraceWrite will write information provided to the trace file. Optionally,
	it will prepend the date and timestamp to the information. If requested,
	printf type arguments can be passed and they will be substituted just as
	printf builds the message text. Sometimes this routine is called from
	WriteTrace and sometimes it is called from other functions that need to
	generate a trace file record. When called from WriteTrace, no formatting
	is done on the buffer (WriteTrace has already performed the required
	formatting). When called from other functions, the message text may or
	may not require formatting, as specified by the calling function.


Arguments:

	fDoFormat	-	TRUE or FALSE, indicating if the message text provided
					requires formatting as a printf type function.

	fDoTime		-	TRUE or FALSE, indicating if the date/timestamp should be
					added to the beginning of the message text.

	szFormat	-	NULL terminated string containing the message text to be
					written to the trace file. If fDoFormat is true, then this
					text will be in the format of a printf statement and will
					contain substitution parameters strings and variable names
					to be substituted will follow.

	...			-	Optional parameters that are used to complete the printf
					type statement. These are variables that are substituted
					for strings specified in szFormat. These parameters will
					only be specified and processed if fDoFormat is TRUE.


Return Value:

	None
	

--*/

{
    static CHAR  szBuffer[4096];
    static FILE  *FFile;
    static SYSTEMTIME NowTime;
    va_list arglist;

    // don't even attempt to open the trace file if
    // the name is ""
    if (szTraceFileName[0] == TEXT('\0'))
        return;

    FFile = fopen(szTraceFileName,"a");     // open trace file in append mode
    if ( FFile != NULL )                    // if file opened okay
    {
        if ( fDoTime )                      // are we adding time?
        {
            GetLocalTime(&NowTime);         // yep, get it
            fprintf(FFile, "%02i/%02i/%02i %02i:%02i:%02i ",
                NowTime.wMonth,
                NowTime.wDay,
                NowTime.wYear,
                NowTime.wHour,
                NowTime.wMinute,
                NowTime.wSecond);           // file printf to add date/time
        }

        if ( fDoFormat )                    // if we need to format the buffer
        {
          va_start(arglist, szFormat);
          vsprintf(szBuffer, szFormat, arglist);  // perform substitution
          va_end(arglist);
          fwrite(szBuffer, strlen(szBuffer), 1, FFile);           // write data to the trace file
        }
        else                                // if no formatting required
        {
            fwrite(szFormat, strlen(szFormat), 1, FFile);   // write message to the trace file
        }

		fflush(FFile);						// flush buffers first
        fclose(FFile);                      // close the trace file
    }
}                                           // end TraceWrite function


VOID LoadMsgDLL(
    IN VOID
    )

/*++

Routine Description:

	LoadMsgDLL is called to load the SNMPELMG.DLL module which contains the
	message and format information for all messages in the SNMP extension agent DLL.
	It is necessary to call this routine only in the event that an event log
	record cannot be written. If this situation occurs, then the DLL will be
	loaded in an attempt to call FormatMessage and write this same information
	to the trace file. This routine is called only once and only if the
	event log write fails.


Arguments:

	None


Return Value:

	None
	

--*/

{
    TCHAR szXMsgModuleName[MAX_PATH+1]; // space for DLL message module
    DWORD nFile = MAX_PATH+1;           // max size for DLL message module name
    DWORD dwType;                       // type of message module name
    DWORD status;                       // status from registry calls
	DWORD cbExpand;						// byte count for REG_EXPAND_SZ parameters
    HKEY  hkResult;                     // handle to registry information

    if ( (status = RegOpenKeyEx(		// open the registry to read the name
        HKEY_LOCAL_MACHINE,				// of the message module DLL
        EVENTLOG_SERVICE,
        0,
        KEY_READ,
        &hkResult) ) != ERROR_SUCCESS)
    {
        TraceWrite(TRUE, TRUE,			// if we can't find it
			"LoadMessageDLL: Unable to open EventLog service registry key; RegOpenKeyEx returned %lu\n",
			status);					// write trace event record
        hMsgModule = (HMODULE) NULL;	// set handle null
        return;							// return
    }
    else
    {
        if ( (status = RegQueryValueEx(	// look up module name
            hkResult,					// handle to registry key
            EXTENSION_MSG_MODULE,		// key to look up
            0,							// ignored
            &dwType,					// address to return type value
            (LPBYTE) szXMsgModuleName,	// where to return message module name
            &nFile) ) != ERROR_SUCCESS)	// size of message module name field
        {
            TraceWrite(TRUE, TRUE,		// if we can't find it
				"LoadMessageDLL: Unable to open EventMessageFile registry key; RegQueryValueEx returned %lu\n",
				status);				// write trace event record
            hMsgModule = (HMODULE) NULL;	// set handle null
			RegCloseKey(hkResult);		// close the registry key
            return;						// return
        }

		RegCloseKey(hkResult);		// close the registry key

        cbExpand = ExpandEnvironmentStrings(	// expand the DLL name
            szXMsgModuleName,					// unexpanded DLL name
            szelMsgModuleName,					// expanded DLL name
            MAX_PATH+1);						// max size of expanded DLL name

        if (cbExpand > MAX_PATH+1)		// if it didn't expand correctly
        {
            TraceWrite(TRUE, TRUE,		// didn't have enough space
				"LoadMessageDLL: Unable to expand message module %s; expanded size required is %lu bytes\n",
                szXMsgModuleName, cbExpand);	// log error message
            hMsgModule = (HMODULE) NULL;	// set handle null
            return;							// and exit
        }

        if ( (hMsgModule = (HMODULE) LoadLibraryEx(szelMsgModuleName, NULL, LOAD_LIBRARY_AS_DATAFILE) )   // load the message module name
            == (HMODULE) NULL )			// if module didn't load
        {
            TraceWrite(TRUE, TRUE,		// can't load message dll
				"LoadMessageDLL: Unable to load message module %s; LoadLibraryEx returned %lu\n",
                szelMsgModuleName, GetLastError() );  // log error message
        }
	}

    return;								// exit routine

}


VOID
FormatTrace(
    IN CONST NTSTATUS nMsg,         // message number to format
    IN CONST LPVOID   lpArguments   // strings to insert
    )

/*++

Routine Description:

	FormatTrace will write the message text specified by nMsg to the trace
	file. If supplied, the substitution arguments supplied by lpArguments
	will be inserted in the message. FormatMessage is called to format the
	message text and insert the substitution arguments into the text. The
	text of the message is loaded from the SNMPELMG.DLL message module as
	specified in the Eventlog\Application\Snmpelea registry entry under the key of
	EventMessageFile. This information is read, the file name is expanded and
	the message module is loaded. If the message cannot be formatted, then
	a record is written to the trace file indicating the problem.


Arguments:

	nMsg		-	This is the message number in SNMPELMG.H in NTSTATUS format
					that is to be written.

	lpArguments	-	This is a pointer to an array of strings that will be
					substituted in the message text specified. If this value
					is NULL, there are no substitution values to insert.


Return Value:

	None
	

--*/

{
    static DWORD nBytes;            // return value from FormatMessage
    static LPTSTR lpBuffer;         // temporary message buffer

    if ( !fMsgModule ) {            // if we don't have dll loaded yet
        fMsgModule = TRUE;          // indicate we've looked now
        LoadMsgDLL();				// load the DLL
    }

    if ( hMsgModule ) {

        nBytes = FormatMessage(     // see if we can format the message
            FORMAT_MESSAGE_ALLOCATE_BUFFER |    // let api build buffer
   			FORMAT_MESSAGE_ARGUMENT_ARRAY |		// indicate an array of string inserts
            FORMAT_MESSAGE_FROM_HMODULE,        // look thru message DLL
            (LPVOID) hMsgModule,                // handle to message module
            nMsg,                               // message number to get
            (ULONG) NULL,                       // specify no language
            (LPTSTR) &lpBuffer,                 // address for buffer pointer
            80,                                 // minimum space to allocate
            (va_list* )lpArguments);            // address of array of pointers

        if (nBytes == 0) {              // format is not okay
            TraceWrite(TRUE, TRUE,
				"FormatTrace: Error formatting message number %08X is %lu\n",
                nMsg, GetLastError() ); // trace the problem
        }
        else {                          // format is okay
            TraceWrite(FALSE, TRUE, lpBuffer);       // log the message in the trace file
        }

        if ( LocalFree(lpBuffer) != NULL ) {    // free buffer storage
            TraceWrite(TRUE, TRUE,
				"FormatTrace: Error freeing FormatMessage buffer is %lu\n",
                GetLastError() );
        }
    }
    else {
        TraceWrite(TRUE, TRUE,
			"FormatTrace: Unable to format message number %08X; message DLL handle is null.\n",
            nMsg); // trace the problem
    }

    return;                         // exit routine
}


USHORT
MessageType(
    IN CONST NTSTATUS nMsg
    )

/*++

Routine Description:

	MessageType is used to return the severity type of an NTSTATUS formatted
	message number. This information is needed to log the appropriate event
	log information when writing a record to the system event log. Acceptable
	message types are defined in NTELFAPI.H.


Arguments:

	nMsg		-	This is the message number in SNMPELMG.H in NTSTATUS format
					that is to be analyzed.


Return Value:

	Unsigned short integer containing the message severity as described in
	NTELFAPI.H. If no message type is matched, the default of informational
	is returned.

--*/

{
    switch ((ULONG) nMsg >> 30) {           // get message type
    case (SNMPELEA_SUCCESS) :
        return(EVENTLOG_SUCCESS);           // success message

    case (SNMPELEA_INFORMATIONAL) :
        return(EVENTLOG_INFORMATION_TYPE);  // informational message

    case (SNMPELEA_WARNING) :
        return(EVENTLOG_WARNING_TYPE);      // warning message

    case (SNMPELEA_ERROR) :
        return(EVENTLOG_ERROR_TYPE);        // error message

    default:
        return(EVENTLOG_INFORMATION_TYPE);  // default to informational
    }
}


VOID
WriteLog(
    IN NTSTATUS nMsgNumber
    )

/*++

Routine Description:

	WriteLog is called to write message text to the system event log. This is
	a C++ overloaded function. In case a log record cannot be written
	to the system event log, TraceWrite is called to write the appropriate
	message text to the trace file.


Arguments:

	nMsgNumber	-	This is the message number in SNMPELMG.H in NTSTATUS format
					that is to be written to the event log.


Return Value:

	None

--*/

{
    static USHORT wLogType;			// to hold event log type
    static BOOL   fReportEvent;		// return flag from report event

    if (hWriteEvent != NULL)		// if we have previous log access ability
    {
        wLogType = MessageType(nMsgNumber);	// get message type

        fReportEvent = ReportEvent(	// write message
            hWriteEvent,			// handle to log file
            wLogType,				// message type
            0,						// message category
            nMsgNumber,				// message number
            NULL,					// user sid
            0,						// number of strings
            0,						// data length
            0,						// pointer to string array
            (PVOID) NULL);			// data address

        if ( !fReportEvent )		// did the event log okay?
        {							// not if we get here.....
            TraceWrite(TRUE, TRUE,	// show error in trace file
				"WriteLog: Error writing to system event log is %lu\n",
                GetLastError() );
            FormatTrace(nMsgNumber, NULL);	// format trace information
        }
    }
    else							// if we can't write to event log
    {
        TraceWrite(FALSE, TRUE,		// show error in trace file
			"WriteLog: Unable to write to system event log; handle is null\n");
        FormatTrace(nMsgNumber, NULL);  // format trace information
    }
    return;							// exit the function
}


VOID
WriteLog(
    IN NTSTATUS nMsgNumber,           // message number to log
    IN DWORD dwCode                   // code to pass to message
    )

/*++

Routine Description:

	WriteLog is called to write message text to the system event log. This is
	a C++ overloaded function. In case a log record cannot be written
	to the system event log, TraceWrite is called to write the appropriate
	message text to the trace file.


Arguments:

	nMsgNumber	-	This is the message number in SNMPELMG.H in NTSTATUS format
					that is to be written to the event log.

	dwCode		-	This is a double word code that is to be converted to a
					string and substituted appropriately in the message text.


Return Value:

	None

--*/

{
    static USHORT wLogType;				// to hold event log type
    static TCHAR  *lpszEventString[1];	// array of strings to pass to event logger
    static BOOL   fReportEvent;			// return flag from report event

	lpszEventString[0] = new TCHAR[34];	// allocate space for string conversion

    if (hWriteEvent != NULL)			// if we have previous log access ability
    {
        if ( lpszEventString[0] != (TCHAR *) NULL )	// if storage allocated
        {
            wLogType = MessageType(nMsgNumber);		// get message type

            _ultoa(dwCode, lpszEventString[0], 10);	// convert to string

            fReportEvent = ReportEvent(	// write message
                hWriteEvent,			// handle to log file
                wLogType,				// message type
                0,						// message category
                nMsgNumber,				// message number
                NULL,					// user sid
                1,						// number of strings
                0,						// data length
                (const char **) lpszEventString,		// pointer to string array
                NULL);					// data address

            if ( !fReportEvent )		// did the event log okay?
            {							// not if we get here.....
                TraceWrite(TRUE, TRUE,	// write trace file record
					"WriteLog: Error writing to system event log is %lu\n",
                    GetLastError() );
                FormatTrace(nMsgNumber, lpszEventString);	// format trace information
            }
        }
        else							// if we can't allocate memory
        {
            TraceWrite(FALSE, TRUE,		// write trace file record
				"WriteLog: Error allocating memory for system event log write\n");
            FormatTrace(nMsgNumber, NULL);	// format trace information
        }
    }
    else								// if we can't write to system log
    {
        TraceWrite(FALSE, TRUE,			// write trace file record
			"WriteLog: Unable to write to system event log; handle is null\n");

        if ( lpszEventString[0] != (TCHAR *) NULL )	// if storage allocated
        {
            _ultoa(dwCode, lpszEventString[0], 10);	// convert to string
            FormatTrace(nMsgNumber, lpszEventString);	// format trace information
        }
        else							// if we can't allocate memory
        {
            TraceWrite(FALSE, TRUE,		// write trace file record
				"WriteLog: Error allocating memory for system event log write\n");
            FormatTrace(nMsgNumber, NULL);	// format trace information
        }
    }
	delete lpszEventString[0];			// free storage
    return;								// exit function
}


VOID
WriteLog(
    IN NTSTATUS nMsgNumber,
    IN DWORD dwCode1,
    IN DWORD dwCode2
    )

/*++

Routine Description:

	WriteLog is called to write message text to the system event log. This is
	a C++ overloaded function. In case a log record cannot be written
	to the system event log, TraceWrite is called to write the appropriate
	message text to the trace file.


Arguments:

	nMsgNumber	-	This is the message number in SNMPELMG.H in NTSTATUS format
					that is to be written to the event log.

	dwCode1		-	This is a double word code that is to be converted to a
					string and substituted appropriately in the message text.

	dwCode2		-	This is a double word code that is to be converted to a
					string and substituted appropriately in the message text.


Return Value:

	None

--*/

{
    static USHORT wLogType;				// to hold event log type
    static TCHAR  *lpszEventString[2];	// array of strings to pass to event logger
    static BOOL   fReportEvent;			// return flag from report event

	lpszEventString[0] = new TCHAR[34];	// allocate space for string conversion
	lpszEventString[1] = new TCHAR[34];	// allocate space for string conversion

    if (hWriteEvent != NULL)			// if we have previous log access ability
    {
        if ( (lpszEventString[0] != (TCHAR *) NULL) &&
             (lpszEventString[1] != (TCHAR *) NULL) )	// if storage allocated
        {
            wLogType = MessageType(nMsgNumber);	// get message type

            _ultoa(dwCode1, lpszEventString[0], 10);	// convert to string
            _ultoa(dwCode2, lpszEventString[1], 10);	// convert to string

            fReportEvent = ReportEvent(	// write message
                hWriteEvent,			// handle to log file
                wLogType,				// message type
                0,						// message category
                nMsgNumber,				// message number
                NULL,					// user sid
                2,						// number of strings
                0,						// data length
                (const char **) lpszEventString,		// pointer to string array
                NULL);					// data address

            if ( !fReportEvent )		// did the event log okay?
            {							// not if we get here.....
                TraceWrite(TRUE, TRUE,	// write a trace file entry
					"WriteLog: Error writing to system event log is %lu\n",
                    GetLastError() );
                FormatTrace(nMsgNumber, lpszEventString);	// format trace information
            }
        }
        else							// if we can't allocate memory
        {
            TraceWrite(FALSE, TRUE,		// write trace file record
				"WriteLog: Error allocating memory for system event log write\n");
            FormatTrace(nMsgNumber, NULL);	// format trace information
        }
    }
    else								// if we can't write to system log
    {
        TraceWrite(FALSE, TRUE,			// write trace file entry
			"WriteLog: Unable to write to system event log; handle is null\n");

        if ( (lpszEventString[0] != (TCHAR *) NULL) &&
             (lpszEventString[1] != (TCHAR *) NULL) )	// if storage allocated
        {
            _ultoa(dwCode1, lpszEventString[0], 10);	// convert to string
            _ultoa(dwCode2, lpszEventString[1], 10);	// convert to string
            FormatTrace(nMsgNumber, lpszEventString);	// format trace information
        }
        else							// if we can't allocate memory
        {
            TraceWrite(FALSE, TRUE,		// write trace file record
				"WriteLog: Error allocating memory for system event log write\n");
            FormatTrace(nMsgNumber, NULL);	// format trace information
        }
    }
	delete lpszEventString[0];			// free storage
	delete lpszEventString[1];			// free storage
    return;								// exit function
}


VOID
WriteLog(
    IN NTSTATUS nMsgNumber,
    IN DWORD dwCode1,
    IN LPTSTR lpszText1,
    IN LPTSTR lpszText2,
    IN DWORD dwCode2
    )

/*++

Routine Description:

	WriteLog is called to write message text to the system event log. This is
	a C++ overloaded function. In case a log record cannot be written
	to the system event log, TraceWrite is called to write the appropriate
	message text to the trace file.


Arguments:

	nMsgNumber	-	This is the message number in SNMPELMG.H in NTSTATUS format
					that is to be written to the event log.

	dwCode1		-	This is a double word code that is to be converted to a
					string and substituted appropriately in the message text.

	lpszText1	-	This contains a string parameter that is to be substituted
					into the message text.

	lpszText2	-	This contains a string parameter that is to be substituted
					into the message text.

	dwCode2		-	This is a double word code that is to be converted to a
					string and substituted appropriately in the message text.


Return Value:

	None

--*/

{
    static USHORT wLogType;				// to hold event log type
    static TCHAR  *lpszEventString[4];	// array of strings to pass to event logger
    static BOOL   fReportEvent;			// return flag from report event

	lpszEventString[0] = new TCHAR[34];	// allocate space for string conversion
	lpszEventString[1] = new TCHAR[MAX_PATH+1];	// allocate space for string conversion
	lpszEventString[2] = new TCHAR[MAX_PATH+1];	// allocate space for string conversion
	lpszEventString[3] = new TCHAR[34];	// allocate space for string conversion

    if (hWriteEvent != NULL)			// if we have previous log access ability
    {
        if ( (lpszEventString[0] != (TCHAR *) NULL) &&
             (lpszEventString[1] != (TCHAR *) NULL) &&
             (lpszEventString[2] != (TCHAR *) NULL) &&
             (lpszEventString[3] != (TCHAR *) NULL) )	// if storage allocated
        {
            wLogType = MessageType(nMsgNumber);			// get message type

            _ultoa(dwCode1, lpszEventString[0], 10);	// convert to string
            strcpy(lpszEventString[1],lpszText1);		// copy the string
            strcpy(lpszEventString[2],lpszText2);		// copy the string
            _ultoa(dwCode2, lpszEventString[3], 10);	// convert to string

            fReportEvent = ReportEvent(	// write message
                hWriteEvent,			// handle to log file
                wLogType,				// message type
                0,						// message category
                nMsgNumber,				// message number
                NULL,					// user sid
                4,						// number of strings
                0,						// data length
                (const char **) lpszEventString,		// pointer to string array
                NULL);					// data address

            if ( !fReportEvent )		// did the event log okay?
            {							// not if we get here.....
                TraceWrite(TRUE, TRUE,	// write trace file record
					"WriteLog: Error writing to system event log is %lu\n",
                    GetLastError() );
                FormatTrace(nMsgNumber, lpszEventString);	// format trace information
            }
        }
        else							// if we can't allocate memory
        {
            TraceWrite(FALSE, TRUE,		// write trace file record
				"WriteLog: Error allocating memory for system event log write\n");
            FormatTrace(nMsgNumber, NULL);	// format trace information
        }
    }
    else								// if we can't write to system log
    {
        TraceWrite(FALSE, TRUE,			// write trace file record
			"WriteLog: Unable to write to system event log; handle is null\n");

        if ( (lpszEventString[0] != (TCHAR *) NULL) &&
             (lpszEventString[1] != (TCHAR *) NULL) &&
             (lpszEventString[2] != (TCHAR *) NULL) &&
             (lpszEventString[3] != (TCHAR *) NULL) )	// if storage allocated
        {
            _ultoa(dwCode1, lpszEventString[0], 10);	// convert to string
            strcpy(lpszEventString[1],lpszText1);		// copy the string
            strcpy(lpszEventString[2],lpszText2);		// copy the string
            _ultoa(dwCode2, lpszEventString[3], 10);	// convert to string
            FormatTrace(nMsgNumber, lpszEventString);	// format trace information
        }
        else							// if we can't allocate memory
        {
            TraceWrite(FALSE, TRUE,		// write trace file record
				"WriteLog: Error allocating memory for system event log write\n");
            FormatTrace(nMsgNumber, NULL);	// format trace information
        }
    }
	delete lpszEventString[0];			// free storage
	delete lpszEventString[1];			// free storage
	delete lpszEventString[2];			// free storage
	delete lpszEventString[3];			// free storage
    return;								// exit function
}


VOID
WriteLog(
    IN NTSTATUS nMsgNumber,
    IN DWORD dwCode1,
    IN LPTSTR lpszText,
    IN DWORD dwCode2,
    IN DWORD dwCode3
    )

/*++

Routine Description:

	WriteLog is called to write message text to the system event log. This is
	a C++ overloaded function. In case a log record cannot be written
	to the system event log, TraceWrite is called to write the appropriate
	message text to the trace file.


Arguments:

	nMsgNumber	-	This is the message number in SNMPELMG.H in NTSTATUS format
					that is to be written to the event log.

	dwCode1		-	This is a double word code that is to be converted to a
					string and substituted appropriately in the message text.

	lpszText	-	This contains a string parameter that is to be substituted
					into the message text.

	dwCode2		-	This is a double word code that is to be converted to a
					string and substituted appropriately in the message text.

	dwCode3		-	This is a double word code that is to be converted to a
					string and substituted appropriately in the message text.


Return Value:

	None

--*/

{
    static USHORT wLogType;				// to hold event log type
    static TCHAR  *lpszEventString[4];	// array of strings to pass to event logger
    static BOOL   fReportEvent;			// return flag from report event

	lpszEventString[0] = new TCHAR[34];	// allocate space for string conversion
	lpszEventString[1] = new TCHAR[MAX_PATH+1];	// allocate space for string conversion
	lpszEventString[2] = new TCHAR[34];	// allocate space for string conversion
	lpszEventString[3] = new TCHAR[34];	// allocate space for string conversion

    if (hWriteEvent != NULL)			// if we have previous log access ability
    {
        if ( (lpszEventString[0] != (TCHAR *) NULL) &&
             (lpszEventString[1] != (TCHAR *) NULL) &&
             (lpszEventString[2] != (TCHAR *) NULL) &&
             (lpszEventString[3] != (TCHAR *) NULL) )	// if storage allocated
        {
            wLogType = MessageType(nMsgNumber);	// get message type

            _ultoa(dwCode1, lpszEventString[0], 10);	// convert to string
            strcpy(lpszEventString[1],lpszText);		// copy the string
            _ultoa(dwCode2, lpszEventString[2], 10);	// convert to string
            _ultoa(dwCode3, lpszEventString[3], 10);	// convert to string

            fReportEvent = ReportEvent(	// write message
                hWriteEvent,			// handle to log file
                wLogType,				// message type
                0,						// message category
                nMsgNumber,				// message number
                NULL,					// user sid
                4,						// number of strings
                0,						// data length
                (const char **) lpszEventString,		// pointer to string array
                NULL);					// data address

            if ( !fReportEvent )		// did the event log okay?
            {							// not if we get here.....
                TraceWrite(TRUE, TRUE,	// write trace file record
					"WriteLog: Error writing to system event log is %lu\n",
                    GetLastError() );
                FormatTrace(nMsgNumber, lpszEventString);	// format trace information
            }
        }
        else							// if we can't allocate memory
        {
            TraceWrite(FALSE, TRUE,		// write trace file record
				"WriteLog: Error allocating memory for system event log write\n");
            FormatTrace(nMsgNumber, NULL);	// format trace information
        }
    }
    else								// if we can't write to system log
    {
        TraceWrite(FALSE, TRUE,			// write trace file record
			"WriteLog: Unable to write to system event log; handle is null\n");

        if ( (lpszEventString[0] != (TCHAR *) NULL) &&
             (lpszEventString[1] != (TCHAR *) NULL) &&
             (lpszEventString[2] != (TCHAR *) NULL) &&
             (lpszEventString[3] != (TCHAR *) NULL) )	// if storage allocated
        {
            _ultoa(dwCode1, lpszEventString[0], 10);	// convert to string
            strcpy(lpszEventString[1],lpszText);		// copy the string
            _ultoa(dwCode2, lpszEventString[2], 10);	// convert to string
            _ultoa(dwCode3, lpszEventString[3], 10);	// convert to string
            FormatTrace(nMsgNumber, lpszEventString);	// format trace information
        }
        else							// if we can't allocate memory
        {
            TraceWrite(FALSE, TRUE,		// write trace file record
				"WriteLog: Error allocating memory for system event log write\n");
            FormatTrace(nMsgNumber, NULL);  // format trace information
        }
    }
	delete lpszEventString[0];			// free storage
	delete lpszEventString[1];			// free storage
	delete lpszEventString[2];			// free storage
	delete lpszEventString[3];			// free storage
    return;								// exit the function
}


VOID
WriteLog(
    IN NTSTATUS nMsgNumber,
    IN LPTSTR lpszText,
    IN DWORD dwCode1,
    IN DWORD dwCode2
    )

/*++

Routine Description:

	WriteLog is called to write message text to the system event log. This is
	a C++ overloaded function. In case a log record cannot be written
	to the system event log, TraceWrite is called to write the appropriate
	message text to the trace file.


Arguments:

	nMsgNumber	-	This is the message number in SNMPELMG.H in NTSTATUS format
					that is to be written to the event log.

	lpszText	-	This contains a string parameter that is to be substituted
					into the message text.

	dwCode1		-	This is a double word code that is to be converted to a
					string and substituted appropriately in the message text.

	dwCode2		-	This is a double word code that is to be converted to a
					string and substituted appropriately in the message text.


Return Value:

	None

--*/

{
    static USHORT wLogType;				// to hold event log type
    static TCHAR  *lpszEventString[3];	// array of strings to pass to event logger
    static BOOL   fReportEvent;			// return flag from report event

	lpszEventString[0] = new TCHAR[MAX_PATH+1];	// allocate space for string conversion
	lpszEventString[1] = new TCHAR[34];	// allocate space for string conversion
	lpszEventString[2] = new TCHAR[34];	// allocate space for string conversion

    if (hWriteEvent != NULL)			// if we have previous log access ability
    {
        if ( (lpszEventString[0] != (TCHAR *) NULL) &&
             (lpszEventString[1] != (TCHAR *) NULL) &&
             (lpszEventString[2] != (TCHAR *) NULL) )	// if storage allocated
        {
            wLogType = MessageType(nMsgNumber);	// get message type

            strcpy(lpszEventString[0],lpszText);		// copy the string
            _ultoa(dwCode1, lpszEventString[1], 10);	// convert to string
            _ultoa(dwCode2, lpszEventString[2], 10);	// convert to string

            fReportEvent = ReportEvent(	// write message
                hWriteEvent,			// handle to log file
                wLogType,				// message type
                0,						// message category
                nMsgNumber,				// message number
                NULL,					// user sid
                3,						// number of strings
                0,						// data length
                (const char **) lpszEventString,		// pointer to string array
                NULL);					// data address

            if ( !fReportEvent )		// did the event log okay?
            {							// not if we get here.....
                TraceWrite(TRUE, TRUE,	// write trace file record
					"WriteLog: Error writing to system event log is %lu\n",
                    GetLastError() );
                FormatTrace(nMsgNumber, lpszEventString);	// format trace information
            }
        }
        else							// if we can't allocate memory
        {
            TraceWrite(FALSE, TRUE,		// write trace file record
				"WriteLog: Error allocating memory for system event log write\n");
            FormatTrace(nMsgNumber, NULL);	// format trace information
        }
    }
    else								// if we can't write to system log
    {
        TraceWrite(FALSE, TRUE,			// write trace file record
			"WriteLog: Unable to write to system event log; handle is null\n");

        if ( (lpszEventString[0] != (TCHAR *) NULL) &&
             (lpszEventString[1] != (TCHAR *) NULL) &&
             (lpszEventString[2] != (TCHAR *) NULL) )	// if storage allocated
        {
            strcpy(lpszEventString[0],lpszText);		// copy the string
            _ultoa(dwCode1, lpszEventString[1], 10);	// convert to string
            _ultoa(dwCode2, lpszEventString[2], 10);	// convert to string
            FormatTrace(nMsgNumber, lpszEventString);	// format trace information
        }
        else							// if we can't allocate memory
        {
            TraceWrite(FALSE, TRUE,		// write trace file record
				"WriteLog: Error allocating memory for system event log write\n");
            FormatTrace(nMsgNumber, NULL);  // format trace information
        }
    }
	delete lpszEventString[0];			// free storage
	delete lpszEventString[1];			// free storage
	delete lpszEventString[2];			// free storage
    return;								// exit the function
}


VOID
WriteLog(
    IN NTSTATUS nMsgNumber,
    IN LPTSTR lpszText,
    IN DWORD dwCode
    )

/*++

Routine Description:

	WriteLog is called to write message text to the system event log. This is
	a C++ overloaded function. In case a log record cannot be written
	to the system event log, TraceWrite is called to write the appropriate
	message text to the trace file.


Arguments:

	nMsgNumber	-	This is the message number in SNMPELMG.H in NTSTATUS format
					that is to be written to the event log.

	lpszText	-	This contains a string parameter that is to be substituted
					into the message text.

	dwCode		-	This is a double word code that is to be converted to a
					string and substituted appropriately in the message text.


Return Value:

	None

--*/

{
    static USHORT wLogType;				// to hold event log type
    static TCHAR  *lpszEventString[2];	// array of strings to pass to event logger
    static BOOL   fReportEvent;			// return flag from report event

	lpszEventString[0] = new TCHAR[MAX_PATH+1];	// allocate space for string conversion
	lpszEventString[1] = new TCHAR[34];	// allocate space for string conversion

    if (hWriteEvent != NULL)			// if we have previous log access ability
    {
        if ( (lpszEventString[0] != (TCHAR *) NULL) &&
             (lpszEventString[1] != (TCHAR *) NULL) )	// if storage allocated
        {
            wLogType = MessageType(nMsgNumber);		// get message type

            strcpy(lpszEventString[0],lpszText);	// copy the string
            _ultoa(dwCode, lpszEventString[1], 10);	// convert to string

            fReportEvent = ReportEvent(	// write message
                hWriteEvent,			// handle to log file
                wLogType,				// message type
                0,						// message category
                nMsgNumber,				// message number
                NULL,					// user sid
                2,						// number of strings
                0,						// data length
                (const char **) lpszEventString,		// pointer to string array
                NULL);					// data address

            if ( !fReportEvent )		// did the event log okay?
            {							// not if we get here.....
                TraceWrite(TRUE, TRUE,	// write trace record
					"WriteLog: Error writing to system event log is %lu\n",
                    GetLastError() );
                FormatTrace(nMsgNumber, lpszEventString);	// format trace information
            }
        }
        else							// if we can't allocate memory
        {
            TraceWrite(FALSE, TRUE,		// write trace record
				"WriteLog: Error allocating memory for system event log write\n");
            FormatTrace(nMsgNumber, NULL);	// format trace information
        }
    }
    else								// if we can't write to system log
    {
        TraceWrite(FALSE, TRUE,			// write trace record
			"WriteLog: Unable to write to system event log; handle is null\n");

        if ( (lpszEventString[0] != (TCHAR *) NULL) &&
             (lpszEventString[1] != (TCHAR *) NULL) )	// if storage allocated
        {
            strcpy(lpszEventString[0],lpszText);		// copy the string
            _ultoa(dwCode, lpszEventString[1], 10);		// convert to string
            FormatTrace(nMsgNumber, lpszEventString);	// format trace information
        }
        else							// if we can't allocate memory
        {
            TraceWrite(FALSE, TRUE,		// write trace record
				"WriteLog: Error allocating memory for system event log write\n");
            FormatTrace(nMsgNumber, NULL);  // format trace information
        }
    }
	delete lpszEventString[0];			// free storage
	delete lpszEventString[1];			// free storage
    return;								// exit function
}


VOID
WriteLog(
    IN NTSTATUS nMsgNumber,
    IN LPTSTR lpszText
    )

/*++

Routine Description:

	WriteLog is called to write message text to the system event log. This is
	a C++ overloaded function. In case a log record cannot be written
	to the system event log, TraceWrite is called to write the appropriate
	message text to the trace file.


Arguments:

	nMsgNumber	-	This is the message number in SNMPELMG.H in NTSTATUS format
					that is to be written to the event log.

	lpszText	-	This contains a string parameter that is to be substituted
					into the message text.


Return Value:

	None

--*/

{
    static USHORT wLogType;				// to hold event log type
    static TCHAR  *lpszEventString[1];	// array of strings to pass to event logger
    static BOOL   fReportEvent;			// return flag from report event

	lpszEventString[0] = new TCHAR[MAX_PATH+1];	// allocate space for string conversion

    if (hWriteEvent != NULL)			// if we have previous log access ability
    {
        if ( lpszEventString[0] != (TCHAR *) NULL )	// if storage allocated
        {
            wLogType = MessageType(nMsgNumber);		// get message type

            strcpy(lpszEventString[0],lpszText);	// copy the string

            fReportEvent = ReportEvent(	// write message
                hWriteEvent,			// handle to log file
                wLogType,				// message type
                0,						// message category
                nMsgNumber,				// message number
                NULL,					// user sid
                1,						// number of strings
                0,						// data length
                (const char **) lpszEventString,		// pointer to string array
                NULL);					// data address

            if ( !fReportEvent )		// did the event log okay?
            {							// not if we get here.....
                TraceWrite(TRUE, TRUE,	// write trace file record
					"WriteLog: Error writing to system event log is %lu\n",
                    GetLastError() );
                FormatTrace(nMsgNumber, lpszEventString);	// format trace information
            }
        }
        else							// if we can't allocate memory
        {
            TraceWrite(FALSE, TRUE,		// write trace record
				"WriteLog: Error allocating memory for system event log write\n");
            FormatTrace(nMsgNumber, NULL);	// format trace information
        }
    }
    else								// if we can't write to system log
    {
        TraceWrite(FALSE, TRUE,			// write trace record
			"WriteLog: Unable to write to system event log; handle is null\n");

        if ( lpszEventString[0] != (TCHAR *) NULL )	// if storage allocated
        {
            strcpy(lpszEventString[0],lpszText);		// copy the string
            FormatTrace(nMsgNumber, lpszEventString);	// format trace information
        }
        else
        {
            TraceWrite(FALSE, TRUE,		// write trace record
				"WriteLog: Error allocating memory for system event log write\n");
            FormatTrace(nMsgNumber, NULL);	// format trace information
        }
    }
	delete lpszEventString[0];			// free storage
    return;								// exit function
}


VOID
WriteLog(
    IN	NTSTATUS	nMsgNumber,
    IN	LPCTSTR		lpszText1,
    IN	LPCTSTR		lpszText2
    )

/*++

Routine Description:

	WriteLog is called to write message text to the system event log. This is
	a C++ overloaded function. In case a log record cannot be written
	to the system event log, TraceWrite is called to write the appropriate
	message text to the trace file.


Arguments:

	nMsgNumber	-	This is the message number in SNMPELMG.H in NTSTATUS format
					that is to be written to the event log.

	lpszText	-	This contains a string parameter that is to be substituted
					into the message text.


Return Value:

	None

--*/

{
    static USHORT wLogType;				// to hold event log type
    static TCHAR  *lpszEventString[2];	// array of strings to pass to event logger
    static BOOL   fReportEvent;			// return flag from report event

	lpszEventString[0] = new TCHAR[MAX_PATH+1];	// allocate space for string conversion
	lpszEventString[1] = new TCHAR[MAX_PATH+1];	// allocate space for string conversion

    if (hWriteEvent != NULL)			// if we have previous log access ability
    {
        if ( (lpszEventString[0] != (TCHAR *) NULL ) &&
			 (lpszEventString[1] != (TCHAR *) NULL ) )	// if storage allocated
        {
            wLogType = MessageType(nMsgNumber);		// get message type

            strcpy(lpszEventString[0],lpszText1);	// copy the string
            strcpy(lpszEventString[1],lpszText2);	// copy the string

            fReportEvent = ReportEvent(	// write message
                hWriteEvent,			// handle to log file
                wLogType,				// message type
                0,						// message category
                nMsgNumber,				// message number
                NULL,					// user sid
                2,						// number of strings
                0,						// data length
                (const char **) lpszEventString,		// pointer to string array
                NULL);					// data address

            if ( !fReportEvent )		// did the event log okay?
            {							// not if we get here.....
                TraceWrite(TRUE, TRUE,	// write trace file record
					"WriteLog: Error writing to system event log is %lu\n",
                    GetLastError() );
                FormatTrace(nMsgNumber, lpszEventString);	// format trace information
            }
        }
        else							// if we can't allocate memory
        {
            TraceWrite(FALSE, TRUE,		// write trace record
				"WriteLog: Error allocating memory for system event log write\n");
            FormatTrace(nMsgNumber, NULL);	// format trace information
        }
    }
    else								// if we can't write to system log
    {
        TraceWrite(FALSE, TRUE,			// write trace record
			"WriteLog: Unable to write to system event log; handle is null\n");

        if ( (lpszEventString[0] != (TCHAR *) NULL) &&	// if storage allocated
			 (lpszEventString[1] != (TCHAR *) NULL ) )
        {
            strcpy(lpszEventString[0],lpszText1);		// copy the string
            strcpy(lpszEventString[1],lpszText2);		// copy the string
            FormatTrace(nMsgNumber, lpszEventString);	// format trace information
        }
        else
        {
            TraceWrite(FALSE, TRUE,		// write trace record
				"WriteLog: Error allocating memory for system event log write\n");
            FormatTrace(nMsgNumber, NULL);	// format trace information
        }
    }
	delete lpszEventString[0];			// free storage
	delete lpszEventString[1];			// free storage
    return;								// exit function
}


extern "C" {
VOID
WriteTrace(
    IN CONST UINT  nLevel,           // level of trace message
    IN CONST LPSTR szFormat,         // trace message to write
    IN ...                           // other printf type operands
    )

/*++

Routine Description:

	WriteTrace is called to write the requested trace information to the trace
	file specified in the configuration registry. The key to the trace file
	name is \SOFTWARE\Microsoft\SNMP_EVENTS\EventLog\Parameters\TraceFile.
	The registry information is only read for the first time WriteTrace is called.

	The TraceLevel parameter is also used to determine if the level of this
	message is part of a group of messages being traced. If the level of this
	message is greater than or equal the TraceLevel parameter, then this
	message will be sent to the file, otherwise the message is ignored.


Arguments:

	nLevel		-	This is the trace level of the message being logged.

	szFormat	-	This is the string text of the message to write to the
					trace file. This string is in the format of printf strings
					and will be formatted accordingly.


Return Value:

	None

--*/

{

    static	CHAR 	szBuffer[4096];
    static	TCHAR	szFile[MAX_PATH+1];
    static	DWORD	nFile = MAX_PATH+1;
    static	DWORD	dwLevel;
    static	DWORD	dwType;
    static	DWORD	nLvl = sizeof(DWORD);
    static	DWORD	status;
    static	HKEY	hkResult;
	static	DWORD	cbExpand;
			va_list arglist;

    if ( !fTraceFileName )           // if we haven't yet read registry
    {
        fTraceFileName = TRUE;       // set flag to not open registry info again
        if ( (status = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            EXTENSION_PARM,
            0,
            KEY_READ,
            &hkResult) ) != ERROR_SUCCESS)
        {
            WriteLog(SNMPELEA_NO_REGISTRY_PARAMETERS,status);    // write log/trace event record
        }
        else
        {
            if ( (status = RegQueryValueEx(         // look up trace file name
                hkResult,
                EXTENSION_TRACE_FILE,
                0,
                &dwType,
                (LPBYTE) szFile,
                &nFile) ) == ERROR_SUCCESS)
            {
				if (dwType != REG_SZ)		// we have a bad value.
				{
					WriteLog(SNMPELEA_REGISTRY_TRACE_FILE_PARAMETER_TYPE, szTraceFileName);  // write log/trace event record
				}
				else
					strcpy(szTraceFileName, szFile);
            }
            else
            {
                WriteLog(SNMPELEA_NO_REGISTRY_TRACE_FILE_PARAMETER,szTraceFileName);  // write log/trace event record
            }

            if ( (status = RegQueryValueEx(         // look up trace level
                hkResult,
                EXTENSION_TRACE_LEVEL,
                0,
                &dwType,
                (LPBYTE) &dwLevel,
                &nLvl) ) == ERROR_SUCCESS)
            {
                if (dwType == REG_DWORD)
                	nTraceLevel = dwLevel;	// copy registry trace level
                else
					WriteLog(SNMPELEA_REGISTRY_TRACE_LEVEL_PARAMETER_TYPE, nTraceLevel);  // write log/trace event record
            }
            else
            {
                WriteLog(SNMPELEA_NO_REGISTRY_TRACE_LEVEL_PARAMETER,nTraceLevel); // write log/trace event record
            }

            status = RegCloseKey(hkResult);

        } // end else registry lookup successful

    } // end Trace information registry processing

    // return if we are not supposed to trace this message
    if ( nLevel < nTraceLevel )      // are we tracing this message
    {
        return;                      // nope, just exit
    }

    // if the value could not be read from the registry (we still have the default value)
    // then we have no file name, so return.
    if (szTraceFileName[0] == TEXT('\0'))
        return;

   va_start(arglist, szFormat);
   vsprintf(szBuffer, szFormat, arglist);
   va_end(arglist);

   if (nLevel == MAXDWORD)
	{
		TraceWrite(FALSE, FALSE, szBuffer);
	}
	else
	{
		TraceWrite(FALSE, TRUE, szBuffer);
	}
}

}

