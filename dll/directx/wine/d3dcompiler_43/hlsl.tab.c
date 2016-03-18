/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.0.2"

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

/* Copy the first part of user declarations.  */
#line 21 "hlsl.y" /* yacc.c:339  */

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


#line 959 "hlsl.tab.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif


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
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 910 "hlsl.y" /* yacc.c:355  */

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

#line 1121 "hlsl.tab.c" /* yacc.c:355  */
};
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



/* Copy the second part of user declarations.  */

#line 1150 "hlsl.tab.c" /* yacc.c:358  */

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
# elif ! defined YYSIZE_T
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

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
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
#define YYLAST   821

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  129
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  65
/* YYNRULES -- Number of rules.  */
#define YYNRULES  175
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  321

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   359

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
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
       0,  1095,  1095,  1097,  1134,  1138,  1141,  1146,  1168,  1185,
    1186,  1188,  1214,  1224,  1225,  1226,  1229,  1233,  1252,  1256,
    1261,  1268,  1275,  1305,  1310,  1317,  1321,  1322,  1325,  1329,
    1334,  1340,  1346,  1351,  1360,  1365,  1370,  1384,  1398,  1409,
    1412,  1423,  1427,  1431,  1436,  1440,  1459,  1479,  1483,  1488,
    1493,  1498,  1503,  1508,  1516,  1534,  1535,  1536,  1547,  1555,
    1564,  1570,  1576,  1584,  1590,  1593,  1598,  1604,  1610,  1619,
    1632,  1635,  1643,  1646,  1650,  1654,  1658,  1662,  1666,  1670,
    1674,  1678,  1682,  1686,  1691,  1697,  1701,  1706,  1711,  1717,
    1723,  1727,  1732,  1736,  1743,  1744,  1745,  1746,  1747,  1748,
    1751,  1774,  1798,  1803,  1809,  1824,  1839,  1847,  1859,  1864,
    1872,  1886,  1900,  1914,  1925,  1930,  1944,  1948,  1967,  1986,
    2040,  2100,  2136,  2140,  2156,  2172,  2192,  2224,  2228,  2232,
    2236,  2241,  2245,  2252,  2259,  2267,  2271,  2278,  2286,  2290,
    2294,  2299,  2303,  2310,  2317,  2324,  2332,  2336,  2343,  2351,
    2355,  2360,  2364,  2369,  2373,  2378,  2382,  2387,  2391,  2396,
    2400,  2405,  2409,  2426,  2430,  2434,  2438,  2442,  2446,  2450,
    2454,  2458,  2462,  2466,  2471,  2475
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
  "primary_expr", "variable", "postfix_expr", "unary_expr", "unary_op",
  "mul_expr", "add_expr", "shift_expr", "relational_expr", "equality_expr",
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

#define YYPACT_NINF -237

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-237)))

