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


#include <limits.h>
#include <math.h>

#include "jscript.h"
#include "activscp.h"
#include "objsafe.h"
#include "engine.h"
#include "parser.h"

#include "parser.tab.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

static const struct {
    const WCHAR *word;
    int token;
    BOOL no_nl;
    unsigned min_version;
} keywords[] = {
    {L"break",       kBREAK,       TRUE},
    {L"case",        kCASE},
    {L"catch",       kCATCH},
    {L"const",       kCONST},
    {L"continue",    kCONTINUE,    TRUE},
    {L"default",     kDEFAULT},
    {L"delete",      kDELETE},
    {L"do",          kDO},
    {L"else",        kELSE},
    {L"false",       kFALSE},
    {L"finally",     kFINALLY},
    {L"for",         kFOR},
    {L"function",    kFUNCTION},
    {L"get",         kGET,         FALSE, SCRIPTLANGUAGEVERSION_ES5},
    {L"if",          kIF},
    {L"in",          kIN},
    {L"instanceof",  kINSTANCEOF},
    {L"let",         kLET,         FALSE, SCRIPTLANGUAGEVERSION_ES5},
    {L"new",         kNEW},
    {L"null",        kNULL},
    {L"return",      kRETURN,      TRUE},
    {L"set",         kSET,         FALSE, SCRIPTLANGUAGEVERSION_ES5},
    {L"switch",      kSWITCH},
    {L"this",        kTHIS},
    {L"throw",       kTHROW},
    {L"true",        kTRUE},
    {L"try",         kTRY},
    {L"typeof",      kTYPEOF},
    {L"var",         kVAR},
    {L"void",        kVOID},
    {L"while",       kWHILE},
    {L"with",        kWITH}
};

static int lex_error(parser_ctx_t *ctx, HRESULT hres)
{
    ctx->hres = hres;
    ctx->lexer_error = TRUE;
    return -1;
}

/* ECMA-262 3rd Edition    7.6 */
BOOL is_identifier_char(WCHAR c)
{
    return iswalnum(c) || c == '$' || c == '_' || c == '\\';
}

static BOOL is_identifier_first_char(WCHAR c)
{
    return iswalpha(c) || c == '$' || c == '_' || c == '\\';
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

    if(*p2 || (p1 < ctx->end && is_identifier_char(*p1)))
        return 1;

    if(lval)
        *lval = word;
    ctx->ptr = p1;
    return 0;
}

/* ECMA-262 3rd Edition    7.3 */
static BOOL is_endline(WCHAR c)
{
    return c == '\n' || c == '\r' || c == 0x2028 || c == 0x2029;
}

int hex_to_int(WCHAR c)
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
    int min = 0, max = ARRAY_SIZE(keywords)-1, r, i;

    while(min <= max) {
        i = (min+max)/2;

        r = check_keyword(ctx, keywords[i].word, lval);
        if(!r) {
            if(ctx->script->version < keywords[i].min_version) {
                TRACE("ignoring keyword %s in incompatible mode\n",
                      debugstr_w(keywords[i].word));
                ctx->ptr -= lstrlenW(keywords[i].word);
                return 0;
            }
            ctx->implicit_nl_semicolon = keywords[i].no_nl;
            return keywords[i].token;
        }

        if(r > 0)
            min = i+1;
        else
            max = i-1;
    }

    return 0;
}

static BOOL skip_html_comment(parser_ctx_t *ctx)
{
    if(!ctx->is_html || ctx->ptr+3 >= ctx->end ||
        memcmp(ctx->ptr, L"<!--", sizeof(WCHAR)*4))
        return FALSE;

    ctx->nl = TRUE;
    while(ctx->ptr < ctx->end && !is_endline(*ctx->ptr++));

    return TRUE;
}

