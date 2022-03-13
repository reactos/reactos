/*
 * Custom Action processing for the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Aric Stewart for CodeWeavers
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

#define COBJMACROS

#include <stdarg.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "msidefs.h"
#include "winuser.h"
#include "objbase.h"
#include "oleauto.h"

#include "msipriv.h"
#include "winemsi_s.h"
#include "wine/asm.h"
#include "wine/heap.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/exception.h"

#ifdef __REACTOS__
#ifndef STATUS_ACCESS_VIOLATION
#define STATUS_ACCESS_VIOLATION ((NTSTATUS)0xC0000005)
#endif
#endif

WINE_DEFAULT_DEBUG_CHANNEL(msi);

#define CUSTOM_ACTION_TYPE_MASK 0x3F

typedef struct tagMSIRUNNINGACTION
{
    struct list entry;
    HANDLE handle;
    BOOL   process;
    LPWSTR name;
} MSIRUNNINGACTION;

typedef UINT (WINAPI *MsiCustomActionEntryPoint)( MSIHANDLE );

static CRITICAL_SECTION msi_custom_action_cs;
static CRITICAL_SECTION_DEBUG msi_custom_action_cs_debug =
{
    0, 0, &msi_custom_action_cs,
    { &msi_custom_action_cs_debug.ProcessLocksList,
      &msi_custom_action_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": msi_custom_action_cs") }
};
static CRITICAL_SECTION msi_custom_action_cs = { &msi_custom_action_cs_debug, -1, 0, 0, 0, 0 };

static struct list msi_pending_custom_actions = LIST_INIT( msi_pending_custom_actions );

void  __RPC_FAR * __RPC_USER MIDL_user_allocate(SIZE_T len)
{
    return heap_alloc(len);
}

void __RPC_USER MIDL_user_free(void __RPC_FAR * ptr)
{
    heap_free(ptr);
}

LONG WINAPI rpc_filter(EXCEPTION_POINTERS *eptr)
{
    return I_RpcExceptionFilter(eptr->ExceptionRecord->ExceptionCode);
}

UINT msi_schedule_action( MSIPACKAGE *package, UINT script, const WCHAR *action )
{
    UINT count;
    WCHAR **newbuf = NULL;

    if (script >= SCRIPT_MAX)
    {
        FIXME("Unknown script requested %u\n", script);
        return ERROR_FUNCTION_FAILED;
    }
    TRACE("Scheduling action %s in script %u\n", debugstr_w(action), script);

    count = package->script_actions_count[script];
    package->script_actions_count[script]++;
    if (count != 0) newbuf = msi_realloc( package->script_actions[script],
                                          package->script_actions_count[script] * sizeof(WCHAR *) );
    else newbuf = msi_alloc( sizeof(WCHAR *) );

    newbuf[count] = strdupW( action );
    package->script_actions[script] = newbuf;
    return ERROR_SUCCESS;
}

UINT msi_register_unique_action( MSIPACKAGE *package, const WCHAR *action )
{
    UINT count;
    WCHAR **newbuf = NULL;

    TRACE("Registering %s as unique action\n", debugstr_w(action));

    count = package->unique_actions_count;
    package->unique_actions_count++;
    if (count != 0) newbuf = msi_realloc( package->unique_actions,
                                          package->unique_actions_count * sizeof(WCHAR *) );
    else newbuf = msi_alloc( sizeof(WCHAR *) );

    newbuf[count] = strdupW( action );
    package->unique_actions = newbuf;
    return ERROR_SUCCESS;
}

BOOL msi_action_is_unique( const MSIPACKAGE *package, const WCHAR *action )
{
    UINT i;

    for (i = 0; i < package->unique_actions_count; i++)
    {
        if (!wcscmp( package->unique_actions[i], action )) return TRUE;
    }
    return FALSE;
}

static BOOL check_execution_scheduling_options(MSIPACKAGE *package, LPCWSTR action, UINT options)
{
    if ((options & msidbCustomActionTypeClientRepeat) ==
            msidbCustomActionTypeClientRepeat)
    {
        if (!(package->InWhatSequence & SEQUENCE_UI &&
            package->InWhatSequence & SEQUENCE_EXEC))
        {
            TRACE("Skipping action due to dbCustomActionTypeClientRepeat option.\n");
            return FALSE;
        }
    }
    else if (options & msidbCustomActionTypeFirstSequence)
    {
        if (package->InWhatSequence & SEQUENCE_UI &&
            package->InWhatSequence & SEQUENCE_EXEC )
        {
            TRACE("Skipping action due to msidbCustomActionTypeFirstSequence option.\n");
            return FALSE;
        }
    }
    else if (options & msidbCustomActionTypeOncePerProcess)
    {
        if (msi_action_is_unique(package, action))
        {
            TRACE("Skipping action due to msidbCustomActionTypeOncePerProcess option.\n");
            return FALSE;
        }
        else
            msi_register_unique_action(package, action);
    }

    return TRUE;
}

/* stores the following properties before the action:
 *
 *    [CustomActionData<=>UserSID<=>ProductCode]Action
 */
static LPWSTR msi_get_deferred_action(LPCWSTR action, LPCWSTR actiondata,
                                      LPCWSTR usersid, LPCWSTR prodcode)
{
    LPWSTR deferred;
    DWORD len;

    if (!actiondata)
        return strdupW(action);

    len = lstrlenW(action) + lstrlenW(actiondata) +
          lstrlenW(usersid) + lstrlenW(prodcode) +
          lstrlenW(L"[%s<=>%s<=>%s]%s") - 7;
    deferred = msi_alloc(len * sizeof(WCHAR));

    swprintf(deferred, len, L"[%s<=>%s<=>%s]%s", actiondata, usersid, prodcode, action);
    return deferred;
}

static void set_deferred_action_props( MSIPACKAGE *package, const WCHAR *deferred_data )
{
    const WCHAR *end, *beg = deferred_data + 1;

    end = wcsstr(beg, L"<=>");
    msi_set_property( package->db, L"CustomActionData", beg, end - beg );
    beg = end + 3;

    end = wcsstr(beg, L"<=>");
    msi_set_property( package->db, L"UserSID", beg, end - beg );
    beg = end + 3;

    end = wcschr(beg, ']');
    msi_set_property( package->db, L"ProductCode", beg, end - beg );
}

