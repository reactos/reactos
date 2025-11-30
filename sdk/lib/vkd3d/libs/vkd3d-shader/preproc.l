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

%{

#include "preproc.tab.h"

#undef ERROR  /* defined in wingdi.h */

#define YYSTYPE PREPROC_YYSTYPE
#define YYLTYPE PREPROC_YYLTYPE

#define YY_DECL static int preproc_lexer_lex(YYSTYPE *yylval_param, YYLTYPE *yylloc_param, yyscan_t yyscanner)

static struct preproc_expansion *preproc_get_top_expansion(struct preproc_ctx *ctx)
{
    if (!ctx->expansion_count)
        return NULL;
    return &ctx->expansion_stack[ctx->expansion_count - 1];
}

static void update_location(struct preproc_ctx *ctx);

#define YY_USER_ACTION update_location(yyget_extra(yyscanner));

%}

%option 8bit
%option bison-bridge
%option bison-locations
%option extra-type="struct preproc_ctx *"
%option never-interactive
%option nodefault
%option noinput
%option nounput
%option noyy_top_state
%option noyywrap
%option prefix="preproc_yy"
%option reentrant
%option stack

    /* Because these can both be terminated by EOF, we need states for them. */
%s C_COMMENT
%s CXX_COMMENT

%s ERROR
%s INCLUDE
%s LINE

NEWLINE         \r?\n
WS              [ \t\r]
IDENTIFIER      (::)?[A-Za-z_]((::)?[A-Za-z0-9_]+)*
INT_SUFFIX      [uUlL]{0,2}

%%

<INITIAL>"//"                       {yy_push_state(CXX_COMMENT, yyscanner);}
<INITIAL>"/*"                       {yy_push_state(C_COMMENT, yyscanner);}
<CXX_COMMENT>\\{NEWLINE}            {}
<CXX_COMMENT>\n                     {
        yy_pop_state(yyscanner);
        BEGIN(INITIAL);
        return T_NEWLINE;
    }
<C_COMMENT>"*/"                     {yy_pop_state(yyscanner);}
<C_COMMENT,CXX_COMMENT><<EOF>>      {yy_pop_state(yyscanner);}
<C_COMMENT,CXX_COMMENT>.            {}
<C_COMMENT>\n                       {}

<ERROR>(\\{NEWLINE}|[^\n])*         {return T_STRING;}

<INITIAL>defined/\(                 {return T_DEFINED;}
<INITIAL>defined                    {return T_DEFINED;}
<INITIAL>{IDENTIFIER}/\(            {return T_IDENTIFIER_PAREN;}
<INITIAL>{IDENTIFIER}               {return T_IDENTIFIER;}

<INITIAL>"<="                       {return T_LE;}
<INITIAL>">="                       {return T_GE;}
<INITIAL>"=="                       {return T_EQ;}
<INITIAL>"!="                       {return T_NE;}
<INITIAL>"&&"                       {return T_AND;}
<INITIAL>"||"                       {return T_OR;}

    /* We have no use for floats, but shouldn't parse them as integers. */

<INITIAL>[0-9]*\.[0-9]+([eE][+-]?[0-9]+)?[hHfF]?    {return T_TEXT;}
<INITIAL>[0-9]+\.([eE][+-]?[0-9]+)?[hHfF]?          {return T_TEXT;}
<INITIAL>[0-9]+([eE][+-]?[0-9]+)?[hHfF]             {return T_TEXT;}
<INITIAL>[0-9]+[eE][+-]?[0-9]+                      {return T_TEXT;}
<INITIAL,LINE>0[xX][0-9a-fA-f]+{INT_SUFFIX}         {return T_INTEGER;}
<INITIAL,LINE>0[0-7]*{INT_SUFFIX}                   {return T_INTEGER;}
<INITIAL,LINE>[1-9][0-9]*{INT_SUFFIX}               {return T_INTEGER;}

<INITIAL>##                         {return T_CONCAT;}

<INITIAL>"++"                       {return T_TEXT;}
<INITIAL>"--"                       {return T_TEXT;}
<INITIAL>"<<"=?                     {return T_TEXT;}
<INITIAL>">>"=?                     {return T_TEXT;}
<INITIAL>[-+*/%&|^]=                {return T_TEXT;}

    /* Native doesn't preserve these tokens when running the preprocessor on its
     * own, but there's no good reason to emulate that difference yet. */
<INITIAL>[pv]s\.[123]\.[0-4x]       {return T_TEXT;}

<INCLUDE,LINE>\"[^"]*\"             {return T_STRING;}
<INCLUDE>\<[^>]*\>                  {return T_STRING;}

    /* C strings (including escaped quotes). */
<INITIAL>\"([^"\\]|\\.)*\"          {return T_TEXT;}

