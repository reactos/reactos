/*
 * Wine Message Compiler parser
 *
 * Copyright 2000 Bertho A. Stultiens (BS)
 *
 * The basic grammar of the file is yet another example of, humpf,
 * design. There is is mix of context-insensitive and -sentitive
 * stuff, which makes it rather complicated.
 * The header definitions are all context-insensitive because they have
 * delimited arguments, whereas the message headers are (semi-) context-
 * sensitive and the messages themselves are, well, RFC82[12] delimited.
 * This mixture seems to originate from the time that ms and ibm were
 * good friends and developing os/2 according to the "compatibility"
 * switch and reading some comments here and there.
 *
 * I'll ignore most of the complications and concentrate on the concept
 * which allows me to use yacc. Basically, everything is context-
 * insensitive now, with the exception of the message-text itself and
 * the preceding language declaration.
 *
 */

%{

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "config.h"

#include "utils.h"
#include "wmc.h"
#include "lang.h"

static const char err_syntax[]	= "Syntax error";
static const char err_number[]	= "Number expected";
static const char err_ident[]	= "Identifier expected";
static const char err_assign[]	= "'=' expected";
static const char err_popen[]	= "'(' expected";
static const char err_pclose[]	= "')' expected";
static const char err_colon[]	= "':' expected";
static const char err_msg[]	= "Message expected";

/* Scanner switches */
int want_nl = 0;		/* Request next newlinw */
int want_line = 0;		/* Request next complete line */
int want_file = 0;		/* Request next ident as filename */

node_t *nodehead = NULL;	/* The list of all parsed elements */
static node_t *nodetail = NULL;
lan_blk_t *lanblockhead;	/* List of parsed elements transposed */

static int base = 16;		/* Current printout base to use (8, 10 or 16) */
static WCHAR *cast = NULL;	/* Current typecast to use */

static int last_id = 0;		/* The last message ID parsed */
static int last_sev = 0;	/* Last severity code parsed */
static int last_fac = 0;	/* Last facility code parsed */
static WCHAR *last_sym = NULL;/* Last alias symbol parsed */
static int have_sev;		/* Set if severity parsed for current message */
static int have_fac;		/* Set if facility parsed for current message */
static int have_sym;		/* Set is symbol parsed for current message */

static cp_xlat_t *cpxlattab = NULL;	/* Codepage translation table */
static int ncpxlattab = 0;

/* Prototypes */
static WCHAR *merge(WCHAR *s1, WCHAR *s2);
static lanmsg_t *new_lanmsg(lan_cp_t *lcp, WCHAR *msg);
static msg_t *add_lanmsg(msg_t *msg, lanmsg_t *lanmsg);
static msg_t *complete_msg(msg_t *msg, int id);
static void add_node(node_e type, void *p);
static void do_add_token(tok_e type, token_t *tok, const char *code);
static void test_id(int id);
static int check_languages(node_t *head);
static lan_blk_t *block_messages(node_t *head);
static void add_cpxlat(int lan, int cpin, int cpout);
static cp_xlat_t *find_cpxlat(int lan);

%}


%union {
	WCHAR		*str;
	unsigned	num;
	token_t		*tok;
	lanmsg_t	*lmp;
	msg_t		*msg;
	lan_cp_t	lcp;
}


%token tSEVNAMES tFACNAMES tLANNAMES tBASE tCODEPAGE
%token tTYPEDEF tNL tSYMNAME tMSGEND
%token tSEVERITY tFACILITY tLANGUAGE tMSGID
%token <str> tIDENT tLINE tFILE tCOMMENT
%token <num> tNUMBER
%token <tok> tTOKEN

%type <str>	alias lines
%type <num>	optcp id msgid clan
%type <tok>	token
%type <lmp>	body
%type <msg>	bodies msg
%type <lcp>	lang

%%
file	: items	{
		if(!check_languages(nodehead))
			xyyerror("No messages defined");
		lanblockhead = block_messages(nodehead);
	}
	;

