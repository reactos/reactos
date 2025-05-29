/*
 * Win32 debugger functions
 *
 * Copyright (C) 1999 Alexandre Julliard
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#define PSAPI_VERSION 1  /* avoid K32 function remapping */
#include "psapi.h"
#include "werapi.h"

#include "wine/exception.h"
#include "wine/asm.h"
#include "kernelbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);
WINE_DECLARE_DEBUG_CHANNEL(winedbg);

typedef INT (WINAPI *MessageBoxA_funcptr)(HWND,LPCSTR,LPCSTR,UINT);
typedef INT (WINAPI *MessageBoxW_funcptr)(HWND,LPCWSTR,LPCWSTR,UINT);

static PTOP_LEVEL_EXCEPTION_FILTER top_filter;

void *dummy = RtlUnwind;  /* force importing RtlUnwind from ntdll */

/***********************************************************************
 *           CheckRemoteDebuggerPresent   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CheckRemoteDebuggerPresent( HANDLE process, BOOL *present )
{
    DWORD_PTR port;

    if (!process || !present)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!set_ntstatus( NtQueryInformationProcess( process, ProcessDebugPort, &port, sizeof(port), NULL )))
        return FALSE;
    *present = !!port;
    return TRUE;
}


/**********************************************************************
 *           ContinueDebugEvent   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ContinueDebugEvent( DWORD pid, DWORD tid, DWORD status )
{
    CLIENT_ID id;

    id.UniqueProcess = ULongToHandle( pid );
    id.UniqueThread  = ULongToHandle( tid );
    return set_ntstatus( DbgUiContinue( &id, status ));
}


/**********************************************************************
 *           DebugActiveProcess   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DebugActiveProcess( DWORD pid )
{
    HANDLE process;
    NTSTATUS status;

    if (!set_ntstatus( DbgUiConnectToDbg() )) return FALSE;
    if (!(process = OpenProcess( PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_SUSPEND_RESUME |
                                 PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD, FALSE, pid )))
        return FALSE;
    status = DbgUiDebugActiveProcess( process );
    NtClose( process );
    return set_ntstatus( status );
}


/**********************************************************************
 *           DebugActiveProcessStop   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DebugActiveProcessStop( DWORD pid )
{
    HANDLE process;
    NTSTATUS status;

    if (!(process = OpenProcess( PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_SUSPEND_RESUME, FALSE, pid )))
        return FALSE;
    status = DbgUiStopDebugging( process );
    NtClose( process );
    return set_ntstatus( status );
}


/***********************************************************************
 *           DebugBreak   (kernelbase.@)
 */
#ifdef __i386__
__ASM_STDCALL_FUNC( DebugBreak, 0, "jmp " __ASM_STDCALL("DbgBreakPoint", 0) )
#elif defined(__aarch64__)
__ASM_GLOBAL_FUNC( DebugBreak, "brk #0xf000; ret" )
#elif defined(__arm64ec__)
void __attribute__((naked)) WINAPI DebugBreak(void) { asm( "brk #0xf000; ret" ); }
#elif defined(__x86_64__)
__ASM_GLOBAL_FUNC( DebugBreak, "jmp " __ASM_NAME("DbgBreakPoint") )
#elif defined(__arm__)
__ASM_GLOBAL_FUNC( DebugBreak, "udf #0xfe; bx lr" )
#endif


/**************************************************************************
 *           FatalAppExitA   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH FatalAppExitA( UINT action, LPCSTR str )
{
    HMODULE mod = GetModuleHandleA( "user32.dll" );
    MessageBoxA_funcptr pMessageBoxA = NULL;

    if (mod) pMessageBoxA = (MessageBoxA_funcptr)GetProcAddress( mod, "MessageBoxA" );
    if (pMessageBoxA) pMessageBoxA( 0, str, NULL, MB_SYSTEMMODAL | MB_OK );
    else ERR( "%s\n", debugstr_a(str) );
    RtlExitUserProcess( 1 );
}


/**************************************************************************
 *           FatalAppExitW   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH FatalAppExitW( UINT action, LPCWSTR str )
{
    HMODULE mod = GetModuleHandleW( L"user32.dll" );
    MessageBoxW_funcptr pMessageBoxW = NULL;

    if (mod) pMessageBoxW = (MessageBoxW_funcptr)GetProcAddress( mod, "MessageBoxW" );
    if (pMessageBoxW) pMessageBoxW( 0, str, NULL, MB_SYSTEMMODAL | MB_OK );
    else ERR( "%s\n", debugstr_w(str) );
    RtlExitUserProcess( 1 );
}


/***********************************************************************
 *           IsDebuggerPresent   (kernelbase.@)
 */
BOOL WINAPI IsDebuggerPresent(void)
{
    return NtCurrentTeb()->Peb->BeingDebugged;
}


static LONG WINAPI debug_exception_handler( EXCEPTION_POINTERS *eptr )
{
    EXCEPTION_RECORD *rec = eptr->ExceptionRecord;
    return (rec->ExceptionCode == DBG_PRINTEXCEPTION_C) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

/***********************************************************************
 *           OutputDebugStringA   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH OutputDebugStringA( LPCSTR str )
{
    static HANDLE DBWinMutex = NULL;
    static BOOL mutex_inited = FALSE;
    BOOL caught_by_dbg = TRUE;

    if (!str) str = "";
    WARN( "%s\n", debugstr_a(str) );

    /* raise exception, WaitForDebugEvent() will generate a corresponding debug event */
    __TRY
    {
        ULONG_PTR args[2];
        args[0] = strlen(str) + 1;
        args[1] = (ULONG_PTR)str;
        RaiseException( DBG_PRINTEXCEPTION_C, 0, 2, args );
    }
    __EXCEPT(debug_exception_handler)
    {
        caught_by_dbg = FALSE;
    }
    __ENDTRY
    if (caught_by_dbg) return;

    /* send string to a system-wide monitor */
    if (!mutex_inited)
    {
        /* first call to OutputDebugString, initialize mutex handle */
        HANDLE mutex = CreateMutexExW( NULL, L"DBWinMutex", 0, SYNCHRONIZE );
        if (mutex)
        {
            if (InterlockedCompareExchangePointer( &DBWinMutex, mutex, 0 ) != 0)
                /* someone beat us here... */
                CloseHandle( mutex );
        }
        mutex_inited = TRUE;
    }

    if (DBWinMutex)
    {
        HANDLE mapping;

        mapping = OpenFileMappingW( FILE_MAP_WRITE, FALSE, L"DBWIN_BUFFER" );
        if (mapping)
        {
            LPVOID buffer;
            HANDLE eventbuffer, eventdata;

            buffer = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 0 );
            eventbuffer = OpenEventW( SYNCHRONIZE, FALSE, L"DBWIN_BUFFER_READY" );
            eventdata = OpenEventW( EVENT_MODIFY_STATE, FALSE, L"DBWIN_DATA_READY" );

            if (buffer && eventbuffer && eventdata)
            {
                /* monitor is present, synchronize with other OutputDebugString invocations */
                WaitForSingleObject( DBWinMutex, INFINITE );

                /* acquire control over the buffer */
                if (WaitForSingleObject( eventbuffer, 10000 ) == WAIT_OBJECT_0)
                {
                    int str_len = strlen( str );
                    struct _mon_buffer_t
                    {
                        DWORD pid;
                        char buffer[1];
                    } *mon_buffer = (struct _mon_buffer_t*) buffer;

                    if (str_len > (4096 - sizeof(DWORD) - 1)) str_len = 4096 - sizeof(DWORD) - 1;
                    mon_buffer->pid = GetCurrentProcessId();
                    memcpy( mon_buffer->buffer, str, str_len );
                    mon_buffer->buffer[str_len] = 0;

                    /* signal data ready */
                    SetEvent( eventdata );
                }
                ReleaseMutex( DBWinMutex );
            }

            if (buffer) UnmapViewOfFile( buffer );
            if (eventbuffer) CloseHandle( eventbuffer );
            if (eventdata) CloseHandle( eventdata );
            CloseHandle( mapping );
        }
    }
}

