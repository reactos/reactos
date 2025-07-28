/*
 * Unit test suite Wow64 functions
 *
 * Copyright 2021 Alexandre Julliard
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
 *
 */

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "winuser.h"
#include "ddk/wdm.h"
#include "wine/test.h"

#ifdef __REACTOS__
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define wcsicmp _wcsicmp
#if defined(_MSC_VER) && defined(_M_AMD64)
USHORT __readsegfs(void);
USHORT __readsegss(void);
#endif // _M_AMD64
#endif

static NTSTATUS (WINAPI *pNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS,void*,ULONG,ULONG*);
static NTSTATUS (WINAPI *pNtQuerySystemInformationEx)(SYSTEM_INFORMATION_CLASS,void*,ULONG,void*,ULONG,ULONG*);
static NTSTATUS (WINAPI *pRtlGetNativeSystemInformation)(SYSTEM_INFORMATION_CLASS,void*,ULONG,ULONG*);
static void     (WINAPI *pRtlOpenCrossProcessEmulatorWorkConnection)(HANDLE,HANDLE*,void**);
static void *   (WINAPI *pRtlFindExportedRoutineByName)(HMODULE,const char *);
static USHORT   (WINAPI *pRtlWow64GetCurrentMachine)(void);
static NTSTATUS (WINAPI *pRtlWow64GetProcessMachines)(HANDLE,WORD*,WORD*);
static NTSTATUS (WINAPI *pRtlWow64GetSharedInfoProcess)(HANDLE,BOOLEAN*,WOW64INFO*);
static NTSTATUS (WINAPI *pRtlWow64GetThreadContext)(HANDLE,WOW64_CONTEXT*);
static NTSTATUS (WINAPI *pRtlWow64IsWowGuestMachineSupported)(USHORT,BOOLEAN*);
static NTSTATUS (WINAPI *pNtMapViewOfSectionEx)(HANDLE,HANDLE,PVOID*,const LARGE_INTEGER*,SIZE_T*,ULONG,ULONG,MEM_EXTENDED_PARAMETER*,ULONG);
#ifdef _WIN64
static NTSTATUS (WINAPI *pKiUserExceptionDispatcher)(EXCEPTION_RECORD*,CONTEXT*);
static NTSTATUS (WINAPI *pRtlWow64GetCpuAreaInfo)(WOW64_CPURESERVED*,ULONG,WOW64_CPU_AREA_INFO*);
static NTSTATUS (WINAPI *pRtlWow64GetThreadSelectorEntry)(HANDLE,THREAD_DESCRIPTOR_INFORMATION*,ULONG,ULONG*);
static CROSS_PROCESS_WORK_ENTRY * (WINAPI *pRtlWow64PopAllCrossProcessWorkFromWorkList)(CROSS_PROCESS_WORK_HDR*,BOOLEAN*);
static CROSS_PROCESS_WORK_ENTRY * (WINAPI *pRtlWow64PopCrossProcessWorkFromFreeList)(CROSS_PROCESS_WORK_HDR*);
static BOOLEAN (WINAPI *pRtlWow64PushCrossProcessWorkOntoFreeList)(CROSS_PROCESS_WORK_HDR*,CROSS_PROCESS_WORK_ENTRY*);
static BOOLEAN (WINAPI *pRtlWow64PushCrossProcessWorkOntoWorkList)(CROSS_PROCESS_WORK_HDR*,CROSS_PROCESS_WORK_ENTRY*,void**);
static BOOLEAN (WINAPI *pRtlWow64RequestCrossProcessHeavyFlush)(CROSS_PROCESS_WORK_HDR*);
static void (WINAPI *pProcessPendingCrossProcessEmulatorWork)(void);
#else
static NTSTATUS (WINAPI *pNtWow64AllocateVirtualMemory64)(HANDLE,ULONG64*,ULONG64,ULONG64*,ULONG,ULONG);
static NTSTATUS (WINAPI *pNtWow64GetNativeSystemInformation)(SYSTEM_INFORMATION_CLASS,void*,ULONG,ULONG*);
static NTSTATUS (WINAPI *pNtWow64IsProcessorFeaturePresent)(ULONG);
static NTSTATUS (WINAPI *pNtWow64QueryInformationProcess64)(HANDLE,PROCESSINFOCLASS,void*,ULONG,ULONG*);
static NTSTATUS (WINAPI *pNtWow64ReadVirtualMemory64)(HANDLE,ULONG64,void*,ULONG64,ULONG64*);
static NTSTATUS (WINAPI *pNtWow64WriteVirtualMemory64)(HANDLE,ULONG64,const void *,ULONG64,ULONG64*);
#endif

static BOOL is_win64 = sizeof(void *) > sizeof(int);
static BOOL is_wow64;
static BOOL old_wow64;  /* Wine old-style wow64 */
static void *code_mem;

#ifdef __i386__
static USHORT current_machine = IMAGE_FILE_MACHINE_I386;
static USHORT native_machine = IMAGE_FILE_MACHINE_I386;
#elif defined __x86_64__
static USHORT current_machine = IMAGE_FILE_MACHINE_AMD64;
static USHORT native_machine = IMAGE_FILE_MACHINE_AMD64;
#elif defined __arm__
static USHORT current_machine = IMAGE_FILE_MACHINE_ARMNT;
static USHORT native_machine = IMAGE_FILE_MACHINE_ARMNT;
#elif defined __aarch64__
static USHORT current_machine = IMAGE_FILE_MACHINE_ARM64;
static USHORT native_machine = IMAGE_FILE_MACHINE_ARM64;
#else
static USHORT current_machine;
static USHORT native_machine;
#endif

struct arm64ec_shared_info
{
    ULONG     Wow64ExecuteFlags;
    USHORT    NativeMachineType;
    USHORT    EmulatedMachineType;
    ULONGLONG SectionHandle;
    ULONGLONG CrossProcessWorkList;
    ULONGLONG unknown;
};

#if !defined(__REACTOS__) || (DLL_EXPORT_VERSION >= 0x600)
static BOOL is_machine_32bit( USHORT machine )
{
    return machine == IMAGE_FILE_MACHINE_I386 || machine == IMAGE_FILE_MACHINE_ARMNT;
}
#endif // !defined(__REACTOS__) || (DLL_EXPORT_VERSION >= 0x600)

static void init(void)
{
    HMODULE ntdll = GetModuleHandleA( "ntdll.dll" );

    if (!IsWow64Process( GetCurrentProcess(), &is_wow64 )) is_wow64 = FALSE;

    if (is_wow64)
    {
        TEB64 *teb64 = ULongToPtr( NtCurrentTeb()->GdiBatchCount );

        if (teb64)
        {
            PEB64 *peb64 = ULongToPtr(teb64->Peb);
            old_wow64 = !peb64->LdrData;
        }
    }

#define GET_PROC(func) p##func = (void *)GetProcAddress( ntdll, #func )
    GET_PROC( NtMapViewOfSectionEx );
    GET_PROC( NtQuerySystemInformation );
    GET_PROC( NtQuerySystemInformationEx );
    GET_PROC( RtlGetNativeSystemInformation );
    GET_PROC( RtlOpenCrossProcessEmulatorWorkConnection );
    GET_PROC( RtlFindExportedRoutineByName );
    GET_PROC( RtlWow64GetCurrentMachine );
    GET_PROC( RtlWow64GetProcessMachines );
    GET_PROC( RtlWow64GetSharedInfoProcess );
    GET_PROC( RtlWow64GetThreadContext );
    GET_PROC( RtlWow64IsWowGuestMachineSupported );
#ifdef _WIN64
    GET_PROC( KiUserExceptionDispatcher );
    GET_PROC( RtlWow64GetCpuAreaInfo );
    GET_PROC( RtlWow64GetThreadSelectorEntry );
    GET_PROC( RtlWow64PopAllCrossProcessWorkFromWorkList );
    GET_PROC( RtlWow64PopCrossProcessWorkFromFreeList );
    GET_PROC( RtlWow64PushCrossProcessWorkOntoFreeList );
    GET_PROC( RtlWow64PushCrossProcessWorkOntoWorkList );
    GET_PROC( RtlWow64RequestCrossProcessHeavyFlush );
    GET_PROC( ProcessPendingCrossProcessEmulatorWork );
#else
    GET_PROC( NtWow64AllocateVirtualMemory64 );
    GET_PROC( NtWow64GetNativeSystemInformation );
    GET_PROC( NtWow64IsProcessorFeaturePresent );
    GET_PROC( NtWow64QueryInformationProcess64 );
    GET_PROC( NtWow64ReadVirtualMemory64 );
    GET_PROC( NtWow64WriteVirtualMemory64 );
#endif
#undef GET_PROC

    if (pNtQuerySystemInformationEx)
    {
        SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION machines[8];
        HANDLE process = GetCurrentProcess();
        NTSTATUS status = pNtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process,
                                                       sizeof(process), machines, sizeof(machines), NULL );
        if (!status)
            for (int i = 0; machines[i].Machine; i++)
                trace( "machine %04x kernel %u user %u native %u process %u wow64 %u\n",
                       machines[i].Machine, machines[i].KernelMode, machines[i].UserMode,
                       machines[i].Native, machines[i].Process, machines[i].WoW64Container );
    }

    if (pRtlGetNativeSystemInformation)
    {
        SYSTEM_CPU_INFORMATION info;
        ULONG len;

        pRtlGetNativeSystemInformation( SystemCpuInformation, &info, sizeof(info), &len );
        switch (info.ProcessorArchitecture)
        {
        case PROCESSOR_ARCHITECTURE_ARM64:
            native_machine = IMAGE_FILE_MACHINE_ARM64;
            break;
        case PROCESSOR_ARCHITECTURE_AMD64:
            native_machine = IMAGE_FILE_MACHINE_AMD64;
            break;
        }
    }

    trace( "current %04x native %04x\n", current_machine, native_machine );

    if (native_machine == IMAGE_FILE_MACHINE_AMD64)
        code_mem = VirtualAlloc( NULL, 65536, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE );
}

#if !defined (__REACTOS__) || (DLL_EXPORT_VERSION >= 0x600)
static BOOL create_process_machine( char *cmdline, DWORD flags, USHORT machine, PROCESS_INFORMATION *pi )
{
    struct _PROC_THREAD_ATTRIBUTE_LIST *list;
    STARTUPINFOEXA si = {{ sizeof(si) }};
    SIZE_T size = 1024;
    BOOL ret;

    si.lpAttributeList = list = malloc( size );
    InitializeProcThreadAttributeList( list, 1, 0, &size );
    UpdateProcThreadAttribute( list, 0, PROC_THREAD_ATTRIBUTE_MACHINE_TYPE,
                               &machine, sizeof(machine), NULL, NULL );
    ret = CreateProcessA( NULL, cmdline, NULL, NULL, FALSE,
                          EXTENDED_STARTUPINFO_PRESENT | flags, NULL, NULL, &si.StartupInfo, pi );
    DeleteProcThreadAttributeList( list );
    free( list );
    return ret;
}

static void test_process_architecture( HANDLE process, USHORT expect_machine, USHORT expect_native )
{
    SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION machines[8];
    NTSTATUS status;
    ULONG i, len;

    len = 0xdead;
    status = pNtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, sizeof(process),
                                          machines, sizeof(machines), &len );
    ok( !status, "failed %lx\n", status );
    ok( !(len & 3), "wrong len %lx\n", len );
    len /= sizeof(machines[0]);
    for (i = 0; i < len - 1; i++)
    {
        if (machines[i].Process)
            ok( machines[i].Machine == expect_machine, "wrong process machine %x\n", machines[i].Machine);
        else
            ok( machines[i].Machine != expect_machine, "wrong machine %x\n", machines[i].Machine);

        if (machines[i].Native)
            ok( machines[i].Machine == expect_native, "wrong native machine %x\n", machines[i].Machine);
        else
            ok( machines[i].Machine != expect_native, "wrong machine %x\n", machines[i].Machine);

        if (machines[i].WoW64Container)
            ok( is_machine_32bit( machines[i].Machine ) && !is_machine_32bit( native_machine ),
                "wrong wow64 %x\n", machines[i].Machine);
    }
    ok( !*(DWORD *)&machines[i], "missing terminating null\n" );

    len = i * sizeof(machines[0]);
    status = pNtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, sizeof(process),
                                          machines, len, &len );
    ok( status == STATUS_BUFFER_TOO_SMALL, "failed %lx\n", status );
    ok( len == (i + 1) * sizeof(machines[0]), "wrong len %lu\n", len );

    if (pRtlWow64GetProcessMachines)
    {
        USHORT current = 0xdead, native = 0xbeef;
        status = pRtlWow64GetProcessMachines( process, &current, &native );
        ok( !status, "failed %lx\n", status );
        if (expect_machine == expect_native)
            ok( current == 0, "wrong current machine %x / %x\n", current, expect_machine );
        else
            ok( current == expect_machine, "wrong current machine %x / %x\n", current, expect_machine );
        ok( native == expect_native, "wrong native machine %x / %x\n", native, expect_native );
    }
}

static void test_process_machine( HANDLE process, HANDLE thread,
                                  USHORT expect_machine, USHORT expect_image )
{
    PROCESS_BASIC_INFORMATION basic;
    SECTION_IMAGE_INFORMATION image;
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS nt;
    PEB peb;
    ULONG len;
    SIZE_T size;
    NTSTATUS status;
    void *entry_point = NULL;
    void *win32_entry = NULL;

    status = NtQueryInformationProcess( process, ProcessBasicInformation, &basic, sizeof(basic), &len );
    ok( !status, "ProcessBasicInformation failed %lx\n", status );
    if (ReadProcessMemory( process, basic.PebBaseAddress, &peb, sizeof(peb), &size ) &&
        ReadProcessMemory( process, peb.ImageBaseAddress, &dos, sizeof(dos), &size ) &&
        ReadProcessMemory( process, (char *)peb.ImageBaseAddress + dos.e_lfanew, &nt, sizeof(nt), &size ))
    {
        ok( nt.FileHeader.Machine == expect_machine, "wrong nt machine %x / %x\n",
            nt.FileHeader.Machine, expect_machine );
        entry_point = (char *)peb.ImageBaseAddress + nt.OptionalHeader.AddressOfEntryPoint;
    }

    status = NtQueryInformationProcess( process, ProcessImageInformation, &image, sizeof(image), &len );
    ok( !status, "ProcessImageInformation failed %lx\n", status );
    ok( image.Machine == expect_image, "wrong image info %x / %x\n", image.Machine, expect_image );

    status = NtQueryInformationThread( thread, ThreadQuerySetWin32StartAddress,
                                       &win32_entry, sizeof(win32_entry), &len );
    ok( !status, "ThreadQuerySetWin32StartAddress failed %lx\n", status );

    if (!entry_point) return;

    if (image.Machine == expect_machine)
    {
        ok( image.TransferAddress == entry_point, "wrong entry %p / %p\n",
            image.TransferAddress, entry_point );
        ok( win32_entry == entry_point, "wrong win32 entry %p / %p\n",
            win32_entry, entry_point );
    }
    else
    {
        /* image.TransferAddress is the ARM64 entry, entry_point is the x86-64 one,
           win32_entry is the redirected x86-64 -> ARM64EC one */
        ok( image.TransferAddress != entry_point, "wrong entry %p\n", image.TransferAddress );
        ok( image.TransferAddress != win32_entry, "wrong entry %p\n", image.TransferAddress );
        ok( win32_entry != entry_point, "wrong win32 entry %p\n", win32_entry );
    }
}

static void test_query_architectures(void)
{
    static char cmd_sysnative[] = "C:\\windows\\sysnative\\cmd.exe /c exit";
    static char cmd_system32[] = "C:\\windows\\system32\\cmd.exe /c exit";
    static char cmd_syswow64[] = "C:\\windows\\syswow64\\cmd.exe /c exit";
    SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION machines[8];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { sizeof(si) };
    NTSTATUS status;
    HANDLE process;
    ULONG i, len;
#ifdef __arm64ec__
    BOOL is_arm64ec = TRUE;
#else
    BOOL is_arm64ec = FALSE;
#endif

    if (!pNtQuerySystemInformationEx) return;

    process = GetCurrentProcess();
    status = pNtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, sizeof(process),
                                          machines, sizeof(machines), &len );
    if (status == STATUS_INVALID_INFO_CLASS)
    {
        win_skip( "SystemSupportedProcessorArchitectures not supported\n" );
        return;
    }
    ok( !status, "failed %lx\n", status );

    process = (HANDLE)0xdeadbeef;
    status = pNtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, sizeof(process),
                                          machines, sizeof(machines), &len );
    ok( status == STATUS_INVALID_HANDLE, "failed %lx\n", status );
    process = (HANDLE)0xdeadbeef;
    status = pNtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, 3,
                                          machines, sizeof(machines), &len );
    ok( status == STATUS_INVALID_PARAMETER || broken(status == STATUS_INVALID_HANDLE),
        "failed %lx\n", status );
    process = GetCurrentProcess();
    status = pNtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, 3,
                                          machines, sizeof(machines), &len );
    ok( status == STATUS_INVALID_PARAMETER || broken( status == STATUS_SUCCESS),
        "failed %lx\n", status );
    status = pNtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, NULL, 0,
                                          machines, sizeof(machines), &len );
    ok( status == STATUS_INVALID_PARAMETER, "failed %lx\n", status );

    winetest_push_context( "current" );
    test_process_architecture( GetCurrentProcess(), is_win64 ? native_machine : current_machine,
                               native_machine );
    test_process_machine( GetCurrentProcess(), GetCurrentThread(), current_machine,
                          is_arm64ec ? native_machine : current_machine );
    winetest_pop_context();

    winetest_push_context( "zero" );
    test_process_architecture( 0, 0, native_machine );
    winetest_pop_context();

    if (CreateProcessA( NULL, is_win64 ? cmd_system32 : cmd_sysnative, NULL, NULL,
                        FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi ))
    {
        winetest_push_context( "system32" );
        test_process_architecture( pi.hProcess, native_machine, native_machine );
        test_process_machine( pi.hProcess, pi.hThread,
                              is_win64 ? current_machine : native_machine, native_machine );
        TerminateProcess( pi.hProcess, 0 );
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
        winetest_pop_context();
    }
    if (CreateProcessA( NULL, is_win64 ? cmd_syswow64 : cmd_system32, NULL, NULL,
                        FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi ))
    {
        winetest_push_context( "syswow64" );
        test_process_architecture( pi.hProcess, IMAGE_FILE_MACHINE_I386, native_machine );
        test_process_machine( pi.hProcess, pi.hThread, IMAGE_FILE_MACHINE_I386, IMAGE_FILE_MACHINE_I386 );
        TerminateProcess( pi.hProcess, 0 );
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
        winetest_pop_context();
    }
    if (is_win64 && native_machine == IMAGE_FILE_MACHINE_ARM64)
    {
        USHORT machine = IMAGE_FILE_MACHINE_ARM64 + IMAGE_FILE_MACHINE_AMD64 - current_machine;

        if (create_process_machine( cmd_system32, CREATE_SUSPENDED, machine, &pi ))
        {
            winetest_push_context( "%04x", machine );
            test_process_architecture( pi.hProcess, native_machine, native_machine );
            test_process_machine( pi.hProcess, pi.hThread, machine, native_machine );
            TerminateProcess( pi.hProcess, 0 );
            CloseHandle( pi.hProcess );
            CloseHandle( pi.hThread );
            winetest_pop_context();
        }
    }

    if (pRtlWow64GetCurrentMachine)
    {
        USHORT machine = pRtlWow64GetCurrentMachine();
        ok( machine == current_machine, "wrong machine %x / %x\n", machine, current_machine );
    }
    if (pRtlWow64IsWowGuestMachineSupported)
    {
        static const WORD machines[] = { IMAGE_FILE_MACHINE_I386, IMAGE_FILE_MACHINE_ARMNT,
                                         IMAGE_FILE_MACHINE_AMD64, IMAGE_FILE_MACHINE_ARM64, 0xdead };

        for (i = 0; i < ARRAY_SIZE(machines); i++)
        {
            BOOLEAN ret = 0xcc;
            status = pRtlWow64IsWowGuestMachineSupported( machines[i], &ret );
            ok( !status, "failed %lx\n", status );
            if (is_machine_32bit( machines[i] ) && !is_machine_32bit( native_machine ))
                ok( ret || machines[i] == IMAGE_FILE_MACHINE_ARMNT ||
                    broken(current_machine == IMAGE_FILE_MACHINE_I386), /* win10-1607 wow64 */
                    "%04x: got %u\n", machines[i], ret );
            else
                ok( !ret, "%04x: got %u\n", machines[i], ret );
        }
    }
}
#endif // !defined (__REACTOS__) || (DLL_EXPORT_VERSION >= 0x600)

static void push_onto_free_list( CROSS_PROCESS_WORK_HDR *list, CROSS_PROCESS_WORK_ENTRY *entry )
{
#ifdef _WIN64
    pRtlWow64PushCrossProcessWorkOntoFreeList( list, entry );
#else
    entry->next = list->first;
    list->first = (char *)entry - (char *)list;
#endif
}

