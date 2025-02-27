/*
 * Copyright 2016 Paul Gofman
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


#include "d3dx9_private.h"

#include <float.h>
#include <math.h>
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

enum pres_ops
{
    PRESHADER_OP_NOP,
    PRESHADER_OP_MOV,
    PRESHADER_OP_NEG,
    PRESHADER_OP_RCP,
    PRESHADER_OP_FRC,
    PRESHADER_OP_EXP,
    PRESHADER_OP_LOG,
    PRESHADER_OP_RSQ,
    PRESHADER_OP_SIN,
    PRESHADER_OP_COS,
    PRESHADER_OP_ASIN,
    PRESHADER_OP_ACOS,
    PRESHADER_OP_ATAN,
    PRESHADER_OP_MIN,
    PRESHADER_OP_MAX,
    PRESHADER_OP_LT,
    PRESHADER_OP_GE,
    PRESHADER_OP_ADD,
    PRESHADER_OP_MUL,
    PRESHADER_OP_ATAN2,
    PRESHADER_OP_DIV,
    PRESHADER_OP_CMP,
    PRESHADER_OP_DOT,
    PRESHADER_OP_DOTSWIZ6,
    PRESHADER_OP_DOTSWIZ8,
};

typedef double (*pres_op_func)(double *args, int n);

static double to_signed_nan(double v)
{
    static const union
    {
        ULONG64 ulong64_value;
        double double_value;
    }
    signed_nan =
    {
        0xfff8000000000000
    };

    return isnan(v) ? signed_nan.double_value : v;
}

static double pres_mov(double *args, int n) {return args[0];}
static double pres_add(double *args, int n) {return args[0] + args[1];}
static double pres_mul(double *args, int n) {return args[0] * args[1];}
static double pres_dot(double *args, int n)
{
    int i;
    double sum;

    sum = 0.0;
    for (i = 0; i < n; ++i)
        sum += args[i] * args[i + n];
    return sum;
}

static double pres_dotswiz6(double *args, int n)
{
    return pres_dot(args, 3);
}

static double pres_dotswiz8(double *args, int n)
{
    return pres_dot(args, 4);
}

static double pres_neg(double *args, int n) {return -args[0];}
static double pres_rcp(double *args, int n) {return 1.0 / args[0];}
static double pres_lt(double *args, int n)  {return args[0] < args[1] ? 1.0 : 0.0;}
static double pres_ge(double *args, int n)  {return args[0] >= args[1] ? 1.0 : 0.0;}
static double pres_frc(double *args, int n) {return args[0] - floor(args[0]);}
static double pres_min(double *args, int n) {return fmin(args[0], args[1]);}
static double pres_max(double *args, int n) {return fmax(args[0], args[1]);}
static double pres_cmp(double *args, int n) {return args[0] >= 0.0 ? args[1] : args[2];}
static double pres_sin(double *args, int n) {return sin(args[0]);}
static double pres_cos(double *args, int n) {return cos(args[0]);}
static double pres_rsq(double *args, int n)
{
    double v;

    v = fabs(args[0]);
    if (v == 0.0)
        return INFINITY;
    else
        return 1.0 / sqrt(v);
}
static double pres_exp(double *args, int n) {return pow(2.0, args[0]);}
static double pres_log(double *args, int n)
{
    double v;

    v = fabs(args[0]);
    if (v == 0.0)
        return 0.0;
    else
        return log2(v);
}
static double pres_asin(double *args, int n) {return to_signed_nan(asin(args[0]));}
static double pres_acos(double *args, int n) {return to_signed_nan(acos(args[0]));}
static double pres_atan(double *args, int n) {return atan(args[0]);}
static double pres_atan2(double *args, int n) {return atan2(args[0], args[1]);}

/* According to the test results 'div' operation always returns 0. Compiler does not seem to ever
 * generate it, using rcp + mul instead, so probably it is not implemented in native d3dx. */
static double pres_div(double *args, int n) {return 0.0;}

#define PRES_OPCODE_MASK 0x7ff00000
#define PRES_OPCODE_SHIFT 20
#define PRES_SCALAR_FLAG 0x80000000
#define PRES_NCOMP_MASK  0x0000ffff

#define FOURCC_PRES 0x53455250
#define FOURCC_CLIT 0x54494c43
#define FOURCC_FXLC 0x434c5846
#define FOURCC_PRSI 0x49535250
#define PRES_SIGN 0x46580000

struct op_info
{
    unsigned int opcode;
    char mnem[16];
    unsigned int input_count;
    BOOL func_all_comps;
    pres_op_func func;
};

static const struct op_info pres_op_info[] =
{
    {0x000, "nop", 0, 0, NULL    }, /* PRESHADER_OP_NOP */
    {0x100, "mov", 1, 0, pres_mov}, /* PRESHADER_OP_MOV */
    {0x101, "neg", 1, 0, pres_neg}, /* PRESHADER_OP_NEG */
    {0x103, "rcp", 1, 0, pres_rcp}, /* PRESHADER_OP_RCP */
    {0x104, "frc", 1, 0, pres_frc}, /* PRESHADER_OP_FRC */
    {0x105, "exp", 1, 0, pres_exp}, /* PRESHADER_OP_EXP */
    {0x106, "log", 1, 0, pres_log}, /* PRESHADER_OP_LOG */
    {0x107, "rsq", 1, 0, pres_rsq}, /* PRESHADER_OP_RSQ */
    {0x108, "sin", 1, 0, pres_sin}, /* PRESHADER_OP_SIN */
    {0x109, "cos", 1, 0, pres_cos}, /* PRESHADER_OP_COS */
    {0x10a, "asin", 1, 0, pres_asin}, /* PRESHADER_OP_ASIN */
    {0x10b, "acos", 1, 0, pres_acos}, /* PRESHADER_OP_ACOS */
    {0x10c, "atan", 1, 0, pres_atan}, /* PRESHADER_OP_ATAN */
    {0x200, "min", 2, 0, pres_min}, /* PRESHADER_OP_MIN */
    {0x201, "max", 2, 0, pres_max}, /* PRESHADER_OP_MAX */
    {0x202, "lt",  2, 0, pres_lt }, /* PRESHADER_OP_LT  */
    {0x203, "ge",  2, 0, pres_ge }, /* PRESHADER_OP_GE  */
    {0x204, "add", 2, 0, pres_add}, /* PRESHADER_OP_ADD */
    {0x205, "mul", 2, 0, pres_mul}, /* PRESHADER_OP_MUL */
    {0x206, "atan2", 2, 0, pres_atan2}, /* PRESHADER_OP_ATAN2 */
    {0x208, "div", 2, 0, pres_div}, /* PRESHADER_OP_DIV */
    {0x300, "cmp", 3, 0, pres_cmp}, /* PRESHADER_OP_CMP */
    {0x500, "dot", 2, 1, pres_dot}, /* PRESHADER_OP_DOT */
    {0x70e, "d3ds_dotswiz", 6, 0, pres_dotswiz6}, /* PRESHADER_OP_DOTSWIZ6 */
    {0x70e, "d3ds_dotswiz", 8, 0, pres_dotswiz8}, /* PRESHADER_OP_DOTSWIZ8 */
};

enum pres_value_type
{
    PRES_VT_FLOAT,
    PRES_VT_DOUBLE,
    PRES_VT_INT,
    PRES_VT_BOOL,
    PRES_VT_COUNT
};

static const struct
{
    unsigned int component_size;
    enum pres_value_type type;
}
table_info[] =
{
    {sizeof(double), PRES_VT_DOUBLE}, /* PRES_REGTAB_IMMED */
    {sizeof(float),  PRES_VT_FLOAT }, /* PRES_REGTAB_CONST */
    {sizeof(float),  PRES_VT_FLOAT }, /* PRES_REGTAB_INPUT */
    {sizeof(float),  PRES_VT_FLOAT }, /* PRES_REGTAB_OCONST */
    {sizeof(BOOL),   PRES_VT_BOOL  }, /* PRES_REGTAB_OBCONST */
    {sizeof(int),    PRES_VT_INT,  }, /* PRES_REGTAB_OICONST */
    /* TODO: use double precision for 64 bit */
    {sizeof(float),  PRES_VT_FLOAT }  /* PRES_REGTAB_TEMP */
};

