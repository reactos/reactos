/* A Bison parser, made by GNU Bison 3.4.1.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         hlsl_parse
#define yylex           hlsl_lex
#define yyerror         hlsl_error
#define yydebug         hlsl_debug
#define yynerrs         hlsl_nerrs

#define yylval          hlsl_lval
#define yychar          hlsl_char
#define yylloc          hlsl_lloc

/* First part of user prologue.  */
#line 21 "hlsl.y"

#include "wine/debug.h"

#include <stdio.h>

#include "d3dcompiler_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(hlsl_parser);

int hlsl_lex(void);

struct hlsl_parse_ctx hlsl_ctx;

struct YYLTYPE;
static void set_location(struct source_location *loc, const struct YYLTYPE *l);

void WINAPIV hlsl_message(const char *fmt, ...)
{
    __ms_va_list args;

    __ms_va_start(args, fmt);
    compilation_message(&hlsl_ctx.messages, fmt, args);
    __ms_va_end(args);
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

void WINAPIV hlsl_report_message(const char *filename, DWORD line, DWORD column,
        enum hlsl_error_level level, const char *fmt, ...)
{
    __ms_va_list args;
    char *string = NULL;
    int rc, size = 0;

    while (1)
    {
        __ms_va_start(args, fmt);
        rc = vsnprintf(string, size, fmt, args);
        __ms_va_end(args);

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
    if (decl->data_type->type == HLSL_CLASS_MATRIX)
    {
        if (!(decl->modifiers & (HLSL_MODIFIER_ROW_MAJOR | HLSL_MODIFIER_COLUMN_MAJOR)))
        {
            decl->modifiers |= hlsl_ctx.matrix_majority == HLSL_ROW_MAJOR
                    ? HLSL_MODIFIER_ROW_MAJOR : HLSL_MODIFIER_COLUMN_MAJOR;
        }
    }
    else
        check_invalid_matrix_modifiers(decl->modifiers, &decl->loc);

    if (local)
    {
        DWORD invalid = decl->modifiers & (HLSL_STORAGE_EXTERN | HLSL_STORAGE_SHARED
                | HLSL_STORAGE_GROUPSHARED | HLSL_STORAGE_UNIFORM);
        if (invalid)
        {
            hlsl_report_message(decl->loc.file, decl->loc.line, decl->loc.col, HLSL_LEVEL_ERROR,
                    "modifier '%s' invalid for local variables", debug_modifiers(invalid));
        }
        if (decl->semantic)
        {
            hlsl_report_message(decl->loc.file, decl->loc.line, decl->loc.col, HLSL_LEVEL_ERROR,
                    "semantics are not allowed on local variables");
            return FALSE;
        }
    }
    else
    {
        if (find_function(decl->name))
        {
            hlsl_report_message(decl->loc.file, decl->loc.line, decl->loc.col, HLSL_LEVEL_ERROR,
                    "redefinition of '%s'", decl->name);
            return FALSE;
        }
    }
    ret = add_declaration(hlsl_ctx.cur_scope, decl, local);
    if (!ret)
    {
        struct hlsl_ir_var *old = get_variable(hlsl_ctx.cur_scope, decl->name);

        hlsl_report_message(decl->loc.file, decl->loc.line, decl->loc.col, HLSL_LEVEL_ERROR,
                "\"%s\" already declared", decl->name);
        hlsl_report_message(old->loc.file, old->loc.line, old->loc.col, HLSL_LEVEL_NOTE,
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
    struct hlsl_ir_node *cond, *not_cond;
    struct hlsl_ir_if *out_cond;
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
    if (!(not_cond = new_unary_expr(HLSL_IR_UNOP_LOGIC_NOT, cond, cond->loc)))
    {
        ERR("Out of memory.\n");
        d3dcompiler_free(out_cond);
        return NULL;
    }
    out_cond->condition = not_cond;
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

static unsigned int initializer_size(const struct parse_initializer *initializer)
{
    unsigned int count = 0, i;

    for (i = 0; i < initializer->args_count; ++i)
    {
        count += components_count_type(initializer->args[i]->data_type);
    }
    TRACE("Initializer size = %u.\n", count);
    return count;
}

static void free_parse_initializer(struct parse_initializer *initializer)
{
    unsigned int i;
    for (i = 0; i < initializer->args_count; ++i)
        free_instr(initializer->args[i]);
    d3dcompiler_free(initializer->args);
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

static void struct_var_initializer(struct list *list, struct hlsl_ir_var *var,
        struct parse_initializer *initializer)
{
    struct hlsl_type *type = var->data_type;
    struct hlsl_struct_field *field;
    struct hlsl_ir_node *assignment;
    struct hlsl_ir_deref *deref;
    unsigned int i = 0;

    if (initializer_size(initializer) != components_count_type(type))
    {
        hlsl_report_message(var->loc.file, var->loc.line, var->loc.col, HLSL_LEVEL_ERROR,
                "structure initializer mismatch");
        free_parse_initializer(initializer);
        return;
    }

    LIST_FOR_EACH_ENTRY(field, type->e.elements, struct hlsl_struct_field, entry)
    {
        struct hlsl_ir_node *node = initializer->args[i];

        if (i++ >= initializer->args_count)
        {
            d3dcompiler_free(initializer->args);
            return;
        }
        if (components_count_type(field->type) == components_count_type(node->data_type))
        {
            deref = new_record_deref(&new_var_deref(var)->node, field);
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
    }

    /* Free initializer elements in excess. */
    for (; i < initializer->args_count; ++i)
        free_instr(initializer->args[i]);
    d3dcompiler_free(initializer->args);
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
        if (v->array_size)
            type = new_array_type(basic_type, v->array_size);
        else
            type = basic_type;
        var->data_type = type;
        var->loc = v->loc;
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

        if (var->modifiers & HLSL_MODIFIER_CONST && !(var->modifiers & HLSL_STORAGE_UNIFORM) && !v->initializer.args_count)
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

        if (v->initializer.args_count)
        {
            unsigned int size = initializer_size(&v->initializer);

            TRACE("Variable with initializer.\n");
            if (type->type <= HLSL_CLASS_LAST_NUMERIC
                    && type->dimx * type->dimy != size && size != 1)
            {
                if (size < type->dimx * type->dimy)
                {
                    hlsl_report_message(v->loc.file, v->loc.line, v->loc.col, HLSL_LEVEL_ERROR,
                            "'%s' initializer does not match", v->name);
                    free_parse_initializer(&v->initializer);
                    d3dcompiler_free(v);
                    continue;
                }
            }
            if ((type->type == HLSL_CLASS_STRUCT || type->type == HLSL_CLASS_ARRAY)
                    && components_count_type(type) != size)
            {
                hlsl_report_message(v->loc.file, v->loc.line, v->loc.col, HLSL_LEVEL_ERROR,
                        "'%s' initializer does not match", v->name);
                free_parse_initializer(&v->initializer);
                d3dcompiler_free(v);
                continue;
            }

            if (type->type == HLSL_CLASS_STRUCT)
            {
                struct_var_initializer(statements_list, var, &v->initializer);
                d3dcompiler_free(v);
                continue;
            }
            if (type->type > HLSL_CLASS_LAST_NUMERIC)
            {
                FIXME("Initializers for non scalar/struct variables not supported yet.\n");
                free_parse_initializer(&v->initializer);
                d3dcompiler_free(v);
                continue;
            }
            if (v->array_size > 0)
            {
                FIXME("Initializing arrays is not supported yet.\n");
                free_parse_initializer(&v->initializer);
                d3dcompiler_free(v);
                continue;
            }
            if (v->initializer.args_count > 1)
            {
                FIXME("Complex initializers are not supported yet.\n");
                free_parse_initializer(&v->initializer);
                d3dcompiler_free(v);
                continue;
            }

            assignment = make_assignment(&new_var_deref(var)->node, ASSIGN_OP_ASSIGN,
                    BWRITERSP_WRITEMASK_ALL, v->initializer.args[0]);
            d3dcompiler_free(v->initializer.args);
            list_add_tail(statements_list, &assignment->entry);
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
        if (v->initializer.args_count)
        {
            hlsl_report_message(v->loc.file, v->loc.line, v->loc.col, HLSL_LEVEL_ERROR,
                    "struct field with an initializer.\n");
            free_parse_initializer(&v->initializer);
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
    decl->data_type = param->type;
    decl->loc = *loc;
    decl->name = param->name;
    decl->semantic = param->semantic;
    decl->reg_reservation = param->reg_reservation;
    decl->modifiers = param->modifiers;

    if (!add_declaration(hlsl_ctx.cur_scope, decl, FALSE))
    {
        free_declaration(decl);
        return FALSE;
    }
    list_add_tail(list, &decl->param_entry);
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


#line 943 "hlsl.tab.c"

# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_HLSL_E_REACTOSSYNC_GCC_DLL_DIRECTX_WINE_D3DCOMPILER_43_HLSL_TAB_H_INCLUDED
# define YY_HLSL_E_REACTOSSYNC_GCC_DLL_DIRECTX_WINE_D3DCOMPILER_43_HLSL_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int hlsl_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    KW_BLENDSTATE = 258,
    KW_BREAK = 259,
    KW_BUFFER = 260,
    KW_CBUFFER = 261,
    KW_COLUMN_MAJOR = 262,
    KW_COMPILE = 263,
    KW_CONST = 264,
    KW_CONTINUE = 265,
    KW_DEPTHSTENCILSTATE = 266,
    KW_DEPTHSTENCILVIEW = 267,
    KW_DISCARD = 268,
    KW_DO = 269,
    KW_DOUBLE = 270,
    KW_ELSE = 271,
    KW_EXTERN = 272,
    KW_FALSE = 273,
    KW_FOR = 274,
    KW_GEOMETRYSHADER = 275,
    KW_GROUPSHARED = 276,
    KW_IF = 277,
    KW_IN = 278,
    KW_INLINE = 279,
    KW_INOUT = 280,
    KW_MATRIX = 281,
    KW_NAMESPACE = 282,
    KW_NOINTERPOLATION = 283,
    KW_OUT = 284,
    KW_PASS = 285,
    KW_PIXELSHADER = 286,
    KW_PRECISE = 287,
    KW_RASTERIZERSTATE = 288,
    KW_RENDERTARGETVIEW = 289,
    KW_RETURN = 290,
    KW_REGISTER = 291,
    KW_ROW_MAJOR = 292,
    KW_SAMPLER = 293,
    KW_SAMPLER1D = 294,
    KW_SAMPLER2D = 295,
    KW_SAMPLER3D = 296,
    KW_SAMPLERCUBE = 297,
    KW_SAMPLER_STATE = 298,
    KW_SAMPLERCOMPARISONSTATE = 299,
    KW_SHARED = 300,
    KW_STATEBLOCK = 301,
    KW_STATEBLOCK_STATE = 302,
    KW_STATIC = 303,
    KW_STRING = 304,
    KW_STRUCT = 305,
    KW_SWITCH = 306,
    KW_TBUFFER = 307,
    KW_TECHNIQUE = 308,
    KW_TECHNIQUE10 = 309,
    KW_TEXTURE = 310,
    KW_TEXTURE1D = 311,
    KW_TEXTURE1DARRAY = 312,
    KW_TEXTURE2D = 313,
    KW_TEXTURE2DARRAY = 314,
    KW_TEXTURE2DMS = 315,
    KW_TEXTURE2DMSARRAY = 316,
    KW_TEXTURE3D = 317,
    KW_TEXTURE3DARRAY = 318,
    KW_TEXTURECUBE = 319,
    KW_TRUE = 320,
    KW_TYPEDEF = 321,
    KW_UNIFORM = 322,
    KW_VECTOR = 323,
    KW_VERTEXSHADER = 324,
    KW_VOID = 325,
    KW_VOLATILE = 326,
    KW_WHILE = 327,
    OP_INC = 328,
    OP_DEC = 329,
    OP_AND = 330,
    OP_OR = 331,
    OP_EQ = 332,
    OP_LEFTSHIFT = 333,
    OP_LEFTSHIFTASSIGN = 334,
    OP_RIGHTSHIFT = 335,
    OP_RIGHTSHIFTASSIGN = 336,
    OP_ELLIPSIS = 337,
    OP_LE = 338,
    OP_GE = 339,
    OP_NE = 340,
    OP_ADDASSIGN = 341,
    OP_SUBASSIGN = 342,
    OP_MULASSIGN = 343,
    OP_DIVASSIGN = 344,
    OP_MODASSIGN = 345,
    OP_ANDASSIGN = 346,
    OP_ORASSIGN = 347,
    OP_XORASSIGN = 348,
    OP_UNKNOWN1 = 349,
    OP_UNKNOWN2 = 350,
    OP_UNKNOWN3 = 351,
    OP_UNKNOWN4 = 352,
    PRE_LINE = 353,
    VAR_IDENTIFIER = 354,
    TYPE_IDENTIFIER = 355,
    NEW_IDENTIFIER = 356,
    STRING = 357,
    C_FLOAT = 358,
    C_INTEGER = 359
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 890 "hlsl.y"

    struct hlsl_type *type;
    INT intval;
    FLOAT floatval;
    BOOL boolval;
    char *name;
    DWORD modifiers;
    struct hlsl_ir_node *instr;
    struct list *list;
    struct parse_function function;
    struct parse_parameter parameter;
    struct parse_initializer initializer;
    struct parse_variable_def *variable_def;
    struct parse_if_body if_body;
    enum parse_unary_op unary_op;
    enum parse_assign_op assign_op;
    struct reg_reservation *reg_reservation;
    struct parse_colon_attribute colon_attribute;

#line 1111 "hlsl.tab.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


extern YYSTYPE hlsl_lval;
extern YYLTYPE hlsl_lloc;
int hlsl_parse (void);

#endif /* !YY_HLSL_E_REACTOSSYNC_GCC_DLL_DIRECTX_WINE_D3DCOMPILER_43_HLSL_TAB_H_INCLUDED  */



#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   820

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  129
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  64
/* YYNRULES -- Number of rules.  */
#define YYNRULES  174
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  320

#define YYUNDEFTOK  2
#define YYMAXUTOK   359

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   120,     2,     2,     2,   124,   125,     2,
     108,   109,   122,   118,   111,   119,   117,   123,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   110,   105,
     112,   114,   113,   128,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   115,     2,   116,   126,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   106,   127,   107,   121,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1074,  1074,  1076,  1113,  1117,  1120,  1125,  1147,  1164,
    1165,  1167,  1193,  1203,  1204,  1205,  1208,  1212,  1231,  1235,
    1240,  1247,  1254,  1284,  1289,  1296,  1300,  1301,  1304,  1308,
    1313,  1319,  1325,  1330,  1339,  1344,  1349,  1363,  1377,  1388,
    1391,  1402,  1406,  1410,  1415,  1419,  1438,  1458,  1462,  1467,
    1472,  1477,  1482,  1487,  1495,  1513,  1514,  1515,  1526,  1534,
    1543,  1549,  1555,  1563,  1569,  1572,  1577,  1583,  1589,  1598,
    1611,  1614,  1622,  1625,  1629,  1633,  1637,  1641,  1645,  1649,
    1653,  1657,  1661,  1665,  1670,  1677,  1681,  1686,  1691,  1698,
    1706,  1710,  1715,  1719,  1726,  1727,  1728,  1729,  1730,  1731,
    1734,  1757,  1781,  1786,  1792,  1807,  1822,  1830,  1842,  1847,
    1855,  1869,  1883,  1897,  1917,  1922,  1926,  1942,  1958,  2012,
    2072,  2110,  2114,  2127,  2140,  2157,  2189,  2193,  2197,  2201,
    2206,  2210,  2217,  2224,  2232,  2236,  2243,  2251,  2255,  2259,
    2264,  2268,  2275,  2282,  2289,  2297,  2301,  2308,  2316,  2320,
    2325,  2329,  2334,  2338,  2343,  2347,  2352,  2356,  2361,  2365,
    2370,  2374,  2391,  2395,  2399,  2403,  2407,  2411,  2415,  2419,
    2423,  2427,  2431,  2436,  2440
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "KW_BLENDSTATE", "KW_BREAK", "KW_BUFFER",
  "KW_CBUFFER", "KW_COLUMN_MAJOR", "KW_COMPILE", "KW_CONST", "KW_CONTINUE",
  "KW_DEPTHSTENCILSTATE", "KW_DEPTHSTENCILVIEW", "KW_DISCARD", "KW_DO",
  "KW_DOUBLE", "KW_ELSE", "KW_EXTERN", "KW_FALSE", "KW_FOR",
  "KW_GEOMETRYSHADER", "KW_GROUPSHARED", "KW_IF", "KW_IN", "KW_INLINE",
  "KW_INOUT", "KW_MATRIX", "KW_NAMESPACE", "KW_NOINTERPOLATION", "KW_OUT",
  "KW_PASS", "KW_PIXELSHADER", "KW_PRECISE", "KW_RASTERIZERSTATE",
  "KW_RENDERTARGETVIEW", "KW_RETURN", "KW_REGISTER", "KW_ROW_MAJOR",
  "KW_SAMPLER", "KW_SAMPLER1D", "KW_SAMPLER2D", "KW_SAMPLER3D",
  "KW_SAMPLERCUBE", "KW_SAMPLER_STATE", "KW_SAMPLERCOMPARISONSTATE",
  "KW_SHARED", "KW_STATEBLOCK", "KW_STATEBLOCK_STATE", "KW_STATIC",
  "KW_STRING", "KW_STRUCT", "KW_SWITCH", "KW_TBUFFER", "KW_TECHNIQUE",
  "KW_TECHNIQUE10", "KW_TEXTURE", "KW_TEXTURE1D", "KW_TEXTURE1DARRAY",
  "KW_TEXTURE2D", "KW_TEXTURE2DARRAY", "KW_TEXTURE2DMS",
  "KW_TEXTURE2DMSARRAY", "KW_TEXTURE3D", "KW_TEXTURE3DARRAY",
  "KW_TEXTURECUBE", "KW_TRUE", "KW_TYPEDEF", "KW_UNIFORM", "KW_VECTOR",
  "KW_VERTEXSHADER", "KW_VOID", "KW_VOLATILE", "KW_WHILE", "OP_INC",
  "OP_DEC", "OP_AND", "OP_OR", "OP_EQ", "OP_LEFTSHIFT",
  "OP_LEFTSHIFTASSIGN", "OP_RIGHTSHIFT", "OP_RIGHTSHIFTASSIGN",
  "OP_ELLIPSIS", "OP_LE", "OP_GE", "OP_NE", "OP_ADDASSIGN", "OP_SUBASSIGN",
  "OP_MULASSIGN", "OP_DIVASSIGN", "OP_MODASSIGN", "OP_ANDASSIGN",
  "OP_ORASSIGN", "OP_XORASSIGN", "OP_UNKNOWN1", "OP_UNKNOWN2",
  "OP_UNKNOWN3", "OP_UNKNOWN4", "PRE_LINE", "VAR_IDENTIFIER",
  "TYPE_IDENTIFIER", "NEW_IDENTIFIER", "STRING", "C_FLOAT", "C_INTEGER",
  "';'", "'{'", "'}'", "'('", "')'", "':'", "','", "'<'", "'>'", "'='",
  "'['", "']'", "'.'", "'+'", "'-'", "'!'", "'~'", "'*'", "'/'", "'%'",
  "'&'", "'^'", "'|'", "'?'", "$accept", "hlsl_prog", "preproc_directive",
  "struct_declaration", "struct_spec", "named_struct_spec",
  "unnamed_struct_spec", "any_identifier", "fields_list", "field",
  "func_declaration", "func_prototype", "compound_statement",
  "scope_start", "var_identifier", "colon_attribute", "semantic",
  "register_opt", "parameters", "param_list", "parameter", "input_mods",
  "input_mod", "type", "base_type", "declaration_statement", "typedef",
  "type_specs", "type_spec", "declaration", "variables_def_optional",
  "variables_def", "variable_def", "array", "var_modifiers",
  "complex_initializer", "initializer_expr", "initializer_expr_list",
  "boolean", "statement_list", "statement", "jump_statement",
  "selection_statement", "if_body", "loop_statement", "expr_statement",
  "primary_expr", "postfix_expr", "unary_expr", "unary_op", "mul_expr",
  "add_expr", "shift_expr", "relational_expr", "equality_expr",
  "bitand_expr", "bitxor_expr", "bitor_expr", "logicand_expr",
  "logicor_expr", "conditional_expr", "assignment_expr", "assign_op",
  "expr", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,    59,   123,   125,    40,    41,
      58,    44,    60,    62,    61,    91,    93,    46,    43,    45,
      33,   126,    42,    47,    37,    38,    94,   124,    63
};
# endif

#define YYPACT_NINF -228

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-228)))

#define YYTABLE_NINF -35

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -228,   673,  -228,   749,   749,   749,   749,   749,   749,   749,
     749,   749,   749,   749,   749,   -66,  -228,  -228,  -228,    80,
    -228,  -228,  -228,   107,  -228,  -228,  -228,    38,  -228,  -228,
    -228,  -228,  -228,  -228,  -228,  -228,  -228,    80,    38,  -228,
    -228,  -228,  -228,  -228,  -228,   -64,   -51,   -72,  -228,  -228,
     -44,  -228,   -29,  -228,  -228,  -228,  -228,  -228,    24,   -20,
    -228,  -228,    83,  -228,   -64,   -55,  -228,    80,   612,    33,
    -228,    80,  -228,   353,     5,     1,  -228,     8,     5,    77,
      92,    95,   -10,  -228,  -228,    80,     4,  -228,  -228,   612,
     612,  -228,  -228,  -228,   612,  -228,  -228,  -228,  -228,   257,
    -228,  -228,   -15,   137,   612,    97,    70,   -54,    86,   -24,
     118,    96,   129,   175,   -58,  -228,  -228,   -76,    -3,   141,
    -228,  -228,  -228,   353,   150,   152,   612,   160,  -228,  -228,
    -228,    38,   245,  -228,  -228,  -228,  -228,  -228,    21,   172,
     167,    20,  -228,   168,  -228,  -228,  -228,  -228,  -228,  -228,
     257,   -38,   173,  -228,  -228,   612,    80,  -228,  -228,  -228,
    -228,  -228,  -228,  -228,  -228,  -228,  -228,  -228,   612,  -228,
     612,   612,   612,   612,   612,   612,   612,   612,   612,   612,
     612,   612,   612,   612,   612,   612,   612,   612,   612,   612,
    -228,   178,  -228,   432,   215,  -228,   612,    23,   612,    11,
    -228,  -228,  -228,  -228,   184,  -228,    80,  -228,   365,   169,
     185,   182,   183,   -85,  -228,   612,   -12,  -228,  -228,  -228,
    -228,  -228,  -228,    97,    97,    70,    70,   -54,   -54,   -54,
     -54,    86,    86,   -24,   118,    96,   129,   175,   128,  -228,
      80,   612,  -228,  -228,  -228,   186,   472,    82,  -228,    85,
     189,    28,    29,    80,  -228,   188,   191,  -228,   720,    33,
     194,  -228,    98,  -228,   612,   122,    10,   612,   472,   257,
     472,   353,   353,   200,  -228,    66,  -228,  -228,  -228,  -228,
    -228,  -228,   257,  -228,   612,  -228,   612,  -228,  -228,    80,
    -228,   551,   123,   612,   612,   289,  -228,  -228,   193,  -228,
    -228,    80,  -228,  -228,   206,  -228,   204,   126,   135,   353,
    -228,    33,  -228,  -228,   353,   353,  -228,  -228,  -228,  -228
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,    72,     1,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,     0,     6,     5,    56,    64,
       9,    10,     3,     0,     4,    57,    55,     0,    83,    81,
      73,    77,    74,    75,    82,    76,    78,     0,     0,    79,
      80,     7,    13,    14,    15,    70,     0,    65,    66,    21,
      25,    20,     0,    48,    49,    50,    51,    52,     0,     0,
      47,    53,     0,    44,    70,     0,    60,     0,    72,    28,
       8,     0,    23,    72,     0,    54,    16,     0,     0,    13,
      15,     0,     0,    62,    59,     0,     0,    91,    90,    72,
      72,   113,   110,   111,    72,   126,   127,   128,   129,     0,
     112,   115,   121,   130,    72,   134,   137,   140,   145,   148,
     150,   152,   154,   156,   158,   160,   173,     0,     0,    68,
      29,    30,    67,    72,     0,     0,    72,     0,   108,    96,
      94,     0,    72,    92,    97,    98,    99,    95,     0,     0,
       0,    72,    16,     0,    25,    63,    61,    58,   122,   123,
       0,     0,     0,   116,   117,    72,     0,   168,   169,   163,
     164,   165,   166,   167,   170,   171,   172,   162,    72,   124,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      71,     0,    31,    72,     0,    25,    72,     0,    72,     0,
      24,    93,   109,    54,     0,    12,     0,    17,     0,    72,
       0,    39,     0,    70,   114,    72,     0,   118,   161,   131,
     132,   133,   130,   135,   136,   138,   139,   143,   144,   141,
     142,   146,   147,   149,   151,   153,   155,   157,     0,   174,
       0,    72,    69,    84,    87,     0,    72,     0,   100,     0,
       0,     0,     0,     0,    11,     0,    35,    36,    72,    28,
       0,    88,     0,   119,    72,     0,     0,    72,    72,     0,
      72,    72,    72,     0,    19,     0,    45,    39,    41,    43,
      42,    40,     0,    22,    72,   120,    72,   159,    32,     0,
      85,    72,     0,    72,    72,   102,   101,   104,     0,    18,
      37,     0,   125,    89,     0,    86,     0,     0,     0,    72,
      46,    28,    33,   105,    72,    72,   103,    38,   107,   106
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -228,  -228,  -228,  -228,   308,  -228,  -120,   -36,   179,  -228,
    -228,  -228,   299,  -110,  -228,  -227,  -228,  -228,  -228,  -228,
      46,  -228,  -228,   -13,    67,   323,  -228,   259,   243,    84,
    -228,    -4,   258,   -47,    -1,  -228,  -173,    90,  -228,  -228,
    -104,  -228,  -228,  -228,  -228,  -208,  -228,  -228,   -23,  -228,
      74,    99,    -5,   103,   149,   151,   148,   153,   147,  -228,
    -228,   -99,  -228,   -52
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    17,    18,    19,    20,    21,    45,   141,   207,
      22,    23,   129,    73,    81,   119,   120,   121,   212,   256,
     257,   258,   281,   199,    63,   130,    25,    65,    66,    26,
      46,    82,    48,    69,    99,   242,   261,   262,   100,   132,
     133,   134,   135,   296,   136,   137,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   168,   138
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      27,    64,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    38,    39,    40,    62,    47,   117,    83,   187,   194,
     243,   206,    77,   215,   175,    67,   176,     3,   201,     4,
      68,    64,   283,   191,   211,   189,    41,     5,   270,    71,
     190,     6,   151,    53,    54,    55,    56,    57,     7,    64,
      84,    68,     8,   181,    70,   139,    85,     9,   153,   154,
     293,   182,   294,    72,    52,    10,   148,   149,    11,   218,
     188,   214,   131,   189,   197,    60,    53,    54,    55,    56,
      57,   169,   192,    74,   317,   246,   152,    13,    58,   206,
     239,    14,    78,   150,   244,   145,    42,    43,    44,   189,
     155,    71,   156,   216,   263,    61,    59,   -14,    60,   147,
      42,    43,    44,   303,   142,    85,   244,   290,   303,   215,
     217,   291,   131,    42,    75,    44,   202,   205,   248,   203,
      76,   131,   189,   274,   189,    76,   238,   213,    61,    71,
     208,   140,   244,   118,   247,   143,   249,   219,   220,   221,
     222,   222,   222,   222,   222,   222,   222,   222,   222,   222,
     222,   222,   222,   222,   222,   287,   260,   295,   297,   177,
     178,   299,   227,   228,   229,   230,     3,    71,     4,    42,
      43,    44,    79,    43,    80,   -26,     5,   244,   173,   174,
       6,   271,   244,   189,   272,   253,   189,     7,   179,   180,
     -27,     8,   251,   144,   265,   316,     9,   285,   208,   286,
     318,   319,    49,    50,    10,   292,   157,    11,   158,   170,
     171,   172,   184,   159,   160,   161,   162,   163,   164,   165,
     166,   288,   306,   289,   189,   314,    13,   189,   264,   189,
      14,   307,   308,   183,   315,   269,   189,   223,   224,   275,
     186,   167,     3,   304,     4,   193,   185,   282,   195,   123,
     196,   302,     5,    87,   124,   311,     6,   125,   198,   301,
     131,   131,   203,     7,   225,   226,   254,     8,   204,   210,
     126,   215,     9,    52,   231,   232,   240,   245,   250,   255,
      10,   -34,   259,    11,   267,    53,    54,    55,    56,    57,
     273,   276,   277,   284,   298,   309,   310,   139,   131,   313,
      88,    12,    13,   131,   131,   312,    14,   127,    89,    90,
      37,   209,    51,   300,    24,    59,    86,    60,   146,   122,
     268,   266,   233,   235,   237,   234,     0,     0,     0,   236,
       0,     0,     0,     0,    91,     0,     0,     0,    92,    93,
     128,    50,   200,    94,     0,     0,     0,    61,     0,     0,
       3,     0,     4,    95,    96,    97,    98,   123,     0,     0,
       5,    87,   124,     0,     6,   125,     0,     0,     0,     0,
       0,     7,     0,     0,     0,     8,     0,     0,   126,     0,
       9,    52,     0,     0,     0,     0,     0,     0,    10,     0,
       0,    11,     0,    53,    54,    55,    56,    57,     0,     0,
       0,     0,     0,     0,     0,   252,     0,     0,    88,    12,
      13,     0,     0,     0,    14,   127,    89,    90,     0,     0,
       0,     0,     0,    59,     0,    60,     0,     0,     0,     3,
       0,     4,     0,     0,     0,     0,     0,     0,     0,     5,
      87,     0,    91,     6,     0,     0,    92,    93,   128,    50,
       7,    94,     0,     0,     8,    61,     0,     0,     0,     9,
       0,    95,    96,    97,    98,     0,     0,    10,     0,     3,
      11,     4,     0,     0,     0,     0,     0,     0,     0,     5,
      87,     0,     0,     6,     0,     0,     0,    88,     0,    13,
       7,     0,     0,    14,     8,    89,    90,     0,     0,     9,
       0,     0,     0,     0,     0,     0,     0,    10,     0,     0,
      11,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    91,     0,     0,     0,    92,    93,    88,   241,    13,
      94,     0,     0,    14,     0,    89,    90,     0,     0,     0,
      95,    96,    97,    98,     0,     0,     0,     0,     3,     0,
       4,     0,     0,     0,     0,     0,     0,     0,     5,    87,
       0,    91,     6,     0,     0,    92,    93,   128,     0,     7,
      94,     0,     0,     8,     0,     0,     0,     0,     9,     0,
      95,    96,    97,    98,     0,     0,    10,     0,     0,    11,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    88,     0,    13,     3,
       0,     4,    14,     0,    89,    90,     0,     0,     0,     5,
      87,     0,     0,     6,     0,     0,     0,     0,     0,     0,
       7,     0,     0,     0,     8,     0,     0,     0,     0,     9,
      91,     0,     0,     0,    92,    93,     0,    10,   305,    94,
      11,     0,     0,     0,     0,     0,     0,     0,     0,    95,
      96,    97,    98,     2,     0,     0,     0,    88,     0,    13,
       3,     0,     4,    14,     0,    89,    90,     0,     0,     0,
       5,     0,     0,     0,     6,     0,     0,     0,     0,     0,
       0,     7,     0,     0,     0,     8,     0,     0,     0,     0,
       9,    91,     0,     0,     0,    92,    93,     0,    10,     0,
      94,    11,     0,     0,     0,     0,     0,     3,     0,     4,
      95,    96,    97,    98,     0,     0,     0,     5,     0,    12,
      13,     6,     0,   278,    14,   279,     0,     0,     7,   280,
       0,     0,     8,     0,     0,     0,     3,     9,     4,     0,
       0,     0,     0,     0,     0,    10,     5,     0,    11,     0,
       6,    15,     0,     0,     0,     0,     0,     7,    16,     0,
       0,     8,     0,     0,     0,     0,     9,    13,     0,     0,
       0,    14,     0,     0,    10,     0,     0,    11,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    13,     0,     0,     0,
      14
};

static const yytype_int16 yycheck[] =
{
       1,    37,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    27,    19,    68,    64,    76,   123,
     193,   141,    58,   108,    78,    38,    80,     7,   132,     9,
     115,    67,   259,    36,   144,   111,   102,    17,   246,   111,
     116,    21,    94,    38,    39,    40,    41,    42,    28,    85,
     105,   115,    32,    77,   105,    50,   111,    37,    73,    74,
     268,    85,   270,   107,    26,    45,    89,    90,    48,   168,
     128,   109,    73,   111,   126,    70,    38,    39,    40,    41,
      42,   104,   118,   112,   311,   195,    99,    67,    50,   209,
     189,    71,   112,    94,   193,   105,    99,   100,   101,   111,
     115,   111,   117,   155,   116,   100,    68,   106,    70,   105,
      99,   100,   101,   286,   106,   111,   215,   107,   291,   108,
     156,   111,   123,    99,   100,   101,   105,   107,   105,   100,
     106,   132,   111,   105,   111,   106,   188,   150,   100,   111,
     141,    74,   241,   110,   196,    78,   198,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   187,   264,   213,   271,   272,    83,
      84,   105,   177,   178,   179,   180,     7,   111,     9,    99,
     100,   101,    99,   100,   101,   108,    17,   286,   118,   119,
      21,   109,   291,   111,   109,   208,   111,    28,   112,   113,
     108,    32,   206,   108,   240,   309,    37,   109,   209,   111,
     314,   315,   105,   106,    45,   267,    79,    48,    81,   122,
     123,   124,   126,    86,    87,    88,    89,    90,    91,    92,
      93,   109,   109,   111,   111,   109,    67,   111,   110,   111,
      71,   293,   294,   125,   109,   246,   111,   173,   174,   253,
      75,   114,     7,   289,     9,   114,   127,   258,   108,    14,
     108,   284,    17,    18,    19,   301,    21,    22,   108,   282,
     271,   272,   100,    28,   175,   176,   107,    32,   111,   111,
      35,   108,    37,    26,   181,   182,   108,    72,   104,   104,
      45,   109,   109,    48,   108,    38,    39,    40,    41,    42,
     111,   113,   111,   109,   104,    16,   113,    50,   309,   105,
      65,    66,    67,   314,   315,   109,    71,    72,    73,    74,
      12,   142,    23,   277,     1,    68,    67,    70,    85,    71,
     246,   241,   183,   185,   187,   184,    -1,    -1,    -1,   186,
      -1,    -1,    -1,    -1,    99,    -1,    -1,    -1,   103,   104,
     105,   106,   107,   108,    -1,    -1,    -1,   100,    -1,    -1,
       7,    -1,     9,   118,   119,   120,   121,    14,    -1,    -1,
      17,    18,    19,    -1,    21,    22,    -1,    -1,    -1,    -1,
      -1,    28,    -1,    -1,    -1,    32,    -1,    -1,    35,    -1,
      37,    26,    -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    48,    -1,    38,    39,    40,    41,    42,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    50,    -1,    -1,    65,    66,
      67,    -1,    -1,    -1,    71,    72,    73,    74,    -1,    -1,
      -1,    -1,    -1,    68,    -1,    70,    -1,    -1,    -1,     7,
      -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,
      18,    -1,    99,    21,    -1,    -1,   103,   104,   105,   106,
      28,   108,    -1,    -1,    32,   100,    -1,    -1,    -1,    37,
      -1,   118,   119,   120,   121,    -1,    -1,    45,    -1,     7,
      48,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,
      18,    -1,    -1,    21,    -1,    -1,    -1,    65,    -1,    67,
      28,    -1,    -1,    71,    32,    73,    74,    -1,    -1,    37,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    99,    -1,    -1,    -1,   103,   104,    65,   106,    67,
     108,    -1,    -1,    71,    -1,    73,    74,    -1,    -1,    -1,
     118,   119,   120,   121,    -1,    -1,    -1,    -1,     7,    -1,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,    18,
      -1,    99,    21,    -1,    -1,   103,   104,   105,    -1,    28,
     108,    -1,    -1,    32,    -1,    -1,    -1,    -1,    37,    -1,
     118,   119,   120,   121,    -1,    -1,    45,    -1,    -1,    48,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    67,     7,
      -1,     9,    71,    -1,    73,    74,    -1,    -1,    -1,    17,
      18,    -1,    -1,    21,    -1,    -1,    -1,    -1,    -1,    -1,
      28,    -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,    37,
      99,    -1,    -1,    -1,   103,   104,    -1,    45,   107,   108,
      48,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   118,
     119,   120,   121,     0,    -1,    -1,    -1,    65,    -1,    67,
       7,    -1,     9,    71,    -1,    73,    74,    -1,    -1,    -1,
      17,    -1,    -1,    -1,    21,    -1,    -1,    -1,    -1,    -1,
      -1,    28,    -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,
      37,    99,    -1,    -1,    -1,   103,   104,    -1,    45,    -1,
     108,    48,    -1,    -1,    -1,    -1,    -1,     7,    -1,     9,
     118,   119,   120,   121,    -1,    -1,    -1,    17,    -1,    66,
      67,    21,    -1,    23,    71,    25,    -1,    -1,    28,    29,
      -1,    -1,    32,    -1,    -1,    -1,     7,    37,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    45,    17,    -1,    48,    -1,
      21,    98,    -1,    -1,    -1,    -1,    -1,    28,   105,    -1,
      -1,    32,    -1,    -1,    -1,    -1,    37,    67,    -1,    -1,
      -1,    71,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    67,    -1,    -1,    -1,
      71
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   130,     0,     7,     9,    17,    21,    28,    32,    37,
      45,    48,    66,    67,    71,    98,   105,   131,   132,   133,
     134,   135,   139,   140,   154,   155,   158,   163,   163,   163,
     163,   163,   163,   163,   163,   163,   163,   133,   163,   163,
     163,   102,    99,   100,   101,   136,   159,   160,   161,   105,
     106,   141,    26,    38,    39,    40,    41,    42,    50,    68,
      70,   100,   152,   153,   136,   156,   157,   152,   115,   162,
     105,   111,   107,   142,   112,   100,   106,   136,   112,    99,
     101,   143,   160,   162,   105,   111,   156,    18,    65,    73,
      74,    99,   103,   104,   108,   118,   119,   120,   121,   163,
     167,   175,   176,   177,   178,   179,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   192,   110,   144,
     145,   146,   161,    14,    19,    22,    35,    72,   105,   141,
     154,   163,   168,   169,   170,   171,   173,   174,   192,    50,
     153,   137,   106,   153,   108,   105,   157,   105,   177,   177,
     163,   192,   152,    73,    74,   115,   117,    79,    81,    86,
      87,    88,    89,    90,    91,    92,    93,   114,   191,   177,
     122,   123,   124,   118,   119,    78,    80,    83,    84,   112,
     113,    77,    85,   125,   126,   127,    75,    76,   128,   111,
     116,    36,   136,   114,   169,   108,   108,   192,   108,   152,
     107,   169,   105,   100,   111,   107,   135,   138,   163,   137,
     111,   142,   147,   152,   109,   108,   192,   136,   190,   177,
     177,   177,   177,   179,   179,   180,   180,   181,   181,   181,
     181,   182,   182,   183,   184,   185,   186,   187,   192,   190,
     108,   106,   164,   165,   190,    72,   142,   192,   105,   192,
     104,   160,    50,   152,   107,   104,   148,   149,   150,   109,
     162,   165,   166,   116,   110,   136,   166,   108,   158,   163,
     174,   109,   109,   111,   105,   160,   113,   111,    23,    25,
      29,   151,   163,   144,   109,   109,   111,   190,   109,   111,
     107,   111,   192,   174,   174,   169,   172,   169,   104,   105,
     149,   152,   177,   165,   136,   107,   109,   192,   192,    16,
     113,   136,   109,   105,   109,   109,   169,   144,   169,   169
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   129,   130,   130,   130,   130,   130,   131,   132,   133,
     133,   134,   135,   136,   136,   136,   137,   137,   138,   138,
     139,   139,   140,   141,   141,   142,   143,   143,   144,   144,
     144,   145,   146,   146,   147,   147,   148,   148,   149,   150,
     150,   151,   151,   151,   152,   152,   152,   153,   153,   153,
     153,   153,   153,   153,   153,   154,   154,   154,   155,   155,
     156,   156,   157,   158,   159,   159,   160,   160,   161,   161,
     162,   162,   163,   163,   163,   163,   163,   163,   163,   163,
     163,   163,   163,   163,   164,   164,   164,   165,   166,   166,
     167,   167,   168,   168,   169,   169,   169,   169,   169,   169,
     170,   171,   172,   172,   173,   173,   173,   173,   174,   174,
     175,   175,   175,   175,   175,   176,   176,   176,   176,   176,
     176,   177,   177,   177,   177,   177,   178,   178,   178,   178,
     179,   179,   179,   179,   180,   180,   180,   181,   181,   181,
     182,   182,   182,   182,   182,   183,   183,   183,   184,   184,
     185,   185,   186,   186,   187,   187,   188,   188,   189,   189,
     190,   190,   191,   191,   191,   191,   191,   191,   191,   191,
     191,   191,   191,   192,   192
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     2,     2,     2,     2,     3,     1,
       1,     6,     5,     1,     1,     1,     0,     2,     4,     3,
       2,     2,     7,     2,     4,     0,     1,     1,     0,     1,
       1,     2,     5,     7,     1,     2,     1,     3,     5,     0,
       2,     1,     1,     1,     1,     6,     8,     1,     1,     1,
       1,     1,     1,     1,     2,     1,     1,     1,     5,     4,
       1,     3,     2,     4,     0,     1,     1,     3,     3,     5,
       0,     3,     0,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     1,     3,     4,     1,     1,     3,
       1,     1,     1,     2,     1,     1,     1,     1,     1,     1,
       3,     5,     1,     3,     5,     7,     8,     8,     1,     2,
       1,     1,     1,     1,     3,     1,     2,     2,     3,     4,
       5,     1,     2,     2,     2,     6,     1,     1,     1,     1,
       1,     3,     3,     3,     1,     3,     3,     1,     3,     3,
       1,     3,     3,     3,     3,     1,     3,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     5,
       1,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
 }

#  define YY_LOCATION_PRINT(File, Loc)          \
  yy_location_print_ (File, &(Loc))

# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, Location); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  YYUSE (yylocationp);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  YY_LOCATION_PRINT (yyo, *yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yytype, yyvaluep, yylocationp);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule)
{
  unsigned long yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
                       , &(yylsp[(yyi + 1) - (yynrhs)])                       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return (YYSIZE_T) (yystpcpy (yyres, yystr) - yyres);
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Location data for the lookahead symbol.  */
YYLTYPE yylloc
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.
       'yyls': related to locations.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[3];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yylsp = yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yynewstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  *yyssp = (yytype_int16) yystate;

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = (YYSIZE_T) (yyssp - yyss + 1);

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yyls1, yysize * sizeof (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2:
#line 1074 "hlsl.y"
    {
                            }
#line 2687 "hlsl.tab.c"
    break;

  case 3:
#line 1077 "hlsl.y"
    {
                                const struct hlsl_ir_function_decl *decl;

                                decl = get_overloaded_func(&hlsl_ctx.functions, (yyvsp[0].function).name, (yyvsp[0].function).decl->parameters, TRUE);
                                if (decl && !decl->func->intrinsic)
                                {
                                    if (decl->body && (yyvsp[0].function).decl->body)
                                    {
                                        hlsl_report_message((yyvsp[0].function).decl->loc.file, (yyvsp[0].function).decl->loc.line,
                                                (yyvsp[0].function).decl->loc.col, HLSL_LEVEL_ERROR,
                                                "redefinition of function %s", debugstr_a((yyvsp[0].function).name));
                                        YYABORT;
                                    }
                                    else if (!compare_hlsl_types(decl->return_type, (yyvsp[0].function).decl->return_type))
                                    {
                                        hlsl_report_message((yyvsp[0].function).decl->loc.file, (yyvsp[0].function).decl->loc.line,
                                                (yyvsp[0].function).decl->loc.col, HLSL_LEVEL_ERROR,
                                                "redefining function %s with a different return type",
                                                debugstr_a((yyvsp[0].function).name));
                                        hlsl_report_message(decl->loc.file, decl->loc.line, decl->loc.col, HLSL_LEVEL_NOTE,
                                                "%s previously declared here",
                                                debugstr_a((yyvsp[0].function).name));
                                        YYABORT;
                                    }
                                }

                                if ((yyvsp[0].function).decl->return_type->base_type == HLSL_TYPE_VOID && (yyvsp[0].function).decl->semantic)
                                {
                                    hlsl_report_message((yyvsp[0].function).decl->loc.file, (yyvsp[0].function).decl->loc.line,
                                            (yyvsp[0].function).decl->loc.col, HLSL_LEVEL_ERROR,
                                            "void function with a semantic");
                                }

                                TRACE("Adding function '%s' to the function list.\n", (yyvsp[0].function).name);
                                add_function_decl(&hlsl_ctx.functions, (yyvsp[0].function).name, (yyvsp[0].function).decl, FALSE);
                            }
#line 2728 "hlsl.tab.c"
    break;

  case 4:
#line 1114 "hlsl.y"
    {
                                TRACE("Declaration statement parsed.\n");
                            }
#line 2736 "hlsl.tab.c"
    break;

  case 5:
#line 1118 "hlsl.y"
    {
                            }
#line 2743 "hlsl.tab.c"
    break;

  case 6:
#line 1121 "hlsl.y"
    {
                                TRACE("Skipping stray semicolon.\n");
                            }
#line 2751 "hlsl.tab.c"
    break;

  case 7:
#line 1126 "hlsl.y"
    {
                                const char **new_array = NULL;

                                TRACE("Updating line information to file %s, line %u\n", debugstr_a((yyvsp[0].name)), (yyvsp[-1].intval));
                                hlsl_ctx.line_no = (yyvsp[-1].intval);
                                if (strcmp((yyvsp[0].name), hlsl_ctx.source_file))
                                    new_array = d3dcompiler_realloc((void*)hlsl_ctx.source_files,
                                            sizeof(*hlsl_ctx.source_files) * (hlsl_ctx.source_files_count + 1));

                                if (new_array)
                                {
                                    hlsl_ctx.source_files = new_array;
                                    hlsl_ctx.source_files[hlsl_ctx.source_files_count++] = (yyvsp[0].name);
                                    hlsl_ctx.source_file = (yyvsp[0].name);
                                }
                                else
                                {
                                    d3dcompiler_free((yyvsp[0].name));
                                }
                            }
#line 2776 "hlsl.tab.c"
    break;

  case 8:
#line 1148 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[0]));
                                if (!(yyvsp[-1].list))
                                {
                                    if (!(yyvsp[-2].type)->name)
                                    {
                                        hlsl_report_message(loc.file, loc.line, loc.col,
                                                HLSL_LEVEL_ERROR, "anonymous struct declaration with no variables");
                                    }
                                    check_type_modifiers((yyvsp[-2].type)->modifiers, &loc);
                                }
                                (yyval.list) = declare_vars((yyvsp[-2].type), 0, (yyvsp[-1].list));
                            }
#line 2796 "hlsl.tab.c"
    break;

  case 11:
#line 1168 "hlsl.y"
    {
                                BOOL ret;
                                struct source_location loc;

                                TRACE("Structure %s declaration.\n", debugstr_a((yyvsp[-3].name)));
                                set_location(&loc, &(yylsp[-5]));
                                check_invalid_matrix_modifiers((yyvsp[-5].modifiers), &loc);
                                (yyval.type) = new_struct_type((yyvsp[-3].name), (yyvsp[-5].modifiers), (yyvsp[-1].list));

                                if (get_variable(hlsl_ctx.cur_scope, (yyvsp[-3].name)))
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, (yylsp[-3]).first_line, (yylsp[-3]).first_column,
                                            HLSL_LEVEL_ERROR, "redefinition of '%s'", (yyvsp[-3].name));
                                    YYABORT;
                                }

                                ret = add_type_to_scope(hlsl_ctx.cur_scope, (yyval.type));
                                if (!ret)
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, (yylsp[-3]).first_line, (yylsp[-3]).first_column,
                                            HLSL_LEVEL_ERROR, "redefinition of struct '%s'", (yyvsp[-3].name));
                                    YYABORT;
                                }
                            }
#line 2825 "hlsl.tab.c"
    break;

  case 12:
#line 1194 "hlsl.y"
    {
                                struct source_location loc;

                                TRACE("Anonymous structure declaration.\n");
                                set_location(&loc, &(yylsp[-4]));
                                check_invalid_matrix_modifiers((yyvsp[-4].modifiers), &loc);
                                (yyval.type) = new_struct_type(NULL, (yyvsp[-4].modifiers), (yyvsp[-1].list));
                            }
#line 2838 "hlsl.tab.c"
    break;

  case 16:
#line 1208 "hlsl.y"
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                            }
#line 2847 "hlsl.tab.c"
    break;

  case 17:
#line 1213 "hlsl.y"
    {
                                BOOL ret;
                                struct hlsl_struct_field *field, *next;

                                (yyval.list) = (yyvsp[-1].list);
                                LIST_FOR_EACH_ENTRY_SAFE(field, next, (yyvsp[0].list), struct hlsl_struct_field, entry)
                                {
                                    ret = add_struct_field((yyval.list), field);
                                    if (ret == FALSE)
                                    {
                                        hlsl_report_message(hlsl_ctx.source_file, (yylsp[0]).first_line, (yylsp[0]).first_column,
                                                HLSL_LEVEL_ERROR, "redefinition of '%s'", field->name);
                                        d3dcompiler_free(field);
                                    }
                                }
                                d3dcompiler_free((yyvsp[0].list));
                            }
#line 2869 "hlsl.tab.c"
    break;

  case 18:
#line 1232 "hlsl.y"
    {
                                (yyval.list) = gen_struct_fields((yyvsp[-2].type), (yyvsp[-3].modifiers), (yyvsp[-1].list));
                            }
#line 2877 "hlsl.tab.c"
    break;

  case 19:
#line 1236 "hlsl.y"
    {
                                (yyval.list) = gen_struct_fields((yyvsp[-2].type), 0, (yyvsp[-1].list));
                            }
#line 2885 "hlsl.tab.c"
    break;

  case 20:
#line 1241 "hlsl.y"
    {
                                TRACE("Function %s parsed.\n", (yyvsp[-1].function).name);
                                (yyval.function) = (yyvsp[-1].function);
                                (yyval.function).decl->body = (yyvsp[0].list);
                                pop_scope(&hlsl_ctx);
                            }
#line 2896 "hlsl.tab.c"
    break;

  case 21:
#line 1248 "hlsl.y"
    {
                                TRACE("Function prototype for %s.\n", (yyvsp[-1].function).name);
                                (yyval.function) = (yyvsp[-1].function);
                                pop_scope(&hlsl_ctx);
                            }
#line 2906 "hlsl.tab.c"
    break;

  case 22:
#line 1255 "hlsl.y"
    {
                                if (get_variable(hlsl_ctx.globals, (yyvsp[-4].name)))
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, (yylsp[-4]).first_line, (yylsp[-4]).first_column,
                                            HLSL_LEVEL_ERROR, "redefinition of '%s'\n", (yyvsp[-4].name));
                                    YYABORT;
                                }
                                if ((yyvsp[-5].type)->base_type == HLSL_TYPE_VOID && (yyvsp[0].colon_attribute).semantic)
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, (yylsp[0]).first_line, (yylsp[0]).first_column,
                                            HLSL_LEVEL_ERROR, "void function with a semantic");
                                }

                                if ((yyvsp[0].colon_attribute).reg_reservation)
                                {
                                    FIXME("Unexpected register reservation for a function.\n");
                                    d3dcompiler_free((yyvsp[0].colon_attribute).reg_reservation);
                                }
                                (yyval.function).decl = new_func_decl((yyvsp[-5].type), (yyvsp[-2].list));
                                if (!(yyval.function).decl)
                                {
                                    ERR("Out of memory.\n");
                                    YYABORT;
                                }
                                (yyval.function).name = (yyvsp[-4].name);
                                (yyval.function).decl->semantic = (yyvsp[0].colon_attribute).semantic;
                                set_location(&(yyval.function).decl->loc, &(yylsp[-4]));
                            }
