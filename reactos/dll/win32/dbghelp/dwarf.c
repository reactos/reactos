/*
 * File dwarf.c - read dwarf2 information from the ELF modules
 *
 * Copyright (C) 2005, Raphael Junqueira
 * Copyright (C) 2006, Eric Pouech
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

#include "config.h"

#include <sys/types.h>
#include <fcntl.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdio.h>
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "oleauto.h"

#include "dbghelp_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp_dwarf);

/* FIXME:
 * - Functions:
 *      o unspecified parameters
 *      o inlined functions
 *      o Debug{Start|End}Point
 *      o CFA
 * - Udt
 *      o proper types loading (nesting)
 */

#if 0
static void dump(const void* ptr, unsigned len)
{
    int         i, j;
    BYTE        msg[128];
    static const char hexof[] = "0123456789abcdef";
    const       BYTE* x = (const BYTE*)ptr;

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
        TRACE("%s\n", msg);
    }
}
#endif

/**
 *
 * Main Specs:
 *  http://www.eagercon.com/dwarf/dwarf3std.htm
 *  http://www.eagercon.com/dwarf/dwarf-2.0.0.pdf
 *
 * dwarf2.h: http://www.hakpetzna.com/b/binutils/dwarf2_8h-source.html
 *
 * example of projects who do dwarf2 parsing:
 *  http://www.x86-64.org/cgi-bin/cvsweb.cgi/binutils.dead/binutils/readelf.c?rev=1.1.1.2
 *  http://elis.ugent.be/diota/log/ltrace_elf.c
 */
#include "dwarf.h"

/**
 * Parsers
 */

typedef struct dwarf2_abbrev_entry_attr_s
{
  unsigned long attribute;
  unsigned long form;
  struct dwarf2_abbrev_entry_attr_s* next;
} dwarf2_abbrev_entry_attr_t;

typedef struct dwarf2_abbrev_entry_s
{
    unsigned long entry_code;
    unsigned long tag;
    unsigned char have_child;
    unsigned num_attr;
    dwarf2_abbrev_entry_attr_t* attrs;
} dwarf2_abbrev_entry_t;

struct dwarf2_block
{
    unsigned                    size;
    const unsigned char*        ptr;
};

struct attribute
{
    unsigned long               form;
    union
    {
        unsigned long                   uvalue;
        long                            svalue;
        const char*                     string;
        struct dwarf2_block             block;
    } u;
};

typedef struct dwarf2_debug_info_s
{
    const dwarf2_abbrev_entry_t*abbrev;
    struct symt*                symt;
    const unsigned char**       data;
    struct vector               children;
} dwarf2_debug_info_t;

typedef struct dwarf2_section_s
{
    const unsigned char*        address;
    unsigned                    size;
} dwarf2_section_t;

enum dwarf2_sections {section_debug, section_string, section_abbrev, section_line, section_max};

typedef struct dwarf2_traverse_context_s
{
    const unsigned char*        data;
    const unsigned char*        start_data;
    const unsigned char*        end_data;
    unsigned char               word_size;
} dwarf2_traverse_context_t;

typedef struct dwarf2_parse_context_s
{
    const dwarf2_section_t*     sections;
    unsigned                    section;
    struct pool                 pool;
    struct module*              module;
    const struct elf_thunk_area*thunks;
    struct sparse_array         abbrev_table;
    struct sparse_array         debug_info_table;
    unsigned long               load_offset;
    unsigned long               ref_offset;
    unsigned char               word_size;
} dwarf2_parse_context_t;

/* stored in the dbghelp's module internal structure for later reuse */
struct dwarf2_module_info_s
{
    dwarf2_section_t            debug_loc;
};

#define loc_dwarf2_location_list        (loc_user + 0)
#define loc_dwarf2_block                (loc_user + 1)

/* forward declarations */
static struct symt* dwarf2_parse_enumeration_type(dwarf2_parse_context_t* ctx, dwarf2_debug_info_t* entry);

static unsigned char dwarf2_get_byte(const unsigned char* ptr)
{
    return *ptr;
}

static unsigned char dwarf2_parse_byte(dwarf2_traverse_context_t* ctx)
{
    unsigned char uvalue = dwarf2_get_byte(ctx->data);
    ctx->data += 1;
    return uvalue;
}

static unsigned short dwarf2_get_u2(const unsigned char* ptr)
{
    return *(const unsigned short*)ptr;
}

static unsigned short dwarf2_parse_u2(dwarf2_traverse_context_t* ctx)
{
    unsigned short uvalue = dwarf2_get_u2(ctx->data);
    ctx->data += 2;
    return uvalue;
}

static unsigned long dwarf2_get_u4(const unsigned char* ptr)
{
    return *(const unsigned long*)ptr;
}

static unsigned long dwarf2_parse_u4(dwarf2_traverse_context_t* ctx)
{
    unsigned long uvalue = dwarf2_get_u4(ctx->data);
    ctx->data += 4;
    return uvalue;
}

static unsigned long dwarf2_get_leb128_as_unsigned(const unsigned char* ptr, const unsigned char** end)
{
    unsigned long ret = 0;
    unsigned char byte;
    unsigned shift = 0;

    do
    {
        byte = dwarf2_get_byte(ptr++);
        ret |= (byte & 0x7f) << shift;
        shift += 7;
    } while (byte & 0x80);

    if (end) *end = ptr;
    return ret;
}

static unsigned long dwarf2_leb128_as_unsigned(dwarf2_traverse_context_t* ctx)
{
    unsigned long ret;

    assert(ctx);

    ret = dwarf2_get_leb128_as_unsigned(ctx->data, &ctx->data);

    return ret;
}

static long dwarf2_get_leb128_as_signed(const unsigned char* ptr, const unsigned char** end)
{
    long ret = 0;
    unsigned char byte;
    unsigned shift = 0;
    const unsigned size = sizeof(int) * 8;

    do
    {
        byte = dwarf2_get_byte(ptr++);
        ret |= (byte & 0x7f) << shift;
        shift += 7;
    } while (byte & 0x80);
    if (end) *end = ptr;

    /* as spec: sign bit of byte is 2nd high order bit (80x40)
     *  -> 0x80 is used as flag.
     */
    if ((shift < size) && (byte & 0x40))
    {
        ret |= - (1 << shift);
    }
    return ret;
}

static long dwarf2_leb128_as_signed(dwarf2_traverse_context_t* ctx)
{
    long ret = 0;

    assert(ctx);

    ret = dwarf2_get_leb128_as_signed(ctx->data, &ctx->data);
    return ret;
}

static unsigned dwarf2_leb128_length(const dwarf2_traverse_context_t* ctx)
{
    unsigned    ret;
    for (ret = 0; ctx->data[ret] & 0x80; ret++);
    return ret + 1;
}

static unsigned long dwarf2_get_addr(const unsigned char* ptr, unsigned word_size)
{
    unsigned long ret;

    switch (word_size)
    {
    case 4:
        ret = dwarf2_get_u4(ptr);
        break;
    default:
        FIXME("Unsupported Word Size %u\n", word_size);
        ret = 0;
    }
    return ret;
}

static unsigned long dwarf2_parse_addr(dwarf2_traverse_context_t* ctx)
{
    unsigned long ret = dwarf2_get_addr(ctx->data, ctx->word_size);
    ctx->data += ctx->word_size;
    return ret;
}

static const char* dwarf2_debug_traverse_ctx(const dwarf2_traverse_context_t* ctx) 
{
    return wine_dbg_sprintf("ctx(%p)", ctx->data); 
}

static const char* dwarf2_debug_ctx(const dwarf2_parse_context_t* ctx)
{
    return wine_dbg_sprintf("ctx(%p,%s)",
                            ctx, debugstr_w(ctx->module->module.ModuleName));
}

static const char* dwarf2_debug_di(const dwarf2_debug_info_t* di)
{
    return wine_dbg_sprintf("debug_info(abbrev:%p,symt:%p)",
                            di->abbrev, di->symt);
}

static dwarf2_abbrev_entry_t*
dwarf2_abbrev_table_find_entry(const struct sparse_array* abbrev_table,
                               unsigned long entry_code)
{
    assert( NULL != abbrev_table );
    return sparse_array_find(abbrev_table, entry_code);
}

static void dwarf2_parse_abbrev_set(dwarf2_traverse_context_t* abbrev_ctx, 
                                    struct sparse_array* abbrev_table,
                                    struct pool* pool)
{
    unsigned long entry_code;
    dwarf2_abbrev_entry_t* abbrev_entry;
    dwarf2_abbrev_entry_attr_t* new = NULL;
    dwarf2_abbrev_entry_attr_t* last = NULL;
    unsigned long attribute;
    unsigned long form;

    assert( NULL != abbrev_ctx );

    TRACE("%s, end at %p\n",
          dwarf2_debug_traverse_ctx(abbrev_ctx), abbrev_ctx->end_data); 

