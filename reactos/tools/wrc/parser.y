%{
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

%}
%union{
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
}

%token tNL
%token <num> tNUMBER tLNUMBER
%token <str> tSTRING tIDENT tFILENAME
%token <raw> tRAWDATA
%token tACCELERATORS tBITMAP tCURSOR tDIALOG tDIALOGEX tMENU tMENUEX tMESSAGETABLE
%token tRCDATA tVERSIONINFO tSTRINGTABLE tFONT tFONTDIR tICON tHTML
%token tAUTO3STATE tAUTOCHECKBOX tAUTORADIOBUTTON tCHECKBOX tDEFPUSHBUTTON
%token tPUSHBUTTON tRADIOBUTTON tSTATE3 /* PUSHBOX */
%token tGROUPBOX tCOMBOBOX tLISTBOX tSCROLLBAR
%token tCONTROL tEDITTEXT
%token tRTEXT tCTEXT tLTEXT
%token tBLOCK tVALUE
%token tSHIFT tALT tASCII tVIRTKEY tGRAYED tCHECKED tINACTIVE tNOINVERT
%token tPURE tIMPURE tDISCARDABLE tLOADONCALL tPRELOAD tFIXED tMOVEABLE
%token tCLASS tCAPTION tCHARACTERISTICS tEXSTYLE tSTYLE tVERSION tLANGUAGE
%token tFILEVERSION tPRODUCTVERSION tFILEFLAGSMASK tFILEOS tFILETYPE tFILEFLAGS tFILESUBTYPE
%token tMENUBARBREAK tMENUBREAK tMENUITEM tPOPUP tSEPARATOR
%token tHELP
%token tTOOLBAR tBUTTON
%token tBEGIN tEND
%token tDLGINIT
%left '|'
%left '^'
%left '&'
%left '+' '-'
%left '*' '/'
%right '~' tNOT
%left pUPM

%type <res> 	resource_file resource resources resource_definition
%type <stt>	stringtable strings
%type <fnt>	font
%type <fnd>	fontdir
%type <acc> 	accelerators
%type <event> 	events
%type <bmp> 	bitmap
%type <ani> 	cursor icon
%type <dlg> 	dialog dlg_attributes
%type <ctl> 	ctrls gen_ctrl lab_ctrl ctrl_desc iconinfo
%type <iptr>	helpid
%type <dlgex> 	dialogex dlgex_attribs
%type <ctl>	exctrls gen_exctrl lab_exctrl exctrl_desc
%type <html>	html
%type <rdt> 	rcdata
%type <raw>	raw_data raw_elements opt_data file_raw
%type <veri> 	versioninfo fix_version
%type <verw>	ver_words
%type <blk>	ver_blocks ver_block
%type <val>	ver_values ver_value
%type <men> 	menu
%type <menitm>	item_definitions menu_body
%type <menex>	menuex
%type <menexitm> itemex_definitions menuex_body
%type <exopt>	itemex_p_options itemex_options
%type <msg> 	messagetable
%type <usr> 	userres
%type <num> 	item_options
%type <nid> 	nameid nameid_s ctlclass usertype
%type <num> 	acc_opt acc accs
%type <iptr>	loadmemopts lamo lama
%type <fntid>	opt_font opt_exfont opt_expr
%type <lvc>	opt_lvc
%type <lan>	opt_language
%type <chars>	opt_characts
%type <ver>	opt_version
%type <num>	expr xpr
%type <iptr>	e_expr
%type <tlbar>	toolbar
%type <tlbarItems>	toolbar_items
%type <dginit>  dlginit
%type <styles>  optional_style_pair
%type <num>	any_num
%type <style>   style
%type <str>	filename

%%

resource_file
	: resources {
		resource_t *rsc;
		/* First add stringtables to the resource-list */
		rsc = build_stt_resources(sttres);
		/* 'build_stt_resources' returns a head and $1 is a tail */
		if($1)
		{
			$1->next = rsc;
			if(rsc)
				rsc->prev = $1;
		}
		else
			$1 = rsc;
		/* Find the tail again */
		while($1 && $1->next)
			$1 = $1->next;
		/* Now add any fontdirecory */
		rsc = build_fontdirs($1);
		/* 'build_fontdir' returns a head and $1 is a tail */
		if($1)
		{
			$1->next = rsc;
			if(rsc)
				rsc->prev = $1;
		}
		else
			$1 = rsc;
		/* Final statement before were done */
		resource_top = get_resource_head($1);
		}
	;

