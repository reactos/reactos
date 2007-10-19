/*
 * File symbol.c - management of symbols (lexical tree)
 *
 * Copyright (C) 1993, Eric Youngdale.
 *               2004, Eric Pouech
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <assert.h>
#define HAVE_REGEX_H 1
#ifdef HAVE_REGEX_H
# include "regex.h"
#endif

#include "wine/debug.h"
#include "dbghelp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);
WINE_DECLARE_DEBUG_CHANNEL(dbghelp_symt);

struct line_info
{
    unsigned long               is_first : 1,
                                is_last : 1,
                                is_source_file : 1,
                                line_number;
    union
    {
        unsigned long               pc_offset;   /* if is_source_file isn't set */
        unsigned                    source_file; /* if is_source_file is set */
    } u;
};

__inline static int cmp_addr(ULONG64 a1, ULONG64 a2)
{
    if (a1 > a2) return 1;
    if (a1 < a2) return -1;
    return 0;
}

__inline static int cmp_sorttab_addr(const struct module* module, int idx, ULONG64 addr)
{
    ULONG64     ref;

    symt_get_info(&module->addr_sorttab[idx]->symt, TI_GET_ADDRESS, &ref);
    return cmp_addr(ref, addr);
}

int symt_cmp_addr(const void* p1, const void* p2)
{
    const struct symt*  sym1 = *(const struct symt* const *)p1;
    const struct symt*  sym2 = *(const struct symt* const *)p2;
    ULONG64     a1, a2;

    symt_get_info(sym1, TI_GET_ADDRESS, &a1);
    symt_get_info(sym2, TI_GET_ADDRESS, &a2);
    return cmp_addr(a1, a2);
}

static __inline void re_append(char** mask, unsigned* len, char ch)
{
    *mask = HeapReAlloc(GetProcessHeap(), 0, *mask, ++(*len));
    (*mask)[*len - 2] = ch;
}

/* transforms a dbghelp's regular expression into a POSIX one
 * Here are the valid dbghelp reg ex characters:
 *      *       0 or more characters
 *      ?       a single character
 *      []      list
 *      #       0 or more of preceding char
 *      +       1 or more of preceding char
 *      escapes \ on #, ?, [, ], *, +. don't work on -
 */
static void compile_regex(const char* str, int numchar, regex_t* re)
{
    char*       mask = HeapAlloc(GetProcessHeap(), 0, 1);
    unsigned    len = 1;
    BOOL        in_escape = FALSE;

    re_append(&mask, &len, '^');

    while (*str && numchar--)
    {
        /* FIXME: this shouldn't be valid on '-' */
        if (in_escape)
        {
            re_append(&mask, &len, '\\');
            re_append(&mask, &len, *str);
            in_escape = FALSE;
        }
        else switch (*str)
        {
        case '\\': in_escape = TRUE; break;
        case '*':  re_append(&mask, &len, '.'); re_append(&mask, &len, '*'); break;
        case '?':  re_append(&mask, &len, '.'); break;
        case '#':  re_append(&mask, &len, '*'); break;
        /* escape some valid characters in dbghelp reg exp:s */
        case '$':  re_append(&mask, &len, '\\'); re_append(&mask, &len, '$'); break;
        /* +, [, ], - are the same in dbghelp & POSIX, use them as any other char */
        default:   re_append(&mask, &len, *str); break;
        }
        str++;
    }
    if (in_escape)
    {
        re_append(&mask, &len, '\\');
        re_append(&mask, &len, '\\');
    }
    re_append(&mask, &len, '$');
    mask[len - 1] = '\0';
    if (regcomp(re, mask, REG_NOSUB)) FIXME("Couldn't compile %s\n", mask);
    HeapFree(GetProcessHeap(), 0, mask);
}

struct symt_compiland* symt_new_compiland(struct module* module, const char* name)
{
    struct symt_compiland*    sym;

    TRACE_(dbghelp_symt)("Adding compiland symbol %s:%s\n",
                         module->module.ModuleName, name);
    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag = SymTagCompiland;
        sym->source   = source_new(module, name);
        vector_init(&sym->vchildren, sizeof(struct symt*), 32);
    }
    return sym;
}

struct symt_public* symt_new_public(struct module* module,
                                    struct symt_compiland* compiland,
                                    const char* name,
                                    unsigned long address, unsigned size,
                                    BOOL in_code, BOOL is_func)
{
    struct symt_public* sym;
    struct symt**       p;

    TRACE_(dbghelp_symt)("Adding public symbol %s:%s @%lx\n",
                         module->module.ModuleName, name, address);
    if ((dbghelp_options & SYMOPT_AUTO_PUBLICS) &&
        symt_find_nearest(module, address) != -1)
        return NULL;
    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag      = SymTagPublicSymbol;
        sym->hash_elt.name = pool_strdup(&module->pool, name);
        hash_table_add(&module->ht_symbols, &sym->hash_elt);
        module->sortlist_valid = FALSE;
        sym->container     = compiland ? &compiland->symt : NULL;
        sym->address       = address;
        sym->size          = size;
        sym->in_code       = in_code;
        sym->is_function   = is_func;
        if (compiland)
        {
            p = vector_add(&compiland->vchildren, &module->pool);
            *p = &sym->symt;
        }
    }
    return sym;
}

