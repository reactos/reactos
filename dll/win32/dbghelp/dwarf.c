/*
 * File dwarf.c - read dwarf2 information from the ELF modules
 *
 * Copyright (C) 2005, Raphael Junqueira
 * Copyright (C) 2006-2011, Eric Pouech
 * Copyright (C) 2010, Alexandre Julliard
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

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

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
    const       BYTE* x = ptr;

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
    enum {attr_direct, attr_abstract_origin, attr_specification} gotten_from;
    union
    {
        unsigned long                   uvalue;
        ULONGLONG                       lluvalue;
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
    struct dwarf2_debug_info_s* parent;
} dwarf2_debug_info_t;

typedef struct dwarf2_section_s
{
    BOOL                        compressed;
    const unsigned char*        address;
    unsigned                    size;
    DWORD_PTR                   rva;
} dwarf2_section_t;

enum dwarf2_sections {section_debug, section_string, section_abbrev, section_line, section_ranges, section_max};

typedef struct dwarf2_traverse_context_s
{
    const unsigned char*        data;
    const unsigned char*        end_data;
    unsigned char               word_size;
} dwarf2_traverse_context_t;

/* symt_cache indexes */
#define sc_void 0
#define sc_int1 1
#define sc_int2 2
#define sc_int4 3
#define sc_num  4

typedef struct dwarf2_parse_context_s
{
    const dwarf2_section_t*     sections;
    unsigned                    section;
    struct pool                 pool;
    struct module*              module;
    struct symt_compiland*      compiland;
    const struct elf_thunk_area*thunks;
    struct sparse_array         abbrev_table;
    struct sparse_array         debug_info_table;
    unsigned long               load_offset;
    unsigned long               ref_offset;
    struct symt*                symt_cache[sc_num]; /* void, int1, int2, int4 */
    char*                       cpp_name;
} dwarf2_parse_context_t;

/* stored in the dbghelp's module internal structure for later reuse */
struct dwarf2_module_info_s
{
    dwarf2_section_t            debug_loc;
    dwarf2_section_t            debug_frame;
    dwarf2_section_t            eh_frame;
    unsigned char               word_size;
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
    return *(const UINT16*)ptr;
}

static unsigned short dwarf2_parse_u2(dwarf2_traverse_context_t* ctx)
{
    unsigned short uvalue = dwarf2_get_u2(ctx->data);
    ctx->data += 2;
    return uvalue;
}

static unsigned long dwarf2_get_u4(const unsigned char* ptr)
{
    return *(const UINT32*)ptr;
}

static unsigned long dwarf2_parse_u4(dwarf2_traverse_context_t* ctx)
{
    unsigned long uvalue = dwarf2_get_u4(ctx->data);
    ctx->data += 4;
    return uvalue;
}

static DWORD64 dwarf2_get_u8(const unsigned char* ptr)
{
    return *(const UINT64*)ptr;
}