/* Resources are put into a linked list */
resources
	: /* Empty */		{ $$ = NULL; want_id = 1; }
	| resources resource	{
		if($2)
		{
			resource_t *tail = $2;
			resource_t *head = $2;
			while(tail->next)
				tail = tail->next;
			while(head->prev)
				head = head->prev;
			head->prev = $1;
			if($1)
				$1->next = head;
			$$ = tail;
			/* Check for duplicate identifiers */
			while($1 && head)
			{
				resource_t *rsc = $1;
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
		else if($1)
		{
			resource_t *tail = $1;
			while(tail->next)
				tail = tail->next;
			$$ = tail;
		}
		else
			$$ = NULL;

		if(!dont_want_id)	/* See comments in language parsing below */
			want_id = 1;
		dont_want_id = 0;
		}
	/*
	 * The following newline rule will never get reduced because we never
	 * get the tNL token, unless we explicitly set the 'want_nl'
	 * flag, which we don't.
	 * The *ONLY* reason for this to be here is because Berkeley
	 * yacc (byacc), at least version 1.9, has a bug.
	 * (identified in the generated parser on the second
	 *  line with:
	 *  static char yysccsid[] = "@(#)yaccpar   1.9 (Berkeley) 02/21/93";
	 * )
	 * This extra rule fixes it.
	 * The problem is that the expression handling rule "expr: xpr"
	 * is not reduced on non-terminal tokens, defined above in the
	 * %token declarations. Token tNL is the only non-terminal that
	 * can occur. The error becomes visible in the language parsing
	 * rule below, which looks at the look-ahead token and tests it
	 * for tNL. However, byacc already generates an error upon reading
	 * the token instead of keeping it as a lookahead. The reason
	 * lies in the lack of a $default transition in the "expr : xpr . "
	 * state (currently state 25). It is probably omitted because tNL
	 * is a non-terminal and the state contains 2 s/r conflicts. The
	 * state enumerates all possible transitions instead of using a
	 * $default transition.
	 * All in all, it is a bug in byacc. (period)
	 */
	| resources tNL
	;


/* Parse top level resource definitions etc. */
resource
	: expr usrcvt resource_definition {
		$$ = $3;
		if($$)
		{
			if($1 > 65535 || $1 < -32768)
				yyerror("Resource's ID out of range (%d)", $1);
			$$->name = new_name_id();
			$$->name->type = name_ord;
			$$->name->name.i_name = $1;
			chat("Got %s (%d)\n", get_typename($3), $$->name->name.i_name);
			}
			}
	| tIDENT usrcvt resource_definition {
		$$ = $3;
		if($$)
		{
			$$->name = new_name_id();
			$$->name->type = name_str;
			$$->name->name.s_name = $1;
			chat("Got %s (%s)\n", get_typename($3), $$->name->name.s_name->str.cstr);
		}
		}
	| stringtable {
		/* Don't do anything, stringtables are converted to
		 * resource_t structures when we are finished parsing and
		 * the final rule of the parser is reduced (see above)
		 */
		$$ = NULL;
		chat("Got STRINGTABLE\n");
		}
	| tLANGUAGE {want_nl = 1; } expr ',' expr {
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
		if (get_language_codepage($3, $5) == -1)
			yyerror( "Language %04x is not supported", ($5<<10) + $3);
		currentlanguage = new_language($3, $5);
		$$ = NULL;
		chat("Got LANGUAGE %d,%d (0x%04x)\n", $3, $5, ($5<<10) + $3);
		}
	;

/*
 * Remapping of numerical resource types
 * (see also comment of called function below)
 */
usrcvt	: /* Empty */	{ yychar = rsrcid_to_token(yychar); }
	;

/*
 * Get a valid name/id
 */
nameid	: expr	{
		if($1 > 65535 || $1 < -32768)
			yyerror("Resource's ID out of range (%d)", $1);
		$$ = new_name_id();
		$$->type = name_ord;
		$$->name.i_name = $1;
		}
	| tIDENT {
		$$ = new_name_id();
		$$->type = name_str;
		$$->name.s_name = $1;
		}
	;

/*
 * Extra string recognition for CLASS statement in dialogs
 */
nameid_s: nameid	{ $$ = $1; }
	| tSTRING	{
		$$ = new_name_id();
		$$->type = name_str;
		$$->name.s_name = $1;
		}
	;

/* get the value for a single resource*/
resource_definition
	: accelerators	{ $$ = new_resource(res_acc, $1, $1->memopt, $1->lvc.language); }
	| bitmap	{ $$ = new_resource(res_bmp, $1, $1->memopt, $1->data->lvc.language); }
	| cursor {
		resource_t *rsc;
		if($1->type == res_anicur)
		{
			$$ = rsc = new_resource(res_anicur, $1->u.ani, $1->u.ani->memopt, $1->u.ani->data->lvc.language);
		}
		else if($1->type == res_curg)
		{
			cursor_t *cur;
			$$ = rsc = new_resource(res_curg, $1->u.curg, $1->u.curg->memopt, $1->u.curg->lvc.language);
			for(cur = $1->u.curg->cursorlist; cur; cur = cur->next)
			{
				rsc->prev = new_resource(res_cur, cur, $1->u.curg->memopt, $1->u.curg->lvc.language);
				rsc->prev->next = rsc;
				rsc = rsc->prev;
				rsc->name = new_name_id();
				rsc->name->type = name_ord;
				rsc->name->name.i_name = cur->id;
			}
		}
		else
			internal_error(__FILE__, __LINE__, "Invalid top-level type %d in cursor resource\n", $1->type);
		free($1);
		}
	| dialog	{ $$ = new_resource(res_dlg, $1, $1->memopt, $1->lvc.language); }
	| dialogex {
		if(win32)
			$$ = new_resource(res_dlgex, $1, $1->memopt, $1->lvc.language);
		else
			$$ = NULL;
		}
	| dlginit	{ $$ = new_resource(res_dlginit, $1, $1->memopt, $1->data->lvc.language); }
	| font		{ $$ = new_resource(res_fnt, $1, $1->memopt, $1->data->lvc.language); }
	| fontdir	{ $$ = new_resource(res_fntdir, $1, $1->memopt, $1->data->lvc.language); }
	| icon {
		resource_t *rsc;
		if($1->type == res_aniico)
		{
			$$ = rsc = new_resource(res_aniico, $1->u.ani, $1->u.ani->memopt, $1->u.ani->data->lvc.language);
		}
		else if($1->type == res_icog)
		{
			icon_t *ico;
			$$ = rsc = new_resource(res_icog, $1->u.icog, $1->u.icog->memopt, $1->u.icog->lvc.language);
			for(ico = $1->u.icog->iconlist; ico; ico = ico->next)
			{
				rsc->prev = new_resource(res_ico, ico, $1->u.icog->memopt, $1->u.icog->lvc.language);
				rsc->prev->next = rsc;
				rsc = rsc->prev;
				rsc->name = new_name_id();
				rsc->name->type = name_ord;
				rsc->name->name.i_name = ico->id;
			}
		}
		else
			internal_error(__FILE__, __LINE__, "Invalid top-level type %d in icon resource\n", $1->type);
		free($1);
		}
	| menu		{ $$ = new_resource(res_men, $1, $1->memopt, $1->lvc.language); }
	| menuex {
		if(win32)
			$$ = new_resource(res_menex, $1, $1->memopt, $1->lvc.language);
		else
			$$ = NULL;
		}
	| messagetable	{ $$ = new_resource(res_msg, $1, WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE, $1->data->lvc.language); }
	| html		{ $$ = new_resource(res_html, $1, $1->memopt, $1->data->lvc.language); }
	| rcdata	{ $$ = new_resource(res_rdt, $1, $1->memopt, $1->data->lvc.language); }
	| toolbar	{ $$ = new_resource(res_toolbar, $1, $1->memopt, $1->lvc.language); }
	| userres	{ $$ = new_resource(res_usr, $1, $1->memopt, $1->data->lvc.language); }
	| versioninfo	{ $$ = new_resource(res_ver, $1, WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE, $1->lvc.language); }
	;


filename: tFILENAME	{ $$ = make_filename($1); }
	| tIDENT	{ $$ = make_filename($1); }
	| tSTRING	{ $$ = make_filename($1); }
	;

/* ------------------------------ Bitmap ------------------------------ */
bitmap	: tBITMAP loadmemopts file_raw	{ $$ = new_bitmap($3, $2); }
	;

/* ------------------------------ Cursor ------------------------------ */
cursor	: tCURSOR loadmemopts file_raw	{
		$$ = new_ani_any();
		if($3->size > 4 && !memcmp($3->data, riff, sizeof(riff)))
		{
			$$->type = res_anicur;
			$$->u.ani = new_ani_curico(res_anicur, $3, $2);
		}
		else
		{
			$$->type = res_curg;
			$$->u.curg = new_cursor_group($3, $2);
		}
	}
	;

/* ------------------------------ Icon ------------------------------ */
icon	: tICON loadmemopts file_raw	{
		$$ = new_ani_any();
		if($3->size > 4 && !memcmp($3->data, riff, sizeof(riff)))
		{
			$$->type = res_aniico;
			$$->u.ani = new_ani_curico(res_aniico, $3, $2);
		}
		else
		{
			$$->type = res_icog;
			$$->u.icog = new_icon_group($3, $2);
		}
	}
	;

/* ------------------------------ Font ------------------------------ */
	/*
	 * The reading of raw_data for fonts is a Borland BRC
	 * extension. MS generates an error. However, it is
	 * most logical to support this, considering how wine
	 * enters things in CVS (ascii).
	 */
font	: tFONT loadmemopts file_raw	{ $$ = new_font($3, $2); }
	;

	/*
	 * The fontdir is a Borland BRC extension which only
	 * reads the data as 'raw_data' from the file.
	 * I don't know whether it is interpreted.
	 * The fontdir is generated if it was not present and
	 * fonts are defined in the source.
	 */
fontdir	: tFONTDIR loadmemopts file_raw	{ $$ = new_fontdir($3, $2); }
	;

/* ------------------------------ MessageTable ------------------------------ */
/* It might be interesting to implement the MS Message compiler here as well
 * to get everything in one source. Might be a future project.
 */
messagetable
	: tMESSAGETABLE loadmemopts file_raw	{
		if(!win32)
			parser_warning("MESSAGETABLE not supported in 16-bit mode\n");
		$$ = new_messagetable($3, $2);
		}
	;

/* ------------------------------ HTML ------------------------------ */
html	: tHTML loadmemopts file_raw	{ $$ = new_html($3, $2); }
	;

/* ------------------------------ RCData ------------------------------ */
rcdata	: tRCDATA loadmemopts file_raw	{ $$ = new_rcdata($3, $2); }
	;

/* ------------------------------ DLGINIT ------------------------------ */
dlginit	: tDLGINIT loadmemopts file_raw	{ $$ = new_dlginit($3, $2); }
	;

/* ------------------------------ UserType ------------------------------ */
userres	: usertype loadmemopts file_raw		{
#ifdef WORDS_BIGENDIAN
			if(pedantic && byteorder != WRC_BO_LITTLE)
#else
			if(pedantic && byteorder == WRC_BO_BIG)
#endif
				parser_warning("Byteordering is not little-endian and type cannot be interpreted\n");
			$$ = new_user($1, $3, $2);
		}
	;

usertype: tNUMBER {
		$$ = new_name_id();
		$$->type = name_ord;
		$$->name.i_name = $1;
		}
	| tIDENT {
		$$ = new_name_id();
		$$->type = name_str;
		$$->name.s_name = $1;
		}
	;

/* ------------------------------ Accelerator ------------------------------ */
accelerators
	: tACCELERATORS loadmemopts opt_lvc tBEGIN events tEND {
		$$ = new_accelerator();
		if($2)
		{
			$$->memopt = *($2);
			free($2);
		}
		else
		{
			$$->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE;
		}
		if(!$5)
			yyerror("Accelerator table must have at least one entry");
		$$->events = get_event_head($5);
		if($3)
		{
			$$->lvc = *($3);
			free($3);
		}
		if(!$$->lvc.language)
			$$->lvc.language = dup_language(currentlanguage);
		}
	;

events	: /* Empty */ 				{ $$=NULL; }
	| events tSTRING ',' expr acc_opt	{ $$=add_string_event($2, $4, $5, $1); }
	| events expr ',' expr acc_opt		{ $$=add_event($2, $4, $5, $1); }
	;

/*
 * The empty rule generates a s/r conflict because of {bi,u}nary expr
 * on - and +. It cannot be solved in any way because it is the same as
 * the if/then/else problem (LALR(1) problem). The conflict is moved
 * away by forcing it to be in the expression handling below.
 */
acc_opt	: /* Empty */	{ $$ = 0; }
	| ',' accs	{ $$ = $2; }
	;

accs	: acc		{ $$ = $1; }
	| accs ',' acc	{ $$ = $1 | $3; }
	;

acc	: tNOINVERT 	{ $$ = WRC_AF_NOINVERT; }
	| tSHIFT	{ $$ = WRC_AF_SHIFT; }
	| tCONTROL	{ $$ = WRC_AF_CONTROL; }
	| tALT		{ $$ = WRC_AF_ALT; }
	| tASCII	{ $$ = WRC_AF_ASCII; }
	| tVIRTKEY	{ $$ = WRC_AF_VIRTKEY; }
	;

/* ------------------------------ Dialog ------------------------------ */
/* FIXME: Support EXSTYLE in the dialog line itself */
dialog	: tDIALOG loadmemopts expr ',' expr ',' expr ',' expr dlg_attributes
	  tBEGIN  ctrls tEND {
		if($2)
		{
			$10->memopt = *($2);
			free($2);
		}
		else
			$10->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
		$10->x = $3;
		$10->y = $5;
		$10->width = $7;
		$10->height = $9;
		$10->controls = get_control_head($12);
		$$ = $10;
		if(!$$->gotstyle)
		{
			$$->style = new_style(0,0);
			$$->style->or_mask = WS_POPUP;
			$$->gotstyle = TRUE;
		}
		if($$->title)
			$$->style->or_mask |= WS_CAPTION;
		if($$->font)
			$$->style->or_mask |= DS_SETFONT;

		$$->style->or_mask &= ~($$->style->and_mask);
		$$->style->and_mask = 0;

		if(!$$->lvc.language)
			$$->lvc.language = dup_language(currentlanguage);
		}
	;

dlg_attributes
	: /* Empty */				{ $$=new_dialog(); }
	| dlg_attributes tSTYLE style		{ $$=dialog_style($3,$1); }
	| dlg_attributes tEXSTYLE style		{ $$=dialog_exstyle($3,$1); }
	| dlg_attributes tCAPTION tSTRING	{ $$=dialog_caption($3,$1); }
	| dlg_attributes opt_font		{ $$=dialog_font($2,$1); }
	| dlg_attributes tCLASS nameid_s	{ $$=dialog_class($3,$1); }
	| dlg_attributes tMENU nameid		{ $$=dialog_menu($3,$1); }
	| dlg_attributes opt_language		{ $$=dialog_language($2,$1); }
	| dlg_attributes opt_characts		{ $$=dialog_characteristics($2,$1); }
	| dlg_attributes opt_version		{ $$=dialog_version($2,$1); }
	;

ctrls	: /* Empty */				{ $$ = NULL; }
	| ctrls tCONTROL	gen_ctrl	{ $$=ins_ctrl(-1, 0, $3, $1); }
	| ctrls tEDITTEXT	ctrl_desc	{ $$=ins_ctrl(CT_EDIT, 0, $3, $1); }
	| ctrls tLISTBOX	ctrl_desc	{ $$=ins_ctrl(CT_LISTBOX, 0, $3, $1); }
	| ctrls tCOMBOBOX	ctrl_desc	{ $$=ins_ctrl(CT_COMBOBOX, 0, $3, $1); }
	| ctrls tSCROLLBAR	ctrl_desc	{ $$=ins_ctrl(CT_SCROLLBAR, 0, $3, $1); }
	| ctrls tCHECKBOX	lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_CHECKBOX, $3, $1); }
	| ctrls tDEFPUSHBUTTON	lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_DEFPUSHBUTTON, $3, $1); }
	| ctrls tGROUPBOX	lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_GROUPBOX, $3, $1);}
	| ctrls tPUSHBUTTON	lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_PUSHBUTTON, $3, $1); }
