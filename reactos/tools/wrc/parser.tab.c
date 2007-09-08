/* A Bison parser, made by GNU Bison 1.875c.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

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

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     tNL = 258,
     tNUMBER = 259,
     tLNUMBER = 260,
     tSTRING = 261,
     tIDENT = 262,
     tFILENAME = 263,
     tRAWDATA = 264,
     tACCELERATORS = 265,
     tBITMAP = 266,
     tCURSOR = 267,
     tDIALOG = 268,
     tDIALOGEX = 269,
     tMENU = 270,
     tMENUEX = 271,
     tMESSAGETABLE = 272,
     tRCDATA = 273,
     tVERSIONINFO = 274,
     tSTRINGTABLE = 275,
     tFONT = 276,
     tFONTDIR = 277,
     tICON = 278,
     tHTML = 279,
     tAUTO3STATE = 280,
     tAUTOCHECKBOX = 281,
     tAUTORADIOBUTTON = 282,
     tCHECKBOX = 283,
     tDEFPUSHBUTTON = 284,
     tPUSHBUTTON = 285,
     tRADIOBUTTON = 286,
     tSTATE3 = 287,
     tGROUPBOX = 288,
     tCOMBOBOX = 289,
     tLISTBOX = 290,
     tSCROLLBAR = 291,
     tCONTROL = 292,
     tEDITTEXT = 293,
     tRTEXT = 294,
     tCTEXT = 295,
     tLTEXT = 296,
     tBLOCK = 297,
     tVALUE = 298,
     tSHIFT = 299,
     tALT = 300,
     tASCII = 301,
     tVIRTKEY = 302,
     tGRAYED = 303,
     tCHECKED = 304,
     tINACTIVE = 305,
     tNOINVERT = 306,
     tPURE = 307,
     tIMPURE = 308,
     tDISCARDABLE = 309,
     tLOADONCALL = 310,
     tPRELOAD = 311,
     tFIXED = 312,
     tMOVEABLE = 313,
     tCLASS = 314,
     tCAPTION = 315,
     tCHARACTERISTICS = 316,
     tEXSTYLE = 317,
     tSTYLE = 318,
     tVERSION = 319,
     tLANGUAGE = 320,
     tFILEVERSION = 321,
     tPRODUCTVERSION = 322,
     tFILEFLAGSMASK = 323,
     tFILEOS = 324,
     tFILETYPE = 325,
     tFILEFLAGS = 326,
     tFILESUBTYPE = 327,
     tMENUBARBREAK = 328,
     tMENUBREAK = 329,
     tMENUITEM = 330,
     tPOPUP = 331,
     tSEPARATOR = 332,
     tHELP = 333,
     tTOOLBAR = 334,
     tBUTTON = 335,
     tBEGIN = 336,
     tEND = 337,
     tDLGINIT = 338,
     tNOT = 339,
     pUPM = 340
   };
#endif
#define tNL 258
#define tNUMBER 259
#define tLNUMBER 260
#define tSTRING 261
#define tIDENT 262
#define tFILENAME 263
#define tRAWDATA 264
#define tACCELERATORS 265
#define tBITMAP 266
#define tCURSOR 267
#define tDIALOG 268
#define tDIALOGEX 269
#define tMENU 270
#define tMENUEX 271
#define tMESSAGETABLE 272
#define tRCDATA 273
#define tVERSIONINFO 274
#define tSTRINGTABLE 275
#define tFONT 276
#define tFONTDIR 277
#define tICON 278
#define tHTML 279
#define tAUTO3STATE 280
#define tAUTOCHECKBOX 281
#define tAUTORADIOBUTTON 282
#define tCHECKBOX 283
#define tDEFPUSHBUTTON 284
#define tPUSHBUTTON 285
#define tRADIOBUTTON 286
#define tSTATE3 287
#define tGROUPBOX 288
#define tCOMBOBOX 289
#define tLISTBOX 290
#define tSCROLLBAR 291
#define tCONTROL 292
#define tEDITTEXT 293
#define tRTEXT 294
#define tCTEXT 295
#define tLTEXT 296
#define tBLOCK 297
#define tVALUE 298
#define tSHIFT 299
#define tALT 300
#define tASCII 301
#define tVIRTKEY 302
#define tGRAYED 303
#define tCHECKED 304
#define tINACTIVE 305
#define tNOINVERT 306
#define tPURE 307
#define tIMPURE 308
#define tDISCARDABLE 309
#define tLOADONCALL 310
#define tPRELOAD 311
#define tFIXED 312
#define tMOVEABLE 313
#define tCLASS 314
#define tCAPTION 315
#define tCHARACTERISTICS 316
#define tEXSTYLE 317
#define tSTYLE 318
#define tVERSION 319
#define tLANGUAGE 320
#define tFILEVERSION 321
#define tPRODUCTVERSION 322
#define tFILEFLAGSMASK 323
#define tFILEOS 324
#define tFILETYPE 325
#define tFILEFLAGS 326
#define tFILESUBTYPE 327
#define tMENUBARBREAK 328
#define tMENUBREAK 329
#define tMENUITEM 330
#define tPOPUP 331
#define tSEPARATOR 332
#define tHELP 333
#define tTOOLBAR 334
#define tBUTTON 335
#define tBEGIN 336
#define tEND 337
#define tDLGINIT 338
#define tNOT 339
#define pUPM 340




/* Copy the first part of user declarations.  */
#line 1 "parser.y"

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
#include "wine/port.h"

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



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 241 "parser.y"
typedef union YYSTYPE {
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
	html_t		*html;
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
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 531 "parser.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 543 "parser.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

# ifndef YYFREE
#  define YYFREE free
# endif
# ifndef YYMALLOC
#  define YYMALLOC malloc
# endif

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   define YYSTACK_ALLOC alloca
#  endif
# else
#  if defined (alloca) || defined (_ALLOCA_H)
#   define YYSTACK_ALLOC alloca
#  else
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
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
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
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
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   713

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  97
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  84
/* YYNRULES -- Number of rules. */
#define YYNRULES  259
/* YYNRULES -- Number of states. */
#define YYNSTATES  575

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   340

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    86,     2,
      95,    96,    89,    87,    94,    88,     2,    90,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    85,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    84,     2,    91,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    92,
      93
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     5,     6,     9,    12,    16,    20,    22,
      23,    29,    30,    32,    34,    36,    38,    40,    42,    44,
      46,    48,    50,    52,    54,    56,    58,    60,    62,    64,
      66,    68,    70,    72,    74,    76,    78,    82,    86,    90,
      94,    98,   102,   106,   110,   114,   118,   120,   122,   129,
     130,   136,   142,   143,   146,   148,   152,   154,   156,   158,
     160,   162,   164,   178,   179,   183,   187,   191,   194,   198,
     202,   205,   208,   211,   212,   216,   220,   224,   228,   232,
     236,   240,   244,   248,   252,   256,   260,   264,   268,   272,
     276,   280,   291,   304,   315,   316,   321,   328,   337,   355,
     371,   376,   377,   380,   385,   389,   393,   395,   398,   400,
     402,   417,   418,   422,   426,   430,   433,   436,   440,   444,
     447,   450,   453,   454,   458,   462,   466,   470,   474,   478,
     482,   486,   490,   494,   498,   502,   506,   510,   514,   518,
     522,   533,   553,   570,   585,   598,   599,   601,   602,   605,
     615,   616,   619,   624,   628,   629,   636,   640,   646,   647,
     651,   655,   659,   663,   667,   671,   676,   680,   681,   686,
     690,   696,   697,   700,   706,   713,   714,   717,   722,   729,
     738,   743,   747,   748,   753,   754,   756,   763,   764,   774,
     784,   788,   792,   796,   800,   804,   805,   808,   814,   815,
     818,   820,   825,   830,   832,   836,   846,   847,   851,   854,
     855,   858,   861,   863,   865,   867,   869,   871,   873,   875,
     876,   879,   882,   885,   890,   893,   896,   901,   903,   905,
     908,   910,   913,   915,   919,   923,   928,   932,   937,   941,
     943,   945,   946,   948,   950,   954,   958,   962,   966,   970,
     974,   978,   981,   984,   987,   991,   993,   995,   998,  1000
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
      98,     0,    -1,    99,    -1,    -1,    99,   100,    -1,    99,
       3,    -1,   177,   102,   105,    -1,     7,   102,   105,    -1,
     153,    -1,    -1,    65,   101,   177,    94,   177,    -1,    -1,
     177,    -1,     7,    -1,   103,    -1,     6,    -1,   118,    -1,
     107,    -1,   108,    -1,   123,    -1,   134,    -1,   115,    -1,
     110,    -1,   111,    -1,   109,    -1,   144,    -1,   148,    -1,
     112,    -1,   113,    -1,   114,    -1,   164,    -1,   116,    -1,
     157,    -1,     8,    -1,     7,    -1,     6,    -1,    11,   166,
     175,    -1,    12,   166,   175,    -1,    23,   166,   175,    -1,
      21,   166,   175,    -1,    22,   166,   175,    -1,    17,   166,
     175,    -1,    24,   166,   175,    -1,    18,   166,   175,    -1,
      83,   166,   175,    -1,   117,   166,   175,    -1,     4,    -1,
       7,    -1,    10,   166,   169,    81,   119,    82,    -1,    -1,
     119,     6,    94,   177,   120,    -1,   119,   177,    94,   177,
     120,    -1,    -1,    94,   121,    -1,   122,    -1,   121,    94,
     122,    -1,    51,    -1,    44,    -1,    37,    -1,    45,    -1,
      46,    -1,    47,    -1,    13,   166,   177,    94,   177,    94,
     177,    94,   177,   124,    81,   125,    82,    -1,    -1,   124,
      63,   132,    -1,   124,    62,   132,    -1,   124,    60,     6,
      -1,   124,   130,    -1,   124,    59,   104,    -1,   124,    15,
     103,    -1,   124,   170,    -1,   124,   171,    -1,   124,   172,
      -1,    -1,   125,    37,   129,    -1,   125,    38,   127,    -1,
     125,    35,   127,    -1,   125,    34,   127,    -1,   125,    36,
     127,    -1,   125,    28,   126,    -1,   125,    29,   126,    -1,
     125,    33,   126,    -1,   125,    30,   126,    -1,   125,    31,
     126,    -1,   125,    25,   126,    -1,   125,    32,   126,    -1,
     125,    26,   126,    -1,   125,    27,   126,    -1,   125,    41,
     126,    -1,   125,    40,   126,    -1,   125,    39,   126,    -1,
     125,    23,   104,   156,   177,    94,   177,    94,   177,   128,
      -1,     6,   156,   177,    94,   177,    94,   177,    94,   177,
      94,   177,   131,    -1,   177,    94,   177,    94,   177,    94,
     177,    94,   177,   131,    -1,    -1,    94,   177,    94,   177,
      -1,    94,   177,    94,   177,    94,   132,    -1,    94,   177,
      94,   177,    94,   132,    94,   132,    -1,   104,   156,   177,
      94,   133,    94,   132,    94,   177,    94,   177,    94,   177,
      94,   177,    94,   132,    -1,   104,   156,   177,    94,   133,
      94,   132,    94,   177,    94,   177,    94,   177,    94,   177,
      -1,    21,   177,    94,     6,    -1,    -1,    94,   132,    -1,
      94,   132,    94,   132,    -1,   132,    84,   132,    -1,    95,
     132,    96,    -1,   178,    -1,    92,   178,    -1,   177,    -1,
       6,    -1,    14,   166,   177,    94,   177,    94,   177,    94,
     177,   141,   135,    81,   136,    82,    -1,    -1,   135,    63,
     132,    -1,   135,    62,   132,    -1,   135,    60,     6,    -1,
     135,   130,    -1,   135,   142,    -1,   135,    59,   104,    -1,
     135,    15,   103,    -1,   135,   170,    -1,   135,   171,    -1,
     135,   172,    -1,    -1,   136,    37,   137,    -1,   136,    38,
     139,    -1,   136,    35,   139,    -1,   136,    34,   139,    -1,
     136,    36,   139,    -1,   136,    28,   138,    -1,   136,    29,
     138,    -1,   136,    33,   138,    -1,   136,    30,   138,    -1,
     136,    31,   138,    -1,   136,    25,   138,    -1,   136,    32,
     138,    -1,   136,    26,   138,    -1,   136,    27,   138,    -1,
     136,    41,   138,    -1,   136,    40,   138,    -1,   136,    39,
     138,    -1,   136,    23,   104,   156,   177,    94,   177,    94,
     177,   128,    -1,   104,   156,   177,    94,   133,    94,   132,
      94,   177,    94,   177,    94,   177,    94,   177,    94,   132,
     141,   140,    -1,   104,   156,   177,    94,   133,    94,   132,
      94,   177,    94,   177,    94,   177,    94,   177,   140,    -1,
       6,   156,   177,    94,   177,    94,   177,    94,   177,    94,
     177,   131,   141,   140,    -1,   177,    94,   177,    94,   177,
      94,   177,    94,   177,   131,   141,   140,    -1,    -1,   173,
      -1,    -1,    94,   177,    -1,    21,   177,    94,     6,    94,
     177,    94,   177,   143,    -1,    -1,    94,   177,    -1,    15,
     166,   169,   145,    -1,    81,   146,    82,    -1,    -1,   146,
      75,     6,   156,   177,   147,    -1,   146,    75,    77,    -1,
     146,    76,     6,   147,   145,    -1,    -1,   156,    49,   147,
      -1,   156,    48,   147,    -1,   156,    78,   147,    -1,   156,
      50,   147,    -1,   156,    73,   147,    -1,   156,    74,   147,
      -1,    16,   166,   169,   149,    -1,    81,   150,    82,    -1,
      -1,   150,    75,     6,   151,    -1,   150,    75,    77,    -1,
     150,    76,     6,   152,   149,    -1,    -1,    94,   177,    -1,
      94,   176,    94,   176,   147,    -1,    94,   176,    94,   176,
      94,   177,    -1,    -1,    94,   177,    -1,    94,   176,    94,
     177,    -1,    94,   176,    94,   176,    94,   177,    -1,    94,
     176,    94,   176,    94,   176,    94,   177,    -1,   154,    81,
     155,    82,    -1,    20,   166,   169,    -1,    -1,   155,   177,
     156,     6,    -1,    -1,    94,    -1,    19,   166,   158,    81,
     159,    82,    -1,    -1,   158,    66,   177,    94,   177,    94,
     177,    94,   177,    -1,   158,    67,   177,    94,   177,    94,
     177,    94,   177,    -1,   158,    71,   177,    -1,   158,    68,
     177,    -1,   158,    69,   177,    -1,   158,    70,   177,    -1,
     158,    72,   177,    -1,    -1,   159,   160,    -1,    42,     6,
      81,   161,    82,    -1,    -1,   161,   162,    -1,   160,    -1,
      43,     6,    94,     6,    -1,    43,     6,    94,   163,    -1,
     177,    -1,   163,    94,   177,    -1,    79,   166,   177,    94,
     177,   169,    81,   165,    82,    -1,    -1,   165,    80,   177,
      -1,   165,    77,    -1,    -1,   166,   167,    -1,   166,   168,
      -1,    56,    -1,    58,    -1,    54,    -1,    52,    -1,    55,
      -1,    57,    -1,    53,    -1,    -1,   169,   170,    -1,   169,
     171,    -1,   169,   172,    -1,    65,   177,    94,   177,    -1,
      61,   177,    -1,    64,   177,    -1,   169,    81,   174,    82,
      -1,     9,    -1,     4,    -1,    88,     4,    -1,     5,    -1,
      88,     5,    -1,     6,    -1,   174,   156,     9,    -1,   174,
     156,     4,    -1,   174,   156,    88,     4,    -1,   174,   156,
       5,    -1,   174,   156,    88,     5,    -1,   174,   156,     6,
      -1,   106,    -1,   173,    -1,    -1,   177,    -1,   179,    -1,
     179,    87,   179,    -1,   179,    88,   179,    -1,   179,    84,
     179,    -1,   179,    86,   179,    -1,   179,    89,   179,    -1,
     179,    90,   179,    -1,   179,    85,   179,    -1,    91,   179,
      -1,    88,   179,    -1,    87,   179,    -1,    95,   179,    96,
      -1,   180,    -1,   178,    -1,    92,   180,    -1,     4,    -1,
       5,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   364,   364,   398,   399,   469,   475,   487,   497,   505,
     505,   549,   555,   562,   572,   573,   582,   583,   584,   608,
     609,   615,   616,   617,   618,   642,   643,   649,   650,   651,
     652,   653,   654,   658,   659,   660,   664,   668,   684,   706,
     716,   724,   732,   736,   740,   744,   755,   760,   769,   793,
     794,   795,   804,   805,   808,   809,   812,   813,   814,   815,
     816,   817,   822,   857,   858,   859,   860,   861,   862,   863,
     864,   865,   866,   869,   870,   871,   872,   873,   874,   875,
     876,   877,   878,   880,   881,   882,   883,   884,   885,   886,
     887,   889,   899,   924,   946,   948,   953,   960,   971,   985,
    1000,  1005,  1006,  1007,  1011,  1012,  1013,  1014,  1018,  1023,
    1031,  1075,  1076,  1077,  1078,  1079,  1080,  1081,  1082,  1083,
    1084,  1085,  1088,  1089,  1090,  1091,  1092,  1093,  1094,  1095,
    1096,  1097,  1099,  1100,  1101,  1102,  1103,  1104,  1105,  1106,
    1108,  1118,  1143,  1159,  1187,  1210,  1211,  1214,  1215,  1219,
    1226,  1227,  1231,  1254,  1258,  1259,  1268,  1274,  1293,  1294,
    1295,  1296,  1297,  1298,  1299,  1303,  1328,  1332,  1333,  1349,
    1355,  1375,  1376,  1380,  1388,  1399,  1400,  1404,  1410,  1418,
    1438,  1479,  1490,  1491,  1524,  1526,  1531,  1547,  1548,  1558,
    1568,  1575,  1582,  1589,  1596,  1606,  1607,  1616,  1624,  1625,
    1634,  1639,  1645,  1654,  1655,  1659,  1685,  1686,  1691,  1700,
    1701,  1711,  1726,  1727,  1728,  1729,  1732,  1733,  1734,  1738,
    1739,  1747,  1755,  1773,  1780,  1784,  1788,  1803,  1804,  1805,
    1806,  1807,  1808,  1809,  1810,  1811,  1812,  1813,  1814,  1818,
    1819,  1826,  1827,  1831,  1834,  1835,  1836,  1837,  1838,  1839,
    1840,  1841,  1842,  1843,  1844,  1845,  1849,  1850,  1853,  1854
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "tNL", "tNUMBER", "tLNUMBER", "tSTRING",
  "tIDENT", "tFILENAME", "tRAWDATA", "tACCELERATORS", "tBITMAP", "tCURSOR",
  "tDIALOG", "tDIALOGEX", "tMENU", "tMENUEX", "tMESSAGETABLE", "tRCDATA",
  "tVERSIONINFO", "tSTRINGTABLE", "tFONT", "tFONTDIR", "tICON", "tHTML",
  "tAUTO3STATE", "tAUTOCHECKBOX", "tAUTORADIOBUTTON", "tCHECKBOX",
  "tDEFPUSHBUTTON", "tPUSHBUTTON", "tRADIOBUTTON", "tSTATE3", "tGROUPBOX",
  "tCOMBOBOX", "tLISTBOX", "tSCROLLBAR", "tCONTROL", "tEDITTEXT", "tRTEXT",
  "tCTEXT", "tLTEXT", "tBLOCK", "tVALUE", "tSHIFT", "tALT", "tASCII",
  "tVIRTKEY", "tGRAYED", "tCHECKED", "tINACTIVE", "tNOINVERT", "tPURE",
  "tIMPURE", "tDISCARDABLE", "tLOADONCALL", "tPRELOAD", "tFIXED",
  "tMOVEABLE", "tCLASS", "tCAPTION", "tCHARACTERISTICS", "tEXSTYLE",
  "tSTYLE", "tVERSION", "tLANGUAGE", "tFILEVERSION", "tPRODUCTVERSION",
  "tFILEFLAGSMASK", "tFILEOS", "tFILETYPE", "tFILEFLAGS", "tFILESUBTYPE",
  "tMENUBARBREAK", "tMENUBREAK", "tMENUITEM", "tPOPUP", "tSEPARATOR",
  "tHELP", "tTOOLBAR", "tBUTTON", "tBEGIN", "tEND", "tDLGINIT", "'|'",
  "'^'", "'&'", "'+'", "'-'", "'*'", "'/'", "'~'", "tNOT", "pUPM", "','",
  "'('", "')'", "$accept", "resource_file", "resources", "resource", "@1",
  "usrcvt", "nameid", "nameid_s", "resource_definition", "filename",
  "bitmap", "cursor", "icon", "font", "fontdir", "messagetable", "html",
  "rcdata", "dlginit", "userres", "usertype", "accelerators", "events",
  "acc_opt", "accs", "acc", "dialog", "dlg_attributes", "ctrls",
  "lab_ctrl", "ctrl_desc", "iconinfo", "gen_ctrl", "opt_font",
  "optional_style_pair", "style", "ctlclass", "dialogex", "dlgex_attribs",
  "exctrls", "gen_exctrl", "lab_exctrl", "exctrl_desc", "opt_data",
  "helpid", "opt_exfont", "opt_expr", "menu", "menu_body",
  "item_definitions", "item_options", "menuex", "menuex_body",
  "itemex_definitions", "itemex_options", "itemex_p_options",
  "stringtable", "stt_head", "strings", "opt_comma", "versioninfo",
  "fix_version", "ver_blocks", "ver_block", "ver_values", "ver_value",
  "ver_words", "toolbar", "toolbar_items", "loadmemopts", "lamo", "lama",
  "opt_lvc", "opt_language", "opt_characts", "opt_version", "raw_data",
  "raw_elements", "file_raw", "e_expr", "expr", "xpr_no_not", "xpr",
  "any_num", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   124,    94,    38,    43,    45,    42,
      47,   126,   339,   340,    44,    40,    41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    97,    98,    99,    99,    99,   100,   100,   100,   101,
     100,   102,   103,   103,   104,   104,   105,   105,   105,   105,
     105,   105,   105,   105,   105,   105,   105,   105,   105,   105,
     105,   105,   105,   106,   106,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   117,   118,   119,
     119,   119,   120,   120,   121,   121,   122,   122,   122,   122,
     122,   122,   123,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   124,   125,   125,   125,   125,   125,   125,   125,
     125,   125,   125,   125,   125,   125,   125,   125,   125,   125,
     125,   125,   126,   127,   128,   128,   128,   128,   129,   129,
     130,   131,   131,   131,   132,   132,   132,   132,   133,   133,
     134,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   136,   136,   136,   136,   136,   136,   136,   136,
     136,   136,   136,   136,   136,   136,   136,   136,   136,   136,
     136,   137,   137,   138,   139,   140,   140,   141,   141,   142,
     143,   143,   144,   145,   146,   146,   146,   146,   147,   147,
     147,   147,   147,   147,   147,   148,   149,   150,   150,   150,
     150,   151,   151,   151,   151,   152,   152,   152,   152,   152,
     153,   154,   155,   155,   156,   156,   157,   158,   158,   158,
     158,   158,   158,   158,   158,   159,   159,   160,   161,   161,
     162,   162,   162,   163,   163,   164,   165,   165,   165,   166,
     166,   166,   167,   167,   167,   167,   168,   168,   168,   169,
     169,   169,   169,   170,   171,   172,   173,   174,   174,   174,
     174,   174,   174,   174,   174,   174,   174,   174,   174,   175,
     175,   176,   176,   177,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   179,   179,   180,   180
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     0,     2,     2,     3,     3,     1,     0,
       5,     0,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     1,     1,     6,     0,
       5,     5,     0,     2,     1,     3,     1,     1,     1,     1,
       1,     1,    13,     0,     3,     3,     3,     2,     3,     3,
       2,     2,     2,     0,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,    10,    12,    10,     0,     4,     6,     8,    17,    15,
       4,     0,     2,     4,     3,     3,     1,     2,     1,     1,
      14,     0,     3,     3,     3,     2,     2,     3,     3,     2,
       2,     2,     0,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
      10,    19,    16,    14,    12,     0,     1,     0,     2,     9,
       0,     2,     4,     3,     0,     6,     3,     5,     0,     3,
       3,     3,     3,     3,     3,     4,     3,     0,     4,     3,
       5,     0,     2,     5,     6,     0,     2,     4,     6,     8,
       4,     3,     0,     4,     0,     1,     6,     0,     9,     9,
       3,     3,     3,     3,     3,     0,     2,     5,     0,     2,
       1,     4,     4,     1,     3,     9,     0,     3,     2,     0,
       2,     2,     1,     1,     1,     1,     1,     1,     1,     0,
       2,     2,     2,     4,     2,     2,     4,     1,     1,     2,
       1,     2,     1,     3,     3,     4,     3,     4,     3,     1,
       1,     0,     1,     1,     3,     3,     3,     3,     3,     3,
       3,     2,     2,     2,     3,     1,     1,     2,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short yydefact[] =
{
       3,     0,     2,     1,     5,   258,   259,    11,   209,     9,
       0,     0,     0,     0,     0,     4,     8,     0,    11,   256,
     243,   255,     0,   219,     0,   253,   252,   251,   257,     0,
     182,     0,     0,     0,     0,     0,     0,     0,     0,    46,
      47,   209,   209,   209,   209,   209,   209,   209,   209,   209,
     209,   209,   209,   209,   209,   209,   209,     7,    17,    18,
      24,    22,    23,    27,    28,    29,    21,    31,   209,    16,
      19,    20,    25,    26,    32,    30,   215,   218,   214,   216,
     212,   217,   213,   210,   211,   181,     0,   254,     0,     6,
     246,   250,   247,   244,   245,   248,   249,   219,   219,   219,
       0,     0,   219,   219,   219,   219,   187,   219,   219,   219,
     219,     0,   219,   219,     0,     0,     0,   220,   221,   222,
       0,   180,   184,     0,    35,    34,    33,   239,     0,   240,
      36,    37,     0,     0,     0,     0,    41,    43,     0,    39,
      40,    38,    42,     0,    44,    45,   224,   225,     0,    10,
     185,     0,    49,     0,     0,     0,   154,   152,   167,   165,
       0,     0,     0,     0,     0,     0,     0,   195,     0,     0,
     183,     0,   228,   230,   232,   227,     0,   184,     0,     0,
       0,     0,     0,     0,   191,   192,   193,   190,   194,     0,
     219,   223,     0,    48,     0,   229,   231,   226,     0,     0,
       0,     0,     0,   153,     0,     0,   166,     0,     0,     0,
     186,   196,     0,     0,     0,   234,   236,   238,   233,     0,
       0,     0,   184,   156,   184,   171,   169,   175,     0,     0,
       0,   206,    52,    52,   235,   237,     0,     0,     0,     0,
       0,   241,   168,   241,     0,     0,     0,   198,     0,     0,
      50,    51,    63,   147,   184,   157,   184,   184,   184,   184,
     184,   184,     0,   172,     0,   176,   170,     0,     0,     0,
     208,     0,   205,    58,    57,    59,    60,    61,    56,    53,
      54,     0,     0,   111,   155,   160,   159,   162,   163,   164,
     161,   241,   241,     0,     0,     0,   197,   200,   199,   207,
       0,     0,     0,     0,     0,     0,     0,    73,    67,    70,
      71,    72,   148,     0,   184,   242,     0,   177,   188,   189,
       0,    55,    13,    69,    12,     0,    15,    14,    68,    66,
       0,     0,    65,   106,     0,    64,     0,     0,     0,     0,
       0,     0,     0,   122,   115,   116,   119,   120,   121,   185,
     173,   241,     0,     0,   107,   255,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    62,   118,     0,   117,
     114,   113,   112,     0,   174,     0,   178,   201,   202,   203,
     100,   105,   104,   184,   184,    84,    86,    87,    79,    80,
      82,    83,    85,    81,    77,     0,    76,    78,   184,    74,
      75,    90,    89,    88,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   110,     0,     0,     0,     0,     0,     0,
     100,   184,   184,   133,   135,   136,   128,   129,   131,   132,
     134,   130,   126,     0,   125,   127,   184,   123,   124,   139,
     138,   137,   179,   204,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   109,     0,   108,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   150,     0,     0,
       0,     0,    94,     0,     0,     0,     0,   149,     0,     0,
       0,     0,     0,    91,     0,     0,     0,   151,    94,     0,
       0,     0,     0,     0,   101,     0,   140,     0,     0,     0,
       0,     0,     0,    93,     0,     0,   101,     0,    95,   101,
     102,     0,     0,   147,     0,     0,    92,     0,     0,   101,
     145,     0,    96,   103,     0,   147,   144,   146,     0,     0,
       0,   145,     0,    97,    99,   143,     0,     0,   145,    98,
       0,   142,   147,   145,   141
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,     1,     2,    15,    24,    22,   327,   328,    57,   127,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,   171,   250,   279,   280,    70,   281,   336,   395,
     404,   513,   409,   308,   533,   332,   486,    71,   313,   383,
     457,   443,   452,   556,   283,   345,   507,    72,   157,   180,
     239,    73,   159,   181,   242,   244,    16,    17,    88,   240,
      74,   138,   189,   211,   269,   298,   388,    75,   248,    23,
      83,    84,   128,   117,   118,   119,   129,   177,   130,   262,
     324,    19,    20,    21
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -472
static const short yypact[] =
{
    -472,    17,     1,  -472,  -472,  -472,  -472,  -472,  -472,  -472,
      86,    86,    86,   140,    86,  -472,  -472,   -55,  -472,  -472,
     610,  -472,   376,   649,    86,  -472,  -472,  -472,  -472,   597,
    -472,   376,    86,    86,    86,    86,    86,    86,    86,  -472,
    -472,  -472,  -472,  -472,  -472,  -472,  -472,  -472,  -472,  -472,
    -472,  -472,  -472,  -472,  -472,  -472,  -472,  -472,  -472,  -472,
    -472,  -472,  -472,  -472,  -472,  -472,  -472,  -472,  -472,  -472,
    -472,  -472,  -472,  -472,  -472,  -472,  -472,  -472,  -472,  -472,
    -472,  -472,  -472,  -472,  -472,   268,    21,  -472,    59,  -472,
     480,   409,   327,   -78,   -78,  -472,  -472,   649,   395,   395,
      48,    48,   649,   649,   395,   395,   649,   395,   395,   395,
     395,    48,   395,   395,    86,    86,    86,  -472,  -472,  -472,
      86,  -472,    44,    67,  -472,  -472,  -472,  -472,   156,  -472,
    -472,  -472,    55,    62,   240,   247,  -472,  -472,   413,  -472,
    -472,  -472,  -472,    71,  -472,  -472,  -472,  -472,    78,  -472,
    -472,    68,  -472,   187,    86,    86,  -472,  -472,  -472,  -472,
      86,    86,    86,    86,    86,    86,    86,  -472,    86,    86,
    -472,    29,  -472,  -472,  -472,  -472,   183,   -21,    82,    85,
     249,   262,   108,   114,  -472,  -472,  -472,  -472,  -472,    12,
    -472,  -472,   128,  -472,   137,  -472,  -472,  -472,   195,    86,
      86,     3,   117,  -472,     7,   236,  -472,    86,    86,   243,
    -472,  -472,   278,    86,    86,  -472,  -472,  -472,  -472,   282,
     159,   160,    44,  -472,   -71,   168,  -472,   174,   188,   203,
     191,  -472,   220,   220,  -472,  -472,    86,    86,    86,   241,
     178,    86,  -472,    86,   259,    86,    86,  -472,   -10,   472,
    -472,  -472,  -472,   223,   182,  -472,   364,   364,   364,   364,
     364,   364,   254,   264,   266,   264,  -472,   270,   271,    15,
    -472,    86,  -472,  -472,  -472,  -472,  -472,  -472,  -472,   281,
    -472,   292,    86,  -472,  -472,  -472,  -472,  -472,  -472,  -472,
    -472,    86,    86,    86,    86,   324,  -472,  -472,  -472,  -472,
     472,   211,    86,   179,   356,   228,   228,  -472,  -472,  -472,
    -472,  -472,  -472,   348,   381,  -472,   285,   264,  -472,  -472,
     302,  -472,  -472,  -472,  -472,   310,  -472,  -472,  -472,  -472,
      86,   228,   300,   623,   610,   300,   550,   211,    86,   179,
     370,   228,   228,  -472,  -472,  -472,  -472,  -472,  -472,    86,
    -472,    86,    42,   399,   623,  -472,   -15,   228,   179,   400,
     400,   400,   400,   400,   400,   400,   400,   400,    86,    86,
      86,   179,    86,   400,   400,   400,  -472,  -472,   325,  -472,
    -472,   300,   300,   569,  -472,   326,   264,  -472,   332,  -472,
    -472,  -472,  -472,    44,    44,  -472,  -472,  -472,  -472,  -472,
    -472,  -472,  -472,  -472,  -472,   334,  -472,  -472,    44,  -472,
    -472,  -472,  -472,  -472,   424,   179,   425,   425,   425,   425,
     425,   425,   425,   425,   425,    86,    86,    86,   179,    86,
     425,   425,   425,  -472,    86,    86,    86,    86,    86,    86,
     347,    44,    44,  -472,  -472,  -472,  -472,  -472,  -472,  -472,
    -472,  -472,  -472,   349,  -472,  -472,    44,  -472,  -472,  -472,
    -472,  -472,  -472,  -472,   350,   360,   366,   371,    86,    86,
      86,    86,    86,    86,    86,    86,   290,   382,   383,   384,
     407,   408,   411,   417,   426,  -472,   427,  -472,    86,    86,
      86,    86,   290,    86,    86,    86,   228,   428,   430,   436,
     437,   439,   440,   441,   443,   125,    86,  -472,    86,    86,
      86,   228,    86,  -472,    86,    86,    86,  -472,   440,   444,
     445,   129,   447,   449,   450,   451,  -472,    86,    86,    86,
      86,    86,   228,  -472,    86,   453,   450,   454,   456,   450,
     146,   458,    86,   223,    86,   228,  -472,   228,    86,   450,
     357,   459,   154,   300,   460,   223,  -472,  -472,    86,   228,
      86,   357,   461,   300,   463,  -472,    86,   228,    61,   300,
     228,  -472,   161,   357,  -472
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -472,  -472,  -472,  -472,  -472,   485,  -258,  -168,   528,  -472,
    -472,  -472,  -472,  -472,  -472,  -472,  -472,  -472,  -472,  -472,
    -472,  -472,  -472,   209,  -472,   260,  -472,  -472,  -472,   279,
       2,    43,  -472,   280,  -471,  -282,    70,  -472,  -472,  -472,
    -472,   238,    35,  -379,  -348,  -472,  -472,  -472,   333,  -472,
    -229,  -472,   385,  -472,  -472,  -472,  -472,  -472,  -472,  -115,
    -472,  -472,  -472,   358,  -472,  -472,  -472,  -472,  -472,   570,
    -472,  -472,   -20,  -205,  -194,  -186,  -393,  -472,   567,  -241,
      -2,  -286,     4,   -12
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -257
static const short yytable[] =
{
      18,    28,   264,    85,     4,     5,     6,   151,     7,   222,
    -158,    37,    38,   225,    25,    26,    27,     3,    29,   333,
     333,     8,    86,   150,   335,   284,    30,   285,   286,   287,
     288,   289,   290,     5,     6,   192,    90,    91,    92,    93,
      94,    95,    96,   323,   354,   333,     5,     6,   387,   356,
     314,   316,     5,     6,   209,   333,   333,   209,   295,   381,
     382,   197,   198,     5,     6,   543,     9,   270,   546,   357,
     271,   333,   272,   150,   170,   392,   309,   123,   555,   377,
     223,   391,   134,   135,   226,   350,   122,   310,    10,    11,
       5,     6,    12,    13,   210,   311,    14,   296,   132,   133,
      76,    77,    78,    79,    80,    81,    82,   238,   346,   143,
     385,   193,   146,   147,   148,   120,    10,    11,   149,   347,
      12,    13,  -219,   224,    14,  -219,  -219,   348,   114,    10,
      11,   115,   116,    12,    13,    10,    11,    14,   150,    12,
      13,   121,  -219,    14,     5,     6,    10,    11,   152,   154,
      12,    13,   178,   179,    14,   570,   155,   557,   182,   183,
     184,   185,   186,   187,   188,   168,   190,   191,   557,   194,
     212,   379,   169,    10,    11,   557,   199,    12,    13,   200,
     557,    14,   565,     5,     6,   326,   322,   195,   196,   571,
     393,   172,   173,   174,   574,   550,   175,   220,   221,   215,
     216,   217,   207,   408,   218,   228,   229,   561,   208,   357,
     333,   232,   233,   357,   505,     5,     6,   114,   322,   516,
     115,   116,   213,   529,   573,   333,   256,   257,   258,   521,
     357,   214,     5,     6,   252,   253,   254,   153,   357,   263,
     547,   265,   227,   267,   268,   357,   333,   441,   559,   230,
     540,   259,   260,   236,   237,   282,   261,  -158,  -158,   333,
     456,   333,   241,   552,  -158,   553,    10,    11,   243,   299,
      12,    13,   247,   333,    14,   176,   150,   563,   436,   437,
     312,   333,   245,   219,   333,   569,   234,   235,   572,   315,
     317,   318,   319,   439,     5,     6,   485,   246,    10,    11,
     325,   114,    12,    13,   115,   116,    14,   301,   114,   334,
     334,   115,   116,   302,   249,    10,    11,   282,   355,    12,
     330,   156,   156,   331,   201,   202,   469,   470,   158,   114,
     320,   203,   115,   116,   334,    29,   378,   204,   205,   114,
     158,   472,   115,   116,   206,   334,   334,   384,   291,   386,
     389,   303,   304,   114,   305,   306,   115,   116,  -242,   231,
     292,   334,   329,   337,   293,   294,   405,   405,   405,   338,
     405,   406,   407,   307,   410,   300,   380,    10,    11,   351,
      39,    12,    13,    40,   357,    14,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,   352,    51,    52,    53,
      54,   124,   125,   126,   353,   390,   394,   339,   340,   114,
     341,   342,   115,   116,    35,    36,    37,    38,  -219,   414,
     434,  -219,  -219,   453,   453,   453,   435,   453,   438,   343,
     440,   442,   462,   463,   464,   465,   466,   467,  -219,  -158,
    -158,   468,   251,   471,   473,  -158,  -158,    76,    77,    78,
      79,    80,    81,    82,   474,    55,  -158,  -158,   150,    56,
     475,   454,   455,  -158,   458,   476,   477,   478,   479,   480,
     481,   482,   483,   484,   487,   349,   488,   489,   490,   160,
     161,   162,   163,   164,   165,   166,   497,   498,   499,   500,
     487,   502,   503,   504,   167,    34,    35,    36,    37,    38,
     334,   491,   492,    31,   517,   493,   518,   519,   520,   273,
     522,   494,   523,   524,   525,   334,   274,   275,   276,   277,
     495,   496,   506,   278,   508,   535,   536,   537,   538,   539,
     509,   510,   541,   511,   512,   514,   334,   515,   527,   528,
     549,   530,   551,   531,   532,   534,   554,   542,   544,   334,
     545,   334,   548,   558,   560,   566,   562,   567,   564,    89,
     321,   526,   501,   334,   568,    33,    34,    35,    36,    37,
      38,   334,   255,   358,   334,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,   369,   370,   371,   372,   373,
     374,   375,   415,   344,   416,   417,   418,   419,   420,   421,
     422,   423,   424,   425,   426,   427,   428,   429,   430,   431,
     432,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   297,     0,   266,
       0,     0,   376,     0,     0,     0,     0,     0,   113,   396,
     397,   398,   399,   400,   401,   402,   403,     0,     0,     0,
       0,   433,   411,   412,   413,   444,   445,   446,   447,   448,
     449,   450,   451,     0,     0,     0,   131,     0,   459,   460,
     461,   136,   137,     0,   139,   140,   141,   142,     0,   144,
     145,    32,    33,    34,    35,    36,    37,    38,     0,     0,
       0,     0,     0,    87,    32,    33,    34,    35,    36,    37,
      38,    76,    77,    78,    79,    80,    81,    82,  -256,  -256,
    -256,  -256,  -256,  -256
};

static const short yycheck[] =
{
       2,    13,   243,    23,     3,     4,     5,   122,     7,     6,
      81,    89,    90,     6,    10,    11,    12,     0,    14,   305,
     306,    20,    24,    94,   306,   254,    81,   256,   257,   258,
     259,   260,   261,     4,     5,     6,    32,    33,    34,    35,
      36,    37,    38,   301,   330,   331,     4,     5,     6,   331,
     291,   292,     4,     5,    42,   341,   342,    42,    43,   341,
     342,    82,   177,     4,     5,   536,    65,    77,   539,    84,
      80,   357,    82,    94,     6,   357,   281,    97,   549,   337,
      77,    96,   102,   103,    77,   314,    88,   281,    87,    88,
       4,     5,    91,    92,    82,   281,    95,    82,   100,   101,
      52,    53,    54,    55,    56,    57,    58,   222,   313,   111,
     351,    82,   114,   115,   116,    94,    87,    88,   120,   313,
      91,    92,    61,     6,    95,    64,    65,   313,    61,    87,
      88,    64,    65,    91,    92,    87,    88,    95,    94,    91,
      92,    82,    81,    95,     4,     5,    87,    88,    81,    94,
      91,    92,   154,   155,    95,    94,    94,   550,   160,   161,
     162,   163,   164,   165,   166,    94,   168,   169,   561,   171,
     190,   339,    94,    87,    88,   568,    94,    91,    92,    94,
     573,    95,   561,     4,     5,     6,     7,     4,     5,   568,
     358,     4,     5,     6,   573,   543,     9,   199,   200,     4,
       5,     6,    94,   371,     9,   207,   208,   555,    94,    84,
     496,   213,   214,    84,   496,     4,     5,    61,     7,    94,
      64,    65,    94,    94,   572,   511,    48,    49,    50,   511,
      84,    94,     4,     5,   236,   237,   238,    81,    84,   241,
      94,   243,     6,   245,   246,    84,   532,   415,    94,     6,
     532,    73,    74,    94,    94,    94,    78,    75,    76,   545,
     428,   547,    94,   545,    82,   547,    87,    88,    94,   271,
      91,    92,    81,   559,    95,    88,    94,   559,   393,   394,
     282,   567,    94,    88,   570,   567,     4,     5,   570,   291,
     292,   293,   294,   408,     4,     5,     6,    94,    87,    88,
     302,    61,    91,    92,    64,    65,    95,    15,    61,   305,
     306,    64,    65,    21,    94,    87,    88,    94,   330,    91,
      92,    81,    81,    95,    75,    76,   441,   442,    81,    61,
       6,    82,    64,    65,   330,   331,   338,    75,    76,    61,
      81,   456,    64,    65,    82,   341,   342,   349,    94,   351,
     352,    59,    60,    61,    62,    63,    64,    65,    94,    81,
      94,   357,     6,    15,    94,    94,   368,   369,   370,    21,
     372,   369,   370,    81,   372,    94,     6,    87,    88,    94,
       4,    91,    92,     7,    84,    95,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    94,    21,    22,    23,
      24,     6,     7,     8,    94,     6,     6,    59,    60,    61,
      62,    63,    64,    65,    87,    88,    89,    90,    61,    94,
      94,    64,    65,   425,   426,   427,    94,   429,    94,    81,
       6,     6,   434,   435,   436,   437,   438,   439,    81,    75,
      76,    94,   233,    94,    94,    81,    82,    52,    53,    54,
      55,    56,    57,    58,    94,    79,    75,    76,    94,    83,
      94,   426,   427,    82,   429,    94,   468,   469,   470,   471,
     472,   473,   474,   475,   476,    94,    94,    94,    94,    66,
      67,    68,    69,    70,    71,    72,   488,   489,   490,   491,
     492,   493,   494,   495,    81,    86,    87,    88,    89,    90,
     496,    94,    94,    18,   506,    94,   508,   509,   510,    37,
     512,    94,   514,   515,   516,   511,    44,    45,    46,    47,
      94,    94,    94,    51,    94,   527,   528,   529,   530,   531,
      94,    94,   534,    94,    94,    94,   532,    94,    94,    94,
     542,    94,   544,    94,    94,    94,   548,    94,    94,   545,
      94,   547,    94,    94,    94,    94,   558,    94,   560,    31,
     300,   518,   492,   559,   566,    85,    86,    87,    88,    89,
      90,   567,   239,    23,   570,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    23,   313,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,   269,    -1,   244,
      -1,    -1,    82,    -1,    -1,    -1,    -1,    -1,    68,   360,
     361,   362,   363,   364,   365,   366,   367,    -1,    -1,    -1,
      -1,    82,   373,   374,   375,   417,   418,   419,   420,   421,
     422,   423,   424,    -1,    -1,    -1,    99,    -1,   430,   431,
     432,   104,   105,    -1,   107,   108,   109,   110,    -1,   112,
     113,    84,    85,    86,    87,    88,    89,    90,    -1,    -1,
      -1,    -1,    -1,    96,    84,    85,    86,    87,    88,    89,
      90,    52,    53,    54,    55,    56,    57,    58,    85,    86,
      87,    88,    89,    90
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    98,    99,     0,     3,     4,     5,     7,    20,    65,
      87,    88,    91,    92,    95,   100,   153,   154,   177,   178,
     179,   180,   102,   166,   101,   179,   179,   179,   180,   179,
      81,   102,    84,    85,    86,    87,    88,    89,    90,     4,
       7,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    21,    22,    23,    24,    79,    83,   105,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     123,   134,   144,   148,   157,   164,    52,    53,    54,    55,
      56,    57,    58,   167,   168,   169,   177,    96,   155,   105,
     179,   179,   179,   179,   179,   179,   179,   166,   166,   166,
     166,   166,   166,   166,   166,   166,   166,   166,   166,   166,
     166,   166,   166,   166,    61,    64,    65,   170,   171,   172,
      94,    82,   177,   169,     6,     7,     8,   106,   169,   173,
     175,   175,   177,   177,   169,   169,   175,   175,   158,   175,
     175,   175,   175,   177,   175,   175,   177,   177,   177,   177,
      94,   156,    81,    81,    94,    94,    81,   145,    81,   149,
      66,    67,    68,    69,    70,    71,    72,    81,    94,    94,
       6,   119,     4,     5,     6,     9,    88,   174,   177,   177,
     146,   150,   177,   177,   177,   177,   177,   177,   177,   159,
     177,   177,     6,    82,   177,     4,     5,    82,   156,    94,
      94,    75,    76,    82,    75,    76,    82,    94,    94,    42,
      82,   160,   169,    94,    94,     4,     5,     6,     9,    88,
     177,   177,     6,    77,     6,     6,    77,     6,   177,   177,
       6,    81,   177,   177,     4,     5,    94,    94,   156,   147,
     156,    94,   151,    94,   152,    94,    94,    81,   165,    94,
     120,   120,   177,   177,   177,   145,    48,    49,    50,    73,
      74,    78,   176,   177,   176,   177,   149,   177,   177,   161,
      77,    80,    82,    37,    44,    45,    46,    47,    51,   121,
     122,   124,    94,   141,   147,   147,   147,   147,   147,   147,
     147,    94,    94,    94,    94,    43,    82,   160,   162,   177,
      94,    15,    21,    59,    60,    62,    63,    81,   130,   170,
     171,   172,   177,   135,   176,   177,   176,   177,   177,   177,
       6,   122,     7,   103,   177,   177,     6,   103,   104,     6,
      92,    95,   132,   178,   179,   132,   125,    15,    21,    59,
      60,    62,    63,    81,   130,   142,   170,   171,   172,    94,
     147,    94,    94,    94,   178,   180,   132,    84,    23,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    82,   103,   177,   104,
       6,   132,   132,   136,   177,   176,   177,     6,   163,   177,
       6,    96,   132,   104,     6,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   127,   177,   127,   127,   104,   129,
     127,   126,   126,   126,    94,    23,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    82,    94,    94,   156,   156,    94,   156,
       6,   104,     6,   138,   138,   138,   138,   138,   138,   138,
     138,   138,   139,   177,   139,   139,   104,   137,   139,   138,
     138,   138,   177,   177,   177,   177,   177,   177,    94,   156,
     156,    94,   156,    94,    94,    94,    94,   177,   177,   177,
     177,   177,   177,   177,   177,     6,   133,   177,    94,    94,
      94,    94,    94,    94,    94,    94,    94,   177,   177,   177,
     177,   133,   177,   177,   177,   132,    94,   143,    94,    94,
      94,    94,    94,   128,    94,    94,    94,   177,   177,   177,
     177,   132,   177,   177,   177,   177,   128,    94,    94,    94,
      94,    94,    94,   131,    94,   177,   177,   177,   177,   177,
     132,   177,    94,   131,    94,    94,   131,    94,    94,   177,
     141,   177,   132,   132,   177,   131,   140,   173,    94,    94,
      94,   141,   177,   132,   177,   140,    94,    94,   177,   132,
      94,   140,   132,   141,   140
};

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
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


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
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)		\
   ((Current).first_line   = (Rhs)[1].first_line,	\
    (Current).first_column = (Rhs)[1].first_column,	\
    (Current).last_line    = (Rhs)[N].last_line,	\
    (Current).last_column  = (Rhs)[N].last_column)
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

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

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
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

#if defined (YYMAXDEPTH) && YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

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

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
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

  if (yyss + yystacksize - 1 <= yyssp)
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
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
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
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


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

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 364 "parser.y"
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
		;}
    break;

  case 3:
#line 398 "parser.y"
    { yyval.res = NULL; want_id = 1; ;}
    break;

  case 4:
#line 399 "parser.y"
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
		;}
    break;

  case 6:
#line 475 "parser.y"
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
			;}
    break;

  case 7:
#line 487 "parser.y"
    {
		yyval.res = yyvsp[0].res;
		if(yyval.res)
		{
			yyval.res->name = new_name_id();
			yyval.res->name->type = name_str;
			yyval.res->name->name.s_name = yyvsp[-2].str;
			chat("Got %s (%s)", get_typename(yyvsp[0].res), yyval.res->name->name.s_name->str.cstr);
		}
		;}
    break;

  case 8:
#line 497 "parser.y"
    {
		/* Don't do anything, stringtables are converted to
		 * resource_t structures when we are finished parsing and
		 * the final rule of the parser is reduced (see above)
		 */
		yyval.res = NULL;
		chat("Got STRINGTABLE");
		;}
    break;

  case 9:
#line 505 "parser.y"
    {want_nl = 1; ;}
    break;

  case 10:
#line 505 "parser.y"
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
		;}
    break;

  case 11:
#line 549 "parser.y"
    { yychar = rsrcid_to_token(yychar); ;}
    break;

  case 12:
#line 555 "parser.y"
    {
		if(yyvsp[0].num > 65535 || yyvsp[0].num < -32768)
			yyerror("Resource's ID out of range (%d)", yyvsp[0].num);
		yyval.nid = new_name_id();
		yyval.nid->type = name_ord;
		yyval.nid->name.i_name = yyvsp[0].num;
		;}
    break;

  case 13:
