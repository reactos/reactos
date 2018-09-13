/*++

Copyright (c) 1994	Microsoft Corporation

Module Name:

	SNMPELPT.CPP


Abstract:

	This routine is the event log processing thread for the SNMP Event Log Agent DLL.
	The function of this routine is to wait for an event to occur, as indicated by an
	event log record, check the registry to determine if the event is being tracked,
	then to return a buffer to the processing agent DLL indicating that an SNMP trap
	should be sent to the extension agent. An event is posted complete when a buffer is
	built and ready for trap processing.

	In order to maintain data integrity between this thread and the processing agent
	thread, a MUTEX object is used to synchronize access to the trap buffer queue. If
	an error occurs, an event log message and trace records are written to indicate the
	problem and the event is ignored.

	When the extension agent is terminated, the processing agent DLL receives control
	in the process detach routine. An event is posted complete to indicate to this thread
	that processing should be terminated and all event logs should be closed.

Author:

	Randy G. Braze	Created 16 October 1994


Revision History:

	7 Feb 96	Restructured building of varbinds to be outside of trap generation.
				Calculated trap buffer length correctly.
				Created varbind queue and removed event log buffer queue.

	28 Feb 96	Added code to support a performance threshold reached indicator.
				Removed inclusion of base OID information from varbind OIDs.
				Added conversion from OEM to current code page for varbind data.
				Removed pointer references to varbindlist and enterpriseoid.
				Fixed memory leak for not freeing storage arrays upon successful build of trap.

	10 Mar 96	Removed OemToChar coding and registry checking.
				Modifications to read log file names from EventLog registry entries and not
				from specific entries in the SNMP Extension Agent's registry entries.
				Included SnmpMgrStrToOid as an internal function, as opposed to using the function
				provided by MGMTAPI.DLL. SNMPTRAP.EXE will be called if MGMTAPI is called, which
				will disable other agents from being able to receive any traps. All references
				to MGMTAPI.DLL and MGMTAPI.H will be removed.
				Added a ThresholdEnabled flag to the registry to indicate if the threshold values
				were to be monitored or ignored.

	15 Mar 96	Modified to move the sources for the eventlog in the registry down below a new
				key called Sources.

	07 May 96	Removed SnmpUtilOidFree and use two SNMP_free. One for the OID's ids array and
				one for the OID itself.

	22 May 96	Edited FreeVarBind to make sure we only freed memory we allocated.

	26 Jun 96	Added code to make sure message dlls were not loaded and unloaded (leaks) just have
				a list of handles to the loaded dlls and free them at the end. Also plugged some other
				memory leaks. Added a function to make sure the CountTable is kept tidy.


--*/

extern "C" {
#include <windows.h>		// basic windows applications information
#include <winperf.h>
#include <stdlib.h>
#include <malloc.h> 		// needed for memory allocations
#include <string.h> 		// string stuff
#include <snmp.h>			// snmp stuff
// #include <mgmtapi.h> 	// snmp mgr definitions
#include <TCHAR.H>
#include <time.h>

#include "snmpelea.h"		// global dll definitions
#include "snmpelpt.h"		// module specific definitions
#include "snmpelmg.h"		// message definitions
}

#include "snmpelep.h"		// c++ definitions and variables
extern	BOOL				StrToOid(PCHAR str, AsnObjectIdentifier *oid);

void
TidyCountTimeTable(
	IN		LPTSTR		lpszLog,			// pointer to log file name
	IN		LPTSTR		lpszSource, 		// pointer to source of event
	IN		DWORD		nEventID			// event ID
	)

/*++

Routine Description:

	TidyCountTimeTable is called to remove items from the lpCountTable which no longer
	have a count greater than 1.


Arguments:

	lpszLog 	-	Pointer to the log file for this event.

	lpszSource	-	Pointer to source for this event.

	nEventID	-	Event ID.


Return Value:

	None.
	

--*/

{
	PCOUNTTABLE lpTable;				// temporary fields
	PCOUNTTABLE lpPrev;

	WriteTrace(0x0a,"TidyCountTimeTable: Entering TidyCountTimeTable routine\n");
	
	if (lpCountTable == NULL)
	{
		WriteTrace(0x0a,"TidyCountTimeTable: Empty table, exiting TidyCountTimeTable\n");
		return;
	}

	// if we get here, then a table exists and must be scanned for a current entry

	lpTable = lpCountTable; 						// start with first table pointer
	lpPrev = NULL;									// set previous to NULL

	while (TRUE)
	{
		WriteTrace(0x0a,"TidyCountTimeTable: Checking entry %08X\n", lpTable);

		if ((strcmp(lpTable->log,lpszLog) != 0) ||
			(strcmp(lpTable->source,lpszSource) != 0) ||
			(lpTable->event != nEventID)
			)
		{
			if (lpTable->lpNext == NULL)
			{
				WriteTrace(0x0a,"TidyCountTimeTable: Entry not found\n");		
				break;
			}

			lpPrev = lpTable;
			lpTable = lpTable->lpNext;				// point to next entry
			continue;								// continue the loop
		}

		if (lpPrev == NULL)
		{
			WriteTrace(0x0a,"TidyCountTimeTable: Freeing first entry in lpCountTable at %08X\n", lpTable);
			lpCountTable = lpCountTable->lpNext;
			SNMP_free(lpTable);
		}
		else
		{
			WriteTrace(0x0a,"TidyCountTimeTable: Freeing entry in lpCountTable at %08X\n", lpTable);
			lpPrev->lpNext = lpTable->lpNext;
			SNMP_free(lpTable);
		}

		break;
	}
	
	WriteTrace(0x0a,"TidyCountTimeTable: Exiting TidyCountTimeTable\n");
	return;
}

BOOL
CheckCountTime(
	IN		LPTSTR		lpszLog,			// pointer to log file name
	IN		LPTSTR		lpszSource, 		// pointer to source of event
	IN		DWORD		nEventID,			// event ID
	IN		DWORD		dwTime, 			// time of event
	IN		PREGSTRUCT	regStruct			// pointer to registry structure
	)

/*++

Routine Description:

	CheckCountTime is called to determine if a specific event with count and/or time
	values specified in the registry have met the indicated criteria. If an entry does
	not exist in the current table of entries, a new entry is added for later tracking.


Arguments:

	lpszLog 	-	Pointer to the log file for this event.

	lpszSource	-	Pointer to source for this event.

	nEventID	-	Event ID.

	regStruct	-	Pointer to a structure where data read from the registry is provided.


Return Value:

	TRUE	-	If a trap should be sent. Count and/or time value criteria satisified.

	FALSE	-	If no trap should be sent.


--*/

{
	PCOUNTTABLE lpTable;				// temporary field
	DWORD		dwTimeDiff = 0; 			// temporary field

	WriteTrace(0x0a,"CheckCountTime: Entering CheckCountTime routine\n");
	if (lpCountTable == NULL)
	{
		WriteTrace(0x0a,"CheckCountTime: Count/Time table is currently empty. Adding entry.\n");
		lpCountTable = (PCOUNTTABLE) SNMP_malloc(sizeof(COUNTTABLE));
		if (lpCountTable == NULL)
		{
			WriteTrace(0x14,"CheckCountTime: Unable to acquire storage for Count/Time table entry.\n");
			WriteLog(SNMPELEA_COUNT_TABLE_ALLOC_ERROR);
			return(FALSE);
		}
		lpCountTable->lpNext = NULL;				// set forward pointer to null
		strcpy(lpCountTable->log,lpszLog);			// copy log file name to table
		strcpy(lpCountTable->source,lpszSource);	// copy source name to table
		lpCountTable->event = nEventID; 			// copy event id to table
		lpCountTable->curcount = 0; 				// set table count to 0
		lpCountTable->time = dwTime;				// set table time to event time
		WriteTrace(0x0a,"CheckCountTime: New table entry is %08X\n", lpCountTable);
	}

	// if we get here, then a table exists and must be scanned for a current entry

	lpTable = lpCountTable; 						// start with first table pointer

	while (TRUE)
	{
		WriteTrace(0x0a,"CheckCountTime: Checking entry %08X\n", lpTable);

		if ((strcmp(lpTable->log,lpszLog) != 0) ||
			(strcmp(lpTable->source,lpszSource) != 0) ||
			(lpTable->event != nEventID)
			)
		{
			if (lpTable->lpNext == NULL)
			{
				break;
			}
			lpTable = lpTable->lpNext;				// point to next entry
			continue;								// continue the loop
		}

		dwTimeDiff = dwTime - lpTable->time;		// compute elapsed time in seconds

		WriteTrace(0x0a,"CheckCountTime: Entry information located in table at %08X\n", lpTable);
		WriteTrace(0x00,"CheckCountTime: Entry count value is %lu\n",lpTable->curcount);
		WriteTrace(0x00,"CheckCountTime: Entry last time value is %08X\n",lpTable->time);
		WriteTrace(0x00,"CheckCountTime: Entry current time value is %08X\n",dwTime);
		WriteTrace(0x00,"CheckCountTime: Time difference is %lu\n",dwTimeDiff);
		WriteTrace(0x00,"CheckCountTime: Registry count is %lu, time is %lu\n",
			regStruct->nCount, regStruct->nTime);

		if (regStruct->nTime)
		{
			WriteTrace(0x0a,"CheckCountTime: Time value is being checked\n");
			if (dwTimeDiff > regStruct->nTime)
			{
				WriteTrace(0x0a,"CheckCountTime: Specified time parameters exceeded for entry. Resetting table information.\n");
				lpTable->time = dwTime; 				// reset time field
				lpTable->curcount = 1;					// reset count field
				WriteTrace(0x0a,"CheckCountTime: Exiting CheckCountTime with FALSE\n");
				return(FALSE);
			}
		}

		if (++lpTable->curcount >= regStruct->nCount)
		{
			WriteTrace(0x0a,"CheckCountTime: Count field has been satisfied for entry\n");
			lpTable->curcount = 0;						// reset count field for event
			lpTable->time = dwTime; 					// reset time field
			WriteTrace(0x0a,"CheckCountTime: Exiting CheckCountTime with TRUE\n");
			return(TRUE);
		}
		else
		{
			WriteTrace(0x0a,"CheckCountTime: Count field not satisfied for entry\n");
			WriteTrace(0x0a,"CheckCountTime: Exiting CheckCountTime with FALSE\n");
			return(FALSE);
		}
	}

	// if we get here, then a table entry does not exist for the current entry

	lpTable->lpNext = (PCOUNTTABLE) SNMP_malloc(sizeof(COUNTTABLE));	// allocate storage for new entry
	lpTable = lpTable->lpNext;				// set table pointer

	if (lpCountTable == NULL)
	{
		WriteTrace(0x14,"CheckCountTime: Unable to acquire storage for Count/Time table entry.\n");
		WriteLog(SNMPELEA_COUNT_TABLE_ALLOC_ERROR);
		return(FALSE);
	}

	lpTable->lpNext = NULL; 				// set forward pointer to NULL
	strcpy(lpTable->log,lpszLog);			// copy log file name to table
	strcpy(lpTable->source,lpszSource); 	// copy source name to table
	lpTable->event = nEventID;				// copy event id to table
	lpTable->curcount = 0;					// set table count to 0
	lpTable->time = dwTime; 				// set table time to event time
	WriteTrace(0x0a,"CheckCountTime: New table entry added at %08X\n", lpTable);

	if (regStruct->nTime)
	{
		WriteTrace(0x0a,"CheckCountTime: Time value is being checked\n");
		if (dwTimeDiff > regStruct->nTime)
		{
			WriteTrace(0x0a,"CheckCountTime: Specified time parameters exceeded for entry. Resetting table information.\n");
			lpTable->time = dwTime; 				// reset time field
			lpTable->curcount = 1;					// reset count field
			WriteTrace(0x0a,"CheckCountTime: Exiting CheckCountTime with FALSE\n");
			return(FALSE);
		}
	}

	if (++lpTable->curcount >= regStruct->nCount)
	{
		WriteTrace(0x0a,"CheckCountTime: Count field has been satisfied for entry\n");
		lpTable->curcount = 0;						// reset count field for event
		lpTable->time = dwTime; 					// reset time field
		WriteTrace(0x0a,"CheckCountTime: Exiting CheckCountTime with TRUE\n");
		return(TRUE);
	}
	else
	{
		WriteTrace(0x0a,"CheckCountTime: Count field not satisfied for entry\n");
		WriteTrace(0x0a,"CheckCountTime: Exiting CheckCountTime with FALSE\n");
		return(FALSE);
	}

//	default exit point (should never occur)

	WriteTrace(0x0a,"CheckCountTime: Exiting CheckCountTime with FALSE\n");
	return(FALSE);
}

BOOL
GetRegistryValue(
	IN		LPTSTR		sourceName, 		// source name for event
	IN		LPTSTR		eventID,			// event ID for event
	IN		LPTSTR		logFile,			// log file of event
	IN		DWORD		timeGenerated,		// time this event was generated
	IN	OUT PREGSTRUCT	regStruct			// pointer to registry structure to return
	)

/*++

Routine Description:

	GetRegistryValue is called to read a specific key value from the system registry.


Arguments:

	sourceName		-	Specifies the source name from the event log.

	eventID 		-	This the event ID from the event log record.

	regStruct		-	Pointer to a structure where data will be returned from the registry.


Return Value:

	TRUE	-	If a registry entry is located and all parameters could be read.

	FALSE	-	If no registry entry exists or some other error occurs.


--*/

