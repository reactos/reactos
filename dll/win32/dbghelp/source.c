/*
 * File source.c - source files management
 *
 * Copyright (C) 2004,      Eric Pouech.
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
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "dbghelp_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

/******************************************************************
 *		source_find
 *
 * check whether a source file has already been stored
 */
static unsigned source_find(const struct module* module, const char* name)
{
    char*       ptr = module->sources;

    while (*ptr)
    {
        if (strcmp(ptr, name) == 0) return ptr - module->sources;
        ptr += strlen(ptr) + 1;
    }
    return (unsigned)-1;
}

/******************************************************************
 *		source_new
 *
 * checks if source exists. if not, add it
 */
unsigned source_new(struct module* module, const char* base, const char* name)
{
    unsigned    ret;
    const char* full;
    char*       tmp = NULL;

    if (!name) return (unsigned)-1;
    if (!base || *name == '/')
        full = name;
    else
    {
        unsigned bsz = strlen(base);

        tmp = HeapAlloc(GetProcessHeap(), 0, bsz + 1 + strlen(name) + 1);
        if (!tmp) return (unsigned)-1;
        full = tmp;
        strcpy(tmp, base);
        if (tmp[bsz - 1] != '/') tmp[bsz++] = '/';
        strcpy(&tmp[bsz], name);
    }
    if (!module->sources || (ret = source_find(module, full)) == (unsigned)-1)
    {
        int len = strlen(full) + 1;
        if (module->sources_used + len + 1 > module->sources_alloc)
        {
            if (!module->sources)
            {
                module->sources_alloc = (module->sources_used + len + 1 + 255) & ~255;
                module->sources = HeapAlloc(GetProcessHeap(), 0, module->sources_alloc);
            }
            else
            {
                module->sources_alloc = max( module->sources_alloc * 2,
                                             (module->sources_used + len + 1 + 255) & ~255 );
                module->sources = HeapReAlloc(GetProcessHeap(), 0, module->sources,
                                              module->sources_alloc);
            }
        }
        ret = module->sources_used;
        memcpy(module->sources + module->sources_used, full, len);
        module->sources_used += len;
        module->sources[module->sources_used] = '\0';
    }
    HeapFree(GetProcessHeap(), 0, tmp);
    return ret;
}

/******************************************************************
 *		source_get
 *
 * returns a stored source file name
 */
const char* source_get(const struct module* module, unsigned idx)
{
    if (idx == -1) return "";
    assert(module->sources);
    return module->sources + idx;
}

/******************************************************************
 *		SymEnumSourceFiles (DBGHELP.@)
 *
 */
BOOL WINAPI SymEnumSourceFiles(HANDLE hProcess, ULONG64 ModBase, PCSTR Mask,
                               PSYM_ENUMSOURCEFILES_CALLBACK cbSrcFiles,
                               PVOID UserContext)
{
    struct module_pair  pair;
    SOURCEFILE          sf;
    char*               ptr;
    
    if (!cbSrcFiles) return FALSE;
    pair.pcs = process_find_by_handle(hProcess);
    if (!pair.pcs) return FALSE;
         
    if (ModBase)
    {
        pair.requested = module_find_by_addr(pair.pcs, ModBase, DMT_UNKNOWN);
        if (!module_get_debug(&pair)) return FALSE;
    }
    else
    {
        if (Mask[0] == '!')
        {
            pair.requested = module_find_by_nameA(pair.pcs, Mask + 1);
            if (!module_get_debug(&pair)) return FALSE;
        }
        else
        {
            FIXME("Unsupported yet (should get info from current context)\n");
            return FALSE;
        }
    }
    if (!pair.effective->sources) return FALSE;
    for (ptr = pair.effective->sources; *ptr; ptr += strlen(ptr) + 1)
    {
        /* FIXME: not using Mask */
        sf.ModBase = ModBase;
        sf.FileName = ptr;
        if (!cbSrcFiles(&sf, UserContext)) break;
    }

    return TRUE;
}

/******************************************************************
 *		SymGetSourceFileToken (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetSourceFileToken(HANDLE hProcess, ULONG64 base,
                                  PCSTR src, PVOID* token, DWORD* size)
{
    FIXME("%p %s %s %p %p: stub!\n",
          hProcess, wine_dbgstr_longlong(base), debugstr_a(src), token, size);
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

/******************************************************************
 *		SymGetSourceFileTokenW (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetSourceFileTokenW(HANDLE hProcess, ULONG64 base,
                                   PCWSTR src, PVOID* token, DWORD* size)
{
    FIXME("%p %s %s %p %p: stub!\n",
          hProcess, wine_dbgstr_longlong(base), debugstr_w(src), token, size);
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}