static const char *table_symbol[] =
{
    "imm", "c", "v", "oc", "ob", "oi", "r", "(null)",
};

static const enum pres_reg_tables pres_regset2table[] =
{
    PRES_REGTAB_OBCONST,  /* D3DXRS_BOOL */
    PRES_REGTAB_OICONST,  /* D3DXRS_INT4 */
    PRES_REGTAB_CONST,    /* D3DXRS_FLOAT4 */
    PRES_REGTAB_COUNT,     /* D3DXRS_SAMPLER */
};

static const enum pres_reg_tables shad_regset2table[] =
{
    PRES_REGTAB_OBCONST,  /* D3DXRS_BOOL */
    PRES_REGTAB_OICONST,  /* D3DXRS_INT4 */
    PRES_REGTAB_OCONST,   /* D3DXRS_FLOAT4 */
    PRES_REGTAB_COUNT,     /* D3DXRS_SAMPLER */
};

struct d3dx_pres_reg
{
    enum pres_reg_tables table;
    /* offset is component index, not register index, e. g.
       offset for component c3.y is 13 (3 * 4 + 1) */
    unsigned int offset;
};

struct d3dx_pres_operand
{
    struct d3dx_pres_reg reg;
    struct d3dx_pres_reg index_reg;
};

#define MAX_INPUTS_COUNT 8

struct d3dx_pres_ins
{
    enum pres_ops op;
    /* first input argument is scalar,
       scalar component is propagated */
    BOOL scalar_op;
    unsigned int component_count;
    struct d3dx_pres_operand inputs[MAX_INPUTS_COUNT];
    struct d3dx_pres_operand output;
};

struct const_upload_info
{
    BOOL transpose;
    unsigned int major, minor;
    unsigned int major_stride;
    unsigned int major_count;
    unsigned int count;
    unsigned int minor_remainder;
};

static enum pres_value_type table_type_from_param_type(D3DXPARAMETER_TYPE type)
{
    switch (type)
    {
        case D3DXPT_FLOAT:
            return PRES_VT_FLOAT;
        case D3DXPT_INT:
            return PRES_VT_INT;
        case D3DXPT_BOOL:
            return PRES_VT_BOOL;
        default:
            FIXME("Unsupported type %u.\n", type);
            return PRES_VT_COUNT;
    }
}

static unsigned int get_reg_offset(unsigned int table, unsigned int offset)
{
    return table == PRES_REGTAB_OBCONST ? offset : offset >> 2;
}

static unsigned int get_offset_reg(unsigned int table, unsigned int reg_idx)
{
    return table == PRES_REGTAB_OBCONST ? reg_idx : reg_idx << 2;
}

static unsigned int get_reg_components(unsigned int table)
{
    return get_offset_reg(table, 1);
}

#define PRES_BITMASK_BLOCK_SIZE (sizeof(unsigned int) * 8)

static HRESULT regstore_alloc_table(struct d3dx_regstore *rs, unsigned int table)
{
    unsigned int size;

    size = get_offset_reg(table, rs->table_sizes[table]) * table_info[table].component_size;
    if (size)
    {
        rs->tables[table] = calloc(1, size);
        if (!rs->tables[table])
            return E_OUTOFMEMORY;
    }
    return D3D_OK;
}

static void regstore_free_tables(struct d3dx_regstore *rs)
{
    unsigned int i;

    for (i = 0; i < PRES_REGTAB_COUNT; ++i)
    {
        free(rs->tables[i]);
    }
}

static void regstore_set_values(struct d3dx_regstore *rs, unsigned int table, const void *data,
        unsigned int start_offset, unsigned int count)
{
    BYTE *dst = rs->tables[table];
    const BYTE *src = data;
    unsigned int size;

    dst += start_offset * table_info[table].component_size;
    size = count * table_info[table].component_size;
    assert((src < dst && size <= dst - src) || (src > dst && size <= src - dst));
    memcpy(dst, src, size);
}

static double regstore_get_double(struct d3dx_regstore *rs, unsigned int table, unsigned int offset)
{
    BYTE *p;

    p = (BYTE *)rs->tables[table] + table_info[table].component_size * offset;
    switch (table_info[table].type)
    {
        case PRES_VT_FLOAT:
            return *(float *)p;
        case PRES_VT_DOUBLE:
            return *(double *)p;
        default:
            FIXME("Unexpected preshader input from table %u.\n", table);
            return NAN;
    }
}

static void regstore_set_double(struct d3dx_regstore *rs, unsigned int table, unsigned int offset, double v)
{
    BYTE *p;

    p = (BYTE *)rs->tables[table] + table_info[table].component_size * offset;
    switch (table_info[table].type)
    {
        case PRES_VT_FLOAT : *(float *)p = v; break;
        case PRES_VT_DOUBLE: *(double *)p = v; break;
        case PRES_VT_INT   : *(int *)p = lrint(v); break;
        case PRES_VT_BOOL  : *(BOOL *)p = !!v; break;
        default:
            FIXME("Bad type %u.\n", table_info[table].type);
            break;
    }
}

static void dump_bytecode(void *data, unsigned int size)
{
    unsigned int *bytecode = (unsigned int *)data;
    unsigned int i, j, n;

    size /= sizeof(*bytecode);
    i = 0;
    while (i < size)
    {
        n = min(size - i, 8);
        for (j = 0; j < n; ++j)
            TRACE("0x%08x,", bytecode[i + j]);
        i += n;
        TRACE("\n");
    }
}

static unsigned int *find_bytecode_comment(unsigned int *ptr, unsigned int count,
        unsigned int fourcc, unsigned int *size)
{
    /* Provide at least one value in comment section on non-NULL return. */
    while (count > 2 && (*ptr & 0xffff) == 0xfffe)
    {
        unsigned int section_size;

        section_size = (*ptr >> 16);
        if (!section_size || section_size + 1 > count)
            break;
        if (*(ptr + 1) == fourcc)
        {
            *size = section_size;
            return ptr + 2;
        }
        count -= section_size + 1;
        ptr += section_size + 1;
    }
    return NULL;
}

static unsigned int *parse_pres_reg(unsigned int *ptr, struct d3dx_pres_reg *reg)
{
    static const enum pres_reg_tables reg_table[8] =
    {
        PRES_REGTAB_COUNT, PRES_REGTAB_IMMED, PRES_REGTAB_CONST, PRES_REGTAB_INPUT,
        PRES_REGTAB_OCONST, PRES_REGTAB_OBCONST, PRES_REGTAB_OICONST, PRES_REGTAB_TEMP
    };

    if (*ptr >= ARRAY_SIZE(reg_table) || reg_table[*ptr] == PRES_REGTAB_COUNT)
    {
        FIXME("Unsupported register table %#x.\n", *ptr);
        return NULL;
    }

    reg->table = reg_table[*ptr++];
    reg->offset = *ptr++;
    return ptr;
}

static unsigned int *parse_pres_arg(unsigned int *ptr, unsigned int count, struct d3dx_pres_operand *opr)
{
    if (count < 3 || (*ptr && count < 5))
    {
        WARN("Byte code buffer ends unexpectedly, count %u.\n", count);
        return NULL;
    }

    if (*ptr)
    {
        if (*ptr != 1)
        {
            FIXME("Unknown relative addressing flag, word %#x.\n", *ptr);
            return NULL;
        }
        ptr = parse_pres_reg(ptr + 1, &opr->index_reg);
        if (!ptr)
            return NULL;
    }
    else
    {
        opr->index_reg.table = PRES_REGTAB_COUNT;
        ++ptr;
    }

    ptr = parse_pres_reg(ptr, &opr->reg);

    if (opr->reg.table == PRES_REGTAB_OBCONST)
        opr->reg.offset /= 4;
    return ptr;
}

