/*
 * File msc.c - read VC++ debug information from COFF and eventually
 * from PDB files.
 *
 * Copyright (C) 1996,      Eric Youngdale.
 * Copyright (C) 1999-2000, Ulrich Weigand.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
/*
 * Note - this handles reading debug information for 32 bit applications
 * that run under Windows-NT for example.  I doubt that this would work well
 * for 16 bit applications, but I don't think it really matters since the
 * file format is different, and we should never get in here in such cases.
 *
 * TODO:
 *	Get 16 bit CV stuff working.
 *	Add symbol size to internal symbol table.
 */
 
#include "config.h"
#include "wine/port.h"
 
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
 
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
 
#include <pseh/pseh.h>
 
#include "wine/debug.h"
#include "dbghelp_private.h"
#include "mscvpdb.h"
 
WINE_DEFAULT_DEBUG_CHANNEL(dbghelp_msc);
 
#define MAX_PATHNAME_LEN 1024
 
/*========================================================================
 * Debug file access helper routines
 */
 
static _SEH_FILTER(page_fault)
{
    if (_SEH_GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        return _SEH_EXECUTE_HANDLER;
    return _SEH_CONTINUE_SEARCH;
}
 
static void dump(const void* ptr, unsigned len)
{
    int         i, j;
    BYTE        msg[128];
    const char* hexof = "0123456789abcdef";
    const BYTE* x = (const BYTE*)ptr;
 
    for (i = 0; i < len; i += 16)
    {
        sprintf(msg, "%08x: ", i);
        memset(msg + 10, ' ', 3 * 16 + 1 + 16);
        for (j = 0; j < min(16, len - i); j++)
        {
            msg[10 + 3 * j + 0] = hexof[x[i + j] >> 4];
            msg[10 + 3 * j + 1] = hexof[x[i + j] & 15];
            msg[10 + 3 * j + 2] = ' ';
            msg[10 + 3 * 16 + 1 + j] = (x[i + j] >= 0x20 && x[i + j] < 0x7f) ?
                x[i + j] : '.';
        }
        msg[10 + 3 * 16] = ' ';
        msg[10 + 3 * 16 + 1 + 16] = '\0';
        FIXME("%s\n", msg);
    }
}
 
/*========================================================================
 * Process CodeView type information.
 */
 
#define MAX_BUILTIN_TYPES	0x0480
#define FIRST_DEFINABLE_TYPE    0x1000
 
static struct symt*     cv_basic_types[MAX_BUILTIN_TYPES];
 
#define SymTagCVBitField        (SymTagMax + 0x100)
struct codeview_bitfield
{
    struct symt         symt;
    unsigned            subtype;
    unsigned            bitposition;
    unsigned            bitlength;
};
 
struct cv_defined_module
{
    BOOL                allowed;
    unsigned int        num_defined_types;
    struct symt**       defined_types;
 
    struct codeview_bitfield* bitfields;
    unsigned            num_bitfields;
    unsigned            used_bitfields;
};
/* FIXME: don't make it static */
#define CV_MAX_MODULES          32
static struct cv_defined_module cv_zmodules[CV_MAX_MODULES];
static struct cv_defined_module*cv_current_module;
 
static void codeview_init_basic_types(struct module* module)
{
    /*
     * These are the common builtin types that are used by VC++.
     */
    cv_basic_types[T_NOTYPE] = NULL;
    cv_basic_types[T_ABS]    = NULL;
    cv_basic_types[T_VOID]   = &symt_new_basic(module, btVoid,  "void", 0)->symt;
    cv_basic_types[T_CHAR]   = &symt_new_basic(module, btChar,  "char", 1)->symt;
    cv_basic_types[T_SHORT]  = &symt_new_basic(module, btInt,   "short int", 2)->symt;
    cv_basic_types[T_LONG]   = &symt_new_basic(module, btInt,   "long int", 4)->symt;
    cv_basic_types[T_QUAD]   = &symt_new_basic(module, btInt,   "long long int", 8)->symt;
    cv_basic_types[T_UCHAR]  = &symt_new_basic(module, btUInt,  "unsigned char", 1)->symt;
    cv_basic_types[T_USHORT] = &symt_new_basic(module, btUInt,  "unsigned short", 2)->symt;
    cv_basic_types[T_ULONG]  = &symt_new_basic(module, btUInt,  "unsigned long", 4)->symt;
    cv_basic_types[T_UQUAD]  = &symt_new_basic(module, btUInt,  "unsigned long long", 8)->symt;
    cv_basic_types[T_REAL32] = &symt_new_basic(module, btFloat, "float", 4)->symt;
    cv_basic_types[T_REAL64] = &symt_new_basic(module, btFloat, "double", 8)->symt;
    cv_basic_types[T_RCHAR]  = &symt_new_basic(module, btInt,   "signed char", 1)->symt;
    cv_basic_types[T_WCHAR]  = &symt_new_basic(module, btWChar, "wchar_t", 2)->symt;
    cv_basic_types[T_INT4]   = &symt_new_basic(module, btInt,   "INT4", 4)->symt;
    cv_basic_types[T_UINT4]  = &symt_new_basic(module, btUInt,  "UINT4", 4)->symt;
 
    cv_basic_types[T_32PVOID]   = &symt_new_pointer(module, cv_basic_types[T_VOID])->symt;
    cv_basic_types[T_32PCHAR]   = &symt_new_pointer(module, cv_basic_types[T_CHAR])->symt;
    cv_basic_types[T_32PSHORT]  = &symt_new_pointer(module, cv_basic_types[T_SHORT])->symt;
    cv_basic_types[T_32PLONG]   = &symt_new_pointer(module, cv_basic_types[T_LONG])->symt;
    cv_basic_types[T_32PQUAD]   = &symt_new_pointer(module, cv_basic_types[T_QUAD])->symt;
    cv_basic_types[T_32PUCHAR]  = &symt_new_pointer(module, cv_basic_types[T_UCHAR])->symt;
    cv_basic_types[T_32PUSHORT] = &symt_new_pointer(module, cv_basic_types[T_USHORT])->symt;
    cv_basic_types[T_32PULONG]  = &symt_new_pointer(module, cv_basic_types[T_ULONG])->symt;
    cv_basic_types[T_32PUQUAD]  = &symt_new_pointer(module, cv_basic_types[T_UQUAD])->symt;
    cv_basic_types[T_32PREAL32] = &symt_new_pointer(module, cv_basic_types[T_REAL32])->symt;
    cv_basic_types[T_32PREAL64] = &symt_new_pointer(module, cv_basic_types[T_REAL64])->symt;
    cv_basic_types[T_32PRCHAR]  = &symt_new_pointer(module, cv_basic_types[T_RCHAR])->symt;
    cv_basic_types[T_32PWCHAR]  = &symt_new_pointer(module, cv_basic_types[T_WCHAR])->symt;
    cv_basic_types[T_32PINT4]   = &symt_new_pointer(module, cv_basic_types[T_INT4])->symt;
    cv_basic_types[T_32PUINT4]  = &symt_new_pointer(module, cv_basic_types[T_UINT4])->symt;
}
 
static int numeric_leaf(int* value, const unsigned short int* leaf)
{
    unsigned short int type = *leaf++;
    int length = 2;
 
    if (type < LF_NUMERIC)
    {
        *value = type;
    }
    else
    {
        switch (type)
        {
        case LF_CHAR:
            length += 1;
            *value = *(const char*)leaf;
            break;
 
        case LF_SHORT:
            length += 2;
            *value = *(const short*)leaf;
            break;
 
        case LF_USHORT:
            length += 2;
            *value = *(const unsigned short*)leaf;
            break;
 
        case LF_LONG:
            length += 4;
            *value = *(const int*)leaf;
            break;
 
        case LF_ULONG:
            length += 4;
            *value = *(const unsigned int*)leaf;
            break;
 
        case LF_QUADWORD:
        case LF_UQUADWORD:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 8;
            *value = 0;    /* FIXME */
            break;
 
        case LF_REAL32:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 4;
            *value = 0;    /* FIXME */
            break;
 
        case LF_REAL48:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 6;
            *value = 0;    /* FIXME */
            break;
 
        case LF_REAL64:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 8;
            *value = 0;    /* FIXME */
            break;
 
        case LF_REAL80:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 10;
            *value = 0;    /* FIXME */
            break;
 
        case LF_REAL128:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 16;
            *value = 0;    /* FIXME */
            break;
 
        case LF_COMPLEX32:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 4;
            *value = 0;    /* FIXME */
            break;
 
        case LF_COMPLEX64:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 8;
            *value = 0;    /* FIXME */
            break;
 
        case LF_COMPLEX80:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 10;
            *value = 0;    /* FIXME */
            break;
 
        case LF_COMPLEX128:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 16;
            *value = 0;    /* FIXME */
            break;
 
        case LF_VARSTRING:
	    FIXME("Unsupported numeric leaf type %04x\n", type);
            length += 2 + *leaf;
            *value = 0;    /* FIXME */
            break;
 
        default:
	    FIXME("Unknown numeric leaf type %04x\n", type);
            *value = 0;
            break;
        }
    }
 
    return length;
}
 
/* convert a pascal string (as stored in debug information) into
 * a C string (null terminated).
 */
static const char* terminate_string(const struct p_string* p_name)
{
    static char symname[256];
 
    memcpy(symname, p_name->name, p_name->namelen);
    symname[p_name->namelen] = '\0';
 
    return (!*symname || strcmp(symname, "__unnamed") == 0) ? NULL : symname;
}
 
static struct symt*  codeview_get_type(unsigned int typeno, BOOL allow_special)
{
    struct symt*        symt = NULL;
 
    /*
     * Convert Codeview type numbers into something we can grok internally.
     * Numbers < FIRST_DEFINABLE_TYPE are all fixed builtin types.
     * Numbers from FIRST_DEFINABLE_TYPE and up are all user defined (structs, etc).
     */
    if (typeno < FIRST_DEFINABLE_TYPE)
    {
        if (typeno < MAX_BUILTIN_TYPES)
	    symt = cv_basic_types[typeno];
    }
    else
    {
        unsigned        mod_index = typeno >> 24;
        unsigned        mod_typeno = typeno & 0x00FFFFFF;
        struct cv_defined_module*       mod;
 
        mod = (mod_index == 0) ? cv_current_module : &cv_zmodules[mod_index];
 
        if (mod_index >= CV_MAX_MODULES || !mod->allowed) 
            FIXME("Module of index %d isn't loaded yet (%x)\n", mod_index, typeno);
        else
        {
            if (mod_typeno - FIRST_DEFINABLE_TYPE < mod->num_defined_types)
                symt = mod->defined_types[mod_typeno - FIRST_DEFINABLE_TYPE];
        }
    }
    if (!allow_special && symt && symt->tag == SymTagCVBitField)
        FIXME("bitfields are only handled for UDTs\n");
    if (!symt && typeno) FIXME("Returning NULL symt for type-id %x\n", typeno);
    return symt;
}
 
static int codeview_add_type(unsigned int typeno, struct symt* dt)
{
    if (typeno < FIRST_DEFINABLE_TYPE)
        FIXME("What the heck\n");
    if (!cv_current_module)
    {
        FIXME("Adding %x to non allowed module\n", typeno);
        return FALSE;
    }
    if ((typeno >> 24) != 0)
        FIXME("No module index while inserting type-id assumption is wrong %x\n",
              typeno);
    while (typeno - FIRST_DEFINABLE_TYPE >= cv_current_module->num_defined_types)
    {
        cv_current_module->num_defined_types += 0x100;
        if (cv_current_module->defined_types)
            cv_current_module->defined_types = (struct symt**)
                HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
                            cv_current_module->defined_types,
                            cv_current_module->num_defined_types * sizeof(struct symt*));
        else
            cv_current_module->defined_types = (struct symt**)
                HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                          cv_current_module->num_defined_types * sizeof(struct symt*));
 
        if (cv_current_module->defined_types == NULL) return FALSE;
    }
 
    cv_current_module->defined_types[typeno - FIRST_DEFINABLE_TYPE] = dt;
    return TRUE;
}
 
