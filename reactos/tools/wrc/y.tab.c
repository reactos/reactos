/* A Bison parser, made by GNU Bison 1.875b.  */

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
     tAUTO3STATE = 279,
     tAUTOCHECKBOX = 280,
     tAUTORADIOBUTTON = 281,
     tCHECKBOX = 282,
     tDEFPUSHBUTTON = 283,
     tPUSHBUTTON = 284,
     tRADIOBUTTON = 285,
     tSTATE3 = 286,
     tGROUPBOX = 287,
     tCOMBOBOX = 288,
     tLISTBOX = 289,
     tSCROLLBAR = 290,
     tCONTROL = 291,
     tEDITTEXT = 292,
     tRTEXT = 293,
     tCTEXT = 294,
     tLTEXT = 295,
     tBLOCK = 296,
     tVALUE = 297,
     tSHIFT = 298,
     tALT = 299,
     tASCII = 300,
     tVIRTKEY = 301,
     tGRAYED = 302,
     tCHECKED = 303,
     tINACTIVE = 304,
     tNOINVERT = 305,
     tPURE = 306,
     tIMPURE = 307,
     tDISCARDABLE = 308,
     tLOADONCALL = 309,
     tPRELOAD = 310,
     tFIXED = 311,
     tMOVEABLE = 312,
     tCLASS = 313,
     tCAPTION = 314,
     tCHARACTERISTICS = 315,
     tEXSTYLE = 316,
     tSTYLE = 317,
     tVERSION = 318,
     tLANGUAGE = 319,
     tFILEVERSION = 320,
     tPRODUCTVERSION = 321,
     tFILEFLAGSMASK = 322,
     tFILEOS = 323,
     tFILETYPE = 324,
     tFILEFLAGS = 325,
     tFILESUBTYPE = 326,
     tMENUBARBREAK = 327,
     tMENUBREAK = 328,
     tMENUITEM = 329,
     tPOPUP = 330,
     tSEPARATOR = 331,
     tHELP = 332,
     tTOOLBAR = 333,
     tBUTTON = 334,
     tBEGIN = 335,
     tEND = 336,
     tDLGINIT = 337,
     tNOT = 338,
     pUPM = 339
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
#define tAUTO3STATE 279
#define tAUTOCHECKBOX 280
#define tAUTORADIOBUTTON 281
#define tCHECKBOX 282
#define tDEFPUSHBUTTON 283
#define tPUSHBUTTON 284
#define tRADIOBUTTON 285
#define tSTATE3 286
#define tGROUPBOX 287
#define tCOMBOBOX 288
#define tLISTBOX 289
#define tSCROLLBAR 290
#define tCONTROL 291
#define tEDITTEXT 292
#define tRTEXT 293
#define tCTEXT 294
#define tLTEXT 295
#define tBLOCK 296
#define tVALUE 297
#define tSHIFT 298
#define tALT 299
#define tASCII 300
#define tVIRTKEY 301
#define tGRAYED 302
#define tCHECKED 303
#define tINACTIVE 304
#define tNOINVERT 305
#define tPURE 306
#define tIMPURE 307
#define tDISCARDABLE 308
#define tLOADONCALL 309
#define tPRELOAD 310
#define tFIXED 311
#define tMOVEABLE 312
#define tCLASS 313
#define tCAPTION 314
#define tCHARACTERISTICS 315
#define tEXSTYLE 316
#define tSTYLE 317
#define tVERSION 318
#define tLANGUAGE 319
#define tFILEVERSION 320
#define tPRODUCTVERSION 321
#define tFILEFLAGSMASK 322
#define tFILEOS 323
#define tFILETYPE 324
#define tFILEFLAGS 325
#define tFILESUBTYPE 326
#define tMENUBARBREAK 327
#define tMENUBREAK 328
#define tMENUITEM 329
#define tPOPUP 330
#define tSEPARATOR 331
#define tHELP 332
#define tTOOLBAR 333
#define tBUTTON 334
#define tBEGIN 335
#define tEND 336
#define tDLGINIT 337
#define tNOT 338
#define pUPM 339




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
#line 240 "parser.y"
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
#line 527 "y.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 539 "y.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

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
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

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
#define YYLAST   674

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  96
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  83
/* YYNRULES -- Number of rules. */
#define YYNRULES  258
/* YYNRULES -- Number of states. */
#define YYNSTATES  570

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   339

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
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
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    91,    92
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     5,     6,     9,    12,    16,    20,    22,
      23,    29,    30,    32,    34,    36,    38,    40,    42,    44,
      46,    48,    50,    52,    54,    56,    58,    60,    62,    64,
      66,    68,    70,    72,    74,    76,    80,    84,    88,    92,
      96,   100,   104,   108,   112,   114,   116,   123,   124,   130,
     136,   137,   140,   142,   146,   148,   150,   152,   154,   156,
     158,   172,   173,   177,   181,   185,   188,   192,   196,   199,
     202,   205,   206,   210,   214,   218,   222,   226,   230,   234,
     238,   242,   246,   250,   254,   258,   262,   266,   270,   274,
     285,   298,   309,   310,   315,   322,   331,   349,   365,   370,
     371,   374,   375,   378,   383,   387,   391,   393,   396,   398,
     400,   415,   416,   420,   424,   428,   431,   434,   438,   442,
     445,   448,   451,   452,   456,   460,   464,   468,   472,   476,
     480,   484,   488,   492,   496,   500,   504,   508,   512,   516,
     520,   531,   551,   568,   583,   596,   597,   599,   600,   603,
     613,   614,   617,   622,   626,   627,   634,   638,   644,   645,
     649,   653,   657,   661,   665,   669,   674,   678,   679,   684,
     688,   694,   695,   698,   704,   711,   712,   715,   720,   727,
     736,   741,   745,   746,   751,   752,   754,   761,   762,   772,
     782,   786,   790,   794,   798,   802,   803,   806,   812,   813,
     816,   818,   823,   828,   830,   834,   844,   845,   849,   852,
     853,   856,   859,   861,   863,   865,   867,   869,   871,   873,
     874,   877,   880,   883,   888,   891,   894,   899,   901,   903,
     906,   908,   911,   913,   917,   921,   926,   930,   935,   939,
     941,   943,   944,   946,   948,   952,   956,   960,   964,   968,
     972,   976,   979,   982,   985,   989,   991,   994,   996
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
      97,     0,    -1,    98,    -1,    -1,    98,    99,    -1,    98,
       3,    -1,   176,   101,   104,    -1,     7,   101,   104,    -1,
     152,    -1,    -1,    64,   100,   176,    93,   176,    -1,    -1,
     176,    -1,     7,    -1,   102,    -1,     6,    -1,   116,    -1,
     106,    -1,   107,    -1,   121,    -1,   133,    -1,   113,    -1,
     109,    -1,   110,    -1,   108,    -1,   143,    -1,   147,    -1,
     111,    -1,   112,    -1,   163,    -1,   114,    -1,   156,    -1,
       8,    -1,     7,    -1,     6,    -1,    11,   165,   174,    -1,
      12,   165,   174,    -1,    23,   165,   174,    -1,    21,   165,
     174,    -1,    22,   165,   174,    -1,    17,   165,   174,    -1,
      18,   165,   174,    -1,    82,   165,   174,    -1,   115,   165,
     174,    -1,     4,    -1,     7,    -1,    10,   165,   168,    80,
     117,    81,    -1,    -1,   117,     6,    93,   176,   118,    -1,
     117,   176,    93,   176,   118,    -1,    -1,    93,   119,    -1,
     120,    -1,   119,    93,   120,    -1,    50,    -1,    43,    -1,
      36,    -1,    44,    -1,    45,    -1,    46,    -1,    13,   165,
     176,    93,   176,    93,   176,    93,   176,   122,    80,   123,
      81,    -1,    -1,   122,    62,   131,    -1,   122,    61,   131,
      -1,   122,    59,     6,    -1,   122,   128,    -1,   122,    58,
     103,    -1,   122,    15,   102,    -1,   122,   169,    -1,   122,
     170,    -1,   122,   171,    -1,    -1,   123,    36,   127,    -1,
     123,    37,   125,    -1,   123,    34,   125,    -1,   123,    33,
     125,    -1,   123,    35,   125,    -1,   123,    27,   124,    -1,
     123,    28,   124,    -1,   123,    32,   124,    -1,   123,    29,
     124,    -1,   123,    30,   124,    -1,   123,    24,   124,    -1,
     123,    31,   124,    -1,   123,    25,   124,    -1,   123,    26,
     124,    -1,   123,    40,   124,    -1,   123,    39,   124,    -1,
     123,    38,   124,    -1,   123,    23,   103,   155,   176,    93,
     176,    93,   176,   126,    -1,     6,   155,   176,    93,   176,
      93,   176,    93,   176,    93,   176,   129,    -1,   176,    93,
     176,    93,   176,    93,   176,    93,   176,   129,    -1,    -1,
      93,   176,    93,   176,    -1,    93,   176,    93,   176,    93,
     131,    -1,    93,   176,    93,   176,    93,   131,    93,   131,
      -1,   103,   155,   176,    93,   132,    93,   131,    93,   176,
      93,   176,    93,   176,    93,   176,    93,   131,    -1,   103,
     155,   176,    93,   132,    93,   131,    93,   176,    93,   176,
      93,   176,    93,   176,    -1,    21,   176,    93,     6,    -1,
      -1,    93,   131,    -1,    -1,    93,   131,    -1,    93,   131,
      93,   131,    -1,   131,    83,   131,    -1,    94,   131,    95,
      -1,   178,    -1,    91,   178,    -1,   176,    -1,     6,    -1,
      14,   165,   176,    93,   176,    93,   176,    93,   176,   140,
     134,    80,   135,    81,    -1,    -1,   134,    62,   131,    -1,
     134,    61,   131,    -1,   134,    59,     6,    -1,   134,   128,
      -1,   134,   141,    -1,   134,    58,   103,    -1,   134,    15,
     102,    -1,   134,   169,    -1,   134,   170,    -1,   134,   171,
      -1,    -1,   135,    36,   136,    -1,   135,    37,   138,    -1,
     135,    34,   138,    -1,   135,    33,   138,    -1,   135,    35,
     138,    -1,   135,    27,   137,    -1,   135,    28,   137,    -1,
     135,    32,   137,    -1,   135,    29,   137,    -1,   135,    30,
     137,    -1,   135,    24,   137,    -1,   135,    31,   137,    -1,
     135,    25,   137,    -1,   135,    26,   137,    -1,   135,    40,
     137,    -1,   135,    39,   137,    -1,   135,    38,   137,    -1,
     135,    23,   103,   155,   176,    93,   176,    93,   176,   126,
      -1,   103,   155,   176,    93,   132,    93,   131,    93,   176,
      93,   176,    93,   176,    93,   176,    93,   131,   140,   139,
      -1,   103,   155,   176,    93,   132,    93,   131,    93,   176,
      93,   176,    93,   176,    93,   176,   139,    -1,     6,   155,
     176,    93,   176,    93,   176,    93,   176,    93,   176,   130,
     140,   139,    -1,   176,    93,   176,    93,   176,    93,   176,
      93,   176,   130,   140,   139,    -1,    -1,   172,    -1,    -1,
      93,   176,    -1,    21,   176,    93,     6,    93,   176,    93,
     176,   142,    -1,    -1,    93,   176,    -1,    15,   165,   168,
     144,    -1,    80,   145,    81,    -1,    -1,   145,    74,     6,
     155,   176,   146,    -1,   145,    74,    76,    -1,   145,    75,
       6,   146,   144,    -1,    -1,    93,    48,   146,    -1,    93,
      47,   146,    -1,    93,    77,   146,    -1,    93,    49,   146,
      -1,    93,    72,   146,    -1,    93,    73,   146,    -1,    16,
     165,   168,   148,    -1,    80,   149,    81,    -1,    -1,   149,
      74,     6,   150,    -1,   149,    74,    76,    -1,   149,    75,
       6,   151,   148,    -1,    -1,    93,   176,    -1,    93,   175,
      93,   175,   146,    -1,    93,   175,    93,   175,    93,   176,
      -1,    -1,    93,   176,    -1,    93,   175,    93,   176,    -1,
      93,   175,    93,   175,    93,   176,    -1,    93,   175,    93,
     175,    93,   175,    93,   176,    -1,   153,    80,   154,    81,
      -1,    20,   165,   168,    -1,    -1,   154,   176,   155,     6,
      -1,    -1,    93,    -1,    19,   165,   157,    80,   158,    81,
      -1,    -1,   157,    65,   176,    93,   176,    93,   176,    93,
     176,    -1,   157,    66,   176,    93,   176,    93,   176,    93,
     176,    -1,   157,    70,   176,    -1,   157,    67,   176,    -1,
     157,    68,   176,    -1,   157,    69,   176,    -1,   157,    71,
     176,    -1,    -1,   158,   159,    -1,    41,     6,    80,   160,
      81,    -1,    -1,   160,   161,    -1,   159,    -1,    42,     6,
      93,     6,    -1,    42,     6,    93,   162,    -1,   176,    -1,
     162,    93,   176,    -1,    78,   165,   176,    93,   176,   168,
      80,   164,    81,    -1,    -1,   164,    79,   176,    -1,   164,
      76,    -1,    -1,   165,   166,    -1,   165,   167,    -1,    55,
      -1,    57,    -1,    53,    -1,    51,    -1,    54,    -1,    56,
      -1,    52,    -1,    -1,   168,   169,    -1,   168,   170,    -1,
     168,   171,    -1,    64,   176,    93,   176,    -1,    60,   176,
      -1,    63,   176,    -1,   168,    80,   173,    81,    -1,     9,
      -1,     4,    -1,    87,     4,    -1,     5,    -1,    87,     5,
      -1,     6,    -1,   173,   155,     9,    -1,   173,   155,     4,
      -1,   173,   155,    87,     4,    -1,   173,   155,     5,    -1,
     173,   155,    87,     5,    -1,   173,   155,     6,    -1,   105,
      -1,   172,    -1,    -1,   176,    -1,   177,    -1,   177,    86,
     177,    -1,   177,    87,   177,    -1,   177,    83,   177,    -1,
     177,    85,   177,    -1,   177,    88,   177,    -1,   177,    89,
     177,    -1,   177,    84,   177,    -1,    90,   177,    -1,    87,
     177,    -1,    86,   177,    -1,    94,   177,    95,    -1,   178,
      -1,    91,   178,    -1,     4,    -1,     5,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   362,   362,   396,   397,   467,   473,   485,   495,   503,
     503,   547,   553,   560,   570,   571,   580,   581,   582,   606,
     607,   613,   614,   615,   616,   640,   641,   647,   648,   649,
     650,   651,   655,   656,   657,   661,   665,   681,   703,   713,
     721,   729,   733,   737,   748,   753,   762,   786,   787,   788,
     797,   798,   801,   802,   805,   806,   807,   808,   809,   810,
     815,   850,   851,   852,   853,   854,   855,   856,   857,   858,
     859,   862,   863,   864,   865,   866,   867,   868,   869,   870,
     871,   873,   874,   875,   876,   877,   878,   879,   880,   882,
     892,   911,   927,   929,   934,   941,   952,   966,   981,   986,
     987,   991,   992,   993,   997,   998,   999,  1000,  1004,  1009,
    1017,  1061,  1062,  1063,  1064,  1065,  1066,  1067,  1068,  1069,
    1070,  1071,  1074,  1075,  1076,  1077,  1078,  1079,  1080,  1081,
    1082,  1083,  1085,  1086,  1087,  1088,  1089,  1090,  1091,  1092,
    1094,  1104,  1129,  1145,  1173,  1196,  1197,  1200,  1201,  1205,
    1212,  1213,  1217,  1240,  1244,  1245,  1254,  1260,  1279,  1280,
    1281,  1282,  1283,  1284,  1285,  1289,  1314,  1318,  1319,  1335,
    1341,  1361,  1362,  1366,  1374,  1385,  1386,  1390,  1396,  1404,
    1424,  1465,  1476,  1477,  1510,  1512,  1517,  1533,  1534,  1544,
    1554,  1561,  1568,  1575,  1582,  1592,  1593,  1602,  1610,  1611,
    1620,  1625,  1631,  1640,  1641,  1645,  1671,  1672,  1677,  1686,
    1687,  1697,  1712,  1713,  1714,  1715,  1718,  1719,  1720,  1724,
    1725,  1733,  1741,  1759,  1766,  1770,  1774,  1789,  1790,  1791,
    1792,  1793,  1794,  1795,  1796,  1797,  1798,  1799,  1800,  1804,
    1805,  1812,  1813,  1817,  1820,  1821,  1822,  1823,  1824,  1825,
    1826,  1827,  1828,  1829,  1830,  1831,  1832,  1835,  1836
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "tNL", "tNUMBER", "tLNUMBER", "tSTRING", 
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
  "'('", "')'", "$accept", "resource_file", "resources", "resource", "@1", 
  "usrcvt", "nameid", "nameid_s", "resource_definition", "filename", 
  "bitmap", "cursor", "icon", "font", "fontdir", "messagetable", "rcdata", 
  "dlginit", "userres", "usertype", "accelerators", "events", "acc_opt", 
  "accs", "acc", "dialog", "dlg_attributes", "ctrls", "lab_ctrl", 
  "ctrl_desc", "iconinfo", "gen_ctrl", "opt_font", "optional_style", 
  "optional_style_pair", "style", "ctlclass", "dialogex", "dlgex_attribs", 
  "exctrls", "gen_exctrl", "lab_exctrl", "exctrl_desc", "opt_data", 
  "helpid", "opt_exfont", "opt_expr", "menu", "menu_body", 
  "item_definitions", "item_options", "menuex", "menuex_body", 
  "itemex_definitions", "itemex_options", "itemex_p_options", 
  "stringtable", "stt_head", "strings", "opt_comma", "versioninfo", 
  "fix_version", "ver_blocks", "ver_block", "ver_values", "ver_value", 
  "ver_words", "toolbar", "toolbar_items", "loadmemopts", "lamo", "lama", 
  "opt_lvc", "opt_language", "opt_characts", "opt_version", "raw_data", 
  "raw_elements", "file_raw", "e_expr", "expr", "xpr", "any_num", 0
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
     335,   336,   337,   124,    94,    38,    43,    45,    42,    47,
     126,   338,   339,    44,    40,    41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    96,    97,    98,    98,    98,    99,    99,    99,   100,
      99,   101,   102,   102,   103,   103,   104,   104,   104,   104,
     104,   104,   104,   104,   104,   104,   104,   104,   104,   104,
     104,   104,   105,   105,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   115,   116,   117,   117,   117,
     118,   118,   119,   119,   120,   120,   120,   120,   120,   120,
     121,   122,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   123,   123,   123,   123,   123,   123,   123,   123,   123,
     123,   123,   123,   123,   123,   123,   123,   123,   123,   123,
     124,   125,   126,   126,   126,   126,   127,   127,   128,   129,
     129,   130,   130,   130,   131,   131,   131,   131,   132,   132,
     133,   134,   134,   134,   134,   134,   134,   134,   134,   134,
     134,   134,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   136,   136,   137,   138,   139,   139,   140,   140,   141,
     142,   142,   143,   144,   145,   145,   145,   145,   146,   146,
     146,   146,   146,   146,   146,   147,   148,   149,   149,   149,
     149,   150,   150,   150,   150,   151,   151,   151,   151,   151,
     152,   153,   154,   154,   155,   155,   156,   157,   157,   157,
     157,   157,   157,   157,   157,   158,   158,   159,   160,   160,
     161,   161,   161,   162,   162,   163,   164,   164,   164,   165,
     165,   165,   166,   166,   166,   166,   167,   167,   167,   168,
     168,   168,   168,   169,   170,   171,   172,   173,   173,   173,
     173,   173,   173,   173,   173,   173,   173,   173,   173,   174,
     174,   175,   175,   176,   177,   177,   177,   177,   177,   177,
     177,   177,   177,   177,   177,   177,   177,   178,   178
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     0,     2,     2,     3,     3,     1,     0,
       5,     0,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     1,     1,     6,     0,     5,     5,
       0,     2,     1,     3,     1,     1,     1,     1,     1,     1,
      13,     0,     3,     3,     3,     2,     3,     3,     2,     2,
       2,     0,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,    10,
      12,    10,     0,     4,     6,     8,    17,    15,     4,     0,
       2,     0,     2,     4,     3,     3,     1,     2,     1,     1,
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
       3,     2,     2,     2,     3,     1,     2,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short yydefact[] =
{
       3,     0,     2,     1,     5,   257,   258,    11,   209,     9,
       0,     0,     0,     0,     0,     4,     8,     0,    11,   243,
     255,     0,   219,     0,   253,   252,   251,   256,     0,   182,
       0,     0,     0,     0,     0,     0,     0,     0,    44,    45,
     209,   209,   209,   209,   209,   209,   209,   209,   209,   209,
     209,   209,   209,   209,   209,     7,    17,    18,    24,    22,
      23,    27,    28,    21,    30,   209,    16,    19,    20,    25,
      26,    31,    29,   215,   218,   214,   216,   212,   217,   213,
     210,   211,   181,     0,   254,     0,     6,   246,   250,   247,
     244,   245,   248,   249,   219,   219,   219,     0,     0,   219,
     219,   219,   219,   187,   219,   219,   219,     0,   219,   219,
       0,     0,     0,   220,   221,   222,     0,   180,   184,     0,
      34,    33,    32,   239,     0,   240,    35,    36,     0,     0,
       0,     0,    40,    41,     0,    38,    39,    37,     0,    42,
      43,   224,   225,     0,    10,   185,     0,    47,     0,     0,
       0,   154,   152,   167,   165,     0,     0,     0,     0,     0,
       0,     0,   195,     0,     0,   183,     0,   228,   230,   232,
     227,     0,   184,     0,     0,     0,     0,     0,     0,   191,
     192,   193,   190,   194,     0,   219,   223,     0,    46,     0,
     229,   231,   226,     0,     0,     0,     0,     0,   153,     0,
       0,   166,     0,     0,     0,   186,   196,     0,     0,     0,
     234,   236,   238,   233,     0,     0,     0,   184,   156,   158,
     171,   169,   175,     0,     0,     0,   206,    50,    50,   235,
     237,     0,     0,     0,     0,     0,   241,   168,   241,     0,
       0,     0,   198,     0,     0,    48,    49,    61,   147,   158,
     158,   158,   158,   158,   158,   158,   157,     0,   172,     0,
     176,   170,     0,     0,     0,   208,     0,   205,    56,    55,
      57,    58,    59,    54,    51,    52,     0,     0,   111,   155,
     160,   159,   162,   163,   164,   161,   241,   241,     0,     0,
       0,   197,   200,   199,   207,     0,     0,     0,     0,     0,
       0,     0,    71,    65,    68,    69,    70,   148,     0,   158,
     242,     0,   177,   188,   189,     0,    53,    13,    67,    12,
       0,    15,    14,    66,    64,     0,     0,    63,   106,    62,
       0,     0,     0,     0,     0,     0,     0,   122,   115,   116,
     119,   120,   121,     0,   173,   241,     0,     0,   107,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    60,
     118,     0,   117,   114,   113,   112,     0,   174,     0,   178,
     201,   202,   203,    98,   105,   104,   184,   184,    82,    84,
      85,    77,    78,    80,    81,    83,    79,    75,     0,    74,
      76,   184,    72,    73,    88,    87,    86,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   110,     0,     0,     0,
       0,     0,     0,    98,   184,   184,   133,   135,   136,   128,
     129,   131,   132,   134,   130,   126,     0,   125,   127,   184,
     123,   124,   139,   138,   137,   179,   204,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   109,     0,
     108,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     150,     0,     0,     0,     0,    92,     0,     0,     0,     0,
     149,     0,     0,     0,     0,     0,    89,     0,     0,     0,
     151,    92,     0,     0,     0,     0,     0,    99,     0,   140,
       0,     0,     0,     0,     0,     0,    91,     0,     0,   101,
       0,    93,    99,   100,     0,     0,     0,   147,     0,     0,
      90,     0,   101,   102,   145,     0,    94,     0,   147,     0,
     144,   146,     0,     0,     0,   145,   103,     0,    95,    97,
     143,     0,     0,   145,    96,     0,   142,   147,   145,   141
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,     1,     2,    15,    23,    21,   322,   323,    55,   123,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,   166,   245,   274,   275,    67,   276,   330,   388,   397,
     506,   402,   303,   526,   537,   327,   479,    68,   308,   376,
     450,   436,   445,   550,   278,   339,   500,    69,   152,   175,
     235,    70,   154,   176,   237,   239,    16,    17,    85,   146,
      71,   134,   184,   206,   264,   293,   381,    72,   243,    22,
      80,    81,   124,   113,   114,   115,   125,   172,   126,   257,
     319,    19,    20
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -492
static const short yypact[] =
{
    -492,    13,     3,  -492,  -492,  -492,  -492,  -492,  -492,  -492,
     238,   238,   238,    64,   238,  -492,  -492,   -56,  -492,   405,
    -492,   363,   617,   238,  -492,  -492,  -492,  -492,   556,  -492,
     363,   238,   238,   238,   238,   238,   238,   238,  -492,  -492,
    -492,  -492,  -492,  -492,  -492,  -492,  -492,  -492,  -492,  -492,
    -492,  -492,  -492,  -492,  -492,  -492,  -492,  -492,  -492,  -492,
    -492,  -492,  -492,  -492,  -492,  -492,  -492,  -492,  -492,  -492,
    -492,  -492,  -492,  -492,  -492,  -492,  -492,  -492,  -492,  -492,
    -492,  -492,   -45,   -53,  -492,    82,  -492,   232,   368,   309,
     134,   134,  -492,  -492,   617,   282,   282,    80,    80,   617,
     617,   282,   282,   617,   282,   282,   282,    80,   282,   282,
     238,   238,   238,  -492,  -492,  -492,   238,  -492,   -44,   -38,
    -492,  -492,  -492,  -492,    18,  -492,  -492,  -492,   -23,    11,
     199,   289,  -492,  -492,   407,  -492,  -492,  -492,    34,  -492,
    -492,  -492,  -492,    95,  -492,  -492,    67,  -492,    41,   238,
     238,  -492,  -492,  -492,  -492,   238,   238,   238,   238,   238,
     238,   238,  -492,   238,   238,  -492,    26,  -492,  -492,  -492,
    -492,   241,   -52,    97,   103,   128,   175,   112,   115,  -492,
    -492,  -492,  -492,  -492,   -27,  -492,  -492,   121,  -492,   123,
    -492,  -492,  -492,    57,   238,   238,    -5,    86,  -492,    -1,
     109,  -492,   238,   238,   139,  -492,  -492,   359,   238,   238,
    -492,  -492,  -492,  -492,   278,   131,   144,   -44,  -492,   148,
     161,  -492,   174,   181,   185,   192,  -492,   198,   198,  -492,
    -492,   238,   238,   238,   204,   214,   238,  -492,   238,   218,
     238,   238,  -492,   179,   498,  -492,  -492,  -492,   211,   148,
     148,   148,   148,   148,   148,   148,  -492,   217,   220,   233,
     220,  -492,   234,   247,   -30,  -492,   238,  -492,  -492,  -492,
    -492,  -492,  -492,  -492,   253,  -492,   330,   238,  -492,  -492,
    -492,  -492,  -492,  -492,  -492,  -492,   238,   238,   238,   238,
     169,  -492,  -492,  -492,  -492,   498,   206,   238,    32,   293,
      12,    12,  -492,  -492,  -492,  -492,  -492,  -492,   341,   261,
    -492,   271,   220,  -492,  -492,   272,  -492,  -492,  -492,  -492,
     273,  -492,  -492,  -492,  -492,    64,    12,   231,  -492,   231,
     565,   206,   238,    32,   297,    12,    12,  -492,  -492,  -492,
    -492,  -492,  -492,    52,  -492,   238,   215,   362,  -492,    -7,
      12,    32,   365,   365,   365,   365,   365,   365,   365,   365,
     365,   238,   238,   238,    32,   238,   365,   365,   365,  -492,
    -492,   290,  -492,  -492,   231,   231,   583,  -492,   294,   220,
    -492,   313,  -492,  -492,  -492,  -492,   -44,   -44,  -492,  -492,
    -492,  -492,  -492,  -492,  -492,  -492,  -492,  -492,   314,  -492,
    -492,   -44,  -492,  -492,  -492,  -492,  -492,   402,    32,   403,
     403,   403,   403,   403,   403,   403,   403,   403,   238,   238,
     238,    32,   238,   403,   403,   403,  -492,   238,   238,   238,
     238,   238,   238,   321,   -44,   -44,  -492,  -492,  -492,  -492,
    -492,  -492,  -492,  -492,  -492,  -492,   331,  -492,  -492,   -44,
    -492,  -492,  -492,  -492,  -492,  -492,  -492,   338,   347,   349,
     350,   238,   238,   238,   238,   238,   238,   238,   238,   221,
     351,   353,   354,   356,   357,   376,   377,   378,  -492,   409,
    -492,   238,   238,   238,   238,   221,   238,   238,   238,    12,
     415,   422,   430,   431,   434,   435,   436,   437,    47,   238,
    -492,   238,   238,   238,    12,   238,  -492,   238,   238,   238,
    -492,   435,   438,   439,    58,   442,   444,   445,   447,  -492,
     238,   238,   238,   238,   238,    12,  -492,   238,   452,   453,
     456,   458,   445,   231,   460,   238,    12,   211,   238,    12,
    -492,   238,   453,    94,   388,   461,    96,   462,   211,    12,
    -492,  -492,   238,    12,   238,   388,   231,   463,   231,   464,
    -492,   238,    12,   135,   231,    12,  -492,   111,   388,  -492
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -492,  -492,  -492,  -492,  -492,   440,  -276,  -173,   465,  -492,
    -492,  -492,  -492,  -492,  -492,  -492,  -492,  -492,  -492,  -492,
    -492,  -492,   268,  -492,   203,  -492,  -492,  -492,   210,   -94,
      49,  -492,   254,    40,    31,  -292,    89,  -492,  -492,  -492,
    -492,   242,  -270,  -491,  -446,  -492,  -492,  -492,   344,  -492,
     -69,  -492,   342,  -492,  -492,  -492,  -492,  -492,  -492,  -169,
    -492,  -492,  -492,   316,  -492,  -492,  -492,  -492,  -492,   584,
    -492,  -492,   -20,  -249,  -248,  -243,  -213,  -492,   408,  -234,
      -2,   401,    22
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -243
static const short yytable[] =
{
      18,   217,    82,   193,   259,   220,     4,     5,     6,   329,
       7,   204,   290,     3,   204,   110,     5,     6,   111,   112,
     318,    83,   110,     8,    29,   111,   112,   304,   305,   192,
       5,     6,   187,   306,   349,    27,     5,     6,   321,   317,
     116,   145,   147,   374,   375,   167,   168,   169,   233,   145,
     170,   291,   309,   311,   205,   370,     5,     6,   385,   340,
     341,   210,   211,   212,   560,   342,   213,     9,     5,     6,
     149,   218,   566,   165,   119,   221,   350,   569,   110,   130,
     131,   111,   112,   118,     5,     6,     5,     6,   384,    10,
      11,   544,   219,    12,    13,   128,   129,    14,   148,   250,
     251,   252,   555,   325,   150,   138,   326,   188,   141,   142,
     143,   378,    10,    11,   144,   222,    12,    13,    10,    11,
      14,   568,    12,    13,   253,   254,    14,   163,   171,   255,
     350,    73,    74,    75,    76,    77,    78,    79,    10,    11,
     509,   350,    12,    13,   214,   225,    14,   173,   174,   447,
     448,   522,   451,   177,   178,   179,   180,   181,   182,   183,
     372,   185,   186,   117,   189,   207,    10,    11,    10,    11,
      12,    13,    12,    13,    14,   315,    14,   350,   386,   350,
     279,   280,   281,   282,   283,   284,   285,   549,   164,   553,
     194,   401,   215,   216,   350,  -219,   195,   498,  -219,  -219,
     223,   224,   196,   197,   277,   202,   227,   228,   203,   198,
       5,     6,   514,   317,   208,  -219,   209,   429,   430,     5,
       6,   380,    36,    37,   231,     5,     6,   478,   565,   247,
     248,   249,   432,   533,   258,   434,   260,   232,   262,   263,
     344,   234,     5,     6,   543,   190,   191,   546,   449,   199,
     200,   250,   251,   252,   236,   265,   201,   556,   266,   110,
     267,   558,   111,   112,   294,   462,   463,   238,   399,   400,
     564,   403,   242,   567,   240,   307,   253,   254,   241,   151,
     465,   255,   229,   230,   310,   312,   313,   314,   120,   121,
     122,   244,    10,    11,   151,   320,    12,    13,   153,   324,
      14,    10,    11,   373,   277,    12,    13,    10,    11,    14,
     286,    12,    13,  -242,   350,    14,    32,    33,    34,    35,
      36,    37,   328,   328,    10,    11,   287,   288,    12,    13,
     371,   551,    14,    73,    74,    75,    76,    77,    78,    79,
     289,   377,   551,   379,   382,   296,   295,   348,   328,   110,
     551,   297,   111,   112,   343,   551,   331,   328,   328,   398,
     398,   398,   332,   398,   345,   346,   347,    38,   383,   153,
      39,   387,   328,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,   407,    50,    51,    52,   427,   298,   299,
     110,   300,   301,   111,   112,    34,    35,    36,    37,   333,
     334,   110,   335,   336,   111,   112,   428,   431,   433,   435,
     302,    24,    25,    26,   461,    28,   446,   446,   446,   110,
     446,   337,   111,   112,   464,   455,   456,   457,   458,   459,
     460,   466,    87,    88,    89,    90,    91,    92,    93,   226,
     467,    53,   468,   469,   481,    54,   482,   483,  -219,   484,
     485,  -219,  -219,    33,    34,    35,    36,    37,    30,   470,
     471,   472,   473,   474,   475,   476,   477,   480,  -219,   486,
     487,   488,   155,   156,   157,   158,   159,   160,   161,   490,
     491,   492,   493,   480,   495,   496,   497,   162,    31,    32,
      33,    34,    35,    36,    37,    86,   246,   510,   316,   511,
     512,   513,   489,   515,   127,   516,   517,   518,   499,   132,
     133,   328,   135,   136,   137,   501,   139,   140,   528,   529,
     530,   531,   532,   502,   503,   534,   328,   504,   505,   507,
     508,   520,   521,   542,   268,   523,   545,   524,   525,   547,
     527,   269,   270,   271,   272,   535,   536,   328,   273,   538,
     557,   539,   559,   541,   552,   554,   561,   562,   328,   563,
     519,   328,   338,   389,   390,   391,   392,   393,   394,   395,
     396,   328,   540,   548,   494,   328,   404,   405,   406,   256,
     292,   261,     0,     0,   328,     0,     0,   328,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   368,   408,   409,   410,   411,
     412,   413,   414,   415,   416,   417,   418,   419,   420,   421,
     422,   423,   424,   425,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,    31,
      32,    33,    34,    35,    36,    37,   369,     0,     0,   109,
       0,    84,   437,   438,   439,   440,   441,   442,   443,   444,
       0,     0,     0,     0,   426,   452,   453,   454,    73,    74,
      75,    76,    77,    78,    79
};

static const short yycheck[] =
{
       2,     6,    22,   172,   238,     6,     3,     4,     5,   301,
       7,    41,    42,     0,    41,    60,     4,     5,    63,    64,
     296,    23,    60,    20,    80,    63,    64,   276,   276,    81,
       4,     5,     6,   276,   326,    13,     4,     5,     6,     7,
      93,    93,    80,   335,   336,     4,     5,     6,   217,    93,
       9,    81,   286,   287,    81,   331,     4,     5,   350,   308,
     308,     4,     5,     6,   555,   308,     9,    64,     4,     5,
      93,    76,   563,     6,    94,    76,    83,   568,    60,    99,
     100,    63,    64,    85,     4,     5,     4,     5,    95,    86,
      87,   537,     6,    90,    91,    97,    98,    94,    80,    47,
      48,    49,   548,    91,    93,   107,    94,    81,   110,   111,
     112,   345,    86,    87,   116,     6,    90,    91,    86,    87,
      94,   567,    90,    91,    72,    73,    94,    93,    87,    77,
      83,    51,    52,    53,    54,    55,    56,    57,    86,    87,
      93,    83,    90,    91,    87,     6,    94,   149,   150,   419,
     420,    93,   422,   155,   156,   157,   158,   159,   160,   161,
     333,   163,   164,    81,   166,   185,    86,    87,    86,    87,
      90,    91,    90,    91,    94,     6,    94,    83,   351,    83,
     249,   250,   251,   252,   253,   254,   255,    93,    93,    93,
      93,   364,   194,   195,    83,    60,    93,   489,    63,    64,
     202,   203,    74,    75,    93,    93,   208,   209,    93,    81,
       4,     5,   504,     7,    93,    80,    93,   386,   387,     4,
       5,     6,    88,    89,    93,     4,     5,     6,    93,   231,
     232,   233,   401,   525,   236,   408,   238,    93,   240,   241,
     309,    93,     4,     5,   536,     4,     5,   539,   421,    74,
      75,    47,    48,    49,    93,    76,    81,   549,    79,    60,
      81,   553,    63,    64,   266,   434,   435,    93,   362,   363,
     562,   365,    80,   565,    93,   277,    72,    73,    93,    80,
     449,    77,     4,     5,   286,   287,   288,   289,     6,     7,
       8,    93,    86,    87,    80,   297,    90,    91,    80,     6,
      94,    86,    87,     6,    93,    90,    91,    86,    87,    94,
      93,    90,    91,    93,    83,    94,    84,    85,    86,    87,
      88,    89,   300,   301,    86,    87,    93,    93,    90,    91,
     332,   544,    94,    51,    52,    53,    54,    55,    56,    57,
      93,   343,   555,   345,   346,    15,    93,   325,   326,    60,
     563,    21,    63,    64,    93,   568,    15,   335,   336,   361,
     362,   363,    21,   365,    93,    93,    93,     4,     6,    80,
       7,     6,   350,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    93,    21,    22,    23,    93,    58,    59,
      60,    61,    62,    63,    64,    86,    87,    88,    89,    58,
      59,    60,    61,    62,    63,    64,    93,    93,     6,     6,
      80,    10,    11,    12,    93,    14,   418,   419,   420,    60,
     422,    80,    63,    64,    93,   427,   428,   429,   430,   431,
     432,    93,    31,    32,    33,    34,    35,    36,    37,    80,
      93,    78,    93,    93,    93,    82,    93,    93,    60,    93,
      93,    63,    64,    85,    86,    87,    88,    89,    18,   461,
     462,   463,   464,   465,   466,   467,   468,   469,    80,    93,
      93,    93,    65,    66,    67,    68,    69,    70,    71,   481,
     482,   483,   484,   485,   486,   487,   488,    80,    83,    84,
      85,    86,    87,    88,    89,    30,   228,   499,   295,   501,
     502,   503,    93,   505,    96,   507,   508,   509,    93,   101,
     102,   489,   104,   105,   106,    93,   108,   109,   520,   521,
     522,   523,   524,    93,    93,   527,   504,    93,    93,    93,
      93,    93,    93,   535,    36,    93,   538,    93,    93,   541,
      93,    43,    44,    45,    46,    93,    93,   525,    50,    93,
     552,    93,   554,    93,    93,    93,    93,    93,   536,   561,
     511,   539,   308,   353,   354,   355,   356,   357,   358,   359,
     360,   549,   532,   542,   485,   553,   366,   367,   368,   235,
     264,   239,    -1,    -1,   562,    -1,    -1,   565,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    83,
      84,    85,    86,    87,    88,    89,    81,    -1,    -1,    65,
      -1,    95,   410,   411,   412,   413,   414,   415,   416,   417,
      -1,    -1,    -1,    -1,    81,   423,   424,   425,    51,    52,
      53,    54,    55,    56,    57
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    97,    98,     0,     3,     4,     5,     7,    20,    64,
      86,    87,    90,    91,    94,    99,   152,   153,   176,   177,
     178,   101,   165,   100,   177,   177,   177,   178,   177,    80,
     101,    83,    84,    85,    86,    87,    88,    89,     4,     7,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      21,    22,    23,    78,    82,   104,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   121,   133,   143,
     147,   156,   163,    51,    52,    53,    54,    55,    56,    57,
     166,   167,   168,   176,    95,   154,   104,   177,   177,   177,
     177,   177,   177,   177,   165,   165,   165,   165,   165,   165,
     165,   165,   165,   165,   165,   165,   165,   165,   165,   165,
      60,    63,    64,   169,   170,   171,    93,    81,   176,   168,
       6,     7,     8,   105,   168,   172,   174,   174,   176,   176,
     168,   168,   174,   174,   157,   174,   174,   174,   176,   174,
     174,   176,   176,   176,   176,    93,   155,    80,    80,    93,
      93,    80,   144,    80,   148,    65,    66,    67,    68,    69,
      70,    71,    80,    93,    93,     6,   117,     4,     5,     6,
       9,    87,   173,   176,   176,   145,   149,   176,   176,   176,
     176,   176,   176,   176,   158,   176,   176,     6,    81,   176,
       4,     5,    81,   155,    93,    93,    74,    75,    81,    74,
      75,    81,    93,    93,    41,    81,   159,   168,    93,    93,
       4,     5,     6,     9,    87,   176,   176,     6,    76,     6,
       6,    76,     6,   176,   176,     6,    80,   176,   176,     4,
       5,    93,    93,   155,    93,   146,    93,   150,    93,   151,
      93,    93,    80,   164,    93,   118,   118,   176,   176,   176,
      47,    48,    49,    72,    73,    77,   144,   175,   176,   175,
     176,   148,   176,   176,   160,    76,    79,    81,    36,    43,
      44,    45,    46,    50,   119,   120,   122,    93,   140,   146,
     146,   146,   146,   146,   146,   146,    93,    93,    93,    93,
      42,    81,   159,   161,   176,    93,    15,    21,    58,    59,
      61,    62,    80,   128,   169,   170,   171,   176,   134,   175,
     176,   175,   176,   176,   176,     6,   120,     7,   102,   176,
     176,     6,   102,   103,     6,    91,    94,   131,   178,   131,
     123,    15,    21,    58,    59,    61,    62,    80,   128,   141,
     169,   170,   171,    93,   146,    93,    93,    93,   178,   131,
      83,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    81,
     102,   176,   103,     6,   131,   131,   135,   176,   175,   176,
       6,   162,   176,     6,    95,   131,   103,     6,   124,   124,
     124,   124,   124,   124,   124,   124,   124,   125,   176,   125,
     125,   103,   127,   125,   124,   124,   124,    93,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    81,    93,    93,   155,
     155,    93,   155,     6,   103,     6,   137,   137,   137,   137,
     137,   137,   137,   137,   137,   138,   176,   138,   138,   103,
     136,   138,   137,   137,   137,   176,   176,   176,   176,   176,
     176,    93,   155,   155,    93,   155,    93,    93,    93,    93,
     176,   176,   176,   176,   176,   176,   176,   176,     6,   132,
     176,    93,    93,    93,    93,    93,    93,    93,    93,    93,
     176,   176,   176,   176,   132,   176,   176,   176,   131,    93,
     142,    93,    93,    93,    93,    93,   126,    93,    93,    93,
     176,   176,   176,   176,   131,   176,   176,   176,   176,   126,
      93,    93,    93,    93,    93,    93,   129,    93,   176,   176,
     176,   176,   176,   131,   176,    93,    93,   130,    93,    93,
     129,    93,   176,   131,   140,   176,   131,   176,   130,    93,
     139,   172,    93,    93,    93,   140,   131,   176,   131,   176,
     139,    93,    93,   176,   131,    93,   139,   131,   140,   139
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
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
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
| TOP (cinluded).                                                   |
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

#if YYMAXDEPTH == 0
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
#line 362 "parser.y"
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
#line 396 "parser.y"
    { yyval.res = NULL; want_id = 1; ;}
    break;

  case 4:
#line 397 "parser.y"
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
#line 473 "parser.y"
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
#line 485 "parser.y"
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
#line 495 "parser.y"
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
#line 503 "parser.y"
    {want_nl = 1; ;}
    break;

  case 10:
#line 503 "parser.y"
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
#line 547 "parser.y"
    { yychar = rsrcid_to_token(yychar); ;}
    break;

  case 12:
#line 553 "parser.y"
    {
		if(yyvsp[0].num > 65535 || yyvsp[0].num < -32768)
			yyerror("Resource's ID out of range (%d)", yyvsp[0].num);
		yyval.nid = new_name_id();
		yyval.nid->type = name_ord;
		yyval.nid->name.i_name = yyvsp[0].num;
		;}
    break;

  case 13:
#line 560 "parser.y"
    {
		yyval.nid = new_name_id();
		yyval.nid->type = name_str;
		yyval.nid->name.s_name = yyvsp[0].str;
		;}
    break;

  case 14:
#line 570 "parser.y"
    { yyval.nid = yyvsp[0].nid; ;}
    break;

  case 15:
#line 571 "parser.y"
    {
		yyval.nid = new_name_id();
		yyval.nid->type = name_str;
		yyval.nid->name.s_name = yyvsp[0].str;
		;}
    break;

  case 16:
#line 580 "parser.y"
    { yyval.res = new_resource(res_acc, yyvsp[0].acc, yyvsp[0].acc->memopt, yyvsp[0].acc->lvc.language); ;}
    break;

  case 17:
#line 581 "parser.y"
    { yyval.res = new_resource(res_bmp, yyvsp[0].bmp, yyvsp[0].bmp->memopt, yyvsp[0].bmp->data->lvc.language); ;}
    break;

  case 18:
#line 582 "parser.y"
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
#line 606 "parser.y"
    { yyval.res = new_resource(res_dlg, yyvsp[0].dlg, yyvsp[0].dlg->memopt, yyvsp[0].dlg->lvc.language); ;}
    break;

  case 20:
#line 607 "parser.y"
    {
		if(win32)
			yyval.res = new_resource(res_dlgex, yyvsp[0].dlgex, yyvsp[0].dlgex->memopt, yyvsp[0].dlgex->lvc.language);
		else
			yyval.res = NULL;
		;}
    break;

  case 21:
#line 613 "parser.y"
    { yyval.res = new_resource(res_dlginit, yyvsp[0].dginit, yyvsp[0].dginit->memopt, yyvsp[0].dginit->data->lvc.language); ;}
    break;

  case 22:
#line 614 "parser.y"
    { yyval.res = new_resource(res_fnt, yyvsp[0].fnt, yyvsp[0].fnt->memopt, yyvsp[0].fnt->data->lvc.language); ;}
    break;

  case 23:
#line 615 "parser.y"
    { yyval.res = new_resource(res_fntdir, yyvsp[0].fnd, yyvsp[0].fnd->memopt, yyvsp[0].fnd->data->lvc.language); ;}
    break;

  case 24:
#line 616 "parser.y"
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
#line 640 "parser.y"
    { yyval.res = new_resource(res_men, yyvsp[0].men, yyvsp[0].men->memopt, yyvsp[0].men->lvc.language); ;}
    break;

  case 26:
#line 641 "parser.y"
    {
		if(win32)
			yyval.res = new_resource(res_menex, yyvsp[0].menex, yyvsp[0].menex->memopt, yyvsp[0].menex->lvc.language);
		else
			yyval.res = NULL;
		;}
    break;

  case 27:
#line 647 "parser.y"
    { yyval.res = new_resource(res_msg, yyvsp[0].msg, WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE, yyvsp[0].msg->data->lvc.language); ;}
    break;

  case 28:
#line 648 "parser.y"
    { yyval.res = new_resource(res_rdt, yyvsp[0].rdt, yyvsp[0].rdt->memopt, yyvsp[0].rdt->data->lvc.language); ;}
    break;

  case 29:
#line 649 "parser.y"
    { yyval.res = new_resource(res_toolbar, yyvsp[0].tlbar, yyvsp[0].tlbar->memopt, yyvsp[0].tlbar->lvc.language); ;}
    break;

  case 30:
#line 650 "parser.y"
    { yyval.res = new_resource(res_usr, yyvsp[0].usr, yyvsp[0].usr->memopt, yyvsp[0].usr->data->lvc.language); ;}
    break;

  case 31:
#line 651 "parser.y"
    { yyval.res = new_resource(res_ver, yyvsp[0].veri, WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE, yyvsp[0].veri->lvc.language); ;}
    break;

  case 32:
#line 655 "parser.y"
    { yyval.str = make_filename(yyvsp[0].str); ;}
    break;

  case 33:
#line 656 "parser.y"
    { yyval.str = make_filename(yyvsp[0].str); ;}
    break;

  case 34:
#line 657 "parser.y"
    { yyval.str = make_filename(yyvsp[0].str); ;}
    break;

  case 35:
#line 661 "parser.y"
    { yyval.bmp = new_bitmap(yyvsp[0].raw, yyvsp[-1].iptr); ;}
    break;

  case 36:
#line 665 "parser.y"
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

  case 37:
#line 681 "parser.y"
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

  case 38:
#line 703 "parser.y"
    { yyval.fnt = new_font(yyvsp[0].raw, yyvsp[-1].iptr); ;}
    break;

  case 39:
#line 713 "parser.y"
    { yyval.fnd = new_fontdir(yyvsp[0].raw, yyvsp[-1].iptr); ;}
    break;

  case 40:
#line 721 "parser.y"
    {
		if(!win32)
			yywarning("MESSAGETABLE not supported in 16-bit mode");
		yyval.msg = new_messagetable(yyvsp[0].raw, yyvsp[-1].iptr);
		;}
    break;

  case 41:
#line 729 "parser.y"
    { yyval.rdt = new_rcdata(yyvsp[0].raw, yyvsp[-1].iptr); ;}
    break;

  case 42:
#line 733 "parser.y"
    { yyval.dginit = new_dlginit(yyvsp[0].raw, yyvsp[-1].iptr); ;}
    break;

  case 43:
#line 737 "parser.y"
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

  case 44:
#line 748 "parser.y"
    {
		yyval.nid = new_name_id();
		yyval.nid->type = name_ord;
		yyval.nid->name.i_name = yyvsp[0].num;
		;}
    break;

  case 45:
#line 753 "parser.y"
    {
		yyval.nid = new_name_id();
		yyval.nid->type = name_str;
		yyval.nid->name.s_name = yyvsp[0].str;
		;}
    break;

  case 46:
#line 762 "parser.y"
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

  case 47:
#line 786 "parser.y"
    { yyval.event=NULL; ;}
    break;

  case 48:
#line 787 "parser.y"
    { yyval.event=add_string_event(yyvsp[-3].str, yyvsp[-1].num, yyvsp[0].num, yyvsp[-4].event); ;}
    break;

  case 49:
#line 788 "parser.y"
    { yyval.event=add_event(yyvsp[-3].num, yyvsp[-1].num, yyvsp[0].num, yyvsp[-4].event); ;}
    break;

  case 50:
#line 797 "parser.y"
    { yyval.num = 0; ;}
    break;

  case 51:
#line 798 "parser.y"
    { yyval.num = yyvsp[0].num; ;}
    break;

  case 52:
#line 801 "parser.y"
    { yyval.num = yyvsp[0].num; ;}
    break;

  case 53:
#line 802 "parser.y"
    { yyval.num = yyvsp[-2].num | yyvsp[0].num; ;}
    break;

  case 54:
#line 805 "parser.y"
    { yyval.num = WRC_AF_NOINVERT; ;}
    break;

  case 55:
#line 806 "parser.y"
    { yyval.num = WRC_AF_SHIFT; ;}
    break;

  case 56:
#line 807 "parser.y"
    { yyval.num = WRC_AF_CONTROL; ;}
    break;

  case 57:
#line 808 "parser.y"
    { yyval.num = WRC_AF_ALT; ;}
    break;

  case 58:
#line 809 "parser.y"
    { yyval.num = WRC_AF_ASCII; ;}
    break;

  case 59:
#line 810 "parser.y"
    { yyval.num = WRC_AF_VIRTKEY; ;}
    break;

  case 60:
#line 816 "parser.y"
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

  case 61:
#line 850 "parser.y"
    { yyval.dlg=new_dialog(); ;}
    break;

  case 62:
#line 851 "parser.y"
    { yyval.dlg=dialog_style(yyvsp[0].style,yyvsp[-2].dlg); ;}
    break;

  case 63:
#line 852 "parser.y"
    { yyval.dlg=dialog_exstyle(yyvsp[0].style,yyvsp[-2].dlg); ;}
    break;

  case 64:
#line 853 "parser.y"
    { yyval.dlg=dialog_caption(yyvsp[0].str,yyvsp[-2].dlg); ;}
    break;

  case 65:
#line 854 "parser.y"
    { yyval.dlg=dialog_font(yyvsp[0].fntid,yyvsp[-1].dlg); ;}
    break;

  case 66:
#line 855 "parser.y"
    { yyval.dlg=dialog_class(yyvsp[0].nid,yyvsp[-2].dlg); ;}
    break;

  case 67:
#line 856 "parser.y"
    { yyval.dlg=dialog_menu(yyvsp[0].nid,yyvsp[-2].dlg); ;}
    break;

  case 68:
#line 857 "parser.y"
    { yyval.dlg=dialog_language(yyvsp[0].lan,yyvsp[-1].dlg); ;}
    break;

  case 69:
#line 858 "parser.y"
    { yyval.dlg=dialog_characteristics(yyvsp[0].chars,yyvsp[-1].dlg); ;}
    break;

  case 70:
#line 859 "parser.y"
    { yyval.dlg=dialog_version(yyvsp[0].ver,yyvsp[-1].dlg); ;}
    break;

  case 71:
#line 862 "parser.y"
    { yyval.ctl = NULL; ;}
    break;

  case 72:
#line 863 "parser.y"
    { yyval.ctl=ins_ctrl(-1, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 73:
#line 864 "parser.y"
    { yyval.ctl=ins_ctrl(CT_EDIT, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 74:
#line 865 "parser.y"
    { yyval.ctl=ins_ctrl(CT_LISTBOX, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 75:
#line 866 "parser.y"
    { yyval.ctl=ins_ctrl(CT_COMBOBOX, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 76:
#line 867 "parser.y"
    { yyval.ctl=ins_ctrl(CT_SCROLLBAR, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 77:
#line 868 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_CHECKBOX, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 78:
#line 869 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_DEFPUSHBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 79:
#line 870 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_GROUPBOX, yyvsp[0].ctl, yyvsp[-2].ctl);;}
    break;

  case 80:
#line 871 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_PUSHBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 81:
#line 873 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_RADIOBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 82:
#line 874 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_AUTO3STATE, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 83:
#line 875 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_3STATE, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 84:
#line 876 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_AUTOCHECKBOX, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 85:
#line 877 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_AUTORADIOBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 86:
#line 878 "parser.y"
    { yyval.ctl=ins_ctrl(CT_STATIC, SS_LEFT, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 87:
#line 879 "parser.y"
    { yyval.ctl=ins_ctrl(CT_STATIC, SS_CENTER, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 88:
#line 880 "parser.y"
    { yyval.ctl=ins_ctrl(CT_STATIC, SS_RIGHT, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 89:
#line 882 "parser.y"
    {
		yyvsp[0].ctl->title = yyvsp[-7].nid;
		yyvsp[0].ctl->id = yyvsp[-5].num;
		yyvsp[0].ctl->x = yyvsp[-3].num;
		yyvsp[0].ctl->y = yyvsp[-1].num;
		yyval.ctl = ins_ctrl(CT_STATIC, SS_ICON, yyvsp[0].ctl, yyvsp[-9].ctl);
		;}
    break;

  case 90:
#line 892 "parser.y"
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
		if(yyvsp[0].style)
		{
			yyval.ctl->style = yyvsp[0].style;
			yyval.ctl->gotstyle = TRUE;
		}
		;}
    break;

  case 91:
#line 911 "parser.y"
    {
		yyval.ctl = new_control();
		yyval.ctl->id = yyvsp[-9].num;
		yyval.ctl->x = yyvsp[-7].num;
		yyval.ctl->y = yyvsp[-5].num;
		yyval.ctl->width = yyvsp[-3].num;
		yyval.ctl->height = yyvsp[-1].num;
		if(yyvsp[0].style)
		{
			yyval.ctl->style = yyvsp[0].style;
			yyval.ctl->gotstyle = TRUE;
		}
		;}
    break;

  case 92:
#line 927 "parser.y"
    { yyval.ctl = new_control(); ;}
    break;

  case 93:
#line 929 "parser.y"
    {
		yyval.ctl = new_control();
		yyval.ctl->width = yyvsp[-2].num;
		yyval.ctl->height = yyvsp[0].num;
		;}
    break;

  case 94:
#line 934 "parser.y"
    {
		yyval.ctl = new_control();
		yyval.ctl->width = yyvsp[-4].num;
		yyval.ctl->height = yyvsp[-2].num;
		yyval.ctl->style = yyvsp[0].style;
		yyval.ctl->gotstyle = TRUE;
		;}
    break;

  case 95:
#line 941 "parser.y"
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

  case 96:
#line 952 "parser.y"
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

  case 97:
#line 966 "parser.y"
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

  case 98:
#line 981 "parser.y"
    { yyval.fntid = new_font_id(yyvsp[-2].num, yyvsp[0].str, 0, 0); ;}
    break;

  case 99:
#line 986 "parser.y"
    { yyval.style = NULL; ;}
    break;

  case 100:
#line 987 "parser.y"
    { yyval.style = yyvsp[0].style; ;}
    break;

  case 101:
#line 991 "parser.y"
    { yyval.styles = NULL; ;}
    break;

  case 102:
#line 992 "parser.y"
    { yyval.styles = new_style_pair(yyvsp[0].style, 0); ;}
    break;

  case 103:
#line 993 "parser.y"
    { yyval.styles = new_style_pair(yyvsp[-2].style, yyvsp[0].style); ;}
    break;

  case 104:
#line 997 "parser.y"
    { yyval.style = new_style(yyvsp[-2].style->or_mask | yyvsp[0].style->or_mask, yyvsp[-2].style->and_mask | yyvsp[0].style->and_mask); free(yyvsp[-2].style); free(yyvsp[0].style);;}
    break;

  case 105:
#line 998 "parser.y"
    { yyval.style = yyvsp[-1].style; ;}
    break;

  case 106:
#line 999 "parser.y"
    { yyval.style = new_style(yyvsp[0].num, 0); ;}
    break;

  case 107:
#line 1000 "parser.y"
    { yyval.style = new_style(0, yyvsp[0].num); ;}
    break;

  case 108:
#line 1004 "parser.y"
    {
		yyval.nid = new_name_id();
		yyval.nid->type = name_ord;
		yyval.nid->name.i_name = yyvsp[0].num;
		;}
    break;

  case 109:
#line 1009 "parser.y"
    {
		yyval.nid = new_name_id();
		yyval.nid->type = name_str;
		yyval.nid->name.s_name = yyvsp[0].str;
		;}
    break;

  case 110:
#line 1018 "parser.y"
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
#line 1061 "parser.y"
    { yyval.dlgex=new_dialogex(); ;}
    break;

  case 112:
#line 1062 "parser.y"
    { yyval.dlgex=dialogex_style(yyvsp[0].style,yyvsp[-2].dlgex); ;}
    break;

  case 113:
#line 1063 "parser.y"
    { yyval.dlgex=dialogex_exstyle(yyvsp[0].style,yyvsp[-2].dlgex); ;}
    break;

  case 114:
#line 1064 "parser.y"
    { yyval.dlgex=dialogex_caption(yyvsp[0].str,yyvsp[-2].dlgex); ;}
    break;

  case 115:
#line 1065 "parser.y"
    { yyval.dlgex=dialogex_font(yyvsp[0].fntid,yyvsp[-1].dlgex); ;}
    break;

  case 116:
#line 1066 "parser.y"
    { yyval.dlgex=dialogex_font(yyvsp[0].fntid,yyvsp[-1].dlgex); ;}
    break;

  case 117:
#line 1067 "parser.y"
    { yyval.dlgex=dialogex_class(yyvsp[0].nid,yyvsp[-2].dlgex); ;}
    break;

  case 118:
#line 1068 "parser.y"
    { yyval.dlgex=dialogex_menu(yyvsp[0].nid,yyvsp[-2].dlgex); ;}
    break;

  case 119:
#line 1069 "parser.y"
    { yyval.dlgex=dialogex_language(yyvsp[0].lan,yyvsp[-1].dlgex); ;}
    break;

  case 120:
#line 1070 "parser.y"
    { yyval.dlgex=dialogex_characteristics(yyvsp[0].chars,yyvsp[-1].dlgex); ;}
    break;

  case 121:
#line 1071 "parser.y"
    { yyval.dlgex=dialogex_version(yyvsp[0].ver,yyvsp[-1].dlgex); ;}
    break;

  case 122:
#line 1074 "parser.y"
    { yyval.ctl = NULL; ;}
    break;

  case 123:
#line 1075 "parser.y"
    { yyval.ctl=ins_ctrl(-1, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 124:
#line 1076 "parser.y"
    { yyval.ctl=ins_ctrl(CT_EDIT, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 125:
#line 1077 "parser.y"
    { yyval.ctl=ins_ctrl(CT_LISTBOX, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 126:
#line 1078 "parser.y"
    { yyval.ctl=ins_ctrl(CT_COMBOBOX, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 127:
#line 1079 "parser.y"
    { yyval.ctl=ins_ctrl(CT_SCROLLBAR, 0, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 128:
#line 1080 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_CHECKBOX, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 129:
#line 1081 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_DEFPUSHBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 130:
#line 1082 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_GROUPBOX, yyvsp[0].ctl, yyvsp[-2].ctl);;}
    break;

  case 131:
#line 1083 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_PUSHBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 132:
#line 1085 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_RADIOBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 133:
#line 1086 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_AUTO3STATE, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 134:
#line 1087 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_3STATE, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 135:
#line 1088 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_AUTOCHECKBOX, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 136:
#line 1089 "parser.y"
    { yyval.ctl=ins_ctrl(CT_BUTTON, BS_AUTORADIOBUTTON, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 137:
#line 1090 "parser.y"
    { yyval.ctl=ins_ctrl(CT_STATIC, SS_LEFT, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 138:
#line 1091 "parser.y"
    { yyval.ctl=ins_ctrl(CT_STATIC, SS_CENTER, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 139:
#line 1092 "parser.y"
    { yyval.ctl=ins_ctrl(CT_STATIC, SS_RIGHT, yyvsp[0].ctl, yyvsp[-2].ctl); ;}
    break;

  case 140:
#line 1094 "parser.y"
    {
		yyvsp[0].ctl->title = yyvsp[-7].nid;
		yyvsp[0].ctl->id = yyvsp[-5].num;
		yyvsp[0].ctl->x = yyvsp[-3].num;
		yyvsp[0].ctl->y = yyvsp[-1].num;
		yyval.ctl = ins_ctrl(CT_STATIC, SS_ICON, yyvsp[0].ctl, yyvsp[-9].ctl);
		;}
    break;

  case 141:
#line 1105 "parser.y"
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
#line 1129 "parser.y"
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
#line 1145 "parser.y"
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
#line 1173 "parser.y"
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
#line 1196 "parser.y"
    { yyval.raw = NULL; ;}
    break;

  case 146:
#line 1197 "parser.y"
    { yyval.raw = yyvsp[0].raw; ;}
    break;

  case 147:
#line 1200 "parser.y"
    { yyval.iptr = NULL; ;}
    break;

  case 148:
#line 1201 "parser.y"
    { yyval.iptr = new_int(yyvsp[0].num); ;}
    break;

  case 149:
#line 1205 "parser.y"
    { yyval.fntid = new_font_id(yyvsp[-7].num, yyvsp[-5].str, yyvsp[-3].num, yyvsp[-1].num); ;}
    break;

  case 150:
#line 1212 "parser.y"
    { yyval.fntid = NULL; ;}
    break;

  case 151:
#line 1213 "parser.y"
    { yyval.fntid = NULL; ;}
    break;

  case 152:
#line 1217 "parser.y"
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
#line 1240 "parser.y"
    { yyval.menitm = yyvsp[-1].menitm; ;}
    break;

  case 154:
#line 1244 "parser.y"
    {yyval.menitm = NULL;;}
    break;

  case 155:
#line 1245 "parser.y"
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
#line 1254 "parser.y"
    {
		yyval.menitm=new_menu_item();
		yyval.menitm->prev = yyvsp[-2].menitm;
		if(yyvsp[-2].menitm)
			yyvsp[-2].menitm->next = yyval.menitm;
		;}
    break;

  case 157:
#line 1260 "parser.y"
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
#line 1279 "parser.y"
    { yyval.num = 0; ;}
    break;

  case 159:
#line 1280 "parser.y"
    { yyval.num = yyvsp[0].num | MF_CHECKED; ;}
    break;

  case 160:
#line 1281 "parser.y"
    { yyval.num = yyvsp[0].num | MF_GRAYED; ;}
    break;

  case 161:
#line 1282 "parser.y"
    { yyval.num = yyvsp[0].num | MF_HELP; ;}
    break;

  case 162:
#line 1283 "parser.y"
    { yyval.num = yyvsp[0].num | MF_DISABLED; ;}
    break;

  case 163:
#line 1284 "parser.y"
    { yyval.num = yyvsp[0].num | MF_MENUBARBREAK; ;}
    break;

  case 164:
#line 1285 "parser.y"
    { yyval.num = yyvsp[0].num | MF_MENUBREAK; ;}
    break;

  case 165:
#line 1289 "parser.y"
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
#line 1314 "parser.y"
    { yyval.menexitm = yyvsp[-1].menexitm; ;}
    break;

  case 167:
#line 1318 "parser.y"
    {yyval.menexitm = NULL; ;}
    break;

  case 168:
#line 1319 "parser.y"
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
#line 1335 "parser.y"
    {
		yyval.menexitm = new_menuex_item();
		yyval.menexitm->prev = yyvsp[-2].menexitm;
		if(yyvsp[-2].menexitm)
			yyvsp[-2].menexitm->next = yyval.menexitm;
		;}
    break;

  case 170:
#line 1341 "parser.y"
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
#line 1361 "parser.y"
    { yyval.exopt = new_itemex_opt(0, 0, 0, 0); ;}
    break;

  case 172:
#line 1362 "parser.y"
    {
		yyval.exopt = new_itemex_opt(yyvsp[0].num, 0, 0, 0);
		yyval.exopt->gotid = TRUE;
		;}
    break;

  case 173:
#line 1366 "parser.y"
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
#line 1374 "parser.y"
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
#line 1385 "parser.y"
    { yyval.exopt = new_itemex_opt(0, 0, 0, 0); ;}
    break;

  case 176:
#line 1386 "parser.y"
    {
		yyval.exopt = new_itemex_opt(yyvsp[0].num, 0, 0, 0);
		yyval.exopt->gotid = TRUE;
		;}
    break;

  case 177:
#line 1390 "parser.y"
    {
		yyval.exopt = new_itemex_opt(yyvsp[-2].iptr ? *(yyvsp[-2].iptr) : 0, yyvsp[0].num, 0, 0);
		if(yyvsp[-2].iptr) free(yyvsp[-2].iptr);
		yyval.exopt->gotid = TRUE;
		yyval.exopt->gottype = TRUE;
		;}
    break;

  case 178:
#line 1396 "parser.y"
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
#line 1404 "parser.y"
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
#line 1424 "parser.y"
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
#line 1465 "parser.y"
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
#line 1476 "parser.y"
    { yyval.stt = NULL; ;}
    break;

  case 183:
#line 1477 "parser.y"
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
#line 1517 "parser.y"
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
#line 1533 "parser.y"
    { yyval.veri = new_versioninfo(); ;}
    break;

  case 188:
#line 1534 "parser.y"
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
#line 1544 "parser.y"
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
#line 1554 "parser.y"
    {
		if(yyvsp[-2].veri->gotit.ff)
			yyerror("FILEFLAGS already defined");
		yyval.veri = yyvsp[-2].veri;
		yyval.veri->fileflags = yyvsp[0].num;
		yyval.veri->gotit.ff = 1;
		;}
    break;

  case 191:
#line 1561 "parser.y"
    {
		if(yyvsp[-2].veri->gotit.ffm)
			yyerror("FILEFLAGSMASK already defined");
		yyval.veri = yyvsp[-2].veri;
		yyval.veri->fileflagsmask = yyvsp[0].num;
		yyval.veri->gotit.ffm = 1;
		;}
    break;

  case 192:
#line 1568 "parser.y"
    {
		if(yyvsp[-2].veri->gotit.fo)
			yyerror("FILEOS already defined");
		yyval.veri = yyvsp[-2].veri;
		yyval.veri->fileos = yyvsp[0].num;
		yyval.veri->gotit.fo = 1;
		;}
    break;

  case 193:
#line 1575 "parser.y"
    {
		if(yyvsp[-2].veri->gotit.ft)
			yyerror("FILETYPE already defined");
		yyval.veri = yyvsp[-2].veri;
		yyval.veri->filetype = yyvsp[0].num;
		yyval.veri->gotit.ft = 1;
		;}
    break;

  case 194:
#line 1582 "parser.y"
    {
		if(yyvsp[-2].veri->gotit.fst)
			yyerror("FILESUBTYPE already defined");
		yyval.veri = yyvsp[-2].veri;
		yyval.veri->filesubtype = yyvsp[0].num;
		yyval.veri->gotit.fst = 1;
		;}
    break;

  case 195:
#line 1592 "parser.y"
    { yyval.blk = NULL; ;}
    break;

  case 196:
#line 1593 "parser.y"
    {
		yyval.blk = yyvsp[0].blk;
		yyval.blk->prev = yyvsp[-1].blk;
		if(yyvsp[-1].blk)
			yyvsp[-1].blk->next = yyval.blk;
		;}
    break;

  case 197:
#line 1602 "parser.y"
    {
		yyval.blk = new_ver_block();
		yyval.blk->name = yyvsp[-3].str;
		yyval.blk->values = get_ver_value_head(yyvsp[-1].val);
		;}
    break;

  case 198:
#line 1610 "parser.y"
    { yyval.val = NULL; ;}
    break;

  case 199:
#line 1611 "parser.y"
    {
		yyval.val = yyvsp[0].val;
		yyval.val->prev = yyvsp[-1].val;
		if(yyvsp[-1].val)
			yyvsp[-1].val->next = yyval.val;
		;}
    break;

  case 200:
#line 1620 "parser.y"
    {
		yyval.val = new_ver_value();
		yyval.val->type = val_block;
		yyval.val->value.block = yyvsp[0].blk;
		;}
    break;

  case 201:
#line 1625 "parser.y"
    {
		yyval.val = new_ver_value();
		yyval.val->type = val_str;
		yyval.val->key = yyvsp[-2].str;
		yyval.val->value.str = yyvsp[0].str;
		;}
    break;

  case 202:
#line 1631 "parser.y"
    {
		yyval.val = new_ver_value();
		yyval.val->type = val_words;
		yyval.val->key = yyvsp[-2].str;
		yyval.val->value.words = yyvsp[0].verw;
		;}
    break;

  case 203:
#line 1640 "parser.y"
    { yyval.verw = new_ver_words(yyvsp[0].num); ;}
    break;

  case 204:
#line 1641 "parser.y"
    { yyval.verw = add_ver_words(yyvsp[-2].verw, yyvsp[0].num); ;}
    break;

  case 205:
#line 1645 "parser.y"
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
#line 1671 "parser.y"
    { yyval.tlbarItems = NULL; ;}
    break;

  case 207:
#line 1672 "parser.y"
    {
		toolbar_item_t *idrec = new_toolbar_item();
		idrec->id = yyvsp[0].num;
		yyval.tlbarItems = ins_tlbr_button(yyvsp[-2].tlbarItems, idrec);
		;}
    break;

  case 208:
#line 1677 "parser.y"
    {
		toolbar_item_t *idrec = new_toolbar_item();
		idrec->id = 0;
		yyval.tlbarItems = ins_tlbr_button(yyvsp[-1].tlbarItems, idrec);
	;}
    break;

  case 209:
#line 1686 "parser.y"
    { yyval.iptr = NULL; ;}
    break;

  case 210:
#line 1687 "parser.y"
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
#line 1697 "parser.y"
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
#line 1712 "parser.y"
    { yyval.iptr = new_int(WRC_MO_PRELOAD); ;}
    break;

  case 213:
#line 1713 "parser.y"
    { yyval.iptr = new_int(WRC_MO_MOVEABLE); ;}
    break;

  case 214:
#line 1714 "parser.y"
    { yyval.iptr = new_int(WRC_MO_DISCARDABLE); ;}
    break;

  case 215:
#line 1715 "parser.y"
    { yyval.iptr = new_int(WRC_MO_PURE); ;}
    break;

  case 216:
#line 1718 "parser.y"
    { yyval.iptr = new_int(~WRC_MO_PRELOAD); ;}
    break;

  case 217:
#line 1719 "parser.y"
    { yyval.iptr = new_int(~WRC_MO_MOVEABLE); ;}
    break;

  case 218:
#line 1720 "parser.y"
    { yyval.iptr = new_int(~WRC_MO_PURE); ;}
    break;

  case 219:
#line 1724 "parser.y"
    { yyval.lvc = new_lvc(); ;}
    break;

  case 220:
#line 1725 "parser.y"
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
#line 1733 "parser.y"
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
#line 1741 "parser.y"
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
#line 1759 "parser.y"
    { yyval.lan = new_language(yyvsp[-2].num, yyvsp[0].num);
					  if (get_language_codepage(yyvsp[-2].num, yyvsp[0].num) == -1)
						yyerror( "Language %04x is not supported", (yyvsp[0].num<<10) + yyvsp[-2].num);
					;}
    break;

  case 224:
#line 1766 "parser.y"
    { yyval.chars = new_characts(yyvsp[0].num); ;}
    break;

  case 225:
#line 1770 "parser.y"
    { yyval.ver = new_version(yyvsp[0].num); ;}
    break;

  case 226:
#line 1774 "parser.y"
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
#line 1789 "parser.y"
    { yyval.raw = yyvsp[0].raw; ;}
    break;

  case 228:
#line 1790 "parser.y"
    { yyval.raw = int2raw_data(yyvsp[0].num); ;}
    break;

  case 229:
#line 1791 "parser.y"
    { yyval.raw = int2raw_data(-(yyvsp[0].num)); ;}
    break;

  case 230:
#line 1792 "parser.y"
    { yyval.raw = long2raw_data(yyvsp[0].num); ;}
    break;

  case 231:
#line 1793 "parser.y"
    { yyval.raw = long2raw_data(-(yyvsp[0].num)); ;}
    break;

  case 232:
#line 1794 "parser.y"
    { yyval.raw = str2raw_data(yyvsp[0].str); ;}
    break;

  case 233:
#line 1795 "parser.y"
    { yyval.raw = merge_raw_data(yyvsp[-2].raw, yyvsp[0].raw); free(yyvsp[0].raw->data); free(yyvsp[0].raw); ;}
    break;

  case 234:
#line 1796 "parser.y"
    { yyval.raw = merge_raw_data_int(yyvsp[-2].raw, yyvsp[0].num); ;}
    break;

  case 235:
#line 1797 "parser.y"
    { yyval.raw = merge_raw_data_int(yyvsp[-3].raw, -(yyvsp[0].num)); ;}
    break;

  case 236:
#line 1798 "parser.y"
    { yyval.raw = merge_raw_data_long(yyvsp[-2].raw, yyvsp[0].num); ;}
    break;

  case 237:
#line 1799 "parser.y"
    { yyval.raw = merge_raw_data_long(yyvsp[-3].raw, -(yyvsp[0].num)); ;}
    break;

  case 238:
#line 1800 "parser.y"
    { yyval.raw = merge_raw_data_str(yyvsp[-2].raw, yyvsp[0].str); ;}
    break;

  case 239:
#line 1804 "parser.y"
    { yyval.raw = load_file(yyvsp[0].str,dup_language(currentlanguage)); ;}
    break;

  case 240:
#line 1805 "parser.y"
    { yyval.raw = yyvsp[0].raw; ;}
    break;

  case 241:
#line 1812 "parser.y"
    { yyval.iptr = 0; ;}
    break;

  case 242:
#line 1813 "parser.y"
    { yyval.iptr = new_int(yyvsp[0].num); ;}
    break;

  case 243:
#line 1817 "parser.y"
    { yyval.num = (yyvsp[0].num); ;}
    break;

  case 244:
#line 1820 "parser.y"
    { yyval.num = (yyvsp[-2].num) + (yyvsp[0].num); ;}
    break;

  case 245:
#line 1821 "parser.y"
    { yyval.num = (yyvsp[-2].num) - (yyvsp[0].num); ;}
    break;

  case 246:
#line 1822 "parser.y"
    { yyval.num = (yyvsp[-2].num) | (yyvsp[0].num); ;}
    break;

  case 247:
#line 1823 "parser.y"
    { yyval.num = (yyvsp[-2].num) & (yyvsp[0].num); ;}
    break;

  case 248:
#line 1824 "parser.y"
    { yyval.num = (yyvsp[-2].num) * (yyvsp[0].num); ;}
    break;

  case 249:
#line 1825 "parser.y"
    { yyval.num = (yyvsp[-2].num) / (yyvsp[0].num); ;}
    break;

  case 250:
#line 1826 "parser.y"
    { yyval.num = (yyvsp[-2].num) ^ (yyvsp[0].num); ;}
    break;

  case 251:
#line 1827 "parser.y"
    { yyval.num = ~(yyvsp[0].num); ;}
    break;

  case 252:
#line 1828 "parser.y"
    { yyval.num = -(yyvsp[0].num); ;}
    break;

  case 253:
#line 1829 "parser.y"
    { yyval.num = yyvsp[0].num; ;}
    break;

  case 254:
#line 1830 "parser.y"
    { yyval.num = yyvsp[-1].num; ;}
    break;

  case 255:
#line 1831 "parser.y"
    { yyval.num = yyvsp[0].num; ;}
    break;

  case 256:
#line 1832 "parser.y"
    { yyval.num = ~(yyvsp[0].num); ;}
    break;

  case 257:
#line 1835 "parser.y"
    { yyval.num = yyvsp[0].num; ;}
    break;

  case 258:
#line 1836 "parser.y"
    { yyval.num = yyvsp[0].num; ;}
    break;


    }

/* Line 999 of yacc.c.  */
#line 4129 "y.tab.c"

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

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
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
      yyvsp--;
      yystate = *--yyssp;

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


#line 1839 "parser.y"

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

