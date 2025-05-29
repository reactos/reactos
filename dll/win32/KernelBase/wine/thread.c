/*
 * Win32 threads
 *
 * Copyright 1996, 2002, 2019 Alexandre Julliard
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
#include <string.h>
#include <limits.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"

#include "kernelbase.h"
#include "wine/exception.h"
#include "wine/asm.h"
#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(thread);


/***********************************************************************
 * Threads
 ***********************************************************************/


static DWORD rtlmode_to_win32mode( DWORD rtlmode )
{
    DWORD win32mode = 0;

    if (rtlmode & 0x10) win32mode |= SEM_FAILCRITICALERRORS;
    if (rtlmode & 0x20) win32mode |= SEM_NOGPFAULTERRORBOX;
    if (rtlmode & 0x40) win32mode |= SEM_NOOPENFILEERRORBOX;
    return win32mode;
}


/***************************************************************************
 *           CreateRemoteThread   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateRemoteThread( HANDLE process, SECURITY_ATTRIBUTES *sa, SIZE_T stack,
                                                    LPTHREAD_START_ROUTINE start, LPVOID param,
                                                    DWORD flags, DWORD *id )
{
    return CreateRemoteThreadEx( process, sa, stack, start, param, flags, NULL, id );
}


/***************************************************************************
 *           CreateRemoteThreadEx   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateRemoteThreadEx( HANDLE process, SECURITY_ATTRIBUTES *sa,
                                                      SIZE_T stack, LPTHREAD_START_ROUTINE start,
                                                      LPVOID param, DWORD flags,
                                                      LPPROC_THREAD_ATTRIBUTE_LIST attributes, DWORD *id )
{
    HANDLE handle;
    CLIENT_ID client_id;
    SIZE_T stack_reserve = 0, stack_commit = 0;

    if (attributes) FIXME("thread attributes ignored\n");

    if (flags & STACK_SIZE_PARAM_IS_A_RESERVATION) stack_reserve = stack;
    else stack_commit = stack;

    if (!set_ntstatus( RtlCreateUserThread( process, sa ? sa->lpSecurityDescriptor : NULL, TRUE,
                                            0, stack_reserve, stack_commit,
                                            (PRTL_THREAD_START_ROUTINE)start, param, &handle, &client_id )))
        return 0;

    if (id) *id = HandleToULong( client_id.UniqueThread );
    if (sa && sa->nLength >= sizeof(*sa) && sa->bInheritHandle)
        SetHandleInformation( handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT );
    if (!(flags & CREATE_SUSPENDED))
    {
        ULONG ret;
        if (NtResumeThread( handle, &ret ))
        {
            NtClose( handle );
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            handle = 0;
        }
    }
    return handle;
}


/***********************************************************************
 *           CreateThread   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateThread( SECURITY_ATTRIBUTES *sa, SIZE_T stack,
                                              LPTHREAD_START_ROUTINE start, LPVOID param,
                                              DWORD flags, LPDWORD id )
{
     return CreateRemoteThread( GetCurrentProcess(), sa, stack, start, param, flags, id );
}


/***********************************************************************
 *           FreeLibraryAndExitThread   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH FreeLibraryAndExitThread( HINSTANCE module, DWORD exit_code )
{
    FreeLibrary( module );
    RtlExitUserThread( exit_code );
}


/***********************************************************************
 *	     GetCurrentThreadStackLimits   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH GetCurrentThreadStackLimits( ULONG_PTR *low, ULONG_PTR *high )
{
    *low = (ULONG_PTR)NtCurrentTeb()->DeallocationStack;
    *high = (ULONG_PTR)NtCurrentTeb()->Tib.StackBase;
}


/***********************************************************************
 *           GetCurrentThread   (kernelbase.@)
 */
HANDLE WINAPI kernelbase_GetCurrentThread(void)
{
    return (HANDLE)~(ULONG_PTR)1;
}


/***********************************************************************
 *           GetCurrentThreadId   (kernelbase.@)
 */
DWORD WINAPI kernelbase_GetCurrentThreadId(void)
{
    return HandleToULong( NtCurrentTeb()->ClientId.UniqueThread );
}


/**********************************************************************
 *           GetExitCodeThread   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetExitCodeThread( HANDLE thread, LPDWORD exit_code )
{
    THREAD_BASIC_INFORMATION info;
    NTSTATUS status = NtQueryInformationThread( thread, ThreadBasicInformation,
                                                &info, sizeof(info), NULL );
    if (!status && exit_code) *exit_code = info.ExitStatus;
    return set_ntstatus( status );
}


/**********************************************************************
 *           GetLastError   (kernelbase.@)
 */
DWORD WINAPI kernelbase_GetLastError(void)
{
    return NtCurrentTeb()->LastErrorValue;
}


/**********************************************************************
 *           GetProcessIdOfThread   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetProcessIdOfThread( HANDLE thread )
{
    THREAD_BASIC_INFORMATION tbi;

    if (!set_ntstatus( NtQueryInformationThread( thread, ThreadBasicInformation, &tbi, sizeof(tbi), NULL)))
        return 0;
    return HandleToULong( tbi.ClientId.UniqueProcess );
}


/***********************************************************************
 *           GetThreadContext   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetThreadContext( HANDLE thread, CONTEXT *context )
{
    return set_ntstatus( NtGetContextThread( thread, context ));
}


/***********************************************************************
 *           GetThreadErrorMode   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetThreadErrorMode(void)
{
    return rtlmode_to_win32mode( RtlGetThreadErrorMode() );
}


/***********************************************************************
 *           GetThreadGroupAffinity   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetThreadGroupAffinity( HANDLE thread, GROUP_AFFINITY *affinity )
{
    if (!affinity)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return set_ntstatus( NtQueryInformationThread( thread, ThreadGroupInformation,
                                                   affinity, sizeof(*affinity), NULL ));
}


/***********************************************************************
 *	     GetThreadIOPendingFlag   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetThreadIOPendingFlag( HANDLE thread, PBOOL pending )
{
    return set_ntstatus( NtQueryInformationThread( thread, ThreadIsIoPending,
                                                   pending, sizeof(*pending), NULL ));
}


/**********************************************************************
 *           GetThreadId   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetThreadId( HANDLE thread )
{
    THREAD_BASIC_INFORMATION tbi;

    if (!set_ntstatus( NtQueryInformationThread( thread, ThreadBasicInformation, &tbi, sizeof(tbi), NULL)))
        return 0;
    return HandleToULong( tbi.ClientId.UniqueThread );
}


/***********************************************************************
 *           GetThreadIdealProcessorEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetThreadIdealProcessorEx( HANDLE thread, PROCESSOR_NUMBER *ideal )
{
    return set_ntstatus( NtQueryInformationThread( thread, ThreadIdealProcessorEx, ideal, sizeof(*ideal), NULL));
}


/***********************************************************************
 *	GetThreadLocale   (kernelbase.@)
 */