static void push_onto_work_list( CROSS_PROCESS_WORK_HDR *list, CROSS_PROCESS_WORK_ENTRY *entry )
{
#ifdef _WIN64
    void *ret;
    pRtlWow64PushCrossProcessWorkOntoWorkList( list, entry, &ret );
#else
    entry->next = list->first;
    list->first = (char *)entry - (char *)list;
#endif
}

static CROSS_PROCESS_WORK_ENTRY *pop_from_free_list( CROSS_PROCESS_WORK_HDR *list )
{
#ifdef _WIN64
    return pRtlWow64PopCrossProcessWorkFromFreeList( list );
#else
    CROSS_PROCESS_WORK_ENTRY *ret;

    if (!list->first) return NULL;
    ret = (CROSS_PROCESS_WORK_ENTRY *)((char *)list + list->first);
    list->first = ret->next;
    ret->next = 0;
    return ret;
#endif
}

static CROSS_PROCESS_WORK_ENTRY *pop_from_work_list( CROSS_PROCESS_WORK_HDR *list )
{
#ifdef _WIN64
    BOOLEAN flush;

    return pRtlWow64PopAllCrossProcessWorkFromWorkList( list, &flush );
#else
    UINT pos = list->first, prev_pos = 0;

    list->first = 0;
    if (!pos) return NULL;

    for (;;)  /* reverse the list */
    {
        CROSS_PROCESS_WORK_ENTRY *entry = CROSS_PROCESS_LIST_ENTRY( list, pos );
        UINT next = entry->next;
        entry->next = prev_pos;
        if (!next) return entry;
        prev_pos = pos;
        pos = next;
    }
#endif
}

static void request_cross_process_flush( CROSS_PROCESS_WORK_HDR *list )
{
#ifdef _WIN64
    pRtlWow64RequestCrossProcessHeavyFlush( list );
#else
    list->first |= CROSS_PROCESS_LIST_FLUSH;
#endif
}

#define expect_cross_work_entry(list,entry,id,addr,size,arg0,arg1,arg2,arg3) \
    expect_cross_work_entry_(list,entry,id,addr,size,arg0,arg1,arg2,arg3,__LINE__)
static CROSS_PROCESS_WORK_ENTRY *expect_cross_work_entry_( CROSS_PROCESS_WORK_LIST *list,
                                                           CROSS_PROCESS_WORK_ENTRY *entry,
                                                           UINT id, void *addr, SIZE_T size,
                                                           UINT arg0, UINT arg1, UINT arg2, UINT arg3,
                                                           int line )
{
    CROSS_PROCESS_WORK_ENTRY *next;

    ok_(__FILE__,line)( entry != NULL, "no more entries in list\n" );
    if (!entry) return NULL;
    ok_(__FILE__,line)( entry->id == id, "wrong type %u / %u\n", entry->id, id );
    ok_(__FILE__,line)( entry->addr == (ULONG_PTR)addr, "wrong address %s / %p\n",
                        wine_dbgstr_longlong(entry->addr), addr );
    ok_(__FILE__,line)( entry->size == size, "wrong size %s / %Ix\n",
                        wine_dbgstr_longlong(entry->size), size );
    ok_(__FILE__,line)( entry->args[0] == arg0, "wrong args[0] %x / %x\n", entry->args[0], arg0 );
    ok_(__FILE__,line)( entry->args[1] == arg1, "wrong args[1] %x / %x\n", entry->args[1], arg1 );
    ok_(__FILE__,line)( entry->args[2] == arg2, "wrong args[2] %x / %x\n", entry->args[2], arg2 );
    ok_(__FILE__,line)( entry->args[3] == arg3, "wrong args[3] %x / %x\n", entry->args[3], arg3 );
    next = entry->next ? CROSS_PROCESS_LIST_ENTRY( &list->work_list, entry->next ) : NULL;
    memset( entry, 0xcc, sizeof(*entry) );
    push_onto_free_list( &list->free_list, entry );
    return next;
}

static void test_cross_process_notifications( HANDLE process, ULONG_PTR section, ULONG_PTR ptr )
{
    CROSS_PROCESS_WORK_ENTRY *entry;
    CROSS_PROCESS_WORK_LIST *list;
    UINT pos;
    void *addr = NULL, *addr2;
    SIZE_T size = 0;
    DWORD old_prot;
    LARGE_INTEGER offset;
    HANDLE file, mapping;
    NTSTATUS status;
    BOOL ret;
    BYTE data[] = { 0xcc, 0xcc, 0xcc };

    ret = DuplicateHandle( process, (HANDLE)section, GetCurrentProcess(), &mapping,
                           0, FALSE, DUPLICATE_SAME_ACCESS );
    ok( ret, "DuplicateHandle failed %lu\n", GetLastError() );
    status = NtMapViewOfSection( mapping, GetCurrentProcess(), &addr, 0, 0, NULL,
                                 &size, ViewShare, 0, PAGE_READWRITE );
    ok( !status, "NtMapViewOfSection failed %lx\n", status );
    ok( size == 0x4000, "unexpected size %Ix\n", size );
    list = addr;
    addr2 = malloc( size );
    ret = ReadProcessMemory( process, (void *)ptr, addr2, size, &size );
    ok( ret, "ReadProcessMemory failed %lu\n", GetLastError() );
    ok( !memcmp( addr2, addr, size ), "wrong data\n" );
    free( addr2 );
    CloseHandle( mapping );

    if (pRtlOpenCrossProcessEmulatorWorkConnection)
    {
        pRtlOpenCrossProcessEmulatorWorkConnection( process, &mapping, &addr2 );
        ok( mapping != 0, "got 0 handle\n" );
        ok( addr2 != NULL, "got NULL data\n" );
        ok( !memcmp( addr2, addr, size ), "wrong data\n" );
        UnmapViewOfFile( addr2 );
        addr2 = NULL;
        size = 0;
        status = NtMapViewOfSection( mapping, GetCurrentProcess(), &addr2, 0, 0, NULL,
                                     &size, ViewShare, 0, PAGE_READWRITE );
        ok( !status, "NtMapViewOfSection failed %lx\n", status );
        ok( !memcmp( addr2, addr, size ), "wrong data\n" );
        ok( CloseHandle( mapping ), "invalid handle\n" );
        UnmapViewOfFile( addr2 );

        mapping = (HANDLE)0xdead;
        addr2 = (void *)0xdeadbeef;
        pRtlOpenCrossProcessEmulatorWorkConnection( GetCurrentProcess(), &mapping, &addr2 );
        ok( !mapping, "got handle %p\n", mapping );
        ok( !addr2, "got data %p\n", addr2 );
    }
    else skip( "RtlOpenCrossProcessEmulatorWorkConnection not supported\n" );

    NtSuspendProcess( process );

    /* set argument values in free list to detect changes */
    for (pos = list->free_list.first; pos; pos = entry->next )
    {
        entry = CROSS_PROCESS_LIST_ENTRY( &list->free_list, pos );
        memset( entry->args, 0xcc, sizeof(entry->args) );
    }

    addr = VirtualAllocEx( process, NULL, 0x1234, MEM_COMMIT, PAGE_READWRITE );
    entry = pop_from_work_list( &list->work_list );
    if (current_machine != IMAGE_FILE_MACHINE_ARM64)
    {
        entry = expect_cross_work_entry( list, entry, CrossProcessPreVirtualAlloc, NULL, 0x1234,
                                         MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE, 0, 0xcccccccc );
        entry = expect_cross_work_entry( list, entry, CrossProcessPostVirtualAlloc, addr, 0x2000,
                                         MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE, 0, 0xcccccccc );
    }
    ok( !entry, "not at end of list\n" );

    VirtualProtectEx( process, (char *)addr + 0x333, 17, PAGE_READONLY, &old_prot );
    entry = pop_from_work_list( &list->work_list );
    if (current_machine != IMAGE_FILE_MACHINE_ARM64)
    {
        entry = expect_cross_work_entry( list, entry, CrossProcessPreVirtualProtect,
                                         (char *)addr + 0x333, 17,
                                         PAGE_READONLY, 0, 0xcccccccc, 0xcccccccc );
        entry = expect_cross_work_entry( list, entry, CrossProcessPostVirtualProtect, addr, 0x1000,
                                         PAGE_READONLY, 0, 0xcccccccc, 0xcccccccc );
    }
    ok( !entry, "not at end of list\n" );

    VirtualFreeEx( process, addr, 0, MEM_RELEASE );
    entry = pop_from_work_list( &list->work_list );
    if (current_machine != IMAGE_FILE_MACHINE_ARM64)
    {
        entry = expect_cross_work_entry( list, entry, CrossProcessPreVirtualFree, addr, 0,
                                         MEM_RELEASE, 0, 0xcccccccc, 0xcccccccc );
        entry = expect_cross_work_entry( list, entry, CrossProcessPostVirtualFree, addr, 0x2000,
                                         MEM_RELEASE, 0, 0xcccccccc, 0xcccccccc );
    }
    ok( !entry, "not at end of list\n" );

    addr = (void *)0x123;
    size = 0x321;
    status = NtAllocateVirtualMemory( process, &addr, 0, &size, MEM_COMMIT, PAGE_EXECUTE_READ );
    ok( status == STATUS_CONFLICTING_ADDRESSES || status == STATUS_INVALID_PARAMETER,
        "NtAllocateVirtualMemory failed %lx\n", status );
    entry = pop_from_work_list( &list->work_list );
    if (current_machine != IMAGE_FILE_MACHINE_ARM64)
    {
        entry = expect_cross_work_entry( list, entry, CrossProcessPreVirtualAlloc, addr, 0x321,
                                         MEM_COMMIT, PAGE_EXECUTE_READ, 0, 0xcccccccc );
        entry = expect_cross_work_entry( list, entry, CrossProcessPostVirtualAlloc, addr, 0x321,
                                         MEM_COMMIT, PAGE_EXECUTE_READ, status, 0xcccccccc );
    }
    ok( !entry, "not at end of list\n" );

    addr = NULL;
    size = 0x321;
    status = NtAllocateVirtualMemory( process, &addr, 0, &size, 0, PAGE_EXECUTE_READ );
    ok( status == STATUS_INVALID_PARAMETER, "NtAllocateVirtualMemory failed %lx\n", status );
    entry = pop_from_work_list( &list->work_list );
    if (current_machine != IMAGE_FILE_MACHINE_ARM64)
    {
        entry = expect_cross_work_entry( list, entry, CrossProcessPreVirtualAlloc, addr, 0x321,
                                         0, PAGE_EXECUTE_READ, 0, 0xcccccccc );
        entry = expect_cross_work_entry( list, entry, CrossProcessPostVirtualAlloc, addr, 0x321,
                                         0, PAGE_EXECUTE_READ, status, 0xcccccccc );
    }
    ok( !entry, "not at end of list\n" );

    addr = NULL;
    size = 0x4321;
    status = NtAllocateVirtualMemory( process, &addr, 0, &size, MEM_RESERVE, PAGE_EXECUTE_READWRITE );
    ok( !status, "NtAllocateVirtualMemory failed %lx\n", status );
    entry = pop_from_work_list( &list->work_list );
    if (current_machine != IMAGE_FILE_MACHINE_ARM64)
    {
        entry = expect_cross_work_entry( list, entry, CrossProcessPreVirtualAlloc, NULL, 0x4321,
                                         MEM_RESERVE, PAGE_EXECUTE_READWRITE, 0, 0xcccccccc );
        entry = expect_cross_work_entry( list, entry, CrossProcessPostVirtualAlloc, addr, 0x5000,
                                         MEM_RESERVE, PAGE_EXECUTE_READWRITE, 0, 0xcccccccc );
    }
    ok( !entry, "not at end of list\n" );

    size = 0x4321;
    status = NtAllocateVirtualMemory( process, &addr, 0, &size, MEM_COMMIT, PAGE_READWRITE );
    ok( !status, "NtAllocateVirtualMemory failed %lx\n", status );
    entry = pop_from_work_list( &list->work_list );
    if (current_machine != IMAGE_FILE_MACHINE_ARM64)
    {
        entry = expect_cross_work_entry( list, entry, CrossProcessPreVirtualAlloc, addr, 0x4321,
                                         MEM_COMMIT, PAGE_READWRITE, 0, 0xcccccccc );
        entry = expect_cross_work_entry( list, entry, CrossProcessPostVirtualAlloc, addr, 0x5000,
                                         MEM_COMMIT, PAGE_READWRITE, 0, 0xcccccccc );
    }
    ok( !entry, "not at end of list\n" );

    addr2 = (char *)addr + 0x111;
    size = 23;
    status = NtProtectVirtualMemory( process, &addr2, &size, PAGE_EXECUTE_READWRITE, &old_prot );
    ok( !status, "NtProtectVirtualMemory failed %lx\n", status );
    entry = pop_from_work_list( &list->work_list );
    if (current_machine != IMAGE_FILE_MACHINE_ARM64)
    {
        entry = expect_cross_work_entry( list, entry, CrossProcessPreVirtualProtect, (char *)addr + 0x111, 23,
                                         PAGE_EXECUTE_READWRITE, 0, 0xcccccccc, 0xcccccccc );
        entry = expect_cross_work_entry( list, entry, CrossProcessPostVirtualProtect, addr, 0x1000,
                                         PAGE_EXECUTE_READWRITE, 0, 0xcccccccc, 0xcccccccc );
    }
    ok( !entry, "not at end of list\n" );

    addr2 = (char *)addr + 0x222;
    size = 34;
    status = NtProtectVirtualMemory( process, &addr2, &size, PAGE_EXECUTE_WRITECOPY, &old_prot );
    ok( status == STATUS_INVALID_PARAMETER_4 || status == STATUS_INVALID_PAGE_PROTECTION,
        "NtProtectVirtualMemory failed %lx\n", status );
    entry = pop_from_work_list( &list->work_list );
    if (current_machine != IMAGE_FILE_MACHINE_ARM64)
    {
        entry = expect_cross_work_entry( list, entry, CrossProcessPreVirtualProtect,
                                         (char *)addr + 0x222, 34,
                                         PAGE_EXECUTE_WRITECOPY, 0, 0xcccccccc, 0xcccccccc );
        entry = expect_cross_work_entry( list, entry, CrossProcessPostVirtualProtect,
                                         (char *)addr + 0x222, 34,
                                         PAGE_EXECUTE_WRITECOPY, status, 0xcccccccc, 0xcccccccc );
    }
    ok( !entry, "not at end of list\n" );

    status = NtWriteVirtualMemory( process, (char *)addr + 0x1111, data, sizeof(data), &size );
    ok( !status, "NtWriteVirtualMemory failed %lx\n", status );
    entry = pop_from_work_list( &list->work_list );
    ok( !entry, "not at end of list\n" );

    addr2 = (char *)addr + 0x1234;
    size = 45;
    status = NtFreeVirtualMemory( process, &addr2, &size, MEM_DECOMMIT );
    ok( !status, "NtFreeVirtualMemory failed %lx\n", status );
    entry = pop_from_work_list( &list->work_list );
    if (current_machine != IMAGE_FILE_MACHINE_ARM64)
    {
        entry = expect_cross_work_entry( list, entry, CrossProcessPreVirtualFree, (char *)addr + 0x1234, 45,
                                         MEM_DECOMMIT, 0, 0xcccccccc, 0xcccccccc );
        entry = expect_cross_work_entry( list, entry, CrossProcessPostVirtualFree, addr2, 0x1000,
                                         MEM_DECOMMIT, 0, 0xcccccccc, 0xcccccccc );
    }
    ok( !entry, "not at end of list\n" );

    size = 0;
    status = NtFreeVirtualMemory( process, &addr, &size, MEM_RELEASE );
    ok( !status, "NtFreeVirtualMemory failed %lx\n", status );
    entry = pop_from_work_list( &list->work_list );
    if (current_machine != IMAGE_FILE_MACHINE_ARM64)
    {
        entry = expect_cross_work_entry( list, entry, CrossProcessPreVirtualFree, addr, 0,
                                         MEM_RELEASE, 0, 0xcccccccc, 0xcccccccc );
        entry = expect_cross_work_entry( list, entry, CrossProcessPostVirtualFree, addr, 0x5000,
                                         MEM_RELEASE, 0, 0xcccccccc, 0xcccccccc );
    }
    ok( !entry, "not at end of list\n" );

    addr = (void *)0x123;
    size = 0;
    status = NtFreeVirtualMemory( process, &addr, &size, MEM_RELEASE );
    ok( status == STATUS_MEMORY_NOT_ALLOCATED || status == STATUS_INVALID_PARAMETER,
        "NtFreeVirtualMemory failed %lx\n", status );
    entry = pop_from_work_list( &list->work_list );
    if (current_machine != IMAGE_FILE_MACHINE_ARM64)
    {
        entry = expect_cross_work_entry( list, entry, CrossProcessPreVirtualFree, addr, 0,
                                         MEM_RELEASE, 0, 0xcccccccc, 0xcccccccc );
        entry = expect_cross_work_entry( list, entry, CrossProcessPostVirtualFree, addr, 0,
                                         MEM_RELEASE, status, 0xcccccccc, 0xcccccccc );
    }
    ok( !entry, "not at end of list\n" );

    file = CreateFileA( "c:\\windows\\syswow64\\version.dll", GENERIC_READ | GENERIC_EXECUTE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "Failed to open version.dll\n" );
    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL );
    ok( mapping != 0, "CreateFileMapping failed\n" );
    addr = NULL;
    size = 0;
    offset.QuadPart = 0;
    status = NtMapViewOfSection( mapping, process, &addr, 0, 0, &offset, &size, ViewShare, 0, PAGE_READONLY );
    ok( NT_SUCCESS(status), "NtMapViewOfSection failed %lx\n", status );
    entry = pop_from_work_list( &list->work_list );
    ok( !entry, "list not empty\n" );

    FlushInstructionCache( process, addr, 0x1234 );
    entry = pop_from_work_list( &list->work_list );
    entry = expect_cross_work_entry( list, entry, CrossProcessFlushCache, addr, 0x1234,
                                     0xcccccccc, 0xcccccccc, 0xcccccccc, 0xcccccccc );
    ok( !entry, "not at end of list\n" );

    NtFlushInstructionCache( process, addr, 0x1234 );
    entry = pop_from_work_list( &list->work_list );
    if (current_machine != IMAGE_FILE_MACHINE_ARM64)
    {
        entry = expect_cross_work_entry( list, entry, CrossProcessFlushCache, addr, 0x1234,
                                         0xcccccccc, 0xcccccccc, 0xcccccccc, 0xcccccccc );
    }
    ok( !entry, "not at end of list\n" );

    WriteProcessMemory( process, (char *)addr + 0x1ffe, data, sizeof(data), &size );
    entry = pop_from_work_list( &list->work_list );
    entry = expect_cross_work_entry( list, entry, CrossProcessPreVirtualProtect,
                                     (char *)addr + 0x1000, 0x2000, 0x60000000 | PAGE_EXECUTE_WRITECOPY,
                                     (current_machine != IMAGE_FILE_MACHINE_ARM64) ? 0 : 0xcccccccc,
                                     0xcccccccc, 0xcccccccc );
    entry = expect_cross_work_entry( list, entry, CrossProcessPostVirtualProtect,
                                     (char *)addr + 0x1000, 0x2000,
                                     0x60000000 | PAGE_EXECUTE_WRITECOPY, 0, 0xcccccccc, 0xcccccccc );
    entry = expect_cross_work_entry( list, entry, CrossProcessFlushCache,
                                     (char *)addr + 0x1ffe, sizeof(data),
                                     0xcccccccc, 0xcccccccc, 0xcccccccc, 0xcccccccc );
    entry = expect_cross_work_entry( list, entry, CrossProcessPreVirtualProtect,
                                     (char *)addr + 0x1000, 0x2000, 0x60000000 | PAGE_EXECUTE_READ,
                                     (current_machine != IMAGE_FILE_MACHINE_ARM64) ? 0 : 0xcccccccc,
                                     0xcccccccc, 0xcccccccc );
    entry = expect_cross_work_entry( list, entry, CrossProcessPostVirtualProtect,
                                     (char *)addr + 0x1000, 0x2000,
                                     0x60000000 | PAGE_EXECUTE_READ, 0, 0xcccccccc, 0xcccccccc );
    ok( !entry, "not at end of list\n" );

    status = NtUnmapViewOfSection( process, addr );
    ok( !status, "NtUnmapViewOfSection failed %lx\n", status );
    entry = pop_from_work_list( &list->work_list );
    ok( !entry, "list not empty\n" );

    CloseHandle( mapping );
    CloseHandle( file );
    UnmapViewOfFile( list );
}