#line 562 "parser.y"
    {
		yyval.nid = new_name_id();
		yyval.nid->type = name_str;
		yyval.nid->name.s_name = yyvsp[0].str;
		;}
    break;

  case 14:
#line 572 "parser.y"
    { yyval.nid = yyvsp[0].nid; ;}
    break;

  case 15:
#line 573 "parser.y"
    {
		yyval.nid = new_name_id();
		yyval.nid->type = name_str;
		yyval.nid->name.s_name = yyvsp[0].str;
		;}
    break;

  case 16:
#line 582 "parser.y"
    { yyval.res = new_resource(res_acc, yyvsp[0].acc, yyvsp[0].acc->memopt, yyvsp[0].acc->lvc.language); ;}
    break;

  case 17:
#line 583 "parser.y"
    { yyval.res = new_resource(res_bmp, yyvsp[0].bmp, yyvsp[0].bmp->memopt, yyvsp[0].bmp->data->lvc.language); ;}
    break;

  case 18:
#line 584 "parser.y"
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
		;}
    break;

  case 19:
#line 608 "parser.y"
    { yyval.res = new_resource(res_dlg, yyvsp[0].dlg, yyvsp[0].dlg->memopt, yyvsp[0].dlg->lvc.language); ;}
    break;

  case 20:
