/*
 * HLSL preprocessor
 *
 * Copyright 2020 Zebediah Figura for CodeWeavers
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

%code requires
{

#include "vkd3d_shader_private.h"
#include "preproc.h"
#include <stdio.h>
#include <sys/stat.h>

#define PREPROC_YYLTYPE struct vkd3d_shader_location

struct parse_arg_names
{
    char **args;
    size_t count;
};

}

%code provides
{

int preproc_yylex(PREPROC_YYSTYPE *yylval_param, PREPROC_YYLTYPE *yylloc_param, void *scanner);

}

%code
{

#define YYLLOC_DEFAULT(cur, rhs, n) (cur) = YYRHSLOC(rhs, !!n)

#ifndef S_ISREG
# define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

static void preproc_error(struct preproc_ctx *ctx, const struct vkd3d_shader_location *loc,
        enum vkd3d_shader_error error, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vkd3d_shader_verror(ctx->message_context, loc, error, format, args);
    va_end(args);
    ctx->error = true;
}

void preproc_warning(struct preproc_ctx *ctx, const struct vkd3d_shader_location *loc,
        enum vkd3d_shader_error error, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vkd3d_shader_vwarning(ctx->message_context, loc, error, format, args);
    va_end(args);
}

static void yyerror(const YYLTYPE *loc, void *scanner, struct preproc_ctx *ctx, const char *string)
{
    preproc_error(ctx, loc, VKD3D_SHADER_ERROR_PP_INVALID_SYNTAX, "%s", string);
}

struct preproc_macro *preproc_find_macro(struct preproc_ctx *ctx, const char *name)
{
    struct rb_entry *entry;

    if ((entry = rb_get(&ctx->macros, name)))
        return RB_ENTRY_VALUE(entry, struct preproc_macro, entry);
    return NULL;
}

bool preproc_add_macro(struct preproc_ctx *ctx, const struct vkd3d_shader_location *loc, char *name, char **arg_names,
        size_t arg_count, const struct vkd3d_shader_location *body_loc, struct vkd3d_string_buffer *body)
{
    struct preproc_macro *macro;
    int ret;

    if ((macro = preproc_find_macro(ctx, name)))
    {
        preproc_warning(ctx, loc, VKD3D_SHADER_WARNING_PP_ALREADY_DEFINED, "Redefinition of %s.", name);
        rb_remove(&ctx->macros, &macro->entry);
        preproc_free_macro(macro);
    }

    TRACE("Defining new macro %s with %zu arguments.\n", debugstr_a(name), arg_count);

    if (!(macro = vkd3d_malloc(sizeof(*macro))))
        return false;
    macro->name = name;
    macro->arg_names = arg_names;
    macro->arg_count = arg_count;
    macro->body.text = *body;
    macro->body.location = *body_loc;
    ret = rb_put(&ctx->macros, name, &macro->entry);
    VKD3D_ASSERT(!ret);
    return true;
}

void preproc_free_macro(struct preproc_macro *macro)
{
    unsigned int i;

    vkd3d_free(macro->name);
    for (i = 0; i < macro->arg_count; ++i)
        vkd3d_free(macro->arg_names[i]);
    vkd3d_free(macro->arg_names);
    vkd3d_string_buffer_cleanup(&macro->body.text);
    vkd3d_free(macro);
}

static bool preproc_was_writing(struct preproc_ctx *ctx)
{
    const struct preproc_file *file = preproc_get_top_file(ctx);

    /* This applies across files, since we can't #include anyway if we weren't
     * writing. */
    if (file->if_count < 2)
        return true;
    return file->if_stack[file->if_count - 2].current_true;
}

static bool preproc_push_if(struct preproc_ctx *ctx, bool condition)
{
    struct preproc_file *file = preproc_get_top_file(ctx);
    struct preproc_if_state *state;

    if (!vkd3d_array_reserve((void **)&file->if_stack, &file->if_stack_size,
            file->if_count + 1, sizeof(*file->if_stack)))
        return false;
    state = &file->if_stack[file->if_count++];
    state->current_true = condition && preproc_was_writing(ctx);
    state->seen_true = condition;
    state->seen_else = false;
    return true;
}

