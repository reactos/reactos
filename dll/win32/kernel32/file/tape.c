/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/tape.c
 * PURPOSE:         Tape functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
DWORD STDCALL
CreateTapePartition (HANDLE hDevice,
		     DWORD dwPartitionMethod,
		     DWORD dwCount,
		     DWORD dwSize)
{
  TAPE_CREATE_PARTITION TapeCreatePartition;
  IO_STATUS_BLOCK IoStatusBlock;
  DWORD ErrorCode;
  NTSTATUS Status;

  TapeCreatePartition.Method = dwPartitionMethod;
  TapeCreatePartition.Count = dwCount;
  TapeCreatePartition.Size = dwSize;

  Status = NtDeviceIoControlFile (hDevice,
				  NULL,
				  NULL,
				  NULL,
				  &IoStatusBlock,
				  IOCTL_TAPE_CREATE_PARTITION,
				  &TapeCreatePartition,
				  sizeof(TAPE_CREATE_PARTITION),
				  NULL,
				  0);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError(Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/*
 * @implemented
 */
DWORD STDCALL
EraseTape (HANDLE hDevice,
	   DWORD dwEraseType,
	   BOOL bImmediate)
{
  TAPE_ERASE TapeErase;
  IO_STATUS_BLOCK IoStatusBlock;
  DWORD ErrorCode;
  NTSTATUS Status;

  TapeErase.Type = dwEraseType;
  TapeErase.Immediate = (BOOLEAN)bImmediate;

  Status = NtDeviceIoControlFile (hDevice,
				  NULL,
				  NULL,
				  NULL,
				  &IoStatusBlock,
				  IOCTL_TAPE_ERASE,
				  &TapeErase,
				  sizeof(TAPE_ERASE),
				  NULL,
				  0);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError(Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/*
 * @implemented
 */
DWORD STDCALL
GetTapeParameters (HANDLE hDevice,
		   DWORD dwOperation,
		   LPDWORD lpdwSize,
		   LPVOID lpTapeInformation)
{
  IO_STATUS_BLOCK IoStatusBlock;
  DWORD ErrorCode;
  NTSTATUS Status;

  if (dwOperation == GET_TAPE_MEDIA_INFORMATION)
    {
      if (*lpdwSize < sizeof(TAPE_GET_MEDIA_PARAMETERS))
	{
	  *lpdwSize = sizeof(TAPE_GET_MEDIA_PARAMETERS);
	  return ERROR_MORE_DATA;
	}

      Status = NtDeviceIoControlFile (hDevice,
				      NULL,
				      NULL,
				      NULL,
				      &IoStatusBlock,
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

      Status = NtDeviceIoControlFile (hDevice,
				      NULL,
				      NULL,
				      NULL,
				      &IoStatusBlock,
				      IOCTL_TAPE_GET_DRIVE_PARAMS,
				      NULL,
				      0,
				      lpTapeInformation,
				      sizeof(TAPE_GET_DRIVE_PARAMETERS));
    }
  else
    {
      return ERROR_INVALID_FUNCTION;
    }

  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError(Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/*
 * @implemented
 */
DWORD STDCALL
GetTapePosition (HANDLE hDevice,
		 DWORD dwPositionType,
		 LPDWORD lpdwPartition,
		 LPDWORD lpdwOffsetLow,
		 LPDWORD lpdwOffsetHigh)
{
  TAPE_GET_POSITION TapeGetPosition;
  IO_STATUS_BLOCK IoStatusBlock;
  DWORD ErrorCode;
  NTSTATUS Status;

  TapeGetPosition.Type = dwPositionType;

  Status = NtDeviceIoControlFile (hDevice,
				  NULL,
				  NULL,
				  NULL,
				  &IoStatusBlock,
				  IOCTL_TAPE_GET_POSITION,
				  &TapeGetPosition,
				  sizeof(TAPE_GET_POSITION),
				  &TapeGetPosition,
				  sizeof(TAPE_GET_POSITION));
  if (!NT_SUCCESS(Status))
    {
      *lpdwPartition = 0;
      *lpdwOffsetLow = 0;
      *lpdwOffsetHigh = 0;

      ErrorCode = RtlNtStatusToDosError(Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  *lpdwPartition = TapeGetPosition.Partition;
  *lpdwOffsetLow = TapeGetPosition.Offset.u.LowPart;
  *lpdwOffsetHigh = TapeGetPosition.Offset.u.HighPart;

  return ERROR_SUCCESS;
}


/*
 * @implemented
 */
DWORD STDCALL
GetTapeStatus (HANDLE hDevice)
{
  IO_STATUS_BLOCK IoStatusBlock;
  DWORD ErrorCode;
  NTSTATUS Status;

  Status = NtDeviceIoControlFile (hDevice,
				  NULL,
				  NULL,
				  NULL,
				  &IoStatusBlock,
				  IOCTL_TAPE_GET_STATUS,
				  NULL,
				  0,
				  NULL,
				  0);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError(Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/*
 * @implemented
 */
DWORD STDCALL
PrepareTape (HANDLE hDevice,
	     DWORD dwOperation,
	     BOOL bImmediate)
{
  TAPE_PREPARE TapePrepare;
  IO_STATUS_BLOCK IoStatusBlock;
  DWORD ErrorCode;
  NTSTATUS Status;

  TapePrepare.Operation = dwOperation;
  TapePrepare.Immediate = (BOOLEAN)bImmediate;

  Status = NtDeviceIoControlFile (hDevice,
				  NULL,
				  NULL,
				  NULL,
				  &IoStatusBlock,
				  IOCTL_TAPE_PREPARE,
				  &TapePrepare,
				  sizeof(TAPE_PREPARE),
				  NULL,
				  0);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError(Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/*
 * @implemented
 */
DWORD STDCALL
SetTapeParameters (HANDLE hDevice,
		   DWORD dwOperation,
		   LPVOID lpTapeInformation)
{
  IO_STATUS_BLOCK IoStatusBlock;
  DWORD ErrorCode;
  NTSTATUS Status;

  if (dwOperation == SET_TAPE_MEDIA_INFORMATION)
    {
      Status = NtDeviceIoControlFile (hDevice,
				      NULL,
				      NULL,
				      NULL,
				      &IoStatusBlock,
				      IOCTL_TAPE_SET_MEDIA_PARAMS,
				      lpTapeInformation,
				      sizeof(TAPE_SET_MEDIA_PARAMETERS),
				      NULL,
				      0);
    }
  else if (dwOperation == SET_TAPE_DRIVE_INFORMATION)
    {
      Status = NtDeviceIoControlFile (hDevice,
				      NULL,
				      NULL,
				      NULL,
				      &IoStatusBlock,
				      IOCTL_TAPE_SET_DRIVE_PARAMS,
				      lpTapeInformation,
				      sizeof(TAPE_SET_DRIVE_PARAMETERS),
				      NULL,
				      0);
    }
  else
    {
      return ERROR_INVALID_FUNCTION;
    }

  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError(Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/*
 * @implemented
 */
DWORD STDCALL
SetTapePosition (HANDLE hDevice,
		 DWORD dwPositionMethod,
		 DWORD dwPartition,
		 DWORD dwOffsetLow,
		 DWORD dwOffsetHigh,
		 BOOL bImmediate)
{
  TAPE_SET_POSITION TapeSetPosition;
  IO_STATUS_BLOCK IoStatusBlock;
  DWORD ErrorCode;
  NTSTATUS Status;

  TapeSetPosition.Method = dwPositionMethod;
  TapeSetPosition.Partition = dwPartition;
  TapeSetPosition.Offset.u.LowPart = dwOffsetLow;
  TapeSetPosition.Offset.u.HighPart = dwOffsetHigh;
  TapeSetPosition.Immediate = (BOOLEAN)bImmediate;

  Status = NtDeviceIoControlFile (hDevice,
				  NULL,
				  NULL,
				  NULL,
				  &IoStatusBlock,
				  IOCTL_TAPE_SET_POSITION,
				  &TapeSetPosition,
				  sizeof(TAPE_SET_POSITION),
				  NULL,
				  0);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError(Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/*
 * @implemented
 */
DWORD STDCALL
WriteTapemark (HANDLE hDevice,
	       DWORD dwTapemarkType,
	       DWORD dwTapemarkCount,
	       BOOL bImmediate)
{
  TAPE_WRITE_MARKS TapeWriteMarks;
  IO_STATUS_BLOCK IoStatusBlock;
  DWORD ErrorCode;
  NTSTATUS Status;

  TapeWriteMarks.Type = dwTapemarkType;
  TapeWriteMarks.Count = dwTapemarkCount;
  TapeWriteMarks.Immediate = (BOOLEAN)bImmediate;

  Status = NtDeviceIoControlFile (hDevice,
				  NULL,
				  NULL,
				  NULL,
				  &IoStatusBlock,
				  IOCTL_TAPE_WRITE_MARKS,
				  &TapeWriteMarks,
				  sizeof(TAPE_WRITE_MARKS),
				  NULL,
				  0);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError(Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}

/* EOF */