static void test_wow64_shared_info( HANDLE process )
{
    ULONG i, peb_data[0x200], buffer[16];
    WOW64INFO *info = (WOW64INFO *)buffer;
    ULONG_PTR peb_ptr;
    NTSTATUS status;
    SIZE_T res;
    BOOLEAN wow64 = 0xcc;

    NtQueryInformationProcess( process, ProcessWow64Information, &peb_ptr, sizeof(peb_ptr), NULL );
    memset( buffer, 0xcc, sizeof(buffer) );
    status = pRtlWow64GetSharedInfoProcess( process, &wow64, info );
    ok( !status, "RtlWow64GetSharedInfoProcess failed %lx\n", status );
    ok( wow64 == TRUE, "wrong wow64 %u\n", wow64 );
    todo_wine_if (!info->NativeSystemPageSize) /* not set in old wow64 */
    {
        ok( info->NativeSystemPageSize == 0x1000, "wrong page size %lx\n",
            info->NativeSystemPageSize );
        ok( info->CpuFlags == (native_machine == IMAGE_FILE_MACHINE_AMD64 ? WOW64_CPUFLAGS_MSFT64 : WOW64_CPUFLAGS_SOFTWARE),
            "wrong flags %lx\n", info->CpuFlags );
        ok( info->NativeMachineType == native_machine, "wrong machine %x / %x\n",
            info->NativeMachineType, native_machine );
        ok( info->EmulatedMachineType == IMAGE_FILE_MACHINE_I386, "wrong machine %x\n",
            info->EmulatedMachineType );
    }
    ok( buffer[sizeof(*info) / sizeof(ULONG)] == 0xcccccccc, "buffer set %lx\n",
        buffer[sizeof(*info) / sizeof(ULONG)] );
    if (ReadProcessMemory( process, (void *)peb_ptr, peb_data, sizeof(peb_data), &res ))
    {
        ULONG limit = (sizeof(peb_data) - sizeof(info)) / sizeof(ULONG);
        for (i = 0; i < limit; i++)
        {
            if (!memcmp( peb_data + i, info, sizeof(*info) ))
            {
                trace( "wow64info found at %lx\n", i * 4 );
                break;
            }
        }
        ok( i < limit, "wow64info not found in PEB\n" );
    }
    if (info->SectionHandle && info->CrossProcessWorkList)
        test_cross_process_notifications( process, info->SectionHandle, info->CrossProcessWorkList );
    else
        trace( "no WOW64INFO section handle\n" );
}

#if !defined (__REACTOS__) || (DLL_EXPORT_VERSION >= 0x600)
static void test_amd64_shared_info( HANDLE process )
{
    ULONG i, peb_data[0x200], buffer[16];
    PROCESS_BASIC_INFORMATION proc_info;
    NTSTATUS status;
    SIZE_T res;
    BOOLEAN wow64 = 0xcc;
    struct arm64ec_shared_info *info = NULL;

    NtQueryInformationProcess( process, ProcessBasicInformation, &proc_info, sizeof(proc_info), NULL );

    memset( buffer, 0xcc, sizeof(buffer) );
    status = pRtlWow64GetSharedInfoProcess( process, &wow64, (WOW64INFO *)buffer );
    ok( !status, "RtlWow64GetSharedInfoProcess failed %lx\n", status );
    ok( !wow64, "wrong wow64 %u\n", wow64 );
    ok( buffer[0] == 0xcccccccc, "buffer initialized %lx\n", buffer[0] );

    if (ReadProcessMemory( process, (void *)proc_info.PebBaseAddress, peb_data, sizeof(peb_data), &res ))
    {
        ULONG limit = (sizeof(peb_data) - sizeof(*info)) / sizeof(ULONG);
        for (i = 0; i < limit; i++)
        {
            info = (struct arm64ec_shared_info *)(peb_data + i);
             if (info->NativeMachineType == IMAGE_FILE_MACHINE_ARM64 &&
                 info->EmulatedMachineType == IMAGE_FILE_MACHINE_AMD64)
            {
                trace( "shared info found at %lx\n", i * 4 );
                break;
            }
        }
        ok( i < limit, "shared info not found in PEB\n" );
    }
    if (info && info->SectionHandle && info->CrossProcessWorkList)
        test_cross_process_notifications( process, info->SectionHandle, info->CrossProcessWorkList );
    else
        trace( "no shared info section handle\n" );
}
#endif // !defined (__REACTOS__) || (DLL_EXPORT_VERSION >= 0x600)

static void test_peb_teb(void)
{
    PROCESS_BASIC_INFORMATION proc_info;
    THREAD_BASIC_INFORMATION info;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = {0};
    NTSTATUS status;
    void *redir;
    SIZE_T res;
    BOOL ret;
    TEB teb;
    PEB peb;
    TEB32 teb32;
    PEB32 peb32;
    RTL_USER_PROCESS_PARAMETERS params;
    RTL_USER_PROCESS_PARAMETERS32 params32;
    ULONG_PTR peb_ptr;
    ULONG buffer[16];
    WOW64INFO *wow64info = (WOW64INFO *)buffer;
    BOOLEAN wow64;

    Wow64DisableWow64FsRedirection( &redir );

    if (CreateProcessA( "C:\\windows\\syswow64\\msinfo32.exe", NULL, NULL, NULL,
                        FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi ))
    {
        memset( &info, 0xcc, sizeof(info) );
        status = NtQueryInformationThread( pi.hThread, ThreadBasicInformation, &info, sizeof(info), NULL );
        ok( !status, "ThreadBasicInformation failed %lx\n", status );
        if (!ReadProcessMemory( pi.hProcess, info.TebBaseAddress, &teb, sizeof(teb), &res )) res = 0;
        ok( res == sizeof(teb), "wrong len %Ix\n", res );
        ok( teb.Tib.Self == info.TebBaseAddress, "wrong teb %p / %p\n", teb.Tib.Self, info.TebBaseAddress );
        if (is_wow64)
        {
            ok( !!teb.GdiBatchCount, "GdiBatchCount not set\n" );
            ok( (char *)info.TebBaseAddress + teb.WowTebOffset == ULongToPtr(teb.GdiBatchCount) ||
                broken(!NtCurrentTeb()->WowTebOffset),  /* pre-win10 */
                "wrong teb offset %ld\n", teb.WowTebOffset );
        }
        else
        {
            ok( !teb.GdiBatchCount, "GdiBatchCount set\n" );
            ok( teb.WowTebOffset == 0x2000 ||
                broken( !teb.WowTebOffset || teb.WowTebOffset == 1 ),  /* pre-win10 */
                "wrong teb offset %ld\n", teb.WowTebOffset );
            ok( (char *)teb.Tib.ExceptionList == (char *)info.TebBaseAddress + 0x2000,
                "wrong Tib.ExceptionList %p / %p\n",
                (char *)teb.Tib.ExceptionList, (char *)info.TebBaseAddress + 0x2000 );
            if (!ReadProcessMemory( pi.hProcess, teb.Tib.ExceptionList, &teb32, sizeof(teb32), &res )) res = 0;
            ok( res == sizeof(teb32), "wrong len %Ix\n", res );
            ok( (char *)ULongToPtr(teb32.Peb) == (char *)teb.Peb + 0x1000 ||
                broken( ULongToPtr(teb32.Peb) != teb.Peb ), /* vista */
                "wrong peb %p / %p\n", ULongToPtr(teb32.Peb), teb.Peb );
        }

        status = NtQueryInformationProcess( pi.hProcess, ProcessBasicInformation,
                                            &proc_info, sizeof(proc_info), NULL );
        ok( !status, "ProcessBasicInformation failed %lx\n", status );
        ok( proc_info.PebBaseAddress == teb.Peb, "wrong peb %p / %p\n", proc_info.PebBaseAddress, teb.Peb );

        status = NtQueryInformationProcess( pi.hProcess, ProcessWow64Information,
                                            &peb_ptr, sizeof(peb_ptr), NULL );
        ok( !status, "ProcessWow64Information failed %lx\n", status );
        ok( (void *)peb_ptr == (is_wow64 ? teb.Peb : ULongToPtr(teb32.Peb)),
            "wrong peb %p\n", (void *)peb_ptr );

        if (!ReadProcessMemory( pi.hProcess, proc_info.PebBaseAddress, &peb, sizeof(peb), &res )) res = 0;
        ok( res == sizeof(peb), "wrong len %Ix\n", res );
        ok( !peb.BeingDebugged, "BeingDebugged is %u\n", peb.BeingDebugged );
        if (!is_wow64)
        {
            if (!ReadProcessMemory( pi.hProcess, ULongToPtr(teb32.Peb), &peb32, sizeof(peb32), &res )) res = 0;
            ok( res == sizeof(peb32), "wrong len %Ix\n", res );
            ok( !peb32.BeingDebugged, "BeingDebugged is %u\n", peb32.BeingDebugged );
        }

        if (!ReadProcessMemory( pi.hProcess, peb.ProcessParameters, &params, sizeof(params), &res )) res = 0;
        ok( res == sizeof(params), "wrong len %Ix\n", res );
#define CHECK_STR(name) \
        ok( (char *)params.name.Buffer >= (char *)peb.ProcessParameters && \
            (char *)params.name.Buffer < (char *)peb.ProcessParameters + params.Size, \
            "wrong " #name " ptr %p / %p-%p\n", params.name.Buffer, peb.ProcessParameters, \
            (char *)peb.ProcessParameters + params.Size )
        CHECK_STR( ImagePathName );
        CHECK_STR( CommandLine );
        CHECK_STR( WindowTitle );
        CHECK_STR( Desktop );
        CHECK_STR( ShellInfo );
#undef CHECK_STR
        if (!is_wow64)
        {
            ok( peb32.ProcessParameters && ULongToPtr(peb32.ProcessParameters) != peb.ProcessParameters,
                "wrong ptr32 %p / %p\n", ULongToPtr(peb32.ProcessParameters), peb.ProcessParameters );
            if (!ReadProcessMemory( pi.hProcess, ULongToPtr(peb32.ProcessParameters), &params32, sizeof(params32), &res )) res = 0;
            ok( res == sizeof(params32), "wrong len %Ix\n", res );
#define CHECK_STR(name) \
            ok( ULongToPtr(params32.name.Buffer) >= ULongToPtr(peb32.ProcessParameters) && \
                ULongToPtr(params32.name.Buffer) < ULongToPtr(peb32.ProcessParameters + params32.Size), \
                "wrong " #name " ptr %lx / %lx-%lx\n", params32.name.Buffer, peb32.ProcessParameters, \
                peb32.ProcessParameters + params.Size ); \
            ok( params32.name.Length == params.name.Length, "wrong " #name "len %u / %u\n", \
                params32.name.Length, params.name.Length )
            CHECK_STR( ImagePathName );
            CHECK_STR( CommandLine );
            CHECK_STR( WindowTitle );
            CHECK_STR( Desktop );
            CHECK_STR( ShellInfo );
#undef CHECK_STR
            ok( params32.EnvironmentSize == params.EnvironmentSize, "wrong size %lu / %Iu\n",
                params32.EnvironmentSize, params.EnvironmentSize );
        }

        ResumeThread( pi.hThread );
        WaitForInputIdle( pi.hProcess, 1000 );

        if (pRtlWow64GetSharedInfoProcess) test_wow64_shared_info( pi.hProcess );
        else win_skip( "RtlWow64GetSharedInfoProcess not supported\n" );

        ret = DebugActiveProcess( pi.dwProcessId );
        ok( ret, "debugging failed\n" );
        if (!ReadProcessMemory( pi.hProcess, proc_info.PebBaseAddress, &peb, sizeof(peb), &res )) res = 0;
        ok( res == sizeof(peb), "wrong len %Ix\n", res );
        ok( peb.BeingDebugged == !!ret, "BeingDebugged is %u\n", peb.BeingDebugged );
        if (!is_wow64)
        {
            if (!ReadProcessMemory( pi.hProcess, ULongToPtr(teb32.Peb), &peb32, sizeof(peb32), &res )) res = 0;
            ok( res == sizeof(peb32), "wrong len %Ix\n", res );
            ok( peb32.BeingDebugged == !!ret, "BeingDebugged is %u\n", peb32.BeingDebugged );
        }

        TerminateProcess( pi.hProcess, 0 );
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
    }

#if !defined (__REACTOS__) || (DLL_EXPORT_VERSION >= 0x600)
    if (is_win64 && native_machine == IMAGE_FILE_MACHINE_ARM64 &&
        create_process_machine( (char *)"C:\\windows\\system32\\regsvr32.exe /?", CREATE_SUSPENDED,
                                IMAGE_FILE_MACHINE_AMD64, &pi ))
    {
        memset( &info, 0xcc, sizeof(info) );
        status = NtQueryInformationThread( pi.hThread, ThreadBasicInformation, &info, sizeof(info), NULL );
        ok( !status, "ThreadBasicInformation failed %lx\n", status );
        if (!ReadProcessMemory( pi.hProcess, info.TebBaseAddress, &teb, sizeof(teb), &res )) res = 0;
        ok( res == sizeof(teb), "wrong len %Ix\n", res );
        ok( teb.Tib.Self == info.TebBaseAddress, "wrong teb %p / %p\n", teb.Tib.Self, info.TebBaseAddress );
        ok( !teb.GdiBatchCount, "GdiBatchCount set\n" );
        ok( !teb.WowTebOffset, "wrong teb offset %ld\n", teb.WowTebOffset );
        ok( !teb.Tib.ExceptionList, "wrong Tib.ExceptionList %p\n", (char *)teb.Tib.ExceptionList );

        status = NtQueryInformationProcess( pi.hProcess, ProcessBasicInformation,
                                            &proc_info, sizeof(proc_info), NULL );
        ok( !status, "ProcessBasicInformation failed %lx\n", status );
        ok( proc_info.PebBaseAddress == teb.Peb, "wrong peb %p / %p\n", proc_info.PebBaseAddress, teb.Peb );

        status = NtQueryInformationProcess( pi.hProcess, ProcessWow64Information,
                                            &peb_ptr, sizeof(peb_ptr), NULL );
        ok( !status, "ProcessWow64Information failed %lx\n", status );
        ok( !peb_ptr, "wrong peb %p\n", (void *)peb_ptr );

        if (!ReadProcessMemory( pi.hProcess, proc_info.PebBaseAddress, &peb, sizeof(peb), &res )) res = 0;
        ok( res == sizeof(peb), "wrong len %Ix\n", res );
        ok( !peb.BeingDebugged, "BeingDebugged is %u\n", peb.BeingDebugged );

        ResumeThread( pi.hThread );
        WaitForInputIdle( pi.hProcess, 1000 );

        test_amd64_shared_info( pi.hProcess );

        TerminateProcess( pi.hProcess, 0 );
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
    }
#endif

    if (CreateProcessA( "C:\\windows\\system32\\msinfo32.exe", NULL, NULL, NULL,
                        FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi ))
    {
        memset( &info, 0xcc, sizeof(info) );
        status = NtQueryInformationThread( pi.hThread, ThreadBasicInformation, &info, sizeof(info), NULL );
        ok( !status, "ThreadBasicInformation failed %lx\n", status );
        if (!is_wow64)
        {
            if (!ReadProcessMemory( pi.hProcess, info.TebBaseAddress, &teb, sizeof(teb), &res )) res = 0;
            ok( res == sizeof(teb), "wrong len %Ix\n", res );
            ok( teb.Tib.Self == info.TebBaseAddress, "wrong teb %p / %p\n",
                teb.Tib.Self, info.TebBaseAddress );
            ok( !teb.GdiBatchCount, "GdiBatchCount set\n" );
            ok( !teb.WowTebOffset || broken( teb.WowTebOffset == 1 ),  /* vista */
                "wrong teb offset %ld\n", teb.WowTebOffset );
        }
        else ok( !info.TebBaseAddress, "got teb %p\n", info.TebBaseAddress );

        status = NtQueryInformationProcess( pi.hProcess, ProcessBasicInformation,
                                            &proc_info, sizeof(proc_info), NULL );
        ok( !status, "ProcessBasicInformation failed %lx\n", status );
        if (is_wow64)
            ok( !proc_info.PebBaseAddress ||
                broken( (char *)proc_info.PebBaseAddress >= (char *)0x7f000000 ), /* vista */
                "wrong peb %p\n", proc_info.PebBaseAddress );
        else
            ok( proc_info.PebBaseAddress == teb.Peb, "wrong peb %p / %p\n",
                proc_info.PebBaseAddress, teb.Peb );

        ResumeThread( pi.hThread );
        WaitForInputIdle( pi.hProcess, 1000 );

        if (pRtlWow64GetSharedInfoProcess)
        {
            wow64 = 0xcc;
            memset( buffer, 0xcc, sizeof(buffer) );
            status = pRtlWow64GetSharedInfoProcess( pi.hProcess, &wow64, wow64info );
            ok( !status, "RtlWow64GetSharedInfoProcess failed %lx\n", status );
            ok( !wow64, "wrong wow64 %u\n", wow64 );
            ok( buffer[0] == 0xcccccccc, "buffer set %lx\n", buffer[0] );
        }

        TerminateProcess( pi.hProcess, 0 );
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
    }

    Wow64RevertWow64FsRedirection( redir );

#ifndef _WIN64
    if (is_wow64)
    {
        PEB64 *peb64;
        TEB64 *teb64 = (TEB64 *)NtCurrentTeb()->GdiBatchCount;

        ok( !!teb64, "GdiBatchCount not set\n" );
        ok( (char *)NtCurrentTeb() + NtCurrentTeb()->WowTebOffset == (char *)teb64 ||
            broken(!NtCurrentTeb()->WowTebOffset),  /* pre-win10 */
            "wrong WowTebOffset %lx (%p/%p)\n", NtCurrentTeb()->WowTebOffset, teb64, NtCurrentTeb() );
        ok( (char *)teb64 + 0x2000 == (char *)NtCurrentTeb(), "unexpected diff %p / %p\n",
            teb64, NtCurrentTeb() );
        ok( (char *)teb64 + teb64->WowTebOffset == (char *)NtCurrentTeb() ||
            broken( !teb64->WowTebOffset || teb64->WowTebOffset == 1 ),  /* pre-win10 */
            "wrong WowTebOffset %lx (%p/%p)\n", teb64->WowTebOffset, teb64, NtCurrentTeb() );
        ok( !teb64->GdiBatchCount, "GdiBatchCount set %lx\n", teb64->GdiBatchCount );
        ok( teb64->Tib.ExceptionList == PtrToUlong( NtCurrentTeb() ), "wrong Tib.ExceptionList %s / %p\n",
            wine_dbgstr_longlong(teb64->Tib.ExceptionList), NtCurrentTeb() );
        ok( teb64->Tib.Self == PtrToUlong( teb64 ), "wrong Tib.Self %s / %p\n",
            wine_dbgstr_longlong(teb64->Tib.Self), teb64 );
        ok( teb64->StaticUnicodeString.Buffer == PtrToUlong( teb64->StaticUnicodeBuffer ),
            "wrong StaticUnicodeString %s / %p\n",
            wine_dbgstr_longlong(teb64->StaticUnicodeString.Buffer), teb64->StaticUnicodeBuffer );
        ok( teb64->ClientId.UniqueProcess == GetCurrentProcessId(), "wrong pid %s / %lx\n",
            wine_dbgstr_longlong(teb64->ClientId.UniqueProcess), GetCurrentProcessId() );
        ok( teb64->ClientId.UniqueThread == GetCurrentThreadId(), "wrong tid %s / %lx\n",
            wine_dbgstr_longlong(teb64->ClientId.UniqueThread), GetCurrentThreadId() );
        peb64 = ULongToPtr( teb64->Peb );
        ok( peb64->ImageBaseAddress == PtrToUlong( NtCurrentTeb()->Peb->ImageBaseAddress ),
            "wrong ImageBaseAddress %s / %p\n",
            wine_dbgstr_longlong(peb64->ImageBaseAddress), NtCurrentTeb()->Peb->ImageBaseAddress);
        ok( peb64->OSBuildNumber == NtCurrentTeb()->Peb->OSBuildNumber, "wrong OSBuildNumber %lx / %lx\n",
            peb64->OSBuildNumber, NtCurrentTeb()->Peb->OSBuildNumber );
        ok( peb64->OSPlatformId == NtCurrentTeb()->Peb->OSPlatformId, "wrong OSPlatformId %lx / %lx\n",
            peb64->OSPlatformId, NtCurrentTeb()->Peb->OSPlatformId );
        ok( peb64->AnsiCodePageData == PtrToUlong( NtCurrentTeb()->Peb->AnsiCodePageData ),
            "wrong AnsiCodePageData %I64x / %p\n",
            peb64->AnsiCodePageData, NtCurrentTeb()->Peb->AnsiCodePageData );
        ok( peb64->OemCodePageData == PtrToUlong( NtCurrentTeb()->Peb->OemCodePageData ),
            "wrong OemCodePageData %I64x / %p\n",
            peb64->OemCodePageData, NtCurrentTeb()->Peb->OemCodePageData );
        ok( peb64->UnicodeCaseTableData == PtrToUlong( NtCurrentTeb()->Peb->UnicodeCaseTableData ),
            "wrong UnicodeCaseTableData %I64x / %p\n",
            peb64->UnicodeCaseTableData, NtCurrentTeb()->Peb->UnicodeCaseTableData );
        return;
    }
#endif
    ok( !NtCurrentTeb()->GdiBatchCount, "GdiBatchCount set to %lx\n", NtCurrentTeb()->GdiBatchCount );
    ok( !NtCurrentTeb()->WowTebOffset || broken( NtCurrentTeb()->WowTebOffset == 1 ), /* vista */
        "WowTebOffset set to %lx\n", NtCurrentTeb()->WowTebOffset );
}

