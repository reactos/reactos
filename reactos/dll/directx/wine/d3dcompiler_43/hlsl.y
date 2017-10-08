/*
 * HLSL parser
 *
 * Copyright 2008 Stefan Dösinger
 * Copyright 2012 Matteo Bruni for CodeWeavers
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
%{
#include "config.h"
#include "wine/debug.h"

#include <stdio.h>

#include "d3dcompiler_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(hlsl_parser);

int hlsl_lex(void);

struct hlsl_parse_ctx hlsl_ctx;

struct YYLTYPE;
static void set_location(struct source_location *loc, const struct YYLTYPE *l);

void hlsl_message(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    compilation_message(&hlsl_ctx.messages, fmt, args);
    va_end(args);
}

static const char *hlsl_get_error_level_name(enum hlsl_error_level level)
{
    static const char * const names[] =
    {
        "error",
        "warning",
        "note",
    };
    return names[level];
}

void hlsl_report_message(const char *filename, DWORD line, DWORD column,
        enum hlsl_error_level level, const char *fmt, ...)
{
    va_list args;
    char *string = NULL;
    int rc, size = 0;

    while (1)
    {
        va_start(args, fmt);
        rc = vsnprintf(string, size, fmt, args);
        va_end(args);

        if (rc >= 0 && rc < size)
            break;

        if (rc >= size)
            size = rc + 1;
        else
            size = size ? size * 2 : 32;

        if (!string)
            string = d3dcompiler_alloc(size);
        else
            string = d3dcompiler_realloc(string, size);
        if (!string)
        {
            ERR("Error reallocating memory for a string.\n");
            return;
        }
    }

    hlsl_message("%s:%u:%u: %s: %s\n", filename, line, column, hlsl_get_error_level_name(level), string);
    d3dcompiler_free(string);

    if (level == HLSL_LEVEL_ERROR)
        set_parse_status(&hlsl_ctx.status, PARSE_ERR);
    else if (level == HLSL_LEVEL_WARNING)
        set_parse_status(&hlsl_ctx.status, PARSE_WARN);
}

static void hlsl_error(const char *s)
{
    hlsl_report_message(hlsl_ctx.source_file, hlsl_ctx.line_no, hlsl_ctx.column, HLSL_LEVEL_ERROR, "%s", s);
}

static void debug_dump_decl(struct hlsl_type *type, DWORD modifiers, const char *declname, unsigned int line_no)
{
    TRACE("Line %u: ", line_no);
    if (modifiers)
        TRACE("%s ", debug_modifiers(modifiers));
    TRACE("%s %s;\n", debug_hlsl_type(type), declname);
}

static void check_invalid_matrix_modifiers(DWORD modifiers, struct source_location *loc)
{
    if (modifiers & (HLSL_MODIFIER_ROW_MAJOR | HLSL_MODIFIER_COLUMN_MAJOR))
    {
        hlsl_report_message(loc->file, loc->line, loc->col, HLSL_LEVEL_ERROR,
                "'row_major' or 'column_major' modifiers are only allowed for matrices");
    }
}

static BOOL declare_variable(struct hlsl_ir_var *decl, BOOL local)
{
    BOOL ret;

    TRACE("Declaring variable %s.\n", decl->name);
    if (decl->node.data_type->type == HLSL_CLASS_MATRIX)
    {
        if (!(decl->modifiers & (HLSL_MODIFIER_ROW_MAJOR | HLSL_MODIFIER_COLUMN_MAJOR)))
        {
            decl->modifiers |= hlsl_ctx.matrix_majority == HLSL_ROW_MAJOR
                    ? HLSL_MODIFIER_ROW_MAJOR : HLSL_MODIFIER_COLUMN_MAJOR;
        }
    }
    else
        check_invalid_matrix_modifiers(decl->modifiers, &decl->node.loc);

    if (local)
    {
        DWORD invalid = decl->modifiers & (HLSL_STORAGE_EXTERN | HLSL_STORAGE_SHARED
                | HLSL_STORAGE_GROUPSHARED | HLSL_STORAGE_UNIFORM);
        if (invalid)
        {
            hlsl_report_message(decl->node.loc.file, decl->node.loc.line, decl->node.loc.col, HLSL_LEVEL_ERROR,
                    "modifier '%s' invalid for local variables", debug_modifiers(invalid));
        }
        if (decl->semantic)
        {
            hlsl_report_message(decl->node.loc.file, decl->node.loc.line, decl->node.loc.col, HLSL_LEVEL_ERROR,
                    "semantics are not allowed on local variables");
            return FALSE;
        }
    }
    else
    {
        if (find_function(decl->name))
        {
            hlsl_report_message(decl->node.loc.file, decl->node.loc.line, decl->node.loc.col, HLSL_LEVEL_ERROR,
                    "redefinition of '%s'", decl->name);
            return FALSE;
        }
    }
    ret = add_declaration(hlsl_ctx.cur_scope, decl, local);
    if (!ret)
    {
        struct hlsl_ir_var *old = get_variable(hlsl_ctx.cur_scope, decl->name);

        hlsl_report_message(decl->node.loc.file, decl->node.loc.line, decl->node.loc.col, HLSL_LEVEL_ERROR,
                "\"%s\" already declared", decl->name);
        hlsl_report_message(old->node.loc.file, old->node.loc.line, old->node.loc.col, HLSL_LEVEL_NOTE,
                "\"%s\" was previously declared here", old->name);
        return FALSE;
    }
    return TRUE;
}

static DWORD add_modifier(DWORD modifiers, DWORD mod, const struct YYLTYPE *loc);

static BOOL check_type_modifiers(DWORD modifiers, struct source_location *loc)
{
    if (modifiers & ~HLSL_TYPE_MODIFIERS_MASK)
    {
        hlsl_report_message(loc->file, loc->line, loc->col, HLSL_LEVEL_ERROR,
                "modifier not allowed on typedefs");
        return FALSE;
    }
    return TRUE;
}

static BOOL add_type_to_scope(struct hlsl_scope *scope, struct hlsl_type *def)
{
    if (get_type(scope, def->name, FALSE))
        return FALSE;

    wine_rb_put(&scope->types, def->name, &def->scope_entry);
    return TRUE;
}

static void declare_predefined_types(struct hlsl_scope *scope)
{
    struct hlsl_type *type;
    unsigned int x, y, bt;
    static const char * const names[] =
    {
        "float",
        "half",
        "double",
        "int",
        "uint",
        "bool",
    };
    char name[10];

    for (bt = 0; bt <= HLSL_TYPE_LAST_SCALAR; ++bt)
    {
        for (y = 1; y <= 4; ++y)
        {
            for (x = 1; x <= 4; ++x)
            {
                sprintf(name, "%s%ux%u", names[bt], x, y);
                type = new_hlsl_type(d3dcompiler_strdup(name), HLSL_CLASS_MATRIX, bt, x, y);
                add_type_to_scope(scope, type);

                if (y == 1)
                {
                    sprintf(name, "%s%u", names[bt], x);
                    type = new_hlsl_type(d3dcompiler_strdup(name), HLSL_CLASS_VECTOR, bt, x, y);
                    add_type_to_scope(scope, type);

                    if (x == 1)
                    {
                        sprintf(name, "%s", names[bt]);
                        type = new_hlsl_type(d3dcompiler_strdup(name), HLSL_CLASS_SCALAR, bt, x, y);
                        add_type_to_scope(scope, type);
                    }
                }
            }
        }
    }

    /* DX8 effects predefined types */
    type = new_hlsl_type(d3dcompiler_strdup("DWORD"), HLSL_CLASS_SCALAR, HLSL_TYPE_INT, 1, 1);
    add_type_to_scope(scope, type);
    type = new_hlsl_type(d3dcompiler_strdup("FLOAT"), HLSL_CLASS_SCALAR, HLSL_TYPE_FLOAT, 1, 1);
    add_type_to_scope(scope, type);
    type = new_hlsl_type(d3dcompiler_strdup("VECTOR"), HLSL_CLASS_VECTOR, HLSL_TYPE_FLOAT, 4, 1);
    add_type_to_scope(scope, type);
    type = new_hlsl_type(d3dcompiler_strdup("MATRIX"), HLSL_CLASS_MATRIX, HLSL_TYPE_FLOAT, 4, 4);
    add_type_to_scope(scope, type);
    type = new_hlsl_type(d3dcompiler_strdup("STRING"), HLSL_CLASS_OBJECT, HLSL_TYPE_STRING, 1, 1);
    add_type_to_scope(scope, type);
    type = new_hlsl_type(d3dcompiler_strdup("TEXTURE"), HLSL_CLASS_OBJECT, HLSL_TYPE_TEXTURE, 1, 1);
    add_type_to_scope(scope, type);
    type = new_hlsl_type(d3dcompiler_strdup("PIXELSHADER"), HLSL_CLASS_OBJECT, HLSL_TYPE_PIXELSHADER, 1, 1);
    add_type_to_scope(scope, type);
    type = new_hlsl_type(d3dcompiler_strdup("VERTEXSHADER"), HLSL_CLASS_OBJECT, HLSL_TYPE_VERTEXSHADER, 1, 1);
    add_type_to_scope(scope, type);
}

static struct hlsl_ir_if *loop_condition(struct list *cond_list)
{
    struct hlsl_ir_if *out_cond;
    struct hlsl_ir_expr *not_cond;
    struct hlsl_ir_node *cond, *operands[3];
    struct hlsl_ir_jump *jump;
    unsigned int count = list_count(cond_list);

    if (!count)
        return NULL;
    if (count != 1)
        ERR("Got multiple expressions in a for condition.\n");