struct symt_data* symt_new_global_variable(struct module* module,
                                           struct symt_compiland* compiland,
                                           const char* name, unsigned is_static,
                                           unsigned long addr, unsigned long size,
                                           struct symt* type)
{
    struct symt_data*   sym;
    struct symt**       p;
    DWORD               tsz;

    TRACE_(dbghelp_symt)("Adding global symbol %s:%s @%lx %p\n",
                         module->module.ModuleName, name, addr, type);
    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag      = SymTagData;
        sym->hash_elt.name = pool_strdup(&module->pool, name);
        hash_table_add(&module->ht_symbols, &sym->hash_elt);
        module->sortlist_valid = FALSE;
        sym->kind          = is_static ? DataIsFileStatic : DataIsGlobal;
        sym->container     = compiland ? &compiland->symt : NULL;
        sym->type          = type;
        sym->u.address     = addr;
        if (type && size && symt_get_info(type, TI_GET_LENGTH, &tsz))
        {
            if (tsz != size)
                FIXME("Size mismatch for %s.%s between type (%lu) and src (%lu)\n",
                      module->module.ModuleName, name, tsz, size);
        }
        if (compiland)
        {
            p = vector_add(&compiland->vchildren, &module->pool);
            *p = &sym->symt;
        }
    }
    return sym;
}

struct symt_function* symt_new_function(struct module* module,
                                        struct symt_compiland* compiland,
                                        const char* name,
                                        unsigned long addr, unsigned long size,
                                        struct symt* sig_type)
{
    struct symt_function*       sym;
    struct symt**               p;

    TRACE_(dbghelp_symt)("Adding global function %s:%s @%lx-%lx\n",
                         module->module.ModuleName, name, addr, addr + size - 1);

    assert(!sig_type || sig_type->tag == SymTagFunctionType);
    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag  = SymTagFunction;
        sym->hash_elt.name = pool_strdup(&module->pool, name);
        hash_table_add(&module->ht_symbols, &sym->hash_elt);
        module->sortlist_valid = FALSE;
        sym->container = &compiland->symt;
        sym->address   = addr;
        sym->type      = sig_type;
        sym->size      = size;
        vector_init(&sym->vlines,  sizeof(struct line_info), 64);
        vector_init(&sym->vchildren, sizeof(struct symt*), 8);
        if (compiland)
        {
            p = vector_add(&compiland->vchildren, &module->pool);
            *p = &sym->symt;
        }
    }
    return sym;
}

void symt_add_func_line(struct module* module, struct symt_function* func,
                        unsigned source_idx, int line_num, unsigned long offset)
{
    struct line_info*   dli;
    BOOL                last_matches = FALSE;

    if (func == NULL || !(dbghelp_options & SYMOPT_LOAD_LINES)) return;

    TRACE_(dbghelp_symt)("(%p)%s:%lx %s:%u\n",
                         func, func->hash_elt.name, offset,
                         source_get(module, source_idx), line_num);

    assert(func->symt.tag == SymTagFunction);

    dli = NULL;
    while ((dli = vector_iter_down(&func->vlines, dli)))
    {
        if (dli->is_source_file)
        {
            last_matches = (source_idx == dli->u.source_file);
            break;
        }
    }

    if (!last_matches)
    {
        /* we shouldn't have line changes on first line of function */
        dli = vector_add(&func->vlines, &module->pool);
        dli->is_source_file = 1;
        dli->is_first       = dli->is_last = 0;
        dli->line_number    = 0;
        dli->u.source_file  = source_idx;
    }
    dli = vector_add(&func->vlines, &module->pool);
    dli->is_source_file = 0;
    dli->is_first       = dli->is_last = 0;
    dli->line_number    = line_num;
    dli->u.pc_offset    = func->address + offset;
}

struct symt_data* symt_add_func_local(struct module* module,
                                      struct symt_function* func,
                                      int regno, int offset,
                                      struct symt_block* block,
                                      struct symt* type, const char* name)
{
    struct symt_data*   locsym;
    struct symt**       p;

    assert(func);
    assert(func->symt.tag == SymTagFunction);

    TRACE_(dbghelp_symt)("Adding local symbol (%s:%s): %s %p\n",
                         module->module.ModuleName, func->hash_elt.name,
                         name, type);
    locsym = pool_alloc(&module->pool, sizeof(*locsym));
    locsym->symt.tag      = SymTagData;
    locsym->hash_elt.name = pool_strdup(&module->pool, name);
    locsym->hash_elt.next = NULL;
    locsym->kind          = (offset < 0) ? DataIsParam : DataIsLocal;
    locsym->container     = &block->symt;
    locsym->type          = type;
    if (regno)
    {
        locsym->u.s.reg_id = regno;
        locsym->u.s.offset = 0;
        locsym->u.s.length = 0;
    }
    else
    {
        locsym->u.s.reg_id = 0;
        locsym->u.s.offset = offset * 8;
        locsym->u.s.length = 0;
    }
    if (block)
        p = vector_add(&block->vchildren, &module->pool);
    else
        p = vector_add(&func->vchildren, &module->pool);
    *p = &locsym->symt;
    return locsym;
}

