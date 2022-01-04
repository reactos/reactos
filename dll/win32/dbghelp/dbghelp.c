/*
 * File dbghelp.c - generic routines (process) for dbghelp DLL
 *
 * Copyright (C) 2004, Eric Pouech
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

#ifndef DBGHELP_STATIC_LIB

#include <unistd.h>
#include "dbghelp_private.h"
#include "winternl.h"
#include "winerror.h"
#include "psapi.h"
#include "wine/debug.h"
#include "wdbgexts.h"
#include "winnls.h"
#else
#include "dbghelp_private.h"
#include "wdbgexts.h"
#endif

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

/* TODO
 *  - support for symbols' types is still partly missing
 *      + C++ support
 *      + we should store the underlying type for an enum in the symt_enum struct
 *      + for enums, we store the names & values (associated to the enum type),
 *        but those values are not directly usable from a debugger (that's why, I
 *        assume, that we have also to define constants for enum values, as
 *        Codeview does BTW.
 *      + SymEnumTypes should only return *user* defined types (UDT, typedefs...) not
 *        all the types stored/used in the modules (like char*)
 *  - SymGetLine{Next|Prev} don't work as expected (they don't seem to work across
 *    functions, and even across function blocks...). Basically, for *Next* to work
 *    it requires an address after the prolog of the func (the base address of the
 *    func doesn't work)
 *  - most options (dbghelp_options) are not used (loading lines...)
 *  - in symbol lookup by name, we don't use RE everywhere we should. Moreover, when
 *    we're supposed to use RE, it doesn't make use of our hash tables. Therefore,
 *    we could use hash if name isn't a RE, and fall back to a full search when we
 *    get a full RE
 *  - msc:
 *      + we should add parameters' types to the function's signature
 *        while processing a function's parameters
 *      + add support for function-less labels (as MSC seems to define them)
 *      + C++ management
 *  - stabs:
 *      + when, in a same module, the same definition is used in several compilation
 *        units, we get several definitions of the same object (especially
 *        struct/union). we should find a way not to duplicate them
 *      + in some cases (dlls/user/dialog16.c DIALOG_GetControl16), the same static
 *        global variable is defined several times (at different scopes). We are
 *        getting several of those while looking for a unique symbol. Part of the
 *        issue is that we don't give a scope to a static variable inside a function
 *      + C++ management
 */

unsigned   dbghelp_options = SYMOPT_UNDNAME;
BOOL       dbghelp_opt_native = FALSE;
#ifndef DBGHELP_STATIC_LIB
SYSTEM_INFO sysinfo;
#endif

static struct process* process_first /* = NULL */;

#ifndef DBGHELP_STATIC_LIB
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        GetSystemInfo(&sysinfo);
        DisableThreadLibraryCalls(instance);
        break;
    }
    return TRUE;
}
#endif

/******************************************************************
 *		process_find_by_handle
 *
 */
struct process*    process_find_by_handle(HANDLE hProcess)
{
    struct process* p;

    for (p = process_first; p && p->handle != hProcess; p = p->next);
    if (!p) SetLastError(ERROR_INVALID_HANDLE);
    return p;
}

/******************************************************************
 *             validate_addr64 (internal)
 *
 */
