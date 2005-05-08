/* A Bison parser, made from ./parser.y
   by GNU bison 1.35.  */

#define YYBISON 1  /* Identify Bison output.  */

# define	tNL	257
# define	tNUMBER	258
# define	tLNUMBER	259
# define	tSTRING	260
# define	tIDENT	261
# define	tFILENAME	262
# define	tRAWDATA	263
# define	tACCELERATORS	264
# define	tBITMAP	265
# define	tCURSOR	266
# define	tDIALOG	267
# define	tDIALOGEX	268
# define	tMENU	269
# define	tMENUEX	270
# define	tMESSAGETABLE	271
# define	tRCDATA	272
# define	tVERSIONINFO	273
# define	tSTRINGTABLE	274
# define	tFONT	275
# define	tFONTDIR	276
# define	tICON	277
# define	tAUTO3STATE	278
# define	tAUTOCHECKBOX	279
# define	tAUTORADIOBUTTON	280
# define	tCHECKBOX	281
# define	tDEFPUSHBUTTON	282
# define	tPUSHBUTTON	283
# define	tRADIOBUTTON	284
# define	tSTATE3	285
# define	tGROUPBOX	286
# define	tCOMBOBOX	287
# define	tLISTBOX	288
# define	tSCROLLBAR	289
# define	tCONTROL	290
# define	tEDITTEXT	291
# define	tRTEXT	292
# define	tCTEXT	293
# define	tLTEXT	294
# define	tBLOCK	295
# define	tVALUE	296
# define	tSHIFT	297
# define	tALT	298
# define	tASCII	299
# define	tVIRTKEY	300
# define	tGRAYED	301
# define	tCHECKED	302
# define	tINACTIVE	303
# define	tNOINVERT	304
# define	tPURE	305
# define	tIMPURE	306
# define	tDISCARDABLE	307
# define	tLOADONCALL	308
# define	tPRELOAD	309
# define	tFIXED	310
# define	tMOVEABLE	311
# define	tCLASS	312
# define	tCAPTION	313
# define	tCHARACTERISTICS	314
# define	tEXSTYLE	315
# define	tSTYLE	316
# define	tVERSION	317
# define	tLANGUAGE	318
# define	tFILEVERSION	319
# define	tPRODUCTVERSION	320
# define	tFILEFLAGSMASK	321
# define	tFILEOS	322
# define	tFILETYPE	323
# define	tFILEFLAGS	324
# define	tFILESUBTYPE	325
# define	tMENUBARBREAK	326
# define	tMENUBREAK	327
# define	tMENUITEM	328
# define	tPOPUP	329
# define	tSEPARATOR	330
# define	tHELP	331
# define	tTOOLBAR	332
# define	tBUTTON	333
# define	tBEGIN	334
# define	tEND	335
# define	tDLGINIT	336
# define	tNOT	337
# define	pUPM	338

#line 1 "./parser.y"

/*
 * Copyright 1994	Martin von Loewis
 * Copyright 1998-2000	Bertho A. Stultiens (BS)
 *           1999	Juergen Schmied (JS)
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
 *
 * History:
 * 24-Jul-2000 BS	- Made a fix for broken Berkeley yacc on
 *			  non-terminals (see cjunk rule).
 * 21-May-2000 BS	- Partial implementation of font resources.
 *			- Corrected language propagation for binary
 *			  resources such as bitmaps, icons, cursors,
 *			  userres and rcdata. The language is now
 *			  correct in .res files.
 *			- Fixed reading the resource name as ident,
 *			  so that it may overlap keywords.
 * 20-May-2000 BS	- Implemented animated cursors and icons
 *			  resource types.
 * 30-Apr-2000 BS	- Reintegration into the wine-tree
 * 14-Jan-2000 BS	- Redid the usertype resources so that they
 *			  are compatible.
 * 02-Jan-2000 BS	- Removed the preprocessor from the grammar
 *			  except for the # command (line numbers).
 *
 * 06-Nov-1999 JS	- see CHANGES
 *
 * 29-Dec-1998 AdH	- Grammar and function extensions.
 *			     grammar: TOOLBAR resources, Named ICONs in
 *				DIALOGS
 *			     functions: semantic actions for the grammar
 *				changes, resource files can now be anywhere
 *				on the include path instead of just in the
 *				current directory
 *
 * 20-Jun-1998 BS	- Fixed a bug in load_file() where the name was not
 *			  printed out correctly.
 *
 * 17-Jun-1998 BS	- Fixed a bug in CLASS statement parsing which should
 *			  also accept a tSTRING as argument.
 *
 * 25-May-1998 BS	- Found out that I need to support language, version
 *			  and characteristics in inline resources (bitmap,
 *			  cursor, etc) but they can also be specified with
 *			  a filename. This renders my filename-scanning scheme
 *			  worthless. Need to build newline parsing to solve
 *			  this one.
 *			  It will come with version 1.1.0 (sigh).
 *
 * 19-May-1998 BS	- Started to build a builtin preprocessor
 *
 * 30-Apr-1998 BS	- Redid the stringtable parsing/handling. My previous
 *			  ideas had some serious flaws.
 *
 * 27-Apr-1998 BS	- Removed a lot of dead comments and put it in a doc
 *			  file.
 *
 * 21-Apr-1998 BS	- Added correct behavior for cursors and icons.
 *			- This file is growing too big. It is time to strip
 *			  things and put it in a support file.
 *
 * 19-Apr-1998 BS	- Tagged the stringtable resource so that only one
 *			  resource will be created. This because the table
 *			  has a different layout than other resources. The
 *			  table has to be sorted, and divided into smaller
 *			  resource entries (see comment in source).
 *
 * 17-Apr-1998 BS	- Almost all strings, including identifiers, are parsed
 *			  as string_t which include unicode strings upon
 *			  input.
 *			- Parser now emits a warning when compiling win32
 *			  extensions in win16 mode.
 *
 * 16-Apr-1998 BS	- Raw data elements are now *optionally* separated
 *			  by commas. Read the comments in file sq2dq.l.
 *			- FIXME: there are instances in the source that rely
 *			  on the fact that int==32bit and pointers are int size.
 *			- Fixed the conflict in menuex by changing a rule
 *			  back into right recursion. See note in source.
 *			- UserType resources cannot have an expression as its
 *			  typeclass. See note in source.
 *
 * 15-Apr-1998 BS	- Changed all right recursion into left recursion to
 *			  get reduction of the parsestack.
 *			  This also helps communication between bison and flex.
 *			  Main advantage is that the Empty rule gets reduced
 *			  first, which is used to allocate/link things.
 *			  It also added a shift/reduce conflict in the menuex
 *			  handling, due to expression/option possibility,
 *			  although not serious.
 *
 * 14-Apr-1998 BS	- Redone almost the entire parser. We're not talking
 *			  about making it more efficient, but readable (for me)
 *			  and slightly easier to expand/change.
 *			  This is done primarily by using more reduce states
 *			  with many (intuitive) types for the various resource
 *			  statements.
 *			- Added expression handling for all resources where a
 *			  number is accepted (not only for win32). Also added
 *			  multiply and division (not MS compatible, but handy).
 *			  Unary minus introduced a shift/reduce conflict, but
 *			  it is not serious.
 *
 * 13-Apr-1998 BS	- Reordered a lot of things
 *			- Made the source more readable
 *			- Added Win32 resource definitions
 *			- Corrected syntax problems with an old yacc (;)
 *			- Added extra comment about grammar
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include "wrc.h"
#include "utils.h"
#include "newstruc.h"
#include "dumpres.h"
#include "wine/wpp.h"
#include "wine/unicode.h"
#include "parser.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#if defined(YYBYACC)
	/* Berkeley yacc (byacc) doesn't seem to know about these */
	/* Some *BSD supplied versions do define these though */
# ifndef YYEMPTY
#  define YYEMPTY	(-1)	/* Empty lookahead value of yychar */
# endif
# ifndef YYLEX
#  define YYLEX		yylex()
# endif

#elif defined(YYBISON)
	/* Bison was used for original development */
	/* #define YYEMPTY -2 */
	/* #define YYLEX   yylex() */

#else
	/* No yacc we know yet */
# if !defined(YYEMPTY) || !defined(YYLEX)
#  error Yacc version/type unknown. This version needs to be verified for settings of YYEMPTY and YYLEX.
# elif defined(__GNUC__)	/* gcc defines the #warning directive */
#  warning Yacc version/type unknown. It defines YYEMPTY and YYLEX, but is not tested
  /* #else we just take a chance that it works... */
# endif
#endif

int want_nl = 0;	/* Signal flex that we need the next newline */
int want_id = 0;	/* Signal flex that we need the next identifier */
stringtable_t *tagstt;	/* Stringtable tag.
			 * It is set while parsing a stringtable to one of
			 * the stringtables in the sttres list or a new one
			 * if the language was not parsed before.
			 */
stringtable_t *sttres;	/* Stringtable resources. This holds the list of
			 * stringtables with different lanuages
			 */
static int dont_want_id = 0;	/* See language parsing for details */

/* Set to the current options of the currently scanning stringtable */
static int *tagstt_memopt;
static characts_t *tagstt_characts;
static version_t *tagstt_version;

static const char riff[4] = "RIFF";	/* RIFF file magic for animated cursor/icon */

/* Prototypes of here defined functions */
static event_t *get_event_head(event_t *p);
static control_t *get_control_head(control_t *p);
static ver_value_t *get_ver_value_head(ver_value_t *p);
static ver_block_t *get_ver_block_head(ver_block_t *p);
static resource_t *get_resource_head(resource_t *p);
static menuex_item_t *get_itemex_head(menuex_item_t *p);
static menu_item_t *get_item_head(menu_item_t *p);
static raw_data_t *merge_raw_data_str(raw_data_t *r1, string_t *str);
static raw_data_t *merge_raw_data_int(raw_data_t *r1, int i);
static raw_data_t *merge_raw_data_long(raw_data_t *r1, int i);
static raw_data_t *merge_raw_data(raw_data_t *r1, raw_data_t *r2);
static raw_data_t *str2raw_data(string_t *str);
static raw_data_t *int2raw_data(int i);
static raw_data_t *long2raw_data(int i);
static raw_data_t *load_file(string_t *name, language_t *lang);
static itemex_opt_t *new_itemex_opt(int id, int type, int state, int helpid);
static event_t *add_string_event(string_t *key, int id, int flags, event_t *prev);
static event_t *add_event(int key, int id, int flags, event_t *prev);
static dialogex_t *dialogex_version(version_t *v, dialogex_t *dlg);
static dialogex_t *dialogex_characteristics(characts_t *c, dialogex_t *dlg);
static dialogex_t *dialogex_language(language_t *l, dialogex_t *dlg);
static dialogex_t *dialogex_menu(name_id_t *m, dialogex_t *dlg);
static dialogex_t *dialogex_class(name_id_t *n, dialogex_t *dlg);
static dialogex_t *dialogex_font(font_id_t *f, dialogex_t *dlg);
static dialogex_t *dialogex_caption(string_t *s, dialogex_t *dlg);
static dialogex_t *dialogex_exstyle(style_t *st, dialogex_t *dlg);
static dialogex_t *dialogex_style(style_t *st, dialogex_t *dlg);
static name_id_t *convert_ctlclass(name_id_t *cls);
static control_t *ins_ctrl(int type, int special_style, control_t *ctrl, control_t *prev);
static dialog_t *dialog_version(version_t *v, dialog_t *dlg);
static dialog_t *dialog_characteristics(characts_t *c, dialog_t *dlg);
static dialog_t *dialog_language(language_t *l, dialog_t *dlg);
static dialog_t *dialog_menu(name_id_t *m, dialog_t *dlg);
static dialog_t *dialog_class(name_id_t *n, dialog_t *dlg);
static dialog_t *dialog_font(font_id_t *f, dialog_t *dlg);
static dialog_t *dialog_caption(string_t *s, dialog_t *dlg);
static dialog_t *dialog_exstyle(style_t * st, dialog_t *dlg);
static dialog_t *dialog_style(style_t * st, dialog_t *dlg);
static resource_t *build_stt_resources(stringtable_t *stthead);
static stringtable_t *find_stringtable(lvc_t *lvc);
static toolbar_item_t *ins_tlbr_button(toolbar_item_t *prev, toolbar_item_t *idrec);
static toolbar_item_t *get_tlbr_buttons_head(toolbar_item_t *p, int *nitems);
static string_t *make_filename(string_t *s);
static resource_t *build_fontdirs(resource_t *tail);
static resource_t *build_fontdir(resource_t **fnt, int nfnt);
static int rsrcid_to_token(int lookahead);


