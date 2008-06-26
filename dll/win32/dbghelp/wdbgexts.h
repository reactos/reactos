/*
 * File wdbgexts.h: definition of windbg extensions
 *                  (dbghelp.dll is seen as a windbg extension)
 *
 * Copyright (C) 2005, Eric Pouech
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

typedef struct EXT_API_VERSION
{
    USHORT  MajorVersion;
    USHORT  MinorVersion;
    USHORT  Revision;
    USHORT  Reserved;
} EXT_API_VERSION, *LPEXT_API_VERSION;

typedef void  (*PWINDBG_OUTPUT_ROUTINE)(PCSTR, ...);
typedef ULONG_PTR (WINAPI *PWINDBG_GET_EXPRESSION)(PCSTR);
typedef void  (WINAPI *PWINDBG_GET_SYMBOL)(void*, char*, ULONG_PTR*);
typedef ULONG (WINAPI *PWINDBG_DISASM)(ULONG_PTR*, PCSTR, ULONG);
typedef ULONG (WINAPI *PWINDBG_CHECK_CONTROL_C)(void);
typedef ULONG (WINAPI *PWINDBG_READ_PROCESS_MEMORY_ROUTINE)(ULONG_PTR, void*, ULONG, PULONG);
typedef ULONG (WINAPI *PWINDBG_WRITE_PROCESS_MEMORY_ROUTINE)(ULONG_PTR, const void*, ULONG, PULONG);
typedef ULONG (WINAPI *PWINDBG_GET_THREAD_CONTEXT_ROUTINE)(ULONG, PCONTEXT, ULONG);
typedef ULONG (WINAPI *PWINDBG_SET_THREAD_CONTEXT_ROUTINE)(ULONG, PCONTEXT, ULONG);
typedef ULONG (WINAPI *PWINDBG_IOCTL_ROUTINE)(USHORT, void*);
typedef struct _EXTSTACKTRACE
{
    ULONG       FramePointer;
    ULONG       ProgramCounter;
    ULONG       ReturnAddress;
    ULONG       Args[4];
} EXTSTACKTRACE, *PEXTSTACKTRACE;
typedef ULONG (WINAPI *PWINDBG_STACKTRACE_ROUTINE)(ULONG, ULONG, ULONG, PEXTSTACKTRACE, ULONG);

typedef struct _WINDBG_EXTENSION_APIS
{
    ULONG                                  nSize;
    PWINDBG_OUTPUT_ROUTINE                 lpOutputRoutine;
    PWINDBG_GET_EXPRESSION                 lpGetExpressionRoutine;
    PWINDBG_GET_SYMBOL                     lpGetSymbolRoutine;
    PWINDBG_DISASM                         lpDisasmRoutine;
    PWINDBG_CHECK_CONTROL_C                lpCheckControlCRoutine;
    PWINDBG_READ_PROCESS_MEMORY_ROUTINE    lpReadProcessMemoryRoutine;
    PWINDBG_WRITE_PROCESS_MEMORY_ROUTINE   lpWriteProcessMemoryRoutine;
    PWINDBG_GET_THREAD_CONTEXT_ROUTINE     lpGetThreadContextRoutine;
    PWINDBG_SET_THREAD_CONTEXT_ROUTINE     lpSetThreadContextRoutine;
    PWINDBG_IOCTL_ROUTINE                  lpIoctlRoutine;
    PWINDBG_STACKTRACE_ROUTINE             lpStackTraceRoutine;
} WINDBG_EXTENSION_APIS, *PWINDBG_EXTENSION_APIS;