    cond = LIST_ENTRY(list_head(cond_list), struct hlsl_ir_node, entry);
    out_cond = d3dcompiler_alloc(sizeof(*out_cond));
    if (!out_cond)
    {
        ERR("Out of memory.\n");
        return NULL;
    }
    out_cond->node.type = HLSL_IR_IF;
    operands[0] = cond;
    operands[1] = operands[2] = NULL;
    not_cond = new_expr(HLSL_IR_UNOP_LOGIC_NOT, operands, &cond->loc);
    if (!not_cond)
    {
        ERR("Out of memory.\n");
        d3dcompiler_free(out_cond);
        return NULL;
    }
    out_cond->condition = &not_cond->node;
    jump = d3dcompiler_alloc(sizeof(*jump));
    if (!jump)
    {
        ERR("Out of memory.\n");
        d3dcompiler_free(out_cond);
        d3dcompiler_free(not_cond);
        return NULL;
    }
    jump->node.type = HLSL_IR_JUMP;
    jump->type = HLSL_IR_JUMP_BREAK;
    out_cond->then_instrs = d3dcompiler_alloc(sizeof(*out_cond->then_instrs));
    if (!out_cond->then_instrs)
    {
        ERR("Out of memory.\n");
        d3dcompiler_free(out_cond);
        d3dcompiler_free(not_cond);
        d3dcompiler_free(jump);
        return NULL;
    }
    list_init(out_cond->then_instrs);
    list_add_head(out_cond->then_instrs, &jump->node.entry);

    return out_cond;
}

enum loop_type
{
    LOOP_FOR,
    LOOP_WHILE,
    LOOP_DO_WHILE
};

static struct list *create_loop(enum loop_type type, struct list *init, struct list *cond,
        struct hlsl_ir_node *iter, struct list *body, struct source_location *loc)
{
    struct list *list = NULL;
    struct hlsl_ir_loop *loop = NULL;
    struct hlsl_ir_if *cond_jump = NULL;

    list = d3dcompiler_alloc(sizeof(*list));
    if (!list)
        goto oom;
    list_init(list);

    if (init)
        list_move_head(list, init);

    loop = d3dcompiler_alloc(sizeof(*loop));
    if (!loop)
        goto oom;
    loop->node.type = HLSL_IR_LOOP;
    loop->node.loc = *loc;
    list_add_tail(list, &loop->node.entry);
    loop->body = d3dcompiler_alloc(sizeof(*loop->body));
    if (!loop->body)
        goto oom;
    list_init(loop->body);

    cond_jump = loop_condition(cond);
    if (!cond_jump)
        goto oom;

    if (type != LOOP_DO_WHILE)
        list_add_tail(loop->body, &cond_jump->node.entry);

    list_move_tail(loop->body, body);

    if (iter)
        list_add_tail(loop->body, &iter->entry);

    if (type == LOOP_DO_WHILE)
        list_add_tail(loop->body, &cond_jump->node.entry);

    d3dcompiler_free(init);
    d3dcompiler_free(cond);
    d3dcompiler_free(body);
    return list;

oom:
    ERR("Out of memory.\n");
    if (loop)
        d3dcompiler_free(loop->body);
    d3dcompiler_free(loop);
    d3dcompiler_free(cond_jump);
    d3dcompiler_free(list);
    free_instr_list(init);
    free_instr_list(cond);
    free_instr(iter);
    free_instr_list(body);
    return NULL;
}

static unsigned int initializer_size(struct list *initializer)
{
    unsigned int count = 0;
    struct hlsl_ir_node *node;

    LIST_FOR_EACH_ENTRY(node, initializer, struct hlsl_ir_node, entry)
    {
        count += components_count_type(node->data_type);
    }
    TRACE("Initializer size = %u\n", count);
    return count;
}

static unsigned int components_count_expr_list(struct list *list)
{
    struct hlsl_ir_node *node;
    unsigned int count = 0;

    LIST_FOR_EACH_ENTRY(node, list, struct hlsl_ir_node, entry)
    {
        count += components_count_type(node->data_type);
    }
    return count;
}

static struct hlsl_ir_swizzle *new_swizzle(DWORD s, unsigned int components,
        struct hlsl_ir_node *val, struct source_location *loc)
{
    struct hlsl_ir_swizzle *swizzle = d3dcompiler_alloc(sizeof(*swizzle));

    if (!swizzle)
        return NULL;
    swizzle->node.type = HLSL_IR_SWIZZLE;
    swizzle->node.loc = *loc;
    swizzle->node.data_type = new_hlsl_type(NULL, HLSL_CLASS_VECTOR, val->data_type->base_type, components, 1);
    swizzle->val = val;
    swizzle->swizzle = s;
    return swizzle;
}

static struct hlsl_ir_swizzle *get_swizzle(struct hlsl_ir_node *value, const char *swizzle,
        struct source_location *loc)
{
    unsigned int len = strlen(swizzle), component = 0;
    unsigned int i, set, swiz = 0;
    BOOL valid;

    if (value->data_type->type == HLSL_CLASS_MATRIX)
    {
        /* Matrix swizzle */
        BOOL m_swizzle;
        unsigned int inc, x, y;

        if (len < 3 || swizzle[0] != '_')
            return NULL;
        m_swizzle = swizzle[1] == 'm';
        inc = m_swizzle ? 4 : 3;

        if (len % inc || len > inc * 4)
            return NULL;

        for (i = 0; i < len; i += inc)
        {
            if (swizzle[i] != '_')
                return NULL;
            if (m_swizzle)
            {
                if (swizzle[i + 1] != 'm')
                    return NULL;
                x = swizzle[i + 2] - '0';
                y = swizzle[i + 3] - '0';
            }
            else
            {
                x = swizzle[i + 1] - '1';
                y = swizzle[i + 2] - '1';
            }

            if (x >= value->data_type->dimx || y >= value->data_type->dimy)
                return NULL;
            swiz |= (y << 4 | x) << component * 8;
            component++;
        }
        return new_swizzle(swiz, component, value, loc);
    }

    /* Vector swizzle */
    if (len > 4)
        return NULL;

    for (set = 0; set < 2; ++set)
    {
        valid = TRUE;
        component = 0;
        for (i = 0; i < len; ++i)
        {
            char c[2][4] = {{'x', 'y', 'z', 'w'}, {'r', 'g', 'b', 'a'}};
            unsigned int s = 0;

            for (s = 0; s < 4; ++s)
            {
                if (swizzle[i] == c[set][s])
                    break;
            }
            if (s == 4)
            {
                valid = FALSE;
                break;
            }

            if (s >= value->data_type->dimx)
                return NULL;
            swiz |= s << component * 2;
            component++;
        }
        if (valid)
            return new_swizzle(swiz, component, value, loc);
    }

    return NULL;
}

static void struct_var_initializer(struct list *list, struct hlsl_ir_var *var, struct list *initializer)
{
    struct hlsl_type *type = var->node.data_type;
    struct hlsl_ir_node *node;
    struct hlsl_struct_field *field;
    struct list *cur_node;
    struct hlsl_ir_node *assignment;
    struct hlsl_ir_deref *deref;

    if (initializer_size(initializer) != components_count_type(type))
    {
        hlsl_report_message(var->node.loc.file, var->node.loc.line, var->node.loc.col, HLSL_LEVEL_ERROR,
                "structure initializer mismatch");
        free_instr_list(initializer);
        return;
    }
    cur_node = list_head(initializer);
    assert(cur_node);
    node = LIST_ENTRY(cur_node, struct hlsl_ir_node, entry);
    LIST_FOR_EACH_ENTRY(field, type->e.elements, struct hlsl_struct_field, entry)
    {
        if (!cur_node)
        {
            d3dcompiler_free(initializer);
            return;
        }
        if (components_count_type(field->type) == components_count_type(node->data_type))
        {
            deref = new_record_deref(&var->node, field);
            if (!deref)
            {
                ERR("Out of memory.\n");
                break;
            }
            deref->node.loc = node->loc;
            assignment = make_assignment(&deref->node, ASSIGN_OP_ASSIGN, BWRITERSP_WRITEMASK_ALL, node);
            list_add_tail(list, &assignment->entry);
        }
        else
            FIXME("Initializing with \"mismatched\" fields is not supported yet.\n");
        cur_node = list_next(initializer, cur_node);
        node = LIST_ENTRY(cur_node, struct hlsl_ir_node, entry);
    }

    /* Free initializer elements in excess. */
    while (cur_node)
    {
        struct list *next = list_next(initializer, cur_node);
        free_instr(node);
        cur_node = next;
        node = LIST_ENTRY(cur_node, struct hlsl_ir_node, entry);
    }
    d3dcompiler_free(initializer);
}

static struct list *declare_vars(struct hlsl_type *basic_type, DWORD modifiers, struct list *var_list)
{
    struct hlsl_type *type;
    struct parse_variable_def *v, *v_next;
    struct hlsl_ir_var *var;
    struct hlsl_ir_node *assignment;
    BOOL ret, local = TRUE;
    struct list *statements_list = d3dcompiler_alloc(sizeof(*statements_list));

    if (!statements_list)
    {
        ERR("Out of memory.\n");
        LIST_FOR_EACH_ENTRY_SAFE(v, v_next, var_list, struct parse_variable_def, entry)
            d3dcompiler_free(v);
        d3dcompiler_free(var_list);
        return NULL;
    }
    list_init(statements_list);

    if (!var_list)
        return statements_list;