struct symt_block* symt_open_func_block(struct module* module,
                                        struct symt_function* func,
                                        struct symt_block* parent_block,
                                        unsigned pc, unsigned len)
{
    struct symt_block*  block;
    struct symt**       p;

    assert(func);
    assert(func->symt.tag == SymTagFunction);

    assert(!parent_block || parent_block->symt.tag == SymTagBlock);
    block = pool_alloc(&module->pool, sizeof(*block));
    block->symt.tag = SymTagBlock;
    block->address  = func->address + pc;
    block->size     = len;
    block->container = parent_block ? &parent_block->symt : &func->symt;
    vector_init(&block->vchildren, sizeof(struct symt*), 4);
    if (parent_block)
        p = vector_add(&parent_block->vchildren, &module->pool);
    else
        p = vector_add(&func->vchildren, &module->pool);
    *p = &block->symt;

    return block;
}

struct symt_block* symt_close_func_block(struct module* module,
                                         struct symt_function* func,
                                         struct symt_block* block, unsigned pc)
{
    assert(func->symt.tag == SymTagFunction);

    if (pc) block->size = func->address + pc - block->address;
    return (block->container->tag == SymTagBlock) ?
        GET_ENTRY(block->container, struct symt_block, symt) : NULL;
}

struct symt_function_point* symt_add_function_point(struct module* module,
                                                    struct symt_function* func,
                                                    enum SymTagEnum point,
                                                    unsigned offset, const char* name)
{
    struct symt_function_point* sym;
    struct symt**               p;

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag = point;
        sym->parent   = func;
        sym->offset   = offset;
        sym->name     = name ? pool_strdup(&module->pool, name) : NULL;
        p = vector_add(&func->vchildren, &module->pool);
        *p = &sym->symt;
    }
    return sym;
}

BOOL symt_normalize_function(struct module* module, struct symt_function* func)
{
    unsigned            len;
    struct line_info*   dli;

    assert(func);
    /* We aren't adding any more locals or line numbers to this function.
     * Free any spare memory that we might have allocated.
     */
    assert(func->symt.tag == SymTagFunction);

/* EPP     vector_pool_normalize(&func->vlines,    &module->pool); */
/* EPP     vector_pool_normalize(&func->vchildren, &module->pool); */

    len = vector_length(&func->vlines);
    if (len--)
    {
        dli = vector_at(&func->vlines,   0);  dli->is_first = 1;
        dli = vector_at(&func->vlines, len);  dli->is_last  = 1;
    }
    return TRUE;
}

struct symt_thunk* symt_new_thunk(struct module* module,
                                  struct symt_compiland* compiland,
                                  const char* name, THUNK_ORDINAL ord,
                                  unsigned long addr, unsigned long size)
{
    struct symt_thunk*  sym;

    TRACE_(dbghelp_symt)("Adding global thunk %s:%s @%lx-%lx\n",
                         module->module.ModuleName, name, addr, addr + size - 1);

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag  = SymTagThunk;
        sym->hash_elt.name = pool_strdup(&module->pool, name);
        hash_table_add(&module->ht_symbols, &sym->hash_elt);
        module->sortlist_valid = FALSE;
        sym->container = &compiland->symt;
        sym->address   = addr;
        sym->size      = size;
        sym->ordinal   = ord;
        if (compiland)
        {
            struct symt**       p;
            p = vector_add(&compiland->vchildren, &module->pool);
            *p = &sym->symt;
        }
    }
    return sym;
}