#line 240 "./parser.y"
#ifndef YYSTYPE
typedef union{
	string_t	*str;
	int		num;
	int		*iptr;
	char		*cptr;
	resource_t	*res;
	accelerator_t	*acc;
	bitmap_t	*bmp;
	dialog_t	*dlg;
	dialogex_t	*dlgex;
	font_t		*fnt;
	fontdir_t	*fnd;
	menu_t		*men;
	menuex_t	*menex;
	rcdata_t	*rdt;
	stringtable_t	*stt;
	stt_entry_t	*stte;
	user_t		*usr;
	messagetable_t	*msg;
	versioninfo_t	*veri;
	control_t	*ctl;
	name_id_t	*nid;
	font_id_t	*fntid;
	language_t	*lan;
	version_t	*ver;
	characts_t	*chars;
	event_t		*event;
	menu_item_t	*menitm;
	menuex_item_t	*menexitm;
	itemex_opt_t	*exopt;
	raw_data_t	*raw;
	lvc_t		*lvc;
	ver_value_t	*val;
	ver_block_t	*blk;
	ver_words_t	*verw;
	toolbar_t	*tlbar;
	toolbar_item_t	*tlbarItems;
	dlginit_t       *dginit;
	style_pair_t	*styles;
	style_t		*style;
	ani_any_t	*ani;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
#ifndef YYDEBUG
# define YYDEBUG 1
#endif



#define	YYFINAL		568
#define	YYFLAG		-32768
#define	YYNTBASE	96

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 338 ? yytranslate[x] : 177)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    85,     2,
      94,    95,    88,    86,    93,    87,     2,    89,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    84,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    83,     2,    90,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    91,    92
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     2,     3,     6,     9,    13,    17,    19,    20,
      26,    27,    29,    31,    33,    35,    37,    39,    41,    43,
      45,    47,    49,    51,    53,    55,    57,    59,    61,    63,
      65,    67,    69,    71,    73,    77,    81,    85,    89,    93,
      97,   101,   105,   109,   111,   113,   120,   121,   127,   133,
     134,   137,   139,   143,   145,   147,   149,   151,   153,   155,
     169,   170,   174,   178,   182,   185,   189,   193,   196,   199,
     202,   203,   207,   211,   215,   219,   223,   227,   231,   235,
     239,   243,   247,   251,   255,   259,   263,   267,   271,   282,
     295,   306,   307,   312,   319,   328,   346,   362,   367,   368,
     371,   376,   380,   384,   386,   389,   391,   393,   408,   409,
     413,   417,   421,   424,   427,   431,   435,   438,   441,   444,
     445,   449,   453,   457,   461,   465,   469,   473,   477,   481,
     485,   489,   493,   497,   501,   505,   509,   513,   524,   544,
     561,   576,   589,   590,   592,   593,   596,   606,   607,   610,
     615,   619,   620,   627,   631,   637,   638,   642,   646,   650,
     654,   658,   662,   667,   671,   672,   677,   681,   687,   688,
     691,   697,   704,   705,   708,   713,   720,   729,   734,   738,
     739,   744,   745,   747,   754,   755,   765,   775,   779,   783,
     787,   791,   795,   796,   799,   805,   806,   809,   811,   816,
     821,   823,   827,   837,   838,   842,   845,   846,   849,   852,
     854,   856,   858,   860,   862,   864,   866,   867,   870,   873,
     876,   881,   884,   887,   892,   894,   896,   899,   901,   904,
     906,   910,   914,   919,   923,   928,   932,   934,   936,   937,
     939,   941,   945,   949,   953,   957,   961,   965,   969,   972,
     975,   978,   982,   984,   987,   989
};
static const short yyrhs[] =
{
      97,     0,     0,    97,    98,     0,    97,     3,     0,   174,
     100,   103,     0,     7,   100,   103,     0,   150,     0,     0,
      64,    99,   174,    93,   174,     0,     0,   174,     0,     7,
       0,   101,     0,     6,     0,   115,     0,   105,     0,   106,
       0,   120,     0,   131,     0,   112,     0,   108,     0,   109,
       0,   107,     0,   141,     0,   145,     0,   110,     0,   111,
       0,   161,     0,   113,     0,   154,     0,     8,     0,     7,
       0,     6,     0,    11,   163,   172,     0,    12,   163,   172,
       0,    23,   163,   172,     0,    21,   163,   172,     0,    22,
     163,   172,     0,    17,   163,   172,     0,    18,   163,   172,
       0,    82,   163,   172,     0,   114,   163,   172,     0,     4,
       0,     7,     0,    10,   163,   166,    80,   116,    81,     0,
       0,   116,     6,    93,   174,   117,     0,   116,   174,    93,
     174,   117,     0,     0,    93,   118,     0,   119,     0,   118,
      93,   119,     0,    50,     0,    43,     0,    36,     0,    44,
       0,    45,     0,    46,     0,    13,   163,   174,    93,   174,
      93,   174,    93,   174,   121,    80,   122,    81,     0,     0,
     121,    62,   129,     0,   121,    61,   129,     0,   121,    59,
       6,     0,   121,   127,     0,   121,    58,   102,     0,   121,
      15,   101,     0,   121,   167,     0,   121,   168,     0,   121,
     169,     0,     0,   122,    36,   126,     0,   122,    37,   124,
       0,   122,    34,   124,     0,   122,    33,   124,     0,   122,
      35,   124,     0,   122,    27,   123,     0,   122,    28,   123,
       0,   122,    32,   123,     0,   122,    29,   123,     0,   122,
      30,   123,     0,   122,    24,   123,     0,   122,    31,   123,
       0,   122,    25,   123,     0,   122,    26,   123,     0,   122,
      40,   123,     0,   122,    39,   123,     0,   122,    38,   123,
       0,   122,    23,   102,   153,   174,    93,   174,    93,   174,
     125,     0,     6,   153,   174,    93,   174,    93,   174,    93,
     174,    93,   174,   128,     0,   174,    93,   174,    93,   174,
      93,   174,    93,   174,   128,     0,     0,    93,   174,    93,
     174,     0,    93,   174,    93,   174,    93,   129,     0,    93,
     174,    93,   174,    93,   129,    93,   129,     0,   102,   153,
     174,    93,   130,    93,   129,    93,   174,    93,   174,    93,
     174,    93,   174,    93,   129,     0,   102,   153,   174,    93,
     130,    93,   129,    93,   174,    93,   174,    93,   174,    93,
     174,     0,    21,   174,    93,     6,     0,     0,    93,   129,
       0,    93,   129,    93,   129,     0,   129,    83,   129,     0,
      94,   129,    95,     0,   176,     0,    91,   176,     0,   174,
       0,     6,     0,    14,   163,   174,    93,   174,    93,   174,
      93,   174,   138,   132,    80,   133,    81,     0,     0,   132,
      62,   129,     0,   132,    61,   129,     0,   132,    59,     6,
       0,   132,   127,     0,   132,   139,     0,   132,    58,   102,
       0,   132,    15,   101,     0,   132,   167,     0,   132,   168,
       0,   132,   169,     0,     0,   133,    36,   134,     0,   133,
      37,   136,     0,   133,    34,   136,     0,   133,    33,   136,
       0,   133,    35,   136,     0,   133,    27,   135,     0,   133,
      28,   135,     0,   133,    32,   135,     0,   133,    29,   135,
       0,   133,    30,   135,     0,   133,    24,   135,     0,   133,
      31,   135,     0,   133,    25,   135,     0,   133,    26,   135,
       0,   133,    40,   135,     0,   133,    39,   135,     0,   133,
      38,   135,     0,   133,    23,   102,   153,   174,    93,   174,
      93,   174,   125,     0,   102,   153,   174,    93,   130,    93,
     129,    93,   174,    93,   174,    93,   174,    93,   174,    93,
     129,   138,   137,     0,   102,   153,   174,    93,   130,    93,
     129,    93,   174,    93,   174,    93,   174,    93,   174,   137,
       0,     6,   153,   174,    93,   174,    93,   174,    93,   174,
      93,   174,   128,   138,   137,     0,   174,    93,   174,    93,
     174,    93,   174,    93,   174,   128,   138,   137,     0,     0,
     170,     0,     0,    93,   174,     0,    21,   174,    93,     6,
      93,   174,    93,   174,   140,     0,     0,    93,   174,     0,
      15,   163,   166,   142,     0,    80,   143,    81,     0,     0,
     143,    74,     6,   153,   174,   144,     0,   143,    74,    76,
       0,   143,    75,     6,   144,   142,     0,     0,   153,    48,
     144,     0,   153,    47,   144,     0,   153,    77,   144,     0,
     153,    49,   144,     0,   153,    72,   144,     0,   153,    73,
     144,     0,    16,   163,   166,   146,     0,    80,   147,    81,
       0,     0,   147,    74,     6,   148,     0,   147,    74,    76,
       0,   147,    75,     6,   149,   146,     0,     0,    93,   174,
       0,    93,   173,    93,   173,   144,     0,    93,   173,    93,
     173,    93,   174,     0,     0,    93,   174,     0,    93,   173,
      93,   174,     0,    93,   173,    93,   173,    93,   174,     0,
      93,   173,    93,   173,    93,   173,    93,   174,     0,   151,
      80,   152,    81,     0,    20,   163,   166,     0,     0,   152,
     174,   153,     6,     0,     0,    93,     0,    19,   163,   155,
      80,   156,    81,     0,     0,   155,    65,   174,    93,   174,
      93,   174,    93,   174,     0,   155,    66,   174,    93,   174,
      93,   174,    93,   174,     0,   155,    70,   174,     0,   155,
      67,   174,     0,   155,    68,   174,     0,   155,    69,   174,
       0,   155,    71,   174,     0,     0,   156,   157,     0,    41,
       6,    80,   158,    81,     0,     0,   158,   159,     0,   157,
       0,    42,     6,    93,     6,     0,    42,     6,    93,   160,
       0,   174,     0,   160,    93,   174,     0,    78,   163,   174,
      93,   174,   166,    80,   162,    81,     0,     0,   162,    79,
     174,     0,   162,    76,     0,     0,   163,   164,     0,   163,
     165,     0,    55,     0,    57,     0,    53,     0,    51,     0,
      54,     0,    56,     0,    52,     0,     0,   166,   167,     0,
     166,   168,     0,   166,   169,     0,    64,   174,    93,   174,
       0,    60,   174,     0,    63,   174,     0,   166,    80,   171,
      81,     0,     9,     0,     4,     0,    87,     4,     0,     5,
       0,    87,     5,     0,     6,     0,   171,   153,     9,     0,
     171,   153,     4,     0,   171,   153,    87,     4,     0,   171,
     153,     5,     0,   171,   153,    87,     5,     0,   171,   153,
       6,     0,   104,     0,   170,     0,     0,   174,     0,   175,
       0,   175,    86,   175,     0,   175,    87,   175,     0,   175,
      83,   175,     0,   175,    85,   175,     0,   175,    88,   175,
       0,   175,    89,   175,     0,   175,    84,   175,     0,    90,
     175,     0,    87,   175,     0,    86,   175,     0,    94,   175,
      95,     0,   176,     0,    91,   176,     0,     4,     0,     5,
       0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] =
{
       0,   361,   395,   396,   466,   472,   484,   494,   502,   502,
     546,   552,   559,   569,   570,   579,   580,   581,   605,   606,
     612,   613,   614,   615,   639,   640,   646,   647,   648,   649,
     650,   654,   655,   656,   660,   664,   680,   702,   712,   720,
     728,   732,   736,   747,   752,   761,   785,   786,   787,   796,
     797,   800,   801,   804,   805,   806,   807,   808,   809,   814,
     849,   850,   851,   852,   853,   854,   855,   856,   857,   858,
     861,   862,   863,   864,   865,   866,   867,   868,   869,   870,
     872,   873,   874,   875,   876,   877,   878,   879,   881,   891,
     916,   937,   940,   945,   952,   963,   977,   992,   997,   998,
     999,  1003,  1004,  1005,  1006,  1010,  1015,  1023,  1067,  1068,
    1069,  1070,  1071,  1072,  1073,  1074,  1075,  1076,  1077,  1080,
    1081,  1082,  1083,  1084,  1085,  1086,  1087,  1088,  1089,  1091,
    1092,  1093,  1094,  1095,  1096,  1097,  1098,  1100,  1110,  1135,
    1151,  1179,  1202,  1203,  1206,  1207,  1211,  1218,  1219,  1223,
    1246,  1250,  1251,  1260,  1266,  1285,  1286,  1287,  1288,  1289,
    1290,  1291,  1295,  1320,  1324,  1325,  1341,  1347,  1367,  1368,
    1372,  1380,  1391,  1392,  1396,  1402,  1410,  1430,  1471,  1482,
    1483,  1517,  1518,  1523,  1539,  1540,  1550,  1560,  1567,  1574,
    1581,  1588,  1598,  1599,  1608,  1616,  1617,  1626,  1631,  1637,
    1646,  1647,  1651,  1677,  1678,  1683,  1692,  1693,  1703,  1718,
    1719,  1720,  1721,  1724,  1725,  1726,  1730,  1731,  1739,  1747,
    1765,  1772,  1776,  1780,  1795,  1796,  1797,  1798,  1799,  1800,
    1801,  1802,  1803,  1804,  1805,  1806,  1810,  1811,  1818,  1819,
    1823,  1826,  1827,  1828,  1829,  1830,  1831,  1832,  1833,  1834,
    1835,  1836,  1837,  1838,  1841,  1842
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] =
{
  "$", "error", "$undefined.", "tNL", "tNUMBER", "tLNUMBER", "tSTRING", 
  "tIDENT", "tFILENAME", "tRAWDATA", "tACCELERATORS", "tBITMAP", 
  "tCURSOR", "tDIALOG", "tDIALOGEX", "tMENU", "tMENUEX", "tMESSAGETABLE", 
  "tRCDATA", "tVERSIONINFO", "tSTRINGTABLE", "tFONT", "tFONTDIR", "tICON", 
  "tAUTO3STATE", "tAUTOCHECKBOX", "tAUTORADIOBUTTON", "tCHECKBOX", 
  "tDEFPUSHBUTTON", "tPUSHBUTTON", "tRADIOBUTTON", "tSTATE3", "tGROUPBOX", 
  "tCOMBOBOX", "tLISTBOX", "tSCROLLBAR", "tCONTROL", "tEDITTEXT", 
  "tRTEXT", "tCTEXT", "tLTEXT", "tBLOCK", "tVALUE", "tSHIFT", "tALT", 
  "tASCII", "tVIRTKEY", "tGRAYED", "tCHECKED", "tINACTIVE", "tNOINVERT", 
  "tPURE", "tIMPURE", "tDISCARDABLE", "tLOADONCALL", "tPRELOAD", "tFIXED", 
  "tMOVEABLE", "tCLASS", "tCAPTION", "tCHARACTERISTICS", "tEXSTYLE", 
  "tSTYLE", "tVERSION", "tLANGUAGE", "tFILEVERSION", "tPRODUCTVERSION", 
  "tFILEFLAGSMASK", "tFILEOS", "tFILETYPE", "tFILEFLAGS", "tFILESUBTYPE", 
  "tMENUBARBREAK", "tMENUBREAK", "tMENUITEM", "tPOPUP", "tSEPARATOR", 
  "tHELP", "tTOOLBAR", "tBUTTON", "tBEGIN", "tEND", "tDLGINIT", "'|'", 
  "'^'", "'&'", "'+'", "'-'", "'*'", "'/'", "'~'", "tNOT", "pUPM", "','", 
  "'('", "')'", "resource_file", "resources", "resource", "@1", "usrcvt", 
  "nameid", "nameid_s", "resource_definition", "filename", "bitmap", 
  "cursor", "icon", "font", "fontdir", "messagetable", "rcdata", 
  "dlginit", "userres", "usertype", "accelerators", "events", "acc_opt", 
  "accs", "acc", "dialog", "dlg_attributes", "ctrls", "lab_ctrl", 
  "ctrl_desc", "iconinfo", "gen_ctrl", "opt_font", "optional_style_pair", 
  "style", "ctlclass", "dialogex", "dlgex_attribs", "exctrls", 
  "gen_exctrl", "lab_exctrl", "exctrl_desc", "opt_data", "helpid", 
  "opt_exfont", "opt_expr", "menu", "menu_body", "item_definitions", 
  "item_options", "menuex", "menuex_body", "itemex_definitions", 
  "itemex_options", "itemex_p_options", "stringtable", "stt_head", 
  "strings", "opt_comma", "versioninfo", "fix_version", "ver_blocks", 
  "ver_block", "ver_values", "ver_value", "ver_words", "toolbar", 
  "toolbar_items", "loadmemopts", "lamo", "lama", "opt_lvc", 
  "opt_language", "opt_characts", "opt_version", "raw_data", 
  "raw_elements", "file_raw", "e_expr", "expr", "xpr", "any_num", 0
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] =
{
       0,    96,    97,    97,    97,    98,    98,    98,    99,    98,
     100,   101,   101,   102,   102,   103,   103,   103,   103,   103,
     103,   103,   103,   103,   103,   103,   103,   103,   103,   103,
     103,   104,   104,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   114,   115,   116,   116,   116,   117,
     117,   118,   118,   119,   119,   119,   119,   119,   119,   120,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     122,   122,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   122,   122,   122,   123,
     124,   125,   125,   125,   125,   126,   126,   127,   128,   128,
     128,   129,   129,   129,   129,   130,   130,   131,   132,   132,
     132,   132,   132,   132,   132,   132,   132,   132,   132,   133,
     133,   133,   133,   133,   133,   133,   133,   133,   133,   133,
     133,   133,   133,   133,   133,   133,   133,   133,   134,   134,
     135,   136,   137,   137,   138,   138,   139,   140,   140,   141,
     142,   143,   143,   143,   143,   144,   144,   144,   144,   144,
     144,   144,   145,   146,   147,   147,   147,   147,   148,   148,
     148,   148,   149,   149,   149,   149,   149,   150,   151,   152,
     152,   153,   153,   154,   155,   155,   155,   155,   155,   155,
     155,   155,   156,   156,   157,   158,   158,   159,   159,   159,
     160,   160,   161,   162,   162,   162,   163,   163,   163,   164,
     164,   164,   164,   165,   165,   165,   166,   166,   166,   166,
     167,   168,   169,   170,   171,   171,   171,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   172,   172,   173,   173,
     174,   175,   175,   175,   175,   175,   175,   175,   175,   175,
     175,   175,   175,   175,   176,   176
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] =
{
       0,     1,     0,     2,     2,     3,     3,     1,     0,     5,
       0,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     1,     1,     6,     0,     5,     5,     0,
       2,     1,     3,     1,     1,     1,     1,     1,     1,    13,
       0,     3,     3,     3,     2,     3,     3,     2,     2,     2,
       0,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,    10,    12,
      10,     0,     4,     6,     8,    17,    15,     4,     0,     2,
       4,     3,     3,     1,     2,     1,     1,    14,     0,     3,
       3,     3,     2,     2,     3,     3,     2,     2,     2,     0,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,    10,    19,    16,
      14,    12,     0,     1,     0,     2,     9,     0,     2,     4,
       3,     0,     6,     3,     5,     0,     3,     3,     3,     3,
       3,     3,     4,     3,     0,     4,     3,     5,     0,     2,
       5,     6,     0,     2,     4,     6,     8,     4,     3,     0,
       4,     0,     1,     6,     0,     9,     9,     3,     3,     3,
       3,     3,     0,     2,     5,     0,     2,     1,     4,     4,
       1,     3,     9,     0,     3,     2,     0,     2,     2,     1,
       1,     1,     1,     1,     1,     1,     0,     2,     2,     2,
       4,     2,     2,     4,     1,     1,     2,     1,     2,     1,
       3,     3,     4,     3,     4,     3,     1,     1,     0,     1,
       1,     3,     3,     3,     3,     3,     3,     3,     2,     2,
       2,     3,     1,     2,     1,     1
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] =
{
       2,     1,     4,   254,   255,    10,   206,     8,     0,     0,
       0,     0,     0,     3,     7,     0,    10,   240,   252,     0,
     216,     0,   250,   249,   248,   253,     0,   179,     0,     0,
       0,     0,     0,     0,     0,     0,    43,    44,   206,   206,
     206,   206,   206,   206,   206,   206,   206,   206,   206,   206,
     206,   206,   206,     6,    16,    17,    23,    21,    22,    26,
      27,    20,    29,   206,    15,    18,    19,    24,    25,    30,
      28,   212,   215,   211,   213,   209,   214,   210,   207,   208,
     178,     0,   251,     0,     5,   243,   247,   244,   241,   242,
     245,   246,   216,   216,   216,     0,     0,   216,   216,   216,
     216,   184,   216,   216,   216,     0,   216,   216,     0,     0,
       0,   217,   218,   219,     0,   177,   181,     0,    33,    32,
      31,   236,     0,   237,    34,    35,     0,     0,     0,     0,
      39,    40,     0,    37,    38,    36,     0,    41,    42,   221,
     222,     0,     9,   182,     0,    46,     0,     0,     0,   151,
     149,   164,   162,     0,     0,     0,     0,     0,     0,     0,
     192,     0,     0,   180,     0,   225,   227,   229,   224,     0,
     181,     0,     0,     0,     0,     0,     0,   188,   189,   190,
     187,   191,     0,   216,   220,     0,    45,     0,   226,   228,
     223,     0,     0,     0,     0,     0,   150,     0,     0,   163,
       0,     0,     0,   183,   193,     0,     0,     0,   231,   233,
     235,   230,     0,     0,     0,   181,   153,   181,   168,   166,
     172,     0,     0,     0,   203,    49,    49,   232,   234,     0,
       0,     0,     0,     0,   238,   165,   238,     0,     0,     0,
     195,     0,     0,    47,    48,    60,   144,   181,   154,   181,
     181,   181,   181,   181,   181,     0,   169,     0,   173,   167,
       0,     0,     0,   205,     0,   202,    55,    54,    56,    57,
      58,    53,    50,    51,     0,     0,   108,   152,   157,   156,
     159,   160,   161,   158,   238,   238,     0,     0,     0,   194,
     197,   196,   204,     0,     0,     0,     0,     0,     0,     0,
      70,    64,    67,    68,    69,   145,     0,   181,   239,     0,
     174,   185,   186,     0,    52,    12,    66,    11,     0,    14,
      13,    65,    63,     0,     0,    62,   103,    61,     0,     0,
       0,     0,     0,     0,     0,   119,   112,   113,   116,   117,
     118,   182,   170,   238,     0,     0,   104,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    59,   115,     0,
     114,   111,   110,   109,     0,   171,     0,   175,   198,   199,
     200,    97,   102,   101,   181,   181,    81,    83,    84,    76,
      77,    79,    80,    82,    78,    74,     0,    73,    75,   181,
      71,    72,    87,    86,    85,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   107,     0,     0,     0,     0,     0,
       0,    97,   181,   181,   130,   132,   133,   125,   126,   128,
     129,   131,   127,   123,     0,   122,   124,   181,   120,   121,
     136,   135,   134,   176,   201,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   106,     0,   105,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   147,     0,
       0,     0,     0,    91,     0,     0,     0,     0,   146,     0,
       0,     0,     0,     0,    88,     0,     0,     0,   148,    91,
       0,     0,     0,     0,     0,    98,     0,   137,     0,     0,
       0,     0,     0,     0,    90,     0,     0,    98,     0,    92,
      98,    99,     0,     0,   144,     0,     0,    89,     0,     0,
      98,   142,     0,    93,   100,     0,   144,   141,   143,     0,
       0,     0,   142,     0,    94,    96,   140,     0,     0,   142,
      95,     0,   139,   144,   142,   138,     0,     0,     0
};

static const short yydefgoto[] =
{
     566,     1,    13,    21,    19,   320,   321,    53,   121,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
     164,   243,   272,   273,    65,   274,   328,   386,   395,   504,
     400,   301,   524,   325,   477,    66,   306,   374,   448,   434,
     443,   547,   276,   337,   498,    67,   150,   173,   232,    68,
     152,   174,   235,   237,    14,    15,    83,   233,    69,   132,
     182,   204,   262,   291,   379,    70,   241,    20,    78,    79,
     122,   111,   112,   113,   123,   170,   124,   255,   317,    17,
      18
};

static const short yypact[] =
{
  -32768,     6,-32768,-32768,-32768,-32768,-32768,-32768,   107,   107,
     107,    86,   107,-32768,-32768,   -39,-32768,   566,-32768,   285,
     605,   107,-32768,-32768,-32768,-32768,   553,-32768,   285,   107,
     107,   107,   107,   107,   107,   107,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
     258,   -31,-32768,    54,-32768,   579,   368,   334,    28,    28,
  -32768,-32768,   605,   328,   328,    32,    32,   605,   605,   328,
     328,   605,   328,   328,   328,    32,   328,   328,   107,   107,
     107,-32768,-32768,-32768,   107,-32768,    10,     0,-32768,-32768,
  -32768,-32768,   110,-32768,-32768,-32768,    69,    72,   123,   180,
  -32768,-32768,   549,-32768,-32768,-32768,    78,-32768,-32768,-32768,
  -32768,    85,-32768,-32768,   108,-32768,    62,   107,   107,-32768,
  -32768,-32768,-32768,   107,   107,   107,   107,   107,   107,   107,
  -32768,   107,   107,-32768,    34,-32768,-32768,-32768,-32768,   264,
     -62,   102,   114,   206,   420,   125,   141,-32768,-32768,-32768,
  -32768,-32768,   -27,-32768,-32768,   143,-32768,   149,-32768,-32768,
  -32768,   204,   107,   107,    -4,   130,-32768,    -2,   210,-32768,
     107,   107,   221,-32768,-32768,   191,   107,   107,-32768,-32768,
  -32768,-32768,   274,   153,   157,    10,-32768,   -59,   189,-32768,
     195,   200,   212,    -5,-32768,   223,   223,-32768,-32768,   107,
     107,   107,    47,   341,   107,-32768,   107,   184,   107,   107,
  -32768,   446,   497,-32768,-32768,-32768,   224,    57,-32768,   435,
     435,   435,   435,   435,   435,   232,   233,   237,   233,-32768,
     239,   244,   -36,-32768,   107,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,   248,-32768,   251,   107,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,   107,   107,   107,   107,   267,-32768,
  -32768,-32768,-32768,   497,    94,   107,    43,   342,    11,    11,
  -32768,-32768,-32768,-32768,-32768,-32768,   312,   275,-32768,   254,
     233,-32768,-32768,   259,-32768,-32768,-32768,-32768,   260,-32768,
  -32768,-32768,-32768,    86,    11,   278,-32768,   278,   370,    94,
     107,    43,   351,    11,    11,-32768,-32768,-32768,-32768,-32768,
  -32768,   107,-32768,   107,   162,   358,-32768,   -18,    11,    43,
     359,   359,   359,   359,   359,   359,   359,   359,   359,   107,
     107,   107,    43,   107,   359,   359,   359,-32768,-32768,   273,
  -32768,-32768,   278,   278,   407,-32768,   284,   233,-32768,   293,
  -32768,-32768,-32768,-32768,    10,    10,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,   294,-32768,-32768,    10,
  -32768,-32768,-32768,-32768,-32768,   372,    43,   385,   385,   385,
     385,   385,   385,   385,   385,   385,   107,   107,   107,    43,
     107,   385,   385,   385,-32768,   107,   107,   107,   107,   107,
     107,   318,    10,    10,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,   319,-32768,-32768,    10,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,   374,   375,   383,   384,   107,
     107,   107,   107,   107,   107,   107,   107,   171,   393,   396,
     399,   400,   404,   410,   418,   419,-32768,   421,-32768,   107,
     107,   107,   107,   171,   107,   107,   107,    11,   433,   436,
     437,   438,   442,   443,   444,   452,    96,   107,-32768,   107,
     107,   107,    11,   107,-32768,   107,   107,   107,-32768,   443,
     453,   456,   119,   460,   462,   465,   475,-32768,   107,   107,
     107,   107,   107,    11,-32768,   107,   476,   465,   477,   479,
     465,   121,   483,   107,   224,   107,    11,-32768,    11,   107,
     465,   427,   484,   132,   278,   485,   224,-32768,-32768,   107,
      11,   107,   427,   487,   278,   488,-32768,   107,    11,    79,
     278,    11,-32768,   148,   427,-32768,   513,   539,-32768
};