items	: decl
	| items decl
	;

decl	: global
	| msg		{ add_node(nd_msg, $1); }
	| tCOMMENT	{ add_node(nd_comment, $1); }
	| error		{ xyyerror(err_syntax); /* `Catch all' error */ }
	;

global	: tSEVNAMES '=' '(' smaps ')'
	| tSEVNAMES '=' '(' smaps error	{ xyyerror(err_pclose); }
	| tSEVNAMES '=' error		{ xyyerror(err_popen); }
	| tSEVNAMES error		{ xyyerror(err_assign); }
	| tFACNAMES '=' '(' fmaps ')'
	| tFACNAMES '=' '(' fmaps error	{ xyyerror(err_pclose); }
	| tFACNAMES '=' error		{ xyyerror(err_popen); }
	| tFACNAMES error		{ xyyerror(err_assign); }
	| tLANNAMES '=' '(' lmaps ')'
	| tLANNAMES '=' '(' lmaps error	{ xyyerror(err_pclose); }
	| tLANNAMES '=' error		{ xyyerror(err_popen); }
	| tLANNAMES error		{ xyyerror(err_assign); }
	| tCODEPAGE '=' '(' cmaps ')'
	| tCODEPAGE '=' '(' cmaps error	{ xyyerror(err_pclose); }
	| tCODEPAGE '=' error		{ xyyerror(err_popen); }
	| tCODEPAGE error		{ xyyerror(err_assign); }
	| tTYPEDEF '=' tIDENT		{ cast = $3; }
	| tTYPEDEF '=' error		{ xyyerror(err_number); }
	| tTYPEDEF error		{ xyyerror(err_assign); }
	| tBASE '=' tNUMBER		{
		switch(base)
		{
		case 8:
		case 10:
		case 16:
			base = $3;
			break;
		default:
			xyyerror("Numberbase must be 8, 10 or 16");
		}
	}
	| tBASE '=' error		{ xyyerror(err_number); }
	| tBASE error			{ xyyerror(err_assign); }
	;

/*----------------------------------------------------------------------
 * SeverityNames mapping
 */
smaps	: smap
	| smaps smap
	| error		{ xyyerror(err_ident); }
	;

smap	: token '=' tNUMBER alias {
		$1->token = $3;
		$1->alias = $4;
		if($3 & (~0x3))
			xyyerror("Severity value out of range (0x%08x > 0x3)", $3);
		do_add_token(tok_severity, $1, "severity");
	}
	| token '=' error	{ xyyerror(err_number); }
	| token error		{ xyyerror(err_assign); }
	;

/*----------------------------------------------------------------------
 * FacilityNames mapping
 */
fmaps	: fmap
	| fmaps fmap
	| error		{ xyyerror(err_ident); }
	;

fmap	: token '=' tNUMBER alias {
		$1->token = $3;
		$1->alias = $4;
		if($3 & (~0xfff))
			xyyerror("Facility value out of range (0x%08x > 0xfff)", $3);
		do_add_token(tok_facility, $1, "facility");
	}
	| token '=' error	{ xyyerror(err_number); }
	| token error		{ xyyerror(err_assign); }
	;

alias	: /* Empty */	{ $$ = NULL; }
	| ':' tIDENT	{ $$ = $2; }
	| ':' error	{ xyyerror(err_ident); }
	;

/*----------------------------------------------------------------------
 * LanguageNames mapping
 */
lmaps	: lmap
	| lmaps lmap
	| error		{ xyyerror(err_ident); }
	;

lmap	: token '=' tNUMBER setfile ':' tFILE optcp {
		$1->token = $3;
		$1->alias = $6;
		$1->codepage = $7;
		do_add_token(tok_language, $1, "language");
		if(!find_language($1->token) && !find_cpxlat($1->token))
			yywarning("Language 0x%x not built-in, using codepage %d; use explicit codepage to override", $1->token, WMC_DEFAULT_CODEPAGE);
	}
	| token '=' tNUMBER setfile ':' error	{ xyyerror("Filename expected"); }
	| token '=' tNUMBER error		{ xyyerror(err_colon); }
	| token '=' error			{ xyyerror(err_number); }
	| token error				{ xyyerror(err_assign); }
	;

