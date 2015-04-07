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

#include "jscript.h"

#include "parser.tab.h"

#define LONGLONG_MAX (((LONGLONG)0x7fffffff<<32)|0xffffffff)

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
static const WCHAR varW[] = {'v','a','r',0};
static const WCHAR voidW[] = {'v','o','i','d',0};
static const WCHAR whileW[] = {'w','h','i','l','e',0};
static const WCHAR withW[] = {'w','i','t','h',0};

static const WCHAR elifW[] = {'e','l','i','f',0};
static const WCHAR endW[] = {'e','n','d',0};

static const struct {
    const WCHAR *word;
    int token;
    BOOL no_nl;
} keywords[] = {
    {breakW,       kBREAK, TRUE},
    {caseW,        kCASE},
    {catchW,       kCATCH},
    {continueW,    kCONTINUE, TRUE},
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
    {returnW,      kRETURN, TRUE},
    {switchW,      kSWITCH},
    {thisW,        kTHIS},
    {throwW,       kTHROW},
    {trueW,        kTRUE},
    {tryW,         kTRY},
    {typeofW,      kTYPEOF},
    {varW,         kVAR},
    {voidW,        kVOID},
    {whileW,       kWHILE},
    {withW,        kWITH}
};

static int lex_error(parser_ctx_t *ctx, HRESULT hres)
{
    ctx->hres = hres;
    ctx->lexer_error = TRUE;
    return -1;
}

/* ECMA-262 3rd Edition    7.6 */
static BOOL is_identifier_char(WCHAR c)
{
    return isalnumW(c) || c == '$' || c == '_' || c == '\\';
}

static BOOL is_identifier_first_char(WCHAR c)
{
    return isalphaW(c) || c == '$' || c == '_' || c == '\\';
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
        *lval = ctx->ptr;
    ctx->ptr = p1;
    return 0;
}

