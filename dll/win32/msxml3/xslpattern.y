/*
 *    XSLPattern parser (XSLPattern => XPath)
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
#include "config.h"
#include "wine/port.h"

#ifdef HAVE_LIBXML2
#include "xslpattern.h"
#include <libxml/xpathInternals.h>
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);


static const xmlChar NameTest_mod_pre[] = "*[name()='";
static const xmlChar NameTest_mod_post[] = "']";

#define U(str) BAD_CAST str

static inline BOOL is_literal(xmlChar const* tok)
{
    return (tok && tok[0] && tok[1] &&
            tok[0]== tok[xmlStrlen(tok)-1] &&
            (tok[0] == '\'' || tok[0] == '"'));
}

static void xslpattern_error(parser_param* param, void const* scanner, char const* msg)
{
    FIXME("%s:\n"
          "  param {\n"
          "    yyscanner=%p\n"
          "    ctx=%p\n"
          "    in=\"%s\"\n"
          "    pos=%i\n"
          "    len=%i\n"
          "    out=\"%s\"\n"
          "    err=%i\n"
          "  }\n"
          "  scanner=%p\n",
          msg, param->yyscanner, param->ctx, param->in, param->pos,
          param->len, param->out, ++param->err, scanner);
}

%}

%token TOK_Parent TOK_Self TOK_DblFSlash TOK_FSlash TOK_Axis TOK_Colon
%token TOK_OpAnd TOK_OpOr TOK_OpNot
%token TOK_OpEq TOK_OpIEq TOK_OpNEq TOK_OpINEq
%token TOK_OpLt TOK_OpILt TOK_OpGt TOK_OpIGt TOK_OpLEq TOK_OpILEq TOK_OpGEq TOK_OpIGEq
%token TOK_OpAll TOK_OpAny
%token TOK_NCName TOK_Literal TOK_Number

%start XSLPattern

%pure-parser
%parse-param {parser_param* p}
%parse-param {void* scanner}
%lex-param {yyscan_t* scanner}

%left TOK_OpAnd TOK_OpOr
%left TOK_OpEq TOK_OpIEq TOK_OpNEq TOK_OpINEq
%left TOK_OpLt TOK_OpILt TOK_OpGt TOK_OpIGt TOK_OpLEq TOK_OpILEq TOK_OpGEq TOK_OpIGEq

%expect 14

%%

    XSLPattern              : Expr
                            {
                                p->out = $1;
                            }
    ;

/* Mostly verbatim from the w3c XML Namespaces standard.
 * <http://www.w3.org/TR/REC-xml-names/> */

    /* [4] Qualified Names */
    QName                   : PrefixedName
                            | UnprefixedName
    ;
    PrefixedName            : TOK_NCName TOK_Colon TOK_NCName
                            {
                                TRACE("Got PrefixedName: \"%s:%s\"\n", $1, $3);
                                $$=$1;
                                $$=xmlStrcat($$,U(":"));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
    ;
    UnprefixedName          : TOK_NCName
                            {
                                TRACE("Got UnprefixedName: \"%s\"\n", $1);
                                $$=$1;
                            }
    ;

/* Based on the w3c XPath standard, adapted where needed.
 * <http://www.w3.org/TR/xpath/> */

    /* [2] Location Paths */
    LocationPath            : RelativeLocationPath
                            | AbsoluteLocationPath
    ;
    AbsoluteLocationPath    : TOK_FSlash RelativeLocationPath
                            {
                                TRACE("Got AbsoluteLocationPath: \"/%s\"\n", $2);
                                $$=xmlStrdup(U("/"));
                                $$=xmlStrcat($$,$2);
                                xmlFree($2);
                            }
                            | TOK_FSlash
                            {
                                TRACE("Got AbsoluteLocationPath: \"/\"\n");
                                $$=xmlStrdup(U("/"));
                            }
                            | AbbreviatedAbsoluteLocationPath
    ;
    RelativeLocationPath    : Step
                            | RelativeLocationPath TOK_FSlash Step
                            {
                                TRACE("Got RelativeLocationPath: \"%s/%s\"\n", $1, $3);
                                $$=$1;
                                $$=xmlStrcat($$,U("/"));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
                            | AbbreviatedRelativeLocationPath
    ;
    /* [2.1] Location Steps */
    Step                    : AxisSpecifier NodeTest Predicates
                            {
                                TRACE("Got Step: \"%s%s%s\"\n", $1, $2, $3);
                                $$=$1;
                                $$=xmlStrcat($$,$2);
                                xmlFree($2);
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
                            | NodeTest Predicates
                            {
                                TRACE("Got Step: \"%s%s\"\n", $1, $2);
                                $$=$1;
                                $$=xmlStrcat($$,$2);
                                xmlFree($2);
                            }
                            | AxisSpecifier NodeTest
                            {
                                TRACE("Got Step: \"%s%s\"\n", $1, $2);
                                $$=$1;
                                $$=xmlStrcat($$,$2);
                                xmlFree($2);
                            }
                            | NodeTest
                            | Attribute
                            | AbbreviatedStep
    ;
    AxisSpecifier           : TOK_NCName TOK_Axis
                            {
                                TRACE("Got AxisSpecifier: \"%s::\"\n", $1);
                                $$=$1;
                                $$=xmlStrcat($$,U("::"));
                            }
    ;
    Attribute               : '@' QName
                            {
                                TRACE("Got Attribute: \"@%s\"\n", $2);
                                $$=xmlStrdup(U("@"));
                                $$=xmlStrcat($$,$2);
                                xmlFree($2);
                            }
                            | '@' '*'
                            {
                                TRACE("Got All attributes pattern: \"@*\"\n");
                                $$=xmlStrdup(U("@*"));
                            }
    ;

    /* [2.3] Node Tests */
    NodeTest                : NameTest
                            | FunctionCall
    ;
    NameTest                : '*'
                            {
                                TRACE("Got NameTest: \"*\"\n");
                                $$=xmlStrdup(U("*"));
                            }
                            | TOK_NCName TOK_Colon '*'
                            {
                                TRACE("Got NameTest: \"%s:*\"\n", $1);
                                $$=$1;
                                $$=xmlStrcat($$,U(":*"));
                            }
                            | TOK_NCName TOK_Colon TOK_NCName
                            { /* PrefixedName */
                                xmlChar const* registeredNsURI = xmlXPathNsLookup(p->ctx, $1);
                                TRACE("Got PrefixedName: \"%s:%s\"\n", $1, $3);

                                if (registeredNsURI)
                                    $$=xmlStrdup(U(""));
                                else
                                    $$=xmlStrdup(NameTest_mod_pre);

                                $$=xmlStrcat($$,$1);
                                xmlFree($1);
                                $$=xmlStrcat($$,U(":"));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);

                                if (!registeredNsURI)
                                    $$=xmlStrcat($$,NameTest_mod_post);
                            }
                            | UnprefixedName
                            {
                                $$=xmlStrdup(NameTest_mod_pre);
                                $$=xmlStrcat($$,$1);
                                xmlFree($1);
                                $$=xmlStrcat($$,NameTest_mod_post);
                            }
    /* [2.4] Predicates */
    Predicates              : Predicates Predicate
                            {
                                $$=$1;
                                $$=xmlStrcat($$,$2);
                                xmlFree($2);
                            }
                            | Predicate
    ;
    Predicate               : '[' PredicateExpr ']'
                            {
                                TRACE("Got Predicate: \"[%s]\"\n", $2);
                                $$=xmlStrdup(U("["));
                                $$=xmlStrcat($$,$2);
                                xmlFree($2);
                                $$=xmlStrcat($$,U("]"));
                            }
    ;
    PredicateExpr           : TOK_Number
                            {
                                $$=xmlStrdup(U("index()="));
                                $$=xmlStrcat($$,$1);
                                xmlFree($1);
                            }
                            | BoolExpr
                            | Attribute
                            | TOK_NCName
    ;
    /* [2.5] Abbreviated Syntax */
    AbbreviatedAbsoluteLocationPath : TOK_DblFSlash RelativeLocationPath
                            {
                                TRACE("Got AbbreviatedAbsoluteLocationPath: \"//%s\"\n", $2);
                                $$=xmlStrdup(U("//"));
                                $$=xmlStrcat($$,$2);
                                xmlFree($2);
                            }
    ;
    AbbreviatedRelativeLocationPath : RelativeLocationPath TOK_DblFSlash Step
                            {
                                TRACE("Got AbbreviatedRelativeLocationPath: \"%s//%s\"\n", $1, $3);
                                $$=$1;
                                $$=xmlStrcat($$,U("//"));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
    ;
    AbbreviatedStep         : TOK_Parent
                            {
                                TRACE("Got AbbreviatedStep: \"..\"\n");
                                $$=xmlStrdup(U(".."));
                            }
                            | TOK_Self
                            {
                                TRACE("Got AbbreviatedStep: \".\"\n");
                                $$=xmlStrdup(U("."));
                            }
    ;

    /* [3] Expressions */
    /* [3.1] Basics */
    Expr                    : OrExpr
    ;
    BoolExpr                : FunctionCall
                            | BoolUnaryExpr
                            | BoolRelationalExpr
                            | BoolEqualityExpr
                            | BoolAndExpr
                            | BoolOrExpr
    ;
    PrimaryExpr             : '(' Expr ')'
                            {
                                TRACE("Got PrimaryExpr: \"(%s)\"\n", $1);
                                $$=xmlStrdup(U("("));
                                $$=xmlStrcat($$,$2);
                                xmlFree($2);
                                $$=xmlStrcat($$,U(")"));
                            }
                            | PathExpr '!' FunctionCall
                            {
                                TRACE("Got PrimaryExpr: \"%s!%s\"\n", $1, $3);
                                $$=$1;
                                $$=xmlStrcat($$,U("/"));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
                            | TOK_Literal
                            | TOK_Number
    ;
    /* [3.2] Function Calls */
    FunctionCall            : QName '(' Arguments ')'
                            {
                                TRACE("Got FunctionCall: \"%s(%s)\"\n", $1, $3);
                                if (xmlStrEqual($1,U("ancestor")))
                                {
                                    $$=$1;
                                    $$=xmlStrcat($$,U("::"));
                                    $$=xmlStrcat($$,$3);
                                    xmlFree($3);
                                }
                                else if (xmlStrEqual($1,U("attribute")))
                                {
                                    if (is_literal($3))
                                    {
                                        $$=xmlStrdup(U("@*[name()="));
                                        xmlFree($1);
                                        $$=xmlStrcat($$,$3);
                                        xmlFree($3);
                                        $$=xmlStrcat($$,U("]"));
                                    }
                                    else
                                    {
                                        /* XML_XPATH_INVALID_TYPE */
                                        $$=xmlStrdup(U("error(1211, 'Error: attribute("));
                                        xmlFree($1);
                                        $$=xmlStrcat($$,$3);
                                        xmlFree($3);
                                        $$=xmlStrcat($$,U("): invalid argument')"));
                                    }
                                }
                                else if (xmlStrEqual($1,U("element")))
                                {
                                    if (is_literal($3))
                                    {
                                        $$=xmlStrdup(U("node()[nodeType()=1][name()="));
                                        xmlFree($1);
                                        $$=xmlStrcat($$,$3);
                                        xmlFree($3);
                                        $$=xmlStrcat($$,U("]"));
                                    }
                                    else
                                    {
                                        /* XML_XPATH_INVALID_TYPE */
                                        $$=xmlStrdup(U("error(1211, 'Error: element("));
                                        xmlFree($1);
                                        $$=xmlStrcat($$,$3);
                                        xmlFree($3);
                                        $$=xmlStrcat($$,U("): invalid argument')"));
                                    }
                                }
                                else
                                {
                                    $$=$1;
                                    $$=xmlStrcat($$,U("("));
                                    $$=xmlStrcat($$,$3);
                                    xmlFree($3);
                                    $$=xmlStrcat($$,U(")"));
                                }
                            }
                            | QName '(' ')'
                            {
                                TRACE("Got FunctionCall: \"%s()\"\n", $1);
                                /* comment() & node() work the same in XPath */
                                if (xmlStrEqual($1,U("attribute")))
                                {
                                    $$=xmlStrdup(U("@*"));
                                    xmlFree($1);
                                }
                                else if (xmlStrEqual($1,U("element")))
                                {
                                    $$=xmlStrdup(U("node()[nodeType()=1]"));
                                    xmlFree($1);
                                }
                                else if (xmlStrEqual($1,U("pi")))
                                {
                                    $$=xmlStrdup(U("processing-instruction()"));
                                    xmlFree($1);
                                }
                                else if (xmlStrEqual($1,U("textnode")))
                                {
                                    $$=xmlStrdup(U("text()"));
                                    xmlFree($1);
                                }
                                else
                                {
                                    $$=$1;
                                    $$=xmlStrcat($$,U("()"));
                                }
                            }
    ;
    Arguments               : Argument ',' Arguments
                            {
                                $$=$1;
                                $$=xmlStrcat($$,U(","));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
                            | Argument
    ;
    Argument                : Expr
    ;
    /* [3.3] Node-sets */
    UnionExpr               : PathExpr
                            | UnionExpr '|' PathExpr
                            {
                                TRACE("Got UnionExpr: \"%s|%s\"\n", $1, $3);
                                $$=$1;
                                $$=xmlStrcat($$,U("|"));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
    ;
    PathExpr                : LocationPath
                            | FilterExpr TOK_FSlash RelativeLocationPath
                            {
                                TRACE("Got PathExpr: \"%s/%s\"\n", $1, $3);
                                $$=$1;
                                $$=xmlStrcat($$,U("/"));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
                            | FilterExpr TOK_DblFSlash RelativeLocationPath
                            {
                                TRACE("Got PathExpr: \"%s//%s\"\n", $1, $3);
                                $$=$1;
                                $$=xmlStrcat($$,U("//"));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
                            | FilterExpr
    ;
    FilterExpr              : PrimaryExpr
                            | FilterExpr Predicate
                            {
                                TRACE("Got FilterExpr: \"%s%s\"\n", $1, $2);
                                $$=$1;
                                $$=xmlStrcat($$,$2);
                                xmlFree($2);
                            }
    ;
    /* [3.4] Booleans */
    OrExpr                  : AndExpr
                            | BoolOrExpr
    ;
    BoolOrExpr              : OrExpr TOK_OpOr AndExpr
                            {
                                TRACE("Got OrExpr: \"%s $or$ %s\"\n", $1, $3);
                                $$=$1;
                                $$=xmlStrcat($$,U(" or "));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
    ;
    AndExpr                 : EqualityExpr
                            | BoolAndExpr
    ;
    BoolAndExpr             : AndExpr TOK_OpAnd EqualityExpr
                            {
                                TRACE("Got AndExpr: \"%s $and$ %s\"\n", $1, $3);
                                $$=$1;
                                $$=xmlStrcat($$,U(" and "));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
    ;
    EqualityExpr            : RelationalExpr
                            | BoolEqualityExpr
    ;
    BoolEqualityExpr        : EqualityExpr TOK_OpEq RelationalExpr
                            {
                                TRACE("Got EqualityExpr: \"%s $eq$ %s\"\n", $1, $3);
                                $$=$1;
                                $$=xmlStrcat($$,U("="));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
                            | EqualityExpr TOK_OpIEq RelationalExpr
                            {
                                TRACE("Got EqualityExpr: \"%s $ieq$ %s\"\n", $1, $3);
                                $$=xmlStrdup(U("OP_IEq("));
                                $$=xmlStrcat($$,$1);
                                xmlFree($1);
                                $$=xmlStrcat($$,U(","));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                                $$=xmlStrcat($$,U(")"));
                            }
                            | EqualityExpr TOK_OpNEq RelationalExpr
                            {
                                TRACE("Got EqualityExpr: \"%s $ne$ %s\"\n", $1, $3);
                                $$=$1;
                                $$=xmlStrcat($$,U("!="));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
                            | EqualityExpr TOK_OpINEq RelationalExpr
                            {
                                TRACE("Got EqualityExpr: \"%s $ine$ %s\"\n", $1, $3);
                                $$=xmlStrdup(U("OP_INEq("));
                                $$=xmlStrcat($$,$1);
                                xmlFree($1);
                                $$=xmlStrcat($$,U(","));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                                $$=xmlStrcat($$,U(")"));
                            }
    ;
    RelationalExpr          : UnaryExpr
                            | BoolRelationalExpr
    ;
    BoolRelationalExpr      : RelationalExpr TOK_OpLt UnaryExpr
                            {
                                TRACE("Got RelationalExpr: \"%s $lt$ %s\"\n", $1, $3);
                                $$=$1;
                                $$=xmlStrcat($$,U("<"));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
                            | RelationalExpr TOK_OpILt UnaryExpr
                            {
                                TRACE("Got RelationalExpr: \"%s $ilt$ %s\"\n", $1, $3);
                                $$=xmlStrdup(U("OP_ILt("));
                                $$=xmlStrcat($$,$1);
                                xmlFree($1);
                                $$=xmlStrcat($$,U(","));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                                $$=xmlStrcat($$,U(")"));
                            }
                            | RelationalExpr TOK_OpGt UnaryExpr
                            {
                                TRACE("Got RelationalExpr: \"%s $gt$ %s\"\n", $1, $3);
                                $$=$1;
                                $$=xmlStrcat($$,U(">"));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
                            | RelationalExpr TOK_OpIGt UnaryExpr
                            {
                                TRACE("Got RelationalExpr: \"%s $igt$ %s\"\n", $1, $3);
                                $$=xmlStrdup(U("OP_IGt("));
                                $$=xmlStrcat($$,$1);
                                xmlFree($1);
                                $$=xmlStrcat($$,U(","));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                                $$=xmlStrcat($$,U(")"));
                            }
                            | RelationalExpr TOK_OpLEq UnaryExpr
                            {
                                TRACE("Got RelationalExpr: \"%s $le$ %s\"\n", $1, $3);
                                $$=$1;
                                $$=xmlStrcat($$,U("<="));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
                            | RelationalExpr TOK_OpILEq UnaryExpr
                            {
                                TRACE("Got RelationalExpr: \"%s $ile$ %s\"\n", $1, $3);
                                $$=xmlStrdup(U("OP_ILEq("));
                                $$=xmlStrcat($$,$1);
                                xmlFree($1);
                                $$=xmlStrcat($$,U(","));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                                $$=xmlStrcat($$,U(")"));
                            }
                            | RelationalExpr TOK_OpGEq UnaryExpr
                            {
                                TRACE("Got RelationalExpr: \"%s $ge$ %s\"\n", $1, $3);
                                $$=$1;
                                $$=xmlStrcat($$,U(">="));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
                            | RelationalExpr TOK_OpIGEq UnaryExpr
                            {
                                TRACE("Got RelationalExpr: \"%s $ige$ %s\"\n", $1, $3);
                                $$=xmlStrdup(U("OP_IGEq("));
                                $$=xmlStrcat($$,$1);
                                xmlFree($1);
                                $$=xmlStrcat($$,U(","));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                                $$=xmlStrcat($$,U(")"));
                            }
    ;

    /* [3.5] Numbers */
    UnaryExpr               : UnionExpr
                            | BoolUnaryExpr
    ;
    BoolUnaryExpr           : TOK_OpNot UnaryExpr
                            {
                                TRACE("Got UnaryExpr: \"$not$ %s\"\n", $2);
                                $$=xmlStrdup(U(" not("));
                                $$=xmlStrcat($$,$2);
                                xmlFree($2);
                                $$=xmlStrcat($$,U(")"));
                            }
                            | TOK_OpAny Expr
                            {
                                TRACE("Got UnaryExpr: \"$any$ %s\"\n", $2);
                                $$=xmlStrdup(U("boolean("));
                                $$=xmlStrcat($$,$2);
                                xmlFree($2);
                                $$=xmlStrcat($$,U(")"));
                            }
                            | TOK_OpAll AllExpr
                            {
                                TRACE("Got UnaryExpr: \"$all$ %s\"\n", $2);
                                $$=xmlStrdup(U("not("));
                                $$=xmlStrcat($$,$2);
                                xmlFree($2);
                                $$=xmlStrcat($$,U(")"));
                            }
                            | TOK_OpAll
                            {
                                FIXME("Unrecognized $all$ expression - ignoring\n");
                                $$=xmlStrdup(U(""));
                            }
    ;
    AllExpr                 : PathExpr TOK_OpEq PathExpr
                            {
                                $$=$1;
                                $$=xmlStrcat($$,U("!="));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
                            | PathExpr TOK_OpNEq PathExpr
                            {
                                $$=$1;
                                $$=xmlStrcat($$,U("="));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
                            | PathExpr TOK_OpLt PathExpr
                            {
                                $$=$1;
                                $$=xmlStrcat($$,U(">="));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
                            | PathExpr TOK_OpLEq PathExpr
                            {
                                $$=$1;
                                $$=xmlStrcat($$,U(">"));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
                            | PathExpr TOK_OpGt PathExpr
                            {
                                $$=$1;
                                $$=xmlStrcat($$,U("<="));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
                            | PathExpr TOK_OpGEq PathExpr
                            {
                                $$=$1;
                                $$=xmlStrcat($$,U("<"));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                            }
                            | PathExpr TOK_OpIEq PathExpr
                            {
                                $$=xmlStrdup(U("OP_INEq("));
                                $$=xmlStrcat($$,$1);
                                xmlFree($1);
                                $$=xmlStrcat($$,U(","));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                                $$=xmlStrcat($$,U(")"));
                            }
                            | PathExpr TOK_OpINEq PathExpr
                            {
                                $$=xmlStrdup(U("OP_IEq("));
                                $$=xmlStrcat($$,$1);
                                xmlFree($1);
                                $$=xmlStrcat($$,U(","));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                                $$=xmlStrcat($$,U(")"));
                            }
                            | PathExpr TOK_OpILt PathExpr
                            {
                                $$=xmlStrdup(U("OP_IGEq("));
                                $$=xmlStrcat($$,$1);
                                xmlFree($1);
                                $$=xmlStrcat($$,U(","));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                                $$=xmlStrcat($$,U(")"));
                            }
                            | PathExpr TOK_OpILEq PathExpr
                            {
                                $$=xmlStrdup(U("OP_IGt("));
                                $$=xmlStrcat($$,$1);
                                xmlFree($1);
                                $$=xmlStrcat($$,U(","));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                                $$=xmlStrcat($$,U(")"));
                            }
                            | PathExpr TOK_OpIGt PathExpr
                            {
                                $$=xmlStrdup(U("OP_ILEq("));
                                $$=xmlStrcat($$,$1);
                                xmlFree($1);
                                $$=xmlStrcat($$,U(","));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                                $$=xmlStrcat($$,U(")"));
                            }
                            | PathExpr TOK_OpIGEq PathExpr
                            {
                                $$=xmlStrdup(U("OP_ILt("));
                                $$=xmlStrcat($$,$1);
                                xmlFree($1);
                                $$=xmlStrcat($$,U(","));
                                $$=xmlStrcat($$,$3);
                                xmlFree($3);
                                $$=xmlStrcat($$,U(")"));
                            }
    ;

%%

#endif  /* HAVE_LIBXML2 */