optcp	: /* Empty */	{ $$ = 0; }
	| ':' tNUMBER	{ $$ = $2; }
	| ':' error	{ xyyerror("Codepage-number expected"); }
	;

/*----------------------------------------------------------------------
 * Codepages mapping
 */
cmaps	: cmap
	| cmaps cmap
	| error		{ xyyerror(err_ident); }
	;

cmap	: clan '=' tNUMBER ':' tNUMBER {
		static const char err_nocp[] = "Codepage %d not builtin; cannot convert";
		if(find_cpxlat($1))
			xyyerror("Codepage translation already defined for language 0x%x", $1);
		if($3 && !find_codepage($3))
			xyyerror(err_nocp, $3);
		if($5 && !find_codepage($5))
			xyyerror(err_nocp, $5);
		add_cpxlat($1, $3, $5);
	}
	| clan '=' tNUMBER ':' error	{ xyyerror(err_number); }
	| clan '=' tNUMBER error	{ xyyerror(err_colon); }
	| clan '=' error		{ xyyerror(err_number); }
	| clan error			{ xyyerror(err_assign); }
	;

clan	: tNUMBER	{ $$ = $1; }
	| tTOKEN	{
		if($1->type != tok_language)
			xyyerror("Language name or code expected");
		$$ = $1->token;
	}
	;

/*----------------------------------------------------------------------
 * Message-definition parsing
 */
msg	: msgid sevfacsym { test_id($1); } bodies	{ $$ = complete_msg($4, $1); }
	;

msgid	: tMSGID '=' id	{
		if($3 & (~0xffff))
			xyyerror("Message ID value out of range (0x%08x > 0xffff)", $3);
		$$ = $3;
	}
	| tMSGID error	{ xyyerror(err_assign); }
	;

id	: /* Empty */	{ $$ = ++last_id; }
	| tNUMBER	{ $$ = last_id = $1; }
	| '+' tNUMBER	{ $$ = last_id += $2; }
	| '+' error	{ xyyerror(err_number); }
	;

sevfacsym: /* Empty */	{ have_sev = have_fac = have_sym = 0; }
	| sevfacsym sev	{ if(have_sev) xyyerror("Severity already defined"); have_sev = 1; }
	| sevfacsym fac	{ if(have_fac) xyyerror("Facility already defined"); have_fac = 1; }
	| sevfacsym sym	{ if(have_sym) xyyerror("Symbolname already defined"); have_sym = 1; }
	;

sym	: tSYMNAME '=' tIDENT	{ last_sym = $3; }
	| tSYMNAME '=' error	{ xyyerror(err_ident); }
	| tSYMNAME error	{ xyyerror(err_assign); }
	;

sev	: tSEVERITY '=' token	{
		token_t *tok = lookup_token($3->name);
		if(!tok)
			xyyerror("Undefined severityname");
		if(tok->type != tok_severity)
			xyyerror("Identifier is not of class 'severity'");
		last_sev = tok->token;
	}
	| tSEVERITY '=' error	{ xyyerror(err_ident); }
	| tSEVERITY error	{ xyyerror(err_assign); }
	;

fac	: tFACILITY '=' token	{
		token_t *tok = lookup_token($3->name);
		if(!tok)
			xyyerror("Undefined facilityname");
		if(tok->type != tok_facility)
			xyyerror("Identifier is not of class 'facility'");
		last_fac = tok->token;
	}
	| tFACILITY '=' error	{ xyyerror(err_ident); }
	| tFACILITY error	{ xyyerror(err_assign); }
	;

/*----------------------------------------------------------------------
 * Message-text parsing
 */