BOOL validate_addr64(DWORD64 addr)
{
    if (sizeof(void*) == sizeof(int) && (addr >> 32))
    {
        FIXME("Unsupported address %s\n", wine_dbgstr_longlong(addr));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return TRUE;
}

/******************************************************************
 *		fetch_buffer
 *
 * Ensures process' internal buffer is large enough.
 */
void* fetch_buffer(struct process* pcs, unsigned size)
{
    if (size > pcs->buffer_size)
    {
        if (pcs->buffer)
            pcs->buffer = HeapReAlloc(GetProcessHeap(), 0, pcs->buffer, size);
        else
            pcs->buffer = HeapAlloc(GetProcessHeap(), 0, size);
        pcs->buffer_size = (pcs->buffer) ? size : 0;
    }
    return pcs->buffer;
}

#ifndef DBGHELP_STATIC_LIB
const char* wine_dbgstr_addr(const ADDRESS64* addr)
{
    if (!addr) return "(null)";
    switch (addr->Mode)
    {
    case AddrModeFlat:
        return wine_dbg_sprintf("flat<%s>", wine_dbgstr_longlong(addr->Offset));
    case AddrMode1616:
        return wine_dbg_sprintf("1616<%04x:%04x>", addr->Segment, (DWORD)addr->Offset);
    case AddrMode1632:
        return wine_dbg_sprintf("1632<%04x:%08x>", addr->Segment, (DWORD)addr->Offset);
    case AddrModeReal:
        return wine_dbg_sprintf("real<%04x:%04x>", addr->Segment, (DWORD)addr->Offset);
    default:
        return "unknown";
    }
}
#endif

extern struct cpu       cpu_i386, cpu_x86_64, cpu_arm, cpu_arm64;

#ifndef DBGHELP_STATIC_LIB
static struct cpu*      dbghelp_cpus[] = {&cpu_i386, &cpu_x86_64, &cpu_arm, &cpu_arm64, NULL};
#else
static struct cpu*      dbghelp_cpus[] = {&cpu_i386, NULL};
#endif

struct cpu*             dbghelp_current_cpu =
#if defined(__i386__) || defined(DBGHELP_STATIC_LIB)
    &cpu_i386
#elif defined(__x86_64__)
    &cpu_x86_64
#elif defined(__arm__)
    &cpu_arm
#elif defined(__aarch64__)
    &cpu_arm64
#else
#error define support for your CPU
#endif
    ;

struct cpu* cpu_find(DWORD machine)
{
    struct cpu** cpu;

    for (cpu = dbghelp_cpus ; *cpu; cpu++)
    {
        if (cpu[0]->machine == machine) return cpu[0];
    }
    return NULL;
}

static WCHAR* make_default_search_path(void)
{
    WCHAR*      search_path;
    WCHAR*      p;
    unsigned    sym_path_len;
    unsigned    alt_sym_path_len;

    sym_path_len = GetEnvironmentVariableW(L"_NT_SYMBOL_PATH", NULL, 0);
    alt_sym_path_len = GetEnvironmentVariableW(L"_NT_ALT_SYMBOL_PATH", NULL, 0);

    /* The default symbol path is ".[;%_NT_SYMBOL_PATH%][;%_NT_ALT_SYMBOL_PATH%]".
     * If the variables exist, the lengths include a null-terminator. We use that
     * space for the semicolons, and only add the initial dot and the final null. */
    search_path = HeapAlloc(GetProcessHeap(), 0,
                            (1 + sym_path_len + alt_sym_path_len + 1) * sizeof(WCHAR));
    if (!search_path) return NULL;

    p = search_path;
    *p++ = L'.';
    if (sym_path_len)
    {
        *p++ = L';';
        GetEnvironmentVariableW(L"_NT_SYMBOL_PATH", p, sym_path_len);
        p += sym_path_len - 1;
    }

    if (alt_sym_path_len)
    {
        *p++ = L';';
        GetEnvironmentVariableW(L"_NT_ALT_SYMBOL_PATH", p, alt_sym_path_len);
        p += alt_sym_path_len - 1;
    }
    *p = L'\0';

    return search_path;
}

/******************************************************************
 *		SymSetSearchPathW (DBGHELP.@)
 *
 */
BOOL WINAPI SymSetSearchPathW(HANDLE hProcess, PCWSTR searchPath)
{
    struct process* pcs = process_find_by_handle(hProcess);
    WCHAR*          search_path_buffer;

    if (!pcs) return FALSE;

    if (searchPath)
    {
        search_path_buffer = HeapAlloc(GetProcessHeap(), 0,
                                       (lstrlenW(searchPath) + 1) * sizeof(WCHAR));
        if (!search_path_buffer) return FALSE;
        lstrcpyW(search_path_buffer, searchPath);
    }
    else
    {
        search_path_buffer = make_default_search_path();
        if (!search_path_buffer) return FALSE;
    }
    HeapFree(GetProcessHeap(), 0, pcs->search_path);
    pcs->search_path = search_path_buffer;
    return TRUE;
}

/******************************************************************
 *		SymSetSearchPath (DBGHELP.@)
 *
 */
BOOL WINAPI SymSetSearchPath(HANDLE hProcess, PCSTR searchPath)
{
    BOOL        ret = FALSE;
    unsigned    len;
    WCHAR*      sp = NULL;

    if (searchPath)
    {
        len = MultiByteToWideChar(CP_ACP, 0, searchPath, -1, NULL, 0);
        sp = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!sp) return FALSE;
        MultiByteToWideChar(CP_ACP, 0, searchPath, -1, sp, len);
    }

    ret = SymSetSearchPathW(hProcess, sp);

    HeapFree(GetProcessHeap(), 0, sp);
    return ret;
}

