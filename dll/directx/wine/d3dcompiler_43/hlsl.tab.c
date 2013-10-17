/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
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

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.5"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 1

/* Substitute the variable and function names.  */
#define yyparse         hlsl_parse
#define yylex           hlsl_lex
#define yyerror         hlsl_error
#define yylval          hlsl_lval
#define yychar          hlsl_char
#define yydebug         hlsl_debug
#define yynerrs         hlsl_nerrs
#define yylloc          hlsl_lloc

/* Copy the first part of user declarations.  */

/* Line 268 of yacc.c  */
#line 21 "hlsl.y"

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
    const char *names[] =
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
    static const char *names[] =
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



/* Line 268 of yacc.c  */
#line 895 "hlsl.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
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



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 293 of yacc.c  */
#line 841 "hlsl.y"

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



/* Line 293 of yacc.c  */
#line 1055 "hlsl.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


/* Copy the second part of user declarations.  */


/* Line 343 of yacc.c  */
#line 1080 "hlsl.tab.c"

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
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
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
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
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
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   848

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  129
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  63
/* YYNRULES -- Number of rules.  */
#define YYNRULES  171
/* YYNRULES -- Number of states.  */
#define YYNSTATES  312

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   359

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
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
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     7,    10,    13,    16,    19,    23,
      25,    27,    34,    40,    42,    44,    46,    47,    50,    55,
      59,    62,    65,    73,    76,    81,    82,    84,    86,    87,
      90,    92,    95,    97,   101,   107,   108,   111,   113,   115,
     117,   119,   126,   135,   137,   139,   141,   143,   145,   147,
     149,   152,   154,   156,   158,   164,   169,   171,   175,   178,
     183,   184,   186,   188,   192,   196,   202,   203,   207,   208,
     211,   214,   217,   220,   223,   226,   229,   232,   235,   238,
     241,   243,   247,   252,   254,   256,   260,   262,   264,   266,
     269,   271,   273,   275,   277,   279,   281,   285,   291,   293,
     297,   303,   311,   320,   329,   331,   334,   336,   338,   340,
     342,   346,   348,   350,   353,   356,   360,   365,   371,   373,
     376,   379,   382,   389,   391,   393,   395,   397,   399,   403,
     407,   411,   413,   417,   421,   423,   427,   431,   433,   437,
     441,   445,   449,   451,   455,   459,   461,   465,   467,   471,
     473,   477,   479,   483,   485,   489,   491,   497,   499,   503,
     505,   507,   509,   511,   513,   515,   517,   519,   521,   523,
     525,   527
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     130,     0,    -1,    -1,   130,   139,    -1,   130,   152,    -1,
     130,   131,    -1,   130,   105,    -1,    98,   102,    -1,   133,
     157,   105,    -1,   134,    -1,   135,    -1,   161,    50,   136,
     106,   137,   107,    -1,   161,    50,   106,   137,   107,    -1,
      99,    -1,   100,    -1,   101,    -1,    -1,   137,   138,    -1,
     161,   150,   158,   105,    -1,   135,   158,   105,    -1,   140,
     141,    -1,   140,   105,    -1,   161,   150,   143,   108,   145,
     109,   144,    -1,   106,   107,    -1,   106,   142,   166,   107,
      -1,    -1,    99,    -1,   101,    -1,    -1,   110,   136,    -1,
     142,    -1,   142,   146,    -1,   147,    -1,   146,   111,   147,
      -1,   148,   161,   150,   136,   144,    -1,    -1,   148,   149,
      -1,    23,    -1,    29,    -1,    25,    -1,   151,    -1,    68,
     112,   151,   111,   104,   113,    -1,    26,   112,   151,   111,
     104,   111,   104,   113,    -1,    70,    -1,    38,    -1,    39,
      -1,    40,    -1,    41,    -1,    42,    -1,   100,    -1,    50,
     100,    -1,   156,    -1,   132,    -1,   153,    -1,    66,   161,
     150,   154,   105,    -1,    66,   133,   154,   105,    -1,   155,
      -1,   154,   111,   155,    -1,   136,   160,    -1,   161,   150,
     158,   105,    -1,    -1,   158,    -1,   159,    -1,   158,   111,
     159,    -1,   136,   160,   144,    -1,   136,   160,   144,   114,
     162,    -1,    -1,   115,   191,   116,    -1,    -1,    17,   161,
      -1,    28,   161,    -1,    32,   161,    -1,    45,   161,    -1,
      21,   161,    -1,    48,   161,    -1,    67,   161,    -1,    71,
     161,    -1,     9,   161,    -1,    37,   161,    -1,     7,   161,
      -1,   163,    -1,   106,   164,   107,    -1,   106,   164,   111,
     107,    -1,   189,    -1,   163,    -1,   164,   111,   163,    -1,
      65,    -1,    18,    -1,   167,    -1,   166,   167,    -1,   152,
      -1,   172,    -1,   141,    -1,   168,    -1,   169,    -1,   171,
      -1,    35,   191,   105,    -1,    22,   108,   191,   109,   170,
      -1,   167,    -1,   167,    16,   167,    -1,    72,   108,   191,
     109,   167,    -1,    14,   167,    72,   108,   191,   109,   105,
      -1,    19,   108,   142,   172,   172,   191,   109,   167,    -1,
      19,   108,   142,   156,   172,   191,   109,   167,    -1,   105,
      -1,   191,   105,    -1,   103,    -1,   104,    -1,   165,    -1,
     174,    -1,   108,   191,   109,    -1,    99,    -1,   173,    -1,
     175,    73,    -1,   175,    74,    -1,   175,   117,   136,    -1,
     175,   115,   191,   116,    -1,   161,   150,   108,   164,   109,
      -1,   175,    -1,    73,   176,    -1,    74,   176,    -1,   177,
     176,    -1,   108,   161,   150,   160,   109,   176,    -1,   118,
      -1,   119,    -1,   120,    -1,   121,    -1,   176,    -1,   178,
     122,   176,    -1,   178,   123,   176,    -1,   178,   124,   176,
      -1,   178,    -1,   179,   118,   178,    -1,   179,   119,   178,
      -1,   179,    -1,   180,    78,   179,    -1,   180,    80,   179,
      -1,   180,    -1,   181,   112,   180,    -1,   181,   113,   180,
      -1,   181,    83,   180,    -1,   181,    84,   180,    -1,   181,
      -1,   182,    77,   181,    -1,   182,    85,   181,    -1,   182,
      -1,   183,   125,   182,    -1,   183,    -1,   184,   126,   183,
      -1,   184,    -1,   185,   127,   184,    -1,   185,    -1,   186,
      75,   185,    -1,   186,    -1,   187,    76,   186,    -1,   187,
      -1,   187,   128,   191,   110,   189,    -1,   188,    -1,   176,
     190,   189,    -1,   114,    -1,    86,    -1,    87,    -1,    88,
      -1,    89,    -1,    90,    -1,    79,    -1,    81,    -1,    91,
      -1,    92,    -1,    93,    -1,   189,    -1,   191,   111,   189,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1022,  1022,  1024,  1061,  1065,  1068,  1073,  1092,  1109,
    1110,  1112,  1138,  1148,  1149,  1150,  1153,  1157,  1176,  1180,
    1185,  1192,  1199,  1224,  1229,  1236,  1240,  1241,  1244,  1247,
    1252,  1257,  1262,  1276,  1290,  1300,  1303,  1314,  1318,  1322,
    1327,  1331,  1350,  1370,  1374,  1379,  1384,  1389,  1394,  1399,
    1408,  1427,  1428,  1429,  1440,  1448,  1457,  1463,  1469,  1477,
    1483,  1486,  1491,  1497,  1503,  1511,  1523,  1526,  1534,  1537,
    1541,  1545,  1549,  1553,  1557,  1561,  1565,  1569,  1573,  1577,
    1582,  1588,  1592,  1597,  1602,  1608,  1614,  1618,  1623,  1627,
    1634,  1635,  1636,  1637,  1638,  1639,  1642,  1665,  1689,  1694,
    1700,  1715,  1730,  1738,  1750,  1755,  1763,  1777,  1791,  1805,
    1816,  1821,  1835,  1839,  1858,  1877,  1931,  1991,  2027,  2031,
    2047,  2063,  2083,  2115,  2119,  2123,  2127,  2132,  2136,  2143,
    2150,  2158,  2162,  2169,  2177,  2181,  2185,  2190,  2194,  2201,
    2208,  2215,  2223,  2227,  2234,  2242,  2246,  2251,  2255,  2260,
    2264,  2269,  2273,  2278,  2282,  2287,  2291,  2296,  2300,  2317,
    2321,  2325,  2329,  2333,  2337,  2341,  2345,  2349,  2353,  2357,
    2362,  2366
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
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
  "scope_start", "var_identifier", "semantic", "parameters", "param_list",
  "parameter", "input_mods", "input_mod", "type", "base_type",
  "declaration_statement", "typedef", "type_specs", "type_spec",
  "declaration", "variables_def_optional", "variables_def", "variable_def",
  "array", "var_modifiers", "complex_initializer", "initializer_expr",
  "initializer_expr_list", "boolean", "statement_list", "statement",
  "jump_statement", "selection_statement", "if_body", "loop_statement",
  "expr_statement", "primary_expr", "variable", "postfix_expr",
  "unary_expr", "unary_op", "mul_expr", "add_expr", "shift_expr",
  "relational_expr", "equality_expr", "bitand_expr", "bitxor_expr",
  "bitor_expr", "logicand_expr", "logicor_expr", "conditional_expr",
  "assignment_expr", "assign_op", "expr", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
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

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   129,   130,   130,   130,   130,   130,   131,   132,   133,
     133,   134,   135,   136,   136,   136,   137,   137,   138,   138,
     139,   139,   140,   141,   141,   142,   143,   143,   144,   144,
     145,   145,   146,   146,   147,   148,   148,   149,   149,   149,
     150,   150,   150,   151,   151,   151,   151,   151,   151,   151,
     151,   152,   152,   152,   153,   153,   154,   154,   155,   156,
     157,   157,   158,   158,   159,   159,   160,   160,   161,   161,
     161,   161,   161,   161,   161,   161,   161,   161,   161,   161,
     162,   162,   162,   163,   164,   164,   165,   165,   166,   166,
     167,   167,   167,   167,   167,   167,   168,   169,   170,   170,
     171,   171,   171,   171,   172,   172,   173,   173,   173,   173,
     173,   174,   175,   175,   175,   175,   175,   175,   176,   176,
     176,   176,   176,   177,   177,   177,   177,   178,   178,   178,
     178,   179,   179,   179,   180,   180,   180,   181,   181,   181,
     181,   181,   182,   182,   182,   183,   183,   184,   184,   185,
     185,   186,   186,   187,   187,   188,   188,   189,   189,   190,
     190,   190,   190,   190,   190,   190,   190,   190,   190,   190,
     191,   191
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     2,     2,     2,     2,     3,     1,
       1,     6,     5,     1,     1,     1,     0,     2,     4,     3,
       2,     2,     7,     2,     4,     0,     1,     1,     0,     2,
       1,     2,     1,     3,     5,     0,     2,     1,     1,     1,
       1,     6,     8,     1,     1,     1,     1,     1,     1,     1,
       2,     1,     1,     1,     5,     4,     1,     3,     2,     4,
       0,     1,     1,     3,     3,     5,     0,     3,     0,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       1,     3,     4,     1,     1,     3,     1,     1,     1,     2,
       1,     1,     1,     1,     1,     1,     3,     5,     1,     3,
       5,     7,     8,     8,     1,     2,     1,     1,     1,     1,
       3,     1,     1,     2,     2,     3,     4,     5,     1,     2,
       2,     2,     6,     1,     1,     1,     1,     1,     3,     3,
       3,     1,     3,     3,     1,     3,     3,     1,     3,     3,
       3,     3,     1,     3,     3,     1,     3,     1,     3,     1,
       3,     1,     3,     1,     3,     1,     5,     1,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     3
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,    68,     1,    68,    68,    68,    68,    68,    68,    68,
      68,    68,    68,    68,    68,     0,     6,     5,    52,    60,
       9,    10,     3,     0,     4,    53,    51,     0,    79,    77,
      69,    73,    70,    71,    78,    72,    74,     0,     0,    75,
      76,     7,    13,    14,    15,    66,     0,    61,    62,    21,
      25,    20,     0,    44,    45,    46,    47,    48,     0,     0,
      43,    49,     0,    40,    66,     0,    56,     0,    68,    28,
       8,     0,    23,    68,     0,    50,    16,     0,     0,    13,
      15,     0,     0,    58,    55,     0,     0,    87,    86,    68,
      68,   111,   106,   107,    68,   123,   124,   125,   126,     0,
     108,   112,   109,   118,   127,    68,   131,   134,   137,   142,
     145,   147,   149,   151,   153,   155,   157,   170,     0,     0,
      64,    63,    68,     0,     0,    68,     0,   104,    92,    90,
       0,    68,    88,    93,    94,    95,    91,     0,     0,     0,
      68,    16,     0,    25,    59,    57,    54,   119,   120,     0,
       0,     0,   113,   114,    68,     0,   165,   166,   160,   161,
     162,   163,   164,   167,   168,   169,   159,    68,   121,    68,
      68,    68,    68,    68,    68,    68,    68,    68,    68,    68,
      68,    68,    68,    68,    68,    68,    68,    68,    68,    67,
      29,    68,     0,    25,    68,     0,    68,     0,    24,    89,
     105,    50,     0,    12,     0,    17,     0,    68,     0,    35,
       0,    66,   110,    68,     0,   115,   158,   128,   129,   130,
     127,   132,   133,   135,   136,   140,   141,   138,   139,   143,
     144,   146,   148,   150,   152,   154,     0,   171,    68,    65,
      80,    83,     0,    68,     0,    96,     0,     0,     0,     0,
       0,    11,     0,    31,    32,    68,    28,     0,    84,     0,
     116,    68,     0,    68,    68,     0,    68,    68,    68,     0,
      19,     0,    41,    35,    37,    39,    38,    36,     0,    22,
      68,   117,    68,   156,    81,    68,     0,    68,    68,    98,
      97,   100,     0,    18,    33,     0,   122,    85,    82,     0,
       0,     0,    68,    42,    28,   101,    68,    68,    99,    34,
     103,   102
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    17,    18,    19,    20,    21,    45,   140,   205,
      22,    23,   128,    73,    81,   120,   210,   253,   254,   255,
     277,   197,    63,   129,    25,    65,    66,    26,    46,    82,
      48,    69,    99,   239,   258,   259,   100,   131,   132,   133,
     134,   290,   135,   136,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   167,   137
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -231
static const yytype_int16 yypact[] =
{
    -231,   640,  -231,   777,   777,   777,   777,   777,   777,   777,
     777,   777,   777,   777,   777,   -69,  -231,  -231,  -231,    32,
    -231,  -231,  -231,   118,  -231,  -231,  -231,    12,  -231,  -231,
    -231,  -231,  -231,  -231,  -231,  -231,  -231,    32,    12,  -231,
    -231,  -231,  -231,  -231,  -231,   -67,   -41,   -26,  -231,  -231,
       8,  -231,   -20,  -231,  -231,  -231,  -231,  -231,   -60,    14,
    -231,  -231,    72,  -231,   -67,   -81,  -231,    32,   579,   -39,
    -231,    32,  -231,   320,   145,    28,  -231,    34,   145,    15,
      35,    74,   -15,  -231,  -231,    32,   -10,  -231,  -231,   579,
     579,  -231,  -231,  -231,   579,  -231,  -231,  -231,  -231,    37,
    -231,  -231,  -231,   -18,   182,   579,    52,    86,   -19,   -55,
     -40,    66,    70,    90,   126,   -44,  -231,  -231,     2,    32,
     127,  -231,   320,   105,   124,   579,   135,  -231,  -231,  -231,
      12,   212,  -231,  -231,  -231,  -231,  -231,    -7,   125,   137,
     684,  -231,   141,  -231,  -231,  -231,  -231,  -231,  -231,    37,
      79,   147,  -231,  -231,   579,    32,  -231,  -231,  -231,  -231,
    -231,  -231,  -231,  -231,  -231,  -231,  -231,   579,  -231,   579,
     579,   579,   579,   579,   579,   579,   579,   579,   579,   579,
     579,   579,   579,   579,   579,   579,   579,   579,   579,  -231,
    -231,   399,   181,  -231,   579,    -5,   579,   -65,  -231,  -231,
    -231,  -231,   154,  -231,    32,  -231,   139,   716,   158,   155,
     167,   -48,  -231,   579,    11,  -231,  -231,  -231,  -231,  -231,
    -231,    52,    52,    86,    86,   -19,   -19,   -19,   -19,   -55,
     -55,   -40,    66,    70,    90,   126,   117,  -231,   579,  -231,
    -231,  -231,   174,   439,    83,  -231,    88,   176,    -2,    10,
      32,  -231,   175,   178,  -231,   745,   -39,   183,  -231,    99,
    -231,   579,    17,   579,   439,    37,   439,   320,   320,   186,
    -231,     9,  -231,  -231,  -231,  -231,  -231,  -231,    37,  -231,
     579,  -231,   579,  -231,  -231,   518,   103,   579,   579,   275,
    -231,  -231,   180,  -231,  -231,    32,  -231,  -231,  -231,   189,
     107,   111,   320,  -231,   -39,  -231,   320,   320,  -231,  -231,
    -231,  -231
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -231,  -231,  -231,  -231,   283,  -231,  -119,   -36,   156,  -231,
    -231,  -231,   276,  -123,  -231,  -230,  -231,  -231,    25,  -231,
    -231,   -13,    51,   299,  -231,   235,   218,    61,  -231,    -4,
     236,   -45,    -1,  -231,  -174,    71,  -231,  -231,  -104,  -231,
    -231,  -231,  -231,  -175,  -231,  -231,  -231,   -24,  -231,    65,
      76,    -9,   100,   128,   129,   130,   123,   136,  -231,  -231,
    -144,  -231,   -52
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -31
static const yytype_int16 yytable[] =
{
      27,    64,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    38,    39,    40,    62,    47,   118,   240,   192,    83,
     209,   204,    77,   216,    84,    67,   279,   199,   176,   177,
      85,    64,   186,    41,    42,    43,    44,   180,    52,    42,
      75,    44,   150,   213,   237,   181,    76,   241,    68,    64,
      53,    54,    55,    56,    57,   152,   153,   178,   179,   174,
     213,   175,    58,    52,    70,   147,   148,    68,   266,   241,
     243,   119,   130,   195,   309,    53,    54,    55,    56,    57,
      59,   168,    60,   190,   187,    71,   151,   138,   204,   287,
     144,   288,    74,   149,   241,   146,    71,   154,   200,   155,
     245,    85,   214,   270,   188,    59,   188,    60,   297,    71,
     201,   297,    61,   188,   293,    72,    76,   283,   189,   215,
      71,   130,   188,   -26,   284,   139,    78,   260,   285,   142,
     130,    42,    43,    44,   -14,   236,   211,    61,   241,   206,
     141,   241,   244,   -27,   246,   217,   218,   219,   220,   220,
     220,   220,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   289,   291,    52,   257,   225,   226,   227,
     228,    79,    43,    80,   169,   170,   171,    53,    54,    55,
      56,    57,   143,    53,    54,    55,    56,    57,   212,   249,
     188,   182,   267,   250,   188,   138,   183,   268,   308,   188,
     248,   185,   310,   311,   172,   173,   206,    59,   281,    60,
     282,   286,   299,   193,   188,    60,   306,   184,   188,     3,
     307,     4,   188,    49,    50,   201,   122,   261,   188,     5,
      87,   123,   194,     6,   124,   300,   301,   221,   222,    61,
       7,   191,   265,   196,     8,    61,   271,   125,   202,     9,
     223,   224,   208,   242,   278,   213,   296,    10,   247,   304,
      11,   156,   252,   157,   -30,   295,   130,   130,   158,   159,
     160,   161,   162,   163,   164,   165,   256,    88,    12,    13,
     229,   230,   263,    14,   126,    89,    90,   269,   272,   273,
     292,   302,   280,   303,   305,    37,   166,   207,   294,    51,
      24,   130,    86,   145,   264,   130,   130,   121,   234,   262,
     231,    91,   232,     0,   233,    92,    93,   127,    50,   198,
      94,     0,   235,     0,     0,     0,     0,     3,     0,     4,
      95,    96,    97,    98,   122,     0,     0,     5,    87,   123,
       0,     6,   124,     0,     0,     0,     0,     0,     7,     0,
       0,     0,     8,     0,     0,   125,     0,     9,     0,     0,
       0,     0,     0,     0,     0,    10,     0,     0,    11,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    88,    12,    13,     0,     0,
       0,    14,   126,    89,    90,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     3,     0,     4,     0,
       0,     0,     0,     0,     0,     0,     5,    87,     0,    91,
       6,     0,     0,    92,    93,   127,    50,     7,    94,     0,
       0,     8,     0,     0,     0,     0,     9,     0,    95,    96,
      97,    98,     0,     0,    10,     0,     3,    11,     4,     0,
       0,     0,     0,     0,     0,     0,     5,    87,     0,     0,
       6,     0,     0,     0,    88,     0,    13,     7,     0,     0,
      14,     8,    89,    90,     0,     0,     9,     0,     0,     0,
       0,     0,     0,     0,    10,     0,     0,    11,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    91,     0,
       0,     0,    92,    93,    88,   238,    13,    94,     0,     0,
      14,     0,    89,    90,     0,     0,     0,    95,    96,    97,
      98,     0,     0,     0,     0,     3,     0,     4,     0,     0,
       0,     0,     0,     0,     0,     5,    87,     0,    91,     6,
       0,     0,    92,    93,   127,     0,     7,    94,     0,     0,
       8,     0,     0,     0,     0,     9,     0,    95,    96,    97,
      98,     0,     0,    10,     0,     0,    11,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    88,     0,    13,     3,     0,     4,    14,
       0,    89,    90,     0,     0,     0,     5,    87,     0,     0,
       6,     0,     0,     0,     0,     0,     0,     7,     0,     0,
       0,     8,     0,     0,     0,     0,     9,    91,     0,     0,
       0,    92,    93,     0,    10,   298,    94,    11,     0,     0,
       0,     0,     0,     0,     0,     0,    95,    96,    97,    98,
       2,     0,     0,     0,    88,     0,    13,     3,     0,     4,
      14,     0,    89,    90,     0,     0,     0,     5,     0,     0,
       0,     6,     0,     0,     0,     0,     0,     0,     7,     0,
       0,     0,     8,     0,     0,     0,     0,     9,    91,     0,
       0,     0,    92,    93,     0,    10,     0,    94,    11,     0,
       0,     3,     0,     4,     0,     0,     0,    95,    96,    97,
      98,     5,     0,     0,     0,     6,    12,    13,     0,     0,
       0,    14,     7,     0,     0,     0,     8,     0,     0,     0,
       0,     9,     0,     3,     0,     4,     0,     0,     0,    10,
       0,     0,    11,     5,     0,     0,     0,     6,    15,     0,
       0,     0,     0,     0,     7,    16,     0,     0,     8,     0,
       0,    13,     3,     9,     4,    14,     0,     0,     0,     0,
       0,    10,     5,     0,    11,     0,     6,     0,   274,     0,
     275,     0,     0,     7,   276,     0,     0,     8,     0,     0,
       0,     0,     9,    13,     3,     0,     4,    14,     0,     0,
      10,   203,     0,    11,     5,     0,     0,     0,     6,     0,
       0,     0,     0,     0,     0,     7,     0,     0,     0,     8,
       0,     0,    13,     0,     9,     0,    14,     0,     0,     0,
       0,     0,    10,   251,     0,    11,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    13,     0,     0,     0,    14
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-231))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
       1,    37,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    27,    19,    68,   191,   122,    64,
     143,   140,    58,   167,   105,    38,   256,   131,    83,    84,
     111,    67,    76,   102,    99,   100,   101,    77,    26,    99,
     100,   101,    94,   108,   188,    85,   106,   191,   115,    85,
      38,    39,    40,    41,    42,    73,    74,   112,   113,    78,
     108,    80,    50,    26,   105,    89,    90,   115,   243,   213,
     193,   110,    73,   125,   304,    38,    39,    40,    41,    42,
      68,   105,    70,   119,   128,   111,    99,    50,   207,   264,
     105,   266,   112,    94,   238,   105,   111,   115,   105,   117,
     105,   111,   154,   105,   111,    68,   111,    70,   282,   111,
     100,   285,   100,   111,   105,   107,   106,   261,   116,   155,
     111,   122,   111,   108,   107,    74,   112,   116,   111,    78,
     131,    99,   100,   101,   106,   187,   149,   100,   282,   140,
     106,   285,   194,   108,   196,   169,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,   180,   181,   182,   183,
     184,   185,   186,   267,   268,    26,   211,   176,   177,   178,
     179,    99,   100,   101,   122,   123,   124,    38,    39,    40,
      41,    42,   108,    38,    39,    40,    41,    42,   109,    50,
     111,   125,   109,   206,   111,    50,   126,   109,   302,   111,
     204,    75,   306,   307,   118,   119,   207,    68,   109,    70,
     111,   263,   109,   108,   111,    70,   109,   127,   111,     7,
     109,     9,   111,   105,   106,   100,    14,   110,   111,    17,
      18,    19,   108,    21,    22,   287,   288,   172,   173,   100,
      28,   114,   243,   108,    32,   100,   250,    35,   111,    37,
     174,   175,   111,    72,   255,   108,   280,    45,   104,   295,
      48,    79,   104,    81,   109,   278,   267,   268,    86,    87,
      88,    89,    90,    91,    92,    93,   109,    65,    66,    67,
     180,   181,   108,    71,    72,    73,    74,   111,   113,   111,
     104,    16,   109,   113,   105,    12,   114,   141,   273,    23,
       1,   302,    67,    85,   243,   306,   307,    71,   185,   238,
     182,    99,   183,    -1,   184,   103,   104,   105,   106,   107,
     108,    -1,   186,    -1,    -1,    -1,    -1,     7,    -1,     9,
     118,   119,   120,   121,    14,    -1,    -1,    17,    18,    19,
      -1,    21,    22,    -1,    -1,    -1,    -1,    -1,    28,    -1,
      -1,    -1,    32,    -1,    -1,    35,    -1,    37,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    65,    66,    67,    -1,    -1,
      -1,    71,    72,    73,    74,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     7,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    -1,    99,
      21,    -1,    -1,   103,   104,   105,   106,    28,   108,    -1,
      -1,    32,    -1,    -1,    -1,    -1,    37,    -1,   118,   119,
     120,   121,    -1,    -1,    45,    -1,     7,    48,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    17,    18,    -1,    -1,
      21,    -1,    -1,    -1,    65,    -1,    67,    28,    -1,    -1,
      71,    32,    73,    74,    -1,    -1,    37,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    99,    -1,
      -1,    -1,   103,   104,    65,   106,    67,   108,    -1,    -1,
      71,    -1,    73,    74,    -1,    -1,    -1,   118,   119,   120,
     121,    -1,    -1,    -1,    -1,     7,    -1,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    17,    18,    -1,    99,    21,
      -1,    -1,   103,   104,   105,    -1,    28,   108,    -1,    -1,
      32,    -1,    -1,    -1,    -1,    37,    -1,   118,   119,   120,
     121,    -1,    -1,    45,    -1,    -1,    48,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    65,    -1,    67,     7,    -1,     9,    71,
      -1,    73,    74,    -1,    -1,    -1,    17,    18,    -1,    -1,
      21,    -1,    -1,    -1,    -1,    -1,    -1,    28,    -1,    -1,
      -1,    32,    -1,    -1,    -1,    -1,    37,    99,    -1,    -1,
      -1,   103,   104,    -1,    45,   107,   108,    48,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   118,   119,   120,   121,
       0,    -1,    -1,    -1,    65,    -1,    67,     7,    -1,     9,
      71,    -1,    73,    74,    -1,    -1,    -1,    17,    -1,    -1,
      -1,    21,    -1,    -1,    -1,    -1,    -1,    -1,    28,    -1,
      -1,    -1,    32,    -1,    -1,    -1,    -1,    37,    99,    -1,
      -1,    -1,   103,   104,    -1,    45,    -1,   108,    48,    -1,
      -1,     7,    -1,     9,    -1,    -1,    -1,   118,   119,   120,
     121,    17,    -1,    -1,    -1,    21,    66,    67,    -1,    -1,
      -1,    71,    28,    -1,    -1,    -1,    32,    -1,    -1,    -1,
      -1,    37,    -1,     7,    -1,     9,    -1,    -1,    -1,    45,
      -1,    -1,    48,    17,    -1,    -1,    -1,    21,    98,    -1,
      -1,    -1,    -1,    -1,    28,   105,    -1,    -1,    32,    -1,
      -1,    67,     7,    37,     9,    71,    -1,    -1,    -1,    -1,
      -1,    45,    17,    -1,    48,    -1,    21,    -1,    23,    -1,
      25,    -1,    -1,    28,    29,    -1,    -1,    32,    -1,    -1,
      -1,    -1,    37,    67,     7,    -1,     9,    71,    -1,    -1,
      45,   107,    -1,    48,    17,    -1,    -1,    -1,    21,    -1,
      -1,    -1,    -1,    -1,    -1,    28,    -1,    -1,    -1,    32,
      -1,    -1,    67,    -1,    37,    -1,    71,    -1,    -1,    -1,
      -1,    -1,    45,   107,    -1,    48,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    67,    -1,    -1,    -1,    71
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   130,     0,     7,     9,    17,    21,    28,    32,    37,
      45,    48,    66,    67,    71,    98,   105,   131,   132,   133,
     134,   135,   139,   140,   152,   153,   156,   161,   161,   161,
     161,   161,   161,   161,   161,   161,   161,   133,   161,   161,
     161,   102,    99,   100,   101,   136,   157,   158,   159,   105,
     106,   141,    26,    38,    39,    40,    41,    42,    50,    68,
      70,   100,   150,   151,   136,   154,   155,   150,   115,   160,
     105,   111,   107,   142,   112,   100,   106,   136,   112,    99,
     101,   143,   158,   160,   105,   111,   154,    18,    65,    73,
      74,    99,   103,   104,   108,   118,   119,   120,   121,   161,
     165,   173,   174,   175,   176,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,   191,   110,
     144,   159,    14,    19,    22,    35,    72,   105,   141,   152,
     161,   166,   167,   168,   169,   171,   172,   191,    50,   151,
     137,   106,   151,   108,   105,   155,   105,   176,   176,   161,
     191,   150,    73,    74,   115,   117,    79,    81,    86,    87,
      88,    89,    90,    91,    92,    93,   114,   190,   176,   122,
     123,   124,   118,   119,    78,    80,    83,    84,   112,   113,
      77,    85,   125,   126,   127,    75,    76,   128,   111,   116,
     136,   114,   167,   108,   108,   191,   108,   150,   107,   167,
     105,   100,   111,   107,   135,   138,   161,   137,   111,   142,
     145,   150,   109,   108,   191,   136,   189,   176,   176,   176,
     176,   178,   178,   179,   179,   180,   180,   180,   180,   181,
     181,   182,   183,   184,   185,   186,   191,   189,   106,   162,
     163,   189,    72,   142,   191,   105,   191,   104,   158,    50,
     150,   107,   104,   146,   147,   148,   109,   160,   163,   164,
     116,   110,   164,   108,   156,   161,   172,   109,   109,   111,
     105,   158,   113,   111,    23,    25,    29,   149,   161,   144,
     109,   109,   111,   189,   107,   111,   191,   172,   172,   167,
     170,   167,   104,   105,   147,   150,   176,   163,   107,   109,
     191,   191,    16,   113,   136,   105,   109,   109,   167,   144,
     167,   167
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule); \
} while (YYID (0))

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
#ifndef	YYINITDEPTH
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
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
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
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
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
	    /* Fall through.  */
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

  return yystpcpy (yyres, yystr) - yyres;
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
  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  YYSIZE_T yysize1;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = 0;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
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
                yysize1 = yysize + yytnamerr (0, yytname[yyx]);
                if (! (yysize <= yysize1
                       && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                  return 2;
                yysize = yysize1;
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  yysize1 = yysize + yystrlen (yyformat);
  if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
    return 2;
  yysize = yysize1;

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

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Location data for the lookahead symbol.  */
YYLTYPE yylloc;

/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.
       `yyls': related to locations.

       Refer to the stacks thru separate pointers, to allow yyoverflow
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
  int yytoken;
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

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;

#if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 1;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
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

	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
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
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

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
      yychar = YYLEX;
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
  *++yyvsp = yylval;
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
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1806 of yacc.c  */
#line 1022 "hlsl.y"
    {
                            }
    break;

  case 3:

/* Line 1806 of yacc.c  */
#line 1025 "hlsl.y"
    {
                                const struct hlsl_ir_function_decl *decl;

                                decl = get_overloaded_func(&hlsl_ctx.functions, (yyvsp[(2) - (2)].function).name, (yyvsp[(2) - (2)].function).decl->parameters, TRUE);
                                if (decl && !decl->func->intrinsic)
                                {
                                    if (decl->body && (yyvsp[(2) - (2)].function).decl->body)
                                    {
                                        hlsl_report_message((yyvsp[(2) - (2)].function).decl->node.loc.file, (yyvsp[(2) - (2)].function).decl->node.loc.line,
                                                (yyvsp[(2) - (2)].function).decl->node.loc.col, HLSL_LEVEL_ERROR,
                                                "redefinition of function %s", debugstr_a((yyvsp[(2) - (2)].function).name));
                                        return 1;
                                    }
                                    else if (!compare_hlsl_types(decl->node.data_type, (yyvsp[(2) - (2)].function).decl->node.data_type))
                                    {
                                        hlsl_report_message((yyvsp[(2) - (2)].function).decl->node.loc.file, (yyvsp[(2) - (2)].function).decl->node.loc.line,
                                                (yyvsp[(2) - (2)].function).decl->node.loc.col, HLSL_LEVEL_ERROR,
                                                "redefining function %s with a different return type",
                                                debugstr_a((yyvsp[(2) - (2)].function).name));
                                        hlsl_report_message(decl->node.loc.file, decl->node.loc.line, decl->node.loc.col, HLSL_LEVEL_NOTE,
                                                "%s previously declared here",
                                                debugstr_a((yyvsp[(2) - (2)].function).name));
                                        return 1;
                                    }
                                }

                                if ((yyvsp[(2) - (2)].function).decl->node.data_type->base_type == HLSL_TYPE_VOID && (yyvsp[(2) - (2)].function).decl->semantic)
                                {
                                    hlsl_report_message((yyvsp[(2) - (2)].function).decl->node.loc.file, (yyvsp[(2) - (2)].function).decl->node.loc.line,
                                            (yyvsp[(2) - (2)].function).decl->node.loc.col, HLSL_LEVEL_ERROR,
                                            "void function with a semantic");
                                }

                                TRACE("Adding function '%s' to the function list.\n", (yyvsp[(2) - (2)].function).name);
                                add_function_decl(&hlsl_ctx.functions, (yyvsp[(2) - (2)].function).name, (yyvsp[(2) - (2)].function).decl, FALSE);
                            }
    break;

  case 4:

/* Line 1806 of yacc.c  */
#line 1062 "hlsl.y"
    {
                                TRACE("Declaration statement parsed.\n");
                            }
    break;

  case 5:

/* Line 1806 of yacc.c  */
#line 1066 "hlsl.y"
    {
                            }
    break;

  case 6:

/* Line 1806 of yacc.c  */
#line 1069 "hlsl.y"
    {
                                TRACE("Skipping stray semicolon.\n");
                            }
    break;

  case 7:

/* Line 1806 of yacc.c  */
#line 1074 "hlsl.y"
    {
                                TRACE("Updating line information to file %s, line %u\n", debugstr_a((yyvsp[(2) - (2)].name)), (yyvsp[(1) - (2)].intval));
                                hlsl_ctx.line_no = (yyvsp[(1) - (2)].intval);
                                if (strcmp((yyvsp[(2) - (2)].name), hlsl_ctx.source_file))
                                {
                                    const char **new_array;

                                    hlsl_ctx.source_file = (yyvsp[(2) - (2)].name);
                                    new_array = d3dcompiler_realloc(hlsl_ctx.source_files,
                                            sizeof(*hlsl_ctx.source_files) * hlsl_ctx.source_files_count + 1);
                                    if (new_array)
                                    {
                                        hlsl_ctx.source_files = new_array;
                                        hlsl_ctx.source_files[hlsl_ctx.source_files_count++] = (yyvsp[(2) - (2)].name);
                                    }
                                }
                            }
    break;

  case 8:

/* Line 1806 of yacc.c  */
#line 1093 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(3) - (3)]));
                                if (!(yyvsp[(2) - (3)].list))
                                {
                                    if (!(yyvsp[(1) - (3)].type)->name)
                                    {
                                        hlsl_report_message(loc.file, loc.line, loc.col,
                                                HLSL_LEVEL_ERROR, "anonymous struct declaration with no variables");
                                    }
                                    check_type_modifiers((yyvsp[(1) - (3)].type)->modifiers, &loc);
                                }
                                (yyval.list) = declare_vars((yyvsp[(1) - (3)].type), 0, (yyvsp[(2) - (3)].list));
                            }
    break;

  case 11:

/* Line 1806 of yacc.c  */
#line 1113 "hlsl.y"
    {
                                BOOL ret;
                                struct source_location loc;

                                TRACE("Structure %s declaration.\n", debugstr_a((yyvsp[(3) - (6)].name)));
                                set_location(&loc, &(yylsp[(1) - (6)]));
                                check_invalid_matrix_modifiers((yyvsp[(1) - (6)].modifiers), &loc);
                                (yyval.type) = new_struct_type((yyvsp[(3) - (6)].name), (yyvsp[(1) - (6)].modifiers), (yyvsp[(5) - (6)].list));

                                if (get_variable(hlsl_ctx.cur_scope, (yyvsp[(3) - (6)].name)))
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, (yylsp[(3) - (6)]).first_line, (yylsp[(3) - (6)]).first_column,
                                            HLSL_LEVEL_ERROR, "redefinition of '%s'", (yyvsp[(3) - (6)].name));
                                    return 1;
                                }

                                ret = add_type_to_scope(hlsl_ctx.cur_scope, (yyval.type));
                                if (!ret)
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, (yylsp[(3) - (6)]).first_line, (yylsp[(3) - (6)]).first_column,
                                            HLSL_LEVEL_ERROR, "redefinition of struct '%s'", (yyvsp[(3) - (6)].name));
                                    return 1;
                                }
                            }
    break;

  case 12:

/* Line 1806 of yacc.c  */
#line 1139 "hlsl.y"
    {
                                struct source_location loc;

                                TRACE("Anonymous structure declaration.\n");
                                set_location(&loc, &(yylsp[(1) - (5)]));
                                check_invalid_matrix_modifiers((yyvsp[(1) - (5)].modifiers), &loc);
                                (yyval.type) = new_struct_type(NULL, (yyvsp[(1) - (5)].modifiers), (yyvsp[(4) - (5)].list));
                            }
    break;

  case 16:

/* Line 1806 of yacc.c  */
#line 1153 "hlsl.y"
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                            }
    break;

  case 17:

/* Line 1806 of yacc.c  */
#line 1158 "hlsl.y"
    {
                                BOOL ret;
                                struct hlsl_struct_field *field, *next;

                                (yyval.list) = (yyvsp[(1) - (2)].list);
                                LIST_FOR_EACH_ENTRY_SAFE(field, next, (yyvsp[(2) - (2)].list), struct hlsl_struct_field, entry)
                                {
                                    ret = add_struct_field((yyval.list), field);
                                    if (ret == FALSE)
                                    {
                                        hlsl_report_message(hlsl_ctx.source_file, (yylsp[(2) - (2)]).first_line, (yylsp[(2) - (2)]).first_column,
                                                HLSL_LEVEL_ERROR, "redefinition of '%s'", field->name);
                                        d3dcompiler_free(field);
                                    }
                                }
                                d3dcompiler_free((yyvsp[(2) - (2)].list));
                            }
    break;

  case 18:

/* Line 1806 of yacc.c  */
#line 1177 "hlsl.y"
    {
                                (yyval.list) = gen_struct_fields((yyvsp[(2) - (4)].type), (yyvsp[(1) - (4)].modifiers), (yyvsp[(3) - (4)].list));
                            }
    break;

  case 19:

/* Line 1806 of yacc.c  */
#line 1181 "hlsl.y"
    {
                                (yyval.list) = gen_struct_fields((yyvsp[(1) - (3)].type), 0, (yyvsp[(2) - (3)].list));
                            }
    break;

  case 20:

/* Line 1806 of yacc.c  */
#line 1186 "hlsl.y"
    {
                                TRACE("Function %s parsed.\n", (yyvsp[(1) - (2)].function).name);
                                (yyval.function) = (yyvsp[(1) - (2)].function);
                                (yyval.function).decl->body = (yyvsp[(2) - (2)].list);
                                pop_scope(&hlsl_ctx);
                            }
    break;

  case 21:

/* Line 1806 of yacc.c  */
#line 1193 "hlsl.y"
    {
                                TRACE("Function prototype for %s.\n", (yyvsp[(1) - (2)].function).name);
                                (yyval.function) = (yyvsp[(1) - (2)].function);
                                pop_scope(&hlsl_ctx);
                            }
    break;

  case 22:

/* Line 1806 of yacc.c  */
#line 1200 "hlsl.y"
    {
                                if (get_variable(hlsl_ctx.globals, (yyvsp[(3) - (7)].name)))
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, (yylsp[(3) - (7)]).first_line, (yylsp[(3) - (7)]).first_column,
                                            HLSL_LEVEL_ERROR, "redefinition of '%s'\n", (yyvsp[(3) - (7)].name));
                                    return 1;
                                }
                                if ((yyvsp[(2) - (7)].type)->base_type == HLSL_TYPE_VOID && (yyvsp[(7) - (7)].name))
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, (yylsp[(7) - (7)]).first_line, (yylsp[(7) - (7)]).first_column,
                                            HLSL_LEVEL_ERROR, "void function with a semantic");
                                }

                                (yyval.function).decl = new_func_decl((yyvsp[(2) - (7)].type), (yyvsp[(5) - (7)].list));
                                if (!(yyval.function).decl)
                                {
                                    ERR("Out of memory.\n");
                                    return -1;
                                }
                                (yyval.function).name = (yyvsp[(3) - (7)].name);
                                (yyval.function).decl->semantic = (yyvsp[(7) - (7)].name);
                                set_location(&(yyval.function).decl->node.loc, &(yylsp[(3) - (7)]));
                            }
    break;

  case 23:

/* Line 1806 of yacc.c  */
#line 1225 "hlsl.y"
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                            }
    break;

  case 24:

/* Line 1806 of yacc.c  */
#line 1230 "hlsl.y"
    {
                                pop_scope(&hlsl_ctx);
                                (yyval.list) = (yyvsp[(3) - (4)].list);
                            }
    break;

  case 25:

/* Line 1806 of yacc.c  */
#line 1236 "hlsl.y"
    {
                                push_scope(&hlsl_ctx);
                            }
    break;

  case 28:

/* Line 1806 of yacc.c  */
#line 1244 "hlsl.y"
    {
                                (yyval.name) = NULL;
                            }
    break;

  case 29:

/* Line 1806 of yacc.c  */
#line 1248 "hlsl.y"
    {
                                (yyval.name) = (yyvsp[(2) - (2)].name);
                            }
    break;

  case 30:

/* Line 1806 of yacc.c  */
#line 1253 "hlsl.y"
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                            }
    break;

  case 31:

/* Line 1806 of yacc.c  */
#line 1258 "hlsl.y"
    {
                                (yyval.list) = (yyvsp[(2) - (2)].list);
                            }
    break;

  case 32:

/* Line 1806 of yacc.c  */
#line 1263 "hlsl.y"
    {
                                struct source_location loc;

                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                set_location(&loc, &(yylsp[(1) - (1)]));
                                if (!add_func_parameter((yyval.list), &(yyvsp[(1) - (1)].parameter), &loc))
                                {
                                    ERR("Error adding function parameter %s.\n", (yyvsp[(1) - (1)].parameter).name);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return -1;
                                }
                            }
    break;

  case 33:

/* Line 1806 of yacc.c  */
#line 1277 "hlsl.y"
    {
                                struct source_location loc;

                                (yyval.list) = (yyvsp[(1) - (3)].list);
                                set_location(&loc, &(yylsp[(3) - (3)]));
                                if (!add_func_parameter((yyval.list), &(yyvsp[(3) - (3)].parameter), &loc))
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "duplicate parameter %s", (yyvsp[(3) - (3)].parameter).name);
                                    return 1;
                                }
                            }
    break;

  case 34:

/* Line 1806 of yacc.c  */
#line 1291 "hlsl.y"
    {
                                (yyval.parameter).modifiers = (yyvsp[(1) - (5)].modifiers) ? (yyvsp[(1) - (5)].modifiers) : HLSL_MODIFIER_IN;
                                (yyval.parameter).modifiers |= (yyvsp[(2) - (5)].modifiers);
                                (yyval.parameter).type = (yyvsp[(3) - (5)].type);
                                (yyval.parameter).name = (yyvsp[(4) - (5)].name);
                                (yyval.parameter).semantic = (yyvsp[(5) - (5)].name);
                            }
    break;

  case 35:

/* Line 1806 of yacc.c  */
#line 1300 "hlsl.y"
    {
                                (yyval.modifiers) = 0;
                            }
    break;

  case 36:

/* Line 1806 of yacc.c  */
#line 1304 "hlsl.y"
    {
                                if ((yyvsp[(1) - (2)].modifiers) & (yyvsp[(2) - (2)].modifiers))
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, (yylsp[(2) - (2)]).first_line, (yylsp[(2) - (2)]).first_column,
                                            HLSL_LEVEL_ERROR, "duplicate input-output modifiers");
                                    return 1;
                                }
                                (yyval.modifiers) = (yyvsp[(1) - (2)].modifiers) | (yyvsp[(2) - (2)].modifiers);
                            }
    break;

  case 37:

/* Line 1806 of yacc.c  */
#line 1315 "hlsl.y"
    {
                                (yyval.modifiers) = HLSL_MODIFIER_IN;
                            }
    break;

  case 38:

/* Line 1806 of yacc.c  */
#line 1319 "hlsl.y"
    {
                                (yyval.modifiers) = HLSL_MODIFIER_OUT;
                            }
    break;

  case 39:

/* Line 1806 of yacc.c  */
#line 1323 "hlsl.y"
    {
                                (yyval.modifiers) = HLSL_MODIFIER_IN | HLSL_MODIFIER_OUT;
                            }
    break;

  case 40:

/* Line 1806 of yacc.c  */
#line 1328 "hlsl.y"
    {
                                (yyval.type) = (yyvsp[(1) - (1)].type);
                            }
    break;

  case 41:

/* Line 1806 of yacc.c  */
#line 1332 "hlsl.y"
    {
                                if ((yyvsp[(3) - (6)].type)->type != HLSL_CLASS_SCALAR)
                                {
                                    hlsl_message("Line %u: vectors of non-scalar types are not allowed.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }
                                if ((yyvsp[(5) - (6)].intval) < 1 || (yyvsp[(5) - (6)].intval) > 4)
                                {
                                    hlsl_message("Line %u: vector size must be between 1 and 4.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }

                                (yyval.type) = new_hlsl_type(NULL, HLSL_CLASS_VECTOR, (yyvsp[(3) - (6)].type)->base_type, (yyvsp[(5) - (6)].intval), 1);
                            }
    break;

  case 42:

/* Line 1806 of yacc.c  */
#line 1351 "hlsl.y"
    {
                                if ((yyvsp[(3) - (8)].type)->type != HLSL_CLASS_SCALAR)
                                {
                                    hlsl_message("Line %u: matrices of non-scalar types are not allowed.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }
                                if ((yyvsp[(5) - (8)].intval) < 1 || (yyvsp[(5) - (8)].intval) > 4 || (yyvsp[(7) - (8)].intval) < 1 || (yyvsp[(7) - (8)].intval) > 4)
                                {
                                    hlsl_message("Line %u: matrix dimensions must be between 1 and 4.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }

                                (yyval.type) = new_hlsl_type(NULL, HLSL_CLASS_MATRIX, (yyvsp[(3) - (8)].type)->base_type, (yyvsp[(5) - (8)].intval), (yyvsp[(7) - (8)].intval));
                            }
    break;

  case 43:

/* Line 1806 of yacc.c  */
#line 1371 "hlsl.y"
    {
                                (yyval.type) = new_hlsl_type(d3dcompiler_strdup("void"), HLSL_CLASS_OBJECT, HLSL_TYPE_VOID, 1, 1);
                            }
    break;

  case 44:

/* Line 1806 of yacc.c  */
#line 1375 "hlsl.y"
    {
                                (yyval.type) = new_hlsl_type(d3dcompiler_strdup("sampler"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                (yyval.type)->sampler_dim = HLSL_SAMPLER_DIM_GENERIC;
                            }
    break;

  case 45:

/* Line 1806 of yacc.c  */
#line 1380 "hlsl.y"
    {
                                (yyval.type) = new_hlsl_type(d3dcompiler_strdup("sampler1D"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                (yyval.type)->sampler_dim = HLSL_SAMPLER_DIM_1D;
                            }
    break;

  case 46:

/* Line 1806 of yacc.c  */
#line 1385 "hlsl.y"
    {
                                (yyval.type) = new_hlsl_type(d3dcompiler_strdup("sampler2D"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                (yyval.type)->sampler_dim = HLSL_SAMPLER_DIM_2D;
                            }
    break;

  case 47:

/* Line 1806 of yacc.c  */
#line 1390 "hlsl.y"
    {
                                (yyval.type) = new_hlsl_type(d3dcompiler_strdup("sampler3D"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                (yyval.type)->sampler_dim = HLSL_SAMPLER_DIM_3D;
                            }
    break;

  case 48:

/* Line 1806 of yacc.c  */
#line 1395 "hlsl.y"
    {
                                (yyval.type) = new_hlsl_type(d3dcompiler_strdup("samplerCUBE"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                (yyval.type)->sampler_dim = HLSL_SAMPLER_DIM_CUBE;
                            }
    break;

  case 49:

/* Line 1806 of yacc.c  */
#line 1400 "hlsl.y"
    {
                                struct hlsl_type *type;

                                TRACE("Type %s.\n", (yyvsp[(1) - (1)].name));
                                type = get_type(hlsl_ctx.cur_scope, (yyvsp[(1) - (1)].name), TRUE);
                                (yyval.type) = type;
                                d3dcompiler_free((yyvsp[(1) - (1)].name));
                            }
    break;

  case 50:

/* Line 1806 of yacc.c  */
#line 1409 "hlsl.y"
    {
                                struct hlsl_type *type;

                                TRACE("Struct type %s.\n", (yyvsp[(2) - (2)].name));
                                type = get_type(hlsl_ctx.cur_scope, (yyvsp[(2) - (2)].name), TRUE);
                                if (type->type != HLSL_CLASS_STRUCT)
                                {
                                    hlsl_message("Line %u: redefining %s as a structure.\n",
                                            hlsl_ctx.line_no, (yyvsp[(2) - (2)].name));
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                }
                                else
                                {
                                    (yyval.type) = type;
                                }
                                d3dcompiler_free((yyvsp[(2) - (2)].name));
                            }
    break;

  case 53:

/* Line 1806 of yacc.c  */
#line 1430 "hlsl.y"
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                if (!(yyval.list))
                                {
                                    ERR("Out of memory\n");
                                    return -1;
                                }
                                list_init((yyval.list));
                            }
    break;

  case 54:

/* Line 1806 of yacc.c  */
#line 1441 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(1) - (5)]));
                                if (!add_typedef((yyvsp[(2) - (5)].modifiers), (yyvsp[(3) - (5)].type), (yyvsp[(4) - (5)].list), &loc))
                                    return 1;
                            }
    break;

  case 55:

/* Line 1806 of yacc.c  */
#line 1449 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(1) - (4)]));
                                if (!add_typedef(0, (yyvsp[(2) - (4)].type), (yyvsp[(3) - (4)].list), &loc))
                                    return 1;
                            }
    break;

  case 56:

/* Line 1806 of yacc.c  */
#line 1458 "hlsl.y"
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                list_add_head((yyval.list), &(yyvsp[(1) - (1)].variable_def)->entry);
                            }
    break;

  case 57:

/* Line 1806 of yacc.c  */
#line 1464 "hlsl.y"
    {
                                (yyval.list) = (yyvsp[(1) - (3)].list);
                                list_add_tail((yyval.list), &(yyvsp[(3) - (3)].variable_def)->entry);
                            }
    break;

  case 58:

/* Line 1806 of yacc.c  */
#line 1470 "hlsl.y"
    {
                                (yyval.variable_def) = d3dcompiler_alloc(sizeof(*(yyval.variable_def)));
                                set_location(&(yyval.variable_def)->loc, &(yylsp[(1) - (2)]));
                                (yyval.variable_def)->name = (yyvsp[(1) - (2)].name);
                                (yyval.variable_def)->array_size = (yyvsp[(2) - (2)].intval);
                            }
    break;

  case 59:

/* Line 1806 of yacc.c  */
#line 1478 "hlsl.y"
    {
                                (yyval.list) = declare_vars((yyvsp[(2) - (4)].type), (yyvsp[(1) - (4)].modifiers), (yyvsp[(3) - (4)].list));
                            }
    break;

  case 60:

/* Line 1806 of yacc.c  */
#line 1483 "hlsl.y"
    {
                                (yyval.list) = NULL;
                            }
    break;

  case 61:

/* Line 1806 of yacc.c  */
#line 1487 "hlsl.y"
    {
                                (yyval.list) = (yyvsp[(1) - (1)].list);
                            }
    break;

  case 62:

/* Line 1806 of yacc.c  */
#line 1492 "hlsl.y"
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                list_add_head((yyval.list), &(yyvsp[(1) - (1)].variable_def)->entry);
                            }
    break;

  case 63:

/* Line 1806 of yacc.c  */
#line 1498 "hlsl.y"
    {
                                (yyval.list) = (yyvsp[(1) - (3)].list);
                                list_add_tail((yyval.list), &(yyvsp[(3) - (3)].variable_def)->entry);
                            }
    break;

  case 64:

/* Line 1806 of yacc.c  */
#line 1504 "hlsl.y"
    {
                                (yyval.variable_def) = d3dcompiler_alloc(sizeof(*(yyval.variable_def)));
                                set_location(&(yyval.variable_def)->loc, &(yylsp[(1) - (3)]));
                                (yyval.variable_def)->name = (yyvsp[(1) - (3)].name);
                                (yyval.variable_def)->array_size = (yyvsp[(2) - (3)].intval);
                                (yyval.variable_def)->semantic = (yyvsp[(3) - (3)].name);
                            }
    break;

  case 65:

/* Line 1806 of yacc.c  */
#line 1512 "hlsl.y"
    {
                                TRACE("Declaration with initializer.\n");
                                (yyval.variable_def) = d3dcompiler_alloc(sizeof(*(yyval.variable_def)));
                                set_location(&(yyval.variable_def)->loc, &(yylsp[(1) - (5)]));
                                (yyval.variable_def)->name = (yyvsp[(1) - (5)].name);
                                (yyval.variable_def)->array_size = (yyvsp[(2) - (5)].intval);
                                (yyval.variable_def)->semantic = (yyvsp[(3) - (5)].name);
                                (yyval.variable_def)->initializer = (yyvsp[(5) - (5)].list);
                            }
    break;

  case 66:

/* Line 1806 of yacc.c  */
#line 1523 "hlsl.y"
    {
                                (yyval.intval) = 0;
                            }
    break;

  case 67:

/* Line 1806 of yacc.c  */
#line 1527 "hlsl.y"
    {
                                FIXME("Array.\n");
                                (yyval.intval) = 0;
                                free_instr((yyvsp[(2) - (3)].instr));
                            }
    break;

  case 68:

/* Line 1806 of yacc.c  */
#line 1534 "hlsl.y"
    {
                                (yyval.modifiers) = 0;
                            }
    break;

  case 69:

/* Line 1806 of yacc.c  */
#line 1538 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[(2) - (2)].modifiers), HLSL_STORAGE_EXTERN, &(yylsp[(1) - (2)]));
                            }
    break;

  case 70:

/* Line 1806 of yacc.c  */
#line 1542 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[(2) - (2)].modifiers), HLSL_STORAGE_NOINTERPOLATION, &(yylsp[(1) - (2)]));
                            }
    break;

  case 71:

/* Line 1806 of yacc.c  */
#line 1546 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[(2) - (2)].modifiers), HLSL_MODIFIER_PRECISE, &(yylsp[(1) - (2)]));
                            }
    break;

  case 72:

/* Line 1806 of yacc.c  */
#line 1550 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[(2) - (2)].modifiers), HLSL_STORAGE_SHARED, &(yylsp[(1) - (2)]));
                            }
    break;

  case 73:

/* Line 1806 of yacc.c  */
#line 1554 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[(2) - (2)].modifiers), HLSL_STORAGE_GROUPSHARED, &(yylsp[(1) - (2)]));
                            }
    break;

  case 74:

/* Line 1806 of yacc.c  */
#line 1558 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[(2) - (2)].modifiers), HLSL_STORAGE_STATIC, &(yylsp[(1) - (2)]));
                            }
    break;

  case 75:

/* Line 1806 of yacc.c  */
#line 1562 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[(2) - (2)].modifiers), HLSL_STORAGE_UNIFORM, &(yylsp[(1) - (2)]));
                            }
    break;

  case 76:

/* Line 1806 of yacc.c  */
#line 1566 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[(2) - (2)].modifiers), HLSL_STORAGE_VOLATILE, &(yylsp[(1) - (2)]));
                            }
    break;

  case 77:

/* Line 1806 of yacc.c  */
#line 1570 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[(2) - (2)].modifiers), HLSL_MODIFIER_CONST, &(yylsp[(1) - (2)]));
                            }
    break;

  case 78:

/* Line 1806 of yacc.c  */
#line 1574 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[(2) - (2)].modifiers), HLSL_MODIFIER_ROW_MAJOR, &(yylsp[(1) - (2)]));
                            }
    break;

  case 79:

/* Line 1806 of yacc.c  */
#line 1578 "hlsl.y"
    {
                                (yyval.modifiers) = add_modifier((yyvsp[(2) - (2)].modifiers), HLSL_MODIFIER_COLUMN_MAJOR, &(yylsp[(1) - (2)]));
                            }
    break;

  case 80:

/* Line 1806 of yacc.c  */
#line 1583 "hlsl.y"
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                list_add_head((yyval.list), &(yyvsp[(1) - (1)].instr)->entry);
                            }
    break;

  case 81:

/* Line 1806 of yacc.c  */
#line 1589 "hlsl.y"
    {
                                (yyval.list) = (yyvsp[(2) - (3)].list);
                            }
    break;

  case 82:

/* Line 1806 of yacc.c  */
#line 1593 "hlsl.y"
    {
                                (yyval.list) = (yyvsp[(2) - (4)].list);
                            }
    break;

  case 83:

/* Line 1806 of yacc.c  */
#line 1598 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[(1) - (1)].instr);
                            }
    break;

  case 84:

/* Line 1806 of yacc.c  */
#line 1603 "hlsl.y"
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                list_add_head((yyval.list), &(yyvsp[(1) - (1)].instr)->entry);
                            }
    break;

  case 85:

/* Line 1806 of yacc.c  */
#line 1609 "hlsl.y"
    {
                                (yyval.list) = (yyvsp[(1) - (3)].list);
                                list_add_tail((yyval.list), &(yyvsp[(3) - (3)].instr)->entry);
                            }
    break;

  case 86:

/* Line 1806 of yacc.c  */
#line 1615 "hlsl.y"
    {
                                (yyval.boolval) = TRUE;
                            }
    break;

  case 87:

/* Line 1806 of yacc.c  */
#line 1619 "hlsl.y"
    {
                                (yyval.boolval) = FALSE;
                            }
    break;

  case 88:

/* Line 1806 of yacc.c  */
#line 1624 "hlsl.y"
    {
                                (yyval.list) = (yyvsp[(1) - (1)].list);
                            }
    break;

  case 89:

/* Line 1806 of yacc.c  */
#line 1628 "hlsl.y"
    {
                                (yyval.list) = (yyvsp[(1) - (2)].list);
                                list_move_tail((yyval.list), (yyvsp[(2) - (2)].list));
                                d3dcompiler_free((yyvsp[(2) - (2)].list));
                            }
    break;

  case 96:

/* Line 1806 of yacc.c  */
#line 1643 "hlsl.y"
    {
                                struct hlsl_ir_jump *jump = d3dcompiler_alloc(sizeof(*jump));
                                if (!jump)
                                {
                                    ERR("Out of memory\n");
                                    return -1;
                                }
                                jump->node.type = HLSL_IR_JUMP;
                                set_location(&jump->node.loc, &(yylsp[(1) - (3)]));
                                jump->type = HLSL_IR_JUMP_RETURN;
                                jump->node.data_type = (yyvsp[(2) - (3)].instr)->data_type;
                                jump->return_value = (yyvsp[(2) - (3)].instr);

                                FIXME("Check for valued return on void function.\n");
                                FIXME("Implicit conversion to the return type if needed, "
				        "error out if conversion not possible.\n");

                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                list_add_tail((yyval.list), &jump->node.entry);
                            }
    break;

  case 97:

/* Line 1806 of yacc.c  */
#line 1666 "hlsl.y"
    {
                                struct hlsl_ir_if *instr = d3dcompiler_alloc(sizeof(*instr));
                                if (!instr)
                                {
                                    ERR("Out of memory\n");
                                    return -1;
                                }
                                instr->node.type = HLSL_IR_IF;
                                set_location(&instr->node.loc, &(yylsp[(1) - (5)]));
                                instr->condition = (yyvsp[(3) - (5)].instr);
                                instr->then_instrs = (yyvsp[(5) - (5)].if_body).then_instrs;
                                instr->else_instrs = (yyvsp[(5) - (5)].if_body).else_instrs;
                                if ((yyvsp[(3) - (5)].instr)->data_type->dimx > 1 || (yyvsp[(3) - (5)].instr)->data_type->dimy > 1)
                                {
                                    hlsl_report_message(instr->node.loc.file, instr->node.loc.line,
                                            instr->node.loc.col, HLSL_LEVEL_ERROR,
                                            "if condition requires a scalar");
                                }
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                list_add_head((yyval.list), &instr->node.entry);
                            }
    break;

  case 98:

/* Line 1806 of yacc.c  */
#line 1690 "hlsl.y"
    {
                                (yyval.if_body).then_instrs = (yyvsp[(1) - (1)].list);
                                (yyval.if_body).else_instrs = NULL;
                            }
    break;

  case 99:

/* Line 1806 of yacc.c  */
#line 1695 "hlsl.y"
    {
                                (yyval.if_body).then_instrs = (yyvsp[(1) - (3)].list);
                                (yyval.if_body).else_instrs = (yyvsp[(3) - (3)].list);
                            }
    break;

  case 100:

/* Line 1806 of yacc.c  */
#line 1701 "hlsl.y"
    {
                                struct source_location loc;
                                struct list *cond = d3dcompiler_alloc(sizeof(*cond));

                                if (!cond)
                                {
                                    ERR("Out of memory.\n");
                                    return -1;
                                }
                                list_init(cond);
                                list_add_head(cond, &(yyvsp[(3) - (5)].instr)->entry);
                                set_location(&loc, &(yylsp[(1) - (5)]));
                                (yyval.list) = create_loop(LOOP_WHILE, NULL, cond, NULL, (yyvsp[(5) - (5)].list), &loc);
                            }
    break;

  case 101:

/* Line 1806 of yacc.c  */
#line 1716 "hlsl.y"
    {
                                struct source_location loc;
                                struct list *cond = d3dcompiler_alloc(sizeof(*cond));

                                if (!cond)
                                {
                                    ERR("Out of memory.\n");
                                    return -1;
                                }
                                list_init(cond);
                                list_add_head(cond, &(yyvsp[(5) - (7)].instr)->entry);
                                set_location(&loc, &(yylsp[(1) - (7)]));
                                (yyval.list) = create_loop(LOOP_DO_WHILE, NULL, cond, NULL, (yyvsp[(2) - (7)].list), &loc);
                            }
    break;

  case 102:

/* Line 1806 of yacc.c  */
#line 1731 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(1) - (8)]));
                                (yyval.list) = create_loop(LOOP_FOR, (yyvsp[(4) - (8)].list), (yyvsp[(5) - (8)].list), (yyvsp[(6) - (8)].instr), (yyvsp[(8) - (8)].list), &loc);
                                pop_scope(&hlsl_ctx);
                            }
    break;

  case 103:

/* Line 1806 of yacc.c  */
#line 1739 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(1) - (8)]));
                                if (!(yyvsp[(4) - (8)].list))
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_WARNING,
                                            "no expressions in for loop initializer");
                                (yyval.list) = create_loop(LOOP_FOR, (yyvsp[(4) - (8)].list), (yyvsp[(5) - (8)].list), (yyvsp[(6) - (8)].instr), (yyvsp[(8) - (8)].list), &loc);
                                pop_scope(&hlsl_ctx);
                            }
    break;

  case 104:

/* Line 1806 of yacc.c  */
#line 1751 "hlsl.y"
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                            }
    break;

  case 105:

/* Line 1806 of yacc.c  */
#line 1756 "hlsl.y"
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                if ((yyvsp[(1) - (2)].instr))
                                    list_add_head((yyval.list), &(yyvsp[(1) - (2)].instr)->entry);
                            }
    break;

  case 106:

/* Line 1806 of yacc.c  */
#line 1764 "hlsl.y"
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
                                c->v.value.f[0] = (yyvsp[(1) - (1)].floatval);
                                (yyval.instr) = &c->node;
                            }
    break;

  case 107:

/* Line 1806 of yacc.c  */
#line 1778 "hlsl.y"
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
                                c->v.value.i[0] = (yyvsp[(1) - (1)].intval);
                                (yyval.instr) = &c->node;
                            }
    break;

  case 108:

/* Line 1806 of yacc.c  */
#line 1792 "hlsl.y"
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
                                c->v.value.b[0] = (yyvsp[(1) - (1)].boolval);
                                (yyval.instr) = &c->node;
                            }
    break;

  case 109:

/* Line 1806 of yacc.c  */
#line 1806 "hlsl.y"
    {
                                struct hlsl_ir_deref *deref = new_var_deref((yyvsp[(1) - (1)].var));
                                if (deref)
                                {
                                    (yyval.instr) = &deref->node;
                                    set_location(&(yyval.instr)->loc, &(yylsp[(1) - (1)]));
                                }
                                else
                                    (yyval.instr) = NULL;
                            }
    break;

  case 110:

/* Line 1806 of yacc.c  */
#line 1817 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[(2) - (3)].instr);
                            }
    break;

  case 111:

/* Line 1806 of yacc.c  */
#line 1822 "hlsl.y"
    {
                                struct hlsl_ir_var *var;
                                var = get_variable(hlsl_ctx.cur_scope, (yyvsp[(1) - (1)].name));
                                if (!var)
                                {
                                    hlsl_message("Line %d: variable '%s' not declared\n",
                                            hlsl_ctx.line_no, (yyvsp[(1) - (1)].name));
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }
                                (yyval.var) = var;
                            }
    break;

  case 112:

/* Line 1806 of yacc.c  */
#line 1836 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[(1) - (1)].instr);
                            }
    break;

  case 113:

/* Line 1806 of yacc.c  */
#line 1840 "hlsl.y"
    {
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(2) - (2)]));
                                if ((yyvsp[(1) - (2)].instr)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "modifying a const expression");
                                    return 1;
                                }
                                operands[0] = (yyvsp[(1) - (2)].instr);
                                operands[1] = operands[2] = NULL;
                                (yyval.instr) = &new_expr(HLSL_IR_BINOP_POSTINC, operands, &loc)->node;
                                /* Post increment/decrement expressions are considered const */
                                (yyval.instr)->data_type = clone_hlsl_type((yyval.instr)->data_type);
                                (yyval.instr)->data_type->modifiers |= HLSL_MODIFIER_CONST;
                            }
    break;

  case 114:

/* Line 1806 of yacc.c  */
#line 1859 "hlsl.y"
    {
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(2) - (2)]));
                                if ((yyvsp[(1) - (2)].instr)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "modifying a const expression");
                                    return 1;
                                }
                                operands[0] = (yyvsp[(1) - (2)].instr);
                                operands[1] = operands[2] = NULL;
                                (yyval.instr) = &new_expr(HLSL_IR_BINOP_POSTDEC, operands, &loc)->node;
                                /* Post increment/decrement expressions are considered const */
                                (yyval.instr)->data_type = clone_hlsl_type((yyval.instr)->data_type);
                                (yyval.instr)->data_type->modifiers |= HLSL_MODIFIER_CONST;
                            }
    break;

  case 115:

/* Line 1806 of yacc.c  */
#line 1878 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(2) - (3)]));
                                if ((yyvsp[(1) - (3)].instr)->data_type->type == HLSL_CLASS_STRUCT)
                                {
                                    struct hlsl_type *type = (yyvsp[(1) - (3)].instr)->data_type;
                                    struct hlsl_struct_field *field;

                                    (yyval.instr) = NULL;
                                    LIST_FOR_EACH_ENTRY(field, type->e.elements, struct hlsl_struct_field, entry)
                                    {
                                        if (!strcmp((yyvsp[(3) - (3)].name), field->name))
                                        {
                                            struct hlsl_ir_deref *deref = new_record_deref((yyvsp[(1) - (3)].instr), field);

                                            if (!deref)
                                            {
                                                ERR("Out of memory\n");
                                                return -1;
                                            }
                                            deref->node.loc = loc;
                                            (yyval.instr) = &deref->node;
                                            break;
                                        }
                                    }
                                    if (!(yyval.instr))
                                    {
                                        hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                                "invalid subscript %s", debugstr_a((yyvsp[(3) - (3)].name)));
                                        return 1;
                                    }
                                }
                                else if ((yyvsp[(1) - (3)].instr)->data_type->type <= HLSL_CLASS_LAST_NUMERIC)
                                {
                                    struct hlsl_ir_swizzle *swizzle;

                                    swizzle = get_swizzle((yyvsp[(1) - (3)].instr), (yyvsp[(3) - (3)].name), &loc);
                                    if (!swizzle)
                                    {
                                        hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                                "invalid swizzle %s", debugstr_a((yyvsp[(3) - (3)].name)));
                                        return 1;
                                    }
                                    (yyval.instr) = &swizzle->node;
                                }
                                else
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "invalid subscript %s", debugstr_a((yyvsp[(3) - (3)].name)));
                                    return 1;
                                }
                            }
    break;

  case 116:

/* Line 1806 of yacc.c  */
#line 1932 "hlsl.y"
    {
                                /* This may be an array dereference or a vector/matrix
                                 * subcomponent access.
                                 * We store it as an array dereference in any case. */
                                struct hlsl_ir_deref *deref = d3dcompiler_alloc(sizeof(*deref));
                                struct hlsl_type *expr_type = (yyvsp[(1) - (4)].instr)->data_type;
                                struct source_location loc;

                                TRACE("Array dereference from type %s\n", debug_hlsl_type(expr_type));
                                if (!deref)
                                {
                                    ERR("Out of memory\n");
                                    return -1;
                                }
                                deref->node.type = HLSL_IR_DEREF;
                                set_location(&loc, &(yylsp[(2) - (4)]));
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
                                    free_instr((yyvsp[(1) - (4)].instr));
                                    free_instr((yyvsp[(3) - (4)].instr));
                                    return 1;
                                }
                                if ((yyvsp[(3) - (4)].instr)->data_type->type != HLSL_CLASS_SCALAR)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "array index is not scalar");
                                    d3dcompiler_free(deref);
                                    free_instr((yyvsp[(1) - (4)].instr));
                                    free_instr((yyvsp[(3) - (4)].instr));
                                    return 1;
                                }
                                deref->type = HLSL_IR_DEREF_ARRAY;
                                deref->v.array.array = (yyvsp[(1) - (4)].instr);
                                deref->v.array.index = (yyvsp[(3) - (4)].instr);

                                (yyval.instr) = &deref->node;
                            }
    break;

  case 117:

/* Line 1806 of yacc.c  */
#line 1992 "hlsl.y"
    {
                                struct hlsl_ir_constructor *constructor;

                                TRACE("%s constructor.\n", debug_hlsl_type((yyvsp[(2) - (5)].type)));
                                if ((yyvsp[(1) - (5)].modifiers))
                                {
                                    hlsl_message("Line %u: unexpected modifier in a constructor.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return -1;
                                }
                                if ((yyvsp[(2) - (5)].type)->type > HLSL_CLASS_LAST_NUMERIC)
                                {
                                    hlsl_message("Line %u: constructors are allowed only for numeric data types.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return -1;
                                }
                                if ((yyvsp[(2) - (5)].type)->dimx * (yyvsp[(2) - (5)].type)->dimy != components_count_expr_list((yyvsp[(4) - (5)].list)))
                                {
                                    hlsl_message("Line %u: wrong number of components in constructor.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return -1;
                                }

                                constructor = d3dcompiler_alloc(sizeof(*constructor));
                                constructor->node.type = HLSL_IR_CONSTRUCTOR;
                                set_location(&constructor->node.loc, &(yylsp[(3) - (5)]));
                                constructor->node.data_type = (yyvsp[(2) - (5)].type);
                                constructor->arguments = (yyvsp[(4) - (5)].list);

                                (yyval.instr) = &constructor->node;
                            }
    break;

  case 118:

/* Line 1806 of yacc.c  */
#line 2028 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[(1) - (1)].instr);
                            }
    break;

  case 119:

/* Line 1806 of yacc.c  */
#line 2032 "hlsl.y"
    {
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(1) - (2)]));
                                if ((yyvsp[(2) - (2)].instr)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "modifying a const expression");
                                    return 1;
                                }
                                operands[0] = (yyvsp[(2) - (2)].instr);
                                operands[1] = operands[2] = NULL;
                                (yyval.instr) = &new_expr(HLSL_IR_BINOP_PREINC, operands, &loc)->node;
                            }
    break;

  case 120:

/* Line 1806 of yacc.c  */
#line 2048 "hlsl.y"
    {
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(1) - (2)]));
                                if ((yyvsp[(2) - (2)].instr)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "modifying a const expression");
                                    return 1;
                                }
                                operands[0] = (yyvsp[(2) - (2)].instr);
                                operands[1] = operands[2] = NULL;
                                (yyval.instr) = &new_expr(HLSL_IR_BINOP_PREDEC, operands, &loc)->node;
                            }
    break;

  case 121:

/* Line 1806 of yacc.c  */
#line 2064 "hlsl.y"
    {
                                enum hlsl_ir_expr_op ops[] = {0, HLSL_IR_UNOP_NEG,
                                        HLSL_IR_UNOP_LOGIC_NOT, HLSL_IR_UNOP_BIT_NOT};
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                if ((yyvsp[(1) - (2)].unary_op) == UNARY_OP_PLUS)
                                {
                                    (yyval.instr) = (yyvsp[(2) - (2)].instr);
                                }
                                else
                                {
                                    operands[0] = (yyvsp[(2) - (2)].instr);
                                    operands[1] = operands[2] = NULL;
                                    set_location(&loc, &(yylsp[(1) - (2)]));
                                    (yyval.instr) = &new_expr(ops[(yyvsp[(1) - (2)].unary_op)], operands, &loc)->node;
                                }
                            }
    break;

  case 122:

/* Line 1806 of yacc.c  */
#line 2084 "hlsl.y"
    {
                                struct hlsl_ir_expr *expr;
                                struct hlsl_type *src_type = (yyvsp[(6) - (6)].instr)->data_type;
                                struct hlsl_type *dst_type;
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(3) - (6)]));
                                if ((yyvsp[(2) - (6)].modifiers))
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "unexpected modifier in a cast");
                                    return 1;
                                }

                                if ((yyvsp[(4) - (6)].intval))
                                    dst_type = new_array_type((yyvsp[(3) - (6)].type), (yyvsp[(4) - (6)].intval));
                                else
                                    dst_type = (yyvsp[(3) - (6)].type);

                                if (!compatible_data_types(src_type, dst_type))
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "can't cast from %s to %s",
                                            debug_hlsl_type(src_type), debug_hlsl_type(dst_type));
                                    return 1;
                                }

                                expr = new_cast((yyvsp[(6) - (6)].instr), dst_type, &loc);
                                (yyval.instr) = expr ? &expr->node : NULL;
                            }
    break;

  case 123:

/* Line 1806 of yacc.c  */
#line 2116 "hlsl.y"
    {
                                (yyval.unary_op) = UNARY_OP_PLUS;
                            }
    break;

  case 124:

/* Line 1806 of yacc.c  */
#line 2120 "hlsl.y"
    {
                                (yyval.unary_op) = UNARY_OP_MINUS;
                            }
    break;

  case 125:

/* Line 1806 of yacc.c  */
#line 2124 "hlsl.y"
    {
                                (yyval.unary_op) = UNARY_OP_LOGICNOT;
                            }
    break;

  case 126:

/* Line 1806 of yacc.c  */
#line 2128 "hlsl.y"
    {
                                (yyval.unary_op) = UNARY_OP_BITNOT;
                            }
    break;

  case 127:

/* Line 1806 of yacc.c  */
#line 2133 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[(1) - (1)].instr);
                            }
    break;

  case 128:

/* Line 1806 of yacc.c  */
#line 2137 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(2) - (3)]));
                                (yyval.instr) = &hlsl_mul((yyvsp[(1) - (3)].instr), (yyvsp[(3) - (3)].instr), &loc)->node;
                            }
    break;

  case 129:

/* Line 1806 of yacc.c  */
#line 2144 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(2) - (3)]));
                                (yyval.instr) = &hlsl_div((yyvsp[(1) - (3)].instr), (yyvsp[(3) - (3)].instr), &loc)->node;
                            }
    break;

  case 130:

/* Line 1806 of yacc.c  */
#line 2151 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(2) - (3)]));
                                (yyval.instr) = &hlsl_mod((yyvsp[(1) - (3)].instr), (yyvsp[(3) - (3)].instr), &loc)->node;
                            }
    break;

  case 131:

/* Line 1806 of yacc.c  */
#line 2159 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[(1) - (1)].instr);
                            }
    break;

  case 132:

/* Line 1806 of yacc.c  */
#line 2163 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(2) - (3)]));
                                (yyval.instr) = &hlsl_add((yyvsp[(1) - (3)].instr), (yyvsp[(3) - (3)].instr), &loc)->node;
                            }
    break;

  case 133:

/* Line 1806 of yacc.c  */
#line 2170 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(2) - (3)]));
                                (yyval.instr) = &hlsl_sub((yyvsp[(1) - (3)].instr), (yyvsp[(3) - (3)].instr), &loc)->node;
                            }
    break;

  case 134:

/* Line 1806 of yacc.c  */
#line 2178 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[(1) - (1)].instr);
                            }
    break;

  case 135:

/* Line 1806 of yacc.c  */
#line 2182 "hlsl.y"
    {
                                FIXME("Left shift\n");
                            }
    break;

  case 136:

/* Line 1806 of yacc.c  */
#line 2186 "hlsl.y"
    {
                                FIXME("Right shift\n");
                            }
    break;

  case 137:

/* Line 1806 of yacc.c  */
#line 2191 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[(1) - (1)].instr);
                            }
    break;

  case 138:

/* Line 1806 of yacc.c  */
#line 2195 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(2) - (3)]));
                                (yyval.instr) = &hlsl_lt((yyvsp[(1) - (3)].instr), (yyvsp[(3) - (3)].instr), &loc)->node;
                            }
    break;

  case 139:

/* Line 1806 of yacc.c  */
#line 2202 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(2) - (3)]));
                                (yyval.instr) = &hlsl_gt((yyvsp[(1) - (3)].instr), (yyvsp[(3) - (3)].instr), &loc)->node;
                            }
    break;

  case 140:

/* Line 1806 of yacc.c  */
#line 2209 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(2) - (3)]));
                                (yyval.instr) = &hlsl_le((yyvsp[(1) - (3)].instr), (yyvsp[(3) - (3)].instr), &loc)->node;
                            }
    break;

  case 141:

/* Line 1806 of yacc.c  */
#line 2216 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(2) - (3)]));
                                (yyval.instr) = &hlsl_ge((yyvsp[(1) - (3)].instr), (yyvsp[(3) - (3)].instr), &loc)->node;
                            }
    break;

  case 142:

/* Line 1806 of yacc.c  */
#line 2224 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[(1) - (1)].instr);
                            }
    break;

  case 143:

/* Line 1806 of yacc.c  */
#line 2228 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(2) - (3)]));
                                (yyval.instr) = &hlsl_eq((yyvsp[(1) - (3)].instr), (yyvsp[(3) - (3)].instr), &loc)->node;
                            }
    break;

  case 144:

/* Line 1806 of yacc.c  */
#line 2235 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(2) - (3)]));
                                (yyval.instr) = &hlsl_ne((yyvsp[(1) - (3)].instr), (yyvsp[(3) - (3)].instr), &loc)->node;
                            }
    break;

  case 145:

/* Line 1806 of yacc.c  */
#line 2243 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[(1) - (1)].instr);
                            }
    break;

  case 146:

/* Line 1806 of yacc.c  */
#line 2247 "hlsl.y"
    {
                                FIXME("bitwise AND\n");
                            }
    break;

  case 147:

/* Line 1806 of yacc.c  */
#line 2252 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[(1) - (1)].instr);
                            }
    break;

  case 148:

/* Line 1806 of yacc.c  */
#line 2256 "hlsl.y"
    {
                                FIXME("bitwise XOR\n");
                            }
    break;

  case 149:

/* Line 1806 of yacc.c  */
#line 2261 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[(1) - (1)].instr);
                            }
    break;

  case 150:

/* Line 1806 of yacc.c  */
#line 2265 "hlsl.y"
    {
                                FIXME("bitwise OR\n");
                            }
    break;

  case 151:

/* Line 1806 of yacc.c  */
#line 2270 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[(1) - (1)].instr);
                            }
    break;

  case 152:

/* Line 1806 of yacc.c  */
#line 2274 "hlsl.y"
    {
                                FIXME("logic AND\n");
                            }
    break;

  case 153:

/* Line 1806 of yacc.c  */
#line 2279 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[(1) - (1)].instr);
                            }
    break;

  case 154:

/* Line 1806 of yacc.c  */
#line 2283 "hlsl.y"
    {
                                FIXME("logic OR\n");
                            }
    break;

  case 155:

/* Line 1806 of yacc.c  */
#line 2288 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[(1) - (1)].instr);
                            }
    break;

  case 156:

/* Line 1806 of yacc.c  */
#line 2292 "hlsl.y"
    {
                                FIXME("ternary operator\n");
                            }
    break;

  case 157:

/* Line 1806 of yacc.c  */
#line 2297 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[(1) - (1)].instr);
                            }
    break;

  case 158:

/* Line 1806 of yacc.c  */
#line 2301 "hlsl.y"
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[(2) - (3)]));
                                if ((yyvsp[(1) - (3)].instr)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "l-value is const");
                                    return 1;
                                }
                                (yyval.instr) = make_assignment((yyvsp[(1) - (3)].instr), (yyvsp[(2) - (3)].assign_op), BWRITERSP_WRITEMASK_ALL, (yyvsp[(3) - (3)].instr));
                                if (!(yyval.instr))
                                    return 1;
                                (yyval.instr)->loc = loc;
                            }
    break;

  case 159:

/* Line 1806 of yacc.c  */
#line 2318 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_ASSIGN;
                            }
    break;

  case 160:

/* Line 1806 of yacc.c  */
#line 2322 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_ADD;
                            }
    break;

  case 161:

/* Line 1806 of yacc.c  */
#line 2326 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_SUB;
                            }
    break;

  case 162:

/* Line 1806 of yacc.c  */
#line 2330 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_MUL;
                            }
    break;

  case 163:

/* Line 1806 of yacc.c  */
#line 2334 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_DIV;
                            }
    break;

  case 164:

/* Line 1806 of yacc.c  */
#line 2338 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_MOD;
                            }
    break;

  case 165:

/* Line 1806 of yacc.c  */
#line 2342 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_LSHIFT;
                            }
    break;

  case 166:

/* Line 1806 of yacc.c  */
#line 2346 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_RSHIFT;
                            }
    break;

  case 167:

/* Line 1806 of yacc.c  */
#line 2350 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_AND;
                            }
    break;

  case 168:

/* Line 1806 of yacc.c  */
#line 2354 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_OR;
                            }
    break;

  case 169:

/* Line 1806 of yacc.c  */
#line 2358 "hlsl.y"
    {
                                (yyval.assign_op) = ASSIGN_OP_XOR;
                            }
    break;

  case 170:

/* Line 1806 of yacc.c  */
#line 2363 "hlsl.y"
    {
                                (yyval.instr) = (yyvsp[(1) - (1)].instr);
                            }
    break;

  case 171:

/* Line 1806 of yacc.c  */
#line 2367 "hlsl.y"
    {
                                FIXME("Comma expression\n");
                            }
    break;



/* Line 1806 of yacc.c  */
#line 4835 "hlsl.tab.c"
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

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[1] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
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
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

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

  *++yyvsp = yylval;

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

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc);
    }
  /* Do not reclaim the symbols of the rule which action triggered
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
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 2067 of yacc.c  */
#line 2371 "hlsl.y"


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