static void codeview_clear_type_table(void)
{
    int i;
 
    for (i = 0; i < CV_MAX_MODULES; i++)
    {
        if (cv_zmodules[i].allowed && cv_zmodules[i].defined_types)
            HeapFree(GetProcessHeap(), 0, cv_zmodules[i].defined_types);
        cv_zmodules[i].allowed = FALSE;
        cv_zmodules[i].defined_types = NULL;
        cv_zmodules[i].num_defined_types = 0;
        if (cv_zmodules[i].bitfields)
            HeapFree(GetProcessHeap(), 0, cv_zmodules[i].bitfields);
        cv_zmodules[i].bitfields = NULL;
        cv_zmodules[i].num_bitfields = cv_zmodules[i].used_bitfields = 0;
    }
    cv_current_module = NULL;
}
 
static int codeview_add_type_pointer(struct module* module, unsigned int typeno, 
                                     unsigned int datatype)
{
    struct symt* symt = &symt_new_pointer(module,
                                          codeview_get_type(datatype, FALSE))->symt;
    return codeview_add_type(typeno, symt);
}
 
static int codeview_add_type_array(struct module* module, 
                                   unsigned int typeno, const char* name,
                                   unsigned int elemtype, unsigned int arr_len)
{
    struct symt*        symt;
    struct symt*        elem = codeview_get_type(elemtype, FALSE);
    DWORD               arr_max = 0;
 
    if (elem)
    {
        DWORD elem_size;
        symt_get_info(elem, TI_GET_LENGTH, &elem_size);
        if (elem_size) arr_max = arr_len / elem_size;
    }
    symt = &symt_new_array(module, 0, arr_max, elem)->symt;
    return codeview_add_type(typeno, symt);
}
 
static int codeview_add_type_bitfield(unsigned int typeno, unsigned int bitoff,
                                      unsigned int nbits, unsigned int basetype)
{
    if (cv_current_module->used_bitfields >= cv_current_module->num_bitfields)
    {
        if (cv_current_module->bitfields)
        {
            cv_current_module->num_bitfields *= 2;
            cv_current_module->bitfields = 
                HeapReAlloc(GetProcessHeap(), 0, 
                            cv_current_module->bitfields, 
                            cv_current_module->num_bitfields * sizeof(struct codeview_bitfield));
        }
        else
        {
            cv_current_module->num_bitfields = 64;
            cv_current_module->bitfields = 
                HeapAlloc(GetProcessHeap(), 0, 
                          cv_current_module->num_bitfields * sizeof(struct codeview_bitfield));
        }
        if (!cv_current_module->bitfields) return 0;
    }
 
    cv_current_module->bitfields[cv_current_module->used_bitfields].symt.tag    = SymTagCVBitField;
    cv_current_module->bitfields[cv_current_module->used_bitfields].subtype     = basetype;
    cv_current_module->bitfields[cv_current_module->used_bitfields].bitposition = bitoff;
    cv_current_module->bitfields[cv_current_module->used_bitfields].bitlength   = nbits;
 
    return codeview_add_type(typeno, &cv_current_module->bitfields[cv_current_module->used_bitfields++].symt);
}
 
static int codeview_add_type_enum_field_list(struct module* module, 
                                             unsigned int typeno, 
                                             const unsigned char* list, int len)
{
    struct symt_enum*           symt;
    const unsigned char*        ptr = list;
 
    symt = symt_new_enum(module, NULL);
    while (ptr - list < len)
    {
        const union codeview_fieldtype* type = (const union codeview_fieldtype*)ptr;
 
        if (*ptr >= 0xf0)       /* LF_PAD... */
        {
            ptr += *ptr & 0x0f;
            continue;
        }
 
        switch (type->generic.id)
        {
        case LF_ENUMERATE_V1:
        {
            int value, vlen = numeric_leaf(&value, &type->enumerate_v1.value);
            const struct p_string* p_name = (const struct p_string*)((const unsigned char*)&type->enumerate_v1.value + vlen);
 
            symt_add_enum_element(module, symt, terminate_string(p_name), value);
            ptr += 2 + 2 + vlen + (1 + p_name->namelen);
            break;
        }
        case LF_ENUMERATE_V3:
        {
            int value, vlen = numeric_leaf(&value, &type->enumerate_v3.value);
            const char* name = (const char*)&type->enumerate_v3.value + vlen;
 
            symt_add_enum_element(module, symt, name, value);
            ptr += 2 + 2 + vlen + (1 + strlen(name));
            break;
        }
 
        default:
            FIXME("Unsupported type %04x in ENUM field list\n", type->generic.id);
            return FALSE;
        }
    }
 
    return codeview_add_type(typeno, &symt->symt);
}
 
static int codeview_add_type_struct_field_list(struct module* module, 
                                               unsigned int typeno, 
                                               const unsigned char* list, int len)
{
    struct symt_udt*            symt;
    const unsigned char*        ptr = list;
    int                         value, leaf_len, vpoff, vplen;
    const struct p_string*      p_name;
    const char*                 c_name;
    struct symt*                subtype;
    const unsigned short int* p_vboff;
 
    symt = symt_new_udt(module, NULL, 0, UdtStruct /* don't care */);
    while (ptr - list < len)
    {
        const union codeview_fieldtype* type = (const union codeview_fieldtype*)ptr;
 
        if (*ptr >= 0xf0)       /* LF_PAD... */
        {
            ptr +=* ptr & 0x0f;
            continue;
        }
 
        switch (type->generic.id)
        {
        case LF_BCLASS_V1:
            leaf_len = numeric_leaf(&value, &type->bclass_v1.offset);
 
            /* FIXME: ignored for now */
 
            ptr += 2 + 2 + 2 + leaf_len;
            break;
 
        case LF_BCLASS_V2:
            leaf_len = numeric_leaf(&value, &type->bclass_v2.offset);
 
            /* FIXME: ignored for now */
 
            ptr += 2 + 2 + 4 + leaf_len;
            break;
 
        case LF_VBCLASS_V1:
        case LF_IVBCLASS_V1:
            {
                leaf_len = numeric_leaf(&value, &type->vbclass_v1.vbpoff);
                p_vboff = (const unsigned short int*)((const char*)&type->vbclass_v1.vbpoff + leaf_len);
                vplen = numeric_leaf(&vpoff, p_vboff);
 
                /* FIXME: ignored for now */
 
                ptr += 2 + 2 + 2 + 2 + leaf_len + vplen;
            }
            break;
 
        case LF_VBCLASS_V2:
        case LF_IVBCLASS_V2:
            {
                leaf_len = numeric_leaf(&value, &type->vbclass_v2.vbpoff);
                p_vboff = (const unsigned short int*)((const char*)&type->vbclass_v2.vbpoff + leaf_len);
                vplen = numeric_leaf(&vpoff, p_vboff);
 
                /* FIXME: ignored for now */
 
                ptr += 2 + 2 + 4 + 4 + leaf_len + vplen;
            }
            break;
 
        case LF_MEMBER_V1:
            leaf_len = numeric_leaf(&value, &type->member_v1.offset);
            p_name = (const struct p_string*)((const char*)&type->member_v1.offset + leaf_len);
            subtype = codeview_get_type(type->member_v1.type, TRUE);
 
            if (!subtype || subtype->tag != SymTagCVBitField)
            {
                DWORD elem_size = 0;
                if (subtype) symt_get_info(subtype, TI_GET_LENGTH, &elem_size);
                symt_add_udt_element(module, symt, terminate_string(p_name),
                                     codeview_get_type(type->member_v1.type, TRUE),
                                     value << 3, elem_size << 3);
            }
            else
            {
                struct codeview_bitfield* cvbf = (struct codeview_bitfield*)subtype;
                symt_add_udt_element(module, symt, terminate_string(p_name),
                                     codeview_get_type(cvbf->subtype, FALSE),
                                     cvbf->bitposition, cvbf->bitlength);
            }
 
            ptr += 2 + 2 + 2 + leaf_len + (1 + p_name->namelen);
            break;
 
        case LF_MEMBER_V2:
            leaf_len = numeric_leaf(&value, &type->member_v2.offset);
            p_name = (const struct p_string*)((const unsigned char*)&type->member_v2.offset + leaf_len);
            subtype = codeview_get_type(type->member_v2.type, TRUE);
 
            if (!subtype || subtype->tag != SymTagCVBitField)
            {
                DWORD elem_size = 0;
                if (subtype) symt_get_info(subtype, TI_GET_LENGTH, &elem_size);
                symt_add_udt_element(module, symt, terminate_string(p_name),
                                     subtype, value << 3, elem_size << 3);
            }
            else
            {
                struct codeview_bitfield* cvbf = (struct codeview_bitfield*)subtype;
                symt_add_udt_element(module, symt, terminate_string(p_name),
                                     codeview_get_type(cvbf->subtype, FALSE),
                                     cvbf->bitposition, cvbf->bitlength);
            }
 
            ptr += 2 + 2 + 4 + leaf_len + (1 + p_name->namelen);
            break;
 
        case LF_MEMBER_V3:
            leaf_len = numeric_leaf(&value, &type->member_v3.offset);
            c_name = (const char*)&type->member_v3.offset + leaf_len;
            subtype = codeview_get_type(type->member_v3.type, TRUE);
 
            if (!subtype || subtype->tag != SymTagCVBitField)
            {
                DWORD elem_size = 0;
                if (subtype) symt_get_info(subtype, TI_GET_LENGTH, &elem_size);
                symt_add_udt_element(module, symt, c_name,
                                     subtype, value << 3, elem_size << 3);
            }
            else
            {
                struct codeview_bitfield* cvbf = (struct codeview_bitfield*)subtype;
                symt_add_udt_element(module, symt, c_name,
                                     codeview_get_type(cvbf->subtype, FALSE),
                                     cvbf->bitposition, cvbf->bitlength);
            }
 
            ptr += 2 + 2 + 4 + leaf_len + (strlen(c_name) + 1);
            break;
 
        case LF_STMEMBER_V1:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + 2 + (1 + type->stmember_v1.p_name.namelen);
            break;
 
        case LF_STMEMBER_V2:
            /* FIXME: ignored for now */
            ptr += 2 + 4 + 2 + (1 + type->stmember_v2.p_name.namelen);
            break;
 
        case LF_METHOD_V1:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + 2 + (1 + type->method_v1.p_name.namelen);
            break;
 
        case LF_METHOD_V2:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + 4 + (1 + type->method_v2.p_name.namelen);
            break;
 
        case LF_NESTTYPE_V1:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + (1 + type->nesttype_v1.p_name.namelen);
            break;
 
        case LF_NESTTYPE_V2:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + 4 + (1 + type->nesttype_v2.p_name.namelen);
            break;
 
        case LF_VFUNCTAB_V1:
            /* FIXME: ignored for now */
            ptr += 2 + 2;
            break;
 
        case LF_VFUNCTAB_V2:
            /* FIXME: ignored for now */
            ptr += 2 + 2 + 4;
            break;
 
        case LF_ONEMETHOD_V1:
            /* FIXME: ignored for now */
            switch ((type->onemethod_v1.attribute >> 2) & 7)
            {
            case 4: case 6: /* (pure) introducing virtual method */
                ptr += 2 + 2 + 2 + 4 + (1 + type->onemethod_virt_v1.p_name.namelen);
                break;
 
            default:
                ptr += 2 + 2 + 2 + (1 + type->onemethod_v1.p_name.namelen);
                break;
            }
            break;
 
        case LF_ONEMETHOD_V2:
            /* FIXME: ignored for now */
            switch ((type->onemethod_v2.attribute >> 2) & 7)
            {
            case 4: case 6: /* (pure) introducing virtual method */
                ptr += 2 + 2 + 4 + 4 + (1 + type->onemethod_virt_v2.p_name.namelen);
                break;
 
            default:
                ptr += 2 + 2 + 4 + (1 + type->onemethod_v2.p_name.namelen);
                break;
            }
            break;
 
        default:
            FIXME("Unsupported type %04x in STRUCT field list\n", type->generic.id);
            return FALSE;
        }
    }
 
    return codeview_add_type(typeno, &symt->symt);
}
 
