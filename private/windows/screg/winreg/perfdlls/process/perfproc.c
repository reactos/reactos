/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perfproc.c

Abstract:

    This file implements an Performance Object that presents
    Image details performance object data

Created:

    Bob Watson  22-Oct-1996

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
#include "perfmsg.h"
#include "dataproc.h"

static  BOOL           bOldestProcessTime = FALSE;
static  LARGE_INTEGER  OldestProcessTime = {0,0};


DWORD APIENTRY
CollectProcessObjectData (
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

    PSYSTEM_PROCESS_INFORMATION ProcessInfo;

    PPERF_INSTANCE_DEFINITION   pPerfInstanceDefinition;
    PPROCESS_DATA_DEFINITION    pProcessDataDefinition;
    PPROCESS_COUNTER_DATA       pPCD;
    PROCESS_COUNTER_DATA        pcdTotal;

    ULONG   NumProcessInstances;
    BOOLEAN NullProcess;

    NTSTATUS    Status;
    DWORD       dwReturnedBufferSize;

    PUNICODE_STRING pProcessName;
    ULONG ProcessBufferOffset;

    POOLED_USAGE_AND_LIMITS PoolUsageInfo;

    LARGE_INTEGER    CreateTimeDiff;

    pProcessDataDefinition = (PROCESS_DATA_DEFINITION *) *lppData;

    //
    //  Check for sufficient space for Process object type definition
    //

    TotalLen = sizeof(PROCESS_DATA_DEFINITION) +
               sizeof (PERF_INSTANCE_DEFINITION) +
               MAX_VALUE_NAME_LENGTH +
               sizeof(PROCESS_COUNTER_DATA);

    if ( *lpcbTotalBytes < TotalLen ) {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        return ERROR_MORE_DATA;
    }

    //
    //  Define Process data block
    //

    memcpy(pProcessDataDefinition,
           &ProcessDataDefinition,
           sizeof(PROCESS_DATA_DEFINITION));

    pProcessDataDefinition->ProcessObjectType.PerfTime = SysTimeInfo.CurrentTime;

    ProcessBufferOffset = 0;

    // Now collect data for each process

    NumProcessInstances = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) pProcessBuffer;

    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                  &pProcessDataDefinition[1];

    // adjust TotalLen to be the size of the buffer already in use
    TotalLen = sizeof (PROCESS_DATA_DEFINITION);

    // zero the total instance buffer
    memset (&pcdTotal, 0, sizeof (pcdTotal));

    while ( TRUE ) {

        // see if this instance will fit
        TotalLen += sizeof(PERF_INSTANCE_DEFINITION) +
                   ((MAX_PROCESS_NAME_LENGTH+1+sizeof(DWORD)) * sizeof(WCHAR)) +
                   sizeof (PROCESS_COUNTER_DATA);

        if ( *lpcbTotalBytes < TotalLen ) {
            *lpcbTotalBytes = 0;
            *lpNumObjectTypes = 0;
            return ERROR_MORE_DATA;
        }

        // check for Live processes
        //  (i.e. name or threads)

        if ((ProcessInfo->ImageName.Buffer != NULL) ||
            (ProcessInfo->NumberOfThreads > 0)){
                // thread is not Dead
            // get process name
            if (lProcessNameCollectionMethod == PNCM_MODULE_FILE) {
                pProcessName = GetProcessSlowName (ProcessInfo);
            } else {
               pProcessName = GetProcessShortName (ProcessInfo);
            }
            NullProcess = FALSE;
        } else {
            // thread is dead
            NullProcess = TRUE;
        }

        if ( !NullProcess ) {

            // get the old process creation time the first time we are in
            // this routine
            if (!bOldestProcessTime) {
                if (OldestProcessTime.QuadPart <= 0) {
                    OldestProcessTime = ProcessInfo->CreateTime;
                } else if (ProcessInfo->CreateTime.QuadPart > 0) {
                    // both time values are not zero, see which one is smaller
                    if (OldestProcessTime.QuadPart >
                        ProcessInfo->CreateTime.QuadPart) {
                        OldestProcessTime = ProcessInfo->CreateTime;
                    }
                }
            }

            // get Pool usage for this process

            NumProcessInstances++;

            MonBuildInstanceDefinition(pPerfInstanceDefinition,
                (PVOID *) &pPCD,
                0,
                0,
                (DWORD)-1,
                pProcessName->Buffer);

            // test structure for Quadword Alignment
            assert (((DWORD)(pPCD) & 0x00000007) == 0);

            //
            //  Format and collect Process data
            //

            pPCD->CounterBlock.ByteLength = sizeof (PROCESS_COUNTER_DATA);
            //
            //  Convert User time from 100 nsec units to counter frequency.
            //
            pcdTotal.ProcessorTime +=
                pPCD->ProcessorTime = ProcessInfo->KernelTime.QuadPart +
                                    ProcessInfo->UserTime.QuadPart;
            pcdTotal.UserTime +=
                pPCD->UserTime = ProcessInfo->UserTime.QuadPart;
            pcdTotal.KernelTime +=
                pPCD->KernelTime = ProcessInfo->KernelTime.QuadPart;

            pcdTotal.PeakVirtualSize +=
                pPCD->PeakVirtualSize = ProcessInfo->PeakVirtualSize;
            pcdTotal.VirtualSize +=
                pPCD->VirtualSize = ProcessInfo->VirtualSize;

            pcdTotal.PageFaults +=
                pPCD->PageFaults = ProcessInfo->PageFaultCount;
            pcdTotal.PeakWorkingSet +=
                pPCD->PeakWorkingSet = ProcessInfo->PeakWorkingSetSize;
            pcdTotal.TotalWorkingSet +=
                pPCD->TotalWorkingSet = ProcessInfo->WorkingSetSize;

#ifdef _DATAPROC_PRIVATE_WS_
            pcdTotal.PrivateWorkingSet +=
                pPCD->PrivateWorkingSet = ProcessInfo->PrivateWorkingSetSize;
            pcdTotal.SharedWorkingSet +=
                pPCD->SharedWorkingSet =
					ProcessInfo->WorkingSetSize -
					ProcessInfo->PrivateWorkingSetSize;
#endif //_DATAPROC_PRIVATE_WS_

            pcdTotal.PeakPageFile +=
                pPCD->PeakPageFile = ProcessInfo->PeakPagefileUsage;
            pcdTotal.PageFile +=
                pPCD->PageFile = ProcessInfo->PagefileUsage;

            pcdTotal.PrivatePages +=
                pPCD->PrivatePages = ProcessInfo->PrivatePageCount;

            pcdTotal.ThreadCount +=
                pPCD->ThreadCount = ProcessInfo->NumberOfThreads;

            // base priority is not totaled
            pPCD->BasePriority = ProcessInfo->BasePriority;

            // elpased time is not totaled
            if (bOldestProcessTime &&
                (ProcessInfo->CreateTime.QuadPart <= 0)) {
                pPCD->ElapsedTime = OldestProcessTime.QuadPart;
            } else {
                pPCD->ElapsedTime = ProcessInfo->CreateTime.QuadPart;
            }

            pPCD->ProcessId = HandleToUlong(ProcessInfo->UniqueProcessId);
            pPCD->CreatorProcessId = HandleToUlong(ProcessInfo->InheritedFromUniqueProcessId);

            pcdTotal.PagedPool +=
                pPCD->PagedPool = (DWORD)ProcessInfo->QuotaPagedPoolUsage;
            pcdTotal.NonPagedPool +=
                pPCD->NonPagedPool = (DWORD)ProcessInfo->QuotaNonPagedPoolUsage;
            pcdTotal.HandleCount +=
                pPCD->HandleCount = (DWORD)ProcessInfo->HandleCount;

            
            // update I/O counters
            pcdTotal.ReadOperationCount +=
                pPCD->ReadOperationCount = ProcessInfo->ReadOperationCount.QuadPart;
            pcdTotal.DataOperationCount += 
                pPCD->DataOperationCount = ProcessInfo->ReadOperationCount.QuadPart;
            pcdTotal.WriteOperationCount +=
                pPCD->WriteOperationCount = ProcessInfo->WriteOperationCount.QuadPart;
            pcdTotal.DataOperationCount += ProcessInfo->WriteOperationCount.QuadPart;
                pPCD->DataOperationCount += ProcessInfo->WriteOperationCount.QuadPart;
            pcdTotal.OtherOperationCount +=
                pPCD->OtherOperationCount = ProcessInfo->OtherOperationCount.QuadPart;

            pcdTotal.ReadTransferCount +=
                pPCD->ReadTransferCount = ProcessInfo->ReadTransferCount.QuadPart;
            pcdTotal.DataTransferCount +=
                pPCD->DataTransferCount = ProcessInfo->ReadTransferCount.QuadPart;
            pcdTotal.WriteTransferCount +=
                pPCD->WriteTransferCount = ProcessInfo->WriteTransferCount.QuadPart;
            pcdTotal.DataTransferCount += ProcessInfo->WriteTransferCount.QuadPart;
                pPCD->DataTransferCount += ProcessInfo->WriteTransferCount.QuadPart;
            pcdTotal.OtherTransferCount +=
                pPCD->OtherTransferCount = ProcessInfo->OtherTransferCount.QuadPart;
                        
            // set perfdata pointer to next byte
            pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pPCD[1];
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

    if (NumProcessInstances > 0) {

        // see if the total instance will fit
        TotalLen += sizeof(PERF_INSTANCE_DEFINITION) +
                    (MAX_PROCESS_NAME_LENGTH+1+sizeof(DWORD))*
                        sizeof(WCHAR) +
                   sizeof (PROCESS_COUNTER_DATA);

        if ( *lpcbTotalBytes < TotalLen ) {
            *lpcbTotalBytes = 0;
            *lpNumObjectTypes = 0;
            return ERROR_MORE_DATA;
        }

        // it looks like it will fit so create "total" instance

        NumProcessInstances++;

        // set the Total Elapsed Time to be the current time so that it will
        // show up as 0 when displayed.
        pcdTotal.ElapsedTime = pProcessDataDefinition->ProcessObjectType.PerfTime.QuadPart;

        MonBuildInstanceDefinition(pPerfInstanceDefinition,
            (PVOID *) &pPCD,
            0,
            0,
            (DWORD)-1,
            wszTotal);

        // test structure for Quadword Alignment
        assert (((DWORD)(pPCD) & 0x00000007) == 0);

        //
        //  Format and collect Process data
        //
        memcpy (pPCD, &pcdTotal, sizeof (pcdTotal));
        pPCD->CounterBlock.ByteLength = sizeof (PROCESS_COUNTER_DATA);
        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pPCD[1];

    }

    // flag so we don't have to get the oldest Process Creation time again.
    bOldestProcessTime = TRUE;

    // Note number of process instances

    pProcessDataDefinition->ProcessObjectType.NumInstances =
        NumProcessInstances;

    //
    //  Now we know how large an area we used for the
    //  Process definition, so we can update the offset
    //  to the next object definition
    //

    *lpcbTotalBytes =
        pProcessDataDefinition->ProcessObjectType.TotalByteLength =
        (DWORD)((PCHAR) pPerfInstanceDefinition -
        (PCHAR) pProcessDataDefinition);

#if DBG
    if (*lpcbTotalBytes > TotalLen ) {
        DbgPrint ("\nPERFPROC: Process Perf Ctr. Instance Size Underestimated:");
        DbgPrint ("\nPERFPROC:   Estimated size: %d, Actual Size: %d", TotalLen, *lpcbTotalBytes);
    }
#endif

    *lppData = (LPVOID) pPerfInstanceDefinition;

    *lpNumObjectTypes = 1;

    return ERROR_SUCCESS;
}