{
	LONG	status; 					// registry read results
	HKEY	hkResult;					// handle returned from API
	DWORD	iValue; 					// temporary counter
	DWORD	dwType; 					// type of the parameter read
	DWORD	nameSize;					// length of parameter name
	DWORD	nReadBytes = 0; 			// number of bytes read from profile information
	LPTSTR	lpszSourceKey;				// temporary string for registry source key
	LPTSTR	lpszEventKey;				// temporary string for registry event key
	TCHAR	temp[2*MAX_PATH+1]; 		// temporary string

	WriteTrace(0x0a,"GetRegistryValue: Entering GetRegistryValue function\n");

	if (fThresholdEnabled && fThreshold)
	{
		WriteTrace(0x0a,"GetRegistryValue: Performance threshold flag is on. No data will be processed.\n");
		WriteTrace(0x0a,"GetRegistryValue: Exiting GetRegistryValue function with FALSE\n");
		return(FALSE);
	}

	if ( (lpszSourceKey = (LPTSTR) SNMP_malloc(strlen(EXTENSION_SOURCES)+strlen(sourceName)+2)) == NULL )
	{
		WriteTrace(0x14,"GetRegistryValue: Unable to allocate registry source key storage. Trap not sent.\n");
		WriteTrace(0x0a,"GetRegistryValue: Exiting GetRegistryValue function with FALSE\n");
		return(FALSE);
	}

	if ( (lpszEventKey = (LPTSTR) SNMP_malloc(strlen(EXTENSION_SOURCES)+strlen(sourceName)+strlen(eventID)+3)) == NULL )
	{
		WriteTrace(0x14,"GetRegistryValue: Unable to allocate registry event key storage. Trap not sent.\n");
		SNMP_free(lpszSourceKey);
		WriteTrace(0x0a,"GetRegistryValue: Exiting GetRegistryValue function with FALSE\n");
		return(FALSE);
	}

	strcpy(lpszSourceKey,EXTENSION_SOURCES);	// start with root
	strcat(lpszSourceKey,sourceName);			// append the source name
	strcpy(lpszEventKey,lpszSourceKey); 		// build prefix for event key
	strcat(lpszEventKey,TEXT("\\"));			// add the backslash
	strcat(lpszEventKey,eventID);				// complete it with the event ID

	WriteTrace(0x00,"GetRegistryValue: Opening registry key for %s\n",lpszEventKey);

	if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpszEventKey, 0,
		(KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS), &hkResult))
		!= ERROR_SUCCESS)					// open registry information
	{
		WriteTrace(0x00,"GetRegistryValue: No registry entry exists for %s. RegOpenKeyEx returned %lu\n",
			lpszEventKey, status);
		SNMP_free(lpszSourceKey);			// free storage
		SNMP_free(lpszEventKey);				// free storage
		WriteTrace(0x0a,"GetRegistryValue: Exiting GetRegistryValue function with FALSE\n");
		return(FALSE);					// show nothing exists
	}

	nameSize = sizeof(iValue);			// set field length
	if ( (status = RegQueryValueEx( 	// look up count
		hkResult,						// handle to registry key
		EXTENSION_COUNT,				// key to look up
		0,								// ignored
		&dwType,						// address to return type value
		(LPBYTE) &iValue,				// where to return count field
		&nameSize) ) != ERROR_SUCCESS)	// size of count field
	{
		WriteTrace(0x00,"GetRegistryValue: No registry entry exists for %s. RegOpenKeyEx returned %lu\n",
			EXTENSION_COUNT, status);
		regStruct->nCount = 0;			// set default value
	}
	else
	{
		regStruct->nCount = iValue; 	// save returned value
		WriteTrace(0x00,"GetRegistryValue: Count field is %lu\n", regStruct->nCount);
	}

	if ( (status = RegQueryValueEx( 	// look up local trim
		hkResult,						// handle to registry key
		EXTENSION_TRIM, 				// key to look up
		0,								// ignored
		&dwType,						// address to return type value
		(LPBYTE) &iValue,				// where to return count field
		&nameSize) ) != ERROR_SUCCESS)	// size of count field
	{
		WriteTrace(0x00,"GetRegistryValue: No registry entry exists for %s. RegOpenKeyEx returned %lu\n",
			EXTENSION_TRIM, status);
		WriteTrace(0x00,"GetRegistryValue: Using default of global trim message flag of %lu\n",
			fGlobalTrim);
		regStruct->fLocalTrim = fGlobalTrim;	// set default value
	}
	else
	{
		regStruct->fLocalTrim = ((iValue == 1) ? TRUE : FALSE); // save returned value
		WriteTrace(0x00,"GetRegistryValue: Local message trim field is %lu\n", regStruct->fLocalTrim);
	}

	if ( (status = RegQueryValueEx( 	// look up time
		hkResult,						// handle to registry key
		EXTENSION_TIME, 				// key to look up
		0,								// ignored
		&dwType,						// address to return type value
		(LPBYTE) &iValue,				// where to return time field
		&nameSize) ) != ERROR_SUCCESS)	// size of time field
	{
		WriteTrace(0x00,"GetRegistryValue: No registry entry exists for %s. RegOpenKeyEx returned %lu\n",
			EXTENSION_TIME, status);
		regStruct->nTime = 0;			// set default value
	}
	else
	{
		regStruct->nTime = iValue;		// save returned value
		WriteTrace(0x00,"GetRegistryValue: Time field is %lu\n", regStruct->nTime);
	}

	RegCloseKey(hkResult);				// close registry key for event
	SNMP_free(lpszEventKey);					// free the storage for the event key

	WriteTrace(0x00,"GetRegistryValue: Opening registry key for %s\n",lpszSourceKey);

	if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpszSourceKey, 0,
		(KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS), &hkResult))
		!= ERROR_SUCCESS)					// open registry information
	{
		WriteTrace(0x00,"GetRegistryValue: No registry entry exists for %s. RegOpenKeyEx returned %lu\n",
			lpszSourceKey, status);
		SNMP_free(lpszSourceKey);			// free storage
		WriteTrace(0x0a,"GetRegistryValue: Exiting GetRegistryValue function with FALSE\n");
		return(FALSE);					// show nothing exists
	}

	nameSize = sizeof(regStruct->szOID)-1;	// set field length
	if ( (status = RegQueryValueEx( 	// look up EnterpriseOID
		hkResult,						// handle to registry key
		EXTENSION_ENTERPRISE_OID,		// key to look up
		0,								// ignored
		&dwType,						// address to return type value
		(LPBYTE) regStruct->szOID,		// where to return OID string field
		&nameSize) ) != ERROR_SUCCESS)	// size of OID string field
	{
		WriteTrace(0x00,"GetRegistryValue: No registry entry exists for %s. RegOpenKeyEx returned %lu\n",
			EXTENSION_ENTERPRISE_OID, status);
		SNMP_free(lpszSourceKey);			// free storage
		RegCloseKey(hkResult);			// close the registry key
		WriteTrace(0x0a,"GetRegistryValue: Exiting GetRegistryValue function with FALSE\n");
		return(FALSE);					// indicate error
	}

	WriteTrace(0x00,"GetRegistryValue: EnterpriseOID field is %s\n", regStruct->szOID);

	nameSize = sizeof(iValue);			// set field length
	if ( (status = RegQueryValueEx( 	// look up time
		hkResult,						// handle to registry key
		EXTENSION_APPEND,				// key to look up
		0,								// ignored
		&dwType,						// address to return type value
		(LPBYTE) &iValue,				// where to return time field
		&nameSize) ) != ERROR_SUCCESS)	// size of time field
	{
		WriteTrace(0x00,"GetRegistryValue: No registry entry exists for %s. RegOpenKeyEx returned %lu\n",
			EXTENSION_APPEND, status);
		regStruct->fAppend = TRUE;		// default to true
	}
	else
	{
		regStruct->fAppend = ((iValue == 1) ? TRUE : FALSE);		// reflect append flag
		WriteTrace(0x00,"GetRegistryValue: Append field is %lu\n", regStruct->fAppend);
	}

	RegCloseKey(hkResult);				// close registry key for source
	SNMP_free(lpszSourceKey);				// free the storage for the source key

	if (regStruct->fAppend)
	{
		strcpy(temp,regStruct->szOID);							// copy enterprise suffix temporarily
		strcpy(regStruct->szOID, szBaseOID);					// copy base enterprise oid first
		strcpy(regStruct->szOID+strlen(szBaseOID), TEXT("."));	// add the .
		strcpy(regStruct->szOID+strlen(szBaseOID)+1, temp); 	// now add the suffix
		WriteTrace(0x0a,"GetRegistryValue: Appended enterprise OID is %s\n", regStruct->szOID);
	}

	if ((regStruct->nCount > 1) || regStruct->nTime)
	{
		WriteTrace(0x0a,"GetRegistryValue: Values found for Count and/or Time for this entry\n");

		if (regStruct->nCount == 0)
		{
			regStruct->nCount = 2;		// set a default value of 2
		}

		if (!CheckCountTime(logFile, sourceName, atol(eventID), timeGenerated, regStruct))
		{
			WriteTrace(0x0a,"GetRegistryValue: Count/Time values not met for this entry\n");
			WriteTrace(0x0a,"GetRegistryValue: Exiting ReadRegistryValue with FALSE\n");
			return(FALSE);				// indicate nothing to send
		}
	}
	else
	{
		TidyCountTimeTable(logFile, sourceName, atol(eventID));
	}

	WriteTrace(0x0a,"GetRegistryValue: Exiting ReadRegistryValue with TRUE\n");
	return(TRUE);						// indicate got all of the data
}


VOID
StopAll(
	 IN VOID
	 )

/*++

Routine Description:

	This routine is called to write trace and log records and notify the
	other DLL threads that this thread is terminating.

Arguments:

	None

Return Value:

	None


--*/

{
	LONG	lastError;				// for GetLastError()

	WriteTrace(0x0a,"StopAll: Signaling DLL shutdown event %08X from Event Log Processing thread.\n",
		hStopAll);

	if ( !SetEvent(hStopAll) )
	{
		lastError = GetLastError(); // save error code status
		WriteTrace(0x14,"StopAll: Error signaling DLL shutdown event %08X in SNMPELPT; code %lu\n",
			hStopAll, lastError);
		WriteLog(SNMPELEA_ERROR_SET_AGENT_STOP_EVENT,
			HandleToUlong(hStopAll), lastError);  // log error message
	}
}


VOID
DoExitLogEv(
	 IN DWORD dwReturn
	)

/*++

Routine Description:

	This routine is called to write trace and log records when SnmpEvLogProc is
	terminating.

Arguments:

	dwReturn	-	Value to return in ExitThread.

Return Value:

	None

Notes:

	ExitThread is used to return control to the caller. A return code of 1 is
	supplied to indicate that a problem was encountered. A return code of 0
	is supplied to indicate that no problems were encountered.

--*/

{
	PCOUNTTABLE lpTable;			// pointer to count table address

	if (dwReturn)
	{
		WriteTrace(0x14,"DoExitLogEv: SnmpEvLogProc has encountered an error.\n");
	}

	if (lpCountTable != NULL)
	{
		WriteTrace(0x0a,"DoExitLogEv: Count/Time table has storage allocated. Freeing table.\n");
		lpTable = lpCountTable; 	// start at first entry

		while (lpCountTable != NULL)
		{
			WriteTrace(0x00,"DoExitLogEv: Freeing Count/Time table entry at %08X\n", lpCountTable);
			lpTable = lpCountTable->lpNext; 			// get pointer to next entry
			SNMP_free(lpCountTable);							// free this storage
			lpCountTable = lpTable; 					// set to next entry
		}
	}

	WriteTrace(0x0a,"DoExitLogEv: Exiting SnmpEvLogProc routine.....\n");
	ExitThread(dwReturn);
}


VOID
CloseEvents(
	 IN PHANDLE phWaitEventPtr
	 )

/*++

Routine Description:

	This routine is called to close event handles that are open and to free
	the storage currently allocated to those handles.

Arguments:

	phWaitEventPtr	-	This is the pointer to the array of event handles used
						for notification of a log event.

Return Value:

	None


--*/

{
	UINT	i;						// temporary loop counter
	LONG	lastError;				// last API error code

	for (i = 0; i < uNumEventLogs; i++)
	{
		WriteTrace(0x0a,"CloseEvents: Closing handle for wait event %lu - %08X\n",
			i, *(phWaitEventPtr+i));

		if ( !CloseHandle(*(phWaitEventPtr+i)) )
		{
			lastError = GetLastError(); 	// save error status
			WriteTrace(0x14,"CloseEvents: Error closing event handle %08X is %lu\n",
				*(phWaitEventPtr+i), lastError);	// trace error message
			WriteLog(SNMPELEA_ERROR_CLOSE_WAIT_EVENT_HANDLE,
				HandleToUlong(*(phWaitEventPtr+i)), lastError);	// trace error message
		}
	}

	WriteTrace(0x0a,"CloseEvents: Freeing memory for wait event list %08X\n",
		phWaitEventPtr);
	SNMP_free( (LPVOID) phWaitEventPtr );		 // Free the memory
}


BOOL
ReopenLog(
	IN DWORD	dwOffset,		// offset into event handle array
	IN PHANDLE	phWaitEventPtr	// event handle array pointer
	)

/*++

Routine Description:

	This routine is called to close and reopen an event log that has been
	cleared. When this happens, the handle becomes invalid and the log must
	be reopened and the NotifyChangeEventLog API must be called again.

Arguments:

	dwOffset	-	This field contains the index into the handle pointer
					array of the currently invalid handle. This invalid
					handle will be replaced with the valid handle if the
					function is successful.

Return Value:

	TRUE	-	If the log was successfully reopened and a new NotifyChangeEventLog
				was issued successfully.

	FALSE	-	If the log could not be opened or the NotifyChangeEventLog failed.


--*/

{
	HANDLE		hLogHandle; 		// temporary for log file handle
	LPTSTR		lpszLogName;		// name of this log file
	LONG		lastError;			// temporary for GetLastError;

	hLogHandle = *(phEventLogs+dwOffset);	// load the current handle
	lpszLogName = lpszEventLogs+dwOffset*(MAX_PATH+1);

	WriteTrace(0x14,"ReopenLog: Log file %s has been cleared; reopening log\n",
		lpszLogName);

	CloseEventLog(hLogHandle);	// first, close old handle

	hLogHandle = OpenEventLog( (LPTSTR) NULL, lpszLogName);

	if (hLogHandle == NULL)
	{						  // did log file open?
		lastError = GetLastError(); // save error code
		WriteTrace(0x14,"ReopenLog: Error in EventLogOpen for %s = %lu \n",
			lpszLogName, lastError);

		WriteLog(SNMPELEA_ERROR_OPEN_EVENT_LOG, lpszLogName, lastError);  // log the error message
		return(FALSE);				  // failed -- forget this one
	}

	WriteTrace(0x00,"ReopenLog: New handle for %s is %08X\n",
		lpszLogName, hLogHandle);
	*(phEventLogs+dwOffset) = hLogHandle;	// save new handle now

	WriteTrace(0x00,"ReopenLog: Reissuing NotifyChangeEventLog for log\n");
	if (!NotifyChangeEventLog(*(phEventLogs+dwOffset),
		*(phWaitEventPtr+dwOffset)))
	{
		lastError = GetLastError();
		WriteTrace(0x14,"ReopenLog: NotifyChangeEventLog failed with code %lu\n",
			lastError);
		WriteLog(SNMPELEA_ERROR_LOG_NOTIFY, lastError); // log error message
		return(FALSE);
	}

	WriteTrace(0x00,"ReopenLog: ChangeNotify was successful\n");
	return(TRUE);
}


VOID
DisplayLogRecord(
	IN PEVENTLOGRECORD	pEventBuffer,
	IN DWORD			dwSize,
	IN DWORD			dwNeeded
	)

/*++

Routine Description:

	This routine is called to display the event log record after reading it.

Arguments:

	pEventBuffer	-	This is a pointer to an EVENTLOGRECORD structure
						containing the current event log record.

	dwSize			-	Contains the size in bytes of the amount of data
						just read into the buffer specified on the
						ReadEventLog.

	dwNeeded		-	Contains the size in bytes of the amount of storage
						required to read the next log record if GetLastError()
						returns ERROR_INSUFFICIENT_BUFFER.

Return Value:

	None

--*/

{
	PCHAR	pcString;				// temporary string pointer
	UINT	j;						// temporary loop counter

	if (nTraceLevel)				// if not maximum tracing
	{
		return; 					// just get out
	}

	WriteTrace(0x00,"DisplayLogRecord: Values from ReadEventLog follow:\n");
	WriteTrace(0x00,"DisplayLogRecord: EventSize = %lu EventNeeded = %lu\n",
		dwSize, dwNeeded);

	WriteTrace(0x00,"DisplayLogRecord: Event Log Buffer contents follow:\n");
	WriteTrace(0x00,"DisplayLogRecord: Length = %lu Record Number = %lu\n",
		pEventBuffer->Length, pEventBuffer->RecordNumber);
	WriteTrace(0x00,"DisplayLogRecord: Time generated = %08X Time written = %08X\n",
		pEventBuffer->TimeGenerated, pEventBuffer->TimeWritten);
	WriteTrace(0x00,"DisplayLogRecord: Event ID = %lu (%08X) Event Type = %04X\n",
		pEventBuffer->EventID, pEventBuffer->EventID, pEventBuffer->EventType);
	WriteTrace(0x00,"DisplayLogRecord: Num Strings = %lu EventCategory = %04X\n",
		pEventBuffer->NumStrings, pEventBuffer->EventCategory);
	WriteTrace(0x00,"DisplayLogRecord: String Offset = %lu Data Length = %lu\n",
		pEventBuffer->StringOffset, pEventBuffer->DataLength);
	WriteTrace(0x00,"DisplayLogRecord: Data Offset = %lu\n",
		pEventBuffer->DataOffset);

	pcString = (PCHAR) pEventBuffer + EVENTRECSIZE;
	WriteTrace(0x00,"DisplayLogRecord: EventBuffer address is %08X\n", pEventBuffer);
	WriteTrace(0x00,"DisplayLogRecord: EVENTRECSIZE is %lu\n",EVENTRECSIZE);

	WriteTrace(0x00,"DisplayLogRecord: String pointer is assigned address %08X\n",
		pcString);
	WriteTrace(0x00,"DisplayLogRecord: SourceName[] = %s\n", pcString);
	pcString += strlen(pcString) + 1;

	WriteTrace(0x00,"DisplayLogRecord: Computername[] = %s\n", pcString);
	pcString = (PCHAR) pEventBuffer + pEventBuffer->StringOffset;

	WriteTrace(0x00,"DisplayLogRecord: String pointer is assigned address %08X\n",
		pcString);
	for (j = 0; j < pEventBuffer->NumStrings; j++)
	{
		WriteTrace(0x00,"DisplayLogRecord: String #%lu ->%s\n", j, pcString);
		pcString += strlen(pcString) + 1;
	}
}


BOOL
AddBufferToQueue(
	 IN PVarBindQueue	lpVarBindEntry	// pointer to varbind entry structure
	 )

/*++

Routine Description:

	This routine will add a varbind entry to the queue of traps to send.


Arguments:

	lpVarBindEntry	-	This is a pointer to a varbind entry.

Return Value:

	TRUE	-	The varbind entry was successfully added to the queue.

	FALSE	-	The varbind entry could not be added to the queue.

Notes:


--*/

