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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "dbghelp_private.h"
#ifndef DBGHELP_STATIC_LIB
#include "wine/debug.h"
#endif

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

static struct module*   rb_module;
struct source_rb
{
    struct wine_rb_entry        entry;
    unsigned                    source;
};

int source_rb_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct source_rb *t = WINE_RB_ENTRY_VALUE(entry, const struct source_rb, entry);

    return strcmp((const char*)key, rb_module->sources + t->source);
}

/******************************************************************
 *		source_find
 *
 * check whether a source file has already been stored
 */
static unsigned source_find(const char* name)
{
    struct wine_rb_entry*       e;

    e = wine_rb_get(&rb_module->sources_offsets_tree, name);
    if (!e) return -1;
    return WINE_RB_ENTRY_VALUE(e, struct source_rb, entry)->source;
}

/******************************************************************
 *		source_new
 *
 * checks if source exists. if not, add it
 */
unsigned source_new(struct module* module, const char* base, const char* name)
{
    unsigned    ret = -1;
    const char* full;
    char*       tmp = NULL;

    if (!name) return ret;
    if (!base || *name == '/')
        full = name;
    else
    {
        unsigned bsz = strlen(base);

        tmp = HeapAlloc(GetProcessHeap(), 0, bsz + 1 + strlen(name) + 1);
        if (!tmp) return ret;
        full = tmp;
        strcpy(tmp, base);
        if (tmp[bsz - 1] != '/') tmp[bsz++] = '/';
        strcpy(&tmp[bsz], name);
    }
    rb_module = module;
    if (!module->sources || (ret = source_find(full)) == (unsigned)-1)
    {
        char* new;
        int len = strlen(full) + 1;
        struct source_rb* rb;

        if (module->sources_used + len + 1 > module->sources_alloc)
        {
            if (!module->sources)
            {
                module->sources_alloc = (module->sources_used + len + 1 + 255) & ~255;
                new = HeapAlloc(GetProcessHeap(), 0, module->sources_alloc);
            }
            else
            {
                module->sources_alloc = max( module->sources_alloc * 2,
                                             (module->sources_used + len + 1 + 255) & ~255 );
                new = HeapReAlloc(GetProcessHeap(), 0, module->sources,
                                  module->sources_alloc);
            }
            if (!new) goto done;
            module->sources = new;
        }
        ret = module->sources_used;
        memcpy(module->sources + module->sources_used, full, len);
        module->sources_used += len;
        module->sources[module->sources_used] = '\0';
        if ((rb = pool_alloc(&module->pool, sizeof(*rb))))
        {
            rb->source = ret;
            wine_rb_put(&module->sources_offsets_tree, full, &rb->entry);
        }
    }
done:
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
 *		SymEnumSourceFilesW (DBGHELP.@)
 *
 */
BOOL WINAPI SymEnumSourceFilesW(HANDLE hProcess, ULONG64 ModBase, PCWSTR Mask,
                                PSYM_ENUMSOURCEFILES_CALLBACKW cbSrcFiles,
                                PVOID UserContext)
{
    struct module_pair  pair;
    SOURCEFILEW         sf;
    char*               ptr;
    WCHAR*              conversion_buffer = NULL;
    DWORD               conversion_buffer_len = 0;

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
            pair.requested = module_find_by_nameW(pair.pcs, Mask + 1);
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
        DWORD len = MultiByteToWideChar(CP_ACP, 0, ptr, -1, NULL, 0);

        if (len > conversion_buffer_len)
        {
            HeapFree(GetProcessHeap(), 0, conversion_buffer);
            conversion_buffer = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            if (!conversion_buffer) return FALSE;
            conversion_buffer_len = len;
        }

        MultiByteToWideChar(CP_ACP, 0, ptr, -1, conversion_buffer, len);

        /* FIXME: not using Mask */
        sf.ModBase = ModBase;
        sf.FileName = conversion_buffer;
        if (!cbSrcFiles(&sf, UserContext)) break;
    }

    HeapFree(GetProcessHeap(), 0, conversion_buffer);
    return TRUE;
}

