/*
 * File module.c - module handling for the wine debugger
 *
 * Copyright (C) 1993,      Eric Youngdale.
 * 		 2000-2004, Eric Pouech
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "dbghelp_private.h"
#include "psapi.h"
#include "winreg.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

static void module_fill_module(const char* in, char* out, unsigned size)
{
    const char*         ptr;
    unsigned            len;

    for (ptr = in + strlen(in) - 1; 
         *ptr != '/' && *ptr != '\\' && ptr >= in; 
         ptr--);
    if (ptr < in || *ptr == '/' || *ptr == '\\') ptr++;
    strncpy(out, ptr, size);
    out[size - 1] = '\0';
    len = strlen(out);
    if (len > 4 && 
        (!strcasecmp(&out[len - 4], ".dll") || !strcasecmp(&out[len - 4], ".exe")))
        out[len - 4] = '\0';
    else
    {
        if (len > 7 && 
            (!strcasecmp(&out[len - 7], ".dll.so") || !strcasecmp(&out[len - 7], ".exe.so")))
            strcpy(&out[len - 7], "<elf>");
        else if (len > 7 &&
                 out[len - 7] == '.' && !strcasecmp(&out[len - 3], ".so"))
        {
            if (len + 3 < size) strcpy(&out[len - 3], "<elf>");
            else WARN("Buffer too short: %s\n", out);
        }
    }
    while ((*out = tolower(*out))) out++;
}

/***********************************************************************
 * Creates and links a new module to a process 
 */
struct module* module_new(struct process* pcs, const char* name, 
                          enum module_type type, 
                          unsigned long mod_addr, unsigned long size,
                          unsigned long stamp, unsigned long checksum) 
{
    struct module*      module;

    if (!(module = HeapAlloc(GetProcessHeap(), 0, sizeof(*module))))
	return NULL;

    memset(module, 0, sizeof(*module));

    module->next = pcs->lmodules;
    pcs->lmodules = module;

    TRACE("=> %s %08lx-%08lx %s\n", 
          type == DMT_ELF ? "ELF" : (type == DMT_PE ? "PE" : "---"),
          mod_addr, mod_addr + size, name);

    pool_init(&module->pool, 65536);
    
    module->module.SizeOfStruct = sizeof(module->module);
    module->module.BaseOfImage = mod_addr;
    module->module.ImageSize = size;
    module_fill_module(name, module->module.ModuleName, sizeof(module->module.ModuleName));
    module->module.ImageName[0] = '\0';
    strncpy(module->module.LoadedImageName, name, 
            sizeof(module->module.LoadedImageName));
    module->module.LoadedImageName[sizeof(module->module.LoadedImageName) - 1] = '\0';
    module->module.SymType = SymNone;
    module->module.NumSyms = 0;
    module->module.TimeDateStamp = stamp;
    module->module.CheckSum = checksum;

    module->type              = type;
    module->sortlist_valid    = FALSE;
    module->addr_sorttab      = NULL;
    /* FIXME: this seems a bit too high (on a per module basis)
     * need some statistics about this
     */
    hash_table_init(&module->pool, &module->ht_symbols, 4096);
    hash_table_init(&module->pool, &module->ht_types,   4096);
    vector_init(&module->vtypes, sizeof(struct symt*),  32);

    module->sources_used      = 0;
    module->sources_alloc     = 0;
    module->sources           = 0;

    return module;
}

/***********************************************************************
 *	module_find_by_name
 *
 */
struct module* module_find_by_name(const struct process* pcs, 
                                   const char* name, enum module_type type)
{
    struct module*      module;

    if (type == DMT_UNKNOWN)
    {
        if ((module = module_find_by_name(pcs, name, DMT_PE)) ||
            (module = module_find_by_name(pcs, name, DMT_ELF)))
            return module;
    }
    else
    {
        for (module = pcs->lmodules; module; module = module->next)
        {
            if (type == module->type && !strcasecmp(name, module->module.LoadedImageName)) 
                return module;
        }
        for (module = pcs->lmodules; module; module = module->next)
        {
            if (type == module->type && !strcasecmp(name, module->module.ModuleName)) 
                return module;
        }
    }
    SetLastError(ERROR_INVALID_NAME);
    return NULL;
}

