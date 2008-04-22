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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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
#ifdef HAVE_REGEX_H
# include <regex.h>
#endif

#include "wine/debug.h"
#include "dbghelp_private.h"
#include "winnls.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);
WINE_DECLARE_DEBUG_CHANNEL(dbghelp_symt);

static inline int cmp_addr(ULONG64 a1, ULONG64 a2)
{
    if (a1 > a2) return 1;
    if (a1 < a2) return -1;
    return 0;
}

static inline int cmp_sorttab_addr(const struct module* module, int idx, ULONG64 addr)
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

static inline void re_append(char** mask, unsigned* len, char ch)
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
static void compile_regex(const char* str, int numchar, regex_t* re, BOOL _case)
{
    char*       mask = HeapAlloc(GetProcessHeap(), 0, 1);
    unsigned    len = 1;
    BOOL        in_escape = FALSE;
    unsigned    flags = REG_NOSUB;

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
    if (_case) flags |= REG_ICASE;
    if (regcomp(re, mask, flags)) FIXME("Couldn't compile %s\n", mask);
    HeapFree(GetProcessHeap(), 0, mask);
}

struct symt_compiland* symt_new_compiland(struct module* module, 
                                          unsigned long address, unsigned src_idx)
{
    struct symt_compiland*    sym;

    TRACE_(dbghelp_symt)("Adding compiland symbol %s:%s\n",
                         debugstr_w(module->module.ModuleName), source_get(module, src_idx));
    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag = SymTagCompiland;
        sym->address  = address;
        sym->source   = src_idx;
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
                         debugstr_w(module->module.ModuleName), name, address);
    if ((dbghelp_options & SYMOPT_AUTO_PUBLICS) &&
        symt_find_nearest(module, address) != NULL)
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
    DWORD64             tsz;

    TRACE_(dbghelp_symt)("Adding global symbol %s:%s @%lx %p\n",
                         debugstr_w(module->module.ModuleName), name, addr, type);
    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag      = SymTagData;
        sym->hash_elt.name = pool_strdup(&module->pool, name);
        hash_table_add(&module->ht_symbols, &sym->hash_elt);
        module->sortlist_valid = FALSE;
        sym->kind          = is_static ? DataIsFileStatic : DataIsGlobal;
        sym->container     = compiland ? &compiland->symt : NULL;
        sym->type          = type;
        sym->u.var.offset  = addr;
        if (type && size && symt_get_info(type, TI_GET_LENGTH, &tsz))
        {
            if (tsz != size)
                FIXME("Size mismatch for %s.%s between type (%s) and src (%lu)\n",
                      debugstr_w(module->module.ModuleName), name,
                      wine_dbgstr_longlong(tsz), size);
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
                         debugstr_w(module->module.ModuleName), name, addr, addr + size - 1);

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
    int                 i;

    if (func == NULL || !(dbghelp_options & SYMOPT_LOAD_LINES)) return;

    TRACE_(dbghelp_symt)("(%p)%s:%lx %s:%u\n", 
                         func, func->hash_elt.name, offset, 
                         source_get(module, source_idx), line_num);

    assert(func->symt.tag == SymTagFunction);

    for (i=vector_length(&func->vlines)-1; i>=0; i--)
    {
        dli = vector_at(&func->vlines, i);
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

/******************************************************************
 *             symt_add_func_local
 *
 * Adds a new local/parameter to a given function:
 * In any cases, dt tells whether it's a local variable or a parameter
 * If regno it's not 0:
 *      - then variable is stored in a register
 *      - otherwise, value is referenced by register + offset
 * Otherwise, the variable is stored on the stack:
 *      - offset is then the offset from the frame register
 */
struct symt_data* symt_add_func_local(struct module* module, 
                                      struct symt_function* func, 
                                      enum DataKind dt,
                                      const struct location* loc,
                                      struct symt_block* block, 
                                      struct symt* type, const char* name)
{
    struct symt_data*   locsym;
    struct symt**       p;

    TRACE_(dbghelp_symt)("Adding local symbol (%s:%s): %s %p\n",
                         debugstr_w(module->module.ModuleName), func->hash_elt.name,
                         name, type);

    assert(func);
    assert(func->symt.tag == SymTagFunction);
    assert(dt == DataIsParam || dt == DataIsLocal);

    locsym = pool_alloc(&module->pool, sizeof(*locsym));
    locsym->symt.tag      = SymTagData;
    locsym->hash_elt.name = pool_strdup(&module->pool, name);
    locsym->hash_elt.next = NULL;
    locsym->kind          = dt;
    locsym->container     = &block->symt;
    locsym->type          = type;
    locsym->u.var         = *loc;
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
    assert(func);
    assert(func->symt.tag == SymTagFunction);

    if (pc) block->size = func->address + pc - block->address;
    return (block->container->tag == SymTagBlock) ? 
        GET_ENTRY(block->container, struct symt_block, symt) : NULL;
}

struct symt_function_point* symt_add_function_point(struct module* module, 
                                                    struct symt_function* func,
                                                    enum SymTagEnum point, 
                                                    const struct location* loc,
                                                    const char* name)
{
    struct symt_function_point* sym;
    struct symt**               p;

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag = point;
        sym->parent   = func;
        sym->loc      = *loc;
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
                         debugstr_w(module->module.ModuleName), name, addr, addr + size - 1);

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

struct symt_data* symt_new_constant(struct module* module,
                                    struct symt_compiland* compiland,
                                    const char* name, struct symt* type,
                                    const VARIANT* v)
{
    struct symt_data*  sym;

    TRACE_(dbghelp_symt)("Adding constant value %s:%s\n",
                         debugstr_w(module->module.ModuleName), name);

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag      = SymTagData;
        sym->hash_elt.name = pool_strdup(&module->pool, name);
        hash_table_add(&module->ht_symbols, &sym->hash_elt);
        module->sortlist_valid = FALSE;
        sym->kind          = DataIsConstant;
        sym->container     = compiland ? &compiland->symt : NULL;
        sym->type          = type;
        sym->u.value       = *v;
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
static void symt_fill_sym_info(const struct module_pair* pair,
                               const struct symt_function* func,
                               const struct symt* sym, SYMBOL_INFO* sym_info)
{
    const char* name;
    DWORD64 size;

    if (!symt_get_info(sym, TI_GET_TYPE, &sym_info->TypeIndex))
        sym_info->TypeIndex = 0;
    sym_info->info = (DWORD)sym;
    sym_info->Reserved[0] = sym_info->Reserved[1] = 0;
    if (!symt_get_info(sym, TI_GET_LENGTH, &size) &&
        (!sym_info->TypeIndex ||
         !symt_get_info((struct symt*)sym_info->TypeIndex, TI_GET_LENGTH, &size)))
        size = 0;
    sym_info->Size = (DWORD)size;
    sym_info->ModBase = pair->requested->module.BaseOfImage;
    sym_info->Flags = 0;
    sym_info->Value = 0;

    switch (sym->tag)
    {
    case SymTagData:
        {
            const struct symt_data*  data = (const struct symt_data*)sym;
            switch (data->kind)
            {
            case DataIsParam:
                sym_info->Flags |= SYMFLAG_PARAMETER;
                /* fall through */
            case DataIsLocal:
                {
                    struct location loc = data->u.var;

                    if (loc.kind >= loc_user)
                        pair->effective->loc_compute(pair->pcs, pair->effective, func, &loc);

                    switch (loc.kind)
                    {
                    case loc_error:
                        /* for now we report error cases as a negative register number */
                        sym_info->Flags |= SYMFLAG_LOCAL;
                        /* fall through */
                    case loc_register:
                        sym_info->Flags |= SYMFLAG_REGISTER;
                        sym_info->Register = loc.reg;
                        sym_info->Address = 0;
                        break;
                    case loc_regrel:
                        sym_info->Flags |= SYMFLAG_LOCAL | SYMFLAG_REGREL;
                        /* FIXME: it's i386 dependent !!! */
                        sym_info->Register = loc.reg ? loc.reg : CV_REG_EBP;
                        sym_info->Address = loc.offset;
                        break;
                    default:
                        FIXME("Shouldn't happen (kind=%d), debug reader backend is broken\n", loc.kind);
                        assert(0);
                    }
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
                case VT_I1 | VT_BYREF: sym_info->Value = (ULONG)data->u.value.n1.n2.n3.byref; break;
                default:
                    FIXME("Unsupported variant type (%u)\n", data->u.value.n1.n2.vt);
                    sym_info->Value = 0;
                    break;
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
            (sym_info->NameLen = UnDecorateSymbolName(name, sym_info->Name, 
                                                      sym_info->MaxNameLen, UNDNAME_COMPLETE) == 0))
        {
            sym_info->NameLen = min(strlen(name), sym_info->MaxNameLen - 1);
            memcpy(sym_info->Name, name, sym_info->NameLen);
            sym_info->Name[sym_info->NameLen] = '\0';
        }
    }
    TRACE_(dbghelp_symt)("%p => %s %u %s\n",
                         sym, sym_info->Name, sym_info->Size,
                         wine_dbgstr_longlong(sym_info->Address));
}

struct sym_enum
{
    PSYM_ENUMERATESYMBOLS_CALLBACK      cb;
    PVOID                               user;
    SYMBOL_INFO*                        sym_info;
    DWORD                               index;
    DWORD                               tag;
    DWORD64                             addr;
    char                                buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
};

static BOOL send_symbol(const struct sym_enum* se, const struct module_pair* pair,
                        const struct symt_function* func, const struct symt* sym)
{
    symt_fill_sym_info(pair, func, sym, se->sym_info);
    if (se->index && se->sym_info->info != se->index) return FALSE;
    if (se->tag && se->sym_info->Tag != se->tag) return FALSE;
    if (se->addr && !(se->addr >= se->sym_info->Address && se->addr < se->sym_info->Address + se->sym_info->Size)) return FALSE;
    return !se->cb(se->sym_info, se->sym_info->Size, se->user);
}

static BOOL symt_enum_module(struct module_pair* pair, const regex_t* regex,
                             const struct sym_enum* se)
{
    void*                       ptr;
    struct symt_ht*             sym = NULL;
    struct hash_table_iter      hti;

    hash_table_iter_init(&pair->effective->ht_symbols, &hti, NULL);
    while ((ptr = hash_table_iter_up(&hti)))
    {
        sym = GET_ENTRY(ptr, struct symt_ht, hash_elt);
        if (sym->hash_elt.name &&
            regexec(regex, sym->hash_elt.name, 0, NULL, 0) == 0)
        {
            se->sym_info->SizeOfStruct = sizeof(SYMBOL_INFO);
            se->sym_info->MaxNameLen = sizeof(se->buffer) - sizeof(SYMBOL_INFO);
            if (send_symbol(se, pair, NULL, &sym->symt)) return TRUE;
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
    void*                       ptr;
    struct symt_ht*             sym;
    struct hash_table_iter      hti;
    ULONG64                     addr;

    if (!(module->module.NumSyms = module->ht_symbols.num_elts))
        return FALSE;
    
    if (module->addr_sorttab)
        module->addr_sorttab = HeapReAlloc(GetProcessHeap(), 0,
                                           module->addr_sorttab, 
                                           module->module.NumSyms * sizeof(struct symt_ht*));
    else
        module->addr_sorttab = HeapAlloc(GetProcessHeap(), 0,
                                         module->module.NumSyms * sizeof(struct symt_ht*));
    if (!module->addr_sorttab) return FALSE;

    module->num_sorttab = 0;
    hash_table_iter_init(&module->ht_symbols, &hti, NULL);
    while ((ptr = hash_table_iter_up(&hti)))
    {
        sym = GET_ENTRY(ptr, struct symt_ht, hash_elt);
        assert(sym);
        /* Don't store in sorttab symbol without address, they are of
         * no use here (e.g. constant values)
         * As the number of those symbols is very couple (a couple per module)
         * we don't bother for the unused spots at the end of addr_sorttab
         */
        if (symt_get_info(&sym->symt, TI_GET_ADDRESS, &addr))
            module->addr_sorttab[module->num_sorttab++] = sym;
    }
    qsort(module->addr_sorttab, module->num_sorttab, sizeof(struct symt_ht*), symt_cmp_addr);
    return module->sortlist_valid = TRUE;
}

/* assume addr is in module */
struct symt_ht* symt_find_nearest(struct module* module, DWORD addr)
{
    int         mid, high, low;
    ULONG64     ref_addr, ref_size;

    if (!module->sortlist_valid || !module->addr_sorttab)
    {
        if (!resort_symbols(module)) return NULL;
    }

    /*
     * Binary search to find closest symbol.
     */
    low = 0;
    high = module->num_sorttab;

    symt_get_info(&module->addr_sorttab[0]->symt, TI_GET_ADDRESS, &ref_addr);
    if (addr < ref_addr) return NULL;
    if (high)
    {
        symt_get_info(&module->addr_sorttab[high - 1]->symt, TI_GET_ADDRESS, &ref_addr);
        if (!symt_get_info(&module->addr_sorttab[high - 1]->symt, TI_GET_LENGTH, &ref_size) || !ref_size)
            ref_size = 0x1000; /* arbitrary value */
        if (addr >= ref_addr + ref_size) return NULL;
    }
    
    while (high > low + 1)
    {
        mid = (high + low) / 2;
        if (cmp_sorttab_addr(module, mid, addr) < 0)
            low = mid;
        else
            high = mid;
    }
    if (low != high && high != module->num_sorttab &&
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
        else if (low < module->num_sorttab - 1 &&
                 module->addr_sorttab[low + 1]->symt.tag != SymTagPublicSymbol &&
                 !cmp_sorttab_addr(module, low + 1, ref_addr))
            low++;
    }
    /* finally check that we fit into the found symbol */
    symt_get_info(&module->addr_sorttab[low]->symt, TI_GET_ADDRESS, &ref_addr);
    if (addr < ref_addr) return NULL;
    if (!symt_get_info(&module->addr_sorttab[high - 1]->symt, TI_GET_LENGTH, &ref_size) || !ref_size)
        ref_size = 0x1000; /* arbitrary value */
    if (addr >= ref_addr + ref_size) return NULL;

    return module->addr_sorttab[low];
}

static BOOL symt_enum_locals_helper(struct module_pair* pair,
                                    regex_t* preg, const struct sym_enum* se,
                                    struct symt_function* func, const struct vector* v)
{
    struct symt*        lsym = NULL;
    DWORD               pc = pair->pcs->ctx_frame.InstructionOffset;
    int                 i;

    for (i=0; i<vector_length(v); i++)
    {
        lsym = *(struct symt**)vector_at(v, i);
        switch (lsym->tag)
        {
        case SymTagBlock:
            {
                struct symt_block*  block = (struct symt_block*)lsym;
                if (pc < block->address || block->address + block->size <= pc)
                    continue;
                if (!symt_enum_locals_helper(pair, preg, se, func, &block->vchildren))
                    return FALSE;
            }
            break;
        case SymTagData:
            if (regexec(preg, symt_get_name(lsym), 0, NULL, 0) == 0)
            {
                if (send_symbol(se, pair, func, lsym)) return FALSE;
            }
            break;
        case SymTagLabel:
        case SymTagFuncDebugStart:
        case SymTagFuncDebugEnd:
        case SymTagCustom:
            break;
        default:
            FIXME("Unknown type: %u (%x)\n", lsym->tag, lsym->tag);
            assert(0);
        }
    }
    return TRUE;
}

static BOOL symt_enum_locals(struct process* pcs, const char* mask, 
                             const struct sym_enum* se)
{
    struct module_pair  pair;
    struct symt_ht*     sym;
    DWORD               pc = pcs->ctx_frame.InstructionOffset;

    se->sym_info->SizeOfStruct = sizeof(*se->sym_info);
    se->sym_info->MaxNameLen = sizeof(se->buffer) - sizeof(SYMBOL_INFO);

    pair.pcs = pcs;
    pair.requested = module_find_by_addr(pair.pcs, pc, DMT_UNKNOWN);
    if (!module_get_debug(&pair)) return FALSE;
    if ((sym = symt_find_nearest(pair.effective, pc)) == NULL) return FALSE;

    if (sym->symt.tag == SymTagFunction)
    {
        BOOL            ret;
        regex_t         preg;

        compile_regex(mask ? mask : "*", -1, &preg,
                      dbghelp_options & SYMOPT_CASE_INSENSITIVE);
        ret = symt_enum_locals_helper(&pair, &preg, se, (struct symt_function*)sym,
                                      &((struct symt_function*)sym)->vchildren);
        regfree(&preg);
        return ret;
        
    }
    return send_symbol(se, &pair, NULL, &sym->symt);
}

/******************************************************************
 *		copy_symbolW
 *
 * Helper for transforming an ANSI symbol info into an UNICODE one.
 * Assume that MaxNameLen is the same for both version (A & W).
 */
void copy_symbolW(SYMBOL_INFOW* siw, const SYMBOL_INFO* si)
{
    siw->SizeOfStruct = si->SizeOfStruct;
    siw->TypeIndex = si->TypeIndex; 
    siw->Reserved[0] = si->Reserved[0];
    siw->Reserved[1] = si->Reserved[1];
    siw->Index = si->info; /* FIXME: see dbghelp.h */
    siw->Size = si->Size;
    siw->ModBase = si->ModBase;
    siw->Flags = si->Flags;
    siw->Value = si->Value;
    siw->Address = si->Address;
    siw->Register = si->Register;
    siw->Scope = si->Scope;
    siw->Tag = si->Tag;
    siw->NameLen = si->NameLen;
    siw->MaxNameLen = si->MaxNameLen;
    MultiByteToWideChar(CP_ACP, 0, si->Name, -1, siw->Name, siw->MaxNameLen);
}

/******************************************************************
 *		sym_enum
 *
 * Core routine for most of the enumeration of symbols
 */
static BOOL sym_enum(HANDLE hProcess, ULONG64 BaseOfDll, PCSTR Mask,
                     const struct sym_enum* se)
{
    struct module_pair  pair;
    const char*         bang;
    regex_t             mod_regex, sym_regex;

    pair.pcs = process_find_by_handle(hProcess);
    if (BaseOfDll == 0)
    {
        /* do local variables ? */
        if (!Mask || !(bang = strchr(Mask, '!')))
            return symt_enum_locals(pair.pcs, Mask, se);

        if (bang == Mask) return FALSE;

        compile_regex(Mask, bang - Mask, &mod_regex, TRUE);
        compile_regex(bang + 1, -1, &sym_regex, 
                      dbghelp_options & SYMOPT_CASE_INSENSITIVE);
        
        for (pair.requested = pair.pcs->lmodules; pair.requested; pair.requested = pair.requested->next)
        {
            if (pair.requested->type == DMT_PE && module_get_debug(&pair))
            {
                if (regexec(&mod_regex, pair.requested->module_name, 0, NULL, 0) == 0 &&
                    symt_enum_module(&pair, &sym_regex, se))
                    break;
            }
        }
        /* not found in PE modules, retry on the ELF ones
         */
        if (!pair.requested && (dbghelp_options & SYMOPT_WINE_WITH_ELF_MODULES))
        {
            for (pair.requested = pair.pcs->lmodules; pair.requested; pair.requested = pair.requested->next)
            {
                if (pair.requested->type == DMT_ELF &&
                    !module_get_containee(pair.pcs, pair.requested) &&
                    module_get_debug(&pair))
                {
                    if (regexec(&mod_regex, pair.requested->module_name, 0, NULL, 0) == 0 &&
                        symt_enum_module(&pair, &sym_regex, se))
                    break;
                }
            }
        }
        regfree(&mod_regex);
        regfree(&sym_regex);
        return TRUE;
    }
    pair.requested = module_find_by_addr(pair.pcs, BaseOfDll, DMT_UNKNOWN);
    if (!module_get_debug(&pair))
        return FALSE;

    /* we always ignore module name from Mask when BaseOfDll is defined */
    if (Mask && (bang = strchr(Mask, '!')))
    {
        if (bang == Mask) return FALSE;
        Mask = bang + 1;
    }

    compile_regex(Mask ? Mask : "*", -1, &sym_regex, 
                  dbghelp_options & SYMOPT_CASE_INSENSITIVE);
    symt_enum_module(&pair, &sym_regex, se);
    regfree(&sym_regex);

    return TRUE;
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
    struct sym_enum     se;

    TRACE("(%p %s %s %p %p)\n", 
          hProcess, wine_dbgstr_longlong(BaseOfDll), debugstr_a(Mask),
          EnumSymbolsCallback, UserContext);

    se.cb = EnumSymbolsCallback;
    se.user = UserContext;
    se.index = 0;
    se.tag = 0;
    se.addr = 0;
    se.sym_info = (PSYMBOL_INFO)se.buffer;

    return sym_enum(hProcess, BaseOfDll, Mask, &se);
}

struct sym_enumW
{
    PSYM_ENUMERATESYMBOLS_CALLBACKW     cb;
    void*                               ctx;
    PSYMBOL_INFOW                       sym_info;
    char                                buffer[sizeof(SYMBOL_INFOW) + MAX_SYM_NAME];

};
    
static BOOL CALLBACK sym_enumW(PSYMBOL_INFO si, ULONG size, PVOID ctx)
{
    struct sym_enumW*   sew = ctx;

    copy_symbolW(sew->sym_info, si);

    return (sew->cb)(sew->sym_info, size, sew->ctx);
}

/******************************************************************
 *		SymEnumSymbolsW (DBGHELP.@)
 *
 */
BOOL WINAPI SymEnumSymbolsW(HANDLE hProcess, ULONG64 BaseOfDll, PCWSTR Mask,
                            PSYM_ENUMERATESYMBOLS_CALLBACKW EnumSymbolsCallback,
                            PVOID UserContext)
{
    struct sym_enumW    sew;
    BOOL                ret = FALSE;
    char*               maskA = NULL;

    sew.ctx = UserContext;
    sew.cb = EnumSymbolsCallback;
    sew.sym_info = (PSYMBOL_INFOW)sew.buffer;

    if (Mask)
    {
        unsigned len = WideCharToMultiByte(CP_ACP, 0, Mask, -1, NULL, 0, NULL, NULL);
        maskA = HeapAlloc(GetProcessHeap(), 0, len);
        if (!maskA) return FALSE;
        WideCharToMultiByte(CP_ACP, 0, Mask, -1, maskA, len, NULL, NULL);
    }
    ret = SymEnumSymbols(hProcess, BaseOfDll, maskA, sym_enumW, &sew);
    HeapFree(GetProcessHeap(), 0, maskA);

    return ret;
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
    struct module_pair  pair;
    struct symt_ht*     sym;

    pair.pcs = process_find_by_handle(hProcess);
    if (!pair.pcs) return FALSE;
    pair.requested = module_find_by_addr(pair.pcs, Address, DMT_UNKNOWN);
    if (!module_get_debug(&pair)) return FALSE;
    if ((sym = symt_find_nearest(pair.effective, Address)) == NULL) return FALSE;

    symt_fill_sym_info(&pair, NULL, &sym->symt, Symbol);
    *Displacement = Address - Symbol->Address;
    return TRUE;
}

/******************************************************************
 *		SymFromAddrW (DBGHELP.@)
 *
 */
BOOL WINAPI SymFromAddrW(HANDLE hProcess, DWORD64 Address, 
                         DWORD64* Displacement, PSYMBOL_INFOW Symbol)
{
    PSYMBOL_INFO        si;
    unsigned            len;
    BOOL                ret;

    len = sizeof(*si) + Symbol->MaxNameLen * sizeof(WCHAR);
    si = HeapAlloc(GetProcessHeap(), 0, len);
    if (!si) return FALSE;

    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen = Symbol->MaxNameLen;
    if ((ret = SymFromAddr(hProcess, Address, Displacement, si)))
    {
        copy_symbolW(Symbol, si);
    }
    HeapFree(GetProcessHeap(), 0, si);
    return ret;
}

/******************************************************************
 *		SymGetSymFromAddr (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetSymFromAddr(HANDLE hProcess, DWORD Address,
                              PDWORD Displacement, PIMAGEHLP_SYMBOL Symbol)
{
    char        buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    SYMBOL_INFO*si = (SYMBOL_INFO*)buffer;
    size_t      len;
    DWORD64     Displacement64;

    if (Symbol->SizeOfStruct < sizeof(*Symbol)) return FALSE;
    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen = MAX_SYM_NAME;
    if (!SymFromAddr(hProcess, Address, &Displacement64, si))
        return FALSE;

    if (Displacement)
        *Displacement = Displacement64;
    Symbol->Address = si->Address;
    Symbol->Size    = si->Size;
    Symbol->Flags   = si->Flags;
    len = min(Symbol->MaxNameLength, si->MaxNameLen);
    lstrcpynA(Symbol->Name, si->Name, len);
    return TRUE;
}

/******************************************************************
 *		SymGetSymFromAddr64 (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetSymFromAddr64(HANDLE hProcess, DWORD64 Address,
                                PDWORD64 Displacement, PIMAGEHLP_SYMBOL64 Symbol)
{
    char        buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    SYMBOL_INFO*si = (SYMBOL_INFO*)buffer;
    size_t      len;
    DWORD64     Displacement64;

    if (Symbol->SizeOfStruct < sizeof(*Symbol)) return FALSE;
    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen = MAX_SYM_NAME;
    if (!SymFromAddr(hProcess, Address, &Displacement64, si))
        return FALSE;

    if (Displacement)
        *Displacement = Displacement64;
    Symbol->Address = si->Address;
    Symbol->Size    = si->Size;
    Symbol->Flags   = si->Flags;
    len = min(Symbol->MaxNameLength, si->MaxNameLen);
    lstrcpynA(Symbol->Name, si->Name, len);
    return TRUE;
}

static BOOL find_name(struct process* pcs, struct module* module, const char* name,
                      SYMBOL_INFO* symbol)
{
    struct hash_table_iter      hti;
    void*                       ptr;
    struct symt_ht*             sym = NULL;
    struct module_pair          pair;

    pair.pcs = pcs;
    if (!(pair.requested = module)) return FALSE;
    if (!module_get_debug(&pair)) return FALSE;

    hash_table_iter_init(&pair.effective->ht_symbols, &hti, name);
    while ((ptr = hash_table_iter_up(&hti)))
    {
        sym = GET_ENTRY(ptr, struct symt_ht, hash_elt);

        if (!strcmp(sym->hash_elt.name, name))
        {
            symt_fill_sym_info(&pair, NULL, &sym->symt, symbol);
            return TRUE;
        }
    }
    return FALSE;

}
/******************************************************************
 *		SymFromName (DBGHELP.@)
 *
 */
BOOL WINAPI SymFromName(HANDLE hProcess, PCSTR Name, PSYMBOL_INFO Symbol)
{
    struct process*             pcs = process_find_by_handle(hProcess);
    struct module*              module;
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
        module = module_find_by_nameA(pcs, tmp);
        return find_name(pcs, module, name + 1, Symbol);
    }
    for (module = pcs->lmodules; module; module = module->next)
    {
        if (module->type == DMT_PE && find_name(pcs, module, Name, Symbol))
            return TRUE;
    }
    /* not found in PE modules, retry on the ELF ones
     */
    if (dbghelp_options & SYMOPT_WINE_WITH_ELF_MODULES)
    {
        for (module = pcs->lmodules; module; module = module->next)
        {
            if (module->type == DMT_ELF && !module_get_containee(pcs, module) &&
                find_name(pcs, module, Name, Symbol))
                return TRUE;
        }
    }
    return FALSE;
}

/***********************************************************************
 *		SymGetSymFromName (DBGHELP.@)
 */
BOOL WINAPI SymGetSymFromName(HANDLE hProcess, PCSTR Name, PIMAGEHLP_SYMBOL Symbol)
{
    char        buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    SYMBOL_INFO*si = (SYMBOL_INFO*)buffer;
    size_t      len;

    if (Symbol->SizeOfStruct < sizeof(*Symbol)) return FALSE;
    si->SizeOfStruct = sizeof(*si);
    si->MaxNameLen = MAX_SYM_NAME;
    if (!SymFromName(hProcess, Name, si)) return FALSE;

    Symbol->Address = si->Address;
    Symbol->Size    = si->Size;
    Symbol->Flags   = si->Flags;
    len = min(Symbol->MaxNameLength, si->MaxNameLen);
    lstrcpynA(Symbol->Name, si->Name, len);
    return TRUE;
}

/******************************************************************
 *		sym_fill_func_line_info
 *
 * fills information about a file
 */
BOOL symt_fill_func_line_info(const struct module* module, const struct symt_function* func,
                              DWORD addr, IMAGEHLP_LINE* line)
{
    struct line_info*   dli = NULL;
    BOOL                found = FALSE;
    int                 i;

    assert(func->symt.tag == SymTagFunction);

    for (i=vector_length(&func->vlines)-1; i>=0; i--)
    {
        dli = vector_at(&func->vlines, i);
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
    struct module_pair  pair;
    struct symt_ht*     symt;

    TRACE("%p %08x %p %p\n", hProcess, dwAddr, pdwDisplacement, Line);

    if (Line->SizeOfStruct < sizeof(*Line)) return FALSE;

    pair.pcs = process_find_by_handle(hProcess);
    if (!pair.pcs) return FALSE;
    pair.requested = module_find_by_addr(pair.pcs, dwAddr, DMT_UNKNOWN);
    if (!module_get_debug(&pair)) return FALSE;
    if ((symt = symt_find_nearest(pair.effective, dwAddr)) == NULL) return FALSE;

    if (symt->symt.tag != SymTagFunction) return FALSE;
    if (!symt_fill_func_line_info(pair.effective, (struct symt_function*)symt,
                                  dwAddr, Line)) return FALSE;
    *pdwDisplacement = dwAddr - Line->Address;
    return TRUE;
}

/******************************************************************
 *		copy_line_64_from_32 (internal)
 *
 */
static void copy_line_64_from_32(IMAGEHLP_LINE64* l64, const IMAGEHLP_LINE* l32)

{
    l64->Key = l32->Key;
    l64->LineNumber = l32->LineNumber;
    l64->FileName = l32->FileName;
    l64->Address = l32->Address;
}

/******************************************************************
 *		copy_line_W64_from_32 (internal)
 *
 */
static void copy_line_W64_from_32(struct process* pcs, IMAGEHLP_LINEW64* l64, const IMAGEHLP_LINE* l32)
{
    unsigned len;

    l64->Key = l32->Key;
    l64->LineNumber = l32->LineNumber;
    len = MultiByteToWideChar(CP_ACP, 0, l32->FileName, -1, NULL, 0);
    if ((l64->FileName = fetch_buffer(pcs, len * sizeof(WCHAR))))
        MultiByteToWideChar(CP_ACP, 0, l32->FileName, -1, l64->FileName, len);
    l64->Address = l32->Address;
}

/******************************************************************
 *		copy_line_32_from_64 (internal)
 *
 */
static void copy_line_32_from_64(IMAGEHLP_LINE* l32, const IMAGEHLP_LINE64* l64)

{
    l32->Key = l64->Key;
    l32->LineNumber = l64->LineNumber;
    l32->FileName = l64->FileName;
    l32->Address = l64->Address;
}

/******************************************************************
 *		SymGetLineFromAddr64 (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLineFromAddr64(HANDLE hProcess, DWORD64 dwAddr, 
                                 PDWORD pdwDisplacement, PIMAGEHLP_LINE64 Line)
{
    IMAGEHLP_LINE       line32;

    if (Line->SizeOfStruct < sizeof(*Line)) return FALSE;
    if (!validate_addr64(dwAddr)) return FALSE;
    line32.SizeOfStruct = sizeof(line32);
    if (!SymGetLineFromAddr(hProcess, (DWORD)dwAddr, pdwDisplacement, &line32))
        return FALSE;
    copy_line_64_from_32(Line, &line32);
    return TRUE;
}

/******************************************************************
 *		SymGetLineFromAddrW64 (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLineFromAddrW64(HANDLE hProcess, DWORD64 dwAddr, 
                                  PDWORD pdwDisplacement, PIMAGEHLP_LINEW64 Line)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    IMAGEHLP_LINE       line32;

    if (!pcs) return FALSE;
    if (Line->SizeOfStruct < sizeof(*Line)) return FALSE;
    if (!validate_addr64(dwAddr)) return FALSE;
    line32.SizeOfStruct = sizeof(line32);
    if (!SymGetLineFromAddr(hProcess, (DWORD)dwAddr, pdwDisplacement, &line32))
        return FALSE;
    copy_line_W64_from_32(pcs, Line, &line32);
    return TRUE;
}

/******************************************************************
 *		SymGetLinePrev (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLinePrev(HANDLE hProcess, PIMAGEHLP_LINE Line)
{
    struct module_pair  pair;
    struct line_info*   li;
    BOOL                in_search = FALSE;

    TRACE("(%p %p)\n", hProcess, Line);

    if (Line->SizeOfStruct < sizeof(*Line)) return FALSE;

    pair.pcs = process_find_by_handle(hProcess);
    if (!pair.pcs) return FALSE;
    pair.requested = module_find_by_addr(pair.pcs, Line->Address, DMT_UNKNOWN);
    if (!module_get_debug(&pair)) return FALSE;

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
                Line->FileName = (char*)source_get(pair.effective, li->u.source_file);
                return TRUE;
            }
            in_search = TRUE;
        }
    }
    SetLastError(ERROR_NO_MORE_ITEMS); /* FIXME */
    return FALSE;
}

/******************************************************************
 *		SymGetLinePrev64 (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLinePrev64(HANDLE hProcess, PIMAGEHLP_LINE64 Line)
{
    IMAGEHLP_LINE       line32;

    line32.SizeOfStruct = sizeof(line32);
    copy_line_32_from_64(&line32, Line);
    if (!SymGetLinePrev(hProcess, &line32)) return FALSE;
    copy_line_64_from_32(Line, &line32);
    return TRUE;
}
    
BOOL symt_get_func_line_next(const struct module* module, PIMAGEHLP_LINE line)
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
    struct module_pair  pair;

    TRACE("(%p %p)\n", hProcess, Line);

    if (Line->SizeOfStruct < sizeof(*Line)) return FALSE;
    pair.pcs = process_find_by_handle(hProcess);
    if (!pair.pcs) return FALSE;
    pair.requested = module_find_by_addr(pair.pcs, Line->Address, DMT_UNKNOWN);
    if (!module_get_debug(&pair)) return FALSE;

    if (symt_get_func_line_next(pair.effective, Line)) return TRUE;
    SetLastError(ERROR_NO_MORE_ITEMS); /* FIXME */
    return FALSE;
}

/******************************************************************
 *		SymGetLineNext64 (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLineNext64(HANDLE hProcess, PIMAGEHLP_LINE64 Line)
{
    IMAGEHLP_LINE       line32;

    line32.SizeOfStruct = sizeof(line32);
    copy_line_32_from_64(&line32, Line);
    if (!SymGetLineNext(hProcess, &line32)) return FALSE;
    copy_line_64_from_32(Line, &line32);
    return TRUE;
}
    
/***********************************************************************
 *		SymFunctionTableAccess (DBGHELP.@)
 */
PVOID WINAPI SymFunctionTableAccess(HANDLE hProcess, DWORD AddrBase)
{
    WARN("(%p, 0x%08x): stub\n", hProcess, AddrBase);
    return NULL;
}

/***********************************************************************
 *		SymFunctionTableAccess64 (DBGHELP.@)
 */
PVOID WINAPI SymFunctionTableAccess64(HANDLE hProcess, DWORD64 AddrBase)
{
    WARN("(%p, %s): stub\n", hProcess, wine_dbgstr_longlong(AddrBase));
    return NULL;
}

/***********************************************************************
 *		SymUnDName (DBGHELP.@)
 */
BOOL WINAPI SymUnDName(PIMAGEHLP_SYMBOL sym, PSTR UnDecName, DWORD UnDecNameLength)
{
    TRACE("(%p %s %u)\n", sym, UnDecName, UnDecNameLength);
    return UnDecorateSymbolName(sym->Name, UnDecName, UnDecNameLength,
                                UNDNAME_COMPLETE) != 0;
}

static void* und_alloc(size_t len) { return HeapAlloc(GetProcessHeap(), 0, len); }
static void  und_free (void* ptr)  { HeapFree(GetProcessHeap(), 0, ptr); }

/***********************************************************************
 *		UnDecorateSymbolName (DBGHELP.@)
 */
DWORD WINAPI UnDecorateSymbolName(PCSTR DecoratedName, PSTR UnDecoratedName,
                                  DWORD UndecoratedLength, DWORD Flags)
{
    /* undocumented from msvcrt */
    static char* (*p_undname)(char*, const char*, int, void* (*)(size_t), void (*)(void*), unsigned short);
    static const WCHAR szMsvcrt[] = {'m','s','v','c','r','t','.','d','l','l',0};

    TRACE("(%s, %p, %d, 0x%08x)\n",
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

/******************************************************************
 *		SymMatchString (DBGHELP.@)
 *
 */
BOOL WINAPI SymMatchString(PCSTR string, PCSTR re, BOOL _case)
{
    regex_t     preg;
    BOOL        ret;

    TRACE("%s %s %c\n", string, re, _case ? 'Y' : 'N');

    compile_regex(re, -1, &preg, _case);
    ret = regexec(&preg, string, 0, NULL, 0) == 0;
    regfree(&preg);
    return ret;
}

/******************************************************************
 *		SymSearch (DBGHELP.@)
 */
BOOL WINAPI SymSearch(HANDLE hProcess, ULONG64 BaseOfDll, DWORD Index,
                      DWORD SymTag, PCSTR Mask, DWORD64 Address,
                      PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
                      PVOID UserContext, DWORD Options)
{
    struct sym_enum     se;

    TRACE("(%p %s %u %u %s %s %p %p %x)\n",
          hProcess, wine_dbgstr_longlong(BaseOfDll), Index, SymTag, Mask,
          wine_dbgstr_longlong(Address), EnumSymbolsCallback,
          UserContext, Options);

    if (Options != SYMSEARCH_GLOBALSONLY)
    {
        FIXME("Unsupported searching with options (%x)\n", Options);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    se.cb = EnumSymbolsCallback;
    se.user = UserContext;
    se.index = Index;
    se.tag = SymTag;
    se.addr = Address;
    se.sym_info = (PSYMBOL_INFO)se.buffer;

    return sym_enum(hProcess, BaseOfDll, Mask, &se);
}

/******************************************************************
 *		SymSearchW (DBGHELP.@)
 */
BOOL WINAPI SymSearchW(HANDLE hProcess, ULONG64 BaseOfDll, DWORD Index,
                       DWORD SymTag, PCWSTR Mask, DWORD64 Address,
                       PSYM_ENUMERATESYMBOLS_CALLBACKW EnumSymbolsCallback,
                       PVOID UserContext, DWORD Options)
{
    struct sym_enumW    sew;
    BOOL                ret = FALSE;
    char*               maskA = NULL;

    TRACE("(%p %s %u %u %s %s %p %p %x)\n",
          hProcess, wine_dbgstr_longlong(BaseOfDll), Index, SymTag, debugstr_w(Mask),
          wine_dbgstr_longlong(Address), EnumSymbolsCallback,
          UserContext, Options);

    sew.ctx = UserContext;
    sew.cb = EnumSymbolsCallback;
    sew.sym_info = (PSYMBOL_INFOW)sew.buffer;

    if (Mask)
    {
        unsigned len = WideCharToMultiByte(CP_ACP, 0, Mask, -1, NULL, 0, NULL, NULL);
        maskA = HeapAlloc(GetProcessHeap(), 0, len);
        if (!maskA) return FALSE;
        WideCharToMultiByte(CP_ACP, 0, Mask, -1, maskA, len, NULL, NULL);
    }
    ret = SymSearch(hProcess, BaseOfDll, Index, SymTag, maskA, Address,
                    sym_enumW, &sew, Options);
    HeapFree(GetProcessHeap(), 0, maskA);

    return ret;
}
