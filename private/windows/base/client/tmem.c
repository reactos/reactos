/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tbase.c

Abstract:

    Skeleton for a Win32 Base API Test Program

Author:

    Steve Wood (stevewo) 26-Oct-1990

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <stdio.h>

#ifndef DBG
#define DBG 0
#endif
#if DBG
#define IF_DEBUG if (TRUE)
#else
#define IF_DEBUG if (FALSE)
#endif
#include "basertl.h"

DWORD
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    NTSTATUS Status;
    LPVOID p1, p2, p3, p4, p5, p6;
    HANDLE h1, h2, h3, h4, h5, h6;
    BOOL result;
    LPDWORD p;

    printf( "TMEM: Entering Test Program\n" );

    p1 = LocalAlloc( LMEM_ZEROINIT, 0 );
    printf( "LocalAlloc( 0 ) == %lX\n", p1 );
    printf( "LocalSize( %lX ) == %ld\n", p1, LocalSize( p1 ) );

    p2 = LocalReAlloc( p1, 23, 0 );
    printf( "LocalReAlloc( %lX, 23 ) == %lX\n", p1, p2 );
    printf( "LocalSize( %lX ) == %ld\n", p2, LocalSize( p2 ) );
    printf( "LocalFree( %lX ) == %ld\n", p2, LocalFree( p2 ) );

    p1 = LocalAlloc( LMEM_ZEROINIT, 123 );
    printf( "LocalAlloc( 123 ) == %lX\n", p1 );
    printf( "LocalSize( %lX ) == %ld\n", p1, LocalSize( p1 ) );

    p2 = LocalReAlloc( p1, 23, 0 );
    printf( "LocalReAlloc( %lX, 23 ) == %lX\n", p1, p2 );
    printf( "LocalSize( %lX ) == %ld\n", p2, LocalSize( p2 ) );

    p3 = LocalReAlloc( p2, 223, LMEM_ZEROINIT );
    printf( "LocalReAlloc( %lX, 223 ) == %lX\n", p2, p3 );
    printf( "LocalSize( %lX ) == %ld\n", p3, LocalSize( p3 ) );

    p4 = LocalReAlloc( p3, 22, 0 );
    printf( "LocalReAlloc( %lX, 22 ) == %lX\n", p3, p4 );
    printf( "LocalSize( %lX ) == %ld\n", p4, LocalSize( p4 ) );
    printf( "LocalFree( %lX ) == %ld\n", p4, LocalFree( p4 ) );

    h1 = LocalAlloc( LMEM_MOVEABLE | LMEM_ZEROINIT, 123 );
    printf( "LocalAlloc( MOVEABLE, 123 ) == %lX (-> %lX)\n", h1, LocalLock( h1 ) );
    printf( "LocalSize( %lX ) == %ld\n", h1, LocalSize( h1 ) );

    h2 = LocalReAlloc( h1, 23, 0 );
    printf( "LocalReAlloc( %lX, 23 ) == %lX\n", h1, h2, LocalLock( h2 ) );
    printf( "LocalSize( %lX ) == %ld\n", h2, LocalSize( h2 ) );

    h3 = LocalReAlloc( h2, 223, LMEM_ZEROINIT );
    printf( "LocalReAlloc( %lX, 223 ) == %lX\n", h2, h3, LocalLock( h3 ) );
    printf( "LocalSize( %lX ) == %ld\n", h3, LocalSize( h3 ) );

    h4 = LocalReAlloc( h3, 22, 0 );
    printf( "LocalReAlloc( %lX, 22 ) == %lX\n", h3, h4, LocalLock( h4 ) );
    printf( "LocalSize( %lX ) == %ld\n", h4, LocalSize( h4 ) );
    printf( "LocalFree( %lX ) == %ld\n", h4, LocalFree( h4 ) );


    h1 = GlobalAlloc( GMEM_ZEROINIT, 123 );
    printf( "GlobalAlloc( 123 ) == %lX\n", h1 );
    printf( "GlobalFlags( %lX ) == %ld\n", h1, GlobalFlags( h1 ) );
    printf( "GlobalSize( %lX ) == %ld\n", h1, GlobalSize( h1 ) );

    h2 = GlobalReAlloc( h1, 23, 0 );
    printf( "GlobalReAlloc( %lX, 23 ) == %lX\n", h1, h2 );
    printf( "GlobalSize( %lX ) == %ld\n", h2, GlobalSize( h2 ) );

    h3 = GlobalReAlloc( h2, 223, GMEM_ZEROINIT );
    printf( "GlobalReAlloc( %lX, 223 ) == %lX\n", h2, h3 );
    printf( "GlobalSize( %lX ) == %ld\n", h3, GlobalSize( h3 ) );

    h4 = GlobalReAlloc( h3, 22, 0 );
    printf( "GlobalReAlloc( %lX, 22 ) == %lX\n", h3, h4 );
    printf( "GlobalSize( %lX ) == %ld\n", h4, GlobalSize( h4 ) );
    printf( "GlobalFree( %lX ) == %ld\n", h4, GlobalFree( h4 ) );

    h1 = GlobalAlloc( GMEM_ZEROINIT, 12003 );
    printf( "GlobalAlloc( 123 ) == %lX\n", h1 );
    printf( "GlobalFlags( %lX ) == %ld\n", h1, GlobalFlags( h1 ) );
    printf( "GlobalSize( %lX ) == %ld\n", h1, GlobalSize( h1 ) );

    h2 = GlobalReAlloc( h1, 4000, 0 );
    printf( "GlobalReAlloc( %lX, 23 ) == %lX\n", h1, h2 );
    printf( "GlobalSize( %lX ) == %ld\n", h2, GlobalSize( h2 ) );

    h3 = GlobalReAlloc( h2, 8000, GMEM_ZEROINIT );
    printf( "GlobalReAlloc( %lX, 223 ) == %lX\n", h2, h3 );
    printf( "GlobalSize( %lX ) == %ld\n", h3, GlobalSize( h3 ) );

    h4 = GlobalReAlloc( h3, 22, 0 );
    printf( "GlobalReAlloc( %lX, 22 ) == %lX\n", h3, h4 );
    printf( "GlobalSize( %lX ) == %ld\n", h4, GlobalSize( h4 ) );
    printf( "GlobalFree( %lX ) == %ld\n", h4, GlobalFree( h4 ) );

    DbgBreakPoint();

    h1 = GlobalAlloc( GMEM_DDESHARE, 123 );
    printf( "GlobalAlloc( 123 ) == %lX\n", h1 );
    printf( "GlobalFlags( %lX ) == %ld\n", h1, GlobalFlags( h1 ) );
    printf( "GlobalSize( %lX ) == %ld\n", h1, GlobalSize( h1 ) );

    p = GlobalLock( h1 );
    if (*p != 0xABCDEF01) {
        DbgBreakPoint();
        }

    h2 = GlobalReAlloc( h1, 23, 0 );
    printf( "GlobalReAlloc( %lX, 23 ) == %lX\n", h1, h2 );
    printf( "GlobalSize( %lX ) == %ld\n", h2, GlobalSize( h2 ) );

    h3 = GlobalReAlloc( h2, 223, GMEM_ZEROINIT );
    printf( "GlobalReAlloc( %lX, 223 ) == %lX\n", h2, h3 );
    printf( "GlobalSize( %lX ) == %ld\n", h3, GlobalSize( h3 ) );

    h4 = GlobalReAlloc( h3, 22, 0 );
    printf( "GlobalReAlloc( %lX, 22 ) == %lX\n", h3, h4 );
    printf( "GlobalSize( %lX ) == %ld\n", h4, GlobalSize( h4 ) );
    printf( "GlobalFree( %lX ) == %ld\n", h4, GlobalFree( h4 ) );

    printf( "TMEM: Exiting Test Program\n" );

    return 0;
}