static unsigned int *parse_pres_ins(unsigned int *ptr, unsigned int count, struct d3dx_pres_ins *ins)
{
    unsigned int ins_code, ins_raw;
    unsigned int input_count;
    unsigned int i;

    if (count < 2)
    {
        WARN("Byte code buffer ends unexpectedly.\n");
        return NULL;
    }

    ins_raw = *ptr++;
    ins_code = (ins_raw & PRES_OPCODE_MASK) >> PRES_OPCODE_SHIFT;
    ins->component_count = ins_raw & PRES_NCOMP_MASK;
    ins->scalar_op = !!(ins_raw & PRES_SCALAR_FLAG);

    if (ins->component_count < 1 || ins->component_count > 4)
    {
        FIXME("Unsupported number of components %u.\n", ins->component_count);
        return NULL;
    }
    input_count = *ptr++;
    count -= 2;
    for (i = 0; i < ARRAY_SIZE(pres_op_info); ++i)
        if (ins_code == pres_op_info[i].opcode && input_count == pres_op_info[i].input_count)
            break;
    if (i == ARRAY_SIZE(pres_op_info))
    {
        FIXME("Unknown opcode %#x, input_count %u, raw %#x.\n", ins_code, input_count, ins_raw);
        return NULL;
    }
    ins->op = i;
    if (input_count > ARRAY_SIZE(ins->inputs))
    {
        FIXME("Actual input args count %u exceeds inputs array size, instruction %s.\n", input_count,
                pres_op_info[i].mnem);
        return NULL;
    }
    for (i = 0; i < input_count; ++i)
    {
        unsigned int *p;

        p = parse_pres_arg(ptr, count, &ins->inputs[i]);
        if (!p)
            return NULL;
        count -= p - ptr;
        ptr = p;
    }
    ptr = parse_pres_arg(ptr, count, &ins->output);
    if (ins->output.index_reg.table != PRES_REGTAB_COUNT)
    {
        FIXME("Relative addressing in output register not supported.\n");
        return NULL;
    }
    if (get_reg_offset(ins->output.reg.table, ins->output.reg.offset
            + (pres_op_info[ins->op].func_all_comps ? 0 : ins->component_count - 1))
            != get_reg_offset(ins->output.reg.table, ins->output.reg.offset))
    {
        FIXME("Instructions outputting multiple registers are not supported.\n");
        return NULL;
    }
    return ptr;
}

static HRESULT get_ctab_constant_desc(ID3DXConstantTable *ctab, D3DXHANDLE hc, D3DXCONSTANT_DESC *desc,
        WORD *constantinfo_reserved)
{
    const struct ctab_constant *constant = d3dx_shader_get_ctab_constant(ctab, hc);

    if (!constant)
    {
        FIXME("Could not get constant desc.\n");
        if (constantinfo_reserved)
            *constantinfo_reserved = 0;
        return D3DERR_INVALIDCALL;
    }
    *desc = constant->desc;
    if (constantinfo_reserved)
        *constantinfo_reserved = constant->constantinfo_reserved;
    return D3D_OK;
}

static void get_const_upload_info(struct d3dx_const_param_eval_output *const_set,
        struct const_upload_info *info)
{
    struct d3dx_parameter *param = const_set->param;
    unsigned int table = const_set->table;

    info->transpose = (const_set->constant_class == D3DXPC_MATRIX_COLUMNS && param->class == D3DXPC_MATRIX_ROWS)
            || (param->class == D3DXPC_MATRIX_COLUMNS && const_set->constant_class == D3DXPC_MATRIX_ROWS);
    if (const_set->constant_class == D3DXPC_MATRIX_COLUMNS)
    {
        info->major = param->columns;
        info->minor = param->rows;
    }
    else
    {
        info->major = param->rows;
        info->minor = param->columns;
    }

    if (get_reg_components(table) == 1)
    {
        unsigned int const_length = get_offset_reg(table, const_set->register_count);

        info->major_stride = info->minor;
        info->major_count = const_length / info->major_stride;
        info->minor_remainder = const_length % info->major_stride;
    }
    else
    {
        info->major_stride = get_reg_components(table);
        info->major_count = const_set->register_count;
        info->minor_remainder = 0;
    }
    info->count = info->major_count * info->minor + info->minor_remainder;
}

#define INITIAL_CONST_SET_SIZE 16

static HRESULT append_const_set(struct d3dx_const_tab *const_tab, struct d3dx_const_param_eval_output *set)
{
    if (const_tab->const_set_count >= const_tab->const_set_size)
    {
        unsigned int new_size;
        struct d3dx_const_param_eval_output *new_alloc;

        if (!const_tab->const_set_size)
        {
            new_size = INITIAL_CONST_SET_SIZE;
            new_alloc = malloc(sizeof(*const_tab->const_set) * new_size);
            if (!new_alloc)
            {
                ERR("Out of memory.\n");
                return E_OUTOFMEMORY;
            }
        }
        else
        {
            new_size = const_tab->const_set_size * 2;
            new_alloc = realloc(const_tab->const_set, sizeof(*const_tab->const_set) * new_size);
            if (!new_alloc)
            {
                ERR("Out of memory.\n");
                return E_OUTOFMEMORY;
            }
        }
        const_tab->const_set = new_alloc;
        const_tab->const_set_size = new_size;
    }
    const_tab->const_set[const_tab->const_set_count++] = *set;
    return D3D_OK;
}

static void append_pres_const_sets_for_shader_input(struct d3dx_const_tab *const_tab,
        struct d3dx_preshader *pres)
{
    unsigned int i;
    struct d3dx_const_param_eval_output const_set = {NULL};

    for (i = 0; i < pres->ins_count; ++i)
    {
        const struct d3dx_pres_ins *ins = &pres->ins[i];
        const struct d3dx_pres_reg *reg = &ins->output.reg;

        if (reg->table == PRES_REGTAB_TEMP)
            continue;

        const_set.register_index = get_reg_offset(reg->table, reg->offset);
        const_set.register_count = 1;
        const_set.table = reg->table;
        const_set.constant_class = D3DXPC_FORCE_DWORD;
        const_set.element_count = 1;
        append_const_set(const_tab, &const_set);
    }
}

static int __cdecl compare_const_set(const void *a, const void *b)
{
    const struct d3dx_const_param_eval_output *r1 = a;
    const struct d3dx_const_param_eval_output *r2 = b;

    if (r1->table != r2->table)
        return r1->table - r2->table;
    return r1->register_index - r2->register_index;
}

static HRESULT merge_const_set_entries(struct d3dx_const_tab *const_tab,
        struct d3dx_parameter *param, unsigned int index)
{
    unsigned int i, start_index = index;
    DWORD *current_data;
    enum pres_reg_tables current_table;
    unsigned int current_start_offset, element_count;
    struct d3dx_const_param_eval_output *first_const;

    if (!const_tab->const_set_count)
        return D3D_OK;