static const short yypgoto[] =
{
  -32768,-32768,-32768,-32768,   590,  -276,  -180,   581,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,   381,-32768,   317,-32768,-32768,-32768,   209,  -333,   112,
  -32768,   320,  -505,  -291,   140,-32768,-32768,-32768,-32768,   190,
     134,  -483,  -517,-32768,-32768,-32768,   392,-32768,   -30,-32768,
     388,-32768,-32768,-32768,-32768,-32768,-32768,  -109,-32768,-32768,
  -32768,   371,-32768,-32768,-32768,-32768,-32768,   545,-32768,-32768,
     -19,  -262,  -251,  -250,  -213,-32768,   528,  -233,    -1,   440,
      21
};


#define	YYLAST		668


static const short yytable[] =
{
      16,    80,   215,   257,   218,   202,   288,   144,   327,     2,
       3,     4,   302,     5,   202,     3,     4,   541,   316,   190,
      81,  -155,   534,   303,   304,   537,     6,   397,   398,   552,
     401,   143,    25,   347,   143,   546,     3,     4,     3,     4,
     185,    27,   372,   373,   338,   289,   564,     3,     4,   319,
     315,   307,   309,   368,   203,   339,   340,   383,     3,     4,
     108,   191,   114,   109,   110,   348,   165,   166,   167,   556,
       7,   168,   216,   117,   219,   240,   562,   382,   128,   129,
     145,   565,   116,    71,    72,    73,    74,    75,    76,    77,
       3,     4,     8,     9,   126,   127,    10,    11,     3,     4,
      12,   315,   323,   143,   136,   324,   231,   139,   140,   141,
     376,     3,     4,   142,   163,   186,    34,    35,     8,     9,
       8,     9,    10,    11,    10,    11,    12,   149,    12,     8,
       9,  -155,  -155,    10,    11,   115,   217,    12,  -155,  -216,
       8,     9,  -216,  -216,    10,    11,   171,   172,    12,   169,
     143,   370,   175,   176,   177,   178,   179,   180,   181,  -216,
     183,   184,   147,   187,   205,   148,     3,     4,   378,   384,
     108,   161,   561,   109,   110,     3,     4,   476,   162,   348,
       8,     9,   399,   108,    10,    11,   109,   110,    12,   507,
     146,   213,   214,     8,     9,   192,   496,    10,    11,   221,
     222,    12,   348,   149,   348,   225,   226,   193,   208,   209,
     210,   512,   520,   211,   538,   348,   220,   277,   200,   278,
     279,   280,   281,   282,   283,   550,   432,   223,   245,   246,
     247,   348,   531,   256,   201,   258,   206,   260,   261,   447,
     108,   275,   207,   109,   110,   543,   229,   544,     8,     9,
     230,   108,    10,    11,   109,   110,    12,     8,     9,   554,
     151,    10,    11,   292,   151,    12,   294,   560,   188,   189,
     563,   224,   295,   313,   305,   427,   428,   342,   227,   228,
     194,   195,   234,   308,   310,   311,   312,   196,   236,    36,
     430,   212,    37,   238,   318,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,   239,    48,    49,    50,   296,
     297,   108,   298,   299,   109,   110,   242,   275,   108,   326,
     326,   109,   110,   460,   461,   284,  -239,   329,   548,   369,
     285,   300,   286,   330,   118,   119,   120,   287,   463,   548,
     375,   293,   377,   380,   346,   326,   548,   343,   322,  -155,
    -155,   548,   344,   345,   326,   326,  -155,   371,   396,   396,
     396,   348,   396,    51,   381,   385,   405,    52,   341,   326,
     331,   332,   108,   333,   334,   109,   110,   425,   431,    71,
      72,    73,    74,    75,    76,    77,   426,   429,   249,   250,
     251,   433,   335,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     366,   459,   462,   252,   253,   444,   444,   444,   254,   444,
      32,    33,    34,    35,   453,   454,   455,   456,   457,   458,
     406,   407,   408,   409,   410,   411,   412,   413,   414,   415,
     416,   417,   418,   419,   420,   421,   422,   423,    22,    23,
      24,   367,    26,    31,    32,    33,    34,    35,   468,   469,
     470,   471,   472,   473,   474,   475,   478,   464,   465,    85,
      86,    87,    88,    89,    90,    91,   466,   467,   488,   489,
     490,   491,   478,   493,   494,   495,   479,  -216,   424,   480,
    -216,  -216,   481,   482,   197,   198,   508,   483,   509,   510,
     511,   199,   513,   484,   514,   515,   516,  -216,   326,  -155,
    -155,   485,   486,   567,   487,  -155,  -155,   526,   527,   528,
     529,   530,   263,   326,   532,   264,   497,   265,   143,   499,
     500,   501,   540,   266,   542,   502,   503,   505,   545,   568,
     267,   268,   269,   270,   326,   506,   518,   271,   553,   519,
     555,   445,   446,   521,   449,   522,   559,   326,   523,   326,
     387,   388,   389,   390,   391,   392,   393,   394,   525,   533,
     535,   326,   536,   402,   403,   404,   539,   549,   551,   326,
     557,   558,   326,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   435,   436,
     437,   438,   439,   440,   441,   442,    28,   244,   107,    84,
     314,   450,   451,   452,   153,   154,   155,   156,   157,   158,
     159,   517,   125,   492,   248,   259,   336,   130,   131,   160,
     133,   134,   135,   290,   137,   138,    29,    30,    31,    32,
      33,    34,    35,     0,     0,     0,     0,     0,    82,    29,
      30,    31,    32,    33,    34,    35,    71,    72,    73,    74,
      75,    76,    77,    30,    31,    32,    33,    34,    35
};

static const short yycheck[] =
{
       1,    20,     6,   236,     6,    41,    42,   116,   299,     3,
       4,     5,   274,     7,    41,     4,     5,   534,   294,    81,
      21,    80,   527,   274,   274,   530,    20,   360,   361,   546,
     363,    93,    11,   324,    93,   540,     4,     5,     4,     5,
       6,    80,   333,   334,   306,    81,   563,     4,     5,     6,
       7,   284,   285,   329,    81,   306,   306,   348,     4,     5,
      60,   170,    93,    63,    64,    83,     4,     5,     6,   552,
      64,     9,    76,    92,    76,    80,   559,    95,    97,    98,
      80,   564,    83,    51,    52,    53,    54,    55,    56,    57,
       4,     5,    86,    87,    95,    96,    90,    91,     4,     5,
      94,     7,    91,    93,   105,    94,   215,   108,   109,   110,
     343,     4,     5,   114,     6,    81,    88,    89,    86,    87,
      86,    87,    90,    91,    90,    91,    94,    80,    94,    86,
      87,    74,    75,    90,    91,    81,     6,    94,    81,    60,
      86,    87,    63,    64,    90,    91,   147,   148,    94,    87,
      93,   331,   153,   154,   155,   156,   157,   158,   159,    80,
     161,   162,    93,   164,   183,    93,     4,     5,     6,   349,
      60,    93,    93,    63,    64,     4,     5,     6,    93,    83,
      86,    87,   362,    60,    90,    91,    63,    64,    94,    93,
      80,   192,   193,    86,    87,    93,   487,    90,    91,   200,
     201,    94,    83,    80,    83,   206,   207,    93,     4,     5,
       6,   502,    93,     9,    93,    83,     6,   247,    93,   249,
     250,   251,   252,   253,   254,    93,   406,     6,   229,   230,
     231,    83,   523,   234,    93,   236,    93,   238,   239,   419,
      60,    93,    93,    63,    64,   536,    93,   538,    86,    87,
      93,    60,    90,    91,    63,    64,    94,    86,    87,   550,
      80,    90,    91,   264,    80,    94,    15,   558,     4,     5,
     561,    80,    21,     6,   275,   384,   385,   307,     4,     5,
      74,    75,    93,   284,   285,   286,   287,    81,    93,     4,
     399,    87,     7,    93,   295,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    93,    21,    22,    23,    58,
      59,    60,    61,    62,    63,    64,    93,    93,    60,   298,
     299,    63,    64,   432,   433,    93,    93,    15,   541,   330,
      93,    80,    93,    21,     6,     7,     8,    93,   447,   552,
     341,    93,   343,   344,   323,   324,   559,    93,     6,    74,
      75,   564,    93,    93,   333,   334,    81,     6,   359,   360,
     361,    83,   363,    78,     6,     6,    93,    82,    93,   348,
      58,    59,    60,    61,    62,    63,    64,    93,     6,    51,
      52,    53,    54,    55,    56,    57,    93,    93,    47,    48,
      49,     6,    80,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    93,    93,    72,    73,   416,   417,   418,    77,   420,
      86,    87,    88,    89,   425,   426,   427,   428,   429,   430,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,     8,     9,
      10,    81,    12,    85,    86,    87,    88,    89,   459,   460,
     461,   462,   463,   464,   465,   466,   467,    93,    93,    29,
      30,    31,    32,    33,    34,    35,    93,    93,   479,   480,
     481,   482,   483,   484,   485,   486,    93,    60,    81,    93,
      63,    64,    93,    93,    74,    75,   497,    93,   499,   500,
     501,    81,   503,    93,   505,   506,   507,    80,   487,    74,
      75,    93,    93,     0,    93,    80,    81,   518,   519,   520,
     521,   522,    76,   502,   525,    79,    93,    81,    93,    93,
      93,    93,   533,    36,   535,    93,    93,    93,   539,     0,
      43,    44,    45,    46,   523,    93,    93,    50,   549,    93,
     551,   417,   418,    93,   420,    93,   557,   536,    93,   538,
     351,   352,   353,   354,   355,   356,   357,   358,    93,    93,
      93,   550,    93,   364,   365,   366,    93,    93,    93,   558,
      93,    93,   561,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,   408,   409,
     410,   411,   412,   413,   414,   415,    16,   226,    63,    28,
     293,   421,   422,   423,    65,    66,    67,    68,    69,    70,
      71,   509,    94,   483,   232,   237,   306,    99,   100,    80,
     102,   103,   104,   262,   106,   107,    83,    84,    85,    86,
      87,    88,    89,    -1,    -1,    -1,    -1,    -1,    95,    83,
      84,    85,    86,    87,    88,    89,    51,    52,    53,    54,
      55,    56,    57,    84,    85,    86,    87,    88,    89
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/share/bison/bison.simple"

/* Skeleton output parser for bison,

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software
   Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser when
   the %semantic_parser declaration is not specified in the grammar.
   It was written by Richard Stallman by simplifying the hairy parser
   used when %semantic_parser is specified.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

#if ! defined (yyoverflow) || defined (YYERROR_VERBOSE)

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || defined (YYERROR_VERBOSE) */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYLTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
# if YYLSP_NEEDED
  YYLTYPE yyls;
# endif
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAX (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# if YYLSP_NEEDED
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE) + sizeof (YYLTYPE))	\
      + 2 * YYSTACK_GAP_MAX)
# else
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAX)
# endif

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAX;	\
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif


#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");			\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).

   When YYLLOC_DEFAULT is run, CURRENT is set the location of the
   first token.  By default, to implement support for ranges, extend
   its range to the last symbol.  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)       	\
   Current.last_line   = Rhs[N].last_line;	\
   Current.last_column = Rhs[N].last_column;
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#if YYPURE
# if YYLSP_NEEDED
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, &yylloc, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval, &yylloc)
#  endif
# else /* !YYLSP_NEEDED */
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval)
#  endif
# endif /* !YYLSP_NEEDED */
#else /* !YYPURE */
# define YYLEX			yylex ()
#endif /* !YYPURE */


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)
/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
#endif /* !YYDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif

#ifdef YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif
#endif

#line 315 "/usr/share/bison/bison.simple"


/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
#  define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL
# else
#  define YYPARSE_PARAM_ARG YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
# endif
#else /* !YYPARSE_PARAM */
# define YYPARSE_PARAM_ARG
# define YYPARSE_PARAM_DECL
#endif /* !YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
# ifdef YYPARSE_PARAM
int yyparse (void *);
# else
int yyparse (void);
# endif
#endif

/* YY_DECL_VARIABLES -- depending whether we use a pure parser,
   variables are global, or local to YYPARSE.  */

#define YY_DECL_NON_LSP_VARIABLES			\
/* The lookahead symbol.  */				\
int yychar;						\
							\
/* The semantic value of the lookahead symbol. */	\
YYSTYPE yylval;						\
							\
/* Number of parse errors so far.  */			\
int yynerrs;

#if YYLSP_NEEDED
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES			\
						\
/* Location data for the lookahead symbol.  */	\
YYLTYPE yylloc;
#else
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES
#endif


/* If nonreentrant, generate the variables here. */

#if !YYPURE
YY_DECL_VARIABLES
#endif  /* !YYPURE */