#define YYTABLE_NINF -35

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -237,   674,  -237,   750,   750,   750,   750,   750,   750,   750,
     750,   750,   750,   750,   750,   -50,  -237,  -237,  -237,   100,
    -237,  -237,  -237,   -78,  -237,  -237,  -237,    39,  -237,  -237,
    -237,  -237,  -237,  -237,  -237,  -237,  -237,   100,    39,  -237,
    -237,  -237,  -237,  -237,  -237,   -54,     0,   -42,  -237,  -237,
       3,  -237,   -10,  -237,  -237,  -237,  -237,  -237,    13,     6,
    -237,  -237,   121,  -237,   -54,    -5,  -237,   100,   613,     5,
    -237,   100,  -237,   354,   134,    18,  -237,    30,   134,    36,
      70,   115,    15,  -237,  -237,   100,    17,  -237,  -237,   613,
     613,  -237,  -237,  -237,   613,  -237,  -237,  -237,  -237,   258,
    -237,  -237,  -237,   -16,   138,   613,   116,   -34,   -37,   -49,
     -38,    64,   106,    67,   161,   -55,  -237,  -237,   -57,    -3,
     130,  -237,  -237,  -237,   354,   153,   168,   613,   174,  -237,
    -237,  -237,    39,   246,  -237,  -237,  -237,  -237,  -237,    20,
     156,   137,    23,  -237,   158,  -237,  -237,  -237,  -237,  -237,
    -237,   258,    81,   177,  -237,  -237,   613,   100,  -237,  -237,
    -237,  -237,  -237,  -237,  -237,  -237,  -237,  -237,  -237,   613,
    -237,   613,   613,   613,   613,   613,   613,   613,   613,   613,
     613,   613,   613,   613,   613,   613,   613,   613,   613,   613,
     613,  -237,   178,  -237,   433,   215,  -237,   613,    22,   613,
     -63,  -237,  -237,  -237,  -237,   184,  -237,   100,  -237,   366,
     170,   185,   181,   183,   -62,  -237,   613,    -8,  -237,  -237,
    -237,  -237,  -237,  -237,   116,   116,   -34,   -34,   -37,   -37,
     -37,   -37,   -49,   -49,   -38,    64,   106,    67,   161,    60,
    -237,   100,   613,  -237,  -237,  -237,   187,   473,    86,  -237,
      99,   182,    24,   -44,   100,  -237,   188,   191,  -237,   721,
       5,   194,  -237,   124,  -237,   613,   136,   -19,   613,   473,
     258,   473,   354,   354,   200,  -237,    29,  -237,  -237,  -237,
    -237,  -237,  -237,   258,  -237,   613,  -237,   613,  -237,  -237,
     100,  -237,   552,   140,   613,   613,   289,  -237,  -237,   193,
    -237,  -237,   100,  -237,  -237,   198,  -237,   205,   148,   164,
     354,  -237,     5,  -237,  -237,   354,   354,  -237,  -237,  -237,
    -237
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
      72,   115,   110,   111,    72,   127,   128,   129,   130,     0,
     112,   116,   113,   122,   131,    72,   135,   138,   141,   146,
     149,   151,   153,   155,   157,   159,   161,   174,     0,     0,
      68,    29,    30,    67,    72,     0,     0,    72,     0,   108,
      96,    94,     0,    72,    92,    97,    98,    99,    95,     0,
       0,     0,    72,    16,     0,    25,    63,    61,    58,   123,
     124,     0,     0,     0,   117,   118,    72,     0,   169,   170,
     164,   165,   166,   167,   168,   171,   172,   173,   163,    72,
     125,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    71,     0,    31,    72,     0,    25,    72,     0,    72,
       0,    24,    93,   109,    54,     0,    12,     0,    17,     0,
      72,     0,    39,     0,    70,   114,    72,     0,   119,   162,
     132,   133,   134,   131,   136,   137,   139,   140,   144,   145,
     142,   143,   147,   148,   150,   152,   154,   156,   158,     0,
     175,     0,    72,    69,    84,    87,     0,    72,     0,   100,
       0,     0,     0,     0,     0,    11,     0,    35,    36,    72,
      28,     0,    88,     0,   120,    72,     0,     0,    72,    72,
       0,    72,    72,    72,     0,    19,     0,    45,    39,    41,
      43,    42,    40,     0,    22,    72,   121,    72,   160,    32,
       0,    85,    72,     0,    72,    72,   102,   101,   104,     0,
      18,    37,     0,   126,    89,     0,    86,     0,     0,     0,
      72,    46,    28,    33,   105,    72,    72,   103,    38,   107,
     106
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -237,  -237,  -237,  -237,   304,  -237,  -123,   -36,   179,  -237,
    -237,  -237,   298,  -122,  -237,  -236,  -237,  -237,  -237,  -237,
      45,  -237,  -237,   -13,    68,   323,  -237,   260,   240,    82,
    -237,    -4,   259,   -47,    -1,  -237,  -176,    89,  -237,  -237,
    -104,  -237,  -237,  -237,  -237,  -221,  -237,  -237,  -237,   -23,
    -237,    11,    37,     2,    97,   149,   147,   150,   151,   146,
    -237,  -237,   -99,  -237,   -52
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    17,    18,    19,    20,    21,    45,   142,   208,
      22,    23,   130,    73,    81,   120,   121,   122,   213,   257,
     258,   259,   282,   200,    63,   131,    25,    65,    66,    26,
      46,    82,    48,    69,    99,   243,   262,   263,   100,   133,
     134,   135,   136,   297,   137,   138,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   169,   139
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      27,    64,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    38,    39,    40,    62,    47,   118,    83,   244,   207,
     195,   188,    77,   212,   284,    67,   271,    49,    50,   202,
       3,    64,     4,   192,   178,   179,    42,    43,    44,   182,
       5,   176,   152,   177,     6,   216,   216,   183,   294,    64,
     295,     7,    41,    68,   190,     8,   204,   154,   155,   191,
       9,    68,    76,   180,   181,    52,   149,   150,    10,    71,
     219,    11,   132,   189,   247,   198,   318,    53,    54,    55,
      56,    57,   170,   193,   174,   175,   153,   207,   291,    58,
      13,   240,   292,   151,    14,   245,    42,    43,    44,   156,
      84,   157,    74,   190,   217,    70,    85,    59,   264,    60,
      72,   304,    42,    75,    44,   119,   304,   245,    78,    76,
     146,   218,   148,   132,   -14,   203,    71,   249,    85,   275,
     206,   190,   132,   190,   300,    71,   143,   239,   214,    61,
      71,   209,   141,   245,   -26,   248,   144,   250,   220,   221,
     222,   223,   223,   223,   223,   223,   223,   223,   223,   223,
     223,   223,   223,   223,   223,   223,   288,   261,   296,   298,
     265,   190,    53,    54,    55,    56,    57,     3,   -27,     4,
     228,   229,   230,   231,   140,   224,   225,     5,   245,   184,
     215,     6,   190,   245,   186,   272,   254,   190,     7,    42,
      43,    44,     8,   252,    60,   266,   317,     9,   273,   209,
     190,   319,   320,   226,   227,    10,   293,   158,    11,   159,
      79,    43,    80,   145,   160,   161,   162,   163,   164,   165,
     166,   167,   185,   286,    61,   287,   187,    13,   171,   172,
     173,    14,   308,   309,   194,   289,   270,   290,   205,   307,
     276,   190,   168,     3,   305,     4,   204,   315,   283,   190,
     124,   196,   303,     5,    87,   125,   312,     6,   126,   211,
     302,   132,   132,   316,     7,   190,   197,   255,     8,   232,
     233,   127,   199,     9,    52,   216,   241,   246,   251,   256,
     -34,    10,   260,   274,    11,   268,    53,    54,    55,    56,
      57,   277,   278,   285,   299,   310,   311,   313,   140,   132,
     314,    88,    12,    13,   132,   132,    37,    14,   128,    89,
      90,    51,   210,   301,    24,   147,    59,    86,    60,   269,
     123,   267,   235,   234,   238,     0,   236,     0,   237,     0,
       0,     0,     0,     0,     0,    91,     0,     0,     0,    92,
      93,   129,    50,   201,    94,     0,     0,     0,    61,     0,
       0,     3,     0,     4,    95,    96,    97,    98,   124,     0,
       0,     5,    87,   125,     0,     6,   126,     0,     0,     0,
       0,     0,     7,     0,     0,     0,     8,     0,     0,   127,
       0,     9,    52,     0,     0,     0,     0,     0,     0,    10,
       0,     0,    11,     0,    53,    54,    55,    56,    57,     0,
       0,     0,     0,     0,     0,     0,   253,     0,     0,    88,
      12,    13,     0,     0,     0,    14,   128,    89,    90,     0,
       0,     0,     0,     0,    59,     0,    60,     0,     0,     0,
       3,     0,     4,     0,     0,     0,     0,     0,     0,     0,
       5,    87,     0,    91,     6,     0,     0,    92,    93,   129,
      50,     7,    94,     0,     0,     8,    61,     0,     0,     0,
       9,     0,    95,    96,    97,    98,     0,     0,    10,     0,
       3,    11,     4,     0,     0,     0,     0,     0,     0,     0,
       5,    87,     0,     0,     6,     0,     0,     0,    88,     0,
      13,     7,     0,     0,    14,     8,    89,    90,     0,     0,
       9,     0,     0,     0,     0,     0,     0,     0,    10,     0,
       0,    11,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    91,     0,     0,     0,    92,    93,    88,   242,
      13,    94,     0,     0,    14,     0,    89,    90,     0,     0,
       0,    95,    96,    97,    98,     0,     0,     0,     0,     3,
       0,     4,     0,     0,     0,     0,     0,     0,     0,     5,
      87,     0,    91,     6,     0,     0,    92,    93,   129,     0,
       7,    94,     0,     0,     8,     0,     0,     0,     0,     9,
       0,    95,    96,    97,    98,     0,     0,    10,     0,     0,
      11,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    88,     0,    13,
       3,     0,     4,    14,     0,    89,    90,     0,     0,     0,
       5,    87,     0,     0,     6,     0,     0,     0,     0,     0,
       0,     7,     0,     0,     0,     8,     0,     0,     0,     0,
       9,    91,     0,     0,     0,    92,    93,     0,    10,   306,
      94,    11,     0,     0,     0,     0,     0,     0,     0,     0,
      95,    96,    97,    98,     2,     0,     0,     0,    88,     0,
      13,     3,     0,     4,    14,     0,    89,    90,     0,     0,
       0,     5,     0,     0,     0,     6,     0,     0,     0,     0,
       0,     0,     7,     0,     0,     0,     8,     0,     0,     0,
       0,     9,    91,     0,     0,     0,    92,    93,     0,    10,
       0,    94,    11,     0,     0,     0,     0,     0,     3,     0,
       4,    95,    96,    97,    98,     0,     0,     0,     5,     0,
      12,    13,     6,     0,   279,    14,   280,     0,     0,     7,
     281,     0,     0,     8,     0,     0,     0,     3,     9,     4,
       0,     0,     0,     0,     0,     0,    10,     5,     0,    11,
       0,     6,    15,     0,     0,     0,     0,     0,     7,    16,
       0,     0,     8,     0,     0,     0,     0,     9,    13,     0,
       0,     0,    14,     0,     0,    10,     0,     0,    11,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    13,     0,     0,
       0,    14
};

