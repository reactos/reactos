/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tapeapi.c

Abstract:

    This module implements Win32 Tape APIs

Author:

    Steve Wood (stevewo) 26-Mar-1992
    Lori Brown (Maynard)

Revision History:

--*/

#include "basedll.h"
#pragma hdrstop

#include <ntddtape.h>

DWORD
BasepDoTapeOperation(
    IN HANDLE TapeDevice,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    );

DWORD
BasepDoTapeOperation(
    IN HANDLE TapeDevice,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    PIO_STATUS_BLOCK IoStatusBlock;

    IoStatusBlock = &IoStatus;

    Status = NtDeviceIoControlFile( TapeDevice,
                                    NULL,
                                    NULL,
                                    NULL,
                                    IoStatusBlock,
                                    IoControlCode,
                                    InputBuffer,
                                    InputBufferLength,
                                    OutputBuffer,
                                    OutputBufferLength
                                  );

    if (!NT_SUCCESS( Status )) {
        return BaseSetLastNTError( Status );
        }
    else {
        return NO_ERROR;
        }
}


DWORD
WINAPI
SetTapePosition(
    HANDLE hDevice,
    DWORD dwPositionMethod,
    DWORD dwPartition,
    DWORD dwOffsetLow,
    DWORD dwOffsetHigh,
    BOOL bImmediate
    )

/*++

Routine Description:

    This API is used to set the tape position.

Arguments:

    hDevice - Handle to the device on which to set the tape position.

    dwPositionMethod - Type of positioning to perform.
        This parameter can have one of the following values:

        TAPE_REWIND - Position the tape to beginning-of-tape or to
            beginning-of-partition if a multiple partition mode is in
            effect (ref: CreateTapePartition API). The parameters
            dwPartition, dwOffsetHigh, and dwOffsetLow are ignored.

        TAPE_ABSOLUTE_BLOCK - Position the tape to the device specific
            block address specified by dwOffsetHigh/dwOffsetLow. The
            dwPartition parameter is ignored.

        TAPE_LOGICAL_BLOCK - Position the tape to the logical block address
            specified by dwOffsetHigh/dwOffsetLow. If a multiple partition
            mode is in effect (ref: CreateTapePartition API), then the tape
            is positioned to the specified logical block address in the
            partition specified by dwPartition; otherwise, the dwPartition
            parameter value must be 0.

        TAPE_SPACE_END_OF_DATA - Position the tape to the end-of-data
            on tape or to the end-of-data in the current partition if a
            multiple partition mode is in effect (ref: CreateTapePartition
            API). The parameters dwPartition, dwOffsetHigh, and dwOffsetLow
            are ignored.

        TAPE_SPACE_RELATIVE_BLOCKS - Position forward or reverse the number
            of blocks specified by dwOffsetHigh/dwOffsetLow. The dwPartition
            parameter is ignored.

        TAPE_SPACE_FILEMARKS - Position forward or reverse the number of
            filemarks specified by dwOffsetHigh/dwOffsetLow. The dwPartition
            parameter is ignored.

        TAPE_SPACE_SEQUENTIAL_FMKS - Position forward or reverse to the
            next occurrence, if any, of the consecutive number of filemarks
            specified by dwOffsetHigh/dwOffsetLow. The dwPartition parameter
            is ignored.

        TAPE_SPACE_SETMARKS - Position forward or reverse the number of
            setmarks specified by dwOffsetHigh/dwOffsetLow. The dwPartition
            parameter is ignored.

        TAPE_SPACE_SEQUENTIAL_SMKS - Position forward or reverse to the
            next occurrence, if any, of the consecutive number of setmarks
            specified by dwOffsetHigh/dwOffsetLow. The dwPartition parameter
            is ignored.

        Note that a drive/tape may not support all dwPositionMethod values:
        an "unsupported" error indication is returned if the dwPositionMethod
        is one that is not flagged as supported in the drive's features bits
        (ref: GetTapeParameters API).

    dwPartition - The partition number for the position operation specified
        by dwPositionMethod (if not ignored).

        A partition number value of 0 selects the current partition for
        the position operation.

        Partitions are numbered logically from 1 to N: the first partition
        of the tape is partition number 1, the next is partition number 2,
        etc. However, a partition number does not imply a physical/linear
        position on tape -- partition number 1 on tape may not be at BOT.

        This parameter must be set to 0 if a multiple partition mode is not
        in effect (ref: CreateTapePartition API).

    dwOffsetHigh/dwOffsetLow - The block address or count for the position
        operation specified by dwPositionMethod.

        When the offset specifies the number of blocks, filemarks, or
        setmarks to position over, a positive value N in the offset shall
        cause forward positioning over N blocks, filemarks, or setmarks,
        ending on the end-of-partition/tape side of a block, filemark, or
        setmark. A zero value in the offset shall cause no change of
        position. A negative value N in the offset shall cause reverse
        positioning (toward beginning-of-partition/tape) over N blocks,
        filemarks, or setmarks, ending on the beginning-of-partition side
        of a block, filemark, or setmark.

    bImmediate - Return immediately without waiting for the operation to
        complete.

        Note that a drive/tape may not support the bImmediate option for
        either some or all dwPositionMethod values: an "unsupported" error
        indication is returned if the bImmediate dwPositionMethod is one
        that is not flagged as supported in the drive's features bits
        (ref: GetTapeParameters API).


Return Value:

    If the function is successful, the return value is NO_ERROR. Otherwise,
    it is a Win32 API error code.

--*/