    sparse_array_init(abbrev_table, sizeof(dwarf2_abbrev_entry_t), 32);
    while (abbrev_ctx->data < abbrev_ctx->end_data)
    {
        TRACE("now at %s\n", dwarf2_debug_traverse_ctx(abbrev_ctx)); 
        entry_code = dwarf2_leb128_as_unsigned(abbrev_ctx);
        TRACE("found entry_code %lu\n", entry_code);
        if (!entry_code)
        {
            TRACE("NULL entry code at %s\n", dwarf2_debug_traverse_ctx(abbrev_ctx)); 
            break;
        }
        abbrev_entry = sparse_array_add(abbrev_table, entry_code, pool);
        assert( NULL != abbrev_entry );

        abbrev_entry->entry_code = entry_code;
        abbrev_entry->tag        = dwarf2_leb128_as_unsigned(abbrev_ctx);
        abbrev_entry->have_child = dwarf2_parse_byte(abbrev_ctx);
        abbrev_entry->attrs      = NULL;
        abbrev_entry->num_attr   = 0;

        TRACE("table:(%p,#%u) entry_code(%lu) tag(0x%lx) have_child(%u) -> %p\n",
              abbrev_table, sparse_array_length(abbrev_table),
              entry_code, abbrev_entry->tag, abbrev_entry->have_child, abbrev_entry);

        last = NULL;
        while (1)
        {
            attribute = dwarf2_leb128_as_unsigned(abbrev_ctx);
            form = dwarf2_leb128_as_unsigned(abbrev_ctx);
            if (!attribute) break;

            new = pool_alloc(pool, sizeof(dwarf2_abbrev_entry_attr_t));
            assert(new);

            new->attribute = attribute;
            new->form      = form;
            new->next      = NULL;
            if (abbrev_entry->attrs)    last->next = new;
            else                        abbrev_entry->attrs = new;
            last = new;
            abbrev_entry->num_attr++;
        }
    }
    TRACE("found %u entries\n", sparse_array_length(abbrev_table));
}

static void dwarf2_swallow_attribute(dwarf2_traverse_context_t* ctx,
                                     const dwarf2_abbrev_entry_attr_t* abbrev_attr)
{
    unsigned    step;

    TRACE("(attr:0x%lx,form:0x%lx)\n", abbrev_attr->attribute, abbrev_attr->form);

    switch (abbrev_attr->form)
    {
    case DW_FORM_ref_addr:
    case DW_FORM_addr:   step = ctx->word_size; break;
    case DW_FORM_flag:
    case DW_FORM_data1:
    case DW_FORM_ref1:   step = 1; break;
    case DW_FORM_data2:
    case DW_FORM_ref2:   step = 2; break;
    case DW_FORM_data4:
    case DW_FORM_ref4:
    case DW_FORM_strp:   step = 4; break;
    case DW_FORM_data8:
    case DW_FORM_ref8:   step = 8; break;
    case DW_FORM_sdata:
    case DW_FORM_ref_udata:
    case DW_FORM_udata:  step = dwarf2_leb128_length(ctx); break;
    case DW_FORM_string: step = strlen((const char*)ctx->data) + 1; break;
    case DW_FORM_block:  step = dwarf2_leb128_as_unsigned(ctx); break;
    case DW_FORM_block1: step = dwarf2_parse_byte(ctx); break;
    case DW_FORM_block2: step = dwarf2_parse_u2(ctx); break;
    case DW_FORM_block4: step = dwarf2_parse_u4(ctx); break;
    default:
        FIXME("Unhandled attribute form %lx\n", abbrev_attr->form);
        return;
    }
    ctx->data += step;
}

static void dwarf2_fill_attr(const dwarf2_parse_context_t* ctx,
                             const dwarf2_abbrev_entry_attr_t* abbrev_attr,
                             const unsigned char* data,
                             struct attribute* attr)
{
    attr->form = abbrev_attr->form;
    switch (attr->form)
    {
    case DW_FORM_ref_addr:
    case DW_FORM_addr:
        attr->u.uvalue = dwarf2_get_addr(data, ctx->word_size);
        TRACE("addr<0x%lx>\n", attr->u.uvalue);
        break;

    case DW_FORM_flag:
        attr->u.uvalue = dwarf2_get_byte(data);
        TRACE("flag<0x%lx>\n", attr->u.uvalue);
        break;

    case DW_FORM_data1:
        attr->u.uvalue = dwarf2_get_byte(data);
        TRACE("data1<%lu>\n", attr->u.uvalue);
        break;

    case DW_FORM_data2:
        attr->u.uvalue = dwarf2_get_u2(data);
        TRACE("data2<%lu>\n", attr->u.uvalue);
        break;

    case DW_FORM_data4:
        attr->u.uvalue = dwarf2_get_u4(data);
        TRACE("data4<%lu>\n", attr->u.uvalue);
        break;

    case DW_FORM_data8:
        FIXME("Unhandled 64bits support\n");
        break;

    case DW_FORM_ref1:
        attr->u.uvalue = ctx->ref_offset + dwarf2_get_byte(data);
        TRACE("ref1<0x%lx>\n", attr->u.uvalue);
        break;

    case DW_FORM_ref2:
        attr->u.uvalue = ctx->ref_offset + dwarf2_get_u2(data);
        TRACE("ref2<0x%lx>\n", attr->u.uvalue);
        break;

    case DW_FORM_ref4:
        attr->u.uvalue = ctx->ref_offset + dwarf2_get_u4(data);
        TRACE("ref4<0x%lx>\n", attr->u.uvalue);
        break;
    
    case DW_FORM_ref8:
        FIXME("Unhandled 64 bit support\n");
        break;

    case DW_FORM_sdata:
        attr->u.svalue = dwarf2_get_leb128_as_signed(data, NULL);
        break;

    case DW_FORM_ref_udata:
        attr->u.uvalue = dwarf2_get_leb128_as_unsigned(data, NULL);
        break;

    case DW_FORM_udata:
        attr->u.uvalue = dwarf2_get_leb128_as_unsigned(data, NULL);
        break;

    case DW_FORM_string:
        attr->u.string = (const char *)data;
        TRACE("string<%s>\n", attr->u.string);
        break;

    case DW_FORM_strp:
    {
        unsigned long offset = dwarf2_get_u4(data);
        attr->u.string = (const char*)ctx->sections[section_string].address + offset;
    }
    TRACE("strp<%s>\n", attr->u.string);
    break;
        
    case DW_FORM_block:
        attr->u.block.size = dwarf2_get_leb128_as_unsigned(data, &attr->u.block.ptr);
        break;

    case DW_FORM_block1:
        attr->u.block.size = dwarf2_get_byte(data);
        attr->u.block.ptr  = data + 1;
        break;

    case DW_FORM_block2:
        attr->u.block.size = dwarf2_get_u2(data);
        attr->u.block.ptr  = data + 2;
        break;

    case DW_FORM_block4:
        attr->u.block.size = dwarf2_get_u4(data);
        attr->u.block.ptr  = data + 4;
        break;

    default:
        FIXME("Unhandled attribute form %lx\n", abbrev_attr->form);
        break;
    }
}

static BOOL dwarf2_find_attribute(const dwarf2_parse_context_t* ctx,
                                  const dwarf2_debug_info_t* di,
                                  unsigned at, struct attribute* attr)
{
    unsigned                    i, ai = 0;
    dwarf2_abbrev_entry_attr_t* abbrev_attr;
    dwarf2_abbrev_entry_attr_t* abstract_abbrev_attr;

    while (di)
    {
        abstract_abbrev_attr = NULL;
        for (i = 0, abbrev_attr = di->abbrev->attrs; abbrev_attr; i++, abbrev_attr = abbrev_attr->next)
        {
            if (abbrev_attr->attribute == at)
            {
                dwarf2_fill_attr(ctx, abbrev_attr, di->data[i], attr);
                return TRUE;
            }
            if (abbrev_attr->attribute == DW_AT_abstract_origin &&
                at != DW_AT_sibling)
            {
                abstract_abbrev_attr = abbrev_attr;
                ai = i;
            }
        }
        /* do we have an abstract origin debug entry to look into ? */
        if (!abstract_abbrev_attr) break;
        dwarf2_fill_attr(ctx, abstract_abbrev_attr, di->data[ai], attr);
        if (!(di = sparse_array_find(&ctx->debug_info_table, attr->u.uvalue)))
            FIXME("Should have found the debug info entry\n");
    }
    return FALSE;
}

static void dwarf2_load_one_entry(dwarf2_parse_context_t*, dwarf2_debug_info_t*,
                                  struct symt_compiland*);

#define Wine_DW_no_register     0x7FFFFFFF

static unsigned dwarf2_map_register(int regno)
{
    unsigned    reg;

    switch (regno)
    {
    case Wine_DW_no_register: FIXME("What the heck map reg 0x%x\n",regno); reg = 0; break;
    case  0: reg = CV_REG_EAX; break;
    case  1: reg = CV_REG_ECX; break;
    case  2: reg = CV_REG_EDX; break;
    case  3: reg = CV_REG_EBX; break;
    case  4: reg = CV_REG_ESP; break;
    case  5: reg = CV_REG_EBP; break;
    case  6: reg = CV_REG_ESI; break;
    case  7: reg = CV_REG_EDI; break;
    case  8: reg = CV_REG_EIP; break;
    case  9: reg = CV_REG_EFLAGS; break;
    case 10: reg = CV_REG_CS;  break;
    case 11: reg = CV_REG_SS;  break;
    case 12: reg = CV_REG_DS;  break;
    case 13: reg = CV_REG_ES;  break;
    case 14: reg = CV_REG_FS;  break;
    case 15: reg = CV_REG_GS;  break;
    case 16: case 17: case 18: case 19:
    case 20: case 21: case 22: case 23:
        reg = CV_REG_ST0 + regno - 16; break;
    case 24: reg = CV_REG_CTRL; break;
    case 25: reg = CV_REG_STAT; break;
    case 26: reg = CV_REG_TAG; break;
/*
reg: fiseg 27
reg: fioff 28
reg: foseg 29
reg: fooff 30
reg: fop   31
*/
    case 32: case 33: case 34: case 35:
    case 36: case 37: case 38: case 39:
        reg = CV_REG_XMM0 + regno - 32; break;
    case 40: reg = CV_REG_MXCSR; break;
    default:
        FIXME("Don't know how to map register %d\n", regno);
        return 0;
    }
    return reg;
}

