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

#include "dbghelp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);
WINE_DECLARE_DEBUG_CHANNEL(dbghelp_symt);

static const WCHAR starW[] = {'*','\0'};

static inline int cmp_addr(ULONG64 a1, ULONG64 a2)
{
    if (a1 > a2) return 1;
    if (a1 < a2) return -1;
    return 0;
}

static inline int cmp_sorttab_addr(struct module* module, int idx, ULONG64 addr)
{
    ULONG64     ref;
    symt_get_address(&module->addr_sorttab[idx]->symt, &ref);
    return cmp_addr(ref, addr);
}

int symt_cmp_addr(const void* p1, const void* p2)
{
    const struct symt*  sym1 = *(const struct symt* const *)p1;
    const struct symt*  sym2 = *(const struct symt* const *)p2;
    ULONG64     a1, a2;

    symt_get_address(sym1, &a1);
    symt_get_address(sym2, &a2);
    return cmp_addr(a1, a2);
}

DWORD             symt_ptr2index(struct module* module, const struct symt* sym)
{
#ifdef __x86_64__
    const struct symt** c;
    int len = vector_length(&module->vsymt);
    struct hash_table_iter hti;
    void *ptr;
    struct symt_idx_to_ptr *idx_to_ptr;
    /* place enough storage on the stack to represent a pointer in %p form */
    char ptrbuf[3 + (sizeof(void *) * 2)];

    /* make a string representation of the pointer to use as a hash key */
    sprintf(ptrbuf, "%p", sym);
    hash_table_iter_init(&module->ht_symaddr, &hti, ptrbuf);

    /* try to find the pointer in our ht */
    while ((ptr = hash_table_iter_up(&hti))) {
        idx_to_ptr = GET_ENTRY(ptr, struct symt_idx_to_ptr, hash_elt);
        if (idx_to_ptr->sym == sym)
            return idx_to_ptr->idx;
    }

    /* not found */
    /* add the symbol to our symbol vector */
    c = vector_add(&module->vsymt, &module->pool);

    /* add an idx to ptr mapping so we can find it again by address */
    if ((idx_to_ptr = pool_alloc(&module->pool, sizeof(*idx_to_ptr)))) 
    {
        idx_to_ptr->hash_elt.name = pool_strdup(&module->pool, ptrbuf);
        idx_to_ptr->sym = sym;
        idx_to_ptr->idx = len + 1;
        hash_table_add(&module->ht_symaddr, &idx_to_ptr->hash_elt);
    }

    if (c) *c = sym;
    return len + 1;
#else
    return (DWORD)sym;
#endif
}

struct symt*      symt_index2ptr(struct module* module, DWORD id)
{
#ifdef __x86_64__
    if (!id-- || id >= vector_length(&module->vsymt)) return NULL;
    return *(struct symt**)vector_at(&module->vsymt, id);
#else
    return (struct symt*)id;
#endif
}

static BOOL symt_grow_sorttab(struct module* module, unsigned sz)
{
    struct symt_ht**    new;
    unsigned int size;

    if (sz <= module->sorttab_size) return TRUE;
    if (module->addr_sorttab)
    {
        size = module->sorttab_size * 2;
        new = HeapReAlloc(GetProcessHeap(), 0, module->addr_sorttab,
                          size * sizeof(struct symt_ht*));
    }
    else
    {
        size = 64;
        new = HeapAlloc(GetProcessHeap(), 0, size * sizeof(struct symt_ht*));
    }
    if (!new) return FALSE;
    module->sorttab_size = size;
    module->addr_sorttab = new;
    return TRUE;
}

static void symt_add_module_ht(struct module* module, struct symt_ht* ht)
{
    ULONG64             addr;

    hash_table_add(&module->ht_symbols, &ht->hash_elt);
    /* Don't store in sorttab a symbol without address, they are of
     * no use here (e.g. constant values)
     */
    if (symt_get_address(&ht->symt, &addr) &&
        symt_grow_sorttab(module, module->num_symbols + 1))
    {
        module->addr_sorttab[module->num_symbols++] = ht;
        module->sortlist_valid = FALSE;
    }
}

static WCHAR* file_regex(const char* srcfile)
{
    WCHAR* mask;
    WCHAR* p;

    if (!srcfile || !*srcfile)
    {
        if (!(p = mask = HeapAlloc(GetProcessHeap(), 0, 3 * sizeof(WCHAR)))) return NULL;
        *p++ = '?';
        *p++ = '#';
    }
    else
    {
        DWORD  sz = MultiByteToWideChar(CP_ACP, 0, srcfile, -1, NULL, 0);
        WCHAR* srcfileW;

        /* FIXME: we use here the largest conversion for every char... could be optimized */
        p = mask = HeapAlloc(GetProcessHeap(), 0, (5 * strlen(srcfile) + 1 + sz) * sizeof(WCHAR));
        if (!mask) return NULL;
        srcfileW = mask + 5 * strlen(srcfile) + 1;
        MultiByteToWideChar(CP_ACP, 0, srcfile, -1, srcfileW, sz);

        while (*srcfileW)
        {
            switch (*srcfileW)
            {
            case '\\':
            case '/':
                *p++ = '[';
                *p++ = '\\';
                *p++ = '\\';
                *p++ = '/';
                *p++ = ']';
                break;
            case '.':
                *p++ = '?';
                break;
            default:
                *p++ = *srcfileW;
                break;
            }
            srcfileW++;
        }
    }
    *p = 0;
    return mask;
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
                                    unsigned long address, unsigned size)
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
        sym->container     = compiland ? &compiland->symt : NULL;
        sym->address       = address;
        sym->size          = size;
        symt_add_module_ht(module, (struct symt_ht*)sym);
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
                                           struct location loc, unsigned long size,
                                           struct symt* type)
{
    struct symt_data*   sym;
    struct symt**       p;
    DWORD64             tsz;