static int default_open_include(const char *filename, bool local,
        const char *parent_data, void *context, struct vkd3d_shader_code *out)
{
    uint8_t *data, *new_data;
    size_t size = 4096;
    struct stat st;
    size_t pos = 0;
    size_t ret;
    FILE *f;

    if (!(f = fopen(filename, "rb")))
    {
        ERR("Unable to open %s for reading.\n", debugstr_a(filename));
        return VKD3D_ERROR;
    }

    if (fstat(fileno(f), &st) == -1)
    {
        ERR("Could not stat file %s.\n", debugstr_a(filename));
        fclose(f);
        return VKD3D_ERROR;
    }

    if (S_ISREG(st.st_mode))
        size = st.st_size;

    if (!(data = vkd3d_malloc(size)))
    {
        fclose(f);
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    for (;;)
    {
        if (pos >= size)
        {
            if (size > SIZE_MAX / 2 || !(new_data = vkd3d_realloc(data, size * 2)))
            {
                vkd3d_free(data);
                fclose(f);
                return VKD3D_ERROR_OUT_OF_MEMORY;
            }
            data = new_data;
            size *= 2;
        }

        if (!(ret = fread(&data[pos], 1, size - pos, f)))
            break;
        pos += ret;
    }

    if (!feof(f))
    {
        vkd3d_free(data);
        return VKD3D_ERROR;
    }

    fclose(f);

    out->code = data;
    out->size = pos;

    return VKD3D_OK;
}

static void default_close_include(const struct vkd3d_shader_code *code, void *context)
{
    vkd3d_free((void *)code->code);
}

void preproc_close_include(struct preproc_ctx *ctx, const struct vkd3d_shader_code *code)
{
    PFN_vkd3d_shader_close_include close_include = ctx->preprocess_info->pfn_close_include;

    if (!close_include)
        close_include = default_close_include;

    close_include(code, ctx->preprocess_info->include_context);
}

static const void *get_parent_data(struct preproc_ctx *ctx)
{
    if (ctx->file_count == 1)
        return NULL;
    return preproc_get_top_file(ctx)->code.code;
}

static void free_parse_arg_names(struct parse_arg_names *args)
{
    unsigned int i;

    for (i = 0; i < args->count; ++i)
        vkd3d_free(args->args[i]);
    vkd3d_free(args->args);
}

}

%define api.prefix {preproc_yy}
%define api.pure full
%define parse.error verbose
%expect 0
%locations
%lex-param {yyscan_t scanner}
%parse-param {void *scanner}
%parse-param {struct preproc_ctx *ctx}

%union
{
    char *string;
    const char *const_string;
    uint32_t integer;
    struct vkd3d_string_buffer string_buffer;
    struct parse_arg_names arg_names;
}

%token <string> T_HASHSTRING
%token <string> T_IDENTIFIER
%token <string> T_IDENTIFIER_PAREN
%token <string> T_INTEGER
%token <string> T_STRING
%token <string> T_TEXT

%token T_NEWLINE

%token T_DEFINE "#define"
%token T_ERROR "#error"
%token T_ELIF "#elif"
%token T_ELSE "#else"
%token T_ENDIF "#endif"
%token T_IF "#if"
%token T_IFDEF "#ifdef"
%token T_IFNDEF "#ifndef"
%token T_INCLUDE "#include"
%token T_LINE "#line"
%token T_PRAGMA "#pragma"
%token T_UNDEF "#undef"

%token T_CONCAT "##"

%token T_LE "<="
%token T_GE ">="
%token T_EQ "=="
%token T_NE "!="
%token T_AND "&&"
%token T_OR "||"
%token T_DEFINED "defined"