{
	PVarBindQueue	pBuffer;		// temporary pointer
	HANDLE			hWaitList[2];	// wait event array
	LONG			lastError;		// for GetLastError()
	DWORD			status; 		// for wait

	WriteTrace(0x0a,"AddBufferToQueue: Entering AddBufferToQueue function\n");

	if (fThresholdEnabled && fThreshold)
	{
		WriteTrace(0x0a,"AddBufferToQueue: Performance threshold flag is on. No data will be processed.\n");
		WriteTrace(0x0a,"AddBufferToQueue: Exiting AddBufferToQueue function with FALSE\n");
		return(FALSE);
	}

	WriteTrace(0x00,"AddBufferToQueue: Current buffer pointer is %08X\n", lpVarBindQueue);
	WriteTrace(0x00,"AddBufferToQueue: Adding buffer address %08X to queue\n", lpVarBindEntry);

	hWaitList[0] = hMutex;				// mutex handle
	hWaitList[1] = hStopAll;			// DLL termination event handle

	WriteTrace(0x00,"AddBufferToQueue: Handle to Mutex object is %08X\n", hMutex);
	WriteTrace(0x0a,"AddBufferToQueue: Waiting for Mutex object to become available\n");

	while (TRUE)
	{
		status = WaitForMultipleObjects(
			2,								// only two objects to wait on
			(CONST PHANDLE) &hWaitList, 	// address of array of event handles
			FALSE,							// only one event is required
			1000);							// only wait one second

		lastError = GetLastError(); 		// save any error conditions
		WriteTrace(0x0a,"AddBufferToQueue: WaitForMulitpleObjects returned a value of %lu\n", status);

		switch (status)
		{
			case WAIT_FAILED:
				WriteTrace(0x14,"AddBufferToQueue: Error waiting for mutex event array is %lu\n",
					lastError); 				// trace error message
				WriteLog(SNMPELEA_ERROR_WAIT_ARRAY, lastError); // log error message
				WriteTrace(0x0a,"AddBufferToQueue: Exiting AddBufferToQueue routine with FALSE\n");
				return(FALSE);					// get out now
			case WAIT_TIMEOUT:
				WriteTrace(0x0a,"AddBufferToQueue: Mutex object not available yet. Wait will continue.\n");
				continue;						// retry the wait
			case WAIT_ABANDONED:
				WriteTrace(0x14,"AddBufferToQueue: Mutex object has been abandoned.\n");
				WriteLog(SNMPELEA_MUTEX_ABANDONED);
				WriteTrace(0x0a,"AddBufferToQueue: Exiting AddBufferToQueue routine with FALSE\n");
				return(FALSE);					// get out now
			case 1:
				WriteTrace(0x0a,"AddBufferToQueue: DLL shutdown detected. Wait abandoned.\n");
				WriteTrace(0x0a,"AddBufferToQueue: Exiting AddBufferToQueue routine with FALSE\n");
				return(FALSE);
			case 0:
				WriteTrace(0x0a,"AddBufferToQueue: Mutex object acquired.\n");
				break;
			default:
				WriteTrace(0x14,"AddBufferToQueue: Undefined error encountered in WaitForMultipleObjects. Wait abandoned.\n");
				WriteLog(SNMPELEA_ERROR_WAIT_UNKNOWN);
				WriteTrace(0x0a,"AddBufferToQueue: Exiting AddBufferToQueue routine with FALSE\n");
				return(FALSE);					// get out now
		}	// end switch for processing WaitForMultipleObjects

		if (dwTrapQueueSize > MAX_QUEUE_SIZE)
		{
			WriteTrace(0x14,"AddBufferToQueue: queue too big -- posting notification event %08X\n",
				hEventNotify);
			
			if ( !SetEvent(hEventNotify) )
			{
				lastError = GetLastError(); 			// get error return codes
				WriteTrace(0x14,"AddBufferToQueue: Unable to post event %08X; reason is %lu\n",
					hEventNotify, lastError);
				WriteLog(SNMPELEA_CANT_POST_NOTIFY_EVENT, HandleToUlong(hEventNotify), lastError);
			}
			else
			{
				if (!ReleaseMutex(hMutex))
				{
					lastError = GetLastError(); 	// get error information
					WriteTrace(0x14,"AddBufferToQueue: Unable to release mutex object for reason code %lu\n",
						lastError);
					WriteLog(SNMPELEA_RELEASE_MUTEX_ERROR, lastError);
				}
				else
				{
					Sleep(1000);	//try and let the other thread get the mutex
					continue;		//and try and get the mutex again...
				}
			}
		}
		break;			// if we get here, then we've got the Mutex object

	}	// end while true for acquiring Mutex object

	if (lpVarBindQueue == (PVarBindQueue) NULL)
	{
		dwTrapQueueSize = 1;
		WriteTrace(0x0a,"AddBufferToQueue: Current queue is empty. Adding %08X as first queue entry\n",
			lpVarBindEntry);
		lpVarBindQueue = lpVarBindEntry;		// indicate first in queue

		WriteTrace(0x0a,"AddBufferToQueue: Releasing mutex object %08X\n", hMutex);
		if (!ReleaseMutex(hMutex))
		{
			lastError = GetLastError(); 	// get error information
			WriteTrace(0x14,"AddBufferToQueue: Unable to release mutex object for reason code %lu\n",
				lastError);
			WriteLog(SNMPELEA_RELEASE_MUTEX_ERROR, lastError);
		}

		WriteTrace(0x0a,"AddBufferToQueue: Exiting AddBufferToQueue function with TRUE\n");
		return(TRUE);						// show added to queue
	}

	WriteTrace(0x0a,"AddBufferToQueue: Queue is not empty. Scanning for end of queue.\n");
	pBuffer = lpVarBindQueue;			// starting point

	while (pBuffer->lpNextQueueEntry != (PVarBindQueue) NULL)
	{
		WriteTrace(0x00,"AddBufferToQueue: This buffer address is %08X, next buffer pointer is %08X\n",
			pBuffer, pBuffer->lpNextQueueEntry);
		pBuffer = pBuffer->lpNextQueueEntry;	// point to next buffer
	}

	WriteTrace(0x0a,"AddBufferToQueue: Adding buffer address %08X as next buffer pointer in %08X\n",
		lpVarBindEntry, pBuffer);
	pBuffer->lpNextQueueEntry = lpVarBindEntry; // add to end of chain
	dwTrapQueueSize++;

	WriteTrace(0x0a,"AddBufferToQueue: Releasing mutex object %08X\n", hMutex);
	if (!ReleaseMutex(hMutex))
	{
		lastError = GetLastError(); 	// get error information
		WriteTrace(0x14,"AddBufferToQueue: Unable to release mutex object for reason code %lu\n",
			lastError);
		WriteLog(SNMPELEA_RELEASE_MUTEX_ERROR, lastError);
	}

	WriteTrace(0x0a,"AddBufferToQueue: Exiting AddBufferToQueue function with TRUE\n");
	return(TRUE);							// show added to queue
}

HINSTANCE
AddSourceHandle(
    IN LPTSTR   lpszModuleName
    )
{
    PSourceHandleList   pNewModule;
	
    pNewModule = (PSourceHandleList) SNMP_malloc(sizeof(SourceHandleList));

	if (pNewModule == NULL)
    {
		WriteTrace(0x14,"AddSourceHandle: Unable to acquire storage for source/handle entry.\n");
		WriteLog(SNMPELEA_COUNT_TABLE_ALLOC_ERROR);

		return NULL;
    }

    pNewModule->handle = NULL;
    _tcscpy(pNewModule->sourcename, lpszModuleName);

    // load the module as a data file; we look only for messages
    pNewModule->handle = LoadLibraryEx(lpszModuleName, NULL, LOAD_LIBRARY_AS_DATAFILE);
	
    // loading the module failed
    if (pNewModule->handle == (HINSTANCE) NULL )
    {
        DWORD dwError = GetLastError();

		WriteTrace(
            0x14,
            "AddSourceHandle: Unable to load message module %s; LoadLibraryEx returned %lu\n",
            lpszModuleName,
            dwError);

        WriteLog(
            SNMPELEA_CANT_LOAD_MSG_DLL,
            lpszModuleName,
            dwError);

		WriteTrace(0x0a,"AddSourceHandle: Exiting AddSourceHandle with NULL.\n");

        SNMP_free(pNewModule);

		return NULL;

    }

    pNewModule->Next = lpSourceHandleList;	// set forward pointer
    lpSourceHandleList = pNewModule;        //add item to list.

    return pNewModule->handle;
}

HINSTANCE
FindSourceHandle(
	IN LPTSTR	lpszSource
	)
{
   PSourceHandleList	lpSource;

	if (lpSourceHandleList == (PSourceHandleList) NULL)
	{
		return ((HINSTANCE) NULL);
	}

	lpSource = lpSourceHandleList;

	while (lpSource != (PSourceHandleList) NULL)
	{
		if (_tcscmp(lpszSource, lpSource->sourcename) == 0)
		{
			return (lpSource->handle);
		}
		lpSource = lpSource->Next;
	}

	return ((HINSTANCE) NULL);
}





VOID
ScanParameters(
	IN	OUT LPTSTR	*lpStringArray, 				// pointer to array of insertion strings
	IN		UINT	nNumStr,						// number of insertion strings
	IN	OUT PUINT	nStringsSize,					// address of size of all insertion strings
	IN		LPTSTR	lpszSrc,						// pointer to source name for event
	IN		LPTSTR	lpszLog,						// pointer to the registry name for this source
	IN		HMODULE hPrimModule 					// handle to secondary message module DLL
	 )

/*++

Routine Description:

	This routine will scan the insertion strings looking for occurances of %%n, where
	n is a number indicating a substitution parameter value. If no occurances of %%n are found,
	then the routine simply returns without making any modifications.

	If any occurance of %%n is found in the buffer, secondary parameter substitution is then
	required. FormatMessage is called, without any insertion strings. The event ID is the
	value of n following the %%. The message module DLL is one of the following:

	Registry
		Machine
			SYSTEM
				CurrentControlSet
					Services
						EventLog
							LogFile (Security, Application, System, etc.)
								Source
									ParameterMessageFile		REG_EXPAND_SZ

									- or -

								PrimaryModule					REG_SZ

	If the ParameterMessageFile does not exist for the indicated source, then the PrimaryModule
	value will be used for the LogFile key. If this value does not exist, or if any error occurs
	when loading any of these DLL's, or if the parameter value cannot be found, the %%n value is
	replaced with a NULL string and processing continues.

Arguments:

	lpStringArray	-	Pointer to an array of insertion strings.

	nNumStr 		-	Number of insertion strings in lpStrArray.

	nStringsSize	-	Pointer to total size of all insertion strings in lpStrArray.

	lpszSrc 		-	Pointer to the source name for this event.

	lpszLog 		-	Pointer to the registry event source name.

	hPrimModule 	-	Secondary parameter module DLL handle for this event.

Return Value:

	None.

Notes:


--*/