/***********************************************************************
 *           module_get_container
 *
 */
struct module* module_get_container(const struct process* pcs, 
                                    const struct module* inner)
{
    struct module*      module;
     
    for (module = pcs->lmodules; module; module = module->next)
    {
        if (module != inner &&
            module->module.BaseOfImage <= inner->module.BaseOfImage &&
            module->module.BaseOfImage + module->module.ImageSize >=
            inner->module.BaseOfImage + inner->module.ImageSize)
            return module;
    }
    return NULL;
}

/***********************************************************************
 *           module_get_containee
 *
 */
struct module* module_get_containee(const struct process* pcs, 
                                    const struct module* outter)
{
    struct module*      module;
     
    for (module = pcs->lmodules; module; module = module->next)
    {
        if (module != outter &&
            outter->module.BaseOfImage <= module->module.BaseOfImage &&
            outter->module.BaseOfImage + outter->module.ImageSize >=
            module->module.BaseOfImage + module->module.ImageSize)
            return module;
    }
    return NULL;
}

/******************************************************************
 *		module_get_debug
 *
 * get the debug information from a module:
 * - if the module's type is deferred, then force loading of debug info (and return
 *   the module itself)
 * - if the module has no debug info and has an ELF container, then return the ELF
 *   container (and also force the ELF container's debug info loading if deferred)
 * - otherwise return the module itself if it has some debug info
 */
struct module* module_get_debug(const struct process* pcs, struct module* module)
{
    struct module*      parent;

    if (!module) return NULL;
    /* for a PE builtin, always get info from parent */
    if ((parent = module_get_container(pcs, module)))
        module = parent;
    /* if deferred, force loading */
    if (module->module.SymType == SymDeferred)
    {
        BOOL ret;

        switch (module->type)
        {
        case DMT_ELF: ret = elf_load_debug_info(module);     break;
        case DMT_PE:  ret = pe_load_debug_info(pcs, module); break;
        default:      ret = FALSE;                           break;
        }
        if (!ret) module->module.SymType = SymNone;
        assert(module->module.SymType != SymDeferred);
    }
    return (module && module->module.SymType != SymNone) ? module : NULL;
}

/***********************************************************************
 *	module_find_by_addr
 *
 * either the addr where module is loaded, or any address inside the 
 * module
 */
struct module* module_find_by_addr(const struct process* pcs, unsigned long addr, 
                                   enum module_type type)
{
    struct module*      module;
    
    if (type == DMT_UNKNOWN)
    {
        if ((module = module_find_by_addr(pcs, addr, DMT_PE)) ||
            (module = module_find_by_addr(pcs, addr, DMT_ELF)))
            return module;
    }
    else
    {
        for (module = pcs->lmodules; module; module = module->next)
        {
            if (type == module->type && addr >= module->module.BaseOfImage &&
                addr < module->module.BaseOfImage + module->module.ImageSize) 
                return module;
        }
    }
    SetLastError(ERROR_INVALID_ADDRESS);
    return module;
}