%type <integer> primary_expr
%type <integer> unary_expr
%type <integer> mul_expr
%type <integer> add_expr
%type <integer> ineq_expr
%type <integer> eq_expr
%type <integer> bitand_expr
%type <integer> bitxor_expr
%type <integer> bitor_expr
%type <integer> logicand_expr
%type <integer> logicor_expr
%type <integer> expr
%type <string> body_token
%type <const_string> body_token_const
%type <string_buffer> body_text
%type <arg_names> identifier_list

%%

shader_text
    : %empty
    | shader_text directive
        {
            vkd3d_string_buffer_printf(&ctx->buffer, "\n");
        }

identifier_list
    : T_IDENTIFIER
        {
            if (!($$.args = vkd3d_malloc(sizeof(*$$.args))))
                YYABORT;
            $$.args[0] = $1;
            $$.count = 1;
        }
    | identifier_list ',' T_IDENTIFIER
        {
            char **new_array;

            if (!(new_array = vkd3d_realloc($1.args, ($1.count + 1) * sizeof(*$$.args))))
            {
                free_parse_arg_names(&$1);
                YYABORT;
            }
            $$.args = new_array;
            $$.count = $1.count + 1;
            $$.args[$1.count] = $3;
        }

body_text
    : %empty
        {
            vkd3d_string_buffer_init(&$$);
        }
    | body_text body_token
        {
            if (vkd3d_string_buffer_printf(&$$, "%s ", $2) < 0)
            {
                vkd3d_free($2);
                YYABORT;
            }
            vkd3d_free($2);
        }
    | body_text body_token_const
        {
            if (vkd3d_string_buffer_printf(&$$, "%s ", $2) < 0)
                YYABORT;
        }

body_token
    : T_HASHSTRING
    | T_IDENTIFIER
    | T_IDENTIFIER_PAREN
    | T_INTEGER
    | T_TEXT

body_token_const
    : '('
        {
            $$ = "(";
        }
    | ')'
        {
            $$ = ")";
        }
    | '['
        {
            $$ = "[";
        }
    | ']'
        {
            $$ = "]";
        }
    | '{'
        {
            $$ = "{";
        }
    | '}'
        {
            $$ = "}";
        }
    | ','
        {
            $$ = ",";
        }
    | '+'
        {
            $$ = "+";
        }
    | '-'
        {
            $$ = "-";
        }
    | '!'
        {
            $$ = "!";
        }
    | '*'
        {
            $$ = "*";
        }
    | '/'
        {
            $$ = "/";
        }
    | '<'
        {
            $$ = "<";
        }
    | '>'
        {
            $$ = ">";
        }
    | '&'
        {
            $$ = "&";
        }
    | '|'
        {
            $$ = "|";
        }
    | '^'
        {
            $$ = "^";
        }
    | '?'
        {
            $$ = "?";
        }
    | ':'
        {
            $$ = ":";
        }
    | T_CONCAT
        {
            $$ = "##";
        }
    | T_LE
        {
            $$ = "<=";
        }
    | T_GE
        {
            $$ = ">=";
        }
    | T_EQ
        {
            $$ = "==";
        }
    | T_NE
        {
            $$ = "!=";
        }
    | T_AND
        {
            $$ = "&&";
        }
    | T_OR
        {
            $$ = "||";
        }
    | T_DEFINED
        {
            $$ = "defined";
        }