#line 609 "parser.y"
    {
		if(win32)
			yyval.res = new_resource(res_dlgex, yyvsp[0].dlgex, yyvsp[0].dlgex->memopt, yyvsp[0].dlgex->lvc.language);
		else
			yyval.res = NULL;
		;}
    break;

  case 21:
#line 615 "parser.y"
    { yyval.res = new_resource(res_dlginit, yyvsp[0].dginit, yyvsp[0].dginit->memopt, yyvsp[0].dginit->data->lvc.language); ;}
    break;

  case 22:
#line 616 "parser.y"
    { yyval.res = new_resource(res_fnt, yyvsp[0].fnt, yyvsp[0].fnt->memopt, yyvsp[0].fnt->data->lvc.language); ;}
    break;

  case 23:
#line 617 "parser.y"
    { yyval.res = new_resource(res_fntdir, yyvsp[0].fnd, yyvsp[0].fnd->memopt, yyvsp[0].fnd->data->lvc.language); ;}
    break;

  case 24:
#line 618 "parser.y"
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
		;}
    break;

  case 25:
#line 642 "parser.y"
    { yyval.res = new_resource(res_men, yyvsp[0].men, yyvsp[0].men->memopt, yyvsp[0].men->lvc.language); ;}
    break;

  case 26:
#line 643 "parser.y"
    {
		if(win32)
			yyval.res = new_resource(res_menex, yyvsp[0].menex, yyvsp[0].menex->memopt, yyvsp[0].menex->lvc.language);
		else
			yyval.res = NULL;
		;}
    break;

  case 27:
#line 649 "parser.y"
    { yyval.res = new_resource(res_msg, yyvsp[0].msg, WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE, yyvsp[0].msg->data->lvc.language); ;}
    break;

  case 28:
#line 650 "parser.y"
    { yyval.res = new_resource(res_html, yyvsp[0].html, yyvsp[0].html->memopt, yyvsp[0].html->data->lvc.language); ;}
    break;

  case 29:
#line 651 "parser.y"
    { yyval.res = new_resource(res_rdt, yyvsp[0].rdt, yyvsp[0].rdt->memopt, yyvsp[0].rdt->data->lvc.language); ;}
    break;

  case 30:
#line 652 "parser.y"
    { yyval.res = new_resource(res_toolbar, yyvsp[0].tlbar, yyvsp[0].tlbar->memopt, yyvsp[0].tlbar->lvc.language); ;}
    break;

  case 31:
#line 653 "parser.y"
    { yyval.res = new_resource(res_usr, yyvsp[0].usr, yyvsp[0].usr->memopt, yyvsp[0].usr->data->lvc.language); ;}
    break;

  case 32:
#line 654 "parser.y"
    { yyval.res = new_resource(res_ver, yyvsp[0].veri, WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE, yyvsp[0].veri->lvc.language); ;}
    break;

  case 33:
#line 658 "parser.y"
    { yyval.str = make_filename(yyvsp[0].str); ;}
    break;

  case 34:
#line 659 "parser.y"
    { yyval.str = make_filename(yyvsp[0].str); ;}
    break;

  case 35:
#line 660 "parser.y"
    { yyval.str = make_filename(yyvsp[0].str); ;}
    break;

  case 36:
#line 664 "parser.y"
    { yyval.bmp = new_bitmap(yyvsp[0].raw, yyvsp[-1].iptr); ;}
    break;

  case 37:
#line 668 "parser.y"
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
	;}
    break;

  case 38:
#line 684 "parser.y"
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
	;}
    break;

  case 39:
#line 706 "parser.y"
    { yyval.fnt = new_font(yyvsp[0].raw, yyvsp[-1].iptr); ;}
    break;

  case 40:
#line 716 "parser.y"
    { yyval.fnd = new_fontdir(yyvsp[0].raw, yyvsp[-1].iptr); ;}
    break;

  case 41:
#line 724 "parser.y"
    {
		if(!win32)
			yywarning("MESSAGETABLE not supported in 16-bit mode");
		yyval.msg = new_messagetable(yyvsp[0].raw, yyvsp[-1].iptr);
		;}
    break;

  case 42:
#line 732 "parser.y"
    { yyval.html = new_html(yyvsp[0].raw, yyvsp[-1].iptr); ;}
    break;

  case 43:
#line 736 "parser.y"
    { yyval.rdt = new_rcdata(yyvsp[0].raw, yyvsp[-1].iptr); ;}
    break;

  case 44:
#line 740 "parser.y"
    { yyval.dginit = new_dlginit(yyvsp[0].raw, yyvsp[-1].iptr); ;}
    break;

  case 45:
#line 744 "parser.y"
    {
#ifdef WORDS_BIGENDIAN
			if(pedantic && byteorder != WRC_BO_LITTLE)
#else
			if(pedantic && byteorder == WRC_BO_BIG)
#endif
				yywarning("Byteordering is not little-endian and type cannot be interpreted");
			yyval.usr = new_user(yyvsp[-2].nid, yyvsp[0].raw, yyvsp[-1].iptr);
		;}
    break;

  case 46:
#line 755 "parser.y"
    {
		yyval.nid = new_name_id();
		yyval.nid->type = name_ord;
		yyval.nid->name.i_name = yyvsp[0].num;
		;}
    break;

  case 47:
#line 760 "parser.y"
    {
		yyval.nid = new_name_id();
		yyval.nid->type = name_str;
		yyval.nid->name.s_name = yyvsp[0].str;
		;}
    break;

  case 48:
#line 769 "parser.y"
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
		;}
    break;

  case 49:
#line 793 "parser.y"
    { yyval.event=NULL; ;}
    break;

  case 50:
#line 794 "parser.y"
    { yyval.event=add_string_event(yyvsp[-3].str, yyvsp[-1].num, yyvsp[0].num, yyvsp[-4].event); ;}
    break;

  case 51:
#line 795 "parser.y"
    { yyval.event=add_event(yyvsp[-3].num, yyvsp[-1].num, yyvsp[0].num, yyvsp[-4].event); ;}
    break;

  case 52:
#line 804 "parser.y"
    { yyval.num = 0; ;}
    break;

  case 53:
#line 805 "parser.y"
    { yyval.num = yyvsp[0].num; ;}
    break;

  case 54:
#line 808 "parser.y"
    { yyval.num = yyvsp[0].num; ;}
    break;

  case 55:
#line 809 "parser.y"
    { yyval.num = yyvsp[-2].num | yyvsp[0].num; ;}
    break;

  case 56:
#line 812 "parser.y"
    { yyval.num = WRC_AF_NOINVERT; ;}
    break;

  case 57:
#line 813 "parser.y"
    { yyval.num = WRC_AF_SHIFT; ;}
    break;

  case 58:
#line 814 "parser.y"
    { yyval.num = WRC_AF_CONTROL; ;}
    break;

  case 59:
#line 815 "parser.y"
    { yyval.num = WRC_AF_ALT; ;}
    break;

  case 60:
#line 816 "parser.y"
    { yyval.num = WRC_AF_ASCII; ;}
    break;

  case 61:
#line 817 "parser.y"
    { yyval.num = WRC_AF_VIRTKEY; ;}
    break;

  case 62:
#line 823 "parser.y"
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
		;}
    break;

  case 63:
#line 857 "parser.y"
    { yyval.dlg=new_dialog(); ;}
    break;

  case 64:
#line 858 "parser.y"
    { yyval.dlg=dialog_style(yyvsp[0].style,yyvsp[-2].dlg); ;}
    break;

  case 65:
#line 859 "parser.y"
    { yyval.dlg=dialog_exstyle(yyvsp[0].style,yyvsp[-2].dlg); ;}
    break;

  case 66:
#line 860 "parser.y"
    { yyval.dlg=dialog_caption(yyvsp[0].str,yyvsp[-2].dlg); ;}
    break;

  case 67:
#line 861 "parser.y"
    { yyval.dlg=dialog_font(yyvsp[0].fntid,yyvsp[-1].dlg); ;}
    break;

  case 68:
#line 862 "parser.y"
    { yyval.dlg=dialog_class(yyvsp[0].nid,yyvsp[-2].dlg); ;}
    break;

  case 69:
#line 863 "parser.y"
    { yyval.dlg=dialog_menu(yyvsp[0].nid,yyvsp[-2].dlg); ;}
    break;

  case 70:
#line 864 "parser.y"
    { yyval.dlg=dialog_language(yyvsp[0].lan,yyvsp[-1].dlg); ;}
    break;

  case 71:
#line 865 "parser.y"
    { yyval.dlg=dialog_characteristics(yyvsp[0].chars,yyvsp[-1].dlg); ;}
    break;

  case 72:
#line 866 "parser.y"
    { yyval.dlg=dialog_version(yyvsp[0].ver,yyvsp[-1].dlg); ;}
    break;

  case 73:
#line 869 "parser.y"
    { yyval.ctl = NULL; ;}
    break;

  case 74:
#line 870 "parser.y"
    { yyval.ctl=ins_ctrl(-1, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 75:
#line 871 "parser.y"
    { yyval.ctl=ins_ctrl(CT_EDIT, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 76:
#line 872 "parser.y"
    { yyval.ctl=ins_ctrl(CT_LISTBOX, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 77:
#line 873 "parser.y"
    { yyval.ctl=ins_ctrl(CT_COMBOBOX, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 78:
#line 874 "parser.y"
    { yyval.ctl=ins_ctrl(CT_SCROLLBAR, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 79:
#line 875 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_CHECKBOX, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 80:
#line 876 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_DEFPUSHBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 81:
#line 877 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_GROUPBOX, yyvsp[0].ctl, yyvsp[-2].ctl);;}
    break;

  case 82:
#line 878 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_PUSHBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 83:
#line 880 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_RADIOBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 84:
#line 881 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_AUTO3STATE, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 85:
#line 882 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_3STATE, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 86:
#line 883 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_AUTOCHECKBOX, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 87:
#line 884 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_AUTORADIOBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 88:
#line 885 "parser.y"
    { yyval.ctl=ins_ctrl(CT_STATIC, SS_LEFT, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 89:
#line 886 "parser.y"
    { yyval.ctl=ins_ctrl(CT_STATIC, SS_CENTER, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 90:
#line 887 "parser.y"
    { yyval.ctl=ins_ctrl(CT_STATIC, SS_RIGHT, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 91:
#line 889 "parser.y"
    {
		yyvsp[0].ctl->title = yyvsp[-7].nid;
		yyvsp[0].ctl->id = yyvsp[-5].num;
		yyvsp[0].ctl->x = yyvsp[-3].num;
		yyvsp[0].ctl->y = yyvsp[-1].num;
		yyval.ctl = ins_ctrl(CT_STATIC, SS_ICON, yyvsp[0].ctl, yyvsp[-9].ctl);
		;}
    break;

  case 92:
#line 899 "parser.y"
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
		;}
    break;

  case 93:
#line 924 "parser.y"
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
		;}
    break;

  case 94:
#line 946 "parser.y"
    { yyval.ctl = new_control(); ;}
    break;

  case 95:
#line 948 "parser.y"
    {
		yyval.ctl = new_control();
		yyval.ctl->width = yyvsp[-2].num;
		yyval.ctl->height = yyvsp[0].num;
		;}
    break;

  case 96:
#line 953 "parser.y"
    {
		yyval.ctl = new_control();
		yyval.ctl->width = yyvsp[-4].num;
		yyval.ctl->height = yyvsp[-2].num;
		yyval.ctl->style = yyvsp[0].style;
		yyval.ctl->gotstyle = TRUE;
		;}
    break;

  case 97:
#line 960 "parser.y"
    {
		yyval.ctl = new_control();
		yyval.ctl->width = yyvsp[-6].num;
		yyval.ctl->height = yyvsp[-4].num;
		yyval.ctl->style = yyvsp[-2].style;
		yyval.ctl->gotstyle = TRUE;
		yyval.ctl->exstyle = yyvsp[0].style;
		yyval.ctl->gotexstyle = TRUE;
		;}
    break;

  case 98:
#line 971 "parser.y"
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
		;}
    break;

  case 99:
#line 985 "parser.y"
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
		;}
    break;

  case 100:
#line 1000 "parser.y"
    { yyval.fntid = new_font_id(yyvsp[-2].num, yyvsp[0].str, 0, 0); ;}
    break;

  case 101:
#line 1005 "parser.y"
    { yyval.styles = NULL; ;}
    break;

  case 102:
#line 1006 "parser.y"
    { yyval.styles = new_style_pair(yyvsp[0].style, 0); ;}
    break;

  case 103:
#line 1007 "parser.y"
    { yyval.styles = new_style_pair(yyvsp[-2].style, yyvsp[0].style); ;}
    break;

  case 104:
#line 1011 "parser.y"
    { yyval.style = new_style(yyvsp[-2].style->or_mask | yyvsp[0].style->or_mask, yyvsp[-2].style->and_mask | yyvsp[0].style->and_mask); free(yyvsp[-2].style); free(yyvsp[0].style);;}
    break;

  case 105:
#line 1012 "parser.y"
    { yyval.style = yyvsp[-1].style; ;}
    break;

  case 106:
#line 1013 "parser.y"
    { yyval.style = new_style(yyvsp[0].num, 0); ;}
    break;

  case 107:
#line 1014 "parser.y"
    { yyval.style = new_style(0, yyvsp[0].num); ;}
    break;

  case 108:
#line 1018 "parser.y"
    {
		yyval.nid = new_name_id();
		yyval.nid->type = name_ord;
		yyval.nid->name.i_name = yyvsp[0].num;
		;}
    break;

  case 109:
#line 1023 "parser.y"
    {
		yyval.nid = new_name_id();
		yyval.nid->type = name_str;
		yyval.nid->name.s_name = yyvsp[0].str;
		;}
    break;

  case 110:
#line 1032 "parser.y"
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
		;}
    break;

  case 111:
#line 1075 "parser.y"
    { yyval.dlgex=new_dialogex(); ;}
    break;

  case 112:
#line 1076 "parser.y"
    { yyval.dlgex=dialogex_style(yyvsp[0].style,yyvsp[-2].dlgex); ;}
    break;

  case 113:
#line 1077 "parser.y"
    { yyval.dlgex=dialogex_exstyle(yyvsp[0].style,yyvsp[-2].dlgex); ;}
    break;

  case 114:
#line 1078 "parser.y"
    { yyval.dlgex=dialogex_caption(yyvsp[0].str,yyvsp[-2].dlgex); ;}
    break;

  case 115:
#line 1079 "parser.y"
    { yyval.dlgex=dialogex_font(yyvsp[0].fntid,yyvsp[-1].dlgex); ;}
    break;

  case 116:
#line 1080 "parser.y"
    { yyval.dlgex=dialogex_font(yyvsp[0].fntid,yyvsp[-1].dlgex); ;}
    break;

  case 117:
#line 1081 "parser.y"
    { yyval.dlgex=dialogex_class(yyvsp[0].nid,yyvsp[-2].dlgex); ;}
    break;

  case 118:
#line 1082 "parser.y"
    { yyval.dlgex=dialogex_menu(yyvsp[0].nid,yyvsp[-2].dlgex); ;}
    break;

  case 119:
#line 1083 "parser.y"
    { yyval.dlgex=dialogex_language(yyvsp[0].lan,yyvsp[-1].dlgex); ;}
    break;

  case 120:
#line 1084 "parser.y"
    { yyval.dlgex=dialogex_characteristics(yyvsp[0].chars,yyvsp[-1].dlgex); ;}
    break;

  case 121:
#line 1085 "parser.y"
    { yyval.dlgex=dialogex_version(yyvsp[0].ver,yyvsp[-1].dlgex); ;}
    break;

  case 122:
#line 1088 "parser.y"
    { yyval.ctl = NULL; ;}
    break;

  case 123:
#line 1089 "parser.y"
    { yyval.ctl=ins_ctrl(-1, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 124:
#line 1090 "parser.y"
    { yyval.ctl=ins_ctrl(CT_EDIT, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 125:
#line 1091 "parser.y"
    { yyval.ctl=ins_ctrl(CT_LISTBOX, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 126:
#line 1092 "parser.y"
    { yyval.ctl=ins_ctrl(CT_COMBOBOX, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 127:
#line 1093 "parser.y"
    { yyval.ctl=ins_ctrl(CT_SCROLLBAR, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 128:
#line 1094 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_CHECKBOX, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 129:
#line 1095 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_DEFPUSHBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 130:
#line 1096 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_GROUPBOX, yyvsp[0].ctl, yyvsp[-2].ctl);;}
    break;

  case 131:
#line 1097 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_PUSHBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 132:
#line 1099 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_RADIOBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 133:
#line 1100 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_AUTO3STATE, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 134:
#line 1101 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_3STATE, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 135:
#line 1102 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_AUTOCHECKBOX, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 136:
#line 1103 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_AUTORADIOBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 137:
#line 1104 "parser.y"
    { yyval.ctl=ins_ctrl(CT_STATIC, SS_LEFT, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 138:
#line 1105 "parser.y"
    { yyval.ctl=ins_ctrl(CT_STATIC, SS_CENTER, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 139:
#line 1106 "parser.y"
    { yyval.ctl=ins_ctrl(CT_STATIC, SS_RIGHT, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 140:
#line 1108 "parser.y"
    {
		yyvsp[0].ctl->title = yyvsp[-7].nid;
		yyvsp[0].ctl->id = yyvsp[-5].num;
		yyvsp[0].ctl->x = yyvsp[-3].num;
		yyvsp[0].ctl->y = yyvsp[-1].num;
		yyval.ctl = ins_ctrl(CT_STATIC, SS_ICON, yyvsp[0].ctl, yyvsp[-9].ctl);
		;}
    break;

  case 141:
#line 1119 "parser.y"
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
		;}
    break;

  case 142:
#line 1143 "parser.y"
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
		;}
    break;

  case 143:
#line 1159 "parser.y"
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
		;}
    break;

  case 144:
#line 1187 "parser.y"
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
		;}
    break;

  case 145:
#line 1210 "parser.y"
    { yyval.raw = NULL; ;}
    break;

  case 146:
#line 1211 "parser.y"
    { yyval.raw = yyvsp[0].raw; ;}
    break;

  case 147:
#line 1214 "parser.y"
    { yyval.iptr = NULL; ;}
    break;

  case 148:
#line 1215 "parser.y"
    { yyval.iptr = new_int(yyvsp[0].num); ;}
    break;

  case 149:
#line 1219 "parser.y"
    { yyval.fntid = new_font_id(yyvsp[-7].num, yyvsp[-5].str, yyvsp[-3].num, yyvsp[-1].num); ;}
    break;

  case 150:
#line 1226 "parser.y"
    { yyval.fntid = NULL; ;}
    break;

  case 151:
#line 1227 "parser.y"
    { yyval.fntid = NULL; ;}
    break;

  case 152:
#line 1231 "parser.y"
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
		;}
    break;

  case 153:
#line 1254 "parser.y"
    { yyval.menitm = yyvsp[-1].menitm; ;}
    break;

  case 154:
#line 1258 "parser.y"
    {yyval.menitm = NULL;;}
    break;

  case 155:
#line 1259 "parser.y"
    {
		yyval.menitm=new_menu_item();
		yyval.menitm->prev = yyvsp[-5].menitm;
		if(yyvsp[-5].menitm)
			yyvsp[-5].menitm->next = yyval.menitm;
		yyval.menitm->id =  yyvsp[-1].num;
		yyval.menitm->state = yyvsp[0].num;
		yyval.menitm->name = yyvsp[-3].str;
		;}
    break;

  case 156:
#line 1268 "parser.y"
    {
		yyval.menitm=new_menu_item();
		yyval.menitm->prev = yyvsp[-2].menitm;
		if(yyvsp[-2].menitm)
			yyvsp[-2].menitm->next = yyval.menitm;
		;}
    break;

  case 157:
#line 1274 "parser.y"
    {
		yyval.menitm = new_menu_item();
		yyval.menitm->prev = yyvsp[-4].menitm;
		if(yyvsp[-4].menitm)
			yyvsp[-4].menitm->next = yyval.menitm;
		yyval.menitm->popup = get_item_head(yyvsp[0].menitm);
		yyval.menitm->name = yyvsp[-2].str;
		;}
    break;

  case 158:
#line 1293 "parser.y"
    { yyval.num = 0; ;}
    break;

  case 159:
#line 1294 "parser.y"
    { yyval.num = yyvsp[0].num | MF_CHECKED; ;}
    break;

  case 160:
#line 1295 "parser.y"
    { yyval.num = yyvsp[0].num | MF_GRAYED; ;}
    break;

  case 161:
#line 1296 "parser.y"
    { yyval.num = yyvsp[0].num | MF_HELP; ;}
    break;

  case 162:
#line 1297 "parser.y"
    { yyval.num = yyvsp[0].num | MF_DISABLED; ;}
    break;

  case 163:
#line 1298 "parser.y"
    { yyval.num = yyvsp[0].num | MF_MENUBARBREAK; ;}
    break;

  case 164:
#line 1299 "parser.y"
    { yyval.num = yyvsp[0].num | MF_MENUBREAK; ;}
    break;

  case 165:
#line 1303 "parser.y"
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
		;}
    break;

  case 166:
#line 1328 "parser.y"
    { yyval.menexitm = yyvsp[-1].menexitm; ;}
    break;

  case 167:
#line 1332 "parser.y"
    {yyval.menexitm = NULL; ;}
    break;

  case 168:
#line 1333 "parser.y"
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
		;}
    break;

  case 169:
#line 1349 "parser.y"
    {
		yyval.menexitm = new_menuex_item();
		yyval.menexitm->prev = yyvsp[-2].menexitm;
		if(yyvsp[-2].menexitm)
			yyvsp[-2].menexitm->next = yyval.menexitm;
		;}
    break;

  case 170:
#line 1355 "parser.y"
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
		;}
    break;

  case 171:
#line 1375 "parser.y"
    { yyval.exopt = new_itemex_opt(0, 0, 0, 0); ;}
    break;

  case 172:
#line 1376 "parser.y"
    {
		yyval.exopt = new_itemex_opt(yyvsp[0].num, 0, 0, 0);
		yyval.exopt->gotid = TRUE;
		;}
    break;

  case 173:
#line 1380 "parser.y"
    {
		yyval.exopt = new_itemex_opt(yyvsp[-3].iptr ? *(yyvsp[-3].iptr) : 0, yyvsp[-1].iptr ? *(yyvsp[-1].iptr) : 0, yyvsp[0].num, 0);
		yyval.exopt->gotid = TRUE;
		yyval.exopt->gottype = TRUE;
		yyval.exopt->gotstate = TRUE;
		if(yyvsp[-3].iptr) free(yyvsp[-3].iptr);
		if(yyvsp[-1].iptr) free(yyvsp[-1].iptr);
		;}
    break;

  case 174:
#line 1388 "parser.y"
    {
		yyval.exopt = new_itemex_opt(yyvsp[-4].iptr ? *(yyvsp[-4].iptr) : 0, yyvsp[-2].iptr ? *(yyvsp[-2].iptr) : 0, yyvsp[0].num, 0);
		yyval.exopt->gotid = TRUE;
		yyval.exopt->gottype = TRUE;
		yyval.exopt->gotstate = TRUE;
		if(yyvsp[-4].iptr) free(yyvsp[-4].iptr);
		if(yyvsp[-2].iptr) free(yyvsp[-2].iptr);
		;}
    break;

  case 175:
#line 1399 "parser.y"
    { yyval.exopt = new_itemex_opt(0, 0, 0, 0); ;}
    break;

  case 176:
#line 1400 "parser.y"
    {
		yyval.exopt = new_itemex_opt(yyvsp[0].num, 0, 0, 0);
		yyval.exopt->gotid = TRUE;
		;}
    break;

  case 177:
#line 1404 "parser.y"
    {
		yyval.exopt = new_itemex_opt(yyvsp[-2].iptr ? *(yyvsp[-2].iptr) : 0, yyvsp[0].num, 0, 0);
		if(yyvsp[-2].iptr) free(yyvsp[-2].iptr);
		yyval.exopt->gotid = TRUE;
		yyval.exopt->gottype = TRUE;
		;}
    break;

  case 178:
#line 1410 "parser.y"
    {
		yyval.exopt = new_itemex_opt(yyvsp[-4].iptr ? *(yyvsp[-4].iptr) : 0, yyvsp[-2].iptr ? *(yyvsp[-2].iptr) : 0, yyvsp[0].num, 0);
		if(yyvsp[-4].iptr) free(yyvsp[-4].iptr);
		if(yyvsp[-2].iptr) free(yyvsp[-2].iptr);
		yyval.exopt->gotid = TRUE;
		yyval.exopt->gottype = TRUE;
		yyval.exopt->gotstate = TRUE;
		;}
    break;

  case 179:
#line 1418 "parser.y"
    {
		yyval.exopt = new_itemex_opt(yyvsp[-6].iptr ? *(yyvsp[-6].iptr) : 0, yyvsp[-4].iptr ? *(yyvsp[-4].iptr) : 0, yyvsp[-2].iptr ? *(yyvsp[-2].iptr) : 0, yyvsp[0].num);
		if(yyvsp[-6].iptr) free(yyvsp[-6].iptr);
		if(yyvsp[-4].iptr) free(yyvsp[-4].iptr);
		if(yyvsp[-2].iptr) free(yyvsp[-2].iptr);
		yyval.exopt->gotid = TRUE;
		yyval.exopt->gottype = TRUE;
		yyval.exopt->gotstate = TRUE;
		yyval.exopt->gothelpid = TRUE;
		;}
    break;

  case 180:
#line 1438 "parser.y"
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
		;}
    break;

  case 181:
#line 1479 "parser.y"
    {
		if((tagstt = find_stringtable(yyvsp[0].lvc)) == NULL)
			tagstt = new_stringtable(yyvsp[0].lvc);
		tagstt_memopt = yyvsp[-1].iptr;
		tagstt_version = yyvsp[0].lvc->version;
		tagstt_characts = yyvsp[0].lvc->characts;
		if(yyvsp[0].lvc)
			free(yyvsp[0].lvc);
		;}
    break;

  case 182:
#line 1490 "parser.y"
    { yyval.stt = NULL; ;}
    break;

  case 183:
#line 1491 "parser.y"
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
		;}
    break;

  case 186:
#line 1531 "parser.y"
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
		;}
    break;

  case 187:
#line 1547 "parser.y"
    { yyval.veri = new_versioninfo(); ;}
    break;

  case 188:
#line 1548 "parser.y"
    {
		if(yyvsp[-8].veri->gotit.fv)
			yyerror("FILEVERSION already defined");
		yyval.veri = yyvsp[-8].veri;
		yyval.veri->filever_maj1 = yyvsp[-6].num;
		yyval.veri->filever_maj2 = yyvsp[-4].num;
		yyval.veri->filever_min1 = yyvsp[-2].num;
		yyval.veri->filever_min2 = yyvsp[0].num;
		yyval.veri->gotit.fv = 1;
		;}
    break;

  case 189:
#line 1558 "parser.y"
    {
		if(yyvsp[-8].veri->gotit.pv)
			yyerror("PRODUCTVERSION already defined");
		yyval.veri = yyvsp[-8].veri;
		yyval.veri->prodver_maj1 = yyvsp[-6].num;
		yyval.veri->prodver_maj2 = yyvsp[-4].num;
		yyval.veri->prodver_min1 = yyvsp[-2].num;
		yyval.veri->prodver_min2 = yyvsp[0].num;
		yyval.veri->gotit.pv = 1;
		;}
    break;

  case 190:
#line 1568 "parser.y"
    {
		if(yyvsp[-2].veri->gotit.ff)
			yyerror("FILEFLAGS already defined");
		yyval.veri = yyvsp[-2].veri;
		yyval.veri->fileflags = yyvsp[0].num;
		yyval.veri->gotit.ff = 1;
		;}
    break;

  case 191:
#line 1575 "parser.y"
    {
		if(yyvsp[-2].veri->gotit.ffm)
			yyerror("FILEFLAGSMASK already defined");
		yyval.veri = yyvsp[-2].veri;
		yyval.veri->fileflagsmask = yyvsp[0].num;
		yyval.veri->gotit.ffm = 1;
		;}
    break;

  case 192:
#line 1582 "parser.y"
    {
		if(yyvsp[-2].veri->gotit.fo)
			yyerror("FILEOS already defined");
		yyval.veri = yyvsp[-2].veri;
		yyval.veri->fileos = yyvsp[0].num;
		yyval.veri->gotit.fo = 1;
		;}
    break;

  case 193:
#line 1589 "parser.y"
    {
		if(yyvsp[-2].veri->gotit.ft)
			yyerror("FILETYPE already defined");
		yyval.veri = yyvsp[-2].veri;
		yyval.veri->filetype = yyvsp[0].num;
		yyval.veri->gotit.ft = 1;
		;}
    break;

  case 194:
#line 1596 "parser.y"
    {
		if(yyvsp[-2].veri->gotit.fst)
			yyerror("FILESUBTYPE already defined");
		yyval.veri = yyvsp[-2].veri;
		yyval.veri->filesubtype = yyvsp[0].num;
		yyval.veri->gotit.fst = 1;
		;}
    break;

  case 195:
#line 1606 "parser.y"
    { yyval.blk = NULL; ;}
    break;

  case 196:
#line 1607 "parser.y"
    {
		yyval.blk = yyvsp[0].blk;
		yyval.blk->prev = yyvsp[-1].blk;
		if(yyvsp[-1].blk)
			yyvsp[-1].blk->next = yyval.blk;
		;}
    break;

  case 197:
#line 1616 "parser.y"
    {
		yyval.blk = new_ver_block();
		yyval.blk->name = yyvsp[-3].str;
		yyval.blk->values = get_ver_value_head(yyvsp[-1].val);
		;}
    break;

  case 198:
#line 1624 "parser.y"
    { yyval.val = NULL; ;}
    break;

  case 199:
#line 1625 "parser.y"
    {
		yyval.val = yyvsp[0].val;
		yyval.val->prev = yyvsp[-1].val;
		if(yyvsp[-1].val)
			yyvsp[-1].val->next = yyval.val;
		;}
    break;

  case 200:
#line 1634 "parser.y"
    {
		yyval.val = new_ver_value();
		yyval.val->type = val_block;
		yyval.val->value.block = yyvsp[0].blk;
		;}
    break;

  case 201:
#line 1639 "parser.y"
    {
		yyval.val = new_ver_value();
		yyval.val->type = val_str;
		yyval.val->key = yyvsp[-2].str;
		yyval.val->value.str = yyvsp[0].str;
		;}
    break;

  case 202:
#line 1645 "parser.y"
    {
		yyval.val = new_ver_value();
		yyval.val->type = val_words;
		yyval.val->key = yyvsp[-2].str;
		yyval.val->value.words = yyvsp[0].verw;
		;}
    break;

  case 203:
#line 1654 "parser.y"
    { yyval.verw = new_ver_words(yyvsp[0].num); ;}
    break;

  case 204:
#line 1655 "parser.y"
    { yyval.verw = add_ver_words(yyvsp[-2].verw, yyvsp[0].num); ;}
    break;

  case 205:
#line 1659 "parser.y"
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
		;}
    break;

  case 206:
#line 1685 "parser.y"
    { yyval.tlbarItems = NULL; ;}
    break;

  case 207:
#line 1686 "parser.y"
    {
		toolbar_item_t *idrec = new_toolbar_item();
		idrec->id = yyvsp[0].num;
		yyval.tlbarItems = ins_tlbr_button(yyvsp[-2].tlbarItems, idrec);
		;}
    break;

  case 208:
#line 1691 "parser.y"
    {
		toolbar_item_t *idrec = new_toolbar_item();
		idrec->id = 0;
		yyval.tlbarItems = ins_tlbr_button(yyvsp[-1].tlbarItems, idrec);
	;}
    break;

  case 209:
#line 1700 "parser.y"
    { yyval.iptr = NULL; ;}
    break;

  case 210:
#line 1701 "parser.y"
    {
		if(yyvsp[-1].iptr)
		{
			*(yyvsp[-1].iptr) |= *(yyvsp[0].iptr);
			yyval.iptr = yyvsp[-1].iptr;
			free(yyvsp[0].iptr);
		}
		else
			yyval.iptr = yyvsp[0].iptr;
		;}
    break;

  case 211:
#line 1711 "parser.y"
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
		;}
    break;

  case 212:
#line 1726 "parser.y"
    { yyval.iptr = new_int(WRC_MO_PRELOAD); ;}
    break;

  case 213:
#line 1727 "parser.y"
    { yyval.iptr = new_int(WRC_MO_MOVEABLE); ;}
    break;

  case 214:
#line 1728 "parser.y"
    { yyval.iptr = new_int(WRC_MO_DISCARDABLE); ;}
    break;

  case 215:
#line 1729 "parser.y"
    { yyval.iptr = new_int(WRC_MO_PURE); ;}
    break;

  case 216:
#line 1732 "parser.y"
    { yyval.iptr = new_int(~WRC_MO_PRELOAD); ;}
    break;

  case 217:
#line 1733 "parser.y"
    { yyval.iptr = new_int(~WRC_MO_MOVEABLE); ;}
    break;

  case 218:
#line 1734 "parser.y"
    { yyval.iptr = new_int(~WRC_MO_PURE); ;}
    break;

  case 219:
#line 1738 "parser.y"
    { yyval.lvc = new_lvc(); ;}
    break;

  case 220:
#line 1739 "parser.y"
    {
		if(!win32)
			yywarning("LANGUAGE not supported in 16-bit mode");
		if(yyvsp[-1].lvc->language)
			yyerror("Language already defined");
		yyval.lvc = yyvsp[-1].lvc;
		yyvsp[-1].lvc->language = yyvsp[0].lan;
		;}
    break;

  case 221:
#line 1747 "parser.y"
    {
		if(!win32)
			yywarning("CHARACTERISTICS not supported in 16-bit mode");
		if(yyvsp[-1].lvc->characts)
			yyerror("Characteristics already defined");
		yyval.lvc = yyvsp[-1].lvc;
		yyvsp[-1].lvc->characts = yyvsp[0].chars;
		;}
    break;

  case 222:
#line 1755 "parser.y"
    {
		if(!win32)
			yywarning("VERSION not supported in 16-bit mode");
		if(yyvsp[-1].lvc->version)
			yyerror("Version already defined");
		yyval.lvc = yyvsp[-1].lvc;
		yyvsp[-1].lvc->version = yyvsp[0].ver;
		;}
    break;

  case 223:
#line 1773 "parser.y"
    { yyval.lan = new_language(yyvsp[-2].num, yyvsp[0].num);
					  if (get_language_codepage(yyvsp[-2].num, yyvsp[0].num) == -1)
						yyerror( "Language %04x is not supported", (yyvsp[0].num<<10) + yyvsp[-2].num);
					;}
    break;

  case 224:
#line 1780 "parser.y"
    { yyval.chars = new_characts(yyvsp[0].num); ;}
    break;

  case 225:
#line 1784 "parser.y"
    { yyval.ver = new_version(yyvsp[0].num); ;}
    break;

  case 226:
#line 1788 "parser.y"
    {
		if(yyvsp[-3].lvc)
		{
			yyvsp[-1].raw->lvc = *(yyvsp[-3].lvc);
			free(yyvsp[-3].lvc);
		}

		if(!yyvsp[-1].raw->lvc.language)
			yyvsp[-1].raw->lvc.language = dup_language(currentlanguage);

		yyval.raw = yyvsp[-1].raw;
		;}
    break;

  case 227:
#line 1803 "parser.y"
    { yyval.raw = yyvsp[0].raw; ;}
    break;

  case 228:
#line 1804 "parser.y"
    { yyval.raw = int2raw_data(yyvsp[0].num); ;}
    break;

  case 229:
#line 1805 "parser.y"
    { yyval.raw = int2raw_data(-(yyvsp[0].num)); ;}
    break;

  case 230:
#line 1806 "parser.y"
    { yyval.raw = long2raw_data(yyvsp[0].num); ;}
    break;

  case 231:
#line 1807 "parser.y"
    { yyval.raw = long2raw_data(-(yyvsp[0].num)); ;}
    break;

  case 232:
#line 1808 "parser.y"
    { yyval.raw = str2raw_data(yyvsp[0].str); ;}
    break;

  case 233:
#line 1809 "parser.y"
    { yyval.raw = merge_raw_data(yyvsp[-2].raw, yyvsp[0].raw); free(yyvsp[0].raw->data); free(yyvsp[0].raw); ;}
    break;

  case 234:
#line 1810 "parser.y"
    { yyval.raw = merge_raw_data_int(yyvsp[-2].raw, yyvsp[0].num); ;}
    break;

  case 235:
#line 1811 "parser.y"
    { yyval.raw = merge_raw_data_int(yyvsp[-3].raw, -(yyvsp[0].num)); ;}
    break;

  case 236:
#line 1812 "parser.y"
    { yyval.raw = merge_raw_data_long(yyvsp[-2].raw, yyvsp[0].num); ;}
    break;

  case 237:
#line 1813 "parser.y"
    { yyval.raw = merge_raw_data_long(yyvsp[-3].raw, -(yyvsp[0].num)); ;}
    break;

  case 238:
#line 1814 "parser.y"
    { yyval.raw = merge_raw_data_str(yyvsp[-2].raw, yyvsp[0].str); ;}
    break;

  case 239:
#line 1818 "parser.y"
    { yyval.raw = load_file(yyvsp[0].str,dup_language(currentlanguage)); ;}
    break;

  case 240:
#line 1819 "parser.y"
    { yyval.raw = yyvsp[0].raw; ;}
    break;

  case 241:
#line 1826 "parser.y"
    { yyval.iptr = 0; ;}
    break;

  case 242:
#line 1827 "parser.y"
    { yyval.iptr = new_int(yyvsp[0].num); ;}
    break;

  case 243:
#line 1831 "parser.y"
    { yyval.num = (yyvsp[0].num); ;}
    break;

  case 244:
#line 1834 "parser.y"
    { yyval.num = (yyvsp[-2].num) + (yyvsp[0].num); ;}
    break;

  case 245:
#line 1835 "parser.y"
    { yyval.num = (yyvsp[-2].num) - (yyvsp[0].num); ;}
    break;

  case 246:
#line 1836 "parser.y"
    { yyval.num = (yyvsp[-2].num) | (yyvsp[0].num); ;}
    break;

  case 247:
#line 1837 "parser.y"
    { yyval.num = (yyvsp[-2].num) & (yyvsp[0].num); ;}
    break;

  case 248:
#line 1838 "parser.y"
    { yyval.num = (yyvsp[-2].num) * (yyvsp[0].num); ;}
    break;

  case 249:
#line 1839 "parser.y"
    { yyval.num = (yyvsp[-2].num) / (yyvsp[0].num); ;}
    break;

  case 250:
#line 1840 "parser.y"
    { yyval.num = (yyvsp[-2].num) ^ (yyvsp[0].num); ;}
    break;

  case 251:
#line 1841 "parser.y"
    { yyval.num = ~(yyvsp[0].num); ;}
    break;

  case 252:
#line 1842 "parser.y"
    { yyval.num = -(yyvsp[0].num); ;}
    break;

  case 253:
#line 1843 "parser.y"
    { yyval.num = yyvsp[0].num; ;}
    break;

  case 254:
#line 1844 "parser.y"
    { yyval.num = yyvsp[-1].num; ;}
    break;

  case 255:
#line 1845 "parser.y"
    { yyval.num = yyvsp[0].num; ;}
    break;

  case 256:
#line 1849 "parser.y"
    { yyval.num = (yyvsp[0].num); ;}
    break;

  case 257:
#line 1850 "parser.y"
    { yyval.num = ~(yyvsp[0].num); ;}
    break;

  case 258:
#line 1853 "parser.y"
    { yyval.num = yyvsp[0].num; ;}
    break;

  case 259:
#line 1854 "parser.y"
    { yyval.num = yyvsp[0].num; ;}
    break;


    }

/* Line 1000 of yacc.c.  */
#line 4171 "parser.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  const char* yyprefix;
	  char *yymsg;
	  int yyx;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 0;

	  yyprefix = ", expecting ";
	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
		yycount += 1;
		if (yycount == 5)
		  {
		    yysize = 0;
		    break;
		  }
	      }
	  yysize += (sizeof ("syntax error, unexpected ")
		     + yystrlen (yytname[yytype]));
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yyprefix = ", expecting ";
		  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			yyp = yystpcpy (yyp, yyprefix);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yyprefix = " or ";
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (yychar == YYEOF)
	     for (;;)
	       {
		 YYPOPSTACK;
		 if (yyssp == yyss)
		   YYABORT;
		 YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
		 yydestruct (yystos[*yyssp], yyvsp);
	       }
        }
      else
	{
	  YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
	  yydestruct (yytoken, &yylval);
	  yychar = YYEMPTY;

	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes YYERROR and the label
     yyerrorlab therefore never appears in user code.  */
  if (0)
     goto yyerrorlab;
#endif

  yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


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

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 1857 "parser.y"

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
	int keycode = 0, keysym = 0;
	event_t *ev = new_event();

	if(key->type == str_char)
		keysym = key->str.cstr[0];
	else
		keysym = key->str.wstr[0];

	if((flags & WRC_AF_VIRTKEY) && (!isupper(keysym & 0xff) && !isdigit(keysym & 0xff)))
		yyerror("VIRTKEY code is not equal to ascii value");

	if(keysym == '^' && (flags & WRC_AF_CONTROL) != 0)
	{
		yyerror("Cannot use both '^' and CONTROL modifier");
	}
	else if(keysym == '^')
	{
		if(key->type == str_char)
			keycode = toupper(key->str.cstr[1]) - '@';
		else
			keycode = toupper(key->str.wstr[1]) - '@';
		if(keycode >= ' ')
			yyerror("Control-code out of range");
	}
	else
		keycode = keysym;
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
	if (!(path = wpp_find_include(name->str.cstr, input_name)))
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
#define STE(p)	((const stt_entry_t *)(p))
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
	case WRC_RT_HTML:
		type = "HTML";
		token = tHTML;
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
		yywarning("Usertype uses reserved type ID %d, which is not supported by wrc yet", yylval.num);
	default:
		return lookahead;
	}

	return token;
}