LCID WINAPI /* DECLSPEC_HOTPATCH */ GetThreadLocale(void)
{
    LCID ret = NtCurrentTeb()->CurrentLocale;
    if (!ret) NtCurrentTeb()->CurrentLocale = ret = GetUserDefaultLCID();
    return ret;
}


/**********************************************************************
 *           GetThreadPriority   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetThreadPriority( HANDLE thread )
{
    THREAD_BASIC_INFORMATION info;

    if (!set_ntstatus( NtQueryInformationThread( thread, ThreadBasicInformation,
                                                 &info, sizeof(info), NULL )))
        return THREAD_PRIORITY_ERROR_RETURN;
    return info.Priority;
}


/**********************************************************************
 *           GetThreadPriorityBoost   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetThreadPriorityBoost( HANDLE thread, BOOL *state )
{
    return set_ntstatus( NtQueryInformationThread( thread, ThreadPriorityBoost, state, sizeof(*state), NULL ));
}


/**********************************************************************
 *           GetThreadTimes   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetThreadTimes( HANDLE thread, LPFILETIME creationtime, LPFILETIME exittime,
                                              LPFILETIME kerneltime, LPFILETIME usertime )
{
    KERNEL_USER_TIMES times;

    if (!set_ntstatus( NtQueryInformationThread( thread, ThreadTimes, &times, sizeof(times), NULL )))
        return FALSE;

    if (creationtime)
    {
        creationtime->dwLowDateTime = times.CreateTime.u.LowPart;
        creationtime->dwHighDateTime = times.CreateTime.u.HighPart;
    }
    if (exittime)
    {
        exittime->dwLowDateTime = times.ExitTime.u.LowPart;
        exittime->dwHighDateTime = times.ExitTime.u.HighPart;
    }
    if (kerneltime)
    {
        kerneltime->dwLowDateTime = times.KernelTime.u.LowPart;
        kerneltime->dwHighDateTime = times.KernelTime.u.HighPart;
    }
    if (usertime)
    {
        usertime->dwLowDateTime = times.UserTime.u.LowPart;
        usertime->dwHighDateTime = times.UserTime.u.HighPart;
    }
    return TRUE;
}


/***********************************************************************
 *	     GetThreadUILanguage   (kernelbase.@)
 */
LANGID WINAPI DECLSPEC_HOTPATCH GetThreadUILanguage(void)
{
    LANGID lang;

    FIXME(": stub, returning default language.\n");
    NtQueryDefaultUILanguage( &lang );
    return lang;
}


/***********************************************************************
 *	     OpenThread   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenThread( DWORD access, BOOL inherit, DWORD id )
{
    HANDLE handle;
    OBJECT_ATTRIBUTES attr;
    CLIENT_ID cid;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = inherit ? OBJ_INHERIT : 0;
    attr.ObjectName = NULL;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    cid.UniqueProcess = 0;
    cid.UniqueThread = ULongToHandle( id );

    if (!set_ntstatus( NtOpenThread( &handle, access, &attr, &cid ))) handle = 0;
    return handle;
}


/* callback for QueueUserAPC */
static void CALLBACK call_user_apc( ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3 )
{
    PAPCFUNC func = (PAPCFUNC)arg1;
    func( arg2 );
}

/***********************************************************************
 *	     QueueUserAPC   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH QueueUserAPC( PAPCFUNC func, HANDLE thread, ULONG_PTR data )
{
    return set_ntstatus( NtQueueApcThread( thread, call_user_apc, (ULONG_PTR)func, data, 0 ));
}


/***********************************************************************
 *           QueryThreadCycleTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryThreadCycleTime( HANDLE thread, ULONG64 *cycle )
{
    static int once;
    if (!once++) FIXME( "(%p,%p): stub!\n", thread, cycle );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/**********************************************************************
 *           ResumeThread   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH ResumeThread( HANDLE thread )
{
    DWORD ret;

    if (!set_ntstatus( NtResumeThread( thread, &ret ))) ret = ~0U;
    return ret;
}


/***********************************************************************
 *           SetThreadContext   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetThreadContext( HANDLE thread, const CONTEXT *context )
{
    return set_ntstatus( NtSetContextThread( thread, context ));
}


/***********************************************************************
 *           SetThreadDescription   (kernelbase.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH SetThreadDescription( HANDLE thread, PCWSTR description )
{
    THREAD_NAME_INFORMATION info;
    int length;

    TRACE( "(%p, %s)\n", thread, debugstr_w( description ));

    length = description ? lstrlenW( description ) * sizeof(WCHAR) : 0;

    if (length > USHRT_MAX)
        return HRESULT_FROM_NT(STATUS_INVALID_PARAMETER);

    info.ThreadName.Length = info.ThreadName.MaximumLength = length;
    info.ThreadName.Buffer = (WCHAR *)description;

    return HRESULT_FROM_NT(NtSetInformationThread( thread, ThreadNameInformation, &info, sizeof(info) ));
}

/***********************************************************************
 *           GetThreadDescription   (kernelbase.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH GetThreadDescription( HANDLE thread, WCHAR **description )
{
    THREAD_NAME_INFORMATION *info;
    NTSTATUS status;
    ULONG length;

    TRACE( "(%p, %p)\n", thread, description );

    *description = NULL;

    length = 0;
    status = NtQueryInformationThread( thread, ThreadNameInformation, NULL, 0, &length );
    if (status != STATUS_BUFFER_TOO_SMALL)
        return HRESULT_FROM_NT(status);

    if (!(info = heap_alloc( length )))
        return HRESULT_FROM_NT(STATUS_NO_MEMORY);

    status = NtQueryInformationThread( thread, ThreadNameInformation, info, length, &length );
    if (!status)
    {
        if (!(*description = LocalAlloc( 0, info->ThreadName.Length + sizeof(WCHAR))))
            status = STATUS_NO_MEMORY;
        else
        {
            if (info->ThreadName.Length)
                memcpy(*description, info->ThreadName.Buffer, info->ThreadName.Length);
            (*description)[info->ThreadName.Length / sizeof(WCHAR)] = 0;
        }
    }

    heap_free(info);

    return HRESULT_FROM_NT(status);
}

/***********************************************************************
 *           SetThreadErrorMode   (kernelbase.@)
 */
