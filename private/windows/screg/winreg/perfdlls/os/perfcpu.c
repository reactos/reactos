/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perfcpu.c

Abstract:

    This file implements an Performance Object that presents
    System Processor performance object data

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
#include "perfos.h"
#include "perfosmc.h"
#include "datacpu.h"

DWORD   dwCpuOpenCount = 0;        // count of "Open" threads

// variables local to this module.
static  SYSTEM_INTERRUPT_INFORMATION *pProcessorInterruptInformation = NULL;
static  DWORD dwInterruptInfoBufferSize = 0;
static  UCHAR *pProcessorBuffer;
static  ULONG ProcessorBufSize;


DWORD APIENTRY
OpenProcessorObject (
    LPWSTR lpDeviceNames
    )
/*++

Routine Description:

    This routine will initialize the data structures used to pass
    data back to the registry

Arguments:

    Pointer to object ID of each device to be opened (PerfGen)

Return Value:

    None.

--*/
{
    DWORD   status = ERROR_SUCCESS;
    //
    //  Since WINLOGON is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). the registry routines will
    //  limit access to the initialization routine to only one thread
    //  at a time so synchronization (i.e. reentrancy) should not be
    //  a problem
    //

    if (!dwCpuOpenCount) {
        dwInterruptInfoBufferSize = (ULONG)BasicInfo.NumberOfProcessors *
            sizeof (SYSTEM_INTERRUPT_INFORMATION);

        pProcessorInterruptInformation = ALLOCMEM (hLibHeap,
            HEAP_ZERO_MEMORY, dwInterruptInfoBufferSize);

        if (pProcessorInterruptInformation == NULL) {
            status = ERROR_OUTOFMEMORY;
            goto OpenExitPoint;
        }

        ProcessorBufSize = BasicInfo.NumberOfProcessors *
                 sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);

        pProcessorBuffer = ALLOCMEM(hLibHeap, HEAP_ZERO_MEMORY,
                                                  ProcessorBufSize);
        if (pProcessorBuffer == NULL) {
            status = ERROR_OUTOFMEMORY;
            goto OpenExitPoint;
        }
    }
    dwCpuOpenCount++;  // increment OPEN counter

    status = ERROR_SUCCESS; // for successful exit

OpenExitPoint:

    return status;
}