#line 2939 "hlsl.tab.c"
    break;

  case 23:
#line 1285 "hlsl.y"
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                            }
#line 2948 "hlsl.tab.c"
    break;

  case 24:
#line 1290 "hlsl.y"
    {
                                pop_scope(&hlsl_ctx);
                                (yyval.list) = (yyvsp[-1].list);
                            }
#line 2957 "hlsl.tab.c"
    break;

  case 25:
#line 1296 "hlsl.y"
    {
                                push_scope(&hlsl_ctx);
                            }
#line 2965 "hlsl.tab.c"
    break;

  case 28:
#line 1304 "hlsl.y"
    {
                                (yyval.colon_attribute).semantic = NULL;
                                (yyval.colon_attribute).reg_reservation = NULL;
                            }
#line 2974 "hlsl.tab.c"
    break;

  case 29:
#line 1309 "hlsl.y"
    {
                                (yyval.colon_attribute).semantic = (yyvsp[0].name);
                                (yyval.colon_attribute).reg_reservation = NULL;
                            }
#line 2983 "hlsl.tab.c"
    break;

  case 30:
#line 1314 "hlsl.y"
    {
                                (yyval.colon_attribute).semantic = NULL;
                                (yyval.colon_attribute).reg_reservation = (yyvsp[0].reg_reservation);
                            }
