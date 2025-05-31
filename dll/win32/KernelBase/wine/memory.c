/*
 * Win32 memory management functions
 *
 * Copyright 1997 Alexandre Julliard
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "winerror.h"
#include "ddk/wdm.h"

#include "kernelbase.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(heap);
WINE_DECLARE_DEBUG_CHANNEL(virtual);
WINE_DECLARE_DEBUG_CHANNEL(globalmem);



static CROSS_PROCESS_WORK_LIST *open_cross_process_connection( HANDLE process )
{
#ifdef __aarch64__
    CROSS_PROCESS_WORK_LIST *list;
    HANDLE section;

    RtlOpenCrossProcessEmulatorWorkConnection( process, &section, (void **)&list );
    if (section) NtClose( section );
    return list;
#else
    return NULL;
#endif
}

static void close_cross_process_connection( CROSS_PROCESS_WORK_LIST *list )
{
    if (list) NtUnmapViewOfSection( GetCurrentProcess(), list );
}

static void send_cross_process_notification( CROSS_PROCESS_WORK_LIST *list, UINT id,
                                             const void *addr, SIZE_T size, int nb_args, ... )
{
#ifdef __aarch64__
    CROSS_PROCESS_WORK_ENTRY *entry;
    void *unused;
    va_list args;
    int i;

    if (!list) return;
    if ((entry = RtlWow64PopCrossProcessWorkFromFreeList( &list->free_list )))
    {
        entry->id = id;
        entry->addr = (ULONG_PTR)addr;
        entry->size = size;
        if (nb_args)
        {
            va_start( args, nb_args );
            for (i = 0; i < nb_args; i++) entry->args[i] = va_arg( args, int );
            va_end( args );
        }
        RtlWow64PushCrossProcessWorkOntoWorkList( &list->work_list, entry, &unused );
    }
#endif
}


/***********************************************************************
 * Virtual memory functions
 ***********************************************************************/

static const SIZE_T page_mask = 0xfff;
#define ROUND_ADDR(addr) ((void *)((UINT_PTR)(addr) & ~page_mask))
#define ROUND_SIZE(addr,size) (((SIZE_T)(size) + ((UINT_PTR)(addr) & page_mask) + page_mask) & ~page_mask)

/***********************************************************************
 *             DiscardVirtualMemory   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH DiscardVirtualMemory( void *addr, SIZE_T size )
{
    NTSTATUS status;
    LPVOID ret = addr;

    status = NtAllocateVirtualMemory( GetCurrentProcess(), &ret, 0, &size, MEM_RESET, PAGE_NOACCESS );
    return RtlNtStatusToDosError( status );
}


/***********************************************************************
 *             FlushViewOfFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FlushViewOfFile( const void *base, SIZE_T size )
{
    NTSTATUS status = NtFlushVirtualMemory( GetCurrentProcess(), &base, &size, 0 );

    if (status == STATUS_NOT_MAPPED_DATA) status = STATUS_SUCCESS;
    return set_ntstatus( status );
}


/****************************************************************************
 *           FlushInstructionCache   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FlushInstructionCache( HANDLE process, LPCVOID addr, SIZE_T size )
{
    CROSS_PROCESS_WORK_LIST *list;

    if ((list = open_cross_process_connection( process )))
    {
        send_cross_process_notification( list, CrossProcessFlushCache, addr, size, 0 );
        close_cross_process_connection( list );
    }
    return set_ntstatus( NtFlushInstructionCache( process, addr, size ));
}


/***********************************************************************
 *          GetLargePageMinimum   (kernelbase.@)
 */
SIZE_T WINAPI GetLargePageMinimum(void)
{
    return 2 * 1024 * 1024;
}


static void fill_system_info( SYSTEM_INFO *si, const SYSTEM_BASIC_INFORMATION *basic_info,
                              const SYSTEM_CPU_INFORMATION *cpu_info )
{
    si->wProcessorArchitecture      = cpu_info->ProcessorArchitecture;
    si->wReserved                   = 0;
    si->dwPageSize                  = basic_info->PageSize;
    si->lpMinimumApplicationAddress = basic_info->LowestUserAddress;
    si->lpMaximumApplicationAddress = basic_info->HighestUserAddress;
    si->dwActiveProcessorMask       = basic_info->ActiveProcessorsAffinityMask;
    si->dwNumberOfProcessors        = basic_info->NumberOfProcessors;
    si->dwAllocationGranularity     = basic_info->AllocationGranularity;
    si->wProcessorLevel             = cpu_info->ProcessorLevel;
    si->wProcessorRevision          = cpu_info->ProcessorRevision;

    switch (cpu_info->ProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
        switch (cpu_info->ProcessorLevel)
        {
        case 3:  si->dwProcessorType = PROCESSOR_INTEL_386;     break;
        case 4:  si->dwProcessorType = PROCESSOR_INTEL_486;     break;
        case 5:
        case 6:  si->dwProcessorType = PROCESSOR_INTEL_PENTIUM; break;
        default: si->dwProcessorType = PROCESSOR_INTEL_PENTIUM; break;
        }
        break;
    case PROCESSOR_ARCHITECTURE_AMD64:
        si->dwProcessorType = PROCESSOR_AMD_X8664;
        break;
    case PROCESSOR_ARCHITECTURE_ARM:
        switch (cpu_info->ProcessorLevel)
        {
        case 4:  si->dwProcessorType = PROCESSOR_ARM_7TDMI;     break;
        default: si->dwProcessorType = PROCESSOR_ARM920;
        }
        break;
    case PROCESSOR_ARCHITECTURE_ARM64:
        si->dwProcessorType = 0;
        break;
    default:
        FIXME( "Unknown processor architecture %x\n", cpu_info->ProcessorArchitecture );
        si->dwProcessorType = 0;
        break;
    }
}