static LONG WINAPI debug_exception_handler_wide( EXCEPTION_POINTERS *eptr )
{
    EXCEPTION_RECORD *rec = eptr->ExceptionRecord;
    return (rec->ExceptionCode == DBG_PRINTEXCEPTION_WIDE_C) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

/***********************************************************************
 *           OutputDebugStringW   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH OutputDebugStringW( LPCWSTR str )
{
    UNICODE_STRING strW;
    STRING strA;

    WARN( "%s\n", debugstr_w(str) );

    RtlInitUnicodeString( &strW, str );
    if (!RtlUnicodeStringToAnsiString( &strA, &strW, TRUE ))
    {
        BOOL exc_handled;

        __TRY
        {
            ULONG_PTR args[4];
            args[0] = wcslen(str) + 1;
            args[1] = (ULONG_PTR)str;
            args[2] = strlen(strA.Buffer) + 1;
            args[3] = (ULONG_PTR)strA.Buffer;
            RaiseException( DBG_PRINTEXCEPTION_WIDE_C, 0, 4, args );
            exc_handled = TRUE;
        }
        __EXCEPT(debug_exception_handler_wide)
        {
            exc_handled = FALSE;
        }
        __ENDTRY

        if (!exc_handled)
            OutputDebugStringA( strA.Buffer );

        RtlFreeAnsiString( &strA );
    }
}


/*******************************************************************
 *           RaiseException  (kernelbase.@)
 */
#ifdef __x86_64__
#ifdef __arm64ec__
void __attribute__((naked)) RaiseException( DWORD code, DWORD flags, DWORD count, const ULONG_PTR *args )
{
    asm( ".seh_proc RaiseException\n\t"
         "stp x29, x30, [sp, #-0xb0]!\n\t"
         ".seh_save_fplr_x 0xb0\n\t"
         ".seh_endprologue\n\t"
         "and w1, w1, #0x01\n\t"        /* EXCEPTION_NONCONTINUABLE */
         "stp w0, w1, [sp, #0x10]\n\t"  /* ExceptionCode, ExceptionFlags */
         "adr x4, RaiseException\n\t"
         "stp xzr, x4, [sp, #0x18]\n\t" /* ExceptionRecord, ExceptionAddress */
         "mov w5, #0x0f\n\t"            /* EXCEPTION_MAXIMUM_PARAMETERS */
         "cmp w2, w5\n\t"
         "csel w2, w2, w5, lo\n\t"
         "str x2, [sp, #0x28]\n\t"      /* NumberParameters */
         "cbz x3, 1f\n\t"
         "lsl w2, w2, #3\n\t"
         "add x0, sp, #0x30\n\t"        /* ExceptionInformation */
         "mov x1, x3\n\t"               /* args */
         "bl \"#memcpy\"\n"
         "1:\tadd x0, sp, #0x10\n\t"    /* rec */
         "bl \"#RtlRaiseException\"\n\t"
         "ldp x29, x30, [sp], #0xb0\n\t"
         "ret\n\t"
         ".seh_endproc" );
}
#else
/* Some DRMs depend on RaiseException not altering non-volatile registers. */
__ASM_GLOBAL_FUNC( RaiseException,
                   ".byte 0x48,0x8d,0xa4,0x24,0x00,0x00,0x00,0x00\n\t" /* hotpatch prolog */
                   "sub $0xc8,%rsp\n\t"
                   __ASM_SEH(".seh_stackalloc 0xc8\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 0xc8\n\t")
                   "leaq 0x20(%rsp),%rax\n\t"
                   "movl %ecx,(%rax)\n\t" /* ExceptionCode */
                   "and $1,%edx\n\t"
                   "movl %edx,4(%rax)\n\t" /* ExceptionFlags */
                   "movq $0,8(%rax)\n\t" /* ExceptionRecord */
                   "leaq " __ASM_NAME("RaiseException") "(%rip),%rcx\n\t"
                   "movq %rcx,0x10(%rax)\n\t" /* ExceptionAddress */
                   "movq %rax,%rcx\n\t"
                   "movl $0,0x18(%rcx)\n\t" /* NumberParameters */
                   "testl %r8d,%r8d\n\t"
                   "jz 2f\n\t"
                   "testq %r9,%r9\n\t"
                   "jz 2f\n\t"
                   "movl $15,%edx\n\t"
                   "cmp %edx,%r8d\n\t"
                   "cmovb %r8d,%edx\n\t"
                   "movl %edx,0x18(%rcx)\n\t" /* NumberParameters */
                   "leaq 0x20(%rcx),%rax\n" /* ExceptionInformation */
                   "1:\tmovq (%r9),%r8\n\t"
                   "movq %r8,(%rax)\n\t"
                   "decl %edx\n\t"
                   "jz 2f\n\t"
                   "addq $8,%rax\n\t"
                   "addq $8,%r9\n\t"
                   "jmp 1b\n"
                   "2:\tcall " __ASM_NAME("RtlRaiseException") "\n\t"
                   "add $0xc8,%rsp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -0xc8\n\t")
                   "ret" )
#endif  /* __arm64ec__ */
C_ASSERT( offsetof(EXCEPTION_RECORD, ExceptionCode) == 0 );
C_ASSERT( offsetof(EXCEPTION_RECORD, ExceptionFlags) == 4 );
C_ASSERT( offsetof(EXCEPTION_RECORD, ExceptionRecord) == 8 );
C_ASSERT( offsetof(EXCEPTION_RECORD, ExceptionAddress) == 0x10 );
C_ASSERT( offsetof(EXCEPTION_RECORD, NumberParameters) == 0x18 );
C_ASSERT( offsetof(EXCEPTION_RECORD, ExceptionInformation) == 0x20 );
#else  /* __x86_64__ */
void WINAPI DECLSPEC_HOTPATCH RaiseException( DWORD code, DWORD flags, DWORD count, const ULONG_PTR *args )
{
    EXCEPTION_RECORD record;

    record.ExceptionCode    = code;
    record.ExceptionFlags   = flags & EXCEPTION_NONCONTINUABLE;
    record.ExceptionRecord  = NULL;
    record.ExceptionAddress = RaiseException;
    if (count && args)
    {
        if (count > EXCEPTION_MAXIMUM_PARAMETERS) count = EXCEPTION_MAXIMUM_PARAMETERS;
        record.NumberParameters = count;
        memcpy( record.ExceptionInformation, args, count * sizeof(*args) );
    }
    else record.NumberParameters = 0;

    RtlRaiseException( &record );
}
#endif

#ifdef __i386__
__ASM_STDCALL_IMPORT(RaiseException,16)
#else
__ASM_GLOBAL_IMPORT(RaiseException)
#endif

/*******************************************************************
 *           RaiseFailFastException  (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH RaiseFailFastException( EXCEPTION_RECORD *record, CONTEXT *context, DWORD flags )
{
    FIXME( "(%p, %p, %ld) stub\n", record, context, flags );
    TerminateProcess( GetCurrentProcess(), STATUS_FAIL_FAST_EXCEPTION );
}

/***********************************************************************
 *           SetUnhandledExceptionFilter   (kernelbase.@)
 */
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI DECLSPEC_HOTPATCH SetUnhandledExceptionFilter(
    LPTOP_LEVEL_EXCEPTION_FILTER filter )
{
    return InterlockedExchangePointer( (void **)&top_filter, filter );
}


/*******************************************************************
 *         format_exception_msg
 */