/* expect sym_info->MaxNameLen to be set before being called */
static void symt_fill_sym_info(const struct module* module,
                               const struct symt* sym, SYMBOL_INFO* sym_info)
{
    const char* name;

    sym_info->TypeIndex = (DWORD)sym;
    sym_info->info = 0; /* TBD */
    symt_get_info(sym, TI_GET_LENGTH, &sym_info->Size);
    sym_info->ModBase = module->module.BaseOfImage;
    sym_info->Flags = 0;
    switch (sym->tag)
    {
    case SymTagData:
        {
            const struct symt_data*  data = (const struct symt_data*)sym;
            switch (data->kind)
            {
            case DataIsLocal:
            case DataIsParam:
                if (data->u.s.reg_id)
                {
                    sym_info->Flags |= SYMFLAG_LOCAL | SYMFLAG_REGISTER;
                    sym_info->Register = data->u.s.reg_id;
                    sym_info->Address = 0;
                }
                else
                {
                    if (data->u.s.offset < 0)
                        sym_info->Flags |= SYMFLAG_LOCAL | SYMFLAG_FRAMEREL;
                    else
                        sym_info->Flags |= SYMFLAG_PARAMETER | SYMFLAG_FRAMEREL;
                    /* FIXME: needed ? moreover, it's i386 dependent !!! */
                    sym_info->Register = CV_REG_EBP;
                    sym_info->Address = data->u.s.offset;
                }
                break;
            case DataIsGlobal:
            case DataIsFileStatic:
                symt_get_info(sym, TI_GET_ADDRESS, &sym_info->Address);
                sym_info->Register = 0;
                break;
            case DataIsConstant:
                sym_info->Flags |= SYMFLAG_VALUEPRESENT;
                switch (data->u.value.n1.n2.vt)
                {
                case VT_I4:  sym_info->Value = (ULONG)data->u.value.n1.n2.n3.lVal; break;
                case VT_I2:  sym_info->Value = (ULONG)(long)data->u.value.n1.n2.n3.iVal; break;
                case VT_I1:  sym_info->Value = (ULONG)(long)data->u.value.n1.n2.n3.cVal; break;
                case VT_UI4: sym_info->Value = (ULONG)data->u.value.n1.n2.n3.ulVal; break;
                case VT_UI2: sym_info->Value = (ULONG)data->u.value.n1.n2.n3.uiVal; break;
                case VT_UI1: sym_info->Value = (ULONG)data->u.value.n1.n2.n3.bVal; break;
                default:
                    FIXME("Unsupported variant type (%u)\n", data->u.value.n1.n2.vt);
                }
                break;
            default:
                FIXME("Unhandled kind (%u) in sym data\n", data->kind);
            }
        }
        break;
    case SymTagPublicSymbol:
        sym_info->Flags |= SYMFLAG_EXPORT;
        symt_get_info(sym, TI_GET_ADDRESS, &sym_info->Address);
        break;
    case SymTagFunction:
        sym_info->Flags |= SYMFLAG_FUNCTION;
        symt_get_info(sym, TI_GET_ADDRESS, &sym_info->Address);
        break;
    case SymTagThunk:
        sym_info->Flags |= SYMFLAG_THUNK;
        symt_get_info(sym, TI_GET_ADDRESS, &sym_info->Address);
        break;
    default:
        symt_get_info(sym, TI_GET_ADDRESS, &sym_info->Address);
        sym_info->Register = 0;
        break;
    }
    sym_info->Scope = 0; /* FIXME */
    sym_info->Tag = sym->tag;
    name = symt_get_name(sym);
    if (sym_info->MaxNameLen)
    {
        if (sym->tag != SymTagPublicSymbol || !(dbghelp_options & SYMOPT_UNDNAME) ||
            (sym_info->NameLen = UnDecorateSymbolName(sym_info->Name, sym_info->Name,
                                                      sym_info->MaxNameLen, UNDNAME_COMPLETE) == 0))
        {
            sym_info->NameLen = min(strlen(name), sym_info->MaxNameLen - 1);
            strncpy(sym_info->Name, name, sym_info->NameLen);
            sym_info->Name[sym_info->NameLen] = '\0';
        }
    }
    TRACE_(dbghelp_symt)("%p => %s %lu %s\n",
                         sym, sym_info->Name, sym_info->Size,
                         wine_dbgstr_longlong(sym_info->Address));
}

static BOOL symt_enum_module(struct module* module, regex_t* regex,
                             PSYM_ENUMERATESYMBOLS_CALLBACK cb, PVOID user)
{
    char                        buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*                sym_info = (SYMBOL_INFO*)buffer;
    void*                       ptr;
    struct symt_ht*             sym = NULL;
    struct hash_table_iter      hti;

    hash_table_iter_init(&module->ht_symbols, &hti, NULL);
    while ((ptr = hash_table_iter_up(&hti)))
    {
        sym = GET_ENTRY(ptr, struct symt_ht, hash_elt);
        if (sym->hash_elt.name &&
            regexec(regex, sym->hash_elt.name, 0, NULL, 0) == 0)
        {
            sym_info->SizeOfStruct = sizeof(SYMBOL_INFO);
            sym_info->MaxNameLen = sizeof(buffer) - sizeof(SYMBOL_INFO);
            symt_fill_sym_info(module, &sym->symt, sym_info);
            if (!cb(sym_info, sym_info->Size, user)) return TRUE;
        }
    }
    return FALSE;
}

/***********************************************************************
 *              resort_symbols
 *
 * Rebuild sorted list of symbols for a module.
 */
static BOOL resort_symbols(struct module* module)
{
    int		                nsym = 0;
    void*                       ptr;
    struct symt_ht*             sym;
    struct hash_table_iter      hti;

    hash_table_iter_init(&module->ht_symbols, &hti, NULL);
    while ((ptr = hash_table_iter_up(&hti)))
        nsym++;

    if (!(module->module.NumSyms = nsym)) return FALSE;

    if (module->addr_sorttab)
        module->addr_sorttab = HeapReAlloc(GetProcessHeap(), 0,
                                           module->addr_sorttab,
                                           nsym * sizeof(struct symt_ht*));
    else
        module->addr_sorttab = HeapAlloc(GetProcessHeap(), 0,
                                         nsym * sizeof(struct symt_ht*));
    if (!module->addr_sorttab) return FALSE;

    nsym = 0;
    hash_table_iter_init(&module->ht_symbols, &hti, NULL);
    while ((ptr = hash_table_iter_up(&hti)))
    {
        sym = GET_ENTRY(ptr, struct symt_ht, hash_elt);
        assert(sym);
        module->addr_sorttab[nsym++] = sym;
    }

    qsort(module->addr_sorttab, nsym, sizeof(struct symt_ht*), symt_cmp_addr);
    return module->sortlist_valid = TRUE;
}