BOOL WINAPI SetThreadErrorMode( DWORD mode, DWORD *old )
{
    NTSTATUS status;
    DWORD new = 0;

    if (mode & ~(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (mode & SEM_FAILCRITICALERRORS) new |= 0x10;
    if (mode & SEM_NOGPFAULTERRORBOX) new |= 0x20;
    if (mode & SEM_NOOPENFILEERRORBOX) new |= 0x40;

    status = RtlSetThreadErrorMode( new, old );
    if (!status && old) *old = rtlmode_to_win32mode( *old );
    return set_ntstatus( status );
}


/***********************************************************************
 *           SetThreadGroupAffinity   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetThreadGroupAffinity( HANDLE thread, const GROUP_AFFINITY *new,
                                                      GROUP_AFFINITY *old )
{
    if (old && !GetThreadGroupAffinity( thread, old )) return FALSE;
    return set_ntstatus( NtSetInformationThread( thread, ThreadGroupInformation, new, sizeof(*new) ));
}


/**********************************************************************
 *           SetThreadIdealProcessor   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH SetThreadIdealProcessor( HANDLE thread, DWORD proc )
{
    NTSTATUS status;

    status = NtSetInformationThread( thread, ThreadIdealProcessor, &proc, sizeof(proc) );
    if (NT_SUCCESS(status)) return status;

    SetLastError( RtlNtStatusToDosError( status ));
    return ~0u;
}


/***********************************************************************
 *           SetThreadIdealProcessorEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetThreadIdealProcessorEx( HANDLE thread, PROCESSOR_NUMBER *ideal,
                                                         PROCESSOR_NUMBER *previous )
{
    FIXME( "(%p %p %p): stub\n", thread, ideal, previous );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/**********************************************************************
 *	SetThreadLocale   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetThreadLocale( LCID lcid )
{
    lcid = ConvertDefaultLocale( lcid );
    if (lcid != GetThreadLocale())
    {
        if (!IsValidLocale( lcid, LCID_SUPPORTED ))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        NtCurrentTeb()->CurrentLocale = lcid;
    }
    return TRUE;
}


/**********************************************************************
 *           SetThreadPriority   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetThreadPriority( HANDLE thread, INT priority )
{
    DWORD prio = priority;
    return set_ntstatus( NtSetInformationThread( thread, ThreadBasePriority, &prio, sizeof(prio) ));
}


/**********************************************************************
 *           SetThreadPriorityBoost   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetThreadPriorityBoost( HANDLE thread, BOOL disable )
{
    return set_ntstatus( NtSetInformationThread( thread, ThreadPriorityBoost, &disable, sizeof(disable) ));
}


/**********************************************************************
 *           SetThreadStackGuarantee   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetThreadStackGuarantee( ULONG *size )
{
    ULONG prev_size = NtCurrentTeb()->GuaranteedStackBytes;
    ULONG new_size = (*size + 4095) & ~4095;

    /* at least 2 pages on 64-bit */
    if (sizeof(void *) > sizeof(int) && new_size) new_size = max( new_size, 8192 );

    *size = prev_size;
    if (new_size >= (char *)NtCurrentTeb()->Tib.StackBase - (char *)NtCurrentTeb()->DeallocationStack)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (new_size > prev_size) NtCurrentTeb()->GuaranteedStackBytes = (new_size + 4095) & ~4095;
    return TRUE;
}


/**********************************************************************
 *	SetThreadUILanguage   (kernelbase.@)
 */
LANGID WINAPI DECLSPEC_HOTPATCH SetThreadUILanguage( LANGID langid )
{
    TRACE( "(0x%04x) stub - returning success\n", langid );

    if (!langid) langid = GetThreadUILanguage();
    return langid;
}


/**********************************************************************
 *            SetThreadInformation   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetThreadInformation( HANDLE thread, THREAD_INFORMATION_CLASS info_class,
        VOID *info, DWORD size )
{
    switch (info_class)
    {
        case ThreadMemoryPriority:
            return set_ntstatus( NtSetInformationThread( thread, ThreadPagePriority, info, size ));
        case ThreadPowerThrottling:
            return set_ntstatus( NtSetInformationThread( thread, ThreadPowerThrottlingState, info, size ));
        default:
            FIXME("Unsupported class %u.\n", info_class);
            return FALSE;
    }
}


/**********************************************************************
 *           SuspendThread   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH SuspendThread( HANDLE thread )
{
    DWORD ret;

    if (!set_ntstatus( NtSuspendThread( thread, &ret ))) ret = ~0U;
    return ret;
}


/***********************************************************************
 *           SwitchToThread   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SwitchToThread(void)
{
    return (NtYieldExecution() != STATUS_NO_YIELD_PERFORMED);
}


/**********************************************************************
 *           TerminateThread   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH TerminateThread( HANDLE handle, DWORD exit_code )
{
    return set_ntstatus( NtTerminateThread( handle, exit_code ));
}


/**********************************************************************
 *           TlsAlloc   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH TlsAlloc(void)
{
    DWORD index;
    PEB * const peb = NtCurrentTeb()->Peb;

    RtlAcquirePebLock();
    index = RtlFindClearBitsAndSet( peb->TlsBitmap, 1, 1 );
    if (index != ~0U) NtCurrentTeb()->TlsSlots[index] = 0; /* clear the value */
    else
    {
        index = RtlFindClearBitsAndSet( peb->TlsExpansionBitmap, 1, 0 );
        if (index != ~0U)
        {
            if (!NtCurrentTeb()->TlsExpansionSlots &&
                !(NtCurrentTeb()->TlsExpansionSlots = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                         8 * sizeof(peb->TlsExpansionBitmapBits) * sizeof(void*) )))
            {
                RtlClearBits( peb->TlsExpansionBitmap, index, 1 );
                index = ~0U;
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            }
            else
            {
                NtCurrentTeb()->TlsExpansionSlots[index] = 0; /* clear the value */
                index += TLS_MINIMUM_AVAILABLE;
            }
        }
        else SetLastError( ERROR_NO_MORE_ITEMS );
    }
    RtlReleasePebLock();
    return index;
}