/***********************************************************************
 *		SymGetSearchPathW (DBGHELP.@)
 */
BOOL WINAPI SymGetSearchPathW(HANDLE hProcess, PWSTR szSearchPath,
                              DWORD SearchPathLength)
{
    struct process* pcs = process_find_by_handle(hProcess);
    if (!pcs) return FALSE;

    lstrcpynW(szSearchPath, pcs->search_path, SearchPathLength);
    return TRUE;
}

/***********************************************************************
 *		SymGetSearchPath (DBGHELP.@)
 */
BOOL WINAPI SymGetSearchPath(HANDLE hProcess, PSTR szSearchPath,
                             DWORD SearchPathLength)
{
    WCHAR*      buffer = HeapAlloc(GetProcessHeap(), 0, SearchPathLength * sizeof(WCHAR));
    BOOL        ret = FALSE;

    if (buffer)
    {
        ret = SymGetSearchPathW(hProcess, buffer, SearchPathLength);
        if (ret)
            WideCharToMultiByte(CP_ACP, 0, buffer, SearchPathLength,
                                szSearchPath, SearchPathLength, NULL, NULL);
        HeapFree(GetProcessHeap(), 0, buffer);
    }
    return ret;
}

#ifndef DBGHELP_STATIC_LIB
/******************************************************************
 *		invade_process
 *
 * SymInitialize helper: loads in dbghelp all known (and loaded modules)
 * this assumes that hProcess is a handle on a valid process
 */
static BOOL WINAPI process_invade_cb(PCWSTR name, ULONG64 base, ULONG size, PVOID user)
{
    WCHAR       tmp[MAX_PATH];
    HANDLE      hProcess = user;

    if (!GetModuleFileNameExW(hProcess, (HMODULE)(DWORD_PTR)base, tmp, ARRAY_SIZE(tmp)))
        lstrcpynW(tmp, name, ARRAY_SIZE(tmp));

    SymLoadModuleExW(hProcess, 0, tmp, name, base, size, NULL, 0);
    return TRUE;
}

const WCHAR *process_getenv(const struct process *process, const WCHAR *name)
{
    size_t name_len;
    const WCHAR *iter;

    if (!process->environment) return NULL;
    name_len = lstrlenW(name);

    for (iter = process->environment; *iter; iter += lstrlenW(iter) + 1)
    {
        if (!wcsnicmp(iter, name, name_len) && iter[name_len] == '=')
            return iter + name_len + 1;
    }

    return NULL;
}

/******************************************************************
 *		check_live_target
 *
 */
static BOOL check_live_target(struct process* pcs)
{
    PROCESS_BASIC_INFORMATION pbi;
    ULONG_PTR base = 0, env = 0;

    if (!GetProcessId(pcs->handle)) return FALSE;
    if (GetEnvironmentVariableA("DBGHELP_NOLIVE", NULL, 0)) return FALSE;

    if (NtQueryInformationProcess( pcs->handle, ProcessBasicInformation,
                                   &pbi, sizeof(pbi), NULL ))
        return FALSE;

    if (!pcs->is_64bit)
    {
        DWORD env32;
        PEB32 peb32;
        C_ASSERT(sizeof(void*) != 4 || FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, Environment) == 0x48);
        if (!ReadProcessMemory(pcs->handle, pbi.PebBaseAddress, &peb32, sizeof(peb32), NULL)) return FALSE;
        base = peb32.Reserved[0];
        if (read_process_memory(pcs, peb32.ProcessParameters + 0x48, &env32, sizeof(env32))) env = env32;
    }
    else
    {
        PEB peb;
        if (!ReadProcessMemory(pcs->handle, pbi.PebBaseAddress, &peb, sizeof(peb), NULL)) return FALSE;
        base = peb.Reserved[0];
        ReadProcessMemory(pcs->handle, &peb.ProcessParameters->Environment, &env, sizeof(env), NULL);
    }

#ifdef __REACTOS__
    /* Wine store their loader base address in peb.reserved[0] and load its symbol from there.
     * ReactOS does not care about it, we are just happy if we managed to read the value */
    base = 1;