int
yyparse (YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  /* If reentrant, generate the variables here. */
#if YYPURE
  YY_DECL_VARIABLES
#endif  /* !YYPURE */

  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yychar1 = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack. */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;

#if YYLSP_NEEDED
  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
#endif

#if YYLSP_NEEDED
# define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
# define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  YYSIZE_T yystacksize = YYINITDEPTH;


  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
#if YYLSP_NEEDED
  YYLTYPE yyloc;
#endif

  /* When reducing, the number of symbols on the RHS of the reduced
     rule. */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;
#if YYLSP_NEEDED
  yylsp = yyls;
#endif
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  */
# if YYLSP_NEEDED
	YYLTYPE *yyls1 = yyls;
	/* This used to be a conditional around just the two extra args,
	   but that might be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
# else
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);
# endif
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
# if YYLSP_NEEDED
	YYSTACK_RELOCATE (yyls);
# endif
# undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
#if YYLSP_NEEDED
      yylsp = yyls + yysize - 1;
#endif

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yychar1 = YYTRANSLATE (yychar);

#if YYDEBUG
     /* We have to keep this `#if YYDEBUG', since we use variables
	which are defined only if `YYDEBUG' is set.  */
      if (yydebug)
	{
	  YYFPRINTF (stderr, "Next token is %d (%s",
		     yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise
	     meaning of a token, for further debugging info.  */
# ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
# endif
	  YYFPRINTF (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %d (%s), ",
	      yychar, yytname[yychar1]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to the semantic value of
     the lookahead token.  This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

#if YYLSP_NEEDED
  /* Similarly for the default location.  Let the user run additional
     commands if for instance locations are ranges.  */
  yyloc = yylsp[1-yylen];
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
#endif

#if YYDEBUG
  /* We have to keep this `#if YYDEBUG', since we use variables which
     are defined only if `YYDEBUG' is set.  */
  if (yydebug)
    {
      int yyi;

      YYFPRINTF (stderr, "Reducing via rule %d (line %d), ",
		 yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (yyi = yyprhs[yyn]; yyrhs[yyi] > 0; yyi++)
	YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
      YYFPRINTF (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif

  switch (yyn) {

case 1:
#line 361 "./parser.y"
{
		resource_t *rsc;
		/* First add stringtables to the resource-list */
		rsc = build_stt_resources(sttres);
		/* 'build_stt_resources' returns a head and $1 is a tail */
		if(yyvsp[0].res)
		{
			yyvsp[0].res->next = rsc;
			if(rsc)
				rsc->prev = yyvsp[0].res;
		}
		else
			yyvsp[0].res = rsc;
		/* Find the tail again */
		while(yyvsp[0].res && yyvsp[0].res->next)
			yyvsp[0].res = yyvsp[0].res->next;
		/* Now add any fontdirecory */
		rsc = build_fontdirs(yyvsp[0].res);
		/* 'build_fontdir' returns a head and $1 is a tail */
		if(yyvsp[0].res)
		{
			yyvsp[0].res->next = rsc;
			if(rsc)
				rsc->prev = yyvsp[0].res;
		}
		else
			yyvsp[0].res = rsc;
		/* Final statement before were done */
		resource_top = get_resource_head(yyvsp[0].res);
		}
    break;
case 2:
#line 395 "./parser.y"
{ yyval.res = NULL; want_id = 1; }
    break;
case 3:
#line 396 "./parser.y"
{
		if(yyvsp[0].res)
		{
			resource_t *tail = yyvsp[0].res;
			resource_t *head = yyvsp[0].res;
			while(tail->next)
				tail = tail->next;
			while(head->prev)
				head = head->prev;
			head->prev = yyvsp[-1].res;
			if(yyvsp[-1].res)
				yyvsp[-1].res->next = head;
			yyval.res = tail;
			/* Check for duplicate identifiers */
			while(yyvsp[-1].res && head)
			{
				resource_t *rsc = yyvsp[-1].res;
				while(rsc)
				{
					if(rsc->type == head->type
					&& rsc->lan->id == head->lan->id
					&& rsc->lan->sub == head->lan->sub
					&& !compare_name_id(rsc->name, head->name))
					{
						yyerror("Duplicate resource name '%s'", get_nameid_str(rsc->name));
					}
					rsc = rsc->prev;
				}
				head = head->next;
			}
		}
		else if(yyvsp[-1].res)
		{
			resource_t *tail = yyvsp[-1].res;
			while(tail->next)
				tail = tail->next;
			yyval.res = tail;
		}
		else
			yyval.res = NULL;

		if(!dont_want_id)	/* See comments in language parsing below */
			want_id = 1;
		dont_want_id = 0;
		}
    break;
case 5:
#line 472 "./parser.y"
{
		yyval.res = yyvsp[0].res;
		if(yyval.res)
		{
			if(yyvsp[-2].num > 65535 || yyvsp[-2].num < -32768)
				yyerror("Resource's ID out of range (%d)", yyvsp[-2].num);
			yyval.res->name = new_name_id();
			yyval.res->name->type = name_ord;
			yyval.res->name->name.i_name = yyvsp[-2].num;
			chat("Got %s (%d)", get_typename(yyvsp[0].res), yyval.res->name->name.i_name);
			}
			}
    break;
case 6:
#line 484 "./parser.y"
{
		yyval.res = yyvsp[0].res;
		if(yyval.res)
		{
			yyval.res->name = new_name_id();
			yyval.res->name->type = name_str;
			yyval.res->name->name.s_name = yyvsp[-2].str;
			chat("Got %s (%s)", get_typename(yyvsp[0].res), yyval.res->name->name.s_name->str.cstr);
		}
		}
    break;
case 7:
#line 494 "./parser.y"
{
		/* Don't do anything, stringtables are converted to
		 * resource_t structures when we are finished parsing and
		 * the final rule of the parser is reduced (see above)
		 */
		yyval.res = NULL;
		chat("Got STRINGTABLE");
		}
    break;
case 8:
#line 502 "./parser.y"
{want_nl = 1; }
    break;
case 9:
#line 502 "./parser.y"
{
		/* We *NEED* the newline to delimit the expression.
		 * Otherwise, we would not be able to set the next
		 * want_id anymore because of the token-lookahead.
		 *
		 * However, we can test the lookahead-token for
		 * being "non-expression" type, in which case we
		 * continue. Fortunately, tNL is the only token that
		 * will break expression parsing and is implicitely
		 * void, so we just remove it. This scheme makes it
		 * possible to do some (not all) fancy preprocessor
		 * stuff.
		 * BTW, we also need to make sure that the next
		 * reduction of 'resources' above will *not* set
		 * want_id because we already have a lookahead that
		 * cannot be undone.
		 */
		if(yychar != YYEMPTY && yychar != tNL)
			dont_want_id = 1;

		if(yychar == tNL)
			yychar = YYEMPTY;	/* Could use 'yyclearin', but we already need the*/
						/* direct access to yychar in rule 'usrcvt' below. */
		else if(yychar == tIDENT)
			yywarning("LANGUAGE statement not delimited with newline; next identifier might be wrong");

		want_nl = 0;	/* We don't want it anymore if we didn't get it */

		if(!win32)
			yywarning("LANGUAGE not supported in 16-bit mode");
		if(currentlanguage)
			free(currentlanguage);
		if (get_language_codepage(yyvsp[-2].num, yyvsp[0].num) == -1)
			yyerror( "Language %04x is not supported", (yyvsp[0].num<<10) + yyvsp[-2].num);
		currentlanguage = new_language(yyvsp[-2].num, yyvsp[0].num);
		yyval.res = NULL;
		chat("Got LANGUAGE %d,%d (0x%04x)", yyvsp[-2].num, yyvsp[0].num, (yyvsp[0].num<<10) + yyvsp[-2].num);
		}
    break;
case 10:
#line 546 "./parser.y"
{ yychar = rsrcid_to_token(yychar); }
    break;
case 11:
#line 552 "./parser.y"
{
		if(yyvsp[0].num > 65535 || yyvsp[0].num < -32768)
			yyerror("Resource's ID out of range (%d)", yyvsp[0].num);
		yyval.nid = new_name_id();
		yyval.nid->type = name_ord;
		yyval.nid->name.i_name = yyvsp[0].num;
		}
    break;
case 12:
#line 559 "./parser.y"
{
		yyval.nid = new_name_id();
		yyval.nid->type = name_str;
		yyval.nid->name.s_name = yyvsp[0].str;
		}
    break;
case 13:
#line 569 "./parser.y"
{ yyval.nid = yyvsp[0].nid; }
    break;
case 14:
#line 570 "./parser.y"
{
		yyval.nid = new_name_id();
		yyval.nid->type = name_str;
		yyval.nid->name.s_name = yyvsp[0].str;
		}
    break;
case 15:
#line 579 "./parser.y"
{ yyval.res = new_resource(res_acc, yyvsp[0].acc, yyvsp[0].acc->memopt, yyvsp[0].acc->lvc.language); }
    break;
case 16:
#line 580 "./parser.y"
{ yyval.res = new_resource(res_bmp, yyvsp[0].bmp, yyvsp[0].bmp->memopt, yyvsp[0].bmp->data->lvc.language); }
    break;
case 17:
#line 581 "./parser.y"
{
		resource_t *rsc;
		if(yyvsp[0].ani->type == res_anicur)
		{
			yyval.res = rsc = new_resource(res_anicur, yyvsp[0].ani->u.ani, yyvsp[0].ani->u.ani->memopt, yyvsp[0].ani->u.ani->data->lvc.language);
		}
		else if(yyvsp[0].ani->type == res_curg)
		{
			cursor_t *cur;
			yyval.res = rsc = new_resource(res_curg, yyvsp[0].ani->u.curg, yyvsp[0].ani->u.curg->memopt, yyvsp[0].ani->u.curg->lvc.language);
			for(cur = yyvsp[0].ani->u.curg->cursorlist; cur; cur = cur->next)
			{
				rsc->prev = new_resource(res_cur, cur, yyvsp[0].ani->u.curg->memopt, yyvsp[0].ani->u.curg->lvc.language);
				rsc->prev->next = rsc;
				rsc = rsc->prev;
				rsc->name = new_name_id();
				rsc->name->type = name_ord;
				rsc->name->name.i_name = cur->id;
			}
		}
		else
			internal_error(__FILE__, __LINE__, "Invalid top-level type %d in cursor resource", yyvsp[0].ani->type);
		free(yyvsp[0].ani);
		}
    break;
case 18:
#line 605 "./parser.y"
{ yyval.res = new_resource(res_dlg, yyvsp[0].dlg, yyvsp[0].dlg->memopt, yyvsp[0].dlg->lvc.language); }
    break;
case 19:
#line 606 "./parser.y"
{
		if(win32)
			yyval.res = new_resource(res_dlgex, yyvsp[0].dlgex, yyvsp[0].dlgex->memopt, yyvsp[0].dlgex->lvc.language);
		else
			yyval.res = NULL;
		}
    break;
case 20:
#line 612 "./parser.y"
{ yyval.res = new_resource(res_dlginit, yyvsp[0].dginit, yyvsp[0].dginit->memopt, yyvsp[0].dginit->data->lvc.language); }
    break;
case 21:
#line 613 "./parser.y"
{ yyval.res = new_resource(res_fnt, yyvsp[0].fnt, yyvsp[0].fnt->memopt, yyvsp[0].fnt->data->lvc.language); }
    break;
case 22:
#line 614 "./parser.y"
{ yyval.res = new_resource(res_fntdir, yyvsp[0].fnd, yyvsp[0].fnd->memopt, yyvsp[0].fnd->data->lvc.language); }
    break;
case 23:
#line 615 "./parser.y"
{
		resource_t *rsc;
		if(yyvsp[0].ani->type == res_aniico)
		{
			yyval.res = rsc = new_resource(res_aniico, yyvsp[0].ani->u.ani, yyvsp[0].ani->u.ani->memopt, yyvsp[0].ani->u.ani->data->lvc.language);
		}
		else if(yyvsp[0].ani->type == res_icog)
		{
			icon_t *ico;
			yyval.res = rsc = new_resource(res_icog, yyvsp[0].ani->u.icog, yyvsp[0].ani->u.icog->memopt, yyvsp[0].ani->u.icog->lvc.language);
			for(ico = yyvsp[0].ani->u.icog->iconlist; ico; ico = ico->next)
			{
				rsc->prev = new_resource(res_ico, ico, yyvsp[0].ani->u.icog->memopt, yyvsp[0].ani->u.icog->lvc.language);
				rsc->prev->next = rsc;
				rsc = rsc->prev;
				rsc->name = new_name_id();
				rsc->name->type = name_ord;
				rsc->name->name.i_name = ico->id;
			}
		}
		else
			internal_error(__FILE__, __LINE__, "Invalid top-level type %d in icon resource", yyvsp[0].ani->type);
		free(yyvsp[0].ani);
		}
    break;
case 24:
#line 639 "./parser.y"
{ yyval.res = new_resource(res_men, yyvsp[0].men, yyvsp[0].men->memopt, yyvsp[0].men->lvc.language); }
    break;
case 25:
#line 640 "./parser.y"
{
		if(win32)
			yyval.res = new_resource(res_menex, yyvsp[0].menex, yyvsp[0].menex->memopt, yyvsp[0].menex->lvc.language);
		else
			yyval.res = NULL;
		}
    break;
case 26:
#line 646 "./parser.y"
{ yyval.res = new_resource(res_msg, yyvsp[0].msg, WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE, yyvsp[0].msg->data->lvc.language); }
    break;
case 27:
#line 647 "./parser.y"
{ yyval.res = new_resource(res_rdt, yyvsp[0].rdt, yyvsp[0].rdt->memopt, yyvsp[0].rdt->data->lvc.language); }
    break;
case 28:
#line 648 "./parser.y"
{ yyval.res = new_resource(res_toolbar, yyvsp[0].tlbar, yyvsp[0].tlbar->memopt, yyvsp[0].tlbar->lvc.language); }
    break;
case 29:
#line 649 "./parser.y"
{ yyval.res = new_resource(res_usr, yyvsp[0].usr, yyvsp[0].usr->memopt, yyvsp[0].usr->data->lvc.language); }
    break;
case 30:
#line 650 "./parser.y"
{ yyval.res = new_resource(res_ver, yyvsp[0].veri, WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE, yyvsp[0].veri->lvc.language); }
    break;
case 31:
#line 654 "./parser.y"
{ yyval.str = make_filename(yyvsp[0].str); }
    break;
case 32:
#line 655 "./parser.y"
{ yyval.str = make_filename(yyvsp[0].str); }
    break;
case 33:
#line 656 "./parser.y"
{ yyval.str = make_filename(yyvsp[0].str); }
    break;
case 34:
#line 660 "./parser.y"
{ yyval.bmp = new_bitmap(yyvsp[0].raw, yyvsp[-1].iptr); }
    break;
case 35:
#line 664 "./parser.y"
{
		yyval.ani = new_ani_any();
		if(yyvsp[0].raw->size > 4 && !memcmp(yyvsp[0].raw->data, riff, sizeof(riff)))
		{
			yyval.ani->type = res_anicur;
			yyval.ani->u.ani = new_ani_curico(res_anicur, yyvsp[0].raw, yyvsp[-1].iptr);
		}
		else
		{
			yyval.ani->type = res_curg;
			yyval.ani->u.curg = new_cursor_group(yyvsp[0].raw, yyvsp[-1].iptr);
		}
	}
    break;
case 36:
#line 680 "./parser.y"
{
		yyval.ani = new_ani_any();
		if(yyvsp[0].raw->size > 4 && !memcmp(yyvsp[0].raw->data, riff, sizeof(riff)))
		{
			yyval.ani->type = res_aniico;
			yyval.ani->u.ani = new_ani_curico(res_aniico, yyvsp[0].raw, yyvsp[-1].iptr);
		}
		else
		{
			yyval.ani->type = res_icog;
			yyval.ani->u.icog = new_icon_group(yyvsp[0].raw, yyvsp[-1].iptr);
		}
	}
    break;
case 37:
#line 702 "./parser.y"
{ yyval.fnt = new_font(yyvsp[0].raw, yyvsp[-1].iptr); }
    break;
case 38:
#line 712 "./parser.y"
{ yyval.fnd = new_fontdir(yyvsp[0].raw, yyvsp[-1].iptr); }
    break;
case 39:
#line 720 "./parser.y"
{
		if(!win32)
			yywarning("MESSAGETABLE not supported in 16-bit mode");
		yyval.msg = new_messagetable(yyvsp[0].raw, yyvsp[-1].iptr);
		}
    break;
case 40:
#line 728 "./parser.y"
{ yyval.rdt = new_rcdata(yyvsp[0].raw, yyvsp[-1].iptr); }
    break;
case 41:
#line 732 "./parser.y"
{ yyval.dginit = new_dlginit(yyvsp[0].raw, yyvsp[-1].iptr); }
    break;
case 42:
#line 736 "./parser.y"
{
#ifdef WORDS_BIGENDIAN
			if(pedantic && byteorder != WRC_BO_LITTLE)
#else
			if(pedantic && byteorder == WRC_BO_BIG)
#endif
				yywarning("Byteordering is not little-endian and type cannot be interpreted");
			yyval.usr = new_user(yyvsp[-2].nid, yyvsp[0].raw, yyvsp[-1].iptr);
		}
    break;
case 43:
#line 747 "./parser.y"
{
		yyval.nid = new_name_id();
		yyval.nid->type = name_ord;
		yyval.nid->name.i_name = yyvsp[0].num;
		}
    break;
case 44:
#line 752 "./parser.y"
{
		yyval.nid = new_name_id();
		yyval.nid->type = name_str;
		yyval.nid->name.s_name = yyvsp[0].str;
		}
    break;
case 45:
#line 761 "./parser.y"
{
		yyval.acc = new_accelerator();
		if(yyvsp[-4].iptr)
		{
			yyval.acc->memopt = *(yyvsp[-4].iptr);
			free(yyvsp[-4].iptr);
		}
		else
		{
			yyval.acc->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE;
		}
		if(!yyvsp[-1].event)
			yyerror("Accelerator table must have at least one entry");
		yyval.acc->events = get_event_head(yyvsp[-1].event);
		if(yyvsp[-3].lvc)
		{
			yyval.acc->lvc = *(yyvsp[-3].lvc);
			free(yyvsp[-3].lvc);
		}
		if(!yyval.acc->lvc.language)
			yyval.acc->lvc.language = dup_language(currentlanguage);
		}
    break;
case 46:
#line 785 "./parser.y"
{ yyval.event=NULL; }
    break;
case 47:
#line 786 "./parser.y"
{ yyval.event=add_string_event(yyvsp[-3].str, yyvsp[-1].num, yyvsp[0].num, yyvsp[-4].event); }
    break;
case 48:
#line 787 "./parser.y"
{ yyval.event=add_event(yyvsp[-3].num, yyvsp[-1].num, yyvsp[0].num, yyvsp[-4].event); }
    break;
case 49:
#line 796 "./parser.y"
{ yyval.num = 0; }
    break;
case 50:
#line 797 "./parser.y"
{ yyval.num = yyvsp[0].num; }
    break;
case 51:
#line 800 "./parser.y"
{ yyval.num = yyvsp[0].num; }
    break;
case 52:
#line 801 "./parser.y"
{ yyval.num = yyvsp[-2].num | yyvsp[0].num; }
    break;
case 53:
#line 804 "./parser.y"
{ yyval.num = WRC_AF_NOINVERT; }
    break;
case 54:
#line 805 "./parser.y"
{ yyval.num = WRC_AF_SHIFT; }
    break;
case 55:
#line 806 "./parser.y"
{ yyval.num = WRC_AF_CONTROL; }
    break;
case 56:
#line 807 "./parser.y"
{ yyval.num = WRC_AF_ALT; }
    break;
case 57:
#line 808 "./parser.y"
{ yyval.num = WRC_AF_ASCII; }
    break;
case 58:
#line 809 "./parser.y"
{ yyval.num = WRC_AF_VIRTKEY; }
    break;
case 59:
#line 815 "./parser.y"
{
		if(yyvsp[-11].iptr)
		{
			yyvsp[-3].dlg->memopt = *(yyvsp[-11].iptr);
			free(yyvsp[-11].iptr);
		}
		else
			yyvsp[-3].dlg->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
		yyvsp[-3].dlg->x = yyvsp[-10].num;
		yyvsp[-3].dlg->y = yyvsp[-8].num;
		yyvsp[-3].dlg->width = yyvsp[-6].num;
		yyvsp[-3].dlg->height = yyvsp[-4].num;
		yyvsp[-3].dlg->controls = get_control_head(yyvsp[-1].ctl);
		yyval.dlg = yyvsp[-3].dlg;
		if(!yyval.dlg->gotstyle)
		{
			yyval.dlg->style = new_style(0,0);
			yyval.dlg->style->or_mask = WS_POPUP;
			yyval.dlg->gotstyle = TRUE;
		}
		if(yyval.dlg->title)
			yyval.dlg->style->or_mask |= WS_CAPTION;
		if(yyval.dlg->font)
			yyval.dlg->style->or_mask |= DS_SETFONT;

		yyval.dlg->style->or_mask &= ~(yyval.dlg->style->and_mask);
		yyval.dlg->style->and_mask = 0;

		if(!yyval.dlg->lvc.language)
			yyval.dlg->lvc.language = dup_language(currentlanguage);
		}
    break;
case 60:
#line 849 "./parser.y"
{ yyval.dlg=new_dialog(); }
    break;
case 61:
#line 850 "./parser.y"
{ yyval.dlg=dialog_style(yyvsp[0].style,yyvsp[-2].dlg); }
    break;
case 62:
#line 851 "./parser.y"
{ yyval.dlg=dialog_exstyle(yyvsp[0].style,yyvsp[-2].dlg); }
    break;
case 63:
#line 852 "./parser.y"
{ yyval.dlg=dialog_caption(yyvsp[0].str,yyvsp[-2].dlg); }
    break;
case 64:
#line 853 "./parser.y"
{ yyval.dlg=dialog_font(yyvsp[0].fntid,yyvsp[-1].dlg); }
    break;
case 65:
#line 854 "./parser.y"
{ yyval.dlg=dialog_class(yyvsp[0].nid,yyvsp[-2].dlg); }
    break;
case 66:
#line 855 "./parser.y"
{ yyval.dlg=dialog_menu(yyvsp[0].nid,yyvsp[-2].dlg); }
    break;
case 67:
#line 856 "./parser.y"
{ yyval.dlg=dialog_language(yyvsp[0].lan,yyvsp[-1].dlg); }
    break;
case 68:
#line 857 "./parser.y"
{ yyval.dlg=dialog_characteristics(yyvsp[0].chars,yyvsp[-1].dlg); }
    break;
case 69:
#line 858 "./parser.y"
{ yyval.dlg=dialog_version(yyvsp[0].ver,yyvsp[-1].dlg); }
    break;
case 70:
#line 861 "./parser.y"
{ yyval.ctl = NULL; }
    break;
case 71:
#line 862 "./parser.y"
{ yyval.ctl=ins_ctrl(-1, 0, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 72:
#line 863 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_EDIT, 0, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 73:
#line 864 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_LISTBOX, 0, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 74:
#line 865 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_COMBOBOX, 0, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 75:
#line 866 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_SCROLLBAR, 0, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 76:
#line 867 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_BUTTON, BS_CHECKBOX, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 77:
#line 868 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_BUTTON, BS_DEFPUSHBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 78:
#line 869 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_BUTTON, BS_GROUPBOX, yyvsp[0].ctl, yyvsp[-2].ctl);}
    break;
case 79:
#line 870 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_BUTTON, BS_PUSHBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 80:
#line 872 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_BUTTON, BS_RADIOBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 81:
#line 873 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_BUTTON, BS_AUTO3STATE, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 82:
#line 874 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_BUTTON, BS_3STATE, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 83:
#line 875 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_BUTTON, BS_AUTOCHECKBOX, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 84:
#line 876 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_BUTTON, BS_AUTORADIOBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 85:
#line 877 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_STATIC, SS_LEFT, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 86:
#line 878 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_STATIC, SS_CENTER, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 87:
#line 879 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_STATIC, SS_RIGHT, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 88:
#line 881 "./parser.y"
{
		yyvsp[0].ctl->title = yyvsp[-7].nid;
		yyvsp[0].ctl->id = yyvsp[-5].num;
		yyvsp[0].ctl->x = yyvsp[-3].num;
		yyvsp[0].ctl->y = yyvsp[-1].num;
		yyval.ctl = ins_ctrl(CT_STATIC, SS_ICON, yyvsp[0].ctl, yyvsp[-9].ctl);
		}
    break;
case 89:
#line 891 "./parser.y"
{
		yyval.ctl=new_control();
		yyval.ctl->title = new_name_id();
		yyval.ctl->title->type = name_str;
		yyval.ctl->title->name.s_name = yyvsp[-11].str;
		yyval.ctl->id = yyvsp[-9].num;
		yyval.ctl->x = yyvsp[-7].num;
		yyval.ctl->y = yyvsp[-5].num;
		yyval.ctl->width = yyvsp[-3].num;
		yyval.ctl->height = yyvsp[-1].num;
		if(yyvsp[0].styles)
		{
			yyval.ctl->style = yyvsp[0].styles->style;
			yyval.ctl->gotstyle = TRUE;
			if (yyvsp[0].styles->exstyle)
			{
			    yyval.ctl->exstyle = yyvsp[0].styles->exstyle;
			    yyval.ctl->gotexstyle = TRUE;
			}
			free(yyvsp[0].styles);
		}
		}
    break;
case 90:
#line 916 "./parser.y"
{
		yyval.ctl = new_control();
		yyval.ctl->id = yyvsp[-9].num;
		yyval.ctl->x = yyvsp[-7].num;
		yyval.ctl->y = yyvsp[-5].num;
		yyval.ctl->width = yyvsp[-3].num;
		yyval.ctl->height = yyvsp[-1].num;
		if(yyvsp[0].styles)
		{
			yyval.ctl->style = yyvsp[0].styles->style;
			yyval.ctl->gotstyle = TRUE;
			if (yyvsp[0].styles->exstyle)
			{
			    yyval.ctl->exstyle = yyvsp[0].styles->exstyle;
			    yyval.ctl->gotexstyle = TRUE;
			}
			free(yyvsp[0].styles);
		}
		}
    break;
case 91:
#line 938 "./parser.y"
{ yyval.ctl = new_control(); }
    break;
case 92:
#line 940 "./parser.y"
{
		yyval.ctl = new_control();
		yyval.ctl->width = yyvsp[-2].num;
		yyval.ctl->height = yyvsp[0].num;
		}
    break;
case 93:
#line 945 "./parser.y"
{
		yyval.ctl = new_control();
		yyval.ctl->width = yyvsp[-4].num;
		yyval.ctl->height = yyvsp[-2].num;
		yyval.ctl->style = yyvsp[0].style;
		yyval.ctl->gotstyle = TRUE;
		}
    break;
case 94:
#line 952 "./parser.y"
{
		yyval.ctl = new_control();
		yyval.ctl->width = yyvsp[-6].num;
		yyval.ctl->height = yyvsp[-4].num;
		yyval.ctl->style = yyvsp[-2].style;
		yyval.ctl->gotstyle = TRUE;
		yyval.ctl->exstyle = yyvsp[0].style;
		yyval.ctl->gotexstyle = TRUE;
		}
    break;
case 95:
#line 963 "./parser.y"
{
		yyval.ctl=new_control();
		yyval.ctl->title = yyvsp[-16].nid;
		yyval.ctl->id = yyvsp[-14].num;
		yyval.ctl->ctlclass = convert_ctlclass(yyvsp[-12].nid);
		yyval.ctl->style = yyvsp[-10].style;
		yyval.ctl->gotstyle = TRUE;
		yyval.ctl->x = yyvsp[-8].num;
		yyval.ctl->y = yyvsp[-6].num;
		yyval.ctl->width = yyvsp[-4].num;
		yyval.ctl->height = yyvsp[-2].num;
		yyval.ctl->exstyle = yyvsp[0].style;
		yyval.ctl->gotexstyle = TRUE;
		}
    break;
case 96:
#line 977 "./parser.y"
{
		yyval.ctl=new_control();
		yyval.ctl->title = yyvsp[-14].nid;
		yyval.ctl->id = yyvsp[-12].num;
		yyval.ctl->ctlclass = convert_ctlclass(yyvsp[-10].nid);
		yyval.ctl->style = yyvsp[-8].style;
		yyval.ctl->gotstyle = TRUE;
		yyval.ctl->x = yyvsp[-6].num;
		yyval.ctl->y = yyvsp[-4].num;
		yyval.ctl->width = yyvsp[-2].num;
		yyval.ctl->height = yyvsp[0].num;
		}
    break;
case 97:
#line 992 "./parser.y"
{ yyval.fntid = new_font_id(yyvsp[-2].num, yyvsp[0].str, 0, 0); }
    break;
case 98:
#line 997 "./parser.y"
{ yyval.styles = NULL; }
    break;
case 99:
#line 998 "./parser.y"
{ yyval.styles = new_style_pair(yyvsp[0].style, 0); }
    break;
case 100:
#line 999 "./parser.y"
{ yyval.styles = new_style_pair(yyvsp[-2].style, yyvsp[0].style); }
    break;
case 101:
#line 1003 "./parser.y"
{ yyval.style = new_style(yyvsp[-2].style->or_mask | yyvsp[0].style->or_mask, yyvsp[-2].style->and_mask | yyvsp[0].style->and_mask); free(yyvsp[-2].style); free(yyvsp[0].style);}
    break;
case 102:
#line 1004 "./parser.y"
{ yyval.style = yyvsp[-1].style; }
    break;
case 103:
#line 1005 "./parser.y"
{ yyval.style = new_style(yyvsp[0].num, 0); }
    break;
case 104:
#line 1006 "./parser.y"
{ yyval.style = new_style(0, yyvsp[0].num); }
    break;
case 105:
#line 1010 "./parser.y"
{
		yyval.nid = new_name_id();
		yyval.nid->type = name_ord;
		yyval.nid->name.i_name = yyvsp[0].num;
		}
    break;
case 106:
#line 1015 "./parser.y"
{
		yyval.nid = new_name_id();
		yyval.nid->type = name_str;
		yyval.nid->name.s_name = yyvsp[0].str;
		}
    break;
case 107:
#line 1024 "./parser.y"
{
		if(!win32)
			yywarning("DIALOGEX not supported in 16-bit mode");
		if(yyvsp[-12].iptr)
		{
			yyvsp[-3].dlgex->memopt = *(yyvsp[-12].iptr);
			free(yyvsp[-12].iptr);
		}
		else
			yyvsp[-3].dlgex->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
		yyvsp[-3].dlgex->x = yyvsp[-11].num;
		yyvsp[-3].dlgex->y = yyvsp[-9].num;
		yyvsp[-3].dlgex->width = yyvsp[-7].num;
		yyvsp[-3].dlgex->height = yyvsp[-5].num;
		if(yyvsp[-4].iptr)
		{
			yyvsp[-3].dlgex->helpid = *(yyvsp[-4].iptr);
			yyvsp[-3].dlgex->gothelpid = TRUE;
			free(yyvsp[-4].iptr);
		}
		yyvsp[-3].dlgex->controls = get_control_head(yyvsp[-1].ctl);
		yyval.dlgex = yyvsp[-3].dlgex;

		assert(yyval.dlgex->style != NULL);
		if(!yyval.dlgex->gotstyle)
		{
			yyval.dlgex->style->or_mask = WS_POPUP;
			yyval.dlgex->gotstyle = TRUE;
		}
		if(yyval.dlgex->title)
			yyval.dlgex->style->or_mask |= WS_CAPTION;
		if(yyval.dlgex->font)
			yyval.dlgex->style->or_mask |= DS_SETFONT;

		yyval.dlgex->style->or_mask &= ~(yyval.dlgex->style->and_mask);
		yyval.dlgex->style->and_mask = 0;

		if(!yyval.dlgex->lvc.language)
			yyval.dlgex->lvc.language = dup_language(currentlanguage);
		}
    break;
case 108:
#line 1067 "./parser.y"
{ yyval.dlgex=new_dialogex(); }
    break;
case 109:
#line 1068 "./parser.y"
{ yyval.dlgex=dialogex_style(yyvsp[0].style,yyvsp[-2].dlgex); }
    break;
case 110:
#line 1069 "./parser.y"
{ yyval.dlgex=dialogex_exstyle(yyvsp[0].style,yyvsp[-2].dlgex); }
    break;
case 111:
#line 1070 "./parser.y"
{ yyval.dlgex=dialogex_caption(yyvsp[0].str,yyvsp[-2].dlgex); }
    break;
case 112:
#line 1071 "./parser.y"
{ yyval.dlgex=dialogex_font(yyvsp[0].fntid,yyvsp[-1].dlgex); }
    break;
case 113:
#line 1072 "./parser.y"
{ yyval.dlgex=dialogex_font(yyvsp[0].fntid,yyvsp[-1].dlgex); }
    break;
case 114:
#line 1073 "./parser.y"
{ yyval.dlgex=dialogex_class(yyvsp[0].nid,yyvsp[-2].dlgex); }
    break;
case 115:
#line 1074 "./parser.y"
{ yyval.dlgex=dialogex_menu(yyvsp[0].nid,yyvsp[-2].dlgex); }
    break;
case 116:
#line 1075 "./parser.y"
{ yyval.dlgex=dialogex_language(yyvsp[0].lan,yyvsp[-1].dlgex); }
    break;
case 117:
#line 1076 "./parser.y"
{ yyval.dlgex=dialogex_characteristics(yyvsp[0].chars,yyvsp[-1].dlgex); }
    break;
case 118:
#line 1077 "./parser.y"
{ yyval.dlgex=dialogex_version(yyvsp[0].ver,yyvsp[-1].dlgex); }
    break;
case 119:
#line 1080 "./parser.y"
{ yyval.ctl = NULL; }
    break;
case 120:
#line 1081 "./parser.y"
{ yyval.ctl=ins_ctrl(-1, 0, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 121:
#line 1082 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_EDIT, 0, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 122:
#line 1083 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_LISTBOX, 0, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 123:
#line 1084 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_COMBOBOX, 0, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 124:
#line 1085 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_SCROLLBAR, 0, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 125:
#line 1086 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_BUTTON, BS_CHECKBOX, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 126:
#line 1087 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_BUTTON, BS_DEFPUSHBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 127:
#line 1088 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_BUTTON, BS_GROUPBOX, yyvsp[0].ctl, yyvsp[-2].ctl);}
    break;
case 128:
#line 1089 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_BUTTON, BS_PUSHBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 129:
#line 1091 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_BUTTON, BS_RADIOBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 130:
#line 1092 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_BUTTON, BS_AUTO3STATE, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 131:
#line 1093 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_BUTTON, BS_3STATE, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 132:
#line 1094 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_BUTTON, BS_AUTOCHECKBOX, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 133:
#line 1095 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_BUTTON, BS_AUTORADIOBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 134:
#line 1096 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_STATIC, SS_LEFT, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 135:
#line 1097 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_STATIC, SS_CENTER, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 136:
#line 1098 "./parser.y"
{ yyval.ctl=ins_ctrl(CT_STATIC, SS_RIGHT, yyvsp[0].ctl, yyvsp[-2].ctl); }
    break;
case 137:
#line 1100 "./parser.y"
{
		yyvsp[0].ctl->title = yyvsp[-7].nid;
		yyvsp[0].ctl->id = yyvsp[-5].num;
		yyvsp[0].ctl->x = yyvsp[-3].num;
		yyvsp[0].ctl->y = yyvsp[-1].num;
		yyval.ctl = ins_ctrl(CT_STATIC, SS_ICON, yyvsp[0].ctl, yyvsp[-9].ctl);
		}
    break;
case 138:
#line 1111 "./parser.y"
{
		yyval.ctl=new_control();
		yyval.ctl->title = yyvsp[-18].nid;
		yyval.ctl->id = yyvsp[-16].num;
		yyval.ctl->ctlclass = convert_ctlclass(yyvsp[-14].nid);
		yyval.ctl->style = yyvsp[-12].style;
		yyval.ctl->gotstyle = TRUE;
		yyval.ctl->x = yyvsp[-10].num;
		yyval.ctl->y = yyvsp[-8].num;
		yyval.ctl->width = yyvsp[-6].num;
		yyval.ctl->height = yyvsp[-4].num;
		if(yyvsp[-2].style)
		{
			yyval.ctl->exstyle = yyvsp[-2].style;
			yyval.ctl->gotexstyle = TRUE;
		}
		if(yyvsp[-1].iptr)
		{
			yyval.ctl->helpid = *(yyvsp[-1].iptr);
			yyval.ctl->gothelpid = TRUE;
			free(yyvsp[-1].iptr);
		}
		yyval.ctl->extra = yyvsp[0].raw;
		}
    break;
case 139:
#line 1135 "./parser.y"
{
		yyval.ctl=new_control();
		yyval.ctl->title = yyvsp[-15].nid;
		yyval.ctl->id = yyvsp[-13].num;
		yyval.ctl->style = yyvsp[-9].style;
		yyval.ctl->gotstyle = TRUE;
		yyval.ctl->ctlclass = convert_ctlclass(yyvsp[-11].nid);
		yyval.ctl->x = yyvsp[-7].num;
		yyval.ctl->y = yyvsp[-5].num;
		yyval.ctl->width = yyvsp[-3].num;
		yyval.ctl->height = yyvsp[-1].num;
		yyval.ctl->extra = yyvsp[0].raw;
		}
    break;
case 140:
#line 1151 "./parser.y"
{
		yyval.ctl=new_control();
		yyval.ctl->title = new_name_id();
		yyval.ctl->title->type = name_str;
		yyval.ctl->title->name.s_name = yyvsp[-13].str;
		yyval.ctl->id = yyvsp[-11].num;
		yyval.ctl->x = yyvsp[-9].num;
		yyval.ctl->y = yyvsp[-7].num;
		yyval.ctl->width = yyvsp[-5].num;
		yyval.ctl->height = yyvsp[-3].num;
		if(yyvsp[-2].styles)
		{
			yyval.ctl->style = yyvsp[-2].styles->style;
			yyval.ctl->gotstyle = TRUE;

			if (yyvsp[-2].styles->exstyle)
			{
			    yyval.ctl->exstyle = yyvsp[-2].styles->exstyle;
			    yyval.ctl->gotexstyle = TRUE;
			}
			free(yyvsp[-2].styles);
		}

		yyval.ctl->extra = yyvsp[0].raw;
		}
    break;
case 141:
#line 1179 "./parser.y"
{
		yyval.ctl = new_control();
		yyval.ctl->id = yyvsp[-11].num;
		yyval.ctl->x = yyvsp[-9].num;
		yyval.ctl->y = yyvsp[-7].num;
		yyval.ctl->width = yyvsp[-5].num;
		yyval.ctl->height = yyvsp[-3].num;
		if(yyvsp[-2].styles)
		{
			yyval.ctl->style = yyvsp[-2].styles->style;
			yyval.ctl->gotstyle = TRUE;

			if (yyvsp[-2].styles->exstyle)
			{
			    yyval.ctl->exstyle = yyvsp[-2].styles->exstyle;
			    yyval.ctl->gotexstyle = TRUE;
			}
			free(yyvsp[-2].styles);
		}
		yyval.ctl->extra = yyvsp[0].raw;
		}
    break;
case 142:
#line 1202 "./parser.y"
{ yyval.raw = NULL; }
    break;
case 143:
#line 1203 "./parser.y"
{ yyval.raw = yyvsp[0].raw; }
    break;
case 144:
#line 1206 "./parser.y"
{ yyval.iptr = NULL; }
    break;
case 145:
#line 1207 "./parser.y"
{ yyval.iptr = new_int(yyvsp[0].num); }
    break;
case 146:
#line 1211 "./parser.y"
{ yyval.fntid = new_font_id(yyvsp[-7].num, yyvsp[-5].str, yyvsp[-3].num, yyvsp[-1].num); }
    break;
case 147:
#line 1218 "./parser.y"
{ yyval.fntid = NULL; }
    break;
case 148:
#line 1219 "./parser.y"
{ yyval.fntid = NULL; }
    break;
case 149:
#line 1223 "./parser.y"
{
		if(!yyvsp[0].menitm)
			yyerror("Menu must contain items");
		yyval.men = new_menu();
		if(yyvsp[-2].iptr)
		{
			yyval.men->memopt = *(yyvsp[-2].iptr);
			free(yyvsp[-2].iptr);
		}
		else
			yyval.men->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
		yyval.men->items = get_item_head(yyvsp[0].menitm);
		if(yyvsp[-1].lvc)
		{
			yyval.men->lvc = *(yyvsp[-1].lvc);
			free(yyvsp[-1].lvc);
		}
		if(!yyval.men->lvc.language)
			yyval.men->lvc.language = dup_language(currentlanguage);
		}
    break;
case 150:
#line 1246 "./parser.y"
{ yyval.menitm = yyvsp[-1].menitm; }
    break;
case 151:
#line 1250 "./parser.y"
{yyval.menitm = NULL;}
    break;
case 152:
#line 1251 "./parser.y"
{
		yyval.menitm=new_menu_item();
		yyval.menitm->prev = yyvsp[-5].menitm;
		if(yyvsp[-5].menitm)
			yyvsp[-5].menitm->next = yyval.menitm;
		yyval.menitm->id =  yyvsp[-1].num;
		yyval.menitm->state = yyvsp[0].num;
		yyval.menitm->name = yyvsp[-3].str;
		}
    break;
case 153:
#line 1260 "./parser.y"
{
		yyval.menitm=new_menu_item();
		yyval.menitm->prev = yyvsp[-2].menitm;
		if(yyvsp[-2].menitm)
			yyvsp[-2].menitm->next = yyval.menitm;
		}
    break;
case 154:
#line 1266 "./parser.y"
{
		yyval.menitm = new_menu_item();
		yyval.menitm->prev = yyvsp[-4].menitm;
		if(yyvsp[-4].menitm)
			yyvsp[-4].menitm->next = yyval.menitm;
		yyval.menitm->popup = get_item_head(yyvsp[0].menitm);
		yyval.menitm->name = yyvsp[-2].str;
		}
    break;
case 155:
#line 1285 "./parser.y"
{ yyval.num = 0; }
    break;
case 156:
#line 1286 "./parser.y"
{ yyval.num = yyvsp[0].num | MF_CHECKED; }
    break;
case 157:
#line 1287 "./parser.y"
{ yyval.num = yyvsp[0].num | MF_GRAYED; }
    break;
case 158:
#line 1288 "./parser.y"
{ yyval.num = yyvsp[0].num | MF_HELP; }
    break;
case 159:
#line 1289 "./parser.y"
{ yyval.num = yyvsp[0].num | MF_DISABLED; }
    break;
case 160:
#line 1290 "./parser.y"
{ yyval.num = yyvsp[0].num | MF_MENUBARBREAK; }
    break;
case 161:
#line 1291 "./parser.y"
{ yyval.num = yyvsp[0].num | MF_MENUBREAK; }
    break;
case 162:
#line 1295 "./parser.y"
{
		if(!win32)
			yywarning("MENUEX not supported in 16-bit mode");
		if(!yyvsp[0].menexitm)
			yyerror("MenuEx must contain items");
		yyval.menex = new_menuex();
		if(yyvsp[-2].iptr)
		{
			yyval.menex->memopt = *(yyvsp[-2].iptr);
			free(yyvsp[-2].iptr);
		}
		else
			yyval.menex->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
		yyval.menex->items = get_itemex_head(yyvsp[0].menexitm);
		if(yyvsp[-1].lvc)
		{
			yyval.menex->lvc = *(yyvsp[-1].lvc);
			free(yyvsp[-1].lvc);
		}
		if(!yyval.menex->lvc.language)
			yyval.menex->lvc.language = dup_language(currentlanguage);
		}
    break;
case 163:
#line 1320 "./parser.y"
{ yyval.menexitm = yyvsp[-1].menexitm; }
    break;
case 164:
#line 1324 "./parser.y"
{yyval.menexitm = NULL; }
    break;
case 165:
#line 1325 "./parser.y"
{
		yyval.menexitm = new_menuex_item();
		yyval.menexitm->prev = yyvsp[-3].menexitm;
		if(yyvsp[-3].menexitm)
			yyvsp[-3].menexitm->next = yyval.menexitm;
		yyval.menexitm->name = yyvsp[-1].str;
		yyval.menexitm->id = yyvsp[0].exopt->id;
		yyval.menexitm->type = yyvsp[0].exopt->type;
		yyval.menexitm->state = yyvsp[0].exopt->state;
		yyval.menexitm->helpid = yyvsp[0].exopt->helpid;
		yyval.menexitm->gotid = yyvsp[0].exopt->gotid;
		yyval.menexitm->gottype = yyvsp[0].exopt->gottype;
		yyval.menexitm->gotstate = yyvsp[0].exopt->gotstate;
		yyval.menexitm->gothelpid = yyvsp[0].exopt->gothelpid;
		free(yyvsp[0].exopt);
		}
    break;
case 166:
#line 1341 "./parser.y"
{
		yyval.menexitm = new_menuex_item();
		yyval.menexitm->prev = yyvsp[-2].menexitm;
		if(yyvsp[-2].menexitm)
			yyvsp[-2].menexitm->next = yyval.menexitm;
		}
    break;
case 167:
#line 1347 "./parser.y"
{
		yyval.menexitm = new_menuex_item();
		yyval.menexitm->prev = yyvsp[-4].menexitm;
		if(yyvsp[-4].menexitm)
			yyvsp[-4].menexitm->next = yyval.menexitm;
		yyval.menexitm->popup = get_itemex_head(yyvsp[0].menexitm);
		yyval.menexitm->name = yyvsp[-2].str;
		yyval.menexitm->id = yyvsp[-1].exopt->id;
		yyval.menexitm->type = yyvsp[-1].exopt->type;
		yyval.menexitm->state = yyvsp[-1].exopt->state;
		yyval.menexitm->helpid = yyvsp[-1].exopt->helpid;
		yyval.menexitm->gotid = yyvsp[-1].exopt->gotid;
		yyval.menexitm->gottype = yyvsp[-1].exopt->gottype;
		yyval.menexitm->gotstate = yyvsp[-1].exopt->gotstate;
		yyval.menexitm->gothelpid = yyvsp[-1].exopt->gothelpid;
		free(yyvsp[-1].exopt);
		}
    break;
case 168:
#line 1367 "./parser.y"
{ yyval.exopt = new_itemex_opt(0, 0, 0, 0); }
    break;
case 169:
#line 1368 "./parser.y"
{
		yyval.exopt = new_itemex_opt(yyvsp[0].num, 0, 0, 0);
		yyval.exopt->gotid = TRUE;
		}
    break;
case 170:
#line 1372 "./parser.y"
{
		yyval.exopt = new_itemex_opt(yyvsp[-3].iptr ? *(yyvsp[-3].iptr) : 0, yyvsp[-1].iptr ? *(yyvsp[-1].iptr) : 0, yyvsp[0].num, 0);
		yyval.exopt->gotid = TRUE;
		yyval.exopt->gottype = TRUE;
		yyval.exopt->gotstate = TRUE;
		if(yyvsp[-3].iptr) free(yyvsp[-3].iptr);
		if(yyvsp[-1].iptr) free(yyvsp[-1].iptr);
		}
    break;
case 171:
#line 1380 "./parser.y"
{
		yyval.exopt = new_itemex_opt(yyvsp[-4].iptr ? *(yyvsp[-4].iptr) : 0, yyvsp[-2].iptr ? *(yyvsp[-2].iptr) : 0, yyvsp[0].num, 0);
		yyval.exopt->gotid = TRUE;
		yyval.exopt->gottype = TRUE;
		yyval.exopt->gotstate = TRUE;
		if(yyvsp[-4].iptr) free(yyvsp[-4].iptr);
		if(yyvsp[-2].iptr) free(yyvsp[-2].iptr);
		}
    break;
case 172:
#line 1391 "./parser.y"
{ yyval.exopt = new_itemex_opt(0, 0, 0, 0); }
    break;
case 173:
#line 1392 "./parser.y"
{
		yyval.exopt = new_itemex_opt(yyvsp[0].num, 0, 0, 0);
		yyval.exopt->gotid = TRUE;
		}
    break;
case 174:
#line 1396 "./parser.y"
{
		yyval.exopt = new_itemex_opt(yyvsp[-2].iptr ? *(yyvsp[-2].iptr) : 0, yyvsp[0].num, 0, 0);
		if(yyvsp[-2].iptr) free(yyvsp[-2].iptr);
		yyval.exopt->gotid = TRUE;
		yyval.exopt->gottype = TRUE;
		}
    break;
case 175:
#line 1402 "./parser.y"
{
		yyval.exopt = new_itemex_opt(yyvsp[-4].iptr ? *(yyvsp[-4].iptr) : 0, yyvsp[-2].iptr ? *(yyvsp[-2].iptr) : 0, yyvsp[0].num, 0);
		if(yyvsp[-4].iptr) free(yyvsp[-4].iptr);
		if(yyvsp[-2].iptr) free(yyvsp[-2].iptr);
		yyval.exopt->gotid = TRUE;
		yyval.exopt->gottype = TRUE;
		yyval.exopt->gotstate = TRUE;
		}
    break;
case 176:
#line 1410 "./parser.y"
{
		yyval.exopt = new_itemex_opt(yyvsp[-6].iptr ? *(yyvsp[-6].iptr) : 0, yyvsp[-4].iptr ? *(yyvsp[-4].iptr) : 0, yyvsp[-2].iptr ? *(yyvsp[-2].iptr) : 0, yyvsp[0].num);
		if(yyvsp[-6].iptr) free(yyvsp[-6].iptr);
		if(yyvsp[-4].iptr) free(yyvsp[-4].iptr);
		if(yyvsp[-2].iptr) free(yyvsp[-2].iptr);
		yyval.exopt->gotid = TRUE;
		yyval.exopt->gottype = TRUE;
		yyval.exopt->gotstate = TRUE;
		yyval.exopt->gothelpid = TRUE;
		}
    break;
case 177:
#line 1430 "./parser.y"
{
		if(!yyvsp[-1].stt)
		{
			yyerror("Stringtable must have at least one entry");
		}
		else
		{
			stringtable_t *stt;
			/* Check if we added to a language table or created
			 * a new one.
			 */
			 for(stt = sttres; stt; stt = stt->next)
			 {
				if(stt == tagstt)
					break;
			 }
			 if(!stt)
			 {
				/* It is a new one */
				if(sttres)
				{
					sttres->prev = tagstt;
					tagstt->next = sttres;
					sttres = tagstt;
				}
				else
					sttres = tagstt;
			 }
			 /* Else were done */
		}
		if(tagstt_memopt)
		{
			free(tagstt_memopt);
			tagstt_memopt = NULL;
		}

		yyval.stt = tagstt;
		}
    break;
case 178:
#line 1471 "./parser.y"
{
		if((tagstt = find_stringtable(yyvsp[0].lvc)) == NULL)
			tagstt = new_stringtable(yyvsp[0].lvc);
		tagstt_memopt = yyvsp[-1].iptr;
		tagstt_version = yyvsp[0].lvc->version;
		tagstt_characts = yyvsp[0].lvc->characts;
		if(yyvsp[0].lvc)
			free(yyvsp[0].lvc);
		}
    break;
case 179:
#line 1482 "./parser.y"
{ yyval.stt = NULL; }
    break;
case 180:
#line 1483 "./parser.y"
{
		int i;
		assert(tagstt != NULL);
		if(yyvsp[-2].num > 65535 || yyvsp[-2].num < -32768)
			yyerror("Stringtable entry's ID out of range (%d)", yyvsp[-2].num);
		/* Search for the ID */
		for(i = 0; i < tagstt->nentries; i++)
		{
			if(tagstt->entries[i].id == yyvsp[-2].num)
				yyerror("Stringtable ID %d already in use", yyvsp[-2].num);
		}
		/* If we get here, then we have a new unique entry */
		tagstt->nentries++;
		tagstt->entries = xrealloc(tagstt->entries, sizeof(tagstt->entries[0]) * tagstt->nentries);
		tagstt->entries[tagstt->nentries-1].id = yyvsp[-2].num;
		tagstt->entries[tagstt->nentries-1].str = yyvsp[0].str;
		if(tagstt_memopt)
			tagstt->entries[tagstt->nentries-1].memopt = *tagstt_memopt;
		else
			tagstt->entries[tagstt->nentries-1].memopt = WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE | WRC_MO_PURE;
		tagstt->entries[tagstt->nentries-1].version = tagstt_version;
		tagstt->entries[tagstt->nentries-1].characts = tagstt_characts;

		if(pedantic && !yyvsp[0].str->size)
			yywarning("Zero length strings make no sense");
		if(!win32 && yyvsp[0].str->size > 254)
			yyerror("Stringtable entry more than 254 characters");
		if(win32 && yyvsp[0].str->size > 65534) /* Hmm..., does this happen? */
			yyerror("Stringtable entry more than 65534 characters (probably something else that went wrong)");
		yyval.stt = tagstt;
		}
    break;
case 183:
#line 1523 "./parser.y"
{
		yyval.veri = yyvsp[-3].veri;
		if(yyvsp[-4].iptr)
		{
			yyval.veri->memopt = *(yyvsp[-4].iptr);
			free(yyvsp[-4].iptr);
		}
		else
			yyval.veri->memopt = WRC_MO_MOVEABLE | (win32 ? WRC_MO_PURE : 0);
		yyval.veri->blocks = get_ver_block_head(yyvsp[-1].blk);
		/* Set language; there is no version or characteristics */
		yyval.veri->lvc.language = dup_language(currentlanguage);
		}
    break;
case 184:
#line 1539 "./parser.y"
{ yyval.veri = new_versioninfo(); }
    break;
case 185:
#line 1540 "./parser.y"
{
		if(yyvsp[-8].veri->gotit.fv)
			yyerror("FILEVERSION already defined");
		yyval.veri = yyvsp[-8].veri;
		yyval.veri->filever_maj1 = yyvsp[-6].num;
		yyval.veri->filever_maj2 = yyvsp[-4].num;
		yyval.veri->filever_min1 = yyvsp[-2].num;
		yyval.veri->filever_min2 = yyvsp[0].num;
		yyval.veri->gotit.fv = 1;
		}
    break;
case 186:
#line 1550 "./parser.y"
{
		if(yyvsp[-8].veri->gotit.pv)
			yyerror("PRODUCTVERSION already defined");
		yyval.veri = yyvsp[-8].veri;
		yyval.veri->prodver_maj1 = yyvsp[-6].num;
		yyval.veri->prodver_maj2 = yyvsp[-4].num;
		yyval.veri->prodver_min1 = yyvsp[-2].num;
		yyval.veri->prodver_min2 = yyvsp[0].num;
		yyval.veri->gotit.pv = 1;
		}
    break;
case 187:
#line 1560 "./parser.y"
{
		if(yyvsp[-2].veri->gotit.ff)
			yyerror("FILEFLAGS already defined");
		yyval.veri = yyvsp[-2].veri;
		yyval.veri->fileflags = yyvsp[0].num;
		yyval.veri->gotit.ff = 1;
		}
    break;
case 188:
#line 1567 "./parser.y"
{
		if(yyvsp[-2].veri->gotit.ffm)
			yyerror("FILEFLAGSMASK already defined");
		yyval.veri = yyvsp[-2].veri;
		yyval.veri->fileflagsmask = yyvsp[0].num;
		yyval.veri->gotit.ffm = 1;
		}
    break;
case 189:
#line 1574 "./parser.y"
{
		if(yyvsp[-2].veri->gotit.fo)
			yyerror("FILEOS already defined");
		yyval.veri = yyvsp[-2].veri;
		yyval.veri->fileos = yyvsp[0].num;
		yyval.veri->gotit.fo = 1;
		}
    break;
case 190:
#line 1581 "./parser.y"
{
		if(yyvsp[-2].veri->gotit.ft)
			yyerror("FILETYPE already defined");
		yyval.veri = yyvsp[-2].veri;
		yyval.veri->filetype = yyvsp[0].num;
		yyval.veri->gotit.ft = 1;
		}
    break;
case 191:
#line 1588 "./parser.y"
{
		if(yyvsp[-2].veri->gotit.fst)
			yyerror("FILESUBTYPE already defined");
		yyval.veri = yyvsp[-2].veri;
		yyval.veri->filesubtype = yyvsp[0].num;
		yyval.veri->gotit.fst = 1;
		}
    break;
case 192:
#line 1598 "./parser.y"
{ yyval.blk = NULL; }
    break;
case 193:
#line 1599 "./parser.y"
{
		yyval.blk = yyvsp[0].blk;
		yyval.blk->prev = yyvsp[-1].blk;
		if(yyvsp[-1].blk)
			yyvsp[-1].blk->next = yyval.blk;
		}
    break;
case 194:
#line 1608 "./parser.y"
{
		yyval.blk = new_ver_block();
		yyval.blk->name = yyvsp[-3].str;
		yyval.blk->values = get_ver_value_head(yyvsp[-1].val);
		}
    break;
case 195:
#line 1616 "./parser.y"
{ yyval.val = NULL; }
    break;
case 196:
#line 1617 "./parser.y"
{
		yyval.val = yyvsp[0].val;
		yyval.val->prev = yyvsp[-1].val;
		if(yyvsp[-1].val)
			yyvsp[-1].val->next = yyval.val;
		}
    break;
case 197:
#line 1626 "./parser.y"
{
		yyval.val = new_ver_value();
		yyval.val->type = val_block;
		yyval.val->value.block = yyvsp[0].blk;
		}
    break;
case 198:
#line 1631 "./parser.y"
{
		yyval.val = new_ver_value();
		yyval.val->type = val_str;
		yyval.val->key = yyvsp[-2].str;
		yyval.val->value.str = yyvsp[0].str;
		}
    break;
case 199:
#line 1637 "./parser.y"
{
		yyval.val = new_ver_value();
		yyval.val->type = val_words;
		yyval.val->key = yyvsp[-2].str;
		yyval.val->value.words = yyvsp[0].verw;
		}
    break;
case 200:
#line 1646 "./parser.y"
{ yyval.verw = new_ver_words(yyvsp[0].num); }
    break;
case 201:
#line 1647 "./parser.y"
{ yyval.verw = add_ver_words(yyvsp[-2].verw, yyvsp[0].num); }
    break;
case 202:
#line 1651 "./parser.y"
{
		int nitems;
		toolbar_item_t *items = get_tlbr_buttons_head(yyvsp[-1].tlbarItems, &nitems);
		yyval.tlbar = new_toolbar(yyvsp[-6].num, yyvsp[-4].num, items, nitems);
		if(yyvsp[-7].iptr)
		{
			yyval.tlbar->memopt = *(yyvsp[-7].iptr);
			free(yyvsp[-7].iptr);
		}
		else
		{
			yyval.tlbar->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE;
		}
		if(yyvsp[-3].lvc)
		{
			yyval.tlbar->lvc = *(yyvsp[-3].lvc);
			free(yyvsp[-3].lvc);
		}
		if(!yyval.tlbar->lvc.language)
		{
			yyval.tlbar->lvc.language = dup_language(currentlanguage);
		}
		}
    break;
case 203:
#line 1677 "./parser.y"
{ yyval.tlbarItems = NULL; }
    break;
case 204:
#line 1678 "./parser.y"
{
		toolbar_item_t *idrec = new_toolbar_item();
		idrec->id = yyvsp[0].num;
		yyval.tlbarItems = ins_tlbr_button(yyvsp[-2].tlbarItems, idrec);
		}
    break;
case 205:
#line 1683 "./parser.y"
{
		toolbar_item_t *idrec = new_toolbar_item();
		idrec->id = 0;
		yyval.tlbarItems = ins_tlbr_button(yyvsp[-1].tlbarItems, idrec);
	}
    break;
case 206:
#line 1692 "./parser.y"
{ yyval.iptr = NULL; }
    break;
case 207:
#line 1693 "./parser.y"
{
		if(yyvsp[-1].iptr)
		{
			*(yyvsp[-1].iptr) |= *(yyvsp[0].iptr);
			yyval.iptr = yyvsp[-1].iptr;
			free(yyvsp[0].iptr);
		}
		else
			yyval.iptr = yyvsp[0].iptr;
		}
    break;
case 208:
#line 1703 "./parser.y"
{
		if(yyvsp[-1].iptr)
		{
			*(yyvsp[-1].iptr) &= *(yyvsp[0].iptr);
			yyval.iptr = yyvsp[-1].iptr;
			free(yyvsp[0].iptr);
		}
		else
		{
			*yyvsp[0].iptr &= WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE | WRC_MO_PURE;
			yyval.iptr = yyvsp[0].iptr;
		}
		}
    break;
case 209:
#line 1718 "./parser.y"
{ yyval.iptr = new_int(WRC_MO_PRELOAD); }
    break;
case 210:
#line 1719 "./parser.y"
{ yyval.iptr = new_int(WRC_MO_MOVEABLE); }
    break;
case 211:
#line 1720 "./parser.y"
{ yyval.iptr = new_int(WRC_MO_DISCARDABLE); }
    break;
case 212:
#line 1721 "./parser.y"
{ yyval.iptr = new_int(WRC_MO_PURE); }
    break;
case 213:
#line 1724 "./parser.y"
{ yyval.iptr = new_int(~WRC_MO_PRELOAD); }
    break;
case 214:
#line 1725 "./parser.y"
{ yyval.iptr = new_int(~WRC_MO_MOVEABLE); }
    break;
case 215:
#line 1726 "./parser.y"
{ yyval.iptr = new_int(~WRC_MO_PURE); }
    break;
case 216:
#line 1730 "./parser.y"
{ yyval.lvc = new_lvc(); }
    break;
case 217:
#line 1731 "./parser.y"
{
		if(!win32)
			yywarning("LANGUAGE not supported in 16-bit mode");
		if(yyvsp[-1].lvc->language)
			yyerror("Language already defined");
		yyval.lvc = yyvsp[-1].lvc;
		yyvsp[-1].lvc->language = yyvsp[0].lan;
		}
    break;
case 218:
#line 1739 "./parser.y"
{
		if(!win32)
			yywarning("CHARACTERISTICS not supported in 16-bit mode");
		if(yyvsp[-1].lvc->characts)
			yyerror("Characteristics already defined");
		yyval.lvc = yyvsp[-1].lvc;
		yyvsp[-1].lvc->characts = yyvsp[0].chars;
		}
    break;
case 219:
#line 1747 "./parser.y"
{
		if(!win32)
			yywarning("VERSION not supported in 16-bit mode");
		if(yyvsp[-1].lvc->version)
			yyerror("Version already defined");
		yyval.lvc = yyvsp[-1].lvc;
		yyvsp[-1].lvc->version = yyvsp[0].ver;
		}
    break;
case 220:
#line 1765 "./parser.y"
{ yyval.lan = new_language(yyvsp[-2].num, yyvsp[0].num);
					  if (get_language_codepage(yyvsp[-2].num, yyvsp[0].num) == -1)
						yyerror( "Language %04x is not supported", (yyvsp[0].num<<10) + yyvsp[-2].num);
					}
    break;
case 221:
#line 1772 "./parser.y"
{ yyval.chars = new_characts(yyvsp[0].num); }
    break;
case 222:
#line 1776 "./parser.y"
{ yyval.ver = new_version(yyvsp[0].num); }
    break;
case 223:
#line 1780 "./parser.y"
{
		if(yyvsp[-3].lvc)
		{
			yyvsp[-1].raw->lvc = *(yyvsp[-3].lvc);
			free(yyvsp[-3].lvc);
		}

		if(!yyvsp[-1].raw->lvc.language)
			yyvsp[-1].raw->lvc.language = dup_language(currentlanguage);

		yyval.raw = yyvsp[-1].raw;
		}
    break;
case 224:
#line 1795 "./parser.y"
{ yyval.raw = yyvsp[0].raw; }
    break;
case 225:
#line 1796 "./parser.y"
{ yyval.raw = int2raw_data(yyvsp[0].num); }
    break;
case 226:
#line 1797 "./parser.y"
{ yyval.raw = int2raw_data(-(yyvsp[0].num)); }
    break;
case 227:
#line 1798 "./parser.y"
{ yyval.raw = long2raw_data(yyvsp[0].num); }
    break;
case 228:
#line 1799 "./parser.y"
{ yyval.raw = long2raw_data(-(yyvsp[0].num)); }
    break;
case 229:
#line 1800 "./parser.y"
{ yyval.raw = str2raw_data(yyvsp[0].str); }
    break;
case 230:
#line 1801 "./parser.y"
{ yyval.raw = merge_raw_data(yyvsp[-2].raw, yyvsp[0].raw); free(yyvsp[0].raw->data); free(yyvsp[0].raw); }
    break;
case 231:
#line 1802 "./parser.y"
{ yyval.raw = merge_raw_data_int(yyvsp[-2].raw, yyvsp[0].num); }
    break;
case 232:
#line 1803 "./parser.y"
{ yyval.raw = merge_raw_data_int(yyvsp[-3].raw, -(yyvsp[0].num)); }
    break;
case 233:
#line 1804 "./parser.y"
{ yyval.raw = merge_raw_data_long(yyvsp[-2].raw, yyvsp[0].num); }
    break;
case 234:
#line 1805 "./parser.y"
{ yyval.raw = merge_raw_data_long(yyvsp[-3].raw, -(yyvsp[0].num)); }
    break;
case 235:
#line 1806 "./parser.y"
{ yyval.raw = merge_raw_data_str(yyvsp[-2].raw, yyvsp[0].str); }
    break;
case 236:
#line 1810 "./parser.y"
{ yyval.raw = load_file(yyvsp[0].str,dup_language(currentlanguage)); }
    break;
case 237:
#line 1811 "./parser.y"
{ yyval.raw = yyvsp[0].raw; }
    break;
case 238:
#line 1818 "./parser.y"
{ yyval.iptr = 0; }
    break;
case 239:
#line 1819 "./parser.y"
{ yyval.iptr = new_int(yyvsp[0].num); }
    break;
case 240:
#line 1823 "./parser.y"
{ yyval.num = (yyvsp[0].num); }
    break;
case 241:
#line 1826 "./parser.y"
{ yyval.num = (yyvsp[-2].num) + (yyvsp[0].num); }
    break;
case 242:
#line 1827 "./parser.y"
{ yyval.num = (yyvsp[-2].num) - (yyvsp[0].num); }
    break;
case 243:
#line 1828 "./parser.y"
{ yyval.num = (yyvsp[-2].num) | (yyvsp[0].num); }
    break;
case 244:
#line 1829 "./parser.y"
{ yyval.num = (yyvsp[-2].num) & (yyvsp[0].num); }
    break;
case 245:
#line 1830 "./parser.y"
{ yyval.num = (yyvsp[-2].num) * (yyvsp[0].num); }
    break;
case 246:
#line 1831 "./parser.y"
{ yyval.num = (yyvsp[-2].num) / (yyvsp[0].num); }
    break;
case 247:
#line 1832 "./parser.y"
{ yyval.num = (yyvsp[-2].num) ^ (yyvsp[0].num); }
    break;
case 248:
#line 1833 "./parser.y"
{ yyval.num = ~(yyvsp[0].num); }
    break;
case 249:
#line 1834 "./parser.y"
{ yyval.num = -(yyvsp[0].num); }
    break;
case 250:
#line 1835 "./parser.y"
{ yyval.num = yyvsp[0].num; }
    break;
case 251:
#line 1836 "./parser.y"
{ yyval.num = yyvsp[-1].num; }
    break;
case 252:
#line 1837 "./parser.y"
{ yyval.num = yyvsp[0].num; }
    break;
case 253:
#line 1838 "./parser.y"
{ yyval.num = ~(yyvsp[0].num); }
    break;
case 254:
#line 1841 "./parser.y"
{ yyval.num = yyvsp[0].num; }
    break;
case 255:
#line 1842 "./parser.y"
{ yyval.num = yyvsp[0].num; }
    break;
}

#line 705 "/usr/share/bison/bison.simple"


  yyvsp -= yylen;
  yyssp -= yylen;
#if YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;
#if YYLSP_NEEDED
  *++yylsp = yyloc;
#endif

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("parse error, unexpected ") + 1;
	  yysize += yystrlen (yytname[YYTRANSLATE (yychar)]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "parse error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[YYTRANSLATE (yychar)]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exhausted");
	}
      else
#endif /* defined (YYERROR_VERBOSE) */
	yyerror ("parse error");
    }
  goto yyerrlab1;


/*--------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action |
`--------------------------------------------------*/
yyerrlab1:
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;
      YYDPRINTF ((stderr, "Discarding token %d (%s).\n",
		  yychar, yytname[yychar1]));
      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;


/*-------------------------------------------------------------------.
| yyerrdefault -- current state does not do anything special for the |
| error token.                                                       |
`-------------------------------------------------------------------*/
yyerrdefault:
#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */

  /* If its default is to accept any token, ok.  Otherwise pop it.  */
  yyn = yydefact[yystate];
  if (yyn)
    goto yydefault;
#endif


/*---------------------------------------------------------------.
| yyerrpop -- pop the current state because it cannot handle the |
| error token                                                    |
`---------------------------------------------------------------*/
yyerrpop:
  if (yyssp == yyss)
    YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#if YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "Error: state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

/*--------------.
| yyerrhandle.  |
`--------------*/
yyerrhandle:
  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

/*---------------------------------------------.
| yyoverflowab -- parser overflow comes here.  |
`---------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}
#line 1845 "./parser.y"

/* Dialog specific functions */
static dialog_t *dialog_style(style_t * st, dialog_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->style == NULL)
	{
		dlg->style = new_style(0,0);
	}

	if(dlg->gotstyle)
	{
		yywarning("Style already defined, or-ing together");
	}
	else
	{
		dlg->style->or_mask = 0;
		dlg->style->and_mask = 0;
	}
	dlg->style->or_mask |= st->or_mask;
	dlg->style->and_mask |= st->and_mask;
	dlg->gotstyle = TRUE;
	free(st);
	return dlg;
}

static dialog_t *dialog_exstyle(style_t *st, dialog_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->exstyle == NULL)
	{
		dlg->exstyle = new_style(0,0);
	}

	if(dlg->gotexstyle)
	{
		yywarning("ExStyle already defined, or-ing together");
	}
	else
	{
		dlg->exstyle->or_mask = 0;
		dlg->exstyle->and_mask = 0;
	}
	dlg->exstyle->or_mask |= st->or_mask;
	dlg->exstyle->and_mask |= st->and_mask;
	dlg->gotexstyle = TRUE;
	free(st);
	return dlg;
}

static dialog_t *dialog_caption(string_t *s, dialog_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->title)
		yyerror("Caption already defined");
	dlg->title = s;
	return dlg;
}

static dialog_t *dialog_font(font_id_t *f, dialog_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->font)
		yyerror("Font already defined");
	dlg->font = f;
	return dlg;
}

static dialog_t *dialog_class(name_id_t *n, dialog_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->dlgclass)
		yyerror("Class already defined");
	dlg->dlgclass = n;
	return dlg;
}

static dialog_t *dialog_menu(name_id_t *m, dialog_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->menu)
		yyerror("Menu already defined");
	dlg->menu = m;
	return dlg;
}

static dialog_t *dialog_language(language_t *l, dialog_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->lvc.language)
		yyerror("Language already defined");
	dlg->lvc.language = l;
	return dlg;
}

static dialog_t *dialog_characteristics(characts_t *c, dialog_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->lvc.characts)
		yyerror("Characteristics already defined");
	dlg->lvc.characts = c;
	return dlg;
}

static dialog_t *dialog_version(version_t *v, dialog_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->lvc.version)
		yyerror("Version already defined");
	dlg->lvc.version = v;
	return dlg;
}

