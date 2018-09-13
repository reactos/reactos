/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    perfjob.c

Abstract:

    This file implements an Performance Job Object that presents
	information on the Job Object

Created:

    Bob Watson  8-Oct-1997

Revision History


--*/
//
//  Include Files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <winperf.h>
#include <ntprfctr.h>
#include <perfutil.h>
#include "perfsprc.h"
#include "procmsg.h"
#include "datajob.h"

#define MAX_STR_CHAR	1024
#define	MAX_STR_SIZE	((DWORD)((MAX_STR_CHAR - 1)* sizeof(WCHAR)))
#define MAX_NAME_LENGTH	MAX_PATH

#define BUFFERSIZE 1024

DWORD	dwBufferSize = BUFFERSIZE;

const WCHAR	szJob[] = L"Job";
const WCHAR szObjDirName[] = L"\\BaseNamedObjects";

#define MAX_EVENT_STRINGS	4
WORD	wEvtStringCount;
LPWSTR	szEvtStringArray[MAX_EVENT_STRINGS];

UNICODE_STRING DirectoryName = {(sizeof(szObjDirName) - sizeof(WCHAR)), // name len - NULL
								sizeof(szObjDirName),				   // size of buffer	
								(PWCHAR)szObjDirName};				   // address of buffer

BOOL    bOpenJobErrorLogged = FALSE;

static
PSYSTEM_PROCESS_INFORMATION APIENTRY
GetProcessPointerFromProcessId (
	IN	ULONG_PTR	dwPid
)
{
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    ULONG ProcessBufferOffset = 0;
    BOOLEAN NullProcess;
	
	DWORD	dwIndex = 0;
	BOOL	bFound	= FALSE;

    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) pProcessBuffer;
    while ( TRUE ) {
        // check for Live processes
        //  (i.e. name or threads)

        if ((ProcessInfo->ImageName.Buffer != NULL) ||
            (ProcessInfo->NumberOfThreads > 0)){
                // thread is not Dead
            NullProcess = FALSE;
        } else {
            // thread is dead
            NullProcess = TRUE;
        }

        if (( !NullProcess )  && (dwPid == (HandleToUlong(ProcessInfo->UniqueProcessId)))) {
			// found it so return current value
			bFound = TRUE;
			break;
		} else {
			dwIndex++;
		}
        // exit if this was the last process in list
        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }

        // point to next buffer in list
        ProcessBufferOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
                          &pProcessBuffer[ProcessBufferOffset];
    }

	if (bFound) {
		return ProcessInfo;
	} else {
		return NULL;
	}
}

