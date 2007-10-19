/*
 * File types.c - management of types (hierarchical tree)
 *
 * Copyright (C) 1997, Eric Youngdale.
 *               2004, Eric Pouech.
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
 *
 * Note: This really doesn't do much at the moment, but it forms the framework
 * upon which full support for datatype handling will eventually be built.
 */
#include "config.h"
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "wine/debug.h"
#include "dbghelp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);
WINE_DECLARE_DEBUG_CHANNEL(dbghelp_symt);

static const char* symt_get_tag_str(DWORD tag)
{
    switch (tag)
    {
    case SymTagNull:                    return "SymTagNull";
    case SymTagExe:                     return "SymTagExe";
    case SymTagCompiland:               return "SymTagCompiland";
    case SymTagCompilandDetails:        return "SymTagCompilandDetails";
    case SymTagCompilandEnv:            return "SymTagCompilandEnv";
    case SymTagFunction:                return "SymTagFunction";
    case SymTagBlock:                   return "SymTagBlock";
    case SymTagData:                    return "SymTagData";
    case SymTagAnnotation:              return "SymTagAnnotation";
    case SymTagLabel:                   return "SymTagLabel";
    case SymTagPublicSymbol:            return "SymTagPublicSymbol";
    case SymTagUDT:                     return "SymTagUDT";
    case SymTagEnum:                    return "SymTagEnum";
    case SymTagFunctionType:            return "SymTagFunctionType";
    case SymTagPointerType:             return "SymTagPointerType";
    case SymTagArrayType:               return "SymTagArrayType";
    case SymTagBaseType:                return "SymTagBaseType";
    case SymTagTypedef:                 return "SymTagTypedef,";
    case SymTagBaseClass:               return "SymTagBaseClass";
    case SymTagFriend:                  return "SymTagFriend";
    case SymTagFunctionArgType:         return "SymTagFunctionArgType,";
    case SymTagFuncDebugStart:          return "SymTagFuncDebugStart,";
    case SymTagFuncDebugEnd:            return "SymTagFuncDebugEnd";
    case SymTagUsingNamespace:          return "SymTagUsingNamespace,";
    case SymTagVTableShape:             return "SymTagVTableShape";
    case SymTagVTable:                  return "SymTagVTable";
    case SymTagCustom:                  return "SymTagCustom";
    case SymTagThunk:                   return "SymTagThunk";
    case SymTagCustomType:              return "SymTagCustomType";
    case SymTagManagedType:             return "SymTagManagedType";
    case SymTagDimension:               return "SymTagDimension";
    default:                            return "---";
    }
}

const char* symt_get_name(const struct symt* sym)
{
    switch (sym->tag)
    {
    /* lexical tree */
    case SymTagData:            return ((const struct symt_data*)sym)->hash_elt.name;
    case SymTagFunction:        return ((const struct symt_function*)sym)->hash_elt.name;
    case SymTagPublicSymbol:    return ((const struct symt_public*)sym)->hash_elt.name;
    case SymTagBaseType:        return ((const struct symt_basic*)sym)->hash_elt.name;
    case SymTagLabel:           return ((const struct symt_function_point*)sym)->name;
    case SymTagThunk:           return ((const struct symt_thunk*)sym)->hash_elt.name;
    /* hierarchy tree */
    case SymTagEnum:            return ((const struct symt_enum*)sym)->name;
    case SymTagTypedef:         return ((const struct symt_typedef*)sym)->hash_elt.name;
    case SymTagUDT:             return ((const struct symt_udt*)sym)->hash_elt.name;
    default:
        FIXME("Unsupported sym-tag %s\n", symt_get_tag_str(sym->tag));
        /* fall through */
    case SymTagArrayType:
    case SymTagPointerType:
    case SymTagFunctionType:
        return NULL;
    }
}

static struct symt* symt_find_type_by_name(struct module* module,
                                           enum SymTagEnum sym_tag,
                                           const char* typename)
{
    void*                       ptr;
    struct symt_ht*             type;
    struct hash_table_iter      hti;

    assert(typename);
    assert(module);

    hash_table_iter_init(&module->ht_types, &hti, typename);
    while ((ptr = hash_table_iter_up(&hti)))
    {
        type = GET_ENTRY(ptr, struct symt_ht, hash_elt);

        if ((sym_tag == SymTagNull || type->symt.tag == sym_tag) &&
            type->hash_elt.name && !strcmp(type->hash_elt.name, typename))
            return &type->symt;
    }
    SetLastError(ERROR_INVALID_NAME); /* FIXME ?? */
    return NULL;
}