static int codeview_add_type_enum(struct module* module, unsigned int typeno, 
                                  const char* name, unsigned int fieldlist)
{
    struct symt_enum*   symt = symt_new_enum(module, name);
    struct symt*        list = codeview_get_type(fieldlist, FALSE);
 
    /* FIXME: this is rather ugly !!! */
    if (list) symt->vchildren = ((struct symt_enum*)list)->vchildren;
 
    return codeview_add_type(typeno, &symt->symt);
}
 
static int codeview_add_type_struct(struct module* module, unsigned int typeno, 
                                    const char* name, int structlen, 
                                    unsigned int fieldlist, enum UdtKind kind)
{
    struct symt_udt*    symt = symt_new_udt(module, name, structlen, kind);
    struct symt*        list = codeview_get_type(fieldlist, FALSE);
 
    /* FIXME: this is rather ugly !!! */
    if (list) symt->vchildren = ((struct symt_udt*)list)->vchildren;
 
    return codeview_add_type(typeno, &symt->symt);
}
 
static int codeview_new_func_signature(struct module* module, unsigned typeno,
                                           unsigned ret_type)
{
    struct symt* symt;
    symt = &symt_new_function_signature(module, 
                                        codeview_get_type(ret_type, FALSE))->symt;
    return codeview_add_type(typeno, symt);
}
 
static int codeview_parse_type_table(struct module* module, const char* table,
                                     int len)
{
    unsigned int                curr_type = 0x1000;
    const char*                 ptr = table;
    int                         retv;
    const union codeview_type*  type;
    int                         value, leaf_len;
    const struct p_string*      p_name;
    const char*                 c_name;
 
    while (ptr - table < len)
    {
        retv = TRUE;
        type = (const union codeview_type*)ptr;
 
        switch (type->generic.id)
        {
        case LF_MODIFIER_V1:
            /* FIXME: we don't handle modifiers, 
             * but readd previous type on the curr_type 
             */
            WARN("Modifier on %x: %s%s%s%s\n",
                 type->modifier_v1.type,
                 type->modifier_v1.attribute & 0x01 ? "const " : "",
                 type->modifier_v1.attribute & 0x02 ? "volatile " : "",
                 type->modifier_v1.attribute & 0x04 ? "unaligned " : "",
                 type->modifier_v1.attribute & ~0x07 ? "unknown " : "");
            codeview_add_type(curr_type, 
                              codeview_get_type(type->modifier_v1.type, FALSE));
            break;
        case LF_MODIFIER_V2:
            /* FIXME: we don't handle modifiers, but readd previous type on the curr_type */
            WARN("Modifier on %x: %s%s%s%s\n",
                 type->modifier_v2.type,
                 type->modifier_v2.attribute & 0x01 ? "const " : "",
                 type->modifier_v2.attribute & 0x02 ? "volatile " : "",
                 type->modifier_v2.attribute & 0x04 ? "unaligned " : "",
                 type->modifier_v2.attribute & ~0x07 ? "unknown " : "");
            codeview_add_type(curr_type, 
                              codeview_get_type(type->modifier_v2.type, FALSE));
            break;
 
        case LF_POINTER_V1:
            retv = codeview_add_type_pointer(module, curr_type, 
                                             type->pointer_v1.datatype);
            break;
        case LF_POINTER_V2:
            retv = codeview_add_type_pointer(module, curr_type, 
                                             type->pointer_v2.datatype);
            break;
 
        case LF_ARRAY_V1:
            leaf_len = numeric_leaf(&value, &type->array_v1.arrlen);
            p_name = (const struct p_string*)((const unsigned char*)&type->array_v1.arrlen + leaf_len);
 
            retv = codeview_add_type_array(module, curr_type, terminate_string(p_name),
                                           type->array_v1.elemtype, value);
            break;
        case LF_ARRAY_V2:
            leaf_len = numeric_leaf(&value, &type->array_v2.arrlen);
            p_name = (const struct p_string*)((const unsigned char*)&type->array_v2.arrlen + leaf_len);
 
            retv = codeview_add_type_array(module, curr_type, terminate_string(p_name),
                                           type->array_v2.elemtype, value);
            break;
        case LF_ARRAY_V3:
            leaf_len = numeric_leaf(&value, &type->array_v3.arrlen);
            c_name = (const char*)&type->array_v3.arrlen + leaf_len;
 
            retv = codeview_add_type_array(module, curr_type, c_name,
                                           type->array_v3.elemtype, value);
            break;
 
        case LF_BITFIELD_V1:
            /* a bitfield is a CodeView specific data type which represent a bitfield
             * in a structure or a class. For now, we store it in a SymTag-like type
             * (so that the rest of the process is seamless), but check at udt 
             * inclusion type for its presence
             */
            retv = codeview_add_type_bitfield(curr_type, type->bitfield_v1.bitoff,
                                              type->bitfield_v1.nbits,
                                              type->bitfield_v1.type);
            break;
        case LF_BITFIELD_V2:
            retv = codeview_add_type_bitfield(curr_type, type->bitfield_v2.bitoff,
                                              type->bitfield_v2.nbits,
                                              type->bitfield_v2.type);
            break;
        case LF_FIELDLIST_V1:
        case LF_FIELDLIST_V2:
           {
               /*
                * A 'field list' is a CodeView-specific data type which doesn't
                * directly correspond to any high-level data type.  It is used
                * to hold the collection of members of a struct, class, union
                * or enum type.  The actual definition of that type will follow
                * later, and refer to the field list definition record.
                *
                * As we don't have a field list type ourselves, we look ahead
                * in the field list to try to find out whether this field list
                * will be used for an enum or struct type, and create a dummy
                * type of the corresponding sort.  Later on, the definition of
                * the 'real' type will copy the member / enumeration data.
                */
               const char* list = type->fieldlist.list;
               int   len  = (ptr + type->generic.len + 2) - list;
 
               if (((const union codeview_fieldtype*)list)->generic.id == LF_ENUMERATE_V1 ||
                   ((const union codeview_fieldtype*)list)->generic.id == LF_ENUMERATE_V3)
                   retv = codeview_add_type_enum_field_list(module, curr_type, list, len);
               else
                   retv = codeview_add_type_struct_field_list(module, curr_type, list, len);
           }
           break;
 
        case LF_STRUCTURE_V1:
        case LF_CLASS_V1:
            leaf_len = numeric_leaf(&value, &type->struct_v1.structlen);
            p_name = (const struct p_string*)((const unsigned char*)&type->struct_v1.structlen + leaf_len);
 
            retv = codeview_add_type_struct(module, curr_type, terminate_string(p_name),
                                            value, type->struct_v1.fieldlist,
                                            type->generic.id == LF_CLASS_V1 ? UdtClass : UdtStruct);
            break;
 
        case LF_STRUCTURE_V2:
        case LF_CLASS_V2:
            leaf_len = numeric_leaf(&value, &type->struct_v2.structlen);
            p_name = (const struct p_string*)((const unsigned char*)&type->struct_v2.structlen + leaf_len);
 
            retv = codeview_add_type_struct(module, curr_type, terminate_string(p_name),
                                            value, type->struct_v2.fieldlist,
                                            type->generic.id == LF_CLASS_V2 ? UdtClass : UdtStruct);
            break;
 
        case LF_STRUCTURE_V3:
        case LF_CLASS_V3:
            leaf_len = numeric_leaf(&value, &type->struct_v3.structlen);
            c_name = (const char*)&type->struct_v3.structlen + leaf_len;
 
            retv = codeview_add_type_struct(module, curr_type, c_name,
                                            value, type->struct_v3.fieldlist,
                                            type->generic.id == LF_CLASS_V3 ? UdtClass : UdtStruct);
            break;
 
        case LF_UNION_V1:
            leaf_len = numeric_leaf(&value, &type->union_v1.un_len);
            p_name = (const struct p_string*)((const unsigned char*)&type->union_v1.un_len + leaf_len);
 
            retv = codeview_add_type_struct(module, curr_type, terminate_string(p_name),
                                            value, type->union_v1.fieldlist, UdtUnion);
            break;
        case LF_UNION_V2:
            leaf_len = numeric_leaf(&value, &type->union_v2.un_len);
            p_name = (const struct p_string*)((const unsigned char*)&type->union_v2.un_len + leaf_len);
 
            retv = codeview_add_type_struct(module, curr_type, terminate_string(p_name),
                                            value, type->union_v2.fieldlist, UdtUnion);
            break;
        case LF_UNION_V3:
            leaf_len = numeric_leaf(&value, &type->union_v3.un_len);
            c_name = (const char*)&type->union_v3.un_len + leaf_len;
 
            retv = codeview_add_type_struct(module, curr_type, c_name,
                                            value, type->union_v3.fieldlist, UdtUnion);
 
        case LF_ENUM_V1:
            retv = codeview_add_type_enum(module, curr_type, terminate_string(&type->enumeration_v1.p_name),
                                          type->enumeration_v1.field);
            break;
 
        case LF_ENUM_V2:
            retv = codeview_add_type_enum(module, curr_type, terminate_string(&type->enumeration_v2.p_name),
                                          type->enumeration_v2.field);
            break;
        case LF_ENUM_V3:
            retv = codeview_add_type_enum(module, curr_type, type->enumeration_v3.name,
                                          type->enumeration_v3.field);
            break;
        case LF_PROCEDURE_V1:
            retv = codeview_new_func_signature(module, curr_type, 
                                               type->procedure_v1.rvtype);
            break;
        case LF_PROCEDURE_V2:
            retv = codeview_new_func_signature(module, curr_type, 
                                               type->procedure_v2.rvtype);
            break;
        case LF_MFUNCTION_V1:
            /* FIXME: for C++, this is plain wrong, but as we don't use arg types
             * nor class information, this would just do for now
             */
            retv = codeview_new_func_signature(module, curr_type,
                                               type->mfunction_v1.rvtype);
            break;
        case LF_MFUNCTION_V2:
            /* FIXME: for C++, this is plain wrong, but as we don't use arg types
             * nor class information, this would just do for now
             */
            retv = codeview_new_func_signature(module, curr_type,
                                               type->mfunction_v2.rvtype);
            break;
        case LF_ARGLIST_V1:
        case LF_ARGLIST_V2:
            {
                static int once;
                if (!once++) 
                    FIXME("Not adding parameters' types to function signature\n");
            }
            break;
 
        default:
            FIXME("Unsupported type-id leaf %x\n", type->generic.id);
            dump(type, 2 + type->generic.len);
            break;
        }
        if (!retv) return FALSE;
        curr_type++;
        ptr += type->generic.len + 2;
    }
 
    return TRUE;
}
 