/* ECMA-262 3rd Edition    7.3 */
static BOOL is_endline(WCHAR c)
{
    return c == '\n' || c == '\r' || c == 0x2028 || c == 0x2029;
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
        if(!r) {
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
    while(ctx->ptr < ctx->end && (isspaceW(*ctx->ptr) || *ctx->ptr == 0xFEFF /* UTF16 BOM */)) {
        if(is_endline(*ctx->ptr++))
            ctx->nl = TRUE;
    }

    return ctx->ptr != ctx->end;
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
            c += i << 4;

            i = hex_to_int(*++p);
            if(i == -1)
                return FALSE;
            c += i;
            break;
        default:
            if(isdigitW(*p)) {
                c = *p++ - '0';
                if(isdigitW(*p)) {
                    c = c*8 + (*p++ - '0');
                    if(isdigitW(*p))
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
    memcpy(wstr, ptr, len*sizeof(WCHAR));
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

    if(ctx->ptr == ctx->end)
        return lex_error(ctx, JS_E_UNTERMINATED_STRING);

    len = ctx->ptr-ptr;

    *ret = wstr = parser_alloc(ctx, (len+1)*sizeof(WCHAR));
    memcpy(wstr, ptr, len*sizeof(WCHAR));
    wstr[len] = 0;

    ctx->ptr++;

    if(!unescape(wstr)) {
        WARN("unescape failed\n");
        return lex_error(ctx, E_FAIL);
    }

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

static BOOL parse_double_literal(parser_ctx_t *ctx, LONG int_part, double *ret)
{
    LONGLONG d, hlp;
    int exp = 0;

    d = int_part;
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
                lex_error(ctx, E_FAIL);
                return FALSE;
            }
        }

        if(ctx->ptr == ctx->end) {
            WARN("unexpected end of file\n");
            lex_error(ctx, E_FAIL);
            return FALSE;
        }

        while(ctx->ptr < ctx->end && isdigitW(*ctx->ptr)) {
            if(e > INT_MAX/10 || (e = e*10 + *ctx->ptr++ - '0')<0)
                e = INT_MAX;
        }
        e *= sign;

        if(exp<0 && e<0 && e+exp>0) exp = INT_MIN;
        else if(exp>0 && e>0 && e+exp<0) exp = INT_MAX;
        else exp += e;
    }

    if(is_identifier_char(*ctx->ptr)) {
        WARN("wrong char after zero\n");
        lex_error(ctx, JS_E_MISSING_SEMICOLON);
        return FALSE;
    }

    *ret = exp>=0 ? d*pow(10, exp) : d/pow(10, -exp);
    return TRUE;
}

static BOOL parse_numeric_literal(parser_ctx_t *ctx, double *ret)
{
    LONG l, d;

    l = *ctx->ptr++ - '0';
    if(!l) {
        if(*ctx->ptr == 'x' || *ctx->ptr == 'X') {
            if(++ctx->ptr == ctx->end) {
                ERR("unexpected end of file\n");
                return FALSE;
            }

            while(ctx->ptr < ctx->end && (d = hex_to_int(*ctx->ptr)) != -1) {
                l = l*16 + d;
                ctx->ptr++;
            }

            if(ctx->ptr < ctx->end && is_identifier_char(*ctx->ptr)) {
                WARN("unexpected identifier char\n");
                lex_error(ctx, JS_E_MISSING_SEMICOLON);
                return FALSE;
            }

            *ret = l;
            return TRUE;
        }

        if(isdigitW(*ctx->ptr)) {
            unsigned base = 8;
            const WCHAR *ptr;
            double val = 0;

            for(ptr = ctx->ptr; ptr < ctx->end && isdigitW(*ptr); ptr++) {
                if(*ptr > '7') {
                    base = 10;
                    break;
                }
            }

            do {
                val = val*base + *ctx->ptr-'0';
            }while(++ctx->ptr < ctx->end && isdigitW(*ctx->ptr));

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

    return parse_double_literal(ctx, l, ret);
}

static int next_token(parser_ctx_t *ctx, void *lval)
{
    do {
        if(!skip_spaces(ctx))
            return tEOF;
    }while(skip_comment(ctx) || skip_html_comment(ctx));

    if(ctx->implicit_nl_semicolon) {
        if(ctx->nl)
            return ';';
        ctx->implicit_nl_semicolon = FALSE;
    }

    if(isalphaW(*ctx->ptr)) {
        int ret = check_keywords(ctx, lval);
        if(ret)
            return ret;

        return parse_identifier(ctx, lval);
    }

    if(isdigitW(*ctx->ptr)) {
        double n;

        if(!parse_numeric_literal(ctx, &n))
            return -1;

        *(literal_t**)lval = new_double_literal(ctx, n);
        return tNumericLiteral;
    }

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
        if(++ctx->ptr < ctx->end && isdigitW(*ctx->ptr)) {
            double n;
            if(!parse_double_literal(ctx, 0, &n))
                return -1;
            *(literal_t**)lval = new_double_literal(ctx, n);
            return tNumericLiteral;
        }
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
        heap_free(iter);
    }

    heap_free(cc);
}

static BOOL new_cc_var(cc_ctx_t *cc, const WCHAR *name, int len, ccval_t v)
{
    cc_var_t *new_v;

    if(len == -1)
        len = strlenW(name);

    new_v = heap_alloc(sizeof(cc_var_t) + (len+1)*sizeof(WCHAR));
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

    static const WCHAR _win32W[] = {'_','w','i','n','3','2',0};
    static const WCHAR _win64W[] = {'_','w','i','n','6','4',0};
    static const WCHAR _x86W[] = {'_','x','8','6',0};
    static const WCHAR _amd64W[] = {'_','a','m','d','6','4',0};
    static const WCHAR _jscriptW[] = {'_','j','s','c','r','i','p','t',0};
    static const WCHAR _jscript_buildW[] = {'_','j','s','c','r','i','p','t','_','b','u','i','l','d',0};
    static const WCHAR _jscript_versionW[] = {'_','j','s','c','r','i','p','t','_','v','e','r','s','i','o','n',0};

    if(ctx->script->cc)
        return TRUE;

    cc = heap_alloc(sizeof(cc_ctx_t));
    if(!cc) {
        lex_error(ctx, E_OUTOFMEMORY);
        return FALSE;
    }

    cc->vars = NULL;

    if(!new_cc_var(cc, _jscriptW, -1, ccval_bool(TRUE))
       || !new_cc_var(cc, sizeof(void*) == 8 ? _win64W : _win32W, -1, ccval_bool(TRUE))
       || !new_cc_var(cc, sizeof(void*) == 8 ? _amd64W : _x86W, -1, ccval_bool(TRUE))
       || !new_cc_var(cc, _jscript_versionW, -1, ccval_num(JSCRIPT_MAJOR_VERSION + (DOUBLE)JSCRIPT_MINOR_VERSION/10.0))
       || !new_cc_var(cc, _jscript_buildW, -1, ccval_num(JSCRIPT_BUILD_VERSION))) {
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

    if(isdigitW(*ctx->ptr)) {
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

    if(!check_keyword(ctx, trueW, NULL)) {
        *r = ccval_bool(TRUE);
        return 1;
    }

    if(!check_keyword(ctx, falseW, NULL)) {
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
        ptr = strchrW(ctx->ptr, '@');
        if(!ptr) {
            WARN("No @end\n");
            return lex_error(ctx, JS_E_EXPECTED_CCEND);
        }
        ctx->ptr = ptr+1;

        if(!check_keyword(ctx, endW, NULL)) {
            if(--if_depth)
                continue;
            return 0;
        }

        if(exec_else && !check_keyword(ctx, elifW, NULL)) {
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

        if(exec_else && !check_keyword(ctx, elseW, NULL)) {
            if(if_depth > 1)
                continue;

            /* parse else block */
            ctx->cc_if_depth++;
            return 0;
        }

        if(!check_keyword(ctx, ifW, NULL)) {
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

    static const WCHAR cc_onW[] = {'c','c','_','o','n',0};
    static const WCHAR setW[] = {'s','e','t',0};

    ctx->ptr++;

    if(!check_keyword(ctx, cc_onW, NULL))
        return init_cc(ctx) ? 0 : -1;

    if(!check_keyword(ctx, setW, NULL)) {
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

    if(!check_keyword(ctx, ifW, NULL)) {
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

    if(!check_keyword(ctx, elifW, NULL) || !check_keyword(ctx, elseW, NULL)) {
        if(!ctx->cc_if_depth)
            return lex_error(ctx, JS_E_SYNTAX);

        return skip_code(ctx, FALSE);
    }

    if(!check_keyword(ctx, endW, NULL)) {
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

int parser_lex(void *lval, parser_ctx_t *ctx)
{
    int ret;

    ctx->nl = ctx->ptr == ctx->begin;

    do {
        ret = next_token(ctx, lval);
    } while(ret == '@' && !(ret = cc_token(ctx, lval)));

    return ret;
}

literal_t *parse_regexp(parser_ctx_t *ctx)
{
    const WCHAR *re, *flags_ptr;
    BOOL in_class = FALSE;
    DWORD re_len, flags;
    literal_t *ret;
    HRESULT hres;

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
        return NULL;
    }

    re_len = ctx->ptr-re;

    flags_ptr = ++ctx->ptr;
    while(ctx->ptr < ctx->end && isalnumW(*ctx->ptr))
        ctx->ptr++;

    hres = parse_regexp_flags(flags_ptr, ctx->ptr-flags_ptr, &flags);
    if(FAILED(hres))
        return NULL;

    ret = parser_alloc(ctx, sizeof(literal_t));
    ret->type = LT_REGEXP;
    ret->u.regexp.str = re;
    ret->u.regexp.str_len = re_len;
    ret->u.regexp.flags = flags;
    return ret;
}