/**********************************************************************
 *           TlsFree   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH TlsFree( DWORD index )
{
    BOOL ret;

    RtlAcquirePebLock();
    if (index >= TLS_MINIMUM_AVAILABLE)
    {
        ret = RtlAreBitsSet( NtCurrentTeb()->Peb->TlsExpansionBitmap, index - TLS_MINIMUM_AVAILABLE, 1 );
        if (ret) RtlClearBits( NtCurrentTeb()->Peb->TlsExpansionBitmap, index - TLS_MINIMUM_AVAILABLE, 1 );
    }
    else
    {
        ret = RtlAreBitsSet( NtCurrentTeb()->Peb->TlsBitmap, index, 1 );
        if (ret) RtlClearBits( NtCurrentTeb()->Peb->TlsBitmap, index, 1 );
    }
    if (ret) NtSetInformationThread( GetCurrentThread(), ThreadZeroTlsCell, &index, sizeof(index) );
    else SetLastError( ERROR_INVALID_PARAMETER );
    RtlReleasePebLock();
    return ret;
}


/**********************************************************************
 *           TlsGetValue   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH TlsGetValue( DWORD index )
{
    SetLastError( ERROR_SUCCESS );
    if (index < TLS_MINIMUM_AVAILABLE) return NtCurrentTeb()->TlsSlots[index];

    index -= TLS_MINIMUM_AVAILABLE;
    if (index >= 8 * sizeof(NtCurrentTeb()->Peb->TlsExpansionBitmapBits))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    if (!NtCurrentTeb()->TlsExpansionSlots) return NULL;
    return NtCurrentTeb()->TlsExpansionSlots[index];
}


/**********************************************************************
 *           TlsSetValue   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH TlsSetValue( DWORD index, LPVOID value )
{
    if (index < TLS_MINIMUM_AVAILABLE)
    {
        NtCurrentTeb()->TlsSlots[index] = value;
    }
    else
    {
        index -= TLS_MINIMUM_AVAILABLE;
        if (index >= 8 * sizeof(NtCurrentTeb()->Peb->TlsExpansionBitmapBits))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        if (!NtCurrentTeb()->TlsExpansionSlots &&
            !(NtCurrentTeb()->TlsExpansionSlots = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                         8 * sizeof(NtCurrentTeb()->Peb->TlsExpansionBitmapBits) * sizeof(void*) )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }
        NtCurrentTeb()->TlsExpansionSlots[index] = value;
    }
    return TRUE;
}


/***********************************************************************
 *           Wow64GetThreadContext   (kernelbase.@)
 */
BOOL WINAPI Wow64GetThreadContext( HANDLE handle, WOW64_CONTEXT *context)
{
#ifdef __i386__
    return set_ntstatus( NtGetContextThread( handle, (CONTEXT *)context ));
#elif defined(__x86_64__)
    return set_ntstatus( RtlWow64GetThreadContext( handle, context ));
#else
    return set_ntstatus( STATUS_NOT_IMPLEMENTED );
#endif
}


/***********************************************************************
 *           Wow64SetThreadContext   (kernelbase.@)
 */
BOOL WINAPI Wow64SetThreadContext( HANDLE handle, const WOW64_CONTEXT *context)
{
#ifdef __i386__
    return set_ntstatus( NtSetContextThread( handle, (const CONTEXT *)context ));
#elif defined(__x86_64__)
    return set_ntstatus( RtlWow64SetThreadContext( handle, context ));
#else
    return set_ntstatus( STATUS_NOT_IMPLEMENTED );
#endif
}


/***********************************************************************
 * Fibers
 ***********************************************************************/


struct fiber_actctx
{
    ACTIVATION_CONTEXT_STACK stack_space;    /* activation context stack space */
    ACTIVATION_CONTEXT_STACK *stack_ptr;     /* last value of ActivationContextStackPointer */
};

struct fiber_data
{
    LPVOID                param;             /* 00/00 fiber param */
    void                 *except;            /* 04/08 saved exception handlers list */
    void                 *stack_base;        /* 08/10 top of fiber stack */
    void                 *stack_limit;       /* 0c/18 fiber stack low-water mark */
    void                 *stack_allocation;  /* 10/20 base of the fiber stack allocation */
    CONTEXT               context;           /* 14/30 fiber context */
    DWORD                 flags;             /*       fiber flags */
    LPFIBER_START_ROUTINE start;             /*       start routine */
    void                 *fls_slots;         /*       fiber storage slots */
    struct fiber_actctx   actctx;            /*       activation context state */
};

#ifdef __i386__
extern void WINAPI switch_fiber( CONTEXT *old, CONTEXT *new );
__ASM_STDCALL_FUNC( switch_fiber, 8,
                    "movl 4(%esp),%ecx\n\t"     /* old */
                    "movl %edi,0x9c(%ecx)\n\t"  /* old->Edi */
                    "movl %esi,0xa0(%ecx)\n\t"  /* old->Esi */
                    "movl %ebx,0xa4(%ecx)\n\t"  /* old->Ebx */
                    "movl %ebp,0xb4(%ecx)\n\t"  /* old->Ebp */
                    "movl 0(%esp),%eax\n\t"
                    "movl %eax,0xb8(%ecx)\n\t"  /* old->Eip */
                    "leal 12(%esp),%eax\n\t"
                    "movl %eax,0xc4(%ecx)\n\t"  /* old->Esp */
                    "movl 8(%esp),%ecx\n\t"     /* new */
                    "movl 0x9c(%ecx),%edi\n\t"  /* new->Edi */
                    "movl 0xa0(%ecx),%esi\n\t"  /* new->Esi */
                    "movl 0xa4(%ecx),%ebx\n\t"  /* new->Ebx */
                    "movl 0xb4(%ecx),%ebp\n\t"  /* new->Ebp */
                    "movl 0xc4(%ecx),%esp\n\t"  /* new->Esp */
                    "jmp *0xb8(%ecx)" )         /* new->Eip */