#if 0
    MEMORY_BASIC_INFORMATION MemoryInformation;
    DWORD cb;
    DWORD oldProtect;

    p1 = VirtualAlloc( NULL, 8192, MEM_RESERVE, PAGE_READONLY );
    p2 = VirtualAlloc( (LPVOID)((LPSTR)p1 + 4096), 4096, MEM_COMMIT, PAGE_READWRITE );
    try {
        *(LPDWORD)p1 = -1;
        *(LPDWORD)p2 = -2;
        }
    except( 1 ) {
        }

    cb = VirtualQuery( (LPVOID)p1, &MemoryInformation, sizeof( MemoryInformation ) );
    printf( "VirtualQuery( %X ) = %u (%d)\n", p1, cb, GetLastError() );
    printf( "    BaseAddress:       %x\n", MemoryInformation.BaseAddress       );
    printf( "    AllocationBase:    %x\n", MemoryInformation.AllocationBase    );
    printf( "    AllocationProtect: %x\n", MemoryInformation.AllocationProtect );
    printf( "    RegionSize:        %u\n", MemoryInformation.RegionSize        );
    printf( "    State:             %x\n", MemoryInformation.State             );
    printf( "    Protect:           %x\n", MemoryInformation.Protect           );
    printf( "    Type:              %x\n", MemoryInformation.Type              );

    cb = VirtualQuery( (LPVOID)p2, &MemoryInformation, sizeof( MemoryInformation ) );
    printf( "VirtualQuery( %X ) = %u (%d)\n", p2, cb, GetLastError() );
    printf( "    BaseAddress:       %x\n", MemoryInformation.BaseAddress       );
    printf( "    AllocationBase:    %x\n", MemoryInformation.AllocationBase    );
    printf( "    AllocationProtect: %x\n", MemoryInformation.AllocationProtect );
    printf( "    RegionSize:        %u\n", MemoryInformation.RegionSize        );
    printf( "    State:             %x\n", MemoryInformation.State             );
    printf( "    Protect:           %x\n", MemoryInformation.Protect           );
    printf( "    Type:              %x\n", MemoryInformation.Type              );

    result = VirtualProtect( (LPVOID)p2, 4096, PAGE_READONLY, &oldProtect );
    printf( "VirtualProtect( %X, 4096, PAGE_READONLY, oldProtect == %x ) = %d (%d)\n", p2, oldProtect, result, GetLastError() );

    cb = VirtualQuery( (LPVOID)p2, &MemoryInformation, sizeof( MemoryInformation ) );
    printf( "VirtualQuery( %X ) = %u (%d)\n", p2, cb, GetLastError() );
    printf( "    BaseAddress:       %x\n", MemoryInformation.BaseAddress       );
    printf( "    AllocationBase:    %x\n", MemoryInformation.AllocationBase    );
    printf( "    AllocationProtect: %x\n", MemoryInformation.AllocationProtect );
    printf( "    RegionSize:        %u\n", MemoryInformation.RegionSize        );
    printf( "    State:             %x\n", MemoryInformation.State             );
    printf( "    Protect:           %x\n", MemoryInformation.Protect           );
    printf( "    Type:              %x\n", MemoryInformation.Type              );

    result = VirtualFree( (LPVOID)p2, 4096, MEM_DECOMMIT );
    printf( "VirtualFree( %X, 4096, MEM_DECOMMIT ) = %d\n", p2, result, GetLastError() );

    cb = VirtualQuery( (LPVOID)p2, &MemoryInformation, sizeof( MemoryInformation ) );
    printf( "VirtualQuery( %X ) = %u (%d)\n", p2, cb, GetLastError() );
    printf( "    BaseAddress:       %x\n", MemoryInformation.BaseAddress       );
    printf( "    AllocationBase:    %x\n", MemoryInformation.AllocationBase    );
    printf( "    AllocationProtect: %x\n", MemoryInformation.AllocationProtect );
    printf( "    RegionSize:        %u\n", MemoryInformation.RegionSize        );
    printf( "    State:             %x\n", MemoryInformation.State             );
    printf( "    Protect:           %x\n", MemoryInformation.Protect           );
    printf( "    Type:              %x\n", MemoryInformation.Type              );

    result = VirtualFree( (LPVOID)p2, 4096, MEM_RELEASE );
    printf( "VirtualFree( %X, 4096, MEM_RELEASE ) = %d\n", p2, result, GetLastError() );

    cb = VirtualQuery( (LPVOID)p2, &MemoryInformation, sizeof( MemoryInformation ) );
    printf( "VirtualQuery( %X ) = %u (%d)\n", p2, cb, GetLastError() );
    printf( "    BaseAddress:       %x\n", MemoryInformation.BaseAddress       );
    printf( "    AllocationBase:    %x\n", MemoryInformation.AllocationBase    );
    printf( "    AllocationProtect: %x\n", MemoryInformation.AllocationProtect );
    printf( "    RegionSize:        %u\n", MemoryInformation.RegionSize        );
    printf( "    State:             %x\n", MemoryInformation.State             );
    printf( "    Protect:           %x\n", MemoryInformation.Protect           );
    printf( "    Type:              %x\n", MemoryInformation.Type              );

    result = VirtualFree( (LPVOID)p1, 4096, MEM_RELEASE );
    printf( "VirtualFree( %X, 4096, MEM_RELEASE ) = %d\n", p1, result, GetLastError() );

    cb = VirtualQuery( (LPVOID)p1, &MemoryInformation, sizeof( MemoryInformation ) );
    printf( "VirtualQuery( %X ) = %u (%d)\n", p1, cb, GetLastError() );
    printf( "    BaseAddress:       %x\n", MemoryInformation.BaseAddress       );
    printf( "    AllocationBase:    %x\n", MemoryInformation.AllocationBase    );
    printf( "    AllocationProtect: %x\n", MemoryInformation.AllocationProtect );
    printf( "    RegionSize:        %u\n", MemoryInformation.RegionSize        );
    printf( "    State:             %x\n", MemoryInformation.State             );
    printf( "    Protect:           %x\n", MemoryInformation.Protect           );
    printf( "    Type:              %x\n", MemoryInformation.Type              );

#endif