static BOOL module_is_elf_container_loaded(struct process* pcs, const char* ImageName, 
                                           const char* ModuleName)
{
    char                buffer[MAX_PATH];
    size_t              len;
    struct module*      module;

    if (!ModuleName)
    {
        module_fill_module(ImageName, buffer, sizeof(buffer));
        ModuleName = buffer;
    }
    len = strlen(ModuleName);
    for (module = pcs->lmodules; module; module = module->next)
    {
        if (!strncasecmp(module->module.ModuleName, ModuleName, len) &&
            module->type == DMT_ELF &&
            !strcmp(module->module.ModuleName + len, "<elf>"))
            return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *			SymLoadModule (DBGHELP.@)
 */
DWORD WINAPI SymLoadModule(HANDLE hProcess, HANDLE hFile, char* ImageName,
                           char* ModuleName, DWORD BaseOfDll, DWORD SizeOfDll)
{
    struct process*     pcs;
    struct module*	module = NULL;

    TRACE("(%p %p %s %s %08lx %08lx)\n", 
          hProcess, hFile, debugstr_a(ImageName), debugstr_a(ModuleName), 
          BaseOfDll, SizeOfDll);

    pcs = process_find_by_handle(hProcess);
    if (!pcs) return FALSE;

    /* force transparent ELF loading / unloading */
    elf_synchronize_module_list(pcs);

    /* this is a Wine extension to the API just to redo the synchronisation */
    if (!ImageName && !hFile) return 0;

    if (module_is_elf_container_loaded(pcs, ImageName, ModuleName))
    {
        /* force the loading of DLL as builtin */
        if ((module = pe_load_module_from_pcs(pcs, ImageName, ModuleName, BaseOfDll, SizeOfDll)))
            goto done;
        WARN("Couldn't locate %s\n", ImageName);
        return 0;
    }
    TRACE("Assuming %s as native DLL\n", ImageName);
    if (!(module = pe_load_module(pcs, ImageName, hFile, BaseOfDll, SizeOfDll)))
    {
        unsigned        len = strlen(ImageName);

        if (!strcmp(ImageName + len - 3, ".so") &&
            (module = elf_load_module(pcs, ImageName))) goto done;
        FIXME("should have successfully loaded some debug information for image %s\n", ImageName);
        if ((module = pe_load_module_from_pcs(pcs, ImageName, ModuleName, BaseOfDll, SizeOfDll)))
            goto done;
        WARN("Couldn't locate %s\n", ImageName);
        return 0;
    }

done:
    /* by default pe_load_module fills module.ModuleName from a derivation 
     * of ImageName. Overwrite it, if we have better information
     */
    if (ModuleName)
    {
        strncpy(module->module.ModuleName, ModuleName, 
                sizeof(module->module.ModuleName));
        module->module.ModuleName[sizeof(module->module.ModuleName) - 1] = '\0';
    }
    strncpy(module->module.ImageName, ImageName, sizeof(module->module.ImageName));
    module->module.ImageName[sizeof(module->module.ImageName) - 1] = '\0';

    return module->module.BaseOfImage;
}

/******************************************************************
 *		module_remove
 *
 */
BOOL module_remove(struct process* pcs, struct module* module)
{
    struct module**     p;

    TRACE("%s (%p)\n", module->module.ModuleName, module);
    hash_table_destroy(&module->ht_symbols);
    hash_table_destroy(&module->ht_types);
    HeapFree(GetProcessHeap(), 0, (char*)module->sources);
    HeapFree(GetProcessHeap(), 0, module->addr_sorttab);
    pool_destroy(&module->pool);

    for (p = &pcs->lmodules; *p; p = &(*p)->next)
    {
        if (*p == module)
        {
            *p = module->next;
            HeapFree(GetProcessHeap(), 0, module);
            return TRUE;
        }
    }
    FIXME("This shouldn't happen\n");
    return FALSE;
}

/******************************************************************
 *		SymUnloadModule (DBGHELP.@)
 *
 */
BOOL WINAPI SymUnloadModule(HANDLE hProcess, DWORD BaseOfDll)
{
    struct process*     pcs;
    struct module*      module;

    pcs = process_find_by_handle(hProcess);
    if (!pcs) return FALSE;
    module = module_find_by_addr(pcs, BaseOfDll, DMT_UNKNOWN);
    if (!module) return FALSE;
    return module_remove(pcs, module);
}

/******************************************************************
 *		SymEnumerateModules (DBGHELP.@)
 *
 */
BOOL  WINAPI SymEnumerateModules(HANDLE hProcess,
                                 PSYM_ENUMMODULES_CALLBACK EnumModulesCallback,  
                                 PVOID UserContext)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    struct module*      module;

    if (!pcs) return FALSE;
    
    for (module = pcs->lmodules; module; module = module->next)
    {
        if (!(dbghelp_options & SYMOPT_WINE_WITH_ELF_MODULES) && module->type != DMT_PE)
            continue;
        if (!EnumModulesCallback(module->module.ModuleName, 
                                 module->module.BaseOfImage, UserContext))
            break;
    }
    return TRUE;
}

/******************************************************************
 *		EnumerateLoadedModules (DBGHELP.@)
 *
 */
BOOL  WINAPI EnumerateLoadedModules(HANDLE hProcess,
                                    PENUMLOADED_MODULES_CALLBACK EnumLoadedModulesCallback,
                                    PVOID UserContext)
{
    HMODULE*    hMods;
    char        base[256], mod[256];
    DWORD       i, sz;
    MODULEINFO  mi;

    hMods = HeapAlloc(GetProcessHeap(), 0, sz);
    if (!hMods) return FALSE;

    if (!EnumProcessModules(hProcess, hMods, 256 * sizeof(hMods[0]), &sz))
    {
        /* hProcess should also be a valid process handle !! */
        FIXME("If this happens, bump the number in mod\n");
        HeapFree(GetProcessHeap(), 0, hMods);
        return FALSE;
    }
    sz /= sizeof(HMODULE);
    for (i = 0; i < sz; i++)
    {
        if (!GetModuleInformation(hProcess, hMods[i], &mi, sizeof(mi)) ||
            !GetModuleBaseNameA(hProcess, hMods[i], base, sizeof(base)))
            continue;
        module_fill_module(base, mod, sizeof(mod));

        EnumLoadedModulesCallback(mod, (DWORD)mi.lpBaseOfDll, mi.SizeOfImage, 
                                  UserContext);
    }
    HeapFree(GetProcessHeap(), 0, hMods);

    return sz != 0 && i == sz;
}

/******************************************************************
 *		SymGetModuleInfo (DBGHELP.@)
 *
 */
BOOL  WINAPI SymGetModuleInfo(HANDLE hProcess, DWORD dwAddr, 
                              PIMAGEHLP_MODULE ModuleInfo)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    struct module*      module;

    if (!pcs) return FALSE;
    if (ModuleInfo->SizeOfStruct < sizeof(*ModuleInfo)) return FALSE;
    module = module_find_by_addr(pcs, dwAddr, DMT_UNKNOWN);
    if (!module) return FALSE;

    *ModuleInfo = module->module;
    if (module->module.SymType == SymNone)
    {
        module = module_get_container(pcs, module);
        if (module && module->module.SymType != SymNone)
            ModuleInfo->SymType = module->module.SymType;
    }

    return TRUE;
}

/***********************************************************************
 *		SymGetModuleBase (IMAGEHLP.@)
 */
DWORD WINAPI SymGetModuleBase(HANDLE hProcess, DWORD dwAddr)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    struct module*      module;

    if (!pcs) return 0;
    module = module_find_by_addr(pcs, dwAddr, DMT_UNKNOWN);
    if (!module) return 0;
    return module->module.BaseOfImage;
}

/******************************************************************
 *		module_reset_debug_info
 * Removes any debug information linked to a given module.
 */
void module_reset_debug_info(struct module* module)
{
    module->sortlist_valid = TRUE;
    module->addr_sorttab = NULL;
    hash_table_destroy(&module->ht_symbols);
    module->ht_symbols.num_buckets = 0;
    module->ht_symbols.buckets = NULL;
    hash_table_destroy(&module->ht_types);
    module->ht_types.num_buckets = 0;
    module->ht_types.buckets = NULL;
    module->vtypes.num_elts = 0;
    hash_table_destroy(&module->ht_symbols);
    module->sources_used = module->sources_alloc = 0;
    module->sources = NULL;
}