bodies	: body		{ $$ = add_lanmsg(NULL, $1); }
	| bodies body	{ $$ = add_lanmsg($1, $2); }
	| error		{ xyyerror("'Language=...' (start of message text-definition) expected"); }
	;

body	: lang setline lines tMSGEND	{ $$ = new_lanmsg(&$1, $3); }
	;

	/*
	 * The newline is to be able to set the codepage
	 * to the language based codepage for the next
	 * message to be parsed.
	 */
lang	: tLANGUAGE setnl '=' token tNL	{
		token_t *tok = lookup_token($4->name);
		cp_xlat_t *cpx;
		if(!tok)
			xyyerror("Undefined language");
		if(tok->type != tok_language)
			xyyerror("Identifier is not of class 'language'");
		if((cpx = find_cpxlat(tok->token)))
		{
			set_codepage($$.codepage = cpx->cpin);
		}
		else if(!tok->codepage)
		{
			const language_t *lan = find_language(tok->token);
			if(!lan)
			{
				/* Just set default; warning was given while parsing languagenames */
				set_codepage($$.codepage = WMC_DEFAULT_CODEPAGE);
			}
			else
			{
				/* The default seems to be to use the DOS codepage... */
				set_codepage($$.codepage = lan->doscp);
			}
		}
		else
			set_codepage($$.codepage = tok->codepage);
		$$.language = tok->token;
	}
	| tLANGUAGE setnl '=' token error	{ xyyerror("Missing newline"); }
	| tLANGUAGE setnl '=' error		{ xyyerror(err_ident); }
	| tLANGUAGE error			{ xyyerror(err_assign); }
	;

lines	: tLINE		{ $$ = $1; }
	| lines tLINE	{ $$ = merge($1, $2); }
	| error		{ xyyerror(err_msg); }
	| lines error	{ xyyerror(err_msg); }
	;

/*----------------------------------------------------------------------
 * Helper rules
 */
token	: tIDENT	{ $$ = xmalloc(sizeof(token_t)); $$->name = $1; }
	| tTOKEN	{ $$ = $1; }
	;

setnl	: /* Empty */	{ want_nl = 1; }
	;

setline	: /* Empty */	{ want_line = 1; }
	;

setfile	: /* Empty */	{ want_file = 1; }
	;

%%

static WCHAR *merge(WCHAR *s1, WCHAR *s2)
{
	int l1 = unistrlen(s1);
	int l2 = unistrlen(s2);
	s1 = xrealloc(s1, (l1 + l2 + 1) * sizeof(*s1));
	unistrcpy(s1+l1, s2);
	free(s2);
	return s1;
}

static void do_add_token(tok_e type, token_t *tok, const char *code)
{
	token_t *tp = lookup_token(tok->name);
	if(tp)
	{
		if(tok->type != type)
			yywarning("Type change in token");
		if(tp != tok)
			xyyerror("Overlapping token not the same");
		/* else its already defined and changed */
		if(tok->fixed)
			xyyerror("Redefinition of %s", code);
		tok->fixed = 1;
	}
	else
	{
		add_token(type, tok->name, tok->token, tok->codepage, tok->alias, 1);
		free(tok);
	}
}

static lanmsg_t *new_lanmsg(lan_cp_t *lcp, WCHAR *msg)
{
	lanmsg_t *lmp = (lanmsg_t *)xmalloc(sizeof(lanmsg_t));
	lmp->lan = lcp->language;
	lmp->cp  = lcp->codepage;
	lmp->msg = msg;
	lmp->len = unistrlen(msg) + 1;	/* Include termination */
	if(lmp->len > 4096)
		yywarning("Message exceptionally long; might be a missing termination");
	return lmp;
}

