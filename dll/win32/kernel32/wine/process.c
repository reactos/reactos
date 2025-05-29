/*
 * Win32 processes
 *
 * Copyright 1996, 1998 Alexandre Julliard
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

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#ifndef __REACTOS__
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "winbase.h"
#include "winnls.h"
#include "wincon.h"
#include "kernel_private.h"
#include "psapi.h"
#include "ddk/wdm.h"
#include "wine/asm.h"
#include "wine/debug.h"
#else

#include <k32.h>

#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(profile);


static inline BOOL set_ntstatus( NTSTATUS status )
{
    if (status) SetLastError( RtlNtStatusToDosError( status ));
    return !status;
}
BOOL WINAPI DECLSPEC_HOTPATCH GetLogicalProcessorInformationEx( LOGICAL_PROCESSOR_RELATIONSHIP relationship,
                                            SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buffer, DWORD *len );


typedef enum _RTL_UMS_SCHEDULER_REASON
{
    UmsSchedulerStartup,
    UmsSchedulerThreadBlocked,
    UmsSchedulerThreadYield,
} RTL_UMS_SCHEDULER_REASON, *PRTL_UMS_SCHEDULER_REASON;

typedef void (CALLBACK *PRTL_UMS_SCHEDULER_ENTRY_POINT)(RTL_UMS_SCHEDULER_REASON,ULONG_PTR,PVOID);

typedef void *PUMS_CONTEXT;
typedef void *PUMS_COMPLETION_LIST;
typedef PRTL_UMS_SCHEDULER_ENTRY_POINT PUMS_SCHEDULER_ENTRY_POINT;
typedef struct _UMS_SCHEDULER_STARTUP_INFO
{
    ULONG UmsVersion;
    PUMS_COMPLETION_LIST CompletionList;
    PUMS_SCHEDULER_ENTRY_POINT SchedulerProc;
    PVOID SchedulerParam;
} UMS_SCHEDULER_STARTUP_INFO, *PUMS_SCHEDULER_STARTUP_INFO;

typedef enum _RTL_UMS_THREAD_INFO_CLASS
{
    UmsThreadInvalidInfoClass,
    UmsThreadUserContext,
    UmsThreadPriority,
    UmsThreadAffinity,
    UmsThreadTeb,
    UmsThreadIsSuspended,
    UmsThreadIsTerminated,
    UmsThreadMaxInfoClass
} RTL_UMS_THREAD_INFO_CLASS, *PRTL_UMS_THREAD_INFO_CLASS;
typedef enum _RTL_UMS_SCHEDULER_REASON UMS_SCHEDULER_REASON;
typedef enum _RTL_UMS_THREAD_INFO_CLASS UMS_THREAD_INFO_CLASS, *PUMS_THREAD_INFO_CLASS;


#endif


static const struct _KUSER_SHARED_DATA *user_shared_data = (struct _KUSER_SHARED_DATA *)0x7ffe0000;

SYSTEM_BASIC_INFORMATION system_info = { 0 };

/* Process flags */
#define PDB32_DEBUGGED      0x0001  /* Process is being debugged */
#define PDB32_WIN16_PROC    0x0008  /* Win16 process */
#define PDB32_DOS_PROC      0x0010  /* Dos process */
#define PDB32_CONSOLE_PROC  0x0020  /* Console process */
#define PDB32_FILE_APIS_OEM 0x0040  /* File APIs are OEM */
#define PDB32_WIN32S_PROC   0x8000  /* Win32s process */

#ifndef __REACTOS__
/***********************************************************************
 *           wait_input_idle
 *
 * Wrapper to call WaitForInputIdle USER function
 */
typedef DWORD (WINAPI *WaitForInputIdle_ptr)( HANDLE hProcess, DWORD dwTimeOut );

static DWORD wait_input_idle( HANDLE process, DWORD timeout )
{
    HMODULE mod = GetModuleHandleA( "user32.dll" );
    if (mod)
    {
        WaitForInputIdle_ptr ptr = (WaitForInputIdle_ptr)GetProcAddress( mod, "WaitForInputIdle" );
        if (ptr) return ptr( process, timeout );
    }
    return 0;
}