{
    TAPE_SET_POSITION TapeSetPosition;

    TapeSetPosition.Method = dwPositionMethod;
    TapeSetPosition.Partition = dwPartition;
    TapeSetPosition.Offset.LowPart = dwOffsetLow;
    TapeSetPosition.Offset.HighPart = dwOffsetHigh;
    TapeSetPosition.Immediate = (BOOLEAN)bImmediate;

    return BasepDoTapeOperation( hDevice,
                                 IOCTL_TAPE_SET_POSITION,
                                 &TapeSetPosition,
                                 sizeof( TapeSetPosition ),
                                 NULL,
                                 0
                               );
}


DWORD
WINAPI
GetTapePosition(
    HANDLE hDevice,
    DWORD dwPositionType,
    LPDWORD lpdwPartition,
    LPDWORD lpdwOffsetLow,
    LPDWORD lpdwOffsetHigh
    )

/*++

Routine Description:

    This API is used to get the tape position.

Arguments:

    hDevice - Handle to the device on which to get the tape position.

    dwPositionType - Type of position to return.
        This parameter can have one of the following values:

        TAPE_ABSOLUTE_POSITION - Return a device specific block address to
            the LARGE_INTEGER pointed to by lpliOffset.

            The DWORD pointed to by the lpdwPartition parameter is set to 0.

        TAPE_LOGICAL_POSITION - Return a logical block address to the
            LARGE_INTEGER pointed to by lpliOffset.

            The DWORD pointed to by the lpdwPartition parameter is set to 0
            if a multiple partition mode is not in effect; otherwise, it is
            set to the partition number of the currently selected partition
            (ref: CreateTapePartition API).

            Logical block addresses are 0 based -- 0 is a valid logical
            block address. A logical block address is a relative reference
            point (ref: logical positioning whitepaper).

    lpdwPartition - Pointer to a DWORD that receives the appropriate return
            value for the dwPositionType values explained above.

    lpliOffset - Pointer to a LARGE_INTEGER that receives the appropriate
            return value for the dwPositionType values explained above.

Return Value:

    If the function is successful, the return value is NO_ERROR. Otherwise,
    it is a Win32 API error code.

--*/

