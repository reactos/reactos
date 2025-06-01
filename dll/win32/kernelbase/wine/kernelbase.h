/*
 * Kernelbase internal definitions
 *
 * Copyright 2019 Alexandre Julliard
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

#ifndef __WINE_KERNELBASE_H
#define __WINE_KERNELBASE_H

#include "windef.h"
#include "winbase.h"

struct pseudo_console
{
    HANDLE signal;
    HANDLE reference;
    HANDLE process;
};

extern WCHAR *file_name_AtoW( LPCSTR name, BOOL alloc );
extern DWORD file_name_WtoA( LPCWSTR src, INT srclen, LPSTR dest, INT destlen );
extern void init_global_data(void);
extern void init_startup_info( RTL_USER_PROCESS_PARAMETERS *params );
extern void init_locale( HMODULE module );
extern void init_console(void);

extern const WCHAR windows_dir[];
extern const WCHAR system_dir[];

static const BOOL is_win64 = (sizeof(void *) > sizeof(int));
extern BOOL is_wow64;

static inline BOOL set_ntstatus( NTSTATUS status )
{
    if (status) SetLastError( RtlNtStatusToDosError( status ));
    return !status;
}

/* make the kernel32 names available */
#define HeapAlloc(heap, flags, size) RtlAllocateHeap(heap, flags, size)
#define HeapReAlloc(heap, flags, ptr, size) RtlReAllocateHeap(heap, flags, ptr, size)
#define HeapFree(heap, flags, ptr) RtlFreeHeap(heap, flags, ptr)

#endif /* __WINE_KERNELBASE_H */
