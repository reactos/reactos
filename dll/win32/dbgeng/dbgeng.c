/*
 * Support for Microsoft Debugging Extension API
 *
 * Copyright (C) 2010 Volodymyr Shcherbyna
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "psapi.h"

#include "initguid.h"
#include "dbgeng.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbgeng);

#ifdef __REACTOS__
extern NTSTATUS WINAPI NtSuspendProcess(HANDLE handle);
extern NTSTATUS WINAPI NtResumeProcess(HANDLE handle);
#define EnumProcessModulesEx(a, b, c, d, e) EnumProcessModules(a, b, c, d)

#define LIST_MODULES_DEFAULT 0x00
#define LIST_MODULES_32BIT 0x01
#endif

struct module_info
{
    DEBUG_MODULE_PARAMETERS params;
    char image_name[MAX_PATH];
};

struct target_process
{
    struct list entry;
    unsigned int pid;
    unsigned int attach_flags;
    HANDLE handle;
    struct
    {
        struct module_info *info;
        unsigned int loaded;
        unsigned int unloaded;
        BOOL initialized;
    } modules;
    ULONG cpu_type;
};

struct debug_client
{
    IDebugClient7 IDebugClient_iface;
    IDebugDataSpaces IDebugDataSpaces_iface;
    IDebugSymbols3 IDebugSymbols3_iface;
    IDebugControl4 IDebugControl4_iface;
    IDebugAdvanced3 IDebugAdvanced3_iface;
    IDebugSystemObjects IDebugSystemObjects_iface;
    LONG refcount;
    ULONG engine_options;
    struct list targets;
    IDebugEventCallbacks *event_callbacks;
};

static struct target_process *debug_client_get_target(struct debug_client *debug_client)
{
    if (list_empty(&debug_client->targets))
        return NULL;

    return LIST_ENTRY(list_head(&debug_client->targets), struct target_process, entry);
}

static HRESULT debug_target_return_string(const char *str, char *buffer, unsigned int buffer_size,
                                          ULONG *size)
{
    unsigned int len = strlen(str), dst_len;

    if (size)
        *size = len + 1;

    if (buffer && buffer_size)
    {
        dst_len = min(len, buffer_size - 1);
        if (dst_len)
            memcpy(buffer, str, dst_len);
        buffer[dst_len] = 0;
    }

    return len < buffer_size ? S_OK : S_FALSE;
}

static WORD debug_target_get_module_machine(struct target_process *target, HMODULE module)
{
    IMAGE_DOS_HEADER dos = { 0 };
    WORD machine = 0;

    ReadProcessMemory(target->handle, module, &dos, sizeof(dos), NULL);
    if (dos.e_magic == IMAGE_DOS_SIGNATURE)
    {
        ReadProcessMemory(target->handle, (const char *)module + dos.e_lfanew + 4 /* PE signature */, &machine,
                sizeof(machine), NULL);
    }

    return machine;
}

static DWORD debug_target_get_module_timestamp(struct target_process *target, HMODULE module)
{
    IMAGE_DOS_HEADER dos = { 0 };
    DWORD timestamp = 0;

    ReadProcessMemory(target->handle, module, &dos, sizeof(dos), NULL);
    if (dos.e_magic == IMAGE_DOS_SIGNATURE)
    {
        ReadProcessMemory(target->handle, (const char *)module + dos.e_lfanew + 4 /* PE signature */ +
                FIELD_OFFSET(IMAGE_FILE_HEADER, TimeDateStamp), &timestamp, sizeof(timestamp), NULL);
    }

    return timestamp;
}

static HRESULT debug_target_init_modules_info(struct target_process *target)
{
    unsigned int i, count;
    HMODULE *modules;
    MODULEINFO info;
    DWORD needed;
    BOOL wow64;
    DWORD filter = LIST_MODULES_DEFAULT;
#ifdef __REACTOS__
    (DWORD)filter;
    (BOOL)wow64;
#endif

    if (target->modules.initialized)
        return S_OK;

    if (!target->handle)
        return E_UNEXPECTED;

    if (sizeof(void*) > sizeof(int) &&
        IsWow64Process(target->handle, &wow64) &&
        wow64)
        filter = LIST_MODULES_32BIT;

    needed = 0;
    EnumProcessModulesEx(target->handle, NULL, 0, &needed, filter);
    if (!needed)
        return E_FAIL;

    count = needed / sizeof(HMODULE);

    if (!(modules = calloc(count, sizeof(*modules))))
        return E_OUTOFMEMORY;

    if (!(target->modules.info = calloc(count, sizeof(*target->modules.info))))
    {
        free(modules);
        return E_OUTOFMEMORY;
    }

    if (EnumProcessModulesEx(target->handle, modules, count * sizeof(*modules), &needed, filter))
    {
        for (i = 0; i < count; ++i)
        {
            if (!GetModuleInformation(target->handle, modules[i], &info, sizeof(info)))
            {
                WARN("Failed to get module information, error %ld.\n", GetLastError());
                continue;
            }

            target->modules.info[i].params.Base = (ULONG_PTR)info.lpBaseOfDll;
            target->modules.info[i].params.Size = info.SizeOfImage;
            target->modules.info[i].params.TimeDateStamp = debug_target_get_module_timestamp(target, modules[i]);

            GetModuleFileNameExA(target->handle, modules[i], target->modules.info[i].image_name,
                    ARRAY_SIZE(target->modules.info[i].image_name));
        }
    }

    target->cpu_type = debug_target_get_module_machine(target, modules[0]);

    free(modules);

    target->modules.loaded = count;
    target->modules.unloaded = 0; /* FIXME */

    target->modules.initialized = TRUE;

    return S_OK;
}

static const struct module_info *debug_target_get_module_info(struct target_process *target, unsigned int i)
{
    if (FAILED(debug_target_init_modules_info(target)))
        return NULL;

    if (i >= target->modules.loaded)
        return NULL;

    return &target->modules.info[i];
}

static const struct module_info *debug_target_get_module_info_by_base(struct target_process *target, ULONG64 base)
{
    unsigned int i;

    if (FAILED(debug_target_init_modules_info(target)))
        return NULL;

    for (i = 0; i < target->modules.loaded; ++i)
    {
        if (target->modules.info[i].params.Base == base)
            return &target->modules.info[i];
    }

    return NULL;
}

static void debug_client_detach_target(struct target_process *target)
{
    NTSTATUS status;

    if (!target->handle)
        return;

    if (target->attach_flags & DEBUG_ATTACH_NONINVASIVE)
    {
        BOOL resume = !(target->attach_flags & DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND);

        if (resume)
        {
            if ((status = NtResumeProcess(target->handle)))
                WARN("Failed to resume process, status %#lx.\n", status);
        }
    }

    CloseHandle(target->handle);
    target->handle = NULL;
}

static struct debug_client *impl_from_IDebugClient(IDebugClient7 *iface)
{
    return CONTAINING_RECORD(iface, struct debug_client, IDebugClient_iface);
}

static struct debug_client *impl_from_IDebugDataSpaces(IDebugDataSpaces *iface)
{
    return CONTAINING_RECORD(iface, struct debug_client, IDebugDataSpaces_iface);
}

static struct debug_client *impl_from_IDebugSymbols3(IDebugSymbols3 *iface)
{
    return CONTAINING_RECORD(iface, struct debug_client, IDebugSymbols3_iface);
}

static struct debug_client *impl_from_IDebugControl4(IDebugControl4 *iface)
{
    return CONTAINING_RECORD(iface, struct debug_client, IDebugControl4_iface);
}

static struct debug_client *impl_from_IDebugAdvanced3(IDebugAdvanced3 *iface)
{
    return CONTAINING_RECORD(iface, struct debug_client, IDebugAdvanced3_iface);
}

static struct debug_client *impl_from_IDebugSystemObjects(IDebugSystemObjects *iface)
{
    return CONTAINING_RECORD(iface, struct debug_client, IDebugSystemObjects_iface);
}