{
    TAPE_GET_POSITION TapeGetPosition;
    DWORD rc;

    TapeGetPosition.Type = dwPositionType;

    rc = BasepDoTapeOperation( hDevice,
                               IOCTL_TAPE_GET_POSITION,
                               &TapeGetPosition,
                               sizeof( TapeGetPosition ),
                               &TapeGetPosition,
                               sizeof( TapeGetPosition )
                             );

    if (rc == NO_ERROR) {
        *lpdwPartition = TapeGetPosition.Partition;
        *lpdwOffsetLow = TapeGetPosition.Offset.LowPart;
        *lpdwOffsetHigh = TapeGetPosition.Offset.HighPart;
    }
    else {
        *lpdwPartition = 0;
        *lpdwOffsetLow = 0;
        *lpdwOffsetHigh = 0;
    }

    return rc;
}


DWORD
WINAPI
PrepareTape(
    HANDLE hDevice,
    DWORD dwOperation,
    BOOL bImmediate
    )

/*++

Routine Description:

    This API is used to prepare the tape.

Arguments:

    hDevice - Handle to the device on which to prepare the tape.

    dwOperation - Type of tape preparation to perform.
        This parameter can have one of the following values:

        TAPE_LOAD - Load the tape and position the tape to beginning-of-medium.

        TAPE_UNLOAD - Position the tape to beginning-of-medium for removal from
            the device.

            Following a successful unload operation, the device shall return an
            error for all subsequent medium-access commands until a load
            operation is successfully completed.

        TAPE_TENSION - Tension the tape in the device as required.  The
            implementation of this operation is device specific.

        TAPE_LOCK - Disable the removal of the tape from the device.

        TAPE_UNLOCK - Enable the removal of the tape from the device.

        TAPE_FORMAT - Format media in tape device.

    bImmediate - Return immediately without waiting for operation to complete.

Return Value:

    If the function is successful, the return value is NO_ERROR. Otherwise,
    it is a Win32 API error code.

--*/

{
    TAPE_PREPARE TapePrepare;

    TapePrepare.Operation = dwOperation;
    TapePrepare.Immediate = (BOOLEAN)bImmediate;

    return BasepDoTapeOperation( hDevice,
                                 IOCTL_TAPE_PREPARE,
                                 &TapePrepare,
                                 sizeof( TapePrepare ),
                                 NULL,
                                 0
                               );
}


DWORD
WINAPI
EraseTape(
    HANDLE hDevice,
    DWORD dwEraseType,
    BOOL bImmediate
    )

/*++

Routine Description:

    This API is used to erase the tape partition.

Arguments:

    hDevice - Handle to the device on which to erase the tape partition.

    dwEraseType - Type of erase to perform.
        This parameter can have one of the following values:

        TAPE_ERASE_SHORT - Write an erase gap or end-of-recorded data marker
            beginning at the current position.

        TAPE_ERASE_LONG - Erase all remaining media in the current partition
            beginning at the current position.

    bImmediate - Return immediately without waiting for operation to complete.

Return Value:

    If the function is successful, the return value is NO_ERROR. Otherwise,
    it is a Win32 API error code.

--*/

{
    TAPE_ERASE TapeErase;

    TapeErase.Type = dwEraseType;
    TapeErase.Immediate = (BOOLEAN)bImmediate;

    return BasepDoTapeOperation( hDevice,
                                 IOCTL_TAPE_ERASE,
                                 &TapeErase,
                                 sizeof( TapeErase ),
                                 NULL,
                                 0
                               );
}


DWORD
WINAPI
CreateTapePartition(
    HANDLE hDevice,
    DWORD dwPartitionMethod,
    DWORD dwCount,
    DWORD dwSize
    )