#elif defined(__arm64ec__)
static void __attribute__((naked)) WINAPI switch_fiber( CONTEXT *old, CONTEXT *new )
{
    asm( "mov x2, sp\n\t"
         "stp x27, x2,  [x0, #0x90]\n\t"  /* old->Rbx,Rsp */
         "str x29,      [x0, #0xa0]\n\t"  /* old->Rbp */
         "stp x25, x26, [x0, #0xa8]\n\t"  /* old->Rsi,Rdi */
         "stp x19, x20, [x0, #0xd8]\n\t"  /* old->R12,R13 */
         "stp x21, x22, [x0, #0xe8]\n\t"  /* old->R14,R15 */
         "str x30,      [x0, #0xf8]\n\t"  /* old->Rip */
         "stp q8,  q9,  [x0, #0x220]\n\t" /* old->Xmm8,Xmm9 */
         "stp q10, q11, [x0, #0x240]\n\t" /* old->Xmm10,Xmm11 */
         "stp q12, q13, [x0, #0x260]\n\t" /* old->Xmm12,Xmm13 */
         "stp q14, q15, [x0, #0x280]\n\t" /* old->Xmm14,Xmm15 */
         /* FIXME: MxCsr */
         "ldp x27, x2,  [x1, #0x90]\n\t"  /* old->Rbx,Rsp */
         "ldr x29,      [x1, #0xa0]\n\t"  /* old->Rbp */
         "ldp x25, x26, [x1, #0xa8]\n\t"  /* old->Rsi,Rdi */
         "ldp x19, x20, [x1, #0xd8]\n\t"  /* old->R12,R13 */
         "ldp x21, x22, [x1, #0xe8]\n\t"  /* old->R14,R15 */
         "ldr x30,      [x1, #0xf8]\n\t"  /* old->Rip */
         "ldp q8,  q9,  [x1, #0x220]\n\t" /* old->Xmm8,Xmm9 */
         "ldp q10, q11, [x1, #0x240]\n\t" /* old->Xmm10,Xmm11 */
         "ldp q12, q13, [x1, #0x260]\n\t" /* old->Xmm12,Xmm13 */
         "ldp q14, q15, [x1, #0x280]\n\t" /* old->Xmm14,Xmm15 */
         "mov sp, x2\n\t"
         "ret" );
}
#elif defined(__x86_64__)
extern void WINAPI switch_fiber( CONTEXT *old, CONTEXT *new );
__ASM_GLOBAL_FUNC( switch_fiber,
                    "movq %rbx,0x90(%rcx)\n\t"       /* old->Rbx */
                    "leaq 0x8(%rsp),%rax\n\t"
                    "movq %rax,0x98(%rcx)\n\t"       /* old->Rsp */
                    "movq %rbp,0xa0(%rcx)\n\t"       /* old->Rbp */
                    "movq %rsi,0xa8(%rcx)\n\t"       /* old->Rsi */
                    "movq %rdi,0xb0(%rcx)\n\t"       /* old->Rdi */
                    "movq %r12,0xd8(%rcx)\n\t"       /* old->R12 */
                    "movq %r13,0xe0(%rcx)\n\t"       /* old->R13 */
                    "movq %r14,0xe8(%rcx)\n\t"       /* old->R14 */
                    "movq %r15,0xf0(%rcx)\n\t"       /* old->R15 */
                    "movq (%rsp),%rax\n\t"
                    "movq %rax,0xf8(%rcx)\n\t"       /* old->Rip */
                    "movdqa %xmm6,0x200(%rcx)\n\t"   /* old->Xmm6 */
                    "movdqa %xmm7,0x210(%rcx)\n\t"   /* old->Xmm7 */
                    "movdqa %xmm8,0x220(%rcx)\n\t"   /* old->Xmm8 */
                    "movdqa %xmm9,0x230(%rcx)\n\t"   /* old->Xmm9 */
                    "movdqa %xmm10,0x240(%rcx)\n\t"  /* old->Xmm10 */
                    "movdqa %xmm11,0x250(%rcx)\n\t"  /* old->Xmm11 */
                    "movdqa %xmm12,0x260(%rcx)\n\t"  /* old->Xmm12 */
                    "movdqa %xmm13,0x270(%rcx)\n\t"  /* old->Xmm13 */
                    "movdqa %xmm14,0x280(%rcx)\n\t"  /* old->Xmm14 */
                    "movdqa %xmm15,0x290(%rcx)\n\t"  /* old->Xmm15 */
                    "movq 0x90(%rdx),%rbx\n\t"       /* new->Rbx */
                    "movq 0xa0(%rdx),%rbp\n\t"       /* new->Rbp */
                    "movq 0xa8(%rdx),%rsi\n\t"       /* new->Rsi */
                    "movq 0xb0(%rdx),%rdi\n\t"       /* new->Rdi */
                    "movq 0xd8(%rdx),%r12\n\t"       /* new->R12 */
                    "movq 0xe0(%rdx),%r13\n\t"       /* new->R13 */
                    "movq 0xe8(%rdx),%r14\n\t"       /* new->R14 */
                    "movq 0xf0(%rdx),%r15\n\t"       /* new->R15 */
                    "movdqa 0x200(%rdx),%xmm6\n\t"   /* new->Xmm6 */
                    "movdqa 0x210(%rdx),%xmm7\n\t"   /* new->Xmm7 */
                    "movdqa 0x220(%rdx),%xmm8\n\t"   /* new->Xmm8 */
                    "movdqa 0x230(%rdx),%xmm9\n\t"   /* new->Xmm9 */
                    "movdqa 0x240(%rdx),%xmm10\n\t"  /* new->Xmm10 */
                    "movdqa 0x250(%rdx),%xmm11\n\t"  /* new->Xmm11 */
                    "movdqa 0x260(%rdx),%xmm12\n\t"  /* new->Xmm12 */
                    "movdqa 0x270(%rdx),%xmm13\n\t"  /* new->Xmm13 */
                    "movdqa 0x280(%rdx),%xmm14\n\t"  /* new->Xmm14 */
                    "movdqa 0x290(%rdx),%xmm15\n\t"  /* new->Xmm15 */
                    "movq 0x98(%rdx),%rsp\n\t"       /* new->Rsp */
                    "jmp *0xf8(%rdx)" )              /* new->Rip */