static void test_selectors(void)
{
#ifndef __arm__
    THREAD_DESCRIPTOR_INFORMATION info;
    NTSTATUS status;
    ULONG base, limit, sel, retlen;
    I386_CONTEXT context = { CONTEXT_I386_CONTROL | CONTEXT_I386_SEGMENTS };

#ifdef _WIN64
    if (!pRtlWow64GetThreadSelectorEntry)
    {
        win_skip( "RtlWow64GetThreadSelectorEntry not supported\n" );
        return;
    }
    if (!pRtlWow64GetThreadContext || pRtlWow64GetThreadContext( GetCurrentThread(), &context ))
    {
        /* hardcoded values */
#ifdef __arm64ec__
        context.SegCs = 0x23;
        context.SegSs = 0x2b;
        context.SegFs = 0x53;
#elif defined __x86_64__
#ifdef _MSC_VER
        context.SegFs = __readsegfs();
        context.SegSs = __readsegss();
#else
        __asm__( "movw %%fs,%0" : "=m" (context.SegFs) );
        __asm__( "movw %%ss,%0" : "=m" (context.SegSs) );
#endif
#else
        context.SegCs = 0x1b;
        context.SegSs = 0x23;
        context.SegFs = 0x3b;
#endif
    }
#define GET_ENTRY(info,size,ret) \
    pRtlWow64GetThreadSelectorEntry( GetCurrentThread(), info, size, ret )

#else
    GetThreadContext( GetCurrentThread(), &context );
#define GET_ENTRY(info,size,ret) \
    NtQueryInformationThread( GetCurrentThread(), ThreadDescriptorTableEntry, info, size, ret )
#endif

    trace( "cs %04lx ss %04lx fs %04lx\n", context.SegCs, context.SegSs, context.SegFs );
    retlen = 0xdeadbeef;
    info.Selector = 0;
    status = GET_ENTRY( &info, sizeof(info) - 1, &retlen );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "wrong status %lx\n", status );
    ok( retlen == 0xdeadbeef, "len set %lu\n", retlen );

    retlen = 0xdeadbeef;
    status = GET_ENTRY( &info, sizeof(info) + 1, &retlen );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "wrong status %lx\n", status );
    ok( retlen == 0xdeadbeef, "len set %lu\n", retlen );

    retlen = 0xdeadbeef;
    status = GET_ENTRY( NULL, 0, &retlen );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "wrong status %lx\n", status );
    ok( retlen == 0xdeadbeef, "len set %lu\n", retlen );

    status = GET_ENTRY( &info, sizeof(info), NULL );
    ok( !status, "wrong status %lx\n", status );

    for (info.Selector = 0; info.Selector < 0x100; info.Selector++)
    {
        retlen = 0xdeadbeef;
        status = GET_ENTRY( &info, sizeof(info), &retlen );
        base = (info.Entry.BaseLow |
                (info.Entry.HighWord.Bytes.BaseMid << 16) |
                (info.Entry.HighWord.Bytes.BaseHi << 24));
        limit = (info.Entry.LimitLow | info.Entry.HighWord.Bits.LimitHi << 16);
        sel = info.Selector | 3;

        if (sel == 0x03)  /* null selector */
        {
            ok( !status, "wrong status %lx\n", status );
            ok( retlen == sizeof(info.Entry), "len set %lu\n", retlen );
            ok( !base, "wrong base %lx\n", base );
            ok( !limit, "wrong limit %lx\n", limit );
            ok( !info.Entry.HighWord.Bytes.Flags1, "wrong flags1 %x\n", info.Entry.HighWord.Bytes.Flags1 );
            ok( !info.Entry.HighWord.Bytes.Flags2, "wrong flags2 %x\n", info.Entry.HighWord.Bytes.Flags2 );
        }
        else if (sel == context.SegCs)  /* 32-bit code selector */
        {
            ok( !status, "wrong status %lx\n", status );
            ok( retlen == sizeof(info.Entry), "len set %lu\n", retlen );
            ok( !base, "wrong base %lx\n", base );
            ok( limit == 0xfffff, "wrong limit %lx\n", limit );
            ok( info.Entry.HighWord.Bits.Type == 0x1b, "wrong type %x\n", info.Entry.HighWord.Bits.Type );
            ok( info.Entry.HighWord.Bits.Dpl == 3, "wrong dpl %x\n", info.Entry.HighWord.Bits.Dpl );
            ok( info.Entry.HighWord.Bits.Pres, "wrong pres\n" );
            ok( !info.Entry.HighWord.Bits.Sys, "wrong sys\n" );
            ok( info.Entry.HighWord.Bits.Default_Big, "wrong big\n" );
            ok( info.Entry.HighWord.Bits.Granularity, "wrong granularity\n" );
        }
        else if (sel == context.SegSs)  /* 32-bit data selector */
        {
            ok( !status, "wrong status %lx\n", status );
            ok( retlen == sizeof(info.Entry), "len set %lu\n", retlen );
            ok( !base, "wrong base %lx\n", base );
            ok( limit == 0xfffff, "wrong limit %lx\n", limit );
            ok( info.Entry.HighWord.Bits.Type == 0x13, "wrong type %x\n", info.Entry.HighWord.Bits.Type );
            ok( info.Entry.HighWord.Bits.Dpl == 3, "wrong dpl %x\n", info.Entry.HighWord.Bits.Dpl );
            ok( info.Entry.HighWord.Bits.Pres, "wrong pres\n" );
            ok( !info.Entry.HighWord.Bits.Sys, "wrong sys\n" );
            ok( info.Entry.HighWord.Bits.Default_Big, "wrong big\n" );
            ok( info.Entry.HighWord.Bits.Granularity, "wrong granularity\n" );
        }
        else if (sel == context.SegFs)  /* TEB selector */
        {
            ok( !status, "wrong status %lx\n", status );
            ok( retlen == sizeof(info.Entry), "len set %lu\n", retlen );
#ifdef _WIN64
            if (NtCurrentTeb()->WowTebOffset == 0x2000)
                ok( base == (ULONG_PTR)NtCurrentTeb() + 0x2000, "wrong base %lx / %p\n",
                    base, NtCurrentTeb() );
#else
            ok( base == (ULONG_PTR)NtCurrentTeb(), "wrong base %lx / %p\n", base, NtCurrentTeb() );
#endif
            ok( limit == 0xfff || broken(limit == 0x4000),  /* <= win8 */
                "wrong limit %lx\n", limit );
            ok( info.Entry.HighWord.Bits.Type == 0x13, "wrong type %x\n", info.Entry.HighWord.Bits.Type );
            ok( info.Entry.HighWord.Bits.Dpl == 3, "wrong dpl %x\n", info.Entry.HighWord.Bits.Dpl );
            ok( info.Entry.HighWord.Bits.Pres, "wrong pres\n" );
            ok( !info.Entry.HighWord.Bits.Sys, "wrong sys\n" );
            ok( info.Entry.HighWord.Bits.Default_Big, "wrong big\n" );
            ok( !info.Entry.HighWord.Bits.Granularity, "wrong granularity\n" );
        }
        else if (!status)
        {
            ok( retlen == sizeof(info.Entry), "len set %lu\n", retlen );
            trace( "succeeded for %lx base %lx limit %lx type %x\n",
                   sel, base, limit, info.Entry.HighWord.Bits.Type );
        }
        else
        {
            ok( status == STATUS_UNSUCCESSFUL ||
                ((sel & 4) && (status == STATUS_NO_LDT)) ||
                broken( status == STATUS_ACCESS_VIOLATION),  /* <= win8 */
                "%lx: wrong status %lx\n", info.Selector, status );
            ok( retlen == 0xdeadbeef, "len set %lu\n", retlen );
        }
    }
#undef GET_ENTRY
#endif /* __arm__ */
}

static void test_image_mappings(void)
{
    MEM_EXTENDED_PARAMETER ext = { .Type = MemExtendedParameterImageMachine };
    HANDLE file, mapping, process = GetCurrentProcess();
    NTSTATUS status;
    SIZE_T size;
    LARGE_INTEGER offset;
    void *ptr;

    if (!pNtMapViewOfSectionEx)
    {
        win_skip( "NtMapViewOfSectionEx() not supported\n" );
        return;
    }

    offset.QuadPart = 0;
    file = CreateFileA( "c:\\windows\\system32\\version.dll", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "Failed to open version.dll\n" );
    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL );
    ok( mapping != 0, "CreateFileMapping failed\n" );
    CloseHandle( file );

    ptr = NULL;
    size = 0;
    ext.ULong = IMAGE_FILE_MACHINE_AMD64;
    status = pNtMapViewOfSectionEx( mapping, process, &ptr, &offset, &size, 0, PAGE_READONLY, &ext, 1 );
    if (status == STATUS_INVALID_PARAMETER)
    {
        win_skip( "MemExtendedParameterImageMachine not supported\n" );
        NtClose( mapping );
        return;
    }
    if (current_machine == IMAGE_FILE_MACHINE_AMD64)
    {
        ok( status == STATUS_SUCCESS || status == STATUS_IMAGE_NOT_AT_BASE,
            "NtMapViewOfSection returned %08lx\n", status );
        NtUnmapViewOfSection( process, ptr );
    }
    else if (current_machine == IMAGE_FILE_MACHINE_ARM64)
    {
        todo_wine
        ok( status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH, "NtMapViewOfSection returned %08lx\n", status );
        NtUnmapViewOfSection( process, ptr );
    }
    else ok( status == STATUS_NOT_SUPPORTED, "NtMapViewOfSection returned %08lx\n", status );

    ptr = NULL;
    size = 0;
    ext.ULong = IMAGE_FILE_MACHINE_I386;
    status = pNtMapViewOfSectionEx( mapping, process, &ptr, &offset, &size, 0, PAGE_READONLY, &ext, 1 );
    if (current_machine == IMAGE_FILE_MACHINE_I386)
    {
        ok( status == STATUS_SUCCESS || status == STATUS_IMAGE_NOT_AT_BASE,
            "NtMapViewOfSection returned %08lx\n", status );
        NtUnmapViewOfSection( process, ptr );
    }
    else ok( status == STATUS_NOT_SUPPORTED, "NtMapViewOfSection returned %08lx\n", status );

    ptr = NULL;
    size = 0;
    ext.ULong = IMAGE_FILE_MACHINE_ARM64;
    status = pNtMapViewOfSectionEx( mapping, process, &ptr, &offset, &size, 0, PAGE_READONLY, &ext, 1 );
    if (native_machine == IMAGE_FILE_MACHINE_ARM64)
    {
        switch (current_machine)
        {
        case IMAGE_FILE_MACHINE_ARM64:
            ok( status == STATUS_SUCCESS || status == STATUS_IMAGE_NOT_AT_BASE,
                "NtMapViewOfSection returned %08lx\n", status );
            NtUnmapViewOfSection( process, ptr );
            break;
        case IMAGE_FILE_MACHINE_AMD64:
            ok( status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH, "NtMapViewOfSection returned %08lx\n", status );
            NtUnmapViewOfSection( process, ptr );
            break;
        default:
            ok( status == STATUS_NOT_SUPPORTED, "NtMapViewOfSection returned %08lx\n", status );
            break;
        }
    }
    else ok( status == STATUS_NOT_SUPPORTED, "NtMapViewOfSection returned %08lx\n", status );

    ptr = NULL;
    size = 0;
    ext.ULong = IMAGE_FILE_MACHINE_R3000;
    status = pNtMapViewOfSectionEx( mapping, process, &ptr, &offset, &size, 0, PAGE_READONLY, &ext, 1 );
    ok( status == STATUS_NOT_SUPPORTED, "NtMapViewOfSection returned %08lx\n", status );

    ptr = NULL;
    size = 0;
    ext.ULong = 0;
    status = pNtMapViewOfSectionEx( mapping, process, &ptr, &offset, &size, 0, PAGE_READONLY, &ext, 1 );
    ok( status == STATUS_SUCCESS || status == STATUS_IMAGE_NOT_AT_BASE,
        "NtMapViewOfSection returned %08lx\n", status );
    NtUnmapViewOfSection( process, ptr );

    NtClose( mapping );

    if (is_wow64)
    {
        file = CreateFileA( "c:\\windows\\sysnative\\version.dll", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0 );
        ok( file != INVALID_HANDLE_VALUE, "Failed to open version.dll\n" );

        mapping = CreateFileMappingA( file, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL );
        ok( mapping != 0, "CreateFileMapping failed\n" );
        CloseHandle( file );

        ptr = NULL;
        size = 0;
        ext.ULong = native_machine;
        status = pNtMapViewOfSectionEx( mapping, process, &ptr, &offset, &size, 0, PAGE_READONLY, &ext, 1 );
        ok( status == STATUS_SUCCESS || status == STATUS_IMAGE_NOT_AT_BASE,
            "NtMapViewOfSection returned %08lx\n", status );
        NtUnmapViewOfSection( process, ptr );

        ptr = NULL;
        size = 0;
        ext.ULong = IMAGE_FILE_MACHINE_I386;
        status = pNtMapViewOfSectionEx( mapping, process, &ptr, &offset, &size, 0, PAGE_READONLY, &ext, 1 );
        ok( status == STATUS_NOT_SUPPORTED, "NtMapViewOfSection returned %08lx\n", status );
        NtClose( mapping );
    }
    else if (native_machine == IMAGE_FILE_MACHINE_AMD64 || native_machine == IMAGE_FILE_MACHINE_ARM64)
    {
        file = CreateFileA( "c:\\windows\\syswow64\\version.dll", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
        ok( file != INVALID_HANDLE_VALUE, "Failed to open version.dll\n" );

        mapping = CreateFileMappingA( file, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL );
        ok( mapping != 0, "CreateFileMapping failed\n" );
        CloseHandle( file );

        ptr = NULL;
        size = 0;
        ext.ULong = native_machine;
        status = pNtMapViewOfSectionEx( mapping, process, &ptr, &offset, &size, 0, PAGE_READONLY, &ext, 1 );
        ok( status == STATUS_NOT_SUPPORTED, "NtMapViewOfSection returned %08lx\n", status );

        ptr = NULL;
        size = 0;
        ext.ULong = IMAGE_FILE_MACHINE_I386;
        status = pNtMapViewOfSectionEx( mapping, process, &ptr, &offset, &size, 0, PAGE_READONLY, &ext, 1 );
        ok( status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH, "NtMapViewOfSection returned %08lx\n", status );
        NtUnmapViewOfSection( process, ptr );
        NtClose( mapping );
    }
}

static DWORD hook_code[] =
{
    0x58000048, /* ldr x8, 1f */
    0xd61f0100, /* br x8 */
    0, 0        /* 1: .quad ptr */
};

static const DWORD log_params_code[] =
{
    0x10008009, /* adr x9, .+0x1000 */
    0xf940012a, /* ldr x10, [x9] */
    0xa8810540, /* stp x0, x1, [x10], #0x10 */
    0xa8810d42, /* stp x2, x3, [x10], #0x10 */
    0xa8811544, /* stp x4, x5, [x10], #0x10 */
    0xa8811d46, /* stp x6, x7, [x10], #0x10 */
    0xf900012a, /* str x10, [x9] */
    0xf9400520, /* ldr x0, [x9, #0x8] */
    0xd65f03c0, /* ret */
};

static void CALLBACK dummy_apc( ULONG_PTR arg )
{
}

struct expected_notification
{
    UINT    nb_args;
    ULONG64 args[6];
};

static void reset_results( ULONG64 *results )
{
    memset( results + 1, 0xcc, 0x1000 - sizeof(*results) );
    results[0] = (ULONG_PTR)(results + 2);
}

#define expect_notifications(results, count, expect, syscall) \
    expect_notifications_(results, count, expect, syscall, __LINE__)
static void expect_notifications_( ULONG64 *results, UINT count, const struct expected_notification *expect,
                                   BOOL syscall, int line )
{
    ULONG64 *regs = results + 2;
    UINT i, j, len = (results[0] - (ULONG_PTR)regs) / 8 / sizeof(*regs);

#ifdef _WIN64
    if (syscall)
    {
        CHPE_V2_CPU_AREA_INFO *cpu_area = NtCurrentTeb()->ChpeV2CpuAreaInfo;
        if (cpu_area && cpu_area->InSyscallCallback) count = 0;
    }
#endif

    ok_(__FILE__,line)( count == len, "wrong notification count %u / %u\n", len, count );
    for (i = 0; i < min( count, len ); i++, expect++, regs += 8)
        for (j = 0; j < expect->nb_args; j++)
            ok_(__FILE__,line)( regs[j] == expect->args[j], "%u: wrong args[%u] %I64x / %I64x\n",
                                i, j, regs[j], expect->args[j] );
    reset_results( results );
}

static void add_work_item( CROSS_PROCESS_WORK_LIST *list, UINT id, ULONG64 addr, ULONG64 size,
                           UINT arg0, UINT arg1, UINT arg2, UINT arg3 )
{
    CROSS_PROCESS_WORK_ENTRY *entry = pop_from_free_list( &list->free_list );

    entry->id = id;
    entry->addr = addr;
    entry->size = size;
    entry->args[0] = arg0;
    entry->args[1] = arg1;
    entry->args[2] = arg2;
    entry->args[3] = arg3;
    push_onto_work_list( &list->work_list, entry );
}

static void process_work_items(void)
{
#ifdef _WIN64
    if (pProcessPendingCrossProcessEmulatorWork)
    {
        pProcessPendingCrossProcessEmulatorWork();
        return;
    }
#endif
    QueueUserAPC( dummy_apc, GetCurrentThread(), 0 );
    SleepEx( 1, TRUE );
}

static BYTE old_code[sizeof(hook_code)];

static void *hook_notification_function( HMODULE module, const char *win32_name, const char *win64_name )
{
    BYTE *ptr;
    BOOL ret;

    if (current_machine == IMAGE_FILE_MACHINE_AMD64)
    {
        static const BYTE fast_forward[] = { 0x48, 0x8b, 0xc4, 0x48, 0x89, 0x58, 0x20, 0x55, 0x5d, 0xe9 };

        if (!(ptr = pRtlFindExportedRoutineByName( module, win64_name )))
        {
            skip( "%s not exported\n", win64_name  );
            return NULL;
        }
        if (memcmp( ptr, fast_forward, sizeof(fast_forward) ))
        {
            skip( "unrecognized x64 thunk for %s\n", win64_name  );
            return NULL;
        }
        ptr += sizeof(fast_forward);
        ptr += sizeof(LONG) + *(LONG *)ptr;
    }
    else if (!(ptr = pRtlFindExportedRoutineByName( module, win32_name )))
    {
        skip( "%s not exported\n", win32_name  );
        return NULL;
    }

    memcpy( old_code, ptr, sizeof(old_code) );
    ret = WriteProcessMemory( GetCurrentProcess(), ptr, hook_code, sizeof(hook_code), NULL );
    ok( ret, "hooking failed %p %lu\n", ptr, GetLastError() );
    return ptr;
}

