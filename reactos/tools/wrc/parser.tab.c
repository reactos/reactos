
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         parser_parse
#define yylex           parser_lex
#define yyerror         parser_error
#define yylval          parser_lval
#define yychar          parser_char
#define yydebug         parser_debug
#define yynerrs         parser_nerrs


/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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
static stringtable_t *tagstt;	/* Stringtable tag.
			 * It is set while parsing a stringtable to one of
			 * the stringtables in the sttres list or a new one
			 * if the language was not parsed before.
			 */
static stringtable_t *sttres;	/* Stringtable resources. This holds the list of
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



/* Line 189 of yacc.c  */
#line 320 "parser.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


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



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 238 "parser.y"

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



/* Line 214 of yacc.c  */
#line 487 "parser.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 499 "parser.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   677

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  97
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  83
/* YYNRULES -- Number of rules.  */
#define YYNRULES  258
/* YYNRULES -- Number of states.  */
#define YYNSTATES  572

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   340

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
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
static const yytype_uint16 yyprhs[] =
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
     974,   978,   981,   984,   987,   991,   993,   996,   998
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
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
      -1,   104,   156,   177,    94,   177,    94,   177,    94,   177,
      94,   177,   131,    -1,   177,    94,   177,    94,   177,    94,
     177,    94,   177,   131,    -1,    -1,    94,   177,    94,   177,
      -1,    94,   177,    94,   177,    94,   132,    -1,    94,   177,
      94,   177,    94,   132,    94,   132,    -1,   104,   156,   177,
      94,   133,    94,   132,    94,   177,    94,   177,    94,   177,
      94,   177,    94,   132,    -1,   104,   156,   177,    94,   133,
      94,   132,    94,   177,    94,   177,    94,   177,    94,   177,
      -1,    21,   177,    94,     6,    -1,    -1,    94,   132,    -1,
      94,   132,    94,   132,    -1,   132,    84,   132,    -1,    95,
     132,    96,    -1,   179,    -1,    92,   179,    -1,   177,    -1,
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
     104,   156,   177,    94,   177,    94,   177,    94,   177,    94,
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
      -1,   106,    -1,   173,    -1,    -1,   177,    -1,   178,    -1,
     178,    87,   178,    -1,   178,    88,   178,    -1,   178,    84,
     178,    -1,   178,    86,   178,    -1,   178,    89,   178,    -1,
     178,    90,   178,    -1,   178,    85,   178,    -1,    91,   178,
      -1,    88,   178,    -1,    87,   178,    -1,    95,   178,    96,
      -1,   179,    -1,    92,   179,    -1,     4,    -1,     5,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   360,   360,   406,   407,   478,   484,   496,   506,   514,
     514,   557,   563,   570,   580,   581,   590,   591,   592,   616,
     617,   623,   624,   625,   626,   650,   651,   657,   658,   659,
     660,   661,   662,   666,   667,   668,   672,   676,   692,   714,
     724,   732,   740,   744,   748,   752,   763,   768,   777,   801,
     802,   803,   812,   813,   816,   817,   820,   821,   822,   823,
     824,   825,   830,   865,   866,   867,   868,   869,   870,   871,
     872,   873,   874,   877,   878,   879,   880,   881,   882,   883,
     884,   885,   886,   888,   889,   890,   891,   892,   893,   894,
     895,   897,   907,   930,   952,   954,   959,   966,   977,   991,
    1006,  1011,  1012,  1013,  1017,  1018,  1019,  1020,  1024,  1029,
    1037,  1081,  1082,  1083,  1084,  1085,  1086,  1087,  1088,  1089,
    1090,  1091,  1094,  1095,  1096,  1097,  1098,  1099,  1100,  1101,
    1102,  1103,  1105,  1106,  1107,  1108,  1109,  1110,  1111,  1112,
    1114,  1124,  1149,  1165,  1191,  1214,  1215,  1218,  1219,  1223,
    1230,  1231,  1235,  1258,  1262,  1263,  1272,  1278,  1297,  1298,
    1299,  1300,  1301,  1302,  1303,  1307,  1332,  1336,  1337,  1353,
    1359,  1379,  1380,  1384,  1392,  1403,  1404,  1408,  1414,  1422,
    1442,  1480,  1490,  1491,  1524,  1526,  1531,  1547,  1548,  1558,
    1568,  1575,  1582,  1589,  1596,  1606,  1607,  1616,  1624,  1625,
    1634,  1639,  1645,  1654,  1655,  1659,  1685,  1686,  1691,  1700,
    1701,  1711,  1726,  1727,  1728,  1729,  1732,  1733,  1734,  1738,
    1739,  1747,  1755,  1773,  1780,  1784,  1788,  1803,  1804,  1805,
    1806,  1807,  1808,  1809,  1810,  1811,  1812,  1813,  1814,  1818,
    1819,  1826,  1827,  1831,  1834,  1835,  1836,  1837,  1838,  1839,
    1840,  1841,  1842,  1843,  1844,  1845,  1846,  1849,  1850
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
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
  "'('", "')'", "$accept", "resource_file", "resources", "resource", "$@1",
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
  "raw_elements", "file_raw", "e_expr", "expr", "xpr", "any_num", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
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
static const yytype_uint8 yyr1[] =
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
     178,   178,   178,   178,   178,   178,   178,   179,   179
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
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
       3,     2,     2,     2,     3,     1,     2,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       3,     0,     2,     1,     5,   257,   258,    11,   209,     9,
       0,     0,     0,     0,     0,     4,     8,     0,    11,   243,
     255,     0,   219,     0,   253,   252,   251,   256,     0,   182,
       0,     0,     0,     0,     0,     0,     0,     0,    46,    47,
     209,   209,   209,   209,   209,   209,   209,   209,   209,   209,
     209,   209,   209,   209,   209,   209,     7,    17,    18,    24,
      22,    23,    27,    28,    29,    21,    31,   209,    16,    19,
      20,    25,    26,    32,    30,   215,   218,   214,   216,   212,
     217,   213,   210,   211,   181,     0,   254,     0,     6,   246,
     250,   247,   244,   245,   248,   249,   219,   219,   219,     0,
       0,   219,   219,   219,   219,   187,   219,   219,   219,   219,
       0,   219,   219,     0,     0,     0,   220,   221,   222,     0,
     180,   184,     0,    35,    34,    33,   239,     0,   240,    36,
      37,     0,     0,     0,     0,    41,    43,     0,    39,    40,
      38,    42,     0,    44,    45,   224,   225,     0,    10,   185,
       0,    49,     0,     0,     0,   154,   152,   167,   165,     0,
       0,     0,     0,     0,     0,     0,   195,     0,     0,   183,
       0,   228,   230,   232,   227,     0,   184,     0,     0,     0,
       0,     0,     0,   191,   192,   193,   190,   194,     0,   219,
     223,     0,    48,     0,   229,   231,   226,     0,     0,     0,
       0,     0,   153,     0,     0,   166,     0,     0,     0,   186,
     196,     0,     0,     0,   234,   236,   238,   233,     0,     0,
       0,   184,   156,   184,   171,   169,   175,     0,     0,     0,
     206,    52,    52,   235,   237,     0,     0,     0,     0,     0,
     241,   168,   241,     0,     0,     0,   198,     0,     0,    50,
      51,    63,   147,   184,   157,   184,   184,   184,   184,   184,
     184,     0,   172,     0,   176,   170,     0,     0,     0,   208,
       0,   205,    58,    57,    59,    60,    61,    56,    53,    54,
       0,     0,   111,   155,   160,   159,   162,   163,   164,   161,
     241,   241,     0,     0,     0,   197,   200,   199,   207,     0,
       0,     0,     0,     0,     0,     0,    73,    67,    70,    71,
      72,   148,     0,   184,   242,     0,   177,   188,   189,     0,
      55,    13,    69,    12,     0,    15,    14,    68,    66,     0,
       0,    65,   106,    64,     0,     0,     0,     0,     0,     0,
       0,   122,   115,   116,   119,   120,   121,   185,   173,   241,
       0,     0,   107,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    62,   118,     0,   117,   114,   113,   112,
       0,   174,     0,   178,   201,   202,   203,   100,   105,   104,
     184,   184,    84,    86,    87,    79,    80,    82,    83,    85,
      81,    77,     0,    76,    78,   184,    74,    75,    90,    89,
      88,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     110,     0,     0,     0,     0,     0,     0,   100,   184,   184,
     133,   135,   136,   128,   129,   131,   132,   134,   130,   126,
       0,   125,   127,   184,   123,   124,   139,   138,   137,   179,
     204,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   109,     0,   108,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   150,     0,     0,     0,     0,    94,
       0,     0,     0,     0,   149,     0,     0,     0,     0,     0,
      91,     0,     0,     0,   151,    94,     0,     0,     0,     0,
       0,   101,     0,   140,     0,     0,     0,     0,     0,     0,
      93,     0,     0,   101,     0,    95,   101,   102,     0,     0,
     147,     0,     0,    92,     0,     0,   101,   145,     0,    96,
     103,     0,   147,   144,   146,     0,     0,     0,   145,     0,
      97,    99,   143,     0,     0,   145,    98,     0,   142,   147,
     145,   141
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     2,    15,    23,    21,   326,   391,    56,   126,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,   170,   249,   278,   279,    69,   280,   334,   392,
     401,   510,   406,   307,   530,   331,   483,    70,   312,   380,
     454,   440,   449,   553,   282,   343,   504,    71,   156,   179,
     238,    72,   158,   180,   241,   243,    16,    17,    87,   239,
      73,   137,   188,   210,   268,   297,   385,    74,   247,    22,
      82,    83,   127,   116,   117,   118,   128,   176,   129,   261,
     323,    19,    20
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -535
static const yytype_int16 yypact[] =
{
    -535,    31,    12,  -535,  -535,  -535,  -535,  -535,  -535,  -535,
     240,   240,   240,   182,   240,  -535,  -535,   -69,  -535,   577,
    -535,   365,   616,   240,  -535,  -535,  -535,  -535,   564,  -535,
     365,   240,   240,   240,   240,   240,   240,   240,  -535,  -535,
    -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,
    -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,
    -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,
    -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,
    -535,  -535,  -535,  -535,   362,   -60,  -535,   111,  -535,   392,
     329,   363,   131,   131,  -535,  -535,   616,   403,   403,    84,
      84,   616,   616,   403,   403,   616,   403,   403,   403,   403,
      84,   403,   403,   240,   240,   240,  -535,  -535,  -535,   240,
    -535,    -7,    -6,  -535,  -535,  -535,  -535,    83,  -535,  -535,
    -535,     0,     8,   202,   265,  -535,  -535,   449,  -535,  -535,
    -535,  -535,    15,  -535,  -535,  -535,  -535,    36,  -535,  -535,
      90,  -535,    18,   240,   240,  -535,  -535,  -535,  -535,   240,
     240,   240,   240,   240,   240,   240,  -535,   240,   240,  -535,
     213,  -535,  -535,  -535,  -535,   318,   -11,    51,    55,   -46,
     211,    88,    94,  -535,  -535,  -535,  -535,  -535,   -28,  -535,
    -535,   119,  -535,   134,  -535,  -535,  -535,    40,   240,   240,
       1,   108,  -535,     3,   174,  -535,   240,   240,   248,  -535,
    -535,   290,   240,   240,  -535,  -535,  -535,  -535,   339,   162,
     175,    -7,  -535,   -41,   188,  -535,   190,   200,   203,   144,
    -535,   258,   258,  -535,  -535,   240,   240,   240,   195,   364,
     240,  -535,   240,   199,   240,   240,  -535,   159,   501,  -535,
    -535,  -535,   267,   171,  -535,   133,   133,   133,   133,   133,
     133,   268,   272,   274,   272,  -535,   276,   291,   -32,  -535,
     240,  -535,  -535,  -535,  -535,  -535,  -535,  -535,   297,  -535,
       5,   240,  -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,
     240,   240,   240,   240,   419,  -535,  -535,  -535,  -535,   501,
     219,   240,    86,   422,    37,    37,  -535,  -535,  -535,  -535,
    -535,  -535,   277,   227,  -535,   341,   272,  -535,  -535,   342,
    -535,  -535,  -535,  -535,   345,  -535,  -535,  -535,  -535,   182,
      37,   212,  -535,   212,   367,   219,   240,    86,   439,    37,
      37,  -535,  -535,  -535,  -535,  -535,  -535,   240,  -535,   240,
     186,   440,  -535,    71,    37,    86,    86,    86,    86,    86,
      86,    86,    86,    86,    86,   240,   240,   240,    86,   240,
      86,    86,    86,  -535,  -535,   353,  -535,  -535,   212,   212,
     563,  -535,   360,   272,  -535,   368,  -535,  -535,  -535,  -535,
      -7,    -7,  -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,
    -535,  -535,   381,  -535,  -535,    -7,  -535,  -535,  -535,  -535,
    -535,   485,    86,    86,    86,    86,    86,    86,    86,    86,
      86,    86,   240,   240,   240,    86,   240,    86,    86,    86,
    -535,   240,   240,   240,   240,   240,   240,   398,    -7,    -7,
    -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,
     406,  -535,  -535,    -7,  -535,  -535,  -535,  -535,  -535,  -535,
    -535,   408,   412,   414,   418,   240,   240,   240,   240,   240,
     240,   240,   240,   225,   420,   433,   437,   438,   441,   442,
     446,   447,  -535,   448,  -535,   240,   240,   240,   240,   225,
     240,   240,   240,    37,   450,   456,   457,   460,   463,   464,
     465,   466,   -21,   240,  -535,   240,   240,   240,    37,   240,
    -535,   240,   240,   240,  -535,   464,   469,   479,    11,   480,
     481,   483,   487,  -535,   240,   240,   240,   240,   240,    37,
    -535,   240,   488,   483,   489,   491,   483,   100,   536,   240,
     267,   240,    37,  -535,    37,   240,   483,   292,   537,   101,
     212,   543,   267,  -535,  -535,   240,    37,   240,   292,   546,
     212,   561,  -535,   240,    37,    89,   212,    37,  -535,   138,
     292,  -535
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -535,  -535,  -535,  -535,  -535,   515,  -296,  -294,   504,  -535,
    -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,  -535,
    -535,  -535,  -535,   324,  -535,   330,  -535,  -535,  -535,   208,
    -118,   141,  -535,   346,  -390,  -292,   168,  -535,  -535,  -535,
    -535,   207,    17,  -486,  -534,  -535,  -535,  -535,   421,  -535,
       2,  -535,   432,  -535,  -535,  -535,  -535,  -535,  -535,  -120,
    -535,  -535,  -535,   409,  -535,  -535,  -535,  -535,  -535,   565,
    -535,  -535,   -20,  -275,  -255,  -252,  -358,  -535,   535,  -239,
      -2,   462,    20
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -243
static const yytype_int16 yytable[] =
{
      18,   150,    84,   263,   322,   308,   547,   221,   327,   224,
     208,   294,    29,   333,   208,     4,     5,     6,   558,     7,
     300,    85,   171,   172,   173,   309,   301,   174,   310,   200,
     201,     3,     8,    27,   119,   570,   202,   344,   353,   374,
    -158,     5,     6,   376,   214,   215,   216,   378,   379,   217,
     295,   313,   315,   149,   209,   113,   197,   345,   114,   115,
     346,   390,   389,   354,   302,   303,   113,   304,   305,   114,
     115,   196,   562,   513,   405,   151,   122,     9,   222,   568,
     225,   133,   134,   149,   571,   121,   306,   149,     5,     6,
       5,     6,   325,   321,   153,   354,   169,   131,   132,    10,
      11,   237,   154,    12,    13,   526,   175,    14,   142,   167,
     382,   145,   146,   147,   223,     5,     6,   148,   438,   439,
     439,   439,   439,   439,   439,   439,   439,   439,   218,   329,
     168,   453,   330,   439,   439,   439,    75,    76,    77,    78,
      79,    80,    81,   540,   113,   198,   543,   114,   115,   199,
    -219,   177,   178,  -219,  -219,   354,   552,   181,   182,   183,
     184,   185,   186,   187,   152,   189,   190,   388,   193,   211,
    -219,    10,    11,    10,    11,    12,    13,    12,    13,    14,
     226,    14,   206,   567,   354,   354,     5,     6,   207,   554,
       5,     6,   384,   120,   544,   556,   219,   220,    10,    11,
     554,   502,    12,    13,   227,   228,    14,   554,  -158,  -158,
     231,   232,   554,   212,  -158,  -158,   518,     5,     6,   191,
      36,    37,   354,     5,     6,   246,   321,   149,   213,     5,
       6,   482,   281,   251,   252,   253,   269,   537,   262,   270,
     264,   271,   266,   267,     5,     6,  -158,  -158,   403,   404,
     549,   407,   550,  -158,   229,   283,   235,   284,   285,   286,
     287,   288,   289,   113,   560,   149,   114,   115,   298,   236,
     433,   434,   566,    10,    11,   569,   155,    12,    13,   311,
     157,    14,   240,   155,   242,   436,   203,   204,   314,   316,
     317,   318,   335,   205,   244,   192,   354,   245,   336,   324,
      10,    11,  -158,  -158,    12,    13,    10,    11,    14,  -158,
      12,    13,    10,    11,    14,   348,    12,    13,   466,   467,
      14,   347,   194,   195,   332,   332,   113,    10,    11,   114,
     115,    12,    13,   469,   375,    14,   337,   338,   113,   339,
     340,   114,   115,   233,   234,   381,   157,   383,   386,   352,
     332,   113,   248,  -219,   114,   115,  -219,  -219,   341,   332,
     332,   281,   290,   402,   402,   402,  -242,   402,   291,    38,
     292,   230,    39,  -219,   332,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,   293,    50,    51,    52,    53,
     355,   299,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,   369,   370,   371,   372,   123,
     124,   125,   255,   256,   257,    33,    34,    35,    36,    37,
     450,   450,   450,   113,   450,   319,   114,   115,   328,   459,
     460,   461,   462,   463,   464,   349,   350,   258,   259,   351,
     451,   452,   260,   455,    54,   377,   387,   411,    55,   373,
      34,    35,    36,    37,   431,    75,    76,    77,    78,    79,
      80,    81,   432,   474,   475,   476,   477,   478,   479,   480,
     481,   484,    24,    25,    26,   435,    28,    32,    33,    34,
      35,    36,    37,   494,   495,   496,   497,   484,   499,   500,
     501,   437,   465,    89,    90,    91,    92,    93,    94,    95,
     468,   514,   470,   515,   516,   517,   471,   519,   472,   520,
     521,   522,   473,   332,   485,   159,   160,   161,   162,   163,
     164,   165,   532,   533,   534,   535,   536,   486,   332,   538,
     166,   487,   488,    30,    88,   489,   490,   546,   272,   548,
     491,   492,   493,   551,   503,   273,   274,   275,   276,   332,
     505,   506,   277,   559,   507,   561,   250,   508,   509,   511,
     512,   565,   332,   524,   332,   393,   394,   395,   396,   397,
     398,   399,   400,   525,   527,   528,   332,   529,   408,   409,
     410,   531,   539,   541,   332,   542,   412,   332,   413,   414,
     415,   416,   417,   418,   419,   420,   421,   422,   423,   424,
     425,   426,   427,   428,   429,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   441,   442,   443,   444,   445,   446,   447,   448,   320,
     545,   555,   112,   130,   456,   457,   458,   557,   135,   136,
     563,   138,   139,   140,   141,   430,   143,   144,    31,    32,
      33,    34,    35,    36,    37,   564,   523,   498,   342,   254,
      86,    31,    32,    33,    34,    35,    36,    37,    75,    76,
      77,    78,    79,    80,    81,   265,     0,   296
};

static const yytype_int16 yycheck[] =
{
       2,   121,    22,   242,   300,   280,   540,     6,   302,     6,
      42,    43,    81,   305,    42,     3,     4,     5,   552,     7,
      15,    23,     4,     5,     6,   280,    21,     9,   280,    75,
      76,     0,    20,    13,    94,   569,    82,   312,   330,   335,
      81,     4,     5,   337,     4,     5,     6,   339,   340,     9,
      82,   290,   291,    94,    82,    61,   176,   312,    64,    65,
     312,   355,   354,    84,    59,    60,    61,    62,    63,    64,
      65,    82,   558,    94,   368,    81,    96,    65,    77,   565,
      77,   101,   102,    94,   570,    87,    81,    94,     4,     5,
       4,     5,     6,     7,    94,    84,     6,    99,   100,    87,
      88,   221,    94,    91,    92,    94,    88,    95,   110,    94,
     349,   113,   114,   115,     6,     4,     5,   119,   412,   413,
     414,   415,   416,   417,   418,   419,   420,   421,    88,    92,
      94,   425,    95,   427,   428,   429,    52,    53,    54,    55,
      56,    57,    58,   533,    61,    94,   536,    64,    65,    94,
      61,   153,   154,    64,    65,    84,   546,   159,   160,   161,
     162,   163,   164,   165,    81,   167,   168,    96,   170,   189,
      81,    87,    88,    87,    88,    91,    92,    91,    92,    95,
       6,    95,    94,    94,    84,    84,     4,     5,    94,   547,
       4,     5,     6,    82,    94,    94,   198,   199,    87,    88,
     558,   493,    91,    92,   206,   207,    95,   565,    75,    76,
     212,   213,   570,    94,    81,    82,   508,     4,     5,     6,
      89,    90,    84,     4,     5,    81,     7,    94,    94,     4,
       5,     6,    94,   235,   236,   237,    77,   529,   240,    80,
     242,    82,   244,   245,     4,     5,    75,    76,   366,   367,
     542,   369,   544,    82,     6,   253,    94,   255,   256,   257,
     258,   259,   260,    61,   556,    94,    64,    65,   270,    94,
     390,   391,   564,    87,    88,   567,    81,    91,    92,   281,
      81,    95,    94,    81,    94,   405,    75,    76,   290,   291,
     292,   293,    15,    82,    94,    82,    84,    94,    21,   301,
      87,    88,    75,    76,    91,    92,    87,    88,    95,    82,
      91,    92,    87,    88,    95,   313,    91,    92,   438,   439,
      95,    94,     4,     5,   304,   305,    61,    87,    88,    64,
      65,    91,    92,   453,   336,    95,    59,    60,    61,    62,
      63,    64,    65,     4,     5,   347,    81,   349,   350,   329,
     330,    61,    94,    61,    64,    65,    64,    65,    81,   339,
     340,    94,    94,   365,   366,   367,    94,   369,    94,     4,
      94,    81,     7,    81,   354,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    94,    21,    22,    23,    24,
      23,    94,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,     6,
       7,     8,    48,    49,    50,    86,    87,    88,    89,    90,
     422,   423,   424,    61,   426,     6,    64,    65,     6,   431,
     432,   433,   434,   435,   436,    94,    94,    73,    74,    94,
     423,   424,    78,   426,    79,     6,     6,    94,    83,    82,
      87,    88,    89,    90,    94,    52,    53,    54,    55,    56,
      57,    58,    94,   465,   466,   467,   468,   469,   470,   471,
     472,   473,    10,    11,    12,    94,    14,    85,    86,    87,
      88,    89,    90,   485,   486,   487,   488,   489,   490,   491,
     492,     6,    94,    31,    32,    33,    34,    35,    36,    37,
      94,   503,    94,   505,   506,   507,    94,   509,    94,   511,
     512,   513,    94,   493,    94,    66,    67,    68,    69,    70,
      71,    72,   524,   525,   526,   527,   528,    94,   508,   531,
      81,    94,    94,    18,    30,    94,    94,   539,    37,   541,
      94,    94,    94,   545,    94,    44,    45,    46,    47,   529,
      94,    94,    51,   555,    94,   557,   232,    94,    94,    94,
      94,   563,   542,    94,   544,   357,   358,   359,   360,   361,
     362,   363,   364,    94,    94,    94,   556,    94,   370,   371,
     372,    94,    94,    94,   564,    94,    23,   567,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,   414,   415,   416,   417,   418,   419,   420,   421,   299,
      94,    94,    67,    98,   427,   428,   429,    94,   103,   104,
      94,   106,   107,   108,   109,    82,   111,   112,    84,    85,
      86,    87,    88,    89,    90,    94,   515,   489,   312,   238,
      96,    84,    85,    86,    87,    88,    89,    90,    52,    53,
      54,    55,    56,    57,    58,   243,    -1,   268
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    98,    99,     0,     3,     4,     5,     7,    20,    65,
      87,    88,    91,    92,    95,   100,   153,   154,   177,   178,
     179,   102,   166,   101,   178,   178,   178,   179,   178,    81,
     102,    84,    85,    86,    87,    88,    89,    90,     4,     7,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      21,    22,    23,    24,    79,    83,   105,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   123,
     134,   144,   148,   157,   164,    52,    53,    54,    55,    56,
      57,    58,   167,   168,   169,   177,    96,   155,   105,   178,
     178,   178,   178,   178,   178,   178,   166,   166,   166,   166,
     166,   166,   166,   166,   166,   166,   166,   166,   166,   166,
     166,   166,   166,    61,    64,    65,   170,   171,   172,    94,
      82,   177,   169,     6,     7,     8,   106,   169,   173,   175,
     175,   177,   177,   169,   169,   175,   175,   158,   175,   175,
     175,   175,   177,   175,   175,   177,   177,   177,   177,    94,
     156,    81,    81,    94,    94,    81,   145,    81,   149,    66,
      67,    68,    69,    70,    71,    72,    81,    94,    94,     6,
     119,     4,     5,     6,     9,    88,   174,   177,   177,   146,
     150,   177,   177,   177,   177,   177,   177,   177,   159,   177,
     177,     6,    82,   177,     4,     5,    82,   156,    94,    94,
      75,    76,    82,    75,    76,    82,    94,    94,    42,    82,
     160,   169,    94,    94,     4,     5,     6,     9,    88,   177,
     177,     6,    77,     6,     6,    77,     6,   177,   177,     6,
      81,   177,   177,     4,     5,    94,    94,   156,   147,   156,
      94,   151,    94,   152,    94,    94,    81,   165,    94,   120,
     120,   177,   177,   177,   145,    48,    49,    50,    73,    74,
      78,   176,   177,   176,   177,   149,   177,   177,   161,    77,
      80,    82,    37,    44,    45,    46,    47,    51,   121,   122,
     124,    94,   141,   147,   147,   147,   147,   147,   147,   147,
      94,    94,    94,    94,    43,    82,   160,   162,   177,    94,
      15,    21,    59,    60,    62,    63,    81,   130,   170,   171,
     172,   177,   135,   176,   177,   176,   177,   177,   177,     6,
     122,     7,   103,   177,   177,     6,   103,   104,     6,    92,
      95,   132,   179,   132,   125,    15,    21,    59,    60,    62,
      63,    81,   130,   142,   170,   171,   172,    94,   147,    94,
      94,    94,   179,   132,    84,    23,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    82,   103,   177,   104,     6,   132,   132,
     136,   177,   176,   177,     6,   163,   177,     6,    96,   132,
     104,   104,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   127,   177,   127,   127,   104,   129,   127,   126,   126,
     126,    94,    23,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      82,    94,    94,   156,   156,    94,   156,     6,   104,   104,
     138,   138,   138,   138,   138,   138,   138,   138,   138,   139,
     177,   139,   139,   104,   137,   139,   138,   138,   138,   177,
     177,   177,   177,   177,   177,    94,   156,   156,    94,   156,
      94,    94,    94,    94,   177,   177,   177,   177,   177,   177,
     177,   177,     6,   133,   177,    94,    94,    94,    94,    94,
      94,    94,    94,    94,   177,   177,   177,   177,   133,   177,
     177,   177,   132,    94,   143,    94,    94,    94,    94,    94,
     128,    94,    94,    94,   177,   177,   177,   177,   132,   177,
     177,   177,   177,   128,    94,    94,    94,    94,    94,    94,
     131,    94,   177,   177,   177,   177,   177,   132,   177,    94,
     131,    94,    94,   131,    94,    94,   177,   141,   177,   132,
     132,   177,   131,   140,   173,    94,    94,    94,   141,   177,
     132,   177,   140,    94,    94,   177,   132,    94,   140,   132,
     141,   140
};

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
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
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
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
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
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
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



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

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
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
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

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

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
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
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

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

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

/* Line 1455 of yacc.c  */
#line 360 "parser.y"
    {
		resource_t *rsc, *head;
		/* First add stringtables to the resource-list */
		rsc = build_stt_resources(sttres);
		/* 'build_stt_resources' returns a head and $1 is a tail */
		if((yyvsp[(1) - (1)].res))
		{
			(yyvsp[(1) - (1)].res)->next = rsc;
			if(rsc)
				rsc->prev = (yyvsp[(1) - (1)].res);
		}
		else
			(yyvsp[(1) - (1)].res) = rsc;
		/* Find the tail again */
		while((yyvsp[(1) - (1)].res) && (yyvsp[(1) - (1)].res)->next)
			(yyvsp[(1) - (1)].res) = (yyvsp[(1) - (1)].res)->next;
		/* Now add any fontdirecory */
		rsc = build_fontdirs((yyvsp[(1) - (1)].res));
		/* 'build_fontdir' returns a head and $1 is a tail */
		if((yyvsp[(1) - (1)].res))
		{
			(yyvsp[(1) - (1)].res)->next = rsc;
			if(rsc)
				rsc->prev = (yyvsp[(1) - (1)].res);
		}
		else
			(yyvsp[(1) - (1)].res) = rsc;

		/* Final statements before were done */
                if ((head = get_resource_head((yyvsp[(1) - (1)].res))) != NULL)
                {
                    if (resource_top)  /* append to existing resources */
                    {
                        resource_t *tail = resource_top;
                        while (tail->next) tail = tail->next;
                        tail->next = head;
                        head->prev = tail;
                    }
                    else resource_top = head;
                }
                sttres = NULL;
		;}
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 406 "parser.y"
    { (yyval.res) = NULL; want_id = 1; ;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 407 "parser.y"
    {
		if((yyvsp[(2) - (2)].res))
		{
			resource_t *tail = (yyvsp[(2) - (2)].res);
			resource_t *head = (yyvsp[(2) - (2)].res);
			while(tail->next)
				tail = tail->next;
			while(head->prev)
				head = head->prev;
			head->prev = (yyvsp[(1) - (2)].res);
			if((yyvsp[(1) - (2)].res))
				(yyvsp[(1) - (2)].res)->next = head;
			(yyval.res) = tail;
			/* Check for duplicate identifiers */
			while((yyvsp[(1) - (2)].res) && head)
			{
				resource_t *rsc = (yyvsp[(1) - (2)].res);
				while(rsc)
				{
					if(rsc->type == head->type
					&& rsc->lan->id == head->lan->id
					&& rsc->lan->sub == head->lan->sub
					&& !compare_name_id(rsc->name, head->name)
					&& (rsc->type != res_usr || !compare_name_id(rsc->res.usr->type,head->res.usr->type)))
					{
						yyerror("Duplicate resource name '%s'", get_nameid_str(rsc->name));
					}
					rsc = rsc->prev;
				}
				head = head->next;
			}
		}
		else if((yyvsp[(1) - (2)].res))
		{
			resource_t *tail = (yyvsp[(1) - (2)].res);
			while(tail->next)
				tail = tail->next;
			(yyval.res) = tail;
		}
		else
			(yyval.res) = NULL;

		if(!dont_want_id)	/* See comments in language parsing below */
			want_id = 1;
		dont_want_id = 0;
		;}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 484 "parser.y"
    {
		(yyval.res) = (yyvsp[(3) - (3)].res);
		if((yyval.res))
		{
			if((yyvsp[(1) - (3)].num) > 65535 || (yyvsp[(1) - (3)].num) < -32768)
				yyerror("Resource's ID out of range (%d)", (yyvsp[(1) - (3)].num));
			(yyval.res)->name = new_name_id();
			(yyval.res)->name->type = name_ord;
			(yyval.res)->name->name.i_name = (yyvsp[(1) - (3)].num);
			chat("Got %s (%d)\n", get_typename((yyvsp[(3) - (3)].res)), (yyval.res)->name->name.i_name);
			}
			;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 496 "parser.y"
    {
		(yyval.res) = (yyvsp[(3) - (3)].res);
		if((yyval.res))
		{
			(yyval.res)->name = new_name_id();
			(yyval.res)->name->type = name_str;
			(yyval.res)->name->name.s_name = (yyvsp[(1) - (3)].str);
			chat("Got %s (%s)\n", get_typename((yyvsp[(3) - (3)].res)), (yyval.res)->name->name.s_name->str.cstr);
		}
		;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 506 "parser.y"
    {
		/* Don't do anything, stringtables are converted to
		 * resource_t structures when we are finished parsing and
		 * the final rule of the parser is reduced (see above)
		 */
		(yyval.res) = NULL;
		chat("Got STRINGTABLE\n");
		;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 514 "parser.y"
    {want_nl = 1; ;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 514 "parser.y"
    {
		/* We *NEED* the newline to delimit the expression.
		 * Otherwise, we would not be able to set the next
		 * want_id anymore because of the token-lookahead.
		 *
		 * However, we can test the lookahead-token for
		 * being "non-expression" type, in which case we
		 * continue. Fortunately, tNL is the only token that
		 * will break expression parsing and is implicitly
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
			parser_warning("LANGUAGE statement not delimited with newline; next identifier might be wrong\n");

		want_nl = 0;	/* We don't want it anymore if we didn't get it */

		if(!win32)
			parser_warning("LANGUAGE not supported in 16-bit mode\n");
		free(currentlanguage);
		if (get_language_codepage((yyvsp[(3) - (5)].num), (yyvsp[(5) - (5)].num)) == -1)
			yyerror( "Language %04x is not supported", ((yyvsp[(5) - (5)].num)<<10) + (yyvsp[(3) - (5)].num));
		currentlanguage = new_language((yyvsp[(3) - (5)].num), (yyvsp[(5) - (5)].num));
		(yyval.res) = NULL;
		chat("Got LANGUAGE %d,%d (0x%04x)\n", (yyvsp[(3) - (5)].num), (yyvsp[(5) - (5)].num), ((yyvsp[(5) - (5)].num)<<10) + (yyvsp[(3) - (5)].num));
		;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 557 "parser.y"
    { yychar = rsrcid_to_token(yychar); ;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 563 "parser.y"
    {
		if((yyvsp[(1) - (1)].num) > 65535 || (yyvsp[(1) - (1)].num) < -32768)
			yyerror("Resource's ID out of range (%d)", (yyvsp[(1) - (1)].num));
		(yyval.nid) = new_name_id();
		(yyval.nid)->type = name_ord;
		(yyval.nid)->name.i_name = (yyvsp[(1) - (1)].num);
		;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 570 "parser.y"
    {
		(yyval.nid) = new_name_id();
		(yyval.nid)->type = name_str;
		(yyval.nid)->name.s_name = (yyvsp[(1) - (1)].str);
		;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 580 "parser.y"
    { (yyval.nid) = (yyvsp[(1) - (1)].nid); ;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 581 "parser.y"
    {
		(yyval.nid) = new_name_id();
		(yyval.nid)->type = name_str;
		(yyval.nid)->name.s_name = (yyvsp[(1) - (1)].str);
		;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 590 "parser.y"
    { (yyval.res) = new_resource(res_acc, (yyvsp[(1) - (1)].acc), (yyvsp[(1) - (1)].acc)->memopt, (yyvsp[(1) - (1)].acc)->lvc.language); ;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 591 "parser.y"
    { (yyval.res) = new_resource(res_bmp, (yyvsp[(1) - (1)].bmp), (yyvsp[(1) - (1)].bmp)->memopt, (yyvsp[(1) - (1)].bmp)->data->lvc.language); ;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 592 "parser.y"
    {
		resource_t *rsc;
		if((yyvsp[(1) - (1)].ani)->type == res_anicur)
		{
			(yyval.res) = rsc = new_resource(res_anicur, (yyvsp[(1) - (1)].ani)->u.ani, (yyvsp[(1) - (1)].ani)->u.ani->memopt, (yyvsp[(1) - (1)].ani)->u.ani->data->lvc.language);
		}
		else if((yyvsp[(1) - (1)].ani)->type == res_curg)
		{
			cursor_t *cur;
			(yyval.res) = rsc = new_resource(res_curg, (yyvsp[(1) - (1)].ani)->u.curg, (yyvsp[(1) - (1)].ani)->u.curg->memopt, (yyvsp[(1) - (1)].ani)->u.curg->lvc.language);
			for(cur = (yyvsp[(1) - (1)].ani)->u.curg->cursorlist; cur; cur = cur->next)
			{
				rsc->prev = new_resource(res_cur, cur, (yyvsp[(1) - (1)].ani)->u.curg->memopt, (yyvsp[(1) - (1)].ani)->u.curg->lvc.language);
				rsc->prev->next = rsc;
				rsc = rsc->prev;
				rsc->name = new_name_id();
				rsc->name->type = name_ord;
				rsc->name->name.i_name = cur->id;
			}
		}
		else
			internal_error(__FILE__, __LINE__, "Invalid top-level type %d in cursor resource\n", (yyvsp[(1) - (1)].ani)->type);
		free((yyvsp[(1) - (1)].ani));
		;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 616 "parser.y"
    { (yyval.res) = new_resource(res_dlg, (yyvsp[(1) - (1)].dlg), (yyvsp[(1) - (1)].dlg)->memopt, (yyvsp[(1) - (1)].dlg)->lvc.language); ;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 617 "parser.y"
    {
		if(win32)
			(yyval.res) = new_resource(res_dlgex, (yyvsp[(1) - (1)].dlgex), (yyvsp[(1) - (1)].dlgex)->memopt, (yyvsp[(1) - (1)].dlgex)->lvc.language);
		else
			(yyval.res) = NULL;
		;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 623 "parser.y"
    { (yyval.res) = new_resource(res_dlginit, (yyvsp[(1) - (1)].dginit), (yyvsp[(1) - (1)].dginit)->memopt, (yyvsp[(1) - (1)].dginit)->data->lvc.language); ;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 624 "parser.y"
    { (yyval.res) = new_resource(res_fnt, (yyvsp[(1) - (1)].fnt), (yyvsp[(1) - (1)].fnt)->memopt, (yyvsp[(1) - (1)].fnt)->data->lvc.language); ;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 625 "parser.y"
    { (yyval.res) = new_resource(res_fntdir, (yyvsp[(1) - (1)].fnd), (yyvsp[(1) - (1)].fnd)->memopt, (yyvsp[(1) - (1)].fnd)->data->lvc.language); ;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 626 "parser.y"
    {
		resource_t *rsc;
		if((yyvsp[(1) - (1)].ani)->type == res_aniico)
		{
			(yyval.res) = rsc = new_resource(res_aniico, (yyvsp[(1) - (1)].ani)->u.ani, (yyvsp[(1) - (1)].ani)->u.ani->memopt, (yyvsp[(1) - (1)].ani)->u.ani->data->lvc.language);
		}
		else if((yyvsp[(1) - (1)].ani)->type == res_icog)
		{
			icon_t *ico;
			(yyval.res) = rsc = new_resource(res_icog, (yyvsp[(1) - (1)].ani)->u.icog, (yyvsp[(1) - (1)].ani)->u.icog->memopt, (yyvsp[(1) - (1)].ani)->u.icog->lvc.language);
			for(ico = (yyvsp[(1) - (1)].ani)->u.icog->iconlist; ico; ico = ico->next)
			{
				rsc->prev = new_resource(res_ico, ico, (yyvsp[(1) - (1)].ani)->u.icog->memopt, (yyvsp[(1) - (1)].ani)->u.icog->lvc.language);
				rsc->prev->next = rsc;
				rsc = rsc->prev;
				rsc->name = new_name_id();
				rsc->name->type = name_ord;
				rsc->name->name.i_name = ico->id;
			}
		}
		else
			internal_error(__FILE__, __LINE__, "Invalid top-level type %d in icon resource\n", (yyvsp[(1) - (1)].ani)->type);
		free((yyvsp[(1) - (1)].ani));
		;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 650 "parser.y"
    { (yyval.res) = new_resource(res_men, (yyvsp[(1) - (1)].men), (yyvsp[(1) - (1)].men)->memopt, (yyvsp[(1) - (1)].men)->lvc.language); ;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 651 "parser.y"
    {
		if(win32)
			(yyval.res) = new_resource(res_menex, (yyvsp[(1) - (1)].menex), (yyvsp[(1) - (1)].menex)->memopt, (yyvsp[(1) - (1)].menex)->lvc.language);
		else
			(yyval.res) = NULL;
		;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 657 "parser.y"
    { (yyval.res) = new_resource(res_msg, (yyvsp[(1) - (1)].msg), WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE, (yyvsp[(1) - (1)].msg)->data->lvc.language); ;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 658 "parser.y"
    { (yyval.res) = new_resource(res_html, (yyvsp[(1) - (1)].html), (yyvsp[(1) - (1)].html)->memopt, (yyvsp[(1) - (1)].html)->data->lvc.language); ;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 659 "parser.y"
    { (yyval.res) = new_resource(res_rdt, (yyvsp[(1) - (1)].rdt), (yyvsp[(1) - (1)].rdt)->memopt, (yyvsp[(1) - (1)].rdt)->data->lvc.language); ;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 660 "parser.y"
    { (yyval.res) = new_resource(res_toolbar, (yyvsp[(1) - (1)].tlbar), (yyvsp[(1) - (1)].tlbar)->memopt, (yyvsp[(1) - (1)].tlbar)->lvc.language); ;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 661 "parser.y"
    { (yyval.res) = new_resource(res_usr, (yyvsp[(1) - (1)].usr), (yyvsp[(1) - (1)].usr)->memopt, (yyvsp[(1) - (1)].usr)->data->lvc.language); ;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 662 "parser.y"
    { (yyval.res) = new_resource(res_ver, (yyvsp[(1) - (1)].veri), WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE, (yyvsp[(1) - (1)].veri)->lvc.language); ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 666 "parser.y"
    { (yyval.str) = make_filename((yyvsp[(1) - (1)].str)); ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 667 "parser.y"
    { (yyval.str) = make_filename((yyvsp[(1) - (1)].str)); ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 668 "parser.y"
    { (yyval.str) = make_filename((yyvsp[(1) - (1)].str)); ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 672 "parser.y"
    { (yyval.bmp) = new_bitmap((yyvsp[(3) - (3)].raw), (yyvsp[(2) - (3)].iptr)); ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 676 "parser.y"
    {
		(yyval.ani) = new_ani_any();
		if((yyvsp[(3) - (3)].raw)->size > 4 && !memcmp((yyvsp[(3) - (3)].raw)->data, riff, sizeof(riff)))
		{
			(yyval.ani)->type = res_anicur;
			(yyval.ani)->u.ani = new_ani_curico(res_anicur, (yyvsp[(3) - (3)].raw), (yyvsp[(2) - (3)].iptr));
		}
		else
		{
			(yyval.ani)->type = res_curg;
			(yyval.ani)->u.curg = new_cursor_group((yyvsp[(3) - (3)].raw), (yyvsp[(2) - (3)].iptr));
		}
	;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 692 "parser.y"
    {
		(yyval.ani) = new_ani_any();
		if((yyvsp[(3) - (3)].raw)->size > 4 && !memcmp((yyvsp[(3) - (3)].raw)->data, riff, sizeof(riff)))
		{
			(yyval.ani)->type = res_aniico;
			(yyval.ani)->u.ani = new_ani_curico(res_aniico, (yyvsp[(3) - (3)].raw), (yyvsp[(2) - (3)].iptr));
		}
		else
		{
			(yyval.ani)->type = res_icog;
			(yyval.ani)->u.icog = new_icon_group((yyvsp[(3) - (3)].raw), (yyvsp[(2) - (3)].iptr));
		}
	;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 714 "parser.y"
    { (yyval.fnt) = new_font((yyvsp[(3) - (3)].raw), (yyvsp[(2) - (3)].iptr)); ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 724 "parser.y"
    { (yyval.fnd) = new_fontdir((yyvsp[(3) - (3)].raw), (yyvsp[(2) - (3)].iptr)); ;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 732 "parser.y"
    {
		if(!win32)
			parser_warning("MESSAGETABLE not supported in 16-bit mode\n");
		(yyval.msg) = new_messagetable((yyvsp[(3) - (3)].raw), (yyvsp[(2) - (3)].iptr));
		;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 740 "parser.y"
    { (yyval.html) = new_html((yyvsp[(3) - (3)].raw), (yyvsp[(2) - (3)].iptr)); ;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 744 "parser.y"
    { (yyval.rdt) = new_rcdata((yyvsp[(3) - (3)].raw), (yyvsp[(2) - (3)].iptr)); ;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 748 "parser.y"
    { (yyval.dginit) = new_dlginit((yyvsp[(3) - (3)].raw), (yyvsp[(2) - (3)].iptr)); ;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 752 "parser.y"
    {
#ifdef WORDS_BIGENDIAN
			if(pedantic && byteorder != WRC_BO_LITTLE)
#else
			if(pedantic && byteorder == WRC_BO_BIG)
#endif
				parser_warning("Byteordering is not little-endian and type cannot be interpreted\n");
			(yyval.usr) = new_user((yyvsp[(1) - (3)].nid), (yyvsp[(3) - (3)].raw), (yyvsp[(2) - (3)].iptr));
		;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 763 "parser.y"
    {
		(yyval.nid) = new_name_id();
		(yyval.nid)->type = name_ord;
		(yyval.nid)->name.i_name = (yyvsp[(1) - (1)].num);
		;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 768 "parser.y"
    {
		(yyval.nid) = new_name_id();
		(yyval.nid)->type = name_str;
		(yyval.nid)->name.s_name = (yyvsp[(1) - (1)].str);
		;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 777 "parser.y"
    {
		(yyval.acc) = new_accelerator();
		if((yyvsp[(2) - (6)].iptr))
		{
			(yyval.acc)->memopt = *((yyvsp[(2) - (6)].iptr));
			free((yyvsp[(2) - (6)].iptr));
		}
		else
		{
			(yyval.acc)->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE;
		}
		if(!(yyvsp[(5) - (6)].event))
			yyerror("Accelerator table must have at least one entry");
		(yyval.acc)->events = get_event_head((yyvsp[(5) - (6)].event));
		if((yyvsp[(3) - (6)].lvc))
		{
			(yyval.acc)->lvc = *((yyvsp[(3) - (6)].lvc));
			free((yyvsp[(3) - (6)].lvc));
		}
		if(!(yyval.acc)->lvc.language)
			(yyval.acc)->lvc.language = dup_language(currentlanguage);
		;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 801 "parser.y"
    { (yyval.event)=NULL; ;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 802 "parser.y"
    { (yyval.event)=add_string_event((yyvsp[(2) - (5)].str), (yyvsp[(4) - (5)].num), (yyvsp[(5) - (5)].num), (yyvsp[(1) - (5)].event)); ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 803 "parser.y"
    { (yyval.event)=add_event((yyvsp[(2) - (5)].num), (yyvsp[(4) - (5)].num), (yyvsp[(5) - (5)].num), (yyvsp[(1) - (5)].event)); ;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 812 "parser.y"
    { (yyval.num) = 0; ;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 813 "parser.y"
    { (yyval.num) = (yyvsp[(2) - (2)].num); ;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 816 "parser.y"
    { (yyval.num) = (yyvsp[(1) - (1)].num); ;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 817 "parser.y"
    { (yyval.num) = (yyvsp[(1) - (3)].num) | (yyvsp[(3) - (3)].num); ;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 820 "parser.y"
    { (yyval.num) = WRC_AF_NOINVERT; ;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 821 "parser.y"
    { (yyval.num) = WRC_AF_SHIFT; ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 822 "parser.y"
    { (yyval.num) = WRC_AF_CONTROL; ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 823 "parser.y"
    { (yyval.num) = WRC_AF_ALT; ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 824 "parser.y"
    { (yyval.num) = WRC_AF_ASCII; ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 825 "parser.y"
    { (yyval.num) = WRC_AF_VIRTKEY; ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 831 "parser.y"
    {
		if((yyvsp[(2) - (13)].iptr))
		{
			(yyvsp[(10) - (13)].dlg)->memopt = *((yyvsp[(2) - (13)].iptr));
			free((yyvsp[(2) - (13)].iptr));
		}
		else
			(yyvsp[(10) - (13)].dlg)->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
		(yyvsp[(10) - (13)].dlg)->x = (yyvsp[(3) - (13)].num);
		(yyvsp[(10) - (13)].dlg)->y = (yyvsp[(5) - (13)].num);
		(yyvsp[(10) - (13)].dlg)->width = (yyvsp[(7) - (13)].num);
		(yyvsp[(10) - (13)].dlg)->height = (yyvsp[(9) - (13)].num);
		(yyvsp[(10) - (13)].dlg)->controls = get_control_head((yyvsp[(12) - (13)].ctl));
		(yyval.dlg) = (yyvsp[(10) - (13)].dlg);
		if(!(yyval.dlg)->gotstyle)
		{
			(yyval.dlg)->style = new_style(0,0);
			(yyval.dlg)->style->or_mask = WS_POPUP;
			(yyval.dlg)->gotstyle = TRUE;
		}
		if((yyval.dlg)->title)
			(yyval.dlg)->style->or_mask |= WS_CAPTION;
		if((yyval.dlg)->font)
			(yyval.dlg)->style->or_mask |= DS_SETFONT;

		(yyval.dlg)->style->or_mask &= ~((yyval.dlg)->style->and_mask);
		(yyval.dlg)->style->and_mask = 0;

		if(!(yyval.dlg)->lvc.language)
			(yyval.dlg)->lvc.language = dup_language(currentlanguage);
		;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 865 "parser.y"
    { (yyval.dlg)=new_dialog(); ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 866 "parser.y"
    { (yyval.dlg)=dialog_style((yyvsp[(3) - (3)].style),(yyvsp[(1) - (3)].dlg)); ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 867 "parser.y"
    { (yyval.dlg)=dialog_exstyle((yyvsp[(3) - (3)].style),(yyvsp[(1) - (3)].dlg)); ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 868 "parser.y"
    { (yyval.dlg)=dialog_caption((yyvsp[(3) - (3)].str),(yyvsp[(1) - (3)].dlg)); ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 869 "parser.y"
    { (yyval.dlg)=dialog_font((yyvsp[(2) - (2)].fntid),(yyvsp[(1) - (2)].dlg)); ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 870 "parser.y"
    { (yyval.dlg)=dialog_class((yyvsp[(3) - (3)].nid),(yyvsp[(1) - (3)].dlg)); ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 871 "parser.y"
    { (yyval.dlg)=dialog_menu((yyvsp[(3) - (3)].nid),(yyvsp[(1) - (3)].dlg)); ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 872 "parser.y"
    { (yyval.dlg)=dialog_language((yyvsp[(2) - (2)].lan),(yyvsp[(1) - (2)].dlg)); ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 873 "parser.y"
    { (yyval.dlg)=dialog_characteristics((yyvsp[(2) - (2)].chars),(yyvsp[(1) - (2)].dlg)); ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 874 "parser.y"
    { (yyval.dlg)=dialog_version((yyvsp[(2) - (2)].ver),(yyvsp[(1) - (2)].dlg)); ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 877 "parser.y"
    { (yyval.ctl) = NULL; ;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 878 "parser.y"
    { (yyval.ctl)=ins_ctrl(-1, 0, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 879 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_EDIT, 0, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 880 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_LISTBOX, 0, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 881 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_COMBOBOX, 0, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 882 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_SCROLLBAR, 0, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 883 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_BUTTON, BS_CHECKBOX, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 884 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_BUTTON, BS_DEFPUSHBUTTON, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 885 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_BUTTON, BS_GROUPBOX, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl));;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 886 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_BUTTON, BS_PUSHBUTTON, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 888 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_BUTTON, BS_RADIOBUTTON, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 889 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_BUTTON, BS_AUTO3STATE, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 890 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_BUTTON, BS_3STATE, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 891 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_BUTTON, BS_AUTOCHECKBOX, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 892 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_BUTTON, BS_AUTORADIOBUTTON, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 893 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_STATIC, SS_LEFT, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 894 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_STATIC, SS_CENTER, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 895 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_STATIC, SS_RIGHT, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 897 "parser.y"
    {
		(yyvsp[(10) - (10)].ctl)->title = (yyvsp[(3) - (10)].nid);
		(yyvsp[(10) - (10)].ctl)->id = (yyvsp[(5) - (10)].num);
		(yyvsp[(10) - (10)].ctl)->x = (yyvsp[(7) - (10)].num);
		(yyvsp[(10) - (10)].ctl)->y = (yyvsp[(9) - (10)].num);
		(yyval.ctl) = ins_ctrl(CT_STATIC, SS_ICON, (yyvsp[(10) - (10)].ctl), (yyvsp[(1) - (10)].ctl));
		;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 907 "parser.y"
    {
		(yyval.ctl)=new_control();
		(yyval.ctl)->title = (yyvsp[(1) - (12)].nid);
		(yyval.ctl)->id = (yyvsp[(3) - (12)].num);
		(yyval.ctl)->x = (yyvsp[(5) - (12)].num);
		(yyval.ctl)->y = (yyvsp[(7) - (12)].num);
		(yyval.ctl)->width = (yyvsp[(9) - (12)].num);
		(yyval.ctl)->height = (yyvsp[(11) - (12)].num);
		if((yyvsp[(12) - (12)].styles))
		{
			(yyval.ctl)->style = (yyvsp[(12) - (12)].styles)->style;
			(yyval.ctl)->gotstyle = TRUE;
			if ((yyvsp[(12) - (12)].styles)->exstyle)
			{
			    (yyval.ctl)->exstyle = (yyvsp[(12) - (12)].styles)->exstyle;
			    (yyval.ctl)->gotexstyle = TRUE;
			}
			free((yyvsp[(12) - (12)].styles));
		}
		;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 930 "parser.y"
    {
		(yyval.ctl) = new_control();
		(yyval.ctl)->id = (yyvsp[(1) - (10)].num);
		(yyval.ctl)->x = (yyvsp[(3) - (10)].num);
		(yyval.ctl)->y = (yyvsp[(5) - (10)].num);
		(yyval.ctl)->width = (yyvsp[(7) - (10)].num);
		(yyval.ctl)->height = (yyvsp[(9) - (10)].num);
		if((yyvsp[(10) - (10)].styles))
		{
			(yyval.ctl)->style = (yyvsp[(10) - (10)].styles)->style;
			(yyval.ctl)->gotstyle = TRUE;
			if ((yyvsp[(10) - (10)].styles)->exstyle)
			{
			    (yyval.ctl)->exstyle = (yyvsp[(10) - (10)].styles)->exstyle;
			    (yyval.ctl)->gotexstyle = TRUE;
			}
			free((yyvsp[(10) - (10)].styles));
		}
		;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 952 "parser.y"
    { (yyval.ctl) = new_control(); ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 954 "parser.y"
    {
		(yyval.ctl) = new_control();
		(yyval.ctl)->width = (yyvsp[(2) - (4)].num);
		(yyval.ctl)->height = (yyvsp[(4) - (4)].num);
		;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 959 "parser.y"
    {
		(yyval.ctl) = new_control();
		(yyval.ctl)->width = (yyvsp[(2) - (6)].num);
		(yyval.ctl)->height = (yyvsp[(4) - (6)].num);
		(yyval.ctl)->style = (yyvsp[(6) - (6)].style);
		(yyval.ctl)->gotstyle = TRUE;
		;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 966 "parser.y"
    {
		(yyval.ctl) = new_control();
		(yyval.ctl)->width = (yyvsp[(2) - (8)].num);
		(yyval.ctl)->height = (yyvsp[(4) - (8)].num);
		(yyval.ctl)->style = (yyvsp[(6) - (8)].style);
		(yyval.ctl)->gotstyle = TRUE;
		(yyval.ctl)->exstyle = (yyvsp[(8) - (8)].style);
		(yyval.ctl)->gotexstyle = TRUE;
		;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 977 "parser.y"
    {
		(yyval.ctl)=new_control();
		(yyval.ctl)->title = (yyvsp[(1) - (17)].nid);
		(yyval.ctl)->id = (yyvsp[(3) - (17)].num);
		(yyval.ctl)->ctlclass = convert_ctlclass((yyvsp[(5) - (17)].nid));
		(yyval.ctl)->style = (yyvsp[(7) - (17)].style);
		(yyval.ctl)->gotstyle = TRUE;
		(yyval.ctl)->x = (yyvsp[(9) - (17)].num);
		(yyval.ctl)->y = (yyvsp[(11) - (17)].num);
		(yyval.ctl)->width = (yyvsp[(13) - (17)].num);
		(yyval.ctl)->height = (yyvsp[(15) - (17)].num);
		(yyval.ctl)->exstyle = (yyvsp[(17) - (17)].style);
		(yyval.ctl)->gotexstyle = TRUE;
		;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 991 "parser.y"
    {
		(yyval.ctl)=new_control();
		(yyval.ctl)->title = (yyvsp[(1) - (15)].nid);
		(yyval.ctl)->id = (yyvsp[(3) - (15)].num);
		(yyval.ctl)->ctlclass = convert_ctlclass((yyvsp[(5) - (15)].nid));
		(yyval.ctl)->style = (yyvsp[(7) - (15)].style);
		(yyval.ctl)->gotstyle = TRUE;
		(yyval.ctl)->x = (yyvsp[(9) - (15)].num);
		(yyval.ctl)->y = (yyvsp[(11) - (15)].num);
		(yyval.ctl)->width = (yyvsp[(13) - (15)].num);
		(yyval.ctl)->height = (yyvsp[(15) - (15)].num);
		;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 1006 "parser.y"
    { (yyval.fntid) = new_font_id((yyvsp[(2) - (4)].num), (yyvsp[(4) - (4)].str), 0, 0); ;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1011 "parser.y"
    { (yyval.styles) = NULL; ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1012 "parser.y"
    { (yyval.styles) = new_style_pair((yyvsp[(2) - (2)].style), 0); ;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 1013 "parser.y"
    { (yyval.styles) = new_style_pair((yyvsp[(2) - (4)].style), (yyvsp[(4) - (4)].style)); ;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 1017 "parser.y"
    { (yyval.style) = new_style((yyvsp[(1) - (3)].style)->or_mask | (yyvsp[(3) - (3)].style)->or_mask, (yyvsp[(1) - (3)].style)->and_mask | (yyvsp[(3) - (3)].style)->and_mask); free((yyvsp[(1) - (3)].style)); free((yyvsp[(3) - (3)].style));;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 1018 "parser.y"
    { (yyval.style) = (yyvsp[(2) - (3)].style); ;}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 1019 "parser.y"
    { (yyval.style) = new_style((yyvsp[(1) - (1)].num), 0); ;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 1020 "parser.y"
    { (yyval.style) = new_style(0, (yyvsp[(2) - (2)].num)); ;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 1024 "parser.y"
    {
		(yyval.nid) = new_name_id();
		(yyval.nid)->type = name_ord;
		(yyval.nid)->name.i_name = (yyvsp[(1) - (1)].num);
		;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 1029 "parser.y"
    {
		(yyval.nid) = new_name_id();
		(yyval.nid)->type = name_str;
		(yyval.nid)->name.s_name = (yyvsp[(1) - (1)].str);
		;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 1038 "parser.y"
    {
		if(!win32)
			parser_warning("DIALOGEX not supported in 16-bit mode\n");
		if((yyvsp[(2) - (14)].iptr))
		{
			(yyvsp[(11) - (14)].dlgex)->memopt = *((yyvsp[(2) - (14)].iptr));
			free((yyvsp[(2) - (14)].iptr));
		}
		else
			(yyvsp[(11) - (14)].dlgex)->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
		(yyvsp[(11) - (14)].dlgex)->x = (yyvsp[(3) - (14)].num);
		(yyvsp[(11) - (14)].dlgex)->y = (yyvsp[(5) - (14)].num);
		(yyvsp[(11) - (14)].dlgex)->width = (yyvsp[(7) - (14)].num);
		(yyvsp[(11) - (14)].dlgex)->height = (yyvsp[(9) - (14)].num);
		if((yyvsp[(10) - (14)].iptr))
		{
			(yyvsp[(11) - (14)].dlgex)->helpid = *((yyvsp[(10) - (14)].iptr));
			(yyvsp[(11) - (14)].dlgex)->gothelpid = TRUE;
			free((yyvsp[(10) - (14)].iptr));
		}
		(yyvsp[(11) - (14)].dlgex)->controls = get_control_head((yyvsp[(13) - (14)].ctl));
		(yyval.dlgex) = (yyvsp[(11) - (14)].dlgex);

		assert((yyval.dlgex)->style != NULL);
		if(!(yyval.dlgex)->gotstyle)
		{
			(yyval.dlgex)->style->or_mask = WS_POPUP;
			(yyval.dlgex)->gotstyle = TRUE;
		}
		if((yyval.dlgex)->title)
			(yyval.dlgex)->style->or_mask |= WS_CAPTION;
		if((yyval.dlgex)->font)
			(yyval.dlgex)->style->or_mask |= DS_SETFONT;

		(yyval.dlgex)->style->or_mask &= ~((yyval.dlgex)->style->and_mask);
		(yyval.dlgex)->style->and_mask = 0;

		if(!(yyval.dlgex)->lvc.language)
			(yyval.dlgex)->lvc.language = dup_language(currentlanguage);
		;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 1081 "parser.y"
    { (yyval.dlgex)=new_dialogex(); ;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 1082 "parser.y"
    { (yyval.dlgex)=dialogex_style((yyvsp[(3) - (3)].style),(yyvsp[(1) - (3)].dlgex)); ;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 1083 "parser.y"
    { (yyval.dlgex)=dialogex_exstyle((yyvsp[(3) - (3)].style),(yyvsp[(1) - (3)].dlgex)); ;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 1084 "parser.y"
    { (yyval.dlgex)=dialogex_caption((yyvsp[(3) - (3)].str),(yyvsp[(1) - (3)].dlgex)); ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 1085 "parser.y"
    { (yyval.dlgex)=dialogex_font((yyvsp[(2) - (2)].fntid),(yyvsp[(1) - (2)].dlgex)); ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 1086 "parser.y"
    { (yyval.dlgex)=dialogex_font((yyvsp[(2) - (2)].fntid),(yyvsp[(1) - (2)].dlgex)); ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 1087 "parser.y"
    { (yyval.dlgex)=dialogex_class((yyvsp[(3) - (3)].nid),(yyvsp[(1) - (3)].dlgex)); ;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 1088 "parser.y"
    { (yyval.dlgex)=dialogex_menu((yyvsp[(3) - (3)].nid),(yyvsp[(1) - (3)].dlgex)); ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 1089 "parser.y"
    { (yyval.dlgex)=dialogex_language((yyvsp[(2) - (2)].lan),(yyvsp[(1) - (2)].dlgex)); ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1090 "parser.y"
    { (yyval.dlgex)=dialogex_characteristics((yyvsp[(2) - (2)].chars),(yyvsp[(1) - (2)].dlgex)); ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1091 "parser.y"
    { (yyval.dlgex)=dialogex_version((yyvsp[(2) - (2)].ver),(yyvsp[(1) - (2)].dlgex)); ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1094 "parser.y"
    { (yyval.ctl) = NULL; ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1095 "parser.y"
    { (yyval.ctl)=ins_ctrl(-1, 0, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1096 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_EDIT, 0, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1097 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_LISTBOX, 0, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1098 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_COMBOBOX, 0, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1099 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_SCROLLBAR, 0, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1100 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_BUTTON, BS_CHECKBOX, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1101 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_BUTTON, BS_DEFPUSHBUTTON, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1102 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_BUTTON, BS_GROUPBOX, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl));;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1103 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_BUTTON, BS_PUSHBUTTON, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1105 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_BUTTON, BS_RADIOBUTTON, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1106 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_BUTTON, BS_AUTO3STATE, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1107 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_BUTTON, BS_3STATE, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1108 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_BUTTON, BS_AUTOCHECKBOX, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1109 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_BUTTON, BS_AUTORADIOBUTTON, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1110 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_STATIC, SS_LEFT, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1111 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_STATIC, SS_CENTER, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1112 "parser.y"
    { (yyval.ctl)=ins_ctrl(CT_STATIC, SS_RIGHT, (yyvsp[(3) - (3)].ctl), (yyvsp[(1) - (3)].ctl)); ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1114 "parser.y"
    {
		(yyvsp[(10) - (10)].ctl)->title = (yyvsp[(3) - (10)].nid);
		(yyvsp[(10) - (10)].ctl)->id = (yyvsp[(5) - (10)].num);
		(yyvsp[(10) - (10)].ctl)->x = (yyvsp[(7) - (10)].num);
		(yyvsp[(10) - (10)].ctl)->y = (yyvsp[(9) - (10)].num);
		(yyval.ctl) = ins_ctrl(CT_STATIC, SS_ICON, (yyvsp[(10) - (10)].ctl), (yyvsp[(1) - (10)].ctl));
		;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1125 "parser.y"
    {
		(yyval.ctl)=new_control();
		(yyval.ctl)->title = (yyvsp[(1) - (19)].nid);
		(yyval.ctl)->id = (yyvsp[(3) - (19)].num);
		(yyval.ctl)->ctlclass = convert_ctlclass((yyvsp[(5) - (19)].nid));
		(yyval.ctl)->style = (yyvsp[(7) - (19)].style);
		(yyval.ctl)->gotstyle = TRUE;
		(yyval.ctl)->x = (yyvsp[(9) - (19)].num);
		(yyval.ctl)->y = (yyvsp[(11) - (19)].num);
		(yyval.ctl)->width = (yyvsp[(13) - (19)].num);
		(yyval.ctl)->height = (yyvsp[(15) - (19)].num);
		if((yyvsp[(17) - (19)].style))
		{
			(yyval.ctl)->exstyle = (yyvsp[(17) - (19)].style);
			(yyval.ctl)->gotexstyle = TRUE;
		}
		if((yyvsp[(18) - (19)].iptr))
		{
			(yyval.ctl)->helpid = *((yyvsp[(18) - (19)].iptr));
			(yyval.ctl)->gothelpid = TRUE;
			free((yyvsp[(18) - (19)].iptr));
		}
		(yyval.ctl)->extra = (yyvsp[(19) - (19)].raw);
		;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1149 "parser.y"
    {
		(yyval.ctl)=new_control();
		(yyval.ctl)->title = (yyvsp[(1) - (16)].nid);
		(yyval.ctl)->id = (yyvsp[(3) - (16)].num);
		(yyval.ctl)->style = (yyvsp[(7) - (16)].style);
		(yyval.ctl)->gotstyle = TRUE;
		(yyval.ctl)->ctlclass = convert_ctlclass((yyvsp[(5) - (16)].nid));
		(yyval.ctl)->x = (yyvsp[(9) - (16)].num);
		(yyval.ctl)->y = (yyvsp[(11) - (16)].num);
		(yyval.ctl)->width = (yyvsp[(13) - (16)].num);
		(yyval.ctl)->height = (yyvsp[(15) - (16)].num);
		(yyval.ctl)->extra = (yyvsp[(16) - (16)].raw);
		;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1165 "parser.y"
    {
		(yyval.ctl)=new_control();
		(yyval.ctl)->title = (yyvsp[(1) - (14)].nid);
		(yyval.ctl)->id = (yyvsp[(3) - (14)].num);
		(yyval.ctl)->x = (yyvsp[(5) - (14)].num);
		(yyval.ctl)->y = (yyvsp[(7) - (14)].num);
		(yyval.ctl)->width = (yyvsp[(9) - (14)].num);
		(yyval.ctl)->height = (yyvsp[(11) - (14)].num);
		if((yyvsp[(12) - (14)].styles))
		{
			(yyval.ctl)->style = (yyvsp[(12) - (14)].styles)->style;
			(yyval.ctl)->gotstyle = TRUE;

			if ((yyvsp[(12) - (14)].styles)->exstyle)
			{
			    (yyval.ctl)->exstyle = (yyvsp[(12) - (14)].styles)->exstyle;
			    (yyval.ctl)->gotexstyle = TRUE;
			}
			free((yyvsp[(12) - (14)].styles));
		}

		(yyval.ctl)->extra = (yyvsp[(14) - (14)].raw);
		;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1191 "parser.y"
    {
		(yyval.ctl) = new_control();
		(yyval.ctl)->id = (yyvsp[(1) - (12)].num);
		(yyval.ctl)->x = (yyvsp[(3) - (12)].num);
		(yyval.ctl)->y = (yyvsp[(5) - (12)].num);
		(yyval.ctl)->width = (yyvsp[(7) - (12)].num);
		(yyval.ctl)->height = (yyvsp[(9) - (12)].num);
		if((yyvsp[(10) - (12)].styles))
		{
			(yyval.ctl)->style = (yyvsp[(10) - (12)].styles)->style;
			(yyval.ctl)->gotstyle = TRUE;

			if ((yyvsp[(10) - (12)].styles)->exstyle)
			{
			    (yyval.ctl)->exstyle = (yyvsp[(10) - (12)].styles)->exstyle;
			    (yyval.ctl)->gotexstyle = TRUE;
			}
			free((yyvsp[(10) - (12)].styles));
		}
		(yyval.ctl)->extra = (yyvsp[(12) - (12)].raw);
		;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1214 "parser.y"
    { (yyval.raw) = NULL; ;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1215 "parser.y"
    { (yyval.raw) = (yyvsp[(1) - (1)].raw); ;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1218 "parser.y"
    { (yyval.iptr) = NULL; ;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1219 "parser.y"
    { (yyval.iptr) = new_int((yyvsp[(2) - (2)].num)); ;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 1223 "parser.y"
    { (yyval.fntid) = new_font_id((yyvsp[(2) - (9)].num), (yyvsp[(4) - (9)].str), (yyvsp[(6) - (9)].num), (yyvsp[(8) - (9)].num)); ;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1230 "parser.y"
    { (yyval.fntid) = NULL; ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1231 "parser.y"
    { (yyval.fntid) = NULL; ;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1235 "parser.y"
    {
		if(!(yyvsp[(4) - (4)].menitm))
			yyerror("Menu must contain items");
		(yyval.men) = new_menu();
		if((yyvsp[(2) - (4)].iptr))
		{
			(yyval.men)->memopt = *((yyvsp[(2) - (4)].iptr));
			free((yyvsp[(2) - (4)].iptr));
		}
		else
			(yyval.men)->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
		(yyval.men)->items = get_item_head((yyvsp[(4) - (4)].menitm));
		if((yyvsp[(3) - (4)].lvc))
		{
			(yyval.men)->lvc = *((yyvsp[(3) - (4)].lvc));
			free((yyvsp[(3) - (4)].lvc));
		}
		if(!(yyval.men)->lvc.language)
			(yyval.men)->lvc.language = dup_language(currentlanguage);
		;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1258 "parser.y"
    { (yyval.menitm) = (yyvsp[(2) - (3)].menitm); ;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 1262 "parser.y"
    {(yyval.menitm) = NULL;;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1263 "parser.y"
    {
		(yyval.menitm)=new_menu_item();
		(yyval.menitm)->prev = (yyvsp[(1) - (6)].menitm);
		if((yyvsp[(1) - (6)].menitm))
			(yyvsp[(1) - (6)].menitm)->next = (yyval.menitm);
		(yyval.menitm)->id =  (yyvsp[(5) - (6)].num);
		(yyval.menitm)->state = (yyvsp[(6) - (6)].num);
		(yyval.menitm)->name = (yyvsp[(3) - (6)].str);
		;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1272 "parser.y"
    {
		(yyval.menitm)=new_menu_item();
		(yyval.menitm)->prev = (yyvsp[(1) - (3)].menitm);
		if((yyvsp[(1) - (3)].menitm))
			(yyvsp[(1) - (3)].menitm)->next = (yyval.menitm);
		;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1278 "parser.y"
    {
		(yyval.menitm) = new_menu_item();
		(yyval.menitm)->prev = (yyvsp[(1) - (5)].menitm);
		if((yyvsp[(1) - (5)].menitm))
			(yyvsp[(1) - (5)].menitm)->next = (yyval.menitm);
		(yyval.menitm)->popup = get_item_head((yyvsp[(5) - (5)].menitm));
		(yyval.menitm)->name = (yyvsp[(3) - (5)].str);
		;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1297 "parser.y"
    { (yyval.num) = 0; ;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1298 "parser.y"
    { (yyval.num) = (yyvsp[(3) - (3)].num) | MF_CHECKED; ;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1299 "parser.y"
    { (yyval.num) = (yyvsp[(3) - (3)].num) | MF_GRAYED; ;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1300 "parser.y"
    { (yyval.num) = (yyvsp[(3) - (3)].num) | MF_HELP; ;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1301 "parser.y"
    { (yyval.num) = (yyvsp[(3) - (3)].num) | MF_DISABLED; ;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1302 "parser.y"
    { (yyval.num) = (yyvsp[(3) - (3)].num) | MF_MENUBARBREAK; ;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1303 "parser.y"
    { (yyval.num) = (yyvsp[(3) - (3)].num) | MF_MENUBREAK; ;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1307 "parser.y"
    {
		if(!win32)
			parser_warning("MENUEX not supported in 16-bit mode\n");
		if(!(yyvsp[(4) - (4)].menexitm))
			yyerror("MenuEx must contain items");
		(yyval.menex) = new_menuex();
		if((yyvsp[(2) - (4)].iptr))
		{
			(yyval.menex)->memopt = *((yyvsp[(2) - (4)].iptr));
			free((yyvsp[(2) - (4)].iptr));
		}
		else
			(yyval.menex)->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
		(yyval.menex)->items = get_itemex_head((yyvsp[(4) - (4)].menexitm));
		if((yyvsp[(3) - (4)].lvc))
		{
			(yyval.menex)->lvc = *((yyvsp[(3) - (4)].lvc));
			free((yyvsp[(3) - (4)].lvc));
		}
		if(!(yyval.menex)->lvc.language)
			(yyval.menex)->lvc.language = dup_language(currentlanguage);
		;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1332 "parser.y"
    { (yyval.menexitm) = (yyvsp[(2) - (3)].menexitm); ;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1336 "parser.y"
    {(yyval.menexitm) = NULL; ;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1337 "parser.y"
    {
		(yyval.menexitm) = new_menuex_item();
		(yyval.menexitm)->prev = (yyvsp[(1) - (4)].menexitm);
		if((yyvsp[(1) - (4)].menexitm))
			(yyvsp[(1) - (4)].menexitm)->next = (yyval.menexitm);
		(yyval.menexitm)->name = (yyvsp[(3) - (4)].str);
		(yyval.menexitm)->id = (yyvsp[(4) - (4)].exopt)->id;
		(yyval.menexitm)->type = (yyvsp[(4) - (4)].exopt)->type;
		(yyval.menexitm)->state = (yyvsp[(4) - (4)].exopt)->state;
		(yyval.menexitm)->helpid = (yyvsp[(4) - (4)].exopt)->helpid;
		(yyval.menexitm)->gotid = (yyvsp[(4) - (4)].exopt)->gotid;
		(yyval.menexitm)->gottype = (yyvsp[(4) - (4)].exopt)->gottype;
		(yyval.menexitm)->gotstate = (yyvsp[(4) - (4)].exopt)->gotstate;
		(yyval.menexitm)->gothelpid = (yyvsp[(4) - (4)].exopt)->gothelpid;
		free((yyvsp[(4) - (4)].exopt));
		;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1353 "parser.y"
    {
		(yyval.menexitm) = new_menuex_item();
		(yyval.menexitm)->prev = (yyvsp[(1) - (3)].menexitm);
		if((yyvsp[(1) - (3)].menexitm))
			(yyvsp[(1) - (3)].menexitm)->next = (yyval.menexitm);
		;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1359 "parser.y"
    {
		(yyval.menexitm) = new_menuex_item();
		(yyval.menexitm)->prev = (yyvsp[(1) - (5)].menexitm);
		if((yyvsp[(1) - (5)].menexitm))
			(yyvsp[(1) - (5)].menexitm)->next = (yyval.menexitm);
		(yyval.menexitm)->popup = get_itemex_head((yyvsp[(5) - (5)].menexitm));
		(yyval.menexitm)->name = (yyvsp[(3) - (5)].str);
		(yyval.menexitm)->id = (yyvsp[(4) - (5)].exopt)->id;
		(yyval.menexitm)->type = (yyvsp[(4) - (5)].exopt)->type;
		(yyval.menexitm)->state = (yyvsp[(4) - (5)].exopt)->state;
		(yyval.menexitm)->helpid = (yyvsp[(4) - (5)].exopt)->helpid;
		(yyval.menexitm)->gotid = (yyvsp[(4) - (5)].exopt)->gotid;
		(yyval.menexitm)->gottype = (yyvsp[(4) - (5)].exopt)->gottype;
		(yyval.menexitm)->gotstate = (yyvsp[(4) - (5)].exopt)->gotstate;
		(yyval.menexitm)->gothelpid = (yyvsp[(4) - (5)].exopt)->gothelpid;
		free((yyvsp[(4) - (5)].exopt));
		;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1379 "parser.y"
    { (yyval.exopt) = new_itemex_opt(0, 0, 0, 0); ;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1380 "parser.y"
    {
		(yyval.exopt) = new_itemex_opt((yyvsp[(2) - (2)].num), 0, 0, 0);
		(yyval.exopt)->gotid = TRUE;
		;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1384 "parser.y"
    {
		(yyval.exopt) = new_itemex_opt((yyvsp[(2) - (5)].iptr) ? *((yyvsp[(2) - (5)].iptr)) : 0, (yyvsp[(4) - (5)].iptr) ? *((yyvsp[(4) - (5)].iptr)) : 0, (yyvsp[(5) - (5)].num), 0);
		(yyval.exopt)->gotid = TRUE;
		(yyval.exopt)->gottype = TRUE;
		(yyval.exopt)->gotstate = TRUE;
		free((yyvsp[(2) - (5)].iptr));
		free((yyvsp[(4) - (5)].iptr));
		;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1392 "parser.y"
    {
		(yyval.exopt) = new_itemex_opt((yyvsp[(2) - (6)].iptr) ? *((yyvsp[(2) - (6)].iptr)) : 0, (yyvsp[(4) - (6)].iptr) ? *((yyvsp[(4) - (6)].iptr)) : 0, (yyvsp[(6) - (6)].num), 0);
		(yyval.exopt)->gotid = TRUE;
		(yyval.exopt)->gottype = TRUE;
		(yyval.exopt)->gotstate = TRUE;
		free((yyvsp[(2) - (6)].iptr));
		free((yyvsp[(4) - (6)].iptr));
		;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1403 "parser.y"
    { (yyval.exopt) = new_itemex_opt(0, 0, 0, 0); ;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1404 "parser.y"
    {
		(yyval.exopt) = new_itemex_opt((yyvsp[(2) - (2)].num), 0, 0, 0);
		(yyval.exopt)->gotid = TRUE;
		;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1408 "parser.y"
    {
		(yyval.exopt) = new_itemex_opt((yyvsp[(2) - (4)].iptr) ? *((yyvsp[(2) - (4)].iptr)) : 0, (yyvsp[(4) - (4)].num), 0, 0);
		free((yyvsp[(2) - (4)].iptr));
		(yyval.exopt)->gotid = TRUE;
		(yyval.exopt)->gottype = TRUE;
		;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1414 "parser.y"
    {
		(yyval.exopt) = new_itemex_opt((yyvsp[(2) - (6)].iptr) ? *((yyvsp[(2) - (6)].iptr)) : 0, (yyvsp[(4) - (6)].iptr) ? *((yyvsp[(4) - (6)].iptr)) : 0, (yyvsp[(6) - (6)].num), 0);
		free((yyvsp[(2) - (6)].iptr));
		free((yyvsp[(4) - (6)].iptr));
		(yyval.exopt)->gotid = TRUE;
		(yyval.exopt)->gottype = TRUE;
		(yyval.exopt)->gotstate = TRUE;
		;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1422 "parser.y"
    {
		(yyval.exopt) = new_itemex_opt((yyvsp[(2) - (8)].iptr) ? *((yyvsp[(2) - (8)].iptr)) : 0, (yyvsp[(4) - (8)].iptr) ? *((yyvsp[(4) - (8)].iptr)) : 0, (yyvsp[(6) - (8)].iptr) ? *((yyvsp[(6) - (8)].iptr)) : 0, (yyvsp[(8) - (8)].num));
		free((yyvsp[(2) - (8)].iptr));
		free((yyvsp[(4) - (8)].iptr));
		free((yyvsp[(6) - (8)].iptr));
		(yyval.exopt)->gotid = TRUE;
		(yyval.exopt)->gottype = TRUE;
		(yyval.exopt)->gotstate = TRUE;
		(yyval.exopt)->gothelpid = TRUE;
		;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1442 "parser.y"
    {
		if(!(yyvsp[(3) - (4)].stt))
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
		free(tagstt_memopt);
		tagstt_memopt = NULL;

		(yyval.stt) = tagstt;
		;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1480 "parser.y"
    {
		if((tagstt = find_stringtable((yyvsp[(3) - (3)].lvc))) == NULL)
			tagstt = new_stringtable((yyvsp[(3) - (3)].lvc));
		tagstt_memopt = (yyvsp[(2) - (3)].iptr);
		tagstt_version = (yyvsp[(3) - (3)].lvc)->version;
		tagstt_characts = (yyvsp[(3) - (3)].lvc)->characts;
		free((yyvsp[(3) - (3)].lvc));
		;}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 1490 "parser.y"
    { (yyval.stt) = NULL; ;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1491 "parser.y"
    {
		int i;
		assert(tagstt != NULL);
		if((yyvsp[(2) - (4)].num) > 65535 || (yyvsp[(2) - (4)].num) < -32768)
			yyerror("Stringtable entry's ID out of range (%d)", (yyvsp[(2) - (4)].num));
		/* Search for the ID */
		for(i = 0; i < tagstt->nentries; i++)
		{
			if(tagstt->entries[i].id == (yyvsp[(2) - (4)].num))
				yyerror("Stringtable ID %d already in use", (yyvsp[(2) - (4)].num));
		}
		/* If we get here, then we have a new unique entry */
		tagstt->nentries++;
		tagstt->entries = xrealloc(tagstt->entries, sizeof(tagstt->entries[0]) * tagstt->nentries);
		tagstt->entries[tagstt->nentries-1].id = (yyvsp[(2) - (4)].num);
		tagstt->entries[tagstt->nentries-1].str = (yyvsp[(4) - (4)].str);
		if(tagstt_memopt)
			tagstt->entries[tagstt->nentries-1].memopt = *tagstt_memopt;
		else
			tagstt->entries[tagstt->nentries-1].memopt = WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE | WRC_MO_PURE;
		tagstt->entries[tagstt->nentries-1].version = tagstt_version;
		tagstt->entries[tagstt->nentries-1].characts = tagstt_characts;

		if(pedantic && !(yyvsp[(4) - (4)].str)->size)
			parser_warning("Zero length strings make no sense\n");
		if(!win32 && (yyvsp[(4) - (4)].str)->size > 254)
			yyerror("Stringtable entry more than 254 characters");
		if(win32 && (yyvsp[(4) - (4)].str)->size > 65534) /* Hmm..., does this happen? */
			yyerror("Stringtable entry more than 65534 characters (probably something else that went wrong)");
		(yyval.stt) = tagstt;
		;}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1531 "parser.y"
    {
		(yyval.veri) = (yyvsp[(3) - (6)].veri);
		if((yyvsp[(2) - (6)].iptr))
		{
			(yyval.veri)->memopt = *((yyvsp[(2) - (6)].iptr));
			free((yyvsp[(2) - (6)].iptr));
		}
		else
			(yyval.veri)->memopt = WRC_MO_MOVEABLE | (win32 ? WRC_MO_PURE : 0);
		(yyval.veri)->blocks = get_ver_block_head((yyvsp[(5) - (6)].blk));
		/* Set language; there is no version or characteristics */
		(yyval.veri)->lvc.language = dup_language(currentlanguage);
		;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1547 "parser.y"
    { (yyval.veri) = new_versioninfo(); ;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1548 "parser.y"
    {
		if((yyvsp[(1) - (9)].veri)->gotit.fv)
			yyerror("FILEVERSION already defined");
		(yyval.veri) = (yyvsp[(1) - (9)].veri);
		(yyval.veri)->filever_maj1 = (yyvsp[(3) - (9)].num);
		(yyval.veri)->filever_maj2 = (yyvsp[(5) - (9)].num);
		(yyval.veri)->filever_min1 = (yyvsp[(7) - (9)].num);
		(yyval.veri)->filever_min2 = (yyvsp[(9) - (9)].num);
		(yyval.veri)->gotit.fv = 1;
		;}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1558 "parser.y"
    {
		if((yyvsp[(1) - (9)].veri)->gotit.pv)
			yyerror("PRODUCTVERSION already defined");
		(yyval.veri) = (yyvsp[(1) - (9)].veri);
		(yyval.veri)->prodver_maj1 = (yyvsp[(3) - (9)].num);
		(yyval.veri)->prodver_maj2 = (yyvsp[(5) - (9)].num);
		(yyval.veri)->prodver_min1 = (yyvsp[(7) - (9)].num);
		(yyval.veri)->prodver_min2 = (yyvsp[(9) - (9)].num);
		(yyval.veri)->gotit.pv = 1;
		;}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1568 "parser.y"
    {
		if((yyvsp[(1) - (3)].veri)->gotit.ff)
			yyerror("FILEFLAGS already defined");
		(yyval.veri) = (yyvsp[(1) - (3)].veri);
		(yyval.veri)->fileflags = (yyvsp[(3) - (3)].num);
		(yyval.veri)->gotit.ff = 1;
		;}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 1575 "parser.y"
    {
		if((yyvsp[(1) - (3)].veri)->gotit.ffm)
			yyerror("FILEFLAGSMASK already defined");
		(yyval.veri) = (yyvsp[(1) - (3)].veri);
		(yyval.veri)->fileflagsmask = (yyvsp[(3) - (3)].num);
		(yyval.veri)->gotit.ffm = 1;
		;}
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 1582 "parser.y"
    {
		if((yyvsp[(1) - (3)].veri)->gotit.fo)
			yyerror("FILEOS already defined");
		(yyval.veri) = (yyvsp[(1) - (3)].veri);
		(yyval.veri)->fileos = (yyvsp[(3) - (3)].num);
		(yyval.veri)->gotit.fo = 1;
		;}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 1589 "parser.y"
    {
		if((yyvsp[(1) - (3)].veri)->gotit.ft)
			yyerror("FILETYPE already defined");
		(yyval.veri) = (yyvsp[(1) - (3)].veri);
		(yyval.veri)->filetype = (yyvsp[(3) - (3)].num);
		(yyval.veri)->gotit.ft = 1;
		;}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 1596 "parser.y"
    {
		if((yyvsp[(1) - (3)].veri)->gotit.fst)
			yyerror("FILESUBTYPE already defined");
		(yyval.veri) = (yyvsp[(1) - (3)].veri);
		(yyval.veri)->filesubtype = (yyvsp[(3) - (3)].num);
		(yyval.veri)->gotit.fst = 1;
		;}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 1606 "parser.y"
    { (yyval.blk) = NULL; ;}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 1607 "parser.y"
    {
		(yyval.blk) = (yyvsp[(2) - (2)].blk);
		(yyval.blk)->prev = (yyvsp[(1) - (2)].blk);
		if((yyvsp[(1) - (2)].blk))
			(yyvsp[(1) - (2)].blk)->next = (yyval.blk);
		;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 1616 "parser.y"
    {
		(yyval.blk) = new_ver_block();
		(yyval.blk)->name = (yyvsp[(2) - (5)].str);
		(yyval.blk)->values = get_ver_value_head((yyvsp[(4) - (5)].val));
		;}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 1624 "parser.y"
    { (yyval.val) = NULL; ;}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 1625 "parser.y"
    {
		(yyval.val) = (yyvsp[(2) - (2)].val);
		(yyval.val)->prev = (yyvsp[(1) - (2)].val);
		if((yyvsp[(1) - (2)].val))
			(yyvsp[(1) - (2)].val)->next = (yyval.val);
		;}
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 1634 "parser.y"
    {
		(yyval.val) = new_ver_value();
		(yyval.val)->type = val_block;
		(yyval.val)->value.block = (yyvsp[(1) - (1)].blk);
		;}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 1639 "parser.y"
    {
		(yyval.val) = new_ver_value();
		(yyval.val)->type = val_str;
		(yyval.val)->key = (yyvsp[(2) - (4)].str);
		(yyval.val)->value.str = (yyvsp[(4) - (4)].str);
		;}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 1645 "parser.y"
    {
		(yyval.val) = new_ver_value();
		(yyval.val)->type = val_words;
		(yyval.val)->key = (yyvsp[(2) - (4)].str);
		(yyval.val)->value.words = (yyvsp[(4) - (4)].verw);
		;}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 1654 "parser.y"
    { (yyval.verw) = new_ver_words((yyvsp[(1) - (1)].num)); ;}
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 1655 "parser.y"
    { (yyval.verw) = add_ver_words((yyvsp[(1) - (3)].verw), (yyvsp[(3) - (3)].num)); ;}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 1659 "parser.y"
    {
		int nitems;
		toolbar_item_t *items = get_tlbr_buttons_head((yyvsp[(8) - (9)].tlbarItems), &nitems);
		(yyval.tlbar) = new_toolbar((yyvsp[(3) - (9)].num), (yyvsp[(5) - (9)].num), items, nitems);
		if((yyvsp[(2) - (9)].iptr))
		{
			(yyval.tlbar)->memopt = *((yyvsp[(2) - (9)].iptr));
			free((yyvsp[(2) - (9)].iptr));
		}
		else
		{
			(yyval.tlbar)->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE;
		}
		if((yyvsp[(6) - (9)].lvc))
		{
			(yyval.tlbar)->lvc = *((yyvsp[(6) - (9)].lvc));
			free((yyvsp[(6) - (9)].lvc));
		}
		if(!(yyval.tlbar)->lvc.language)
		{
			(yyval.tlbar)->lvc.language = dup_language(currentlanguage);
		}
		;}
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 1685 "parser.y"
    { (yyval.tlbarItems) = NULL; ;}
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 1686 "parser.y"
    {
		toolbar_item_t *idrec = new_toolbar_item();
		idrec->id = (yyvsp[(3) - (3)].num);
		(yyval.tlbarItems) = ins_tlbr_button((yyvsp[(1) - (3)].tlbarItems), idrec);
		;}
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 1691 "parser.y"
    {
		toolbar_item_t *idrec = new_toolbar_item();
		idrec->id = 0;
		(yyval.tlbarItems) = ins_tlbr_button((yyvsp[(1) - (2)].tlbarItems), idrec);
	;}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 1700 "parser.y"
    { (yyval.iptr) = NULL; ;}
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 1701 "parser.y"
    {
		if((yyvsp[(1) - (2)].iptr))
		{
			*((yyvsp[(1) - (2)].iptr)) |= *((yyvsp[(2) - (2)].iptr));
			(yyval.iptr) = (yyvsp[(1) - (2)].iptr);
			free((yyvsp[(2) - (2)].iptr));
		}
		else
			(yyval.iptr) = (yyvsp[(2) - (2)].iptr);
		;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 1711 "parser.y"
    {
		if((yyvsp[(1) - (2)].iptr))
		{
			*((yyvsp[(1) - (2)].iptr)) &= *((yyvsp[(2) - (2)].iptr));
			(yyval.iptr) = (yyvsp[(1) - (2)].iptr);
			free((yyvsp[(2) - (2)].iptr));
		}
		else
		{
			*(yyvsp[(2) - (2)].iptr) &= WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE | WRC_MO_PURE;
			(yyval.iptr) = (yyvsp[(2) - (2)].iptr);
		}
		;}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 1726 "parser.y"
    { (yyval.iptr) = new_int(WRC_MO_PRELOAD); ;}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 1727 "parser.y"
    { (yyval.iptr) = new_int(WRC_MO_MOVEABLE); ;}
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 1728 "parser.y"
    { (yyval.iptr) = new_int(WRC_MO_DISCARDABLE); ;}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 1729 "parser.y"
    { (yyval.iptr) = new_int(WRC_MO_PURE); ;}
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 1732 "parser.y"
    { (yyval.iptr) = new_int(~WRC_MO_PRELOAD); ;}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 1733 "parser.y"
    { (yyval.iptr) = new_int(~WRC_MO_MOVEABLE); ;}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 1734 "parser.y"
    { (yyval.iptr) = new_int(~WRC_MO_PURE); ;}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 1738 "parser.y"
    { (yyval.lvc) = new_lvc(); ;}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 1739 "parser.y"
    {
		if(!win32)
			parser_warning("LANGUAGE not supported in 16-bit mode\n");
		if((yyvsp[(1) - (2)].lvc)->language)
			yyerror("Language already defined");
		(yyval.lvc) = (yyvsp[(1) - (2)].lvc);
		(yyvsp[(1) - (2)].lvc)->language = (yyvsp[(2) - (2)].lan);
		;}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 1747 "parser.y"
    {
		if(!win32)
			parser_warning("CHARACTERISTICS not supported in 16-bit mode\n");
		if((yyvsp[(1) - (2)].lvc)->characts)
			yyerror("Characteristics already defined");
		(yyval.lvc) = (yyvsp[(1) - (2)].lvc);
		(yyvsp[(1) - (2)].lvc)->characts = (yyvsp[(2) - (2)].chars);
		;}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 1755 "parser.y"
    {
		if(!win32)
			parser_warning("VERSION not supported in 16-bit mode\n");
		if((yyvsp[(1) - (2)].lvc)->version)
			yyerror("Version already defined");
		(yyval.lvc) = (yyvsp[(1) - (2)].lvc);
		(yyvsp[(1) - (2)].lvc)->version = (yyvsp[(2) - (2)].ver);
		;}
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 1773 "parser.y"
    { (yyval.lan) = new_language((yyvsp[(2) - (4)].num), (yyvsp[(4) - (4)].num));
					  if (get_language_codepage((yyvsp[(2) - (4)].num), (yyvsp[(4) - (4)].num)) == -1)
						yyerror( "Language %04x is not supported", ((yyvsp[(4) - (4)].num)<<10) + (yyvsp[(2) - (4)].num));
					;}
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 1780 "parser.y"
    { (yyval.chars) = new_characts((yyvsp[(2) - (2)].num)); ;}
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 1784 "parser.y"
    { (yyval.ver) = new_version((yyvsp[(2) - (2)].num)); ;}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 1788 "parser.y"
    {
		if((yyvsp[(1) - (4)].lvc))
		{
			(yyvsp[(3) - (4)].raw)->lvc = *((yyvsp[(1) - (4)].lvc));
			free((yyvsp[(1) - (4)].lvc));
		}

		if(!(yyvsp[(3) - (4)].raw)->lvc.language)
			(yyvsp[(3) - (4)].raw)->lvc.language = dup_language(currentlanguage);

		(yyval.raw) = (yyvsp[(3) - (4)].raw);
		;}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 1803 "parser.y"
    { (yyval.raw) = (yyvsp[(1) - (1)].raw); ;}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 1804 "parser.y"
    { (yyval.raw) = int2raw_data((yyvsp[(1) - (1)].num)); ;}
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 1805 "parser.y"
    { (yyval.raw) = int2raw_data(-((yyvsp[(2) - (2)].num))); ;}
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 1806 "parser.y"
    { (yyval.raw) = long2raw_data((yyvsp[(1) - (1)].num)); ;}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 1807 "parser.y"
    { (yyval.raw) = long2raw_data(-((yyvsp[(2) - (2)].num))); ;}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 1808 "parser.y"
    { (yyval.raw) = str2raw_data((yyvsp[(1) - (1)].str)); ;}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 1809 "parser.y"
    { (yyval.raw) = merge_raw_data((yyvsp[(1) - (3)].raw), (yyvsp[(3) - (3)].raw)); free((yyvsp[(3) - (3)].raw)->data); free((yyvsp[(3) - (3)].raw)); ;}
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 1810 "parser.y"
    { (yyval.raw) = merge_raw_data_int((yyvsp[(1) - (3)].raw), (yyvsp[(3) - (3)].num)); ;}
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 1811 "parser.y"
    { (yyval.raw) = merge_raw_data_int((yyvsp[(1) - (4)].raw), -((yyvsp[(4) - (4)].num))); ;}
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 1812 "parser.y"
    { (yyval.raw) = merge_raw_data_long((yyvsp[(1) - (3)].raw), (yyvsp[(3) - (3)].num)); ;}
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 1813 "parser.y"
    { (yyval.raw) = merge_raw_data_long((yyvsp[(1) - (4)].raw), -((yyvsp[(4) - (4)].num))); ;}
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 1814 "parser.y"
    { (yyval.raw) = merge_raw_data_str((yyvsp[(1) - (3)].raw), (yyvsp[(3) - (3)].str)); ;}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 1818 "parser.y"
    { (yyval.raw) = load_file((yyvsp[(1) - (1)].str),dup_language(currentlanguage)); ;}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 1819 "parser.y"
    { (yyval.raw) = (yyvsp[(1) - (1)].raw); ;}
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 1826 "parser.y"
    { (yyval.iptr) = 0; ;}
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 1827 "parser.y"
    { (yyval.iptr) = new_int((yyvsp[(1) - (1)].num)); ;}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 1831 "parser.y"
    { (yyval.num) = ((yyvsp[(1) - (1)].num)); ;}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 1834 "parser.y"
    { (yyval.num) = ((yyvsp[(1) - (3)].num)) + ((yyvsp[(3) - (3)].num)); ;}
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 1835 "parser.y"
    { (yyval.num) = ((yyvsp[(1) - (3)].num)) - ((yyvsp[(3) - (3)].num)); ;}
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 1836 "parser.y"
    { (yyval.num) = ((yyvsp[(1) - (3)].num)) | ((yyvsp[(3) - (3)].num)); ;}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 1837 "parser.y"
    { (yyval.num) = ((yyvsp[(1) - (3)].num)) & ((yyvsp[(3) - (3)].num)); ;}
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 1838 "parser.y"
    { (yyval.num) = ((yyvsp[(1) - (3)].num)) * ((yyvsp[(3) - (3)].num)); ;}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 1839 "parser.y"
    { (yyval.num) = ((yyvsp[(1) - (3)].num)) / ((yyvsp[(3) - (3)].num)); ;}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 1840 "parser.y"
    { (yyval.num) = ((yyvsp[(1) - (3)].num)) ^ ((yyvsp[(3) - (3)].num)); ;}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 1841 "parser.y"
    { (yyval.num) = ~((yyvsp[(2) - (2)].num)); ;}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 1842 "parser.y"
    { (yyval.num) = -((yyvsp[(2) - (2)].num)); ;}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 1843 "parser.y"
    { (yyval.num) = (yyvsp[(2) - (2)].num); ;}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 1844 "parser.y"
    { (yyval.num) = (yyvsp[(2) - (3)].num); ;}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 1845 "parser.y"
    { (yyval.num) = (yyvsp[(1) - (1)].num); ;}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 1846 "parser.y"
    { (yyval.num) = ~((yyvsp[(2) - (2)].num)); ;}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 1849 "parser.y"
    { (yyval.num) = (yyvsp[(1) - (1)].num); ;}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 1850 "parser.y"
    { (yyval.num) = (yyvsp[(1) - (1)].num); ;}
    break;



/* Line 1455 of yacc.c  */
#line 4933 "parser.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
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
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
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


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

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

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 1853 "parser.y"

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
		parser_warning("Style already defined, or-ing together\n");
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
		parser_warning("ExStyle already defined, or-ing together\n");
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

	/* Check for duplicate identifiers */
	while (prev)
	{
		if (ctrl->id != -1 && ctrl->id == prev->id)
                        parser_warning("Duplicate dialog control id %d\n", ctrl->id);
		prev = prev->prev;
	}

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
				parser_warning("Unknown default button control-style 0x%08x\n", special_style);
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
				parser_warning("Unknown default static control-style 0x%08x\n", special_style);
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

static int get_class_idW(const WCHAR *cc)
{
        static const WCHAR szBUTTON[]    = {'B','U','T','T','O','N',0};
        static const WCHAR szCOMBOBOX[]  = {'C','O','M','B','O','B','O','X',0};
        static const WCHAR szLISTBOX[]   = {'L','I','S','T','B','O','X',0};
        static const WCHAR szEDIT[]      = {'E','D','I','T',0};
        static const WCHAR szSTATIC[]    = {'S','T','A','T','I','C',0};
        static const WCHAR szSCROLLBAR[] = {'S','C','R','O','L','L','B','A','R',0};

        if(!strcmpiW(szBUTTON, cc))
                return CT_BUTTON;
        if(!strcmpiW(szCOMBOBOX, cc))
                return CT_COMBOBOX;
        if(!strcmpiW(szLISTBOX, cc))
                return CT_LISTBOX;
        if(!strcmpiW(szEDIT, cc))
                return CT_EDIT;
        if(!strcmpiW(szSTATIC, cc))
                return CT_STATIC;
        if(!strcmpiW(szSCROLLBAR, cc))
                return CT_SCROLLBAR;

        return -1;
}

static int get_class_idA(const char *cc)
{
        if(!strcasecmp("BUTTON", cc))
                return CT_BUTTON;
        if(!strcasecmp("COMBOBOX", cc))
                return CT_COMBOBOX;
        if(!strcasecmp("LISTBOX", cc))
                return CT_LISTBOX;
        if(!strcasecmp("EDIT", cc))
                return CT_EDIT;
        if(!strcasecmp("STATIC", cc))
                return CT_STATIC;
        if(!strcasecmp("SCROLLBAR", cc))
                return CT_SCROLLBAR;

        return -1;
}


static name_id_t *convert_ctlclass(name_id_t *cls)
{
	int iclass;

	if(cls->type == name_ord)
		return cls;
	assert(cls->type == name_str);
        if(cls->name.s_name->type == str_unicode)
                iclass = get_class_idW(cls->name.s_name->str.wstr);
        else
                iclass = get_class_idA(cls->name.s_name->str.cstr);

        if (iclass == -1)
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
		parser_warning("Style already defined, or-ing together\n");
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
		parser_warning("ExStyle already defined, or-ing together\n");
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

    if(key->type == str_char)
    {
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
    }
    else
    {
	if((flags & WRC_AF_VIRTKEY) && !isupperW(key->str.wstr[0]) && !isdigitW(key->str.wstr[0]))
		yyerror("VIRTKEY code is not equal to ascii value");

	if(key->str.wstr[0] == '^' && (flags & WRC_AF_CONTROL) != 0)
	{
		yyerror("Cannot use both '^' and CONTROL modifier");
	}
	else if(key->str.wstr[0] == '^')
	{
		keycode = toupperW(key->str.wstr[1]) - '@';
		if(keycode >= ' ')
			yyerror("Control-code out of range");
	}
	else
		keycode = key->str.wstr[0];
    }

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
	itemex_opt_t *opt = xmalloc(sizeof(itemex_opt_t));
	memset( opt, 0, sizeof(*opt) );
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
		rd->data = xmalloc(rd->size);
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
		parser_warning("Integer constant out of 16bit range (%d), truncated to %d\n", i, (short)i);

	rd = new_raw_data();
	rd->size = sizeof(short);
	rd->data = xmalloc(rd->size);
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
	rd->data = xmalloc(rd->size);
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
	rd->data = xmalloc(rd->size);
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
		internal_error(__FILE__, __LINE__, "Invalid stringtype\n");
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
				parser_warning("Stringtable's versions are not the same, using first definition\n");

			if((stt->lvc.characts && lvc->characts && *(stt->lvc.characts) != *(lvc->characts))
			|| (!stt->lvc.characts && lvc->characts)
			|| (stt->lvc.characts && !lvc->characts))
				parser_warning("Stringtable's characteristics are not the same, using first definition\n");
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
			newstt->entries = xmalloc(16 * sizeof(stt_entry_t));
			memset( newstt->entries, 0, 16 * sizeof(stt_entry_t) );
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
				warning("Stringtable's memory options are not equal (idbase: %d)\n", newstt->idbase);
			}
			/* Check version and characteristics */
			for(j = 0; j < 16; j++)
			{
				if(characts
				&& newstt->entries[j].characts
				&& *newstt->entries[j].characts != *characts)
					warning("Stringtable's characteristics are not the same (idbase: %d)\n", newstt->idbase);
				if(version
				&& newstt->entries[j].version
				&& *newstt->entries[j].version != *version)
					warning("Stringtable's versions are not the same (idbase: %d)\n", newstt->idbase);
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
		warning("Need to parse fonts, not yet implemented (fnt: %p, nfnt: %d)\n", fnt, nfnt);
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
			warning("User supplied FONTDIR entry has an invalid name '%s', ignored\n",
				get_nameid_str(fnd[i]->name));
			fnd[i] = NULL;
		}
	}

	/* Sanity check */
	if(nfnt == 0)
	{
		if(nfnd != 0)
			warning("Found %d FONTDIR entries without any fonts present\n", nfnd);
		goto clean;
	}

	/* Copy space */
	lanfnt = xmalloc(nfnt * sizeof(*lanfnt));
	memset( lanfnt, 0, nfnt * sizeof(*lanfnt));

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
			error("FONTDIR for language %d,%d has wrong count (%d, expected %d)\n",
				fnd[i]->lan->id, fnd[i]->lan->sub, cnt, nlanfnt);
#ifdef WORDS_BIGENDIAN
		if((byteorder == WRC_BO_LITTLE && !isswapped) || (byteorder != WRC_BO_LITTLE && isswapped))
#else
		if((byteorder == WRC_BO_BIG && !isswapped) || (byteorder != WRC_BO_BIG && isswapped))
#endif
		{
			internal_error(__FILE__, __LINE__, "User supplied FONTDIR needs byteswapping\n");
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
	free(fnt);
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
		parser_warning("Usertype uses reserved type ID %d, which is auto-generated\n", yylval.num);
		return lookahead;

	case WRC_RT_DLGINCLUDE:
	case WRC_RT_PLUGPLAY:
	case WRC_RT_VXD:
		parser_warning("Usertype uses reserved type ID %d, which is not supported by wrc yet\n", yylval.num);
	default:
		return lookahead;
	}

	return token;
}