static void symt_add_type(struct module* module, struct symt* symt)
{
    struct symt**       p;
    p = vector_add(&module->vtypes, &module->pool);
    assert(p);
    *p = symt;
}

struct symt_basic* symt_new_basic(struct module* module, enum BasicType bt,
                                  const char* typename, unsigned size)
{
    struct symt_basic*          sym;

    if (typename)
    {
        sym = (struct symt_basic*)symt_find_type_by_name(module, SymTagBaseType,
                                                         typename);
        if (sym && sym->bt == bt && sym->size == size)
            return sym;
    }
    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag = SymTagBaseType;
        if (typename)
        {
            sym->hash_elt.name = pool_strdup(&module->pool, typename);
            hash_table_add(&module->ht_types, &sym->hash_elt);
        } else sym->hash_elt.name = NULL;
        sym->bt = bt;
        sym->size = size;
        symt_add_type(module, &sym->symt);
    }
    return sym;
}

struct symt_udt* symt_new_udt(struct module* module, const char* typename,
                              unsigned size, enum UdtKind kind)
{
    struct symt_udt*            sym;

    TRACE_(dbghelp_symt)("Adding udt %s:%s\n", module->module.ModuleName, typename);
    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag = SymTagUDT;
        sym->kind     = kind;
        sym->size     = size;
        if (typename)
        {
            sym->hash_elt.name = pool_strdup(&module->pool, typename);
            hash_table_add(&module->ht_types, &sym->hash_elt);
        } else sym->hash_elt.name = NULL;
        vector_init(&sym->vchildren, sizeof(struct symt*), 8);
        symt_add_type(module, &sym->symt);
    }
    return sym;
}

BOOL symt_set_udt_size(struct module* module, struct symt_udt* udt, unsigned size)
{
    assert(udt->symt.tag == SymTagUDT);
    if (vector_length(&udt->vchildren) != 0)
    {
        if (udt->size != size)
            FIXME_(dbghelp_symt)("Changing size for %s from %u to %u\n",
                                 udt->hash_elt.name, udt->size, size);
        return TRUE;
    }
    udt->size = size;
    return TRUE;
}

/******************************************************************
 *		symt_add_udt_element
 *
 * add an element to a udt (struct, class, union)
 * the size & offset parameters are expressed in bits (not bytes) so that
 * we can mix in the single call bytes aligned elements (regular fields) and
 * the others (bit fields)
 */
BOOL symt_add_udt_element(struct module* module, struct symt_udt* udt_type,
                          const char* name, struct symt* elt_type,
                          unsigned offset, unsigned size)
{
    struct symt_data*   m;
    struct symt**       p;

    assert(udt_type->symt.tag == SymTagUDT);

    TRACE_(dbghelp_symt)("Adding %s to UDT %s\n", name, udt_type->hash_elt.name);
    p = NULL;
    while ((p = vector_iter_up(&udt_type->vchildren, p)))
    {
        m = (struct symt_data*)*p;
        assert(m);
        assert(m->symt.tag == SymTagData);
        if (m->hash_elt.name[0] == name[0] && strcmp(m->hash_elt.name, name) == 0)
            return TRUE;
    }

    if ((m = pool_alloc(&module->pool, sizeof(*m))) == NULL) return FALSE;
    memset(m, 0, sizeof(*m));
    m->symt.tag      = SymTagData;
    m->hash_elt.name = pool_strdup(&module->pool, name);
    m->hash_elt.next = NULL;

    m->kind          = DataIsMember;
    m->container     = &udt_type->symt;
    m->type          = elt_type;
    m->u.s.offset    = offset;
    m->u.s.length    = ((offset & 7) || (size & 7)) ? size : 0;
    m->u.s.reg_id    = 0;
    p = vector_add(&udt_type->vchildren, &module->pool);
    *p = &m->symt;

    return TRUE;
}

struct symt_enum* symt_new_enum(struct module* module, const char* typename)
{
    struct symt_enum*   sym;

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag            = SymTagEnum;
        sym->name = (typename) ? pool_strdup(&module->pool, typename) : NULL;
        vector_init(&sym->vchildren, sizeof(struct symt*), 8);
    }
    return sym;
}