{
	LONG			lastError;							// return code from GetLastError
	TCHAR			szXParmModuleName[MAX_PATH+1];		// space for DLL message module
	TCHAR			szParmModuleName[MAX_PATH+1];		// space for expanded DLL message module
    BOOL            bExistParmModule;                   // says whether a ParmModuleName is specified or not
	DWORD			nFile = MAX_PATH+1; 				// max size for DLL message module name
	DWORD			dwType; 							// type of message module name
	DWORD			status; 							// status from registry calls
	DWORD			cbExpand;							// byte count for REG_EXPAND_SZ parameters
	HKEY			hkResult;							// handle to registry information
	HINSTANCE		hParmModule;						// handle to message module DLL
	UINT			nBytes; 							// temporary field
	UINT			i;									// temporary counter
	LPTSTR			lpParmBuffer;
	LPTSTR			lpszString, lpStartDigit, lpNew;
	UINT			nStrSize, nSubNo, nParmSize, nNewSize, nOffset;
	PSourceHandleList	lpsource;						//pointer to source/handle list


	WriteTrace(0x0a,"ScanParameters: Entering ScanParameters routine\n");
	WriteTrace(0x00,"ScanParameters: Size of original insertion strings is %lu\n", *nStringsSize);

	WriteTrace(0x0a,"ScanParameters: Opening registry for parameter module for %s\n", lpszLog);

	if ( (status = RegOpenKeyEx(		// open the registry to read the name
		HKEY_LOCAL_MACHINE, 			// of the message module DLL
		lpszLog,						// registry key to open
		0,
		KEY_READ,
		&hkResult) ) != ERROR_SUCCESS)
	{
		WriteTrace(0x14,"ScanParameters: Unable to open EventLog service registry key %s; RegOpenKeyEx returned %lu\n",
			lpszLog, status);			// write trace event record
		WriteLog(SNMPELEA_CANT_OPEN_REGISTRY_PARM_DLL, lpszLog, status);
		WriteTrace(0x0a,"ScanParameters: Exiting ScanParameters\n");
		return; 						// return
	}

	if ( (status = RegQueryValueEx( 		// look up module name
		hkResult,							// handle to registry key
		EXTENSION_PARM_MODULE,				// key to look up
		0,									// ignored
		&dwType,							// address to return type value
		(LPBYTE) szXParmModuleName, 		// where to return parameter module name
		&nFile) ) != ERROR_SUCCESS) 		// size of parameter module name field
	{
		WriteTrace(0x14,"ScanParameters: No ParameterMessageFile registry key for %s; RegQueryValueEx returned %lu\n",
			lpszLog, status);			// write trace event record

        bExistParmModule = FALSE;
	}
	else
	{
		WriteTrace(0x0a,"ScanParameters: ParameterMessageFile value read was %s\n", szXParmModuleName);
		cbExpand = ExpandEnvironmentStrings(	// expand the DLL name
			szXParmModuleName,					// unexpanded DLL name
			szParmModuleName,					// expanded DLL name
			MAX_PATH+1);						// max size of expanded DLL name

		if (cbExpand > MAX_PATH+1)		// if it didn't expand correctly
		{
			WriteTrace(0x14,"ScanParameters: Unable to expand parameter module %s; expanded size required is %lu bytes\n",
				szXParmModuleName, cbExpand);	// log error message
			WriteLog(SNMPELEA_CANT_EXPAND_PARM_DLL, szXParmModuleName, cbExpand);

            bExistParmModule = FALSE;
		}
		else
		{
			WriteTrace(0x0a,"ScanParameters: ParameterMessageFile expanded to %s\n", szParmModuleName);

            bExistParmModule = TRUE;
    	}
	}
    // at this point either bExistParmModule = FALSE 
    // or we have the ';' separated list of ParmModules
    // in szParmModuleName

	WriteTrace(0x0a,"ScanParameters: Closing registry key for parameter module\n");
	RegCloseKey(hkResult);		// close the registry key

	// for each insertion string
    for (i = 0; i < nNumStr; i++)
	{
		WriteTrace(0x00,"ScanParameters: Scanning insertion string %lu: %s\n",
			i, lpStringArray[i]);
		nStrSize = strlen(lpStringArray[i]);	// get size of insertion string
		lpszString = lpStringArray[i];			// set initial pointer

        // for each sub string identifier in the insertion string
		while (nStrSize > 2)
		{
			if ( (lpStartDigit = strstr(lpszString, TEXT("%%"))) == NULL )
			{
				WriteTrace(0x00,"ScanParameters: No secondary substitution parameters found\n");
				break;
			}

			nOffset = (UINT)(lpStartDigit - lpStringArray[i]);	// calculate offset in buffer of %%
			lpStartDigit += 2;					// point to start of potential digit
			lpszString = lpStartDigit;			// set new string pointer
			nStrSize = strlen(lpszString);		// calculate new string length

			if (nStrSize == 0)
			{
				WriteTrace(0x00,"ScanParameters: %% found, but remainder of string is null\n");
				break;
			}

			nSubNo = atol(lpStartDigit);		// convert to long integer

			if (nSubNo == 0 && *lpStartDigit != '0')
			{
				WriteTrace(0x0a,"ScanParameters: %% found, but following characters were not numeric\n");
				lpszString--;					// back up 1 byte
// DBCS start
// not need
//				if(WHATISCHAR(lpszString-1, 2) == CHAR_DBCS_TRAIL)
//					lpszString--;
// DBCS end
				nStrSize = strlen(lpszString);	// recalculate length
				continue;						// continue parsing the string
			}

            // initialize nBytes to 0 to make clear no message formatting was done.
            nBytes = 0;
            lastError = 0;

            // if there is a parameter file, look for into it for the secondary substitution strings
            if (bExistParmModule)
			{
                LPTSTR pNextModule = szParmModuleName;

                // for each module name in ParameterMessageFile list of modules
                while (pNextModule != NULL)
                {
                    // look for the next delimiter and change it with a string terminator
                    // in order to isolate the first module name - pointed by pNextModule
                    LPTSTR pDelim = _tcschr(pNextModule, _T(';'));
                    if (pDelim != NULL)
                        *pDelim = _T('\0');

                    WriteTrace(
                        0x0a,
                        "ScanParameters: Looking up secondary substitution string %lu in ParameterMessageFile %s\n",
					    nSubNo,
                        pNextModule);

                    // get the handle to the module (load the module now if need be)
                    hParmModule = FindSourceHandle(pNextModule);
                    if (!hParmModule)
                        hParmModule = AddSourceHandle(pNextModule);

                    // careful to restore the szParmModuleName string to its original content
                    // we need this as far as the scanning should be done for each of the insertion strings
                    if (pDelim != NULL)
                        *pDelim = _T(';');
                    
                    // it's not clear whether FormatMessage() allocates any memory in lpParmBuffer when it fails
                    // so initialize the pointer here, and free it in case of failure. LocalFree is harmless on
                    // NULL pointer.
                    lpParmBuffer = NULL;

                    // if we have a valid parameter module handle at this point,
                    // format the insertion string using this module
                    if (hParmModule != NULL)
                    {
				        nBytes = FormatMessage(
					        FORMAT_MESSAGE_ALLOCATE_BUFFER |	// let api build buffer
					        FORMAT_MESSAGE_IGNORE_INSERTS | 	// ignore inserted strings
					        FORMAT_MESSAGE_FROM_HMODULE,		// look thru message DLL
					        (LPVOID) hParmModule,				// use parameter file
					        nSubNo, 							// parameter number to get
					        (ULONG) NULL,						// specify no language
					        (LPTSTR) &lpParmBuffer, 			// address for buffer pointer
					        80, 								// minimum space to allocate
					        NULL);								// no inserted strings

                        lastError = GetLastError();
                    }

                    // if the formatting succeeds, break the loop (szParmModuleName should
                    // be at this point exactly as it was when the loop was entered)
                    if (nBytes != 0)
                        break;

                    LocalFree(lpParmBuffer);

                    // move on to the next module name
                    pNextModule = pDelim != NULL ? pDelim + 1 : NULL;
                }
			}

			if (nBytes == 0)
			{

				WriteTrace(0x0a,"ScanParameters: ParameterMessageFile did not locate parameter - error %lu\n",
					lastError);
//				WriteLog(SNMPELEA_PARM_NOT_FOUND, nSubNo, lastError);
				LocalFree(lpParmBuffer);	// free storage

				WriteTrace(0x0a,"ScanParameters: Searching PrimaryModule for parameter\n");

				nBytes = FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER |	// let api build buffer
					FORMAT_MESSAGE_IGNORE_INSERTS | 	// ignore inserted strings
					FORMAT_MESSAGE_FROM_HMODULE,		// look thru message DLL
					(LPVOID) hPrimModule,				// use parameter file
					nSubNo, 							// parameter number to get
					(ULONG) NULL,						// specify no language
					(LPTSTR) &lpParmBuffer, 			// address for buffer pointer
					80, 								// minimum space to allocate
					NULL);								// no inserted strings

				if (nBytes == 0)
				{
					lastError = GetLastError(); // get error code
					WriteTrace(0x0a,"ScanParameters: PrimaryModule did not locate parameter - error %lu\n",
						lastError);
					WriteLog(SNMPELEA_PRIM_NOT_FOUND, nSubNo, lastError);
					LocalFree(lpParmBuffer);	// free storage
				}
			}

			nParmSize = 2;					// set initialize parameter size (%%)

			while (strlen(lpszString))
			{
				if (!isdigit(*lpszString))
				{
					break;					// exit if no more digits
				}

				nParmSize++;				// increment parameter size
// DBCS start
				if (IsDBCSLeadByte(*lpszString))
					lpszString++;
// DBCS end
				lpszString++;				// point to next byte
			}

			nNewSize = strlen(lpStringArray[i])+nBytes-nParmSize+1; // calculate new length
			nStrSize = strlen(lpStringArray[i])+1;	// get original length
			WriteTrace(0x00,"ScanParameters: Original string length is %lu, new string length is %lu\n",
				nStrSize, nNewSize);

			if (nNewSize > nStrSize)
			{
				lpNew = (TCHAR *) SNMP_realloc(lpStringArray[i], nNewSize);

				if ( lpNew == NULL)
				{
					WriteTrace(0x14,"ScanParameters: Unable to reallocate storage for insertion strings. Scanning terminated.\n");
					WriteLog(SNMPELEA_REALLOC_INSERTION_STRINGS_FAILED);
					WriteTrace(0x00,"ScanParameters: Size of new insertion strings is %lu\n", *nStringsSize);
					return; 				// return
				}

				WriteTrace(0x0a,"ScanParameters: Insertion string reallocated to %08X\n", lpNew);
				lpStringArray[i] = lpNew;					// set new pointer
				lpStartDigit = lpStringArray[i] + nOffset;	// point to new start of current %%
				lpszString = lpStartDigit+nBytes;			// set new start of scan spot
				*nStringsSize += nBytes-nParmSize;			// calculate new total size
				WriteTrace(0x00,"ScanParameters: Old size of all insertion strings was %lu, new size is %lu\n",
					*(nStringsSize)-nBytes+nParmSize, *nStringsSize);
			}
			else
			{
				WriteTrace(0x0a,"ScanParameters: New size of string is <= old size of string\n");
				lpStartDigit -= 2;							// now point to %%
				lpszString = lpStartDigit;					// set new start of scan spot
			}

			nStrSize = strlen(lpStartDigit)+1;	// calculate length of remainder of string

			if (nBytes)
			{
				memmove(lpStartDigit+nBytes-nParmSize,		// destination address
					lpStartDigit,							// source address
					nStrSize);								// amount of data to move

				memmove(lpStartDigit,						// destination address
					lpParmBuffer,							// source address
					nBytes);								// amount of data to move
				
				LocalFree(lpParmBuffer);
			}
			else
			{
				memmove(lpStartDigit,						// destination address
					lpStartDigit+nParmSize, 				// source address
					nStrSize);								// amount of data to move
			}
			
			WriteTrace(0x00,"ScanParameters: New insertion string is %s\n",
				lpStringArray[i]);
			nStrSize = strlen(lpszString);	// get length of remainder of string
		}
	}

	WriteTrace(0x00,"ScanParameters: Size of new insertion strings is %lu\n", *nStringsSize);
	WriteTrace(0x0a,"ScanParameters: Exiting ScanParameters routine\n");
}


VOID
FreeArrays(
	 IN UINT	nCount, 		// number of array entries to free
	 IN PUINT	lpStrLenArray,	// pointer to string length array
	 IN LPTSTR	*lpStringArray, // pointer to string pointer array
	 IN BOOL	DelStrs = TRUE
	 )

/*++

Routine Description:

	This routine will free the allocated storage for strings in case of an error when
	building the varbind entries.


Arguments:

	nCount			-	This is a count of the number of entries to free

	lpStrLenArray	-	This is a pointer to the string length array to be freed.

	lpStringArray	-	This is a pointer to the string array to be freed.

	DelStrs 		-	Should the strings be deleted?

Return Value:

	None.

Notes:


--*/

{
	if (DelStrs)
	{
		WriteTrace(0x00,"FreeArrays: Freeing storage for strings and string length arrays\n");

		for (UINT j=0; j < nCount+5; j++)
		{
			if (lpStrLenArray[j] != 0)
			{
				WriteTrace(0x0a,"FreeArrays: Freeing string storage at address %08X\n",
					lpStringArray[j]);
				SNMP_free(lpStringArray[j]);
			}
		}

		WriteTrace(0x0a,"FreeArrays: Freeing storage for string array %08X\n", lpStringArray);
		SNMP_free(lpStringArray);
	}
	else
		WriteTrace(0x00,"FreeArrays: Freeing storage for string length array only\n");

	WriteTrace(0x0a,"FreeArrays: Freeing storage for string length array %08X\n", lpStrLenArray);
	SNMP_free(lpStrLenArray);

	return;
}


VOID
FreeVarBind(
	IN	UINT				count,
	IN	RFC1157VarBindList	*varBind
	)

/*++

Routine Description:

	FreeVarBind will free the storage allocated to the indicated varbind and associated
	varbind list.

Arguments:

	count	-	Number of entries to free.

	varBind -	Pointer to the varbind list structure.

Return Value:

	None.

--*/

{
	UINT	j;						// counter

	WriteTrace(0x0a,"FreeVarBind: Entering FreeVarBind routine\n");
	WriteTrace(0x00,"FreeVarBind: Varbind list is %08X\n", varBind);
	WriteTrace(0x00,"FreeVarBind: varBind->list is %08X\n", varBind->list);

	for (j=0; j < count; j++)
	{
		WriteTrace(0x00,"FreeVarBind: Freeing OID #%lu ids at %08X\n", j, &varBind->list[j].name.ids);
		SNMP_free((&varBind->list[j].name)->ids);
		WriteTrace(0x00,"FreeVarBind: Freeing  varbind stream #%lu at %08X\n", j, &varBind->list[j].value.asnValue.string.stream);
		SNMP_free((&varBind->list[j].value.asnValue.string)->stream);

//22 May 96****************************************************************************************************
//Varbind was allocated as an array in BuildTrapBuffer and so one SNMP_free should be called after this method

//		WriteTrace(0x0a,"FreeVarBind: Freeing varbind %lu at %08X\n",
//			j, &varBind->list[j]);
//		SnmpUtilVarBindFree(&varBind->list[j]);
	}

//22 May 96***************************************************************************************************
//Let the procedure that calls this procedure delete the varBind object

//	WriteTrace(0x0a,"FreeVarBind: Freeing varbind list %08X\n", varBind);
//	SnmpUtilVarBindListFree(varBind);
	WriteTrace(0x0a,"FreeVarBind: Exiting FreeVarBind routine\n");
	return; 										// exit
}


UINT
TrimTrap(
	IN	OUT RFC1157VarBindList	*varBind,
	IN	OUT UINT				size,
	IN		BOOL				fTrimMessage
	)

/*++

Routine Description:

	TrimTrap will trim the trap in order to keep the trap size below 4096 bytes (SNMP
	maximum packet size). The global trim flag will be used to determine if data should be
	trimmed or omitted.

Arguments:

	varBind -	Pointer to the varbind list structure.

	size	-	Current size, upon entry, of the entire trap structure.

Return Value:

	None.

Notes:

	This routine does not correctly trim the trap data. Microsoft indicated that this routine
	currently was not required, thus no calls are being made to this routine.

--*/

{
	UINT	i;							// counter
	UINT	nTrim;						// temporary variable
	UINT	nVarBind;					// temporary variable

	WriteTrace(0x0a,"TrimTrap: Entering TrimTrap routine\n");

	nTrim = size - nMaxTrapSize;		// see how much we have to trim
	WriteTrace(0x00,"TrimTrap: Trimming %lu bytes\n", nTrim);
	WriteTrace(0x00,"TrimTrap: Trap size is %lu bytes\n", size);

	if (fTrimMessage)					// if we're trimming the message text first
	{
		WriteTrace(0x0a,"TrimTrap: Registry values indicate EventLog text to be trimmed first\n");

		nVarBind = varBind->list[0].value.asnValue.string.length;

		if (nVarBind > nTrim)
		{
			WriteTrace(0x0a,"TrimTrap: EventLog text size is greater than amount to trim. Trimming EventLog text only\n");
			WriteTrace(0x00,"TrimTrap: EventLog text size is %lu, trim amount is %lu\n",
				nVarBind, nTrim);

			varBind->list[0].value.asnValue.string.length -= nTrim;
			*(varBind->list[0].value.asnValue.string.stream + nVarBind + 1) = '\0'; // add null pointer for tracing

			WriteTrace(0x00,"TrimTrap: New EventLog text is %s\n",
				varBind->list[0].value.asnValue.string.stream);
			WriteTrace(0x0a,"TrimTrap: Exiting TrimTrap routine\n");

			size -= nTrim;		// drop by length of string
			return(size);							// exit
		}

		WriteTrace(0x0a,"TrimTrap: EventLog text size is less than or equal to the amount to trim. Zeroing varbinds.\n");
		WriteTrace(0x0a,"TrimTrap: Zeroing EventLog text.\n");

		size -= nVarBind;

		WriteTrace(0x00,"TrimTrap: Trimming off %lu bytes from EventLog text.\n", nVarBind);
		WriteTrace(0x00,"TrimTrap: New size is now %lu bytes.\n", size);

		varBind->list[0].value.asnValue.string.length = 0;
		*(varBind->list[0].value.asnValue.string.stream) = '\0';	// make it null

		i = varBind->len-1; 	// set index counter

		while (size > nMaxTrapSize && i != 0)
		{
			nVarBind = varBind->list[i].value.asnValue.string.length;

			WriteTrace(0x0a,"TrimTrap: Trap size is %lu, max size is %lu. Zeroing varbind entry %lu of size %lu.\n",
				size, nMaxTrapSize, i, nVarBind);

			size -= nVarBind;
			varBind->list[i].value.asnValue.string.length = 0;			// set length
			*(varBind->list[i--].value.asnValue.string.stream) = '\0';	// make it null
		}

		WriteTrace(0x0a,"TrimTrap: Trap size is now %lu.\n", size);

		if (size > nMaxTrapSize)
		{
			WriteTrace(0x14,"TrimTrap: All varbinds have been zeroed, but trap still too large.\n");
			WriteLog(SNMPELEA_TRIM_FAILED);
			return(0);			// exit
		}

		return(size);			// exit
	}
	else
	{
		WriteTrace(0x0a,"TrimTrap: Registry values indicate varbind insertion strings to be trimmed first\n");

		i = varBind->len-1; 	// set index counter

		while ( (size > nMaxTrapSize) && (i != 0) )
		{
			nVarBind = varBind->list[i].value.asnValue.string.length;

			WriteTrace(0x0a,"TrimTrap: Trap size is %lu, max size is %lu. Zeroing varbind entry %lu of size %lu.\n",
				size, nMaxTrapSize, i, nVarBind);

			size -= nVarBind;
			varBind->list[i].value.asnValue.string.length = 0;			// set length
			*(varBind->list[i--].value.asnValue.string.stream) = '\0';	// make it null
		}

		if (size <= nMaxTrapSize)
		{
			WriteTrace(0x0a,"TrimTrap: Trap size is now %lu.\n", size);
			WriteTrace(0x0a,"TrimTrap: Exiting TrimTrap routine\n");
			return(size);
		}

		nVarBind = varBind->list[0].value.asnValue.string.length;	// get length of event log text

		WriteTrace(0x0a,"TrimTrap: All insertion strings removed. Only EventLog text remains of size %lu.\n",
			nVarBind);

		nTrim = size - nMaxTrapSize;		// compute how much to trim

		WriteTrace(0x00,"TrimTrap: Need to trim %lu bytes from Event Log text.\n", nTrim);

		if (nVarBind < nTrim)
		{
			WriteTrace(0x14,"TrimTrap: Data to be trimmed exceeds data in trap.\n");
			WriteLog(SNMPELEA_TRIM_FAILED);
			return(0);
		}

		varBind->list[0].value.asnValue.string.length -= nTrim;

		WriteTrace(0x00,"TrimTrap: EventLog text string length is now %lu\n",
			varBind->list[0].value.asnValue.string.length);

		*(varBind->list[0].value.asnValue.string.stream + nVarBind + 1) = '\0'; // add null pointer for tracing

		WriteTrace(0x00,"TrimTrap: New EventLog text is %s\n",
			varBind->list[0].value.asnValue.string.stream);

		size -= nTrim;		// drop by length of string

		WriteTrace(0x0a,"TrimTrap: Trap size is now %lu.\n", size);
		WriteTrace(0x0a,"TrimTrap: Exiting TrimTrap routine\n");

		return(size);							// exit
	}

	WriteTrace(0x0a,"TrimTrap: Exiting TrimTrap routine. Default return.\n");
	return(size);							// exit
}