static BOOL skip_comment(parser_ctx_t *ctx)
{
    if(ctx->ptr+1 >= ctx->end)
        return FALSE;

    if(*ctx->ptr != '/') {
        if(*ctx->ptr == '@' && ctx->ptr+2 < ctx->end && ctx->ptr[1] == '*' && ctx->ptr[2] == '/') {
            ctx->ptr += 3;
            return TRUE;
        }

        return FALSE;
    }

    switch(ctx->ptr[1]) {
    case '*':
        ctx->ptr += 2;
        if(ctx->ptr+2 < ctx->end && *ctx->ptr == '@' && is_identifier_char(ctx->ptr[1]))
            return FALSE;
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
        if(ctx->ptr+2 < ctx->end && *ctx->ptr == '@' && is_identifier_char(ctx->ptr[1]))
            return FALSE;
        while(ctx->ptr < ctx->end && !is_endline(*ctx->ptr))
            ctx->ptr++;
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

static BOOL skip_spaces(parser_ctx_t *ctx)
{
    while(ctx->ptr < ctx->end && (iswspace(*ctx->ptr) || *ctx->ptr == 0xFEFF /* UTF16 BOM */)) {
        if(is_endline(*ctx->ptr++))
            ctx->nl = TRUE;
    }

    return ctx->ptr != ctx->end;
}

BOOL unescape(WCHAR *str, size_t *len)
{
    WCHAR *pd, *p, c, *end = str + *len;
    int i;

    pd = p = str;
    while(p < end) {
        if(*p != '\\') {
            *pd++ = *p++;
            continue;
        }

        if(++p == end)
            return FALSE;

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
        case 'f':
            c = '\f';
            break;
        case 'r':
            c = '\r';
            break;
        case 'x':
            if(p + 2 >= end)
                return FALSE;
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
            if(p + 4 >= end)
                return FALSE;
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
            c += i << 4;

            i = hex_to_int(*++p);
            if(i == -1)
                return FALSE;
            c += i;
            break;
        default:
            if(is_digit(*p)) {
                c = *p++ - '0';
                if(p < end && is_digit(*p)) {
                    c = c*8 + (*p++ - '0');
                    if(p < end && is_digit(*p))
                        c = c*8 + (*p++ - '0');
                }
                p--;
            }
            else
                c = *p;
        }

        *pd++ = c;
        p++;
    }

    *len = pd - str;
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
    memcpy(wstr, ptr, len*sizeof(WCHAR));
    wstr[len] = 0;

    /* FIXME: unescape */
    return tIdentifier;
}

static int parse_string_literal(parser_ctx_t *ctx, jsstr_t **ret, WCHAR endch)
{
    const WCHAR *ptr = ++ctx->ptr, *ret_str = ptr;
    BOOL needs_unescape = FALSE;
    WCHAR *unescape_str;
    size_t len;

    while(ctx->ptr < ctx->end && *ctx->ptr != endch) {
        if(*ctx->ptr++ == '\\') {
            ctx->ptr++;
            needs_unescape = TRUE;
        }
    }

    if(ctx->ptr == ctx->end)
        return lex_error(ctx, JS_E_UNTERMINATED_STRING);

    len = ctx->ptr - ptr;
    ctx->ptr++;

    if(needs_unescape) {
        ret_str = unescape_str = parser_alloc(ctx, len * sizeof(WCHAR));
        if(!unescape_str)
            return lex_error(ctx, E_OUTOFMEMORY);
        memcpy(unescape_str, ptr, len * sizeof(WCHAR));
        if(!unescape(unescape_str, &len)) {
            WARN("unescape failed\n");
            return lex_error(ctx, E_FAIL);
        }
    }

    if(!(*ret = compiler_alloc_string_len(ctx->compiler, ret_str, len)))
        return lex_error(ctx, E_OUTOFMEMORY);

    /* FIXME: leaking string */
    return tStringLiteral;
}

static literal_t *new_double_literal(parser_ctx_t *ctx, DOUBLE d)
{
    literal_t *ret = parser_alloc(ctx, sizeof(literal_t));

    ret->type = LT_DOUBLE;
    ret->u.dval = d;
    return ret;
}

literal_t *new_boolean_literal(parser_ctx_t *ctx, BOOL bval)
{
    literal_t *ret = parser_alloc(ctx, sizeof(literal_t));

    ret->type = LT_BOOL;
    ret->u.bval = bval;

    return ret;
}

HRESULT parse_decimal(const WCHAR **iter, const WCHAR *end, double *ret)
{
    const WCHAR *ptr = *iter;
    LONGLONG d = 0, hlp;
    int exp = 0;

    while(ptr < end && is_digit(*ptr)) {
        hlp = d*10 + *(ptr++) - '0';
        if(d>MAXLONGLONG/10 || hlp<0) {
            exp++;
            break;
        }
        else
            d = hlp;
    }
    while(ptr < end && is_digit(*ptr)) {
        exp++;
        ptr++;
    }

    if(*ptr == '.') {
        ptr++;

        while(ptr < end && is_digit(*ptr)) {
            hlp = d*10 + *(ptr++) - '0';
            if(d>MAXLONGLONG/10 || hlp<0)
                break;

            d = hlp;
            exp--;
        }
        while(ptr < end && is_digit(*ptr))
            ptr++;
    }

    if(ptr < end && (*ptr == 'e' || *ptr == 'E')) {
        int sign = 1, e = 0;

        if(++ptr < end) {
            if(*ptr == '+') {
                ptr++;
            }else if(*ptr == '-') {
                sign = -1;
                ptr++;
            }else if(!is_digit(*ptr)) {
                WARN("Expected exponent part\n");
                return E_FAIL;
            }
        }

        if(ptr == end) {
            WARN("unexpected end of file\n");
            return E_FAIL;
        }

        while(ptr < end && is_digit(*ptr)) {
            if(e > INT_MAX/10 || (e = e*10 + *ptr++ - '0')<0)
                e = INT_MAX;
        }
        e *= sign;

        if(exp<0 && e<0 && e+exp>0) exp = INT_MIN;
        else if(exp>0 && e>0 && e+exp<0) exp = INT_MAX;
        else exp += e;
    }

    if(is_identifier_char(*ptr)) {
        WARN("wrong char after zero\n");
        return JS_E_MISSING_SEMICOLON;
    }

    *ret = exp>=0 ? d*pow(10, exp) : d/pow(10, -exp);
    *iter = ptr;
    return S_OK;
}

static BOOL parse_numeric_literal(parser_ctx_t *ctx, double *ret)
{
    HRESULT hres;

    if(*ctx->ptr == '0') {
        ctx->ptr++;

        if(*ctx->ptr == 'x' || *ctx->ptr == 'X') {
            double r = 0;
            int d;
            if(++ctx->ptr == ctx->end) {
                ERR("unexpected end of file\n");
                return FALSE;
            }

            while(ctx->ptr < ctx->end && (d = hex_to_int(*ctx->ptr)) != -1) {
                r = r*16 + d;
                ctx->ptr++;
            }

            if(ctx->ptr < ctx->end && is_identifier_char(*ctx->ptr)) {
                WARN("unexpected identifier char\n");
                lex_error(ctx, JS_E_MISSING_SEMICOLON);
                return FALSE;
            }

            *ret = r;
            return TRUE;
        }

        if(is_digit(*ctx->ptr)) {
            unsigned base = 8;
            const WCHAR *ptr;
            double val = 0;

            for(ptr = ctx->ptr; ptr < ctx->end && is_digit(*ptr); ptr++) {
                if(*ptr > '7') {
                    base = 10;
                    break;
                }
            }

            do {
                val = val*base + *ctx->ptr-'0';
            }while(++ctx->ptr < ctx->end && is_digit(*ctx->ptr));

            /* FIXME: Do we need it here? */
            if(ctx->ptr < ctx->end && (is_identifier_char(*ctx->ptr) || *ctx->ptr == '.')) {
                WARN("wrong char after octal literal: '%c'\n", *ctx->ptr);
                lex_error(ctx, JS_E_MISSING_SEMICOLON);
                return FALSE;
            }

            *ret = val;
            return TRUE;
        }

        if(is_identifier_char(*ctx->ptr)) {
            WARN("wrong char after zero\n");
            lex_error(ctx, JS_E_MISSING_SEMICOLON);
            return FALSE;
        }
    }

    hres = parse_decimal(&ctx->ptr, ctx->end, ret);
    if(FAILED(hres)) {
        lex_error(ctx, hres);
        return FALSE;
    }

    return TRUE;
}

static int next_token(parser_ctx_t *ctx, unsigned *loc, void *lval)
{
    do {
        if(!skip_spaces(ctx)) {
            *loc  = ctx->ptr - ctx->begin;
            return 0;
        }
    }while(skip_comment(ctx) || skip_html_comment(ctx));
    *loc  = ctx->ptr - ctx->begin;

    if(ctx->implicit_nl_semicolon) {
        if(ctx->nl)
            return ';';
        ctx->implicit_nl_semicolon = FALSE;
    }

    if(iswalpha(*ctx->ptr)) {
        int ret = check_keywords(ctx, lval);
        if(ret)
            return ret;

        return parse_identifier(ctx, lval);
    }

    if(is_digit(*ctx->ptr)) {
        double n;

        if(!parse_numeric_literal(ctx, &n))
            return -1;

        *(literal_t**)lval = new_double_literal(ctx, n);
        return tNumericLiteral;
    }

    switch(*ctx->ptr) {
    case '{':
    case '}':
    case '(':
    case ')':
    case '[':
    case ']':
    case ';':
    case ',':
    case '~':
    case '?':
        return *ctx->ptr++;

    case '.':
        if(ctx->ptr+1 < ctx->end && is_digit(ctx->ptr[1])) {
            double n;
            HRESULT hres;
            hres = parse_decimal(&ctx->ptr, ctx->end, &n);
            if(FAILED(hres)) {
                lex_error(ctx, hres);
                return -1;
            }
            *(literal_t**)lval = new_double_literal(ctx, n);
            return tNumericLiteral;
        }
        ctx->ptr++;
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
                return kDIVEQ;
            }
        }
        return '/';

    case ':':
        if(++ctx->ptr < ctx->end && *ctx->ptr == ':') {
            ctx->ptr++;
            return kDCOL;
        }
        return ':';

    case '\"':
    case '\'':
        return parse_string_literal(ctx, lval, *ctx->ptr);

    case '_':
    case '$':
        return parse_identifier(ctx, lval);

    case '@':
        return '@';
    }

    WARN("unexpected char '%c' %d\n", *ctx->ptr, *ctx->ptr);
    return 0;
}

