/*
 * Copyright 1998 Bertho A. Stultiens (BS)
 * Copyright 2002 Alexandre Julliard
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

#ifndef __WINE_WPP_PRIVATE_H
#define __WINE_WPP_PRIVATE_H

#include <stdio.h>
#include <string.h>
#include "wine/list.h"

extern void wpp_del_define( const char *name );
extern void wpp_add_cmdline_define( const char *value );
extern void wpp_set_debug( int lex_debug, int parser_debug, int msg_debug );
extern void wpp_add_include_path( const char *path );
extern char *wpp_find_include( const char *name, const char *parent_name );
/* Return value == 0 means successful execution */
extern int wpp_parse( const char *input, FILE *output );

struct pp_entry;	/* forward */
/*
 * Include logic
 * A stack of files which are already included and
 * are protected in the #ifndef/#endif way.
 */
typedef struct includelogicentry {
	struct list entry;
	struct pp_entry	*ppp;		/* The define which protects the file */
	char		*filename;	/* The filename of the include */
} includelogicentry_t;

/*
 * The expansiontext of a macro
 */
typedef enum {
	exp_text,	/* Simple text substitution */
	exp_concat,	/* Concat (##) operator requested */
	exp_stringize,	/* Stringize (#) operator requested */
	exp_subst	/* Substitute argument */
} def_exp_t;

typedef struct mtext {
	struct mtext	*next;
	struct mtext	*prev;
	def_exp_t	type;
	union {
		char	*text;
		int	argidx;		/* For exp_subst and exp_stringize reference */
	} subst;
} mtext_t;

/*
 * The define descriptor
 */
typedef enum {
	def_none,	/* Not-a-define; used as return value */
	def_define,	/* Simple defines */
	def_macro,	/* Macro defines */
	def_special	/* Special expansions like __LINE__ and __FILE__ */
} def_type_t;

typedef struct pp_entry {
	struct list entry;
	def_type_t	type;		/* Define or macro */
	char		*ident;		/* The key */
	char		**margs;	/* Macro arguments array or NULL if none */
	int		nargs;
	int             variadic;
	union {
		mtext_t	*mtext;		/* The substitution sequence or NULL if none */
		char	*text;
	} subst;
	int		expanding;	/* Set when feeding substitution into the input */
	char		*filename;	/* Filename where it was defined */
	int		linenumber;	/* Linenumber where it was defined */
	includelogicentry_t *iep;	/* Points to the include it protects */
} pp_entry_t;


/*
 * If logic
 */
#define MAXIFSTACK	64	/* If this isn't enough you should alter the source... */

typedef enum {
	if_false,
	if_true,
	if_elif,
	if_elsefalse,
	if_elsetrue,
	if_ignore,
	if_error
} pp_if_state_t;


/*
 * Trace the include files to prevent double reading.
 * This save 20..30% of processing time for most stuff
 * that uses complex includes.
 * States:
 * -1	Don't track or seen junk
 *  0	New include, waiting for "#ifndef __xxx_h"
 *  1	Seen #ifndef, waiting for "#define __xxx_h ..."
 *  2	Seen #endif, waiting for EOF
 */
typedef struct
{
    int state;
    char *ppp;             /* The define to be set from the #ifndef */
    int ifdepth;           /* The level of ifs at the #ifdef */
    int seen_junk;         /* Set when junk is seen */
} include_state_t;

#define SIZE_INT	1
#define SIZE_LONG	2
#define SIZE_LONGLONG	3
#define SIZE_MASK	0x00ff
#define FLAG_SIGNED	0x0100

typedef enum {
	cv_sint   = SIZE_INT + FLAG_SIGNED,
	cv_uint   = SIZE_INT,
	cv_slong  = SIZE_LONG + FLAG_SIGNED,
	cv_ulong  = SIZE_LONG,
	cv_sll    = SIZE_LONGLONG + FLAG_SIGNED,
	cv_ull    = SIZE_LONGLONG
} ctype_t;

typedef struct cval {
	ctype_t	type;
	union {
		int		si;
		unsigned int	ui;
		long		sl;
		unsigned long	ul;
		__int64		sll;
		unsigned __int64 ull;
	} val;
} cval_t;



pp_entry_t *pplookup(const char *ident);
pp_entry_t *pp_add_define(const char *def, const char *text);
pp_entry_t *pp_add_macro(char *ident, char *args[], int nargs, int variadic, mtext_t *exp);
void pp_del_define(const char *name);
void *pp_open_include(const char *name, int type, const char *parent_name, char **newpath);
void pp_push_if(pp_if_state_t s);
void pp_next_if_state(int);
pp_if_state_t pp_pop_if(void);
pp_if_state_t pp_if_state(void);
int pp_get_if_depth(void);

#ifdef __REACTOS__
#ifndef __GNUC__
#define __attribute__(x)  /*nothing*/
#endif
#endif

int ppy_error(const char *s, ...) __attribute__((format (printf, 1, 2)));
int ppy_warning(const char *s, ...) __attribute__((format (printf, 1, 2)));

/* current preprocessor state */
/* everything is in this structure to avoid polluting the global symbol space */
struct pp_status
{
    char *input;        /* current input file name */
    FILE *file;         /* current input file descriptor */
    int line_number;    /* current line number */
    int char_number;    /* current char number in line */
    int debug;          /* debug messages flag */
};

extern struct pp_status pp_status;
extern include_state_t pp_incl_state;
extern int pedantic;

/*
 * From ppl.l
 */
extern FILE *ppy_in;
extern FILE *ppy_out;
extern char *ppy_text;
extern int pp_flex_debug;
int ppy_lex(void);

void pp_do_include(char *fname, int type);
void pp_push_ignore_state(void);
void pp_pop_ignore_state(void);

/*
 * From ppy.y
 */
int ppy_parse(void);
extern int ppy_debug;

#endif  /* __WINE_WPP_PRIVATE_H */