    TRACE_(dbghelp_symt)("Adding global symbol %s:%s %d@%lx %p\n",
                         debugstr_w(module->module.ModuleName), name, loc.kind, loc.offset, type);
    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag      = SymTagData;
        sym->hash_elt.name = pool_strdup(&module->pool, name);
        sym->kind          = is_static ? DataIsFileStatic : DataIsGlobal;
        sym->container     = compiland ? &compiland->symt : NULL;
        sym->type          = type;
        sym->u.var         = loc;
        if (type && size && symt_get_info(module, type, TI_GET_LENGTH, &tsz))
        {
            if (tsz != size)
                FIXME("Size mismatch for %s.%s between type (%s) and src (%lu)\n",
                      debugstr_w(module->module.ModuleName), name,
                      wine_dbgstr_longlong(tsz), size);
        }
        symt_add_module_ht(module, (struct symt_ht*)sym);
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
        sym->container = &compiland->symt;
        sym->address   = addr;
        sym->type      = sig_type;
        sym->size      = size;
        vector_init(&sym->vlines,  sizeof(struct line_info), 64);
        vector_init(&sym->vchildren, sizeof(struct symt*), 8);
        symt_add_module_ht(module, (struct symt_ht*)sym);
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
    locsym->container     = block ? &block->symt : &func->symt;
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
                                         const struct symt_function* func,
                                         struct symt_block* block, unsigned pc)
{
    assert(func);
    assert(func->symt.tag == SymTagFunction);

    if (pc) block->size = func->address + pc - block->address;
    return (block->container->tag == SymTagBlock) ? 
        GET_ENTRY(block->container, struct symt_block, symt) : NULL;
}

struct symt_hierarchy_point* symt_add_function_point(struct module* module,
                                                     struct symt_function* func,
                                                     enum SymTagEnum point,
                                                     const struct location* loc,
                                                     const char* name)
{
    struct symt_hierarchy_point*sym;
    struct symt**               p;

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag = point;
        sym->parent   = &func->symt;
        sym->loc      = *loc;
        sym->hash_elt.name = name ? pool_strdup(&module->pool, name) : NULL;
        p = vector_add(&func->vchildren, &module->pool);
        *p = &sym->symt;
    }
    return sym;
}

BOOL symt_normalize_function(struct module* module, const struct symt_function* func)
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
        sym->container = &compiland->symt;
        sym->address   = addr;
        sym->size      = size;
        sym->ordinal   = ord;
        symt_add_module_ht(module, (struct symt_ht*)sym);
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
        sym->kind          = DataIsConstant;
        sym->container     = compiland ? &compiland->symt : NULL;
        sym->type          = type;
        sym->u.value       = *v;
        symt_add_module_ht(module, (struct symt_ht*)sym);
        if (compiland)
        {
            struct symt**       p;
            p = vector_add(&compiland->vchildren, &module->pool);
            *p = &sym->symt;
        }
    }
    return sym;
}