<INITIAL>#{WS}*{IDENTIFIER}         {
        struct preproc_ctx *ctx = yyget_extra(yyscanner);
        const char *p;

        if (!ctx->last_was_newline)
        {
            struct preproc_expansion *exp;

            /* Stringification is only done for function-like macro bodies.
             * Anywhere else, we need to parse it as two separate tokens.
             * We could use a state for this, but yyless() is easier and cheap.
             */

            if ((exp = preproc_get_top_expansion(ctx)) && exp->macro && exp->macro->arg_count)
                return T_HASHSTRING;

            yyless(1);
            return T_TEXT;
        }

        for (p = yytext + 1; strchr(" \t", *p); ++p)
            ;

        if (!strcmp(p, "error"))
        {
            BEGIN(ERROR);
            return T_ERROR;
        }

        if (!strcmp(p, "include"))
        {
            BEGIN(INCLUDE);
            return T_INCLUDE;
        }

        if (!strcmp(p, "line"))
        {
            BEGIN(LINE);
            return T_LINE;
        }

        if (!strcmp(p, "define"))
            return T_DEFINE;
        if (!strcmp(p, "elif"))
            return T_ELIF;
        if (!strcmp(p, "else"))
            return T_ELSE;
        if (!strcmp(p, "endif"))
            return T_ENDIF;
        if (!strcmp(p, "if"))
            return T_IF;
        if (!strcmp(p, "ifdef"))
            return T_IFDEF;
        if (!strcmp(p, "ifndef"))
            return T_IFNDEF;
        if (!strcmp(p, "pragma"))
            return T_PRAGMA;
        if (!strcmp(p, "undef"))
            return T_UNDEF;

        preproc_warning(ctx, yyget_lloc(yyscanner), VKD3D_SHADER_WARNING_PP_UNKNOWN_DIRECTIVE,
                "Ignoring unknown directive \"%s\".", yytext);
        return T_TEXT;
    }

<INITIAL,INCLUDE,LINE>\\{NEWLINE}       {}
<INITIAL,INCLUDE,ERROR,LINE>{NEWLINE}   {
        BEGIN(INITIAL);
        return T_NEWLINE;
    }

<INITIAL,INCLUDE,LINE>{WS}+         {}
<INITIAL>[-()\[\]{},+!*/<>&|^?:]    {return yytext[0];}
<INITIAL,INCLUDE,LINE>.             {return T_TEXT;}

%%

static void update_location(struct preproc_ctx *ctx)
{
    struct preproc_buffer *buffer = &preproc_get_top_file(ctx)->buffer;
    unsigned int i, leng = yyget_leng(ctx->scanner);
    const char *text = yyget_text(ctx->scanner);

    /* We want to do this here, rather than before calling yylex(), because
     * some tokens are skipped by the lexer. */

    *yyget_lloc(ctx->scanner) = buffer->location;

    for (i = 0; i < leng; ++i)
    {
        ++buffer->location.column;
        if (text[i] == '\n')
        {
            buffer->location.column = 1;
            ++buffer->location.line;
        }
    }
}

static bool preproc_is_writing(struct preproc_ctx *ctx)
{
    const struct preproc_file *file;

    /* This can happen while checking for unterminated macro invocation. */
    if (!ctx->file_count)
        return true;
    file = preproc_get_top_file(ctx);
    if (!file->if_count)
        return true;
    return file->if_stack[file->if_count - 1].current_true;
}

