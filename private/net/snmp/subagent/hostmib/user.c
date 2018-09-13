 /*
 *  user.c v0.10
 *
 ****************************************************************************
 *                                                                          *
 *      (C) Copyright 1995 DIGITAL EQUIPMENT CORPORATION                    *
 *                                                                          *
 *      This  software  is  an  unpublished work protected under the        *
 *      the copyright laws of the  United  States  of  America,  all        *
 *      rights reserved.                                                    *
 *                                                                          *
 *      In the event this software is licensed for use by the United        *
 *      States Government, all use, duplication or disclosure by the        *
 *      United States Government is subject to restrictions  as  set        *
 *      forth in either subparagraph  (c)(1)(ii)  of the  Rights  in        *
 *      Technical  Data  And  Computer  Software  Clause  at   DFARS        *
 *      252.227-7013, or the Commercial Computer Software Restricted        *
 *      Rights Clause at FAR 52.221-19, whichever is applicable.            *
 *                                                                          *
 ****************************************************************************
 *
 *  Facility:
 *
 *    SNMP Extension Agent
 *
 *  Abstract:
 *  
 *    This module contains support functions for the HostMIB Subagent.
 *
 *
 *  Author:
 *
 *    D. D. Burns @ WebEnable, Inc.
 *
 *
 *  Revision History:
 *
 *    V0.01 - 04/16/97  D. D. Burns     Original Creation
 *
 *
 */




/*

|
| Support Functions accessible from outside this module:
|

Spt_GetProcessCount
    This function supports hrSystem table attribute "hrSystemProcesses"
    by returning the number of active processes in the system.  This
    code is derived from PERFDLL code in files "PERFSPRC.C" and
    "PERFPROC.C".

|
| Support Functions accessible only from inside this module:
|

*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>


/*
|| LOCAL DEFINES
*/

/*
| Spt_GetProcessCount
*/
#define INCREMENT_BUFFER_SIZE ((DWORD)(4096*2))
#define LARGE_BUFFER_SIZE   ((DWORD)(4096*16))



/* Spt_GetProcessCount - Retrieve count of number of active processes */
/* Spt_GetProcessCount - Retrieve count of number of active processes */
/* Spt_GetProcessCount - Retrieve count of number of active processes */

ULONG
Spt_GetProcessCount(
                    void
                    )
/*
|  IN SUPPORT OF:
|
|       HRSYSTEM.C - "hrSystemProcesses"
|
|  EXPLICIT INPUTS:
|
|       None.
|
|  IMPLICIT INPUTS:
|
|       System performance information is fetched thru
|       "NtQuerySystemInformation()".
|
|  OUTPUTS:
|
|     On Success:
|       Function returns the count of active processes as determined by
|       the number of performance information blocks for processes that
|       have both a name and a non-zero thread count (in the style of code in
|       "PERFPROC.C").
|
|     On any Failure:
|       Function returns zero (not a legal number of processes).
|
|  THE BIG PICTURE:
|
|       The generated function "GetHrSystemProcesses()" in HRSYSTEM.C is
|       invoked by the generic subagent to retrieve the current value of 
|       the SNMP attribute "GetHrSystemProcesses".  All the work of 
|       retrieving that value is done by this support function.
|
|  OTHER THINGS TO KNOW:
|
|       This function incurs a rather substantial bit of overhead in that
|       to determine the number of processes active it actually fetches
|       a large slug of performance data (a "slug" per process) for all
|       processes and merely counts the number of slugs returned.
|       This seems to be the only available way to acquire this information.
|
*/
{
DWORD   dwReturnedBufferSize;
NTSTATUS Status;
DWORD   ProcessBufSize = LARGE_BUFFER_SIZE;     // Initial Process-Buf size
LPBYTE  pProcessBuffer = NULL;                  // Pointer to Process-Buf
PSYSTEM_PROCESS_INFORMATION ProcessInfo;        // Walking ptr thru Process-Buf
ULONG   ProcessBufferOffset = 0;                // 
ULONG   Process_count = 0;                      // Count of Live processes


//
//  Get process data from system.
//

// Grab an initially-sized buffer to receive data
pProcessBuffer = malloc(ProcessBufSize);
if (pProcessBuffer == NULL) {
    return (0);         // Out of memory
    }

/*
| Loop until we've allocated a buffer big enough to receive all the data
| NtQuery wants to give us.
|
| Exit with the buffer loaded with info or on some kind of non-mismatch error.
*/

while( (Status = NtQuerySystemInformation(
                        SystemProcessInformation,
                        pProcessBuffer,
                        ProcessBufSize,
                        &dwReturnedBufferSize))
      == STATUS_INFO_LENGTH_MISMATCH ) {

    LPBYTE  pNewProcessBuffer;               // For use on realloc

    // expand buffer & retry
    ProcessBufSize += INCREMENT_BUFFER_SIZE;

    if ( !(pNewProcessBuffer = realloc(pProcessBuffer,ProcessBufSize)) ) {

        /* If realloc failed and left us with the old buffer, free it */
        if (pProcessBuffer != NULL) {
            free(pProcessBuffer);
            }

        return (0);     // Out of memory
        }
    else {
        /* Successful Realloc */
        pProcessBuffer = pNewProcessBuffer;
        }

    /* Try another query */        
    }

/* If we didn't meet with full success. . . */
if ( !NT_SUCCESS(Status) ) {
    if (pProcessBuffer != NULL) {
        free(pProcessBuffer);
        }

    return (0);     // Unknown error that prevents us from continuing
    }

/*
| At this point, "pProcessBuffer" points to a buffer formatted as a
| "System Process Information" structure.
|
| Setup to go a-walking it.
*/
ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) pProcessBuffer;

while ( TRUE ) {

    // check for Live processes
    //  (i.e. name or threads)

    if ((ProcessInfo->ImageName.Buffer != NULL) ||
        (ProcessInfo->NumberOfThreads > 0)) {

        /* thread is not Dead */
        Process_count += 1;
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


free(pProcessBuffer);

return (Process_count);
}