static enum location_error
compute_location(dwarf2_traverse_context_t* ctx, struct location* loc,
                 HANDLE hproc, const struct location* frame)
{
    unsigned long stack[64];
    unsigned stk;
    unsigned char op;
    BOOL piece_found = FALSE;

    stack[stk = 0] = 0;

    loc->kind = loc_absolute;
    loc->reg = Wine_DW_no_register;

    while (ctx->data < ctx->end_data)
    {
        op = dwarf2_parse_byte(ctx);
        switch (op)
        {
        case DW_OP_addr:    stack[++stk] = dwarf2_parse_addr(ctx); break;
        case DW_OP_const1u: stack[++stk] = dwarf2_parse_byte(ctx); break;
        case DW_OP_const1s: stack[++stk] = (long)(signed char)dwarf2_parse_byte(ctx); break;
        case DW_OP_const2u: stack[++stk] = dwarf2_parse_u2(ctx); break;
        case DW_OP_const2s: stack[++stk] = (long)(short)dwarf2_parse_u2(ctx); break;
        case DW_OP_const4u: stack[++stk] = dwarf2_parse_u4(ctx); break;
        case DW_OP_const4s: stack[++stk] = dwarf2_parse_u4(ctx); break;
        case DW_OP_constu:  stack[++stk] = dwarf2_leb128_as_unsigned(ctx); break;
        case DW_OP_consts:  stack[++stk] = dwarf2_leb128_as_signed(ctx); break;
        case DW_OP_plus_uconst:
            stack[stk] += dwarf2_leb128_as_unsigned(ctx); break;
        case DW_OP_reg0:  case DW_OP_reg1:  case DW_OP_reg2:  case DW_OP_reg3:
        case DW_OP_reg4:  case DW_OP_reg5:  case DW_OP_reg6:  case DW_OP_reg7:
        case DW_OP_reg8:  case DW_OP_reg9:  case DW_OP_reg10: case DW_OP_reg11:
        case DW_OP_reg12: case DW_OP_reg13: case DW_OP_reg14: case DW_OP_reg15:
        case DW_OP_reg16: case DW_OP_reg17: case DW_OP_reg18: case DW_OP_reg19:
        case DW_OP_reg20: case DW_OP_reg21: case DW_OP_reg22: case DW_OP_reg23:
        case DW_OP_reg24: case DW_OP_reg25: case DW_OP_reg26: case DW_OP_reg27:
        case DW_OP_reg28: case DW_OP_reg29: case DW_OP_reg30: case DW_OP_reg31:
            /* dbghelp APIs don't know how to cope with this anyway
             * (for example 'long long' stored in two registers)
             * FIXME: We should tell winedbg how to deal with it (sigh)
             */
            if (!piece_found)
            {
                if (loc->reg != Wine_DW_no_register)
                    FIXME("Only supporting one reg (%d -> %d)\n",
                          loc->reg, dwarf2_map_register(op - DW_OP_reg0));
                loc->reg = dwarf2_map_register(op - DW_OP_reg0);
            }
            loc->kind = loc_register;
            break;
        case DW_OP_breg0:  case DW_OP_breg1:  case DW_OP_breg2:  case DW_OP_breg3:
        case DW_OP_breg4:  case DW_OP_breg5:  case DW_OP_breg6:  case DW_OP_breg7:
        case DW_OP_breg8:  case DW_OP_breg9:  case DW_OP_breg10: case DW_OP_breg11:
        case DW_OP_breg12: case DW_OP_breg13: case DW_OP_breg14: case DW_OP_breg15:
        case DW_OP_breg16: case DW_OP_breg17: case DW_OP_breg18: case DW_OP_breg19:
        case DW_OP_breg20: case DW_OP_breg21: case DW_OP_breg22: case DW_OP_breg23:
        case DW_OP_breg24: case DW_OP_breg25: case DW_OP_breg26: case DW_OP_breg27:
        case DW_OP_breg28: case DW_OP_breg29: case DW_OP_breg30: case DW_OP_breg31:
            /* dbghelp APIs don't know how to cope with this anyway
             * (for example 'long long' stored in two registers)
             * FIXME: We should tell winedbg how to deal with it (sigh)
             */
            if (!piece_found)
            {
                if (loc->reg != Wine_DW_no_register)
                    FIXME("Only supporting one reg (%d -> %d)\n",
                          loc->reg, dwarf2_map_register(op - DW_OP_breg0));
                loc->reg = dwarf2_map_register(op - DW_OP_breg0);
            }
            stack[++stk] = dwarf2_leb128_as_signed(ctx);
            loc->kind = loc_regrel;
            break;
        case DW_OP_fbreg:
            if (loc->reg != Wine_DW_no_register)
                FIXME("Only supporting one reg (%d -> -2)\n", loc->reg);
            if (frame && frame->kind == loc_register)
            {
                loc->kind = loc_regrel;
                loc->reg = frame->reg;
                stack[++stk] = dwarf2_leb128_as_signed(ctx);
            }
            else if (frame && frame->kind == loc_regrel)
            {
                loc->kind = loc_regrel;
                loc->reg = frame->reg;
                stack[++stk] = dwarf2_leb128_as_signed(ctx) + frame->offset;
            }
            else
            {
                /* FIXME: this could be later optimized by not recomputing
                 * this very location expression
                 */
                loc->kind = loc_dwarf2_block;
                stack[++stk] = dwarf2_leb128_as_signed(ctx);
            }
            break;
        case DW_OP_piece:
            {
                unsigned sz = dwarf2_leb128_as_unsigned(ctx);
                WARN("Not handling OP_piece (size=%d)\n", sz);
                piece_found = TRUE;
            }
            break;
        case DW_OP_deref:
            if (!stk)
            {
                FIXME("Unexpected empty stack\n");
                return loc_err_internal;
            }
            if (loc->reg != Wine_DW_no_register)
            {
                WARN("Too complex expression for deref\n");
                return loc_err_too_complex;
            }
            if (hproc)
            {
                DWORD   addr = stack[stk--];
                DWORD   deref;

                if (!ReadProcessMemory(hproc, (void*)addr, &deref, sizeof(deref), NULL))
                {
                    WARN("Couldn't read memory at %x\n", addr);
                    return loc_err_cant_read;
                }
                stack[++stk] = deref;
            }
            else
            {
               loc->kind = loc_dwarf2_block;
            }
            break;
        default:
            FIXME("Unhandled attr op: %x\n", op);
            return loc_err_internal;
        }
    }
    loc->offset = stack[stk];
    return 0;
}

static BOOL dwarf2_compute_location_attr(dwarf2_parse_context_t* ctx,
                                         const dwarf2_debug_info_t* di,
                                         unsigned long dw,
                                         struct location* loc,
                                         const struct location* frame)
{
    struct attribute xloc;

    if (!dwarf2_find_attribute(ctx, di, dw, &xloc)) return FALSE;

    switch (xloc.form)
    {
    case DW_FORM_data1: case DW_FORM_data2:
    case DW_FORM_udata: case DW_FORM_sdata:
        loc->kind = loc_absolute;
        loc->reg = 0;
        loc->offset = xloc.u.uvalue;
        return TRUE;
    case DW_FORM_data4: case DW_FORM_data8:
        loc->kind = loc_dwarf2_location_list;
        loc->reg = Wine_DW_no_register;
        loc->offset = xloc.u.uvalue;
        return TRUE;
    }

    /* assume we have a block form */

    if (xloc.u.block.size)
    {
        dwarf2_traverse_context_t       lctx;
        enum location_error             err;

        lctx.data = xloc.u.block.ptr;
        lctx.end_data = xloc.u.block.ptr + xloc.u.block.size;
        lctx.word_size = ctx->word_size;

        err = compute_location(&lctx, loc, NULL, frame);
        if (err < 0)
        {
            loc->kind = loc_error;
            loc->reg = err;
        }
        else if (loc->kind == loc_dwarf2_block)
        {
            unsigned*   ptr = pool_alloc(&ctx->module->pool,
                                         sizeof(unsigned) + xloc.u.block.size);
            *ptr = xloc.u.block.size;
            memcpy(ptr + 1, xloc.u.block.ptr, xloc.u.block.size);
            loc->offset = (unsigned long)ptr;
        }
    }
    return TRUE;
}

static struct symt* dwarf2_lookup_type(dwarf2_parse_context_t* ctx,
                                       const dwarf2_debug_info_t* di)
{
    struct attribute    attr;

    if (dwarf2_find_attribute(ctx, di, DW_AT_type, &attr))
    {
        dwarf2_debug_info_t*    type;
        
        type = sparse_array_find(&ctx->debug_info_table, attr.u.uvalue);
        if (!type) FIXME("Unable to find back reference to type %lx\n", attr.u.uvalue);
        if (!type->symt)
        {
            /* load the debug info entity */
            dwarf2_load_one_entry(ctx, type, NULL);
        }
        return type->symt;
    }
    return NULL;
}

/******************************************************************
 *		dwarf2_read_one_debug_info
 *
 * Loads into memory one debug info entry, and recursively its children (if any)
 */
