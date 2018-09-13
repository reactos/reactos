/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perfexpr.c

Abstract:

    This file implements an Performance Object that presents
    Extended Process performance object data

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
#include "dataexpr.h"

DWORD APIENTRY
CollectExProcessObjectData (
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
    DWORD   *pdwCounter;
    DWORD   NumExProcessInstances;

    PPROCESS_VA_INFO            pThisProcess;   // pointer to current process
    PERF_INSTANCE_DEFINITION    *pPerfInstanceDefinition;
    EXPROCESS_DATA_DEFINITION   *pExProcessDataDefinition;

    PEXPROCESS_COUNTER_DATA     pECD;

    if (pProcessVaInfo) {   // process only if a buffer is available
        pExProcessDataDefinition = (EXPROCESS_DATA_DEFINITION *)*lppData;

        // check for sufficient space in buffer for at least one entry

        TotalLen = sizeof(EXPROCESS_DATA_DEFINITION) +
                    sizeof(PERF_INSTANCE_DEFINITION) +
                    (MAX_PROCESS_NAME_LENGTH + 1) * sizeof (WCHAR) +
                    sizeof(EXPROCESS_COUNTER_DATA);

        if (*lpcbTotalBytes < TotalLen) {
            *lpcbTotalBytes = 0;
            *lpNumObjectTypes = 0;
            return ERROR_MORE_DATA;
        }

        // copy process data block to buffer

        memcpy (pExProcessDataDefinition,
                        &ExProcessDataDefinition,
                        sizeof(EXPROCESS_DATA_DEFINITION));

        NumExProcessInstances = 0;

        pThisProcess = pProcessVaInfo;

        TotalLen = sizeof(EXPROCESS_DATA_DEFINITION);

        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                    &pExProcessDataDefinition[1];

        while (pThisProcess) {

            // see if this instance will fit

            TotalLen += sizeof (PERF_INSTANCE_DEFINITION) +
                (MAX_PROCESS_NAME_LENGTH + 1) * sizeof (WCHAR) +
                sizeof (DWORD) +
                sizeof (EXPROCESS_COUNTER_DATA);

            if (*lpcbTotalBytes < TotalLen) {
                *lpcbTotalBytes = 0;
                *lpNumObjectTypes = 0;
                return ERROR_MORE_DATA;
            }

            MonBuildInstanceDefinition (pPerfInstanceDefinition,
                (PVOID *) &pECD,
                0,
                0,
                (DWORD)-1,
                pThisProcess->pProcessName->Buffer);

            NumExProcessInstances++;

            pECD->CounterBlock.ByteLength = sizeof (EXPROCESS_COUNTER_DATA);

            // load counters from the process va data structure

            pECD->ProcessId             = pThisProcess->dwProcessId;
            pECD->ImageReservedBytes    = pThisProcess->ImageReservedBytes;
            pECD->ImageFreeBytes        = pThisProcess->ImageFreeBytes;
            pECD->ReservedBytes         = pThisProcess->ReservedBytes;
            pECD->FreeBytes             = pThisProcess->FreeBytes;

            pECD->CommitNoAccess        = pThisProcess->MappedCommit[NOACCESS];
            pECD->CommitReadOnly        = pThisProcess->MappedCommit[READONLY];
            pECD->CommitReadWrite       = pThisProcess->MappedCommit[READWRITE];
            pECD->CommitWriteCopy       = pThisProcess->MappedCommit[WRITECOPY];
            pECD->CommitExecute         = pThisProcess->MappedCommit[EXECUTE];
            pECD->CommitExecuteRead     = pThisProcess->MappedCommit[EXECUTEREAD];
            pECD->CommitExecuteWrite    = pThisProcess->MappedCommit[EXECUTEREADWRITE];
            pECD->CommitExecuteWriteCopy = pThisProcess->MappedCommit[EXECUTEWRITECOPY];

            pECD->ReservedNoAccess      = pThisProcess->PrivateCommit[NOACCESS];
            pECD->ReservedReadOnly      = pThisProcess->PrivateCommit[READONLY];
            pECD->ReservedReadWrite     = pThisProcess->PrivateCommit[READWRITE];
            pECD->ReservedWriteCopy     = pThisProcess->PrivateCommit[WRITECOPY];
            pECD->ReservedExecute       = pThisProcess->PrivateCommit[EXECUTE];
            pECD->ReservedExecuteRead   = pThisProcess->PrivateCommit[EXECUTEREAD];
            pECD->ReservedExecuteWrite  = pThisProcess->PrivateCommit[EXECUTEREADWRITE];
            pECD->ReservedExecuteWriteCopy = pThisProcess->PrivateCommit[EXECUTEWRITECOPY];

            pECD->UnassignedNoAccess    = pThisProcess->OrphanTotals.CommitVector[NOACCESS];
            pECD->UnassignedReadOnly    = pThisProcess->OrphanTotals.CommitVector[READONLY];
            pECD->UnassignedReadWrite   = pThisProcess->OrphanTotals.CommitVector[READWRITE];
            pECD->UnassignedWriteCopy   = pThisProcess->OrphanTotals.CommitVector[WRITECOPY];
            pECD->UnassignedExecute     = pThisProcess->OrphanTotals.CommitVector[EXECUTE];
            pECD->UnassignedExecuteRead = pThisProcess->OrphanTotals.CommitVector[EXECUTEREAD];
            pECD->UnassignedExecuteWrite = pThisProcess->OrphanTotals.CommitVector[EXECUTEREADWRITE];
            pECD->UnassignedExecuteWriteCopy = pThisProcess->OrphanTotals.CommitVector[EXECUTEWRITECOPY];

            pECD->ImageTotalNoAccess    = pThisProcess->MemTotals.CommitVector[NOACCESS];
            pECD->ImageTotalReadOnly    = pThisProcess->MemTotals.CommitVector[READONLY];
            pECD->ImageTotalReadWrite   = pThisProcess->MemTotals.CommitVector[READWRITE];
            pECD->ImageTotalWriteCopy   = pThisProcess->MemTotals.CommitVector[WRITECOPY];
            pECD->ImageTotalExecute     = pThisProcess->MemTotals.CommitVector[EXECUTE];
            pECD->ImageTotalExecuteRead = pThisProcess->MemTotals.CommitVector[EXECUTEREAD];
            pECD->ImageTotalExecuteWrite = pThisProcess->MemTotals.CommitVector[EXECUTEREADWRITE];
            pECD->ImageTotalExecuteWriteCopy = pThisProcess->MemTotals.CommitVector[EXECUTEWRITECOPY];

            pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pECD[1];

            pThisProcess = pThisProcess->pNextProcess; // point to next process
        } // end while not at end of list

    } // end if valid process info buffer
    else {
        // pProcessVaInfo is NULL.  Initialize the DataDef and return
        // with no data
        pExProcessDataDefinition = (EXPROCESS_DATA_DEFINITION *)*lppData;

        TotalLen = sizeof(EXPROCESS_DATA_DEFINITION) +
            sizeof (PERF_INSTANCE_DEFINITION) +
            (MAX_PROCESS_NAME_LENGTH + 1) * sizeof (WCHAR) +
            sizeof (DWORD) +
            sizeof (EXPROCESS_COUNTER_DATA);

        if (*lpcbTotalBytes < TotalLen) {
            *lpcbTotalBytes = 0;
            *lpNumObjectTypes = 0;
            return ERROR_MORE_DATA;
        }

        // copy process data block to buffer

        memcpy (pExProcessDataDefinition,
                        &ExProcessDataDefinition,
                        sizeof(EXPROCESS_DATA_DEFINITION));

        NumExProcessInstances = 0;

        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                    &pExProcessDataDefinition[1];

    }

    pExProcessDataDefinition->ExProcessObjectType.NumInstances =
        NumExProcessInstances;

    *lpcbTotalBytes =
        pExProcessDataDefinition->ExProcessObjectType.TotalByteLength =
        (DWORD)((PCHAR) pPerfInstanceDefinition -
        (PCHAR) pExProcessDataDefinition);

    *lppData = (LPVOID) pPerfInstanceDefinition;

    *lpNumObjectTypes = 1;

    return ERROR_SUCCESS;
}
