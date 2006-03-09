/*
 * Main definitions and externals
 *
 * Copyright 2000 Bertho A. Stultiens (BS)
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

#ifndef __WMC_WMC_H
#define __WMC_WMC_H

#ifndef __WMC_WMCTYPES_H
#include "wmctypes.h"
#endif

#include <time.h>	/* For time_t */

#define WMC_MAJOR_VERSION	1
#define WMC_MINOR_VERSION	0
#define WMC_MICRO_VERSION	0
#define WMC_RELEASEDATE		"(12-Jun-2000)"

#define WMC_STRINGIZE(a)	#a
#define WMC_VERSIONIZE(a,b,c)	WMC_STRINGIZE(a) "." WMC_STRINGIZE(b) "." WMC_STRINGIZE(c)  
#define WMC_VERSION		WMC_VERSIONIZE(WMC_MAJOR_VERSION, WMC_MINOR_VERSION, WMC_MICRO_VERSION)
#define WMC_FULLVERSION 	WMC_VERSION " " WMC_RELEASEDATE

/*
 * The default codepage setting is only to
 * read and convert input which is non-message
 * text. It doesn't really matter that much because
 * all codepages map 0x00-0x7f to 0x0000-0x007f from
 * char to unicode and all non-message text should
 * be plain ASCII.
 * However, we do implement iso-8859-1 for 1-to-1
 * mapping for all other chars, so this is very close
 * to what we really want.
 */
#define WMC_DEFAULT_CODEPAGE	28591

extern int pedantic;
extern int leave_case;
extern int byteorder;
extern int decimal;
extern int custombit;
extern int unicodein;
extern int unicodeout;
extern int rcinline;

extern char *output_name;
extern char *input_name;
extern char *header_name;
extern char *cmdline;			
extern time_t now;

extern int line_number;
extern int char_number;

int yyparse(void);
extern int yydebug;
extern int want_nl;
extern int want_line;
extern int want_file;
extern node_t *nodehead;
extern lan_blk_t *lanblockhead;

int yylex(void);
FILE *yyin;
void set_codepage(int cp);

void add_token(tok_e type, const WCHAR *name, int tok, int cp, const WCHAR *alias, int fix);
token_t *lookup_token(const WCHAR *s);
void get_tokentable(token_t **tab, int *len);


#ifdef __GNUC__
#define _alloca alloca
//#define alloca __builtin_alloca
#else /* not GNU C.  */
//#define alloca __builtin_alloca

#pragma warning (disable:4305) // 'initializing' : truncation from 'const int ' to 'const char '

#endif /*__GNUC__*/

#endif