DWORD APIENTRY
CollectProcessorObjectData (
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
    DWORD   TotalLen;            //  Length of the total return block
    DWORD   *pdwCounter;

    DWORD   dwReturnedBufferSize;

    PPROCESSOR_DATA_DEFINITION  pProcessorDataDefinition;
    PPROCESSOR_COUNTER_DATA     pPCD;

    PROCESSOR_COUNTER_DATA      pcdTotalData;

    PERF_INSTANCE_DEFINITION *pPerfInstanceDefinition;

    ULONG CurProc;

    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UNALIGNED *pliCounter;
    LARGE_INTEGER UNALIGNED *pliProcessorTime;

    UNICODE_STRING ProcessorName;
    WCHAR ProcessorNameBuffer[512];

    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *pProcessorInformation = NULL;

    SYSTEM_INTERRUPT_INFORMATION *pThisProcessorInterruptInformation = NULL;
    DWORD   dwInterruptInfoBufferSize;
    ULONG      Remainder;
    NTSTATUS    ntStatus;

    DWORD       dwQueryType;

    //
    //  Check for sufficient space for processor data
    //

    // check for QUADWORD alignment of incoming pointer
    assert (((DWORD)(*lppData) & 0x00000007) == 0);

    pProcessorDataDefinition = (PROCESSOR_DATA_DEFINITION *) *lppData;

    TotalLen =
        sizeof(PROCESSOR_DATA_DEFINITION) +     // object def header
        ((sizeof (PERF_INSTANCE_DEFINITION) +   // plus an instance for
            ((MAX_INSTANCE_NAME + 1) * sizeof(WCHAR)) +
            sizeof (PROCESSOR_COUNTER_DATA)) *     // each processor and
            (BasicInfo.NumberOfProcessors + 1)); // the "total" instance

    if ( *lpcbTotalBytes < TotalLen ) {
        lReturn = ERROR_MORE_DATA;
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        goto COLLECT_BAIL_OUT;
    }

    //
    // Get processor data from system
    //

    if ( ProcessorBufSize ) {
        ntStatus = NtQuerySystemInformation(
            SystemProcessorPerformanceInformation,
            pProcessorBuffer,
            ProcessorBufSize,
            &dwReturnedBufferSize
            );

        if (!NT_SUCCESS(ntStatus)) {
            // clear buffer & log error
            ReportEvent (hEventLog,
                EVENTLOG_WARNING_TYPE,
                0,
                PERFOS_UNABLE_QUERY_PROCSSOR_INFO,
                NULL,
                0,
                sizeof(DWORD),
                NULL,
                (LPVOID)&ntStatus);

            memset (pProcessorBuffer, 0, ProcessorBufSize);
        }
    }

    //
    // get system interrupt information by processor
    //
    dwInterruptInfoBufferSize = (ULONG)BasicInfo.NumberOfProcessors *
        sizeof (SYSTEM_INTERRUPT_INFORMATION);

    ntStatus = NtQuerySystemInformation(
        SystemInterruptInformation,
        pProcessorInterruptInformation,
        dwInterruptInfoBufferSize,
        &dwReturnedBufferSize
    );

    if (!NT_SUCCESS(ntStatus)) {
        // clear buffer & log error
        ReportEvent (hEventLog,
            EVENTLOG_WARNING_TYPE,
            0,
            PERFOS_UNABLE_QUERY_INTERRUPT_INFO,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            (LPVOID)&ntStatus);

        memset (pProcessorInterruptInformation, 0,
                    (BasicInfo.NumberOfProcessors *
                    sizeof (SYSTEM_INTERRUPT_INFORMATION)));
    }

    // clear the "Total" instance
    memset (&pcdTotalData, 0, sizeof (pcdTotalData));

    //  Define processor data block
    //

    memcpy (pProcessorDataDefinition,
        &ProcessorDataDefinition,
        sizeof(PROCESSOR_DATA_DEFINITION));

    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                            &pProcessorDataDefinition[1];

    pProcessorInformation = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *)
                                pProcessorBuffer;

    // point to the first processor in the returned array of interrupt
    // information. data is returned as an array of structures.

    pThisProcessorInterruptInformation = pProcessorInterruptInformation;

    for ( CurProc = 0;
        CurProc < (ULONG) BasicInfo.NumberOfProcessors;
        CurProc++ ) {

        //
        //  Define processor instance 0;
        //  More could be defined like this
        //

        ProcessorName.Length = 0;
        ProcessorName.MaximumLength = sizeof(ProcessorNameBuffer);
        ProcessorName.Buffer = ProcessorNameBuffer;

        RtlIntegerToUnicodeString(CurProc, 10, &ProcessorName);

        MonBuildInstanceDefinition(pPerfInstanceDefinition,
                                (PVOID *) &pPCD,
                                0,
                                0,
                                (DWORD)-1,
                                ProcessorNameBuffer);

        // test for Quadword Alignment
        assert (((DWORD)(pPCD) & 0x00000007) == 0);
        //
        //  Format and collect processor data.  While doing so,
        //  accumulate totals in the System Object Type data block.
        //  Pointers to these were initialized in QuerySystemData.
        //

        pPCD->CounterBlock.ByteLength = sizeof (PROCESSOR_COUNTER_DATA);

        pcdTotalData.ProcessorTime +=
            pPCD->ProcessorTime =
                pProcessorInformation->IdleTime.QuadPart;
        pcdTotalData.UserTime +=
            pPCD->UserTime      =
                pProcessorInformation->UserTime.QuadPart;
        // kernel time is total kernel time less the time spent in the
        // idle thread for that processor
        pcdTotalData.KernelTime +=
            pPCD->KernelTime    =
                pProcessorInformation->KernelTime.QuadPart -
                                pPCD->ProcessorTime;

        pcdTotalData.Interrupts +=
            pPCD->Interrupts    = pProcessorInformation->InterruptCount;
        pcdTotalData.DpcTime +=
            pPCD->DpcTime       = pProcessorInformation->DpcTime.QuadPart;
        pcdTotalData.InterruptTime +=
            pPCD->InterruptTime =
                pProcessorInformation->InterruptTime.QuadPart;

        pcdTotalData.DpcCountRate +=
            pPCD->DpcCountRate  =
                pThisProcessorInterruptInformation->DpcCount;

        pcdTotalData.DpcRate +=
            pPCD->DpcRate       =
                pThisProcessorInterruptInformation->DpcRate;
        pcdTotalData.DpcBypassRate +=
            pPCD->DpcBypassRate =
                pThisProcessorInterruptInformation->DpcBypassCount;
        pcdTotalData.ApcBypassRate +=
            pPCD->ApcBypassRate =
                pThisProcessorInterruptInformation->ApcBypassCount;

        //
        //  Advance to next processor
        //

        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pPCD[1];

        // point to next processor's data in return array(s)
        pProcessorInformation++;
        pThisProcessorInterruptInformation++;
    }

    // do the total instance now
    ProcessorName.Length = (WORD)((lstrlenW (wszTotal) + 1) * sizeof (WCHAR));
    ProcessorName.MaximumLength = (WORD)(sizeof (ProcessorNameBuffer));
    lstrcpyW (ProcessorNameBuffer, wszTotal);
    ProcessorName.Buffer = ProcessorNameBuffer;

    MonBuildInstanceDefinition(pPerfInstanceDefinition,
                            (PVOID *) &pPCD,
                            0,
                            0,
                            (DWORD)-1,
                            ProcessorNameBuffer);

    // define the size
    pcdTotalData.CounterBlock.ByteLength = sizeof (PROCESSOR_COUNTER_DATA);

    // adjust the total values of the time fields to the number of
    // processors to "normalize" the values

    pcdTotalData.ProcessorTime /= BasicInfo.NumberOfProcessors;
    pcdTotalData.UserTime /= BasicInfo.NumberOfProcessors;
    pcdTotalData.KernelTime /= BasicInfo.NumberOfProcessors;

    pcdTotalData.DpcTime /= BasicInfo.NumberOfProcessors;
    pcdTotalData.InterruptTime /= BasicInfo.NumberOfProcessors;

    // these fields are OK as totals
    //
    //  pcdTotalData.Interrupts
    //  pcdTotalData.DpcCountRate
    //  pcdTotalData.DpcRate
    //  pcdTotalData.DpcBypassRate
    //  pcdTotalData.ApcBypassRate

    // copy total data to buffer
    memcpy (pPCD, &pcdTotalData, sizeof (pcdTotalData));

    // adjust local buffer pointer
    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pPCD[1];

    //
    //  Now we know how large an area we used for the
    //  processor definition, so we can update the offset
    //  to the next object definition
    //

    pProcessorDataDefinition->ProcessorObjectType.NumInstances =
        BasicInfo.NumberOfProcessors + 1;

    *lppData = (LPVOID)pPerfInstanceDefinition;

    // round up buffer to the nearest QUAD WORD

    *lppData = ALIGN_ON_QWORD (*lppData);

    *lpcbTotalBytes =
        pProcessorDataDefinition->ProcessorObjectType.TotalByteLength =
            (DWORD)((LPBYTE) pPerfInstanceDefinition -
            (LPBYTE) pProcessorDataDefinition);