#endif

    /* read debuggee environment block */
    if (env)
    {
        size_t buf_size = 0, i, last_null = -1;
        WCHAR *buf = NULL;

        do
        {
            size_t read_size = sysinfo.dwAllocationGranularity - (env & (sysinfo.dwAllocationGranularity - 1));
            if (buf)
            {
                WCHAR *new_buf;
                if (!(new_buf = realloc(buf, buf_size + read_size))) break;
                buf = new_buf;
            }
            else if(!(buf = malloc(read_size))) break;

            if (!read_process_memory(pcs, env, (char*)buf + buf_size, read_size)) break;
            for (i = buf_size / sizeof(WCHAR); i < (buf_size + read_size) / sizeof(WCHAR); i++)
            {
                if (buf[i]) continue;
                if (last_null + 1 == i)
                {
                    pcs->environment = realloc(buf, (i + 1) * sizeof(WCHAR));
                    buf = NULL;
                    break;
                }
                last_null = i;
            }
            env += read_size;
            buf_size += read_size;
        }
        while (buf);
        free(buf);
    }

    if (!base) return FALSE;

    TRACE("got debug info address %#lx from PEB %p\n", base, pbi.PebBaseAddress);
#ifndef __REACTOS__
    if (!elf_read_wine_loader_dbg_info(pcs, base) && !macho_read_wine_loader_dbg_info(pcs, base))
        WARN("couldn't load process debug info at %#lx\n", base);
#endif
    return TRUE;
}
#endif

/******************************************************************
 *		SymInitializeW (DBGHELP.@)
 *
 * The initialisation of a dbghelp's context.
 * Note that hProcess doesn't need to be a valid process handle (except
 * when fInvadeProcess is TRUE).
 * Since we also allow loading ELF (pure) libraries and Wine ELF libraries
 * containing PE (and NE) module(s), here's how we handle it:
 * - we load every module (ELF, NE, PE) passed in SymLoadModule
 * - in fInvadeProcess (in SymInitialize) is TRUE, we set up what is called ELF
 *   synchronization: hProcess should be a valid process handle, and we hook
 *   ourselves on hProcess's loaded ELF-modules, and keep this list in sync with
 *   our internal ELF modules representation (loading / unloading). This way,
 *   we'll pair every loaded builtin PE module with its ELF counterpart (and
 *   access its debug information).
 * - if fInvadeProcess (in SymInitialize) is FALSE, we check anyway if the
 *   hProcess refers to a running process. We use some heuristics here, so YMMV.
 *   If we detect a live target, then we get the same handling as if
 *   fInvadeProcess is TRUE (except that the modules are not loaded). Otherwise,
 *   we won't be able to make the peering between a builtin PE module and its ELF
 *   counterpart. Hence we won't be able to provide the requested debug
 *   information. We'll however be able to load native PE modules (and their
 *   debug information) without any trouble.
 * Note also that this scheme can be intertwined with the deferred loading
 * mechanism (ie only load the debug information when we actually need it).
 */
BOOL WINAPI SymInitializeW(HANDLE hProcess, PCWSTR UserSearchPath, BOOL fInvadeProcess)
{
    struct process*     pcs;
    BOOL wow64, child_wow64;

    TRACE("(%p %s %u)\n", hProcess, debugstr_w(UserSearchPath), fInvadeProcess);

    if (process_find_by_handle(hProcess))
    {
        WARN("the symbols for this process have already been initialized!\n");

        /* MSDN says to only call this function once unless SymCleanup() has been called since the last call.
           It also says to call SymRefreshModuleList() instead if you just want the module list refreshed.
           Native still returns TRUE even if the process has already been initialized. */
        return TRUE;
    }

    IsWow64Process(GetCurrentProcess(), &wow64);

    if (GetProcessId(hProcess) && !IsWow64Process(hProcess, &child_wow64))
        return FALSE;

    pcs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pcs));
    if (!pcs) return FALSE;

    pcs->handle = hProcess;
    pcs->is_64bit = (sizeof(void *) == 8 || wow64) && !child_wow64;
    pcs->loader = &no_loader_ops; /* platform-specific initialization will override it if loader debug info can be found */

    if (UserSearchPath)
    {
        pcs->search_path = lstrcpyW(HeapAlloc(GetProcessHeap(), 0,      
                                              (lstrlenW(UserSearchPath) + 1) * sizeof(WCHAR)),
                                    UserSearchPath);
    }
    else
    {
        pcs->search_path = make_default_search_path();
    }

    pcs->lmodules = NULL;
    pcs->dbg_hdr_addr = 0;
    pcs->next = process_first;
    process_first = pcs;