static const yytype_int16 yycheck[] =
{
       1,    37,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    27,    19,    68,    64,   194,   142,
     124,    76,    58,   145,   260,    38,   247,   105,   106,   133,
       7,    67,     9,    36,    83,    84,    99,   100,   101,    77,
      17,    78,    94,    80,    21,   108,   108,    85,   269,    85,
     271,    28,   102,   115,   111,    32,   100,    73,    74,   116,
      37,   115,   106,   112,   113,    26,    89,    90,    45,   111,
     169,    48,    73,   128,   196,   127,   312,    38,    39,    40,
      41,    42,   105,   119,   118,   119,    99,   210,   107,    50,
      67,   190,   111,    94,    71,   194,    99,   100,   101,   115,
     105,   117,   112,   111,   156,   105,   111,    68,   116,    70,
     107,   287,    99,   100,   101,   110,   292,   216,   112,   106,
     105,   157,   105,   124,   106,   105,   111,   105,   111,   105,
     107,   111,   133,   111,   105,   111,   106,   189,   151,   100,
     111,   142,    74,   242,   108,   197,    78,   199,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   187,   188,   265,   214,   272,   273,
     110,   111,    38,    39,    40,    41,    42,     7,   108,     9,
     178,   179,   180,   181,    50,   174,   175,    17,   287,   125,
     109,    21,   111,   292,   127,   109,   209,   111,    28,    99,
     100,   101,    32,   207,    70,   241,   310,    37,   109,   210,
     111,   315,   316,   176,   177,    45,   268,    79,    48,    81,
      99,   100,   101,   108,    86,    87,    88,    89,    90,    91,
      92,    93,   126,   109,   100,   111,    75,    67,   122,   123,
     124,    71,   294,   295,   114,   109,   247,   111,   111,   109,
     254,   111,   114,     7,   290,     9,   100,   109,   259,   111,
      14,   108,   285,    17,    18,    19,   302,    21,    22,   111,
     283,   272,   273,   109,    28,   111,   108,   107,    32,   182,
     183,    35,   108,    37,    26,   108,   108,    72,   104,   104,
     109,    45,   109,   111,    48,   108,    38,    39,    40,    41,
      42,   113,   111,   109,   104,    16,   113,   109,    50,   310,
     105,    65,    66,    67,   315,   316,    12,    71,    72,    73,
      74,    23,   143,   278,     1,    85,    68,    67,    70,   247,
      71,   242,   185,   184,   188,    -1,   186,    -1,   187,    -1,
      -1,    -1,    -1,    -1,    -1,    99,    -1,    -1,    -1,   103,
     104,   105,   106,   107,   108,    -1,    -1,    -1,   100,    -1,
      -1,     7,    -1,     9,   118,   119,   120,   121,    14,    -1,
      -1,    17,    18,    19,    -1,    21,    22,    -1,    -1,    -1,
      -1,    -1,    28,    -1,    -1,    -1,    32,    -1,    -1,    35,
      -1,    37,    26,    -1,    -1,    -1,    -1,    -1,    -1,    45,
      -1,    -1,    48,    -1,    38,    39,    40,    41,    42,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    50,    -1,    -1,    65,
      66,    67,    -1,    -1,    -1,    71,    72,    73,    74,    -1,
      -1,    -1,    -1,    -1,    68,    -1,    70,    -1,    -1,    -1,
       7,    -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      17,    18,    -1,    99,    21,    -1,    -1,   103,   104,   105,
     106,    28,   108,    -1,    -1,    32,   100,    -1,    -1,    -1,
      37,    -1,   118,   119,   120,   121,    -1,    -1,    45,    -1,
       7,    48,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      17,    18,    -1,    -1,    21,    -1,    -1,    -1,    65,    -1,
      67,    28,    -1,    -1,    71,    32,    73,    74,    -1,    -1,
      37,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,    -1,
      -1,    48,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    99,    -1,    -1,    -1,   103,   104,    65,   106,
      67,   108,    -1,    -1,    71,    -1,    73,    74,    -1,    -1,
      -1,   118,   119,   120,   121,    -1,    -1,    -1,    -1,     7,
      -1,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    17,
      18,    -1,    99,    21,    -1,    -1,   103,   104,   105,    -1,
      28,   108,    -1,    -1,    32,    -1,    -1,    -1,    -1,    37,
      -1,   118,   119,   120,   121,    -1,    -1,    45,    -1,    -1,
      48,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    65,    -1,    67,
       7,    -1,     9,    71,    -1,    73,    74,    -1,    -1,    -1,
      17,    18,    -1,    -1,    21,    -1,    -1,    -1,    -1,    -1,
      -1,    28,    -1,    -1,    -1,    32,    -1,    -1,    -1,    -1,
      37,    99,    -1,    -1,    -1,   103,   104,    -1,    45,   107,
     108,    48,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     118,   119,   120,   121,     0,    -1,    -1,    -1,    65,    -1,
      67,     7,    -1,     9,    71,    -1,    73,    74,    -1,    -1,
      -1,    17,    -1,    -1,    -1,    21,    -1,    -1,    -1,    -1,
      -1,    -1,    28,    -1,    -1,    -1,    32,    -1,    -1,    -1,
      -1,    37,    99,    -1,    -1,    -1,   103,   104,    -1,    45,
      -1,   108,    48,    -1,    -1,    -1,    -1,    -1,     7,    -1,
       9,   118,   119,   120,   121,    -1,    -1,    -1,    17,    -1,
      66,    67,    21,    -1,    23,    71,    25,    -1,    -1,    28,
      29,    -1,    -1,    32,    -1,    -1,    -1,     7,    37,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    45,    17,    -1,    48,
      -1,    21,    98,    -1,    -1,    -1,    -1,    -1,    28,   105,
      -1,    -1,    32,    -1,    -1,    -1,    -1,    37,    67,    -1,
      -1,    -1,    71,    -1,    -1,    45,    -1,    -1,    48,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    67,    -1,    -1,
      -1,    71
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
     184,   185,   186,   187,   188,   189,   190,   191,   193,   110,
     144,   145,   146,   161,    14,    19,    22,    35,    72,   105,
     141,   154,   163,   168,   169,   170,   171,   173,   174,   193,
      50,   153,   137,   106,   153,   108,   105,   157,   105,   178,
     178,   163,   193,   152,    73,    74,   115,   117,    79,    81,
      86,    87,    88,    89,    90,    91,    92,    93,   114,   192,
     178,   122,   123,   124,   118,   119,    78,    80,    83,    84,
     112,   113,    77,    85,   125,   126,   127,    75,    76,   128,
     111,   116,    36,   136,   114,   169,   108,   108,   193,   108,
     152,   107,   169,   105,   100,   111,   107,   135,   138,   163,
     137,   111,   142,   147,   152,   109,   108,   193,   136,   191,
     178,   178,   178,   178,   180,   180,   181,   181,   182,   182,
     182,   182,   183,   183,   184,   185,   186,   187,   188,   193,
     191,   108,   106,   164,   165,   191,    72,   142,   193,   105,
     193,   104,   160,    50,   152,   107,   104,   148,   149,   150,
     109,   162,   165,   166,   116,   110,   136,   166,   108,   158,
     163,   174,   109,   109,   111,   105,   160,   113,   111,    23,
      25,    29,   151,   163,   144,   109,   109,   111,   191,   109,
     111,   107,   111,   193,   174,   174,   169,   172,   169,   104,
     105,   149,   152,   178,   165,   136,   107,   109,   193,   193,
      16,   113,   136,   109,   105,   109,   109,   169,   144,   169,
     169
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
     175,   175,   175,   175,   175,   176,   177,   177,   177,   177,
     177,   177,   178,   178,   178,   178,   178,   179,   179,   179,
     179,   180,   180,   180,   180,   181,   181,   181,   182,   182,
     182,   183,   183,   183,   183,   183,   184,   184,   184,   185,
     185,   186,   186,   187,   187,   188,   188,   189,   189,   190,
     190,   191,   191,   192,   192,   192,   192,   192,   192,   192,
     192,   192,   192,   192,   193,   193
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
       1,     1,     1,     1,     3,     1,     1,     2,     2,     3,
       4,     5,     1,     2,     2,     2,     6,     1,     1,     1,
       1,     1,     3,     3,     3,     1,     3,     3,     1,     3,
       3,     1,     3,     3,     3,     3,     1,     3,     3,     1,
       3,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       5,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
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
static unsigned
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  unsigned res = 0;
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


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (yylocationp);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp);
  YYFPRINTF (yyoutput, ")");
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
  unsigned long int yylno = yyrline[yyrule];
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
                       &(yyvsp[(yyi + 1) - (yynrhs)])
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
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
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
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
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
| yyreduce -- Do a reduction.  |
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

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 1095 "hlsl.y" /* yacc.c:1646  */
    {
                            }
