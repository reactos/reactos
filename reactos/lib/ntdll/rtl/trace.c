/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: trace.c,v 1.2 2002/09/07 15:12:41 chorns Exp $
 *
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Tracing library calls
 * FILE:              lib/ntdll/rtl/trace.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#define NTOS_USER_MODE
#include <ntos.h>
#include <string.h>
#include <stdarg.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static NTDLL_TRACE_TABLE TraceTable;
static BOOLEAN TraceTableValid = FALSE;

/* FUNCTIONS *****************************************************************/

NTSTATUS
RtlpInitTrace(VOID)
{
  HANDLE SectionHandle;
  UNICODE_STRING SectionName;
  OBJECT_ATTRIBUTES ObjectAttributes;
  CHAR Buffer[4096];
  NTSTATUS Status;
  PROCESS_BASIC_INFORMATION Pbi;
  ULONG ReturnedSize;
  PVOID BaseAddress;
  LARGE_INTEGER Offset;
  ULONG ViewSize;

  Status = NtQueryInformationProcess(NtCurrentProcess(),
				     ProcessBasicInformation,
				     (PVOID)&Pbi,
				     sizeof(Pbi),
				     &ReturnedSize);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  sprintf(Buffer, "\\??\\TraceSection%d", Pbi.UniqueProcessId);

  InitializeObjectAttributes(&ObjectAttributes,
			     &SectionName,
			     0,
			     NULL,
			     NULL);
  Status = NtOpenSection(&SectionHandle,
			 SECTION_MAP_READ,
			 &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  BaseAddress = 0;
  Offset.QuadPart = 0;
  ViewSize = 0;
  Status = NtMapViewOfSection(SectionHandle,
			      NtCurrentProcess(),
			      &BaseAddress,
			      0,
			      sizeof(NTDLL_TRACE_TABLE),
			      &Offset,
			      &ViewSize,
			      ViewUnmap,
			      0,
			      PAGE_READONLY);
  if (!NT_SUCCESS(Status))
    {
      NtClose(SectionHandle);
      return(Status);
    }
  NtClose(SectionHandle);

  memcpy(&TraceTable, BaseAddress, sizeof(TraceTable));
  TraceTableValid = TRUE;

  Status = NtUnmapViewOfSection(NtCurrentProcess(), BaseAddress);

  return(STATUS_SUCCESS);
}

VOID
RtlPrintTrace(ULONG Flag, PCH Format, ...)
{
  va_list ap;
  CHAR FString[4096];

  if (!TraceTableValid)
    {
      return;
    }
  if (TraceTable.Flags[Flag / BITS_IN_CHAR] & (1 << (Flag % BITS_IN_CHAR)))
    {
      va_start(ap, Format);
      vsprintf(FString, Format, ap);
      DbgPrint(FString);
      va_end(ap);
    }
}