/* assume addr is in module */
int symt_find_nearest(struct module* module, DWORD addr)
{
    int         mid, high, low;
    ULONG64     ref_addr;
    DWORD       ref_size;

    if (!module->sortlist_valid || !module->addr_sorttab)
    {
        if (!resort_symbols(module)) return -1;
    }

    /*
     * Binary search to find closest symbol.
     */
    low = 0;
    high = module->module.NumSyms;

    symt_get_info(&module->addr_sorttab[0]->symt, TI_GET_ADDRESS, &ref_addr);
    if (addr < ref_addr) return -1;
    if (high)
    {
        symt_get_info(&module->addr_sorttab[high - 1]->symt, TI_GET_ADDRESS, &ref_addr);
        if (!symt_get_info(&module->addr_sorttab[high - 1]->symt,  TI_GET_LENGTH, &ref_size) || !ref_size)
            ref_size = 0x1000; /* arbitrary value */
        if (addr >= ref_addr + ref_size) return -1;
    }

    while (high > low + 1)
    {
        mid = (high + low) / 2;
        if (cmp_sorttab_addr(module, mid, addr) < 0)
            low = mid;
        else
            high = mid;
    }
    if (low != high && high != module->module.NumSyms &&
        cmp_sorttab_addr(module, high, addr) <= 0)
        low = high;

    /* If found symbol is a public symbol, check if there are any other entries that
     * might also have the same address, but would get better information
     */
    if (module->addr_sorttab[low]->symt.tag == SymTagPublicSymbol)
    {
        symt_get_info(&module->addr_sorttab[low]->symt, TI_GET_ADDRESS, &ref_addr);
        if (low > 0 &&
            module->addr_sorttab[low - 1]->symt.tag != SymTagPublicSymbol &&
            !cmp_sorttab_addr(module, low - 1, ref_addr))
            low--;
        else if (low < module->module.NumSyms - 1 &&
                 module->addr_sorttab[low + 1]->symt.tag != SymTagPublicSymbol &&
                 !cmp_sorttab_addr(module, low + 1, ref_addr))
            low++;
    }
    /* finally check that we fit into the found symbol */
    symt_get_info(&module->addr_sorttab[low]->symt, TI_GET_ADDRESS, &ref_addr);
    if (addr < ref_addr) return -1;
    if (!symt_get_info(&module->addr_sorttab[high - 1]->symt, TI_GET_LENGTH, &ref_size) || !ref_size)
        ref_size = 0x1000; /* arbitrary value */
    if (addr >= ref_addr + ref_size) return -1;

    return low;
}

static BOOL symt_enum_locals_helper(struct process* pcs, struct module* module,
                                    regex_t* preg, PSYM_ENUMERATESYMBOLS_CALLBACK cb,
                                    PVOID user, SYMBOL_INFO* sym_info,
                                    struct vector* v)
{
    struct symt**       plsym = NULL;
    struct symt*        lsym = NULL;
    DWORD               pc = pcs->ctx_frame.InstructionOffset;

    while ((plsym = vector_iter_up(v, plsym)))
    {
        lsym = *plsym;
        switch (lsym->tag)
        {
        case SymTagBlock:
            {
                struct symt_block*  block = (struct symt_block*)lsym;
                if (pc < block->address || block->address + block->size <= pc)
                    continue;
                if (!symt_enum_locals_helper(pcs, module, preg, cb, user,
                                             sym_info, &block->vchildren))
                    return FALSE;
            }
            break;
        case SymTagData:
            if (regexec(preg, symt_get_name(lsym), 0, NULL, 0) == 0)
            {
                symt_fill_sym_info(module, lsym, sym_info);
                if (!cb(sym_info, sym_info->Size, user))
                    return FALSE;
            }
            break;
        case SymTagLabel:
        case SymTagFuncDebugStart:
        case SymTagFuncDebugEnd:
            break;
        default:
            FIXME("Unknown type: %u (%x)\n", lsym->tag, lsym->tag);
            assert(0);
        }
    }
    return TRUE;
}

static BOOL symt_enum_locals(struct process* pcs, const char* mask,
                             PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
                             PVOID UserContext)
{
    struct module*      module;
    struct symt_ht*     sym;
    char                buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*        sym_info = (SYMBOL_INFO*)buffer;
    DWORD               pc = pcs->ctx_frame.InstructionOffset;
    int                 idx;

    sym_info->SizeOfStruct = sizeof(*sym_info);
    sym_info->MaxNameLen = sizeof(buffer) - sizeof(SYMBOL_INFO);

    module = module_find_by_addr(pcs, pc, DMT_UNKNOWN);
    if (!(module = module_get_debug(pcs, module))) return FALSE;
    if ((idx = symt_find_nearest(module, pc)) == -1) return FALSE;

    sym = module->addr_sorttab[idx];
    if (sym->symt.tag == SymTagFunction)
    {
        BOOL            ret;
        regex_t         preg;

        compile_regex(mask ? mask : "*", -1, &preg);
        ret = symt_enum_locals_helper(pcs, module, &preg, EnumSymbolsCallback,
                                      UserContext, sym_info,
                                      &((struct symt_function*)sym)->vchildren);
        regfree(&preg);
        return ret;

    }
    symt_fill_sym_info(module, &sym->symt, sym_info);
    return EnumSymbolsCallback(sym_info, sym_info->Size, UserContext);
}

/******************************************************************
 *		SymEnumSymbols (DBGHELP.@)
 *
 * cases BaseOfDll = 0
 *      !foo fails always (despite what MSDN states)
 *      RE1!RE2 looks up all modules matching RE1, and in all these modules, lookup RE2
 *      no ! in Mask, lookup in local Context
 * cases BaseOfDll != 0
 *      !foo fails always (despite what MSDN states)
 *      RE1!RE2 gets RE2 from BaseOfDll (whatever RE1 is)
 */