    LIST_FOR_EACH_ENTRY_SAFE(v, v_next, var_list, struct parse_variable_def, entry)
    {
        var = d3dcompiler_alloc(sizeof(*var));
        if (!var)
        {
            ERR("Out of memory.\n");
            d3dcompiler_free(v);
            continue;
        }
        var->node.type = HLSL_IR_VAR;
        if (v->array_size)
            type = new_array_type(basic_type, v->array_size);
        else
            type = basic_type;
        var->node.data_type = type;
        var->node.loc = v->loc;
        var->name = v->name;
        var->modifiers = modifiers;
        var->semantic = v->semantic;
        var->reg_reservation = v->reg_reservation;
        debug_dump_decl(type, modifiers, v->name, v->loc.line);

        if (hlsl_ctx.cur_scope == hlsl_ctx.globals)
        {
            var->modifiers |= HLSL_STORAGE_UNIFORM;
            local = FALSE;
        }

        if (var->modifiers & HLSL_MODIFIER_CONST && !(var->modifiers & HLSL_STORAGE_UNIFORM) && !v->initializer)
        {
            hlsl_report_message(v->loc.file, v->loc.line, v->loc.col,
                    HLSL_LEVEL_ERROR, "const variable without initializer");
            free_declaration(var);
            d3dcompiler_free(v);
            continue;
        }

        ret = declare_variable(var, local);
        if (!ret)
        {
            free_declaration(var);
            d3dcompiler_free(v);
            continue;
        }
        TRACE("Declared variable %s.\n", var->name);

        if (v->initializer)
        {
            unsigned int size = initializer_size(v->initializer);
            struct hlsl_ir_node *node;

            TRACE("Variable with initializer.\n");
            if (type->type <= HLSL_CLASS_LAST_NUMERIC
                    && type->dimx * type->dimy != size && size != 1)
            {
                if (size < type->dimx * type->dimy)
                {
                    hlsl_report_message(v->loc.file, v->loc.line, v->loc.col, HLSL_LEVEL_ERROR,
                            "'%s' initializer does not match", v->name);
                    free_instr_list(v->initializer);
                    d3dcompiler_free(v);
                    continue;
                }
            }
            if ((type->type == HLSL_CLASS_STRUCT || type->type == HLSL_CLASS_ARRAY)
                    && components_count_type(type) != size)
            {
                hlsl_report_message(v->loc.file, v->loc.line, v->loc.col, HLSL_LEVEL_ERROR,
                        "'%s' initializer does not match", v->name);
                free_instr_list(v->initializer);
                d3dcompiler_free(v);
                continue;
            }

            if (type->type == HLSL_CLASS_STRUCT)
            {
                struct_var_initializer(statements_list, var, v->initializer);
                d3dcompiler_free(v);
                continue;
            }
            if (type->type > HLSL_CLASS_LAST_NUMERIC)
            {
                FIXME("Initializers for non scalar/struct variables not supported yet.\n");
                free_instr_list(v->initializer);
                d3dcompiler_free(v);
                continue;
            }
            if (v->array_size > 0)
            {
                FIXME("Initializing arrays is not supported yet.\n");
                free_instr_list(v->initializer);
                d3dcompiler_free(v);
                continue;
            }
            if (list_count(v->initializer) > 1)
            {
                FIXME("Complex initializers are not supported yet.\n");
                free_instr_list(v->initializer);
                d3dcompiler_free(v);
                continue;
            }
            node = LIST_ENTRY(list_head(v->initializer), struct hlsl_ir_node, entry);
            assignment = make_assignment(&var->node, ASSIGN_OP_ASSIGN,
                    BWRITERSP_WRITEMASK_ALL, node);
            list_add_tail(statements_list, &assignment->entry);
            d3dcompiler_free(v->initializer);
        }
        d3dcompiler_free(v);
    }
    d3dcompiler_free(var_list);
    return statements_list;
}

static BOOL add_struct_field(struct list *fields, struct hlsl_struct_field *field)
{
    struct hlsl_struct_field *f;

    LIST_FOR_EACH_ENTRY(f, fields, struct hlsl_struct_field, entry)
    {
        if (!strcmp(f->name, field->name))
            return FALSE;
    }
    list_add_tail(fields, &field->entry);
    return TRUE;
}

static struct list *gen_struct_fields(struct hlsl_type *type, DWORD modifiers, struct list *fields)
{
    struct parse_variable_def *v, *v_next;
    struct hlsl_struct_field *field;
    struct list *list;

    list = d3dcompiler_alloc(sizeof(*list));
    if (!list)
    {
        ERR("Out of memory.\n");
        return NULL;
    }
    list_init(list);
    LIST_FOR_EACH_ENTRY_SAFE(v, v_next, fields, struct parse_variable_def, entry)
    {
        debug_dump_decl(type, 0, v->name, v->loc.line);
        field = d3dcompiler_alloc(sizeof(*field));
        if (!field)
        {
            ERR("Out of memory.\n");
            d3dcompiler_free(v);
            return list;
        }
        field->type = type;
        field->name = v->name;
        field->modifiers = modifiers;
        field->semantic = v->semantic;
        if (v->initializer)
        {
            hlsl_report_message(v->loc.file, v->loc.line, v->loc.col, HLSL_LEVEL_ERROR,
                    "struct field with an initializer.\n");
            free_instr_list(v->initializer);
        }
        list_add_tail(list, &field->entry);
        d3dcompiler_free(v);
    }
    d3dcompiler_free(fields);
    return list;
}

static struct hlsl_type *new_struct_type(const char *name, DWORD modifiers, struct list *fields)
{
    struct hlsl_type *type = d3dcompiler_alloc(sizeof(*type));

    if (!type)
    {
        ERR("Out of memory.\n");
        return NULL;
    }
    type->type = HLSL_CLASS_STRUCT;
    type->name = name;
    type->dimx = type->dimy = 1;
    type->modifiers = modifiers;
    type->e.elements = fields;

    list_add_tail(&hlsl_ctx.types, &type->entry);

    return type;
}

static BOOL add_typedef(DWORD modifiers, struct hlsl_type *orig_type, struct list *list,
        struct source_location *loc)
{
    BOOL ret;
    struct hlsl_type *type;
    struct parse_variable_def *v, *v_next;

    if (!check_type_modifiers(modifiers, loc))
    {
        LIST_FOR_EACH_ENTRY_SAFE(v, v_next, list, struct parse_variable_def, entry)
            d3dcompiler_free(v);
        d3dcompiler_free(list);
        return FALSE;
    }

    LIST_FOR_EACH_ENTRY_SAFE(v, v_next, list, struct parse_variable_def, entry)
    {
        if (v->array_size)
            type = new_array_type(orig_type, v->array_size);
        else
            type = clone_hlsl_type(orig_type);
        if (!type)
        {
            ERR("Out of memory\n");
            return FALSE;
        }
        d3dcompiler_free((void *)type->name);
        type->name = v->name;
        type->modifiers |= modifiers;

        if (type->type != HLSL_CLASS_MATRIX)
            check_invalid_matrix_modifiers(type->modifiers, &v->loc);

        ret = add_type_to_scope(hlsl_ctx.cur_scope, type);
        if (!ret)
        {
            hlsl_report_message(v->loc.file, v->loc.line, v->loc.col, HLSL_LEVEL_ERROR,
                    "redefinition of custom type '%s'", v->name);
        }
        d3dcompiler_free(v);
    }
    d3dcompiler_free(list);
    return TRUE;
}

static BOOL add_func_parameter(struct list *list, struct parse_parameter *param, const struct source_location *loc)
{
    struct hlsl_ir_var *decl = d3dcompiler_alloc(sizeof(*decl));

    if (!decl)
    {
        ERR("Out of memory.\n");
        return FALSE;
    }
    decl->node.type = HLSL_IR_VAR;
    decl->node.data_type = param->type;
    decl->node.loc = *loc;
    decl->name = param->name;
    decl->semantic = param->semantic;
    decl->reg_reservation = param->reg_reservation;
    decl->modifiers = param->modifiers;

    if (!add_declaration(hlsl_ctx.cur_scope, decl, FALSE))
    {
        free_declaration(decl);
        return FALSE;
    }
    list_add_tail(list, &decl->node.entry);
    return TRUE;
}

static struct reg_reservation *parse_reg_reservation(const char *reg_string)
{
    struct reg_reservation *reg_res;
    enum bwritershader_param_register_type type;
    DWORD regnum = 0;

    switch (reg_string[0])
    {
        case 'c':
            type = BWRITERSPR_CONST;
            break;
        case 'i':
            type = BWRITERSPR_CONSTINT;
            break;
        case 'b':
            type = BWRITERSPR_CONSTBOOL;
            break;
        case 's':
            type = BWRITERSPR_SAMPLER;
            break;
        default:
            FIXME("Unsupported register type.\n");
            return NULL;
     }

    if (!sscanf(reg_string + 1, "%u", &regnum))
    {
        FIXME("Unsupported register reservation syntax.\n");
        return NULL;
    }

    reg_res = d3dcompiler_alloc(sizeof(*reg_res));
    if (!reg_res)
    {
        ERR("Out of memory.\n");
        return NULL;
    }
    reg_res->type = type;
    reg_res->regnum = regnum;
    return reg_res;
}

static const struct hlsl_ir_function_decl *get_overloaded_func(struct wine_rb_tree *funcs, char *name,
        struct list *params, BOOL exact_signature)
{
    struct hlsl_ir_function *func;
    struct wine_rb_entry *entry;