static DWORD64 dwarf2_parse_u8(dwarf2_traverse_context_t* ctx)
{
    DWORD64 uvalue = dwarf2_get_u8(ctx->data);
    ctx->data += 8;
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

/******************************************************************
 *		dwarf2_get_addr
 *
 * Returns an address.
 * We assume that in all cases word size from Dwarf matches the size of
 * addresses in platform where the exec is compiled.
 */
static unsigned long dwarf2_get_addr(const unsigned char* ptr, unsigned word_size)
{
    unsigned long ret;

    switch (word_size)
    {
    case 4:
        ret = dwarf2_get_u4(ptr);
        break;
    case 8:
        ret = dwarf2_get_u8(ptr);
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
        attr->u.uvalue = dwarf2_get_addr(data,
                                         ctx->module->format_info[DFI_DWARF]->u.dwarf2_info->word_size);
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
        attr->u.lluvalue = dwarf2_get_u8(data);
        TRACE("data8<%s>\n", wine_dbgstr_longlong(attr->u.uvalue));
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
        FIXME("Unhandled 64-bit support\n");
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
    unsigned                    i, refidx = 0;
    dwarf2_abbrev_entry_attr_t* abbrev_attr;
    dwarf2_abbrev_entry_attr_t* ref_abbrev_attr = NULL;

    attr->gotten_from = attr_direct;
    while (di)
    {
        ref_abbrev_attr = NULL;
        for (i = 0, abbrev_attr = di->abbrev->attrs; abbrev_attr; i++, abbrev_attr = abbrev_attr->next)
        {
            if (abbrev_attr->attribute == at)
            {
                dwarf2_fill_attr(ctx, abbrev_attr, di->data[i], attr);
                return TRUE;
            }
            if ((abbrev_attr->attribute == DW_AT_abstract_origin ||
                 abbrev_attr->attribute == DW_AT_specification) &&
                at != DW_AT_sibling)
            {
                if (ref_abbrev_attr)
                    FIXME("two references %lx and %lx\n", ref_abbrev_attr->attribute, abbrev_attr->attribute);
                ref_abbrev_attr = abbrev_attr;
                refidx = i;
                attr->gotten_from = (abbrev_attr->attribute == DW_AT_abstract_origin) ?
                    attr_abstract_origin : attr_specification;
            }
        }
        /* do we have either an abstract origin or a specification debug entry to look into ? */
        if (!ref_abbrev_attr) break;
        dwarf2_fill_attr(ctx, ref_abbrev_attr, di->data[refidx], attr);
        if (!(di = sparse_array_find(&ctx->debug_info_table, attr->u.uvalue)))
            FIXME("Should have found the debug info entry\n");
    }
    return FALSE;
}

static void dwarf2_load_one_entry(dwarf2_parse_context_t*, dwarf2_debug_info_t*);

#define Wine_DW_no_register     0x7FFFFFFF

static unsigned dwarf2_map_register(int regno)
{
    if (regno == Wine_DW_no_register)
    {
        FIXME("What the heck map reg 0x%x\n",regno);
        return 0;
    }
    return dbghelp_current_cpu->map_dwarf_register(regno);
}

static enum location_error
compute_location(dwarf2_traverse_context_t* ctx, struct location* loc,
                 HANDLE hproc, const struct location* frame)
{
    DWORD_PTR tmp, stack[64];
    unsigned stk;
    unsigned char op;
    BOOL piece_found = FALSE;

    stack[stk = 0] = 0;

    loc->kind = loc_absolute;
    loc->reg = Wine_DW_no_register;

    while (ctx->data < ctx->end_data)
    {
        op = dwarf2_parse_byte(ctx);

        if (op >= DW_OP_lit0 && op <= DW_OP_lit31)
            stack[++stk] = op - DW_OP_lit0;
        else if (op >= DW_OP_reg0 && op <= DW_OP_reg31)
        {
            /* dbghelp APIs don't know how to cope with this anyway
             * (for example 'long long' stored in two registers)
             * FIXME: We should tell winedbg how to deal with it (sigh)
             */
            if (!piece_found)
            {
                DWORD   cvreg = dwarf2_map_register(op - DW_OP_reg0);
                if (loc->reg != Wine_DW_no_register)
                    FIXME("Only supporting one reg (%s/%d -> %s/%d)\n",
                          dbghelp_current_cpu->fetch_regname(loc->reg), loc->reg,
                          dbghelp_current_cpu->fetch_regname(cvreg), cvreg);
                loc->reg = cvreg;
            }
            loc->kind = loc_register;
        }
        else if (op >= DW_OP_breg0 && op <= DW_OP_breg31)
        {
            /* dbghelp APIs don't know how to cope with this anyway
             * (for example 'long long' stored in two registers)
             * FIXME: We should tell winedbg how to deal with it (sigh)
             */
            if (!piece_found)
            {
                DWORD   cvreg = dwarf2_map_register(op - DW_OP_breg0);
                if (loc->reg != Wine_DW_no_register)
                    FIXME("Only supporting one breg (%s/%d -> %s/%d)\n",
                          dbghelp_current_cpu->fetch_regname(loc->reg), loc->reg,
                          dbghelp_current_cpu->fetch_regname(cvreg), cvreg);
                loc->reg = cvreg;
            }
            stack[++stk] = dwarf2_leb128_as_signed(ctx);
            loc->kind = loc_regrel;
        }
        else switch (op)
        {
        case DW_OP_nop:         break;
        case DW_OP_addr:        stack[++stk] = dwarf2_parse_addr(ctx); break;
        case DW_OP_const1u:     stack[++stk] = dwarf2_parse_byte(ctx); break;
        case DW_OP_const1s:     stack[++stk] = dwarf2_parse_byte(ctx); break;
        case DW_OP_const2u:     stack[++stk] = dwarf2_parse_u2(ctx); break;
        case DW_OP_const2s:     stack[++stk] = dwarf2_parse_u2(ctx); break;
        case DW_OP_const4u:     stack[++stk] = dwarf2_parse_u4(ctx); break;
        case DW_OP_const4s:     stack[++stk] = dwarf2_parse_u4(ctx); break;
        case DW_OP_const8u:     stack[++stk] = dwarf2_parse_u8(ctx); break;
        case DW_OP_const8s:     stack[++stk] = dwarf2_parse_u8(ctx); break;
        case DW_OP_constu:      stack[++stk] = dwarf2_leb128_as_unsigned(ctx); break;
        case DW_OP_consts:      stack[++stk] = dwarf2_leb128_as_signed(ctx); break;
        case DW_OP_dup:         stack[stk + 1] = stack[stk]; stk++; break;
        case DW_OP_drop:        stk--; break;
        case DW_OP_over:        stack[stk + 1] = stack[stk - 1]; stk++; break;
        case DW_OP_pick:        stack[stk + 1] = stack[stk - dwarf2_parse_byte(ctx)]; stk++; break;
        case DW_OP_swap:        tmp = stack[stk]; stack[stk] = stack[stk-1]; stack[stk-1] = tmp; break;
        case DW_OP_rot:         tmp = stack[stk]; stack[stk] = stack[stk-1]; stack[stk-1] = stack[stk-2]; stack[stk-2] = tmp; break;
        case DW_OP_abs:         stack[stk] = labs(stack[stk]); break;
        case DW_OP_neg:         stack[stk] = -stack[stk]; break;
        case DW_OP_not:         stack[stk] = ~stack[stk]; break;
        case DW_OP_and:         stack[stk-1] &= stack[stk]; stk--; break;
        case DW_OP_or:          stack[stk-1] |= stack[stk]; stk--; break;
        case DW_OP_minus:       stack[stk-1] -= stack[stk]; stk--; break;
        case DW_OP_mul:         stack[stk-1] *= stack[stk]; stk--; break;
        case DW_OP_plus:        stack[stk-1] += stack[stk]; stk--; break;
        case DW_OP_xor:         stack[stk-1] ^= stack[stk]; stk--; break;
        case DW_OP_shl:         stack[stk-1] <<= stack[stk]; stk--; break;
        case DW_OP_shr:         stack[stk-1] >>= stack[stk]; stk--; break;
        case DW_OP_plus_uconst: stack[stk] += dwarf2_leb128_as_unsigned(ctx); break;
        case DW_OP_shra:        stack[stk-1] = stack[stk-1] / (1 << stack[stk]); stk--; break;
        case DW_OP_div:         stack[stk-1] = stack[stk-1] / stack[stk]; stk--; break;
        case DW_OP_mod:         stack[stk-1] = stack[stk-1] % stack[stk]; stk--; break;
        case DW_OP_ge:          stack[stk-1] = (stack[stk-1] >= stack[stk]); stk--; break;
        case DW_OP_gt:          stack[stk-1] = (stack[stk-1] >  stack[stk]); stk--; break;
        case DW_OP_le:          stack[stk-1] = (stack[stk-1] <= stack[stk]); stk--; break;
        case DW_OP_lt:          stack[stk-1] = (stack[stk-1] <  stack[stk]); stk--; break;
        case DW_OP_eq:          stack[stk-1] = (stack[stk-1] == stack[stk]); stk--; break;
        case DW_OP_ne:          stack[stk-1] = (stack[stk-1] != stack[stk]); stk--; break;
        case DW_OP_skip:        tmp = dwarf2_parse_u2(ctx); ctx->data += tmp; break;
        case DW_OP_bra:         tmp = dwarf2_parse_u2(ctx); if (!stack[stk--]) ctx->data += tmp; break;
        case DW_OP_regx:
            tmp = dwarf2_leb128_as_unsigned(ctx);
            if (!piece_found)
            {
                if (loc->reg != Wine_DW_no_register)
                    FIXME("Only supporting one reg\n");
                loc->reg = dwarf2_map_register(tmp);
            }
            loc->kind = loc_register;
            break;
        case DW_OP_bregx:
            tmp = dwarf2_leb128_as_unsigned(ctx);
            if (loc->reg != Wine_DW_no_register)
                FIXME("Only supporting one regx\n");
            loc->reg = dwarf2_map_register(tmp);
            stack[++stk] = dwarf2_leb128_as_signed(ctx);
            loc->kind = loc_regrel;
            break;
        case DW_OP_fbreg:
            if (loc->reg != Wine_DW_no_register)
                FIXME("Only supporting one reg (%s/%d -> -2)\n",
                      dbghelp_current_cpu->fetch_regname(loc->reg), loc->reg);
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
                DWORD_PTR addr = stack[stk--];
                DWORD_PTR deref;

                if (!ReadProcessMemory(hproc, (void*)addr, &deref, sizeof(deref), NULL))
                {
                    WARN("Couldn't read memory at %lx\n", addr);
                    return loc_err_cant_read;
                }
                stack[++stk] = deref;
            }
            else
            {
               loc->kind = loc_dwarf2_block;
            }
            break;
        case DW_OP_deref_size:
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
                DWORD_PTR addr = stack[stk--];
                BYTE derefsize = dwarf2_parse_byte(ctx);
                DWORD64 deref;

                if (!ReadProcessMemory(hproc, (void*)addr, &deref, derefsize, NULL))
                {
                    WARN("Couldn't read memory at %lx\n", addr);
                       return loc_err_cant_read;
                }

                switch (derefsize)
                {
                   case 1: stack[++stk] = *(unsigned char*)&deref; break;
                   case 2: stack[++stk] = *(unsigned short*)&deref; break;
                   case 4: stack[++stk] = *(DWORD*)&deref; break;
                   case 8: if (ctx->word_size >= derefsize) stack[++stk] = deref; break;
                }
            }
            else
            {
                dwarf2_parse_byte(ctx);
                loc->kind = loc_dwarf2_block;
            }
            break;
        case DW_OP_stack_value:
            /* Expected behaviour is that this is the last instruction of this
             * expression and just the "top of stack" value should be put to loc->offset. */
            break;
        default:
            if (op < DW_OP_lo_user) /* as DW_OP_hi_user is 0xFF, we don't need to test against it */
                FIXME("Unhandled attr op: %x\n", op);
            /* FIXME else unhandled extension */
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
    case DW_FORM_block:
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
        break;
    default: FIXME("Unsupported yet form %lx\n", xloc.form);
        return FALSE;
    }

    /* assume we have a block form */

    if (xloc.u.block.size)
    {
        dwarf2_traverse_context_t       lctx;
        enum location_error             err;

        lctx.data = xloc.u.block.ptr;
        lctx.end_data = xloc.u.block.ptr + xloc.u.block.size;
        lctx.word_size = ctx->module->format_info[DFI_DWARF]->u.dwarf2_info->word_size;

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
    struct attribute attr;
    dwarf2_debug_info_t* type;

    if (!dwarf2_find_attribute(ctx, di, DW_AT_type, &attr))
        return NULL;
    if (!(type = sparse_array_find(&ctx->debug_info_table, attr.u.uvalue)))
    {
        FIXME("Unable to find back reference to type %lx\n", attr.u.uvalue);
        return NULL;
    }
    if (!type->symt)
    {
        /* load the debug info entity */
        dwarf2_load_one_entry(ctx, type);
        if (!type->symt)
            FIXME("Unable to load forward reference for tag %lx\n", type->abbrev->tag);
    }
    return type->symt;
}

static const char* dwarf2_get_cpp_name(dwarf2_parse_context_t* ctx, dwarf2_debug_info_t* di, const char* name)
{
    char* last;
    struct attribute diname;
    struct attribute spec;

    if (di->abbrev->tag == DW_TAG_compile_unit) return name;
    if (!ctx->cpp_name)
        ctx->cpp_name = pool_alloc(&ctx->pool, MAX_SYM_NAME);
    last = ctx->cpp_name + MAX_SYM_NAME - strlen(name) - 1;
    strcpy(last, name);

    /* if the di is a definition, but has also a (previous) declaration, then scope must
     * be gotten from declaration not definition
     */
    if (dwarf2_find_attribute(ctx, di, DW_AT_specification, &spec) && spec.gotten_from == attr_direct)
    {
        di = sparse_array_find(&ctx->debug_info_table, spec.u.uvalue);
        if (!di)
        {
            FIXME("Should have found the debug info entry\n");
            return NULL;
        }
    }

    for (di = di->parent; di; di = di->parent)
    {
        switch (di->abbrev->tag)
        {
        case DW_TAG_namespace:
        case DW_TAG_structure_type:
        case DW_TAG_class_type:
        case DW_TAG_interface_type:
        case DW_TAG_union_type:
            if (dwarf2_find_attribute(ctx, di, DW_AT_name, &diname))
            {
                size_t  len = strlen(diname.u.string);
                last -= 2 + len;
                if (last < ctx->cpp_name) return NULL;
                memcpy(last, diname.u.string, len);
                last[len] = last[len + 1] = ':';
            }
            break;
        default:
            break;
        }
    }
    return last;
}

/******************************************************************
 *		dwarf2_read_range
 *
 * read a range for a given debug_info (either using AT_range attribute, in which
 * case we don't return all the details, or using AT_low_pc & AT_high_pc attributes)
 * in all cases, range is relative to beginning of compilation unit
 */
static BOOL dwarf2_read_range(dwarf2_parse_context_t* ctx, const dwarf2_debug_info_t* di,
                              unsigned long* plow, unsigned long* phigh)
{
    struct attribute            range;

    if (dwarf2_find_attribute(ctx, di, DW_AT_ranges, &range))
    {
        dwarf2_traverse_context_t   traverse;
        unsigned long               low, high;

        traverse.data = ctx->sections[section_ranges].address + range.u.uvalue;
        traverse.end_data = ctx->sections[section_ranges].address +
            ctx->sections[section_ranges].size;
        traverse.word_size = ctx->module->format_info[DFI_DWARF]->u.dwarf2_info->word_size;

        *plow  = ULONG_MAX;
        *phigh = 0;
        while (traverse.data + 2 * traverse.word_size < traverse.end_data)
        {
            low = dwarf2_parse_addr(&traverse);
            high = dwarf2_parse_addr(&traverse);
            if (low == 0 && high == 0) break;
            if (low == ULONG_MAX) FIXME("unsupported yet (base address selection)\n");
            if (low  < *plow)  *plow = low;
            if (high > *phigh) *phigh = high;
        }
        if (*plow == ULONG_MAX || *phigh == 0) {FIXME("no entry found\n"); return FALSE;}
        if (*plow == *phigh) {FIXME("entry found, but low=high\n"); return FALSE;}

        return TRUE;
    }
    else
    {
        struct attribute            low_pc;
        struct attribute            high_pc;

        if (!dwarf2_find_attribute(ctx, di, DW_AT_low_pc, &low_pc) ||
            !dwarf2_find_attribute(ctx, di, DW_AT_high_pc, &high_pc))
            return FALSE;
        *plow = low_pc.u.uvalue;
        *phigh = high_pc.u.uvalue;
        return TRUE;
    }
}

/******************************************************************
 *		dwarf2_read_one_debug_info
 *
 * Loads into memory one debug info entry, and recursively its children (if any)
 */
static BOOL dwarf2_read_one_debug_info(dwarf2_parse_context_t* ctx,
                                       dwarf2_traverse_context_t* traverse,
                                       dwarf2_debug_info_t* parent_di,
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
    di->parent = parent_di;

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
            if (!dwarf2_read_one_debug_info(ctx, traverse, di, &child)) return FALSE;
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

static struct vector* dwarf2_get_di_children(dwarf2_parse_context_t* ctx,
                                             dwarf2_debug_info_t* di)
{
    struct attribute    spec;

    while (di)
    {
        if (di->abbrev->have_child)
            return &di->children;
        if (!dwarf2_find_attribute(ctx, di, DW_AT_specification, &spec)) break;
        if (!(di = sparse_array_find(&ctx->debug_info_table, spec.u.uvalue)))
            FIXME("Should have found the debug info entry\n");
    }
    return NULL;
}

static struct symt* dwarf2_parse_base_type(dwarf2_parse_context_t* ctx,
                                           dwarf2_debug_info_t* di)
{
    struct attribute name;
    struct attribute size;
    struct attribute encoding;
    enum BasicType bt;
    int cache_idx = -1;
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
    switch (bt)
    {
    case btVoid:
        assert(size.u.uvalue == 0);
        cache_idx = sc_void;
        break;
    case btInt:
        switch (size.u.uvalue)
        {
        case 1: cache_idx = sc_int1; break;
        case 2: cache_idx = sc_int2; break;
        case 4: cache_idx = sc_int4; break;
        }
        break;
    default: break;
    }
    if (cache_idx != -1 && !ctx->symt_cache[cache_idx])
        ctx->symt_cache[cache_idx] = di->symt;

    if (dwarf2_get_di_children(ctx, di)) FIXME("Unsupported children\n");
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
    if (dwarf2_get_di_children(ctx, di)) FIXME("Unsupported children\n");
    return di->symt;
}

static struct symt* dwarf2_parse_pointer_type(dwarf2_parse_context_t* ctx,
                                              dwarf2_debug_info_t* di)
{
    struct symt*        ref_type;
    struct attribute    size;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di)); 

    if (!dwarf2_find_attribute(ctx, di, DW_AT_byte_size, &size)) size.u.uvalue = sizeof(void *);
    if (!(ref_type = dwarf2_lookup_type(ctx, di)))
    {
        ref_type = ctx->symt_cache[sc_void];
        assert(ref_type);
    }
    di->symt = &symt_new_pointer(ctx->module, ref_type, size.u.uvalue)->symt;
    if (dwarf2_get_di_children(ctx, di)) FIXME("Unsupported children\n");
    return di->symt;
}

static struct symt* dwarf2_parse_array_type(dwarf2_parse_context_t* ctx,
                                            dwarf2_debug_info_t* di)
{
    struct symt* ref_type;
    struct symt* idx_type = NULL;
    struct attribute min, max, cnt;
    dwarf2_debug_info_t* child;
    unsigned int i;
    const struct vector* children;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    ref_type = dwarf2_lookup_type(ctx, di);

    if (!(children = dwarf2_get_di_children(ctx, di)))
    {
        /* fake an array with unknown size */
        /* FIXME: int4 even on 64bit machines??? */
        idx_type = ctx->symt_cache[sc_int4];
        min.u.uvalue = 0;
        max.u.uvalue = -1;
    }
    else for (i = 0; i < vector_length(children); i++)
    {
        child = *(dwarf2_debug_info_t**)vector_at(children, i);
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

    if (!(ref_type = dwarf2_lookup_type(ctx, di)))
    {
        ref_type = ctx->symt_cache[sc_void];
        assert(ref_type);
    }
    if (dwarf2_get_di_children(ctx, di)) FIXME("Unsupported children\n");
    di->symt = ref_type;

    return ref_type;
}

static struct symt* dwarf2_parse_volatile_type(dwarf2_parse_context_t* ctx,
                                               dwarf2_debug_info_t* di)
{
    struct symt* ref_type;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    if (!(ref_type = dwarf2_lookup_type(ctx, di)))
    {
        ref_type = ctx->symt_cache[sc_void];
        assert(ref_type);
    }
    if (dwarf2_get_di_children(ctx, di)) FIXME("Unsupported children\n");
    di->symt = ref_type;

    return ref_type;
}

static struct symt* dwarf2_parse_unspecified_type(dwarf2_parse_context_t* ctx,
                                           dwarf2_debug_info_t* di)
{
    struct attribute name;
    struct attribute size;
    struct symt_basic *basic;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    if (di->symt) return di->symt;

    if (!dwarf2_find_attribute(ctx, di, DW_AT_name, &name))
        name.u.string = "void";
    size.u.uvalue = sizeof(void *);

    basic = symt_new_basic(ctx->module, btVoid, name.u.string, size.u.uvalue);
    di->symt = &basic->symt;

    if (!ctx->symt_cache[sc_void])
        ctx->symt_cache[sc_void] = di->symt;

    if (dwarf2_get_di_children(ctx, di)) FIXME("Unsupported children\n");
    return di->symt;
}

static struct symt* dwarf2_parse_reference_type(dwarf2_parse_context_t* ctx,
                                                dwarf2_debug_info_t* di)
{
    struct symt* ref_type = NULL;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    ref_type = dwarf2_lookup_type(ctx, di);
    /* FIXME: for now, we hard-wire C++ references to pointers */
    di->symt = &symt_new_pointer(ctx->module, ref_type, sizeof(void *))->symt;

    if (dwarf2_get_di_children(ctx, di)) FIXME("Unsupported children\n");

    return di->symt;
}

static void dwarf2_parse_udt_member(dwarf2_parse_context_t* ctx,
                                    dwarf2_debug_info_t* di,
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
            nbytes.u.uvalue = symt_get_info(ctx->module, elt_type, TI_GET_LENGTH, &size) ?
                (unsigned long)size : 0;
        }
        bit_offset.u.uvalue = nbytes.u.uvalue * 8 - bit_offset.u.uvalue - bit_size.u.uvalue;
    }
    else bit_offset.u.uvalue = 0;
    symt_add_udt_element(ctx->module, parent, name.u.string, elt_type,    
                         (loc.offset << 3) + bit_offset.u.uvalue,
                         bit_size.u.uvalue);

    if (dwarf2_get_di_children(ctx, di)) FIXME("Unsupported children\n");
}