struct symt_hierarchy_point* symt_new_label(struct module* module,
                                            struct symt_compiland* compiland,
                                            const char* name, unsigned long address)
{
    struct symt_hierarchy_point*        sym;

    TRACE_(dbghelp_symt)("Adding global label value %s:%s\n",
                         debugstr_w(module->module.ModuleName), name);

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag      = SymTagLabel;
        sym->hash_elt.name = pool_strdup(&module->pool, name);
        sym->loc.kind      = loc_absolute;
        sym->loc.offset    = address;
        sym->parent        = compiland ? &compiland->symt : NULL;
        symt_add_module_ht(module, (struct symt_ht*)sym);
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
static void symt_fill_sym_info(struct module_pair* pair,
                               const struct symt_function* func,
                               const struct symt* sym, SYMBOL_INFO* sym_info)
{
    const char* name;
    DWORD64 size;

    if (!symt_get_info(pair->effective, sym, TI_GET_TYPE, &sym_info->TypeIndex))
        sym_info->TypeIndex = 0;
    sym_info->info = symt_ptr2index(pair->effective, sym);
    sym_info->Reserved[0] = sym_info->Reserved[1] = 0;
    if (!symt_get_info(pair->effective, sym, TI_GET_LENGTH, &size) &&
        (!sym_info->TypeIndex ||
         !symt_get_info(pair->effective, symt_index2ptr(pair->effective, sym_info->TypeIndex),
                         TI_GET_LENGTH, &size)))
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
                sym_info->Flags |= SYMFLAG_LOCAL;
                {
                    struct location loc = data->u.var;

                    if (loc.kind >= loc_user)
                    {
                        unsigned                i;
                        struct module_format*   modfmt;

                        for (i = 0; i < DFI_LAST; i++)
                        {
                            modfmt = pair->effective->format_info[i];
                            if (modfmt && modfmt->loc_compute)
                            {
                                modfmt->loc_compute(pair->pcs, modfmt, func, &loc);
                                break;
                            }
                        }
                    }
                    switch (loc.kind)
                    {
                    case loc_error:
                        /* for now we report error cases as a negative register number */
                        /* fall through */
                    case loc_register:
                        sym_info->Flags |= SYMFLAG_REGISTER;
                        sym_info->Register = loc.reg;
                        sym_info->Address = 0;
                        break;
                    case loc_regrel:
                        sym_info->Flags |= SYMFLAG_REGREL;
                        sym_info->Register = loc.reg;
                        if (loc.reg == CV_REG_NONE || (int)loc.reg < 0 /* error */)
                            FIXME("suspicious register value %x\n", loc.reg);
                        sym_info->Address = loc.offset;
                        break;
                    case loc_absolute:
                        sym_info->Flags |= SYMFLAG_VALUEPRESENT;
                        sym_info->Value = loc.offset;
                        break;
                    default:
                        FIXME("Shouldn't happen (kind=%d), debug reader backend is broken\n", loc.kind);
                        assert(0);
                    }
                }
                break;
            case DataIsGlobal:
            case DataIsFileStatic:
                switch (data->u.var.kind)
                {
                case loc_tlsrel:
                    sym_info->Flags |= SYMFLAG_TLSREL;
                    /* fall through */
                case loc_absolute:
                    symt_get_address(sym, &sym_info->Address);
                    sym_info->Register = 0;
                    break;
                default:
                    FIXME("Shouldn't happen (kind=%d), debug reader backend is broken\n", data->u.var.kind);
                    assert(0);
                }
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
                case VT_I1 | VT_BYREF: sym_info->Value = (ULONG64)(DWORD_PTR)data->u.value.n1.n2.n3.byref; break;
                case VT_EMPTY: sym_info->Value = 0; break;
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
        symt_get_address(sym, &sym_info->Address);
        break;
    case SymTagFunction:
        sym_info->Flags |= SYMFLAG_FUNCTION;
        symt_get_address(sym, &sym_info->Address);
        break;
    case SymTagThunk:
        sym_info->Flags |= SYMFLAG_THUNK;
        symt_get_address(sym, &sym_info->Address);
        break;
    default:
        symt_get_address(sym, &sym_info->Address);
        sym_info->Register = 0;
        break;
    }
    sym_info->Scope = 0; /* FIXME */
    sym_info->Tag = sym->tag;
    name = symt_get_name(sym);
    if (sym_info->MaxNameLen)
    {
        if (sym->tag != SymTagPublicSymbol || !(dbghelp_options & SYMOPT_UNDNAME) ||
            ((sym_info->NameLen = UnDecorateSymbolName(name, sym_info->Name,
                                                       sym_info->MaxNameLen, UNDNAME_NAME_ONLY)) == 0))
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

static BOOL send_symbol(const struct sym_enum* se, struct module_pair* pair,
                        const struct symt_function* func, const struct symt* sym)
{
    symt_fill_sym_info(pair, func, sym, se->sym_info);
    if (se->index && se->sym_info->info != se->index) return FALSE;
    if (se->tag && se->sym_info->Tag != se->tag) return FALSE;
    if (se->addr && !(se->addr >= se->sym_info->Address && se->addr < se->sym_info->Address + se->sym_info->Size)) return FALSE;
    return !se->cb(se->sym_info, se->sym_info->Size, se->user);
}

static BOOL symt_enum_module(struct module_pair* pair, const WCHAR* match,
                             const struct sym_enum* se)
{
    void*                       ptr;
    struct symt_ht*             sym = NULL;
    struct hash_table_iter      hti;
    WCHAR*                      nameW;
    BOOL                        ret;

    hash_table_iter_init(&pair->effective->ht_symbols, &hti, NULL);
    while ((ptr = hash_table_iter_up(&hti)))
    {
        sym = GET_ENTRY(ptr, struct symt_ht, hash_elt);
        nameW = symt_get_nameW(&sym->symt);
        ret = SymMatchStringW(nameW, match, FALSE);
        HeapFree(GetProcessHeap(), 0, nameW);
        if (ret)
        {
            se->sym_info->SizeOfStruct = sizeof(SYMBOL_INFO);
            se->sym_info->MaxNameLen = sizeof(se->buffer) - sizeof(SYMBOL_INFO);
            if (send_symbol(se, pair, NULL, &sym->symt)) return TRUE;
        }
    }
    return FALSE;
}

static inline unsigned where_to_insert(struct module* module, unsigned high, const struct symt_ht* elt)
{
    unsigned    low = 0, mid = high / 2;
    ULONG64     addr;

    if (!high) return 0;
    symt_get_address(&elt->symt, &addr);
    do
    {
        switch (cmp_sorttab_addr(module, mid, addr))
        {
        case 0: return mid;
        case -1: low = mid + 1; break;
        case 1: high = mid; break;
        }
        mid = low + (high - low) / 2;
    } while (low < high);
    return mid;
}

/***********************************************************************
 *              resort_symbols
 *
 * Rebuild sorted list of symbols for a module.
 */
static BOOL resort_symbols(struct module* module)
{
    int delta;

    if (!(module->module.NumSyms = module->num_symbols))
        return FALSE;

    /* we know that set from 0 up to num_sorttab is already sorted
     * so sort the remaining (new) symbols, and merge the two sets
     * (unless the first set is empty)
     */
    delta = module->num_symbols - module->num_sorttab;
    qsort(&module->addr_sorttab[module->num_sorttab], delta, sizeof(struct symt_ht*), symt_cmp_addr);
    if (module->num_sorttab)
    {
        int     i, ins_idx = module->num_sorttab, prev_ins_idx;
        static struct symt_ht** tmp;
        static unsigned num_tmp;

        if (num_tmp < delta)
        {
            static struct symt_ht** new;
            if (tmp)
                new = HeapReAlloc(GetProcessHeap(), 0, tmp, delta * sizeof(struct symt_ht*));
            else
                new = HeapAlloc(GetProcessHeap(), 0, delta * sizeof(struct symt_ht*));
            if (!new)
            {
                module->num_sorttab = 0;
                return resort_symbols(module);
            }
            tmp = new;
            num_tmp = delta;
        }
        memcpy(tmp, &module->addr_sorttab[module->num_sorttab], delta * sizeof(struct symt_ht*));
        qsort(tmp, delta, sizeof(struct symt_ht*), symt_cmp_addr);

        for (i = delta - 1; i >= 0; i--)
        {
            prev_ins_idx = ins_idx;
            ins_idx = where_to_insert(module, ins_idx, tmp[i]);
            memmove(&module->addr_sorttab[ins_idx + i + 1],
                    &module->addr_sorttab[ins_idx],
                    (prev_ins_idx - ins_idx) * sizeof(struct symt_ht*));
            module->addr_sorttab[ins_idx + i] = tmp[i];
        }
    }
    module->num_sorttab = module->num_symbols;
    return module->sortlist_valid = TRUE;
}

static void symt_get_length(struct module* module, const struct symt* symt, ULONG64* size)
{
    DWORD       type_index;

    if (symt_get_info(module,  symt, TI_GET_LENGTH, size) && *size)
        return;

    if (symt_get_info(module, symt, TI_GET_TYPE, &type_index) &&
        symt_get_info(module, symt_index2ptr(module, type_index), TI_GET_LENGTH, size)) return;
    *size = 0x1000; /* arbitrary value */
}

/* assume addr is in module */
struct symt_ht* symt_find_nearest(struct module* module, DWORD_PTR addr)
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

    symt_get_address(&module->addr_sorttab[0]->symt, &ref_addr);
    if (addr < ref_addr) return NULL;
    if (high)
    {
        symt_get_address(&module->addr_sorttab[high - 1]->symt, &ref_addr);
        symt_get_length(module, &module->addr_sorttab[high - 1]->symt, &ref_size);
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
        symt_get_address(&module->addr_sorttab[low]->symt, &ref_addr);
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
    symt_get_address(&module->addr_sorttab[low]->symt, &ref_addr);
    if (addr < ref_addr) return NULL;
    symt_get_length(module, &module->addr_sorttab[low]->symt, &ref_size);
    if (addr >= ref_addr + ref_size) return NULL;

    return module->addr_sorttab[low];
}

static BOOL symt_enum_locals_helper(struct module_pair* pair,
                                    const WCHAR* match, const struct sym_enum* se,
                                    struct symt_function* func, const struct vector* v)
{
    struct symt*        lsym = NULL;
    DWORD               pc = pair->pcs->ctx_frame.InstructionOffset;
    unsigned int        i;
    WCHAR*              nameW;
    BOOL                ret;

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
                if (!symt_enum_locals_helper(pair, match, se, func, &block->vchildren))
                    return FALSE;
            }
            break;
        case SymTagData:
            nameW = symt_get_nameW(lsym);
            ret = SymMatchStringW(nameW, match,
                                  !(dbghelp_options & SYMOPT_CASE_INSENSITIVE));
            HeapFree(GetProcessHeap(), 0, nameW);
            if (ret)
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

static BOOL symt_enum_locals(struct process* pcs, const WCHAR* mask,
                             const struct sym_enum* se)
{
    struct module_pair  pair;
    struct symt_ht*     sym;
    DWORD_PTR           pc = pcs->ctx_frame.InstructionOffset;

    se->sym_info->SizeOfStruct = sizeof(*se->sym_info);
    se->sym_info->MaxNameLen = sizeof(se->buffer) - sizeof(SYMBOL_INFO);

    pair.pcs = pcs;
    pair.requested = module_find_by_addr(pair.pcs, pc, DMT_UNKNOWN);
    if (!module_get_debug(&pair)) return FALSE;
    if ((sym = symt_find_nearest(pair.effective, pc)) == NULL) return FALSE;

    if (sym->symt.tag == SymTagFunction)
    {
        return symt_enum_locals_helper(&pair, mask ? mask : starW, se, (struct symt_function*)sym,
                                       &((struct symt_function*)sym)->vchildren);
    }
    return FALSE;
}

/******************************************************************
 *		copy_symbolW
 *
 * Helper for transforming an ANSI symbol info into a UNICODE one.
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
static BOOL sym_enum(HANDLE hProcess, ULONG64 BaseOfDll, PCWSTR Mask,
                     const struct sym_enum* se)
{
    struct module_pair  pair;
    const WCHAR*        bang;
    WCHAR*              mod;

    pair.pcs = process_find_by_handle(hProcess);
    if (!pair.pcs) return FALSE;
    if (BaseOfDll == 0)
    {
        /* do local variables ? */
        if (!Mask || !(bang = strchrW(Mask, '!')))
            return symt_enum_locals(pair.pcs, Mask, se);

        if (bang == Mask) return FALSE;

        mod = HeapAlloc(GetProcessHeap(), 0, (bang - Mask + 1) * sizeof(WCHAR));
        if (!mod) return FALSE;
        memcpy(mod, Mask, (bang - Mask) * sizeof(WCHAR));
        mod[bang - Mask] = 0;

        for (pair.requested = pair.pcs->lmodules; pair.requested; pair.requested = pair.requested->next)
        {
            if (pair.requested->type == DMT_PE && module_get_debug(&pair))
            {
                if (SymMatchStringW(pair.requested->module.ModuleName, mod, FALSE) &&
                    symt_enum_module(&pair, bang + 1, se))
                    break;
            }
        }
        /* not found in PE modules, retry on the ELF ones
         */
        if (!pair.requested && (dbghelp_options & SYMOPT_WINE_WITH_NATIVE_MODULES))
        {
            for (pair.requested = pair.pcs->lmodules; pair.requested; pair.requested = pair.requested->next)
            {
                if ((pair.requested->type == DMT_ELF || pair.requested->type == DMT_MACHO) &&
                    !module_get_containee(pair.pcs, pair.requested) &&
                    module_get_debug(&pair))
                {
                    if (SymMatchStringW(pair.requested->module.ModuleName, mod, FALSE) &&
                        symt_enum_module(&pair, bang + 1, se))
                    break;
                }
            }
        }
        HeapFree(GetProcessHeap(), 0, mod);
        return TRUE;
    }
    pair.requested = module_find_by_addr(pair.pcs, BaseOfDll, DMT_UNKNOWN);
    if (!module_get_debug(&pair))
        return FALSE;

    /* we always ignore module name from Mask when BaseOfDll is defined */
    if (Mask && (bang = strchrW(Mask, '!')))
    {
        if (bang == Mask) return FALSE;
        Mask = bang + 1;
    }

    symt_enum_module(&pair, Mask ? Mask : starW, se);

    return TRUE;
}