/*++

Routine Description:

    This API is used to create partitions.

Arguments:

    hDevice - Handle to the device on which to create partitions.

    dwPartitionMethod - Type of partitioning to perform.

        Creating partitions causes the tape to be reformatted.  All previous
        information recorded on the tape is destroyed.

        This parameter can have one of the following values:

        TAPE_FIXED_PARTITIONS - Partition the tape based on the device's fixed
            definition of partitions.  The dwCount and dwSize parameters are
            ignored.

        TAPE_SELECT_PARTITIONS - Partition the tape into the number of
            partitions specified by dwCount using the partition sizes defined
            by the device.  The dwSize parameter is ignored.

        TAPE_INITIATOR_PARTITIONS - Partition the tape into the number of
            partitions specified by dwCount using the partition size specified
            by dwSize for all but the last partition.  The size of the last
            partition is the remainder of the tape.

    dwCount - Number of partitions to create.  The maximum number of partitions
        a device can create is returned by GetTapeParameters.

    dwSize - Partition size in megabytes.  The maximum capacity of a tape is
        returned by GetTapeParameters.

Return Value:

    If the function is successful, the return value is NO_ERROR. Otherwise,
    it is a Win32 API error code.

--*/

{
    TAPE_CREATE_PARTITION TapeCreatePartition;

    TapeCreatePartition.Method = dwPartitionMethod;
    TapeCreatePartition.Count = dwCount;
    TapeCreatePartition.Size = dwSize;

    return BasepDoTapeOperation( hDevice,
                                 IOCTL_TAPE_CREATE_PARTITION,
                                 &TapeCreatePartition,
                                 sizeof( TapeCreatePartition ),
                                 NULL,
                                 0
                               );
}


DWORD
WINAPI
WriteTapemark(
    HANDLE hDevice,
    DWORD dwTapemarkType,
    DWORD dwTapemarkCount,
    BOOL bImmediate
    )

/*++

Routine Description:

    This API is used to write tapemarks.

Arguments:

    hDevice - Handle to the device on which to write the tapemarks.

    dwTapemarkType - Type of tapemarks to write.
        This parameter can have one of the following values:

        TAPE_SETMARKS - Write the number of setmarks specified by
            dwTapemarkCount to the tape.

            A setmark is a special recorded element containing no user data.
            A setmark provides a segmentation scheme hierarchically superior
            to filemarks.

        TAPE_FILEMARKS - Write the number of filemarks specified by
            dwTapemarkCount to the tape.

            A filemark is a special recorded element containing no user data.

        TAPE_SHORT_FILEMARKS - Write the number of short filemarks specified by
            dwTapemarkCount to the tape.

            A short filemark contains a short erase gap that does not allow a
            write operation to be performed.  The short filemark cannot be
            overwritten except when the write operation is performed from the
            beginning-of-partition or from a previous long filemark.

        TAPE_LONG_FILEMARKS - Write the number of long filemarks specified by
            dwTapemarkCount to the tape.

            A long filemark includes a long erase gap.  This gap allows the
            initiator to position on the beginning-of-partition side of the
            filemark, in the erase gap, and append data with the write
            operation.  This causes the long filemark and any data following
            the long filemark to be erased.

    dwTapemarkCount - The number of tapemarks to write.

    bImmediate - Return immediately without waiting for operation to complete.

Return Value:

    If the function is successful, the return value is NO_ERROR. Otherwise,
    it is a Win32 API error code.

--*/

{
    TAPE_WRITE_MARKS TapeWriteMarks;

    TapeWriteMarks.Type = dwTapemarkType;
    TapeWriteMarks.Count = dwTapemarkCount;
    TapeWriteMarks.Immediate = (BOOLEAN)bImmediate;

    return BasepDoTapeOperation( hDevice,
                                 IOCTL_TAPE_WRITE_MARKS,
                                 &TapeWriteMarks,
                                 sizeof( TapeWriteMarks ),
                                 NULL,
                                 0
                               );
}


DWORD
WINAPI
GetTapeParameters(
    HANDLE hDevice,
    DWORD dwOperation,
    LPDWORD lpdwSize,
    LPVOID lpTapeInformation
    )