BOOL WINAPI SymEnumSymbols(HANDLE hProcess, ULONG64 BaseOfDll, PCSTR Mask,
                           PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
                           PVOID UserContext)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    struct module*      module;
    struct module*      dbg_module;
    const char*         bang;
    regex_t             mod_regex, sym_regex;

    TRACE("(%p %s %s %p %p)\n",
          hProcess, wine_dbgstr_longlong(BaseOfDll), debugstr_a(Mask),
          EnumSymbolsCallback, UserContext);

    if (!pcs) return FALSE;

    if (BaseOfDll == 0)
    {
        /* do local variables ? */
        if (!Mask || !(bang = strchr(Mask, '!')))
            return symt_enum_locals(pcs, Mask, EnumSymbolsCallback, UserContext);

        if (bang == Mask) return FALSE;

        compile_regex(Mask, bang - Mask, &mod_regex);
        compile_regex(bang + 1, -1, &sym_regex);

        for (module = pcs->lmodules; module; module = module->next)
        {
            if (module->type == DMT_PE && (dbg_module = module_get_debug(pcs, module)))
            {
                if (regexec(&mod_regex, module->module.ModuleName, 0, NULL, 0) == 0 &&
                    symt_enum_module(dbg_module, &sym_regex,
                                     EnumSymbolsCallback, UserContext))
                    break;
            }
        }
        /* not found in PE modules, retry on the ELF ones
         */
        if (!module && (dbghelp_options & SYMOPT_WINE_WITH_ELF_MODULES))
        {
            for (module = pcs->lmodules; module; module = module->next)
            {
                if (module->type == DMT_ELF &&
                    !module_get_containee(pcs, module) &&
                    (dbg_module = module_get_debug(pcs, module)))
                {
                    if (regexec(&mod_regex, module->module.ModuleName, 0, NULL, 0) == 0 &&
                        symt_enum_module(dbg_module, &sym_regex, EnumSymbolsCallback, UserContext))
                    break;
                }
            }
        }
        regfree(&mod_regex);
        regfree(&sym_regex);
        return TRUE;
    }
    module = module_find_by_addr(pcs, BaseOfDll, DMT_UNKNOWN);
    if (!(module = module_get_debug(pcs, module)))
        return FALSE;

    /* we always ignore module name from Mask when BaseOfDll is defined */
    if (Mask && (bang = strchr(Mask, '!')))
    {
        if (bang == Mask) return FALSE;
        Mask = bang + 1;
    }

    compile_regex(Mask ? Mask : "*", -1, &sym_regex);
    symt_enum_module(module, &sym_regex, EnumSymbolsCallback, UserContext);
    regfree(&sym_regex);

    return TRUE;
}

struct sym_enumerate
{
    void*                       ctx;
    PSYM_ENUMSYMBOLS_CALLBACK   cb;
};

static BOOL CALLBACK sym_enumerate_cb(PSYMBOL_INFO syminfo, ULONG size, void* ctx)
{
    struct sym_enumerate*       se = (struct sym_enumerate*)ctx;
    return (se->cb)(syminfo->Name, syminfo->Address, syminfo->Size, se->ctx);
}

/***********************************************************************
 *		SymEnumerateSymbols (DBGHELP.@)
 */
BOOL WINAPI SymEnumerateSymbols(HANDLE hProcess, DWORD BaseOfDll,
                                PSYM_ENUMSYMBOLS_CALLBACK EnumSymbolsCallback,
                                PVOID UserContext)
{
    struct sym_enumerate        se;

    se.ctx = UserContext;
    se.cb  = EnumSymbolsCallback;

    return SymEnumSymbols(hProcess, BaseOfDll, NULL, sym_enumerate_cb, &se);
}

/******************************************************************
 *		SymFromAddr (DBGHELP.@)
 *
 */
BOOL WINAPI SymFromAddr(HANDLE hProcess, DWORD64 Address,
                        DWORD64* Displacement, PSYMBOL_INFO Symbol)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    struct module*      module;
    struct symt_ht*     sym;
    int                 idx;

    if (!pcs) return FALSE;
    module = module_find_by_addr(pcs, Address, DMT_UNKNOWN);
    if (!(module = module_get_debug(pcs, module))) return FALSE;
    if ((idx = symt_find_nearest(module, Address)) == -1) return FALSE;

    sym = module->addr_sorttab[idx];

    symt_fill_sym_info(module, &sym->symt, Symbol);
    if (Displacement) *Displacement = Address - Symbol->Address;
    return TRUE;
}

/******************************************************************
 *		SymGetSymFromAddr (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetSymFromAddr(HANDLE hProcess, DWORD Address,
                              PDWORD Displacement, PIMAGEHLP_SYMBOL Symbol)
{
    char        buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*si = (SYMBOL_INFO*)buffer;
    size_t      len;
    DWORD64     Displacement64;

    if (Symbol->SizeOfStruct < sizeof(*Symbol)) return FALSE;
    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen = 256;
    if (!SymFromAddr(hProcess, Address, &Displacement64, si))
        return FALSE;

    if (Displacement)
        *Displacement = Displacement64;
    Symbol->Address = si->Address;
    Symbol->Size    = si->Size;
    Symbol->Flags   = si->Flags;
    len = min(Symbol->MaxNameLength, si->MaxNameLen);
    strncpy(Symbol->Name, si->Name, len);
    Symbol->Name[len - 1] = '\0';
    return TRUE;
}

/******************************************************************
 *		SymFromName (DBGHELP.@)
 *
 */