BOOL
BuildTrapBuffer(
	 IN PEVENTLOGRECORD EventBuffer,		// Event Record from Event Log
	 IN REGSTRUCT		rsRegStruct,		// Registry information structure
	 IN LPTSTR			lpszLogFile,		// log file name for event
	 IN HMODULE 		hPrimModule 		// handle to secondary parameter module
	 )

/*++

Routine Description:

	This routine will build the buffer that contains the variable bindings for the
	trap data to be sent. Coordination between this routine and the trap sending thread
	is done with a MUTEX object. This thread will block until the object can be acquired
	or until it is notified that the agent DLL is terminating.


Arguments:

	EventBuffer -	This is a pointer to a buffer containing the event log text.

	rsRegStruct -	This is a structure containing registry information that relates
					to the information contained on the event log buffer.

	lpszLogFile -	The name of the log file read for this event. Used to read the
					registry to get the message file DLL and then to acquire the text
					of the message for this event id.

	hPrimModule -	Handle to the module loaded for secondary parameter insertions for
					secondary insertion strings. This is the PrimaryModule as specified
					in the registry for each log file.

Return Value:

	TRUE	-	A trap buffer was successfully built and added to the queue.

	FALSE	-	A trap buffer could not be constructed or the DLL is terminating.

Notes:


--*/