/*	| ctrls tPUSHBOX	lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_PUSHBOX, $3, $1); } */
	| ctrls tRADIOBUTTON	lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_RADIOBUTTON, $3, $1); }
	| ctrls tAUTO3STATE	lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_AUTO3STATE, $3, $1); }
	| ctrls tSTATE3		lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_3STATE, $3, $1); }
	| ctrls tAUTOCHECKBOX	lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_AUTOCHECKBOX, $3, $1); }
	| ctrls tAUTORADIOBUTTON lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_AUTORADIOBUTTON, $3, $1); }
	| ctrls tLTEXT		lab_ctrl	{ $$=ins_ctrl(CT_STATIC, SS_LEFT, $3, $1); }
	| ctrls tCTEXT		lab_ctrl	{ $$=ins_ctrl(CT_STATIC, SS_CENTER, $3, $1); }
	| ctrls tRTEXT		lab_ctrl	{ $$=ins_ctrl(CT_STATIC, SS_RIGHT, $3, $1); }
	/* special treatment for icons, as the extent is optional */
	| ctrls tICON nameid_s opt_comma expr ',' expr ',' expr iconinfo {
		$10->title = $3;
		$10->id = $5;
		$10->x = $7;
		$10->y = $9;
		$$ = ins_ctrl(CT_STATIC, SS_ICON, $10, $1);
		}
	;