    entry = wine_rb_get(funcs, name);
    if (entry)
    {
        func = WINE_RB_ENTRY_VALUE(entry, struct hlsl_ir_function, entry);

        entry = wine_rb_get(&func->overloads, params);
        if (!entry)
        {
            if (!exact_signature)
                FIXME("No exact match, search for a compatible overloaded function (if any).\n");
            return NULL;
        }
        return WINE_RB_ENTRY_VALUE(entry, struct hlsl_ir_function_decl, entry);
    }
    return NULL;
}

%}

%locations
%error-verbose
%expect 1

%union
{
    struct hlsl_type *type;
    INT intval;
    FLOAT floatval;
    BOOL boolval;
    char *name;
    DWORD modifiers;
    struct hlsl_ir_var *var;
    struct hlsl_ir_node *instr;
    struct list *list;
    struct parse_function function;
    struct parse_parameter parameter;
    struct parse_variable_def *variable_def;
    struct parse_if_body if_body;
    enum parse_unary_op unary_op;
    enum parse_assign_op assign_op;
    struct reg_reservation *reg_reservation;
    struct parse_colon_attribute colon_attribute;
}

%token KW_BLENDSTATE
%token KW_BREAK
%token KW_BUFFER
%token KW_CBUFFER
%token KW_COLUMN_MAJOR
%token KW_COMPILE
%token KW_CONST
%token KW_CONTINUE
%token KW_DEPTHSTENCILSTATE
%token KW_DEPTHSTENCILVIEW
%token KW_DISCARD
%token KW_DO
%token KW_DOUBLE
%token KW_ELSE
%token KW_EXTERN
%token KW_FALSE
%token KW_FOR
%token KW_GEOMETRYSHADER
%token KW_GROUPSHARED
%token KW_IF
%token KW_IN
%token KW_INLINE
%token KW_INOUT
%token KW_MATRIX
%token KW_NAMESPACE
%token KW_NOINTERPOLATION
%token KW_OUT
%token KW_PASS
%token KW_PIXELSHADER
%token KW_PRECISE
%token KW_RASTERIZERSTATE
%token KW_RENDERTARGETVIEW
%token KW_RETURN
%token KW_REGISTER
%token KW_ROW_MAJOR
%token KW_SAMPLER
%token KW_SAMPLER1D
%token KW_SAMPLER2D
%token KW_SAMPLER3D
%token KW_SAMPLERCUBE
%token KW_SAMPLER_STATE
%token KW_SAMPLERCOMPARISONSTATE
%token KW_SHARED
%token KW_STATEBLOCK
%token KW_STATEBLOCK_STATE
%token KW_STATIC
%token KW_STRING
%token KW_STRUCT
%token KW_SWITCH
%token KW_TBUFFER
%token KW_TECHNIQUE
%token KW_TECHNIQUE10
%token KW_TEXTURE
%token KW_TEXTURE1D
%token KW_TEXTURE1DARRAY
%token KW_TEXTURE2D
%token KW_TEXTURE2DARRAY
%token KW_TEXTURE2DMS
%token KW_TEXTURE2DMSARRAY
%token KW_TEXTURE3D
%token KW_TEXTURE3DARRAY
%token KW_TEXTURECUBE
%token KW_TRUE
%token KW_TYPEDEF
%token KW_UNIFORM
%token KW_VECTOR
%token KW_VERTEXSHADER
%token KW_VOID
%token KW_VOLATILE
%token KW_WHILE

%token OP_INC
%token OP_DEC
%token OP_AND
%token OP_OR
%token OP_EQ
%token OP_LEFTSHIFT
%token OP_LEFTSHIFTASSIGN
%token OP_RIGHTSHIFT
%token OP_RIGHTSHIFTASSIGN
%token OP_ELLIPSIS
%token OP_LE
%token OP_GE
%token OP_NE
%token OP_ADDASSIGN
%token OP_SUBASSIGN
%token OP_MULASSIGN
%token OP_DIVASSIGN
%token OP_MODASSIGN
%token OP_ANDASSIGN
%token OP_ORASSIGN
%token OP_XORASSIGN
%token OP_UNKNOWN1
%token OP_UNKNOWN2
%token OP_UNKNOWN3
%token OP_UNKNOWN4

%token <intval> PRE_LINE

%token <name> VAR_IDENTIFIER TYPE_IDENTIFIER NEW_IDENTIFIER
%type <name> any_identifier var_identifier
%token <name> STRING
%token <floatval> C_FLOAT
%token <intval> C_INTEGER
%type <boolval> boolean
%type <type> base_type
%type <type> type
%type <list> declaration_statement
%type <list> declaration
%type <list> struct_declaration
%type <type> struct_spec
%type <type> named_struct_spec
%type <type> unnamed_struct_spec
%type <list> type_specs
%type <variable_def> type_spec
%type <list> complex_initializer
%type <list> initializer_expr_list
%type <instr> initializer_expr
%type <modifiers> var_modifiers
%type <list> field
%type <list> parameters
%type <list> param_list
%type <instr> expr
%type <var> variable
%type <intval> array
%type <list> statement
%type <list> statement_list
%type <list> compound_statement
%type <list> jump_statement
%type <list> selection_statement
%type <list> loop_statement
%type <function> func_declaration
%type <function> func_prototype
%type <list> fields_list
%type <parameter> parameter
%type <colon_attribute> colon_attribute
%type <name> semantic
%type <reg_reservation> register_opt
%type <variable_def> variable_def
%type <list> variables_def
%type <list> variables_def_optional
%type <if_body> if_body
%type <instr> primary_expr
%type <instr> postfix_expr
%type <instr> unary_expr
%type <instr> mul_expr
%type <instr> add_expr
%type <instr> shift_expr
%type <instr> relational_expr
%type <instr> equality_expr
%type <instr> bitand_expr
%type <instr> bitxor_expr
%type <instr> bitor_expr
%type <instr> logicand_expr
%type <instr> logicor_expr
%type <instr> conditional_expr
%type <instr> assignment_expr
%type <list> expr_statement
%type <unary_op> unary_op
%type <assign_op> assign_op
%type <modifiers> input_mods
%type <modifiers> input_mod
%%

hlsl_prog:                /* empty */
                            {
                            }
                        | hlsl_prog func_declaration
                            {
                                const struct hlsl_ir_function_decl *decl;

                                decl = get_overloaded_func(&hlsl_ctx.functions, $2.name, $2.decl->parameters, TRUE);
                                if (decl && !decl->func->intrinsic)
                                {
                                    if (decl->body && $2.decl->body)
                                    {
                                        hlsl_report_message($2.decl->node.loc.file, $2.decl->node.loc.line,
                                                $2.decl->node.loc.col, HLSL_LEVEL_ERROR,
                                                "redefinition of function %s", debugstr_a($2.name));
                                        return 1;
                                    }
                                    else if (!compare_hlsl_types(decl->node.data_type, $2.decl->node.data_type))
                                    {
                                        hlsl_report_message($2.decl->node.loc.file, $2.decl->node.loc.line,
                                                $2.decl->node.loc.col, HLSL_LEVEL_ERROR,
                                                "redefining function %s with a different return type",
                                                debugstr_a($2.name));
                                        hlsl_report_message(decl->node.loc.file, decl->node.loc.line, decl->node.loc.col, HLSL_LEVEL_NOTE,
                                                "%s previously declared here",
                                                debugstr_a($2.name));
                                        return 1;
                                    }
                                }

                                if ($2.decl->node.data_type->base_type == HLSL_TYPE_VOID && $2.decl->semantic)
                                {
                                    hlsl_report_message($2.decl->node.loc.file, $2.decl->node.loc.line,
                                            $2.decl->node.loc.col, HLSL_LEVEL_ERROR,
                                            "void function with a semantic");
                                }

                                TRACE("Adding function '%s' to the function list.\n", $2.name);
                                add_function_decl(&hlsl_ctx.functions, $2.name, $2.decl, FALSE);
                            }
                        | hlsl_prog declaration_statement
                            {
                                TRACE("Declaration statement parsed.\n");
                            }
                        | hlsl_prog preproc_directive
                            {
                            }
                        | hlsl_prog ';'
                            {
                                TRACE("Skipping stray semicolon.\n");
                            }

preproc_directive:        PRE_LINE STRING
                            {
                                const char **new_array = NULL;

                                TRACE("Updating line information to file %s, line %u\n", debugstr_a($2), $1);
                                hlsl_ctx.line_no = $1;
                                if (strcmp($2, hlsl_ctx.source_file))
                                    new_array = d3dcompiler_realloc(hlsl_ctx.source_files,
                                            sizeof(*hlsl_ctx.source_files) * hlsl_ctx.source_files_count + 1);

                                if (new_array)
                                {
                                    hlsl_ctx.source_files = new_array;
                                    hlsl_ctx.source_files[hlsl_ctx.source_files_count++] = $2;
                                    hlsl_ctx.source_file = $2;
                                }
                                else
                                {
                                    d3dcompiler_free($2);
                                }
                            }

struct_declaration:       struct_spec variables_def_optional ';'
                            {
                                struct source_location loc;

                                set_location(&loc, &@3);
                                if (!$2)
                                {
                                    if (!$1->name)
                                    {
                                        hlsl_report_message(loc.file, loc.line, loc.col,
                                                HLSL_LEVEL_ERROR, "anonymous struct declaration with no variables");
                                    }
                                    check_type_modifiers($1->modifiers, &loc);
                                }
                                $$ = declare_vars($1, 0, $2);
                            }

struct_spec:              named_struct_spec
                        | unnamed_struct_spec