{
	LONG			lastError;							// return code from GetLastError
	TCHAR			szXMsgModuleName[MAX_PATH+1];		// space for DLL message module
	TCHAR			szMsgModuleName[MAX_PATH+1];		// space for expanded DLL message module
	DWORD			nFile = MAX_PATH+1; 				// max size for DLL message module name
	DWORD			dwType; 							// type of message module name
	DWORD			status; 							// status from registry calls
	DWORD			cbExpand;							// byte count for REG_EXPAND_SZ parameters
	HKEY			hkResult;							// handle to registry information
	HINSTANCE		hMsgModule; 						// handle to message module DLL
	LPTSTR			*lpStringArray; 					// pointer to array of strings
	PUINT			lpStrLenArray;						// pointer to array of string lengths
	LPTSTR			lpszSource; 						// pointer to source name
	PSID			psidUserSid;						// pointer to user sid
	LPTSTR			lpszString; 						// pointer to inserted strings
	UINT			size;								// size of trap buffer
	UINT			nStringSize;						// temporary field
	UINT			nBytes; 							// temporary field
	UINT			i, j;								// temporary counters
	TCHAR			lpszLog[MAX_PATH+1];				// temporary registry name
	LPTSTR			lpBuffer;							// pointer to event log text
	DWORD			cchReferencedDomain = MAX_PATH*2+1; // size of referenced domain
	TCHAR			lpszReferencedDomain[MAX_PATH+1];	// referenced domain
	TCHAR			szTempBuffer[MAX_PATH*2+1]; 		// temporary buffer
	DWORD			nBuffer;							// temporary size field
	SID_NAME_USE	snu;								// SID name use field
	PVarBindQueue	varBindEntry;						// pointer to varbind queue entry
	PSourceHandleList	lpsource;						//pointer to source/handle list
	TCHAR			szTempBuffer2[MAX_PATH*2+1];

	WriteTrace(0x0a,"BuildTrapBuffer: Entering BuildTrapBuffer\n");

	if (fThresholdEnabled && fThreshold)
	{
		WriteTrace(0x0a,"BuildTrapBuffer: Performance threshold flag is on. No data will be processed.\n");
		WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer function with FALSE\n");
		return(FALSE);
	}

	WriteTrace(0x00,"BuildTrapBuffer: Notify event handle is %08X\n", hEventNotify);

	nBuffer = MAX_PATH*2+1; 							// reset length field to default
	lpszSource = (LPTSTR) EventBuffer + EVENTRECSIZE;	// point to source name
	psidUserSid = (PSID) ( (LPTSTR) EventBuffer + EventBuffer->UserSidOffset);	// point to user sid
	lpszString = (LPTSTR) EventBuffer + EventBuffer->StringOffset;	// point to first string

	WriteTrace(0x00,"BuildTrapBuffer: Source name is %s, length is %u\n", lpszSource, strlen(lpszSource));
	WriteTrace(0x00,"BuildTrapBuffer: Computer name is %s, length is %u\n",
		lpszSource+strlen(lpszSource)+1, strlen(lpszSource+strlen(lpszSource)+1) );
	WriteTrace(0x00,"BuildTrapBuffer: Pointer to User SID is %08X\n", psidUserSid);
	WriteTrace(0x00,"BuildTrapBuffer: First inserted string is %s\n", lpszString);


	strcpy(lpszLog, EVENTLOG_BASE); 	// copy base registry name
	strcat(lpszLog, lpszLogFile);		// add on the log file name read
	strcat(lpszLog, TEXT("\\"));		// tack on backslash
	strcat(lpszLog, lpszSource);		// add on the source name

	WriteTrace(0x0a,"BuildTrapBuffer: Opening registry for message module for %s\n", lpszLog);

	if ( (status = RegOpenKeyEx(		// open the registry to read the name
		HKEY_LOCAL_MACHINE, 			// of the message module DLL
		lpszLog,						// registry key to open
		0,
		KEY_READ,
		&hkResult) ) != ERROR_SUCCESS)
	{
		WriteTrace(0x14,"BuildTrapBuffer: Unable to open EventLog service registry key %s; RegOpenKeyEx returned %lu\n",
			lpszLog, status);			// write trace event record
		WriteLog(SNMPELEA_CANT_OPEN_REGISTRY_MSG_DLL, lpszLog, status);
		WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with FALSE\n");
		return(FALSE);					// return
	}

	if ( (status = RegQueryValueEx( // look up module name
		hkResult,					// handle to registry key
		EXTENSION_MSG_MODULE,		// key to look up
		0,							// ignored
		&dwType,					// address to return type value
		(LPBYTE) szXMsgModuleName,	// where to return message module name
		&nFile) ) != ERROR_SUCCESS) // size of message module name field
	{
		WriteTrace(0x14,"BuildTrapBuffer: No EventMessageFile registry key for %s; RegQueryValueEx returned %lu\n",
			lpszLog, status);			// write trace event record
		WriteLog(SNMPELEA_NO_REGISTRY_MSG_DLL, lpszLog, status);
		RegCloseKey(hkResult);		// close the registry key
		WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with FALSE\n");
		return(FALSE);					// return
	}

	RegCloseKey(hkResult);		// close the registry key

	cbExpand = ExpandEnvironmentStrings(	// expand the DLL name
		szXMsgModuleName,					// unexpanded DLL name
		szMsgModuleName,					// expanded DLL name
		MAX_PATH+1);						// max size of expanded DLL name

	if (cbExpand > MAX_PATH+1)		// if it didn't expand correctly
	{
		WriteTrace(0x14,"BuildTrapBuffer: Unable to expand message module %s; expanded size required is %lu bytes\n",
			szXMsgModuleName, cbExpand);	// log error message
		WriteLog(SNMPELEA_CANT_EXPAND_MSG_DLL, szXMsgModuleName, cbExpand);
		WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with FALSE\n");
		return(FALSE);					// return
	}
    //---- at this point, szMsgModuleName is the value for "EventMessageFile" parameter----
    //---- it might be one module name or a ';' separated list of module names

    // alloc here the array of pointer to varbind values
    // the first 5 varbind are:
    // 1.3.1.0 - message description 
    // 1.3.2.0 - user name
    // 1.3.3.0 - system name
    // 1.3.4.0 - event type
    // 1.3.5.0 - event category
    // the rest varbinds are one for each insertion string
	nStringSize = 0;
	lpStringArray = (LPTSTR *) SNMP_malloc((EventBuffer->NumStrings+5) * sizeof(LPTSTR) );

	if (lpStringArray == NULL)
	{
		WriteTrace(0x14,"BuildTrapBuffer: Unable to allocate storage for string array\n");
		WriteLog(SNMPELEA_INSERTION_STRING_ARRAY_ALLOC_FAILED);
		return(FALSE);
	}

	WriteTrace(0x00,"BuildTrapBuffer: String array allocated at %08X\n", lpStringArray);
	lpStrLenArray = (PUINT) SNMP_malloc((EventBuffer->NumStrings+5) * sizeof(UINT) );

	if (lpStrLenArray == NULL)
	{
		WriteTrace(0x14,"BuildTrapBuffer: Unable to allocate storage for string length array\n");
		WriteLog(SNMPELEA_INSERTION_STRING_LENGTH_ARRAY_ALLOC_FAILED);
		SNMP_free(lpStringArray);
		return(FALSE);
	}

	for (i = 0; i < (UINT) EventBuffer->NumStrings+5; i++)
	{
		lpStrLenArray[i] = 0;
	}

	WriteTrace(0x00,"BuildTrapBuffer: String length array allocated at %08X\n", lpStrLenArray);

	if (EventBuffer->NumStrings)
	{
		for (i = 5; i < (UINT) EventBuffer->NumStrings+5; i++)
		{
			lpStrLenArray[i] = _tcslen(lpszString);		// get size of insertion string
			WriteTrace(0x00,"BuildTrapBuffer: String %lu is %s, size of %lu\n",
				i, lpszString, lpStrLenArray[i]);

			lpStringArray[i] = (TCHAR *) SNMP_malloc(lpStrLenArray[i]+1);

			if ( lpStringArray[i] == NULL)
			{
				WriteTrace(0x14,"BuildTrapBuffer: Unable to allocate storage for insertion string\n");
				WriteLog(SNMPELEA_INSERTION_STRING_ALLOC_FAILED);

				FreeArrays(i, lpStrLenArray, lpStringArray);

				WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with FALSE\n");
				return(FALSE);					// return
			}

			WriteTrace(0x00,"BuildTrapBuffer: Insertion string %lu address at %08X\n",
				i, lpStringArray[i]);
			strcpy(lpStringArray[i],lpszString);	// copy string to storage
			
			nStringSize += lpStrLenArray[i]+1;		// accumulate total insertion string length
			lpszString += lpStrLenArray[i]+1;		// point to next insertion string
		}

		ScanParameters(&lpStringArray[5],	// address of insertion string array
			EventBuffer->NumStrings,		// number of insertion strings in array
			&nStringSize,					// address of size of all insertion strings
			lpszSource, 					// pointer to source name for event
			lpszLog,						// pointer to registry name
			hPrimModule);					// handle to secondary parameter scan module

		for (i=5; i < (UINT) EventBuffer->NumStrings+5; i++)
		{
			WriteTrace(0x00,"BuildTrapBuffer: Scanned string %lu is %s\n",
				i, lpStringArray[i]);

            // the insertion string might have been enlarged with substrings. Need to recompute their length
            lpStrLenArray[i] = _tcslen(lpStringArray[i]);
		}

    }

    LPTSTR pNextModule = szMsgModuleName;
    while (pNextModule != NULL)
    {
        LPTSTR pDelim = _tcschr(pNextModule, _T(';'));
        if (pDelim != NULL)
            *pDelim = _T('\0');

        //nadir
        //-------<We need now the 'EventMessageFile'>-----
        if ( _tcscmp(pNextModule, szelMsgModuleName) == 0)
	    {
		    WriteTrace(0x14,"BuildTrapBuffer: Request to trap extension agent log event ignored.\n");
		    WriteLog(SNMPELEA_LOG_EVENT_IGNORED);

    	    FreeArrays(EventBuffer->NumStrings, lpStrLenArray, lpStringArray);

            WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with FALSE\n");
		    return(FALSE);					// simply exit now
	    }

        if ((hMsgModule = FindSourceHandle(pNextModule)) == NULL)
           hMsgModule = AddSourceHandle(pNextModule);

        if (hMsgModule != NULL)
        {
            //-------<At this point format the message>--------
            lpBuffer = NULL;

            nBytes = FormatMessage( 				// see if we can format the message
		        FORMAT_MESSAGE_ALLOCATE_BUFFER |	// let api build buffer
                (EventBuffer->NumStrings ? FORMAT_MESSAGE_ARGUMENT_ARRAY : 0 ) | // indicate an array of string inserts
		        FORMAT_MESSAGE_FROM_HMODULE,		// look thru message DLL
		        (LPVOID) hMsgModule,				// handle to message module
		        EventBuffer->EventID,				// message number to get
		        (ULONG) NULL,						// specify no language
		        (LPTSTR) &lpBuffer, 				// address for buffer pointer
		        80, 								// minimum space to allocate
                EventBuffer->NumStrings ? (va_list*) &lpStringArray[5] : NULL); // address of array of pointers

            // store here the last error encountered while formatting (will be used to catch
            // the error condition at the end of all the iterations)
            lastError = GetLastError();

            // the event was formatted successfully so break the loop
            if (nBytes != 0)
                break;

            // is not clear whether FormatMessage is not allocating the buffer on failure.
            // just in case, free it here. As it was initialized to NULL, this call shouldn't harm
            LocalFree(lpBuffer);
        }

        // try the next module
        pNextModule = pDelim != NULL ? pDelim + 1 : NULL;
        //--------------
    }

    // the event could not be formatted by any of the 'EventMessageFile' modules. Will bail out
    if (nBytes == 0)
    {
		WriteTrace(0x14,"BuildTrapBuffer: Error formatting message number %lu (%08X) is %lu\n",
			EventBuffer->EventID, EventBuffer->EventID, lastError); // trace the problem
		WriteLog(SNMPELEA_CANT_FORMAT_MSG, EventBuffer->EventID, lastError);
		LocalFree(lpBuffer);					// free storage

		FreeArrays(EventBuffer->NumStrings, lpStrLenArray, lpStringArray);

		WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with FALSE\n");

        return FALSE;
    }

	WriteTrace(0x00,"BuildTrapBuffer: Formatted message: %s\n", lpBuffer);	// log the message in the trace file

	lpStrLenArray[0] = strlen(lpBuffer);			// set varbind length
	lpStringArray[0] = (TCHAR *) SNMP_malloc(lpStrLenArray[0] + 1); // get storage for varbind string

	if ( lpStringArray[0] == NULL)
	{
		lpStrLenArray[0] = 0;					// reset so storage isn't freed
		WriteTrace(0x14,"BuildTrapBuffer: Unable to allocate storage for insertion string\n");
		WriteLog(SNMPELEA_INSERTION_STRING_ALLOC_FAILED);

		FreeArrays(EventBuffer->NumStrings, lpStrLenArray, lpStringArray);		// free allocated storage
		LocalFree(lpBuffer);					// free storage

		WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with FALSE\n");
		return(FALSE);					// return
	}

	WriteTrace(0x00,"BuildTrapBuffer: Insertion string 0 address at %08X\n",
		lpStringArray[0]);

	strcpy(lpStringArray[0], lpBuffer); 			// copy buffer to varbind

	if ( LocalFree(lpBuffer) != NULL )			// free buffer storage
	{
		lastError = GetLastError(); 			// get error codes
		WriteTrace(0x14,"BuildTrapBuffer: Error freeing FormatMessage buffer is %lu\n",lastError);
		WriteLog(SNMPELEA_FREE_LOCAL_FAILED, lastError);
	}

	if (EventBuffer->UserSidLength)
	{
		if ( !LookupAccountSid( 					// lookup account name
				NULL,								// system to lookup account on
				psidUserSid,						// pointer to SID for this account
				szTempBuffer,						// return account name in this buffer
				&nBuffer,							// pointer to size of account name returned
				lpszReferencedDomain,				// domain where account was found
				&cchReferencedDomain,				// pointer to size of domain name
				&snu) ) 							// sid name use field pointer
		{
			lastError = GetLastError(); 			// get reason call failed
			WriteTrace(0x14,"BuildTrapBuffer: Unable to acquire account name for event, reason %lu. Unknown is used.\n",
				lastError);
			WriteLog(SNMPELEA_SID_UNKNOWN, lastError);
			strcpy(szTempBuffer,TEXT("Unknown"));	// set default account name
			nBuffer = strlen(szTempBuffer); 		// set default size
		}
	}
	else
	{
		WriteTrace(0x0a,"BuildTrapBuffer: UserSidLength was 0. No SID is present. Unknown is used.\n");
		strcpy(szTempBuffer,TEXT("Unknown"));		// set default account name
		nBuffer = strlen(szTempBuffer); 			// set default size
	}

	lpStringArray[1] = (TCHAR *) SNMP_malloc(nBuffer + 1);	// get storage for varbind string

	if ( lpStringArray[1] == NULL)
	{
		WriteTrace(0x14,"BuildTrapBuffer: Unable to allocate storage for insertion string\n");
		WriteLog(SNMPELEA_INSERTION_STRING_ALLOC_FAILED);

		FreeArrays(EventBuffer->NumStrings, lpStrLenArray, lpStringArray);		// free allocated storage

		WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with FALSE\n");
		return(FALSE);					// return
	}

	WriteTrace(0x00,"BuildTrapBuffer: Insertion string 1 address at %08X\n",
		lpStringArray[1]);

	strcpy(lpStringArray[1], szTempBuffer); 			// copy buffer to varbind
	lpStrLenArray[1] = nBuffer; 						// set varbind length

	lpStringArray[2] = (TCHAR *) SNMP_malloc(strlen(lpszSource + strlen(lpszSource) + 1) + 1);	// allocate storage for string
	if (lpStringArray[2] == NULL)
	{
		WriteTrace(0x14,"BuildTrapBuffer: Unable to allocate storage for computer name string. Trap not sent.\n");
		WriteLog(SNMPELEA_CANT_ALLOCATE_COMPUTER_NAME_STORAGE);

		FreeArrays(EventBuffer->NumStrings, lpStrLenArray, lpStringArray);		// free allocated storage

		WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with FALSE\n");
		return(FALSE);							// exit
	}

	WriteTrace(0x00,"BuildTrapBuffer: Insertion string 2 address at %08X\n",
		lpStringArray[2]);

	strcpy(lpStringArray[2], lpszSource + strlen(lpszSource) + 1);	// copy to varbind
	lpStrLenArray[2] = strlen(lpStringArray[2]);				// get actual string length

	_ultoa(EventBuffer->EventType, szTempBuffer, 10);	// convert to string
	lpStrLenArray[3] = strlen(szTempBuffer);			// get actual string length

	lpStringArray[3] = (TCHAR *) SNMP_malloc(lpStrLenArray[3] + 1); // allocate storage for string

	if (lpStringArray[3] == NULL)
	{
		lpStrLenArray[3] = 0;					// reset to 0 so storage isn't freed
		WriteTrace(0x14,"BuildTrapBuffer: Unable to allocate storage for event type string. Trap not sent.\n");
		WriteLog(SNMPELEA_CANT_ALLOCATE_EVENT_TYPE_STORAGE);

		FreeArrays(EventBuffer->NumStrings, lpStrLenArray, lpStringArray);		// free allocated storage

		WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with FALSE\n");
		return(FALSE);							// exit
	}

	WriteTrace(0x00,"BuildTrapBuffer: Insertion string 3 address at %08X\n",
		lpStringArray[3]);

	strcpy(lpStringArray[3], szTempBuffer); 	// copy string to varbind

	_ultoa(EventBuffer->EventCategory, szTempBuffer, 10);	// convert to string
	lpStrLenArray[4] = strlen(szTempBuffer);				// get actual string length

	lpStringArray[4] = (TCHAR *) SNMP_malloc(lpStrLenArray[4] + 1); // allocate storage for string

	if (lpStringArray[4] == NULL)
	{
		lpStrLenArray[4] = 0;					// reset to 0 so storage isn't freed
		WriteTrace(0x14,"BuildTrapBuffer: Unable to allocate storage for event category string. Trap not sent.\n");
		WriteLog(SNMPELEA_CANT_ALLOCATE_EVENT_CATEGORY_STORAGE);

		FreeArrays(EventBuffer->NumStrings, lpStrLenArray, lpStringArray);		// free allocated storage

		WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with FALSE\n");
		return(FALSE);							// exit
	}

	WriteTrace(0x00,"BuildTrapBuffer: Insertion string 4 address at %08X\n",
		lpStringArray[4]);

	strcpy(lpStringArray[4], szTempBuffer); 	// copy string to varbind

//
//	At this point, we have everything we need to actually build the varbind entries
//	We will now allocate the storage for the varbind queue entry, allocate the varbind list
//	and point to the data that we have previously constructed.
//
//	Storage allocated will be freed by SNMP or by the TrapExtension routine after the trap
//	has been sent. If an error conditions occurs during the building of the varbind, then
//	any allocated storage must be freed in this routine.
//

	varBindEntry = (PVarBindQueue) SNMP_malloc(sizeof(VarBindQueue));	// get varbind queue entry storage

	if (varBindEntry == NULL)
	{
		WriteTrace(0x14,"BuildTrapBuffer: Unable to allocate storage for varbind queue entry. Trap not sent.\n");
		WriteLog(SNMPELEA_CANT_ALLOCATE_VARBIND_ENTRY_STORAGE);

		FreeArrays(EventBuffer->NumStrings, lpStrLenArray, lpStringArray);		// free allocated storage

		WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with FALSE\n");
		return(FALSE);							// exit
	}

	WriteTrace(0x00,"BuildTrapBuffer: Storage allocated for varbind queue entry at address at %08X\n",
		varBindEntry);

	varBindEntry->lpNextQueueEntry = NULL;								// set forward pointer to null
	varBindEntry->dwEventID = EventBuffer->EventID; 					// set event id
	varBindEntry->dwEventTime = EventBuffer->TimeGenerated - dwTimeZero;// set event time
	varBindEntry->fProcessed = FALSE;									// indicate trap not processed yet

	varBindEntry->lpVariableBindings = (RFC1157VarBindList *) SNMP_malloc(sizeof(RFC1157VarBindList));	// allocate storage for varbind list

	if (varBindEntry->lpVariableBindings == NULL)
	{
		WriteTrace(0x14,"BuildTrapBuffer: Unable to allocate storage for varbind list. Trap not sent.\n");
		WriteLog(SNMPELEA_CANT_ALLOC_VARBIND_LIST_STORAGE);

		FreeArrays(EventBuffer->NumStrings, lpStrLenArray, lpStringArray);	// free allocated storage
		SNMP_free(varBindEntry);											// free varbind entry

		WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with FALSE\n");
		return(FALSE);							// exit
	}

	WriteTrace(0x00,"BuildTrapBuffer: Storage allocated for varbind list at address at %08X\n",
		varBindEntry->lpVariableBindings);

	varBindEntry->lpVariableBindings->list = (RFC1157VarBind *) SNMP_malloc(
		(EventBuffer->NumStrings+5) * sizeof(RFC1157VarBind));	// allocate storage for varbinds

	if (varBindEntry->lpVariableBindings->list == NULL)
	{
		WriteTrace(0x14,"BuildTrapBuffer: Unable to allocate storage for varbind. Trap not sent.\n");
		WriteLog(SNMPELEA_ERROR_ALLOC_VAR_BIND);

		FreeArrays(EventBuffer->NumStrings, lpStrLenArray, lpStringArray);	// free allocated storage
		SNMP_free(varBindEntry->lpVariableBindings);						// free varbind list storage
		SNMP_free(varBindEntry);											// free varbind entry

		WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with FALSE\n");
		return(FALSE);							// exit
	}

	WriteTrace(0x00,"BuildTrapBuffer: Storage allocated for varbind array at address at %08X\n",
		varBindEntry->lpVariableBindings->list);

	varBindEntry->lpVariableBindings->len = EventBuffer->NumStrings+5;		// set # of varbinds

	WriteTrace(0x00,"BuildTrapBuffer: Number of varbinds present set to %lu\n",
		varBindEntry->lpVariableBindings->len);

	varBindEntry->enterprise = (AsnObjectIdentifier *) SNMP_malloc(sizeof(AsnObjectIdentifier));	// allocate storage for entprise OID

	if (varBindEntry->enterprise == NULL)
	{
		WriteTrace(0x14,"BuildTrapBuffer: Unable to allocate storage for enterprise OID. Trap not sent.\n");
		WriteLog(SNMPELEA_CANT_ALLOC_ENTERPRISE_OID);

		FreeArrays(EventBuffer->NumStrings, lpStrLenArray, lpStringArray);	// free allocated storage
		SNMP_free(varBindEntry->lpVariableBindings->list);					// free varbind storage
		SNMP_free(varBindEntry->lpVariableBindings);						// free varbind list storage
		SNMP_free(varBindEntry);											// free varbind entry

		WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with FALSE\n");
		return(FALSE);							// exit
	}

	WriteTrace(0x00,"BuildTrapBuffer: Storage allocated for enterprise OID at address at %08X\n",
		varBindEntry->enterprise);

	if ( !StrToOid((char *) rsRegStruct.szOID, varBindEntry->enterprise) )
	{
		WriteTrace(0x14,"BuildTrapBuffer: Unable to convert OID from buffer. Trap not sent.\n");
		WriteLog(SNMPELEA_CANT_CONVERT_ENTERPRISE_OID);

		FreeArrays(EventBuffer->NumStrings, lpStrLenArray, lpStringArray);	// free allocated storage
		SNMP_free(varBindEntry->lpVariableBindings->list);					// free varbind storage
		SNMP_free(varBindEntry->lpVariableBindings);						// free varbind list storage
		SNMP_free(varBindEntry->enterprise);								// free storage for enterprise OID
		SNMP_free(varBindEntry);											// free varbind entry

		WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with FALSE\n");
		return(FALSE);							// exit
	}

	size = BASE_PDU_SIZE + (varBindEntry->enterprise->idLength) * sizeof(UINT);
	size += varBindEntry->lpVariableBindings->len * sizeof(RFC1157VarBind);

	for (i = 0; i < varBindEntry->lpVariableBindings->len; i++)
	{

//remove the #if 0 if no control characters are to be left in the varbinds
#if 0
		char *tmp = lpStringArray[i];

		for (int m=0; m < lpStrLenArray[i]; m++)
		{
			if (!tmp)
			{
				break;
			}

			if ((*tmp < 32) || (*tmp > 126))
			{
				*tmp = 32; //32 is the space char
			}

			tmp++;
		}
#endif

		WriteTrace(0x00,"BuildTrapBuffer: String %lu is %s\n", i, lpStringArray[i]);

		varBindEntry->lpVariableBindings->list[i].value.asnValue.string.length = lpStrLenArray[i];	// get string length
		size += lpStrLenArray[i];																	// add to total size
		varBindEntry->lpVariableBindings->list[i].value.asnValue.string.stream = (PUCHAR) lpStringArray[i]; // point to string
		varBindEntry->lpVariableBindings->list[i].value.asnValue.string.dynamic = TRUE; 			// indicate dynamically allocated
		varBindEntry->lpVariableBindings->list[i].value.asnType = ASN_RFC1213_DISPSTRING;			// indicate type of object

		
		strcpy(szTempBuffer, TEXT("."));
		_ultoa(i+1, szTempBuffer2, 10); 				// convert loop counter to string
		strcat(szTempBuffer, szTempBuffer2);				
		strcat(szTempBuffer, TEXT(".0"));				// stick in the .0
		WriteTrace(0x00,"BuildTrapBuffer: Current OID name is %s\n", szTempBuffer);

		if ( !StrToOid((char *)&szTempBuffer, &varBindEntry->lpVariableBindings->list[i].name) )
		{
			WriteTrace(0x14,"BuildTrapBuffer: Unable to convert appended OID for variable binding %lu. Trap not sent.\n",i);
			FreeVarBind(i+1, varBindEntry->lpVariableBindings); 				// free varbind information
			SNMP_free(varBindEntry->enterprise->ids);							// free enterprise OID field
			SNMP_free(varBindEntry->enterprise);							// free enterprise OID field
			SNMP_free(varBindEntry->lpVariableBindings->list);					// free varbind storage
			SNMP_free(varBindEntry->lpVariableBindings);						// free varbind list storage
			SNMP_free(varBindEntry);											// free varbind entry
				
			for (int k = i + 1; k < EventBuffer->NumStrings + 5; k++)
			{
				if (lpStrLenArray[k] != 0)
					SNMP_free(lpStringArray[k]);
			}

			FreeArrays(EventBuffer->NumStrings, lpStrLenArray, lpStringArray, FALSE);	// free allocated storage
			WriteTrace(0x0a,"BuildTrapBuffer: Freeing storage for string array %08X\n", lpStringArray);
			SNMP_free(lpStringArray);
		}

		WriteTrace(0x00,"BuildTrapBuffer: Current OID address is %08X\n", &varBindEntry->lpVariableBindings->list[i].name);

		size += varBindEntry->lpVariableBindings->list[i].name.idLength * sizeof(UINT);
	}

	WriteTrace(0x0a,"BuildTrapBuffer: All variable bindings have been built, size of %lu\n",
		size);

	if (fTrimFlag)						// call trim routine if requested
	{
		if (size > nMaxTrapSize)							// if trap is too big to send
		{
			size = TrimTrap(varBindEntry->lpVariableBindings, size, rsRegStruct.fLocalTrim);	// trim trap data
			WriteTrace(0x0a,"BuildTrapBuffer: TrimTrap returned new size of %lu\n", size);

			if (size == 0 || size > nMaxTrapSize)
			{
				WriteTrace(0x14,"BuildTrapBuffer: TrimTrap could not trim buffer. Trap not sent\n");
				WriteLog(SNMPELEA_TRIM_TRAP_FAILURE);

				FreeVarBind(varBindEntry->lpVariableBindings->len, varBindEntry->lpVariableBindings);	// free varbind information
				SNMP_free(varBindEntry->enterprise->ids);							// free enterprise OID field
				SNMP_free(varBindEntry->enterprise);							// free enterprise OID field
				FreeArrays(EventBuffer->NumStrings, lpStrLenArray, lpStringArray, FALSE);	// free allocated storage
				SNMP_free(varBindEntry->lpVariableBindings->list);					// free varbind storage
				SNMP_free(varBindEntry->lpVariableBindings);						// free varbind list storage
				SNMP_free(varBindEntry);												// free varbind entry
				WriteTrace(0x0a,"BuildTrapBuffer: Freeing storage for string array %08X\n", lpStringArray);
				SNMP_free(lpStringArray);
				WriteTrace(0x00,"BuildTrapBuffer: Notify event handle is %08X\n", hEventNotify);
				WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with FALSE\n");
				return(FALSE);								// exit, all is not well
			}
		}
	}

	if ( !AddBufferToQueue(varBindEntry) )			// add this buffer to the queue
	{
		WriteTrace(0x14,"BuildTrapBuffer: Unable to add trap buffer to queue. Trap not sent.\n");

		FreeVarBind(varBindEntry->lpVariableBindings->len, varBindEntry->lpVariableBindings);	// free varbind information
		SNMP_free(varBindEntry->enterprise->ids);							// free enterprise OID field
		SNMP_free(varBindEntry->enterprise);
		FreeArrays(EventBuffer->NumStrings, lpStrLenArray, lpStringArray, FALSE);	// free allocated storage
		WriteTrace(0x0a,"BuildTrapBuffer: Freeing storage for string array %08X\n", lpStringArray);
		SNMP_free(lpStringArray);
		SNMP_free(varBindEntry->lpVariableBindings->list);					// free varbind storage
		SNMP_free(varBindEntry->lpVariableBindings);						// free varbind list storage
		SNMP_free(varBindEntry);											// free varbind entry

		WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with FALSE\n");
		return(FALSE);							// exit
	}

	WriteTrace(0x0a,"BuildTrapBuffer: Freeing storage for string array %08X\n", lpStringArray);
	SNMP_free(lpStringArray);

	WriteTrace(0x0a,"BuildTrapBuffer: Freeing storage for string length array %08X\n", lpStrLenArray);
	SNMP_free(lpStrLenArray);

	WriteTrace(0x00,"BuildTrapBuffer: Notify event handle is %08X\n", hEventNotify);
	WriteTrace(0x0a,"BuildTrapBuffer: Exiting BuildTrapBuffer with TRUE\n");

	return(TRUE);								// exit, all is well
}