static BOOL dwarf2_read_one_debug_info(dwarf2_parse_context_t* ctx,
                                       dwarf2_traverse_context_t* traverse,
                                       dwarf2_debug_info_t** pdi)
{
    const dwarf2_abbrev_entry_t*abbrev;
    unsigned long               entry_code;
    unsigned long               offset;
    dwarf2_debug_info_t*        di;
    dwarf2_debug_info_t*        child;
    dwarf2_debug_info_t**       where;
    dwarf2_abbrev_entry_attr_t* attr;
    unsigned                    i;
    struct attribute            sibling;

    offset = traverse->data - ctx->sections[ctx->section].address;
    entry_code = dwarf2_leb128_as_unsigned(traverse);
    TRACE("found entry_code %lu at 0x%lx\n", entry_code, offset);
    if (!entry_code)
    {
        *pdi = NULL;
        return TRUE;
    }
    abbrev = dwarf2_abbrev_table_find_entry(&ctx->abbrev_table, entry_code);
    if (!abbrev)
    {
	WARN("Cannot find abbrev entry for %lu at 0x%lx\n", entry_code, offset);
	return FALSE;
    }
    di = sparse_array_add(&ctx->debug_info_table, offset, &ctx->pool);
    if (!di) return FALSE;
    di->abbrev = abbrev;
    di->symt   = NULL;

    if (abbrev->num_attr)
    {
        di->data = pool_alloc(&ctx->pool, abbrev->num_attr * sizeof(const char*));
        for (i = 0, attr = abbrev->attrs; attr; i++, attr = attr->next)
        {
            di->data[i] = traverse->data;
            dwarf2_swallow_attribute(traverse, attr);
        }
    }
    else di->data = NULL;
    if (abbrev->have_child)
    {
        vector_init(&di->children, sizeof(dwarf2_debug_info_t*), 16);
        while (traverse->data < traverse->end_data)
        {
            if (!dwarf2_read_one_debug_info(ctx, traverse, &child)) return FALSE;
            if (!child) break;
            where = vector_add(&di->children, &ctx->pool);
            if (!where) return FALSE;
            *where = child;
        }
    }
    if (dwarf2_find_attribute(ctx, di, DW_AT_sibling, &sibling) &&
        traverse->data != ctx->sections[ctx->section].address + sibling.u.uvalue)
    {
        WARN("setting cursor for %s to next sibling <0x%lx>\n",
             dwarf2_debug_traverse_ctx(traverse), sibling.u.uvalue);
        traverse->data = ctx->sections[ctx->section].address + sibling.u.uvalue;
    }
    *pdi = di;
    return TRUE;
}

static struct symt* dwarf2_parse_base_type(dwarf2_parse_context_t* ctx,
                                           dwarf2_debug_info_t* di)
{
    struct attribute name;
    struct attribute size;
    struct attribute encoding;
    enum BasicType bt;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di)); 

    if (!dwarf2_find_attribute(ctx, di, DW_AT_name, &name))
        name.u.string = NULL;
    if (!dwarf2_find_attribute(ctx, di, DW_AT_byte_size, &size)) size.u.uvalue = 0;
    if (!dwarf2_find_attribute(ctx, di, DW_AT_encoding, &encoding)) encoding.u.uvalue = DW_ATE_void;

    switch (encoding.u.uvalue)
    {
    case DW_ATE_void:           bt = btVoid; break;
    case DW_ATE_address:        bt = btULong; break;
    case DW_ATE_boolean:        bt = btBool; break;
    case DW_ATE_complex_float:  bt = btComplex; break;
    case DW_ATE_float:          bt = btFloat; break;
    case DW_ATE_signed:         bt = btInt; break;
    case DW_ATE_unsigned:       bt = btUInt; break;
    case DW_ATE_signed_char:    bt = btChar; break;
    case DW_ATE_unsigned_char:  bt = btChar; break;
    default:                    bt = btNoType; break;
    }
    di->symt = &symt_new_basic(ctx->module, bt, name.u.string, size.u.uvalue)->symt;
    if (di->abbrev->have_child) FIXME("Unsupported children\n");
    return di->symt;
}

static struct symt* dwarf2_parse_typedef(dwarf2_parse_context_t* ctx,
                                         dwarf2_debug_info_t* di)
{
    struct symt*        ref_type;
    struct attribute    name;

    if (di->symt) return di->symt;

    TRACE("%s, for %lu\n", dwarf2_debug_ctx(ctx), di->abbrev->entry_code); 

    if (!dwarf2_find_attribute(ctx, di, DW_AT_name, &name)) name.u.string = NULL;
    ref_type = dwarf2_lookup_type(ctx, di);

    if (name.u.string)
        di->symt = &symt_new_typedef(ctx->module, ref_type, name.u.string)->symt;
    if (di->abbrev->have_child) FIXME("Unsupported children\n");
    return di->symt;
}

static struct symt* dwarf2_parse_pointer_type(dwarf2_parse_context_t* ctx,
                                              dwarf2_debug_info_t* di)
{
    struct symt*        ref_type;
    struct attribute    size;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di)); 

    if (!dwarf2_find_attribute(ctx, di, DW_AT_byte_size, &size)) size.u.uvalue = 0;
    if (!(ref_type = dwarf2_lookup_type(ctx, di)))
        ref_type = &symt_new_basic(ctx->module, btVoid, "void", 0)->symt;

    di->symt = &symt_new_pointer(ctx->module, ref_type)->symt;
    if (di->abbrev->have_child) FIXME("Unsupported children\n");
    return di->symt;
}

static struct symt* dwarf2_parse_array_type(dwarf2_parse_context_t* ctx,
                                            dwarf2_debug_info_t* di)
{
    struct symt* ref_type;
    struct symt* idx_type = NULL;
    struct attribute min, max, cnt;
    dwarf2_debug_info_t* child;
    int    i;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    if (!di->abbrev->have_child)
    {
        FIXME("array without range information\n");
        return NULL;
    }
    ref_type = dwarf2_lookup_type(ctx, di);

    for (i=0; i<vector_length(&di->children); i++)
    {
        child = *(dwarf2_debug_info_t**)vector_at(&di->children, i);
        switch (child->abbrev->tag)
        {
        case DW_TAG_subrange_type:
            idx_type = dwarf2_lookup_type(ctx, child);
            if (!dwarf2_find_attribute(ctx, child, DW_AT_lower_bound, &min))
                min.u.uvalue = 0;
            if (!dwarf2_find_attribute(ctx, child, DW_AT_upper_bound, &max))
                max.u.uvalue = 0;
            if (dwarf2_find_attribute(ctx, child, DW_AT_count, &cnt))
                max.u.uvalue = min.u.uvalue + cnt.u.uvalue;
            break;
        default:
            FIXME("Unhandled Tag type 0x%lx at %s, for %s\n",
                  child->abbrev->tag, dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));
            break;
        }
    }
    di->symt = &symt_new_array(ctx->module, min.u.uvalue, max.u.uvalue, ref_type, idx_type)->symt;
    return di->symt;
}

static struct symt* dwarf2_parse_const_type(dwarf2_parse_context_t* ctx,
                                            dwarf2_debug_info_t* di)
{
    struct symt* ref_type;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di)); 

    ref_type = dwarf2_lookup_type(ctx, di);
    if (di->abbrev->have_child) FIXME("Unsupported children\n");
    di->symt = ref_type;

    return ref_type;
}

static struct symt* dwarf2_parse_volatile_type(dwarf2_parse_context_t* ctx,
                                               dwarf2_debug_info_t* di)
{
    struct symt* ref_type;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di)); 

    ref_type = dwarf2_lookup_type(ctx, di);
    if (di->abbrev->have_child) FIXME("Unsupported children\n");
    di->symt = ref_type;

    return ref_type;
}

static struct symt* dwarf2_parse_reference_type(dwarf2_parse_context_t* ctx,
                                                dwarf2_debug_info_t* di)
{
    struct symt* ref_type = NULL;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    ref_type = dwarf2_lookup_type(ctx, di);
    /* FIXME: for now, we hard-wire C++ references to pointers */
    di->symt = &symt_new_pointer(ctx->module, ref_type)->symt;

    if (di->abbrev->have_child) FIXME("Unsupported children\n");

    return di->symt;
}

static void dwarf2_parse_udt_member(dwarf2_parse_context_t* ctx,
                                    const dwarf2_debug_info_t* di,
                                    struct symt_udt* parent)
{
    struct symt* elt_type;
    struct attribute name;
    struct attribute bit_size;
    struct attribute bit_offset;
    struct location  loc;

    assert(parent);

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    if (!dwarf2_find_attribute(ctx, di, DW_AT_name, &name)) name.u.string = NULL;
    elt_type = dwarf2_lookup_type(ctx, di);
    if (dwarf2_compute_location_attr(ctx, di, DW_AT_data_member_location, &loc, NULL))
    {
        if (loc.kind != loc_absolute)
        {
           FIXME("Found register, while not expecting it\n");
           loc.offset = 0;
        }
        else
            TRACE("found member_location at %s -> %lu\n",
                  dwarf2_debug_ctx(ctx), loc.offset);
    }
    else
        loc.offset = 0;
    if (!dwarf2_find_attribute(ctx, di, DW_AT_bit_size, &bit_size))
        bit_size.u.uvalue = 0;
    if (dwarf2_find_attribute(ctx, di, DW_AT_bit_offset, &bit_offset))
    {
        /* FIXME: we should only do this when implementation is LSB (which is
         * the case on i386 processors)
         */
        struct attribute nbytes;
        if (!dwarf2_find_attribute(ctx, di, DW_AT_byte_size, &nbytes))
        {
            DWORD64     size;
            nbytes.u.uvalue = symt_get_info(elt_type, TI_GET_LENGTH, &size) ? (unsigned long)size : 0;
        }
        bit_offset.u.uvalue = nbytes.u.uvalue * 8 - bit_offset.u.uvalue - bit_size.u.uvalue;
    }
    else bit_offset.u.uvalue = 0;
    symt_add_udt_element(ctx->module, parent, name.u.string, elt_type,    
                         (loc.offset << 3) + bit_offset.u.uvalue,
                         bit_size.u.uvalue);

    if (di->abbrev->have_child) FIXME("Unsupported children\n");
}

