/*
 * Definitions for Rtl exception handling functions
 *
 * Copyright 2024 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _APISETRTLSUPPORT_
#define _APISETRTLSUPPORT_

NTSYSAPI void   WINAPI RtlCaptureContext(CONTEXT*);
NTSYSAPI void   WINAPI RtlCaptureContext2(CONTEXT*);
NTSYSAPI USHORT WINAPI RtlCaptureStackBackTrace(ULONG,ULONG,void**,ULONG*);
NTSYSAPI void   WINAPI RtlGetCallersAddress(void**,void**);
NTSYSAPI void   WINAPI RtlRaiseException(EXCEPTION_RECORD*);
NTSYSAPI void    CDECL RtlRestoreContext(CONTEXT*,EXCEPTION_RECORD*);
NTSYSAPI void   WINAPI RtlUnwind(void*,void*,EXCEPTION_RECORD*,void*);
NTSYSAPI void*  WINAPI RtlPcToFileHeader(void*,void**);
NTSYSAPI ULONG  WINAPI RtlWalkFrameChain(void**,ULONG,ULONG);

#ifndef __i386__

#define UNWIND_HISTORY_TABLE_SIZE 12

typedef struct _UNWIND_HISTORY_TABLE_ENTRY
{
    ULONG_PTR         ImageBase;
    PRUNTIME_FUNCTION FunctionEntry;
} UNWIND_HISTORY_TABLE_ENTRY, *PUNWIND_HISTORY_TABLE_ENTRY;

typedef struct _UNWIND_HISTORY_TABLE
{
    DWORD     Count;
    BYTE      LocalHint;
    BYTE      GlobalHint;
    BYTE      Search;
    BYTE      Once;
    ULONG_PTR LowAddress;
    ULONG_PTR HighAddress;
    UNWIND_HISTORY_TABLE_ENTRY Entry[UNWIND_HISTORY_TABLE_SIZE];
} UNWIND_HISTORY_TABLE, *PUNWIND_HISTORY_TABLE;

typedef PRUNTIME_FUNCTION (CALLBACK *PGET_RUNTIME_FUNCTION_CALLBACK)(DWORD_PTR,PVOID);

#define RTL_VIRTUAL_UNWIND2_VALIDATE_PAC 0x0001

NTSYSAPI BOOLEAN             CDECL RtlAddFunctionTable(RUNTIME_FUNCTION*,ULONG,ULONG_PTR);
NTSYSAPI NTSTATUS           WINAPI RtlAddGrowableFunctionTable(void**,PRUNTIME_FUNCTION,ULONG,ULONG,ULONG_PTR,ULONG_PTR);
NTSYSAPI BOOLEAN             CDECL RtlDeleteFunctionTable(RUNTIME_FUNCTION*);
NTSYSAPI void               WINAPI RtlDeleteGrowableFunctionTable(void*);
NTSYSAPI void               WINAPI RtlGrowFunctionTable(void*,ULONG);
NTSYSAPI BOOLEAN             CDECL RtlInstallFunctionTableCallback(ULONG_PTR,ULONG_PTR,ULONG,PGET_RUNTIME_FUNCTION_CALLBACK,PVOID,PCWSTR);
NTSYSAPI PRUNTIME_FUNCTION  WINAPI RtlLookupFunctionEntry(ULONG_PTR,ULONG_PTR*,UNWIND_HISTORY_TABLE*);
NTSYSAPI PRUNTIME_FUNCTION  WINAPI RtlLookupFunctionTable(ULONG_PTR,ULONG_PTR*,ULONG*);
NTSYSAPI void               WINAPI RtlUnwindEx(void*,void*,EXCEPTION_RECORD*,void*,CONTEXT*,UNWIND_HISTORY_TABLE*);
NTSYSAPI PEXCEPTION_ROUTINE WINAPI RtlVirtualUnwind(ULONG,ULONG_PTR,ULONG_PTR,RUNTIME_FUNCTION*,CONTEXT*,void**,ULONG_PTR*,KNONVOLATILE_CONTEXT_POINTERS*);
NTSYSAPI NTSTATUS           WINAPI RtlVirtualUnwind2(ULONG,ULONG_PTR,ULONG_PTR,RUNTIME_FUNCTION*,CONTEXT*,BOOLEAN*,void**,ULONG_PTR*,KNONVOLATILE_CONTEXT_POINTERS*,ULONG_PTR*,ULONG_PTR*,PEXCEPTION_ROUTINE*,ULONG);
#ifdef __x86_64__
NTSYSAPI BOOLEAN            WINAPI RtlIsEcCode(ULONG_PTR);
#endif

#endif  /* __i386__ */

#endif  /* _APISETRTLSUPPORT_ */