#ifndef DBGHELP_STATIC_LIB
    if (check_live_target(pcs))
    {
        if (fInvadeProcess)
            EnumerateLoadedModulesW64(hProcess, process_invade_cb, hProcess);
        if (pcs->loader) pcs->loader->synchronize_module_list(pcs);
    }
    else if (fInvadeProcess)
#else
    if (fInvadeProcess)
#endif
    {
        SymCleanup(hProcess);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return TRUE;
}

/******************************************************************
 *		SymInitialize (DBGHELP.@)
 *
 *
 */
BOOL WINAPI SymInitialize(HANDLE hProcess, PCSTR UserSearchPath, BOOL fInvadeProcess)
{
    WCHAR*              sp = NULL;
    BOOL                ret;

    if (UserSearchPath)
    {
        unsigned len;

        len = MultiByteToWideChar(CP_ACP, 0, UserSearchPath, -1, NULL, 0);
        sp = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, UserSearchPath, -1, sp, len);
    }

    ret = SymInitializeW(hProcess, sp, fInvadeProcess);
    HeapFree(GetProcessHeap(), 0, sp);
    return ret;
}

/******************************************************************
 *		SymCleanup (DBGHELP.@)
 *
 */
BOOL WINAPI SymCleanup(HANDLE hProcess)
{
    struct process**    ppcs;
    struct process*     next;

    for (ppcs = &process_first; *ppcs; ppcs = &(*ppcs)->next)
    {
        if ((*ppcs)->handle == hProcess)
        {
            while ((*ppcs)->lmodules) module_remove(*ppcs, (*ppcs)->lmodules);

            HeapFree(GetProcessHeap(), 0, (*ppcs)->search_path);
            free((*ppcs)->environment);
            next = (*ppcs)->next;
            HeapFree(GetProcessHeap(), 0, *ppcs);
            *ppcs = next;
            return TRUE;
        }
    }

    ERR("this process has not had SymInitialize() called for it!\n");
    return FALSE;
}

/******************************************************************
 *		SymSetOptions (DBGHELP.@)
 *
 */
DWORD WINAPI SymSetOptions(DWORD opts)
{
    struct process* pcs;

    for (pcs = process_first; pcs; pcs = pcs->next)
    {
        pcs_callback(pcs, CBA_SET_OPTIONS, &opts);
    }
    return dbghelp_options = opts;
}

/******************************************************************
 *		SymGetOptions (DBGHELP.@)
 *
 */
DWORD WINAPI SymGetOptions(void)
{
    return dbghelp_options;
}

/******************************************************************
 *		SymSetExtendedOption (DBGHELP.@)
 *
 */
BOOL WINAPI SymSetExtendedOption(IMAGEHLP_EXTENDED_OPTIONS option, BOOL value)
{
    BOOL old = FALSE;

    switch(option)
    {
        case SYMOPT_EX_WINE_NATIVE_MODULES:
            old = dbghelp_opt_native;
            dbghelp_opt_native = value;
            break;
        default:
            FIXME("Unsupported option %d with value %d\n", option, value);
    }

    return old;
}

/******************************************************************
 *		SymGetExtendedOption (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetExtendedOption(IMAGEHLP_EXTENDED_OPTIONS option)
{
    switch(option)
    {
        case SYMOPT_EX_WINE_NATIVE_MODULES:
            return dbghelp_opt_native;
        default:
            FIXME("Unsupported option %d\n", option);
    }

    return FALSE;
}

/******************************************************************
 *		SymSetParentWindow (DBGHELP.@)
 *
 */
BOOL WINAPI SymSetParentWindow(HWND hwnd)
{
    /* Save hwnd so it can be used as parent window */
    FIXME("(%p): stub\n", hwnd);
    return TRUE;
}

/******************************************************************
 *		SymSetContext (DBGHELP.@)
 *
 */