    while (index < const_tab->const_set_count - 1)
    {
        first_const = &const_tab->const_set[index];
        current_data = first_const->param->data;
        current_table = first_const->table;
        current_start_offset = get_offset_reg(current_table, first_const->register_index);
        element_count = 0;
        for (i = index; i < const_tab->const_set_count; ++i)
        {
            struct d3dx_const_param_eval_output *const_set = &const_tab->const_set[i];
            unsigned int count = get_offset_reg(const_set->table,
                    const_set->register_count * const_set->element_count);
            unsigned int start_offset = get_offset_reg(const_set->table, const_set->register_index);

            if (!(const_set->table == current_table && current_start_offset == start_offset
                    && const_set->direct_copy == first_const->direct_copy
                    && current_data == const_set->param->data
                    && (const_set->direct_copy || (first_const->param->type == const_set->param->type
                    && first_const->param->class == const_set->param->class
                    && first_const->param->columns == const_set->param->columns
                    && first_const->param->rows == const_set->param->rows
                    && first_const->register_count == const_set->register_count
                    && (i == const_tab->const_set_count - 1
                    || first_const->param->element_count == const_set->param->element_count)))))
                break;

            current_start_offset += count;
            current_data += const_set->direct_copy ? count : const_set->param->rows
                    * const_set->param->columns * const_set->element_count;
            element_count += const_set->element_count;
        }

        if (i > index + 1)
        {
            TRACE("Merging %u child parameters for %s, not merging %u, direct_copy %#x.\n", i - index,
                    debugstr_a(param->name), const_tab->const_set_count - i, first_const->direct_copy);

            first_const->element_count = element_count;
            if (first_const->direct_copy)
            {
                first_const->element_count = 1;
                if (index == start_index
                        && !(param->type == D3DXPT_VOID && param->class == D3DXPC_STRUCT))
                {
                    if (table_type_from_param_type(param->type) == PRES_VT_COUNT)
                        return D3DERR_INVALIDCALL;
                    first_const->param = param;
                }
                first_const->register_count = get_reg_offset(current_table, current_start_offset)
                        - first_const->register_index;
            }
            memmove(&const_tab->const_set[index + 1], &const_tab->const_set[i],
                    sizeof(*const_tab->const_set) * (const_tab->const_set_count - i));
            const_tab->const_set_count -= i - index - 1;
        }
        else
        {
            TRACE("Not merging %u child parameters for %s, direct_copy %#x.\n",
                    const_tab->const_set_count - i, debugstr_a(param->name), first_const->direct_copy);
        }
        index = i;
    }
    return D3D_OK;
}

static HRESULT init_set_constants_param(struct d3dx_const_tab *const_tab, ID3DXConstantTable *ctab,
        D3DXHANDLE hc, struct d3dx_parameter *param)
{
    D3DXCONSTANT_DESC desc;
    unsigned int const_count, param_count, i;
    BOOL get_element;
    struct d3dx_const_param_eval_output const_set;
    struct const_upload_info info;
    enum pres_value_type table_type;
    HRESULT hr;

    if (FAILED(get_ctab_constant_desc(ctab, hc, &desc, NULL)))
        return D3DERR_INVALIDCALL;

    if (param->element_count)
    {
        param_count = param->element_count;
        const_count = desc.Elements;
        get_element = TRUE;
    }
    else
    {
        if (desc.Elements > 1)
        {
            FIXME("Unexpected number of constant elements %u.\n", desc.Elements);
            return D3DERR_INVALIDCALL;
        }
        param_count = param->member_count;
        const_count = desc.StructMembers;
        get_element = FALSE;
    }
    if (const_count != param_count)
    {
        FIXME("Number of elements or struct members differs between parameter (%u) and constant (%u).\n",
                param_count, const_count);
        return D3DERR_INVALIDCALL;
    }
    if (const_count)
    {
        HRESULT ret = D3D_OK;
        D3DXHANDLE hc_element;
        unsigned int index = const_tab->const_set_count;

        for (i = 0; i < const_count; ++i)
        {
            if (get_element)
                hc_element = ID3DXConstantTable_GetConstantElement(ctab, hc, i);
            else
                hc_element = ID3DXConstantTable_GetConstant(ctab, hc, i);
            if (!hc_element)
            {
                FIXME("Could not get constant.\n");
                hr = D3DERR_INVALIDCALL;
            }
            else
            {
                hr = init_set_constants_param(const_tab, ctab, hc_element, &param->members[i]);
            }
            if (FAILED(hr))
                ret = hr;
        }
        if (FAILED(ret))
            return ret;
        return merge_const_set_entries(const_tab, param, index);
    }

    TRACE("Constant %s, rows %u, columns %u, class %u, bytes %u.\n",
            debugstr_a(desc.Name), desc.Rows, desc.Columns, desc.Class, desc.Bytes);
    TRACE("Parameter %s, rows %u, columns %u, class %u, flags %#x, bytes %u.\n",
            debugstr_a(param->name), param->rows, param->columns, param->class,
            param->flags, param->bytes);

    const_set.element_count = 1;
    const_set.param = param;
    const_set.constant_class = desc.Class;
    if (desc.RegisterSet >= ARRAY_SIZE(shad_regset2table))
    {
        FIXME("Unknown register set %u.\n", desc.RegisterSet);
        return D3DERR_INVALIDCALL;
    }
    const_set.register_index = desc.RegisterIndex;
    const_set.table = const_tab->regset2table[desc.RegisterSet];
    if (const_set.table >= PRES_REGTAB_COUNT)
    {
        ERR("Unexpected register set %u.\n", desc.RegisterSet);
        return D3DERR_INVALIDCALL;
    }
    assert(table_info[const_set.table].component_size == sizeof(unsigned int));
    assert(param->bytes / (param->rows * param->columns) == sizeof(unsigned int));
    const_set.register_count = desc.RegisterCount;
    table_type = table_info[const_set.table].type;
    get_const_upload_info(&const_set, &info);
    if (!info.count)
    {
        TRACE("%s has zero count, skipping.\n", debugstr_a(param->name));
        return D3D_OK;
    }

    if (table_type_from_param_type(param->type) == PRES_VT_COUNT)
        return D3DERR_INVALIDCALL;

    const_set.direct_copy = table_type_from_param_type(param->type) == table_type
            && !info.transpose && info.minor == info.major_stride
            && info.count == get_offset_reg(const_set.table, const_set.register_count)
            && info.count * sizeof(unsigned int) <= param->bytes;
    if (info.minor_remainder && !const_set.direct_copy && !info.transpose)
        FIXME("Incomplete last row for not transposed matrix which cannot be directly copied, parameter %s.\n",
                debugstr_a(param->name));

    if (info.major_count > info.major
            || (info.major_count == info.major && info.minor_remainder))
    {
        WARN("Constant dimensions exceed parameter size.\n");
        return D3DERR_INVALIDCALL;
    }

    if (FAILED(hr = append_const_set(const_tab, &const_set)))
        return hr;

    return D3D_OK;
}

