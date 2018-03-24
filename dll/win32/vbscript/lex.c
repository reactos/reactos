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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <limits.h>

#include "vbscript.h"
#include "parse.h"
#include "parser.tab.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);

static const WCHAR andW[] = {'a','n','d',0};
static const WCHAR byrefW[] = {'b','y','r','e','f',0};
static const WCHAR byvalW[] = {'b','y','v','a','l',0};
static const WCHAR callW[] = {'c','a','l','l',0};
static const WCHAR caseW[] = {'c','a','s','e',0};
static const WCHAR classW[] = {'c','l','a','s','s',0};
static const WCHAR constW[] = {'c','o','n','s','t',0};
static const WCHAR defaultW[] = {'d','e','f','a','u','l','t',0};
static const WCHAR dimW[] = {'d','i','m',0};
static const WCHAR doW[] = {'d','o',0};
static const WCHAR eachW[] = {'e','a','c','h',0};
static const WCHAR elseW[] = {'e','l','s','e',0};
static const WCHAR elseifW[] = {'e','l','s','e','i','f',0};
static const WCHAR emptyW[] = {'e','m','p','t','y',0};
static const WCHAR endW[] = {'e','n','d',0};
static const WCHAR eqvW[] = {'e','q','v',0};
static const WCHAR errorW[] = {'e','r','r','o','r',0};
static const WCHAR exitW[] = {'e','x','i','t',0};
static const WCHAR explicitW[] = {'e','x','p','l','i','c','i','t',0};
static const WCHAR falseW[] = {'f','a','l','s','e',0};
static const WCHAR forW[] = {'f','o','r',0};
static const WCHAR functionW[] = {'f','u','n','c','t','i','o','n',0};
static const WCHAR getW[] = {'g','e','t',0};
static const WCHAR gotoW[] = {'g','o','t','o',0};
static const WCHAR ifW[] = {'i','f',0};
static const WCHAR impW[] = {'i','m','p',0};
static const WCHAR inW[] = {'i','n',0};
static const WCHAR isW[] = {'i','s',0};
static const WCHAR letW[] = {'l','e','t',0};
static const WCHAR loopW[] = {'l','o','o','p',0};
static const WCHAR meW[] = {'m','e',0};
static const WCHAR modW[] = {'m','o','d',0};
static const WCHAR newW[] = {'n','e','w',0};
static const WCHAR nextW[] = {'n','e','x','t',0};
static const WCHAR notW[] = {'n','o','t',0};
static const WCHAR nothingW[] = {'n','o','t','h','i','n','g',0};
static const WCHAR nullW[] = {'n','u','l','l',0};
static const WCHAR onW[] = {'o','n',0};
static const WCHAR optionW[] = {'o','p','t','i','o','n',0};
static const WCHAR orW[] = {'o','r',0};
static const WCHAR privateW[] = {'p','r','i','v','a','t','e',0};
static const WCHAR propertyW[] = {'p','r','o','p','e','r','t','y',0};
static const WCHAR publicW[] = {'p','u','b','l','i','c',0};
static const WCHAR remW[] = {'r','e','m',0};
static const WCHAR resumeW[] = {'r','e','s','u','m','e',0};
static const WCHAR selectW[] = {'s','e','l','e','c','t',0};
static const WCHAR setW[] = {'s','e','t',0};
static const WCHAR stepW[] = {'s','t','e','p',0};
static const WCHAR stopW[] = {'s','t','o','p',0};
static const WCHAR subW[] = {'s','u','b',0};
static const WCHAR thenW[] = {'t','h','e','n',0};
static const WCHAR toW[] = {'t','o',0};
static const WCHAR trueW[] = {'t','r','u','e',0};
static const WCHAR untilW[] = {'u','n','t','i','l',0};
static const WCHAR wendW[] = {'w','e','n','d',0};
static const WCHAR whileW[] = {'w','h','i','l','e',0};
static const WCHAR xorW[] = {'x','o','r',0};

static const struct {
    const WCHAR *word;
    int token;
} keywords[] = {
    {andW,       tAND},
    {byrefW,     tBYREF},
    {byvalW,     tBYVAL},
    {callW,      tCALL},
    {caseW,      tCASE},
    {classW,     tCLASS},
    {constW,     tCONST},
    {defaultW,   tDEFAULT},
    {dimW,       tDIM},
    {doW,        tDO},
    {eachW,      tEACH},
    {elseW,      tELSE},
    {elseifW,    tELSEIF},
    {emptyW,     tEMPTY},
    {endW,       tEND},
    {eqvW,       tEQV},
    {errorW,     tERROR},
    {exitW,      tEXIT},
    {explicitW,  tEXPLICIT},
    {falseW,     tFALSE},
    {forW,       tFOR},
    {functionW,  tFUNCTION},
    {getW,       tGET},
    {gotoW,      tGOTO},
    {ifW,        tIF},
    {impW,       tIMP},
    {inW,        tIN},
    {isW,        tIS},
    {letW,       tLET},
    {loopW,      tLOOP},
    {meW,        tME},
    {modW,       tMOD},
    {newW,       tNEW},
    {nextW,      tNEXT},
    {notW,       tNOT},
    {nothingW,   tNOTHING},
    {nullW,      tNULL},
    {onW,        tON},
    {optionW,    tOPTION},
    {orW,        tOR},
    {privateW,   tPRIVATE},
    {propertyW,  tPROPERTY},
    {publicW,    tPUBLIC},
    {remW,       tREM},
    {resumeW,    tRESUME},
    {selectW,    tSELECT},
    {setW,       tSET},
    {stepW,      tSTEP},
    {stopW,      tSTOP},
    {subW,       tSUB},
    {thenW,      tTHEN},
    {toW,        tTO},
    {trueW,      tTRUE},
    {untilW,     tUNTIL},
    {wendW,      tWEND},
    {whileW,     tWHILE},
    {xorW,       tXOR}
};