static struct symt* dwarf2_parse_udt_type(dwarf2_parse_context_t* ctx,
                                          dwarf2_debug_info_t* di,
                                          enum UdtKind udt)
{
    struct attribute    name;
    struct attribute    size;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di)); 

    if (!dwarf2_find_attribute(ctx, di, DW_AT_name, &name)) name.u.string = NULL;
    if (!dwarf2_find_attribute(ctx, di, DW_AT_byte_size, &size)) size.u.uvalue = 0;

    di->symt = &symt_new_udt(ctx->module, name.u.string, size.u.uvalue, udt)->symt;

    if (di->abbrev->have_child) /** any interest to not have child ? */
    {
        dwarf2_debug_info_t*    child;
        int    i;

        for (i=0; i<vector_length(&di->children); i++)
        {
            child = *(dwarf2_debug_info_t**)vector_at(&di->children, i);

            switch (child->abbrev->tag)
            {
            case DW_TAG_member:
                /* FIXME: should I follow the sibling stuff ?? */
                dwarf2_parse_udt_member(ctx, child, (struct symt_udt*)di->symt);
                break;
            case DW_TAG_enumeration_type:
                dwarf2_parse_enumeration_type(ctx, child);
                break;
            case DW_TAG_structure_type:
            case DW_TAG_class_type:
            case DW_TAG_union_type:
                /* FIXME: we need to handle nested udt definitions */
                break;
            default:
                FIXME("Unhandled Tag type 0x%lx at %s, for %s\n",
                      child->abbrev->tag, dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));
                break;
            }
        }
    }

    return di->symt;
}

static void dwarf2_parse_enumerator(dwarf2_parse_context_t* ctx,
                                    const dwarf2_debug_info_t* di,
                                    struct symt_enum* parent)
{
    struct attribute    name;
    struct attribute    value;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di)); 

    if (!dwarf2_find_attribute(ctx, di, DW_AT_name, &name)) return;
    if (!dwarf2_find_attribute(ctx, di, DW_AT_const_value, &value)) value.u.svalue = 0;
    symt_add_enum_element(ctx->module, parent, name.u.string, value.u.svalue);

    if (di->abbrev->have_child) FIXME("Unsupported children\n");
}

static struct symt* dwarf2_parse_enumeration_type(dwarf2_parse_context_t* ctx,
                                                  dwarf2_debug_info_t* di)
{
    struct attribute    name;
    struct attribute    size;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di)); 

    if (!dwarf2_find_attribute(ctx, di, DW_AT_name, &name)) name.u.string = NULL;
    if (!dwarf2_find_attribute(ctx, di, DW_AT_byte_size, &size)) size.u.uvalue = 0;

    di->symt = &symt_new_enum(ctx->module, name.u.string)->symt;

    if (di->abbrev->have_child) /* any interest to not have child ? */
    {
        dwarf2_debug_info_t*    child;
        int    i;

        /* FIXME: should we use the sibling stuff ?? */
        for (i=0; i<vector_length(&di->children); i++)
        {
            child = *(dwarf2_debug_info_t**)vector_at(&di->children, i);

            switch (child->abbrev->tag)
            {
            case DW_TAG_enumerator:
                dwarf2_parse_enumerator(ctx, child, (struct symt_enum*)di->symt);
                break;
            default:
                FIXME("Unhandled Tag type 0x%lx at %s, for %s\n",
                      di->abbrev->tag, dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));
            }
	}
    }
    return di->symt;
}

/* structure used to pass information around when parsing a subprogram */
typedef struct dwarf2_subprogram_s
{
    dwarf2_parse_context_t*     ctx;
    struct symt_compiland*      compiland;
    struct symt_function*       func;
    BOOL                        non_computed_variable;
    struct location             frame;
} dwarf2_subprogram_t;

/******************************************************************
 *		dwarf2_parse_variable
 *
 * Parses any variable (parameter, local/global variable)
 */
static void dwarf2_parse_variable(dwarf2_subprogram_t* subpgm,
                                  struct symt_block* block,
                                  dwarf2_debug_info_t* di)
{
    struct symt*        param_type;
    struct attribute    name, value;
    struct location     loc;
    BOOL                is_pmt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(subpgm->ctx), dwarf2_debug_di(di));

    is_pmt = !block && di->abbrev->tag == DW_TAG_formal_parameter;
    param_type = dwarf2_lookup_type(subpgm->ctx, di);
        
    if (!dwarf2_find_attribute(subpgm->ctx, di, DW_AT_name, &name)) {
	/* cannot do much without the name, the functions below won't like it. */
        return;
    }
    if (dwarf2_compute_location_attr(subpgm->ctx, di, DW_AT_location,
                                     &loc, &subpgm->frame))
    {
        struct attribute ext;

	TRACE("found parameter %s (kind=%d, offset=%ld, reg=%d) at %s\n",
              name.u.string, loc.kind, loc.offset, loc.reg,
              dwarf2_debug_ctx(subpgm->ctx));

        switch (loc.kind)
        {
        case loc_absolute:
            /* it's a global variable */
            /* FIXME: we don't handle its scope yet */
            if (!dwarf2_find_attribute(subpgm->ctx, di, DW_AT_external, &ext))
                ext.u.uvalue = 0;
            symt_new_global_variable(subpgm->ctx->module, subpgm->compiland,
                                     name.u.string, !ext.u.uvalue,
                                     subpgm->ctx->load_offset + loc.offset,
                                     0, param_type);
            break;
        default:
            subpgm->non_computed_variable = TRUE;
            /* fall through */
        case loc_register:
        case loc_regrel:
            /* either a pmt/variable relative to frame pointer or
             * pmt/variable in a register
             */
            assert(subpgm->func);
            symt_add_func_local(subpgm->ctx->module, subpgm->func, 
                                is_pmt ? DataIsParam : DataIsLocal,
                                &loc, block, param_type, name.u.string);
            break;
        }
    }
    else if (dwarf2_find_attribute(subpgm->ctx, di, DW_AT_const_value, &value))
    {
        VARIANT v;
        if (subpgm->func) WARN("Unsupported constant %s in function\n", name.u.string);
        if (is_pmt)       FIXME("Unsupported constant (parameter) %s in function\n", name.u.string);
        switch (value.form)
        {
        case DW_FORM_data1:
        case DW_FORM_data2:
        case DW_FORM_data4:
        case DW_FORM_udata:
            v.n1.n2.vt = VT_UI4;
            v.n1.n2.n3.lVal = value.u.uvalue;
            break;

        case DW_FORM_sdata:
            v.n1.n2.vt = VT_I4;
            v.n1.n2.n3.lVal = value.u.svalue;
            break;

        case DW_FORM_strp:
        case DW_FORM_string:
            /* FIXME: native doesn't report const strings from here !!
             * however, the value of the string is in the code somewhere
             */
            v.n1.n2.vt = VT_I1 | VT_BYREF;
            v.n1.n2.n3.byref = pool_strdup(&subpgm->ctx->module->pool, value.u.string);
            break;

        case DW_FORM_data8:
        case DW_FORM_block:
        case DW_FORM_block1:
        case DW_FORM_block2:
        case DW_FORM_block4:

        default:
            FIXME("Unsupported form for const value %s (%lx)\n",
                  name.u.string, value.form);
            v.n1.n2.vt = VT_EMPTY;
        }
        di->symt = &symt_new_constant(subpgm->ctx->module, subpgm->compiland,
                                      name.u.string, param_type, &v)->symt;
    }
    if (is_pmt && subpgm->func && subpgm->func->type)
        symt_add_function_signature_parameter(subpgm->ctx->module,
                                              (struct symt_function_signature*)subpgm->func->type,
                                              param_type);

    if (di->abbrev->have_child) FIXME("Unsupported children\n");
}

static void dwarf2_parse_subprogram_label(dwarf2_subprogram_t* subpgm,
                                          const dwarf2_debug_info_t* di)
{
    struct attribute    name;
    struct attribute    low_pc;
    struct location     loc;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(subpgm->ctx), dwarf2_debug_di(di));

    if (!dwarf2_find_attribute(subpgm->ctx, di, DW_AT_low_pc, &low_pc)) low_pc.u.uvalue = 0;
    if (!dwarf2_find_attribute(subpgm->ctx, di, DW_AT_name, &name))
        name.u.string = NULL;

    loc.kind = loc_absolute;
    loc.offset = subpgm->ctx->load_offset + low_pc.u.uvalue;
    symt_add_function_point(subpgm->ctx->module, subpgm->func, SymTagLabel,
                            &loc, name.u.string);
}

static void dwarf2_parse_subprogram_block(dwarf2_subprogram_t* subpgm,
                                          struct symt_block* parent_block,
					  const dwarf2_debug_info_t* di);

static void dwarf2_parse_inlined_subroutine(dwarf2_subprogram_t* subpgm,
                                            struct symt_block* parent_block,
                                            const dwarf2_debug_info_t* di)
{
    struct symt_block*  block;
    struct attribute    low_pc;
    struct attribute    high_pc;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(subpgm->ctx), dwarf2_debug_di(di));

    if (!dwarf2_find_attribute(subpgm->ctx, di, DW_AT_low_pc, &low_pc)) low_pc.u.uvalue = 0;
    if (!dwarf2_find_attribute(subpgm->ctx, di, DW_AT_high_pc, &high_pc)) high_pc.u.uvalue = 0;

    block = symt_open_func_block(subpgm->ctx->module, subpgm->func, parent_block,
                                 subpgm->ctx->load_offset + low_pc.u.uvalue - subpgm->func->address,
                                 high_pc.u.uvalue - low_pc.u.uvalue);

    if (di->abbrev->have_child) /** any interest to not have child ? */
    {
        dwarf2_debug_info_t*    child;
        int    i;

        for (i=0; i<vector_length(&di->children); i++)
        {
            child = *(dwarf2_debug_info_t**)vector_at(&di->children, i);

            switch (child->abbrev->tag)
            {
            case DW_TAG_formal_parameter:
            case DW_TAG_variable:
                dwarf2_parse_variable(subpgm, block, child);
                break;
            case DW_TAG_lexical_block:
                dwarf2_parse_subprogram_block(subpgm, block, child);
                break;
            case DW_TAG_inlined_subroutine:
                dwarf2_parse_inlined_subroutine(subpgm, block, child);
                break;
            case DW_TAG_label:
                dwarf2_parse_subprogram_label(subpgm, child);
                break;
            default:
                FIXME("Unhandled Tag type 0x%lx at %s, for %s\n",
                      child->abbrev->tag, dwarf2_debug_ctx(subpgm->ctx),
                      dwarf2_debug_di(di));
            }
        }
    }
    symt_close_func_block(subpgm->ctx->module, subpgm->func, block, 0);
}