/* Controls specific functions */
static control_t *ins_ctrl(int type, int special_style, control_t *ctrl, control_t *prev)
{
	/* Hm... this seems to be jammed in at all time... */
	int defaultstyle = WS_CHILD | WS_VISIBLE;

	assert(ctrl != NULL);
	ctrl->prev = prev;

	if(prev)
		prev->next = ctrl;

	if(type != -1)
	{
		ctrl->ctlclass = new_name_id();
		ctrl->ctlclass->type = name_ord;
		ctrl->ctlclass->name.i_name = type;
	}

	switch(type)
	{
	case CT_BUTTON:
		if(special_style != BS_GROUPBOX && special_style != BS_RADIOBUTTON)
			defaultstyle |= WS_TABSTOP;
		break;
	case CT_EDIT:
		defaultstyle |= WS_TABSTOP | WS_BORDER;
		break;
	case CT_LISTBOX:
		defaultstyle |= LBS_NOTIFY | WS_BORDER;
		break;
	case CT_COMBOBOX:
                if (!(ctrl->style->or_mask & (CBS_SIMPLE | CBS_DROPDOWN | CBS_DROPDOWNLIST)))
                    defaultstyle |= CBS_SIMPLE;
		break;
	case CT_STATIC:
		if(special_style == SS_CENTER || special_style == SS_LEFT || special_style == SS_RIGHT)
			defaultstyle |= WS_GROUP;
		break;
	}

	if(!ctrl->gotstyle)	/* Handle default style setting */
	{
		switch(type)
		{
		case CT_EDIT:
			defaultstyle |= ES_LEFT;
			break;
		case CT_LISTBOX:
			defaultstyle |= LBS_NOTIFY;
			break;
		case CT_COMBOBOX:
			defaultstyle |= CBS_SIMPLE | WS_TABSTOP;
			break;
		case CT_SCROLLBAR:
			defaultstyle |= SBS_HORZ;
			break;
		case CT_BUTTON:
			switch(special_style)
			{
			case BS_CHECKBOX:
			case BS_DEFPUSHBUTTON:
			case BS_PUSHBUTTON:
/*			case BS_PUSHBOX:	*/
			case BS_AUTORADIOBUTTON:
			case BS_AUTO3STATE:
			case BS_3STATE:
			case BS_AUTOCHECKBOX:
				defaultstyle |= WS_TABSTOP;
				break;
			default:
				yywarning("Unknown default button control-style 0x%08x", special_style);
			case BS_GROUPBOX:
			case BS_RADIOBUTTON:
				break;
			}
			break;

		case CT_STATIC:
			switch(special_style)
			{
			case SS_LEFT:
			case SS_RIGHT:
			case SS_CENTER:
				defaultstyle |= WS_GROUP;
				break;
			case SS_ICON:	/* Special case */
				break;
			default:
				yywarning("Unknown default static control-style 0x%08x", special_style);
				break;
			}
			break;
		case -1:	/* Generic control */
			goto byebye;

		default:
			yyerror("Internal error (report this): Got weird control type 0x%08x", type);
		}
	}

	/* The SS_ICON flag is always forced in for icon controls */
	if(type == CT_STATIC && special_style == SS_ICON)
		defaultstyle |= SS_ICON;

	if (!ctrl->gotstyle)
		ctrl->style = new_style(0,0);

	/* combine all styles */
	ctrl->style->or_mask = ctrl->style->or_mask | defaultstyle | special_style;
	ctrl->gotstyle = TRUE;
byebye:
	/* combine with NOT mask */
	if (ctrl->gotstyle)
	{
		ctrl->style->or_mask &= ~(ctrl->style->and_mask);
		ctrl->style->and_mask = 0;
	}
	if (ctrl->gotexstyle)
	{
		ctrl->exstyle->or_mask &= ~(ctrl->exstyle->and_mask);
		ctrl->exstyle->and_mask = 0;
	}
	return ctrl;
}

