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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "dbghelp_private.h"
#include "winerror.h"
#include "psapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

/* TODO
 *  - support for symbols' types is still partly missing
 *      + C++ support
 *      + funcargtype:s are (partly) wrong: they should be a specific struct (like
 *        typedef) pointing to the actual type (and not a direct access)
 *      + we should store the underlying type for an enum in the symt_enum struct
 *      + for enums, we store the names & values (associated to the enum type), 
 *        but those values are not directly usable from a debugger (that's why, I
 *        assume, that we have also to define constants for enum values, as 
 *        Codeview does BTW.
 *  - most options (dbghelp_options) are not used (loading lines...)
 *  - in symbol lookup by name, we don't use RE everywhere we should. Moreover, when
 *    we're supposed to use RE, it doesn't make use of our hash tables. Therefore,
 *    we could use hash if name isn't a RE, and fall back to a full search when we
 *    get a full RE
 *  - msc:
 *      + we should add parameters' types to the function's signature
 *        while processing a function's parameters
 *      + get rid of MSC reading FIXME:s (lots of types are not defined)
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
 *  - implement the callback notification mechanism
 */

unsigned   dbghelp_options = SYMOPT_UNDNAME;
HANDLE     hMsvcrt = NULL;

/***********************************************************************
 *           DllMain (DEBUGHLP.@)
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:    break;
    case DLL_PROCESS_DETACH:
        if (hMsvcrt) FreeLibrary(hMsvcrt);
        break;
    case DLL_THREAD_ATTACH:     break;
    case DLL_THREAD_DETACH:     break;
    default:                    break;
    }
    return TRUE;
}

static struct process* process_first /* = NULL */;

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
 *		SymSetSearchPath (DBGHELP.@)
 *
 */
BOOL WINAPI SymSetSearchPath(HANDLE hProcess, PSTR searchPath)
{
    struct process* pcs = process_find_by_handle(hProcess);

    if (!pcs) return FALSE;
    if (!searchPath) return FALSE;

    HeapFree(GetProcessHeap(), 0, pcs->search_path);
    pcs->search_path = strcpy(HeapAlloc(GetProcessHeap(), 0, strlen(searchPath) + 1),
                              searchPath);
    return TRUE;
}

/***********************************************************************
 *		SymGetSearchPath (DBGHELP.@)
 */
BOOL WINAPI SymGetSearchPath(HANDLE hProcess, LPSTR szSearchPath, 
                             DWORD SearchPathLength)
{
    struct process* pcs = process_find_by_handle(hProcess);
    if (!pcs) return FALSE;

    strncpy(szSearchPath, pcs->search_path, SearchPathLength);
    szSearchPath[SearchPathLength - 1] = '\0';
    return TRUE;
}

/******************************************************************
 *		invade_process
 *
 * SymInitialize helper: loads in dbghelp all known (and loaded modules)
 * this assumes that hProcess is a handle on a valid process
 */
static BOOL process_invade(HANDLE hProcess)
{
    HMODULE     hMods[256];
    char        img[256];
    DWORD       i, sz;
    MODULEINFO  mi;

    if (!EnumProcessModules(hProcess, hMods, sizeof(hMods), &sz))
        return FALSE; /* FIXME should grow hMods */
    
    for (i = 0; i < sz / sizeof(HMODULE); i++)
    {
        if (!GetModuleInformation(hProcess, hMods[i], &mi, sizeof(mi)) ||
            !GetModuleFileNameExA(hProcess, hMods[i], img, sizeof(img)) ||
            !SymLoadModule(hProcess, 0, img, NULL, (DWORD)mi.lpBaseOfDll, mi.SizeOfImage))
            return FALSE;
    }

    return sz != 0;
}

/******************************************************************
 *		SymInitialize (DBGHELP.@)
 *
 * The initialisation of a dbghelp's context.
 * Note that hProcess doesn't need to be a valid process handle (except
 * when fInvadeProcess is TRUE).
 * Since, we're also allow to load ELF (pure) libraries and Wine ELF libraries 
 * containing PE (and NE) module(s), here's how we handle it:
 * - we load every module (ELF, NE, PE) passed in SymLoadModule
 * - in fInvadeProcess (in SymInitialize) is TRUE, we set up what is called ELF
 *   synchronization: hProcess should be a valid process handle, and we hook
 *   ourselves on hProcess's loaded ELF-modules, and keep this list in sync with
 *   our internal ELF modules representation (loading / unloading). This way,
 *   we'll pair every loaded builtin PE module with its ELF counterpart (and
 *   access its debug information).
 * - if fInvadeProcess (in SymInitialize) is FALSE, we won't be able to
 *   make the peering between a builtin PE module and its ELF counterpart, hence
 *   we won't be able to provide the requested debug information. We'll
 *   however be able to load native PE modules (and their debug information)
 *   without any trouble.
 * Note also that this scheme can be intertwined with the deferred loading 
 * mechanism (ie only load the debug information when we actually need it).
 */