/***********************************************************************
 *          GetNativeSystemInfo   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH GetNativeSystemInfo( SYSTEM_INFO *si )
{
    SYSTEM_BASIC_INFORMATION basic_info;
    SYSTEM_CPU_INFORMATION cpu_info;

    if (is_wow64)
    {
        USHORT current_machine, native_machine;

        RtlWow64GetProcessMachines( 0, &current_machine, &native_machine );
        if (native_machine != IMAGE_FILE_MACHINE_AMD64)
        {
            GetSystemInfo( si );
            si->wProcessorArchitecture = PROCESSOR_ARCHITECTURE_AMD64;
            return;
        }
    }

    if (!set_ntstatus( RtlGetNativeSystemInformation( SystemBasicInformation,
                                                      &basic_info, sizeof(basic_info), NULL )) ||
        !set_ntstatus( RtlGetNativeSystemInformation( SystemCpuInformation,
                                                      &cpu_info, sizeof(cpu_info), NULL )))
        return;

    fill_system_info( si, &basic_info, &cpu_info );
}


/***********************************************************************
 *          GetSystemInfo   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH GetSystemInfo( SYSTEM_INFO *si )
{
    SYSTEM_BASIC_INFORMATION basic_info;
    SYSTEM_CPU_INFORMATION cpu_info;

    if (!set_ntstatus( NtQuerySystemInformation( SystemBasicInformation,
                                                 &basic_info, sizeof(basic_info), NULL )) ||
        !set_ntstatus( NtQuerySystemInformation( SystemCpuInformation,
                                                 &cpu_info, sizeof(cpu_info), NULL )))
        return;

    fill_system_info( si, &basic_info, &cpu_info );
}


/***********************************************************************
 *          GetSystemFileCacheSize   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetSystemFileCacheSize( SIZE_T *mincache, SIZE_T *maxcache, DWORD *flags )
{
    FIXME( "stub: %p %p %p\n", mincache, maxcache, flags );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 *             GetWriteWatch   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetWriteWatch( DWORD flags, void *base, SIZE_T size, void **addresses,
                                             ULONG_PTR *count, ULONG *granularity )
{
    if (!set_ntstatus( NtGetWriteWatch( GetCurrentProcess(), flags, base, size,
                                        addresses, count, granularity )))
        return ~0u;
    return 0;
}


/***********************************************************************
 *             MapViewOfFile   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH MapViewOfFile( HANDLE mapping, DWORD access, DWORD offset_high,
                                               DWORD offset_low, SIZE_T count )
{
    return MapViewOfFileEx( mapping, access, offset_high, offset_low, count, NULL );
}


/***********************************************************************
 *             MapViewOfFileEx   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH MapViewOfFileEx( HANDLE handle, DWORD access, DWORD offset_high,
                                                 DWORD offset_low, SIZE_T count, LPVOID addr )
{
    NTSTATUS status;
    LARGE_INTEGER offset;
    ULONG protect;
    BOOL exec;

    offset.u.LowPart  = offset_low;
    offset.u.HighPart = offset_high;

    exec = access & FILE_MAP_EXECUTE;
    access &= ~FILE_MAP_EXECUTE;

    if (access == FILE_MAP_COPY)
        protect = exec ? PAGE_EXECUTE_WRITECOPY : PAGE_WRITECOPY;
    else if (access & FILE_MAP_WRITE)
        protect = exec ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
    else if (access & FILE_MAP_READ)
        protect = exec ? PAGE_EXECUTE_READ : PAGE_READONLY;
    else protect = PAGE_NOACCESS;

    if ((status = NtMapViewOfSection( handle, GetCurrentProcess(), &addr, 0, 0, &offset,
                                      &count, ViewShare, 0, protect )) < 0)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        addr = NULL;
    }
    return addr;
}


/***********************************************************************
 *             MapViewOfFileFromApp   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH MapViewOfFileFromApp( HANDLE handle, ULONG access, ULONG64 offset, SIZE_T size )
{
    return MapViewOfFile( handle, access, offset << 32, offset, size );
}

/***********************************************************************
 *             MapViewOfFile3   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH MapViewOfFile3( HANDLE handle, HANDLE process, PVOID baseaddr, ULONG64 offset,
        SIZE_T size, ULONG alloc_type, ULONG protection, MEM_EXTENDED_PARAMETER *params, ULONG params_count )
{
    LARGE_INTEGER off;
    void *addr;

    if (!process) process = GetCurrentProcess();

    addr = baseaddr;
    off.QuadPart = offset;
    if (!set_ntstatus( NtMapViewOfSectionEx( handle, process, &addr, &off, &size, alloc_type, protection,
            params, params_count )))
    {
        return NULL;
    }
    return addr;
}

/***********************************************************************
 *	       ReadProcessMemory   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReadProcessMemory( HANDLE process, const void *addr, void *buffer,
                                                 SIZE_T size, SIZE_T *bytes_read )
{
    return set_ntstatus( NtReadVirtualMemory( process, addr, buffer, size, bytes_read ));
}


/***********************************************************************
 *             ResetWriteWatch   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH ResetWriteWatch( void *base, SIZE_T size )
{
    if (!set_ntstatus( NtResetWriteWatch( GetCurrentProcess(), base, size )))
        return ~0u;
    return 0;
}


/***********************************************************************
 *          SetSystemFileCacheSize   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetSystemFileCacheSize( SIZE_T mincache, SIZE_T maxcache, DWORD flags )
{
    FIXME( "stub: %Id %Id %ld\n", mincache, maxcache, flags );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 *             UnmapViewOfFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH UnmapViewOfFile( const void *addr )
{
    if (GetVersion() & 0x80000000)
    {
        MEMORY_BASIC_INFORMATION info;
        if (!VirtualQuery( addr, &info, sizeof(info) ) || info.AllocationBase != addr)
        {
            SetLastError( ERROR_INVALID_ADDRESS );
            return FALSE;
        }
    }
    return set_ntstatus( NtUnmapViewOfSection( GetCurrentProcess(), (void *)addr ));
}


/***********************************************************************
 *             UnmapViewOfFile2   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH UnmapViewOfFile2( HANDLE process, void *addr, ULONG flags )
{
    return set_ntstatus( NtUnmapViewOfSectionEx( process, addr, flags ));
}


/***********************************************************************
 *             UnmapViewOfFileEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH UnmapViewOfFileEx( void *addr, ULONG flags )
{
    return set_ntstatus( NtUnmapViewOfSectionEx( GetCurrentProcess(), addr, flags ));
}


/***********************************************************************
 *             VirtualAlloc   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH VirtualAlloc( void *addr, SIZE_T size, DWORD type, DWORD protect )
{
    return VirtualAllocEx( GetCurrentProcess(), addr, size, type, protect );
}


/***********************************************************************
 *             VirtualAllocEx   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH VirtualAllocEx( HANDLE process, void *addr, SIZE_T size,
                                                DWORD type, DWORD protect )
{
    LPVOID ret = addr;

    if (!set_ntstatus( NtAllocateVirtualMemory( process, &ret, 0, &size, type, protect ))) return NULL;
    return ret;
}


/***********************************************************************
 *             VirtualAlloc2   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH VirtualAlloc2( HANDLE process, void *addr, SIZE_T size,
                                               DWORD type, DWORD protect,
                                               MEM_EXTENDED_PARAMETER *parameters, ULONG count )
{
    LPVOID ret = addr;

    if (!process) process = GetCurrentProcess();
    if (!set_ntstatus( NtAllocateVirtualMemoryEx( process, &ret, &size, type, protect, parameters, count )))
        return NULL;
    return ret;
}

static BOOL is_exec_prot( DWORD protect )
{
    return protect == PAGE_EXECUTE || protect == PAGE_EXECUTE_READ || protect == PAGE_EXECUTE_READWRITE
            || protect == PAGE_EXECUTE_WRITECOPY;
}

/***********************************************************************
 *             VirtualAlloc2FromApp   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH VirtualAlloc2FromApp( HANDLE process, void *addr, SIZE_T size,
        DWORD type, DWORD protect, MEM_EXTENDED_PARAMETER *parameters, ULONG count )
{
    LPVOID ret = addr;

    TRACE_(virtual)( "addr %p, size %p, type %#lx, protect %#lx, params %p, count %lu.\n", addr, (void *)size, type, protect,
            parameters, count );

    if (is_exec_prot( protect ))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    if (!process) process = GetCurrentProcess();
    if (!set_ntstatus( NtAllocateVirtualMemoryEx( process, &ret, &size, type, protect, parameters, count )))
        return NULL;
    return ret;
}


/***********************************************************************
 *             VirtualAllocFromApp   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH VirtualAllocFromApp( void *addr, SIZE_T size,
                                                DWORD type, DWORD protect )
{
    LPVOID ret = addr;

    TRACE_(virtual)( "addr %p, size %p, type %#lx, protect %#lx.\n", addr, (void *)size, type, protect );

    if (is_exec_prot( protect ))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    if (!set_ntstatus( NtAllocateVirtualMemory( GetCurrentProcess(), &ret, 0, &size, type, protect ))) return NULL;
    return ret;
}


/***********************************************************************
 *             PrefetchVirtualMemory   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH PrefetchVirtualMemory( HANDLE process, ULONG_PTR count,
                                                     WIN32_MEMORY_RANGE_ENTRY *addresses, ULONG flags )
{
    return set_ntstatus( NtSetInformationVirtualMemory( process, VmPrefetchInformation,
                                                        count, (PMEMORY_RANGE_ENTRY)addresses,
                                                        &flags, sizeof(flags) ));
}


/***********************************************************************
 *             VirtualFree   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH VirtualFree( void *addr, SIZE_T size, DWORD type )
{
    return VirtualFreeEx( GetCurrentProcess(), addr, size, type );
}


/***********************************************************************
 *             VirtualFreeEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH VirtualFreeEx( HANDLE process, void *addr, SIZE_T size, DWORD type )
{
    if (type == MEM_RELEASE && size)
    {
        WARN( "Trying to release memory with specified size.\n" );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return set_ntstatus( NtFreeVirtualMemory( process, &addr, &size, type ));
}


/***********************************************************************
 *             VirtualLock   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH  VirtualLock( void *addr, SIZE_T size )
{
    return set_ntstatus( NtLockVirtualMemory( GetCurrentProcess(), &addr, &size, 1 ));
}


/***********************************************************************
 *             VirtualProtect   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH VirtualProtect( void *addr, SIZE_T size, DWORD new_prot, DWORD *old_prot )
{
    return VirtualProtectEx( GetCurrentProcess(), addr, size, new_prot, old_prot );
}


/***********************************************************************
 *             VirtualProtectEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH VirtualProtectEx( HANDLE process, void *addr, SIZE_T size,
                                                DWORD new_prot, DWORD *old_prot )
{
    DWORD prot;

    /* Win9x allows passing NULL as old_prot while this fails on NT */
    if (!old_prot && (GetVersion() & 0x80000000)) old_prot = &prot;
    return set_ntstatus( NtProtectVirtualMemory( process, &addr, &size, new_prot, old_prot ));
}


