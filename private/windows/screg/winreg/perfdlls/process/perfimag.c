/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perfimag.c

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
#include <winperf.h>
#include <ntprfctr.h>
#include <perfutil.h>
#include "perfsprc.h"
#include "perfmsg.h"
#include "dataimag.h"

static
DWORD APIENTRY
BuildImageObject (
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes,
    IN      BOOL    bLongImageName
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

    IN      BOOL    bLongImageName
            TRUE -- use the full path of the library file name in the instance
            FALSE - use only the file name in the instance

    Returns:

             0 if successful, else Win 32 error code of failure

--*/
{
    DWORD   TotalLen;            //  Length of the total return block

    PIMAGE_DATA_DEFINITION          pImageDataDefinition;
    PPERF_INSTANCE_DEFINITION       pPerfInstanceDefinition;
    PIMAGE_COUNTER_DATA             pICD;
    DWORD                           dwNumInstances;

    DWORD                           dwImageNameLength;

    DWORD                           dwProcessIndex;

    PPROCESS_VA_INFO                pThisProcess;
    PMODINFO                        pThisImage;

    dwNumInstances = 0;

    pImageDataDefinition = (IMAGE_DATA_DEFINITION *) *lppData;

    //
    //  Check for sufficient space for Image object type definition
    //

    TotalLen = sizeof(IMAGE_DATA_DEFINITION) +
               sizeof(PERF_INSTANCE_DEFINITION) +
               MAX_PROCESS_NAME_LENGTH +
               sizeof(IMAGE_COUNTER_DATA);

    if ( *lpcbTotalBytes < TotalLen ) {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    //
    //  Define Page File data block
    //

    memcpy(pImageDataDefinition,
        &ImageDataDefinition,
        sizeof(IMAGE_DATA_DEFINITION));

    // update object title index if this is a Long Image object

    if (bLongImageName) {
        pImageDataDefinition->ImageObjectType.ObjectNameTitleIndex =
            LONG_IMAGE_OBJECT_TITLE_INDEX;
        pImageDataDefinition->ImageObjectType.ObjectHelpTitleIndex =
            LONG_IMAGE_OBJECT_TITLE_INDEX + 1;
    }

    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                &pImageDataDefinition[1];

    // Now load data for each Image

    pThisProcess = pProcessVaInfo;
    dwProcessIndex = 0;
    TotalLen = sizeof(IMAGE_DATA_DEFINITION);

    while (pThisProcess) {

        pThisImage = pThisProcess->pMemBlockInfo;

        while (pThisImage) {

            dwImageNameLength = (bLongImageName ? pThisImage->LongInstanceName->Length :
                    pThisImage->InstanceName->Length);
            dwImageNameLength += sizeof(WCHAR);
            dwImageNameLength = QWORD_MULTIPLE(dwImageNameLength);
        
            // see if this instance will fit

            TotalLen += sizeof (PERF_INSTANCE_DEFINITION) +
                dwImageNameLength + 
//                (MAX_PROCESS_NAME_LENGTH + 1) * sizeof (WCHAR) +
                sizeof (DWORD) +
                sizeof (IMAGE_COUNTER_DATA);

            if ( *lpcbTotalBytes < TotalLen ) {
                *lpcbTotalBytes = (DWORD) 0;
                *lpNumObjectTypes = (DWORD) 0;
                return ERROR_MORE_DATA;
            }

            MonBuildInstanceDefinition (pPerfInstanceDefinition,
                (PVOID *) &pICD,
                EXPROCESS_OBJECT_TITLE_INDEX,
                dwProcessIndex,
                (DWORD)-1,
                (bLongImageName ? pThisImage->LongInstanceName->Buffer :
                    pThisImage->InstanceName->Buffer));

            pICD->CounterBlock.ByteLength = sizeof(IMAGE_COUNTER_DATA);

            pICD->ImageAddrNoAccess           = pThisImage->CommitVector[NOACCESS];
            pICD->ImageAddrReadOnly           = pThisImage->CommitVector[READONLY];
            pICD->ImageAddrReadWrite          = pThisImage->CommitVector[READWRITE];
            pICD->ImageAddrWriteCopy          = pThisImage->CommitVector[WRITECOPY];
            pICD->ImageAddrExecute            = pThisImage->CommitVector[EXECUTE];
            pICD->ImageAddrExecuteReadOnly    = pThisImage->CommitVector[EXECUTEREAD];
            pICD->ImageAddrExecuteReadWrite   = pThisImage->CommitVector[EXECUTEREADWRITE];
            pICD->ImageAddrExecuteWriteCopy   = pThisImage->CommitVector[EXECUTEWRITECOPY];

            pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)&pICD[1];

            // adjust Total Length value to reflect ACTUAL size used

            TotalLen = (DWORD)((PCHAR) pPerfInstanceDefinition -
                (PCHAR) pImageDataDefinition);

            dwNumInstances++;

            pThisImage = pThisImage->pNextModule;
        }
        pThisProcess = pThisProcess->pNextProcess;
        dwProcessIndex++;
    }

    pImageDataDefinition->ImageObjectType.NumInstances += dwNumInstances;

    *lpcbTotalBytes =
        pImageDataDefinition->ImageObjectType.TotalByteLength =
            (DWORD)((PCHAR) pPerfInstanceDefinition -
            (PCHAR) pImageDataDefinition);

#if DBG
    if (*lpcbTotalBytes > TotalLen ) {
        DbgPrint ("\nPERFPROC: Image Perf Ctr. Instance Size Underestimated:");
        DbgPrint ("\nPERFPROC:   Estimated size: %d, Actual Size: %d", TotalLen, *lpcbTotalBytes);
    }
#endif

    *lppData = (LPVOID) pPerfInstanceDefinition;

    // increment number of objects in this data block
    *lpNumObjectTypes = 1;

    return ERROR_SUCCESS;
}

DWORD APIENTRY
CollectImageObjectData (
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
    return BuildImageObject (
                lppData,
                lpcbTotalBytes,
                lpNumObjectTypes,
                FALSE); // use short names
}

DWORD APIENTRY
CollectLongImageObjectData (
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
    return BuildImageObject (
                lppData,
                lpcbTotalBytes,
                lpNumObjectTypes,
                TRUE); // use long names
}
