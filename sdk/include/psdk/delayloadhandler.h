/*
 * Copyright 2013 André Hentschel
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

#ifndef __WINE_DELAYLOADHANDLER_H
#define __WINE_DELAYLOADHANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#define DELAYLOAD_GPA_FAILURE 4

typedef struct _DELAYLOAD_PROC_DESCRIPTOR
{
    ULONG ImportDescribedByName;
    union {
        LPCSTR Name;
        ULONG Ordinal;
    } Description;
} DELAYLOAD_PROC_DESCRIPTOR, *PDELAYLOAD_PROC_DESCRIPTOR;

typedef struct _DELAYLOAD_INFO
{
    ULONG Size;
    PCIMAGE_DELAYLOAD_DESCRIPTOR DelayloadDescriptor;
    PIMAGE_THUNK_DATA ThunkAddress;
    LPCSTR TargetDllName;
    DELAYLOAD_PROC_DESCRIPTOR TargetApiDescriptor;
    PVOID TargetModuleBase;
    PVOID Unused;
    ULONG LastError;
} DELAYLOAD_INFO, *PDELAYLOAD_INFO;

typedef PVOID (WINAPI *PDELAYLOAD_FAILURE_DLL_CALLBACK)(ULONG, PDELAYLOAD_INFO);
typedef PVOID (WINAPI *PDELAYLOAD_FAILURE_SYSTEM_ROUTINE)(LPCSTR, LPCSTR);

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_DELAYLOADHANDLER_H */