/***********************************************************************
 *             VirtualQuery   (kernelbase.@)
 */
SIZE_T WINAPI DECLSPEC_HOTPATCH VirtualQuery( LPCVOID addr, PMEMORY_BASIC_INFORMATION info, SIZE_T len )
{
    return VirtualQueryEx( GetCurrentProcess(), addr, info, len );
}


/***********************************************************************
 *             VirtualQueryEx   (kernelbase.@)
 */
SIZE_T WINAPI DECLSPEC_HOTPATCH VirtualQueryEx( HANDLE process, LPCVOID addr,
                                                PMEMORY_BASIC_INFORMATION info, SIZE_T len )
{
    SIZE_T ret;

    if (!set_ntstatus( NtQueryVirtualMemory( process, addr, MemoryBasicInformation, info, len, &ret )))
        return 0;
    return ret;
}


/***********************************************************************
 *             VirtualUnlock   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH VirtualUnlock( void *addr, SIZE_T size )
{
    return set_ntstatus( NtUnlockVirtualMemory( GetCurrentProcess(), &addr, &size, 1 ));
}


/***********************************************************************
 *             WriteProcessMemory    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteProcessMemory( HANDLE process, void *addr, const void *buffer,
                                                  SIZE_T size, SIZE_T *bytes_written )
{
    CROSS_PROCESS_WORK_LIST *list = open_cross_process_connection( process );
    DWORD old_prot, prot = PAGE_TARGETS_NO_UPDATE | PAGE_ENCLAVE_NO_CHANGE;
    MEMORY_BASIC_INFORMATION info;
    void *base_addr;
    SIZE_T region_size;
    NTSTATUS status, status2;

    if (!VirtualQueryEx( process, addr, &info, sizeof(info) ))
    {
        close_cross_process_connection( list );
        return FALSE;
    }

    switch (info.Protect & ~(PAGE_GUARD | PAGE_NOCACHE))
    {
    case PAGE_READWRITE:
    case PAGE_WRITECOPY:
    case PAGE_EXECUTE_READWRITE:
    case PAGE_EXECUTE_WRITECOPY:
        /* already writable */
        if ((status = NtWriteVirtualMemory( process, addr, buffer, size, bytes_written ))) break;
        send_cross_process_notification( list, CrossProcessFlushCache, addr, size, 0 );
        NtFlushInstructionCache( process, addr, size );
        break;

    case PAGE_EXECUTE:
    case PAGE_EXECUTE_READ:
        /* make it writable */
        base_addr = ROUND_ADDR( addr );
        region_size = ROUND_SIZE( addr, size );
        region_size = min( region_size,  (char *)info.BaseAddress + info.RegionSize - (char *)base_addr );
        prot |= (info.Type == MEM_PRIVATE) ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_WRITECOPY;

        send_cross_process_notification( list, CrossProcessPreVirtualProtect,
                                         base_addr, region_size, 1, prot );
        status = NtProtectVirtualMemory( process, &base_addr, &region_size, prot, &old_prot );
        send_cross_process_notification( list, CrossProcessPostVirtualProtect,
                                         base_addr, region_size, 2, prot, status );
        if (status) break;

        status = NtWriteVirtualMemory( process, addr, buffer, size, bytes_written );
        if (!status)
        {
            send_cross_process_notification( list, CrossProcessFlushCache, addr, size, 0 );
            NtFlushInstructionCache( process, addr, size );
        }

        prot = PAGE_TARGETS_NO_UPDATE | PAGE_ENCLAVE_NO_CHANGE | old_prot;
        send_cross_process_notification( list, CrossProcessPreVirtualProtect,
                                         base_addr, region_size, 1, prot );
        status2 = NtProtectVirtualMemory( process, &base_addr, &region_size, prot, &old_prot );
        send_cross_process_notification( list, CrossProcessPostVirtualProtect,
                                         base_addr, region_size, 2, prot, status2 );
        break;

    default:
        /* not writable */
        status = STATUS_ACCESS_VIOLATION;
        break;
    }

    close_cross_process_connection( list );
    return set_ntstatus( status );
}


/* IsBadStringPtrA replacement for kernelbase, to catch exception in debug traces. */
BOOL WINAPI IsBadStringPtrA( LPCSTR str, UINT_PTR max )
{
    if (!str) return TRUE;
    __TRY
    {
        volatile const char *p = str;
        while (p != str + max) if (!*p++) break;
    }
    __EXCEPT_PAGE_FAULT
    {
        return TRUE;
    }
    __ENDTRY
    return FALSE;
}


/* IsBadStringPtrW replacement for kernelbase, to catch exception in debug traces. */
BOOL WINAPI IsBadStringPtrW( LPCWSTR str, UINT_PTR max )
{
    if (!str) return TRUE;
    __TRY
    {
        volatile const WCHAR *p = str;
        while (p != str + max) if (!*p++) break;
    }
    __EXCEPT_PAGE_FAULT
    {
        return TRUE;
    }
    __ENDTRY
    return FALSE;
}


/***********************************************************************
 * Heap functions
 ***********************************************************************/


