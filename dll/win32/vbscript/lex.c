/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

#ifdef __REACTOS__
#include <wine/config.h>
#include <wine/port.h>
#endif
#include <assert.h>
#include <limits.h>
#include <math.h>

#include "vbscript.h"
#include "parse.h"
#include "parser.tab.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);

static const struct {
    const WCHAR *word;
    int token;
} keywords[] = {
    {L"and",       tAND},
    {L"byref",     tBYREF},
    {L"byval",     tBYVAL},
    {L"call",      tCALL},
    {L"case",      tCASE},
    {L"class",     tCLASS},
    {L"const",     tCONST},
    {L"default",   tDEFAULT},
    {L"dim",       tDIM},
    {L"do",        tDO},
    {L"each",      tEACH},
    {L"else",      tELSE},
    {L"elseif",    tELSEIF},
    {L"empty",     tEMPTY},
    {L"end",       tEND},
    {L"eqv",       tEQV},
    {L"error",     tERROR},
    {L"exit",      tEXIT},
    {L"explicit",  tEXPLICIT},
    {L"false",     tFALSE},
    {L"for",       tFOR},
    {L"function",  tFUNCTION},
    {L"get",       tGET},
    {L"goto",      tGOTO},
    {L"if",        tIF},
    {L"imp",       tIMP},
    {L"in",        tIN},
    {L"is",        tIS},
    {L"let",       tLET},
    {L"loop",      tLOOP},
    {L"me",        tME},
    {L"mod",       tMOD},
    {L"new",       tNEW},
    {L"next",      tNEXT},
    {L"not",       tNOT},
    {L"nothing",   tNOTHING},
    {L"null",      tNULL},
    {L"on",        tON},
    {L"option",    tOPTION},
    {L"or",        tOR},
    {L"preserve",  tPRESERVE},
    {L"private",   tPRIVATE},
    {L"property",  tPROPERTY},
    {L"public",    tPUBLIC},
    {L"redim",     tREDIM},
    {L"rem",       tREM},
    {L"resume",    tRESUME},
    {L"select",    tSELECT},
    {L"set",       tSET},
    {L"step",      tSTEP},
    {L"stop",      tSTOP},
    {L"sub",       tSUB},
    {L"then",      tTHEN},
    {L"to",        tTO},
    {L"true",      tTRUE},
    {L"until",     tUNTIL},
    {L"wend",      tWEND},
    {L"while",     tWHILE},
    {L"with",      tWITH},
    {L"xor",       tXOR}
};

static inline BOOL is_identifier_char(WCHAR c)
{
    return iswalnum(c) || c == '_';
}

static int check_keyword(parser_ctx_t *ctx, const WCHAR *word, const WCHAR **lval)
{
    const WCHAR *p1 = ctx->ptr;
    const WCHAR *p2 = word;
    WCHAR c;

    while(p1 < ctx->end && *p2) {
        c = towlower(*p1);
        if(c != *p2)
            return c - *p2;
        p1++;
        p2++;
    }

    if(*p2 || (p1 < ctx->end && is_identifier_char(*p1)))
        return 1;

    ctx->ptr = p1;
    *lval = word;
    return 0;
}

static int check_keywords(parser_ctx_t *ctx, const WCHAR **lval)
{
    int min = 0, max = ARRAY_SIZE(keywords)-1, r, i;

    while(min <= max) {
        i = (min+max)/2;

        r = check_keyword(ctx, keywords[i].word, lval);
        if(!r)
            return keywords[i].token;

        if(r > 0)
            min = i+1;
        else
            max = i-1;
    }

    return 0;
}

static int parse_identifier(parser_ctx_t *ctx, const WCHAR **ret)
{
    const WCHAR *ptr = ctx->ptr++;
    WCHAR *str;
    int len;

    while(ctx->ptr < ctx->end && is_identifier_char(*ctx->ptr))
        ctx->ptr++;
    len = ctx->ptr-ptr;

    str = parser_alloc(ctx, (len+1)*sizeof(WCHAR));
    if(!str)
        return 0;

    memcpy(str, ptr, (len+1)*sizeof(WCHAR));
    str[len] = 0;
    *ret = str;
    return tIdentifier;
}