struct _cc_var_t {
    ccval_t val;
    struct _cc_var_t *next;
    unsigned name_len;
    WCHAR name[0];
};

void release_cc(cc_ctx_t *cc)
{
    cc_var_t *iter, *next;

    for(iter = cc->vars; iter; iter = next) {
        next = iter->next;
        free(iter);
    }

    free(cc);
}

static BOOL new_cc_var(cc_ctx_t *cc, const WCHAR *name, int len, ccval_t v)
{
    cc_var_t *new_v;

    if(len == -1)
        len = lstrlenW(name);

    new_v = malloc(sizeof(cc_var_t) + (len+1)*sizeof(WCHAR));
    if(!new_v)
        return FALSE;

    new_v->val = v;
    memcpy(new_v->name, name, (len+1)*sizeof(WCHAR));
    new_v->name_len = len;
    new_v->next = cc->vars;
    cc->vars = new_v;
    return TRUE;
}

static cc_var_t *find_cc_var(cc_ctx_t *cc, const WCHAR *name, unsigned name_len)
{
    cc_var_t *iter;

    for(iter = cc->vars; iter; iter = iter->next) {
        if(iter->name_len == name_len && !memcmp(iter->name, name, name_len*sizeof(WCHAR)))
            return iter;
    }

    return NULL;
}