#line 2992 "hlsl.tab.c"
    break;

  case 31:
#line 1320 "hlsl.y"
    {
                                (yyval.name) = (yyvsp[0].name);
                            }
#line 3000 "hlsl.tab.c"
    break;

  case 32:
#line 1326 "hlsl.y"
    {
                                (yyval.reg_reservation) = parse_reg_reservation((yyvsp[-1].name));
                                d3dcompiler_free((yyvsp[-1].name));
                            }
#line 3009 "hlsl.tab.c"
    break;

  case 33:
#line 1331 "hlsl.y"
    {
                                FIXME("Ignoring shader target %s in a register reservation.\n", debugstr_a((yyvsp[-3].name)));
                                d3dcompiler_free((yyvsp[-3].name));

                                (yyval.reg_reservation) = parse_reg_reservation((yyvsp[-1].name));
                                d3dcompiler_free((yyvsp[-1].name));
                            }
#line 3021 "hlsl.tab.c"
    break;

  case 34:
#line 1340 "hlsl.y"
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                            }
#line 3030 "hlsl.tab.c"
    break;

  case 35:
#line 1345 "hlsl.y"
    {
                                (yyval.list) = (yyvsp[0].list);
                            }
#line 3038 "hlsl.tab.c"
    break;

  case 36:
#line 1350 "hlsl.y"
    {
                                struct source_location loc;

                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                set_location(&loc, &(yylsp[0]));
                                if (!add_func_parameter((yyval.list), &(yyvsp[0].parameter), &loc))
                                {
                                    ERR("Error adding function parameter %s.\n", (yyvsp[0].parameter).name);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    YYABORT;
                                }
                            }
#line 3056 "hlsl.tab.c"
    break;

  case 37:
#line 1364 "hlsl.y"
    {
                                struct source_location loc;

                                (yyval.list) = (yyvsp[-2].list);
                                set_location(&loc, &(yylsp[0]));
                                if (!add_func_parameter((yyval.list), &(yyvsp[0].parameter), &loc))
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "duplicate parameter %s", (yyvsp[0].parameter).name);
                                    YYABORT;
                                }
                            }
#line 3073 "hlsl.tab.c"
    break;

  case 38:
#line 1378 "hlsl.y"
    {
                                (yyval.parameter).modifiers = (yyvsp[-4].modifiers) ? (yyvsp[-4].modifiers) : HLSL_MODIFIER_IN;
                                (yyval.parameter).modifiers |= (yyvsp[-3].modifiers);
                                (yyval.parameter).type = (yyvsp[-2].type);
                                (yyval.parameter).name = (yyvsp[-1].name);
                                (yyval.parameter).semantic = (yyvsp[0].colon_attribute).semantic;
                                (yyval.parameter).reg_reservation = (yyvsp[0].colon_attribute).reg_reservation;
                            }