BOOL WINAPI SymInitialize(HANDLE hProcess, PSTR UserSearchPath, BOOL fInvadeProcess)
{
    struct process*     pcs;

    TRACE("(%p %s %u)\n", hProcess, debugstr_a(UserSearchPath), fInvadeProcess);

    if (process_find_by_handle(hProcess))
        FIXME("what to do ??\n");

    pcs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pcs));
    if (!pcs) return FALSE;

    pcs->handle = hProcess;

    if (UserSearchPath)
    {
        pcs->search_path = strcpy(HeapAlloc(GetProcessHeap(), 0, strlen(UserSearchPath) + 1), 
                                  UserSearchPath);
    }
    else
    {
        unsigned        size;
        unsigned        len;

        pcs->search_path = HeapAlloc(GetProcessHeap(), 0, len = MAX_PATH);
        while ((size = GetCurrentDirectoryA(len, pcs->search_path)) >= len)
            pcs->search_path = HeapReAlloc(GetProcessHeap(), 0, pcs->search_path, len *= 2);
        pcs->search_path = HeapReAlloc(GetProcessHeap(), 0, pcs->search_path, size + 1);

        len = GetEnvironmentVariableA("_NT_SYMBOL_PATH", NULL, 0);
        if (len)
        {
            pcs->search_path = HeapReAlloc(GetProcessHeap(), 0, pcs->search_path, size + 1 + len + 1);
            pcs->search_path[size] = ';';
            GetEnvironmentVariableA("_NT_SYMBOL_PATH", pcs->search_path + size + 1, len);
            size += 1 + len;
        }
        len = GetEnvironmentVariableA("_NT_ALTERNATE_SYMBOL_PATH", NULL, 0);
        if (len)
        {
            pcs->search_path = HeapReAlloc(GetProcessHeap(), 0, pcs->search_path, size + 1 + len + 1);
            pcs->search_path[size] = ';';
            GetEnvironmentVariableA("_NT_ALTERNATE_SYMBOL_PATH", pcs->search_path + size + 1, len);
            size += 1 + len;
        }
    }

    pcs->lmodules = NULL;
    pcs->dbg_hdr_addr = 0;
    pcs->next = process_first;
    process_first = pcs;

    if (fInvadeProcess)
    {
#ifndef __REACTOS__
        if (!elf_read_wine_loader_dbg_info(pcs))
        {
            SymCleanup(hProcess);
            return FALSE;
        }
#endif
        process_invade(hProcess);
        elf_synchronize_module_list(pcs);
    }
    DbgPrint("SymInitialize - Success\n");
    return TRUE;
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
            next = (*ppcs)->next;
            HeapFree(GetProcessHeap(), 0, *ppcs);
            *ppcs = next;
            return TRUE;
        }
    }
    return FALSE;
}

/******************************************************************
 *		SymSetOptions (DBGHELP.@)
 *
 */
DWORD WINAPI SymSetOptions(DWORD opts)
{
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
 *		SymSetContext (DBGHELP.@)
 *
 */
BOOL WINAPI SymSetContext(HANDLE hProcess, PIMAGEHLP_STACK_FRAME StackFrame,
                          PIMAGEHLP_CONTEXT Context)
{
    struct process* pcs = process_find_by_handle(hProcess);
    if (!pcs) return FALSE;

    pcs->ctx_frame = *StackFrame;
    /* MSDN states that Context is not (no longer?) used */
    return TRUE;
}

/***********************************************************************
 *		SymRegisterCallback (DBGHELP.@)
 */
BOOL WINAPI SymRegisterCallback(HANDLE hProcess, 
                                PSYMBOL_REGISTERED_CALLBACK CallbackFunction,
                                PVOID UserContext)
{
    FIXME("(%p, %p, %p): stub\n", hProcess, CallbackFunction, UserContext);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
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