/*========================================================================
 * Process CodeView line number information.
 */
 
static struct codeview_linetab* codeview_snarf_linetab(struct module* module, 
                                                       const char* linetab, int size,
                                                       BOOL pascal_str)
{
    int				file_segcount;
    char			filename[PATH_MAX];
    const unsigned int*         filetab;
    const struct p_string*      p_fn;
    int				i;
    int				k;
    struct codeview_linetab*    lt_hdr;
    const unsigned int*         lt_ptr;
    int				nfile;
    int				nseg;
    union any_size		pnt;
    union any_size		pnt2;
    const struct startend*      start;
    int				this_seg;
    struct symt_compiland*      compiland;
 
    /*
     * Now get the important bits.
     */
    pnt.c = linetab;
    nfile = *pnt.s++;
    nseg = *pnt.s++;
 
    filetab = (const unsigned int*) pnt.c;
 
    /*
     * Now count up the number of segments in the file.
     */
    nseg = 0;
    for (i = 0; i < nfile; i++)
    {
        pnt2.c = linetab + filetab[i];
        nseg += *pnt2.s;
    }
 
    /*
     * Next allocate the header we will be returning.
     * There is one header for each segment, so that we can reach in
     * and pull bits as required.
     */
    lt_hdr = (struct codeview_linetab*)
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (nseg + 1) * sizeof(*lt_hdr));
    if (lt_hdr == NULL)
    {
        goto leave;
    }
 
    /*
     * Now fill the header we will be returning, one for each segment.
     * Note that this will basically just contain pointers into the existing
     * line table, and we do not actually copy any additional information
     * or allocate any additional memory.
     */
 
    this_seg = 0;
    for (i = 0; i < nfile; i++)
    {
        /*
         * Get the pointer into the segment information.
         */
        pnt2.c = linetab + filetab[i];
        file_segcount = *pnt2.s;
 
        pnt2.ui++;
        lt_ptr = (const unsigned int*) pnt2.c;
        start = (const struct startend*)(lt_ptr + file_segcount);
 
        /*
         * Now snarf the filename for all of the segments for this file.
         */
        if (pascal_str)
        {
            p_fn = (const struct p_string*)(start + file_segcount);
            memset(filename, 0, sizeof(filename));
            memcpy(filename, p_fn->name, p_fn->namelen);
            compiland = symt_new_compiland(module, filename);
        }
        else
            compiland = symt_new_compiland(module, (const char*)(start + file_segcount));
 
        for (k = 0; k < file_segcount; k++, this_seg++)
	{
            pnt2.c = linetab + lt_ptr[k];
            lt_hdr[this_seg].start      = start[k].start;
            lt_hdr[this_seg].end        = start[k].end;
            lt_hdr[this_seg].compiland  = compiland;
            lt_hdr[this_seg].segno      = *pnt2.s++;
            lt_hdr[this_seg].nline      = *pnt2.s++;
            lt_hdr[this_seg].offtab     = pnt2.ui;
            lt_hdr[this_seg].linetab    = (const unsigned short*)(pnt2.ui + lt_hdr[this_seg].nline);
	}
    }
 
leave:
 
  return lt_hdr;
 
}
 
/*========================================================================
 * Process CodeView symbol information.
 */
 
static unsigned int codeview_map_offset(const struct msc_debug_info* msc_dbg,
                                        unsigned int offset)
{
    int                 nomap = msc_dbg->nomap;
    const OMAP_DATA*    omapp = msc_dbg->omapp;
    int                 i;
 
    if (!nomap || !omapp) return offset;
 
    /* FIXME: use binary search */
    for (i = 0; i < nomap - 1; i++)
        if (omapp[i].from <= offset && omapp[i+1].from > offset)
            return !omapp[i].to ? 0 : omapp[i].to + (offset - omapp[i].from);
 
    return 0;
}
 
static const struct codeview_linetab*
codeview_get_linetab(const struct codeview_linetab* linetab,
                     unsigned seg, unsigned offset)
{
    /*
     * Check whether we have line number information
     */
    if (linetab)
    {
        for (; linetab->linetab; linetab++)
            if (linetab->segno == seg &&
                linetab->start <= offset && linetab->end   >  offset)
                break;
        if (!linetab->linetab) linetab = NULL;
    }
    return linetab;
}
 
static unsigned codeview_get_address(const struct msc_debug_info* msc_dbg, 
                                     unsigned seg, unsigned offset)
{
    int			        nsect = msc_dbg->nsect;
    const IMAGE_SECTION_HEADER* sectp = msc_dbg->sectp;
 
    if (!seg || seg > nsect) return 0;
    return msc_dbg->module->module.BaseOfImage +
        codeview_map_offset(msc_dbg, sectp[seg-1].VirtualAddress + offset);
}
 
static void codeview_add_func_linenum(struct module* module, 
                                      struct symt_function* func,
                                      const struct codeview_linetab* linetab,
                                      unsigned offset, unsigned size)
{
    unsigned int        i;
 
    if (!linetab) return;
    for (i = 0; i < linetab->nline; i++)
    {
        if (linetab->offtab[i] >= offset && linetab->offtab[i] < offset + size)
        {
            symt_add_func_line(module, func, linetab->compiland->source,
                               linetab->linetab[i], linetab->offtab[i] - offset);
        }
    }
}
 
static int codeview_snarf(const struct msc_debug_info* msc_dbg, const BYTE* root, 
                          int offset, int size,
                          struct codeview_linetab* linetab)
{
    struct symt_function*               curr_func = NULL;
    int                                 i, length;
    const struct codeview_linetab*      flt;
    struct symt_block*                  block = NULL;
    struct symt*                        symt;
    const char*                         name;
 