/* Concatenation is not done for object-like macros, but is done for both
 * function-like macro bodies and their arguments. */
static bool should_concat(struct preproc_ctx *ctx)
{
    struct preproc_macro *macro;

    if (!ctx->expansion_count)
        return false;
    macro = ctx->expansion_stack[ctx->expansion_count - 1].macro;
    return !macro || macro->arg_count;
}

static void preproc_pop_buffer(struct preproc_ctx *ctx)
{
    if (ctx->expansion_count)
    {
        struct preproc_expansion *exp = &ctx->expansion_stack[ctx->expansion_count - 1];

        yy_delete_buffer(exp->buffer.lexer_buffer, ctx->scanner);

        if (exp->macro)
        {
            for (unsigned int i = 0; i < exp->macro->arg_count; ++i)
                vkd3d_string_buffer_cleanup(&exp->arg_values[i].text);
            free(exp->arg_values);
        }
        --ctx->expansion_count;
        TRACE("Expansion stack size is now %zu.\n", ctx->expansion_count);
    }
    else
    {
        struct preproc_file *file = preproc_get_top_file(ctx);

        if (ctx->file_count > 1)
            preproc_close_include(ctx, &file->code);

        if (file->if_count)
        {
            const struct vkd3d_shader_location loc = {.source_name = file->filename};

            preproc_warning(ctx, &loc, VKD3D_SHADER_WARNING_PP_UNTERMINATED_IF, "Unterminated #if block.");
        }
        vkd3d_free(file->if_stack);

        vkd3d_free(file->filename);

        yy_delete_buffer(file->buffer.lexer_buffer, ctx->scanner);

        --ctx->file_count;
        TRACE("File stack size is now %zu.\n", ctx->file_count);
    }

    if (ctx->expansion_count)
        yy_switch_to_buffer(ctx->expansion_stack[ctx->expansion_count - 1].buffer.lexer_buffer, ctx->scanner);
    else if (ctx->file_count)
        yy_switch_to_buffer(ctx->file_stack[ctx->file_count - 1].buffer.lexer_buffer, ctx->scanner);
}

static int return_token(int token, YYSTYPE *lval, const char *text)
{
    switch (token)
    {
        case T_HASHSTRING:
        case T_IDENTIFIER:
        case T_IDENTIFIER_PAREN:
        case T_INTEGER:
        case T_STRING:
        case T_TEXT:
            if (!(lval->string = vkd3d_strdup(text)))
                return 0;
            break;
    }

    return token;
}

static const struct preproc_text *find_arg_expansion(struct preproc_ctx *ctx, const char *s)
{
    struct preproc_expansion *exp;
    unsigned int i;

    if ((exp = preproc_get_top_expansion(ctx)) && exp->macro)
    {
        for (i = 0; i < exp->macro->arg_count; ++i)
        {
            if (!strcmp(s, exp->macro->arg_names[i]))
                return &exp->arg_values[i];
        }
    }
    return NULL;
}

static void preproc_text_add(struct preproc_text *text, const char *string)
{
    vkd3d_string_buffer_printf(&text->text, "%s", string);
}

static bool preproc_push_expansion(struct preproc_ctx *ctx,
        const struct preproc_text *text, struct preproc_macro *macro, struct preproc_text *arg_values)
{
    struct preproc_expansion *exp;

    if (!vkd3d_array_reserve((void **)&ctx->expansion_stack, &ctx->expansion_stack_size,
            ctx->expansion_count + 1, sizeof(*ctx->expansion_stack)))
        return false;
    exp = &ctx->expansion_stack[ctx->expansion_count++];
    exp->text = text;
    exp->buffer.lexer_buffer = yy_scan_bytes(text->text.buffer, text->text.content_size, ctx->scanner);
    exp->buffer.location = text->location;
    exp->macro = macro;
    exp->arg_values = arg_values;
    TRACE("Expansion stack size is now %zu.\n", ctx->expansion_count);
    return true;
}

