/* $Id: errlog.c,v 1.8 2002/09/08 10:23:24 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/errlog.c
 * PURPOSE:         Error logging
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/port.h>

#include <internal/debug.h>

/* TYPES *********************************************************************/

#define  LOG_FILE_APPLICATION	L"\\SystemRoot\\System32\\Config\\AppEvent.Evt"
#define  LOG_FILE_SECURITY	L"\\SystemRoot\\System32\\Config\\SecEvent.Evt"
#define  LOG_FILE_SYSTEM	L"\\SystemRoot\\System32\\Config\\SysEvent.Evt"

typedef struct _IO_ERROR_LOG_PACKET
{
   UCHAR MajorFunctionCode;
   UCHAR RetryCount;
   USHORT DumpDataSize;
   USHORT NumberOfStrings;
   USHORT StringOffset;
   USHORT EventCategory;
   NTSTATUS ErrorCode;
   ULONG UniqueErrorValue;
   NTSTATUS FinalStatus;
   ULONG SequenceNumber;
   ULONG IoControlCode;
   LARGE_INTEGER DeviceOffset;
   ULONG DumpData[1];
} IO_ERROR_LOG_PACKET, *PIO_ERROR_LOG_PACKET;

/* FUNCTIONS *****************************************************************/

NTSTATUS IoInitErrorLog(VOID)
{
   return(STATUS_SUCCESS);
}

PVOID STDCALL IoAllocateErrorLogEntry(PVOID IoObject, UCHAR EntrySize)
{
   UNIMPLEMENTED;
}

VOID STDCALL IoWriteErrorLogEntry(PVOID ElEntry)
{
} 


/* EOF */