/***********************************************************************
 *           HeapCompact   (kernelbase.@)
 */
SIZE_T WINAPI DECLSPEC_HOTPATCH HeapCompact( HANDLE heap, DWORD flags )
{
    return RtlCompactHeap( heap, flags );
}


/***********************************************************************
 *           HeapCreate   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH HeapCreate( DWORD flags, SIZE_T init_size, SIZE_T max_size )
{
    HANDLE ret = RtlCreateHeap( flags, NULL, max_size, init_size, NULL, NULL );
    if (!ret) SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    return ret;
}


/***********************************************************************
 *           HeapDestroy   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH HeapDestroy( HANDLE heap )
{
    if (!RtlDestroyHeap( heap )) return TRUE;
    SetLastError( ERROR_INVALID_HANDLE );
    return FALSE;
}


/***********************************************************************
 *           HeapLock   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH HeapLock( HANDLE heap )
{
    return RtlLockHeap( heap );
}


/***********************************************************************
 *           HeapQueryInformation   (kernelbase.@)
 */
BOOL WINAPI HeapQueryInformation( HANDLE heap, HEAP_INFORMATION_CLASS info_class,
                                  PVOID info, SIZE_T size, PSIZE_T size_out )
{
    return set_ntstatus( RtlQueryHeapInformation( heap, info_class, info, size, size_out ));
}


/***********************************************************************
 *           HeapSetInformation   (kernelbase.@)
 */
BOOL WINAPI HeapSetInformation( HANDLE heap, HEAP_INFORMATION_CLASS infoclass, PVOID info, SIZE_T size )
{
    return set_ntstatus( RtlSetHeapInformation( heap, infoclass, info, size ));
}


/***********************************************************************
 *           HeapUnlock   (kernelbase.@)
 */
BOOL WINAPI HeapUnlock( HANDLE heap )
{
    return RtlUnlockHeap( heap );
}


/***********************************************************************
 *           HeapValidate   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH HeapValidate( HANDLE heap, DWORD flags, LPCVOID ptr )
{
    return RtlValidateHeap( heap, flags, ptr );
}


/* undocumented RtlWalkHeap structure */

struct rtl_heap_entry
{
    LPVOID lpData;
    SIZE_T cbData; /* differs from PROCESS_HEAP_ENTRY */
    BYTE cbOverhead;
    BYTE iRegionIndex;
    WORD wFlags; /* value differs from PROCESS_HEAP_ENTRY */
    union {
        struct {
            HANDLE hMem;
            DWORD dwReserved[3];
        } Block;
        struct {
            DWORD dwCommittedSize;
            DWORD dwUnCommittedSize;
            LPVOID lpFirstBlock;
            LPVOID lpLastBlock;
        } Region;
    };
};

/* rtl_heap_entry flags, names made up */

#define RTL_HEAP_ENTRY_BUSY         0x0001
#define RTL_HEAP_ENTRY_REGION       0x0002
#define RTL_HEAP_ENTRY_BLOCK        0x0010
#define RTL_HEAP_ENTRY_UNCOMMITTED  0x1000
#define RTL_HEAP_ENTRY_COMMITTED    0x4000
#define RTL_HEAP_ENTRY_LFH          0x8000


/***********************************************************************
 *           HeapWalk   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH HeapWalk( HANDLE heap, PROCESS_HEAP_ENTRY *entry )
{
    struct rtl_heap_entry rtl_entry = {0};
    NTSTATUS status;

    if (!entry) return set_ntstatus( STATUS_INVALID_PARAMETER );

    rtl_entry.lpData = entry->lpData;
    rtl_entry.cbData = entry->cbData;
    rtl_entry.cbOverhead = entry->cbOverhead;
    rtl_entry.iRegionIndex = entry->iRegionIndex;

    if (entry->wFlags & PROCESS_HEAP_ENTRY_BUSY)
        rtl_entry.wFlags |= RTL_HEAP_ENTRY_BUSY;
    if (entry->wFlags & PROCESS_HEAP_REGION)
        rtl_entry.wFlags |= RTL_HEAP_ENTRY_REGION;
    if (entry->wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE)
        rtl_entry.wFlags |= RTL_HEAP_ENTRY_UNCOMMITTED;
    memcpy( &rtl_entry.Region, &entry->Region, sizeof(entry->Region) );

    if (!(status = RtlWalkHeap( heap, &rtl_entry )))
    {
        entry->lpData = rtl_entry.lpData;
        entry->cbData = rtl_entry.cbData;
        entry->cbOverhead = rtl_entry.cbOverhead;
        entry->iRegionIndex = rtl_entry.iRegionIndex;

        if (rtl_entry.wFlags & RTL_HEAP_ENTRY_BUSY)
            entry->wFlags = PROCESS_HEAP_ENTRY_BUSY;
        else if (rtl_entry.wFlags & RTL_HEAP_ENTRY_REGION)
            entry->wFlags = PROCESS_HEAP_REGION;
        else if (rtl_entry.wFlags & RTL_HEAP_ENTRY_UNCOMMITTED)
            entry->wFlags = PROCESS_HEAP_UNCOMMITTED_RANGE;
        else
            entry->wFlags = 0;

        memcpy( &entry->Region, &rtl_entry.Region, sizeof(entry->Region) );
    }

    return set_ntstatus( status );
}


/***********************************************************************
 * Global/local heap functions
 ***********************************************************************/

/* some undocumented flags (names are made up) */
#define HEAP_ADD_USER_INFO    0x00000100

/* not compatible with windows */
struct kernelbase_global_data
{
    struct mem_entry *mem_entries;
    struct mem_entry *mem_entries_end;
};

#define MEM_FLAG_USED        1
#define MEM_FLAG_MOVEABLE    2
#define MEM_FLAG_DISCARDABLE 4
#define MEM_FLAG_DISCARDED   8
#define MEM_FLAG_DDESHARE    0x8000

struct mem_entry
{
    union
    {
        struct
        {
            WORD flags;
            BYTE lock;
        };
        void *next_free;
    };
    void *ptr;
};

C_ASSERT(sizeof(struct mem_entry) == 2 * sizeof(void *));

#define MAX_MEM_HANDLES  0x10000
static struct mem_entry *next_free_mem;
static struct kernelbase_global_data global_data = {0};

static inline struct mem_entry *unsafe_mem_from_HLOCAL( HLOCAL handle )
{
    struct mem_entry *mem = CONTAINING_RECORD( *(volatile HANDLE *)&handle, struct mem_entry, ptr );
    struct kernelbase_global_data *data = &global_data;
    if (((UINT_PTR)handle & ((sizeof(void *) << 1) - 1)) != sizeof(void *)) return NULL;
    if (mem < data->mem_entries || mem >= data->mem_entries_end) return NULL;
    if (!(mem->flags & MEM_FLAG_USED)) return NULL;
    return mem;
}

static inline HLOCAL HLOCAL_from_mem( struct mem_entry *mem )
{
    if (!mem) return 0;
    return &mem->ptr;
}

static inline void *unsafe_ptr_from_HLOCAL( HLOCAL handle )
{
    if (((UINT_PTR)handle & ((sizeof(void *) << 1) - 1))) return NULL;
    return handle;
}