#line 2697 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 3:
#line 1098 "hlsl.y" /* yacc.c:1646  */
    {
                                const struct hlsl_ir_function_decl *decl;

                                decl = get_overloaded_func(&hlsl_ctx.functions, (yyvsp[0].function).name, (yyvsp[0].function).decl->parameters, TRUE);
                                if (decl && !decl->func->intrinsic)
                                {
                                    if (decl->body && (yyvsp[0].function).decl->body)
                                    {
                                        hlsl_report_message((yyvsp[0].function).decl->node.loc.file, (yyvsp[0].function).decl->node.loc.line,
                                                (yyvsp[0].function).decl->node.loc.col, HLSL_LEVEL_ERROR,
                                                "redefinition of function %s", debugstr_a((yyvsp[0].function).name));
                                        return 1;
                                    }
                                    else if (!compare_hlsl_types(decl->node.data_type, (yyvsp[0].function).decl->node.data_type))
                                    {
                                        hlsl_report_message((yyvsp[0].function).decl->node.loc.file, (yyvsp[0].function).decl->node.loc.line,
                                                (yyvsp[0].function).decl->node.loc.col, HLSL_LEVEL_ERROR,
                                                "redefining function %s with a different return type",
                                                debugstr_a((yyvsp[0].function).name));
                                        hlsl_report_message(decl->node.loc.file, decl->node.loc.line, decl->node.loc.col, HLSL_LEVEL_NOTE,
                                                "%s previously declared here",
                                                debugstr_a((yyvsp[0].function).name));
                                        return 1;
                                    }
                                }

                                if ((yyvsp[0].function).decl->node.data_type->base_type == HLSL_TYPE_VOID && (yyvsp[0].function).decl->semantic)
                                {
                                    hlsl_report_message((yyvsp[0].function).decl->node.loc.file, (yyvsp[0].function).decl->node.loc.line,
                                            (yyvsp[0].function).decl->node.loc.col, HLSL_LEVEL_ERROR,
                                            "void function with a semantic");
                                }

                                TRACE("Adding function '%s' to the function list.\n", (yyvsp[0].function).name);
                                add_function_decl(&hlsl_ctx.functions, (yyvsp[0].function).name, (yyvsp[0].function).decl, FALSE);
                            }
#line 2738 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 4:
#line 1135 "hlsl.y" /* yacc.c:1646  */
    {
                                TRACE("Declaration statement parsed.\n");
                            }
#line 2746 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 5:
#line 1139 "hlsl.y" /* yacc.c:1646  */
    {
                            }
#line 2753 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 6:
#line 1142 "hlsl.y" /* yacc.c:1646  */
    {
                                TRACE("Skipping stray semicolon.\n");
                            }
#line 2761 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 1147 "hlsl.y" /* yacc.c:1646  */
    {
                                const char **new_array = NULL;

                                TRACE("Updating line information to file %s, line %u\n", debugstr_a((yyvsp[0].name)), (yyvsp[-1].intval));
                                hlsl_ctx.line_no = (yyvsp[-1].intval);
                                if (strcmp((yyvsp[0].name), hlsl_ctx.source_file))
                                    new_array = d3dcompiler_realloc(hlsl_ctx.source_files,
                                            sizeof(*hlsl_ctx.source_files) * hlsl_ctx.source_files_count + 1);

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
#line 2786 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 1169 "hlsl.y" /* yacc.c:1646  */
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
#line 2806 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 1189 "hlsl.y" /* yacc.c:1646  */
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
                                    return 1;
                                }

                                ret = add_type_to_scope(hlsl_ctx.cur_scope, (yyval.type));
                                if (!ret)
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, (yylsp[-3]).first_line, (yylsp[-3]).first_column,
                                            HLSL_LEVEL_ERROR, "redefinition of struct '%s'", (yyvsp[-3].name));
                                    return 1;
                                }
                            }
#line 2835 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 1215 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                TRACE("Anonymous structure declaration.\n");
                                set_location(&loc, &(yylsp[-4]));
                                check_invalid_matrix_modifiers((yyvsp[-4].modifiers), &loc);
                                (yyval.type) = new_struct_type(NULL, (yyvsp[-4].modifiers), (yyvsp[-1].list));
                            }
#line 2848 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 1229 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                            }
#line 2857 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 1234 "hlsl.y" /* yacc.c:1646  */
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
#line 2879 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 1253 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = gen_struct_fields((yyvsp[-2].type), (yyvsp[-3].modifiers), (yyvsp[-1].list));
                            }
#line 2887 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 1257 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = gen_struct_fields((yyvsp[-2].type), 0, (yyvsp[-1].list));
                            }
#line 2895 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 1262 "hlsl.y" /* yacc.c:1646  */
    {
                                TRACE("Function %s parsed.\n", (yyvsp[-1].function).name);
                                (yyval.function) = (yyvsp[-1].function);
                                (yyval.function).decl->body = (yyvsp[0].list);
                                pop_scope(&hlsl_ctx);
                            }
#line 2906 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 1269 "hlsl.y" /* yacc.c:1646  */
    {
                                TRACE("Function prototype for %s.\n", (yyvsp[-1].function).name);
                                (yyval.function) = (yyvsp[-1].function);
                                pop_scope(&hlsl_ctx);
                            }
#line 2916 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 1276 "hlsl.y" /* yacc.c:1646  */
    {
                                if (get_variable(hlsl_ctx.globals, (yyvsp[-4].name)))
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, (yylsp[-4]).first_line, (yylsp[-4]).first_column,
                                            HLSL_LEVEL_ERROR, "redefinition of '%s'\n", (yyvsp[-4].name));
                                    return 1;
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
                                    return -1;
                                }
                                (yyval.function).name = (yyvsp[-4].name);
                                (yyval.function).decl->semantic = (yyvsp[0].colon_attribute).semantic;
                                set_location(&(yyval.function).decl->node.loc, &(yylsp[-4]));
                            }
#line 2949 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 1306 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                            }
#line 2958 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 1311 "hlsl.y" /* yacc.c:1646  */
    {
                                pop_scope(&hlsl_ctx);
                                (yyval.list) = (yyvsp[-1].list);
                            }
#line 2967 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 1317 "hlsl.y" /* yacc.c:1646  */
    {
                                push_scope(&hlsl_ctx);
                            }
#line 2975 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 1325 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.colon_attribute).semantic = NULL;
                                (yyval.colon_attribute).reg_reservation = NULL;
                            }
#line 2984 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 1330 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.colon_attribute).semantic = (yyvsp[0].name);
                                (yyval.colon_attribute).reg_reservation = NULL;
                            }
#line 2993 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 1335 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.colon_attribute).semantic = NULL;
                                (yyval.colon_attribute).reg_reservation = (yyvsp[0].reg_reservation);
                            }