static void preproc_stringify(struct preproc_ctx *ctx, struct vkd3d_string_buffer *buffer, const char *text)
{
    const struct preproc_text *expansion;
    const char *p = text + 1;
    unsigned int i;

    while (*p == ' ' || *p == '\t')
        ++p;

    vkd3d_string_buffer_printf(buffer, "\"");
    if ((expansion = find_arg_expansion(ctx, p)))
    {
        size_t len = expansion->text.content_size;
        size_t start = 0;

        while (len && strchr(" \t\r\n", expansion->text.buffer[len - 1]))
            --len;

        while (start < len && strchr(" \t\r\n", expansion->text.buffer[start]))
            ++start;

        for (i = start; i < len; ++i)
        {
            char c = expansion->text.buffer[i];

            if (c == '\\' || c == '"')
                vkd3d_string_buffer_printf(buffer, "\\");
            vkd3d_string_buffer_printf(buffer, "%c", c);
        }
    }
    else
    {
        vkd3d_string_buffer_printf(buffer, "%s", p);
    }
    vkd3d_string_buffer_printf(buffer, "\"");
}

int yylex(YYSTYPE *lval, YYLTYPE *lloc, yyscan_t scanner)
{
    struct preproc_ctx *ctx = yyget_extra(scanner);

    for (;;)
    {
        struct preproc_func_state *func_state;
        const char *text;
        int token;

        if (ctx->lookahead_token)
        {
            token = ctx->lookahead_token;
            text = yyget_text(scanner);
        }
        else
        {
            if (ctx->last_was_eof)
            {
                preproc_pop_buffer(ctx);
                if (!ctx->file_count)
                    return 0;
            }
            ctx->last_was_eof = false;

            VKD3D_ASSERT(ctx->file_count);
            if (!(token = preproc_lexer_lex(lval, lloc, scanner)))
            {
                ctx->last_was_eof = true;

                /* If we have reached the end of an included file, inject a newline. */
                if (ctx->expansion_count)
                    continue;
                token = T_NEWLINE;
                text = "\n";
            }
            else
            {
                text = yyget_text(scanner);
            }

            if (ctx->last_was_newline)
            {
                switch (token)
                {
                    case T_DEFINE:
                    case T_ELIF:
                    case T_ELSE:
                    case T_ENDIF:
                    case T_ERROR:
                    case T_IF:
                    case T_IFDEF:
                    case T_IFNDEF:
                    case T_INCLUDE:
                    case T_LINE:
                    case T_PRAGMA:
                    case T_UNDEF:
                        ctx->current_directive = token;
                        break;

                    default:
                        ctx->current_directive = 0;
                }
            }

            ctx->last_was_newline = (token == T_NEWLINE);
        }

        if (ctx->current_directive && token == T_DEFINED)
            ctx->last_was_defined = true;

        func_state = ctx->current_directive ? &ctx->directive_func : &ctx->text_func;

        TRACE("Parsing token %d%s, line %d, in directive %d, state %#x, string %s.\n",
                token, ctx->lookahead_token ? " (lookahead)" : "", lloc->line,
                ctx->current_directive, func_state->state, debugstr_a(text));

        ctx->lookahead_token = 0;

        switch (ctx->current_directive)
        {
            case T_ELIF:
            case T_ELSE:
            case T_ENDIF:
            case T_IF:
            case T_IFDEF:
            case T_IFNDEF:
                break;

            default:
                if (!preproc_is_writing(ctx))
                    continue;
        }

        if (ctx->current_directive == T_PRAGMA)
        {
            /* Print all tokens verbatim. */
            if (token == T_PRAGMA)
                vkd3d_string_buffer_printf(&ctx->buffer, "#pragma ");
            else
                vkd3d_string_buffer_printf(&ctx->buffer, "%s", text);
            continue;
        }

        switch (func_state->state)
        {
            case STATE_NONE:
                if (token == T_CONCAT && should_concat(ctx))
                {
                    while (ctx->buffer.content_size
                            && strchr(" \t\r\n", ctx->buffer.buffer[ctx->buffer.content_size - 1]))
                        --ctx->buffer.content_size;
                    break;
                }

                if (token == T_HASHSTRING)
                {
                    struct vkd3d_string_buffer buffer;

                    if (ctx->current_directive)
                        return return_token(token, lval, text);

                    vkd3d_string_buffer_init(&buffer);
                    preproc_stringify(ctx, &buffer, text);
                    vkd3d_string_buffer_printf(&ctx->buffer, "%s", buffer.buffer);
                    vkd3d_string_buffer_cleanup(&buffer);
                    break;
                }

                if (token == T_IDENTIFIER || token == T_IDENTIFIER_PAREN)
                {
                    const struct preproc_text *expansion;
                    struct preproc_macro *macro;

                    switch (ctx->current_directive)
                    {
                        case T_DEFINE:
                        case T_IFDEF:
                        case T_IFNDEF:
                        case T_UNDEF:
                            /* Return identifiers verbatim. */
                            return return_token(token, lval, text);

                        case T_IF:
                        case T_ELIF:
                            /* Return identifiers verbatim only if they're the
                             * argument to "defined". */
                            if (ctx->last_was_defined)
                            {
                                ctx->last_was_defined = false;
                                return return_token(token, lval, text);
                            }
                            break;
                    }

                    /* Otherwise, expand a macro if there is one. */

                    if ((expansion = find_arg_expansion(ctx, text)))
                    {
                        preproc_push_expansion(ctx, expansion, NULL, NULL);
                        continue;
                    }

                    if ((macro = preproc_find_macro(ctx, text)))
                    {
                        if (!macro->arg_count)
                        {
                            preproc_push_expansion(ctx, &macro->body, macro, NULL);
                        }
                        else
                        {
                            func_state->state = STATE_IDENTIFIER;
                            func_state->macro = macro;
                        }
                        continue;
                    }

                    if (!strcmp(text, "__FILE__"))
                    {
                        const struct preproc_file *file = preproc_get_top_file(ctx);

                        /* Not the current file name, but rather the file name
                         * before invoking any macros. */

                        if (ctx->current_directive)
                        {
                            char *string;

                            if (!(string = vkd3d_malloc(strlen(file->filename) + 3)))
                                return 0;
                            sprintf(string, "\"%s\"", file->filename);
                            lval->string = string;
                            return T_STRING;
                        }

                        if (preproc_is_writing(ctx))
                            vkd3d_string_buffer_printf(&ctx->buffer, "\"%s\" ", file->filename);
                        continue;
                    }

                    if (!strcmp(text, "__LINE__"))
                    {
                        const struct preproc_file *file = preproc_get_top_file(ctx);

                        /* Not the current line number, but rather the line
                         * number before invoking any macros. */

                        if (ctx->current_directive)
                        {
                            char string[13];

                            sprintf(string, "%d", file->buffer.location.line);
                            return return_token(T_INTEGER, lval, string);
                        }

                        if (preproc_is_writing(ctx))
                            vkd3d_string_buffer_printf(&ctx->buffer, "%d ", file->buffer.location.line);
                        continue;
                    }
                }

                if (ctx->current_directive)
                    return return_token(token, lval, text);

                if (isspace(text[0]))
                    vkd3d_string_buffer_printf(&ctx->buffer, "%s", text);
                else
                    vkd3d_string_buffer_printf(&ctx->buffer, "%s ", text);
                break;

            case STATE_IDENTIFIER:
                if (token == '(')
                {
                    struct preproc_text *arg_values;

                    if (!(arg_values = calloc(func_state->macro->arg_count, sizeof(*arg_values))))
                        return 0;

                    for (unsigned int i = 0; i < func_state->macro->arg_count; ++i)
                        vkd3d_string_buffer_init(&arg_values[i].text);
                    arg_values[0].location = *lloc;

                    func_state->arg_count = 0;
                    func_state->paren_depth = 1;
                    func_state->state = STATE_ARGS;
                    func_state->arg_values = arg_values;
                }
                else
                {
                    const char *name = func_state->macro->name;

                    ctx->lookahead_token = token;
                    func_state->macro = NULL;
                    func_state->state = STATE_NONE;

                    if (ctx->current_directive)
                        return return_token(T_IDENTIFIER, lval, name);

                    vkd3d_string_buffer_printf(&ctx->buffer, "%s ", name);
                }
                break;

            case STATE_ARGS:
            {
                struct preproc_text *current_arg = NULL;

                VKD3D_ASSERT(func_state->macro->arg_count);

                if (func_state->arg_count < func_state->macro->arg_count)
                    current_arg = &func_state->arg_values[func_state->arg_count];

                switch (token)
                {
                    /* Most text gets left alone (e.g. if it contains macros,
                     * the macros should be evaluated later).
                     * Arguments are a special case, and are replaced with
                     * their values immediately. */
                    case T_IDENTIFIER:
                    case T_IDENTIFIER_PAREN:
                    {
                        const struct preproc_text *expansion;

                        if ((expansion = find_arg_expansion(ctx, text)))
                        {
                            preproc_push_expansion(ctx, expansion, NULL, NULL);
                            continue;
                        }

                        if (current_arg)
                            preproc_text_add(current_arg, text);
                        break;
                    }

                    /* Stringification is another special case. Unsurprisingly,
                     * we need to stringify if this is an argument. More
                     * surprisingly, we need to stringify even if it's not. */
                    case T_HASHSTRING:
                    {
                        struct vkd3d_string_buffer buffer;

                        vkd3d_string_buffer_init(&buffer);
                        preproc_stringify(ctx, &buffer, text);
                        if (current_arg)
                            preproc_text_add(current_arg, buffer.buffer);
                        vkd3d_string_buffer_cleanup(&buffer);
                        break;
                    }

                    case T_NEWLINE:
                        if (current_arg)
                            preproc_text_add(current_arg, " ");
                        break;

                    case ')':
                    case ']':
                    case '}':
                        if (!--func_state->paren_depth)
                        {
                            if (++func_state->arg_count == func_state->macro->arg_count)
                            {
                                preproc_push_expansion(ctx, &func_state->macro->body,
                                        func_state->macro, func_state->arg_values);
                            }
                            else
                            {
                                preproc_warning(ctx, lloc, VKD3D_SHADER_WARNING_PP_ARGUMENT_COUNT_MISMATCH,
                                        "Wrong number of arguments to macro \"%s\": expected %zu, got %zu.",
                                        func_state->macro->name, func_state->macro->arg_count, func_state->arg_count);

                                if (ctx->current_directive)
                                    return return_token(T_IDENTIFIER, lval, func_state->macro->name);

                                vkd3d_string_buffer_printf(&ctx->buffer, "%s ", func_state->macro->name);
                            }
                            func_state->macro = NULL;
                            func_state->state = STATE_NONE;
                        }
                        else
                        {
                            if (current_arg)
                                preproc_text_add(current_arg, text);
                        }
                        break;

                    case ',':
                        if (func_state->paren_depth == 1)
                        {
                            ++func_state->arg_count;
                            if (current_arg)
                                current_arg->location = *lloc;
                        }
                        else if (current_arg)
                        {
                            preproc_text_add(current_arg, text);
                        }
                        break;

                    case '(':
                    case '[':
                    case '{':
                        ++func_state->paren_depth;
                        /* fall through */

                    default:
                        if (current_arg)
                            preproc_text_add(current_arg, text);
                }

                if (current_arg)
                    preproc_text_add(current_arg, " ");
                break;
            }
        }
    }
}