static inline BOOL is_identifier_char(WCHAR c)
{
    return isalnumW(c) || c == '_';
}

static int check_keyword(parser_ctx_t *ctx, const WCHAR *word)
{
    const WCHAR *p1 = ctx->ptr;
    const WCHAR *p2 = word;
    WCHAR c;

    while(p1 < ctx->end && *p2) {
        c = tolowerW(*p1);
        if(c != *p2)
            return c - *p2;
        p1++;
        p2++;
    }

    if(*p2 || (p1 < ctx->end && is_identifier_char(*p1)))
        return 1;

    ctx->ptr = p1;
    return 0;
}

static int check_keywords(parser_ctx_t *ctx)
{
    int min = 0, max = sizeof(keywords)/sizeof(keywords[0])-1, r, i;

    while(min <= max) {
        i = (min+max)/2;

        r = check_keyword(ctx, keywords[i].word);
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
        if(*ctx->ptr == '\n') {
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

    while(ctx->ptr < ctx->end && isdigitW(*ctx->ptr)) {
        hlp = d*10 + *(ctx->ptr++) - '0';
        if(d>MAXLONGLONG/10 || hlp<0) {
            exp++;
            break;
        }
        else
            d = hlp;
    }
    while(ctx->ptr < ctx->end && isdigitW(*ctx->ptr)) {
        exp++;
        ctx->ptr++;
    }

    if(*ctx->ptr == '.') {
        use_int = FALSE;
        ctx->ptr++;

        while(ctx->ptr < ctx->end && isdigitW(*ctx->ptr)) {
            hlp = d*10 + *(ctx->ptr++) - '0';
            if(d>MAXLONGLONG/10 || hlp<0)
                break;

            d = hlp;
            exp--;
        }
        while(ctx->ptr < ctx->end && isdigitW(*ctx->ptr))
            ctx->ptr++;
    }

    if(*ctx->ptr == 'e' || *ctx->ptr == 'E') {
        int e = 0, sign = 1;

        if(*++ctx->ptr == '-') {
            ctx->ptr++;
            sign = -1;
        }

        if(!isdigitW(*ctx->ptr)) {
            FIXME("Invalid numeric literal\n");
            return 0;
        }

        use_int = FALSE;

        do {
            e = e*10 + *(ctx->ptr++) - '0';
            if(sign == -1 && -e+exp < -(INT_MAX/100)) {
                /* The literal will be rounded to 0 anyway. */
                while(isdigitW(*ctx->ptr))
                    ctx->ptr++;
                *(double*)ret = 0;
                return tDouble;
            }

            if(sign*e + exp > INT_MAX/100) {
                FIXME("Invalid numeric literal\n");
                return 0;
            }
        } while(isdigitW(*ctx->ptr));

        exp += sign*e;
    }

    if(use_int && (LONG)d == d) {
        LONG l = d;
        *(LONG*)ret = l;
        return (short)l == l ? tShort : tLong;
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
    LONG l = 0, d;

    while((d = hex_to_int(*++ctx->ptr)) != -1)
        l = l*16 + d;

    if(begin + 9 /* max digits+1 */ < ctx->ptr || (*ctx->ptr != '&' && is_identifier_char(*ctx->ptr))) {
        FIXME("invalid literal\n");
        return 0;
    }

    if(*ctx->ptr == '&')
        ctx->ptr++;

    *ret = l;
    return (short)l == l ? tShort : tLong;
}

static void skip_spaces(parser_ctx_t *ctx)
{
    while(*ctx->ptr == ' ' || *ctx->ptr == '\t' || *ctx->ptr == '\r')
        ctx->ptr++;
}

static int comment_line(parser_ctx_t *ctx)
{
    ctx->ptr = strchrW(ctx->ptr, '\n');
    if(ctx->ptr)
        ctx->ptr++;
    else
        ctx->ptr = ctx->end;
    return tNL;
}

static int parse_next_token(void *lval, parser_ctx_t *ctx)
{
    WCHAR c;

    skip_spaces(ctx);
    if(ctx->ptr == ctx->end)
        return ctx->last_token == tNL ? tEOF : tNL;

    c = *ctx->ptr;

    if('0' <= c && c <= '9')
        return parse_numeric_literal(ctx, lval);

    if(isalphaW(c)) {
        int ret = check_keywords(ctx);
        if(!ret)
            return parse_identifier(ctx, lval);
        if(ret != tREM)
            return ret;
        c = '\'';
    }

    switch(c) {
    case '\n':
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
    case '.':
    case '_':
        return *ctx->ptr++;
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
        return '(';
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

int parser_lex(void *lval, parser_ctx_t *ctx)
{
    int ret;

    while(1) {
        ret = parse_next_token(lval, ctx);
        if(ret == '_') {
            skip_spaces(ctx);
            if(*ctx->ptr != '\n') {
                FIXME("'_' not followed by newline\n");
                return 0;
            }
            ctx->ptr++;
            continue;
        }
        if(ret != tNL || ctx->last_token != tNL)
            break;

        ctx->last_nl = ctx->ptr-ctx->code;
    }

    return (ctx->last_token = ret);
}