BOOL symt_add_enum_element(struct module* module, struct symt_enum* enum_type,
                           const char* name, int value)
{
    struct symt_data*   e;
    struct symt**       p;

    assert(enum_type->symt.tag == SymTagEnum);
    e = pool_alloc(&module->pool, sizeof(*e));
    if (e == NULL) return FALSE;

    e->symt.tag = SymTagData;
    e->hash_elt.name = pool_strdup(&module->pool, name);
    e->hash_elt.next = NULL;
    e->kind = DataIsConstant;
    e->container = &enum_type->symt;
    /* CV defines the underlying type for the enumeration */
    e->type = &symt_new_basic(module, btInt, "int", 4)->symt;
    e->u.value.n1.n2.vt = VT_I4;
    e->u.value.n1.n2.n3.lVal = value;

    p = vector_add(&enum_type->vchildren, &module->pool);
    if (!p) return FALSE; /* FIXME we leak e */
    *p = &e->symt;

    return TRUE;
}

struct symt_array* symt_new_array(struct module* module, int min, int max,
                                  struct symt* base)
{
    struct symt_array*  sym;

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag  = SymTagArrayType;
        sym->start     = min;
        sym->end       = max;
        sym->basetype  = base;
        symt_add_type(module, &sym->symt);
    }
    return sym;
}

struct symt_function_signature* symt_new_function_signature(struct module* module,
                                                            struct symt* ret_type)
{
    struct symt_function_signature*     sym;

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag = SymTagFunctionType;
        sym->rettype  = ret_type;
        vector_init(&sym->vchildren, sizeof(struct symt*), 4);
        symt_add_type(module, &sym->symt);
    }
    return sym;
}

BOOL symt_add_function_signature_parameter(struct module* module,
                                           struct symt_function_signature* sig_type,
                                           struct symt* param)
{
    struct symt**       p;

    assert(sig_type->symt.tag == SymTagFunctionType);
    p = vector_add(&sig_type->vchildren, &module->pool);
    if (!p) return FALSE; /* FIXME we leak e */
    *p = param;

    return TRUE;
}

struct symt_pointer* symt_new_pointer(struct module* module, struct symt* ref_type)
{
    struct symt_pointer*        sym;

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag = SymTagPointerType;
        sym->pointsto = ref_type;
        symt_add_type(module, &sym->symt);
    }
    return sym;
}

struct symt_typedef* symt_new_typedef(struct module* module, struct symt* ref,
                                      const char* name)
{
    struct symt_typedef* sym;

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag = SymTagTypedef;
        sym->type     = ref;
        sym->hash_elt.name = pool_strdup(&module->pool, name);
        hash_table_add(&module->ht_types, &sym->hash_elt);
        symt_add_type(module, &sym->symt);
    }
    return sym;
}

/******************************************************************
 *		SymEnumTypes (DBGHELP.@)
 *
 */
BOOL WINAPI SymEnumTypes(HANDLE hProcess, ULONG64 BaseOfDll,
                         PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
                         PVOID UserContext)
{
    struct process*     pcs;
    struct module*      module;
    char                buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*        sym_info = (SYMBOL_INFO*)buffer;
    const char*         tmp;
    struct symt*        type;
    void*               pos = NULL;

    TRACE("(%p %s %p %p)\n",
          hProcess, wine_dbgstr_longlong(BaseOfDll), EnumSymbolsCallback,
          UserContext);

    if (!(pcs = process_find_by_handle(hProcess))) return FALSE;
    module = module_find_by_addr(pcs, BaseOfDll, DMT_UNKNOWN);
    if (!(module = module_get_debug(pcs, module))) return FALSE;

    sym_info->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym_info->MaxNameLen = sizeof(buffer) - sizeof(SYMBOL_INFO);

    while ((pos = vector_iter_up(&module->vtypes, pos)))
    {
        type = *(struct symt**)pos;
        sym_info->TypeIndex = (DWORD)type;
        sym_info->info = 0; /* FIXME */
        symt_get_info(type, TI_GET_LENGTH, &sym_info->Size);
        sym_info->ModBase = module->module.BaseOfImage;
        sym_info->Flags = 0; /* FIXME */
        sym_info->Value = 0; /* FIXME */
        sym_info->Address = 0; /* FIXME */
        sym_info->Register = 0; /* FIXME */
        sym_info->Scope = 0; /* FIXME */
        sym_info->Tag = type->tag;
        tmp = symt_get_name(type);
        if (tmp)
        {
            sym_info->NameLen = strlen(tmp) + 1;
            strncpy(sym_info->Name, tmp, min(sym_info->NameLen, sym_info->MaxNameLen));
            sym_info->Name[sym_info->MaxNameLen - 1] = '\0';
        }
        else sym_info->Name[sym_info->NameLen = 0] = '\0';
        if (!EnumSymbolsCallback(sym_info, sym_info->Size, UserContext)) break;
    }
    return TRUE;
}