static void format_exception_msg( const EXCEPTION_POINTERS *ptr, char *buffer, int size )
{
    const EXCEPTION_RECORD *rec = ptr->ExceptionRecord;
    int len;

    switch(rec->ExceptionCode)
    {
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        len = snprintf( buffer, size, "Unhandled division by zero" );
        break;
    case EXCEPTION_INT_OVERFLOW:
        len = snprintf( buffer, size, "Unhandled overflow" );
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        len = snprintf( buffer, size, "Unhandled array bounds" );
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        len = snprintf( buffer, size, "Unhandled illegal instruction" );
        break;
    case EXCEPTION_STACK_OVERFLOW:
        len = snprintf( buffer, size, "Unhandled stack overflow" );
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        len = snprintf( buffer, size, "Unhandled privileged instruction" );
        break;
    case EXCEPTION_ACCESS_VIOLATION:
        if (rec->NumberParameters == 2)
            len = snprintf( buffer, size, "Unhandled page fault on %s access to %p",
                            rec->ExceptionInformation[0] == EXCEPTION_WRITE_FAULT ? "write" :
                            rec->ExceptionInformation[0] == EXCEPTION_EXECUTE_FAULT ? "execute" : "read",
                            (void *)rec->ExceptionInformation[1]);
        else
            len = snprintf( buffer, size, "Unhandled page fault");
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        len = snprintf( buffer, size, "Unhandled alignment" );
        break;
    case CONTROL_C_EXIT:
        len = snprintf( buffer, size, "Unhandled ^C");
        break;
    case STATUS_POSSIBLE_DEADLOCK:
        len = snprintf( buffer, size, "Critical section %p wait failed",
                        (void *)rec->ExceptionInformation[0]);
        break;
    case EXCEPTION_WINE_STUB:
        if ((ULONG_PTR)rec->ExceptionInformation[1] >> 16)
            len = snprintf( buffer, size, "Unimplemented function %s.%s called",
                            (char *)rec->ExceptionInformation[0], (char *)rec->ExceptionInformation[1] );
        else
            len = snprintf( buffer, size, "Unimplemented function %s.%Id called",
                            (char *)rec->ExceptionInformation[0], rec->ExceptionInformation[1] );
        break;
    case EXCEPTION_WINE_ASSERTION:
        len = snprintf( buffer, size, "Assertion failed" );
        break;
    default:
        len = snprintf( buffer, size, "Unhandled exception 0x%08lx in thread %lx",
                        rec->ExceptionCode, GetCurrentThreadId());
        break;
    }
    if (len < 0 || len >= size) return;
    snprintf( buffer + len,  size - len, " at address %p", ptr->ExceptionRecord->ExceptionAddress );
}


/******************************************************************
 *		start_debugger
 *
 * Does the effective debugger startup according to 'format'
 */
static BOOL start_debugger( EXCEPTION_POINTERS *epointers, HANDLE event )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW = RTL_CONSTANT_STRING( L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug" );
    WCHAR *cmdline, *env, *p, *format = NULL;
    HANDLE dbg_key;
    DWORD autostart = TRUE;
    PROCESS_INFORMATION	info;
    STARTUPINFOW startup;
    BOOL ret = FALSE;
    char buffer[256];

    format_exception_msg( epointers, buffer, sizeof(buffer) );
    MESSAGE( "wine: %s (thread %04lx), starting debugger...\n", buffer, GetCurrentThreadId() );

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    if (!NtOpenKey( &dbg_key, KEY_READ, &attr ))
    {
        KEY_VALUE_PARTIAL_INFORMATION *info;
        DWORD format_size = 0;

        RtlInitUnicodeString( &nameW, L"Debugger" );
        if (NtQueryValueKey( dbg_key, &nameW, KeyValuePartialInformation,
                             NULL, 0, &format_size ) == STATUS_BUFFER_TOO_SMALL)
        {
            char *data = HeapAlloc( GetProcessHeap(), 0, format_size );
            NtQueryValueKey( dbg_key, &nameW, KeyValuePartialInformation,
                             data, format_size, &format_size );
            info = (KEY_VALUE_PARTIAL_INFORMATION *)data;
            format = HeapAlloc( GetProcessHeap(), 0, info->DataLength + sizeof(WCHAR) );
            memcpy( format, info->Data, info->DataLength );
            format[info->DataLength / sizeof(WCHAR)] = 0;

            if (info->Type == REG_EXPAND_SZ)
            {
                WCHAR *tmp;

                format_size = ExpandEnvironmentStringsW( format, NULL, 0 );
                tmp = HeapAlloc( GetProcessHeap(), 0, format_size * sizeof(WCHAR));
                ExpandEnvironmentStringsW( format, tmp, format_size );
                HeapFree( GetProcessHeap(), 0, format );
                format = tmp;
            }
            HeapFree( GetProcessHeap(), 0, data );
        }

        RtlInitUnicodeString( &nameW, L"Auto" );
        if (!NtQueryValueKey( dbg_key, &nameW, KeyValuePartialInformation,
                              buffer, sizeof(buffer)-sizeof(WCHAR), &format_size ))
       {
           info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
           if (info->Type == REG_DWORD) memcpy( &autostart, info->Data, sizeof(DWORD) );
           else if (info->Type == REG_SZ)
           {
               WCHAR *str = (WCHAR *)info->Data;
               str[info->DataLength/sizeof(WCHAR)] = 0;
               autostart = wcstol( str, NULL, 10 );
           }
       }

       NtClose( dbg_key );
    }

    if (format)
    {
        size_t format_size = lstrlenW( format ) + 2*20;
        cmdline = HeapAlloc( GetProcessHeap(), 0, format_size * sizeof(WCHAR) );
        swprintf( cmdline, format_size, format, GetCurrentProcessId(), HandleToLong(event) );
        HeapFree( GetProcessHeap(), 0, format );
    }
    else
    {
        cmdline = HeapAlloc( GetProcessHeap(), 0, 80 * sizeof(WCHAR) );
        swprintf( cmdline, 80, L"winedbg --auto %ld %ld", GetCurrentProcessId(), HandleToLong(event) );
    }

    if (!autostart)
    {
	HMODULE mod = GetModuleHandleA( "user32.dll" );
	MessageBoxA_funcptr pMessageBoxA = NULL;

	if (mod) pMessageBoxA = (void *)GetProcAddress( mod, "MessageBoxA" );
	if (pMessageBoxA)
	{
            static const char msg[] = ".\nDo you wish to debug it?";

            format_exception_msg( epointers, buffer, sizeof(buffer) - sizeof(msg) );
            strcat( buffer, msg );
	    if (pMessageBoxA( 0, buffer, "Exception raised", MB_YESNO | MB_ICONHAND ) == IDNO)
	    {
		TRACE( "Killing process\n" );
		goto exit;
	    }
	}
    }

    /* make WINEDEBUG empty in the environment */
    env = GetEnvironmentStringsW();
    if (!TRACE_ON(winedbg))
    {
        for (p = env; *p; p += lstrlenW(p) + 1)
        {
            if (!wcsncmp( p, L"WINEDEBUG=", 10 ))
            {
                WCHAR *next = p + lstrlenW(p);
                WCHAR *end = next + 1;
                while (*end) end += lstrlenW(end) + 1;
                memmove( p + 10, next, end + 1 - next );
                break;
            }
        }
    }

    TRACE( "Starting debugger %s\n", debugstr_w(cmdline) );
    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    ret = CreateProcessW( NULL, cmdline, NULL, NULL, TRUE, CREATE_UNICODE_ENVIRONMENT, env, NULL, &startup, &info );
    FreeEnvironmentStringsW( env );

    if (ret)
    {
        /* wait for debugger to come up... */
        HANDLE handles[2];
        CloseHandle( info.hThread );
        handles[0] = event;
        handles[1] = info.hProcess;
        WaitForMultipleObjects( 2, handles, FALSE, INFINITE );
        CloseHandle( info.hProcess );
    }
    else ERR( "Couldn't start debugger %s (%ld)\n"
              "Read the Wine Developers Guide on how to set up winedbg or another debugger\n",
              debugstr_w(cmdline), GetLastError() );
exit:
    HeapFree(GetProcessHeap(), 0, cmdline);
    return ret;
}