#if DBG
    if (*lpcbTotalBytes > TotalLen ) {
        DbgPrint ("\nPERFOS: Processor Perf Ctr. Instance Size Underestimated:");
        DbgPrint ("\nPERFOS:   Estimated size: %d, Actual Size: %d", TotalLen, *lpcbTotalBytes);
    }
#endif

    *lpNumObjectTypes = 1;

    return ERROR_SUCCESS;

COLLECT_BAIL_OUT:

    return lReturn;
}

#pragma warning (disable : 4706)
DWORD APIENTRY
CloseProcessorObject (
)
/*++

Routine Description:

    This routine closes the open handles

Arguments:

    None.


Return Value:

    ERROR_SUCCESS

--*/

{
    if (dwCpuOpenCount > 0) {
        if (!(--dwCpuOpenCount)) { // when this is the last thread...
            // close stuff here
            if (hLibHeap != NULL) {
                if (pProcessorInterruptInformation != NULL) {
                    FREEMEM (hLibHeap, 0, pProcessorInterruptInformation);
                    pProcessorInterruptInformation = NULL;
                }

                if (pProcessorBuffer != NULL) {
                    FREEMEM (hLibHeap, 0, pProcessorBuffer);
                    pProcessorBuffer = NULL;
                }
            }
        }
    } else {
        // if the open count is 0, then these should have been deleted
        assert (pProcessorBuffer == NULL);
        assert (pProcessorInterruptInformation == NULL);
    }

    return ERROR_SUCCESS;

}
#pragma warning (default : 4706)