/******************************************************************
 *		symt_get_info
 *
 * Retrieves inforamtion about a symt (either symbol or type)
 */
BOOL symt_get_info(const struct symt* type, IMAGEHLP_SYMBOL_TYPE_INFO req,
                   void* pInfo)
{
    unsigned            len;

    if (!type) return FALSE;

/* helper to typecast pInfo to its expected type (_t) */
#define X(_t) (*((_t*)pInfo))

    switch (req)
    {
    case TI_FINDCHILDREN:
        {
            const struct vector*        v;
            struct symt**               pt;
            unsigned                    i;
            TI_FINDCHILDREN_PARAMS*     tifp = pInfo;

            switch (type->tag)
            {
            case SymTagUDT:          v = &((const struct symt_udt*)type)->vchildren; break;
            case SymTagEnum:         v = &((const struct symt_enum*)type)->vchildren; break;
            case SymTagFunctionType: v = &((const struct symt_function_signature*)type)->vchildren; break;
            case SymTagFunction:     v = &((const struct symt_function*)type)->vchildren; break;
            default:
                FIXME("Unsupported sym-tag %s for find-children\n",
                      symt_get_tag_str(type->tag));
                return FALSE;
            }
            for (i = 0; i < tifp->Count; i++)
            {
                if (!(pt = vector_at(v, tifp->Start + i))) return FALSE;
                tifp->ChildId[i] = (DWORD)*pt;
            }
        }
        break;

    case TI_GET_ADDRESS:
        switch (type->tag)
        {
        case SymTagData:
            switch (((const struct symt_data*)type)->kind)
            {
            case DataIsGlobal:
            case DataIsFileStatic:
                X(ULONG64) = ((const struct symt_data*)type)->u.address;
                break;
            default: return FALSE;
            }
            break;
        case SymTagFunction:
            X(ULONG64) = ((const struct symt_function*)type)->address;
            break;
        case SymTagPublicSymbol:
            X(ULONG64) = ((const struct symt_public*)type)->address;
            break;
        case SymTagFuncDebugStart:
        case SymTagFuncDebugEnd:
        case SymTagLabel:
            X(ULONG64) = ((const struct symt_function_point*)type)->parent->address +
                ((const struct symt_function_point*)type)->offset;
            break;
        case SymTagThunk:
            X(ULONG64) = ((const struct symt_thunk*)type)->address;
            break;
        default:
            FIXME("Unsupported sym-tag %s for get-address\n",
                  symt_get_tag_str(type->tag));
            return FALSE;
        }
        break;

    case TI_GET_BASETYPE:
        switch (type->tag)
        {
        case SymTagBaseType:
            X(DWORD) = ((const struct symt_basic*)type)->bt;
            break;
        case SymTagEnum:
            X(DWORD) = btInt;
            break;
        default:
            return FALSE;
        }
        break;

    case TI_GET_BITPOSITION:
        if (type->tag != SymTagData ||
            ((const struct symt_data*)type)->kind != DataIsMember ||
            ((const struct symt_data*)type)->u.s.length == 0)
            return FALSE;
        X(DWORD) = ((const struct symt_data*)type)->u.s.offset & 7;
        break;

    case TI_GET_CHILDRENCOUNT:
        switch (type->tag)
        {
        case SymTagUDT:
            X(DWORD) = vector_length(&((const struct symt_udt*)type)->vchildren);
            break;
        case SymTagEnum:
            X(DWORD) = vector_length(&((const struct symt_enum*)type)->vchildren);
            break;
        case SymTagFunctionType:
            X(DWORD) = vector_length(&((const struct symt_function_signature*)type)->vchildren);
            break;
        case SymTagFunction:
            X(DWORD) = vector_length(&((const struct symt_function*)type)->vchildren);
            break;
        case SymTagPointerType: /* MS does it that way */
        case SymTagArrayType: /* MS does it that way */
        case SymTagThunk: /* MS does it that way */
            X(DWORD) = 0;
            break;
        default:
            FIXME("Unsupported sym-tag %s for get-children-count\n",
                  symt_get_tag_str(type->tag));
            /* fall through */
        case SymTagData:
        case SymTagPublicSymbol:
        case SymTagBaseType:
            return FALSE;
        }
        break;

    case TI_GET_COUNT:
        /* it seems that FunctionType also react to GET_COUNT (same value as
         * GET_CHILDREN_COUNT ?, except for C++ methods, where it seems to
         * also include 'this' (GET_CHILDREN_COUNT+1)
         */
        if (type->tag != SymTagArrayType) return FALSE;
        X(DWORD) = ((const struct symt_array*)type)->end -
            ((const struct symt_array*)type)->start + 1;
        break;

    case TI_GET_DATAKIND:
        if (type->tag != SymTagData) return FALSE;
        X(DWORD) = ((const struct symt_data*)type)->kind;
        break;

    case TI_GET_LENGTH:
        switch (type->tag)
        {
        case SymTagBaseType:
            X(DWORD) = ((const struct symt_basic*)type)->size;
            break;
        case SymTagFunction:
            X(DWORD) = ((const struct symt_function*)type)->size;
            break;
        case SymTagPointerType:
            X(DWORD) = sizeof(void*);
            break;
        case SymTagUDT:
            X(DWORD) = ((const struct symt_udt*)type)->size;
            break;
        case SymTagEnum:
            X(DWORD) = sizeof(int); /* FIXME: should be size of base-type of enum !!! */
            break;
        case SymTagData:
            if (((const struct symt_data*)type)->kind != DataIsMember ||
                !((const struct symt_data*)type)->u.s.length)
                return FALSE;
            X(DWORD) = ((const struct symt_data*)type)->u.s.length;
            break;
        case SymTagArrayType:
            if (!symt_get_info(((const struct symt_array*)type)->basetype,
                               TI_GET_LENGTH, pInfo))
                return FALSE;
            X(DWORD) *= ((const struct symt_array*)type)->end -
                ((const struct symt_array*)type)->start + 1;
            break;
        case SymTagPublicSymbol:
            X(DWORD) = ((const struct symt_public*)type)->size;
            break;
        case SymTagTypedef:
            return symt_get_info(((const struct symt_typedef*)type)->type, TI_GET_LENGTH, pInfo);
            break;
        case SymTagThunk:
            X(DWORD) = ((const struct symt_thunk*)type)->size;
            break;
        default:
            FIXME("Unsupported sym-tag %s for get-length\n",
                  symt_get_tag_str(type->tag));
            /* fall through */
        case SymTagFunctionType:
            return 0;
        }
        break;

    case TI_GET_LEXICALPARENT:
        switch (type->tag)
        {
        case SymTagBlock:
            X(DWORD) = (DWORD)((const struct symt_block*)type)->container;
            break;
        case SymTagData:
            X(DWORD) = (DWORD)((const struct symt_data*)type)->container;
            break;
        case SymTagFunction:
            X(DWORD) = (DWORD)((const struct symt_function*)type)->container;
            break;
        case SymTagThunk:
            X(DWORD) = (DWORD)((const struct symt_thunk*)type)->container;
            break;
        default:
            FIXME("Unsupported sym-tag %s for get-lexical-parent\n",
                  symt_get_tag_str(type->tag));
            return FALSE;
        }
        break;

    case TI_GET_NESTED:
        switch (type->tag)
        {
        case SymTagUDT:
        case SymTagEnum:
            X(DWORD) = 0;
            break;
        default:
            return FALSE;
        }
        break;

    case TI_GET_OFFSET:
        switch (type->tag)
        {
        case SymTagData:
            switch (((const struct symt_data*)type)->kind)
            {
            case DataIsParam:
            case DataIsLocal:
            case DataIsMember:
                X(ULONG) = ((const struct symt_data*)type)->u.s.offset >> 3;
                break;
            default:
                FIXME("Unknown kind (%u) for get-offset\n",
                      ((const struct symt_data*)type)->kind);
                return FALSE;
            }
            break;
        default:
            FIXME("Unsupported sym-tag %s for get-offset\n",
                  symt_get_tag_str(type->tag));
            return FALSE;
        }
        break;

    case TI_GET_SYMNAME:
        {
            const char* name = symt_get_name(type);
            if (!name) return FALSE;
            len = MultiByteToWideChar(CP_ACP, 0, name, -1, NULL, 0);
            X(WCHAR*) = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            if (!X(WCHAR*)) return FALSE;
            MultiByteToWideChar(CP_ACP, 0, name, -1, X(WCHAR*), len);
        }
        break;

    case TI_GET_SYMTAG:
        X(DWORD) = type->tag;
        break;

    case TI_GET_TYPE:
    case TI_GET_TYPEID:
        switch (type->tag)
        {
            /* hierarchical => hierarchical */
        case SymTagArrayType:
            X(DWORD) = (DWORD)((const struct symt_array*)type)->basetype;
            break;
        case SymTagPointerType:
            X(DWORD) = (DWORD)((const struct symt_pointer*)type)->pointsto;
            break;
        case SymTagFunctionType:
            X(DWORD) = (DWORD)((const struct symt_function_signature*)type)->rettype;
            break;
        case SymTagTypedef:
            X(DWORD) = (DWORD)((const struct symt_typedef*)type)->type;
            break;
            /* lexical => hierarchical */
        case SymTagData:
            X(DWORD) = (DWORD)((const struct symt_data*)type)->type;
            break;
        case SymTagFunction:
            X(DWORD) = (DWORD)((const struct symt_function*)type)->type;
            break;
            /* FIXME: should also work for enums and FunctionArgType */
        default:
            FIXME("Unsupported sym-tag %s for get-type\n",
                  symt_get_tag_str(type->tag));
        case SymTagThunk:
            return FALSE;
        }
        break;

    case TI_GET_UDTKIND:
        if (type->tag != SymTagUDT) return FALSE;
        X(DWORD) = ((const struct symt_udt*)type)->kind;
        break;

    case TI_GET_VALUE:
        if (type->tag != SymTagData || ((const struct symt_data*)type)->kind != DataIsConstant)
            return FALSE;
        X(VARIANT) = ((const struct symt_data*)type)->u.value;
        break;

#undef X

    case TI_GET_ADDRESSOFFSET:
    case TI_GET_ARRAYINDEXTYPEID:
    case TI_GET_CALLING_CONVENTION:
    case TI_GET_CLASSPARENTID:
    case TI_GET_SYMINDEX:
    case TI_GET_THISADJUST:
    case TI_GET_VIRTUALBASECLASS:
    case TI_GET_VIRTUALBASEPOINTEROFFSET:
    case TI_GET_VIRTUALTABLESHAPEID:
    case TI_IS_EQUIV_TO:
        FIXME("Unsupported GetInfo request (%u)\n", req);
        return FALSE;
    }

    return TRUE;
}