static inline BOOL doSymEnumSymbols(HANDLE hProcess, ULONG64 BaseOfDll, PCWSTR Mask,
                                    PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
                                    PVOID UserContext)
{
    struct sym_enum     se;

    se.cb = EnumSymbolsCallback;
    se.user = UserContext;
    se.index = 0;
    se.tag = 0;
    se.addr = 0;
    se.sym_info = (PSYMBOL_INFO)se.buffer;

    return sym_enum(hProcess, BaseOfDll, Mask, &se);
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
    BOOL                ret;
    PWSTR               maskW = NULL;

    TRACE("(%p %s %s %p %p)\n",
          hProcess, wine_dbgstr_longlong(BaseOfDll), debugstr_a(Mask),
          EnumSymbolsCallback, UserContext);

    if (Mask)
    {
        DWORD sz = MultiByteToWideChar(CP_ACP, 0, Mask, -1, NULL, 0);
        if (!(maskW = HeapAlloc(GetProcessHeap(), 0, sz * sizeof(WCHAR))))
            return FALSE;
        MultiByteToWideChar(CP_ACP, 0, Mask, -1, maskW, sz);
    }
    ret = doSymEnumSymbols(hProcess, BaseOfDll, maskW, EnumSymbolsCallback, UserContext);
    HeapFree(GetProcessHeap(), 0, maskW);
    return ret;
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

    sew.ctx = UserContext;
    sew.cb = EnumSymbolsCallback;
    sew.sym_info = (PSYMBOL_INFOW)sew.buffer;

    return doSymEnumSymbols(hProcess, BaseOfDll, Mask, sym_enumW, &sew);
}