named_struct_spec:        var_modifiers KW_STRUCT any_identifier '{' fields_list '}'
                            {
                                BOOL ret;
                                struct source_location loc;

                                TRACE("Structure %s declaration.\n", debugstr_a($3));
                                set_location(&loc, &@1);
                                check_invalid_matrix_modifiers($1, &loc);
                                $$ = new_struct_type($3, $1, $5);

                                if (get_variable(hlsl_ctx.cur_scope, $3))
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, @3.first_line, @3.first_column,
                                            HLSL_LEVEL_ERROR, "redefinition of '%s'", $3);
                                    return 1;
                                }

                                ret = add_type_to_scope(hlsl_ctx.cur_scope, $$);
                                if (!ret)
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, @3.first_line, @3.first_column,
                                            HLSL_LEVEL_ERROR, "redefinition of struct '%s'", $3);
                                    return 1;
                                }
                            }

unnamed_struct_spec:      var_modifiers KW_STRUCT '{' fields_list '}'
                            {
                                struct source_location loc;

                                TRACE("Anonymous structure declaration.\n");
                                set_location(&loc, &@1);
                                check_invalid_matrix_modifiers($1, &loc);
                                $$ = new_struct_type(NULL, $1, $4);
                            }

any_identifier:           VAR_IDENTIFIER
                        | TYPE_IDENTIFIER
                        | NEW_IDENTIFIER

fields_list:              /* Empty */
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                            }
                        | fields_list field
                            {
                                BOOL ret;
                                struct hlsl_struct_field *field, *next;

                                $$ = $1;
                                LIST_FOR_EACH_ENTRY_SAFE(field, next, $2, struct hlsl_struct_field, entry)
                                {
                                    ret = add_struct_field($$, field);
                                    if (ret == FALSE)
                                    {
                                        hlsl_report_message(hlsl_ctx.source_file, @2.first_line, @2.first_column,
                                                HLSL_LEVEL_ERROR, "redefinition of '%s'", field->name);
                                        d3dcompiler_free(field);
                                    }
                                }
                                d3dcompiler_free($2);
                            }

field:                    var_modifiers type variables_def ';'
                            {
                                $$ = gen_struct_fields($2, $1, $3);
                            }
                        | unnamed_struct_spec variables_def ';'
                            {
                                $$ = gen_struct_fields($1, 0, $2);
                            }

func_declaration:         func_prototype compound_statement
                            {
                                TRACE("Function %s parsed.\n", $1.name);
                                $$ = $1;
                                $$.decl->body = $2;
                                pop_scope(&hlsl_ctx);
                            }
                        | func_prototype ';'
                            {
                                TRACE("Function prototype for %s.\n", $1.name);
                                $$ = $1;
                                pop_scope(&hlsl_ctx);
                            }

func_prototype:           var_modifiers type var_identifier '(' parameters ')' colon_attribute
                            {
                                if (get_variable(hlsl_ctx.globals, $3))
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, @3.first_line, @3.first_column,
                                            HLSL_LEVEL_ERROR, "redefinition of '%s'\n", $3);
                                    return 1;
                                }
                                if ($2->base_type == HLSL_TYPE_VOID && $7.semantic)
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, @7.first_line, @7.first_column,
                                            HLSL_LEVEL_ERROR, "void function with a semantic");
                                }

                                if ($7.reg_reservation)
                                {
                                    FIXME("Unexpected register reservation for a function.\n");
                                    d3dcompiler_free($7.reg_reservation);
                                }
                                $$.decl = new_func_decl($2, $5);
                                if (!$$.decl)
                                {
                                    ERR("Out of memory.\n");
                                    return -1;
                                }
                                $$.name = $3;
                                $$.decl->semantic = $7.semantic;
                                set_location(&$$.decl->node.loc, &@3);
                            }

compound_statement:       '{' '}'
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                            }
                        | '{' scope_start statement_list '}'
                            {
                                pop_scope(&hlsl_ctx);
                                $$ = $3;
                            }

scope_start:              /* Empty */
                            {
                                push_scope(&hlsl_ctx);
                            }

var_identifier:           VAR_IDENTIFIER
                        | NEW_IDENTIFIER

colon_attribute:          /* Empty */
                            {
                                $$.semantic = NULL;
                                $$.reg_reservation = NULL;
                            }
                        | semantic
                            {
                                $$.semantic = $1;
                                $$.reg_reservation = NULL;
                            }
                        | register_opt
                            {
                                $$.semantic = NULL;
                                $$.reg_reservation = $1;
                            }

semantic:                 ':' any_identifier
                            {
                                $$ = $2;
                            }

                          /* FIXME: Writemasks */
register_opt:             ':' KW_REGISTER '(' any_identifier ')'
                            {
                                $$ = parse_reg_reservation($4);
                                d3dcompiler_free($4);
                            }
                        | ':' KW_REGISTER '(' any_identifier ',' any_identifier ')'
                            {
                                FIXME("Ignoring shader target %s in a register reservation.\n", debugstr_a($4));
                                d3dcompiler_free($4);

                                $$ = parse_reg_reservation($6);
                                d3dcompiler_free($6);
                            }

parameters:               scope_start
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                            }
                        | scope_start param_list
                            {
                                $$ = $2;
                            }

param_list:               parameter
                            {
                                struct source_location loc;

                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                                set_location(&loc, &@1);
                                if (!add_func_parameter($$, &$1, &loc))
                                {
                                    ERR("Error adding function parameter %s.\n", $1.name);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return -1;
                                }
                            }
                        | param_list ',' parameter
                            {
                                struct source_location loc;

                                $$ = $1;
                                set_location(&loc, &@3);
                                if (!add_func_parameter($$, &$3, &loc))
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "duplicate parameter %s", $3.name);
                                    return 1;
                                }
                            }

parameter:                input_mods var_modifiers type any_identifier colon_attribute
                            {
                                $$.modifiers = $1 ? $1 : HLSL_MODIFIER_IN;
                                $$.modifiers |= $2;
                                $$.type = $3;
                                $$.name = $4;
                                $$.semantic = $5.semantic;
                                $$.reg_reservation = $5.reg_reservation;
                            }

input_mods:               /* Empty */
                            {
                                $$ = 0;
                            }
                        | input_mods input_mod
                            {
                                if ($1 & $2)
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, @2.first_line, @2.first_column,
                                            HLSL_LEVEL_ERROR, "duplicate input-output modifiers");
                                    return 1;
                                }
                                $$ = $1 | $2;
                            }

input_mod:                KW_IN
                            {
                                $$ = HLSL_MODIFIER_IN;
                            }
                        | KW_OUT
                            {
                                $$ = HLSL_MODIFIER_OUT;
                            }
                        | KW_INOUT
                            {
                                $$ = HLSL_MODIFIER_IN | HLSL_MODIFIER_OUT;
                            }