static void dwarf2_parse_subprogram_block(dwarf2_subprogram_t* subpgm, 
                                          struct symt_block* parent_block,
					  const dwarf2_debug_info_t* di)
{
    struct symt_block*  block;
    struct attribute    low_pc;
    struct attribute    high_pc;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(subpgm->ctx), dwarf2_debug_di(di));

    if (!dwarf2_find_attribute(subpgm->ctx, di, DW_AT_low_pc, &low_pc))
        low_pc.u.uvalue = 0;
    if (!dwarf2_find_attribute(subpgm->ctx, di, DW_AT_high_pc, &high_pc))
        high_pc.u.uvalue = 0;

    block = symt_open_func_block(subpgm->ctx->module, subpgm->func, parent_block,
                                 subpgm->ctx->load_offset + low_pc.u.uvalue - subpgm->func->address,
                                 high_pc.u.uvalue - low_pc.u.uvalue);

    if (di->abbrev->have_child) /** any interest to not have child ? */
    {
        dwarf2_debug_info_t*    child;
        int    i;

        for (i=0; i<vector_length(&di->children); i++)
        {
            child = *(dwarf2_debug_info_t**)vector_at(&di->children, i);

            switch (child->abbrev->tag)
            {
            case DW_TAG_inlined_subroutine:
                dwarf2_parse_inlined_subroutine(subpgm, block, child);
                break;
            case DW_TAG_variable:
                dwarf2_parse_variable(subpgm, block, child);
                break;
            case DW_TAG_lexical_block:
                dwarf2_parse_subprogram_block(subpgm, block, child);
                break;
            case DW_TAG_subprogram:
                /* FIXME: likely a declaration (to be checked)
                 * skip it for now
                 */
                break;
            case DW_TAG_formal_parameter:
                /* FIXME: likely elements for exception handling (GCC flavor)
                 * Skip it for now
                 */
                break;
            case DW_TAG_label:
                dwarf2_parse_subprogram_label(subpgm, child);
                break;
            case DW_TAG_class_type:
            case DW_TAG_structure_type:
            case DW_TAG_union_type:
            case DW_TAG_enumeration_type:
            case DW_TAG_typedef:
                /* the type referred to will be loaded when we need it, so skip it */
                break;
            default:
                FIXME("Unhandled Tag type 0x%lx at %s, for %s\n",
                      child->abbrev->tag, dwarf2_debug_ctx(subpgm->ctx), dwarf2_debug_di(di));
            }
        }
    }

    symt_close_func_block(subpgm->ctx->module, subpgm->func, block, 0);
}

static struct symt* dwarf2_parse_subprogram(dwarf2_parse_context_t* ctx,
                                            dwarf2_debug_info_t* di,
                                            struct symt_compiland* compiland)
{
    struct attribute name;
    struct attribute low_pc;
    struct attribute high_pc;
    struct attribute is_decl;
    struct attribute inline_flags;
    struct symt* ret_type;
    struct symt_function_signature* sig_type;
    dwarf2_subprogram_t subpgm;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    if (!dwarf2_find_attribute(ctx, di, DW_AT_name, &name)) name.u.string = NULL;
    /* if it's an abstract representation of an inline function, there should be
     * a concrete object that we'll handle
     */
    if (dwarf2_find_attribute(ctx, di, DW_AT_inline, &inline_flags))
    {
        TRACE("Function %s declared as inlined (%ld)... skipping\n",
              name.u.string ? name.u.string : "(null)", inline_flags.u.uvalue);
        return NULL;
    }

    if (!dwarf2_find_attribute(ctx, di, DW_AT_low_pc, &low_pc)) low_pc.u.uvalue = 0;
    if (!dwarf2_find_attribute(ctx, di, DW_AT_high_pc, &high_pc)) high_pc.u.uvalue = 0;
    /* As functions (defined as inline assembly) get debug info with dwarf
     * (not the case for stabs), we just drop Wine's thunks here...
     * Actual thunks will be created in elf_module from the symbol table
     */
    if (elf_is_in_thunk_area(ctx->load_offset + low_pc.u.uvalue,
                             ctx->thunks) >= 0)
        return NULL;
    if (!dwarf2_find_attribute(ctx, di, DW_AT_declaration, &is_decl))
        is_decl.u.uvalue = 0;
        
    if (!(ret_type = dwarf2_lookup_type(ctx, di)))
        ret_type = &symt_new_basic(ctx->module, btVoid, "void", 0)->symt;

    /* FIXME: assuming C source code */
    sig_type = symt_new_function_signature(ctx->module, ret_type, CV_CALL_FAR_C);
    if (!is_decl.u.uvalue)
    {
        subpgm.func = symt_new_function(ctx->module, compiland, name.u.string,
                                        ctx->load_offset + low_pc.u.uvalue,
                                        high_pc.u.uvalue - low_pc.u.uvalue,
                                        &sig_type->symt);
        di->symt = &subpgm.func->symt;
    }
    else subpgm.func = NULL;

    subpgm.ctx = ctx;
    subpgm.compiland = compiland;
    if (!dwarf2_compute_location_attr(ctx, di, DW_AT_frame_base,
                                      &subpgm.frame, NULL))
    {
        /* on stack !! */
        subpgm.frame.kind = loc_regrel;
        subpgm.frame.reg = 0;
        subpgm.frame.offset = 0;
    }
    subpgm.non_computed_variable = FALSE;

    if (di->abbrev->have_child) /** any interest to not have child ? */
    {
        dwarf2_debug_info_t*    child;
        int    i;

        for (i=0; i<vector_length(&di->children); i++)
        {
            child = *(dwarf2_debug_info_t**)vector_at(&di->children, i);

            switch (child->abbrev->tag)
            {
            case DW_TAG_variable:
            case DW_TAG_formal_parameter:
                dwarf2_parse_variable(&subpgm, NULL, child);
                break;
            case DW_TAG_lexical_block:
                dwarf2_parse_subprogram_block(&subpgm, NULL, child);
                break;
            case DW_TAG_inlined_subroutine:
                dwarf2_parse_inlined_subroutine(&subpgm, NULL, child);
                break;
            case DW_TAG_subprogram:
                /* FIXME: likely a declaration (to be checked)
                 * skip it for now
                 */
                break;
            case DW_TAG_label:
                dwarf2_parse_subprogram_label(&subpgm, child);
                break;
            case DW_TAG_class_type:
            case DW_TAG_structure_type:
            case DW_TAG_union_type:
            case DW_TAG_enumeration_type:
            case DW_TAG_typedef:
                /* the type referred to will be loaded when we need it, so skip it */
                break;
            case DW_TAG_unspecified_parameters:
                /* FIXME: no support in dbghelp's internals so far */
                break;
            default:
                FIXME("Unhandled Tag type 0x%lx at %s, for %s\n",
                      child->abbrev->tag, dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));
            }
	}
    }

    if (subpgm.non_computed_variable || subpgm.frame.kind >= loc_user)
    {
        symt_add_function_point(ctx->module, subpgm.func, SymTagCustom,
                                &subpgm.frame, NULL);
    }
    symt_normalize_function(subpgm.ctx->module, subpgm.func);

    return di->symt;
}

static struct symt* dwarf2_parse_subroutine_type(dwarf2_parse_context_t* ctx,
                                                 dwarf2_debug_info_t* di)
{
    struct symt* ret_type;
    struct symt_function_signature* sig_type;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    if (!(ret_type = dwarf2_lookup_type(ctx, di)))
        ret_type = &symt_new_basic(ctx->module, btVoid, "void", 0)->symt;

    /* FIXME: assuming C source code */
    sig_type = symt_new_function_signature(ctx->module, ret_type, CV_CALL_FAR_C);

    if (di->abbrev->have_child) /** any interest to not have child ? */
    {
        dwarf2_debug_info_t*    child;
        int    i;

        for (i=0; i<vector_length(&di->children); i++)
        {
            child = *(dwarf2_debug_info_t**)vector_at(&di->children, i);

            switch (child->abbrev->tag)
            {
            case DW_TAG_formal_parameter:
                symt_add_function_signature_parameter(ctx->module, sig_type,
                                                      dwarf2_lookup_type(ctx, child));
                break;
            case DW_TAG_unspecified_parameters:
                WARN("Unsupported unspecified parameters\n");
                break;
            }
	}
    }

    return di->symt = &sig_type->symt;
}