static void test_notifications( HMODULE module, CROSS_PROCESS_WORK_LIST *list )
{
    void *code, *ptr, *addr = NULL;
    DWORD old_prot;
    SIZE_T size;
    ULONG64 *results;
    NTSTATUS status;
    HANDLE file, mapping;

    code = VirtualAlloc( NULL, 0x2000, MEM_COMMIT, PAGE_READWRITE );
    memcpy( code, log_params_code, sizeof(log_params_code) );
    VirtualProtect( code, 0x1000, PAGE_EXECUTE_READ, &old_prot );
    *(void **)&hook_code[2] = code;

    results = (ULONG64 *)((char *)code + 0x1000);
    reset_results( results );

    file = CreateFileA( "c:\\windows\\system32\\version.dll", GENERIC_READ | GENERIC_EXECUTE,
                        FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "Failed to open version.dll\n" );
    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL );
    ok( mapping != 0, "CreateFileMapping failed\n" );

    if ((ptr = hook_notification_function( module, "BTCpuNotifyMemoryAlloc", "NotifyMemoryAlloc" )))
    {
        struct expected_notification expect_cross[2] =
        {
            { 6, { 0x1234567890, 0x6543210000, MEM_COMMIT, PAGE_EXECUTE_READ, 0, 0 } },
            { 6, { 0x1234567890, 0x6543210000, MEM_COMMIT, PAGE_EXECUTE_READ, 1, 0xdeadbeef } }
        };
        struct expected_notification expect_alloc[2] =
        {
            { 6, { 0, 0x123456, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE, 0, 0 } },
            { 6, { 0, 0x124000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE, 1, 0 } }
        };

        add_work_item( list, CrossProcessPreVirtualAlloc, expect_cross[0].args[0], expect_cross[0].args[1],
                       expect_cross[0].args[2], expect_cross[0].args[3], 0, 0 );
        add_work_item( list, CrossProcessPostVirtualAlloc, expect_cross[1].args[0], expect_cross[1].args[1],
                       expect_cross[1].args[2], expect_cross[1].args[3], 0xdeadbeef, 0 );
        process_work_items();
        expect_notifications( results, 2, expect_cross, FALSE );
        ok( !list->work_list.first, "list not empty\n" );

        size = expect_alloc[0].args[1];
        status = NtAllocateVirtualMemory( GetCurrentProcess(), &addr, 0, &size, MEM_COMMIT, PAGE_READWRITE );
        ok( !status, "NtAllocateVirtualMemory failed %lx\n", status );
        expect_alloc[1].args[0] = (ULONG_PTR)addr;
        expect_notifications( results, 2, expect_alloc, TRUE );
        WriteProcessMemory( GetCurrentProcess(), ptr, old_code, sizeof(old_code), NULL );
    }

    if ((ptr = hook_notification_function( module, "BTCpuNotifyMemoryProtect", "NotifyMemoryProtect" )))
    {
        struct expected_notification expect_cross[2] =
        {
            { 5, { 0x1234567890, 0x6543210000, PAGE_READWRITE, 0, 0 } },
            { 5, { 0x1234567890, 0x6543210000, PAGE_READWRITE, 1, 0xdeadbeef } }
        };
        struct expected_notification expect_protect[2] =
        {
            { 5, { 0, 0x123456, PAGE_EXECUTE_READ, 0, 0 } },
            { 5, { 0, 0x124000, PAGE_EXECUTE_READ, 1, 0 } }
        };

        reset_results( results );
        add_work_item( list, CrossProcessPreVirtualProtect, expect_cross[0].args[0],
                       expect_cross[0].args[1], expect_cross[0].args[2], 0, 0, 0 );
        add_work_item( list, CrossProcessPostVirtualProtect, expect_cross[1].args[0],
                       expect_cross[1].args[1], expect_cross[1].args[2], 0xdeadbeef, 0, 0 );
        process_work_items();
        expect_notifications( results, 2, expect_cross, FALSE );
        ok( !list->work_list.first, "list not empty\n" );

        expect_protect[1].args[0] = (ULONG_PTR)addr;
        addr = (char *)addr + 0x123;
        expect_protect[0].args[0] = (ULONG_PTR)addr;
        size = expect_protect[0].args[1];
        status = NtProtectVirtualMemory( GetCurrentProcess(), &addr, &size, PAGE_EXECUTE_READ, &old_prot );
        ok( !status, "NtProtectVirtualMemory failed %lx\n", status );
        expect_notifications( results, 2, expect_protect, TRUE );

        WriteProcessMemory( GetCurrentProcess(), ptr, old_code, sizeof(old_code), NULL );
        reset_results( results );
    }

    if ((ptr = hook_notification_function( module, "BTCpuNotifyMemoryFree", "NotifyMemoryFree" )))
    {
        struct expected_notification expect_cross[2] =
        {
            { 5, { 0x1234567890, 0x6543210000, MEM_RELEASE, 0, 0 } },
            { 5, { 0x1234567890, 0x6543210000, MEM_RELEASE, 1, 0xdeadbeef } }
        };
        struct expected_notification expect_free[2] =
        {
            { 5, { 0, 0x123456, MEM_RELEASE, 0, 0 } },
            { 5, { 0, 0x124000, MEM_RELEASE, 1, 0 } }
        };

        add_work_item( list, CrossProcessPreVirtualFree, expect_cross[0].args[0],
                       expect_cross[0].args[1], expect_cross[0].args[2], 0, 0, 0 );
        add_work_item( list, CrossProcessPostVirtualFree, expect_cross[1].args[0],
                       expect_cross[1].args[1], expect_cross[1].args[2], 0xdeadbeef, 0, 0 );
        process_work_items();
        expect_notifications( results, 2, expect_cross, FALSE );
        ok( !list->work_list.first, "list not empty\n" );

        expect_free[0].args[0] = (ULONG_PTR)addr;
        expect_free[1].args[0] = (ULONG_PTR)addr;
        size = expect_free[0].args[1];
        status = NtFreeVirtualMemory( GetCurrentProcess(), &addr, &size, MEM_RELEASE );
        ok( !status, "NtFreeVirtualMemory failed %lx\n", status );
        expect_notifications( results, 2, expect_free, TRUE );

        WriteProcessMemory( GetCurrentProcess(), ptr, old_code, sizeof(old_code), NULL );
    }

    if ((ptr = hook_notification_function( module, "BTCpuNotifyMemoryDirty", "BTCpu64NotifyMemoryDirty" )))
    {
        struct expected_notification expect = { 2, { 0x1234567890, 0x6543210000 } };

        add_work_item( list, CrossProcessMemoryWrite, expect.args[0], expect.args[1], 0, 0, 0, 0 );
        process_work_items();
        expect_notifications( results, 1, &expect, FALSE );
        ok( !list->work_list.first, "list not empty\n" );

        WriteProcessMemory( GetCurrentProcess(), ptr, old_code, sizeof(old_code), NULL );
    }

    if ((ptr = hook_notification_function( module, "BTCpuFlushInstructionCache2", "BTCpu64FlushInstructionCache" )))
    {
        struct expected_notification expect_cross = { 2, { 0x1234567890, 0x6543210000 } };
        struct expected_notification expect_flush = { 2, { 0, 0x1234 } };

        reset_results( results );
        add_work_item(list, CrossProcessFlushCache, expect_cross.args[0],
                      expect_cross.args[1], 0, 0, 0, 0 );
        process_work_items();
        expect_notifications( results, 1, &expect_cross, FALSE );
        ok( !list->work_list.first, "list not empty\n" );

        expect_flush.args[0] = (ULONG_PTR)ptr;
        NtFlushInstructionCache( GetCurrentProcess(), ptr, expect_flush.args[1] );
        expect_notifications( results, 1, &expect_flush, TRUE );

        WriteProcessMemory( GetCurrentProcess(), ptr, old_code, sizeof(old_code), NULL );
    }

    if ((ptr = hook_notification_function( module, "BTCpuFlushInstructionCacheHeavy", "FlushInstructionCacheHeavy" )))
    {
        struct expected_notification expect = { 2, { 0x1234567890, 0x6543210000 } };
        struct expected_notification expect2 = { 2 };

        reset_results( results );
        add_work_item( list, CrossProcessFlushCacheHeavy, expect.args[0], expect.args[1], 0, 0, 0, 0 );
        process_work_items();
        expect_notifications( results, 1, &expect, FALSE );
        ok( !list->work_list.first, "list not empty\n" );

        request_cross_process_flush( &list->work_list );
        process_work_items();
        expect_notifications( results, 1, &expect2, FALSE );
        ok( !list->work_list.first, "list not empty\n" );

        WriteProcessMemory( GetCurrentProcess(), ptr, old_code, sizeof(old_code), NULL );
    }

    if ((ptr = hook_notification_function( module, "BTCpuNotifyMapViewOfSection", "NotifyMapViewOfSection" )))
    {
        struct expected_notification expect = { 6 };
        LARGE_INTEGER offset;

        addr = NULL;
        size = 0;
        offset.QuadPart = 0;
        status = NtMapViewOfSection( mapping, GetCurrentProcess(), &addr, 0, 0, &offset, &size,
                                     ViewShare, 0, PAGE_READONLY );
        ok( NT_SUCCESS(status), "NtMapViewOfSection failed %lx\n", status );
        expect_notifications( results, 0, NULL, TRUE );
        NtUnmapViewOfSection( GetCurrentProcess(), addr );

        /* only NtMapViewOfSection calls coming from the loader trigger a notification */
        NtCurrentTeb()->Tib.ArbitraryUserPointer = (WCHAR *)L"c:\\windows\\system32\\version.dll";
        addr = NULL;
        size = 0;
        results[1] = STATUS_SUCCESS;
        status = NtMapViewOfSection( mapping, GetCurrentProcess(), &addr, 0, 0, &offset, &size,
                                     ViewShare, 0, PAGE_READONLY );
        ok( NT_SUCCESS(status), "NtMapViewOfSection failed %lx\n", status );
        expect.args[0] = results[2];  /* FIXME: first parameter unknown */
        expect.args[1] = (ULONG_PTR)addr;
        expect.args[3] = size;
        expect.args[5] = PAGE_READONLY;
        expect_notifications( results, 1, &expect, TRUE );
        NtUnmapViewOfSection( GetCurrentProcess(), addr );

        results[1] = 0xdeadbeef;
        status = NtMapViewOfSection( mapping, GetCurrentProcess(), &addr, 0, 0, &offset, &size,
                                     ViewShare, 0, PAGE_READONLY );
#ifdef _WIN64
        if (NtCurrentTeb()->ChpeV2CpuAreaInfo->InSyscallCallback)
        {
            ok( status == STATUS_SUCCESS, "NtMapViewOfSection failed %lx\n", status );
            expect_notifications( results, 0, NULL, TRUE );
            NtUnmapViewOfSection( GetCurrentProcess(), addr );
        }
        else
#endif
        {
            ok( status == 0xdeadbeef, "NtMapViewOfSection failed %lx\n", status );
            expect.args[0] = results[2];  /* FIXME: first parameter unknown */
            expect.args[1] = (ULONG_PTR)addr;
            expect.args[3] = size;
            expect.args[5] = PAGE_READONLY;
            expect_notifications( results, 1, &expect, TRUE );
        }
        NtCurrentTeb()->Tib.ArbitraryUserPointer = NULL;
        WriteProcessMemory( GetCurrentProcess(), ptr, old_code, sizeof(old_code), NULL );
    }

    if ((ptr = hook_notification_function( module, "BTCpuNotifyUnmapViewOfSection", "NotifyUnmapViewOfSection" )))
    {
        struct expected_notification expect[2] = { { 3 }, { 3 } };
        LARGE_INTEGER offset;

        addr = NULL;
        size = 0;
        offset.QuadPart = 0;
        status = NtMapViewOfSection( mapping, GetCurrentProcess(), &addr, 0, 0, &offset, &size,
                                     ViewShare, 0, PAGE_READONLY );
        ok( NT_SUCCESS(status), "NtMapViewOfSection failed %lx\n", status );
        NtUnmapViewOfSection( GetCurrentProcess(), (char *)addr + 0x123 );
        expect[0].args[0] = expect[1].args[0] = (ULONG_PTR)addr + 0x123;
        expect[1].args[1] = 1;
        expect_notifications( results, 2, expect, TRUE );

        NtUnmapViewOfSection( GetCurrentProcess(), (char *)0x1234 );
        expect[0].args[0] = expect[1].args[0] = 0x1234;
        expect[1].args[1] = 1;
        expect[1].args[2] = (ULONG)STATUS_NOT_MAPPED_VIEW;
        expect_notifications( results, 2, expect, TRUE );

        WriteProcessMemory( GetCurrentProcess(), ptr, old_code, sizeof(old_code), NULL );
    }

    if ((ptr = hook_notification_function( module, "BTCpuNotifyReadFile", "BTCpu64NotifyReadFile" )))
    {
        char buffer[0x123];
        IO_STATUS_BLOCK io;
        struct expected_notification expect[2] =
        {
            { 5, { (ULONG_PTR)file, (ULONG_PTR)buffer, sizeof(buffer), 0, 0 } },
            { 5, { (ULONG_PTR)file, (ULONG_PTR)buffer, sizeof(buffer), 1, 0 } }
        };

        reset_results( results );
        status = NtReadFile( file, 0, NULL, NULL, &io, buffer, sizeof(buffer), NULL, NULL );
        ok( !status, "NtReadFile failed %lx\n", status );
        expect_notifications( results, 2, expect, TRUE );

        status = NtReadFile( (HANDLE)0xdead, 0, NULL, NULL, &io, buffer, sizeof(buffer), NULL, NULL );
        ok( status == STATUS_INVALID_HANDLE, "NtReadFile failed %lx\n", status );
        expect[0].args[0] = expect[1].args[0] = 0xdead;
        expect[1].args[4] = (ULONG)STATUS_INVALID_HANDLE;
        expect_notifications( results, 2, expect, TRUE );

        WriteProcessMemory( GetCurrentProcess(), ptr, old_code, sizeof(old_code), NULL );
    }

    if ((ptr = hook_notification_function( module, "BTCpuThreadTerm", "ThreadTerm" )))
    {
        struct expected_notification expect = { 2, { 0xdead, 0xbeef } };

        reset_results( results );
        status = NtTerminateThread( (HANDLE)0xdead, 0xbeef );
        ok( status == STATUS_INVALID_HANDLE, "NtTerminateThread failed %lx\n", status );
        expect_notifications( results, 1, &expect, TRUE );

        WriteProcessMemory( GetCurrentProcess(), ptr, old_code, sizeof(old_code), NULL );
    }

    if ((ptr = hook_notification_function( module, "BTCpuProcessTerm", "ProcessTerm" )))
    {
        struct expected_notification expect[2] =
        {
            { 3, { 0, 0, 0 } },
            { 3, { 0, 1, 0 } }
        };

        reset_results( results );
        status = NtTerminateProcess( (HANDLE)0xdead, 0xbeef );
        ok( status == STATUS_INVALID_HANDLE, "NtTerminateProcess failed %lx\n", status );
        expect_notifications( results, 0, NULL, TRUE );

        status = NtTerminateProcess( 0, 0xbeef );
        ok( !status, "NtTerminateProcess failed %lx\n", status );
        expect_notifications( results, 2, expect, TRUE );

        WriteProcessMemory( GetCurrentProcess(), ptr, old_code, sizeof(old_code), NULL );
    }

    NtClose( mapping );
    NtClose( file );
    VirtualFree( code, 0, MEM_RELEASE );
}


#ifdef _WIN64

static void test_cross_process_work_list(void)
{
    UINT i, next, count = 10, size = offsetof( CROSS_PROCESS_WORK_LIST, entries[count] );
    BOOLEAN res, flush;
    CROSS_PROCESS_WORK_ENTRY *ptr, *ret;
    CROSS_PROCESS_WORK_LIST *list = calloc( size, 1 );

    if (!pRtlWow64PopAllCrossProcessWorkFromWorkList)
    {
        win_skip( "cross process list not supported\n" );
        return;
    }

    list = calloc( size, 1 );
    for (i = 0; i < count; i++)
    {
        res = pRtlWow64PushCrossProcessWorkOntoFreeList( &list->free_list, &list->entries[i] );
        ok( res == TRUE, "%u: RtlWow64PushCrossProcessWorkOntoFreeList failed\n", i );
    }

    ok( list->free_list.counter == count, "wrong counter %u\n", list->free_list.counter );
    ok( CROSS_PROCESS_LIST_ENTRY( &list->free_list, list->free_list.first ) == &list->entries[count - 1],
        "wrong offset %u\n", list->free_list.first );
    for (i = count; i > 1; i--)
        ok( CROSS_PROCESS_LIST_ENTRY( &list->free_list, list->entries[i - 1].next ) == &list->entries[i - 2],
            "%u: wrong offset %x / %x\n", i, list->entries[i - 1].next,
            (UINT)((char *)&list->entries[i - 2] - (char *)&list->free_list) );
    ok( !list->entries[0].next, "wrong last offset %x\n", list->entries[0].next );

    next = list->entries[count - 1].next;
    ptr = pRtlWow64PopCrossProcessWorkFromFreeList( &list->free_list );
    ok( ptr == (void *)&list->entries[count - 1], "wrong ptr %p (%p)\n", ptr, list );
    ok( !ptr->next, "next not reset %x\n", ptr->next );
    ok( list->free_list.first == next, "wrong offset %x / %x\n", list->free_list.first, next );
    ok( list->free_list.counter == count + 1, "wrong counter %u\n", list->free_list.counter );

    ptr->next = 0xdead;
    ptr->id = 3;
    ptr->addr = 0xdeadbeef;
    ptr->size = 0x1000;
    ptr->args[0] = 7;
    ret = (void *)0xdeadbeef;
    res = pRtlWow64PushCrossProcessWorkOntoWorkList( &list->work_list, ptr, (void **)&ret );
    ok( res == TRUE, "RtlWow64PushCrossProcessWorkOntoWorkList failed\n" );
    ok( !ret, "got ret ptr %p\n", ret );
    ok( list->work_list.counter == 1, "wrong counter %u\n", list->work_list.counter );
    ok( ptr == CROSS_PROCESS_LIST_ENTRY( &list->work_list, list->work_list.first), "wrong ptr %p / %p\n",
        ptr, CROSS_PROCESS_LIST_ENTRY( &list->work_list, list->work_list.first ));
    ok( !ptr->next, "got next %x\n", ptr->next );

    next = list->work_list.first;
    ptr = pRtlWow64PopCrossProcessWorkFromFreeList( &list->free_list );
    ok( list->free_list.counter == count + 2, "wrong counter %u\n", list->free_list.counter );
    ptr->id = 20;
    ptr->addr = 0x123456;
    ptr->size = 0x2345;
    res = pRtlWow64PushCrossProcessWorkOntoWorkList( &list->work_list, ptr, (void **)&ret );
    ok( res == TRUE, "RtlWow64PushCrossProcessWorkOntoWorkList failed\n" );
    ok( !ret, "got ret ptr %p\n", ret );
    ok( list->work_list.counter == 2, "wrong counter %u\n", list->work_list.counter );
    ok( list->work_list.first == (char *)ptr - (char *)&list->work_list, "wrong ptr %p / %p\n",
        ptr, (char *)list + list->work_list.first );
    ok( ptr->next == next, "got wrong next %x / %x\n", ptr->next, next );

    flush = 0xcc;
    ptr = pRtlWow64PopAllCrossProcessWorkFromWorkList( &list->work_list, &flush );
    ok( !flush, "RtlWow64PopAllCrossProcessWorkFromWorkList flush is TRUE\n" );
    ok( list->work_list.counter == 3, "wrong counter %u\n", list->work_list.counter );
    ok( !list->work_list.first, "list not empty %x\n", list->work_list.first );
    ok( ptr->addr == 0xdeadbeef, "wrong addr %s\n", wine_dbgstr_longlong(ptr->addr) );
    ok( ptr->size == 0x1000, "wrong size %s\n", wine_dbgstr_longlong(ptr->size) );
    ok( ptr->next, "next not set\n" );

    ptr = CROSS_PROCESS_LIST_ENTRY( &list->work_list, ptr->next );
    ok( ptr->addr == 0x123456, "wrong addr %s\n", wine_dbgstr_longlong(ptr->addr) );
    ok( ptr->size == 0x2345, "wrong size %s\n", wine_dbgstr_longlong(ptr->size) );
    ok( !ptr->next, "list not terminated\n" );

    res = pRtlWow64PushCrossProcessWorkOntoWorkList( &list->work_list, ptr, (void **)&ret );
    ok( res == TRUE, "RtlWow64PushCrossProcessWorkOntoWorkList failed\n" );
    ok( !ret, "got ret ptr %p\n", ret );
    ok( list->work_list.counter == 4, "wrong counter %u\n", list->work_list.counter );

    res = pRtlWow64RequestCrossProcessHeavyFlush( &list->work_list );
    ok( res == TRUE, "RtlWow64RequestCrossProcessHeavyFlush failed\n" );
    ok( list->work_list.counter == 5, "wrong counter %u\n", list->work_list.counter );
    ok( list->work_list.first & CROSS_PROCESS_LIST_FLUSH, "flush flag not set %x\n", list->work_list.first );
    ok( ptr == CROSS_PROCESS_LIST_ENTRY( &list->work_list, list->work_list.first), "wrong ptr %p / %p\n",
        ptr, CROSS_PROCESS_LIST_ENTRY( &list->work_list, list->work_list.first ));

    flush = 0xcc;
    ptr = pRtlWow64PopAllCrossProcessWorkFromWorkList( &list->work_list, &flush );
    ok( flush == TRUE, "RtlWow64PopAllCrossProcessWorkFromWorkList flush not set\n" );
    ok( list->work_list.counter == 6, "wrong counter %u\n", list->work_list.counter );
    ok( !list->work_list.first, "list not empty %x\n", list->work_list.first );
    ok( ptr->addr == 0x123456, "wrong addr %s\n", wine_dbgstr_longlong(ptr->addr) );
    ok( ptr->size == 0x2345, "wrong size %s\n", wine_dbgstr_longlong(ptr->size) );
    ok( !ptr->next, "next not set\n" );

    flush = 0xcc;
    ptr = pRtlWow64PopAllCrossProcessWorkFromWorkList( &list->work_list, &flush );
    ok( flush == FALSE, "RtlWow64PopAllCrossProcessWorkFromWorkList flush set\n" );
    ok( list->work_list.counter == 6, "wrong counter %u\n", list->work_list.counter );
    ok( !list->work_list.first, "list not empty %x\n", list->work_list.first );
    ok( !ptr, "got ptr %p\n", ptr );

    res = pRtlWow64RequestCrossProcessHeavyFlush( &list->work_list );
    ok( res == TRUE, "RtlWow64RequestCrossProcessHeavyFlush failed\n" );
    ok( list->work_list.counter == 7, "wrong counter %u\n", list->work_list.counter );
    ok( list->work_list.first & CROSS_PROCESS_LIST_FLUSH, "flush flag not set %x\n", list->work_list.first );

    res = pRtlWow64RequestCrossProcessHeavyFlush( &list->work_list );
    ok( res == TRUE, "RtlWow64RequestCrossProcessHeavyFlush failed\n" );
    ok( list->work_list.counter == 8, "wrong counter %u\n", list->work_list.counter );
    ok( list->work_list.first & CROSS_PROCESS_LIST_FLUSH, "flush flag not set %x\n", list->work_list.first );

    flush = 0xcc;
    ptr = pRtlWow64PopAllCrossProcessWorkFromWorkList( &list->work_list, &flush );
    ok( flush == TRUE, "RtlWow64PopAllCrossProcessWorkFromWorkList flush set\n" );
    ok( list->work_list.counter == 9, "wrong counter %u\n", list->work_list.counter );
    ok( !list->work_list.first, "list not empty %x\n", list->work_list.first );
    ok( !ptr, "got ptr %p\n", ptr );

    for (i = 0; i < count; i++)
    {
        ptr = pRtlWow64PopCrossProcessWorkFromFreeList( &list->free_list );
        if (!ptr) break;
        ok( list->free_list.counter == count + 3 + i, "wrong counter %u\n", list->free_list.counter );
    }
    ok( list->free_list.counter == count + 2 + i, "wrong counter %u\n", list->free_list.counter );
    ok( !list->free_list.first, "first still set %x\n", list->free_list.first );

    free( list );
}