static msg_t *add_lanmsg(msg_t *msg, lanmsg_t *lanmsg)
{
	int i;
	if(!msg)
		msg = xmalloc(sizeof(msg_t));
	msg->msgs = xrealloc(msg->msgs, (msg->nmsgs+1) * sizeof(*(msg->msgs)));
	msg->msgs[msg->nmsgs] = lanmsg;
	msg->nmsgs++;
	for(i = 0; i < msg->nmsgs-1; i++)
	{
		if(msg->msgs[i]->lan == lanmsg->lan)
			xyyerror("Message for language 0x%x already defined", lanmsg->lan);
	}
	return msg;
}

static int sort_lanmsg(const void *p1, const void *p2)
{
	return (*(lanmsg_t **)p1)->lan - (*(lanmsg_t **)p2)->lan;
}

static msg_t *complete_msg(msg_t *mp, int id)
{
	assert(mp != NULL);
	mp->id = id;
	if(have_sym)
		mp->sym = last_sym;
	else
		xyyerror("No symbolic name defined for message id %d", id);
	mp->sev = last_sev;
	mp->fac = last_fac;
	qsort(mp->msgs, mp->nmsgs, sizeof(*(mp->msgs)), sort_lanmsg);
	mp->realid = id | (last_sev << 30) | (last_fac << 16);
	if(custombit)
		mp->realid |= 1 << 29;
	mp->base = base;
	mp->cast = cast;
	return mp;
}

static void add_node(node_e type, void *p)
{
	node_t *ndp = (node_t *)xmalloc(sizeof(node_t));
	ndp->type = type;
	ndp->u.all = p;

	if(nodetail)
	{
		ndp->prev = nodetail;
		nodetail->next = ndp;
		nodetail = ndp;
	}
	else
	{
		nodehead = nodetail = ndp;
	}
}

static void test_id(int id)
{
	node_t *ndp;
	for(ndp = nodehead; ndp; ndp = ndp->next)
	{
		if(ndp->type != nd_msg)
			continue;
		if(ndp->u.msg->id == id && ndp->u.msg->sev == last_sev && ndp->u.msg->fac == last_fac)
			xyyerror("MessageId %d with facility 0x%x and severity 0x%x already defined", id, last_fac, last_sev);
	}
}

static int check_languages(node_t *head)
{
	static char err_missing[] = "Missing definition for language 0x%x; MessageID %d, facility 0x%x, severity 0x%x";
	node_t *ndp;
	int nm = 0;
	msg_t *msg = NULL;

	for(ndp = head; ndp; ndp = ndp->next)
	{
		if(ndp->type != nd_msg)
			continue;
		if(!nm)
		{
			msg = ndp->u.msg;
		}
		else
		{
			int i;
			msg_t *m1;
			msg_t *m2;
			if(ndp->u.msg->nmsgs > msg->nmsgs)
			{
				m1 = ndp->u.msg;
				m2 = msg;
			}
			else
			{
				m1 = msg;
				m2 = ndp->u.msg;
			}

			for(i = 0; i < m1->nmsgs; i++)
			{
				if(i > m2->nmsgs)
					error(err_missing, m1->msgs[i]->lan, m2->id, m2->fac, m2->sev);
				else if(m1->msgs[i]->lan < m2->msgs[i]->lan)
					error(err_missing, m1->msgs[i]->lan, m2->id, m2->fac, m2->sev);
				else if(m1->msgs[i]->lan > m2->msgs[i]->lan)
					error(err_missing, m2->msgs[i]->lan, m1->id, m1->fac, m1->sev);
			}
		}
		nm++;
	}
	return nm;
}

#define MSGRID(x)	((*(msg_t **)(x))->realid)
static int sort_msg(const void *p1, const void *p2)
{
	return MSGRID(p1) > MSGRID(p2) ? 1 : (MSGRID(p1) == MSGRID(p2) ? 0 : -1);
	/* return (*(msg_t **)p1)->realid - (*(msg_t **)p1)->realid; */
}

/*
 * block_messages() basically transposes the messages
 * from ID/language based list to a language/ID
 * based list.
 */
