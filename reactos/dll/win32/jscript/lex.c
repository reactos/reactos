/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

#include <math.h>

#include "jscript.h"
#include "activscp.h"
#include "objsafe.h"
#include "engine.h"

#include "parser.tab.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

static const WCHAR breakW[] = {'b','r','e','a','k',0};
static const WCHAR caseW[] = {'c','a','s','e',0};
static const WCHAR catchW[] = {'c','a','t','c','h',0};
static const WCHAR continueW[] = {'c','o','n','t','i','n','u','e',0};
static const WCHAR defaultW[] = {'d','e','f','a','u','l','t',0};
static const WCHAR deleteW[] = {'d','e','l','e','t','e',0};
static const WCHAR doW[] = {'d','o',0};
static const WCHAR elseW[] = {'e','l','s','e',0};
static const WCHAR falseW[] = {'f','a','l','s','e',0};
static const WCHAR finallyW[] = {'f','i','n','a','l','l','y',0};
static const WCHAR forW[] = {'f','o','r',0};
static const WCHAR functionW[] = {'f','u','n','c','t','i','o','n',0};
static const WCHAR ifW[] = {'i','f',0};
static const WCHAR inW[] = {'i','n',0};
static const WCHAR instanceofW[] = {'i','n','s','t','a','n','c','e','o','f',0};
static const WCHAR newW[] = {'n','e','w',0};
static const WCHAR nullW[] = {'n','u','l','l',0};
static const WCHAR returnW[] = {'r','e','t','u','r','n',0};
static const WCHAR switchW[] = {'s','w','i','t','c','h',0};
static const WCHAR thisW[] = {'t','h','i','s',0};
static const WCHAR throwW[] = {'t','h','r','o','w',0};
static const WCHAR trueW[] = {'t','r','u','e',0};
static const WCHAR tryW[] = {'t','r','y',0};
static const WCHAR typeofW[] = {'t','y','p','e','o','f',0};
static const WCHAR undefinedW[] = {'u','n','d','e','f','i','n','e','d',0};
static const WCHAR varW[] = {'v','a','r',0};
static const WCHAR voidW[] = {'v','o','i','d',0};
static const WCHAR whileW[] = {'w','h','i','l','e',0};
static const WCHAR withW[] = {'w','i','t','h',0};

static const struct {
    const WCHAR *word;
    int token;
} keywords[] = {
    {breakW,       kBREAK},
    {caseW,        kCASE},
    {catchW,       kCATCH},
    {continueW,    kCONTINUE},
    {defaultW,     kDEFAULT},
    {deleteW,      kDELETE},
    {doW,          kDO},
    {elseW,        kELSE},
    {falseW,       kFALSE},
    {finallyW,     kFINALLY},
    {forW,         kFOR},
    {functionW,    kFUNCTION},
    {ifW,          kIF},
    {inW,          kIN},
    {instanceofW,  kINSTANCEOF},
    {newW,         kNEW},
    {nullW,        kNULL},
    {returnW,      kRETURN},
    {switchW,      kSWITCH},
    {thisW,        kTHIS},
    {throwW,       kTHROW},
    {trueW,        kTRUE},
    {tryW,         kTRY},
    {typeofW,      kTYPEOF},
    {undefinedW,   kUNDEFINED},
    {varW,         kVAR},
    {voidW,        kVOID},
    {whileW,       kWHILE},
    {withW,        kWITH}
};

static int lex_error(parser_ctx_t *ctx, HRESULT hres)
{
    ctx->hres = hres;
    return -1;
}

static int check_keyword(parser_ctx_t *ctx, const WCHAR *word, const WCHAR **lval)
{
    const WCHAR *p1 = ctx->ptr;
    const WCHAR *p2 = word;

    while(p1 < ctx->end && *p2) {
        if(*p1 != *p2)
            return *p1 - *p2;
        p1++;
        p2++;
    }

    if(*p2 || (p1 < ctx->end && isalnumW(*p1)))
        return 1;

    *lval = ctx->ptr;
    ctx->ptr = p1;
    return 0;
}

/* ECMA-262 3rd Edition    7.3 */
static BOOL is_endline(WCHAR c)
{
    return c == '\n' || c == '\r' || c == 0x2028 || c == 0x2029;
}