WCHAR *msi_create_temp_file( MSIDATABASE *db )
{
    WCHAR *ret;

    if (!db->tempfolder)
    {
        WCHAR tmp[MAX_PATH];
        UINT len = ARRAY_SIZE( tmp );

        if (msi_get_property( db, L"TempFolder", tmp, &len ) ||
            GetFileAttributesW( tmp ) != FILE_ATTRIBUTE_DIRECTORY)
        {
            GetTempPathW( MAX_PATH, tmp );
        }
        if (!(db->tempfolder = strdupW( tmp ))) return NULL;
    }

    if ((ret = msi_alloc( (lstrlenW( db->tempfolder ) + 20) * sizeof(WCHAR) )))
    {
        if (!GetTempFileNameW( db->tempfolder, L"msi", 0, ret ))
        {
            msi_free( ret );
            return NULL;
        }
    }

    return ret;
}

static MSIBINARY *create_temp_binary(MSIPACKAGE *package, LPCWSTR source)
{
    MSIRECORD *row;
    MSIBINARY *binary = NULL;
    HANDLE file;
    CHAR buffer[1024];
    WCHAR *tmpfile;
    DWORD sz, write;
    UINT r;

    if (!(tmpfile = msi_create_temp_file( package->db ))) return NULL;

    if (!(row = MSI_QueryGetRecord( package->db, L"SELECT * FROM `Binary` WHERE `Name` = '%s'", source ))) goto error;
    if (!(binary = msi_alloc_zero( sizeof(MSIBINARY) ))) goto error;

    file = CreateFileW( tmpfile, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if (file == INVALID_HANDLE_VALUE) goto error;

    do
    {
        sz = sizeof(buffer);
        r = MSI_RecordReadStream( row, 2, buffer, &sz );
        if (r != ERROR_SUCCESS)
        {
            ERR("Failed to get stream\n");
            break;
        }
        WriteFile( file, buffer, sz, &write, NULL );
    } while (sz == sizeof buffer);

    CloseHandle( file );
    if (r != ERROR_SUCCESS) goto error;

    binary->source = strdupW( source );
    binary->tmpfile = tmpfile;
    list_add_tail( &package->binaries, &binary->entry );

    msiobj_release( &row->hdr );
    return binary;

error:
    if (row) msiobj_release( &row->hdr );
    DeleteFileW( tmpfile );
    msi_free( tmpfile );
    msi_free( binary );
    return NULL;
}

static MSIBINARY *get_temp_binary(MSIPACKAGE *package, LPCWSTR source)
{
    MSIBINARY *binary;

    LIST_FOR_EACH_ENTRY( binary, &package->binaries, MSIBINARY, entry )
    {
        if (!wcscmp( binary->source, source ))
            return binary;
    }

    return create_temp_binary(package, source);
}

static void file_running_action(MSIPACKAGE* package, HANDLE Handle,
                                BOOL process, LPCWSTR name)
{
    MSIRUNNINGACTION *action;

    action = msi_alloc( sizeof(MSIRUNNINGACTION) );

    action->handle = Handle;
    action->process = process;
    action->name = strdupW(name);

    list_add_tail( &package->RunningActions, &action->entry );
}

static UINT custom_get_process_return( HANDLE process )
{
    DWORD rc = 0;

    GetExitCodeProcess( process, &rc );
    TRACE("exit code is %u\n", rc);
    if (rc != 0)
        return ERROR_FUNCTION_FAILED;
    return ERROR_SUCCESS;
}

static UINT custom_get_thread_return( MSIPACKAGE *package, HANDLE thread )
{
    DWORD rc = 0;

    GetExitCodeThread( thread, &rc );

    switch (rc)
    {
    case ERROR_FUNCTION_NOT_CALLED:
    case ERROR_SUCCESS:
    case ERROR_INSTALL_USEREXIT:
    case ERROR_INSTALL_FAILURE:
        return rc;
    case ERROR_NO_MORE_ITEMS:
        return ERROR_SUCCESS;
    case ERROR_INSTALL_SUSPEND:
        ACTION_ForceReboot( package );
        return ERROR_SUCCESS;
    default:
        ERR("Invalid Return Code %d\n",rc);
        return ERROR_INSTALL_FAILURE;
    }
}

static UINT wait_process_handle(MSIPACKAGE* package, UINT type,
                           HANDLE ProcessHandle, LPCWSTR name)
{
    UINT rc = ERROR_SUCCESS;

    if (!(type & msidbCustomActionTypeAsync))
    {
        TRACE("waiting for %s\n", debugstr_w(name));

        msi_dialog_check_messages(ProcessHandle);

        if (!(type & msidbCustomActionTypeContinue))
            rc = custom_get_process_return(ProcessHandle);

        CloseHandle(ProcessHandle);
    }
    else
    {
        TRACE("%s running in background\n", debugstr_w(name));

        if (!(type & msidbCustomActionTypeContinue))
            file_running_action(package, ProcessHandle, TRUE, name);
        else
            CloseHandle(ProcessHandle);
    }

    return rc;
}

typedef struct _msi_custom_action_info {
    struct list entry;
    MSIPACKAGE *package;
    LPWSTR source;
    LPWSTR target;
    HANDLE handle;
    LPWSTR action;
    INT type;
    GUID guid;
    DWORD arch;
} msi_custom_action_info;

static void free_custom_action_data( msi_custom_action_info *info )
{
    EnterCriticalSection( &msi_custom_action_cs );

    list_remove( &info->entry );
    if (info->handle)
        CloseHandle( info->handle );
    msi_free( info->action );
    msi_free( info->source );
    msi_free( info->target );
    msiobj_release( &info->package->hdr );
    msi_free( info );

    LeaveCriticalSection( &msi_custom_action_cs );
}

static UINT wait_thread_handle( msi_custom_action_info *info )
{
    UINT rc = ERROR_SUCCESS;

    if (!(info->type & msidbCustomActionTypeAsync))
    {
        TRACE("waiting for %s\n", debugstr_w( info->action ));

        msi_dialog_check_messages( info->handle );

        if (!(info->type & msidbCustomActionTypeContinue))
            rc = custom_get_thread_return( info->package, info->handle );

        free_custom_action_data( info );
    }
    else
    {
        TRACE("%s running in background\n", debugstr_w( info->action ));
    }

    return rc;
}

static msi_custom_action_info *find_action_by_guid( const GUID *guid )
{
    msi_custom_action_info *info;
    BOOL found = FALSE;

    EnterCriticalSection( &msi_custom_action_cs );

    LIST_FOR_EACH_ENTRY( info, &msi_pending_custom_actions, msi_custom_action_info, entry )
    {
        if (IsEqualGUID( &info->guid, guid ))
        {
            found = TRUE;
            break;
        }
    }

    LeaveCriticalSection( &msi_custom_action_cs );

    if (!found)
        return NULL;

    return info;
}

static void handle_msi_break(LPCSTR target)
{
    char format[] = "To debug your custom action, attach your debugger to "
                    "process %i (0x%X) and press OK";
    char val[MAX_PATH];
    char msg[100];

    if (!GetEnvironmentVariableA("MsiBreak", val, MAX_PATH))
        return;

    if (strcmp(val, target))
        return;

    sprintf(msg, format, GetCurrentProcessId(), GetCurrentProcessId());
    MessageBoxA(NULL, msg, "Windows Installer", MB_OK);
    DebugBreak();
}

#if defined __i386__ && defined _MSC_VER
__declspec(naked) UINT custom_proc_wrapper(MsiCustomActionEntryPoint entry, MSIHANDLE hinst)
{
    __asm
    {
        push ebp
        mov ebp, esp
        sub esp, 4
        push [ebp+12]
        call [ebp+8]
        leave
        ret
    }
}
#elif defined __i386__ && defined __GNUC__
/* wrapper for apps that don't declare the thread function correctly */
extern UINT custom_proc_wrapper( MsiCustomActionEntryPoint entry, MSIHANDLE hinst );
__ASM_GLOBAL_FUNC(custom_proc_wrapper,
                  "pushl %ebp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                  "movl %esp,%ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                  "subl $4,%esp\n\t"
                  "pushl 12(%ebp)\n\t"
                  "call *8(%ebp)\n\t"
                  "leave\n\t"
                  __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                  __ASM_CFI(".cfi_same_value %ebp\n\t")
                  "ret" )
#else
static UINT custom_proc_wrapper( MsiCustomActionEntryPoint entry, MSIHANDLE hinst )
{
    return entry(hinst);
}
#endif

UINT CDECL __wine_msi_call_dll_function(DWORD client_pid, const GUID *guid)
{
    MsiCustomActionEntryPoint fn;
    MSIHANDLE remote_package = 0;
    RPC_WSTR binding_str;
    MSIHANDLE hPackage;
    RPC_STATUS status;
    LPWSTR dll = NULL;
    LPSTR proc = NULL;
    HANDLE hModule;
    INT type;
    UINT r;

    TRACE("%s\n", debugstr_guid( guid ));

    if (!rpc_handle)
    {
        WCHAR endpoint[12];

        swprintf(endpoint, ARRAY_SIZE(endpoint), L"msi%x", client_pid);
        status = RpcStringBindingComposeW(NULL, (WCHAR *)L"ncalrpc", NULL, endpoint, NULL, &binding_str);
        if (status != RPC_S_OK)
        {
            ERR("RpcStringBindingCompose failed: %#x\n", status);
            return status;
        }
        status = RpcBindingFromStringBindingW(binding_str, &rpc_handle);
        if (status != RPC_S_OK)
        {
            ERR("RpcBindingFromStringBinding failed: %#x\n", status);
            return status;
        }
        RpcStringFreeW(&binding_str);
    }

    r = remote_GetActionInfo(guid, &type, &dll, &proc, &remote_package);
    if (r != ERROR_SUCCESS)
        return r;

    hPackage = alloc_msi_remote_handle( remote_package );
    if (!hPackage)
    {
        ERR( "failed to create handle for %x\n", remote_package );
        midl_user_free( dll );
        midl_user_free( proc );
        return ERROR_INSTALL_FAILURE;
    }

    hModule = LoadLibraryW( dll );
    if (!hModule)
    {
        ERR( "failed to load dll %s (%u)\n", debugstr_w( dll ), GetLastError() );
        midl_user_free( dll );
        midl_user_free( proc );
        MsiCloseHandle( hPackage );
        return ERROR_SUCCESS;
    }

    fn = (MsiCustomActionEntryPoint) GetProcAddress( hModule, proc );
    if (!fn) WARN( "GetProcAddress(%s) failed\n", debugstr_a(proc) );
    else
    {
        handle_msi_break(proc);

        __TRY
        {
            r = custom_proc_wrapper( fn, hPackage );
        }
        __EXCEPT_PAGE_FAULT
        {
            ERR( "Custom action (%s:%s) caused a page fault: %08x\n",
                 debugstr_w(dll), debugstr_a(proc), GetExceptionCode() );
            r = ERROR_SUCCESS;
        }
        __ENDTRY;
    }

    FreeLibrary(hModule);

    midl_user_free(dll);
    midl_user_free(proc);

    MsiCloseAllHandles();

    return r;
}

static DWORD custom_start_server(MSIPACKAGE *package, DWORD arch)
{
    WCHAR path[MAX_PATH], cmdline[MAX_PATH + 23];
    PROCESS_INFORMATION pi = {0};
    STARTUPINFOW si = {0};
    WCHAR buffer[24];
    void *cookie;
    HANDLE pipe;

    if ((arch == SCS_32BIT_BINARY && package->custom_server_32_process) ||
        (arch == SCS_64BIT_BINARY && package->custom_server_64_process))
        return ERROR_SUCCESS;

    swprintf(buffer, ARRAY_SIZE(buffer), L"\\\\.\\pipe\\msica_%x_%d",
             GetCurrentProcessId(), arch == SCS_32BIT_BINARY ? 32 : 64);
    pipe = CreateNamedPipeW(buffer, PIPE_ACCESS_DUPLEX, 0, 1, sizeof(DWORD64),
        sizeof(GUID), 0, NULL);
    if (pipe == INVALID_HANDLE_VALUE)
        ERR("Failed to create custom action client pipe: %u\n", GetLastError());

    if ((sizeof(void *) == 8 || is_wow64) && arch == SCS_32BIT_BINARY)
        GetSystemWow64DirectoryW(path, MAX_PATH - ARRAY_SIZE(L"\\msiexec.exe"));
    else
        GetSystemDirectoryW(path, MAX_PATH - ARRAY_SIZE(L"\\msiexec.exe"));
    lstrcatW(path, L"\\msiexec.exe");
    swprintf(cmdline, ARRAY_SIZE(cmdline), L"%s -Embedding %d", path, GetCurrentProcessId());

    if (is_wow64 && arch == SCS_64BIT_BINARY)
    {
        Wow64DisableWow64FsRedirection(&cookie);
        CreateProcessW(path, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        Wow64RevertWow64FsRedirection(cookie);
    }
    else
        CreateProcessW(path, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    CloseHandle(pi.hThread);

    if (arch == SCS_32BIT_BINARY)
    {
        package->custom_server_32_process = pi.hProcess;
        package->custom_server_32_pipe = pipe;
    }
    else
    {
        package->custom_server_64_process = pi.hProcess;
        package->custom_server_64_pipe = pipe;
    }

    if (!ConnectNamedPipe(pipe, NULL))
    {
        ERR("Failed to connect to custom action server: %u\n", GetLastError());
        return GetLastError();
    }

    return ERROR_SUCCESS;
}

void custom_stop_server(HANDLE process, HANDLE pipe)
{
    DWORD size;

    WriteFile(pipe, &GUID_NULL, sizeof(GUID_NULL), &size, NULL);
    WaitForSingleObject(process, INFINITE);
    CloseHandle(process);
    CloseHandle(pipe);
}

static DWORD WINAPI custom_client_thread(void *arg)
{
    msi_custom_action_info *info = arg;
    DWORD64 thread64;
    HANDLE process;
    HANDLE thread;
    HANDLE pipe;
    DWORD size;
    UINT rc;

    CoInitializeEx(NULL, COINIT_MULTITHREADED); /* needed to marshal streams */

    if (info->arch == SCS_32BIT_BINARY)
    {
        process = info->package->custom_server_32_process;
        pipe = info->package->custom_server_32_pipe;
    }
    else
    {
        process = info->package->custom_server_64_process;
        pipe = info->package->custom_server_64_pipe;
    }

    EnterCriticalSection(&msi_custom_action_cs);

    if (!WriteFile(pipe, &info->guid, sizeof(info->guid), &size, NULL) ||
        size != sizeof(info->guid))
    {
        ERR("Failed to write to custom action client pipe: %u\n", GetLastError());
        LeaveCriticalSection(&msi_custom_action_cs);
        return GetLastError();
    }
    if (!ReadFile(pipe, &thread64, sizeof(thread64), &size, NULL) || size != sizeof(thread64))
    {
        ERR("Failed to read from custom action client pipe: %u\n", GetLastError());
        LeaveCriticalSection(&msi_custom_action_cs);
        return GetLastError();
    }

    LeaveCriticalSection(&msi_custom_action_cs);

    if (DuplicateHandle(process, (HANDLE)(DWORD_PTR)thread64, GetCurrentProcess(),
        &thread, 0, FALSE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE))
    {
        WaitForSingleObject(thread, INFINITE);
        GetExitCodeThread(thread, &rc);
        CloseHandle(thread);
    }
    else
        rc = GetLastError();

    CoUninitialize();
    return rc;
}

static msi_custom_action_info *do_msidbCustomActionTypeDll(
    MSIPACKAGE *package, INT type, LPCWSTR source, LPCWSTR target, LPCWSTR action )
{
    msi_custom_action_info *info;
    RPC_STATUS status;
    BOOL ret;

    info = msi_alloc( sizeof *info );
    if (!info)
        return NULL;

    msiobj_addref( &package->hdr );
    info->package = package;
    info->type = type;
    info->target = strdupW( target );
    info->source = strdupW( source );
    info->action = strdupW( action );
    CoCreateGuid( &info->guid );

    EnterCriticalSection( &msi_custom_action_cs );
    list_add_tail( &msi_pending_custom_actions, &info->entry );
    LeaveCriticalSection( &msi_custom_action_cs );

    if (!package->rpc_server_started)
    {
        WCHAR endpoint[12];

        swprintf(endpoint, ARRAY_SIZE(endpoint), L"msi%x", GetCurrentProcessId());
        status = RpcServerUseProtseqEpW((WCHAR *)L"ncalrpc", RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
            endpoint, NULL);
        if (status != RPC_S_OK)
        {
            ERR("RpcServerUseProtseqEp failed: %#x\n", status);
            return NULL;
        }

        status = RpcServerRegisterIfEx(s_IWineMsiRemote_v0_0_s_ifspec, NULL, NULL,
            RPC_IF_AUTOLISTEN, RPC_C_LISTEN_MAX_CALLS_DEFAULT, NULL);
        if (status != RPC_S_OK)
        {
            ERR("RpcServerRegisterIfEx failed: %#x\n", status);
            return NULL;
        }

        info->package->rpc_server_started = 1;
    }

    ret = GetBinaryTypeW(source, &info->arch);
    if (!ret)
        info->arch = (sizeof(void *) == 8 ? SCS_64BIT_BINARY : SCS_32BIT_BINARY);

    custom_start_server(package, info->arch);

    info->handle = CreateThread(NULL, 0, custom_client_thread, info, 0, NULL);
    if (!info->handle)
    {
        free_custom_action_data( info );
        return NULL;
    }

    return info;
}

static UINT HANDLE_CustomType1( MSIPACKAGE *package, const WCHAR *source, const WCHAR *target,
                                INT type, const WCHAR *action )
{
    msi_custom_action_info *info;
    MSIBINARY *binary;

    if (!(binary = get_temp_binary(package, source)))
        return ERROR_FUNCTION_FAILED;

    TRACE("Calling function %s from %s\n", debugstr_w(target), debugstr_w(binary->tmpfile));

    if (!(info = do_msidbCustomActionTypeDll( package, type, binary->tmpfile, target, action )))
        return ERROR_FUNCTION_FAILED;
    return wait_thread_handle( info );
}

static HANDLE execute_command( const WCHAR *app, WCHAR *arg, const WCHAR *dir )
{
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    WCHAR *exe = NULL, *cmd = NULL, *p;
    BOOL ret;

    if (app)
    {
        int len_arg = 0;
        DWORD len_exe;

        if (!(exe = msi_alloc( MAX_PATH * sizeof(WCHAR) ))) return INVALID_HANDLE_VALUE;
        len_exe = SearchPathW( NULL, app, L".exe", MAX_PATH, exe, NULL );
        if (len_exe >= MAX_PATH)
        {
            msi_free( exe );
            if (!(exe = msi_alloc( len_exe * sizeof(WCHAR) ))) return INVALID_HANDLE_VALUE;
            len_exe = SearchPathW( NULL, app, L".exe", len_exe, exe, NULL );
        }
        if (!len_exe)
        {
            ERR("can't find executable %u\n", GetLastError());
            msi_free( exe );
            return INVALID_HANDLE_VALUE;
        }

        if (arg) len_arg = lstrlenW( arg );
        if (!(cmd = msi_alloc( (len_exe + len_arg + 4) * sizeof(WCHAR) )))
        {
            msi_free( exe );
            return INVALID_HANDLE_VALUE;
        }
        p = cmd;
        if (wcschr( exe, ' ' ))
        {
            *p++ = '\"';
            memcpy( p, exe, len_exe * sizeof(WCHAR) );
            p += len_exe;
            *p++ = '\"';
            *p = 0;
        }
        else
        {
            lstrcpyW( p, exe );
            p += len_exe;
        }
        if (arg)
        {
            *p++ = ' ';
            memcpy( p, arg, len_arg * sizeof(WCHAR) );
            p[len_arg] = 0;
        }
    }
    memset( &si, 0, sizeof(STARTUPINFOW) );
    ret = CreateProcessW( exe, exe ? cmd : arg, NULL, NULL, FALSE, 0, NULL, dir, &si, &info );
    msi_free( cmd );
    msi_free( exe );
    if (!ret)
    {
        ERR("unable to execute command %u\n", GetLastError());
        return INVALID_HANDLE_VALUE;
    }
    CloseHandle( info.hThread );
    return info.hProcess;
}

static UINT HANDLE_CustomType2( MSIPACKAGE *package, const WCHAR *source, const WCHAR *target,
                                INT type, const WCHAR *action )
{
    MSIBINARY *binary;
    HANDLE handle;
    WCHAR *arg;

    if (!(binary = get_temp_binary(package, source)))
        return ERROR_FUNCTION_FAILED;

    deformat_string( package, target, &arg );
    TRACE("exe %s arg %s\n", debugstr_w(binary->tmpfile), debugstr_w(arg));

    handle = execute_command( binary->tmpfile, arg, L"C:\\" );
    msi_free( arg );
    if (handle == INVALID_HANDLE_VALUE) return ERROR_SUCCESS;
    return wait_process_handle( package, type, handle, action );
}

static UINT HANDLE_CustomType17( MSIPACKAGE *package, const WCHAR *source, const WCHAR *target,
                                 INT type, const WCHAR *action )
{
    msi_custom_action_info *info;
    MSIFILE *file;

    TRACE("%s %s\n", debugstr_w(source), debugstr_w(target));

    file = msi_get_loaded_file( package, source );
    if (!file)
    {
        ERR("invalid file key %s\n", debugstr_w( source ));
        return ERROR_FUNCTION_FAILED;
    }

    if (!(info = do_msidbCustomActionTypeDll( package, type, file->TargetPath, target, action )))
        return ERROR_FUNCTION_FAILED;
    return wait_thread_handle( info );
}

static UINT HANDLE_CustomType18( MSIPACKAGE *package, const WCHAR *source, const WCHAR *target,
                                 INT type, const WCHAR *action )
{
    MSIFILE *file;
    HANDLE handle;
    WCHAR *arg;

    if (!(file = msi_get_loaded_file( package, source ))) return ERROR_FUNCTION_FAILED;

    deformat_string( package, target, &arg );
    TRACE("exe %s arg %s\n", debugstr_w(file->TargetPath), debugstr_w(arg));

    handle = execute_command( file->TargetPath, arg, L"C:\\" );
    msi_free( arg );
    if (handle == INVALID_HANDLE_VALUE) return ERROR_SUCCESS;
    return wait_process_handle( package, type, handle, action );
}

static UINT HANDLE_CustomType19( MSIPACKAGE *package, const WCHAR *source, const WCHAR *target,
                                 INT type, const WCHAR *action )
{
    MSIRECORD *row = 0;
    LPWSTR deformated = NULL;

    deformat_string( package, target, &deformated );

    /* first try treat the error as a number */
    row = MSI_QueryGetRecord( package->db, L"SELECT `Message` FROM `Error` WHERE `Error` = '%s'", deformated );
    if( row )
    {
        LPCWSTR error = MSI_RecordGetString( row, 1 );
        if ((package->ui_level & INSTALLUILEVEL_MASK) != INSTALLUILEVEL_NONE)
            MessageBoxW( NULL, error, NULL, MB_OK );
        msiobj_release( &row->hdr );
    }
    else if ((package->ui_level & INSTALLUILEVEL_MASK) != INSTALLUILEVEL_NONE)
        MessageBoxW( NULL, deformated, NULL, MB_OK );

    msi_free( deformated );

    return ERROR_INSTALL_FAILURE;
}

static WCHAR *build_msiexec_args( const WCHAR *filename, const WCHAR *params )
{
    UINT len_filename = lstrlenW( filename ), len_params = lstrlenW( params );
    UINT len = ARRAY_SIZE(L"/qb /i ") - 1;
    WCHAR *ret;

    if (!(ret = msi_alloc( (len + len_filename + len_params + 4) * sizeof(WCHAR) ))) return NULL;
    memcpy( ret, L"/qb /i ", sizeof(L"/qb /i ") );
    ret[len++] = '"';
    memcpy( ret + len, filename, len_filename * sizeof(WCHAR) );
    len += len_filename;
    ret[len++] = '"';
    ret[len++] = ' ';
    lstrcpyW( ret + len, params );
    return ret;
}

static UINT HANDLE_CustomType23( MSIPACKAGE *package, const WCHAR *source, const WCHAR *target,
                                 INT type, const WCHAR *action )
{
    WCHAR *dir, *filename, *args, *p;
    UINT len_dir, len_source = lstrlenW( source );
    HANDLE handle;

    if (!(dir = msi_dup_property( package->db, L"OriginalDatabase" ))) return ERROR_OUTOFMEMORY;
    if (!(p = wcsrchr( dir, '\\' )) && !(p = wcsrchr( dir, '/' )))
    {
        msi_free( dir );
        return ERROR_FUNCTION_FAILED;
    }
    *p = 0;
    len_dir = p - dir;
    if (!(filename = msi_alloc( (len_dir + len_source + 2) * sizeof(WCHAR) )))
    {
        msi_free( dir );
        return ERROR_OUTOFMEMORY;
    }
    memcpy( filename, dir, len_dir * sizeof(WCHAR) );
    filename[len_dir++] = '\\';
    memcpy( filename + len_dir, source, len_source * sizeof(WCHAR) );
    filename[len_dir + len_source] = 0;

    if (!(args = build_msiexec_args( filename, target )))
    {
        msi_free( dir );
        return ERROR_OUTOFMEMORY;
    }

    TRACE("installing %s concurrently\n", debugstr_w(source));

    handle = execute_command( L"msiexec", args, dir );
    msi_free( dir );
    msi_free( args );
    if (handle == INVALID_HANDLE_VALUE) return ERROR_SUCCESS;
    return wait_process_handle( package, type, handle, action );
}

static UINT write_substorage_to_file( MSIPACKAGE *package, const WCHAR *source, const WCHAR *filename )
{
    IStorage *src = NULL, *dst = NULL;
    UINT r = ERROR_FUNCTION_FAILED;
    HRESULT hr;

    hr = StgCreateDocfile( filename, STGM_CREATE|STGM_TRANSACTED|STGM_WRITE|STGM_SHARE_EXCLUSIVE, 0, &dst );
    if (FAILED( hr ))
    {
        WARN( "can't open destination storage %s (%08x)\n", debugstr_w(filename), hr );
        goto done;
    }

    hr = IStorage_OpenStorage( package->db->storage, source, NULL, STGM_SHARE_EXCLUSIVE, NULL, 0, &src );
    if (FAILED( hr ))
    {
        WARN( "can't open source storage %s (%08x)\n", debugstr_w(source), hr );
        goto done;
    }

    hr = IStorage_CopyTo( src, 0, NULL, NULL, dst );
    if (FAILED( hr ))
    {
        ERR( "failed to copy storage %s (%08x)\n", debugstr_w(source), hr );
        goto done;
    }

    hr = IStorage_Commit( dst, 0 );
    if (FAILED( hr ))
        ERR( "failed to commit storage (%08x)\n", hr );
    else
        r = ERROR_SUCCESS;

done:
    if (src) IStorage_Release( src );
    if (dst) IStorage_Release( dst );
    return r;
}

static UINT HANDLE_CustomType7( MSIPACKAGE *package, const WCHAR *source, const WCHAR *target,
                                INT type, const WCHAR *action )
{
    WCHAR *tmpfile, *args;
    MSIBINARY *binary = NULL;
    HANDLE handle;
    UINT r;

    if (!(tmpfile = msi_create_temp_file( package->db ))) return ERROR_FUNCTION_FAILED;

    r = write_substorage_to_file( package, source, tmpfile );
    if (r != ERROR_SUCCESS)
        goto error;

    if (!(binary = msi_alloc( sizeof(*binary) ))) goto error;
    binary->source  = NULL;
    binary->tmpfile = tmpfile;
    list_add_tail( &package->binaries, &binary->entry );

    if (!(args = build_msiexec_args( tmpfile, target ))) return ERROR_OUTOFMEMORY;

    TRACE("installing %s concurrently\n", debugstr_w(source));

    handle = execute_command( L"msiexec", args, L"C:\\" );
    msi_free( args );
    if (handle == INVALID_HANDLE_VALUE) return ERROR_SUCCESS;
    return wait_process_handle( package, type, handle, action );

error:
    DeleteFileW( tmpfile );
    msi_free( tmpfile );
    return ERROR_FUNCTION_FAILED;
}

static UINT HANDLE_CustomType50( MSIPACKAGE *package, const WCHAR *source, const WCHAR *target,
                                 INT type, const WCHAR *action )
{
    WCHAR *exe, *arg;
    HANDLE handle;

    if (!(exe = msi_dup_property( package->db, source ))) return ERROR_SUCCESS;

    deformat_string( package, target, &arg );
    TRACE("exe %s arg %s\n", debugstr_w(exe), debugstr_w(arg));

    handle = execute_command( exe, arg, L"C:\\" );
    msi_free( exe );
    msi_free( arg );
    if (handle == INVALID_HANDLE_VALUE) return ERROR_SUCCESS;
    return wait_process_handle( package, type, handle, action );
}

static UINT HANDLE_CustomType34( MSIPACKAGE *package, const WCHAR *source, const WCHAR *target,
                                 INT type, const WCHAR *action )
{
    const WCHAR *workingdir = NULL;
    HANDLE handle;
    WCHAR *cmd;

    if (source)
    {
        workingdir = msi_get_target_folder( package, source );
        if (!workingdir) return ERROR_FUNCTION_FAILED;
    }
    deformat_string( package, target, &cmd );
    if (!cmd) return ERROR_FUNCTION_FAILED;

    TRACE("cmd %s dir %s\n", debugstr_w(cmd), debugstr_w(workingdir));

    handle = execute_command( NULL, cmd, workingdir );
    msi_free( cmd );
    if (handle == INVALID_HANDLE_VALUE) return ERROR_SUCCESS;
    return wait_process_handle( package, type, handle, action );
}

static DWORD ACTION_CallScript( const GUID *guid )
{
    msi_custom_action_info *info;
    MSIHANDLE hPackage;
    UINT r = ERROR_FUNCTION_FAILED;

    info = find_action_by_guid( guid );
    if (!info)
    {
        ERR("failed to find action %s\n", debugstr_guid( guid) );
        return ERROR_FUNCTION_FAILED;
    }

    TRACE("function %s, script %s\n", debugstr_w( info->target ), debugstr_w( info->source ) );

    hPackage = alloc_msihandle( &info->package->hdr );
    if (hPackage)
    {
        r = call_script( hPackage, info->type, info->source, info->target, info->action );
        TRACE("script returned %u\n", r);
        MsiCloseHandle( hPackage );
    }
    else
        ERR("failed to create handle for %p\n", info->package );

    return r;
}

static DWORD WINAPI ScriptThread( LPVOID arg )
{
    LPGUID guid = arg;
    DWORD rc;

    TRACE("custom action (%x) started\n", GetCurrentThreadId() );

    rc = ACTION_CallScript( guid );

    TRACE("custom action (%x) returned %i\n", GetCurrentThreadId(), rc );

    MsiCloseAllHandles();
    return rc;
}

static msi_custom_action_info *do_msidbCustomActionTypeScript(
    MSIPACKAGE *package, INT type, LPCWSTR script, LPCWSTR function, LPCWSTR action )
{
    msi_custom_action_info *info;

    info = msi_alloc( sizeof *info );
    if (!info)
        return NULL;

    msiobj_addref( &package->hdr );
    info->package = package;
    info->type = type;
    info->target = strdupW( function );
    info->source = strdupW( script );
    info->action = strdupW( action );
    CoCreateGuid( &info->guid );

    EnterCriticalSection( &msi_custom_action_cs );
    list_add_tail( &msi_pending_custom_actions, &info->entry );
    LeaveCriticalSection( &msi_custom_action_cs );

    info->handle = CreateThread( NULL, 0, ScriptThread, &info->guid, 0, NULL );
    if (!info->handle)
    {
        free_custom_action_data( info );
        return NULL;
    }

    return info;
}

static UINT HANDLE_CustomType37_38( MSIPACKAGE *package, const WCHAR *source, const WCHAR *target,
                                    INT type, const WCHAR *action )
{
    msi_custom_action_info *info;

    TRACE("%s %s\n", debugstr_w(source), debugstr_w(target));

    info = do_msidbCustomActionTypeScript( package, type, target, NULL, action );
    return wait_thread_handle( info );
}

static UINT HANDLE_CustomType5_6( MSIPACKAGE *package, const WCHAR *source, const WCHAR *target,
                                  INT type, const WCHAR *action )
{
    MSIRECORD *row = NULL;
    msi_custom_action_info *info;
    CHAR *buffer = NULL;
    WCHAR *bufferw = NULL;
    DWORD sz = 0;
    UINT r;

    TRACE("%s %s\n", debugstr_w(source), debugstr_w(target));

    row = MSI_QueryGetRecord(package->db, L"SELECT * FROM `Binary` WHERE `Name` = '%s'", source);
    if (!row)
        return ERROR_FUNCTION_FAILED;

    r = MSI_RecordReadStream(row, 2, NULL, &sz);
    if (r != ERROR_SUCCESS) goto done;

    buffer = msi_alloc( sz + 1 );
    if (!buffer)
    {
       r = ERROR_FUNCTION_FAILED;
       goto done;
    }

    r = MSI_RecordReadStream(row, 2, buffer, &sz);
    if (r != ERROR_SUCCESS)
        goto done;

    buffer[sz] = 0;
    bufferw = strdupAtoW(buffer);
    if (!bufferw)
    {
        r = ERROR_FUNCTION_FAILED;
        goto done;
    }

    info = do_msidbCustomActionTypeScript( package, type, bufferw, target, action );
    r = wait_thread_handle( info );

done:
    msi_free(bufferw);
    msi_free(buffer);
    msiobj_release(&row->hdr);
    return r;
}

static UINT HANDLE_CustomType21_22( MSIPACKAGE *package, const WCHAR *source, const WCHAR *target,
                                    INT type, const WCHAR *action )
{
    msi_custom_action_info *info;
    MSIFILE *file;
    HANDLE hFile;
    DWORD sz, szHighWord = 0, read;
    CHAR *buffer=NULL;
    WCHAR *bufferw=NULL;
    BOOL bRet;
    UINT r;

    TRACE("%s %s\n", debugstr_w(source), debugstr_w(target));

    file = msi_get_loaded_file(package, source);
    if (!file)
    {
        ERR("invalid file key %s\n", debugstr_w(source));
        return ERROR_FUNCTION_FAILED;
    }

    hFile = msi_create_file( package, file->TargetPath, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, 0 );
    if (hFile == INVALID_HANDLE_VALUE) return ERROR_FUNCTION_FAILED;

    sz = GetFileSize(hFile, &szHighWord);
    if (sz == INVALID_FILE_SIZE || szHighWord != 0)
    {
        CloseHandle(hFile);
        return ERROR_FUNCTION_FAILED;
    }
    buffer = msi_alloc( sz + 1 );
    if (!buffer)
    {
        CloseHandle(hFile);
        return ERROR_FUNCTION_FAILED;
    }
    bRet = ReadFile(hFile, buffer, sz, &read, NULL);
    CloseHandle(hFile);
    if (!bRet)
    {
        r = ERROR_FUNCTION_FAILED;
        goto done;
    }
    buffer[read] = 0;
    bufferw = strdupAtoW(buffer);
    if (!bufferw)
    {
        r = ERROR_FUNCTION_FAILED;
        goto done;
    }
    info = do_msidbCustomActionTypeScript( package, type, bufferw, target, action );
    r = wait_thread_handle( info );

done:
    msi_free(bufferw);
    msi_free(buffer);
    return r;
}

static UINT HANDLE_CustomType53_54( MSIPACKAGE *package, const WCHAR *source, const WCHAR *target,
                                    INT type, const WCHAR *action )
{
    msi_custom_action_info *info;
    WCHAR *prop;

    TRACE("%s %s\n", debugstr_w(source), debugstr_w(target));

    prop = msi_dup_property( package->db, source );
    if (!prop) return ERROR_SUCCESS;

    info = do_msidbCustomActionTypeScript( package, type, prop, NULL, action );
    msi_free(prop);
    return wait_thread_handle( info );
}

static BOOL action_type_matches_script( UINT type, UINT script )
{
    switch (script)
    {
    case SCRIPT_NONE:
        return FALSE;
    case SCRIPT_INSTALL:
        return !(type & msidbCustomActionTypeCommit) && !(type & msidbCustomActionTypeRollback);
    case SCRIPT_COMMIT:
        return (type & msidbCustomActionTypeCommit);
    case SCRIPT_ROLLBACK:
        return (type & msidbCustomActionTypeRollback);
    default:
        ERR("unhandled script %u\n", script);
    }
    return FALSE;
}

static UINT defer_custom_action( MSIPACKAGE *package, const WCHAR *action, UINT type )
{
    WCHAR *actiondata = msi_dup_property( package->db, action );
    WCHAR *usersid = msi_dup_property( package->db, L"UserSID" );
    WCHAR *prodcode = msi_dup_property( package->db, L"ProductCode" );
    WCHAR *deferred = msi_get_deferred_action( action, actiondata, usersid, prodcode );

    if (!deferred)
    {
        msi_free( actiondata );
        msi_free( usersid );
        msi_free( prodcode );
        return ERROR_OUTOFMEMORY;
    }
    if (type & msidbCustomActionTypeCommit)
    {
        TRACE("deferring commit action\n");
        msi_schedule_action( package, SCRIPT_COMMIT, deferred );
    }
    else if (type & msidbCustomActionTypeRollback)
    {
        TRACE("deferring rollback action\n");
        msi_schedule_action( package, SCRIPT_ROLLBACK, deferred );
    }
    else
    {
        TRACE("deferring install action\n");
        msi_schedule_action( package, SCRIPT_INSTALL, deferred );
    }

    msi_free( actiondata );
    msi_free( usersid );
    msi_free( prodcode );
    msi_free( deferred );
    return ERROR_SUCCESS;
}

UINT ACTION_CustomAction(MSIPACKAGE *package, const WCHAR *action)
{
    UINT rc = ERROR_SUCCESS;
    MSIRECORD *row;
    UINT type;
    const WCHAR *source, *target, *ptr, *deferred_data = NULL;
    WCHAR *deformated = NULL;
    int len;

    /* deferred action: [properties]Action */
    if ((ptr = wcsrchr(action, ']')))
    {
        deferred_data = action;
        action = ptr + 1;
    }

    row = MSI_QueryGetRecord( package->db, L"SELECT * FROM `CustomAction` WHERE `Action` = '%s'", action );
    if (!row)
        return ERROR_FUNCTION_NOT_CALLED;

    type = MSI_RecordGetInteger(row,2);
    source = MSI_RecordGetString(row,3);
    target = MSI_RecordGetString(row,4);

    TRACE("Handling custom action %s (%x %s %s)\n",debugstr_w(action),type,
          debugstr_w(source), debugstr_w(target));

    /* handle some of the deferred actions */
    if (type & msidbCustomActionTypeTSAware)
        FIXME("msidbCustomActionTypeTSAware not handled\n");

    if (type & msidbCustomActionTypeInScript)
    {
        if (type & msidbCustomActionTypeNoImpersonate)
            WARN("msidbCustomActionTypeNoImpersonate not handled\n");

        if (!action_type_matches_script(type, package->script))
        {
            rc = defer_custom_action( package, action, type );
            goto end;
        }
        else
        {
            LPWSTR actiondata = msi_dup_property( package->db, action );

            if (type & msidbCustomActionTypeInScript)
                package->scheduled_action_running = TRUE;

            if (type & msidbCustomActionTypeCommit)
                package->commit_action_running = TRUE;

            if (type & msidbCustomActionTypeRollback)
                package->rollback_action_running = TRUE;

            if (deferred_data)
                set_deferred_action_props(package, deferred_data);
            else if (actiondata)
                msi_set_property( package->db, L"CustomActionData", actiondata, -1 );
            else
                msi_set_property( package->db, L"CustomActionData", L"", -1 );

            msi_free(actiondata);
        }
    }
    else if (!check_execution_scheduling_options(package,action,type))
    {
        rc = ERROR_SUCCESS;
        goto end;
    }

    switch (type & CUSTOM_ACTION_TYPE_MASK)
    {
    case 1: /* DLL file stored in a Binary table stream */
        rc = HANDLE_CustomType1( package, source, target, type, action );
        break;
    case 2: /* EXE file stored in a Binary table stream */
        rc = HANDLE_CustomType2( package, source, target, type, action );
        break;
    case 5:
    case 6: /* JScript/VBScript file stored in a Binary table stream */
        rc = HANDLE_CustomType5_6( package, source, target, type, action );
        break;
    case 7: /* Concurrent install from substorage */
        deformat_string( package, target, &deformated );
        rc = HANDLE_CustomType7( package, source, target, type, action );
        msi_free( deformated );
        break;
    case 17:
        rc = HANDLE_CustomType17( package, source, target, type, action );
        break;
    case 18: /* EXE file installed with package */
        rc = HANDLE_CustomType18( package, source, target, type, action );
        break;
    case 19: /* Error that halts install */
        rc = HANDLE_CustomType19( package, source, target, type, action );
        break;
    case 21: /* JScript/VBScript file installed with the product */
    case 22:
        rc = HANDLE_CustomType21_22( package, source, target, type, action );
        break;
    case 23: /* Installs another package in the source tree */
        deformat_string( package, target, &deformated );
        rc = HANDLE_CustomType23( package, source, deformated, type, action );
        msi_free( deformated );
        break;
    case 34: /* EXE to be run in specified directory */
        rc = HANDLE_CustomType34( package, source, target, type, action );
        break;
    case 35: /* Directory set with formatted text */
        deformat_string( package, target, &deformated );
        MSI_SetTargetPathW( package, source, deformated );
        msi_free( deformated );
        break;
    case 37: /* JScript/VBScript text stored in target column */
    case 38:
        rc = HANDLE_CustomType37_38( package, source, target, type, action );
        break;
    case 50: /* EXE file specified by a property value */
        rc = HANDLE_CustomType50( package, source, target, type, action );
        break;
    case 51: /* Property set with formatted text */
        if (!source) break;
        len = deformat_string( package, target, &deformated );
        rc = msi_set_property( package->db, source, deformated, len );
        if (rc == ERROR_SUCCESS && !wcscmp( source, L"SourceDir" )) msi_reset_source_folders( package );
        msi_free( deformated );
        break;
    case 53: /* JScript/VBScript text specified by a property value */
    case 54:
        rc = HANDLE_CustomType53_54( package, source, target, type, action );
        break;
    default:
        FIXME( "unhandled action type %u (%s %s)\n", type & CUSTOM_ACTION_TYPE_MASK, debugstr_w(source),
               debugstr_w(target) );
    }

end:
    package->scheduled_action_running = FALSE;
    package->commit_action_running = FALSE;
    package->rollback_action_running = FALSE;
    msiobj_release(&row->hdr);
    return rc;
}

void ACTION_FinishCustomActions(const MSIPACKAGE* package)
{
    struct list *item;
    HANDLE *wait_handles;
    unsigned int handle_count, i;
    msi_custom_action_info *info, *cursor;

    while ((item = list_head( &package->RunningActions )))
    {
        MSIRUNNINGACTION *action = LIST_ENTRY( item, MSIRUNNINGACTION, entry );

        list_remove( &action->entry );

        TRACE("waiting for %s\n", debugstr_w( action->name ) );
        msi_dialog_check_messages( action->handle );

        CloseHandle( action->handle );
        msi_free( action->name );
        msi_free( action );
    }

    EnterCriticalSection( &msi_custom_action_cs );

    handle_count = list_count( &msi_pending_custom_actions );
    wait_handles = msi_alloc( handle_count * sizeof(HANDLE) );

    handle_count = 0;
    LIST_FOR_EACH_ENTRY_SAFE( info, cursor, &msi_pending_custom_actions, msi_custom_action_info, entry )
    {
        if (info->package == package )
        {
            if (DuplicateHandle(GetCurrentProcess(), info->handle, GetCurrentProcess(), &wait_handles[handle_count], SYNCHRONIZE, FALSE, 0))
                handle_count++;
        }
    }

    LeaveCriticalSection( &msi_custom_action_cs );

    for (i = 0; i < handle_count; i++)
    {
        msi_dialog_check_messages( wait_handles[i] );
        CloseHandle( wait_handles[i] );
    }
    msi_free( wait_handles );

    EnterCriticalSection( &msi_custom_action_cs );
    LIST_FOR_EACH_ENTRY_SAFE( info, cursor, &msi_pending_custom_actions, msi_custom_action_info, entry )
    {
        if (info->package == package)
            free_custom_action_data( info );
    }
    LeaveCriticalSection( &msi_custom_action_cs );
}

UINT __cdecl s_remote_GetActionInfo(const GUID *guid, int *type, LPWSTR *dll, LPSTR *func, MSIHANDLE *hinst)
{
    msi_custom_action_info *info;

    info = find_action_by_guid(guid);
    if (!info)
        return ERROR_INVALID_DATA;

    *type = info->type;
    *hinst = alloc_msihandle(&info->package->hdr);
    *dll = strdupW(info->source);
    *func = strdupWtoA(info->target);

    return ERROR_SUCCESS;
}
