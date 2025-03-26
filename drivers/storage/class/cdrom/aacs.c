/*--

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:

    aacs.c

Abstract:

    The CDROM class driver implementation of handling AACS IOCTLs.

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#include "stddef.h"
#include "string.h"

#include "ntddk.h"
#include "ntddstor.h"
#include "cdrom.h"
#include "ioctl.h"
#include "scratch.h"

#ifdef DEBUG_USE_WPP
#include "aacs.tmh"
#endif

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, DeviceHandleAacsReadMediaKeyBlock)
#pragma alloc_text(PAGE, DeviceHandleAacsStartSession)
#pragma alloc_text(PAGE, DeviceHandleAacsEndSession)
#pragma alloc_text(PAGE, DeviceHandleAacsSendCertificate)
#pragma alloc_text(PAGE, DeviceHandleAacsGetCertificate)
#pragma alloc_text(PAGE, DeviceHandleAacsGetChallengeKey)
#pragma alloc_text(PAGE, DeviceHandleAacsReadSerialNumber)
#pragma alloc_text(PAGE, DeviceHandleAacsReadMediaId)
#pragma alloc_text(PAGE, DeviceHandleAacsReadBindingNonce)
#pragma alloc_text(PAGE, DeviceHandleAacsGenerateBindingNonce)
#pragma alloc_text(PAGE, DeviceHandleReadVolumeId)
#pragma alloc_text(PAGE, DeviceHandleSendChallengeKey)

#endif

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsReadMediaKeyBlock(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:
    This routine is used to process IOCTLs:
        IOCTL_AACS_READ_MEDIA_KEY_BLOCK_SIZE
        IOCTL_AACS_READ_MEDIA_KEY_BLOCK
Arguments:
    DeviceExtension - device context

    Request - the request that will be formatted

    RequestParameters - request parameter structur

    DataLength - data transferred length

Return Value:
    NTSTATUS

  --*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PAACS_LAYER_NUMBER  layerNumber = NULL;
    PVOID               outputBuffer = NULL;
    ULONG               transferSize = sizeof(READ_DVD_STRUCTURES_HEADER);

    PAGED_CODE();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           (PVOID*)&layerNumber,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                (PVOID*)&outputBuffer,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        if (RequestParameters.Parameters.DeviceIoControl.IoControlCode == IOCTL_AACS_READ_MEDIA_KEY_BLOCK)
        {
            // maximum size for this transfer is one pack + header
            transferSize += AACS_MKB_PACK_SIZE;
        }

        if (transferSize > DeviceExtension->ScratchContext.ScratchBufferSize)
        {
            // rare case. normally the size of scratch buffer is 64k.
            status = STATUS_INTERNAL_ERROR;
        }
    }

    if (NT_SUCCESS(status))
    {
        UCHAR                       rmdBlockNumber = 0;
        BOOLEAN                     sendChangedCommand = TRUE;
        BOOLEAN                     shouldRetry = TRUE;
        CDB                         cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.READ_DVD_STRUCTURE.OperationCode = SCSIOP_READ_DVD_STRUCTURE;
        // cdb->AsByte[1] = 0x01; // AACS sub-command not required for this

        cdb.READ_DVD_STRUCTURE.LayerNumber = (UCHAR)(*layerNumber);
        cdb.READ_DVD_STRUCTURE.Format = 0x83; // MKB
        cdb.READ_DVD_STRUCTURE.AllocationLength[0] = (UCHAR)(transferSize >> (8*1));
        cdb.READ_DVD_STRUCTURE.AllocationLength[1] = (UCHAR)(transferSize >> (8*0));

        while (sendChangedCommand)
        {
            // RMDBlockNumber is set to zero....
            // RMDBlockNumber[3] maybe changed for other blocks.
            cdb.READ_DVD_STRUCTURE.RMDBlockNumber[3] = rmdBlockNumber;

            if (shouldRetry)
            {
                status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, transferSize, TRUE, &cdb, 12);
            }

    #ifdef ENABLE_AACS_TESTING
            if (RequestParameters.Parameters.DeviceIoControl.IoControlCode == IOCTL_AACS_READ_MEDIA_KEY_BLOCK_SIZE)
            {
                static const UCHAR results[] = { 0x80, 0x02, 0x00, 0x02 };
                RtlCopyMemory(DeviceExtension->ScratchContext.ScratchBuffer, results, SIZEOF_ARRAY(results));
                status = STATUS_SUCCESS;
            }
            else if (RequestParameters.Parameters.DeviceIoControl.IoControlCode == IOCTL_AACS_READ_MEDIA_KEY_BLOCK)
            {
                static const UCHAR results[] = { 0x80, 0x02, 0x00, 0x02 };
                static const UCHAR defaultFill = 0x30; // '0'
                RtlFillMemory(DeviceExtension->ScratchContext.ScratchBuffer, 0x8004, defaultFill);
                RtlCopyMemory(DeviceExtension->ScratchContext.ScratchBuffer, results, SIZEOF_ARRAY(results));
                status = STATUS_SUCCESS;
            }
    #endif //ENABLE_AACS_TESTING

            if (NT_SUCCESS(status))
            {
                // command succeeded, process data...
                PDVD_DESCRIPTOR_HEADER  header = DeviceExtension->ScratchContext.ScratchBuffer;
                UCHAR                   thisPackNumber = cdb.READ_DVD_STRUCTURE.RMDBlockNumber[3];
                UCHAR                   otherPacks = header->Reserved[1];

                // validate and zero-base the otherPacks
                if (otherPacks == 0)
                {
                    TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                                "AACS: Device is reporting zero total packs (invalid)\n"));
                    *DataLength = 0;
                    status = STATUS_IO_DEVICE_ERROR;
                }
                else
                {
                    otherPacks--;

                    if (RequestParameters.Parameters.DeviceIoControl.IoControlCode == IOCTL_AACS_READ_MEDIA_KEY_BLOCK_SIZE)
                    {
                        // if not already requested last pack, do so now
                        if (otherPacks != thisPackNumber)
                        {
                            // re-send the command for the other pack number.
                            // this is safe here because NT_SUCCESS() is TRUE,
                            // and all the rest of this routine does is SetHardError()
                            // and release of resources we're still about to use.

                            // re-zero the output buffer
                            RtlZeroMemory(DeviceExtension->ScratchContext.ScratchBuffer, sizeof(READ_DVD_STRUCTURES_HEADER));

                            // modify the CDB to get the very last pack of the MKB
                            rmdBlockNumber = otherPacks;

                            transferSize = sizeof(READ_DVD_STRUCTURES_HEADER);

                            // keep items clean
                            ScratchBuffer_ResetItems(DeviceExtension, TRUE);

                            // make sure the loop will be executed for modified command.
                            sendChangedCommand = TRUE;
                            shouldRetry = TRUE;
                        }
                        else
                        {
                            // this request already got the last pack
                            // so just interpret the data
                            REVERSE_SHORT(&header->Length);
                            if (header->Length < sizeof(DVD_DESCRIPTOR_HEADER) - RTL_SIZEOF_THROUGH_FIELD(DVD_DESCRIPTOR_HEADER, Length))
                            {
                                *DataLength = 0;
                                status = STATUS_IO_DEVICE_ERROR;
                            }
                            else
                            {
                                ULONG totalSize = header->Length;
                                // subtract out any remaining bytes in the header
                                // to get the number of usable bytes in this pack
                                totalSize -= sizeof(DVD_DESCRIPTOR_HEADER) - RTL_SIZEOF_THROUGH_FIELD(DVD_DESCRIPTOR_HEADER, Length);
                                totalSize += otherPacks * AACS_MKB_PACK_SIZE;

                                // save the result and complete the request
                                *((PULONG)outputBuffer) = totalSize;
                                *DataLength = sizeof(ULONG);
                                status = STATUS_SUCCESS;
                            }
                            // This will exit the loop of sendChangedCommand
                            sendChangedCommand = FALSE;
                            shouldRetry = FALSE;
                        }
                    }
                    else if (RequestParameters.Parameters.DeviceIoControl.IoControlCode == IOCTL_AACS_READ_MEDIA_KEY_BLOCK)
                    {
                        // make length field native byte ordering
                        REVERSE_SHORT(&header->Length);

                        // exit if getting invalid data from the drive
                        if (header->Length < sizeof(DVD_DESCRIPTOR_HEADER) - RTL_SIZEOF_THROUGH_FIELD(DVD_DESCRIPTOR_HEADER, Length))
                        {
                            *DataLength = 0;
                            status = STATUS_IO_DEVICE_ERROR;
                        }
                        else
                        {
                            // success, how many bytes to copy for this pack?
                            ULONG totalSize = header->Length;
                            size_t originalBufferSize;

                            // subtract out any remaining bytes in the header
                            // to get the number of usable bytes in this pack
                            totalSize -= sizeof(DVD_DESCRIPTOR_HEADER) - RTL_SIZEOF_THROUGH_FIELD(DVD_DESCRIPTOR_HEADER, Length);

                            // if not the final pack, this should be a full transfer per spec
                            NT_ASSERT( (totalSize == AACS_MKB_PACK_SIZE) || (thisPackNumber == otherPacks) );

                            // validate the user's buffer is large enough to accept the full data
                            originalBufferSize = RequestParameters.Parameters.DeviceIoControl.OutputBufferLength;

                            if (originalBufferSize < (totalSize + (AACS_MKB_PACK_SIZE*thisPackNumber)))
                            {
                                // just return a slightly bigger-than-normal size
                                *DataLength = (otherPacks + 1)*AACS_MKB_PACK_SIZE;
                                status = STATUS_BUFFER_TOO_SMALL;
                            }
                            else
                            {
                                PUCHAR whereToCopy;
                                // determine where to copy to the user's memory
                                whereToCopy = outputBuffer;
                                whereToCopy += AACS_MKB_PACK_SIZE * thisPackNumber;

                                RtlCopyMemory(whereToCopy, header->Data, totalSize);

                                // update the Information field here because we already
                                // have calculated the size of the block
                                *DataLength = totalSize + (AACS_MKB_PACK_SIZE * thisPackNumber);
                                status = STATUS_SUCCESS;

                                // if there are more packs to get from the device, send it again....
                                if (thisPackNumber != otherPacks)
                                {
                                    // re-send the command for the next pack number.
                                    // this is safe here because NT_SUCCESS() is TRUE,
                                    // and all the rest of this routine does is SetHardError()
                                    // and release of resources we're still about to use.

                                    // re-zero the output buffer
                                    RtlZeroMemory(DeviceExtension->ScratchContext.ScratchBuffer, sizeof(READ_DVD_STRUCTURES_HEADER));

                                    // modify the CDB to get the next pack of the MKB
                                    rmdBlockNumber = cdb.READ_DVD_STRUCTURE.RMDBlockNumber[3]++;

                                    // modify the SRB to be resent
                                    //
                                    transferSize = AACS_MKB_PACK_SIZE + sizeof(READ_DVD_STRUCTURES_HEADER);

                                    // keep items clean
                                    ScratchBuffer_ResetItems(DeviceExtension, FALSE);

                                    // make sure the loop will be executed for modified command.
                                    sendChangedCommand = TRUE;
                                    shouldRetry = TRUE;
                                }
                                else
                                {
                                    // else, that was the end of the transfer, so just complete the request
                                    sendChangedCommand = FALSE;
                                }
                            }
                        }

                    } // end of IOCTL_AACS_READ_MEDIA_KEY_BLOCK
                }
            } // end of NT_SUCCESS(status)

            if (!NT_SUCCESS(status))
            {
                // command failed.
                sendChangedCommand = FALSE;
            }
        } //end of while (sendChangedCommand)

        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsStartSession(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:
    This routine is used to process IOCTL:
        IOCTL_AACS_START_SESSION
Arguments:
    DeviceExtension - device context

    Request - the request that will be formatted

    RequestParameters - request parameter structur

    DataLength - data transferred length

Return Value:
    NTSTATUS

  --*/
{
    //AacsGetAgid

    NTSTATUS        status = STATUS_SUCCESS;
    PDVD_SESSION_ID sessionId = NULL;

    PAGED_CODE();

    *DataLength = 0;

    status = WdfRequestRetrieveOutputBuffer(Request,
                                            RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                            (PVOID*)&sessionId,
                                            NULL);

    if (NT_SUCCESS(status))
    {
        ULONG   dataTransferLength = sizeof(CDVD_KEY_HEADER) + sizeof(CDVD_REPORT_AGID_DATA);
        CDB     cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.REPORT_KEY.OperationCode = SCSIOP_REPORT_KEY;
        cdb.AsByte[7] = 0x02; // AACS key class
        cdb.REPORT_KEY.AllocationLength[0] = (UCHAR)(dataTransferLength >> (8*1));
        cdb.REPORT_KEY.AllocationLength[1] = (UCHAR)(dataTransferLength >> (8*0));
        cdb.REPORT_KEY.KeyFormat = 0x00; // DVD_REPORT_AGID?

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, dataTransferLength, TRUE, &cdb, 12);

#ifdef ENABLE_AACS_TESTING
        static const UCHAR results[] = { 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0 };
        RtlCopyMemory(DeviceExtension->ScratchContext.ScratchBuffer, results, SIZEOF_ARRAY(results));
        status = STATUS_SUCCESS;
#endif
        if (NT_SUCCESS(status))
        {
            PCDVD_KEY_HEADER        keyHeader = DeviceExtension->ScratchContext.ScratchBuffer;
            PCDVD_REPORT_AGID_DATA  keyData = (PCDVD_REPORT_AGID_DATA)keyHeader->Data;

            *sessionId = (DVD_SESSION_ID)(keyData->AGID);
            *DataLength = sizeof(DVD_SESSION_ID);
        }

        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsEndSession(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:
    This routine is used to process IOCTL:
        IOCTL_AACS_END_SESSION
Arguments:
    DeviceExtension - device context

    Request - the request that will be formatted

    RequestParameters - request parameter structur

    DataLength - data transferred length

Return Value:
    NTSTATUS

  --*/
{
    //AacsReleaseAgid

    NTSTATUS        status = STATUS_SUCCESS;
    PDVD_SESSION_ID sessionId = NULL;

    PAGED_CODE();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           (PVOID*)&sessionId,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        ULONG                       transferSize = 0;
        CDB                         cdb;
        DVD_SESSION_ID              currentSession = 0;
        DVD_SESSION_ID              limitSession = 0;

        if(*sessionId == DVD_END_ALL_SESSIONS)
        {
            currentSession = 0;
            limitSession = MAX_COPY_PROTECT_AGID - 1;
        }
        else
        {
            currentSession = *sessionId;
            limitSession = *sessionId;
        }

        ScratchBuffer_BeginUse(DeviceExtension);

        do
        {
            RtlZeroMemory(&cdb, sizeof(CDB));
            // Set up the CDB
            cdb.SEND_KEY.OperationCode = SCSIOP_SEND_KEY;
            cdb.AsByte[7] = 0x02; // AACS key class
            cdb.SEND_KEY.AGID = (UCHAR)(currentSession);
            cdb.SEND_KEY.KeyFormat = DVD_INVALIDATE_AGID;

            status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, transferSize, FALSE, &cdb, 12);

            currentSession++;
        } while ((currentSession <= limitSession) && NT_SUCCESS(status));

#ifdef ENABLE_AACS_TESTING
        status = STATUS_SUCCESS;
#endif

        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsSendCertificate(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:
    This routine is used to process IOCTL:
        IOCTL_AACS_SEND_CERTIFICATE
Arguments:
    DeviceExtension - device context

    Request - the request that will be formatted

    RequestParameters - request parameter structur

    DataLength - data transferred length

Return Value:
    NTSTATUS

  --*/
{
    //AacsSendHostCertificate

    NTSTATUS                status = STATUS_SUCCESS;
    PAACS_SEND_CERTIFICATE  input = NULL;

    PAGED_CODE();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           (PVOID*)&input,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        ULONG   dataTransferLength = sizeof(CDVD_KEY_HEADER) + sizeof(AACS_CERTIFICATE);
        CDB     cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        // copy the input buffer to the data buffer for the transfer
        {
            PCDVD_KEY_HEADER header = (PCDVD_KEY_HEADER)DeviceExtension->ScratchContext.ScratchBuffer;
            ULONG            tmp = dataTransferLength;

            tmp -= RTL_SIZEOF_THROUGH_FIELD(CDVD_KEY_HEADER, DataLength);

            header->DataLength[0] = (UCHAR)(tmp >> (8*1));
            header->DataLength[1] = (UCHAR)(tmp >> (8*0));
            RtlCopyMemory(header->Data, &(input->Certificate), sizeof(AACS_CERTIFICATE));
        }

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.SEND_KEY.OperationCode = SCSIOP_SEND_KEY;
        cdb.AsByte[7] = 0x02; // AACS key class
        cdb.SEND_KEY.ParameterListLength[0] = (UCHAR)(dataTransferLength >> (8*1));
        cdb.SEND_KEY.ParameterListLength[1] = (UCHAR)(dataTransferLength >> (8*0));
        cdb.SEND_KEY.AGID = (UCHAR)( input->SessionId );
        cdb.SEND_KEY.KeyFormat = 0x01; // Send Host Challenge Certificate

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, dataTransferLength, FALSE, &cdb, 12);

#ifdef ENABLE_AACS_TESTING
        status = STATUS_SUCCESS;
#endif
        if (NT_SUCCESS(status))
        {
            *DataLength = 0;
        }

        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsGetCertificate(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:
    This routine is used to process IOCTL:
        IOCTL_AACS_GET_CERTIFICATE
Arguments:
    DeviceExtension - device context

    Request - the request that will be formatted

    RequestParameters - request parameter structur

    DataLength - data transferred length

Return Value:
    NTSTATUS

  --*/
{
    //AacsGetDriveCertificate

    NTSTATUS        status = STATUS_SUCCESS;
    PDVD_SESSION_ID input = NULL;
    PVOID           outputBuffer = NULL;

    PAGED_CODE();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           (PVOID*)&input,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                (PVOID*)&outputBuffer,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        ULONG   dataTransferLength = sizeof(CDVD_KEY_HEADER) + sizeof(AACS_CERTIFICATE);
        CDB     cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.REPORT_KEY.OperationCode = SCSIOP_REPORT_KEY;
        cdb.AsByte[7] = 0x02; // AACS key class
        cdb.REPORT_KEY.AllocationLength[0] = (UCHAR)(dataTransferLength >> (8*1));
        cdb.REPORT_KEY.AllocationLength[1] = (UCHAR)(dataTransferLength >> (8*0));
        cdb.REPORT_KEY.AGID = (UCHAR)(*input);
        cdb.REPORT_KEY.KeyFormat = 0x01; // Return a drive certificate challenge

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, dataTransferLength, TRUE, &cdb, 12);

#ifdef ENABLE_AACS_TESTING
            static const UCHAR results[] = { 0x00, 0x72, 0x00, 0x00 };
            static const UCHAR defaultFill = 0x31; // '1'
            RtlFillMemory(DeviceExtension->ScratchContext.ScratchBuffer, 0x0074, defaultFill);
            RtlCopyMemory(DeviceExtension->ScratchContext.ScratchBuffer, results, SIZEOF_ARRAY(results));
            status = STATUS_SUCCESS;
#endif
        if (NT_SUCCESS(status))
        {
            PDVD_DESCRIPTOR_HEADER  header = DeviceExtension->ScratchContext.ScratchBuffer;
            ULONG                   dataLengthToCopy = sizeof(AACS_CERTIFICATE);

            // make length field native byte ordering
            REVERSE_SHORT(&header->Length);

            // exit if getting invalid data from the drive
            if (header->Length < (sizeof(DVD_DESCRIPTOR_HEADER) - RTL_SIZEOF_THROUGH_FIELD(DVD_DESCRIPTOR_HEADER, Length)))
            {
                *DataLength = 0;
                status = STATUS_IO_DEVICE_ERROR;
            }

            if (NT_SUCCESS(status))
            {
                // adjust data length to reflect only the addition data
                header->Length -= sizeof(DVD_DESCRIPTOR_HEADER) - RTL_SIZEOF_THROUGH_FIELD(DVD_DESCRIPTOR_HEADER, Length);

                // exit if the drive is returning an unexpected data size
                if (header->Length != dataLengthToCopy)
                {
                    *DataLength = 0;
                    status = STATUS_IO_DEVICE_ERROR;
                }
            }

            if (NT_SUCCESS(status))
            {
                // else copy the data to the user's buffer
                RtlCopyMemory(outputBuffer, header->Data, dataLengthToCopy);
                *DataLength = dataLengthToCopy;
            }
        }

        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsGetChallengeKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:
    This routine is used to process IOCTL:
        IOCTL_AACS_GET_CHALLENGE_KEY
Arguments:
    DeviceExtension - device context

    Request - the request that will be formatted

    RequestParameters - request parameter structur

    DataLength - data transferred length

Return Value:
    NTSTATUS

  --*/
{
    //AacsGetChallengeKey

    NTSTATUS        status = STATUS_SUCCESS;
    PDVD_SESSION_ID input = NULL;
    PVOID           outputBuffer = NULL;

    PAGED_CODE();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           (PVOID*)&input,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                (PVOID*)&outputBuffer,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        ULONG   dataTransferLength = sizeof(CDVD_KEY_HEADER) + sizeof(AACS_CHALLENGE_KEY);
        CDB     cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.REPORT_KEY.OperationCode = SCSIOP_REPORT_KEY;
        cdb.AsByte[7] = 0x02; // AACS key class
        cdb.REPORT_KEY.AllocationLength[0] = (UCHAR)(dataTransferLength >> (8*1));
        cdb.REPORT_KEY.AllocationLength[1] = (UCHAR)(dataTransferLength >> (8*0));
        cdb.REPORT_KEY.AGID = (UCHAR)(*input);
        cdb.REPORT_KEY.KeyFormat = 0x02; // Return a drive certificate challenge

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, dataTransferLength, TRUE, &cdb, 12);

#ifdef ENABLE_AACS_TESTING
            static const UCHAR results[] = { 0x00, 0x52, 0x00, 0x00 };
            static const UCHAR defaultFill = 0x32; // '2'
            RtlFillMemory(DeviceExtension->ScratchContext.ScratchBuffer, 0x0054, defaultFill);
            RtlCopyMemory(DeviceExtension->ScratchContext.ScratchBuffer, results, SIZEOF_ARRAY(results));
            status = STATUS_SUCCESS;
#endif
        if (NT_SUCCESS(status))
        {
            PDVD_DESCRIPTOR_HEADER  header = DeviceExtension->ScratchContext.ScratchBuffer;
            ULONG                   dataLengthToCopy = sizeof(AACS_CHALLENGE_KEY);

            // make length field native byte ordering
            REVERSE_SHORT(&header->Length);

            // exit if getting invalid data from the drive
            if (header->Length < sizeof(DVD_DESCRIPTOR_HEADER) - RTL_SIZEOF_THROUGH_FIELD(DVD_DESCRIPTOR_HEADER, Length))
            {
                *DataLength = 0;
                status = STATUS_IO_DEVICE_ERROR;
            }

            if (NT_SUCCESS(status))
            {
                // adjust data length to reflect only the addition data
                header->Length -= sizeof(DVD_DESCRIPTOR_HEADER) - RTL_SIZEOF_THROUGH_FIELD(DVD_DESCRIPTOR_HEADER, Length);

                // exit if the drive is returning an unexpected data size
                if (header->Length != dataLengthToCopy)
                {
                    *DataLength = 0;
                    status = STATUS_IO_DEVICE_ERROR;
                }
            }

            if (NT_SUCCESS(status))
            {
                // else copy the data to the user's buffer
                RtlCopyMemory(outputBuffer, header->Data, dataLengthToCopy);
                *DataLength = dataLengthToCopy;
            }
        }

        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleSendChallengeKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:
    This routine is used to process IOCTL:
        IOCTL_AACS_SEND_CHALLENGE_KEY
Arguments:
    DeviceExtension - device context

    Request - the request that will be formatted

    RequestParameters - request parameter structur

    DataLength - data transferred length

Return Value:
    NTSTATUS

  --*/
{
    //AacsSendChallengeKey

    NTSTATUS                 status = STATUS_SUCCESS;
    PAACS_SEND_CHALLENGE_KEY input = NULL;

    PAGED_CODE();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           (PVOID*)&input,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        ULONG   dataTransferLength = sizeof(CDVD_KEY_HEADER) + sizeof(AACS_CHALLENGE_KEY);
        CDB     cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        // copy the input buffer to the data buffer for the transfer
        {
            PCDVD_KEY_HEADER header = DeviceExtension->ScratchContext.ScratchBuffer;
            ULONG tmp = dataTransferLength;
            tmp -= RTL_SIZEOF_THROUGH_FIELD(CDVD_KEY_HEADER, DataLength);

            header->DataLength[0] = (UCHAR)(tmp >> (8*1));
            header->DataLength[1] = (UCHAR)(tmp >> (8*0));
            RtlCopyMemory(header->Data, &(input->ChallengeKey), sizeof(AACS_CHALLENGE_KEY));
        }

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.SEND_KEY.OperationCode = SCSIOP_SEND_KEY;
        cdb.AsByte[7] = 0x02; // AACS key class
        cdb.SEND_KEY.ParameterListLength[0] = (UCHAR)(dataTransferLength >> (8*1));
        cdb.SEND_KEY.ParameterListLength[1] = (UCHAR)(dataTransferLength >> (8*0));
        cdb.SEND_KEY.AGID = (UCHAR)( input->SessionId );
        cdb.SEND_KEY.KeyFormat = 0x02; // Send Host Challenge Certificate

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, dataTransferLength, FALSE, &cdb, 12);

#ifdef ENABLE_AACS_TESTING
            status = STATUS_SUCCESS;
#endif
        if (NT_SUCCESS(status))
        {
            *DataLength = 0;
        }

        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleReadVolumeId(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:
    This routine is used to process IOCTL:
        IOCTL_AACS_READ_VOLUME_ID
Arguments:
    DeviceExtension - device context

    Request - the request that will be formatted

    RequestParameters - request parameter structur

    DataLength - data transferred length

Return Value:
    NTSTATUS

  --*/
{
    //AacsReadVolumeID

    NTSTATUS        status = STATUS_SUCCESS;
    PDVD_SESSION_ID input = NULL;
    PVOID           outputBuffer = NULL;

    PAGED_CODE();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           (PVOID*)&input,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                (PVOID*)&outputBuffer,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        ULONG   dataTransferLength = sizeof(CDVD_KEY_HEADER) + sizeof(AACS_VOLUME_ID);
        CDB     cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.READ_DVD_STRUCTURE.OperationCode = SCSIOP_READ_DVD_STRUCTURE;
        cdb.READ_DVD_STRUCTURE.Format = 0x80; // Return the AACS volumeID
        cdb.READ_DVD_STRUCTURE.AllocationLength[0] = (UCHAR)(dataTransferLength >> (8*1));
        cdb.READ_DVD_STRUCTURE.AllocationLength[1] = (UCHAR)(dataTransferLength >> (8*0));
        cdb.READ_DVD_STRUCTURE.AGID = (UCHAR)(*input);

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, dataTransferLength, TRUE, &cdb, 12);

#ifdef ENABLE_AACS_TESTING
            static const UCHAR results[] = { 0x00, 0x22, 0x00, 0x00 };
            static const UCHAR defaultFill = 0x33; // '3'
            RtlFillMemory(DeviceExtension->ScratchContext.ScratchBuffer, 0x0024, defaultFill);
            RtlCopyMemory(DeviceExtension->ScratchContext.ScratchBuffer, results, SIZEOF_ARRAY(results));
            status = STATUS_SUCCESS;
#endif
        if (NT_SUCCESS(status))
        {
            PDVD_DESCRIPTOR_HEADER  header = DeviceExtension->ScratchContext.ScratchBuffer;
            ULONG                   dataLengthToCopy = sizeof(AACS_VOLUME_ID);

            // make length field native byte ordering
            REVERSE_SHORT(&header->Length);

            // exit if getting invalid data from the drive
            if (header->Length < sizeof(DVD_DESCRIPTOR_HEADER) - RTL_SIZEOF_THROUGH_FIELD(DVD_DESCRIPTOR_HEADER, Length))
            {
                *DataLength = 0;
                status = STATUS_IO_DEVICE_ERROR;
            }

            if (NT_SUCCESS(status))
            {
                // adjust data length to reflect only the addition data
                header->Length -= sizeof(DVD_DESCRIPTOR_HEADER) - RTL_SIZEOF_THROUGH_FIELD(DVD_DESCRIPTOR_HEADER, Length);

                // exit if the drive is returning an unexpected data size
                if (header->Length != dataLengthToCopy)
                {
                    *DataLength = 0;
                    status = STATUS_IO_DEVICE_ERROR;
                }
            }

            if (NT_SUCCESS(status))
            {
                // else copy the data to the user's buffer
                RtlCopyMemory(outputBuffer, header->Data, dataLengthToCopy);
                *DataLength = dataLengthToCopy;
            }
        }

        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsReadSerialNumber(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:
    This routine is used to process IOCTL:
        IOCTL_AACS_READ_SERIAL_NUMBER
Arguments:
    DeviceExtension - device context

    Request - the request that will be formatted

    RequestParameters - request parameter structur

    DataLength - data transferred length

Return Value:
    NTSTATUS

  --*/
{
    //AacsReadSerialNumber

    NTSTATUS        status = STATUS_SUCCESS;
    PDVD_SESSION_ID input = NULL;
    PVOID           outputBuffer = NULL;

    PAGED_CODE();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           (PVOID*)&input,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                (PVOID*)&outputBuffer,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        ULONG   dataTransferLength = sizeof(CDVD_KEY_HEADER) + sizeof(AACS_SERIAL_NUMBER);
        CDB     cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.READ_DVD_STRUCTURE.OperationCode = SCSIOP_READ_DVD_STRUCTURE;
        cdb.READ_DVD_STRUCTURE.Format = 0x81; // Return the AACS volumeID
        cdb.READ_DVD_STRUCTURE.AllocationLength[0] = (UCHAR)(dataTransferLength >> (8*1));
        cdb.READ_DVD_STRUCTURE.AllocationLength[1] = (UCHAR)(dataTransferLength >> (8*0));
        cdb.READ_DVD_STRUCTURE.AGID = (UCHAR)(*input);

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, dataTransferLength, TRUE, &cdb, 12);

#ifdef ENABLE_AACS_TESTING
            static const UCHAR results[] = { 0x00, 0x22, 0x00, 0x00 };
            static const UCHAR defaultFill = 0x34; // '4'
            RtlFillMemory(DeviceExtension->ScratchContext.ScratchBuffer, 0x0024, defaultFill);
            RtlCopyMemory(DeviceExtension->ScratchContext.ScratchBuffer, results, SIZEOF_ARRAY(results));
            status = STATUS_SUCCESS;
#endif
        if (NT_SUCCESS(status))
        {
            PDVD_DESCRIPTOR_HEADER  header = DeviceExtension->ScratchContext.ScratchBuffer;
            ULONG                   dataLengthToCopy = sizeof(AACS_SERIAL_NUMBER);

            // make length field native byte ordering
            REVERSE_SHORT(&header->Length);

            // exit if getting invalid data from the drive
            if (header->Length < sizeof(DVD_DESCRIPTOR_HEADER) - RTL_SIZEOF_THROUGH_FIELD(DVD_DESCRIPTOR_HEADER, Length))
            {
                *DataLength = 0;
                status = STATUS_IO_DEVICE_ERROR;
            }

            if (NT_SUCCESS(status))
            {
                // adjust data length to reflect only the addition data
                header->Length -= sizeof(DVD_DESCRIPTOR_HEADER) - RTL_SIZEOF_THROUGH_FIELD(DVD_DESCRIPTOR_HEADER, Length);

                // exit if the drive is returning an unexpected data size
                if (header->Length != dataLengthToCopy)
                {
                    *DataLength = 0;
                    status = STATUS_IO_DEVICE_ERROR;
                }
            }

            if (NT_SUCCESS(status))
            {
                // else copy the data to the user's buffer
                RtlCopyMemory(outputBuffer, header->Data, dataLengthToCopy);
                *DataLength = dataLengthToCopy;
            }
        }

        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsReadMediaId(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:
    This routine is used to process IOCTL:
        IOCTL_AACS_READ_MEDIA_ID
Arguments:
    DeviceExtension - device context

    Request - the request that will be formatted

    RequestParameters - request parameter structur

    DataLength - data transferred length

Return Value:
    NTSTATUS

  --*/
{
    //AacsReadMediaID

    NTSTATUS        status = STATUS_SUCCESS;
    PDVD_SESSION_ID input = NULL;
    PVOID           outputBuffer = NULL;

    PAGED_CODE();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           (PVOID*)&input,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                (PVOID*)&outputBuffer,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        ULONG   dataTransferLength = sizeof(CDVD_KEY_HEADER) + sizeof(AACS_MEDIA_ID);
        CDB     cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.READ_DVD_STRUCTURE.OperationCode = SCSIOP_READ_DVD_STRUCTURE;
        cdb.READ_DVD_STRUCTURE.Format = 0x82; // Return the AACS volumeID
        cdb.READ_DVD_STRUCTURE.AllocationLength[0] = (UCHAR)(dataTransferLength >> (8*1));
        cdb.READ_DVD_STRUCTURE.AllocationLength[1] = (UCHAR)(dataTransferLength >> (8*0));
        cdb.READ_DVD_STRUCTURE.AGID = (UCHAR)(*input);

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, dataTransferLength, TRUE, &cdb, 12);

#ifdef ENABLE_AACS_TESTING
            static const UCHAR results[] = { 0x00, 0x22, 0x00, 0x00 };
            static const UCHAR defaultFill = 0x35; // '5'
            RtlFillMemory(DeviceExtension->ScratchContext.ScratchBuffer, 0x0024, defaultFill);
            RtlCopyMemory(DeviceExtension->ScratchContext.ScratchBuffer, results, SIZEOF_ARRAY(results));
            status = STATUS_SUCCESS;
#endif
        if (NT_SUCCESS(status))
        {
            PDVD_DESCRIPTOR_HEADER  header = DeviceExtension->ScratchContext.ScratchBuffer;
            ULONG                   dataLengthToCopy = sizeof(AACS_MEDIA_ID);

            // make length field native byte ordering
            REVERSE_SHORT(&header->Length);

            // exit if getting invalid data from the drive
            if (header->Length < sizeof(DVD_DESCRIPTOR_HEADER) - RTL_SIZEOF_THROUGH_FIELD(DVD_DESCRIPTOR_HEADER, Length))
            {
                *DataLength = 0;
                status = STATUS_IO_DEVICE_ERROR;
            }

            if (NT_SUCCESS(status))
            {
                // adjust data length to reflect only the addition data
                header->Length -= sizeof(DVD_DESCRIPTOR_HEADER) - RTL_SIZEOF_THROUGH_FIELD(DVD_DESCRIPTOR_HEADER, Length);

                // exit if the drive is returning an unexpected data size
                if (header->Length != dataLengthToCopy)
                {
                    *DataLength = 0;
                    status = STATUS_IO_DEVICE_ERROR;
                }
            }

            if (NT_SUCCESS(status))
            {
                // else copy the data to the user's buffer
                RtlCopyMemory(outputBuffer, header->Data, dataLengthToCopy);
                *DataLength = dataLengthToCopy;
            }
        }

        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsReadBindingNonce(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:
    This routine is used to process IOCTL:
        IOCTL_AACS_READ_BINDING_NONCE
Arguments:
    DeviceExtension - device context

    Request - the request that will be formatted

    RequestParameters - request parameter structur

    DataLength - data transferred length

Return Value:
    NTSTATUS

  --*/
{
    //AacsReadBindingNonce

    NTSTATUS                 status = STATUS_SUCCESS;
    PAACS_READ_BINDING_NONCE input = NULL;
    PVOID                    outputBuffer = NULL;

    PAGED_CODE();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           (PVOID*)&input,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                (PVOID*)&outputBuffer,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        ULONG   dataTransferLength = sizeof(CDVD_KEY_HEADER) + sizeof(AACS_BINDING_NONCE);
        CDB     cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.REPORT_KEY.OperationCode = SCSIOP_REPORT_KEY;
        cdb.REPORT_KEY.LogicalBlockAddress[0] = (UCHAR)( input->StartLba >> (3*8) );
        cdb.REPORT_KEY.LogicalBlockAddress[1] = (UCHAR)( input->StartLba >> (2*8) );
        cdb.REPORT_KEY.LogicalBlockAddress[2] = (UCHAR)( input->StartLba >> (1*8) );
        cdb.REPORT_KEY.LogicalBlockAddress[3] = (UCHAR)( input->StartLba >> (0*8) );
        cdb.AsByte[6] = (UCHAR)( input->NumberOfSectors );
        cdb.AsByte[7] = 0x02; // AACS key class
        cdb.REPORT_KEY.AllocationLength[0] = (UCHAR)(dataTransferLength >> (8*1));
        cdb.REPORT_KEY.AllocationLength[1] = (UCHAR)(dataTransferLength >> (8*0));
        cdb.REPORT_KEY.AGID = (UCHAR)( input->SessionId );
        cdb.REPORT_KEY.KeyFormat = 0x21; // Return an existing binding nonce

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, dataTransferLength, TRUE, &cdb, 12);

#ifdef ENABLE_AACS_TESTING
            static const UCHAR results[] = { 0x00, 0x22, 0x00, 0x00 };
            static const UCHAR defaultFill = 0x36; // '6'
            RtlFillMemory(DeviceExtension->ScratchContext.ScratchBuffer, 0x0024, defaultFill);
            RtlCopyMemory(DeviceExtension->ScratchContext.ScratchBuffer, results, SIZEOF_ARRAY(results));
            status = STATUS_SUCCESS;
#endif
        if (NT_SUCCESS(status))
        {
            PDVD_DESCRIPTOR_HEADER  header = DeviceExtension->ScratchContext.ScratchBuffer;
            ULONG                   dataLengthToCopy = sizeof(AACS_BINDING_NONCE);

            // make length field native byte ordering
            REVERSE_SHORT(&header->Length);

            // exit if getting invalid data from the drive
            if (header->Length < sizeof(DVD_DESCRIPTOR_HEADER) - RTL_SIZEOF_THROUGH_FIELD(DVD_DESCRIPTOR_HEADER, Length))
            {
                *DataLength = 0;
                status = STATUS_IO_DEVICE_ERROR;
            }

            if (NT_SUCCESS(status))
            {
                // adjust data length to reflect only the addition data
                header->Length -= sizeof(DVD_DESCRIPTOR_HEADER) - RTL_SIZEOF_THROUGH_FIELD(DVD_DESCRIPTOR_HEADER, Length);

                // exit if the drive is returning an unexpected data size
                if (header->Length != dataLengthToCopy)
                {
                    *DataLength = 0;
                    status = STATUS_IO_DEVICE_ERROR;
                }
            }

            if (NT_SUCCESS(status))
            {
                // else copy the data to the user's buffer
                RtlCopyMemory(outputBuffer, header->Data, dataLengthToCopy);
                *DataLength = dataLengthToCopy;
            }
        }

        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsGenerateBindingNonce(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:
    This routine is used to process IOCTL:
        IOCTL_AACS_GENERATE_BINDING_NONCE
Arguments:
    DeviceExtension - device context

    Request - the request that will be formatted

    RequestParameters - request parameter structur

    DataLength - data transferred length

Return Value:
    NTSTATUS

  --*/
{
    //AacsGenerateBindingNonce

    NTSTATUS                 status = STATUS_SUCCESS;
    PAACS_READ_BINDING_NONCE input = NULL;
    PVOID                    outputBuffer = NULL;

    PAGED_CODE();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           (PVOID*)&input,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                (PVOID*)&outputBuffer,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        ULONG   dataTransferLength = sizeof(CDVD_KEY_HEADER) + sizeof(AACS_BINDING_NONCE);
        CDB     cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, dataTransferLength, TRUE, &cdb, 12);

#ifdef ENABLE_AACS_TESTING
            static const UCHAR results[] = { 0x00, 0x22, 0x00, 0x00 };
            static const UCHAR defaultFill = 0x37; // '7'
            RtlFillMemory(DeviceExtension->ScratchContext.ScratchBuffer, 0x0024, defaultFill);
            RtlCopyMemory(DeviceExtension->ScratchContext.ScratchBuffer, results, SIZEOF_ARRAY(results));
            status = STATUS_SUCCESS;
#endif
        if (NT_SUCCESS(status))
        {
            PDVD_DESCRIPTOR_HEADER  header = DeviceExtension->ScratchContext.ScratchBuffer;
            ULONG                   dataLengthToCopy = sizeof(AACS_BINDING_NONCE);

            // make length field native byte ordering
            REVERSE_SHORT(&header->Length);

            // exit if getting invalid data from the drive
            if (header->Length < sizeof(DVD_DESCRIPTOR_HEADER) - RTL_SIZEOF_THROUGH_FIELD(DVD_DESCRIPTOR_HEADER, Length))
            {
                *DataLength = 0;
                status = STATUS_IO_DEVICE_ERROR;
            }

            if (NT_SUCCESS(status))
            {
                // adjust data length to reflect only the addition data
                header->Length -= sizeof(DVD_DESCRIPTOR_HEADER) - RTL_SIZEOF_THROUGH_FIELD(DVD_DESCRIPTOR_HEADER, Length);

                // exit if the drive is returning an unexpected data size
                if (header->Length != dataLengthToCopy)
                {
                    *DataLength = 0;
                    status = STATUS_IO_DEVICE_ERROR;
                }
            }

            if (NT_SUCCESS(status))
            {
                // else copy the data to the user's buffer
                RtlCopyMemory(outputBuffer, header->Data, dataLengthToCopy);
                *DataLength = dataLengthToCopy;
            }
        }

        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