static HRESULT get_constants_desc(unsigned int *byte_code, struct d3dx_const_tab *out,
        struct d3dx_parameters_store *parameters, const char **skip_constants,
        unsigned int skip_constants_count, struct d3dx_preshader *pres)
{
    ID3DXConstantTable *ctab;
    D3DXCONSTANT_DESC *cdesc;
    struct d3dx_parameter **inputs_param;
    D3DXCONSTANTTABLE_DESC desc;
    HRESULT hr;
    D3DXHANDLE hc;
    unsigned int i, j;

    hr = D3DXGetShaderConstantTable((DWORD *)byte_code, &ctab);
    if (FAILED(hr) || !ctab)
    {
        TRACE("Could not get CTAB data, hr %#lx.\n", hr);
        /* returning OK, shaders and preshaders without CTAB are valid */
        return D3D_OK;
    }
    if (FAILED(hr = ID3DXConstantTable_GetDesc(ctab, &desc)))
    {
        FIXME("Could not get CTAB desc, hr %#lx.\n", hr);
        goto cleanup;
    }

    out->inputs = cdesc = malloc(sizeof(*cdesc) * desc.Constants);
    out->inputs_param = inputs_param = malloc(sizeof(*inputs_param) * desc.Constants);
    if (!cdesc || !inputs_param)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    for (i = 0; i < desc.Constants; ++i)
    {
        unsigned int index = out->input_count;
        WORD constantinfo_reserved;

        hc = ID3DXConstantTable_GetConstant(ctab, NULL, i);
        if (!hc)
        {
            FIXME("Null constant handle.\n");
            goto cleanup;
        }
        if (FAILED(hr = get_ctab_constant_desc(ctab, hc, &cdesc[index], &constantinfo_reserved)))
            goto cleanup;
        inputs_param[index] = get_parameter_by_name(parameters, NULL, cdesc[index].Name);
        if (!inputs_param[index])
        {
            WARN("Could not find parameter %s in effect.\n", cdesc[index].Name);
            continue;
        }
        if (cdesc[index].Class == D3DXPC_OBJECT)
        {
            TRACE("Object %s, parameter %p.\n", cdesc[index].Name, inputs_param[index]);
            if (cdesc[index].RegisterSet != D3DXRS_SAMPLER || inputs_param[index]->class != D3DXPC_OBJECT
                    || !is_param_type_sampler(inputs_param[index]->type))
            {
                WARN("Unexpected object type, constant %s.\n", debugstr_a(cdesc[index].Name));
                hr = D3DERR_INVALIDCALL;
                goto cleanup;
            }
            if (max(inputs_param[index]->element_count, 1) < cdesc[index].RegisterCount)
            {
                WARN("Register count exceeds parameter size, constant %s.\n", debugstr_a(cdesc[index].Name));
                hr = D3DERR_INVALIDCALL;
                goto cleanup;
            }
        }
        if (!is_top_level_parameter(inputs_param[index]))
        {
            WARN("Expected top level parameter '%s'.\n", debugstr_a(cdesc[index].Name));
            hr = E_FAIL;
            goto cleanup;
        }

        for (j = 0; j < skip_constants_count; ++j)
        {
            if (!strcmp(cdesc[index].Name, skip_constants[j]))
            {
                if (!constantinfo_reserved)
                {
                    WARN("skip_constants parameter %s is not register bound.\n",
                            cdesc[index].Name);
                    hr = D3DERR_INVALIDCALL;
                    goto cleanup;
                }
                TRACE("Skipping constant %s.\n", cdesc[index].Name);
                break;
            }
        }
        if (j < skip_constants_count)
            continue;
        ++out->input_count;
        if (inputs_param[index]->class == D3DXPC_OBJECT)
            continue;
        if (FAILED(hr = init_set_constants_param(out, ctab, hc, inputs_param[index])))
            goto cleanup;
    }
    if (pres)
        append_pres_const_sets_for_shader_input(out, pres);
    if (out->const_set_count)
    {
        struct d3dx_const_param_eval_output *new_alloc;

        qsort(out->const_set, out->const_set_count, sizeof(*out->const_set), compare_const_set);

        i = 0;
        while (i < out->const_set_count - 1)
        {
            if (out->const_set[i].constant_class == D3DXPC_FORCE_DWORD
                    && out->const_set[i + 1].constant_class == D3DXPC_FORCE_DWORD
                    && out->const_set[i].table == out->const_set[i + 1].table
                    && out->const_set[i].register_index + out->const_set[i].register_count
                    >= out->const_set[i + 1].register_index)
            {
                assert(out->const_set[i].register_index + out->const_set[i].register_count
                        <= out->const_set[i + 1].register_index + 1);
                out->const_set[i].register_count = out->const_set[i + 1].register_index + 1
                        - out->const_set[i].register_index;
                memmove(&out->const_set[i + 1], &out->const_set[i + 2], sizeof(out->const_set[i])
                        * (out->const_set_count - i - 2));
                --out->const_set_count;
            }
            else
            {
                ++i;
            }
        }

        new_alloc = realloc(out->const_set, sizeof(*out->const_set) * out->const_set_count);
        if (new_alloc)
        {
            out->const_set = new_alloc;
            out->const_set_size = out->const_set_count;
        }
        else
        {
            WARN("Out of memory.\n");
        }
    }
cleanup:
    ID3DXConstantTable_Release(ctab);
    return hr;
}

static void update_table_size(unsigned int *table_sizes, unsigned int table, unsigned int max_register)
{
    if (table < PRES_REGTAB_COUNT)
        table_sizes[table] = max(table_sizes[table], max_register + 1);
}

static void update_table_sizes_consts(unsigned int *table_sizes, struct d3dx_const_tab *ctab)
{
    unsigned int i, table, max_register;

    for (i = 0; i < ctab->input_count; ++i)
    {
        if (!ctab->inputs[i].RegisterCount)
            continue;
        max_register = ctab->inputs[i].RegisterIndex + ctab->inputs[i].RegisterCount - 1;
        table = ctab->regset2table[ctab->inputs[i].RegisterSet];
        update_table_size(table_sizes, table, max_register);
    }
}

static void dump_arg(struct d3dx_regstore *rs, const struct d3dx_pres_operand *arg, int component_count)
{
    static const char *xyzw_str = "xyzw";
    unsigned int i, table, reg_offset;

    table = arg->reg.table;
    if (table == PRES_REGTAB_IMMED && arg->index_reg.table == PRES_REGTAB_COUNT)
    {
        TRACE("(");
        for (i = 0; i < component_count; ++i)
            TRACE(i < component_count - 1 ? "%.16e, " : "%.16e",
                    ((double *)rs->tables[PRES_REGTAB_IMMED])[arg->reg.offset + i]);
        TRACE(")");
    }
    else
    {
        reg_offset = get_reg_offset(table, arg->reg.offset);

        if (arg->index_reg.table == PRES_REGTAB_COUNT)
        {
            if (table == PRES_REGTAB_INPUT && reg_offset < 2)
                TRACE("%s%s.", table_symbol[table], reg_offset ? "PSize" : "Pos");
            else
                TRACE("%s%u.", table_symbol[table], reg_offset);
        }
        else
        {
            unsigned int index_reg;

            index_reg = get_reg_offset(arg->index_reg.table, arg->index_reg.offset);
            TRACE("%s[%u + %s%u.%c].", table_symbol[table], reg_offset,
                    table_symbol[arg->index_reg.table], index_reg,
                    xyzw_str[arg->index_reg.offset - get_offset_reg(arg->index_reg.table, index_reg)]);
        }
        for (i = 0; i < component_count; ++i)
            TRACE("%c", xyzw_str[(arg->reg.offset + i) % 4]);
    }
}

static void dump_registers(struct d3dx_const_tab *ctab)
{
    unsigned int table, i;

    for (i = 0; i < ctab->input_count; ++i)
    {
        table = ctab->regset2table[ctab->inputs[i].RegisterSet];
        TRACE("//   %-12s %s%-4u %u\n", ctab->inputs_param[i] ? ctab->inputs_param[i]->name : "(nil)",
                table_symbol[table], ctab->inputs[i].RegisterIndex, ctab->inputs[i].RegisterCount);
    }
}

static void dump_ins(struct d3dx_regstore *rs, const struct d3dx_pres_ins *ins)
{
    unsigned int i;

    TRACE("%s ", pres_op_info[ins->op].mnem);
    dump_arg(rs, &ins->output, pres_op_info[ins->op].func_all_comps ? 1 : ins->component_count);
    for (i = 0; i < pres_op_info[ins->op].input_count; ++i)
    {
        TRACE(", ");
        dump_arg(rs, &ins->inputs[i], ins->scalar_op && !i ? 1 : ins->component_count);
    }
    TRACE("\n");
}

static void dump_preshader(struct d3dx_preshader *pres)
{
    unsigned int i, immediate_count = pres->regs.table_sizes[PRES_REGTAB_IMMED] * 4;
    const double *immediates = pres->regs.tables[PRES_REGTAB_IMMED];

    if (immediate_count)
        TRACE("// Immediates:\n");
    for (i = 0; i < immediate_count; ++i)
    {
        if (!(i % 4))
            TRACE("// ");
        TRACE("%.8e", immediates[i]);
        if (i % 4 == 3)
            TRACE("\n");
        else
            TRACE(", ");
    }
    TRACE("// Preshader registers:\n");
    dump_registers(&pres->inputs);
    TRACE("preshader\n");
    for (i = 0; i < pres->ins_count; ++i)
        dump_ins(&pres->regs, &pres->ins[i]);
}