#line 3002 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 1341 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.name) = (yyvsp[0].name);
                            }
#line 3010 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 1347 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.reg_reservation) = parse_reg_reservation((yyvsp[-1].name));
                                d3dcompiler_free((yyvsp[-1].name));
                            }
#line 3019 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 1352 "hlsl.y" /* yacc.c:1646  */
    {
                                FIXME("Ignoring shader target %s in a register reservation.\n", debugstr_a((yyvsp[-3].name)));
                                d3dcompiler_free((yyvsp[-3].name));

                                (yyval.reg_reservation) = parse_reg_reservation((yyvsp[-1].name));
                                d3dcompiler_free((yyvsp[-1].name));
                            }
#line 3031 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 1361 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                            }
#line 3040 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 35:
#line 1366 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = (yyvsp[0].list);
                            }
#line 3048 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 36:
#line 1371 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                set_location(&loc, &(yylsp[0]));
                                if (!add_func_parameter((yyval.list), &(yyvsp[0].parameter), &loc))
                                {
                                    ERR("Error adding function parameter %s.\n", (yyvsp[0].parameter).name);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return -1;
                                }
                            }
#line 3066 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 37:
#line 1385 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                (yyval.list) = (yyvsp[-2].list);
                                set_location(&loc, &(yylsp[0]));
                                if (!add_func_parameter((yyval.list), &(yyvsp[0].parameter), &loc))
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "duplicate parameter %s", (yyvsp[0].parameter).name);
                                    return 1;
                                }
                            }
#line 3083 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 1399 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.parameter).modifiers = (yyvsp[-4].modifiers) ? (yyvsp[-4].modifiers) : HLSL_MODIFIER_IN;
                                (yyval.parameter).modifiers |= (yyvsp[-3].modifiers);
                                (yyval.parameter).type = (yyvsp[-2].type);
                                (yyval.parameter).name = (yyvsp[-1].name);
                                (yyval.parameter).semantic = (yyvsp[0].colon_attribute).semantic;
                                (yyval.parameter).reg_reservation = (yyvsp[0].colon_attribute).reg_reservation;
                            }
#line 3096 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 1409 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.modifiers) = 0;
                            }
#line 3104 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 1413 "hlsl.y" /* yacc.c:1646  */
    {
                                if ((yyvsp[-1].modifiers) & (yyvsp[0].modifiers))
                                {
                                    hlsl_report_message(hlsl_ctx.source_file, (yylsp[0]).first_line, (yylsp[0]).first_column,
                                            HLSL_LEVEL_ERROR, "duplicate input-output modifiers");
                                    return 1;
                                }
                                (yyval.modifiers) = (yyvsp[-1].modifiers) | (yyvsp[0].modifiers);
                            }
#line 3118 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 41:
#line 1424 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.modifiers) = HLSL_MODIFIER_IN;
                            }
#line 3126 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 1428 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.modifiers) = HLSL_MODIFIER_OUT;
                            }
#line 3134 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 43:
#line 1432 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.modifiers) = HLSL_MODIFIER_IN | HLSL_MODIFIER_OUT;
                            }
#line 3142 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 1437 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.type) = (yyvsp[0].type);
                            }
#line 3150 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 45:
#line 1441 "hlsl.y" /* yacc.c:1646  */
    {
                                if ((yyvsp[-3].type)->type != HLSL_CLASS_SCALAR)
                                {
                                    hlsl_message("Line %u: vectors of non-scalar types are not allowed.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }
                                if ((yyvsp[-1].intval) < 1 || (yyvsp[-1].intval) > 4)
                                {
                                    hlsl_message("Line %u: vector size must be between 1 and 4.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }

                                (yyval.type) = new_hlsl_type(NULL, HLSL_CLASS_VECTOR, (yyvsp[-3].type)->base_type, (yyvsp[-1].intval), 1);
                            }
#line 3173 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 46:
#line 1460 "hlsl.y" /* yacc.c:1646  */
    {
                                if ((yyvsp[-5].type)->type != HLSL_CLASS_SCALAR)
                                {
                                    hlsl_message("Line %u: matrices of non-scalar types are not allowed.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }
                                if ((yyvsp[-3].intval) < 1 || (yyvsp[-3].intval) > 4 || (yyvsp[-1].intval) < 1 || (yyvsp[-1].intval) > 4)
                                {
                                    hlsl_message("Line %u: matrix dimensions must be between 1 and 4.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }

                                (yyval.type) = new_hlsl_type(NULL, HLSL_CLASS_MATRIX, (yyvsp[-5].type)->base_type, (yyvsp[-3].intval), (yyvsp[-1].intval));
                            }
#line 3196 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 1480 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.type) = new_hlsl_type(d3dcompiler_strdup("void"), HLSL_CLASS_OBJECT, HLSL_TYPE_VOID, 1, 1);
                            }
#line 3204 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 1484 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.type) = new_hlsl_type(d3dcompiler_strdup("sampler"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                (yyval.type)->sampler_dim = HLSL_SAMPLER_DIM_GENERIC;
                            }
#line 3213 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 1489 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.type) = new_hlsl_type(d3dcompiler_strdup("sampler1D"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                (yyval.type)->sampler_dim = HLSL_SAMPLER_DIM_1D;
                            }
#line 3222 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 50:
#line 1494 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.type) = new_hlsl_type(d3dcompiler_strdup("sampler2D"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                (yyval.type)->sampler_dim = HLSL_SAMPLER_DIM_2D;
                            }
#line 3231 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 51:
#line 1499 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.type) = new_hlsl_type(d3dcompiler_strdup("sampler3D"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                (yyval.type)->sampler_dim = HLSL_SAMPLER_DIM_3D;
                            }
#line 3240 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 1504 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.type) = new_hlsl_type(d3dcompiler_strdup("samplerCUBE"), HLSL_CLASS_OBJECT, HLSL_TYPE_SAMPLER, 1, 1);
                                (yyval.type)->sampler_dim = HLSL_SAMPLER_DIM_CUBE;
                            }
#line 3249 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 1509 "hlsl.y" /* yacc.c:1646  */
    {
                                struct hlsl_type *type;

                                type = get_type(hlsl_ctx.cur_scope, (yyvsp[0].name), TRUE);
                                (yyval.type) = type;
                                d3dcompiler_free((yyvsp[0].name));
                            }
#line 3261 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 1517 "hlsl.y" /* yacc.c:1646  */
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
#line 3282 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 57:
#line 1537 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                if (!(yyval.list))
                                {
                                    ERR("Out of memory\n");
                                    return -1;
                                }
                                list_init((yyval.list));
                            }
#line 3296 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 1548 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-4]));
                                if (!add_typedef((yyvsp[-3].modifiers), (yyvsp[-2].type), (yyvsp[-1].list), &loc))
                                    return 1;
                            }
#line 3308 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 59:
#line 1556 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-3]));
                                if (!add_typedef(0, (yyvsp[-2].type), (yyvsp[-1].list), &loc))
                                    return 1;
                            }
#line 3320 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 1565 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                list_add_head((yyval.list), &(yyvsp[0].variable_def)->entry);
                            }
#line 3330 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 61:
#line 1571 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = (yyvsp[-2].list);
                                list_add_tail((yyval.list), &(yyvsp[0].variable_def)->entry);
                            }
#line 3339 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 62:
#line 1577 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.variable_def) = d3dcompiler_alloc(sizeof(*(yyval.variable_def)));
                                set_location(&(yyval.variable_def)->loc, &(yylsp[-1]));
                                (yyval.variable_def)->name = (yyvsp[-1].name);
                                (yyval.variable_def)->array_size = (yyvsp[0].intval);
                            }
