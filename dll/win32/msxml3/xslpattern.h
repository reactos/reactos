/*
 *    XSLPattern lexer/parser shared internals
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

#ifndef __XSLPATTERN__
#define __XSLPATTERN__

#include <stdlib.h>
#include <libxml/tree.h>
#include <libxml/xmlstring.h>
#include <libxml/xpath.h>

#include "windef.h"
#include "winnt.h"

typedef struct _parser_param {
    void* yyscanner;
    xmlXPathContextPtr ctx;
    xmlChar const* in;
    int pos;
    int len;
    xmlChar* out;
    int err;
} parser_param;

#define XSLPATTERN_STYPE xmlChar*
#define YY_EXTRA_TYPE parser_param*

int  xslpattern_lex(xmlChar**, void*);
int  xslpattern_lex_init(void**);
int  xslpattern_lex_destroy(void*);
void xslpattern_set_extra(parser_param*, void*);
int  xslpattern_parse(parser_param*, void*);


#endif /* __XSLPATTERN__ */