static HRESULT STDMETHODCALLTYPE debugclient_QueryInterface(IDebugClient7 *iface, REFIID riid, void **obj)
{
    struct debug_client *debug_client = impl_from_IDebugClient(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDebugClient) ||
        IsEqualIID(riid, &IID_IDebugClient2) ||
        IsEqualIID(riid, &IID_IDebugClient3) ||
        IsEqualIID(riid, &IID_IDebugClient4) ||
        IsEqualIID(riid, &IID_IDebugClient5) ||
        IsEqualIID(riid, &IID_IDebugClient6) ||
        IsEqualIID(riid, &IID_IDebugClient7) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_IDebugDataSpaces))
    {
        *obj = &debug_client->IDebugDataSpaces_iface;
    }
    else if (IsEqualIID(riid, &IID_IDebugSymbols)
             || IsEqualIID(riid, &IID_IDebugSymbols2)
             || IsEqualIID(riid, &IID_IDebugSymbols3))
    {
        *obj = &debug_client->IDebugSymbols3_iface;
    }
    else if (IsEqualIID(riid, &IID_IDebugControl4)
            || IsEqualIID(riid, &IID_IDebugControl3)
            || IsEqualIID(riid, &IID_IDebugControl2)
            || IsEqualIID(riid, &IID_IDebugControl))
    {
        *obj = &debug_client->IDebugControl4_iface;
    }
    else if (IsEqualIID(riid, &IID_IDebugAdvanced3)
            || IsEqualIID(riid, &IID_IDebugAdvanced2)
            || IsEqualIID(riid, &IID_IDebugAdvanced))
    {
        *obj = &debug_client->IDebugAdvanced3_iface;
    }
    else if (IsEqualIID(riid, &IID_IDebugSystemObjects))
    {
        *obj = &debug_client->IDebugSystemObjects_iface;
    }
    else
    {
        WARN("Unsupported interface %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE debugclient_AddRef(IDebugClient7 *iface)
{
    struct debug_client *debug_client = impl_from_IDebugClient(iface);
    ULONG refcount = InterlockedIncrement(&debug_client->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static void debug_target_free(struct target_process *target)
{
    free(target->modules.info);
    free(target);
}

static ULONG STDMETHODCALLTYPE debugclient_Release(IDebugClient7 *iface)
{
    struct debug_client *debug_client = impl_from_IDebugClient(iface);
    ULONG refcount = InterlockedDecrement(&debug_client->refcount);
    struct target_process *cur, *cur2;

    TRACE("%p, refcount %lu.\n", debug_client, refcount);

    if (!refcount)
    {
        LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &debug_client->targets, struct target_process, entry)
        {
            debug_client_detach_target(cur);
            list_remove(&cur->entry);
            debug_target_free(cur);
        }
        if (debug_client->event_callbacks)
            debug_client->event_callbacks->lpVtbl->Release(debug_client->event_callbacks);
        free(debug_client);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE debugclient_AttachKernel(IDebugClient7 *iface, ULONG flags, const char *options)
{
    FIXME("%p, %#lx, %s stub.\n", iface, flags, debugstr_a(options));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetKernelConnectionOptions(IDebugClient7 *iface, char *buffer,
        ULONG buffer_size, ULONG *options_size)
{
    FIXME("%p, %p, %lu, %p stub.\n", iface, buffer, buffer_size, options_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetKernelConnectionOptions(IDebugClient7 *iface, const char *options)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(options));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_StartProcessServer(IDebugClient7 *iface, ULONG flags, const char *options,
        void *reserved)
{
    FIXME("%p, %#lx, %s, %p stub.\n", iface, flags, debugstr_a(options), reserved);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_ConnectProcessServer(IDebugClient7 *iface, const char *remote_options,
        ULONG64 *server)
{
    FIXME("%p, %s, %p stub.\n", iface, debugstr_a(remote_options), server);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_DisconnectProcessServer(IDebugClient7 *iface, ULONG64 server)
{
    FIXME("%p, %s stub.\n", iface, wine_dbgstr_longlong(server));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetRunningProcessSystemIds(IDebugClient7 *iface, ULONG64 server,
        ULONG *ids, ULONG count, ULONG *actual_count)
{
    FIXME("%p, %s, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(server), ids, count, actual_count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetRunningProcessSystemIdByExecutableName(IDebugClient7 *iface,
        ULONG64 server, const char *exe_name, ULONG flags, ULONG *id)
{
    FIXME("%p, %s, %s, %#lx, %p stub.\n", iface, wine_dbgstr_longlong(server), debugstr_a(exe_name), flags, id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetRunningProcessDescription(IDebugClient7 *iface, ULONG64 server,
        ULONG systemid, ULONG flags, char *exe_name, ULONG exe_name_size, ULONG *actual_exe_name_size,
        char *description, ULONG description_size, ULONG *actual_description_size)
{
    FIXME("%p, %s, %lu, %#lx, %p, %lu, %p, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(server), systemid, flags,
            exe_name, exe_name_size, actual_exe_name_size, description, description_size, actual_description_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_AttachProcess(IDebugClient7 *iface, ULONG64 server, ULONG pid, ULONG flags)
{
    struct debug_client *debug_client = impl_from_IDebugClient(iface);
    struct target_process *process;

    TRACE("%p, %s, %lu, %#lx.\n", iface, wine_dbgstr_longlong(server), pid, flags);

    if (server)
    {
        FIXME("Remote debugging is not supported.\n");
        return E_NOTIMPL;
    }

    if (!(process = calloc(1, sizeof(*process))))
        return E_OUTOFMEMORY;

    process->pid = pid;
    process->attach_flags = flags;

    list_add_head(&debug_client->targets, &process->entry);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE debugclient_CreateProcess(IDebugClient7 *iface, ULONG64 server, char *cmdline,
        ULONG flags)
{
    FIXME("%p, %s, %s, %#lx stub.\n", iface, wine_dbgstr_longlong(server), debugstr_a(cmdline), flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_CreateProcessAndAttach(IDebugClient7 *iface, ULONG64 server, char *cmdline,
        ULONG create_flags, ULONG pid, ULONG attach_flags)
{
    FIXME("%p, %s, %s, %#lx, %lu, %#lx stub.\n", iface, wine_dbgstr_longlong(server), debugstr_a(cmdline), create_flags,
            pid, attach_flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetProcessOptions(IDebugClient7 *iface, ULONG *options)
{
    FIXME("%p, %p stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_AddProcessOptions(IDebugClient7 *iface, ULONG options)
{
    FIXME("%p, %#lx stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_RemoveProcessOptions(IDebugClient7 *iface, ULONG options)
{
    FIXME("%p, %#lx stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetProcessOptions(IDebugClient7 *iface, ULONG options)
{
    FIXME("%p, %#lx stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_OpenDumpFile(IDebugClient7 *iface, const char *filename)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(filename));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_WriteDumpFile(IDebugClient7 *iface, const char *filename, ULONG qualifier)
{
    FIXME("%p, %s, %lu stub.\n", iface, debugstr_a(filename), qualifier);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_ConnectSession(IDebugClient7 *iface, ULONG flags, ULONG history_limit)
{
    FIXME("%p, %#lx, %lu stub.\n", iface, flags, history_limit);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_StartServer(IDebugClient7 *iface, const char *options)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(options));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_OutputServers(IDebugClient7 *iface, ULONG output_control,
        const char *machine, ULONG flags)
{
    FIXME("%p, %lu, %s, %#lx stub.\n", iface, output_control, debugstr_a(machine), flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_TerminateProcesses(IDebugClient7 *iface)
{
    FIXME("%p stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_DetachProcesses(IDebugClient7 *iface)
{
    struct debug_client *debug_client = impl_from_IDebugClient(iface);
    struct target_process *target;

    TRACE("%p.\n", iface);

    LIST_FOR_EACH_ENTRY(target, &debug_client->targets, struct target_process, entry)
    {
        debug_client_detach_target(target);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE debugclient_EndSession(IDebugClient7 *iface, ULONG flags)
{
    FIXME("%p, %#lx stub.\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetExitCode(IDebugClient7 *iface, ULONG *code)
{
    FIXME("%p, %p stub.\n", iface, code);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_DispatchCallbacks(IDebugClient7 *iface, ULONG timeout)
{
    FIXME("%p, %lu stub.\n", iface, timeout);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_ExitDispatch(IDebugClient7 *iface, IDebugClient *client)
{
    FIXME("%p, %p stub.\n", iface, client);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_CreateClient(IDebugClient7 *iface, IDebugClient **client)
{
    FIXME("%p, %p stub.\n", iface, client);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetInputCallbacks(IDebugClient7 *iface, IDebugInputCallbacks **callbacks)
{
    FIXME("%p, %p stub.\n", iface, callbacks);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetInputCallbacks(IDebugClient7 *iface, IDebugInputCallbacks *callbacks)
{
    FIXME("%p, %p stub.\n", iface, callbacks);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetOutputCallbacks(IDebugClient7 *iface, IDebugOutputCallbacks **callbacks)
{
    FIXME("%p, %p stub.\n", iface, callbacks);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetOutputCallbacks(IDebugClient7 *iface, IDebugOutputCallbacks *callbacks)
{
    FIXME("%p, %p stub.\n", iface, callbacks);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetOutputMask(IDebugClient7 *iface, ULONG *mask)
{
    FIXME("%p, %p stub.\n", iface, mask);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetOutputMask(IDebugClient7 *iface, ULONG mask)
{
    FIXME("%p, %#lx stub.\n", iface, mask);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetOtherOutputMask(IDebugClient7 *iface, IDebugClient *client, ULONG *mask)
{
    FIXME("%p, %p, %p stub.\n", iface, client, mask);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetOtherOutputMask(IDebugClient7 *iface, IDebugClient *client, ULONG mask)
{
    FIXME("%p, %p, %#lx stub.\n", iface, client, mask);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetOutputWidth(IDebugClient7 *iface, ULONG *columns)
{
    FIXME("%p, %p stub.\n", iface, columns);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetOutputWidth(IDebugClient7 *iface, ULONG columns)
{
    FIXME("%p, %lu stub.\n", iface, columns);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetOutputLinePrefix(IDebugClient7 *iface, char *buffer, ULONG buffer_size,
        ULONG *prefix_size)
{
    FIXME("%p, %p, %lu, %p stub.\n", iface, buffer, buffer_size, prefix_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetOutputLinePrefix(IDebugClient7 *iface, const char *prefix)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(prefix));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetIdentity(IDebugClient7 *iface, char *buffer, ULONG buffer_size,
        ULONG *identity_size)
{
    FIXME("%p, %p, %lu, %p stub.\n", iface, buffer, buffer_size, identity_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_OutputIdentity(IDebugClient7 *iface, ULONG output_control, ULONG flags,
        const char *format)
{
    FIXME("%p, %lu, %#lx, %s stub.\n", iface, output_control, flags, debugstr_a(format));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetEventCallbacks(IDebugClient7 *iface, IDebugEventCallbacks **callbacks)
{
    struct debug_client *debug_client = impl_from_IDebugClient(iface);

    TRACE("%p, %p.\n", iface, callbacks);

    if (debug_client->event_callbacks)
    {
        *callbacks = debug_client->event_callbacks;
        (*callbacks)->lpVtbl->AddRef(*callbacks);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetEventCallbacks(IDebugClient7 *iface, IDebugEventCallbacks *callbacks)
{
    struct debug_client *debug_client = impl_from_IDebugClient(iface);

    TRACE("%p, %p.\n", iface, callbacks);

    if (debug_client->event_callbacks)
        debug_client->event_callbacks->lpVtbl->Release(debug_client->event_callbacks);
    if ((debug_client->event_callbacks = callbacks))
        debug_client->event_callbacks->lpVtbl->AddRef(debug_client->event_callbacks);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE debugclient_FlushCallbacks(IDebugClient7 *iface)
{
    FIXME("%p stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_WriteDumpFile2(IDebugClient7 *iface, const char *dumpfile, ULONG qualifier,
            ULONG flags, const char *comment)
{
    FIXME("%p, %s, %lu, %#lx, %s.\n", iface, debugstr_a(dumpfile), qualifier, flags, debugstr_a(comment));
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_AddDumpInformationFile(IDebugClient7 *iface, const char *infofile, ULONG type)
{
    FIXME("%p, %s, %lu.\n", iface, debugstr_a(infofile), type);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_EndProcessServer(IDebugClient7 *iface, ULONG64 server)
{
    FIXME("%p, %s.\n", iface, wine_dbgstr_longlong(server));
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_WaitForProcessServerEnd(IDebugClient7 *iface, ULONG timeout)
{
    FIXME("%p, %lu.\n", iface, timeout);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_IsKernelDebuggerEnabled(IDebugClient7 *iface)
{
    FIXME("%p.\n", iface);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_TerminateCurrentProcess(IDebugClient7 *iface)
{
    FIXME("%p.\n", iface);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_DetachCurrentProcess(IDebugClient7 *iface)
{
    FIXME("%p.\n", iface);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_AbandonCurrentProcess(IDebugClient7 *iface)
{
    FIXME("%p.\n", iface);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetRunningProcessSystemIdByExecutableNameWide(IDebugClient7 *iface, ULONG64 server,
            const WCHAR *exename, ULONG flags, ULONG *id)
{
    FIXME("%p, %s, %s, %#lx, %p.\n", iface, wine_dbgstr_longlong(server), debugstr_w(exename), flags, id);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetRunningProcessDescriptionWide(IDebugClient7 *iface, ULONG64 server, ULONG id,
            ULONG flags, WCHAR *exename, ULONG size,  ULONG *actualsize, WCHAR *description, ULONG desc_size, ULONG *actual_desc_size)
{
    FIXME("%p, %s, %lu, %#lx, %s, %lu, %p, %s, %lu, %p.\n", iface, wine_dbgstr_longlong(server), id, flags, debugstr_w(exename), size,
            actualsize, debugstr_w(description), desc_size, actual_desc_size );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_CreateProcessWide(IDebugClient7 *iface, ULONG64 server, WCHAR *commandline, ULONG flags)
{
    FIXME("%p, %s, %s, %#lx.\n", iface, wine_dbgstr_longlong(server), debugstr_w(commandline), flags);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_CreateProcessAndAttachWide(IDebugClient7 *iface, ULONG64 server, WCHAR *commandline,
            ULONG flags, ULONG processid, ULONG attachflags)
{
    FIXME("%p, %s, %s, %#lx, %lu, %#lx.\n", iface, wine_dbgstr_longlong(server), debugstr_w(commandline), flags, processid, attachflags);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_OpenDumpFileWide(IDebugClient7 *iface, const WCHAR *filename, ULONG64 handle)
{
    FIXME("%p, %s, %s.\n", iface, debugstr_w(filename), wine_dbgstr_longlong(handle));
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_WriteDumpFileWide(IDebugClient7 *iface, const WCHAR *filename, ULONG64 handle,
            ULONG qualifier, ULONG flags, const WCHAR *comment)
{
    FIXME("%p, %s, %s, %lu, %#lx, %s.\n", iface, debugstr_w(filename), wine_dbgstr_longlong(handle),
                qualifier, flags, debugstr_w(comment));
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_AddDumpInformationFileWide(IDebugClient7 *iface, const WCHAR *filename,
            ULONG64 handle, ULONG type)
{
    FIXME("%p, %s, %s, %lu.\n", iface, debugstr_w(filename), wine_dbgstr_longlong(handle), type);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetNumberDumpFiles(IDebugClient7 *iface, ULONG *count)
{
    FIXME("%p, %p.\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetDumpFile(IDebugClient7 *iface, ULONG index, char *buffer, ULONG buf_size,
            ULONG *name_size, ULONG64 *handle, ULONG *type)
{
    FIXME("%p, %lu, %p, %lu, %p, %p, %p.\n", iface, index, buffer, buf_size, name_size, handle, type);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetDumpFileWide(IDebugClient7 *iface, ULONG index, WCHAR *buffer, ULONG buf_size,
            ULONG *name_size, ULONG64 *handle, ULONG *type)
{
    FIXME("%p, %lu, %p, %lu, %p, %p, %p.\n", iface, index, buffer, buf_size, name_size, handle, type);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_AttachKernelWide(IDebugClient7 *iface, ULONG flags, const WCHAR *options)
{
    FIXME("%p, %#lx, %s.\n", iface, flags, debugstr_w(options));
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetKernelConnectionOptionsWide(IDebugClient7 *iface, WCHAR *buffer,
                ULONG buf_size, ULONG *size)
{
    FIXME("%p, %p, %lu, %p.\n", iface, buffer, buf_size, size);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetKernelConnectionOptionsWide(IDebugClient7 *iface, const WCHAR *options)
{
    FIXME("%p, %p.\n", iface, options);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_StartProcessServerWide(IDebugClient7 *iface, ULONG flags, const WCHAR *options, void *reserved)
{
    FIXME("%p, %#lx, %s, %p.\n", iface, flags, debugstr_w(options), reserved);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_ConnectProcessServerWide(IDebugClient7 *iface, const WCHAR *options, ULONG64 *server)
{
    FIXME("%p, %s, %p.\n", iface, debugstr_w(options), server);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_StartServerWide(IDebugClient7 *iface, const WCHAR *options)
{
    FIXME("%p, %s.\n", iface, debugstr_w(options));
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_OutputServersWide(IDebugClient7 *iface, ULONG control, const WCHAR *machine, ULONG flags)
{
    FIXME("%p, %lu, %s, %#lx.\n", iface, control, debugstr_w(machine), flags);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetOutputCallbacksWide(IDebugClient7 *iface, IDebugOutputCallbacksWide **callbacks)
{
    FIXME("%p, %p.\n", iface, callbacks);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetOutputCallbacksWide(IDebugClient7 *iface, IDebugOutputCallbacksWide *callbacks)
{
    FIXME("%p, %p.\n", iface, callbacks);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetOutputLinePrefixWide(IDebugClient7 *iface, WCHAR *buffer, ULONG buf_size, ULONG *size)
{
    FIXME("%p, %p, %lu, %p.\n", iface, buffer, buf_size, size);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetOutputLinePrefixWide(IDebugClient7 *iface, const WCHAR *prefix)
{
    FIXME("%p, %s.\n", iface, debugstr_w(prefix));
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetIdentityWide(IDebugClient7 *iface, WCHAR *buffer, ULONG buf_size, ULONG *identity)
{
    FIXME("%p, %p, %lu, %p.\n", iface, buffer, buf_size, identity);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_OutputIdentityWide(IDebugClient7 *iface, ULONG control, ULONG flags, const WCHAR *format)
{
    FIXME("%p, %ld, %#lx, %s.\n", iface, control, flags, debugstr_w(format));
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetEventCallbacksWide(IDebugClient7 *iface, IDebugEventCallbacksWide **callbacks)
{
    FIXME("%p, %p .\n", iface, callbacks);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetEventCallbacksWide(IDebugClient7 *iface, IDebugEventCallbacksWide *callbacks)
{
    FIXME("%p .\n", iface);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_CreateProcess2(IDebugClient7 *iface, ULONG64 server, char *command, void *options,
            ULONG buf_size, const char *initial, const char *environment)
{
    FIXME("%p, %s, %s, %p, %ld, %s, %s.\n", iface, wine_dbgstr_longlong(server), debugstr_a(command), options,
            buf_size, debugstr_a(initial), debugstr_a(environment));
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_CreateProcess2Wide(IDebugClient7 *iface, ULONG64 server, WCHAR *command, void *options,
            ULONG size, const WCHAR *initial, const WCHAR *environment)
{
    FIXME("%p, %s, %s, %p, %ld, %s, %s.\n", iface, wine_dbgstr_longlong(server), debugstr_w(command), options,
            size, debugstr_w(initial), debugstr_w(environment));
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_CreateProcessAndAttach2(IDebugClient7 *iface, ULONG64 server, char *command,
            void *options, ULONG buf_size, const char *initial, const char *environment, ULONG processid, ULONG flags)
{
    FIXME("%p, %s, %s, %p, %ld, %s, %s, %ld, %#lx.\n", iface, wine_dbgstr_longlong(server), debugstr_a(command), options,
            buf_size, debugstr_a(initial), debugstr_a(environment), processid, flags);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_CreateProcessAndAttach2Wide(IDebugClient7 *iface, ULONG64 server, WCHAR *command,
            void *buffer, ULONG buf_size, const WCHAR *initial, const WCHAR *environment, ULONG processid, ULONG flags)
{
    FIXME("%p %s, %s, %p, %ld, %s, %s, %ld, %#lx.\n", iface, wine_dbgstr_longlong(server), debugstr_w(command), buffer,
            buf_size, debugstr_w(initial), debugstr_w(environment), processid, flags);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_PushOutputLinePrefix(IDebugClient7 *iface, const char *prefix, ULONG64 *handle)
{
    FIXME("%p, %p.\n", iface, handle);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_PushOutputLinePrefixWide(IDebugClient7 *iface, const WCHAR *prefix, ULONG64 *handle)
{
    FIXME("%p, %p.\n", iface, handle);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_PopOutputLinePrefix(IDebugClient7 *iface, ULONG64 handle)
{
    FIXME("%p, %s.\n", iface, wine_dbgstr_longlong(handle));
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetNumberInputCallbacks(IDebugClient7 *iface, ULONG *count)
{
    FIXME("%p, %p.\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetNumberOutputCallbacks(IDebugClient7 *iface, ULONG *count)
{
    FIXME("%p, %p.\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetNumberEventCallbacks(IDebugClient7 *iface, ULONG flags, ULONG *count)
{
    FIXME("%p, %#lx, %p.\n", iface, flags, count);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetQuitLockString(IDebugClient7 *iface, char *buffer, ULONG buf_size, ULONG *size)
{
    FIXME("%p, %s, %ld, %p.\n", iface, debugstr_a(buffer), buf_size, size);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetQuitLockString(IDebugClient7 *iface, char *string)
{
    FIXME("%p, %s.\n", iface, debugstr_a(string));
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetQuitLockStringWide(IDebugClient7 *iface, WCHAR *buffer, ULONG buf_size, ULONG *size)
{
    FIXME("%p, %s, %ld, %p.\n", iface, debugstr_w(buffer), buf_size, size);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetQuitLockStringWide(IDebugClient7 *iface, const WCHAR *string)
{
    FIXME("%p, %s.\n", iface, debugstr_w(string));
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetEventContextCallbacks(IDebugClient7 *iface, IDebugEventContextCallbacks *callbacks)
{
    FIXME("%p, %p.\n", iface, callbacks);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetClientContext(IDebugClient7 *iface, void *context, ULONG size)
{
    FIXME("%p, %p, %ld.\n", iface, context, size);
    return E_NOTIMPL;
}

static const IDebugClient7Vtbl debugclientvtbl =
{
    debugclient_QueryInterface,
    debugclient_AddRef,
    debugclient_Release,
    debugclient_AttachKernel,
    debugclient_GetKernelConnectionOptions,
    debugclient_SetKernelConnectionOptions,
    debugclient_StartProcessServer,
    debugclient_ConnectProcessServer,
    debugclient_DisconnectProcessServer,
    debugclient_GetRunningProcessSystemIds,
    debugclient_GetRunningProcessSystemIdByExecutableName,
    debugclient_GetRunningProcessDescription,
    debugclient_AttachProcess,
    debugclient_CreateProcess,
    debugclient_CreateProcessAndAttach,
    debugclient_GetProcessOptions,
    debugclient_AddProcessOptions,
    debugclient_RemoveProcessOptions,
    debugclient_SetProcessOptions,
    debugclient_OpenDumpFile,
    debugclient_WriteDumpFile,
    debugclient_ConnectSession,
    debugclient_StartServer,
    debugclient_OutputServers,
    debugclient_TerminateProcesses,
    debugclient_DetachProcesses,
    debugclient_EndSession,
    debugclient_GetExitCode,
    debugclient_DispatchCallbacks,
    debugclient_ExitDispatch,
    debugclient_CreateClient,
    debugclient_GetInputCallbacks,
    debugclient_SetInputCallbacks,
    debugclient_GetOutputCallbacks,
    debugclient_SetOutputCallbacks,
    debugclient_GetOutputMask,
    debugclient_SetOutputMask,
    debugclient_GetOtherOutputMask,
    debugclient_SetOtherOutputMask,
    debugclient_GetOutputWidth,
    debugclient_SetOutputWidth,
    debugclient_GetOutputLinePrefix,
    debugclient_SetOutputLinePrefix,
    debugclient_GetIdentity,
    debugclient_OutputIdentity,
    debugclient_GetEventCallbacks,
    debugclient_SetEventCallbacks,
    debugclient_FlushCallbacks,
    /* IDebugClient2 */
    debugclient_WriteDumpFile2,
    debugclient_AddDumpInformationFile,
    debugclient_EndProcessServer,
    debugclient_WaitForProcessServerEnd,
    debugclient_IsKernelDebuggerEnabled,
    debugclient_TerminateCurrentProcess,
    debugclient_DetachCurrentProcess,
    debugclient_AbandonCurrentProcess,
    /* IDebugClient3 */
    debugclient_GetRunningProcessSystemIdByExecutableNameWide,
    debugclient_GetRunningProcessDescriptionWide,
    debugclient_CreateProcessWide,
    debugclient_CreateProcessAndAttachWide,
    /* IDebugClient4 */
    debugclient_OpenDumpFileWide,
    debugclient_WriteDumpFileWide,
    debugclient_AddDumpInformationFileWide,
    debugclient_GetNumberDumpFiles,
    debugclient_GetDumpFile,
    debugclient_GetDumpFileWide,
    /* IDebugClient5 */
    debugclient_AttachKernelWide,
    debugclient_GetKernelConnectionOptionsWide,
    debugclient_SetKernelConnectionOptionsWide,
    debugclient_StartProcessServerWide,
    debugclient_ConnectProcessServerWide,
    debugclient_StartServerWide,
    debugclient_OutputServersWide,
    debugclient_GetOutputCallbacksWide,
    debugclient_SetOutputCallbacksWide,
    debugclient_GetOutputLinePrefixWide,
    debugclient_SetOutputLinePrefixWide,
    debugclient_GetIdentityWide,
    debugclient_OutputIdentityWide,
    debugclient_GetEventCallbacksWide,
    debugclient_SetEventCallbacksWide,
    debugclient_CreateProcess2,
    debugclient_CreateProcess2Wide,
    debugclient_CreateProcessAndAttach2,
    debugclient_CreateProcessAndAttach2Wide,
    debugclient_PushOutputLinePrefix,
    debugclient_PushOutputLinePrefixWide,
    debugclient_PopOutputLinePrefix,
    debugclient_GetNumberInputCallbacks,
    debugclient_GetNumberOutputCallbacks,
    debugclient_GetNumberEventCallbacks,
    debugclient_GetQuitLockString,
    debugclient_SetQuitLockString,
    debugclient_GetQuitLockStringWide,
    debugclient_SetQuitLockStringWide,
    /* IDebugClient6 */
    debugclient_SetEventContextCallbacks,
    /* IDebugClient7 */
    debugclient_SetClientContext,
};

static HRESULT STDMETHODCALLTYPE debugdataspaces_QueryInterface(IDebugDataSpaces *iface, REFIID riid, void **obj)
{
    struct debug_client *debug_client = impl_from_IDebugDataSpaces(iface);
    return IUnknown_QueryInterface(&debug_client->IDebugClient_iface, riid, obj);
}

static ULONG STDMETHODCALLTYPE debugdataspaces_AddRef(IDebugDataSpaces *iface)
{
    struct debug_client *debug_client = impl_from_IDebugDataSpaces(iface);
    return IUnknown_AddRef(&debug_client->IDebugClient_iface);
}

static ULONG STDMETHODCALLTYPE debugdataspaces_Release(IDebugDataSpaces *iface)
{
    struct debug_client *debug_client = impl_from_IDebugDataSpaces(iface);
    return IUnknown_Release(&debug_client->IDebugClient_iface);
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadVirtual(IDebugDataSpaces *iface, ULONG64 offset, void *buffer,
        ULONG buffer_size, ULONG *read_len)
{
    struct debug_client *debug_client = impl_from_IDebugDataSpaces(iface);
    static struct target_process *target;
    HRESULT hr = S_OK;
    SIZE_T length;

    TRACE("%p, %s, %p, %lu, %p.\n", iface, wine_dbgstr_longlong(offset), buffer, buffer_size, read_len);

    if (!(target = debug_client_get_target(debug_client)))
        return E_UNEXPECTED;

    if (ReadProcessMemory(target->handle, (const void *)(ULONG_PTR)offset, buffer, buffer_size, &length))
    {
        if (read_len)
            *read_len = length;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        WARN("Failed to read process memory %#lx.\n", hr);
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_WriteVirtual(IDebugDataSpaces *iface, ULONG64 offset, void *buffer,
        ULONG buffer_size, ULONG *written)
{
    FIXME("%p, %s, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(offset), buffer, buffer_size, written);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_SearchVirtual(IDebugDataSpaces *iface, ULONG64 offset, ULONG64 length,
        void *pattern, ULONG pattern_size, ULONG pattern_granularity, ULONG64 *ret_offset)
{
    FIXME("%p, %s, %s, %p, %lu, %lu, %p stub.\n", iface, wine_dbgstr_longlong(offset), wine_dbgstr_longlong(length),
            pattern, pattern_size, pattern_granularity, ret_offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadVirtualUncached(IDebugDataSpaces *iface, ULONG64 offset,
        void *buffer, ULONG buffer_size, ULONG *read_len)
{
    FIXME("%p, %s, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(offset), buffer, buffer_size, read_len);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_WriteVirtualUncached(IDebugDataSpaces *iface, ULONG64 offset,
        void *buffer, ULONG buffer_size, ULONG *written)
{
    FIXME("%p, %s, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(offset), buffer, buffer_size, written);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadPointersVirtual(IDebugDataSpaces *iface, ULONG count,
        ULONG64 offset, ULONG64 *pointers)
{
    FIXME("%p, %lu, %s, %p stub.\n", iface, count, wine_dbgstr_longlong(offset), pointers);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_WritePointersVirtual(IDebugDataSpaces *iface, ULONG count,
        ULONG64 offset, ULONG64 *pointers)
{
    FIXME("%p, %lu, %s, %p stub.\n", iface, count, wine_dbgstr_longlong(offset), pointers);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadPhysical(IDebugDataSpaces *iface, ULONG64 offset, void *buffer,
        ULONG buffer_size, ULONG *read_len)
{
    FIXME("%p, %s, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(offset), buffer, buffer_size, read_len);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_WritePhysical(IDebugDataSpaces *iface, ULONG64 offset, void *buffer,
        ULONG buffer_size, ULONG *written)
{
    FIXME("%p, %s, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(offset), buffer, buffer_size, written);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadControl(IDebugDataSpaces *iface, ULONG processor, ULONG64 offset,
        void *buffer, ULONG buffer_size, ULONG *read_len)
{
    FIXME("%p, %lu, %s, %p, %lu, %p stub.\n", iface, processor, wine_dbgstr_longlong(offset), buffer, buffer_size, read_len);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_WriteControl(IDebugDataSpaces *iface, ULONG processor, ULONG64 offset,
        void *buffer, ULONG buffer_size, ULONG *written)
{
    FIXME("%p, %lu, %s, %p, %lu, %p stub.\n", iface, processor, wine_dbgstr_longlong(offset), buffer, buffer_size, written);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadIo(IDebugDataSpaces *iface, ULONG type, ULONG bus_number,
        ULONG address_space, ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *read_len)
{
    FIXME("%p, %lu, %lu, %lu, %s, %p, %lu, %p stub.\n", iface, type, bus_number, address_space, wine_dbgstr_longlong(offset),
            buffer, buffer_size, read_len);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_WriteIo(IDebugDataSpaces *iface, ULONG type, ULONG bus_number,
        ULONG address_space, ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *written)
{
    FIXME("%p, %lu, %lu, %lu, %s, %p, %lu, %p stub.\n", iface, type, bus_number, address_space, wine_dbgstr_longlong(offset),
            buffer, buffer_size, written);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadMsr(IDebugDataSpaces *iface, ULONG msr, ULONG64 *value)
{
    FIXME("%p, %lu, %p stub.\n", iface, msr, value);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_WriteMsr(IDebugDataSpaces *iface, ULONG msr, ULONG64 value)
{
    FIXME("%p, %lu, %s stub.\n", iface, msr, wine_dbgstr_longlong(value));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadBusData(IDebugDataSpaces *iface, ULONG data_type,
        ULONG bus_number, ULONG slot_number, ULONG offset, void *buffer, ULONG buffer_size, ULONG *read_len)
{
    FIXME("%p, %lu, %lu, %lu, %lu, %p, %lu, %p stub.\n", iface, data_type, bus_number, slot_number, offset, buffer,
            buffer_size, read_len);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_WriteBusData(IDebugDataSpaces *iface, ULONG data_type,
        ULONG bus_number, ULONG slot_number, ULONG offset, void *buffer, ULONG buffer_size, ULONG *written)
{
    FIXME("%p, %lu, %lu, %lu, %lu, %p, %lu, %p stub.\n", iface, data_type, bus_number, slot_number, offset, buffer,
            buffer_size, written);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_CheckLowMemory(IDebugDataSpaces *iface)
{
    FIXME("%p stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadDebuggerData(IDebugDataSpaces *iface, ULONG index, void *buffer,
        ULONG buffer_size, ULONG *data_size)
{
    FIXME("%p, %lu, %p, %lu, %p stub.\n", iface, index, buffer, buffer_size, data_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadProcessorSystemData(IDebugDataSpaces *iface, ULONG processor,
        ULONG index, void *buffer, ULONG buffer_size, ULONG *data_size)
{
    FIXME("%p, %lu, %lu, %p, %lu, %p stub.\n", iface, processor, index, buffer, buffer_size, data_size);

    return E_NOTIMPL;
}

static const IDebugDataSpacesVtbl debugdataspacesvtbl =
{
    debugdataspaces_QueryInterface,
    debugdataspaces_AddRef,
    debugdataspaces_Release,
    debugdataspaces_ReadVirtual,
    debugdataspaces_WriteVirtual,
    debugdataspaces_SearchVirtual,
    debugdataspaces_ReadVirtualUncached,
    debugdataspaces_WriteVirtualUncached,
    debugdataspaces_ReadPointersVirtual,
    debugdataspaces_WritePointersVirtual,
    debugdataspaces_ReadPhysical,
    debugdataspaces_WritePhysical,
    debugdataspaces_ReadControl,
    debugdataspaces_WriteControl,
    debugdataspaces_ReadIo,
    debugdataspaces_WriteIo,
    debugdataspaces_ReadMsr,
    debugdataspaces_WriteMsr,
    debugdataspaces_ReadBusData,
    debugdataspaces_WriteBusData,
    debugdataspaces_CheckLowMemory,
    debugdataspaces_ReadDebuggerData,
    debugdataspaces_ReadProcessorSystemData,
};

static HRESULT STDMETHODCALLTYPE debugsymbols_QueryInterface(IDebugSymbols3 *iface, REFIID riid, void **obj)
{
    struct debug_client *debug_client = impl_from_IDebugSymbols3(iface);
    return IUnknown_QueryInterface(&debug_client->IDebugClient_iface, riid, obj);
}

static ULONG STDMETHODCALLTYPE debugsymbols_AddRef(IDebugSymbols3 *iface)
{
    struct debug_client *debug_client = impl_from_IDebugSymbols3(iface);
    return IUnknown_AddRef(&debug_client->IDebugClient_iface);
}

static ULONG STDMETHODCALLTYPE debugsymbols_Release(IDebugSymbols3 *iface)
{
    struct debug_client *debug_client = impl_from_IDebugSymbols3(iface);
    return IUnknown_Release(&debug_client->IDebugClient_iface);
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolOptions(IDebugSymbols3 *iface, ULONG *options)
{
    FIXME("%p, %p stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_AddSymbolOptions(IDebugSymbols3 *iface, ULONG options)
{
    FIXME("%p, %#lx stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_RemoveSymbolOptions(IDebugSymbols3 *iface, ULONG options)
{
    FIXME("%p, %#lx stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_SetSymbolOptions(IDebugSymbols3 *iface, ULONG options)
{
    FIXME("%p, %#lx stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetNameByOffset(IDebugSymbols3 *iface, ULONG64 offset, char *buffer,
        ULONG buffer_size, ULONG *name_size, ULONG64 *displacement)
{
    FIXME("%p, %s, %p, %lu, %p, %p stub.\n", iface, wine_dbgstr_longlong(offset), buffer, buffer_size,
            name_size, displacement);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetOffsetByName(IDebugSymbols3 *iface, const char *symbol,
        ULONG64 *offset)
{
    FIXME("%p, %s, %p stub.\n", iface, debugstr_a(symbol), offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetNearNameByOffset(IDebugSymbols3 *iface, ULONG64 offset, LONG delta,
        char *buffer, ULONG buffer_size, ULONG *name_size, ULONG64 *displacement)
{
    FIXME("%p, %s, %ld, %p, %lu, %p, %p stub.\n", iface, wine_dbgstr_longlong(offset), delta, buffer, buffer_size,
            name_size, displacement);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetLineByOffset(IDebugSymbols3 *iface, ULONG64 offset, ULONG *line,
        char *buffer, ULONG buffer_size, ULONG *file_size, ULONG64 *displacement)
{
    FIXME("%p, %s, %p, %p, %lu, %p, %p stub.\n", iface, wine_dbgstr_longlong(offset), line, buffer, buffer_size,
            file_size, displacement);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetOffsetByLine(IDebugSymbols3 *iface, ULONG line, const char *file,
        ULONG64 *offset)
{
    FIXME("%p, %lu, %s, %p stub.\n", iface, line, debugstr_a(file), offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetNumberModules(IDebugSymbols3 *iface, ULONG *loaded, ULONG *unloaded)
{
    struct debug_client *debug_client = impl_from_IDebugSymbols3(iface);
    static struct target_process *target;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", iface, loaded, unloaded);

    if (!(target = debug_client_get_target(debug_client)))
        return E_UNEXPECTED;

    if (FAILED(hr = debug_target_init_modules_info(target)))
        return hr;

    *loaded = target->modules.loaded;
    *unloaded = target->modules.unloaded;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetModuleByIndex(IDebugSymbols3 *iface, ULONG index, ULONG64 *base)
{
    struct debug_client *debug_client = impl_from_IDebugSymbols3(iface);
    const struct module_info *info;
    struct target_process *target;

    TRACE("%p, %lu, %p.\n", iface, index, base);

    if (!(target = debug_client_get_target(debug_client)))
        return E_UNEXPECTED;

    if (!(info = debug_target_get_module_info(target, index)))
        return E_INVALIDARG;

    *base = info->params.Base;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetModuleByModuleName(IDebugSymbols3 *iface, const char *name,
        ULONG start_index, ULONG *index, ULONG64 *base)
{
    FIXME("%p, %s, %lu, %p, %p stub.\n", iface, debugstr_a(name), start_index, index, base);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetModuleByOffset(IDebugSymbols3 *iface, ULONG64 offset,
        ULONG start_index, ULONG *index, ULONG64 *base)
{
    struct debug_client *debug_client = impl_from_IDebugSymbols3(iface);
    static struct target_process *target;
    const struct module_info *info;

    TRACE("%p, %s, %lu, %p, %p.\n", iface, wine_dbgstr_longlong(offset), start_index, index, base);

    if (!(target = debug_client_get_target(debug_client)))
        return E_UNEXPECTED;

    while ((info = debug_target_get_module_info(target, start_index)))
    {
        if (offset >= info->params.Base && offset < info->params.Base + info->params.Size)
        {
            if (index)
                *index = start_index;
            if (base)
                *base = info->params.Base;
            return S_OK;
        }

        start_index++;
    }

    return E_INVALIDARG;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetModuleNames(IDebugSymbols3 *iface, ULONG index, ULONG64 base,
        char *image_name, ULONG image_name_buffer_size, ULONG *image_name_size, char *module_name,
        ULONG module_name_buffer_size, ULONG *module_name_size, char *loaded_image_name,
        ULONG loaded_image_name_buffer_size, ULONG *loaded_image_size)
{
    FIXME("%p, %lu, %s, %p, %lu, %p, %p, %lu, %p, %p, %lu, %p stub.\n", iface, index, wine_dbgstr_longlong(base),
            image_name, image_name_buffer_size, image_name_size, module_name, module_name_buffer_size,
            module_name_size, loaded_image_name, loaded_image_name_buffer_size, loaded_image_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetModuleParameters(IDebugSymbols3 *iface, ULONG count, ULONG64 *bases,
        ULONG start, DEBUG_MODULE_PARAMETERS *params)
{
    struct debug_client *debug_client = impl_from_IDebugSymbols3(iface);
    const struct module_info *info;
    struct target_process *target;
    unsigned int i;

    TRACE("%p, %lu, %p, %lu, %p.\n", iface, count, bases, start, params);

    if (!(target = debug_client_get_target(debug_client)))
        return E_UNEXPECTED;

    if (bases)
    {
        for (i = 0; i < count; ++i)
        {
            if ((info = debug_target_get_module_info_by_base(target, bases[i])))
            {
                params[i] = info->params;
            }
            else
            {
                memset(&params[i], 0, sizeof(*params));
                params[i].Base = DEBUG_INVALID_OFFSET;
            }
        }
    }
    else
    {
        for (i = start; i < start + count; ++i)
        {
            if (!(info = debug_target_get_module_info(target, i)))
                return E_INVALIDARG;
            params[i] = info->params;
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolModule(IDebugSymbols3 *iface, const char *symbol, ULONG64 *base)
{
    FIXME("%p, %s, %p stub.\n", iface, debugstr_a(symbol), base);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetTypeName(IDebugSymbols3 *iface, ULONG64 base, ULONG type_id,
        char *buffer, ULONG buffer_size, ULONG *name_size)
{
    FIXME("%p, %s, %lu, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(base), type_id, buffer,
            buffer_size, name_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetTypeId(IDebugSymbols3 *iface, ULONG64 base, const char *name,
        ULONG *type_id)
{
    FIXME("%p, %s, %s, %p stub.\n", iface, wine_dbgstr_longlong(base), debugstr_a(name), type_id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetTypeSize(IDebugSymbols3 *iface, ULONG64 base, ULONG type_id,
        ULONG *size)
{
    FIXME("%p, %s, %lu, %p stub.\n", iface, wine_dbgstr_longlong(base), type_id, size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetFieldOffset(IDebugSymbols3 *iface, ULONG64 base, ULONG type_id,
        const char *field, ULONG *offset)
{
    FIXME("%p, %s, %lu, %s, %p stub.\n", iface, wine_dbgstr_longlong(base), type_id, debugstr_a(field), offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolTypeId(IDebugSymbols3 *iface, const char *symbol, ULONG *type_id,
        ULONG64 *base)
{
    FIXME("%p, %s, %p, %p stub.\n", iface, debugstr_a(symbol), type_id, base);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetOffsetTypeId(IDebugSymbols3 *iface, ULONG64 offset, ULONG *type_id,
        ULONG64 *base)
{
    FIXME("%p, %s, %p, %p stub.\n", iface, wine_dbgstr_longlong(offset), type_id, base);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_ReadTypedDataVirtual(IDebugSymbols3 *iface, ULONG64 offset, ULONG64 base,
        ULONG type_id, void *buffer, ULONG buffer_size, ULONG *read_len)
{
    FIXME("%p, %s, %s, %lu, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(offset), wine_dbgstr_longlong(base),
            type_id, buffer, buffer_size, read_len);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_WriteTypedDataVirtual(IDebugSymbols3 *iface, ULONG64 offset, ULONG64 base,
        ULONG type_id, void *buffer, ULONG buffer_size, ULONG *written)
{
    FIXME("%p, %s, %s, %lu, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(offset), wine_dbgstr_longlong(base),
            type_id, buffer, buffer_size, written);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_OutputTypedDataVirtual(IDebugSymbols3 *iface, ULONG output_control,
        ULONG64 offset, ULONG64 base, ULONG type_id, ULONG flags)
{
    FIXME("%p, %#lx, %s, %s, %lu, %#lx stub.\n", iface, output_control, wine_dbgstr_longlong(offset),
            wine_dbgstr_longlong(base), type_id, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_ReadTypedDataPhysical(IDebugSymbols3 *iface, ULONG64 offset, ULONG64 base,
        ULONG type_id, void *buffer, ULONG buffer_size, ULONG *read_len)
{
    FIXME("%p, %s, %s, %lu, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(offset), wine_dbgstr_longlong(base),
            type_id, buffer, buffer_size, read_len);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_WriteTypedDataPhysical(IDebugSymbols3 *iface, ULONG64 offset,
        ULONG64 base, ULONG type_id, void *buffer, ULONG buffer_size, ULONG *written)
{
    FIXME("%p, %s, %s, %lu, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(offset), wine_dbgstr_longlong(base),
            type_id, buffer, buffer_size, written);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_OutputTypedDataPhysical(IDebugSymbols3 *iface, ULONG output_control,
        ULONG64 offset, ULONG64 base, ULONG type_id, ULONG flags)
{
    FIXME("%p, %#lx, %s, %s, %lu, %#lx stub.\n", iface, output_control, wine_dbgstr_longlong(offset),
            wine_dbgstr_longlong(base), type_id, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetScope(IDebugSymbols3 *iface, ULONG64 *instr_offset,
        DEBUG_STACK_FRAME *frame, void *scope_context, ULONG scope_context_size)
{
    FIXME("%p, %p, %p, %p, %lu stub.\n", iface, instr_offset, frame, scope_context, scope_context_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_SetScope(IDebugSymbols3 *iface, ULONG64 instr_offset,
        DEBUG_STACK_FRAME *frame, void *scope_context, ULONG scope_context_size)
{
    FIXME("%p, %s, %p, %p, %lu stub.\n", iface, wine_dbgstr_longlong(instr_offset), frame, scope_context,
            scope_context_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_ResetScope(IDebugSymbols3 *iface)
{
    FIXME("%p stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetScopeSymbolGroup(IDebugSymbols3 *iface, ULONG flags,
        IDebugSymbolGroup *update, IDebugSymbolGroup **symbols)
{
    FIXME("%p, %#lx, %p, %p stub.\n", iface, flags, update, symbols);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_CreateSymbolGroup(IDebugSymbols3 *iface, IDebugSymbolGroup **group)
{
    FIXME("%p, %p stub.\n", iface, group);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_StartSymbolMatch(IDebugSymbols3 *iface, const char *pattern,
        ULONG64 *handle)
{
    FIXME("%p, %s, %p stub.\n", iface, pattern, handle);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetNextSymbolMatch(IDebugSymbols3 *iface, ULONG64 handle, char *buffer,
        ULONG buffer_size, ULONG *match_size, ULONG64 *offset)
{
    FIXME("%p, %s, %p, %lu, %p, %p stub.\n", iface, wine_dbgstr_longlong(handle), buffer, buffer_size, match_size, offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_EndSymbolMatch(IDebugSymbols3 *iface, ULONG64 handle)
{
    FIXME("%p, %s stub.\n", iface, wine_dbgstr_longlong(handle));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_Reload(IDebugSymbols3 *iface, const char *path)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolPath(IDebugSymbols3 *iface, char *buffer, ULONG buffer_size,
        ULONG *path_size)
{
    FIXME("%p, %p, %lu, %p stub.\n", iface, buffer, buffer_size, path_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_SetSymbolPath(IDebugSymbols3 *iface, const char *path)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_AppendSymbolPath(IDebugSymbols3 *iface, const char *path)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetImagePath(IDebugSymbols3 *iface, char *buffer, ULONG buffer_size,
        ULONG *path_size)
{
    FIXME("%p, %p, %lu, %p stub.\n", iface, buffer, buffer_size, path_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_SetImagePath(IDebugSymbols3 *iface, const char *path)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_AppendImagePath(IDebugSymbols3 *iface, const char *path)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSourcePath(IDebugSymbols3 *iface, char *buffer, ULONG buffer_size,
        ULONG *path_size)
{
    FIXME("%p, %p, %lu, %p stub.\n", iface, buffer, buffer_size, path_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSourcePathElement(IDebugSymbols3 *iface, ULONG index, char *buffer,
        ULONG buffer_size, ULONG *element_size)
{
    FIXME("%p, %lu, %p, %lu, %p stub.\n", iface, index, buffer, buffer_size, element_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_SetSourcePath(IDebugSymbols3 *iface, const char *path)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_AppendSourcePath(IDebugSymbols3 *iface, const char *path)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_FindSourceFile(IDebugSymbols3 *iface, ULONG start, const char *file,
        ULONG flags, ULONG *found_element, char *buffer, ULONG buffer_size, ULONG *found_size)
{
    FIXME("%p, %lu, %s, %#lx, %p, %p, %lu, %p stub.\n", iface, start, debugstr_a(file), flags, found_element, buffer,
            buffer_size, found_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSourceFileLineOffsets(IDebugSymbols3 *iface, const char *file,
        ULONG64 *buffer, ULONG buffer_lines, ULONG *file_lines)
{
    FIXME("%p, %s, %p, %lu, %p stub.\n", iface, debugstr_a(file), buffer, buffer_lines, file_lines);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetModuleVersionInformation(IDebugSymbols3 *iface, ULONG index,
        ULONG64 base, const char *item, void *buffer, ULONG buffer_size, ULONG *info_size)
{
    struct debug_client *debug_client = impl_from_IDebugSymbols3(iface);
    const struct module_info *info;
    struct target_process *target;
    void *version_info, *ptr;
    HRESULT hr = E_FAIL;
    DWORD handle;
    UINT size;

    TRACE("%p, %lu, %s, %s, %p, %lu, %p.\n", iface, index, wine_dbgstr_longlong(base), debugstr_a(item), buffer,
            buffer_size, info_size);

    if (!(target = debug_client_get_target(debug_client)))
        return E_UNEXPECTED;

    if (index == DEBUG_ANY_ID)
        info = debug_target_get_module_info_by_base(target, base);
    else
        info = debug_target_get_module_info(target, index);

    if (!info)
    {
        WARN("Was unable to locate module.\n");
        return E_INVALIDARG;
    }

    if (!(size = GetFileVersionInfoSizeA(info->image_name, &handle)))
        return E_FAIL;

    if (!(version_info = malloc(size)))
        return E_OUTOFMEMORY;

    if (GetFileVersionInfoA(info->image_name, handle, size, version_info))
    {
        if (VerQueryValueA(version_info, item, &ptr, &size))
        {
            if (info_size)
                *info_size = size;

            if (buffer && buffer_size)
            {
                unsigned int dst_len = min(size, buffer_size);
                if (dst_len)
                    memcpy(buffer, ptr, dst_len);
            }

            hr = buffer && buffer_size < size ? S_FALSE : S_OK;
        }
    }

    free(version_info);

    return hr;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetModuleNameString(IDebugSymbols3 *iface, ULONG which, ULONG index,
        ULONG64 base, char *buffer, ULONG buffer_size, ULONG *name_size)
{
    struct debug_client *debug_client = impl_from_IDebugSymbols3(iface);
    const struct module_info *info;
    struct target_process *target;
    HRESULT hr;

    TRACE("%p, %lu, %lu, %s, %p, %lu, %p.\n", iface, which, index, wine_dbgstr_longlong(base), buffer, buffer_size,
            name_size);

    if (!(target = debug_client_get_target(debug_client)))
        return E_UNEXPECTED;

    if (index == DEBUG_ANY_ID)
        info = debug_target_get_module_info_by_base(target, base);
    else
        info = debug_target_get_module_info(target, index);

    if (!info)
    {
        WARN("Was unable to locate module.\n");
        return E_INVALIDARG;
    }

    switch (which)
    {
        case DEBUG_MODNAME_IMAGE:
            hr = debug_target_return_string(info->image_name, buffer, buffer_size, name_size);
            break;
        case DEBUG_MODNAME_MODULE:
        case DEBUG_MODNAME_LOADED_IMAGE:
        case DEBUG_MODNAME_SYMBOL_FILE:
        case DEBUG_MODNAME_MAPPED_IMAGE:
            FIXME("Unsupported name info %ld.\n", which);
            return E_NOTIMPL;
        default:
            WARN("Unknown name info %ld.\n", which);
            return E_INVALIDARG;
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetConstantName(IDebugSymbols3 *iface, ULONG64 module, ULONG type_id,
        ULONG64 value, char *buffer, ULONG buffer_size, ULONG *name_size)
{
    FIXME("%p, %s, %lu, %s, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(module), type_id,
            wine_dbgstr_longlong(value), buffer, buffer_size, name_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetFieldName(IDebugSymbols3 *iface, ULONG64 module, ULONG type_id,
        ULONG field_index, char *buffer, ULONG buffer_size, ULONG *name_size)
{
    FIXME("%p, %s, %lu, %lu, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(module), type_id, field_index, buffer,
            buffer_size, name_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetTypeOptions(IDebugSymbols3 *iface, ULONG *options)
{
    FIXME("%p, %p stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_AddTypeOptions(IDebugSymbols3 *iface, ULONG options)
{
    FIXME("%p, %#lx stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_RemoveTypeOptions(IDebugSymbols3 *iface, ULONG options)
{
    FIXME("%p, %#lx stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_SetTypeOptions(IDebugSymbols3 *iface, ULONG options)
{
    FIXME("%p, %#lx stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetNameByOffsetWide(IDebugSymbols3 *iface, ULONG64 offset, WCHAR *buffer,
        ULONG buffer_size, ULONG *name_size, ULONG64 *displacement)
{
    FIXME("%p, %s, %p, %lu, %p, %p stub.\n", iface, wine_dbgstr_longlong(offset), buffer, buffer_size, name_size,
            displacement);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetOffsetByNameWide(IDebugSymbols3 *iface, const WCHAR *symbol,
        ULONG64 *offset)
{
    FIXME("%p, %s, %p stub.\n", iface, debugstr_w(symbol), offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetNearNameByOffsetWide(IDebugSymbols3 *iface, ULONG64 offset,
        LONG delta, WCHAR *buffer, ULONG buffer_size, ULONG *name_size, ULONG64 *displacement)
{
    FIXME("%p, %s, %ld, %p, %lu, %p, %p stub.\n", iface, wine_dbgstr_longlong(offset), delta, buffer, buffer_size,
            name_size, displacement);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetLineByOffsetWide(IDebugSymbols3 *iface, ULONG64 offset, ULONG *line,
        WCHAR *buffer, ULONG buffer_size, ULONG *file_size, ULONG64 *displacement)
{
    FIXME("%p, %s, %p, %p, %lu, %p, %p stub.\n", iface, wine_dbgstr_longlong(offset), line, buffer, buffer_size,
            file_size, displacement);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetOffsetByLineWide(IDebugSymbols3 *iface, ULONG line, const WCHAR *file,
        ULONG64 *offset)
{
    FIXME("%p, %lu, %s, %p stub.\n", iface, line, debugstr_w(file), offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetModuleByModuleNameWide(IDebugSymbols3 *iface, const WCHAR *name,
        ULONG start_index, ULONG *index, ULONG64 *base)
{
    FIXME("%p, %s, %lu, %p, %p stub.\n", iface, debugstr_w(name), start_index, index, base);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolModuleWide(IDebugSymbols3 *iface, const WCHAR *symbol,
        ULONG64 *base)
{
    FIXME("%p, %s, %p stub.\n", iface, debugstr_w(symbol), base);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetTypeNameWide(IDebugSymbols3 *iface, ULONG64 module, ULONG type_id,
        WCHAR *buffer, ULONG buffer_size, ULONG *name_size)
{
    FIXME("%p, %s, %lu, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(module), type_id, buffer, buffer_size,
            name_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetTypeIdWide(IDebugSymbols3 *iface, ULONG64 module, const WCHAR *name,
        ULONG *type_id)
{
    FIXME("%p, %s, %s, %p stub.\n", iface, wine_dbgstr_longlong(module), debugstr_w(name), type_id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetFieldOffsetWide(IDebugSymbols3 *iface, ULONG64 module, ULONG type_id,
        const WCHAR *field, ULONG *offset)
{
    FIXME("%p, %s, %lu, %s, %p stub.\n", iface, wine_dbgstr_longlong(module), type_id, debugstr_w(field), offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolTypeIdWide(IDebugSymbols3 *iface, const WCHAR *symbol,
        ULONG *type_id, ULONG64 *module)
{
    FIXME("%p, %s, %p, %p stub.\n", iface, debugstr_w(symbol), type_id, module);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetScopeSymbolGroup2(IDebugSymbols3 *iface, ULONG flags,
        PDEBUG_SYMBOL_GROUP2 update, PDEBUG_SYMBOL_GROUP2 *symbols)
{
    FIXME("%p, %#lx, %p, %p stub.\n", iface, flags, update, symbols);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_CreateSymbolGroup2(IDebugSymbols3 *iface, PDEBUG_SYMBOL_GROUP2 *group)
{
    FIXME("%p, %p stub.\n", iface, group);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_StartSymbolMatchWide(IDebugSymbols3 *iface, const WCHAR *pattern,
        ULONG64 *handle)
{
    FIXME("%p, %s, %p stub.\n", iface, debugstr_w(pattern), handle);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetNextSymbolMatchWide(IDebugSymbols3 *iface, ULONG64 handle,
        WCHAR *buffer, ULONG buffer_size, ULONG *match_size, ULONG64 *offset)
{
    FIXME("%p, %s, %p, %lu, %p, %p stub.\n", iface, wine_dbgstr_longlong(handle), buffer, buffer_size, match_size, offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_ReloadWide(IDebugSymbols3 *iface, const WCHAR *module)
{
    FIXME("%p, %s stub.\n", iface, debugstr_w(module));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolPathWide(IDebugSymbols3 *iface, WCHAR *buffer, ULONG buffer_size,
        ULONG *path_size)
{
    FIXME("%p, %p, %lu, %p stub.\n", iface, buffer, buffer_size, path_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_SetSymbolPathWide(IDebugSymbols3 *iface, const WCHAR *path)
{
    FIXME("%p, %s stub.\n", iface, debugstr_w(path));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_AppendSymbolPathWide(IDebugSymbols3 *iface, const WCHAR *addition)
{
    FIXME("%p, %s stub.\n", iface, debugstr_w(addition));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetImagePathWide(IDebugSymbols3 *iface, WCHAR *buffer, ULONG buffer_size,
        ULONG *path_size)
{
    FIXME("%p, %p, %lu, %p stub.\n", iface, buffer, buffer_size, path_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_SetImagePathWide(IDebugSymbols3 *iface, const WCHAR *path)
{
    FIXME("%p, %s stub.\n", iface, debugstr_w(path));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_AppendImagePathWide(IDebugSymbols3 *iface, const WCHAR *addition)
{
    FIXME("%p, %s stub.\n", iface, debugstr_w(addition));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSourcePathWide(IDebugSymbols3 *iface, WCHAR *buffer, ULONG buffer_size,
        ULONG *path_size)
{
    FIXME("%p, %p, %lu, %p stub.\n", iface, buffer, buffer_size, path_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSourcePathElementWide(IDebugSymbols3 *iface, ULONG index,
        WCHAR *buffer, ULONG buffer_size, ULONG *element_size)
{
    FIXME("%p, %lu, %p, %lu, %p stub.\n", iface, index, buffer, buffer_size, element_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_SetSourcePathWide(IDebugSymbols3 *iface, const WCHAR *path)
{
    FIXME("%p, %s stub.\n", iface, debugstr_w(path));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_AppendSourcePathWide(IDebugSymbols3 *iface, const WCHAR *addition)
{
    FIXME("%p, %s stub.\n", iface, debugstr_w(addition));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_FindSourceFileWide(IDebugSymbols3 *iface, ULONG start_element,
        const WCHAR *file, ULONG flags, ULONG *found_element, WCHAR *buffer, ULONG buffer_size, ULONG *found_size)
{
    FIXME("%p, %lu, %s, %#lx, %p, %p, %lu, %p stub.\n", iface, start_element, debugstr_w(file), flags, found_element,
            buffer, buffer_size, found_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSourceFileLineOffsetsWide(IDebugSymbols3 *iface, const WCHAR *file,
        ULONG64 *buffer, ULONG buffer_lines, ULONG *file_lines)
{
    FIXME("%p, %s, %p, %lu, %p stub.\n", iface, debugstr_w(file), buffer, buffer_lines, file_lines);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetModuleVersionInformationWide(IDebugSymbols3 *iface, ULONG index,
        ULONG64 base, const WCHAR *item, void *buffer, ULONG buffer_size, ULONG *version_info_size)
{
    FIXME("%p, %lu, %s, %s, %p, %lu, %p stub.\n", iface, index, wine_dbgstr_longlong(base), debugstr_w(item), buffer,
            buffer_size, version_info_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetModuleNameStringWide(IDebugSymbols3 *iface, ULONG which, ULONG index,
        ULONG64 base, WCHAR *buffer, ULONG buffer_size, ULONG *name_size)
{
    FIXME("%p, %lu, %lu, %s, %p, %lu, %p stub.\n", iface, which, index, wine_dbgstr_longlong(base), buffer, buffer_size,
            name_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetConstantNameWide(IDebugSymbols3 *iface, ULONG64 module, ULONG type_id,
        ULONG64 value, WCHAR *buffer, ULONG buffer_size, ULONG *name_size)
{
    FIXME("%p, %s, %lu, %s, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(module), type_id,
            wine_dbgstr_longlong(value), buffer, buffer_size, name_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetFieldNameWide(IDebugSymbols3 *iface, ULONG64 module, ULONG type_id,
        ULONG field_index, WCHAR *buffer, ULONG buffer_size, ULONG *name_size)
{
    FIXME("%p, %s, %lu, %lu, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(module), type_id, field_index, buffer,
            buffer_size, name_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_IsManagedModule(IDebugSymbols3 *iface, ULONG index, ULONG64 base)
{
    FIXME("%p, %lu, %s stub.\n", iface, index, wine_dbgstr_longlong(base));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetModuleByModuleName2(IDebugSymbols3 *iface, const char *name,
        ULONG start_index, ULONG flags, ULONG *index, ULONG64 *base)
{
    FIXME("%p, %s, %lu, %#lx, %p, %p stub.\n", iface, debugstr_a(name), start_index, flags, index, base);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetModuleByModuleName2Wide(IDebugSymbols3 *iface, const WCHAR *name,
        ULONG start_index, ULONG flags, ULONG *index, ULONG64 *base)
{
    FIXME("%p, %s, %lu, %#lx, %p, %p stub.\n", iface, debugstr_w(name), start_index, flags, index, base);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetModuleByOffset2(IDebugSymbols3 *iface, ULONG64 offset,
        ULONG start_index, ULONG flags, ULONG *index, ULONG64 *base)
{
    FIXME("%p, %s, %lu, %#lx, %p, %p stub.\n", iface, wine_dbgstr_longlong(offset), start_index, flags, index, base);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_AddSyntheticModule(IDebugSymbols3 *iface, ULONG64 base, ULONG size,
        const char *image_path, const char *module_name, ULONG flags)
{
    FIXME("%p, %s, %lu, %s, %s, %#lx stub.\n", iface, wine_dbgstr_longlong(base), size, debugstr_a(image_path),
            debugstr_a(module_name), flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_AddSyntheticModuleWide(IDebugSymbols3 *iface, ULONG64 base, ULONG size,
        const WCHAR *image_path, const WCHAR *module_name, ULONG flags)
{
    FIXME("%p, %s, %lu, %s, %s, %#lx stub.\n", iface, wine_dbgstr_longlong(base), size, debugstr_w(image_path),
            debugstr_w(module_name), flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_RemoveSyntheticModule(IDebugSymbols3 *iface, ULONG64 base)
{
    FIXME("%p, %s stub.\n", iface, wine_dbgstr_longlong(base));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetCurrentScopeFrameIndex(IDebugSymbols3 *iface, ULONG *index)
{
    FIXME("%p, %p stub.\n", iface, index);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_SetScopeFrameByIndex(IDebugSymbols3 *iface, ULONG index)
{
    FIXME("%p, %lu stub.\n", iface, index);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_SetScopeFromJitDebugInfo(IDebugSymbols3 *iface, ULONG output_control,
        ULONG64 info_offset)
{
    FIXME("%p, %lu, %s stub.\n", iface, output_control, wine_dbgstr_longlong(info_offset));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_SetScopeFromStoredEvent(IDebugSymbols3 *iface)
{
    FIXME("%p stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_OutputSymbolByOffset(IDebugSymbols3 *iface, ULONG output_control,
        ULONG flags, ULONG64 offset)
{
    FIXME("%p, %lu, %#lx, %s stub.\n", iface, output_control, flags, wine_dbgstr_longlong(offset));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetFunctionEntryByOffset(IDebugSymbols3 *iface, ULONG64 offset,
        ULONG flags, void *buffer, ULONG buffer_size, ULONG *needed_size)
{
    FIXME("%p, %s, %#lx, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(offset), flags, buffer, buffer_size,
            needed_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetFieldTypeAndOffset(IDebugSymbols3 *iface, ULONG64 module,
        ULONG container_type_id, const char *field, ULONG *field_type_id, ULONG *offset)
{
    FIXME("%p, %s, %lu, %s, %p, %p stub.\n", iface, wine_dbgstr_longlong(module), container_type_id, debugstr_a(field),
            field_type_id, offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetFieldTypeAndOffsetWide(IDebugSymbols3 *iface, ULONG64 module,
        ULONG container_type_id, const WCHAR *field, ULONG *field_type_id, ULONG *offset)
{
    FIXME("%p, %s, %lu, %s, %p, %p stub.\n", iface, wine_dbgstr_longlong(module), container_type_id, debugstr_w(field),
            field_type_id, offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_AddSyntheticSymbol(IDebugSymbols3 *iface, ULONG64 offset, ULONG size,
        const char *name, ULONG flags, DEBUG_MODULE_AND_ID *id)
{
    FIXME("%p, %s, %lu, %s, %#lx, %p stub.\n", iface, wine_dbgstr_longlong(offset), size, debugstr_a(name), flags, id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_AddSyntheticSymbolWide(IDebugSymbols3 *iface, ULONG64 offset, ULONG size,
        const WCHAR *name, ULONG flags, DEBUG_MODULE_AND_ID *id)
{
    FIXME("%p, %s, %lu, %s, %#lx, %p stub.\n", iface, wine_dbgstr_longlong(offset), size, debugstr_w(name), flags, id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_RemoveSyntheticSymbol(IDebugSymbols3 *iface, DEBUG_MODULE_AND_ID *id)
{
    FIXME("%p, %p stub.\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolEntriesByOffset(IDebugSymbols3 *iface, ULONG64 offset,
        ULONG flags, DEBUG_MODULE_AND_ID *ids, LONG64 *displacements, ULONG count, ULONG *entries)
{
    FIXME("%p, %s, %#lx, %p, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(offset), flags, ids, displacements, count,
            entries);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolEntriesByName(IDebugSymbols3 *iface, const char *symbol,
        ULONG flags, DEBUG_MODULE_AND_ID *ids, ULONG count, ULONG *entries)
{
    FIXME("%p, %s, %#lx, %p, %lu, %p stub.\n", iface, debugstr_a(symbol), flags, ids, count, entries);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolEntriesByNameWide(IDebugSymbols3 *iface, const WCHAR *symbol,
        ULONG flags, DEBUG_MODULE_AND_ID *ids, ULONG count, ULONG *entries)
{
    FIXME("%p, %s, %#lx, %p, %lu, %p stub.\n", iface, debugstr_w(symbol), flags, ids, count, entries);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolEntryByToken(IDebugSymbols3 *iface, ULONG64 base, ULONG token,
        DEBUG_MODULE_AND_ID *id)
{
    FIXME("%p, %s, %p stub.\n", iface, wine_dbgstr_longlong(base), id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolEntryInformation(IDebugSymbols3 *iface, DEBUG_MODULE_AND_ID *id,
        DEBUG_SYMBOL_ENTRY *info)
{
    FIXME("%p, %p, %p stub.\n", iface, id, info);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolEntryString(IDebugSymbols3 *iface, DEBUG_MODULE_AND_ID *id,
        ULONG which, char *buffer, ULONG buffer_size, ULONG *string_size)
{
    FIXME("%p, %p, %lu, %p, %lu, %p stub.\n", iface, id, which, buffer, buffer_size, string_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolEntryStringWide(IDebugSymbols3 *iface, DEBUG_MODULE_AND_ID *id,
        ULONG which, WCHAR *buffer, ULONG buffer_size, ULONG *string_size)
{
    FIXME("%p, %p, %lu, %p, %lu, %p stub.\n", iface, id, which, buffer, buffer_size, string_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolEntryOffsetRegions(IDebugSymbols3 *iface, DEBUG_MODULE_AND_ID *id,
        ULONG flags, DEBUG_OFFSET_REGION *regions, ULONG regions_count, ULONG *regions_avail)
{
    FIXME("%p, %p, %#lx, %p, %lu, %p stub.\n", iface, id, flags, regions, regions_count, regions_avail);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolEntryBySymbolEntry(IDebugSymbols3 *iface,
        DEBUG_MODULE_AND_ID *from_id, ULONG flags, DEBUG_MODULE_AND_ID *to_id)
{
    FIXME("%p, %p, %#lx, %p stub.\n", iface, from_id, flags, to_id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSourceEntriesByOffset(IDebugSymbols3 *iface, ULONG64 offset,
        ULONG flags, DEBUG_SYMBOL_SOURCE_ENTRY *entries, ULONG count, ULONG *entries_avail)
{
    FIXME("%p, %s, %#lx, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(offset), flags, entries, count, entries_avail);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSourceEntriesByLine(IDebugSymbols3 *iface, ULONG line,
        const char *file, ULONG flags, DEBUG_SYMBOL_SOURCE_ENTRY *entries, ULONG count, ULONG *entries_avail)
{
    FIXME("%p, %s, %#lx, %p, %lu, %p stub.\n", iface, debugstr_a(file), flags, entries, count, entries_avail);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSourceEntriesByLineWide(IDebugSymbols3 *iface, ULONG line,
        const WCHAR *file, ULONG flags, DEBUG_SYMBOL_SOURCE_ENTRY *entries, ULONG count, ULONG *entries_avail)
{
    FIXME("%p, %s, %#lx, %p, %lu, %p stub.\n", iface, debugstr_w(file), flags, entries, count, entries_avail);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSourceEntryString(IDebugSymbols3 *iface,
        DEBUG_SYMBOL_SOURCE_ENTRY *entry, ULONG which, char *buffer, ULONG buffer_size, ULONG *string_size)
{
    FIXME("%p, %p, %lu, %p, %lu, %p stub.\n", iface, entry, which, buffer, buffer_size, string_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSourceEntryStringWide(IDebugSymbols3 *iface,
        DEBUG_SYMBOL_SOURCE_ENTRY *entry, ULONG which, WCHAR *buffer, ULONG buffer_size, ULONG *string_size)
{
    FIXME("%p, %p, %lu, %p, %lu, %p stub.\n", iface, entry, which, buffer, buffer_size, string_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSourceEntryOffsetRegions(IDebugSymbols3 *iface,
        DEBUG_SYMBOL_SOURCE_ENTRY *entry, ULONG flags, DEBUG_OFFSET_REGION *regions, ULONG count, ULONG *regions_avail)
{
    FIXME("%p, %p, %#lx, %p, %lu, %p stub.\n", iface, entry, flags, regions, count, regions_avail);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSourceEntryBySourceEntry(IDebugSymbols3 *iface,
        DEBUG_SYMBOL_SOURCE_ENTRY *from_entry, ULONG flags, DEBUG_SYMBOL_SOURCE_ENTRY *to_entry)
{
    FIXME("%p, %p, %#lx, %p stub.\n", iface, from_entry, flags, to_entry);

    return E_NOTIMPL;
}

static const IDebugSymbols3Vtbl debugsymbolsvtbl =
{
    debugsymbols_QueryInterface,
    debugsymbols_AddRef,
    debugsymbols_Release,
    debugsymbols_GetSymbolOptions,
    debugsymbols_AddSymbolOptions,
    debugsymbols_RemoveSymbolOptions,
    debugsymbols_SetSymbolOptions,
    debugsymbols_GetNameByOffset,
    debugsymbols_GetOffsetByName,
    debugsymbols_GetNearNameByOffset,
    debugsymbols_GetLineByOffset,
    debugsymbols_GetOffsetByLine,
    debugsymbols_GetNumberModules,
    debugsymbols_GetModuleByIndex,
    debugsymbols_GetModuleByModuleName,
    debugsymbols_GetModuleByOffset,
    debugsymbols_GetModuleNames,
    debugsymbols_GetModuleParameters,
    debugsymbols_GetSymbolModule,
    debugsymbols_GetTypeName,
    debugsymbols_GetTypeId,
    debugsymbols_GetTypeSize,
    debugsymbols_GetFieldOffset,
    debugsymbols_GetSymbolTypeId,
    debugsymbols_GetOffsetTypeId,
    debugsymbols_ReadTypedDataVirtual,
    debugsymbols_WriteTypedDataVirtual,
    debugsymbols_OutputTypedDataVirtual,
    debugsymbols_ReadTypedDataPhysical,
    debugsymbols_WriteTypedDataPhysical,
    debugsymbols_OutputTypedDataPhysical,
    debugsymbols_GetScope,
    debugsymbols_SetScope,
    debugsymbols_ResetScope,
    debugsymbols_GetScopeSymbolGroup,
    debugsymbols_CreateSymbolGroup,
    debugsymbols_StartSymbolMatch,
    debugsymbols_GetNextSymbolMatch,
    debugsymbols_EndSymbolMatch,
    debugsymbols_Reload,
    debugsymbols_GetSymbolPath,
    debugsymbols_SetSymbolPath,
    debugsymbols_AppendSymbolPath,
    debugsymbols_GetImagePath,
    debugsymbols_SetImagePath,
    debugsymbols_AppendImagePath,
    debugsymbols_GetSourcePath,
    debugsymbols_GetSourcePathElement,
    debugsymbols_SetSourcePath,
    debugsymbols_AppendSourcePath,
    debugsymbols_FindSourceFile,
    debugsymbols_GetSourceFileLineOffsets,
    /* IDebugSymbols2 */
    debugsymbols_GetModuleVersionInformation,
    debugsymbols_GetModuleNameString,
    debugsymbols_GetConstantName,
    debugsymbols_GetFieldName,
    debugsymbols_GetTypeOptions,
    debugsymbols_AddTypeOptions,
    debugsymbols_RemoveTypeOptions,
    debugsymbols_SetTypeOptions,
    /* IDebugSymbols3 */
    debugsymbols_GetNameByOffsetWide,
    debugsymbols_GetOffsetByNameWide,
    debugsymbols_GetNearNameByOffsetWide,
    debugsymbols_GetLineByOffsetWide,
    debugsymbols_GetOffsetByLineWide,
    debugsymbols_GetModuleByModuleNameWide,
    debugsymbols_GetSymbolModuleWide,
    debugsymbols_GetTypeNameWide,
    debugsymbols_GetTypeIdWide,
    debugsymbols_GetFieldOffsetWide,
    debugsymbols_GetSymbolTypeIdWide,
    debugsymbols_GetScopeSymbolGroup2,
    debugsymbols_CreateSymbolGroup2,
    debugsymbols_StartSymbolMatchWide,
    debugsymbols_GetNextSymbolMatchWide,
    debugsymbols_ReloadWide,
    debugsymbols_GetSymbolPathWide,
    debugsymbols_SetSymbolPathWide,
    debugsymbols_AppendSymbolPathWide,
    debugsymbols_GetImagePathWide,
    debugsymbols_SetImagePathWide,
    debugsymbols_AppendImagePathWide,
    debugsymbols_GetSourcePathWide,
    debugsymbols_GetSourcePathElementWide,
    debugsymbols_SetSourcePathWide,
    debugsymbols_AppendSourcePathWide,
    debugsymbols_FindSourceFileWide,
    debugsymbols_GetSourceFileLineOffsetsWide,
    debugsymbols_GetModuleVersionInformationWide,
    debugsymbols_GetModuleNameStringWide,
    debugsymbols_GetConstantNameWide,
    debugsymbols_GetFieldNameWide,
    debugsymbols_IsManagedModule,
    debugsymbols_GetModuleByModuleName2,
    debugsymbols_GetModuleByModuleName2Wide,
    debugsymbols_GetModuleByOffset2,
    debugsymbols_AddSyntheticModule,
    debugsymbols_AddSyntheticModuleWide,
    debugsymbols_RemoveSyntheticModule,
    debugsymbols_GetCurrentScopeFrameIndex,
    debugsymbols_SetScopeFrameByIndex,
    debugsymbols_SetScopeFromJitDebugInfo,
    debugsymbols_SetScopeFromStoredEvent,
    debugsymbols_OutputSymbolByOffset,
    debugsymbols_GetFunctionEntryByOffset,
    debugsymbols_GetFieldTypeAndOffset,
    debugsymbols_GetFieldTypeAndOffsetWide,
    debugsymbols_AddSyntheticSymbol,
    debugsymbols_AddSyntheticSymbolWide,
    debugsymbols_RemoveSyntheticSymbol,
    debugsymbols_GetSymbolEntriesByOffset,
    debugsymbols_GetSymbolEntriesByName,
    debugsymbols_GetSymbolEntriesByNameWide,
    debugsymbols_GetSymbolEntryByToken,
    debugsymbols_GetSymbolEntryInformation,
    debugsymbols_GetSymbolEntryString,
    debugsymbols_GetSymbolEntryStringWide,
    debugsymbols_GetSymbolEntryOffsetRegions,
    debugsymbols_GetSymbolEntryBySymbolEntry,
    debugsymbols_GetSourceEntriesByOffset,
    debugsymbols_GetSourceEntriesByLine,
    debugsymbols_GetSourceEntriesByLineWide,
    debugsymbols_GetSourceEntryString,
    debugsymbols_GetSourceEntryStringWide,
    debugsymbols_GetSourceEntryOffsetRegions,
    debugsymbols_GetSourceEntryBySourceEntry,
};

static HRESULT STDMETHODCALLTYPE debugcontrol_QueryInterface(IDebugControl4 *iface, REFIID riid, void **obj)
{
    struct debug_client *debug_client = impl_from_IDebugControl4(iface);
    return IUnknown_QueryInterface(&debug_client->IDebugClient_iface, riid, obj);
}

static ULONG STDMETHODCALLTYPE debugcontrol_AddRef(IDebugControl4 *iface)
{
    struct debug_client *debug_client = impl_from_IDebugControl4(iface);
    return IUnknown_AddRef(&debug_client->IDebugClient_iface);
}

static ULONG STDMETHODCALLTYPE debugcontrol_Release(IDebugControl4 *iface)
{
    struct debug_client *debug_client = impl_from_IDebugControl4(iface);
    return IUnknown_Release(&debug_client->IDebugClient_iface);
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetInterrupt(IDebugControl4 *iface)
{
    FIXME("%p stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetInterrupt(IDebugControl4 *iface, ULONG flags)
{
    FIXME("%p, %#lx stub.\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetInterruptTimeout(IDebugControl4 *iface, ULONG *timeout)
{
    FIXME("%p, %p stub.\n", iface, timeout);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetInterruptTimeout(IDebugControl4 *iface, ULONG timeout)
{
    FIXME("%p, %lu stub.\n", iface, timeout);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetLogFile(IDebugControl4 *iface, char *buffer, ULONG buffer_size,
        ULONG *file_size, BOOL *append)
{
    FIXME("%p, %p, %lu, %p, %p stub.\n", iface, buffer, buffer_size, file_size, append);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_OpenLogFile(IDebugControl4 *iface, const char *file, BOOL append)
{
    FIXME("%p, %s, %d stub.\n", iface, debugstr_a(file), append);

    return E_NOTIMPL;
}
static HRESULT STDMETHODCALLTYPE debugcontrol_CloseLogFile(IDebugControl4 *iface)
{
    FIXME("%p stub.\n", iface);

    return E_NOTIMPL;
}
static HRESULT STDMETHODCALLTYPE debugcontrol_GetLogMask(IDebugControl4 *iface, ULONG *mask)
{
    FIXME("%p, %p stub.\n", iface, mask);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetLogMask(IDebugControl4 *iface, ULONG mask)
{
    FIXME("%p, %#lx stub.\n", iface, mask);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_Input(IDebugControl4 *iface, char *buffer, ULONG buffer_size,
        ULONG *input_size)
{
    FIXME("%p, %p, %lu, %p stub.\n", iface, buffer, buffer_size, input_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_ReturnInput(IDebugControl4 *iface, const char *buffer)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(buffer));

    return E_NOTIMPL;
}

static HRESULT STDMETHODVCALLTYPE debugcontrol_Output(IDebugControl4 *iface, ULONG mask, const char *format, ...)
{
    FIXME("%p, %#lx, %s stub.\n", iface, mask, debugstr_a(format));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_OutputVaList(IDebugControl4 *iface, ULONG mask, const char *format,
        va_list args)
{
    FIXME("%p, %#lx, %s stub.\n", iface, mask, debugstr_a(format));

    return E_NOTIMPL;
}

static HRESULT STDMETHODVCALLTYPE debugcontrol_ControlledOutput(IDebugControl4 *iface, ULONG output_control,
        ULONG mask, const char *format, ...)
{
    FIXME("%p, %lu, %#lx, %s stub.\n", iface, output_control, mask, debugstr_a(format));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_ControlledOutputVaList(IDebugControl4 *iface, ULONG output_control,
        ULONG mask, const char *format, va_list args)
{
    FIXME("%p, %lu, %#lx, %s stub.\n", iface, output_control, mask, debugstr_a(format));

    return E_NOTIMPL;
}

static HRESULT STDMETHODVCALLTYPE debugcontrol_OutputPrompt(IDebugControl4 *iface, ULONG output_control,
        const char *format, ...)
{
    FIXME("%p, %lu, %s stub.\n", iface, output_control, debugstr_a(format));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_OutputPromptVaList(IDebugControl4 *iface, ULONG output_control,
        const char *format, va_list args)
{
    FIXME("%p, %lu, %s stub.\n", iface, output_control, debugstr_a(format));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetPromptText(IDebugControl4 *iface, char *buffer, ULONG buffer_size,
        ULONG *text_size)
{
    FIXME("%p, %p, %lu, %p stub.\n", iface, buffer, buffer_size, text_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_OutputCurrentState(IDebugControl4 *iface, ULONG output_control,
        ULONG flags)
{
    FIXME("%p, %lu, %#lx stub.\n", iface, output_control, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_OutputVersionInformation(IDebugControl4 *iface, ULONG output_control)
{
    FIXME("%p, %lu stub.\n", iface, output_control);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetNotifyEventHandle(IDebugControl4 *iface, ULONG64 *handle)
{
    FIXME("%p, %p stub.\n", iface, handle);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetNotifyEventHandle(IDebugControl4 *iface, ULONG64 handle)
{
    FIXME("%p, %s stub.\n", iface, wine_dbgstr_longlong(handle));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_Assemble(IDebugControl4 *iface, ULONG64 offset, const char *code,
        ULONG64 *end_offset)
{
    FIXME("%p, %s, %s, %p stub.\n", iface, wine_dbgstr_longlong(offset), debugstr_a(code), end_offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_Disassemble(IDebugControl4 *iface, ULONG64 offset, ULONG flags,
        char *buffer, ULONG buffer_size, ULONG *disassm_size, ULONG64 *end_offset)
{
    FIXME("%p, %s, %#lx, %p, %lu, %p, %p stub.\n", iface, wine_dbgstr_longlong(offset), flags, buffer, buffer_size,
            disassm_size, end_offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetDisassembleEffectiveOffset(IDebugControl4 *iface, ULONG64 *offset)
{
    FIXME("%p, %p stub.\n", iface, offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_OutputDisassembly(IDebugControl4 *iface, ULONG output_control,
        ULONG64 offset, ULONG flags, ULONG64 *end_offset)
{
    FIXME("%p, %lu, %s, %#lx, %p stub.\n", iface, output_control, wine_dbgstr_longlong(offset), flags, end_offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_OutputDisassemblyLines(IDebugControl4 *iface, ULONG output_control,
        ULONG prev_lines, ULONG total_lines, ULONG64 offset, ULONG flags, ULONG *offset_line, ULONG64 *start_offset,
        ULONG64 *end_offset, ULONG64 *line_offsets)
{
    FIXME("%p, %lu, %lu, %lu, %s, %#lx, %p, %p, %p, %p stub.\n", iface, output_control, prev_lines, total_lines,
            wine_dbgstr_longlong(offset), flags, offset_line, start_offset, end_offset, line_offsets);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetNearInstruction(IDebugControl4 *iface, ULONG64 offset, LONG delta,
        ULONG64 *instr_offset)
{
    FIXME("%p, %s, %ld, %p stub.\n", iface, wine_dbgstr_longlong(offset), delta, instr_offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetStackTrace(IDebugControl4 *iface, ULONG64 frame_offset,
        ULONG64 stack_offset, ULONG64 instr_offset, DEBUG_STACK_FRAME *frames, ULONG frames_size, ULONG *frames_filled)
{
    FIXME("%p, %s, %s, %s, %p, %lu, %p stub.\n", iface, wine_dbgstr_longlong(frame_offset),
            wine_dbgstr_longlong(stack_offset), wine_dbgstr_longlong(instr_offset), frames, frames_size, frames_filled);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetReturnOffset(IDebugControl4 *iface, ULONG64 *offset)
{
    FIXME("%p, %p stub.\n", iface, offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_OutputStackTrace(IDebugControl4 *iface, ULONG output_control,
        DEBUG_STACK_FRAME *frames, ULONG frames_size, ULONG flags)
{
    FIXME("%p, %lu, %p, %lu, %#lx stub.\n", iface, output_control, frames, frames_size, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetDebuggeeType(IDebugControl4 *iface, ULONG *debug_class,
        ULONG *qualifier)
{
    struct debug_client *debug_client = impl_from_IDebugControl4(iface);
    static struct target_process *target;

    FIXME("%p, %p, %p stub.\n", iface, debug_class, qualifier);

    *debug_class = DEBUG_CLASS_UNINITIALIZED;
    *qualifier = 0;

    if (!(target = debug_client_get_target(debug_client)))
        return E_UNEXPECTED;

    *debug_class = DEBUG_CLASS_USER_WINDOWS;
    *qualifier = DEBUG_USER_WINDOWS_PROCESS;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetActualProcessorType(IDebugControl4 *iface, ULONG *type)
{
    FIXME("%p, %p stub.\n", iface, type);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetExecutingProcessorType(IDebugControl4 *iface, ULONG *type)
{
    struct debug_client *debug_client = impl_from_IDebugControl4(iface);
    static struct target_process *target;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, type);

    if (!(target = debug_client_get_target(debug_client)))
        return E_UNEXPECTED;

    if (FAILED(hr = debug_target_init_modules_info(target)))
        return hr;

    *type = target->cpu_type;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetNumberPossibleExecutingProcessorTypes(IDebugControl4 *iface,
        ULONG *count)
{
    FIXME("%p, %p stub.\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetPossibleExecutingProcessorTypes(IDebugControl4 *iface, ULONG start,
        ULONG count, ULONG *types)
{
    FIXME("%p, %lu, %lu, %p stub.\n", iface, start, count, types);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetNumberProcessors(IDebugControl4 *iface, ULONG *count)
{
    FIXME("%p, %p stub.\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetSystemVersion(IDebugControl4 *iface, ULONG *platform_id, ULONG *major,
        ULONG *minor, char *sp_string, ULONG sp_string_size, ULONG *sp_string_used, ULONG *sp_number,
        char *build_string, ULONG build_string_size, ULONG *build_string_used)
{
    FIXME("%p, %p, %p, %p, %p, %lu, %p, %p, %p, %lu, %p stub.\n", iface, platform_id, major, minor, sp_string,
            sp_string_size, sp_string_used, sp_number, build_string, build_string_size, build_string_used);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetPageSize(IDebugControl4 *iface, ULONG *size)
{
    FIXME("%p, %p stub.\n", iface, size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_IsPointer64Bit(IDebugControl4 *iface)
{
    struct debug_client *debug_client = impl_from_IDebugControl4(iface);
    static struct target_process *target;
    HRESULT hr;

    TRACE("%p.\n", iface);

    if (!(target = debug_client_get_target(debug_client)))
        return E_UNEXPECTED;

    if (FAILED(hr = debug_target_init_modules_info(target)))
        return hr;

    switch (target->cpu_type)
    {
        case IMAGE_FILE_MACHINE_I386:
        case IMAGE_FILE_MACHINE_ARMNT:
            hr = S_FALSE;
            break;
        case IMAGE_FILE_MACHINE_IA64:
        case IMAGE_FILE_MACHINE_AMD64:
        case IMAGE_FILE_MACHINE_ARM64:
            hr = S_OK;
            break;
        default:
            FIXME("Unexpected cpu type %#lx.\n", target->cpu_type);
            hr = E_UNEXPECTED;
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_ReadBugCheckData(IDebugControl4 *iface, ULONG *code, ULONG64 *arg1,
        ULONG64 *arg2, ULONG64 *arg3, ULONG64 *arg4)
{
    FIXME("%p, %p, %p, %p, %p, %p stub.\n", iface, code, arg1, arg2, arg3, arg4);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetNumberSupportedProcessorTypes(IDebugControl4 *iface, ULONG *count)
{
    FIXME("%p, %p stub.\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetSupportedProcessorTypes(IDebugControl4 *iface, ULONG start,
        ULONG count, ULONG *types)
{
    FIXME("%p, %lu, %lu, %p stub.\n", iface, start, count, types);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetProcessorTypeNames(IDebugControl4 *iface, ULONG type, char *full_name,
        ULONG full_name_buffer_size, ULONG *full_name_size, char *abbrev_name, ULONG abbrev_name_buffer_size,
        ULONG *abbrev_name_size)
{
    FIXME("%p, %lu, %p, %lu, %p, %p, %lu, %p stub.\n", iface, type, full_name, full_name_buffer_size, full_name_size,
            abbrev_name, abbrev_name_buffer_size, abbrev_name_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetEffectiveProcessorType(IDebugControl4 *iface, ULONG *type)
{
    FIXME("%p, %p stub.\n", iface, type);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetEffectiveProcessorType(IDebugControl4 *iface, ULONG type)
{
    FIXME("%p, %lu stub.\n", iface, type);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetExecutionStatus(IDebugControl4 *iface, ULONG *status)
{
    FIXME("%p, %p stub.\n", iface, status);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetExecutionStatus(IDebugControl4 *iface, ULONG status)
{
    FIXME("%p, %lu stub.\n", iface, status);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetCodeLevel(IDebugControl4 *iface, ULONG *level)
{
    FIXME("%p, %p stub.\n", iface, level);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetCodeLevel(IDebugControl4 *iface, ULONG level)
{
    FIXME("%p, %lu stub.\n", iface, level);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetEngineOptions(IDebugControl4 *iface, ULONG *options)
{
    struct debug_client *debug_client = impl_from_IDebugControl4(iface);

    TRACE("%p, %p.\n", iface, options);

    *options = debug_client->engine_options;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_AddEngineOptions(IDebugControl4 *iface, ULONG options)
{
    struct debug_client *debug_client = impl_from_IDebugControl4(iface);

    TRACE("%p, %#lx.\n", iface, options);

    if (options & ~DEBUG_ENGOPT_ALL)
        return E_INVALIDARG;

    debug_client->engine_options |= options;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_RemoveEngineOptions(IDebugControl4 *iface, ULONG options)
{
    struct debug_client *debug_client = impl_from_IDebugControl4(iface);

    TRACE("%p, %#lx.\n", iface, options);

    debug_client->engine_options &= ~options;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetEngineOptions(IDebugControl4 *iface, ULONG options)
{
    struct debug_client *debug_client = impl_from_IDebugControl4(iface);

    TRACE("%p, %#lx.\n", iface, options);

    if (options & ~DEBUG_ENGOPT_ALL)
        return E_INVALIDARG;

    debug_client->engine_options = options;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetSystemErrorControl(IDebugControl4 *iface, ULONG *output_level,
        ULONG *break_level)
{
    FIXME("%p, %p, %p stub.\n", iface, output_level, break_level);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetSystemErrorControl(IDebugControl4 *iface, ULONG output_level,
        ULONG break_level)
{
    FIXME("%p, %lu, %lu stub.\n", iface, output_level, break_level);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetTextMacro(IDebugControl4 *iface, ULONG slot, char *buffer,
        ULONG buffer_size, ULONG *macro_size)
{
    FIXME("%p, %lu, %p, %lu, %p stub.\n", iface, slot, buffer, buffer_size, macro_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetTextMacro(IDebugControl4 *iface, ULONG slot, const char *macro)
{
    FIXME("%p, %lu, %s stub.\n", iface, slot, debugstr_a(macro));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetRadix(IDebugControl4 *iface, ULONG *radix)
{
    FIXME("%p, %p stub.\n", iface, radix);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetRadix(IDebugControl4 *iface, ULONG radix)
{
    FIXME("%p, %lu stub.\n", iface, radix);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_Evaluate(IDebugControl4 *iface, const char *expression,
        ULONG desired_type, DEBUG_VALUE *value, ULONG *remainder_index)
{
    FIXME("%p, %s, %lu, %p, %p stub.\n", iface, debugstr_a(expression), desired_type, value, remainder_index);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_CoerceValue(IDebugControl4 *iface, DEBUG_VALUE input, ULONG output_type,
        DEBUG_VALUE *output)
{
    FIXME("%p, %lu, %p stub.\n", iface, output_type, output);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_CoerceValues(IDebugControl4 *iface, ULONG count, DEBUG_VALUE *input,
        ULONG *output_types, DEBUG_VALUE *output)
{
    FIXME("%p, %lu, %p, %p, %p stub.\n", iface, count, input, output_types, output);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_Execute(IDebugControl4 *iface, ULONG output_control, const char *command,
        ULONG flags)
{
    FIXME("%p, %lu, %s, %#lx stub.\n", iface, output_control, debugstr_a(command), flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_ExecuteCommandFile(IDebugControl4 *iface, ULONG output_control,
        const char *command_file, ULONG flags)
{
    FIXME("%p, %lu, %s, %#lx stub.\n", iface, output_control, debugstr_a(command_file), flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetNumberBreakpoints(IDebugControl4 *iface, ULONG *count)
{
    FIXME("%p, %p stub.\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetBreakpointByIndex(IDebugControl4 *iface, ULONG index,
        IDebugBreakpoint **bp)
{
    FIXME("%p, %lu, %p stub.\n", iface, index, bp);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetBreakpointById(IDebugControl4 *iface, ULONG id, IDebugBreakpoint **bp)
{
    FIXME("%p, %lu, %p stub.\n", iface, id, bp);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetBreakpointParameters(IDebugControl4 *iface, ULONG count, ULONG *ids,
        ULONG start, DEBUG_BREAKPOINT_PARAMETERS *parameters)
{
    FIXME("%p, %lu, %p, %lu, %p stub.\n", iface, count, ids, start, parameters);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_AddBreakpoint(IDebugControl4 *iface, ULONG type, ULONG desired_id,
        IDebugBreakpoint **bp)
{
    FIXME("%p, %lu, %lu, %p stub.\n", iface, type, desired_id, bp);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_RemoveBreakpoint(IDebugControl4 *iface, IDebugBreakpoint *bp)
{
    FIXME("%p, %p stub.\n", iface, bp);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_AddExtension(IDebugControl4 *iface, const char *path, ULONG flags,
        ULONG64 *handle)
{
    FIXME("%p, %s, %#lx, %p stub.\n", iface, debugstr_a(path), flags, handle);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_RemoveExtension(IDebugControl4 *iface, ULONG64 handle)
{
    FIXME("%p, %s stub.\n", iface, wine_dbgstr_longlong(handle));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetExtensionByPath(IDebugControl4 *iface, const char *path,
        ULONG64 *handle)
{
    FIXME("%p, %s, %p stub.\n", iface, debugstr_a(path), handle);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_CallExtension(IDebugControl4 *iface, ULONG64 handle,
        const char *function, const char *args)
{
    FIXME("%p, %s, %s, %s stub.\n", iface, wine_dbgstr_longlong(handle), debugstr_a(function), debugstr_a(args));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetExtensionFunction(IDebugControl4 *iface, ULONG64 handle,
        const char *name, void *function)
{
    FIXME("%p, %s, %s, %p stub.\n", iface, wine_dbgstr_longlong(handle), debugstr_a(name), function);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetWindbgExtensionApis32(IDebugControl4 *iface,
        PWINDBG_EXTENSION_APIS32 api)
{
    FIXME("%p, %p stub.\n", iface, api);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetWindbgExtensionApis64(IDebugControl4 *iface,
        PWINDBG_EXTENSION_APIS64 api)
{
    FIXME("%p, %p stub.\n", iface, api);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetNumberEventFilters(IDebugControl4 *iface, ULONG *specific_events,
        ULONG *specific_exceptions, ULONG *arbitrary_exceptions)
{
    FIXME("%p, %p, %p, %p stub.\n", iface, specific_events, specific_exceptions, arbitrary_exceptions);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetEventFilterText(IDebugControl4 *iface, ULONG index, char *buffer,
        ULONG buffer_size, ULONG *text_size)
{
    FIXME("%p, %lu, %p, %lu, %p stub.\n", iface, index, buffer, buffer_size, text_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetEventFilterCommand(IDebugControl4 *iface, ULONG index, char *buffer,
        ULONG buffer_size, ULONG *command_size)
{
    FIXME("%p, %lu, %p, %lu, %p stub.\n", iface, index, buffer, buffer_size, command_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetEventFilterCommand(IDebugControl4 *iface, ULONG index,
        const char *command)
{
    FIXME("%p, %lu, %s stub.\n", iface, index, debugstr_a(command));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetSpecificFilterParameters(IDebugControl4 *iface, ULONG start,
        ULONG count, DEBUG_SPECIFIC_FILTER_PARAMETERS *parameters)
{
    FIXME("%p, %lu, %lu, %p stub.\n", iface, start, count, parameters);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetSpecificFilterParameters(IDebugControl4 *iface, ULONG start,
        ULONG count, DEBUG_SPECIFIC_FILTER_PARAMETERS *parameters)
{
    FIXME("%p, %lu, %lu, %p stub.\n", iface, start, count, parameters);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetSpecificFilterArgument(IDebugControl4 *iface, ULONG index,
        char *buffer, ULONG buffer_size, ULONG *argument_size)
{
    FIXME("%p, %lu, %p, %lu, %p stub.\n", iface, index, buffer, buffer_size, argument_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetSpecificFilterArgument(IDebugControl4 *iface, ULONG index,
        const char *argument)
{
    FIXME("%p, %lu, %s stub.\n", iface, index, debugstr_a(argument));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetExceptionFilterParameters(IDebugControl4 *iface, ULONG count,
        ULONG *codes, ULONG start, DEBUG_EXCEPTION_FILTER_PARAMETERS *parameters)
{
    FIXME("%p, %lu, %p, %lu, %p stub.\n", iface, count, codes, start, parameters);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetExceptionFilterParameters(IDebugControl4 *iface, ULONG count,
        DEBUG_EXCEPTION_FILTER_PARAMETERS *parameters)
{
    FIXME("%p, %lu, %p stub.\n", iface, count, parameters);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetExceptionFilterSecondCommand(IDebugControl4 *iface, ULONG index,
        char *buffer, ULONG buffer_size, ULONG *command_size)
{
    FIXME("%p, %lu, %p, %lu, %p stub.\n", iface, index, buffer, buffer_size, command_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetExceptionFilterSecondCommand(IDebugControl4 *iface, ULONG index,
        const char *command)
{
    FIXME("%p, %lu, %s stub.\n", iface, index, debugstr_a(command));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_WaitForEvent(IDebugControl4 *iface, ULONG flags, ULONG timeout)
{
    struct debug_client *debug_client = impl_from_IDebugControl4(iface);
    struct target_process *target;

    TRACE("%p, %#lx, %lu.\n", iface, flags, timeout);

    /* FIXME: only one target is used currently */

    if (!(target = debug_client_get_target(debug_client)))
        return E_UNEXPECTED;

    if (target->attach_flags & DEBUG_ATTACH_NONINVASIVE)
    {
        BOOL suspend = !(target->attach_flags & DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND);
        DWORD access = PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_LIMITED_INFORMATION;
        NTSTATUS status;

        if (suspend)
            access |= PROCESS_SUSPEND_RESUME;

        target->handle = OpenProcess(access, FALSE, target->pid);
        if (!target->handle)
        {
            WARN("Failed to get process handle for pid %#x.\n", target->pid);
            return E_UNEXPECTED;
        }

        if (suspend)
        {
            status = NtSuspendProcess(target->handle);
            if (status)
                WARN("Failed to suspend a process, status %#lx.\n", status);
        }

        return S_OK;
    }
    else
    {
        FIXME("Unsupported attach flags %#x.\n", target->attach_flags);
    }

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetLastEventInformation(IDebugControl4 *iface, ULONG *type, ULONG *pid,
        ULONG *tid, void *extra_info, ULONG extra_info_size, ULONG *extra_info_used, char *description,
        ULONG desc_size, ULONG *desc_used)
{
    FIXME("%p, %p, %p, %p, %p, %lu, %p, %p, %lu, %p stub.\n", iface, type, pid, tid, extra_info, extra_info_size,
            extra_info_used, description, desc_size, desc_used);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetCurrentTimeDate(IDebugControl4 *iface, ULONG timedate)
{
    FIXME("%p, %lu stub.\n", iface, timedate);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetCurrentSystemUpTime(IDebugControl4 *iface, ULONG uptime)
{
    FIXME("%p, %lu stub.\n", iface, uptime);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetDumpFormatFlags(IDebugControl4 *iface, ULONG *flags)
{
    FIXME("%p, %p stub.\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetNumberTextPlacements(IDebugControl4 *iface, ULONG *count)
{
    FIXME("%p, %p stub.\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetNumberTextReplacement(IDebugControl4 *iface, const char *src_text,
        ULONG index, char *src_buffer, ULONG src_buffer_size, ULONG *src_size, char *dst_buffer,
        ULONG dst_buffer_size, ULONG *dst_size)
{
    FIXME("%p, %s, %lu, %p, %lu, %p, %p, %lu, %p stub.\n", iface, debugstr_a(src_text), index, src_buffer,
            src_buffer_size, src_size, dst_buffer, dst_buffer_size, dst_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetTextReplacement(IDebugControl4 *iface, const char *src_text,
        const char *dst_text)
{
    FIXME("%p, %s, %s stub.\n", iface, debugstr_a(src_text), debugstr_a(dst_text));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_RemoveTextReplacements(IDebugControl4 *iface)
{
    FIXME("%p stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_OutputTextReplacements(IDebugControl4 *iface, ULONG output_control,
        ULONG flags)
{
    FIXME("%p, %lu, %#lx stub.\n", iface, output_control, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetAssemblyOptions(IDebugControl4 *iface, ULONG *options)
{
    FIXME("%p, %p stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_AddAssemblyOptions(IDebugControl4 *iface, ULONG options)
{
    FIXME("%p, %#lx stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_RemoveAssemblyOptions(IDebugControl4 *iface, ULONG options)
{
    FIXME("%p, %#lx stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetAssemblyOptions(IDebugControl4 *iface, ULONG options)
{
    FIXME("%p, %#lx stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetExpressionSyntax(IDebugControl4 *iface, ULONG *flags)
{
    FIXME("%p, %p stub.\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetExpressionSyntax(IDebugControl4 *iface, ULONG flags)
{
    FIXME("%p, %#lx stub.\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetExpressionSyntaxByName(IDebugControl4 *iface, const char *name)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(name));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetNumberExpressionSyntaxes(IDebugControl4 *iface, ULONG *number)
{
    FIXME("%p, %p stub.\n", iface, number);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetExpressionSyntaxNames(IDebugControl4 *iface, ULONG index, char *fullname,
        ULONG fullname_buffer_size, ULONG *fullname_size, char *abbrevname, ULONG abbrevname_buffer_size, ULONG *abbrevname_size)
{
    FIXME("%p, %lu, %p, %lu, %p, %p, %lu, %p stub.\n", iface, index, fullname, fullname_buffer_size, fullname_size, abbrevname,
            abbrevname_buffer_size, abbrevname_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetNumberEvents(IDebugControl4 *iface, ULONG *events)
{
    FIXME("%p, %p stub.\n", iface, events);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetEventIndexDescription(IDebugControl4 *iface, ULONG index, ULONG which,
        char *buffer, ULONG buffer_size, ULONG *desc_size)
{
    FIXME("%p, %lu, %lu, %p, %lu, %p stub.\n", iface, index, which, buffer, buffer_size, desc_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetCurrentEventIndex(IDebugControl4 *iface, ULONG *index)
{
    FIXME("%p, %p stub.\n", iface, index);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetNextEventIndex(IDebugControl4 *iface, ULONG relation, ULONG value,
        ULONG *next_index)
{
    FIXME("%p, %lu, %lu, %p stub.\n", iface, relation, value, next_index);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetLogFileWide(IDebugControl4 *iface, WCHAR *buffer, ULONG buffer_size,
        ULONG *file_size, BOOL *append)
{
    FIXME("%p, %p, %lu, %p, %p stub.\n", iface, buffer, buffer_size, file_size, append);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_OpenLogFileWide(IDebugControl4 *iface, const WCHAR *filename, BOOL append)
{
    FIXME("%p, %s, %d stub.\n", iface, debugstr_w(filename), append);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_InputWide(IDebugControl4 *iface, WCHAR *buffer, ULONG buffer_size,
       ULONG *input_size)
{
    FIXME("%p, %p, %lu, %p stub.\n", iface, buffer, buffer_size, input_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_ReturnInputWide(IDebugControl4 *iface, const WCHAR *buffer)
{
    FIXME("%p, %p stub.\n", iface, buffer);

    return E_NOTIMPL;
}

static HRESULT STDMETHODVCALLTYPE debugcontrol_OutputWide(IDebugControl4 *iface, ULONG mask, const WCHAR *format, ...)
{
    FIXME("%p, %lx, %s stub.\n", iface, mask, debugstr_w(format));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_OutputVaListWide(IDebugControl4 *iface, ULONG mask, const WCHAR *format,
        va_list args)
{
    FIXME("%p, %lx, %s stub.\n", iface, mask, debugstr_w(format));

    return E_NOTIMPL;
}

static HRESULT STDMETHODVCALLTYPE debugcontrol_ControlledOutputWide(IDebugControl4 *iface, ULONG output_control, ULONG mask,
        const WCHAR *format, ...)
{
    FIXME("%p, %lx, %lx, %s stub.\n", iface, output_control, mask, debugstr_w(format));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_ControlledOutputVaListWide(IDebugControl4 *iface, ULONG output_control,
        ULONG mask, const WCHAR *format, va_list args)
{
    FIXME("%p, %lx, %lx, %s stub.\n", iface, output_control, mask, debugstr_w(format));

    return E_NOTIMPL;
}

static HRESULT STDMETHODVCALLTYPE debugcontrol_OutputPromptWide(IDebugControl4 *iface, ULONG output_control,
        const WCHAR *format, ...)
{
    FIXME("%p, %lx, %s stub.\n", iface, output_control, debugstr_w(format));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_OutputPromptVaListWide(IDebugControl4 *iface, ULONG output_control,
        const WCHAR *format, va_list args)
{
    FIXME("%p, %lx, %s stub.\n", iface, output_control, debugstr_w(format));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetPromptTextWide(IDebugControl4 *iface, WCHAR *buffer, ULONG buffer_size,
        ULONG *text_size)
{
    FIXME("%p, %p, %lu, %p stub.\n", iface, buffer, buffer_size, text_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_AssembleWide(IDebugControl4 *iface, ULONG64 offset, const WCHAR *instr,
        ULONG64 *end_offset)
{
    FIXME("%p, %s, %s, %p stub.\n", iface, wine_dbgstr_longlong(offset), debugstr_w(instr), end_offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_DisassembleWide(IDebugControl4 *iface, ULONG64 offset, ULONG flags, WCHAR *buffer,
        ULONG buffer_size, ULONG *size, ULONG64 *end_offset)
{
    FIXME("%p, %s, %#lx, %p, %lu, %p, %p stub.\n", iface, wine_dbgstr_longlong(offset), flags, buffer, buffer_size, size, end_offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetProcessorTypeNamesWide(IDebugControl4 *iface, ULONG type, WCHAR *fullname,
        ULONG fullname_buffer_size, ULONG *fullname_size, WCHAR *abbrevname, ULONG abbrevname_buffer_size, ULONG *abbrevname_size)
{
    FIXME("%p, %lu, %p, %lu, %p, %p, %lu, %p stub.\n", iface, type, fullname, fullname_buffer_size, fullname_size, abbrevname,
            abbrevname_buffer_size, abbrevname_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetTextMacroWide(IDebugControl4 *iface, ULONG slot, WCHAR *buffer,
        ULONG buffer_size, ULONG *macro_size)
{
    FIXME("%p, %lu, %p, %lu, %p stub.\n", iface, slot, buffer, buffer_size, macro_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetTextMacroWide(IDebugControl4 *iface, ULONG slot, const WCHAR *macro)
{
    FIXME("%p, %lu, %s stub.\n", iface, slot, debugstr_w(macro));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_EvaluateWide(IDebugControl4 *iface, const WCHAR *expression, ULONG desired_type,
        DEBUG_VALUE *value, ULONG *remainder_index)
{
    FIXME("%p, %s, %lu, %p, %p stub.\n", iface, debugstr_w(expression), desired_type, value, remainder_index);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_ExecuteWide(IDebugControl4 *iface, ULONG output_control, const WCHAR *command,
        ULONG flags)
{
    FIXME("%p, %lx, %s, %lx stub.\n", iface, output_control, debugstr_w(command), flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_ExecuteCommandFileWide(IDebugControl4 *iface, ULONG output_control,
        const WCHAR *commandfile, ULONG flags)
{
    FIXME("%p, %lx, %s, %lx stub.\n", iface, output_control, debugstr_w(commandfile), flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetBreakpointByIndex2(IDebugControl4 *iface, ULONG index, PDEBUG_BREAKPOINT2 *bp)
{
    FIXME("%p, %lu, %p stub.\n", iface, index, bp);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetBreakpointById2(IDebugControl4 *iface, ULONG id, PDEBUG_BREAKPOINT2 *bp)
{
    FIXME("%p, %lu, %p stub.\n", iface, id, bp);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_AddBreakpoint2(IDebugControl4 *iface, ULONG type, ULONG desired_id,
        PDEBUG_BREAKPOINT2 *bp)
{
    FIXME("%p, %lu, %lu, %p stub.\n", iface, type, desired_id, bp);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_RemoveBreakpoint2(IDebugControl4 *iface, PDEBUG_BREAKPOINT2 bp)
{
    FIXME("%p, %p stub.\n", iface, bp);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_AddExtensionWide(IDebugControl4 *iface, const WCHAR *path, ULONG flags,
        ULONG64 *handle)
{
    FIXME("%p, %s, %#lx, %p stub.\n", iface, debugstr_w(path), flags, handle);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetExtensionByPathWide(IDebugControl4 *iface, const WCHAR *path, ULONG64 *handle)
{
    FIXME("%p, %s, %p stub.\n", iface, debugstr_w(path), handle);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_CallExtensionWide(IDebugControl4 *iface, ULONG64 handle, const WCHAR *function,
        const WCHAR *arguments)
{
    FIXME("%p, %s, %s, %s stub.\n", iface, wine_dbgstr_longlong(handle), debugstr_w(function), debugstr_w(arguments));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetExtensionFunctionWide(IDebugControl4 *iface, ULONG64 handle,
        const WCHAR *function, FARPROC *func)
{
    FIXME("%p, %s, %s, %p stub.\n", iface, wine_dbgstr_longlong(handle), debugstr_w(function), func);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetEventFilterTextWide(IDebugControl4 *iface, ULONG index, WCHAR *buffer,
        ULONG buffer_size, ULONG *text_size)
{
    FIXME("%p, %lu, %p, %lu, %p stub.\n", iface, index, buffer, buffer_size, text_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetEventFilterCommandWide(IDebugControl4 *iface, ULONG index, WCHAR *buffer,
        ULONG buffer_size, ULONG *command_size)
{
    FIXME("%p, %lu, %p, %lu, %p stub.\n", iface, index, buffer, buffer_size, command_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetEventFilterCommandWide(IDebugControl4 *iface, ULONG index, const WCHAR *command)
{
    FIXME("%p, %lu, %s stub.\n", iface, index, debugstr_w(command));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetSpecificFilterArgumentWide(IDebugControl4 *iface, ULONG index, WCHAR *buffer,
        ULONG buffer_size, ULONG *argument_size)
{
    FIXME("%p, %lu, %p, %lu, %p stub.\n", iface, index, buffer, buffer_size, argument_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetSpecificFilterArgumentWide(IDebugControl4 *iface, ULONG index,
        const WCHAR *argument)
{
    FIXME("%p, %lu, %s stub.\n", iface, index, debugstr_w(argument));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetSpecificFilterSecondCommandWide(IDebugControl4 *iface, ULONG index,
        WCHAR *buffer, ULONG buffer_size, ULONG *command_size)
{
    FIXME("%p, %lu, %p, %lu, %p stub.\n", iface, index, buffer, buffer_size, command_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetSpecificFilterSecondCommandWide(IDebugControl4 *iface, ULONG index,
        const WCHAR *command)
{
    FIXME("%p, %lu, %s stub.\n", iface, index, debugstr_w(command));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetLastEventInformationWide(IDebugControl4 *iface, ULONG *type, ULONG *processid,
        ULONG *threadid, void *extra_info, ULONG extra_info_size, ULONG *extra_info_used, WCHAR *desc, ULONG desc_size,
        ULONG *desc_used)
{
    FIXME("%p, %p, %p, %p, %p, %lu, %p, %p, %lu, %p stub.\n", iface, type, processid, threadid, extra_info, extra_info_size,
            extra_info_used, desc, desc_size, desc_used);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetTextReplacementWide(IDebugControl4 *iface, const WCHAR *src_text, ULONG index,
        WCHAR *src_buffer, ULONG src_buffer_size, ULONG *src_size, WCHAR *dst_buffer, ULONG dest_buffer_size, ULONG *dst_size)
{
    FIXME("%p, %s, %lu, %p, %lu, %p, %p, %lu, %p stub.\n", iface, debugstr_w(src_text), index, src_buffer, src_buffer_size,
            src_size, dst_buffer, dest_buffer_size, dst_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetTextReplacementWide(IDebugControl4 *iface, const WCHAR *src_text,
        const WCHAR *dst_text)
{
    FIXME("%p, %s, %s stub.\n", iface, debugstr_w(src_text), debugstr_w(dst_text));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_SetExpressionSyntaxByNameWide(IDebugControl4 *iface, const WCHAR *abbrevname)
{
    FIXME("%p, %s stub.\n", iface, debugstr_w(abbrevname));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetExpressionSyntaxNamesWide(IDebugControl4 *iface, ULONG index,
        WCHAR *fullname_buffer, ULONG fullname_buffer_size, ULONG *fullname_size, WCHAR *abbrevname_buffer,
        ULONG abbrevname_buffer_size, ULONG *abbrev_size)
{
    FIXME("%p, %lu, %p, %lu, %p, %p, %lu, %p stub.\n", iface, index, fullname_buffer, fullname_buffer_size, fullname_size,
            abbrevname_buffer, abbrevname_buffer_size, abbrev_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetEventIndexDescriptionWide(IDebugControl4 *iface, ULONG index, ULONG which,
        WCHAR *buffer, ULONG buffer_size, ULONG *desc_size)
{
    FIXME("%p, %lu, %lu, %p, %lu, %p stub.\n", iface, index, which, buffer, buffer_size, desc_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetLogFile2(IDebugControl4 *iface, char *buffer, ULONG buffer_size,
        ULONG *file_size, ULONG *flags)
{
    FIXME("%p, %p, %lu, %p, %p stub.\n", iface, buffer, buffer_size, file_size, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_OpenLogFile2(IDebugControl4 *iface, const char *filename, ULONG flags)
{
    FIXME("%p, %s, %#lx stub.\n", iface, debugstr_a(filename), flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetLogFile2Wide(IDebugControl4 *iface, WCHAR *buffer, ULONG buffer_size,
        ULONG *file_size, ULONG *flags)
{
    FIXME("%p, %p, %lu, %p, %p stub.\n", iface, buffer, buffer_size, file_size, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_OpenLogFile2Wide(IDebugControl4 *iface, const WCHAR *filename, ULONG flags)
{
    FIXME("%p, %s, %#lx stub.\n", iface, debugstr_w(filename), flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetSystemVersionValues(IDebugControl4 *iface, ULONG *platformid,
        ULONG *win32major, ULONG *win32minor, ULONG *kdmajor, ULONG *kdminor)
{
    FIXME("%p, %p, %p, %p, %p, %p stub.\n", iface, platformid, win32major, win32minor, kdmajor, kdminor);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetSystemVersionString(IDebugControl4 *iface, ULONG which, char *buffer,
        ULONG buffer_size, ULONG *string_size)
{
    FIXME("%p, %lu, %p, %lu, %p stub.\n", iface, which, buffer, buffer_size, string_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetSystemVersionStringWide(IDebugControl4 *iface, ULONG which, WCHAR *buffer,
        ULONG buffer_size, ULONG *string_size)
{
    FIXME("%p, %lu, %p, %lu, %p stub.\n", iface, which, buffer, buffer_size, string_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetContextStackTrace(IDebugControl4 *iface, void *start_context,
        ULONG start_context_size, PDEBUG_STACK_FRAME frames, ULONG frames_size, void *frame_contexts, ULONG frame_contexts_size,
        ULONG frame_contexts_entry_size, ULONG *frames_filled)
{
    FIXME("%p, %p, %lu, %p, %lu, %p, %lu, %lu, %p stub.\n", iface, start_context, start_context_size, frames, frames_size,
            frame_contexts, frame_contexts_size, frame_contexts_entry_size, frames_filled);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_OutputContextStackTrace(IDebugControl4 *iface, ULONG output_control,
        PDEBUG_STACK_FRAME frames, ULONG frames_size, void *frame_contexts, ULONG frame_contexts_size,
        ULONG frame_contexts_entry_size, ULONG flags)
{
    FIXME("%p, %#lx, %p, %lu, %p, %lu, %lu, %#lx stub.\n", iface, output_control, frames, frames_size, frame_contexts,
            frame_contexts_size, frame_contexts_entry_size, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetStoredEventInformation(IDebugControl4 *iface, ULONG *type, ULONG *processid,
        ULONG *threadid, void *context, ULONG context_size, ULONG *context_used, void *extra_info, ULONG extra_info_size,
        ULONG *extra_info_used)
{
    FIXME("%p, %p, %p, %p, %p, %lu, %p, %p, %lu, %p stub.\n", iface, type, processid, threadid, context, context_size,
            context_used, extra_info, extra_info_size, extra_info_used);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetManagedStatus(IDebugControl4 *iface, ULONG *flags, ULONG which_string,
        char *string, ULONG string_size, ULONG string_needed)
{
    FIXME("%p, %p, %lu, %p, %lu, %lu stub.\n", iface, flags, which_string, string, string_size, string_needed);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_GetManagedStatusWide(IDebugControl4 *iface, ULONG *flags, ULONG which_string,
        WCHAR *string, ULONG string_size, ULONG string_needed)
{
    FIXME("%p, %p, %lu, %p, %lu, %lu stub.\n", iface, flags, which_string, string, string_size, string_needed);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugcontrol_ResetManagedStatus(IDebugControl4 *iface, ULONG flags)
{
    FIXME("%p, %#lx stub.\n", iface, flags);

    return E_NOTIMPL;
}

static const IDebugControl4Vtbl debugcontrolvtbl =
{
    debugcontrol_QueryInterface,
    debugcontrol_AddRef,
    debugcontrol_Release,
    debugcontrol_GetInterrupt,
    debugcontrol_SetInterrupt,
    debugcontrol_GetInterruptTimeout,
    debugcontrol_SetInterruptTimeout,
    debugcontrol_GetLogFile,
    debugcontrol_OpenLogFile,
    debugcontrol_CloseLogFile,
    debugcontrol_GetLogMask,
    debugcontrol_SetLogMask,
    debugcontrol_Input,
    debugcontrol_ReturnInput,
    debugcontrol_Output,
    debugcontrol_OutputVaList,
    debugcontrol_ControlledOutput,
    debugcontrol_ControlledOutputVaList,
    debugcontrol_OutputPrompt,
    debugcontrol_OutputPromptVaList,
    debugcontrol_GetPromptText,
    debugcontrol_OutputCurrentState,
    debugcontrol_OutputVersionInformation,
    debugcontrol_GetNotifyEventHandle,
    debugcontrol_SetNotifyEventHandle,
    debugcontrol_Assemble,
    debugcontrol_Disassemble,
    debugcontrol_GetDisassembleEffectiveOffset,
    debugcontrol_OutputDisassembly,
    debugcontrol_OutputDisassemblyLines,
    debugcontrol_GetNearInstruction,
    debugcontrol_GetStackTrace,
    debugcontrol_GetReturnOffset,
    debugcontrol_OutputStackTrace,
    debugcontrol_GetDebuggeeType,
    debugcontrol_GetActualProcessorType,
    debugcontrol_GetExecutingProcessorType,
    debugcontrol_GetNumberPossibleExecutingProcessorTypes,
    debugcontrol_GetPossibleExecutingProcessorTypes,
    debugcontrol_GetNumberProcessors,
    debugcontrol_GetSystemVersion,
    debugcontrol_GetPageSize,
    debugcontrol_IsPointer64Bit,
    debugcontrol_ReadBugCheckData,
    debugcontrol_GetNumberSupportedProcessorTypes,
    debugcontrol_GetSupportedProcessorTypes,
    debugcontrol_GetProcessorTypeNames,
    debugcontrol_GetEffectiveProcessorType,
    debugcontrol_SetEffectiveProcessorType,
    debugcontrol_GetExecutionStatus,
    debugcontrol_SetExecutionStatus,
    debugcontrol_GetCodeLevel,
    debugcontrol_SetCodeLevel,
    debugcontrol_GetEngineOptions,
    debugcontrol_AddEngineOptions,
    debugcontrol_RemoveEngineOptions,
    debugcontrol_SetEngineOptions,
    debugcontrol_GetSystemErrorControl,
    debugcontrol_SetSystemErrorControl,
    debugcontrol_GetTextMacro,
    debugcontrol_SetTextMacro,
    debugcontrol_GetRadix,
    debugcontrol_SetRadix,
    debugcontrol_Evaluate,
    debugcontrol_CoerceValue,
    debugcontrol_CoerceValues,
    debugcontrol_Execute,
    debugcontrol_ExecuteCommandFile,
    debugcontrol_GetNumberBreakpoints,
    debugcontrol_GetBreakpointByIndex,
    debugcontrol_GetBreakpointById,
    debugcontrol_GetBreakpointParameters,
    debugcontrol_AddBreakpoint,
    debugcontrol_RemoveBreakpoint,
    debugcontrol_AddExtension,
    debugcontrol_RemoveExtension,
    debugcontrol_GetExtensionByPath,
    debugcontrol_CallExtension,
    debugcontrol_GetExtensionFunction,
    debugcontrol_GetWindbgExtensionApis32,
    debugcontrol_GetWindbgExtensionApis64,
    debugcontrol_GetNumberEventFilters,
    debugcontrol_GetEventFilterText,
    debugcontrol_GetEventFilterCommand,
    debugcontrol_SetEventFilterCommand,
    debugcontrol_GetSpecificFilterParameters,
    debugcontrol_SetSpecificFilterParameters,
    debugcontrol_GetSpecificFilterArgument,
    debugcontrol_SetSpecificFilterArgument,
    debugcontrol_GetExceptionFilterParameters,
    debugcontrol_SetExceptionFilterParameters,
    debugcontrol_GetExceptionFilterSecondCommand,
    debugcontrol_SetExceptionFilterSecondCommand,
    debugcontrol_WaitForEvent,
    debugcontrol_GetLastEventInformation,
    debugcontrol_GetCurrentTimeDate,
    debugcontrol_GetCurrentSystemUpTime,
    debugcontrol_GetDumpFormatFlags,
    debugcontrol_GetNumberTextPlacements,
    debugcontrol_GetNumberTextReplacement,
    debugcontrol_SetTextReplacement,
    debugcontrol_RemoveTextReplacements,
    debugcontrol_OutputTextReplacements,
    debugcontrol_GetAssemblyOptions,
    debugcontrol_AddAssemblyOptions,
    debugcontrol_RemoveAssemblyOptions,
    debugcontrol_SetAssemblyOptions,
    debugcontrol_GetExpressionSyntax,
    debugcontrol_SetExpressionSyntax,
    debugcontrol_SetExpressionSyntaxByName,
    debugcontrol_GetNumberExpressionSyntaxes,
    debugcontrol_GetExpressionSyntaxNames,
    debugcontrol_GetNumberEvents,
    debugcontrol_GetEventIndexDescription,
    debugcontrol_GetCurrentEventIndex,
    debugcontrol_SetNextEventIndex,
    debugcontrol_GetLogFileWide,
    debugcontrol_OpenLogFileWide,
    debugcontrol_InputWide,
    debugcontrol_ReturnInputWide,
    debugcontrol_OutputWide,
    debugcontrol_OutputVaListWide,
    debugcontrol_ControlledOutputWide,
    debugcontrol_ControlledOutputVaListWide,
    debugcontrol_OutputPromptWide,
    debugcontrol_OutputPromptVaListWide,
    debugcontrol_GetPromptTextWide,
    debugcontrol_AssembleWide,
    debugcontrol_DisassembleWide,
    debugcontrol_GetProcessorTypeNamesWide,
    debugcontrol_GetTextMacroWide,
    debugcontrol_SetTextMacroWide,
    debugcontrol_EvaluateWide,
    debugcontrol_ExecuteWide,
    debugcontrol_ExecuteCommandFileWide,
    debugcontrol_GetBreakpointByIndex2,
    debugcontrol_GetBreakpointById2,
    debugcontrol_AddBreakpoint2,
    debugcontrol_RemoveBreakpoint2,
    debugcontrol_AddExtensionWide,
    debugcontrol_GetExtensionByPathWide,
    debugcontrol_CallExtensionWide,
    debugcontrol_GetExtensionFunctionWide,
    debugcontrol_GetEventFilterTextWide,
    debugcontrol_GetEventFilterCommandWide,
    debugcontrol_SetEventFilterCommandWide,
    debugcontrol_GetSpecificFilterArgumentWide,
    debugcontrol_SetSpecificFilterArgumentWide,
    debugcontrol_GetSpecificFilterSecondCommandWide,
    debugcontrol_SetSpecificFilterSecondCommandWide,
    debugcontrol_GetLastEventInformationWide,
    debugcontrol_GetTextReplacementWide,
    debugcontrol_SetTextReplacementWide,
    debugcontrol_SetExpressionSyntaxByNameWide,
    debugcontrol_GetExpressionSyntaxNamesWide,
    debugcontrol_GetEventIndexDescriptionWide,
    debugcontrol_GetLogFile2,
    debugcontrol_OpenLogFile2,
    debugcontrol_GetLogFile2Wide,
    debugcontrol_OpenLogFile2Wide,
    debugcontrol_GetSystemVersionValues,
    debugcontrol_GetSystemVersionString,
    debugcontrol_GetSystemVersionStringWide,
    debugcontrol_GetContextStackTrace,
    debugcontrol_OutputContextStackTrace,
    debugcontrol_GetStoredEventInformation,
    debugcontrol_GetManagedStatus,
    debugcontrol_GetManagedStatusWide,
    debugcontrol_ResetManagedStatus,
};

static HRESULT STDMETHODCALLTYPE debugadvanced_QueryInterface(IDebugAdvanced3 *iface, REFIID riid, void **obj)
{
    struct debug_client *debug_client = impl_from_IDebugAdvanced3(iface);
    return IUnknown_QueryInterface(&debug_client->IDebugClient_iface, riid, obj);
}

static ULONG STDMETHODCALLTYPE debugadvanced_AddRef(IDebugAdvanced3 *iface)
{
    struct debug_client *debug_client = impl_from_IDebugAdvanced3(iface);
    return IUnknown_AddRef(&debug_client->IDebugClient_iface);
}

static ULONG STDMETHODCALLTYPE debugadvanced_Release(IDebugAdvanced3 *iface)
{
    struct debug_client *debug_client = impl_from_IDebugAdvanced3(iface);
    return IUnknown_Release(&debug_client->IDebugClient_iface);
}

static HRESULT STDMETHODCALLTYPE debugadvanced_GetThreadContext(IDebugAdvanced3 *iface, void *context,
        ULONG context_size)
{
    FIXME("%p, %p, %lu stub.\n", iface, context, context_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugadvanced_SetThreadContext(IDebugAdvanced3 *iface, void *context,
        ULONG context_size)
{
    FIXME("%p, %p, %lu stub.\n", iface, context, context_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugadvanced_Request(IDebugAdvanced3 *iface, ULONG request, void *inbuffer,
        ULONG inbuffer_size, void *outbuffer, ULONG outbuffer_size, ULONG *outsize)
{
    FIXME("%p, %lu, %p, %lu, %p, %lu, %p stub.\n", iface, request, inbuffer, inbuffer_size, outbuffer, outbuffer_size, outsize);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugadvanced_GetSourceFileInformation(IDebugAdvanced3 *iface, ULONG which, char *sourcefile,
        ULONG64 arg64, ULONG arg32, void *buffer, ULONG buffer_size, ULONG *info_size)
{
    FIXME("%p, %lu, %p, %s, %#lx, %p, %lu, %p stub.\n", iface, which, sourcefile, wine_dbgstr_longlong(arg64),
            arg32, buffer, buffer_size, info_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugadvanced_FindSourceFileAndToken(IDebugAdvanced3 *iface, ULONG start_element,
        ULONG64 modaddr, const char *filename, ULONG flags, void *filetoken, ULONG filetokensize, ULONG *found_element,
        char *buffer, ULONG buffer_size, ULONG *found_size)
{
    FIXME("%p, %lu, %s, %s, %#lx, %p, %lu, %p, %p, %lu, %p stub.\n", iface, start_element, wine_dbgstr_longlong(modaddr),
            debugstr_a(filename), flags, filetoken, filetokensize, found_element, buffer, buffer_size, found_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugadvanced_GetSymbolInformation(IDebugAdvanced3 *iface, ULONG which, ULONG64 arg64,
        ULONG arg32, void *buffer, ULONG buffer_size, ULONG *info_size, char *string_buffer, ULONG string_buffer_size,
        ULONG *string_size)
{
    FIXME("%p, %lu, %s, %#lx, %p, %lu, %p, %p, %lu, %p stub.\n", iface, which, wine_dbgstr_longlong(arg64),
            arg32, buffer, buffer_size, info_size, string_buffer, string_buffer_size, string_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugadvanced_GetSystemObjectInformation(IDebugAdvanced3 *iface, ULONG which, ULONG64 arg64,
        ULONG arg32, void *buffer, ULONG buffer_size, ULONG *info_size)
{
    FIXME("%p, %lu, %s, %#lx, %p, %lu, %p stub.\n", iface, which, wine_dbgstr_longlong(arg64), arg32, buffer,
            buffer_size, info_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugadvanced_GetSourceFileInformationWide(IDebugAdvanced3 *iface, ULONG which,
        WCHAR *source_file, ULONG64 arg64, ULONG arg32, void *buffer, ULONG buffer_size, ULONG *info_size)
{
    FIXME("%p, %lu, %p, %s, %#lx, %p, %lu, %p stub.\n", iface, which, source_file, wine_dbgstr_longlong(arg64),
            arg32, buffer, buffer_size, info_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugadvanced_FindSourceFileAndTokenWide(IDebugAdvanced3 *iface, ULONG start_element,
        ULONG64 modaddr, const WCHAR *filename, ULONG flags, void *filetoken, ULONG filetokensize, ULONG *found_element,
        WCHAR *buffer, ULONG buffer_size, ULONG *found_size)
{
    FIXME("%p, %lu, %s, %s, %#lx, %p, %lu, %p, %p, %lu, %p stub.\n", iface, start_element, wine_dbgstr_longlong(modaddr),
            debugstr_w(filename), flags, filetoken, filetokensize, found_element, buffer, buffer_size, found_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugadvanced_GetSymbolInformationWide(IDebugAdvanced3 *iface, ULONG which, ULONG64 arg64,
        ULONG arg32, void *buffer, ULONG buffer_size, ULONG *info_size, WCHAR *string_buffer, ULONG string_buffer_size,
        ULONG *string_size)
{
    FIXME("%p, %lu, %s, %#lx, %p, %lu, %p, %p, %lu, %p stub.\n", iface, which, wine_dbgstr_longlong(arg64),
            arg32, buffer, buffer_size, info_size, string_buffer, string_buffer_size, string_size);

    return E_NOTIMPL;
}

static const IDebugAdvanced3Vtbl debugadvancedvtbl =
{
    debugadvanced_QueryInterface,
    debugadvanced_AddRef,
    debugadvanced_Release,
    debugadvanced_GetThreadContext,
    debugadvanced_SetThreadContext,
    debugadvanced_Request,
    debugadvanced_GetSourceFileInformation,
    debugadvanced_FindSourceFileAndToken,
    debugadvanced_GetSymbolInformation,
    debugadvanced_GetSystemObjectInformation,
    debugadvanced_GetSourceFileInformationWide,
    debugadvanced_FindSourceFileAndTokenWide,
    debugadvanced_GetSymbolInformationWide,
};

static HRESULT STDMETHODCALLTYPE debugsystemobjects_QueryInterface(IDebugSystemObjects *iface, REFIID riid, void **obj)
{
    struct debug_client *debug_client = impl_from_IDebugSystemObjects(iface);
    return IUnknown_QueryInterface(&debug_client->IDebugClient_iface, riid, obj);
}

static ULONG STDMETHODCALLTYPE debugsystemobjects_AddRef(IDebugSystemObjects *iface)
{
    struct debug_client *debug_client = impl_from_IDebugSystemObjects(iface);
    return IUnknown_AddRef(&debug_client->IDebugClient_iface);
}

static ULONG STDMETHODCALLTYPE debugsystemobjects_Release(IDebugSystemObjects *iface)
{
    struct debug_client *debug_client = impl_from_IDebugSystemObjects(iface);
    return IUnknown_Release(&debug_client->IDebugClient_iface);
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetEventThread(IDebugSystemObjects *iface, ULONG *id)
{
    FIXME("%p, %p stub.\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetEventProcess(IDebugSystemObjects *iface, ULONG *id)
{
    FIXME("%p, %p stub.\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetCurrentThreadId(IDebugSystemObjects *iface, ULONG *id)
{
    FIXME("%p, %p stub.\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_SetCurrentThreadId(IDebugSystemObjects *iface, ULONG id)
{
    FIXME("%p, %lu stub.\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_SetCurrentProcessId(IDebugSystemObjects *iface, ULONG id)
{
    FIXME("%p, %lu stub.\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetNumberThreads(IDebugSystemObjects *iface, ULONG *number)
{
    FIXME("%p, %p stub.\n", iface, number);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetTotalNumberThreads(IDebugSystemObjects *iface, ULONG *total,
        ULONG *largest_process)
{
    FIXME("%p, %p, %p stub.\n", iface, total, largest_process);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetThreadIdsByIndex(IDebugSystemObjects *iface, ULONG start,
        ULONG count, ULONG *ids, ULONG *sysids)
{
    FIXME("%p, %lu, %lu, %p, %p stub.\n", iface, start, count, ids, sysids);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetThreadIdByProcessor(IDebugSystemObjects *iface, ULONG processor,
        ULONG *id)
{
    FIXME("%p, %lu, %p stub.\n", iface, processor, id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetCurrentThreadDataOffset(IDebugSystemObjects *iface,
        ULONG64 *offset)
{
    FIXME("%p, %p stub.\n", iface, offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetThreadIdByDataOffset(IDebugSystemObjects *iface, ULONG64 offset,
        ULONG *id)
{
    FIXME("%p, %s, %p stub.\n", iface, wine_dbgstr_longlong(offset), id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetCurrentThreadTeb(IDebugSystemObjects *iface, ULONG64 *offset)
{
    FIXME("%p, %p stub.\n", iface, offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetThreadIdByTeb(IDebugSystemObjects *iface, ULONG64 offset,
        ULONG *id)
{
    FIXME("%p, %s, %p stub.\n", iface, wine_dbgstr_longlong(offset), id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetCurrentThreadSystemId(IDebugSystemObjects *iface, ULONG *sysid)
{
    FIXME("%p, %p stub.\n", iface, sysid);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetThreadIdBySystemId(IDebugSystemObjects *iface, ULONG sysid,
        ULONG *id)
{
    FIXME("%p, %lu, %p stub.\n", iface, sysid, id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetCurrentThreadHandle(IDebugSystemObjects *iface, ULONG64 *handle)
{
    FIXME("%p, %p stub.\n", iface, handle);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetThreadIdByHandle(IDebugSystemObjects *iface, ULONG64 handle,
        ULONG *id)
{
    FIXME("%p, %s, %p stub.\n", iface, wine_dbgstr_longlong(handle), id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetNumberProcesses(IDebugSystemObjects *iface, ULONG *number)
{
    FIXME("%p, %p stub.\n", iface, number);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetProcessIdsByIndex(IDebugSystemObjects *iface, ULONG start,
        ULONG count, ULONG *ids, ULONG *sysids)
{
    FIXME("%p, %lu, %lu, %p, %p stub.\n", iface, start, count, ids, sysids);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetCurrentProcessDataOffset(IDebugSystemObjects *iface,
        ULONG64 *offset)
{
    FIXME("%p, %p stub.\n", iface, offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetProcessIdByDataOffset(IDebugSystemObjects *iface,
        ULONG64 offset, ULONG *id)
{
    FIXME("%p, %s, %p stub.\n", iface, wine_dbgstr_longlong(offset), id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetCurrentProcessPeb(IDebugSystemObjects *iface, ULONG64 *offset)
{
    FIXME("%p, %p stub.\n", iface, offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetProcessIdByPeb(IDebugSystemObjects *iface, ULONG64 offset,
        ULONG *id)
{
    FIXME("%p, %s, %p stub.\n", iface, wine_dbgstr_longlong(offset), id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetCurrentProcessSystemId(IDebugSystemObjects *iface, ULONG *sysid)
{
    FIXME("%p, %p stub.\n", iface, sysid);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetProcessIdBySystemId(IDebugSystemObjects *iface, ULONG sysid,
        ULONG *id)
{
    FIXME("%p, %lu, %p stub.\n", iface, sysid, id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetCurrentProcessHandle(IDebugSystemObjects *iface,
        ULONG64 *handle)
{
    FIXME("%p, %p stub.\n", iface, handle);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetProcessIdByHandle(IDebugSystemObjects *iface, ULONG64 handle,
        ULONG *id)
{
    FIXME("%p, %s, %p stub.\n", iface, wine_dbgstr_longlong(handle), id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsystemobjects_GetCurrentProcessExecutableName(IDebugSystemObjects *iface,
        char *buffer, ULONG buffer_size, ULONG *exe_size)
{
    FIXME("%p, %p, %lu, %p stub.\n", iface, buffer, buffer_size, exe_size);

    return E_NOTIMPL;
}

static const IDebugSystemObjectsVtbl debugsystemobjectsvtbl =
{
    debugsystemobjects_QueryInterface,
    debugsystemobjects_AddRef,
    debugsystemobjects_Release,
    debugsystemobjects_GetEventThread,
    debugsystemobjects_GetEventProcess,
    debugsystemobjects_GetCurrentThreadId,
    debugsystemobjects_SetCurrentThreadId,
    debugsystemobjects_SetCurrentProcessId,
    debugsystemobjects_GetNumberThreads,
    debugsystemobjects_GetTotalNumberThreads,
    debugsystemobjects_GetThreadIdsByIndex,
    debugsystemobjects_GetThreadIdByProcessor,
    debugsystemobjects_GetCurrentThreadDataOffset,
    debugsystemobjects_GetThreadIdByDataOffset,
    debugsystemobjects_GetCurrentThreadTeb,
    debugsystemobjects_GetThreadIdByTeb,
    debugsystemobjects_GetCurrentThreadSystemId,
    debugsystemobjects_GetThreadIdBySystemId,
    debugsystemobjects_GetCurrentThreadHandle,
    debugsystemobjects_GetThreadIdByHandle,
    debugsystemobjects_GetNumberProcesses,
    debugsystemobjects_GetProcessIdsByIndex,
    debugsystemobjects_GetCurrentProcessDataOffset,
    debugsystemobjects_GetProcessIdByDataOffset,
    debugsystemobjects_GetCurrentProcessPeb,
    debugsystemobjects_GetProcessIdByPeb,
    debugsystemobjects_GetCurrentProcessSystemId,
    debugsystemobjects_GetProcessIdBySystemId,
    debugsystemobjects_GetCurrentProcessHandle,
    debugsystemobjects_GetProcessIdByHandle,
    debugsystemobjects_GetCurrentProcessExecutableName,
};

/************************************************************
*                    DebugExtensionInitialize   (DBGENG.@)
*
* Initializing Debug Engine
*
* PARAMS
*   pVersion  [O] Receiving the version of extension
*   pFlags    [O] Reserved
*
* RETURNS
*   Success: S_OK
*   Failure: Anything other than S_OK
*
* BUGS
*   Unimplemented
*/
HRESULT WINAPI DebugExtensionInitialize(ULONG * pVersion, ULONG * pFlags)
{
    FIXME("(%p,%p): stub\n", pVersion, pFlags);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return E_NOTIMPL;
}

/************************************************************
*                    DebugCreate   (dbgeng.@)
*/
HRESULT WINAPI DebugCreate(REFIID riid, void **obj)
{
    struct debug_client *debug_client;
    HRESULT hr;

    TRACE("%s, %p.\n", debugstr_guid(riid), obj);

    if (!(debug_client = calloc(1, sizeof(*debug_client))))
        return E_OUTOFMEMORY;

    debug_client->IDebugClient_iface.lpVtbl = &debugclientvtbl;
    debug_client->IDebugDataSpaces_iface.lpVtbl = &debugdataspacesvtbl;
    debug_client->IDebugSymbols3_iface.lpVtbl = &debugsymbolsvtbl;
    debug_client->IDebugControl4_iface.lpVtbl = &debugcontrolvtbl;
    debug_client->IDebugAdvanced3_iface.lpVtbl = &debugadvancedvtbl;
    debug_client->IDebugSystemObjects_iface.lpVtbl = &debugsystemobjectsvtbl;
    debug_client->refcount = 1;
    list_init(&debug_client->targets);

    hr = IUnknown_QueryInterface(&debug_client->IDebugClient_iface, riid, obj);
    IUnknown_Release(&debug_client->IDebugClient_iface);

    return hr;
}

/************************************************************
*                    DebugCreateEx   (DBGENG.@)
*/
HRESULT WINAPI DebugCreateEx(REFIID riid, DWORD flags, void **obj)
{
    FIXME("%s, %#lx, %p: stub\n", debugstr_guid(riid), flags, obj);

    return E_NOTIMPL;
}

/************************************************************
*                    DebugConnect   (DBGENG.@)
*
* Creating Debug Engine client object and connecting it to remote host
*
* PARAMS
*   RemoteOptions [I] Options which define how debugger engine connects to remote host
*   InterfaceId   [I] Interface Id of debugger client
*   pInterface    [O] Pointer to interface as requested via InterfaceId
*
* RETURNS
*   Success: S_OK
*   Failure: Anything other than S_OK
*
* BUGS
*   Unimplemented
*/
HRESULT WINAPI DebugConnect(PCSTR RemoteOptions, REFIID InterfaceId, PVOID * pInterface)
{
    FIXME("(%p,%p,%p): stub\n", RemoteOptions, InterfaceId, pInterface);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return E_NOTIMPL;
}