void Position_LogfilesToBootTime(BOOL* fValidHandles, PHANDLE phWaitEventPtr, DWORD* dwRecId)
{
	UINT count;
	HANDLE	hLogHandle;
	PEVENTLOGRECORD EventBuffer;
	PEVENTLOGRECORD pOrigEventBuffer;
	DWORD dwBufferSize = LOG_BUF_SIZE;
	DWORD lastError;
	BOOL fContinue;
	DWORD dwEventSize;
	DWORD dwEventNeeded;

	EventBuffer = (PEVENTLOGRECORD) SNMP_malloc(dwBufferSize);
	pOrigEventBuffer = EventBuffer; 	// save start of buffer

	if ( EventBuffer == NULL )
	{
		WriteTrace(0x14,"Position_LogfilesToBootTime: Error allocating memory for log event record\n");
		WriteTrace(0x14,"Position_LogfilesToBootTime: Alert will not be processed\n");
		WriteLog(SNMPELEA_ERROR_LOG_BUFFER_ALLOCATE_BAD);	// log error message
		return;
	}

	for (count = 0; count < uNumEventLogs; count++)
	{
		if (!fValidHandles[count])
		{
			continue;
		}

		hLogHandle = *(phEventLogs+count);
		fContinue = TRUE;

		while(fContinue) 	// read event log until EOF or boot time
		{
			EventBuffer = pOrigEventBuffer;
			WriteTrace(0x00,"Position_LogfilesToBootTime: Log event buffer is at address %08X\n",
				EventBuffer);
			WriteTrace(0x0a,"Position_LogfilesToBootTime: Reading log event for handle %08X\n",
				hLogHandle);

			if ( !ReadEventLog(hLogHandle,
				EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ,
				0,
				(LPVOID) EventBuffer,
				dwBufferSize,
				&dwEventSize,
				&dwEventNeeded) )
			{
				lastError = GetLastError(); 	// save error status
				
				if (lastError == ERROR_INSUFFICIENT_BUFFER)
				{
					EventBuffer = (PEVENTLOGRECORD) SNMP_realloc((void*)EventBuffer, dwEventNeeded);

					if ( EventBuffer == NULL )
					{
						WriteTrace(0x14,"Position_LogfilesToBootTime: Error reallocating memory for log event record\n");
						WriteTrace(0x14,"Position_LogfilesToBootTime: Alert will not be processed\n");
						WriteLog(SNMPELEA_ERROR_LOG_BUFFER_ALLOCATE_BAD);	// log error message
    					break;
					}

                    pOrigEventBuffer = EventBuffer;
                    dwBufferSize = dwEventNeeded;

					if (!ReadEventLog(hLogHandle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ, 0,
						(LPVOID) EventBuffer, dwBufferSize, &dwEventSize, &dwEventNeeded))
					{
						lastError = GetLastError();
					}
				}

				if (lastError != ERROR_SUCCESS)
				{
					if (lastError == ERROR_HANDLE_EOF)
					{
						WriteTrace(0x0a,"Position_LogfilesToBootTime: END OF FILE of event log is reached\n");
					}
					else
					{//doesn't matter what the error was, reset the eventlog handle
						if ( !ReopenLog(count, phWaitEventPtr) )	// reopen log?
						{
							fValidHandles[count]= FALSE; //this log is no good!
							break;					// if no reopen, exit loop
						}

						if (lastError == ERROR_EVENTLOG_FILE_CHANGED)
						{		// then log file must have been cleared
							hLogHandle = *(phEventLogs+count); // load new handle
							continue;					// if okay, must reread records
						}
						else
						{//Unknown Error! Get to the last record and continue
							WriteTrace(0x14,"Position_LogfilesToBootTime: Error reading event log %08X record is %lu\n",
								hLogHandle, lastError);
							WriteLog(SNMPELEA_ERROR_READ_LOG_EVENT,
								HandleToUlong(hLogHandle), lastError); 	// log error message

							DisplayLogRecord(EventBuffer,	// display log record
								dwEventSize,				// size of this total read
								dwEventNeeded); 			// needed for next read
							
							hLogHandle = *(phEventLogs+count); // load new handle

							if (!Position_to_Log_End(hLogHandle))
							{
								fValidHandles[count]= FALSE; //this log is no good!
								break;
							}
						}
					}
					break;			// exit: finished reading this event log
				}
			} // end unable to ReadEventLog

			while (dwEventSize)
			{
				DisplayLogRecord(EventBuffer,	// display log record
					dwEventSize,				// size of this total read
					dwEventNeeded); 			// needed for next read

				if (EventBuffer->TimeGenerated > dwLastBootTime)
				{
					dwRecId[count] = EventBuffer->RecordNumber;
				}
				else
				{
					fContinue = FALSE;
					break;
				}

				dwEventSize -= EventBuffer->Length; 	// drop by length of this record
				EventBuffer = (PEVENTLOGRECORD) ((LPBYTE) EventBuffer +
					EventBuffer->Length);				// point to next record
			}
		} // end while(TRUE) , finished reading this event log
	} //end while(count <= uNumEventLogs)

	WriteTrace(0x0a,"Position_LogfilesToBootTime: Freeing log event buffer %08X\n",
		pOrigEventBuffer);
	SNMP_free(pOrigEventBuffer);  // free event log record buffer
}

DWORD  GetLastBootTime()
{
	HKEY hKeyPerflib009;
	DWORD retVal = 0;

    LONG status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
    TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib\\009"),
    0, KEY_READ, &hKeyPerflib009);

	if (status != ERROR_SUCCESS)
	{
		return retVal;
	}

	DWORD dwMaxValueLen = 0;
    status = RegQueryInfoKey( hKeyPerflib009,
        NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, &dwMaxValueLen, NULL, NULL);

	if (dwMaxValueLen == 0)
	{
		return retVal;
	}

    DWORD BufferSize = dwMaxValueLen + 1;
    unsigned char* lpNameStrings = new unsigned char[BufferSize];

    // since we are here the counters should be accessible.
    // in case we fail to load them, just bail out.
    status = RegQueryValueEx( hKeyPerflib009,
                TEXT("Counter"), NULL, NULL, lpNameStrings, &BufferSize );
    if (status != ERROR_SUCCESS || BufferSize == 0)
    {
        delete [] lpNameStrings;
        return retVal;
    }

	DWORD dwTime = 0;
	DWORD dwSystem = 0;

    for(TCHAR* lpCurrentString = (TCHAR*)lpNameStrings; *lpCurrentString;
         lpCurrentString += (_tcslen(lpCurrentString)+1) )
    {
        DWORD dwCounter = _ttol( lpCurrentString );
        lpCurrentString += (_tcslen(lpCurrentString)+1);

		if (0 == _tcsicmp((LPTSTR)lpCurrentString, TEXT("System")))
		{
			dwSystem = dwCounter;

			if (dwTime != 0)
			{
				break;
			}
		}
		else if (0 == _tcsicmp((LPTSTR)lpCurrentString, TEXT("System Up Time")))
		{
			dwTime = dwCounter;

			if (dwSystem != 0)
			{
				break;
			}

		}
    }

	PPERF_DATA_BLOCK PerfData = (struct _PERF_DATA_BLOCK *)lpNameStrings;
	TCHAR sysBuff[40];
	_ultot(dwSystem, (TCHAR*)sysBuff, 10);
	DWORD tmpBuffsz = BufferSize;
	status = RegQueryValueEx(HKEY_PERFORMANCE_DATA,
                               sysBuff,
                               NULL,
                               NULL,
                               (LPBYTE) PerfData,
                               &BufferSize);

	
	while (status == ERROR_MORE_DATA)
	{
		if (BufferSize <= tmpBuffsz)
		{
			tmpBuffsz = tmpBuffsz * 2;
			BufferSize = tmpBuffsz;
		}

		delete [] PerfData;
		PerfData = (struct _PERF_DATA_BLOCK *) new unsigned char[BufferSize];
		status = RegQueryValueEx(HKEY_PERFORMANCE_DATA,
                               sysBuff,
                               NULL,
                               NULL,
                               (LPBYTE) PerfData,
                               &BufferSize);
	}

	if (status == ERROR_SUCCESS)
	{

		// 5/22/98 mikemid Fix for SMS Bug1 #20662
        // SNMP trap agent fails with an invalid event handle only on NT 3.51.
        // Now I have no idea why it fails after we close this key but it does. A
        // debugger shows that the WaitForMultipleObjects() event array contains valid
        // event handles, yet commenting out this next line allows this to work properly.
        // The SNMP trap service doesn't even use this key.
        // According to the docs, we want to close this key after using it "so that
        // network transports and drivers can be removed or installed". Well, since this
        // is an already open system key, and we're local anyway, this shouldn't affect
        // anything else.
        // I'm chalking this one up as "extreme weirdness in NT 3.51".
        //===============================================================================
        // RegCloseKey(HKEY_PERFORMANCE_DATA);

		PPERF_OBJECT_TYPE PerfObj = (PPERF_OBJECT_TYPE)((PBYTE)PerfData +
															PerfData->HeaderLength);

		for(DWORD i=0; (i < PerfData->NumObjectTypes) && (retVal == 0); i++ )
		{
			if (PerfObj->ObjectNameTitleIndex == dwSystem)
			{
				PPERF_COUNTER_DEFINITION PerfCntr = (PPERF_COUNTER_DEFINITION) ((PBYTE)PerfObj +
																	PerfObj->HeaderLength);
				//only ever one instance of system so no need to check
				//for instances of system, just get the counter block.
				PPERF_COUNTER_BLOCK PtrToCntr = (PPERF_COUNTER_BLOCK) ((PBYTE)PerfObj +
								PerfObj->DefinitionLength );

				// Retrieve all counters.
				for(DWORD j=0; j < PerfObj->NumCounters; j++ )
				{
					if (dwTime == PerfCntr->CounterNameTitleIndex)
					{
						//got the time counter, get the data!
						FILETIME timeRebootf;
						memcpy(&timeRebootf, ((PBYTE)PtrToCntr + PerfCntr->CounterOffset), sizeof(FILETIME));
						SYSTEMTIME timeReboots;

						if (FileTimeToSystemTime(&timeRebootf, &timeReboots))
						{
							struct tm timeReboott;
							timeReboott.tm_year = timeReboots.wYear - 1900;
							timeReboott.tm_mon = timeReboots.wMonth - 1;
							timeReboott.tm_mday = timeReboots.wDay;
							timeReboott.tm_hour = timeReboots.wHour;
							timeReboott.tm_min = timeReboots.wMinute;
							timeReboott.tm_sec = timeReboots.wSecond;
							timeReboott.tm_isdst = 0;
						    time_t tt = mktime(&timeReboott);
							
							if(tt != 0xffffffff)
							{
								tt -= _timezone;
								retVal = (DWORD) tt;
							}
						}

						break;
					}

					// Get the next counter.
					PerfCntr = (PPERF_COUNTER_DEFINITION)((PBYTE)PerfCntr +
									PerfCntr->ByteLength);
				}
			}

			PerfObj = (PPERF_OBJECT_TYPE)((PBYTE)PerfObj + PerfObj->TotalByteLength);
		}
	}

	if (PerfData != NULL)
	{
		delete [] PerfData;
	}

	return retVal;
}