#line 3086 "hlsl.tab.c"
    break;

  case 39:
#line 1388 "hlsl.y"
    {
                                (yyval.modifiers) = 0;
                            }
#line 3094 "hlsl.tab.c"
    break;

  case 40:
#line 1392 "hlsl.y"
    {
                                if ((yyvsp[-1].modifiers) & (yyvsp[0].modifiers))
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, (yylsp[0]).first_line, (yylsp[0]).first_column,
                                            HLSL_LEVEL_ERROR, "duplicate input-output modifiers");
                                    YYABORT;
                                }
                                (yyval.modifiers) = (yyvsp[-1].modifiers) | (yyvsp[0].modifiers);
                            }
#line 3108 "hlsl.tab.c"
    break;

  case 41:
#line 1403 "hlsl.y"
    {
                                (yyval.modifiers) = HLSL_MODIFIER_IN;
                            }
#line 3116 "hlsl.tab.c"
    break;

  case 42:
#line 1407 "hlsl.y"
    {
                                (yyval.modifiers) = HLSL_MODIFIER_OUT;
                            }
#line 3124 "hlsl.tab.c"
    break;

  case 43:
#line 1411 "hlsl.y"
    {
                                (yyval.modifiers) = HLSL_MODIFIER_IN | HLSL_MODIFIER_OUT;
                            }
#line 3132 "hlsl.tab.c"
    break;

  case 44:
#line 1416 "hlsl.y"
    {
                                (yyval.type) = (yyvsp[0].type);
                            }
#line 3140 "hlsl.tab.c"
    break;

  case 45:
#line 1420 "hlsl.y"
    {
                                if ((yyvsp[-3].type)->type != HLSL_CLASS_SCALAR)
                                {
                                    hlsl_message("Line %u: vectors of non-scalar types are not allowed.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    YYABORT;
                                }
                                if ((yyvsp[-1].intval) < 1 || (yyvsp[-1].intval) > 4)
                                {
                                    hlsl_message("Line %u: vector size must be between 1 and 4.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    YYABORT;
                                }

                                (yyval.type) = new_hlsl_type(NULL, HLSL_CLASS_VECTOR, (yyvsp[-3].type)->base_type, (yyvsp[-1].intval), 1);
                            }
#line 3163 "hlsl.tab.c"
    break;

  case 46:
#line 1439 "hlsl.y"
    {
                                if ((yyvsp[-5].type)->type != HLSL_CLASS_SCALAR)
                                {
                                    hlsl_message("Line %u: matrices of non-scalar types are not allowed.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    YYABORT;
                                }
                                if ((yyvsp[-3].intval) < 1 || (yyvsp[-3].intval) > 4 || (yyvsp[-1].intval) < 1 || (yyvsp[-1].intval) > 4)
                                {
                                    hlsl_message("Line %u: matrix dimensions must be between 1 and 4.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    YYABORT;
                                }

                                (yyval.type) = new_hlsl_type(NULL, HLSL_CLASS_MATRIX, (yyvsp[-5].type)->base_type, (yyvsp[-3].intval), (yyvsp[-1].intval));
                            }
#line 3186 "hlsl.tab.c"
    break;

  case 47:
#line 1459 "hlsl.y"
    {
                                (yyval.type) = new_hlsl_type(d3dcompiler_strdup("void"), HLSL_CLASS_OBJECT, HLSL_TYPE_VOID, 1, 1);
                            }
#line 3194 "hlsl.tab.c"
    break;

  case 48:
#line 1463 "hlsl.y"
    {
                                (yyval.type) = new_hlsl_type(d3dcompiler_strdup("sampler"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                (yyval.type)->sampler_dim = HLSL_SAMPLER_DIM_GENERIC;
                            }
#line 3203 "hlsl.tab.c"
    break;

  case 49:
#line 1468 "hlsl.y"
    {
                                (yyval.type) = new_hlsl_type(d3dcompiler_strdup("sampler1D"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                (yyval.type)->sampler_dim = HLSL_SAMPLER_DIM_1D;
                            }
#line 3212 "hlsl.tab.c"
    break;

  case 50:
#line 1473 "hlsl.y"
    {
                                (yyval.type) = new_hlsl_type(d3dcompiler_strdup("sampler2D"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                (yyval.type)->sampler_dim = HLSL_SAMPLER_DIM_2D;
                            }
#line 3221 "hlsl.tab.c"
    break;

  case 51:
#line 1478 "hlsl.y"
    {
                                (yyval.type) = new_hlsl_type(d3dcompiler_strdup("sampler3D"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                (yyval.type)->sampler_dim = HLSL_SAMPLER_DIM_3D;
                            }
#line 3230 "hlsl.tab.c"
    break;

  case 52:
#line 1483 "hlsl.y"
    {
                                (yyval.type) = new_hlsl_type(d3dcompiler_strdup("samplerCUBE"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                (yyval.type)->sampler_dim = HLSL_SAMPLER_DIM_CUBE;
                            }
#line 3239 "hlsl.tab.c"
    break;

  case 53:
#line 1488 "hlsl.y"
    {
                                struct hlsl_type *type;

                                type = get_type(hlsl_ctx.cur_scope, (yyvsp[0].name), TRUE);
                                (yyval.type) = type;
                                d3dcompiler_free((yyvsp[0].name));
                            }
#line 3251 "hlsl.tab.c"
    break;

  case 54:
#line 1496 "hlsl.y"
    {
                                struct hlsl_type *type;

                                type = get_type(hlsl_ctx.cur_scope, (yyvsp[0].name), TRUE);
                                if (type->type != HLSL_CLASS_STRUCT)
                                {
                                    hlsl_message("Line %u: redefining %s as a structure.\n",
                                            hlsl_ctx.line_no, (yyvsp[0].name));
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                }
                                else
                                {
                                    (yyval.type) = type;
                                }
                                d3dcompiler_free((yyvsp[0].name));
                            }
#line 3272 "hlsl.tab.c"
    break;

  case 57:
#line 1516 "hlsl.y"
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                if (!(yyval.list))
                                {
                                    ERR("Out of memory\n");
                                    YYABORT;
                                }
                                list_init((yyval.list));
                            }
#line 3286 "hlsl.tab.c"
    break;

  case 58:
#line 1527 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-4]));
                                if (!add_typedef((yyvsp[-3].modifiers), (yyvsp[-2].type), (yyvsp[-1].list), &loc))
                                    YYABORT;
                            }
#line 3298 "hlsl.tab.c"
    break;

  case 59:
#line 1535 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-3]));
                                if (!add_typedef(0, (yyvsp[-2].type), (yyvsp[-1].list), &loc))
                                    YYABORT;
                            }
#line 3310 "hlsl.tab.c"
    break;

  case 60:
#line 1544 "hlsl.y"
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                list_add_head((yyval.list), &(yyvsp[0].variable_def)->entry);
                            }
#line 3320 "hlsl.tab.c"
    break;

  case 61:
#line 1550 "hlsl.y"
    {
                                (yyval.list) = (yyvsp[-2].list);
                                list_add_tail((yyval.list), &(yyvsp[0].variable_def)->entry);
                            }
#line 3329 "hlsl.tab.c"
    break;

  case 62:
#line 1556 "hlsl.y"
    {
                                (yyval.variable_def) = d3dcompiler_alloc(sizeof(*(yyval.variable_def)));
                                set_location(&(yyval.variable_def)->loc, &(yylsp[-1]));
                                (yyval.variable_def)->name = (yyvsp[-1].name);
                                (yyval.variable_def)->array_size = (yyvsp[0].intval);
                            }
#line 3340 "hlsl.tab.c"
    break;

  case 63:
#line 1564 "hlsl.y"
    {
                                (yyval.list) = declare_vars((yyvsp[-2].type), (yyvsp[-3].modifiers), (yyvsp[-1].list));
                            }
#line 3348 "hlsl.tab.c"
    break;

  case 64:
#line 1569 "hlsl.y"
    {
                                (yyval.list) = NULL;
                            }
#line 3356 "hlsl.tab.c"
    break;

  case 65:
#line 1573 "hlsl.y"
    {
                                (yyval.list) = (yyvsp[0].list);
                            }
#line 3364 "hlsl.tab.c"
    break;

  case 66:
#line 1578 "hlsl.y"
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                list_add_head((yyval.list), &(yyvsp[0].variable_def)->entry);
                            }
#line 3374 "hlsl.tab.c"
    break;

  case 67:
#line 1584 "hlsl.y"
    {
                                (yyval.list) = (yyvsp[-2].list);
                                list_add_tail((yyval.list), &(yyvsp[0].variable_def)->entry);
                            }
#line 3383 "hlsl.tab.c"
    break;

  case 68:
#line 1590 "hlsl.y"
    {
                                (yyval.variable_def) = d3dcompiler_alloc(sizeof(*(yyval.variable_def)));
                                set_location(&(yyval.variable_def)->loc, &(yylsp[-2]));
                                (yyval.variable_def)->name = (yyvsp[-2].name);
                                (yyval.variable_def)->array_size = (yyvsp[-1].intval);
                                (yyval.variable_def)->semantic = (yyvsp[0].colon_attribute).semantic;
                                (yyval.variable_def)->reg_reservation = (yyvsp[0].colon_attribute).reg_reservation;
                            }
#line 3396 "hlsl.tab.c"
    break;

  case 69:
#line 1599 "hlsl.y"
    {
                                TRACE("Declaration with initializer.\n");
                                (yyval.variable_def) = d3dcompiler_alloc(sizeof(*(yyval.variable_def)));
                                set_location(&(yyval.variable_def)->loc, &(yylsp[-4]));
                                (yyval.variable_def)->name = (yyvsp[-4].name);
                                (yyval.variable_def)->array_size = (yyvsp[-3].intval);
                                (yyval.variable_def)->semantic = (yyvsp[-2].colon_attribute).semantic;
                                (yyval.variable_def)->reg_reservation = (yyvsp[-2].colon_attribute).reg_reservation;
                                (yyval.variable_def)->initializer = (yyvsp[0].initializer);
                            }
#line 3411 "hlsl.tab.c"
    break;

  case 70:
#line 1611 "hlsl.y"
    {
                                (yyval.intval) = 0;
                            }
#line 3419 "hlsl.tab.c"
    break;

  case 71:
#line 1615 "hlsl.y"
    {
                                FIXME("Array.\n");
                                (yyval.intval) = 0;
                                free_instr((yyvsp[-1].instr));
                            }
#line 3429 "hlsl.tab.c"
    break;

  case 72:
#line 1622 "hlsl.y"
    {
                                (yyval.modifiers) = 0;
                            }
#line 3437 "hlsl.tab.c"
    break;

  case 73:
#line 1626 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_STORAGE_EXTERN, &(yylsp[-1]));
                            }
#line 3445 "hlsl.tab.c"
    break;

  case 74:
#line 1630 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_STORAGE_NOINTERPOLATION, &(yylsp[-1]));
                            }
#line 3453 "hlsl.tab.c"
    break;

  case 75:
#line 1634 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_MODIFIER_PRECISE, &(yylsp[-1]));
                            }
#line 3461 "hlsl.tab.c"
    break;

  case 76:
#line 1638 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_STORAGE_SHARED, &(yylsp[-1]));
                            }
#line 3469 "hlsl.tab.c"
    break;

  case 77:
#line 1642 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_STORAGE_GROUPSHARED, &(yylsp[-1]));
                            }
#line 3477 "hlsl.tab.c"
    break;

  case 78:
#line 1646 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_STORAGE_STATIC, &(yylsp[-1]));
                            }
#line 3485 "hlsl.tab.c"
    break;

  case 79:
#line 1650 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_STORAGE_UNIFORM, &(yylsp[-1]));
                            }
#line 3493 "hlsl.tab.c"
    break;

  case 80:
#line 1654 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_STORAGE_VOLATILE, &(yylsp[-1]));
                            }
#line 3501 "hlsl.tab.c"
    break;

  case 81:
#line 1658 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_MODIFIER_CONST, &(yylsp[-1]));
                            }
#line 3509 "hlsl.tab.c"
    break;

  case 82:
#line 1662 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_MODIFIER_ROW_MAJOR, &(yylsp[-1]));
                            }
#line 3517 "hlsl.tab.c"
    break;

  case 83:
#line 1666 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_MODIFIER_COLUMN_MAJOR, &(yylsp[-1]));
                            }
#line 3525 "hlsl.tab.c"
    break;

  case 84:
#line 1671 "hlsl.y"
    {
                                (yyval.initializer).args_count = 1;
                                if (!((yyval.initializer).args = d3dcompiler_alloc(sizeof(*(yyval.initializer).args))))
                                    YYABORT;
                                (yyval.initializer).args[0] = (yyvsp[0].instr);
                            }
#line 3536 "hlsl.tab.c"
    break;

  case 85:
#line 1678 "hlsl.y"
    {
                                (yyval.initializer) = (yyvsp[-1].initializer);
                            }
#line 3544 "hlsl.tab.c"
    break;

  case 86:
#line 1682 "hlsl.y"
    {
                                (yyval.initializer) = (yyvsp[-2].initializer);
                            }
#line 3552 "hlsl.tab.c"
    break;

  case 87:
#line 1687 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 3560 "hlsl.tab.c"
    break;

  case 88:
#line 1692 "hlsl.y"
    {
                                (yyval.initializer).args_count = 1;
                                if (!((yyval.initializer).args = d3dcompiler_alloc(sizeof(*(yyval.initializer).args))))
                                    YYABORT;
                                (yyval.initializer).args[0] = (yyvsp[0].instr);
                            }
#line 3571 "hlsl.tab.c"
    break;

  case 89:
#line 1699 "hlsl.y"
    {
                                (yyval.initializer) = (yyvsp[-2].initializer);
                                if (!((yyval.initializer).args = d3dcompiler_realloc((yyval.initializer).args, ((yyval.initializer).args_count + 1) * sizeof(*(yyval.initializer).args))))
                                    YYABORT;
                                (yyval.initializer).args[(yyval.initializer).args_count++] = (yyvsp[0].instr);
                            }
#line 3582 "hlsl.tab.c"
    break;

  case 90:
#line 1707 "hlsl.y"
    {
                                (yyval.boolval) = TRUE;
                            }
#line 3590 "hlsl.tab.c"
    break;

  case 91:
#line 1711 "hlsl.y"
    {
                                (yyval.boolval) = FALSE;
                            }
#line 3598 "hlsl.tab.c"
    break;

  case 92:
#line 1716 "hlsl.y"
    {
                                (yyval.list) = (yyvsp[0].list);
                            }
#line 3606 "hlsl.tab.c"
    break;

  case 93:
#line 1720 "hlsl.y"
    {
                                (yyval.list) = (yyvsp[-1].list);
                                list_move_tail((yyval.list), (yyvsp[0].list));
                                d3dcompiler_free((yyvsp[0].list));
                            }
#line 3616 "hlsl.tab.c"
    break;

  case 100:
