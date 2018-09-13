/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perftdet.c

Abstract:

    This file implements an Performance Object that presents
    Thread details performance object data

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
#include <winperf.h>
#include <ntprfctr.h>
#include <perfutil.h>
#include "perfsprc.h"
#include "perfmsg.h"
#include "datatdet.h"

DWORD APIENTRY
CollectThreadDetailsObjectData (
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
    DWORD  TotalLen;            //  Length of the total return block

    PTHREAD_DETAILS_DATA_DEFINITION pThreadDetailDataDefinition;
    PPERF_INSTANCE_DEFINITION       pPerfInstanceDefinition;
    PTHREAD_DETAILS_COUNTER_DATA    pTDCD;

    PSYSTEM_PROCESS_INFORMATION     ProcessInfo;
    PSYSTEM_THREAD_INFORMATION      ThreadInfo;

    ULONG ProcessNumber;
    ULONG NumThreadInstances;
    ULONG ThreadNumber;
    ULONG ProcessBufferOffset;
    BOOLEAN NullProcess;

    NTSTATUS            Status;     // return from Nt Calls
    LONGLONG		llPcValue;  // value of current thread PC
    OBJECT_ATTRIBUTES   Obja;       // object attributes for thread context
    HANDLE              hThread;    // handle to current thread
    CONTEXT             ThreadContext; // current thread context struct

    UNICODE_STRING ThreadName;
    WCHAR ThreadNameBuffer[MAX_THREAD_NAME_LENGTH+1];

    pThreadDetailDataDefinition = (THREAD_DETAILS_DATA_DEFINITION *) *lppData;

    //
    //  Check for sufficient space for Thread object type definition
    //

    TotalLen = sizeof(THREAD_DETAILS_DATA_DEFINITION) +
               sizeof(PERF_INSTANCE_DEFINITION) +
               sizeof(THREAD_DETAILS_COUNTER_DATA);

    if ( *lpcbTotalBytes < TotalLen ) {
        *lpcbTotalBytes = 0;
        *lpNumObjectTypes = 0;
        return ERROR_MORE_DATA;
    }

    //
    //  Define Thread data block
    //

    ThreadName.Length =
    ThreadName.MaximumLength = (MAX_THREAD_NAME_LENGTH + 1) * sizeof(WCHAR);
    ThreadName.Buffer = ThreadNameBuffer;

    memcpy (pThreadDetailDataDefinition,
           &ThreadDetailsDataDefinition,
           sizeof(THREAD_DETAILS_DATA_DEFINITION));

    ProcessBufferOffset = 0;

    // Now collect data for each Thread

    ProcessNumber = 0;
    NumThreadInstances = 0;

    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pProcessBuffer;

    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                              &pThreadDetailDataDefinition[1];

    TotalLen = sizeof (THREAD_DETAILS_DATA_DEFINITION);

    while ( TRUE ) {

        if ( ProcessInfo->ImageName.Buffer != NULL ||
            ProcessInfo->NumberOfThreads > 0 ) {
            NullProcess = FALSE;
            ThreadNumber = 0;       //  Thread number of this process
            ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
        } else {
            NullProcess = TRUE;
        }

        while ( !NullProcess &&
                ThreadNumber < ProcessInfo->NumberOfThreads ) {

            TotalLen += sizeof(PERF_INSTANCE_DEFINITION) +
                       (MAX_THREAD_NAME_LENGTH+1+sizeof(DWORD))*
                           sizeof(WCHAR) +
                       sizeof (THREAD_DETAILS_COUNTER_DATA);

            if ( *lpcbTotalBytes < TotalLen ) {
                *lpcbTotalBytes = 0;
                *lpNumObjectTypes = 0;
                return ERROR_MORE_DATA;
            }

            // Get Thread Context Information for Current PC field

            llPcValue = 0;
            InitializeObjectAttributes(&Obja, NULL, 0, NULL, NULL);
            Status = NtOpenThread(
                        &hThread,
                        THREAD_GET_CONTEXT,
                        &Obja,
                        &ThreadInfo->ClientId
                        );
            if ( NT_SUCCESS(Status) ) {
                ThreadContext.ContextFlags = CONTEXT_CONTROL;
                Status = NtGetContextThread(hThread,&ThreadContext);
                NtClose(hThread);
                if ( NT_SUCCESS(Status) ) {
                    llPcValue = (LONGLONG)CONTEXT_TO_PROGRAM_COUNTER(&ThreadContext);
                } else {
                    llPcValue = 0;  // an error occured so send back 0 PC
                }
            } else {
                llPcValue = 0;  // an error occured so send back 0 PC
            }

            // The only name we've got is the thread number

            RtlIntegerToUnicodeString(ThreadNumber,
                                      10,
                                      &ThreadName);

            MonBuildInstanceDefinition(pPerfInstanceDefinition,
                (PVOID *) &pTDCD,
                EXPROCESS_OBJECT_TITLE_INDEX,
                ProcessNumber,
                (DWORD)-1,
                ThreadName.Buffer);

            //
            //
            //  Format and collect Thread data
            //

            pTDCD->CounterBlock.ByteLength = sizeof (THREAD_DETAILS_COUNTER_DATA);

            pTDCD->UserPc = llPcValue;

            pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pTDCD[1];
            NumThreadInstances++;
            ThreadNumber++;
            ThreadInfo++;
        }

        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }

        ProcessBufferOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
                          &pProcessBuffer[ProcessBufferOffset];

        if ( !NullProcess ) {
            ProcessNumber++;
        }
    }

    // Note number of Thread instances

    pThreadDetailDataDefinition->ThreadDetailsObjectType.NumInstances =
        NumThreadInstances;

    //
    //  Now we know how large an area we used for the
    //  Thread definition, so we can update the offset
    //  to the next object definition
    //

    *lpcbTotalBytes =
        pThreadDetailDataDefinition->ThreadDetailsObjectType.TotalByteLength =
            (DWORD)((PCHAR) pPerfInstanceDefinition -
            (PCHAR) pThreadDetailDataDefinition);

#if DBG
    if (*lpcbTotalBytes > TotalLen ) {
        DbgPrint ("\nPERFPROC: Thread Details Perf Ctr. Instance Size Underestimated:");
        DbgPrint ("\nPERFPROC:   Estimated size: %d, Actual Size: %d", TotalLen, *lpcbTotalBytes);
    }
#endif

    *lppData = (LPVOID) pPerfInstanceDefinition;

    *lpNumObjectTypes = 1;

    return ERROR_SUCCESS;
}