void init_global_data(void)
{
    global_data.mem_entries = VirtualAlloc( NULL, MAX_MEM_HANDLES * sizeof(struct mem_entry), MEM_COMMIT, PAGE_READWRITE );
    if (!(next_free_mem = global_data.mem_entries)) ERR( "Failed to allocate kernelbase global handle table\n" );
    global_data.mem_entries_end = global_data.mem_entries + MAX_MEM_HANDLES;
}

/***********************************************************************
 *           KernelBaseGetGlobalData   (kernelbase.@)
 */
void *WINAPI KernelBaseGetGlobalData(void)
{
    WARN_(globalmem)( "semi-stub!\n" );
    return &global_data;
}


/***********************************************************************
 *           GlobalAlloc   (kernelbase.@)
 */
HGLOBAL WINAPI DECLSPEC_HOTPATCH GlobalAlloc( UINT flags, SIZE_T size )
{
    struct mem_entry *mem;
    HGLOBAL handle;

    /* LocalAlloc allows a 0-size fixed block, but GlobalAlloc doesn't */
    if (!(flags & GMEM_MOVEABLE) && !size) size = 1;

    handle = LocalAlloc( flags, size );

    if ((mem = unsafe_mem_from_HLOCAL( handle )) && (flags & GMEM_DDESHARE))
        mem->flags |= MEM_FLAG_DDESHARE;

    return handle;
}


/***********************************************************************
 *           GlobalFree   (kernelbase.@)
 */
HGLOBAL WINAPI DECLSPEC_HOTPATCH GlobalFree( HLOCAL handle )
{
    return LocalFree( handle );
}


/***********************************************************************
 *           LocalAlloc   (kernelbase.@)
 */
HLOCAL WINAPI DECLSPEC_HOTPATCH LocalAlloc( UINT flags, SIZE_T size )
{
    DWORD heap_flags = 0x200 | HEAP_ADD_USER_INFO;
    HANDLE heap = GetProcessHeap();
    struct mem_entry *mem;
    HLOCAL handle;
    void *ptr;

    TRACE_(globalmem)( "flags %#x, size %#Ix\n", flags, size );

    if (flags & LMEM_ZEROINIT) heap_flags |= HEAP_ZERO_MEMORY;

    if (!(flags & LMEM_MOVEABLE)) /* pointer */
    {
        ptr = HeapAlloc( heap, heap_flags, size );
        if (ptr) RtlSetUserValueHeap( heap, heap_flags, ptr, ptr );
        TRACE_(globalmem)( "return %p\n", ptr );
        return ptr;
    }

    RtlLockHeap( heap );
    if ((mem = next_free_mem) < global_data.mem_entries || mem >= global_data.mem_entries_end)
        mem = NULL;
    else
    {
        if (!mem->next_free) next_free_mem++;
        else next_free_mem = mem->next_free;
        mem->next_free = NULL;
    }
    RtlUnlockHeap( heap );

    if (!mem) goto failed;
    handle = HLOCAL_from_mem( mem );

    mem->flags = MEM_FLAG_USED | MEM_FLAG_MOVEABLE;
    if (flags & LMEM_DISCARDABLE) mem->flags |= MEM_FLAG_DISCARDABLE;
    mem->lock  = 0;
    mem->ptr   = NULL;

    if (!size) mem->flags |= MEM_FLAG_DISCARDED;
    else
    {
        if (!(ptr = HeapAlloc( heap, heap_flags, size ))) goto failed;
        RtlSetUserValueHeap( heap, heap_flags, ptr, handle );
        mem->ptr = ptr;
    }

    TRACE_(globalmem)( "return handle %p, ptr %p\n", handle, mem->ptr );
    return handle;

failed:
    if (mem) LocalFree( *(volatile HANDLE *)&handle );
    SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    return 0;
}


/***********************************************************************
 *           LocalFree   (kernelbase.@)
 */
HLOCAL WINAPI DECLSPEC_HOTPATCH LocalFree( HLOCAL handle )
{
    HANDLE heap = GetProcessHeap();
    struct mem_entry *mem;
    HLOCAL ret = handle;
    void *ptr;

    TRACE_(globalmem)( "handle %p\n", handle );

    RtlLockHeap( heap );
    if ((ptr = unsafe_ptr_from_HLOCAL( handle )) &&
        HeapValidate( heap, HEAP_NO_SERIALIZE, ptr ))
    {
        if (HeapFree( heap, HEAP_NO_SERIALIZE, ptr )) ret = 0;
    }
    else if ((mem = unsafe_mem_from_HLOCAL( handle )))
    {
        if (HeapFree( heap, HEAP_NO_SERIALIZE, mem->ptr )) ret = 0;
        mem->ptr = NULL;
        mem->next_free = next_free_mem;
        next_free_mem = mem;
    }
    RtlUnlockHeap( heap );

    if (ret)
    {
        WARN_(globalmem)( "invalid handle %p\n", handle );
        SetLastError( ERROR_INVALID_HANDLE );
    }
    return ret;
}


/***********************************************************************
 *           LocalLock   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH LocalLock( HLOCAL handle )
{
    HANDLE heap = GetProcessHeap();
    struct mem_entry *mem;
    void *ret = NULL;

    TRACE_(globalmem)( "handle %p\n", handle );

    if (!handle) return NULL;
    if ((ret = unsafe_ptr_from_HLOCAL( handle )))
    {
        __TRY
        {
            volatile char *p = ret;
            *p |= 0;
        }
        __EXCEPT_PAGE_FAULT
        {
            return NULL;
        }
        __ENDTRY
        return ret;
    }

    RtlLockHeap( heap );
    if ((mem = unsafe_mem_from_HLOCAL( handle )))
    {
        if (!(ret = mem->ptr)) SetLastError( ERROR_DISCARDED );
        else if (!++mem->lock) mem->lock--;
    }
    else
    {
        WARN_(globalmem)( "invalid handle %p\n", handle );
        SetLastError( ERROR_INVALID_HANDLE );
    }
    RtlUnlockHeap( heap );

    return ret;
}


/***********************************************************************
 *           LocalReAlloc   (kernelbase.@)
 */