static BOOL is_identifier_char(WCHAR c)
{
    return isalnumW(c) || c == '$' || c == '_' || c == '\\';
}

static int hex_to_int(WCHAR c)
{
    if('0' <= c && c <= '9')
        return c-'0';

    if('a' <= c && c <= 'f')
        return c-'a'+10;

    if('A' <= c && c <= 'F')
        return c-'A'+10;

    return -1;
}

static int check_keywords(parser_ctx_t *ctx, const WCHAR **lval)
{
    int min = 0, max = sizeof(keywords)/sizeof(keywords[0])-1, r, i;

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

static void skip_spaces(parser_ctx_t *ctx)
{
    while(ctx->ptr < ctx->end && isspaceW(*ctx->ptr)) {
        if(is_endline(*ctx->ptr++))
            ctx->nl = TRUE;
    }
}

static BOOL skip_html_comment(parser_ctx_t *ctx)
{
    const WCHAR html_commentW[] = {'<','!','-','-',0};

    if(!ctx->is_html || ctx->ptr+3 >= ctx->end ||
        memcmp(ctx->ptr, html_commentW, sizeof(WCHAR)*4))
        return FALSE;

    ctx->nl = TRUE;
    while(ctx->ptr < ctx->end && !is_endline(*ctx->ptr++));

    return TRUE;
}

static BOOL skip_comment(parser_ctx_t *ctx)
{
    if(ctx->ptr+1 >= ctx->end || *ctx->ptr != '/')
        return FALSE;

    switch(ctx->ptr[1]) {
    case '*':
        ctx->ptr += 2;
        while(ctx->ptr+1 < ctx->end && (ctx->ptr[0] != '*' || ctx->ptr[1] != '/'))
            ctx->ptr++;

        if(ctx->ptr[0] == '*' && ctx->ptr[1] == '/') {
            ctx->ptr += 2;
        }else {
            WARN("unexpected end of file (missing end of comment)\n");
            ctx->ptr = ctx->end;
        }
        break;
    case '/':
        ctx->ptr += 2;
        while(ctx->ptr < ctx->end && !is_endline(*ctx->ptr))
            ctx->ptr++;
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

static BOOL unescape(WCHAR *str)
{
    WCHAR *pd, *p, c;
    int i;

    pd = p = str;
    while(*p) {
        if(*p != '\\') {
            *pd++ = *p++;
            continue;
        }

        p++;
        c = 0;

        switch(*p) {
        case '\'':
        case '\"':
        case '\\':
            c = *p;
            break;
        case 'b':
            c = '\b';
            break;
        case 't':
            c = '\t';
            break;
        case 'n':
            c = '\n';
            break;
        case 'v':
            c = '\v';
            break;
        case 'f':
            c = '\f';
            break;
        case 'r':
            c = '\r';
            break;
        case 'x':
            i = hex_to_int(*++p);
            if(i == -1)
                return FALSE;
            c = i << 4;

            i = hex_to_int(*++p);
            if(i == -1)
                return FALSE;
            c += i;
            break;
        case 'u':
            i = hex_to_int(*++p);
            if(i == -1)
                return FALSE;
            c = i << 12;

            i = hex_to_int(*++p);
            if(i == -1)
                return FALSE;
            c += i << 8;

            i = hex_to_int(*++p);
            if(i == -1)
                return FALSE;
            c += 1 << 4;

            i = hex_to_int(*++p);
            if(i == -1)
                return FALSE;
            c += i;
            break;
        default:
            if(isdigitW(*p)) {
                c = *p++ - '0';
                while(isdigitW(*p))
                    c = c*10 + (*p++ - '0');
                *pd++ = c;
                continue;
            }

            c = *p;
        }

        *pd++ = c;
        p++;
    }

    *pd = 0;
    return TRUE;
}

static int parse_identifier(parser_ctx_t *ctx, const WCHAR **ret)
{
    const WCHAR *ptr = ctx->ptr++;
    WCHAR *wstr;
    int len;

    while(ctx->ptr < ctx->end && is_identifier_char(*ctx->ptr))
        ctx->ptr++;

    len = ctx->ptr-ptr;

    *ret = wstr = parser_alloc(ctx, (len+1)*sizeof(WCHAR));
    memcpy(wstr, ptr, (len+1)*sizeof(WCHAR));
    wstr[len] = 0;

    /* FIXME: unescape */
    return tIdentifier;
}

static int parse_string_literal(parser_ctx_t *ctx, const WCHAR **ret, WCHAR endch)
{
    const WCHAR *ptr = ++ctx->ptr;
    WCHAR *wstr;
    int len;

    while(ctx->ptr < ctx->end && *ctx->ptr != endch) {
        if(*ctx->ptr++ == '\\')
            ctx->ptr++;
    }

    if(ctx->ptr == ctx->end) {
        WARN("unexpected end of file\n");
        return lex_error(ctx, E_FAIL);
    }

    len = ctx->ptr-ptr;

    *ret = wstr = parser_alloc(ctx, (len+1)*sizeof(WCHAR));
    memcpy(wstr, ptr, (len+1)*sizeof(WCHAR));
    wstr[len] = 0;

    ctx->ptr++;

    if(!unescape(wstr)) {
        WARN("unescape failed\n");
        return lex_error(ctx, E_FAIL);
    }

    return tStringLiteral;
}

static literal_t *alloc_int_literal(parser_ctx_t *ctx, LONG l)
{
    literal_t *ret = parser_alloc(ctx, sizeof(literal_t));

    ret->vt = VT_I4;
    ret->u.lval = l;

    return ret;
}

static int parse_double_literal(parser_ctx_t *ctx, LONG int_part, literal_t **literal)
{
    double d, tmp = 1.0;

    if(ctx->ptr == ctx->end || !isdigitW(*ctx->ptr)) {
        ERR("No digit after point\n");
        return 0;
    }

    d = int_part;
    while(ctx->ptr < ctx->end && isdigitW(*ctx->ptr))
        d += (tmp /= 10.0)*(*ctx->ptr++ - '0');

    if(ctx->ptr < ctx->end && (*ctx->ptr == 'e' || *ctx->ptr == 'E')) {
        int sign = 1, e = 0;

        ctx->ptr++;
        if(ctx->ptr < ctx->end) {
            if(*ctx->ptr == '+') {
                ctx->ptr++;
            }else if(*ctx->ptr == '-') {
                sign = -1;
                ctx->ptr++;
            }else if(!isdigitW(*ctx->ptr)) {
                WARN("Expected exponent part\n");
                return lex_error(ctx, E_FAIL);
            }
        }

        if(ctx->ptr == ctx->end) {
            WARN("unexpected end of file\n");
            return lex_error(ctx, E_FAIL);
        }

        while(ctx->ptr < ctx->end && isdigitW(*ctx->ptr))
            e = e*10 + *ctx->ptr++ - '0';
        e *= sign;

        d *= pow(10, e);
    }

    *literal = parser_alloc(ctx, sizeof(literal_t));
    (*literal)->vt = VT_R8;
    (*literal)->u.dval = d;

    return tNumericLiteral;
}

static int parse_numeric_literal(parser_ctx_t *ctx, literal_t **literal)
{
    LONG l, d;

    l = *ctx->ptr++ - '0';
    if(ctx->ptr == ctx->end) {
        *literal = alloc_int_literal(ctx, l);
        return tNumericLiteral;
    }

    if(!l) {
        if(*ctx->ptr == 'x' || *ctx->ptr == 'X') {
            if(++ctx->ptr == ctx->end) {
                ERR("unexpexted end of file\n");
                return 0;
            }

            while(ctx->ptr < ctx->end && (d = hex_to_int(*ctx->ptr)) != -1) {
                l = l*16 + d;
                ctx->ptr++;
            }

            if(ctx->ptr < ctx->end && is_identifier_char(*ctx->ptr)) {
                WARN("unexpected identifier char\n");
                return lex_error(ctx, E_FAIL);
            }

            *literal = alloc_int_literal(ctx, l);
            return tNumericLiteral;
        }

        if(isdigitW(*ctx->ptr) || is_identifier_char(*ctx->ptr)) {
            WARN("wrong char after zero\n");
            return lex_error(ctx, E_FAIL);
        }

        *literal = alloc_int_literal(ctx, 0);
    }

    while(ctx->ptr < ctx->end && isdigitW(*ctx->ptr))
        l = l*10 + *(ctx->ptr++)-'0';

    if(ctx->ptr < ctx->end) {
        if(*ctx->ptr == '.') {
            ctx->ptr++;
            return parse_double_literal(ctx, l, literal);
        }

        if(is_identifier_char(*ctx->ptr)) {
            WARN("unexpected identifier char\n");
            return lex_error(ctx, E_FAIL);
        }
    }

    *literal = alloc_int_literal(ctx, l);
    return tNumericLiteral;
}

int parser_lex(void *lval, parser_ctx_t *ctx)
{
    int ret;

    ctx->nl = ctx->ptr == ctx->begin;

    do {
        skip_spaces(ctx);
        if(ctx->ptr == ctx->end)
            return 0;
    }while(skip_comment(ctx) || skip_html_comment(ctx));

    if(isalphaW(*ctx->ptr)) {
        ret = check_keywords(ctx, lval);
        if(ret)
            return ret;

        return parse_identifier(ctx, lval);
    }

    if(isdigitW(*ctx->ptr))
        return parse_numeric_literal(ctx, lval);

    switch(*ctx->ptr) {
    case '{':
    case '(':
    case ')':
    case '[':
    case ']':
    case ';':
    case ',':
    case '~':
    case '?':
    case ':':
        return *ctx->ptr++;

    case '}':
        *(const WCHAR**)lval = ctx->ptr++;
        return '}';

    case '.':
        if(++ctx->ptr < ctx->end && isdigitW(*ctx->ptr))
            return parse_double_literal(ctx, 0, lval);
        return '.';

    case '<':
        if(++ctx->ptr == ctx->end) {
            *(int*)lval = EXPR_LESS;
            return tRelOper;
        }

        switch(*ctx->ptr) {
        case '=':  /* <= */
            ctx->ptr++;
            *(int*)lval = EXPR_LESSEQ;
            return tRelOper;
        case '<':  /* << */
            if(++ctx->ptr < ctx->end && *ctx->ptr == '=') { /* <<= */
                ctx->ptr++;
                *(int*)lval = EXPR_ASSIGNLSHIFT;
                return tAssignOper;
            }
            *(int*)lval = EXPR_LSHIFT;
            return tShiftOper;
        default: /* < */
            *(int*)lval = EXPR_LESS;
            return tRelOper;
        }

    case '>':
        if(++ctx->ptr == ctx->end) { /* > */
            *(int*)lval = EXPR_GREATER;
            return tRelOper;
        }

        switch(*ctx->ptr) {
        case '=':  /* >= */
            ctx->ptr++;
            *(int*)lval = EXPR_GREATEREQ;
            return tRelOper;
        case '>':  /* >> */
            if(++ctx->ptr < ctx->end) {
                if(*ctx->ptr == '=') {  /* >>= */
                    ctx->ptr++;
                    *(int*)lval = EXPR_ASSIGNRSHIFT;
                    return tAssignOper;
                }
                if(*ctx->ptr == '>') {  /* >>> */
                    if(++ctx->ptr < ctx->end && *ctx->ptr == '=') {  /* >>>= */
                        ctx->ptr++;
                        *(int*)lval = EXPR_ASSIGNRRSHIFT;
                        return tAssignOper;
                    }
                    *(int*)lval = EXPR_RRSHIFT;
                    return tRelOper;
                }
            }
            *(int*)lval = EXPR_RSHIFT;
            return tShiftOper;
        default:
            *(int*)lval = EXPR_GREATER;
            return tRelOper;
        }

    case '+':
        ctx->ptr++;
        if(ctx->ptr < ctx->end) {
            switch(*ctx->ptr) {
            case '+':  /* ++ */
                ctx->ptr++;
                return tINC;
            case '=':  /* += */
                ctx->ptr++;
                *(int*)lval = EXPR_ASSIGNADD;
                return tAssignOper;
            }
        }
        return '+';

    case '-':
        ctx->ptr++;
        if(ctx->ptr < ctx->end) {
            switch(*ctx->ptr) {
            case '-':  /* -- or --> */
                ctx->ptr++;
                if(ctx->is_html && ctx->nl && ctx->ptr < ctx->end && *ctx->ptr == '>') {
                    ctx->ptr++;
                    return tHTMLCOMMENT;
                }
                return tDEC;
            case '=':  /* -= */
                ctx->ptr++;
                *(int*)lval = EXPR_ASSIGNSUB;
                return tAssignOper;
            }
        }
        return '-';

    case '*':
        if(++ctx->ptr < ctx->end && *ctx->ptr == '=') { /* *= */
            ctx->ptr++;
            *(int*)lval = EXPR_ASSIGNMUL;
            return tAssignOper;
        }
        return '*';

    case '%':
        if(++ctx->ptr < ctx->end && *ctx->ptr == '=') { /* %= */
            ctx->ptr++;
            *(int*)lval = EXPR_ASSIGNMOD;
            return tAssignOper;
        }
        return '%';

    case '&':
        if(++ctx->ptr < ctx->end) {
            switch(*ctx->ptr) {
            case '=':  /* &= */
                ctx->ptr++;
                *(int*)lval = EXPR_ASSIGNAND;
                return tAssignOper;
            case '&':  /* && */
                ctx->ptr++;
                return tANDAND;
            }
        }
        return '&';

    case '|':
        if(++ctx->ptr < ctx->end) {
            switch(*ctx->ptr) {
            case '=':  /* |= */
                ctx->ptr++;
                *(int*)lval = EXPR_ASSIGNOR;
                return tAssignOper;
            case '|':  /* || */
                ctx->ptr++;
                return tOROR;
            }
        }
        return '|';

    case '^':
        if(++ctx->ptr < ctx->end && *ctx->ptr == '=') {  /* ^= */
            ctx->ptr++;
            *(int*)lval = EXPR_ASSIGNXOR;
            return tAssignOper;
        }
        return '^';

    case '!':
        if(++ctx->ptr < ctx->end && *ctx->ptr == '=') {  /* != */
            if(++ctx->ptr < ctx->end && *ctx->ptr == '=') {  /* !== */
                ctx->ptr++;
                *(int*)lval = EXPR_NOTEQEQ;
                return tEqOper;
            }
            *(int*)lval = EXPR_NOTEQ;
            return tEqOper;
        }
        return '!';

    case '=':
        if(++ctx->ptr < ctx->end && *ctx->ptr == '=') {  /* == */
            if(++ctx->ptr < ctx->end && *ctx->ptr == '=') {  /* === */
                ctx->ptr++;
                *(int*)lval = EXPR_EQEQ;
                return tEqOper;
            }
            *(int*)lval = EXPR_EQ;
            return tEqOper;
        }
        return '=';

    case '/':
        if(++ctx->ptr < ctx->end) {
            if(*ctx->ptr == '=') {  /* /= */
                ctx->ptr++;
                *(int*)lval = EXPR_ASSIGNDIV;
                return tAssignOper;
            }
        }
        return '/';

    case '\"':
    case '\'':
        return parse_string_literal(ctx, lval, *ctx->ptr);

    case '_':
    case '$':
        return parse_identifier(ctx, lval);
    }

    WARN("unexpected char '%c' %d\n", *ctx->ptr, *ctx->ptr);
    return 0;
}

static void add_object_literal(parser_ctx_t *ctx, DispatchEx *obj)
{
    obj_literal_t *literal = parser_alloc(ctx, sizeof(obj_literal_t));

    literal->obj = obj;
    literal->next = ctx->obj_literals;
    ctx->obj_literals = literal;
}

literal_t *parse_regexp(parser_ctx_t *ctx)
{
    const WCHAR *re, *flags;
    DispatchEx *regexp;
    literal_t *ret;
    DWORD re_len;
    HRESULT hres;

    TRACE("\n");

    re = ctx->ptr;
    while(ctx->ptr < ctx->end && *ctx->ptr != '/') {
        if(*ctx->ptr++ == '\\' && ctx->ptr < ctx->end)
            ctx->ptr++;
    }

    if(ctx->ptr == ctx->end) {
        WARN("unexpected end of file\n");
        return NULL;
    }

    re_len = ctx->ptr-re;

    flags = ++ctx->ptr;
    while(ctx->ptr < ctx->end && isalnumW(*ctx->ptr))
        ctx->ptr++;

    hres = create_regexp_str(ctx->script, re, re_len, flags, ctx->ptr-flags, &regexp);
    if(FAILED(hres))
        return NULL;

    add_object_literal(ctx, regexp);

    ret = parser_alloc(ctx, sizeof(literal_t));
    ret->vt = VT_DISPATCH;
    ret->u.disp = (IDispatch*)_IDispatchEx_(regexp);
    return ret;
}
