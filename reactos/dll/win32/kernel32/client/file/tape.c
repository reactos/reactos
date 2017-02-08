/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/tape.c
 * PURPOSE:         Tape functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *******************************************************************/

#include <k32.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

DWORD
WINAPI
BasepDoTapeOperation(IN HANDLE DeviceHandle,
                     IN ULONG Ioctl,
                     IN PVOID Input,
                     IN ULONG InputLength,
                     IN PVOID Output,
                     IN ULONG OutputLength)
{
    HANDLE TapeEvent;
    DWORD ErrorCode;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Create the wait event */
    TapeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!TapeEvent) return GetLastError();

    /* Send the IOCTL */
    Status = NtDeviceIoControlFile(DeviceHandle,
                                   TapeEvent,
                                   0,
                                   0,
                                   &IoStatusBlock,
                                   Ioctl,
                                   Input,
                                   InputLength,
                                   Output,
                                   OutputLength);
    if (Status == STATUS_PENDING)
    {
        /* Wait for its completion */
        WaitForSingleObject(TapeEvent, INFINITE);
        Status = IoStatusBlock.Status;
    }

    /* Get rid of the wait event and check status */
    CloseHandle(TapeEvent);
    if (!NT_SUCCESS(Status))
    {
        /* Convert to Win32 */
        BaseSetLastNTError(Status);
        ErrorCode = GetLastError();
    }
    else
    {
        /* Set sucess */
        ErrorCode = ERROR_SUCCESS;
    }

    /* Return the Win32 error code */
    return ErrorCode;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
DWORD
WINAPI
CreateTapePartition(IN HANDLE hDevice,
                    IN DWORD dwPartitionMethod,
                    IN DWORD dwCount,
                    IN DWORD dwSize)
{
    TAPE_CREATE_PARTITION TapeCreatePartition;

    TapeCreatePartition.Method = dwPartitionMethod;
    TapeCreatePartition.Count = dwCount;
    TapeCreatePartition.Size = dwSize;
    return BasepDoTapeOperation(hDevice,
                                IOCTL_TAPE_CREATE_PARTITION,
                                &TapeCreatePartition,
                                sizeof(TapeCreatePartition),
                                NULL,
                                0);
}

/*
 * @implemented
 */
DWORD
WINAPI
EraseTape(IN HANDLE hDevice,
          IN DWORD dwEraseType,
          IN BOOL bImmediate)
{
    TAPE_ERASE TapeErase;

    TapeErase.Type = dwEraseType;
    TapeErase.Immediate = (BOOLEAN)bImmediate;
    return BasepDoTapeOperation(hDevice,
                                IOCTL_TAPE_ERASE,
                                &TapeErase,
                                sizeof(TapeErase),
                                NULL,
                                0);
}

/*
 * @implemented
 */