/*++

Routine Description:

    This API is used to get information about a tape device.

Arguments:

    hDevice - Handle to the device on which to get the information.

    dwOperation - Type of information to get.
        This parameter can have one of the following values:

        GET_TAPE_MEDIA_INFORMATION - Return the media specific information in
            lpTapeInformation.

        GET_TAPE_DRIVE_INFORMATION - Return the device specific information in
            lpTapeInformation.

    lpdwSize - Pointer to a DWORD containing the size of the buffer pointed to
        by lpTapeInformation.  If the buffer is too small, this parameter
        returns with the required size in bytes.

    lpTapeInformation - Pointer to a buffer to receive the information.  The
        structure returned in the buffer is determined by dwOperation.

        For GET_TAPE_MEDIA_INFORMATION, lpTapeInformation returns the following
        structure:

        LARGE_INTEGER Capacity - The maximum tape capacity in bytes.

        LARGE_INTEGER Remaining - The remaining tape capacity in bytes.

        DWORD BlockSize - The size of a fixed-length logical block in bytes.
            A block size of  0 indicates variable-length block mode, where the
            length of a block is set by the write operation.  The default
            fixed-block size and the range of valid block sizes are returned
            by GetTapeParameters.

        DWORD PartitionCount - Number of partitions on the tape.  If only one
            partition is supported by the device, this parameter is set to 0.

        BOOLEAN WriteProtected - Indicates if the tape is write protected:
            0 is write enabled, 1 is write protected.


        For GET_TAPE_DRIVE_INFORMATION, lpTapeInformation returns the following
        structure:

        BOOLEAN ECC - Indicates if hardware error correction is enabled or
            disabled: 0 is disabled, 1 is enabled.

        BOOLEAN Compression - Indicates if hardware data compression is enabled
            or disabled: 0 is disabled, 1 is enabled.

        BOOLEAN DataPadding - Indicates if data padding is disabled or enabled:
            0 is disabled, 1 is enabled.

        BOOLEAN ReportSetmarks - Indicates if reporting setmarks is enabled or
            disabled: 0 is disabled, 1 is enabled.

        DWORD DefaultBlockSize - Returns the default fixed-block size for the
            device.

        DWORD MaximumBlockSize - Returns the maximum block size for the device.

        DWORD MinimumBlockSize - Returns the minimum block size for the device.

        DWORD MaximumPartitionCount - Returns the maximum number of partitions
            the device can create.

        DWORD FeaturesLow - The lower 32 bits of the device features flag.

        DWORD FeaturesHigh - The upper 32 bits of the device features flag.

            The device features flag represents the operations a device
            supports by returning a value of 1 in the appropriate bit for each
            feature supported.

            This parameter can have one or more of the following bit values
            set in the lower 32 bits:

            TAPE_DRIVE_FIXED - Supports creating fixed data partitions.

            TAPE_DRIVE_SELECT - Supports creating select data partitions.

            TAPE_DRIVE_INITIATOR - Supports creating initiator-defined
                partitions.

            TAPE_DRIVE_ERASE_SHORT - Supports short erase operation.

            TAPE_DRIVE_ERASE_LONG - Supports long erase operation.

            TAPE_DRIVE_ERASE_BOP_ONLY - Supports erase operation from the
                beginning-of-partition only.

            TAPE_DRIVE_TAPE_CAPACITY - Supports returning the maximum capacity
                of the tape.

            TAPE_DRIVE_TAPE_REMAINING - Supports returning the remaining
                capacity of the tape.

            TAPE_DRIVE_FIXED_BLOCK - Supports fixed-length block mode.

            TAPE_DRIVE_VARIABLE_BLOCK - Supports variable-length block mode.

            TAPE_DRIVE_WRITE_PROTECT - Supports returning if the tape is write
                enabled or write protected.

            TAPE_DRIVE_ECC - Supports hardware error correction.

            TAPE_DRIVE_COMPRESSION - Supports hardware data compression.

            TAPE_DRIVE_PADDING - Supports data padding.

            TAPE_DRIVE_REPORT_SMKS - Supports reporting setmarks.

            TAPE_DRIVE_GET_ABSOLUTE_BLK - Supports returning the current device
                specific block address.

            TAPE_DRIVE_GET_LOGICAL_BLK - Supports returning the current logical
                block address (and logical tape partition).

            This parameter can have one or more of the following bit values
            set in the upper 32 bits:

            TAPE_DRIVE_LOAD_UNLOAD - Supports enabling and disabling the device
                for further operations.

            TAPE_DRIVE_TENSION - Supports tensioning the tape.

            TAPE_DRIVE_LOCK_UNLOCK - Supports enabling and disabling removal of
                the tape from the device.

            TAPE_DRIVE_SET_BLOCK_SIZE - Supports setting the size of a
                fixed-length logical block or setting variable-length block
                mode.

            TAPE_DRIVE_SET_ECC - Supports enabling and disabling hardware error
                correction.

            TAPE_DRIVE_SET_COMPRESSION - Supports enabling and disabling
                hardware data compression.

            TAPE_DRIVE_SET_PADDING - Supports enabling and disabling data
                padding.

            TAPE_DRIVE_SET_REPORT_SMKS - Supports enabling and disabling
                reporting of setmarks.

            TAPE_DRIVE_ABSOLUTE_BLK - Supports positioning to a device specific
                block address.

            TAPE_DRIVE_ABS_BLK_IMMED - Supports immediate positioning to a
                device specific block address.

            TAPE_DRIVE_LOGICAL_BLK - Supports positioning to a logical block
                address in a partition.

            TAPE_DRIVE_LOG_BLK_IMMED - Supports immediate positioning to a
                logical block address in a partition.

            TAPE_DRIVE_END_OF_DATA - Supports positioning to the end-of-data
                in a partition.

            TAPE_DRIVE_RELATIVE_BLKS - Supports positioning forward (or
                reverse) a specified number of blocks.

            TAPE_DRIVE_FILEMARKS - Supports positioning forward (or reverse)
                a specified number of filemarks.

            TAPE_DRIVE_SEQUENTIAL_FMKS - Supports positioning forward (or
                reverse) to the first occurrence of a specified number of
                consecutive filemarks.

            TAPE_DRIVE_SETMARKS - Supports positioning forward (or reverse)
                a specified number of setmarks.

            TAPE_DRIVE_SEQUENTIAL_SMKS - Supports positioning forward (or
                reverse) to the first occurrence of a specified number of
                consecutive setmarks.

            TAPE_DRIVE_REVERSE_POSITION - Supports positioning over blocks,
                filemarks, or setmarks in the reverse direction.

            TAPE_DRIVE_WRITE_SETMARKS - Supports writing setmarks.

            TAPE_DRIVE_WRITE_FILEMARKS - Supports writing filemarks.

            TAPE_DRIVE_WRITE_SHORT_FMKS - Supports writing short filemarks.

            TAPE_DRIVE_WRITE_LONG_FMKS - Supports writing long filemarks.

Return Value:

    If the function is successful, the return value is NO_ERROR. Otherwise,
    it is a Win32 API error code.

--*/