HLOCAL WINAPI DECLSPEC_HOTPATCH LocalReAlloc( HLOCAL handle, SIZE_T size, UINT flags )
{
    DWORD heap_flags = 0x200 | HEAP_ADD_USER_INFO | HEAP_NO_SERIALIZE;
    HANDLE heap = GetProcessHeap();
    struct mem_entry *mem;
    HLOCAL ret = 0;
    void *ptr;

    TRACE_(globalmem)( "handle %p, size %#Ix, flags %#x\n", handle, size, flags );

    if (flags & LMEM_ZEROINIT) heap_flags |= HEAP_ZERO_MEMORY;

    RtlLockHeap( heap );
    if ((ptr = unsafe_ptr_from_HLOCAL( handle )) &&
        HeapValidate( heap, HEAP_NO_SERIALIZE, ptr ))
    {
        if (flags & LMEM_MODIFY) ret = handle;
        else if (flags & LMEM_DISCARDABLE) SetLastError( ERROR_INVALID_PARAMETER );
        else
        {
            if (!(flags & LMEM_MOVEABLE)) heap_flags |= HEAP_REALLOC_IN_PLACE_ONLY;
            ret = HeapReAlloc( heap, heap_flags, ptr, size );
            if (ret) RtlSetUserValueHeap( heap, heap_flags, ret, ret );
            else SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        }
    }
    else if ((mem = unsafe_mem_from_HLOCAL( handle )))
    {
        if (flags & LMEM_MODIFY)
        {
            if (flags & LMEM_DISCARDABLE) mem->flags |= MEM_FLAG_DISCARDABLE;
            ret = handle;
        }
        else if (flags & LMEM_DISCARDABLE) SetLastError( ERROR_INVALID_PARAMETER );
        else
        {
            if (size)
            {
                if (mem->lock && !(flags & LMEM_MOVEABLE)) heap_flags |= HEAP_REALLOC_IN_PLACE_ONLY;
                if (!mem->ptr) ptr = HeapAlloc( heap, heap_flags, size );
                else ptr = HeapReAlloc( heap, heap_flags, mem->ptr, size );

                if (!ptr) SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                else
                {
                    RtlSetUserValueHeap( heap, heap_flags, ptr, handle );
                    mem->flags &= ~MEM_FLAG_DISCARDED;
                    mem->ptr = ptr;
                    ret = handle;
                }
            }
            else if ((flags & LMEM_MOVEABLE) && !mem->lock)
            {
                HeapFree( heap, heap_flags, mem->ptr );
                mem->flags |= MEM_FLAG_DISCARDED;
                mem->ptr = NULL;
                ret = handle;
            }
            else SetLastError( ERROR_INVALID_PARAMETER );
        }
    }
    else SetLastError( ERROR_INVALID_HANDLE );
    RtlUnlockHeap( heap );

    return ret;
}


/***********************************************************************
 *           LocalUnlock   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH LocalUnlock( HLOCAL handle )
{
    HANDLE heap = GetProcessHeap();
    struct mem_entry *mem;
    BOOL ret = FALSE;

    TRACE_(globalmem)( "handle %p\n", handle );

    if (unsafe_ptr_from_HLOCAL( handle ))
    {
        SetLastError( ERROR_NOT_LOCKED );
        return FALSE;
    }

    RtlLockHeap( heap );
    if ((mem = unsafe_mem_from_HLOCAL( handle )))
    {
        if (mem->lock)
        {
            ret = (--mem->lock != 0);
            if (!ret) SetLastError( NO_ERROR );
        }
        else
        {
            WARN_(globalmem)( "handle %p not locked\n", handle );
            SetLastError( ERROR_NOT_LOCKED );
        }
    }
    else
    {
        WARN_(globalmem)( "invalid handle %p\n", handle );
        SetLastError( ERROR_INVALID_HANDLE );
    }
    RtlUnlockHeap( heap );

    return ret;
}


/***********************************************************************
 * Memory resource functions
 ***********************************************************************/


/***********************************************************************
 *           CreateMemoryResourceNotification   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateMemoryResourceNotification( MEMORY_RESOURCE_NOTIFICATION_TYPE type )
{
    HANDLE ret;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;

    switch (type)
    {
    case LowMemoryResourceNotification:
        RtlInitUnicodeString( &nameW, L"\\KernelObjects\\LowMemoryCondition" );
        break;
    case HighMemoryResourceNotification:
        RtlInitUnicodeString( &nameW, L"\\KernelObjects\\HighMemoryCondition" );
        break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    InitializeObjectAttributes( &attr, &nameW, 0, 0, NULL );
    if (!set_ntstatus( NtOpenEvent( &ret, EVENT_ALL_ACCESS, &attr ))) return 0;
    return ret;
}

/***********************************************************************
 *          QueryMemoryResourceNotification   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryMemoryResourceNotification( HANDLE handle, BOOL *state )
{
    switch (WaitForSingleObject( handle, 0 ))
    {
    case WAIT_OBJECT_0:
        *state = TRUE;
        return TRUE;
    case WAIT_TIMEOUT:
        *state = FALSE;
        return TRUE;
    }
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
}


/***********************************************************************
 * Physical memory functions
 ***********************************************************************/


/***********************************************************************
 *             AllocateUserPhysicalPages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH AllocateUserPhysicalPages( HANDLE process, ULONG_PTR *pages,
                                                         ULONG_PTR *userarray )
{
    FIXME( "stub: %p %p %p\n", process, pages, userarray );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 *             FreeUserPhysicalPages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FreeUserPhysicalPages( HANDLE process, ULONG_PTR *pages,
                                                     ULONG_PTR *userarray )
{
    FIXME( "stub: %p %p %p\n", process, pages, userarray );
    *pages = 0;
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 *             GetPhysicallyInstalledSystemMemory   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetPhysicallyInstalledSystemMemory( ULONGLONG *memory )
{
    MEMORYSTATUSEX status;

    if (!memory)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx( &status );
    *memory = status.ullTotalPhys / 1024;
    return TRUE;
}


/***********************************************************************
 *             GlobalMemoryStatusEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GlobalMemoryStatusEx( MEMORYSTATUSEX *status )
{
    static MEMORYSTATUSEX cached_status;
    static DWORD last_check;
    SYSTEM_BASIC_INFORMATION basic_info;
    SYSTEM_PERFORMANCE_INFORMATION perf_info;
    VM_COUNTERS_EX vmc;

    if (status->dwLength != sizeof(*status))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if ((NtGetTickCount() - last_check) < 1000)
    {
	*status = cached_status;
	return TRUE;
    }
    last_check = NtGetTickCount();

    if (!set_ntstatus( NtQuerySystemInformation( SystemBasicInformation,
                                                 &basic_info, sizeof(basic_info), NULL )) ||
        !set_ntstatus( NtQuerySystemInformation( SystemPerformanceInformation,
                                                 &perf_info, sizeof(perf_info), NULL)) ||
        !set_ntstatus( NtQueryInformationProcess( GetCurrentProcess(), ProcessVmCounters,
                                                  &vmc, sizeof(vmc), NULL )))
        return FALSE;

    status->dwMemoryLoad     = 0;
    status->ullTotalPhys     = basic_info.MmNumberOfPhysicalPages;
    status->ullAvailPhys     = perf_info.AvailablePages;
    status->ullTotalPageFile = perf_info.TotalCommitLimit;
    status->ullAvailPageFile = status->ullTotalPageFile - perf_info.TotalCommittedPages;
    status->ullTotalVirtual  = (ULONG_PTR)basic_info.HighestUserAddress - (ULONG_PTR)basic_info.LowestUserAddress + 1;
    status->ullAvailVirtual  = status->ullTotalVirtual - (ULONGLONG)vmc.WorkingSetSize /* approximate */;
    status->ullAvailExtendedVirtual = 0;

    status->ullTotalPhys     *= basic_info.PageSize;
    status->ullAvailPhys     *= basic_info.PageSize;
    status->ullTotalPageFile *= basic_info.PageSize;
    status->ullAvailPageFile *= basic_info.PageSize;

    if (status->ullTotalPhys)
        status->dwMemoryLoad = (status->ullTotalPhys - status->ullAvailPhys) / (status->ullTotalPhys / 100);

    TRACE_(virtual)( "MemoryLoad %lu, TotalPhys %I64u, AvailPhys %I64u, TotalPageFile %I64u, "
                     "AvailPageFile %I64u, TotalVirtual %I64u, AvailVirtual %I64u\n",
                     status->dwMemoryLoad, status->ullTotalPhys, status->ullAvailPhys, status->ullTotalPageFile,
                     status->ullAvailPageFile, status->ullTotalVirtual, status->ullAvailVirtual );

    cached_status = *status;
    return TRUE;
}