static HRESULT parse_preshader(struct d3dx_preshader *pres, unsigned int *ptr, unsigned int count,
        struct d3dx_parameters_store *parameters)
{
    unsigned int *p;
    unsigned int i, j, const_count, magic;
    double *dconst;
    HRESULT hr;
    unsigned int saved_word;
    unsigned int section_size;

    magic = *ptr;

    TRACE("Preshader version %#x.\n", *ptr);

    if (!count)
    {
        WARN("Unexpected end of byte code buffer.\n");
        return D3DXERR_INVALIDDATA;
    }

    p = find_bytecode_comment(ptr + 1, count - 1, FOURCC_CLIT, &section_size);
    if (p)
    {
        const_count = *p++;
        if (const_count > (section_size - 1) / (sizeof(double) / sizeof(unsigned int)))
        {
            WARN("Byte code buffer ends unexpectedly.\n");
            return D3DXERR_INVALIDDATA;
        }
        dconst = (double *)p;
    }
    else
    {
        const_count = 0;
        dconst = NULL;
    }
    TRACE("%u double constants.\n", const_count);

    p = find_bytecode_comment(ptr + 1, count - 1, FOURCC_FXLC, &section_size);
    if (!p)
    {
        WARN("Could not find preshader code.\n");
        return D3D_OK;
    }
    pres->ins_count = *p++;
    --section_size;
    if (pres->ins_count > UINT_MAX / sizeof(*pres->ins))
    {
        WARN("Invalid instruction count %u.\n", pres->ins_count);
        return D3DXERR_INVALIDDATA;
    }
    TRACE("%u instructions.\n", pres->ins_count);
    pres->ins = calloc(pres->ins_count, sizeof(*pres->ins));
    if (!pres->ins)
        return E_OUTOFMEMORY;
    for (i = 0; i < pres->ins_count; ++i)
    {
        unsigned int *ptr_next;

        ptr_next = parse_pres_ins(p, section_size, &pres->ins[i]);
        if (!ptr_next)
            return D3DXERR_INVALIDDATA;
        section_size -= ptr_next - p;
        p = ptr_next;
    }

    pres->inputs.regset2table = pres_regset2table;

    saved_word = *ptr;
    *ptr = 0xfffe0000;
    hr = get_constants_desc(ptr, &pres->inputs, parameters, NULL, 0, NULL);
    *ptr = saved_word;
    if (FAILED(hr))
        return hr;

    if (const_count % get_reg_components(PRES_REGTAB_IMMED))
    {
        FIXME("const_count %u is not a multiple of %u.\n", const_count,
                get_reg_components(PRES_REGTAB_IMMED));
        return D3DXERR_INVALIDDATA;
    }
    pres->regs.table_sizes[PRES_REGTAB_IMMED] = get_reg_offset(PRES_REGTAB_IMMED, const_count);
    if (magic == FOURCC_TX_1)
        pres->regs.table_sizes[PRES_REGTAB_INPUT] = 2;

    update_table_sizes_consts(pres->regs.table_sizes, &pres->inputs);
    for (i = 0; i < pres->ins_count; ++i)
    {
        for (j = 0; j < pres_op_info[pres->ins[i].op].input_count; ++j)
        {
            enum pres_reg_tables table;
            unsigned int reg_idx;

            if (pres->ins[i].inputs[j].index_reg.table == PRES_REGTAB_COUNT)
            {
                unsigned int last_component_index = pres->ins[i].scalar_op && !j ? 0
                        : pres->ins[i].component_count - 1;

                table = pres->ins[i].inputs[j].reg.table;
                reg_idx = get_reg_offset(table, pres->ins[i].inputs[j].reg.offset
                        + last_component_index);
            }
            else
            {
                table = pres->ins[i].inputs[j].index_reg.table;
                reg_idx = get_reg_offset(table, pres->ins[i].inputs[j].index_reg.offset);
            }
            if (reg_idx >= pres->regs.table_sizes[table])
            {
                /* Native accepts these broken preshaders. */
                FIXME("Out of bounds register index, i %u, j %u, table %u, reg_idx %u, preshader parsing failed.\n",
                        i, j, table, reg_idx);
                return D3DXERR_INVALIDDATA;
            }
        }
        update_table_size(pres->regs.table_sizes, pres->ins[i].output.reg.table,
                get_reg_offset(pres->ins[i].output.reg.table, pres->ins[i].output.reg.offset));
    }
    if (FAILED(regstore_alloc_table(&pres->regs, PRES_REGTAB_IMMED)))
        return E_OUTOFMEMORY;
    regstore_set_values(&pres->regs, PRES_REGTAB_IMMED, dconst, 0, const_count);

    return D3D_OK;
}

HRESULT d3dx_create_param_eval(struct d3dx_parameters_store *parameters, void *byte_code, unsigned int byte_code_size,
        D3DXPARAMETER_TYPE type, struct d3dx_param_eval **peval_out, ULONG64 *version_counter,
        const char **skip_constants, unsigned int skip_constants_count)
{
    struct d3dx_param_eval *peval;
    unsigned int *ptr, *shader_ptr = NULL;
    unsigned int i;
    BOOL shader;
    unsigned int count, pres_size;
    HRESULT ret;

    TRACE("parameters %p, byte_code %p, byte_code_size %u, type %u, peval_out %p.\n",
            parameters, byte_code, byte_code_size, type, peval_out);

    count = byte_code_size / sizeof(unsigned int);
    if (!byte_code || !count)
    {
        *peval_out = NULL;
        return D3D_OK;
    }

    peval = calloc(1, sizeof(*peval));
    if (!peval)
    {
        ret = E_OUTOFMEMORY;
        goto err_out;
    }
    peval->version_counter = version_counter;

    peval->param_type = type;
    switch (type)
    {
        case D3DXPT_VERTEXSHADER:
        case D3DXPT_PIXELSHADER:
            shader = TRUE;
            break;
        default:
            shader = FALSE;
            break;
    }
    peval->shader_inputs.regset2table = shad_regset2table;

    ptr = (unsigned int *)byte_code;
    if (shader)
    {
        if ((*ptr & 0xfffe0000) != 0xfffe0000)
        {
            FIXME("Invalid shader signature %#x.\n", *ptr);
            ret = D3DXERR_INVALIDDATA;
            goto err_out;
        }
        TRACE("Shader version %#x.\n", *ptr & 0xffff);
        shader_ptr = ptr;
        ptr = find_bytecode_comment(ptr + 1, count - 1, FOURCC_PRES, &pres_size);
        if (!ptr)
            TRACE("No preshader found.\n");
    }
    else
    {
        pres_size = count;
    }

    if (ptr && FAILED(ret = parse_preshader(&peval->pres, ptr, pres_size, parameters)))
    {
        FIXME("Failed parsing preshader, byte code for analysis follows.\n");
        dump_bytecode(byte_code, byte_code_size);
        goto err_out;
    }

    if (shader)
    {
        if (FAILED(ret = get_constants_desc(shader_ptr, &peval->shader_inputs, parameters,
                skip_constants, skip_constants_count, &peval->pres)))
        {
            TRACE("Could not get shader constant table, hr %#lx.\n", ret);
            goto err_out;
        }
        update_table_sizes_consts(peval->pres.regs.table_sizes, &peval->shader_inputs);
    }

    for (i = PRES_REGTAB_FIRST_SHADER; i < PRES_REGTAB_COUNT; ++i)
    {
        if (FAILED(ret = regstore_alloc_table(&peval->pres.regs, i)))
            goto err_out;
    }

    if (TRACE_ON(d3dx))
    {
        dump_bytecode(byte_code, byte_code_size);
        dump_preshader(&peval->pres);
        if (shader)
        {
            TRACE("// Shader registers:\n");
            dump_registers(&peval->shader_inputs);
        }
    }
    *peval_out = peval;
    TRACE("Created parameter evaluator %p.\n", *peval_out);
    return D3D_OK;

err_out:
    WARN("Error creating parameter evaluator.\n");
    if (TRACE_ON(d3dx))
        dump_bytecode(byte_code, byte_code_size);

    d3dx_free_param_eval(peval);
    *peval_out = NULL;
    return ret;
}