static name_id_t *convert_ctlclass(name_id_t *cls)
{
	char *cc = NULL;
	int iclass;

	if(cls->type == name_ord)
		return cls;
	assert(cls->type == name_str);
	if(cls->type == str_unicode)
	{
		yyerror("Don't yet support unicode class comparison");
	}
	else
		cc = cls->name.s_name->str.cstr;

	if(!strcasecmp("BUTTON", cc))
		iclass = CT_BUTTON;
	else if(!strcasecmp("COMBOBOX", cc))
		iclass = CT_COMBOBOX;
	else if(!strcasecmp("LISTBOX", cc))
		iclass = CT_LISTBOX;
	else if(!strcasecmp("EDIT", cc))
		iclass = CT_EDIT;
	else if(!strcasecmp("STATIC", cc))
		iclass = CT_STATIC;
	else if(!strcasecmp("SCROLLBAR", cc))
		iclass = CT_SCROLLBAR;
	else
		return cls;	/* No default, return user controlclass */

	free(cls->name.s_name->str.cstr);
	free(cls->name.s_name);
	cls->type = name_ord;
	cls->name.i_name = iclass;
	return cls;
}

/* DialogEx specific functions */
static dialogex_t *dialogex_style(style_t * st, dialogex_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->style == NULL)
	{
		dlg->style = new_style(0,0);
	}

	if(dlg->gotstyle)
	{
		yywarning("Style already defined, or-ing together");
	}
	else
	{
		dlg->style->or_mask = 0;
		dlg->style->and_mask = 0;
	}
	dlg->style->or_mask |= st->or_mask;
	dlg->style->and_mask |= st->and_mask;
	dlg->gotstyle = TRUE;
	free(st);
	return dlg;
}