static struct symt* dwarf2_parse_subprogram(dwarf2_parse_context_t* ctx,
                                            dwarf2_debug_info_t* di);

static struct symt* dwarf2_parse_udt_type(dwarf2_parse_context_t* ctx,
                                          dwarf2_debug_info_t* di,
                                          enum UdtKind udt)
{
    struct attribute    name;
    struct attribute    size;
    struct vector*      children;
    dwarf2_debug_info_t*child;
    unsigned int        i;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di)); 

    /* quirk... FIXME provide real support for anonymous UDTs */
    if (!dwarf2_find_attribute(ctx, di, DW_AT_name, &name))
        name.u.string = "zz_anon_zz";
    if (!dwarf2_find_attribute(ctx, di, DW_AT_byte_size, &size)) size.u.uvalue = 0;

    di->symt = &symt_new_udt(ctx->module, dwarf2_get_cpp_name(ctx, di, name.u.string),
                             size.u.uvalue, udt)->symt;

    children = dwarf2_get_di_children(ctx, di);
    if (children) for (i = 0; i < vector_length(children); i++)
    {
        child = *(dwarf2_debug_info_t**)vector_at(children, i);

        switch (child->abbrev->tag)
        {
        case DW_TAG_array_type:
            dwarf2_parse_array_type(ctx, di);
            break;
        case DW_TAG_member:
            /* FIXME: should I follow the sibling stuff ?? */
            dwarf2_parse_udt_member(ctx, child, (struct symt_udt*)di->symt);
            break;
        case DW_TAG_enumeration_type:
            dwarf2_parse_enumeration_type(ctx, child);
            break;
        case DW_TAG_subprogram:
            dwarf2_parse_subprogram(ctx, child);
            break;
        case DW_TAG_const_type:
            dwarf2_parse_const_type(ctx, child);
            break;
        case DW_TAG_structure_type:
        case DW_TAG_class_type:
        case DW_TAG_union_type:
        case DW_TAG_typedef:
            /* FIXME: we need to handle nested udt definitions */
        case DW_TAG_inheritance:
        case DW_TAG_template_type_param:
        case DW_TAG_template_value_param:
        case DW_TAG_variable:
        case DW_TAG_imported_declaration:
        case DW_TAG_ptr_to_member_type:
        case DW_TAG_GNU_template_parameter_pack:
        case DW_TAG_GNU_formal_parameter_pack:
            /* FIXME: some C++ related stuff */
            break;
        default:
            FIXME("Unhandled Tag type 0x%lx at %s, for %s\n",
                  child->abbrev->tag, dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));
            break;
        }
    }

    return di->symt;
}

static void dwarf2_parse_enumerator(dwarf2_parse_context_t* ctx,
                                    dwarf2_debug_info_t* di,
                                    struct symt_enum* parent)
{
    struct attribute    name;
    struct attribute    value;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di)); 

    if (!dwarf2_find_attribute(ctx, di, DW_AT_name, &name)) return;
    if (!dwarf2_find_attribute(ctx, di, DW_AT_const_value, &value)) value.u.svalue = 0;
    symt_add_enum_element(ctx->module, parent, name.u.string, value.u.svalue);

    if (dwarf2_get_di_children(ctx, di)) FIXME("Unsupported children\n");
}

static struct symt* dwarf2_parse_enumeration_type(dwarf2_parse_context_t* ctx,
                                                  dwarf2_debug_info_t* di)
{
    struct attribute    name;
    struct attribute    size;
    struct symt_basic*  basetype;
    struct vector*      children;
    dwarf2_debug_info_t*child;
    unsigned int        i;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di)); 

    if (!dwarf2_find_attribute(ctx, di, DW_AT_name, &name)) name.u.string = NULL;
    if (!dwarf2_find_attribute(ctx, di, DW_AT_byte_size, &size)) size.u.uvalue = 4;

    switch (size.u.uvalue) /* FIXME: that's wrong */
    {
    case 1: basetype = symt_new_basic(ctx->module, btInt, "char", 1); break;
    case 2: basetype = symt_new_basic(ctx->module, btInt, "short", 2); break;
    default:
    case 4: basetype = symt_new_basic(ctx->module, btInt, "int", 4); break;
    }

    di->symt = &symt_new_enum(ctx->module, name.u.string, &basetype->symt)->symt;

    children = dwarf2_get_di_children(ctx, di);
    /* FIXME: should we use the sibling stuff ?? */
    if (children) for (i = 0; i < vector_length(children); i++)
    {
        child = *(dwarf2_debug_info_t**)vector_at(children, i);

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
    return di->symt;
}

/* structure used to pass information around when parsing a subprogram */
typedef struct dwarf2_subprogram_s
{
    dwarf2_parse_context_t*     ctx;
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
        case loc_error:
            break;
        case loc_absolute:
            /* it's a global variable */
            /* FIXME: we don't handle its scope yet */
            if (!dwarf2_find_attribute(subpgm->ctx, di, DW_AT_external, &ext))
                ext.u.uvalue = 0;
            loc.offset += subpgm->ctx->load_offset;
            symt_new_global_variable(subpgm->ctx->module, subpgm->ctx->compiland,
                                     dwarf2_get_cpp_name(subpgm->ctx, di, name.u.string), !ext.u.uvalue,
                                     loc, 0, param_type);
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
        case DW_FORM_addr:
            v.n1.n2.vt = VT_UI4;
            v.n1.n2.n3.lVal = value.u.uvalue;
            break;

        case DW_FORM_data8:
            v.n1.n2.vt = VT_UI8;
            v.n1.n2.n3.llVal = value.u.lluvalue;
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

        case DW_FORM_block:
        case DW_FORM_block1:
        case DW_FORM_block2:
        case DW_FORM_block4:
            v.n1.n2.vt = VT_I4;
            switch (value.u.block.size)
            {
            case 1:     v.n1.n2.n3.lVal = *(BYTE*)value.u.block.ptr;    break;
            case 2:     v.n1.n2.n3.lVal = *(USHORT*)value.u.block.ptr;  break;
            case 4:     v.n1.n2.n3.lVal = *(DWORD*)value.u.block.ptr;   break;
            default:
                v.n1.n2.vt = VT_I1 | VT_BYREF;
                v.n1.n2.n3.byref = pool_alloc(&subpgm->ctx->module->pool, value.u.block.size);
                memcpy(v.n1.n2.n3.byref, value.u.block.ptr, value.u.block.size);
            }
            break;

        default:
            FIXME("Unsupported form for const value %s (%lx)\n",
                  name.u.string, value.form);
            v.n1.n2.vt = VT_EMPTY;
        }
        di->symt = &symt_new_constant(subpgm->ctx->module, subpgm->ctx->compiland,
                                      name.u.string, param_type, &v)->symt;
    }
    else
    {
        /* variable has been optimized away... report anyway */
        loc.kind = loc_error;
        loc.reg = loc_err_no_location;
        if (subpgm->func)
        {
            symt_add_func_local(subpgm->ctx->module, subpgm->func,
                                is_pmt ? DataIsParam : DataIsLocal,
                                &loc, block, param_type, name.u.string);
        }
        else
        {
            WARN("dropping global variable %s which has been optimized away\n", name.u.string);
        }
    }
    if (is_pmt && subpgm->func && subpgm->func->type)
        symt_add_function_signature_parameter(subpgm->ctx->module,
                                              (struct symt_function_signature*)subpgm->func->type,
                                              param_type);

    if (dwarf2_get_di_children(subpgm->ctx, di)) FIXME("Unsupported children\n");
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
                      dwarf2_debug_info_t* di);

static struct symt* dwarf2_parse_subroutine_type(dwarf2_parse_context_t* ctx,
                                                 dwarf2_debug_info_t* di);

static void dwarf2_parse_inlined_subroutine(dwarf2_subprogram_t* subpgm,
                                            struct symt_block* parent_block,
                                            dwarf2_debug_info_t* di)
{
    struct symt_block*  block;
    unsigned long       low_pc, high_pc;
    struct vector*      children;
    dwarf2_debug_info_t*child;
    unsigned int        i;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(subpgm->ctx), dwarf2_debug_di(di));

    if (!dwarf2_read_range(subpgm->ctx, di, &low_pc, &high_pc))
    {
        FIXME("cannot read range\n");
        return;
    }

    block = symt_open_func_block(subpgm->ctx->module, subpgm->func, parent_block,
                                 subpgm->ctx->load_offset + low_pc - subpgm->func->address,
                                 high_pc - low_pc);

    children = dwarf2_get_di_children(subpgm->ctx, di);
    if (children) for (i = 0; i < vector_length(children); i++)
    {
        child = *(dwarf2_debug_info_t**)vector_at(children, i);

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
        case DW_TAG_GNU_call_site:
            /* this isn't properly supported by dbghelp interface. skip it for now */
            break;
        default:
            FIXME("Unhandled Tag type 0x%lx at %s, for %s\n",
                  child->abbrev->tag, dwarf2_debug_ctx(subpgm->ctx),
                  dwarf2_debug_di(di));
        }
    }
    symt_close_func_block(subpgm->ctx->module, subpgm->func, block, 0);
}