/***********************************************************************
 *             MapUserPhysicalPages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH MapUserPhysicalPages( void *addr, ULONG_PTR page_count, ULONG_PTR *pages )
{
    FIXME( "stub: %p %Iu %p\n", addr, page_count, pages );
    *pages = 0;
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 * NUMA functions
 ***********************************************************************/


/***********************************************************************
 *             AllocateUserPhysicalPagesNuma   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH AllocateUserPhysicalPagesNuma( HANDLE process, ULONG_PTR *pages,
                                                             ULONG_PTR *userarray, DWORD node )
{
    if (node) FIXME( "Ignoring preferred node %lu\n", node );
    return AllocateUserPhysicalPages( process, pages, userarray );
}


/***********************************************************************
 *             CreateFileMappingNumaW   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateFileMappingNumaW( HANDLE file, LPSECURITY_ATTRIBUTES sa,
                                                        DWORD protect, DWORD size_high, DWORD size_low,
                                                        LPCWSTR name, DWORD node )
{
    if (node) FIXME( "Ignoring preferred node %lu\n", node );
    return CreateFileMappingW( file, sa, protect, size_high, size_low, name );
}


/***********************************************************************
 *           GetLogicalProcessorInformation   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetLogicalProcessorInformation( SYSTEM_LOGICAL_PROCESSOR_INFORMATION *buffer,
                                                              DWORD *len )
{
    NTSTATUS status;

    if (!len)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    status = NtQuerySystemInformation( SystemLogicalProcessorInformation, buffer, *len, len );
    if (status == STATUS_INFO_LENGTH_MISMATCH) status = STATUS_BUFFER_TOO_SMALL;
    return set_ntstatus( status );
}


/***********************************************************************
 *           GetLogicalProcessorInformationEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetLogicalProcessorInformationEx( LOGICAL_PROCESSOR_RELATIONSHIP relationship,
                                            SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buffer, DWORD *len )
{
    NTSTATUS status;

    if (!len)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    status = NtQuerySystemInformationEx( SystemLogicalProcessorInformationEx, &relationship,
                                         sizeof(relationship), buffer, *len, len );
    if (status == STATUS_INFO_LENGTH_MISMATCH) status = STATUS_BUFFER_TOO_SMALL;
    return set_ntstatus( status );
}


/***********************************************************************
 *           GetSystemCpuSetInformation   (kernelbase.@)
 */
BOOL WINAPI GetSystemCpuSetInformation(SYSTEM_CPU_SET_INFORMATION *info, ULONG buffer_length, ULONG *return_length,
                                            HANDLE process, ULONG flags)
{
    if (flags)
        FIXME("Unsupported flags %#lx.\n", flags);

    *return_length = 0;

    return set_ntstatus( NtQuerySystemInformationEx( SystemCpuSetInformation, &process, sizeof(process), info,
            buffer_length, return_length ));
}


/***********************************************************************
 *           SetThreadSelectedCpuSets   (kernelbase.@)
 */
BOOL WINAPI SetThreadSelectedCpuSets(HANDLE thread, const ULONG *cpu_set_ids, ULONG count)
{
    FIXME( "thread %p, cpu_set_ids %p, count %lu stub.\n", thread, cpu_set_ids, count );

    return TRUE;
}


/***********************************************************************
 *           SetProcessDefaultCpuSets   (kernelbase.@)
 */
BOOL WINAPI SetProcessDefaultCpuSets(HANDLE process, const ULONG *cpu_set_ids, ULONG count)
{
    FIXME( "process %p, cpu_set_ids %p, count %lu stub.\n", process, cpu_set_ids, count );

    return TRUE;
}


/**********************************************************************
 *             GetNumaHighestNodeNumber   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetNumaHighestNodeNumber( ULONG *node )
{
    FIXME( "semi-stub: %p\n", node );
    *node = 0;
    return TRUE;
}


/**********************************************************************
 *             GetNumaNodeProcessorMaskEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetNumaNodeProcessorMaskEx( USHORT node, GROUP_AFFINITY *mask )
{
    FIXME( "stub: %hu %p\n", node, mask );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 *             GetNumaProximityNodeEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetNumaProximityNodeEx( ULONG proximity_id, USHORT *node )
{
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 *             MapViewOfFileExNuma   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH MapViewOfFileExNuma( HANDLE handle, DWORD access, DWORD offset_high,
                                                     DWORD offset_low, SIZE_T count, LPVOID addr,
                                                     DWORD node )
{
    if (node) FIXME( "Ignoring preferred node %lu\n", node );
    return MapViewOfFileEx( handle, access, offset_high, offset_low, count, addr );
}


/***********************************************************************
 *             VirtualAllocExNuma   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH VirtualAllocExNuma( HANDLE process, void *addr, SIZE_T size,
                                                    DWORD type, DWORD protect, DWORD node )
{
    if (node) FIXME( "Ignoring preferred node %lu\n", node );
    return VirtualAllocEx( process, addr, size, type, protect );
}


/***********************************************************************
 *             QueryVirtualMemoryInformation   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryVirtualMemoryInformation( HANDLE process, const void *addr,
        WIN32_MEMORY_INFORMATION_CLASS info_class, void *info, SIZE_T size, SIZE_T *ret_size)
{
    switch (info_class)
    {
        case MemoryRegionInfo:
            return set_ntstatus( NtQueryVirtualMemory( process, addr, MemoryRegionInformation, info, size, ret_size ));
        default:
            FIXME("Unsupported info class %u.\n", info_class);
            return FALSE;
    }
}


/***********************************************************************
 * CPU functions
 ***********************************************************************/


/***********************************************************************
 *             InitializeContext2         (kernelbase.@)
 */
BOOL WINAPI InitializeContext2( void *buffer, DWORD context_flags, CONTEXT **context, DWORD *length,
        ULONG64 compaction_mask )
{
    ULONG orig_length;
    NTSTATUS status;

    TRACE( "buffer %p, context_flags %#lx, context %p, ret_length %p, compaction_mask %s.\n",
            buffer, context_flags, context, length, wine_dbgstr_longlong(compaction_mask) );

    orig_length = *length;

    if ((status = RtlGetExtendedContextLength2( context_flags, length, compaction_mask )))
    {
        if (status == STATUS_NOT_SUPPORTED && context_flags & 0x40)
        {
            context_flags &= ~0x40;
            status = RtlGetExtendedContextLength2( context_flags, length, compaction_mask );
        }

        if (status)
            return set_ntstatus( status );
    }

    if (!buffer || orig_length < *length)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    if ((status = RtlInitializeExtendedContext2( buffer, context_flags, (CONTEXT_EX **)context, compaction_mask )))
        return set_ntstatus( status );

    *context = (CONTEXT *)((BYTE *)*context + (*(CONTEXT_EX **)context)->Legacy.Offset);

    return TRUE;
}

/***********************************************************************
 *             InitializeContext               (kernelbase.@)
 */
BOOL WINAPI InitializeContext( void *buffer, DWORD context_flags, CONTEXT **context, DWORD *length )
{
    return InitializeContext2( buffer, context_flags, context, length, ~(ULONG64)0 );
}

/***********************************************************************
 *           CopyContext                       (kernelbase.@)
 */