extern "C" {
DWORD
SnmpEvLogProc(
	 IN VOID
	 )

/*++

Routine Description:

	This is the log processing routine for the SNMP event log extension agent DLL.
	This is where control is passed from DLL initialization in order to determine
	if an event log entry is to generate an SNMP trap.

	An event is created for every log event handle that is opened.
	NotifyChangeEventLog is then called for each log event handle in turn, to allow
	the associated event to be signaled when a change is made to an event log.

	At this point the function waits on the list of events waiting for a log
	event to occur or for a request from the DLL process termination routine to
	shutdown. If a log eventoccurs, the log event record is analyzed, information
	extracted from the registry and, if requested, a trap buffer is built and sent to
	the trap processing routine. This routine is scheduled via a notification event.

	Once the DLL termination event is signaled, all event handles are
	closed and the thread is terminated. This returns control to process termination
	routine in the main DLL.


Arguments:

	None.

Return Value:

	A double word return value is required by the CreateThread API. This
	routine will return a value of zero (0) if all functions performed as
	expected. A value of 1 is returned if a problem was encountered.


Notes:

	ExitThread is used to return control. The return(0) statementat the end of this
	function is include only to avoid a compiler error.


--*/

{

	PHANDLE 		phWaitEventPtr; 	// points to Wait Event Handles
	HANDLE			hLogHandle; 		// handle to log stuff
	HMODULE 		hPrimHandle;		// handle to secondary parameter insertion module
	DWORD			dwEventOccur;		// event number from wait
	PEVENTLOGRECORD EventBuffer;		// Event Record from Event Log
	PEVENTLOGRECORD pOrigEventBuffer;	// original event buffer pointer
	DWORD			dwEventSize;		// for create event
	DWORD			dwEventNeeded;		// for create event
	UINT			i;					// temporary loop variable
	LPTSTR			lpszThisModuleName; // temporary for module name
	LPTSTR			lpszLogName;		// temporary for log name
	TCHAR			szThisEventID[34];	// temporary for event ID
	ULONG			ulValue;			// temporary for event ID
	DWORD			lastError;			// status of last function error
	REGSTRUCT		rsRegistryInfo; 	// structure for registry information
	BOOL			fNewTrap = FALSE;	// trap ready flag
	BOOL*			fValidHandles;
	DWORD*			dwRecId;
	DWORD dwBufferSize = LOG_BUF_SIZE;
	DWORD dwReadOptions;

	WriteTrace(0x0a,"SnmpEvLogProc: Entering SnmpEvLogProc routine....\n");
	WriteTrace(0x00,"SnmpEvLogProc: Value of hStopAll is %08X\n",hStopAll);
	WriteTrace(0x00,"SnmpEvLogProc: Value of phEventLogs is %08X\n",phEventLogs);

	for (i = 0; i < uNumEventLogs; i++)
	{
		WriteTrace(0x00,"SnmpEvLogProc: Event log %s(%lu) has handle of %08X\n",
			lpszEventLogs+i*(MAX_PATH+1), i, *(phEventLogs+i));
	}

//	WaitEvent structure: all notify-event-write first, stop DLL event, registry change event last

	if (fRegOk)
	{
		phWaitEventPtr = (PHANDLE) SNMP_malloc((uNumEventLogs+2) * HANDLESIZE);
	}
	else
	{
		phWaitEventPtr = (PHANDLE) SNMP_malloc((uNumEventLogs+1) * HANDLESIZE);
	}

	if (phWaitEventPtr == (PHANDLE) NULL)
	{
		WriteTrace(0x14,"SnmpEvLogProc: Unable to allocate memory for wait event array\n");
		WriteLog(SNMPELEA_CANT_ALLOCATE_WAIT_EVENT_ARRAY);

		StopAll();						// show dll in shutdown
		WriteTrace(0x14,"SnmpEvLogProc: SnmpEvLogProc abnormal initialization\n");
		WriteLog(SNMPELEA_ABNORMAL_INITIALIZATION);  // log error message
		DoExitLogEv(1); 			// exit this thread
	}

	fValidHandles = (BOOL*) SNMP_malloc((uNumEventLogs) * sizeof(BOOL));
	
	if (fValidHandles == (BOOL*) NULL)
	{
		WriteTrace(0x14,"SnmpEvLogProc: Unable to allocate memory for boolean array\n");
		WriteLog(SNMPELEA_ALLOC_EVENT);

		StopAll();						// show dll in shutdown
		WriteTrace(0x14,"SnmpEvLogProc: SnmpEvLogProc abnormal initialization\n");
		WriteLog(SNMPELEA_ABNORMAL_INITIALIZATION);  // log error message
		DoExitLogEv(1); 			// exit this thread
	}

	dwRecId = (DWORD*) SNMP_malloc((uNumEventLogs) * sizeof(DWORD));
	
	if (dwRecId == (DWORD*) NULL)
	{
		WriteTrace(0x14,"SnmpEvLogProc: Unable to allocate memory for record ID array\n");
		WriteLog(SNMPELEA_ALLOC_EVENT);

		StopAll();						// show dll in shutdown
		WriteTrace(0x14,"SnmpEvLogProc: SnmpEvLogProc abnormal initialization\n");
		WriteLog(SNMPELEA_ABNORMAL_INITIALIZATION);  // log error message
		DoExitLogEv(1); 			// exit this thread
	}

	for (i = 0; i < uNumEventLogs; i++)
	{
		WriteTrace(0x00,"SnmpEvLogProc: CreateEvent/ChangeNotify loop pass %lu\n",i);
		fValidHandles[i] = TRUE;
		dwRecId[i] = 0;

		if ( (hLogHandle = CreateEvent(
			(LPSECURITY_ATTRIBUTES) NULL,
			FALSE,
			FALSE,
			(LPTSTR) NULL)) == NULL )
		{
			lastError = GetLastError(); 	// save error status
			WriteTrace(0x14,"SnmpEvLogProc: Error creating event for log notify is %lu\n",
				lastError);
			WriteLog(SNMPELEA_ERROR_CREATE_LOG_NOTIFY_EVENT, lastError);	// log error message
		}
		else
		{
			*(phWaitEventPtr+i) = hLogHandle;
		}

		WriteTrace(0x00,"SnmpEvLogProc: CreateEvent returned handle of %08X\n",
			hLogHandle);
		WriteTrace(0x00,"SnmpEvLogProc: Handle address is %08X\n",phWaitEventPtr+i);
		WriteTrace(0x00,"SnmpEvLogProc: Handle contents by pointer is %08X\n",
			*(phWaitEventPtr+i));

		// Associate each event log to its notify-event-write event handle
		WriteTrace(0x00,"SnmpEvLogProc: ChangeNotify on log handle %08X\n",
			*(phEventLogs+i));
		WriteTrace(0x00,"SnmpEvLogProc: Address of log handle %08X\n",phEventLogs+i);

		if (!NotifyChangeEventLog(*(phEventLogs+i),*(phWaitEventPtr+i)))
		{
			lastError = GetLastError();
			WriteTrace(0x14,"SnmpEvLogProc: NotifyChangeEventLog failed with code %lu\n",
				lastError);
			WriteLog(SNMPELEA_ERROR_LOG_NOTIFY, lastError); // log error message
		}
		else
		{
			WriteTrace(0x00,"SnmpEvLogProc: ChangeNotify was successful\n");
		}

	} // end for

	*(phWaitEventPtr+uNumEventLogs) = hStopAll; // set shutdown event

	if (fRegOk)
	{
		*(phWaitEventPtr+uNumEventLogs+1) = hRegChanged;	// set registry changed event
	}

	WriteTrace(0x00,"SnmpEvLogProc: Termination event is set to %08X\n",
		*(phWaitEventPtr+uNumEventLogs));
	WriteTrace(0x00,"SnmpEvLogProc: Address of termination event is %08X\n",
		phWaitEventPtr+uNumEventLogs);
	WriteTrace(0x00,"SnmpEvLogProc: On entry, handle value is %08X\n",
		hStopAll);

	if (fRegOk)
	{
		WriteTrace(0x00,"SnmpEvLogProc: Registry notification event is set to %08X\n",
			*(phWaitEventPtr+uNumEventLogs+1));
		WriteTrace(0x00,"SnmpEvLogProc: Address of registry notification event is %08X\n",
			phWaitEventPtr+uNumEventLogs+1);
		WriteTrace(0x00,"SnmpEvLogProc: On entry, handle value is %08X\n",
			hRegChanged);
	}

	hMutex = CreateMutex(					// create mutex object
		NULL,								// no security attributes
		TRUE,								// initial ownership desired
		MUTEX_NAME);						// name of mutex object

	lastError = GetLastError(); 			// get any error codes

	WriteTrace(0x0a,"SnmpEvLogProc: CreateMutex returned handle of %08X and reason code of %lu\n",
		hMutex, lastError);

	if (hMutex == NULL)
	{
		WriteTrace(0x14,"SnmpEvLogProc: Unable to create Mutex object %s, reason code %lu\n",
			MUTEX_NAME, lastError);
		WriteLog(SNMPELEA_CREATE_MUTEX_ERROR, MUTEX_NAME, lastError);
		StopAll();						// indicate dll shutdown
		CloseEvents(phWaitEventPtr);	// close event handles
		DoExitLogEv(1); 				// exit this thread
	}

	WriteTrace(0x0a,"SnmpEvLogProc: Created mutex object handle is %08X\n", hMutex);

	WriteTrace(0x0a,"SnmpEvLogProc: Releasing mutex object %08X\n", hMutex);
	if (!ReleaseMutex(hMutex))
	{
		lastError = GetLastError(); 	// get error information
		WriteTrace(0x14,"SnmpEvLogProc: Unable to release mutex object for reason code %lu\n",
			lastError);
		WriteLog(SNMPELEA_RELEASE_MUTEX_ERROR, lastError);
	}

	if (fDoLogonEvents)
	{
		DWORD dwboot = GetLastBootTime();

		if ((dwLastBootTime == 0) || (dwboot > dwLastBootTime))
		{
			RegSetValueEx(hkRegResult, EXTENSION_LASTBOOT_TIME, 0, REG_DWORD, (CONST BYTE *)&dwboot, 4);
		}

		if (dwboot > dwLastBootTime)
		{
			dwLastBootTime = dwboot;
		}
		else
		{
			fDoLogonEvents = FALSE;
		}
	}

// Wait repeatedly for any event in phWaitEventPtr to occur
	EventBuffer = (PEVENTLOGRECORD) SNMP_malloc(dwBufferSize);
	pOrigEventBuffer = EventBuffer; 	// save start of buffer
	WriteTrace(0x0a,"SnmpEvLogProc: Allocating memory for log event record\n");

	if ( EventBuffer == NULL )
	{
		WriteTrace(0x14,"SnmpEvLogProc: Error allocating memory for log event record\n");
		WriteLog(SNMPELEA_ERROR_LOG_BUFFER_ALLOCATE_BAD);	// log error message
		StopAll();						// indicate dll shutdown
		CloseEvents(phWaitEventPtr);	// close event handles
		DoExitLogEv(1); 				// exit this thread
	}

	while (TRUE)	// or until hell freezes, whichever comes first
	{
		fNewTrap = FALSE;				// reset trap built indicator
		WriteTrace(0x0a,"SnmpEvLogProc: Waiting for event to occur\n");

		WriteTrace(0x0a,"SnmpEvLogProc: Normal event wait in progress\n");

		if (fDoLogonEvents && !(fThresholdEnabled && fThreshold))
		{
			fDoLogonEvents = FALSE;
			dwEventOccur = 0;
			Position_LogfilesToBootTime(fValidHandles, phWaitEventPtr, dwRecId);
		}
		else
		{
			if (!fRegOk)
			{
				if (nTraceLevel == 0)
				{
					for (i = 0; i < uNumEventLogs+1; i++)
					{
						WriteTrace(0x00,"SnmpEvLogProc: Event handle %lu is %08X\n", i, *(phWaitEventPtr+i));
					}
				}

				dwEventOccur = WaitForMultipleObjects(
					uNumEventLogs+1,				// number of events
					phWaitEventPtr, 				// array of event handles
					FALSE,							// no overlapped i/o
					INFINITE);						// no wait time-out
			}
			else
			{
				if (nTraceLevel == 0)
				{
					for (i = 0; i < uNumEventLogs+2; i++)
					{
						WriteTrace(0x00,"SnmpEvLogProc: Event handle %lu is %08X\n", i, *(phWaitEventPtr+i));
					}
				}
				dwEventOccur = WaitForMultipleObjects(
					uNumEventLogs+2,				// number of events
					phWaitEventPtr, 				// array of event handles
					FALSE,							// no overlapped i/o
					INFINITE);						// no wait time-out
			}
		}

		lastError = GetLastError(); 	// save error status
		WriteTrace(0x0a,"SnmpEvLogProc: EventOccur value: %lu\n", dwEventOccur);

		if (dwEventOccur == MAXDWORD)						// Wait didn't work
		{
			WriteTrace(0x14,"SnmpEvLogProc: Error waiting for event array is %lu\n",
				lastError); 				// trace error message
			WriteLog(SNMPELEA_ERROR_WAIT_ARRAY, lastError); // log error message
			StopAll();						// indicate dll shutdown
			CloseEvents(phWaitEventPtr);	// close event handles
			DoExitLogEv(1); 				// exit this thread
		}

		if (dwEventOccur == uNumEventLogs)
		{
			WriteTrace(0x0a,"SnmpEvLogProc: Event detected DLL shutdown\n");
			CloseEvents(phWaitEventPtr);	// close event handles

			WriteTrace(0x0a,"SnmpEvLogProc: Closing mutex handle %08X\n", hMutex);
			CloseHandle(hMutex);

			break;							// exit this loop
		}

		if (fRegOk)
		{
			if (dwEventOccur == uNumEventLogs+1)
			{
				WriteTrace(0x0a,"SnmpEvLogProc: Event detected registry key change. Rereading registry parameters.\n");
				if (!Read_Registry_Parameters())
				{
					WriteTrace(0x14,"SnmpEvLogProc: Error reading registry information. DLL is terminating.\n");
					WriteLog(SNMPELEA_REGISTRY_INIT_ERROR);
					StopAll();						// indicate dll shutdown
					CloseEvents(phWaitEventPtr);	// close event handles
					DoExitLogEv(1); 				// exit this thread
				}
				else
				{
					WriteTrace(0x0a,"SnmpEvLogProc: Registry parameters have been refreshed.\n");
					continue;						// skip other event log processing stuff
				}
			}
		}

		DWORD dwEvnt = 0;
		DWORD count = 0;

		while (count <= uNumEventLogs)
		{
			dwEvnt++;

			if (dwEvnt == uNumEventLogs)
			{
				dwEvnt = 0;
			}

			if (!fValidHandles[dwEvnt])
			{
				count++;
				continue;
			}

			hLogHandle = *(phEventLogs+dwEvnt);
			hPrimHandle = *(phPrimHandles+dwEvnt);
			lpszLogName = lpszEventLogs+dwEvnt*(MAX_PATH+1);

			WriteTrace(0x0a,"SnmpEvLogProc: Event detected log record written for %s - %lu - %08X\n",
				lpszLogName, dwEvnt, hLogHandle);

			if (fThresholdEnabled && fThreshold)
			{
				WriteTrace(0x0a,"SnmpEvLogProc: Performance threshold flag is on. No data will be processed.\n");
				break;
			}

			while(TRUE) 						// read event log until EOF
			{
				EventBuffer = pOrigEventBuffer;
				WriteTrace(0x00,"SnmpEvLogProc: Log event buffer is at address %08X\n",
					EventBuffer);
				WriteTrace(0x0a,"SnmpEvLogProc: Reading log event for handle %08X\n",
					hLogHandle);
				
				if (dwRecId[dwEvnt] != 0)
				{
					dwReadOptions = EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ;
				}
				else
				{
					dwReadOptions = EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ;
				}

				if ( !ReadEventLog(hLogHandle,
					dwReadOptions,
					dwRecId[dwEvnt],
					(LPVOID) EventBuffer,
					dwBufferSize,
					&dwEventSize,
					&dwEventNeeded) )
				{
					lastError = GetLastError(); 	// save error status
					
					// mikemid 12/01/97, hotfix for SMS Bug1 # 11963.
					// If we ran out of buffer space, take the returned value in dwEventNeeded
					// and realloc a big enough buffer to load in the data.
					//========================================================================
					if (lastError == ERROR_INSUFFICIENT_BUFFER)
					{

						dwBufferSize = dwEventNeeded;

						EventBuffer = (PEVENTLOGRECORD) SNMP_realloc((void*)EventBuffer, dwBufferSize);

						if ( EventBuffer == NULL )
						{
							WriteTrace(0x14,"SnmpEvLogProc: Error reallocating memory for log event record\n");
							WriteTrace(0x14,"SnmpEvLogProc: Alert will not be processed\n");
							WriteLog(SNMPELEA_ERROR_LOG_BUFFER_ALLOCATE_BAD);	// log error message
							dwBufferSize = 0;
							break;
						}

						lastError = ERROR_SUCCESS;
						
						pOrigEventBuffer = EventBuffer;

						if (!ReadEventLog(hLogHandle, dwReadOptions, dwRecId[dwEvnt],
							(LPVOID) EventBuffer, dwBufferSize, &dwEventSize, &dwEventNeeded))
						{
							lastError = GetLastError();
							dwRecId[dwEvnt] = 0;
						}
					}

					if (lastError != ERROR_SUCCESS)
					{
						if (lastError == ERROR_HANDLE_EOF)
						{
							WriteTrace(0x0a,"SnmpEvLogProc: END OF FILE of event log is reached\n");
							count++;
						}
						else
						{//doesn't matter what the error was, reset the eventlog handle
							if ( !ReopenLog(dwEvnt, phWaitEventPtr) )	// reopen log?
							{
								fValidHandles[dwEvnt]= FALSE; //this log is no good!
								count++;
								break;					// if no reopen, exit loop
							}

							if (lastError == ERROR_EVENTLOG_FILE_CHANGED)
							{		// then log file must have been cleared
								hLogHandle = *(phEventLogs+dwEvnt); // load new handle
								continue;					// if okay, must reread records
							}
							else
							{//Unknown Error! Get to the last record and continue
								WriteTrace(0x14,"SnmpEvLogProc: Error reading event log %08X record is %lu\n",
									hLogHandle, lastError);
								WriteLog(SNMPELEA_ERROR_READ_LOG_EVENT,
									HandleToUlong(hLogHandle), lastError); 	// log error message

								DisplayLogRecord(EventBuffer,	// display log record
									dwEventSize,				// size of this total read
									dwEventNeeded); 			// needed for next read
								
								hLogHandle = *(phEventLogs+dwEvnt); // load new handle
								count++;

								if (!Position_to_Log_End(hLogHandle))
								{
									fValidHandles[dwEvnt]= FALSE; //this log is no good!
									break;
								}
							}
						}

						break;			// exit: finished reading this event log
					}
				} // end unable to ReadEventLog
				
				dwRecId[dwEvnt] = 0;
				count = 0;
				
				while (dwEventSize)
				{
					DisplayLogRecord(EventBuffer,	// display log record
						dwEventSize,				// size of this total read
						dwEventNeeded); 			// needed for next read

					WriteTrace(0x00,"SnmpEvLogProc: Preparing to read config file values\n");

					lpszThisModuleName = (LPTSTR) EventBuffer + EVENTRECSIZE;

					ulValue = EventBuffer->EventID;
	//				ulValue = ulValue & 0x0000FFFF; // trim off high order stuff
					_ultoa(ulValue, szThisEventID, 10);

					WriteTrace(0x00,"SnmpEvLogProc: Event ID converted to ASCII\n");
					WriteTrace(0x00,"SnmpEvLogProc: Source is %s. Event ID is %s.\n",
						lpszThisModuleName,
						szThisEventID);

					if ( GetRegistryValue(
						lpszThisModuleName,
						szThisEventID,
						lpszLogName,
						EventBuffer->TimeGenerated,
						&rsRegistryInfo) )
					{
						WriteTrace(0x0a,"SnmpEvLogProc: This event is being tracked -- formatting trap buffer\n");
						if ( !BuildTrapBuffer(EventBuffer, rsRegistryInfo, lpszLogName, hPrimHandle) )
						{
							WriteTrace(0x14,"SnmpEvLogProc: Unable to build trap buffer. Trap not sent.\n");
						}
						else
						{
							fNewTrap = TRUE;			// indicate a new trap buffer built
						}

						WriteTrace(0x00,"SnmpEvLogProc: Notify event handle is %08X\n", hEventNotify);	
					}

					dwEventSize -= EventBuffer->Length; 	// drop by length of this record
					EventBuffer = (PEVENTLOGRECORD) ((LPBYTE) EventBuffer +
						EventBuffer->Length);				// point to next record

				}

			} // end while(TRUE) , finished reading this event log

			if (fNewTrap)
			{
				WriteTrace(0x0a,"SnmpEvLogProc: A new trap buffer was added -- posting notification event %08X\n",
					hEventNotify);
				if ( !SetEvent(hEventNotify) )
				{
					lastError = GetLastError(); 			// get error return codes
					WriteTrace(0x14,"SnmpEvLogProc: Unable to post event %08X; reason is %lu\n",
						hEventNotify, lastError);
					WriteLog(SNMPELEA_CANT_POST_NOTIFY_EVENT, HandleToUlong(hEventNotify), lastError);
				}
			}
		} //end while(count < uNumEventLogs)
	} // end while(TRUE) loop

	WriteTrace(0x0a,"SnmpEvLogProc: Freeing log event buffer %08X\n",
		pOrigEventBuffer);
	SNMP_free(pOrigEventBuffer);  // free event log record buffer
	SNMP_free(fValidHandles);
	SNMP_free(dwRecId);
	WriteTrace(0x0a,"SnmpEvLogProc: Exiting SnmpEvLogProc via normal shutdown\n");
	DoExitLogEv(0);
	return(0);						// to appease the compiler

} // end of SnmpEvLogProc

}