/******************************************************************
 *		start_debugger_atomic
 *
 * starts the debugger in an atomic way:
 *	- either the debugger is not started and it is started
 *	- or the debugger has already been started by another thread
 *	- or the debugger couldn't be started
 *
 * returns TRUE for the two first conditions, FALSE for the last
 */
static BOOL start_debugger_atomic( EXCEPTION_POINTERS *epointers )
{
    static HANDLE once;

    if (once == 0)
    {
	OBJECT_ATTRIBUTES attr;
	HANDLE event;

	attr.Length                   = sizeof(attr);
	attr.RootDirectory            = 0;
	attr.Attributes               = OBJ_INHERIT;
	attr.ObjectName               = NULL;
	attr.SecurityDescriptor       = NULL;
	attr.SecurityQualityOfService = NULL;

	/* ask for manual reset, so that once the debugger is started,
	 * every thread will know it */
	NtCreateEvent( &event, EVENT_ALL_ACCESS, &attr, NotificationEvent, FALSE );
        if (InterlockedCompareExchangePointer( &once, event, 0 ) == 0)
	{
	    /* ok, our event has been set... we're the winning thread */
	    BOOL ret = start_debugger( epointers, once );

	    if (!ret)
	    {
		/* so that the other threads won't be stuck */
		NtSetEvent( once, NULL );
	    }
	    return ret;
	}

	/* someone beat us here... */
	CloseHandle( event );
    }

    /* and wait for the winner to have actually created the debugger */
    WaitForSingleObject( once, INFINITE );
    /* in fact, here, we only know that someone has tried to start the debugger,
     * we'll know by reposting the exception if it has actually attached
     * to the current process */
    return TRUE;
}


/*******************************************************************
 *         check_resource_write
 *
 * Check if the exception is a write attempt to the resource data.
 * If yes, we unprotect the resources to let broken apps continue
 * (Windows does this too).
 */
static BOOL check_resource_write( void *addr )
{
    DWORD old_prot;
    void *rsrc;
    DWORD size;
    MEMORY_BASIC_INFORMATION info;

    if (!VirtualQuery( addr, &info, sizeof(info) )) return FALSE;
    if (info.State == MEM_FREE || !(info.Type & MEM_IMAGE)) return FALSE;
    if (!(rsrc = RtlImageDirectoryEntryToData( info.AllocationBase, TRUE,
                                               IMAGE_DIRECTORY_ENTRY_RESOURCE, &size )))
        return FALSE;
    if (addr < rsrc || (char *)addr >= (char *)rsrc + size) return FALSE;
    TRACE( "Broken app is writing to the resource data, enabling work-around\n" );
    VirtualProtect( rsrc, size, PAGE_READWRITE, &old_prot );
    return TRUE;
}


/*******************************************************************
 *         UnhandledExceptionFilter   (kernelbase.@)
 */
