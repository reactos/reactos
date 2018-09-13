/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perfthrd.c

Abstract:

    This file implements an Performance Object that presents
    Thread performance object data

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
#include "datathrd.h"

DWORD APIENTRY
CollectThreadObjectData (
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
    LONG    lReturn = ERROR_SUCCESS;

    DWORD  TotalLen;            //  Length of the total return block

    THREAD_DATA_DEFINITION *pThreadDataDefinition;
    PERF_INSTANCE_DEFINITION *pPerfInstanceDefinition;
    PTHREAD_COUNTER_DATA    pTCD;
    THREAD_COUNTER_DATA     tcdTotal;

    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    ULONG ProcessNumber;
    ULONG NumThreadInstances;
    ULONG ThreadNumber;
    ULONG ProcessBufferOffset;
    BOOLEAN NullProcess;

    // total thread accumulator variables

    UNICODE_STRING ThreadName;
    WCHAR ThreadNameBuffer[MAX_THREAD_NAME_LENGTH+1];

    pThreadDataDefinition = (THREAD_DATA_DEFINITION *) *lppData;

    //
    //  Check for sufficient space for Thread object type definition
    //

    TotalLen = sizeof(THREAD_DATA_DEFINITION) +
               sizeof(PERF_INSTANCE_DEFINITION) +
               sizeof(THREAD_COUNTER_DATA);

    if ( *lpcbTotalBytes < TotalLen ) {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    //
    //  Define Thread data block
    //

    ThreadName.Length =
    ThreadName.MaximumLength = (MAX_THREAD_NAME_LENGTH + 1) * sizeof(WCHAR);
    ThreadName.Buffer = ThreadNameBuffer;

    memcpy(pThreadDataDefinition,
           &ThreadDataDefinition,
           sizeof(THREAD_DATA_DEFINITION));

    pThreadDataDefinition->ThreadObjectType.PerfTime = SysTimeInfo.CurrentTime;

    ProcessBufferOffset = 0;

    // Now collect data for each Thread

    ProcessNumber = 0;
    NumThreadInstances = 0;

    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pProcessBuffer;

    pPerfInstanceDefinition =
        (PPERF_INSTANCE_DEFINITION)&pThreadDataDefinition[1];
    TotalLen = sizeof(THREAD_DATA_DEFINITION);

    // clear total accumulator
    memset (&tcdTotal, 0, sizeof (tcdTotal));

    while ( TRUE ) {

        if ( ProcessInfo->ImageName.Buffer != NULL ||
             ProcessInfo->NumberOfThreads > 0 ) {
            NullProcess = FALSE;
        } else {
            NullProcess = TRUE;
        }

        ThreadNumber = 0;       //  Thread number of this process

        ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);

        while ( !NullProcess &&
                ThreadNumber < ProcessInfo->NumberOfThreads ) {

            TotalLen += sizeof(PERF_INSTANCE_DEFINITION) +
                       (MAX_THREAD_NAME_LENGTH+1+sizeof(DWORD))*
                           sizeof(WCHAR) +
                       sizeof (THREAD_COUNTER_DATA);

            if ( *lpcbTotalBytes < TotalLen ) {
                *lpcbTotalBytes = (DWORD) 0;
                *lpNumObjectTypes = (DWORD) 0;
                return ERROR_MORE_DATA;
            }

            // The only name we've got is the thread number

            RtlIntegerToUnicodeString(ThreadNumber,
                                      10,
                                      &ThreadName);

            MonBuildInstanceDefinition(pPerfInstanceDefinition,
                (PVOID *) &pTCD,
                PROCESS_OBJECT_TITLE_INDEX,
                ProcessNumber,
                (DWORD)-1,
                ThreadName.Buffer);

            // test structure for Quadword Alignment
            assert (((DWORD)(pTCD) & 0x00000007) == 0);

            //
            //
            //  Format and collect Thread data
            //

            pTCD->CounterBlock.ByteLength = sizeof(THREAD_COUNTER_DATA);

            //
            //  Convert User time from 100 nsec units to counter
            //  frequency.
            //
            tcdTotal.ProcessorTime +=
                pTCD->ProcessorTime = ThreadInfo->KernelTime.QuadPart +
                                        ThreadInfo->UserTime.QuadPart;

            tcdTotal.UserTime +=
                pTCD->UserTime = ThreadInfo->UserTime.QuadPart;
            tcdTotal.KernelTime +=
                pTCD->KernelTime = ThreadInfo->KernelTime.QuadPart;

            tcdTotal.ContextSwitches +=
                pTCD->ContextSwitches = ThreadInfo->ContextSwitches;

            pTCD->ThreadElapsedTime = ThreadInfo->CreateTime.QuadPart;

            pTCD->ThreadPriority = (ThreadInfo->ClientId.UniqueProcess == 0) ?
                0 : ThreadInfo->Priority;

            pTCD->ThreadBasePriority = ThreadInfo->BasePriority;
            pTCD->ThreadStartAddr = ThreadInfo->StartAddress;
            pTCD->ThreadState =
                (DWORD)((ThreadInfo->ThreadState > 7) ?
                    7 : ThreadInfo->ThreadState);
            pTCD->WaitReason = (DWORD)ThreadInfo->WaitReason;

            // now stuff in the process and thread id's
            pTCD->ProcessId = HandleToUlong(ThreadInfo->ClientId.UniqueProcess);
            pTCD->ThreadId = HandleToUlong(ThreadInfo->ClientId.UniqueThread);

            pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pTCD[1];

            NumThreadInstances++;
            ThreadNumber++;
            ThreadInfo++;
        }

        if ( !NullProcess ) {
            ProcessNumber++;
        }

        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }

        ProcessBufferOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
                          &pProcessBuffer[ProcessBufferOffset];
    }

    if (NumThreadInstances > 0) {

        // See if the total instance will fit

        TotalLen += sizeof(PERF_INSTANCE_DEFINITION) +
                    (MAX_THREAD_NAME_LENGTH+1+sizeof(DWORD))*
                        sizeof(WCHAR) +
                    sizeof (THREAD_COUNTER_DATA);

        if ( *lpcbTotalBytes < TotalLen ) {
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_MORE_DATA;
        }

        // set the Total Elapsed Time to be the current time so that it will
        // show up as 0 when displayed.
        tcdTotal.ThreadElapsedTime = pThreadDataDefinition->ThreadObjectType.PerfTime.QuadPart;

        // use the "total" for this instance
        MonBuildInstanceDefinition(pPerfInstanceDefinition,
            (PVOID *) &pTCD,
            PROCESS_OBJECT_TITLE_INDEX,
            ProcessNumber,
            (DWORD)-1,
            wszTotal);

        // test structure for Quadword Alignment
        assert (((DWORD)(pTCD) & 0x00000007) == 0);

        //
        //
        //  Format and collect Thread data
        //

        memcpy (pTCD, &tcdTotal, sizeof(tcdTotal));
        pTCD->CounterBlock.ByteLength = sizeof(THREAD_COUNTER_DATA);

        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pTCD[1];

        NumThreadInstances++;
    }
    // Note number of Thread instances

    pThreadDataDefinition->ThreadObjectType.NumInstances =
        NumThreadInstances;

    //
    //  Now we know how large an area we used for the
    //  Thread definition, so we can update the offset
    //  to the next object definition
    //

    *lpcbTotalBytes =
        pThreadDataDefinition->ThreadObjectType.TotalByteLength =
            (DWORD)((PCHAR) pPerfInstanceDefinition -
            (PCHAR) pThreadDataDefinition);

#if DBG
    if (*lpcbTotalBytes > TotalLen ) {
        DbgPrint ("\nPERFPROC: Thread Perf Ctr. Instance Size Underestimated:");
        DbgPrint ("\nPERFPROC:   Estimated size: %d, Actual Size: %d", TotalLen, *lpcbTotalBytes);
    }
#endif

    *lppData = (LPVOID)pPerfInstanceDefinition;

    *lpNumObjectTypes = 1;

    return lReturn;

}