BOOL WINAPI SymSetContext(HANDLE hProcess, PIMAGEHLP_STACK_FRAME StackFrame,
                          PIMAGEHLP_CONTEXT Context)
{
    struct process* pcs = process_find_by_handle(hProcess);
    if (!pcs) return FALSE;

    if (pcs->ctx_frame.ReturnOffset == StackFrame->ReturnOffset &&
        pcs->ctx_frame.FrameOffset  == StackFrame->FrameOffset  &&
        pcs->ctx_frame.StackOffset  == StackFrame->StackOffset)
    {
        TRACE("Setting same frame {rtn=%s frm=%s stk=%s}\n",
              wine_dbgstr_longlong(pcs->ctx_frame.ReturnOffset),
              wine_dbgstr_longlong(pcs->ctx_frame.FrameOffset),
              wine_dbgstr_longlong(pcs->ctx_frame.StackOffset));
        pcs->ctx_frame.InstructionOffset = StackFrame->InstructionOffset;
        SetLastError(ERROR_ACCESS_DENIED); /* latest MSDN says ERROR_SUCCESS */
        return FALSE;
    }

    pcs->ctx_frame = *StackFrame;
    /* MSDN states that Context is not (no longer?) used */
    return TRUE;
}

/******************************************************************
 *		reg_cb64to32 (internal)
 *
 * Registered callback for converting information from 64 bit to 32 bit
 */
static BOOL CALLBACK reg_cb64to32(HANDLE hProcess, ULONG action, ULONG64 data, ULONG64 user)
{
    struct process*                     pcs = process_find_by_handle(hProcess);
    void*                               data32;
    IMAGEHLP_DEFERRED_SYMBOL_LOAD64*    idsl64;
    IMAGEHLP_DEFERRED_SYMBOL_LOAD       idsl;

    if (!pcs) return FALSE;
    switch (action)
    {
    case CBA_DEBUG_INFO:
    case CBA_DEFERRED_SYMBOL_LOAD_CANCEL:
    case CBA_SET_OPTIONS:
    case CBA_SYMBOLS_UNLOADED:
        data32 = (void*)(DWORD_PTR)data;
        break;
    case CBA_DEFERRED_SYMBOL_LOAD_COMPLETE:
    case CBA_DEFERRED_SYMBOL_LOAD_FAILURE:
    case CBA_DEFERRED_SYMBOL_LOAD_PARTIAL:
    case CBA_DEFERRED_SYMBOL_LOAD_START:
        idsl64 = (IMAGEHLP_DEFERRED_SYMBOL_LOAD64*)(DWORD_PTR)data;
        if (!validate_addr64(idsl64->BaseOfImage))
            return FALSE;
        idsl.SizeOfStruct = sizeof(idsl);
        idsl.BaseOfImage = (DWORD)idsl64->BaseOfImage;
        idsl.CheckSum = idsl64->CheckSum;
        idsl.TimeDateStamp = idsl64->TimeDateStamp;
        memcpy(idsl.FileName, idsl64->FileName, sizeof(idsl.FileName));
        idsl.Reparse = idsl64->Reparse;
        data32 = &idsl;
        break;
    case CBA_DUPLICATE_SYMBOL:
    case CBA_EVENT:
    case CBA_READ_MEMORY:
    default:
        FIXME("No mapping for action %u\n", action);
        return FALSE;
    }
    return pcs->reg_cb32(hProcess, action, data32, (PVOID)(DWORD_PTR)user);
}

/******************************************************************
 *		pcs_callback (internal)
 */
BOOL pcs_callback(const struct process* pcs, ULONG action, void* data)
{
    IMAGEHLP_DEFERRED_SYMBOL_LOAD64 idsl;

    TRACE("%p %u %p\n", pcs, action, data);

    if (!pcs->reg_cb) return FALSE;
    if (!pcs->reg_is_unicode)
    {
        IMAGEHLP_DEFERRED_SYMBOL_LOADW64*   idslW;

        switch (action)
        {
        case CBA_DEBUG_INFO:
        case CBA_DEFERRED_SYMBOL_LOAD_CANCEL:
        case CBA_SET_OPTIONS:
        case CBA_SYMBOLS_UNLOADED:
            break;
        case CBA_DEFERRED_SYMBOL_LOAD_COMPLETE:
        case CBA_DEFERRED_SYMBOL_LOAD_FAILURE:
        case CBA_DEFERRED_SYMBOL_LOAD_PARTIAL:
        case CBA_DEFERRED_SYMBOL_LOAD_START:
            idslW = data;
            idsl.SizeOfStruct = sizeof(idsl);
            idsl.BaseOfImage = idslW->BaseOfImage;
            idsl.CheckSum = idslW->CheckSum;
            idsl.TimeDateStamp = idslW->TimeDateStamp;
            WideCharToMultiByte(CP_ACP, 0, idslW->FileName, -1,
                                idsl.FileName, sizeof(idsl.FileName), NULL, NULL);
            idsl.Reparse = idslW->Reparse;
            data = &idsl;
            break;
        case CBA_DUPLICATE_SYMBOL:
        case CBA_EVENT:
        case CBA_READ_MEMORY:
        default:
            FIXME("No mapping for action %u\n", action);
            return FALSE;
        }
    }
    return pcs->reg_cb(pcs->handle, action, (ULONG64)(DWORD_PTR)data, pcs->reg_user);
}