DWORD
WINAPI
GetTapeParameters(IN HANDLE hDevice,
                  IN DWORD dwOperation,
                  IN LPDWORD lpdwSize,
                  IN LPVOID lpTapeInformation)
{
    if (dwOperation == GET_TAPE_MEDIA_INFORMATION)
    {
        if (*lpdwSize < sizeof(TAPE_GET_MEDIA_PARAMETERS))
        {
            *lpdwSize = sizeof(TAPE_GET_MEDIA_PARAMETERS);
            return ERROR_MORE_DATA;
        }

        return BasepDoTapeOperation(hDevice,
                                    IOCTL_TAPE_GET_MEDIA_PARAMS,
                                    NULL,
                                    0,
                                    lpTapeInformation,
                                    sizeof(TAPE_GET_MEDIA_PARAMETERS));
    }
    else if (dwOperation == GET_TAPE_DRIVE_INFORMATION)
    {
        if (*lpdwSize < sizeof(TAPE_GET_DRIVE_PARAMETERS))
        {
            *lpdwSize = sizeof(TAPE_GET_DRIVE_PARAMETERS);
            return ERROR_MORE_DATA;
        }

        return BasepDoTapeOperation(hDevice,
                                    IOCTL_TAPE_GET_DRIVE_PARAMS,
                                    NULL,
                                    0,
                                    lpTapeInformation,
                                    sizeof(TAPE_GET_DRIVE_PARAMETERS));
    }

    return ERROR_INVALID_FUNCTION;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetTapePosition(IN HANDLE hDevice,
                IN DWORD dwPositionType,
                IN LPDWORD lpdwPartition,
                IN LPDWORD lpdwOffsetLow,
                IN LPDWORD lpdwOffsetHigh)
{
    TAPE_GET_POSITION TapeGetPosition;
    DWORD Result;

    TapeGetPosition.Type = dwPositionType;
    Result = BasepDoTapeOperation(hDevice,
                                  IOCTL_TAPE_GET_POSITION,
                                  &TapeGetPosition,
                                  sizeof(TapeGetPosition),
                                  &TapeGetPosition,
                                  sizeof(TapeGetPosition));

    if (Result)
    {
        *lpdwPartition = 0;
        *lpdwOffsetLow = 0;
        *lpdwOffsetHigh = 0;
    }
    else
    {
        *lpdwPartition = TapeGetPosition.Partition;
        *lpdwOffsetLow = TapeGetPosition.Offset.u.LowPart;
        *lpdwOffsetHigh = TapeGetPosition.Offset.u.HighPart;
    }

    return Result;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetTapeStatus(IN HANDLE hDevice)
{
    return BasepDoTapeOperation(hDevice,
                                IOCTL_TAPE_GET_STATUS,
                                NULL,
                                0,
                                NULL,
                                0);
}

/*
 * @implemented
 */
DWORD
WINAPI
PrepareTape(IN HANDLE hDevice,
            IN DWORD dwOperation,
            IN BOOL bImmediate)
{
    TAPE_PREPARE TapePrepare;

    TapePrepare.Operation = dwOperation;
    TapePrepare.Immediate = (BOOLEAN)bImmediate;
    return BasepDoTapeOperation(hDevice,
                                IOCTL_TAPE_PREPARE,
                                &TapePrepare,
                                sizeof(TapePrepare),
                                NULL,
                                0);
}

/*
 * @implemented
 */
DWORD
WINAPI
SetTapeParameters(IN HANDLE hDevice,
                  IN DWORD dwOperation,
                  IN LPVOID lpTapeInformation)
{
    if (dwOperation == SET_TAPE_MEDIA_INFORMATION)
    {
        return BasepDoTapeOperation(hDevice,
                                    IOCTL_TAPE_SET_MEDIA_PARAMS,
                                    lpTapeInformation,
                                    sizeof(TAPE_SET_MEDIA_PARAMETERS),
                                    NULL,
                                    0);
    }
    else if (dwOperation == SET_TAPE_DRIVE_INFORMATION)
    {
        return BasepDoTapeOperation(hDevice,
                                    IOCTL_TAPE_SET_DRIVE_PARAMS,
                                    lpTapeInformation,
                                    sizeof(TAPE_SET_DRIVE_PARAMETERS),
                                    NULL,
                                    0);
    }

    return ERROR_INVALID_FUNCTION;
}

/*
 * @implemented
 */
DWORD
WINAPI
SetTapePosition(IN HANDLE hDevice,
                IN DWORD dwPositionMethod,
                IN DWORD dwPartition,
                IN DWORD dwOffsetLow,
                IN DWORD dwOffsetHigh,
                IN BOOL bImmediate)
{
    TAPE_SET_POSITION TapeSetPosition;

    TapeSetPosition.Method = dwPositionMethod;
    TapeSetPosition.Partition = dwPartition;
    TapeSetPosition.Offset.u.LowPart = dwOffsetLow;
    TapeSetPosition.Offset.u.HighPart = dwOffsetHigh;
    TapeSetPosition.Immediate = (BOOLEAN)bImmediate;
    return BasepDoTapeOperation(hDevice,
                                IOCTL_TAPE_SET_POSITION,
                                &TapeSetPosition,
                                sizeof(TapeSetPosition),
                                NULL,
                                0);
}

/*
 * @implemented
 */
DWORD
WINAPI
WriteTapemark(IN HANDLE hDevice,
              IN DWORD dwTapemarkType,
              IN DWORD dwTapemarkCount,
              IN BOOL bImmediate)
{
    TAPE_WRITE_MARKS TapeWriteMarks;

    TapeWriteMarks.Type = dwTapemarkType;
    TapeWriteMarks.Count = dwTapemarkCount;
    TapeWriteMarks.Immediate = (BOOLEAN)bImmediate;
    return BasepDoTapeOperation(hDevice,
                                IOCTL_TAPE_WRITE_MARKS,
                                &TapeWriteMarks,
                                sizeof(TapeWriteMarks),
                                NULL,
                                0);
}

/* EOF */