BOOL WINAPI SymFromName(HANDLE hProcess, LPSTR Name, PSYMBOL_INFO Symbol)
{
    struct process*             pcs = process_find_by_handle(hProcess);
    struct module*              module;
    struct hash_table_iter      hti;
    void*                       ptr;
    struct symt_ht*             sym = NULL;
    const char*                 name;

    TRACE("(%p, %s, %p)\n", hProcess, Name, Symbol);
    if (!pcs) return FALSE;
    if (Symbol->SizeOfStruct < sizeof(*Symbol)) return FALSE;
    name = strchr(Name, '!');
    if (name)
    {
        char    tmp[128];
        assert(name - Name < sizeof(tmp));
        memcpy(tmp, Name, name - Name);
        tmp[name - Name] = '\0';
        module = module_find_by_name(pcs, tmp, DMT_UNKNOWN);
        if (!module) return FALSE;
        Name = (char*)(name + 1);
    }
    else module = pcs->lmodules;

    /* FIXME: Name could be made out of a regular expression */
    for (; module; module = (name) ? NULL : module->next)
    {
        if (module->module.SymType == SymNone) continue;
        if (module->module.SymType == SymDeferred)
        {
            struct module*      xmodule = module_get_debug(pcs, module);
            if (!xmodule || xmodule != module) continue;
        }
        hash_table_iter_init(&module->ht_symbols, &hti, Name);
        while ((ptr = hash_table_iter_up(&hti)))
        {
            sym = GET_ENTRY(ptr, struct symt_ht, hash_elt);

            if (!strcmp(sym->hash_elt.name, Name))
            {
                symt_fill_sym_info(module, &sym->symt, Symbol);
                return TRUE;
            }
        }
    }
    return FALSE;
}

/***********************************************************************
 *		SymGetSymFromName (DBGHELP.@)
 */
BOOL WINAPI SymGetSymFromName(HANDLE hProcess, LPSTR Name, PIMAGEHLP_SYMBOL Symbol)
{
    char        buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*si = (SYMBOL_INFO*)buffer;
    size_t      len;

    if (Symbol->SizeOfStruct < sizeof(*Symbol)) return FALSE;
    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen = 256;
    if (!SymFromName(hProcess, Name, si)) return FALSE;

    Symbol->Address = si->Address;
    Symbol->Size    = si->Size;
    Symbol->Flags   = si->Flags;
    len = min(Symbol->MaxNameLength, si->MaxNameLen);
    strncpy(Symbol->Name, si->Name, len);
    Symbol->Name[len - 1] = '\0';
    return TRUE;
}

/******************************************************************
 *		sym_fill_func_line_info
 *
 * fills information about a file
 */
BOOL symt_fill_func_line_info(struct module* module, struct symt_function* func,
                              DWORD addr, IMAGEHLP_LINE* line)
{
    struct line_info*   dli = NULL;
    BOOL                found = FALSE;

    assert(func->symt.tag == SymTagFunction);

    while ((dli = vector_iter_down(&func->vlines, dli)))
    {
        if (!dli->is_source_file)
        {
            if (found || dli->u.pc_offset > addr) continue;
            line->LineNumber = dli->line_number;
            line->Address    = dli->u.pc_offset;
            line->Key        = dli;
            found = TRUE;
            continue;
        }
        if (found)
        {
            line->FileName = (char*)source_get(module, dli->u.source_file);
            return TRUE;
        }
    }
    return FALSE;
}

/***********************************************************************
 *		SymGetSymNext (DBGHELP.@)
 */