/***********************************************************************
 *           WinExec   (KERNEL32.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH WinExec( LPCSTR lpCmdLine, UINT nCmdShow )
{
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    char *cmdline;
    UINT ret;

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = nCmdShow;

    /* cmdline needs to be writable for CreateProcess */
    if (!(cmdline = HeapAlloc( GetProcessHeap(), 0, strlen(lpCmdLine)+1 ))) return 0;
    strcpy( cmdline, lpCmdLine );

    if (CreateProcessA( NULL, cmdline, NULL, NULL, FALSE,
                        0, NULL, NULL, &startup, &info ))
    {
        /* Give 30 seconds to the app to come up */
        if (wait_input_idle( info.hProcess, 30000 ) == WAIT_FAILED)
            WARN("WaitForInputIdle failed: Error %ld\n", GetLastError() );
        ret = 33;
        /* Close off the handles */
        CloseHandle( info.hThread );
        CloseHandle( info.hProcess );
    }
    else if ((ret = GetLastError()) >= 32)
    {
        FIXME("Strange error set by CreateProcess: %d\n", ret );
        ret = 11;
    }
    HeapFree( GetProcessHeap(), 0, cmdline );
    return ret;
}


/**********************************************************************
 *	    LoadModule    (KERNEL32.@)
 */
DWORD WINAPI LoadModule( LPCSTR name, LPVOID paramBlock )
{
    LOADPARMS32 *params = paramBlock;
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    DWORD ret;
    LPSTR cmdline, p;
    char filename[MAX_PATH];
    BYTE len;

    if (!name) return ERROR_FILE_NOT_FOUND;

    if (!SearchPathA( NULL, name, ".exe", sizeof(filename), filename, NULL ) &&
        !SearchPathA( NULL, name, NULL, sizeof(filename), filename, NULL ))
        return GetLastError();

    len = (BYTE)params->lpCmdLine[0];
    if (!(cmdline = HeapAlloc( GetProcessHeap(), 0, strlen(filename) + len + 2 )))
        return ERROR_NOT_ENOUGH_MEMORY;

    strcpy( cmdline, filename );
    p = cmdline + strlen(cmdline);
    *p++ = ' ';
    memcpy( p, params->lpCmdLine + 1, len );
    p[len] = 0;

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);
    if (params->lpCmdShow)
    {
        startup.dwFlags = STARTF_USESHOWWINDOW;
        startup.wShowWindow = ((WORD *)params->lpCmdShow)[1];
    }

    if (CreateProcessA( filename, cmdline, NULL, NULL, FALSE, 0,
                        params->lpEnvAddress, NULL, &startup, &info ))
    {
        /* Give 30 seconds to the app to come up */
        if (wait_input_idle( info.hProcess, 30000 ) == WAIT_FAILED)
            WARN("WaitForInputIdle failed: Error %ld\n", GetLastError() );
        ret = 33;
        /* Close off the handles */
        CloseHandle( info.hThread );
        CloseHandle( info.hProcess );
    }
    else if ((ret = GetLastError()) >= 32)
    {
        FIXME("Strange error set by CreateProcess: %lu\n", ret );
        ret = 11;
    }

    HeapFree( GetProcessHeap(), 0, cmdline );
    return ret;
}


/***********************************************************************
 *           ExitProcess   (KERNEL32.@)
 *
 * Exits the current process.
 *
 * PARAMS
 *  status [I] Status code to exit with.
 *
 * RETURNS
 *  Nothing.
 */
#ifdef __i386__
__ASM_STDCALL_FUNC( ExitProcess, 4, /* Shrinker depend on this particular ExitProcess implementation */
                   "pushl %ebp\n\t"
                   ".byte 0x8B, 0xEC\n\t" /* movl %esp, %ebp */
                   ".byte 0x6A, 0x00\n\t" /* pushl $0 */
                   ".byte 0x68, 0x00, 0x00, 0x00, 0x00\n\t" /* pushl $0 - 4 bytes immediate */
                   "pushl 8(%ebp)\n\t"
                   "call " __ASM_STDCALL("RtlExitUserProcess",4) "\n\t"
                   "leave\n\t"
                   "ret $4" )