DWORD APIENTRY
CollectJobObjectData (
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the processor object

Arguments:

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the address of the DWORD to receive the number of objects added
            by this routine
         OUT: the number of objects added by this routine is writted to the
            DWORD pointed to by this argument

   Returns:

             0 if successful, else Win 32 error code of failure

--*/
{
    DWORD   TotalLen;            //  Length of the total return block

    PPERF_INSTANCE_DEFINITION   pPerfInstanceDefinition;
    PJOB_DATA_DEFINITION		pJobDataDefinition;
    PJOB_COUNTER_DATA			pJCD;
    JOB_COUNTER_DATA			jcdTotal;

    NTSTATUS Status;
    HANDLE DirectoryHandle, JobHandle;
    ULONG ReturnedLength;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    POBJECT_NAME_INFORMATION NameInfo;
    OBJECT_ATTRIBUTES Attributes;
	WCHAR	wszNameBuffer[MAX_STR_CHAR];
	DWORD	i, dwSize;
	PUCHAR  Buffer;
	BOOL	bStatus;
	PJOBOBJECT_BASIC_PROCESS_ID_LIST pJobPidList;
	JOBOBJECT_BASIC_ACCOUNTING_INFORMATION JobAcctInfo;

	DWORD	dwWin32Status = ERROR_SUCCESS;
    ACCESS_MASK ExtraAccess = 0;
    ULONG Context = 0;
	DWORD	NumJobInstances = 0;

	// get size of a data block that has 1 instance
    TotalLen = sizeof(JOB_DATA_DEFINITION) +		// object def + counter defs
               sizeof (PERF_INSTANCE_DEFINITION) +	// 1 instance def
               MAX_VALUE_NAME_LENGTH +				// 1 instance name
               sizeof(JOB_COUNTER_DATA);			// 1 instance data block

    if ( *lpcbTotalBytes < TotalLen ) {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        return ERROR_MORE_DATA;
    }

	// cast callers buffer to the object data definition type
    pJobDataDefinition = (JOB_DATA_DEFINITION *) *lppData;

    //
    //  Define Job Object data block
    //

    memcpy(pJobDataDefinition,
           &JobDataDefinition,
           sizeof(JOB_DATA_DEFINITION));

	// set timestamp of this object
    pJobDataDefinition->JobObjectType.PerfTime = SysTimeInfo.CurrentTime;

    // Now collect data for each job object found in system
    //
    //  Perform initial setup
    //
	Buffer = ALLOCMEM(hLibHeap, HEAP_ZERO_MEMORY, dwBufferSize);
	if ((Buffer == NULL)) {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
		// BUGBUG: NEED EVENT LOG MESSAGE HERE to report mem alloc failure
        return ERROR_SUCCESS;
	}

    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                  &pJobDataDefinition[1];

    // adjust TotalLen to be the size of the buffer already in use
    TotalLen = sizeof (JOB_DATA_DEFINITION);

    // zero the total instance buffer
    memset (&jcdTotal, 0, sizeof (jcdTotal));

    //
    //  Open the directory for list directory access
    //
	// this should always succeed since it's a system name we
	// will be querying
	//
    InitializeObjectAttributes( &Attributes,
                                &DirectoryName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );
	
    Status = NtOpenDirectoryObject( &DirectoryHandle,
                                    DIRECTORY_QUERY | ExtraAccess,
                                    &Attributes
                                  );
    if (NT_SUCCESS( Status )) {
		//
		// Get the actual name of the object directory object.
		//

	    NameInfo = (POBJECT_NAME_INFORMATION) &Buffer[0];
		Status = NtQueryObject( DirectoryHandle,
                                 ObjectNameInformation,
                                 NameInfo,
                                 dwBufferSize,
                                 (PULONG) NULL );
    }

    if (NT_SUCCESS( Status )) {
		//
		//  Query the entire directory in one sweep
		//
		for (Status = NtQueryDirectoryObject( DirectoryHandle,
											  Buffer,
											  dwBufferSize,
											  FALSE,
											  FALSE,
											  &Context,
											  &ReturnedLength );
			 NT_SUCCESS( Status );
			 Status = NtQueryDirectoryObject( DirectoryHandle,
											  Buffer,
											  dwBufferSize,
											  FALSE,
											  FALSE,
											  &Context,
											  &ReturnedLength ) ) {

			//
			//  Check the status of the operation.
			//

			if (!NT_SUCCESS( Status )) {
				break;
			}

			//
			//  For every record in the buffer type out the directory information
			//

			//
			//  Point to the first record in the buffer, we are guaranteed to have
			//  one otherwise Status would have been No More Files
			//

			DirInfo = (POBJECT_DIRECTORY_INFORMATION) &Buffer[0];

			while (TRUE) {

				//
				//  Check if there is another record.  If there isn't, then get out
				//  of the loop now
				//

				if (DirInfo->Name.Length == 0) {
					break;
				}

				//
				//  Print out information about the Job
				//

				if (wcsncmp ( DirInfo->TypeName.Buffer, &szJob[0], ((sizeof(szJob)/sizeof(WCHAR)) - 1)) == 0) {
					// this is really a job, so list the name
					dwSize = DirInfo->Name.Length;
					if (dwSize > MAX_STR_SIZE) dwSize = MAX_STR_SIZE;
					memcpy (wszNameBuffer, DirInfo->Name.Buffer, dwSize);
					wszNameBuffer[dwSize/sizeof(WCHAR)] = 0;

					// now query the process ID's for this job

					JobHandle = OpenJobObjectW (
						JOB_OBJECT_QUERY,
						FALSE,
						wszNameBuffer);

//					ASSERT (JobHandle != NULL);

					if (JobHandle != NULL) {

						bStatus = QueryInformationJobObject (
							JobHandle,
							JobObjectBasicAccountingInformation,
							&JobAcctInfo,
							sizeof(JobAcctInfo),
							&ReturnedLength);

//						ASSERT (bStatus == TRUE);
						ASSERT (ReturnedLength == sizeof(JobAcctInfo));

						if (bStatus) {
							// *** create and initialize perf data instance here ***

							// see if this instance will fit
							TotalLen += sizeof(PERF_INSTANCE_DEFINITION) +
									   DWORD_MULTIPLE ((DirInfo->Name.Length + sizeof(WCHAR))) +
									   sizeof (JOB_COUNTER_DATA);

							if ( *lpcbTotalBytes < TotalLen ) {
								*lpcbTotalBytes = 0;
								*lpNumObjectTypes = 0;
								Status = STATUS_NO_MEMORY;
								dwWin32Status =  ERROR_MORE_DATA;
								break;
							}

							MonBuildInstanceDefinition(pPerfInstanceDefinition,
								(PVOID *) &pJCD,
								0,
								0,
								(DWORD)-1,
								wszNameBuffer);

							// test structure for Quadword Alignment
							assert (((DWORD)(pJCD) & 0x00000007) == 0);

							//
							//  Format and collect Process data
							//

							pJCD->CounterBlock.ByteLength = sizeof (JOB_COUNTER_DATA);

							jcdTotal.CurrentProcessorTime += 					
								pJCD->CurrentProcessorTime =
									JobAcctInfo.TotalUserTime.QuadPart +
									JobAcctInfo.TotalKernelTime.QuadPart;

							jcdTotal.CurrentUserTime +=
								pJCD->CurrentUserTime = JobAcctInfo.TotalUserTime.QuadPart;
							jcdTotal.CurrentKernelTime +=
								pJCD->CurrentKernelTime = JobAcctInfo.TotalKernelTime.QuadPart;

#ifdef _DATAJOB_INCLUDE_TOTAL_COUNTERS
							// convert these times from 100 ns Time base to 1 mS time base
							jcdTotal.TotalProcessorTime +=
								pJCD->TotalProcessorTime =
									(JobAcctInfo.ThisPeriodTotalUserTime.QuadPart +
									JobAcctInfo.ThisPeriodTotalKernelTime.QuadPart) / 10000;
							jcdTotal.TotalUserTime +=
								pJCD->TotalUserTime =
									JobAcctInfo.ThisPeriodTotalUserTime.QuadPart / 10000;
							jcdTotal.TotalKernelTime +=
								pJCD->TotalKernelTime =
									JobAcctInfo.ThisPeriodTotalKernelTime.QuadPart / 1000;
							jcdTotal.CurrentProcessorUsage +=
								pJCD->CurrentProcessorUsage =
									(JobAcctInfo.TotalUserTime.QuadPart +
									 JobAcctInfo.TotalKernelTime.QuadPart) / 10000;

							jcdTotal.CurrentUserUsage +=
								pJCD->CurrentUserUsage =
									JobAcctInfo.TotalUserTime.QuadPart / 10000;

							jcdTotal.CurrentKernelUsage +=
								pJCD->CurrentKernelUsage =
									JobAcctInfo.TotalKernelTime.QuadPart / 10000;
#endif
							jcdTotal.PageFaults +=
								pJCD->PageFaults = JobAcctInfo.TotalPageFaultCount;
							jcdTotal.TotalProcessCount +=
								pJCD->TotalProcessCount = JobAcctInfo.TotalProcesses;
							jcdTotal.ActiveProcessCount +=
								pJCD->ActiveProcessCount = JobAcctInfo.ActiveProcesses;
							jcdTotal.TerminatedProcessCount +=
								pJCD->TerminatedProcessCount = JobAcctInfo.TotalTerminatedProcesses;

							NumJobInstances++;

							CloseHandle (JobHandle);

							// set perfdata pointer to next byte
							pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pJCD[1];
						} else {
							// unable to query job accounting info
							Status = GetLastError();
							wEvtStringCount = 0;
							szEvtStringArray[wEvtStringCount++] = wszNameBuffer;
							// unable to open this Job
							ReportEventW (hEventLog,
								EVENTLOG_WARNING_TYPE,
								0,
								PERFPROC_UNABLE_QUERY_JOB_ACCT,
								NULL,
								wEvtStringCount,
								sizeof(DWORD),
								szEvtStringArray,
								(LPVOID)&Status);
						}
					} else {
						Status = GetLastError();
                        if (bOpenJobErrorLogged == FALSE) {
						    wEvtStringCount = 0;
						    szEvtStringArray[wEvtStringCount++] = wszNameBuffer;
						    // unable to open this Job
						    ReportEventW (hEventLog,
							    EVENTLOG_WARNING_TYPE,
							    0,
							    PERFPROC_UNABLE_OPEN_JOB,
							    NULL,
							    wEvtStringCount,
							    sizeof(DWORD),
							    szEvtStringArray,
							    (LPVOID)&Status);
                            bOpenJobErrorLogged = TRUE;
                        }
                    }
				}

				//
				//  There is another record so advance DirInfo to the next entry
				//

				DirInfo = (POBJECT_DIRECTORY_INFORMATION) (((PUCHAR) DirInfo) +
							  sizeof( OBJECT_DIRECTORY_INFORMATION ) );

			}

			RtlZeroMemory( Buffer, dwBufferSize );

		}

		if ((Status == STATUS_NO_MORE_FILES) ||
			(Status == STATUS_NO_MORE_ENTRIES)) {
			// this is OK
			Status = STATUS_SUCCESS;
		}

		if (Buffer) FREEMEM(hLibHeap, 0, Buffer);

		//
		//  Now close the directory object
		//

		(VOID) NtClose( DirectoryHandle );
	}

    if (NT_SUCCESS(Status)) {
        if (NumJobInstances > 0) {
		    // see if the total instance will fit
		    TotalLen += sizeof(PERF_INSTANCE_DEFINITION) +
					    (MAX_NAME_LENGTH+1+sizeof(DWORD))*
					      sizeof(WCHAR) +
				       sizeof (JOB_COUNTER_DATA);

		    if ( *lpcbTotalBytes < TotalLen ) {
			    *lpcbTotalBytes = 0;
			    *lpNumObjectTypes = 0;
			    return ERROR_MORE_DATA;
		    }

		    // it looks like it will fit so create "total" instance

		    MonBuildInstanceDefinition(pPerfInstanceDefinition,
			    (PVOID *) &pJCD,
			    0,
			    0,
			    (DWORD)-1,
			    wszTotal);

		    // test structure for Quadword Alignment
		    assert (((DWORD)(pJCD) & 0x00000007) == 0);

		    //
		    //  transfer total info
		    //
		    memcpy (pJCD, &jcdTotal, sizeof (jcdTotal));
		    pJCD->CounterBlock.ByteLength = sizeof (JOB_COUNTER_DATA);

		    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pJCD[1];
		    NumJobInstances++;
        }

		pJobDataDefinition->JobObjectType.NumInstances =
			NumJobInstances;
		//
		//  Now we know how large an area we used for the
		//  data, so we can update the offset
		//  to the next object definition
		//

		*lpcbTotalBytes =
			pJobDataDefinition->JobObjectType.TotalByteLength =
			(DWORD)((PCHAR) pPerfInstanceDefinition -
			(PCHAR) pJobDataDefinition);

#if DBG
		if (*lpcbTotalBytes > TotalLen ) {
			DbgPrint ("\nPERFPROC: Job Perf Ctr. Instance Size Underestimated:");
			DbgPrint ("\nPERFPROC:   Estimated size: %d, Actual Size: %d", TotalLen, *lpcbTotalBytes);
		}
#endif

	    *lppData = (LPVOID) pPerfInstanceDefinition;

		*lpNumObjectTypes = 1;

	} else {
		*lpcbTotalBytes = 0;
		*lpNumObjectTypes = 0;
        if (bOpenJobErrorLogged == FALSE) {
		    wEvtStringCount = 0;
		    szEvtStringArray[wEvtStringCount++] = DirectoryName.Buffer;
		    // unable to query the object directory
		    ReportEventW (hEventLog,
			    EVENTLOG_WARNING_TYPE,
			    0,
			    PERFPROC_UNABLE_QUERY_OBJECT_DIR,
			    NULL,
			    wEvtStringCount,
			    sizeof(DWORD),
			    szEvtStringArray,
			    (LPVOID)&Status);
            bOpenJobErrorLogged = TRUE;
        }
	}

    return ERROR_SUCCESS;
}

DWORD APIENTRY
CollectJobDetailData (
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the processor object

Arguments:

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the address of the DWORD to receive the number of objects added
            by this routine
         OUT: the number of objects added by this routine is writted to the
            DWORD pointed to by this argument

   Returns:

             0 if successful, else Win 32 error code of failure

--*/
{
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PUNICODE_STRING pProcessName;

    DWORD   TotalLen;            //  Length of the total return block

    PPERF_INSTANCE_DEFINITION   pPerfInstanceDefinition;
    PJOB_DETAILS_DATA_DEFINITION		pJobDetailsDataDefinition;
    PJOB_DETAILS_COUNTER_DATA			pJDCD;
    JOB_DETAILS_COUNTER_DATA			jdcdTotal;
    JOB_DETAILS_COUNTER_DATA			jdcdGrandTotal;


    NTSTATUS Status;
    HANDLE DirectoryHandle, JobHandle;
    ULONG ReturnedLength;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    POBJECT_NAME_INFORMATION NameInfo;
    OBJECT_ATTRIBUTES Attributes;
	WCHAR	wszNameBuffer[MAX_STR_CHAR];
	DWORD	i, dwSize;
	PUCHAR  Buffer;
	BOOL	bStatus;
	PJOBOBJECT_BASIC_PROCESS_ID_LIST pJobPidList;

	DWORD	dwWin32Status = ERROR_SUCCESS;
    ACCESS_MASK ExtraAccess = 0;
    ULONG Context = 0;
	DWORD	NumJobObjects = 0;
	DWORD	NumJobDetailInstances = 0;

	// get size of a data block that has 1 instance
    TotalLen = sizeof(JOB_DETAILS_DATA_DEFINITION) +		// object def + counter defs
               sizeof (PERF_INSTANCE_DEFINITION) +	// 1 instance def
               MAX_VALUE_NAME_LENGTH +				// 1 instance name
               sizeof(JOB_DETAILS_COUNTER_DATA);			// 1 instance data block

    if ( *lpcbTotalBytes < TotalLen ) {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        return ERROR_MORE_DATA;
    }

	// cast callers buffer to the object data definition type
    pJobDetailsDataDefinition = (JOB_DETAILS_DATA_DEFINITION *) *lppData;

    //
    //  Define Job Details Object data block
    //

    memcpy(pJobDetailsDataDefinition,
           &JobDetailsDataDefinition,
           sizeof(JOB_DETAILS_DATA_DEFINITION));

	// set timestamp of this object
    pJobDetailsDataDefinition->JobDetailsObjectType.PerfTime = SysTimeInfo.CurrentTime;

    // Now collect data for each job object found in system
    //
    //  Perform initial setup
    //
	Buffer = ALLOCMEM(hLibHeap, HEAP_ZERO_MEMORY, dwBufferSize);
	pJobPidList = ALLOCMEM(hLibHeap, HEAP_ZERO_MEMORY, dwBufferSize);
	if ((Buffer == NULL) || (pJobPidList == NULL)) {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
		// free the one that got allocated (if any)
		if (Buffer != NULL) FREEMEM(hLibHeap, 0, Buffer);
		if (pJobPidList != NULL) FREEMEM(hLibHeap, 0, pJobPidList);
		// BUGBUG: NEED EVENT LOG MESSAGE HERE to report mem alloc failure
        return ERROR_SUCCESS;
	}

    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                  &pJobDetailsDataDefinition[1];

    // adjust TotalLen to be the size of the buffer already in use
    TotalLen = sizeof (JOB_DETAILS_DATA_DEFINITION);

    // zero the total instance buffer
    memset (&jdcdGrandTotal, 0, sizeof (jdcdGrandTotal));

    //
    //  Open the directory for list directory access
    //
	// this should always succeed since it's a system name we
	// will be querying
	//
    InitializeObjectAttributes( &Attributes,
                                &DirectoryName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );
	
    Status = NtOpenDirectoryObject( &DirectoryHandle,
                                    DIRECTORY_QUERY | ExtraAccess,
                                    &Attributes
                                  );
    if (NT_SUCCESS( Status )) {
		//
		// Get the actual name of the object directory object.
		//

	    NameInfo = (POBJECT_NAME_INFORMATION) &Buffer[0];
		Status = NtQueryObject( DirectoryHandle,
                                 ObjectNameInformation,
                                 NameInfo,
                                 dwBufferSize,
                                 (PULONG) NULL );
    }

    if (NT_SUCCESS( Status )) {
		//
		//  Query the entire directory in one sweep
		//
		for (Status = NtQueryDirectoryObject( DirectoryHandle,
											  Buffer,
											  dwBufferSize,
											  FALSE,
											  FALSE,
											  &Context,
											  &ReturnedLength );
			 NT_SUCCESS( Status );
			 Status = NtQueryDirectoryObject( DirectoryHandle,
											  Buffer,
											  dwBufferSize,
											  FALSE,
											  FALSE,
											  &Context,
											  &ReturnedLength ) ) {

			//
			//  Check the status of the operation.
			//

			if (!NT_SUCCESS( Status )) {
				break;
			}

			//
			//  For every record in the buffer type out the directory information
			//

			//
			//  Point to the first record in the buffer, we are guaranteed to have
			//  one otherwise Status would have been No More Files
			//

			DirInfo = (POBJECT_DIRECTORY_INFORMATION) &Buffer[0];

			while (TRUE) {

				//
				//  Check if there is another record.  If there isn't, then get out
				//  of the loop now
				//

				if (DirInfo->Name.Length == 0) {
					break;
				}

				//
				//  Print out information about the Job
				//

				if (wcsncmp ( DirInfo->TypeName.Buffer, &szJob[0], ((sizeof(szJob)/sizeof(WCHAR)) - 1)) == 0) {
					// this is really a job, so list the name
					dwSize = DirInfo->Name.Length;
					if (dwSize > MAX_STR_SIZE) dwSize = MAX_STR_SIZE;
					memcpy (wszNameBuffer, DirInfo->Name.Buffer, dwSize);
					wszNameBuffer[dwSize/sizeof(WCHAR)] = 0;

					// clear the Job total counter block

				    memset (&jdcdTotal, 0, sizeof (jdcdTotal));

					// now query the process ID's for this job

					JobHandle = OpenJobObjectW (
						JOB_OBJECT_QUERY,
						FALSE,
						wszNameBuffer);

//					ASSERT (JobHandle != NULL);

					if (JobHandle != NULL) {

						bStatus = QueryInformationJobObject (
							JobHandle,
							JobObjectBasicProcessIdList,
							pJobPidList,
							dwBufferSize,
							&ReturnedLength);

//						ASSERT (bStatus == TRUE);
						ASSERT (ReturnedLength <= BUFFERSIZE);
						ASSERT (pJobPidList->NumberOfAssignedProcesses ==
							pJobPidList->NumberOfProcessIdsInList);

						// test to see if there was enough room in the first buffer
						// for everything, if not, expand the buffer and retry

						if ((bStatus) && (pJobPidList->NumberOfAssignedProcesses >
							pJobPidList->NumberOfProcessIdsInList))	{
							dwBufferSize +=
								(pJobPidList->NumberOfAssignedProcesses -
								 pJobPidList->NumberOfProcessIdsInList) *
								 sizeof (DWORD);
							pJobPidList = REALLOCMEM (hLibHeap, 0,
								pJobPidList, dwBufferSize);
//							ASSERT (pJobPidList != NULL);
							if (pJobPidList != NULL) {
								bStatus = QueryInformationJobObject (
									JobHandle,
									JobObjectBasicProcessIdList,
									pJobPidList,
									dwBufferSize,
									&ReturnedLength);
							} else {		
								bStatus = FALSE;
								SetLastError ( ERROR_OUTOFMEMORY );
							}
						}

						if (bStatus) {

							for (i=0;i < pJobPidList->NumberOfProcessIdsInList; i++) {
								// *** create and initialize perf data instance here ***
								// get process data object from ID
								ProcessInfo = GetProcessPointerFromProcessId (pJobPidList->ProcessIdList[i]);
								
//								ASSERT (ProcessInfo != NULL);

								if (ProcessInfo != NULL) {

									// get process name
									if (lProcessNameCollectionMethod == PNCM_MODULE_FILE) {
										pProcessName = GetProcessSlowName (ProcessInfo);
									} else {
									   pProcessName = GetProcessShortName (ProcessInfo);
									}
									ReturnedLength = pProcessName->Length + sizeof(WCHAR);

									// see if this instance will fit
									TotalLen += sizeof(PERF_INSTANCE_DEFINITION) +
											   DWORD_MULTIPLE (ReturnedLength) +
											   sizeof (JOB_DETAILS_COUNTER_DATA);

									if ( *lpcbTotalBytes < TotalLen ) {
										*lpcbTotalBytes = 0;
										*lpNumObjectTypes = 0;
										Status = STATUS_NO_MEMORY;
										dwWin32Status =  ERROR_MORE_DATA;
										break;
									}

									MonBuildInstanceDefinition(pPerfInstanceDefinition,
										(PVOID *) &pJDCD,
										JOB_OBJECT_TITLE_INDEX,
										NumJobObjects,
										(DWORD)-1,
										pProcessName->Buffer);

									// test structure for Quadword Alignment
									assert (((DWORD)(pJDCD) & 0x00000007) == 0);

									//
									//  Format and collect Process data
									//

									pJDCD->CounterBlock.ByteLength = sizeof (JOB_DETAILS_COUNTER_DATA);
									//
									//  Convert User time from 100 nsec units to counter frequency.
									//
									jdcdTotal.ProcessorTime +=
										pJDCD->ProcessorTime = ProcessInfo->KernelTime.QuadPart +
															ProcessInfo->UserTime.QuadPart;
									jdcdTotal.UserTime +=
										pJDCD->UserTime = ProcessInfo->UserTime.QuadPart;
									jdcdTotal.KernelTime +=
										pJDCD->KernelTime = ProcessInfo->KernelTime.QuadPart;

									jdcdTotal.PeakVirtualSize +=
										pJDCD->PeakVirtualSize = ProcessInfo->PeakVirtualSize;
									jdcdTotal.VirtualSize +=
										pJDCD->VirtualSize = ProcessInfo->VirtualSize;

									jdcdTotal.PageFaults +=
										pJDCD->PageFaults = ProcessInfo->PageFaultCount;
									jdcdTotal.PeakWorkingSet +=
										pJDCD->PeakWorkingSet = ProcessInfo->PeakWorkingSetSize;
									jdcdTotal.TotalWorkingSet +=
										pJDCD->TotalWorkingSet = ProcessInfo->WorkingSetSize;

#ifdef _DATAPROC_PRIVATE_WS_
									jdcdTotal.PrivateWorkingSet +=
										pJDCD->PrivateWorkingSet = ProcessInfo->PrivateWorkingSetSize;
									jdcdTotal.SharedWorkingSet +=
										pJDCD->SharedWorkingSet =
											ProcessInfo->WorkingSetSize -
											ProcessInfo->PrivateWorkingSetSize;
#endif
									jdcdTotal.PeakPageFile +=
										pJDCD->PeakPageFile = ProcessInfo->PeakPagefileUsage;
									jdcdTotal.PageFile +=
										pJDCD->PageFile = ProcessInfo->PagefileUsage;

									jdcdTotal.PrivatePages +=
										pJDCD->PrivatePages = ProcessInfo->PrivatePageCount;

									jdcdTotal.ThreadCount +=
										pJDCD->ThreadCount = ProcessInfo->NumberOfThreads;

									// base priority is not totaled
									pJDCD->BasePriority = ProcessInfo->BasePriority;

									// elpased time is not totaled
									pJDCD->ElapsedTime = ProcessInfo->CreateTime.QuadPart;

									pJDCD->ProcessId = HandleToUlong(ProcessInfo->UniqueProcessId);
									pJDCD->CreatorProcessId = HandleToUlong(ProcessInfo->InheritedFromUniqueProcessId);

									jdcdTotal.PagedPool +=
										pJDCD->PagedPool = (DWORD)ProcessInfo->QuotaPagedPoolUsage;
									jdcdTotal.NonPagedPool +=
										pJDCD->NonPagedPool = (DWORD)ProcessInfo->QuotaNonPagedPoolUsage;
									jdcdTotal.HandleCount +=
										pJDCD->HandleCount = (DWORD)ProcessInfo->HandleCount;

                                    // update I/O counters
                                    jdcdTotal.ReadOperationCount +=
                                        pJDCD->ReadOperationCount = ProcessInfo->ReadOperationCount.QuadPart;
                                    jdcdTotal.DataOperationCount += 
                                        pJDCD->DataOperationCount = ProcessInfo->ReadOperationCount.QuadPart;
                                    jdcdTotal.WriteOperationCount +=
                                        pJDCD->WriteOperationCount = ProcessInfo->WriteOperationCount.QuadPart;
                                    jdcdTotal.DataOperationCount += ProcessInfo->WriteOperationCount.QuadPart;
                                        pJDCD->DataOperationCount += ProcessInfo->WriteOperationCount.QuadPart;
                                    jdcdTotal.OtherOperationCount +=
                                        pJDCD->OtherOperationCount = ProcessInfo->OtherOperationCount.QuadPart;

                                    jdcdTotal.ReadTransferCount +=
                                        pJDCD->ReadTransferCount = ProcessInfo->ReadTransferCount.QuadPart;
                                    jdcdTotal.DataTransferCount +=
                                        pJDCD->DataTransferCount = ProcessInfo->ReadTransferCount.QuadPart;
                                    jdcdTotal.WriteTransferCount +=
                                        pJDCD->WriteTransferCount = ProcessInfo->WriteTransferCount.QuadPart;
                                    jdcdTotal.DataTransferCount += ProcessInfo->WriteTransferCount.QuadPart;
                                        pJDCD->DataTransferCount += ProcessInfo->WriteTransferCount.QuadPart;
                                    jdcdTotal.OtherTransferCount +=
                                        pJDCD->OtherTransferCount = ProcessInfo->OtherTransferCount.QuadPart;
                        
									// set perfdata pointer to next byte
									pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pJDCD[1];

									NumJobDetailInstances++;
								} else {
									// unable to locate info on this process
									// for now, we'll ignore this...
								}
							}
						
							CloseHandle (JobHandle);

							// see if this instance will fit
							TotalLen += sizeof(PERF_INSTANCE_DEFINITION) +
									   DWORD_MULTIPLE (MAX_STR_SIZE) +
									   sizeof (JOB_DETAILS_COUNTER_DATA);

							if ( *lpcbTotalBytes < TotalLen ) {
								*lpcbTotalBytes = 0;
								*lpNumObjectTypes = 0;
								Status = STATUS_NO_MEMORY;
								dwWin32Status =  ERROR_MORE_DATA;
								break;
							}

							MonBuildInstanceDefinition(pPerfInstanceDefinition,
								(PVOID *) &pJDCD,
								JOB_OBJECT_TITLE_INDEX,
								NumJobObjects,
								(DWORD)-1,
								wszTotal);

							// test structure for Quadword Alignment
							assert (((DWORD)(pJDCD) & 0x00000007) == 0);

							// copy total data to caller's buffer

							memcpy (pJDCD, &jdcdTotal, sizeof (jdcdTotal));
							pJDCD->CounterBlock.ByteLength = sizeof (JOB_DETAILS_COUNTER_DATA);

							// update grand total instance
							//
							jdcdGrandTotal.ProcessorTime += jdcdTotal.ProcessorTime;
							jdcdGrandTotal.UserTime += jdcdTotal.UserTime;
							jdcdGrandTotal.KernelTime += jdcdTotal. KernelTime;
							jdcdGrandTotal.PeakVirtualSize += jdcdTotal.PeakVirtualSize;
							jdcdGrandTotal.VirtualSize += jdcdTotal.VirtualSize;

							jdcdGrandTotal.PageFaults += jdcdTotal.PageFaults;
							jdcdGrandTotal.PeakWorkingSet += jdcdTotal.PeakWorkingSet;
							jdcdGrandTotal.TotalWorkingSet += jdcdTotal.TotalWorkingSet;

#ifdef _DATAPROC_PRIVATE_WS_
							jdcdGrandTotal.PrivateWorkingSet += jdcdTotal.PrivateWorkingSet;
							jdcdGrandTotal.SharedWorkingSet += jdcdTotal.SharedWorkingSet;
#endif

							jdcdGrandTotal.PeakPageFile += jdcdTotal.PeakPageFile;
							jdcdGrandTotal.PageFile += jdcdTotal.PageFile;
							jdcdGrandTotal.PrivatePages += jdcdTotal.PrivatePages;
							jdcdGrandTotal.ThreadCount += jdcdTotal.ThreadCount;

							jdcdGrandTotal.PagedPool += jdcdTotal.PagedPool;
							jdcdGrandTotal.NonPagedPool += jdcdTotal.NonPagedPool;
							jdcdGrandTotal.HandleCount += jdcdTotal.HandleCount;

							// set perfdata pointer to next byte
							pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pJDCD[1];

							NumJobDetailInstances++;
							NumJobObjects++;
						} else {
							// unable to read PID list from Job
							Status = GetLastError();
							wEvtStringCount = 0;
							szEvtStringArray[wEvtStringCount++] = wszNameBuffer;
							// unable to open this Job
							ReportEventW (hEventLog,
								EVENTLOG_WARNING_TYPE,
								0,
								PERFPROC_UNABLE_QUERY_JOB_PIDS,
								NULL,
								wEvtStringCount,
								sizeof(DWORD),
								szEvtStringArray,
								(LPVOID)&Status);
						}
					} else {
						Status = GetLastError();
                        if (bOpenJobErrorLogged == FALSE) {
						    wEvtStringCount = 0;
						    szEvtStringArray[wEvtStringCount++] = wszNameBuffer;
						    // unable to open this Job
						    ReportEventW (hEventLog,
							    EVENTLOG_WARNING_TYPE,
							    0,
							    PERFPROC_UNABLE_OPEN_JOB,
							    NULL,
							    wEvtStringCount,
							    sizeof(DWORD),
							    szEvtStringArray,
							    (LPVOID)&Status);
                            bOpenJobErrorLogged = TRUE;
                        }
					}
				}

				//
				//  There is another record so advance DirInfo to the next entry
				//

				DirInfo = (POBJECT_DIRECTORY_INFORMATION) (((PUCHAR) DirInfo) +
							  sizeof( OBJECT_DIRECTORY_INFORMATION ) );

			}

			RtlZeroMemory( Buffer, dwBufferSize );

		}

		if ((Status == STATUS_NO_MORE_FILES) ||
			(Status == STATUS_NO_MORE_ENTRIES)) {
			// this is OK
			Status = STATUS_SUCCESS;
		}

		if (Buffer) FREEMEM(hLibHeap, 0, Buffer);
		if (pJobPidList) FREEMEM(hLibHeap, 0, pJobPidList);

		//
		//  Now close the directory object
		//

		(VOID) NtClose( DirectoryHandle );

        if (NumJobDetailInstances > 0) {
		    // see if this instance will fit
		    TotalLen += sizeof(PERF_INSTANCE_DEFINITION) +
				       DWORD_MULTIPLE (MAX_STR_SIZE) +
				       sizeof (JOB_DETAILS_COUNTER_DATA);

		    if ( *lpcbTotalBytes < TotalLen ) {
			    *lpcbTotalBytes = 0;
			    *lpNumObjectTypes = 0;
			    Status = STATUS_NO_MEMORY;
			    dwWin32Status =  ERROR_MORE_DATA;
		    } else {

                // set the Total Elapsed Time to be the current time so that it will
                // show up as 0 when displayed.
                jdcdGrandTotal.ElapsedTime = pJobDetailsDataDefinition->JobDetailsObjectType.PerfTime.QuadPart;

                // build the grand total instance
			    MonBuildInstanceDefinition(pPerfInstanceDefinition,
				    (PVOID *) &pJDCD,
				    JOB_OBJECT_TITLE_INDEX,
				    NumJobObjects,
				    (DWORD)-1,
				    wszTotal);

			    // test structure for Quadword Alignment
			    ASSERT (((ULONG_PTR)(pJDCD) & 0x00000007) == 0);

			    // copy total data to caller's buffer

			    memcpy (pJDCD, &jdcdGrandTotal, sizeof (jdcdGrandTotal));
			    pJDCD->CounterBlock.ByteLength = sizeof (JOB_DETAILS_COUNTER_DATA);

			    // update pointers
			    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pJDCD[1];
			    NumJobDetailInstances++;
            }
        }

		pJobDetailsDataDefinition->JobDetailsObjectType.NumInstances =
			NumJobDetailInstances;

		//
		//  Now we know how large an area we used for the
		//  Process definition, so we can update the offset
		//  to the next object definition
		//

		*lpcbTotalBytes =
 			pJobDetailsDataDefinition->JobDetailsObjectType.TotalByteLength =
			(DWORD)((PCHAR) pPerfInstanceDefinition -
			(PCHAR) pJobDetailsDataDefinition);

#if DBG
		if (*lpcbTotalBytes > TotalLen ) {
			DbgPrint ("\nPERFPROC: Job Perf Ctr. Instance Size Underestimated:");
			DbgPrint ("\nPERFPROC:   Estimated size: %d, Actual Size: %d", TotalLen, *lpcbTotalBytes);
		}
#endif

		*lppData = (LPVOID) pPerfInstanceDefinition;

		*lpNumObjectTypes = 1;
    
	} else {
		*lpcbTotalBytes = 0;
		*lpNumObjectTypes = 0;
        if (bOpenJobErrorLogged == FALSE) {
		    wEvtStringCount = 0;
		    szEvtStringArray[wEvtStringCount++] = DirectoryName.Buffer;
		    // unable to query the object directory
		    ReportEventW (hEventLog,
			    EVENTLOG_WARNING_TYPE,
			    0,
			    PERFPROC_UNABLE_QUERY_OBJECT_DIR,
			    NULL,
			    wEvtStringCount,
			    sizeof(DWORD),
			    szEvtStringArray,
			    (LPVOID)&Status);
            bOpenJobErrorLogged = TRUE;
        }
    }

    return ERROR_SUCCESS;
}