#elif defined(__arm__)
extern void WINAPI switch_fiber( CONTEXT *old, CONTEXT *new );
__ASM_GLOBAL_FUNC( switch_fiber,
                   "str r4, [r0, #0x14]\n\t"   /* old->R4 */
                   "str r5, [r0, #0x18]\n\t"   /* old->R5 */
                   "str r6, [r0, #0x1c]\n\t"   /* old->R6 */
                   "str r7, [r0, #0x20]\n\t"   /* old->R7 */
                   "str r8, [r0, #0x24]\n\t"   /* old->R8 */
                   "str r9, [r0, #0x28]\n\t"   /* old->R9 */
                   "str r10, [r0, #0x2c]\n\t"  /* old->R10 */
                   "str r11, [r0, #0x30]\n\t"  /* old->R11 */
                   "str sp, [r0, #0x38]\n\t"   /* old->Sp */
                   "str lr, [r0, #0x40]\n\t"   /* old->Pc */
                   "ldr r4, [r1, #0x14]\n\t"   /* new->R4 */
                   "ldr r5, [r1, #0x18]\n\t"   /* new->R5 */
                   "ldr r6, [r1, #0x1c]\n\t"   /* new->R6 */
                   "ldr r7, [r1, #0x20]\n\t"   /* new->R7 */
                   "ldr r8, [r1, #0x24]\n\t"   /* new->R8 */
                   "ldr r9, [r1, #0x28]\n\t"   /* new->R9 */
                   "ldr r10, [r1, #0x2c]\n\t"  /* new->R10 */
                   "ldr r11, [r1, #0x30]\n\t"  /* new->R11 */
                   "ldr sp, [r1, #0x38]\n\t"   /* new->Sp */
                   "ldr r2, [r1, #0x40]\n\t"   /* new->Pc */
                   "bx r2" )
#elif defined(__aarch64__)
extern void WINAPI switch_fiber( CONTEXT *old, CONTEXT *new );
__ASM_GLOBAL_FUNC( switch_fiber,
                   "stp x19, x20, [x0, #0xa0]\n\t"  /* old->X19,X20 */
                   "stp x21, x22, [x0, #0xb0]\n\t"  /* old->X21,X22 */
                   "stp x23, x24, [x0, #0xc0]\n\t"  /* old->X23,X24 */
                   "stp x25, x26, [x0, #0xd0]\n\t"  /* old->X25,X26 */
                   "stp x27, x28, [x0, #0xe0]\n\t"  /* old->X27,X28 */
                   "str x29, [x0, #0xf0]\n\t"       /* old->Fp */
                   "mov x2, sp\n\t"
                   "str x2, [x0, #0x100]\n\t"       /* old->Sp */
                   "str x30, [x0, #0x108]\n\t"      /* old->Pc */
                   "ldp x19, x20, [x1, #0xa0]\n\t"  /* new->X19,X20 */
                   "ldp x21, x22, [x1, #0xb0]\n\t"  /* new->X21,X22 */
                   "ldp x23, x24, [x1, #0xc0]\n\t"  /* new->X23,X24 */
                   "ldp x25, x26, [x1, #0xd0]\n\t"  /* new->X25,X26 */
                   "ldp x27, x28, [x1, #0xe0]\n\t"  /* new->X27,X28 */
                   "ldr x29, [x1, #0xf0]\n\t"       /* new->Fp */
                   "ldr x2, [x1, #0x100]\n\t"       /* new->Sp */
                   "ldr x30, [x1, #0x108]\n\t"      /* new->Pc */
                   "mov sp, x2\n\t"
                   "ret" )
#else
static void WINAPI switch_fiber( CONTEXT *old, CONTEXT *new )
{
    FIXME( "not implemented\n" );
    DbgBreakPoint();
}
#endif

/* call the fiber initial function once we have switched stack */
static void CDECL start_fiber(void)
{
    struct fiber_data *fiber = NtCurrentTeb()->Tib.FiberData;
    LPFIBER_START_ROUTINE start = fiber->start;

    __TRY
    {
        start( fiber->param );
        RtlExitUserThread( 1 );
    }
    __EXCEPT(UnhandledExceptionFilter)
    {
        TerminateThread( GetCurrentThread(), GetExceptionCode() );
    }
    __ENDTRY
}

static void init_fiber_context( struct fiber_data *fiber )
{
#ifdef __i386__
    fiber->context.Esp = (ULONG_PTR)fiber->stack_base - 4;
    fiber->context.Eip = (ULONG_PTR)start_fiber;
#elif defined(__arm64ec__)
    fiber->context.Rsp = (ULONG_PTR)fiber->stack_base;
    fiber->context.Rip = (ULONG_PTR)start_fiber;
#elif defined(__x86_64__)
    fiber->context.Rsp = (ULONG_PTR)fiber->stack_base - 0x28;
    fiber->context.Rip = (ULONG_PTR)start_fiber;
#elif defined(__arm__)
    fiber->context.Sp = (ULONG_PTR)fiber->stack_base;
    fiber->context.Pc = (ULONG_PTR)start_fiber;
#elif defined(__aarch64__)
    fiber->context.Sp = (ULONG_PTR)fiber->stack_base;
    fiber->context.Pc = (ULONG_PTR)start_fiber;
#endif
}