bool preproc_push_include(struct preproc_ctx *ctx, char *filename, const struct vkd3d_shader_code *code)
{
    struct preproc_file *file;

    if (!vkd3d_array_reserve((void **)&ctx->file_stack, &ctx->file_stack_size,
            ctx->file_count + 1, sizeof(*ctx->file_stack)))
        return false;
    file = &ctx->file_stack[ctx->file_count++];
    memset(file, 0, sizeof(*file));
    file->code = *code;
    file->filename = filename;
    file->buffer.lexer_buffer = yy_scan_bytes(code->code, code->size, ctx->scanner);
    file->buffer.location.source_name = file->filename;
    file->buffer.location.line = 1;
    file->buffer.location.column = 1;
    TRACE("File stack size is now %zu.\n", ctx->file_count);
    ctx->last_was_newline = true;
    return true;
}

static int preproc_macro_compare(const void *key, const struct rb_entry *entry)
{
    const struct preproc_macro *macro = RB_ENTRY_VALUE(entry, struct preproc_macro, entry);
    const char *name = key;

    return strcmp(name, macro->name);
}

static void preproc_macro_rb_free(struct rb_entry *entry, void *ctx)
{
    preproc_free_macro(RB_ENTRY_VALUE(entry, struct preproc_macro, entry));
}