lab_ctrl
	: nameid_s opt_comma expr ',' expr ',' expr ',' expr ',' expr optional_style_pair {
		$$=new_control();
		$$->title = $1;
		$$->id = $3;
		$$->x = $5;
		$$->y = $7;
		$$->width = $9;
		$$->height = $11;
		if($12)
		{
			$$->style = $12->style;
			$$->gotstyle = TRUE;
			if ($12->exstyle)
			{
			    $$->exstyle = $12->exstyle;
			    $$->gotexstyle = TRUE;
			}
			free($12);
		}
		}
	;

ctrl_desc
	: expr ',' expr ',' expr ',' expr ',' expr optional_style_pair {
		$$ = new_control();
		$$->id = $1;
		$$->x = $3;
		$$->y = $5;
		$$->width = $7;
		$$->height = $9;
		if($10)
		{
			$$->style = $10->style;
			$$->gotstyle = TRUE;
			if ($10->exstyle)
			{
			    $$->exstyle = $10->exstyle;
			    $$->gotexstyle = TRUE;
			}
			free($10);
		}
		}
	;

iconinfo: /* Empty */
		{ $$ = new_control(); }

	| ',' expr ',' expr {
		$$ = new_control();
		$$->width = $2;
		$$->height = $4;
		}
	| ',' expr ',' expr ',' style {
		$$ = new_control();
		$$->width = $2;
		$$->height = $4;
		$$->style = $6;
		$$->gotstyle = TRUE;
		}
	| ',' expr ',' expr ',' style ',' style {
		$$ = new_control();
		$$->width = $2;
		$$->height = $4;
		$$->style = $6;
		$$->gotstyle = TRUE;
		$$->exstyle = $8;
		$$->gotexstyle = TRUE;
		}
	;

gen_ctrl: nameid_s opt_comma expr ',' ctlclass ',' style ',' expr ',' expr ',' expr ',' expr ',' style {
		$$=new_control();
		$$->title = $1;
		$$->id = $3;
		$$->ctlclass = convert_ctlclass($5);
		$$->style = $7;
		$$->gotstyle = TRUE;
		$$->x = $9;
		$$->y = $11;
		$$->width = $13;
		$$->height = $15;
		$$->exstyle = $17;
		$$->gotexstyle = TRUE;
		}
	| nameid_s opt_comma expr ',' ctlclass ',' style ',' expr ',' expr ',' expr ',' expr {
		$$=new_control();
		$$->title = $1;
		$$->id = $3;
		$$->ctlclass = convert_ctlclass($5);
		$$->style = $7;
		$$->gotstyle = TRUE;
		$$->x = $9;
		$$->y = $11;
		$$->width = $13;
		$$->height = $15;
		}
	;

opt_font
	: tFONT expr ',' tSTRING	{ $$ = new_font_id($2, $4, 0, 0); }
	;

/* ------------------------------ style flags ------------------------------ */
optional_style_pair
	: /* Empty */		{ $$ = NULL; }
	| ',' style		{ $$ = new_style_pair($2, 0); }
	| ',' style ',' style 	{ $$ = new_style_pair($2, $4); }
	;

style
	: style '|' style	{ $$ = new_style($1->or_mask | $3->or_mask, $1->and_mask | $3->and_mask); free($1); free($3);}
	| '(' style ')'		{ $$ = $2; }
        | any_num       	{ $$ = new_style($1, 0); }
        | tNOT any_num		{ $$ = new_style(0, $2); }
        ;

ctlclass
	: expr	{
		$$ = new_name_id();
		$$->type = name_ord;
		$$->name.i_name = $1;
		}
	| tSTRING {
		$$ = new_name_id();
		$$->type = name_str;
		$$->name.s_name = $1;
		}
	;

/* ------------------------------ DialogEx ------------------------------ */
dialogex: tDIALOGEX loadmemopts expr ',' expr ',' expr ',' expr helpid dlgex_attribs
	  tBEGIN  exctrls tEND {
		if(!win32)
			parser_warning("DIALOGEX not supported in 16-bit mode\n");
		if($2)
		{
			$11->memopt = *($2);
			free($2);
		}
		else
			$11->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
		$11->x = $3;
		$11->y = $5;
		$11->width = $7;
		$11->height = $9;
		if($10)
		{
			$11->helpid = *($10);
			$11->gothelpid = TRUE;
			free($10);
		}
		$11->controls = get_control_head($13);
		$$ = $11;

		assert($$->style != NULL);
		if(!$$->gotstyle)
		{
			$$->style->or_mask = WS_POPUP;
			$$->gotstyle = TRUE;
		}
		if($$->title)
			$$->style->or_mask |= WS_CAPTION;
		if($$->font)
			$$->style->or_mask |= DS_SETFONT;

		$$->style->or_mask &= ~($$->style->and_mask);
		$$->style->and_mask = 0;

		if(!$$->lvc.language)
			$$->lvc.language = dup_language(currentlanguage);
		}
	;

