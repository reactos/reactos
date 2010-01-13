/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rossym/zwfile.c
 * PURPOSE:         File I/O using native functions
 *
 * PROGRAMMERS:     Ge van Geldorp (gvg@reactos.com)
 */

#define NTOSAPI
#include <ntddk.h>
#include <reactos/rossym.h>
#include "rossympriv.h"

#define NDEBUG
#include <debug.h>

BOOLEAN
RosSymZwReadFile(PVOID FileContext, PVOID Buffer, ULONG Size)
{
  NTSTATUS Status;
  IO_STATUS_BLOCK IoStatusBlock;

  Status = ZwReadFile(*((HANDLE *) FileContext),
                      0, 0, 0,
                      &IoStatusBlock,
                      Buffer,
                      Size,
                      0, 0);

  return NT_SUCCESS(Status) && IoStatusBlock.Information == Size;
}

BOOLEAN
RosSymZwSeekFile(PVOID FileContext, ULONG_PTR Position)
{
  NTSTATUS Status;
  IO_STATUS_BLOCK IoStatusBlock;
  FILE_POSITION_INFORMATION NewPosition;

  NewPosition.CurrentByteOffset.u.HighPart = 0;
  NewPosition.CurrentByteOffset.u.LowPart = Position;
  Status = ZwSetInformationFile(*((HANDLE *) FileContext),
                                &IoStatusBlock,
                                (PVOID) &NewPosition,
                                sizeof(FILE_POSITION_INFORMATION),
                                FilePositionInformation);

  return NT_SUCCESS(Status);
}

/* EOF */