type:                     base_type
                            {
                                $$ = $1;
                            }
                        | KW_VECTOR '<' base_type ',' C_INTEGER '>'
                            {
                                if ($3->type != HLSL_CLASS_SCALAR)
                                {
                                    hlsl_message("Line %u: vectors of non-scalar types are not allowed.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }
                                if ($5 < 1 || $5 > 4)
                                {
                                    hlsl_message("Line %u: vector size must be between 1 and 4.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }

                                $$ = new_hlsl_type(NULL, HLSL_CLASS_VECTOR, $3->base_type, $5, 1);
                            }
                        | KW_MATRIX '<' base_type ',' C_INTEGER ',' C_INTEGER '>'
                            {
                                if ($3->type != HLSL_CLASS_SCALAR)
                                {
                                    hlsl_message("Line %u: matrices of non-scalar types are not allowed.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }
                                if ($5 < 1 || $5 > 4 || $7 < 1 || $7 > 4)
                                {
                                    hlsl_message("Line %u: matrix dimensions must be between 1 and 4.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }

                                $$ = new_hlsl_type(NULL, HLSL_CLASS_MATRIX, $3->base_type, $5, $7);
                            }

base_type:                KW_VOID
                            {
                                $$ = new_hlsl_type(d3dcompiler_strdup("void"), HLSL_CLASS_OBJECT, HLSL_TYPE_VOID, 1, 1);
                            }
                        | KW_SAMPLER
                            {
                                $$ = new_hlsl_type(d3dcompiler_strdup("sampler"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                $$->sampler_dim = HLSL_SAMPLER_DIM_GENERIC;
                            }
                        | KW_SAMPLER1D
                            {
                                $$ = new_hlsl_type(d3dcompiler_strdup("sampler1D"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                $$->sampler_dim = HLSL_SAMPLER_DIM_1D;
                            }
                        | KW_SAMPLER2D
                            {
                                $$ = new_hlsl_type(d3dcompiler_strdup("sampler2D"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                $$->sampler_dim = HLSL_SAMPLER_DIM_2D;
                            }
                        | KW_SAMPLER3D
                            {
                                $$ = new_hlsl_type(d3dcompiler_strdup("sampler3D"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                $$->sampler_dim = HLSL_SAMPLER_DIM_3D;
                            }
                        | KW_SAMPLERCUBE
                            {
                                $$ = new_hlsl_type(d3dcompiler_strdup("samplerCUBE"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                $$->sampler_dim = HLSL_SAMPLER_DIM_CUBE;
                            }
                        | TYPE_IDENTIFIER
                            {
                                struct hlsl_type *type;

                                type = get_type(hlsl_ctx.cur_scope, $1, TRUE);
                                $$ = type;
                                d3dcompiler_free($1);
                            }
                        | KW_STRUCT TYPE_IDENTIFIER
                            {
                                struct hlsl_type *type;

                                type = get_type(hlsl_ctx.cur_scope, $2, TRUE);
                                if (type->type != HLSL_CLASS_STRUCT)
                                {
                                    hlsl_message("Line %u: redefining %s as a structure.\n",
                                            hlsl_ctx.line_no, $2);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                }
                                else
                                {
                                    $$ = type;
                                }
                                d3dcompiler_free($2);
                            }

declaration_statement:    declaration
                        | struct_declaration
                        | typedef
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                if (!$$)
                                {
                                    ERR("Out of memory\n");
                                    return -1;
                                }
                                list_init($$);
                            }

typedef:                  KW_TYPEDEF var_modifiers type type_specs ';'
                            {
                                struct source_location loc;

                                set_location(&loc, &@1);
                                if (!add_typedef($2, $3, $4, &loc))
                                    return 1;
                            }
                        | KW_TYPEDEF struct_spec type_specs ';'
                            {
                                struct source_location loc;

                                set_location(&loc, &@1);
                                if (!add_typedef(0, $2, $3, &loc))
                                    return 1;
                            }

type_specs:               type_spec
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                                list_add_head($$, &$1->entry);
                            }
                        | type_specs ',' type_spec
                            {
                                $$ = $1;
                                list_add_tail($$, &$3->entry);
                            }

type_spec:                any_identifier array
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                set_location(&$$->loc, &@1);
                                $$->name = $1;
                                $$->array_size = $2;
                            }

declaration:              var_modifiers type variables_def ';'
                            {
                                $$ = declare_vars($2, $1, $3);
                            }

variables_def_optional:   /* Empty */
                            {
                                $$ = NULL;
                            }
                        | variables_def
                            {
                                $$ = $1;
                            }

variables_def:            variable_def
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                                list_add_head($$, &$1->entry);
                            }
                        | variables_def ',' variable_def
                            {
                                $$ = $1;
                                list_add_tail($$, &$3->entry);
                            }

variable_def:             any_identifier array colon_attribute
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                set_location(&$$->loc, &@1);
                                $$->name = $1;
                                $$->array_size = $2;
                                $$->semantic = $3.semantic;
                                $$->reg_reservation = $3.reg_reservation;
                            }
                        | any_identifier array colon_attribute '=' complex_initializer
                            {
                                TRACE("Declaration with initializer.\n");
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                set_location(&$$->loc, &@1);
                                $$->name = $1;
                                $$->array_size = $2;
                                $$->semantic = $3.semantic;
                                $$->reg_reservation = $3.reg_reservation;
                                $$->initializer = $5;
                            }

array:                    /* Empty */
                            {
                                $$ = 0;
                            }
                        | '[' expr ']'
                            {
                                FIXME("Array.\n");
                                $$ = 0;
                                free_instr($2);
                            }

var_modifiers:            /* Empty */
                            {
                                $$ = 0;
                            }
                        | KW_EXTERN var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_STORAGE_EXTERN, &@1);
                            }
                        | KW_NOINTERPOLATION var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_STORAGE_NOINTERPOLATION, &@1);
                            }
                        | KW_PRECISE var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_MODIFIER_PRECISE, &@1);
                            }
                        | KW_SHARED var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_STORAGE_SHARED, &@1);
                            }
                        | KW_GROUPSHARED var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_STORAGE_GROUPSHARED, &@1);
                            }
                        | KW_STATIC var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_STORAGE_STATIC, &@1);
                            }
                        | KW_UNIFORM var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_STORAGE_UNIFORM, &@1);
                            }
                        | KW_VOLATILE var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_STORAGE_VOLATILE, &@1);
                            }
                        | KW_CONST var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_MODIFIER_CONST, &@1);
                            }
                        | KW_ROW_MAJOR var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_MODIFIER_ROW_MAJOR, &@1);
                            }
                        | KW_COLUMN_MAJOR var_modifiers
                            {
                                $$ = add_modifier($2, HLSL_MODIFIER_COLUMN_MAJOR, &@1);
                            }

complex_initializer:      initializer_expr
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                                list_add_head($$, &$1->entry);
                            }
                        | '{' initializer_expr_list '}'
                            {
                                $$ = $2;
                            }
                        | '{' initializer_expr_list ',' '}'
                            {
                                $$ = $2;
                            }

initializer_expr:         assignment_expr
                            {
                                $$ = $1;
                            }

initializer_expr_list:    initializer_expr
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                                list_add_head($$, &$1->entry);
                            }
                        | initializer_expr_list ',' initializer_expr
                            {
                                $$ = $1;
                                list_add_tail($$, &$3->entry);
                            }

boolean:                  KW_TRUE
                            {
                                $$ = TRUE;
                            }
                        | KW_FALSE
                            {
                                $$ = FALSE;
                            }

statement_list:           statement
                            {
                                $$ = $1;
                            }
                        | statement_list statement
                            {
                                $$ = $1;
                                list_move_tail($$, $2);
                                d3dcompiler_free($2);
                            }

statement:                declaration_statement
                        | expr_statement
                        | compound_statement
                        | jump_statement
                        | selection_statement
                        | loop_statement

                          /* FIXME: add rule for return with no value */
jump_statement:           KW_RETURN expr ';'
                            {
                                struct hlsl_ir_jump *jump = d3dcompiler_alloc(sizeof(*jump));
                                if (!jump)
                                {
                                    ERR("Out of memory\n");
                                    return -1;
                                }
                                jump->node.type = HLSL_IR_JUMP;
                                set_location(&jump->node.loc, &@1);
                                jump->type = HLSL_IR_JUMP_RETURN;
                                jump->node.data_type = $2->data_type;
                                jump->return_value = $2;

                                FIXME("Check for valued return on void function.\n");
                                FIXME("Implicit conversion to the return type if needed, "
				        "error out if conversion not possible.\n");

                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                                list_add_tail($$, &jump->node.entry);
                            }

selection_statement:      KW_IF '(' expr ')' if_body
                            {
                                struct hlsl_ir_if *instr = d3dcompiler_alloc(sizeof(*instr));
                                if (!instr)
                                {
                                    ERR("Out of memory\n");
                                    return -1;
                                }
                                instr->node.type = HLSL_IR_IF;
                                set_location(&instr->node.loc, &@1);
                                instr->condition = $3;
                                instr->then_instrs = $5.then_instrs;
                                instr->else_instrs = $5.else_instrs;
                                if ($3->data_type->dimx > 1 || $3->data_type->dimy > 1)
                                {
                                    hlsl_report_message(instr->node.loc.file, instr->node.loc.line,
                                            instr->node.loc.col, HLSL_LEVEL_ERROR,
                                            "if condition requires a scalar");
                                }
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                                list_add_head($$, &instr->node.entry);
                            }

if_body:                  statement
                            {
                                $$.then_instrs = $1;
                                $$.else_instrs = NULL;
                            }
                        | statement KW_ELSE statement
                            {
                                $$.then_instrs = $1;
                                $$.else_instrs = $3;
                            }

loop_statement:           KW_WHILE '(' expr ')' statement
                            {
                                struct source_location loc;
                                struct list *cond = d3dcompiler_alloc(sizeof(*cond));

                                if (!cond)
                                {
                                    ERR("Out of memory.\n");
                                    return -1;
                                }
                                list_init(cond);
                                list_add_head(cond, &$3->entry);
                                set_location(&loc, &@1);
                                $$ = create_loop(LOOP_WHILE, NULL, cond, NULL, $5, &loc);
                            }
                        | KW_DO statement KW_WHILE '(' expr ')' ';'
                            {
                                struct source_location loc;
                                struct list *cond = d3dcompiler_alloc(sizeof(*cond));

                                if (!cond)
                                {
                                    ERR("Out of memory.\n");
                                    return -1;
                                }
                                list_init(cond);
                                list_add_head(cond, &$5->entry);
                                set_location(&loc, &@1);
                                $$ = create_loop(LOOP_DO_WHILE, NULL, cond, NULL, $2, &loc);
                            }
                        | KW_FOR '(' scope_start expr_statement expr_statement expr ')' statement
                            {
                                struct source_location loc;

                                set_location(&loc, &@1);
                                $$ = create_loop(LOOP_FOR, $4, $5, $6, $8, &loc);
                                pop_scope(&hlsl_ctx);
                            }
                        | KW_FOR '(' scope_start declaration expr_statement expr ')' statement
                            {
                                struct source_location loc;

                                set_location(&loc, &@1);
                                if (!$4)
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_WARNING,
                                            "no expressions in for loop initializer");
                                $$ = create_loop(LOOP_FOR, $4, $5, $6, $8, &loc);
                                pop_scope(&hlsl_ctx);
                            }

expr_statement:           ';'
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                            }
                        | expr ';'
                            {
                                $$ = d3dcompiler_alloc(sizeof(*$$));
                                list_init($$);
                                if ($1)
                                    list_add_head($$, &$1->entry);
                            }