static void dwarf2_load_one_entry(dwarf2_parse_context_t* ctx,
                                  dwarf2_debug_info_t* di,
                                  struct symt_compiland* compiland)
{
    switch (di->abbrev->tag)
    {
    case DW_TAG_typedef:
        dwarf2_parse_typedef(ctx, di);
        break;
    case DW_TAG_base_type:
        dwarf2_parse_base_type(ctx, di);
        break;
    case DW_TAG_pointer_type:
        dwarf2_parse_pointer_type(ctx, di);
        break;
    case DW_TAG_class_type:
        dwarf2_parse_udt_type(ctx, di, UdtClass);
        break;
    case DW_TAG_structure_type:
        dwarf2_parse_udt_type(ctx, di, UdtStruct);
        break;
    case DW_TAG_union_type:
        dwarf2_parse_udt_type(ctx, di, UdtUnion);
        break;
    case DW_TAG_array_type:
        dwarf2_parse_array_type(ctx, di);
        break;
    case DW_TAG_const_type:
        dwarf2_parse_const_type(ctx, di);
        break;
    case DW_TAG_volatile_type:
        dwarf2_parse_volatile_type(ctx, di);
        break;
    case DW_TAG_reference_type:
        dwarf2_parse_reference_type(ctx, di);
        break;
    case DW_TAG_enumeration_type:
        dwarf2_parse_enumeration_type(ctx, di);
        break;
    case DW_TAG_subprogram:
        dwarf2_parse_subprogram(ctx, di, compiland);
        break;
    case DW_TAG_subroutine_type:
        dwarf2_parse_subroutine_type(ctx, di);
        break;
    case DW_TAG_variable:
        {
            dwarf2_subprogram_t subpgm;

            subpgm.ctx = ctx;
            subpgm.compiland = compiland;
            subpgm.func = NULL;
            subpgm.frame.kind = loc_absolute;
            subpgm.frame.offset = 0;
            subpgm.frame.reg = Wine_DW_no_register;
            dwarf2_parse_variable(&subpgm, NULL, di);
        }
        break;
    default:
        FIXME("Unhandled Tag type 0x%lx at %s, for %lu\n",
              di->abbrev->tag, dwarf2_debug_ctx(ctx), di->abbrev->entry_code); 
    }
}

static void dwarf2_set_line_number(struct module* module, unsigned long address,
                                   const struct vector* v, unsigned file, unsigned line)
{
    struct symt_function*       func;
    struct symt_ht*             symt;
    unsigned*                   psrc;

    if (!file || !(psrc = vector_at(v, file - 1))) return;

    TRACE("%s %lx %s %u\n",
          debugstr_w(module->module.ModuleName), address, source_get(module, *psrc), line);
    if (!(symt = symt_find_nearest(module, address)) ||
        symt->symt.tag != SymTagFunction) return;
    func = (struct symt_function*)symt;
    symt_add_func_line(module, func, *psrc, line, address - func->address);
}

static BOOL dwarf2_parse_line_numbers(const dwarf2_section_t* sections,
                                      dwarf2_parse_context_t* ctx,
                                      const char* compile_dir,
                                      unsigned long offset)
{
    dwarf2_traverse_context_t   traverse;
    unsigned long               length;
    unsigned                    version, header_len, insn_size, default_stmt;
    unsigned                    line_range, opcode_base;
    int                         line_base;
    const unsigned char*        opcode_len;
    struct vector               dirs;
    struct vector               files;
    const char**                p;

    /* section with line numbers stripped */
    if (sections[section_line].address == ELF_NO_MAP)
        return FALSE;

    traverse.data = sections[section_line].address + offset;
    traverse.start_data = traverse.data;
    traverse.end_data = traverse.data + 4;
    traverse.word_size = ctx->word_size;

    length = dwarf2_parse_u4(&traverse);
    traverse.end_data = traverse.start_data + length;

    version = dwarf2_parse_u2(&traverse);
    header_len = dwarf2_parse_u4(&traverse);
    insn_size = dwarf2_parse_byte(&traverse);
    default_stmt = dwarf2_parse_byte(&traverse);
    line_base = (signed char)dwarf2_parse_byte(&traverse);
    line_range = dwarf2_parse_byte(&traverse);
    opcode_base = dwarf2_parse_byte(&traverse);

    opcode_len = traverse.data;
    traverse.data += opcode_base - 1;

    vector_init(&dirs, sizeof(const char*), 4);
    p = vector_add(&dirs, &ctx->pool);
    *p = compile_dir ? compile_dir : ".";
    while (*traverse.data)
    {
        const char*  rel = (const char*)traverse.data;
        unsigned     rellen = strlen(rel);
        TRACE("Got include %s\n", rel);
        traverse.data += rellen + 1;
        p = vector_add(&dirs, &ctx->pool);

        if (*rel == '/' || !compile_dir)
            *p = rel;
        else
        {
           /* include directory relative to compile directory */
           unsigned  baselen = strlen(compile_dir);
           char*     tmp = pool_alloc(&ctx->pool, baselen + 1 + rellen + 1);
           strcpy(tmp, compile_dir);
           if (tmp[baselen - 1] != '/') tmp[baselen++] = '/';
           strcpy(&tmp[baselen], rel);
           *p = tmp;
        }

    }
    traverse.data++;

    vector_init(&files, sizeof(unsigned), 16);
    while (*traverse.data)
    {
        unsigned int    dir_index, mod_time, length;
        const char*     name;
        const char*     dir;
        unsigned*       psrc;

        name = (const char*)traverse.data;
        traverse.data += strlen(name) + 1;
        dir_index = dwarf2_leb128_as_unsigned(&traverse);
        mod_time = dwarf2_leb128_as_unsigned(&traverse);
        length = dwarf2_leb128_as_unsigned(&traverse);
        dir = *(const char**)vector_at(&dirs, dir_index);
        TRACE("Got file %s/%s (%u,%u)\n", dir, name, mod_time, length);
        psrc = vector_add(&files, &ctx->pool);
        *psrc = source_new(ctx->module, dir, name);
    }
    traverse.data++;

    while (traverse.data < traverse.end_data)
    {
        unsigned long address = 0;
        unsigned file = 1;
        unsigned line = 1;
        unsigned is_stmt = default_stmt;
        BOOL basic_block = FALSE, end_sequence = FALSE;
        unsigned opcode, extopcode, i;

        while (!end_sequence)
        {
            opcode = dwarf2_parse_byte(&traverse);
            TRACE("Got opcode %x\n", opcode);

            if (opcode >= opcode_base)
            {
                unsigned delta = opcode - opcode_base;

                address += (delta / line_range) * insn_size;
                line += line_base + (delta % line_range);
                basic_block = TRUE;
                dwarf2_set_line_number(ctx->module, address, &files, file, line);
            }
            else
            {
                switch (opcode)
                {
                case DW_LNS_copy:
                    basic_block = FALSE;
                    dwarf2_set_line_number(ctx->module, address, &files, file, line);
                    break;
                case DW_LNS_advance_pc:
                    address += insn_size * dwarf2_leb128_as_unsigned(&traverse);
                    break;
                case DW_LNS_advance_line:
                    line += dwarf2_leb128_as_signed(&traverse);
                    break;
                case DW_LNS_set_file:
                    file = dwarf2_leb128_as_unsigned(&traverse);
                    break;
                case DW_LNS_set_column:
                    dwarf2_leb128_as_unsigned(&traverse);
                    break;
                case DW_LNS_negate_stmt:
                    is_stmt = !is_stmt;
                    break;
                case DW_LNS_set_basic_block:
                    basic_block = 1;
                    break;
                case DW_LNS_const_add_pc:
                    address += ((255 - opcode_base) / line_range) * insn_size;
                    break;
                case DW_LNS_fixed_advance_pc:
                    address += dwarf2_parse_u2(&traverse);
                    break;
                case DW_LNS_extended_op:
                    dwarf2_leb128_as_unsigned(&traverse);
                    extopcode = dwarf2_parse_byte(&traverse);
                    switch (extopcode)
                    {
                    case DW_LNE_end_sequence:
                        dwarf2_set_line_number(ctx->module, address, &files, file, line);
                        end_sequence = TRUE;
                        break;
                    case DW_LNE_set_address:
                        address = ctx->load_offset + dwarf2_parse_addr(&traverse);
                        break;
                    case DW_LNE_define_file:
                        FIXME("not handled %s\n", traverse.data);
                        traverse.data += strlen((const char *)traverse.data) + 1;
                        dwarf2_leb128_as_unsigned(&traverse);
                        dwarf2_leb128_as_unsigned(&traverse);
                        dwarf2_leb128_as_unsigned(&traverse);
                        break;
                    default:
                        FIXME("Unsupported extended opcode %x\n", extopcode);
                        break;
                    }
                    break;
                default:
                    WARN("Unsupported opcode %x\n", opcode);
                    for (i = 0; i < opcode_len[opcode]; i++)
                        dwarf2_leb128_as_unsigned(&traverse);
                    break;
                }
            }
        }
    }
    return TRUE;
}