struct enum_sources_files_context
{
    PSYM_ENUMSOURCEFILES_CALLBACK callbackA;
    PVOID caller_context;
    char *conversion_buffer;
    DWORD conversion_buffer_len;
    DWORD callback_error;
};

static BOOL CALLBACK enum_source_files_W_to_A(PSOURCEFILEW source_file, PVOID context)
{
    struct enum_sources_files_context *ctx = context;
    SOURCEFILE source_fileA;
    DWORD len;

    len = WideCharToMultiByte(CP_ACP, 0, source_file->FileName, -1, NULL, 0, NULL, NULL);
    if (len > ctx->conversion_buffer_len)
    {
        char *ptr = ctx->conversion_buffer ? HeapReAlloc(GetProcessHeap(), 0, ctx->conversion_buffer, len) :
                                             HeapAlloc(GetProcessHeap(), 0, len);

        if (!ptr)
        {
            ctx->callback_error = ERROR_OUTOFMEMORY;
            return FALSE;
        }

        ctx->conversion_buffer = ptr;
        ctx->conversion_buffer_len = len;
    }

    WideCharToMultiByte(CP_ACP, 0, source_file->FileName, -1, ctx->conversion_buffer, len, NULL, NULL);

    source_fileA.ModBase = source_file->ModBase;
    source_fileA.FileName = ctx->conversion_buffer;
    return ctx->callbackA(&source_fileA, ctx->caller_context);
}

/******************************************************************
 *		SymEnumSourceFiles (DBGHELP.@)
 *
 */
BOOL WINAPI SymEnumSourceFiles(HANDLE hProcess, ULONG64 ModBase, PCSTR Mask,
                               PSYM_ENUMSOURCEFILES_CALLBACK cbSrcFiles,
                               PVOID UserContext)
{
    WCHAR *maskW = NULL;
    PSYM_ENUMSOURCEFILES_CALLBACKW callbackW;
    PVOID context;
    struct enum_sources_files_context callback_context = {cbSrcFiles, UserContext};
    BOOL ret;

    if (Mask)
    {
        DWORD len = MultiByteToWideChar(CP_ACP, 0, Mask, -1, NULL, 0);

        maskW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!maskW)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        MultiByteToWideChar(CP_ACP, 0, Mask, -1, maskW, len);
    }

    if (cbSrcFiles)
    {
        callbackW = enum_source_files_W_to_A;
        context = &callback_context;
    }
    else
    {
        callbackW = NULL;
        context = UserContext;
    }

    ret = SymEnumSourceFilesW(hProcess, ModBase, maskW, callbackW, context);

    if (callback_context.callback_error)
    {
        SetLastError(callback_context.callback_error);
        ret = FALSE;
    }

    HeapFree(GetProcessHeap(), 0, callback_context.conversion_buffer);
    HeapFree(GetProcessHeap(), 0, maskW);

    return ret;
}

/******************************************************************
 *              SymEnumSourceLines (DBGHELP.@)
 *
 */
BOOL WINAPI SymEnumSourceLines(HANDLE hProcess, ULONG64 base, PCSTR obj,
                               PCSTR file, DWORD line, DWORD flags,
                               PSYM_ENUMLINES_CALLBACK EnumLinesCallback,
                               PVOID UserContext)
{
    FIXME("%p %s %s %s %u %u %p %p: stub!\n",
          hProcess, wine_dbgstr_longlong(base), debugstr_a(obj), debugstr_a(file),
          line, flags, EnumLinesCallback, UserContext);
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

/******************************************************************
 *               SymEnumSourceLinesW(DBGHELP.@)
 *
 */
BOOL WINAPI SymEnumSourceLinesW(HANDLE hProcess, ULONG64 base, PCWSTR obj,
                                PCWSTR file, DWORD line, DWORD flags,
                                PSYM_ENUMLINES_CALLBACKW EnumLinesCallback,
                                PVOID UserContext)
{
    FIXME("%p %s %s %s %u %u %p %p: stub!\n",
          hProcess, wine_dbgstr_longlong(base), debugstr_w(obj), debugstr_w(file),
          line, flags, EnumLinesCallback, UserContext);
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
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