#line 1735 "hlsl.y"
    {
                                struct hlsl_ir_jump *jump = d3dcompiler_alloc(sizeof(*jump));
                                if (!jump)
                                {
                                    ERR("Out of memory\n");
                                    YYABORT;
                                }
                                jump->node.type = HLSL_IR_JUMP;
                                set_location(&jump->node.loc, &(yylsp[-2]));
                                jump->type = HLSL_IR_JUMP_RETURN;
                                jump->node.data_type = (yyvsp[-1].instr)->data_type;
                                jump->return_value = (yyvsp[-1].instr);

                                FIXME("Check for valued return on void function.\n");
                                FIXME("Implicit conversion to the return type if needed, "
				        "error out if conversion not possible.\n");

                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                list_add_tail((yyval.list), &jump->node.entry);
                            }
#line 3642 "hlsl.tab.c"
    break;

  case 101:
#line 1758 "hlsl.y"
    {
                                struct hlsl_ir_if *instr = d3dcompiler_alloc(sizeof(*instr));
                                if (!instr)
                                {
                                    ERR("Out of memory\n");
                                    YYABORT;
                                }
                                instr->node.type = HLSL_IR_IF;
                                set_location(&instr->node.loc, &(yylsp[-4]));
                                instr->condition = (yyvsp[-2].instr);
                                instr->then_instrs = (yyvsp[0].if_body).then_instrs;
                                instr->else_instrs = (yyvsp[0].if_body).else_instrs;
                                if ((yyvsp[-2].instr)->data_type->dimx > 1 || (yyvsp[-2].instr)->data_type->dimy > 1)
                                {
                                    hlsl_report_message(instr->node.loc.file, instr->node.loc.line,
                                            instr->node.loc.col, HLSL_LEVEL_ERROR,
                                            "if condition requires a scalar");
                                }
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                list_add_head((yyval.list), &instr->node.entry);
                            }
#line 3669 "hlsl.tab.c"
    break;

  case 102:
#line 1782 "hlsl.y"
    {
                                (yyval.if_body).then_instrs = (yyvsp[0].list);
                                (yyval.if_body).else_instrs = NULL;
                            }
#line 3678 "hlsl.tab.c"
    break;

  case 103:
#line 1787 "hlsl.y"
    {
                                (yyval.if_body).then_instrs = (yyvsp[-2].list);
                                (yyval.if_body).else_instrs = (yyvsp[0].list);
                            }
#line 3687 "hlsl.tab.c"
    break;

  case 104:
#line 1793 "hlsl.y"
    {
                                struct source_location loc;
                                struct list *cond = d3dcompiler_alloc(sizeof(*cond));

                                if (!cond)
                                {
                                    ERR("Out of memory.\n");
                                    YYABORT;
                                }
                                list_init(cond);
                                list_add_head(cond, &(yyvsp[-2].instr)->entry);
                                set_location(&loc, &(yylsp[-4]));
                                (yyval.list) = create_loop(LOOP_WHILE, NULL, cond, NULL, (yyvsp[0].list), &loc);
                            }
#line 3706 "hlsl.tab.c"
    break;

  case 105:
#line 1808 "hlsl.y"
    {
                                struct source_location loc;
                                struct list *cond = d3dcompiler_alloc(sizeof(*cond));

                                if (!cond)
                                {
                                    ERR("Out of memory.\n");
                                    YYABORT;
                                }
                                list_init(cond);
                                list_add_head(cond, &(yyvsp[-2].instr)->entry);
                                set_location(&loc, &(yylsp[-6]));
                                (yyval.list) = create_loop(LOOP_DO_WHILE, NULL, cond, NULL, (yyvsp[-5].list), &loc);
                            }
#line 3725 "hlsl.tab.c"
    break;

  case 106:
#line 1823 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-7]));
                                (yyval.list) = create_loop(LOOP_FOR, (yyvsp[-4].list), (yyvsp[-3].list), (yyvsp[-2].instr), (yyvsp[0].list), &loc);
                                pop_scope(&hlsl_ctx);
                            }
#line 3737 "hlsl.tab.c"
    break;

  case 107:
#line 1831 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-7]));
                                if (!(yyvsp[-4].list))
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_WARNING,
                                            "no expressions in for loop initializer");
                                (yyval.list) = create_loop(LOOP_FOR, (yyvsp[-4].list), (yyvsp[-3].list), (yyvsp[-2].instr), (yyvsp[0].list), &loc);
                                pop_scope(&hlsl_ctx);
                            }
#line 3752 "hlsl.tab.c"
    break;

  case 108:
#line 1843 "hlsl.y"
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                            }
#line 3761 "hlsl.tab.c"
    break;

  case 109:
#line 1848 "hlsl.y"
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                if ((yyvsp[-1].instr))
                                    list_add_head((yyval.list), &(yyvsp[-1].instr)->entry);
                            }
#line 3772 "hlsl.tab.c"
    break;

  case 110:
#line 1856 "hlsl.y"
    {
                                struct hlsl_ir_constant *c = d3dcompiler_alloc(sizeof(*c));
                                if (!c)
                                {
                                    ERR("Out of memory.\n");
                                    YYABORT;
                                }
                                c->node.type = HLSL_IR_CONSTANT;
                                set_location(&c->node.loc, &yylloc);
                                c->node.data_type = new_hlsl_type(d3dcompiler_strdup("float"), HLSL_CLASS_SCALAR, HLSL_TYPE_FLOAT, 1, 1);
                                c->v.value.f[0] = (yyvsp[0].floatval);
                                (yyval.instr) = &c->node;
                            }
#line 3790 "hlsl.tab.c"
    break;

  case 111:
#line 1870 "hlsl.y"
    {
                                struct hlsl_ir_constant *c = d3dcompiler_alloc(sizeof(*c));
                                if (!c)
                                {
                                    ERR("Out of memory.\n");
                                    YYABORT;
                                }
                                c->node.type = HLSL_IR_CONSTANT;
                                set_location(&c->node.loc, &yylloc);
                                c->node.data_type = new_hlsl_type(d3dcompiler_strdup("int"), HLSL_CLASS_SCALAR, HLSL_TYPE_INT, 1, 1);
                                c->v.value.i[0] = (yyvsp[0].intval);
                                (yyval.instr) = &c->node;
                            }
#line 3808 "hlsl.tab.c"
    break;

  case 112:
#line 1884 "hlsl.y"
    {
                                struct hlsl_ir_constant *c = d3dcompiler_alloc(sizeof(*c));
                                if (!c)
                                {
                                    ERR("Out of memory.\n");
                                    YYABORT;
                                }
                                c->node.type = HLSL_IR_CONSTANT;
                                set_location(&c->node.loc, &yylloc);
                                c->node.data_type = new_hlsl_type(d3dcompiler_strdup("bool"), HLSL_CLASS_SCALAR, HLSL_TYPE_BOOL, 1, 1);
                                c->v.value.b[0] = (yyvsp[0].boolval);
                                (yyval.instr) = &c->node;
                            }
#line 3826 "hlsl.tab.c"
    break;

  case 113:
#line 1898 "hlsl.y"
    {
                                struct hlsl_ir_deref *deref;
                                struct hlsl_ir_var *var;

                                if (!(var = get_variable(hlsl_ctx.cur_scope, (yyvsp[0].name))))
                                {
                                    hlsl_message("Line %d: variable '%s' not declared\n",
                                            hlsl_ctx.line_no, (yyvsp[0].name));
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    YYABORT;
                                }
                                if ((deref = new_var_deref(var)))
                                {
                                    (yyval.instr) = &deref->node;
                                    set_location(&(yyval.instr)->loc, &(yylsp[0]));
                                }
                                else
                                    (yyval.instr) = NULL;
                            }
#line 3850 "hlsl.tab.c"
    break;

  case 114:
#line 1918 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[-1].instr);
                            }
#line 3858 "hlsl.tab.c"
    break;

  case 115:
#line 1923 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 3866 "hlsl.tab.c"
    break;

  case 116:
#line 1927 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[0]));
                                if ((yyvsp[-1].instr)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "modifying a const expression");
                                    YYABORT;
                                }
                                (yyval.instr) = new_unary_expr(HLSL_IR_UNOP_POSTINC, (yyvsp[-1].instr), loc);
                                /* Post increment/decrement expressions are considered const */
                                (yyval.instr)->data_type = clone_hlsl_type((yyval.instr)->data_type);
                                (yyval.instr)->data_type->modifiers |= HLSL_MODIFIER_CONST;
                            }
#line 3886 "hlsl.tab.c"
    break;

  case 117:
#line 1943 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[0]));
                                if ((yyvsp[-1].instr)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "modifying a const expression");
                                    YYABORT;
                                }
                                (yyval.instr) = new_unary_expr(HLSL_IR_UNOP_POSTDEC, (yyvsp[-1].instr), loc);
                                /* Post increment/decrement expressions are considered const */
                                (yyval.instr)->data_type = clone_hlsl_type((yyval.instr)->data_type);
                                (yyval.instr)->data_type->modifiers |= HLSL_MODIFIER_CONST;
                            }
#line 3906 "hlsl.tab.c"
    break;

  case 118:
#line 1959 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                if ((yyvsp[-2].instr)->data_type->type == HLSL_CLASS_STRUCT)
                                {
                                    struct hlsl_type *type = (yyvsp[-2].instr)->data_type;
                                    struct hlsl_struct_field *field;

                                    (yyval.instr) = NULL;
                                    LIST_FOR_EACH_ENTRY(field, type->e.elements, struct hlsl_struct_field, entry)
                                    {
                                        if (!strcmp((yyvsp[0].name), field->name))
                                        {
                                            struct hlsl_ir_deref *deref = new_record_deref((yyvsp[-2].instr), field);

                                            if (!deref)
                                            {
                                                ERR("Out of memory\n");
                                                YYABORT;
                                            }
                                            deref->node.loc = loc;
                                            (yyval.instr) = &deref->node;
                                            break;
                                        }
                                    }
                                    if (!(yyval.instr))
                                    {
                                        hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                                "invalid subscript %s", debugstr_a((yyvsp[0].name)));
                                        YYABORT;
                                    }
                                }
                                else if ((yyvsp[-2].instr)->data_type->type <= HLSL_CLASS_LAST_NUMERIC)
                                {
                                    struct hlsl_ir_swizzle *swizzle;

                                    swizzle = get_swizzle((yyvsp[-2].instr), (yyvsp[0].name), &loc);
                                    if (!swizzle)
                                    {
                                        hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                                "invalid swizzle %s", debugstr_a((yyvsp[0].name)));
                                        YYABORT;
                                    }
                                    (yyval.instr) = &swizzle->node;
                                }
                                else
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "invalid subscript %s", debugstr_a((yyvsp[0].name)));
                                    YYABORT;
                                }
                            }
#line 3964 "hlsl.tab.c"
    break;

  case 119:
#line 2013 "hlsl.y"
    {
                                /* This may be an array dereference or a vector/matrix
                                 * subcomponent access.
                                 * We store it as an array dereference in any case. */
                                struct hlsl_ir_deref *deref = d3dcompiler_alloc(sizeof(*deref));
                                struct hlsl_type *expr_type = (yyvsp[-3].instr)->data_type;
                                struct source_location loc;

                                TRACE("Array dereference from type %s\n", debug_hlsl_type(expr_type));
                                if (!deref)
                                {
                                    ERR("Out of memory\n");
                                    YYABORT;
                                }
                                deref->node.type = HLSL_IR_DEREF;
                                set_location(&loc, &(yylsp[-2]));
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
                                    free_instr((yyvsp[-3].instr));
                                    free_instr((yyvsp[-1].instr));
                                    YYABORT;
                                }
                                if ((yyvsp[-1].instr)->data_type->type != HLSL_CLASS_SCALAR)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "array index is not scalar");
                                    d3dcompiler_free(deref);
                                    free_instr((yyvsp[-3].instr));
                                    free_instr((yyvsp[-1].instr));
                                    YYABORT;
                                }
                                deref->type = HLSL_IR_DEREF_ARRAY;
                                deref->v.array.array = (yyvsp[-3].instr);
                                deref->v.array.index = (yyvsp[-1].instr);

                                (yyval.instr) = &deref->node;
                            }