static void dwarf2_parse_subprogram_block(dwarf2_subprogram_t* subpgm,
                                          struct symt_block* parent_block,
					  dwarf2_debug_info_t* di)
{
    struct symt_block*  block;
    unsigned long       low_pc, high_pc;
    struct vector*      children;
    dwarf2_debug_info_t*child;
    unsigned int        i;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(subpgm->ctx), dwarf2_debug_di(di));

    if (!dwarf2_read_range(subpgm->ctx, di, &low_pc, &high_pc))
    {
        FIXME("no range\n");
        return;
    }

    block = symt_open_func_block(subpgm->ctx->module, subpgm->func, parent_block,
                                 subpgm->ctx->load_offset + low_pc - subpgm->func->address,
                                 high_pc - low_pc);

    children = dwarf2_get_di_children(subpgm->ctx, di);
    if (children) for (i = 0; i < vector_length(children); i++)
    {
        child = *(dwarf2_debug_info_t**)vector_at(children, i);

        switch (child->abbrev->tag)
        {
        case DW_TAG_inlined_subroutine:
            dwarf2_parse_inlined_subroutine(subpgm, block, child);
            break;
        case DW_TAG_variable:
            dwarf2_parse_variable(subpgm, block, child);
            break;
        case DW_TAG_pointer_type:
            dwarf2_parse_pointer_type(subpgm->ctx, di);
            break;
        case DW_TAG_subroutine_type:
            dwarf2_parse_subroutine_type(subpgm->ctx, di);
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
        case DW_TAG_imported_module:
            /* C++ stuff to be silenced (for now) */
            break;
        case DW_TAG_GNU_call_site:
            /* this isn't properly supported by dbghelp interface. skip it for now */
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

    symt_close_func_block(subpgm->ctx->module, subpgm->func, block, 0);
}

static struct symt* dwarf2_parse_subprogram(dwarf2_parse_context_t* ctx,
                                            dwarf2_debug_info_t* di)
{
    struct attribute name;
    unsigned long low_pc, high_pc;
    struct attribute is_decl;
    struct attribute inline_flags;
    struct symt* ret_type;
    struct symt_function_signature* sig_type;
    dwarf2_subprogram_t subpgm;
    struct vector* children;
    dwarf2_debug_info_t* child;
    unsigned int i;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    if (!dwarf2_find_attribute(ctx, di, DW_AT_name, &name))
    {
        WARN("No name for function... dropping function\n");
        return NULL;
    }
    /* if it's an abstract representation of an inline function, there should be
     * a concrete object that we'll handle
     */
    if (dwarf2_find_attribute(ctx, di, DW_AT_inline, &inline_flags) &&
        inline_flags.u.uvalue != DW_INL_not_inlined)
    {
        TRACE("Function %s declared as inlined (%ld)... skipping\n",
              name.u.string ? name.u.string : "(null)", inline_flags.u.uvalue);
        return NULL;
    }

    if (dwarf2_find_attribute(ctx, di, DW_AT_declaration, &is_decl) &&
        is_decl.u.uvalue && is_decl.gotten_from == attr_direct)
    {
        /* it's a real declaration, skip it */
        return NULL;
    }
    if (!dwarf2_read_range(ctx, di, &low_pc, &high_pc))
    {
        WARN("cannot get range for %s\n", name.u.string);
        return NULL;
    }
    /* As functions (defined as inline assembly) get debug info with dwarf
     * (not the case for stabs), we just drop Wine's thunks here...
     * Actual thunks will be created in elf_module from the symbol table
     */
#ifndef DBGHELP_STATIC_LIB
    if (elf_is_in_thunk_area(ctx->load_offset + low_pc, ctx->thunks) >= 0)
        return NULL;
#endif
    if (!(ret_type = dwarf2_lookup_type(ctx, di)))
    {
        ret_type = ctx->symt_cache[sc_void];
        assert(ret_type);
    }
    /* FIXME: assuming C source code */
    sig_type = symt_new_function_signature(ctx->module, ret_type, CV_CALL_FAR_C);
    subpgm.func = symt_new_function(ctx->module, ctx->compiland,
                                    dwarf2_get_cpp_name(ctx, di, name.u.string),
                                    ctx->load_offset + low_pc, high_pc - low_pc,
                                    &sig_type->symt);
    di->symt = &subpgm.func->symt;
    subpgm.ctx = ctx;
    if (!dwarf2_compute_location_attr(ctx, di, DW_AT_frame_base,
                                      &subpgm.frame, NULL))
    {
        /* on stack !! */
        subpgm.frame.kind = loc_regrel;
        subpgm.frame.reg = dbghelp_current_cpu->frame_regno;
        subpgm.frame.offset = 0;
    }
    subpgm.non_computed_variable = FALSE;

    children = dwarf2_get_di_children(ctx, di);
    if (children) for (i = 0; i < vector_length(children); i++)
    {
        child = *(dwarf2_debug_info_t**)vector_at(children, i);

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
        case DW_TAG_pointer_type:
            dwarf2_parse_pointer_type(subpgm.ctx, di);
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
        case DW_TAG_template_type_param:
        case DW_TAG_template_value_param:
        case DW_TAG_GNU_call_site:
        case DW_TAG_GNU_template_parameter_pack:
        case DW_TAG_GNU_formal_parameter_pack:
            /* FIXME: no support in dbghelp's internals so far */
            break;
        default:
            FIXME("Unhandled Tag type 0x%lx at %s, for %s\n",
                  child->abbrev->tag, dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));
	}
    }

    if (subpgm.non_computed_variable || subpgm.frame.kind >= loc_user)
    {
        symt_add_function_point(ctx->module, subpgm.func, SymTagCustom,
                                &subpgm.frame, NULL);
    }
    if (subpgm.func) symt_normalize_function(subpgm.ctx->module, subpgm.func);

    return di->symt;
}