#line 3350 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 63:
#line 1585 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = declare_vars((yyvsp[-2].type), (yyvsp[-3].modifiers), (yyvsp[-1].list));
                            }
#line 3358 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 64:
#line 1590 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = NULL;
                            }
#line 3366 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 65:
#line 1594 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = (yyvsp[0].list);
                            }
#line 3374 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 66:
#line 1599 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                list_add_head((yyval.list), &(yyvsp[0].variable_def)->entry);
                            }
#line 3384 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 67:
#line 1605 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = (yyvsp[-2].list);
                                list_add_tail((yyval.list), &(yyvsp[0].variable_def)->entry);
                            }
#line 3393 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 68:
#line 1611 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.variable_def) = d3dcompiler_alloc(sizeof(*(yyval.variable_def)));
                                set_location(&(yyval.variable_def)->loc, &(yylsp[-2]));
                                (yyval.variable_def)->name = (yyvsp[-2].name);
                                (yyval.variable_def)->array_size = (yyvsp[-1].intval);
                                (yyval.variable_def)->semantic = (yyvsp[0].colon_attribute).semantic;
                                (yyval.variable_def)->reg_reservation = (yyvsp[0].colon_attribute).reg_reservation;
                            }
#line 3406 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 69:
#line 1620 "hlsl.y" /* yacc.c:1646  */
    {
                                TRACE("Declaration with initializer.\n");
                                (yyval.variable_def) = d3dcompiler_alloc(sizeof(*(yyval.variable_def)));
                                set_location(&(yyval.variable_def)->loc, &(yylsp[-4]));
                                (yyval.variable_def)->name = (yyvsp[-4].name);
                                (yyval.variable_def)->array_size = (yyvsp[-3].intval);
                                (yyval.variable_def)->semantic = (yyvsp[-2].colon_attribute).semantic;
                                (yyval.variable_def)->reg_reservation = (yyvsp[-2].colon_attribute).reg_reservation;
                                (yyval.variable_def)->initializer = (yyvsp[0].list);
                            }
#line 3421 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 70:
#line 1632 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.intval) = 0;
                            }
#line 3429 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 71:
#line 1636 "hlsl.y" /* yacc.c:1646  */
    {
                                FIXME("Array.\n");
                                (yyval.intval) = 0;
                                free_instr((yyvsp[-1].instr));
                            }
#line 3439 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 72:
#line 1643 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.modifiers) = 0;
                            }
#line 3447 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 73:
#line 1647 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_STORAGE_EXTERN, &(yylsp[-1]));
                            }
#line 3455 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 74:
#line 1651 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_STORAGE_NOINTERPOLATION, &(yylsp[-1]));
                            }
#line 3463 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 75:
#line 1655 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_MODIFIER_PRECISE, &(yylsp[-1]));
                            }
#line 3471 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 76:
#line 1659 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_STORAGE_SHARED, &(yylsp[-1]));
                            }
#line 3479 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 77:
#line 1663 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_STORAGE_GROUPSHARED, &(yylsp[-1]));
                            }
#line 3487 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 78:
#line 1667 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_STORAGE_STATIC, &(yylsp[-1]));
                            }
#line 3495 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 79:
#line 1671 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_STORAGE_UNIFORM, &(yylsp[-1]));
                            }
#line 3503 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 80:
#line 1675 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_STORAGE_VOLATILE, &(yylsp[-1]));
                            }
#line 3511 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 81:
#line 1679 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_MODIFIER_CONST, &(yylsp[-1]));
                            }
#line 3519 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 82:
#line 1683 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_MODIFIER_ROW_MAJOR, &(yylsp[-1]));
                            }
#line 3527 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 83:
#line 1687 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.modifiers) = add_modifier((yyvsp[0].modifiers), HLSL_MODIFIER_COLUMN_MAJOR, &(yylsp[-1]));
                            }
#line 3535 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 84:
#line 1692 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                list_add_head((yyval.list), &(yyvsp[0].instr)->entry);
                            }
#line 3545 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 85:
#line 1698 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = (yyvsp[-1].list);
                            }
#line 3553 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 86:
#line 1702 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = (yyvsp[-2].list);
                            }
#line 3561 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 87:
#line 1707 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 3569 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 88:
#line 1712 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                list_add_head((yyval.list), &(yyvsp[0].instr)->entry);
                            }
#line 3579 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 89:
#line 1718 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = (yyvsp[-2].list);
                                list_add_tail((yyval.list), &(yyvsp[0].instr)->entry);
                            }
#line 3588 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 90:
#line 1724 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.boolval) = TRUE;
                            }
#line 3596 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 91:
#line 1728 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.boolval) = FALSE;
                            }
#line 3604 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 92:
#line 1733 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = (yyvsp[0].list);
                            }
#line 3612 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 93:
#line 1737 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = (yyvsp[-1].list);
                                list_move_tail((yyval.list), (yyvsp[0].list));
                                d3dcompiler_free((yyvsp[0].list));
                            }
#line 3622 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 100:
#line 1752 "hlsl.y" /* yacc.c:1646  */
    {
                                struct hlsl_ir_jump *jump = d3dcompiler_alloc(sizeof(*jump));
                                if (!jump)
                                {
                                    ERR("Out of memory\n");
                                    return -1;
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
#line 3648 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 101:
#line 1775 "hlsl.y" /* yacc.c:1646  */
    {
                                struct hlsl_ir_if *instr = d3dcompiler_alloc(sizeof(*instr));
                                if (!instr)
                                {
                                    ERR("Out of memory\n");
                                    return -1;
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
#line 3675 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 102:
#line 1799 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.if_body).then_instrs = (yyvsp[0].list);
                                (yyval.if_body).else_instrs = NULL;
                            }
#line 3684 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 103:
#line 1804 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.if_body).then_instrs = (yyvsp[-2].list);
                                (yyval.if_body).else_instrs = (yyvsp[0].list);
                            }
#line 3693 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 104:
#line 1810 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;
                                struct list *cond = d3dcompiler_alloc(sizeof(*cond));

                                if (!cond)
                                {
                                    ERR("Out of memory.\n");
                                    return -1;
                                }
                                list_init(cond);
                                list_add_head(cond, &(yyvsp[-2].instr)->entry);
                                set_location(&loc, &(yylsp[-4]));
                                (yyval.list) = create_loop(LOOP_WHILE, NULL, cond, NULL, (yyvsp[0].list), &loc);
                            }
#line 3712 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 105:
#line 1825 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;
                                struct list *cond = d3dcompiler_alloc(sizeof(*cond));

                                if (!cond)
                                {
                                    ERR("Out of memory.\n");
                                    return -1;
                                }
                                list_init(cond);
                                list_add_head(cond, &(yyvsp[-2].instr)->entry);
                                set_location(&loc, &(yylsp[-6]));
                                (yyval.list) = create_loop(LOOP_DO_WHILE, NULL, cond, NULL, (yyvsp[-5].list), &loc);
                            }
#line 3731 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 106:
#line 1840 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-7]));
                                (yyval.list) = create_loop(LOOP_FOR, (yyvsp[-4].list), (yyvsp[-3].list), (yyvsp[-2].instr), (yyvsp[0].list), &loc);
                                pop_scope(&hlsl_ctx);
                            }