static void move_list( LIST_ENTRY *dest, LIST_ENTRY *src )
{
    LIST_ENTRY *head = src->Flink;
    LIST_ENTRY *tail = src->Blink;

    if (src != head)
    {
        dest->Flink = head;
        dest->Blink = tail;
        head->Blink = dest;
        tail->Flink = dest;
    }
    else InitializeListHead( dest );
}

static void relocate_thread_actctx_stack( ACTIVATION_CONTEXT_STACK *dest )
{
    ACTIVATION_CONTEXT_STACK *src = NtCurrentTeb()->ActivationContextStackPointer;

    C_ASSERT(sizeof(*dest) == sizeof(dest->ActiveFrame) + sizeof(dest->FrameListCache) +
                              sizeof(dest->Flags) + sizeof(dest->NextCookieSequenceNumber) +
                              sizeof(dest->StackId));

    dest->ActiveFrame = src->ActiveFrame;
    move_list( &dest->FrameListCache, &src->FrameListCache );
    dest->Flags = src->Flags;
    dest->NextCookieSequenceNumber = src->NextCookieSequenceNumber;
    dest->StackId = src->StackId;

    NtCurrentTeb()->ActivationContextStackPointer = dest;
}


/***********************************************************************
 *           CreateFiber   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH CreateFiber( SIZE_T stack, LPFIBER_START_ROUTINE start, LPVOID param )
{
    return CreateFiberEx( stack, 0, 0, start, param );
}


/***********************************************************************
 *           CreateFiberEx   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH CreateFiberEx( SIZE_T stack_commit, SIZE_T stack_reserve, DWORD flags,
                                               LPFIBER_START_ROUTINE start, LPVOID param )
{
    struct fiber_data *fiber;
    INITIAL_TEB stack;

    if (!(fiber = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*fiber) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }

    if (!set_ntstatus( RtlCreateUserStack( stack_commit, stack_reserve, 0, 1, 1, &stack )))
    {
        HeapFree( GetProcessHeap(), 0, fiber );
        return NULL;
    }

    fiber->stack_allocation = stack.DeallocationStack;
    fiber->stack_base = stack.StackBase;
    fiber->stack_limit = stack.StackLimit;
    fiber->param       = param;
    fiber->except      = (void *)-1;
    fiber->start       = start;
    fiber->flags       = flags;
    InitializeListHead( &fiber->actctx.stack_space.FrameListCache );
    fiber->actctx.stack_ptr = &fiber->actctx.stack_space;
    init_fiber_context( fiber );
    return fiber;
}


/***********************************************************************
 *           ConvertFiberToThread   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ConvertFiberToThread(void)
{
    struct fiber_data *fiber = NtCurrentTeb()->Tib.FiberData;

    if (fiber)
    {
        relocate_thread_actctx_stack( &NtCurrentTeb()->ActivationContextStack );
        NtCurrentTeb()->Tib.FiberData = NULL;
        HeapFree( GetProcessHeap(), 0, fiber );
    }
    return TRUE;
}


/***********************************************************************
 *           ConvertThreadToFiber   (kernelbase.@)
 */
LPVOID WINAPI /* DECLSPEC_HOTPATCH */ ConvertThreadToFiber( LPVOID param )
{
    return ConvertThreadToFiberEx( param, 0 );
}


/***********************************************************************
 *           ConvertThreadToFiberEx   (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH ConvertThreadToFiberEx( LPVOID param, DWORD flags )
{
    struct fiber_data *fiber;

    if (NtCurrentTeb()->Tib.FiberData)
    {
        SetLastError( ERROR_ALREADY_FIBER );
        return NULL;
    }

    if (!(fiber = HeapAlloc( GetProcessHeap(), 0, sizeof(*fiber) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }
    fiber->param            = param;
    fiber->except           = NtCurrentTeb()->Tib.ExceptionList;
    fiber->stack_base       = NtCurrentTeb()->Tib.StackBase;
    fiber->stack_limit      = NtCurrentTeb()->Tib.StackLimit;
    fiber->stack_allocation = NtCurrentTeb()->DeallocationStack;
    fiber->start            = NULL;
    fiber->flags            = flags;
    fiber->fls_slots        = NtCurrentTeb()->FlsSlots;
    relocate_thread_actctx_stack( &fiber->actctx.stack_space );
    NtCurrentTeb()->Tib.FiberData = fiber;
    return fiber;
}


/***********************************************************************
 *           DeleteFiber   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH DeleteFiber( LPVOID fiber_ptr )
{
    struct fiber_data *fiber = fiber_ptr;

    if (!fiber) return;
    if (fiber == NtCurrentTeb()->Tib.FiberData)
    {
        relocate_thread_actctx_stack( &NtCurrentTeb()->ActivationContextStack );
        HeapFree( GetProcessHeap(), 0, fiber );
        RtlExitUserThread( 1 );
    }
    RtlFreeUserStack( fiber->stack_allocation );
    RtlProcessFlsData( fiber->fls_slots, 3 );
    RtlFreeActivationContextStack( &fiber->actctx.stack_space );
    HeapFree( GetProcessHeap(), 0, fiber );
}


/***********************************************************************
 *           IsThreadAFiber   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsThreadAFiber(void)
{
    return NtCurrentTeb()->Tib.FiberData != NULL;
}


/***********************************************************************
 *           SwitchToFiber   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH SwitchToFiber( LPVOID fiber )
{
    struct fiber_data *new_fiber = fiber;
    struct fiber_data *current_fiber = NtCurrentTeb()->Tib.FiberData;

    current_fiber->except      = NtCurrentTeb()->Tib.ExceptionList;
    current_fiber->stack_limit = NtCurrentTeb()->Tib.StackLimit;
    current_fiber->fls_slots   = NtCurrentTeb()->FlsSlots;
    current_fiber->actctx.stack_ptr = NtCurrentTeb()->ActivationContextStackPointer;
    /* stack_allocation and stack_base never change */

    /* FIXME: should save floating point context if requested in fiber->flags */
    NtCurrentTeb()->Tib.FiberData     = new_fiber;
    NtCurrentTeb()->Tib.ExceptionList = new_fiber->except;
    NtCurrentTeb()->Tib.StackBase     = new_fiber->stack_base;
    NtCurrentTeb()->Tib.StackLimit    = new_fiber->stack_limit;
    NtCurrentTeb()->DeallocationStack = new_fiber->stack_allocation;
    NtCurrentTeb()->FlsSlots          = new_fiber->fls_slots;
    NtCurrentTeb()->ActivationContextStackPointer = new_fiber->actctx.stack_ptr;
    switch_fiber( &current_fiber->context, &new_fiber->context );
}