directive
    : T_DEFINE T_IDENTIFIER body_text T_NEWLINE
        {
            if (!preproc_add_macro(ctx, &@$, $2, NULL, 0, &@3, &$3))
            {
                vkd3d_free($2);
                vkd3d_string_buffer_cleanup(&$3);
                YYABORT;
            }
        }
    | T_DEFINE T_IDENTIFIER_PAREN '(' identifier_list ')' body_text T_NEWLINE
        {
            if (!preproc_add_macro(ctx, &@6, $2, $4.args, $4.count, &@6, &$6))
            {
                vkd3d_free($2);
                free_parse_arg_names(&$4);
                vkd3d_string_buffer_cleanup(&$6);
                YYABORT;
            }
        }
    | T_UNDEF T_IDENTIFIER T_NEWLINE
        {
            struct preproc_macro *macro;

            if ((macro = preproc_find_macro(ctx, $2)))
            {
                TRACE("Removing macro definition %s.\n", debugstr_a($2));
                rb_remove(&ctx->macros, &macro->entry);
                preproc_free_macro(macro);
            }
            vkd3d_free($2);
        }
    | T_IF expr T_NEWLINE
        {
            if (!preproc_push_if(ctx, !!$2))
                YYABORT;
        }
    | T_IFDEF T_IDENTIFIER T_NEWLINE
        {
            preproc_push_if(ctx, !!preproc_find_macro(ctx, $2));
            vkd3d_free($2);
        }
    | T_IFNDEF T_IDENTIFIER T_NEWLINE
        {
            preproc_push_if(ctx, !preproc_find_macro(ctx, $2));
            vkd3d_free($2);
        }
    | T_ELIF expr T_NEWLINE
        {
            const struct preproc_file *file = preproc_get_top_file(ctx);

            if (file->if_count)
            {
                struct preproc_if_state *state = &file->if_stack[file->if_count - 1];

                if (state->seen_else)
                {
                    preproc_warning(ctx, &@$, VKD3D_SHADER_WARNING_PP_INVALID_DIRECTIVE, "Ignoring #elif after #else.");
                }
                else
                {
                    state->current_true = $2 && !state->seen_true && preproc_was_writing(ctx);
                    state->seen_true = $2 || state->seen_true;
                }
            }
            else
            {
                preproc_warning(ctx, &@$, VKD3D_SHADER_WARNING_PP_INVALID_DIRECTIVE,
                        "Ignoring #elif without prior #if.");
            }
        }
    | T_ELSE T_NEWLINE
        {
            const struct preproc_file *file = preproc_get_top_file(ctx);

            if (file->if_count)
            {
                struct preproc_if_state *state = &file->if_stack[file->if_count - 1];

                if (state->seen_else)
                {
                    preproc_warning(ctx, &@$, VKD3D_SHADER_WARNING_PP_INVALID_DIRECTIVE, "Ignoring #else after #else.");
                }
                else
                {
                    state->current_true = !state->seen_true && preproc_was_writing(ctx);
                    state->seen_else = true;
                }
            }
            else
            {
                preproc_warning(ctx, &@$, VKD3D_SHADER_WARNING_PP_INVALID_DIRECTIVE,
                        "Ignoring #else without prior #if.");
            }
        }
    | T_ENDIF T_NEWLINE
        {
            struct preproc_file *file = preproc_get_top_file(ctx);

            if (file->if_count)
                --file->if_count;
            else
                preproc_warning(ctx, &@$, VKD3D_SHADER_WARNING_PP_INVALID_DIRECTIVE,
                        "Ignoring #endif without prior #if.");
        }
    | T_ERROR T_NEWLINE
        {
            preproc_error(ctx, &@$, VKD3D_SHADER_ERROR_PP_ERROR_DIRECTIVE, "Error directive.");
        }
    | T_ERROR T_STRING T_NEWLINE
        {
            preproc_error(ctx, &@$, VKD3D_SHADER_ERROR_PP_ERROR_DIRECTIVE, "Error directive: %s", $2);
            vkd3d_free($2);
        }
    | T_INCLUDE T_STRING T_NEWLINE
        {
            PFN_vkd3d_shader_open_include open_include = ctx->preprocess_info->pfn_open_include;
            struct vkd3d_shader_code code;
            char *filename;
            int result;

            if (!(filename = vkd3d_malloc(strlen($2) - 1)))
                YYABORT;

            if (!open_include)
                open_include = default_open_include;

            memcpy(filename, $2 + 1, strlen($2) - 2);
            filename[strlen($2) - 2] = 0;

            if (!(result = open_include(filename, $2[0] == '"', get_parent_data(ctx),
                    ctx->preprocess_info->include_context, &code)))
            {
                if (!preproc_push_include(ctx, filename, &code))
                {
                    preproc_close_include(ctx, &code);
                    vkd3d_free(filename);
                }
            }
            else
            {
                preproc_error(ctx, &@$, VKD3D_SHADER_ERROR_PP_INCLUDE_FAILED, "Failed to open %s.", $2);
                vkd3d_free(filename);
            }
            vkd3d_free($2);
        }
    | T_LINE T_INTEGER T_NEWLINE
        {
            FIXME("#line directive.\n");
            vkd3d_free($2);
        }
    | T_LINE T_INTEGER T_STRING T_NEWLINE
        {
            FIXME("#line directive.\n");
            vkd3d_free($2);
            vkd3d_free($3);
        }