primary_expr:             C_FLOAT
                            {
                                struct hlsl_ir_constant *c = d3dcompiler_alloc(sizeof(*c));
                                if (!c)
                                {
                                    ERR("Out of memory.\n");
                                    return -1;
                                }
                                c->node.type = HLSL_IR_CONSTANT;
                                set_location(&c->node.loc, &yylloc);
                                c->node.data_type = new_hlsl_type(d3dcompiler_strdup("float"), HLSL_CLASS_SCALAR, HLSL_TYPE_FLOAT, 1, 1);
                                c->v.value.f[0] = $1;
                                $$ = &c->node;
                            }
                        | C_INTEGER
                            {
                                struct hlsl_ir_constant *c = d3dcompiler_alloc(sizeof(*c));
                                if (!c)
                                {
                                    ERR("Out of memory.\n");
                                    return -1;
                                }
                                c->node.type = HLSL_IR_CONSTANT;
                                set_location(&c->node.loc, &yylloc);
                                c->node.data_type = new_hlsl_type(d3dcompiler_strdup("int"), HLSL_CLASS_SCALAR, HLSL_TYPE_INT, 1, 1);
                                c->v.value.i[0] = $1;
                                $$ = &c->node;
                            }
                        | boolean
                            {
                                struct hlsl_ir_constant *c = d3dcompiler_alloc(sizeof(*c));
                                if (!c)
                                {
                                    ERR("Out of memory.\n");
                                    return -1;
                                }
                                c->node.type = HLSL_IR_CONSTANT;
                                set_location(&c->node.loc, &yylloc);
                                c->node.data_type = new_hlsl_type(d3dcompiler_strdup("bool"), HLSL_CLASS_SCALAR, HLSL_TYPE_BOOL, 1, 1);
                                c->v.value.b[0] = $1;
                                $$ = &c->node;
                            }
                        | variable
                            {
                                struct hlsl_ir_deref *deref = new_var_deref($1);
                                if (deref)
                                {
                                    $$ = &deref->node;
                                    set_location(&$$->loc, &@1);
                                }
                                else
                                    $$ = NULL;
                            }
                        | '(' expr ')'
                            {
                                $$ = $2;
                            }

variable:                 VAR_IDENTIFIER
                            {
                                struct hlsl_ir_var *var;
                                var = get_variable(hlsl_ctx.cur_scope, $1);
                                if (!var)
                                {
                                    hlsl_message("Line %d: variable '%s' not declared\n",
                                            hlsl_ctx.line_no, $1);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }
                                $$ = var;
                            }

postfix_expr:             primary_expr
                            {
                                $$ = $1;
                            }
                        | postfix_expr OP_INC
                            {
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                set_location(&loc, &@2);
                                if ($1->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "modifying a const expression");
                                    return 1;
                                }
                                operands[0] = $1;
                                operands[1] = operands[2] = NULL;
                                $$ = &new_expr(HLSL_IR_UNOP_POSTINC, operands, &loc)->node;
                                /* Post increment/decrement expressions are considered const */
                                $$->data_type = clone_hlsl_type($$->data_type);
                                $$->data_type->modifiers |= HLSL_MODIFIER_CONST;
                            }
                        | postfix_expr OP_DEC
                            {
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                set_location(&loc, &@2);
                                if ($1->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "modifying a const expression");
                                    return 1;
                                }
                                operands[0] = $1;
                                operands[1] = operands[2] = NULL;
                                $$ = &new_expr(HLSL_IR_UNOP_POSTDEC, operands, &loc)->node;
                                /* Post increment/decrement expressions are considered const */
                                $$->data_type = clone_hlsl_type($$->data_type);
                                $$->data_type->modifiers |= HLSL_MODIFIER_CONST;
                            }
                        | postfix_expr '.' any_identifier
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                if ($1->data_type->type == HLSL_CLASS_STRUCT)
                                {
                                    struct hlsl_type *type = $1->data_type;
                                    struct hlsl_struct_field *field;

                                    $$ = NULL;
                                    LIST_FOR_EACH_ENTRY(field, type->e.elements, struct hlsl_struct_field, entry)
                                    {
                                        if (!strcmp($3, field->name))
                                        {
                                            struct hlsl_ir_deref *deref = new_record_deref($1, field);

                                            if (!deref)
                                            {
                                                ERR("Out of memory\n");
                                                return -1;
                                            }
                                            deref->node.loc = loc;
                                            $$ = &deref->node;
                                            break;
                                        }
                                    }
                                    if (!$$)
                                    {
                                        hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                                "invalid subscript %s", debugstr_a($3));
                                        return 1;
                                    }
                                }
                                else if ($1->data_type->type <= HLSL_CLASS_LAST_NUMERIC)
                                {
                                    struct hlsl_ir_swizzle *swizzle;

                                    swizzle = get_swizzle($1, $3, &loc);
                                    if (!swizzle)
                                    {
                                        hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                                "invalid swizzle %s", debugstr_a($3));
                                        return 1;
                                    }
                                    $$ = &swizzle->node;
                                }
                                else
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "invalid subscript %s", debugstr_a($3));
                                    return 1;
                                }
                            }
                        | postfix_expr '[' expr ']'
                            {
                                /* This may be an array dereference or a vector/matrix
                                 * subcomponent access.
                                 * We store it as an array dereference in any case. */
                                struct hlsl_ir_deref *deref = d3dcompiler_alloc(sizeof(*deref));
                                struct hlsl_type *expr_type = $1->data_type;
                                struct source_location loc;

                                TRACE("Array dereference from type %s\n", debug_hlsl_type(expr_type));
                                if (!deref)
                                {
                                    ERR("Out of memory\n");
                                    return -1;
                                }
                                deref->node.type = HLSL_IR_DEREF;
                                set_location(&loc, &@2);
                                deref->node.loc = loc;
                                if (expr_type->type == HLSL_CLASS_ARRAY)
                                {
                                    deref->node.data_type = expr_type->e.array.type;
                                }
                                else if (expr_type->type == HLSL_CLASS_MATRIX)
                                {
                                    deref->node.data_type = new_hlsl_type(NULL, HLSL_CLASS_VECTOR, expr_type->base_type, expr_type->dimx, 1);
                                }
                                else if (expr_type->type == HLSL_CLASS_VECTOR)
                                {
                                    deref->node.data_type = new_hlsl_type(NULL, HLSL_CLASS_SCALAR, expr_type->base_type, 1, 1);
                                }
                                else
                                {
                                    if (expr_type->type == HLSL_CLASS_SCALAR)
                                        hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                                "array-indexed expression is scalar");
                                    else
                                        hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                                "expression is not array-indexable");
                                    d3dcompiler_free(deref);
                                    free_instr($1);
                                    free_instr($3);
                                    return 1;
                                }
                                if ($3->data_type->type != HLSL_CLASS_SCALAR)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "array index is not scalar");
                                    d3dcompiler_free(deref);
                                    free_instr($1);
                                    free_instr($3);
                                    return 1;
                                }
                                deref->type = HLSL_IR_DEREF_ARRAY;
                                deref->v.array.array = $1;
                                deref->v.array.index = $3;

                                $$ = &deref->node;
                            }
                          /* "var_modifiers" doesn't make sense in this case, but it's needed
                             in the grammar to avoid shift/reduce conflicts. */
                        | var_modifiers type '(' initializer_expr_list ')'
                            {
                                struct hlsl_ir_constructor *constructor;

                                TRACE("%s constructor.\n", debug_hlsl_type($2));
                                if ($1)
                                {
                                    hlsl_message("Line %u: unexpected modifier in a constructor.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return -1;
                                }
                                if ($2->type > HLSL_CLASS_LAST_NUMERIC)
                                {
                                    hlsl_message("Line %u: constructors are allowed only for numeric data types.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return -1;
                                }
                                if ($2->dimx * $2->dimy != components_count_expr_list($4))
                                {
                                    hlsl_message("Line %u: wrong number of components in constructor.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return -1;
                                }

                                constructor = d3dcompiler_alloc(sizeof(*constructor));
                                constructor->node.type = HLSL_IR_CONSTRUCTOR;
                                set_location(&constructor->node.loc, &@3);
                                constructor->node.data_type = $2;
                                constructor->arguments = $4;

                                $$ = &constructor->node;
                            }

unary_expr:               postfix_expr
                            {
                                $$ = $1;
                            }
                        | OP_INC unary_expr
                            {
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                set_location(&loc, &@1);
                                if ($2->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "modifying a const expression");
                                    return 1;
                                }
                                operands[0] = $2;
                                operands[1] = operands[2] = NULL;
                                $$ = &new_expr(HLSL_IR_UNOP_PREINC, operands, &loc)->node;
                            }
                        | OP_DEC unary_expr
                            {
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                set_location(&loc, &@1);
                                if ($2->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "modifying a const expression");
                                    return 1;
                                }
                                operands[0] = $2;
                                operands[1] = operands[2] = NULL;
                                $$ = &new_expr(HLSL_IR_UNOP_PREDEC, operands, &loc)->node;
                            }
                        | unary_op unary_expr
                            {
                                enum hlsl_ir_expr_op ops[] = {0, HLSL_IR_UNOP_NEG,
                                        HLSL_IR_UNOP_LOGIC_NOT, HLSL_IR_UNOP_BIT_NOT};
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                if ($1 == UNARY_OP_PLUS)
                                {
                                    $$ = $2;
                                }
                                else
                                {
                                    operands[0] = $2;
                                    operands[1] = operands[2] = NULL;
                                    set_location(&loc, &@1);
                                    $$ = &new_expr(ops[$1], operands, &loc)->node;
                                }
                            }
                          /* var_modifiers just to avoid shift/reduce conflicts */
                        | '(' var_modifiers type array ')' unary_expr
                            {
                                struct hlsl_ir_expr *expr;
                                struct hlsl_type *src_type = $6->data_type;
                                struct hlsl_type *dst_type;
                                struct source_location loc;

                                set_location(&loc, &@3);
                                if ($2)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "unexpected modifier in a cast");
                                    return 1;
                                }

                                if ($4)
                                    dst_type = new_array_type($3, $4);
                                else
                                    dst_type = $3;

                                if (!compatible_data_types(src_type, dst_type))
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "can't cast from %s to %s",
                                            debug_hlsl_type(src_type), debug_hlsl_type(dst_type));
                                    return 1;
                                }

                                expr = new_cast($6, dst_type, &loc);
                                $$ = expr ? &expr->node : NULL;
                            }