struct sym_enumerate
{
    void*                       ctx;
    PSYM_ENUMSYMBOLS_CALLBACK   cb;
};

static BOOL CALLBACK sym_enumerate_cb(PSYMBOL_INFO syminfo, ULONG size, void* ctx)
{
    struct sym_enumerate*       se = ctx;
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

struct sym_enumerate64
{
    void*                       ctx;
    PSYM_ENUMSYMBOLS_CALLBACK64 cb;
};

static BOOL CALLBACK sym_enumerate_cb64(PSYMBOL_INFO syminfo, ULONG size, void* ctx)
{
    struct sym_enumerate64*     se = ctx;
    return (se->cb)(syminfo->Name, syminfo->Address, syminfo->Size, se->ctx);
}

/***********************************************************************
 *              SymEnumerateSymbols64 (DBGHELP.@)
 */
BOOL WINAPI SymEnumerateSymbols64(HANDLE hProcess, DWORD64 BaseOfDll,
                                  PSYM_ENUMSYMBOLS_CALLBACK64 EnumSymbolsCallback,
                                  PVOID UserContext)
{
    struct sym_enumerate64      se;

    se.ctx = UserContext;
    se.cb  = EnumSymbolsCallback;

    return SymEnumSymbols(hProcess, BaseOfDll, NULL, sym_enumerate_cb64, &se);
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
    if (Displacement)
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
    if (dbghelp_options & SYMOPT_WINE_WITH_NATIVE_MODULES)
    {
        for (module = pcs->lmodules; module; module = module->next)
        {
            if ((module->type == DMT_ELF || module->type == DMT_MACHO) &&
                !module_get_containee(pcs, module) &&
                find_name(pcs, module, Name, Symbol))
                return TRUE;
        }
    }
    return FALSE;
}

/***********************************************************************
 *		SymGetSymFromName64 (DBGHELP.@)
 */
BOOL WINAPI SymGetSymFromName64(HANDLE hProcess, PCSTR Name, PIMAGEHLP_SYMBOL64 Symbol)
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
                              DWORD64 addr, IMAGEHLP_LINE64* line)
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
 *		SymGetSymNext64 (DBGHELP.@)
 */
BOOL WINAPI SymGetSymNext64(HANDLE hProcess, PIMAGEHLP_SYMBOL64 Symbol)
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
 *		SymGetSymNext (DBGHELP.@)
 */