static BOOL init_cc(parser_ctx_t *ctx)
{
    cc_ctx_t *cc;

    if(ctx->script->cc)
        return TRUE;

    cc = malloc(sizeof(cc_ctx_t));
    if(!cc) {
        lex_error(ctx, E_OUTOFMEMORY);
        return FALSE;
    }

    cc->vars = NULL;

    if(!new_cc_var(cc, L"_jscript", -1, ccval_bool(TRUE))
       || !new_cc_var(cc, sizeof(void*) == 8 ? L"_win64" : L"_win32", -1, ccval_bool(TRUE))
       || !new_cc_var(cc, sizeof(void*) == 8 ? L"_amd64" : L"_x86", -1, ccval_bool(TRUE))
       || !new_cc_var(cc, L"_jscript_version", -1, ccval_num(JSCRIPT_MAJOR_VERSION + (DOUBLE)JSCRIPT_MINOR_VERSION/10.0))
       || !new_cc_var(cc, L"_jscript_build", -1, ccval_num(JSCRIPT_BUILD_VERSION))) {
        release_cc(cc);
        lex_error(ctx, E_OUTOFMEMORY);
        return FALSE;
    }

    ctx->script->cc = cc;
    return TRUE;
}

static BOOL parse_cc_identifier(parser_ctx_t *ctx, const WCHAR **ret, unsigned *ret_len)
{
    if(*ctx->ptr != '@') {
        lex_error(ctx, JS_E_EXPECTED_AT);
        return FALSE;
    }

    if(!is_identifier_first_char(*++ctx->ptr)) {
        lex_error(ctx, JS_E_EXPECTED_IDENTIFIER);
        return FALSE;
    }

    *ret = ctx->ptr;
    while(++ctx->ptr < ctx->end && is_identifier_char(*ctx->ptr));
    *ret_len = ctx->ptr - *ret;
    return TRUE;
}