static lan_blk_t *block_messages(node_t *head)
{
	lan_blk_t *lbp;
	lan_blk_t *lblktail = NULL;
	lan_blk_t *lblkhead = NULL;
	msg_t **msgtab = NULL;
	node_t *ndp;
	int nmsg = 0;
	int i;
	int nl;
	int factor = unicodeout ? 2 : 1;

	for(ndp = head; ndp; ndp = ndp->next)
	{
		if(ndp->type != nd_msg)
			continue;
		msgtab = xrealloc(msgtab, (nmsg+1) * sizeof(*msgtab));
		msgtab[nmsg++] = ndp->u.msg;
	}

	assert(nmsg != 0);
	qsort(msgtab, nmsg, sizeof(*msgtab), sort_msg);

	for(nl = 0; nl < msgtab[0]->nmsgs; nl++)	/* This should be equal for all after check_languages() */
	{
		lbp = xmalloc(sizeof(lan_blk_t));

		if(!lblktail)
		{
			lblkhead = lblktail = lbp;
		}
		else
		{
			lblktail->next = lbp;
			lbp->prev = lblktail;
			lblktail = lbp;
		}
		lbp->nblk = 1;
		lbp->blks = xmalloc(sizeof(*lbp->blks));
		lbp->blks[0].idlo = msgtab[0]->realid;
		lbp->blks[0].idhi = msgtab[0]->realid;
		/* The plus 4 is the entry header; (+3)&~3 is DWORD alignment */
		lbp->blks[0].size = ((factor * msgtab[0]->msgs[nl]->len + 3) & ~3) + 4;
		lbp->blks[0].msgs = xmalloc(sizeof(*lbp->blks[0].msgs));
		lbp->blks[0].nmsg = 1;
		lbp->blks[0].msgs[0] = msgtab[0]->msgs[nl];
		lbp->lan = msgtab[0]->msgs[nl]->lan;

		for(i = 1; i < nmsg; i++)
		{
			block_t *blk = &(lbp->blks[lbp->nblk-1]);
			if(msgtab[i]->realid == blk->idhi+1)
			{
				blk->size += ((factor * msgtab[i]->msgs[nl]->len + 3) & ~3) + 4;
				blk->idhi++;
				blk->msgs = xrealloc(blk->msgs, (blk->nmsg+1) * sizeof(*blk->msgs));
				blk->msgs[blk->nmsg++] = msgtab[i]->msgs[nl];
			}
			else
			{
				lbp->nblk++;
				lbp->blks = xrealloc(lbp->blks, lbp->nblk * sizeof(*lbp->blks));
				blk = &(lbp->blks[lbp->nblk-1]);
				blk->idlo = msgtab[i]->realid;
				blk->idhi = msgtab[i]->realid;
				blk->size = ((factor * msgtab[i]->msgs[nl]->len + 3) & ~3) + 4;
				blk->msgs = xmalloc(sizeof(*blk->msgs));
				blk->nmsg = 1;
				blk->msgs[0] = msgtab[i]->msgs[nl];
			}
		}
	}
	free(msgtab);
	return lblkhead;
}

static int sc_xlat(const void *p1, const void *p2)
{
	return ((cp_xlat_t *)p1)->lan - ((cp_xlat_t *)p2)->lan;
}

static void add_cpxlat(int lan, int cpin, int cpout)
{
	cpxlattab = xrealloc(cpxlattab, (ncpxlattab+1) * sizeof(*cpxlattab));
	cpxlattab[ncpxlattab].lan   = lan;
	cpxlattab[ncpxlattab].cpin  = cpin;
	cpxlattab[ncpxlattab].cpout = cpout;
	ncpxlattab++;
	qsort(cpxlattab, ncpxlattab, sizeof(*cpxlattab), sc_xlat);
}

static cp_xlat_t *find_cpxlat(int lan)
{
	cp_xlat_t t;

	if(!cpxlattab) return NULL;

	t.lan = lan;
	return (cp_xlat_t *)bsearch(&t, cpxlattab, ncpxlattab, sizeof(*cpxlattab), sc_xlat);
}