/***********************************************************************
 *           FlsAlloc   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH FlsAlloc( PFLS_CALLBACK_FUNCTION callback )
{
    DWORD index;

    if (!set_ntstatus( RtlFlsAlloc( callback, &index ))) return FLS_OUT_OF_INDEXES;
    return index;
}


/***********************************************************************
 *           FlsFree   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FlsFree( DWORD index )
{
    return set_ntstatus( RtlFlsFree( index ));
}


/***********************************************************************
 *           FlsGetValue   (kernelbase.@)
 */
PVOID WINAPI DECLSPEC_HOTPATCH FlsGetValue( DWORD index )
{
    void *data;

    if (!set_ntstatus( RtlFlsGetValue( index, &data ))) return NULL;
    SetLastError( ERROR_SUCCESS );
    return data;
}


/***********************************************************************
 *           FlsSetValue   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FlsSetValue( DWORD index, PVOID data )
{
    return set_ntstatus( RtlFlsSetValue( index, data ));
}


/***********************************************************************
 * Thread pool
 ***********************************************************************/


/***********************************************************************
 *           CallbackMayRunLong   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CallbackMayRunLong( TP_CALLBACK_INSTANCE *instance )
{
    return set_ntstatus( TpCallbackMayRunLong( instance ));
}


/***********************************************************************
 *           CreateThreadpool   (kernelbase.@)
 */
PTP_POOL WINAPI DECLSPEC_HOTPATCH CreateThreadpool( void *reserved )
{
    TP_POOL *pool;

    if (!set_ntstatus( TpAllocPool( &pool, reserved ))) pool = NULL;
    return pool;
}


/***********************************************************************
 *           CreateThreadpoolCleanupGroup   (kernelbase.@)
 */
PTP_CLEANUP_GROUP WINAPI DECLSPEC_HOTPATCH CreateThreadpoolCleanupGroup(void)
{
    TP_CLEANUP_GROUP *group;

    if (!set_ntstatus( TpAllocCleanupGroup( &group ))) return NULL;
    return group;
}


static void WINAPI tp_io_callback( TP_CALLBACK_INSTANCE *instance, void *userdata, void *cvalue, IO_STATUS_BLOCK *iosb, TP_IO *io )
{
    PTP_WIN32_IO_CALLBACK callback = *(void **)io;
    callback( instance, userdata, cvalue, RtlNtStatusToDosError( iosb->Status ), iosb->Information, io );
}


/***********************************************************************
 *           CreateThreadpoolIo   (kernelbase.@)
 */
PTP_IO WINAPI DECLSPEC_HOTPATCH CreateThreadpoolIo( HANDLE handle, PTP_WIN32_IO_CALLBACK callback,
                                                    PVOID userdata, TP_CALLBACK_ENVIRON *environment )
{
    TP_IO *io;
    if (!set_ntstatus( TpAllocIoCompletion( &io, handle, tp_io_callback, userdata, environment ))) return NULL;
    *(void **)io = callback; /* ntdll leaves us space to store our callback at the beginning of TP_IO struct */
    return io;
}


/***********************************************************************
 *           CreateThreadpoolTimer   (kernelbase.@)
 */
PTP_TIMER WINAPI DECLSPEC_HOTPATCH CreateThreadpoolTimer( PTP_TIMER_CALLBACK callback, PVOID userdata,
                                                          TP_CALLBACK_ENVIRON *environment )
{
    TP_TIMER *timer;

    if (!set_ntstatus( TpAllocTimer( &timer, callback, userdata, environment ))) return NULL;
    return timer;
}


/***********************************************************************
 *           CreateThreadpoolWait   (kernelbase.@)
 */
PTP_WAIT WINAPI DECLSPEC_HOTPATCH CreateThreadpoolWait( PTP_WAIT_CALLBACK callback, PVOID userdata,
                                                       TP_CALLBACK_ENVIRON *environment )
{
    TP_WAIT *wait;

    if (!set_ntstatus( TpAllocWait( &wait, callback, userdata, environment ))) return NULL;
    return wait;
}


/***********************************************************************
 *           CreateThreadpoolWork   (kernelbase.@)
 */
PTP_WORK WINAPI DECLSPEC_HOTPATCH CreateThreadpoolWork( PTP_WORK_CALLBACK callback, PVOID userdata,
                                                        TP_CALLBACK_ENVIRON *environment )
{
    TP_WORK *work;

    if (!set_ntstatus( TpAllocWork( &work, callback, userdata, environment ))) return NULL;
    return work;
}


/***********************************************************************
 *           TrySubmitThreadpoolCallback   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH TrySubmitThreadpoolCallback( PTP_SIMPLE_CALLBACK callback, PVOID userdata,
                                                           TP_CALLBACK_ENVIRON *environment )
{
    return set_ntstatus( TpSimpleTryPost( callback, userdata, environment ));
}


/***********************************************************************
 *           QueueUserWorkItem   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueueUserWorkItem( LPTHREAD_START_ROUTINE func, PVOID context, ULONG flags )
{
    return set_ntstatus( RtlQueueWorkItem( func, context, flags ));
}

/***********************************************************************
 *           SetThreadpoolStackInformation   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetThreadpoolStackInformation( PTP_POOL pool, PTP_POOL_STACK_INFORMATION stack_info )
{
    return set_ntstatus( TpSetPoolStackInformation( pool, stack_info ));
}

/***********************************************************************
 *           QueryThreadpoolStackInformation   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryThreadpoolStackInformation( PTP_POOL pool, PTP_POOL_STACK_INFORMATION stack_info )
{
    return set_ntstatus( TpQueryPoolStackInformation( pool, stack_info ));
}