int try_parse_ccval(parser_ctx_t *ctx, ccval_t *r)
{
    if(!skip_spaces(ctx))
        return -1;

    if(is_digit(*ctx->ptr)) {
        double n;

        if(!parse_numeric_literal(ctx, &n))
            return -1;

        *r = ccval_num(n);
        return 1;
    }

    if(*ctx->ptr == '@') {
        const WCHAR *ident;
        unsigned ident_len;
        cc_var_t *cc_var;

        if(!parse_cc_identifier(ctx, &ident, &ident_len))
            return -1;

        cc_var = find_cc_var(ctx->script->cc, ident, ident_len);
        *r = cc_var ? cc_var->val : ccval_num(NAN);
        return 1;
    }

    if(!check_keyword(ctx, L"true", NULL)) {
        *r = ccval_bool(TRUE);
        return 1;
    }

    if(!check_keyword(ctx, L"false", NULL)) {
        *r = ccval_bool(FALSE);
        return 1;
    }

    return 0;
}

static int skip_code(parser_ctx_t *ctx, BOOL exec_else)
{
    int if_depth = 1;
    const WCHAR *ptr;

    while(1) {
        ptr = wcschr(ctx->ptr, '@');
        if(!ptr) {
            WARN("No @end\n");
            return lex_error(ctx, JS_E_EXPECTED_CCEND);
        }
        ctx->ptr = ptr+1;

        if(!check_keyword(ctx, L"end", NULL)) {
            if(--if_depth)
                continue;
            return 0;
        }

        if(exec_else && !check_keyword(ctx, L"elif", NULL)) {
            if(if_depth > 1)
                continue;

            if(!skip_spaces(ctx) || *ctx->ptr != '(')
                return lex_error(ctx, JS_E_MISSING_LBRACKET);

            if(!parse_cc_expr(ctx))
                return -1;

            if(!get_ccbool(ctx->ccval))
                continue; /* skip block of code */

            /* continue parsing */
            ctx->cc_if_depth++;
            return 0;
        }

        if(exec_else && !check_keyword(ctx, L"else", NULL)) {
            if(if_depth > 1)
                continue;

            /* parse else block */
            ctx->cc_if_depth++;
            return 0;
        }

        if(!check_keyword(ctx, L"if", NULL)) {
            if_depth++;
            continue;
        }

        ctx->ptr++;
    }
}