static void d3dx_free_const_tab(struct d3dx_const_tab *ctab)
{
    free(ctab->inputs);
    free(ctab->inputs_param);
    free(ctab->const_set);
}

static void d3dx_free_preshader(struct d3dx_preshader *pres)
{
    free(pres->ins);

    regstore_free_tables(&pres->regs);
    d3dx_free_const_tab(&pres->inputs);
}

void d3dx_free_param_eval(struct d3dx_param_eval *peval)
{
    TRACE("peval %p.\n", peval);

    if (!peval)
        return;

    d3dx_free_preshader(&peval->pres);
    d3dx_free_const_tab(&peval->shader_inputs);
    free(peval);
}

static void pres_int_from_float(void *out, const void *in, unsigned int count)
{
    unsigned int i;
    const float *in_float = in;
    int *out_int = out;

    for (i = 0; i < count; ++i)
        out_int[i] = in_float[i];
}

static void pres_bool_from_value(void *out, const void *in, unsigned int count)
{
    unsigned int i;
    const DWORD *in_dword = in;
    BOOL *out_bool = out;

    for (i = 0; i < count; ++i)
        out_bool[i] = !!in_dword[i];
}

static void pres_float_from_int(void *out, const void *in, unsigned int count)
{
    unsigned int i;
    const int *in_int = in;
    float *out_float = out;

    for (i = 0; i < count; ++i)
        out_float[i] = in_int[i];
}

static void pres_float_from_bool(void *out, const void *in, unsigned int count)
{
    unsigned int i;
    const BOOL *in_bool = in;
    float *out_float = out;

    for (i = 0; i < count; ++i)
        out_float[i] = !!in_bool[i];
}

static void pres_int_from_bool(void *out, const void *in, unsigned int count)
{
    unsigned int i;
    const float *in_bool = in;
    int *out_int = out;

    for (i = 0; i < count; ++i)
        out_int[i] = !!in_bool[i];
}

static void regstore_set_data(struct d3dx_regstore *rs, unsigned int table,
        unsigned int offset, const unsigned int *in, unsigned int count, enum pres_value_type param_type)
{
    typedef void (*conv_func)(void *out, const void *in, unsigned int count);
    static const conv_func set_const_funcs[PRES_VT_COUNT][PRES_VT_COUNT] =
    {
        {NULL,                 NULL, pres_int_from_float, pres_bool_from_value},
        {NULL,                 NULL, NULL,                NULL},
        {pres_float_from_int,  NULL, NULL,                pres_bool_from_value},
        {pres_float_from_bool, NULL, pres_int_from_bool,  NULL}
    };
    enum pres_value_type table_type = table_info[table].type;

    if (param_type == table_type)
    {
        regstore_set_values(rs, table, in, offset, count);
        return;
    }

    set_const_funcs[param_type][table_type]((unsigned int *)rs->tables[table] + offset, in, count);
}

static HRESULT set_constants_device(ID3DXEffectStateManager *manager, struct IDirect3DDevice9 *device,
        D3DXPARAMETER_TYPE type, enum pres_reg_tables table, void *ptr,
        unsigned int start, unsigned int count)
{
    if (type == D3DXPT_VERTEXSHADER)
    {
        switch(table)
        {
            case PRES_REGTAB_OCONST:
                return SET_D3D_STATE_(manager, device, SetVertexShaderConstantF, start, ptr, count);
            case PRES_REGTAB_OICONST:
                return SET_D3D_STATE_(manager, device, SetVertexShaderConstantI, start, ptr, count);
            case PRES_REGTAB_OBCONST:
                return SET_D3D_STATE_(manager, device, SetVertexShaderConstantB, start, ptr, count);
            default:
                FIXME("Unexpected register table %u.\n", table);
                return D3DERR_INVALIDCALL;
        }
    }
    else if (type == D3DXPT_PIXELSHADER)
    {
        switch(table)
        {
            case PRES_REGTAB_OCONST:
                return SET_D3D_STATE_(manager, device, SetPixelShaderConstantF, start, ptr, count);
            case PRES_REGTAB_OICONST:
                return SET_D3D_STATE_(manager, device, SetPixelShaderConstantI, start, ptr, count);
            case PRES_REGTAB_OBCONST:
                return SET_D3D_STATE_(manager, device, SetPixelShaderConstantB, start, ptr, count);
            default:
                FIXME("Unexpected register table %u.\n", table);
                return D3DERR_INVALIDCALL;
        }
    }
    else
    {
        FIXME("Unexpected parameter type %u.\n", type);
        return D3DERR_INVALIDCALL;
    }
}

static HRESULT set_constants(struct d3dx_regstore *rs, struct d3dx_const_tab *const_tab,
        ULONG64 new_update_version, ID3DXEffectStateManager *manager, struct IDirect3DDevice9 *device,
        D3DXPARAMETER_TYPE type, BOOL device_update_all, BOOL pres_dirty)
{
    unsigned int const_idx;
    unsigned int current_start = 0, current_count = 0;
    enum pres_reg_tables current_table = PRES_REGTAB_COUNT;
    BOOL update_device = manager || device;
    HRESULT hr, result = D3D_OK;
    ULONG64 update_version = const_tab->update_version;

    for (const_idx = 0; const_idx < const_tab->const_set_count; ++const_idx)
    {
        struct d3dx_const_param_eval_output *const_set = &const_tab->const_set[const_idx];
        enum pres_reg_tables table = const_set->table;
        struct d3dx_parameter *param = const_set->param;
        unsigned int element, i, j, start_offset;
        struct const_upload_info info;
        unsigned int *data;
        enum pres_value_type param_type;

        if (!(param && is_param_dirty(param, update_version)))
            continue;

        data = param->data;
        start_offset = get_offset_reg(table, const_set->register_index);
        if (const_set->direct_copy)
        {
            regstore_set_values(rs, table, data, start_offset,
                    get_offset_reg(table, const_set->register_count));
            continue;
        }
        param_type = table_type_from_param_type(param->type);
        if (const_set->constant_class == D3DXPC_SCALAR || const_set->constant_class == D3DXPC_VECTOR)
        {
            unsigned int count = max(param->rows, param->columns);

            if (count >= get_reg_components(table))
            {
                regstore_set_data(rs, table, start_offset, data,
                        count * const_set->element_count, param_type);
            }
            else
            {
                for (element = 0; element < const_set->element_count; ++element)
                    regstore_set_data(rs, table, start_offset + get_offset_reg(table, element),
                            &data[element * count], count, param_type);
            }
            continue;
        }
        get_const_upload_info(const_set, &info);
        for (element = 0; element < const_set->element_count; ++element)
        {
            unsigned int *out = (unsigned int *)rs->tables[table] + start_offset;

            /* Store reshaped but (possibly) not converted yet data temporarily in the same constants buffer.
             * All the supported types of parameters and table values have the same size. */
            if (info.transpose)
            {
                for (i = 0; i < info.major_count; ++i)
                    for (j = 0; j < info.minor; ++j)
                        out[i * info.major_stride + j] = data[i + j * info.major];

                for (j = 0; j < info.minor_remainder; ++j)
                    out[i * info.major_stride + j] = data[i + j * info.major];
            }
            else
            {
                for (i = 0; i < info.major_count; ++i)
                    for (j = 0; j < info.minor; ++j)
                        out[i * info.major_stride + j] = data[i * info.minor + j];
            }
            start_offset += get_offset_reg(table, const_set->register_count);
            data += param->rows * param->columns;
        }
        start_offset = get_offset_reg(table, const_set->register_index);
        if (table_info[table].type != param_type)
            regstore_set_data(rs, table, start_offset, (unsigned int *)rs->tables[table] + start_offset,
                    get_offset_reg(table, const_set->register_count) * const_set->element_count, param_type);
    }
    const_tab->update_version = new_update_version;
    if (!update_device)
        return D3D_OK;

    for (const_idx = 0; const_idx < const_tab->const_set_count; ++const_idx)
    {
        struct d3dx_const_param_eval_output *const_set = &const_tab->const_set[const_idx];

        if (device_update_all || (const_set->param
                ? is_param_dirty(const_set->param, update_version) : pres_dirty))
        {
            enum pres_reg_tables table = const_set->table;

            if (table == current_table && current_start + current_count == const_set->register_index)
            {
                current_count += const_set->register_count * const_set->element_count;
            }
            else
            {
                if (current_count)
                {
                    if (FAILED(hr = set_constants_device(manager, device, type, current_table,
                            (DWORD *)rs->tables[current_table]
                            + get_offset_reg(current_table, current_start), current_start, current_count)))
                        result = hr;
                }
                current_table = table;
                current_start = const_set->register_index;
                current_count = const_set->register_count * const_set->element_count;
            }
        }
    }
    if (current_count)
    {
        if (FAILED(hr = set_constants_device(manager, device, type, current_table,
                (DWORD *)rs->tables[current_table]
                + get_offset_reg(current_table, current_start), current_start, current_count)))
            result = hr;
    }
    return result;
}