static int parse_string_literal(parser_ctx_t *ctx, const WCHAR **ret)
{
    const WCHAR *ptr = ++ctx->ptr;
    WCHAR *rptr;
    int len = 0;

    while(ctx->ptr < ctx->end) {
        if(*ctx->ptr == '\n' || *ctx->ptr == '\r') {
            FIXME("newline inside string literal\n");
            return 0;
        }

       if(*ctx->ptr == '"') {
            if(ctx->ptr[1] != '"')
                break;
            len--;
            ctx->ptr++;
        }
        ctx->ptr++;
    }

    if(ctx->ptr == ctx->end) {
        FIXME("unterminated string literal\n");
        return 0;
    }

    len += ctx->ptr-ptr;

    *ret = rptr = parser_alloc(ctx, (len+1)*sizeof(WCHAR));
    if(!rptr)
        return 0;

    while(ptr < ctx->ptr) {
        if(*ptr == '"')
            ptr++;
        *rptr++ = *ptr++;
    }

    *rptr = 0;
    ctx->ptr++;
    return tString;
}

static int parse_numeric_literal(parser_ctx_t *ctx, void **ret)
{
    BOOL use_int = TRUE;
    LONGLONG d = 0, hlp;
    int exp = 0;
    double r;

    if(*ctx->ptr == '0' && !('0' <= ctx->ptr[1] && ctx->ptr[1] <= '9') && ctx->ptr[1] != '.')
        return *ctx->ptr++;

    while(ctx->ptr < ctx->end && is_digit(*ctx->ptr)) {
        hlp = d*10 + *(ctx->ptr++) - '0';
        if(d>MAXLONGLONG/10 || hlp<0) {
            exp++;
            break;
        }
        else
            d = hlp;
    }
    while(ctx->ptr < ctx->end && is_digit(*ctx->ptr)) {
        exp++;
        ctx->ptr++;
    }

    if(*ctx->ptr == '.') {
        use_int = FALSE;
        ctx->ptr++;

        while(ctx->ptr < ctx->end && is_digit(*ctx->ptr)) {
            hlp = d*10 + *(ctx->ptr++) - '0';
            if(d>MAXLONGLONG/10 || hlp<0)
                break;

            d = hlp;
            exp--;
        }
        while(ctx->ptr < ctx->end && is_digit(*ctx->ptr))
            ctx->ptr++;
    }

    if(*ctx->ptr == 'e' || *ctx->ptr == 'E') {
        int e = 0, sign = 1;

        ctx->ptr++;
        if(*ctx->ptr == '-') {
            ctx->ptr++;
            sign = -1;
        }else if(*ctx->ptr == '+') {
            ctx->ptr++;
        }

        if(!is_digit(*ctx->ptr)) {
            FIXME("Invalid numeric literal\n");
            return 0;
        }

        use_int = FALSE;

        do {
            e = e*10 + *(ctx->ptr++) - '0';
            if(sign == -1 && -e+exp < -(INT_MAX/100)) {
                /* The literal will be rounded to 0 anyway. */
                while(is_digit(*ctx->ptr))
                    ctx->ptr++;
                *(double*)ret = 0;
                return tDouble;
            }

            if(sign*e + exp > INT_MAX/100) {
                FIXME("Invalid numeric literal\n");
                return 0;
            }
        } while(is_digit(*ctx->ptr));

        exp += sign*e;
    }

    if(use_int && (LONG)d == d) {
        *(LONG*)ret = d;
        return tInt;
    }

    r = exp>=0 ? d*pow(10, exp) : d/pow(10, -exp);
    if(isinf(r)) {
        FIXME("Invalid numeric literal\n");
        return 0;
    }

    *(double*)ret = r;
    return tDouble;
}

static int hex_to_int(WCHAR c)
{
    if('0' <= c && c <= '9')
        return c-'0';
    if('a' <= c && c <= 'f')
        return c+10-'a';
    if('A' <= c && c <= 'F')
        return c+10-'A';
    return -1;
}

static int parse_hex_literal(parser_ctx_t *ctx, LONG *ret)
{
    const WCHAR *begin = ctx->ptr;
    unsigned l = 0, d;

    while((d = hex_to_int(*++ctx->ptr)) != -1)
        l = l*16 + d;

    if(begin + 9 /* max digits+1 */ < ctx->ptr || (*ctx->ptr != '&' && is_identifier_char(*ctx->ptr))) {
        FIXME("invalid literal\n");
        return 0;
    }

    if(*ctx->ptr == '&') {
        ctx->ptr++;
        *ret = l;
    }else {
        *ret = l == (UINT16)l ? (INT16)l : l;
    }
    return tInt;
}