dlgex_attribs
	: /* Empty */				{ $$=new_dialogex(); }
	| dlgex_attribs tSTYLE style		{ $$=dialogex_style($3,$1); }
	| dlgex_attribs tEXSTYLE style		{ $$=dialogex_exstyle($3,$1); }
	| dlgex_attribs tCAPTION tSTRING	{ $$=dialogex_caption($3,$1); }
	| dlgex_attribs opt_font		{ $$=dialogex_font($2,$1); }
	| dlgex_attribs opt_exfont		{ $$=dialogex_font($2,$1); }
	| dlgex_attribs tCLASS nameid_s		{ $$=dialogex_class($3,$1); }
	| dlgex_attribs tMENU nameid		{ $$=dialogex_menu($3,$1); }
	| dlgex_attribs opt_language		{ $$=dialogex_language($2,$1); }
	| dlgex_attribs opt_characts		{ $$=dialogex_characteristics($2,$1); }
	| dlgex_attribs opt_version		{ $$=dialogex_version($2,$1); }
	;

exctrls	: /* Empty */				{ $$ = NULL; }
	| exctrls tCONTROL	gen_exctrl	{ $$=ins_ctrl(-1, 0, $3, $1); }
	| exctrls tEDITTEXT	exctrl_desc	{ $$=ins_ctrl(CT_EDIT, 0, $3, $1); }
	| exctrls tLISTBOX	exctrl_desc	{ $$=ins_ctrl(CT_LISTBOX, 0, $3, $1); }
	| exctrls tCOMBOBOX	exctrl_desc	{ $$=ins_ctrl(CT_COMBOBOX, 0, $3, $1); }
	| exctrls tSCROLLBAR	exctrl_desc	{ $$=ins_ctrl(CT_SCROLLBAR, 0, $3, $1); }
	| exctrls tCHECKBOX	lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_CHECKBOX, $3, $1); }
	| exctrls tDEFPUSHBUTTON lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_DEFPUSHBUTTON, $3, $1); }
	| exctrls tGROUPBOX	lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_GROUPBOX, $3, $1);}
	| exctrls tPUSHBUTTON	lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_PUSHBUTTON, $3, $1); }
/*	| exctrls tPUSHBOX	lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_PUSHBOX, $3, $1); } */
	| exctrls tRADIOBUTTON	lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_RADIOBUTTON, $3, $1); }
	| exctrls tAUTO3STATE	lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_AUTO3STATE, $3, $1); }
	| exctrls tSTATE3	lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_3STATE, $3, $1); }
	| exctrls tAUTOCHECKBOX	lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_AUTOCHECKBOX, $3, $1); }
	| exctrls tAUTORADIOBUTTON lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_AUTORADIOBUTTON, $3, $1); }
	| exctrls tLTEXT	lab_exctrl	{ $$=ins_ctrl(CT_STATIC, SS_LEFT, $3, $1); }
	| exctrls tCTEXT	lab_exctrl	{ $$=ins_ctrl(CT_STATIC, SS_CENTER, $3, $1); }
	| exctrls tRTEXT	lab_exctrl	{ $$=ins_ctrl(CT_STATIC, SS_RIGHT, $3, $1); }
	/* special treatment for icons, as the extent is optional */
	| exctrls tICON nameid_s opt_comma expr ',' expr ',' expr iconinfo {
		$10->title = $3;
		$10->id = $5;
		$10->x = $7;
		$10->y = $9;
		$$ = ins_ctrl(CT_STATIC, SS_ICON, $10, $1);
		}
	;

gen_exctrl
	: nameid_s opt_comma expr ',' ctlclass ',' style ',' expr ',' expr ',' expr ','
	  expr ',' style helpid opt_data {
		$$=new_control();
		$$->title = $1;
		$$->id = $3;
		$$->ctlclass = convert_ctlclass($5);
		$$->style = $7;
		$$->gotstyle = TRUE;
		$$->x = $9;
		$$->y = $11;
		$$->width = $13;
		$$->height = $15;
		if($17)
		{
			$$->exstyle = $17;
			$$->gotexstyle = TRUE;
		}
		if($18)
		{
			$$->helpid = *($18);
			$$->gothelpid = TRUE;
			free($18);
		}
		$$->extra = $19;
		}
	| nameid_s opt_comma expr ',' ctlclass ',' style ',' expr ',' expr ',' expr ',' expr opt_data {
		$$=new_control();
		$$->title = $1;
		$$->id = $3;
		$$->style = $7;
		$$->gotstyle = TRUE;
		$$->ctlclass = convert_ctlclass($5);
		$$->x = $9;
		$$->y = $11;
		$$->width = $13;
		$$->height = $15;
		$$->extra = $16;
		}
	;

lab_exctrl
	: nameid_s opt_comma expr ',' expr ',' expr ',' expr ',' expr optional_style_pair helpid opt_data {
		$$=new_control();
		$$->title = $1;
		$$->id = $3;
		$$->x = $5;
		$$->y = $7;
		$$->width = $9;
		$$->height = $11;
		if($12)
		{
			$$->style = $12->style;
			$$->gotstyle = TRUE;

			if ($12->exstyle)
			{
			    $$->exstyle = $12->exstyle;
			    $$->gotexstyle = TRUE;
			}
			free($12);
		}

		$$->extra = $14;
		}
	;

exctrl_desc
	: expr ',' expr ',' expr ',' expr ',' expr optional_style_pair helpid opt_data {
		$$ = new_control();
		$$->id = $1;
		$$->x = $3;
		$$->y = $5;
		$$->width = $7;
		$$->height = $9;
		if($10)
		{
			$$->style = $10->style;
			$$->gotstyle = TRUE;

			if ($10->exstyle)
			{
			    $$->exstyle = $10->exstyle;
			    $$->gotexstyle = TRUE;
			}
			free($10);
		}
		$$->extra = $12;
		}
	;

opt_data: /* Empty */	{ $$ = NULL; }
	| raw_data	{ $$ = $1; }
	;

helpid	: /* Empty */	{ $$ = NULL; }
	| ',' expr	{ $$ = new_int($2); }
	;

opt_exfont
	: tFONT expr ',' tSTRING ',' expr ',' expr  opt_expr { $$ = new_font_id($2, $4, $6, $8); }
	;