#line 3743 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 107:
#line 1848 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-7]));
                                if (!(yyvsp[-4].list))
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_WARNING,
                                            "no expressions in for loop initializer");
                                (yyval.list) = create_loop(LOOP_FOR, (yyvsp[-4].list), (yyvsp[-3].list), (yyvsp[-2].instr), (yyvsp[0].list), &loc);
                                pop_scope(&hlsl_ctx);
                            }
#line 3758 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 108:
#line 1860 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                            }
#line 3767 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 109:
#line 1865 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.list) = d3dcompiler_alloc(sizeof(*(yyval.list)));
                                list_init((yyval.list));
                                if ((yyvsp[-1].instr))
                                    list_add_head((yyval.list), &(yyvsp[-1].instr)->entry);
                            }
#line 3778 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 110:
#line 1873 "hlsl.y" /* yacc.c:1646  */
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
                                c->v.value.f[0] = (yyvsp[0].floatval);
                                (yyval.instr) = &c->node;
                            }
#line 3796 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 111:
#line 1887 "hlsl.y" /* yacc.c:1646  */
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
                                c->v.value.i[0] = (yyvsp[0].intval);
                                (yyval.instr) = &c->node;
                            }
#line 3814 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 112:
#line 1901 "hlsl.y" /* yacc.c:1646  */
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
                                c->v.value.b[0] = (yyvsp[0].boolval);
                                (yyval.instr) = &c->node;
                            }
#line 3832 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 113:
#line 1915 "hlsl.y" /* yacc.c:1646  */
    {
                                struct hlsl_ir_deref *deref = new_var_deref((yyvsp[0].var));
                                if (deref)
                                {
                                    (yyval.instr) = &deref->node;
                                    set_location(&(yyval.instr)->loc, &(yylsp[0]));
                                }
                                else
                                    (yyval.instr) = NULL;
                            }
#line 3847 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 114:
#line 1926 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.instr) = (yyvsp[-1].instr);
                            }
#line 3855 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 115:
#line 1931 "hlsl.y" /* yacc.c:1646  */
    {
                                struct hlsl_ir_var *var;
                                var = get_variable(hlsl_ctx.cur_scope, (yyvsp[0].name));
                                if (!var)
                                {
                                    hlsl_message("Line %d: variable '%s' not declared\n",
                                            hlsl_ctx.line_no, (yyvsp[0].name));
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return 1;
                                }
                                (yyval.var) = var;
                            }
#line 3872 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 116:
#line 1945 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 3880 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 117:
#line 1949 "hlsl.y" /* yacc.c:1646  */
    {
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                set_location(&loc, &(yylsp[0]));
                                if ((yyvsp[-1].instr)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "modifying a const expression");
                                    return 1;
                                }
                                operands[0] = (yyvsp[-1].instr);
                                operands[1] = operands[2] = NULL;
                                (yyval.instr) = &new_expr(HLSL_IR_UNOP_POSTINC, operands, &loc)->node;
                                /* Post increment/decrement expressions are considered const */
                                (yyval.instr)->data_type = clone_hlsl_type((yyval.instr)->data_type);
                                (yyval.instr)->data_type->modifiers |= HLSL_MODIFIER_CONST;
                            }
#line 3903 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 118:
#line 1968 "hlsl.y" /* yacc.c:1646  */
    {
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                set_location(&loc, &(yylsp[0]));
                                if ((yyvsp[-1].instr)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "modifying a const expression");
                                    return 1;
                                }
                                operands[0] = (yyvsp[-1].instr);
                                operands[1] = operands[2] = NULL;
                                (yyval.instr) = &new_expr(HLSL_IR_UNOP_POSTDEC, operands, &loc)->node;
                                /* Post increment/decrement expressions are considered const */
                                (yyval.instr)->data_type = clone_hlsl_type((yyval.instr)->data_type);
                                (yyval.instr)->data_type->modifiers |= HLSL_MODIFIER_CONST;
                            }
#line 3926 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 119:
#line 1987 "hlsl.y" /* yacc.c:1646  */
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
                                                "invalid subscript %s", debugstr_a((yyvsp[0].name)));
                                        return 1;
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
                                        return 1;
                                    }
                                    (yyval.instr) = &swizzle->node;
                                }
                                else
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "invalid subscript %s", debugstr_a((yyvsp[0].name)));
                                    return 1;
                                }
                            }
#line 3984 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 120:
#line 2041 "hlsl.y" /* yacc.c:1646  */
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
                                    return -1;
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
                                    return 1;
                                }
                                if ((yyvsp[-1].instr)->data_type->type != HLSL_CLASS_SCALAR)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "array index is not scalar");
                                    d3dcompiler_free(deref);
                                    free_instr((yyvsp[-3].instr));
                                    free_instr((yyvsp[-1].instr));
                                    return 1;
                                }
                                deref->type = HLSL_IR_DEREF_ARRAY;
                                deref->v.array.array = (yyvsp[-3].instr);
                                deref->v.array.index = (yyvsp[-1].instr);

                                (yyval.instr) = &deref->node;
                            }
#line 4046 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 121:
#line 2101 "hlsl.y" /* yacc.c:1646  */
    {
                                struct hlsl_ir_constructor *constructor;

                                TRACE("%s constructor.\n", debug_hlsl_type((yyvsp[-3].type)));
                                if ((yyvsp[-4].modifiers))
                                {
                                    hlsl_message("Line %u: unexpected modifier in a constructor.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return -1;
                                }
                                if ((yyvsp[-3].type)->type > HLSL_CLASS_LAST_NUMERIC)
                                {
                                    hlsl_message("Line %u: constructors are allowed only for numeric data types.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return -1;
                                }
                                if ((yyvsp[-3].type)->dimx * (yyvsp[-3].type)->dimy != components_count_expr_list((yyvsp[-1].list)))
                                {
                                    hlsl_message("Line %u: wrong number of components in constructor.\n",
                                            hlsl_ctx.line_no);
                                    set_parse_status(&hlsl_ctx.status, PARSE_ERR);
                                    return -1;
                                }

                                constructor = d3dcompiler_alloc(sizeof(*constructor));
                                constructor->node.type = HLSL_IR_CONSTRUCTOR;
                                set_location(&constructor->node.loc, &(yylsp[-2]));
                                constructor->node.data_type = (yyvsp[-3].type);
                                constructor->arguments = (yyvsp[-1].list);

                                (yyval.instr) = &constructor->node;
                            }
#line 4085 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 122:
#line 2137 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4093 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 123:
#line 2141 "hlsl.y" /* yacc.c:1646  */
    {
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                if ((yyvsp[0].instr)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "modifying a const expression");
                                    return 1;
                                }
                                operands[0] = (yyvsp[0].instr);
                                operands[1] = operands[2] = NULL;
                                (yyval.instr) = &new_expr(HLSL_IR_UNOP_PREINC, operands, &loc)->node;
                            }
#line 4113 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 124:
#line 2157 "hlsl.y" /* yacc.c:1646  */
    {
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                if ((yyvsp[0].instr)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "modifying a const expression");
                                    return 1;
                                }
                                operands[0] = (yyvsp[0].instr);
                                operands[1] = operands[2] = NULL;
                                (yyval.instr) = &new_expr(HLSL_IR_UNOP_PREDEC, operands, &loc)->node;
                            }