static void skip_spaces(parser_ctx_t *ctx)
{
    while(*ctx->ptr == ' ' || *ctx->ptr == '\t')
        ctx->ptr++;
}

static int comment_line(parser_ctx_t *ctx)
{
    static const WCHAR newlineW[] = {'\n','\r',0};
    ctx->ptr = wcspbrk(ctx->ptr, newlineW);
    if(ctx->ptr)
        ctx->ptr++;
    else
        ctx->ptr = ctx->end;
    return tNL;
}

static int parse_next_token(void *lval, unsigned *loc, parser_ctx_t *ctx)
{
    WCHAR c;

    skip_spaces(ctx);
    *loc = ctx->ptr - ctx->code;
    if(ctx->ptr == ctx->end)
        return ctx->last_token == tNL ? 0 : tNL;

    c = *ctx->ptr;

    if('0' <= c && c <= '9')
        return parse_numeric_literal(ctx, lval);

    if(iswalpha(c)) {
        int ret = 0;
        if(ctx->last_token != '.' && ctx->last_token != tDOT)
            ret = check_keywords(ctx, lval);
        if(!ret)
            return parse_identifier(ctx, lval);
        if(ret != tREM)
            return ret;
        c = '\'';
    }

    switch(c) {
    case '\n':
    case '\r':
        ctx->ptr++;
        return tNL;
    case '\'':
        return comment_line(ctx);
    case ':':
    case ')':
    case ',':
    case '=':
    case '+':
    case '*':
    case '/':
    case '^':
    case '\\':
    case '_':
        return *ctx->ptr++;
    case '.':
        /*
         * We need to distinguish between '.' used as part of a member expression and
         * a beginning of a dot expression (a member expression accessing with statement
         * expression).
         */
        c = ctx->ptr > ctx->code ? ctx->ptr[-1] : '\n';
        ctx->ptr++;
        return is_identifier_char(c) || c == ')' ? '.' : tDOT;
    case '-':
        if(ctx->is_html && ctx->ptr[1] == '-' && ctx->ptr[2] == '>')
            return comment_line(ctx);
        ctx->ptr++;
        return '-';
    case '(':
        /* NOTE:
         * We resolve empty brackets in lexer instead of parser to avoid complex conflicts
         * in call statement special case |f()| without 'call' keyword
         */
        ctx->ptr++;
        skip_spaces(ctx);
        if(*ctx->ptr == ')') {
            ctx->ptr++;
            return tEMPTYBRACKETS;
        }
        /*
         * Parser can't predict if bracket is part of argument expression or an argument
         * in call expression. We predict it here instead.
         */
        if(ctx->last_token == tIdentifier || ctx->last_token == ')')
            return '(';
        return tEXPRLBRACKET;
    case '"':
        return parse_string_literal(ctx, lval);
    case '&':
        if(*++ctx->ptr == 'h' || *ctx->ptr == 'H')
            return parse_hex_literal(ctx, lval);
        return '&';
    case '<':
        switch(*++ctx->ptr) {
        case '>':
            ctx->ptr++;
            return tNEQ;
        case '=':
            ctx->ptr++;
            return tLTEQ;
        case '!':
            if(ctx->is_html && ctx->ptr[1] == '-' && ctx->ptr[2] == '-')
                return comment_line(ctx);
        }
        return '<';
    case '>':
        if(*++ctx->ptr == '=') {
            ctx->ptr++;
            return tGTEQ;
        }
        return '>';
    default:
        FIXME("Unhandled char %c in %s\n", *ctx->ptr, debugstr_w(ctx->ptr));
    }

    return 0;
}

int parser_lex(void *lval, unsigned *loc, parser_ctx_t *ctx)
{
    int ret;

    if (ctx->last_token == tEXPRESSION)
    {
        ctx->last_token = tNL;
        return tEXPRESSION;
    }

    while(1) {
        ret = parse_next_token(lval, loc, ctx);
        if(ret == '_') {
            skip_spaces(ctx);
            if(*ctx->ptr != '\n' && *ctx->ptr != '\r') {
                FIXME("'_' not followed by newline\n");
                return 0;
            }
            if(*ctx->ptr == '\r')
                ctx->ptr++;
            if(*ctx->ptr == '\n')
                ctx->ptr++;
            continue;
        }
        if(ret != tNL || ctx->last_token != tNL)
            break;

        ctx->last_nl = ctx->ptr-ctx->code;
    }

    return (ctx->last_token = ret);
}