#else

void WINAPI ExitProcess( DWORD status )
{
    RtlExitUserProcess( status );
}

#endif

/***********************************************************************
 * GetExitCodeProcess           [KERNEL32.@]
 *
 * Gets termination status of specified process.
 *
 * PARAMS
 *   hProcess   [in]  Handle to the process.
 *   lpExitCode [out] Address to receive termination status.
 *
 * RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 */
BOOL WINAPI GetExitCodeProcess( HANDLE hProcess, LPDWORD lpExitCode )
{
    PROCESS_BASIC_INFORMATION pbi;

    if (!set_ntstatus( NtQueryInformationProcess( hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), NULL )))
        return FALSE;
    if (lpExitCode) *lpExitCode = pbi.ExitStatus;
    return TRUE;
}


/**************************************************************************
 *           FatalExit   (KERNEL32.@)
 */
void WINAPI FatalExit( int code )
{
    WARN( "FatalExit\n" );
    ExitProcess( code );
}


/***********************************************************************
 *           GetProcessFlags    (KERNEL32.@)
 */
DWORD WINAPI GetProcessFlags( DWORD processid )
{
    IMAGE_NT_HEADERS *nt;
    DWORD flags = 0;

    if (processid && processid != GetCurrentProcessId()) return 0;

    if ((nt = RtlImageNtHeader( NtCurrentTeb()->Peb->ImageBaseAddress )))
    {
        if (nt->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
            flags |= PDB32_CONSOLE_PROC;
    }
    if (!AreFileApisANSI()) flags |= PDB32_FILE_APIS_OEM;
    if (IsDebuggerPresent()) flags |= PDB32_DEBUGGED;
    return flags;
}


/***********************************************************************
 *           ConvertToGlobalHandle  (KERNEL32.@)
 */
HANDLE WINAPI ConvertToGlobalHandle(HANDLE hSrc)
{
    HANDLE ret = INVALID_HANDLE_VALUE;
    DuplicateHandle( GetCurrentProcess(), hSrc, GetCurrentProcess(), &ret, 0, FALSE,
                     DUPLICATE_MAKE_GLOBAL | DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE );
    return ret;
}


/***********************************************************************
 *           SetHandleContext   (KERNEL32.@)
 */
BOOL WINAPI SetHandleContext(HANDLE hnd,DWORD context)
{
    FIXME("(%p,%ld), stub. In case this got called by WSOCK32/WS2_32: "
          "the external WINSOCK DLLs won't work with WINE, don't use them.\n",hnd,context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *           GetHandleContext   (KERNEL32.@)
 */
DWORD WINAPI GetHandleContext(HANDLE hnd)
{
    FIXME("(%p), stub. In case this got called by WSOCK32/WS2_32: "
          "the external WINSOCK DLLs won't work with WINE, don't use them.\n",hnd);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/***********************************************************************
 *           CreateSocketHandle   (KERNEL32.@)
 */
HANDLE WINAPI CreateSocketHandle(void)
{
    FIXME("(), stub. In case this got called by WSOCK32/WS2_32: "
          "the external WINSOCK DLLs won't work with WINE, don't use them.\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return INVALID_HANDLE_VALUE;
}


/***********************************************************************
 *          SetProcessAffinityMask   (KERNEL32.@)
 */
BOOL WINAPI SetProcessAffinityMask( HANDLE hProcess, DWORD_PTR affmask )
{
    return set_ntstatus( NtSetInformationProcess( hProcess, ProcessAffinityMask,
                                                  &affmask, sizeof(DWORD_PTR) ));
}


/**********************************************************************
 *          GetProcessAffinityMask    (KERNEL32.@)
 */
BOOL WINAPI GetProcessAffinityMask( HANDLE hProcess, PDWORD_PTR process_mask, PDWORD_PTR system_mask )
{
    if (process_mask)
    {
        if (!set_ntstatus( NtQueryInformationProcess( hProcess, ProcessAffinityMask,
                                                      process_mask, sizeof(*process_mask), NULL )))
            return FALSE;
    }
    if (system_mask)
    {
        SYSTEM_BASIC_INFORMATION info;

        if (!set_ntstatus( NtQuerySystemInformation( SystemBasicInformation, &info, sizeof(info), NULL )))
            return FALSE;
        *system_mask = info.ActiveProcessorsAffinityMask;
    }
    return TRUE;
}


/***********************************************************************
 *		SetProcessWorkingSetSize	[KERNEL32.@]
 * Sets the min/max working set sizes for a specified process.
 *
 * PARAMS
 *    process  [I] Handle to the process of interest
 *    minset   [I] Specifies minimum working set size
 *    maxset   [I] Specifies maximum working set size
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI SetProcessWorkingSetSize(HANDLE process, SIZE_T minset, SIZE_T maxset)
{
    return SetProcessWorkingSetSizeEx(process, minset, maxset, 0);
}

/***********************************************************************
 *           GetProcessWorkingSetSize    (KERNEL32.@)
 */
BOOL WINAPI GetProcessWorkingSetSize(HANDLE process, SIZE_T *minset, SIZE_T *maxset)
{
    return GetProcessWorkingSetSizeEx(process, minset, maxset, NULL);
}


/******************************************************************
 *		GetProcessIoCounters (KERNEL32.@)
 */
BOOL WINAPI GetProcessIoCounters(HANDLE hProcess, PIO_COUNTERS ioc)
{
    return set_ntstatus( NtQueryInformationProcess(hProcess, ProcessIoCounters, ioc, sizeof(*ioc), NULL ));
}

/***********************************************************************
 *		RegisterServiceProcess (KERNEL32.@)
 *
 * A service process calls this function to ensure that it continues to run
 * even after a user logged off.
 */
DWORD WINAPI RegisterServiceProcess(DWORD dwProcessId, DWORD dwType)
{
    /* I don't think that Wine needs to do anything in this function */
    return 1; /* success */
}


/***********************************************************************
 *           GetCurrentProcess   (KERNEL32.@)
 *
 * Get a handle to the current process.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  A handle representing the current process.
 */
HANDLE WINAPI KERNEL32_GetCurrentProcess(void)
{
    return (HANDLE)~(ULONG_PTR)0;
}


/***********************************************************************
 *           CreateActCtxA   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateActCtxA( const ACTCTXA *actctx )
{
    ACTCTXW actw;
    SIZE_T len;
    HANDLE ret = INVALID_HANDLE_VALUE;
    LPWSTR src = NULL, assdir = NULL, resname = NULL, appname = NULL;

    TRACE("%p %08lx\n", actctx, actctx ? actctx->dwFlags : 0);

#define CHECK_LIMIT( field ) (actctx->cbSize >= RTL_SIZEOF_THROUGH_FIELD( ACTCTXA, field ))
    if (!actctx || !CHECK_LIMIT( lpSource ) ||
        ((actctx->dwFlags & ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID) && !CHECK_LIMIT( wProcessorArchitecture )) ||
        ((actctx->dwFlags & ACTCTX_FLAG_LANGID_VALID) && !CHECK_LIMIT( wLangId )) ||
        ((actctx->dwFlags & ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID) && !CHECK_LIMIT( lpAssemblyDirectory )) ||
        ((actctx->dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID) && !CHECK_LIMIT( lpResourceName )) ||
        ((actctx->dwFlags & ACTCTX_FLAG_APPLICATION_NAME_VALID) && !CHECK_LIMIT( lpApplicationName )) ||
        ((actctx->dwFlags & ACTCTX_FLAG_HMODULE_VALID) && !CHECK_LIMIT( hModule )))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
#undef CHECK_LIMIT

    actw.cbSize = sizeof(actw);
    actw.dwFlags = actctx->dwFlags;
    if (actctx->lpSource)
    {
        len = MultiByteToWideChar(CP_ACP, 0, actctx->lpSource, -1, NULL, 0);
        src = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!src) return INVALID_HANDLE_VALUE;
        MultiByteToWideChar(CP_ACP, 0, actctx->lpSource, -1, src, len);
    }
    actw.lpSource = src;

    if (actw.dwFlags & ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID)
        actw.wProcessorArchitecture = actctx->wProcessorArchitecture;
    if (actw.dwFlags & ACTCTX_FLAG_LANGID_VALID)
        actw.wLangId = actctx->wLangId;
    if (actw.dwFlags & ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID)
    {
        len = MultiByteToWideChar(CP_ACP, 0, actctx->lpAssemblyDirectory, -1, NULL, 0);
        assdir = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!assdir) goto done;
        MultiByteToWideChar(CP_ACP, 0, actctx->lpAssemblyDirectory, -1, assdir, len);
        actw.lpAssemblyDirectory = assdir;
    }
    if (actw.dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID)
    {
        if ((ULONG_PTR)actctx->lpResourceName >> 16)
        {
            len = MultiByteToWideChar(CP_ACP, 0, actctx->lpResourceName, -1, NULL, 0);
            resname = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            if (!resname) goto done;
            MultiByteToWideChar(CP_ACP, 0, actctx->lpResourceName, -1, resname, len);
            actw.lpResourceName = resname;
        }
        else actw.lpResourceName = (LPCWSTR)actctx->lpResourceName;
    }
    if (actw.dwFlags & ACTCTX_FLAG_APPLICATION_NAME_VALID)
    {
        len = MultiByteToWideChar(CP_ACP, 0, actctx->lpApplicationName, -1, NULL, 0);
        appname = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!appname) goto done;
        MultiByteToWideChar(CP_ACP, 0, actctx->lpApplicationName, -1, appname, len);
        actw.lpApplicationName = appname;
    }
    if (actw.dwFlags & ACTCTX_FLAG_HMODULE_VALID)
        actw.hModule = actctx->hModule;

    ret = CreateActCtxW(&actw);

done:
    HeapFree(GetProcessHeap(), 0, src);
    HeapFree(GetProcessHeap(), 0, assdir);
    HeapFree(GetProcessHeap(), 0, resname);
    HeapFree(GetProcessHeap(), 0, appname);
    return ret;
}

/***********************************************************************
 *           FindActCtxSectionStringA   (KERNEL32.@)
 */
BOOL WINAPI FindActCtxSectionStringA( DWORD flags, const GUID *guid, ULONG id, const char *search,
                                      ACTCTX_SECTION_KEYED_DATA *info )
{
    LPWSTR searchW;
    DWORD len;
    BOOL ret;

    TRACE("%08lx %s %lu %s %p\n", flags, debugstr_guid(guid), id, debugstr_a(search), info);

    if (!search || !info)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    len = MultiByteToWideChar(CP_ACP, 0, search, -1, NULL, 0);
    searchW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, search, -1, searchW, len);
    ret = FindActCtxSectionStringW( flags, guid, id, searchW, info );
    HeapFree(GetProcessHeap(), 0, searchW);
    return ret;
}


/***********************************************************************
 *           CmdBatNotification   (KERNEL32.@)
 *
 * Notifies the system that a batch file has started or finished.
 *
 * PARAMS
 *  bBatchRunning [I]  TRUE if a batch file has started or
 *                     FALSE if a batch file has finished executing.
 *
 * RETURNS
 *  Unknown.
 */
BOOL WINAPI CmdBatNotification( BOOL bBatchRunning )
{
    FIXME("%d\n", bBatchRunning);
    return FALSE;
}
#endif
/***********************************************************************
 *           RegisterApplicationRestart       (KERNEL32.@)
 */
HRESULT WINAPI RegisterApplicationRestart(PCWSTR pwzCommandLine, DWORD dwFlags)
{
    FIXME("(%s,%ld)\n", debugstr_w(pwzCommandLine), dwFlags);

    return S_OK;
}

/**********************************************************************
 *           GetSystemDEPPolicy     (KERNEL32.@)
 */
DEP_SYSTEM_POLICY_TYPE WINAPI GetSystemDEPPolicy(void)
{
    return user_shared_data->NXSupportPolicy;
}

/**********************************************************************
 *           SetProcessDEPPolicy     (KERNEL32.@)
 */
BOOL WINAPI SetProcessDEPPolicy( DWORD flags )
{
    ULONG dep_flags = 0;

    TRACE("%#lx\n", flags);

    if (flags & PROCESS_DEP_ENABLE)
        dep_flags |= MEM_EXECUTE_OPTION_DISABLE | MEM_EXECUTE_OPTION_PERMANENT;
    if (flags & PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION)
        dep_flags |= MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION;

    return set_ntstatus( NtSetInformationProcess( GetCurrentProcess(), ProcessExecuteFlags,
                                                  &dep_flags, sizeof(dep_flags) ) );
}

/**********************************************************************
 *           ApplicationRecoveryFinished     (KERNEL32.@)
 */
VOID WINAPI ApplicationRecoveryFinished(BOOL success)
{
    FIXME(": stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/**********************************************************************
 *           ApplicationRecoveryInProgress     (KERNEL32.@)
 */
HRESULT WINAPI ApplicationRecoveryInProgress(PBOOL canceled)
{
    FIXME(":%p stub\n", canceled);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return E_FAIL;
}

/**********************************************************************
 *           RegisterApplicationRecoveryCallback     (KERNEL32.@)
 */
HRESULT WINAPI RegisterApplicationRecoveryCallback(APPLICATION_RECOVERY_CALLBACK callback, PVOID param, DWORD pingint, DWORD flags)
{
    FIXME("%p, %p, %ld, %ld: stub, faking success\n", callback, param, pingint, flags);
    return S_OK;
}

static SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *get_logical_processor_info(void)
{
    DWORD size = 0;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *info;

    GetLogicalProcessorInformationEx( RelationGroup, NULL, &size );
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return NULL;
    if (!(info = HeapAlloc( GetProcessHeap(), 0, size ))) return NULL;
    if (!GetLogicalProcessorInformationEx( RelationGroup, info, &size ))
    {
        HeapFree( GetProcessHeap(), 0, info );
        return NULL;
    }
    return info;
}

#ifndef __REACTOS__
/***********************************************************************
 *           GetActiveProcessorGroupCount (KERNEL32.@)
 */
WORD WINAPI GetActiveProcessorGroupCount(void)
{
    WORD groups;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *info;

    TRACE("()\n");

    if (!(info = get_logical_processor_info())) return 0;

    groups = info->Group.ActiveGroupCount;

    HeapFree(GetProcessHeap(), 0, info);
    return groups;
}

/***********************************************************************
 *           GetActiveProcessorCount (KERNEL32.@)
 */
DWORD WINAPI GetActiveProcessorCount(WORD group)
{
    DWORD cpus = 0;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *info;

    TRACE("(0x%x)\n", group);

    if (!(info = get_logical_processor_info())) return 0;

    if (group == ALL_PROCESSOR_GROUPS)
    {
        for (group = 0; group < info->Group.ActiveGroupCount; group++)
            cpus += info->Group.GroupInfo[group].ActiveProcessorCount;
    }
    else
    {
        if (group < info->Group.ActiveGroupCount)
            cpus = info->Group.GroupInfo[group].ActiveProcessorCount;
    }

    HeapFree(GetProcessHeap(), 0, info);
    return cpus;
}
#endif

/***********************************************************************
 *           GetMaximumProcessorCount (KERNEL32.@)
 */
DWORD WINAPI GetMaximumProcessorCount(WORD group)
{
    DWORD cpus = 0;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *info;

    TRACE("(0x%x)\n", group);

    if (!(info = get_logical_processor_info())) return 0;

    if (group == ALL_PROCESSOR_GROUPS)
    {
        for (group = 0; group < info->Group.MaximumGroupCount; group++)
            cpus += info->Group.GroupInfo[group].MaximumProcessorCount;
    }
    else
    {
        if (group < info->Group.MaximumGroupCount)
            cpus = info->Group.GroupInfo[group].MaximumProcessorCount;
    }

    HeapFree(GetProcessHeap(), 0, info);
    return cpus;
}

/***********************************************************************
 *           GetMaximumProcessorGroupCount (KERNEL32.@)
 */
WORD WINAPI GetMaximumProcessorGroupCount(void)
{
    WORD groups;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *info;

    TRACE("()\n");

    if (!(info = get_logical_processor_info())) return 0;

    groups = info->Group.MaximumGroupCount;

    HeapFree(GetProcessHeap(), 0, info);
    return groups;
}
#ifndef __REACTOS__
/***********************************************************************
 *           GetFirmwareEnvironmentVariableA     (KERNEL32.@)
 */
DWORD WINAPI GetFirmwareEnvironmentVariableA(LPCSTR name, LPCSTR guid, PVOID buffer, DWORD size)
{
    FIXME("stub: %s %s %p %lu\n", debugstr_a(name), debugstr_a(guid), buffer, size);
    SetLastError(ERROR_INVALID_FUNCTION);
    return 0;
}

/***********************************************************************
 *           GetFirmwareEnvironmentVariableW     (KERNEL32.@)
 */
DWORD WINAPI GetFirmwareEnvironmentVariableW(LPCWSTR name, LPCWSTR guid, PVOID buffer, DWORD size)
{
    FIXME("stub: %s %s %p %lu\n", debugstr_w(name), debugstr_w(guid), buffer, size);
    SetLastError(ERROR_INVALID_FUNCTION);
    return 0;
}

/***********************************************************************
 *           SetFirmwareEnvironmentVariableA     (KERNEL32.@)
 */
BOOL WINAPI SetFirmwareEnvironmentVariableA(const char *name, const char *guid, void *buffer, DWORD size)
{
    FIXME("stub: %s %s %p %lu\n", debugstr_a(name), debugstr_a(guid), buffer, size);
    SetLastError(ERROR_INVALID_FUNCTION);
    return FALSE;
}

/***********************************************************************
 *           SetFirmwareEnvironmentVariableW     (KERNEL32.@)
 */
BOOL WINAPI SetFirmwareEnvironmentVariableW(const WCHAR *name, const WCHAR *guid, void *buffer, DWORD size)
{
    FIXME("stub: %s %s %p %lu\n", debugstr_w(name), debugstr_w(guid), buffer, size);
    SetLastError(ERROR_INVALID_FUNCTION);
    return FALSE;
}

/***********************************************************************
 *           GetFirmwareType     (KERNEL32.@)
 */
BOOL WINAPI GetFirmwareType(FIRMWARE_TYPE *type)
{
    if (!type)
        return FALSE;

    *type = FirmwareTypeUnknown;
    return TRUE;
}

/**********************************************************************
 *           GetNumaNodeProcessorMask     (KERNEL32.@)
 */
BOOL WINAPI GetNumaNodeProcessorMask(UCHAR node, PULONGLONG mask)
{
    FIXME("(%c %p): stub\n", node, mask);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 *           GetNumaAvailableMemoryNode     (KERNEL32.@)
 */
BOOL WINAPI GetNumaAvailableMemoryNode(UCHAR node, PULONGLONG available_bytes)
{
    FIXME("(%c %p): stub\n", node, available_bytes);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif

/**********************************************************************
 *           GetNumaAvailableMemoryNodeEx     (KERNEL32.@)
 */
BOOL WINAPI GetNumaAvailableMemoryNodeEx(USHORT node, PULONGLONG available_bytes)
{
    FIXME("(%hu %p): stub\n", node, available_bytes);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#ifndef __REACTOS__
/***********************************************************************
 *           GetNumaProcessorNode (KERNEL32.@)
 */
BOOL WINAPI GetNumaProcessorNode(UCHAR processor, PUCHAR node)
{
    TRACE("(%d, %p)\n", processor, node);

    if (processor < system_info.NumberOfProcessors)
    {
        *node = 0;
        return TRUE;
    }

    *node = 0xFF;
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}
#endif

/***********************************************************************
 *           GetNumaProcessorNodeEx (KERNEL32.@)
 */
BOOL WINAPI GetNumaProcessorNodeEx(PPROCESSOR_NUMBER processor, PUSHORT node_number)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           GetNumaProximityNode (KERNEL32.@)
 */
BOOL WINAPI GetNumaProximityNode(ULONG  proximity_id, PUCHAR node_number)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 *           GetProcessDEPPolicy     (KERNEL32.@)
 */
BOOL WINAPI GetProcessDEPPolicy(HANDLE process, LPDWORD flags, PBOOL permanent)
{
    ULONG dep_flags;

    TRACE("(%p %p %p)\n", process, flags, permanent);

    if (!set_ntstatus( NtQueryInformationProcess( GetCurrentProcess(), ProcessExecuteFlags,
                                                  &dep_flags, sizeof(dep_flags), NULL )))
        return FALSE;

    if (flags)
    {
        *flags = 0;
        if (dep_flags & MEM_EXECUTE_OPTION_DISABLE)
            *flags |= PROCESS_DEP_ENABLE;
        if (dep_flags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION)
            *flags |= PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION;
    }

    if (permanent) *permanent = (dep_flags & MEM_EXECUTE_OPTION_PERMANENT) != 0;
    return TRUE;
}

/***********************************************************************
 *           UnregisterApplicationRestart       (KERNEL32.@)
 */
HRESULT WINAPI UnregisterApplicationRestart(void)
{
    FIXME(": stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return S_OK;
}

/***********************************************************************
 *           CreateUmsCompletionList   (KERNEL32.@)
 */
BOOL WINAPI CreateUmsCompletionList(PUMS_COMPLETION_LIST *list)
{
    FIXME( "%p: stub\n", list );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           CreateUmsThreadContext   (KERNEL32.@)
 */
BOOL WINAPI CreateUmsThreadContext(PUMS_CONTEXT *ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           DeleteUmsCompletionList   (KERNEL32.@)
 */
BOOL WINAPI DeleteUmsCompletionList(PUMS_COMPLETION_LIST list)
{
    FIXME( "%p: stub\n", list );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           DeleteUmsThreadContext   (KERNEL32.@)
 */
BOOL WINAPI DeleteUmsThreadContext(PUMS_CONTEXT ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           DequeueUmsCompletionListItems   (KERNEL32.@)
 */
BOOL WINAPI DequeueUmsCompletionListItems(void *list, DWORD timeout, PUMS_CONTEXT *ctx)
{
    FIXME( "%p,%08lx,%p: stub\n", list, timeout, ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           EnterUmsSchedulingMode   (KERNEL32.@)
 */
BOOL WINAPI EnterUmsSchedulingMode(UMS_SCHEDULER_STARTUP_INFO *info)
{
    FIXME( "%p: stub\n", info );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           ExecuteUmsThread   (KERNEL32.@)
 */
BOOL WINAPI ExecuteUmsThread(PUMS_CONTEXT ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           GetCurrentUmsThread   (KERNEL32.@)
 */
PUMS_CONTEXT WINAPI GetCurrentUmsThread(void)
{
    FIXME( "stub\n" );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           GetNextUmsListItem   (KERNEL32.@)
 */
PUMS_CONTEXT WINAPI GetNextUmsListItem(PUMS_CONTEXT ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return NULL;
}

/***********************************************************************
 *           GetUmsCompletionListEvent   (KERNEL32.@)
 */
BOOL WINAPI GetUmsCompletionListEvent(PUMS_COMPLETION_LIST list, HANDLE *event)
{
    FIXME( "%p,%p: stub\n", list, event );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           QueryUmsThreadInformation   (KERNEL32.@)
 */
BOOL WINAPI QueryUmsThreadInformation(PUMS_CONTEXT ctx, UMS_THREAD_INFO_CLASS class,
                                       void *buf, ULONG length, ULONG *ret_length)
{
    FIXME( "%p,%08x,%p,%08lx,%p: stub\n", ctx, class, buf, length, ret_length );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           SetUmsThreadInformation   (KERNEL32.@)
 */
BOOL WINAPI SetUmsThreadInformation(PUMS_CONTEXT ctx, UMS_THREAD_INFO_CLASS class,
                                     void *buf, ULONG length)
{
    FIXME( "%p,%08x,%p,%08lx: stub\n", ctx, class, buf, length );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           UmsThreadYield   (KERNEL32.@)
 */
BOOL WINAPI UmsThreadYield(void *param)
{
    FIXME( "%p: stub\n", param );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}
