/* $Id: errlog.c,v 1.7 2002/09/07 15:12:52 chorns Exp $
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

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* TYPES *********************************************************************/

#define  LOG_FILE_APPLICATION	L"\\SystemRoot\\System32\\Config\\AppEvent.Evt"
#define  LOG_FILE_SECURITY	L"\\SystemRoot\\System32\\Config\\SecEvent.Evt"
#define  LOG_FILE_SYSTEM	L"\\SystemRoot\\System32\\Config\\SysEvent.Evt"

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