static void test_cpu_area(void)
{
    if (pRtlWow64GetCpuAreaInfo)
    {
        static const struct
        {
            USHORT machine;
            NTSTATUS expect;
            ULONG_PTR align, size, offset, flag;
        } tests[] =
        {
            { IMAGE_FILE_MACHINE_I386,  0,  4, 0x2cc, 0x00, 0x00010000 },
            { IMAGE_FILE_MACHINE_AMD64, 0, 16, 0x4d0, 0x30, 0x00100000 },
            { IMAGE_FILE_MACHINE_ARMNT, 0,  8, 0x1a0, 0x00, 0x00200000 },
            { IMAGE_FILE_MACHINE_ARM64, 0, 16, 0x390, 0x00, 0x00400000 },
            { IMAGE_FILE_MACHINE_ARM,   STATUS_INVALID_PARAMETER },
            { IMAGE_FILE_MACHINE_THUMB, STATUS_INVALID_PARAMETER },
        };
        USHORT buffer[2048];
        WOW64_CPURESERVED *cpu;
        WOW64_CPU_AREA_INFO info;
        ULONG i, j;
        NTSTATUS status;
#define ALIGN(ptr,align) ((void *)(((ULONG_PTR)(ptr) + (align) - 1) & ~((align) - 1)))

        for (i = 0; i < ARRAY_SIZE(tests); i++)
        {
            for (j = 0; j < 8; j++)
            {
                cpu = (WOW64_CPURESERVED *)(buffer + j);
                cpu->Flags = 0;
                cpu->Machine = tests[i].machine;
                status = pRtlWow64GetCpuAreaInfo( cpu, 0, &info );
                ok( status == tests[i].expect, "%lu:%lu: failed %lx\n", i, j, status );
                if (status) continue;
                ok( info.Context == ALIGN( cpu + 1, tests[i].align ) ||
                    broken( (ULONG_PTR)info.Context == (ULONG)(ULONG_PTR)ALIGN( cpu + 1, tests[i].align ) ), /* win10 <= 1709 */
                    "%lu:%lu: wrong offset %Iu cpu %p context %p\n",
                    i, j, (ULONG_PTR)((char *)info.Context - (char *)cpu), cpu, info.Context );
                ok( info.ContextEx == ALIGN( (char *)info.Context + tests[i].size, sizeof(void*) ),
                    "%lu:%lu: wrong ex offset %lu\n", i, j, (ULONG)((char *)info.ContextEx - (char *)cpu) );
                ok( info.ContextFlagsLocation == (char *)info.Context + tests[i].offset,
                    "%lu:%lu: wrong flags offset %lu\n",
                    i, j, (ULONG)((char *)info.ContextFlagsLocation - (char *)info.Context) );
                ok( info.CpuReserved == cpu, "%lu:%lu: wrong cpu %p / %p\n", i, j, info.CpuReserved, cpu );
                ok( info.ContextFlag == tests[i].flag, "%lu:%lu: wrong flag %08lx\n", i, j, info.ContextFlag );
                ok( info.Machine == tests[i].machine, "%lu:%lu: wrong machine %x\n", i, j, info.Machine );
            }
        }
#undef ALIGN
    }
    else win_skip( "RtlWow64GetCpuAreaInfo not supported\n" );
}

static void test_exception_dispatcher(void)
{
#ifdef __x86_64__
    BYTE *code = (BYTE *)pKiUserExceptionDispatcher;
    void **hook;

    /* cld; mov xxx(%rip),%rax */
    ok( code[0] == 0xfc && code[1] == 0x48 && code[2] == 0x8b && code[3] == 0x05,
        "wrong opcodes %02x %02x %02x %02x\n", code[0], code[1], code[2], code[3] );
    hook = (void **)(code + 8 + *(int *)(code + 4));
    ok( !*hook, "hook %p set to %p\n", hook, *hook );
#endif
}

#ifdef __arm64ec__
static DWORD CALLBACK simulation_thread( void *arg )
{
    BYTE code[] =
    {
        0x48, 0xc7, 0xc1, 0x34, 0x12, 0x00, 0x00, /* mov $0x1234,%rcx */
        0x48, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0,       /* movabs $RtlExitUserThread,%rax */
        0xff, 0xd0,                               /* call *%rax */
        0xc3,                                     /* ret */
    };
    DWORD old_prot;
    CONTEXT *context;
    void (WINAPI *pBeginSimulation)(void) = arg;
    void *addr = VirtualAlloc( NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE );

    *(void **)(code + 9) = GetProcAddress( GetModuleHandleA("ntdll.dll"), "RtlExitUserThread" );
    memcpy( addr, code, sizeof(code) );
    VirtualProtect( addr, 0x1000, PAGE_EXECUTE_READ, &old_prot );

    context = &NtCurrentTeb()->ChpeV2CpuAreaInfo->ContextAmd64->AMD64_Context;
    context->Rsp = (ULONG_PTR)&context - 0x800;
    context->Rip = (ULONG_PTR)addr;

    NtCurrentTeb()->ChpeV2CpuAreaInfo->InSimulation = 1;  /* otherwise it crashes on recent Windows */
    pBeginSimulation();
    return 0x5678;
}
#endif

static void test_xtajit64(void)
{
#ifdef __arm64ec__
    HMODULE module = GetModuleHandleA( "xtajit64.dll" );
    BOOLEAN (WINAPI *pBTCpu64IsProcessorFeaturePresent)( UINT feature );
    void (WINAPI *pUpdateProcessorInformation)( SYSTEM_CPU_INFORMATION *info );
    void (WINAPI *pBeginSimulation)(void);
    UINT i;

    if (!module)
    {
        win_skip( "xtaji64.dll not loaded\n" );
        return;
    }
#define GET_PROC(func) p##func = pRtlFindExportedRoutineByName( module, #func )
    GET_PROC( BTCpu64IsProcessorFeaturePresent );
    GET_PROC( BeginSimulation );
    GET_PROC( UpdateProcessorInformation );
#undef GET_PROC

    if (pBTCpu64IsProcessorFeaturePresent)
    {
        static const ULONGLONG expect_features =
            (1ull << PF_COMPARE_EXCHANGE_DOUBLE) |
            (1ull << PF_MMX_INSTRUCTIONS_AVAILABLE) |
            (1ull << PF_XMMI_INSTRUCTIONS_AVAILABLE) |
            (1ull << PF_RDTSC_INSTRUCTION_AVAILABLE) |
            (1ull << PF_XMMI64_INSTRUCTIONS_AVAILABLE) |
            (1ull << PF_NX_ENABLED) |
            (1ull << PF_SSE3_INSTRUCTIONS_AVAILABLE) |
            (1ull << PF_COMPARE_EXCHANGE128) |
            (1ull << PF_FASTFAIL_AVAILABLE) |
            (1ull << PF_RDTSCP_INSTRUCTION_AVAILABLE) |
            (1ull << PF_SSSE3_INSTRUCTIONS_AVAILABLE) |
            (1ull << PF_SSE4_1_INSTRUCTIONS_AVAILABLE) |
            (1ull << PF_SSE4_2_INSTRUCTIONS_AVAILABLE);

        for (i = 0; i < 64; i++)
        {
            BOOLEAN ret = pBTCpu64IsProcessorFeaturePresent( i );
            if (expect_features & (1ull << i)) ok( ret, "missing feature %u\n", i );
            else if (ret) trace( "extra feature %u supported\n", i );
        }
    }
    else win_skip( "BTCpu64IsProcessorFeaturePresent missing\n" );

    if (pUpdateProcessorInformation)
    {
        SYSTEM_CPU_INFORMATION info;

        memset( &info, 0xcc, sizeof(info) );
        info.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_ARM64;
        pUpdateProcessorInformation( &info );

        ok( info.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64,
            "wrong architecture %u\n", info.ProcessorArchitecture );
        ok( info.ProcessorLevel == 21, "wrong level %u\n", info.ProcessorLevel );
        ok( info.ProcessorRevision == 1, "wrong revision %u\n", info.ProcessorRevision );
        ok( info.MaximumProcessors == 0xcccc, "wrong max proc %u\n", info.MaximumProcessors );
        ok( info.ProcessorFeatureBits == 0xcccccccc, "wrong features %lx\n", info.ProcessorFeatureBits );
    }
    else win_skip( "UpdateProcessorInformation missing\n" );

    if (pBeginSimulation)
    {
        DWORD ret, exit_code;
        HANDLE thread = CreateThread( NULL, 0, simulation_thread, pBeginSimulation, 0, NULL );

        ok( thread != 0, "thread creation failed\n" );
        ret = WaitForSingleObject( thread, 10000 );
        ok( !ret, "wait failed %lx\n", ret );
        GetExitCodeThread( thread, &exit_code );
        ok( exit_code == 0x1234, "wrong exit code %lx\n", exit_code );
        CloseHandle( thread );
    }
    else win_skip( "BeginSimulation missing\n" );
#endif
}


static void test_memory_notifications(void)
{
    HMODULE module;
    CHPEV2_PROCESS_INFO *info;

    if (current_machine == IMAGE_FILE_MACHINE_ARM64) return;
    if (!(module = GetModuleHandleA( "xtajit64.dll" ))) return;
    info = NtCurrentTeb()->Peb->ChpeV2ProcessInfo;
    if (info->NativeMachineType == native_machine &&
        info->EmulatedMachineType == IMAGE_FILE_MACHINE_AMD64)
    {
        test_notifications( module, (CROSS_PROCESS_WORK_LIST *)info->CrossProcessWorkList );

        NtCurrentTeb()->ChpeV2CpuAreaInfo->InSyscallCallback++;
        test_notifications( module, (CROSS_PROCESS_WORK_LIST *)info->CrossProcessWorkList );
        NtCurrentTeb()->ChpeV2CpuAreaInfo->InSyscallCallback--;
    }
    skip( "arm64ec shared info not found\n" );
}


#else  /* _WIN64 */

static const BYTE call_func64_code[] =
{
    0x58,                               /* pop %eax */
    0x0e,                               /* push %cs */
    0x50,                               /* push %eax */
    0x6a, 0x33,                         /* push $0x33 */
    0xe8, 0x00, 0x00, 0x00, 0x00,       /* call 1f */
    0x83, 0x04, 0x24, 0x05,             /* 1: addl $0x5,(%esp) */
    0xcb,                               /* lret */
    /* in 64-bit mode: */
    0x4c, 0x87, 0xf4,                   /* xchg %r14,%rsp */
    0x55,                               /* push %rbp */
    0x48, 0x89, 0xe5,                   /* mov %rsp,%rbp */
    0x56,                               /* push %rsi */
    0x57,                               /* push %rdi */
    0x41, 0x8b, 0x4e, 0x10,             /* mov 0x10(%r14),%ecx */
    0x41, 0x8b, 0x76, 0x14,             /* mov 0x14(%r14),%esi */
    0x67, 0x8d, 0x04, 0xcd, 0, 0, 0, 0, /* lea 0x0(,%ecx,8),%eax */
    0x83, 0xf8, 0x20,                   /* cmp $0x20,%eax */
    0x7d, 0x05,                         /* jge 1f */
    0xb8, 0x20, 0x00, 0x00, 0x00,       /* mov $0x20,%eax */
    0x48, 0x29, 0xc4,                   /* 1: sub %rax,%rsp */
    0x48, 0x83, 0xe4, 0xf0,             /* and $~15,%rsp */
    0x48, 0x89, 0xe7,                   /* mov %rsp,%rdi */
    0xf3, 0x48, 0xa5,                   /* rep movsq */
    0x48, 0x8b, 0x0c, 0x24,             /* mov (%rsp),%rcx */
    0x48, 0x8b, 0x54, 0x24, 0x08,       /* mov 0x8(%rsp),%rdx */
    0x4c, 0x8b, 0x44, 0x24, 0x10,       /* mov 0x10(%rsp),%r8 */
    0x4c, 0x8b, 0x4c, 0x24, 0x18,       /* mov 0x18(%rsp),%r9 */
    0x41, 0xff, 0x56, 0x08,             /* callq *0x8(%r14) */
    0x48, 0x8d, 0x65, 0xf0,             /* lea -0x10(%rbp),%rsp */
    0x5f,                               /* pop %rdi */
    0x5e,                               /* pop %rsi */
    0x5d,                               /* pop %rbp */
    0x4c, 0x87, 0xf4,                   /* xchg %r14,%rsp */
    0xcb,                               /* lret */
};

static NTSTATUS call_func64( ULONG64 func64, int nb_args, ULONG64 *args )
{
    NTSTATUS (WINAPI *func)( ULONG64 func64, int nb_args, ULONG64 *args ) = code_mem;

    memcpy( code_mem, call_func64_code, sizeof(call_func64_code) );
    return func( func64, nb_args, args );
}

static ULONG64 main_module, ntdll_module, wow64_module, wow64base_module, wow64con_module,
               wow64cpu_module, xtajit_module, wow64win_module;

static void enum_modules64( void (*func)(ULONG64,const WCHAR *) )
{
    typedef struct
    {
        LIST_ENTRY64     InLoadOrderLinks;
        LIST_ENTRY64     InMemoryOrderLinks;
        LIST_ENTRY64     InInitializationOrderLinks;
        ULONG64          DllBase;
        ULONG64          EntryPoint;
        ULONG            SizeOfImage;
        UNICODE_STRING64 FullDllName;
        UNICODE_STRING64 BaseDllName;
        /* etc. */
    } LDR_DATA_TABLE_ENTRY64;

    TEB64 *teb64 = (TEB64 *)NtCurrentTeb()->GdiBatchCount;
    PEB64 peb64;
    ULONG64 ptr;
    PEB_LDR_DATA64 ldr;
    LDR_DATA_TABLE_ENTRY64 entry;
    NTSTATUS status;
    HANDLE process;

    process = OpenProcess( PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId() );
    ok( process != 0, "failed to open current process %lu\n", GetLastError() );
    status = pNtWow64ReadVirtualMemory64( process, teb64->Peb, &peb64, sizeof(peb64), NULL );
    ok( !status, "NtWow64ReadVirtualMemory64 failed %lx\n", status );
    todo_wine_if( old_wow64 )
    ok( peb64.LdrData, "LdrData not initialized\n" );
    if (!peb64.LdrData) goto done;
    status = pNtWow64ReadVirtualMemory64( process, peb64.LdrData, &ldr, sizeof(ldr), NULL );
    ok( !status, "NtWow64ReadVirtualMemory64 failed %lx\n", status );
    ptr = ldr.InLoadOrderModuleList.Flink;
    for (;;)
    {
        WCHAR buffer[256];
        status = pNtWow64ReadVirtualMemory64( process, ptr, &entry, sizeof(entry), NULL );
        ok( !status, "NtWow64ReadVirtualMemory64 failed %lx\n", status );
        status = pNtWow64ReadVirtualMemory64( process, entry.BaseDllName.Buffer, buffer, sizeof(buffer), NULL );
        ok( !status, "NtWow64ReadVirtualMemory64 failed %lx\n", status );
        if (status) break;
        func( entry.DllBase, buffer );
        ptr = entry.InLoadOrderLinks.Flink;
        if (ptr == peb64.LdrData + offsetof( PEB_LDR_DATA64, InLoadOrderModuleList )) break;
    }
done:
    NtClose( process );
}

static ULONG64 get_proc_address64( ULONG64 module, const char *name )
{
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS64 nt;
    IMAGE_EXPORT_DIRECTORY exports;
    ULONG i, *names, *funcs;
    USHORT *ordinals;
    NTSTATUS status;
    HANDLE process;
    ULONG64 ret = 0;
    char buffer[64];

    if (!module) return 0;
    process = OpenProcess( PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId() );
    ok( process != 0, "failed to open current process %lu\n", GetLastError() );
    status = pNtWow64ReadVirtualMemory64( process, module, &dos, sizeof(dos), NULL );
    ok( !status, "NtWow64ReadVirtualMemory64 failed %lx\n", status );
    status = pNtWow64ReadVirtualMemory64( process, module + dos.e_lfanew, &nt, sizeof(nt), NULL );
    ok( !status, "NtWow64ReadVirtualMemory64 failed %lx\n", status );
    status = pNtWow64ReadVirtualMemory64( process, module + nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,
                                          &exports, sizeof(exports), NULL );
    ok( !status, "NtWow64ReadVirtualMemory64 failed %lx\n", status );
    names = calloc( exports.NumberOfNames, sizeof(*names) );
    ordinals = calloc( exports.NumberOfNames, sizeof(*ordinals) );
    funcs = calloc( exports.NumberOfFunctions, sizeof(*funcs) );
    status = pNtWow64ReadVirtualMemory64( process, module + exports.AddressOfNames,
                                          names, exports.NumberOfNames * sizeof(*names), NULL );
    ok( !status, "NtWow64ReadVirtualMemory64 failed %lx\n", status );
    status = pNtWow64ReadVirtualMemory64( process, module + exports.AddressOfNameOrdinals,
                                          ordinals, exports.NumberOfNames * sizeof(*ordinals), NULL );
    ok( !status, "NtWow64ReadVirtualMemory64 failed %lx\n", status );
    status = pNtWow64ReadVirtualMemory64( process, module + exports.AddressOfFunctions,
                                          funcs, exports.NumberOfFunctions * sizeof(*funcs), NULL );
    ok( !status, "NtWow64ReadVirtualMemory64 failed %lx\n", status );
    for (i = 0; i < exports.NumberOfNames && !ret; i++)
    {
        status = pNtWow64ReadVirtualMemory64( process, module + names[i], buffer, sizeof(buffer), NULL );
        ok( !status, "NtWow64ReadVirtualMemory64 failed %lx\n", status );
        if (!strcmp( buffer, name )) ret = module + funcs[ordinals[i]];
    }
    free( funcs );
    free( ordinals );
    free( names );
    NtClose( process );
    return ret;
}

static void check_module( ULONG64 base, const WCHAR *name )
{
    if (base == (ULONG_PTR)GetModuleHandleW(0))
    {
        WCHAR *p, module[MAX_PATH];

        GetModuleFileNameW( 0, module, MAX_PATH );
        if ((p = wcsrchr( module, '\\' ))) p++;
        else p = module;
        ok( !wcsicmp( name, p ), "wrong name %s / %s\n", debugstr_w(name), debugstr_w(module));
        main_module = base;
        return;
    }
#define CHECK_MODULE(mod) do { if (!wcsicmp( name, L"" #mod ".dll" )) { mod ## _module = base; return; } } while(0)
    CHECK_MODULE(ntdll);
    CHECK_MODULE(wow64);
    CHECK_MODULE(wow64base);
    CHECK_MODULE(wow64con);
    CHECK_MODULE(wow64win);
    if (native_machine == IMAGE_FILE_MACHINE_ARM64)
        CHECK_MODULE(xtajit);
    else
        CHECK_MODULE(wow64cpu);
#undef CHECK_MODULE
    todo_wine_if( !wcscmp( name, L"win32u.dll" ))
    ok( 0, "unknown module %s %s found\n", wine_dbgstr_longlong(base), wine_dbgstr_w(name));
}