{
    DWORD rc;

    switch (dwOperation) {
        case GET_TAPE_MEDIA_INFORMATION:

            if (*lpdwSize < sizeof(TAPE_GET_MEDIA_PARAMETERS)) {
                *lpdwSize = sizeof(TAPE_GET_MEDIA_PARAMETERS);
                rc = ERROR_MORE_DATA ;
            } else {
                rc = BasepDoTapeOperation( hDevice,
                                           IOCTL_TAPE_GET_MEDIA_PARAMS,
                                           NULL,
                                           0,
                                           lpTapeInformation,
                                           sizeof( TAPE_GET_MEDIA_PARAMETERS )
                                         );
            }
            break;

        case GET_TAPE_DRIVE_INFORMATION:
            if (*lpdwSize < sizeof(TAPE_GET_DRIVE_PARAMETERS)) {
                *lpdwSize = sizeof(TAPE_GET_DRIVE_PARAMETERS);
                rc = ERROR_MORE_DATA ;
            } else {
                rc = BasepDoTapeOperation( hDevice,
                                           IOCTL_TAPE_GET_DRIVE_PARAMS,
                                           NULL,
                                           0,
                                           lpTapeInformation,
                                           sizeof( TAPE_GET_DRIVE_PARAMETERS )
                                         );
            }
            break;

        default:
            rc = ERROR_INVALID_FUNCTION;
            break;
    }

    return rc;
}