primary_expr
    : T_INTEGER
        {
            $$ = vkd3d_parse_integer($1);
            vkd3d_free($1);
        }
    | T_IDENTIFIER
        {
            $$ = 0;
            vkd3d_free($1);
        }
    | T_DEFINED T_IDENTIFIER
        {
            $$ = !!preproc_find_macro(ctx, $2);
            vkd3d_free($2);
        }
    | T_DEFINED '(' T_IDENTIFIER ')'
        {
            $$ = !!preproc_find_macro(ctx, $3);
            vkd3d_free($3);
        }
    | '(' expr ')'
        {
            $$ = $2;
        }

unary_expr
    : primary_expr
    | '+' unary_expr
        {
            $$ = $2;
        }
    | '-' unary_expr
        {
            $$ = -$2;
        }
    | '!' unary_expr
        {
            $$ = !$2;
        }

mul_expr
    : unary_expr
    | mul_expr '*' unary_expr
        {
            $$ = $1 * $3;
        }
    | mul_expr '/' unary_expr
        {
            if (!$3)
            {
                preproc_warning(ctx, &@3, VKD3D_SHADER_WARNING_PP_DIV_BY_ZERO, "Division by zero.");
                $3 = 1;
            }
            $$ = $1 / $3;
        }

add_expr
    : mul_expr
    | add_expr '+' mul_expr
        {
            $$ = $1 + $3;
        }
    | add_expr '-' mul_expr
        {
            $$ = $1 - $3;
        }

ineq_expr
    : add_expr
    | ineq_expr '<' add_expr
        {
            $$ = $1 < $3;
        }
    | ineq_expr '>' add_expr
        {
            $$ = $1 > $3;
        }
    | ineq_expr T_LE add_expr
        {
            $$ = $1 <= $3;
        }
    | ineq_expr T_GE add_expr
        {
            $$ = $1 >= $3;
        }

eq_expr
    : ineq_expr
    | eq_expr T_EQ ineq_expr
        {
            $$ = $1 == $3;
        }
    | eq_expr T_NE ineq_expr
        {
            $$ = $1 != $3;
        }

bitand_expr
    : eq_expr
    | bitand_expr '&' eq_expr
        {
            $$ = $1 & $3;
        }

bitxor_expr
    : bitand_expr
    | bitxor_expr '^' bitand_expr
        {
            $$ = $1 ^ $3;
        }

bitor_expr
    : bitxor_expr
    | bitor_expr '|' bitxor_expr
        {
            $$ = $1 | $3;
        }

logicand_expr
    : bitor_expr
    | logicand_expr T_AND bitor_expr
        {
            $$ = $1 && $3;
        }

logicor_expr
    : logicand_expr
    | logicor_expr T_OR logicand_expr
        {
            $$ = $1 || $3;
        }

expr
    : logicor_expr
    | expr '?' logicor_expr ':' logicor_expr
        {
            $$ = $1 ? $3 : $5;
        }