/******************************************************************
 *		SymGetTypeInfo (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetTypeInfo(HANDLE hProcess, DWORD64 ModBase,
                           ULONG TypeId, IMAGEHLP_SYMBOL_TYPE_INFO GetType,
                           PVOID pInfo)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    struct module*      module;

    if (!pcs) return FALSE;

    module = module_find_by_addr(pcs, ModBase, DMT_UNKNOWN);
    if (!(module = module_get_debug(pcs, module)))
    {
        FIXME("Someone didn't properly set ModBase (%s)\n", wine_dbgstr_longlong(ModBase));
        return FALSE;
    }

    return symt_get_info((struct symt*)TypeId, GetType, pInfo);
}

/******************************************************************
 *		SymGetTypeFromName (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetTypeFromName(HANDLE hProcess, ULONG64 BaseOfDll,
                               LPSTR Name, PSYMBOL_INFO Symbol)
{
    struct process*     pcs = process_find_by_handle(hProcess);
    struct module*      module;
    struct symt*        type;

    if (!pcs) return FALSE;
    module = module_find_by_addr(pcs, BaseOfDll, DMT_UNKNOWN);
    if (!module) return FALSE;
    type = symt_find_type_by_name(module, SymTagNull, Name);
    if (!type) return FALSE;
    Symbol->TypeIndex = (DWORD)type;

    return TRUE;
}