/*
 * FIXME: This odd expression is here to nullify an extra token found
 * in some appstudio produced resources which appear to do nothing.
 */
opt_expr: /* Empty */	{ $$ = NULL; }
	| ',' expr	{ $$ = NULL; }
	;

/* ------------------------------ Menu ------------------------------ */
menu	: tMENU loadmemopts opt_lvc menu_body {
		if(!$4)
			yyerror("Menu must contain items");
		$$ = new_menu();
		if($2)
		{
			$$->memopt = *($2);
			free($2);
		}
		else
			$$->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
		$$->items = get_item_head($4);
		if($3)
		{
			$$->lvc = *($3);
			free($3);
		}
		if(!$$->lvc.language)
			$$->lvc.language = dup_language(currentlanguage);
		}
	;

menu_body
	: tBEGIN item_definitions tEND	{ $$ = $2; }
	;

item_definitions
	: /* Empty */	{$$ = NULL;}
	| item_definitions tMENUITEM tSTRING opt_comma expr item_options {
		$$=new_menu_item();
		$$->prev = $1;
		if($1)
			$1->next = $$;
		$$->id =  $5;
		$$->state = $6;
		$$->name = $3;
		}
	| item_definitions tMENUITEM tSEPARATOR {
		$$=new_menu_item();
		$$->prev = $1;
		if($1)
			$1->next = $$;
		}
	| item_definitions tPOPUP tSTRING item_options menu_body {
		$$ = new_menu_item();
		$$->prev = $1;
		if($1)
			$1->next = $$;
		$$->popup = get_item_head($5);
		$$->name = $3;
		}
	;

/* NOTE: item_options is right recursive because it would introduce
 * a shift/reduce conflict on ',' in itemex_options due to the
 * empty rule here. The parser is now forced to look beyond the ','
 * before reducing (force shift).
 * Right recursion here is not a problem because we cannot expect
 * more than 7 parserstack places to be occupied while parsing this
 * (who would want to specify a MF_x flag twice?).
 */
item_options
	: /* Empty */				{ $$ = 0; }
	| opt_comma tCHECKED		item_options	{ $$ = $3 | MF_CHECKED; }
	| opt_comma tGRAYED		item_options	{ $$ = $3 | MF_GRAYED; }
	| opt_comma tHELP		item_options	{ $$ = $3 | MF_HELP; }
	| opt_comma tINACTIVE		item_options	{ $$ = $3 | MF_DISABLED; }
	| opt_comma tMENUBARBREAK	item_options	{ $$ = $3 | MF_MENUBARBREAK; }
	| opt_comma tMENUBREAK	item_options	{ $$ = $3 | MF_MENUBREAK; }
	;

/* ------------------------------ MenuEx ------------------------------ */
menuex	: tMENUEX loadmemopts opt_lvc menuex_body	{
		if(!win32)
			parser_warning("MENUEX not supported in 16-bit mode\n");
		if(!$4)
			yyerror("MenuEx must contain items");
		$$ = new_menuex();
		if($2)
		{
			$$->memopt = *($2);
			free($2);
		}
		else
			$$->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
		$$->items = get_itemex_head($4);
		if($3)
		{
			$$->lvc = *($3);
			free($3);
		}
		if(!$$->lvc.language)
			$$->lvc.language = dup_language(currentlanguage);
		}
	;

menuex_body
	: tBEGIN itemex_definitions tEND { $$ = $2; }
	;

itemex_definitions
	: /* Empty */	{$$ = NULL; }
	| itemex_definitions tMENUITEM tSTRING itemex_options {
		$$ = new_menuex_item();
		$$->prev = $1;
		if($1)
			$1->next = $$;
		$$->name = $3;
		$$->id = $4->id;
		$$->type = $4->type;
		$$->state = $4->state;
		$$->helpid = $4->helpid;
		$$->gotid = $4->gotid;
		$$->gottype = $4->gottype;
		$$->gotstate = $4->gotstate;
		$$->gothelpid = $4->gothelpid;
		free($4);
		}
	| itemex_definitions tMENUITEM tSEPARATOR {
		$$ = new_menuex_item();
		$$->prev = $1;
		if($1)
			$1->next = $$;
		}
	| itemex_definitions tPOPUP tSTRING itemex_p_options menuex_body {
		$$ = new_menuex_item();
		$$->prev = $1;
		if($1)
			$1->next = $$;
		$$->popup = get_itemex_head($5);
		$$->name = $3;
		$$->id = $4->id;
		$$->type = $4->type;
		$$->state = $4->state;
		$$->helpid = $4->helpid;
		$$->gotid = $4->gotid;
		$$->gottype = $4->gottype;
		$$->gotstate = $4->gotstate;
		$$->gothelpid = $4->gothelpid;
		free($4);
		}
	;

itemex_options
	: /* Empty */			{ $$ = new_itemex_opt(0, 0, 0, 0); }
	| ',' expr {
		$$ = new_itemex_opt($2, 0, 0, 0);
		$$->gotid = TRUE;
		}
	| ',' e_expr ',' e_expr item_options {
		$$ = new_itemex_opt($2 ? *($2) : 0, $4 ? *($4) : 0, $5, 0);
		$$->gotid = TRUE;
		$$->gottype = TRUE;
		$$->gotstate = TRUE;
		free($2);
		free($4);
		}
	| ',' e_expr ',' e_expr ',' expr {
		$$ = new_itemex_opt($2 ? *($2) : 0, $4 ? *($4) : 0, $6, 0);
		$$->gotid = TRUE;
		$$->gottype = TRUE;
		$$->gotstate = TRUE;
		free($2);
		free($4);
		}
	;

itemex_p_options
	: /* Empty */			{ $$ = new_itemex_opt(0, 0, 0, 0); }
	| ',' expr {
		$$ = new_itemex_opt($2, 0, 0, 0);
		$$->gotid = TRUE;
		}
	| ',' e_expr ',' expr {
		$$ = new_itemex_opt($2 ? *($2) : 0, $4, 0, 0);
		free($2);
		$$->gotid = TRUE;
		$$->gottype = TRUE;
		}
	| ',' e_expr ',' e_expr ',' expr {
		$$ = new_itemex_opt($2 ? *($2) : 0, $4 ? *($4) : 0, $6, 0);
		free($2);
		free($4);
		$$->gotid = TRUE;
		$$->gottype = TRUE;
		$$->gotstate = TRUE;
		}
	| ',' e_expr ',' e_expr ',' e_expr ',' expr {
		$$ = new_itemex_opt($2 ? *($2) : 0, $4 ? *($4) : 0, $6 ? *($6) : 0, $8);
		free($2);
		free($4);
		free($6);
		$$->gotid = TRUE;
		$$->gottype = TRUE;
		$$->gotstate = TRUE;
		$$->gothelpid = TRUE;
		}
	;