DWORD
WINAPI
SetTapeParameters(
    HANDLE hDevice,
    DWORD dwOperation,
    LPVOID lpTapeInformation
    )

/*++

Routine Description:

    This API is used to set information about a tape device.

Arguments:

    hDevice - Handle to the device on which to set the information.

    dwOperation - Type of information to set.
        This parameter can have one of the following values:

        SET_TAPE_MEDIA_INFORMATION - Set the media specific information
            specified in lpTapeInformation.

        SET_TAPE_DRIVE_INFORMATION - Set the device specific information
            specified in lpTapeInformation.

    lpTapeInformation - Pointer to a buffer containing the information to set.
        The structure returned in the buffer is determined by dwOperation.

        For SET_TAPE_MEDIA_INFORMATION, lpTapeInformation contains the
        following structure:

        DWORD BlockSize - The size of a fixed-length logical block in bytes.
            A block size of 0 indicates variable-length block mode, where the
            length of a block is set by the write operation.  The default
            fixed-block size and the range of valid block sizes are returned
            by GetTapeParameters.


        For SET_TAPE_DRIVE_INFORMATION, lpTapeInformation contains the
        following structure:

        BOOLEAN ECC - Enables or disables hardware error correction: 0 is
            disabled, 1 is enabled.

        BOOLEAN Compression - Enables or disables hardware data compression:
            0 is disabled, 1 is enabled.

        BOOLEAN DataPadding - Enables or disables data padding: 0 is disabled,
            1 is enabled.

        BOOLEAN ReportSetmarks - Enables or disables reporting of setmarks:
            0 is disabled, 1 is enabled.

Return Value:

    If the function is successful, the return value is NO_ERROR. Otherwise,
    it is a Win32 API error code.

--*/

{
    DWORD rc;

    switch (dwOperation) {
        case SET_TAPE_MEDIA_INFORMATION:
            rc = BasepDoTapeOperation( hDevice,
                                       IOCTL_TAPE_SET_MEDIA_PARAMS,
                                       lpTapeInformation,
                                       sizeof( TAPE_SET_MEDIA_PARAMETERS ),
                                       NULL,
                                       0
                                     );
            break;

        case SET_TAPE_DRIVE_INFORMATION:
            rc = BasepDoTapeOperation( hDevice,
                                       IOCTL_TAPE_SET_DRIVE_PARAMS,
                                       lpTapeInformation,
                                       sizeof( TAPE_SET_DRIVE_PARAMETERS ),
                                       NULL,
                                       0
                                     );
            break;

        default:
            rc = ERROR_INVALID_FUNCTION;
            break;
    }

    return rc;
}


DWORD
WINAPI
GetTapeStatus(
    HANDLE hDevice
    )

/*++

Routine Description:

    This API is used to get the status of a tape device.

Arguments:

    hDevice - Handle to the device on which to get the status.

Return Value:

    If the device is ready to accept an appropriate medium-access command
    without returning an error, the return value is NO_ERROR. Otherwise,
    it is a Win32 API error code.

--*/

{
    return BasepDoTapeOperation( hDevice,
                                 IOCTL_TAPE_GET_STATUS,
                                 NULL,
                                 0,
                                 NULL,
                                 0
                               );
}