#line 4026 "hlsl.tab.c"
    break;

  case 120:
#line 2073 "hlsl.y"
    {
                                struct hlsl_ir_constructor *constructor;

                                TRACE("%s constructor.\n", debug_hlsl_type((yyvsp[-3].type)));
                                if ((yyvsp[-4].modifiers))
                                {
                                    hlsl_message("Line %u: unexpected modifier in a constructor.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    YYABORT;
                                }
                                if ((yyvsp[-3].type)->type > HLSL_CLASS_LAST_NUMERIC)
                                {
                                    hlsl_message("Line %u: constructors are allowed only for numeric data types.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    YYABORT;
                                }
                                if ((yyvsp[-3].type)->dimx * (yyvsp[-3].type)->dimy != initializer_size(&(yyvsp[-1].initializer)))
                                {
                                    hlsl_message("Line %u: wrong number of components in constructor.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    YYABORT;
                                }
                                assert((yyvsp[-1].initializer).args_count <= ARRAY_SIZE(constructor->args));

                                constructor = d3dcompiler_alloc(sizeof(*constructor));
                                constructor->node.type = HLSL_IR_CONSTRUCTOR;
                                set_location(&constructor->node.loc, &(yylsp[-2]));
                                constructor->node.data_type = (yyvsp[-3].type);
                                constructor->args_count = (yyvsp[-1].initializer).args_count;
                                memcpy(constructor->args, (yyvsp[-1].initializer).args, (yyvsp[-1].initializer).args_count * sizeof(*(yyvsp[-1].initializer).args));
                                d3dcompiler_free((yyvsp[-1].initializer).args);
                                (yyval.instr) = &constructor->node;
                            }
#line 4067 "hlsl.tab.c"
    break;

  case 121:
#line 2111 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4075 "hlsl.tab.c"
    break;

  case 122:
#line 2115 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                if ((yyvsp[0].instr)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "modifying a const expression");
                                    YYABORT;
                                }
                                (yyval.instr) = new_unary_expr(HLSL_IR_UNOP_PREINC, (yyvsp[0].instr), loc);
                            }
#line 4092 "hlsl.tab.c"
    break;

  case 123:
#line 2128 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                if ((yyvsp[0].instr)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "modifying a const expression");
                                    YYABORT;
                                }
                                (yyval.instr) = new_unary_expr(HLSL_IR_UNOP_PREDEC, (yyvsp[0].instr), loc);
                            }
#line 4109 "hlsl.tab.c"
    break;

  case 124:
#line 2141 "hlsl.y"
    {
                                enum hlsl_ir_expr_op ops[] = {0, HLSL_IR_UNOP_NEG,
                                        HLSL_IR_UNOP_LOGIC_NOT, HLSL_IR_UNOP_BIT_NOT};
                                struct source_location loc;

                                if ((yyvsp[-1].unary_op) == UNARY_OP_PLUS)
                                {
                                    (yyval.instr) = (yyvsp[0].instr);
                                }
                                else
                                {
                                    set_location(&loc, &(yylsp[-1]));
                                    (yyval.instr) = new_unary_expr(ops[(yyvsp[-1].unary_op)], (yyvsp[0].instr), loc);
                                }
                            }
#line 4129 "hlsl.tab.c"
    break;

  case 125:
#line 2158 "hlsl.y"
    {
                                struct hlsl_ir_expr *expr;
                                struct hlsl_type *src_type = (yyvsp[0].instr)->data_type;
                                struct hlsl_type *dst_type;
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-3]));
                                if ((yyvsp[-4].modifiers))
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "unexpected modifier in a cast");
                                    YYABORT;
                                }

                                if ((yyvsp[-2].intval))
                                    dst_type = new_array_type((yyvsp[-3].type), (yyvsp[-2].intval));
                                else
                                    dst_type = (yyvsp[-3].type);

                                if (!compatible_data_types(src_type, dst_type))
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "can't cast from %s to %s",
                                            debug_hlsl_type(src_type), debug_hlsl_type(dst_type));
                                    YYABORT;
                                }

                                expr = new_cast((yyvsp[0].instr), dst_type, &loc);
                                (yyval.instr) = expr ? &expr->node : NULL;
                            }
#line 4164 "hlsl.tab.c"
    break;

  case 126:
#line 2190 "hlsl.y"
    {
                                (yyval.unary_op) = UNARY_OP_PLUS;
                            }
#line 4172 "hlsl.tab.c"
    break;

  case 127:
#line 2194 "hlsl.y"
    {
                                (yyval.unary_op) = UNARY_OP_MINUS;
                            }
#line 4180 "hlsl.tab.c"
    break;

  case 128:
#line 2198 "hlsl.y"
    {
                                (yyval.unary_op) = UNARY_OP_LOGICNOT;
                            }
#line 4188 "hlsl.tab.c"
    break;

  case 129:
#line 2202 "hlsl.y"
    {
                                (yyval.unary_op) = UNARY_OP_BITNOT;
                            }
#line 4196 "hlsl.tab.c"
    break;

  case 130:
#line 2207 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4204 "hlsl.tab.c"
    break;

  case 131:
#line 2211 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = new_binary_expr(HLSL_IR_BINOP_MUL, (yyvsp[-2].instr), (yyvsp[0].instr), loc);
                            }
#line 4215 "hlsl.tab.c"
    break;

  case 132:
#line 2218 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = new_binary_expr(HLSL_IR_BINOP_DIV, (yyvsp[-2].instr), (yyvsp[0].instr), loc);
                            }
#line 4226 "hlsl.tab.c"
    break;

  case 133:
#line 2225 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = new_binary_expr(HLSL_IR_BINOP_MOD, (yyvsp[-2].instr), (yyvsp[0].instr), loc);
                            }
#line 4237 "hlsl.tab.c"
    break;

  case 134:
#line 2233 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4245 "hlsl.tab.c"
    break;

  case 135:
#line 2237 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = new_binary_expr(HLSL_IR_BINOP_ADD, (yyvsp[-2].instr), (yyvsp[0].instr), loc);
                            }
#line 4256 "hlsl.tab.c"
    break;

  case 136:
#line 2244 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = new_binary_expr(HLSL_IR_BINOP_SUB, (yyvsp[-2].instr), (yyvsp[0].instr), loc);
                            }
#line 4267 "hlsl.tab.c"
    break;

  case 137:
#line 2252 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4275 "hlsl.tab.c"
    break;

  case 138:
#line 2256 "hlsl.y"
    {
                                FIXME("Left shift\n");
                            }
#line 4283 "hlsl.tab.c"
    break;

  case 139:
#line 2260 "hlsl.y"
    {
                                FIXME("Right shift\n");
                            }
#line 4291 "hlsl.tab.c"
    break;

  case 140:
#line 2265 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4299 "hlsl.tab.c"
    break;

  case 141:
#line 2269 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = new_binary_expr(HLSL_IR_BINOP_LESS, (yyvsp[-2].instr), (yyvsp[0].instr), loc);
                            }
#line 4310 "hlsl.tab.c"
    break;

  case 142:
#line 2276 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = new_binary_expr(HLSL_IR_BINOP_GREATER, (yyvsp[-2].instr), (yyvsp[0].instr), loc);
                            }
#line 4321 "hlsl.tab.c"
    break;

  case 143:
#line 2283 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = new_binary_expr(HLSL_IR_BINOP_LEQUAL, (yyvsp[-2].instr), (yyvsp[0].instr), loc);
                            }
#line 4332 "hlsl.tab.c"
    break;

  case 144:
#line 2290 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = new_binary_expr(HLSL_IR_BINOP_GEQUAL, (yyvsp[-2].instr), (yyvsp[0].instr), loc);
                            }
#line 4343 "hlsl.tab.c"
    break;

  case 145:
#line 2298 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4351 "hlsl.tab.c"
    break;

  case 146:
#line 2302 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = new_binary_expr(HLSL_IR_BINOP_EQUAL, (yyvsp[-2].instr), (yyvsp[0].instr), loc);
                            }
#line 4362 "hlsl.tab.c"
    break;

  case 147:
#line 2309 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = new_binary_expr(HLSL_IR_BINOP_NEQUAL, (yyvsp[-2].instr), (yyvsp[0].instr), loc);
                            }
#line 4373 "hlsl.tab.c"
    break;

  case 148:
#line 2317 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4381 "hlsl.tab.c"
    break;

  case 149:
#line 2321 "hlsl.y"
    {
                                FIXME("bitwise AND\n");
                            }
#line 4389 "hlsl.tab.c"
    break;

  case 150:
#line 2326 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4397 "hlsl.tab.c"
    break;

  case 151:
#line 2330 "hlsl.y"
    {
                                FIXME("bitwise XOR\n");
                            }
#line 4405 "hlsl.tab.c"
    break;

  case 152:
#line 2335 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4413 "hlsl.tab.c"
    break;

  case 153:
#line 2339 "hlsl.y"
    {
                                FIXME("bitwise OR\n");
                            }
#line 4421 "hlsl.tab.c"
    break;

  case 154:
#line 2344 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4429 "hlsl.tab.c"
    break;

  case 155:
#line 2348 "hlsl.y"
    {
                                FIXME("logic AND\n");
                            }
#line 4437 "hlsl.tab.c"
    break;

  case 156:
#line 2353 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4445 "hlsl.tab.c"
    break;

  case 157:
#line 2357 "hlsl.y"
    {
                                FIXME("logic OR\n");
                            }
#line 4453 "hlsl.tab.c"
    break;

  case 158:
#line 2362 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4461 "hlsl.tab.c"
    break;

  case 159:
#line 2366 "hlsl.y"
    {
                                FIXME("ternary operator\n");
                            }
#line 4469 "hlsl.tab.c"
    break;

  case 160:
#line 2371 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4477 "hlsl.tab.c"
    break;

  case 161:
#line 2375 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                if ((yyvsp[-2].instr)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "l-value is const");
                                    YYABORT;
                                }
                                (yyval.instr) = make_assignment((yyvsp[-2].instr), (yyvsp[-1].assign_op), BWRITERSP_WRITEMASK_ALL, (yyvsp[0].instr));
                                if (!(yyval.instr))
                                    YYABORT;
                                (yyval.instr)->loc = loc;
                            }
#line 4497 "hlsl.tab.c"
    break;

  case 162:
#line 2392 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_ASSIGN;
                            }
#line 4505 "hlsl.tab.c"
    break;

  case 163:
#line 2396 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_ADD;
                            }
#line 4513 "hlsl.tab.c"
    break;

  case 164:
#line 2400 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_SUB;
                            }
#line 4521 "hlsl.tab.c"
    break;

  case 165:
#line 2404 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_MUL;
                            }
#line 4529 "hlsl.tab.c"
    break;

  case 166:
#line 2408 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_DIV;
                            }
#line 4537 "hlsl.tab.c"
    break;

  case 167:
#line 2412 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_MOD;
                            }
#line 4545 "hlsl.tab.c"
    break;

  case 168:
#line 2416 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_LSHIFT;
                            }
#line 4553 "hlsl.tab.c"
    break;

  case 169:
#line 2420 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_RSHIFT;
                            }
#line 4561 "hlsl.tab.c"
    break;

  case 170:
#line 2424 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_AND;
                            }
#line 4569 "hlsl.tab.c"
    break;

  case 171:
#line 2428 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_OR;
                            }
#line 4577 "hlsl.tab.c"
    break;

  case 172:
#line 2432 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_XOR;
                            }
#line 4585 "hlsl.tab.c"
    break;

  case 173:
#line 2437 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4593 "hlsl.tab.c"
    break;

  case 174:
#line 2441 "hlsl.y"
    {
                                FIXME("Comma expression\n");
                            }
#line 4601 "hlsl.tab.c"
    break;


#line 4605 "hlsl.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }

  yyerror_range[1] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 2445 "hlsl.y"


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
    d3dcompiler_free((void*)hlsl_ctx.source_files);

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