/* ------------------------------ StringTable ------------------------------ */
/* Stringtables are parsed differently than other resources because their
 * layout is substantially different from other resources.
 * The table is parsed through a _global_ variable 'tagstt' which holds the
 * current stringtable descriptor (stringtable_t *) and 'sttres' that holds a
 * list of stringtables of different languages.
 */
stringtable
	: stt_head tBEGIN strings tEND {
		if(!$3)
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

		$$ = tagstt;
		}
	;

/* This is to get the language of the currently parsed stringtable */
stt_head: tSTRINGTABLE loadmemopts opt_lvc {
		if((tagstt = find_stringtable($3)) == NULL)
			tagstt = new_stringtable($3);
		tagstt_memopt = $2;
		tagstt_version = $3->version;
		tagstt_characts = $3->characts;
		free($3);
		}
	;

strings	: /* Empty */	{ $$ = NULL; }
	| strings expr opt_comma tSTRING {
		int i;
		assert(tagstt != NULL);
		if($2 > 65535 || $2 < -32768)
			yyerror("Stringtable entry's ID out of range (%d)", $2);
		/* Search for the ID */
		for(i = 0; i < tagstt->nentries; i++)
		{
			if(tagstt->entries[i].id == $2)
				yyerror("Stringtable ID %d already in use", $2);
		}
		/* If we get here, then we have a new unique entry */
		tagstt->nentries++;
		tagstt->entries = xrealloc(tagstt->entries, sizeof(tagstt->entries[0]) * tagstt->nentries);
		tagstt->entries[tagstt->nentries-1].id = $2;
		tagstt->entries[tagstt->nentries-1].str = $4;
		if(tagstt_memopt)
			tagstt->entries[tagstt->nentries-1].memopt = *tagstt_memopt;
		else
			tagstt->entries[tagstt->nentries-1].memopt = WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE | WRC_MO_PURE;
		tagstt->entries[tagstt->nentries-1].version = tagstt_version;
		tagstt->entries[tagstt->nentries-1].characts = tagstt_characts;

		if(pedantic && !$4->size)
			parser_warning("Zero length strings make no sense\n");
		if(!win32 && $4->size > 254)
			yyerror("Stringtable entry more than 254 characters");
		if(win32 && $4->size > 65534) /* Hmm..., does this happen? */
			yyerror("Stringtable entry more than 65534 characters (probably something else that went wrong)");
		$$ = tagstt;
		}
	;

opt_comma	/* There seem to be two ways to specify a stringtable... */
	: /* Empty */
	| ','
	;

/* ------------------------------ VersionInfo ------------------------------ */
versioninfo
	: tVERSIONINFO loadmemopts fix_version tBEGIN ver_blocks tEND {
		$$ = $3;
		if($2)
		{
			$$->memopt = *($2);
			free($2);
		}
		else
			$$->memopt = WRC_MO_MOVEABLE | (win32 ? WRC_MO_PURE : 0);
		$$->blocks = get_ver_block_head($5);
		/* Set language; there is no version or characteristics */
		$$->lvc.language = dup_language(currentlanguage);
		}
	;

fix_version
	: /* Empty */			{ $$ = new_versioninfo(); }
	| fix_version tFILEVERSION expr ',' expr ',' expr ',' expr {
		if($1->gotit.fv)
			yyerror("FILEVERSION already defined");
		$$ = $1;
		$$->filever_maj1 = $3;
		$$->filever_maj2 = $5;
		$$->filever_min1 = $7;
		$$->filever_min2 = $9;
		$$->gotit.fv = 1;
		}
	| fix_version tPRODUCTVERSION expr ',' expr ',' expr ',' expr {
		if($1->gotit.pv)
			yyerror("PRODUCTVERSION already defined");
		$$ = $1;
		$$->prodver_maj1 = $3;
		$$->prodver_maj2 = $5;
		$$->prodver_min1 = $7;
		$$->prodver_min2 = $9;
		$$->gotit.pv = 1;
		}
	| fix_version tFILEFLAGS expr {
		if($1->gotit.ff)
			yyerror("FILEFLAGS already defined");
		$$ = $1;
		$$->fileflags = $3;
		$$->gotit.ff = 1;
		}
	| fix_version tFILEFLAGSMASK expr {
		if($1->gotit.ffm)
			yyerror("FILEFLAGSMASK already defined");
		$$ = $1;
		$$->fileflagsmask = $3;
		$$->gotit.ffm = 1;
		}
	| fix_version tFILEOS expr {
		if($1->gotit.fo)
			yyerror("FILEOS already defined");
		$$ = $1;
		$$->fileos = $3;
		$$->gotit.fo = 1;
		}
	| fix_version tFILETYPE expr {
		if($1->gotit.ft)
			yyerror("FILETYPE already defined");
		$$ = $1;
		$$->filetype = $3;
		$$->gotit.ft = 1;
		}
	| fix_version tFILESUBTYPE expr {
		if($1->gotit.fst)
			yyerror("FILESUBTYPE already defined");
		$$ = $1;
		$$->filesubtype = $3;
		$$->gotit.fst = 1;
		}
	;

ver_blocks
	: /* Empty */			{ $$ = NULL; }
	| ver_blocks ver_block {
		$$ = $2;
		$$->prev = $1;
		if($1)
			$1->next = $$;
		}
	;

ver_block
	: tBLOCK tSTRING tBEGIN ver_values tEND {
		$$ = new_ver_block();
		$$->name = $2;
		$$->values = get_ver_value_head($4);
		}
	;

ver_values
	: /* Empty */			{ $$ = NULL; }
	| ver_values ver_value {
		$$ = $2;
		$$->prev = $1;
		if($1)
			$1->next = $$;
		}
	;

ver_value
	: ver_block {
		$$ = new_ver_value();
		$$->type = val_block;
		$$->value.block = $1;
		}
	| tVALUE tSTRING ',' tSTRING {
		$$ = new_ver_value();
		$$->type = val_str;
		$$->key = $2;
		$$->value.str = $4;
		}
	| tVALUE tSTRING ',' ver_words {
		$$ = new_ver_value();
		$$->type = val_words;
		$$->key = $2;
		$$->value.words = $4;
		}
	;

ver_words
	: expr			{ $$ = new_ver_words($1); }
	| ver_words ',' expr	{ $$ = add_ver_words($1, $3); }
	;