BOOL WINAPI SymGetSymNext(HANDLE hProcess, PIMAGEHLP_SYMBOL Symbol)
{
    /* algo:
     * get module from Symbol.Address
     * get index in module.addr_sorttab of Symbol.Address
     * increment index
     * if out of module bounds, move to next module in process address space
     */
    FIXME("(%p, %p): stub\n", hProcess, Symbol);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		SymGetSymPrev (DBGHELP.@)
 */

BOOL WINAPI SymGetSymPrev(HANDLE hProcess, PIMAGEHLP_SYMBOL Symbol)
{
    FIXME("(%p, %p): stub\n", hProcess, Symbol);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************
 *		SymGetLineFromAddr (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLineFromAddr(HANDLE hProcess, DWORD dwAddr,
                               PDWORD pdwDisplacement, PIMAGEHLP_LINE Line)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    struct module*      module;
    int                 idx;

    TRACE("%p %08lx %p %p\n", hProcess, dwAddr, pdwDisplacement, Line);

    if (Line->SizeOfStruct < sizeof(*Line)) return FALSE;

    if (!pcs) return FALSE;
    module = module_find_by_addr(pcs, dwAddr, DMT_UNKNOWN);
    if (!(module = module_get_debug(pcs, module))) return FALSE;
    if ((idx = symt_find_nearest(module, dwAddr)) == -1) return FALSE;

    if (module->addr_sorttab[idx]->symt.tag != SymTagFunction) return FALSE;
    if (!symt_fill_func_line_info(module,
                                  (struct symt_function*)module->addr_sorttab[idx],
                                  dwAddr, Line)) return FALSE;
    if (pdwDisplacement) *pdwDisplacement = dwAddr - Line->Address;
    return TRUE;
}

/******************************************************************
 *		SymGetLinePrev (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLinePrev(HANDLE hProcess, PIMAGEHLP_LINE Line)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    struct module*      module;
    struct line_info*   li;
    BOOL                in_search = FALSE;

    TRACE("(%p %p)\n", hProcess, Line);

    if (Line->SizeOfStruct < sizeof(*Line)) return FALSE;

    if (!pcs) return FALSE;
    module = module_find_by_addr(pcs, Line->Address, DMT_UNKNOWN);
    if (!(module = module_get_debug(pcs, module))) return FALSE;

    if (Line->Key == 0) return FALSE;
    li = (struct line_info*)Line->Key;
    /* things are a bit complicated because when we encounter a DLIT_SOURCEFILE
     * element we have to go back until we find the prev one to get the real
     * source file name for the DLIT_OFFSET element just before
     * the first DLIT_SOURCEFILE
     */
    while (!li->is_first)
    {
        li--;
        if (!li->is_source_file)
        {
            Line->LineNumber = li->line_number;
            Line->Address    = li->u.pc_offset;
            Line->Key        = li;
            if (!in_search) return TRUE;
        }
        else
        {
            if (in_search)
            {
                Line->FileName = (char*)source_get(module, li->u.source_file);
                return TRUE;
            }
            in_search = TRUE;
        }
    }
    SetLastError(ERROR_NO_MORE_ITEMS); /* FIXME */
    return FALSE;
}

BOOL symt_get_func_line_next(struct module* module, PIMAGEHLP_LINE line)
{
    struct line_info*   li;

    if (line->Key == 0) return FALSE;
    li = (struct line_info*)line->Key;
    while (!li->is_last)
    {
        li++;
        if (!li->is_source_file)
        {
            line->LineNumber = li->line_number;
            line->Address    = li->u.pc_offset;
            line->Key        = li;
            return TRUE;
        }
        line->FileName = (char*)source_get(module, li->u.source_file);
    }
    return FALSE;
}

/******************************************************************
 *		SymGetLineNext (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLineNext(HANDLE hProcess, PIMAGEHLP_LINE Line)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    struct module*      module;

    TRACE("(%p %p)\n", hProcess, Line);

    if (Line->SizeOfStruct < sizeof(*Line)) return FALSE;
    if (!pcs) return FALSE;
    module = module_find_by_addr(pcs, Line->Address, DMT_UNKNOWN);
    if (!(module = module_get_debug(pcs, module))) return FALSE;

    if (symt_get_func_line_next(module, Line)) return TRUE;
    SetLastError(ERROR_NO_MORE_ITEMS); /* FIXME */
    return FALSE;
}

/***********************************************************************
 *		SymFunctionTableAccess (DBGHELP.@)
 */
PVOID WINAPI SymFunctionTableAccess(HANDLE hProcess, DWORD AddrBase)
{
    FIXME("(%p, 0x%08lx): stub\n", hProcess, AddrBase);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		SymUnDName (DBGHELP.@)
 */
BOOL WINAPI SymUnDName(PIMAGEHLP_SYMBOL sym, LPSTR UnDecName, DWORD UnDecNameLength)
{
    TRACE("(%p %s %lu): stub\n", sym, UnDecName, UnDecNameLength);
    return UnDecorateSymbolName(sym->Name, UnDecName, UnDecNameLength,
                                UNDNAME_COMPLETE) != 0;
}

static void* und_alloc(size_t len) { return HeapAlloc(GetProcessHeap(), 0, len); }
static void  und_free (void* ptr)  { HeapFree(GetProcessHeap(), 0, ptr); }

/***********************************************************************
 *		UnDecorateSymbolName (DBGHELP.@)
 */
DWORD WINAPI UnDecorateSymbolName(LPCSTR DecoratedName, LPSTR UnDecoratedName,
                                  DWORD UndecoratedLength, DWORD Flags)
{
    /* undocumented from msvcrt */
    static char* (*p_undname)(char*, const char*, int, void* (*)(size_t), void (*)(void*), unsigned short);
    static WCHAR szMsvcrt[] = {'m','s','v','c','r','t','.','d','l','l',0};

    TRACE("(%s, %p, %ld, 0x%08lx): stub\n",
          debugstr_a(DecoratedName), UnDecoratedName, UndecoratedLength, Flags);

    if (!p_undname)
    {
        if (!hMsvcrt) hMsvcrt = LoadLibraryW(szMsvcrt);
        if (hMsvcrt) p_undname = (void*)GetProcAddress(hMsvcrt, "__unDName");
        if (!p_undname) return 0;
    }

    if (!UnDecoratedName) return 0;
    if (!p_undname(UnDecoratedName, DecoratedName, UndecoratedLength,
                   und_alloc, und_free, Flags))
        return 0;
    return strlen(UnDecoratedName);
}