    /*
     * Loop over the different types of records and whenever we
     * find something we are interested in, record it and move on.
     */
    for (i = offset; i < size; i += length)
    {
        const union codeview_symbol* sym = (const union codeview_symbol*)(root + i);
        length = sym->generic.len + 2;
        if (length & 3) FIXME("unpadded len %u\n", length + 2);
 
        switch (sym->generic.id)
        {
        /*
         * Global and local data symbols.  We don't associate these
         * with any given source file.
         */
	case S_GDATA_V1:
	case S_LDATA_V1:
            flt = codeview_get_linetab(linetab, sym->data_v1.segment, sym->data_v1.offset);
            symt_new_global_variable(msc_dbg->module, 
                                     flt ? flt->compiland : NULL,
                                     terminate_string(&sym->data_v1.p_name), sym->generic.id == S_LDATA_V1,
                                     codeview_get_address(msc_dbg, sym->data_v1.segment, sym->data_v1.offset),
                                     0,
                                     codeview_get_type(sym->data_v1.symtype, FALSE));
	    break;
	case S_GDATA_V2:
	case S_LDATA_V2:
            flt = codeview_get_linetab(linetab, sym->data_v2.segment, sym->data_v2.offset);
            name = terminate_string(&sym->data_v2.p_name);
            if (name)
                symt_new_global_variable(msc_dbg->module, flt ? flt->compiland : NULL,
                                         name, sym->generic.id == S_LDATA_V2,
                                         codeview_get_address(msc_dbg, sym->data_v2.segment, sym->data_v2.offset),
                                         0,
                                         codeview_get_type(sym->data_v2.symtype, FALSE));
	    break;
	case S_GDATA_V3:
	case S_LDATA_V3:
            flt = codeview_get_linetab(linetab, sym->data_v3.segment, sym->data_v3.offset);
            if (*sym->data_v3.name)
                symt_new_global_variable(msc_dbg->module, flt ? flt->compiland : NULL,
                                         sym->data_v3.name,
                                         sym->generic.id == S_LDATA_V3,
                                         codeview_get_address(msc_dbg, sym->data_v3.segment, sym->data_v3.offset),
                                         0,
                                         codeview_get_type(sym->data_v3.symtype, FALSE));
	    break;
 
	case S_PUB_V1: /* FIXME is this really a 'data_v1' structure ?? */
            if (!(dbghelp_options & SYMOPT_NO_PUBLICS))
            {
                flt = codeview_get_linetab(linetab, sym->data_v1.segment, sym->data_v1.offset);
                symt_new_public(msc_dbg->module, flt ? flt->compiland : NULL,
                                terminate_string(&sym->data_v1.p_name), 
                                codeview_get_address(msc_dbg, sym->data_v1.segment, sym->data_v1.offset),
                                0, TRUE /* FIXME */, TRUE /* FIXME */);
            }
            break;
	case S_PUB_V2: /* FIXME is this really a 'data_v2' structure ?? */
            if (!(dbghelp_options & SYMOPT_NO_PUBLICS))
            {
                flt = codeview_get_linetab(linetab, sym->data_v2.segment, sym->data_v2.offset);
                symt_new_public(msc_dbg->module, flt ? flt->compiland : NULL,
                                terminate_string(&sym->data_v2.p_name), 
                                codeview_get_address(msc_dbg, sym->data_v2.segment, sym->data_v2.offset),
                                0, TRUE /* FIXME */, TRUE /* FIXME */);
            }
	    break;
 
        /*
         * Sort of like a global function, but it just points
         * to a thunk, which is a stupid name for what amounts to
         * a PLT slot in the normal jargon that everyone else uses.
         */
	case S_THUNK_V1:
            flt = codeview_get_linetab(linetab, sym->thunk_v1.segment, sym->thunk_v1.offset);
            symt_new_thunk(msc_dbg->module, flt ? flt->compiland : NULL,
                           terminate_string(&sym->thunk_v1.p_name), sym->thunk_v1.thtype,
                           codeview_get_address(msc_dbg, sym->thunk_v1.segment, sym->thunk_v1.offset),
                           sym->thunk_v1.thunk_len);
	    break;
	case S_THUNK_V3:
            flt = codeview_get_linetab(linetab, sym->thunk_v3.segment, sym->thunk_v3.offset);
            symt_new_thunk(msc_dbg->module, flt ? flt->compiland : NULL,
                           sym->thunk_v3.name, sym->thunk_v3.thtype,
                           codeview_get_address(msc_dbg, sym->thunk_v3.segment, sym->thunk_v3.offset),
                           sym->thunk_v3.thunk_len);
	    break;
 
        /*
         * Global and static functions.
         */
	case S_GPROC_V1:
	case S_LPROC_V1:
            flt = codeview_get_linetab(linetab, sym->proc_v1.segment, sym->proc_v1.offset);
            if (curr_func) FIXME("nested function\n");
            curr_func = symt_new_function(msc_dbg->module,
                                          flt ? flt->compiland : NULL,
                                          terminate_string(&sym->proc_v1.p_name),
                                          codeview_get_address(msc_dbg, sym->proc_v1.segment, sym->proc_v1.offset),
                                          sym->proc_v1.proc_len,
                                          codeview_get_type(sym->proc_v1.proctype, FALSE));
            codeview_add_func_linenum(msc_dbg->module, curr_func, flt, 
                                      sym->proc_v1.offset, sym->proc_v1.proc_len);
            symt_add_function_point(msc_dbg->module, curr_func, SymTagFuncDebugStart, sym->proc_v1.debug_start, NULL);
            symt_add_function_point(msc_dbg->module, curr_func, SymTagFuncDebugEnd, sym->proc_v1.debug_end, NULL);
	    break;
	case S_GPROC_V2:
	case S_LPROC_V2:
            flt = codeview_get_linetab(linetab, sym->proc_v2.segment, sym->proc_v2.offset);
            if (curr_func) FIXME("nested function\n");
            curr_func = symt_new_function(msc_dbg->module, 
                                          flt ? flt->compiland : NULL,
                                          terminate_string(&sym->proc_v2.p_name),
                                          codeview_get_address(msc_dbg, sym->proc_v2.segment, sym->proc_v2.offset),
                                          sym->proc_v2.proc_len,
                                          codeview_get_type(sym->proc_v2.proctype, FALSE));
            codeview_add_func_linenum(msc_dbg->module, curr_func, flt, 
                                      sym->proc_v2.offset, sym->proc_v2.proc_len);
            symt_add_function_point(msc_dbg->module, curr_func, SymTagFuncDebugStart, sym->proc_v2.debug_start, NULL);
            symt_add_function_point(msc_dbg->module, curr_func, SymTagFuncDebugEnd, sym->proc_v2.debug_end, NULL);
	    break;
	case S_GPROC_V3:
	case S_LPROC_V3:
            flt = codeview_get_linetab(linetab, sym->proc_v3.segment, sym->proc_v3.offset);
            if (curr_func) FIXME("nested function\n");
            curr_func = symt_new_function(msc_dbg->module, 
                                          flt ? flt->compiland : NULL,
                                          sym->proc_v3.name,
                                          codeview_get_address(msc_dbg, sym->proc_v3.segment, sym->proc_v3.offset),
                                          sym->proc_v3.proc_len,
                                          codeview_get_type(sym->proc_v3.proctype, FALSE));
            codeview_add_func_linenum(msc_dbg->module, curr_func, flt, 
                                      sym->proc_v3.offset, sym->proc_v3.proc_len);
            symt_add_function_point(msc_dbg->module, curr_func, SymTagFuncDebugStart, sym->proc_v3.debug_start, NULL);
            symt_add_function_point(msc_dbg->module, curr_func, SymTagFuncDebugEnd, sym->proc_v3.debug_end, NULL);
	    break;
        /*
         * Function parameters and stack variables.
         */
	case S_BPREL_V1:
            symt_add_func_local(msc_dbg->module, curr_func, 0, sym->stack_v1.offset,
                                block, codeview_get_type(sym->stack_v1.symtype, FALSE),
                                terminate_string(&sym->stack_v1.p_name));
            break;
	case S_BPREL_V2:
            symt_add_func_local(msc_dbg->module, curr_func, 0, sym->stack_v2.offset,
                                block, codeview_get_type(sym->stack_v2.symtype, FALSE),
                                terminate_string(&sym->stack_v2.p_name));
            break;
	case S_BPREL_V3:
            symt_add_func_local(msc_dbg->module, curr_func, 0, sym->stack_v3.offset,
                                block, codeview_get_type(sym->stack_v3.symtype, FALSE),
                                sym->stack_v3.name);
            break;
 
        case S_REGISTER_V1:
            symt_add_func_local(msc_dbg->module, curr_func, 0, sym->register_v1.reg,
                                block, codeview_get_type(sym->register_v1.type, FALSE),
                                terminate_string(&sym->register_v1.p_name));
            break;
        case S_REGISTER_V2:
            symt_add_func_local(msc_dbg->module, curr_func, 0, sym->register_v2.reg,
                                block, codeview_get_type(sym->register_v2.type, FALSE),
                                terminate_string(&sym->register_v2.p_name));
            break;
 
        case S_BLOCK_V1:
            block = symt_open_func_block(msc_dbg->module, curr_func, block, 
                                         codeview_get_address(msc_dbg, sym->block_v1.segment, sym->block_v1.offset),
                                         sym->block_v1.length);
            break;
        case S_BLOCK_V3:
            block = symt_open_func_block(msc_dbg->module, curr_func, block, 
                                         codeview_get_address(msc_dbg, sym->block_v3.segment, sym->block_v3.offset),
                                         sym->block_v3.length);
            break;
 
        case S_END_V1:
            if (block)
            {
                block = symt_close_func_block(msc_dbg->module, curr_func, block, 0);
            }
            else if (curr_func)
            {
                symt_normalize_function(msc_dbg->module, curr_func);
                curr_func = NULL;
            }
            break;
 
        /* FIXME: we should use this as a compiland, instead of guessing it on the fly */
        case S_COMPILAND_V1:
            TRACE("S-Compiland-V1e %x %s\n", 
                  sym->compiland_v1.unknown, 
                  terminate_string(&sym->compiland_v1.p_name));
            break;
 
        case S_COMPILAND_V2:
            TRACE("S-Compiland-V2 %s\n", 
                  terminate_string(&sym->compiland_v2.p_name));
            if (TRACE_ON(dbghelp_msc))
            {
                const char* ptr1 = sym->compiland_v2.p_name.name + sym->compiland_v2.p_name.namelen;
                const char* ptr2;
                while (*ptr1)
                {
                    ptr2 = ptr1 + strlen(ptr1) + 1;
                    TRACE("\t%s => %s\n", ptr1, ptr2); 
                    ptr1 = ptr2 + strlen(ptr2) + 1;
                }
            }
            break;
        case S_COMPILAND_V3:
            TRACE("S-Compiland-V3 %s\n", sym->compiland_v3.name);
            if (TRACE_ON(dbghelp_msc))
            {
                const char* ptr1 = sym->compiland_v3.name + strlen(sym->compiland_v3.name);
                const char* ptr2;
                while (*ptr1)
                {
                    ptr2 = ptr1 + strlen(ptr1) + 1;
                    TRACE("\t%s => %s\n", ptr1, ptr2); 
                    ptr1 = ptr2 + strlen(ptr2) + 1;
                }
            }
            break;
 
        case S_OBJNAME_V1:
            TRACE("S-ObjName %.*s\n", ((const BYTE*)sym)[8], (const BYTE*)sym + 9);
            break;
 
        case S_LABEL_V1:
            if (curr_func)
            {
                symt_add_function_point(msc_dbg->module, curr_func, SymTagLabel, 
                                        codeview_get_address(msc_dbg, sym->label_v1.segment, sym->label_v1.offset) - curr_func->address,
                                        terminate_string(&sym->label_v1.p_name));
            }
            else
                FIXME("No current function for label %s\n",
                      terminate_string(&sym->label_v1.p_name));
            break;
        case S_LABEL_V3:
            if (curr_func)
            {
                symt_add_function_point(msc_dbg->module, curr_func, SymTagLabel, 
                                        codeview_get_address(msc_dbg, sym->label_v3.segment, sym->label_v3.offset) - curr_func->address,
                                        sym->label_v3.name);
            }
            else
                FIXME("No current function for label %s\n", sym->label_v3.name);
            break;
 
        case S_CONSTANT_V1:
            {
                int                     val, vlen;
                const struct p_string*  name;
                const char*             x;
                struct symt*            se;
 
                vlen = numeric_leaf(&val, &sym->constant_v1.cvalue);
                name = (const struct p_string*)((const char*)&sym->constant_v1.cvalue + vlen);
                se = codeview_get_type(sym->constant_v1.type, FALSE);
                if (!se) x = "---";
                else if (se->tag == SymTagEnum) x = ((struct symt_enum*)se)->name;
                else x = "###";
 
                TRACE("S-Constant-V1 %u %s %x (%s)\n", 
                      val, terminate_string(name), sym->constant_v1.type, x);
                /* FIXME: we should add this as a constant value */
            }
            break;
        case S_CONSTANT_V2:
            {
                int                     val, vlen;
                const struct p_string*  name;
                const char*             x;
                struct symt*            se;
 
                vlen = numeric_leaf(&val, &sym->constant_v2.cvalue);
                name = (const struct p_string*)((const char*)&sym->constant_v2.cvalue + vlen);
                se = codeview_get_type(sym->constant_v2.type, FALSE);
                if (!se) x = "---";
                else if (se->tag == SymTagEnum) x = ((struct symt_enum*)se)->name;
                else x = "###";
 
                TRACE("S-Constant-V2 %u %s %x (%s)\n", 
                      val, terminate_string(name), sym->constant_v2.type, x);
                /* FIXME: we should add this as a constant value */
            }
            break;
        case S_CONSTANT_V3:
            {
                int                     val, vlen;
                const char*             name;
                const char*             x;
                struct symt*            se;
 
                vlen = numeric_leaf(&val, &sym->constant_v3.cvalue);
                name = (const char*)&sym->constant_v3.cvalue + vlen;
                se = codeview_get_type(sym->constant_v3.type, FALSE);
                if (!se) x = "---";
                else if (se->tag == SymTagEnum) x = ((struct symt_enum*)se)->name;
                else x = "###";
 
                TRACE("S-Constant-V3 %u %s %x (%s)\n", 
                      val, name, sym->constant_v3.type, x);
                /* FIXME: we should add this as a constant value */
            }
            break;
 
        case S_UDT_V1:
            if (sym->udt_v1.type)
            {
                if ((symt = codeview_get_type(sym->udt_v1.type, FALSE)))
                    symt_new_typedef(msc_dbg->module, symt, 
                                     terminate_string(&sym->udt_v1.p_name));
                else
                    FIXME("S-Udt %s: couldn't find type 0x%x\n", 
                          terminate_string(&sym->udt_v1.p_name), sym->udt_v1.type);
            }
            break;
        case S_UDT_V2:
            if (sym->udt_v2.type)
            {
                if ((symt = codeview_get_type(sym->udt_v2.type, FALSE)))
                    symt_new_typedef(msc_dbg->module, symt, 
                                     terminate_string(&sym->udt_v2.p_name));
                else
                    FIXME("S-Udt %s: couldn't find type 0x%x\n", 
                          terminate_string(&sym->udt_v2.p_name), sym->udt_v2.type);
            }
            break;
        case S_UDT_V3:
            if (sym->udt_v3.type)
            {
                if ((symt = codeview_get_type(sym->udt_v3.type, FALSE)))
                    symt_new_typedef(msc_dbg->module, symt, sym->udt_v3.name);
                else
                    FIXME("S-Udt %s: couldn't find type 0x%x\n", 
                          sym->udt_v3.name, sym->udt_v3.type);
            }
            break;
 
         /*
         * These are special, in that they are always followed by an
         * additional length-prefixed string which is *not* included
         * into the symbol length count.  We need to skip it.
         */
	case S_PROCREF_V1:
	case S_DATAREF_V1:
	case S_LPROCREF_V1:
            name = (const char*)sym + length;
            length += (*name + 1 + 3) & ~3;
            break;
 
        case S_PUB_DATA_V3:
            if (!(dbghelp_options & SYMOPT_NO_PUBLICS))
            {
                flt = codeview_get_linetab(linetab, sym->data_v3.segment, sym->data_v3.offset);
                symt_new_public(msc_dbg->module, 
                                flt ? flt->compiland : NULL,
                                sym->data_v3.name, 
                                codeview_get_address(msc_dbg, sym->data_v3.segment, sym->data_v3.offset),
                                0, FALSE /* FIXME */, FALSE);
            }
            break;
        case S_PUB_FUNC1_V3:
        case S_PUB_FUNC2_V3: /* using a data_v3 isn't what we'd expect */
            if (!(dbghelp_options & SYMOPT_NO_PUBLICS))
            {
                flt = codeview_get_linetab(linetab, sym->data_v3.segment, sym->data_v3.offset);
                symt_new_public(msc_dbg->module, 
                                flt ? flt->compiland : NULL,
                                sym->data_v3.name, 
                                codeview_get_address(msc_dbg, sym->data_v3.segment, sym->data_v3.offset),
                                0, TRUE /* FIXME */, TRUE);
            }
            break;
 
        case S_MSTOOL_V3: /* just to silence a few warnings */
            break;
 
        default:
            FIXME("Unsupported symbol id %x\n", sym->generic.id);
            dump(sym, 2 + sym->generic.len);
            break;
        }
    }
 