unary_op:                 '+'
                            {
                                $$ = UNARY_OP_PLUS;
                            }
                        | '-'
                            {
                                $$ = UNARY_OP_MINUS;
                            }
                        | '!'
                            {
                                $$ = UNARY_OP_LOGICNOT;
                            }
                        | '~'
                            {
                                $$ = UNARY_OP_BITNOT;
                            }

mul_expr:                 unary_expr
                            {
                                $$ = $1;
                            }
                        | mul_expr '*' unary_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_mul($1, $3, &loc)->node;
                            }
                        | mul_expr '/' unary_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_div($1, $3, &loc)->node;
                            }
                        | mul_expr '%' unary_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_mod($1, $3, &loc)->node;
                            }

add_expr:                 mul_expr
                            {
                                $$ = $1;
                            }
                        | add_expr '+' mul_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_add($1, $3, &loc)->node;
                            }
                        | add_expr '-' mul_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_sub($1, $3, &loc)->node;
                            }

shift_expr:               add_expr
                            {
                                $$ = $1;
                            }
                        | shift_expr OP_LEFTSHIFT add_expr
                            {
                                FIXME("Left shift\n");
                            }
                        | shift_expr OP_RIGHTSHIFT add_expr
                            {
                                FIXME("Right shift\n");
                            }

relational_expr:          shift_expr
                            {
                                $$ = $1;
                            }
                        | relational_expr '<' shift_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_lt($1, $3, &loc)->node;
                            }
                        | relational_expr '>' shift_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_gt($1, $3, &loc)->node;
                            }
                        | relational_expr OP_LE shift_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_le($1, $3, &loc)->node;
                            }
                        | relational_expr OP_GE shift_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_ge($1, $3, &loc)->node;
                            }

equality_expr:            relational_expr
                            {
                                $$ = $1;
                            }
                        | equality_expr OP_EQ relational_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_eq($1, $3, &loc)->node;
                            }
                        | equality_expr OP_NE relational_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                $$ = &hlsl_ne($1, $3, &loc)->node;
                            }

bitand_expr:              equality_expr
                            {
                                $$ = $1;
                            }
                        | bitand_expr '&' equality_expr
                            {
                                FIXME("bitwise AND\n");
                            }

bitxor_expr:              bitand_expr
                            {
                                $$ = $1;
                            }
                        | bitxor_expr '^' bitand_expr
                            {
                                FIXME("bitwise XOR\n");
                            }

bitor_expr:               bitxor_expr
                            {
                                $$ = $1;
                            }
                        | bitor_expr '|' bitxor_expr
                            {
                                FIXME("bitwise OR\n");
                            }

logicand_expr:            bitor_expr
                            {
                                $$ = $1;
                            }
                        | logicand_expr OP_AND bitor_expr
                            {
                                FIXME("logic AND\n");
                            }

logicor_expr:             logicand_expr
                            {
                                $$ = $1;
                            }
                        | logicor_expr OP_OR logicand_expr
                            {
                                FIXME("logic OR\n");
                            }

conditional_expr:         logicor_expr
                            {
                                $$ = $1;
                            }
                        | logicor_expr '?' expr ':' assignment_expr
                            {
                                FIXME("ternary operator\n");
                            }

assignment_expr:          conditional_expr
                            {
                                $$ = $1;
                            }
                        | unary_expr assign_op assignment_expr
                            {
                                struct source_location loc;

                                set_location(&loc, &@2);
                                if ($1->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "l-value is const");
                                    return 1;
                                }
                                $$ = make_assignment($1, $2, BWRITERSP_WRITEMASK_ALL, $3);
                                if (!$$)
                                    return 1;
                                $$->loc = loc;
                            }

assign_op:                '='
                            {
                                $$ = ASSIGN_OP_ASSIGN;
                            }
                        | OP_ADDASSIGN
                            {
                                $$ = ASSIGN_OP_ADD;
                            }
                        | OP_SUBASSIGN
                            {
                                $$ = ASSIGN_OP_SUB;
                            }
                        | OP_MULASSIGN
                            {
                                $$ = ASSIGN_OP_MUL;
                            }
                        | OP_DIVASSIGN
                            {
                                $$ = ASSIGN_OP_DIV;
                            }
                        | OP_MODASSIGN
                            {
                                $$ = ASSIGN_OP_MOD;
                            }
                        | OP_LEFTSHIFTASSIGN
                            {
                                $$ = ASSIGN_OP_LSHIFT;
                            }
                        | OP_RIGHTSHIFTASSIGN
                            {
                                $$ = ASSIGN_OP_RSHIFT;
                            }
                        | OP_ANDASSIGN
                            {
                                $$ = ASSIGN_OP_AND;
                            }
                        | OP_ORASSIGN
                            {
                                $$ = ASSIGN_OP_OR;
                            }
                        | OP_XORASSIGN
                            {
                                $$ = ASSIGN_OP_XOR;
                            }

expr:                     assignment_expr
                            {
                                $$ = $1;
                            }
                        | expr ',' assignment_expr
                            {
                                FIXME("Comma expression\n");
                            }

%%

static void set_location(struct source_location *loc, const struct YYLTYPE *l)
{
    loc->file = hlsl_ctx.source_file;
    loc->line = l->first_line;
    loc->col = l->first_column;
}

static DWORD add_modifier(DWORD modifiers, DWORD mod, const struct YYLTYPE *loc)
{
    if (modifiers & mod)
    {
        hlsl_report_message(hlsl_ctx.source_file, loc->first_line, loc->first_column, HLSL_LEVEL_ERROR,
                "modifier '%s' already specified", debug_modifiers(mod));
        return modifiers;
    }
    if (mod & (HLSL_MODIFIER_ROW_MAJOR | HLSL_MODIFIER_COLUMN_MAJOR)
            && modifiers & (HLSL_MODIFIER_ROW_MAJOR | HLSL_MODIFIER_COLUMN_MAJOR))
    {
        hlsl_report_message(hlsl_ctx.source_file, loc->first_line, loc->first_column, HLSL_LEVEL_ERROR,
                "more than one matrix majority keyword");
        return modifiers;
    }
    return modifiers | mod;
}

static void dump_function_decl(struct wine_rb_entry *entry, void *context)
{
    struct hlsl_ir_function_decl *func = WINE_RB_ENTRY_VALUE(entry, struct hlsl_ir_function_decl, entry);
    if (func->body)
        debug_dump_ir_function_decl(func);
}

static void dump_function(struct wine_rb_entry *entry, void *context)
{
    struct hlsl_ir_function *func = WINE_RB_ENTRY_VALUE(entry, struct hlsl_ir_function, entry);
    wine_rb_for_each_entry(&func->overloads, dump_function_decl, NULL);
}

struct bwriter_shader *parse_hlsl(enum shader_type type, DWORD major, DWORD minor,
        const char *entrypoint, char **messages)
{
    struct hlsl_scope *scope, *next_scope;
    struct hlsl_type *hlsl_type, *next_type;
    struct hlsl_ir_var *var, *next_var;
    unsigned int i;

    hlsl_ctx.status = PARSE_SUCCESS;
    hlsl_ctx.messages.size = hlsl_ctx.messages.capacity = 0;
    hlsl_ctx.line_no = hlsl_ctx.column = 1;
    hlsl_ctx.source_file = d3dcompiler_strdup("");
    hlsl_ctx.source_files = d3dcompiler_alloc(sizeof(*hlsl_ctx.source_files));
    if (hlsl_ctx.source_files)
        hlsl_ctx.source_files[0] = hlsl_ctx.source_file;
    hlsl_ctx.source_files_count = 1;
    hlsl_ctx.cur_scope = NULL;
    hlsl_ctx.matrix_majority = HLSL_COLUMN_MAJOR;
    list_init(&hlsl_ctx.scopes);
    list_init(&hlsl_ctx.types);
    init_functions_tree(&hlsl_ctx.functions);

    push_scope(&hlsl_ctx);
    hlsl_ctx.globals = hlsl_ctx.cur_scope;
    declare_predefined_types(hlsl_ctx.globals);

    hlsl_parse();

    if (TRACE_ON(hlsl_parser))
    {
        TRACE("IR dump.\n");
        wine_rb_for_each_entry(&hlsl_ctx.functions, dump_function, NULL);
    }

    TRACE("Compilation status = %d\n", hlsl_ctx.status);
    if (messages)
    {
        if (hlsl_ctx.messages.size)
            *messages = hlsl_ctx.messages.string;
        else
            *messages = NULL;
    }
    else
    {
        if (hlsl_ctx.messages.capacity)
            d3dcompiler_free(hlsl_ctx.messages.string);
    }

    for (i = 0; i < hlsl_ctx.source_files_count; ++i)
        d3dcompiler_free((void *)hlsl_ctx.source_files[i]);
    d3dcompiler_free(hlsl_ctx.source_files);

    TRACE("Freeing functions IR.\n");
    wine_rb_destroy(&hlsl_ctx.functions, free_function_rb, NULL);

    TRACE("Freeing variables.\n");
    LIST_FOR_EACH_ENTRY_SAFE(scope, next_scope, &hlsl_ctx.scopes, struct hlsl_scope, entry)
    {
        LIST_FOR_EACH_ENTRY_SAFE(var, next_var, &scope->vars, struct hlsl_ir_var, scope_entry)
        {
            free_declaration(var);
        }
        wine_rb_destroy(&scope->types, NULL, NULL);
        d3dcompiler_free(scope);
    }

    TRACE("Freeing types.\n");
    LIST_FOR_EACH_ENTRY_SAFE(hlsl_type, next_type, &hlsl_ctx.types, struct hlsl_type, entry)
    {
        free_hlsl_type(hlsl_type);
    }

    return NULL;
}
