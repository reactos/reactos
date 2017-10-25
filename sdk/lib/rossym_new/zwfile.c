/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rossym/zwfile.c
 * PURPOSE:         File I/O using native functions
 *
 * PROGRAMMERS:     Ge van Geldorp (gvg@reactos.com)
 */

#include <precomp.h>

NTSTATUS RosSymStatus;

BOOLEAN
RosSymZwReadFile(PVOID FileContext, PVOID Buffer, ULONG Size)
{
	//NTSTATUS Status;
  IO_STATUS_BLOCK IoStatusBlock;

  RosSymStatus = ZwReadFile(*((HANDLE *) FileContext),
                      NULL, NULL, NULL,
                      &IoStatusBlock,
                      Buffer,
                      Size,
                      NULL, NULL);

  return NT_SUCCESS(RosSymStatus) && IoStatusBlock.Information == Size;
}

BOOLEAN
RosSymZwSeekFile(PVOID FileContext, ULONG_PTR Position)
{
	//NTSTATUS Status;
  IO_STATUS_BLOCK IoStatusBlock;
  FILE_POSITION_INFORMATION NewPosition;

  NewPosition.CurrentByteOffset.u.HighPart = 0;
  NewPosition.CurrentByteOffset.u.LowPart = Position;
  RosSymStatus = ZwSetInformationFile(*((HANDLE *) FileContext),
                                &IoStatusBlock,
                                (PVOID) &NewPosition,
                                sizeof(FILE_POSITION_INFORMATION),
                                FilePositionInformation);

  return NT_SUCCESS(RosSymStatus);
}

/* EOF */