    if (curr_func) symt_normalize_function(msc_dbg->module, curr_func);
 
    if (linetab) HeapFree(GetProcessHeap(), 0, linetab);
    return TRUE;
}
 
/*========================================================================
 * Process PDB file.
 */
 
struct pdb_lookup
{
    const char*                 filename;
    enum {PDB_JG, PDB_DS}       kind;
    union
    {
        struct
        {
            DWORD               timestamp;
            struct PDB_JG_TOC*  toc;
        } jg;
        struct
        {
            GUID                guid;
            struct PDB_DS_TOC*  toc;
        } ds;
    } u;
};
 
static void* pdb_jg_read(const struct PDB_JG_HEADER* pdb, const WORD* block_list,
                         int size)
{
    int                         i, num_blocks;
    BYTE*                       buffer;
 
    if (!size) return NULL;
 
    num_blocks = (size + pdb->block_size - 1) / pdb->block_size;
    buffer = HeapAlloc(GetProcessHeap(), 0, num_blocks * pdb->block_size);
 
    for (i = 0; i < num_blocks; i++)
        memcpy(buffer + i * pdb->block_size,
               (const char*)pdb + block_list[i] * pdb->block_size, pdb->block_size);
 
    return buffer;
}
 
static void* pdb_ds_read(const struct PDB_DS_HEADER* pdb, const DWORD* block_list,
                         int size)
{
    int                         i, num_blocks;
    BYTE*                       buffer;
 
    if (!size) return NULL;
 
    num_blocks = (size + pdb->block_size - 1) / pdb->block_size;
    buffer = HeapAlloc(GetProcessHeap(), 0, num_blocks * pdb->block_size);
 
    for (i = 0; i < num_blocks; i++)
        memcpy(buffer + i * pdb->block_size,
               (const char*)pdb + block_list[i] * pdb->block_size, pdb->block_size);
 
    return buffer;
}
 
static void* pdb_read_jg_file(const struct PDB_JG_HEADER* pdb,
                              const struct PDB_JG_TOC* toc, DWORD file_nr)
{
    const WORD*                 block_list;
    DWORD                       i;
 
    if (!toc || file_nr >= toc->num_files) return NULL;
 
    block_list = (const WORD*) &toc->file[toc->num_files];
    for (i = 0; i < file_nr; i++)
        block_list += (toc->file[i].size + pdb->block_size - 1) / pdb->block_size;
 
    return pdb_jg_read(pdb, block_list, toc->file[file_nr].size);
}
 
static void* pdb_read_ds_file(const struct PDB_DS_HEADER* pdb,
                              const struct PDB_DS_TOC* toc, DWORD file_nr)
{
    const DWORD*                block_list;
    DWORD                       i;
 
    if (!toc || file_nr >= toc->num_files) return NULL;
 
    if (toc->file_size[file_nr] == 0 || toc->file_size[file_nr] == 0xFFFFFFFF)
    {
        FIXME(">>> requesting NULL stream (%lu)\n", file_nr);
        return NULL;
    }
    block_list = &toc->file_size[toc->num_files];
    for (i = 0; i < file_nr; i++)
        block_list += (toc->file_size[i] + pdb->block_size - 1) / pdb->block_size;
 
    return pdb_ds_read(pdb, block_list, toc->file_size[file_nr]);
}
 
static void* pdb_read_file(const BYTE* image, const struct pdb_lookup* pdb_lookup,
                           DWORD file_nr)
{
    switch (pdb_lookup->kind)
    {
    case PDB_JG:
        return pdb_read_jg_file((const struct PDB_JG_HEADER*)image, 
                                pdb_lookup->u.jg.toc, file_nr);
    case PDB_DS:
        return pdb_read_ds_file((const struct PDB_DS_HEADER*)image,
                                pdb_lookup->u.ds.toc, file_nr);
    }
    return NULL;
}
 
static unsigned pdb_get_file_size(const struct pdb_lookup* pdb_lookup, DWORD file_nr)
{
    switch (pdb_lookup->kind)
    {
    case PDB_JG: return pdb_lookup->u.jg.toc->file[file_nr].size;
    case PDB_DS: return pdb_lookup->u.ds.toc->file_size[file_nr];
    }
    return 0;
}
 
static void pdb_free(void* buffer)
{
    HeapFree(GetProcessHeap(), 0, buffer);
}
 
static void pdb_free_lookup(const struct pdb_lookup* pdb_lookup)
{
    switch (pdb_lookup->kind)
    {
    case PDB_JG:
        if (pdb_lookup->u.jg.toc) pdb_free(pdb_lookup->u.jg.toc);
        break;
    case PDB_DS:
        if (pdb_lookup->u.ds.toc) pdb_free(pdb_lookup->u.ds.toc);
        break;
    }
}
 
static void pdb_convert_types_header(PDB_TYPES* types, const BYTE* image)
{
    memset(types, 0, sizeof(PDB_TYPES));
    if (!image) return;
 
    if (*(const DWORD*)image < 19960000)   /* FIXME: correct version? */
    {
        /* Old version of the types record header */
        const PDB_TYPES_OLD*    old = (const PDB_TYPES_OLD*)image;
        types->version     = old->version;
        types->type_offset = sizeof(PDB_TYPES_OLD);
        types->type_size   = old->type_size;
        types->first_index = old->first_index;
        types->last_index  = old->last_index;
        types->file        = old->file;
    }
    else
    {
        /* New version of the types record header */
        *types = *(const PDB_TYPES*)image;
    }
}
 
static void pdb_convert_symbols_header(PDB_SYMBOLS* symbols,
                                       int* header_size, const BYTE* image)
{
    memset(symbols, 0, sizeof(PDB_SYMBOLS));
    if (!image) return;
 
    if (*(const DWORD*)image != 0xffffffff)
    {
        /* Old version of the symbols record header */
        const PDB_SYMBOLS_OLD*  old = (const PDB_SYMBOLS_OLD*)image;
        symbols->version         = 0;
        symbols->module_size     = old->module_size;
        symbols->offset_size     = old->offset_size;
        symbols->hash_size       = old->hash_size;
        symbols->srcmodule_size  = old->srcmodule_size;
        symbols->pdbimport_size  = 0;
        symbols->hash1_file      = old->hash1_file;
        symbols->hash2_file      = old->hash2_file;
        symbols->gsym_file       = old->gsym_file;
 
        *header_size = sizeof(PDB_SYMBOLS_OLD);
    }
    else
    {
        /* New version of the symbols record header */
        *symbols = *(const PDB_SYMBOLS*)image;
        *header_size = sizeof(PDB_SYMBOLS);
    }
}
 
static void pdb_convert_symbol_file(const PDB_SYMBOLS* symbols, 
                                    PDB_SYMBOL_FILE_EX* sfile, 
                                    unsigned* size, const void* image)
 