LONG WINAPI UnhandledExceptionFilter( EXCEPTION_POINTERS *epointers )
{
    const EXCEPTION_RECORD *rec = epointers->ExceptionRecord;

    if (rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && rec->NumberParameters >= 2)
    {
        switch (rec->ExceptionInformation[0])
        {
        case EXCEPTION_WRITE_FAULT:
            if (check_resource_write( (void *)rec->ExceptionInformation[1] ))
                return EXCEPTION_CONTINUE_EXECUTION;
            break;
        }
    }

    if (!NtCurrentTeb()->Peb->BeingDebugged)
    {
        if (rec->ExceptionCode == CONTROL_C_EXIT)
        {
            /* do not launch the debugger on ^C, simply terminate the process */
            TerminateProcess( GetCurrentProcess(), 1 );
        }

        if (top_filter)
        {
            LONG ret = top_filter( epointers );
            if (ret != EXCEPTION_CONTINUE_SEARCH) return ret;
        }

        if ((GetErrorMode() & SEM_NOGPFAULTERRORBOX) ||
            !start_debugger_atomic( epointers ) || !NtCurrentTeb()->Peb->BeingDebugged)
            return EXCEPTION_EXECUTE_HANDLER;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}


/***********************************************************************
 *         WerGetFlags   (kernelbase.@)
 */
HRESULT WINAPI /* DECLSPEC_HOTPATCH */ WerGetFlags( HANDLE process, DWORD *flags )
{
    FIXME( "(%p, %p) stub\n", process, flags );
    return E_NOTIMPL;
}


/***********************************************************************
 *         WerRegisterFile   (kernelbase.@)
 */
HRESULT WINAPI /* DECLSPEC_HOTPATCH */ WerRegisterFile( const WCHAR *file, WER_REGISTER_FILE_TYPE type,
                                                        DWORD flags )
{
    FIXME( "(%s, %d, %ld) stub\n", debugstr_w(file), type, flags );
    return E_NOTIMPL;
}


/***********************************************************************
 *         WerRegisterMemoryBlock   (kernelbase.@)
 */
HRESULT WINAPI /* DECLSPEC_HOTPATCH */ WerRegisterMemoryBlock( void *block, DWORD size )
{
    FIXME( "(%p %ld) stub\n", block, size );
    return E_NOTIMPL;
}


/***********************************************************************
 *         WerRegisterRuntimeExceptionModule   (kernelbase.@)
 */
HRESULT WINAPI /* DECLSPEC_HOTPATCH */ WerRegisterRuntimeExceptionModule( const WCHAR *dll, void *context )
{
    FIXME( "(%s, %p) stub\n", debugstr_w(dll), context );
    return S_OK;
}


/***********************************************************************
 *         WerSetFlags   (kernelbase.@)
 */
HRESULT WINAPI /* DECLSPEC_HOTPATCH */ WerSetFlags( DWORD flags )
{
    FIXME("(%ld) stub\n", flags);
    return S_OK;
}


/***********************************************************************
 *         WerUnregisterFile   (kernelbase.@)
 */
HRESULT WINAPI /* DECLSPEC_HOTPATCH */ WerUnregisterFile( const WCHAR *file )
{
    FIXME( "(%s) stub\n", debugstr_w(file) );
    return E_NOTIMPL;
}


/***********************************************************************
 *         WerUnregisterMemoryBlock   (kernelbase.@)
 */
HRESULT WINAPI /* DECLSPEC_HOTPATCH */ WerUnregisterMemoryBlock( void *block )
{
    FIXME( "(%p) stub\n", block );
    return E_NOTIMPL;
}


/***********************************************************************
 *         WerUnregisterRuntimeExceptionModule   (kernelbase.@)
 */
HRESULT WINAPI /* DECLSPEC_HOTPATCH */ WerUnregisterRuntimeExceptionModule( const WCHAR *dll, void *context )
{
    FIXME( "(%s, %p) stub\n", debugstr_w(dll), context );
    return S_OK;
}


/***********************************************************************
 * psapi functions
 ***********************************************************************/


typedef struct _LDR_DATA_TABLE_ENTRY32
{
    LIST_ENTRY32        InLoadOrderModuleList;
    LIST_ENTRY32        InMemoryOrderModuleList;
    LIST_ENTRY32        InInitializationOrderModuleList;
    DWORD               BaseAddress;
    DWORD               EntryPoint;
    ULONG               SizeOfImage;
    UNICODE_STRING32    FullDllName;
    UNICODE_STRING32    BaseDllName;
} LDR_DATA_TABLE_ENTRY32;

struct module_iterator
{
    HANDLE                 process;
    LIST_ENTRY            *head;
    LIST_ENTRY            *current;
    BOOL                   wow64;
    LDR_DATA_TABLE_ENTRY   ldr_module;
    LDR_DATA_TABLE_ENTRY32 ldr_module32;
};


static BOOL init_module_iterator_wow64( struct module_iterator *iter, HANDLE process )
{
    PEB_LDR_DATA32 *ldr_data32_ptr;
    DWORD ldr_data32, first_module;
    PEB32 *peb32;

    iter->wow64 = TRUE;
    if (!set_ntstatus( NtQueryInformationProcess( process, ProcessWow64Information,
                                                  &peb32, sizeof(peb32), NULL )))
        return FALSE;
    if (!ReadProcessMemory( process, &peb32->LdrData, &ldr_data32, sizeof(ldr_data32), NULL ))
        return FALSE;
    ldr_data32_ptr = (PEB_LDR_DATA32 *)(DWORD_PTR) ldr_data32;
    if (!ReadProcessMemory( process, &ldr_data32_ptr->InLoadOrderModuleList.Flink,
                            &first_module, sizeof(first_module), NULL ))
        return FALSE;
    iter->head = (LIST_ENTRY *)&ldr_data32_ptr->InLoadOrderModuleList;
    iter->current = (LIST_ENTRY *)(DWORD_PTR)first_module;
    iter->process = process;
    return TRUE;
}


static BOOL init_module_iterator( struct module_iterator *iter, HANDLE process )
{
    PROCESS_BASIC_INFORMATION pbi;
    PPEB_LDR_DATA ldr_data;

    iter->wow64 = FALSE;
    if (!set_ntstatus( NtQueryInformationProcess( process, ProcessBasicInformation,
                                                  &pbi, sizeof(pbi), NULL )))
        return FALSE;

    /* read address of LdrData from PEB */
    if (!ReadProcessMemory( process, &pbi.PebBaseAddress->LdrData, &ldr_data, sizeof(ldr_data), NULL ))
        return FALSE;

    /* This happens when running "old" wow64 configuration. Mark it as such. */
    if (!ldr_data)
    {
        SetLastError( ERROR_EMPTY );
        return FALSE;
    }
    /* read address of first module from LdrData */
    if (!ReadProcessMemory( process, &ldr_data->InLoadOrderModuleList.Flink,
                            &iter->current, sizeof(iter->current), NULL ))
        return FALSE;

    iter->head = &ldr_data->InLoadOrderModuleList;
    iter->process = process;
    return TRUE;
}


static int module_iterator_next( struct module_iterator *iter )
{
    if (iter->current == iter->head) return 0;

    if (is_win64 && iter->wow64)
    {
        LIST_ENTRY32 *entry32 = (LIST_ENTRY32 *)iter->current;

        if (!ReadProcessMemory( iter->process,
                                CONTAINING_RECORD(entry32, LDR_DATA_TABLE_ENTRY32, InLoadOrderModuleList),
                                &iter->ldr_module32, sizeof(iter->ldr_module32), NULL ))
            return -1;
        iter->current = (LIST_ENTRY *)(DWORD_PTR)iter->ldr_module32.InLoadOrderModuleList.Flink;
        return 1;
    }

    if (!ReadProcessMemory( iter->process,
                            CONTAINING_RECORD(iter->current, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks),
                            &iter->ldr_module, sizeof(iter->ldr_module), NULL ))
         return -1;

    iter->current = iter->ldr_module.InLoadOrderLinks.Flink;
    return 1;
}


static BOOL get_ldr_module( HANDLE process, HMODULE module, LDR_DATA_TABLE_ENTRY *ldr_module )
{
    struct module_iterator iter;
    INT ret;

    if (!init_module_iterator( &iter, process )) return FALSE;

    while ((ret = module_iterator_next( &iter )) > 0)
        /* When hModule is NULL we return the process image - which will be
         * the first module since our iterator uses InLoadOrderModuleList */
        if (!module || module == iter.ldr_module.DllBase)
        {
            *ldr_module = iter.ldr_module;
            return TRUE;
        }

    if (ret == 0) SetLastError( ERROR_INVALID_HANDLE );
    return FALSE;
}


static BOOL get_ldr_module32( HANDLE process, HMODULE module, LDR_DATA_TABLE_ENTRY32 *ldr_module )
{
    struct module_iterator iter;
    INT ret;

#ifdef _WIN64
    if ((ULONG_PTR)module >> 32)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
#endif
    if (!init_module_iterator_wow64( &iter, process )) return FALSE;

    while ((ret = module_iterator_next( &iter )) > 0)
        /* When hModule is NULL we return the process image - which will be
         * the first module since our iterator uses InLoadOrderModuleList */
        if (!module || (DWORD)(DWORD_PTR)module == iter.ldr_module32.BaseAddress)
        {
            *ldr_module = iter.ldr_module32;
            return TRUE;
        }

    if (ret == 0) SetLastError( ERROR_INVALID_HANDLE );
    return FALSE;
}


/***********************************************************************
 *         EmptyWorkingSet   (kernelbase.@)
 *         K32EmptyWorkingSet   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EmptyWorkingSet( HANDLE process )
{
    return SetProcessWorkingSetSizeEx( process, (SIZE_T)-1, (SIZE_T)-1, 0 );
}


/***********************************************************************
 *         EnumDeviceDrivers   (kernelbase.@)
 *         K32EnumDeviceDrivers   (kernelbase.@)
 */
BOOL WINAPI EnumDeviceDrivers( void **image_base, DWORD count, DWORD *needed )
{
    FIXME( "(%p, %ld, %p): stub\n", image_base, count, needed );
    if (needed) *needed = 0;
    return TRUE;
}


/***********************************************************************
 *         EnumPageFilesA   (kernelbase.@)
 *         K32EnumPageFilesA   (kernelbase.@)
 */
BOOL WINAPI /* DECLSPEC_HOTPATCH */ EnumPageFilesA( PENUM_PAGE_FILE_CALLBACKA callback, void *context )
{
    FIXME( "(%p, %p) stub\n", callback, context );
    return FALSE;
}


/***********************************************************************
 *         EnumPageFilesW   (kernelbase.@)
 *         K32EnumPageFilesW   (kernelbase.@)
 */
BOOL WINAPI /* DECLSPEC_HOTPATCH */ EnumPageFilesW( PENUM_PAGE_FILE_CALLBACKW callback, void *context )
{
    FIXME( "(%p, %p) stub\n", callback, context );
    return FALSE;
}


/***********************************************************************
 *         EnumProcessModules   (kernelbase.@)
 *         K32EnumProcessModules   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumProcessModules( HANDLE process, HMODULE *module,
                                                  DWORD count, DWORD *needed )
{
    return EnumProcessModulesEx( process, module, count, needed, LIST_MODULES_DEFAULT );
}


struct module_push
{
    HMODULE *module;
    unsigned count;
    unsigned size;
};

static void module_push( struct module_push *mp, HMODULE module )
{
    if (mp->count >= sizeof(HMODULE))
    {
        *mp->module++ = module;
        mp->count -= sizeof(HMODULE);
    }
    mp->size += sizeof(HMODULE);
}

static void module_push_iter( struct module_push *mp, struct module_iterator *iter )
{
    if (is_win64 && iter->wow64)
        module_push( mp, (HMODULE) (DWORD_PTR)iter->ldr_module32.BaseAddress );
    else
        module_push( mp, iter->ldr_module.DllBase );
}

static int module_push_all( struct module_push *mp, struct module_iterator *iter )
{
    int ret;

    while ((ret = module_iterator_next( iter )) > 0)
        module_push_iter( mp, iter );

    return ret;
}

/***********************************************************************
 *         EnumProcessModulesEx   (kernelbase.@)
 *         K32EnumProcessModulesEx   (kernelbase.@)
 */
BOOL WINAPI EnumProcessModulesEx( HANDLE process, HMODULE *module, DWORD count,
                                  DWORD *needed, DWORD filter )
{
    struct module_push mp = {module, count, 0};
    unsigned list_mode;
    BOOL target_wow64;
    INT ret = 0;

    TRACE( "(%p, %p, %ld, %p, %ld)\n", process, module, count, needed, filter );

    if (process != GetCurrentProcess())
    {
        if (!IsWow64Process( process, &target_wow64 )) return FALSE;
    }
    else target_wow64 = is_wow64;

    if (filter & ~LIST_MODULES_ALL)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    list_mode = filter & LIST_MODULES_ALL;
    /* Can't access 64bit process from (wow64) 32bit */
    if (is_wow64 && !target_wow64)
    {
        SetLastError( ERROR_PARTIAL_COPY );
        return FALSE;
    }
    if (count && !module)
    {
        SetLastError( ERROR_NOACCESS );
        return FALSE;
    }

    if (process == GetCurrentProcess())
    {
        if (!(is_win64 && list_mode == LIST_MODULES_32BIT))
        {
            PPEB_LDR_DATA ldr_data = NtCurrentTeb()->Peb->LdrData;
            PLIST_ENTRY head = &ldr_data->InLoadOrderModuleList;
            PLIST_ENTRY entry = head->Flink;

            while (entry != head)
            {
                LDR_DATA_TABLE_ENTRY *ldr = CONTAINING_RECORD( entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks );
                module_push( &mp, ldr->DllBase );
                entry = entry->Flink;
            }
        }
    }
    else
    {
        struct module_iterator iter;

        if (is_win64 && target_wow64 && (list_mode & LIST_MODULES_32BIT))
        {
            if (!init_module_iterator_wow64( &iter, process ) || module_push_all( &mp, &iter ) < 0)
                return FALSE;
        }
        if (!(is_win64 && list_mode == LIST_MODULES_32BIT))
        {
            if (init_module_iterator( &iter, process ))
            {
                if (is_win64 && target_wow64 && (list_mode & LIST_MODULES_64BIT))
                    /* Don't add main module twice in _ALL mode */
                    ret = module_iterator_next( &iter );
                if (ret >= 0) ret = module_push_all( &mp, &iter );
            }
            else if (GetLastError() == ERROR_EMPTY)
            {
                /* We're running on "old" wow configuration.
                 * Fallback to PEB32 to get at least main module if requested.
                 */
                if (list_mode == LIST_MODULES_DEFAULT)
                {
                    if (init_module_iterator_wow64( &iter, process ) && module_iterator_next( &iter ) > 0)
                        module_push_iter( &mp, &iter );
                    else
                        ret = -1;
                }
            }
            else
                return FALSE;
        }
    }

    if (!needed)
    {
        SetLastError( ERROR_NOACCESS );
        return FALSE;
    }
    *needed = mp.size;
    return ret == 0;
}


/***********************************************************************
 *         EnumProcesses   (kernelbase.@)
 *         K32EnumProcesses   (kernelbase.@)
 */
BOOL WINAPI EnumProcesses( DWORD *ids, DWORD count, DWORD *used )
{
    SYSTEM_PROCESS_INFORMATION *spi;
    ULONG size = 0x4000;
    void *buf = NULL;
    NTSTATUS status;

    do
    {
        size *= 2;
        HeapFree( GetProcessHeap(), 0, buf );
        if (!(buf = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;
        status = NtQuerySystemInformation( SystemProcessInformation, buf, size, NULL );
    } while (status == STATUS_INFO_LENGTH_MISMATCH);

    if (!set_ntstatus( status ))
    {
        HeapFree( GetProcessHeap(), 0, buf );
        return FALSE;
    }
    spi = buf;
    for (*used = 0; count >= sizeof(DWORD); count -= sizeof(DWORD))
    {
        *ids++ = HandleToUlong( spi->UniqueProcessId );
        *used += sizeof(DWORD);
        if (spi->NextEntryOffset == 0) break;
        spi = (SYSTEM_PROCESS_INFORMATION *)(((PCHAR)spi) + spi->NextEntryOffset);
    }
    HeapFree( GetProcessHeap(), 0, buf );
    return TRUE;
}


/***********************************************************************
 *         GetDeviceDriverBaseNameA   (kernelbase.@)
 *         K32GetDeviceDriverBaseNameA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetDeviceDriverBaseNameA( void *image_base, char *name, DWORD size )
{
    FIXME( "(%p, %p, %ld): stub\n", image_base, name, size );
    if (name && size) name[0] = 0;
    return 0;
}


/***********************************************************************
 *         GetDeviceDriverBaseNameW   (kernelbase.@)
 *         K32GetDeviceDriverBaseNameW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetDeviceDriverBaseNameW( void *image_base, WCHAR *name, DWORD size )
{
    FIXME( "(%p, %p, %ld): stub\n", image_base, name, size );
    if (name && size) name[0] = 0;
    return 0;
}


/***********************************************************************
 *         GetDeviceDriverFileNameA   (kernelbase.@)
 *         K32GetDeviceDriverFileNameA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetDeviceDriverFileNameA( void *image_base, char *name, DWORD size )
{
    FIXME( "(%p, %p, %ld): stub\n", image_base, name, size );
    if (name && size) name[0] = 0;
    return 0;
}


/***********************************************************************
 *         GetDeviceDriverFileNameW   (kernelbase.@)
 *         K32GetDeviceDriverFileNameW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetDeviceDriverFileNameW( void *image_base, WCHAR *name, DWORD size )
{
    FIXME( "(%p, %p, %ld): stub\n", image_base, name, size );
    if (name && size) name[0] = 0;
    return 0;
}


/***********************************************************************
 *         GetMappedFileNameA   (kernelbase.@)
 *         K32GetMappedFileNameA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetMappedFileNameA( HANDLE process, void *addr, char *name, DWORD size )
{
    WCHAR nameW[MAX_PATH];
    DWORD len;

    if (size && !name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!GetMappedFileNameW( process, addr, nameW, MAX_PATH )) return 0;
    if (!size)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    len = file_name_WtoA( nameW, wcslen(nameW), name, size );
    name[min(len, size - 1)] = 0;
    return len;
}


/***********************************************************************
 *         GetMappedFileNameW   (kernelbase.@)
 *         K32GetMappedFileNameW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetMappedFileNameW( HANDLE process, void *addr, WCHAR *name, DWORD size )
{
    ULONG_PTR buffer[(sizeof(MEMORY_SECTION_NAME) + MAX_PATH * sizeof(WCHAR)) / sizeof(ULONG_PTR)];
    MEMORY_SECTION_NAME *mem = (MEMORY_SECTION_NAME *)buffer;
    DWORD len;

    if (size && !name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!set_ntstatus( NtQueryVirtualMemory( process, addr, MemoryMappedFilenameInformation,
                                             mem, sizeof(buffer), NULL )))
        return 0;

    len = mem->SectionFileName.Length / sizeof(WCHAR);
    memcpy( name, mem->SectionFileName.Buffer, min( mem->SectionFileName.Length, size * sizeof(WCHAR) ));
    if (len >= size) SetLastError( ERROR_INSUFFICIENT_BUFFER );
    name[min(len, size - 1)] = 0;
    return len;
}


/***********************************************************************
 *         GetModuleBaseNameA   (kernelbase.@)
 *         K32GetModuleBaseNameA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetModuleBaseNameA( HANDLE process, HMODULE module,
                                                   char *name, DWORD size )
{
    WCHAR *name_w;
    DWORD len, ret = 0;

    if (!name || !size)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!(name_w = HeapAlloc( GetProcessHeap(), 0, sizeof(WCHAR) * size ))) return 0;

    len = GetModuleBaseNameW( process, module, name_w, size );
    TRACE( "%ld, %s\n", len, debugstr_w(name_w) );
    if (len)
    {
        ret = WideCharToMultiByte( CP_ACP, 0, name_w, len, name, size, NULL, NULL );
        if (ret < size) name[ret] = 0;
    }
    HeapFree( GetProcessHeap(), 0, name_w );
    return ret;
}


/***********************************************************************
 *         GetModuleBaseNameW   (kernelbase.@)
 *         K32GetModuleBaseNameW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetModuleBaseNameW( HANDLE process, HMODULE module,
                                                   WCHAR *name, DWORD size )
{
    BOOL wow64, found = FALSE;

    if (!IsWow64Process( process, &wow64 )) return 0;

    if (is_win64 && wow64)
    {
        LDR_DATA_TABLE_ENTRY32 ldr_module32;

        if (get_ldr_module32(process, module, &ldr_module32))
        {
            size = min( ldr_module32.BaseDllName.Length / sizeof(WCHAR), size );
            if (ReadProcessMemory( process, (void *)(DWORD_PTR)ldr_module32.BaseDllName.Buffer,
                                   name, size * sizeof(WCHAR), NULL ))
                found = TRUE;
        }
    }
    if (!found)
    {
        LDR_DATA_TABLE_ENTRY ldr_module;

        if (!get_ldr_module( process, module, &ldr_module )) return 0;
        size = min( ldr_module.BaseDllName.Length / sizeof(WCHAR), size );
        if (!ReadProcessMemory( process, ldr_module.BaseDllName.Buffer,
                                name, size * sizeof(WCHAR), NULL ))
            return 0;
    }
    name[size] = 0;
    return size;
}


/***********************************************************************
 *         GetModuleFileNameExA   (kernelbase.@)
 *         K32GetModuleFileNameExA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetModuleFileNameExA( HANDLE process, HMODULE module,
                                                     char *name, DWORD size )
{
    WCHAR *ptr;
    DWORD len;

    TRACE( "(process=%p, module=%p, %p, %ld)\n", process, module, name, size );

    if (!name || !size)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (process == GetCurrentProcess())
    {
        len = GetModuleFileNameA( module, name, size );
        name[size - 1] = '\0';
        return len;
    }

    if (!(ptr = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return 0;
    len = GetModuleFileNameExW( process, module, ptr, size );
    if (!len)
    {
        name[0] = 0;
    }
    else
    {
        if (!WideCharToMultiByte( CP_ACP, 0, ptr, -1, name, size, NULL, NULL ))
        {
            name[size - 1] = 0;
            len = size;
        }
        else if (len < size) len = strlen( name );
    }
    HeapFree( GetProcessHeap(), 0, ptr );
    return len;
}


/***********************************************************************
 *         GetModuleFileNameExW   (kernelbase.@)
 *         K32GetModuleFileNameExW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetModuleFileNameExW( HANDLE process, HMODULE module,
                                                     WCHAR *name, DWORD size )
{
    BOOL wow64, found = FALSE;
    DWORD len = 0;

    if (!size) return 0;

    if (!IsWow64Process( process, &wow64 )) return 0;

    if (is_win64 && wow64)
    {
        LDR_DATA_TABLE_ENTRY32 ldr_module32;

        if (get_ldr_module32( process, module, &ldr_module32 ))
        {
            len = ldr_module32.FullDllName.Length / sizeof(WCHAR);
            if (ReadProcessMemory( process, (void *)(DWORD_PTR)ldr_module32.FullDllName.Buffer,
                                   name, min( len, size ) * sizeof(WCHAR), NULL ))
                found = TRUE;
        }
    }
    if (!found)
    {
        LDR_DATA_TABLE_ENTRY ldr_module;

        if (!get_ldr_module(process, module, &ldr_module)) return 0;
        len = ldr_module.FullDllName.Length / sizeof(WCHAR);
        if (!ReadProcessMemory( process, ldr_module.FullDllName.Buffer,
                                name, min( len, size ) * sizeof(WCHAR), NULL ))
            return 0;
    }

    if (len < size)
    {
        name[len] = 0;
        return len;
    }
    else
    {
        name[size - 1] = 0;
        return size;
    }
}


/***********************************************************************
 *         GetModuleInformation   (kernelbase.@)
 *         K32GetModuleInformation   (kernelbase.@)
 */
BOOL WINAPI GetModuleInformation( HANDLE process, HMODULE module, MODULEINFO *modinfo, DWORD count )
{
    BOOL wow64, found = FALSE;

    if (count < sizeof(MODULEINFO))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    if (!IsWow64Process( process, &wow64 )) return FALSE;

    if (is_win64 && wow64)
    {
        LDR_DATA_TABLE_ENTRY32 ldr_module32;

        if (get_ldr_module32( process, module, &ldr_module32 ))
        {
            modinfo->lpBaseOfDll = (void *)(DWORD_PTR)ldr_module32.BaseAddress;
            modinfo->SizeOfImage = ldr_module32.SizeOfImage;
            modinfo->EntryPoint  = (void *)(DWORD_PTR)ldr_module32.EntryPoint;
            found = TRUE;
        }
    }
    if (!found)
    {
        LDR_DATA_TABLE_ENTRY ldr_module;

        if (!get_ldr_module( process, module, &ldr_module )) return FALSE;
        modinfo->lpBaseOfDll = ldr_module.DllBase;
        modinfo->SizeOfImage = ldr_module.SizeOfImage;
        modinfo->EntryPoint  = ldr_module.EntryPoint;
    }
    return TRUE;
}


/***********************************************************************
 *         GetPerformanceInfo   (kernelbase.@)
 *         K32GetPerformanceInfo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetPerformanceInfo( PPERFORMANCE_INFORMATION info, DWORD size )
{
    SYSTEM_PERFORMANCE_INFORMATION perf;
    SYSTEM_BASIC_INFORMATION basic;
    SYSTEM_PROCESS_INFORMATION *process, *spi;
    DWORD info_size;
    NTSTATUS status;

    TRACE( "(%p, %ld)\n", info, size );

    if (size < sizeof(*info))
    {
        SetLastError( ERROR_BAD_LENGTH );
        return FALSE;
    }

    status = NtQuerySystemInformation( SystemPerformanceInformation, &perf, sizeof(perf), NULL );
    if (!set_ntstatus( status )) return FALSE;
    status = NtQuerySystemInformation( SystemBasicInformation, &basic, sizeof(basic), NULL );
    if (!set_ntstatus( status )) return FALSE;

    info->cb                 = sizeof(*info);
    info->CommitTotal        = perf.TotalCommittedPages;
    info->CommitLimit        = perf.TotalCommitLimit;
    info->CommitPeak         = perf.PeakCommitment;
    info->PhysicalTotal      = basic.MmNumberOfPhysicalPages;
    info->PhysicalAvailable  = perf.AvailablePages;
    info->SystemCache        = 0;
    info->KernelTotal        = perf.PagedPoolUsage + perf.NonPagedPoolUsage;
    info->KernelPaged        = perf.PagedPoolUsage;
    info->KernelNonpaged     = perf.NonPagedPoolUsage;
    info->PageSize           = basic.PageSize;

    /* fields from SYSTEM_PROCESS_INFORMATION */
    NtQuerySystemInformation( SystemProcessInformation, NULL, 0, &info_size );
    for (;;)
    {
        process = HeapAlloc( GetProcessHeap(), 0, info_size );
        if (!process)
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }
        status = NtQuerySystemInformation( SystemProcessInformation, process, info_size, &info_size );
        if (!status) break;
        HeapFree( GetProcessHeap(), 0, process );
        if (status != STATUS_INFO_LENGTH_MISMATCH)
        {
            SetLastError( RtlNtStatusToDosError( status ) );
            return FALSE;
        }
    }
    info->HandleCount = info->ProcessCount = info->ThreadCount = 0;
    spi = process;
    for (;;)
    {
        info->ProcessCount++;
        info->HandleCount += spi->HandleCount;
        info->ThreadCount += spi->dwThreadCount;
        if (spi->NextEntryOffset == 0) break;
        spi = (SYSTEM_PROCESS_INFORMATION *)((char *)spi + spi->NextEntryOffset);
    }
    HeapFree( GetProcessHeap(), 0, process );
    return TRUE;
}


/***********************************************************************
 *         GetProcessImageFileNameA   (kernelbase.@)
 *         K32GetProcessImageFileNameA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetProcessImageFileNameA( HANDLE process, char *file, DWORD size )
{
    return QueryFullProcessImageNameA( process, PROCESS_NAME_NATIVE, file, &size ) ? size : 0;
}


/***********************************************************************
 *         GetProcessImageFileNameW   (kernelbase.@)
 *         K32GetProcessImageFileNameW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetProcessImageFileNameW( HANDLE process, WCHAR *file, DWORD size )
{
    return QueryFullProcessImageNameW( process, PROCESS_NAME_NATIVE, file, &size ) ? size : 0;
}


/***********************************************************************
 *         GetProcessMemoryInfo   (kernelbase.@)
 *         K32GetProcessMemoryInfo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetProcessMemoryInfo( HANDLE process, PROCESS_MEMORY_COUNTERS *pmc,
                                                    DWORD count )
{
    VM_COUNTERS vmc;

    if (count < sizeof(PROCESS_MEMORY_COUNTERS))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    if (!set_ntstatus( NtQueryInformationProcess( process, ProcessVmCounters, &vmc, sizeof(vmc), NULL )))
        return FALSE;

    pmc->cb = sizeof(PROCESS_MEMORY_COUNTERS);
    pmc->PageFaultCount = vmc.PageFaultCount;
    pmc->PeakWorkingSetSize = vmc.PeakWorkingSetSize;
    pmc->WorkingSetSize = vmc.WorkingSetSize;
    pmc->QuotaPeakPagedPoolUsage = vmc.QuotaPeakPagedPoolUsage;
    pmc->QuotaPagedPoolUsage = vmc.QuotaPagedPoolUsage;
    pmc->QuotaPeakNonPagedPoolUsage = vmc.QuotaPeakNonPagedPoolUsage;
    pmc->QuotaNonPagedPoolUsage = vmc.QuotaNonPagedPoolUsage;
    pmc->PagefileUsage = vmc.PagefileUsage;
    pmc->PeakPagefileUsage = vmc.PeakPagefileUsage;
    return TRUE;
}


/***********************************************************************
 *         GetWsChanges   (kernelbase.@)
 *         K32GetWsChanges   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetWsChanges( HANDLE process, PSAPI_WS_WATCH_INFORMATION *info, DWORD size )
{
    TRACE( "(%p, %p, %ld)\n", process, info, size );
    return set_ntstatus( NtQueryInformationProcess( process, ProcessWorkingSetWatch, info, size, NULL ));
}


/***********************************************************************
 *         GetWsChangesEx   (kernelbase.@)
 *         K32GetWsChangesEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetWsChangesEx( HANDLE process, PSAPI_WS_WATCH_INFORMATION_EX *info,
                                              DWORD *size )
{
    FIXME( "(%p, %p, %p)\n", process, info, size );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 *         InitializeProcessForWsWatch   (kernelbase.@)
 *         K32InitializeProcessForWsWatch   (kernelbase.@)
 */
BOOL WINAPI /* DECLSPEC_HOTPATCH */ InitializeProcessForWsWatch( HANDLE process )
{
    FIXME( "(process=%p): stub\n", process );
    return TRUE;
}


/***********************************************************************
 *         QueryWorkingSet   (kernelbase.@)
 *         K32QueryWorkingSet   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryWorkingSet( HANDLE process, void *buffer, DWORD size )
{
    TRACE( "(%p, %p, %ld)\n", process, buffer, size );
    return set_ntstatus( NtQueryVirtualMemory( process, NULL, MemoryWorkingSetInformation,
                                               buffer, size, NULL ));
}


/***********************************************************************
 *         QueryWorkingSetEx   (kernelbase.@)
 *         K32QueryWorkingSetEx   (kernelbase.@)
 */
BOOL WINAPI QueryWorkingSetEx( HANDLE process, void *buffer, DWORD size )
{
    TRACE( "(%p, %p, %ld)\n", process, buffer, size );
    return set_ntstatus( NtQueryVirtualMemory( process, NULL, MemoryWorkingSetExInformation,
                                               buffer, size, NULL ));
}


/******************************************************************
 *         QueryFullProcessImageNameA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryFullProcessImageNameA( HANDLE process, DWORD flags,
                                                          char *name, DWORD *size )
{
    BOOL ret;
    DWORD sizeW = *size;
    WCHAR *nameW = HeapAlloc( GetProcessHeap(), 0, *size * sizeof(WCHAR) );

    ret = QueryFullProcessImageNameW( process, flags, nameW, &sizeW );
    if (ret) ret = (WideCharToMultiByte( CP_ACP, 0, nameW, -1, name, *size, NULL, NULL) > 0);
    if (ret) *size = strlen( name );
    HeapFree( GetProcessHeap(), 0, nameW );
    return ret;
}


/******************************************************************
 *         QueryFullProcessImageNameW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryFullProcessImageNameW( HANDLE process, DWORD flags,
                                                          WCHAR *name, DWORD *size )
{
    BYTE buffer[sizeof(UNICODE_STRING) + MAX_PATH*sizeof(WCHAR)];  /* this buffer should be enough */
    UNICODE_STRING *dynamic_buffer = NULL;
    UNICODE_STRING *result = NULL;
    NTSTATUS status;
    DWORD needed;

    /* FIXME: Use ProcessImageFileName for the PROCESS_NAME_NATIVE case */
    status = NtQueryInformationProcess( process, ProcessImageFileNameWin32, buffer,
                                        sizeof(buffer) - sizeof(WCHAR), &needed );
    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        dynamic_buffer = HeapAlloc( GetProcessHeap(), 0, needed + sizeof(WCHAR) );
        status = NtQueryInformationProcess( process, ProcessImageFileNameWin32, dynamic_buffer,
                                            needed, &needed );
        result = dynamic_buffer;
    }
    else
        result = (UNICODE_STRING *)buffer;

    if (status) goto cleanup;

    if (flags & PROCESS_NAME_NATIVE && result->Length > 2 * sizeof(WCHAR))
    {
        WCHAR drive[3];
        WCHAR device[1024];
        DWORD ntlen, devlen;

        if (result->Buffer[1] != ':' || result->Buffer[0] < 'A' || result->Buffer[0] > 'Z')
        {
            /* We cannot convert it to an NT device path so fail */
            status = STATUS_NO_SUCH_DEVICE;
            goto cleanup;
        }

        /* Find this drive's NT device path */
        drive[0] = result->Buffer[0];
        drive[1] = ':';
        drive[2] = 0;
        if (!QueryDosDeviceW(drive, device, ARRAY_SIZE(device)))
        {
            status = STATUS_NO_SUCH_DEVICE;
            goto cleanup;
        }

        devlen = lstrlenW(device);
        ntlen = devlen + (result->Length/sizeof(WCHAR) - 2);
        if (ntlen + 1 > *size)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            goto cleanup;
        }
        *size = ntlen;

        memcpy( name, device, devlen * sizeof(*device) );
        memcpy( name + devlen, result->Buffer + 2, result->Length - 2 * sizeof(WCHAR) );
        name[*size] = 0;
        TRACE( "NT path: %s\n", debugstr_w(name) );
    }
    else
    {
        if (result->Length/sizeof(WCHAR) + 1 > *size)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            goto cleanup;
        }

        *size = result->Length/sizeof(WCHAR);
        memcpy( name, result->Buffer, result->Length );
        name[*size] = 0;
    }

cleanup:
    HeapFree( GetProcessHeap(), 0, dynamic_buffer );
    return set_ntstatus( status );
}