BOOL WINAPI SymGetSymNext(HANDLE hProcess, PIMAGEHLP_SYMBOL Symbol)
{
    FIXME("(%p, %p): stub\n", hProcess, Symbol);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		SymGetSymPrev64 (DBGHELP.@)
 */
BOOL WINAPI SymGetSymPrev64(HANDLE hProcess, PIMAGEHLP_SYMBOL64 Symbol)
{
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
static void copy_line_W64_from_64(struct process* pcs, IMAGEHLP_LINEW64* l64w, const IMAGEHLP_LINE64* l64)
{
    unsigned len;

    l64w->Key = l64->Key;
    l64w->LineNumber = l64->LineNumber;
    len = MultiByteToWideChar(CP_ACP, 0, l64->FileName, -1, NULL, 0);
    if ((l64w->FileName = fetch_buffer(pcs, len * sizeof(WCHAR))))
        MultiByteToWideChar(CP_ACP, 0, l64->FileName, -1, l64w->FileName, len);
    l64w->Address = l64->Address;
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
 *		SymGetLineFromAddr (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLineFromAddr(HANDLE hProcess, DWORD dwAddr,
                               PDWORD pdwDisplacement, PIMAGEHLP_LINE Line)
{
    IMAGEHLP_LINE64     il64;

    il64.SizeOfStruct = sizeof(il64);
    if (!SymGetLineFromAddr64(hProcess, dwAddr, pdwDisplacement, &il64))
        return FALSE;
    copy_line_32_from_64(Line, &il64);
    return TRUE;
}

/******************************************************************
 *		SymGetLineFromAddr64 (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLineFromAddr64(HANDLE hProcess, DWORD64 dwAddr, 
                                 PDWORD pdwDisplacement, PIMAGEHLP_LINE64 Line)
{
    struct module_pair  pair;
    struct symt_ht*     symt;

    TRACE("%p %s %p %p\n", hProcess, wine_dbgstr_longlong(dwAddr), pdwDisplacement, Line);

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
 *		SymGetLineFromAddrW64 (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLineFromAddrW64(HANDLE hProcess, DWORD64 dwAddr, 
                                  PDWORD pdwDisplacement, PIMAGEHLP_LINEW64 Line)
{
    IMAGEHLP_LINE64     il64;

    il64.SizeOfStruct = sizeof(il64);
    if (!SymGetLineFromAddr64(hProcess, dwAddr, pdwDisplacement, &il64))
        return FALSE;
    copy_line_W64_from_64(process_find_by_handle(hProcess), Line, &il64);
    return TRUE;
}

/******************************************************************
 *		SymGetLinePrev64 (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLinePrev64(HANDLE hProcess, PIMAGEHLP_LINE64 Line)
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
    li = Line->Key;
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
 *		SymGetLinePrev (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLinePrev(HANDLE hProcess, PIMAGEHLP_LINE Line)
{
    IMAGEHLP_LINE64     line64;

    line64.SizeOfStruct = sizeof(line64);
    copy_line_64_from_32(&line64, Line);
    if (!SymGetLinePrev64(hProcess, &line64)) return FALSE;
    copy_line_32_from_64(Line, &line64);
    return TRUE;
}

BOOL symt_get_func_line_next(const struct module* module, PIMAGEHLP_LINE64 line)
{
    struct line_info*   li;

    if (line->Key == 0) return FALSE;
    li = line->Key;
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
 *		SymGetLineNext64 (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLineNext64(HANDLE hProcess, PIMAGEHLP_LINE64 Line)
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
 *		SymGetLineNext (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetLineNext(HANDLE hProcess, PIMAGEHLP_LINE Line)
{
    IMAGEHLP_LINE64     line64;

    line64.SizeOfStruct = sizeof(line64);
    copy_line_64_from_32(&line64, Line);
    if (!SymGetLineNext64(hProcess, &line64)) return FALSE;
    copy_line_32_from_64(Line, &line64);
    return TRUE;
}

/***********************************************************************
 *		SymUnDName (DBGHELP.@)
 */
BOOL WINAPI SymUnDName(PIMAGEHLP_SYMBOL sym, PSTR UnDecName, DWORD UnDecNameLength)
{
    return UnDecorateSymbolName(sym->Name, UnDecName, UnDecNameLength,
                                UNDNAME_COMPLETE) != 0;
}

/***********************************************************************
 *		SymUnDName64 (DBGHELP.@)
 */
BOOL WINAPI SymUnDName64(PIMAGEHLP_SYMBOL64 sym, PSTR UnDecName, DWORD UnDecNameLength)
{
    return UnDecorateSymbolName(sym->Name, UnDecName, UnDecNameLength,
                                UNDNAME_COMPLETE) != 0;
}

static void * CDECL und_alloc(size_t len) { return HeapAlloc(GetProcessHeap(), 0, len); }
static void   CDECL und_free (void* ptr)  { HeapFree(GetProcessHeap(), 0, ptr); }

static char *und_name(char *buffer, const char *mangled, int buflen, unsigned short flags)
{
    /* undocumented from msvcrt */
    static HANDLE hMsvcrt;
    static char* (CDECL *p_undname)(char*, const char*, int, void* (CDECL*)(size_t), void (CDECL*)(void*), unsigned short);
    static const WCHAR szMsvcrt[] = {'m','s','v','c','r','t','.','d','l','l',0};

    if (!p_undname)
    {
        if (!hMsvcrt) hMsvcrt = LoadLibraryW(szMsvcrt);
        if (hMsvcrt) p_undname = (void*)GetProcAddress(hMsvcrt, "__unDName");
        if (!p_undname) return NULL;
    }

    return p_undname(buffer, mangled, buflen, und_alloc, und_free, flags);
}

/***********************************************************************
 *		UnDecorateSymbolName (DBGHELP.@)
 */
DWORD WINAPI UnDecorateSymbolName(const char *decorated_name, char *undecorated_name,
                                  DWORD undecorated_length, DWORD flags)
{
    TRACE("(%s, %p, %d, 0x%08x)\n",
          debugstr_a(decorated_name), undecorated_name, undecorated_length, flags);

    if (!undecorated_name || !undecorated_length)
        return 0;
    if (!und_name(undecorated_name, decorated_name, undecorated_length, flags))
        return 0;
    return strlen(undecorated_name);
}

/***********************************************************************
 *		UnDecorateSymbolNameW (DBGHELP.@)
 */
DWORD WINAPI UnDecorateSymbolNameW(const WCHAR *decorated_name, WCHAR *undecorated_name,
                                   DWORD undecorated_length, DWORD flags)
{
    char *buf, *ptr;
    int len, ret = 0;

    TRACE("(%s, %p, %d, 0x%08x)\n",
          debugstr_w(decorated_name), undecorated_name, undecorated_length, flags);

    if (!undecorated_name || !undecorated_length)
        return 0;

    len = WideCharToMultiByte(CP_ACP, 0, decorated_name, -1, NULL, 0, NULL, NULL);
    if ((buf = HeapAlloc(GetProcessHeap(), 0, len)))
    {
        WideCharToMultiByte(CP_ACP, 0, decorated_name, -1, buf, len, NULL, NULL);
        if ((ptr = und_name(NULL, buf, 0, flags)))
        {
            MultiByteToWideChar(CP_ACP, 0, ptr, -1, undecorated_name, undecorated_length);
            undecorated_name[undecorated_length - 1] = 0;
            ret = strlenW(undecorated_name);
            und_free(ptr);
        }
        HeapFree(GetProcessHeap(), 0, buf);
    }

    return ret;
}

#define WILDCHAR(x)      (-(x))

static  int     re_fetch_char(const WCHAR** re)
{
    switch (**re)
    {
    case '\\': (*re)++; return *(*re)++;
    case '*': case '[': case '?': case '+': case '#': case ']': return WILDCHAR(*(*re)++);
    default: return *(*re)++;
    }
}

static inline int  re_match_char(WCHAR ch1, WCHAR ch2, BOOL _case)
{
    return _case ? ch1 - ch2 : toupperW(ch1) - toupperW(ch2);
}

static const WCHAR* re_match_one(const WCHAR* string, const WCHAR* elt, BOOL _case)
{
    int         ch1, prev = 0;
    unsigned    state = 0;

    switch (ch1 = re_fetch_char(&elt))
    {
    default:
        return (ch1 >= 0 && re_match_char(*string, ch1, _case) == 0) ? ++string : NULL;
    case WILDCHAR('?'): return *string ? ++string : NULL;
    case WILDCHAR('*'): assert(0);
    case WILDCHAR('['): break;
    }

    for (;;)
    {
        ch1 = re_fetch_char(&elt);
        if (ch1 == WILDCHAR(']')) return NULL;
        if (state == 1 && ch1 == '-') state = 2;
        else
        {
            if (re_match_char(*string, ch1, _case) == 0) return ++string;
            switch (state)
            {
            case 0:
                state = 1;
                prev = ch1;
                break;
            case 1:
                state = 0;
                break;
            case 2:
                if (prev >= 0 && ch1 >= 0 && re_match_char(prev, *string, _case) <= 0 &&
                    re_match_char(*string, ch1, _case) <= 0)
                    return ++string;
                state = 0;
                break;
            }
        }
    }
}

/******************************************************************
 *		re_match_multi
 *
 * match a substring of *pstring according to *pre regular expression
 * pstring and pre are only updated in case of successful match
 */
static BOOL re_match_multi(const WCHAR** pstring, const WCHAR** pre, BOOL _case)
{
    const WCHAR* re_end = *pre;
    const WCHAR* string_end = *pstring;
    const WCHAR* re_beg;
    const WCHAR* string_beg;
    const WCHAR* next;
    int          ch;

    while (*re_end && *string_end)
    {
        string_beg = string_end;
        re_beg = re_end;
        switch (ch = re_fetch_char(&re_end))
        {
        case WILDCHAR(']'): case WILDCHAR('+'): case WILDCHAR('#'): return FALSE;
        case WILDCHAR('*'):
            /* transform '*' into '?#' */
            {static const WCHAR qmW[] = {'?',0}; re_beg = qmW;}
            goto closure;
        case WILDCHAR('['):
            do
            {
                if (!(ch = re_fetch_char(&re_end))) return FALSE;
            } while (ch != WILDCHAR(']'));
            /* fall through */
        case WILDCHAR('?'):
        default:
            break;
        }

        switch (*re_end)
        {
        case '+':
            if (!(next = re_match_one(string_end, re_beg, _case))) return FALSE;
            string_beg++;
            /* fall through */
        case '#':
            re_end++;
        closure:
            while ((next = re_match_one(string_end, re_beg, _case))) string_end = next;
            for ( ; string_end >= string_beg; string_end--)
            {
                if (re_match_multi(&string_end, &re_end, _case)) goto found;
            }
            return FALSE;
        default:
            if (!(next = re_match_one(string_end, re_beg, _case))) return FALSE;
            string_end = next;
        }
        re_beg = re_end;
    }

    if (*re_end || *string_end) return FALSE;

found:
    *pre = re_end;
    *pstring = string_end;
    return TRUE;
}

/******************************************************************
 *		SymMatchStringA (DBGHELP.@)
 *
 */
BOOL WINAPI SymMatchStringA(PCSTR string, PCSTR re, BOOL _case)
{
    WCHAR*      strW;
    WCHAR*      reW;
    BOOL        ret = FALSE;
    DWORD       sz;

    if (!string || !re)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    TRACE("%s %s %c\n", string, re, _case ? 'Y' : 'N');

    sz = MultiByteToWideChar(CP_ACP, 0, string, -1, NULL, 0);
    if ((strW = HeapAlloc(GetProcessHeap(), 0, sz * sizeof(WCHAR))))
        MultiByteToWideChar(CP_ACP, 0, string, -1, strW, sz);
    sz = MultiByteToWideChar(CP_ACP, 0, re, -1, NULL, 0);
    if ((reW = HeapAlloc(GetProcessHeap(), 0, sz * sizeof(WCHAR))))
        MultiByteToWideChar(CP_ACP, 0, re, -1, reW, sz);

    if (strW && reW)
        ret = SymMatchStringW(strW, reW, _case);
    HeapFree(GetProcessHeap(), 0, strW);
    HeapFree(GetProcessHeap(), 0, reW);
    return ret;
}

/******************************************************************
 *		SymMatchStringW (DBGHELP.@)
 *
 */
BOOL WINAPI SymMatchStringW(PCWSTR string, PCWSTR re, BOOL _case)
{
    TRACE("%s %s %c\n", debugstr_w(string), debugstr_w(re), _case ? 'Y' : 'N');

    if (!string || !re)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    return re_match_multi(&string, &re, _case);
}

static inline BOOL doSymSearch(HANDLE hProcess, ULONG64 BaseOfDll, DWORD Index,
                               DWORD SymTag, PCWSTR Mask, DWORD64 Address,
                               PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
                               PVOID UserContext, DWORD Options)
{
    struct sym_enum     se;

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
 *		SymSearch (DBGHELP.@)
 */
BOOL WINAPI SymSearch(HANDLE hProcess, ULONG64 BaseOfDll, DWORD Index,
                      DWORD SymTag, PCSTR Mask, DWORD64 Address,
                      PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
                      PVOID UserContext, DWORD Options)
{
    LPWSTR      maskW = NULL;
    BOOLEAN     ret;

    TRACE("(%p %s %u %u %s %s %p %p %x)\n",
          hProcess, wine_dbgstr_longlong(BaseOfDll), Index, SymTag, Mask,
          wine_dbgstr_longlong(Address), EnumSymbolsCallback,
          UserContext, Options);

    if (Mask)
    {
        DWORD sz = MultiByteToWideChar(CP_ACP, 0, Mask, -1, NULL, 0);

        if (!(maskW = HeapAlloc(GetProcessHeap(), 0, sz * sizeof(WCHAR))))
            return FALSE;
        MultiByteToWideChar(CP_ACP, 0, Mask, -1, maskW, sz);
    }
    ret = doSymSearch(hProcess, BaseOfDll, Index, SymTag, maskW, Address,
                      EnumSymbolsCallback, UserContext, Options);
    HeapFree(GetProcessHeap(), 0, maskW);
    return ret;
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

    TRACE("(%p %s %u %u %s %s %p %p %x)\n",
          hProcess, wine_dbgstr_longlong(BaseOfDll), Index, SymTag, debugstr_w(Mask),
          wine_dbgstr_longlong(Address), EnumSymbolsCallback,
          UserContext, Options);

    sew.ctx = UserContext;
    sew.cb = EnumSymbolsCallback;
    sew.sym_info = (PSYMBOL_INFOW)sew.buffer;

    return doSymSearch(hProcess, BaseOfDll, Index, SymTag, Mask, Address,
                       sym_enumW, &sew, Options);
}

/******************************************************************
 *		SymAddSymbol (DBGHELP.@)
 *
 */
BOOL WINAPI SymAddSymbol(HANDLE hProcess, ULONG64 BaseOfDll, PCSTR name,
                         DWORD64 addr, DWORD size, DWORD flags)
{
    WCHAR       nameW[MAX_SYM_NAME];

    MultiByteToWideChar(CP_ACP, 0, name, -1, nameW, sizeof(nameW) / sizeof(WCHAR));
    return SymAddSymbolW(hProcess, BaseOfDll, nameW, addr, size, flags);
}

/******************************************************************
 *		SymAddSymbolW (DBGHELP.@)
 *
 */
BOOL WINAPI SymAddSymbolW(HANDLE hProcess, ULONG64 BaseOfDll, PCWSTR name,
                          DWORD64 addr, DWORD size, DWORD flags)
{
    struct module_pair  pair;

    TRACE("(%p %s %s %u)\n", hProcess, wine_dbgstr_w(name), wine_dbgstr_longlong(addr), size);

    pair.pcs = process_find_by_handle(hProcess);
    if (!pair.pcs) return FALSE;
    pair.requested = module_find_by_addr(pair.pcs, BaseOfDll, DMT_UNKNOWN);
    if (!module_get_debug(&pair)) return FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************
 *		SymSetScopeFromAddr (DBGHELP.@)
 */
BOOL WINAPI SymSetScopeFromAddr(HANDLE hProcess, ULONG64 addr)
{
    struct process*     pcs;

    FIXME("(%p %s): stub\n", hProcess, wine_dbgstr_longlong(addr));

    if (!(pcs = process_find_by_handle(hProcess))) return FALSE;
    return TRUE;
}

/******************************************************************
 *		SymEnumLines (DBGHELP.@)
 *
 */
BOOL WINAPI SymEnumLines(HANDLE hProcess, ULONG64 base, PCSTR compiland,
                         PCSTR srcfile, PSYM_ENUMLINES_CALLBACK cb, PVOID user)
{
    struct module_pair          pair;
    struct hash_table_iter      hti;
    struct symt_ht*             sym;
    WCHAR*                      srcmask;
    struct line_info*           dli;
    void*                       ptr;
    SRCCODEINFO                 sci;
    const char*                 file;

    if (!cb) return FALSE;
    if (!(dbghelp_options & SYMOPT_LOAD_LINES)) return TRUE;

    pair.pcs = process_find_by_handle(hProcess);
    if (!pair.pcs) return FALSE;
    if (compiland) FIXME("Unsupported yet (filtering on compiland %s)\n", compiland);
    pair.requested = module_find_by_addr(pair.pcs, base, DMT_UNKNOWN);
    if (!module_get_debug(&pair)) return FALSE;
    if (!(srcmask = file_regex(srcfile))) return FALSE;

    sci.SizeOfStruct = sizeof(sci);
    sci.ModBase      = base;

    hash_table_iter_init(&pair.effective->ht_symbols, &hti, NULL);
    while ((ptr = hash_table_iter_up(&hti)))
    {
        unsigned int    i;

        sym = GET_ENTRY(ptr, struct symt_ht, hash_elt);
        if (sym->symt.tag != SymTagFunction) continue;

        sci.FileName[0] = '\0';
        for (i=0; i<vector_length(&((struct symt_function*)sym)->vlines); i++)
        {
            dli = vector_at(&((struct symt_function*)sym)->vlines, i);
            if (dli->is_source_file)
            {
                file = source_get(pair.effective, dli->u.source_file);
                if (!file) sci.FileName[0] = '\0';
                else
                {
                    DWORD   sz = MultiByteToWideChar(CP_ACP, 0, file, -1, NULL, 0);
                    WCHAR*  fileW;

                    if ((fileW = HeapAlloc(GetProcessHeap(), 0, sz * sizeof(WCHAR))))
                        MultiByteToWideChar(CP_ACP, 0, file, -1, fileW, sz);
                    if (SymMatchStringW(fileW, srcmask, FALSE))
                        strcpy(sci.FileName, file);
                    else
                        sci.FileName[0] = '\0';
                    HeapFree(GetProcessHeap(), 0, fileW);
                }
            }
            else if (sci.FileName[0])
            {
                sci.Key = dli;
                sci.Obj[0] = '\0'; /* FIXME */
                sci.LineNumber = dli->line_number;
                sci.Address = dli->u.pc_offset;
                if (!cb(&sci, user)) break;
            }
        }
    }
    HeapFree(GetProcessHeap(), 0, srcmask);
    return TRUE;
}

BOOL WINAPI SymGetLineFromName(HANDLE hProcess, PCSTR ModuleName, PCSTR FileName,
                DWORD dwLineNumber, PLONG plDisplacement, PIMAGEHLP_LINE Line)
{
    FIXME("(%p) (%s, %s, %d %p %p): stub\n", hProcess, ModuleName, FileName,
                dwLineNumber, plDisplacement, Line);
    return FALSE;
}

BOOL WINAPI SymGetLineFromName64(HANDLE hProcess, PCSTR ModuleName, PCSTR FileName,
                DWORD dwLineNumber, PLONG lpDisplacement, PIMAGEHLP_LINE64 Line)
{
    FIXME("(%p) (%s, %s, %d %p %p): stub\n", hProcess, ModuleName, FileName,
                dwLineNumber, lpDisplacement, Line);
    return FALSE;
}

BOOL WINAPI SymGetLineFromNameW64(HANDLE hProcess, PCWSTR ModuleName, PCWSTR FileName,
                DWORD dwLineNumber, PLONG plDisplacement, PIMAGEHLP_LINEW64 Line)
{
    FIXME("(%p) (%s, %s, %d %p %p): stub\n", hProcess, debugstr_w(ModuleName), debugstr_w(FileName),
                dwLineNumber, plDisplacement, Line);
    return FALSE;
}

/******************************************************************
 *		SymFromIndex (DBGHELP.@)
 *
 */
BOOL WINAPI SymFromIndex(HANDLE hProcess, ULONG64 BaseOfDll, DWORD index, PSYMBOL_INFO symbol)
{
    FIXME("hProcess = %p, BaseOfDll = %s, index = %d, symbol = %p\n",
          hProcess, wine_dbgstr_longlong(BaseOfDll), index, symbol);

    return FALSE;
}

/******************************************************************
 *		SymFromIndexW (DBGHELP.@)
 *
 */
BOOL WINAPI SymFromIndexW(HANDLE hProcess, ULONG64 BaseOfDll, DWORD index, PSYMBOL_INFOW symbol)
{
    FIXME("hProcess = %p, BaseOfDll = %s, index = %d, symbol = %p\n",
          hProcess, wine_dbgstr_longlong(BaseOfDll), index, symbol);

    return FALSE;
}

/******************************************************************
 *		SymSetHomeDirectory (DBGHELP.@)
 *
 */
PCHAR WINAPI SymSetHomeDirectory(HANDLE hProcess, PCSTR dir)
{
    FIXME("(%p, %s): stub\n", hProcess, dir);

    return NULL;
}

/******************************************************************
 *		SymSetHomeDirectoryW (DBGHELP.@)
 *
 */
PWSTR WINAPI SymSetHomeDirectoryW(HANDLE hProcess, PCWSTR dir)
{
    FIXME("(%p, %s): stub\n", hProcess, debugstr_w(dir));

    return NULL;
}