BOOL WINAPI CopyContext( CONTEXT *dst, DWORD context_flags, CONTEXT *src )
{
    return set_ntstatus( RtlCopyContext( dst, context_flags, src ));
}


#if defined(__x86_64__)

/***********************************************************************
 *             GetEnabledXStateFeatures   (kernelbase.@)
 */
DWORD64 WINAPI GetEnabledXStateFeatures(void)
{
    TRACE( "\n" );
    return RtlGetEnabledExtendedFeatures( ~(ULONG64)0 );
}

/***********************************************************************
 *           LocateXStateFeature   (kernelbase.@)
 */
void * WINAPI LocateXStateFeature( CONTEXT *context, DWORD feature_id, DWORD *length )
{
    if (!(context->ContextFlags & CONTEXT_AMD64))
        return NULL;

    if (feature_id >= 2)
        return ((context->ContextFlags & CONTEXT_XSTATE) == CONTEXT_XSTATE)
                ? RtlLocateExtendedFeature( (CONTEXT_EX *)(context + 1), feature_id, length ) : NULL;

    if (feature_id == 1)
    {
        if (length)
            *length = sizeof(M128A) * 16;

        return &context->FltSave.XmmRegisters;
    }

    if (length)
        *length = offsetof(XSAVE_FORMAT, XmmRegisters);

    return &context->FltSave;
}

/***********************************************************************
 *           SetXStateFeaturesMask (kernelbase.@)
 */
BOOL WINAPI SetXStateFeaturesMask( CONTEXT *context, DWORD64 feature_mask )
{
    if (!(context->ContextFlags & CONTEXT_AMD64))
        return FALSE;

    if (feature_mask & 0x3)
        context->ContextFlags |= CONTEXT_FLOATING_POINT;

    if ((context->ContextFlags & CONTEXT_XSTATE) != CONTEXT_XSTATE)
        return !(feature_mask & ~(DWORD64)3);

    RtlSetExtendedFeaturesMask( (CONTEXT_EX *)(context + 1), feature_mask );
    return TRUE;
}

/***********************************************************************
 *           GetXStateFeaturesMask (kernelbase.@)
 */
BOOL WINAPI GetXStateFeaturesMask( CONTEXT *context, DWORD64 *feature_mask )
{
    if (!(context->ContextFlags & CONTEXT_AMD64))
        return FALSE;

    *feature_mask = (context->ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT
            ? 3 : 0;

    if ((context->ContextFlags & CONTEXT_XSTATE) == CONTEXT_XSTATE)
        *feature_mask |= RtlGetExtendedFeaturesMask( (CONTEXT_EX *)(context + 1) );

    return TRUE;
}

#elif defined(__i386__)

/***********************************************************************
 *             GetEnabledXStateFeatures   (kernelbase.@)
 */
DWORD64 WINAPI GetEnabledXStateFeatures(void)
{
    TRACE( "\n" );
    return RtlGetEnabledExtendedFeatures( ~(ULONG64)0 );
}

/***********************************************************************
 *           LocateXStateFeature   (kernelbase.@)
 */
void * WINAPI LocateXStateFeature( CONTEXT *context, DWORD feature_id, DWORD *length )
{
    if (!(context->ContextFlags & CONTEXT_i386))
        return NULL;

    if (feature_id >= 2)
        return ((context->ContextFlags & CONTEXT_XSTATE) == CONTEXT_XSTATE)
                ? RtlLocateExtendedFeature( (CONTEXT_EX *)(context + 1), feature_id, length ) : NULL;

    if (feature_id == 1)
    {
        if (length)
            *length = sizeof(M128A) * 8;

        return (BYTE *)&context->ExtendedRegisters + offsetof(XSAVE_FORMAT, XmmRegisters);
    }

    if (length)
        *length = offsetof(XSAVE_FORMAT, XmmRegisters);

    return &context->ExtendedRegisters;
}

/***********************************************************************
 *           SetXStateFeaturesMask (kernelbase.@)
 */
BOOL WINAPI SetXStateFeaturesMask( CONTEXT *context, DWORD64 feature_mask )
{
    if (!(context->ContextFlags & CONTEXT_i386))
        return FALSE;

    if (feature_mask & 0x3)
        context->ContextFlags |= CONTEXT_EXTENDED_REGISTERS;

    if ((context->ContextFlags & CONTEXT_XSTATE) != CONTEXT_XSTATE)
        return !(feature_mask & ~(DWORD64)3);

    RtlSetExtendedFeaturesMask( (CONTEXT_EX *)(context + 1), feature_mask );
    return TRUE;
}

/***********************************************************************
 *           GetXStateFeaturesMask (kernelbase.@)
 */
BOOL WINAPI GetXStateFeaturesMask( CONTEXT *context, DWORD64 *feature_mask )
{
    if (!(context->ContextFlags & CONTEXT_i386))
        return FALSE;

    *feature_mask = (context->ContextFlags & CONTEXT_EXTENDED_REGISTERS) == CONTEXT_EXTENDED_REGISTERS
            ? 3 : 0;

    if ((context->ContextFlags & CONTEXT_XSTATE) == CONTEXT_XSTATE)
        *feature_mask |= RtlGetExtendedFeaturesMask( (CONTEXT_EX *)(context + 1) );

    return TRUE;
}
#endif

/***********************************************************************
 * Firmware functions
 ***********************************************************************/

static UINT get_firmware_table( DWORD provider, SYSTEM_FIRMWARE_TABLE_ACTION action, DWORD id,
                                void *buffer, DWORD size )
{
    SYSTEM_FIRMWARE_TABLE_INFORMATION *info;
    ULONG buffer_size = offsetof( SYSTEM_FIRMWARE_TABLE_INFORMATION, TableBuffer ) + size;
    NTSTATUS status;

    if (!(info = RtlAllocateHeap( GetProcessHeap(), 0, buffer_size )))
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }

    info->ProviderSignature = provider;
    info->Action = action;
    info->TableID = id;

    status = NtQuerySystemInformation( SystemFirmwareTableInformation, info, buffer_size, &buffer_size );
    set_ntstatus(status);
    buffer_size -= offsetof( SYSTEM_FIRMWARE_TABLE_INFORMATION, TableBuffer );
    if (buffer_size <= size) memcpy( buffer, info->TableBuffer, buffer_size );

    HeapFree( GetProcessHeap(), 0, info );
    return NT_SUCCESS(status) || status == STATUS_BUFFER_TOO_SMALL ? buffer_size : 0;
}

/***********************************************************************
 *             EnumSystemFirmwareTables   (kernelbase.@)
 */
UINT WINAPI EnumSystemFirmwareTables( DWORD provider, void *buffer, DWORD size )
{
    TRACE( "(0x%08lx, %p, %ld)\n", provider, buffer, size );

    return get_firmware_table( provider, SystemFirmwareTable_Enumerate, 0, buffer, size );
}

/***********************************************************************
 *             GetSystemFirmwareTable   (kernelbase.@)
 */
UINT WINAPI GetSystemFirmwareTable( DWORD provider, DWORD id, void *buffer, DWORD size )
{
    TRACE( "(0x%08lx, 0x%08lx, %p, %ld)\n", provider, id, buffer, size );

    return get_firmware_table( provider, SystemFirmwareTable_Get, id, buffer, size );
}