static struct symt* dwarf2_parse_subroutine_type(dwarf2_parse_context_t* ctx,
                                                 dwarf2_debug_info_t* di)
{
    struct symt* ret_type;
    struct symt_function_signature* sig_type;
    struct vector* children;
    dwarf2_debug_info_t* child;
    unsigned int i;

    if (di->symt) return di->symt;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    if (!(ret_type = dwarf2_lookup_type(ctx, di)))
    {
        ret_type = ctx->symt_cache[sc_void];
        assert(ret_type);
    }

    /* FIXME: assuming C source code */
    sig_type = symt_new_function_signature(ctx->module, ret_type, CV_CALL_FAR_C);

    children = dwarf2_get_di_children(ctx, di);
    if (children) for (i = 0; i < vector_length(children); i++)
    {
        child = *(dwarf2_debug_info_t**)vector_at(children, i);

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

    return di->symt = &sig_type->symt;
}

static void dwarf2_parse_namespace(dwarf2_parse_context_t* ctx,
                                   dwarf2_debug_info_t* di)
{
    struct vector*          children;
    dwarf2_debug_info_t*    child;
    unsigned int            i;

    if (di->symt) return;

    TRACE("%s, for %s\n", dwarf2_debug_ctx(ctx), dwarf2_debug_di(di));

    di->symt = ctx->symt_cache[sc_void];

    children = dwarf2_get_di_children(ctx, di);
    if (children) for (i = 0; i < vector_length(children); i++)
    {
        child = *(dwarf2_debug_info_t**)vector_at(children, i);
        dwarf2_load_one_entry(ctx, child);
    }
}

static void dwarf2_load_one_entry(dwarf2_parse_context_t* ctx,
                                  dwarf2_debug_info_t* di)
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
    case DW_TAG_unspecified_type:
        dwarf2_parse_unspecified_type(ctx, di);
        break;
    case DW_TAG_reference_type:
        dwarf2_parse_reference_type(ctx, di);
        break;
    case DW_TAG_enumeration_type:
        dwarf2_parse_enumeration_type(ctx, di);
        break;
    case DW_TAG_subprogram:
        dwarf2_parse_subprogram(ctx, di);
        break;
    case DW_TAG_subroutine_type:
        dwarf2_parse_subroutine_type(ctx, di);
        break;
    case DW_TAG_variable:
        {
            dwarf2_subprogram_t subpgm;

            subpgm.ctx = ctx;
            subpgm.func = NULL;
            subpgm.frame.kind = loc_absolute;
            subpgm.frame.offset = 0;
            subpgm.frame.reg = Wine_DW_no_register;
            dwarf2_parse_variable(&subpgm, NULL, di);
        }
        break;
    case DW_TAG_namespace:
        dwarf2_parse_namespace(ctx, di);
        break;
    /* silence a couple of C++ defines */
    case DW_TAG_imported_module:
    case DW_TAG_imported_declaration:
    case DW_TAG_ptr_to_member_type:
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
    unsigned                    insn_size, default_stmt;
    unsigned                    line_range, opcode_base;
    int                         line_base;
    const unsigned char*        opcode_len;
    struct vector               dirs;
    struct vector               files;
    const char**                p;

    /* section with line numbers stripped */
    if (sections[section_line].address == IMAGE_NO_MAP)
        return FALSE;

    if (offset + 4 > sections[section_line].size)
    {
        WARN("out of bounds offset\n");
        return FALSE;
    }
    traverse.data = sections[section_line].address + offset;
    traverse.end_data = traverse.data + 4;
    traverse.word_size = ctx->module->format_info[DFI_DWARF]->u.dwarf2_info->word_size;

    length = dwarf2_parse_u4(&traverse);
    traverse.end_data = sections[section_line].address + offset + length;

    if (offset + 4 + length > sections[section_line].size)
    {
        WARN("out of bounds header\n");
        return FALSE;
    }
    dwarf2_parse_u2(&traverse); /* version */
    dwarf2_parse_u4(&traverse); /* header_len */
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
        unsigned int    dir_index, mod_time;
        const char*     name;
        const char*     dir;
        unsigned*       psrc;

        name = (const char*)traverse.data;
        traverse.data += strlen(name) + 1;
        dir_index = dwarf2_leb128_as_unsigned(&traverse);
        mod_time = dwarf2_leb128_as_unsigned(&traverse);
        length = dwarf2_leb128_as_unsigned(&traverse);
        dir = *(const char**)vector_at(&dirs, dir_index);
        TRACE("Got file %s/%s (%u,%lu)\n", dir, name, mod_time, length);
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
        BOOL end_sequence = FALSE;
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
                dwarf2_set_line_number(ctx->module, address, &files, file, line);
            }
            else
            {
                switch (opcode)
                {
                case DW_LNS_copy:
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
                        FIXME("not handled define file %s\n", traverse.data);
                        traverse.data += strlen((const char *)traverse.data) + 1;
                        dwarf2_leb128_as_unsigned(&traverse);
                        dwarf2_leb128_as_unsigned(&traverse);
                        dwarf2_leb128_as_unsigned(&traverse);
                        break;
                    case DW_LNE_set_discriminator:
                        {
                            unsigned descr;

                            descr = dwarf2_leb128_as_unsigned(&traverse);
                            WARN("not handled discriminator %x\n", descr);
                        }
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
                                          struct module* module,
                                          const struct elf_thunk_area* thunks,
                                          dwarf2_traverse_context_t* mod_ctx,
                                          unsigned long load_offset)
{
    dwarf2_parse_context_t ctx;
    dwarf2_traverse_context_t abbrev_ctx;
    dwarf2_debug_info_t* di;
    dwarf2_traverse_context_t cu_ctx;
    const unsigned char* comp_unit_start = mod_ctx->data;
    unsigned long cu_length;
    unsigned short cu_version;
    unsigned long cu_abbrev_offset;
    BOOL ret = FALSE;

    cu_length = dwarf2_parse_u4(mod_ctx);
    cu_ctx.data = mod_ctx->data;
    cu_ctx.end_data = mod_ctx->data + cu_length;
    mod_ctx->data += cu_length;
    cu_version = dwarf2_parse_u2(&cu_ctx);
    cu_abbrev_offset = dwarf2_parse_u4(&cu_ctx);
    cu_ctx.word_size = dwarf2_parse_byte(&cu_ctx);

    TRACE("Compilation Unit Header found at 0x%x:\n",
          (int)(comp_unit_start - sections[section_debug].address));
    TRACE("- length:        %lu\n", cu_length);
    TRACE("- version:       %u\n",  cu_version);
    TRACE("- abbrev_offset: %lu\n", cu_abbrev_offset);
    TRACE("- word_size:     %u\n",  cu_ctx.word_size);

    if (cu_version != 2)
    {
        WARN("%u DWARF version unsupported. Wine dbghelp only support DWARF 2.\n",
             cu_version);
        return FALSE;
    }

    module->format_info[DFI_DWARF]->u.dwarf2_info->word_size = cu_ctx.word_size;
    mod_ctx->word_size = cu_ctx.word_size;

    pool_init(&ctx.pool, 65536);
    ctx.sections = sections;
    ctx.section = section_debug;
    ctx.module = module;
    ctx.thunks = thunks;
    ctx.load_offset = load_offset;
    ctx.ref_offset = comp_unit_start - sections[section_debug].address;
    memset(ctx.symt_cache, 0, sizeof(ctx.symt_cache));
    ctx.symt_cache[sc_void] = &symt_new_basic(module, btVoid, "void", 0)->symt;
    ctx.cpp_name = NULL;

    abbrev_ctx.data = sections[section_abbrev].address + cu_abbrev_offset;
    abbrev_ctx.end_data = sections[section_abbrev].address + sections[section_abbrev].size;
    abbrev_ctx.word_size = cu_ctx.word_size;
    dwarf2_parse_abbrev_set(&abbrev_ctx, &ctx.abbrev_table, &ctx.pool);

    sparse_array_init(&ctx.debug_info_table, sizeof(dwarf2_debug_info_t), 128);
    dwarf2_read_one_debug_info(&ctx, &cu_ctx, NULL, &di);

    if (di->abbrev->tag == DW_TAG_compile_unit)
    {
        struct attribute            name;
        struct vector*              children;
        dwarf2_debug_info_t*        child = NULL;
        unsigned int                i;
        struct attribute            stmt_list, low_pc;
        struct attribute            comp_dir;

        if (!dwarf2_find_attribute(&ctx, di, DW_AT_name, &name))
            name.u.string = NULL;

        /* get working directory of current compilation unit */
        if (!dwarf2_find_attribute(&ctx, di, DW_AT_comp_dir, &comp_dir))
            comp_dir.u.string = NULL;

        if (!dwarf2_find_attribute(&ctx, di, DW_AT_low_pc, &low_pc))
            low_pc.u.uvalue = 0;
        ctx.compiland = symt_new_compiland(module, ctx.load_offset + low_pc.u.uvalue,
                                           source_new(module, comp_dir.u.string, name.u.string));
        di->symt = &ctx.compiland->symt;
        children = dwarf2_get_di_children(&ctx, di);
        if (children) for (i = 0; i < vector_length(children); i++)
        {
            child = *(dwarf2_debug_info_t**)vector_at(children, i);
            dwarf2_load_one_entry(&ctx, child);
        }
        if (dwarf2_find_attribute(&ctx, di, DW_AT_stmt_list, &stmt_list))
        {
#if defined(__REACTOS__) && defined(__clang__)
            unsigned long stmt_list_val = stmt_list.u.uvalue;
            if (stmt_list_val > module->module.BaseOfImage)
            {
                /* FIXME: Clang is recording this as an address, not an offset */
                stmt_list_val -= module->module.BaseOfImage + sections[section_line].rva;
            }
            if (dwarf2_parse_line_numbers(sections, &ctx, comp_dir.u.string, stmt_list_val))
#else
            if (dwarf2_parse_line_numbers(sections, &ctx, comp_dir.u.string, stmt_list.u.uvalue))
#endif
                module->module.LineNumbers = TRUE;
        }
        ret = TRUE;
    }
    else FIXME("Should have a compilation unit here\n");
    pool_destroy(&ctx.pool);
    return ret;
}

static BOOL dwarf2_lookup_loclist(const struct module_format* modfmt, const BYTE* start,
                                  unsigned long ip, dwarf2_traverse_context_t* lctx)
{
    DWORD_PTR                   beg, end;
    const BYTE*                 ptr = start;
    DWORD                       len;

    while (ptr < modfmt->u.dwarf2_info->debug_loc.address + modfmt->u.dwarf2_info->debug_loc.size)
    {
        beg = dwarf2_get_addr(ptr, modfmt->u.dwarf2_info->word_size); ptr += modfmt->u.dwarf2_info->word_size;
        end = dwarf2_get_addr(ptr, modfmt->u.dwarf2_info->word_size); ptr += modfmt->u.dwarf2_info->word_size;
        if (!beg && !end) break;
        len = dwarf2_get_u2(ptr); ptr += 2;

        if (beg <= ip && ip < end)
        {
            lctx->data = ptr;
            lctx->end_data = ptr + len;
            lctx->word_size = modfmt->u.dwarf2_info->word_size;
            return TRUE;
        }
        ptr += len;
    }
    WARN("Couldn't find ip in location list\n");
    return FALSE;
}

static enum location_error loc_compute_frame(struct process* pcs,
                                             const struct module_format* modfmt,
                                             const struct symt_function* func,
                                             DWORD_PTR ip, struct location* frame)
{
    struct symt**               psym = NULL;
    struct location*            pframe;
    dwarf2_traverse_context_t   lctx;
    enum location_error         err;
    unsigned int                i;

    for (i=0; i<vector_length(&func->vchildren); i++)
    {
        psym = vector_at(&func->vchildren, i);
        if ((*psym)->tag == SymTagCustom)
        {
            pframe = &((struct symt_hierarchy_point*)*psym)->loc;

            /* First, recompute the frame information, if needed */
            switch (pframe->kind)
            {
            case loc_regrel:
            case loc_register:
                *frame = *pframe;
                break;
            case loc_dwarf2_location_list:
                WARN("Searching loclist for %s\n", func->hash_elt.name);
                if (!dwarf2_lookup_loclist(modfmt,
                                           modfmt->u.dwarf2_info->debug_loc.address + pframe->offset,
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

enum reg_rule
{
    RULE_UNSET,          /* not set at all */
    RULE_UNDEFINED,      /* undefined value */
    RULE_SAME,           /* same value as previous frame */
    RULE_CFA_OFFSET,     /* stored at cfa offset */
    RULE_OTHER_REG,      /* stored in other register */
    RULE_EXPRESSION,     /* address specified by expression */
    RULE_VAL_EXPRESSION  /* value specified by expression */
};

/* make it large enough for all CPUs */
#define NB_FRAME_REGS 64
#define MAX_SAVED_STATES 16

struct frame_state
{
    ULONG_PTR     cfa_offset;
    unsigned char cfa_reg;
    enum reg_rule cfa_rule;
    enum reg_rule rules[NB_FRAME_REGS];
    ULONG_PTR     regs[NB_FRAME_REGS];
};

struct frame_info
{
    ULONG_PTR     ip;
    ULONG_PTR     code_align;
    LONG_PTR      data_align;
    unsigned char retaddr_reg;
    unsigned char fde_encoding;
    unsigned char lsda_encoding;
    unsigned char signal_frame;
    unsigned char aug_z_format;
    unsigned char state_sp;
    struct frame_state state;
    struct frame_state state_stack[MAX_SAVED_STATES];
};

static ULONG_PTR dwarf2_parse_augmentation_ptr(dwarf2_traverse_context_t* ctx, unsigned char encoding)
{
    ULONG_PTR   base;

    if (encoding == DW_EH_PE_omit) return 0;

    switch (encoding & 0xf0)
    {
    case DW_EH_PE_abs:
        base = 0;
        break;
    case DW_EH_PE_pcrel:
        base = (ULONG_PTR)ctx->data;
        break;
    default:
        FIXME("unsupported encoding %02x\n", encoding);
        return 0;
    }

    switch (encoding & 0x0f)
    {
    case DW_EH_PE_native:
        return base + dwarf2_parse_addr(ctx);
    case DW_EH_PE_leb128:
        return base + dwarf2_leb128_as_unsigned(ctx);
    case DW_EH_PE_data2:
        return base + dwarf2_parse_u2(ctx);
    case DW_EH_PE_data4:
        return base + dwarf2_parse_u4(ctx);
    case DW_EH_PE_data8:
        return base + dwarf2_parse_u8(ctx);
    case DW_EH_PE_signed|DW_EH_PE_leb128:
        return base + dwarf2_leb128_as_signed(ctx);
    case DW_EH_PE_signed|DW_EH_PE_data2:
        return base + (signed short)dwarf2_parse_u2(ctx);
    case DW_EH_PE_signed|DW_EH_PE_data4:
        return base + (signed int)dwarf2_parse_u4(ctx);
    case DW_EH_PE_signed|DW_EH_PE_data8:
        return base + (LONG64)dwarf2_parse_u8(ctx);
    default:
        FIXME("unsupported encoding %02x\n", encoding);
        return 0;
    }
}

static BOOL parse_cie_details(dwarf2_traverse_context_t* ctx, struct frame_info* info)
{
    unsigned char version;
    const char* augmentation;
    const unsigned char* end;
    ULONG_PTR len;

    memset(info, 0, sizeof(*info));
    info->lsda_encoding = DW_EH_PE_omit;
    info->aug_z_format = 0;

    /* parse the CIE first */
    version = dwarf2_parse_byte(ctx);
    if (version != 1)
    {
        FIXME("unknown CIE version %u at %p\n", version, ctx->data - 1);
        return FALSE;
    }
    augmentation = (const char*)ctx->data;
    ctx->data += strlen(augmentation) + 1;

    info->code_align = dwarf2_leb128_as_unsigned(ctx);
    info->data_align = dwarf2_leb128_as_signed(ctx);
    info->retaddr_reg = dwarf2_parse_byte(ctx);
    info->state.cfa_rule = RULE_CFA_OFFSET;

    end = NULL;
    TRACE("\tparsing augmentation %s\n", augmentation);
    if (*augmentation) do
    {
        switch (*augmentation)
        {
        case 'z':
            len = dwarf2_leb128_as_unsigned(ctx);
            end = ctx->data + len;
            info->aug_z_format = 1;
            continue;
        case 'L':
            info->lsda_encoding = dwarf2_parse_byte(ctx);
            continue;
        case 'P':
        {
            unsigned char encoding = dwarf2_parse_byte(ctx);
            /* throw away the indirect bit, as we don't care for the result */
            encoding &= ~DW_EH_PE_indirect;
            dwarf2_parse_augmentation_ptr(ctx, encoding); /* handler */
            continue;
        }
        case 'R':
            info->fde_encoding = dwarf2_parse_byte(ctx);
            continue;
        case 'S':
            info->signal_frame = 1;
            continue;
        }
        FIXME("unknown augmentation '%c'\n", *augmentation);
        if (!end) return FALSE;
        break;
    } while (*++augmentation);
    if (end) ctx->data = end;
    return TRUE;
}

static BOOL dwarf2_get_cie(unsigned long addr, struct module* module, DWORD_PTR delta,
                           dwarf2_traverse_context_t* fde_ctx, dwarf2_traverse_context_t* cie_ctx,
                           struct frame_info* info, BOOL in_eh_frame)
{
    const unsigned char*        ptr_blk;
    const unsigned char*        cie_ptr;
    const unsigned char*        last_cie_ptr = (const unsigned char*)~0;
    unsigned                    len, id;
    unsigned long               start, range;
    unsigned                    cie_id;
    const BYTE*                 start_data = fde_ctx->data;

    cie_id = in_eh_frame ? 0 : DW_CIE_ID;
    /* skip 0-padding at beginning of section (alignment) */
    while (fde_ctx->data + 2 * 4 < fde_ctx->end_data)
    {
        if (dwarf2_parse_u4(fde_ctx))
        {
            fde_ctx->data -= 4;
            break;
        }
    }
    for (; fde_ctx->data + 2 * 4 < fde_ctx->end_data; fde_ctx->data = ptr_blk)
    {
        /* find the FDE for address addr (skip CIE) */
        len = dwarf2_parse_u4(fde_ctx);
        if (len == 0xffffffff) FIXME("Unsupported yet 64-bit CIEs\n");
        ptr_blk = fde_ctx->data + len;
        id  = dwarf2_parse_u4(fde_ctx);
        if (id == cie_id)
        {
            last_cie_ptr = fde_ctx->data - 8;
            /* we need some bits out of the CIE in order to parse all contents */
            if (!parse_cie_details(fde_ctx, info)) return FALSE;
            cie_ctx->data = fde_ctx->data;
            cie_ctx->end_data = ptr_blk;
            cie_ctx->word_size = fde_ctx->word_size;
            continue;
        }
        cie_ptr = (in_eh_frame) ? fde_ctx->data - id - 4 : start_data + id;
        if (cie_ptr != last_cie_ptr)
        {
            last_cie_ptr = cie_ptr;
            cie_ctx->data = cie_ptr;
            cie_ctx->word_size = fde_ctx->word_size;
            cie_ctx->end_data = cie_ptr + 4;
            cie_ctx->end_data = cie_ptr + 4 + dwarf2_parse_u4(cie_ctx);
            if (dwarf2_parse_u4(cie_ctx) != cie_id)
            {
                FIXME("wrong CIE pointer at %x from FDE %x\n",
                      (unsigned)(cie_ptr - start_data),
                      (unsigned)(fde_ctx->data - start_data));
                return FALSE;
            }
            if (!parse_cie_details(cie_ctx, info)) return FALSE;
        }
        start = delta + dwarf2_parse_augmentation_ptr(fde_ctx, info->fde_encoding);
        range = dwarf2_parse_augmentation_ptr(fde_ctx, info->fde_encoding & 0x0F);

        if (addr >= start && addr < start + range)
        {
            /* reset the FDE context */
            fde_ctx->end_data = ptr_blk;

            info->ip = start;
            return TRUE;
        }
    }
    return FALSE;
}

static int valid_reg(ULONG_PTR reg)
{
    if (reg >= NB_FRAME_REGS) FIXME("unsupported reg %lx\n", reg);
    return (reg < NB_FRAME_REGS);
}

static void execute_cfa_instructions(dwarf2_traverse_context_t* ctx,
                                     ULONG_PTR last_ip, struct frame_info *info)
{
    while (ctx->data < ctx->end_data && info->ip <= last_ip + info->signal_frame)
    {
        enum dwarf_call_frame_info op = dwarf2_parse_byte(ctx);

        if (op & 0xc0)
        {
            switch (op & 0xc0)
            {
            case DW_CFA_advance_loc:
            {
                ULONG_PTR offset = (op & 0x3f) * info->code_align;
                TRACE("%lx: DW_CFA_advance_loc %lu\n", info->ip, offset);
                info->ip += offset;
                break;
            }
            case DW_CFA_offset:
            {
                ULONG_PTR reg = op & 0x3f;
                LONG_PTR offset = dwarf2_leb128_as_unsigned(ctx) * info->data_align;
                if (!valid_reg(reg)) break;
                TRACE("%lx: DW_CFA_offset %s, %ld\n",
                      info->ip,
                      dbghelp_current_cpu->fetch_regname(dbghelp_current_cpu->map_dwarf_register(reg)),
                      offset);
                info->state.regs[reg]  = offset;
                info->state.rules[reg] = RULE_CFA_OFFSET;
                break;
            }
            case DW_CFA_restore:
            {
                ULONG_PTR reg = op & 0x3f;
                if (!valid_reg(reg)) break;
                TRACE("%lx: DW_CFA_restore %s\n",
                      info->ip,
                      dbghelp_current_cpu->fetch_regname(dbghelp_current_cpu->map_dwarf_register(reg)));
                info->state.rules[reg] = RULE_UNSET;
                break;
            }
            }
        }
        else switch (op)
        {
        case DW_CFA_nop:
            break;
        case DW_CFA_set_loc:
        {
            ULONG_PTR loc = dwarf2_parse_augmentation_ptr(ctx, info->fde_encoding);
            TRACE("%lx: DW_CFA_set_loc %lx\n", info->ip, loc);
            info->ip = loc;
            break;
        }
        case DW_CFA_advance_loc1:
        {
            ULONG_PTR offset = dwarf2_parse_byte(ctx) * info->code_align;
            TRACE("%lx: DW_CFA_advance_loc1 %lu\n", info->ip, offset);
            info->ip += offset;
            break;
        }
        case DW_CFA_advance_loc2:
        {
            ULONG_PTR offset = dwarf2_parse_u2(ctx) * info->code_align;
            TRACE("%lx: DW_CFA_advance_loc2 %lu\n", info->ip, offset);
            info->ip += offset;
            break;
        }
        case DW_CFA_advance_loc4:
        {
            ULONG_PTR offset = dwarf2_parse_u4(ctx) * info->code_align;
            TRACE("%lx: DW_CFA_advance_loc4 %lu\n", info->ip, offset);
            info->ip += offset;
            break;
        }
        case DW_CFA_offset_extended:
        case DW_CFA_offset_extended_sf:
        {
            ULONG_PTR reg = dwarf2_leb128_as_unsigned(ctx);
            LONG_PTR offset = (op == DW_CFA_offset_extended) ? dwarf2_leb128_as_unsigned(ctx) * info->data_align
                                                             : dwarf2_leb128_as_signed(ctx) * info->data_align;
            if (!valid_reg(reg)) break;
            TRACE("%lx: DW_CFA_offset_extended %s, %ld\n",
                  info->ip,
                  dbghelp_current_cpu->fetch_regname(dbghelp_current_cpu->map_dwarf_register(reg)),
                  offset);
            info->state.regs[reg]  = offset;
            info->state.rules[reg] = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_restore_extended:
        {
            ULONG_PTR reg = dwarf2_leb128_as_unsigned(ctx);
            if (!valid_reg(reg)) break;
            TRACE("%lx: DW_CFA_restore_extended %s\n",
                  info->ip,
                  dbghelp_current_cpu->fetch_regname(dbghelp_current_cpu->map_dwarf_register(reg)));
            info->state.rules[reg] = RULE_UNSET;
            break;
        }
        case DW_CFA_undefined:
        {
            ULONG_PTR reg = dwarf2_leb128_as_unsigned(ctx);
            if (!valid_reg(reg)) break;
            TRACE("%lx: DW_CFA_undefined %s\n",
                  info->ip,
                  dbghelp_current_cpu->fetch_regname(dbghelp_current_cpu->map_dwarf_register(reg)));
            info->state.rules[reg] = RULE_UNDEFINED;
            break;
        }
        case DW_CFA_same_value:
        {
            ULONG_PTR reg = dwarf2_leb128_as_unsigned(ctx);
            if (!valid_reg(reg)) break;
            TRACE("%lx: DW_CFA_same_value %s\n",
                  info->ip,
                  dbghelp_current_cpu->fetch_regname(dbghelp_current_cpu->map_dwarf_register(reg)));
            info->state.regs[reg]  = reg;
            info->state.rules[reg] = RULE_SAME;
            break;
        }
        case DW_CFA_register:
        {
            ULONG_PTR reg = dwarf2_leb128_as_unsigned(ctx);
            ULONG_PTR reg2 = dwarf2_leb128_as_unsigned(ctx);
            if (!valid_reg(reg) || !valid_reg(reg2)) break;
            TRACE("%lx: DW_CFA_register %s == %s\n",
                  info->ip,
                  dbghelp_current_cpu->fetch_regname(dbghelp_current_cpu->map_dwarf_register(reg)),
                  dbghelp_current_cpu->fetch_regname(dbghelp_current_cpu->map_dwarf_register(reg2)));
            info->state.regs[reg]  = reg2;
            info->state.rules[reg] = RULE_OTHER_REG;
            break;
        }
        case DW_CFA_remember_state:
            TRACE("%lx: DW_CFA_remember_state\n", info->ip);
            if (info->state_sp >= MAX_SAVED_STATES)
                FIXME("%lx: DW_CFA_remember_state too many nested saves\n", info->ip);
            else
                info->state_stack[info->state_sp++] = info->state;
            break;
        case DW_CFA_restore_state:
            TRACE("%lx: DW_CFA_restore_state\n", info->ip);
            if (!info->state_sp)
                FIXME("%lx: DW_CFA_restore_state without corresponding save\n", info->ip);
            else
                info->state = info->state_stack[--info->state_sp];
            break;
        case DW_CFA_def_cfa:
        case DW_CFA_def_cfa_sf:
        {
            ULONG_PTR reg = dwarf2_leb128_as_unsigned(ctx);
            ULONG_PTR offset = (op == DW_CFA_def_cfa) ? dwarf2_leb128_as_unsigned(ctx)
                                                      : dwarf2_leb128_as_signed(ctx) * info->data_align;
            if (!valid_reg(reg)) break;
            TRACE("%lx: DW_CFA_def_cfa %s, %ld\n",
                  info->ip,
                  dbghelp_current_cpu->fetch_regname(dbghelp_current_cpu->map_dwarf_register(reg)),
                  offset);
            info->state.cfa_reg    = reg;
            info->state.cfa_offset = offset;
            info->state.cfa_rule   = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_def_cfa_register:
        {
            ULONG_PTR reg = dwarf2_leb128_as_unsigned(ctx);
            if (!valid_reg(reg)) break;
            TRACE("%lx: DW_CFA_def_cfa_register %s\n",
                  info->ip,
                  dbghelp_current_cpu->fetch_regname(dbghelp_current_cpu->map_dwarf_register(reg)));
            info->state.cfa_reg  = reg;
            info->state.cfa_rule = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_def_cfa_offset:
        case DW_CFA_def_cfa_offset_sf:
        {
            ULONG_PTR offset = (op == DW_CFA_def_cfa_offset) ? dwarf2_leb128_as_unsigned(ctx)
                                                             : dwarf2_leb128_as_signed(ctx) * info->data_align;
            TRACE("%lx: DW_CFA_def_cfa_offset %ld\n", info->ip, offset);
            info->state.cfa_offset = offset;
            info->state.cfa_rule   = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_def_cfa_expression:
        {
            ULONG_PTR expr = (ULONG_PTR)ctx->data;
            ULONG_PTR len = dwarf2_leb128_as_unsigned(ctx);
            TRACE("%lx: DW_CFA_def_cfa_expression %lx-%lx\n", info->ip, expr, expr+len);
            info->state.cfa_offset = expr;
            info->state.cfa_rule   = RULE_VAL_EXPRESSION;
            ctx->data += len;
            break;
        }
        case DW_CFA_expression:
        case DW_CFA_val_expression:
        {
            ULONG_PTR reg = dwarf2_leb128_as_unsigned(ctx);
            ULONG_PTR expr = (ULONG_PTR)ctx->data;
            ULONG_PTR len = dwarf2_leb128_as_unsigned(ctx);
            if (!valid_reg(reg)) break;
            TRACE("%lx: DW_CFA_%sexpression %s %lx-%lx\n",
                  info->ip, (op == DW_CFA_expression) ? "" : "val_",
                  dbghelp_current_cpu->fetch_regname(dbghelp_current_cpu->map_dwarf_register(reg)),
                  expr, expr + len);
            info->state.regs[reg]  = expr;
            info->state.rules[reg] = (op == DW_CFA_expression) ? RULE_EXPRESSION : RULE_VAL_EXPRESSION;
            ctx->data += len;
            break;
        }
        case DW_CFA_GNU_args_size:
        /* FIXME: should check that GCC is the compiler for this CU */
        {
            ULONG_PTR   args = dwarf2_leb128_as_unsigned(ctx);
            TRACE("%lx: DW_CFA_GNU_args_size %lu\n", info->ip, args);
            /* ignored */
            break;
        }
        default:
            FIXME("%lx: unknown CFA opcode %02x\n", info->ip, op);
            break;
        }
    }
}

/* retrieve a context register from its dwarf number */
static ULONG_PTR get_context_reg(CONTEXT *context, ULONG_PTR dw_reg)
{
    unsigned regno = dbghelp_current_cpu->map_dwarf_register(dw_reg), sz;
    ULONG_PTR* ptr = dbghelp_current_cpu->fetch_context_reg(context, regno, &sz);

    if (sz != sizeof(ULONG_PTR))
    {
        FIXME("reading register %lu/%u of wrong size %u\n", dw_reg, regno, sz);
        return 0;
    }
    return *ptr;
}

/* set a context register from its dwarf number */
static void set_context_reg(struct cpu_stack_walk* csw, CONTEXT *context, ULONG_PTR dw_reg,
                            ULONG_PTR val, BOOL isdebuggee)
{
    unsigned regno = dbghelp_current_cpu->map_dwarf_register(dw_reg), sz;
    ULONG_PTR* ptr = dbghelp_current_cpu->fetch_context_reg(context, regno, &sz);

    if (isdebuggee)
    {
        char    tmp[16];

        if (sz > sizeof(tmp))
        {
            FIXME("register %lu/%u size is too wide: %u\n", dw_reg, regno, sz);
            return;
        }
        if (!sw_read_mem(csw, val, tmp, sz))
        {
            WARN("Couldn't read memory at %p\n", (void*)val);
            return;
        }
        memcpy(ptr, tmp, sz);
    }
    else
    {
        if (sz != sizeof(ULONG_PTR))
        {
            FIXME("assigning to register %lu/%u of wrong size %u\n", dw_reg, regno, sz);
            return;
        }
        *ptr = val;
    }
}

/* copy a register from one context to another using dwarf number */
static void copy_context_reg(CONTEXT *dstcontext, ULONG_PTR dwregdst, CONTEXT* srccontext, ULONG_PTR dwregsrc)
{
    unsigned regdstno = dbghelp_current_cpu->map_dwarf_register(dwregdst), szdst;
    unsigned regsrcno = dbghelp_current_cpu->map_dwarf_register(dwregsrc), szsrc;
    ULONG_PTR* ptrdst = dbghelp_current_cpu->fetch_context_reg(dstcontext, regdstno, &szdst);
    ULONG_PTR* ptrsrc = dbghelp_current_cpu->fetch_context_reg(srccontext, regsrcno, &szsrc);

    if (szdst != szsrc)
    {
        FIXME("Cannot copy register %lu/%u => %lu/%u because of size mismatch (%u => %u)\n",
              dwregsrc, regsrcno, dwregdst, regdstno, szsrc, szdst);
        return;
    }
    memcpy(ptrdst, ptrsrc, szdst);
}

static ULONG_PTR eval_expression(const struct module* module, struct cpu_stack_walk* csw,
                                 const unsigned char* zp, CONTEXT *context)
{
    dwarf2_traverse_context_t    ctx;
    ULONG_PTR reg, sz, tmp, stack[64];
    int sp = -1;
    ULONG_PTR len;

    ctx.data = zp;
    ctx.end_data = zp + 4;
    len = dwarf2_leb128_as_unsigned(&ctx);
    ctx.end_data = ctx.data + len;
    ctx.word_size = module->format_info[DFI_DWARF]->u.dwarf2_info->word_size;

    while (ctx.data < ctx.end_data)
    {
        unsigned char opcode = dwarf2_parse_byte(&ctx);

        if (opcode >= DW_OP_lit0 && opcode <= DW_OP_lit31)
            stack[++sp] = opcode - DW_OP_lit0;
        else if (opcode >= DW_OP_reg0 && opcode <= DW_OP_reg31)
            stack[++sp] = get_context_reg(context, opcode - DW_OP_reg0);
        else if (opcode >= DW_OP_breg0 && opcode <= DW_OP_breg31)
            stack[++sp] = get_context_reg(context, opcode - DW_OP_breg0) + dwarf2_leb128_as_signed(&ctx);
        else switch (opcode)
        {
        case DW_OP_nop:         break;
        case DW_OP_addr:        stack[++sp] = dwarf2_parse_addr(&ctx); break;
        case DW_OP_const1u:     stack[++sp] = dwarf2_parse_byte(&ctx); break;
        case DW_OP_const1s:     stack[++sp] = (signed char)dwarf2_parse_byte(&ctx); break;
        case DW_OP_const2u:     stack[++sp] = dwarf2_parse_u2(&ctx); break;
        case DW_OP_const2s:     stack[++sp] = (short)dwarf2_parse_u2(&ctx); break;
        case DW_OP_const4u:     stack[++sp] = dwarf2_parse_u4(&ctx); break;
        case DW_OP_const4s:     stack[++sp] = (signed int)dwarf2_parse_u4(&ctx); break;
        case DW_OP_const8u:     stack[++sp] = dwarf2_parse_u8(&ctx); break;
        case DW_OP_const8s:     stack[++sp] = (LONG_PTR)dwarf2_parse_u8(&ctx); break;
        case DW_OP_constu:      stack[++sp] = dwarf2_leb128_as_unsigned(&ctx); break;
        case DW_OP_consts:      stack[++sp] = dwarf2_leb128_as_signed(&ctx); break;
        case DW_OP_deref:
            if (!sw_read_mem(csw, stack[sp], &tmp, sizeof(tmp)))
            {
                ERR("Couldn't read memory at %lx\n", stack[sp]);
                tmp = 0;
            }
            stack[sp] = tmp;
            break;
        case DW_OP_dup:         stack[sp + 1] = stack[sp]; sp++; break;
        case DW_OP_drop:        sp--; break;
        case DW_OP_over:        stack[sp + 1] = stack[sp - 1]; sp++; break;
        case DW_OP_pick:        stack[sp + 1] = stack[sp - dwarf2_parse_byte(&ctx)]; sp++; break;
        case DW_OP_swap:        tmp = stack[sp]; stack[sp] = stack[sp-1]; stack[sp-1] = tmp; break;
        case DW_OP_rot:         tmp = stack[sp]; stack[sp] = stack[sp-1]; stack[sp-1] = stack[sp-2]; stack[sp-2] = tmp; break;
        case DW_OP_abs:         stack[sp] = labs(stack[sp]); break;
        case DW_OP_neg:         stack[sp] = -stack[sp]; break;
        case DW_OP_not:         stack[sp] = ~stack[sp]; break;
        case DW_OP_and:         stack[sp-1] &= stack[sp]; sp--; break;
        case DW_OP_or:          stack[sp-1] |= stack[sp]; sp--; break;
        case DW_OP_minus:       stack[sp-1] -= stack[sp]; sp--; break;
        case DW_OP_mul:         stack[sp-1] *= stack[sp]; sp--; break;
        case DW_OP_plus:        stack[sp-1] += stack[sp]; sp--; break;
        case DW_OP_xor:         stack[sp-1] ^= stack[sp]; sp--; break;
        case DW_OP_shl:         stack[sp-1] <<= stack[sp]; sp--; break;
        case DW_OP_shr:         stack[sp-1] >>= stack[sp]; sp--; break;
        case DW_OP_plus_uconst: stack[sp] += dwarf2_leb128_as_unsigned(&ctx); break;
        case DW_OP_shra:        stack[sp-1] = (LONG_PTR)stack[sp-1] / (1 << stack[sp]); sp--; break;
        case DW_OP_div:         stack[sp-1] = (LONG_PTR)stack[sp-1] / (LONG_PTR)stack[sp]; sp--; break;
        case DW_OP_mod:         stack[sp-1] = (LONG_PTR)stack[sp-1] % (LONG_PTR)stack[sp]; sp--; break;
        case DW_OP_ge:          stack[sp-1] = ((LONG_PTR)stack[sp-1] >= (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_gt:          stack[sp-1] = ((LONG_PTR)stack[sp-1] >  (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_le:          stack[sp-1] = ((LONG_PTR)stack[sp-1] <= (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_lt:          stack[sp-1] = ((LONG_PTR)stack[sp-1] <  (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_eq:          stack[sp-1] = (stack[sp-1] == stack[sp]); sp--; break;
        case DW_OP_ne:          stack[sp-1] = (stack[sp-1] != stack[sp]); sp--; break;
        case DW_OP_skip:        tmp = (short)dwarf2_parse_u2(&ctx); ctx.data += tmp; break;
        case DW_OP_bra:         tmp = (short)dwarf2_parse_u2(&ctx); if (!stack[sp--]) ctx.data += tmp; break;
        case DW_OP_GNU_encoded_addr:
            tmp = dwarf2_parse_byte(&ctx);
            stack[++sp] = dwarf2_parse_augmentation_ptr(&ctx, tmp);
            break;
        case DW_OP_regx:
            stack[++sp] = get_context_reg(context, dwarf2_leb128_as_unsigned(&ctx));
            break;
        case DW_OP_bregx:
            reg = dwarf2_leb128_as_unsigned(&ctx);
            tmp = dwarf2_leb128_as_signed(&ctx);
            stack[++sp] = get_context_reg(context, reg) + tmp;
            break;
        case DW_OP_deref_size:
            sz = dwarf2_parse_byte(&ctx);
            if (!sw_read_mem(csw, stack[sp], &tmp, sz))
            {
                ERR("Couldn't read memory at %lx\n", stack[sp]);
                tmp = 0;
            }
            /* do integral promotion */
            switch (sz)
            {
            case 1: stack[sp] = *(unsigned char*)&tmp; break;
            case 2: stack[sp] = *(unsigned short*)&tmp; break;
            case 4: stack[sp] = *(unsigned int*)&tmp; break;
            case 8: stack[sp] = *(ULONG_PTR*)&tmp; break; /* FIXME: won't work on 32bit platform */
            default: FIXME("Unknown size for deref 0x%lx\n", sz);
            }
            break;
        default:
            FIXME("unhandled opcode %02x\n", opcode);
        }
    }
    return stack[sp];
}

static void apply_frame_state(const struct module* module, struct cpu_stack_walk* csw,
                              CONTEXT *context, struct frame_state *state, ULONG_PTR* cfa)
{
    unsigned int i;
    ULONG_PTR value;
    CONTEXT new_context = *context;

    switch (state->cfa_rule)
    {
    case RULE_EXPRESSION:
        *cfa = eval_expression(module, csw, (const unsigned char*)state->cfa_offset, context);
        if (!sw_read_mem(csw, *cfa, cfa, sizeof(*cfa)))
        {
            WARN("Couldn't read memory at %p\n", (void*)*cfa);
            return;
        }
        break;
    case RULE_VAL_EXPRESSION:
        *cfa = eval_expression(module, csw, (const unsigned char*)state->cfa_offset, context);
        break;
    default:
        *cfa = get_context_reg(context, state->cfa_reg) + state->cfa_offset;
        break;
    }
    if (!*cfa) return;

    for (i = 0; i < NB_FRAME_REGS; i++)
    {
        switch (state->rules[i])
        {
        case RULE_UNSET:
        case RULE_UNDEFINED:
        case RULE_SAME:
            break;
        case RULE_CFA_OFFSET:
            set_context_reg(csw, &new_context, i, *cfa + state->regs[i], TRUE);
            break;
        case RULE_OTHER_REG:
            copy_context_reg(&new_context, i, context, state->regs[i]);
            break;
        case RULE_EXPRESSION:
            value = eval_expression(module, csw, (const unsigned char*)state->regs[i], context);
            set_context_reg(csw, &new_context, i, value, TRUE);
            break;
        case RULE_VAL_EXPRESSION:
            value = eval_expression(module, csw, (const unsigned char*)state->regs[i], context);
            set_context_reg(csw, &new_context, i, value, FALSE);
            break;
        }
    }
    *context = new_context;
}

/***********************************************************************
 *           dwarf2_virtual_unwind
 *
 */
BOOL dwarf2_virtual_unwind(struct cpu_stack_walk* csw, ULONG_PTR ip, CONTEXT* context, ULONG_PTR* cfa)
{
    struct module_pair pair;
    struct frame_info info;
    dwarf2_traverse_context_t cie_ctx, fde_ctx;
    struct module_format* modfmt;
    const unsigned char* end;
    DWORD_PTR delta;

    if (!(pair.pcs = process_find_by_handle(csw->hProcess)) ||
        !(pair.requested = module_find_by_addr(pair.pcs, ip, DMT_UNKNOWN)) ||
        !module_get_debug(&pair))
        return FALSE;
    modfmt = pair.effective->format_info[DFI_DWARF];
    if (!modfmt) return FALSE;
    memset(&info, 0, sizeof(info));
    fde_ctx.data = modfmt->u.dwarf2_info->eh_frame.address;
    fde_ctx.end_data = fde_ctx.data + modfmt->u.dwarf2_info->eh_frame.size;
    fde_ctx.word_size = modfmt->u.dwarf2_info->word_size;
    /* let offsets relative to the eh_frame sections be correctly computed, as we'll map
     * in this process the IMAGE section at a different address as the one expected by
     * the image
     */
    delta = pair.effective->module.BaseOfImage + modfmt->u.dwarf2_info->eh_frame.rva -
        (DWORD_PTR)modfmt->u.dwarf2_info->eh_frame.address;
    if (!dwarf2_get_cie(ip, pair.effective, delta, &fde_ctx, &cie_ctx, &info, TRUE))
    {
        fde_ctx.data = modfmt->u.dwarf2_info->debug_frame.address;
        fde_ctx.end_data = fde_ctx.data + modfmt->u.dwarf2_info->debug_frame.size;
        fde_ctx.word_size = modfmt->u.dwarf2_info->word_size;
        delta = pair.effective->reloc_delta;
        if (!dwarf2_get_cie(ip, pair.effective, delta, &fde_ctx, &cie_ctx, &info, FALSE))
        {
            TRACE("Couldn't find information for %lx\n", ip);
            return FALSE;
        }
    }

    TRACE("function %lx/%lx code_align %lu data_align %ld retaddr %s\n",
          ip, info.ip, info.code_align, info.data_align,
          dbghelp_current_cpu->fetch_regname(dbghelp_current_cpu->map_dwarf_register(info.retaddr_reg)));

    /* if at very beginning of function, return and use default unwinder */
    if (ip == info.ip) return FALSE;
    execute_cfa_instructions(&cie_ctx, ip, &info);

    if (info.aug_z_format)  /* get length of augmentation data */
    {
        ULONG_PTR len = dwarf2_leb128_as_unsigned(&fde_ctx);
        end = fde_ctx.data + len;
    }
    else end = NULL;
    dwarf2_parse_augmentation_ptr(&fde_ctx, info.lsda_encoding); /* handler_data */
    if (end) fde_ctx.data = end;

    execute_cfa_instructions(&fde_ctx, ip, &info);
    apply_frame_state(pair.effective, csw, context, &info.state, cfa);

    return TRUE;
}

static void dwarf2_location_compute(struct process* pcs,
                                    const struct module_format* modfmt,
                                    const struct symt_function* func,
                                    struct location* loc)
{
    struct location             frame;
    DWORD_PTR                   ip;
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

        if ((err = loc_compute_frame(pcs, modfmt, func, ip, &frame)) == 0)
        {
            switch (loc->kind)
            {
            case loc_dwarf2_location_list:
                /* Then, if the variable has a location list, find it !! */
                if (dwarf2_lookup_loclist(modfmt,
                                          modfmt->u.dwarf2_info->debug_loc.address + loc->offset,
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
                    lctx.word_size = modfmt->u.dwarf2_info->word_size;
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

#ifdef HAVE_ZLIB
static void *zalloc(void *priv, uInt items, uInt sz)
{
    return HeapAlloc(GetProcessHeap(), 0, items * sz);
}

static void zfree(void *priv, void *addr)
{
    HeapFree(GetProcessHeap(), 0, addr);
}

static inline BOOL dwarf2_init_zsection(dwarf2_section_t* section,
                                        const char* zsectname,
                                        struct image_section_map* ism)
{
    z_stream z;
    LARGE_INTEGER li;
    int res;
    BOOL ret = FALSE;

    BYTE *addr, *sect = (BYTE *)image_map_section(ism);
    size_t sz = image_get_map_size(ism);

    if (sz <= 12 || memcmp(sect, "ZLIB", 4))
    {
        ERR("invalid compressed section %s\n", zsectname);
        goto out;
    }

#ifdef WORDS_BIGENDIAN
    li.u.HighPart = *(DWORD*)&sect[4];
    li.u.LowPart = *(DWORD*)&sect[8];
#else
    li.u.HighPart = RtlUlongByteSwap(*(DWORD*)&sect[4]);
    li.u.LowPart = RtlUlongByteSwap(*(DWORD*)&sect[8]);
#endif

    addr = HeapAlloc(GetProcessHeap(), 0, li.QuadPart);
    if (!addr)
        goto out;

    z.next_in = &sect[12];
    z.avail_in = sz - 12;
    z.opaque = NULL;
    z.zalloc = zalloc;
    z.zfree = zfree;

    res = inflateInit(&z);
    if (res != Z_OK)
    {
        FIXME("inflateInit failed with %i / %s\n", res, z.msg);
        goto out_free;
    }

    do {
        z.next_out = addr + z.total_out;
        z.avail_out = li.QuadPart - z.total_out;
        res = inflate(&z, Z_FINISH);
    } while (z.avail_in && res == Z_STREAM_END);

    if (res != Z_STREAM_END)
    {
        FIXME("Decompression failed with %i / %s\n", res, z.msg);
        goto out_end;
    }

    ret = TRUE;
    section->compressed = TRUE;
    section->address = addr;
    section->rva = image_get_map_rva(ism);
    section->size = z.total_out;

out_end:
    inflateEnd(&z);
out_free:
    if (!ret)
        HeapFree(GetProcessHeap(), 0, addr);
out:
    image_unmap_section(ism);
    return ret;
}

#endif

static inline BOOL dwarf2_init_section(dwarf2_section_t* section, struct image_file_map* fmap,
                                       const char* sectname, const char* zsectname,
                                       struct image_section_map* ism)
{
    struct image_section_map    local_ism;

    if (!ism) ism = &local_ism;

    section->compressed = FALSE;
    if (image_find_section(fmap, sectname, ism))
    {
        section->address = (const BYTE*)image_map_section(ism);
        section->size    = image_get_map_size(ism);
        section->rva     = image_get_map_rva(ism);
        return TRUE;
    }

    section->address = NULL;
    section->size    = 0;
    section->rva     = 0;

    if (zsectname && image_find_section(fmap, zsectname, ism))
    {
#ifdef HAVE_ZLIB
        return dwarf2_init_zsection(section, zsectname, ism);
#else
        FIXME("dbghelp not built with zlib, but compressed section found\n" );
#endif
    }

    return FALSE;
}

static inline void dwarf2_fini_section(dwarf2_section_t* section)
{
    if (section->compressed)
        HeapFree(GetProcessHeap(), 0, (void*)section->address);
}

static void dwarf2_module_remove(struct process* pcs, struct module_format* modfmt)
{
    dwarf2_fini_section(&modfmt->u.dwarf2_info->debug_loc);
    dwarf2_fini_section(&modfmt->u.dwarf2_info->debug_frame);
    HeapFree(GetProcessHeap(), 0, modfmt);
}

BOOL dwarf2_parse(struct module* module, unsigned long load_offset,
                  const struct elf_thunk_area* thunks,
                  struct image_file_map* fmap)
{
    dwarf2_section_t    eh_frame, section[section_max];
    dwarf2_traverse_context_t   mod_ctx;
    struct image_section_map    debug_sect, debug_str_sect, debug_abbrev_sect,
                                debug_line_sect, debug_ranges_sect, eh_frame_sect;
    BOOL                ret = TRUE;
    struct module_format* dwarf2_modfmt;

    dwarf2_init_section(&eh_frame,                fmap, ".eh_frame",     NULL,             &eh_frame_sect);
    dwarf2_init_section(&section[section_debug],  fmap, ".debug_info",   ".zdebug_info",   &debug_sect);
    dwarf2_init_section(&section[section_abbrev], fmap, ".debug_abbrev", ".zdebug_abbrev", &debug_abbrev_sect);
    dwarf2_init_section(&section[section_string], fmap, ".debug_str",    ".zdebug_str",    &debug_str_sect);
    dwarf2_init_section(&section[section_line],   fmap, ".debug_line",   ".zdebug_line",   &debug_line_sect);
    dwarf2_init_section(&section[section_ranges], fmap, ".debug_ranges", ".zdebug_ranges", &debug_ranges_sect);

    /* to do anything useful we need either .eh_frame or .debug_info */
    if ((!eh_frame.address || eh_frame.address == IMAGE_NO_MAP) &&
        (!section[section_debug].address || section[section_debug].address == IMAGE_NO_MAP))
    {
        ret = FALSE;
        goto leave;
    }

    if (fmap->modtype == DMT_ELF && debug_sect.fmap)
    {
        /* debug info might have a different base address than .so file
         * when elf file is prelinked after splitting off debug info
         * adjust symbol base addresses accordingly
         */
        load_offset += fmap->u.elf.elf_start - debug_sect.fmap->u.elf.elf_start;
    }

    TRACE("Loading Dwarf2 information for %s\n", debugstr_w(module->module.ModuleName));

    mod_ctx.data = section[section_debug].address;
    mod_ctx.end_data = mod_ctx.data + section[section_debug].size;
    mod_ctx.word_size = 0; /* will be correctly set later on */

    dwarf2_modfmt = HeapAlloc(GetProcessHeap(), 0,
                              sizeof(*dwarf2_modfmt) + sizeof(*dwarf2_modfmt->u.dwarf2_info));
    if (!dwarf2_modfmt)
    {
        ret = FALSE;
        goto leave;
    }
    dwarf2_modfmt->module = module;
    dwarf2_modfmt->remove = dwarf2_module_remove;
    dwarf2_modfmt->loc_compute = dwarf2_location_compute;
    dwarf2_modfmt->u.dwarf2_info = (struct dwarf2_module_info_s*)(dwarf2_modfmt + 1);
    dwarf2_modfmt->u.dwarf2_info->word_size = 0; /* will be correctly set later on */
    dwarf2_modfmt->module->format_info[DFI_DWARF] = dwarf2_modfmt;

    /* As we'll need later some sections' content, we won't unmap these
     * sections upon existing this function
     */
    dwarf2_init_section(&dwarf2_modfmt->u.dwarf2_info->debug_loc,   fmap, ".debug_loc",   ".zdebug_loc",   NULL);
    dwarf2_init_section(&dwarf2_modfmt->u.dwarf2_info->debug_frame, fmap, ".debug_frame", ".zdebug_frame", NULL);
    dwarf2_modfmt->u.dwarf2_info->eh_frame = eh_frame;

    while (mod_ctx.data < mod_ctx.end_data)
    {
        dwarf2_parse_compilation_unit(section, dwarf2_modfmt->module, thunks, &mod_ctx, load_offset);
    }
    dwarf2_modfmt->module->module.SymType = SymDia;
    dwarf2_modfmt->module->module.CVSig = 'D' | ('W' << 8) | ('A' << 16) | ('R' << 24);
    /* FIXME: we could have a finer grain here */
    dwarf2_modfmt->module->module.GlobalSymbols = TRUE;
    dwarf2_modfmt->module->module.TypeInfo = TRUE;
    dwarf2_modfmt->module->module.SourceIndexed = TRUE;
    dwarf2_modfmt->module->module.Publics = TRUE;

    /* set the word_size for eh_frame parsing */
    dwarf2_modfmt->u.dwarf2_info->word_size = fmap->addr_size / 8;

leave:
    dwarf2_fini_section(&section[section_debug]);
    dwarf2_fini_section(&section[section_abbrev]);
    dwarf2_fini_section(&section[section_string]);
    dwarf2_fini_section(&section[section_line]);
    dwarf2_fini_section(&section[section_ranges]);

    image_unmap_section(&debug_sect);
    image_unmap_section(&debug_abbrev_sect);
    image_unmap_section(&debug_str_sect);
    image_unmap_section(&debug_line_sect);
    image_unmap_section(&debug_ranges_sect);
    if (!ret) image_unmap_section(&eh_frame_sect);

    return ret;
}