static double exec_get_reg_value(struct d3dx_regstore *rs, enum pres_reg_tables table, unsigned int offset)
{
    return regstore_get_double(rs, table, offset);
}

static double exec_get_arg(struct d3dx_regstore *rs, const struct d3dx_pres_operand *opr, unsigned int comp)
{
    unsigned int offset, base_index, reg_index, table;

    table = opr->reg.table;

    if (opr->index_reg.table == PRES_REGTAB_COUNT)
        base_index = 0;
    else
        base_index = lrint(exec_get_reg_value(rs, opr->index_reg.table, opr->index_reg.offset));

    offset = get_offset_reg(table, base_index) + opr->reg.offset + comp;
    reg_index = get_reg_offset(table, offset);

    if (reg_index >= rs->table_sizes[table])
    {
        unsigned int wrap_size;

        if (table == PRES_REGTAB_CONST)
        {
            /* As it can be guessed from tests, offset into floating constant table is wrapped
             * to the nearest power of 2 and not to the actual table size. */
            for (wrap_size = 1; wrap_size < rs->table_sizes[table]; wrap_size <<= 1)
                ;
        }
        else
        {
            wrap_size = rs->table_sizes[table];
        }
        WARN("Wrapping register index %u, table %u, wrap_size %u, table size %u.\n",
                reg_index, table, wrap_size, rs->table_sizes[table]);
        reg_index %= wrap_size;

        if (reg_index >= rs->table_sizes[table])
            return 0.0;

        offset = get_offset_reg(table, reg_index) + offset % get_reg_components(table);
    }

    return exec_get_reg_value(rs, table, offset);
}

static void exec_set_arg(struct d3dx_regstore *rs, const struct d3dx_pres_reg *reg,
        unsigned int comp, double res)
{
    regstore_set_double(rs, reg->table, reg->offset + comp, res);
}

#define ARGS_ARRAY_SIZE 8
static HRESULT execute_preshader(struct d3dx_preshader *pres)
{
    unsigned int i, j, k;
    double args[ARGS_ARRAY_SIZE];
    double res;

    for (i = 0; i < pres->ins_count; ++i)
    {
        const struct d3dx_pres_ins *ins;
        const struct op_info *oi;

        ins = &pres->ins[i];
        oi = &pres_op_info[ins->op];
        if (oi->func_all_comps)
        {
            if (oi->input_count * ins->component_count > ARGS_ARRAY_SIZE)
            {
                FIXME("Too many arguments (%u) for one instruction.\n", oi->input_count * ins->component_count);
                return E_FAIL;
            }
            for (k = 0; k < oi->input_count; ++k)
                for (j = 0; j < ins->component_count; ++j)
                    args[k * ins->component_count + j] = exec_get_arg(&pres->regs, &ins->inputs[k],
                            ins->scalar_op && !k ? 0 : j);
            res = oi->func(args, ins->component_count);

            /* only 'dot' instruction currently falls here */
            exec_set_arg(&pres->regs, &ins->output.reg, 0, res);
        }
        else
        {
            for (j = 0; j < ins->component_count; ++j)
            {
                for (k = 0; k < oi->input_count; ++k)
                    args[k] = exec_get_arg(&pres->regs, &ins->inputs[k], ins->scalar_op && !k ? 0 : j);
                res = oi->func(args, ins->component_count);
                exec_set_arg(&pres->regs, &ins->output.reg, j, res);
            }
        }
    }
    return D3D_OK;
}

static BOOL is_const_tab_input_dirty(struct d3dx_const_tab *ctab, ULONG64 update_version)
{
    unsigned int i;

    if (update_version == ULONG64_MAX)
        update_version = ctab->update_version;
    for (i = 0; i < ctab->input_count; ++i)
    {
        if (is_top_level_param_dirty(top_level_parameter_from_parameter(ctab->inputs_param[i]),
                update_version))
            return TRUE;
    }
    return FALSE;
}

BOOL is_param_eval_input_dirty(struct d3dx_param_eval *peval, ULONG64 update_version)
{
    return is_const_tab_input_dirty(&peval->pres.inputs, update_version)
            || is_const_tab_input_dirty(&peval->shader_inputs, update_version);
}

HRESULT d3dx_evaluate_parameter(struct d3dx_param_eval *peval, const struct d3dx_parameter *param,
        void *param_value)
{
    HRESULT hr;
    unsigned int i;
    unsigned int elements, elements_param, elements_table;
    BOOL is_dirty;
    float *oc;

    TRACE("peval %p, param %p, param_value %p.\n", peval, param, param_value);

    if ((is_dirty = is_const_tab_input_dirty(&peval->pres.inputs, ULONG64_MAX)))
    {
        set_constants(&peval->pres.regs, &peval->pres.inputs,
                next_update_version(peval->version_counter), NULL, NULL,
                peval->param_type, FALSE, FALSE);
    }

    if (is_dirty || peval->pres.regs.table_sizes[PRES_REGTAB_INPUT])
    {
        if (FAILED(hr = execute_preshader(&peval->pres)))
            return hr;
    }

    elements_table = get_offset_reg(PRES_REGTAB_OCONST, peval->pres.regs.table_sizes[PRES_REGTAB_OCONST]);
    elements_param = param->bytes / sizeof(unsigned int);
    elements = min(elements_table, elements_param);
    oc = (float *)peval->pres.regs.tables[PRES_REGTAB_OCONST];
    for (i = 0; i < elements; ++i)
        set_number((unsigned int *)param_value + i, param->type, oc + i, D3DXPT_FLOAT);
    return D3D_OK;
}

HRESULT d3dx_param_eval_set_shader_constants(ID3DXEffectStateManager *manager, struct IDirect3DDevice9 *device,
        struct d3dx_param_eval *peval, BOOL update_all)
{
    HRESULT hr;
    struct d3dx_preshader *pres = &peval->pres;
    struct d3dx_regstore *rs = &pres->regs;
    ULONG64 new_update_version = next_update_version(peval->version_counter);
    BOOL pres_dirty = FALSE;

    TRACE("device %p, peval %p, param_type %u.\n", device, peval, peval->param_type);

    if (is_const_tab_input_dirty(&pres->inputs, ULONG64_MAX))
    {
        set_constants(rs, &pres->inputs, new_update_version,
                NULL, NULL, peval->param_type, FALSE, FALSE);
        if (FAILED(hr = execute_preshader(pres)))
            return hr;
        pres_dirty = TRUE;
    }

    return set_constants(rs, &peval->shader_inputs, new_update_version,
            manager, device, peval->param_type, update_all, pres_dirty);
}