static int cc_token(parser_ctx_t *ctx, void *lval)
{
    unsigned id_len = 0;
    cc_var_t *var;

    ctx->ptr++;

    if(!check_keyword(ctx, L"cc_on", NULL))
        return init_cc(ctx) ? 0 : -1;

    if(!check_keyword(ctx, L"set", NULL)) {
        const WCHAR *ident;
        unsigned ident_len;
        cc_var_t *var;

        if(!init_cc(ctx))
            return -1;

        if(!skip_spaces(ctx))
            return lex_error(ctx, JS_E_EXPECTED_AT);

        if(!parse_cc_identifier(ctx, &ident, &ident_len))
            return -1;

        if(!skip_spaces(ctx) || *ctx->ptr != '=')
            return lex_error(ctx, JS_E_EXPECTED_ASSIGN);
        ctx->ptr++;

        if(!parse_cc_expr(ctx)) {
            WARN("parsing CC expression failed\n");
            return -1;
        }

        var = find_cc_var(ctx->script->cc, ident, ident_len);
        if(var) {
            var->val = ctx->ccval;
        }else {
            if(!new_cc_var(ctx->script->cc, ident, ident_len, ctx->ccval))
                return lex_error(ctx, E_OUTOFMEMORY);
        }

        return 0;
    }

    if(!check_keyword(ctx, L"if", NULL)) {
        if(!init_cc(ctx))
            return -1;

        if(!skip_spaces(ctx) || *ctx->ptr != '(')
            return lex_error(ctx, JS_E_MISSING_LBRACKET);

        if(!parse_cc_expr(ctx))
            return -1;

        if(get_ccbool(ctx->ccval)) {
            /* continue parsing block inside if */
            ctx->cc_if_depth++;
            return 0;
        }

        return skip_code(ctx, TRUE);
    }

    if(!check_keyword(ctx, L"elif", NULL) || !check_keyword(ctx, L"else", NULL)) {
        if(!ctx->cc_if_depth)
            return lex_error(ctx, JS_E_SYNTAX);

        return skip_code(ctx, FALSE);
    }

    if(!check_keyword(ctx, L"end", NULL)) {
        if(!ctx->cc_if_depth)
            return lex_error(ctx, JS_E_SYNTAX);

        ctx->cc_if_depth--;
        return 0;
    }

    if(!ctx->script->cc)
        return lex_error(ctx, JS_E_DISABLED_CC);

    while(ctx->ptr+id_len < ctx->end && is_identifier_char(ctx->ptr[id_len]))
        id_len++;
    if(!id_len)
        return '@';

    TRACE("var %s\n", debugstr_wn(ctx->ptr, id_len));

    var = find_cc_var(ctx->script->cc, ctx->ptr, id_len);
    ctx->ptr += id_len;
    if(!var || var->val.is_num) {
        *(literal_t**)lval = new_double_literal(ctx, var ? var->val.u.n : NAN);
        return tNumericLiteral;
    }

    *(literal_t**)lval = new_boolean_literal(ctx, var->val.u.b);
    return tBooleanLiteral;
}

int parser_lex(void *lval, unsigned *loc, parser_ctx_t *ctx)
{
    int ret;

    ctx->nl = ctx->ptr == ctx->begin;

    do {
        ret = next_token(ctx, loc, lval);
    } while(ret == '@' && !(ret = cc_token(ctx, lval)));

    return ret;
}

literal_t *parse_regexp(parser_ctx_t *ctx)
{
    const WCHAR *re, *flags_ptr;
    BOOL in_class = FALSE;
    DWORD re_len, flags;
    literal_t *ret;

    TRACE("\n");

    while(*--ctx->ptr != '/');

    /* Simple regexp pre-parser; '/' if used in char class does not terminate regexp literal */
    re = ++ctx->ptr;
    while(ctx->ptr < ctx->end) {
        if(*ctx->ptr == '\\') {
            if(++ctx->ptr == ctx->end)
                break;
        }else if(in_class) {
            if(*ctx->ptr == '\n')
                break;
            if(*ctx->ptr == ']')
                in_class = FALSE;
        }else {
            if(*ctx->ptr == '/')
                break;

            if(*ctx->ptr == '[')
                in_class = TRUE;
        }
        ctx->ptr++;
    }

    if(ctx->ptr == ctx->end || *ctx->ptr != '/') {
        WARN("pre-parsing failed\n");
        ctx->hres = JS_E_SYNTAX;
        return NULL;
    }

    re_len = ctx->ptr-re;

    flags_ptr = ++ctx->ptr;
    while(ctx->ptr < ctx->end && iswalnum(*ctx->ptr))
        ctx->ptr++;

    ctx->hres = parse_regexp_flags(flags_ptr, ctx->ptr-flags_ptr, &flags);
    if(FAILED(ctx->hres))
        return NULL;

    ret = parser_alloc(ctx, sizeof(literal_t));
    ret->type = LT_REGEXP;
    ret->u.regexp.str = compiler_alloc_string_len(ctx->compiler, re, re_len);
    ret->u.regexp.flags = flags;
    return ret;
}