int preproc_lexer_parse(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, struct vkd3d_shader_message_context *message_context)
{
    static const struct vkd3d_shader_preprocess_info default_preprocess_info = {0};
    struct preproc_ctx ctx = {0};
    char *source_name = NULL;
    void *output_code;
    unsigned int i;

    vkd3d_string_buffer_init(&ctx.buffer);
    rb_init(&ctx.macros, preproc_macro_compare);
    if (!(ctx.preprocess_info = vkd3d_find_struct(compile_info->next, PREPROCESS_INFO)))
        ctx.preprocess_info = &default_preprocess_info;
    ctx.message_context = message_context;

    if (!(source_name = vkd3d_strdup(compile_info->source_name ? compile_info->source_name : "<anonymous>")))
        goto fail;

    for (i = 0; i < ctx.preprocess_info->macro_count; ++i)
    {
        const struct vkd3d_shader_location loc = {.source_name = source_name};
        struct vkd3d_string_buffer body;
        char *name;

        vkd3d_string_buffer_init(&body);
        vkd3d_string_buffer_printf(&body, "%s", ctx.preprocess_info->macros[i].value);
        if (!(name = vkd3d_strdup(ctx.preprocess_info->macros[i].name)))
        {
            vkd3d_string_buffer_cleanup(&body);
            goto fail;
        }
        if (!preproc_add_macro(&ctx, &loc, name, NULL, 0, &loc, &body))
        {
            vkd3d_free(name);
            vkd3d_string_buffer_cleanup(&body);
            goto fail;
        }
    }

    yylex_init_extra(&ctx, &ctx.scanner);
    if (!preproc_push_include(&ctx, source_name, &compile_info->source))
    {
        yylex_destroy(ctx.scanner);
        goto fail;
    }

    preproc_yyparse(ctx.scanner, &ctx);

    switch (ctx.text_func.state)
    {
        case STATE_NONE:
            break;

        case STATE_ARGS:
        {
            const struct vkd3d_shader_location loc = {.source_name = source_name};

            preproc_warning(&ctx, &loc, VKD3D_SHADER_WARNING_PP_UNTERMINATED_MACRO,
                    "Unterminated macro invocation.");
        }
        /* fall through */

        case STATE_IDENTIFIER:
            if (preproc_is_writing(&ctx))
                vkd3d_string_buffer_printf(&ctx.buffer, "%s ", ctx.text_func.macro->name);
            break;
    }

    while (ctx.file_count)
        preproc_pop_buffer(&ctx);
    yylex_destroy(ctx.scanner);

    rb_destroy(&ctx.macros, preproc_macro_rb_free, NULL);
    vkd3d_free(ctx.file_stack);
    vkd3d_free(ctx.expansion_stack);

    if (ctx.error)
    {
        WARN("Failed to preprocess.\n");
        vkd3d_string_buffer_cleanup(&ctx.buffer);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    if (!(output_code = vkd3d_malloc(ctx.buffer.content_size)))
    {
        vkd3d_string_buffer_cleanup(&ctx.buffer);
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }
    memcpy(output_code, ctx.buffer.buffer, ctx.buffer.content_size);
    out->size = ctx.buffer.content_size;
    out->code = output_code;
    vkd3d_string_buffer_trace(&ctx.buffer);
    vkd3d_string_buffer_cleanup(&ctx.buffer);
    return VKD3D_OK;

fail:
    rb_destroy(&ctx.macros, preproc_macro_rb_free, NULL);
    vkd3d_free(source_name);
    vkd3d_string_buffer_cleanup(&ctx.buffer);
    return VKD3D_ERROR_OUT_OF_MEMORY;
}