{
    if (symbols->version < 19970000)
    {
        const PDB_SYMBOL_FILE *sym_file = (const PDB_SYMBOL_FILE*)image;
        memset(sfile, 0, sizeof(*sfile));
        sfile->file        = sym_file->file;
        sfile->range.index = sym_file->range.index;
        sfile->symbol_size = sym_file->symbol_size;
        sfile->lineno_size = sym_file->lineno_size;
        *size = sizeof(PDB_SYMBOL_FILE) - 1;
    }
    else
    {
        memcpy(sfile, image, sizeof(PDB_SYMBOL_FILE_EX));
        *size = sizeof(PDB_SYMBOL_FILE_EX) - 1;
    }
}
 
static BOOL CALLBACK pdb_match(char* file, void* user)
{
    /* accept first file */
    return FALSE;
}
 
static HANDLE open_pdb_file(const struct process* pcs, const char* filename)
{
    HANDLE      h;
    char        dbg_file_path[MAX_PATH];
 
    h = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, 
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    /* FIXME: should give more bits on the file to look at */
    if (h == INVALID_HANDLE_VALUE &&
        SymFindFileInPath(pcs->handle, NULL, (char*)filename, NULL, 0, 0, 0,
                          dbg_file_path, pdb_match, NULL))
    {
        h = CreateFileA(dbg_file_path, GENERIC_READ, FILE_SHARE_READ, NULL, 
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    return (h == INVALID_HANDLE_VALUE) ? NULL : h;
}
 
static void pdb_process_types(const struct msc_debug_info* msc_dbg, 
                              const char* image, struct pdb_lookup* pdb_lookup)
{
    char*       types_image = NULL;
 
    types_image = pdb_read_file(image, pdb_lookup, 2);
    if (types_image)
    {
        PDB_TYPES   types;
        pdb_convert_types_header(&types, types_image);
 
        /* Check for unknown versions */
        switch (types.version)
        {
        case 19950410:      /* VC 4.0 */
        case 19951122:
        case 19961031:      /* VC 5.0 / 6.0 */
        case 19990903:
            break;
        default:
            ERR("-Unknown type info version %ld\n", types.version);
        }
 
        /* Read type table */
        codeview_parse_type_table(msc_dbg->module, types_image + types.type_offset,
                                  types.type_size);
        pdb_free(types_image);
    }
}
 
static const char       PDB_JG_IDENT[] = "Microsoft C/C++ program database 2.00\r\n\032JG\0";
static const char       PDB_DS_IDENT[] = "Microsoft C/C++ MSF 7.00\r\n\032DS\0";
 
static BOOL pdb_init(struct pdb_lookup* pdb_lookup, const char* image)
{
    /* check the file header, and if ok, load the TOC */
    TRACE("PDB(%s): %.40s\n", pdb_lookup->filename, debugstr_an(image, 40));
    switch (pdb_lookup->kind)
    {
    case PDB_JG:
        pdb_lookup->u.jg.toc = NULL;
        if (memcmp(image, PDB_JG_IDENT, sizeof(PDB_JG_IDENT)))
        {
            FIXME("Couldn't match JG header\n");
            return FALSE;
        }
        else
        {
            const struct PDB_JG_HEADER* pdb = (const struct PDB_JG_HEADER*)image;
            struct PDB_JG_ROOT*         root;
 
            pdb_lookup->u.jg.toc = pdb_jg_read(pdb, pdb->toc_block, pdb->toc.size);
            root = pdb_read_jg_file(pdb, pdb_lookup->u.jg.toc, 1);
            if (!root)
            {
                ERR("-Unable to get root from .PDB in %s\n", pdb_lookup->filename);
                return FALSE;
            }
            switch (root->Version)
            {
            case 19950623:      /* VC 4.0 */
            case 19950814:
            case 19960307:      /* VC 5.0 */
            case 19970604:      /* VC 6.0 */
                break;
            default:
                ERR("-Unknown root block version %ld\n", root->Version);
            } 
            /* Check .PDB time stamp */
            if (root->TimeDateStamp != pdb_lookup->u.jg.timestamp)
            {
                ERR("-Wrong time stamp of .PDB file %s (0x%08lx, 0x%08lx)\n",
                    pdb_lookup->filename, root->TimeDateStamp, 
                    pdb_lookup->u.jg.timestamp);
            }
            pdb_free(root);
        }
        break;
    case PDB_DS:
        pdb_lookup->u.ds.toc = NULL;
        if (memcmp(image, PDB_DS_IDENT, sizeof(PDB_DS_IDENT)))
        {
            FIXME("Couldn't match DS header\n");
            return FALSE;
        }
        else
        {
            const struct PDB_DS_HEADER* pdb = (const struct PDB_DS_HEADER*)image;
            struct PDB_DS_ROOT*         root;
 
            pdb_lookup->u.ds.toc = 
                pdb_ds_read(pdb, 
                            (const DWORD*)((const char*)pdb + pdb->toc_page * pdb->block_size), 
                            pdb->toc_size);
            root = pdb_read_ds_file(pdb, pdb_lookup->u.ds.toc, 1);
            if (!root)
            {
                ERR("-Unable to get root from .PDB in %s\n", pdb_lookup->filename);
                return FALSE;
            }
            switch (root->Version)
            {
            case 20000404:
                break;
            default:
                ERR("-Unknown root block version %ld\n", root->Version);
            } 
            /* Check .PDB time stamp */
            if (memcmp(&root->guid, &pdb_lookup->u.ds.guid, sizeof(GUID)))
            {
                ERR("-Wrong GUID of .PDB file %s (%s, %s)\n",
                    pdb_lookup->filename, 
                    wine_dbgstr_guid(&root->guid), 
                    wine_dbgstr_guid(&pdb_lookup->u.ds.guid));
            }
            pdb_free(root);
        }
        break;
    }
 
    if (0) /* some tool to dump the internal files from a PDB file */
    {
        int     i, num_files;
 
        switch (pdb_lookup->kind)
        {
        case PDB_JG: num_files = pdb_lookup->u.jg.toc->num_files; break;
        case PDB_DS: num_files = pdb_lookup->u.ds.toc->num_files; break;
        }
 
        for (i = 1; i < num_files; i++)
        {
            unsigned char* x = pdb_read_file(image, pdb_lookup, i);
            FIXME("********************** [%u]: size=%08x\n",
                  i, pdb_get_file_size(pdb_lookup, i));
            dump(x, pdb_get_file_size(pdb_lookup, i));
            pdb_free(x);
        }
    }
    return TRUE;
}
 
static BOOL pdb_process_internal(const struct process* pcs, 
                                 const struct msc_debug_info* msc_dbg,
                                 struct pdb_lookup* pdb_lookup,
                                 unsigned module_index);
 
static void pdb_process_symbol_imports(const struct process* pcs, 
                                       const struct msc_debug_info* msc_dbg,
                                       PDB_SYMBOLS* symbols, 
                                       const void* symbols_image,
                                       char* image, struct pdb_lookup* pdb_lookup,
                                       unsigned module_index)
{
    if (module_index == -1 && symbols && symbols->pdbimport_size)
    {
        const PDB_SYMBOL_IMPORT*imp;
        const void*             first;
        const void*             last;
        const char*             ptr;
        int                     i = 0;
 
        imp = (const PDB_SYMBOL_IMPORT*)((const char*)symbols_image + sizeof(PDB_SYMBOLS) + 
                                         symbols->module_size + symbols->offset_size + 
                                         symbols->hash_size + symbols->srcmodule_size);
        first = (const char*)imp;
        last = (const char*)imp + symbols->pdbimport_size;
        while (imp < (const PDB_SYMBOL_IMPORT*)last)
        {
            ptr = (const char*)imp + sizeof(*imp) + strlen(imp->filename);
            if (i >= CV_MAX_MODULES) FIXME("Out of bounds !!!\n");
            if (!strcasecmp(pdb_lookup->filename, imp->filename))
            {
                if (module_index != -1) FIXME("Twice the entry\n");
                else module_index = i;
            }
            else
            {
                struct pdb_lookup       imp_pdb_lookup;
 
                imp_pdb_lookup.filename = imp->filename;
                imp_pdb_lookup.kind = PDB_JG;
                imp_pdb_lookup.u.jg.timestamp = imp->TimeDateStamp;
                pdb_process_internal(pcs, msc_dbg, &imp_pdb_lookup, i);
            }
            i++;
            imp = (const PDB_SYMBOL_IMPORT*)((const char*)first + ((ptr - (const char*)first + strlen(ptr) + 1 + 3) & ~3));
        }
    }
    cv_current_module = &cv_zmodules[(module_index == -1) ? 0 : module_index];
    if (cv_current_module->allowed) FIXME("Already allowed ??\n");
    cv_current_module->allowed = TRUE;
    pdb_process_types(msc_dbg, image, pdb_lookup);
}
 
static BOOL pdb_process_internal(const struct process* pcs, 
                                 const struct msc_debug_info* msc_dbg,
                                 struct pdb_lookup* pdb_lookup, 
                                 unsigned module_index)
{
    BOOL        ret = FALSE;
    HANDLE      hFile, hMap = NULL;
    char*       image = NULL;
    char*       symbols_image = NULL;
 
    TRACE("Processing PDB file %s\n", pdb_lookup->filename);
 
    /* Open and map() .PDB file */
    if ((hFile = open_pdb_file(pcs, pdb_lookup->filename)) == NULL ||
        ((hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) == NULL) ||
        ((image = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) == NULL))
    {
        ERR("-Unable to peruse .PDB file %s\n", pdb_lookup->filename);
        goto leave;
    }
    pdb_init(pdb_lookup, image);
 
    symbols_image = pdb_read_file(image, pdb_lookup, 3);
    if (symbols_image)
    {
        PDB_SYMBOLS symbols;
        char*       modimage;
        char*       file;
        int         header_size = 0;
 
        pdb_convert_symbols_header(&symbols, &header_size, symbols_image);
        switch (symbols.version)
        {
        case 0:            /* VC 4.0 */
        case 19960307:     /* VC 5.0 */
        case 19970606:     /* VC 6.0 */
        case 19990903:
            break;
        default:
            ERR("-Unknown symbol info version %ld %08lx\n", 
                symbols.version, symbols.version);
        }
 
        pdb_process_symbol_imports(pcs, msc_dbg, &symbols, symbols_image, image, pdb_lookup, module_index);
 
        /* Read global symbol table */
        modimage = pdb_read_file(image, pdb_lookup, symbols.gsym_file);
        if (modimage)
        {
            codeview_snarf(msc_dbg, modimage, 0, 
                           pdb_get_file_size(pdb_lookup, symbols.gsym_file), NULL);
 
            pdb_free(modimage);
        }
 
        /* Read per-module symbol / linenumber tables */
        file = symbols_image + header_size;
        while (file - symbols_image < header_size + symbols.module_size)
        {
            PDB_SYMBOL_FILE_EX          sfile;
            const char*                 file_name;
            unsigned                    size;
 
            HeapValidate(GetProcessHeap(), 0, NULL);
            pdb_convert_symbol_file(&symbols, &sfile, &size, file);
 
            modimage = pdb_read_file(image, pdb_lookup, sfile.file);
            if (modimage)
            {
                struct codeview_linetab*    linetab = NULL;
 
                if (sfile.lineno_size)
                    linetab = codeview_snarf_linetab(msc_dbg->module, 
                                                     modimage + sfile.symbol_size,
                                                     sfile.lineno_size,
                                                     pdb_lookup->kind == PDB_JG);
 
                if (sfile.symbol_size)
                    codeview_snarf(msc_dbg, modimage, sizeof(DWORD),
                                   sfile.symbol_size, linetab);
 
                pdb_free(modimage);
            }
            file_name = (const char*)file + size;
            file_name += strlen(file_name) + 1;
            file = (char*)((DWORD)(file_name + strlen(file_name) + 1 + 3) & ~3);
        }
    }
    else
        pdb_process_symbol_imports(pcs, msc_dbg, NULL, NULL, image, pdb_lookup, 
                                   module_index);
    msc_dbg->module->module.SymType = SymCv;
    ret = TRUE;
 
 leave:
    /* Cleanup */
    if (symbols_image) pdb_free(symbols_image);
    pdb_free_lookup(pdb_lookup);
 
    if (image) UnmapViewOfFile(image);
    if (hMap) CloseHandle(hMap);
    if (hFile) CloseHandle(hFile);
 
    return ret;
}
 
static BOOL pdb_process_file(const struct process* pcs, 
                             const struct msc_debug_info* msc_dbg,
                             struct pdb_lookup* pdb_lookup)
{
    BOOL        ret;
 
    memset(cv_zmodules, 0, sizeof(cv_zmodules));
    codeview_init_basic_types(msc_dbg->module);
    ret = pdb_process_internal(pcs, msc_dbg, pdb_lookup, -1);
    codeview_clear_type_table();
    return ret;
}
 
/*========================================================================
 * Process CodeView debug information.
 */
 
#define MAKESIG(a,b,c,d)        ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))
#define CODEVIEW_NB09_SIG       MAKESIG('N','B','0','9')
#define CODEVIEW_NB10_SIG       MAKESIG('N','B','1','0')
#define CODEVIEW_NB11_SIG       MAKESIG('N','B','1','1')
#define CODEVIEW_RSDS_SIG       MAKESIG('R','S','D','S')
 
typedef struct _CODEVIEW_HEADER_NBxx
{
    DWORD       dwSignature;
    DWORD       lfoDirectory;
} CODEVIEW_HEADER_NBxx,* PCODEVIEW_HEADER_NBxx;
 
typedef struct _CODEVIEW_HEADER_RSDS
{
    DWORD       dwSignature;
    GUID        guid;
    DWORD       unknown;
    CHAR        name[1];
} CODEVIEW_HEADER_RSDS,* PCODEVIEW_HEADER_RSDS;
 
typedef struct _CODEVIEW_PDB_DATA
{
    DWORD       timestamp;
    DWORD       unknown;
    CHAR        name[1];
} CODEVIEW_PDB_DATA, *PCODEVIEW_PDB_DATA;
 
typedef struct _CV_DIRECTORY_HEADER
{
    WORD        cbDirHeader;
    WORD        cbDirEntry;
    DWORD       cDir;
    DWORD       lfoNextDir;
    DWORD       flags;
} CV_DIRECTORY_HEADER, *PCV_DIRECTORY_HEADER;
 
typedef struct _CV_DIRECTORY_ENTRY
{
    WORD        subsection;
    WORD        iMod;
    DWORD       lfo;
    DWORD       cb;
} CV_DIRECTORY_ENTRY, *PCV_DIRECTORY_ENTRY;
 
#define	sstAlignSym		0x125
#define	sstSrcModule		0x127
 
static BOOL codeview_process_info(const struct process* pcs, 
                                  const struct msc_debug_info* msc_dbg)
{
    const CODEVIEW_HEADER_NBxx* cv = (const CODEVIEW_HEADER_NBxx*)msc_dbg->root;
    BOOL                        ret = FALSE;
    struct pdb_lookup           pdb_lookup;
 
    TRACE("Processing signature %.4s\n", (const char*)&cv->dwSignature);
 
    switch (cv->dwSignature)
    {
    case CODEVIEW_NB09_SIG:
    case CODEVIEW_NB11_SIG:
    {
        const CV_DIRECTORY_HEADER*      hdr = (const CV_DIRECTORY_HEADER*)(msc_dbg->root + cv->lfoDirectory);
        const CV_DIRECTORY_ENTRY*       ent;
        const CV_DIRECTORY_ENTRY*       prev;
        const CV_DIRECTORY_ENTRY*       next;
        unsigned int                    i;
 
        codeview_init_basic_types(msc_dbg->module);
        ent = (const CV_DIRECTORY_ENTRY*)((const BYTE*)hdr + hdr->cbDirHeader);
        for (i = 0; i < hdr->cDir; i++, ent = next)
        {
            next = (i == hdr->cDir-1)? NULL :
                   (const CV_DIRECTORY_ENTRY*)((const BYTE*)ent + hdr->cbDirEntry);
            prev = (i == 0)? NULL :
                   (const CV_DIRECTORY_ENTRY*)((const BYTE*)ent - hdr->cbDirEntry);
 
            if (ent->subsection == sstAlignSym)
            {
                /*
                 * Check the next and previous entry.  If either is a
                 * sstSrcModule, it contains the line number info for
                 * this file.
                 *
                 * FIXME: This is not a general solution!
                 */
                struct codeview_linetab*        linetab = NULL;
 
                if (next && next->iMod == ent->iMod && 
                    next->subsection == sstSrcModule)
                    linetab = codeview_snarf_linetab(msc_dbg->module, 
                                                     msc_dbg->root + next->lfo, next->cb, 
                                                     TRUE);
 
                if (prev && prev->iMod == ent->iMod &&
                    prev->subsection == sstSrcModule)
                    linetab = codeview_snarf_linetab(msc_dbg->module, 
                                                     msc_dbg->root + prev->lfo, prev->cb, 
                                                     TRUE);
 
                codeview_snarf(msc_dbg, msc_dbg->root + ent->lfo, sizeof(DWORD),
                               ent->cb, linetab);
            }
        }
 
        msc_dbg->module->module.SymType = SymCv;
        ret = TRUE;
        break;
    }
 
    case CODEVIEW_NB10_SIG:
    {
        const CODEVIEW_PDB_DATA* pdb = (const CODEVIEW_PDB_DATA*)(cv + 1);
        pdb_lookup.filename = pdb->name;
        pdb_lookup.kind = PDB_JG;
        pdb_lookup.u.jg.timestamp = pdb->timestamp;
        pdb_lookup.u.jg.toc = NULL;
        ret = pdb_process_file(pcs, msc_dbg, &pdb_lookup);
        break;
    }
    case CODEVIEW_RSDS_SIG:
    {
        const CODEVIEW_HEADER_RSDS* rsds = (const CODEVIEW_HEADER_RSDS*)msc_dbg->root;
 
        TRACE("Got RSDS type of PDB file: guid=%s unk=%08lx name=%s\n",
              wine_dbgstr_guid(&rsds->guid), rsds->unknown, rsds->name);
        pdb_lookup.filename = rsds->name;
        pdb_lookup.kind = PDB_DS;
        pdb_lookup.u.ds.guid = rsds->guid;
        pdb_lookup.u.ds.toc = NULL;
        ret = pdb_process_file(pcs, msc_dbg, &pdb_lookup);
        break;
    }
    default:
        ERR("Unknown CODEVIEW signature %08lX in module %s\n",
            cv->dwSignature, msc_dbg->module->module.ModuleName);
        break;
    }
 
    return ret;
}
 
/*========================================================================
 * Process debug directory.
 */
BOOL pe_load_debug_directory(const struct process* pcs, struct module* module, 
                             const BYTE* mapping,
                             const IMAGE_SECTION_HEADER* sectp, DWORD nsect,
                             const IMAGE_DEBUG_DIRECTORY* dbg, int nDbg)
{
    BOOL                        ret = FALSE;
    int                         i;
    struct msc_debug_info       msc_dbg;
 
    msc_dbg.module = module;
    msc_dbg.nsect  = nsect;
    msc_dbg.sectp  = sectp;
    msc_dbg.nomap  = 0;
    msc_dbg.omapp  = NULL;
 
    _SEH_TRY
    {
        /* First, watch out for OMAP data */
        for (i = 0; i < nDbg; i++)
        {
            if (dbg[i].Type == IMAGE_DEBUG_TYPE_OMAP_FROM_SRC)
            {
                msc_dbg.nomap = dbg[i].SizeOfData / sizeof(OMAP_DATA);
                msc_dbg.omapp = (const OMAP_DATA*)(mapping + dbg[i].PointerToRawData);
                break;
            }
        }
 
        /* Now, try to parse CodeView debug info */
        for (i = 0; i < nDbg; i++)
        {
            if (dbg[i].Type == IMAGE_DEBUG_TYPE_CODEVIEW)
            {
                msc_dbg.root = mapping + dbg[i].PointerToRawData;
                if ((ret = codeview_process_info(pcs, &msc_dbg))) goto done;
            }
        }
 
        /* If not found, try to parse COFF debug info */
        for (i = 0; i < nDbg; i++)
        {
            if (dbg[i].Type == IMAGE_DEBUG_TYPE_COFF)
            {
                msc_dbg.root = mapping + dbg[i].PointerToRawData;
                if ((ret = coff_process_info(&msc_dbg))) goto done;
            }
        }
    done:
	 /* FIXME: this should be supported... this is the debug information for
	  * functions compiled without a frame pointer (FPO = frame pointer omission)
	  * the associated data helps finding out the relevant information
	  */
        for (i = 0; i < nDbg; i++)
            if (dbg[i].Type == IMAGE_DEBUG_TYPE_FPO)
                FIXME("This guy has FPO information\n");
#if 0
 
#define FRAME_FPO   0
#define FRAME_TRAP  1
#define FRAME_TSS   2
 
typedef struct _FPO_DATA 
{
	DWORD       ulOffStart;            /* offset 1st byte of function code */
	DWORD       cbProcSize;            /* # bytes in function */
	DWORD       cdwLocals;             /* # bytes in locals/4 */
	WORD        cdwParams;             /* # bytes in params/4 */
 
	WORD        cbProlog : 8;          /* # bytes in prolog */
	WORD        cbRegs   : 3;          /* # regs saved */
	WORD        fHasSEH  : 1;          /* TRUE if SEH in func */
	WORD        fUseBP   : 1;          /* TRUE if EBP has been allocated */
	WORD        reserved : 1;          /* reserved for future use */
	WORD        cbFrame  : 2;          /* frame type */
} FPO_DATA;
#endif
 
    }
    _SEH_EXCEPT(page_fault)
    {
        ERR("Got a page fault while loading symbols\n");
        ret = FALSE;
    }
    _SEH_END;
    return ret;
}