static BOOL dwarf2_parse_compilation_unit(const dwarf2_section_t* sections,
                                          const dwarf2_comp_unit_t* comp_unit,
                                          struct module* module,
                                          const struct elf_thunk_area* thunks,
                                          const unsigned char* comp_unit_cursor,
                                          unsigned long load_offset)
{
    dwarf2_parse_context_t ctx;
    dwarf2_traverse_context_t traverse;
    dwarf2_traverse_context_t abbrev_ctx;
    dwarf2_debug_info_t* di;
    BOOL ret = FALSE;

    TRACE("Compilation Unit Header found at 0x%x:\n",
          comp_unit_cursor - sections[section_debug].address);
    TRACE("- length:        %lu\n", comp_unit->length);
    TRACE("- version:       %u\n",  comp_unit->version);
    TRACE("- abbrev_offset: %lu\n", comp_unit->abbrev_offset);
    TRACE("- word_size:     %u\n",  comp_unit->word_size);

    if (comp_unit->version != 2)
    {
        WARN("%u DWARF version unsupported. Wine dbghelp only support DWARF 2.\n",
             comp_unit->version);
        return FALSE;
    }

    pool_init(&ctx.pool, 65536);
    ctx.sections = sections;
    ctx.section = section_debug;
    ctx.module = module;
    ctx.word_size = comp_unit->word_size;
    ctx.thunks = thunks;
    ctx.load_offset = load_offset;
    ctx.ref_offset = comp_unit_cursor - sections[section_debug].address;

    traverse.start_data = comp_unit_cursor + sizeof(dwarf2_comp_unit_stream_t);
    traverse.data = traverse.start_data;
    traverse.word_size = comp_unit->word_size;
    traverse.end_data = comp_unit_cursor + comp_unit->length + sizeof(unsigned);

    abbrev_ctx.start_data = sections[section_abbrev].address + comp_unit->abbrev_offset;
    abbrev_ctx.data = abbrev_ctx.start_data;
    abbrev_ctx.end_data = sections[section_abbrev].address + sections[section_abbrev].size;
    abbrev_ctx.word_size = comp_unit->word_size;
    dwarf2_parse_abbrev_set(&abbrev_ctx, &ctx.abbrev_table, &ctx.pool);

    sparse_array_init(&ctx.debug_info_table, sizeof(dwarf2_debug_info_t), 128);
    dwarf2_read_one_debug_info(&ctx, &traverse, &di);

    if (di->abbrev->tag == DW_TAG_compile_unit)
    {
        struct attribute            name;
        dwarf2_debug_info_t**       pdi = NULL;
        struct attribute            stmt_list, low_pc;
        struct attribute            comp_dir;

        if (!dwarf2_find_attribute(&ctx, di, DW_AT_name, &name))
            name.u.string = NULL;

        /* get working directory of current compilation unit */
        if (!dwarf2_find_attribute(&ctx, di, DW_AT_comp_dir, &comp_dir))
            comp_dir.u.string = NULL;

        if (!dwarf2_find_attribute(&ctx, di, DW_AT_low_pc, &low_pc))
            low_pc.u.uvalue = 0;
        di->symt = &symt_new_compiland(module, 
                                       ctx.load_offset + low_pc.u.uvalue,
                                       source_new(module, comp_dir.u.string, name.u.string))->symt;

        if (di->abbrev->have_child)
        {
            int    i;
            for (i=0; i<vector_length(&di->children); i++)
            {
                pdi = vector_at(&di->children, i);
                dwarf2_load_one_entry(&ctx, *pdi, (struct symt_compiland*)di->symt);
            }
        }
        if (dwarf2_find_attribute(&ctx, di, DW_AT_stmt_list, &stmt_list))
        {
            if (dwarf2_parse_line_numbers(sections, &ctx, comp_dir.u.string, stmt_list.u.uvalue))
                module->module.LineNumbers = TRUE;
        }
        ret = TRUE;
    }
    else FIXME("Should have a compilation unit here\n");
    pool_destroy(&ctx.pool);
    return ret;
}

static BOOL dwarf2_lookup_loclist(const struct module* module, const BYTE* start,
                                  unsigned long ip,
                                  dwarf2_traverse_context_t*  lctx)
{
    DWORD                       beg, end;
    const BYTE*                 ptr = start;
    DWORD                       len;

    while (ptr < module->dwarf2_info->debug_loc.address + module->dwarf2_info->debug_loc.size)
    {
        beg = dwarf2_get_u4(ptr); ptr += 4;
        end = dwarf2_get_u4(ptr); ptr += 4;
        if (!beg && !end) break;
        len = dwarf2_get_u2(ptr); ptr += 2;

        if (beg <= ip && ip < end)
        {
            lctx->data = ptr;
            lctx->end_data = ptr + len;
            lctx->word_size = 4; /* FIXME word size !!! */
            return TRUE;
        }
        ptr += len;
    }
    WARN("Couldn't find ip in location list\n");
    return FALSE;
}

static enum location_error loc_compute_frame(struct process* pcs,
                                             const struct module* module,
                                             const struct symt_function* func,
                                             DWORD ip, struct location* frame)
{
    struct symt**               psym = NULL;
    struct location*            pframe;
    dwarf2_traverse_context_t   lctx;
    enum location_error         err;
    int                         i;

    for (i=0; i<vector_length(&func->vchildren); i++)
    {
        psym = vector_at(&func->vchildren, i);
        if ((*psym)->tag == SymTagCustom)
        {
            pframe = &((struct symt_function_point*)*psym)->loc;

            /* First, recompute the frame information, if needed */
            switch (pframe->kind)
            {
            case loc_regrel:
            case loc_register:
                *frame = *pframe;
                break;
            case loc_dwarf2_location_list:
                WARN("Searching loclist for %s\n", func->hash_elt.name);
                if (!dwarf2_lookup_loclist(module, 
                                           module->dwarf2_info->debug_loc.address + pframe->offset,
                                           ip, &lctx))
                    return loc_err_out_of_scope;
                if ((err = compute_location(&lctx, frame, pcs->handle, NULL)) < 0) return err;
                if (frame->kind >= loc_user)
                {
                    WARN("Couldn't compute runtime frame location\n");
                    return loc_err_too_complex;
                }
                break;
            default:
                WARN("Unsupported frame kind %d\n", pframe->kind);
                return loc_err_internal;
            }
            return 0;
        }
    }
    WARN("Couldn't find Custom function point, whilst location list offset is searched\n");
    return loc_err_internal;
}

static void dwarf2_location_compute(struct process* pcs,
                                    const struct module* module,
                                    const struct symt_function* func,
                                    struct location* loc)
{
    struct location             frame;
    DWORD                       ip;
    int                         err;
    dwarf2_traverse_context_t   lctx;

    if (!func->container || func->container->tag != SymTagCompiland)
    {
        WARN("We'd expect function %s's container to exist and be a compiland\n", func->hash_elt.name);
        err = loc_err_internal;
    }
    else
    {
        /* instruction pointer relative to compiland's start */
        ip = pcs->ctx_frame.InstructionOffset - ((struct symt_compiland*)func->container)->address;

        if ((err = loc_compute_frame(pcs, module, func, ip, &frame)) == 0)
        {
            switch (loc->kind)
            {
            case loc_dwarf2_location_list:
                /* Then, if the variable has a location list, find it !! */
                if (dwarf2_lookup_loclist(module, 
                                          module->dwarf2_info->debug_loc.address + loc->offset,
                                          ip, &lctx))
                    goto do_compute;
                err = loc_err_out_of_scope;
                break;
            case loc_dwarf2_block:
                /* or if we have a copy of an existing block, get ready for it */
                {
                    unsigned*   ptr = (unsigned*)loc->offset;

                    lctx.data = (const BYTE*)(ptr + 1);
                    lctx.end_data = lctx.data + *ptr;
                    lctx.word_size = 4; /* FIXME !! */
                }
            do_compute:
                /* now get the variable */
                err = compute_location(&lctx, loc, pcs->handle, &frame);
                break;
            case loc_register:
            case loc_regrel:
                /* nothing to do */
                break;
            default:
                WARN("Unsupported local kind %d\n", loc->kind);
                err = loc_err_internal;
            }
        }
    }
    if (err < 0)
    {
        loc->kind = loc_register;
        loc->reg = err;
    }
}

BOOL dwarf2_parse(struct module* module, unsigned long load_offset,
                  const struct elf_thunk_area* thunks,
		  const unsigned char* debug, unsigned int debug_size,
		  const unsigned char* abbrev, unsigned int abbrev_size,
		  const unsigned char* str, unsigned int str_size,
		  const unsigned char* line, unsigned int line_size,
		  const unsigned char* loclist, unsigned int loclist_size)
{
    dwarf2_section_t    section[section_max];
    unsigned char*      ptr;
    const unsigned char*comp_unit_cursor = debug;
    const unsigned char*end_debug = debug + debug_size;

    module->loc_compute = dwarf2_location_compute;

    section[section_debug].address = debug;
    section[section_debug].size = debug_size;
    section[section_abbrev].address = abbrev;
    section[section_abbrev].size = abbrev_size;
    section[section_string].address = str;
    section[section_string].size = str_size;
    section[section_line].address = line;
    section[section_line].size = line_size;

    if (loclist_size)
    {
        /* initialize the dwarf2 specific info block for this module.
         * As we'll need later on the .debug_loc section content, we copy it in
         * the module structure for later reuse
         */
        module->dwarf2_info = HeapAlloc(GetProcessHeap(), 0, sizeof(*module->dwarf2_info) + loclist_size);
        if (!module->dwarf2_info) return FALSE;
        ptr = (unsigned char*)(module->dwarf2_info + 1);
        memcpy(ptr, loclist, loclist_size);
        module->dwarf2_info->debug_loc.address = ptr;
        module->dwarf2_info->debug_loc.size    = loclist_size;
    }

    while (comp_unit_cursor < end_debug)
    {
        const dwarf2_comp_unit_stream_t* comp_unit_stream;
        dwarf2_comp_unit_t comp_unit;
    
        comp_unit_stream = (const dwarf2_comp_unit_stream_t*) comp_unit_cursor;
        comp_unit.length = *(const unsigned long*)  comp_unit_stream->length;
        comp_unit.version = *(const unsigned short*) comp_unit_stream->version;
        comp_unit.abbrev_offset = *(const unsigned long*) comp_unit_stream->abbrev_offset;
        comp_unit.word_size = *(const unsigned char*) comp_unit_stream->word_size;

        dwarf2_parse_compilation_unit(section, &comp_unit, module,
                                      thunks, comp_unit_cursor, load_offset);
        comp_unit_cursor += comp_unit.length + sizeof(unsigned);
    }
    module->module.SymType = SymDia;
    module->module.CVSig = 'D' | ('W' << 8) | ('A' << 16) | ('R' << 24);
    /* FIXME: we could have a finer grain here */
    module->module.GlobalSymbols = TRUE;
    module->module.TypeInfo = TRUE;
    module->module.SourceIndexed = TRUE;
    module->module.Publics = TRUE;
    return TRUE;
}
