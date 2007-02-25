/*
 * Copyright 1998 Bertho A. Stultiens (BS)
 *
 * Shared things between parser.l and parser.y and some others
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WRC_PARSER_H
#define __WRC_PARSER_H

/* From parser.y */
extern int yydebug;
extern int want_nl;		/* Set when getting line-numers */
extern int want_id;		/* Set when getting the resource name */

int yyparse(void);

/* From parser.l */
extern FILE *yyin;
extern char *yytext;
extern int yy_flex_debug;

int yylex(void);

#endif