#line 4133 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 125:
#line 2173 "hlsl.y" /* yacc.c:1646  */
    {
                                enum hlsl_ir_expr_op ops[] = {0, HLSL_IR_UNOP_NEG,
                                        HLSL_IR_UNOP_LOGIC_NOT, HLSL_IR_UNOP_BIT_NOT};
                                struct hlsl_ir_node *operands[3];
                                struct source_location loc;

                                if ((yyvsp[-1].unary_op) == UNARY_OP_PLUS)
                                {
                                    (yyval.instr) = (yyvsp[0].instr);
                                }
                                else
                                {
                                    operands[0] = (yyvsp[0].instr);
                                    operands[1] = operands[2] = NULL;
                                    set_location(&loc, &(yylsp[-1]));
                                    (yyval.instr) = &new_expr(ops[(yyvsp[-1].unary_op)], operands, &loc)->node;
                                }
                            }
#line 4156 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 126:
#line 2193 "hlsl.y" /* yacc.c:1646  */
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
                                    return 1;
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
                                    return 1;
                                }

                                expr = new_cast((yyvsp[0].instr), dst_type, &loc);
                                (yyval.instr) = expr ? &expr->node : NULL;
                            }
#line 4191 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 127:
#line 2225 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.unary_op) = UNARY_OP_PLUS;
                            }
#line 4199 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 128:
#line 2229 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.unary_op) = UNARY_OP_MINUS;
                            }
#line 4207 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 129:
#line 2233 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.unary_op) = UNARY_OP_LOGICNOT;
                            }
#line 4215 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 130:
#line 2237 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.unary_op) = UNARY_OP_BITNOT;
                            }
#line 4223 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 131:
#line 2242 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4231 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 132:
#line 2246 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = &hlsl_mul((yyvsp[-2].instr), (yyvsp[0].instr), &loc)->node;
                            }
#line 4242 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 133:
#line 2253 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = &hlsl_div((yyvsp[-2].instr), (yyvsp[0].instr), &loc)->node;
                            }
#line 4253 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 134:
#line 2260 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = &hlsl_mod((yyvsp[-2].instr), (yyvsp[0].instr), &loc)->node;
                            }
#line 4264 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 135:
#line 2268 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4272 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 136:
#line 2272 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = &hlsl_add((yyvsp[-2].instr), (yyvsp[0].instr), &loc)->node;
                            }
#line 4283 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 137:
#line 2279 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = &hlsl_sub((yyvsp[-2].instr), (yyvsp[0].instr), &loc)->node;
                            }
#line 4294 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 138:
#line 2287 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4302 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 139:
#line 2291 "hlsl.y" /* yacc.c:1646  */
    {
                                FIXME("Left shift\n");
                            }
#line 4310 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 140:
#line 2295 "hlsl.y" /* yacc.c:1646  */
    {
                                FIXME("Right shift\n");
                            }
#line 4318 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 141:
#line 2300 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4326 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 142:
#line 2304 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = &hlsl_lt((yyvsp[-2].instr), (yyvsp[0].instr), &loc)->node;
                            }
#line 4337 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 143:
#line 2311 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = &hlsl_gt((yyvsp[-2].instr), (yyvsp[0].instr), &loc)->node;
                            }
#line 4348 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 144:
#line 2318 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = &hlsl_le((yyvsp[-2].instr), (yyvsp[0].instr), &loc)->node;
                            }
#line 4359 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 145:
#line 2325 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = &hlsl_ge((yyvsp[-2].instr), (yyvsp[0].instr), &loc)->node;
                            }
#line 4370 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 146:
#line 2333 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4378 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 147:
#line 2337 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = &hlsl_eq((yyvsp[-2].instr), (yyvsp[0].instr), &loc)->node;
                            }
#line 4389 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 148:
#line 2344 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                (yyval.instr) = &hlsl_ne((yyvsp[-2].instr), (yyvsp[0].instr), &loc)->node;
                            }
#line 4400 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 149:
#line 2352 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4408 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 150:
#line 2356 "hlsl.y" /* yacc.c:1646  */
    {
                                FIXME("bitwise AND\n");
                            }
#line 4416 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 151:
#line 2361 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4424 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 152:
#line 2365 "hlsl.y" /* yacc.c:1646  */
    {
                                FIXME("bitwise XOR\n");
                            }
#line 4432 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 153:
#line 2370 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4440 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 154:
#line 2374 "hlsl.y" /* yacc.c:1646  */
    {
                                FIXME("bitwise OR\n");
                            }
#line 4448 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 155:
#line 2379 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4456 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 156:
#line 2383 "hlsl.y" /* yacc.c:1646  */
    {
                                FIXME("logic AND\n");
                            }
#line 4464 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 157:
#line 2388 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4472 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 158:
#line 2392 "hlsl.y" /* yacc.c:1646  */
    {
                                FIXME("logic OR\n");
                            }
#line 4480 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 159:
#line 2397 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4488 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 160:
#line 2401 "hlsl.y" /* yacc.c:1646  */
    {
                                FIXME("ternary operator\n");
                            }
#line 4496 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 161:
#line 2406 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4504 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 162:
#line 2410 "hlsl.y" /* yacc.c:1646  */
    {
                                struct source_location loc;

                                set_location(&loc, &(yylsp[-1]));
                                if ((yyvsp[-2].instr)->data_type->modifiers & HLSL_MODIFIER_CONST)
                                {
                                    hlsl_report_message(loc.file, loc.line, loc.col, HLSL_LEVEL_ERROR,
                                            "l-value is const");
                                    return 1;
                                }
                                (yyval.instr) = make_assignment((yyvsp[-2].instr), (yyvsp[-1].assign_op), BWRITERSP_WRITEMASK_ALL, (yyvsp[0].instr));
                                if (!(yyval.instr))
                                    return 1;
                                (yyval.instr)->loc = loc;
                            }
#line 4524 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 163:
#line 2427 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.assign_op) = ASSIGN_OP_ASSIGN;
                            }
#line 4532 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 164:
#line 2431 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.assign_op) = ASSIGN_OP_ADD;
                            }
#line 4540 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 165:
#line 2435 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.assign_op) = ASSIGN_OP_SUB;
                            }
#line 4548 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 166:
#line 2439 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.assign_op) = ASSIGN_OP_MUL;
                            }
#line 4556 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 167:
#line 2443 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.assign_op) = ASSIGN_OP_DIV;
                            }
#line 4564 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 168:
#line 2447 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.assign_op) = ASSIGN_OP_MOD;
                            }
#line 4572 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 169:
#line 2451 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.assign_op) = ASSIGN_OP_LSHIFT;
                            }
#line 4580 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 170:
#line 2455 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.assign_op) = ASSIGN_OP_RSHIFT;
                            }
#line 4588 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 171:
#line 2459 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.assign_op) = ASSIGN_OP_AND;
                            }
#line 4596 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 172:
#line 2463 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.assign_op) = ASSIGN_OP_OR;
                            }
#line 4604 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 173:
#line 2467 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.assign_op) = ASSIGN_OP_XOR;
                            }
#line 4612 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 174:
#line 2472 "hlsl.y" /* yacc.c:1646  */
    {
                                (yyval.instr) = (yyvsp[0].instr);
                            }
#line 4620 "hlsl.tab.c" /* yacc.c:1646  */
    break;

  case 175:
#line 2476 "hlsl.y" /* yacc.c:1646  */
    {
                                FIXME("Comma expression\n");
                            }
#line 4628 "hlsl.tab.c" /* yacc.c:1646  */
    break;


#line 4632 "hlsl.tab.c" /* yacc.c:1646  */
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

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[1] = yylsp[1-yylen];
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
#line 2480 "hlsl.y" /* yacc.c:1906  */


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