/******************************************************************
 *		sym_register_cb
 *
 * Helper for registering a callback.
 */
static BOOL sym_register_cb(HANDLE hProcess,
                            PSYMBOL_REGISTERED_CALLBACK64 cb,
                            PSYMBOL_REGISTERED_CALLBACK cb32,
                            DWORD64 user, BOOL unicode)
{
    struct process* pcs = process_find_by_handle(hProcess);

    if (!pcs) return FALSE;
    pcs->reg_cb = cb;
    pcs->reg_cb32 = cb32;
    pcs->reg_is_unicode = unicode;
    pcs->reg_user = user;

    return TRUE;
}

/***********************************************************************
 *		SymRegisterCallback (DBGHELP.@)
 */
BOOL WINAPI SymRegisterCallback(HANDLE hProcess,
                                PSYMBOL_REGISTERED_CALLBACK CallbackFunction,
                                PVOID UserContext)
{
    TRACE("(%p, %p, %p)\n",
          hProcess, CallbackFunction, UserContext);
    return sym_register_cb(hProcess, reg_cb64to32, CallbackFunction, (DWORD_PTR)UserContext, FALSE);
}

/***********************************************************************
 *		SymRegisterCallback64 (DBGHELP.@)
 */
BOOL WINAPI SymRegisterCallback64(HANDLE hProcess,
                                  PSYMBOL_REGISTERED_CALLBACK64 CallbackFunction,
                                  ULONG64 UserContext)
{
    TRACE("(%p, %p, %s)\n",
          hProcess, CallbackFunction, wine_dbgstr_longlong(UserContext));
    return sym_register_cb(hProcess, CallbackFunction, NULL, UserContext, FALSE);
}

/***********************************************************************
 *		SymRegisterCallbackW64 (DBGHELP.@)
 */
BOOL WINAPI SymRegisterCallbackW64(HANDLE hProcess,
                                   PSYMBOL_REGISTERED_CALLBACK64 CallbackFunction,
                                   ULONG64 UserContext)
{
    TRACE("(%p, %p, %s)\n",
          hProcess, CallbackFunction, wine_dbgstr_longlong(UserContext));
    return sym_register_cb(hProcess, CallbackFunction, NULL, UserContext, TRUE);
}

/* This is imagehlp version not dbghelp !! */
static API_VERSION api_version = { 4, 0, 2, 0 };

/***********************************************************************
 *           ImagehlpApiVersion (DBGHELP.@)
 */
LPAPI_VERSION WINAPI ImagehlpApiVersion(VOID)
{
    return &api_version;
}

/***********************************************************************
 *           ImagehlpApiVersionEx (DBGHELP.@)
 */
LPAPI_VERSION WINAPI ImagehlpApiVersionEx(LPAPI_VERSION AppVersion)
{
    if (!AppVersion) return NULL;

    AppVersion->MajorVersion = api_version.MajorVersion;
    AppVersion->MinorVersion = api_version.MinorVersion;
    AppVersion->Revision = api_version.Revision;
    AppVersion->Reserved = api_version.Reserved;

    return AppVersion;
}

/******************************************************************
 *		ExtensionApiVersion (DBGHELP.@)
 */
LPEXT_API_VERSION WINAPI ExtensionApiVersion(void)
{
    static EXT_API_VERSION      eav = {5, 5, 5, 0};
    return &eav;
}

/******************************************************************
 *		WinDbgExtensionDllInit (DBGHELP.@)
 */
void WINAPI WinDbgExtensionDllInit(PWINDBG_EXTENSION_APIS lpExtensionApis,
                                   unsigned short major, unsigned short minor)
{
}

DWORD calc_crc32(HANDLE handle)
{
    BYTE buffer[8192];
    DWORD crc = 0;
    DWORD len;

    SetFilePointer(handle, 0, 0, FILE_BEGIN);
    while (ReadFile(handle, buffer, sizeof(buffer), &len, NULL) && len)
        crc = RtlComputeCrc32(crc, buffer, len);
    return crc;
}