static dialogex_t *dialogex_exstyle(style_t * st, dialogex_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->exstyle == NULL)
	{
		dlg->exstyle = new_style(0,0);
	}

	if(dlg->gotexstyle)
	{
		yywarning("ExStyle already defined, or-ing together");
	}
	else
	{
		dlg->exstyle->or_mask = 0;
		dlg->exstyle->and_mask = 0;
	}
	dlg->exstyle->or_mask |= st->or_mask;
	dlg->exstyle->and_mask |= st->and_mask;
	dlg->gotexstyle = TRUE;
	free(st);
	return dlg;
}

static dialogex_t *dialogex_caption(string_t *s, dialogex_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->title)
		yyerror("Caption already defined");
	dlg->title = s;
	return dlg;
}

static dialogex_t *dialogex_font(font_id_t *f, dialogex_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->font)
		yyerror("Font already defined");
	dlg->font = f;
	return dlg;
}

static dialogex_t *dialogex_class(name_id_t *n, dialogex_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->dlgclass)
		yyerror("Class already defined");
	dlg->dlgclass = n;
	return dlg;
}

static dialogex_t *dialogex_menu(name_id_t *m, dialogex_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->menu)
		yyerror("Menu already defined");
	dlg->menu = m;
	return dlg;
}

static dialogex_t *dialogex_language(language_t *l, dialogex_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->lvc.language)
		yyerror("Language already defined");
	dlg->lvc.language = l;
	return dlg;
}

static dialogex_t *dialogex_characteristics(characts_t *c, dialogex_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->lvc.characts)
		yyerror("Characteristics already defined");
	dlg->lvc.characts = c;
	return dlg;
}

static dialogex_t *dialogex_version(version_t *v, dialogex_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->lvc.version)
		yyerror("Version already defined");
	dlg->lvc.version = v;
	return dlg;
}

/* Accelerator specific functions */
static event_t *add_event(int key, int id, int flags, event_t *prev)
{
	event_t *ev = new_event();

	if((flags & (WRC_AF_VIRTKEY | WRC_AF_ASCII)) == (WRC_AF_VIRTKEY | WRC_AF_ASCII))
		yyerror("Cannot use both ASCII and VIRTKEY");

	ev->key = key;
	ev->id = id;
	ev->flags = flags & ~WRC_AF_ASCII;
	ev->prev = prev;
	if(prev)
		prev->next = ev;
	return ev;
}

static event_t *add_string_event(string_t *key, int id, int flags, event_t *prev)
{
	int keycode = 0;
	event_t *ev = new_event();

	if(key->type != str_char)
		yyerror("Key code must be an ascii string");

	if((flags & WRC_AF_VIRTKEY) && (!isupper(key->str.cstr[0] & 0xff) && !isdigit(key->str.cstr[0] & 0xff)))
		yyerror("VIRTKEY code is not equal to ascii value");

	if(key->str.cstr[0] == '^' && (flags & WRC_AF_CONTROL) != 0)
	{
		yyerror("Cannot use both '^' and CONTROL modifier");
	}
	else if(key->str.cstr[0] == '^')
	{
		keycode = toupper(key->str.cstr[1]) - '@';
		if(keycode >= ' ')
			yyerror("Control-code out of range");
	}
	else
		keycode = key->str.cstr[0];
	ev->key = keycode;
	ev->id = id;
	ev->flags = flags & ~WRC_AF_ASCII;
	ev->prev = prev;
	if(prev)
		prev->next = ev;
	return ev;
}

/* MenuEx specific functions */
static itemex_opt_t *new_itemex_opt(int id, int type, int state, int helpid)
{
	itemex_opt_t *opt = (itemex_opt_t *)xmalloc(sizeof(itemex_opt_t));
	opt->id = id;
	opt->type = type;
	opt->state = state;
	opt->helpid = helpid;
	return opt;
}

/* Raw data functions */
static raw_data_t *load_file(string_t *filename, language_t *lang)
{
	FILE *fp = NULL;
	char *path;
	raw_data_t *rd;
	string_t *name;
	int codepage = get_language_codepage(lang->id, lang->sub);

	/* FIXME: we may want to use utf-8 here */
	if (codepage <= 0 && filename->type != str_char)
		yyerror("Cannot convert filename to ASCII string");
	name = convert_string( filename, str_char, codepage );
	if (!(path = wpp_find_include(name->str.cstr, 1)))
		yyerror("Cannot open file %s", name->str.cstr);
	if (!(fp = fopen( path, "rb" )))
		yyerror("Cannot open file %s", name->str.cstr);
	free( path );
	rd = new_raw_data();
	fseek(fp, 0, SEEK_END);
	rd->size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if (rd->size)
	{
		rd->data = (char *)xmalloc(rd->size);
		fread(rd->data, rd->size, 1, fp);
	}
	else rd->data = NULL;
	fclose(fp);
	rd->lvc.language = lang;
	free_string(name);
	return rd;
}