static void test_modules(void)
{
    if (!is_wow64) return;
    if (!pNtWow64ReadVirtualMemory64) return;
    enum_modules64( check_module );
    todo_wine_if( old_wow64 )
    {
    ok( main_module, "main module not found\n" );
    ok( ntdll_module, "64-bit ntdll not found\n" );
    ok( wow64_module, "wow64.dll not found\n" );
    if (native_machine == IMAGE_FILE_MACHINE_ARM64)
        ok( xtajit_module, "xtajit.dll not found\n" );
    else
        ok( wow64cpu_module, "wow64cpu.dll not found\n" );
    ok( wow64win_module, "wow64win.dll not found\n" );
    }
}

static void test_nt_wow64(void)
{
    const char str[] = "hello wow64";
    char buffer[100];
    NTSTATUS status;
    ULONG64 res;
    HANDLE process = OpenProcess( PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId() );

    ok( process != 0, "failed to open current process %lu\n", GetLastError() );
    if (pNtWow64ReadVirtualMemory64)
    {
        status = pNtWow64ReadVirtualMemory64( process, (ULONG_PTR)str, buffer, sizeof(str), &res );
        ok( !status, "NtWow64ReadVirtualMemory64 failed %lx\n", status );
        ok( res == sizeof(str), "wrong size %s\n", wine_dbgstr_longlong(res) );
        ok( !strcmp( buffer, str ), "wrong data %s\n", debugstr_a(buffer) );
        status = pNtWow64WriteVirtualMemory64( process, (ULONG_PTR)buffer, " bye ", 5, &res );
        ok( !status, "NtWow64WriteVirtualMemory64 failed %lx\n", status );
        ok( res == 5, "wrong size %s\n", wine_dbgstr_longlong(res) );
        ok( !strcmp( buffer, " bye  wow64" ), "wrong data %s\n", debugstr_a(buffer) );
        /* current process pseudo-handle is broken on some Windows versions */
        status = pNtWow64ReadVirtualMemory64( GetCurrentProcess(), (ULONG_PTR)str, buffer, sizeof(str), &res );
        ok( !status || broken( status == STATUS_INVALID_HANDLE ),
            "NtWow64ReadVirtualMemory64 failed %lx\n", status );
        status = pNtWow64WriteVirtualMemory64( GetCurrentProcess(), (ULONG_PTR)buffer, " bye ", 5, &res );
        ok( !status || broken( status == STATUS_INVALID_HANDLE ),
            "NtWow64WriteVirtualMemory64 failed %lx\n", status );
    }
    else win_skip( "NtWow64ReadVirtualMemory64 not supported\n" );

    if (pNtWow64AllocateVirtualMemory64)
    {
        ULONG64 ptr = 0;
        ULONG64 size = 0x2345;

        status = pNtWow64AllocateVirtualMemory64( process, &ptr, 0, &size,
                                                  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
        ok( !status, "NtWow64AllocateVirtualMemory64 failed %lx\n", status );
        ok( ptr, "ptr not set\n" );
        ok( size == 0x3000, "size not set %s\n", wine_dbgstr_longlong(size) );
        ptr += 0x1000;
        status = pNtWow64AllocateVirtualMemory64( process, &ptr, 0, &size,
                                                  MEM_RESERVE | MEM_COMMIT, PAGE_READONLY );
        ok( status == STATUS_CONFLICTING_ADDRESSES, "NtWow64AllocateVirtualMemory64 failed %lx\n", status );
        ptr = 0;
        size = 0;
        status = pNtWow64AllocateVirtualMemory64( process, &ptr, 0, &size,
                                                  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
        ok( status == STATUS_INVALID_PARAMETER || status == STATUS_INVALID_PARAMETER_4,
            "NtWow64AllocateVirtualMemory64 failed %lx\n", status );
        size = 0x1000;
        status = pNtWow64AllocateVirtualMemory64( process, &ptr, 22, &size,
                                                  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
        ok( status == STATUS_INVALID_PARAMETER || status == STATUS_INVALID_PARAMETER_3,
            "NtWow64AllocateVirtualMemory64 failed %lx\n", status );
        status = pNtWow64AllocateVirtualMemory64( process, &ptr, 33, &size,
                                                  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
        ok( status == STATUS_INVALID_PARAMETER || status == STATUS_INVALID_PARAMETER_3,
            "NtWow64AllocateVirtualMemory64 failed %lx\n", status );
        status = pNtWow64AllocateVirtualMemory64( process, &ptr, 0x3fffffff, &size,
                                                  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
        todo_wine_if( !is_wow64 )
        ok( !status, "NtWow64AllocateVirtualMemory64 failed %lx\n", status );
        ok( ptr < 0x40000000, "got wrong ptr %s\n", wine_dbgstr_longlong(ptr) );
        if (!status && pNtWow64WriteVirtualMemory64)
        {
            status = pNtWow64WriteVirtualMemory64( process, ptr, str, sizeof(str), &res );
            ok( !status, "NtWow64WriteVirtualMemory64 failed %lx\n", status );
            ok( res == sizeof(str), "wrong size %s\n", wine_dbgstr_longlong(res) );
            ok( !strcmp( (char *)(ULONG_PTR)ptr, str ), "wrong data %s\n",
                debugstr_a((char *)(ULONG_PTR)ptr) );
            ptr = 0;
            status = pNtWow64AllocateVirtualMemory64( process, &ptr, 0, &size,
                                                      MEM_RESERVE | MEM_COMMIT, PAGE_READONLY );
            ok( !status, "NtWow64AllocateVirtualMemory64 failed %lx\n", status );
            status = pNtWow64WriteVirtualMemory64( process, ptr, str, sizeof(str), &res );
            todo_wine
            ok( status == STATUS_PARTIAL_COPY || broken( status == STATUS_ACCESS_VIOLATION ),
                "NtWow64WriteVirtualMemory64 failed %lx\n", status );
            todo_wine
            ok( !res || broken(res) /* win10 1709 */, "wrong size %s\n", wine_dbgstr_longlong(res) );
        }
        ptr = 0x9876543210ull;
        status = pNtWow64AllocateVirtualMemory64( process, &ptr, 0, &size,
                                                  MEM_RESERVE | MEM_COMMIT, PAGE_READONLY );
        todo_wine_if( !is_wow64 || old_wow64 )
        ok( !status || broken( status == STATUS_CONFLICTING_ADDRESSES ),
            "NtWow64AllocateVirtualMemory64 failed %lx\n", status );
        if (!status) ok( ptr == 0x9876540000ull || broken(ptr == 0x76540000), /* win 8.1 */
                         "wrong ptr %s\n", wine_dbgstr_longlong(ptr) );
        ptr = 0;
        status = pNtWow64AllocateVirtualMemory64( GetCurrentProcess(), &ptr, 0, &size,
                                                  MEM_RESERVE | MEM_COMMIT, PAGE_READONLY );
        ok( !status || broken( status == STATUS_INVALID_HANDLE ),
            "NtWow64AllocateVirtualMemory64 failed %lx\n", status );
    }
    else win_skip( "NtWow64AllocateVirtualMemory64 not supported\n" );

    if (pNtWow64GetNativeSystemInformation)
    {
        ULONG i, len;
        SYSTEM_BASIC_INFORMATION sbi, sbi2, sbi3;

        memset( &sbi, 0xcc, sizeof(sbi) );
        status = pNtQuerySystemInformation( SystemBasicInformation, &sbi, sizeof(sbi), &len );
        ok( status == STATUS_SUCCESS, "failed %lx\n", status );
        ok( len == sizeof(sbi), "wrong length %ld\n", len );

        memset( &sbi2, 0xcc, sizeof(sbi2) );
        status = pRtlGetNativeSystemInformation( SystemBasicInformation, &sbi2, sizeof(sbi2), &len );
        ok( status == STATUS_SUCCESS, "failed %lx\n", status );
        ok( len == sizeof(sbi2), "wrong length %ld\n", len );

        ok( sbi.HighestUserAddress == (void *)0x7ffeffff, "wrong limit %p\n", sbi.HighestUserAddress);
        todo_wine_if( old_wow64 )
        ok( sbi2.HighestUserAddress == (is_wow64 ? (void *)0xfffeffff : (void *)0x7ffeffff),
            "wrong limit %p\n", sbi.HighestUserAddress);

        memset( &sbi3, 0xcc, sizeof(sbi3) );
        status = pNtWow64GetNativeSystemInformation( SystemBasicInformation, &sbi3, sizeof(sbi3), &len );
        ok( status == STATUS_SUCCESS, "failed %lx\n", status );
        ok( len == sizeof(sbi3), "wrong length %ld\n", len );
        ok( !memcmp( &sbi2, &sbi3, offsetof(SYSTEM_BASIC_INFORMATION,NumberOfProcessors)+1 ),
            "info is different\n" );

        memset( &sbi3, 0xcc, sizeof(sbi3) );
        status = pNtWow64GetNativeSystemInformation( SystemEmulationBasicInformation, &sbi3, sizeof(sbi3), &len );
        ok( status == STATUS_SUCCESS, "failed %lx\n", status );
        ok( len == sizeof(sbi3), "wrong length %ld\n", len );
        ok( !memcmp( &sbi, &sbi3, offsetof(SYSTEM_BASIC_INFORMATION,NumberOfProcessors)+1 ),
            "info is different\n" );

        for (i = 0; i < 256; i++)
        {
            NTSTATUS expect = pNtQuerySystemInformation( i, NULL, 0, &len );
            status = pNtWow64GetNativeSystemInformation( i, NULL, 0, &len );
            switch (i)
            {
            case SystemNativeBasicInformation:
                ok( status == STATUS_INVALID_INFO_CLASS || status == STATUS_INFO_LENGTH_MISMATCH ||
                    broken(status == STATUS_NOT_IMPLEMENTED) /* vista */, "%lu: %lx / %lx\n", i, status, expect );
                break;
            case SystemBasicInformation:
            case SystemCpuInformation:
            case SystemEmulationBasicInformation:
            case SystemEmulationProcessorInformation:
                ok( status == expect, "%lu: %lx / %lx\n", i, status, expect );
                break;
            default:
                if (is_wow64)  /* only a few info classes are supported on Wow64 */
                    ok( status == STATUS_INVALID_INFO_CLASS ||
                        broken(status == STATUS_NOT_IMPLEMENTED), /* vista */
                        "%lu: %lx\n", i, status );
                else
                    ok( status == expect, "%lu: %lx / %lx\n", i, status, expect );
                break;
            }
        }
    }
    else win_skip( "NtWow64GetNativeSystemInformation not supported\n" );

    if (pNtWow64IsProcessorFeaturePresent)
    {
        ULONG i;

        for (i = 0; i < 64; i++)
            ok( pNtWow64IsProcessorFeaturePresent( i ) == IsProcessorFeaturePresent( i ),
                "mismatch %lu wow64 returned %lx\n", i, pNtWow64IsProcessorFeaturePresent( i ));

        if (native_machine == IMAGE_FILE_MACHINE_ARM64)
        {
            KSHARED_USER_DATA *user_shared_data = ULongToPtr( 0x7ffe0000 );

            ok( user_shared_data->ProcessorFeatures[PF_ARM_V8_INSTRUCTIONS_AVAILABLE], "no ARM_V8\n" );
            ok( user_shared_data->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE], "no MMX\n" );
            ok( !pNtWow64IsProcessorFeaturePresent( PF_ARM_V8_INSTRUCTIONS_AVAILABLE ), "ARM_V8 present\n" );
            ok( pNtWow64IsProcessorFeaturePresent( PF_MMX_INSTRUCTIONS_AVAILABLE ), "MMX not present\n" );
        }
    }
    else win_skip( "NtWow64IsProcessorFeaturePresent not supported\n" );

    if (pNtWow64QueryInformationProcess64)
    {
        PROCESS_BASIC_INFORMATION pbi32;
        PROCESS_BASIC_INFORMATION64 pbi64;
        ULONG expected_peb;
        ULONG class;

        for (class = 0; class <= MaxProcessInfoClass; class++)
        {
            winetest_push_context( "Process information class %lu", class );

            switch (class)
            {
            case ProcessBasicInformation:
                status = NtQueryInformationProcess( GetCurrentProcess(), ProcessBasicInformation, &pbi32, sizeof(pbi32), NULL );
                ok( !status, "NtQueryInformationProcess returned 0x%08lx\n", status );

                status = pNtWow64QueryInformationProcess64( GetCurrentProcess(), ProcessBasicInformation, &pbi64, sizeof(pbi64), NULL );
                ok( !status, "NtWow64QueryInformationProcess64 returned 0x%08lx\n", status );

                expected_peb = (ULONG)pbi32.PebBaseAddress;
                if (is_wow64) expected_peb -= 0x1000;

                ok( pbi64.ExitStatus == pbi32.ExitStatus,
                    "expected %lu got %lu\n", pbi32.ExitStatus, pbi64.ExitStatus );
                ok( pbi64.PebBaseAddress == expected_peb ||
                    /* The 64-bit PEB is usually, but not always, 4096 bytes below the 32-bit PEB */
                    broken( is_wow64 && llabs( (INT64)pbi64.PebBaseAddress - (INT64)expected_peb ) < 0x10000 ),
                    "expected 0x%lx got 0x%I64x\n", expected_peb, pbi64.PebBaseAddress );
                ok( pbi64.AffinityMask == pbi32.AffinityMask,
                    "expected 0x%Ix got 0x%I64x\n", pbi32.AffinityMask, pbi64.AffinityMask );
                ok( pbi64.UniqueProcessId == pbi32.UniqueProcessId,
                    "expected %Ix got %I64x\n", pbi32.UniqueProcessId, pbi64.UniqueProcessId );
                ok( pbi64.InheritedFromUniqueProcessId == pbi32.InheritedFromUniqueProcessId,
                    "expected %Ix got %I64x\n", pbi32.UniqueProcessId, pbi64.UniqueProcessId );
                break;
            default:
                status = pNtWow64QueryInformationProcess64( GetCurrentProcess(), class, NULL, 0, NULL );
                ok( status == STATUS_NOT_IMPLEMENTED, "NtWow64QueryInformationProcess64 returned 0x%08lx\n", status );
            }

            winetest_pop_context();
        }
    }
    else win_skip( "NtWow64QueryInformationProcess64 not supported\n" );

    NtClose( process );
}

static void test_init_block(void)
{
    HMODULE ntdll = GetModuleHandleA( "ntdll.dll" );
    ULONG i, size = 0, *init_block;
    ULONG64 ptr64, *block64;
    void *ptr;

    if (!is_wow64) return;
    if ((ptr = GetProcAddress( ntdll, "LdrSystemDllInitBlock" )))
    {
        init_block = ptr;
        trace( "got init block %08lx\n", init_block[0] );
#define CHECK_FUNC(val,func) \
            ok( (val) == (ULONG_PTR)GetProcAddress( ntdll, func ), \
                "got %p for %s %p\n", (void *)(ULONG_PTR)(val), func, GetProcAddress( ntdll, func ))
        switch (init_block[0])
        {
        case 0x44:  /* vistau64 */
            CHECK_FUNC( init_block[1], "LdrInitializeThunk" );
            CHECK_FUNC( init_block[2], "KiUserExceptionDispatcher" );
            CHECK_FUNC( init_block[3], "KiUserApcDispatcher" );
            CHECK_FUNC( init_block[4], "KiUserCallbackDispatcher" );
            CHECK_FUNC( init_block[5], "LdrHotPatchRoutine" );
            CHECK_FUNC( init_block[6], "ExpInterlockedPopEntrySListFault" );
            CHECK_FUNC( init_block[7], "ExpInterlockedPopEntrySListResume" );
            CHECK_FUNC( init_block[8], "ExpInterlockedPopEntrySListEnd" );
            CHECK_FUNC( init_block[9], "RtlUserThreadStart" );
            CHECK_FUNC( init_block[10], "RtlpQueryProcessDebugInformationRemote" );
            CHECK_FUNC( init_block[11], "EtwpNotificationThread" );
            ok( init_block[12] == (ULONG_PTR)ntdll, "got %p for ntdll %p\n",
                (void *)(ULONG_PTR)init_block[12], ntdll );
            size = 13 * sizeof(*init_block);
            break;
        case 0x50:  /* win7 */
            CHECK_FUNC( init_block[4], "LdrInitializeThunk" );
            CHECK_FUNC( init_block[5], "KiUserExceptionDispatcher" );
            CHECK_FUNC( init_block[6], "KiUserApcDispatcher" );
            CHECK_FUNC( init_block[7], "KiUserCallbackDispatcher" );
            CHECK_FUNC( init_block[8], "LdrHotPatchRoutine" );
            CHECK_FUNC( init_block[9], "ExpInterlockedPopEntrySListFault" );
            CHECK_FUNC( init_block[10], "ExpInterlockedPopEntrySListResume" );
            CHECK_FUNC( init_block[11], "ExpInterlockedPopEntrySListEnd" );
            CHECK_FUNC( init_block[12], "RtlUserThreadStart" );
            CHECK_FUNC( init_block[13], "RtlpQueryProcessDebugInformationRemote" );
            CHECK_FUNC( init_block[14], "EtwpNotificationThread" );
            ok( init_block[15] == (ULONG_PTR)ntdll, "got %p for ntdll %p\n",
                (void *)(ULONG_PTR)init_block[15], ntdll );
            /* CHECK_FUNC( init_block[16], "LdrSystemDllInitBlock" ); not always present */
            size = 17 * sizeof(*init_block);
            break;
        case 0x70:  /* win8 */
            CHECK_FUNC( init_block[4], "LdrInitializeThunk" );
            CHECK_FUNC( init_block[5], "KiUserExceptionDispatcher" );
            CHECK_FUNC( init_block[6], "KiUserApcDispatcher" );
            CHECK_FUNC( init_block[7], "KiUserCallbackDispatcher" );
            CHECK_FUNC( init_block[8], "ExpInterlockedPopEntrySListFault" );
            CHECK_FUNC( init_block[9], "ExpInterlockedPopEntrySListResume" );
            CHECK_FUNC( init_block[10], "ExpInterlockedPopEntrySListEnd" );
            CHECK_FUNC( init_block[11], "RtlUserThreadStart" );
            CHECK_FUNC( init_block[12], "RtlpQueryProcessDebugInformationRemote" );
            ok( init_block[13] == (ULONG_PTR)ntdll, "got %p for ntdll %p\n",
                (void *)(ULONG_PTR)init_block[13], ntdll );
            CHECK_FUNC( init_block[14], "LdrSystemDllInitBlock" );
            size = 15 * sizeof(*init_block);
            break;
        case 0x80:  /* win10 1507 */
            CHECK_FUNC( init_block[4], "LdrInitializeThunk" );
            CHECK_FUNC( init_block[5], "KiUserExceptionDispatcher" );
            CHECK_FUNC( init_block[6], "KiUserApcDispatcher" );
            CHECK_FUNC( init_block[7], "KiUserCallbackDispatcher" );
            if (GetProcAddress( ntdll, "ExpInterlockedPopEntrySListFault" ))
            {
                CHECK_FUNC( init_block[8], "ExpInterlockedPopEntrySListFault" );
                CHECK_FUNC( init_block[9], "ExpInterlockedPopEntrySListResume" );
                CHECK_FUNC( init_block[10], "ExpInterlockedPopEntrySListEnd" );
                CHECK_FUNC( init_block[11], "RtlUserThreadStart" );
                CHECK_FUNC( init_block[12], "RtlpQueryProcessDebugInformationRemote" );
                ok( init_block[13] == (ULONG_PTR)ntdll, "got %p for ntdll %p\n",
                    (void *)(ULONG_PTR)init_block[13], ntdll );
                CHECK_FUNC( init_block[14], "LdrSystemDllInitBlock" );
                size = 15 * sizeof(*init_block);
            }
            else  /* win10 1607 */
            {
                CHECK_FUNC( init_block[8], "RtlUserThreadStart" );
                CHECK_FUNC( init_block[9], "RtlpQueryProcessDebugInformationRemote" );
                ok( init_block[10] == (ULONG_PTR)ntdll, "got %p for ntdll %p\n",
                    (void *)(ULONG_PTR)init_block[10], ntdll );
                CHECK_FUNC( init_block[11], "LdrSystemDllInitBlock" );
                size = 12 * sizeof(*init_block);
            }
            break;
        case 0xe0:  /* win10 1809 */
        case 0xf0:  /* win10 2004 */
        case 0x128: /* win11 24h2 */
            block64 = ptr;
            CHECK_FUNC( block64[3], "LdrInitializeThunk" );
            CHECK_FUNC( block64[4], "KiUserExceptionDispatcher" );
            CHECK_FUNC( block64[5], "KiUserApcDispatcher" );
            CHECK_FUNC( block64[6], "KiUserCallbackDispatcher" );
            CHECK_FUNC( block64[7], "RtlUserThreadStart" );
            CHECK_FUNC( block64[8], "RtlpQueryProcessDebugInformationRemote" );
            todo_wine_if( old_wow64 )
            ok( block64[9] == (ULONG_PTR)ntdll, "got %p for ntdll %p\n",
                (void *)(ULONG_PTR)block64[9], ntdll );
            CHECK_FUNC( block64[10], "LdrSystemDllInitBlock" );
            CHECK_FUNC( block64[11], "RtlpFreezeTimeBias" );
            size = 12 * sizeof(*block64);
            break;
        default:
            ok( 0, "unknown init block %08lx\n", init_block[0] );
            for (i = 0; i < init_block[0] / sizeof(ULONG); i++) trace("%04lx: %08lx\n", i, init_block[i]);
            break;
        }
#undef CHECK_FUNC

        if (size && (ptr64 = get_proc_address64( ntdll_module, "LdrSystemDllInitBlock" )))
        {
            DWORD buffer[64];
            HANDLE process = OpenProcess( PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId() );
            NTSTATUS status = pNtWow64ReadVirtualMemory64( process, ptr64, buffer, size, NULL );
            ok( !status, "NtWow64ReadVirtualMemory64 failed %lx\n", status );
            ok( !memcmp( buffer, init_block, size ), "wrong 64-bit init block\n" );
            NtClose( process );
        }
    }
    else todo_wine win_skip( "LdrSystemDllInitBlock not supported\n" );
}


static void test_memory_notifications(void)
{
    HMODULE module = (HMODULE)(ULONG_PTR)xtajit_module;
    WOW64INFO *info;
    DWORD i;

    if (!xtajit_module)
    {
        skip( "xtajit.dll not loaded\n" );
        return;
    }
    if ((ULONG_PTR)module != xtajit_module)
    {
        skip( "xtajit.dll loaded above 4G\n" );
        return;
    }

    for (i = 0x400; i < 0x800; i += sizeof(ULONG))
    {
        info = (WOW64INFO *)((char *)NtCurrentTeb()->Peb + i);
        if (info->NativeMachineType == native_machine &&
            info->EmulatedMachineType == IMAGE_FILE_MACHINE_I386)
        {
            if (info->CrossProcessWorkList >> 32)
                skip( "cross-process work list above 4G (%I64x)\n", info->CrossProcessWorkList );
            else
                test_notifications( module, ULongToPtr( info->CrossProcessWorkList ));
            return;
        }
    }
    skip( "WOW64INFO not found\n" );
}


static DWORD WINAPI iosb_delayed_write_thread(void *arg)
{
    HANDLE client = arg;
    DWORD size;
    BOOL ret;

    Sleep(100);

    ret = WriteFile( client, "data", sizeof("data"), &size, NULL );
    ok( ret == TRUE, "got error %lu\n", GetLastError() );

    return 0;
}


static void test_iosb(void)
{
    static const char pipe_name[] = "\\\\.\\pipe\\wow64iosbnamedpipe";
    HANDLE client, server, thread;
    NTSTATUS status;
    ULONG64 read_func, flush_func;
    IO_STATUS_BLOCK iosb32;
    char buffer[6];
    DWORD size;
    BOOL ret;
    struct
    {
        union
        {
            NTSTATUS Status;
            ULONG64 Pointer;
        };
        ULONG64 Information;
    } iosb64;
    ULONG64 args[] = { 0, 0, 0, 0, (ULONG_PTR)&iosb64, (ULONG_PTR)buffer, sizeof(buffer), 0, 0 };
    ULONG64 flush_args[] = { 0, (ULONG_PTR)&iosb64 };

    if (!is_wow64) return;
    if (!code_mem) return;
    if (!ntdll_module) return;
    read_func = get_proc_address64( ntdll_module, "NtReadFile" );
    flush_func = get_proc_address64( ntdll_module, "NtFlushBuffersFile" );

    /* async calls set iosb32 but not iosb64 */

    server = CreateNamedPipeA( pipe_name, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                               PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                               4, 1024, 1024, 1000, NULL );
    ok( server != INVALID_HANDLE_VALUE, "CreateNamedPipe failed: %lu\n", GetLastError() );

    client = CreateFileA( pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                          FILE_FLAG_NO_BUFFERING, NULL );
    ok( client != INVALID_HANDLE_VALUE, "CreateFile failed: %lu\n", GetLastError() );

    memset( buffer, 0xcc, sizeof(buffer) );
    memset( &iosb32, 0x55, sizeof(iosb32) );
    iosb64.Pointer = PtrToUlong( &iosb32 );
    iosb64.Information = 0xdeadbeef;

    args[0] = (LONG_PTR)server;
    status = call_func64( read_func, ARRAY_SIZE(args), args );
    ok( status == STATUS_PENDING, "NtReadFile returned %lx\n", status );
    ok( iosb32.Status == 0x55555555, "status changed to %lx\n", iosb32.Status );
    ok( iosb64.Pointer == PtrToUlong(&iosb32), "pointer changed to %I64x\n", iosb64.Pointer );
    ok( iosb64.Information == 0xdeadbeef, "info changed to %Ix\n", (ULONG_PTR)iosb64.Information );

    ret = WriteFile( client, "data", sizeof("data"), &size, NULL );
    ok( ret == TRUE, "got error %lu\n", GetLastError() );

    ok( iosb32.Status == 0, "Wrong iostatus %lx\n", iosb32.Status );
    ok( iosb32.Information == sizeof("data"), "Wrong information %Ix\n", iosb32.Information );
    ok( iosb64.Pointer == PtrToUlong(&iosb32), "pointer changed to %I64x\n", iosb64.Pointer );
    ok( iosb64.Information == 0xdeadbeef, "info changed to %Ix\n", (ULONG_PTR)iosb64.Information );
    ok( !memcmp( buffer, "data", iosb32.Information ),
        "got wrong data %s\n", debugstr_an(buffer, iosb32.Information) );

    memset( buffer, 0xcc, sizeof(buffer) );
    memset( &iosb32, 0x55, sizeof(iosb32) );
    iosb64.Pointer = PtrToUlong( &iosb32 );
    iosb64.Information = 0xdeadbeef;

    ret = WriteFile( client, "data", sizeof("data"), &size, NULL );
    ok( ret == TRUE, "got error %lu\n", GetLastError() );

    status = call_func64( read_func, ARRAY_SIZE(args), args );
    ok( status == STATUS_SUCCESS, "NtReadFile returned %lx\n", status );
    ok( iosb32.Status == STATUS_SUCCESS, "status changed to %lx\n", iosb32.Status );
    ok( iosb32.Information == sizeof("data"), "info changed to %Ix\n", iosb32.Information );
    ok( iosb64.Pointer == PtrToUlong(&iosb32), "pointer changed to %I64x\n", iosb64.Pointer );
    ok( iosb64.Information == 0xdeadbeef, "info changed to %Ix\n", (ULONG_PTR)iosb64.Information );
    ok( !memcmp( buffer, "data", iosb32.Information ),
        "got wrong data %s\n", debugstr_an(buffer, iosb32.Information) );

    /* syscalls which are always synchronous set iosb64 but not iosb32 */

    memset( &iosb32, 0x55, sizeof(iosb32) );
    iosb64.Pointer = PtrToUlong( &iosb32 );
    iosb64.Information = 0xdeadbeef;

    flush_args[0] = (LONG_PTR)server;
    status = call_func64( flush_func, ARRAY_SIZE(flush_args), flush_args );
    ok( status == STATUS_SUCCESS, "NtFlushBuffersFile returned %lx\n", status );
    ok( iosb32.Status == 0x55555555, "status changed to %lx\n", iosb32.Status );
    ok( iosb32.Information == 0x55555555, "info changed to %Ix\n", iosb32.Information );
    ok( iosb64.Pointer == STATUS_SUCCESS, "pointer changed to %I64x\n", iosb64.Pointer );
    ok( iosb64.Information == 0, "info changed to %Ix\n", (ULONG_PTR)iosb64.Information );

    CloseHandle( client );
    CloseHandle( server );

    /* synchronous calls set iosb64 but not iosb32 */

    server = CreateNamedPipeA( pipe_name, PIPE_ACCESS_DUPLEX,
                               PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                               4, 1024, 1024, 1000, NULL );
    ok( server != INVALID_HANDLE_VALUE, "CreateNamedPipe failed: %lu\n", GetLastError() );

    client = CreateFileA( pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                          FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL );
    ok( client != INVALID_HANDLE_VALUE, "CreateFile failed: %lu\n", GetLastError() );

    ret = WriteFile( client, "data", sizeof("data"), &size, NULL );
    ok( ret == TRUE, "got error %lu\n", GetLastError() );

    memset( buffer, 0xcc, sizeof(buffer) );
    memset( &iosb32, 0x55, sizeof(iosb32) );
    iosb64.Pointer = PtrToUlong( &iosb32 );
    iosb64.Information = 0xdeadbeef;

    args[0] = (LONG_PTR)server;
    status = call_func64( read_func, ARRAY_SIZE(args), args );
    ok( status == STATUS_SUCCESS, "NtReadFile returned %lx\n", status );
    ok( iosb32.Status == 0x55555555, "status changed to %lx\n", iosb32.Status );
    ok( iosb32.Information == 0x55555555, "info changed to %Ix\n", iosb32.Information );
    ok( iosb64.Pointer == STATUS_SUCCESS, "pointer changed to %I64x\n", iosb64.Pointer );
    ok( iosb64.Information == sizeof("data"), "info changed to %Ix\n", (ULONG_PTR)iosb64.Information );
    ok( !memcmp( buffer, "data", iosb64.Information ),
        "got wrong data %s\n", debugstr_an(buffer, iosb64.Information) );

    thread = CreateThread( NULL, 0, iosb_delayed_write_thread, client, 0, NULL );

    memset( buffer, 0xcc, sizeof(buffer) );
    memset( &iosb32, 0x55, sizeof(iosb32) );
    iosb64.Pointer = PtrToUlong( &iosb32 );
    iosb64.Information = 0xdeadbeef;

    args[0] = (LONG_PTR)server;
    status = call_func64( read_func, ARRAY_SIZE(args), args );
    ok( status == STATUS_SUCCESS, "NtReadFile returned %lx\n", status );
    todo_wine
    {
    ok( iosb32.Status == 0x55555555, "status changed to %lx\n", iosb32.Status );
    ok( iosb32.Information == 0x55555555, "info changed to %Ix\n", iosb32.Information );
    ok( iosb64.Pointer == STATUS_SUCCESS, "pointer changed to %I64x\n", iosb64.Pointer );
    ok( iosb64.Information == sizeof("data"), "info changed to %Ix\n", (ULONG_PTR)iosb64.Information );
    ok( !memcmp( buffer, "data", iosb64.Information ),
        "got wrong data %s\n", debugstr_an(buffer, iosb64.Information) );
    }

    ret = WaitForSingleObject( thread, 1000 );
    ok(!ret, "got %d\n", ret );
    CloseHandle( thread );

    memset( &iosb32, 0x55, sizeof(iosb32) );
    iosb64.Pointer = PtrToUlong( &iosb32 );
    iosb64.Information = 0xdeadbeef;

    flush_args[0] = (LONG_PTR)server;
    status = call_func64( flush_func, ARRAY_SIZE(flush_args), flush_args );
    ok( status == STATUS_SUCCESS, "NtFlushBuffersFile returned %lx\n", status );
    ok( iosb32.Status == 0x55555555, "status changed to %lx\n", iosb32.Status );
    ok( iosb32.Information == 0x55555555, "info changed to %Ix\n", iosb32.Information );
    ok( iosb64.Pointer == STATUS_SUCCESS, "pointer changed to %I64x\n", iosb64.Pointer );
    ok( iosb64.Information == 0, "info changed to %Ix\n", (ULONG_PTR)iosb64.Information );

    CloseHandle( client );
    CloseHandle( server );
}

static NTSTATUS invoke_syscall( const char *name, ULONG args32[] )
{
    ULONG64 args64[] = { -1, PtrToUlong( args32 ) };
    ULONG64 func = get_proc_address64( wow64_module, "Wow64SystemServiceEx" );
    BYTE *syscall = (BYTE *)GetProcAddress( GetModuleHandleA("ntdll.dll"), name );

    ok( syscall != NULL, "syscall %s not found\n", name );
    if (syscall[0] == 0xb8)
        args64[0] = *(DWORD *)(syscall + 1);
    else
        win_skip( "syscall thunk %s not recognized\n", name );

    return call_func64( func, ARRAY_SIZE(args64), args64 );
}

static void test_syscalls(void)
{
    ULONG64 func;
    ULONG args32[8];
    HANDLE event, event2;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING name;
    NTSTATUS status;

    if (!is_wow64) return;
    if (!code_mem) return;
    if (!ntdll_module) return;

    func = get_proc_address64( wow64_module, "Wow64SystemServiceEx" );
    ok( func, "Wow64SystemServiceEx not found\n" );

    event = CreateEventA( NULL, FALSE, FALSE, NULL );

    status = NtSetEvent( event, NULL );
    ok( !status, "NtSetEvent failed %lx\n", status );
    args32[0] = HandleToLong( event );
    status = invoke_syscall( "NtClose", args32 );
    ok( !status, "syscall failed %lx\n", status );
    status = NtSetEvent( event, NULL );
    ok( status == STATUS_INVALID_HANDLE, "NtSetEvent failed %lx\n", status );
    status = invoke_syscall( "NtClose", args32 );
    ok( status == STATUS_INVALID_HANDLE, "syscall failed %lx\n", status );
    args32[0] = 0xdeadbeef;
    status = invoke_syscall( "NtClose", args32 );
    ok( status == STATUS_INVALID_HANDLE, "syscall failed %lx\n", status );

    RtlInitUnicodeString( &name, L"\\BaseNamedObjects\\wow64-test");
    InitializeObjectAttributes( &attr, &name, OBJ_OPENIF, 0, NULL );
    event = (HANDLE)0xdeadbeef;
    args32[0] = PtrToUlong(&event );
    args32[1] = EVENT_ALL_ACCESS;
    args32[2] = PtrToUlong( &attr );
    args32[3] = NotificationEvent;
    args32[4] = 0;
    status = invoke_syscall( "NtCreateEvent", args32 );
    ok( !status, "syscall failed %lx\n", status );
    status = NtSetEvent( event, NULL );
    ok( !status, "NtSetEvent failed %lx\n", status );

    event2 = (HANDLE)0xdeadbeef;
    args32[0] = PtrToUlong( &event2 );
    status = invoke_syscall( "NtOpenEvent", args32 );
    ok( !status, "syscall failed %lx\n", status );
    status = NtSetEvent( event2, NULL );
    ok( !status, "NtSetEvent failed %lx\n", status );
    args32[0] = HandleToLong( event2 );
    status = invoke_syscall( "NtClose", args32 );
    ok( !status, "syscall failed %lx\n", status );

    event2 = (HANDLE)0xdeadbeef;
    args32[0] = PtrToUlong( &event2 );
    status = invoke_syscall( "NtCreateEvent", args32 );
    ok( status == STATUS_OBJECT_NAME_EXISTS, "syscall failed %lx\n", status );
    status = NtSetEvent( event2, NULL );
    ok( !status, "NtSetEvent failed %lx\n", status );
    args32[0] = HandleToLong( event2 );
    status = invoke_syscall( "NtClose", args32 );
    ok( !status, "syscall failed %lx\n", status );

    status = NtClose( event );
    ok( !status, "NtClose failed %lx\n", status );

    if (pNtWow64ReadVirtualMemory64)
    {
        TEB64 *teb64 = (TEB64 *)NtCurrentTeb()->GdiBatchCount;
        PEB64 peb64, peb64_2;
        ULONG64 res, res2;
        HANDLE process = OpenProcess( PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId() );
        ULONG args32[] = { HandleToLong( process ), (ULONG)teb64->Peb, teb64->Peb >> 32,
                           PtrToUlong(&peb64_2), sizeof(peb64_2), 0, PtrToUlong(&res2) };

        ok( process != 0, "failed to open current process %lu\n", GetLastError() );
        status = pNtWow64ReadVirtualMemory64( process, teb64->Peb, &peb64, sizeof(peb64), &res );
        ok( !status, "NtWow64ReadVirtualMemory64 failed %lx\n", status );
        status = invoke_syscall( "NtWow64ReadVirtualMemory64", args32 );
        ok( !status, "NtWow64ReadVirtualMemory64 failed %lx\n", status );
        ok( res2 == res, "wrong len %s / %s\n", wine_dbgstr_longlong(res), wine_dbgstr_longlong(res2) );
        ok( !memcmp( &peb64, &peb64_2, res ), "data is different\n" );
        NtClose( process );
    }
}

static void test_cpu_area(void)
{
    TEB64 *teb64 = (TEB64 *)NtCurrentTeb()->GdiBatchCount;
    ULONG64 ptr;
    NTSTATUS status;

    if (!is_wow64) return;
    if (!code_mem) return;
    if (!ntdll_module) return;

    if ((ptr = get_proc_address64( ntdll_module, "RtlWow64GetCurrentCpuArea" )))
    {
        USHORT machine = 0xdead;
        ULONG64 context, context_ex;
        ULONG64 args[] = { (ULONG_PTR)&machine, (ULONG_PTR)&context, (ULONG_PTR)&context_ex };

        status = call_func64( ptr, ARRAY_SIZE(args), args );
        ok( !status, "RtlWow64GetCpuAreaInfo failed %lx\n", status );
        ok( machine == IMAGE_FILE_MACHINE_I386, "wrong machine %x\n", machine );
        ok( context == teb64->TlsSlots[WOW64_TLS_CPURESERVED] + 4, "wrong context %s / %s\n",
            wine_dbgstr_longlong(context), wine_dbgstr_longlong(teb64->TlsSlots[WOW64_TLS_CPURESERVED]) );
        ok( !context_ex, "got context_ex %s\n", wine_dbgstr_longlong(context_ex) );
        args[0] = args[1] = args[2] = 0;
        status = call_func64( ptr, ARRAY_SIZE(args), args );
        ok( !status, "RtlWow64GetCpuAreaInfo failed %lx\n", status );
    }
    else win_skip( "RtlWow64GetCpuAreaInfo not supported\n" );

}

static void test_exception_dispatcher(void)
{
    ULONG64 ptr, hook_ptr, hook, expect, res;
    NTSTATUS status;
    BYTE code[8];

    if (!is_wow64) return;
    if (!code_mem) return;
    if (!ntdll_module) return;

    ptr = get_proc_address64( ntdll_module, "KiUserExceptionDispatcher" );
    ok( ptr, "KiUserExceptionDispatcher not found\n" );

    if (pNtWow64ReadVirtualMemory64)
    {
        HANDLE process = OpenProcess( PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId() );

        ok( process != 0, "failed to open current process %lu\n", GetLastError() );
        status = pNtWow64ReadVirtualMemory64( process, ptr, &code, sizeof(code), &res );
        ok( !status, "NtWow64ReadVirtualMemory64 failed %lx\n", status );

        /* cld; mov xxx(%rip),%rax */
        ok( code[0] == 0xfc && code[1] == 0x48 && code[2] == 0x8b && code[3] == 0x05,
            "wrong opcodes %02x %02x %02x %02x\n", code[0], code[1], code[2], code[3] );
        hook_ptr = ptr + 8 + *(int *)(code + 4);
        status = pNtWow64ReadVirtualMemory64( process, hook_ptr, &hook, sizeof(hook), &res );
        ok( !status, "NtWow64ReadVirtualMemory64 failed %lx\n", status );

        expect = get_proc_address64( wow64_module, "Wow64PrepareForException" );
        ok( hook == expect, "hook %I64x set to %I64x / %I64x\n", hook_ptr, hook, expect );
        NtClose( process );
    }
}

#endif  /* _WIN64 */

static void test_arm64ec(void)
{
#ifdef __aarch64__
    PROCESS_INFORMATION pi;
    char cmdline[MAX_PATH];
    char **argv;

    trace( "restarting test as arm64ec\n" );

    winetest_get_mainargs( &argv );
    sprintf( cmdline, "%s %s", argv[0], argv[1] );
    if (create_process_machine( cmdline, 0, IMAGE_FILE_MACHINE_AMD64, &pi ))
    {
        DWORD exit_code, ret = WaitForSingleObject( pi.hProcess, 10000 );
        ok( ret == 0, "wait failed %lx\n", ret );
        GetExitCodeProcess( pi.hProcess, &exit_code );
        ok( exit_code == 0xbeef, "wrong exit code %lx\n", exit_code );
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
    }
    else skip( "could not start arm64ec process: %lu\n", GetLastError() );
#endif
}

START_TEST(wow64)
{
    init();
#if !defined (__REACTOS__) || (DLL_EXPORT_VERSION >= 0x600)
    test_query_architectures();
#endif
    test_peb_teb();
    test_selectors();
    test_image_mappings();
#ifdef _WIN64
    test_xtajit64();
    test_cross_process_work_list();
#else
    test_nt_wow64();
    test_modules();
    test_init_block();
    test_iosb();
    test_syscalls();
#endif
    test_memory_notifications();
    test_cpu_area();
    test_exception_dispatcher();
    test_arm64ec();
}