/* ------------------------------ Toolbar ------------------------------ */
toolbar: tTOOLBAR loadmemopts expr ',' expr opt_lvc tBEGIN toolbar_items tEND {
		int nitems;
		toolbar_item_t *items = get_tlbr_buttons_head($8, &nitems);
		$$ = new_toolbar($3, $5, items, nitems);
		if($2)
		{
			$$->memopt = *($2);
			free($2);
		}
		else
		{
			$$->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE;
		}
		if($6)
		{
			$$->lvc = *($6);
			free($6);
		}
		if(!$$->lvc.language)
		{
			$$->lvc.language = dup_language(currentlanguage);
		}
		}
	;

toolbar_items
	:  /* Empty */			{ $$ = NULL; }
	| toolbar_items tBUTTON expr	{
		toolbar_item_t *idrec = new_toolbar_item();
		idrec->id = $3;
		$$ = ins_tlbr_button($1, idrec);
		}
	| toolbar_items tSEPARATOR	{
		toolbar_item_t *idrec = new_toolbar_item();
		idrec->id = 0;
		$$ = ins_tlbr_button($1, idrec);
	}
	;

/* ------------------------------ Memory options ------------------------------ */
loadmemopts
	: /* Empty */		{ $$ = NULL; }
	| loadmemopts lamo {
		if($1)
		{
			*($1) |= *($2);
			$$ = $1;
			free($2);
		}
		else
			$$ = $2;
		}
	| loadmemopts lama {
		if($1)
		{
			*($1) &= *($2);
			$$ = $1;
			free($2);
		}
		else
		{
			*$2 &= WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE | WRC_MO_PURE;
			$$ = $2;
		}
		}
	;

lamo	: tPRELOAD	{ $$ = new_int(WRC_MO_PRELOAD); }
	| tMOVEABLE	{ $$ = new_int(WRC_MO_MOVEABLE); }
	| tDISCARDABLE	{ $$ = new_int(WRC_MO_DISCARDABLE); }
	| tPURE		{ $$ = new_int(WRC_MO_PURE); }
	;

lama	: tLOADONCALL	{ $$ = new_int(~WRC_MO_PRELOAD); }
	| tFIXED	{ $$ = new_int(~WRC_MO_MOVEABLE); }
	| tIMPURE	{ $$ = new_int(~WRC_MO_PURE); }
	;

/* ------------------------------ Win32 options ------------------------------ */
opt_lvc	: /* Empty */		{ $$ = new_lvc(); }
	| opt_lvc opt_language {
		if(!win32)
			parser_warning("LANGUAGE not supported in 16-bit mode\n");
		if($1->language)
			yyerror("Language already defined");
		$$ = $1;
		$1->language = $2;
		}
	| opt_lvc opt_characts {
		if(!win32)
			parser_warning("CHARACTERISTICS not supported in 16-bit mode\n");
		if($1->characts)
			yyerror("Characteristics already defined");
		$$ = $1;
		$1->characts = $2;
		}
	| opt_lvc opt_version {
		if(!win32)
			parser_warning("VERSION not supported in 16-bit mode\n");
		if($1->version)
			yyerror("Version already defined");
		$$ = $1;
		$1->version = $2;
		}
	;

	/*
	 * This here is another s/r conflict on {bi,u}nary + and -.
	 * It is due to the look-ahead which must determine when the
	 * rule opt_language ends. It could be solved with adding a
	 * tNL at the end, but that seems unreasonable to do.
	 * The conflict is now moved to the expression handling below.
	 */
opt_language
	: tLANGUAGE expr ',' expr	{ $$ = new_language($2, $4);
					  if (get_language_codepage($2, $4) == -1)
						yyerror( "Language %04x is not supported", ($4<<10) + $2);
					}
	;

opt_characts
	: tCHARACTERISTICS expr		{ $$ = new_characts($2); }
	;

opt_version
	: tVERSION expr			{ $$ = new_version($2); }
	;

/* ------------------------------ Raw data handling ------------------------------ */
raw_data: opt_lvc tBEGIN raw_elements tEND {
		if($1)
		{
			$3->lvc = *($1);
			free($1);
		}

		if(!$3->lvc.language)
			$3->lvc.language = dup_language(currentlanguage);

		$$ = $3;
		}
	;

raw_elements
	: tRAWDATA			{ $$ = $1; }
	| tNUMBER			{ $$ = int2raw_data($1); }
	| '-' tNUMBER	           	{ $$ = int2raw_data(-($2)); }
	| tLNUMBER			{ $$ = long2raw_data($1); }
	| '-' tLNUMBER	           	{ $$ = long2raw_data(-($2)); }
	| tSTRING			{ $$ = str2raw_data($1); }
	| raw_elements opt_comma tRAWDATA { $$ = merge_raw_data($1, $3); free($3->data); free($3); }
	| raw_elements opt_comma tNUMBER  { $$ = merge_raw_data_int($1, $3); }
	| raw_elements opt_comma '-' tNUMBER  { $$ = merge_raw_data_int($1, -($4)); }
	| raw_elements opt_comma tLNUMBER { $$ = merge_raw_data_long($1, $3); }
	| raw_elements opt_comma '-' tLNUMBER { $$ = merge_raw_data_long($1, -($4)); }
	| raw_elements opt_comma tSTRING  { $$ = merge_raw_data_str($1, $3); }
	;

/* File data or raw data */
file_raw: filename	{ $$ = load_file($1,dup_language(currentlanguage)); }
	| raw_data	{ $$ = $1; }
	;

/* ------------------------------ Win32 expressions ------------------------------ */
/* All win16 numbers are also handled here. This is inconsistent with MS'
 * resource compiler, but what the heck, its just handy to have.
 */
e_expr	: /* Empty */	{ $$ = 0; }
	| expr		{ $$ = new_int($1); }
	;

/* This rule moves ALL s/r conflicts on {bi,u}nary - and + to here */
expr	: xpr	{ $$ = ($1); }
	;

xpr	: xpr '+' xpr	{ $$ = ($1) + ($3); }
	| xpr '-' xpr	{ $$ = ($1) - ($3); }
	| xpr '|' xpr	{ $$ = ($1) | ($3); }
	| xpr '&' xpr	{ $$ = ($1) & ($3); }
	| xpr '*' xpr	{ $$ = ($1) * ($3); }
	| xpr '/' xpr	{ $$ = ($1) / ($3); }
	| xpr '^' xpr	{ $$ = ($1) ^ ($3); }
	| '~' xpr	{ $$ = ~($2); }
	| '-' xpr %prec pUPM	{ $$ = -($2); }
	| '+' xpr %prec pUPM	{ $$ = $2; }
	| '(' xpr ')'	{ $$ = $2; }
	| any_num	{ $$ = $1; }
	| tNOT any_num	{ $$ = ~($2); }
	;

any_num	: tNUMBER	{ $$ = $1; }
	| tLNUMBER	{ $$ = $1; }
	;

%%
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
