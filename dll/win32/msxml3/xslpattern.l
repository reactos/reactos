/*
 *    XSLPattern lexer
 *
 * Copyright 2010 Adam Martinson for CodeWeavers
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
#include "xslpattern.h"
#include "xslpattern.tab.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#define SCAN    xslpattern_get_extra(yyscanner)
#define YYSTYPE XSLPATTERN_STYPE

#define YY_INPUT(tok_buf, tok_len, max) \
        do { \
            if (SCAN->pos <= SCAN->len) \
            { \
                tok_len = SCAN->len - SCAN->pos; \
                if (tok_len > max) tok_len = max; \
                memcpy(tok_buf, SCAN->in + SCAN->pos, tok_len); \
                SCAN->pos += tok_len; \
            } \
            else \
            { \
                tok_len = YY_NULL; \
            } \
        } while (0);

#define TOK(tok)    TRACE("token: %s : %s\n", #tok, yytext); return tok
#define OP(tok)     *yylval=NULL; TOK(tok)
#define SYM(tok)    *yylval=NULL; TOK(tok)
#define STR(tok)    *yylval=xmlStrdup(BAD_CAST yytext); TOK(tok)


%}

%option reentrant bison-bridge
%option noyywrap
%option prefix="xslpattern_"
%option noinput nounput never-interactive

/* From the w3c XML standard
 * <http://www.w3.org/TR/REC-xml/> */

    /* [2.3] Common Syntactic Constructs */
WSpace          ([[:space:]])

NCNameStartChar ([A-Za-z_]|[\xc0-\xd6\xd8-\xf6\xf8-\xff])

NameCharEx      ([0-9]|[-._\xb7])

NCNameChar      ({NCNameStartChar}|{NameCharEx})

/* From the w3c XML Namespace standard
 * <http://www.w3.org/TR/REC-xml-names/> */

    /* [3] Declaring Namespaces*/
NCName          ({NCNameStartChar}{NCNameChar}*)

/* Mostly verbatim from the w3c XPath standard.
 * <http://www.w3.org/TR/xpath/> */


    /* [3.4] Booleans
     * ||, &&, $foo$ are XSLPattern only */

OP_Or           ("or"|"||"|"$or$")
OP_And          ("and"|"&&"|"$and$")
OP_Eq           ("="|"$eq$")
OP_IEq          ("$ieq$")
OP_NEq          ("!="|"$ne$")
OP_INEq         ("$ine$")
OP_Lt           ("<"|"$lt$")
OP_ILt          ("$ilt$")
OP_Gt           (">"|"$gt$")
OP_IGt          ("$igt$")
OP_LEq          ("<="|"$le$")
OP_ILEq         ("$ile$")
OP_GEq          (">="|"$ge$")
OP_IGEq         ("$ige$")
OP_Not          ("$not$")
OP_All          ("$all$")
OP_Any          ("$any$")

    /* [3.7] Lexical Structure */
Literal             (([\x22]([^\x22]*)[\x22])|([\x27]([^\x27]*)[\x27]))
Number              ({Digits}("."{Digits}?)?|"."{Digits})
Digits              ([0-9]+)

ANY                 (.)

%%

{WSpace}+                   { /* ignored */ }
{Literal}                   { STR(TOK_Literal); }
"//"                        { SYM(TOK_DblFSlash); }
"/"                         { SYM(TOK_FSlash); }
".."                        { SYM(TOK_Parent); }
"."                         { SYM(TOK_Self); }
"::"                        { SYM(TOK_Axis); }
":"                         { SYM(TOK_Colon); }
"("                         { SYM('('); }
")"                         { SYM(')'); }
"["                         { SYM('['); }
"]"                         { SYM(']'); }
"@"                         { SYM('@'); }
","                         { SYM(','); }
"*"                         { SYM('*'); }
{OP_And}                    { OP(TOK_OpAnd); }
{OP_Or}                     { OP(TOK_OpOr); }
{OP_Not}                    { OP(TOK_OpNot); }
{OP_Eq}                     { OP(TOK_OpEq); }
{OP_IEq}                    { OP(TOK_OpIEq); }
{OP_NEq}                    { OP(TOK_OpNEq); }
{OP_INEq}                   { OP(TOK_OpINEq); }
{OP_Lt}                     { OP(TOK_OpLt); }
{OP_ILt}                    { OP(TOK_OpILt); }
{OP_Gt}                     { OP(TOK_OpGt); }
{OP_IGt}                    { OP(TOK_OpIGt); }
{OP_LEq}                    { OP(TOK_OpLEq); }
{OP_ILEq}                   { OP(TOK_OpILEq); }
{OP_GEq}                    { OP(TOK_OpGEq); }
{OP_IGEq}                   { OP(TOK_OpIGEq); }
{OP_All}                    { OP(TOK_OpAll); }
{OP_Any}                    { OP(TOK_OpAny); }
"|"                         { SYM('|'); }
"!"                         { SYM('!'); }
{NCName}                    { STR(TOK_NCName); }
{Number}                    { STR(TOK_Number); }
{ANY}                       { FIXME("Unexpected character '%s'.\n",yytext); }

%%

xmlChar* XSLPattern_to_XPath(xmlXPathContextPtr, xmlChar const*);
xmlChar* XSLPattern_to_XPath(xmlXPathContextPtr ctxt, xmlChar const* xslpat_str)
{
    parser_param p;
    TRACE("(%s)\n", debugstr_a((char const*)xslpat_str));
    memset(&p, 0, sizeof(parser_param));
    p.ctx = ctxt;
    p.in = xslpat_str;
    p.len = xmlStrlen(xslpat_str);

    xslpattern_lex_init(&p.yyscanner);
    xslpattern_set_extra(&p, p.yyscanner);

    xslpattern_parse(&p, p.yyscanner);

    TRACE("=> %s\n", debugstr_a((char const*)p.out));
    xslpattern_lex_destroy(p.yyscanner);

    if (p.err)
    {
        xmlFree(p.out);
        return xmlStrdup(xslpat_str);
    }
    else
    {
        return p.out;
    }

}