static raw_data_t *int2raw_data(int i)
{
	raw_data_t *rd;

	if( ( i >= 0 && (int)((unsigned short)i) != i) ||
            ( i < 0  && (int)((short)i) != i) )
		yywarning("Integer constant out of 16bit range (%d), truncated to %d\n", i, (short)i);

	rd = new_raw_data();
	rd->size = sizeof(short);
	rd->data = (char *)xmalloc(rd->size);
	switch(byteorder)
	{
#ifdef WORDS_BIGENDIAN
	default:
#endif
	case WRC_BO_BIG:
		rd->data[0] = HIBYTE(i);
		rd->data[1] = LOBYTE(i);
		break;

#ifndef WORDS_BIGENDIAN
	default:
#endif
	case WRC_BO_LITTLE:
		rd->data[1] = HIBYTE(i);
		rd->data[0] = LOBYTE(i);
		break;
	}
	return rd;
}

static raw_data_t *long2raw_data(int i)
{
	raw_data_t *rd;
	rd = new_raw_data();
	rd->size = sizeof(int);
	rd->data = (char *)xmalloc(rd->size);
	switch(byteorder)
	{
#ifdef WORDS_BIGENDIAN
	default:
#endif
	case WRC_BO_BIG:
		rd->data[0] = HIBYTE(HIWORD(i));
		rd->data[1] = LOBYTE(HIWORD(i));
		rd->data[2] = HIBYTE(LOWORD(i));
		rd->data[3] = LOBYTE(LOWORD(i));
		break;

#ifndef WORDS_BIGENDIAN
	default:
#endif
	case WRC_BO_LITTLE:
		rd->data[3] = HIBYTE(HIWORD(i));
		rd->data[2] = LOBYTE(HIWORD(i));
		rd->data[1] = HIBYTE(LOWORD(i));
		rd->data[0] = LOBYTE(LOWORD(i));
		break;
	}
	return rd;
}

static raw_data_t *str2raw_data(string_t *str)
{
	raw_data_t *rd;
	rd = new_raw_data();
	rd->size = str->size * (str->type == str_char ? 1 : 2);
	rd->data = (char *)xmalloc(rd->size);
	if(str->type == str_char)
		memcpy(rd->data, str->str.cstr, rd->size);
	else if(str->type == str_unicode)
	{
		int i;
		switch(byteorder)
		{
#ifdef WORDS_BIGENDIAN
		default:
#endif
		case WRC_BO_BIG:
			for(i = 0; i < str->size; i++)
			{
				rd->data[2*i + 0] = HIBYTE((WORD)str->str.wstr[i]);
				rd->data[2*i + 1] = LOBYTE((WORD)str->str.wstr[i]);
			}
			break;
#ifndef WORDS_BIGENDIAN
		default:
#endif
		case WRC_BO_LITTLE:
			for(i = 0; i < str->size; i++)
			{
				rd->data[2*i + 1] = HIBYTE((WORD)str->str.wstr[i]);
				rd->data[2*i + 0] = LOBYTE((WORD)str->str.wstr[i]);
			}
			break;
		}
	}
	else
		internal_error(__FILE__, __LINE__, "Invalid stringtype");
	return rd;
}

static raw_data_t *merge_raw_data(raw_data_t *r1, raw_data_t *r2)
{
	r1->data = xrealloc(r1->data, r1->size + r2->size);
	memcpy(r1->data + r1->size, r2->data, r2->size);
	r1->size += r2->size;
	return r1;
}

static raw_data_t *merge_raw_data_int(raw_data_t *r1, int i)
{
	raw_data_t *t = int2raw_data(i);
	merge_raw_data(r1, t);
	free(t->data);
	free(t);
	return r1;
}

static raw_data_t *merge_raw_data_long(raw_data_t *r1, int i)
{
	raw_data_t *t = long2raw_data(i);
	merge_raw_data(r1, t);
	free(t->data);
	free(t);
	return r1;
}

static raw_data_t *merge_raw_data_str(raw_data_t *r1, string_t *str)
{
	raw_data_t *t = str2raw_data(str);
	merge_raw_data(r1, t);
	free(t->data);
	free(t);
	return r1;
}

/* Function the go back in a list to get the head */
static menu_item_t *get_item_head(menu_item_t *p)
{
	if(!p)
		return NULL;
	while(p->prev)
		p = p->prev;
	return p;
}

static menuex_item_t *get_itemex_head(menuex_item_t *p)
{
	if(!p)
		return NULL;
	while(p->prev)
		p = p->prev;
	return p;
}

static resource_t *get_resource_head(resource_t *p)
{
	if(!p)
		return NULL;
	while(p->prev)
		p = p->prev;
	return p;
}

static ver_block_t *get_ver_block_head(ver_block_t *p)
{
	if(!p)
		return NULL;
	while(p->prev)
		p = p->prev;
	return p;
}

static ver_value_t *get_ver_value_head(ver_value_t *p)
{
	if(!p)
		return NULL;
	while(p->prev)
		p = p->prev;
	return p;
}

static control_t *get_control_head(control_t *p)
{
	if(!p)
		return NULL;
	while(p->prev)
		p = p->prev;
	return p;
}

static event_t *get_event_head(event_t *p)
{
	if(!p)
		return NULL;
	while(p->prev)
		p = p->prev;
	return p;
}

/* Find a stringtable with given language */
static stringtable_t *find_stringtable(lvc_t *lvc)
{
	stringtable_t *stt;

	assert(lvc != NULL);

	if(!lvc->language)
		lvc->language = dup_language(currentlanguage);

	for(stt = sttres; stt; stt = stt->next)
	{
		if(stt->lvc.language->id == lvc->language->id
		&& stt->lvc.language->sub == lvc->language->sub)
		{
			/* Found a table with the same language */
			/* The version and characteristics are now handled
			 * in the generation of the individual stringtables.
			 * This enables localized analysis.
			if((stt->lvc.version && lvc->version && *(stt->lvc.version) != *(lvc->version))
			|| (!stt->lvc.version && lvc->version)
			|| (stt->lvc.version && !lvc->version))
				yywarning("Stringtable's versions are not the same, using first definition");

			if((stt->lvc.characts && lvc->characts && *(stt->lvc.characts) != *(lvc->characts))
			|| (!stt->lvc.characts && lvc->characts)
			|| (stt->lvc.characts && !lvc->characts))
				yywarning("Stringtable's characteristics are not the same, using first definition");
			*/
			return stt;
		}
	}
	return NULL;
}

/* qsort sorting function for string table entries */
#define STE(p)	((stt_entry_t *)(p))
static int sort_stt_entry(const void *e1, const void *e2)
{
	return STE(e1)->id - STE(e2)->id;
}
#undef STE

static resource_t *build_stt_resources(stringtable_t *stthead)
{
	stringtable_t *stt;
	stringtable_t *newstt;
	resource_t *rsc;
	resource_t *rsclist = NULL;
	resource_t *rsctail = NULL;
	int i;
	int j;
	DWORD andsum;
	DWORD orsum;
	characts_t *characts;
	version_t *version;

	if(!stthead)
		return NULL;

	/* For all languages defined */
	for(stt = stthead; stt; stt = stt->next)
	{
		assert(stt->nentries > 0);

		/* Sort the entries */
		if(stt->nentries > 1)
			qsort(stt->entries, stt->nentries, sizeof(stt->entries[0]), sort_stt_entry);

		for(i = 0; i < stt->nentries; )
		{
			newstt = new_stringtable(&stt->lvc);
			newstt->entries = (stt_entry_t *)xmalloc(16 * sizeof(stt_entry_t));
			newstt->nentries = 16;
			newstt->idbase = stt->entries[i].id & ~0xf;
			for(j = 0; j < 16 && i < stt->nentries; j++)
			{
				if(stt->entries[i].id - newstt->idbase == j)
				{
					newstt->entries[j] = stt->entries[i];
					i++;
				}
			}
			andsum = ~0;
			orsum = 0;
			characts = NULL;
			version = NULL;
			/* Check individual memory options and get
			 * the first characteristics/version
			 */
			for(j = 0; j < 16; j++)
			{
				if(!newstt->entries[j].str)
					continue;
				andsum &= newstt->entries[j].memopt;
				orsum |= newstt->entries[j].memopt;
				if(!characts)
					characts = newstt->entries[j].characts;
				if(!version)
					version = newstt->entries[j].version;
			}
			if(andsum != orsum)
			{
				warning("Stringtable's memory options are not equal (idbase: %d)", newstt->idbase);
			}
			/* Check version and characteristics */
			for(j = 0; j < 16; j++)
			{
				if(characts
				&& newstt->entries[j].characts
				&& *newstt->entries[j].characts != *characts)
					warning("Stringtable's characteristics are not the same (idbase: %d)", newstt->idbase);
				if(version
				&& newstt->entries[j].version
				&& *newstt->entries[j].version != *version)
					warning("Stringtable's versions are not the same (idbase: %d)", newstt->idbase);
			}
			rsc = new_resource(res_stt, newstt, newstt->memopt, newstt->lvc.language);
			rsc->name = new_name_id();
			rsc->name->type = name_ord;
			rsc->name->name.i_name = (newstt->idbase >> 4) + 1;
			rsc->memopt = andsum; /* Set to least common denominator */
			newstt->memopt = andsum;
			newstt->lvc.characts = characts;
			newstt->lvc.version = version;
			if(!rsclist)
			{
				rsclist = rsc;
				rsctail = rsc;
			}
			else
			{
				rsctail->next = rsc;
				rsc->prev = rsctail;
				rsctail = rsc;
			}
		}
	}
	return rsclist;
}


static toolbar_item_t *ins_tlbr_button(toolbar_item_t *prev, toolbar_item_t *idrec)
{
	idrec->prev = prev;
	if(prev)
		prev->next = idrec;

	return idrec;
}

static toolbar_item_t *get_tlbr_buttons_head(toolbar_item_t *p, int *nitems)
{
	if(!p)
	{
		*nitems = 0;
		return NULL;
	}

	*nitems = 1;

	while(p->prev)
	{
		(*nitems)++;
		p = p->prev;
	}

	return p;
}

static string_t *make_filename(string_t *str)
{
    if(str->type == str_char)
    {
	char *cptr;

	/* Remove escaped backslash and convert to forward */
	for(cptr = str->str.cstr; (cptr = strchr(cptr, '\\')) != NULL; cptr++)
	{
		if(cptr[1] == '\\')
		{
			memmove(cptr, cptr+1, strlen(cptr));
			str->size--;
		}
		*cptr = '/';
	}
    }
    else
    {
	WCHAR *wptr;

	/* Remove escaped backslash and convert to forward */
	for(wptr = str->str.wstr; (wptr = strchrW(wptr, '\\')) != NULL; wptr++)
	{
		if(wptr[1] == '\\')
		{
			memmove(wptr, wptr+1, strlenW(wptr));
			str->size--;
		}
		*wptr = '/';
	}
    }
    return str;
}

/*
 * Process all resources to extract fonts and build
 * a fontdir resource.
 *
 * Note: MS' resource compiler (build 1472) does not
 * handle font resources with different languages.
 * The fontdir is generated in the last active language
 * and font identifiers must be unique across the entire
 * source.
 * This is not logical considering the localization
 * constraints of all other resource types. MS has,
 * most probably, never testet localized fonts. However,
 * using fontresources is rare, so it might not occur
 * in normal applications.
 * Wine does require better localization because a lot
 * of languages are coded into the same executable.
 * Therefore, I will generate fontdirs for *each*
 * localized set of fonts.
 */
static resource_t *build_fontdir(resource_t **fnt, int nfnt)
{
	static int once = 0;
	if(!once)
	{
		warning("Need to parse fonts, not yet implemented (fnt: %p, nfnt: %d)", fnt, nfnt);
		once++;
	}
	return NULL;
}

static resource_t *build_fontdirs(resource_t *tail)
{
	resource_t *rsc;
	resource_t *lst = NULL;
	resource_t **fnt = NULL;	/* List of all fonts */
	int nfnt = 0;
	resource_t **fnd = NULL;	/* List of all fontdirs */
	int nfnd = 0;
	resource_t **lanfnt = NULL;
	int nlanfnt = 0;
	int i;
	name_id_t nid;
	string_t str;
	int fntleft;

	nid.type = name_str;
	nid.name.s_name = &str;
	str.type = str_char;
	str.str.cstr = xstrdup("FONTDIR");
	str.size = 7;

	/* Extract all fonts and fontdirs */
	for(rsc = tail; rsc; rsc = rsc->prev)
	{
		if(rsc->type == res_fnt)
		{
			nfnt++;
			fnt = xrealloc(fnt, nfnt * sizeof(*fnt));
			fnt[nfnt-1] = rsc;
		}
		else if(rsc->type == res_fntdir)
		{
			nfnd++;
			fnd = xrealloc(fnd, nfnd * sizeof(*fnd));
			fnd[nfnd-1] = rsc;
		}
	}

	/* Verify the name of the present fontdirs */
	for(i = 0; i < nfnd; i++)
	{
		if(compare_name_id(&nid, fnd[i]->name))
		{
			warning("User supplied FONTDIR entry has an invalid name '%s', ignored",
				get_nameid_str(fnd[i]->name));
			fnd[i] = NULL;
		}
	}

	/* Sanity check */
	if(nfnt == 0)
	{
		if(nfnd != 0)
			warning("Found %d FONTDIR entries without any fonts present", nfnd);
		goto clean;
	}

	/* Copy space */
	lanfnt = xmalloc(nfnt * sizeof(*lanfnt));

	/* Get all fonts covered by fontdirs */
	for(i = 0; i < nfnd; i++)
	{
		int j;
		WORD cnt;
		int isswapped = 0;

		if(!fnd[i])
			continue;
		for(j = 0; j < nfnt; j++)
		{
			if(!fnt[j])
				continue;
			if(fnt[j]->lan->id == fnd[i]->lan->id && fnt[j]->lan->sub == fnd[i]->lan->sub)
			{
				lanfnt[nlanfnt] = fnt[j];
				nlanfnt++;
				fnt[j] = NULL;
			}
		}

		cnt = *(WORD *)fnd[i]->res.fnd->data->data;
		if(nlanfnt == cnt)
			isswapped = 0;
		else if(nlanfnt == BYTESWAP_WORD(cnt))
			isswapped = 1;
		else
			error("FONTDIR for language %d,%d has wrong count (%d, expected %d)",
				fnd[i]->lan->id, fnd[i]->lan->sub, cnt, nlanfnt);
#ifdef WORDS_BIGENDIAN
		if((byteorder == WRC_BO_LITTLE && !isswapped) || (byteorder != WRC_BO_LITTLE && isswapped))
#else
		if((byteorder == WRC_BO_BIG && !isswapped) || (byteorder != WRC_BO_BIG && isswapped))
#endif
		{
			internal_error(__FILE__, __LINE__, "User supplied FONTDIR needs byteswapping");
		}
	}

	/* We now have fonts left where we need to make a fontdir resource */
	for(i = fntleft = 0; i < nfnt; i++)
	{
		if(fnt[i])
			fntleft++;
	}
	while(fntleft)
	{
		/* Get fonts of same language in lanfnt[] */
		for(i = nlanfnt = 0; i < nfnt; i++)
		{
			if(fnt[i])
			{
				if(!nlanfnt)
				{
			addlanfnt:
					lanfnt[nlanfnt] = fnt[i];
					nlanfnt++;
					fnt[i] = NULL;
					fntleft--;
				}
				else if(fnt[i]->lan->id == lanfnt[0]->lan->id && fnt[i]->lan->sub == lanfnt[0]->lan->sub)
					goto addlanfnt;
			}
		}
		/* and build a fontdir */
		rsc = build_fontdir(lanfnt, nlanfnt);
		if(rsc)
		{
			if(lst)
			{
				lst->next = rsc;
				rsc->prev = lst;
			}
			lst = rsc;
		}
	}

	free(lanfnt);
clean:
	if(fnt)
		free(fnt);
	if(fnd)
		free(fnd);
	free(str.str.cstr);
	return lst;
}

/*
 * This gets invoked to determine whether the next resource
 * is to be of a standard-type (e.g. bitmaps etc.), or should
 * be a user-type resource. This function is required because
 * there is the _possibility_ of a lookahead token in the
 * parser, which is generated from the "expr" state in the
 * "nameid" parsing.
 *
 * The general resource format is:
 * <identifier> <type> <flags> <resourcebody>
 *
 * The <identifier> can either be tIDENT or "expr". The latter
 * will always generate a lookahead, which is the <type> of the
 * resource to parse. Otherwise, we need to get a new token from
 * the scanner to determine the next step.
 *
 * The problem arrises when <type> is numerical. This case should
 * map onto default resource-types and be parsed as such instead
 * of being mapped onto user-type resources.
 *
 * The trick lies in the fact that yacc (bison) doesn't care about
 * intermediate changes of the lookahead while reducing a rule. We
 * simply replace the lookahead with a token that will result in
 * a shift to the appropriate rule for the specific resource-type.
 */
static int rsrcid_to_token(int lookahead)
{
	int token;
	const char *type = "?";

	/* Get a token if we don't have one yet */
	if(lookahead == YYEMPTY)
		lookahead = YYLEX;

	/* Only numbers are possibly interesting */
	switch(lookahead)
	{
	case tNUMBER:
	case tLNUMBER:
		break;
	default:
		return lookahead;
	}

	token = lookahead;

	switch(yylval.num)
	{
	case WRC_RT_CURSOR:
		type = "CURSOR";
		token = tCURSOR;
		break;
	case WRC_RT_ICON:
		type = "ICON";
		token = tICON;
		break;
	case WRC_RT_BITMAP:
		type = "BITMAP";
		token = tBITMAP;
		break;
	case WRC_RT_FONT:
		type = "FONT";
		token = tFONT;
		break;
	case WRC_RT_FONTDIR:
		type = "FONTDIR";
		token = tFONTDIR;
		break;
	case WRC_RT_RCDATA:
		type = "RCDATA";
		token = tRCDATA;
		break;
	case WRC_RT_MESSAGETABLE:
		type = "MESSAGETABLE";
		token = tMESSAGETABLE;
		break;
	case WRC_RT_DLGINIT:
		type = "DLGINIT";
		token = tDLGINIT;
		break;
	case WRC_RT_ACCELERATOR:
		type = "ACCELERATOR";
		token = tACCELERATORS;
		break;
	case WRC_RT_MENU:
		type = "MENU";
		token = tMENU;
		break;
	case WRC_RT_DIALOG:
		type = "DIALOG";
		token = tDIALOG;
		break;
	case WRC_RT_VERSION:
		type = "VERSION";
		token = tVERSIONINFO;
		break;
	case WRC_RT_TOOLBAR:
		type = "TOOLBAR";
		token = tTOOLBAR;
		break;

	case WRC_RT_STRING:
		type = "STRINGTABLE";
		break;

	case WRC_RT_ANICURSOR:
	case WRC_RT_ANIICON:
	case WRC_RT_GROUP_CURSOR:
	case WRC_RT_GROUP_ICON:
		yywarning("Usertype uses reserved type ID %d, which is auto-generated", yylval.num);
		return lookahead;

	case WRC_RT_DLGINCLUDE:
	case WRC_RT_PLUGPLAY:
	case WRC_RT_VXD:
	case WRC_RT_HTML:
		yywarning("Usertype uses reserved type ID %d, which is not supported by wrc yet", yylval.num);
	default:
		return lookahead;
	}

	return token;
}
